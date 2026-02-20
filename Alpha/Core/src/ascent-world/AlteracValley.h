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

enum AVObjectiveType
{
	AV_OBJECTIVE_GRAVEYARD = 0,
	AV_OBJECTIVE_TOWER,
	AV_OBJECTIVE_BUNKER,
	AV_OBJECTIVE_MINE,
};

class AlteracValley : public CBattleground
{
public:
	enum
	{
		AV_MAX_REINFORCEMENTS = 600,
		AV_REINFORCEMENT_TOWER_LOSS = 75,
		AV_REINFORCEMENT_CAPTAIN_LOSS = 100,
		AV_MINE_TICK_MS = 45000,
		AV_BURN_TIMER_MS = 240000,
		AV_AREATRIGGER_IRONDEEP = 5892,
		AV_AREATRIGGER_COLDTOOTH = 5893,
	};

	struct AVObjectiveTemplate
	{
		AVObjectiveType type;
		const char* name;
		uint32 assaultGoEntryAlliance;
		uint32 assaultGoEntryHorde;
		uint32 worldStateAlliance;
		uint32 worldStateHorde;
		float x, y, z, o;
		int32 initialOwner; // 0 alliance, 1 horde, -1 neutral
		uint32 linkedNpcEntry;
	};

	struct AVObjectiveState
	{
		int32 owner;
		int32 assaultingTeam;
		uint32 timer;
		bool destroyed;
		Creature* spiritGuide;
		Creature* linkedUnit;
	};

	AlteracValley(MapMgr* mgr, uint32 id, uint32 lgroup, uint32 t);
	~AlteracValley();

	void HookOnPlayerDeath(Player* plr);
	void HookFlagDrop(Player* plr, GameObject* obj);
	void HookFlagStand(Player* plr, GameObject* obj);
	void HookOnMount(Player* plr);
	void HookOnAreaTrigger(Player* plr, uint32 id);
	bool HookHandleRepop(Player* plr);
	void OnAddPlayer(Player* plr);
	void OnRemovePlayer(Player* plr);
	void OnCreate();
	void HookOnPlayerKill(Player* plr, Unit* pVictim);
	void HookOnHK(Player* plr);
	LocationVector GetStartingCoords(uint32 Team);
	void OnStart();
	bool HookSlowLockOpen(GameObject* pGo, Player* pPlayer, Spell* pSpell);

	static CBattleground* Create(MapMgr* m, uint32 i, uint32 l, uint32 t) { return new AlteracValley(m, i, l, t); }
	const char* GetName() { return "Alterac Valley"; }

private:
	void EventUpdateObjectives();
	void EventMineTick();
	void AssaultObjective(Player* pPlayer, uint32 index);
	void FinalizeObjective(uint32 index);
	void UpdateObjectiveWorldStates(uint32 index);
	void UpdateReinforcementWorldStates();
	void ModifyReinforcements(uint32 team, int32 delta);
	void CheckForEnd();
	void EndBattleground(uint32 winningTeam);
	void UpdateBossRoomGuards();
	Creature* FindLinkedCreature(uint32 entry, float x, float y, float z);

	AVObjectiveState m_objectiveStates[13];
	int32 m_reinforcements[2];
	int32 m_mineOwner[2];
	bool m_captainDead[2];
};
