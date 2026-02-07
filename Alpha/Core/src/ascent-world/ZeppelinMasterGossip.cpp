//
// AscentEmu 2.4.3 - Zeppelin Master gossip ("Where is the zeppelin now?")
//
// Core-side gossip script to provide zeppelin status/ETA.
//

#include "StdAfx.h"
#include "ZeppelinMasterGossip.h"

#include <float.h>

// This display id is used by zeppelins in 2.4.3 (see TransporterHandler.cpp horn handling).
static const uint32 ZEPPELIN_DISPLAY_ID = 3031;

Transporter* ZeppelinMasterGossip::FindAssociatedTransport(Creature* master, uint32& outDockTime, float& outDockDistSq)
{
    outDockTime = 0;
    outDockDistSq = 0.0f;

    if (master == NULL)
        return NULL;

    // Associate a Zeppelin Master to a specific zeppelin transport by looking for a transport that
    // has a dock (a delayed waypoint) on the master's map near the master NPC.
    std::vector<Transporter*> transports;
    objmgr.CopyTransports(transports);

    const uint32 masterMap = master->GetMapId();
    const float mx = master->GetPositionX();
    const float my = master->GetPositionY();
    const float mz = master->GetPositionZ();

    Transporter* best = NULL;
    float bestDistSq = FLT_MAX;
    uint32 bestDockTime = 0;

    for (size_t i = 0; i < transports.size(); ++i)
    {
        Transporter* t = transports[i];
        if (t == NULL || t->GetInfo() == NULL)
            continue;

        if (t->GetInfo()->DisplayID != ZEPPELIN_DISPLAY_ID)
            continue;

        uint32 dockTime = 0;
        float dockDistSq = 0.0f;
        if (!t->FindDockNear(masterMap, mx, my, mz, 120.0f, dockTime, dockDistSq))
            continue;

        if (dockDistSq < bestDistSq)
        {
            bestDistSq = dockDistSq;
            bestDockTime = dockTime;
            best = t;
        }
    }

    if (best)
    {
        outDockTime = bestDockTime;
        outDockDistSq = bestDistSq;
    }
    return best;
}

static uint32 ModDiff(uint32 a, uint32 b, uint32 mod)
{
    // returns (a - b) mod mod
    if (mod == 0)
        return 0;

    if (a >= b)
        return (a - b) % mod;

    return (mod - ((b - a) % mod)) % mod;
}

std::string ZeppelinMasterGossip::BuildStatusLine(Creature* master, Transporter* t, uint32 dockTimeMs, float dockDistSq)
{
    if (master == NULL || t == NULL)
        return "I don't have any information right now.";

    const bool sameMap = (t->GetMapId() == master->GetMapId());
    const float dockedDistSq = 75.0f * 75.0f;
    const bool nearDock = sameMap && (dockDistSq <= dockedDistSq);

    // Compute ETA (to this dock) based on the transport's internal path timer.
    uint32 etaMs = 0;
    if (dockTimeMs != 0 && t->m_pathTime != 0)
        etaMs = ModDiff(dockTimeMs, t->m_timer % t->m_pathTime, t->m_pathTime);

    char buf[256];

    if (nearDock)
    {
        // We can't reliably know the remaining dock delay without storing it per waypoint, so keep it simple.
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "The zeppelin is currently docked. It will depart soon.");
        return std::string(buf);
    }

    if (etaMs == 0)
    {
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "The zeppelin is en route.");
        return std::string(buf);
    }

    // Round to seconds for user-facing output.
    uint32 etaSec = (etaMs + 500) / 1000;
    if (etaSec <= 30)
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "The zeppelin should arrive shortly.");
     else
        _snprintf_s(buf, sizeof(buf), _TRUNCATE, "The zeppelin is en route.");

    return std::string(buf);
}

void ZeppelinMasterGossip::GossipHello(Object* pObject, Player* Plr, bool AutoSend)
{
    Creature* pCreature = (pObject && pObject->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(pObject) : NULL;
    if (pCreature == NULL || Plr == NULL)
        return;

    GossipMenu* Menu;
    uint32 textId = 2;

    uint32 dbText = objmgr.GetGossipTextForNpc(pCreature->GetEntry());
    if (dbText != 0 && NpcTextStorage.LookupEntry(dbText) != NULL)
        textId = dbText;

    objmgr.CreateGossipMenuForPlayer(&Menu, pCreature->GetGUID(), textId, Plr);

    Menu->AddItem(0, "Where is the zeppelin now?", GOSSIP_INTID_ZEPPELIN_STATUS);

    if (AutoSend)
        Menu->SendTo(Plr);
}

void ZeppelinMasterGossip::GossipSelectOption(Object* pObject, Player* Plr, uint32 Id, uint32 IntId, const char* EnteredCode)
{
    Creature* pCreature = (pObject && pObject->GetTypeId() == TYPEID_UNIT) ? static_cast<Creature*>(pObject) : NULL;
    if (pCreature == NULL || Plr == NULL)
        return;

    if (IntId != GOSSIP_INTID_ZEPPELIN_STATUS)
    {
        sScriptMgr.GetDefaultGossipScript()->GossipSelectOption(pObject, Plr, Id, IntId, EnteredCode);
        return;
    }

    uint32 dockTimeMs = 0;
    float dockDistSq = 0.0f;
    Transporter* t = FindAssociatedTransport(pCreature, dockTimeMs, dockDistSq);

    std::string msg = BuildStatusLine(pCreature, t, dockTimeMs, dockDistSq);

    // Present in the blizzlike parchment text window.
    // We do this by setting per-player dynamic npc_text and opening a gossip menu that points
    // at Player::DYNAMIC_NPC_TEXT_ID. The core serves the text in HandleNpcTextQueryOpcode.
    Plr->SetDynamicNpcText(msg);

    GossipMenu* Menu;
    objmgr.CreateGossipMenuForPlayer(&Menu, pCreature->GetGUID(), Player::DYNAMIC_NPC_TEXT_ID, Plr);
    Menu->SendTo(Plr);
}