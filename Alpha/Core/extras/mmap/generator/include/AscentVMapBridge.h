#ifndef ASCENT_MMAP_VMAP_BRIDGE_H
#define ASCENT_MMAP_VMAP_BRIDGE_H

#include "TerrainBuilder.h"

namespace MMAP
{
        bool LoadAscentVMapTile(uint32 mapID, uint32 tileX, uint32 tileY, MeshData& meshData, const char* workdir);
        bool LoadAscentVMapModel(const char* workdir, const char* relativeModelPath, MeshData& meshData);
        bool ParseAscentVMapManifestName(const std::string& filename, uint32& mapID, uint32& tileX, uint32& tileY, bool& tiledManifest);
        uint32 PackAscentTileID(uint32 tileX, uint32 tileY);
	void UnpackAscentTileID(uint32 packedTileID, uint32& tileX, uint32& tileY);
}

#endif
