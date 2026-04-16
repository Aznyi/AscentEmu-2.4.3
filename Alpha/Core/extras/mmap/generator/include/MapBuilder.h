/*
 * This file is part of the CMaNGOS Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _MAP_BUILDER_H
#define _MAP_BUILDER_H

#include <vector>
#include <set>
#include <map>
#include <mutex>
#include <sstream>

#include "TerrainBuilder.h"
#include "IntermediateValues.h"

#include "Recast.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"

#include "json.hpp"

using namespace std;
// G3D namespace typedefs conflicts with ACE typedefs

using json = nlohmann::json;

namespace MMAP
{
    // these are WORLD UNIT based metrics
    // this are basic unit dimentions
    // value have to divide GRID_SIZE(533.33333f) ( aka: 0.5333, 0.2666, 0.3333, 0.1333, etc )
    const static float BASE_UNIT_DIM = 0.2666666f;

    // All are in UNIT metrics!
    const static int VERTEX_PER_MAP = int(GRID_SIZE / BASE_UNIT_DIM + 0.5f);
    const static int VERTEX_PER_TILE = 80; // must divide VERTEX_PER_MAP
    const static int TILES_PER_MAP = VERTEX_PER_MAP / VERTEX_PER_TILE;

    class TaskQueue;

    typedef std::map<uint32, std::set<uint32>> TileList;
    typedef std::set<uint32> MapSet;
    struct Tile
    {
        Tile() : chf(NULL), solid(NULL), cset(NULL), pmesh(NULL), dmesh(NULL) {}
        ~Tile()
        {
            rcFreeCompactHeightfield(chf);
            rcFreeContourSet(cset);
            rcFreeHeightField(solid);
            rcFreePolyMesh(pmesh);
            rcFreePolyMeshDetail(dmesh);
        }

        rcCompactHeightfield* chf;
        rcHeightfield* solid;
        rcContourSet* cset;
        rcPolyMesh* pmesh;
        rcPolyMeshDetail* dmesh;
    };

    enum TileBuildReason
    {
        TILE_REASON_WRITTEN = 0,
        TILE_REASON_SKIPPED_BY_CONFIG,
        TILE_REASON_NO_SOURCE_GEOMETRY,
        TILE_REASON_NO_GEOMETRY_AFTER_CLEANUP,
        TILE_REASON_NO_WALKABLE_SPANS,
        TILE_REASON_EMPTY_AFTER_MERGE,
        TILE_REASON_NO_POLYGONS,
        TILE_REASON_NO_DETAIL_MESH,
        TILE_REASON_NAVDATA_BUILD_FAILED,
        TILE_REASON_NAVMESH_ADD_FAILED,
        TILE_REASON_WRITE_FILE_FAILED,
        TILE_REASON_SUBTILE_BUILD_FAILED,
        TILE_REASON_INTERNAL_ERROR,
        TILE_REASON_COUNT
    };

    class MapBuilder
    {
        public:
            struct InspectPoint
            {
                bool enabled;
                float worldX;
                float worldY;
                float worldZ;
                float extents[3];

                InspectPoint() : enabled(false), worldX(0.0f), worldY(0.0f), worldZ(0.0f)
                {
                    extents[0] = 12.0f;
                    extents[1] = 40.0f;
                    extents[2] = 12.0f;
                }
            };

            MapBuilder(const char* configInputPath,
                       int threads,
                       bool skipLiquid          = false,
                       bool skipContinents      = false,
                       bool skipJunkMaps        = true,
                       bool skipBattlegrounds   = false,
                       bool debug               = false,
                       const char* offMeshFilePath = NULL,
                       const char* workdir = NULL,
                       const InspectPoint* inspectPoint = NULL);

            ~MapBuilder();

            // if no ids provided all map will be build
            void BuildMaps(std::vector<uint32>& ids);

            // builds an mmap tile for the specified map and its mesh
            void buildSingleTile(uint32 mapID, uint32 tileX, uint32 tileY);

            // builds all GO models needed for pathfinding
            void buildGameObject(std::string modelName, uint32 displayId);
            void buildTransports();

            bool IsMapDone(uint32 mapId) const;

        private:
            struct BuildSummary
            {
                BuildSummary() : discoveredMapEntries(0), discoveredVMapEntries(0), discoveredTileCount(0), scheduledTileCount(0)
                {
                    memset(reasonCounts, 0, sizeof(reasonCounts));
                }

                uint32 discoveredMapEntries;
                uint32 discoveredVMapEntries;
                uint32 discoveredTileCount;
                uint32 scheduledTileCount;
                uint32 reasonCounts[TILE_REASON_COUNT];
            };

            // builds all mmap tiles for the specified map id (ignores skip settings)
            void buildMap(uint32 mapID);

            // detect maps and tiles
            void discoverTiles();
            std::set<uint32>& getTileList(uint32 mapID);

            void buildNavMesh(uint32 mapID, dtNavMesh*& navMesh);

            void buildTile(uint32 mapID, uint32 tileX, uint32 tileY, dtNavMesh* navMesh, uint32 curTile, uint32 tileCount, uint32 workerIndex);
            bool buildCommonTile(const char* tileString, Tile& tile, rcConfig& tileCfg, float* tVerts, int tVertCount, int* tTris, int tTriCount, float* lVerts, int lVertCount,
                                 int* lTris, int lTriCount, uint8* lTriFlags, const json* tileConfigJson = NULL);

            // move map building
            TileBuildReason buildMoveMapTile(uint32 mapID, uint32 tileX, uint32 tileY, MeshData& meshData, float bmin[3], float bmax[3], dtNavMesh* navMesh);
            void getTileBounds(uint32 tileX, uint32 tileY, float* verts, int vertCount, float* bmin, float* bmax);
            void getGridBounds(uint32 mapID, uint32& minX, uint32& minY, uint32& maxX, uint32& maxY);
            uint32 CountHeightfieldSpans(const rcHeightfield* solid) const;
            bool TileBoundsContainInspectPoint(float bmin[3], float bmax[3], float* inspectNav) const;
            void ToNavMeshCoords(float worldX, float worldY, float worldZ, float* out) const;
            void LogInspectPointResult(const char* tileString, uint32 mapID, uint32 tileX, uint32 tileY, dtNavMesh* navMesh, float* queryPoint) const;
            bool TriangleNearInspectPoint(const float* verts, const int* tris, int triIndex, const float* inspectNav, float horizontalRadius, float verticalRadius) const;
            void LogInputGeometryAroundInspectPoint(const char* tileString, const char* sourceLabel, float* tVerts, int tTriCount, int* tTris, float* lVerts, int lTriCount, int* lTris, float* inspectNav, float walkableSlopeAngle) const;

            bool shouldSkipMap(uint32 mapID);
            bool isTransportMap(uint32 mapID);
            bool shouldSkipTile(uint32 mapID, uint32 tileX, uint32 tileY);

            json getDefaultConfig();
            json getMapIdConfig(uint32 mapId);
            json getTileConfig(uint32 mapId, uint32 tileX, uint32 tileY);
            static const char* GetTileBuildReasonName(TileBuildReason reason);
            void RecordTileOutcome(TileBuildReason reason);
            void LogTileOutcome(uint32 mapID, uint32 tileX, uint32 tileY, TileBuildReason reason, bool hadTerrainGeometry, bool hadVMapGeometry, uint32 solidTriCount, uint32 liquidTriCount);
            void PrintBuildSummary() const;

            TerrainBuilder* m_terrainBuilder;
            TileList m_tiles;

            bool m_debug;

            const char* m_offMeshFilePath;
            const char* m_workdir;
            bool m_skipContinents;
            bool m_skipJunkMaps;
            bool m_skipBattlegrounds;
            InspectPoint m_inspectPoint;

            json m_config;

            uint32 m_threads;
            mutable std::mutex m_terrainMutex;
            mutable std::mutex m_summaryMutex;
            BuildSummary m_summary;

            // used to know wich map have launched all its tile work
            MapSet m_mapDone;
    };

}

#endif
