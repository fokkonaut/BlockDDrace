/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "straight_grenade.h"

#include <game/server/teams.h>

CStraightGrenade::CStraightGrenade(CGameWorld *pGameWorld, int Lifetime, int Owner, float Force, vec2 Dir)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Owner = Owner;
	m_StartTick = Server()->Tick();
	m_Lifetime = Server()->TickSpeed() * Lifetime;

	m_Direction = Dir;
	m_Force = Force;

	m_LastResetPos = GameServer()->GetPlayerChar(Owner)->m_Pos;
	m_LastResetTick = Server()->Tick();
	
	m_Pos = GameServer()->GetPlayerChar(Owner)->m_Pos;

	m_CalculatedVel = false;

	GameWorld()->InsertEntity(this);
}

void CStraightGrenade::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CStraightGrenade::Tick()
{
	m_Lifetime--;

	if (m_Lifetime < 0)
	{
		Reset();
		return;
	}

	if (!GameServer()->m_apPlayers[m_Owner])
	{
		m_Owner = -1;

		GameServer()->CreateExplosion(m_Pos, -1, WEAPON_STRAIGHT_GRENADE, true, 0, -1);
		GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE, -1);

		Reset();
		return;
	}

	if (GameServer()->Collision()->IsSolid(m_Pos.x, m_Pos.y))
	{
		GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_STRAIGHT_GRENADE, true, 0, GameServer()->GetPlayerChar(m_Owner)->Teams()->TeamMask(0));
		GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE, GameServer()->GetPlayerChar(m_Owner)->Teams()->TeamMask(0));

		Reset();
		return;
	}

	m_Pos += m_Direction * 15;

	CCharacter* pTarget = CharacterNear();
	if(!pTarget)
		return;

	if(Hit(pTarget))
	{
		Reset();
		return;
	}
}

void CStraightGrenade::TickDefered()
{
	if (Server()->Tick() % 4 == 1)
	{
		m_LastResetPos = m_Pos;
		m_LastResetTick = Server()->Tick();
	}
	m_CalculatedVel = false;
}

void CStraightGrenade::CalculateVel()
{
	float Time = (Server()->Tick() - m_LastResetTick) / (float)Server()->TickSpeed();
	float Curvature = 0;
	float Speed = 0;

	Curvature = 0;
	Speed = 1000;

	m_VelX = ((m_Pos.x - m_LastResetPos.x) / Time / Speed) * 100;
	m_VelY = ((m_Pos.y - m_LastResetPos.y) / Time / Speed - Time * Speed*Curvature / 10000) * 100;

	m_CalculatedVel = true;
}

void CStraightGrenade::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient))
		return;

	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
	if (!pProj)
		return;

	if (!m_CalculatedVel)
		CalculateVel();

	pProj->m_X = (int)m_LastResetPos.x;
	pProj->m_Y = (int)m_LastResetPos.y;
	pProj->m_VelX = m_VelX;
	pProj->m_VelY = m_VelY;
	pProj->m_StartTick = m_LastResetTick;
	pProj->m_Type = WEAPON_GRENADE;
}

CCharacter* CStraightGrenade::CharacterNear()
{
	CCharacter* pOwner  = GameServer()->GetPlayerChar(m_Owner);
	CCharacter* pTarget = GameWorld()->ClosestCharacter(m_Pos, 2000.f, pOwner ? pOwner : 0);

	if(pTarget)
		return pTarget;

	return 0x0;
}

bool CStraightGrenade::Hit(CCharacter* pHitTarget)
{
	vec2 TargetPos = pHitTarget->m_Pos;
	CCharacter* pOwner = GameServer()->GetPlayerChar(m_Owner);

	if(distance(m_Pos, TargetPos) < 40.f && !pHitTarget->m_Passive && (pOwner && !pOwner->m_Passive))
	{
		pHitTarget->TakeDamage(m_Direction * max(0.001f, m_Force), 1, m_Owner, WEAPON_GRENADE);

		GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_STRAIGHT_GRENADE, true, 0, pOwner ? GameServer()->GetPlayerChar(m_Owner)->Teams()->TeamMask(0) : -1LL);
		GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE, pOwner ? GameServer()->GetPlayerChar(m_Owner)->Teams()->TeamMask(0) : -1LL);

		return true;
	}

	return false;
}