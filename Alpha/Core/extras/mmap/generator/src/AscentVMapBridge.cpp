#include "AscentVMapBridge.h"

#include "../../../collision/collision_dll/vmap/VMapManager.h"
#include "../../../collision/collision_dll/vmap/ManagedModelContainer.h"

#include <cfloat>
#include <cmath>

namespace MMAP
{
        namespace
        {
                bool IsAscentTileMap(uint32 mapID)
                {
			switch(mapID)
			{
			case 509:
			case 469:
			case 189:
			case 30:
			case 37:
			case 33:
			case 533:
			case 209:
			case 309:
			case 560:
			case 534:
			case 532:
			case 543:
			case 568:
			case 564:
			case 0:
			case 1:
			case 530:
				return true;
			default:
				return false;
			}
		}

		string BuildVMapBasePath(const char* workdir)
		{
			string path(workdir ? workdir : "./");
			if(!path.empty() && path[path.length() - 1] != '/' && path[path.length() - 1] != '\\')
				path += "/";
			path += "vmaps/";
			return path;
		}

                G3D::Vector3 ConvertInternalToNavMeshCoords(const G3D::Vector3& internalPosition)
                {
                        const float full = 64.0f * 533.33333333f;
                        const float mid = full * 0.5f;
                        return G3D::Vector3(mid - internalPosition.z, internalPosition.y, mid - internalPosition.x);
                }

                bool IsUpwardFacingInNavCoords(const G3D::Vector3& a, const G3D::Vector3& b, const G3D::Vector3& c)
                {
			const G3D::Vector3 edge0 = b - a;
			const G3D::Vector3 edge1 = c - a;
			const G3D::Vector3 normal = edge0.cross(edge1);
			return normal.y >= 0.0f;
		}

		void AppendTriangle(MeshData& meshData, const G3D::Vector3& a, const G3D::Vector3& b, const G3D::Vector3& c)
		{
			const int baseIndex = meshData.solidVerts.size() / 3;
			meshData.solidVerts.append(a.x, a.y, a.z);
			meshData.solidVerts.append(b.x, b.y, b.z);
                        meshData.solidVerts.append(c.x, c.y, c.z);
                        meshData.solidTris.append(baseIndex + 0, baseIndex + 1, baseIndex + 2);
                }

                void AppendContainerGeometry(VMAP::ModelContainer* container, MeshData& meshData)
                {
                        if(container == NULL)
                                return;

                        for(unsigned int subModelIndex = 0; subModelIndex < container->getNSubModel(); ++subModelIndex)
                        {
                                const VMAP::SubModel& subModel = container->getSubModel(subModelIndex);
                                const G3D::Vector3 basePosition = subModel.getBasePosition();
                                for(unsigned int triangleIndex = 0; triangleIndex < subModel.getNTriangles(); ++triangleIndex)
                                {
                                        const VMAP::TriangleBox& triangle = subModel.getTriangle(triangleIndex);
                                        const G3D::Vector3 a = ConvertInternalToNavMeshCoords(triangle.vertex(0).getVector3() + basePosition);
                                        const G3D::Vector3 b = ConvertInternalToNavMeshCoords(triangle.vertex(1).getVector3() + basePosition);
                                        const G3D::Vector3 c = ConvertInternalToNavMeshCoords(triangle.vertex(2).getVector3() + basePosition);

                                        // Recast's walkability test depends on upward-facing triangle winding in
                                        // navmesh coordinates. Normalize imported vmap triangles here so dungeon
                                        // floors do not get rasterized as steep or inverted geometry.
                                        if(IsUpwardFacingInNavCoords(a, b, c))
                                                AppendTriangle(meshData, a, b, c);
                                        else
                                                AppendTriangle(meshData, a, c, b);
                                }
                        }
                }
        }

        uint32 PackAscentTileID(uint32 tileX, uint32 tileY)
	{
		return (tileX << 16) | tileY;
	}

	void UnpackAscentTileID(uint32 packedTileID, uint32& tileX, uint32& tileY)
	{
		tileX = (packedTileID >> 16) & 0xFFFF;
		tileY = packedTileID & 0xFFFF;
	}

	bool ParseAscentVMapManifestName(const std::string& filename, uint32& mapID, uint32& tileX, uint32& tileY, bool& tiledManifest)
	{
		mapID = 0;
		tileX = 0;
		tileY = 0;
		tiledManifest = false;

		int parsedMapId = 0;
		int parsedTileX = 0;
		int parsedTileY = 0;
		if(sscanf(filename.c_str(), "%3d_%d_%d.vmdir", &parsedMapId, &parsedTileX, &parsedTileY) == 3)
		{
			mapID = uint32(parsedMapId);
			tileX = uint32(parsedTileX);
			tileY = uint32(parsedTileY);
			tiledManifest = true;
			return true;
		}

		if(sscanf(filename.c_str(), "%3d.vmdir", &parsedMapId) == 1)
		{
			mapID = uint32(parsedMapId);
			return true;
		}

		return false;
	}

        bool LoadAscentVMapTile(uint32 mapID, uint32 tileX, uint32 tileY, MeshData& meshData, const char* workdir)
        {
                const string basePath = BuildVMapBasePath(workdir);
                VMAP::MapTree mapTree(basePath.c_str());

                const uint32 packedTileID = PackAscentTileID(tileX, tileY);
                std::string loadedManifest;

		char manifestName[64];
		if(IsAscentTileMap(mapID))
                {
			snprintf(manifestName, sizeof(manifestName), "%03u_%u_%u.vmdir", mapID, tileX, tileY);
                        loadedManifest = manifestName;
                        if(!mapTree.loadMap(loadedManifest, packedTileID))
                        {
                                snprintf(manifestName, sizeof(manifestName), "%03u.vmdir", mapID);
                                loadedManifest = manifestName;
                                if(!mapTree.loadMap(loadedManifest, packedTileID))
                                        return false;
                        }
                }
		else
                {
			snprintf(manifestName, sizeof(manifestName), "%03u.vmdir", mapID);
                        loadedManifest = manifestName;
                        if(!mapTree.loadMap(loadedManifest, packedTileID))
                                return false;
                }

                G3D::Array<VMAP::ModelContainer*> containers;
                mapTree.getModelContainer(containers);
                for(int i = 0; i < containers.size(); ++i)
                        AppendContainerGeometry(containers[i], meshData);

                mapTree.unloadMap(loadedManifest, packedTileID);
                return (meshData.solidVerts.size() > 0);
	}

        bool LoadAscentVMapModel(const char* workdir, const char* relativeModelPath, MeshData& meshData)
        {
                string fullPath = BuildVMapBasePath(workdir);
                fullPath += relativeModelPath;

		VMAP::ManagedModelContainer container;
                if(!container.readFile(fullPath.c_str()))
                        return false;

                AppendContainerGeometry(&container, meshData);
                return (meshData.solidVerts.size() > 0);
        }
}
