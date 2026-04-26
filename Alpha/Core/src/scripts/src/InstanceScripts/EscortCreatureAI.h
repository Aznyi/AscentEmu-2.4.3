#ifndef INSTANCE_SCRIPTS_ESCORT_CREATURE_AI_H
#define INSTANCE_SCRIPTS_ESCORT_CREATURE_AI_H

#include "ScriptedCreatureAI.h"

class Player;
class QuestLogEntry;

// Thin Ascent-native escort/follower helper for future SD2 npc_escortAI-style ports.
// Mapping notes:
// - SD2 STATE_ESCORT_ESCORTING -> IsEscortActive()
// - SD2 STATE_ESCORT_PAUSED    -> IsEscortPaused(), PauseEscort(), ResumeEscort()
// - SD2 WaypointReached(...)   -> CreatureAIScript::OnReachWP -> WaypointReached(...)
// - SD2 PlayerGUID             -> owner low GUID resolved through this creature's MapMgr
//
// This class does not own path movement. It wraps Ascent's existing waypoint hook and
// keeps escort state/range failure logic in one place for script-side ports.
class EscortCreatureAI : public ScriptedCreatureAI
{
public:
    explicit EscortCreatureAI(Creature* creature);
    virtual ~EscortCreatureAI() {}

    void StartEscort(Player* owner);
    void StopEscort(bool completed);
    void PauseEscort();
    void ResumeEscort();
    void FailEscort();

    bool IsEscortActive() const;
    bool IsEscortPaused() const;
    uint32 GetEscortOwnerGuid() const;
    Player* GetEscortOwner();

    void SetMaxOwnerDistance(float distance);

    void OnReachWP(uint32 waypointId, bool forwards);
    void OnDied(Unit* killer);

protected:
    virtual void EscortStarted(Player* /*owner*/) {}
    virtual void EscortStopped(bool /*completed*/) {}
    virtual void EscortPaused() {}
    virtual void EscortResumed() {}
    virtual void EscortFailed() {}
    virtual void WaypointReached(uint32 /*waypointId*/, bool /*forwards*/) {}
    virtual void UpdateEscortAI() {}

    // Gives normal quest kill/interaction credit for a required_mob entry.
    // Escort quests with nonstandard completion rules should still script that logic explicitly.
    bool GiveQuestCredit(uint32 questId, uint32 creditEntry);

    void UpdateAI();

private:
    bool IsOwnerInRange(Player* owner);
    void ClearEscortState();

    uint32 m_ownerGuid;
    uint32 m_rangeCheckTimer;
    float m_maxOwnerDistance;
    bool m_escortActive;
    bool m_escortPaused;
    bool m_escortFailed;
};

#endif
