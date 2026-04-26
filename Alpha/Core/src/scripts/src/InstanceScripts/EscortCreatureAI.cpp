#include "StdAfx.h"
#include "EscortCreatureAI.h"

namespace
{
    const uint32 ESCORT_RANGE_CHECK_MS = 1000;
    const float ESCORT_DEFAULT_OWNER_DISTANCE = 100.0f;
}

EscortCreatureAI::EscortCreatureAI(Creature* creature)
    : ScriptedCreatureAI(creature),
      m_ownerGuid(0),
      m_rangeCheckTimer(AddTimer(ESCORT_RANGE_CHECK_MS)),
      m_maxOwnerDistance(ESCORT_DEFAULT_OWNER_DISTANCE),
      m_escortActive(false),
      m_escortPaused(false),
      m_escortFailed(false)
{
}

void EscortCreatureAI::StartEscort(Player* owner)
{
    if (owner == NULL || GetUnit() == NULL || GetUnit()->isDead())
        return;

    m_ownerGuid = owner->GetLowGUID();
    m_escortActive = true;
    m_escortPaused = false;
    m_escortFailed = false;
    ResetTimer(m_rangeCheckTimer, ESCORT_RANGE_CHECK_MS);
    StartAIUpdate();

    EscortStarted(owner);
}

void EscortCreatureAI::StopEscort(bool completed)
{
    if (!m_escortActive && !m_escortPaused)
        return;

    ClearEscortState();
    EscortStopped(completed);
}

void EscortCreatureAI::PauseEscort()
{
    if (!m_escortActive || m_escortPaused)
        return;

    m_escortPaused = true;
    EscortPaused();
}

void EscortCreatureAI::ResumeEscort()
{
    if (!m_escortActive || !m_escortPaused)
        return;

    m_escortPaused = false;
    ResetTimer(m_rangeCheckTimer, ESCORT_RANGE_CHECK_MS);
    EscortResumed();
}

void EscortCreatureAI::FailEscort()
{
    if (!m_escortActive && !m_escortPaused)
        return;

    m_escortFailed = true;
    ClearEscortState();
    EscortFailed();
    EscortStopped(false);
}

bool EscortCreatureAI::IsEscortActive() const
{
    return m_escortActive && !m_escortFailed;
}

bool EscortCreatureAI::IsEscortPaused() const
{
    return m_escortPaused && !m_escortFailed;
}

uint32 EscortCreatureAI::GetEscortOwnerGuid() const
{
    return m_ownerGuid;
}

Player* EscortCreatureAI::GetEscortOwner()
{
    if (GetUnit() == NULL || GetUnit()->GetMapMgr() == NULL || m_ownerGuid == 0)
        return NULL;

    return GetUnit()->GetMapMgr()->GetPlayer(m_ownerGuid);
}

void EscortCreatureAI::SetMaxOwnerDistance(float distance)
{
    if (distance <= 0.0f)
        return;

    m_maxOwnerDistance = distance;
}

void EscortCreatureAI::OnReachWP(uint32 waypointId, bool forwards)
{
    if (!IsEscortActive() || IsEscortPaused())
        return;

    WaypointReached(waypointId, forwards);
}

void EscortCreatureAI::OnDied(Unit* killer)
{
    if (IsEscortActive() || IsEscortPaused())
        FailEscort();

    ScriptedCreatureAI::OnDied(killer);
}

bool EscortCreatureAI::GiveQuestCredit(uint32 questId, uint32 creditEntry)
{
    Player* owner = GetEscortOwner();
    if (owner == NULL || questId == 0 || creditEntry == 0)
        return false;

    QuestLogEntry* questLog = owner->GetQuestLogForEntry(questId);
    if (questLog == NULL || questLog->GetQuest() == NULL)
        return false;

    Quest* quest = questLog->GetQuest();
    for (uint32 i = 0; i < 4; ++i)
    {
        if (quest->required_mob[i] != creditEntry)
            continue;

        if (questLog->GetMobCount(i) >= quest->required_mobcount[i])
            return false;

        questLog->SetMobCount(i, questLog->GetMobCount(i) + 1);
        questLog->SendUpdateAddKill(i);
        questLog->UpdatePlayerFields();

        if (questLog->CanBeFinished())
            questLog->SendQuestComplete();

        return true;
    }

    return false;
}

void EscortCreatureAI::UpdateAI()
{
    if (!IsEscortActive())
    {
        UpdateEscortAI();
        return;
    }

    if (!IsEscortPaused() && IsTimerFinished(m_rangeCheckTimer))
    {
        Player* owner = GetEscortOwner();
        if (owner == NULL || owner->isDead() || !IsOwnerInRange(owner))
        {
            FailEscort();
            return;
        }

        ResetTimer(m_rangeCheckTimer, ESCORT_RANGE_CHECK_MS);
    }

    UpdateEscortAI();
}

bool EscortCreatureAI::IsOwnerInRange(Player* owner)
{
    if (GetUnit() == NULL || owner == NULL)
        return false;

    return GetUnit()->GetDistanceSq(owner) <= (m_maxOwnerDistance * m_maxOwnerDistance);
}

void EscortCreatureAI::ClearEscortState()
{
    m_ownerGuid = 0;
    m_escortActive = false;
    m_escortPaused = false;

    if (GetUnit() == NULL || GetUnit()->isDead())
        return;

    StopAIUpdate();
}
