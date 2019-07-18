/*************************************************
*                                                *
*              B L O C K D D R A C E             *
*                                                *
**************************************************/

// made by fokkonaut

#include <engine/server.h>
#include <game/server/gamecontext.h>
#include "lightsaber.h"
#include <game/server/teams.h>

CLightsaber::CLightsaber(CGameWorld *pGameWorld, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LIGHTSABER)
{
	m_Owner = Owner;
	m_Pos = GameServer()->GetPlayerChar(Owner)->m_Pos;
	m_EvalTick = Server()->Tick();
	m_SoundTick = 0;

	m_Lightsaber.m_Length = RETRACTED_LENGTH;
	m_Lightsaber.m_Extending = false;
	m_Lightsaber.m_Retracting = false;

	for (int i = 0; i < MAX_CLIENTS; i++)
		m_LastHit[i] = 0;

	GameWorld()->InsertEntity(this);
}

void CLightsaber::Reset()
{
	if (m_pOwner)
		m_pOwner->m_pLightsaber = 0;
	GameWorld()->DestroyEntity(this);
}

void CLightsaber::Extend()
{
	m_Lightsaber.m_Extending = true;
}

void CLightsaber::Retract()
{
	m_Lightsaber.m_Retracting = true;
}

void CLightsaber::PlaySound()
{
	if (m_SoundTick < Server()->Tick())
	{
		GameServer()->CreateSound(m_Pos, SOUND_RIFLE_BOUNCE, m_pOwner->Teams()->TeamMask(m_pOwner->Team(), -1, m_Owner));
		m_SoundTick = Server()->Tick() + Server()->TickSpeed() / 10;
	}
}

bool CLightsaber::HitCharacter()
{
	std::list<CCharacter *> HitCharacters = GameWorld()->IntersectedCharacters(m_Pos, m_Lightsaber.m_To, 0.0f, m_pOwner, m_Owner);
	if (HitCharacters.empty())
		return false;
	for (std::list<CCharacter *>::iterator i = HitCharacters.begin();
		i != HitCharacters.end(); i++)
	{
		CCharacter *pChr = *i;
		if (m_LastHit[pChr->GetPlayer()->GetCID()] < Server()->Tick())
		{
			pChr->TakeDamage(vec2(0.f, 0.f), g_pData->m_Weapons.m_aId[WEAPON_GUN].m_Damage, m_Owner, WEAPON_LIGHTSABER);
			m_LastHit[pChr->GetPlayer()->GetCID()] = Server()->Tick() + Server()->TickSpeed() / 4;
		}
	}
	return true;
}

void CLightsaber::Tick()
{
	m_pOwner = 0;
	if (m_Owner != -1 && GameServer()->GetPlayerChar(m_Owner))
		m_pOwner = GameServer()->GetPlayerChar(m_Owner);

	if (!m_pOwner)
		Reset();

	m_TeamMask = m_pOwner ? m_pOwner->Teams()->TeamMask(m_pOwner->Team(), -1, m_Owner) : -1LL;

	if (Server()->Tick() % int(Server()->TickSpeed() * 0.15f) == 0)
		m_EvalTick = Server()->Tick();
	Step();

	HitCharacter();

	if (m_Lightsaber.m_Extending && m_Lightsaber.m_Length < EXTENDED_LENGTH)
	{
		PlaySound();
		m_Lightsaber.m_Length += SPEED;
		if (m_Lightsaber.m_Length >= EXTENDED_LENGTH)
			m_Lightsaber.m_Extending = false;
	}

	if (m_Lightsaber.m_Retracting && m_Lightsaber.m_Length > RETRACTED_LENGTH)
	{
		PlaySound();
		m_Lightsaber.m_Length -= SPEED;
		if (m_Lightsaber.m_Length <= RETRACTED_LENGTH)
			Reset();
	}
}

void CLightsaber::Step()
{
	vec2 Direction = normalize(vec2(m_pOwner->GetInput().m_TargetX, m_pOwner->GetInput().m_TargetY));
	m_Lightsaber.m_Rotation = -GetAngle(Direction) - 4.725f;

	vec2 dir(sin(m_Lightsaber.m_Rotation), cos(m_Lightsaber.m_Rotation));
	vec2 to2 = m_Pos + normalize(dir) * m_Lightsaber.m_Length;
	GameServer()->Collision()->IntersectLine(m_Pos, to2, &m_Lightsaber.m_To, 0);
}

void CLightsaber::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient, m_Pos) && NetworkClipped(SnappingClient, m_Lightsaber.m_To))
		return;

	if (GameServer()->GetPlayerChar(SnappingClient))
	{
		if (!CmaskIsSet(m_TeamMask, SnappingClient))
			return;
	}

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
	if (!pObj)
		return;

	if (m_pOwner)
		m_Pos = m_pOwner->m_Pos;

	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_Lightsaber.m_To.x;
	pObj->m_FromY = (int)m_Lightsaber.m_To.y;

	int StartTick = m_EvalTick;
	if (StartTick < Server()->Tick() - 2)
		StartTick = Server()->Tick() - 2;
	else if (StartTick > Server()->Tick())
		StartTick = Server()->Tick();
	pObj->m_StartTick = StartTick;
}
