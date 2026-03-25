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

#define BASE_RESOURCES_GAIN 10
#define RESOURCES_WARNING_THRESHOLD 1800
#define RESOURCES_WINVAL 2000
#define RESOURCES_TO_GAIN_BH 200
#define BASE_BH_GAIN 14
uint32 buffentrys[3] = {180380,180362,180146};
// AB define's
#define AB_CAPTURED_STABLES_ALLIANCE		0x6E7 //1767
#define AB_CAPTURED_STABLES_HORDE		   0x6E8 //1768
#define AB_CAPTURING_STABLES_ALLIANCE	   0x6E9 //1769
#define AB_CAPTURING_STABLES_HORDE		  0x6EA //1770
// 0x6EB is unknown
#define AB_CAPTURED_FARM_ALLIANCE		   0x6EC //1772 // 1 is captured by the alliance
#define AB_CAPTURED_FARM_HORDE			  0x6ED // 1773 / 1 is captured by the horde
#define AB_CAPTURING_FARM_ALLIANCE		  0x6EE // 1774 1 is capturing by the alliance
#define AB_CAPTURING_FARM_HORDE			 0x6EF // 1775 1 is capturing by the horde

#define AB_CAPTURED_BLACKSMITH_ALLIANCE	 0x6F6 // 1782
#define AB_CAPTURED_BLACKSMITH_HORDE		0x6F7 //1783
#define AB_CAPTURING_BLACKSMITH_ALLIANCE	0x6F8 //1784
#define AB_CAPTURING_BLACKSMITH_HORDE	   0x6F9 //1785
// 0x6FA is unknown
#define AB_CAPTURED_GOLDMINE_ALLIANCE	   0x6FB //1787
#define AB_CAPTURED_GOLDMINE_HORDE		  0x6FC//1788
#define AB_CAPTURING_GOLDMINE_ALLIANCE	  0x6FD//1789
#define AB_CAPTURING_GOLDMINE_HORDE		 0x6FE//1790
// 0x6FF is unknown
#define AB_CAPTURED_LUMBERMILL_ALLIANCE	 0x700//1792
#define AB_CAPTURED_LUMBERMILL_HORDE		0x701//1793
#define AB_CAPTURING_LUMBERMILL_ALLIANCE	0x702//1794
#define AB_CAPTURING_LUMBERMILL_HORDE	   0x703//1795

#define AB_SHOW_STABLE_ICON				 0x732//1842
#define AB_SHOW_GOLDMINE_ICON			   0x733//1843
#define AB_SHOW_LUMBERMILL_ICON			 0x734//1844
#define AB_SHOW_FARM_ICON				   0x735//1845
#define AB_SHOW_BACKSMITH_ICON			  0x736//1846

/* AB Battleground Data */

	static float GraveyardLocations[AB_NUM_CONTROL_POINTS][3] = {
		{ 1201.869507f, 1163.130615f, -56.285969f },												// STABLES
		{ 834.726379f, 784.978699f, -57.081944f },													// FARM
		{ 1016.588318f, 955.184692f, -42.828693f },													// BLACKSMITH
		{ 1211.523682f, 781.556946f, -82.709511f },													// MINE
		{ 772.755676f, 1213.113770f, 15.797392f },													// LUMBERMILL
	};

	static float NoBaseGYLocations[2][3] = {
		{ 1354.699951f, 1270.270020f, -11.129100f },												// ALLIANCE
		{ 713.710022f, 638.364014f, -10.599900f },													// HORDE
	};

	static const char * ControlPointNames[AB_NUM_CONTROL_POINTS] = {
		"Stable",																					// STABLE
		"Farm",																						// FARM
		"Blacksmith",																				// BLACKSMITH
		"Mine",																						// MINE
		"Lumber Mill",																				// LUMBERMILL
	};

	static uint32 ControlPointGoIds[AB_NUM_CONTROL_POINTS][AB_NUM_SPAWN_TYPES] = {
		  // NEUTRAL    ALLIANCE-ATTACK    HORDE-ATTACK    ALLIANCE-CONTROLLED    HORDE_CONTROLLED
		{ 180087,       180085,            180086,         180076,                180078 },			// STABLE	
		{ 180089,       180085,            180086,         180076,                180078 },			// FARM
		{ 180088,       180085,            180086,         180076,                180078 },			// BLACKSMITH
		{ 180091,       180085,            180086,         180076,                180078 },			// MINE
		{ 180090,       180085,            180086,         180076,                180078 },			// LUMBERMILL
	};

	static float ControlPointCoordinates[AB_NUM_CONTROL_POINTS][4] = {
		{ 1166.779541f, 1200.147583f, -56.701763f, -2.251474f },									// STABLE
		{ 806.2484741f, 874.2167358f, -55.9936981f, 0.8377580f },									// FARM
		{ 977.0503540f, 1046.5208740f, -44.8276138f, 0.5410520f },									// BLACKSMITH
		{ 1146.9224854f, 848.1899414f, -110.9200210f, 2.4260077f },									// MINE
		{ 856.141907f, 1148.902100f, 11.184692f, -2.303835f },										// LUMBERMILL
	};

	static float ControlPointRotations[AB_NUM_CONTROL_POINTS][2] = {
		{ 0.9025853f, -0.4305111f },																// STABLE
		{ 0.4067366f, 0.9135454f },																	// FARM
		{ 0.2672384f, 0.9636304f },																	// BLACKSMITH
		{ 0.9366722f, 0.3502073f },																	// MINE
		{ 0.9135455f, -0.4067366f },																// LUMBERMILL
	};

	static float BuffCoordinates[AB_NUM_CONTROL_POINTS][4] = {
		{ 1185.56616210938f, 1184.62854003906f, -56.3632850646973f, 2.30383467674255f },			// STABLE
		{ 990.113098144531f, 1008.73028564453f, -42.6032752990723f, 0.820304811000824f },			// FARM
		{ 816.906799f, 842.339844f, -56.538746f, 3.272740f },										// BLACKSMITH
		{ 808.846252441406f, 1185.41748046875f, 11.9216051101685f, -0.663225054740906f },			// MINE
		{ 1147.09057617188f, 816.836242675781f, -98.3989562988281f, -0.226892784237862f },			// LUMBERMILL
	};

	static float BuffRotations[AB_NUM_CONTROL_POINTS][2] = {
		{ 0.913545489311218f, 0.406736612319946f },													// STABLE
		{ 0.39874908328056f, 0.917060077190399f },													// FARM
		{ 0.913545489311218f, 0.406736612319946f },													// BLACKSMITH
		{ 0.325568109750748f, -0.945518612861633f },												// MINE
		{ 0.113203197717667f, -0.993571877479553f },												// LUMBERMILL
	};

	static uint32 AssaultFields[AB_NUM_CONTROL_POINTS][2] = {
		{ AB_CAPTURING_STABLES_ALLIANCE, AB_CAPTURING_STABLES_HORDE },								// STABLE
		{ AB_CAPTURING_FARM_ALLIANCE, AB_CAPTURING_FARM_HORDE },									// FARM
		{ AB_CAPTURING_BLACKSMITH_ALLIANCE, AB_CAPTURING_BLACKSMITH_HORDE },						// BLACKSMITH
		{ AB_CAPTURING_GOLDMINE_ALLIANCE, AB_CAPTURING_GOLDMINE_HORDE },							// MINE
		{ AB_CAPTURING_LUMBERMILL_ALLIANCE, AB_CAPTURING_LUMBERMILL_HORDE },						// LUMBERMILL
	};

	static uint32 OwnedFields[AB_NUM_CONTROL_POINTS][2] = {
		{ AB_CAPTURED_STABLES_ALLIANCE, AB_CAPTURED_STABLES_HORDE },								// STABLE
		{ AB_CAPTURED_FARM_ALLIANCE, AB_CAPTURED_FARM_HORDE },										// FARM
		{ AB_CAPTURED_BLACKSMITH_ALLIANCE, AB_CAPTURED_BLACKSMITH_HORDE },							// BLACKSMITH
		{ AB_CAPTURED_GOLDMINE_ALLIANCE, AB_CAPTURED_GOLDMINE_HORDE },								// MINE
		{ AB_CAPTURED_LUMBERMILL_ALLIANCE, AB_CAPTURED_LUMBERMILL_HORDE },							// LUMBERMILL
	};

	static uint32 NeutralFields[AB_NUM_CONTROL_POINTS] = {
		AB_SHOW_STABLE_ICON,
		AB_SHOW_FARM_ICON,
		AB_SHOW_BACKSMITH_ICON,
		AB_SHOW_GOLDMINE_ICON,
		AB_SHOW_LUMBERMILL_ICON,
	};

	static uint32 ResourceUpdateIntervals[6] = {
		0,
		12000,
		9000,
		6000,
		3000,
		1000,
	};

static uint32 PointBonusPerUpdate[6] = {
		0,
		10,
		10,
		10,
		10,
		30,
	};

/* End BG Data */

struct ABControlPointCreatureSpawn
{
	uint32 entry;
	float x, y, z, o;
	uint32 factionId;
	uint32 emoteState;
	uint8 moveType;
};

static const uint32 AB_DB_CREATURE_SQLID_BEGIN = 5290001;
static const uint32 AB_DB_CREATURE_SQLID_END = 5290110;
static const uint32 AB_DB_BANNER_SQLID_BEGIN = 5290005;
static const uint32 AB_DB_BANNER_SQLID_END = 5290029;
static const uint32 AB_DB_GATE_SQLID_BEGIN = 5290253;
static const uint32 AB_DB_GATE_SQLID_END = 5290254;
static const uint32 AB_DB_BUFF_SQLID_BEGIN = 5290300;
static const uint32 AB_DB_BUFF_SQLID_END = 5290314;

static const ABControlPointCreatureSpawn AB_STABLE_ALLIANCE_SPAWNS[] =
{
	{ 15086, 1157.72f, 1162.02f, -56.3494f, 4.68379f, 1577, 0, 2 },
	{ 15086, 1207.82f, 1198.78f, -56.1779f, 2.86234f, 1577, 69, 0 },
	{ 15086, 1184.96f, 1200.12f, -56.3163f, 0.733038f, 1577, 0, 0 },
	{ 15107, 1153.16f, 1154.29f, -56.4169f, 2.5387f, 190, 0, 0 },
	{ 15107, 1157.58f, 1172.96f, -56.2799f, 2.84489f, 190, 0, 0 },
	{ 15107, 1187.5f, 1181.29f, -55.9973f, 2.40855f, 190, 0, 0 },
	{ 15107, 1158.07f, 1165.48f, -56.2712f, 4.56288f, 190, 0, 0 },
	{ 15107, 1186.39f, 1190.51f, -56.009f, 2.35619f, 190, 0, 0 },
	{ 15107, 1169.2f, 1163.53f, -56.4816f, 1.07924f, 190, 0, 0 },
	{ 15107, 1189.31f, 1183.63f, -56.0443f, 2.44346f, 190, 0, 0 },
};

static const ABControlPointCreatureSpawn AB_STABLE_HORDE_SPAWNS[] =
{
	{ 15087, 1187.37f, 1199.46f, -56.3711f, 5.2709f, 412, 0, 0 },
	{ 15087, 1201.63f, 1174.92f, -56.3803f, 5.14872f, 412, 233, 0 },
	{ 15087, 1167.45f, 1182.16f, -56.3106f, 5.17983f, 412, 0, 2 },
	{ 15108, 1160.12f, 1169.93f, -56.3168f, 2.04316f, 190, 0, 0 },
	{ 15108, 1184.85f, 1179.24f, -55.9334f, 2.21657f, 190, 0, 0 },
	{ 15108, 1171.04f, 1144.41f, -56.0605f, 3.24631f, 190, 0, 0 },
	{ 15108, 1175.07f, 1155.1f, -56.4464f, 0.728899f, 190, 0, 0 },
	{ 15108, 1167.13f, 1186.34f, -56.2799f, 4.59022f, 190, 0, 0 },
	{ 15108, 1191.47f, 1185.54f, -56.0253f, 2.30383f, 190, 0, 0 },
	{ 15108, 1186.12f, 1190.79f, -56.0364f, 2.35619f, 190, 0, 0 },
};

static const ABControlPointCreatureSpawn AB_FARM_ALLIANCE_SPAWNS[] =
{
	{ 15045, 811.552f, 792.473f, -57.8178f, 0.471185f, 1577, 0, 2 },
	{ 15045, 841.648f, 864.919f, -57.4323f, 1.26686f, 1577, 0, 2 },
	{ 15045, 822.796f, 869.174f, -57.843f, 4.24193f, 1577, 0, 2 },
	{ 15045, 786.069f, 824.469f, -55.9782f, 5.05953f, 1577, 0, 2 },
	{ 15045, 815.798f, 831.478f, -57.1758f, 4.94565f, 1577, 0, 2 },
	{ 15045, 799.065f, 846.511f, -56.8651f, 2.39458f, 1577, 0, 2 },
	{ 15045, 847.3f, 836.659f, -57.8924f, 5.89317f, 1577, 0, 2 },
	{ 15045, 792.457f, 823.458f, -56.5414f, 2.52741f, 1577, 69, 0 },
};

static const ABControlPointCreatureSpawn AB_FARM_HORDE_SPAWNS[] =
{
	{ 15046, 849.179f, 833.499f, -57.7416f, 5.61996f, 412, 0, 2 },
	{ 15046, 791.34f, 822.852f, -56.4006f, 1.8326f, 412, 69, 0 },
	{ 15046, 812.433f, 791.921f, -57.7863f, 2.91346f, 412, 0, 2 },
	{ 15046, 822.677f, 867.832f, -57.7916f, 4.67619f, 412, 0, 2 },
	{ 15046, 843.426f, 857.894f, -57.6725f, 2.27435f, 412, 0, 2 },
	{ 15046, 815.514f, 831.668f, -57.1119f, 4.46503f, 412, 0, 2 },
	{ 15046, 823.35f, 817.641f, -57.6701f, 1.54966f, 412, 0, 2 },
	{ 15046, 798.336f, 847.737f, -56.7132f, 5.14592f, 412, 0, 2 },
};

static const ABControlPointCreatureSpawn AB_BLACKSMITH_ALLIANCE_SPAWNS[] =
{
	{ 15063, 991.74f, 1000.92f, -42.5199f, 2.67035f, 1577, 233, 0 },
	{ 15063, 969.127f, 999.597f, -43.9439f, 2.3911f, 1577, 233, 1 },
	{ 15063, 983.693f, 1008.41f, -42.5199f, 5.35816f, 1577, 233, 0 },
	{ 15063, 996.188f, 1003.25f, -42.5221f, 4.72984f, 1577, 233, 0 },
	{ 15063, 979.588f, 997.244f, -43.9798f, 0.575959f, 1577, 69, 0 },
	{ 15063, 980.162f, 989.083f, -43.9306f, 0.191986f, 1577, 69, 1 },
	{ 15063, 990.042f, 1014.51f, -42.5199f, 6.16101f, 1577, 233, 0 },
};

static const ABControlPointCreatureSpawn AB_BLACKSMITH_HORDE_SPAWNS[] =
{
	{ 15064, 968.975f, 999.732f, -43.9377f, 2.53073f, 412, 233, 0 },
	{ 15064, 996.413f, 1002.88f, -42.52f, 4.7822f, 412, 233, 0 },
	{ 15064, 990.38f, 1014.8f, -42.5199f, 6.12611f, 412, 233, 0 },
	{ 15064, 983.581f, 1008.55f, -42.5199f, 5.34071f, 412, 233, 0 },
	{ 15064, 979.603f, 997.367f, -43.9784f, 1.01229f, 412, 69, 0 },
};

static const ABControlPointCreatureSpawn AB_MINE_ALLIANCE_SPAWNS[] =
{
	{ 15074, 1214.26f, 803.153f, -102.681f, 5.25344f, 1577, 233, 0 },
	{ 15074, 1138.93f, 811.071f, -99.5951f, 5.16617f, 1577, 0, 0 },
	{ 15074, 1202.03f, 810.835f, -103.166f, 1.46608f, 1577, 233, 0 },
	{ 15074, 1235.97f, 802.637f, -103.035f, 3.64774f, 1577, 233, 0 },
	{ 15074, 1139.79f, 809.247f, -99.5951f, 2.02458f, 1577, 0, 0 },
	{ 15074, 1258.31f, 775.819f, -105.636f, 6.23082f, 1577, 233, 0 },
	{ 15074, 1226.12f, 816.11f, -102.404f, 1.01229f, 1577, 233, 0 },
	{ 15074, 1231.03f, 786.665f, -102.642f, 3.63029f, 1577, 233, 0 },
	{ 15074, 1184.03f, 834.479f, -102.975f, 4.90438f, 1577, 233, 0 },
	{ 15074, 1249.79f, 794.31f, -102.989f, 0.750492f, 1577, 233, 0 },
	{ 15074, 1197.5f, 860.736f, -98.6642f, 1.60941f, 1577, 0, 2 },
};

static const ABControlPointCreatureSpawn AB_MINE_HORDE_SPAWNS[] =
{
	{ 15075, 1200.75f, 802.971f, -103.325f, 5.0091f, 412, 233, 0 },
	{ 15075, 1242.1f, 808.662f, -102.936f, 0.663225f, 412, 233, 0 },
	{ 15075, 1211.4f, 810.13f, -102.83f, 1.3439f, 412, 233, 0 },
	{ 15075, 1229.6f, 807.341f, -103.111f, 4.17134f, 412, 233, 0 },
	{ 15075, 1250.52f, 793.177f, -103.23f, 0.471239f, 412, 233, 0 },
	{ 15075, 1258.08f, 775.87f, -105.596f, 6.0912f, 412, 233, 0 },
	{ 15075, 1195.33f, 849.472f, -98.6098f, 4.39823f, 412, 0, 2 },
	{ 15075, 1232.78f, 788.376f, -102.684f, 2.6529f, 412, 233, 0 },
	{ 15075, 1186.86f, 884.933f, -103.591f, 1.41372f, 412, 133, 0 },
};

static const ABControlPointCreatureSpawn AB_LUMBERMILL_ALLIANCE_SPAWNS[] =
{
	{ 15062, 812.093f, 1161.07f, 11.6124f, 2.3911f, 1577, 233, 2 },
	{ 15062, 838.966f, 1241.34f, 16.6564f, 3.75246f, 1577, 234, 2 },
	{ 15062, 830.316f, 1136.88f, 11.3574f, 5.14872f, 1577, 234, 2 },
	{ 15062, 815.829f, 1088.29f, 9.53132f, 0.05236f, 1577, 234, 2 },
	{ 15062, 909.461f, 1232.72f, 6.57686f, 3.29867f, 1577, 234, 2 },
	{ 15062, 908.642f, 1183.09f, 5.05999f, 0.820305f, 1577, 234, 2 },
	{ 15062, 873.048f, 1264.44f, 18.8553f, 1.39626f, 1577, 234, 2 },
	{ 15062, 884.481f, 1176.69f, 9.99647f, 4.08407f, 1577, 234, 2 },
	{ 15062, 751.68f, 1198.45f, 18.232f, 3.35103f, 1577, 234, 2 },
	{ 15062, 760.732f, 1083.22f, 15.693f, 4.03171f, 1577, 234, 2 },
	{ 15062, 767.403f, 1118.95f, 17.3766f, 2.33874f, 1577, 234, 2 },
};

static const ABControlPointCreatureSpawn AB_LUMBERMILL_HORDE_SPAWNS[] =
{
	{ 15089, 812.393f, 1160.85f, 11.6123f, 2.33874f, 412, 233, 2 },
	{ 15089, 913.165f, 1311.43f, 24.9025f, 0.558505f, 412, 234, 2 },
	{ 15089, 872.633f, 1264.29f, 18.8666f, 1.44862f, 412, 234, 2 },
	{ 15089, 742.763f, 1243.59f, 22.7741f, 1.3439f, 412, 234, 2 },
	{ 15089, 831.255f, 1236.06f, 17.2953f, 0.506145f, 412, 234, 2 },
	{ 15089, 761.081f, 1082.74f, 15.5728f, 3.80482f, 412, 234, 2 },
	{ 15089, 787.803f, 1230.45f, 18.6236f, 3.89208f, 412, 234, 2 },
	{ 15089, 884.648f, 1176.52f, 9.96061f, 3.94444f, 412, 234, 2 },
	{ 15089, 909.529f, 1232.81f, 6.55989f, 3.47321f, 412, 234, 2 },
	{ 15089, 908.431f, 1248.12f, 9.23548f, 0.890118f, 412, 234, 2 },
	{ 15089, 908.765f, 1183.24f, 5.00921f, 0.331613f, 412, 234, 2 },
	{ 15089, 764.691f, 1147.38f, 18.9711f, 1.74533f, 412, 234, 2 },
	{ 15089, 920.323f, 1251.82f, 8.46854f, 3.40339f, 412, 234, 2 },
	{ 15089, 815.915f, 1088.65f, 9.58834f, 6.23082f, 412, 234, 2 },
	{ 15089, 751.929f, 1198.46f, 18.1631f, 3.38594f, 412, 234, 2 },
};

static const ABControlPointCreatureSpawn* ABGetControlPointSpawnSet(uint32 node, uint32 team, size_t& count)
{
	count = 0;

	switch(node)
	{
	case AB_CONTROL_POINT_STABLE:
		if(team == 0)
		{
			count = sizeof(AB_STABLE_ALLIANCE_SPAWNS) / sizeof(ABControlPointCreatureSpawn);
			return AB_STABLE_ALLIANCE_SPAWNS;
		}

		count = sizeof(AB_STABLE_HORDE_SPAWNS) / sizeof(ABControlPointCreatureSpawn);
		return AB_STABLE_HORDE_SPAWNS;

	case AB_CONTROL_POINT_FARM:
		if(team == 0)
		{
			count = sizeof(AB_FARM_ALLIANCE_SPAWNS) / sizeof(ABControlPointCreatureSpawn);
			return AB_FARM_ALLIANCE_SPAWNS;
		}

		count = sizeof(AB_FARM_HORDE_SPAWNS) / sizeof(ABControlPointCreatureSpawn);
		return AB_FARM_HORDE_SPAWNS;

	case AB_CONTROL_POINT_BLACKSMITH:
		if(team == 0)
		{
			count = sizeof(AB_BLACKSMITH_ALLIANCE_SPAWNS) / sizeof(ABControlPointCreatureSpawn);
			return AB_BLACKSMITH_ALLIANCE_SPAWNS;
		}

		count = sizeof(AB_BLACKSMITH_HORDE_SPAWNS) / sizeof(ABControlPointCreatureSpawn);
		return AB_BLACKSMITH_HORDE_SPAWNS;

	case AB_CONTROL_POINT_MINE:
		if(team == 0)
		{
			count = sizeof(AB_MINE_ALLIANCE_SPAWNS) / sizeof(ABControlPointCreatureSpawn);
			return AB_MINE_ALLIANCE_SPAWNS;
		}

		count = sizeof(AB_MINE_HORDE_SPAWNS) / sizeof(ABControlPointCreatureSpawn);
		return AB_MINE_HORDE_SPAWNS;

	case AB_CONTROL_POINT_LUMBERMILL:
		if(team == 0)
		{
			count = sizeof(AB_LUMBERMILL_ALLIANCE_SPAWNS) / sizeof(ABControlPointCreatureSpawn);
			return AB_LUMBERMILL_ALLIANCE_SPAWNS;
		}

		count = sizeof(AB_LUMBERMILL_HORDE_SPAWNS) / sizeof(ABControlPointCreatureSpawn);
		return AB_LUMBERMILL_HORDE_SPAWNS;
	}

	return NULL;
}

void ArathiBasin::SpawnBuff(uint32 x)
{
	uint32 chosen_buffid = buffentrys[RandomUInt(2)];
	GameObjectInfo * goi = GameObjectNameStorage.LookupEntry(chosen_buffid);
	if(goi == NULL)
		return;

	if(m_buffs[x] == NULL)
	{
		m_buffs[x] = SpawnGameObject(chosen_buffid, m_mapMgr->GetMapId(), BuffCoordinates[x][0], BuffCoordinates[x][1], BuffCoordinates[x][2],
			BuffCoordinates[x][3], 0, 114, 1);

		m_buffs[x]->SetFloatValue(GAMEOBJECT_ROTATION_02, BuffRotations[x][0]);
		m_buffs[x]->SetFloatValue(GAMEOBJECT_ROTATION_03, BuffRotations[x][1]);
		m_buffs[x]->SetUInt32Value(GAMEOBJECT_STATE, 1);
		m_buffs[x]->SetUInt32Value(GAMEOBJECT_TYPE_ID, 6);
		m_buffs[x]->SetUInt32Value(GAMEOBJECT_ANIMPROGRESS, 100);
		m_buffs[x]->PushToWorld(m_mapMgr);
	}
	else
	{
		// only need to reassign guid if the entry changes.
		if(m_buffs[x]->IsInWorld())
			m_buffs[x]->RemoveFromWorld(false);

		if(chosen_buffid != m_buffs[x]->GetEntry())
		{
			m_buffs[x]->SetNewGuid(m_mapMgr->GenerateGameobjectGuid());
			m_buffs[x]->SetUInt32Value(OBJECT_FIELD_ENTRY, chosen_buffid);
			m_buffs[x]->SetInfo(goi);
		}

		m_buffs[x]->PushToWorld(m_mapMgr);
	}
}

void ArathiBasin::SpawnControlPoint(uint32 Id, uint32 Type)
{
	GameObjectInfo * gi, * gi_aura;
	gi = GameObjectNameStorage.LookupEntry(ControlPointGoIds[Id][Type]);
	if(gi == NULL)
		return;

	gi_aura = gi->sound3 ? GameObjectNameStorage.LookupEntry(gi->sound3) : NULL;

	if(m_controlPoints[Id] == NULL)
	{
		m_controlPoints[Id] = SpawnGameObject(gi->ID, m_mapMgr->GetMapId(), ControlPointCoordinates[Id][0], ControlPointCoordinates[Id][1],
			ControlPointCoordinates[Id][2], ControlPointCoordinates[Id][3], 0, 35, 1.0f);

		m_controlPoints[Id]->SetFloatValue(GAMEOBJECT_ROTATION_02, ControlPointRotations[Id][0]);
		m_controlPoints[Id]->SetFloatValue(GAMEOBJECT_ROTATION_03, ControlPointRotations[Id][1]);
		m_controlPoints[Id]->SetUInt32Value(GAMEOBJECT_STATE, 1);
		m_controlPoints[Id]->SetUInt32Value(GAMEOBJECT_TYPE_ID, gi->Type);
		m_controlPoints[Id]->SetUInt32Value(GAMEOBJECT_ANIMPROGRESS, 100);
		m_controlPoints[Id]->SetUInt32Value(GAMEOBJECT_DYN_FLAGS, 1);
		m_controlPoints[Id]->SetUInt32Value(GAMEOBJECT_DISPLAYID, gi->DisplayID);

		switch(Type)
		{
		case AB_SPAWN_TYPE_ALLIANCE_ASSAULT:
		case AB_SPAWN_TYPE_ALLIANCE_CONTROLLED:
			m_controlPoints[Id]->SetUInt32Value(GAMEOBJECT_FACTION, 2);
			break;

		case AB_SPAWN_TYPE_HORDE_ASSAULT:
		case AB_SPAWN_TYPE_HORDE_CONTROLLED:
			m_controlPoints[Id]->SetUInt32Value(GAMEOBJECT_FACTION, 1);
			break;

		default:
			m_controlPoints[Id]->SetUInt32Value(GAMEOBJECT_FACTION, 35);		// neutral
			break;
		}

		m_controlPoints[Id]->bannerslot = Id;
		m_controlPoints[Id]->PushToWorld(m_mapMgr);
	}
	else
	{
		if(m_controlPoints[Id]->IsInWorld())
			m_controlPoints[Id]->RemoveFromWorld(false);

		// assign it a new guid (client needs this to see the entry change?)
		m_controlPoints[Id]->SetNewGuid(m_mapMgr->GenerateGameobjectGuid());
		m_controlPoints[Id]->SetUInt32Value(OBJECT_FIELD_ENTRY, gi->ID);
		m_controlPoints[Id]->SetUInt32Value(GAMEOBJECT_DISPLAYID, gi->DisplayID);
		m_controlPoints[Id]->SetUInt32Value(GAMEOBJECT_TYPE_ID, gi->Type);

		switch(Type)
		{
		case AB_SPAWN_TYPE_ALLIANCE_ASSAULT:
		case AB_SPAWN_TYPE_ALLIANCE_CONTROLLED:
			m_controlPoints[Id]->SetUInt32Value(GAMEOBJECT_FACTION, 2);
			break;

		case AB_SPAWN_TYPE_HORDE_ASSAULT:
		case AB_SPAWN_TYPE_HORDE_CONTROLLED:
			m_controlPoints[Id]->SetUInt32Value(GAMEOBJECT_FACTION, 1);
			break;

		default:
			m_controlPoints[Id]->SetUInt32Value(GAMEOBJECT_FACTION, 35);		// neutral
			break;
		}

		m_controlPoints[Id]->SetInfo(gi);
		m_controlPoints[Id]->PushToWorld(m_mapMgr);
	}

	if(gi_aura==NULL)
	{
		// remove it if it exists
		if(m_controlPointAuras[Id]!=NULL && m_controlPointAuras[Id]->IsInWorld())
			m_controlPointAuras[Id]->RemoveFromWorld(false);
			
		return;
	}

	if(m_controlPointAuras[Id] == NULL)
	{
		m_controlPointAuras[Id] = SpawnGameObject(gi_aura->ID, m_mapMgr->GetMapId(), ControlPointCoordinates[Id][0], ControlPointCoordinates[Id][1],
			ControlPointCoordinates[Id][2], ControlPointCoordinates[Id][3], 0, 35, 1.0f);

		m_controlPointAuras[Id]->SetFloatValue(GAMEOBJECT_ROTATION_02, ControlPointRotations[Id][0]);
		m_controlPointAuras[Id]->SetFloatValue(GAMEOBJECT_ROTATION_03, ControlPointRotations[Id][1]);
		m_controlPointAuras[Id]->SetUInt32Value(GAMEOBJECT_STATE, 1);
		m_controlPointAuras[Id]->SetUInt32Value(GAMEOBJECT_TYPE_ID, 6);
		m_controlPointAuras[Id]->SetUInt32Value(GAMEOBJECT_ANIMPROGRESS, 100);
		m_controlPointAuras[Id]->bannerauraslot = Id;
		m_controlPointAuras[Id]->PushToWorld(m_mapMgr);
	}
	else
	{
		if(m_controlPointAuras[Id]->IsInWorld())
			m_controlPointAuras[Id]->RemoveFromWorld(false);

		// re-spawn the aura
		m_controlPointAuras[Id]->SetNewGuid(m_mapMgr->GenerateGameobjectGuid());
		m_controlPointAuras[Id]->SetUInt32Value(OBJECT_FIELD_ENTRY, gi_aura->ID);
		m_controlPointAuras[Id]->SetUInt32Value(GAMEOBJECT_DISPLAYID, gi_aura->DisplayID);
		m_controlPointAuras[Id]->SetInfo(gi_aura);
		m_controlPointAuras[Id]->PushToWorld(m_mapMgr);
	}	
}

void ArathiBasin::OnCreate()
{
	// Alliance Gate
	GameObject *gate = SpawnGameObject(180255, 529, 1284.597290f, 1281.166626f, -15.977916f, 0.706859f, 32, 114, 1.5799990f);
	gate->SetFloatValue(GAMEOBJECT_ROTATION,0.0129570f);
	gate->SetFloatValue(GAMEOBJECT_ROTATION_01,-0.0602880f);
	gate->SetFloatValue(GAMEOBJECT_ROTATION_02,0.3449600f);
	gate->SetFloatValue(GAMEOBJECT_ROTATION_03,0.9365900f);
	gate->PushToWorld(m_mapMgr);
	m_gates.push_back(gate);

	// horde gate
	gate = SpawnGameObject(180256, 529, 708.0902710f, 708.4479370f, -17.3898964f, -2.3910990f, 32, 114, 1.5699990f);
	gate->SetFloatValue(GAMEOBJECT_ROTATION,0.0502910f);
	gate->SetFloatValue(GAMEOBJECT_ROTATION_01,0.0151270f);
	gate->SetFloatValue(GAMEOBJECT_ROTATION_02,0.9292169f);
	gate->SetFloatValue(GAMEOBJECT_ROTATION_03,-0.3657840f);
	gate->PushToWorld(m_mapMgr);
	m_gates.push_back(gate);

	CleanupControlPointDbSpawns();

	// spawn (default) control points
	SpawnControlPoint(AB_CONTROL_POINT_STABLE,		AB_SPAWN_TYPE_NEUTRAL);
	SpawnControlPoint(AB_CONTROL_POINT_BLACKSMITH,	AB_SPAWN_TYPE_NEUTRAL);
	SpawnControlPoint(AB_CONTROL_POINT_LUMBERMILL,	AB_SPAWN_TYPE_NEUTRAL);
	SpawnControlPoint(AB_CONTROL_POINT_MINE,		AB_SPAWN_TYPE_NEUTRAL);
	SpawnControlPoint(AB_CONTROL_POINT_FARM,		AB_SPAWN_TYPE_NEUTRAL);

	// spawn buffs
	SpawnBuff(AB_BUFF_STABLES);
	SpawnBuff(AB_BUFF_BLACKSMITH);
	SpawnBuff(AB_BUFF_LUMBERMILL);
	SpawnBuff(AB_BUFF_MINE);
	SpawnBuff(AB_BUFF_FARM);

	CreateControlPointCreatures();

	// spawn the h/a base spirit guides
	AddSpiritGuide(SpawnSpiritGuide(NoBaseGYLocations[0][0],NoBaseGYLocations[0][1],NoBaseGYLocations[0][2], 0.0f, 0));
	AddSpiritGuide(SpawnSpiritGuide(NoBaseGYLocations[1][0],NoBaseGYLocations[1][1],NoBaseGYLocations[1][2], 0.0f, 1));

	// urrrgh worldstates
	SetWorldState(0x8D8, 0x00);
	SetWorldState(0x8D7, 0x00);
	SetWorldState(0x8D6, 0x00);
	SetWorldState(0x8D5, 0x00);
	SetWorldState(0x8D4, 0x00);
	SetWorldState(0x8D3, 0x00);

	// AB world state's
	// unknowns, need more research
	SetWorldState(0x7A3, 1800); // unknown
	SetWorldState(0x745, 0x02); // unknown

	// Icon stuff for on the map
	SetWorldState(AB_SHOW_BACKSMITH_ICON, 			 0x01);
	SetWorldState(AB_SHOW_FARM_ICON, 				  0x01);
	SetWorldState(AB_SHOW_LUMBERMILL_ICON, 			0x01);
	SetWorldState(AB_SHOW_GOLDMINE_ICON, 			 0x01);
	SetWorldState(AB_SHOW_STABLE_ICON, 			0x01);

	// LumberMill
	SetWorldState(AB_CAPTURING_LUMBERMILL_HORDE, 	   0x00);
	SetWorldState(AB_CAPTURING_LUMBERMILL_ALLIANCE, 	0x00);
	SetWorldState(AB_CAPTURED_LUMBERMILL_HORDE, 		0x00);
	SetWorldState(AB_CAPTURED_LUMBERMILL_ALLIANCE, 	 0x00);

	// GoldMine
	SetWorldState(AB_CAPTURING_GOLDMINE_HORDE, 		 0x00);
	SetWorldState(AB_CAPTURING_GOLDMINE_ALLIANCE, 	  0x00);
	SetWorldState(AB_CAPTURED_GOLDMINE_HORDE, 		  0x00);
	SetWorldState(AB_CAPTURED_GOLDMINE_ALLIANCE, 	   0x00);

	// BlackSmith
	SetWorldState(AB_CAPTURING_BLACKSMITH_HORDE, 	   0x00);
	SetWorldState(AB_CAPTURING_BLACKSMITH_ALLIANCE, 	0x00);
	SetWorldState(AB_CAPTURED_BLACKSMITH_HORDE, 		0x00);
	SetWorldState(AB_CAPTURED_BLACKSMITH_ALLIANCE, 	 0x00);

	SetWorldState(AB_MAX_SCORE, 						RESOURCES_WINVAL);
	SetWorldState(AB_ALLIANCE_CAPTUREBASE, 			 0x00);
	SetWorldState(AB_HORDE_CAPTUREBASE, 				0x00);
	SetWorldState(AB_HORDE_RESOURCES, 				  0x00);
	SetWorldState(AB_ALLIANCE_RESOURCES, 			   0x00);

	// Farm
	SetWorldState(AB_CAPTURING_FARM_ALLIANCE, 		 0x00);
	SetWorldState(AB_CAPTURING_FARM_HORDE, 			 0x00);
	SetWorldState(AB_CAPTURED_FARM_HORDE, 			  0x00);
	SetWorldState(AB_CAPTURED_FARM_ALLIANCE, 		   0x00);

	// Stables
	SetWorldState(AB_CAPTURING_STABLES_HORDE, 		  0x00);
	SetWorldState(AB_CAPTURING_STABLES_ALLIANCE, 	   0x00);
	SetWorldState(AB_CAPTURED_STABLES_HORDE, 		   0x00);
	SetWorldState(AB_CAPTURED_STABLES_ALLIANCE, 		0x00);
}

void ArathiBasin::OnStart()
{
	for(uint32 i = 0; i < 2; ++i) {
		for(set<Player*>::iterator itr = m_players[i].begin(); itr != m_players[i].end(); ++itr) {
			(*itr)->RemoveAura(BG_PREPARATION);
		}
	}

	// open gates
	for(list<GameObject*>::iterator itr = m_gates.begin(); itr != m_gates.end(); ++itr)
	{
		(*itr)->SetUInt32Value(GAMEOBJECT_FLAGS, 64);
		(*itr)->SetUInt32Value(GAMEOBJECT_STATE, 0);
	}

	/* correct? - burlex */
	PlaySoundToAll(SOUND_BATTLEGROUND_BEGIN);

	m_started = true;
}

ArathiBasin::ArathiBasin(MapMgr * mgr, uint32 id, uint32 lgroup, uint32 t) : CBattleground(mgr,id,lgroup,t)
{
	uint32 i;
	m_playerCountPerTeam=15;

	for(i = 0; i < AB_NUM_CONTROL_POINTS; ++i)
	{
		m_buffs[i] = NULL;
		m_controlPointAuras[i] = NULL;
		m_controlPoints[i] = NULL;
		m_spiritGuides[i] = NULL;
		m_basesAssaultedBy[i] = -1;
		m_basesOwnedBy[i] = -1;
	}

	for(i = 0; i < 2; ++i)
	{
		m_resources[i] = 0;
		m_capturedBases[i] = 0;
		m_lastHonorGainResources[i] = 0;
		m_nearingVictory[i] = false;
	}
}

ArathiBasin::~ArathiBasin()
{
	// gates are always spawned, so mapmgr will clean them up
	for(uint32 i = 0; i < AB_NUM_CONTROL_POINTS; ++i)
	{
		// buffs may not be spawned, so delete them if they're not
		if(m_buffs[i] != NULL)
		{
			m_buffs[i]->m_battleground = NULL;
			if( !m_buffs[i]->IsInWorld() )
				delete m_buffs[i];
		}

		if(m_controlPoints[i] != NULL)
		{
			m_controlPoints[i]->m_battleground = NULL;
			if( !m_controlPoints[i]->IsInWorld() )
				delete m_controlPoints[i];
		}

		if(m_controlPointAuras[i])
		{
			m_controlPointAuras[i]->m_battleground = NULL;
			if( !m_controlPointAuras[i]->IsInWorld() )
				delete m_controlPointAuras[i];
		}

		for(uint32 team = 0; team < 2; ++team)
		{
			for(vector<Creature*>::iterator itr = m_controlPointCreatures[i][team].begin(); itr != m_controlPointCreatures[i][team].end(); ++itr)
			{
				Creature* creature = *itr;
				if(creature == NULL)
					continue;

				if(!creature->IsInWorld())
					delete creature;
			}
		}
	}
}

void ArathiBasin::CleanupControlPointDbSpawns()
{
	if(m_mapMgr == NULL)
		return;

	for(CreatureSqlIdMap::iterator itr = m_mapMgr->_sqlids_creatures.begin(); itr != m_mapMgr->_sqlids_creatures.end(); ++itr)
	{
		Creature* creature = itr->second;
		if(creature == NULL || itr->first < AB_DB_CREATURE_SQLID_BEGIN || itr->first > AB_DB_CREATURE_SQLID_END)
			continue;

		if(creature->IsInWorld())
			creature->RemoveFromWorld(false, false);
	}

	for(GameObjectSqlIdMap::iterator itr = m_mapMgr->_sqlids_gameobjects.begin(); itr != m_mapMgr->_sqlids_gameobjects.end(); ++itr)
	{
		GameObject* go = itr->second;
		if(go == NULL)
			continue;

		if((itr->first >= AB_DB_BANNER_SQLID_BEGIN && itr->first <= AB_DB_BANNER_SQLID_END) ||
			(itr->first >= AB_DB_GATE_SQLID_BEGIN && itr->first <= AB_DB_GATE_SQLID_END) ||
			(itr->first >= AB_DB_BUFF_SQLID_BEGIN && itr->first <= AB_DB_BUFF_SQLID_END))
		{
			if(go->IsInWorld())
				go->RemoveFromWorld(false);
		}
	}
}

Creature * ArathiBasin::SpawnControlPointCreature(uint32 entry, float x, float y, float z, float o, uint32 factionId, uint32 emoteState, uint8 moveType)
{
	CreatureProto* proto = CreatureProtoStorage.LookupEntry(entry);
	CreatureInfo* info = CreatureNameStorage.LookupEntry(entry);
	if(proto == NULL || info == NULL || m_mapMgr == NULL)
		return NULL;

	CreatureSpawn* sp = new CreatureSpawn;
	sp->entry = entry;
	sp->form = 0;
	sp->id = 0;
	sp->movetype = moveType;
	sp->x = x;
	sp->y = y;
	sp->z = z;
	sp->o = o;
	sp->emote_state = emoteState;
	sp->flags = 0;
	sp->factionid = factionId ? factionId : proto->Faction;
	sp->bytes = 0;
	sp->bytes2 = 0;
	sp->stand_state = 0;
	sp->channel_spell = 0;
	sp->channel_target_creature = 0;
	sp->channel_target_go = 0;

	Creature* creature = m_mapMgr->CreateCreature(entry);
	if(creature == NULL)
	{
		delete sp;
		return NULL;
	}

	creature->Load(sp, (uint32)NULL, NULL);
	creature->spawnid = 0;
	creature->m_spawn = 0;
	delete sp;
	return creature;
}

void ArathiBasin::CreateControlPointCreatures()
{
	for(uint32 node = 0; node < AB_NUM_CONTROL_POINTS; ++node)
	{
		for(uint32 team = 0; team < 2; ++team)
			m_controlPointCreatures[node][team].clear();
	}

	for(uint32 node = 0; node < AB_NUM_CONTROL_POINTS; ++node)
		UpdateControlPointCreatureState(node);
}

void ArathiBasin::ClearControlPointCreatures(uint32 Id, uint32 Team)
{
	if(Id >= AB_NUM_CONTROL_POINTS || Team > 1)
		return;

	for(vector<Creature*>::iterator itr = m_controlPointCreatures[Id][Team].begin(); itr != m_controlPointCreatures[Id][Team].end(); ++itr)
	{
		Creature* creature = *itr;
		if(creature == NULL)
			continue;

		if(creature->IsInWorld())
			creature->RemoveFromWorld(false, false);

		delete creature;
	}

	m_controlPointCreatures[Id][Team].clear();
}

void ArathiBasin::EnsureControlPointCreatures(uint32 Id, uint32 Team)
{
	if(Id >= AB_NUM_CONTROL_POINTS || Team > 1)
		return;

	if(!m_controlPointCreatures[Id][Team].empty())
	{
		for(vector<Creature*>::iterator itr = m_controlPointCreatures[Id][Team].begin(); itr != m_controlPointCreatures[Id][Team].end(); ++itr)
		{
			Creature* creature = *itr;
			if(creature != NULL && creature->isAlive() && !creature->IsInWorld())
				creature->PushToWorld(m_mapMgr);
		}
		return;
	}

	size_t count = 0;
	const ABControlPointCreatureSpawn* spawns = ABGetControlPointSpawnSet(Id, Team, count);
	if(spawns == NULL)
		return;

	for(size_t i = 0; i < count; ++i)
	{
		Creature* creature = SpawnControlPointCreature(spawns[i].entry, spawns[i].x, spawns[i].y, spawns[i].z, spawns[i].o,
			spawns[i].factionId, spawns[i].emoteState, spawns[i].moveType);
		if(creature == NULL)
			continue;

		creature->PushToWorld(m_mapMgr);
		m_controlPointCreatures[Id][Team].push_back(creature);
	}
}

void ArathiBasin::UpdateControlPointCreatureState(uint32 Id)
{
	if(Id >= AB_NUM_CONTROL_POINTS)
		return;

	const bool showAlliance = (m_basesOwnedBy[Id] == 0);
	const bool showHorde = (m_basesOwnedBy[Id] == 1);

	if(showAlliance)
		EnsureControlPointCreatures(Id, 0);
	else
		ClearControlPointCreatures(Id, 0);

	if(showHorde)
		EnsureControlPointCreatures(Id, 1);
	else
		ClearControlPointCreatures(Id, 1);
}

void ArathiBasin::EventUpdateResources(uint32 Team)
{
	uint32 resource_fields[2] = { AB_ALLIANCE_RESOURCES, AB_HORDE_RESOURCES };

	uint32 current_resources = m_resources[Team];
	uint32 current_bases = m_capturedBases[Team];

	if(current_bases>5)
		current_bases=5;

	// figure out how much resources we have to add to that team based on the number of captured bases.
	current_resources += (PointBonusPerUpdate[current_bases]);

	// did it change?
	if(current_resources == m_resources[Team])
		return;

	// check for overflow
	if(current_resources > RESOURCES_WINVAL)
		current_resources = RESOURCES_WINVAL;

	m_resources[Team] = current_resources;
	if((current_resources - m_lastHonorGainResources[Team]) >= RESOURCES_TO_GAIN_BH)
	{
		m_mainLock.Acquire();
		for(set<Player*>::iterator itr = m_players[Team].begin(); itr != m_players[Team].end(); ++itr)
			(*itr)->m_bgScore.BonusHonor += BASE_BH_GAIN;

		UpdatePvPData();
		m_mainLock.Release();
	}

	// update the world states
	SetWorldState(resource_fields[Team], current_resources);

	if(current_resources >= RESOURCES_WARNING_THRESHOLD && !m_nearingVictory[Team])
	{
		m_nearingVictory[Team] = true;
		SendChatMessage(Team ? CHAT_MSG_BG_EVENT_HORDE : CHAT_MSG_BG_EVENT_ALLIANCE, (uint64)0, "The %s has gathered %u resources and is nearing victory!", Team ? "Horde" : "Alliance", current_resources);
		uint32 sound = SOUND_ALLIANCE_BGALMOSTEND - Team;
		PlaySoundToAll(sound);
	}

	// check for winning condition
	if(current_resources == RESOURCES_WINVAL)
	{
		m_ended = true;
		m_winningteam = Team;
		m_nextPvPUpdateTime = 0;

		sEventMgr.RemoveEvents(this);
		sEventMgr.AddEvent(((CBattleground*)this), &CBattleground::Close, EVENT_BATTLEGROUND_CLOSE, 120000, 1,0);

		/* add the marks of honor to all players */
		m_mainLock.Acquire();

		SpellEntry * winner_spell = dbcSpell.LookupEntry(24953);
		SpellEntry * loser_spell = dbcSpell.LookupEntry(24952);
		for(uint32 i = 0; i < 2; ++i)
		{
			for(set<Player*>::iterator itr = m_players[i].begin(); itr != m_players[i].end(); ++itr)
			{
				(*itr)->Root();
				if(i == m_winningteam)
					(*itr)->CastSpell((*itr), winner_spell, true);
				else
					(*itr)->CastSpell((*itr), loser_spell, true);
			}
		}
		m_mainLock.Release();
		UpdatePvPData();
	}
}

void ArathiBasin::HookOnPlayerDeath(Player * plr)
{
	// nothing in this BG
	plr->m_bgScore.Deaths++;
	UpdatePvPData();
}

void ArathiBasin::HookOnMount(Player * plr)
{
	// nothing in this BG
}

void ArathiBasin::HookOnPlayerKill(Player * plr, Unit * pVictim)
{
	if(pVictim->IsPlayer())
	{
		plr->m_bgScore.KillingBlows++;
		UpdatePvPData();
	}
}

void ArathiBasin::HookOnHK(Player * plr)
{
	plr->m_bgScore.HonorableKills++;
	UpdatePvPData();
}

void ArathiBasin::OnAddPlayer(Player * plr)
{
	if(!m_started)
		plr->CastSpell(plr, BG_PREPARATION, true);
}

void ArathiBasin::OnRemovePlayer(Player * plr)
{
	plr->RemoveAura(BG_PREPARATION);
}

void ArathiBasin::HookFlagDrop(Player * plr, GameObject * obj)
{
	// nothing?
}

void ArathiBasin::HookFlagStand(Player * plr, GameObject * obj)
{
	// nothing?
}

LocationVector ArathiBasin::GetStartingCoords(uint32 Team)
{
	if(Team)
		return LocationVector(684.75629f, 681.945007f, -12.915456f, 0.881211f);
	else
		return LocationVector(1314.932495f, 1311.246948f, -9.00952f,3.802896f);
}

void ArathiBasin::HookOnAreaTrigger(Player * plr, uint32 id)
{
	uint32 spellid=0;
	int32 buffslot = -1;
	switch(id)
	{
	case 3866:			// stables
		buffslot=AB_BUFF_STABLES;
		break;

	case 3867:			// farm
		buffslot=AB_BUFF_FARM;		
		break;

	case 3870:			// blacksmith
		buffslot=AB_BUFF_BLACKSMITH;
		break;

	case 3869:			// mine
		buffslot=AB_BUFF_MINE;
		break;

	case 3868:			// lumbermill
		buffslot=AB_BUFF_LUMBERMILL;
		break;

	case 3948:			// alliance/horde exits
	case 3949:
		{
			RemovePlayer(plr,false);
			return;
		}break;

	default:
		Log.Error("ArathiBasin", "Encountered unhandled areatrigger id %u", id);
		return;
		break;
	}

	if(plr->isDead())		// dont apply to dead players... :P
		return;	

	uint32 x = (uint32)buffslot;
	if(m_buffs[x] && m_buffs[x]->IsInWorld())
	{
		// apply the spell
		spellid = m_buffs[x]->GetInfo()->sound3;
		m_buffs[x]->RemoveFromWorld(false);

		// respawn it in buffrespawntime
		sEventMgr.AddEvent(this,&ArathiBasin::SpawnBuff,x,EVENT_AB_RESPAWN_BUFF,AB_BUFF_RESPAWN_TIME,1,EVENT_FLAG_DO_NOT_EXECUTE_IN_WORLD_CONTEXT);

		// cast the spell on the player
		SpellEntry * sp = dbcSpell.LookupEntryForced(spellid);
		if(sp)
		{
			Spell * pSpell = new Spell(plr, sp, true, NULL);
			SpellCastTargets targets(plr->GetGUID());
			pSpell->prepare(&targets);
		}
	}
}

bool ArathiBasin::HookHandleRepop(Player * plr)
{
	/* our uber leet ab graveyard handler */
	LocationVector dest( NoBaseGYLocations[plr->m_bgTeam][0], NoBaseGYLocations[plr->m_bgTeam][1], NoBaseGYLocations[plr->m_bgTeam][2], 0.0f );
	float current_distance = 999999.0f;
	float dist;

	for(uint32 i = 0; i < AB_NUM_CONTROL_POINTS; ++i)
	{
		if(m_basesOwnedBy[i] == (int32)plr->m_bgTeam)
		{
			dist = plr->GetPositionV()->Distance2DSq(GraveyardLocations[i][0], GraveyardLocations[i][1]);
			if(dist < current_distance)
			{
				current_distance = dist;
				dest.ChangeCoords(GraveyardLocations[i][0], GraveyardLocations[i][1], GraveyardLocations[i][2]);
			}
		}
	}

	// port us there.
	plr->SafeTeleport(plr->GetMapId(),plr->GetInstanceID(),dest);
	return true;
}

void ArathiBasin::CaptureControlPoint(uint32 Id, uint32 Team)
{
	if(m_basesOwnedBy[Id] != -1)
	{
		// there is a very slim chance of this happening, 2 teams evnets could clash..
		// just in case...
		return;
	}

	// anticheat, not really necessary because this is a server method but anyway
	if(m_basesAssaultedBy[Id] != (int32)Team)
		return;

	m_basesOwnedBy[Id] = Team;
	m_basesAssaultedBy[Id]=-1;

	// remove the other spirit guide (if it exists) // burlex: shouldnt' happen
	if(m_spiritGuides[Id] != NULL)
	{
		RemoveSpiritGuide(m_spiritGuides[Id]);
		m_spiritGuides[Id]->Despawn(0,0);
	}

	// spawn the spirit guide for our faction
	m_spiritGuides[Id] = SpawnSpiritGuide(GraveyardLocations[Id][0], GraveyardLocations[Id][1], GraveyardLocations[Id][2], 0.0f, Team);
	AddSpiritGuide(m_spiritGuides[Id]);

	// send the chat message/sounds out
	PlaySoundToAll(Team ? SOUND_HORDE_CAPTURE : SOUND_ALLIANCE_CAPTURE);
	SendChatMessage(Team ? CHAT_MSG_BG_EVENT_HORDE : CHAT_MSG_BG_EVENT_ALLIANCE, 0, "The %s has taken the %s!", Team ? "Horde" : "Alliance", ControlPointNames[Id]);
	
	// update the overhead display on the clients (world states)
	m_capturedBases[Team]++;
	SetWorldState(Team ? AB_HORDE_CAPTUREBASE : AB_ALLIANCE_CAPTUREBASE, m_capturedBases[Team]);

	// respawn the control point with the correct aura
	SpawnControlPoint(Id, Team ? AB_SPAWN_TYPE_HORDE_CONTROLLED : AB_SPAWN_TYPE_ALLIANCE_CONTROLLED);
	UpdateControlPointCreatureState(Id);

	// update the map
	SetWorldState(AssaultFields[Id][Team], 0);
	SetWorldState(OwnedFields[Id][Team], 1);

	// resource update event. :)
	if(m_capturedBases[Team]==1)
	{
		// first
		sEventMgr.AddEvent(this,&ArathiBasin::EventUpdateResources, (uint32)Team, EVENT_AB_RESOURCES_UPDATE_TEAM_0+Team, ResourceUpdateIntervals[1], 0,
			EVENT_FLAG_DO_NOT_EXECUTE_IN_WORLD_CONTEXT);
	}
	else
	{
		// not first
		event_ModifyTime(EVENT_AB_RESOURCES_UPDATE_TEAM_0+Team, ResourceUpdateIntervals[m_capturedBases[Team]]);
	}
}

void ArathiBasin::AssaultControlPoint(Player * pPlayer, uint32 Id)
{
#ifdef ANTI_CHEAT
	if(!m_started)
	{
		SendChatMessage(CHAT_MSG_BG_EVENT_NEUTRAL, pPlayer->GetGUID(), "%s has been removed from the game for cheating.", pPlayer->GetName());
		pPlayer->SoftDisconnect();
		return;
	}
#endif

	uint32 Team = pPlayer->m_bgTeam;
	uint32 Owner;

	if(m_basesOwnedBy[Id]==-1 && m_basesAssaultedBy[Id]==-1)
	{
		// omgwtfbbq our flag is a virgin?
		SetWorldState(NeutralFields[Id], 0);
	}

	if(m_basesOwnedBy[Id] != -1)
	{
		Owner = m_basesOwnedBy[Id];

		// set it to uncontrolled for now
		m_basesOwnedBy[Id] = -1;

		// this control point just got taken over by someone! oh noes!
		if( m_spiritGuides[Id] != NULL )
		{
			map<Creature*,set<uint32> >::iterator itr = m_resurrectMap.find(m_spiritGuides[Id]);
			if( itr != m_resurrectMap.end() )
			{
				for( set<uint32>::iterator it2 = itr->second.begin(); it2 != itr->second.end(); ++it2 )
				{
					Player* r_plr = m_mapMgr->GetPlayer( *it2 );
					if( r_plr != NULL && r_plr->isDead() )
						HookHandleRepop( r_plr );
				}
			}
			m_resurrectMap.erase( itr );
			m_spiritGuides[Id]->Despawn( 0, 0 );
			m_spiritGuides[Id] = NULL;
		}

		// detract one from the teams controlled points
		m_capturedBases[Owner] -= 1;
		SetWorldState(Owner ? AB_HORDE_CAPTUREBASE : AB_ALLIANCE_CAPTUREBASE, m_capturedBases[Owner]);

		// reset the world states
		SetWorldState(OwnedFields[Id][Owner], 0);

		// modify the resource update time period
		if(m_capturedBases[Owner]==0)
			this->event_RemoveEvents(EVENT_AB_RESOURCES_UPDATE_TEAM_0+Owner);
		else
			this->event_ModifyTime(EVENT_AB_RESOURCES_UPDATE_TEAM_0 + Owner, ResourceUpdateIntervals[m_capturedBases[Owner]]);
	}

	// nigga stole my flag!
	if(m_basesAssaultedBy[Id] != -1)
	{
		Owner = m_basesAssaultedBy[Id];

		// woah! vehicle hijack!
		m_basesAssaultedBy[Id] = -1;
		SetWorldState(AssaultFields[Id][Owner], 0);

		// make sure the event does not trigger
		sEventMgr.RemoveEvents(this, EVENT_AB_CAPTURE_CP_1 + Id);

		// no need to remove the spawn, SpawnControlPoint will do this.
	}

	m_basesAssaultedBy[Id] = Team;

	// spawn the new control point gameobject
	SpawnControlPoint(Id, Team ? AB_SPAWN_TYPE_HORDE_ASSAULT : AB_SPAWN_TYPE_ALLIANCE_ASSAULT);
	UpdateControlPointCreatureState(Id);

	// send out the chat message and sound
	SendChatMessage(Team ? CHAT_MSG_BG_EVENT_HORDE : CHAT_MSG_BG_EVENT_ALLIANCE, pPlayer->GetGUID(), "$N claims the %s! If left unchallenged, the %s will control it in 1 minute!", ControlPointNames[Id],
		Team ? "Horde" : "Alliance");

	//NEED THE SOUND ID
	//PlaySoundToAll(Team ? SOUND:SOUND);

	// update the client's map with the new assaulting field
	SetWorldState(AssaultFields[Id][Team], 1);

	// create the 60 second event.
	sEventMgr.AddEvent(this, &ArathiBasin::CaptureControlPoint, Id, Team, EVENT_AB_CAPTURE_CP_1 + Id, 60000, 1, EVENT_FLAG_DO_NOT_EXECUTE_IN_WORLD_CONTEXT);
}

bool ArathiBasin::HookSlowLockOpen(GameObject * pGo, Player * pPlayer, Spell * pSpell)
{
	// burlex todo: find a cleaner way to do this that doesnt waste memory.
	if(pGo->bannerslot >= 0 && pGo->bannerslot < AB_NUM_CONTROL_POINTS)
	{
		// TODO: anticheat here
		AssaultControlPoint(pPlayer,pGo->bannerslot);
		return true;
	}

	return false;
}
