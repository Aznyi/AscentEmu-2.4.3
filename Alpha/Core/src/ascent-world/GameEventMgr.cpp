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

initialiseSingleton(GameEventMgr);

GameEventMgr::GameEventMgr()
{
}

GameEventMgr::~GameEventMgr()
{
}

void GameEventMgr::LoadFromDB()
{
	m_events.clear();
	m_activeEvents.clear();
	m_creatureEvents.clear();
	m_gameObjectEvents.clear();
	m_questEvents.clear();

	LoadEventDefinitions();
	LoadCreatureBindings();
	LoadGameObjectBindings();
	LoadQuestBindings();
	RebuildActiveEvents();

	Log.Notice("GameEventMgr", "Loaded %u definitions, %u active events, %u creature bindings, %u gameobject bindings, %u quest bindings.",
		static_cast<uint32>(m_events.size()),
		static_cast<uint32>(m_activeEvents.size()),
		static_cast<uint32>(m_creatureEvents.size()),
		static_cast<uint32>(m_gameObjectEvents.size()),
		static_cast<uint32>(m_questEvents.size()));
}

bool GameEventMgr::IsCreatureSpawnEnabled(uint32 spawn_id) const
{
	return IsSpawnEnabled(m_creatureEvents, spawn_id);
}

bool GameEventMgr::IsGameObjectSpawnEnabled(uint32 spawn_id) const
{
	return IsSpawnEnabled(m_gameObjectEvents, spawn_id);
}

bool GameEventMgr::IsQuestEnabled(uint32 quest_id) const
{
	return IsBoundEntryEnabled(m_questEvents, quest_id);
}

time_t GameEventMgr::ParseDateTime(const char* value)
{
	if(value == NULL || value[0] == 0)
		return 0;

	int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
	if(sscanf(value, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6)
		return 0;

	tm parsed;
	memset(&parsed, 0, sizeof(parsed));
	parsed.tm_year = year - 1900;
	parsed.tm_mon = month - 1;
	parsed.tm_mday = day;
	parsed.tm_hour = hour;
	parsed.tm_min = minute;
	parsed.tm_sec = second;
	parsed.tm_isdst = -1;
	return mktime(&parsed);
}

void GameEventMgr::LoadEventDefinitions()
{
	QueryResult* result = WorldDatabase.Query(
		"SELECT e.entry, e.occurence, e.length, e.holiday, e.linkedTo, e.description, "
		"COALESCE(t.start_time, '1970-01-01 00:00:00'), COALESCE(t.end_time, '1970-01-01 00:00:00') "
		"FROM game_event e LEFT JOIN game_event_time t ON t.entry = e.entry");

	if(result == NULL)
	{
		Log.Notice("GameEventMgr", "No game_event data found. Event-gated map spawns remain disabled.");
		return;
	}

	do
	{
		Field* fields = result->Fetch();
		GameEventDefinition def;
		def.entry = static_cast<uint16>(fields[0].GetUInt32());
		def.occurence_minutes = fields[1].GetUInt32();
		def.length_minutes = fields[2].GetUInt32();
		def.holiday = static_cast<uint16>(fields[3].GetUInt32());
		def.linked_to = static_cast<uint16>(fields[4].GetUInt32());
		def.description = fields[5].GetString();
		def.start_time = ParseDateTime(fields[6].GetString());
		def.end_time = ParseDateTime(fields[7].GetString());
		m_events[def.entry] = def;
	}
	while(result->NextRow());

	delete result;
}

void GameEventMgr::LoadCreatureBindings()
{
	QueryResult* result = WorldDatabase.Query("SELECT guid, event FROM game_event_creature");
	if(result == NULL)
		return;

	do
	{
		Field* fields = result->Fetch();
		m_creatureEvents[fields[0].GetUInt32()].push_back(static_cast<int16>(fields[1].GetInt32()));
	}
	while(result->NextRow());

	delete result;
}

void GameEventMgr::LoadGameObjectBindings()
{
	QueryResult* result = WorldDatabase.Query("SELECT guid, event FROM game_event_gameobject");
	if(result == NULL)
		return;

	do
	{
		Field* fields = result->Fetch();
		m_gameObjectEvents[fields[0].GetUInt32()].push_back(static_cast<int16>(fields[1].GetInt32()));
	}
	while(result->NextRow());

	delete result;
}

void GameEventMgr::LoadQuestBindings()
{
	QueryResult* result = WorldDatabase.Query("SELECT quest, event FROM game_event_quest");
	if(result == NULL)
		return;

	do
	{
		Field* fields = result->Fetch();
		m_questEvents[fields[0].GetUInt32()].push_back(static_cast<int16>(fields[1].GetInt32()));
	}
	while(result->NextRow());

	delete result;
}

void GameEventMgr::RebuildActiveEvents()
{
	const time_t now = UNIXTIME;
	bool changed = true;

	while(changed)
	{
		changed = false;
		for(EventDefinitionMap::const_iterator itr = m_events.begin(); itr != m_events.end(); ++itr)
		{
			if(m_activeEvents.find(itr->first) != m_activeEvents.end())
				continue;

			const GameEventDefinition& def = itr->second;
			if(def.linked_to != 0 && m_activeEvents.find(def.linked_to) == m_activeEvents.end())
				continue;

			if(EvaluateEventWindow(def, now))
			{
				m_activeEvents.insert(def.entry);
				changed = true;
			}
		}
	}
}

bool GameEventMgr::EvaluateEventWindow(const GameEventDefinition& def, time_t now) const
{
	if(def.start_time != 0 && now < def.start_time)
		return false;

	if(def.end_time != 0 && now >= def.end_time)
		return false;

	if(def.start_time == 0 || def.occurence_minutes == 0)
		return true;

	const time_t elapsed_seconds = now - def.start_time;
	const uint64 occurence_seconds = static_cast<uint64>(def.occurence_minutes) * 60ULL;
	const uint64 length_seconds = static_cast<uint64>(def.length_minutes) * 60ULL;

	if(occurence_seconds == 0)
		return true;

	return (static_cast<uint64>(elapsed_seconds) % occurence_seconds) < length_seconds;
}

bool GameEventMgr::IsSpawnEnabled(const SpawnEventMap& bindings, uint32 spawn_id) const
{
	return IsBoundEntryEnabled(bindings, spawn_id);
}

bool GameEventMgr::IsBoundEntryEnabled(const SpawnEventMap& bindings, uint32 entry_id) const
{
	SpawnEventMap::const_iterator itr = bindings.find(entry_id);
	if(itr == bindings.end())
		return true;

	bool has_positive_binding = false;
	bool positive_active = false;

	for(std::vector<int16>::const_iterator evt = itr->second.begin(); evt != itr->second.end(); ++evt)
	{
		if(*evt > 0)
		{
			has_positive_binding = true;
			if(m_activeEvents.find(static_cast<uint16>(*evt)) != m_activeEvents.end())
				positive_active = true;
		}
		else if(*evt < 0)
		{
			const uint16 event_id = static_cast<uint16>(-*evt);
			if(m_activeEvents.find(event_id) != m_activeEvents.end())
				return false;
		}
	}

	return !has_positive_binding || positive_active;
}
