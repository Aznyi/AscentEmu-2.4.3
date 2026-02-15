/*
 * AscentEmu / OpenAscent - Spell DBC post processing
 *
 * This pass is meant to compute server-side derived fields purely from DBC data
 * (hashes, ranges/radii, school normalization, aura-state remapping, etc.)
 * so SpellFixes can be reserved for true exceptions / behavior overrides.
 */

#pragma once

void PostProcessSpellDBC();

// SkillLineAbility registry (SkillLineAbility.dbc already loaded as dbcSkillLineSpell)
// Provides fast spell<->skill relationships to avoid scanning dbcSkillLineSpell at runtime.
struct SpellSkillLineInfo
{
	uint32 skillLine;   // skillline id
	uint32 minRank;     // required rank
	uint32 nextSpell;   // next rank spell id
	uint32 reqTP;       // trainer points requirement (if used by core)
};

// Returns true if the spell is present in SkillLineAbility and fills out info.
bool GetSpellSkillLineInfo(uint32 spellId, SpellSkillLineInfo& out);

// Returns a pointer to the vector of spell IDs for a skill line (or NULL if none).
const std::vector<uint32>* GetSpellsForSkillLine(uint32 skillLineId);

// Expose SpellCategory.dbc (category flags) for use by cooldown/group logic.
// SpellCategory.dbc is tiny but extremely useful for removing hardcoded cooldown-category exceptions.
uint32 GetSpellCategoryFlags(uint32 categoryId);

// Expose SpellChainEffects.dbc presence for debugging / validation.
// Note: In TBC, this table is mostly visual, but loading it lets you validate DBCs
// and remove any lingering visual hacks or client mismatch assumptions.
bool HasSpellChainEffects(uint32 chainEffectsId);
