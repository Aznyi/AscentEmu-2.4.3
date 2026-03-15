#include "StdAfx.h"
#include "../GossipScripts/Setup.h"

namespace
{
	static const uint32 QUEST_ONYXIA_HORDE_TURNIN = 7491;
	static const uint32 QUEST_ONYXIA_ALLIANCE_TURNIN = 7496;
	static const uint32 QUEST_NEFARIAN_ALLIANCE_TURNIN = 7782;
	static const uint32 QUEST_NEFARIAN_HORDE_TURNIN = 7784;

	static const uint32 SPELL_RALLYING_CRY_OF_THE_DRAGONSLAYER = 22888;

	static const uint32 MAP_EASTERN_KINGDOMS = 0;
	static const uint32 MAP_KALIMDOR = 1;
	static const uint32 ZONE_STORMWIND = 1519;
	static const uint32 ZONE_ORGRIMMAR = 1637;
	static const uint32 WORLD_BUFF_EVENT_DELAY_MS = 15 * 1000;
	static const uint32 WORLD_BUFF_PULSE_DELAY_MS = 3 * 1000;

	static const uint32 ONYXIA_HEAD_DISPLAY_SECONDS = 6 * 60 * 60;
	static const uint32 NEFARIAN_HEAD_DISPLAY_SECONDS = 8 * 60 * 60;

	struct WorldBuffTrigger
	{
		uint32 quest_id;
		uint32 map_id;
		uint32 zone_id;
		const char* state_key;
		const char* description;
		const char* announcement;
		uint32 display_duration_seconds;
	};

	static const WorldBuffTrigger WORLD_BUFF_TRIGGERS[] =
	{
		{ QUEST_ONYXIA_HORDE_TURNIN, MAP_KALIMDOR, ZONE_ORGRIMMAR, "orgrimmar_onyxia_head", "Orgrimmar Onyxia head display", "Overlord Runthak yells: Behold, heroes! The brood mother Onyxia has fallen! Rally to the Horde and celebrate this victory!", ONYXIA_HEAD_DISPLAY_SECONDS },
		{ QUEST_ONYXIA_ALLIANCE_TURNIN, MAP_EASTERN_KINGDOMS, ZONE_STORMWIND, "stormwind_onyxia_head", "Stormwind Onyxia head display", "Major Mattingly yells: Citizens of Stormwind, Onyxia is dead! Rally to your heroes and celebrate this glorious victory!", ONYXIA_HEAD_DISPLAY_SECONDS },
		{ QUEST_NEFARIAN_ALLIANCE_TURNIN, MAP_EASTERN_KINGDOMS, ZONE_STORMWIND, "stormwind_nefarian_head", "Stormwind Nefarian head display", "Field Marshal Afrasiabi yells: Nefarian has been slain! Let all of Stormwind bear witness to this great victory!", NEFARIAN_HEAD_DISPLAY_SECONDS },
		{ QUEST_NEFARIAN_HORDE_TURNIN, MAP_KALIMDOR, ZONE_ORGRIMMAR, "orgrimmar_nefarian_head", "Orgrimmar Nefarian head display", "High Overlord Saurfang yells: Nefarian is dead! Let the Horde honor this triumph and answer the rallying cry!", NEFARIAN_HEAD_DISPLAY_SECONDS },
	};

	static const WorldBuffTrigger* FindWorldBuffTrigger(uint32 quest_id, uint32* trigger_index = NULL)
	{
		for(size_t i = 0; i < sizeof(WORLD_BUFF_TRIGGERS) / sizeof(WORLD_BUFF_TRIGGERS[0]); ++i)
		{
			if(WORLD_BUFF_TRIGGERS[i].quest_id == quest_id)
			{
				if(trigger_index != NULL)
					*trigger_index = static_cast<uint32>(i);

				return &WORLD_BUFF_TRIGGERS[i];
			}
		}

		return NULL;
	}

	class CityHeadWorldBuffController : public EventableObject
	{
	public:
		bool TryStartEvent(Player* player, uint32 quest_id)
		{
			uint32 trigger_index = 0;
			const WorldBuffTrigger* trigger = FindWorldBuffTrigger(quest_id, &trigger_index);
			if(trigger == NULL || player == NULL || !IsEligiblePlayer(player, *trigger))
				return false;

			if(m_pendingEvents.find(trigger->state_key) != m_pendingEvents.end() || sSpawnStateMgr.IsStateActive(trigger->state_key))
				return false;

			m_pendingEvents.insert(trigger->state_key);
			sEventMgr.AddEvent(this, &CityHeadWorldBuffController::BeginEvent, trigger_index, EVENT_SCRIPT_UPDATE_EVENT, WORLD_BUFF_EVENT_DELAY_MS, 1, EVENT_FLAG_DO_NOT_EXECUTE_IN_WORLD_CONTEXT);
			return true;
		}

	private:
		void BeginEvent(uint32 trigger_index)
		{
			if(trigger_index >= static_cast<uint32>(sizeof(WORLD_BUFF_TRIGGERS) / sizeof(WORLD_BUFF_TRIGGERS[0])))
				return;

			const WorldBuffTrigger& trigger = WORLD_BUFF_TRIGGERS[trigger_index];
			m_pendingEvents.erase(trigger.state_key);

			if(sSpawnStateMgr.IsStateActive(trigger.state_key))
				return;

			if(!sSpawnStateMgr.SetStateDuration(trigger.state_key, trigger.display_duration_seconds, trigger.description))
				return;

			BroadcastAnnouncement(trigger);
			sEventMgr.AddEvent(this, &CityHeadWorldBuffController::PulseBuff, trigger_index, EVENT_GMSCRIPT_EVENT, WORLD_BUFF_PULSE_DELAY_MS, 1, EVENT_FLAG_DO_NOT_EXECUTE_IN_WORLD_CONTEXT);
		}

		void PulseBuff(uint32 trigger_index)
		{
			if(trigger_index >= static_cast<uint32>(sizeof(WORLD_BUFF_TRIGGERS) / sizeof(WORLD_BUFF_TRIGGERS[0])))
				return;

			const WorldBuffTrigger& trigger = WORLD_BUFF_TRIGGERS[trigger_index];
			MapMgr* map_mgr = sInstanceMgr.GetMapMgr(trigger.map_id);
			if(map_mgr == NULL)
				return;

			SpellEntry* rallying_cry = dbcSpell.LookupEntry(SPELL_RALLYING_CRY_OF_THE_DRAGONSLAYER);
			if(rallying_cry == NULL)
				return;

			MapMgr::PlayerStorageMap::iterator itr = map_mgr->m_PlayerStorage.begin();
			for(; itr != map_mgr->m_PlayerStorage.end(); ++itr)
			{
				Player* player = itr->second;
				if(!IsEligiblePlayer(player, trigger, rallying_cry))
					continue;

				if(player->HasActiveAura(SPELL_RALLYING_CRY_OF_THE_DRAGONSLAYER))
					continue;

				Aura* aura = new Aura(rallying_cry, -1, player, player);
				player->AddAura(aura);
			}
		}

		void BroadcastAnnouncement(const WorldBuffTrigger& trigger)
		{
			MapMgr* map_mgr = sInstanceMgr.GetMapMgr(trigger.map_id);
			if(map_mgr == NULL)
				return;

			MapMgr::PlayerStorageMap::iterator itr = map_mgr->m_PlayerStorage.begin();
			for(; itr != map_mgr->m_PlayerStorage.end(); ++itr)
			{
				Player* player = itr->second;
				if(!IsInTargetCity(player, trigger))
					continue;

				player->BroadcastMessage("%s", trigger.announcement);
			}
		}

		bool IsEligiblePlayer(Player* player, const WorldBuffTrigger& trigger, SpellEntry* rallying_cry = NULL) const
		{
			return player != NULL &&
				player->IsInWorld() &&
				player->isAlive() &&
				!player->isDead() &&
				IsInTargetCity(player, trigger) &&
				(rallying_cry == NULL || player->getLevel() >= rallying_cry->spellLevel);
		}

		bool IsInTargetCity(Player* player, const WorldBuffTrigger& trigger) const
		{
			if(player == NULL || player->GetMapId() != trigger.map_id)
				return false;

			if(player->GetZoneId() == trigger.zone_id || player->GetAreaID() == trigger.zone_id)
				return true;

			AreaTable* area = dbcArea.LookupEntry(player->GetAreaID());
			return area != NULL && area->ZoneId == trigger.zone_id;
		}

		std::set<string> m_pendingEvents;
	};

	CityHeadWorldBuffController sCityHeadWorldBuffController;

	static void OnWorldBuffQuestFinished(Player* player, Quest* quest)
	{
		if(player == NULL || quest == NULL)
			return;

		sCityHeadWorldBuffController.TryStartEvent(player, quest->id);
	}
}

void SetupWorldBuffs(ScriptMgr* mgr)
{
	mgr->register_hook(SERVER_HOOK_EVENT_ON_QUEST_FINISHED, (void*)&OnWorldBuffQuestFinished);
}
