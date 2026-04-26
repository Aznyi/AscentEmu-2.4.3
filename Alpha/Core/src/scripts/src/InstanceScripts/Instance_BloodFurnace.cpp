#include "StdAfx.h"
#include "Setup.h"
#include "InstanceContext.h"
#include "ScriptedCreatureAI.h"

namespace
{
    enum BloodFurnaceMap : uint32
    {
        MAP_BLOOD_FURNACE = 542,
    };

    enum BloodFurnaceCreature : uint32
    {
        CN_KELIDAN_THE_BREAKER  = 17377,
        CN_BROGGOK              = 17380,
        CN_THE_MAKER            = 17381,
        CN_NASCENT_FEL_ORC      = 17398,
        CN_SHADOWMOON_CHANNELER = 17653,
        CN_BROGGOK_POISON_CLOUD = 17662,
    };

    enum BloodFurnaceGameObject : uint32
    {
        GO_DOOR_FINAL_EXIT       = 181766,
        GO_DOOR_MAKER_FRONT      = 181811,
        GO_DOOR_MAKER_REAR       = 181812,
        GO_PRISON_CELL_BROGGOK_1 = 181817,
        GO_PRISON_CELL_BROGGOK_2 = 181818,
        GO_DOOR_BROGGOK_REAR     = 181819,
        GO_PRISON_CELL_BROGGOK_3 = 181820,
        GO_PRISON_CELL_BROGGOK_4 = 181821,
        GO_DOOR_BROGGOK_FRONT    = 181822,
        GO_DOOR_KELIDAN_EXIT     = 181823,
        GO_PRISON_CELL_LEVER     = 181982,
    };

    enum BloodFurnaceEncounter : uint32
    {
        BF_EVENT_THE_MAKER = 0,
        BF_EVENT_BROGGOK   = 1,
        BF_EVENT_KELIDAN   = 2,
    };

    enum BloodFurnaceStorage : uint32
    {
        BF_NPC_KELIDAN      = 0,
        BF_NPC_BROGGOK      = 1,
        BF_NPC_THE_MAKER    = 2,

        BF_GO_FINAL_EXIT    = 10,
        BF_GO_MAKER_FRONT   = 11,
        BF_GO_MAKER_REAR    = 12,
        BF_GO_BROGGOK_REAR  = 13,
        BF_GO_BROGGOK_FRONT = 14,
        BF_GO_KELIDAN_EXIT  = 15,
        BF_GO_CELL_1        = 16,
        BF_GO_CELL_2        = 17,
        BF_GO_CELL_3        = 18,
        BF_GO_CELL_4        = 19,
        BF_GO_CELL_LEVER    = 20,
    };

    enum BloodFurnaceConst : uint32
    {
        BF_MAX_ORC_WAVES    = 4,
        BF_MAX_CHANNELERS   = 5,
        GO_FLAG_OPEN        = 33,
        GO_FLAG_CLOSED      = 34,
        GO_STATE_OPEN       = 0,
        GO_STATE_CLOSED     = 1,
    };

    enum MakerSpells : uint32
    {
        SPELL_ACID_SPRAY          = 38153,
        SPELL_EXPLODING_BREAKER   = 30925,
        SPELL_EXPLODING_BREAKER_H = 40059,
        SPELL_KNOCKDOWN           = 20276,
        SPELL_DOMINATION          = 30923,
    };

    enum BroggokSpells : uint32
    {
        SPELL_SLIME_SPRAY     = 30913,
        SPELL_SLIME_SPRAY_H   = 38458,
        SPELL_POISON_CLOUD    = 30916,
        SPELL_POISON_BOLT     = 30917,
        SPELL_POISON_BOLT_H   = 38459,
        SPELL_POISON          = 30914,
    };

    enum KelidanSpells : uint32
    {
        SPELL_CORRUPTION           = 30938,
        SPELL_EVOCATION            = 30935,
        SPELL_FIRE_NOVA            = 33132,
        SPELL_FIRE_NOVA_H          = 37371,
        SPELL_SHADOW_BOLT_VOLLEY   = 28599,
        SPELL_SHADOW_BOLT_VOLLEY_H = 40070,
        SPELL_BURNING_NOVA         = 30940,
        SPELL_VORTEX               = 37370,
        SPELL_CHANNELING           = 39123,
        SPELL_CHANNELER_BOLT       = 12739,
        SPELL_CHANNELER_BOLT_H     = 15472,
        SPELL_MARK_OF_SHADOW       = 30937,
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

    void SetDoorState(GameObject* go, bool open)
    {
        if (go == NULL)
            return;

        go->SetUInt32Value(GAMEOBJECT_FLAGS, open ? GO_FLAG_OPEN : GO_FLAG_CLOSED);
        go->SetUInt32Value(GAMEOBJECT_STATE, open ? GO_STATE_OPEN : GO_STATE_CLOSED);
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

    class BloodFurnaceContext : public InstanceContext
    {
    public:
        explicit BloodFurnaceContext(MapMgr* mgr)
            : InstanceContext(mgr),
              m_broggokPhase(0),
              m_broggokStarted(false),
              m_channelerDeaths(0),
              m_kelidanReleased(false)
        {
            for (uint32 i = 0; i < BF_MAX_ORC_WAVES; ++i)
            {
                m_cellOpened[i] = false;
                m_cellKills[i] = 0;
            }
        }

        ADD_INSTANCE_CONTEXT_FACTORY(BloodFurnaceContext)

        void RegisterCreature(Creature* creature)
        {
            if (creature == NULL)
                return;

            switch (creature->GetEntry())
            {
            case CN_THE_MAKER:
                StoreNpcGuid(BF_NPC_THE_MAKER, creature->GetLowGUID());
                break;
            case CN_BROGGOK:
                StoreNpcGuid(BF_NPC_BROGGOK, creature->GetLowGUID());
                PrepareBroggok(creature);
                break;
            case CN_KELIDAN_THE_BREAKER:
                StoreNpcGuid(BF_NPC_KELIDAN, creature->GetLowGUID());
                PrepareKelidan(creature);
                PrepareAllChannelers();
                break;
            case CN_NASCENT_FEL_ORC:
                StoreUniqueGuid(m_orcGuids, creature->GetLowGUID());
                PrepareOrc(creature);
                break;
            case CN_SHADOWMOON_CHANNELER:
                StoreUniqueGuid(m_channelerGuids, creature->GetLowGUID());
                PrepareChanneler(creature);
                break;
            }
        }

        void RegisterGameObject(GameObject* go)
        {
            if (go == NULL)
                return;

            switch (go->GetEntry())
            {
            case GO_DOOR_FINAL_EXIT:
                StoreGOGuid(BF_GO_FINAL_EXIT, go->GetLowGUID());
                SetDoorState(go, IsDone(BF_EVENT_KELIDAN));
                break;
            case GO_DOOR_MAKER_FRONT:
                StoreGOGuid(BF_GO_MAKER_FRONT, go->GetLowGUID());
                SetDoorState(go, GetData(BF_EVENT_THE_MAKER) != ENCOUNTER_STATE_IN_PROGRESS);
                break;
            case GO_DOOR_MAKER_REAR:
                StoreGOGuid(BF_GO_MAKER_REAR, go->GetLowGUID());
                SetDoorState(go, IsDone(BF_EVENT_THE_MAKER));
                break;
            case GO_DOOR_BROGGOK_REAR:
                StoreGOGuid(BF_GO_BROGGOK_REAR, go->GetLowGUID());
                SetDoorState(go, IsDone(BF_EVENT_BROGGOK));
                break;
            case GO_DOOR_BROGGOK_FRONT:
                StoreGOGuid(BF_GO_BROGGOK_FRONT, go->GetLowGUID());
                SetDoorState(go, GetData(BF_EVENT_BROGGOK) != ENCOUNTER_STATE_IN_PROGRESS);
                break;
            case GO_DOOR_KELIDAN_EXIT:
                StoreGOGuid(BF_GO_KELIDAN_EXIT, go->GetLowGUID());
                SetDoorState(go, IsDone(BF_EVENT_KELIDAN));
                break;
            case GO_PRISON_CELL_BROGGOK_1:
                StoreGOGuid(BF_GO_CELL_1, go->GetLowGUID());
                SetDoorState(go, m_cellOpened[0]);
                break;
            case GO_PRISON_CELL_BROGGOK_2:
                StoreGOGuid(BF_GO_CELL_2, go->GetLowGUID());
                SetDoorState(go, m_cellOpened[1]);
                break;
            case GO_PRISON_CELL_BROGGOK_3:
                StoreGOGuid(BF_GO_CELL_3, go->GetLowGUID());
                SetDoorState(go, m_cellOpened[2]);
                break;
            case GO_PRISON_CELL_BROGGOK_4:
                StoreGOGuid(BF_GO_CELL_4, go->GetLowGUID());
                SetDoorState(go, m_cellOpened[3]);
                break;
            case GO_PRISON_CELL_LEVER:
                StoreGOGuid(BF_GO_CELL_LEVER, go->GetLowGUID());
                SetGameObjectUnclickable(go, m_broggokStarted || IsDone(BF_EVENT_BROGGOK));
                break;
            }
        }

        void SetEncounterState(uint32 type, uint32 state)
        {
            if (GetData(type) == state)
                return;

            SetData(type, state);

            if (type == BF_EVENT_THE_MAKER)
            {
                SetDoorState(LookupGO(BF_GO_MAKER_FRONT), state != ENCOUNTER_STATE_IN_PROGRESS);
                if (state == ENCOUNTER_STATE_DONE)
                    SetDoorState(LookupGO(BF_GO_MAKER_REAR), true);
            }
            else if (type == BF_EVENT_BROGGOK)
            {
                if (state == ENCOUNTER_STATE_IN_PROGRESS)
                    StartBroggokEvent(NULL);
                else if (state == ENCOUNTER_STATE_FAILED)
                    ResetBroggokEvent();
                else if (state == ENCOUNTER_STATE_DONE)
                {
                    SetDoorState(LookupGO(BF_GO_BROGGOK_FRONT), true);
                    SetDoorState(LookupGO(BF_GO_BROGGOK_REAR), true);
                    SetGameObjectUnclickable(LookupGO(BF_GO_CELL_LEVER), true);
                }
            }
            else if (type == BF_EVENT_KELIDAN)
            {
                if (state == ENCOUNTER_STATE_FAILED)
                    ResetKelidanEvent();
                else if (state == ENCOUNTER_STATE_DONE)
                {
                    SetDoorState(LookupGO(BF_GO_KELIDAN_EXIT), true);
                    SetDoorState(LookupGO(BF_GO_FINAL_EXIT), true);
                }
            }
        }

        void StartBroggokEvent(Unit* starter)
        {
            if (m_broggokStarted || IsDone(BF_EVENT_BROGGOK))
                return;

            SetData(BF_EVENT_BROGGOK, ENCOUNTER_STATE_IN_PROGRESS);
            m_broggokStarted = true;
            m_broggokPhase = 0;
            SetDoorState(LookupGO(BF_GO_BROGGOK_FRONT), false);
            SetGameObjectUnclickable(LookupGO(BF_GO_CELL_LEVER), true);
            SortBroggokOrcs();
            OpenNextBroggokWave(starter);
        }

        void NotifyBroggokOrcDeath(Creature* orc, Unit* killer)
        {
            if (orc == NULL || GetData(BF_EVENT_BROGGOK) != ENCOUNTER_STATE_IN_PROGRESS)
                return;

            for (uint32 i = 0; i < BF_MAX_ORC_WAVES; ++i)
            {
                if (!m_cellOpened[i] || !ContainsGuid(m_waveOrcs[i], orc->GetLowGUID()))
                    continue;

                ++m_cellKills[i];
                if (m_cellKills[i] >= m_waveOrcs[i].size())
                    OpenNextBroggokWave(killer);
                return;
            }
        }

        void FailBroggokFromOrc(Creature* orc)
        {
            if (orc == NULL || GetData(BF_EVENT_BROGGOK) != ENCOUNTER_STATE_IN_PROGRESS)
                return;

            for (uint32 i = 0; i < BF_MAX_ORC_WAVES; ++i)
            {
                if (m_cellOpened[i] && ContainsGuid(m_waveOrcs[i], orc->GetLowGUID()))
                {
                    SetEncounterState(BF_EVENT_BROGGOK, ENCOUNTER_STATE_FAILED);
                    return;
                }
            }
        }

        void NotifyChannelerDeath(Unit* killer)
        {
            if (IsDone(BF_EVENT_KELIDAN) || m_kelidanReleased)
                return;

            ++m_channelerDeaths;
            if (m_channelerDeaths < BF_MAX_CHANNELERS && HasLivingChannelers())
                return;

            Creature* kelidan = LookupNpc(BF_NPC_KELIDAN);
            if (kelidan != NULL)
            {
                kelidan->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                kelidan->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_9);
                if (killer != NULL)
                    kelidan->GetAIInterface()->AttackReaction(killer, 1, 0);
            }

            m_kelidanReleased = true;
            SetData(BF_EVENT_KELIDAN, ENCOUNTER_STATE_IN_PROGRESS);
        }

        void WakeChannelers(Unit* target)
        {
            if (target == NULL)
                return;

            for (uint32 i = 0; i < m_channelerGuids.size(); ++i)
            {
                Creature* channeler = GetMap() ? GetMap()->GetCreature(m_channelerGuids[i]) : NULL;
                if (channeler != NULL && channeler->isAlive())
                {
                    channeler->InterruptSpell();
                    channeler->GetAIInterface()->AttackReaction(target, 1, 0);
                }
            }
        }

    private:
        void StoreUniqueGuid(vector<uint32>& guids, uint32 lowGuid)
        {
            if (!ContainsGuid(guids, lowGuid))
                guids.push_back(lowGuid);
        }

        bool ContainsGuid(const vector<uint32>& guids, uint32 lowGuid) const
        {
            for (uint32 i = 0; i < guids.size(); ++i)
            {
                if (guids[i] == lowGuid)
                    return true;
            }
            return false;
        }

        uint32 GetCellStorage(uint32 index) const
        {
            switch (index)
            {
            case 0: return BF_GO_CELL_1;
            case 1: return BF_GO_CELL_2;
            case 2: return BF_GO_CELL_3;
            default: return BF_GO_CELL_4;
            }
        }

        void PrepareBroggok(Creature* broggok)
        {
            if (broggok == NULL || IsDone(BF_EVENT_BROGGOK))
                return;

            broggok->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            broggok->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_9);
        }

        void ReleaseBroggok(Unit* target)
        {
            Creature* broggok = LookupNpc(BF_NPC_BROGGOK);
            if (broggok == NULL)
                return;

            broggok->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            broggok->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_9);
            SetDoorState(LookupGO(BF_GO_BROGGOK_REAR), true);

            if (target != NULL)
                broggok->GetAIInterface()->AttackReaction(target, 1, 0);
        }

        void PrepareOrc(Creature* orc)
        {
            if (orc == NULL || IsDone(BF_EVENT_BROGGOK))
                return;

            orc->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            orc->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_9);
        }

        void ReleaseOrc(Creature* orc, Unit* target)
        {
            if (orc == NULL || !orc->isAlive())
                return;

            orc->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            orc->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_9);
            if (target != NULL)
                orc->GetAIInterface()->AttackReaction(target, 1, 0);
        }

        void PrepareKelidan(Creature* kelidan)
        {
            if (kelidan == NULL || IsDone(BF_EVENT_KELIDAN))
                return;

            kelidan->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            kelidan->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_9);
            kelidan->CastSpell(kelidan, SPELL_EVOCATION, false);
        }

        void PrepareChanneler(Creature* channeler)
        {
            if (channeler == NULL || IsDone(BF_EVENT_KELIDAN))
                return;

            Creature* kelidan = LookupNpc(BF_NPC_KELIDAN);
            if (kelidan != NULL)
                channeler->CastSpell(kelidan, SPELL_CHANNELING, false);
        }

        void PrepareAllChannelers()
        {
            for (uint32 i = 0; i < m_channelerGuids.size(); ++i)
            {
                Creature* channeler = GetMap() ? GetMap()->GetCreature(m_channelerGuids[i]) : NULL;
                if (channeler != NULL && channeler->isAlive())
                    PrepareChanneler(channeler);
            }
        }

        bool HasLivingChannelers() const
        {
            if (GetMap() == NULL)
                return false;

            for (uint32 i = 0; i < m_channelerGuids.size(); ++i)
            {
                Creature* channeler = GetMap()->GetCreature(m_channelerGuids[i]);
                if (channeler != NULL && channeler->isAlive())
                    return true;
            }

            return false;
        }

        void SortBroggokOrcs()
        {
            for (uint32 i = 0; i < BF_MAX_ORC_WAVES; ++i)
            {
                m_waveOrcs[i].clear();
                m_cellKills[i] = 0;
            }

            for (uint32 i = 0; i < m_orcGuids.size(); ++i)
            {
                Creature* orc = GetMap() ? GetMap()->GetCreature(m_orcGuids[i]) : NULL;
                if (orc == NULL)
                    continue;

                uint32 bestCell = BF_MAX_ORC_WAVES;
                float bestDistance = 225.0f;

                for (uint32 cell = 0; cell < BF_MAX_ORC_WAVES; ++cell)
                {
                    GameObject* door = LookupGO(GetCellStorage(cell));
                    if (door == NULL)
                        continue;

                    float distance = orc->GetDistanceSq(door);
                    if (distance < bestDistance)
                    {
                        bestDistance = distance;
                        bestCell = cell;
                    }
                }

                if (bestCell < BF_MAX_ORC_WAVES)
                    m_waveOrcs[bestCell].push_back(orc->GetLowGUID());
            }
        }

        void OpenNextBroggokWave(Unit* target)
        {
            if (m_broggokPhase >= BF_MAX_ORC_WAVES)
            {
                ReleaseBroggok(target);
                return;
            }

            const uint32 wave = m_broggokPhase;
            m_cellOpened[wave] = true;
            m_cellKills[wave] = 0;
            SetDoorState(LookupGO(GetCellStorage(wave)), true);

            for (uint32 i = 0; i < m_waveOrcs[wave].size(); ++i)
            {
                Creature* orc = GetMap() ? GetMap()->GetCreature(m_waveOrcs[wave][i]) : NULL;
                ReleaseOrc(orc, target);
            }

            ++m_broggokPhase;

            // Missing or mispositioned DB spawns should not wedge the event.
            // SD2 sorts by nearby cell doors as well; this keeps the Ascent port
            // restartable even when a wave cannot be populated.
            if (m_waveOrcs[wave].empty())
                OpenNextBroggokWave(target);
        }

        void ResetBroggokEvent()
        {
            m_broggokStarted = false;
            m_broggokPhase = 0;
            SetDoorState(LookupGO(BF_GO_BROGGOK_FRONT), true);
            SetDoorState(LookupGO(BF_GO_BROGGOK_REAR), false);
            SetGameObjectUnclickable(LookupGO(BF_GO_CELL_LEVER), false);

            for (uint32 wave = 0; wave < BF_MAX_ORC_WAVES; ++wave)
            {
                SetDoorState(LookupGO(GetCellStorage(wave)), false);
                m_cellOpened[wave] = false;
                m_cellKills[wave] = 0;

                for (uint32 i = 0; i < m_waveOrcs[wave].size(); ++i)
                {
                    Creature* orc = GetMap() ? GetMap()->GetCreature(m_waveOrcs[wave][i]) : NULL;
                    if (orc != NULL)
                    {
                        if (orc->isAlive())
                            PrepareOrc(orc);
                        else
                            orc->Despawn(0, 1000);
                    }
                }
            }

            Creature* broggok = LookupNpc(BF_NPC_BROGGOK);
            if (broggok != NULL)
                broggok->Despawn(0, 1000);
        }

        void ResetKelidanEvent()
        {
            m_channelerDeaths = 0;
            m_kelidanReleased = false;

            Creature* kelidan = LookupNpc(BF_NPC_KELIDAN);
            if (kelidan != NULL)
                PrepareKelidan(kelidan);

            for (uint32 i = 0; i < m_channelerGuids.size(); ++i)
            {
                Creature* channeler = GetMap() ? GetMap()->GetCreature(m_channelerGuids[i]) : NULL;
                if (channeler != NULL)
                {
                    if (!channeler->isAlive())
                        channeler->Despawn(0, 1000);
                    else
                        PrepareChanneler(channeler);
                }
            }
        }

        uint32 m_broggokPhase;
        bool m_broggokStarted;
        bool m_cellOpened[BF_MAX_ORC_WAVES];
        uint32 m_cellKills[BF_MAX_ORC_WAVES];
        vector<uint32> m_orcGuids;
        vector<uint32> m_waveOrcs[BF_MAX_ORC_WAVES];

        uint32 m_channelerDeaths;
        bool m_kelidanReleased;
        vector<uint32> m_channelerGuids;
    };

    BloodFurnaceContext* GetBloodFurnaceCtx(MapMgr* mgr)
    {
        return InstanceContextRegistry::GetContextAs<BloodFurnaceContext>(mgr);
    }

    class BloodFurnaceGOAI : public GameObjectAIScript
    {
    public:
        BloodFurnaceGOAI(GameObject* go) : GameObjectAIScript(go) {}
        static GameObjectAIScript* Create(GameObject* go) { return new BloodFurnaceGOAI(go); }

        void OnCreate() { Register(); }
        void OnSpawn() { Register(); }

        void OnActivate(Player* player)
        {
            if (_gameobject->GetEntry() != GO_PRISON_CELL_LEVER || player == NULL)
                return;

            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(_gameobject->GetMapMgr());
            if (ctx != NULL)
                ctx->StartBroggokEvent(player);
        }

    private:
        void Register()
        {
            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(_gameobject->GetMapMgr());
            if (ctx != NULL)
                ctx->RegisterGameObject(_gameobject);
        }
    };

    class TheMakerAI : public ScriptedCreatureAI
    {
    public:
        TheMakerAI(Creature* c)
            : ScriptedCreatureAI(c),
              m_acidSprayTimer(AddTimer(15000)),
              m_explodingBreakerTimer(AddTimer(6000)),
              m_dominationTimer(AddTimer(20000)),
              m_knockdownTimer(AddTimer(10000))
        {
        }

        ADD_CREATURE_FACTORY_FUNCTION(TheMakerAI)

        void Reset()
        {
            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->RegisterCreature(GetUnit());

            ResetTimer(m_acidSprayTimer, 15000);
            ResetTimer(m_explodingBreakerTimer, 6000);
            ResetTimer(m_dominationTimer, 20000);
            ResetTimer(m_knockdownTimer, 10000);
        }

        void EnterCombat(Unit* /*target*/)
        {
            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->SetEncounterState(BF_EVENT_THE_MAKER, ENCOUNTER_STATE_IN_PROGRESS);

            switch (Urand(0, 2))
            {
            case 0: DoYell("My work must not be interrupted."); break;
            case 1: DoYell("Perhaps I can find a use for you."); break;
            default: DoYell("Anger... Hate... These are tools I can use."); break;
            }
        }

        void LeaveCombat()
        {
            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL && !ctx->IsDone(BF_EVENT_THE_MAKER))
                ctx->SetEncounterState(BF_EVENT_THE_MAKER, ENCOUNTER_STATE_NOT_STARTED);
        }

        void OnTargetDied(Unit* /*target*/)
        {
            DoYell(Urand(0, 1) ? "Let's see what I can make of you." : "It is pointless to resist.");
        }

        void JustDied(Unit* /*killer*/)
        {
            DoYell("Stay away from... me.");

            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->SetEncounterState(BF_EVENT_THE_MAKER, ENCOUNTER_STATE_DONE);
        }

        void UpdateAI()
        {
            Unit* target = GetCurrentTarget(GetUnit());
            if (target == NULL || GetUnit()->isDead())
                return;

            if (IsTimerFinished(m_acidSprayTimer))
            {
                DoCastSelf(SPELL_ACID_SPRAY);
                ResetTimer(m_acidSprayTimer, Urand(15000, 23000));
            }

            if (IsTimerFinished(m_explodingBreakerTimer))
            {
                DoCast(target, IsHeroic(GetUnit()->GetMapMgr()) ? SPELL_EXPLODING_BREAKER_H : SPELL_EXPLODING_BREAKER);
                ResetTimer(m_explodingBreakerTimer, Urand(4000, 12000));
            }

            if (IsTimerFinished(m_dominationTimer))
            {
                DoCast(target, SPELL_DOMINATION);
                ResetTimer(m_dominationTimer, Urand(15000, 25000));
            }

            if (IsTimerFinished(m_knockdownTimer))
            {
                DoCast(target, SPELL_KNOCKDOWN);
                ResetTimer(m_knockdownTimer, Urand(4000, 12000));
            }
        }

    private:
        uint32 m_acidSprayTimer;
        uint32 m_explodingBreakerTimer;
        uint32 m_dominationTimer;
        uint32 m_knockdownTimer;
    };

    class BroggokAI : public ScriptedCreatureAI
    {
    public:
        BroggokAI(Creature* c)
            : ScriptedCreatureAI(c),
              m_slimeSprayTimer(AddTimer(10000)),
              m_poisonCloudTimer(AddTimer(5000)),
              m_poisonBoltTimer(AddTimer(7000))
        {
        }

        ADD_CREATURE_FACTORY_FUNCTION(BroggokAI)

        void Reset()
        {
            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->RegisterCreature(GetUnit());

            ResetTimer(m_slimeSprayTimer, 10000);
            ResetTimer(m_poisonCloudTimer, 5000);
            ResetTimer(m_poisonBoltTimer, 7000);
        }

        void EnterCombat(Unit* /*target*/)
        {
            DoYell("Come, intruders....");

            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->SetData(BF_EVENT_BROGGOK, ENCOUNTER_STATE_IN_PROGRESS);
        }

        void LeaveCombat()
        {
            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL && !ctx->IsDone(BF_EVENT_BROGGOK))
                ctx->SetEncounterState(BF_EVENT_BROGGOK, ENCOUNTER_STATE_FAILED);
        }

        void JustDied(Unit* /*killer*/)
        {
            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->SetEncounterState(BF_EVENT_BROGGOK, ENCOUNTER_STATE_DONE);
        }

        void UpdateAI()
        {
            Unit* target = GetCurrentTarget(GetUnit());
            if (target == NULL || GetUnit()->isDead())
                return;

            if (IsTimerFinished(m_slimeSprayTimer))
            {
                DoCastSelf(IsHeroic(GetUnit()->GetMapMgr()) ? SPELL_SLIME_SPRAY_H : SPELL_SLIME_SPRAY);
                ResetTimer(m_slimeSprayTimer, Urand(4000, 12000));
            }

            if (IsTimerFinished(m_poisonBoltTimer))
            {
                DoCast(target, IsHeroic(GetUnit()->GetMapMgr()) ? SPELL_POISON_BOLT_H : SPELL_POISON_BOLT);
                ResetTimer(m_poisonBoltTimer, Urand(4000, 12000));
            }

            if (IsTimerFinished(m_poisonCloudTimer))
            {
                DoCastSelf(SPELL_POISON_CLOUD);
                ResetTimer(m_poisonCloudTimer, 20000);
            }
        }

    private:
        uint32 m_slimeSprayTimer;
        uint32 m_poisonCloudTimer;
        uint32 m_poisonBoltTimer;
    };

    class BroggokPoisonCloudAI : public ScriptedCreatureAI
    {
    public:
        BroggokPoisonCloudAI(Creature* c) : ScriptedCreatureAI(c) {}
        ADD_CREATURE_FACTORY_FUNCTION(BroggokPoisonCloudAI)

        void Reset()
        {
            GetUnit()->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            GetUnit()->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_ATTACKABLE_9);
            DoCastSelf(SPELL_POISON);
        }
    };

    class NascentFelOrcAI : public ScriptedCreatureAI
    {
    public:
        NascentFelOrcAI(Creature* c) : ScriptedCreatureAI(c) {}
        ADD_CREATURE_FACTORY_FUNCTION(NascentFelOrcAI)

        void Reset()
        {
            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->RegisterCreature(GetUnit());
        }

        void LeaveCombat()
        {
            if (GetUnit()->isDead())
                return;

            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->FailBroggokFromOrc(GetUnit());
        }

        void JustDied(Unit* killer)
        {
            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->NotifyBroggokOrcDeath(GetUnit(), killer);
        }
    };

    class KelidanAI : public ScriptedCreatureAI
    {
    public:
        KelidanAI(Creature* c)
            : ScriptedCreatureAI(c),
              m_shadowVolleyTimer(AddTimer(1000)),
              m_burningNovaTimer(AddTimer(15000)),
              m_fireNovaTimer(AddTimer(5000)),
              m_corruptionTimer(AddTimer(5000)),
              m_fireNovaPending(false)
        {
            StartAIUpdate();
        }

        ADD_CREATURE_FACTORY_FUNCTION(KelidanAI)

        void Reset()
        {
            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->RegisterCreature(GetUnit());

            ResetTimer(m_shadowVolleyTimer, 1000);
            ResetTimer(m_burningNovaTimer, 15000);
            ResetTimer(m_fireNovaTimer, 5000);
            ResetTimer(m_corruptionTimer, 5000);
            m_fireNovaPending = false;
        }

        void EnterCombat(Unit* target)
        {
            DoYell("Who disturbs this sanctuary?");

            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
            {
                ctx->SetData(BF_EVENT_KELIDAN, ENCOUNTER_STATE_IN_PROGRESS);
                ctx->WakeChannelers(target);
            }
        }

        void LeaveCombat()
        {
            if (GetUnit()->isDead())
                return;

            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL && !ctx->IsDone(BF_EVENT_KELIDAN))
                ctx->SetEncounterState(BF_EVENT_KELIDAN, ENCOUNTER_STATE_FAILED);
        }

        void OnTargetDied(Unit* /*target*/)
        {
            if (Urand(0, 1))
                DoYell("Just as you deserve.");
            else
                DoYell("Your friends will soon be joining you.");
        }

        void JustDied(Unit* /*killer*/)
        {
            DoYell("Good luck... You'll need it.");

            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->SetEncounterState(BF_EVENT_KELIDAN, ENCOUNTER_STATE_DONE);
        }

        void UpdateAI()
        {
            Unit* target = GetCurrentTarget(GetUnit());
            if (target == NULL || GetUnit()->isDead())
                return;

            if (m_fireNovaPending && IsTimerFinished(m_fireNovaTimer))
            {
                if (DoCastSelf(IsHeroic(GetUnit()->GetMapMgr()) ? SPELL_FIRE_NOVA_H : SPELL_FIRE_NOVA))
                    m_fireNovaPending = false;
            }

            if (IsTimerFinished(m_shadowVolleyTimer))
            {
                DoCastSelf(IsHeroic(GetUnit()->GetMapMgr()) ? SPELL_SHADOW_BOLT_VOLLEY_H : SPELL_SHADOW_BOLT_VOLLEY);
                ResetTimer(m_shadowVolleyTimer, Urand(5000, 13000));
            }

            if (IsTimerFinished(m_corruptionTimer))
            {
                DoCast(target, SPELL_CORRUPTION);
                ResetTimer(m_corruptionTimer, Urand(30000, 50000));
            }

            if (IsTimerFinished(m_burningNovaTimer))
            {
                if (DoCastSelf(SPELL_BURNING_NOVA))
                {
                    DoYell("Closer... Come closer... and burn!");
                    if (IsHeroic(GetUnit()->GetMapMgr()))
                        DoCastSelf(SPELL_VORTEX);

                    ResetTimer(m_burningNovaTimer, Urand(20000, 28000));
                    ResetTimer(m_fireNovaTimer, 5000);
                    m_fireNovaPending = true;
                }
            }
        }

    private:
        uint32 m_shadowVolleyTimer;
        uint32 m_burningNovaTimer;
        uint32 m_fireNovaTimer;
        uint32 m_corruptionTimer;
        bool m_fireNovaPending;
    };

    class ShadowmoonChannelerAI : public ScriptedCreatureAI
    {
    public:
        ShadowmoonChannelerAI(Creature* c)
            : ScriptedCreatureAI(c),
              m_shadowBoltTimer(AddTimer(1500)),
              m_markTimer(AddTimer(6000))
        {
        }

        ADD_CREATURE_FACTORY_FUNCTION(ShadowmoonChannelerAI)

        void Reset()
        {
            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->RegisterCreature(GetUnit());

            ResetTimer(m_shadowBoltTimer, Urand(1000, 2000));
            ResetTimer(m_markTimer, Urand(5000, 7000));
        }

        void EnterCombat(Unit* target)
        {
            switch (Urand(0, 2))
            {
            case 0: DoYell("You mustn't let him loose!"); break;
            case 1: DoYell("Ignorant whelps!"); break;
            default: DoYell("You fools, he'll kill us all!"); break;
            }

            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->WakeChannelers(target);
        }

        void LeaveCombat()
        {
            if (GetUnit()->isDead())
                return;

            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL && !ctx->IsDone(BF_EVENT_KELIDAN))
                ctx->SetEncounterState(BF_EVENT_KELIDAN, ENCOUNTER_STATE_FAILED);
        }

        void JustDied(Unit* killer)
        {
            BloodFurnaceContext* ctx = GetBloodFurnaceCtx(GetUnit()->GetMapMgr());
            if (ctx != NULL)
                ctx->NotifyChannelerDeath(killer);
        }

        void UpdateAI()
        {
            Unit* target = GetCurrentTarget(GetUnit());
            if (target == NULL || GetUnit()->isDead())
                return;

            if (IsTimerFinished(m_markTimer))
            {
                DoCast(target, SPELL_MARK_OF_SHADOW);
                ResetTimer(m_markTimer, Urand(15000, 20000));
            }

            if (IsTimerFinished(m_shadowBoltTimer))
            {
                DoCast(target, IsHeroic(GetUnit()->GetMapMgr()) ? SPELL_CHANNELER_BOLT_H : SPELL_CHANNELER_BOLT);
                ResetTimer(m_shadowBoltTimer, Urand(5000, 6000));
            }
        }

    private:
        uint32 m_shadowBoltTimer;
        uint32 m_markTimer;
    };

} // anonymous namespace

void SetupBloodFurnace(ScriptMgr* mgr)
{
    InstanceContextRegistry::RegisterFactory(MAP_BLOOD_FURNACE,
                                             BloodFurnaceContext::CreateContext);

    mgr->register_creature_script(CN_THE_MAKER,            TheMakerAI::Create);
    mgr->register_creature_script(CN_BROGGOK,              BroggokAI::Create);
    mgr->register_creature_script(CN_BROGGOK_POISON_CLOUD, BroggokPoisonCloudAI::Create);
    mgr->register_creature_script(CN_NASCENT_FEL_ORC,      NascentFelOrcAI::Create);
    mgr->register_creature_script(CN_KELIDAN_THE_BREAKER,  KelidanAI::Create);
    mgr->register_creature_script(CN_SHADOWMOON_CHANNELER, ShadowmoonChannelerAI::Create);

    mgr->register_gameobject_script(GO_DOOR_FINAL_EXIT,       BloodFurnaceGOAI::Create);
    mgr->register_gameobject_script(GO_DOOR_MAKER_FRONT,      BloodFurnaceGOAI::Create);
    mgr->register_gameobject_script(GO_DOOR_MAKER_REAR,       BloodFurnaceGOAI::Create);
    mgr->register_gameobject_script(GO_DOOR_BROGGOK_REAR,     BloodFurnaceGOAI::Create);
    mgr->register_gameobject_script(GO_DOOR_BROGGOK_FRONT,    BloodFurnaceGOAI::Create);
    mgr->register_gameobject_script(GO_DOOR_KELIDAN_EXIT,     BloodFurnaceGOAI::Create);
    mgr->register_gameobject_script(GO_PRISON_CELL_BROGGOK_1, BloodFurnaceGOAI::Create);
    mgr->register_gameobject_script(GO_PRISON_CELL_BROGGOK_2, BloodFurnaceGOAI::Create);
    mgr->register_gameobject_script(GO_PRISON_CELL_BROGGOK_3, BloodFurnaceGOAI::Create);
    mgr->register_gameobject_script(GO_PRISON_CELL_BROGGOK_4, BloodFurnaceGOAI::Create);
    mgr->register_gameobject_script(GO_PRISON_CELL_LEVER,     BloodFurnaceGOAI::Create);
}
