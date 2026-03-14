#include "StdAfx.h"
#include "Setup.h"

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

	// The player buff lasts two hours. The head display stays up longer so the
	// city aftermath remains visible after the initial announcement.
	static const uint32 WORLD_BUFF_DURATION_SECONDS = 2 * 60 * 60;
	static const uint32 ONYXIA_HEAD_DISPLAY_SECONDS = 6 * 60 * 60;
	static const uint32 NEFARIAN_HEAD_DISPLAY_SECONDS = 8 * 60 * 60;

	struct WorldBuffTrigger
	{
		uint32 quest_id;
		uint32 map_id;
		uint32 zone_id;
		const char* state_key;
		const char* description;
		uint32 display_duration_seconds;
	};

	static const WorldBuffTrigger WORLD_BUFF_TRIGGERS[] =
	{
		{ QUEST_ONYXIA_HORDE_TURNIN, MAP_KALIMDOR, ZONE_ORGRIMMAR, "orgrimmar_onyxia_head", "Orgrimmar Onyxia head display", ONYXIA_HEAD_DISPLAY_SECONDS },
		{ QUEST_ONYXIA_ALLIANCE_TURNIN, MAP_EASTERN_KINGDOMS, ZONE_STORMWIND, "stormwind_onyxia_head", "Stormwind Onyxia head display", ONYXIA_HEAD_DISPLAY_SECONDS },
		{ QUEST_NEFARIAN_ALLIANCE_TURNIN, MAP_EASTERN_KINGDOMS, ZONE_STORMWIND, "stormwind_nefarian_head", "Stormwind Nefarian head display", NEFARIAN_HEAD_DISPLAY_SECONDS },
		{ QUEST_NEFARIAN_HORDE_TURNIN, MAP_KALIMDOR, ZONE_ORGRIMMAR, "orgrimmar_nefarian_head", "Orgrimmar Nefarian head display", NEFARIAN_HEAD_DISPLAY_SECONDS },
	};

	static const WorldBuffTrigger* FindWorldBuffTrigger(uint32 quest_id)
	{
		for(size_t i = 0; i < sizeof(WORLD_BUFF_TRIGGERS) / sizeof(WORLD_BUFF_TRIGGERS[0]); ++i)
		{
			if(WORLD_BUFF_TRIGGERS[i].quest_id == quest_id)
				return &WORLD_BUFF_TRIGGERS[i];
		}

		return NULL;
	}

	static void ApplyRallyingCryToCapital(const WorldBuffTrigger& trigger)
	{
		MapMgr* map_mgr = sInstanceMgr.GetMapMgr(trigger.map_id);
		if(map_mgr == NULL)
			return;

		MapMgr::PlayerStorageMap::iterator itr = map_mgr->m_PlayerStorage.begin();
		for(; itr != map_mgr->m_PlayerStorage.end(); ++itr)
		{
			Player* player = itr->second;
			if(player == NULL || !player->IsInWorld())
				continue;

			if(player->GetZoneId() != trigger.zone_id)
				continue;

			// TBC-era Dragonslayer is a vanilla world buff. Players above 63 do
			// not receive it, so keep the capital event from applying it to later
			// expansion endgame characters.
			if(player->getLevel() > 63)
				continue;

			player->CastSpell(player, SPELL_RALLYING_CRY_OF_THE_DRAGONSLAYER, true);
		}
	}

	static void OnWorldBuffQuestFinished(Player* player, Quest* quest)
	{
		if(player == NULL || quest == NULL)
			return;

		const WorldBuffTrigger* trigger = FindWorldBuffTrigger(quest->id);
		if(trigger == NULL)
			return;

		if(sSpawnStateMgr.IsStateActive(trigger->state_key))
			return;

		sSpawnStateMgr.SetStateDuration(trigger->state_key, trigger->display_duration_seconds, trigger->description);
		ApplyRallyingCryToCapital(*trigger);
	}
}

void SetupWorldBuffs(ScriptMgr* mgr)
{
	mgr->register_hook(SERVER_HOOK_EVENT_ON_QUEST_FINISHED, (void*)&OnWorldBuffQuestFinished);
}
