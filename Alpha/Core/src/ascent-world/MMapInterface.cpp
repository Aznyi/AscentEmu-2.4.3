#include "StdAfx.h"
#include <sys/stat.h>
#include "MMapInterface.h"
#include "MapMgr.h"
#include "../../extras/mmap/runtime/MoveMapSharedDefines.h"
#include "../../extras/third_party/recastnavigation/Detour/Include/DetourAlloc.h"
#include "../../extras/third_party/recastnavigation/Detour/Include/DetourCommon.h"
#include "../../extras/third_party/recastnavigation/Detour/Include/DetourNavMesh.h"
#include "../../extras/third_party/recastnavigation/Detour/Include/DetourNavMeshQuery.h"

namespace
{
        static const int MMAP_MAX_PATH_POLYS = 256;
        static const int MMAP_MAX_PATH_POINTS = 256;
        static const float MMAP_GRID_SIZE = 533.33333333f;

        struct ProjectedNavPoint
        {
                LocationVector originalWorld;
                LocationVector queryWorld;
                float originalNav[3];
                float queryNav[3];
                GroundHeightResult ground;
                bool usedResolvedGround;
                bool hasResolvedGround;

                ProjectedNavPoint() : usedResolvedGround(false), hasResolvedGround(false)
                {
                        memset(originalNav, 0, sizeof(originalNav));
                        memset(queryNav, 0, sizeof(queryNav));
                }
        };

        struct NearestPolyLookupResult
        {
                dtStatus status;
                dtPolyRef polyRef;
                float closest[3];
                float extents[3];
                bool usedProjectedPoint;
                bool usedExpandedExtents;
                const char* failureReason;

                NearestPolyLookupResult() : status(DT_FAILURE), polyRef(0), usedProjectedPoint(false), usedExpandedExtents(false), failureReason("no_poly")
                {
                        memset(closest, 0, sizeof(closest));
                        memset(extents, 0, sizeof(extents));
                }
        };

	bool MMapFileExists(const string& path)
	{
		struct _stat fileInfo;
		return (_stat(path.c_str(), &fileInfo) == 0);
	}

	string BuildMMapPath(const string& root, const string& relativeName)
	{
		string fullPath(root);
		if(!fullPath.empty() && fullPath[fullPath.length() - 1] != '/' && fullPath[fullPath.length() - 1] != '\\')
			fullPath += "/";

		fullPath += relativeName;
		return fullPath;
	}

        string FormatPolyRefChain(const dtPolyRef* refs, int count)
        {
                string chain;
                if(refs == NULL || count <= 0)
                        return chain;

                char buffer[48];
                for(int i = 0; i < count; ++i)
                {
                        if(i != 0)
                                chain += " -> ";
                        _snprintf(buffer, sizeof(buffer), I64FMT, (uint64)refs[i]);
                        buffer[sizeof(buffer) - 1] = 0;
                        chain += buffer;
                }

                return chain;
        }

        string FormatPolyRefTileChain(const dtNavMesh* navMesh, const dtPolyRef* refs, int count, int* outTileTransitions)
        {
                string chain;
                if(outTileTransitions != NULL)
                        *outTileTransitions = 0;
                if(navMesh == NULL || refs == NULL || count <= 0)
                        return chain;

                uint32 previousTileX = UINT_MAX;
                uint32 previousTileY = UINT_MAX;
                uint32 previousTileLayer = UINT_MAX;
                char buffer[96];
                for(int i = 0; i < count; ++i)
                {
                        const dtMeshTile* tile = NULL;
                        const dtPoly* poly = NULL;
                        if(dtStatusFailed(navMesh->getTileAndPolyByRef(refs[i], &tile, &poly)) || tile == NULL || tile->header == NULL)
                        {
                                _snprintf(buffer, sizeof(buffer), I64FMT "[invalid]", (uint64)refs[i]);
                                buffer[sizeof(buffer) - 1] = 0;
                        }
                        else
                        {
                                if(i != 0 &&
                                   (previousTileX != tile->header->x || previousTileY != tile->header->y || previousTileLayer != uint32(tile->header->layer)) &&
                                   outTileTransitions != NULL)
                                        ++(*outTileTransitions);

                                _snprintf(buffer, sizeof(buffer), I64FMT "[%u,%u,%d#%u]",
                                        (uint64)refs[i],
                                        uint32(tile->header->x),
                                        uint32(tile->header->y),
                                        int(tile->header->layer),
                                        uint32(poly - tile->polys));
                                buffer[sizeof(buffer) - 1] = 0;
                                previousTileX = tile->header->x;
                                previousTileY = tile->header->y;
                                previousTileLayer = uint32(tile->header->layer);
                        }

                        if(i != 0)
                                chain += " -> ";
                        chain += buffer;
                }

                return chain;
        }

        bool NearlyEqualPoints(const LocationVector& left, const LocationVector& right, float tolerance)
        {
		const float dx = left.x - right.x;
		const float dy = left.y - right.y;
		const float dz = left.z - right.z;
                return ((dx * dx) + (dy * dy) + (dz * dz)) <= (tolerance * tolerance);
        }

        bool NearlyEqualHorizontalPoints(const LocationVector& left, const LocationVector& right, float tolerance)
        {
		const float dx = left.x - right.x;
		const float dy = left.y - right.y;
                return ((dx * dx) + (dy * dy)) <= (tolerance * tolerance);
        }

        float ClampNearestPolyExtent(float value, float minimumValue, float maximumValue)
        {
                if(value < minimumValue)
                        return minimumValue;
                if(value > maximumValue)
                        return maximumValue;
                return value;
        }

        static const float MMAP_NEAREST_POLY_MAX_Z_DELTA = 8.0f;

        bool IsNearestPolyHeightAcceptable(const NearestPolyLookupResult& lookup, const ProjectedNavPoint& point, float maxZDelta)
        {
                if(lookup.polyRef == 0 || dtStatusFailed(lookup.status))
                        return false;

                const float referenceWorldZ = lookup.usedProjectedPoint ? point.queryWorld.z : point.originalWorld.z;
                const float closestWorldZ = lookup.closest[1];
                return fabsf(closestWorldZ - referenceWorldZ) <= maxZDelta;
        }

        bool IsNearestPolyProjectionCollisionSafe(uint32 mapId, Unit* mover, const ProjectedNavPoint& point, const NearestPolyLookupResult& lookup)
        {
#ifndef COLLISION
                (void)mapId;
                (void)mover;
                (void)point;
                (void)lookup;
                return true;
#else
                if(lookup.polyRef == 0 || dtStatusFailed(lookup.status))
                        return false;

                if(mover != NULL && mover->GetAIInterface() != NULL && mover->GetAIInterface()->IsFlying())
                        return true;

                LocationVector sourceWorld = lookup.usedProjectedPoint ? point.queryWorld : point.originalWorld;
                LocationVector closestWorld(lookup.closest[0], lookup.closest[2], lookup.closest[1]);
                const float dx = closestWorld.x - sourceWorld.x;
                const float dy = closestWorld.y - sourceWorld.y;
                const float dz = closestWorld.z - sourceWorld.z;
                const float horizontalDistanceSq = (dx * dx) + (dy * dy);
                if(horizontalDistanceSq <= (0.35f * 0.35f) && fabsf(dz) <= 1.0f)
                        return true;

                return CollideInterface.CheckLOS(mapId, sourceWorld, closestWorld);
#endif
        }

        bool IsNearestPolyProjectionAcceptable(uint32 mapId, Unit* mover, const ProjectedNavPoint& point, const NearestPolyLookupResult& lookup, float maxZDelta, const char** failureReason)
        {
                if(!IsNearestPolyHeightAcceptable(lookup, point, maxZDelta))
                {
                        if(failureReason != NULL)
                                *failureReason = "height_mismatch";
                        return false;
                }

                if(!IsNearestPolyProjectionCollisionSafe(mapId, mover, point, lookup))
                {
                        if(failureReason != NULL)
                                *failureReason = "projection_blocked";
                        return false;
                }

                if(failureReason != NULL)
                        *failureReason = "ok";
                return true;
        }

        const char* ClassifyEndpointProjectionFailure(const ProjectedNavPoint& point, const NearestPolyLookupResult& lookup, const char* failureReason)
        {
                const float rawZ = point.originalWorld.z;
                const float closestZ = lookup.closest[1];

                if(rawZ > (closestZ + 1.5f))
                        return "endpoint_upper_surface_missing_navmesh";
                if(rawZ < (closestZ - 1.5f))
                        return "endpoint_lower_surface_mismatch";

                if(failureReason != NULL && !strcmp(failureReason, "projection_blocked"))
                        return "endpoint_projection_blocked";
                if(failureReason != NULL && !strcmp(failureReason, "height_mismatch"))
                        return "endpoint_lower_surface_mismatch";

                return "endpoint_projection_blocked";
        }

        bool ShouldAllowEndpointPartial(const ProjectedNavPoint& point, const NearestPolyLookupResult& lookup, const char* classifiedReason)
        {
                if(classifiedReason == NULL)
                        return false;

                const float rawZ = point.originalWorld.z;
                const float closestZ = lookup.closest[1];
                const float dx = lookup.closest[0] - point.originalWorld.x;
                const float dy = lookup.closest[2] - point.originalWorld.y;
                const float xySnapDistance = sqrtf((dx * dx) + (dy * dy));

                if(strcmp(classifiedReason, "endpoint_lower_surface_mismatch") == 0)
                        return (closestZ > rawZ && xySnapDistance <= 2.0f);

                if(strcmp(classifiedReason, "endpoint_upper_surface_missing_navmesh") == 0)
                        return (xySnapDistance <= 2.5f);

                return false;
        }

        bool NormalizePathPoint(MapMgr* mapMgr, const char* reason, LocationVector& point)
        {
                if(mapMgr == NULL)
                        return true;

                const float rawZ = point.z;
                float normalizedZ = point.z;
                if(!mapMgr->NormalizeGroundPosition(point.x, point.y, point.z, &normalizedZ, NULL, reason, 8.0f))
                        return false;

                point.z = normalizedZ;
                if(sWorld.MMapDebugPathing && fabsf(point.z - rawZ) > 0.5f)
                {
                        sLog.outDebug("[MMAP][Z] reason=%s point=(%0.3f,%0.3f) raw=%0.3f validated=%0.3f",
                                reason != NULL ? reason : "normalize_path_point",
                                point.x, point.y, rawZ, point.z);
                }
                return true;
        }

        bool ValidatePathSegment(MapMgr* mapMgr, Unit* mover, const LocationVector& startPoint, const LocationVector& endPoint, string* detail, bool allowFirstSegmentDetourLosBlockedGroundOk = false)
        {
                if(mapMgr == NULL || mover == NULL || mover->IsPlayer())
                        return true;

                if(mover->GetAIInterface() != NULL && mover->GetAIInterface()->IsFlying())
                        return true;

#ifdef COLLISION
                LocationVector collisionStart(startPoint);
                LocationVector collisionEnd(endPoint);
                if(!CollideInterface.CheckLOS(mapMgr->GetMapId(), collisionStart, collisionEnd))
                {
                        const float zDelta = fabsf(endPoint.z - startPoint.z);
                        if(zDelta <= 2.0f)
                        {
                                DirectGroundPathResult directResult;
                                if(allowFirstSegmentDetourLosBlockedGroundOk &&
                                   mapMgr->ValidateDirectGroundPath(mover, startPoint.x, startPoint.y, startPoint.z, endPoint.x, endPoint.y, endPoint.z, &directResult, "mmap_path_segment"))
                                {
                                        if(sWorld.MMapDebugPathing)
                                        {
                                                sLog.outDebug("[MMAP][PATH] map=%u creature=%u decision=allow_first_segment_detour_los_blocked_ground_ok from=(%0.3f,%0.3f,%0.3f) to=(%0.3f,%0.3f,%0.3f)",
                                                        mapMgr->GetMapId(),
                                                        (mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(mover)->GetEntry() : 0,
                                                        startPoint.x, startPoint.y, startPoint.z,
                                                        endPoint.x, endPoint.y, endPoint.z);
                                        }
                                        return true;
                                }
                                // Small Z delta with blocked LOS: treat as real obstruction (wall, fence, etc.)
                                if(detail != NULL)
                                        *detail = "unsafe_segment_blocked_los";
                                return false;
                        }
                        // Large Z delta: the ray may be grazing a slope or ramp surface rather than
                        // hitting a genuine obstruction. Re-test with both endpoints raised by 1 unit.
                        // A slope surface clears because the lifted ray travels above it; a wall,
                        // building floor, or cliff face that truly blocks the path will still intercept
                        // the lifted ray, because lifting by 1 unit does not move the ray laterally.
                        LocationVector liftedStart(startPoint.x, startPoint.y, startPoint.z + 1.0f);
                        LocationVector liftedEnd(endPoint.x, endPoint.y, endPoint.z + 1.0f);
                        if(!CollideInterface.CheckLOS(mapMgr->GetMapId(), liftedStart, liftedEnd))
                        {
                                // Still blocked after lifting — genuine geometry obstruction, not a slope.
                                if(detail != NULL)
                                        *detail = "unsafe_segment_blocked_los";
                                return false;
                        }
                        // Lifted check passes: original failure was slope/terrain surface grazing.
                        // Fall through to ground-path validation for the final safety check.
                }
#endif

                DirectGroundPathResult directResult;
                if(mapMgr->ValidateDirectGroundPath(mover, startPoint.x, startPoint.y, startPoint.z, endPoint.x, endPoint.y, endPoint.z, &directResult, "mmap_path_segment"))
                        return true;

                if(detail != NULL)
                        *detail = string("unsafe_segment_") + GetDirectGroundPathStatusName(directResult.status);
                return false;
        }

	void LogPathingCreatureLine(const char* category, uint32 mapId, Unit* mover, const PathQueryRequest& request, const char* status)
	{
		if(!sWorld.MMapDebugPathing)
			return;

		const uint32 creatureEntry = (mover != NULL && mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(mover)->GetEntry() : 0;
		sLog.outDebug("[MMAP][%s] map=%u creature=%u guid=" I64FMT " start=(%0.3f,%0.3f,%0.3f) end=(%0.3f,%0.3f,%0.3f) status=%s",
			category,
			mapId,
			creatureEntry,
			(mover != NULL) ? mover->GetGUID() : 0,
			request.startX, request.startY, request.startZ,
			request.endX, request.endY, request.endZ,
			status ? status : "unknown");
	}
}

struct MMapInterface::RuntimeTileInfo
{
	dtTileRef tileRef;
	bool temporary;
	int headerX;
	int headerY;
	int headerLayer;
	int polyCount;
	float bmin[3];
	float bmax[3];

	RuntimeTileInfo() : tileRef(0), temporary(false), headerX(0), headerY(0), headerLayer(0), polyCount(0)
	{
		memset(bmin, 0, sizeof(bmin));
		memset(bmax, 0, sizeof(bmax));
	}

	RuntimeTileInfo(dtTileRef ref, bool isTemporary, const dtMeshHeader* header) : tileRef(ref), temporary(isTemporary), headerX(0), headerY(0), headerLayer(0), polyCount(0)
	{
		memset(bmin, 0, sizeof(bmin));
		memset(bmax, 0, sizeof(bmax));
		if(header != NULL)
		{
			headerX = header->x;
			headerY = header->y;
			headerLayer = header->layer;
			polyCount = header->polyCount;
			memcpy(bmin, header->bmin, sizeof(bmin));
			memcpy(bmax, header->bmax, sizeof(bmax));
		}
	}
};

struct MMapInterface::RuntimeMapData
{
	dtNavMesh* navMesh;
	map<uint32, RuntimeTileInfo> loadedTiles;
	map<uint32, dtNavMeshQuery*> queries;

	RuntimeMapData() : navMesh(NULL) {}
};

MMapInterface sMMapInterface;

MMapInterface::MMapInterface() : m_rootAccessible(false)
{
}

MMapInterface::~MMapInterface()
{
	DeInit();
}

void MMapInterface::Init()
{
	DeInit();

	m_rootPath = sWorld.MMapPath;
	m_rootAccessible = HasMMapDirectory();

	Log.Notice("MMapInterface", "Init");
	Log.Notice("MMapInterface", "Using mmap path: %s", m_rootPath.c_str());

	if(!IsPathingEnabled())
		Log.Notice("MMapInterface", "MMap pathing is disabled. Creatures will use direct movement fallback.");
	else if(!m_rootAccessible)
		Log.Error("MMapInterface", "mmap root directory does not exist or is not accessible: %s", m_rootPath.c_str());
	else
		Log.Notice("MMapInterface", "Detour navmesh backend is available. mmap path requests will use routed queries when compatible data is present.");
}

void MMapInterface::DeInit()
{
	m_lock.Acquire();
	for(map<uint32, RuntimeMapData*>::iterator itr = m_runtimeMaps.begin(); itr != m_runtimeMaps.end(); ++itr)
		FreeRuntimeMapData(itr->second);
	m_runtimeMaps.clear();
	m_tileRefCounts.clear();
	m_lock.Release();
}

PathQueryResult MMapInterface::BuildPath(MapMgr* mapMgr, const PathQueryRequest& request)
{
	PathQueryResult result;
	result.attemptedMMap = true;
	result.hadRuntimeBackend = HasRuntimeBackend();

	if(!IsPathingEnabled())
	{
		result.status = PATH_QUERY_STATUS_NAVMESH_UNAVAILABLE;
		result.detail = "disabled";
		return result;
	}

	if(!m_rootAccessible)
	{
		result.status = PATH_QUERY_STATUS_NAVMESH_UNAVAILABLE;
		result.detail = "root_missing";
		LogQueryResult(request, result, 0, 0, 0, 0);
		return result;
	}

	uint32 startTileX = 0;
	uint32 startTileY = 0;
	uint32 endTileX = 0;
	uint32 endTileY = 0;
	if(mapMgr == NULL || !ResolveTileCoords(mapMgr, request.startX, request.startY, startTileX, startTileY) || !ResolveTileCoords(mapMgr, request.endX, request.endY, endTileX, endTileY))
	{
		result.status = PATH_QUERY_STATUS_NAVMESH_UNAVAILABLE;
		result.detail = "tile_coords_unavailable";
		LogQueryResult(request, result, startTileX, startTileY, endTileX, endTileY);
		return result;
	}

	string detail;
	result.hadMapData = ValidateMapFile(request.mapId, &detail);
	if(!result.hadMapData)
	{
		result.status = (detail.find("malformed") != string::npos) ? PATH_QUERY_STATUS_MALFORMED_DATA : PATH_QUERY_STATUS_MISSING_DATA;
		result.detail = detail.empty() ? "map_missing" : detail;
		LogQueryResult(request, result, startTileX, startTileY, endTileX, endTileY);
		return result;
	}

	string startTileDetail;
	const bool hasStartTile = ValidateTileFile(request.mapId, startTileX, startTileY, &startTileDetail);
	string endTileDetail;
	const bool hasEndTile = ValidateTileFile(request.mapId, endTileX, endTileY, &endTileDetail);
	result.hadTileData = hasStartTile && hasEndTile;
	if(!result.hadTileData)
	{
		result.detail = hasStartTile ? endTileDetail : startTileDetail;
		result.status = (result.detail.find("malformed") != string::npos) ? PATH_QUERY_STATUS_MALFORMED_DATA : PATH_QUERY_STATUS_MISSING_DATA;
		LogQueryResult(request, result, startTileX, startTileY, endTileX, endTileY);
		return result;
	}

	bool releaseStartTile = false;
	bool releaseEndTile = false;
	std::vector<uint32> temporaryTiles;

	m_lock.Acquire();

	const uint64 startKey = BuildTileRefKey(request.mapId, startTileX, startTileY);
	const uint64 endKey = BuildTileRefKey(request.mapId, endTileX, endTileY);
	releaseStartTile = (m_tileRefCounts.find(startKey) == m_tileRefCounts.end());
	releaseEndTile = (m_tileRefCounts.find(endKey) == m_tileRefCounts.end());

	if(!EnsureTileLoaded(request.mapId, startTileX, startTileY, releaseStartTile, &detail))
	{
		m_lock.Release();
		result.status = (detail.find("malformed") != string::npos) ? PATH_QUERY_STATUS_MALFORMED_DATA : PATH_QUERY_STATUS_MISSING_DATA;
		result.detail = detail;
		LogQueryResult(request, result, startTileX, startTileY, endTileX, endTileY);
		return result;
	}

	if((startTileX != endTileX || startTileY != endTileY) && !EnsureTileLoaded(request.mapId, endTileX, endTileY, releaseEndTile, &detail))
	{
		if(releaseStartTile)
			ReleaseTemporaryTile(request.mapId, startTileX, startTileY);

		m_lock.Release();
		result.status = (detail.find("malformed") != string::npos) ? PATH_QUERY_STATUS_MALFORMED_DATA : PATH_QUERY_STATUS_MISSING_DATA;
		result.detail = detail;
		LogQueryResult(request, result, startTileX, startTileY, endTileX, endTileY);
		return result;
	}

	if(!EnsurePathTilesLoaded(request.mapId, startTileX, startTileY, endTileX, endTileY, &temporaryTiles, &detail))
	{
		ReleaseTemporaryTiles(request.mapId, temporaryTiles);
		if(releaseStartTile)
			ReleaseTemporaryTile(request.mapId, startTileX, startTileY);
		if(releaseEndTile && (startTileX != endTileX || startTileY != endTileY))
			ReleaseTemporaryTile(request.mapId, endTileX, endTileY);

		m_lock.Release();
		result.status = (detail.find("malformed") != string::npos) ? PATH_QUERY_STATUS_MALFORMED_DATA : PATH_QUERY_STATUS_MISSING_DATA;
		result.detail = detail;
		LogQueryResult(request, result, startTileX, startTileY, endTileX, endTileY);
		return result;
	}

        result = QueryPath(mapMgr, request.mapId, request.instanceId, request);

	ReleaseTemporaryTiles(request.mapId, temporaryTiles);
	if(releaseStartTile)
		ReleaseTemporaryTile(request.mapId, startTileX, startTileY);
	if(releaseEndTile && (startTileX != endTileX || startTileY != endTileY))
		ReleaseTemporaryTile(request.mapId, endTileX, endTileY);

	m_lock.Release();

	LogQueryResult(request, result, startTileX, startTileY, endTileX, endTileY);
	return result;
}

void MMapInterface::ActivateTile(uint32 mapId, uint32 tileX, uint32 tileY)
{
	if(tileX >= 64 || tileY >= 64)
		return;

	const uint64 key = BuildTileRefKey(mapId, tileX, tileY);
	m_lock.Acquire();
	uint32& refCount = m_tileRefCounts[key];
	if(refCount == 0 && IsPathingEnabled())
	{
		if(!m_rootAccessible)
			Log.Error("MMapInterface", "activateTile map=%u tile=%u,%u failed because mmap root is missing: %s", mapId, tileX, tileY, m_rootPath.c_str());
		else
		{
			string detail;
			if(EnsureTileLoaded(mapId, tileX, tileY, false, &detail))
			{
				if(sWorld.MMapLogTileLoads)
					Log.Notice("MMapInterface", "activateTile map=%u tile=%u,%u file=%s", mapId, tileX, tileY, BuildTileFilePath(mapId, tileX, tileY).c_str());
			}
			else
				Log.Error("MMapInterface", "activateTile map=%u tile=%u,%u failed: %s", mapId, tileX, tileY, detail.c_str());
		}
	}

	++refCount;
	m_lock.Release();
}

void MMapInterface::DeactivateTile(uint32 mapId, uint32 tileX, uint32 tileY)
{
	if(tileX >= 64 || tileY >= 64)
		return;

	const uint64 key = BuildTileRefKey(mapId, tileX, tileY);
	m_lock.Acquire();
	map<uint64, uint32>::iterator itr = m_tileRefCounts.find(key);
	if(itr == m_tileRefCounts.end() || itr->second == 0)
	{
		Log.Error("MMapInterface", "DeactivateTile underflow for map %u tile %u,%u", mapId, tileX, tileY);
		m_lock.Release();
		return;
	}

	--itr->second;
	if(itr->second == 0)
	{
		m_tileRefCounts.erase(itr);
		UnloadTile(mapId, tileX, tileY, false);
		if(sWorld.MMapLogTileLoads)
			Log.Notice("MMapInterface", "deactivateTile map=%u tile=%u,%u", mapId, tileX, tileY);
	}

	m_lock.Release();
}

uint32 MMapInterface::GetTileLoadRefCount(uint32 mapId, uint32 tileX, uint32 tileY) const
{
	if(tileX >= 64 || tileY >= 64)
		return 0;

	m_lock.Acquire();
	map<uint64, uint32>::const_iterator itr = m_tileRefCounts.find(BuildTileRefKey(mapId, tileX, tileY));
	const uint32 refCount = (itr == m_tileRefCounts.end()) ? 0 : itr->second;
	m_lock.Release();
	return refCount;
}

bool MMapInterface::HasMMapDirectory() const
{
	return MMapFileExists(sWorld.MMapPath);
}

bool MMapInterface::HasMapData(uint32 mapId) const
{
	return ValidateMapFile(mapId, NULL);
}

bool MMapInterface::HasTileData(uint32 mapId, uint32 tileX, uint32 tileY) const
{
	return ValidateTileFile(mapId, tileX, tileY, NULL);
}

bool MMapInterface::HasRuntimeBackend() const
{
	return true;
}

bool MMapInterface::IsPathingEnabled() const
{
	return sWorld.MMapPathingEnabled;
}

uint64 MMapInterface::BuildTileRefKey(uint32 mapId, uint32 tileX, uint32 tileY) const
{
	return (((uint64)mapId) << 16) | (((uint64)tileX) << 8) | (uint64)tileY;
}

uint32 MMapInterface::BuildPackedTileID(uint32 tileX, uint32 tileY) const
{
	return (tileX << 16) | tileY;
}

bool MMapInterface::ResolveTileCoords(MapMgr* mapMgr, float x, float y, uint32& tileX, uint32& tileY) const
{
	if(mapMgr == NULL)
		return false;

	tileX = mapMgr->GetPosX(x) / 8;
	tileY = mapMgr->GetPosY(y) / 8;
	return (tileX < 64 && tileY < 64);
}

void MMapInterface::ToNavMeshCoords(float worldX, float worldY, float worldZ, float* out) const
{
	out[0] = worldX;
	out[1] = worldZ;
	out[2] = worldY;
}

LocationVector MMapInterface::ToWorldCoords(const float* navPoint) const
{
	return LocationVector(navPoint[0], navPoint[2], navPoint[1]);
}

bool MMapInterface::ValidateMapFile(uint32 mapId, string* detail /* = NULL */) const
{
	const string path = BuildMapFilePath(mapId);
	FILE* fileHandle = fopen(path.c_str(), "rb");
	if(fileHandle == NULL)
	{
		if(detail != NULL)
			*detail = "map_missing";
		return false;
	}

	dtNavMeshParams params;
	memset(&params, 0, sizeof(params));
	const size_t bytesRead = fread(&params, 1, sizeof(params), fileHandle);
	fclose(fileHandle);

	if(bytesRead < sizeof(params) || params.maxTiles == 0 || params.maxPolys == 0)
	{
		if(detail != NULL)
			*detail = "map_malformed";
		return false;
	}

	if(detail != NULL)
		*detail = "ok";
	return true;
}

bool MMapInterface::ValidateTileFile(uint32 mapId, uint32 tileX, uint32 tileY, string* detail /* = NULL */) const
{
	const string path = BuildTileFilePath(mapId, tileX, tileY);
	FILE* fileHandle = fopen(path.c_str(), "rb");
	if(fileHandle == NULL)
	{
		if(detail != NULL)
			*detail = "tile_missing";
		return false;
	}

	MmapTileHeader header;
	memset(&header, 0, sizeof(header));
	const size_t bytesRead = fread(&header, 1, sizeof(header), fileHandle);
	fclose(fileHandle);

	if(bytesRead < sizeof(header) || header.mmapMagic != MMAP_MAGIC || header.mmapVersion != MMAP_VERSION || header.dtVersion != DT_NAVMESH_VERSION || header.size == 0)
	{
		if(detail != NULL)
			*detail = "tile_malformed";
		return false;
	}

	if(detail != NULL)
		*detail = "ok";
	return true;
}

bool MMapInterface::EnsureMapLoaded(uint32 mapId, string* detail /* = NULL */)
{
	if(m_runtimeMaps.find(mapId) != m_runtimeMaps.end())
	{
		if(detail != NULL)
			*detail = "ok";
		return true;
	}

	const string filePath = BuildMapFilePath(mapId);
	FILE* fileHandle = fopen(filePath.c_str(), "rb");
	if(fileHandle == NULL)
	{
		if(detail != NULL)
			*detail = "map_missing";
		return false;
	}

	dtNavMeshParams params;
	memset(&params, 0, sizeof(params));
	if(fread(&params, sizeof(params), 1, fileHandle) != 1)
	{
		fclose(fileHandle);
		if(detail != NULL)
			*detail = "map_malformed";
		return false;
	}
	fclose(fileHandle);

	dtNavMesh* navMesh = dtAllocNavMesh();
	if(navMesh == NULL)
	{
		if(detail != NULL)
			*detail = "map_alloc_failed";
		return false;
	}

	const dtStatus initStatus = navMesh->init(&params);
	if(dtStatusFailed(initStatus))
	{
		const int tileBits = dtIlog2(dtNextPow2((unsigned int)dtMax(params.maxTiles, 1)));
		const int polyBits = dtIlog2(dtNextPow2((unsigned int)dtMax(params.maxPolys, 1)));
		const int saltBits = 32 - tileBits - polyBits;
		dtFreeNavMesh(navMesh);
		if(detail != NULL)
			*detail = "map_init_failed";
		Log.Error("MMapInterface", "Failed to initialize navmesh for map %u from %s (status=0x%08X maxTiles=%u maxPolys=%u tileBits=%d polyBits=%d saltBits=%d)",
			mapId, filePath.c_str(), initStatus, params.maxTiles, params.maxPolys, tileBits, polyBits, saltBits);
		return false;
	}

	RuntimeMapData* data = new RuntimeMapData();
	data->navMesh = navMesh;
	m_runtimeMaps[mapId] = data;

	if(detail != NULL)
		*detail = "ok";

	if(sWorld.MMapLogTileLoads)
		Log.Notice("MMapInterface", "Loaded mmap root file map=%u file=%s", mapId, filePath.c_str());
	return true;
}

bool MMapInterface::EnsureTileLoaded(uint32 mapId, uint32 tileX, uint32 tileY, bool temporaryLoad, string* detail /* = NULL */)
{
	if(!EnsureMapLoaded(mapId, detail))
		return false;

	RuntimeMapData* data = m_runtimeMaps[mapId];
	const uint32 packedTile = BuildPackedTileID(tileX, tileY);
	map<uint32, RuntimeTileInfo>::iterator loadedTile = data->loadedTiles.find(packedTile);
	if(loadedTile != data->loadedTiles.end())
	{
		if(!temporaryLoad)
			loadedTile->second.temporary = false;

		if(detail != NULL)
			*detail = "ok";
		return true;
	}

	const string filePath = BuildTileFilePath(mapId, tileX, tileY);
	FILE* fileHandle = fopen(filePath.c_str(), "rb");
	if(fileHandle == NULL)
	{
		if(detail != NULL)
			*detail = "tile_missing";
		return false;
	}

	MmapTileHeader header;
	memset(&header, 0, sizeof(header));
	if(fread(&header, sizeof(header), 1, fileHandle) != 1)
	{
		fclose(fileHandle);
		if(detail != NULL)
			*detail = "tile_malformed";
		return false;
	}

	if(header.mmapMagic != MMAP_MAGIC || header.mmapVersion != MMAP_VERSION || header.dtVersion != DT_NAVMESH_VERSION || header.size == 0)
	{
		fclose(fileHandle);
		if(detail != NULL)
			*detail = "tile_malformed";
		Log.Error("MMapInterface", "Malformed mmap tile map=%u tile=%u,%u file=%s", mapId, tileX, tileY, filePath.c_str());
		return false;
	}

	unsigned char* tileData = (unsigned char*)dtAlloc(header.size, DT_ALLOC_PERM);
	if(tileData == NULL)
	{
		fclose(fileHandle);
		if(detail != NULL)
			*detail = "tile_alloc_failed";
		return false;
	}

	if(fread(tileData, header.size, 1, fileHandle) != 1)
	{
		fclose(fileHandle);
		dtFree(tileData);
		if(detail != NULL)
			*detail = "tile_malformed";
		return false;
	}

	fclose(fileHandle);

	dtTileRef tileRef = 0;
	const dtStatus addTileStatus = data->navMesh->addTile(tileData, header.size, DT_TILE_FREE_DATA, 0, &tileRef);
        if(dtStatusFailed(addTileStatus))
        {
                dtFree(tileData);
                if(detail != NULL)
                        *detail = "tile_add_failed";
                Log.Error("MMapInterface", "Failed to add mmap tile map=%u tile=%u,%u file=%s (status=0x%08X)", mapId, tileX, tileY, filePath.c_str(), addTileStatus);
                return false;
        }

        const dtMeshTile* addedTile = data->navMesh->getTileByRef(tileRef);
        const dtMeshHeader* meshHeader = (addedTile != NULL) ? addedTile->header : NULL;
        if(meshHeader == NULL || meshHeader->polyCount <= 0 || meshHeader->vertCount <= 0 || meshHeader->layer < 0)
        {
                data->navMesh->removeTile(tileRef, NULL, NULL);
                if(detail != NULL)
                        *detail = "tile_header_invalid";
                Log.Error("MMapInterface", "Loaded mmap tile has invalid Detour header map=%u tile=%u,%u file=%s", mapId, tileX, tileY, filePath.c_str());
                return false;
        }

        data->loadedTiles[packedTile] = RuntimeTileInfo(tileRef, temporaryLoad, meshHeader);
        if(detail != NULL)
                *detail = "ok";

        if(sWorld.MMapLogTileLoads)
        {
                Log.Notice("MMapInterface", "MMap tile header map=%u tile=%u,%u file=%s header=%d,%d,%d polys=%d verts=%d",
                        mapId, tileX, tileY, filePath.c_str(), meshHeader->x, meshHeader->y, meshHeader->layer, meshHeader->polyCount, meshHeader->vertCount);
        }

        return true;
}

bool MMapInterface::EnsurePathTilesLoaded(uint32 mapId, uint32 startTileX, uint32 startTileY, uint32 endTileX, uint32 endTileY, std::vector<uint32>* temporaryTiles, string* detail /* = NULL */)
{
	const uint32 minTileX = (startTileX < endTileX) ? startTileX : endTileX;
	const uint32 maxTileX = (startTileX > endTileX) ? startTileX : endTileX;
	const uint32 minTileY = (startTileY < endTileY) ? startTileY : endTileY;
	const uint32 maxTileY = (startTileY > endTileY) ? startTileY : endTileY;

	for(uint32 tileX = minTileX; tileX <= maxTileX; ++tileX)
	{
		for(uint32 tileY = minTileY; tileY <= maxTileY; ++tileY)
		{
			if((tileX == startTileX && tileY == startTileY) || (tileX == endTileX && tileY == endTileY))
				continue;

			const uint64 tileKey = BuildTileRefKey(mapId, tileX, tileY);
			const bool temporaryLoad = (m_tileRefCounts.find(tileKey) == m_tileRefCounts.end());
			string tileDetail;
			if(!EnsureTileLoaded(mapId, tileX, tileY, temporaryLoad, &tileDetail))
			{
				if(detail != NULL)
					*detail = tileDetail.empty() ? "tile_missing" : tileDetail;
				return false;
			}

			if(temporaryLoad && temporaryTiles != NULL)
				temporaryTiles->push_back(BuildPackedTileID(tileX, tileY));
		}
	}

	if(detail != NULL)
		*detail = "ok";
	return true;
}

void MMapInterface::ReleaseTemporaryTile(uint32 mapId, uint32 tileX, uint32 tileY)
{
	RuntimeMapData* data = NULL;
	map<uint32, RuntimeMapData*>::iterator mapItr = m_runtimeMaps.find(mapId);
	if(mapItr == m_runtimeMaps.end())
		return;

	data = mapItr->second;
	const uint32 packedTile = BuildPackedTileID(tileX, tileY);
	map<uint32, RuntimeTileInfo>::iterator tileItr = data->loadedTiles.find(packedTile);
	if(tileItr == data->loadedTiles.end() || !tileItr->second.temporary)
		return;

	if(m_tileRefCounts.find(BuildTileRefKey(mapId, tileX, tileY)) == m_tileRefCounts.end())
		UnloadTile(mapId, tileX, tileY, false);
}

void MMapInterface::ReleaseTemporaryTiles(uint32 mapId, const std::vector<uint32>& temporaryTiles)
{
	for(size_t i = 0; i < temporaryTiles.size(); ++i)
	{
		const uint32 packedTile = temporaryTiles[i];
		const uint32 tileX = (packedTile >> 16) & 0xFFFF;
		const uint32 tileY = packedTile & 0xFFFF;
		ReleaseTemporaryTile(mapId, tileX, tileY);
	}
}

bool MMapInterface::UnloadTile(uint32 mapId, uint32 tileX, uint32 tileY, bool logFailure)
{
	map<uint32, RuntimeMapData*>::iterator mapItr = m_runtimeMaps.find(mapId);
	if(mapItr == m_runtimeMaps.end())
		return false;

	RuntimeMapData* data = mapItr->second;
	const uint32 packedTile = BuildPackedTileID(tileX, tileY);
	map<uint32, RuntimeTileInfo>::iterator tileItr = data->loadedTiles.find(packedTile);
	if(tileItr == data->loadedTiles.end())
		return false;

	const dtStatus removeStatus = data->navMesh->removeTile(tileItr->second.tileRef, NULL, NULL);
	if(dtStatusFailed(removeStatus))
	{
		if(logFailure)
			Log.Error("MMapInterface", "Failed to unload mmap tile map=%u tile=%u,%u (status=0x%08X)", mapId, tileX, tileY, removeStatus);
		return false;
	}

	data->loadedTiles.erase(tileItr);
	return true;
}

NavMeshInspectionResult MMapInterface::InspectPoint(MapMgr* mapMgr, uint32 mapId, uint32 instanceId, float x, float y, float z, float radius)
{
	NavMeshInspectionResult result;
	result.attempted = true;
	result.originalWorld = LocationVector(x, y, z);
	result.resolvedWorld = result.originalWorld;
	result.radius = radius;

	uint32 tileX = 0;
	uint32 tileY = 0;
	if(!ResolveTileCoords(mapMgr, x, y, tileX, tileY))
	{
		result.detail = "tile_coords_unavailable";
		return result;
	}

	result.tileX = tileX;
	result.tileY = tileY;
	result.hasMapData = HasMapData(mapId);
	result.hasTileData = HasTileData(mapId, tileX, tileY);
	if(!result.hasMapData)
	{
		result.detail = "map_missing";
		return result;
	}

	if(!result.hasTileData)
	{
		result.detail = "tile_missing";
		return result;
	}

	string detail;
	if(!EnsureTileLoaded(mapId, tileX, tileY, true, &detail))
	{
		result.detail = detail.empty() ? "tile_load_failed" : detail;
		return result;
	}

	result.tileLoaded = true;

	map<uint32, RuntimeMapData*>::iterator mapItr = m_runtimeMaps.find(mapId);
	if(mapItr == m_runtimeMaps.end() || mapItr->second == NULL || mapItr->second->navMesh == NULL)
	{
		result.detail = "map_not_loaded";
		ReleaseTemporaryTile(mapId, tileX, tileY);
		return result;
	}

	RuntimeMapData* data = mapItr->second;
	const uint32 packedTile = BuildPackedTileID(tileX, tileY);
	map<uint32, RuntimeTileInfo>::const_iterator tileItr = data->loadedTiles.find(packedTile);
	if(tileItr != data->loadedTiles.end())
	{
		result.tileHeaderX = tileItr->second.headerX;
		result.tileHeaderY = tileItr->second.headerY;
		result.tileHeaderLayer = tileItr->second.headerLayer;
		result.tilePolyCount = tileItr->second.polyCount;
	}

	dtNavMeshQuery* query = NULL;
	map<uint32, dtNavMeshQuery*>::iterator queryItr = data->queries.find(instanceId);
	if(queryItr == data->queries.end())
	{
		query = dtAllocNavMeshQuery();
		if(query == NULL)
		{
			result.detail = "query_alloc_failed";
			ReleaseTemporaryTile(mapId, tileX, tileY);
			return result;
		}

		const dtStatus initStatus = query->init(data->navMesh, 2048);
		if(dtStatusFailed(initStatus))
		{
			dtFreeNavMeshQuery(query);
			result.detail = "query_init_failed";
			ReleaseTemporaryTile(mapId, tileX, tileY);
			return result;
		}

		data->queries[instanceId] = query;
	}
	else
		query = queryItr->second;

	if(mapMgr != NULL)
	{
		GroundHeightResult ground = mapMgr->ResolveGroundHeight(x, y, z, false);
		if(ground.valid)
			result.resolvedWorld.z = ground.z;
	}

	dtQueryFilter filter;
	filter.setIncludeFlags(0xFFFF);
	filter.setExcludeFlags(0);

	const float horizontalExtent = ClampNearestPolyExtent(sWorld.mmap_nearest_poly_extent_horizontal, 2.0f, 30.0f);
	const float verticalExtent = ClampNearestPolyExtent(sWorld.mmap_nearest_poly_extent_vertical, 4.0f, 60.0f);
	const float inspectHorizontalExtent = (horizontalExtent > radius) ? horizontalExtent : radius;
	const float inspectVerticalExtent = (verticalExtent > radius) ? verticalExtent : radius;
	const float extents[3] = { inspectHorizontalExtent, inspectVerticalExtent, inspectHorizontalExtent };

	float navPoint[3];
	ToNavMeshCoords(result.resolvedWorld.x, result.resolvedWorld.y, result.resolvedWorld.z, navPoint);

	dtPolyRef nearestRef = 0;
	float nearestClosest[3] = { 0.0f, 0.0f, 0.0f };
	const dtStatus nearestStatus = query->findNearestPoly(navPoint, extents, &filter, &nearestRef, nearestClosest);
	if(!dtStatusFailed(nearestStatus) && nearestRef != 0)
	{
		result.nearestPolyFound = true;
		result.nearestPolyRef = (uint64)nearestRef;
		result.closestWorld = ToWorldCoords(nearestClosest);
	}

	static const int MAX_INSPECT_POLYS = 128;
	dtPolyRef nearbyPolys[MAX_INSPECT_POLYS];
	int nearbyPolyCount = 0;
	memset(nearbyPolys, 0, sizeof(nearbyPolys));
	if(!dtStatusFailed(query->queryPolygons(navPoint, extents, &filter, nearbyPolys, &nearbyPolyCount, MAX_INSPECT_POLYS)))
		result.nearbyPolyCount = nearbyPolyCount;

	result.valid = true;
	result.detail = result.nearestPolyFound ? "nearest_poly_found" : "no_nearby_poly";
	ReleaseTemporaryTile(mapId, tileX, tileY);
	return result;
}

void MMapInterface::FreeRuntimeMapData(RuntimeMapData* data)
{
	if(data == NULL)
		return;

	for(map<uint32, dtNavMeshQuery*>::iterator itr = data->queries.begin(); itr != data->queries.end(); ++itr)
		dtFreeNavMeshQuery(itr->second);
	data->queries.clear();

	if(data->navMesh != NULL)
		dtFreeNavMesh(data->navMesh);

	delete data;
}

PathQueryResult MMapInterface::QueryPath(MapMgr* mapMgr, uint32 mapId, uint32 instanceId, const PathQueryRequest& request)
{
        PathQueryResult result;
	result.attemptedMMap = true;
	result.hadRuntimeBackend = true;
	result.hadMapData = true;
	result.hadTileData = true;

	map<uint32, RuntimeMapData*>::iterator mapItr = m_runtimeMaps.find(mapId);
	if(mapItr == m_runtimeMaps.end() || mapItr->second == NULL || mapItr->second->navMesh == NULL)
	{
		result.status = PATH_QUERY_STATUS_NAVMESH_UNAVAILABLE;
		result.detail = "map_not_loaded";
		return result;
	}

	RuntimeMapData* data = mapItr->second;
	dtNavMeshQuery* query = NULL;
	map<uint32, dtNavMeshQuery*>::iterator queryItr = data->queries.find(instanceId);
	if(queryItr == data->queries.end())
	{
		query = dtAllocNavMeshQuery();
		if(query == NULL)
		{
			result.status = PATH_QUERY_STATUS_NAVMESH_UNAVAILABLE;
			result.detail = "query_alloc_failed";
			return result;
		}

		const dtStatus initStatus = query->init(data->navMesh, 2048);
		if(dtStatusFailed(initStatus))
		{
			dtFreeNavMeshQuery(query);
			result.status = PATH_QUERY_STATUS_NAVMESH_UNAVAILABLE;
			result.detail = "query_init_failed";
			Log.Error("MMapInterface", "Failed to initialize dtNavMeshQuery for map=%u instance=%u status=0x%08X", mapId, instanceId, initStatus);
			return result;
		}

		data->queries[instanceId] = query;
	}
	else
		query = queryItr->second;

        dtQueryFilter filter;
        filter.setIncludeFlags(0xFFFF);
        filter.setExcludeFlags(0);

        const float horizontalExtent = ClampNearestPolyExtent(sWorld.mmap_nearest_poly_extent_horizontal, 2.0f, 30.0f);
        const float verticalExtent = ClampNearestPolyExtent(sWorld.mmap_nearest_poly_extent_vertical, 4.0f, 60.0f);
        const float baseExtents[3] = { horizontalExtent, verticalExtent, horizontalExtent };
        const float expandedExtents[3] = { horizontalExtent * 2.0f, verticalExtent * 2.0f, horizontalExtent * 2.0f };
        ProjectedNavPoint projectedStart;
        projectedStart.originalWorld = LocationVector(request.startX, request.startY, request.startZ);
        projectedStart.queryWorld = projectedStart.originalWorld;
        ToNavMeshCoords(request.startX, request.startY, request.startZ, projectedStart.originalNav);

        if(mapMgr != NULL)
        {
                projectedStart.ground = mapMgr->ResolveGroundHeight(request.startX, request.startY, request.startZ, false);
                projectedStart.hasResolvedGround = projectedStart.ground.valid;
                if(projectedStart.ground.valid)
                {
                        projectedStart.queryWorld.z = projectedStart.ground.z;
                        projectedStart.usedResolvedGround = fabsf(projectedStart.ground.z - request.startZ) > 0.05f;
                }
        }
        ToNavMeshCoords(projectedStart.queryWorld.x, projectedStart.queryWorld.y, projectedStart.queryWorld.z, projectedStart.queryNav);

        ProjectedNavPoint projectedEnd;
        projectedEnd.originalWorld = LocationVector(request.endX, request.endY, request.endZ);
        projectedEnd.queryWorld = projectedEnd.originalWorld;
        ToNavMeshCoords(request.endX, request.endY, request.endZ, projectedEnd.originalNav);

        if(mapMgr != NULL)
        {
                projectedEnd.ground = mapMgr->ResolveGroundHeight(request.endX, request.endY, request.endZ, false);
                projectedEnd.hasResolvedGround = projectedEnd.ground.valid;
                if(projectedEnd.ground.valid)
                {
                        projectedEnd.queryWorld.z = projectedEnd.ground.z;
                        projectedEnd.usedResolvedGround = fabsf(projectedEnd.ground.z - request.endZ) > 0.05f;
                }
        }
        ToNavMeshCoords(projectedEnd.queryWorld.x, projectedEnd.queryWorld.y, projectedEnd.queryWorld.z, projectedEnd.queryNav);

        bool forceEndpointPartial = false;
        const char* endpointProjectionReason = NULL;

        if(sWorld.MMapDebugPathing)
        {
                LogPathingCreatureLine("MOVE", request.mapId, request.mover, request, request.reason ? request.reason : "query");
                sLog.outDebug("[MMAP][MOVE] map=%u creature=%u start_source=%s end_source=%s extents=(%0.3f,%0.3f,%0.3f) expanded=(%0.3f,%0.3f,%0.3f)",
                        request.mapId,
                        (request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
                        GetGroundHeightSourceName(projectedStart.ground.source),
                        GetGroundHeightSourceName(projectedEnd.ground.source),
                        baseExtents[0], baseExtents[1], baseExtents[2],
                        expandedExtents[0], expandedExtents[1], expandedExtents[2]);
        }

        NearestPolyLookupResult startLookup;
        startLookup.usedProjectedPoint = true;
        memcpy(startLookup.extents, baseExtents, sizeof(startLookup.extents));
        startLookup.status = query->findNearestPoly(projectedStart.queryNav, baseExtents, &filter, &startLookup.polyRef, startLookup.closest);
        if(dtStatusFailed(startLookup.status) || startLookup.polyRef == 0)
        {
                startLookup.usedExpandedExtents = true;
                memcpy(startLookup.extents, expandedExtents, sizeof(startLookup.extents));
                startLookup.status = query->findNearestPoly(projectedStart.queryNav, expandedExtents, &filter, &startLookup.polyRef, startLookup.closest);
        }
        if((dtStatusFailed(startLookup.status) || startLookup.polyRef == 0) && projectedStart.usedResolvedGround)
        {
                startLookup.usedProjectedPoint = false;
                startLookup.usedExpandedExtents = false;
                memcpy(startLookup.extents, baseExtents, sizeof(startLookup.extents));
                startLookup.status = query->findNearestPoly(projectedStart.originalNav, baseExtents, &filter, &startLookup.polyRef, startLookup.closest);
                if(dtStatusFailed(startLookup.status) || startLookup.polyRef == 0)
                {
                        startLookup.usedExpandedExtents = true;
                        memcpy(startLookup.extents, expandedExtents, sizeof(startLookup.extents));
                        startLookup.status = query->findNearestPoly(projectedStart.originalNav, expandedExtents, &filter, &startLookup.polyRef, startLookup.closest);
                }
        }

        if(dtStatusFailed(startLookup.status) || startLookup.polyRef == 0)
        {
                result.status = PATH_QUERY_STATUS_NOPATH;
                result.detail = "no_start_poly";
                if(sWorld.MMapDebugPathing)
                {
                        sLog.outDebug("[MMAP][FALLBACK] map=%u creature=%u reason=no_start_poly status=0x%08X projected=%u expanded=%u extents=(%0.3f,%0.3f,%0.3f)",
                                request.mapId,
                                (request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
                                startLookup.status, startLookup.usedProjectedPoint ? 1 : 0, startLookup.usedExpandedExtents ? 1 : 0,
                                startLookup.extents[0], startLookup.extents[1], startLookup.extents[2]);
                }
                return result;
        }

        if(!IsNearestPolyHeightAcceptable(startLookup, projectedStart, MMAP_NEAREST_POLY_MAX_Z_DELTA))
        {
                result.status = PATH_QUERY_STATUS_NOPATH;
                result.detail = "no_start_poly";
                if(sWorld.MMapDebugPathing)
                {
                        const float referenceZ = startLookup.usedProjectedPoint ? projectedStart.queryWorld.z : projectedStart.originalWorld.z;
                        sLog.outDebug("[MMAP][FALLBACK] map=%u creature=%u reason=no_start_poly detail=height_mismatch closestZ=%0.3f referenceZ=%0.3f delta=%0.3f extents=(%0.3f,%0.3f,%0.3f)",
                                request.mapId,
                                (request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
                                startLookup.closest[1], referenceZ, fabsf(startLookup.closest[1] - referenceZ),
                                startLookup.extents[0], startLookup.extents[1], startLookup.extents[2]);
                }
                return result;
        }

        if(sWorld.MMapDebugPathing)
        {
                const LocationVector& startReference = startLookup.usedProjectedPoint ? projectedStart.queryWorld : projectedStart.originalWorld;
                sLog.outDebug("[MMAP][POLY] map=%u creature=%u which=start ref=" I64FMT " query=(%0.3f,%0.3f,%0.3f) closest=(%0.3f,%0.3f,%0.3f) projected=%u expanded=%u",
                        request.mapId,
                        (request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
                        (uint64)startLookup.polyRef,
                        startReference.x, startReference.y, startReference.z,
                        startLookup.closest[0], startLookup.closest[2], startLookup.closest[1],
                        startLookup.usedProjectedPoint ? 1 : 0,
                        startLookup.usedExpandedExtents ? 1 : 0);
                const float snapStartDx = startLookup.closest[0] - startReference.x;
                const float snapStartDy = startLookup.closest[2] - startReference.y;
                sLog.outDebug("[MMAP][SNAP] map=%u creature=%u which=start xy_dist=%0.3f z_delta=%0.3f",
                        request.mapId,
                        (request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
                        sqrtf((snapStartDx * snapStartDx) + (snapStartDy * snapStartDy)),
                        fabsf(startLookup.closest[1] - startReference.z));
        }

        NearestPolyLookupResult endLookup;
        endLookup.usedProjectedPoint = true;
        memcpy(endLookup.extents, baseExtents, sizeof(endLookup.extents));
        endLookup.status = query->findNearestPoly(projectedEnd.queryNav, baseExtents, &filter, &endLookup.polyRef, endLookup.closest);
        if(dtStatusFailed(endLookup.status) || endLookup.polyRef == 0)
        {
                        endLookup.usedExpandedExtents = true;
                        memcpy(endLookup.extents, expandedExtents, sizeof(endLookup.extents));
                        endLookup.status = query->findNearestPoly(projectedEnd.queryNav, expandedExtents, &filter, &endLookup.polyRef, endLookup.closest);
        }
        if((dtStatusFailed(endLookup.status) || endLookup.polyRef == 0) && projectedEnd.usedResolvedGround)
        {
                endLookup.usedProjectedPoint = false;
                endLookup.usedExpandedExtents = false;
                memcpy(endLookup.extents, baseExtents, sizeof(endLookup.extents));
                endLookup.status = query->findNearestPoly(projectedEnd.originalNav, baseExtents, &filter, &endLookup.polyRef, endLookup.closest);
                if(dtStatusFailed(endLookup.status) || endLookup.polyRef == 0)
                {
                        endLookup.usedExpandedExtents = true;
                        memcpy(endLookup.extents, expandedExtents, sizeof(endLookup.extents));
                        endLookup.status = query->findNearestPoly(projectedEnd.originalNav, expandedExtents, &filter, &endLookup.polyRef, endLookup.closest);
                }
        }

        const char* endProjectionFailure = "no_poly";
        if(!dtStatusFailed(endLookup.status) && endLookup.polyRef != 0)
                IsNearestPolyProjectionAcceptable(request.mapId, request.mover, projectedEnd, endLookup, MMAP_NEAREST_POLY_MAX_Z_DELTA, &endProjectionFailure);

        if(dtStatusFailed(endLookup.status) || endLookup.polyRef == 0)
        {
                result.status = PATH_QUERY_STATUS_NOPATH;
                result.detail = "no_end_poly";
                if(sWorld.MMapDebugPathing)
                {
                        sLog.outDebug("[MMAP][FALLBACK] map=%u creature=%u reason=no_end_poly status=0x%08X projected=%u expanded=%u extents=(%0.3f,%0.3f,%0.3f) rawZ=%0.3f terrainZ=%0.3f vmapZ=%0.3f resolvedZ=%0.3f",
                                request.mapId,
                                (request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
                                endLookup.status, endLookup.usedProjectedPoint ? 1 : 0, endLookup.usedExpandedExtents ? 1 : 0,
                                endLookup.extents[0], endLookup.extents[1], endLookup.extents[2],
                                projectedEnd.originalWorld.z, projectedEnd.ground.terrainZ, projectedEnd.ground.vmapZ, projectedEnd.queryWorld.z);
                }
                return result;
        }

        if(!IsNearestPolyProjectionAcceptable(request.mapId, request.mover, projectedEnd, endLookup, MMAP_NEAREST_POLY_MAX_Z_DELTA, &endProjectionFailure))
        {
                endpointProjectionReason = ClassifyEndpointProjectionFailure(projectedEnd, endLookup, endProjectionFailure);
                forceEndpointPartial = request.allowPartial && ShouldAllowEndpointPartial(projectedEnd, endLookup, endpointProjectionReason);
                if(!forceEndpointPartial)
                {
                        result.status = PATH_QUERY_STATUS_NOPATH;
                        result.detail = endpointProjectionReason;
                }
                if(sWorld.MMapDebugPathing)
                {
                        const LocationVector& endReference = endLookup.usedProjectedPoint ? projectedEnd.queryWorld : projectedEnd.originalWorld;
                        const float rawZ = projectedEnd.originalWorld.z;
                        const float closestZ = endLookup.closest[1];
                        const float snapEndDx = endLookup.closest[0] - endReference.x;
                        const float snapEndDy = endLookup.closest[2] - endReference.y;
                        const float snapDistance = sqrtf((snapEndDx * snapEndDx) + (snapEndDy * snapEndDy));
                        const char* relation = (rawZ > closestZ + 1.5f) ? "above_navmesh" : ((rawZ < closestZ - 1.5f) ? "below_navmesh" : "aligned_navmesh");
                        sLog.outDebug("[MMAP][FALLBACK] map=%u creature=%u reason=no_end_poly detail=%s classified=%s rawZ=%0.3f terrainZ=%0.3f vmapZ=%0.3f referenceZ=%0.3f closestZ=%0.3f snapXY=%0.3f z_delta=%0.3f relation=%s extents=(%0.3f,%0.3f,%0.3f) partial=%u",
                                request.mapId,
                                (request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
                                endProjectionFailure,
                                endpointProjectionReason != NULL ? endpointProjectionReason : "endpoint_projection_blocked",
                                rawZ, projectedEnd.ground.terrainZ, projectedEnd.ground.vmapZ,
                                endReference.z, closestZ, snapDistance, fabsf(closestZ - endReference.z),
                                relation,
                                endLookup.extents[0], endLookup.extents[1], endLookup.extents[2],
                                forceEndpointPartial ? 1 : 0);
                }
                if(!forceEndpointPartial)
                        return result;
        }

        if(sWorld.MMapDebugPathing)
        {
                const LocationVector& endReference = endLookup.usedProjectedPoint ? projectedEnd.queryWorld : projectedEnd.originalWorld;
                sLog.outDebug("[MMAP][POLY] map=%u creature=%u which=end ref=" I64FMT " query=(%0.3f,%0.3f,%0.3f) closest=(%0.3f,%0.3f,%0.3f) projected=%u expanded=%u",
                        request.mapId,
                        (request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
                        (uint64)endLookup.polyRef,
                        endReference.x, endReference.y, endReference.z,
                        endLookup.closest[0], endLookup.closest[2], endLookup.closest[1],
                        endLookup.usedProjectedPoint ? 1 : 0,
                        endLookup.usedExpandedExtents ? 1 : 0);
                const float snapEndDx = endLookup.closest[0] - endReference.x;
                const float snapEndDy = endLookup.closest[2] - endReference.y;
                sLog.outDebug("[MMAP][SNAP] map=%u creature=%u which=end xy_dist=%0.3f z_delta=%0.3f",
                        request.mapId,
                        (request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
                        sqrtf((snapEndDx * snapEndDx) + (snapEndDy * snapEndDy)),
                        fabsf(endLookup.closest[1] - endReference.z));
        }

        dtPolyRef pathPolys[MMAP_MAX_PATH_POLYS];
        int pathPolyCount = 0;
        memset(pathPolys, 0, sizeof(pathPolys));

        dtStatus status = query->findPath(startLookup.polyRef, endLookup.polyRef, startLookup.closest, endLookup.closest, &filter, pathPolys, &pathPolyCount, MMAP_MAX_PATH_POLYS);
        if(dtStatusFailed(status) || pathPolyCount == 0)
        {
                result.status = PATH_QUERY_STATUS_NOPATH;
		result.detail = "find_path_failed";
		if(sWorld.MMapDebugPathing)
			sLog.outDebug("[MMAP][FALLBACK] map=%u creature=%u reason=path_query_failed status=0x%08X polys=%d",
				request.mapId,
				(request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
				status, pathPolyCount);
		return result;
	}
	result.polyCount = pathPolyCount;

	float straightPath[MMAP_MAX_PATH_POINTS * 3];
	unsigned char straightFlags[MMAP_MAX_PATH_POINTS];
	dtPolyRef straightRefs[MMAP_MAX_PATH_POINTS];
	int straightCount = 0;
	memset(straightPath, 0, sizeof(straightPath));
	memset(straightFlags, 0, sizeof(straightFlags));
	memset(straightRefs, 0, sizeof(straightRefs));

        status = query->findStraightPath(startLookup.closest, endLookup.closest, pathPolys, pathPolyCount, straightPath, straightFlags, straightRefs, &straightCount, MMAP_MAX_PATH_POINTS, DT_STRAIGHTPATH_ALL_CROSSINGS);
        if(dtStatusFailed(status) || straightCount == 0)
        {
                result.status = PATH_QUERY_STATUS_NOPATH;
		result.detail = "straight_path_failed";
		if(sWorld.MMapDebugPathing)
			sLog.outDebug("[MMAP][FALLBACK] map=%u creature=%u reason=straight_path_generation_failed status=0x%08X polys=%d points=%d",
				request.mapId,
				(request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
				status, pathPolyCount, straightCount);
		return result;
	}
	result.pointCount = straightCount;

        if(sWorld.MMapDebugPathing)
        {
                int pathTileTransitions = 0;
                int straightTileTransitions = 0;
                const string pathTileChain = FormatPolyRefTileChain(data->navMesh, pathPolys, pathPolyCount, &pathTileTransitions);
                const string straightTileChain = FormatPolyRefTileChain(data->navMesh, straightRefs, straightCount, &straightTileTransitions);
                sLog.outDebug("[MMAP][CHAIN] map=%u creature=%u path_polys=%d refs=%s",
                        request.mapId,
                        (request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
                        pathPolyCount,
                        FormatPolyRefChain(pathPolys, pathPolyCount).c_str());
                sLog.outDebug("[MMAP][CHAIN] map=%u creature=%u path_tiles=%d transitions=%d refs=%s",
                        request.mapId,
                        (request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
                        pathPolyCount,
                        pathTileTransitions,
                        pathTileChain.c_str());
                sLog.outDebug("[MMAP][CHAIN] map=%u creature=%u straight_points=%d refs=%s",
                        request.mapId,
                        (request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
                        straightCount,
                        FormatPolyRefChain(straightRefs, straightCount).c_str());
                sLog.outDebug("[MMAP][CHAIN] map=%u creature=%u straight_tiles=%d transitions=%d refs=%s",
                        request.mapId,
                        (request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
                        straightCount,
                        straightTileTransitions,
                        straightTileChain.c_str());
        }

	LocationVector startPoint(request.startX, request.startY, request.startZ);
	if(!NormalizePathPoint(mapMgr, "mmap_path_start", startPoint))
	{
		result.status = PATH_QUERY_STATUS_NOPATH;
		result.detail = "invalid_start_ground";
		if(sWorld.MMapDebugPathing)
			sLog.outDebug("[MMAP][PATH] map=%u creature=%u reject=invalid_start_ground point=(%0.3f,%0.3f,%0.3f)",
				request.mapId,
				(request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
				request.startX, request.startY, request.startZ);
		return result;
	}

	result.points.push_back(startPoint);
	for(int i = 0; i < straightCount; ++i)
	{
		LocationVector worldPoint = ToWorldCoords(&straightPath[i * 3]);
		const float rawNavZ = worldPoint.z;
		if(!NormalizePathPoint(mapMgr, "mmap_path_point", worldPoint))
		{
			result.status = PATH_QUERY_STATUS_NOPATH;
			result.detail = "invalid_path_point_ground";
			if(sWorld.MMapDebugPathing)
			{
				sLog.outDebug("[MMAP][PATH] map=%u creature=%u reject=invalid_path_point_ground index=%d point=(%0.3f,%0.3f,%0.3f)",
					request.mapId,
					(request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
					i, worldPoint.x, worldPoint.y, worldPoint.z);
			}
			return result;
		}

		// Navmesh surface drop guard: if the raw navmesh Z is significantly above the
		// resolved terrain/VMAP Z, NormalizePathPoint returned a lower surface layer
		// (e.g., navmesh baked on a building roof at nav_z=65, but the VMAP query from
		// zHint+2 only reached terrain at 55). Committing this point would place the
		// creature below the intended walkable surface, causing fall-through.
		// Normal bake tolerances produce a sub-unit delta in this direction.
		const float navSurfaceDrop = rawNavZ - worldPoint.z;
		if(navSurfaceDrop > 3.0f)
		{
			result.status = PATH_QUERY_STATUS_NOPATH;
			result.detail = "navmesh_surface_miss";
			if(sWorld.MMapDebugPathing)
			{
				sLog.outDebug("[MMAP][PATH] map=%u creature=%u reject=navmesh_surface_miss index=%d nav_z=%0.3f resolved_z=%0.3f drop=%0.3f point=(%0.3f,%0.3f)",
					request.mapId,
					(request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
					i, rawNavZ, worldPoint.z, navSurfaceDrop, worldPoint.x, worldPoint.y);
			}
			result.points.clear();
			return result;
		}

		if(NearlyEqualHorizontalPoints(result.points.back(), worldPoint, 0.25f))
		{
			if(sWorld.MMapDebugPathing)
			{
				sLog.outDebug("[MMAP][PATH] map=%u creature=%u skip=start_projection_duplicate index=%d prev=(%0.3f,%0.3f,%0.3f) point=(%0.3f,%0.3f,%0.3f)",
					request.mapId,
					(request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
					i,
					result.points.back().x, result.points.back().y, result.points.back().z,
					worldPoint.x, worldPoint.y, worldPoint.z);
			}
			continue;
		}

		if(result.points.empty() || NearlyEqualPoints(result.points.back(), worldPoint, 0.25f))
			continue;

		string pathValidationDetail;
                const bool allowFirstSegmentDetourLosBlockedGroundOk = (i == 0 && pathPolyCount > 0);
		if(!ValidatePathSegment(mapMgr, request.mover, result.points.back(), worldPoint, &pathValidationDetail, allowFirstSegmentDetourLosBlockedGroundOk))
		{
			result.status = PATH_QUERY_STATUS_NOPATH;
			result.detail = pathValidationDetail.empty() ? "unsafe_path_segment" : pathValidationDetail;
			if(sWorld.MMapDebugPathing)
			{
				sLog.outDebug("[MMAP][PATH] map=%u creature=%u reject=%s from=(%0.3f,%0.3f,%0.3f) to=(%0.3f,%0.3f,%0.3f)",
					request.mapId,
					(request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
					result.detail.c_str(),
					result.points.back().x, result.points.back().y, result.points.back().z,
					worldPoint.x, worldPoint.y, worldPoint.z);
			}
			result.points.clear();
			return result;
		}

		result.points.push_back(worldPoint);
	}

	if(result.points.size() <= 1)
	{
		result.status = PATH_QUERY_STATUS_NOPATH;
		result.detail = "empty_path_points";
		return result;
	}

        if(forceEndpointPartial)
        {
                result.status = PATH_QUERY_STATUS_PARTIAL;
                result.detail = (endpointProjectionReason != NULL) ? endpointProjectionReason : "partial_clamped";
                result.acceptedPartial = true;
        }
        else if(pathPolys[pathPolyCount - 1] == endLookup.polyRef)
	{
		result.status = PATH_QUERY_STATUS_COMPLETE;
		result.detail = "complete";
	}
	else if(request.allowPartial)
	{
		result.status = PATH_QUERY_STATUS_PARTIAL;
		result.detail = "partial_clamped";
		result.acceptedPartial = true;
	}
	else
	{
		result.status = PATH_QUERY_STATUS_NOPATH;
		result.detail = "partial_disallowed";
		result.points.clear();
	}

	if(sWorld.MMapDebugPathing)
	{
		const LocationVector& finalPoint = result.points.back();
		const float goalDx = finalPoint.x - request.endX;
		const float goalDy = finalPoint.y - request.endY;
		const float goalDz = finalPoint.z - request.endZ;
		const float goalDelta = sqrtf((goalDx * goalDx) + (goalDy * goalDy) + (goalDz * goalDz));
                sLog.outDebug("[MMAP][PATH] map=%u creature=%u polys=%u points=%u partial=%u final=(%0.3f,%0.3f,%0.3f) goal_delta=%0.3f",
                        request.mapId,
                        (request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
                        result.polyCount,
                        result.pointCount,
			result.acceptedPartial ? 1 : 0,
                        finalPoint.x, finalPoint.y, finalPoint.z,
                        goalDelta);
                uint32 seamStartTileX = 0;
                uint32 seamStartTileY = 0;
                uint32 seamEndTileX = 0;
                uint32 seamEndTileY = 0;
                if(result.acceptedPartial &&
                   result.polyCount <= 1 &&
                   mapMgr != NULL &&
                   ResolveTileCoords(mapMgr, request.startX, request.startY, seamStartTileX, seamStartTileY) &&
                   ResolveTileCoords(mapMgr, request.endX, request.endY, seamEndTileX, seamEndTileY) &&
                   (seamStartTileX != seamEndTileX || seamStartTileY != seamEndTileY))
                {
                        sLog.outDebug("[MMAP][SEAM] map=%u creature=%u reason=partial_single_poly_seam start_tile=%u,%u end_tile=%u,%u final=(%0.3f,%0.3f,%0.3f) goal=(%0.3f,%0.3f,%0.3f)",
                                request.mapId,
                                (request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
                                seamStartTileX, seamStartTileY, seamEndTileX, seamEndTileY,
                                finalPoint.x, finalPoint.y, finalPoint.z,
                                request.endX, request.endY, request.endZ);
                }
        }

	return result;
}

string MMapInterface::BuildMapFilePath(uint32 mapId) const
{
	char fileName[32];
	snprintf(fileName, sizeof(fileName), "%03u.mmap", mapId);
	return BuildMMapPath(sWorld.MMapPath, fileName);
}

string MMapInterface::BuildTileFilePath(uint32 mapId, uint32 tileX, uint32 tileY) const
{
	char fileName[32];
	snprintf(fileName, sizeof(fileName), "%03u_%02u_%02u.mmtile", mapId, tileY, tileX);
	return BuildMMapPath(sWorld.MMapPath, fileName);
}

void MMapInterface::LogQueryResult(const PathQueryRequest& request, const PathQueryResult& result, uint32 startTileX, uint32 startTileY, uint32 endTileX, uint32 endTileY) const
{
	if(!sWorld.MMapDebugPathing)
		return;

	sLog.outDebug("MMap path query map=%u reason=%s start=%0.3f,%0.3f,%0.3f tile=%u,%u end=%0.3f,%0.3f,%0.3f tile=%u,%u status=%s detail=%s points=%u mapData=%u tileData=%u backend=%u fallback=%u",
		request.mapId,
		request.reason ? request.reason : "unknown",
		request.startX, request.startY, request.startZ, startTileX, startTileY,
		request.endX, request.endY, request.endZ, endTileX, endTileY,
		GetPathQueryStatusName(result.status),
		result.detail.c_str(),
		(uint32)result.points.size(),
		result.hadMapData ? 1 : 0,
		result.hadTileData ? 1 : 0,
		result.hadRuntimeBackend ? 1 : 0,
		result.usedFallback ? 1 : 0);

	sLog.outDebug("[MMAP][MOVE] map=%u creature=%u tile_start=%u,%u tile_end=%u,%u status=%s detail=%s map_loaded=%u tile_loaded=%u",
		request.mapId,
		(request.mover != NULL && request.mover->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(request.mover)->GetEntry() : 0,
		startTileX, startTileY, endTileX, endTileY,
		GetPathQueryStatusName(result.status),
		result.detail.c_str(),
		result.hadMapData ? 1 : 0,
		result.hadTileData ? 1 : 0);
}
