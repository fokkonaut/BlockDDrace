/*************************************************
*                                                *
*              B L O C K D D R A C E             *
*                                                *
**************************************************/

// made by fokkonaut

#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "straight_grenade.h"
#include <game/server/teams.h>
#include <game/server/gamemodes/DDRace.h>
#include <engine/shared/config.h>

CStraightGrenade::CStraightGrenade(CGameWorld *pGameWorld, int Lifetime, int Owner, vec2 Pos, vec2 Dir)
: CStableProjectile(pGameWorld, WEAPON_STRAIGHT_GRENADE, Owner, Pos)
{
	m_Owner = Owner;
	m_StartTick = Server()->Tick();
	m_Lifetime = Server()->TickSpeed() * Lifetime;
	m_Direction = Dir;
	m_Pos = Pos;
	m_PrevPos = m_Pos;
	m_Core = m_Direction * 15;
}

void CStraightGrenade::Reset()
{
	GameWorld()->DestroyEntity(this);
}

void CStraightGrenade::Tick()
{
	pOwner = 0;
	if (GameServer()->GetPlayerChar(m_Owner))
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

	if (distance(m_Pos, pHit->m_Pos) < 40.f)
	{
		GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_STRAIGHT_GRENADE, m_Owner == -1, pHit ? pHit->Team() : -1, m_TeamMask);
		GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE, m_TeamMask);
		Reset();
	}
}
