#include "StdAfx.h"
#include "Setup.h"
#include "InstanceContext.h"

// ---------------------------------------------------------------------------
// Static member definitions for InstanceContextRegistry
// ---------------------------------------------------------------------------

unordered_map<uint32, InstanceContextFactory> InstanceContextRegistry::s_factories;
unordered_map<uint32, InstanceContext*>       InstanceContextRegistry::s_contexts;

// ---------------------------------------------------------------------------
// InstanceContext
// ---------------------------------------------------------------------------

InstanceContext::InstanceContext(MapMgr* mgr)
    : InstanceScript(mgr)
{
}

void InstanceContext::Destroy()
{
    // Matches the GossipScript::Destroy() convention used throughout Ascent:
    // the object is allocated in the script DLL and must be freed by DLL code
    // so that new/delete always pair within the same CRT heap.
    // ~MapMgr() calls Destroy() and must NOT follow with a bare delete.
    if (_instance != NULL)
        InstanceContextRegistry::UnregisterContext(_instance->GetInstanceID());
    delete this;
}

// -- Encounter state ---------------------------------------------------------

void InstanceContext::SetData(uint32 type, uint32 value)
{
    m_data[type] = value;
}

uint32 InstanceContext::GetData(uint32 type) const
{
    unordered_map<uint32, uint32>::const_iterator it = m_data.find(type);
    return (it != m_data.end()) ? it->second : ENCOUNTER_STATE_NOT_STARTED;
}

bool InstanceContext::IsDone(uint32 type) const
{
    return GetData(type) == ENCOUNTER_STATE_DONE;
}

// -- GUID storage ------------------------------------------------------------

void InstanceContext::StoreNpcGuid(uint32 type, uint32 lowGuid)
{
    m_npcGuids[type] = lowGuid;
}

void InstanceContext::StoreGOGuid(uint32 type, uint32 lowGuid)
{
    m_goGuids[type] = lowGuid;
}

uint32 InstanceContext::GetNpcGuid(uint32 type) const
{
    unordered_map<uint32, uint32>::const_iterator it = m_npcGuids.find(type);
    return (it != m_npcGuids.end()) ? it->second : 0;
}

uint32 InstanceContext::GetGOGuid(uint32 type) const
{
    unordered_map<uint32, uint32>::const_iterator it = m_goGuids.find(type);
    return (it != m_goGuids.end()) ? it->second : 0;
}

Creature* InstanceContext::LookupNpc(uint32 type) const
{
    if (!_instance)
        return NULL;
    uint32 guid = GetNpcGuid(type);
    return (guid != 0) ? _instance->GetCreature(guid) : NULL;
}

GameObject* InstanceContext::LookupGO(uint32 type) const
{
    if (!_instance)
        return NULL;
    uint32 guid = GetGOGuid(type);
    return (guid != 0) ? _instance->GetGameObject(guid) : NULL;
}

// -- Door helpers ------------------------------------------------------------

void InstanceContext::OpenDoor(uint32 goType)
{
    SetDoorOpen(LookupGO(goType));
}

void InstanceContext::CloseDoor(uint32 goType)
{
    SetDoorClosed(LookupGO(goType));
}

void InstanceContext::SetDoorOpen(GameObject* go)
{
    if (!go)
        return;
    go->SetUInt32Value(GAMEOBJECT_FLAGS, INSTANCE_DOOR_FLAGS_OPEN);
    go->SetUInt32Value(GAMEOBJECT_STATE, INSTANCE_DOOR_STATE_OPEN);
}

void InstanceContext::SetDoorClosed(GameObject* go)
{
    if (!go)
        return;
    go->SetUInt32Value(GAMEOBJECT_FLAGS, INSTANCE_DOOR_FLAGS_CLOSED);
    go->SetUInt32Value(GAMEOBJECT_STATE, INSTANCE_DOOR_STATE_CLOSED);
}

// -- Safe despawn ------------------------------------------------------------

void InstanceContext::SafeDespawn(Creature* c, uint32 despawnDelayMs, uint32 respawnDelayMs)
{
    if (!c || !c->IsInWorld())
        return;
    c->Despawn(despawnDelayMs, respawnDelayMs);
}

// ---------------------------------------------------------------------------
// InstanceContextRegistry
// ---------------------------------------------------------------------------

void InstanceContextRegistry::RegisterFactory(uint32 mapId, InstanceContextFactory factory)
{
    s_factories[mapId] = factory;
}

InstanceContext* InstanceContextRegistry::GetContext(MapMgr* mgr)
{
    if (!mgr)
        return NULL;

    const uint32 instanceId = mgr->GetInstanceID();

    unordered_map<uint32, InstanceContext*>::iterator ctxIt = s_contexts.find(instanceId);
    if (ctxIt != s_contexts.end())
        return ctxIt->second;

    // No context yet - look for a registered factory for this map.
    const uint32 mapId = mgr->GetMapId();
    unordered_map<uint32, InstanceContextFactory>::iterator factIt = s_factories.find(mapId);
    if (factIt == s_factories.end())
        return NULL;

    InstanceContext* ctx = factIt->second(mgr);
    s_contexts[instanceId] = ctx;
    mgr->m_instanceScript = ctx;  // lets ~MapMgr auto-destroy via Destroy()
    return ctx;
}

void InstanceContextRegistry::UnregisterContext(uint32 instanceId)
{
    s_contexts.erase(instanceId);
}

void InstanceContextRegistry::DestroyContext(uint32 instanceId)
{
    unordered_map<uint32, InstanceContext*>::iterator it = s_contexts.find(instanceId);
    if (it == s_contexts.end())
        return;

    InstanceContext* ctx = it->second;
    s_contexts.erase(it);

    // Null the back-pointer so ~MapMgr does not double-destroy this context.
    if (ctx->GetMap() != NULL)
        ctx->GetMap()->m_instanceScript = NULL;

    delete ctx;
}
