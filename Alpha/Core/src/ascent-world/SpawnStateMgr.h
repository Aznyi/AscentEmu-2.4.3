#ifndef __SPAWNSTATEMGR_H
#define __SPAWNSTATEMGR_H

#include <unordered_map>
#include <unordered_set>
#include <vector>

enum SpawnStateBindingMode
{
	SPAWN_STATE_SHOW_WHEN_ACTIVE = 1,
	SPAWN_STATE_HIDE_WHEN_ACTIVE = 2,
};

struct SpawnStateDefinition
{
	string key;
	time_t active_until;
	string description;
};

struct SpawnStateBinding
{
	string state_key;
	uint8 mode;
	uint32 map_id;
};

class SERVER_DECL SpawnStateMgr : public Singleton<SpawnStateMgr>, public EventableObject
{
public:
	SpawnStateMgr();
	~SpawnStateMgr();

	void LoadFromDB();
	void Update();

	bool IsCreatureSpawnEnabled(uint32 spawn_id) const;
	bool IsGameObjectSpawnEnabled(uint32 spawn_id) const;

	bool SetStateDuration(const char* state_key, uint32 duration_seconds, const char* description = NULL);
	bool ClearState(const char* state_key);
	bool GetStateDefinition(const char* state_key, SpawnStateDefinition& def) const;
	bool IsStateActive(const char* state_key) const;

private:
	typedef std::unordered_map<string, SpawnStateDefinition> StateDefinitionMap;
	typedef std::unordered_set<string> ActiveStateSet;
	typedef std::unordered_map<uint32, std::vector<SpawnStateBinding> > SpawnBindingMap;

	void LoadStates();
	void LoadCreatureBindings();
	void LoadGameObjectBindings();
	void RefreshActiveStates(bool initial_load);
	bool IsSpawnEnabled(const SpawnBindingMap& bindings, uint32 spawn_id) const;
	void ApplyStateChanges(const ActiveStateSet& previous_states);
	void ApplyCreatureRefresh(const ActiveStateSet& changed_states);
	void ApplyGameObjectRefresh(const ActiveStateSet& changed_states);
	bool ShouldRefreshBinding(const SpawnStateBinding& binding, const ActiveStateSet& changed_states) const;
	void SpawnCreatureIfNeeded(uint32 spawn_id, uint32 map_id);
	void SpawnGameObjectIfNeeded(uint32 spawn_id, uint32 map_id);

	StateDefinitionMap m_states;
	ActiveStateSet m_activeStates;
	SpawnBindingMap m_creatureBindings;
	SpawnBindingMap m_gameObjectBindings;
};

#define sSpawnStateMgr SpawnStateMgr::getSingleton()

#endif
