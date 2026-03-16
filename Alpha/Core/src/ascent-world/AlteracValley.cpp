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

#define AV_OBJECTIVE_COUNT 15
#define EVENT_AV_OBJECTIVES_UPDATE 9900
#define EVENT_AV_MINE_TICK 9901

#define AV_WS_ALLIANCE_SCORE 3127
#define AV_WS_HORDE_SCORE 3128
#define AV_WS_SHOW_ALLIANCE_SCORE 3133
#define AV_WS_SHOW_HORDE_SCORE 3134
#define AV_WS_SCOREBOARD_SHOW 3131

#define AV_GO_GRAVE_BANNER_ALLIANCE 178365
#define AV_GO_GRAVE_BANNER_HORDE 178364
#define AV_GO_TOWER_BANNER_ALLIANCE 178925
#define AV_GO_TOWER_BANNER_HORDE 178943
#define AV_GO_GRAVE_BANNER_ALLIANCE_ASSAULT 179286
#define AV_GO_GRAVE_BANNER_HORDE_ASSAULT 179287
#define AV_GO_TOWER_BANNER_ALLIANCE_ASSAULT 178940
#define AV_GO_TOWER_BANNER_HORDE_ASSAULT 179435
#define AV_GO_SNOWFALL_BANNER 180418
#define AV_GO_START_GATE 180424
#define AV_GO_TOWER_BURNING 179065

#define AV_GUARD_SCAN_RADIUS 18.0f
#define AV_FACTION_ALLIANCE 2
#define AV_FACTION_HORDE 1
#define AV_FACTION_NEUTRAL 35
#define AV_VISUAL_FACTION_ALLIANCE 84
#define AV_VISUAL_FACTION_HORDE 83

#define AV_NPC_VANNDAR 11948
#define AV_NPC_DREKTHAR 11946
#define AV_NPC_BALINDA 11949
#define AV_NPC_GALVANGAR 11947

static const AlteracValley::AVObjectiveTemplate AV_OBJECTIVES[AV_OBJECTIVE_COUNT] =
{
	{ AV_OBJECTIVE_GRAVEYARD, "Stormpike Aid Station", AV_GO_GRAVE_BANNER_ALLIANCE, AV_GO_GRAVE_BANNER_HORDE, AV_GO_GRAVE_BANNER_ALLIANCE_ASSAULT, AV_GO_GRAVE_BANNER_HORDE_ASSAULT, 0, AV_CONTROLED_STORMPIKE_AID_STATION_ALLIANCE, 0, AV_ASSAULTED_STORMPIKE_AID_STATION_ALLIANCE, AV_ASSAULTED_STORMPIKE_AID_STATION_HORDE, 0, 638.592f, -32.422f, 46.0608f, -1.62316f, 643.309f, 37.692f, 69.0624f, 1.5708f, 0, 0, true },
	{ AV_OBJECTIVE_GRAVEYARD, "Stormpike Graveyard", AV_GO_GRAVE_BANNER_ALLIANCE, AV_GO_GRAVE_BANNER_HORDE, AV_GO_GRAVE_BANNER_ALLIANCE_ASSAULT, AV_GO_GRAVE_BANNER_HORDE_ASSAULT, 0, AV_CONTROLED_STORMPIKE_GRAVE_ALLIANCE, AV_CONTROLED_STORMPIKE_GRAVE_HORDE, AV_ASSAULTED_STORMPIKE_GRAVE_ALLIANCE, AV_ASSAULTED_STORMPIKE_GRAVE_HORDE, 0, 669.007f, -294.078f, 30.2909f, 2.77507f, 672.984f, -367.533f, 29.8641f, 5.18363f, 0, 0, true },
	{ AV_OBJECTIVE_GRAVEYARD, "Stonehearth Graveyard", AV_GO_GRAVE_BANNER_ALLIANCE, AV_GO_GRAVE_BANNER_HORDE, AV_GO_GRAVE_BANNER_ALLIANCE_ASSAULT, AV_GO_GRAVE_BANNER_HORDE_ASSAULT, 0, AV_CONTROLED_STONEHEART_GRAVE_ALLIANCE, AV_CONTROLED_STONEHEART_GRAVE_HORDE, AV_ASSAULTED_STONEHEART_GRAVE_ALLIANCE, AV_ASSAULTED_STONEHEART_GRAVE_HORDE, 0, 77.5044f, -404.587f, 46.7825f, 2.28638f, 73.2605f, -488.183f, 48.8846f, 4.76475f, 0, 0, true },
	{ AV_OBJECTIVE_GRAVEYARD, "Snowfall Graveyard", AV_GO_GRAVE_BANNER_ALLIANCE, AV_GO_GRAVE_BANNER_HORDE, AV_GO_GRAVE_BANNER_ALLIANCE_ASSAULT, AV_GO_GRAVE_BANNER_HORDE_ASSAULT, AV_GO_SNOWFALL_BANNER, AV_UNCONTROLED_SNOWFALL_GRAVE, 0, AV_ASSAULTED_SNOWFALL_GRAVE_ALLIANCE, AV_ASSAULTED_SNOWFALL_GRAVE_HORDE, 0, -202.611f, -112.778f, 78.4872f, -1.25664f, -160.638f, 18.4137f, 77.2141f, 1.37881f, -1, 0, true },
	{ AV_OBJECTIVE_GRAVEYARD, "Iceblood Graveyard", AV_GO_GRAVE_BANNER_ALLIANCE, AV_GO_GRAVE_BANNER_HORDE, AV_GO_GRAVE_BANNER_ALLIANCE_ASSAULT, AV_GO_GRAVE_BANNER_HORDE_ASSAULT, 0, 0, AV_CONTROLED_ICEBLOOD_GRAVE_HORDE, AV_ASSAULTED_ICEBLOOD_GRAVE_ALLIANCE, AV_ASSAULTED_ICEBLOOD_GRAVE_HORDE, 0, -612.672f, -396.693f, 60.8584f, 3.08923f, -540.714f, -397.231f, 50.1007f, 5.5676f, 1, 0, true },
	{ AV_OBJECTIVE_GRAVEYARD, "Frostwolf Graveyard", AV_GO_GRAVE_BANNER_ALLIANCE, AV_GO_GRAVE_BANNER_HORDE, AV_GO_GRAVE_BANNER_ALLIANCE_ASSAULT, AV_GO_GRAVE_BANNER_HORDE_ASSAULT, 0, 0, AV_CONTROLED_FROSTWOLF_GRAVE_HORDE, AV_ASSAULTED_FROSTWOLF_GRAVE_ALLIANCE, AV_ASSAULTED_FROSTWOLF_GRAVE_HORDE, 0, -1082.53f, -346.567f, 54.9771f, -1.55334f, -1082.53f, -346.567f, 54.9771f, -1.55334f, 1, 0, true },
	{ AV_OBJECTIVE_GRAVEYARD, "Frostwolf Relief Hut", AV_GO_GRAVE_BANNER_ALLIANCE, AV_GO_GRAVE_BANNER_HORDE, AV_GO_GRAVE_BANNER_ALLIANCE_ASSAULT, AV_GO_GRAVE_BANNER_HORDE_ASSAULT, 0, 0, AV_CONTROLED_FROSTWOLF_RELIFHUNT_HORDE, AV_ASSAULTED_FROSTWOLF_RELIEF_HUT_ALLIANCE, AV_ASSAULTED_FROSTWOLF_RELIEF_HUT_HORDE, 0, -1402.21f, -307.431f, 89.4424f, 0.191986f, -1488.14f, -329.205f, 100.853f, 3.59538f, 1, 0, true },

	{ AV_OBJECTIVE_BUNKER, "Dun Baldar South Bunker", AV_GO_TOWER_BANNER_ALLIANCE, AV_GO_TOWER_BANNER_HORDE, AV_GO_TOWER_BANNER_ALLIANCE_ASSAULT, AV_GO_TOWER_BANNER_HORDE_ASSAULT, 0, AV_CONTROLED_DUBALDER_SOUTH_BUNKER_ALLIANCE, 0, 0, AV_DUBALDAR_SOUTH_BUNKER_ASSAULTED, AV_DUBALDAR_SOUTH_BUNKER_DESTROYED, 553.779f, -78.6566f, 51.9378f, -1.22173f, 553.779f, -78.6566f, 51.9378f, -1.22173f, 0, 14763, false },
	{ AV_OBJECTIVE_BUNKER, "Dun Baldar North Bunker", AV_GO_TOWER_BANNER_ALLIANCE, AV_GO_TOWER_BANNER_HORDE, AV_GO_TOWER_BANNER_ALLIANCE_ASSAULT, AV_GO_TOWER_BANNER_HORDE_ASSAULT, 0, AV_CONTROLED_DUBALDER_NORTH_BUNKER_ALLIANCE, 0, 0, AV_DUBALDAR_NORTH_BUNKER_ASSAULTED, AV_DUBALDAR_NORTH_BUNKER_DESTROYED, 674.001f, -143.125f, 63.6615f, 0.994838f, 674.001f, -143.125f, 63.6615f, 0.994838f, 0, 14762, false },
	{ AV_OBJECTIVE_BUNKER, "Icewing Bunker", AV_GO_TOWER_BANNER_ALLIANCE, AV_GO_TOWER_BANNER_HORDE, AV_GO_TOWER_BANNER_ALLIANCE_ASSAULT, AV_GO_TOWER_BANNER_HORDE_ASSAULT, 0, AV_CONTROLED_ICEWING_BUNKER_ALLIANCE, 0, 0, AV_ICEWING_BUNKER_ASSAULTED, AV_ICEWING_BUNKER_DESTROYED, 203.238f, -360.264f, 56.3862f, -0.872665f, 203.238f, -360.264f, 56.3862f, -0.872665f, 0, 14764, false },
	{ AV_OBJECTIVE_BUNKER, "Stonehearth Bunker", AV_GO_TOWER_BANNER_ALLIANCE, AV_GO_TOWER_BANNER_HORDE, AV_GO_TOWER_BANNER_ALLIANCE_ASSAULT, AV_GO_TOWER_BANNER_HORDE_ASSAULT, 0, AV_CONTROLED_STONEHEART_BUNKER_ALLIANCE, 0, 0, AV_STONEHEARTH_BUNKER_ASSAULTED, AV_STONEHEARTH_BUNKER_DESTROYED, -152.434f, -441.615f, 40.3971f, -1.93731f, -152.434f, -441.615f, 40.3971f, -1.93731f, 0, 14765, false },
	{ AV_OBJECTIVE_TOWER, "Iceblood Tower", AV_GO_TOWER_BANNER_ALLIANCE, AV_GO_TOWER_BANNER_HORDE, AV_GO_TOWER_BANNER_ALLIANCE_ASSAULT, AV_GO_TOWER_BANNER_HORDE_ASSAULT, 0, 0, AV_CONTROLED_ICEBLOOD_TOWER_HORDE, AV_ICEBLOOD_TOWER_ASSAULTED, 0, AV_ICEBLOOD_TOWER_DESTROYED, -571.88f, -262.777f, 75.0087f, -0.802851f, -571.88f, -262.777f, 75.0087f, -0.802851f, 1, 14773, false },
	{ AV_OBJECTIVE_TOWER, "Tower Point", AV_GO_TOWER_BANNER_ALLIANCE, AV_GO_TOWER_BANNER_HORDE, AV_GO_TOWER_BANNER_ALLIANCE_ASSAULT, AV_GO_TOWER_BANNER_HORDE_ASSAULT, 0, 0, AV_CONTROLED_TOWER_POINT_HORDE, AV_TOWER_POINT_ASSAULTED, 0, AV_TOWER_POINT_DESTROYED, -768.087f, -362.666f, 90.8949f, 1.11701f, -768.087f, -362.666f, 90.8949f, 1.11701f, 1, 14776, false },
	{ AV_OBJECTIVE_TOWER, "East Frostwolf Tower", AV_GO_TOWER_BANNER_ALLIANCE, AV_GO_TOWER_BANNER_HORDE, AV_GO_TOWER_BANNER_ALLIANCE_ASSAULT, AV_GO_TOWER_BANNER_HORDE_ASSAULT, 0, 0, AV_CONTROLED_EAST_FROSTWOLF_TOWER_HORDE, AV_EAST_FROSTWOLF_TOWER_ASSAULTED, 0, AV_EAST_FROSTWOLF_TOWER_DESTROYED, -1302.87f, -316.968f, 113.867f, 2.00713f, -1302.87f, -316.968f, 113.867f, 2.00713f, 1, 14772, false },
	{ AV_OBJECTIVE_TOWER, "West Frostwolf Tower", AV_GO_TOWER_BANNER_ALLIANCE, AV_GO_TOWER_BANNER_HORDE, AV_GO_TOWER_BANNER_ALLIANCE_ASSAULT, AV_GO_TOWER_BANNER_HORDE_ASSAULT, 0, 0, AV_CONTROLED_WEST_FROSTWOLF_TOWER_HORDE, AV_WEST_FROSTWOLF_TOWER_ASSAULTED, 0, AV_WEST_FROSTWOLF_TOWER_DESTROYED, -1297.72f, -266.741f, 114.151f, -2.96706f, -1297.72f, -266.741f, 114.151f, -2.96706f, 1, 14777, false },
};

static const float AV_HOME_GRAVEYARDS[2][4] =
{
	{ 873.0f, -491.0f, 96.75f, 0.0f },
	{ -1437.46f, -610.50f, 51.33f, 0.0f },
};

static const float AV_START_COORDS[2][4] =
{
	{ 878.56f, -489.94f, 96.5f, 3.1f },
	{ -1437.46f, -610.50f, 51.33f, 0.78f },
};

static const float AV_GATE_COORDS[2][3] =
{
	{ 780.487f, -493.024f, 99.9553f },
	{ -1375.19f, -538.981f, 55.2824f },
};

struct AVFireSpawn
{
	float x, y, z, o;
};

static const AVFireSpawn AV_FIRE_SPAWNS[8][4] =
{
	{
		{ 562.632f, -88.1815f, 61.993f, 0.383972f },
		{ 558.097f, -70.9842f, 52.4876f, 0.820305f },
		{ 556.028f, -94.9242f, 44.8191f, 3.05433f },
		{ 572.149f, -93.7862f, 52.5726f, 0.541052f },
	},
	{
		{ 664.797f, -143.65f, 64.1784f, -0.453786f },
		{ 676.067f, -124.319f, 49.6726f, -1.01229f },
		{ 684.423f, -146.582f, 63.6662f, 0.994838f },
		{ 674.576f, -147.101f, 56.5425f, -1.6057f },
	},
	{
		{ 205.782f, -351.335f, 56.8998f, 1.01229f },
		{ 196.605f, -369.187f, 56.3914f, 2.46091f },
		{ 209.647f, -352.632f, 42.3959f, -0.698132f },
		{ 224.682f, -374.031f, 57.0679f, 0.541052f },
	},
	{
		{ -155.488f, -437.356f, 33.2796f, 2.60054f },
		{ -143.977f, -445.148f, 26.4097f, -1.8675f },
		{ -151.638f, -439.521f, 40.3797f, 0.436332f },
		{ -171.291f, -444.684f, 40.9211f, 2.30383f },
	},
	{
		{ -572.667f, -267.923f, 56.8542f, 2.35619f },
		{ -572.538f, -262.649f, 88.6197f, 1.8326f },
		{ -571.476f, -257.234f, 63.3223f, 3.10669f },
		{ -568.318f, -267.1f, 75.0008f, 1.01229f },
	},
	{
		{ -776.072f, -368.046f, 84.3558f, 2.63545f },
		{ -777.564f, -368.521f, 90.6701f, 1.72788f },
		{ -768.763f, -362.735f, 104.612f, 1.81514f },
		{ -773.333f, -364.653f, 90.6803f, 2.82743f },
	},
	{
		{ -1304.87f, -304.525f, 114.146f, 0.279253f },
		{ -1305.58f, -320.625f, 114.15f, 0.034907f },
		{ -1312.41f, -312.999f, 127.518f, 2.86234f },
		{ -1314.7f, -322.131f, 126.527f, 2.56563f },
	},
	{
		{ -1308.24f, -273.26f, 114.664f, 1.90241f },
		{ -1297.28f, -267.773f, 128.429f, 1.8675f },
		{ -1303.41f, -268.237f, 121.33f, 0.767945f },
		{ -1295.55f, -263.865f, 114.659f, 2.6529f },
	},
};

static inline bool AVIsBannerEntry(uint32 entry)
{
	return entry == AV_GO_GRAVE_BANNER_ALLIANCE || entry == AV_GO_GRAVE_BANNER_HORDE || entry == AV_GO_TOWER_BANNER_ALLIANCE || entry == AV_GO_TOWER_BANNER_HORDE ||
		entry == AV_GO_GRAVE_BANNER_ALLIANCE_ASSAULT || entry == AV_GO_GRAVE_BANNER_HORDE_ASSAULT ||
		entry == AV_GO_TOWER_BANNER_ALLIANCE_ASSAULT || entry == AV_GO_TOWER_BANNER_HORDE_ASSAULT ||
		entry == AV_GO_SNOWFALL_BANNER || entry == 178927 || entry == 178932 || entry == 178947 || entry == 178948 ||
		entry == 178955 || entry == 178956 || entry == 178957 || entry == 178958 ||
		entry == 179436 || entry == 179440 || entry == 179442 || entry == 179444 ||
		entry == 179446 || entry == 179450 || entry == 179454 || entry == 179458;
}

static bool AVIsAllianceGuardEntry(uint32 entry)
{
	return entry == 12050 || entry == 13326 || entry == 13331 || entry == 13422;
}

static bool AVIsHordeGuardEntry(uint32 entry)
{
	return entry == 12053 || entry == 13328 || entry == 13332 || entry == 13421;
}

static bool AVIsBaseAllianceGuardEntry(uint32 entry)
{
	return entry == 12050;
}

static bool AVIsBaseHordeGuardEntry(uint32 entry)
{
	return entry == 12053;
}

struct AVGuardSpawnOffset
{
	float x, y, z, o;
};

static const AVGuardSpawnOffset AV_GRAVEYARD_GUARD_OFFSETS[4] =
{
	{ -7.0f, -1.5f, 0.0f, 0.0f },
	{ 6.5f, -1.0f, 0.0f, 3.14159f },
	{ -1.5f, -7.0f, 0.0f, 1.57079f },
	{ 1.5f, 6.5f, 0.0f, 4.71238f },
};

static bool AVIsObjectivePrisonerEntry(uint32 index, uint32 entry)
{
	switch(index)
	{
	case 8:
		return entry == 13181;
	case 9:
		return entry == 13179;
	case 12:
		return entry == 13438;
	case 13:
		return entry == 13437 || entry == 23345;
	case 14:
		return entry == 13439;
	default:
		return false;
	}
}

static int32 AVGetPrisonerOwningTeam(uint32 index)
{
	switch(index)
	{
	case 8:
	case 9:
		return 1;
	case 12:
	case 13:
	case 14:
		return 0;
	default:
		return -1;
	}
}

static inline float AVRotationSin(float o)
{
	return (float)sin(o * 0.5f);
}

static inline float AVRotationCos(float o)
{
	return (float)cos(o * 0.5f);
}

AlteracValley::AlteracValley(MapMgr* mgr, uint32 id, uint32 lgroup, uint32 t) : CBattleground(mgr, id, lgroup, t)
{
	m_playerCountPerTeam = 40;
	Reset();
}

AlteracValley::~AlteracValley()
{
}

void AlteracValley::Reset()
{
	m_reinforcements[0] = AV_MAX_REINFORCEMENTS;
	m_reinforcements[1] = AV_MAX_REINFORCEMENTS;
	m_mineOwner[0] = -1;
	m_mineOwner[1] = -1;
	m_captainDead[0] = false;
	m_captainDead[1] = false;
	m_gates[0] = NULL;
	m_gates[1] = NULL;
	m_lastDeathTime.clear();

	for(uint32 i = 0; i < AV_OBJECTIVE_COUNT; ++i)
	{
		m_objectiveStates[i].owner = AV_OBJECTIVES[i].initialOwner;
		m_objectiveStates[i].assaultingTeam = -1;
		m_objectiveStates[i].timer = 0;
		m_objectiveStates[i].destroyed = false;
		m_objectiveStates[i].nodeState = (AV_OBJECTIVES[i].initialOwner == 0) ? AV_NODE_STATE_ALLIANCE_CONTROLLED :
			(AV_OBJECTIVES[i].initialOwner == 1) ? AV_NODE_STATE_HORDE_CONTROLLED : AV_NODE_STATE_NEUTRAL;
		m_objectiveStates[i].spiritGuide = NULL;
		m_objectiveStates[i].linkedUnit = NULL;
		m_objectiveStates[i].banner = NULL;
		m_objectiveStates[i].bannerState = AV_BANNER_STATE_DESTROYED;
		m_objectiveStates[i].visuals.clear();
		m_objectiveStates[i].allianceGuards.clear();
		m_objectiveStates[i].hordeGuards.clear();
	}
}

void AlteracValley::OnCreate()
{
	Reset();

	SetWorldState(AV_WS_SHOW_ALLIANCE_SCORE, 1);
	SetWorldState(AV_WS_SHOW_HORDE_SCORE, 1);
	SetWorldState(AV_WS_SCOREBOARD_SHOW, 1);
	UpdateReinforcementWorldStates();

	SetWorldState(AV_CONTROLED_IRONDEEP_MINE_TROGG, 0);
	SetWorldState(AV_CONTROLED_COLDTHOOT_MINE_KOBOLT, 0);

	for(uint32 i = 0; i < AV_OBJECTIVE_COUNT; ++i)
	{
		if((AV_OBJECTIVES[i].type == AV_OBJECTIVE_TOWER || AV_OBJECTIVES[i].type == AV_OBJECTIVE_BUNKER) && AV_OBJECTIVES[i].linkedNpcEntry != 0)
		{
			m_objectiveStates[i].linkedUnit = FindLinkedCreature(AV_OBJECTIVES[i].linkedNpcEntry, AV_OBJECTIVES[i].bannerX, AV_OBJECTIVES[i].bannerY, AV_OBJECTIVES[i].bannerZ);
			if(m_objectiveStates[i].linkedUnit != NULL)
				m_objectiveStates[i].linkedUnit->GetAIInterface()->setOutOfCombatRange(90);
		}
	}

	for(uint32 i = 0; i < 2; ++i)
	{
		Creature* spirit = SpawnSpiritGuide(AV_HOME_GRAVEYARDS[i][0], AV_HOME_GRAVEYARDS[i][1], AV_HOME_GRAVEYARDS[i][2], AV_HOME_GRAVEYARDS[i][3], i);
		if(spirit != NULL)
			AddSpiritGuide(spirit);

		m_gates[i] = FindGate(i);
	}

	InitializeAlteracValleyNodes();
	UpdateBossRoomGuards();

	sEventMgr.AddEvent(this, &AlteracValley::EventUpdateObjectives, EVENT_AV_OBJECTIVES_UPDATE, 1000, 0, EVENT_FLAG_DO_NOT_EXECUTE_IN_WORLD_CONTEXT);
}

void AlteracValley::OnStart()
{
	InitializeAlteracValleyNodes();

	for(uint32 i = 0; i < 2; ++i)
	{
		for(set<Player*>::iterator itr = m_players[i].begin(); itr != m_players[i].end(); ++itr)
			(*itr)->RemoveAura(BG_PREPARATION);
	}

	m_started = true;
	PlaySoundToAll(SOUND_BATTLEGROUND_BEGIN);

	for(uint32 i = 0; i < 2; ++i)
		SetGateOpen(i, true);

	sEventMgr.AddEvent(this, &AlteracValley::EventMineTick, EVENT_AV_MINE_TICK, AV_MINE_TICK_MS, 0, EVENT_FLAG_DO_NOT_EXECUTE_IN_WORLD_CONTEXT);
}

void AlteracValley::OnClose()
{
	for(uint32 i = 0; i < AV_OBJECTIVE_COUNT; ++i)
	{
		if(m_objectiveStates[i].spiritGuide != NULL)
		{
			RemoveSpiritGuide(m_objectiveStates[i].spiritGuide);
			m_objectiveStates[i].spiritGuide->Despawn(0, 0);
			m_objectiveStates[i].spiritGuide = NULL;
		}

		ClearObjectiveBanner(i);
		m_objectiveStates[i].linkedUnit = NULL;

		for(vector<GameObject*>::iterator itr = m_objectiveStates[i].visuals.begin(); itr != m_objectiveStates[i].visuals.end(); ++itr)
		{
			GameObject* go = *itr;
			if(go == NULL)
				continue;

			if(go->IsInWorld())
				go->RemoveFromWorld(true);

			delete go;
		}
		m_objectiveStates[i].visuals.clear();

		for(vector<Creature*>::iterator itr = m_objectiveStates[i].allianceGuards.begin(); itr != m_objectiveStates[i].allianceGuards.end(); ++itr)
		{
			Creature* creature = *itr;
			if(creature == NULL)
				continue;

			if(creature->IsInWorld())
				creature->RemoveFromWorld(false, false);

			delete creature;
		}
		m_objectiveStates[i].allianceGuards.clear();

		for(vector<Creature*>::iterator itr = m_objectiveStates[i].hordeGuards.begin(); itr != m_objectiveStates[i].hordeGuards.end(); ++itr)
		{
			Creature* creature = *itr;
			if(creature == NULL)
				continue;

			if(creature->IsInWorld())
				creature->RemoveFromWorld(false, false);

			delete creature;
		}
		m_objectiveStates[i].hordeGuards.clear();
	}

	for(uint32 i = 0; i < 2; ++i)
		m_gates[i] = NULL;
}

void AlteracValley::OnAddPlayer(Player* plr)
{
	if(!m_started)
	{
		plr->CastSpell(plr, BG_PREPARATION, true);
	}
}

void AlteracValley::OnRemovePlayer(Player* plr)
{
	plr->RemoveAura(BG_PREPARATION);
}

LocationVector AlteracValley::GetStartingCoords(uint32 Team)
{
	if(Team == 0)
		return LocationVector(AV_START_COORDS[0][0], AV_START_COORDS[0][1], AV_START_COORDS[0][2], AV_START_COORDS[0][3]);

	return LocationVector(AV_START_COORDS[1][0], AV_START_COORDS[1][1], AV_START_COORDS[1][2], AV_START_COORDS[1][3]);
}

void AlteracValley::HookOnPlayerDeath(Player* plr)
{
	const uint32 now = getMSTime();
	const uint32 guid = plr->GetLowGUID();
	map<uint32, uint32>::iterator itr = m_lastDeathTime.find(guid);
	if(itr != m_lastDeathTime.end() && (now - itr->second) < 1500)
		return;

	m_lastDeathTime[guid] = now;
	plr->m_bgScore.Deaths++;
	ModifyReinforcements(plr->m_bgTeam, -1);
	UpdatePvPData();
}

void AlteracValley::HookOnPlayerKill(Player* plr, Unit* pVictim)
{
	if(pVictim == NULL)
		return;

	if(pVictim->IsPlayer())
	{
		plr->m_bgScore.KillingBlows++;
		UpdatePvPData();
		return;
	}

	if(!pVictim->IsCreature())
		return;

	uint32 entry = pVictim->GetEntry();
	if(entry == AV_NPC_VANNDAR)
		EndBattleground(1);
	else if(entry == AV_NPC_DREKTHAR)
		EndBattleground(0);
	else if(entry == AV_NPC_BALINDA && !m_captainDead[0])
	{
		m_captainDead[0] = true;
		ModifyReinforcements(0, -AV_REINFORCEMENT_CAPTAIN_LOSS);
		SendChatMessage(CHAT_MSG_BG_EVENT_HORDE, plr->GetGUID(), "$N has slain Captain Balinda Stonehearth!");
	}
	else if(entry == AV_NPC_GALVANGAR && !m_captainDead[1])
	{
		m_captainDead[1] = true;
		ModifyReinforcements(1, -AV_REINFORCEMENT_CAPTAIN_LOSS);
		SendChatMessage(CHAT_MSG_BG_EVENT_ALLIANCE, plr->GetGUID(), "$N has slain Captain Galvangar!");
	}
}

void AlteracValley::HookOnHK(Player* plr)
{
	plr->m_bgScore.HonorableKills++;
	UpdatePvPData();
}

void AlteracValley::HookFlagDrop(Player* plr, GameObject* obj)
{
}

void AlteracValley::HookFlagStand(Player* plr, GameObject* obj)
{
}

void AlteracValley::HookOnMount(Player* plr)
{
}

bool AlteracValley::HookHandleRepop(Player* plr)
{
	LocationVector destination(AV_HOME_GRAVEYARDS[plr->m_bgTeam][0], AV_HOME_GRAVEYARDS[plr->m_bgTeam][1], AV_HOME_GRAVEYARDS[plr->m_bgTeam][2], AV_HOME_GRAVEYARDS[plr->m_bgTeam][3]);
	float nearest = 999999999.0f;

	for(uint32 i = 0; i < AV_OBJECTIVE_COUNT; ++i)
	{
		if(AV_OBJECTIVES[i].type != AV_OBJECTIVE_GRAVEYARD)
			continue;

		if(m_objectiveStates[i].destroyed || m_objectiveStates[i].owner != (int32)plr->m_bgTeam)
			continue;

		float dist = plr->GetPositionV()->Distance2DSq(AV_OBJECTIVES[i].spiritX, AV_OBJECTIVES[i].spiritY);
		if(dist < nearest)
		{
			nearest = dist;
			destination.ChangeCoords(AV_OBJECTIVES[i].spiritX, AV_OBJECTIVES[i].spiritY, AV_OBJECTIVES[i].spiritZ, AV_OBJECTIVES[i].spiritO);
		}
	}

	plr->SafeTeleport(plr->GetMapId(), plr->GetInstanceID(), destination);
	return true;
}

void AlteracValley::HookOnAreaTrigger(Player* plr, uint32 id)
{
	switch(id)
	{
	case AV_AREATRIGGER_IRONDEEP:
		if(m_mineOwner[0] != (int32)plr->m_bgTeam)
		{
			m_mineOwner[0] = plr->m_bgTeam;
			SetWorldState(AV_CONTROLED_IRONDEEP_MINE_TROGG, (plr->m_bgTeam == 0) ? 0 : 1);
			SendChatMessage(plr->m_bgTeam ? CHAT_MSG_BG_EVENT_HORDE : CHAT_MSG_BG_EVENT_ALLIANCE, plr->GetGUID(), "$N has taken Irondeep Mine!");
		}
		break;

	case AV_AREATRIGGER_COLDTOOTH:
		if(m_mineOwner[1] != (int32)plr->m_bgTeam)
		{
			m_mineOwner[1] = plr->m_bgTeam;
			SetWorldState(AV_CONTROLED_COLDTHOOT_MINE_KOBOLT, (plr->m_bgTeam == 0) ? 0 : 1);
			SendChatMessage(plr->m_bgTeam ? CHAT_MSG_BG_EVENT_HORDE : CHAT_MSG_BG_EVENT_ALLIANCE, plr->GetGUID(), "$N has taken Coldtooth Mine!");
		}
		break;

	default:
		break;
	}
}

bool AlteracValley::HookSlowLockOpen(GameObject* pGo, Player* pPlayer, Spell* pSpell)
{
	if(pGo == NULL || pPlayer == NULL)
		return false;

	int32 objective = GetObjectiveFromBanner(pGo);
	if(objective < 0)
		return false;

	AVObjectiveState& state = m_objectiveStates[objective];
	if(state.destroyed)
		return false;

	if(state.assaultingTeam == -1 && state.owner == (int32)pPlayer->m_bgTeam)
		return false;

	if(state.assaultingTeam == (int32)pPlayer->m_bgTeam)
		return false;

	AssaultObjective(pPlayer, (uint32)objective);
	return true;
}

void AlteracValley::AssaultObjective(Player* pPlayer, uint32 index)
{
	if(index >= AV_OBJECTIVE_COUNT || m_ended || !m_started)
		return;

	AVObjectiveState& state = m_objectiveStates[index];
	const int32 team = pPlayer->m_bgTeam;

	if(state.destroyed)
		return;

	if(state.owner == team && state.assaultingTeam == -1)
		return;

	if(state.owner == team && state.assaultingTeam != -1)
	{
		state.assaultingTeam = -1;
		state.timer = 0;
		UpdateObjectiveNodeState(index);
		UpdateObjectiveWorldStates(index);
		UpdateObjectiveBanner(index);
		UpdateObjectiveGuards(index);
		PlaySoundToAll(team ? SOUND_HORDE_CAPTURE : SOUND_ALLIANCE_CAPTURE);
		SendChatMessage(team ? CHAT_MSG_BG_EVENT_HORDE : CHAT_MSG_BG_EVENT_ALLIANCE, pPlayer->GetGUID(), "$N has defended the %s!", AV_OBJECTIVES[index].name);
		return;
	}

	state.assaultingTeam = team;
	state.timer = AV_BURN_TIMER_MS;

	UpdateObjectiveNodeState(index);
	UpdateObjectiveWorldStates(index);
	UpdateObjectiveBanner(index);
	UpdateObjectiveGuards(index);

	PlaySoundToAll(team ? SOUND_HORDE_CAPTURE : SOUND_ALLIANCE_CAPTURE);
	SendChatMessage(team ? CHAT_MSG_BG_EVENT_HORDE : CHAT_MSG_BG_EVENT_ALLIANCE, pPlayer->GetGUID(), "$N assaults the %s!", AV_OBJECTIVES[index].name);
}

void AlteracValley::EventUpdateObjectives()
{
	if(m_ended)
		return;

	if(!m_started)
		return;

	for(uint32 i = 0; i < AV_OBJECTIVE_COUNT; ++i)
	{
		if(m_objectiveStates[i].assaultingTeam == -1 || m_objectiveStates[i].destroyed)
			continue;

		if(m_objectiveStates[i].timer > 1000)
		{
			m_objectiveStates[i].timer -= 1000;
			continue;
		}

		FinalizeObjective(i);
	}
}

void AlteracValley::FinalizeObjective(uint32 index)
{
	if(index >= AV_OBJECTIVE_COUNT)
		return;

	AVObjectiveState& state = m_objectiveStates[index];
	if(state.assaultingTeam == -1)
		return;

	const int32 newOwner = state.assaultingTeam;
	state.assaultingTeam = -1;
	state.timer = 0;

	if(AV_OBJECTIVES[index].type == AV_OBJECTIVE_TOWER || AV_OBJECTIVES[index].type == AV_OBJECTIVE_BUNKER)
	{
		state.destroyed = true;
		state.owner = newOwner;

		if(AV_OBJECTIVES[index].initialOwner >= 0)
			ModifyReinforcements((uint32)AV_OBJECTIVES[index].initialOwner, -AV_REINFORCEMENT_TOWER_LOSS);

		if(state.linkedUnit != NULL)
			state.linkedUnit->Despawn(0, 0);

		UpdateBossRoomGuards();
		SetObjectiveVisualsActive(index, true);

		PlaySoundToAll(newOwner ? SOUND_HORDE_CAPTURE : SOUND_ALLIANCE_CAPTURE);
		SendChatMessage(newOwner ? CHAT_MSG_BG_EVENT_HORDE : CHAT_MSG_BG_EVENT_ALLIANCE, 0, "The %s has been destroyed!", AV_OBJECTIVES[index].name);
	}
	else
	{
		state.owner = newOwner;

		PlaySoundToAll(newOwner ? SOUND_HORDE_CAPTURE : SOUND_ALLIANCE_CAPTURE);
		SendChatMessage(newOwner ? CHAT_MSG_BG_EVENT_HORDE : CHAT_MSG_BG_EVENT_ALLIANCE, 0, "The %s is taken by the %s!", AV_OBJECTIVES[index].name, newOwner ? "Horde" : "Alliance");
	}

	UpdateObjectiveNodeState(index);
	UpdateObjectiveWorldStates(index);
	UpdateObjectiveSpiritGuide(index);
	UpdateObjectiveBanner(index);
	UpdateObjectiveGuards(index);
}

void AlteracValley::InitializeAlteracValleyNodes()
{
	for(uint32 i = 0; i < AV_OBJECTIVE_COUNT; ++i)
	{
		UpdateObjectiveNodeState(i);
		CleanupObjectiveBannerObjects(i);
		CleanupObjectiveDbFireVisuals(i);
		RefreshObjectiveVisuals(i);
		SetObjectiveVisualsActive(i, m_objectiveStates[i].nodeState == AV_NODE_STATE_DESTROYED);
		UpdateObjectiveSpiritGuide(i);
		UpdateObjectiveBanner(i);
		RefreshObjectiveGuards(i);
		UpdateObjectiveGuards(i);
		UpdateObjectivePrisoners(i);
		UpdateObjectiveWorldStates(i);
	}

}

void AlteracValley::UpdateObjectiveNodeState(uint32 index)
{
	if(index >= AV_OBJECTIVE_COUNT)
		return;

	AVObjectiveState& state = m_objectiveStates[index];
	if(state.destroyed)
	{
		state.nodeState = AV_NODE_STATE_DESTROYED;
		return;
	}

	if(state.assaultingTeam == 0)
		state.nodeState = AV_NODE_STATE_ALLIANCE_CONTESTED;
	else if(state.assaultingTeam == 1)
		state.nodeState = AV_NODE_STATE_HORDE_CONTESTED;
	else if(state.owner == 0)
		state.nodeState = AV_NODE_STATE_ALLIANCE_CONTROLLED;
	else if(state.owner == 1)
		state.nodeState = AV_NODE_STATE_HORDE_CONTROLLED;
	else
		state.nodeState = AV_NODE_STATE_NEUTRAL;
}

void AlteracValley::EventMineTick()
{
	if(!m_started || m_ended)
		return;

	for(uint32 i = 0; i < 2; ++i)
	{
		if(m_mineOwner[i] >= 0)
			ModifyReinforcements((uint32)m_mineOwner[i], 1);
	}
}

void AlteracValley::UpdateObjectiveWorldStates(uint32 index)
{
	if(index >= AV_OBJECTIVE_COUNT)
		return;

	const AVObjectiveState& state = m_objectiveStates[index];
	const bool allianceControlled = (state.owner == 0 && state.assaultingTeam == -1 && !state.destroyed);
	const bool hordeControlled = (state.owner == 1 && state.assaultingTeam == -1 && !state.destroyed);
	const bool allianceAssaulting = (state.assaultingTeam == 0 && !state.destroyed);
	const bool hordeAssaulting = (state.assaultingTeam == 1 && !state.destroyed);
	const bool destroyed = state.destroyed;

	if(AV_OBJECTIVES[index].worldStateAlliance)
		SetWorldState(AV_OBJECTIVES[index].worldStateAlliance, allianceControlled ? 1 : 0);

	if(AV_OBJECTIVES[index].worldStateHorde)
		SetWorldState(AV_OBJECTIVES[index].worldStateHorde, hordeControlled ? 1 : 0);

	if(AV_OBJECTIVES[index].worldStateAllianceAssault)
		SetWorldState(AV_OBJECTIVES[index].worldStateAllianceAssault, allianceAssaulting ? 1 : 0);

	if(AV_OBJECTIVES[index].worldStateHordeAssault)
		SetWorldState(AV_OBJECTIVES[index].worldStateHordeAssault, hordeAssaulting ? 1 : 0);

	if(AV_OBJECTIVES[index].worldStateDestroyed)
		SetWorldState(AV_OBJECTIVES[index].worldStateDestroyed, destroyed ? 1 : 0);

	if(AV_OBJECTIVES[index].type == AV_OBJECTIVE_GRAVEYARD && AV_OBJECTIVES[index].worldStateAlliance == AV_UNCONTROLED_SNOWFALL_GRAVE)
		SetWorldState(AV_UNCONTROLED_SNOWFALL_GRAVE, (state.owner < 0 && state.assaultingTeam == -1) ? 1 : 0);
}

void AlteracValley::ModifyReinforcements(uint32 team, int32 delta)
{
	if(team > 1 || m_ended)
		return;

	m_reinforcements[team] += delta;
	if(m_reinforcements[team] < 0)
		m_reinforcements[team] = 0;
	if(m_reinforcements[team] > AV_MAX_REINFORCEMENTS)
		m_reinforcements[team] = AV_MAX_REINFORCEMENTS;

	UpdateReinforcementWorldStates();
	CheckForEnd();
}

void AlteracValley::UpdateReinforcementWorldStates()
{
	SetWorldState(AV_WS_ALLIANCE_SCORE, (uint32)m_reinforcements[0]);
	SetWorldState(AV_WS_HORDE_SCORE, (uint32)m_reinforcements[1]);
}

void AlteracValley::CheckForEnd()
{
	if(m_reinforcements[0] == 0)
		EndBattleground(1);
	else if(m_reinforcements[1] == 0)
		EndBattleground(0);
}

void AlteracValley::UpdateBossRoomGuards()
{
	uint32 destroyedAlliance = 0;
	uint32 destroyedHorde = 0;

	for(uint32 i = 0; i < AV_OBJECTIVE_COUNT; ++i)
	{
		if(!(AV_OBJECTIVES[i].type == AV_OBJECTIVE_TOWER || AV_OBJECTIVES[i].type == AV_OBJECTIVE_BUNKER))
			continue;

		if(!m_objectiveStates[i].destroyed)
			continue;

		if(AV_OBJECTIVES[i].initialOwner == 0)
			++destroyedAlliance;
		else if(AV_OBJECTIVES[i].initialOwner == 1)
			++destroyedHorde;
	}

	Creature* vanndar = FindLinkedCreature(AV_NPC_VANNDAR, 726.0f, -10.0f, 50.0f);
	Creature* drekthar = FindLinkedCreature(AV_NPC_DREKTHAR, -1377.0f, -229.0f, 98.0f);
	if(vanndar != NULL)
		vanndar->GetAIInterface()->setOutOfCombatRange(30 + (destroyedAlliance * 5));
	if(drekthar != NULL)
		drekthar->GetAIInterface()->setOutOfCombatRange(30 + (destroyedHorde * 5));
}

void AlteracValley::RepopPlayersOfTeam(int32 team, Creature* spiritGuide)
{
	if(spiritGuide == NULL)
		return;

	map<Creature*, set<uint32> >::iterator itr = m_resurrectMap.find(spiritGuide);
	if(itr == m_resurrectMap.end())
		return;

	for(set<uint32>::iterator it2 = itr->second.begin(); it2 != itr->second.end(); ++it2)
	{
		Player* rplr = m_mapMgr->GetPlayer(*it2);
		if(rplr != NULL && rplr->isDead() && (team < 0 || team == (int32)rplr->m_bgTeam))
			HookHandleRepop(rplr);
	}
}

Creature* AlteracValley::FindLinkedCreature(uint32 entry, float x, float y, float z)
{
	return m_mapMgr->GetInterface()->GetCreatureNearestCoords(x, y, z, entry);
}

GameObject* AlteracValley::FindGate(uint32 team)
{
	if(team > 1 || m_mapMgr == NULL)
		return NULL;

	return m_mapMgr->GetInterface()->GetGameObjectNearestCoords(AV_GATE_COORDS[team][0], AV_GATE_COORDS[team][1], AV_GATE_COORDS[team][2], AV_GO_START_GATE);
}

void AlteracValley::SetGateOpen(uint32 team, bool open)
{
	if(team > 1)
		return;

	GameObject* gate = m_gates[team];
	if(gate == NULL)
		gate = FindGate(team);
	if(gate == NULL)
		return;

	m_gates[team] = gate;
	gate->SetUInt32Value(GAMEOBJECT_FLAGS, open ? 64 : 32);
	gate->SetUInt32Value(GAMEOBJECT_STATE, open ? 0 : 4);
}

GameObject* AlteracValley::SpawnObjectiveBanner(uint32 index, AVBannerState state)
{
	if(index >= AV_OBJECTIVE_COUNT || state == AV_BANNER_STATE_DESTROYED)
		return NULL;

	uint32 entry = AV_OBJECTIVES[index].neutralGoEntry ? AV_OBJECTIVES[index].neutralGoEntry : AV_GO_GRAVE_BANNER_ALLIANCE_ASSAULT;
	uint32 faction = AV_FACTION_NEUTRAL;

	switch(state)
	{
	case AV_BANNER_STATE_ALLIANCE_CONTROLLED:
		entry = AV_OBJECTIVES[index].controlledGoEntryAlliance;
		faction = AV_FACTION_ALLIANCE;
		break;

	case AV_BANNER_STATE_HORDE_CONTROLLED:
		entry = AV_OBJECTIVES[index].controlledGoEntryHorde;
		faction = AV_FACTION_HORDE;
		break;

	case AV_BANNER_STATE_ALLIANCE_ASSAULTING:
		entry = AV_OBJECTIVES[index].contestedGoEntryAlliance;
		faction = (m_objectiveStates[index].owner == 1) ? AV_FACTION_HORDE : AV_FACTION_ALLIANCE;
		break;

	case AV_BANNER_STATE_HORDE_ASSAULTING:
		entry = AV_OBJECTIVES[index].contestedGoEntryHorde;
		faction = (m_objectiveStates[index].owner == 0) ? AV_FACTION_ALLIANCE : AV_FACTION_HORDE;
		break;

	case AV_BANNER_STATE_NEUTRAL:
	default:
		entry = AV_OBJECTIVES[index].neutralGoEntry ? AV_OBJECTIVES[index].neutralGoEntry : AV_OBJECTIVES[index].contestedGoEntryAlliance;
		faction = AV_FACTION_NEUTRAL;
		break;
	}

	GameObjectInfo* info = GameObjectNameStorage.LookupEntry(entry);
	if(info == NULL)
		return NULL;

	GameObject* banner = SpawnGameObject(entry, m_mapMgr->GetMapId(), AV_OBJECTIVES[index].bannerX, AV_OBJECTIVES[index].bannerY, AV_OBJECTIVES[index].bannerZ, AV_OBJECTIVES[index].bannerO, 0, faction, 1.0f);
	if(banner == NULL)
		return NULL;

	banner->SetUInt32Value(GAMEOBJECT_DISPLAYID, info->DisplayID);
	banner->SetUInt32Value(GAMEOBJECT_TYPE_ID, info->Type);
	banner->SetUInt32Value(GAMEOBJECT_FACTION, faction);
	banner->SetUInt32Value(GAMEOBJECT_STATE, 1);
	banner->SetUInt32Value(GAMEOBJECT_ANIMPROGRESS, 100);
	banner->SetUInt32Value(GAMEOBJECT_DYN_FLAGS, 1);
	banner->SetFloatValue(GAMEOBJECT_ROTATION_02, AVRotationSin(AV_OBJECTIVES[index].bannerO));
	banner->SetFloatValue(GAMEOBJECT_ROTATION_03, AVRotationCos(AV_OBJECTIVES[index].bannerO));
	banner->bannerslot = (int8)index;
	banner->PushToWorld(m_mapMgr);
	return banner;
}

void AlteracValley::ClearObjectiveBanner(uint32 index)
{
	if(index >= AV_OBJECTIVE_COUNT || m_objectiveStates[index].banner == NULL)
		return;

	GameObject* banner = m_objectiveStates[index].banner;
	m_objectiveStates[index].banner = NULL;
	m_objectiveStates[index].bannerState = AV_BANNER_STATE_DESTROYED;

	if(banner->IsInWorld())
		banner->RemoveFromWorld(true);

	delete banner;
}

void AlteracValley::UpdateObjectiveBanner(uint32 index)
{
	if(index >= AV_OBJECTIVE_COUNT)
		return;

	AVBannerState bannerState = AV_BANNER_STATE_DESTROYED;
	AVObjectiveState& state = m_objectiveStates[index];

	if(!state.destroyed)
	{
		if(state.assaultingTeam == 0)
			bannerState = AV_BANNER_STATE_ALLIANCE_ASSAULTING;
		else if(state.assaultingTeam == 1)
			bannerState = AV_BANNER_STATE_HORDE_ASSAULTING;
		else if(state.owner == 0)
			bannerState = AV_BANNER_STATE_ALLIANCE_CONTROLLED;
		else if(state.owner == 1)
			bannerState = AV_BANNER_STATE_HORDE_CONTROLLED;
		else
			bannerState = AV_BANNER_STATE_NEUTRAL;
	}

	if(state.banner != NULL && state.bannerState == bannerState)
		return;

	if(bannerState == AV_BANNER_STATE_DESTROYED)
	{
		ClearObjectiveBanner(index);
		return;
	}

	uint32 entry = AV_OBJECTIVES[index].neutralGoEntry ? AV_OBJECTIVES[index].neutralGoEntry : AV_GO_GRAVE_BANNER_ALLIANCE_ASSAULT;
	uint32 faction = AV_FACTION_NEUTRAL;

	switch(bannerState)
	{
	case AV_BANNER_STATE_ALLIANCE_CONTROLLED:
		entry = AV_OBJECTIVES[index].controlledGoEntryAlliance;
		faction = AV_FACTION_ALLIANCE;
		break;

	case AV_BANNER_STATE_HORDE_CONTROLLED:
		entry = AV_OBJECTIVES[index].controlledGoEntryHorde;
		faction = AV_FACTION_HORDE;
		break;

	case AV_BANNER_STATE_ALLIANCE_ASSAULTING:
		entry = AV_OBJECTIVES[index].contestedGoEntryAlliance;
		faction = (state.owner == 1) ? AV_FACTION_HORDE : AV_FACTION_ALLIANCE;
		break;

	case AV_BANNER_STATE_HORDE_ASSAULTING:
		entry = AV_OBJECTIVES[index].contestedGoEntryHorde;
		faction = (state.owner == 0) ? AV_FACTION_ALLIANCE : AV_FACTION_HORDE;
		break;

	case AV_BANNER_STATE_NEUTRAL:
	default:
		entry = AV_OBJECTIVES[index].neutralGoEntry ? AV_OBJECTIVES[index].neutralGoEntry : AV_OBJECTIVES[index].contestedGoEntryAlliance;
		break;
	}

	GameObjectInfo* info = GameObjectNameStorage.LookupEntry(entry);
	if(info == NULL)
		return;

	if(state.banner == NULL)
	{
		state.banner = SpawnObjectiveBanner(index, bannerState);
		state.bannerState = bannerState;
		return;
	}

	if(state.banner->IsInWorld())
		state.banner->RemoveFromWorld(false);

	state.banner->SetNewGuid(m_mapMgr->GenerateGameobjectGuid());
	state.banner->SetUInt32Value(OBJECT_FIELD_ENTRY, entry);
	state.banner->SetUInt32Value(GAMEOBJECT_DISPLAYID, info->DisplayID);
	state.banner->SetUInt32Value(GAMEOBJECT_TYPE_ID, info->Type);
	state.banner->SetUInt32Value(GAMEOBJECT_FACTION, faction);
	state.banner->SetUInt32Value(GAMEOBJECT_STATE, 1);
	state.banner->SetUInt32Value(GAMEOBJECT_ANIMPROGRESS, 100);
	state.banner->SetUInt32Value(GAMEOBJECT_DYN_FLAGS, 1);
	state.banner->SetFloatValue(GAMEOBJECT_ROTATION_02, AVRotationSin(AV_OBJECTIVES[index].bannerO));
	state.banner->SetFloatValue(GAMEOBJECT_ROTATION_03, AVRotationCos(AV_OBJECTIVES[index].bannerO));
	state.banner->SetInfo(info);
	state.banner->PushToWorld(m_mapMgr);
	state.bannerState = bannerState;
}

void AlteracValley::CleanupObjectiveBannerObjects(uint32 index)
{
	if(index >= AV_OBJECTIVE_COUNT || m_mapMgr == NULL)
		return;

	vector<GameObject*> bannersToHide;

	for(GameObjectSqlIdMap::iterator itr = m_mapMgr->_sqlids_gameobjects.begin(); itr != m_mapMgr->_sqlids_gameobjects.end(); ++itr)
	{
		GameObject* go = itr->second;
		if(go == NULL || go == m_objectiveStates[index].banner || !AVIsBannerEntry(go->GetEntry()))
			continue;

		const float dx = go->GetPositionX() - AV_OBJECTIVES[index].bannerX;
		const float dy = go->GetPositionY() - AV_OBJECTIVES[index].bannerY;
		const float dz = go->GetPositionZ() - AV_OBJECTIVES[index].bannerZ;
		if(((dx * dx) + (dy * dy) + (dz * dz)) > (12.0f * 12.0f))
			continue;

		bannersToHide.push_back(go);
	}

	for(vector<GameObject*>::iterator itr = bannersToHide.begin(); itr != bannersToHide.end(); ++itr)
	{
		GameObject* go = *itr;
		if(go != NULL && go->IsInWorld())
			go->RemoveFromWorld(false);
	}
}

void AlteracValley::UpdateObjectiveSpiritGuide(uint32 index)
{
	if(index >= AV_OBJECTIVE_COUNT || AV_OBJECTIVES[index].type != AV_OBJECTIVE_GRAVEYARD)
		return;

	AVObjectiveState& state = m_objectiveStates[index];
	const bool shouldHaveSpirit = (state.nodeState == AV_NODE_STATE_ALLIANCE_CONTROLLED || state.nodeState == AV_NODE_STATE_HORDE_CONTROLLED);
	const uint32 owner = (state.nodeState == AV_NODE_STATE_HORDE_CONTROLLED) ? 1 : 0;

	if(!shouldHaveSpirit)
	{
		if(state.spiritGuide != NULL)
		{
			RepopPlayersOfTeam(-1, state.spiritGuide);
			RemoveSpiritGuide(state.spiritGuide);
			state.spiritGuide->Despawn(0, 0);
			state.spiritGuide = NULL;
		}
		return;
	}

	if(state.spiritGuide != NULL && state.spiritGuide->GetEntry() == (13116 + owner))
		return;

	if(state.spiritGuide != NULL)
	{
		RepopPlayersOfTeam(-1, state.spiritGuide);
		RemoveSpiritGuide(state.spiritGuide);
		state.spiritGuide->Despawn(0, 0);
		state.spiritGuide = NULL;
	}

	state.spiritGuide = SpawnSpiritGuide(AV_OBJECTIVES[index].spiritX, AV_OBJECTIVES[index].spiritY, AV_OBJECTIVES[index].spiritZ, AV_OBJECTIVES[index].spiritO, owner);
	if(state.spiritGuide != NULL)
		AddSpiritGuide(state.spiritGuide);
}

int32 AlteracValley::GetObjectiveFromBanner(GameObject* pGo)
{
	if(pGo == NULL)
		return -1;

	if(pGo->bannerslot >= 0 && pGo->bannerslot < (int32)AV_OBJECTIVE_COUNT)
		return pGo->bannerslot;

	if(!AVIsBannerEntry(pGo->GetEntry()))
		return -1;

	float bestDistance = 999999.0f;
	int32 bestIndex = -1;

	for(uint32 i = 0; i < AV_OBJECTIVE_COUNT; ++i)
	{
		float dx = pGo->GetPositionX() - AV_OBJECTIVES[i].bannerX;
		float dy = pGo->GetPositionY() - AV_OBJECTIVES[i].bannerY;
		float dist = (dx * dx) + (dy * dy);
		if(dist < bestDistance)
		{
			bestDistance = dist;
			bestIndex = (int32)i;
		}
	}

	return (bestDistance <= (40.0f * 40.0f)) ? bestIndex : -1;
}

void AlteracValley::RefreshObjectiveVisuals(uint32 index)
{
	if(index >= AV_OBJECTIVE_COUNT)
		return;

	AVObjectiveState& state = m_objectiveStates[index];

	if(!(AV_OBJECTIVES[index].type == AV_OBJECTIVE_TOWER || AV_OBJECTIVES[index].type == AV_OBJECTIVE_BUNKER))
		return;

	if(!state.visuals.empty())
		return;

	uint32 visualSet = 0;
	if(index >= 7)
		visualSet = index - 7;

	for(uint32 i = 0; i < 4; ++i)
	{
		GameObject* go = SpawnGameObject(AV_GO_TOWER_BURNING, m_mapMgr->GetMapId(), AV_FIRE_SPAWNS[visualSet][i].x, AV_FIRE_SPAWNS[visualSet][i].y, AV_FIRE_SPAWNS[visualSet][i].z, AV_FIRE_SPAWNS[visualSet][i].o, 32, AV_FACTION_NEUTRAL, 1.5f);
		if(go == NULL)
			continue;

		go->SetUInt32Value(GAMEOBJECT_STATE, 1);
		state.visuals.push_back(go);

		if(state.nodeState == AV_NODE_STATE_DESTROYED)
			go->PushToWorld(m_mapMgr);
	}
}

void AlteracValley::SetObjectiveVisualsActive(uint32 index, bool active)
{
	if(index >= AV_OBJECTIVE_COUNT)
		return;

	AVObjectiveState& state = m_objectiveStates[index];
	for(vector<GameObject*>::iterator itr = state.visuals.begin(); itr != state.visuals.end(); ++itr)
	{
		GameObject* go = (*itr);
		if(go == NULL)
			continue;

		if(active)
		{
			if(!go->IsInWorld())
				go->PushToWorld(m_mapMgr);
		}
		else if(go->IsInWorld())
			go->RemoveFromWorld(false);
	}
}

void AlteracValley::CleanupObjectiveDbFireVisuals(uint32 index)
{
	if(index >= AV_OBJECTIVE_COUNT || m_mapMgr == NULL)
		return;

	if(!(AV_OBJECTIVES[index].type == AV_OBJECTIVE_TOWER || AV_OBJECTIVES[index].type == AV_OBJECTIVE_BUNKER))
		return;

	vector<GameObject*> visualsToHide;

	for(GameObjectSqlIdMap::iterator itr = m_mapMgr->_sqlids_gameobjects.begin(); itr != m_mapMgr->_sqlids_gameobjects.end(); ++itr)
	{
		GameObject* go = itr->second;
		if(go == NULL || go->GetEntry() != AV_GO_TOWER_BURNING)
			continue;

		const float dx = go->GetPositionX() - AV_OBJECTIVES[index].bannerX;
		const float dy = go->GetPositionY() - AV_OBJECTIVES[index].bannerY;
		const float dz = go->GetPositionZ() - AV_OBJECTIVES[index].bannerZ;
		if(((dx * dx) + (dy * dy) + (dz * dz)) > (60.0f * 60.0f))
			continue;

		visualsToHide.push_back(go);
	}

	for(vector<GameObject*>::iterator itr = visualsToHide.begin(); itr != visualsToHide.end(); ++itr)
	{
		GameObject* go = *itr;
		if(go != NULL && go->IsInWorld())
			go->RemoveFromWorld(false);
	}
}

void AlteracValley::RefreshObjectiveGuards(uint32 index)
{
	if(index >= AV_OBJECTIVE_COUNT || !AV_OBJECTIVES[index].spawnGuards || m_mapMgr == NULL)
		return;

	AVObjectiveState& state = m_objectiveStates[index];
	if(state.nodeState == AV_NODE_STATE_ALLIANCE_CONTROLLED && state.allianceGuards.empty())
	{
		for(uint32 i = 0; i < 4; ++i)
		{
			const float guardX = AV_OBJECTIVES[index].bannerX + AV_GRAVEYARD_GUARD_OFFSETS[i].x;
			const float guardY = AV_OBJECTIVES[index].bannerY + AV_GRAVEYARD_GUARD_OFFSETS[i].y;
			float guardZ = m_mapMgr->GetLandHeight(guardX, guardY);
			if(guardZ == 0.0f)
				guardZ = AV_OBJECTIVES[index].bannerZ + AV_GRAVEYARD_GUARD_OFFSETS[i].z;

			Creature* guard = SpawnObjectiveGuard(12050,
				guardX,
				guardY,
				guardZ,
				AV_GRAVEYARD_GUARD_OFFSETS[i].o);
			if(guard != NULL)
				state.allianceGuards.push_back(guard);
		}
	}
	else if(state.nodeState == AV_NODE_STATE_HORDE_CONTROLLED && state.hordeGuards.empty())
	{
		for(uint32 i = 0; i < 4; ++i)
		{
			const float guardX = AV_OBJECTIVES[index].bannerX + AV_GRAVEYARD_GUARD_OFFSETS[i].x;
			const float guardY = AV_OBJECTIVES[index].bannerY + AV_GRAVEYARD_GUARD_OFFSETS[i].y;
			float guardZ = m_mapMgr->GetLandHeight(guardX, guardY);
			if(guardZ == 0.0f)
				guardZ = AV_OBJECTIVES[index].bannerZ + AV_GRAVEYARD_GUARD_OFFSETS[i].z;

			Creature* guard = SpawnObjectiveGuard(12053,
				guardX,
				guardY,
				guardZ,
				AV_GRAVEYARD_GUARD_OFFSETS[i].o);
			if(guard != NULL)
				state.hordeGuards.push_back(guard);
		}
	}
}

void AlteracValley::CleanupObjectiveDbGuards(uint32 index)
{
	if(index >= AV_OBJECTIVE_COUNT || !AV_OBJECTIVES[index].spawnGuards || m_mapMgr == NULL)
		return;

	vector<Creature*> guardsToHide;

	for(CreatureSqlIdMap::iterator itr = m_mapMgr->_sqlids_creatures.begin(); itr != m_mapMgr->_sqlids_creatures.end(); ++itr)
	{
		Creature* creature = itr->second;
		if(creature == NULL)
			continue;

		const uint32 entry = creature->GetEntry();
		if(!AVIsAllianceGuardEntry(entry) && !AVIsHordeGuardEntry(entry))
			continue;

		const float dx = creature->GetPositionX() - AV_OBJECTIVES[index].bannerX;
		const float dy = creature->GetPositionY() - AV_OBJECTIVES[index].bannerY;
		const float dz = creature->GetPositionZ() - AV_OBJECTIVES[index].bannerZ;
		if(((dx * dx) + (dy * dy) + (dz * dz)) > (AV_GUARD_SCAN_RADIUS * AV_GUARD_SCAN_RADIUS))
			continue;

		guardsToHide.push_back(creature);
	}

	for(vector<Creature*>::iterator itr = guardsToHide.begin(); itr != guardsToHide.end(); ++itr)
	{
		Creature* creature = *itr;
		if(creature != NULL && creature->IsInWorld())
			creature->RemoveFromWorld(false, false);
	}
}

void AlteracValley::UpdateObjectiveGuards(uint32 index)
{
	if(index >= AV_OBJECTIVE_COUNT || !AV_OBJECTIVES[index].spawnGuards)
		return;

	RefreshObjectiveGuards(index);
	CleanupObjectiveDbGuards(index);

	AVObjectiveState& state = m_objectiveStates[index];
	const bool showAlliance = (state.nodeState == AV_NODE_STATE_ALLIANCE_CONTROLLED);
	const bool showHorde = (state.nodeState == AV_NODE_STATE_HORDE_CONTROLLED);

	for(vector<Creature*>::iterator itr = state.allianceGuards.begin(); itr != state.allianceGuards.end(); ++itr)
	{
		Creature* creature = *itr;
		if(creature == NULL)
			continue;

		if(showAlliance && AVIsBaseAllianceGuardEntry(creature->GetEntry()))
		{
			if(creature->isAlive() && !creature->IsInWorld())
				creature->PushToWorld(m_mapMgr);
		}
		else if(creature->IsInWorld())
			creature->RemoveFromWorld(false, false);
	}

	for(vector<Creature*>::iterator itr = state.hordeGuards.begin(); itr != state.hordeGuards.end(); ++itr)
	{
		Creature* creature = *itr;
		if(creature == NULL)
			continue;

		if(showHorde && AVIsBaseHordeGuardEntry(creature->GetEntry()))
		{
			if(creature->isAlive() && !creature->IsInWorld())
				creature->PushToWorld(m_mapMgr);
		}
		else if(creature->IsInWorld())
			creature->RemoveFromWorld(false, false);
	}

}

Creature* AlteracValley::SpawnObjectiveGuard(uint32 entry, float x, float y, float z, float o)
{
	CreatureProto* proto = CreatureProtoStorage.LookupEntry(entry);
	CreatureInfo* info = CreatureNameStorage.LookupEntry(entry);
	if(proto == NULL || info == NULL)
		return NULL;

	CreatureSpawn* sp = new CreatureSpawn;
	sp->entry = entry;
	sp->form = 0;
	sp->id = 0;
	sp->movetype = 0;
	sp->x = x;
	sp->y = y;
	sp->z = z;
	sp->o = o;
	sp->emote_state = 0;
	sp->flags = 0;
	sp->factionid = proto->Faction;
	sp->bytes = 0;
	sp->bytes2 = 0;
	sp->stand_state = 0;
	sp->channel_spell = sp->channel_target_creature = sp->channel_target_go = 0;

	Creature* p = m_mapMgr->CreateCreature(entry);
	if(p == NULL)
	{
		delete sp;
		return NULL;
	}

	p->Load(sp, (uint32)NULL, NULL);
	p->spawnid = 0;
	p->m_spawn = 0;
	delete sp;
	p->PushToWorld(m_mapMgr);
	return p;
}

void AlteracValley::UpdateObjectivePrisoners(uint32 index)
{
	if(index >= AV_OBJECTIVE_COUNT || m_mapMgr == NULL)
		return;

	const int32 rescuedByTeam = AVGetPrisonerOwningTeam(index);
	if(rescuedByTeam < 0)
		return;

	const AVObjectiveState& state = m_objectiveStates[index];
	const bool shouldShow = (!state.destroyed && state.assaultingTeam == -1 && state.owner == rescuedByTeam);
	vector<Creature*> prisonersToShow;
	vector<Creature*> prisonersToHide;

	for(CreatureSqlIdMap::iterator itr = m_mapMgr->_sqlids_creatures.begin(); itr != m_mapMgr->_sqlids_creatures.end(); ++itr)
	{
		Creature* creature = itr->second;
		if(creature == NULL || !AVIsObjectivePrisonerEntry(index, creature->GetEntry()))
			continue;

		const float dx = creature->GetPositionX() - AV_OBJECTIVES[index].bannerX;
		const float dy = creature->GetPositionY() - AV_OBJECTIVES[index].bannerY;
		if(((dx * dx) + (dy * dy)) > (80.0f * 80.0f))
			continue;

		if(shouldShow)
		{
			if(!creature->IsInWorld())
				prisonersToShow.push_back(creature);
		}
		else if(creature->IsInWorld())
			prisonersToHide.push_back(creature);
	}

	for(vector<Creature*>::iterator itr = prisonersToShow.begin(); itr != prisonersToShow.end(); ++itr)
	{
		Creature* creature = *itr;
		if(creature != NULL && !creature->IsInWorld())
			creature->PushToWorld(m_mapMgr);
	}

	for(vector<Creature*>::iterator itr = prisonersToHide.begin(); itr != prisonersToHide.end(); ++itr)
	{
		Creature* creature = *itr;
		if(creature != NULL && creature->IsInWorld())
			creature->RemoveFromWorld(false, false);
	}
}

void AlteracValley::EndBattleground(uint32 winningTeam)
{
	if(m_ended)
		return;

	m_ended = true;
	m_winningteam = (uint8)winningTeam;
	m_nextPvPUpdateTime = 0;

	sEventMgr.RemoveEvents(this);
	sEventMgr.AddEvent(((CBattleground*)this), &CBattleground::Close, EVENT_BATTLEGROUND_CLOSE, 120000, 1, 0);

	SpellEntry* winnerSpell = dbcSpell.LookupEntry(24953);
	SpellEntry* loserSpell = dbcSpell.LookupEntry(24952);

	for(uint32 i = 0; i < 2; ++i)
	{
		for(set<Player*>::iterator itr = m_players[i].begin(); itr != m_players[i].end(); ++itr)
		{
			(*itr)->Root();
			if(i == winningTeam && winnerSpell != NULL)
				(*itr)->CastSpell((*itr), winnerSpell, true);
			else if(i != winningTeam && loserSpell != NULL)
				(*itr)->CastSpell((*itr), loserSpell, true);
		}
	}

	PlaySoundToAll(winningTeam ? SOUND_HORDEWINS : SOUND_ALLIANCEWINS);
	UpdatePvPData();
}
