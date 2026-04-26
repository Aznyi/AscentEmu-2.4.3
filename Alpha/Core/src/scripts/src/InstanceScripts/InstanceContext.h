// InstanceContext.h
// Per-instance shared state framework for Ascent TBC dungeon/raid scripts.
//
// Design:
//   - InstanceContext extends the core InstanceScript base (MapScriptInterface.h).
//   - InstanceContextRegistry owns two tables:
//       s_factories : mapId  -> factory function  (one entry per dungeon type)
//       s_contexts  : instanceId -> InstanceContext* (one entry per live run)
//     The context for a live instance is stored and retrieved by instanceId
//     (MapMgr::GetInstanceID()), which is a server-wide monotonically increasing
//     counter and is never reused within a process lifetime.  Two groups running
//     the same map in parallel receive different instanceIds and therefore
//     completely independent InstanceContext objects.
//   - Creature and GO scripts retrieve the context via
//     InstanceContextRegistry::GetContextAs<T>(GetUnit()->GetMapMgr()).
//   - The context is lazily created on first access.
//
// Lifetime / cleanup model:
//   MapMgr::~MapMgr() calls m_instanceScript->Destroy() — which self-deletes
//   the context from within the script DLL (matching GossipScript::Destroy()).
//   ~MapMgr() must NOT follow with delete m_instanceScript; the pointer is
//   dangling after Destroy() returns.  GetContext() sets
//   mgr->m_instanceScript = ctx on first creation so ~MapMgr can always find
//   it regardless of whether a script ever called DestroyContext() early.
//
//   Scripts may also call DestroyContext() early (e.g. after the final boss)
//   to release the context before instance teardown.  DestroyContext() nulls
//   mgr->m_instanceScript so ~MapMgr does not double-destroy.
//
//   The correct early-cleanup place is the FINAL BOSS's OnCombatStop, not
//   OnDied.  In Ascent, OnCombatStop fires AFTER OnDied (via the
//   RemoveThreatByPtr -> EVENT_LEAVECOMBAT path).  If DestroyContext is called
//   from OnDied, OnCombatStop will re-create an empty orphaned context via the
//   lazy-creation path.  Calling it from OnCombatStop (after confirming IsDone)
//   avoids that problem.  Early cleanup is now optional; ~MapMgr will handle
//   it regardless.
//
// Usage:
//   1. Define an encounter-type enum (e.g. MyBoss_BOSS_ARUGAL = 0).
//   2. Subclass InstanceContext, add ADD_INSTANCE_CONTEXT_FACTORY(MyContext).
//   3. In SetupMyInstance(), call InstanceContextRegistry::RegisterFactory(mapId, ...).
//   4. Final boss OnDied:      ctx->SetData(type, ENCOUNTER_STATE_DONE);
//      Final boss OnCombatStop: if (ctx->IsDone(type)) DestroyContext(instanceId);
//
// Include this header after StdAfx.h (types come from the precompiled header).

#ifndef INSTANCE_CONTEXT_H
#define INSTANCE_CONTEXT_H

// ---------------------------------------------------------------------------
// Encounter state values (same semantics as ScriptDev2 for future porting)
// ---------------------------------------------------------------------------
enum EncounterState : uint32
{
    ENCOUNTER_STATE_NOT_STARTED = 0,
    ENCOUNTER_STATE_IN_PROGRESS = 1,
    ENCOUNTER_STATE_DONE        = 2,
    ENCOUNTER_STATE_FAILED      = 3,
};

// ---------------------------------------------------------------------------
// Door flag/state values matching the TBC client wire format.
// These match the constants in Instance_Deadmines.cpp.
// ---------------------------------------------------------------------------
enum InstanceDoorConst : uint32
{
    INSTANCE_DOOR_FLAGS_OPEN   = 33,  // GAMEOBJECT_FLAGS when door is open
    INSTANCE_DOOR_FLAGS_CLOSED = 34,  // GAMEOBJECT_FLAGS when door is closed
    INSTANCE_DOOR_STATE_OPEN   = 0,   // GAMEOBJECT_STATE when open
    INSTANCE_DOOR_STATE_CLOSED = 1,   // GAMEOBJECT_STATE when closed
};

// ---------------------------------------------------------------------------
// InstanceContext
// Per-instance class that holds encounter state, cached GUIDs, and helpers.
// Subclass this for each dungeon/raid instance script.
// ---------------------------------------------------------------------------
class InstanceContext : public InstanceScript
{
public:
    explicit InstanceContext(MapMgr* mgr);
    virtual ~InstanceContext() {}

    // Called by ~MapMgr via m_instanceScript->Destroy().  Removes from the
    // registry, then self-deletes (delete this).  ~MapMgr must NOT delete
    // m_instanceScript afterward — the object is gone after Destroy() returns.
    virtual void Destroy();

    // -- Encounter state (keyed by a script-defined uint32 enum) -------------

    void   SetData(uint32 type, uint32 value);
    uint32 GetData(uint32 type) const;

    // Convenience: true when value == ENCOUNTER_STATE_DONE
    bool IsDone(uint32 type) const;

    // -- GUID storage (use GetLowGUID() when registering) --------------------
    // Store by script-defined type enum so scripts never hold raw pointers
    // across frames.

    void StoreNpcGuid(uint32 type, uint32 lowGuid);
    void StoreGOGuid(uint32 type, uint32 lowGuid);
    uint32 GetNpcGuid(uint32 type) const;
    uint32 GetGOGuid(uint32 type) const;

    // Resolve a stored GUID to a live object (returns NULL if not found).
    Creature*   LookupNpc(uint32 type) const;
    GameObject* LookupGO(uint32 type) const;

    // -- Door / button helpers -----------------------------------------------

    // Open/close a door identified by its stored GO-type enum.
    void OpenDoor(uint32 goType);
    void CloseDoor(uint32 goType);

    // Direct GO state helpers (safe if go is NULL).
    void SetDoorOpen(GameObject* go);
    void SetDoorClosed(GameObject* go);

    // -- Safe creature despawn -----------------------------------------------
    // despawnDelayMs : milliseconds before the unit leaves the world.
    // respawnDelayMs : milliseconds until it respawns at its spawn point;
    //                  0 = permanent removal (SafeDelete).
    static void SafeDespawn(Creature* c,
                            uint32 despawnDelayMs = 500,
                            uint32 respawnDelayMs = 0);

    // -- Map accessor --------------------------------------------------------
    MapMgr* GetMap() const { return _instance; }

protected:
    unordered_map<uint32, uint32> m_data;      // type -> EncounterState value
    unordered_map<uint32, uint32> m_npcGuids;  // type -> creature low GUID
    unordered_map<uint32, uint32> m_goGuids;   // type -> GO low GUID
};

// ---------------------------------------------------------------------------
// InstanceContextFactory
// Factory function type - one per map ID, registered in a SetupXxx() call.
// ---------------------------------------------------------------------------
typedef InstanceContext* (*InstanceContextFactory)(MapMgr*);

// ---------------------------------------------------------------------------
// InstanceContextRegistry
// Script-side singleton registry.  Creature/GO scripts call GetContext() or
// GetContextAs<T>() to obtain their instance's shared state object.
// The registry owns all created context objects.
// ---------------------------------------------------------------------------
struct InstanceContextRegistry
{
    // Register a factory for a map ID.  Call once from SetupXxx().
    static void RegisterFactory(uint32 mapId, InstanceContextFactory factory);

    // Get (or lazily create) the context for the MapMgr's instance.
    // Returns NULL if no factory is registered for mgr->GetMapId().
    static InstanceContext* GetContext(MapMgr* mgr);

    // Typed access - casts result to T* (no dynamic_cast; caller ensures type).
    template<typename T>
    static T* GetContextAs(MapMgr* mgr)
    {
        return static_cast<T*>(GetContext(mgr));
    }

    // Optional early cleanup: delete and remove the context for this instance
    // before MapMgr teardown.  Nulls mgr->m_instanceScript to prevent the
    // ~MapMgr double-destroy path.  Safe to call from the final boss's
    // OnCombatStop after IsDone check; ~MapMgr handles cleanup if not called.
    static void DestroyContext(uint32 instanceId);

    friend class InstanceContext;  // grants Destroy() access to UnregisterContext

private:
    // Remove from s_contexts without deleting (used by Destroy() so ~MapMgr
    // can perform the actual delete).
    static void UnregisterContext(uint32 instanceId);

    static unordered_map<uint32, InstanceContextFactory> s_factories;
    static unordered_map<uint32, InstanceContext*>       s_contexts;
};

// ---------------------------------------------------------------------------
// ADD_INSTANCE_CONTEXT_FACTORY
// Adds a static CreateContext() factory to a subclass, matching the
// InstanceContextFactory signature.  Usage is analogous to
// ADD_CREATURE_FACTORY_FUNCTION from ScriptMgr.h.
// ---------------------------------------------------------------------------
#define ADD_INSTANCE_CONTEXT_FACTORY(cls) \
    static InstanceContext* CreateContext(MapMgr* mgr) { return new cls(mgr); }

#endif // INSTANCE_CONTEXT_H
