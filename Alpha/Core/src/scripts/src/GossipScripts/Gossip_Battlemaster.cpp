#include "StdAfx.h"
#include "Setup.h"

class SCRIPT_DECL BattlemasterGossip : public GossipScript
{
public:
	BattlemasterGossip(uint32 minLevel, uint32 allianceTextId, uint32 hordeTextId) :
		m_minLevel(minLevel), m_allianceTextId(allianceTextId), m_hordeTextId(hordeTextId)
	{
	}

	void GossipHello(Object* pObject, Player* plr, bool AutoSend)
	{
		if(pObject == NULL || plr == NULL)
			return;

		GossipMenu* Menu;
		uint32 team = plr->GetTeam();
		if(team > 1)
			team = 1;

		uint32 textId = (team == 0) ? m_allianceTextId : m_hordeTextId;
		objmgr.CreateGossipMenuForPlayer(&Menu, pObject->GetGUID(), textId, plr);

		if(plr->getLevel() >= m_minLevel)
			Menu->AddItem(0, "I would like to enter the battleground.", 1);

		if(AutoSend)
			Menu->SendTo(plr);
	}

	void GossipSelectOption(Object* pObject, Player* plr, uint32 Id, uint32 IntId, const char* Code)
	{
		if(pObject == NULL || plr == NULL || pObject->GetTypeId() != TYPEID_UNIT)
			return;

		if(IntId != 1)
			return;

		plr->Gossip_Complete();
		plr->GetSession()->SendBattlegroundListForBattlemaster(static_cast< Creature* >(pObject));
	}

	void Destroy()
	{
		delete this;
	}

private:
	uint32 m_minLevel;
	uint32 m_allianceTextId;
	uint32 m_hordeTextId;
};

void SetupBattlemaster(ScriptMgr * mgr)
{
	GossipScript* wsg = (GossipScript*)new BattlemasterGossip(10, 7689, 7705);
	GossipScript* ab = (GossipScript*)new BattlemasterGossip(20, 7700, 7555);
	GossipScript* av = (GossipScript*)new BattlemasterGossip(60, 7658, 7659);

    /* Battlemaster List */
    mgr->register_gossip_script(19910, wsg); // Gargok
    mgr->register_gossip_script(15105, wsg); // Warsong Emissary
    mgr->register_gossip_script(20118, wsg); // Jihi
    mgr->register_gossip_script(16696, wsg); // Krukk
    mgr->register_gossip_script(2804, wsg);  // Kurden Bloodclaw
    mgr->register_gossip_script(20272, wsg); // Lylandor
    mgr->register_gossip_script(20269, wsg); // Montok Redhands
    mgr->register_gossip_script(19908, wsg); // Su'ura Swiftarrow
    mgr->register_gossip_script(15102, wsg); // Silverwing Emissary
    mgr->register_gossip_script(14981, wsg); // Elfarran
    mgr->register_gossip_script(14982, wsg); // Lylandris
    mgr->register_gossip_script(2302, wsg);  // Aethalas
    mgr->register_gossip_script(10360, wsg); // Kergul Bloodaxe
    mgr->register_gossip_script(3890, wsg);  // Brakgul Deathbringer
    mgr->register_gossip_script(20273, ab); // Adam Eternum
    mgr->register_gossip_script(16694, ab); // Karen Wentworth
    mgr->register_gossip_script(20274, ab); // Keldor the Lost
    mgr->register_gossip_script(15007, ab); // Sir Malory Wheeler
    mgr->register_gossip_script(19855, ab); // Sir Maximus Adams
    mgr->register_gossip_script(19905, ab); // The Black Bride
    mgr->register_gossip_script(20120, ab); // Tolo
    mgr->register_gossip_script(15008, ab); // Lady Hoteshem
    mgr->register_gossip_script(857, ab);   // Donald Osgood
    mgr->register_gossip_script(907, ab);   // Keras Wolfheart
    mgr->register_gossip_script(12198, ab); // Martin Lindsev
    mgr->register_gossip_script(14990, ab); // Defilers Emissary
    mgr->register_gossip_script(15006, ab); // Deze Snowbane
    mgr->register_gossip_script(14991, ab); // League of Arathor Emissary
    mgr->register_gossip_script(347, av);   // Grizzle Halfmane
    mgr->register_gossip_script(19907, av); // Grumbol Grimhammer
    mgr->register_gossip_script(16695, av); // Gurak
    mgr->register_gossip_script(20271, av); // Haelga Slatefist
    mgr->register_gossip_script(20119, av); // Mahul
    mgr->register_gossip_script(19906, av); // Usha Eyegouge
    mgr->register_gossip_script(20276, av); // Wolf-Sister Maka
    mgr->register_gossip_script(7410, av);  // Thelman Slatefist
    mgr->register_gossip_script(12197, av); // Glordrum Steelbeard
    mgr->register_gossip_script(5118, av);  // Brogun Stoneshield
    mgr->register_gossip_script(15106, av); // Frostwolf Emissary
    mgr->register_gossip_script(15103, av); // Stormpike Emissary
    mgr->register_gossip_script(14942, av); // Kartra Bloodsnarl

   //cleanup:
   //removed Sandfury Soul Eater(hes a npc in Zul'Farrak and has noting to do whit the battleground masters)
   //added Warsong Emissary, Stormpike Emissary , League of Arathor Emissary
}
