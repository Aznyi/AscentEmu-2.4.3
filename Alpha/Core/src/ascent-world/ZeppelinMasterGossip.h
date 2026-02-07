//
// AscentEmu 2.4.3 - Zeppelin Master gossip ("Where is the zeppelin now?")
//
// Core-side gossip script to provide zeppelin status/ETA.
//

#ifndef ASCENT_ZEPPELIN_MASTER_GOSSIP_H
#define ASCENT_ZEPPELIN_MASTER_GOSSIP_H

#include "StdAfx.h"

class ZeppelinMasterGossip : public GossipScript
{
public:
    ZeppelinMasterGossip() {}
    virtual ~ZeppelinMasterGossip() {}

    virtual void GossipHello(Object* pObject, Player* Plr, bool AutoSend);
    virtual void GossipSelectOption(Object* pObject, Player* Plr, uint32 Id, uint32 IntId, const char* EnteredCode);

private:
    enum
    {
        GOSSIP_INTID_ZEPPELIN_STATUS = 50,
    };

    static Transporter* FindAssociatedTransport(Creature* master, uint32& outDockTime, float& outDockDistSq);
    static std::string BuildStatusLine(Creature* master, Transporter* t, uint32 dockTimeMs, float dockDistSq);
};

#endif
