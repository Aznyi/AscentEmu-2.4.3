#ifndef INSTANCE_SCRIPTS_SCRIPTED_CREATURE_AI_H
#define INSTANCE_SCRIPTS_SCRIPTED_CREATURE_AI_H

// Thin Ascent-native helper for future SD2-style creature script ports.
// This intentionally wraps CreatureAIScript instead of importing SD2 classes.
class ScriptedCreatureAI : public CreatureAIScript
{
public:
    explicit ScriptedCreatureAI(Creature* creature);
    virtual ~ScriptedCreatureAI() {}

    uint32 AddTimer(uint32 ms);
    void ResetTimer(uint32 timerId, uint32 ms);
    bool IsTimerFinished(uint32 timerId) const;

    void OnLoad();
    void OnCombatStart(Unit* target);
    void OnCombatStop(Unit* target);
    void OnDied(Unit* killer);
    void AIUpdate();

protected:
    virtual void Reset() {}
    virtual void EnterCombat(Unit* /*target*/) {}
    virtual void LeaveCombat() {}
    virtual void JustDied(Unit* /*killer*/) {}
    virtual void UpdateAI() {}

    bool DoCast(Unit* target, uint32 spellId);
    bool DoCastSelf(uint32 spellId);
    void DoYell(const char* text);
    void DoEmote(EmoteType emote);

    void SetAIUpdateInterval(uint32 ms);
    void StartAIUpdate();
    void StopAIUpdate();

private:
    struct ScriptTimer
    {
        uint32 remainingMs;
        bool active;

        ScriptTimer(uint32 ms) : remainingMs(ms), active(true) {}
    };

    void UpdateTimers(uint32 elapsedMs);

    vector<ScriptTimer> m_timers;
    uint32 m_updateIntervalMs;
    bool m_updateRegistered;
};

#endif
