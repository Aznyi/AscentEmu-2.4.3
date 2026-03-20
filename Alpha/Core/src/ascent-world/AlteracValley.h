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

#pragma once

enum AVObjectiveType
{
	AV_OBJECTIVE_GRAVEYARD = 0,
	AV_OBJECTIVE_TOWER,
	AV_OBJECTIVE_BUNKER,
	AV_OBJECTIVE_MINE,
};

enum AVBannerState
{
	AV_BANNER_STATE_NEUTRAL = 0,
	AV_BANNER_STATE_ALLIANCE_CONTROLLED,
	AV_BANNER_STATE_HORDE_CONTROLLED,
	AV_BANNER_STATE_ALLIANCE_ASSAULTING,
	AV_BANNER_STATE_HORDE_ASSAULTING,
	AV_BANNER_STATE_DESTROYED,
};

enum AVNodeState
{
	AV_NODE_STATE_NEUTRAL = 0,
	AV_NODE_STATE_ALLIANCE_CONTROLLED,
	AV_NODE_STATE_HORDE_CONTROLLED,
	AV_NODE_STATE_ALLIANCE_CONTESTED,
	AV_NODE_STATE_HORDE_CONTESTED,
	AV_NODE_STATE_DESTROYED,
};

enum AVMineState
{
	AV_MINE_STATE_NEUTRAL = -1,
	AV_MINE_STATE_ALLIANCE = 0,
	AV_MINE_STATE_HORDE = 1,
};

class AlteracValley : public CBattleground
{
public:
	enum
	{
		AV_MAX_REINFORCEMENTS = 600,
		AV_REINFORCEMENT_TOWER_LOSS = 75,
		AV_REINFORCEMENT_CAPTAIN_LOSS = 100,
		AV_MINE_COUNT = 2,
		AV_MINE_TICK_MS = 45000,
		AV_BURN_TIMER_MS = 240000,
		AV_AREATRIGGER_IRONDEEP = 5892,
		AV_AREATRIGGER_COLDTOOTH = 5893,
	};

	struct AVObjectiveTemplate
	{
		AVObjectiveType type;
		const char* name;
		uint32 controlledGoEntryAlliance;
		uint32 controlledGoEntryHorde;
		uint32 contestedGoEntryAlliance;
		uint32 contestedGoEntryHorde;
		uint32 neutralGoEntry;
		uint32 worldStateAlliance;
		uint32 worldStateHorde;
		uint32 worldStateAllianceAssault;
		uint32 worldStateHordeAssault;
		uint32 worldStateDestroyed;
		float bannerX, bannerY, bannerZ, bannerO;
		float spiritX, spiritY, spiritZ, spiritO;
		int32 initialOwner; // 0 alliance, 1 horde, -1 neutral
		uint32 linkedNpcEntry;
		bool spawnGuards;
	};

	struct AVObjectiveState
	{
		int32 owner;
		int32 assaultingTeam;
		uint32 timer;
		bool destroyed;
		AVNodeState nodeState;
		Creature* spiritGuide;
		Creature* linkedUnit;
		GameObject* banner;
		AVBannerState bannerState;
		vector<GameObject*> visuals;
		vector<Creature*> allianceGuards;
		vector<Creature*> hordeGuards;
	};

	struct AVMineCreatureSpawn
	{
		uint32 entry;
		float x, y, z, o;
		uint32 movetype;
		uint32 factionid;
		uint32 flags;
		uint32 bytes;
		uint32 bytes2;
		uint32 emote_state;
		uint32 stand_state;
		uint32 channel_spell;
		uint32 channel_target_go;
		uint32 channel_target_creature;
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
	void HookOnQuestTurnIn(Player* plr, uint32 questId) { HandleQuestTurnIn(plr, questId); }
	void OnStart();
	void OnClose();
	bool HookSlowLockOpen(GameObject* pGo, Player* pPlayer, Spell* pSpell);
	/* AV quest-turn-in progression entry point. Call this from the quest completion handler. */
	void HandleQuestTurnIn(Player* plr, uint32 questId);
	virtual bool HandleAirSupportRescue(Player* plr, Creature* commander);
	virtual bool HandleAirSupportDeploy(Player* plr, Creature* commander);

	bool SupportsPlayerLoot() { return true; }
	void HookGenerateLoot(Player* plr, Corpse* pCorpse);

	static CBattleground* Create(MapMgr* m, uint32 i, uint32 l, uint32 t) { return new AlteracValley(m, i, l, t); }
	const char* GetName() { return "Alterac Valley"; }

private:
	void Reset();
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
	void UpdateMineWorldStates(uint32 mine);
	void HideMineDbSpawns();
	void ClearMineRuntimeSpawns(uint32 mine);
	Creature* SpawnMineCreature(const AVMineCreatureSpawn& spawn);
	void SpawnMineState(uint32 mine);
	void UpdateMineNPCs(uint32 mine);
	void InitializeMines();
	void SetMineOwner(uint32 mine, AVMineState owner, uint64 playerGuid, bool announce);
	void EventRespawnMineNPCs(uint32 mine);
	void ScheduleMineRespawn(uint32 mine);
	void CaptureMine(uint32 mine, uint32 team, uint64 playerGuid);
	bool HandleMineBossKill(Player* pPlayer, Creature* pVictim);
	void RepopPlayersOfTeam(int32 team, Creature* spiritGuide);
	Creature* FindLinkedCreature(uint32 entry, float x, float y, float z);
	GameObject* FindGate(uint32 team);
	void SetGateOpen(uint32 team, bool open);
	void InitializeAlteracValleyNodes();
	void UpdateObjectiveNodeState(uint32 index);
	GameObject* SpawnObjectiveBanner(uint32 index, AVBannerState state);
	void UpdateObjectiveBanner(uint32 index);
	void ClearObjectiveBanner(uint32 index);
	void CleanupObjectiveBannerObjects(uint32 index);
	void UpdateObjectiveSpiritGuide(uint32 index);
	int32 GetObjectiveFromBanner(GameObject* pGo);
	void RefreshObjectiveVisuals(uint32 index);
	void SetObjectiveVisualsActive(uint32 index, bool active);
	void RefreshObjectiveGuards(uint32 index);
	void CleanupObjectiveDbGuards(uint32 index);
	void UpdateObjectiveGuards(uint32 index);
	bool IsAirSupportCommanderRescued(uint32 team, uint32 fleet);
	Creature* FindAirSupportCommander(uint32 team, uint32 fleet);
	void ConfigureAirSupportCommander(Creature* creature, uint32 team, uint32 fleet, bool captive, bool returned);
	void StartAirSupportEscort(uint32 team, uint32 fleet, Creature* commander);
	void CompleteAirSupportEscort(uint32 team, uint32 fleet, Creature* commander);
	void MoveAirSupportCommanderToEscortNode(uint32 team, uint32 fleet, Creature* commander);
	bool AdvanceAirSupportEscort(uint32 team, uint32 fleet, Creature* commander);
	void UpdateAirSupportCommanders();
	void UpdateAirSupportStrikeState(uint32 team, uint32 fleet);
	void UpdateAirSupportStrikes();
	void SpawnAirSupportStrikeVisuals(uint32 team, uint32 fleet);
	void ClearAirSupportStrikeVisuals(uint32 team, uint32 fleet);
	void SpawnAirSupportStrikeRiders(uint32 team, uint32 fleet);
	void ClearAirSupportStrikeRiders(uint32 team, uint32 fleet);
	void AddAirSupportTurnIn(uint32 team, uint32 fleet, uint32 questId);
	Creature* SpawnObjectiveGuard(uint32 entry, float x, float y, float z, float o);
	void CleanupObjectiveDbFireVisuals(uint32 index);
	void UpdateObjectivePrisoners(uint32 index);
	Creature* FindObjectiveLinkedUnit(uint32 index);
	void RefreshObjectiveLinkedUnit(uint32 index);
	void AddScraps(uint32 team, uint32 amount);
	void AddBlood(uint32 team, uint32 amount);
	void AddStormCrystals(uint32 team, uint32 amount);
	Creature* FindBossCreature(uint32 team);
	void RefreshBossSupportHealth(uint32 team);
	void UpdateArmorTier(uint32 team);
	bool IsAirSupportReady(uint32 team, uint32 fleet) const;
	void ApplyArmorTierToDefender(Creature* creature, uint32 team, bool restoreFullHealth = false);
	void UpdateElementalSummonState(uint32 team);
	bool IsElementalSummonReady(uint32 team) const;
	uint32 RefreshArmorTierDefenders(uint32 team);
	void SendProgressMessage(uint32 team, const char* fmt, ...);
	void RemoveObjectiveLinkedUnit(uint32 index);

	AVObjectiveState m_objectiveStates[15];
	int32 m_reinforcements[2];
	AVMineState m_mineOwner[AV_MINE_COUNT];
	vector<Creature*> m_mineRuntimeSpawns[AV_MINE_COUNT];
	bool m_mineRespawnPending[AV_MINE_COUNT];
	bool m_mineDbSpawnsHidden;
	bool m_captainDead[2];
	GameObject* m_gates[2];
	bool m_startGatesShouldBeOpen;
	map<uint32, uint32> m_lastDeathTime;
	map<uint32, uint32> m_defenderBaseHealth;
	uint32 m_teamArmorScraps[2];
	uint32 m_teamArmorTier[2];
	uint32 m_teamAirSupportTurnIns[2][3];
	bool m_teamAirSupportReady[2][3];
	bool m_teamAirSupportEscorting[2][3];
	uint32 m_teamAirSupportEscortNode[2][3];
	bool m_teamAirSupportReturned[2][3];
	bool m_teamAirSupportStrikeActive[2][3];
	uint32 m_teamAirSupportStrikeTimeLeft[2][3];
	uint32 m_teamAirSupportStrikePulse[2][3];
	vector<GameObject*> m_teamAirSupportStrikeVisuals[2][3];
	vector<Creature*> m_teamAirSupportStrikeRiders[2][3];
	uint32 m_teamAirSupportStrikePass[2][3];
	uint32 m_teamBlood[2];
	uint32 m_teamStormCrystals[2];
	bool m_teamElementalReady[2];
};
