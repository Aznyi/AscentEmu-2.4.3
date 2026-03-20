#include "StdAfx.h"
#include "Setup.h"
#include "BattlegroundMgr.h"
#include "AlteracValley.h"
#include "MapMgr.h"
#include "Spell.h"

#define CN_VANNDAR_STORMPIKE 11948
#define CN_DREKTHAR 11946
#define CN_BALINDA_STONEHEARTH 11949
#define CN_GALVANGAR 11947
#define CN_DRAKAN 12121
#define CN_DUROS 12122
#define GOSSIP_INTID_AV_RESCUE 9001
#define GOSSIP_INTID_AV_DEPLOY 9002

#define CN_DUN_BALDAR_NORTH_MARSHAL 14762
#define CN_DUN_BALDAR_SOUTH_MARSHAL 14763
#define CN_ICEWING_MARSHAL 14764
#define CN_STONEHEARTH_MARSHAL 14765
#define CN_EAST_FROSTWOLF_WARMASTER 14772
#define CN_ICEBLOOD_WARMASTER 14773
#define CN_TOWER_POINT_WARMASTER 14776
#define CN_WEST_FROSTWOLF_WARMASTER 14777
#define CN_STORMPIKE_BOWMAN 13358
#define CN_FROSTWOLF_BOWMAN 13359
#define AV_SPELL_BOW_SHOOT 22121
#define CN_WC_SLIDORE 13438
#define CN_WC_VIPORE 13439
#define CN_WC_ICHMAN 13437
#define CN_WC_GUSE 13179
#define CN_WC_JEZTOR 13180
#define CN_WC_MULVERICK 13181

#define AV_SPELL_CLEAVE 15284
#define AV_SPELL_WHIRLWIND 15589
#define AV_SPELL_MORTAL_STRIKE 16856
#define AV_SPELL_THUNDERCLAP 15588
#define AV_SPELL_AVATAR 19135
#define AV_SPELL_STORM_BOLT 20685
#define AV_SPELL_KNOCKDOWN 19128
#define AV_SPELL_FEAR 19134
#define AV_SPELL_FRENZY 8269
#define AV_SPELL_CHAIN_LIGHTNING 16033
#define AV_SPELL_FROSTBOLT 20822
#define AV_SPELL_FIREBALL 20823
#define AV_SPELL_CONE_OF_COLD 20828
#define AV_SPELL_SUMMON_WATER_ELEMENTAL 45067

static bool AVIsAllianceCaptainSupportEntry(uint32 entry)
{
	return entry == CN_STORMPIKE_BOWMAN ||
		entry == CN_DUN_BALDAR_NORTH_MARSHAL ||
		entry == CN_DUN_BALDAR_SOUTH_MARSHAL ||
		entry == CN_ICEWING_MARSHAL ||
		entry == CN_STONEHEARTH_MARSHAL;
}

static bool AVIsHordeCaptainSupportEntry(uint32 entry)
{
	return entry == CN_FROSTWOLF_BOWMAN ||
		entry == CN_EAST_FROSTWOLF_WARMASTER ||
		entry == CN_ICEBLOOD_WARMASTER ||
		entry == CN_TOWER_POINT_WARMASTER ||
		entry == CN_WEST_FROSTWOLF_WARMASTER;
}

static bool AVIsAllianceWingCommanderEntry(uint32 entry)
{
	return entry == CN_WC_SLIDORE || entry == CN_WC_VIPORE || entry == CN_WC_ICHMAN;
}

static bool AVIsHordeWingCommanderEntry(uint32 entry)
{
	return entry == CN_WC_GUSE || entry == CN_WC_JEZTOR || entry == CN_WC_MULVERICK;
}

static const char* AVGetWingCommanderRescueText(uint32 entry)
{
	switch(entry)
	{
	case CN_WC_SLIDORE:   return "Free Wing Commander Slidore and send her back to Dun Baldar.";
	case CN_WC_VIPORE:    return "Free Wing Commander Vipore and send her back to Dun Baldar.";
	case CN_WC_ICHMAN:    return "Free Wing Commander Ichman and send him back to Dun Baldar.";
	case CN_WC_GUSE:      return "Free Wing Commander Guse and send him back to Frostwolf Keep.";
	case CN_WC_JEZTOR:    return "Free Wing Commander Jeztor and send him back to Frostwolf Keep.";
	case CN_WC_MULVERICK: return "Free Wing Commander Mulverick and send him back to Frostwolf Keep.";
	default:              return "Rescue the wing commander.";
	}
}

static const char* AVGetWingCommanderDeployText(uint32 entry)
{
	switch(entry)
	{
	case CN_WC_SLIDORE:   return "Order Slidore's gryphon riders to strike Frostwolf Graveyard.";
	case CN_WC_VIPORE:    return "Order Vipore's gryphon riders to strike Frostwolf Village.";
	case CN_WC_ICHMAN:    return "Order Ichman's gryphon riders to strike Drek'Thar's fortress.";
	case CN_WC_GUSE:      return "Order Guse's war riders to strike Stormpike Graveyard.";
	case CN_WC_JEZTOR:    return "Order Jeztor's war riders to strike the Dun Baldar bunker line.";
	case CN_WC_MULVERICK: return "Order Mulverick's war riders to strike Stormpike Aid Station.";
	default:              return "Launch the air strike.";
	}
}

static uint32 GetAVDefenderAura(uint32 entry)
{
	switch(entry)
	{
		// Alliance Marshals
		case CN_DUN_BALDAR_NORTH_MARSHAL: return 45828;
		case CN_DUN_BALDAR_SOUTH_MARSHAL: return 45829;
		case CN_ICEWING_MARSHAL:          return 45830;
		case CN_STONEHEARTH_MARSHAL:      return 45831;

		// Horde Warmasters
		case CN_EAST_FROSTWOLF_WARMASTER: return 45826;
		case CN_ICEBLOOD_WARMASTER:       return 45822;
		case CN_TOWER_POINT_WARMASTER:    return 45823;
		case CN_WEST_FROSTWOLF_WARMASTER: return 45824;
	}

	return 0;
}

class AlteracValleyCreatureAI : public CreatureAIScript
{
public:
	AlteracValleyCreatureAI(Creature* pCreature, float leashRange) : CreatureAIScript(pCreature)
	{
		m_leashRangeSq = leashRange * leashRange;
		m_homeX = pCreature->GetSpawnX();
		m_homeY = pCreature->GetSpawnY();
		m_homeZ = pCreature->GetSpawnZ();
		m_homeO = pCreature->GetSpawnO();
	}

	void OnCombatStart(Unit* mTarget)
	{
		if(!IsAlteracValley())
			return;

		RegisterAIUpdateEvent(_unit->GetUInt32Value(UNIT_FIELD_BASEATTACKTIME));
	}

	void OnCombatStop(Unit* mTarget)
	{
		RemoveAIUpdateEvent();
	}

	void OnDied(Unit* mKiller)
	{
		RemoveAIUpdateEvent();
	}

	void AIUpdate()
	{
		if(!IsAlteracValley())
		{
			RemoveAIUpdateEvent();
			return;
		}

		if(NeedsLeashReset())
		{
			EvadeToHome();
			return;
		}

		DoBattlefieldAI();
	}

protected:
	virtual void DoBattlefieldAI() {}

	bool IsAlteracValley()
	{
		MapMgr* mgr = _unit->GetMapMgr();
		return mgr != NULL && mgr->m_battleground != NULL && mgr->m_battleground->GetType() == BATTLEGROUND_ALTERAC_VALLEY;
	}

	bool NeedsLeashReset()
	{
		Unit* target = _unit->GetAIInterface()->GetNextTarget();
		if(target == NULL)
			return false;

		float dx = _unit->GetPositionX() - m_homeX;
		float dy = _unit->GetPositionY() - m_homeY;
		return ((dx * dx) + (dy * dy)) > m_leashRangeSq;
	}

	void EvadeToHome()
	{
		_unit->GetAIInterface()->HandleEvent(EVENT_LEAVECOMBAT, _unit, 0);
		_unit->GetAIInterface()->SetAllowedToEnterCombat(true);
		_unit->SetUInt32Value(UNIT_FIELD_HEALTH, _unit->GetUInt32Value(UNIT_FIELD_MAXHEALTH));
		if(_unit->GetPowerType() == POWER_TYPE_MANA)
			_unit->SetUInt32Value(UNIT_FIELD_POWER1, _unit->GetUInt32Value(UNIT_FIELD_MAXPOWER1));
		_unit->GetAIInterface()->MoveTo(m_homeX, m_homeY, m_homeZ, m_homeO);
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

	bool CastSpellOnTarget(uint32 spellId)
	{
		Unit* target = _unit->GetAIInterface()->GetNextTarget();
		SpellEntry* sp = dbcSpell.LookupEntry(spellId);
		if(target == NULL || sp == NULL || _unit->GetCurrentSpell() != NULL)
			return false;

		_unit->CastSpell(target, sp, false);
		return true;
	}

	bool CastSpellOnSelf(uint32 spellId)
	{
		SpellEntry* sp = dbcSpell.LookupEntry(spellId);
		if(sp == NULL || _unit->GetCurrentSpell() != NULL)
			return false;

		_unit->CastSpell(_unit, sp, false);
		return true;
	}

	void SendRandomCombatBark(const char* const* lines, uint32 count)
	{
		if(lines == NULL || count == 0)
			return;

		const char* text = lines[RandomUInt(count - 1)];
		if(text != NULL && text[0] != 0)
			_unit->SendChatMessage(CHAT_MSG_MONSTER_YELL, LANG_UNIVERSAL, text);
	}

	uint32 GetUpdateFrequency()
	{
		uint32 freq = _unit->GetUInt32Value(UNIT_FIELD_BASEATTACKTIME);
		return (freq == 0) ? 2000 : freq;
	}

	float m_leashRangeSq;
	float m_homeX;
	float m_homeY;
	float m_homeZ;
	float m_homeO;
};

class AlteracValleyBossAI : public AlteracValleyCreatureAI
{
public:
	AlteracValleyBossAI(Creature* pCreature, float leashRange) : AlteracValleyCreatureAI(pCreature, leashRange)
	{
		m_primaryTimer = 6000;
		m_secondaryTimer = 12000;
		m_supportTimer = 18000;
		m_frenzied = false;
	}

	void OnCombatStart(Unit* mTarget)
	{
		AlteracValleyCreatureAI::OnCombatStart(mTarget);
		m_primaryTimer = 6000;
		m_secondaryTimer = 12000;
		m_supportTimer = 18000;
		m_frenzied = false;
		SendAggroBark();
		CallBaseDefenders(mTarget);
	}

protected:
	virtual uint32 GetPrimarySpell() = 0;
	virtual uint32 GetSecondarySpell() = 0;
	virtual uint32 GetEmergencySpell() = 0;
	virtual bool DefenderEntry(uint32 entry) = 0;
	virtual const char* const* GetAggroBarks(uint32& count)
	{
		count = 0;
		return NULL;
	}

	void DoBattlefieldAI()
	{
		uint32 diff = GetUpdateFrequency();
		if(_unit->GetAIInterface()->GetNextTarget() == NULL)
			return;

		if(!_unit->GetCurrentSpell() && !_unit->isCasting())
		{
			if(!m_frenzied && _unit->GetHealthPct() < 25 && CastSpellOnSelf(GetEmergencySpell()))
			{
				m_frenzied = true;
				return;
			}

			if(SpellReady(m_primaryTimer, diff) && CastSpellOnTarget(GetPrimarySpell()))
			{
				m_primaryTimer = 12000;
				return;
			}

			if(SpellReady(m_secondaryTimer, diff) && CastSpellOnTarget(GetSecondarySpell()))
			{
				m_secondaryTimer = 18000;
				return;
			}
		}
		else
		{
			SpellReady(m_primaryTimer, diff);
			SpellReady(m_secondaryTimer, diff);
		}

		if(SpellReady(m_supportTimer, diff))
		{
			CallBaseDefenders(_unit->GetAIInterface()->GetNextTarget());
			m_supportTimer = 18000;
		}
	}

	void CallBaseDefenders(Unit* target)
	{
		if(target == NULL)
			return;

		for(Object::InRangeSet::iterator itr = _unit->GetInRangeSetBegin(); itr != _unit->GetInRangeSetEnd(); ++itr)
		{
			if((*itr) == NULL || !(*itr)->IsCreature())
				continue;

			Creature* guard = static_cast<Creature*>(*itr);
			if(guard->isDead() || guard->GetEntry() == _unit->GetEntry() || !DefenderEntry(guard->GetEntry()))
				continue;

			guard->GetAIInterface()->AttackReaction(target, 1, 0);
		}
	}

	void SendAggroBark()
	{
		uint32 count = 0;
		const char* const* lines = GetAggroBarks(count);
		if(lines != NULL && count != 0)
			SendRandomCombatBark(lines, count);
	}

	uint32 m_primaryTimer;
	uint32 m_secondaryTimer;
	uint32 m_supportTimer;
	bool m_frenzied;
};

class AlteracValleyCaptainAI : public AlteracValleyCreatureAI
{
public:
	AlteracValleyCaptainAI(Creature* pCreature, float leashRange, uint32 primarySpell, uint32 secondarySpell, uint32 selfSpell, uint32 extraSpell = 0) : AlteracValleyCreatureAI(pCreature, leashRange)
	{
		m_primarySpell = primarySpell;
		m_secondarySpell = secondarySpell;
		m_selfSpell = selfSpell;
		m_extraSpell = extraSpell;
		m_primaryTimer = 8000;
		m_secondaryTimer = 14000;
		m_selfTimer = 20000;
		m_extraTimer = 26000;
		m_supportTimer = 12000;
	}

	void OnCombatStart(Unit* mTarget)
	{
		AlteracValleyCreatureAI::OnCombatStart(mTarget);
		m_primaryTimer = 8000;
		m_secondaryTimer = 14000;
		m_selfTimer = 20000;
		m_extraTimer = 26000;
		m_supportTimer = 12000;
		SendAggroBark();
		CallNearbySupport(mTarget);
	}

protected:
	virtual bool IsSupportEntry(uint32 entry) = 0;
	virtual const char* const* GetAggroBarks(uint32& count)
	{
		count = 0;
		return NULL;
	}

	void DoBattlefieldAI()
	{
		uint32 diff = GetUpdateFrequency();
		if(_unit->GetAIInterface()->GetNextTarget() == NULL)
			return;

		if(_unit->GetCurrentSpell() == NULL)
		{
			if(SpellReady(m_selfTimer, diff) && CastSpellOnSelf(m_selfSpell))
			{
				m_selfTimer = 30000;
				return;
			}

			if(m_extraSpell != 0 && SpellReady(m_extraTimer, diff) &&
				((m_extraSpell == AV_SPELL_WHIRLWIND || m_extraSpell == AV_SPELL_CONE_OF_COLD || m_extraSpell == AV_SPELL_SUMMON_WATER_ELEMENTAL)
					? CastSpellOnSelf(m_extraSpell)
					: CastSpellOnTarget(m_extraSpell)))
			{
				m_extraTimer = 32000;
				return;
			}

			if(SpellReady(m_primaryTimer, diff) && CastSpellOnTarget(m_primarySpell))
			{
				m_primaryTimer = 12000;
				return;
			}

			if(SpellReady(m_secondaryTimer, diff) && CastSpellOnTarget(m_secondarySpell))
			{
				m_secondaryTimer = 16000;
				return;
			}
		}
		else
		{
			SpellReady(m_selfTimer, diff);
			SpellReady(m_extraTimer, diff);
			SpellReady(m_primaryTimer, diff);
			SpellReady(m_secondaryTimer, diff);
		}

		if(SpellReady(m_supportTimer, diff))
		{
			CallNearbySupport(_unit->GetAIInterface()->GetNextTarget());
			m_supportTimer = 18000;
		}
	}

	void CallNearbySupport(Unit* target)
	{
		if(target == NULL)
			return;

		for(Object::InRangeSet::iterator itr = _unit->GetInRangeSetBegin(); itr != _unit->GetInRangeSetEnd(); ++itr)
		{
			if((*itr) == NULL || !(*itr)->IsCreature())
				continue;

			Creature* ally = static_cast<Creature*>(*itr);
			if(ally->isDead() || ally->GetEntry() == _unit->GetEntry() || !IsSupportEntry(ally->GetEntry()))
				continue;

			ally->GetAIInterface()->AttackReaction(target, 1, 0);
		}
	}

	void SendAggroBark()
	{
		uint32 count = 0;
		const char* const* lines = GetAggroBarks(count);
		if(lines != NULL && count != 0)
			SendRandomCombatBark(lines, count);
	}

	uint32 m_primarySpell;
	uint32 m_secondarySpell;
	uint32 m_selfSpell;
	uint32 m_extraSpell;
	uint32 m_primaryTimer;
	uint32 m_secondaryTimer;
	uint32 m_selfTimer;
	uint32 m_extraTimer;
	uint32 m_supportTimer;
};

class AlteracValleyDefenderAI : public AlteracValleyCreatureAI
{
public:
	AlteracValleyDefenderAI(Creature* pCreature) : AlteracValleyCreatureAI(pCreature, 35.0f)
	{
		uint32 aura = GetAVDefenderAura(pCreature->GetEntry());
		if(aura != 0)
		{
			pCreature->CastSpell(pCreature, aura, true);
		
			// These AV defender auras can modify derived stats/max health after the battleground
			// code has already initialized the creature. Normalize current health afterward so
			// marshals and warmasters do not appear spawned at partial health (observed ~80%).
			const uint32 maxHealth = pCreature->GetUInt32Value(UNIT_FIELD_MAXHEALTH);
			if(maxHealth != 0)
				pCreature->SetUInt32Value(UNIT_FIELD_HEALTH, maxHealth);
		}

		m_cleaveTimer = 8000;
		m_whirlwindTimer = 15000;
	}

	void OnCombatStart(Unit* mTarget)
	{
		AlteracValleyCreatureAI::OnCombatStart(mTarget);

		m_cleaveTimer = 8000;
		m_whirlwindTimer = 15000;
	}

protected:
	void DoBattlefieldAI()
	{
		uint32 diff = GetUpdateFrequency();
		if(_unit->GetAIInterface()->GetNextTarget() == NULL)
			return;

		if(_unit->GetCurrentSpell() == NULL)
		{
			if(SpellReady(m_whirlwindTimer, diff) && CastSpellOnSelf(AV_SPELL_WHIRLWIND))
			{
				m_whirlwindTimer = 18000;
				return;
			}

			if(SpellReady(m_cleaveTimer, diff) && CastSpellOnTarget(AV_SPELL_CLEAVE))
			{
				m_cleaveTimer = 10000;
				return;
			}
		}
		else
		{
			SpellReady(m_cleaveTimer, diff);
			SpellReady(m_whirlwindTimer, diff);
		}
	}

	uint32 m_cleaveTimer;
	uint32 m_whirlwindTimer;
};

// Duros and Drakan
class AVBossWolfAI : public AlteracValleyCreatureAI
{
public:
	ADD_CREATURE_FACTORY_FUNCTION(AVBossWolfAI);

	AVBossWolfAI(Creature* pCreature) : AlteracValleyCreatureAI(pCreature, 35.0f)
	{
	}

protected:
};

class AVWingCommanderGossip : public GossipScript
{
public:
	void GossipHello(Object* pObject, Player* Plr, bool AutoSend)
	{
		if(pObject == NULL || pObject->GetTypeId() != TYPEID_UNIT || Plr == NULL)
			return;

		Creature* pCreature = static_cast<Creature*>(pObject);
		GossipMenu* Menu;
		uint32 textId = objmgr.GetGossipTextForNpc(pCreature->GetEntry());
		if(textId == 0)
			textId = 2;

		objmgr.CreateGossipMenuForPlayer(&Menu, pCreature->GetGUID(), textId, Plr);

		if(Plr->m_bg != NULL && Plr->m_bg->GetType() == BATTLEGROUND_ALTERAC_VALLEY &&
			!pCreature->isDead() && pCreature->GetStandState() == STANDSTATE_KNEEL)
		{
			const uint32 myTeam = (Plr->m_bgTeam > 1) ? 1 : Plr->m_bgTeam;
			if((myTeam == 0 && AVIsAllianceWingCommanderEntry(pCreature->GetEntry())) ||
			   (myTeam == 1 && AVIsHordeWingCommanderEntry(pCreature->GetEntry())))
			{
				Menu->AddItem(0, AVGetWingCommanderRescueText(pCreature->GetEntry()), GOSSIP_INTID_AV_RESCUE);
			}
		}

		if(Plr->m_bg != NULL && Plr->m_bg->GetType() == BATTLEGROUND_ALTERAC_VALLEY &&
			!pCreature->isDead() && pCreature->GetStandState() == STANDSTATE_STAND)
		{
			const uint32 myTeam = (Plr->m_bgTeam > 1) ? 1 : Plr->m_bgTeam;
			if((myTeam == 0 && AVIsAllianceWingCommanderEntry(pCreature->GetEntry())) ||
			   (myTeam == 1 && AVIsHordeWingCommanderEntry(pCreature->GetEntry())))
			{
				Menu->AddItem(0, AVGetWingCommanderDeployText(pCreature->GetEntry()), GOSSIP_INTID_AV_DEPLOY);
			}
		}

		if(AutoSend)
			Menu->SendTo(Plr);
	}

	void GossipSelectOption(Object* pObject, Player* Plr, uint32 Id, uint32 IntId, const char* EnteredCode)
	{
		(void)Id;
		(void)EnteredCode;

		if((IntId != GOSSIP_INTID_AV_RESCUE && IntId != GOSSIP_INTID_AV_DEPLOY) ||
			pObject == NULL || pObject->GetTypeId() != TYPEID_UNIT || Plr == NULL)
			return;

		Creature* pCreature = static_cast<Creature*>(pObject);
		if(Plr->m_bg != NULL && Plr->m_bg->GetType() == BATTLEGROUND_ALTERAC_VALLEY)
		{
			if(IntId == GOSSIP_INTID_AV_RESCUE)
				Plr->m_bg->HandleAirSupportRescue(Plr, pCreature);
			else
				Plr->m_bg->HandleAirSupportDeploy(Plr, pCreature);
		}

		Plr->Gossip_Complete();
	}

	void Destroy()
	{
		delete this;
	}
};

class AVWingCommanderAI : public CreatureAIScript
{
public:
	ADD_CREATURE_FACTORY_FUNCTION(AVWingCommanderAI);

	AVWingCommanderAI(Creature* pCreature) : CreatureAIScript(pCreature)
	{
		if(_unit->GetAIInterface() != NULL)
		{
			_unit->GetAIInterface()->SetAllowedToEnterCombat(false);
			_unit->GetAIInterface()->disable_melee = true;
			_unit->GetAIInterface()->disable_targeting = true;
		}
	}

	void OnCombatStart(Unit* mTarget)
	{
		(void)mTarget;
		if(_unit->GetAIInterface() != NULL)
		{
			_unit->GetAIInterface()->HandleEvent(EVENT_LEAVECOMBAT, _unit, 0);
			_unit->GetAIInterface()->SetAllowedToEnterCombat(false);
			_unit->GetAIInterface()->disable_melee = true;
			_unit->GetAIInterface()->disable_targeting = true;
		}
	}
};

class AVBowmanAI : public CreatureAIScript
{
public:
	ADD_CREATURE_FACTORY_FUNCTION(AVBowmanAI);

	AVBowmanAI(Creature* pCreature) : CreatureAIScript(pCreature)
	{
		_unit->GetAIInterface()->SetAllowedToEnterCombat(true);
		_unit->GetAIInterface()->m_canMove = false;
	}

	void OnLoad()
	{
		uint32 freq = _unit->GetUInt32Value(UNIT_FIELD_BASEATTACKTIME);
		if(freq == 0)
			freq = 2000;
		RegisterAIUpdateEvent(freq);
	}

	void OnCombatStart(Unit* mTarget)
	{
	}

	void OnCombatStop(Unit* mTarget)
	{
		RemoveAIUpdateEvent();
	}

	void OnDied(Unit* mKiller)
	{
		RemoveAIUpdateEvent();
	}

	void AIUpdate()
	{
		Unit* target = _unit->GetAIInterface()->GetNextTarget();
		if(target == NULL || _unit->GetCurrentSpell() != NULL)
			return;

		SpellEntry* sp = dbcSpell.LookupEntry(AV_SPELL_BOW_SHOOT);
		if(sp != NULL)
			_unit->CastSpell(target, sp, false);
	}
};

class VanndarStormpikeAI : public AlteracValleyBossAI
{
public:
	ADD_CREATURE_FACTORY_FUNCTION(VanndarStormpikeAI);
	VanndarStormpikeAI(Creature* pCreature) : AlteracValleyBossAI(pCreature, 65.0f) {}

protected:
	uint32 GetPrimarySpell() { return AV_SPELL_THUNDERCLAP; }
	uint32 GetSecondarySpell() { return AV_SPELL_STORM_BOLT; }
	uint32 GetEmergencySpell() { return AV_SPELL_AVATAR; }
	bool DefenderEntry(uint32 entry)
	{
		return entry == CN_DUN_BALDAR_NORTH_MARSHAL || entry == CN_DUN_BALDAR_SOUTH_MARSHAL || entry == CN_ICEWING_MARSHAL || entry == CN_STONEHEARTH_MARSHAL;
	}

	const char* const* GetAggroBarks(uint32& count)
	{
		static const char* kLines[] =
		{
			"Stormpike soldiers, to me!",
			"You will never leave Dun Baldar alive!",
			"Defend the Stormpike banner!"
		};
		count = 3;
		return kLines;
	}
};

class DrekTharAI : public AlteracValleyBossAI
{
public:
	ADD_CREATURE_FACTORY_FUNCTION(DrekTharAI);
	DrekTharAI(Creature* pCreature) : AlteracValleyBossAI(pCreature, 65.0f)
	{
		m_barkTimer = 18000;
	}

protected:
	uint32 GetPrimarySpell() { return AV_SPELL_KNOCKDOWN; }
	uint32 GetSecondarySpell() { return AV_SPELL_CHAIN_LIGHTNING; }
	uint32 GetEmergencySpell() { return AV_SPELL_FRENZY; }
	bool DefenderEntry(uint32 entry)
	{
		return entry == CN_EAST_FROSTWOLF_WARMASTER || entry == CN_ICEBLOOD_WARMASTER || entry == CN_TOWER_POINT_WARMASTER || entry == CN_WEST_FROSTWOLF_WARMASTER;
	}

	const char* const* GetAggroBarks(uint32& count)
	{
		static const char* kLines[] =
		{
			"Frostwolf warriors, attack!",
			"You face the might of the Frostwolf clan!",
			"Defend Frostwolf Keep!"
		};
		count = 3;
		return kLines;
	}

	void DoBattlefieldAI()
	{
		AlteracValleyBossAI::DoBattlefieldAI();

		uint32 diff = GetUpdateFrequency();
		if(_unit->GetAIInterface()->GetNextTarget() == NULL)
			return;

		if(SpellReady(m_barkTimer, diff))
		{
			static const char* kDrekBarks[] =
			{
				"Your attacks are slowed by the cold, I think!",
				"Today, you will meet your ancestors!",
				"If you will not leave Alterac Valley on your own, then the Frostwolves will force you out!",
				"You cannot defeat the Frostwolf clan!",
				"You are no match for the strength of the Horde!"
			};
			SendRandomCombatBark(kDrekBarks, 5);
			m_barkTimer = 20000 + RandomUInt(10000);
		}
	}

	uint32 m_barkTimer;
};

class BalindaStonehearthAI : public AlteracValleyCaptainAI
{
public:
	ADD_CREATURE_FACTORY_FUNCTION(BalindaStonehearthAI);
	BalindaStonehearthAI(Creature* pCreature) : AlteracValleyCaptainAI(
		pCreature, 45.0f, AV_SPELL_FROSTBOLT, AV_SPELL_FIREBALL, AV_SPELL_CONE_OF_COLD, AV_SPELL_SUMMON_WATER_ELEMENTAL) {}

protected:
	bool IsSupportEntry(uint32 entry)
	{
		return AVIsAllianceCaptainSupportEntry(entry);
	}

	const char* const* GetAggroBarks(uint32& count)
	{
		static const char* kLines[] =
		{
			"To me, Stormpike warriors!",
			"The Horde will break upon us!",
			"You dare attack Captain Balinda?"
		};
		count = 3;
		return kLines;
	}
};

class GalvangarAI : public AlteracValleyCaptainAI
{
public:
	ADD_CREATURE_FACTORY_FUNCTION(GalvangarAI);
	GalvangarAI(Creature* pCreature) : AlteracValleyCaptainAI(
		pCreature, 45.0f, AV_SPELL_CLEAVE, AV_SPELL_MORTAL_STRIKE, AV_SPELL_FEAR, AV_SPELL_WHIRLWIND) {}

protected:
	bool IsSupportEntry(uint32 entry)
	{
		return AVIsHordeCaptainSupportEntry(entry);
	}

	const char* const* GetAggroBarks(uint32& count)
	{
		static const char* kLines[] =
		{
			"Frostwolves, crush them!",
			"The Alliance weaklings have come to die!",
			"Feel the wrath of Galvangar!"
		};
		count = 3;
		return kLines;
	}
};

class DunBaldarNorthMarshalAI : public AlteracValleyDefenderAI
{
public:
	ADD_CREATURE_FACTORY_FUNCTION(DunBaldarNorthMarshalAI);
	DunBaldarNorthMarshalAI(Creature* pCreature) : AlteracValleyDefenderAI(pCreature) {}
};

class DunBaldarSouthMarshalAI : public AlteracValleyDefenderAI
{
public:
	ADD_CREATURE_FACTORY_FUNCTION(DunBaldarSouthMarshalAI);
	DunBaldarSouthMarshalAI(Creature* pCreature) : AlteracValleyDefenderAI(pCreature) {}
};

class IcewingMarshalAI : public AlteracValleyDefenderAI
{
public:
	ADD_CREATURE_FACTORY_FUNCTION(IcewingMarshalAI);
	IcewingMarshalAI(Creature* pCreature) : AlteracValleyDefenderAI(pCreature) {}
};

class StonehearthMarshalAI : public AlteracValleyDefenderAI
{
public:
	ADD_CREATURE_FACTORY_FUNCTION(StonehearthMarshalAI);
	StonehearthMarshalAI(Creature* pCreature) : AlteracValleyDefenderAI(pCreature) {}
};

class EastFrostwolfWarmasterAI : public AlteracValleyDefenderAI
{
public:
	ADD_CREATURE_FACTORY_FUNCTION(EastFrostwolfWarmasterAI);
	EastFrostwolfWarmasterAI(Creature* pCreature) : AlteracValleyDefenderAI(pCreature) {}
};

class IcebloodWarmasterAI : public AlteracValleyDefenderAI
{
public:
	ADD_CREATURE_FACTORY_FUNCTION(IcebloodWarmasterAI);
	IcebloodWarmasterAI(Creature* pCreature) : AlteracValleyDefenderAI(pCreature) {}
};

class TowerPointWarmasterAI : public AlteracValleyDefenderAI
{
public:
	ADD_CREATURE_FACTORY_FUNCTION(TowerPointWarmasterAI);
	TowerPointWarmasterAI(Creature* pCreature) : AlteracValleyDefenderAI(pCreature) {}
};

class WestFrostwolfWarmasterAI : public AlteracValleyDefenderAI
{
public:
	ADD_CREATURE_FACTORY_FUNCTION(WestFrostwolfWarmasterAI);
	WestFrostwolfWarmasterAI(Creature* pCreature) : AlteracValleyDefenderAI(pCreature) {}
};

void SetupAlteracValley(ScriptMgr* mgr)
{
	mgr->register_creature_script(CN_VANNDAR_STORMPIKE, &VanndarStormpikeAI::Create);
	mgr->register_creature_script(CN_DREKTHAR, &DrekTharAI::Create);
	mgr->register_creature_script(CN_BALINDA_STONEHEARTH, &BalindaStonehearthAI::Create);
	mgr->register_creature_script(CN_GALVANGAR, &GalvangarAI::Create);

	mgr->register_creature_script(CN_DRAKAN, &AVBossWolfAI::Create);
	mgr->register_creature_script(CN_DUROS, &AVBossWolfAI::Create);

	mgr->register_creature_script(CN_DUN_BALDAR_NORTH_MARSHAL, &DunBaldarNorthMarshalAI::Create);
	mgr->register_creature_script(CN_DUN_BALDAR_SOUTH_MARSHAL, &DunBaldarSouthMarshalAI::Create);
	mgr->register_creature_script(CN_ICEWING_MARSHAL, &IcewingMarshalAI::Create);
	mgr->register_creature_script(CN_STONEHEARTH_MARSHAL, &StonehearthMarshalAI::Create);
	mgr->register_creature_script(CN_EAST_FROSTWOLF_WARMASTER, &EastFrostwolfWarmasterAI::Create);
	mgr->register_creature_script(CN_ICEBLOOD_WARMASTER, &IcebloodWarmasterAI::Create);
	mgr->register_creature_script(CN_TOWER_POINT_WARMASTER, &TowerPointWarmasterAI::Create);
	mgr->register_creature_script(CN_WEST_FROSTWOLF_WARMASTER, &WestFrostwolfWarmasterAI::Create);

	mgr->register_creature_script(CN_STORMPIKE_BOWMAN, &AVBowmanAI::Create);
	mgr->register_creature_script(CN_FROSTWOLF_BOWMAN, &AVBowmanAI::Create);

	mgr->register_creature_script(CN_WC_SLIDORE, &AVWingCommanderAI::Create);
	mgr->register_creature_script(CN_WC_VIPORE, &AVWingCommanderAI::Create);
	mgr->register_creature_script(CN_WC_ICHMAN, &AVWingCommanderAI::Create);
	mgr->register_creature_script(CN_WC_GUSE, &AVWingCommanderAI::Create);
	mgr->register_creature_script(CN_WC_JEZTOR, &AVWingCommanderAI::Create);
	mgr->register_creature_script(CN_WC_MULVERICK, &AVWingCommanderAI::Create);

	GossipScript* wc = (GossipScript*)new AVWingCommanderGossip();
	mgr->register_gossip_script(CN_WC_SLIDORE, wc);
	mgr->register_gossip_script(CN_WC_VIPORE, wc);
	mgr->register_gossip_script(CN_WC_ICHMAN, wc);
	mgr->register_gossip_script(CN_WC_GUSE, wc);
	mgr->register_gossip_script(CN_WC_JEZTOR, wc);
	mgr->register_gossip_script(CN_WC_MULVERICK, wc);
 
	/* Quest-turn-in progression is handled by AlteracValley::HandleQuestTurnIn().
	 * Rescue / deploy interaction is routed through battleground hooks.
	 */
}
