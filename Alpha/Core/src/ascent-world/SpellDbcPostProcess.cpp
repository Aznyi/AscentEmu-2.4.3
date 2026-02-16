/*
 * AscentEmu / OpenAscent - Spell DBC post processing
 *
 * This file contains the generic, DBC-driven post-load pass for Spell.dbc rows.
 */

#include "StdAfx.h"
#include "SpellDbcPostProcess.h"
#include "SpellAuras.h"
#include <map>
#include <vector>


// -----------------------------------------------------------------------------
// Minimal DBC-backed registries (no need to change SpellEntry structs)
// -----------------------------------------------------------------------------
static std::map<uint32, uint32> sSpellCategoryFlags;				// categoryId -> flags
static std::map<uint32, bool>   sSpellChainEffectsIds;				// chainEffectsId -> present
static std::map<uint32, SpellSkillLineInfo> sSpellToSkillLine;		// spellId -> info
static std::map<uint32, std::vector<uint32> > sSkillLineToSpells;	// skillLineId -> spell ids

uint32 GetSpellCategoryFlags(uint32 categoryId)
{
	std::map<uint32, uint32>::const_iterator it = sSpellCategoryFlags.find(categoryId);
	return (it == sSpellCategoryFlags.end()) ? 0u : it->second;
}

bool HasSpellChainEffects(uint32 chainEffectsId)
{
	return sSpellChainEffectsIds.find(chainEffectsId) != sSpellChainEffectsIds.end();
}

static void LoadSpellCategoryDbc_IntoRegistry()
{
	// SpellCategory.dbc (TBC): typically [0]=ID (u32), [1]=Flags (u32)
	DBCFile cat;
	if(!cat.open("dbc/SpellCategory.dbc"))
	{
		Log.Error("World", "PostProcessSpellDBC(): could not open dbc/SpellCategory.dbc (skipping category flags registry).");
		return;
	}

	sSpellCategoryFlags.clear();
	const uint32 rows = (uint32)cat.getRecordCount();
	for(uint32 i = 0; i < rows; ++i)
	{
		DBCFile::Record r = cat.getRecord(i);
		const uint32 id = r.getUInt(0);
		const uint32 flags = r.getUInt(1);
		if(id != 0)
			sSpellCategoryFlags[id] = flags;
	}

	Log.Notice("World", "PostProcessSpellDBC(): loaded %u SpellCategory rows.", (uint32)sSpellCategoryFlags.size());
}

static void LoadSpellChainEffectsDbc_IntoRegistry()
{
	// SpellChainEffects.dbc: primarily a visual table; we register IDs for debug/validation.
	DBCFile chain;
	if(!chain.open("dbc/SpellChainEffects.dbc"))
	{
		Log.Error("World", "PostProcessSpellDBC(): could not open dbc/SpellChainEffects.dbc (skipping chain effects registry).");
		return;
	}

	sSpellChainEffectsIds.clear();
	const uint32 rows = (uint32)chain.getRecordCount();
	for(uint32 i = 0; i < rows; ++i)
	{
		DBCFile::Record r = chain.getRecord(i);
		const uint32 id = r.getUInt(0);
		if(id != 0)
			sSpellChainEffectsIds[id] = true;
	}

	Log.Notice("World", "PostProcessSpellDBC(): loaded %u SpellChainEffects rows.", (uint32)sSpellChainEffectsIds.size());
}

bool GetSpellSkillLineInfo(uint32 spellId, SpellSkillLineInfo& out)
{
	std::map<uint32, SpellSkillLineInfo>::const_iterator it = sSpellToSkillLine.find(spellId);
	if(it == sSpellToSkillLine.end())
		return false;
	out = it->second;
	return true;
}

const std::vector<uint32>* GetSpellsForSkillLine(uint32 skillLineId)
{
	std::map<uint32, std::vector<uint32> >::const_iterator it = sSkillLineToSpells.find(skillLineId);
	if(it == sSkillLineToSpells.end())
		return NULL;
	return &it->second;
}

static void BuildSkillLineAbilityRegistry()
{
	sSpellToSkillLine.clear();
	sSkillLineToSpells.clear();

	const uint32 rows = dbcSkillLineSpell.GetNumRows();
	for(uint32 i = 0; i < rows; ++i)
	{
		skilllinespell* s = dbcSkillLineSpell.LookupRow(i);
		if(s == NULL)
			continue;

		// Defensive: some rows can be 0 in buggy DBCs
		if(s->spell == 0 || s->skilline == 0)
			continue;

		SpellSkillLineInfo info;
		info.skillLine = s->skilline;
		info.minRank   = s->minrank;
		info.nextSpell = s->next;
		info.reqTP     = s->reqTP;

		// Some spells can appear multiple times. Prefer the "most relevant" row:
		// - lowest minRank (broadest)
		// - otherwise keep first
		std::map<uint32, SpellSkillLineInfo>::iterator it = sSpellToSkillLine.find(s->spell);
		if(it == sSpellToSkillLine.end() || info.minRank < it->second.minRank)
			sSpellToSkillLine[s->spell] = info;

		sSkillLineToSpells[s->skilline].push_back(s->spell);
	}

	Log.Notice("World", "PostProcessSpellDBC(): SkillLineAbility registry built: %u spell mappings, %u skill lines.",
		(uint32)sSpellToSkillLine.size(), (uint32)sSkillLineToSpells.size());
}

// -----------------------------------------------------------------------------
// Spell polarity helpers
//
// Ascent historically decides Aura positivity primarily from caster/target
// reaction (isAttackable). That breaks for spells that are intrinsically buffs
// but cast on enemies (common for NPC spells) and for the rare cases of hostile-
// style auras cast on friendlies.
//
// Keep this intentionally conservative: only force polarity when the DBC clearly
// indicates "buff-only" or "debuff-only" behavior.
// -----------------------------------------------------------------------------
static ASCENT_INLINE bool AuraNameIsClearlyNegative(uint32 aura, bool extended)
{
	// High-certainty debuff auras.
	switch(aura)
	{
		case SPELL_AURA_PERIODIC_DAMAGE:
		case SPELL_AURA_MOD_CONFUSE:
		case SPELL_AURA_MOD_CHARM:
		case SPELL_AURA_MOD_FEAR:
		case SPELL_AURA_MOD_TAUNT:
		case SPELL_AURA_MOD_STUN:
		case SPELL_AURA_MOD_PACIFY:
		case SPELL_AURA_MOD_ROOT:
		case SPELL_AURA_MOD_SILENCE:
		case SPELL_AURA_MOD_DECREASE_SPEED:
		case SPELL_AURA_MOD_PACIFY_SILENCE:
		case SPELL_AURA_MOD_DISARM:
		case SPELL_AURA_MOD_STALKED:
			return true;
		default:
			return false;
			break;
	}

	if(extended)
	{
		// Still high-certainty negatives, but gated to allow incremental rollout.
		switch(aura)
		{
			case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
			case SPELL_AURA_PERIODIC_LEECH:
			case SPELL_AURA_PERIODIC_MANA_LEECH:
				return true;
			default:
				break;
		}
	}

	return false;
}

static ASCENT_INLINE bool AuraNameIsClearlyPositive(uint32 aura, bool extended)
{
	// High-certainty buff auras.
	switch(aura)
	{
		case SPELL_AURA_PERIODIC_HEAL:
		case SPELL_AURA_MOD_STEALTH:
		case SPELL_AURA_MOD_INVISIBILITY:
		case SPELL_AURA_MOD_RESISTANCE:
		case SPELL_AURA_MOD_INCREASE_SPEED:
		case SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED:
		case SPELL_AURA_MOD_INCREASE_SWIM_SPEED:
		case SPELL_AURA_MOD_INCREASE_HEALTH:
		case SPELL_AURA_MOD_INCREASE_ENERGY:
		case SPELL_AURA_MOD_SHAPESHIFT:
		case SPELL_AURA_EFFECT_IMMUNITY:
		case SPELL_AURA_STATE_IMMUNITY:
		case SPELL_AURA_SCHOOL_IMMUNITY:
		case SPELL_AURA_DAMAGE_IMMUNITY:
		case SPELL_AURA_DISPEL_IMMUNITY:
			return true;
		default:
			return false;
			break;
	}

	if(extended)
	{
		// High-certainty positives, gated to allow incremental rollout.
		switch(aura)
		{
			case SPELL_AURA_PERIODIC_ENERGIZE:
			case SPELL_AURA_WATER_BREATHING:
			case SPELL_AURA_MOD_WATER_BREATHING:
			case SPELL_AURA_WATER_WALK:
			case SPELL_AURA_FEATHER_FALL:
				return true;
			default:
				break;
		}
	}

	return false;
}

static void DeriveForcedAuraPolarity(SpellEntry* sp, bool extended)
{
	if(sp == NULL)
		return;

	// Respect explicit overrides from hardcoded fixes.
	if(sp->c_is_flags & (SPELL_FLAG_IS_FORCEDBUFF | SPELL_FLAG_IS_FORCEDDEBUFF))
		return;

	bool hasPositive = false;
	bool hasNegative = false;

	for(uint32 i = 0; i < 3; ++i)
	{
		const uint32 eff = sp->Effect[i];
		if(eff == 0)
			continue;

		// Direct heals / energize are clearly positive.
		switch(eff)
		{
			case SPELL_EFFECT_HEAL:
			case SPELL_EFFECT_HEAL_MAX_HEALTH:
			case SPELL_EFFECT_HEAL_MECHANICAL:
			case SPELL_EFFECT_ENERGIZE:
				hasPositive = true;
				break;
			default:
				break;
		}

		// Direct damage / control effects are clearly negative.
		switch(eff)
		{
			case SPELL_EFFECT_SCHOOL_DAMAGE:
			case SPELL_EFFECT_ENVIRONMENTAL_DAMAGE:
			case SPELL_EFFECT_WEAPON_DAMAGE:
			case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
			case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
			case SPELL_EFFECT_POWER_BURN:
			case SPELL_EFFECT_HEALTH_LEECH:
			case SPELL_EFFECT_INTERRUPT_CAST:
			case SPELL_EFFECT_ATTACK:
				hasNegative = true;
				break;
			default:
				break;
		}

		// Aura polarity: decide using only unambiguous aura names.
		if (eff == SPELL_EFFECT_APPLY_AURA || eff == SPELL_EFFECT_APPLY_AREA_AURA)
		{
			const uint32 aura = sp->EffectApplyAuraName[i];
			if (AuraNameIsClearlyNegative(aura, extended))
				hasNegative = true;
			else if (AuraNameIsClearlyPositive(aura, extended))
				hasPositive = true;
		}
	}

	// Only force when it is one-sided.
	if(hasPositive && !hasNegative)
		sp->c_is_flags |= SPELL_FLAG_IS_FORCEDBUFF;
	else if(hasNegative && !hasPositive)
		sp->c_is_flags |= SPELL_FLAG_IS_FORCEDDEBUFF;
}

// -----------------------------------------------------------------------------
// Proc flag inference (fallback)
//
// This is a conservative, DBC-driven fallback for proc-trigger auras that have
// procFlags left unset (0). When procFlags are missing, the proc system becomes
// inert for those auras.
//
// It ONLY runs when procFlags == 0 and only targets common cases:
// - weapon-enchant style damage procs (default to melee/ranged hit triggers)
// - simple cast-trigger procs (default to spell cast triggers)
//
// Explicit procFlags from DBC, SpellFixes, or optional DB overrides always win.
// -----------------------------------------------------------------------------
static void DeriveProcFlagsIfUnset(SpellEntry* sp)
{
	if(sp == nullptr)
		return;

	if(sp->procFlags != 0)
		return;

	// 42 = SPELL_AURA_PROC_TRIGGER_SPELL
	// 43 = SPELL_AURA_PROC_TRIGGER_DAMAGE
	static const uint32 AURA_PROC_TRIGGER_SPELL  = 42u;
	static const uint32 AURA_PROC_TRIGGER_DAMAGE = 43u;

	int32 procIdx = -1;
	uint32 procAura = 0;
	uint32 triggerSpellId = 0;

	for(uint32 i = 0; i < 3; ++i)
	{
		if(sp->Effect[i] != SPELL_EFFECT_APPLY_AURA && sp->Effect[i] != SPELL_EFFECT_APPLY_AREA_AURA)
			continue;

		const uint32 aura = sp->EffectApplyAuraName[i];
		if(aura != AURA_PROC_TRIGGER_SPELL && aura != AURA_PROC_TRIGGER_DAMAGE)
			continue;

		const uint32 trig = sp->EffectTriggerSpell[i];
		if(trig == 0)
			continue;

		procIdx = (int32)i;
		procAura = aura;
		triggerSpellId = trig;
		break;
	}

	if(procIdx < 0)
		return;

	SpellEntry* triggered = dbcSpell.LookupEntryForced(triggerSpellId);
	const bool trigIsDamage = (triggered != nullptr) ? IsDamagingSpell(triggered) : false;
	const bool trigIsHeal   = (triggered != nullptr) ? IsHealingSpell(triggered) : false;

	if(procAura == AURA_PROC_TRIGGER_DAMAGE || trigIsDamage)
	{
#ifdef NEW_PROCFLAGS
		sp->procFlags = (PROC_ON_MELEE_HIT | PROC_ON_RANGED_HIT);
#else
		sp->procFlags = (PROC_ON_MELEE_ATTACK | PROC_ON_RANGED_ATTACK);
#endif
	}
	else if(trigIsHeal)
	{
		sp->procFlags = PROC_ON_CAST_SPELL;
	}
	else
	{
		sp->procFlags = PROC_ON_CAST_SPELL;
	}
}


// -----------------------------------------------------------------------------
// Effect column normalization (legacy)
//
// Some spells (notably Devastate) encode "bonus first then damage" by placing
// SPELL_EFFECT_WEAPON_PERCENT_DAMAGE in one slot and SPELL_EFFECT_DUMMYMELEE in
// another. Older Ascent spell logic expects the damage-style effect to be first.
//
// This is legacy normalization and should be removed once the spell system
// supports effect overwriting / correct scripted ordering.
// -----------------------------------------------------------------------------
static void NormalizeWeaponPercentDamageDummyMeleeOrder(SpellEntry* sp)
{
	if(sp == NULL)
		return;

	for(uint32 col1 = 0; col1 < 2; ++col1)
	{
		for(uint32 col2 = col1; col2 < 3; ++col2)
		{
			if(sp->Effect[col1] == SPELL_EFFECT_WEAPON_PERCENT_DAMAGE &&
			   sp->Effect[col2] == SPELL_EFFECT_DUMMYMELEE)
			{
				uint32 temp;
				float ftemp;

				temp = sp->Effect[col1];                         sp->Effect[col1] = sp->Effect[col2];                         sp->Effect[col2] = temp;
				temp = sp->EffectDieSides[col1];                 sp->EffectDieSides[col1] = sp->EffectDieSides[col2];         sp->EffectDieSides[col2] = temp;
				temp = sp->EffectBaseDice[col1];                 sp->EffectBaseDice[col1] = sp->EffectBaseDice[col2];         sp->EffectBaseDice[col2] = temp;
				ftemp = sp->EffectDicePerLevel[col1];            sp->EffectDicePerLevel[col1] = sp->EffectDicePerLevel[col2]; sp->EffectDicePerLevel[col2] = ftemp;
				ftemp = sp->EffectRealPointsPerLevel[col1];      sp->EffectRealPointsPerLevel[col1] = sp->EffectRealPointsPerLevel[col2]; sp->EffectRealPointsPerLevel[col2] = ftemp;
				temp = sp->EffectBasePoints[col1];               sp->EffectBasePoints[col1] = sp->EffectBasePoints[col2];     sp->EffectBasePoints[col2] = temp;
				temp = sp->EffectMechanic[col1];                 sp->EffectMechanic[col1] = sp->EffectMechanic[col2];         sp->EffectMechanic[col2] = temp;
				temp = sp->EffectImplicitTargetA[col1];          sp->EffectImplicitTargetA[col1] = sp->EffectImplicitTargetA[col2]; sp->EffectImplicitTargetA[col2] = temp;
				temp = sp->EffectImplicitTargetB[col1];          sp->EffectImplicitTargetB[col1] = sp->EffectImplicitTargetB[col2]; sp->EffectImplicitTargetB[col2] = temp;
				temp = sp->EffectRadiusIndex[col1];              sp->EffectRadiusIndex[col1] = sp->EffectRadiusIndex[col2];   sp->EffectRadiusIndex[col2] = temp;
				temp = sp->EffectApplyAuraName[col1];            sp->EffectApplyAuraName[col1] = sp->EffectApplyAuraName[col2]; sp->EffectApplyAuraName[col2] = temp;
				temp = sp->EffectAmplitude[col1];                sp->EffectAmplitude[col1] = sp->EffectAmplitude[col2];       sp->EffectAmplitude[col2] = temp;
				ftemp = sp->Effectunknown[col1];                 sp->Effectunknown[col1] = sp->Effectunknown[col2];           sp->Effectunknown[col2] = ftemp;
				temp = sp->EffectChainTarget[col1];              sp->EffectChainTarget[col1] = sp->EffectChainTarget[col2];   sp->EffectChainTarget[col2] = temp;
				temp = sp->EffectSpellGroupRelation[col1];       sp->EffectSpellGroupRelation[col1] = sp->EffectSpellGroupRelation[col2]; sp->EffectSpellGroupRelation[col2] = temp;
				temp = sp->EffectMiscValue[col1];                sp->EffectMiscValue[col1] = sp->EffectMiscValue[col2];       sp->EffectMiscValue[col2] = temp;
				temp = sp->EffectMiscValueB[col1];               sp->EffectMiscValueB[col1] = sp->EffectMiscValueB[col2];     sp->EffectMiscValueB[col2] = temp;
				temp = sp->EffectTriggerSpell[col1];             sp->EffectTriggerSpell[col1] = sp->EffectTriggerSpell[col2]; sp->EffectTriggerSpell[col2] = temp;
				ftemp = sp->EffectPointsPerComboPoint[col1];     sp->EffectPointsPerComboPoint[col1] = sp->EffectPointsPerComboPoint[col2]; sp->EffectPointsPerComboPoint[col2] = ftemp;

				return; // only need to normalize once
			}
		}
	}
}

void PostProcessSpellDBC()
{
	// Always visible once; lets you confirm it triggers even if World.cpp toggle is off.
	Log.Notice("World", "PostProcessSpellDBC(): begin (spells=%u)", dbcSpell.GetNumRows());

	// Optional debug
	const bool skillLineDebug = Config.MainConfig.GetBoolDefault("SpellDBC", "SkillLineDebug", false);
	const bool polarityDebug  = Config.MainConfig.GetBoolDefault("SpellDBC", "PolarityDebug", false);
	const bool effectSwapFix  = Config.MainConfig.GetBoolDefault("SpellDBC", "EffectSwapFix", true);
	const bool effectSwapDebug = Config.MainConfig.GetBoolDefault("SpellDBC", "EffectSwapDebug", false);
	const bool polarityExtended = Config.MainConfig.GetBoolDefault("SpellDBC", "PolarityListExtended", false);
	const bool procInferFix = Config.MainConfig.GetBoolDefault("SpellDBC", "ProcInferFix", true);
	const bool procInferDebug = Config.MainConfig.GetBoolDefault("SpellDBC", "ProcInferDebug", false);
	const bool deriveInFrontStatus = Config.MainConfig.GetBoolDefault("SpellDBC", "DeriveInFrontStatus", true);
	const bool facingDebug = Config.MainConfig.GetBoolDefault("SpellDBC", "FacingDebug", false);
  
 
	BuildSkillLineAbilityRegistry();
	if(skillLineDebug)
	{
		// Print a tiny sample of mappings (first 5)
		uint32 printed = 0;
		for(std::map<uint32, SpellSkillLineInfo>::const_iterator it = sSpellToSkillLine.begin();
		    it != sSpellToSkillLine.end() && printed < 5; ++it, ++printed)
		{
			Log.Notice("SpellDBC", "SkillLineMap: spell=%u skill=%u minRank=%u next=%u reqTP=%u",
				it->first, it->second.skillLine, it->second.minRank, it->second.nextSpell, it->second.reqTP);
		}
	}

	// Load high-ROI DBC registries (cooldown categories + chain visuals/validation)
	LoadSpellCategoryDbc_IntoRegistry();
	LoadSpellChainEffectsDbc_IntoRegistry();

	// Build a fast lookup of {spellId -> talentTree} from Talent.dbc
	std::map<uint32, uint32> talentSpells;
	for(uint32 i = 0; i < dbcTalent.GetNumRows(); ++i)
	{
		TalentEntry * tal = dbcTalent.LookupRow(i);
		if(tal == NULL)
			continue;

		for(uint32 j = 0; j < 5; ++j)
			if(tal->RankID[j] != 0)
				talentSpells.insert(std::make_pair(tal->RankID[j], tal->TalentTree));
	}

	uint32 cnt = dbcSpell.GetNumRows();
	for(uint32 x = 0; x < cnt; ++x)
	{
		SpellEntry * sp = dbcSpell.LookupRow(x);
		if(sp == NULL)
			continue;

		// Defaults for server-side helpers
		sp->self_cast_only = false;
		sp->apply_on_shapeshift_change = false;
		sp->always_apply = false;

		// Defaults for server-only spell system scaffolding.
		// These fields are not backed by Spell.dbc and must be initialized deterministically.
		sp->proc_interval = 0;          // trigger at each event
		sp->spell_coef_flags = 0;
		sp->Dspell_coef_override = -1;
		sp->OTspell_coef_override = -1;
		sp->casttime_coef = 0;
		sp->fixed_dddhcoef = -1;
		sp->fixed_hotdotcoef = -1;

		// Hash the name (used all over SpellFixes and runtime lookups)
		if(sp->Name != NULL && sp->Name[0] != 0)
			sp->NameHash = crc32((const unsigned char*)sp->Name, (unsigned int)strlen(sp->Name));
		else
			sp->NameHash = 0;

		const uint32 namehash = sp->NameHash;

		// Talent tree mapping (Talent.dbc -> SpellEntry::talent_tree)
		std::map<uint32, uint32>::iterator itr = talentSpells.find(sp->Id);
		sp->talent_tree = (itr == talentSpells.end() ? 0 : itr->second);

		// Parse rank text (Spell.dbc Rank field -> RankNumber)
		{
			uint32 rank = 0;
			if(sp->Rank != NULL && sp->Rank[0] != 0)
			{
				// client Rank field format is typically "Rank X"
				if(sscanf(sp->Rank, "Rank %u", (unsigned int*)&rank) != 1)
					rank = 0;
			}
			sp->RankNumber = rank;
		}

		// Precompute max(radius, range) squared for LOS / AI range heuristics
		float radius = 0.0f;
		radius = std::max(::GetRadius(dbcSpellRadius.LookupEntry(sp->EffectRadiusIndex[0])), radius);
		radius = std::max(::GetRadius(dbcSpellRadius.LookupEntry(sp->EffectRadiusIndex[1])), radius);
		radius = std::max(::GetRadius(dbcSpellRadius.LookupEntry(sp->EffectRadiusIndex[2])), radius);
		radius = std::max(GetMaxRange(dbcSpellRange.LookupEntry(sp->rangeIndex)), radius);
		sp->base_range_or_radius_sqr = radius * radius;

		// AI target type helper (inline in Spell.h)
		sp->ai_target_type = GetAiTargetType(sp);

		// In-front / behind facing requirements.
		// Historically, many spells were enumerated in SpellFixes::ApplyExtraDataFixes().
		// Prefer a DBC-derived default here and keep SpellFixes as exceptions-only.
		if(deriveInFrontStatus && sp->in_front_status == 0)
		{
			if(sp->Attributes & ATTRIBUTES_PASSIVE)
				sp->in_front_status = SPELL_INFRONT_STATUS_REQUIRE_SKIPCHECK;
			else
				sp->in_front_status = SPELL_INFRONT_STATUS_REQUIRE_INFRONT;

			// Behind-only exceptions (DBC attribute bits are not reliable for facing rules in all 2.4.3 datasets).
			// Keep this list small and surgical; expand only when validated in-game.
			switch(sp->NameHash)
			{
				case SPELL_HASH_BACKSTAB:
				case SPELL_HASH_AMBUSH:
				case SPELL_HASH_GARROTE:
					sp->in_front_status = SPELL_INFRONT_STATUS_REQUIRE_INBACK;
					break;
				default:
					break;
			}

			if(facingDebug)
				Log.Notice("SpellDBC", "Facing: spell=%u (%s) in_front_status=%u", sp->Id, (sp->Name != NULL ? sp->Name : ""), sp->in_front_status);
		}

		// Legacy effect ordering normalization (Devastate-style spells).
		// Keep config-gated so custom servers can opt out once they move to a
		// more accurate effect overwrite / scripted ordering implementation.
		if(effectSwapFix)
		{
			const uint32 pre0 = sp->Effect[0];
			const uint32 pre1 = sp->Effect[1];
			const uint32 pre2 = sp->Effect[2];
			NormalizeWeaponPercentDamageDummyMeleeOrder(sp);
			if(effectSwapDebug && (pre0 != sp->Effect[0] || pre1 != sp->Effect[1] || pre2 != sp->Effect[2]))
				Log.Notice("SpellDBC", "EffectSwap: spell=%u (%s) swapped WEAPON_PERCENT_DAMAGE <-> DUMMYMELEE ordering",
					sp->Id, (sp->Name != NULL ? sp->Name : ""));
		}

		// Conservative procFlags inference for proc-trigger auras.
		// Only applies when procFlags are unset (0) to avoid overriding explicit data.
		if(procInferFix)
		{
			const uint32 preProc = sp->procFlags;
			DeriveProcFlagsIfUnset(sp);
			if(procInferDebug && preProc == 0 && sp->procFlags != 0)
				Log.Notice("SpellDBC", "ProcInfer: spell=%u (%s) inferred procFlags=0x%08X", sp->Id, (sp->Name != NULL ? sp->Name : ""), sp->procFlags);
		}

		// Normalize School: Spell.dbc encodes school as a bitmask; many codepaths expect a single enum.
#define SET_SCHOOL(x) sp->School = x
		if(sp->School & 1)
			SET_SCHOOL(SCHOOL_NORMAL);
		else if(sp->School & 2)
			SET_SCHOOL(SCHOOL_HOLY);
		else if(sp->School & 4)
			SET_SCHOOL(SCHOOL_FIRE);
		else if(sp->School & 8)
			SET_SCHOOL(SCHOOL_NATURE);
		else if(sp->School & 16)
			SET_SCHOOL(SCHOOL_FROST);
		else if(sp->School & 32)
			SET_SCHOOL(SCHOOL_SHADOW);
		else if(sp->School & 64)
			SET_SCHOOL(SCHOOL_ARCANE);
		else
			Log.Error("World", "Spell %u has unknown school mask %u", sp->Id, sp->School);
#undef SET_SCHOOL

		// Classify buff/type groups (SpellEntry::buffType) + a few cheap derived flags.
		{
			uint32 type = 0;

			// these mostly do not mix so we can use else-if chain
			if(sp->Name != NULL)
			{
				if(strstr(sp->Name, "Seal"))
					type |= SPELL_TYPE_SEAL;
				else if(strstr(sp->Name, "Blessing"))
					type |= SPELL_TYPE_BLESSING;
				else if(strstr(sp->Name, "Curse"))
				{
					type |= SPELL_TYPE_CURSE;
					// some curse spells skip in-front checks (legacy behavior)
					sp->in_front_status = SPELL_INFRONT_STATUS_REQUIRE_SKIPCHECK;
				}
				else if(strstr(sp->Name, "Aspect"))
					type |= SPELL_TYPE_ASPECT;
				else if(strstr(sp->Name, "Sting") || strstr(sp->Name, "sting"))
					type |= SPELL_TYPE_STING;
				// don't break armor items!
				else if((strcmp(sp->Name, "Armor") != 0 && strstr(sp->Name, "Armor")) || strstr(sp->Name, "Demon Skin"))
					type |= SPELL_TYPE_ARMOR;
				else if(strstr(sp->Name, "Aura"))
					type |= SPELL_TYPE_AURA;
				else if(strstr(sp->Name, "Track") == sp->Name)
					type |= SPELL_TYPE_TRACK;
				else if(namehash == SPELL_HASH_GIFT_OF_THE_WILD || namehash == SPELL_HASH_MARK_OF_THE_WILD)
					type |= SPELL_TYPE_MARK_GIFT;
				else if(namehash == SPELL_HASH_IMMOLATION_TRAP || namehash == SPELL_HASH_FREEZING_TRAP || namehash == SPELL_HASH_FROST_TRAP ||
				        namehash == SPELL_HASH_EXPLOSIVE_TRAP || namehash == SPELL_HASH_SNAKE_TRAP)
					type |= SPELL_TYPE_HUNTER_TRAP;
				else if(namehash == SPELL_HASH_ARCANE_INTELLECT || namehash == SPELL_HASH_ARCANE_BRILLIANCE)
					type |= SPELL_TYPE_MAGE_INTEL;
				else if(namehash == SPELL_HASH_AMPLIFY_MAGIC || namehash == SPELL_HASH_DAMPEN_MAGIC)
					type |= SPELL_TYPE_MAGE_MAGI;
				else if(namehash == SPELL_HASH_FIRE_WARD || namehash == SPELL_HASH_FROST_WARD)
					type |= SPELL_TYPE_MAGE_WARDS;
				else if(namehash == SPELL_HASH_SHADOW_PROTECTION || namehash == SPELL_HASH_PRAYER_OF_SHADOW_PROTECTION)
					type |= SPELL_TYPE_PRIEST_SH_PPROT;
				else if(namehash == SPELL_HASH_WATER_SHIELD || namehash == SPELL_HASH_EARTH_SHIELD || namehash == SPELL_HASH_LIGHTNING_SHIELD)
					type |= SPELL_TYPE_SHIELD;
				else if(namehash == SPELL_HASH_POWER_WORD__FORTITUDE || namehash == SPELL_HASH_PRAYER_OF_FORTITUDE)
					type |= SPELL_TYPE_FORTITUDE;
				else if(namehash == SPELL_HASH_DIVINE_SPIRIT || namehash == SPELL_HASH_PRAYER_OF_SPIRIT)
					type |= SPELL_TYPE_SPIRIT;
				else if(strstr(sp->Name, "Immolate") || strstr(sp->Name, "Conflagrate"))
					type |= SPELL_TYPE_WARLOCK_IMMOLATE;
				else if(strstr(sp->Name, "Amplify Magic") || strstr(sp->Name, "Dampen Magic"))
					type |= SPELL_TYPE_MAGE_AMPL_DUMP;
			}

			if(sp->Description != NULL)
			{
				if(strstr(sp->Description, "Battle Elixir"))
					type |= SPELL_TYPE_ELIXIR_BATTLE;
				else if(strstr(sp->Description, "Guardian Elixir"))
					type |= SPELL_TYPE_ELIXIR_GUARDIAN;
				else if(strstr(sp->Description, "Battle and Guardian elixir"))
					type |= SPELL_TYPE_ELIXIR_FLASK;
				else if(strstr(sp->Description, "Finishing move") == sp->Description)
					sp->c_is_flags |= SPELL_FLAG_IS_FINISHING_MOVE;
			}

			if(namehash == SPELL_HASH_HUNTER_S_MARK)
				type |= SPELL_TYPE_HUNTER_MARK;
			else if(namehash == SPELL_HASH_COMMANDING_SHOUT || namehash == SPELL_HASH_BATTLE_SHOUT)
				type |= SPELL_TYPE_WARRIOR_SHOUT;

			sp->buffType = type;

			// Convenience classification flags used throughout the core
			if(IsDamagingSpell(sp))
				sp->c_is_flags |= SPELL_FLAG_IS_DAMAGING;
			if(IsHealingSpell(sp))
				sp->c_is_flags |= SPELL_FLAG_IS_HEALING;
			if(IsTargetingStealthed(sp))
				sp->c_is_flags |= SPELL_FLAG_IS_TARGETINGSTEALTHED;
		}

		// Derive forced buff/debuff polarity conservatively from DBC data.
		// Prevents reaction-based positivity from misclassifying intrinsic buffs
		// cast on enemies (common for NPC spells).
		const uint32 preFlags = sp->c_is_flags;
		DeriveForcedAuraPolarity(sp, polarityExtended);
		if(polarityDebug && (sp->c_is_flags != preFlags) && (sp->c_is_flags & (SPELL_FLAG_IS_FORCEDBUFF | SPELL_FLAG_IS_FORCEDDEBUFF)))
		{
			Log.Notice("SpellDBC", "Polarity: spell=%u (%s) forced=%s",
				sp->Id,
				(sp->Name != NULL ? sp->Name : ""),
				(sp->c_is_flags & SPELL_FLAG_IS_FORCEDBUFF) ? "BUFF" : "DEBUFF");
		}

		// “Apprentice/Journeyman/Expert/Artisan/Master” training spells often come in with spellLevel 0.
		// Fix both the trainer spell and the taught spell’s spellLevel.
		if(sp->spellLevel == 0 && sp->Name != NULL)
		{
			uint32 new_level = 0;
			if(strstr(sp->Name, "Apprentice "))
				new_level = 1;
			else if(strstr(sp->Name, "Journeyman "))
				new_level = 2;
			else if(strstr(sp->Name, "Expert "))
				new_level = 3;
			else if(strstr(sp->Name, "Artisan "))
				new_level = 4;
			else if(strstr(sp->Name, "Master "))
				new_level = 5;

			if(new_level != 0)
			{
				uint32 teachspell = 0;
				if(sp->Effect[0] == SPELL_EFFECT_LEARN_SPELL)
					teachspell = sp->EffectTriggerSpell[0];
				else if(sp->Effect[1] == SPELL_EFFECT_LEARN_SPELL)
					teachspell = sp->EffectTriggerSpell[1];
				else if(sp->Effect[2] == SPELL_EFFECT_LEARN_SPELL)
					teachspell = sp->EffectTriggerSpell[2];

				if(teachspell != 0)
				{
					SpellEntry * taught = dbcSpell.LookupEntryForced(teachspell);
					if(taught != NULL)
						taught->spellLevel = new_level;
					sp->spellLevel = new_level;
				}
			}
		}

		// Diminishing group is pure namehash-based classification.
		sp->DiminishStatus = GetDiminishingGroup(namehash);

		// Aura-interrupt helper + melee/ranged classification.
		// (Previously done in ApplyNormalFixes; it's generic derived state.)
		sp->is_melee_spell = false;
		sp->is_ranged_spell = false;
		if((sp->AuraInterruptFlags & AURA_INTERRUPT_ON_ANY_DAMAGE_TAKEN) != 0)
		{
			for(uint32 z = 0; z < 3; ++z)
			{
				if(sp->EffectApplyAuraName[z] == SPELL_AURA_MOD_FEAR ||
				   sp->EffectApplyAuraName[z] == SPELL_AURA_MOD_ROOT)
				{
					sp->AuraInterruptFlags |= AURA_INTERRUPT_ON_UNUSED2;
					break;
				}

				if((sp->Effect[z] == SPELL_EFFECT_SCHOOL_DAMAGE && sp->Spell_Dmg_Type == SPELL_DMG_TYPE_MELEE) ||
				   sp->Effect[z] == SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL ||
				   sp->Effect[z] == SPELL_EFFECT_WEAPON_DAMAGE ||
				   sp->Effect[z] == SPELL_EFFECT_WEAPON_PERCENT_DAMAGE ||
				   sp->Effect[z] == SPELL_EFFECT_DUMMYMELEE)
					sp->is_melee_spell = true;

				if(sp->Effect[z] == SPELL_EFFECT_SCHOOL_DAMAGE && sp->Spell_Dmg_Type == SPELL_DMG_TYPE_RANGED)
					sp->is_ranged_spell = true;
			}
		}

		// Aura-state handling:
		// In this core the aura-state fields are treated as bitmask flags. Many DBCs already store mask-like values
		// (e.g. 64/512/1024/4096/8192...). Only remap the small legacy ordinals (1..16-ish).
		// This prevents false "unknown aura state" spam.
		//
		// If you *want* strict logging for unexpected small values, enable:
		//   [SpellDBC] AuraStateStrict = 1
		const bool auraStateStrict = Config.MainConfig.GetBoolDefault("SpellDBC", "AuraStateStrict", false);

		auto RemapAuraStateLegacyOrdinal = [&](uint32 &state, bool isCaster)
		{
			if(state == 0)
				return;

			// If it looks like a mask already, leave it alone.
			// (Most mask values are >= 32, and/or powers of two.)
			if(state >= 32)
				return;

			// Legacy ordinal remap table.
			switch(state)
			{
			case 1:  state = AURASTATE_FLAG_DODGE_BLOCK; break;
			case 2:  state = AURASTATE_FLAG_HEALTH20; break;
			case 3:  state = AURASTATE_FLAG_BERSERK; break;
			case 5:  state = AURASTATE_FLAG_JUDGEMENT; break;
			case 7:  state = AURASTATE_FLAG_PARRY; break;
			case 10: state = AURASTATE_FLAG_LASTKILLWITHHONOR; break;
			case 11: state = AURASTATE_FLAG_CRITICAL; break;
			case 13: state = AURASTATE_FLAG_HEALTH35; break;
			case 14: state = AURASTATE_FLAG_IMMOLATE; break;
			case 15: state = AURASTATE_FLAG_REJUVENATE; break;
			case 16: state = AURASTATE_FLAG_POISON; break;
			default:
				if(auraStateStrict)
				{
					Log.Error("AuraState", "Spell %u (%s) has unknown %s aura state %u",
						sp->Id, (sp->Name ? sp->Name : ""), (isCaster ? "caster" : "target"), state);
				}
				// Otherwise: ignore silently; legacy ordinals beyond this table are not actionable.
				break;
			}
		};

		RemapAuraStateLegacyOrdinal(sp->CasterAuraState, true);
		RemapAuraStateLegacyOrdinal(sp->TargetAuraState, false);
	}
	Log.Notice("World", "PostProcessSpellDBC(): end");
}