#include "StdAfx.h"
#include "SpellOutcome.h"
#include "SpellDbcPostProcess.h"

namespace
{
	// SpellCategory.dbc flags (safe subset used here)
	static constexpr uint32 SPELL_CATEGORY_FLAG_COOLDOWN_STARTS_ON_EVENT  = 0x4u;
}

void SpellCooldownMgr::OnSpellOutcome(Player* caster, const SpellOutcome& out)
{
	if(caster == nullptr || out.spell == nullptr)
		return;

	// Feature gate: this is a new system and should be enabled explicitly.
	const bool eventCooldowns = Config.MainConfig.GetBoolDefault("SpellDBC", "EventCooldowns", false);
	if(!eventCooldowns)
		return;

	const bool cdDebug = Config.MainConfig.GetBoolDefault("SpellDBC", "CooldownDebug", false);

	// Only apply deferred cooldowns if the cast actually progressed to SpellGo and was not cancelled.
	if(!out.spellGoSent || out.wasCancelled)
		return;

	// If we have a targeted spell that fully missed, and the DBC requests "starts on event",
	// do not start the category cooldown.
	if((out.categoryFlags & SPELL_CATEGORY_FLAG_COOLDOWN_STARTS_ON_EVENT) == 0)
		return;

	if(!out.anySuccessfulTarget && out.primaryTargetGuid != 0)
	{
		if(cdDebug)
			Log.Notice("Cooldown",
				"EventCooldowns: NOT starting category cooldown (no successful targets): spell=%u cat=%u",
				out.spell->Id, out.categoryId);
		return;
	}

	if(out.categoryId == 0 || out.deferredCategoryCooldownMs <= 0)
		return;

	// Unified cooldown application
	Player::CooldownContext ctx;
	ctx.spell = out.spell;
	ctx.itemCaster = nullptr;
	ctx.applySpellCooldown = false;
	ctx.applyCategoryCooldown = true;
	ctx.applyStartRecovery = false;
	ctx.categoryOverrideMs = out.deferredCategoryCooldownMs;
	caster->ApplyCooldownContext(ctx);

	if(cdDebug)
		Log.Notice("Cooldown", "EventCooldowns: started deferred category cooldown: spell=%u cat=%u dur=%dms",
			out.spell->Id, out.categoryId, out.deferredCategoryCooldownMs);
}