//
// Soulstone Emu (C) 2017
// Transport System
//

#include "StdAfx.h"

Mutex m_transportGuidGen;
uint32 m_transportGuidMax = 50;


Transporter::Transporter(uint64 guid) : GameObject(guid)
{}

Transporter::~Transporter()
{}

void Transporter::OnPushToWorld()
{
	// Create waypoint event
	sEventMgr.AddEvent(this, &Transporter::UpdatePosition, EVENT_TRANSPORTER_NEXT_WAYPOINT, 100, 0, EVENT_FLAG_DO_NOT_EXECUTE_IN_WORLD_CONTEXT);
}

// Pull DBC Values to fill our path.
bool FillPathVector(uint32 PathID, TransportPath & Path)
{
	// Store dbc values into current Path array
	Path.Resize(dbcTaxiPathNode.GetNumRows());

	uint32 i = 0;
	for (uint32 j = 0; j < dbcTaxiPathNode.GetNumRows(); ++j)
	{
		auto pathnode = dbcTaxiPathNode.LookupEntry(j);
		if (pathnode == nullptr)
			continue;

		if (pathnode->path == PathID)
		{
			Path[i].mapid = pathnode->mapid;
			Path[i].x = pathnode->x;
			Path[i].y = pathnode->y;
			Path[i].z = pathnode->z;
			Path[i].actionFlag = pathnode->actionFlag;
			Path[i].delay = pathnode->waittime;
			++i;
		}
	}

	Path.Resize(i);
	return (i > 0 ? true : false);
}

//
// Spawn & Assign waypoints.
//
bool Transporter::CreateAsTransporter(uint32 EntryID, const char* Name, int32 Time)
{
	// Lookup GameobjectInfo
	if (!CreateFromProto(EntryID, 0, 0, 0, 0, 0))
		return false;

	// Manual Overrides
	SetUInt32Value(GAMEOBJECT_FLAGS,40);
	SetUInt32Value(GAMEOBJECT_ANIMPROGRESS, 100);

	// Set period
	m_period = Time;

	// Generate waypoints
	if (!GenerateWaypoints())
	{
		return false;
	}

	// Set position
	SetMapId(m_WayPoints[0].mapid);
	SetPosition(m_WayPoints[0].x, m_WayPoints[0].y, m_WayPoints[0].z, 0);
	
	// Add to world
	AddToWorld();

	return true;
}


//
// Generate the path for movement.
// Note: Objects don't move.. the player moves.
//
bool Transporter::GenerateWaypoints()
{
	TransportPath path;
	FillPathVector(GetInfo()->SpellFocus, path);

	if(path.Size() == 0) 
		return false;

	std::vector<keyFrame> keyFrames;
	int mapChange = 0;
	for (int i = 1; i < (int)path.Size() - 1; i++)
	{
		if (mapChange == 0)
		{
			if ((path[i].mapid == path[i + 1].mapid))
			{
				keyFrame k(path[i].x, path[i].y, path[i].z, path[i].mapid, path[i].actionFlag, path[i].delay);
				keyFrames.push_back(k);
			}
			else
			{
				mapChange = 1;
			}
		}
		else
		{
			mapChange--;
		}
	}

	int lastStop = -1;
	int firstStop = -1;

	// first cell is arrived at by teleportation :S
	keyFrames[0].distFromPrev = 0;

	if (keyFrames[0].actionflag == 2)
	{
		lastStop = 0;
	}

	// find the rest of the distances between key points
	for (size_t i = 1; i < keyFrames.size(); i++)
	{
		if ((keyFrames[i].actionflag == 1) || (keyFrames[i].mapid != keyFrames[i - 1].mapid))
		{
			keyFrames[i].distFromPrev = 0;
		}
		else
		{
			keyFrames[i].distFromPrev =
				sqrt(pow(keyFrames[i].x - keyFrames[i - 1].x, 2) +
				pow(keyFrames[i].y - keyFrames[i - 1].y, 2) +
				pow(keyFrames[i].z - keyFrames[i - 1].z, 2));
		}
		if (keyFrames[i].actionflag == 2)
		{
			if (firstStop < 0)
				firstStop = (int)i;

			lastStop = (int)i;
		}
	}

	float tmpDist = 0;

	for (size_t i = 0; i < keyFrames.size(); i++)
	{
		int j = int((i + lastStop) % keyFrames.size());

		if (keyFrames[j].actionflag == 2)
		{
			tmpDist = 0;
		}
		else
		{
			tmpDist += keyFrames[j].distFromPrev;
		}

		keyFrames[j].distSinceStop = tmpDist;
	}
	for (int i = int(keyFrames.size()) - 1; i >= 0; i--)
	{
		int j = (i + (firstStop + 1)) % keyFrames.size();
		tmpDist += keyFrames[(j + 1) % keyFrames.size()].distFromPrev;
		keyFrames[j].distUntilStop = tmpDist;

		if (keyFrames[j].actionflag == 2)
		{
			tmpDist = 0;
		}
	}

	for (size_t i = 0; i < keyFrames.size(); i++)
	{
		if (keyFrames[i].distSinceStop < (30 * 30 * 0.5))
		{
			keyFrames[i].tFrom = sqrt(2 * keyFrames[i].distSinceStop);
		}

		else
		{
			keyFrames[i].tFrom = ((keyFrames[i].distSinceStop - (30 * 30 * 0.5f)) / 30) + 30;
		}

		if (keyFrames[i].distUntilStop < (30 * 30 * 0.5))
		{
			keyFrames[i].tTo = sqrt(2 * keyFrames[i].distUntilStop);
		}

		else
		{
			keyFrames[i].tTo = ((keyFrames[i].distUntilStop - (30 * 30 * 0.5f)) / 30) + 30;
		}

		keyFrames[i].tFrom *= 1000;
		keyFrames[i].tTo *= 1000;
	}

	// Now we're completely set up; we can move along the length of each waypoint at 100 ms intervals
	// speed = max(30, t) (remember x = 0.5s^2, and when accelerating, a = 1 unit/s^2
	int t = 0;
	bool teleport = false;


	if (keyFrames[keyFrames.size() - 1].mapid != keyFrames[0].mapid)
	{
		teleport = true;
	}

	TWayPoint pos(keyFrames[0].mapid, keyFrames[0].x, keyFrames[0].y, keyFrames[0].z, teleport);
	m_WayPoints[0] = pos;
	t += keyFrames[0].delay * 1000;

	uint32 cM = keyFrames[0].mapid;

	for (size_t i = 0; i < keyFrames.size() - 1; i++)	   //
	{
		float d = 0;
		float tFrom = keyFrames[i].tFrom;
		float tTo = keyFrames[i].tTo;

		// keep the generation of all these points; we use only a few now, but may need the others later
		if (((d < keyFrames[i + 1].distFromPrev) && (tTo > 0)))
		{
			while ((d < keyFrames[i + 1].distFromPrev) && (tTo > 0))
			{
				tFrom += 100;
				tTo -= 100;

				if (d > 0)
				{
					float newX, newY, newZ;
					newX = keyFrames[i].x + (keyFrames[i + 1].x - keyFrames[i].x) * d / keyFrames[i + 1].distFromPrev;
					newY = keyFrames[i].y + (keyFrames[i + 1].y - keyFrames[i].y) * d / keyFrames[i + 1].distFromPrev;
					newZ = keyFrames[i].z + (keyFrames[i + 1].z - keyFrames[i].z) * d / keyFrames[i + 1].distFromPrev;

					bool teleport = false;
					if ((int)keyFrames[i].mapid != cM)
					{
						teleport = true;
						cM = keyFrames[i].mapid;
					}

					TWayPoint pos(keyFrames[i].mapid, newX, newY, newZ, teleport);
					m_WayPoints[t] = pos;
				}

				// caught in tFrom dock's "gravitational pull"
				if (tFrom < tTo)							
				{
					if (tFrom <= 30000)
					{
						d = 0.5f * (tFrom / 1000) * (tFrom / 1000);
					}
					else
					{
						d = 0.5f * 30 * 30 + 30 * ((tFrom - 30000) / 1000);
					}
					d = d - keyFrames[i].distSinceStop;
				}
				else
				{
					if (tTo <= 30000)
					{
						d = 0.5f * (tTo / 1000) * (tTo / 1000);
					}
					else
					{
						d = 0.5f * 30 * 30 + 30 * ((tTo - 30000) / 1000);
					}
					d = keyFrames[i].distUntilStop - d;
				}
				t += 100;
			}
			t -= 100;
		}

		if (keyFrames[i + 1].tFrom > keyFrames[i + 1].tTo)
		{
			t += 100 - ((long)keyFrames[i + 1].tTo % 100);
		}

		else
		{
			t += (long)keyFrames[i + 1].tTo % 100;
		}

		teleport = false;

		if ((keyFrames[i + 1].actionflag == 1) || (keyFrames[i + 1].mapid != keyFrames[i].mapid))
		{
			teleport = true;
			cM = keyFrames[i + 1].mapid;
		}

		TWayPoint pos(keyFrames[i + 1].mapid, keyFrames[i + 1].x, keyFrames[i + 1].y, keyFrames[i + 1].z, teleport);

		if (keyFrames[i + 1].delay > 5)
		{
			pos.delayed = true;
		}

		m_WayPoints[t] = pos;

		t += keyFrames[i + 1].delay * 1000;
	}

	uint32 timer = t;

	mCurrentWaypoint = m_WayPoints.begin();
	mNextWaypoint = GetNextWaypoint();
	m_pathTime = timer;

	return true;
}

WaypointIterator Transporter::GetNextWaypoint()
{
	WaypointIterator iter = mCurrentWaypoint;
	iter++;
	if (iter == m_WayPoints.end())
		iter = m_WayPoints.begin();
	return iter;
}

bool Transporter::FindDockNear(uint32 mapid, float x, float y, float z, float maxDist, uint32& outDockTime, float& outDockDistSq)
{
    outDockTime = 0;
    outDockDistSq = 0.0f;

    if (maxDist <= 0.0f)
        return false;

    const float maxDistSq = maxDist * maxDist;

    bool found = false;
    float best = maxDistSq;

    for (WaypointIterator itr = m_WayPoints.begin(); itr != m_WayPoints.end(); ++itr)
    {
        if (!itr->second.delayed)
            continue;

        if (itr->second.mapid != mapid)
            continue;

        const float dx = itr->second.x - x;
        const float dy = itr->second.y - y;
        const float dz = itr->second.z - z;
        const float d2 = dx*dx + dy*dy + dz*dz;

        if (d2 <= best)
        {
            best = d2;
            outDockTime = itr->first;
            outDockDistSq = d2;
            found = true;
        }
    }

    return found;
}

//
// Update the player / objects position.
//
uint32 TimeStamp();
void Transporter::UpdatePosition()
{
	// Waypoint size check.
	if (m_WayPoints.size() <= 1)
		return;

	// Set the timer.
	m_timer = getMSTime() % m_period;

	// Advance through waypoints if our timer has passed them.
	while (((m_timer - mCurrentWaypoint->first) % m_pathTime) >= ((mNextWaypoint->first - mCurrentWaypoint->first) % m_pathTime))
	{
		mCurrentWaypoint = mNextWaypoint;
		mNextWaypoint = GetNextWaypoint();

		if (mCurrentWaypoint->second.mapid != GetMapId() || mCurrentWaypoint->second.teleport)
		{
			TransportPassengers(mCurrentWaypoint->second.mapid, GetMapId(), mCurrentWaypoint->second.x, mCurrentWaypoint->second.y, mCurrentWaypoint->second.z, mCurrentWaypoint->second.o);
			break;
		}
		else
		{
			SetPosition(mCurrentWaypoint->second.x, mCurrentWaypoint->second.y, mCurrentWaypoint->second.z, m_position.o, false);
		}

		// During the delay sounds play..
		if (mCurrentWaypoint->second.delayed)
		{
			// Pull by Display ID instead of entry?
			switch (GetInfo()->DisplayID)
			{
				case 3015:
				case 7087:
				{
					// ShipDocked (LightHouseFogHorn.wav)
					PlaySoundToSet(5154);
				} break;
				case 3031:
				{
					// ZeppelinDocked  (ZeppelinHorn.wav)
					PlaySoundToSet(11804);
				} break;
				default:
				{
					// BoatDockingWarning (BoatDockedWarning.wav)
					PlaySoundToSet(5495);
				} break;
			}
		}
	}

	// Smooth movement between waypoints on the same map.
	// Without this the transporter snaps between sparse waypoints which causes visible jitter
	// and server-side position corrections ("rubberbanding") for passengers.
	if (mCurrentWaypoint == m_WayPoints.end() || mNextWaypoint == m_WayPoints.end())
		return;

	if (mCurrentWaypoint->second.mapid != GetMapId())
		return;

	// Don't attempt to interpolate across a map-teleport edge.
	if (mCurrentWaypoint->second.teleport || mNextWaypoint->second.teleport || (mCurrentWaypoint->second.mapid != mNextWaypoint->second.mapid))
		return;

	uint32 curT = mCurrentWaypoint->first;
	uint32 nextT = mNextWaypoint->first;

	uint32 span = (nextT - curT) % m_pathTime;
	if (span == 0)
		return;

	uint32 elapsed = (m_timer - curT) % m_pathTime;
	if (elapsed > span)
		elapsed = span;

	const float pct = float(elapsed) / float(span);

	const float nx = mCurrentWaypoint->second.x + (mNextWaypoint->second.x - mCurrentWaypoint->second.x) * pct;
	const float ny = mCurrentWaypoint->second.y + (mNextWaypoint->second.y - mCurrentWaypoint->second.y) * pct;
	const float nz = mCurrentWaypoint->second.z + (mNextWaypoint->second.z - mCurrentWaypoint->second.z) * pct;

	// Face toward next point (avoids the "awkward" visual where the transport seems to slide).
	float o = m_position.o;
	const float dx = (mNextWaypoint->second.x - mCurrentWaypoint->second.x);
	const float dy = (mNextWaypoint->second.y - mCurrentWaypoint->second.y);
	if (dx != 0.0f || dy != 0.0f)
		o = atan2f(dy, dx);

	SetPosition(nx, ny, nz, o, false);
}

//
// Move the player to the new map..
//
void Transporter::TransportPassengers(uint32 mapid, uint32 oldmap, float x, float y, float z, float o)
{
	// Move the transport first so clients have a solid collider on the destination map
	// before passengers are relocated. Doing this after relocating passengers can cause
	// them to fall and take damage (or die) during the transfer tick.
	RemoveFromWorld(false);
	SetMapId(mapid);
	SetPosition(x, y, z, o, false);
	AddToWorld();

	if (mPassengers.size() == 0)
		return;

	PassengerIterator itr = mPassengers.begin();
	PassengerIterator it2;

	WorldPacket Pending(SMSG_TRANSFER_PENDING, 12);
	Pending << mapid << GetEntry() << oldmap;

	LocationVector v;

	for (; itr != mPassengers.end();)
	{
		it2 = itr;
		++itr;

		Player* plr = objmgr.GetPlayer(it2->first);
		if (!plr)
		{
			// remove all non players from map
			mPassengers.erase(it2);
			continue;
		}
		if (!plr->GetSession() || !plr->IsInWorld())
			continue;

		v.x = x + plr->m_TransporterX;
		v.y = y + plr->m_TransporterY;
		v.z = z + plr->m_TransporterZ;
		v.o = plr->GetOrientation();

		if (mapid == 530 && !plr->GetSession()->HasFlag(ACCOUNT_FLAG_XPACK_01))
		{
			// player does not have BC content, repop at graveyard
			plr->RepopAtGraveyard(plr->GetPositionX(), plr->GetPositionY(), plr->GetPositionZ(), plr->GetMapId());
			continue;
		}

		plr->GetSession()->SendPacket(&Pending);
		plr->_Relocate(mapid, v, false, true, 0);

		// Lucky bitch. Do it like on official.
		if (plr->isDead())
		{
 			plr->ResurrectPlayer();
 			plr->SetUInt32Value(UNIT_FIELD_HEALTH, plr->GetUInt32Value(UNIT_FIELD_MAXHEALTH));
 			plr->SetUInt32Value(UNIT_FIELD_POWER1, plr->GetUInt32Value(UNIT_FIELD_MAXPOWER1));
		}
	}
}

void ObjectMgr::LoadTransporters()
{
	Log.Notice("ObjectMgr", "Loading Transports...");
	QueryResult* QR = WorldDatabase.Query("SELECT * FROM transport_data");
	if(!QR) return;

	int64 total = QR->GetRowCount();
	TransportersCount=total;
	do 
	{
		uint32 entry = QR->Fetch()[0].GetUInt32();
		int32 period = QR->Fetch()[2].GetInt32();

		Transporter * pTransporter = new Transporter((uint64)HIGHGUID_TYPE_TRANSPORTER<<32 |entry);
		if (!pTransporter->CreateAsTransporter(entry, "", period))
		{
			sLog.outError("Transporter %s failed creation for some reason.", QR->Fetch()[1].GetString());
			delete pTransporter;
		}
		else
		{
            AddTransport(pTransporter);
		}

	} while(QR->NextRow());
	delete QR;
}

