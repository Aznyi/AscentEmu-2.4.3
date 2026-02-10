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
#include <math.h>
#define SWIMMING_TOLERANCE_LEVEL -0.08f
#define MOVEMENT_PACKET_TIME_DELAY 500

static inline void CalcPassengerWorldPos(
	float tX, float tY, float tZ, float tO,
	float offX, float offY, float offZ,
	float &outX, float &outY, float &outZ)
{
	float c = cosf(tO);
	float s = sinf(tO);

	float rx = offX * c - offY * s;
	float ry = offX * s + offY * c;

	outX = tX + rx;
	outY = tY + ry;
	outZ = tZ + offZ;
}

#ifdef WIN32

#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#define DELTA_EPOCH_IN_USEC  11644473600000000ULL

uint32 TimeStamp()
{
	//return timeGetTime();

	FILETIME ft;
	uint64 t;
	GetSystemTimeAsFileTime(&ft);

	t = (uint64)ft.dwHighDateTime << 32;
	t |= ft.dwLowDateTime;
	t /= 10;
	t -= DELTA_EPOCH_IN_USEC;

	return uint32(((t / 1000000L) * 1000) + ((t % 1000000L) / 1000));
}

ASCENT_INLINE uint32 mTimeStamp()
{
	return timeGetTime();
}

#else

uint32 TimeStamp()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}

ASCENT_INLINE uint32 mTimeStamp()
{
	struct timeval tp;
	gettimeofday(&tp, NULL);
	return (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}

#endif

void WorldSession::HandleMoveWorldportAckOpcode( WorldPacket & recv_data )
{
	GetPlayer()->SetPlayerStatus(NONE);
	if(_player->IsInWorld())
	{
		// get outta here
		return;
	}
	sLog.outDebug( "WORLD: got MSG_MOVE_WORLDPORT_ACK." );
	
	if(_player->m_CurrentTransporter && _player->GetMapId() != _player->m_CurrentTransporter->GetMapId())
	{
		/* wow, our pc must really suck. */
		Transporter * pTrans = _player->m_CurrentTransporter;
		float c_tposx, c_tposy, c_tposz;
		CalcPassengerWorldPos(pTrans->GetPositionX(), pTrans->GetPositionY(), pTrans->GetPositionZ(), pTrans->GetOrientation(),
			_player->m_TransporterX, _player->m_TransporterY, _player->m_TransporterZ,
			c_tposx, c_tposy, c_tposz);

		WorldPacket dataw(SMSG_NEW_WORLD, 20);
		dataw << pTrans->GetMapId() << c_tposx << c_tposy << c_tposz << _player->GetOrientation();
		SendPacket(&dataw);
	}
	else
	{
		_player->m_TeleportState = 2;
		_player->AddToWorld();
	}
}

void WorldSession::HandleMoveTeleportAckOpcode( WorldPacket & recv_data )
{
	uint64 guid;
	recv_data >> guid;
	if(guid == _player->GetGUID())
	{
		if(sWorld.antihack_teleport && !(HasGMPermissions() && sWorld.no_antihack_on_gm) && _player->GetPlayerStatus() != TRANSFER_PENDING)
		{
			/* we're obviously cheating */
			sCheatLog.writefromsession(this, "Used teleport hack, disconnecting.");
			Disconnect();
			return;
		}

		if(sWorld.antihack_teleport && !(HasGMPermissions() && sWorld.no_antihack_on_gm) && _player->m_position.Distance2DSq(_player->m_sentTeleportPosition) > 625.0f)	/* 25.0f*25.0f */
		{
			/* cheating.... :( */
			sCheatLog.writefromsession(this, "Used teleport hack {2}, disconnecting.");
			Disconnect();
			return;
		}

		sLog.outDebug( "WORLD: got MSG_MOVE_TELEPORT_ACK." );
		GetPlayer()->SetPlayerStatus(NONE);
		if( GetPlayer()->m_rooted <= 0 )
			GetPlayer()->SetMovement(MOVE_UNROOT,5);
		_player->SpeedCheatReset();

		if(GetPlayer()->GetSummon() != NULL)		// move pet too
			GetPlayer()->GetSummon()->SetPosition((GetPlayer()->GetPositionX() + 2), (GetPlayer()->GetPositionY() + 2), GetPlayer()->GetPositionZ(), float(M_PI));
		if(_player->m_sentTeleportPosition.x != 999999.0f)
		{
			_player->m_position = _player->m_sentTeleportPosition;
			_player->m_sentTeleportPosition.ChangeCoords(999999.0f,999999.0f,999999.0f);
		}
	}

}

void _HandleBreathing(MovementInfo &movement_info, Player * _player, WorldSession * pSession)
{

	// no water breathing is required
	if( !sWorld.BreathingEnabled || _player->FlyCheat || _player->m_bUnlimitedBreath || !_player->isAlive() || _player->GodModeCheat )
	{
		// player is flagged as in water
		if( _player->m_UnderwaterState & UNDERWATERSTATE_SWIMMING  )
			_player->m_UnderwaterState &= ~UNDERWATERSTATE_SWIMMING;

		// player is flagged as under water
		if( _player->m_UnderwaterState & UNDERWATERSTATE_UNDERWATER )
		{
			_player->m_UnderwaterState &= ~UNDERWATERSTATE_UNDERWATER;
			WorldPacket data(SMSG_START_MIRROR_TIMER, 20);
			data << uint32(1) << _player->m_UnderwaterTime << _player->m_UnderwaterMaxTime << int32(-1) << uint32(0);
			pSession->SendPacket(&data);
		}

		// player is above water level
		if( pSession->m_bIsWLevelSet )
		{
			if( ( movement_info.z + _player->m_noseLevel ) > pSession->m_wLevel )
			{
				// remove druid aquatic form on land
				if( _player->getClass() == DRUID )
					_player->RemoveAura( 1066 );

				// unset swim session water level
				pSession->m_bIsWLevelSet = false;
			}
		}

		return;
	}

	//player is swiming and not flagged as in the water
	if( movement_info.flags & MOVEFLAG_SWIMMING && !( _player->m_UnderwaterState & UNDERWATERSTATE_SWIMMING ) )
	{
		// dismount if mounted
		if( _player->m_MountSpellId )
			_player->RemoveAura( _player->m_MountSpellId );

		// get water level only if it was not set before
		if( !pSession->m_bIsWLevelSet )
		{
			// water level is somewhere below the nose of the character when entering water
			pSession->m_wLevel = movement_info.z + _player->m_noseLevel * 0.95f;
			pSession->m_bIsWLevelSet = true;
		}

		_player->m_UnderwaterState |= UNDERWATERSTATE_SWIMMING;
	}

	// player is not swimming and is not stationary and is flagged as in the water
	if( !( movement_info.flags & MOVEFLAG_SWIMMING ) && !( movement_info.flags & MOVEFLAG_MOVE_STOP ) && _player->m_UnderwaterState & UNDERWATERSTATE_SWIMMING )
	{
		// player is above water level
		if( ( movement_info.z + _player->m_noseLevel ) > pSession->m_wLevel )
		{
			// remove druid aquatic form on land
			if( _player->getClass() == DRUID )
				_player->RemoveAura( 1066 );

			// unset swim session water level
			pSession->m_bIsWLevelSet = false;

			_player->m_UnderwaterState &= ~UNDERWATERSTATE_SWIMMING;
		}
	}

	// player is flagged as in the water and is not flagged as under the water
	if( _player->m_UnderwaterState & UNDERWATERSTATE_SWIMMING && !( _player->m_UnderwaterState & UNDERWATERSTATE_UNDERWATER ) )
	{
		//the player is in the water and has gone under water, requires breath bar.
		if( ( movement_info.z + _player->m_noseLevel ) < pSession->m_wLevel )
		{
			_player->m_UnderwaterState |= UNDERWATERSTATE_UNDERWATER;
			WorldPacket data(SMSG_START_MIRROR_TIMER, 20);
			data << uint32(1) << _player->m_UnderwaterTime << _player->m_UnderwaterMaxTime << int32(-1) << uint32(0);
			pSession->SendPacket(&data);
		}
	}

	// player is flagged as in the water and is flagged as under the water
	if( _player->m_UnderwaterState & UNDERWATERSTATE_SWIMMING && _player->m_UnderwaterState & UNDERWATERSTATE_UNDERWATER )
	{
		//the player is in the water but their face is above water, no breath bar neeeded.
		if( ( movement_info.z + _player->m_noseLevel ) > pSession->m_wLevel )
		{
			_player->m_UnderwaterState &= ~UNDERWATERSTATE_UNDERWATER;
			WorldPacket data(SMSG_START_MIRROR_TIMER, 20);
			data << uint32(1) << _player->m_UnderwaterTime << _player->m_UnderwaterMaxTime << int32(10) << uint32(0);
			pSession->SendPacket(&data);
		}
	}

	// player is flagged as not in the water and is flagged as under the water
	if( !( _player->m_UnderwaterState & UNDERWATERSTATE_SWIMMING ) && _player->m_UnderwaterState & UNDERWATERSTATE_UNDERWATER )
	{
		//the player is out of the water, no breath bar neeeded.
		if( ( movement_info.z + _player->m_noseLevel ) > pSession->m_wLevel )
		{
			_player->m_UnderwaterState &= ~UNDERWATERSTATE_UNDERWATER;
			WorldPacket data(SMSG_START_MIRROR_TIMER, 20);
			data << uint32(1) << _player->m_UnderwaterTime << _player->m_UnderwaterMaxTime << int32(10) << uint32(0);
			pSession->SendPacket(&data);
		}
	}

}

void WorldSession::HandleMovementOpcodes( WorldPacket & recv_data )
{
	if(!_player->IsInWorld() || _player->m_uint32Values[UNIT_FIELD_CHARMEDBY] || _player->GetPlayerStatus() == TRANSFER_PENDING || _player->GetTaxiState())
		return;

	// spell cancel on movement, for now only fishing is added
	Object * t_go = _player->m_SummonedObject;
	if (t_go)
	{
		if (t_go->GetEntry() == GO_FISHING_BOBBER)
			((GameObject*)t_go)->EndFishing(GetPlayer(),true);
	}

	/************************************************************************/
	/* Clear standing state to stand.				                        */
	/************************************************************************/
	if( recv_data.GetOpcode() == MSG_MOVE_START_FORWARD )
		_player->SetStandState( STANDSTATE_STAND );
	/************************************************************************/
	/* Make sure the packet is the correct size range.                      */
	/************************************************************************/
	if(recv_data.size() > 80) { Disconnect(); return; }

	/************************************************************************/
	/* Read Movement Data Packet                                            */
	/************************************************************************/
	movement_info.init(recv_data);

	/************************************************************************/
	/* Update player movement state                                         */
	/************************************************************************/

	// update the detector
	// NOTE: When a player is on a transport (boat/zeppelin/elevator), the client still sends world-space
	// movement updates that include the transport's motion. Feeding those samples into the
	// speedhack detector produces false-positives (especially on turns/acceleration), which
	// can lead to forced logout.
	if( sWorld.antihack_speed && !_player->GetTaxiState() && !_player->m_TransporterGUID && !movement_info.transGuid )
	{
		// simplified: just take the fastest speed. less chance of fuckups too
		float speed = ( _player->flying_aura ) ? _player->m_flySpeed : ( _player->m_swimSpeed >_player-> m_runSpeed ) ? _player->m_swimSpeed : _player->m_runSpeed;

		_player->SDetector->AddSample( movement_info.x, movement_info.y, getMSTime(), speed );

		if( _player->SDetector->IsCheatDetected() )
			_player->SDetector->ReportCheater( _player );
	}

	/************************************************************************/
	/* Remove Emote State                                                   */
	/************************************************************************/
	if(_player->m_uint32Values[UNIT_NPC_EMOTESTATE])
		_player->SetUInt32Value(UNIT_NPC_EMOTESTATE,0);

	/************************************************************************/
	/* Make sure the co-ordinates are valid.                                */
	/************************************************************************/
	if( !((movement_info.y >= _minY) && (movement_info.y <= _maxY)) || !((movement_info.x >= _minX) && (movement_info.x <= _maxX)) )
	{
		Disconnect();
		return;
	}

	/************************************************************************/
	/* Dump movement flags - Wheee!                                         */
	/************************************************************************/
#if 0
	printf("=========================================================\n");
	printf("Full movement flags: 0x%.8X\n", movement_info.flags);
	uint32 z, b;
	for(z = 1, b = 1; b < 32;)
	{
		if(movement_info.flags & z)
			printf("   Bit %u (0x%.8X or %u) is set!\n", b, z, z);

		z <<= 1;
		b+=1;
	}
	printf("=========================================================\n");
#endif

	/************************************************************************/
	/* Orientation dumping                                                  */
	/************************************************************************/
#if 0
	printf("Packet: 0x%03X (%s)\n", recv_data.GetOpcode(), LookupName( recv_data.GetOpcode(), g_worldOpcodeNames ) );
	printf("Orientation: %.10f\n", movement_info.orientation);
#endif

	/************************************************************************/
	/* Calculate the timestamp of the packet we have to send out            */
	/************************************************************************/
	size_t pos = (size_t)m_MoverWoWGuid.GetNewGuidLen() + 1;
	uint32 mstime = mTimeStamp();
	int32 move_time;
	if(m_clientTimeDelay == 0)
		m_clientTimeDelay = mstime - movement_info.time;

	/************************************************************************/
	/* Copy into the output buffer.                                         */
	/************************************************************************/
	if(_player->m_inRangePlayers.size())
	{
		move_time = (movement_info.time - (mstime - m_clientTimeDelay)) + MOVEMENT_PACKET_TIME_DELAY + mstime;
		memcpy(&movement_packet[pos], recv_data.contents(), recv_data.size());
		movement_packet[pos+5]=0;

		/************************************************************************/
		/* Distribute to all inrange players.                                   */
		/************************************************************************/
		for(set<Player*>::iterator itr = _player->m_inRangePlayers.begin(); itr != _player->m_inRangePlayers.end(); ++itr)
		{
#ifdef USING_BIG_ENDIAN
			*(uint32*)&movement_packet[pos+5] = swap32(move_time + (*itr)->GetSession()->m_moveDelayTime);
#else
			*(uint32*)&movement_packet[pos+5] = uint32(move_time + (*itr)->GetSession()->m_moveDelayTime);
#endif
#if defined(ENABLE_COMPRESSED_MOVEMENT) && defined(ENABLE_COMPRESSED_MOVEMENT_FOR_PLAYERS)
			if( _player->GetPositionNC().Distance2DSq((*itr)->GetPosition()) >= World::m_movementCompressThreshold )
				(*itr)->AppendMovementData( recv_data.GetOpcode(), uint16(recv_data.size() + pos), movement_packet );
			else
				(*itr)->GetSession()->OutPacket(recv_data.GetOpcode(), uint16(recv_data.size() + pos), movement_packet);
#else
			(*itr)->GetSession()->OutPacket(recv_data.GetOpcode(), uint16(recv_data.size() + pos), movement_packet);			
#endif
		}
	}

	/************************************************************************/
	/* Falling damage checks                                                */
	/************************************************************************/

	if( _player->blinked )
	{
		_player->blinked = false;
		_player->m_fallDisabledUntil = UNIXTIME + 5;
		_player->SpeedCheatDelay( 1000 ); //some say they managed to trigger system with knockback. Maybe they moved in air ?
	}
	else
	{
		if( recv_data.GetOpcode() == MSG_MOVE_FALL_LAND )
		{
			// player has finished falling
			//if _player->z_axisposition contains no data then set to current position
			if( !_player->z_axisposition )
				_player->z_axisposition = movement_info.z;

			// calculate distance fallen
			uint32 falldistance = float2int32( _player->z_axisposition - movement_info.z );

			/*if player is a rogue or druid(in cat form), then apply -17 modifier to fall distance.
			these checks need improving, low level rogue/druid should not receive this benefit*/
			if( ( _player->getClass() == ROGUE ) || ( _player->GetShapeShift() == FORM_CAT ) )
			{
				if( falldistance > 17 ) 
					falldistance -=17;
				else
					falldistance = 1;
			}

			//checks that player has fallen more than 12 units, otherwise no damage will be dealt
			//falltime check is also needed here, otherwise sudden changes in Z axis position, such as using !recall, may result in death			
			if( _player->isAlive() && !_player->GodModeCheat && falldistance > 12 && ( UNIXTIME >= _player->m_fallDisabledUntil ) && movement_info.FallTime > 2000 )
			{
				// 1.7% damage for each unit fallen on Z axis over 13
				uint32 health_loss = float2int32( float( _player->GetUInt32Value( UNIT_FIELD_MAXHEALTH ) * ( ( falldistance - 12 ) * 0.017 ) ) );
													
				if( health_loss >= _player->GetUInt32Value( UNIT_FIELD_HEALTH ) )
					health_loss = _player->GetUInt32Value( UNIT_FIELD_HEALTH );

				_player->SendEnvironmentalDamageLog( _player->GetGUID(), DAMAGE_FALL, health_loss );
				_player->DealDamage( _player, health_loss, 0, 0, 0 );

				//_player->RemoveStealth(); // ceberwow : why again? lost stealth by AURA_INTERRUPT_ON_ANY_DAMAGE_TAKEN already. 
			}
			_player->z_axisposition = 0.0f;
		}
		else
			//whilst player is not falling, continuosly update Z axis position.
			//once player lands this will be used to determine how far he fell.
			if( !( movement_info.flags & MOVEFLAG_FALLING ) )
				_player->z_axisposition = movement_info.z;
	}

	/************************************************************************/
	/* Transporter Setup                                                    */
	/************************************************************************/
	if(!_player->m_lockTransportVariables)
	{
		if(_player->m_TransporterGUID && !movement_info.transGuid)
		{
			// The client can temporarily omit transGuid while still on a transport (notably Deeprun Tram),
			// especially during turns/cell transitions/loading jitter. Detaching immediately causes
			// the server to think the player is no longer on the transport -> player falls through.
			uint32 now = getMSTime();

			if(_player->m_transportLostTime == 0)
				_player->m_transportLostTime = now;

			bool should_detach = false;

			// If we still know the transporter object, only detach immediately if the player is clearly far away.
			if(_player->m_CurrentTransporter)
			{
				float dx = _player->GetPositionX() - _player->m_CurrentTransporter->GetPositionX();
				float dy = _player->GetPositionY() - _player->m_CurrentTransporter->GetPositionY();
				float dist2 = dx*dx + dy*dy;
				if(dist2 > (60.0f * 60.0f)) // ~60 yards => definitely not on it
					should_detach = true;
			}

			// Otherwise, only detach if missing transGuid persists for a short grace period.
			if (!should_detach && _player->m_transportLostTime && (now - _player->m_transportLostTime) > 2000)
				should_detach = true;

			if(should_detach)
			{
				/* we left the transporter we were on */
				if(_player->m_CurrentTransporter)
				{
					_player->m_CurrentTransporter->RemovePlayer(_player);
					_player->m_CurrentTransporter = NULL;
				}

				_player->m_TransporterGUID = 0;
				_player->m_transportLostTime = 0;
				_player->SpeedCheatReset();
				_player->SpeedCheatDelay(5000);
			}
		}
		else if(movement_info.transGuid)
		{
			// We have valid transport data again; clear any pending lost timer.
			_player->m_transportLostTime = 0;

			// Always keep m_CurrentTransporter pointer in sync. It can be NULL after relog/map change/cell churn
			// even while m_TransporterGUID remains set, which causes us to fall back to raw world coords and fall through.
			Transporter* newTrans = objmgr.GetTransporter(GUID_LOPART(movement_info.transGuid));
			if(newTrans && newTrans != _player->m_CurrentTransporter)
			{
				if(_player->m_CurrentTransporter)
					_player->m_CurrentTransporter->RemovePlayer(_player);

				_player->m_CurrentTransporter = newTrans;
				_player->m_CurrentTransporter->AddPlayer(_player);
			}

			if(!_player->m_TransporterGUID)
			{
				/* just walked into a transport */
				if(_player->IsMounted())
					_player->RemoveAura(_player->m_MountSpellId);

				/* set variables */
				_player->m_TransporterGUID = movement_info.transGuid;
				_player->m_TransporterUnk = movement_info.transUnk;
				_player->m_TransporterX = movement_info.transX;
				_player->m_TransporterY = movement_info.transY;
				_player->m_TransporterZ = movement_info.transZ;
				_player->m_TransporterO = movement_info.transO;
				_player->SpeedCheatDelay(5000);
			}
			else
			{
				/* no changes */
				_player->m_TransporterUnk = movement_info.transUnk;
				_player->m_TransporterX = movement_info.transX;
				_player->m_TransporterY = movement_info.transY;
				_player->m_TransporterZ = movement_info.transZ;
				_player->m_TransporterO = movement_info.transO;
			}
		}
	}

	/************************************************************************/
	/* Anti-Speed Hack Checks                                               */
	/************************************************************************/

	

	/************************************************************************/
	/* Breathing System                                                     */
	/************************************************************************/
	_HandleBreathing(movement_info, _player, this);

	/************************************************************************/
	/* Remove Spells                                                        */
	/************************************************************************/
	_player->RemoveAurasByInterruptFlag(AURA_INTERRUPT_ON_MOVEMENT);

	/************************************************************************/
	/* Update our position in the server.                                   */
	/************************************************************************/
	if( _player->m_CurrentCharm && _player->GetMapMgr() )
	{
		Unit *cc = _player->GetMapMgr()->GetUnit( _player->m_CurrentCharm );
		if( cc )
			cc->SetPosition(movement_info.x, movement_info.y, movement_info.z, movement_info.orientation);
	}
	else
	{
		// Guard: if client reports being on a transport but transporter pointer
		// cannot be resolved, do NOT apply raw world-space movement.
		bool skip_world_update = (!_player->m_CurrentTransporter && movement_info.transGuid);

		if(!skip_world_update && !_player->m_CurrentTransporter)
		{
			if( !_player->SetPosition(movement_info.x, movement_info.y, movement_info.z, movement_info.orientation) )
			{
				_player->SetUInt32Value(UNIT_FIELD_HEALTH, 0);
				_player->KillPlayer();
			}
		}
		else if (!skip_world_update)
		{
			float wx, wy, wz;
			CalcPassengerWorldPos(
				_player->m_CurrentTransporter->GetPositionX(),
				_player->m_CurrentTransporter->GetPositionY(),
				_player->m_CurrentTransporter->GetPositionZ(),
				// Use client-reported transport orientation for best sync on turns (Deeprun Tram).
				// The server-side transporter orientation can drift slightly vs the client during rotation,
				// causing passenger drift and occasional fall-off.
				_player->m_TransporterO,
				_player->m_TransporterX, _player->m_TransporterY, _player->m_TransporterZ,
				wx, wy, wz);
			_player->SetPosition(wx, wy, wz, movement_info.orientation + movement_info.transO, false);

			// Deeprun Tram + transports move passengers in small increments.
			// Player::SetPosition only triggers relocation after a threshold; force it so in-range/cell sets stay correct.
			if(_player->IsInWorld() && _player->GetMapMgr())
				_player->GetMapMgr()->ChangeObjectLocation(_player);
		}
	}	
}

void WorldSession::HandleMoveTimeSkippedOpcode( WorldPacket & recv_data )
{
	
}

void WorldSession::HandleMoveNotActiveMoverOpcode( WorldPacket & recv_data )
{

}


void WorldSession::HandleSetActiveMoverOpcode( WorldPacket & recv_data )
{
	// set current movement object
	uint64 guid;
	recv_data >> guid;

	if(guid != m_MoverWoWGuid.GetOldGuid())
	{
		// make sure the guid is valid and we aren't cheating
		if( !(_player->m_CurrentCharm == guid) &&
			!(_player->GetGUID() == guid) )
		{
			// cheater!
			return;
		}

		// generate wowguid
		if(guid != 0)
			m_MoverWoWGuid.Init(guid);
		else
			m_MoverWoWGuid.Init(_player->GetGUID());

		// set up to the movement packet
		movement_packet[0] = m_MoverWoWGuid.GetNewGuidMask();
		memcpy(&movement_packet[1], m_MoverWoWGuid.GetNewGuid(), m_MoverWoWGuid.GetNewGuidLen());
	}
}

void WorldSession::HandleMoveSplineCompleteOpcode(WorldPacket &recvPacket)
{

}

void WorldSession::HandleMountSpecialAnimOpcode(WorldPacket &recvdata)
{
	WorldPacket data(SMSG_MOUNTSPECIAL_ANIM,8);
	data << _player->GetGUID();
	_player->SendMessageToSet(&data, true);
}

void WorldSession::HandleWorldportOpcode(WorldPacket & recv_data)
{
	uint32 unk;
	uint32 mapid;
	float x,y,z,o;
	recv_data >> unk >> mapid >> x >> y >> z >> o;

	//printf("\nTEST: %u %f %f %f %f", mapid, x, y, z, o);
	
	if(!_player->IsInWorld())
		return;

	if(!HasGMPermissions())
	{
		SendNotification("You do not have permission to use this function.");
		return;
	}

	LocationVector vec(x,y,z,o);
	_player->SafeTeleport(mapid,0,vec);
}

void WorldSession::HandleTeleportToUnitOpcode(WorldPacket & recv_data)
{
	uint8 unk;
	Unit * target;
	recv_data >> unk;

	if(!_player->IsInWorld())
		return;

	if(!HasGMPermissions())
	{
		SendNotification("You do not have permission to use this function.");
		return;
	}

	if( (target = _player->GetMapMgr()->GetUnit(_player->GetSelection())) == NULL )
		return;

	_player->SafeTeleport(_player->GetMapId(), _player->GetInstanceID(), target->GetPosition());
}

void WorldSession::HandleTeleportCheatOpcode(WorldPacket & recv_data)
{
	float x,y,z,o;
	LocationVector vec;

	if(!HasGMPermissions())
	{
		SendNotification("You do not have permission to use this function.");
		return;
	}

	recv_data >> x >> y >> z >> o;
	vec.ChangeCoords(x,y,z,o);
	_player->SafeTeleport(_player->GetMapId(),_player->GetInstanceID(),vec);
}

void MovementInfo::init(WorldPacket & data)
{
	transGuid = 0;
	unk13 = 0;
	data >> flags >> unk_230 >> time;
	data >> x >> y >> z >> orientation;

	if (flags & MOVEFLAG_TAXI)
	{
		data >> transGuid >> transX >> transY >> transZ >> transO >> transUnk;
	}
	if (flags & MOVEFLAG_SWIMMING)
	{
		data >> unk6;
	}
	if (flags & MOVEFLAG_FALLING || flags & MOVEFLAG_REDIRECTED)
	{
		data >> FallTime >> unk8 >> unk9 >> unk10;
	}
	if (flags & MOVEFLAG_SPLINE_MOVER)
	{
		data >> unk12;
	}

	data >> unklast;
	if(data.rpos() != data.wpos())
	{
		if(data.rpos() + 4 == data.wpos())
			data >> unk13;
		else
			sLog.outDebug("Extra bits of movement packet left");
	}
}

void MovementInfo::write(WorldPacket & data)
{
	data << flags << unk_230 << getMSTime();

	data << x << y << z << orientation;

	if (flags & MOVEFLAG_TAXI)
	{
		data << transGuid << transX << transY << transZ << transO << transUnk;
	}
	if (flags & MOVEFLAG_SWIMMING)
	{
		data << unk6;
	}
	if (flags & MOVEFLAG_FALLING)
	{
		data << FallTime << unk8 << unk9 << unk10;
	}
	if (flags & MOVEFLAG_SPLINE_MOVER)
	{
		data << unk12;
	}
	data << unklast;
	if(unk13)
		data << unk13;
}
