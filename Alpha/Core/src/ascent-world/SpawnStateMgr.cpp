#include "StdAfx.h"

initialiseSingleton(SpawnStateMgr);

namespace
{
	static const uint32 SPAWN_STATE_UPDATE_INTERVAL_MS = 30000;
}

SpawnStateMgr::SpawnStateMgr()
{
}

SpawnStateMgr::~SpawnStateMgr()
{
	sEventMgr.RemoveEvents(this);
}

void SpawnStateMgr::LoadFromDB()
{
	sEventMgr.RemoveEvents(this);

	m_states.clear();
	m_activeStates.clear();
	m_creatureBindings.clear();
	m_gameObjectBindings.clear();

	LoadStates();
	LoadCreatureBindings();
	LoadGameObjectBindings();
	RefreshActiveStates(true);

	sEventMgr.AddEvent(this, &SpawnStateMgr::Update, EVENT_MAP_SPAWN_EVENTS, SPAWN_STATE_UPDATE_INTERVAL_MS, 0, 0);

	Log.Notice("SpawnStateMgr", "Loaded %u timed states, %u active states, %u creature bindings, %u gameobject bindings.",
		static_cast<uint32>(m_states.size()),
		static_cast<uint32>(m_activeStates.size()),
		static_cast<uint32>(m_creatureBindings.size()),
		static_cast<uint32>(m_gameObjectBindings.size()));
}

void SpawnStateMgr::Update()
{
	RefreshActiveStates(false);
}

bool SpawnStateMgr::IsCreatureSpawnEnabled(uint32 spawn_id) const
{
	return IsSpawnEnabled(m_creatureBindings, spawn_id);
}

bool SpawnStateMgr::IsGameObjectSpawnEnabled(uint32 spawn_id) const
{
	return IsSpawnEnabled(m_gameObjectBindings, spawn_id);
}

bool SpawnStateMgr::SetStateDuration(const char* state_key, uint32 duration_seconds, const char* description)
{
	if(state_key == NULL || state_key[0] == 0)
		return false;

	const time_t active_until = UNIXTIME + duration_seconds;
	const string escaped_key = WorldDatabase.EscapeString(string(state_key));
	const string escaped_description = WorldDatabase.EscapeString(description ? string(description) : string());

	WorldDatabase.Execute("REPLACE INTO spawn_state (`state_key`, `active_until`, `description`) VALUES ('%s', %u, '%s')",
		escaped_key.c_str(), static_cast<uint32>(active_until), escaped_description.c_str());

	SpawnStateDefinition& def = m_states[string(state_key)];
	def.key = state_key;
	def.active_until = active_until;
	if(description != NULL && description[0] != 0)
		def.description = description;

	RefreshActiveStates(false);
	return true;
}

bool SpawnStateMgr::ClearState(const char* state_key)
{
	if(state_key == NULL || state_key[0] == 0)
		return false;

	const string escaped_key = WorldDatabase.EscapeString(string(state_key));
	WorldDatabase.Execute("UPDATE spawn_state SET active_until = 0 WHERE state_key = '%s'", escaped_key.c_str());

	StateDefinitionMap::iterator itr = m_states.find(state_key);
	if(itr != m_states.end())
		itr->second.active_until = 0;

	RefreshActiveStates(false);
	return true;
}

bool SpawnStateMgr::GetStateDefinition(const char* state_key, SpawnStateDefinition& def) const
{
	if(state_key == NULL || state_key[0] == 0)
		return false;

	StateDefinitionMap::const_iterator itr = m_states.find(state_key);
	if(itr == m_states.end())
		return false;

	def = itr->second;
	return true;
}

bool SpawnStateMgr::IsStateActive(const char* state_key) const
{
	if(state_key == NULL || state_key[0] == 0)
		return false;

	return m_activeStates.find(state_key) != m_activeStates.end();
}

void SpawnStateMgr::LoadStates()
{
	QueryResult* result = WorldDatabase.Query("SELECT state_key, active_until, description FROM spawn_state");
	if(result == NULL)
		return;

	do
	{
		Field* fields = result->Fetch();
		SpawnStateDefinition def;
		def.key = fields[0].GetString();
		def.active_until = static_cast<time_t>(fields[1].GetUInt32());
		def.description = fields[2].GetString();
		m_states[def.key] = def;
	}
	while(result->NextRow());

	delete result;
}

void SpawnStateMgr::LoadCreatureBindings()
{
	QueryResult* result = WorldDatabase.Query(
		"SELECT b.guid, b.state_key, b.mode, "
		"COALESCE(cs.Map, c.map, 0) "
		"FROM spawn_state_creature b "
		"LEFT JOIN creature_spawns c ON c.id = b.guid "
		"LEFT JOIN creature_staticspawns cs ON cs.id = b.guid");

	if(result == NULL)
		return;

	do
	{
		Field* fields = result->Fetch();
		SpawnStateBinding binding;
		binding.state_key = fields[1].GetString();
		binding.mode = static_cast<uint8>(fields[2].GetUInt32());
		binding.map_id = fields[3].GetUInt32();
		m_creatureBindings[fields[0].GetUInt32()].push_back(binding);
	}
	while(result->NextRow());

	delete result;
}

void SpawnStateMgr::LoadGameObjectBindings()
{
	QueryResult* result = WorldDatabase.Query(
		"SELECT b.guid, b.state_key, b.mode, "
		"COALESCE(gs.Map, g.map, 0) "
		"FROM spawn_state_gameobject b "
		"LEFT JOIN gameobject_spawns g ON g.id = b.guid "
		"LEFT JOIN gameobject_staticspawns gs ON gs.id = b.guid");

	if(result == NULL)
		return;

	do
	{
		Field* fields = result->Fetch();
		SpawnStateBinding binding;
		binding.state_key = fields[1].GetString();
		binding.mode = static_cast<uint8>(fields[2].GetUInt32());
		binding.map_id = fields[3].GetUInt32();
		m_gameObjectBindings[fields[0].GetUInt32()].push_back(binding);
	}
	while(result->NextRow());

	delete result;
}

void SpawnStateMgr::RefreshActiveStates(bool initial_load)
{
	ActiveStateSet previous_states = m_activeStates;
	ActiveStateSet refreshed_states;

	for(StateDefinitionMap::const_iterator itr = m_states.begin(); itr != m_states.end(); ++itr)
	{
		if(itr->second.active_until > UNIXTIME)
			refreshed_states.insert(itr->first);
	}

	m_activeStates.swap(refreshed_states);

	if(!initial_load && previous_states != m_activeStates)
		ApplyStateChanges(previous_states);
}

bool SpawnStateMgr::IsSpawnEnabled(const SpawnBindingMap& bindings, uint32 spawn_id) const
{
	SpawnBindingMap::const_iterator itr = bindings.find(spawn_id);
	if(itr == bindings.end())
		return true;

	bool has_positive_binding = false;
	bool positive_active = false;

	for(std::vector<SpawnStateBinding>::const_iterator binding = itr->second.begin(); binding != itr->second.end(); ++binding)
	{
		const bool state_is_active = (m_activeStates.find(binding->state_key) != m_activeStates.end());
		if(binding->mode == SPAWN_STATE_SHOW_WHEN_ACTIVE)
		{
			has_positive_binding = true;
			if(state_is_active)
				positive_active = true;
		}
		else if(binding->mode == SPAWN_STATE_HIDE_WHEN_ACTIVE)
		{
			if(state_is_active)
				return false;
		}
	}

	return !has_positive_binding || positive_active;
}

void SpawnStateMgr::ApplyStateChanges(const ActiveStateSet& previous_states)
{
	ApplyCreatureRefresh(previous_states);
	ApplyGameObjectRefresh(previous_states);
}

void SpawnStateMgr::ApplyCreatureRefresh(const ActiveStateSet& previous_states)
{
	ActiveStateSet changed_states;
	for(ActiveStateSet::const_iterator itr = previous_states.begin(); itr != previous_states.end(); ++itr)
	{
		if(m_activeStates.find(*itr) == m_activeStates.end())
			changed_states.insert(*itr);
	}

	for(ActiveStateSet::const_iterator itr = m_activeStates.begin(); itr != m_activeStates.end(); ++itr)
	{
		if(previous_states.find(*itr) == previous_states.end())
			changed_states.insert(*itr);
	}

	for(SpawnBindingMap::const_iterator itr = m_creatureBindings.begin(); itr != m_creatureBindings.end(); ++itr)
	{
		bool refresh_spawn = false;
		uint32 map_id = 0;

		for(std::vector<SpawnStateBinding>::const_iterator binding = itr->second.begin(); binding != itr->second.end(); ++binding)
		{
			if(ShouldRefreshBinding(*binding, changed_states))
			{
				refresh_spawn = true;
				map_id = binding->map_id;
				break;
			}
		}

		if(!refresh_spawn || map_id >= NUM_MAPS)
			continue;

		MapMgr* map_mgr = sInstanceMgr.GetMapMgr(map_id);
		if(map_mgr == NULL)
			continue;

		Creature* creature = map_mgr->GetSqlIdCreature(itr->first);
		const bool should_exist = IsCreatureSpawnEnabled(itr->first) && sGameEventMgr.IsCreatureSpawnEnabled(itr->first);

		if(!should_exist)
		{
			if(creature != NULL)
				creature->Despawn(0, 0);
			continue;
		}

		if(creature == NULL)
			SpawnCreatureIfNeeded(itr->first, map_id);
	}
}

void SpawnStateMgr::ApplyGameObjectRefresh(const ActiveStateSet& previous_states)
{
	ActiveStateSet changed_states;
	for(ActiveStateSet::const_iterator itr = previous_states.begin(); itr != previous_states.end(); ++itr)
	{
		if(m_activeStates.find(*itr) == m_activeStates.end())
			changed_states.insert(*itr);
	}

	for(ActiveStateSet::const_iterator itr = m_activeStates.begin(); itr != m_activeStates.end(); ++itr)
	{
		if(previous_states.find(*itr) == previous_states.end())
			changed_states.insert(*itr);
	}

	for(SpawnBindingMap::const_iterator itr = m_gameObjectBindings.begin(); itr != m_gameObjectBindings.end(); ++itr)
	{
		bool refresh_spawn = false;
		uint32 map_id = 0;

		for(std::vector<SpawnStateBinding>::const_iterator binding = itr->second.begin(); binding != itr->second.end(); ++binding)
		{
			if(ShouldRefreshBinding(*binding, changed_states))
			{
				refresh_spawn = true;
				map_id = binding->map_id;
				break;
			}
		}

		if(!refresh_spawn || map_id >= NUM_MAPS)
			continue;

		MapMgr* map_mgr = sInstanceMgr.GetMapMgr(map_id);
		if(map_mgr == NULL)
			continue;

		GameObject* go = map_mgr->GetSqlIdGameObject(itr->first);
		const bool should_exist = IsGameObjectSpawnEnabled(itr->first) && sGameEventMgr.IsGameObjectSpawnEnabled(itr->first);

		if(!should_exist)
		{
			if(go != NULL)
				go->Despawn(0);
			continue;
		}

		if(go == NULL)
			SpawnGameObjectIfNeeded(itr->first, map_id);
	}
}

bool SpawnStateMgr::ShouldRefreshBinding(const SpawnStateBinding& binding, const ActiveStateSet& changed_states) const
{
	return changed_states.find(binding.state_key) != changed_states.end();
}

void SpawnStateMgr::SpawnCreatureIfNeeded(uint32 spawn_id, uint32 map_id)
{
	MapMgr* map_mgr = sInstanceMgr.GetMapMgr(map_id);
	if(map_mgr == NULL)
		return;

	CreatureSpawn* spawn = map_mgr->GetBaseMap()->FindCreatureSpawn(spawn_id);
	if(spawn == NULL)
		return;

	MapCell* cell = map_mgr->GetCellByCoords(spawn->x, spawn->y);
	if(cell == NULL || !cell->IsLoaded())
		return;

	Creature* creature = map_mgr->CreateCreature(spawn->entry);
	creature->SetMapId(map_mgr->GetMapId());
	creature->SetInstanceID(map_mgr->GetInstanceID());
	creature->m_loadedFromDB = true;

	// Timed world-state spawns reuse the same DB-backed lifecycle as regular cached spawns.
	if(creature->Load(spawn, map_mgr->iInstanceMode, map_mgr->GetMapInfo()))
	{
		if(!creature->CanAddToWorld())
		{
			delete creature;
			return;
		}

		creature->PushToWorld(map_mgr);
	}
	else
	{
		delete creature;
	}
}

void SpawnStateMgr::SpawnGameObjectIfNeeded(uint32 spawn_id, uint32 map_id)
{
	MapMgr* map_mgr = sInstanceMgr.GetMapMgr(map_id);
	if(map_mgr == NULL)
		return;

	GOSpawn* spawn = map_mgr->GetBaseMap()->FindGameObjectSpawn(spawn_id);
	if(spawn == NULL)
		return;

	MapCell* cell = map_mgr->GetCellByCoords(spawn->x, spawn->y);
	if(cell == NULL || !cell->IsLoaded())
		return;

	GameObject* go = map_mgr->CreateGameObject(spawn->entry);
	if(go->Load(spawn))
	{
		go->m_loadedFromDB = true;
		go->PushToWorld(map_mgr);
	}
	else
	{
		delete go;
	}
}
