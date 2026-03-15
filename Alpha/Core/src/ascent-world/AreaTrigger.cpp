/*
 * OpenAscent MMORPG Server
 * Copyright (C) 2008 <http://www.openascent.com/>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "StdAfx.h"
void WorldSession::HandleAreaTriggerOpcode(WorldPacket & recv_data)
{
	 if(!_player->IsInWorld()) return;
	CHECK_PACKET_SIZE(recv_data, 4);
	uint32 id ;
	recv_data >> id;
	_HandleAreaTriggerOpcode(id);
}

enum AreaTriggerFailures
{
	AREA_TRIGGER_FAILURE_OK				= 0,
	AREA_TRIGGER_FAILURE_UNAVAILABLE	= 1,
	AREA_TRIGGER_FAILURE_NO_BC			= 2,
	AREA_TRIGGER_FAILURE_NO_HEROIC		= 3,
	AREA_TRIGGER_FAILURE_NO_RAID		= 4,
	AREA_TRIGGER_FAILURE_NO_ATTUNE_Q	= 5,
	AREA_TRIGGER_FAILURE_NO_ATTUNE_I	= 6,
	AREA_TRIGGER_FAILURE_LEVEL			= 7,
	AREA_TRIGGER_FAILURE_NO_GROUP		= 8,
	AREA_TRIGGER_FAILURE_NO_KEY         = 9,
	AREA_TRIGGER_FAILURE_LEVEL_HEROIC	= 9,
	AREA_TRIGGER_FAILURE_NO_CHECK		= 10,
};

const char * AreaTriggerFailureMessages[] = {
	"-",
	"This instance is unavailable",
	"You must have The Burning Crusade Expansion to access this content.",
	"Heroic mode unavailable for this instance.",
	"You must be in a raid group to pass through here.",
	"You do not have the required attunement to pass through here.", //TODO: Replace attunment with real itemname
	"You do not have the required attunement to pass through here.", //TODO: Replace attunment with real itemname
	"You must be at least level %u to pass through here.",
	"You must be in a party to pass through here.",
	"You do not have the required attunement to pass through here.", //TODO: Replace attunment with real itemname
	"You must be level 70 to enter heroic mode.",
};

uint32 CheckTriggerPrerequsites(AreaTrigger * pAreaTrigger, WorldSession * pSession, Player * pPlayer, MapInfo * pMapInfo);

static void SendAreaTriggerFailure(WorldSession * session, uint32 reason, AreaTrigger * area_trigger)
{
	const char * failure_reason = AreaTriggerFailureMessages[reason];
	char msg[200];
	WorldPacket data(SMSG_AREA_TRIGGER_MESSAGE, 50);
	data << uint32(0);

	switch (reason)
	{
	case AREA_TRIGGER_FAILURE_LEVEL:
		snprintf(msg, 200, failure_reason, area_trigger->required_level);
		data << msg;
		break;
	case AREA_TRIGGER_FAILURE_NO_ATTUNE_I:
		{
			MapInfo * map_info = WorldMapInfoStorage.LookupEntry(area_trigger->Mapid);
			ItemPrototype * item_proto = map_info ? ItemPrototypeStorage.LookupEntry(map_info->required_item) : NULL;
			if(item_proto)
				snprintf(msg, 200, "You must have the item, `%s` to pass through here.", item_proto->Name1);
			else
				snprintf(msg, 200, "You must have the item, UNKNOWN to pass through here.");

			data << msg;
		}break;
	case AREA_TRIGGER_FAILURE_NO_ATTUNE_Q:
		{
			MapInfo * map_info = WorldMapInfoStorage.LookupEntry(area_trigger->Mapid);
			Quest * quest = map_info ? QuestStorage.LookupEntry(map_info->required_quest) : NULL;
			if(quest)
				snprintf(msg, 200, "You must have finished the quest, `%s` to pass through here.", quest->title);
			else
				snprintf(msg, 200, "You must have finished the quest, UNKNOWN to pass through here.");

			data << msg;
		}break;
	default:
		data << failure_reason;
		break;
	}

	data << uint8(0);
	session->SendPacket(&data);
}

static bool HandleInstanceStyleAreaTrigger(WorldSession * session, AreaTrigger * area_trigger)
{
	Player * player = session->GetPlayer();
	if(player->GetPlayerStatus() == TRANSFER_PENDING)
		return true;

	uint32 reason = CheckTriggerPrerequsites(area_trigger, session, player, WorldMapInfoStorage.LookupEntry(area_trigger->Mapid));
	if(reason != AREA_TRIGGER_FAILURE_OK)
	{
		SendAreaTriggerFailure(session, reason, area_trigger);
		return true;
	}

	player->SaveEntryPoint(area_trigger->Mapid);
	player->SafeTeleport(area_trigger->Mapid, 0, LocationVector(area_trigger->x, area_trigger->y, area_trigger->z, area_trigger->o));
	return true;
}

uint32 CheckTriggerPrerequsites(AreaTrigger * pAreaTrigger, WorldSession * pSession, Player * pPlayer, MapInfo * pMapInfo)
{
	if(pAreaTrigger->required_level && pPlayer->getLevel() < pAreaTrigger->required_level)
		return AREA_TRIGGER_FAILURE_LEVEL;

	if(!pMapInfo || !pMapInfo->HasFlag(WMI_INSTANCE_ENABLED))
		return AREA_TRIGGER_FAILURE_UNAVAILABLE;

	if(!pSession->HasFlag(ACCOUNT_FLAG_XPACK_01) && pMapInfo->HasFlag(WMI_INSTANCE_XPACK_01))
		return AREA_TRIGGER_FAILURE_NO_BC;

	// These can be overridden by cheats/GM
	if(pPlayer->triggerpass_cheat)
		return AREA_TRIGGER_FAILURE_OK;

	if(pPlayer->iInstanceType >= MODE_HEROIC && pMapInfo->type != INSTANCE_MULTIMODE && pMapInfo->type != INSTANCE_NULL)
		return AREA_TRIGGER_FAILURE_NO_HEROIC;

	if(pMapInfo->type == INSTANCE_RAID && (!pPlayer->GetGroup() || (pPlayer->GetGroup() && pPlayer->GetGroup()->GetGroupType() != GROUP_TYPE_RAID)))
		return AREA_TRIGGER_FAILURE_NO_RAID;

	if(pMapInfo->type == INSTANCE_MULTIMODE && !pPlayer->GetGroup())
		return AREA_TRIGGER_FAILURE_NO_GROUP;

	if(pMapInfo && pMapInfo->required_quest && !pPlayer->HasFinishedQuest(pMapInfo->required_quest))
		return AREA_TRIGGER_FAILURE_NO_ATTUNE_Q;

	if(pMapInfo && pMapInfo->required_item && !pPlayer->GetItemInterface()->GetItemCount(pMapInfo->required_item, true))
		return AREA_TRIGGER_FAILURE_NO_ATTUNE_I;

	if (pPlayer->iInstanceType >= MODE_HEROIC && 
		pMapInfo->type == INSTANCE_MULTIMODE && 
		!pPlayer->GetItemInterface()->GetItemCount(pMapInfo->heroic_key_1, false) && 
		!pPlayer->GetItemInterface()->GetItemCount(pMapInfo->heroic_key_2, false))
		return AREA_TRIGGER_FAILURE_NO_KEY;

	if(pPlayer->getLevel()<70 && pPlayer->iInstanceType>=MODE_HEROIC && pMapInfo->type != INSTANCE_NULL)
		return AREA_TRIGGER_FAILURE_LEVEL_HEROIC;

	return AREA_TRIGGER_FAILURE_OK;
}

void WorldSession::_HandleAreaTriggerOpcode(uint32 id)
{		
	sLog.outDebug("AreaTrigger: %u", id);

	// Are we REALLY here?
	if( !_player->IsInWorld() )
		return;

    // Search quest log, find any exploration quests
	sQuestMgr.OnPlayerExploreArea(GetPlayer(),id);

	AreaTrigger* pAreaTrigger = AreaTriggerStorage.LookupEntry( id );

	if( pAreaTrigger == NULL )
	{
		sLog.outDebug("Missing AreaTrigger: %u", id);
		return;
	}

	sHookInterface.OnAreaTrigger(GetPlayer(), id);

	if( _player->GetSession()->CanUseCommand('z') )
		sChatHandler.BlueSystemMessage( this, "[%sSystem%s] |rEntered areatrigger: %s%u (%s).", MSG_COLOR_WHITE, MSG_COLOR_LIGHTBLUE, MSG_COLOR_SUBWHITE, id, pAreaTrigger->Name );

	// if in BG handle is triggers
	if( _player->m_bg )
	{
		_player->m_bg->HookOnAreaTrigger(_player, id);
		return;
	}

	// Hook for Scripted Areatriggers
	_player->GetMapMgr()->HookOnAreaTrigger(_player, id);

	switch(pAreaTrigger->Type)
	{
	case ATTYPE_INSTANCE:
		{
			HandleInstanceStyleAreaTrigger(this, pAreaTrigger);
		}break;
	case ATTYPE_QUESTTRIGGER:
		{

		}break;
	case ATTYPE_INN:
		{
			// Inn
			if (!GetPlayer()->m_isResting) GetPlayer()->ApplyPlayerRestState(true);
		}break;
	case ATTYPE_TELEPORT:
		{
			MapInfo * destination_info = WorldMapInfoStorage.LookupEntry(pAreaTrigger->Mapid);
			if(destination_info && destination_info->type != INSTANCE_NULL)
			{
				// Some imported databases mark raid and dungeon portals as plain teleports.
				// Treat those as instance portals so raid/group/attunement checks stay consistent.
				HandleInstanceStyleAreaTrigger(this, pAreaTrigger);
			}
			else if(GetPlayer()->GetPlayerStatus() != TRANSFER_PENDING) //only ports if player is out of pendings
			{
				GetPlayer()->SaveEntryPoint(pAreaTrigger->Mapid);
				GetPlayer()->SafeTeleport(pAreaTrigger->Mapid, 0, LocationVector(pAreaTrigger->x, pAreaTrigger->y, pAreaTrigger->z, pAreaTrigger->o));
			}
		}break;
	default:break;
	}
}
