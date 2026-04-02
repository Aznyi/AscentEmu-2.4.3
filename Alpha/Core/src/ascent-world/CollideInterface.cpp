/*
 * OpenAscent MMORPG Server
 * Copyright (C) 2008 <http://www.openascent.com/>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "StdAfx.h"
#include <sys/stat.h>
#include <stdio.h>

#ifdef WIN32
#include <io.h>
#endif

#ifdef COLLISION

#define MAX_MAP 600

CCollideInterface CollideInterface;
IVMapManager * CollisionMgr;
Mutex m_loadLock;
uint32 m_tilesLoaded[MAX_MAP][64][64];

#ifdef WIN32
#ifdef COLLISION_DEBUG

uint64 c_GetTimerValue()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter( &li );
	return li.QuadPart;
}

uint32 c_GetNanoSeconds(uint64 t1, uint64 t2)
{
	LARGE_INTEGER li;
	double val;
	QueryPerformanceFrequency( &li );
	val = double( t1 - t2 ) * 1000000;
	val /= li.QuadPart;
    return long2int32( val );	
}

#define COLLISION_BEGINTIMER uint64 v1 = c_GetTimerValue();

#endif	// COLLISION_DEBUG
#endif	// WIN32

#ifdef WIN32
#pragma comment(lib, "collision.lib")
#endif

namespace
{
	struct VMapProbeTile
	{
		VMapProbeTile() : found(false), mapId(0), tileX(0), tileY(0) {}

		bool found;
		uint32 mapId;
		uint32 tileX;
		uint32 tileY;
		std::string manifestFile;
	};

	bool CollisionFileExists(const std::string& path)
	{
		struct _stat fileInfo;
		return (_stat(path.c_str(), &fileInfo) == 0);
	}

	std::string BuildVMapPath(const std::string& root, const std::string& relativeName)
	{
		std::string fullPath(root);
		if(!fullPath.empty() && fullPath[fullPath.length() - 1] != '/' && fullPath[fullPath.length() - 1] != '\\')
			fullPath += "/";

		fullPath += relativeName;
		return fullPath;
	}

	bool FindStartupProbeTile(const std::string& root, VMapProbeTile& probe)
	{
#ifdef WIN32
		const std::string searchPattern = BuildVMapPath(root, "*.vmdir");
		struct _finddata_t fileInfo;
		intptr_t handle = _findfirst(searchPattern.c_str(), &fileInfo);
		if(handle == -1)
			return false;

		do
		{
			if((fileInfo.attrib & _A_SUBDIR) != 0)
				continue;

			unsigned int mapId = 0;
			unsigned int tileY = 0;
			unsigned int tileX = 0;
			if(sscanf(fileInfo.name, "%u_%u_%u.vmdir", &mapId, &tileY, &tileX) == 3)
			{
				probe.found = true;
				probe.mapId = mapId;
				probe.tileX = tileX;
				probe.tileY = tileY;
				probe.manifestFile = fileInfo.name;
				_findclose(handle);
				return true;
			}
		}
		while(_findnext(handle, &fileInfo) == 0);

		_findclose(handle);
#else
		(void)root;
		(void)probe;
#endif
		return false;
	}
}

// Debug functions
#ifdef COLLISION_DEBUG

void CCollideInterface::Init()
{
	Log.Notice("CollideInterface", "Init");
	COLLISION_BEGINTIMER;
	CollisionMgr = ((IVMapManager*)collision_init());
	Log.Notice("CollideInterface", "Using vmap path: %s", sWorld.vMapPath.c_str());
	if(!HasVMapDirectory())
		Log.Error("CollideInterface", "vmap root directory does not exist or is not accessible: %s", sWorld.vMapPath.c_str());
	else if(sWorld.CollisionStartupProbe)
	{
		VMapProbeTile probe;
		if(!FindStartupProbeTile(sWorld.vMapPath, probe))
		{
			Log.Error("CollideInterface", "CollisionStartupProbe could not find a tiled .vmdir manifest under %s", sWorld.vMapPath.c_str());
		}
		else
		{
			const int result = CollisionMgr->loadMap(sWorld.vMapPath.c_str(), probe.mapId, probe.tileY, probe.tileX);
			if(result)
			{
				Log.Notice("CollideInterface", "CollisionStartupProbe loaded map=%u tile=%u,%u result=%d file=%s",
					probe.mapId, probe.tileX, probe.tileY, result, probe.manifestFile.c_str());
				CollisionMgr->unloadMap(probe.mapId, probe.tileY, probe.tileX);
				Log.Notice("CollideInterface", "CollisionStartupProbe unloaded map=%u tile=%u,%u file=%s",
					probe.mapId, probe.tileX, probe.tileY, probe.manifestFile.c_str());
			}
			else
			{
				Log.Error("CollideInterface", "CollisionStartupProbe failed for map=%u tile=%u,%u file=%s root=%s",
					probe.mapId, probe.tileX, probe.tileY, probe.manifestFile.c_str(), sWorld.vMapPath.c_str());
			}
		}
	}
	printf("[%u ns] collision_init\n", c_GetNanoSeconds(c_GetTimerValue(), v1));
}

void CCollideInterface::ActivateTile(uint32 mapId, uint32 tileX, uint32 tileY)
{
	m_loadLock.Acquire();
	if(m_tilesLoaded[mapId][tileX][tileY] == 0)
	{
		COLLISION_BEGINTIMER;
		int result = CollisionMgr->loadMap(sWorld.vMapPath.c_str(), mapId, tileY, tileX);
		if(sWorld.CollisionLogTileLoads)
		{
			if(result)
				Log.Notice("CollideInterface", "loadMap map=%u tile=%u,%u result=%d file=%s", mapId, tileX, tileY, result, CollisionMgr->getDirFileName(mapId, tileY, tileX).c_str());
			else
			{
				const std::string tileFile = CollisionMgr->getDirFileName(mapId, tileY, tileX);
				if(!HasVMapDirectory())
					Log.Error("CollideInterface", "loadMap failed for map=%u tile=%u,%u because vmap root is missing: %s", mapId, tileX, tileY, sWorld.vMapPath.c_str());
				else if(!HasVMapTile(mapId, tileX, tileY))
					Log.Error("CollideInterface", "loadMap missing manifest for map=%u tile=%u,%u file=%s root=%s", mapId, tileX, tileY, tileFile.c_str(), sWorld.vMapPath.c_str());
				else
					Log.Error("CollideInterface", "loadMap failed for map=%u tile=%u,%u file=%s root=%s (manifest exists, data may be malformed or incomplete)", mapId, tileX, tileY, tileFile.c_str(), sWorld.vMapPath.c_str());
			}
		}
		printf("[%u ns] collision_activate_cell %u %u %u\n", c_GetNanoSeconds(c_GetTimerValue(), v1), mapId, tileX, tileY);
	}

	++m_tilesLoaded[mapId][tileX][tileY];
	m_loadLock.Release();
}

void CCollideInterface::DeactivateTile(uint32 mapId, uint32 tileX, uint32 tileY)
{
	m_loadLock.Acquire();
	if(m_tilesLoaded[mapId][tileX][tileY] == 0)
	{
		Log.Error("CollideInterface", "DeactivateTile underflow for map %u tile %u,%u", mapId, tileX, tileY);
		m_loadLock.Release();
		return;
	}

	if(!(--m_tilesLoaded[mapId][tileX][tileY]))
	{
		COLLISION_BEGINTIMER;
		CollisionMgr->unloadMap(mapId, tileY, tileX);
		printf("[%u ns] collision_deactivate_cell %u %u %u\n", c_GetNanoSeconds(c_GetTimerValue(), v1), mapId, tileX, tileY);
	}

	m_loadLock.Release();
}

uint32 CCollideInterface::GetTileLoadRefCount(uint32 mapId, uint32 tileX, uint32 tileY) const
{
	if(mapId >= MAX_MAP || tileX >= 64 || tileY >= 64)
		return 0;
	return m_tilesLoaded[mapId][tileX][tileY];
}

void CCollideInterface::DeInit()
{
	Log.Notice("CollideInterface", "DeInit");
	COLLISION_BEGINTIMER;
	collision_shutdown();
	printf("[%u ns] collision_shutdown\n", c_GetNanoSeconds(c_GetTimerValue(), v1));
}

float CCollideInterface::GetHeight(uint32 mapId, float x, float y, float z)
{
	COLLISION_BEGINTIMER;
	float v = CollisionMgr->getHeight(mapId, x, y, z);
	printf("[%u ns] GetHeight Map:%u %f %f %f\n", c_GetNanoSeconds(c_GetTimerValue(), v1), mapId, x, y, z);
	return v;
}

float CCollideInterface::GetHeight(uint32 mapId, LocationVector & pos)
{
	COLLISION_BEGINTIMER;
	float v = CollisionMgr->getHeight(mapId, pos);
	printf("[%u ns] GetHeight Map:%u %f %f %f\n", c_GetNanoSeconds(c_GetTimerValue(), v1), mapId, pos.x, pos.y, pos.z);
	return v;
}

bool CCollideInterface::IsIndoor(uint32 mapId, LocationVector & pos)
{
	bool r;
	COLLISION_BEGINTIMER;
	r = CollisionMgr->isInDoors(mapId, pos);
	printf("[%u ns] IsIndoor Map:%u %f %f %f\n", c_GetNanoSeconds(c_GetTimerValue(), v1), mapId, pos.x, pos.y, pos.z);
	return r;
}

bool CCollideInterface::IsOutdoor(uint32 mapId, LocationVector & pos)
{
	bool r;
	COLLISION_BEGINTIMER;
	r = CollisionMgr->isOutDoors(mapId, pos);
	printf("[%u ns] IsOutdoor Map:%u %f %f %f\n", c_GetNanoSeconds(c_GetTimerValue(), v1), mapId, pos.x, pos.y, pos.z);
	return r;
}

bool CCollideInterface::IsIndoor(uint32 mapId, float x, float y, float z)
{
	bool r;
	COLLISION_BEGINTIMER;
	r = CollisionMgr->isInDoors(mapId, x, y, z);
	printf("[%u ns] IsIndoor Map:%u %f %f %f\n", c_GetNanoSeconds(c_GetTimerValue(), v1), mapId, x, y, z);
	return r;
}

bool CCollideInterface::IsOutdoor(uint32 mapId, float x, float y, float z)
{
	bool r;
	COLLISION_BEGINTIMER;
	r = CollisionMgr->isOutDoors(mapId, x, y, z);
	printf("[%u ns] IsOutdoor Map:%u %f %f %f\n", c_GetNanoSeconds(c_GetTimerValue(), v1), mapId, x, y, z);
	return r;
}

bool CCollideInterface::CheckLOS(uint32 mapId, LocationVector & pos1, LocationVector & pos2)
{
	bool r;
	COLLISION_BEGINTIMER;
	r = CollisionMgr->isInLineOfSight(mapId, pos1, pos2);
	printf("[%u ns] CheckLOS Map:%u %f %f %f -> %f %f %f\n", c_GetNanoSeconds(c_GetTimerValue(), v1), mapId, pos1.x, pos1.y, pos1.z, pos2.x, pos2.y, pos2.z);
	return r;
}

bool CCollideInterface::CheckLOS(uint32 mapId, float x1, float y1, float z1, float x2, float y2, float z2)
{
	bool r;
	COLLISION_BEGINTIMER;
	r = CollisionMgr->isInLineOfSight(mapId, x1, y1, z1, x2, y2, z2);
	printf("[%u ns] CheckLOS Map:%u %f %f %f -> %f %f %f\n", c_GetNanoSeconds(c_GetTimerValue(), v1), mapId, x1, y1, z1, x2, y2, z2);
	return r;
}

bool CCollideInterface::GetFirstPoint(uint32 mapId, LocationVector & pos1, LocationVector & pos2, LocationVector & outvec, float distmod)
{
	bool r;
	COLLISION_BEGINTIMER;
	r = CollisionMgr->getObjectHitPos(mapId, pos1, pos2, outvec, distmod);
	printf("[%u ns] GetFirstPoint Map:%u %f %f %f -> %f %f %f\n", c_GetNanoSeconds(c_GetTimerValue(), v1), mapId, pos1.x, pos1.y, pos1.z, pos2.x, pos2.y, pos2.z);
	return r;
}

bool CCollideInterface::GetFirstPoint(uint32 mapId, float x1, float y1, float z1, float x2, float y2, float z2, float & outx, float & outy, float & outz, float distmod)
{
	bool r;
	COLLISION_BEGINTIMER;
	r = CollisionMgr->getObjectHitPos(mapId, x1, y1, z1, x2, y2, z2, outx, outy, outz, distmod);
	printf("[%u ns] GetFirstPoint Map:%u %f %f %f -> %f %f %f\n", c_GetNanoSeconds(c_GetTimerValue(), v1), mapId, x1, y1, z1, x2, y2, z2);
	return r;
}

#else

void CCollideInterface::Init()
{
	Log.Notice("CollideInterface", "Init");
	CollisionMgr = ((IVMapManager*)collision_init());
	Log.Notice("CollideInterface", "Using vmap path: %s", sWorld.vMapPath.c_str());
	if(!HasVMapDirectory())
		Log.Error("CollideInterface", "vmap root directory does not exist or is not accessible: %s", sWorld.vMapPath.c_str());
	else if(sWorld.CollisionStartupProbe)
	{
		VMapProbeTile probe;
		if(!FindStartupProbeTile(sWorld.vMapPath, probe))
		{
			Log.Error("CollideInterface", "CollisionStartupProbe could not find a tiled .vmdir manifest under %s", sWorld.vMapPath.c_str());
		}
		else
		{
			const int result = CollisionMgr->loadMap(sWorld.vMapPath.c_str(), probe.mapId, probe.tileY, probe.tileX);
			if(result)
			{
				Log.Notice("CollideInterface", "CollisionStartupProbe loaded map=%u tile=%u,%u result=%d file=%s",
					probe.mapId, probe.tileX, probe.tileY, result, probe.manifestFile.c_str());
				CollisionMgr->unloadMap(probe.mapId, probe.tileY, probe.tileX);
				Log.Notice("CollideInterface", "CollisionStartupProbe unloaded map=%u tile=%u,%u file=%s",
					probe.mapId, probe.tileX, probe.tileY, probe.manifestFile.c_str());
			}
			else
			{
				Log.Error("CollideInterface", "CollisionStartupProbe failed for map=%u tile=%u,%u file=%s root=%s",
					probe.mapId, probe.tileX, probe.tileY, probe.manifestFile.c_str(), sWorld.vMapPath.c_str());
			}
		}
	}
}

void CCollideInterface::ActivateTile(uint32 mapId, uint32 tileX, uint32 tileY)
{
	m_loadLock.Acquire();
	if(m_tilesLoaded[mapId][tileX][tileY] == 0)
	{
		int result = CollisionMgr->loadMap(sWorld.vMapPath.c_str(), mapId, tileY, tileX);
		if(sWorld.CollisionLogTileLoads)
		{
			if(result)
				Log.Notice("CollideInterface", "loadMap map=%u tile=%u,%u result=%d file=%s", mapId, tileX, tileY, result, CollisionMgr->getDirFileName(mapId, tileY, tileX).c_str());
			else
			{
				const std::string tileFile = CollisionMgr->getDirFileName(mapId, tileY, tileX);
				if(!HasVMapDirectory())
					Log.Error("CollideInterface", "loadMap failed for map=%u tile=%u,%u because vmap root is missing: %s", mapId, tileX, tileY, sWorld.vMapPath.c_str());
				else if(!HasVMapTile(mapId, tileX, tileY))
					Log.Error("CollideInterface", "loadMap missing manifest for map=%u tile=%u,%u file=%s root=%s", mapId, tileX, tileY, tileFile.c_str(), sWorld.vMapPath.c_str());
				else
					Log.Error("CollideInterface", "loadMap failed for map=%u tile=%u,%u file=%s root=%s (manifest exists, data may be malformed or incomplete)", mapId, tileX, tileY, tileFile.c_str(), sWorld.vMapPath.c_str());
			}
		}
	}

	++m_tilesLoaded[mapId][tileX][tileY];
	m_loadLock.Release();
}

void CCollideInterface::DeactivateTile(uint32 mapId, uint32 tileX, uint32 tileY)
{
	m_loadLock.Acquire();
	if(m_tilesLoaded[mapId][tileX][tileY] == 0)
	{
		Log.Error("CollideInterface", "DeactivateTile underflow for map %u tile %u,%u", mapId, tileX, tileY);
		m_loadLock.Release();
		return;
	}

	if(!(--m_tilesLoaded[mapId][tileX][tileY]))
		CollisionMgr->unloadMap(mapId, tileY, tileX);

	m_loadLock.Release();
}

uint32 CCollideInterface::GetTileLoadRefCount(uint32 mapId, uint32 tileX, uint32 tileY) const
{
	if(mapId >= MAX_MAP || tileX >= 64 || tileY >= 64)
		return 0;
	return m_tilesLoaded[mapId][tileX][tileY];
}

bool CCollideInterface::HasVMapDirectory() const
{
	return CollisionFileExists(sWorld.vMapPath);
}

bool CCollideInterface::HasVMapTile(uint32 mapId, uint32 tileX, uint32 tileY) const
{
	if(CollisionMgr == NULL)
		return false;

	return CollisionFileExists(BuildVMapPath(sWorld.vMapPath, CollisionMgr->getDirFileName(mapId, tileY, tileX)));
}

void CCollideInterface::DeInit()
{
	Log.Notice("CollideInterface", "DeInit");
	collision_shutdown();
}

#endif		// COLLISION_DEBUG
#endif		// COLLISION
