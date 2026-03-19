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
#define EVENT_AV_MINE_RESPAWN 9902
#define AV_AIR_SUPPORT_STRIKE_DURATION_MS 60000
#define AV_AIR_SUPPORT_STRIKE_PULSE_MS 10000
#define AV_AIR_SUPPORT_STRIKE_REINFORCEMENT_DAMAGE 2
#define AV_AIR_SUPPORT_RIDER_ALTITUDE 28.0f

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

#define AV_QUEST_MORE_BOOTY               6741
#define AV_QUEST_MORE_ARMOR_SCRAPS        6781
#define AV_QUEST_LOKHOLAR_THE_ICE_LORD    6801
#define AV_QUEST_CALL_OF_AIR_GUSES_FLEET       6825
#define AV_QUEST_CALL_OF_AIR_JEZTORS_FLEET     6826
#define AV_QUEST_CALL_OF_AIR_MULVERICKS_FLEET  6827
#define AV_QUEST_CALL_OF_AIR_VIPORES_FLEET     6941
#define AV_QUEST_CALL_OF_AIR_SLIDORES_FLEET    6942
#define AV_QUEST_CALL_OF_AIR_ICHMANS_FLEET     6943
#define AV_QUEST_IVUS_THE_FOREST_LORD     6881
#define AV_QUEST_ARMOR_SCRAPS             7223
#define AV_QUEST_ENEMY_BOOTY              7224
#define AV_QUEST_A_GALLON_OF_BLOOD        7385
#define AV_QUEST_CRYSTAL_CLUSTER          7386

#define AV_SCRAPS_PER_TURNIN             20
#define AV_BLOOD_PER_TURNIN               5
#define AV_STORM_CRYSTALS_PER_TURNIN      5

#define AV_ARMOR_TIER1_THRESHOLD        500
#define AV_ARMOR_TIER2_THRESHOLD       1000
#define AV_ARMOR_TIER3_THRESHOLD       1500
#define AV_ELEMENTAL_SUMMON_THRESHOLD   200

// Air support fleet slots:
// Alliance: 0=Slidore, 1=Vipore, 2=Ichman
// Horde:    0=Guse,    1=Jeztor, 2=Mulverick
static const uint32 AV_AIR_SUPPORT_COMMANDER_ENTRY[2][3] =
{
	{ 13438, 13439, 13437 },
	{ 13179, 13180, 13181 }
};

static const uint32 AV_AIR_SUPPORT_READY_THRESHOLD_PER_FLEET[3] = { 90, 60, 30 };

static const uint32 AV_AIR_SUPPORT_RIDER_ENTRY[2][3] =
{
	// Alliance
	{ 14946, 14948, 14947 },
	// Horde
	{ 14943, 14944, 14945 }
};

static inline uint32 AVGetAirSupportReadyThreshold(uint32 fleet)
{
	if(fleet > 2)
		return 90;

	return AV_AIR_SUPPORT_READY_THRESHOLD_PER_FLEET[fleet];
}

struct AVAirSupportStrikeProfile
{
	float x, y, z, radius;
	const char* targetDescription;
	uint32 reinforcementDamagePerPulse;
};

static const AVAirSupportStrikeProfile AV_AIR_SUPPORT_STRIKE_PROFILE[2][3] =
{
	// Alliance strikes against Horde territory
	{
		{ -1082.53f, -346.567f, 54.9771f, 45.0f, "Frostwolf Graveyard", 4 },
		{ -1402.21f, -307.431f, 89.4424f, 65.0f, "Frostwolf Village", 3 },
		{ -1377.0f, -229.0f, 98.0f, 70.0f, "Drek'Thar's fortress", 2 }
	},
	// Horde strikes against Alliance territory
	{
		{ 669.007f, -294.078f, 30.2909f, 45.0f, "Stormpike Graveyard", 4 },
		{ 620.0f, -110.0f, 58.0f, 80.0f, "the Dun Baldar bunker line", 3 },
		{ 638.592f, -32.422f, 46.0608f, 50.0f, "Stormpike Aid Station", 2 }
	}
};

struct AVAirSupportVisualOffset
{
	float x, y, z, o;
};

static const AVAirSupportVisualOffset AV_AIR_SUPPORT_STRIKE_VISUAL_OFFSETS[4] =
{
	{  0.0f,   0.0f, 0.0f, 0.0f },
	{ 12.0f,   8.0f, 0.0f, 0.7f },
	{ -10.0f, 11.0f, 0.0f, 1.9f },
	{  6.0f, -13.0f, 0.0f, 3.2f }
};

static const float AV_AIR_SUPPORT_RIDER_LATERAL_OFFSETS[3] = { -10.0f, 0.0f, 10.0f };

static inline const AVAirSupportStrikeProfile& AVGetAirSupportStrikeProfile(uint32 team, uint32 fleet)
{
	static const AVAirSupportStrikeProfile kFallback = { 0.0f, 0.0f, 0.0f, 20.0f, "enemy territory", AV_AIR_SUPPORT_STRIKE_REINFORCEMENT_DAMAGE };
	if(team > 1 || fleet > 2)
		return kFallback;

	return AV_AIR_SUPPORT_STRIKE_PROFILE[team][fleet];
}

struct AVAirSupportWaypoint
{
	float x, y, z, o, radius;
};

static const AVAirSupportWaypoint AV_AIR_SUPPORT_ROUTE_ALLIANCE_SLIDORE[] =
{
	{ -650.0f, -300.0f, 78.0f, 0.40f, 18.0f },
	{ -450.0f, -225.0f, 72.0f, 0.25f, 20.0f },
	{ -125.0f, -155.0f, 58.0f, 0.15f, 22.0f },
	{  250.0f, -105.0f, 50.0f, 0.05f, 24.0f },
	{  648.0f,  -36.0f, 46.2f, 0.00f, 20.0f }
};

static const AVAirSupportWaypoint AV_AIR_SUPPORT_ROUTE_ALLIANCE_VIPORE[] =
{
	{ -1185.0f, -318.0f, 62.0f, 0.30f, 18.0f },
	{  -950.0f, -235.0f, 69.0f, 0.20f, 20.0f },
	{  -575.0f, -170.0f, 73.0f, 0.15f, 22.0f },
	{  -120.0f,  -90.0f, 56.0f, 0.10f, 24.0f },
	{   657.0f,  -24.0f, 46.2f, 0.00f, 20.0f }
};

static const AVAirSupportWaypoint AV_AIR_SUPPORT_ROUTE_ALLIANCE_ICHMAN[] =
{
	{ -1180.0f, -250.0f, 98.0f, 0.15f, 18.0f },
	{  -925.0f, -185.0f, 83.0f, 0.12f, 20.0f },
	{  -500.0f, -135.0f, 73.0f, 0.10f, 22.0f },
	{   -60.0f,  -72.0f, 55.0f, 0.05f, 24.0f },
	{   666.0f,  -12.0f, 46.2f, 0.00f, 20.0f }
};

static const AVAirSupportWaypoint AV_AIR_SUPPORT_ROUTE_HORDE_GUSE[] =
{
	{   75.0f, -345.0f, 52.0f, 3.10f, 18.0f },
	{ -180.0f, -330.0f, 58.0f, 3.12f, 20.0f },
	{ -475.0f, -315.0f, 64.0f, 3.13f, 22.0f },
	{ -925.0f, -300.0f, 74.0f, 3.13f, 24.0f },
	{ -1390.0f, -308.0f, 89.5f, 3.14f, 20.0f }
};

static const AVAirSupportWaypoint AV_AIR_SUPPORT_ROUTE_HORDE_JEZTOR[] =
{
	{  150.0f, -520.0f, 66.0f, 3.00f, 18.0f },
	{ -125.0f, -475.0f, 60.0f, 3.05f, 20.0f },
	{ -450.0f, -410.0f, 62.0f, 3.10f, 22.0f },
	{ -925.0f, -335.0f, 74.0f, 3.12f, 24.0f },
	{ -1402.0f, -296.0f, 89.5f, 3.14f, 20.0f }
};

static const AVAirSupportWaypoint AV_AIR_SUPPORT_ROUTE_HORDE_MULVERICK[] =
{
	{  425.0f, -165.0f, 57.0f, 3.05f, 18.0f },
	{  100.0f, -205.0f, 56.0f, 3.10f, 20.0f },
	{ -300.0f, -250.0f, 61.0f, 3.12f, 22.0f },
	{ -825.0f, -285.0f, 72.0f, 3.13f, 24.0f },
	{ -1414.0f, -284.0f, 89.5f, 3.14f, 20.0f }
};

static const AVAirSupportWaypoint* AVGetAirSupportRoute(uint32 team, uint32 fleet, uint32& count)
{
	count = 0;
	if(team > 1 || fleet > 2)
		return NULL;

	if(team == 0)
	{
		switch(fleet)
		{
		case 0: count = sizeof(AV_AIR_SUPPORT_ROUTE_ALLIANCE_SLIDORE) / sizeof(AVAirSupportWaypoint); return AV_AIR_SUPPORT_ROUTE_ALLIANCE_SLIDORE;
		case 1: count = sizeof(AV_AIR_SUPPORT_ROUTE_ALLIANCE_VIPORE) / sizeof(AVAirSupportWaypoint); return AV_AIR_SUPPORT_ROUTE_ALLIANCE_VIPORE;
		case 2: count = sizeof(AV_AIR_SUPPORT_ROUTE_ALLIANCE_ICHMAN) / sizeof(AVAirSupportWaypoint); return AV_AIR_SUPPORT_ROUTE_ALLIANCE_ICHMAN;
		}
	}
	else
	{
		switch(fleet)
		{
		case 0: count = sizeof(AV_AIR_SUPPORT_ROUTE_HORDE_GUSE) / sizeof(AVAirSupportWaypoint); return AV_AIR_SUPPORT_ROUTE_HORDE_GUSE;
		case 1: count = sizeof(AV_AIR_SUPPORT_ROUTE_HORDE_JEZTOR) / sizeof(AVAirSupportWaypoint); return AV_AIR_SUPPORT_ROUTE_HORDE_JEZTOR;
		case 2: count = sizeof(AV_AIR_SUPPORT_ROUTE_HORDE_MULVERICK) / sizeof(AVAirSupportWaypoint); return AV_AIR_SUPPORT_ROUTE_HORDE_MULVERICK;
		}
	}

	return NULL;
}

 // Faction here is based on the corpse owner's team:
 // 0 = Alliance corpse drops Stormpike items, 1 = Horde corpse drops Frostwolf items, -1 = both.
struct AVLoot
{
	uint32 ItemId;
	int8 Faction;
	float Chance;
	uint32 MinCount;
	uint32 MaxCount;
};

static const char* AVGetElementalName(uint32 team)
{
	switch(team)
	{
	case 0:
		return "Ivus the Forest Lord";
	case 1:
		return "Lokholar the Ice Lord";
	default:
		return "the ancient elemental";
	}
}

static const char* AVGetElementalResourceName(uint32 team)
{
	return (team == 0) ? "storm crystals" : "enemy blood";
}

static const char* AVGetAirSupportFleetName(uint32 team, uint32 fleet)
{
	static const char* kAlliance[3] = { "Slidore", "Vipore", "Ichman" };
	static const char* kHorde[3] = { "Guse", "Jeztor", "Mulverick" };

	if(team > 1 || fleet > 2)
		return "Unknown";

	return (team == 0) ? kAlliance[fleet] : kHorde[fleet];
}

static const char* AVGetAirSupportStrikeName(uint32 team, uint32 fleet)
{
	if(team > 1 || fleet > 2)
		return "air support";

	if(team == 0)
	{
		switch(fleet)
		{
		case 0: return "Slidore's Gryphon Riders";
		case 1: return "Vipore's Gryphon Riders";
		case 2: return "Ichman's Gryphon Riders";
		}
	}
	else
	{
		switch(fleet)
		{
		case 0: return "Guse's War Riders";
		case 1: return "Jeztor's War Riders";
		case 2: return "Mulverick's War Riders";
		}
	}

	return "air support";
}

static const char* AVGetAirSupportCommanderFullName(uint32 team, uint32 fleet)
{
	if(team > 1 || fleet > 2)
		return "Wing Commander";

	if(team == 0)
	{
		switch(fleet)
		{
		case 0: return "Wing Commander Slidore";
		case 1: return "Wing Commander Vipore";
		case 2: return "Wing Commander Ichman";
		}
	}
	else
	{
		switch(fleet)
		{
		case 0: return "Wing Commander Guse";
		case 1: return "Wing Commander Jeztor";
		case 2: return "Wing Commander Mulverick";
		}
	}

	return "Wing Commander";
}

static bool AVGetAirSupportQuestInfo(uint32 questId, uint32& team, uint32& fleet)
{
	switch(questId)
	{
		case AV_QUEST_CALL_OF_AIR_SLIDORES_FLEET:
			team = 0; fleet = 0; return true;
		case AV_QUEST_CALL_OF_AIR_VIPORES_FLEET:
			team = 0; fleet = 1; return true;
		case AV_QUEST_CALL_OF_AIR_ICHMANS_FLEET:
			team = 0; fleet = 2; return true;
		case AV_QUEST_CALL_OF_AIR_GUSES_FLEET:
			team = 1; fleet = 0; return true;
		case AV_QUEST_CALL_OF_AIR_JEZTORS_FLEET:
			team = 1; fleet = 1; return true;
		case AV_QUEST_CALL_OF_AIR_MULVERICKS_FLEET:
			team = 1; fleet = 2; return true;
		default:
			break;
	}

	team = 0;
	fleet = 0;
	return false;
}

static bool AVGetAirSupportCommanderByEntry(uint32 entry, uint32& team, uint32& fleet)
{
	for(uint32 t = 0; t < 2; ++t)
	{
		for(uint32 f = 0; f < 3; ++f)
		{
			if(AV_AIR_SUPPORT_COMMANDER_ENTRY[t][f] == entry)
			{
				team = t;
				fleet = f;
				return true;
			}
		}
	}

	team = 0;
	fleet = 0;
	return false;
}

static inline uint32 AVGetAirSupportCaptiveFaction(uint32 team) { return (team == 0) ? AV_VISUAL_FACTION_ALLIANCE : AV_VISUAL_FACTION_HORDE; }
static inline uint32 AVGetAirSupportHomeFaction(uint32 team)    { return (team == 0) ? AV_FACTION_ALLIANCE : AV_FACTION_HORDE; }

struct AVMineTemplate
{
	const char* name;
	float x, y, z;
	float radius;
	uint32 neutralBossEntry;
	uint32 allianceBossEntry;
	uint32 hordeBossEntry;
	const uint32* neutralEntries;
	uint32 neutralEntryCount;
	const uint32* allianceEntries;
	uint32 allianceEntryCount;
	const uint32* hordeEntries;
	uint32 hordeEntryCount;
	uint32 worldStateAlliance;
	uint32 worldStateHorde;
	uint32 worldStateNeutral;
};

static const AVLoot g_avLoot[] =
{
	{ 17306,  0, 100.0f, 1, 1 }, // Stormpike Soldier's Blood
	{ 17326,  0,  80.0f, 1, 1 }, // Stormpike Soldier's Flesh
	{ 17423,  1,  50.0f, 1, 1 }, // Crystal Cluster
	{ 17422, -1, 100.0f, 1, 5 }, // Armor Scraps
	{ 0, 0, 0, 0, 0 }
};

// AV mine AreaPOI worldstates
// Coldtooth: 1355 Alliance, 1356 Horde, 1357 Kobold
// Irondeep:  1358 Alliance, 1359 Horde, 1360 Trogg
#define AV_WS_COLDTOOTH_ALLIANCE 1355
#define AV_WS_COLDTOOTH_HORDE    1356
#define AV_WS_COLDTOOTH_NEUTRAL  1357
#define AV_WS_IRONDEEP_ALLIANCE  1358
#define AV_WS_IRONDEEP_HORDE     1359
#define AV_WS_IRONDEEP_NEUTRAL   1360

static const uint32 AV_MINE_ENTRIES_IRONDEEP_NEUTRAL[] = { 10987, 11600, 11602, 11657 };
static const uint32 AV_MINE_ENTRIES_IRONDEEP_ALLIANCE[] = { 13078, 13080, 13081, 13098, 13396 };
static const uint32 AV_MINE_ENTRIES_IRONDEEP_HORDE[] = { 13079, 13099, 13397 };
static const uint32 AV_MINE_ENTRIES_COLDTOOTH_NEUTRAL[] = { 11603, 11604, 11605, 11677 };
static const uint32 AV_MINE_ENTRIES_COLDTOOTH_ALLIANCE[] = { 13086, 13096, 13317 };
static const uint32 AV_MINE_ENTRIES_COLDTOOTH_HORDE[] = { 13088, 13097, 13316 };
static const float AV_BOSS_ROOM_CENTERS[2][4] =
{
	{ 726.0f, -10.0f, 50.0f, 35.0f },
	{ -1377.0f, -229.0f, 98.0f, 35.0f },
};

static const AVMineTemplate AV_MINES[AlteracValley::AV_MINE_COUNT] =
{
	{ "Irondeep Mine", 880.0f, -400.0f, 58.0f, 150.0f, 11657, 13078, 13079,
		AV_MINE_ENTRIES_IRONDEEP_NEUTRAL, sizeof(AV_MINE_ENTRIES_IRONDEEP_NEUTRAL) / sizeof(uint32),
		AV_MINE_ENTRIES_IRONDEEP_ALLIANCE, sizeof(AV_MINE_ENTRIES_IRONDEEP_ALLIANCE) / sizeof(uint32),
		AV_MINE_ENTRIES_IRONDEEP_HORDE, sizeof(AV_MINE_ENTRIES_IRONDEEP_HORDE) / sizeof(uint32),
		AV_WS_IRONDEEP_ALLIANCE, AV_WS_IRONDEEP_HORDE, AV_WS_IRONDEEP_NEUTRAL },
	{ "Coldtooth Mine", -862.0f, -82.0f, 68.0f, 150.0f, 11677, 13086, 13088,
		AV_MINE_ENTRIES_COLDTOOTH_NEUTRAL, sizeof(AV_MINE_ENTRIES_COLDTOOTH_NEUTRAL) / sizeof(uint32),
		AV_MINE_ENTRIES_COLDTOOTH_ALLIANCE, sizeof(AV_MINE_ENTRIES_COLDTOOTH_ALLIANCE) / sizeof(uint32),
		AV_MINE_ENTRIES_COLDTOOTH_HORDE, sizeof(AV_MINE_ENTRIES_COLDTOOTH_HORDE) / sizeof(uint32),
		AV_WS_COLDTOOTH_ALLIANCE, AV_WS_COLDTOOTH_HORDE, AV_WS_COLDTOOTH_NEUTRAL },
};

static const AlteracValley::AVMineCreatureSpawn AV_MINE_SPAWNS_IRONDEEP_NEUTRAL[] =
{
	{ 11602, 922.7150f, -405.0110f, 58.1280f, 4.896920f, 0, 59, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 11657, 865.5541f, -438.7354f, 50.7333f, 0.323308f, 0, 59, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 10987, 783.1050f, -343.7300f, 61.4101f, 5.486630f, 0, 59, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 10987, 834.3540f, -355.5260f, 48.1491f, 6.073750f, 0, 59, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 10987, 847.9900f, -386.2870f, 60.9277f, 2.323740f, 0, 59, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 10987, 874.5770f, -414.7860f, 52.7817f, 1.675520f, 0, 59, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 10987, 888.2080f, -332.5640f, 68.1480f, 1.937320f, 0, 59, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 10987, 905.0670f, -396.0740f, 60.2085f, 5.078910f, 0, 59, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 10987, 916.8520f, -393.8910f, 60.1726f, 2.716950f, 0, 59, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 10987, 931.1460f, -359.6660f, 66.0294f, 3.961900f, 0, 59, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 10987, 971.6710f, -442.6570f, 57.6951f, 3.176500f, 0, 59, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 11600, 808.9530f, -325.9640f, 52.4043f, 3.019420f, 0, 59, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 11600, 827.5700f, -417.4830f, 48.4538f, 1.492370f, 0, 59, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 11600, 850.9220f, -390.3990f, 60.8771f, 2.854050f, 0, 59, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 11600, 858.5930f, -439.6140f, 50.2184f, 0.872665f, 0, 59, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 11600, 871.2820f, -403.8430f, 62.1108f, 0.788382f, 0, 59, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 11600, 876.0470f, -341.8570f, 65.8743f, 4.450590f, 0, 59, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 11600, 884.2370f, -407.5970f, 61.5660f, 0.820305f, 0, 59, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 11600, 924.7290f, -397.4530f, 60.2130f, 2.716950f, 0, 59, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 11600, 957.2930f, -455.0390f, 56.7395f, 5.794490f, 0, 59, 0, 0, 0, 0, 0, 0, 0, 0 },
};

static const AlteracValley::AVMineCreatureSpawn AV_MINE_SPAWNS_IRONDEEP_ALLIANCE[] =
{
	{ 13078, 880.2361f, -444.5867f, 54.6063f, 2.460914f, 0, 1216, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13080, 808.9530f, -325.9640f, 52.4043f, 3.019420f, 0, 1216, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13080, 827.5700f, -417.4830f, 48.4538f, 1.492370f, 0, 1216, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13080, 847.5560f, -388.2280f, 60.9438f, 2.568720f, 0, 1216, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13080, 857.2760f, -395.3950f, 61.2418f, 0.084555f, 0, 1216, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13080, 868.2560f, -392.3630f, 61.4803f, 0.732738f, 0, 1216, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13080, 871.5610f, -404.1140f, 62.1297f, 0.009817f, 0, 1216, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13080, 880.1560f, -400.6780f, 61.3113f, 3.413730f, 0, 1216, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13080, 897.4640f, -338.7580f, 68.1715f, 2.949610f, 0, 1216, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13080, 924.7290f, -397.4530f, 60.2130f, 2.716950f, 0, 1216, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13080, 957.2930f, -455.0390f, 56.7395f, 5.794490f, 0, 1216, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13396, 831.7110f, -346.7850f, 47.2975f, 0.226893f, 0, 1216, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13396, 835.0770f, -379.4180f, 48.2755f, 5.934120f, 0, 1216, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13396, 857.5130f, -351.8170f, 65.1867f, 4.398230f, 0, 1216, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13396, 878.6750f, -345.3600f, 66.1052f, 3.456510f, 0, 1216, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13396, 893.3760f, -343.1710f, 68.1499f, 5.358160f, 0, 1216, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13396, 907.2090f, -428.2670f, 59.8065f, 1.867500f, 0, 1216, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13396, 919.2740f, -394.9860f, 60.3478f, 2.716960f, 0, 1216, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13396, 931.1460f, -359.6660f, 66.0294f, 3.961900f, 0, 1216, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13396, 971.6710f, -442.6570f, 57.6951f, 3.176500f, 0, 1216, 0, 0, 1, 233, 0, 0, 0, 0 },
};

static const AlteracValley::AVMineCreatureSpawn AV_MINE_SPAWNS_IRONDEEP_HORDE[] =
{
	{ 13079, 879.2206f, -443.2573f, 54.6478f, 1.832596f, 0, 1214, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13099, 808.9530f, -325.9640f, 52.4043f, 3.019420f, 0, 1214, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13099, 827.5700f, -417.4830f, 48.4538f, 1.492370f, 0, 1214, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13099, 847.5560f, -388.2280f, 60.9438f, 2.568720f, 0, 1214, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13099, 857.2760f, -395.3950f, 61.2418f, 0.084555f, 0, 1214, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13099, 868.2560f, -392.3630f, 61.4803f, 0.732738f, 0, 1214, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13099, 871.5610f, -404.1140f, 62.1297f, 0.009817f, 0, 1214, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13099, 880.1560f, -400.6780f, 61.3113f, 3.413730f, 0, 1214, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13099, 897.4640f, -338.7580f, 68.1715f, 2.949610f, 0, 1214, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13099, 924.7290f, -397.4530f, 60.2130f, 2.716950f, 0, 1214, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13099, 957.2930f, -455.0390f, 56.7395f, 5.794490f, 0, 1214, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13397, 754.2040f, -322.7540f, 57.4426f, 5.209390f, 0, 1214, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13397, 834.6340f, -365.9810f, 62.8801f, 1.326450f, 0, 1214, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13397, 849.8600f, -340.9440f, 66.2447f, 0.401426f, 0, 1214, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13397, 877.1270f, -351.8000f, 66.5296f, 5.742130f, 0, 1214, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13397, 890.5840f, -406.0490f, 61.1925f, 5.672320f, 0, 1214, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13397, 905.9730f, -459.5280f, 58.7594f, 1.371890f, 0, 1214, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13397, 919.2740f, -394.9860f, 60.3478f, 2.716960f, 0, 1214, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13397, 931.1460f, -359.6660f, 66.0294f, 3.961900f, 0, 1214, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13397, 971.6710f, -442.6570f, 57.6951f, 3.176500f, 0, 1214, 0, 0, 1, 233, 0, 0, 0, 0 },
};

static const AlteracValley::AVMineCreatureSpawn AV_MINE_SPAWNS_COLDTOOTH_NEUTRAL[] =
{
	{ 11605, -857.7100f, -91.4395f, 68.5389f, 6.089830f, 0, 26, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 11677, -848.9024f, -92.9310f, 68.6325f, 3.333579f, 0, 26, 0, 0, 1, 0, 0, 0, 0, 0 },
	{ 11603, -978.6780f, -37.3136f, 75.8364f, 2.844890f, 0, 26, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 11603, -954.2310f, -169.5150f, 78.0482f, 1.962660f, 0, 26, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 11603, -947.8540f, -170.5000f, 79.7618f, 0.942478f, 0, 26, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 11603, -938.1970f, -155.8380f, 61.3111f, 1.658060f, 0, 26, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 11603, -916.7500f, -136.0940f, 62.2357f, 0.069813f, 0, 26, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 11603, -905.4550f, -84.5179f, 75.3642f, 3.298670f, 0, 26, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 11603, -901.9770f, -82.8394f, 74.4376f, 5.232970f, 0, 26, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 11603, -888.4680f, -148.4620f, 61.8012f, 1.658060f, 0, 26, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 11603, -872.1350f, -150.0800f, 62.7513f, 3.572010f, 0, 26, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 11603, -865.6480f, -63.2401f, 71.4081f, 3.174600f, 0, 26, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 11603, -853.3570f, -0.6962f, 72.0655f, 0.994838f, 0, 26, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 11604, -976.0860f, -44.1775f, 76.0290f, 1.466080f, 0, 26, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 11604, -951.4770f, -53.9647f, 80.0235f, 5.323250f, 0, 26, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 11604, -920.8640f, -40.2009f, 78.2560f, 5.166170f, 0, 26, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 11604, -894.8910f, -153.9510f, 61.6827f, 3.235690f, 0, 26, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 11604, -868.4400f, -121.6490f, 64.5056f, 3.333580f, 0, 26, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 11604, -859.8460f, -19.6549f, 70.7304f, 1.972220f, 0, 26, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 11604, -824.2040f, -65.0530f, 72.3381f, 3.019420f, 0, 26, 0, 0, 0, 0, 0, 0, 0, 0 },
};

static const AlteracValley::AVMineCreatureSpawn AV_MINE_SPAWNS_COLDTOOTH_ALLIANCE[] =
{
	{ 13086, -849.4902f, -93.5311f, 68.5934f, 3.700098f, 0, 1216, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13096, -976.0860f, -44.1775f, 76.0290f, 1.466080f, 0, 1216, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13096, -954.6220f, -110.9580f, 80.7911f, 6.248280f, 0, 1216, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13096, -933.9540f, -159.6320f, 60.7780f, 2.565630f, 0, 1216, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13096, -915.8620f, -151.7400f, 76.9427f, 0.942478f, 0, 1216, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13096, -888.3210f, -159.8310f, 62.5303f, 1.204280f, 0, 1216, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13096, -868.4400f, -121.6490f, 64.5056f, 3.333580f, 0, 1216, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13096, -859.8460f, -19.6549f, 70.7304f, 1.972220f, 0, 1216, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13096, -824.2040f, -65.0530f, 72.3381f, 3.019420f, 0, 1216, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13317, -978.6780f, -37.3136f, 75.8364f, 2.844890f, 0, 1216, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13317, -957.6230f, -186.5820f, 66.6021f, 1.954770f, 0, 1216, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13317, -949.9440f, -142.9770f, 80.5382f, 2.705260f, 0, 1216, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13317, -943.6780f, -110.9860f, 80.2557f, 0.959931f, 0, 1216, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13317, -927.4120f, -135.3130f, 61.1987f, 3.298670f, 0, 1216, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13317, -912.6890f, -45.4494f, 76.2277f, 4.607670f, 0, 1216, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13317, -904.0230f, -90.4558f, 75.3706f, 3.403390f, 0, 1216, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13317, -892.4080f, -162.5250f, 64.1212f, 2.698840f, 0, 1216, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13317, -874.9010f, -36.6579f, 69.4246f, 2.007130f, 0, 1216, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13317, -869.0230f, -82.2118f, 69.5848f, 3.228860f, 0, 1216, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13317, -853.3570f, -0.6962f, 72.0655f, 0.994838f, 0, 1216, 0, 0, 1, 233, 0, 0, 0, 0 },
};

static const AlteracValley::AVMineCreatureSpawn AV_MINE_SPAWNS_COLDTOOTH_HORDE[] =
{
	{ 13088, -849.4163f, -93.4279f, 68.5198f, 3.228859f, 0, 1214, 0, 0, 1, 0, 0, 0, 0, 0 },
	{ 13097, -987.3580f, -262.4960f, 65.3914f, 0.510012f, 0, 1214, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13097, -954.6220f, -110.9580f, 80.7911f, 6.248280f, 0, 1214, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13097, -933.9540f, -159.6320f, 60.7780f, 2.565630f, 0, 1214, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13097, -915.8620f, -151.7400f, 76.9427f, 0.942478f, 0, 1214, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13097, -888.3210f, -159.8310f, 62.5303f, 1.204280f, 0, 1214, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13097, -868.4400f, -121.6490f, 64.5056f, 3.333580f, 0, 1214, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13097, -859.8460f, -19.6549f, 70.7304f, 1.972220f, 0, 1214, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13097, -824.2040f, -65.0530f, 72.3381f, 3.019420f, 0, 1214, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 13316, -978.6780f, -37.3136f, 75.8364f, 2.844890f, 0, 1214, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13316, -961.9410f, -90.7252f, 81.6629f, 0.820305f, 0, 1214, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13316, -950.1690f, -188.0990f, 66.6184f, 5.550150f, 0, 1214, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13316, -943.6780f, -110.9860f, 80.2557f, 0.959931f, 0, 1214, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13316, -927.4120f, -135.3130f, 61.1987f, 3.298670f, 0, 1214, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13316, -913.5890f, -146.7940f, 76.9366f, 1.867500f, 0, 1214, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13316, -904.2700f, -160.4190f, 61.9876f, 3.611920f, 0, 1214, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13316, -892.4080f, -162.5250f, 64.1212f, 2.698840f, 0, 1214, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13316, -876.7920f, -128.6460f, 64.1045f, 3.403390f, 0, 1214, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13316, -870.0300f, -6.2744f, 70.3867f, 2.391100f, 0, 1214, 0, 0, 1, 233, 0, 0, 0, 0 },
	{ 13316, -853.3570f, -0.6962f, 72.0655f, 0.994838f, 0, 1214, 0, 0, 1, 233, 0, 0, 0, 0 },
};

static const AlteracValley::AVMineCreatureSpawn* AVGetMineSpawnSet(uint32 mine, AVMineState owner, uint32& count)
{
	switch(mine)
	{
	case 0:
		switch(owner)
		{
		case AV_MINE_STATE_ALLIANCE:
			count = sizeof(AV_MINE_SPAWNS_IRONDEEP_ALLIANCE) / sizeof(AlteracValley::AVMineCreatureSpawn);
			return AV_MINE_SPAWNS_IRONDEEP_ALLIANCE;
		case AV_MINE_STATE_HORDE:
			count = sizeof(AV_MINE_SPAWNS_IRONDEEP_HORDE) / sizeof(AlteracValley::AVMineCreatureSpawn);
			return AV_MINE_SPAWNS_IRONDEEP_HORDE;
		default:
			count = sizeof(AV_MINE_SPAWNS_IRONDEEP_NEUTRAL) / sizeof(AlteracValley::AVMineCreatureSpawn);
			return AV_MINE_SPAWNS_IRONDEEP_NEUTRAL;
		}
	case 1:
		switch(owner)
		{
		case AV_MINE_STATE_ALLIANCE:
			count = sizeof(AV_MINE_SPAWNS_COLDTOOTH_ALLIANCE) / sizeof(AlteracValley::AVMineCreatureSpawn);
			return AV_MINE_SPAWNS_COLDTOOTH_ALLIANCE;
		case AV_MINE_STATE_HORDE:
			count = sizeof(AV_MINE_SPAWNS_COLDTOOTH_HORDE) / sizeof(AlteracValley::AVMineCreatureSpawn);
			return AV_MINE_SPAWNS_COLDTOOTH_HORDE;
		default:
			count = sizeof(AV_MINE_SPAWNS_COLDTOOTH_NEUTRAL) / sizeof(AlteracValley::AVMineCreatureSpawn);
			return AV_MINE_SPAWNS_COLDTOOTH_NEUTRAL;
		}
	default:
		count = 0;
		return NULL;
	}
}

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

static bool AVIsArmorTierDefenderEntry(uint32 entry)
{
	switch(entry)
	{
	case 12050:
	case 12053:
	case 13326:
	case 13328:
	case 13331:
	case 13332:
	case 13421:
	case 13422:
	case 14762:
	case 14763:
	case 14764:
	case 14765:
	case 14772:
	case 14773:
	case 14776:
	case 14777:
		return true;
	default:
		return false;
	}
}

static int32 AVGetArmorTierDefenderTeam(uint32 entry)
{
	if(AVIsAllianceGuardEntry(entry) || entry == 14762 || entry == 14763 || entry == 14764 || entry == 14765)
		return 0;

	if(AVIsHordeGuardEntry(entry) || entry == 14772 || entry == 14773 || entry == 14776 || entry == 14777)
		return 1;

	return -1;
}

static uint32 AVScaleStatByTier(uint32 baseValue, uint32 tier)
{
	static const uint32 kTierPct[4] = { 100, 110, 120, 130 };
	if(tier > 3)
		tier = 3;

	return (baseValue * kTierPct[tier] + 99) / 100;
}

static float AVScaleDamageByTier(float baseValue, uint32 tier)
{
	static const float kTierPct[4] = { 1.00f, 1.10f, 1.20f, 1.30f };
	if(tier > 3)
		tier = 3;

	return baseValue * kTierPct[tier];
}

static const char* AV_ARMOR_REFRESH_ENTRIES =
	"12050,12053,13326,13328,13331,13332,13421,13422,14762,14763,14764,14765,14772,14773,14776,14777";

static const char* AV_ARMOR_TIER_SCALING_MODEL = "100/110/120/130";

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

static const float AV_AIR_SUPPORT_CAPTIVE_POS[2][3][4] =
{
	// Alliance captives: Slidore, Vipore, Ichman
	{
		{ -768.864f, -360.926f, 68.6320f, 35.0f },   // Tower Point
		{ -1219.25f, -353.167f, 57.7513f, 55.0f },   // Frostwolf Village
		{ -1303.38f, -267.989f, 91.9538f, 35.0f }    // West Frostwolf Tower
	},
	// Horde captives: Guse, Jeztor, Mulverick
	{
		{ 210.875f, -357.360f, 56.4586f, 35.0f },    // Icewing Bunker
		{ 320.486f, -502.645f, 71.2321f, 55.0f },    // Stormpike Lumber Mill
		{ 674.469f, -144.534f, 63.7354f, 35.0f }     // Dun Baldar North Bunker
	}
};

static bool AVIsObjectivePrisonerEntry(uint32 index, uint32 entry)
{
	switch(index)
	{
	case 6:
		return entry == 13439;            // Vipore (Frostwolf Village / Relief Hut proxy)
	case 8:
		return entry == 13181;
	case 9:
		return entry == 13179;
	case 12:
		return entry == 13438;
	case 14:
 		return entry == 13437 || entry == 23345;
	default:
		return false;
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

static bool AVEntryInList(const uint32* entries, uint32 count, uint32 entry)
{
	for(uint32 i = 0; i < count; ++i)
	{
		if(entries[i] == entry)
			return true;
	}

	return false;
}

static bool AVIsMineEntry(const AVMineTemplate& mine, uint32 entry)
{
	return AVEntryInList(mine.neutralEntries, mine.neutralEntryCount, entry) ||
		AVEntryInList(mine.allianceEntries, mine.allianceEntryCount, entry) ||
		AVEntryInList(mine.hordeEntries, mine.hordeEntryCount, entry);
}

AlteracValley::AlteracValley(MapMgr* mgr, uint32 id, uint32 lgroup, uint32 t) : CBattleground(mgr, id, lgroup, t)
{
	m_playerCountPerTeam = 40;
	m_mineDbSpawnsHidden = false;
	Reset();
}

AlteracValley::~AlteracValley()
{
}

void AlteracValley::Reset()
{
	m_reinforcements[0] = AV_MAX_REINFORCEMENTS;
	m_reinforcements[1] = AV_MAX_REINFORCEMENTS;
	for(uint32 i = 0; i < AV_MINE_COUNT; ++i)
	{
		ClearMineRuntimeSpawns(i);
		m_mineRespawnPending[i] = false;
		m_mineOwner[i] = AV_MINE_STATE_NEUTRAL;
	}
	m_mineDbSpawnsHidden = false;
	m_captainDead[0] = false;
	m_captainDead[1] = false;

	for(uint32 team = 0; team < 2; ++team)
	{
		for(uint32 fleet = 0; fleet < 3; ++fleet)
		{
			m_teamAirSupportTurnIns[team][fleet] = 0;
			m_teamAirSupportReady[team][fleet] = false;
			m_teamAirSupportEscorting[team][fleet] = false;
			m_teamAirSupportEscortNode[team][fleet] = 0;
			m_teamAirSupportReturned[team][fleet] = false;
			m_teamAirSupportStrikeActive[team][fleet] = false;
			m_teamAirSupportStrikeTimeLeft[team][fleet] = 0;
			m_teamAirSupportStrikePulse[team][fleet] = 0;
			m_teamAirSupportStrikeVisuals[team][fleet].clear();
			m_teamAirSupportStrikeRiders[team][fleet].clear();
			m_teamAirSupportStrikePass[team][fleet] = 0;
		}
	}

	m_gates[0] = NULL;
	m_gates[1] = NULL;
	m_startGatesShouldBeOpen = false;
	m_defenderBaseHealth.clear();
	m_teamArmorScraps[0] = m_teamArmorScraps[1] = 0;
	m_teamArmorTier[0] = m_teamArmorTier[1] = 0;
	m_teamBlood[0] = m_teamBlood[1] = 0;
	m_teamStormCrystals[0] = m_teamStormCrystals[1] = 0;
	m_teamElementalReady[0] = m_teamElementalReady[1] = false;
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

	for(uint32 i = 0; i < AV_OBJECTIVE_COUNT; ++i)
	{
		if((AV_OBJECTIVES[i].type == AV_OBJECTIVE_TOWER || AV_OBJECTIVES[i].type == AV_OBJECTIVE_BUNKER) && AV_OBJECTIVES[i].linkedNpcEntry != 0)
		{
			m_objectiveStates[i].linkedUnit = FindObjectiveLinkedUnit(i);
			RefreshObjectiveLinkedUnit(i);
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
	InitializeMines();
	UpdateBossRoomGuards();
	RefreshArmorTierDefenders(0);
	RefreshArmorTierDefenders(1);

	sEventMgr.AddEvent(this, &AlteracValley::EventUpdateObjectives, EVENT_AV_OBJECTIVES_UPDATE, 1000, 0, EVENT_FLAG_DO_NOT_EXECUTE_IN_WORLD_CONTEXT);
}

void AlteracValley::OnStart()
{
	InitializeAlteracValleyNodes();
	InitializeMines();
	UpdateBossRoomGuards();

	for(uint32 i = 0; i < 2; ++i)
	{
		for(set<Player*>::iterator itr = m_players[i].begin(); itr != m_players[i].end(); ++itr)
			(*itr)->RemoveAura(BG_PREPARATION);
	}

	m_started = true;
	PlaySoundToAll(SOUND_BATTLEGROUND_BEGIN);

	m_startGatesShouldBeOpen = true;

	// Initial attempt (still useful if already loaded)
	for(uint32 i = 0; i < 2; ++i)
		SetGateOpen(i, true);

	sEventMgr.AddEvent(this, &AlteracValley::EventMineTick, EVENT_AV_MINE_TICK, AV_MINE_TICK_MS, 0, EVENT_FLAG_DO_NOT_EXECUTE_IN_WORLD_CONTEXT);
}

void AlteracValley::OnClose()
{
	// Server shutdown / battleground teardown safety:
	// remove all pending AV callbacks before tearing down runtime objects,
	// otherwise delayed mine respawns / mine ticks / objective updates can
	// execute against partially freed battleground state during exit.
	sEventMgr.RemoveEvents(this);

	m_started = false;

	for(uint32 i = 0; i < AV_MINE_COUNT; ++i)
	{
		m_mineRespawnPending[i] = false;
		ClearMineRuntimeSpawns(i);
	}

	for(uint32 team = 0; team < 2; ++team)
	{
		for(uint32 fleet = 0; fleet < 3; ++fleet)
		{
			ClearAirSupportStrikeVisuals(team, fleet);
			ClearAirSupportStrikeRiders(team, fleet);
		}
	}

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

			// During shutdown, the map/world may already be unwinding. Be conservative
			// and only try world removal if both the creature and its map context still look valid.
			if(creature->IsInWorld() && creature->GetMapMgr() != NULL)
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
	else if(HandleMineBossKill(plr, static_cast<Creature*>(pVictim)))
		return;
	else if(entry == AV_NPC_BALINDA && !m_captainDead[0])
	{
		m_captainDead[0] = true;
		ModifyReinforcements(0, -AV_REINFORCEMENT_CAPTAIN_LOSS);
		SendChatMessage(CHAT_MSG_BG_EVENT_HORDE, 0, "The Horde has slain Captain Balinda Stonehearth!");
	}
	else if(entry == AV_NPC_GALVANGAR && !m_captainDead[1])
	{
		m_captainDead[1] = true;
		ModifyReinforcements(1, -AV_REINFORCEMENT_CAPTAIN_LOSS);
		SendChatMessage(CHAT_MSG_BG_EVENT_ALLIANCE, 0, "The Alliance has slain Captain Galvangar!");
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

		if(m_objectiveStates[i].owner != (int32)plr->m_bgTeam)
			continue;

		if(m_objectiveStates[i].nodeState != AV_NODE_STATE_ALLIANCE_CONTROLLED && m_objectiveStates[i].nodeState != AV_NODE_STATE_HORDE_CONTROLLED)
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
	case AV_AREATRIGGER_COLDTOOTH:
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
	if (!m_started || m_ended)
		return;

	// Ensure start gates are opened even if they were not loaded at match start
	if(m_startGatesShouldBeOpen)
	{
		bool allOpen = true;

		for(uint32 i = 0; i < 2; ++i)
		{
			GameObject* gate = m_gates[i];

			if(gate == NULL)
				gate = m_gates[i] = FindGate(i);

			if(gate == NULL)
			{
				allOpen = false;
				continue;
			}

			// If not already open, force it
			if(gate->GetUInt32Value(GAMEOBJECT_STATE) != 0)
			{
				SetGateOpen(i, true);
				allOpen = false;
			}
		}

		// Once both gates are confirmed open, stop retrying
		if(allOpen)
			m_startGatesShouldBeOpen = false;
	}

	for(uint32 i = 0; i < AV_OBJECTIVE_COUNT; ++i)
	{
		UpdateObjectivePrisoners(i);

		if(m_objectiveStates[i].assaultingTeam == -1 || m_objectiveStates[i].destroyed)
			continue;

		if(m_objectiveStates[i].timer > 1000)
		{
			m_objectiveStates[i].timer -= 1000;
			continue;
		}

		FinalizeObjective(i);
	}

	UpdateAirSupportCommanders();
	UpdateAirSupportStrikes();
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

		RemoveObjectiveLinkedUnit(index);

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

	for(uint32 i = 0; i < AV_MINE_COUNT; ++i)
	{
		// Only grant reinforcement ticks while the mine is actually in a stable,
		// faction-controlled state and not waiting on its capture-state respawn pass.
		if(m_mineRespawnPending[i])
			continue;

		if(m_mineOwner[i] == AV_MINE_STATE_ALLIANCE || m_mineOwner[i] == AV_MINE_STATE_HORDE)
			ModifyReinforcements((uint32)m_mineOwner[i], 1);
	}
}

void AlteracValley::UpdateMineWorldStates(uint32 mine)
{
	if(mine >= AV_MINE_COUNT)
		return;

	const AVMineTemplate& mineInfo = AV_MINES[mine];
	const bool allianceControlled = (m_mineOwner[mine] == AV_MINE_STATE_ALLIANCE);
	const bool hordeControlled = (m_mineOwner[mine] == AV_MINE_STATE_HORDE);
	const bool neutralControlled = (m_mineOwner[mine] == AV_MINE_STATE_NEUTRAL);

	if(mineInfo.worldStateAlliance)
		SetWorldState(mineInfo.worldStateAlliance, allianceControlled ? 1 : 0);

	if(mineInfo.worldStateHorde)
		SetWorldState(mineInfo.worldStateHorde, hordeControlled ? 1 : 0);

	if(mineInfo.worldStateNeutral)
		SetWorldState(mineInfo.worldStateNeutral, neutralControlled ? 1 : 0);
}

void AlteracValley::HideMineDbSpawns()
{
	if (m_mineDbSpawnsHidden || m_mapMgr == NULL)
		return;

	vector<Creature*> mineCreatures;

	for(CreatureSqlIdMap::iterator itr = m_mapMgr->_sqlids_creatures.begin(); itr != m_mapMgr->_sqlids_creatures.end(); ++itr)
	{
		Creature* creature = itr->second;
		if(creature == NULL)
			continue;

		for(uint32 mine = 0; mine < AV_MINE_COUNT; ++mine)
		{
			const AVMineTemplate& mineInfo = AV_MINES[mine];
			if (!AVIsMineEntry(mineInfo, creature->GetEntry()))
				continue;

			const float dx = creature->GetPositionX() - mineInfo.x;
			const float dy = creature->GetPositionY() - mineInfo.y;
			const float dz = creature->GetPositionZ() - mineInfo.z;
			if(((dx * dx) + (dy * dy) + (dz * dz)) > (mineInfo.radius * mineInfo.radius))
				continue;

			mineCreatures.push_back(creature);
			break;
		}
	}

	for(vector<Creature*>::iterator itr = mineCreatures.begin(); itr != mineCreatures.end(); ++itr)
	{
		Creature* creature = *itr;
		if(creature != NULL && creature->IsInWorld())
			creature->RemoveFromWorld(false, false);
	}

	m_mineDbSpawnsHidden = true;
}

void AlteracValley::ClearMineRuntimeSpawns(uint32 mine)
{
	if(mine >= AV_MINE_COUNT)
		return;

	for(vector<Creature*>::iterator itr = m_mineRuntimeSpawns[mine].begin(); itr != m_mineRuntimeSpawns[mine].end(); ++itr)
	{
		Creature* creature = *itr;
		if(creature == NULL)
			continue;

		if (creature->IsInWorld() && creature->GetMapMgr() != NULL)
			creature->RemoveFromWorld(false, false);

		delete creature;
	}

	m_mineRuntimeSpawns[mine].clear();
}

Creature* AlteracValley::SpawnMineCreature(const AVMineCreatureSpawn& spawn)
{
	CreatureProto* proto = CreatureProtoStorage.LookupEntry(spawn.entry);
	CreatureInfo* info = CreatureNameStorage.LookupEntry(spawn.entry);
	if(proto == NULL || info == NULL)
		return NULL;

	CreatureSpawn* sp = new CreatureSpawn;
	sp->entry = spawn.entry;
	sp->form = 0;
	sp->id = 0;
	sp->movetype = spawn.movetype;
	sp->x = spawn.x;
	sp->y = spawn.y;
	sp->z = spawn.z;
	sp->o = spawn.o;
	sp->emote_state = spawn.emote_state;
	sp->flags = spawn.flags;
	sp->factionid = spawn.factionid ? spawn.factionid : proto->Faction;
	sp->bytes = spawn.bytes;
	sp->bytes2 = spawn.bytes2;
	sp->stand_state = spawn.stand_state;
	sp->channel_spell = spawn.channel_spell;
	sp->channel_target_go = spawn.channel_target_go;
	sp->channel_target_creature = spawn.channel_target_creature;

	Creature* creature = m_mapMgr->CreateCreature(spawn.entry);
	if(creature == NULL)
	{
		delete sp;
		return NULL;
	}

	creature->Load(sp, (uint32)NULL, NULL);
	creature->spawnid = 0;
	creature->m_spawn = 0;
	delete sp;
	creature->PushToWorld(m_mapMgr);
	return creature;
}

void AlteracValley::SpawnMineState(uint32 mine)
{
	if(mine >= AV_MINE_COUNT || m_mapMgr == NULL)
		return;

	uint32 count = 0;
	const AVMineCreatureSpawn* spawns = AVGetMineSpawnSet(mine, m_mineOwner[mine], count);
	if(spawns == NULL || count == 0)
		return;

	for (uint32 i = 0; i < count; ++i)
	{
		Creature* creature = SpawnMineCreature(spawns[i]);
		if(creature == NULL)
			continue;

		m_mineRuntimeSpawns[mine].push_back(creature);
	}
}

void AlteracValley::UpdateMineNPCs(uint32 mine)
{
	if(mine >= AV_MINE_COUNT || m_mapMgr == NULL)
		return;

	HideMineDbSpawns();
	ClearMineRuntimeSpawns(mine);
	SpawnMineState(mine);
}

void AlteracValley::ScheduleMineRespawn(uint32 mine)
{
	if(mine >= AV_MINE_COUNT)
		return;

	// Do not queue new mine events while the battleground is shutting down.
	if(!m_started || m_ended || m_mapMgr == NULL)
		return;

	if(m_mineRespawnPending[mine])
		return;

	m_mineRespawnPending[mine] = true;
	sEventMgr.AddEvent(this, &AlteracValley::EventRespawnMineNPCs, mine, EVENT_AV_MINE_RESPAWN, 750, 1, EVENT_FLAG_DO_NOT_EXECUTE_IN_WORLD_CONTEXT);
}

void AlteracValley::EventRespawnMineNPCs(uint32 mine)
{
	if(m_ended || !m_started)
	{
		if(mine < AV_MINE_COUNT)
			m_mineRespawnPending[mine] = false;
		return;
	}

	if(mine >= AV_MINE_COUNT)
		return;

	UpdateMineNPCs(mine);
	m_mineRespawnPending[mine] = false;
}

void AlteracValley::InitializeMines()
{
	HideMineDbSpawns();

	for(uint32 i = 0; i < AV_MINE_COUNT; ++i)
	{
		m_mineOwner[i] = AV_MINE_STATE_NEUTRAL;
		m_mineRespawnPending[i] = false;
		UpdateMineWorldStates(i);
		UpdateMineNPCs(i);
	}
}

void AlteracValley::SetMineOwner(uint32 mine, AVMineState owner, uint64 playerGuid, bool announce)
{
	if(mine >= AV_MINE_COUNT || m_mineOwner[mine] == owner)
		return;

	if(owner != AV_MINE_STATE_NEUTRAL &&
		owner != AV_MINE_STATE_ALLIANCE &&
		owner != AV_MINE_STATE_HORDE)
		return;

	(void)playerGuid;

	sLog.outDebug("AV mine ownership change: mine=%u old=%d new=%d pending=%u",
		mine,
		(int32)m_mineOwner[mine],
		(int32)owner,
		m_mineRespawnPending[mine] ? 1 : 0);

	m_mineOwner[mine] = owner;
	UpdateMineWorldStates(mine);

	// Do not immediately delete/respawn mine creatures during the boss kill hook.
	// The just-killed boss is part of the current runtime spawn set and may still
	// be in active death-processing code paths.
	ScheduleMineRespawn(mine);

	if(!announce)
		return;

	uint32 chatType = CHAT_MSG_BG_EVENT_NEUTRAL;
	const char* factionName = "The neutral forces";
	if(owner == AV_MINE_STATE_ALLIANCE)
	{
		chatType = CHAT_MSG_BG_EVENT_ALLIANCE;
		factionName = "The Alliance";
	}
	else if(owner == AV_MINE_STATE_HORDE)
	{
		chatType = CHAT_MSG_BG_EVENT_HORDE;
		factionName = "The Horde";
	}

	SendChatMessage(chatType, 0, "%s has taken control of %s!", factionName, AV_MINES[mine].name);
}

void AlteracValley::CaptureMine(uint32 mine, uint32 team, uint64 playerGuid)
{
	if(mine >= AV_MINE_COUNT || team > 1)
		return;

	// Ignore duplicate capture attempts while the mine is already transitioning
	// to the same owner. This prevents repeated boss death or overlapping hooks
	// from spamming announcements and re-queuing state churn.
	const AVMineState desiredOwner = (team == 0) ? AV_MINE_STATE_ALLIANCE : AV_MINE_STATE_HORDE;
	if(m_mineOwner[mine] == desiredOwner && m_mineRespawnPending[mine])
	{
		sLog.outDebug("AV mine capture ignored: mine=%u team=%u owner already transitioning",
			mine, team);
		return;
	}

	// If the mine is already fully owned by that faction and not pending a respawn,
	// do nothing.
	if(m_mineOwner[mine] == desiredOwner && !m_mineRespawnPending[mine])
	{
		sLog.outDebug("AV mine capture ignored: mine=%u team=%u owner unchanged",
			mine, team);
		return;
	}

	SetMineOwner(mine, team == 0 ? AV_MINE_STATE_ALLIANCE : AV_MINE_STATE_HORDE, playerGuid, true);
}

bool AlteracValley::HandleMineBossKill(Player* pPlayer, Creature* pVictim)
{
	if(pPlayer == NULL || pVictim == NULL || !m_started || m_ended)
		return false;

	const uint32 entry = pVictim->GetEntry();
	for(uint32 i = 0; i < AV_MINE_COUNT; ++i)
	{
		const AVMineTemplate& mineInfo = AV_MINES[i];
		if(entry != mineInfo.neutralBossEntry && entry != mineInfo.allianceBossEntry && entry != mineInfo.hordeBossEntry)
			continue;

		// Prevent reprocessing while a prior capture is already in flight.
		if(m_mineRespawnPending[i])
		{
			sLog.outDebug("AV mine boss kill ignored: mine=%u entry=%u pending=1",
				i, entry);
			return true;
		}

		sLog.outDebug("AV mine boss kill: mine=%u bossEntry=%u killerTeam=%u",
			i, entry, pPlayer->m_bgTeam);
		CaptureMine(i, pPlayer->m_bgTeam, pPlayer->GetGUID());
		return true;
	}

	return false;
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
	for(uint32 i = 0; i < AV_OBJECTIVE_COUNT; ++i)
	{
		if(!(AV_OBJECTIVES[i].type == AV_OBJECTIVE_TOWER || AV_OBJECTIVES[i].type == AV_OBJECTIVE_BUNKER))
			continue;

		if(m_objectiveStates[i].destroyed)
			RemoveObjectiveLinkedUnit(i);
		else if(m_objectiveStates[i].linkedUnit == NULL)
			m_objectiveStates[i].linkedUnit = FindObjectiveLinkedUnit(i);

		RefreshObjectiveLinkedUnit(i);
	}
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

Creature* AlteracValley::FindObjectiveLinkedUnit(uint32 index)
{
	if(index >= AV_OBJECTIVE_COUNT || AV_OBJECTIVES[index].linkedNpcEntry == 0)
		return NULL;

	const float searchX = AV_BOSS_ROOM_CENTERS[AV_OBJECTIVES[index].initialOwner == 1 ? 1 : 0][0];
	const float searchY = AV_BOSS_ROOM_CENTERS[AV_OBJECTIVES[index].initialOwner == 1 ? 1 : 0][1];
	const float searchZ = AV_BOSS_ROOM_CENTERS[AV_OBJECTIVES[index].initialOwner == 1 ? 1 : 0][2];
	return FindLinkedCreature(AV_OBJECTIVES[index].linkedNpcEntry, searchX, searchY, searchZ);
}

void AlteracValley::RefreshObjectiveLinkedUnit(uint32 index)
{
	if(index >= AV_OBJECTIVE_COUNT)
		return;

	AVObjectiveState& state = m_objectiveStates[index];
	if(state.destroyed || m_mapMgr == NULL)
		return;

	const uint32 teamIndex = (AV_OBJECTIVES[index].initialOwner == 1) ? 1 : 0;
	const float centerX = AV_BOSS_ROOM_CENTERS[teamIndex][0];
	const float centerY = AV_BOSS_ROOM_CENTERS[teamIndex][1];
	const float centerZ = AV_BOSS_ROOM_CENTERS[teamIndex][2];
	const float radiusSq = AV_BOSS_ROOM_CENTERS[teamIndex][3] * AV_BOSS_ROOM_CENTERS[teamIndex][3];
	Creature* matched = NULL;

	for(CreatureSqlIdMap::iterator itr = m_mapMgr->_sqlids_creatures.begin(); itr != m_mapMgr->_sqlids_creatures.end(); ++itr)
	{
		Creature* creature = itr->second;
		if(creature == NULL || creature->GetEntry() != AV_OBJECTIVES[index].linkedNpcEntry)
			continue;

		const float dx = creature->GetPositionX() - centerX;
		const float dy = creature->GetPositionY() - centerY;
		const float dz = creature->GetPositionZ() - centerZ;
		if(((dx * dx) + (dy * dy) + (dz * dz)) > radiusSq)
			continue;

		creature->GetAIInterface()->setOutOfCombatRange(90);
		ApplyArmorTierToDefender(creature, teamIndex, false);
		matched = creature;
		break;
	}

	state.linkedUnit = matched;
}

void AlteracValley::RemoveObjectiveLinkedUnit(uint32 index)
{
	if(index >= AV_OBJECTIVE_COUNT)
		return;

	AVObjectiveState& state = m_objectiveStates[index];
	if(m_mapMgr == NULL)
	{
		state.linkedUnit = NULL;
		return;
	}

	const uint32 teamIndex = (AV_OBJECTIVES[index].initialOwner == 1) ? 1 : 0;
	const float centerX = AV_BOSS_ROOM_CENTERS[teamIndex][0];
	const float centerY = AV_BOSS_ROOM_CENTERS[teamIndex][1];
	const float centerZ = AV_BOSS_ROOM_CENTERS[teamIndex][2];
	const float radiusSq = AV_BOSS_ROOM_CENTERS[teamIndex][3] * AV_BOSS_ROOM_CENTERS[teamIndex][3];

	for(CreatureSqlIdMap::iterator itr = m_mapMgr->_sqlids_creatures.begin(); itr != m_mapMgr->_sqlids_creatures.end(); ++itr)
	{
		Creature* creature = itr->second;
		if(creature == NULL || creature->GetEntry() != AV_OBJECTIVES[index].linkedNpcEntry)
			continue;

		const float dx = creature->GetPositionX() - centerX;
		const float dy = creature->GetPositionY() - centerY;
		const float dz = creature->GetPositionZ() - centerZ;
		if(((dx * dx) + (dy * dy) + (dz * dz)) > radiusSq)
			continue;

		creature->Despawn(0, 0);
	}

	state.linkedUnit = NULL;
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
	const int32 team = AVGetArmorTierDefenderTeam(entry);
	if(team >= 0)
		ApplyArmorTierToDefender(p, static_cast<uint32>(team), true);

 	return p;
}

void AlteracValley::UpdateObjectivePrisoners(uint32 index)
{
	if (index >= AV_OBJECTIVE_COUNT || m_mapMgr == NULL)
		return;

	vector<Creature*> prisonersToShow;
	vector<Creature*> prisonersToHide;

	for (CreatureSqlIdMap::iterator itr = m_mapMgr->_sqlids_creatures.begin(); itr != m_mapMgr->_sqlids_creatures.end(); ++itr)
	{
		Creature* creature = itr->second;
		if (creature == NULL || !AVIsObjectivePrisonerEntry(index, creature->GetEntry()))
			continue;

		uint32 team = 0, fleet = 0;
		if (!AVGetAirSupportCommanderByEntry(creature->GetEntry(), team, fleet))
			continue;

		const float dx = creature->GetPositionX() - AV_OBJECTIVES[index].bannerX;
		const float dy = creature->GetPositionY() - AV_OBJECTIVES[index].bannerY;
		if (((dx * dx) + (dy * dy)) > (80.0f * 80.0f))
			continue;

		const bool shouldShow = (!m_teamAirSupportEscorting[team][fleet] && !m_teamAirSupportReturned[team][fleet]);

		if (shouldShow)
		{
			if (!creature->IsInWorld())
				prisonersToShow.push_back(creature);
		}
		else if (creature->IsInWorld())
			prisonersToHide.push_back(creature);
	}

	for (vector<Creature*>::iterator itr = prisonersToShow.begin(); itr != prisonersToShow.end(); ++itr)
	{
		Creature* creature = *itr;
		if (creature == NULL)
			continue;

		uint32 team = 0, fleet = 0;
		if (!AVGetAirSupportCommanderByEntry(creature->GetEntry(), team, fleet))
			continue;

		if (!creature->IsInWorld())
			creature->PushToWorld(m_mapMgr);

		ConfigureAirSupportCommander(creature, team, fleet, true, false);
	}

	for (vector<Creature*>::iterator itr = prisonersToHide.begin(); itr != prisonersToHide.end(); ++itr)
	{
		Creature* creature = *itr;
		if (creature != NULL && creature->IsInWorld())
			creature->RemoveFromWorld(false, false);
	}

	// Jeztor is not tied to a normal AV objective node in this core, so keep his
	// captive state synchronized here as well.
	if (index == 0)
	{
		Creature* jeztor = FindAirSupportCommander(1, 1);
		if (jeztor != NULL && jeztor->IsInWorld() && !m_teamAirSupportEscorting[1][1] && !m_teamAirSupportReturned[1][1])
			ConfigureAirSupportCommander(jeztor, 1, 1, true, false);
	}
}

void AlteracValley::SendProgressMessage(uint32 team, const char* fmt, ...)
{
	char msg[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);

	SendChatMessage(team == 0 ? CHAT_MSG_BG_EVENT_ALLIANCE : CHAT_MSG_BG_EVENT_HORDE, 0, msg);
}

Creature* AlteracValley::FindAirSupportCommander(uint32 team, uint32 fleet)
{
	if(team > 1 || fleet > 2 || m_mapMgr == NULL)
		return NULL;

	const uint32 entry = AV_AIR_SUPPORT_COMMANDER_ENTRY[team][fleet];
	for(CreatureSqlIdMap::iterator itr = m_mapMgr->_sqlids_creatures.begin(); itr != m_mapMgr->_sqlids_creatures.end(); ++itr)
	{
		Creature* creature = itr->second;
		if(creature != NULL && creature->GetEntry() == entry)
			return creature;
	}

	return NULL;
}

void AlteracValley::ConfigureAirSupportCommander(Creature* creature, uint32 team, uint32 fleet, bool captive, bool returned)
{
	if(creature == NULL || team > 1 || fleet > 2)
		return;

	creature->SetUInt32Value(UNIT_FIELD_FACTIONTEMPLATE, captive ? AVGetAirSupportCaptiveFaction(team) : AVGetAirSupportHomeFaction(team));
	creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

	if(captive || returned)
		creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_2);
	else
		creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_2);

	if(creature->GetAIInterface() != NULL)
	{
		creature->GetAIInterface()->SetAllowedToEnterCombat(false);
		creature->GetAIInterface()->disable_melee = true;

		if(captive || returned)
		{
			creature->GetAIInterface()->m_canMove = false;
			creature->GetAIInterface()->StopMovement(0);
		}
		else
		{
			creature->GetAIInterface()->m_canMove = true;
		}
	}

	if(captive)
	{
		creature->Root();
		creature->SetStandState(STANDSTATE_KNEEL);
	}
	else if(returned)
	{
		creature->Root();
		creature->SetStandState(STANDSTATE_STAND);
	}
	else
	{
		creature->Unroot();
		creature->SetStandState(STANDSTATE_STAND);
	}
}

void AlteracValley::MoveAirSupportCommanderToEscortNode(uint32 team, uint32 fleet, Creature* commander)
{
	if(team > 1 || fleet > 2 || commander == NULL || commander->GetAIInterface() == NULL)
		return;

	uint32 count = 0;
	const AVAirSupportWaypoint* route = AVGetAirSupportRoute(team, fleet, count);
	if(route == NULL || count == 0)
		return;

	uint32 node = m_teamAirSupportEscortNode[team][fleet];
	if(node >= count)
		node = count - 1;

	commander->GetAIInterface()->MoveTo(
		route[node].x,
		route[node].y,
		route[node].z,
		route[node].o);
}

bool AlteracValley::AdvanceAirSupportEscort(uint32 team, uint32 fleet, Creature* commander)
{
	if(team > 1 || fleet > 2 || commander == NULL)
		return false;

	uint32 count = 0;
	const AVAirSupportWaypoint* route = AVGetAirSupportRoute(team, fleet, count);
	if(route == NULL || count == 0)
		return false;

	uint32& node = m_teamAirSupportEscortNode[team][fleet];
	if(node >= count)
		node = count - 1;

	if((node + 1) >= count)
		return true;

	++node;
	MoveAirSupportCommanderToEscortNode(team, fleet, commander);
	return false;
}

void AlteracValley::StartAirSupportEscort(uint32 team, uint32 fleet, Creature* commander)
{
	if(team > 1 || fleet > 2 || commander == NULL || m_teamAirSupportEscorting[team][fleet] || m_teamAirSupportReturned[team][fleet])
		return;

	m_teamAirSupportEscorting[team][fleet] = true;
	m_teamAirSupportReturned[team][fleet] = false;
	m_teamAirSupportEscortNode[team][fleet] = 0;

	ConfigureAirSupportCommander(commander, team, fleet, false, false);
	MoveAirSupportCommanderToEscortNode(team, fleet, commander);

	SendProgressMessage(team, "Wing Commander %s has been rescued and is making for home!",
		AVGetAirSupportFleetName(team, fleet));

	sLog.outDebug("AV air support escort start: team=%u fleet=%u commander=%s",
		team, fleet, AVGetAirSupportFleetName(team, fleet));
}

void AlteracValley::CompleteAirSupportEscort(uint32 team, uint32 fleet, Creature* commander)
{
	if(team > 1 || fleet > 2 || commander == NULL)
		return;

	m_teamAirSupportEscorting[team][fleet] = false;
	m_teamAirSupportReturned[team][fleet] = true;
	m_teamAirSupportEscortNode[team][fleet] = 0;

	ConfigureAirSupportCommander(commander, team, fleet, false, true);

	if(commander->GetAIInterface() != NULL)
		commander->GetAIInterface()->StopMovement(0);


	SendProgressMessage(team, "Wing Commander %s has returned home and can now support air operations.",
		AVGetAirSupportFleetName(team, fleet));

	sLog.outDebug("AV air support escort complete: team=%u fleet=%u commander=%s",
		team, fleet, AVGetAirSupportFleetName(team, fleet));
}

void AlteracValley::UpdateAirSupportCommanders()
{
	if(m_mapMgr == NULL || !m_started || m_ended)
		return;

	for(uint32 team = 0; team < 2; ++team)
	{
		for(uint32 fleet = 0; fleet < 3; ++fleet)
		{
			Creature* commander = FindAirSupportCommander(team, fleet);
			if(commander == NULL || !commander->IsInWorld())
				continue;

			if(commander->isDead())
			{
				if(m_teamAirSupportEscorting[team][fleet])
				{
					m_teamAirSupportEscorting[team][fleet] = false;
					SendProgressMessage(team, "Wing Commander %s was slain before reaching home.",
						AVGetAirSupportFleetName(team, fleet));
				}
				continue;
			}

			if(m_teamAirSupportReturned[team][fleet])
			{
				ConfigureAirSupportCommander(commander, team, fleet, false, true);
				continue;
			}

			if(m_teamAirSupportEscorting[team][fleet])
			{
				uint32 count = 0;
				const AVAirSupportWaypoint* route = AVGetAirSupportRoute(team, fleet, count);
				if(route == NULL || count == 0)
				{
					CompleteAirSupportEscort(team, fleet, commander);
					continue;
				}

				ConfigureAirSupportCommander(commander, team, fleet, false, false);
				uint32 node = m_teamAirSupportEscortNode[team][fleet];
				if(node >= count)
					node = count - 1;

				const float dx = commander->GetPositionX() - route[node].x;
				const float dy = commander->GetPositionY() - route[node].y;
				const float dz = commander->GetPositionZ() - route[node].z;
				if(((dx * dx) + (dy * dy) + (dz * dz)) <= (route[node].radius * route[node].radius))
				{
					if(AdvanceAirSupportEscort(team, fleet, commander))
						CompleteAirSupportEscort(team, fleet, commander);
				}

				continue;
			}

			ConfigureAirSupportCommander(commander, team, fleet, true, false);
		}
	}
}

void AlteracValley::ClearAirSupportStrikeVisuals(uint32 team, uint32 fleet)
{
	if(team > 1 || fleet > 2)
		return;

	vector<GameObject*>& visuals = m_teamAirSupportStrikeVisuals[team][fleet];
	for(vector<GameObject*>::iterator itr = visuals.begin(); itr != visuals.end(); ++itr)
	{
		GameObject* go = *itr;
		if(go == NULL)
			continue;

		if(go->IsInWorld())
			go->RemoveFromWorld(true);

		delete go;
	}

	visuals.clear();
}

void AlteracValley::ClearAirSupportStrikeRiders(uint32 team, uint32 fleet)
{
	if(team > 1 || fleet > 2)
		return;

	vector<Creature*>& riders = m_teamAirSupportStrikeRiders[team][fleet];
	for(vector<Creature*>::iterator itr = riders.begin(); itr != riders.end(); ++itr)
	{
		Creature* creature = *itr;
		if(creature == NULL)
			continue;

		if(creature->IsInWorld() && creature->GetMapMgr() != NULL)
			creature->RemoveFromWorld(false, false);

		delete creature;
	}

	riders.clear();
}

void AlteracValley::SpawnAirSupportStrikeVisuals(uint32 team, uint32 fleet)
{
	if(team > 1 || fleet > 2 || m_mapMgr == NULL)
		return;

	ClearAirSupportStrikeVisuals(team, fleet);

	const AVAirSupportStrikeProfile& profile = AVGetAirSupportStrikeProfile(team, fleet);
	for(uint32 i = 0; i < 4; ++i)
	{
		const float x = profile.x + AV_AIR_SUPPORT_STRIKE_VISUAL_OFFSETS[i].x;
		const float y = profile.y + AV_AIR_SUPPORT_STRIKE_VISUAL_OFFSETS[i].y;
		float z = profile.z + AV_AIR_SUPPORT_STRIKE_VISUAL_OFFSETS[i].z;

		float landZ = m_mapMgr->GetLandHeight(x, y);
		if(landZ != 0.0f)
			z = landZ;

		GameObject* go = SpawnGameObject(AV_GO_TOWER_BURNING,
			m_mapMgr->GetMapId(),
			x,
			y,
			z,
			AV_AIR_SUPPORT_STRIKE_VISUAL_OFFSETS[i].o,
			32,
			AV_FACTION_NEUTRAL,
			1.2f);
		if(go == NULL)
			continue;

		go->SetUInt32Value(GAMEOBJECT_STATE, 1);
		go->PushToWorld(m_mapMgr);
		m_teamAirSupportStrikeVisuals[team][fleet].push_back(go);
	}
}

void AlteracValley::SpawnAirSupportStrikeRiders(uint32 team, uint32 fleet)
{
	if(team > 1 || fleet > 2 || m_mapMgr == NULL)
		return;

	ClearAirSupportStrikeRiders(team, fleet);

	const uint32 entry = AV_AIR_SUPPORT_RIDER_ENTRY[team][fleet];
	CreatureProto* proto = CreatureProtoStorage.LookupEntry(entry);
	CreatureInfo* info = CreatureNameStorage.LookupEntry(entry);
	if(proto == NULL || info == NULL)
		return;

	const AVAirSupportStrikeProfile& profile = AVGetAirSupportStrikeProfile(team, fleet);
	const float passRadius = (profile.radius > 35.0f) ? 35.0f : profile.radius;
	float z = profile.z + AV_AIR_SUPPORT_RIDER_ALTITUDE;
	const bool reverse = ((m_teamAirSupportStrikePass[team][fleet] % 2) != 0);
	const float baseStartX = reverse ? (profile.x - passRadius) : (profile.x + passRadius);
	const float baseEndX   = reverse ? (profile.x + passRadius) : (profile.x - passRadius);
	const float o = reverse ? 0.0f : 3.14159f;

	for(uint32 i = 0; i < 3; ++i)
	{
		const float y = profile.y + AV_AIR_SUPPORT_RIDER_LATERAL_OFFSETS[i];

		CreatureSpawn* sp = new CreatureSpawn;
		sp->entry = entry;
		sp->form = 0;
		sp->id = 0;
		sp->movetype = 0;
		sp->x = baseStartX;
		sp->y = y;
		sp->z = z;
		sp->o = o;
		sp->emote_state = 0;
		sp->flags = 0;
		sp->factionid = AV_FACTION_NEUTRAL;
		sp->bytes = 0;
		sp->bytes2 = 0;
		sp->stand_state = 0;
		sp->channel_spell = 0;
		sp->channel_target_go = 0;
		sp->channel_target_creature = 0;

		Creature* rider = m_mapMgr->CreateCreature(entry);
		if(rider == NULL)
		{
			delete sp;
			continue;
		}

		rider->Load(sp, (uint32)NULL, NULL);
		rider->spawnid = 0;
		rider->m_spawn = 0;
		delete sp;

		rider->SetUInt32Value(UNIT_FIELD_FACTIONTEMPLATE, AV_FACTION_NEUTRAL);
		rider->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
		rider->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_2);

		if(rider->GetAIInterface() != NULL)
		{
			rider->GetAIInterface()->SetAllowedToEnterCombat(false);
			rider->GetAIInterface()->disable_melee = true;
			rider->GetAIInterface()->disable_targeting = true;
			rider->GetAIInterface()->m_canMove = true;
		}

		rider->PushToWorld(m_mapMgr);

		if(rider->GetAIInterface() != NULL)
			rider->GetAIInterface()->MoveTo(baseEndX, y, z, o);

		m_teamAirSupportStrikeRiders[team][fleet].push_back(rider);
	}

	++m_teamAirSupportStrikePass[team][fleet];
}

bool AlteracValley::IsAirSupportCommanderRescued(uint32 team, uint32 fleet)
{
	if (team > 1 || fleet > 2)
		return false;

	return m_teamAirSupportReturned[team][fleet];
}

bool AlteracValley::HandleAirSupportRescue(Player* plr, Creature* commander)
{
	if(plr == NULL || commander == NULL || m_mapMgr == NULL || !m_started || m_ended)
		return false;

	if(plr->m_bg != this || plr->GetMapMgr() != m_mapMgr)
		return false;

	uint32 team = 0, fleet = 0;
	if(!AVGetAirSupportCommanderByEntry(commander->GetEntry(), team, fleet))
		return false;

	if(plr->m_bgTeam != team)
		return false;

	if(commander->isDead())
		return false;

	if (m_teamAirSupportEscorting[team][fleet] || m_teamAirSupportReturned[team][fleet])
	{
		SendProgressMessage(team, "%s is already on the move or safely home.", AVGetAirSupportCommanderFullName(team, fleet));
		return false;
	}

	// Captive interaction only: kneeling, rooted, passive commander at prison site.
	if(commander->GetStandState() != STANDSTATE_KNEEL)
		return false;

	StartAirSupportEscort(team, fleet, commander);

	sLog.outDebug("AV air support rescue: player=%s team=%u fleet=%u commanderEntry=%u",
		plr->GetName(),
		team,
		fleet,
		commander->GetEntry());
	return true;
}

bool AlteracValley::HandleAirSupportDeploy(Player* plr, Creature* commander)
{
	if(plr == NULL || commander == NULL || m_mapMgr == NULL || !m_started || m_ended)
		return false;

	if(plr->m_bg != this || plr->GetMapMgr() != m_mapMgr)
		return false;

	uint32 team = 0, fleet = 0;
	if(!AVGetAirSupportCommanderByEntry(commander->GetEntry(), team, fleet))
		return false;

	if(plr->m_bgTeam != team)
		return false;

	if(commander->isDead())
		return false;

	if (!m_teamAirSupportReturned[team][fleet])
	{
		SendProgressMessage(team, "%s must make it home before that strike can be launched.",
			AVGetAirSupportCommanderFullName(team, fleet));
		return false;
	}

	if (!m_teamAirSupportReady[team][fleet])
	{
		const uint32 threshold = AVGetAirSupportReadyThreshold(fleet);
		const uint32 current = m_teamAirSupportTurnIns[team][fleet];
		const uint32 remaining = (current >= threshold) ? 0 : (threshold - current);
		SendProgressMessage(team, "%s still needs %u more supplies before the strike on %s can begin.",
			AVGetAirSupportCommanderFullName(team, fleet),
			remaining,
			AVGetAirSupportStrikeProfile(team, fleet).targetDescription);
		return false;
	}

	if (m_teamAirSupportStrikeActive[team][fleet])
	{
		SendProgressMessage(team, "%s is already carrying out a strike run.",
			AVGetAirSupportCommanderFullName(team, fleet));
		return false;
	}

	m_teamAirSupportReady[team][fleet] = false;
	m_teamAirSupportStrikeActive[team][fleet] = true;
	m_teamAirSupportStrikeTimeLeft[team][fleet] = AV_AIR_SUPPORT_STRIKE_DURATION_MS;
	m_teamAirSupportStrikePulse[team][fleet] = AV_AIR_SUPPORT_STRIKE_PULSE_MS;
	m_teamAirSupportTurnIns[team][fleet] = 0;
	m_teamAirSupportStrikePass[team][fleet] = 0;
	SpawnAirSupportStrikeVisuals(team, fleet);
	SpawnAirSupportStrikeRiders(team, fleet);

	SendProgressMessage(team, "%s has launched an air strike toward %s!",
		AVGetAirSupportStrikeName(team, fleet),
		AVGetAirSupportStrikeProfile(team, fleet).targetDescription);
	SendProgressMessage(team == 0 ? 1 : 0, "Enemy %s are inbound over %s!",
		AVGetAirSupportStrikeName(team, fleet),
		AVGetAirSupportStrikeProfile(team, fleet).targetDescription);

	sLog.outDebug("AV air strike deploy: player=%s team=%u fleet=%u strike=%s target=%s duration=%u pulseDamage=%u",
		plr->GetName(),
		team,
		fleet,
		AVGetAirSupportStrikeName(team, fleet),
		AVGetAirSupportStrikeProfile(team, fleet).targetDescription,
		AV_AIR_SUPPORT_STRIKE_DURATION_MS,
		AVGetAirSupportStrikeProfile(team, fleet).reinforcementDamagePerPulse);

	return true;
}

void AlteracValley::AddAirSupportTurnIn(uint32 team, uint32 fleet, uint32 questId)
{
	if(team > 1 || fleet > 2)
		return;

	const char* fleetName = AVGetAirSupportFleetName(team, fleet);
	if(!IsAirSupportCommanderRescued(team, fleet))
	{
		SendProgressMessage(team, "%s's wing commander must be rescued before that fleet can be supplied.", fleetName);
		sLog.outDebug("AV air support blocked: team=%u fleet=%u quest=%u commander=%s rescued=0",
			team, fleet, questId, fleetName);
		return;
	}

	if(m_teamAirSupportStrikeActive[team][fleet])
	{
		SendProgressMessage(team, "%s's air support is already on a strike run.", fleetName);
		return;
	}

	if(m_teamAirSupportReady[team][fleet])
	{
		SendProgressMessage(team, "%s's air support is already awaiting deployment orders.", fleetName);
		return;
	}

	++m_teamAirSupportTurnIns[team][fleet];
	const uint32 readyThreshold = AVGetAirSupportReadyThreshold(fleet);
	if(m_teamAirSupportTurnIns[team][fleet] > readyThreshold)
		m_teamAirSupportTurnIns[team][fleet] = readyThreshold;

	UpdateAirSupportStrikeState(team, fleet);

	if(!m_teamAirSupportReady[team][fleet])
	{
		const uint32 remaining = readyThreshold - m_teamAirSupportTurnIns[team][fleet];
		SendProgressMessage(team, "%s's fleet has received supplies (%u/%u). %u more are needed before the strike on %s is ready.",
			fleetName,
			m_teamAirSupportTurnIns[team][fleet],
			readyThreshold,
			remaining,
			AVGetAirSupportStrikeProfile(team, fleet).targetDescription);
	}

	sLog.outDebug("AV air support progress: team=%u fleet=%u quest=%u commander=%s turnins=%u rescued=1",
		team, fleet, questId, fleetName, m_teamAirSupportTurnIns[team][fleet]);
}

bool AlteracValley::IsAirSupportReady(uint32 team, uint32 fleet) const
{
	if(team > 1 || fleet > 2)
		return false;

	return m_teamAirSupportReady[team][fleet];
}

void AlteracValley::UpdateAirSupportStrikeState(uint32 team, uint32 fleet)
{
	if(team > 1 || fleet > 2)
		return;

	if(m_teamAirSupportStrikeActive[team][fleet])
		return;

	const bool ready = (m_teamAirSupportReturned[team][fleet] &&
		m_teamAirSupportTurnIns[team][fleet] >= AVGetAirSupportReadyThreshold(fleet));

	if(ready == m_teamAirSupportReady[team][fleet])
		return;

	m_teamAirSupportReady[team][fleet] = ready;

	if(ready)
	{
		SendProgressMessage(team, "%s is fully supplied and ready to strike %s.",
			AVGetAirSupportStrikeName(team, fleet),
			AVGetAirSupportStrikeProfile(team, fleet).targetDescription);
	}
}

void AlteracValley::UpdateAirSupportStrikes()
{
	if(!m_started || m_ended)
		return;

	for(uint32 team = 0; team < 2; ++team)
	{
		const uint32 enemyTeam = (team == 0) ? 1 : 0;

		for(uint32 fleet = 0; fleet < 3; ++fleet)
		{
			if(!m_teamAirSupportStrikeActive[team][fleet])
				continue;

			if(m_teamAirSupportStrikeTimeLeft[team][fleet] <= 1000)
			{
				m_teamAirSupportStrikeActive[team][fleet] = false;
				m_teamAirSupportStrikeTimeLeft[team][fleet] = 0;
				m_teamAirSupportStrikePulse[team][fleet] = 0;
				ClearAirSupportStrikeVisuals(team, fleet);
				ClearAirSupportStrikeRiders(team, fleet);
				SendProgressMessage(team, "%s has completed its strike run.", AVGetAirSupportStrikeName(team, fleet));
				continue;
			}

			m_teamAirSupportStrikeTimeLeft[team][fleet] -= 1000;

			if(m_teamAirSupportStrikePulse[team][fleet] <= 1000)
			{
				m_teamAirSupportStrikePulse[team][fleet] = AV_AIR_SUPPORT_STRIKE_PULSE_MS;
				ModifyReinforcements(enemyTeam, -(int32)AVGetAirSupportStrikeProfile(team, fleet).reinforcementDamagePerPulse);

				// Refresh the strike visuals on each pulse so late-arriving players see the bombardment.
				SpawnAirSupportStrikeVisuals(team, fleet);
				SpawnAirSupportStrikeRiders(team, fleet);
 
				sLog.outDebug("AV air strike pulse: team=%u fleet=%u strike=%s target=%s enemyTeam=%u remainingMs=%u pulseDamage=%u",
					team,
					fleet,
					AVGetAirSupportStrikeName(team, fleet),
					AVGetAirSupportStrikeProfile(team, fleet).targetDescription,
					enemyTeam,
					m_teamAirSupportStrikeTimeLeft[team][fleet],
					AVGetAirSupportStrikeProfile(team, fleet).reinforcementDamagePerPulse);
			}
			else
			{
				m_teamAirSupportStrikePulse[team][fleet] -= 1000;
			}
		}
	}
}

void AlteracValley::ApplyArmorTierToDefender(Creature* creature, uint32 team, bool restoreFullHealth)
{
	if(creature == NULL || team > 1 || !AVIsArmorTierDefenderEntry(creature->GetEntry()))
		return;

	CreatureProto* proto = creature->proto;
	if(proto == NULL)
		proto = CreatureProtoStorage.LookupEntry(creature->GetEntry());
	if(proto == NULL)
		return;

	const uint32 entry = creature->GetEntry();
	map<uint32, uint32>::iterator baseHealthItr = m_defenderBaseHealth.find(entry);
	if(baseHealthItr == m_defenderBaseHealth.end())
	{
		uint32 protoBaseHealth = proto->MaxHealth;

		// AV bunker marshals and tower warmasters are dynamically spawned by the battleground
		// and must always scale from immutable proto max health, not from live instance health.
		if(protoBaseHealth == 0)
			protoBaseHealth = proto->MinHealth;
		if(protoBaseHealth == 0)
			protoBaseHealth = creature->GetUInt32Value(UNIT_FIELD_MAXHEALTH);

		baseHealthItr = m_defenderBaseHealth.insert(make_pair(entry, protoBaseHealth)).first;
	}

	const uint32 tier = (m_teamArmorTier[team] > 3) ? 3 : m_teamArmorTier[team];
	uint32 baseHealth = baseHealthItr->second;
	if(baseHealth == 0)
	{
		baseHealth = proto->MaxHealth;
		if(baseHealth == 0)
			baseHealth = proto->MinHealth;
		if(baseHealth == 0)
			baseHealth = creature->GetUInt32Value(UNIT_FIELD_MAXHEALTH);
	}

	const uint32 oldMaxHealth = creature->GetUInt32Value(UNIT_FIELD_MAXHEALTH);
	const uint32 oldHealth = creature->GetUInt32Value(UNIT_FIELD_HEALTH);
	const uint32 scaledHealth = AVScaleStatByTier(baseHealth, tier);
	const uint32 scaledArmor = AVScaleStatByTier(proto->Resistances[0], tier);
	const float scaledMinDamage = AVScaleDamageByTier(proto->MinDamage, tier);
	const float scaledMaxDamage = AVScaleDamageByTier(proto->MaxDamage, tier);
	const float scaledMinRangedDamage = AVScaleDamageByTier(proto->RangedMinDamage, tier);
	const float scaledMaxRangedDamage = AVScaleDamageByTier(proto->RangedMaxDamage, tier);

	creature->BaseResistance[0] = scaledArmor;
	creature->SetUInt32Value(UNIT_FIELD_RESISTANCES, scaledArmor);

	for(uint32 school = 1; school < 7; ++school)
	{
		creature->BaseResistance[school] = proto->Resistances[school];
		creature->SetUInt32Value(UNIT_FIELD_RESISTANCES + school, proto->Resistances[school]);
	}

	creature->BaseDamage[0] = scaledMinDamage;
	creature->BaseDamage[1] = scaledMaxDamage;
	creature->BaseRangedDamage[0] = scaledMinRangedDamage;
	creature->BaseRangedDamage[1] = scaledMaxRangedDamage;
	creature->SetFloatValue(UNIT_FIELD_MINDAMAGE, scaledMinDamage);
	creature->SetFloatValue(UNIT_FIELD_MAXDAMAGE, scaledMaxDamage);
	creature->SetFloatValue(UNIT_FIELD_MINRANGEDDAMAGE, scaledMinRangedDamage);
	creature->SetFloatValue(UNIT_FIELD_MAXRANGEDDAMAGE, scaledMaxRangedDamage);

	creature->SetUInt32Value(UNIT_FIELD_MAXHEALTH, scaledHealth);

	if(creature->isAlive())
	{
		if(restoreFullHealth || oldMaxHealth == 0 || oldHealth == 0)
		{
			creature->SetUInt32Value(UNIT_FIELD_HEALTH, scaledHealth);
		}
		else
		{
			uint32 newHealth = scaledHealth;
			if(oldHealth < oldMaxHealth)
			{
				newHealth = (uint32)(((uint64)scaledHealth * (uint64)oldHealth) / (uint64)oldMaxHealth);
				if(newHealth == 0)
					newHealth = 1;
				if(newHealth > scaledHealth)
					newHealth = scaledHealth;
			}

			creature->SetUInt32Value(UNIT_FIELD_HEALTH, newHealth);
		}
	}

	sLog.outDebug("AV defender scale: entry=%u team=%u tier=%u baseHealth=%u oldHealth=%u oldMax=%u scaledHealth=%u full=%u scaling=%s",
		entry,
		team,
		tier,
		baseHealth,
		oldHealth,
		oldMaxHealth,
		scaledHealth,
		restoreFullHealth ? 1 : 0,
		AV_ARMOR_TIER_SCALING_MODEL);
}

uint32 AlteracValley::RefreshArmorTierDefenders(uint32 team)
{
	if(team > 1 || m_mapMgr == NULL)
		return 0;

	set<Creature*> defenders;

	for(uint32 i = 0; i < AV_OBJECTIVE_COUNT; ++i)
	{
		AVObjectiveState& state = m_objectiveStates[i];
		if(state.linkedUnit != NULL && AVGetArmorTierDefenderTeam(state.linkedUnit->GetEntry()) == static_cast<int32>(team))
			defenders.insert(state.linkedUnit);

		vector<Creature*>& guards = (team == 0) ? state.allianceGuards : state.hordeGuards;
		for(vector<Creature*>::iterator itr = guards.begin(); itr != guards.end(); ++itr)
		{
			if(*itr != NULL && AVGetArmorTierDefenderTeam((*itr)->GetEntry()) == static_cast<int32>(team))
				defenders.insert(*itr);
		}
	}

	for(CreatureSqlIdMap::iterator itr = m_mapMgr->_sqlids_creatures.begin(); itr != m_mapMgr->_sqlids_creatures.end(); ++itr)
	{
		Creature* creature = itr->second;
		if(creature == NULL || AVGetArmorTierDefenderTeam(creature->GetEntry()) != static_cast<int32>(team))
			continue;

		defenders.insert(creature);
	}

	uint32 refreshed = 0;
	for(set<Creature*>::iterator itr = defenders.begin(); itr != defenders.end(); ++itr)
	{
		Creature* creature = *itr;
		if(creature == NULL)
			continue;

		ApplyArmorTierToDefender(creature, team);
		++refreshed;
	}

	sLog.outDebug("AV armor refresh: team=%u tier=%u refreshed=%u entries=[%s] scaling=%s",
		team, m_teamArmorTier[team], refreshed, AV_ARMOR_REFRESH_ENTRIES, AV_ARMOR_TIER_SCALING_MODEL);
	return refreshed;
}

void AlteracValley::UpdateArmorTier(uint32 team)
{
	if(team > 1)
		return;

	const uint32 oldTier = m_teamArmorTier[team];
	uint32 newTier = 0;
	if(m_teamArmorScraps[team] >= AV_ARMOR_TIER3_THRESHOLD)
		newTier = 3;
	else if(m_teamArmorScraps[team] >= AV_ARMOR_TIER2_THRESHOLD)
		newTier = 2;
	else if(m_teamArmorScraps[team] >= AV_ARMOR_TIER1_THRESHOLD)
		newTier = 1;

	if(newTier == oldTier)
		return;

	m_teamArmorTier[team] = newTier;
	sLog.outDebug("AV armor tier changed: team=%u scraps=%u tier=%u->%u entries=[%s] scaling=%s",
		team, m_teamArmorScraps[team], oldTier, newTier, AV_ARMOR_REFRESH_ENTRIES, AV_ARMOR_TIER_SCALING_MODEL);

	switch(newTier)
	{
	case 1:
		SendProgressMessage(team, "%s armor supplies are improving.", team == 0 ? "Alliance" : "Horde");
		break;
	case 2:
		SendProgressMessage(team, "%s armor supplies have been significantly reinforced.", team == 0 ? "Alliance" : "Horde");
		break;
	case 3:
		SendProgressMessage(team, "%s troops are now fully supplied with armor.", team == 0 ? "Alliance" : "Horde");
		break;
	default:
		break;
	}

	RefreshArmorTierDefenders(team);
}

void AlteracValley::AddScraps(uint32 team, uint32 amount)
{
	if(team > 1 || amount == 0)
		return;

	m_teamArmorScraps[team] += amount;
	UpdateArmorTier(team);
}

bool AlteracValley::IsElementalSummonReady(uint32 team) const
{
	if(team > 1)
		return false;

	return m_teamElementalReady[team];
}

void AlteracValley::UpdateElementalSummonState(uint32 team)
{
	if(team > 1)
		return;

	const uint32 current = (team == 0) ? m_teamStormCrystals[team] : m_teamBlood[team];
	const bool ready = (current >= AV_ELEMENTAL_SUMMON_THRESHOLD);
	if(ready == m_teamElementalReady[team])
		return;

	m_teamElementalReady[team] = ready;

	if(ready)
	{
		SendProgressMessage(team,
			"%s have gathered enough %s to prepare the ritual for %s.",
			team == 0 ? "Alliance" : "Horde",
			AVGetElementalResourceName(team),
			AVGetElementalName(team));

		sLog.outDebug("AV elemental ritual ready: team=%u elemental=%s amount=%u threshold=%u",
			team,
			AVGetElementalName(team),
			current,
			AV_ELEMENTAL_SUMMON_THRESHOLD);
	}
}

void AlteracValley::AddBlood(uint32 team, uint32 amount)
{
	if(team > 1 || amount == 0)
		return;

	if(team != 1)
	{
		sLog.outDebug("AV blood ignored: team=%u amount=%u (Lokholar is Horde-only)", team, amount);
		return;
	}

	m_teamBlood[team] += amount;

	if(m_teamBlood[team] > AV_ELEMENTAL_SUMMON_THRESHOLD)
		m_teamBlood[team] = AV_ELEMENTAL_SUMMON_THRESHOLD;

	UpdateElementalSummonState(team);
}

void AlteracValley::AddStormCrystals(uint32 team, uint32 amount)
{
	if(team > 1 || amount == 0)
		return;

	if(team != 0)
	{
		sLog.outDebug("AV storm crystals ignored: team=%u amount=%u (Ivus is Alliance-only)", team, amount);
		return;
	}

	m_teamStormCrystals[team] += amount;

	if(m_teamStormCrystals[team] > AV_ELEMENTAL_SUMMON_THRESHOLD)
		m_teamStormCrystals[team] = AV_ELEMENTAL_SUMMON_THRESHOLD;

	UpdateElementalSummonState(team);
}

void AlteracValley::HandleQuestTurnIn(Player* plr, uint32 questId)
{
	if(plr == NULL || plr->m_bg != this || plr->GetMapMgr() != m_mapMgr || m_ended)
		return;

	switch(questId)
	{
	case AV_QUEST_ARMOR_SCRAPS:
	case AV_QUEST_MORE_ARMOR_SCRAPS:
		sLog.outDebug("AV scrap turn-in: player=%s team=%u quest=%u amount=%u",
			plr->GetName(), plr->m_bgTeam, questId, AV_SCRAPS_PER_TURNIN);
		AddScraps(plr->m_bgTeam, AV_SCRAPS_PER_TURNIN);
		break;

	case AV_QUEST_MORE_BOOTY:
	case AV_QUEST_ENEMY_BOOTY:
		break;

	case AV_QUEST_A_GALLON_OF_BLOOD:
	case AV_QUEST_LOKHOLAR_THE_ICE_LORD:
		if(plr->m_bgTeam == 1)
		{
			sLog.outDebug("AV blood turn-in: player=%s team=%u quest=%u amount=%u",
				plr->GetName(), plr->m_bgTeam, questId, AV_BLOOD_PER_TURNIN);
			AddBlood(1, AV_BLOOD_PER_TURNIN);
		}
		else
		{
			sLog.outDebug("AV blood turn-in ignored: player=%s team=%u quest=%u",
				plr->GetName(), plr->m_bgTeam, questId);
		}
		break;

	case AV_QUEST_CRYSTAL_CLUSTER:
	case AV_QUEST_IVUS_THE_FOREST_LORD:
		if(plr->m_bgTeam == 0)
		{
			sLog.outDebug("AV storm crystal turn-in: player=%s team=%u quest=%u amount=%u",
				plr->GetName(), plr->m_bgTeam, questId, AV_STORM_CRYSTALS_PER_TURNIN);
			AddStormCrystals(0, AV_STORM_CRYSTALS_PER_TURNIN);
		}
		else
		{
			sLog.outDebug("AV storm crystal turn-in ignored: player=%s team=%u quest=%u",
				plr->GetName(), plr->m_bgTeam, questId);
		}
		break;

	case AV_QUEST_CALL_OF_AIR_SLIDORES_FLEET:
	case AV_QUEST_CALL_OF_AIR_VIPORES_FLEET:
	case AV_QUEST_CALL_OF_AIR_ICHMANS_FLEET:
	case AV_QUEST_CALL_OF_AIR_GUSES_FLEET:
	case AV_QUEST_CALL_OF_AIR_JEZTORS_FLEET:
	case AV_QUEST_CALL_OF_AIR_MULVERICKS_FLEET:
	{
		uint32 airTeam = 0, airFleet = 0;
		if(AVGetAirSupportQuestInfo(questId, airTeam, airFleet) && airTeam == plr->m_bgTeam)
			AddAirSupportTurnIn(airTeam, airFleet, questId);
		else
			sLog.outDebug("AV air support ignored: player=%s team=%u quest=%u", plr->GetName(), plr->m_bgTeam, questId);
		break;
	}

	default:
		break;
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

void AlteracValley::HookGenerateLoot(Player* plr, Corpse* pCorpse)
{
	if(plr == NULL || pCorpse == NULL)
		return;

	const AVLoot* loot_ptr = &g_avLoot[0];
	while(loot_ptr->ItemId != 0)
	{
		// In this code path plr is the corpse owner, not the looter.
		// Match loot to the dead player's faction, as Summit did.
		const int8 dropFaction = (plr->GetTeam() == 0) ? 1 : 0;
		if (loot_ptr->Faction == -1 || loot_ptr->Faction == dropFaction)
		{
			if(Rand(loot_ptr->Chance * sWorld.getRate(RATE_DROP0)))
			{
				ItemPrototype* pProto = ItemPrototypeStorage.LookupEntry(loot_ptr->ItemId);
				if(pProto != NULL)
				{
					__LootItem li;
					li.ffa_loot = 1;
					li.item.displayid = pProto->DisplayInfoID;
					li.item.itemproto = pProto;
					li.iItemsCount = (loot_ptr->MinCount != loot_ptr->MaxCount) ?
						(RandomUInt(loot_ptr->MaxCount - loot_ptr->MinCount) + loot_ptr->MinCount) :
						loot_ptr->MinCount;
					li.iRandomProperty = NULL;
					li.iRandomSuffix = NULL;
					li.roll = NULL;
					li.passed = false;
					pCorpse->loot.items.push_back(li);
				}
			}
		}

		++loot_ptr;
	}

	// Keep AV corpses from being a money faucet.
	// Summit added gold here, but quest items are the real requirement.
	pCorpse->loot.gold = 0;
}
