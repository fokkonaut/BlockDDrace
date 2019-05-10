/*************************************************
*                                                *
*              B L O C K D D R A C E             *
*                                                *
**************************************************/

// made by fokkonaut

#include <engine/server.h>
#include <game/server/gamecontext.h>
#include "clock.h"
#include <time.h>

CClock::CClock(CGameWorld *pGameWorld, vec2 Pos)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_Pos = Pos;

	m_Hand[SECOND].m_Length = 72;
	m_Hand[MINUTE].m_Length = 96;
	m_Hand[HOUR].m_Length = 60;

	m_ID2 = Server()->SnapNewID();
	m_ID3 = Server()->SnapNewID();
	GameWorld()->InsertEntity(this);

	Step();
}

void CClock::Reset()
{
	Server()->SnapFreeID(m_ID2);
	Server()->SnapFreeID(m_ID3);
	GameWorld()->DestroyEntity(this);
}

void CClock::Tick()
{
	if (Server()->Tick() % int(Server()->TickSpeed() * 0.15f) == 0)
		Step();
}

void CClock::Step()
{
	SetHandRotations();
	for (int i = 0; i < 3; i++)
	{
		vec2 dir(sin(m_Hand[i].m_Rotation), cos(m_Hand[i].m_Rotation));
		vec2 to2 = m_Pos + normalize(dir) * m_Hand[i].m_Length;
		GameServer()->Collision()->IntersectLine(m_Pos, to2, &m_Hand[i].m_To, 0);
	}
}

void CClock::SetHandRotations()
{
	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	m_Hand[SECOND].m_Rotation = -(timeinfo->tm_sec+30) * pi / 30;
	m_Hand[MINUTE].m_Rotation = -(timeinfo->tm_min+30) * pi / 30;
	m_Hand[HOUR].m_Rotation = -(timeinfo->tm_hour+6) * pi / 6;
}

int CClock::GetID(int Hand)
{
	switch (Hand)
	{
	case SECOND:
		return m_ID;
	case MINUTE:
		return m_ID2;
	case HOUR:
		return m_ID3;
	}
}

void CClock::Snap(int SnappingClient)
{
	// send hands in swapped order to have the second hand over ther minute and hour hand
	for (int i = 2; i > -1; i--)
	{
		if (NetworkClipped(SnappingClient, m_Pos) && NetworkClipped(SnappingClient, m_Hand[i].m_To))
			return;

		CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, GetID(i), sizeof(CNetObj_Laser)));
		if (!pObj)
			return;

		pObj->m_X = (int)m_Pos.x;
		pObj->m_Y = (int)m_Pos.y;
		pObj->m_FromX = (int)m_Hand[i].m_To.x;
		pObj->m_FromY = (int)m_Hand[i].m_To.y;

		int StartTick = Server()->Tick();

		// the second hand is the thinnest, and the hour hand is the thickest
		if (i == SECOND)
			StartTick = Server()->Tick() - 4;
		else if (i == MINUTE)
			StartTick = Server()->Tick() - 3;
		else if (i == HOUR)
			StartTick = Server()->Tick() - 2;

		pObj->m_StartTick = StartTick;
	}
}
