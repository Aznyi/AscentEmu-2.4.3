#include "StdAfx.h"
#include "Setup.h"
#include "InstanceContext.h"
#include "ScriptedCreatureAI.h"

namespace
{
    enum RampartsMap : uint32
    {
        MAP_HELLFIRE_RAMPARTS = 543,
    };

    enum RampartsCreature : uint32
    {
        CN_WATCHKEEPER_GARGOLMAR = 17306,
        CN_VAZRUDEN_HERALD       = 17307,
        CN_OMOR_THE_UNSCARRED    = 17308,
        CN_HELLFIRE_SENTRY       = 17517,
        CN_NAZAN                 = 17536,
        CN_VAZRUDEN              = 17537,
    };

    enum RampartsGameObject : uint32
    {
        GO_FEL_IRON_CHEST        = 185168,
        GO_FEL_IRON_CHEST_H      = 185169,
    };

    enum RampartsEncounter : uint32
    {
        RAMPARTS_BOSS_GARGOLMAR  = 0,
        RAMPARTS_BOSS_OMOR       = 1,
        RAMPARTS_BOSS_VAZRUDEN   = 2,
        RAMPARTS_BOSS_NAZAN      = 3,
    };

    enum RampartsStorage : uint32
    {
        RAMPARTS_NPC_GARGOLMAR   = 0,
        RAMPARTS_NPC_OMOR        = 1,
        RAMPARTS_NPC_VAZRUDEN    = 2,
        RAMPARTS_NPC_NAZAN       = 3,
        RAMPARTS_NPC_HERALD      = 4,

        RAMPARTS_GO_CHEST        = 10,
        RAMPARTS_GO_CHEST_H      = 11,
    };

    enum RampartsCustomState : uint32
    {
        RAMPARTS_STATE_SPECIAL   = 4,  // SD2 SPECIAL: two Hellfire Sentries are dead.
    };

    enum GargolmarSpells : uint32
    {
        SPELL_MORTAL_WOUND       = 30641,
        SPELL_MORTAL_WOUND_H     = 36814,
        SPELL_SURGE              = 34645,
        SPELL_RETALIATION        = 22857,
        SPELL_OVERPOWER          = 32154,
    };

    enum OmorSpells : uint32
    {
        SPELL_ORBITAL_STRIKE        = 30637,
        SPELL_TREACHEROUS_AURA      = 30695,
        SPELL_BANE_OF_TREACHERY_H   = 37566,
        SPELL_DEMONIC_SHIELD        = 31901,
        SPELL_SHADOW_BOLT           = 30686,
        SPELL_SHADOW_BOLT_H         = 39297,
        SPELL_SUMMON_FIENDISH_HOUND = 30707,
    };

    enum VazrudenNazanSpells : uint32
    {
        SPELL_SUMMON_VAZRUDEN    = 30717,
        SPELL_REVENGE            = 19130,
        SPELL_REVENGE_H          = 40392,
        SPELL_FIREBALL           = 34653,
        SPELL_FIREBALL_H         = 36920,
        SPELL_CONE_OF_FIRE       = 30926,
        SPELL_CONE_OF_FIRE_H     = 36921,
        SPELL_BELLOW_ROAR_H      = 39427,
    };

    enum RampartsWaypointFlags : uint32
    {
        WP_FLAG_RUN              = 256,
        WP_FLAG_FLY              = 768,
    };

    struct RampartsWaypoint
    {
        float x;
        float y;
        float z;
        float o;
        uint32 waitTime;
        uint32 flags;
    };

    // Old Ascent's waypoint chain is used only for the verified Ascent movement API.
    // SD2 point movement is not available in this helper layer.
    static const RampartsWaypoint kNazanWaypoints[] =
    {
        { 0.0f,          0.0f,          0.0f,       0.0f,      0,    WP_FLAG_RUN },
        { -1413.410034f, 1744.969971f,  80.900000f, 0.147398f, 2000, WP_FLAG_RUN },
        { -1413.410034f, 1744.969971f,  92.948196f, 0.147398f, 0,    WP_FLAG_FLY },
        { -1378.454712f, 1687.340332f, 110.200218f, 1.017074f, 0,    WP_FLAG_FLY },
        { -1352.973145f, 1726.131470f, 110.408745f, 1.297234f, 0,    WP_FLAG_FLY },
        { -1362.943970f, 1767.925415f, 110.101616f, 5.212438f, 0,    WP_FLAG_FLY },
        { -1415.544189f, 1804.141357f, 110.075363f, 5.974271f, 0,    WP_FLAG_FLY },
        { -1461.189575f, 1780.554932f, 110.854507f, 0.460774f, 0,    WP_FLAG_FLY },
        { -1482.489380f, 1718.727783f, 110.248772f, 5.847037f, 0,    WP_FLAG_FLY },
        { -1418.811646f, 1676.112427f, 110.405968f, 0.231439f, 0,    WP_FLAG_FLY },
        { -1413.408203f, 1744.974121f,  92.000000f, 0.147398f, 1000, WP_FLAG_FLY },
    };

    uint32 Urand(uint32 minValue, uint32 maxValue)
    {
        if (maxValue <= minValue)
            return minValue;

        return minValue + uint32(rand() % (maxValue - minValue + 1));
    }

    bool IsHeroic(MapMgr* mgr)
    {
        return mgr != NULL && mgr->iInstanceMode != 0;
    }

    Unit* GetCurrentTarget(Creature* creature)
    {
        if (creature == NULL || creature->GetAIInterface() == NULL)
            return NULL;

        return creature->GetAIInterface()->GetNextTarget();
    }

    void SetGameObjectUnclickable(GameObject* go, bool unclickable)
    {
        if (go == NULL)
            return;

        uint32 flags = go->GetUInt32Value(GAMEOBJECT_FLAGS);
        if (unclickable)
            flags |= GAMEOBJECT_UNCLICKABLE;
        else
            flags &= ~GAMEOBJECT_UNCLICKABLE;

        go->SetUInt32Value(GAMEOBJECT_FLAGS, flags);
    }

    class RampartsContext : public InstanceContext
    {
    public:
        explicit RampartsContext(MapMgr* mgr)
            : InstanceContext(mgr),
              m_sentryDeaths(0)
        {
        }

        ADD_INSTANCE_CONTEXT_FACTORY(RampartsContext)

        void RegisterCreature(Creature* creature)
        {
            if (creature == NULL)
                return;

            switch (creature->GetEntry())
            {
            case CN_WATCHKEEPER_GARGOLMAR:
                StoreNpcGuid(RAMPARTS_NPC_GARGOLMAR, creature->GetLowGUID());
                break;
            case CN_OMOR_THE_UNSCARRED:
                StoreNpcGuid(RAMPARTS_NPC_OMOR, creature->GetLowGUID());
                break;
            case CN_VAZRUDEN_HERALD:
                StoreNpcGuid(RAMPARTS_NPC_HERALD, creature->GetLowGUID());
                StoreNpcGuid(RAMPARTS_NPC_NAZAN, creature->GetLowGUID());
                break;
            case CN_NAZAN:
                StoreNpcGuid(RAMPARTS_NPC_NAZAN, creature->GetLowGUID());
                break;
            case CN_VAZRUDEN:
                StoreNpcGuid(RAMPARTS_NPC_VAZRUDEN, creature->GetLowGUID());
                break;
            case CN_HELLFIRE_SENTRY:
                StoreSentryGuid(creature->GetLowGUID());
                break;
            }
        }

        void RegisterChest(GameObject* go)
        {
            if (go == NULL)
                return;

            if (go->GetEntry() == GO_FEL_IRON_CHEST)
                StoreGOGuid(RAMPARTS_GO_CHEST, go->GetLowGUID());
            else if (go->GetEntry() == GO_FEL_IRON_CHEST_H)
                StoreGOGuid(RAMPARTS_GO_CHEST_H, go->GetLowGUID());

            ApplyChestState(go);
        }

        void SetEncounterState(uint32 type, uint32 state)
        {
            if (GetData(type) == state)
                return;

            SetData(type, state);

            if (type == RAMPARTS_BOSS_VAZRUDEN || type == RAMPARTS_BOSS_NAZAN)
            {
                if (state == ENCOUNTER_STATE_FAILED)
                    FailVazrudenEvent();
                else
                    UpdateChestState();
            }
        }

        void IncrementSentryDeaths()
        {
            if (GetData(RAMPARTS_BOSS_NAZAN) == ENCOUNTER_STATE_DONE ||
                GetData(RAMPARTS_BOSS_NAZAN) == ENCOUNTER_STATE_IN_PROGRESS)
                return;

            ++m_sentryDeaths;
            if (m_sentryDeaths >= 2)
                SetData(RAMPARTS_BOSS_NAZAN, RAMPARTS_STATE_SPECIAL);
        }

        bool IsFinalChestReady() const
        {
            return GetData(RAMPARTS_BOSS_VAZRUDEN) == ENCOUNTER_STATE_DONE &&
                   GetData(RAMPARTS_BOSS_NAZAN) == ENCOUNTER_STATE_DONE;
        }

        void UpdateChestState()
        {
            ApplyChestState(LookupGO(RAMPARTS_GO_CHEST));
            ApplyChestState(LookupGO(RAMPARTS_GO_CHEST_H));
        }

        void ApplyChestState(GameObject* go)
        {
            if (go == NULL)
                return;

            SetGameObjectUnclickable(go, !IsFinalChestReady());
        }

        void FailVazrudenEvent()
        {
            SetData(RAMPARTS_BOSS_VAZRUDEN, ENCOUNTER_STATE_FAILED);
            SetData(RAMPARTS_BOSS_NAZAN, ENCOUNTER_STATE_FAILED);
            m_sentryDeaths = 0;
            UpdateChestState();
            RespawnSentries();

            Creature* vazruden = LookupNpc(RAMPARTS_NPC_VAZRUDEN);
            if (vazruden != NULL)
                InstanceContext::SafeDespawn(vazruden, 500, 0);
        }

    private:
        void StoreSentryGuid(uint32 lowGuid)
        {
            for (uint32 i = 0; i < m_sentryGuids.size(); ++i)
            {
                if (m_sentryGuids[i] == lowGuid)
                    return;
            }

            m_sentryGuids.push_back(lowGuid);
        }

        void RespawnSentries()
        {
            if (GetMap() == NULL)
                return;

            for (uint32 i = 0; i < m_sentryGuids.size(); ++i)
            {
                Creature* sentry = GetMap()->GetCreature(m_sentryGuids[i]);
                if (sentry != NULL)
                    sentry->Despawn(0, 1000);
            }
        }

        uint32 m_sentryDeaths;
        vector<uint32> m_sentryGuids;
    };

    RampartsContext* GetRampartsCtx(MapMgr* mgr)
    {
        return InstanceContextRegistry::GetContextAs<RampartsContext>(mgr);
    }

    class WatchkeeperGargolmarAI : public ScriptedCreatureAI
    {
    public:
        WatchkeeperGargolmarAI(Creature* c)
            : ScriptedCreatureAI(c),
              m_surgeTimer(AddTimer(3000)),
              m_mortalWoundTimer(AddTimer(8000)),
              m_retaliationTimer(AddTimer(1000)),
              m_overpowerTimer(AddTimer(9000)),
              m_hasTaunted(false),
              m_yelledForHeal(false)
        {
        }

        ADD_CREATURE_FACTORY_FUNCTION(WatchkeeperGargolmarAI)

        void Reset()
        {
            RampartsContext* ctx = GetRampartsCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->RegisterCreature(GetUnit());

            ResetTimer(m_surgeTimer, Urand(2400, 6100));
            ResetTimer(m_mortalWoundTimer, Urand(3500, 14400));
            ResetTimer(m_retaliationTimer, 1000);
            ResetTimer(m_overpowerTimer, Urand(3600, 14800));
            m_hasTaunted = false;
            m_yelledForHeal = false;
        }

        void EnterCombat(Unit* /*target*/)
        {
            RampartsContext* ctx = GetRampartsCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->SetEncounterState(RAMPARTS_BOSS_GARGOLMAR, ENCOUNTER_STATE_IN_PROGRESS);

            switch (Urand(0, 2))
            {
            case 0: DoYell("What do we have here?"); break;
            case 1: DoYell("Heh... this may hurt a little."); break;
            default: DoYell("I'm gonna enjoy this!"); break;
            }
        }

        void LeaveCombat()
        {
            RampartsContext* ctx = GetRampartsCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL && !ctx->IsDone(RAMPARTS_BOSS_GARGOLMAR))
                ctx->SetEncounterState(RAMPARTS_BOSS_GARGOLMAR, ENCOUNTER_STATE_NOT_STARTED);
        }

        void OnTargetDied(Unit* /*target*/)
        {
            DoYell(Urand(0, 1) ? "Say farewell!" : "Much too easy!");
        }

        void JustDied(Unit* /*killer*/)
        {
            DoYell("Hah...");

            RampartsContext* ctx = GetRampartsCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->SetEncounterState(RAMPARTS_BOSS_GARGOLMAR, ENCOUNTER_STATE_DONE);
        }

        void UpdateAI()
        {
            Unit* target = GetCurrentTarget(GetUnit());
            if (target == NULL || GetUnit()->isDead())
                return;

            if (!m_hasTaunted && GetUnit()->GetHealthPct() > 95)
            {
                DoYell("Back off, pup!");
                m_hasTaunted = true;
            }

            if (IsTimerFinished(m_mortalWoundTimer))
            {
                DoCast(target, IsHeroic(GetUnit()->GetMapMgr()) ? SPELL_MORTAL_WOUND_H : SPELL_MORTAL_WOUND);
                ResetTimer(m_mortalWoundTimer, Urand(6100, 12200));
            }

            if (IsTimerFinished(m_surgeTimer))
            {
                if (DoCast(target, SPELL_SURGE))
                    DoYell("Back off, pup!");
                ResetTimer(m_surgeTimer, Urand(12100, 21700));
            }

            if (GetUnit()->GetHealthPct() < 20 && IsTimerFinished(m_retaliationTimer))
            {
                DoCastSelf(SPELL_RETALIATION);
                ResetTimer(m_retaliationTimer, 30000);
            }

            if (IsTimerFinished(m_overpowerTimer))
            {
                DoCast(target, SPELL_OVERPOWER);
                ResetTimer(m_overpowerTimer, Urand(18100, 33700));
            }

            if (!m_yelledForHeal && GetUnit()->GetHealthPct() < 40)
            {
                DoYell("Heal me! QUICKLY!");
                m_yelledForHeal = true;
            }
        }

    private:
        uint32 m_surgeTimer;
        uint32 m_mortalWoundTimer;
        uint32 m_retaliationTimer;
        uint32 m_overpowerTimer;
        bool m_hasTaunted;
        bool m_yelledForHeal;
    };

    class OmorAI : public ScriptedCreatureAI
    {
    public:
        OmorAI(Creature* c)
            : ScriptedCreatureAI(c),
              m_orbitalStrikeTimer(AddTimer(25000)),
              m_auraTimer(AddTimer(15000)),
              m_demonicShieldTimer(AddTimer(1000)),
              m_shadowBoltTimer(AddTimer(8000)),
              m_summonTimer(AddTimer(20000))
        {
        }

        ADD_CREATURE_FACTORY_FUNCTION(OmorAI)

        void Reset()
        {
            RampartsContext* ctx = GetRampartsCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->RegisterCreature(GetUnit());

            ResetTimer(m_orbitalStrikeTimer, 25000);
            ResetTimer(m_auraTimer, Urand(12300, 23300));
            ResetTimer(m_demonicShieldTimer, 1000);
            ResetTimer(m_shadowBoltTimer, Urand(6600, 8900));
            ResetTimer(m_summonTimer, Urand(19600, 23100));
        }

        void EnterCombat(Unit* /*target*/)
        {
            RampartsContext* ctx = GetRampartsCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->SetEncounterState(RAMPARTS_BOSS_OMOR, ENCOUNTER_STATE_IN_PROGRESS);

            switch (Urand(0, 2))
            {
            case 0: DoYell("I will not be... defeated!"); break;
            case 1: DoYell("You dare stand against me?"); break;
            default: DoYell("Your insolence will be your death!"); break;
            }
        }

        void LeaveCombat()
        {
            RampartsContext* ctx = GetRampartsCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL && !ctx->IsDone(RAMPARTS_BOSS_OMOR))
                ctx->SetEncounterState(RAMPARTS_BOSS_OMOR, ENCOUNTER_STATE_NOT_STARTED);
        }

        void OnTargetDied(Unit* /*target*/)
        {
            if (Urand(0, 1))
                DoYell("Die weakling!");
            else
                DoYell("I am victorious!");
        }

        void JustDied(Unit* /*killer*/)
        {
            DoYell("It is... not... over...");

            RampartsContext* ctx = GetRampartsCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->SetEncounterState(RAMPARTS_BOSS_OMOR, ENCOUNTER_STATE_DONE);
        }

        void UpdateAI()
        {
            Unit* target = GetCurrentTarget(GetUnit());
            if (target == NULL || GetUnit()->isDead())
                return;

            if (IsTimerFinished(m_summonTimer))
            {
                if (DoCastSelf(SPELL_SUMMON_FIENDISH_HOUND))
                    DoYell("Achor-she-ki! Feast my pet! Eat your fill!");
                ResetTimer(m_summonTimer, Urand(24100, 26900));
            }

            if (IsTimerFinished(m_orbitalStrikeTimer))
            {
                DoCast(target, SPELL_ORBITAL_STRIKE);
                ResetTimer(m_orbitalStrikeTimer, Urand(14000, 16000));
            }

            if (GetUnit()->GetHealthPct() < 20 && IsTimerFinished(m_demonicShieldTimer))
            {
                DoCastSelf(SPELL_DEMONIC_SHIELD);
                ResetTimer(m_demonicShieldTimer, 15000);
            }

            if (IsTimerFinished(m_auraTimer))
            {
                if (DoCast(target, IsHeroic(GetUnit()->GetMapMgr()) ? SPELL_BANE_OF_TREACHERY_H : SPELL_TREACHEROUS_AURA))
                    DoYell("Your soul will bleed!");
                ResetTimer(m_auraTimer, Urand(8000, 16000));
            }

            if (IsTimerFinished(m_shadowBoltTimer))
            {
                DoCast(target, IsHeroic(GetUnit()->GetMapMgr()) ? SPELL_SHADOW_BOLT_H : SPELL_SHADOW_BOLT);
                ResetTimer(m_shadowBoltTimer, Urand(4200, 7300));
            }
        }

    private:
        uint32 m_orbitalStrikeTimer;
        uint32 m_auraTimer;
        uint32 m_demonicShieldTimer;
        uint32 m_shadowBoltTimer;
        uint32 m_summonTimer;
    };

    class HellfireSentryAI : public ScriptedCreatureAI
    {
    public:
        HellfireSentryAI(Creature* c)
            : ScriptedCreatureAI(c)
        {
        }

        ADD_CREATURE_FACTORY_FUNCTION(HellfireSentryAI)

        void Reset()
        {
            RampartsContext* ctx = GetRampartsCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->RegisterCreature(GetUnit());
        }

        void JustDied(Unit* /*killer*/)
        {
            RampartsContext* ctx = GetRampartsCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->IncrementSentryDeaths();
        }
    };

    class VazrudenAI : public ScriptedCreatureAI
    {
    public:
        VazrudenAI(Creature* c)
            : ScriptedCreatureAI(c),
              m_revengeTimer(AddTimer(6000)),
              m_requestedLanding(false)
        {
        }

        ADD_CREATURE_FACTORY_FUNCTION(VazrudenAI)

        void Reset()
        {
            RampartsContext* ctx = GetRampartsCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->RegisterCreature(GetUnit());

            ResetTimer(m_revengeTimer, Urand(5500, 8400));
            m_requestedLanding = false;
        }

        void EnterCombat(Unit* /*target*/)
        {
            RampartsContext* ctx = GetRampartsCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->SetEncounterState(RAMPARTS_BOSS_VAZRUDEN, ENCOUNTER_STATE_IN_PROGRESS);

            switch (Urand(0, 2))
            {
            case 0: DoYell("You will bleed!"); break;
            case 1: DoYell("Take your last breath!"); break;
            default: DoYell("The skies belong to us!"); break;
            }
        }

        void LeaveCombat()
        {
            RampartsContext* ctx = GetRampartsCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL && !ctx->IsDone(RAMPARTS_BOSS_VAZRUDEN))
                ctx->SetEncounterState(RAMPARTS_BOSS_VAZRUDEN, ENCOUNTER_STATE_FAILED);
        }

        void OnTargetDied(Unit* /*target*/)
        {
            DoYell(Urand(0, 1) ? "Say farewell!" : "Much too easy!");
        }

        void JustDied(Unit* /*killer*/)
        {
            DoYell("This... cannot be...");

            RampartsContext* ctx = GetRampartsCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
            {
                ctx->SetEncounterState(RAMPARTS_BOSS_VAZRUDEN, ENCOUNTER_STATE_DONE);

                // Ascent does not expose SD2's DamageTaken(uint32&) hook here. If a
                // large hit kills Vazruden before the 30% AIUpdate check fires, force
                // Nazan into the landing/combat state so the encounter cannot stall.
                if (!ctx->IsDone(RAMPARTS_BOSS_NAZAN))
                    ctx->SetEncounterState(RAMPARTS_BOSS_NAZAN, ENCOUNTER_STATE_IN_PROGRESS);
            }
        }

        void UpdateAI()
        {
            Unit* target = GetCurrentTarget(GetUnit());
            if (target == NULL || GetUnit()->isDead())
                return;

            if (!m_requestedLanding && GetUnit()->GetHealthPct() < 30)
            {
                RampartsContext* ctx = GetRampartsCtx(GetUnit()->GetMapMgr());
                if (ctx != NULL)
                    ctx->SetEncounterState(RAMPARTS_BOSS_NAZAN, ENCOUNTER_STATE_IN_PROGRESS);
                m_requestedLanding = true;
            }

            if (IsTimerFinished(m_revengeTimer))
            {
                DoCast(target, IsHeroic(GetUnit()->GetMapMgr()) ? SPELL_REVENGE_H : SPELL_REVENGE);
                ResetTimer(m_revengeTimer, Urand(11400, 14300));
            }
        }

    private:
        uint32 m_revengeTimer;
        bool m_requestedLanding;
    };

    class NazanHeraldAI : public ScriptedCreatureAI
    {
    public:
        NazanHeraldAI(Creature* c)
            : ScriptedCreatureAI(c),
              m_fireballTimer(AddTimer(3000)),
              m_coneTimer(AddTimer(10000)),
              m_roarTimer(AddTimer(8000)),
              m_waypointsAdded(false),
              m_splitDone(false),
              m_landed(false)
        {
            SetAIUpdateInterval(500);
        }

        ADD_CREATURE_FACTORY_FUNCTION(NazanHeraldAI)

        void Reset()
        {
            RampartsContext* ctx = GetRampartsCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->RegisterCreature(GetUnit());

            if (!m_waypointsAdded)
            {
                for (uint32 i = 1; i <= 10; ++i)
                    GetUnit()->GetAIInterface()->addWayPoint(CreateWaypoint(i));
                m_waypointsAdded = true;
            }

            GetUnit()->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            GetUnit()->GetAIInterface()->m_moveFly = true;
            GetUnit()->GetAIInterface()->SetAllowedToEnterCombat(false);
            GetUnit()->GetAIInterface()->SetAIState(STATE_SCRIPTMOVE);
            GetUnit()->GetAIInterface()->setMoveType(MOVEMENTTYPE_WANTEDWP);
            GetUnit()->GetAIInterface()->setWaypointToMove(1);

            ResetTimer(m_fireballTimer, 3000);
            ResetTimer(m_coneTimer, Urand(8100, 19700));
            ResetTimer(m_roarTimer, 8000);
            m_splitDone = false;
            m_landed = false;
            StartAIUpdate();
        }

        void LeaveCombat()
        {
            RampartsContext* ctx = GetRampartsCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL && !ctx->IsDone(RAMPARTS_BOSS_NAZAN))
                ctx->SetEncounterState(RAMPARTS_BOSS_NAZAN, ENCOUNTER_STATE_FAILED);
        }

        void JustDied(Unit* /*killer*/)
        {
            RampartsContext* ctx = GetRampartsCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->SetEncounterState(RAMPARTS_BOSS_NAZAN, ENCOUNTER_STATE_DONE);
        }

        void OnReachWP(uint32 waypointId, bool /*forwards*/)
        {
            RampartsContext* ctx = GetRampartsCtx(GetUnit()->GetMapMgr());
            if (ctx == NULL)
                return;

            if (!m_splitDone && ctx->GetData(RAMPARTS_BOSS_NAZAN) == RAMPARTS_STATE_SPECIAL)
            {
                DoYell("You have faced many challenges, pity they were all in vain.");
                GetUnit()->GetAIInterface()->setWaypointToMove(10);
                return;
            }

            if (waypointId == 10 && !m_splitDone)
            {
                m_splitDone = true;
                DoCastSelf(SPELL_SUMMON_VAZRUDEN);
                ctx->SetEncounterState(RAMPARTS_BOSS_VAZRUDEN, ENCOUNTER_STATE_IN_PROGRESS);
                GetUnit()->GetAIInterface()->setWaypointToMove(3);
                ResetTimer(m_fireballTimer, 1000);
                return;
            }

            if (!m_landed)
            {
                if (waypointId >= 3 && waypointId < 9)
                    GetUnit()->GetAIInterface()->setWaypointToMove(waypointId + 1);
                else if (waypointId == 9)
                    GetUnit()->GetAIInterface()->setWaypointToMove(3);
            }
        }

        void UpdateAI()
        {
            RampartsContext* ctx = GetRampartsCtx(GetUnit()->GetMapMgr());
            if (ctx == NULL || GetUnit()->isDead())
                return;

            if (ctx->GetData(RAMPARTS_BOSS_NAZAN) == ENCOUNTER_STATE_FAILED &&
                (m_splitDone || m_landed))
            {
                Reset();
                return;
            }

            if (!m_landed && ctx->GetData(RAMPARTS_BOSS_NAZAN) == ENCOUNTER_STATE_IN_PROGRESS)
                LandForCombat(ctx);

            if (!m_landed)
            {
                CastFlyingSupport(ctx);
                return;
            }

            Unit* target = GetCurrentTarget(GetUnit());
            if (target == NULL)
                return;

            if (IsTimerFinished(m_fireballTimer))
            {
                DoCast(target, IsHeroic(GetUnit()->GetMapMgr()) ? SPELL_FIREBALL_H : SPELL_FIREBALL);
                ResetTimer(m_fireballTimer, Urand(7300, 13200));
            }

            if (IsTimerFinished(m_coneTimer))
            {
                DoCast(target, IsHeroic(GetUnit()->GetMapMgr()) ? SPELL_CONE_OF_FIRE_H : SPELL_CONE_OF_FIRE);
                ResetTimer(m_coneTimer, Urand(7300, 13200));
            }

            if (IsHeroic(GetUnit()->GetMapMgr()) && IsTimerFinished(m_roarTimer))
            {
                DoCastSelf(SPELL_BELLOW_ROAR_H);
                ResetTimer(m_roarTimer, Urand(8000, 12000));
            }
        }

    private:
        WayPoint* CreateWaypoint(uint32 id)
        {
            WayPoint* wp = GetUnit()->CreateWaypointStruct();
            wp->id = id;
            wp->x = kNazanWaypoints[id].x;
            wp->y = kNazanWaypoints[id].y;
            wp->z = kNazanWaypoints[id].z;
            wp->o = kNazanWaypoints[id].o;
            wp->waittime = kNazanWaypoints[id].waitTime;
            wp->flags = kNazanWaypoints[id].flags;
            wp->forwardemoteoneshot = 0;
            wp->forwardemoteid = 0;
            wp->backwardemoteoneshot = 0;
            wp->backwardemoteid = 0;
            wp->forwardskinid = 0;
            wp->backwardskinid = 0;
            return wp;
        }

        void CastFlyingSupport(RampartsContext* ctx)
        {
            if (ctx->GetData(RAMPARTS_BOSS_VAZRUDEN) != ENCOUNTER_STATE_IN_PROGRESS ||
                !IsTimerFinished(m_fireballTimer))
                return;

            Creature* vazruden = ctx->LookupNpc(RAMPARTS_NPC_VAZRUDEN);
            Unit* target = GetCurrentTarget(vazruden);
            if (target != NULL)
                DoCast(target, IsHeroic(GetUnit()->GetMapMgr()) ? SPELL_FIREBALL_H : SPELL_FIREBALL);

            ResetTimer(m_fireballTimer, Urand(2100, 7300));
        }

        void LandForCombat(RampartsContext* ctx)
        {
            m_landed = true;
            GetUnit()->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            GetUnit()->GetAIInterface()->m_moveFly = false;
            GetUnit()->GetAIInterface()->SetAllowedToEnterCombat(true);
            GetUnit()->GetAIInterface()->setCurrentAgent(AGENT_NULL);
            GetUnit()->GetAIInterface()->SetAIState(STATE_IDLE);
            GetUnit()->GetAIInterface()->setMoveType(MOVEMENTTYPE_DONTMOVEWP);
            GetUnit()->GetAIInterface()->setWaypointToMove(0);
            DoYell("Nazan descends from above!");

            Creature* vazruden = ctx->LookupNpc(RAMPARTS_NPC_VAZRUDEN);
            Unit* target = GetCurrentTarget(vazruden);
            if (target != NULL)
                GetUnit()->GetAIInterface()->AttackReaction(target, 1, 0);

            ResetTimer(m_fireballTimer, Urand(5200, 16500));
        }

        uint32 m_fireballTimer;
        uint32 m_coneTimer;
        uint32 m_roarTimer;
        bool m_waypointsAdded;
        bool m_splitDone;
        bool m_landed;
    };

    class RampartsChestAI : public GameObjectAIScript
    {
    public:
        RampartsChestAI(GameObject* go)
            : GameObjectAIScript(go)
        {
        }

        static GameObjectAIScript* Create(GameObject* go) { return new RampartsChestAI(go); }

        void OnCreate()
        {
            Register();
        }

        void OnSpawn()
        {
            Register();
        }

    private:
        void Register()
        {
            RampartsContext* ctx = GetRampartsCtx(_gameobject->GetMapMgr());
            if (ctx != NULL)
                ctx->RegisterChest(_gameobject);
        }
    };

} // anonymous namespace

void SetupHellfireRamparts(ScriptMgr* mgr)
{
    InstanceContextRegistry::RegisterFactory(MAP_HELLFIRE_RAMPARTS,
                                             RampartsContext::CreateContext);

    mgr->register_creature_script(CN_WATCHKEEPER_GARGOLMAR, WatchkeeperGargolmarAI::Create);
    mgr->register_creature_script(CN_OMOR_THE_UNSCARRED,    OmorAI::Create);
    mgr->register_creature_script(CN_HELLFIRE_SENTRY,       HellfireSentryAI::Create);
    mgr->register_creature_script(CN_VAZRUDEN,              VazrudenAI::Create);
    mgr->register_creature_script(CN_VAZRUDEN_HERALD,       NazanHeraldAI::Create);
    mgr->register_creature_script(CN_NAZAN,                 NazanHeraldAI::Create);

    mgr->register_gameobject_script(GO_FEL_IRON_CHEST,      RampartsChestAI::Create);
    mgr->register_gameobject_script(GO_FEL_IRON_CHEST_H,    RampartsChestAI::Create);
}
