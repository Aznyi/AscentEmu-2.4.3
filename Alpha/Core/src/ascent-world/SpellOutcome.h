#pragma once

#include "StdAfx.h"

// Central spell outcome event used to apply cooldown semantics at the correct time.
// This is the foundation for retiring SpellFixes.cpp and moving behavior to data-driven logic.
struct SpellOutcome
{
    SpellEntry* spell;                    // spell prototype (DBC)
    uint64 casterGuid;                    // caster GUID (for logging)
    uint64 primaryTargetGuid;             // original unit target (may be 0)
    uint32 categoryId;                    // spell->Category
    uint32 startRecoveryCategoryId;       // spell->StartRecoveryCategory
    uint32 categoryFlags;                 // SpellCategory.dbc flags (GetSpellCategoryFlags)

    // Resolution summary
    bool spellGoSent;                     // SpellGo actually sent (cast progressed beyond start)
    bool anySuccessfulTarget;             // at least one target landed for targeted spells
    bool wasCancelled;                    // spell cancelled/interrupted

    // Deferred cooldown payload (ms). Only used when category cooldown is started on event.
    int32 deferredCategoryCooldownMs;

    SpellOutcome()
        : spell(NULL), casterGuid(0), primaryTargetGuid(0), categoryId(0), startRecoveryCategoryId(0), categoryFlags(0),
          spellGoSent(false), anySuccessfulTarget(false), wasCancelled(false),
          deferredCategoryCooldownMs(0)
    {
    }
};

namespace SpellCooldownMgr
{
    // Apply cooldowns that are meant to start on impact/event rather than on cast start.
    // This currently handles SpellCategory flag 0x4 (cooldown starts on event) for CategoryRecoveryTime.
    void OnSpellOutcome(Player* caster, const SpellOutcome& out);
}
