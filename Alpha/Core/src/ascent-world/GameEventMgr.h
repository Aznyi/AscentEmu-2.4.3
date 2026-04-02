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

#ifndef __GAMEEVENTMGR_H
#define __GAMEEVENTMGR_H

#include <unordered_map>
#include <unordered_set>
#include <vector>

struct GameEventDefinition
{
	uint16 entry;
	uint32 occurence_minutes;
	uint32 length_minutes;
	uint16 linked_to;
	uint16 holiday;
	time_t start_time;
	time_t end_time;
	string description;
};

class SERVER_DECL GameEventMgr : public Singleton<GameEventMgr>
{
public:
	GameEventMgr();
	~GameEventMgr();

	void LoadFromDB();
	bool IsCreatureSpawnEnabled(uint32 spawn_id) const;
	bool IsGameObjectSpawnEnabled(uint32 spawn_id) const;
	bool IsQuestEnabled(uint32 quest_id) const;
	bool HasData() const { return !m_events.empty(); }

private:
	typedef std::unordered_map<uint16, GameEventDefinition> EventDefinitionMap;
	typedef std::unordered_set<uint16> ActiveEventSet;
	typedef std::unordered_map<uint32, std::vector<int16> > SpawnEventMap;

	static time_t ParseDateTime(const char* value);

	void LoadEventDefinitions();
	void LoadCreatureBindings();
	void LoadGameObjectBindings();
	void LoadQuestBindings();
	void RebuildActiveEvents();
	bool EvaluateEventWindow(const GameEventDefinition& def, time_t now) const;
	bool IsSpawnEnabled(const SpawnEventMap& bindings, uint32 spawn_id) const;
	bool IsBoundEntryEnabled(const SpawnEventMap& bindings, uint32 entry_id) const;

	EventDefinitionMap m_events;
	ActiveEventSet m_activeEvents;
	SpawnEventMap m_creatureEvents;
	SpawnEventMap m_gameObjectEvents;
	SpawnEventMap m_questEvents;
};

#define sGameEventMgr GameEventMgr::getSingleton()

#endif
