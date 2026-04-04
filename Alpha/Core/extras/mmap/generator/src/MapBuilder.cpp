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

#include "MMapCommon.h"
#include "MapBuilder.h"
#include "AscentVMapBridge.h"

#include "DetourNavMeshBuilder.h"
#include "DetourCommon.h"

#include <climits>
#include <cmath>
#include <fstream>
#include <future>

void rcModAlmostUnwalkableTriangles(rcContext* ctx, const float walkableSlopeAngle,
    const float* verts, int /*nv*/,
    const int* tris, int nt,
    unsigned char* areas)
{
    rcIgnoreUnused(ctx);

    const float walkableThr = cosf(walkableSlopeAngle / 180.0f * RC_PI);

    float norm[3];

    for (int i = 0; i < nt; ++i)
    {
        if (areas[i] & RC_WALKABLE_AREA)
        {
            const int* tri = &tris[i * 3];

            float e0[3], e1[3];
            rcVsub(e0, &verts[tri[1] * 3], &verts[tri[0] * 3]);
            rcVsub(e1, &verts[tri[2] * 3], &verts[tri[0] * 3]);
            rcVcross(norm, e0, e1);
            rcVnormalize(norm);

            // Check if the face is walkable.
            if (norm[1] <= walkableThr)
                areas[i] = NAV_AREA_GROUND_STEEP; //Slopes between 50 and 60. Walkable for mobs, unwalkable for players.
        }
    }
}

void from_json(const json& j, rcConfig& config)
{
    config.tileSize = MMAP::VERTEX_PER_TILE;
    config.borderSize = j["borderSize"].get<int>();
    config.cs = MMAP::BASE_UNIT_DIM;
    config.ch = MMAP::BASE_UNIT_DIM;
    config.walkableSlopeAngle = j["walkableSlopeAngle"].get<float>();
    config.walkableHeight = j["walkableHeight"].get<int>();
    config.walkableClimb = j["walkableClimb"].get<int>();
    config.walkableRadius = j["walkableRadius"].get<int>();
    config.maxEdgeLen = j["maxEdgeLen"].get<int>();
    config.maxSimplificationError = j["maxSimplificationError"].get<float>();
    config.minRegionArea = rcSqr(j["minRegionArea"].get<int>());
    config.mergeRegionArea = rcSqr(j["mergeRegionArea"].get<int>());
    config.maxVertsPerPoly = DT_VERTS_PER_POLYGON;
    config.detailSampleDist = j["detailSampleDist"].get<float>();
    config.detailSampleMaxError = j["detailSampleMaxError"].get<float>();
}

namespace MMAP
{
    namespace
    {
        inline bool HasMapFileExtension(const std::string& fileName)
        {
            return fileName.length() >= 4 && fileName.compare(fileName.length() - 4, 4, ".map") == 0;
        }

        inline bool IsAscentTileMap(uint32 mapID)
        {
            switch (mapID)
            {
                case 0:
                case 1:
                case 30:
                case 33:
                case 37:
                case 189:
                case 209:
                case 309:
                case 469:
                case 509:
                case 530:
                case 532:
                case 533:
                case 534:
                case 543:
                case 560:
                case 564:
                case 568:
                    return true;
                default:
                    return false;
            }
        }

        inline void SeedAllMapTiles(std::set<uint32>& tiles, uint32& count)
        {
            for (uint32 tileX = 0; tileX < 64; ++tileX)
                for (uint32 tileY = 0; tileY < 64; ++tileY)
                    if (tiles.insert(PackAscentTileID(tileX, tileY)).second)
                        ++count;
        }

        inline bool ParseAscentPackedMapName(const std::string& fileName, uint32& mapID)
        {
            if (fileName.length() < 9 || fileName.compare(0, 4, "Map_") != 0)
                return false;

            const std::string digits = fileName.substr(4, fileName.length() - 8);
            if (digits.empty())
                return false;

            for (size_t i = 0; i < digits.length(); ++i)
                if (digits[i] < '0' || digits[i] > '9')
                    return false;

            if (fileName.compare(fileName.length() - 4, 4, ".bin") != 0)
                return false;

            mapID = uint32(atoi(digits.c_str()));
            return true;
        }
    }

    inline char const* GetDTErrorReason(dtStatus status) {
        if ((status & DT_WRONG_MAGIC) != 0)
            return "Reason: 'Input data is not recognized'";
        if ((status & DT_WRONG_VERSION) != 0)
            return "Reason: 'Input data is in wrong version'";
        if ((status & DT_OUT_OF_MEMORY) != 0)
            return "Reason: 'Operation ran out of memory'";
        if ((status & DT_INVALID_PARAM) != 0)
            return "Reason: 'An input parameter was invalid'";
        if ((status & DT_BUFFER_TOO_SMALL) != 0)
            return "Reason: 'Result buffer for the query was too small to store all results'";
        if ((status & DT_OUT_OF_NODES) != 0)
            return "Reason: 'Query ran out of nodes during search'";
        if ((status & DT_PARTIAL_RESULT) != 0)
            return "Reason: 'Query did not reach the end location, returning best guess'";
        if ((status & DT_ALREADY_OCCUPIED) != 0)
            return "Reason: 'A tile has already been assigned to the given x,y coordinate'";
        return "Reason: 'Unknown Detour error'";
    }

    MapBuilder::MapBuilder(const char* configInputPath, int threads, bool skipLiquid, bool skipContinents, bool skipJunkMaps,
                           bool skipBattlegrounds, bool debug, const char* offMeshFilePath, const char* workdir, const InspectPoint* inspectPoint) :
        m_taskQueue(new TaskQueue(this, threads)),
        m_debug(debug),
        m_skipContinents(skipContinents),
        m_skipJunkMaps(skipJunkMaps),
        m_skipBattlegrounds(skipBattlegrounds),
        m_offMeshFilePath(offMeshFilePath),
        m_workdir(workdir)
    {
        if (inspectPoint != NULL)
            m_inspectPoint = *inspectPoint;

        std::ifstream jsonConfig(configInputPath);
        if (jsonConfig)
            m_config = json::parse(jsonConfig);

        m_terrainBuilder = new TerrainBuilder(skipLiquid, workdir);

        printf("Using %d thread(s) for processing.\n", threads);
        discoverTiles();
    }

    /**************************************************************************/
    MapBuilder::~MapBuilder()
    {
        delete m_terrainBuilder;
    }

    /**************************************************************************/
    void MapBuilder::BuildMaps(std::vector<uint32>& ids)
    {
        if (ids.empty())
        {
            for (auto tileItr : m_tiles)
            {
                uint32 const& mapID = tileItr.first;
                if (!shouldSkipMap(mapID))
                    buildMap(mapID);

                m_mapDone.insert(mapID);
            }
        }
        else
        {
            for (auto& mapId : ids)
            {
                if (!shouldSkipMap(mapId))
                    buildMap(mapId);

                m_mapDone.insert(mapId);
            }
        }

        // Wait all work to be done
        m_taskQueue->WaitAll();
    }

    /**************************************************************************/
    void MapBuilder::discoverTiles()
    {
        std::vector<std::string> files;
        uint32 mapID, tileX, tileY, tileID, count = 0;
        char maps_dir[1024];
        char vmaps_dir[1024];

        printf("Discovering maps... ");
        sprintf(maps_dir, "%s/maps", m_workdir);
        getDirContents(files, maps_dir);
        for (uint32 i = 0; i < files.size(); ++i)
        {
            if (ParseAscentPackedMapName(files[i], mapID))
            {
                if (m_tiles.find(mapID) == m_tiles.end())
                {
                    m_tiles.emplace(mapID, std::set<uint32>{});
                    count++;
                }
                continue;
            }

            mapID = uint32(atoi(files[i].substr(0, 3).c_str()));
            if (m_tiles.find(mapID) == m_tiles.end())
            {
                m_tiles.emplace(mapID, std::set<uint32>{});
                count++;
            }
        }

        files.clear();
        sprintf(vmaps_dir, "%s/vmaps", m_workdir);
        getDirContents(files, vmaps_dir, "*.vmdir");
        for (uint32 i = 0; i < files.size(); ++i)
        {
            bool tiledManifest = false;
            tileX = 0;
            tileY = 0;
            if (!ParseAscentVMapManifestName(files[i], mapID, tileX, tileY, tiledManifest))
                continue;

            if (m_tiles.find(mapID) == m_tiles.end())
            {
                m_tiles.emplace(mapID, std::set<uint32>{});
                count++;
            }
        }
        printf("found %u.\n", count);

        count = 0;
        printf("Discovering tiles... ");
        for (TileList::iterator itr = m_tiles.begin(); itr != m_tiles.end(); ++itr)
        {
            std::set<uint32>& tiles = (*itr).second;
            mapID = (*itr).first;
            bool hasGlobalVMapManifest = false;

            files.clear();
            getDirContents(files, vmaps_dir, "*.vmdir");
            for (uint32 i = 0; i < files.size(); ++i)
            {
                uint32 manifestMapId = 0;
                bool tiledManifest = false;
                if(!ParseAscentVMapManifestName(files[i], manifestMapId, tileX, tileY, tiledManifest) || manifestMapId != mapID)
                    continue;

                if (!tiledManifest)
                {
                    hasGlobalVMapManifest = true;
                    continue;
                }

                tileID = PackAscentTileID(tileX, tileY);
                if (tiles.insert(tileID).second)
                    count++;
            }

            files.clear();
            getDirContents(files, maps_dir);
            for (uint32 i = 0; i < files.size(); ++i)
            {
                uint32 packedMapId = 0;
                if (ParseAscentPackedMapName(files[i], packedMapId))
                {
                    if (packedMapId == mapID)
                    {
                        const size_t before = tiles.size();
                        TerrainBuilder::discoverMapTiles(m_workdir, mapID, tiles);
                        count += uint32(tiles.size() - before);
                    }

                    continue;
                }

                if(!HasMapFileExtension(files[i]) || files[i].length() < 7 || uint32(atoi(files[i].substr(0, 3).c_str())) != mapID)
                    continue;

                tileY = uint32(atoi(files[i].substr(3, 2).c_str()));
                tileX = uint32(atoi(files[i].substr(5, 2).c_str()));
                tileID = PackAscentTileID(tileX, tileY);

                if (tiles.insert(tileID).second)
                    count++;
            }

            // Some Ascent/TBC datasets keep continent and city collision in a single
            // global manifest like 000.vmdir instead of tile manifests. Seed the full
            // tile grid so those tiles are scheduled and can use the global-manifest
            // fallback during vmap loading.
            if (hasGlobalVMapManifest && IsAscentTileMap(mapID))
                SeedAllMapTiles(tiles, count);
        }
        printf("found %u.\n\n", count);
    }

    /**************************************************************************/
    std::set<uint32>& MapBuilder::getTileList(uint32 mapID)
    {
        TileList::iterator itr = m_tiles.find(mapID);
        if (itr != m_tiles.end())
            return (*itr).second;

        return m_tiles.emplace(mapID, std::set<uint32>{}).first->second;
    }

    /**************************************************************************/
    bool MapBuilder::IsMapDone(uint32 mapId) const
    {
        auto itr = std::find(m_mapDone.begin(), m_mapDone.end(), mapId);
        return itr != m_mapDone.end();
    }

    /**************************************************************************/
    void MapBuilder::buildGameObject(std::string modelName, uint32 displayId)
    {
        printf("Building GameObject model %s\n", modelName.c_str());
        MeshData meshData;
        if (!LoadAscentVMapModel(m_workdir, modelName.c_str(), meshData))
        {
            printf("* Unable to open file\n");
            return;
        }

        // if there is no data, give up now
        if (!meshData.solidVerts.size())
        {
            printf("* no solid vertices found\n");
            return;
        }
        TerrainBuilder::cleanVertices(meshData.solidVerts, meshData.solidTris);

        // gather all mesh data for final data check, and bounds calculation
        G3D::Array<float> allVerts;
        allVerts.append(meshData.solidVerts);

        if (!allVerts.size())
            return;

        printf("* Model opened (%u vertices)\n", allVerts.size());

        float* tVerts = meshData.solidVerts.getCArray();
        int tVertCount = meshData.solidVerts.size() / 3;
        int* tTris = meshData.solidTris.getCArray();
        int tTriCount = meshData.solidTris.size() / 3;

        // get bounds of current tile
        rcConfig config;
        memset(&config, 0, sizeof(rcConfig));
        config = getDefaultConfig();
        config.detailSampleDist = config.cs * 6.0f;
        config.minRegionArea = config.minRegionArea / 2;

        // this sets the dimensions of the heightfield - should maybe happen before border padding
        rcCalcBounds(tVerts, tVertCount, config.bmin, config.bmax);
        rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);

        Tile tile;
        buildCommonTile(modelName.data(), tile, config, tVerts, tVertCount, tTris, tTriCount, nullptr, 0, nullptr, 0, 0);

        IntermediateValues iv;
        iv.polyMesh = tile.pmesh;
        iv.polyMeshDetail = tile.dmesh;
        for (int i = 0; i < iv.polyMesh->npolys; ++i)
        {
            if (uint8 area = iv.polyMesh->areas[i] & NAV_AREA_ALL_MASK)
            {
                if (area >= NAV_AREA_MIN_VALUE)
                    iv.polyMesh->flags[i] = 1 << (NAV_AREA_MAX_VALUE - area);
                else
                    iv.polyMesh->flags[i] = NAV_GROUND;
            }
        }

        // Will be deleted by IntermediateValues
        tile.pmesh = nullptr;
        tile.dmesh = nullptr;
        // setup mesh parameters
        dtNavMeshCreateParams params;
        memset(&params, 0, sizeof(params));
        params.verts = iv.polyMesh->verts;
        params.vertCount = iv.polyMesh->nverts;
        params.polys = iv.polyMesh->polys;
        params.polyAreas = iv.polyMesh->areas;
        params.polyFlags = iv.polyMesh->flags;
        params.polyCount = iv.polyMesh->npolys;
        params.nvp = iv.polyMesh->nvp;
        params.detailMeshes = iv.polyMeshDetail->meshes;
        params.detailVerts = iv.polyMeshDetail->verts;
        params.detailVertsCount = iv.polyMeshDetail->nverts;
        params.detailTris = iv.polyMeshDetail->tris;
        params.detailTriCount = iv.polyMeshDetail->ntris;

        params.walkableHeight = BASE_UNIT_DIM * config.walkableHeight;
        params.walkableRadius = BASE_UNIT_DIM * config.walkableRadius;
        params.walkableClimb = BASE_UNIT_DIM * config.walkableClimb;

        rcVcopy(params.bmin, iv.polyMesh->bmin);
        rcVcopy(params.bmax, iv.polyMesh->bmax);
        params.cs = config.cs;
        params.ch = config.ch;
        params.buildBvTree = true;

        unsigned char* navData = nullptr;
        int navDataSize = 0;
        printf("* Building navmesh tile [%f %f %f to %f %f %f]\n",
            params.bmin[0], params.bmin[1], params.bmin[2],
            params.bmax[0], params.bmax[1], params.bmax[2]);
        printf(" %u triangles (%u vertices)\n", params.polyCount, params.vertCount);
        printf(" %u polygons (%u vertices)\n", params.detailTriCount, params.detailVertsCount);
        if (params.nvp > DT_VERTS_PER_POLYGON)
        {
            printf("Invalid verts-per-polygon value!        \n");
            return;
        }
        if (params.vertCount >= 0xffff)
        {
            printf("Too many vertices! (0x%8x)        \n", params.vertCount);
            return;
        }
        if (!params.vertCount || !params.verts)
        {
            printf("No vertices to build tile!              \n");
            return;
        }
        if (!params.polyCount || !params.polys)
        {
            // we have flat tiles with no actual geometry - don't build those, its useless
            // keep in mind that we do output those into debug info
            // drop tiles with only exact count - some tiles may have geometry while having less tiles
            printf("No polygons to build on tile!              \n");
            return;
        }
        if (!params.detailMeshes || !params.detailVerts || !params.detailTris)
        {
            printf("No detail mesh to build tile!           \n");
            return;
        }
        if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
        {
            printf("Failed building navmesh tile!           \n");
            return;
        }

        // TODO: extract additional data that will enable RecastDemo viewing of transport mmaps
        // navmesh creation params
        //dtNavMeshParams navMeshParams;
        //memset(&navMeshParams, 0, sizeof(dtNavMeshParams));
        //navMeshParams.tileWidth = GRID_SIZE;
        //navMeshParams.tileHeight = GRID_SIZE;
        //rcVcopy(navMeshParams.orig, config.bmin);
        //navMeshParams.maxTiles = 1;
        //navMeshParams.maxPolys = 1 << DT_POLY_BITS;

        //dtNavMesh* navMesh = nullptr;
        //navMesh = dtAllocNavMesh();
        //printf("[Map %03i] Creating navMesh...                        \r", displayId);
        //if (!navMesh->init(&navMeshParams))
        //{
        //    printf("[Map %03i] Failed creating navmesh!                   \n", displayId);
        //    return;
        //}

        //sprintf(fileName, "mmaps/%03u.mmap", displayId);

        //FILE* file = fopen(fileName, "wb");
        //if (!file)
        //{
        //    dtFreeNavMesh(navMesh);
        //    char message[1024];
        //    sprintf(message, "[Map %03i] Failed to open %s for writing!             \n", displayId, fileName);
        //    perror(message);
        //    return;
        //}

        //// now that we know navMesh params are valid, we can write them to file
        //fwrite(&navMeshParams, sizeof(dtNavMeshParams), 1, file);
        //fclose(file);

        char fileName[255];
        sprintf(fileName, "%s/mmaps/go%04u.mmtile", m_workdir, displayId);
        FILE* file = fopen(fileName, "wb");
        if (!file)
        {
            char message[1024];
            sprintf(message, "Failed to open %s for writing!\n", fileName);
            perror(message);
            return;
        }

        printf("* Writing to file \"%s\" [size=%u]\n", fileName, navDataSize);

        // write header
        MmapTileHeader header;
        header.usesLiquids = false;
        header.size = uint32(navDataSize);
        fwrite(&header, sizeof(MmapTileHeader), 1, file);

        // write data
        fwrite(navData, sizeof(unsigned char), navDataSize, file);
        fclose(file);
        if (m_debug)
        {
            iv.generateObjFile(modelName, meshData);
            // Write navmesh data
            std::string fname = "/meshes/" + modelName + ".nav";
            fname = m_workdir + fname;
            FILE* file = fopen(fname.c_str(), "wb");
            if (file)
            {
                fwrite(&navDataSize, sizeof(uint32), 1, file);
                fwrite(navData, sizeof(unsigned char), navDataSize, file);
                fclose(file);
            }
        }
    }

    void MapBuilder::buildTransports()
    {
        // List of MO Transport gameobjects
        buildGameObject("Transportship.wmo.vmap", 3015);
        buildGameObject("Transport_Zeppelin.wmo.vmap", 3031);
        buildGameObject("Transportship_Ne.wmo.vmap", 7087);
        // List of Transport gameobjects
        buildGameObject("Elevatorcar.m2.vmap", 360);
        buildGameObject("Undeadelevator.m2.vmap", 455);
        // buildGameObject("Undeadelevatordoor.m2.vmo", 462); // no model on which to path
        buildGameObject("Ironforgeelevator.m2.vmap", 561);
        // buildGameObject("Ironforgeelevatordoor.m2.vmo", 562); // no model on which to path
        buildGameObject("Gnomeelevatorcar01.m2.vmap", 807);
        buildGameObject("Gnomeelevatorcar02.m2.vmap", 808);
        buildGameObject("Gnomeelevatorcar03.m2.vmap", 827);
        buildGameObject("Gnomeelevatorcar03.m2.vmap", 852);
        buildGameObject("Gnomehutelevator.m2.vmap", 1587);
        buildGameObject("Burningsteppselevator.m2.vmap", 2454);
        buildGameObject("Subwaycar.m2.vmap", 3831);
        // TBC+
        buildGameObject("Ancdrae_Elevatorpiece.m2.vmap", 7026);
        buildGameObject("Mushroombase_Elevator.m2.vmap", 7028);
        buildGameObject("Cf_Elevatorplatform.m2.vmap", 7043);
        buildGameObject("Cf_Elevatorplatform_Small.m2.vmap", 7060);
        buildGameObject("Factoryelevator.m2.vmap", 7077);
        buildGameObject("Ancdrae_Elevatorpiece_Netherstorm.m2.vmap", 7163);
    }

    /**************************************************************************/
    void MapBuilder::getGridBounds(uint32 mapID, uint32& minX, uint32& minY, uint32& maxX, uint32& maxY)
    {
        maxX = INT_MAX;
        maxY = INT_MAX;
        minX = INT_MIN;
        minY = INT_MIN;

        float bmin[3] = { 0, 0, 0 };
        float bmax[3] = { 0, 0, 0 };
        float lmin[3] = { 0, 0, 0 };
        float lmax[3] = { 0, 0, 0 };
        MeshData meshData;

        // make sure we process maps which don't have tiles
        // initialize the static tree, which loads WDT models
        if (!m_terrainBuilder->loadVMap(mapID, 64, 64, meshData))
            return;

        // get the coord bounds of the model data
        if (meshData.solidVerts.size() + meshData.liquidVerts.size() == 0)
            return;

        // get the coord bounds of the model data
        if (meshData.solidVerts.size() && meshData.liquidVerts.size())
        {
            rcCalcBounds(meshData.solidVerts.getCArray(), meshData.solidVerts.size() / 3, bmin, bmax);
            rcCalcBounds(meshData.liquidVerts.getCArray(), meshData.liquidVerts.size() / 3, lmin, lmax);
            rcVmin(bmin, lmin);
            rcVmax(bmax, lmax);
        }
        else if (meshData.solidVerts.size())
            rcCalcBounds(meshData.solidVerts.getCArray(), meshData.solidVerts.size() / 3, bmin, bmax);
        else
            rcCalcBounds(meshData.liquidVerts.getCArray(), meshData.liquidVerts.size() / 3, lmin, lmax);

        // convert coord bounds to grid bounds
        maxX = 32 - bmin[0] / GRID_SIZE;
        maxY = 32 - bmin[2] / GRID_SIZE;
        minX = 32 - bmax[0] / GRID_SIZE;
        minY = 32 - bmax[2] / GRID_SIZE;
    }

    /**************************************************************************/
    void MapBuilder::buildSingleTile(uint32 mapID, uint32 tileX, uint32 tileY)
    {
        getTileList(mapID).insert(PackAscentTileID(tileX, tileY));

        dtNavMesh* navMesh = NULL;
        buildNavMesh(mapID, navMesh);
        if (!navMesh)
        {
            printf("[Map %03i] Failed creating navmesh!                   \n", mapID);
            return;
        }

        buildTile(mapID, tileX, tileY, navMesh, 1, 1);
        dtFreeNavMesh(navMesh);
    }

    /**************************************************************************/
    void MapBuilder::buildMap(uint32 mapID)
    {
        printf("Building map %03u:                                    \n", mapID);

        std::set<uint32>& tiles = getTileList(mapID);

        // make sure we process maps which don't have tiles
        if (!tiles.size())
        {
            // convert coord bounds to grid bounds
            uint32 minX, minY, maxX, maxY;
            getGridBounds(mapID, minX, minY, maxX, maxY);

            // add all tiles within bounds to tile list.
            for (uint32 i = minX; i <= maxX; ++i)
                for (uint32 j = minY; j <= maxY; ++j)
                    tiles.insert(PackAscentTileID(i, j));
        }

        if (!tiles.size())
            return;

        // build navMesh
        dtNavMesh* navMesh = nullptr;
        buildNavMesh(mapID, navMesh);
        if (!navMesh)
        {
            printf("[Map %03i] Failed creating navmesh!                   \n", mapID);
            return;
        }

        // now start building mmtiles for each tile
        printf("[Map %03i] We have %u tiles.                          \n", mapID, uint32(tiles.size()));

        uint32 currentTile = 0;
        for (std::set<uint32>::iterator it = tiles.begin(); it != tiles.end(); ++it)
        {
            currentTile++;
            uint32 tileX, tileY;

            // unpack tile coords
            UnpackAscentTileID((*it), tileX, tileY);

            if (shouldSkipTile(mapID, tileX, tileY))
                continue;

            // Make a copy of the original navMesh object to work on a separate
            // thread since "the data should not be reused in other nav meshes"
            // (see dtNavMesh::addTile description)
            dtNavMesh* navMeshCopy = dtAllocNavMesh();
            dtStatus dtResult = navMeshCopy->init(navMesh->getParams());
            if (dtStatusFailed(dtResult))
            {
                printf("[Map %03i] Failed to copy navmesh!                   \n", mapID);
                printf("%s\n", GetDTErrorReason(dtResult));
                continue;
            }

            // passing by value
            auto builder = [=]()
            {
                // build tile with copy version of the navmesh
                buildTile(mapID, tileX, tileY, navMeshCopy, currentTile, uint32(tiles.size()));

                // free this navmesh
                dtFreeNavMesh(navMeshCopy);
            };

            m_taskQueue->PushWork(builder, mapID);
        }

        dtFreeNavMesh(navMesh);
    }

    /**************************************************************************/
    void MapBuilder::buildTile(uint32 mapID, uint32 tileX, uint32 tileY, dtNavMesh* navMesh, uint32 curTile, uint32 tileCount)
    {
        std::lock_guard<std::mutex> buildGuard(m_buildMutex);

        printf("[Map %03i] Building tile [%02u,%02u] (%02u / %02u)    \n", mapID, tileX, tileY, curTile, tileCount);

        MeshData meshData;

        // get heightmap data
        m_terrainBuilder->loadMap(mapID, tileX, tileY, meshData);

        // get model data
        const bool loadedVMap = m_terrainBuilder->loadVMap(mapID, tileX, tileY, meshData);
        printf("[Map %03i] Tile [%02u,%02u] geometry: vmap=%u solidVerts=%u solidTris=%u liquidVerts=%u liquidTris=%u\n",
            mapID, tileX, tileY, loadedVMap ? 1u : 0u, uint32(meshData.solidVerts.size() / 3), uint32(meshData.solidTris.size() / 3),
            uint32(meshData.liquidVerts.size() / 3), uint32(meshData.liquidTris.size() / 3));

        // if there is no data, give up now
        if (!meshData.solidVerts.size() && !meshData.liquidVerts.size())
        {
            printf("[Map %03i] Tile [%02u,%02u] skipped: no source geometry\n", mapID, tileX, tileY);
            return;
        }

        // remove unused vertices
        TerrainBuilder::cleanVertices(meshData.solidVerts, meshData.solidTris);
        TerrainBuilder::cleanVertices(meshData.liquidVerts, meshData.liquidTris);
        printf("[Map %03i] Tile [%02u,%02u] cleaned geometry: solidVerts=%u solidTris=%u liquidVerts=%u liquidTris=%u\n",
            mapID, tileX, tileY, uint32(meshData.solidVerts.size() / 3), uint32(meshData.solidTris.size() / 3),
            uint32(meshData.liquidVerts.size() / 3), uint32(meshData.liquidTris.size() / 3));

        // gather all mesh data for final data check, and bounds calculation
        G3D::Array<float> allVerts;
        allVerts.append(meshData.liquidVerts);
        allVerts.append(meshData.solidVerts);

        if (!allVerts.size())
        {
            printf("[Map %03i] Tile [%02u,%02u] skipped: no vertices remained after cleanup\n", mapID, tileX, tileY);
            return;
        }

        // get bounds of current tile
        float bmin[3], bmax[3];
        getTileBounds(tileX, tileY, allVerts.getCArray(), allVerts.size() / 3, bmin, bmax);

        m_terrainBuilder->loadOffMeshConnections(mapID, tileX, tileY, meshData, m_offMeshFilePath);

        // build navmesh tile
        buildMoveMapTile(mapID, tileX, tileY, meshData, bmin, bmax, navMesh);
    }

    /**************************************************************************/
    void MapBuilder::buildNavMesh(uint32 mapID, dtNavMesh*& navMesh)
    {
        std::set<uint32>& tiles = getTileList(mapID);

        if (tiles.empty())
        {
            printf("[Map %03i] Cannot create navmesh root: no tiles were discovered or requested.\n", mapID);
            return;
        }

        // old code for non-statically assigned bitmask sizes:
        ///*** calculate number of bits needed to store tiles & polys ***/
        //int tileBits = dtIlog2(dtNextPow2(tiles.size()));
        //if (tileBits < 1) tileBits = 1;                                     // need at least one bit!
        //int polyBits = sizeof(dtPolyRef)*8 - SALT_MIN_BITS - tileBits;

        uint32 fullMinX = 64, fullMinY = 64, fullMaxX = 0, fullMaxY = 0;
        getGridBounds(mapID, fullMinX, fullMinY, fullMaxX, fullMaxY);

        const bool hasFullBounds = fullMinX <= 63 && fullMinY <= 63 && fullMaxX <= 63 && fullMaxY <= 63 && fullMinX <= fullMaxX && fullMinY <= fullMaxY;
        // Detour's 32-bit poly refs require at least 10 salt bits in
        // dtNavMesh::init(), so large maps cannot always keep the default
        // 14 poly bits. Size the root navmesh by the number of tiles we will
        // actually load and reduce poly bits only when necessary to keep the
        // root valid for continent-scale maps.
        int tileCount = int(tiles.size());
        if (tileCount < 1)
            tileCount = 1;

        int maxTiles = int(dtNextPow2((unsigned int)tileCount));
        int tileBits = dtIlog2((unsigned int)maxTiles);
        if (tileBits < 1)
            tileBits = 1;

        // Keep enough poly-ref space for dense continent city tiles. The legacy
        // 22-bit budget was too small for Stormwind-class tiles and caused
        // addTile() to reject otherwise valid navmesh data once a tile exceeded
        // 4096 polys.
        int polyBits = 14;
        int saltBits = int(sizeof(dtPolyRef) * 8) - tileBits - polyBits;
        if (saltBits < 10)
        {
            polyBits = int(sizeof(dtPolyRef) * 8) - tileBits - 10;
            saltBits = 10;
        }
        if (polyBits < 1)
            polyBits = 1;

        int maxPolysPerTile = 1 << polyBits;

        /***          calculate bounds of map         ***/

        uint32 tileXMin = 64, tileYMin = 64, tileXMax = 0, tileYMax = 0, tileX, tileY;
        for (std::set<uint32>::iterator it = tiles.begin(); it != tiles.end(); ++it)
        {
            UnpackAscentTileID((*it), tileX, tileY);

            if (tileX > tileXMax)
                tileXMax = tileX;
            else if (tileX < tileXMin)
                tileXMin = tileX;

            if (tileY > tileYMax)
                tileYMax = tileY;
            else if (tileY < tileYMin)
                tileYMin = tileY;
        }

        float bmin[3], bmax[3];
        if (hasFullBounds)
            getTileBounds(fullMaxX, fullMaxY, NULL, 0, bmin, bmax);
        else
            getTileBounds(tileXMax, tileYMax, NULL, 0, bmin, bmax);

        printf("[Map %03i] Navmesh root bounds: tiles=%u requestedRange=[%u,%u]-[%u,%u] rootRange=[%u,%u]-[%u,%u]\n",
            mapID, uint32(tiles.size()), tileXMin, tileYMin, tileXMax, tileYMax,
            hasFullBounds ? fullMinX : tileXMin, hasFullBounds ? fullMinY : tileYMin,
            hasFullBounds ? fullMaxX : tileXMax, hasFullBounds ? fullMaxY : tileYMax);
        printf("[Map %03i] Navmesh root params: tileCount=%d maxTiles=%d tileBits=%d maxPolysPerTile=%d polyBits=%d\n",
            mapID, tileCount, maxTiles, tileBits, maxPolysPerTile, polyBits);

        /***       now create the navmesh       ***/

        // navmesh creation params
        dtNavMeshParams navMeshParams;
        memset(&navMeshParams, 0, sizeof(dtNavMeshParams));
        navMeshParams.tileWidth = GRID_SIZE;
        navMeshParams.tileHeight = GRID_SIZE;
        rcVcopy(navMeshParams.orig, bmin);
        navMeshParams.maxTiles = maxTiles;
        navMeshParams.maxPolys = maxPolysPerTile;

        navMesh = dtAllocNavMesh();
        printf("[Map %03i] Creating navMesh...                        \r", mapID);
        dtStatus dtResult = navMesh->init(&navMeshParams);
        if (dtStatusFailed(dtResult))
        {
            printf("[Map %03i] Failed creating navmesh!                   \n", mapID);
            printf("%s\n", GetDTErrorReason(dtResult));
            return;
        }

        char fileName[1024];
        sprintf(fileName, "%s/mmaps/%03u.mmap", m_workdir, mapID);

        FILE* file = fopen(fileName, "wb");
        if (!file)
        {
            dtFreeNavMesh(navMesh);
            char message[1024];
            sprintf(message, "[Map %03i] Failed to open %s for writing!             \n", mapID, fileName);
            perror(message);
            return;
        }

        // now that we know navMesh params are valid, we can write them to file
        fwrite(&navMeshParams, sizeof(dtNavMeshParams), 1, file);
        fclose(file);
    }

    /**************************************************************************/
    void MapBuilder::buildMoveMapTile(uint32 mapID, uint32 tileX, uint32 tileY,
                                      MeshData& meshData, float bmin[3], float bmax[3],
                                      dtNavMesh* navMesh)
    {
        // console output
        char tileString[20];
        sprintf(tileString, "[Map %03i] [%02i,%02i]: ", mapID, tileX, tileY);
        printf("%s Building movemap tiles...                          \r", tileString);

        IntermediateValues iv(m_workdir);

        float* tVerts = meshData.solidVerts.getCArray();
        int tVertCount = meshData.solidVerts.size() / 3;
        int* tTris = meshData.solidTris.getCArray();
        int tTriCount = meshData.solidTris.size() / 3;

        float* lVerts = meshData.liquidVerts.getCArray();
        int lVertCount = meshData.liquidVerts.size() / 3;
        int* lTris = meshData.liquidTris.getCArray();
        int lTriCount = meshData.liquidTris.size() / 3;
        uint8* lTriFlags = meshData.liquidType.getCArray();
        float inspectNav[3] = { 0.0f, 0.0f, 0.0f };
        const bool inspectThisTile = m_inspectPoint.enabled && (ToNavMeshCoords(m_inspectPoint.worldX, m_inspectPoint.worldY, m_inspectPoint.worldZ, inspectNav), TileBoundsContainInspectPoint(bmin, bmax, inspectNav));

        if (inspectThisTile)
        {
            printf("%s Inspect point world=%0.3f,%0.3f,%0.3f nav=%0.3f,%0.3f,%0.3f tileBoundsMin=%0.3f,%0.3f,%0.3f tileBoundsMax=%0.3f,%0.3f,%0.3f\n",
                tileString, m_inspectPoint.worldX, m_inspectPoint.worldY, m_inspectPoint.worldZ, inspectNav[0], inspectNav[1], inspectNav[2],
                bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]);
            LogInputGeometryAroundInspectPoint(tileString, tVerts, tTriCount, tTris, lVerts, lTriCount, lTris, inspectNav);
        }
        else if (m_inspectPoint.enabled)
        {
            printf("%s Inspect point skipped tile containment: world=%0.3f,%0.3f,%0.3f nav=%0.3f,%0.3f,%0.3f tileBoundsMin=%0.3f,%0.3f,%0.3f tileBoundsMax=%0.3f,%0.3f,%0.3f\n",
                tileString, m_inspectPoint.worldX, m_inspectPoint.worldY, m_inspectPoint.worldZ, inspectNav[0], inspectNav[1], inspectNav[2],
                bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]);
        }

        rcConfig config;
        memset(&config, 0, sizeof(rcConfig));
        config = getTileConfig(mapID, tileX, tileY);

        rcVcopy(config.bmin, bmin);
        rcVcopy(config.bmax, bmax);

        // this sets the dimensions of the heightfield - should maybe happen before border padding
        rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);

        // allocate subregions : tiles
        Tile* tiles = new Tile[TILES_PER_MAP * TILES_PER_MAP];

        // Initialize per tile config.
        rcConfig tileCfg;
        memcpy(&tileCfg, &config, sizeof(rcConfig));
        tileCfg.width = config.tileSize + config.borderSize * 2;
        tileCfg.height = config.tileSize + config.borderSize * 2;

        // build all tiles
        for (int y = 0; y < TILES_PER_MAP; ++y)
        {
            for (int x = 0; x < TILES_PER_MAP; ++x)
            {
                Tile& tile = tiles[x + y * TILES_PER_MAP];

                // Calculate the per tile bounding box.
                tileCfg.bmin[0] = config.bmin[0] + x * float(config.tileSize * config.cs);
                tileCfg.bmin[2] = config.bmin[2] + y * float(config.tileSize * config.cs);
                tileCfg.bmax[0] = config.bmin[0] + (x + 1) * float(config.tileSize * config.cs);
                tileCfg.bmax[2] = config.bmin[2] + (y + 1) * float(config.tileSize * config.cs);

                tileCfg.bmin[0] -= tileCfg.borderSize * tileCfg.cs;
                tileCfg.bmin[2] -= tileCfg.borderSize * tileCfg.cs;
                tileCfg.bmax[0] += tileCfg.borderSize * tileCfg.cs;
                tileCfg.bmax[2] += tileCfg.borderSize * tileCfg.cs;

                buildCommonTile(tileString, tile, tileCfg, tVerts, tVertCount, tTris, tTriCount, lVerts, lVertCount, lTris, lTriCount, lTriFlags);
            }
        }

        // merge per tile poly and detail meshes
        rcPolyMesh** pmmerge = new rcPolyMesh*[TILES_PER_MAP * TILES_PER_MAP];
        rcPolyMeshDetail** dmmerge = new rcPolyMeshDetail*[TILES_PER_MAP * TILES_PER_MAP];

        int nmerge = 0;
        for (int y = 0; y < TILES_PER_MAP; ++y)
        {
            for (int x = 0; x < TILES_PER_MAP; ++x)
            {
                Tile& tile = tiles[x + y * TILES_PER_MAP];
                if (tile.pmesh)
                {
                    pmmerge[nmerge] = tile.pmesh;
                    dmmerge[nmerge] = tile.dmesh;
                    nmerge++;
                }
            }
        }

        iv.polyMesh = rcAllocPolyMesh();
        if (!iv.polyMesh)
        {
            printf("%s alloc iv.polyMesh FIALED!                          \r", tileString);
            delete[] pmmerge;
            delete[] dmmerge;
            delete[] tiles;
            return;
        }
        rcContext mergeContext(false);
        rcMergePolyMeshes(&mergeContext, pmmerge, nmerge, *iv.polyMesh);

        iv.polyMeshDetail = rcAllocPolyMeshDetail();
        if (!iv.polyMeshDetail)
        {
            printf("%s alloc m_dmesh FIALED!                              \r", tileString);
            delete[] pmmerge;
            delete[] dmmerge;
            delete[] tiles;
            return;
        }
        rcMergePolyMeshDetails(&mergeContext, dmmerge, nmerge, *iv.polyMeshDetail);

        // free things up
        delete [] pmmerge;
        delete [] dmmerge;
        delete [] tiles;

        printf("%s Recast merge: polys=%u detailPolys=%u detailVerts=%u\n",
            tileString, iv.polyMesh ? uint32(iv.polyMesh->npolys) : 0u,
            iv.polyMeshDetail ? uint32(iv.polyMeshDetail->ntris) : 0u,
            iv.polyMeshDetail ? uint32(iv.polyMeshDetail->nverts) : 0u);

        // set polygons as walkable
        // TODO: special flags for DYNAMIC polygons, ie surfaces that can be turned on and off
        for (int i = 0; i < iv.polyMesh->npolys; ++i)
        {
            if (uint8 area = iv.polyMesh->areas[i] & NAV_AREA_ALL_MASK)
            {
                if (area >= NAV_AREA_MIN_VALUE)
                    iv.polyMesh->flags[i] = 1 << (NAV_AREA_MAX_VALUE - area);
                else
                    iv.polyMesh->flags[i] = NAV_GROUND; // TODO: these will be dynamic in future
            }
        }

        // setup mesh parameters
        dtNavMeshCreateParams params;
        memset(&params, 0, sizeof(params));
        params.verts = iv.polyMesh->verts;
        params.vertCount = iv.polyMesh->nverts;
        params.polys = iv.polyMesh->polys;
        params.polyAreas = iv.polyMesh->areas;
        params.polyFlags = iv.polyMesh->flags;
        params.polyCount = iv.polyMesh->npolys;
        params.nvp = iv.polyMesh->nvp;
        params.detailMeshes = iv.polyMeshDetail->meshes;
        params.detailVerts = iv.polyMeshDetail->verts;
        params.detailVertsCount = iv.polyMeshDetail->nverts;
        params.detailTris = iv.polyMeshDetail->tris;
        params.detailTriCount = iv.polyMeshDetail->ntris;

        params.offMeshConVerts = meshData.offMeshConnections.getCArray();
        params.offMeshConCount = meshData.offMeshConnections.size() / 6;
        params.offMeshConRad = meshData.offMeshConnectionRads.getCArray();
        params.offMeshConDir = meshData.offMeshConnectionDirs.getCArray();
        params.offMeshConAreas = meshData.offMeshConnectionsAreas.getCArray();
        params.offMeshConFlags = meshData.offMeshConnectionsFlags.getCArray();

        params.walkableHeight = BASE_UNIT_DIM * config.walkableHeight;
        params.walkableRadius = BASE_UNIT_DIM * config.walkableRadius;
        params.walkableClimb = BASE_UNIT_DIM * config.walkableClimb;

        params.tileX = (((bmin[0] + bmax[0]) / 2) - navMesh->getParams()->orig[0]) / GRID_SIZE;
        params.tileY = (((bmin[2] + bmax[2]) / 2) - navMesh->getParams()->orig[2]) / GRID_SIZE;
        rcVcopy(params.bmin, bmin);
        rcVcopy(params.bmax, bmax);
        params.cs = config.cs;
        params.ch = config.ch;
        params.tileLayer = 0;
        params.buildBvTree = true;

        // will hold final navmesh
        unsigned char* navData = NULL;
        int navDataSize = 0;

        do
        {
            // these values are checked within dtCreateNavMeshData - handle them here
            // so we have a clear error messages
            if (params.nvp > DT_VERTS_PER_POLYGON)
            {
                printf("%s Invalid verts-per-polygon value!                   \n", tileString);
                continue;
            }
            if (params.vertCount >= 0xffff)
            {
                printf("%s Too many vertices!                                 \n", tileString);
                continue;
            }
            if (!params.vertCount || !params.verts)
            {
                // occurs mostly when adjacent tiles have models
                // loaded but those models don't span into this tile

                // message is an annoyance
                printf("%s No vertices to build tile!                         \n", tileString);
                continue;
            }
            if (!params.polyCount || !params.polys ||
                    TILES_PER_MAP * TILES_PER_MAP == params.polyCount)
            {
                // we have flat tiles with no actual geometry - don't build those, its useless
                // keep in mind that we do output those into debug info
                // drop tiles with only exact count - some tiles may have geometry while having less tiles
                printf("%s No polygons to build on tile!                      \n", tileString);
                continue;
            }
            if (!params.detailMeshes || !params.detailVerts || !params.detailTris)
            {
                printf("%s No detail mesh to build tile!                      \n", tileString);
                continue;
            }

            printf("%s Building navmesh tile...                           \r", tileString);
            if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
            {
                printf("%s Failed building navmesh tile!                      \n", tileString);
                continue;
            }

            dtTileRef tileRef = 0;
            printf("%s Adding tile to navmesh...                          \r", tileString);
            // DT_TILE_FREE_DATA tells detour to unallocate memory when the tile
            // is removed via removeTile()
            dtStatus dtResult = navMesh->addTile(navData, navDataSize, DT_TILE_FREE_DATA, 0, &tileRef);
            if (!tileRef || dtStatusFailed(dtResult))
            {
                printf("%s Failed adding tile to navmesh!                     \n", tileString);
                printf("%s\n", GetDTErrorReason(dtResult));
                continue;
            }

            const dtMeshTile* addedTile = navMesh->getTileByRef(tileRef);
            if (addedTile != NULL && addedTile->header != NULL)
            {
                printf("%s Tile header: x=%d y=%d layer=%d polys=%d verts=%d bmin=%0.3f,%0.3f,%0.3f bmax=%0.3f,%0.3f,%0.3f\n",
                    tileString,
                    addedTile->header->x, addedTile->header->y, addedTile->header->layer,
                    addedTile->header->polyCount, addedTile->header->vertCount,
                    addedTile->header->bmin[0], addedTile->header->bmin[1], addedTile->header->bmin[2],
                    addedTile->header->bmax[0], addedTile->header->bmax[1], addedTile->header->bmax[2]);
            }

            if (m_inspectPoint.enabled)
                LogInspectPointResult(tileString, mapID, tileX, tileY, navMesh, inspectNav);

            // file output
            char fileName[1024];
            sprintf(fileName, "%s/mmaps/%03u_%02i_%02i.mmtile", m_workdir, mapID, tileY, tileX);
            FILE* file = fopen(fileName, "wb");
            if (!file)
            {
                char message[1024];
                sprintf(message, "[Map %03i] Failed to open %s for writing!             \n", mapID, fileName);
                perror(message);
                navMesh->removeTile(tileRef, NULL, NULL);
                continue;
            }

            printf("%s Writing to file...                                 \r", tileString);

            // write header
            MmapTileHeader header;
            header.size = uint32(navDataSize);
            header.usesLiquids = m_terrainBuilder->usesLiquids() ? 1 : 0;
            fwrite(&header, sizeof(MmapTileHeader), 1, file);

            // write data
            fwrite(navData, sizeof(unsigned char), navDataSize, file);
            fclose(file);
            printf("%s Wrote mmap tile %03u_%02i_%02i.mmtile size=%u\n", tileString, mapID, tileY, tileX, uint32(navDataSize));

            // now that tile is written to disk, we can unload it
            navMesh->removeTile(tileRef, nullptr, nullptr);
        }
        while (0);

        if (m_debug)
        {
            // restore padding so that the debug visualization is correct
            for (int i = 0; i < iv.polyMesh->nverts; ++i)
            {
                unsigned short* v = &iv.polyMesh->verts[i * 3];
                v[0] += (unsigned short)config.borderSize;
                v[2] += (unsigned short)config.borderSize;
            }

            iv.generateObjFile(mapID, tileX, tileY, meshData);
            iv.writeIV(mapID, tileX, tileY);
        }
    }

    bool MapBuilder::buildCommonTile(const char* tileString, Tile& tile, rcConfig& tileCfg, float* tVerts, int tVertCount, int* tTris, int tTriCount, float* lVerts, int lVertCount,
                                     int* lTris, int lTriCount, uint8* lTriFlags)
    {
        rcContext buildContext(false);
        float inspectNav[3] = { 0.0f, 0.0f, 0.0f };
        const bool inspectThisSubTile = m_inspectPoint.enabled && (ToNavMeshCoords(m_inspectPoint.worldX, m_inspectPoint.worldY, m_inspectPoint.worldZ, inspectNav), TileBoundsContainInspectPoint(tileCfg.bmin, tileCfg.bmax, inspectNav));

        // Build heightfield for walkable area
        tile.solid = rcAllocHeightfield();
        if (!tile.solid || !rcCreateHeightfield(&buildContext, *tile.solid, tileCfg.width, tileCfg.height, tileCfg.bmin, tileCfg.bmax, tileCfg.cs, tileCfg.ch))
        {
            printf("%s Failed building heightfield!                       \n", tileString);
            return false;
        }

        // mark all walkable tiles, both liquids and solids
        unsigned char* triFlags = new unsigned char[tTriCount];
        memset(triFlags, NAV_AREA_GROUND, tTriCount * sizeof(unsigned char));
        rcClearUnwalkableTriangles(&buildContext, tileCfg.walkableSlopeAngle, tVerts, tVertCount, tTris, tTriCount, triFlags);

        // mark almost unwalkable triangles with steep flag
        rcModAlmostUnwalkableTriangles(&buildContext, 50.0f, tVerts, tVertCount, tTris, tTriCount, triFlags);

        if (inspectThisSubTile)
        {
            uint32 nearbyInputTris = 0;
            uint32 nearbyWalkableTris = 0;
            uint32 nearbySteepTris = 0;
            for (int i = 0; i < tTriCount; ++i)
            {
                if (!TriangleNearInspectPoint(tVerts, tTris, i, inspectNav, m_inspectPoint.extents[0], m_inspectPoint.extents[1]))
                    continue;

                ++nearbyInputTris;
                if (triFlags[i] & RC_WALKABLE_AREA)
                    ++nearbyWalkableTris;
                if (triFlags[i] == NAV_AREA_GROUND_STEEP)
                    ++nearbySteepTris;
            }

            printf("%s Inspect subtile pre-raster: nearbyInputTris=%u nearbyWalkableTris=%u nearbySteepTris=%u boundsMin=%0.3f,%0.3f,%0.3f boundsMax=%0.3f,%0.3f,%0.3f\n",
                tileString, nearbyInputTris, nearbyWalkableTris, nearbySteepTris,
                tileCfg.bmin[0], tileCfg.bmin[1], tileCfg.bmin[2], tileCfg.bmax[0], tileCfg.bmax[1], tileCfg.bmax[2]);
        }

        rcRasterizeTriangles(&buildContext, tVerts, tVertCount, tTris, triFlags, tTriCount, *tile.solid, tileCfg.walkableClimb);
        delete[] triFlags;

        rcFilterLowHangingWalkableObstacles(&buildContext, tileCfg.walkableClimb, *tile.solid);
        rcFilterLedgeSpans(&buildContext, tileCfg.walkableHeight, tileCfg.walkableClimb, *tile.solid);
        rcFilterWalkableLowHeightSpans(&buildContext, tileCfg.walkableHeight, *tile.solid);
        if (lVerts)
            rcRasterizeTriangles(&buildContext, lVerts, lVertCount, lTris, lTriFlags, lTriCount, *tile.solid, tileCfg.walkableClimb);

        // compact heightfield spans
        tile.chf = rcAllocCompactHeightfield();
        if (!tile.chf || !rcBuildCompactHeightfield(&buildContext, tileCfg.walkableHeight, tileCfg.walkableClimb, *tile.solid, *tile.chf))
        {
            printf("%s Failed compacting heightfield!                     \n", tileString);
            return false;
        }

        // build polymesh intermediates
        if (!rcErodeWalkableArea(&buildContext, tileCfg.walkableRadius, *tile.chf))
        {
            printf("%s Failed eroding area!                               \n", tileString);
            return false;
        }

        if (!rcMedianFilterWalkableArea(&buildContext, *tile.chf))
        {
            printf("%s Failed filtering area!                             \n", tileString);
            return false;
        }

        if (!rcBuildDistanceField(&buildContext, *tile.chf))
        {
            printf("%s Failed building distance field!                    \n", tileString);
            return false;
        }

        if (!rcBuildRegions(&buildContext, *tile.chf, tileCfg.borderSize, tileCfg.minRegionArea, tileCfg.mergeRegionArea))
        {
            printf("%s Failed building regions!                           \n", tileString);
            return false;
        }

        tile.cset = rcAllocContourSet();
        if (!tile.cset || !rcBuildContours(&buildContext, *tile.chf, tileCfg.maxSimplificationError, tileCfg.maxEdgeLen, *tile.cset))
        {
            printf("%s Failed building contours!                          \n", tileString);
            return false;
        }

        // build polymesh
        tile.pmesh = rcAllocPolyMesh();
        if (!tile.pmesh || !rcBuildPolyMesh(&buildContext, *tile.cset, tileCfg.maxVertsPerPoly, *tile.pmesh))
        {
            printf("%s Failed building polymesh!                          \n", tileString);
            return false;
        }

        tile.dmesh = rcAllocPolyMeshDetail();
        if (!tile.dmesh || !rcBuildPolyMeshDetail(&buildContext, *tile.pmesh, *tile.chf, tileCfg.detailSampleDist, tileCfg.detailSampleMaxError, *tile.dmesh))
        {
            printf("%s Failed building polymesh detail!                   \n", tileString);
            return false;
        }

        if (inspectThisSubTile)
        {
            printf("%s Inspect subtile final: compactSpans=%u maxRegions=%u contours=%u polys=%u detailMeshes=%u detailTris=%u\n",
                tileString,
                tile.chf ? uint32(tile.chf->spanCount) : 0u,
                tile.chf ? uint32(tile.chf->maxRegions) : 0u,
                tile.cset ? uint32(tile.cset->nconts) : 0u,
                tile.pmesh ? uint32(tile.pmesh->npolys) : 0u,
                tile.dmesh ? uint32(tile.dmesh->nmeshes) : 0u,
                tile.dmesh ? uint32(tile.dmesh->ntris) : 0u);
        }

        printf("%s Recast stats: hfSpans=%u compactSpans=%u compactRegions=%u contours=%u polyVerts=%u polys=%u detailMeshes=%u detailTris=%u\n",
            tileString,
            CountHeightfieldSpans(tile.solid),
            tile.chf ? uint32(tile.chf->spanCount) : 0u,
            tile.chf ? uint32(tile.chf->maxRegions) : 0u,
            tile.cset ? uint32(tile.cset->nconts) : 0u,
            tile.pmesh ? uint32(tile.pmesh->nverts) : 0u,
            tile.pmesh ? uint32(tile.pmesh->npolys) : 0u,
            tile.dmesh ? uint32(tile.dmesh->nmeshes) : 0u,
            tile.dmesh ? uint32(tile.dmesh->ntris) : 0u);

        // free those up
        // we may want to keep them in the future for debug
        // but right now, we don't have the code to merge them
        rcFreeHeightField(tile.solid);
        tile.solid = nullptr;
        rcFreeCompactHeightfield(tile.chf);
        tile.chf = nullptr;
        rcFreeContourSet(tile.cset);
        tile.cset = nullptr;
        return true;
    }

    /**************************************************************************/
    void MapBuilder::getTileBounds(uint32 tileX, uint32 tileY, float* verts, int vertCount, float* bmin, float* bmax)
    {
        // this is for elevation
        if (verts && vertCount)
            rcCalcBounds(verts, vertCount, bmin, bmax);
        else
        {
            bmin[1] = FLT_MIN;
            bmax[1] = FLT_MAX;
        }

        // this is for width and depth
        bmax[0] = (32 - int(tileX)) * GRID_SIZE;
        bmax[2] = (32 - int(tileY)) * GRID_SIZE;
        bmin[0] = bmax[0] - GRID_SIZE;
        bmin[2] = bmax[2] - GRID_SIZE;
    }

    uint32 MapBuilder::CountHeightfieldSpans(const rcHeightfield* solid) const
    {
        if (solid == NULL || solid->spans == NULL)
            return 0;

        uint32 spanCount = 0;
        const int cellCount = solid->width * solid->height;
        for (int i = 0; i < cellCount; ++i)
        {
            for (const rcSpan* span = solid->spans[i]; span != NULL; span = span->next)
                ++spanCount;
        }

        return spanCount;
    }

    bool MapBuilder::TileBoundsContainInspectPoint(float bmin[3], float bmax[3], float* inspectNav) const
    {
        return inspectNav[0] >= bmin[0] && inspectNav[0] <= bmax[0] &&
            inspectNav[1] >= bmin[1] && inspectNav[1] <= bmax[1] &&
            inspectNav[2] >= bmin[2] && inspectNav[2] <= bmax[2];
    }

    void MapBuilder::ToNavMeshCoords(float worldX, float worldY, float worldZ, float* out) const
    {
        out[0] = worldX;
        out[1] = worldZ;
        out[2] = worldY;
    }

    void MapBuilder::LogInspectPointResult(const char* tileString, uint32 mapID, uint32 tileX, uint32 tileY, dtNavMesh* navMesh, float* queryPoint) const
    {
        dtNavMeshQuery* query = dtAllocNavMeshQuery();
        if (query == NULL)
        {
            printf("%s Inspect point query alloc failed\n", tileString);
            return;
        }

        const dtStatus initStatus = query->init(navMesh, 2048);
        if (dtStatusFailed(initStatus))
        {
            printf("%s Inspect point query init failed (%s)\n", tileString, GetDTErrorReason(initStatus));
            dtFreeNavMeshQuery(query);
            return;
        }

        dtQueryFilter filter;
        filter.setIncludeFlags(0xFFFF);
        filter.setExcludeFlags(0);

        dtPolyRef nearestRef = 0;
        float closest[3] = { 0.0f, 0.0f, 0.0f };
        const dtStatus nearestStatus = query->findNearestPoly(queryPoint, m_inspectPoint.extents, &filter, &nearestRef, closest);

        static const int MAX_INSPECT_POLYS = 128;
        dtPolyRef nearbyPolys[MAX_INSPECT_POLYS];
        int nearbyPolyCount = 0;
        memset(nearbyPolys, 0, sizeof(nearbyPolys));
        const dtStatus nearbyStatus = query->queryPolygons(queryPoint, m_inspectPoint.extents, &filter, nearbyPolys, &nearbyPolyCount, MAX_INSPECT_POLYS);

        printf("%s Inspect point result: map=%03u tile=%02u,%02u nearestStatus=0x%08X nearestRef=%llu nearbyStatus=0x%08X nearbyPolys=%d extents=%0.3f,%0.3f,%0.3f\n",
            tileString, mapID, tileX, tileY, nearestStatus, (unsigned long long)nearestRef, nearbyStatus, nearbyPolyCount,
            m_inspectPoint.extents[0], m_inspectPoint.extents[1], m_inspectPoint.extents[2]);

        if (nearestRef != 0)
        {
            printf("%s Inspect point closest nav=%0.3f,%0.3f,%0.3f closest world=%0.3f,%0.3f,%0.3f\n",
                tileString, closest[0], closest[1], closest[2], closest[0], closest[2], closest[1]);
        }
        else
        {
            printf("%s Inspect point found no nearby walkable poly for world=%0.3f,%0.3f,%0.3f\n",
                tileString, m_inspectPoint.worldX, m_inspectPoint.worldY, m_inspectPoint.worldZ);
        }

        dtFreeNavMeshQuery(query);
    }

    bool MapBuilder::TriangleNearInspectPoint(const float* verts, const int* tris, int triIndex, const float* inspectNav, float horizontalRadius, float verticalRadius) const
    {
        const int* tri = &tris[triIndex * 3];
        float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
        float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;

        for (int i = 0; i < 3; ++i)
        {
            const float* v = &verts[tri[i] * 3];
            if (v[0] < minX) minX = v[0];
            if (v[1] < minY) minY = v[1];
            if (v[2] < minZ) minZ = v[2];
            if (v[0] > maxX) maxX = v[0];
            if (v[1] > maxY) maxY = v[1];
            if (v[2] > maxZ) maxZ = v[2];
        }

        if (inspectNav[0] < (minX - horizontalRadius) || inspectNav[0] > (maxX + horizontalRadius))
            return false;
        if (inspectNav[2] < (minZ - horizontalRadius) || inspectNav[2] > (maxZ + horizontalRadius))
            return false;
        if (inspectNav[1] < (minY - verticalRadius) || inspectNav[1] > (maxY + verticalRadius))
            return false;
        return true;
    }

    void MapBuilder::LogInputGeometryAroundInspectPoint(const char* tileString, float* tVerts, int tTriCount, int* tTris, float* lVerts, int lTriCount, int* lTris, float* inspectNav) const
    {
        uint32 nearbySolidTris = 0;
        uint32 nearbyLiquidTris = 0;
        uint32 nearbyUpwardSolidTris = 0;
        uint32 nearbyDownwardSolidTris = 0;
        for (int i = 0; i < tTriCount; ++i)
        {
            if (TriangleNearInspectPoint(tVerts, tTris, i, inspectNav, m_inspectPoint.extents[0], m_inspectPoint.extents[1]))
            {
                ++nearbySolidTris;
                const int* tri = &tTris[i * 3];
                const float* a = &tVerts[tri[0] * 3];
                const float* b = &tVerts[tri[1] * 3];
                const float* c = &tVerts[tri[2] * 3];
                const float e0x = b[0] - a[0];
                const float e0y = b[1] - a[1];
                const float e0z = b[2] - a[2];
                const float e1x = c[0] - a[0];
                const float e1y = c[1] - a[1];
                const float e1z = c[2] - a[2];
                const float normalY = (e0z * e1x) - (e0x * e1z);
                if (normalY >= 0.0f)
                    ++nearbyUpwardSolidTris;
                else
                    ++nearbyDownwardSolidTris;
            }
        }

        for (int i = 0; i < lTriCount; ++i)
        {
            if (TriangleNearInspectPoint(lVerts, lTris, i, inspectNav, m_inspectPoint.extents[0], m_inspectPoint.extents[1]))
                ++nearbyLiquidTris;
        }

        printf("%s Inspect point geometry overlap: nearbySolidTris=%u nearbyUpwardSolidTris=%u nearbyDownwardSolidTris=%u nearbyLiquidTris=%u inspectExtents=%0.3f,%0.3f,%0.3f\n",
            tileString, nearbySolidTris, nearbyUpwardSolidTris, nearbyDownwardSolidTris, nearbyLiquidTris,
            m_inspectPoint.extents[0], m_inspectPoint.extents[1], m_inspectPoint.extents[2]);
    }

    /**************************************************************************/
    bool MapBuilder::shouldSkipMap(uint32 mapID)
    {
        if (m_skipContinents)
            switch (mapID)
            {
                case 0:
                case 1:
                case 530:
                    return true;
                default:
                    break;
            }

        if (m_skipJunkMaps)
            switch (mapID)
            {
                case 13:    // test.wdt
                case 25:    // ScottTest.wdt
                case 29:    // Test.wdt
                case 42:    // Colin.wdt
                case 169:   // EmeraldDream.wdt (unused, and very large)
                case 451:   // development.wdt
                    return true;
                default:
                    if (isTransportMap(mapID))
                        return true;
                    break;
            }

        if (m_skipBattlegrounds)
            switch (mapID)
            {
                case 30:    // AV
                case 37:    // ?
                case 489:   // WSG
                case 529:   // AB
                case 566:   // EotS
                    return true;
                default:
                    break;
            }

        return false;
    }

    /**************************************************************************/
    bool MapBuilder::isTransportMap(uint32 mapID)
    {
        switch (mapID)
        {
            // transport maps
            case 582:
            case 584:
            case 586:
            case 587:
            case 588:
            case 589:
            case 590:
            case 591:
            case 593:
                return true;
            default:
                return false;
        }
    }

    /**************************************************************************/
    bool MapBuilder::shouldSkipTile(uint32 mapID, uint32 tileX, uint32 tileY)
    {
        char fileName[255];
        sprintf(fileName, "%s/mmaps/%03u_%02i_%02i.mmtile", m_workdir, mapID, tileY, tileX);
        FILE* file = fopen(fileName, "rb");
        if (!file)
            return false;

        MmapTileHeader header;
        int count = fread(&header, sizeof(MmapTileHeader), 1, file);
        fclose(file);
        if (count != 1)
            return false;

        if (header.mmapMagic != MMAP_MAGIC || header.dtVersion != uint32(DT_NAVMESH_VERSION))
            return false;

        if (header.mmapVersion != MMAP_VERSION)
            return false;

        return true;
    }

    json MapBuilder::getDefaultConfig()
    {
        return {
            {"borderSize", 5},
            {"detailSampleDist", BASE_UNIT_DIM * 16.0f},
            {"detailSampleMaxError", BASE_UNIT_DIM},
            {"maxEdgeLen", VERTEX_PER_TILE + 1},
            {"maxSimplificationError", 1.8f},
            {"mergeRegionArea", 50},
            {"minRegionArea", 60},
            {"walkableClimb", 4},
            {"walkableHeight", 6},
            {"walkableRadius", 2},
            {"walkableSlopeAngle", 60.0f},
        };
    }

    json MapBuilder::getMapIdConfig(uint32 mapId)
    {
        std::string key = std::to_string(mapId);

        json config = getDefaultConfig();
        if (m_config.find(key) != m_config.end())
            config.merge_patch(m_config.at(key));

        return config;
    }

    json MapBuilder::getTileConfig(uint32 mapId, uint32 tileX, uint32 tileY)
    {
        std::string key = std::to_string(tileX) + std::to_string(tileY);

        json config = getMapIdConfig(mapId);
        if (config.find(key) != config.end())
            config.merge_patch(config.at(key));

        for (json::iterator it = config.begin(); it != config.end();) {
            if ((*it).is_object())
                it = config.erase(it);
            else
                ++it;
        }

        return config;
    }
}
