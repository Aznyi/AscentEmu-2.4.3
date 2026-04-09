#ifndef __MMAPINTERFACE_H
#define __MMAPINTERFACE_H

class MapMgr;
class Unit;

enum PathQueryStatus
{
	PATH_QUERY_STATUS_NOPATH = 0,
	PATH_QUERY_STATUS_DIRECT = 1,
	PATH_QUERY_STATUS_PARTIAL = 2,
	PATH_QUERY_STATUS_COMPLETE = 3,
	PATH_QUERY_STATUS_NAVMESH_UNAVAILABLE = 4,
	PATH_QUERY_STATUS_MISSING_DATA = 5,
	PATH_QUERY_STATUS_MALFORMED_DATA = 6
};

ASCENT_INLINE const char* GetPathQueryStatusName(PathQueryStatus status)
{
	switch(status)
	{
	case PATH_QUERY_STATUS_DIRECT: return "direct";
	case PATH_QUERY_STATUS_PARTIAL: return "partial";
	case PATH_QUERY_STATUS_COMPLETE: return "complete";
	case PATH_QUERY_STATUS_NAVMESH_UNAVAILABLE: return "navmesh_unavailable";
	case PATH_QUERY_STATUS_MISSING_DATA: return "missing_data";
	case PATH_QUERY_STATUS_MALFORMED_DATA: return "malformed_data";
	default: return "no_path";
	}
}

struct PathQueryRequest
{
	uint32 mapId;
	uint32 instanceId;
	Unit* mover;
	float startX;
	float startY;
	float startZ;
	float endX;
	float endY;
	float endZ;
	bool allowPartial;
	bool allowFallback;
	const char* reason;

	PathQueryRequest() : mapId(0), instanceId(0), mover(NULL), startX(0.0f), startY(0.0f), startZ(0.0f), endX(0.0f), endY(0.0f), endZ(0.0f), allowPartial(true), allowFallback(true), reason(NULL) {}
};

struct PathQueryResult
{
	PathQueryStatus status;
	std::vector<LocationVector> points;
	bool attemptedMMap;
	bool usedFallback;
	bool hadMapData;
	bool hadTileData;
	bool hadRuntimeBackend;
	string detail;

	PathQueryResult() : status(PATH_QUERY_STATUS_NAVMESH_UNAVAILABLE), attemptedMMap(false), usedFallback(false), hadMapData(false), hadTileData(false), hadRuntimeBackend(false) {}
};

struct NavMeshInspectionResult
{
	bool attempted;
	bool valid;
	bool hasMapData;
	bool hasTileData;
	bool tileLoaded;
	bool nearestPolyFound;
	uint32 tileX;
	uint32 tileY;
	uint32 tilePolyCount;
	uint32 nearbyPolyCount;
	uint32 tileHeaderX;
	uint32 tileHeaderY;
	uint32 tileHeaderLayer;
	uint64 nearestPolyRef;
	float radius;
	LocationVector originalWorld;
	LocationVector resolvedWorld;
	LocationVector closestWorld;
	string detail;

	NavMeshInspectionResult() : attempted(false), valid(false), hasMapData(false), hasTileData(false), tileLoaded(false), nearestPolyFound(false),
		tileX(0), tileY(0), tilePolyCount(0), nearbyPolyCount(0), tileHeaderX(0), tileHeaderY(0), tileHeaderLayer(0),
		nearestPolyRef(0), radius(0.0f) {}
};

class SERVER_DECL MMapInterface
{
public:
	MMapInterface();
	~MMapInterface();

	void Init();
	void DeInit();

	PathQueryResult BuildPath(MapMgr* mapMgr, const PathQueryRequest& request);

	void ActivateTile(uint32 mapId, uint32 tileX, uint32 tileY);
	void DeactivateTile(uint32 mapId, uint32 tileX, uint32 tileY);
	uint32 GetTileLoadRefCount(uint32 mapId, uint32 tileX, uint32 tileY) const;

	bool HasMMapDirectory() const;
	bool HasMapData(uint32 mapId) const;
	bool HasTileData(uint32 mapId, uint32 tileX, uint32 tileY) const;
	bool HasRuntimeBackend() const;
	bool IsPathingEnabled() const;
	NavMeshInspectionResult InspectPoint(MapMgr* mapMgr, uint32 mapId, uint32 instanceId, float x, float y, float z, float radius);

private:
	struct RuntimeMapData;
	struct RuntimeTileInfo;

	uint64 BuildTileRefKey(uint32 mapId, uint32 tileX, uint32 tileY) const;
	uint32 BuildPackedTileID(uint32 tileX, uint32 tileY) const;
	bool ResolveTileCoords(MapMgr* mapMgr, float x, float y, uint32& tileX, uint32& tileY) const;
	void ToNavMeshCoords(float worldX, float worldY, float worldZ, float* out) const;
	LocationVector ToWorldCoords(const float* navPoint) const;
	bool ValidateMapFile(uint32 mapId, string* detail = NULL) const;
	bool ValidateTileFile(uint32 mapId, uint32 tileX, uint32 tileY, string* detail = NULL) const;
	bool EnsureMapLoaded(uint32 mapId, string* detail = NULL);
	bool EnsureTileLoaded(uint32 mapId, uint32 tileX, uint32 tileY, bool temporaryLoad, string* detail = NULL);
	void ReleaseTemporaryTile(uint32 mapId, uint32 tileX, uint32 tileY);
	bool UnloadTile(uint32 mapId, uint32 tileX, uint32 tileY, bool logFailure);
	void FreeRuntimeMapData(RuntimeMapData* data);
        PathQueryResult QueryPath(MapMgr* mapMgr, uint32 mapId, uint32 instanceId, const PathQueryRequest& request);
	string BuildMapFilePath(uint32 mapId) const;
	string BuildTileFilePath(uint32 mapId, uint32 tileX, uint32 tileY) const;
	void LogQueryResult(const PathQueryRequest& request, const PathQueryResult& result, uint32 startTileX, uint32 startTileY, uint32 endTileX, uint32 endTileY) const;

	mutable Mutex m_lock;
	map<uint64, uint32> m_tileRefCounts;
	map<uint32, RuntimeMapData*> m_runtimeMaps;
	string m_rootPath;
	bool m_rootAccessible;
};

extern SERVER_DECL MMapInterface sMMapInterface;

#endif
