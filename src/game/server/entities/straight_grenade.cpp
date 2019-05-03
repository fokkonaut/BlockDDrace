#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "straight_grenade.h"
#include <game/server/teams.h>
#include <game/server/gamemodes/DDRace.h>
#include <engine/shared/config.h>

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
	m_PrevPos = m_Pos;

	m_Core = m_Direction * 15;
	m_CalculatedVel = false;

	GameWorld()->InsertEntity(this);
}

void CStraightGrenade::Reset()
{
	GameWorld()->DestroyEntity(this);
}

void CStraightGrenade::Tick()
{
	pOwner = 0;
	if (m_Owner != -1 && GameServer()->GetPlayerChar(m_Owner))
		pOwner = GameServer()->GetPlayerChar(m_Owner);

	if (m_Owner >= 0 && !pOwner && g_Config.m_SvDestroyBulletsOnDeath)
		Reset();

	m_TeamMask = pOwner ? pOwner->Teams()->TeamMask(pOwner->Team(), -1, m_Owner) : -1LL;

	m_Lifetime--;
	if (m_Lifetime <= 0)
		Reset();

	if (GameServer()->Collision()->IsSolid(m_Pos.x, m_Pos.y))
	{
		GameServer()->CreateExplosion(m_PrevPos, m_Owner, WEAPON_STRAIGHT_GRENADE, m_Owner == -1, pOwner ? pOwner->Team() : -1, m_TeamMask);
		GameServer()->CreateSound(m_PrevPos, SOUND_GRENADE_EXPLODE, m_TeamMask);
		Reset();
	}

	Move();
	HitCharacter();

	// weapon teleport
	int x = GameServer()->Collision()->GetIndex(m_PrevPos, m_Pos);
	int z;
	if (g_Config.m_SvOldTeleportWeapons)
		z = GameServer()->Collision()->IsTeleport(x);
	else
		z = GameServer()->Collision()->IsTeleportWeapon(x);
	if (z && ((CGameControllerDDRace*)GameServer()->m_pController)->m_TeleOuts[z - 1].size())
	{
		int Num = ((CGameControllerDDRace*)GameServer()->m_pController)->m_TeleOuts[z - 1].size();
		m_Pos = ((CGameControllerDDRace*)GameServer()->m_pController)->m_TeleOuts[z - 1][(!Num) ? Num : rand() % Num];
		m_StartTick = Server()->Tick();
	}

	m_PrevPos = m_Pos;
}

void CStraightGrenade::Move()
{
	m_Pos += m_Core;
}

void CStraightGrenade::HitCharacter()
{
	vec2 NewPos = m_Pos + m_Core;
	CCharacter* pHit = GameWorld()->IntersectCharacter(m_PrevPos, NewPos, 6.0f, NewPos, pOwner, m_Owner);
	if (!pHit)
		return;

	if (distance(m_Pos, pHit->m_Pos) < 40.f && !pHit->m_Passive && !pOwner->m_Passive)
	{
		GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_STRAIGHT_GRENADE, m_Owner == -1, pHit ? pHit->Team() : -1, m_TeamMask);
		GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE, m_TeamMask);
		Reset();
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

	if (GameServer()->GetPlayerChar(SnappingClient))
	{
		if (!CmaskIsSet(m_TeamMask, SnappingClient))
			return;
	}

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