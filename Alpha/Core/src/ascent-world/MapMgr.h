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

//
// MapMgr.h
//

#ifndef __MAPMGR_H
#define __MAPMGR_H

#include "MMapInterface.h"

class MapCell;
class Map;
class Object;
class MapScriptInterface;
class WorldSession;
class GameObject;
class Creature;
class Player;
class Pet;
class Transporter;
class Corpse;
class CBattleground;
class Instance;


enum MapMgrTimers
{
	MMUPDATE_OBJECTS = 0,
	MMUPDATE_SESSIONS = 1,
	MMUPDATE_FIELDS = 2,
	MMUPDATE_IDLE_OBJECTS = 3,
	MMUPDATE_ACTIVE_OBJECTS = 4,
	MMUPDATE_COUNT = 5
};

enum ObjectActiveState
{
	OBJECT_STATE_NONE	 = 0,
	OBJECT_STATE_INACTIVE = 1,
	OBJECT_STATE_ACTIVE   = 2,
};

typedef std::set<Object*> ObjectSet;
typedef std::set<Object*> UpdateQueue;
typedef std::set<Player*> PUpdateQueue;
typedef std::set<Player*> PlayerSet;
typedef unordered_map<uint32, Object*> StorageMap;
typedef set<uint64> CombatProgressMap;
typedef set<Creature*> CreatureSet;
typedef set<GameObject*> GameObjectSet;
typedef unordered_map<uint32, Creature*> CreatureSqlIdMap;
typedef unordered_map<uint32, GameObject*> GameObjectSqlIdMap;

enum GroundHeightSource
{
	GROUND_HEIGHT_SOURCE_NONE = 0,
	GROUND_HEIGHT_SOURCE_TERRAIN = 1,
	GROUND_HEIGHT_SOURCE_VMAP = 2,
	GROUND_HEIGHT_SOURCE_WATER = 3
};

ASCENT_INLINE const char* GetGroundHeightSourceName(GroundHeightSource source)
{
	switch(source)
	{
	case GROUND_HEIGHT_SOURCE_TERRAIN: return "terrain";
	case GROUND_HEIGHT_SOURCE_VMAP: return "vmap";
	case GROUND_HEIGHT_SOURCE_WATER: return "water";
	default: return "none";
	}
}

enum DirectGroundPathStatus
{
	DIRECT_GROUND_PATH_OK = 0,
	DIRECT_GROUND_PATH_INVALID_START = 1,
	DIRECT_GROUND_PATH_INVALID_SAMPLE = 2,
	DIRECT_GROUND_PATH_INVALID_DESTINATION = 3,
	DIRECT_GROUND_PATH_EXCESSIVE_CLIMB = 4,
	DIRECT_GROUND_PATH_EXCESSIVE_DROP = 5
};

ASCENT_INLINE const char* GetDirectGroundPathStatusName(DirectGroundPathStatus status)
{
	switch(status)
	{
	case DIRECT_GROUND_PATH_INVALID_START: return "invalid_start";
	case DIRECT_GROUND_PATH_INVALID_SAMPLE: return "invalid_sample";
	case DIRECT_GROUND_PATH_INVALID_DESTINATION: return "invalid_destination";
	case DIRECT_GROUND_PATH_EXCESSIVE_CLIMB: return "excessive_climb";
	case DIRECT_GROUND_PATH_EXCESSIVE_DROP: return "unsafe_drop";
	default: return "ok";
	}
}

struct GroundHeightResult
{
	float z;
	float terrainZ;
	float vmapZ;
	float waterZ;
	uint8 waterType;
	bool valid;
	bool usedHint;
	bool indoors;
	bool outdoors;
	GroundHeightSource source;

	GroundHeightResult() : z(0.0f), terrainZ(0.0f), vmapZ(-100000.0f), waterZ(0.0f), waterType(0), valid(false), usedHint(false), indoors(false), outdoors(false), source(GROUND_HEIGHT_SOURCE_NONE) {}
};

struct DirectGroundPathResult
{
	DirectGroundPathStatus status;
	uint32 sampleIndex;
	uint32 sampleCount;
	float sampleX;
	float sampleY;
	float previousZ;
	float sampleZ;
	GroundHeightSource source;

	DirectGroundPathResult() : status(DIRECT_GROUND_PATH_OK), sampleIndex(0), sampleCount(0), sampleX(0.0f), sampleY(0.0f), previousZ(0.0f), sampleZ(0.0f), source(GROUND_HEIGHT_SOURCE_NONE) {}
};

#define MAX_TRANSPORTERS_PER_MAP 25

class Transporter;
#define RESERVE_EXPAND_SIZE 1024

class SERVER_DECL MapMgr : public CellHandler <MapCell>, public EventableObject,public CThread
{
	friend class UpdateObjectThread;
	friend class ObjectUpdaterThread;
	friend class MapCell;
	friend class MapScriptInterface;
public:
		
	//This will be done in regular way soon

	Mutex m_objectinsertlock;
	ObjectSet m_objectinsertpool;
	void AddObject(Object *);

////////////////////////////////////////////////////////
// Local (mapmgr) storage/generation of GameObjects
/////////////////////////////////////////////
	uint32 m_GOArraySize;
	uint32 m_GOHighGuid;
	GameObject ** m_GOStorage;
	GameObject * CreateGameObject(uint32 entry);

	ASCENT_INLINE uint32 GenerateGameobjectGuid() { return ++m_GOHighGuid; }
	ASCENT_INLINE GameObject * GetGameObject(uint32 guid)
	{
		if(guid > m_GOHighGuid)
			return 0;
		return m_GOStorage[guid];
	}

/////////////////////////////////////////////////////////
// Local (mapmgr) storage/generation of Creatures
/////////////////////////////////////////////
	uint32 m_CreatureArraySize;
	uint32 m_CreatureHighGuid;
	Creature ** m_CreatureStorage;
	Creature * CreateCreature(uint32 entry);

	__inline Creature * GetCreature(uint32 guid)
	{
		if(guid > m_CreatureHighGuid)
			return 0;
		return m_CreatureStorage[guid];
	}

//////////////////////////////////////////////////////////
// Local (mapmgr) storage/generation of DynamicObjects
////////////////////////////////////////////
	uint32 m_DynamicObjectHighGuid;
	typedef unordered_map<uint32, DynamicObject*> DynamicObjectStorageMap;
	DynamicObjectStorageMap m_DynamicObjectStorage;
	DynamicObject * CreateDynamicObject();
	
	ASCENT_INLINE DynamicObject * GetDynamicObject(uint32 guid)
	{
		DynamicObjectStorageMap::iterator itr = m_DynamicObjectStorage.find(guid);
		return (itr != m_DynamicObjectStorage.end()) ? itr->second : 0;
	}

//////////////////////////////////////////////////////////
// Local (mapmgr) storage of pets
///////////////////////////////////////////
	typedef unordered_map<uint32, Pet*> PetStorageMap;
	PetStorageMap m_PetStorage;
	__inline Pet * GetPet(uint32 guid)
	{
		PetStorageMap::iterator itr = m_PetStorage.find(guid);
		return (itr != m_PetStorage.end()) ? itr->second : 0;
	}

//////////////////////////////////////////////////////////
// Local (mapmgr) storage of players for faster lookup
////////////////////////////////
    
    // double typedef lolz// a compile breaker..
	typedef unordered_map<uint32, Player*>                     PlayerStorageMap;
	PlayerStorageMap m_PlayerStorage;
	__inline Player * GetPlayer(uint32 guid)
	{
		PlayerStorageMap::iterator itr = m_PlayerStorage.find(guid);
		return (itr != m_PlayerStorage.end()) ? itr->second : 0;
	}

//////////////////////////////////////////////////////////
// Local (mapmgr) storage of combats in progress
////////////////////////////////
	CombatProgressMap _combatProgress;
	void AddCombatInProgress(uint64 guid)
	{
		_combatProgress.insert(guid);
	}
	void RemoveCombatInProgress(uint64 guid)
	{
		_combatProgress.erase(guid);
	}

//////////////////////////////////////////////////////////
// Lookup Wrappers
///////////////////////////////////
	Unit * GetUnit(const uint64 & guid);
	Object * _GetObject(const uint64 & guid);

	bool run();
	bool Do();

	MapMgr(Map *map, uint32 mapid, uint32 instanceid);
	~MapMgr();

	void PushObject(Object *obj);
	void PushStaticObject(Object * obj);
	void RemoveObject(Object *obj, bool free_guid);
	void ChangeObjectLocation(Object *obj); // update inrange lists
	void ChangeFarsightLocation(Player *plr, DynamicObject *farsight);

	//! Mark object as updated
	void ObjectUpdated(Object *obj);
	void UpdateCellActivity(uint32 x, uint32 y, int radius);

	// Terrain Functions
	ASCENT_INLINE float  GetLandHeight(float x, float y) { return GetBaseMap()->GetLandHeight(x, y); }
	ASCENT_INLINE float  GetWaterHeight(float x, float y) { return GetBaseMap()->GetWaterHeight(x, y); }
	ASCENT_INLINE uint8  GetWaterType(float x, float y) { return GetBaseMap()->GetWaterType(x, y); }
	ASCENT_INLINE uint8  GetWalkableState(float x, float y) { return GetBaseMap()->GetWalkableState(x, y); }
	ASCENT_INLINE uint16 GetAreaID(float x, float y) { return GetBaseMap()->GetAreaID(x, y); }
	GroundHeightResult ResolveGroundHeight(float x, float y, float zHint, bool allowWater = false);
	bool ValidateGroundMovement(Unit* mover, float x, float y, float zHint, float* outZ = NULL, GroundHeightResult* outResult = NULL, const char* reason = NULL);
	bool ValidateDirectGroundPath(Unit* mover, float startX, float startY, float startZ, float destX, float destY, float destZ, DirectGroundPathResult* outResult = NULL, const char* reason = NULL);
	bool NormalizeGroundPosition(float x, float y, float zHint, float* outZ, GroundHeightResult* outResult = NULL, const char* reason = NULL, float maxTerrainDelta = 5.0f);
	PathQueryResult BuildPath(Unit* mover, float startX, float startY, float startZ, float destX, float destY, float destZ);

	ASCENT_INLINE uint32 GetMapId() { return _mapId; }

	void PushToProcessed(Player* plr);

	ASCENT_INLINE bool HasPlayers() { return (m_PlayerStorage.size() > 0); }
	ASCENT_INLINE bool IsCombatInProgress() { return (_combatProgress.size() > 0); }
	void TeleportPlayers();

	ASCENT_INLINE uint32 GetInstanceID() { return m_instanceID; }
	ASCENT_INLINE MapInfo *GetMapInfo() { return pMapInfo; }

	bool _shutdown;

	ASCENT_INLINE MapScriptInterface * GetInterface() { return ScriptInterface; }
	virtual int32 event_GetInstanceID() { return m_instanceID; }

	void LoadAllCells();
	ASCENT_INLINE size_t GetPlayerCount() { return m_PlayerStorage.size(); }
	uint32 GetTeamPlayersCount(uint32 teamId);

	void _PerformObjectDuties();
	uint32 mLoopCounter;
	uint32 lastGameobjectUpdate;
	uint32 lastUnitUpdate;
	void EventCorpseDespawn(uint64 guid);

	time_t InactiveMoveTime;
    uint32 iInstanceMode;

	void UnloadCell(uint32 x,uint32 y);
	void EventRespawnCreature(Creature * c, MapCell * p);
	void EventRespawnGameObject(GameObject * o, MapCell * c);
	void SendMessageToCellPlayers(Object * obj, WorldPacket * packet, uint32 cell_radius = 2);
	void SendChatMessageToCellPlayers(Object * obj, WorldPacket * packet, uint32 cell_radius, uint32 langpos, int32 lang, WorldSession * originator);

	Instance * pInstance;
	void BeginInstanceExpireCountdown();
	void HookOnAreaTrigger(Player * plr, uint32 id);

	ASCENT_INLINE void SetWorldState(uint32 state, uint32 value);
	ASCENT_INLINE uint32 GetWorldState(uint32 state);
	
	// better hope to clear any references to us when calling this :P
	void InstanceShutdown()
	{
		pInstance = NULL;
		SetThreadState(THREADSTATE_TERMINATE);
	}

	// kill the worker thread only
	void KillThread()
	{
		pInstance=NULL;
		thread_kill_only = true;
		SetThreadState(THREADSTATE_TERMINATE);
		while(thread_running)
		{
			Sleep(100);
		}
	}

//#ifdef FORCED_SERVER_KEEPALIVE
	void	KillThreadWithCleanup();
	void	TeleportCorruptedPlayers();	//we have to be prepared something is corrupted here
//#endif

protected:

	//! Collect and send updates to clients
	void _UpdateObjects();

private:
	//! Objects that exist on map
 
	uint32 _mapId;
	set<Object*> _mapWideStaticObjects;

	std::map<uint32,uint32> _worldStateSet;

	bool _CellActive(uint32 x, uint32 y);
	void UpdateInRangeSet(Object *obj, Player *plObj, MapCell* cell, ByteBuffer ** buf);

	WorldPacket* BuildInitialWorldState();

public:
	// Distance a Player can "see" other objects and receive updates from them (!! ALREADY dist*dist !!)
	float m_UpdateDistance;


	// How many cells around the player are kept active/loaded.
	int m_CellUpdateRadius;

private:
	/* Update System */
	FastMutex m_updateMutex;		// use a user-mode mutex for extra speed
	UpdateQueue _updates;
	PUpdateQueue _processQueue;

	/* Sessions */
	
	SessionSet Sessions;

	/* Map Information */
	MapInfo *pMapInfo;
	uint32 m_instanceID;

	MapScriptInterface * ScriptInterface;

public:
#ifdef WIN32
	DWORD threadid;
#endif

	GameObjectSet activeGameObjects;
	CreatureSet activeCreatures;
	EventableObjectHolder eventHolder;
	CBattleground * m_battleground;
	set<Corpse*> m_corpses;
	CreatureSqlIdMap _sqlids_creatures;
	GameObjectSqlIdMap _sqlids_gameobjects;

	Creature * GetSqlIdCreature(uint32 sqlid);
	GameObject * GetSqlIdGameObject(uint32 sqlid);
	deque<uint32> _reusable_guids_gameobject;
	deque<uint32> _reusable_guids_creature;

	bool forced_expire;
	bool thread_kill_only;
	bool thread_running;
};

#endif
