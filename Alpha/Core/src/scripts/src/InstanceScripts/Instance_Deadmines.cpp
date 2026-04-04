#include "StdAfx.h"
#include "Setup.h"

namespace
{
	enum CreatureEntries : uint32
	{
		CN_DEFIAS_BLACKGUARD = 636,
		CN_DEFIAS_PIRATE = 657,
		CN_DEFIAS_COMPANION = 3450,
		CN_DEFIAS_WATCHMAN = 1725,
		CN_WORLD_INVISIBLE_TRIGGER = 12999,
		CN_SNEED = 643,
		CN_RHAHK_ZOR = 644,
		CN_MR_SMITE = 646,
		CN_CAPTAIN_GREENSKIN = 647,
		CN_EDWIN_VANCLEEF = 639,
		CN_SNEEDS_SHREDDER = 642,
		CN_GILNID = 1763,
	};

	enum GameObjectEntries : uint32
	{
		GO_FACTORY_DOOR = 13965,
		GO_FACTORY_DOOR_LEVER = 101831,
		GO_DOOR_LEVER = 101833,
		GO_IRON_CLAD_DOOR = 16397,
		GO_DEFIAS_CANNON = 16398,
		GO_FOUNDRY_DOOR = 16399,
		GO_MAST_ROOM_DOOR = 16400,
		GO_DEFIAS_GUNPOWDER = 17155,
		GO_MAST_ROOM_LEVER = 101832,
		GO_FOUNDRY_LEVER = 101834,
		GO_MR_SMITE_CHEST = 144111,
		GO_MYSTERIOUS_DEADMINES_CHEST = 180024,
	};

	enum ItemEntries : uint32
	{
		ITEM_DEFIAS_GUNPOWDER = 5397,
	};

	enum QuestEntries : uint32
	{
		QUEST_YOUR_FORTUNE_AWAITS_YOU = 7938,
	};

	enum DoorFlags : uint32
	{
		DOOR_FLAG_NONE = 0,
		DOOR_FLAG_OPEN = 33,
		DOOR_FLAG_LOCKED = 34,
	};

	enum DoorStates : uint32
	{
		DOOR_STATE_OPEN = 0,
		DOOR_STATE_CLOSED = 1,
	};

	enum GameObjectStates : uint32
	{
		GO_STATE_READY = 1,
	};

	enum DeadminesText : uint32
	{
		SOUND_RHAHK_ZOR_AGGRO = 5774,
		SOUND_SMITE_AGGRO_1 = 5775,
		SOUND_SMITE_PHASE_1 = 5778,
		SOUND_SMITE_PHASE_2 = 5779,
		SOUND_SMITE_AGGRO_2 = 5777,
		SOUND_VANCLEEF_AGGRO = 5780,
		SOUND_VANCLEEF_SLAY = 5781,
		SOUND_VANCLEEF_PHASE_1 = 5782,
		SOUND_VANCLEEF_PHASE_2 = 5783,
		SOUND_VANCLEEF_PHASE_3 = 5784,
	};

	enum SmiteEquipment : uint32
	{
		SPELL_SMITE_TRASH = 3391,
		SPELL_NIMBLE_REFLEXES = 6264,
		SMITE_EQUIP_SWORD = 5144,
		SMITE_EQUIP_AXE = 13913,
		SMITE_EQUIP_MACE = 19610,
		SMITE_SWORD_INFO = 218169346,
		SMITE_AXE_INFO = 218169346,
		SMITE_MACE_INFO = 50267394,
	};

	enum CannonTimers : uint32
	{
		CANNON_EVENT_TICK_MS = 500,
		CANNON_FUSE_MS = 3000,
		CANNON_BREACH_MS = 1000,
		CANNON_ATTACK_MS = 1500,
		CANNON_SMITE_ALARM_MS = 2500,
		CANNON_SMOKE_MS = 2500,
		CANNON_TRIGGER_DESPAWN_MS = 5000,
	};

	enum CannonSpells : uint32
	{
		SPELL_CANNON_FIRE = 17278,
		SPELL_EXPLOSION = 5255,
		SPELL_SPAWN_SMOKE = 10389,
	};

	enum CannonSounds : uint32
	{
		SOUND_CANNON_BLAST = 3079,
	};

	enum CannonPhase : uint32
	{
		CANNON_PHASE_IDLE = 0,
		CANNON_PHASE_FUSE,
		CANNON_PHASE_BREACH,
		CANNON_PHASE_ATTACK,
		CANNON_PHASE_SMITE_ALARM,
		CANNON_PHASE_SMOKE,
	};

	struct Position
	{
		float x;
		float y;
		float z;
		float o;
	};

	struct CreatureSpawnPoint
	{
		uint32 entry;
		float x;
		float y;
		float z;
		float o;
	};

	struct DeadminesState
	{
		bool factoryDoorOpen;
		bool ironCladDoorBlasted;
		bool cannonEventInProgress;
		bool cannonDefendersSummoned;
		bool mastDoorOpen;
		bool foundryDoorOpen;
		bool sneedSpawned;

		DeadminesState() :
			factoryDoorOpen(false),
			ironCladDoorBlasted(false),
			cannonEventInProgress(false),
			cannonDefendersSummoned(false),
			mastDoorOpen(false),
			foundryDoorOpen(false),
			sneedSpawned(false)
		{
		}
	};

	typedef unordered_map<uint32, DeadminesState> DeadminesStateMap;
	DeadminesStateMap s_deadminesStates;

	const Position kIronCladDoorPosition = { -100.502000f, -668.771000f, 7.410490f, 0.0f };
	const Position kIronCladBlastPosition = { -101.218000f, -667.969000f, 7.920000f, 0.0f };
	const Position kCannonMuzzlePosition = { -107.562000f, -659.674000f, 7.212110f, 0.0f };
	const Position kCannonDoorLeverPosition = { -96.927800f, -670.597000f, 7.403380f, 0.0f };
	const Position kMrSmiteChestPosition = { 1.379940f, -780.290000f, 9.819290f, 5.470000f };
	const CreatureSpawnPoint kCannonDefenderSpawns[] =
	{
		{ CN_DEFIAS_PIRATE, -102.502000f, -675.771000f, 7.410490f, 0.0f },
		{ CN_DEFIAS_PIRATE, -97.502000f, -674.771000f, 7.410490f, 0.0f },
		{ CN_DEFIAS_COMPANION, -98.502000f, -674.271000f, 7.410490f, 0.0f },
	};

	DeadminesState& GetDeadminesState(MapMgr* mapMgr)
	{
		const uint32 instanceId = (mapMgr != NULL) ? mapMgr->GetInstanceID() : 0;
		return s_deadminesStates[instanceId];
	}

	void ClearDeadminesState(MapMgr* mapMgr)
	{
		if(mapMgr == NULL)
			return;

		s_deadminesStates.erase(mapMgr->GetInstanceID());
	}

	GameObject* FindDoor(MapMgr* mapMgr, uint32 entry, float x, float y, float z)
	{
		if(mapMgr == NULL)
			return NULL;

		return mapMgr->GetInterface()->GetObjectNearestCoords<GameObject, TYPEID_GAMEOBJECT>(entry, x, y, z);
	}

	GameObject* GetFactoryDoor(MapMgr* mapMgr)
	{
		return FindDoor(mapMgr, GO_FACTORY_DOOR, -190.860092f, -456.332184f, 54.496822f);
	}

	GameObject* GetFactoryDoorLever(MapMgr* mapMgr)
	{
		return FindDoor(mapMgr, GO_FACTORY_DOOR_LEVER, -188.136f, -460.313f, 54.5591f);
	}

	GameObject* GetIronCladDoor(MapMgr* mapMgr)
	{
		return FindDoor(mapMgr, GO_IRON_CLAD_DOOR, kIronCladDoorPosition.x, kIronCladDoorPosition.y, kIronCladDoorPosition.z);
	}

	GameObject* GetCannonDoorLever(MapMgr* mapMgr)
	{
		return FindDoor(mapMgr, GO_DOOR_LEVER, kCannonDoorLeverPosition.x, kCannonDoorLeverPosition.y, kCannonDoorLeverPosition.z);
	}

	GameObject* GetMastRoomDoor(MapMgr* mapMgr)
	{
		return FindDoor(mapMgr, GO_MAST_ROOM_DOOR, -289.691650f, -535.988953f, 49.440678f);
	}

	GameObject* GetMastRoomLever(MapMgr* mapMgr)
	{
		return FindDoor(mapMgr, GO_MAST_ROOM_LEVER, -287.282f, -539.877f, 49.4321f);
	}

	GameObject* GetFoundryDoor(MapMgr* mapMgr)
	{
		return FindDoor(mapMgr, GO_FOUNDRY_DOOR, -176.569000f, -577.640991f, 19.311600f);
	}

	GameObject* GetFoundryLever(MapMgr* mapMgr)
	{
		return FindDoor(mapMgr, GO_FOUNDRY_LEVER, -165.404f, -576.924f, 19.3064f);
	}

	GameObject* GetMrSmiteChest(MapMgr* mapMgr)
	{
		return FindDoor(mapMgr, GO_MR_SMITE_CHEST, kMrSmiteChestPosition.x, kMrSmiteChestPosition.y, kMrSmiteChestPosition.z);
	}

	void SetGameObjectUnclickable(GameObject* go, bool unclickable)
	{
		if(go == NULL)
			return;

		uint32 flags = go->GetUInt32Value(GAMEOBJECT_FLAGS);
		if(unclickable)
			flags |= GAMEOBJECT_UNCLICKABLE;
		else
			flags &= ~GAMEOBJECT_UNCLICKABLE;

		go->SetUInt32Value(GAMEOBJECT_FLAGS, flags);
	}

	void SetGameObjectClickable(GameObject* go, bool clickable)
	{
		if(go == NULL)
			return;

		uint32 flags = go->GetUInt32Value(GAMEOBJECT_FLAGS);
		if(clickable)
			flags |= GAMEOBJECT_CLICKABLE;
		else
			flags &= ~GAMEOBJECT_CLICKABLE;

		go->SetUInt32Value(GAMEOBJECT_FLAGS, flags);
	}

	void SetGameObjectInteractive(GameObject* go)
	{
		if(go == NULL)
			return;

		go->SetUInt32Value(GAMEOBJECT_STATE, GO_STATE_READY);
		SetGameObjectUnclickable(go, false);
		SetGameObjectClickable(go, true);
	}

	void SetGameObjectAnimProgress(GameObject* go, uint32 animProgress)
	{
		if(go == NULL)
			return;

		go->SetUInt32Value(GAMEOBJECT_ANIMPROGRESS, animProgress);
	}

	void ApplyDoorStateToAllNearby(MapMgr* mapMgr, uint32 entry, float x, float y, float z, uint32 flags, uint32 state)
	{
		if(mapMgr == NULL)
			return;

		GameObject* nearestDoor = mapMgr->GetInterface()->GetObjectNearestCoords<GameObject, TYPEID_GAMEOBJECT>(entry, x, y, z);
		if(nearestDoor != NULL)
		{
			nearestDoor->SetUInt32Value(GAMEOBJECT_FLAGS, flags);
			nearestDoor->SetUInt32Value(GAMEOBJECT_STATE, state);
		}

		const int32 cellX = mapMgr->GetPosX(x);
		const int32 cellY = mapMgr->GetPosY(y);

		for(int32 dx = -1; dx <= 1; ++dx)
		{
			for(int32 dy = -1; dy <= 1; ++dy)
			{
				MapCell* pCell = mapMgr->GetCell(cellX + dx, cellY + dy);
				if(pCell == NULL)
					continue;

				ObjectSet::const_iterator iter = pCell->Begin();
				for(; iter != pCell->End(); ++iter)
				{
					Object* obj = *iter;
					if(obj == NULL || obj->GetTypeId() != TYPEID_GAMEOBJECT || obj->GetEntry() != entry)
						continue;

					if(obj->CalcDistance(x, y, z) > 12.0f)
						continue;

					GameObject* door = static_cast<GameObject*>(obj);
					door->SetUInt32Value(GAMEOBJECT_FLAGS, flags);
					door->SetUInt32Value(GAMEOBJECT_STATE, state);
				}
			}
		}
	}

	void SetInteractiveStateToAllNearby(MapMgr* mapMgr, uint32 entry, float x, float y, float z)
	{
		if(mapMgr == NULL)
			return;

		MapCell* pCell = mapMgr->GetCell(mapMgr->GetPosX(x), mapMgr->GetPosY(y));
		if(pCell == NULL)
			return;

		ObjectSet::const_iterator iter = pCell->Begin();
		for(; iter != pCell->End(); ++iter)
		{
			Object* obj = *iter;
			if(obj == NULL || obj->GetTypeId() != TYPEID_GAMEOBJECT || obj->GetEntry() != entry)
				continue;

			if(obj->CalcDistance(x, y, z) > 8.0f)
				continue;

			SetGameObjectInteractive(static_cast<GameObject*>(obj));
		}
	}

	void SetDoorState(GameObject* door, uint32 flags, uint32 state)
	{
		if(door == NULL)
			return;

		door->SetUInt32Value(GAMEOBJECT_FLAGS, flags);
		door->SetUInt32Value(GAMEOBJECT_STATE, state);
	}

	void SetDoorLockedClosed(GameObject* door)
	{
		SetDoorState(door, DOOR_FLAG_LOCKED, DOOR_STATE_CLOSED);
		SetGameObjectAnimProgress(door, 0);
	}

	void SetDoorOpen(GameObject* door)
	{
		SetDoorState(door, DOOR_FLAG_OPEN, DOOR_STATE_OPEN);
	}

	void SetDoorOpenAnimated(GameObject* door, uint32 animProgress)
	{
		SetDoorOpen(door);
		SetGameObjectAnimProgress(door, animProgress);
	}

	void SetGameObjectReady(GameObject* go)
	{
		if(go == NULL)
			return;

		go->SetUInt32Value(GAMEOBJECT_STATE, GO_STATE_READY);
	}

	void SetInteractiveReady(GameObject* go)
	{
		if(go == NULL)
			return;

		go->SetUInt32Value(GAMEOBJECT_STATE, GO_STATE_READY);
		SetGameObjectAnimProgress(go, 100);
		SetGameObjectUnclickable(go, false);
		SetGameObjectClickable(go, true);
	}

	void SetLeverReady(GameObject* go)
	{
		SetInteractiveReady(go);
	}

	void SetInteractiveDisabled(GameObject* go)
	{
		if(go == NULL)
			return;

		go->SetUInt32Value(GAMEOBJECT_STATE, DOOR_STATE_OPEN);
		SetGameObjectAnimProgress(go, 100);
		SetGameObjectUnclickable(go, true);
		SetGameObjectClickable(go, false);
	}

	void SetLeverDisabled(GameObject* go)
	{
		SetInteractiveDisabled(go);
	}

	void SetLeverDisabledToAllNearby(MapMgr* mapMgr, uint32 entry, float x, float y, float z)
	{
		if(mapMgr == NULL)
			return;

		MapCell* pCell = mapMgr->GetCell(mapMgr->GetPosX(x), mapMgr->GetPosY(y));
		if(pCell == NULL)
			return;

		ObjectSet::const_iterator iter = pCell->Begin();
		for(; iter != pCell->End(); ++iter)
		{
			Object* obj = *iter;
			if(obj == NULL || obj->GetTypeId() != TYPEID_GAMEOBJECT || obj->GetEntry() != entry)
				continue;

			if(obj->CalcDistance(x, y, z) > 8.0f)
				continue;

			SetLeverDisabled(static_cast<GameObject*>(obj));
		}
	}

	void SetChestReady(GameObject* go)
	{
		SetInteractiveReady(go);
	}

	void ResetInteractiveGameObject(GameObject* go)
	{
		SetInteractiveReady(go);
	}

	void ApplyResolvedDoorState(MapMgr* mapMgr, GameObject* door, uint32 entry, float x, float y, float z, bool open, int32 animProgress = -1)
	{
		if(open)
		{
			if(animProgress >= 0)
				SetDoorOpenAnimated(door, static_cast<uint32>(animProgress));
			else
				SetDoorOpen(door);

			ApplyDoorStateToAllNearby(mapMgr, entry, x, y, z, DOOR_FLAG_OPEN, DOOR_STATE_OPEN);
		}
		else
		{
			SetDoorLockedClosed(door);
			ApplyDoorStateToAllNearby(mapMgr, entry, x, y, z, DOOR_FLAG_LOCKED, DOOR_STATE_CLOSED);
		}
	}

	void ApplyInteractiveStateToResolvedAndNearby(MapMgr* mapMgr, GameObject* go, uint32 entry, float x, float y, float z)
	{
		SetInteractiveReady(go);
		SetInteractiveStateToAllNearby(mapMgr, entry, x, y, z);
	}

	void ApplyFactoryDoorState(MapMgr* mapMgr, const DeadminesState& state)
	{
		ApplyResolvedDoorState(mapMgr, GetFactoryDoor(mapMgr), GO_FACTORY_DOOR, -190.860092f, -456.332184f, 54.496822f, state.factoryDoorOpen);
	}

	void ApplyIronCladDoorState(MapMgr* mapMgr, const DeadminesState& state)
	{
		ApplyResolvedDoorState(mapMgr, GetIronCladDoor(mapMgr), GO_IRON_CLAD_DOOR, kIronCladDoorPosition.x, kIronCladDoorPosition.y, kIronCladDoorPosition.z, state.ironCladDoorBlasted, state.ironCladDoorBlasted ? 255 : 0);
	}

	void ApplyMastDoorState(MapMgr* mapMgr, const DeadminesState& state)
	{
		ApplyResolvedDoorState(mapMgr, GetMastRoomDoor(mapMgr), GO_MAST_ROOM_DOOR, -290.294f, -536.960f, 49.4353f, state.mastDoorOpen, state.mastDoorOpen ? 255 : 0);
	}

	void ApplyFoundryDoorState(MapMgr* mapMgr, const DeadminesState& state)
	{
		ApplyResolvedDoorState(mapMgr, GetFoundryDoor(mapMgr), GO_FOUNDRY_DOOR, -176.569000f, -577.640991f, 19.311600f, state.foundryDoorOpen);
	}

	Creature* FindCreature(MapMgr* mapMgr, uint32 entry, float x, float y, float z)
	{
		if(mapMgr == NULL)
			return NULL;

		return mapMgr->GetInterface()->GetCreatureNearestCoords(x, y, z, entry);
	}

	Creature* GetMrSmite(MapMgr* mapMgr)
	{
		return FindCreature(mapMgr, CN_MR_SMITE, -22.8471f, -797.283f, 20.3745f);
	}

	Creature* GetRhahkZorLeftWatchman(MapMgr* mapMgr)
	{
		if(mapMgr == NULL)
			return NULL;

		return mapMgr->GetInterface()->GetCreatureNearestCoords(-183.914f, -449.757f, 54.6539f, CN_DEFIAS_WATCHMAN);
	}

	Creature* GetRhahkZorRightWatchman(MapMgr* mapMgr)
	{
		if(mapMgr == NULL)
			return NULL;

		return mapMgr->GetInterface()->GetCreatureNearestCoords(-203.097f, -453.160f, 54.2570f, CN_DEFIAS_WATCHMAN);
	}

	void PullRhahkZorGuards(Unit* target, MapMgr* mapMgr)
	{
		if(target == NULL || mapMgr == NULL)
			return;

		Creature* guards[2] =
		{
			GetRhahkZorLeftWatchman(mapMgr),
			GetRhahkZorRightWatchman(mapMgr),
		};

		for(uint32 i = 0; i < 2; ++i)
		{
			Creature* guard = guards[i];
			if(guard == NULL || !guard->isAlive())
				continue;

			guard->GetAIInterface()->AttackReaction(target, 1, 0);
		}
	}

	void OpenFactoryDoor(MapMgr* mapMgr)
	{
		DeadminesState& state = GetDeadminesState(mapMgr);
		state.factoryDoorOpen = true;
		ApplyFactoryDoorState(mapMgr, state);
	}

	Creature* FindOrSpawnCreature(MapMgr* mapMgr, const CreatureSpawnPoint& spawnPoint)
	{
		if(mapMgr == NULL)
			return NULL;

		Creature* creature = FindCreature(mapMgr, spawnPoint.entry, spawnPoint.x, spawnPoint.y, spawnPoint.z);
		if(creature != NULL && creature->isAlive())
			return creature;

		return mapMgr->GetInterface()->SpawnCreature(
			spawnPoint.entry,
			spawnPoint.x,
			spawnPoint.y,
			spawnPoint.z,
			spawnPoint.o,
			true,
			false,
			0,
			0);
	}

	void AttackIfPossible(Creature* creature, Unit* target)
	{
		if(creature == NULL || target == NULL || !creature->isAlive())
			return;

		creature->GetAIInterface()->AttackReaction(target, 1, 0);
	}

	void MoveIfPossible(Creature* creature, float x, float y, float z, float o)
	{
		if(creature == NULL || !creature->isAlive())
			return;

		creature->GetAIInterface()->MoveTo(x, y, z, o);
	}

	Creature* SpawnCannonTrigger(MapMgr* mapMgr, const Position& position)
	{
		if(mapMgr == NULL)
			return NULL;

		Creature* trigger = mapMgr->GetInterface()->SpawnCreature(
			CN_WORLD_INVISIBLE_TRIGGER,
			position.x,
			position.y,
			position.z,
			position.o,
			true,
			false,
			0,
			0);

		if(trigger != NULL)
			trigger->Despawn(CANNON_TRIGGER_DESPAWN_MS, 0);

		return trigger;
	}

	void PlayVisualForSpell(Creature* trigger, uint32 spellId)
	{
		if(trigger == NULL)
			return;

		SpellEntry* spell = dbcSpell.LookupEntryForced(spellId);
		if(spell == NULL)
			return;

		if(spell->SpellVisual != 0)
			trigger->PlaySpellVisual(trigger->GetGUID(), spell->SpellVisual);
		else
			trigger->CastSpell(trigger, spell, true);
	}

	void BroadcastCannonResponse(MapMgr* mapMgr, Unit* activator)
	{
		Creature* smite = GetMrSmite(mapMgr);
		if (smite != NULL && smite->isAlive())
		{
			smite->SendChatMessage(CHAT_MSG_MONSTER_YELL, LANG_UNIVERSAL, "You there! Check out that noise!");
			smite->PlaySoundToSet(SOUND_SMITE_AGGRO_1);
		}
		else if(activator != NULL)
		{
			activator->SendChatMessageAlternateEntry(CN_MR_SMITE, CHAT_MSG_MONSTER_YELL, LANG_UNIVERSAL, "You there! Check out that noise!");
			activator->PlaySoundToSet(SOUND_SMITE_AGGRO_1);
		}
	}

	void BroadcastSmiteAlarm(MapMgr* mapMgr, Unit* activator)
	{
		Creature* smite = GetMrSmite(mapMgr);
		if(smite != NULL && smite->isAlive())
		{
			smite->SendChatMessage(CHAT_MSG_MONSTER_YELL, LANG_UNIVERSAL, "We're under attack! Avast, ye swabs! Repel the invaders!");
			smite->PlaySoundToSet(SOUND_SMITE_AGGRO_2);
		}
		else if(activator != NULL)
		{
			activator->SendChatMessageAlternateEntry(CN_MR_SMITE, CHAT_MSG_MONSTER_YELL, LANG_UNIVERSAL, "We're under attack! Avast, ye swabs! Repel the invaders!");
			activator->PlaySoundToSet(SOUND_SMITE_AGGRO_2);
		}
	}

	void ShowCannonBlast(MapMgr* mapMgr)
	{
		Creature* cannonTrigger = SpawnCannonTrigger(mapMgr, kCannonMuzzlePosition);
		if(cannonTrigger != NULL)
			PlayVisualForSpell(cannonTrigger, SPELL_CANNON_FIRE);

		Creature* blastTrigger = SpawnCannonTrigger(mapMgr, kIronCladBlastPosition);
		if(blastTrigger != NULL)
			PlayVisualForSpell(blastTrigger, SPELL_EXPLOSION);
	}

	void ShowCannonSmoke(MapMgr* mapMgr)
	{
		Creature* smokeTrigger = SpawnCannonTrigger(mapMgr, kIronCladBlastPosition);
		if(smokeTrigger != NULL)
			PlayVisualForSpell(smokeTrigger, SPELL_SPAWN_SMOKE);
	}

	void BreachIronCladDoor(MapMgr* mapMgr)
	{
		if(mapMgr == NULL)
			return;

		GameObject* door = GetIronCladDoor(mapMgr);
		ApplyResolvedDoorState(mapMgr, door, GO_IRON_CLAD_DOOR, kIronCladDoorPosition.x, kIronCladDoorPosition.y, kIronCladDoorPosition.z, true, 255);
		SetGameObjectUnclickable(door, true);
	}

	void LockCannonDoorLever(MapMgr* mapMgr)
	{
		GameObject* lever = GetCannonDoorLever(mapMgr);
		SetLeverDisabled(lever);
		SetLeverDisabledToAllNearby(mapMgr, GO_DOOR_LEVER, kCannonDoorLeverPosition.x, kCannonDoorLeverPosition.y, kCannonDoorLeverPosition.z);
	}

	void MarkCannonBlasted(MapMgr* mapMgr)
	{
		DeadminesState& state = GetDeadminesState(mapMgr);
		state.cannonEventInProgress = false;
		state.ironCladDoorBlasted = true;
		LockCannonDoorLever(mapMgr);
	}

	void TriggerCannonDefenders(MapMgr* mapMgr, Unit* target)
	{
		if(mapMgr == NULL)
			return;

		DeadminesState& state = GetDeadminesState(mapMgr);
		if(state.cannonDefendersSummoned)
			return;

		state.cannonDefendersSummoned = true;
		for(size_t i = 0; i < sizeof(kCannonDefenderSpawns) / sizeof(kCannonDefenderSpawns[0]); ++i)
		{
			Creature* defender = FindOrSpawnCreature(mapMgr, kCannonDefenderSpawns[i]);
			if(defender == NULL || !defender->isAlive())
				continue;

			if(defender->CalcDistance(kIronCladBlastPosition.x, kIronCladBlastPosition.y, kIronCladBlastPosition.z) > 3.0f)
				MoveIfPossible(defender, kIronCladBlastPosition.x, kIronCladBlastPosition.y, kIronCladBlastPosition.z, 0.0f);
			AttackIfPossible(defender, target);
		}
	}

	void OpenMastDoor(MapMgr* mapMgr)
	{
		DeadminesState& state = GetDeadminesState(mapMgr);
		state.mastDoorOpen = true;
		ApplyMastDoorState(mapMgr, state);
	}

	void ToggleMastDoor(MapMgr* mapMgr)
	{
		if(mapMgr == NULL)
			return;

		DeadminesState& state = GetDeadminesState(mapMgr);
		state.mastDoorOpen = !state.mastDoorOpen;
		ApplyMastDoorState(mapMgr, state);
	}

	void ToggleFactoryDoor(MapMgr* mapMgr)
	{
		if(mapMgr == NULL)
			return;

		DeadminesState& state = GetDeadminesState(mapMgr);
		state.factoryDoorOpen = !state.factoryDoorOpen;
		ApplyFactoryDoorState(mapMgr, state);
	}

	void OpenFoundryDoor(MapMgr* mapMgr)
	{
		DeadminesState& state = GetDeadminesState(mapMgr);
		state.foundryDoorOpen = true;
		ApplyFoundryDoorState(mapMgr, state);
	}

	void ToggleFoundryDoor(MapMgr* mapMgr)
	{
		if(mapMgr == NULL)
			return;

		DeadminesState& state = GetDeadminesState(mapMgr);
		state.foundryDoorOpen = !state.foundryDoorOpen;
		ApplyFoundryDoorState(mapMgr, state);
	}

	SpellEntry* FindSpellByNameHash(uint32 nameHash)
	{
		static unordered_map<uint32, SpellEntry*> cache;
		unordered_map<uint32, SpellEntry*>::iterator cached = cache.find(nameHash);
		if(cached != cache.end())
			return cached->second;

		SpellEntry* match = NULL;
		const uint32 rowCount = dbcSpell.GetNumRows();
		for(uint32 i = 0; i < rowCount; ++i)
		{
			SpellEntry* spell = dbcSpell.LookupRow(i);
			if(spell != NULL && spell->NameHash == nameHash)
			{
				match = spell;
				break;
			}
		}

		cache[nameHash] = match;
		return match;
	}

	bool SpellReady(uint32& timer, uint32 diff)
	{
		if(timer > diff)
		{
			timer -= diff;
			return false;
		}

		timer = 0;
		return true;
	}

	class DeadminesCombatAI : public CreatureAIScript
	{
	public:
		DeadminesCombatAI(Creature* creature) : CreatureAIScript(creature), m_updateFrequency(1000)
		{
		}

		void OnCombatStart(Unit* mTarget)
		{
			(void)mTarget;
			uint32 freq = _unit->GetUInt32Value(UNIT_FIELD_BASEATTACKTIME);
			if(freq < 1000)
				freq = 1000;

			m_updateFrequency = freq;
			RegisterAIUpdateEvent(freq);
		}

		void OnCombatStop(Unit* mTarget)
		{
			(void)mTarget;
			_unit->GetAIInterface()->setCurrentAgent(AGENT_NULL);
			_unit->GetAIInterface()->SetAIState(STATE_IDLE);
			RemoveAIUpdateEvent();
			Reset();
		}

		void OnDied(Unit* mKiller)
		{
			(void)mKiller;
			RemoveAIUpdateEvent();
		}

	protected:
		bool CastSpellOnTarget(Unit* target, uint32 spellId, bool instant = false)
		{
			if(target == NULL || _unit->GetCurrentSpell() != NULL)
				return false;

			SpellEntry* spell = dbcSpell.LookupEntryForced(spellId);
			if(spell == NULL)
				return false;

			_unit->CastSpell(target, spell, instant);
			return true;
		}

		bool CastSpellOnTargetByHash(Unit* target, uint32 spellHash, bool instant = false)
		{
			if(target == NULL || _unit->GetCurrentSpell() != NULL)
				return false;

			SpellEntry* spell = FindSpellByNameHash(spellHash);
			if(spell == NULL)
				return false;

			_unit->CastSpell(target, spell, instant);
			return true;
		}

		bool CastSpellOnSelf(uint32 spellId, bool instant = false)
		{
			if(_unit->GetCurrentSpell() != NULL)
				return false;

			SpellEntry* spell = dbcSpell.LookupEntryForced(spellId);
			if(spell == NULL)
				return false;

			_unit->CastSpell(_unit, spell, instant);
			return true;
		}

		bool CastSpellOnSelfByHash(uint32 spellHash, bool instant = false)
		{
			if(_unit->GetCurrentSpell() != NULL)
				return false;

			SpellEntry* spell = FindSpellByNameHash(spellHash);
			if(spell == NULL)
				return false;

			_unit->CastSpell(_unit, spell, instant);
			return true;
		}

		uint32 GetUpdateFrequency() const
		{
			return m_updateFrequency;
		}

		virtual void Reset()
		{
		}

	private:
		uint32 m_updateFrequency;
	};

	class FactoryDoorAI : public GameObjectAIScript
	{
	public:
		FactoryDoorAI(GameObject* go) : GameObjectAIScript(go)
		{
		}

		static GameObjectAIScript* Create(GameObject* go) { return new FactoryDoorAI(go); }

		void OnCreate()
		{
			ApplyFactoryDoorState(_gameobject->GetMapMgr(), GetDeadminesState(_gameobject->GetMapMgr()));
		}

		void OnSpawn()
		{
			ApplyFactoryDoorState(_gameobject->GetMapMgr(), GetDeadminesState(_gameobject->GetMapMgr()));
		}
	};

	class IronCladDoorAI : public GameObjectAIScript
	{
	public:
		IronCladDoorAI(GameObject* go) : GameObjectAIScript(go)
		{
		}

		static GameObjectAIScript* Create(GameObject* go) { return new IronCladDoorAI(go); }

		void OnCreate()
		{
			ApplyIronCladDoorState(_gameobject->GetMapMgr(), GetDeadminesState(_gameobject->GetMapMgr()));
		}

		void OnSpawn()
		{
			ApplyIronCladDoorState(_gameobject->GetMapMgr(), GetDeadminesState(_gameobject->GetMapMgr()));
		}
	};

	class MastDoorAI : public GameObjectAIScript
	{
	public:
		MastDoorAI(GameObject* go) : GameObjectAIScript(go)
		{
		}

		static GameObjectAIScript* Create(GameObject* go) { return new MastDoorAI(go); }

		void OnCreate()
		{
			ApplyMastDoorState(_gameobject->GetMapMgr(), GetDeadminesState(_gameobject->GetMapMgr()));
		}

		void OnSpawn()
		{
			ApplyMastDoorState(_gameobject->GetMapMgr(), GetDeadminesState(_gameobject->GetMapMgr()));
		}
	};

	class FoundryDoorAI : public GameObjectAIScript
	{
	public:
		FoundryDoorAI(GameObject* go) : GameObjectAIScript(go)
		{
		}

		static GameObjectAIScript* Create(GameObject* go) { return new FoundryDoorAI(go); }

		void OnCreate()
		{
			ApplyFoundryDoorState(_gameobject->GetMapMgr(), GetDeadminesState(_gameobject->GetMapMgr()));
		}

		void OnSpawn()
		{
			ApplyFoundryDoorState(_gameobject->GetMapMgr(), GetDeadminesState(_gameobject->GetMapMgr()));
		}
	};

	class FactoryLeverAI : public GameObjectAIScript
	{
	public:
		FactoryLeverAI(GameObject* go) : GameObjectAIScript(go)
		{
		}

		static GameObjectAIScript* Create(GameObject* go) { return new FactoryLeverAI(go); }

		void OnCreate()
		{
			SetLeverReady(_gameobject);
		}

		void OnSpawn()
		{
			SetLeverReady(_gameobject);
		}

		void OnActivate(Player* player)
		{
			(void)player;

			ToggleFactoryDoor(_gameobject->GetMapMgr());
			ApplyInteractiveStateToResolvedAndNearby(_gameobject->GetMapMgr(), GetFactoryDoorLever(_gameobject->GetMapMgr()), GO_FACTORY_DOOR_LEVER, -188.136f, -460.313f, 54.5591f);
		}
	};

	class MastLeverAI : public GameObjectAIScript
	{
	public:
		MastLeverAI(GameObject* go) : GameObjectAIScript(go)
		{
		}

		static GameObjectAIScript* Create(GameObject* go) { return new MastLeverAI(go); }

		void OnCreate()
		{
			SetLeverReady(_gameobject);
		}

		void OnSpawn()
		{
			SetLeverReady(_gameobject);
		}

		void OnActivate(Player* player)
		{
			(void)player;

			MapMgr* mapMgr = _gameobject->GetMapMgr();

			ToggleMastDoor(mapMgr);
			ApplyInteractiveStateToResolvedAndNearby(mapMgr, GetMastRoomLever(mapMgr), GO_MAST_ROOM_LEVER, -287.282f, -539.877f, 49.4321f);
		}
	};

	class FoundryLeverAI : public GameObjectAIScript
	{
	public:
		FoundryLeverAI(GameObject* go) : GameObjectAIScript(go)
		{
		}

		static GameObjectAIScript* Create(GameObject* go) { return new FoundryLeverAI(go); }

		void OnCreate()
		{
			SetLeverReady(_gameobject);
		}

		void OnSpawn()
		{
			SetLeverReady(_gameobject);
		}

		void OnActivate(Player* player)
		{
			(void)player;

			MapMgr* mapMgr = _gameobject->GetMapMgr();

			ToggleFoundryDoor(mapMgr);
			ApplyInteractiveStateToResolvedAndNearby(mapMgr, GetFoundryLever(mapMgr), GO_FOUNDRY_LEVER, -165.404f, -576.924f, 19.3064f);
		}
	};

	class DoorLeverAI : public GameObjectAIScript
	{
	public:
		DoorLeverAI(GameObject* go) : GameObjectAIScript(go)
		{
		}

		static GameObjectAIScript* Create(GameObject* go) { return new DoorLeverAI(go); }

		void OnCreate()
		{
			if(GetDeadminesState(_gameobject->GetMapMgr()).ironCladDoorBlasted)
			{
				SetLeverDisabled(_gameobject);
			}
			else
			{
				SetLeverReady(_gameobject);
			}
		}

		void OnSpawn()
		{
			OnCreate();
		}
	};

	class DefiasCannonAI : public GameObjectAIScript
	{
	public:
		DefiasCannonAI(GameObject* go) : GameObjectAIScript(go), m_phase(CANNON_PHASE_IDLE), m_phaseTimer(0), m_activatorGuid(0)
		{
		}

		static GameObjectAIScript* Create(GameObject* go) { return new DefiasCannonAI(go); }

		void OnCreate()
		{
			DeadminesState& state = GetDeadminesState(_gameobject->GetMapMgr());
			if(state.ironCladDoorBlasted)
			{
				state.cannonEventInProgress = false;
				SetInteractiveDisabled(_gameobject);
			}
			else
			{
				if(state.cannonEventInProgress)
				{
					state.cannonEventInProgress = false;
					state.cannonDefendersSummoned = false;
				}

				SetInteractiveReady(_gameobject);
			}
		}

		void OnSpawn()
		{
			OnCreate();
		}

		void OnActivate(Player* player)
		{
			if(player == NULL)
				return;

			DeadminesState& state = GetDeadminesState(_gameobject->GetMapMgr());
			if(state.ironCladDoorBlasted || state.cannonEventInProgress)
				return;

			if(player->GetItemInterface() == NULL || player->GetItemInterface()->GetItemCount(ITEM_DEFIAS_GUNPOWDER, false) == 0)
			{
				player->BroadcastMessage("The Defias Cannon is missing a reagent: Defias Gunpowder.");
				return;
			}

			player->GetItemInterface()->RemoveItemAmt(ITEM_DEFIAS_GUNPOWDER, 1);
			player->BroadcastMessage("You prime the Defias Cannon.");

			state.cannonEventInProgress = true;
			m_phase = CANNON_PHASE_FUSE;
			m_phaseTimer = CANNON_FUSE_MS;
			m_activatorGuid = player->GetGUID();
			SetInteractiveDisabled(_gameobject);
			RegisterAIUpdateEvent(CANNON_EVENT_TICK_MS);
		}

		void AIUpdate()
		{
			if(m_phaseTimer > CANNON_EVENT_TICK_MS)
			{
				m_phaseTimer -= CANNON_EVENT_TICK_MS;
				return;
			}

			MapMgr* mapMgr = _gameobject->GetMapMgr();
			Unit* activator = (mapMgr != NULL) ? mapMgr->GetUnit(m_activatorGuid) : NULL;

			switch(m_phase)
			{
			case CANNON_PHASE_FUSE:
				if(mapMgr != NULL)
				{
					_gameobject->PlaySoundToSet(SOUND_CANNON_BLAST);
					ShowCannonBlast(mapMgr);
				}

				BroadcastCannonResponse(mapMgr, activator);
				TriggerCannonDefenders(mapMgr, activator);
				m_phase = CANNON_PHASE_BREACH;
				m_phaseTimer = CANNON_BREACH_MS;
				break;

			case CANNON_PHASE_BREACH:
				BreachIronCladDoor(mapMgr);
				m_phase = CANNON_PHASE_ATTACK;
				m_phaseTimer = CANNON_ATTACK_MS;
				break;

			case CANNON_PHASE_ATTACK:
				m_phase = CANNON_PHASE_SMITE_ALARM;
				m_phaseTimer = CANNON_SMITE_ALARM_MS;
				break;

			case CANNON_PHASE_SMITE_ALARM:
				BroadcastSmiteAlarm(mapMgr, activator);
				MarkCannonBlasted(mapMgr);
				m_phase = CANNON_PHASE_SMOKE;
				m_phaseTimer = CANNON_SMOKE_MS;
				break;

			case CANNON_PHASE_SMOKE:
				ShowCannonSmoke(mapMgr);
				m_phase = CANNON_PHASE_IDLE;
				m_phaseTimer = 0;
				m_activatorGuid = 0;
				sEventMgr.RemoveEvents(_gameobject, EVENT_SCRIPT_UPDATE_EVENT);
				break;

			default:
				if(mapMgr != NULL)
					GetDeadminesState(mapMgr).cannonEventInProgress = false;
				m_phase = CANNON_PHASE_IDLE;
				m_phaseTimer = 0;
				m_activatorGuid = 0;
				sEventMgr.RemoveEvents(_gameobject, EVENT_SCRIPT_UPDATE_EVENT);
				break;
			}
		}

	private:
		uint32 m_phase;
		uint32 m_phaseTimer;
		uint64 m_activatorGuid;
	};

	class DefiasGunpowderAI : public GameObjectAIScript
	{
	public:
		DefiasGunpowderAI(GameObject* go) : GameObjectAIScript(go)
		{
		}

		static GameObjectAIScript* Create(GameObject* go) { return new DefiasGunpowderAI(go); }

		void OnCreate()
		{
			SetGameObjectReady(_gameobject);
		}

		void OnSpawn()
		{
			SetGameObjectReady(_gameobject);
		}
	};

	class MrSmiteChestAI : public GameObjectAIScript
	{
	public:
		MrSmiteChestAI(GameObject* go) : GameObjectAIScript(go)
		{
		}

		static GameObjectAIScript* Create(GameObject* go) { return new MrSmiteChestAI(go); }

		void OnCreate()
		{
			SetChestReady(_gameobject);
		}

		void OnSpawn()
		{
			SetChestReady(_gameobject);
		}
	};

	class MysteriousDeadminesChestAI : public GameObjectAIScript
	{
	public:
		MysteriousDeadminesChestAI(GameObject* go) : GameObjectAIScript(go)
		{
		}

		static GameObjectAIScript* Create(GameObject* go) { return new MysteriousDeadminesChestAI(go); }

		void OnCreate()
		{
			SetGameObjectReady(_gameobject);
		}

		void OnSpawn()
		{
			SetGameObjectReady(_gameobject);
		}

		void OnActivate(Player* player)
		{
			if(player == NULL)
				return;

			if(player->GetQuestLogForEntry(QUEST_YOUR_FORTUNE_AWAITS_YOU) == NULL && !player->HasFinishedQuest(QUEST_YOUR_FORTUNE_AWAITS_YOU))
				player->BroadcastMessage("The chest is strangely quiet.");
		}
	};

	class RhahkZorAI : public DeadminesCombatAI
	{
	public:
		ADD_CREATURE_FACTORY_FUNCTION(RhahkZorAI);

		RhahkZorAI(Creature* creature) : DeadminesCombatAI(creature)
		{
			Reset();
		}

		void OnCombatStart(Unit* mTarget)
		{
			_unit->SendChatMessage(CHAT_MSG_MONSTER_YELL, LANG_UNIVERSAL, "VanCleef pay big for your heads!");
			_unit->PlaySoundToSet(SOUND_RHAHK_ZOR_AGGRO);
			PullRhahkZorGuards(mTarget, _unit->GetMapMgr());
			DeadminesCombatAI::OnCombatStart(mTarget);
		}

		void OnDied(Unit* mKiller)
		{
			OpenFactoryDoor(_unit->GetMapMgr());
			DeadminesCombatAI::OnDied(mKiller);
		}

		void AIUpdate()
		{
			Unit* target = _unit->GetAIInterface()->GetNextTarget();
			if(target == NULL)
				return;

			const uint32 diff = GetUpdateFrequency();
			if(SpellReady(m_slamTimer, diff) && CastSpellOnTargetByHash(target, SPELL_HASH_RHAHK_ZOR_SLAM))
				m_slamTimer = 9000 + RandomUInt(3000);
		}

	protected:
		void Reset()
		{
			m_slamTimer = 7000;
		}

	private:
		uint32 m_slamTimer;
	};

	class SneedsShredderAI : public DeadminesCombatAI
	{
	public:
		ADD_CREATURE_FACTORY_FUNCTION(SneedsShredderAI);

		SneedsShredderAI(Creature* creature) : DeadminesCombatAI(creature)
		{
			Reset();
		}

		void OnDied(Unit* mKiller)
		{
			MapMgr* mapMgr = _unit->GetMapMgr();
			DeadminesState& state = GetDeadminesState(mapMgr);
			if(!state.sneedSpawned && mapMgr != NULL)
			{
				state.sneedSpawned = true;
				Creature* sneed = mapMgr->GetInterface()->SpawnCreature(
					CN_SNEED,
					_unit->GetPositionX(),
					_unit->GetPositionY(),
					_unit->GetPositionZ(),
					_unit->GetOrientation(),
					true,
					false,
					_unit->GetUInt32Value(UNIT_FIELD_FACTIONTEMPLATE),
					25);

				if(sneed != NULL && mKiller != NULL)
					sneed->GetAIInterface()->AttackReaction(mKiller, 1, 0);
			}

			DeadminesCombatAI::OnDied(mKiller);
		}

		void AIUpdate()
		{
			Unit* target = _unit->GetAIInterface()->GetNextTarget();
			if(target == NULL)
				return;

			const uint32 diff = GetUpdateFrequency();
			if(SpellReady(m_distractingPainTimer, diff) && CastSpellOnTargetByHash(target, SPELL_HASH_DISTRACTING_PAIN))
			{
				m_distractingPainTimer = 10000 + RandomUInt(3000);
				return;
			}

			if(SpellReady(m_terrifyTimer, diff) && CastSpellOnTargetByHash(target, SPELL_HASH_TERRIFY))
				m_terrifyTimer = 16000 + RandomUInt(4000);
		}

	protected:
		void Reset()
		{
			m_distractingPainTimer = 5000;
			m_terrifyTimer = 11000;
		}

	private:
		uint32 m_distractingPainTimer;
		uint32 m_terrifyTimer;
	};

	class SneedAI : public DeadminesCombatAI
	{
	public:
		ADD_CREATURE_FACTORY_FUNCTION(SneedAI);

		SneedAI(Creature* creature) : DeadminesCombatAI(creature)
		{
			Reset();
		}

		void OnDied(Unit* mKiller)
		{
			OpenMastDoor(_unit->GetMapMgr());
			DeadminesCombatAI::OnDied(mKiller);
		}

		void AIUpdate()
		{
			Unit* target = _unit->GetAIInterface()->GetNextTarget();
			if(target == NULL)
				return;

			const uint32 diff = GetUpdateFrequency();
			if(SpellReady(m_disarmTimer, diff) && CastSpellOnTargetByHash(target, SPELL_HASH_DISARM))
				m_disarmTimer = 10000 + RandomUInt(4000);
		}

	protected:
		void Reset()
		{
			m_disarmTimer = 7000;
		}

	private:
		uint32 m_disarmTimer;
	};

	class GilnidAI : public DeadminesCombatAI
	{
	public:
		ADD_CREATURE_FACTORY_FUNCTION(GilnidAI);

		GilnidAI(Creature* creature) : DeadminesCombatAI(creature)
		{
			Reset();
		}

		void OnDied(Unit* mKiller)
		{
			OpenFoundryDoor(_unit->GetMapMgr());
			DeadminesCombatAI::OnDied(mKiller);
		}

		void AIUpdate()
		{
			Unit* target = _unit->GetAIInterface()->GetNextTarget();
			if(target == NULL)
				return;

			const uint32 diff = GetUpdateFrequency();
			if(SpellReady(m_moltenMetalTimer, diff) && CastSpellOnTargetByHash(target, SPELL_HASH_MOLTEN_METAL))
				m_moltenMetalTimer = 9000 + RandomUInt(4000);
		}

	protected:
		void Reset()
		{
			m_moltenMetalTimer = 6000;
		}

	private:
		uint32 m_moltenMetalTimer;
	};

	class MrSmiteAI : public DeadminesCombatAI
	{
	public:
		ADD_CREATURE_FACTORY_FUNCTION(MrSmiteAI);

		MrSmiteAI(Creature* creature) : DeadminesCombatAI(creature), m_phase(0), m_transitioning(false), m_transitionStep(0), m_transitionTargetGuid(0), m_announcedAggro(false)
		{
			Reset();
		}

		void OnCombatStart(Unit* mTarget)
		{
			if(!m_announcedAggro)
			{
				_unit->SendChatMessage(CHAT_MSG_MONSTER_SAY, LANG_UNIVERSAL, "We're under attack! Repel the invaders!");
				_unit->PlaySoundToSet(SOUND_SMITE_AGGRO_2);
				m_announcedAggro = true;
			}

			DeadminesCombatAI::OnCombatStart(mTarget);
		}

		void OnDamageTaken(Unit* mAttacker, float fAmount)
		{
			(void)mAttacker;
			if(fAmount < 1.0f || m_transitioning)
				return;

			if(_unit->GetHealthPct() <= 66.0f && m_phase == 0)
			{
				BeginWeaponTransition("You land lovers are tougher than I thought! I'll have to improvise.", SOUND_SMITE_PHASE_1);
				m_phase = 1;
			}
			else if(_unit->GetHealthPct() <= 33.0f && m_phase == 1)
			{
				BeginWeaponTransition("Now you're making me angry!", SOUND_SMITE_PHASE_2);
				m_phase = 2;
			}
		}

		void AIUpdate()
		{
			const uint32 diff = GetUpdateFrequency();
			if(m_transitioning)
			{
				UpdateTransition(diff);
				return;
			}

			Unit* target = _unit->GetAIInterface()->GetNextTarget();
			if(target == NULL)
				return;

			if(SpellReady(m_slamTimer, diff) && CastSpellOnTargetByHash(target, SPELL_HASH_SMITE_SLAM))
			{
				m_slamTimer = (m_phase == 0) ? 9000 : 7000;
				if(m_phase > 0)
					return;
			}

			if(SpellReady(m_trashTimer, diff) && CastSpellOnSelf(SPELL_SMITE_TRASH))
				m_trashTimer = 6000 + RandomUInt(9500);

			if(SpellReady(m_nimbleReflexesTimer, diff) && CastSpellOnSelf(SPELL_NIMBLE_REFLEXES))
				m_nimbleReflexesTimer = 27300 + RandomUInt(32800);
		}

	protected:
		void Reset()
		{
			m_phase = 0;
			m_transitioning = false;
			m_transitionStep = 0;
			m_slamTimer = 7000;
			m_trashTimer = 5000 + RandomUInt(4000);
			m_nimbleReflexesTimer = 15500 + RandomUInt(16100);
			m_transitionTimer = 0;
			m_transitionTargetGuid = 0;
			m_announcedAggro = false;
			m_transitionMoveRetries = 0;
			EquipWeaponSet(0);
			_unit->SetStandState(STANDSTATE_STAND);
		}

	private:
		void EquipWeaponSet(uint32 phase)
		{
			switch(phase)
			{
			case 0:
				_unit->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY, SMITE_EQUIP_SWORD);
				_unit->SetUInt32Value(UNIT_VIRTUAL_ITEM_INFO, SMITE_SWORD_INFO);
				_unit->SetUInt32Value(UNIT_VIRTUAL_ITEM_INFO_01, 2);
				_unit->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY_01, 0);
				_unit->SetUInt32Value(UNIT_VIRTUAL_ITEM_INFO_02, 0);
				_unit->SetUInt32Value(UNIT_VIRTUAL_ITEM_INFO_03, 0);
				break;
			case 1:
				_unit->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY, SMITE_EQUIP_AXE);
				_unit->SetUInt32Value(UNIT_VIRTUAL_ITEM_INFO, SMITE_AXE_INFO);
				_unit->SetUInt32Value(UNIT_VIRTUAL_ITEM_INFO_01, 2);
				_unit->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY_01, SMITE_EQUIP_AXE);
				_unit->SetUInt32Value(UNIT_VIRTUAL_ITEM_INFO_02, SMITE_AXE_INFO);
				_unit->SetUInt32Value(UNIT_VIRTUAL_ITEM_INFO_03, 2);
				break;
			default:
				_unit->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY, SMITE_EQUIP_MACE);
				_unit->SetUInt32Value(UNIT_VIRTUAL_ITEM_INFO, SMITE_MACE_INFO);
				_unit->SetUInt32Value(UNIT_VIRTUAL_ITEM_INFO_01, 2);
				_unit->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY_01, 0);
				_unit->SetUInt32Value(UNIT_VIRTUAL_ITEM_INFO_02, 0);
				_unit->SetUInt32Value(UNIT_VIRTUAL_ITEM_INFO_03, 0);
				break;
			}
		}

		void BeginWeaponTransition(const char* text, uint32 soundId)
		{
			Unit* target = _unit->GetAIInterface()->GetNextTarget();
			if(CastSpellOnSelfByHash(SPELL_HASH_SMITE_STOMP))
			{
				_unit->SendChatMessage(CHAT_MSG_MONSTER_SAY, LANG_UNIVERSAL, text);
				_unit->PlaySoundToSet(soundId);
			}

			m_transitioning = true;
			m_transitionStep = 0;
			m_transitionTimer = 1200;
			m_transitionTargetGuid = (target != NULL) ? target->GetGUID() : 0;
			m_transitionMoveRetries = 0;
			_unit->GetAIInterface()->StopMovement(0);
			_unit->GetAIInterface()->setCurrentAgent(AGENT_NULL);
			_unit->GetAIInterface()->setMoveType(MOVEMENTTYPE_FORWARDTHANSTOP);
			_unit->GetAIInterface()->SetAllowedToEnterCombat(false);
			_unit->GetAIInterface()->SetAIState(STATE_SCRIPTMOVE);
			_unit->SetStandState(STANDSTATE_STAND);
			if(target != NULL)
				_unit->smsg_AttackStop(target);
			_unit->setAttackTimer(9000, false);
			_unit->setAttackTimer(9000, true);
		}

		void UpdateTransition(uint32 diff)
		{
			if(!SpellReady(m_transitionTimer, diff))
				return;

			switch(m_transitionStep)
			{
			case 0:
				if(_unit->CalcDistance(kMrSmiteChestPosition.x, kMrSmiteChestPosition.y, kMrSmiteChestPosition.z) <= 1.5f)
				{
					_unit->GetAIInterface()->StopMovement(0);
					_unit->SetFacing(kMrSmiteChestPosition.o);
					_unit->SetStandState(STANDSTATE_KNEEL);
					m_transitionStep = 2;
					m_transitionTimer = 1800;
					break;
				}

				_unit->SetFacing(kMrSmiteChestPosition.o);
				_unit->GetAIInterface()->MoveTo(kMrSmiteChestPosition.x, kMrSmiteChestPosition.y, kMrSmiteChestPosition.z, kMrSmiteChestPosition.o);
				m_transitionStep = 1;
				m_transitionTimer = 250;
				break;
			case 1:
				if (_unit->CalcDistance(kMrSmiteChestPosition.x, kMrSmiteChestPosition.y, kMrSmiteChestPosition.z) > 1.5f)
				{
					if (m_transitionMoveRetries < 10)
					{
						if((m_transitionMoveRetries % 2) == 0)
							_unit->GetAIInterface()->MoveTo(kMrSmiteChestPosition.x, kMrSmiteChestPosition.y, kMrSmiteChestPosition.z, kMrSmiteChestPosition.o);

						++m_transitionMoveRetries;
						m_transitionTimer = 250;
						return;
					}
				}

				_unit->GetAIInterface()->StopMovement(0);
				_unit->SetFacing(kMrSmiteChestPosition.o);
				_unit->SetStandState(STANDSTATE_KNEEL);
				m_transitionStep = 2;
				m_transitionTimer = 1800;
				break;
			case 2:
				EquipWeaponSet(m_phase);
				m_transitionStep = 3;
				m_transitionTimer = 900;
				break;
			case 3:
				_unit->SetStandState(STANDSTATE_STAND);
				m_transitionStep = 4;
				m_transitionTimer = 300;
				break;
			default:
				_unit->GetAIInterface()->SetAllowedToEnterCombat(true);
				_unit->GetAIInterface()->SetAIState(STATE_ATTACKING);
				_unit->GetAIInterface()->setCurrentAgent(AGENT_MELEE);
				m_transitioning = false;
				m_transitionStep = 0;
				m_transitionTimer = 0;
				Unit* target = (m_transitionTargetGuid != 0 && _unit->GetMapMgr() != NULL) ? _unit->GetMapMgr()->GetUnit(m_transitionTargetGuid) : _unit->GetAIInterface()->GetNextTarget();
				m_transitionTargetGuid = 0;
				if(target != NULL)
					_unit->GetAIInterface()->AttackReaction(target, 1, 0);
				break;
			}
		}

		uint32 m_slamTimer;
		uint32 m_trashTimer;
		uint32 m_nimbleReflexesTimer;
		uint32 m_transitionTimer;
		uint32 m_phase;
		uint32 m_transitionStep;
		uint32 m_transitionMoveRetries;
		uint64 m_transitionTargetGuid;
		bool m_announcedAggro;
		bool m_transitioning;
	};

	class CaptainGreenskinAI : public DeadminesCombatAI
	{
	public:
		ADD_CREATURE_FACTORY_FUNCTION(CaptainGreenskinAI);

		CaptainGreenskinAI(Creature* creature) : DeadminesCombatAI(creature)
		{
			Reset();
		}

		void AIUpdate()
		{
			Unit* target = _unit->GetAIInterface()->GetNextTarget();
			if(target == NULL)
				return;

			const uint32 diff = GetUpdateFrequency();
			if(SpellReady(m_harpoonTimer, diff) && CastSpellOnTargetByHash(target, SPELL_HASH_POISONED_HARPOON))
				m_harpoonTimer = 9000 + RandomUInt(4000);
		}

	protected:
		void Reset()
		{
			m_harpoonTimer = 6000;
		}

	private:
		uint32 m_harpoonTimer;
	};

	class VanCleefAI : public DeadminesCombatAI
	{
	public:
		ADD_CREATURE_FACTORY_FUNCTION(VanCleefAI);

		VanCleefAI(Creature* creature) : DeadminesCombatAI(creature), m_phase(0)
		{
			Reset();
		}

		void OnCombatStart(Unit* mTarget)
		{
			_unit->SendChatMessage(CHAT_MSG_MONSTER_SAY, LANG_UNIVERSAL, "None may challenge the brotherhood.");
			_unit->PlaySoundToSet(SOUND_VANCLEEF_AGGRO);
			DeadminesCombatAI::OnCombatStart(mTarget);
		}

		void OnTargetDied(Unit* mTarget)
		{
			if(mTarget == NULL)
				return;

			if(mTarget->GetTypeId() == TYPEID_PLAYER)
			{
				char msg[200];
				sprintf(msg, "And stay down, %s.", static_cast<Player*>(mTarget)->GetName());
				_unit->SendChatMessage(CHAT_MSG_MONSTER_SAY, LANG_UNIVERSAL, msg);
				_unit->PlaySoundToSet(SOUND_VANCLEEF_SLAY);
			}
			else if(mTarget->IsPet())
			{
				char msg[200];
				sprintf(msg, "And stay down, %s.", static_cast<Pet*>(mTarget)->GetName().c_str());
				_unit->SendChatMessage(CHAT_MSG_MONSTER_SAY, LANG_UNIVERSAL, msg);
				_unit->PlaySoundToSet(SOUND_VANCLEEF_SLAY);
			}
		}

		void OnDamageTaken(Unit* mAttacker, float fAmount)
		{
			(void)mAttacker;
			if(fAmount < 1.0f)
				return;

			if(_unit->GetHealthPct() <= 75.0f && m_phase == 0)
			{
				m_phase = 1;
				_unit->SendChatMessage(CHAT_MSG_MONSTER_SAY, LANG_UNIVERSAL, "Lap dogs, all of you.");
				_unit->PlaySoundToSet(SOUND_VANCLEEF_PHASE_1);
			}
			else if(_unit->GetHealthPct() <= 50.0f && m_phase == 1)
			{
				m_phase = 2;
				_unit->SendChatMessage(CHAT_MSG_MONSTER_SAY, LANG_UNIVERSAL, "Fools! Our cause is righteous.");
				_unit->PlaySoundToSet(SOUND_VANCLEEF_PHASE_2);
				SummonBlackguards();
			}
			else if(_unit->GetHealthPct() <= 25.0f && m_phase == 2)
			{
				m_phase = 3;
				_unit->SendChatMessage(CHAT_MSG_MONSTER_SAY, LANG_UNIVERSAL, "The brotherhood shall remain.");
				_unit->PlaySoundToSet(SOUND_VANCLEEF_PHASE_3);
				SummonBlackguards();
			}
		}

		void AIUpdate()
		{
			Unit* target = _unit->GetAIInterface()->GetNextTarget();
			if(target == NULL)
				return;

			const uint32 diff = GetUpdateFrequency();
			if(SpellReady(m_thrashTimer, diff) && CastSpellOnSelfByHash(SPELL_HASH_THRASH))
				m_thrashTimer = 10000 + RandomUInt(5000);
		}

		void OnDied(Unit* mKiller)
		{
			ClearDeadminesState(_unit->GetMapMgr());
			DeadminesCombatAI::OnDied(mKiller);
		}

	protected:
		void Reset()
		{
			m_phase = 0;
			m_thrashTimer = 8000;
		}

	private:
		void SummonBlackguards()
		{
			MapMgr* mapMgr = _unit->GetMapMgr();
			if(mapMgr == NULL)
				return;

			static const float offsets[2][4] =
			{
				{ 2.5f, 2.0f, 0.0f, 0.0f },
				{ -2.5f, 2.0f, 0.0f, 0.0f },
			};

			for(uint32 i = 0; i < 2; ++i)
			{
				Creature* add = mapMgr->GetInterface()->SpawnCreature(
					CN_DEFIAS_BLACKGUARD,
					_unit->GetPositionX() + offsets[i][0],
					_unit->GetPositionY() + offsets[i][1],
					_unit->GetPositionZ() + offsets[i][2],
					_unit->GetOrientation() + offsets[i][3],
					true,
					false,
					_unit->GetUInt32Value(UNIT_FIELD_FACTIONTEMPLATE),
					25);

				if(add != NULL && _unit->GetAIInterface()->GetNextTarget() != NULL)
					add->GetAIInterface()->AttackReaction(_unit->GetAIInterface()->GetNextTarget(), 1, 0);
			}
		}

		uint32 m_thrashTimer;
		uint32 m_phase;
	};
}

void SetupDeadmines(ScriptMgr* mgr)
{
	mgr->register_creature_script(CN_RHAHK_ZOR, &RhahkZorAI::Create);
	mgr->register_creature_script(CN_SNEEDS_SHREDDER, &SneedsShredderAI::Create);
	mgr->register_creature_script(CN_SNEED, &SneedAI::Create);
	mgr->register_creature_script(CN_GILNID, &GilnidAI::Create);
	mgr->register_creature_script(CN_MR_SMITE, &MrSmiteAI::Create);
	mgr->register_creature_script(CN_CAPTAIN_GREENSKIN, &CaptainGreenskinAI::Create);
	mgr->register_creature_script(CN_EDWIN_VANCLEEF, &VanCleefAI::Create);

	mgr->register_gameobject_script(GO_FACTORY_DOOR, &FactoryDoorAI::Create);
	mgr->register_gameobject_script(GO_IRON_CLAD_DOOR, &IronCladDoorAI::Create);
	mgr->register_gameobject_script(GO_MAST_ROOM_DOOR, &MastDoorAI::Create);
	mgr->register_gameobject_script(GO_FOUNDRY_DOOR, &FoundryDoorAI::Create);
	mgr->register_gameobject_script(GO_FACTORY_DOOR_LEVER, &FactoryLeverAI::Create);
	mgr->register_gameobject_script(GO_DOOR_LEVER, &DoorLeverAI::Create);
	mgr->register_gameobject_script(GO_MAST_ROOM_LEVER, &MastLeverAI::Create);
	mgr->register_gameobject_script(GO_FOUNDRY_LEVER, &FoundryLeverAI::Create);
	mgr->register_gameobject_script(GO_DEFIAS_CANNON, &DefiasCannonAI::Create);
	mgr->register_gameobject_script(GO_DEFIAS_GUNPOWDER, &DefiasGunpowderAI::Create);
	mgr->register_gameobject_script(GO_MR_SMITE_CHEST, &MrSmiteChestAI::Create);
	mgr->register_gameobject_script(GO_MYSTERIOUS_DEADMINES_CHEST, &MysteriousDeadminesChestAI::Create);
}
