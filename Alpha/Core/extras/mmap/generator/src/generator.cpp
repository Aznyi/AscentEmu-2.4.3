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

#include <sstream>

#include "MMapCommon.h"
#include "MapBuilder.h"
#include "../runtime/MoveMapSharedDefines.h"

#include "DetourNavMesh.h"

#include <string>

using namespace MMAP;

bool checkDirectories(bool debugOutput, const char* workdir)
{
    vector<string> dirFiles;
    char maps_dir[1024];
    char vmaps_dir[1024];
    char mmaps_dir[1024];
    char meshes_dir[1024];

    sprintf(maps_dir, "%s/%s", workdir, "maps");
    const bool hasMaps = (getDirContents(dirFiles, maps_dir) != LISTFILE_DIRECTORY_NOT_FOUND && !dirFiles.empty());
    if (!hasMaps)
        printf("'%s' directory is empty or does not exist; terrain/liquid input will be skipped where vmaps alone are sufficient\n", maps_dir);

    dirFiles.clear();
    sprintf(vmaps_dir, "%s/%s", workdir, "vmaps");
    if (getDirContents(dirFiles, vmaps_dir, "*.vmdir") == LISTFILE_DIRECTORY_NOT_FOUND || !dirFiles.size())
    {
        printf("'%s' directory is empty or does not exist\n", vmaps_dir);
        return false;
    }

    dirFiles.clear();
    sprintf(mmaps_dir, "%s/%s", workdir, "mmaps");
    if (getDirContents(dirFiles, mmaps_dir) == LISTFILE_DIRECTORY_NOT_FOUND)
    {
        printf("'%s' directory does not exist\n", mmaps_dir);
        return false;
    }

    dirFiles.clear();
    if (debugOutput)
    {
        sprintf(meshes_dir, "%s/%s", workdir, "meshes");
        if (getDirContents(dirFiles, meshes_dir) == LISTFILE_DIRECTORY_NOT_FOUND)
        {
            printf("'%s' directory does not exist (no place to put debugOutput files)\n", meshes_dir);
            return false;
        }
    }

    return true;
}

void printUsage()
{
    printf("Generator command line args\n\n");
    printf("-? or /? or -h : Show this help\n");
    printf("\"[#]\" : Build maps using specified map IDs.\n");
    printf("--tile [#,#] : Build the specified tile\n");
    printf("--skipLiquid : liquid data for maps\n");
    printf("--skipContinents : skip continents\n");
    printf("--skipJunkMaps : junk maps include some unused\n");
    printf("--skipBattlegrounds : does not include PVP arenas\n");
    printf("--debug : create debugging files for use with RecastDemo\n");
    printf("--silent : Make script friendly. No wait for user input, error, completion.\n");
    printf("--offMeshInput [file.*] : Path to file containing off mesh connections data.\n\n");
    printf("--configInputPath [file.*] : Path to json configuration file.\n\n");
    printf("--buildGameObjects : builds only gameobject models for transports\n\n");
    printf("--threads [#]: specifies number of threads to use for maps processing\n\n");
    printf("--workdir [directory] : Path to basedir of maps/vmaps.\n\n");
    printf("--inspectPoint [x,y,z] : Inspect whether built navmesh covers a world-space point.\n\n");
    printf("--inspectTileHeader [file] : Print baked .mmtile header contents.\n\n");
    printf("Example:\nmovemapgen (generate all mmap with default arg\n"
           "movemapgen \"1 0 169\" (generate maps 1, 0 and 169)\n"
           "movemapgen 0 --tile 34,46 (builds only tile 34,46 of map 0)\n"
           "movemapgen 36 --tile 32,32 --inspectPoint -40.120,-370.386,56.504\n\n");
    printf("Please read readme file for more information and examples.\n");
}

static void printTileHeader(const char* filePath)
{
    FILE* file = fopen(filePath, "rb");
    if (!file)
    {
        printf("Failed to open %s\n", filePath);
        return;
    }

    MmapTileHeader header;
    memset(&header, 0, sizeof(header));
    if (fread(&header, sizeof(header), 1, file) != 1)
    {
        printf("Failed reading mmap tile header from %s\n", filePath);
        fclose(file);
        return;
    }

    printf("MMap tile file: %s\n", filePath);
    printf("  mmapMagic=0x%08X mmapVersion=%u dtVersion=%u size=%u usesLiquids=%u\n",
        header.mmapMagic, header.mmapVersion, header.dtVersion, header.size, header.usesLiquids);

    if (header.size < sizeof(dtMeshHeader))
    {
        printf("  Tile payload too small to contain dtMeshHeader\n");
        fclose(file);
        return;
    }

    dtMeshHeader meshHeader;
    memset(&meshHeader, 0, sizeof(meshHeader));
    if (fread(&meshHeader, sizeof(meshHeader), 1, file) != 1)
    {
        printf("  Failed reading dtMeshHeader from %s\n", filePath);
        fclose(file);
        return;
    }

    printf("  dtMeshHeader.magic=0x%08X version=%d\n", meshHeader.magic, meshHeader.version);
    printf("  dtMeshHeader.x=%d y=%d layer=%d userId=%u\n", meshHeader.x, meshHeader.y, meshHeader.layer, meshHeader.userId);
    printf("  polys=%d verts=%d detailMeshes=%d detailVerts=%d detailTris=%d offMesh=%d\n",
        meshHeader.polyCount, meshHeader.vertCount, meshHeader.detailMeshCount, meshHeader.detailVertCount, meshHeader.detailTriCount, meshHeader.offMeshConCount);
    printf("  bmin=%0.3f,%0.3f,%0.3f bmax=%0.3f,%0.3f,%0.3f\n",
        meshHeader.bmin[0], meshHeader.bmin[1], meshHeader.bmin[2], meshHeader.bmax[0], meshHeader.bmax[1], meshHeader.bmax[2]);

    const std::string path(filePath);
    size_t slash = path.find_last_of("/\\");
    const std::string fileName = (slash == std::string::npos) ? path : path.substr(slash + 1);
    unsigned int mapId = 0, fileTileY = 0, fileTileX = 0;
    if (sscanf(fileName.c_str(), "%3u_%2u_%2u.mmtile", &mapId, &fileTileY, &fileTileX) == 3)
        printf("  filename implies map=%u tile=%u,%u\n", mapId, fileTileX, fileTileY);

    fclose(file);
}

bool handleArgs(int argc, char** argv,
                std::vector<uint32>& mapIds,
                int& tileX,
                int& tileY,
                bool& skipLiquid,
                bool& skipContinents,
                bool& skipJunkMaps,
                bool& skipBattlegrounds,
                bool& debugOutput,
                bool& silent,
                bool& buildGameObjects,
                const char*& offMeshInputPath,
                const char*& configInputPath,
                int& threads,
                const char*& workdir,
                MMAP::MapBuilder::InspectPoint& inspectPoint,
                const char*& inspectTileHeaderPath)
{
    char* param = NULL;
    workdir = "./";

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--tile") == 0 && i + 1 < argc)
        {
            param = argv[++i];
            if (!param)
                return false;

            char* stileX = strtok(param, ",");
            char* stileY = strtok(NULL, ",");

            int tilex = atoi(stileX);
            int tiley = atoi(stileY);

            if ((tilex > 0 && tilex < 64) || (tilex == 0 && strcmp(stileX, "0") == 0))
                tileX = tilex;
            if ((tiley > 0 && tiley < 64) || (tiley == 0 && strcmp(stileY, "0") == 0))
                tileY = tiley;

            if (tileX < 0 || tileY < 0)
            {
                printf("invalid tile coords.\n");
                return false;
            }
        }
        else if (strcmp(argv[i], "--skipLiquid") == 0)
        {
            skipLiquid = true;
        }
        else if (strcmp(argv[i], "--skipContinents") == 0)
        {
            skipContinents = true;
        }
        else if (strcmp(argv[i], "--skipJunkMaps") == 0)
        {
            skipJunkMaps = true;
        }
        else if (strcmp(argv[i], "--skipBattlegrounds") == 0)
        {
            skipBattlegrounds = true;
        }
        else if (strcmp(argv[i], "--debug") == 0)
        {
            debugOutput = true;
        }
        else if (strcmp(argv[i], "--silent") == 0)
        {
            silent = true;
        }
        else if (strcmp(argv[i], "--buildGameObjects") == 0)
        {
            buildGameObjects = true;
        }
        else if (strcmp(argv[i], "--offMeshInput") == 0 && i + 1 < argc)
        {
            param = argv[++i];
            if (!param)
                return false;

            offMeshInputPath = param;
        }
        else if (strcmp(argv[i], "--configInputPath") == 0 && i + 1 < argc)
        {
            param = argv[++i];
            if (!param)
                return false;

            configInputPath = param;
        }
        else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc)
        {
            param = argv[++i];
            if (!param)
                return false;

            threads = 0;
            try
            {
                threads = std::stoi(param);
            }
            catch (std::invalid_argument&) {}
            catch (std::out_of_range&) {}
            if (threads <= 0)
            {
                printf("Invalid number of threads.\n");
                return false;
            }
        }
        else if (strcmp(argv[i], "--workdir") == 0 && i + 1 < argc)
        {
            param = argv[++i];
            if (!param)
                return false;

            workdir = param;
        }
        else if (strcmp(argv[i], "--inspectPoint") == 0 && i + 1 < argc)
        {
            param = argv[++i];
            if (!param)
                return false;

            char* sx = strtok(param, ",");
            char* sy = strtok(NULL, ",");
            char* sz = strtok(NULL, ",");
            if (sx == NULL || sy == NULL || sz == NULL)
            {
                printf("invalid inspect point.\n");
                return false;
            }

            inspectPoint.enabled = true;
            inspectPoint.worldX = (float)atof(sx);
            inspectPoint.worldY = (float)atof(sy);
            inspectPoint.worldZ = (float)atof(sz);
        }
        else if (strcmp(argv[i], "--inspectTileHeader") == 0 && i + 1 < argc)
        {
            param = argv[++i];
            if (!param)
                return false;

            inspectTileHeaderPath = param;
        }
        else if ((strcmp(argv[i], "-?") == 0) || (strcmp(argv[i], "/?") == 0) || (strcmp(argv[i], "-h") == 0))
        {
            printUsage();
            exit(1);
        }
        else
        {
            std::istringstream iss(argv[i]);
            std::string token;
            while (std::getline(iss, token, ' ')) {
                try
                {
                    mapIds.push_back(std::stoi(token));
                }
                catch (std::invalid_argument&) {}
                catch (std::out_of_range&) {}
            }
            if (!mapIds.size()) {
                printf("Invalid map IDs provided.\n");
                return false;
            }
        }
    }

    return true;
}

int main(int argc, char** argv)
{
    std::vector<uint32> mapIds;
    int threads = -1;
    int tileX = -1, tileY = -1;

    bool skipLiquid = false;
    bool skipContinents = false;
    bool skipJunkMaps = false;
    bool skipBattlegrounds = false;
    bool debug = false;
    bool silent = false;
    bool buildGameObjects = false;

    const char* offMeshInputPath = "offmesh.txt";
    const char* configInputPath = "config.json";
    const char* workdir = NULL;
    const char* inspectTileHeaderPath = NULL;
    MMAP::MapBuilder::InspectPoint inspectPoint;

    bool validParam = handleArgs(argc, argv, mapIds, tileX, tileY, skipLiquid,
                                 skipContinents, skipJunkMaps, skipBattlegrounds,
                                 debug, silent, buildGameObjects, offMeshInputPath, configInputPath, threads, workdir, inspectPoint, inspectTileHeaderPath);

    if (!validParam)
    {
        if (!silent)
        {
            printf("You have specified invalid parameters (use -? for more help)");
            printUsage();
        }
        return -1;
    }

    if (threads == -1) {
        threads = std::thread::hardware_concurrency();
    }

    if ((mapIds.size() == 0) && debug)
    {
        if (silent)
            return -2;

        printf("You have specified debug output, but didn't specify maps to generate.\n");
        printf("This will generate debug output for ALL maps.\n");
        printf("Are you sure you want to continue? (y/n) ");
        if (getchar() != 'y')
            return 0;
    }

    if (inspectTileHeaderPath != NULL)
    {
        printTileHeader(inspectTileHeaderPath);
        return 0;
    }

    if (!checkDirectories(debug, workdir))
        return -3;

    MapBuilder builder(configInputPath, threads, skipLiquid, skipContinents, skipJunkMaps, skipBattlegrounds, debug, offMeshInputPath, workdir, &inspectPoint);

    if (mapIds.size() == 1 && tileX > -1 && tileY > -1)
        builder.buildSingleTile(mapIds.front(), tileX, tileY);
    else
        builder.BuildMaps(mapIds);

    if (buildGameObjects)
    {
        builder.buildTransports();
    }

    if (!silent)
        printf("Movemap build is complete!\n");

    return 0;
}
