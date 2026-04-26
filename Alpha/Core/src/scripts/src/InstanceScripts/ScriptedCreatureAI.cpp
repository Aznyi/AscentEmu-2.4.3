#include "StdAfx.h"
#include "ScriptedCreatureAI.h"

namespace
{
    const uint32 SCRIPTED_CREATURE_DEFAULT_UPDATE_MS = 100;
}

ScriptedCreatureAI::ScriptedCreatureAI(Creature* creature)
    : CreatureAIScript(creature),
      m_updateIntervalMs(SCRIPTED_CREATURE_DEFAULT_UPDATE_MS),
      m_updateRegistered(false)
{
}

uint32 ScriptedCreatureAI::AddTimer(uint32 ms)
{
    m_timers.push_back(ScriptTimer(ms));
    return uint32(m_timers.size() - 1);
}

void ScriptedCreatureAI::ResetTimer(uint32 timerId, uint32 ms)
{
    if (timerId >= m_timers.size())
        return;

    m_timers[timerId].remainingMs = ms;
    m_timers[timerId].active = true;
}

bool ScriptedCreatureAI::IsTimerFinished(uint32 timerId) const
{
    if (timerId >= m_timers.size())
        return false;

    return m_timers[timerId].active && m_timers[timerId].remainingMs == 0;
}

void ScriptedCreatureAI::OnLoad()
{
    Reset();
}

void ScriptedCreatureAI::OnCombatStart(Unit* target)
{
    StartAIUpdate();
    EnterCombat(target);
}

void ScriptedCreatureAI::OnCombatStop(Unit* /*target*/)
{
    StopAIUpdate();
    LeaveCombat();
}

void ScriptedCreatureAI::OnDied(Unit* killer)
{
    StopAIUpdate();
    JustDied(killer);
}

void ScriptedCreatureAI::AIUpdate()
{
    UpdateTimers(m_updateIntervalMs);
    UpdateAI();
}

bool ScriptedCreatureAI::DoCast(Unit* target, uint32 spellId)
{
    if (GetUnit() == NULL || target == NULL || spellId == 0)
        return false;

    if (GetUnit()->isDead() || target->isDead())
        return false;

    if (GetUnit()->GetCurrentSpell() != NULL || GetUnit()->isCasting())
        return false;

    GetUnit()->CastSpell(target, spellId, false);
    return true;
}

bool ScriptedCreatureAI::DoCastSelf(uint32 spellId)
{
    return DoCast(GetUnit(), spellId);
}

void ScriptedCreatureAI::DoYell(const char* text)
{
    if (GetUnit() == NULL || text == NULL || text[0] == '\0')
        return;

    GetUnit()->SendChatMessage(CHAT_MSG_MONSTER_YELL, LANG_UNIVERSAL, text);
}

void ScriptedCreatureAI::DoEmote(EmoteType emote)
{
    if (GetUnit() == NULL)
        return;

    GetUnit()->Emote(emote);
}

void ScriptedCreatureAI::SetAIUpdateInterval(uint32 ms)
{
    if (ms == 0)
        return;

    m_updateIntervalMs = ms;

    if (m_updateRegistered)
        ModifyAIUpdateEvent(m_updateIntervalMs);
}

void ScriptedCreatureAI::StartAIUpdate()
{
    if (m_updateRegistered)
        return;

    RegisterAIUpdateEvent(m_updateIntervalMs);
    m_updateRegistered = true;
}

void ScriptedCreatureAI::StopAIUpdate()
{
    if (!m_updateRegistered)
        return;

    RemoveAIUpdateEvent();
    m_updateRegistered = false;
}

void ScriptedCreatureAI::UpdateTimers(uint32 elapsedMs)
{
    for (uint32 i = 0; i < m_timers.size(); ++i)
    {
        if (!m_timers[i].active || m_timers[i].remainingMs == 0)
            continue;

        if (m_timers[i].remainingMs <= elapsedMs)
            m_timers[i].remainingMs = 0;
        else
            m_timers[i].remainingMs -= elapsedMs;
    }
}
