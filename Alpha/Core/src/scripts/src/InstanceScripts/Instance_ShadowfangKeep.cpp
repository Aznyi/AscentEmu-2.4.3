// Instance_ShadowfangKeep.cpp
// Shadowfang Keep (Map 33) - framework registration proof-of-concept.
//
// This file exists to validate that InstanceContextRegistry can be wired up
// through the normal Ascent script registration path.  It is NOT a full boss
// script port.  Creature scripts here are intentionally minimal stubs that
// demonstrate:
//   - How to define a per-instance context subclass.
//   - How to register it with InstanceContextRegistry::RegisterFactory().
//   - How a creature script retrieves and writes to the shared context.
//   - The safe-despawn and door-open patterns for future full ports.
//
// When a full SFK script is written it should replace these stubs in place,
// keeping the SfkContext class and the RegisterFactory() call in
// SetupShadowfangKeep().

#include "StdAfx.h"
#include "Setup.h"
#include "InstanceContext.h"
#include "ScriptedCreatureAI.h"

namespace
{
    // -----------------------------------------------------------------------
    // Map / creature IDs
    // -----------------------------------------------------------------------
    enum SfkMapId : uint32
    {
        MAP_SHADOWFANG_KEEP = 33,
    };

    enum SfkCreatureEntry : uint32
    {
        CN_WOLF_MASTER_NANDOS = 3927,
        CN_FENRUS_THE_DEVOURER = 4274,
        CN_ARCHMAGE_ARUGAL     = 3977,
    };

    // -----------------------------------------------------------------------
    // Boss-type enum used as the key for SetData / GetData and GUID storage.
    // Add more entries here as each boss is scripted.
    // -----------------------------------------------------------------------
    enum SfkBoss : uint32
    {
        SFK_BOSS_NANDOS  = 0,
        SFK_BOSS_FENRUS  = 1,
        SFK_BOSS_ARUGAL  = 2,

        SFK_BOSS_COUNT   = 3,
    };

    // -----------------------------------------------------------------------
    // SfkContext
    // Per-instance state for Shadowfang Keep.  Extend as boss scripts are added.
    // -----------------------------------------------------------------------
    class SfkContext : public InstanceContext
    {
    public:
        explicit SfkContext(MapMgr* mgr)
            : InstanceContext(mgr)
        {
        }

        ADD_INSTANCE_CONTEXT_FACTORY(SfkContext)
    };

    // -----------------------------------------------------------------------
    // Helper: fetch the SfkContext for a given MapMgr (may return NULL on a
    // non-SFK map or before the first creature script runs).
    // -----------------------------------------------------------------------
    static SfkContext* GetSfkCtx(MapMgr* mgr)
    {
        return InstanceContextRegistry::GetContextAs<SfkContext>(mgr);
    }

    // -----------------------------------------------------------------------
    // Archmage Arugal - minimal stub demonstrating context write-back.
    // Replace with a full AI when scripting Arugal's abilities.
    // -----------------------------------------------------------------------
    class ArugalAI : public ScriptedCreatureAI
    {
    public:
        ArugalAI(Creature* c)
            : ScriptedCreatureAI(c),
              m_arcaneTimer(AddTimer(5000))
        {
        }

        ADD_CREATURE_FACTORY_FUNCTION(ArugalAI)

        void Reset()
        {
            // Register this creature's GUID so the context can resolve it later
            // (e.g. for a post-reset respawn check).
            SfkContext* ctx = GetSfkCtx(GetUnit()->GetMapMgr());
            if (ctx)
                ctx->StoreNpcGuid(SFK_BOSS_ARUGAL, GetUnit()->GetLowGUID());

            ResetTimer(m_arcaneTimer, 5000);
        }

        void EnterCombat(Unit* /*target*/)
        {
            SfkContext* ctx = GetSfkCtx(GetUnit()->GetMapMgr());
            if (ctx)
                ctx->SetData(SFK_BOSS_ARUGAL, ENCOUNTER_STATE_IN_PROGRESS);

            ResetTimer(m_arcaneTimer, 5000);
        }

        void LeaveCombat()
        {
            // In Ascent, OnCombatStop fires AFTER OnDied (via the
            // RemoveThreatByPtr -> EVENT_LEAVECOMBAT path at AIInterface.cpp:440).
            // The IsDone guard prevents resetting a just-completed encounter.
            SfkContext* ctx = GetSfkCtx(GetUnit()->GetMapMgr());
            if (!ctx)
                return;

            if (!ctx->IsDone(SFK_BOSS_ARUGAL))
            {
                ctx->SetData(SFK_BOSS_ARUGAL, ENCOUNTER_STATE_NOT_STARTED);
                return;
            }

            // Optional early cleanup: ~MapMgr auto-destroys via m_instanceScript
            // if this call is omitted.  Calling it here releases resources sooner
            // for the common case where players leave immediately after the kill.
            // OnCombatStop is safe here because no further script callbacks fire
            // for this creature, and GetMapMgr() is still valid (creature is
            // still in-world until SafeDelete/RemoveFromWorld).
            InstanceContextRegistry::DestroyContext(GetUnit()->GetMapMgr()->GetInstanceID());
        }

        void JustDied(Unit* /*killer*/)
        {
            SfkContext* ctx = GetSfkCtx(GetUnit()->GetMapMgr());
            if (!ctx)
                return;

            ctx->SetData(SFK_BOSS_ARUGAL, ENCOUNTER_STATE_DONE);

            // Door open example (uncomment when entry verified against DB):
            // ctx->OpenDoor(SFK_DOOR_EXIT);

            // DestroyContext is intentionally NOT called here.  OnCombatStop
            // fires after OnDied in Ascent; calling DestroyContext from OnDied
            // causes OnCombatStop to re-create an empty orphaned context via
            // the lazy-creation path.  See InstanceContext.h lifetime notes.
        }

        void UpdateAI()
        {
            if (!IsTimerFinished(m_arcaneTimer))
                return;

            // Timer-only proof of use.  Future full boss ports can replace this
            // with real spell/event logic while keeping the same wrapper style.
            ResetTimer(m_arcaneTimer, 5000);
        }

    private:
        uint32 m_arcaneTimer;
    };

    // -----------------------------------------------------------------------
    // Wolf Master Nandos - stub for encounter state tracking only.
    // -----------------------------------------------------------------------
    class NandosAI : public CreatureAIScript
    {
    public:
        NandosAI(Creature* c) : CreatureAIScript(c) {}

        ADD_CREATURE_FACTORY_FUNCTION(NandosAI)

        void OnLoad()
        {
            SfkContext* ctx = GetSfkCtx(GetUnit()->GetMapMgr());
            if (ctx)
                ctx->StoreNpcGuid(SFK_BOSS_NANDOS, GetUnit()->GetLowGUID());
        }

        void OnDied(Unit* /*killer*/)
        {
            SfkContext* ctx = GetSfkCtx(GetUnit()->GetMapMgr());
            if (ctx)
                ctx->SetData(SFK_BOSS_NANDOS, ENCOUNTER_STATE_DONE);
        }
    };

    // -----------------------------------------------------------------------
    // Fenrus the Devourer - stub for encounter state tracking only.
    // -----------------------------------------------------------------------
    class FenrusAI : public CreatureAIScript
    {
    public:
        FenrusAI(Creature* c) : CreatureAIScript(c) {}

        ADD_CREATURE_FACTORY_FUNCTION(FenrusAI)

        void OnLoad()
        {
            SfkContext* ctx = GetSfkCtx(GetUnit()->GetMapMgr());
            if (ctx)
                ctx->StoreNpcGuid(SFK_BOSS_FENRUS, GetUnit()->GetLowGUID());
        }

        void OnDied(Unit* /*killer*/)
        {
            SfkContext* ctx = GetSfkCtx(GetUnit()->GetMapMgr());
            if (ctx)
                ctx->SetData(SFK_BOSS_FENRUS, ENCOUNTER_STATE_DONE);
        }
    };

} // anonymous namespace

// ---------------------------------------------------------------------------
// SetupShadowfangKeep
// Called from Setup.cpp _exp_script_register.
// ---------------------------------------------------------------------------
void SetupShadowfangKeep(ScriptMgr* mgr)
{
    // Register the SfkContext factory for map 33.  The context is created
    // lazily the first time any creature script on this map calls GetContext().
    InstanceContextRegistry::RegisterFactory(MAP_SHADOWFANG_KEEP,
                                             SfkContext::CreateContext);

    mgr->register_creature_script(CN_ARCHMAGE_ARUGAL,     ArugalAI::Create);
    mgr->register_creature_script(CN_WOLF_MASTER_NANDOS,  NandosAI::Create);
    mgr->register_creature_script(CN_FENRUS_THE_DEVOURER, FenrusAI::Create);
}
