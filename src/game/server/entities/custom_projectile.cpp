/*************************************************
*                                                *
*              B L O C K D D R A C E             *
*                                                *
**************************************************/

#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/teams.h>
#include "custom_projectile.h"
#include <game/server/gamemodes/DDRace.h>
#include <engine/shared/config.h>

CCustomProjectile::CCustomProjectile(CGameWorld *pGameWorld, int Owner, vec2 Pos, vec2 Dir, bool Freeze,
		bool Explosive, bool Unfreeze, bool Bloody, bool Ghost, bool Spooky, int Type, float Lifetime, float Accel, float Speed) :
		CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Owner = Owner;
	m_Pos = Pos;
	m_Core = normalize(Dir) * Speed;
	m_Freeze = Freeze;
	m_Explosive = Explosive;
	m_Unfreeze = Unfreeze;
	m_Bloody = Bloody;
	m_Ghost = Ghost;
	m_Spooky = Spooky;
	m_Direction = Dir;
	m_EvalTick = Server()->Tick();
	m_LifeTime = Server()->TickSpeed() * Lifetime;
	m_Type = Type;
	m_Accel = Accel;

	m_PrevPos = m_Pos;

	GameWorld()->InsertEntity(this);
}

void CCustomProjectile::Reset()
{
	GameWorld()->DestroyEntity(this);
}

void CCustomProjectile::Tick()
{
	pOwner = 0;
	if (GameServer()->GetPlayerChar(m_Owner))
		pOwner = GameServer()->GetPlayerChar(m_Owner);

	if (m_Owner >= 0 && !pOwner && g_Config.m_SvDestroyBulletsOnDeath)
		Reset();

	m_TeamMask = pOwner ? pOwner->Teams()->TeamMask(pOwner->Team(), -1, m_Owner) : -1LL;

	m_LifeTime--;
	if (m_LifeTime <= 0)
		Reset();

	Move();
	HitCharacter();

	if (GameServer()->Collision()->IsSolid(m_Pos.x, m_Pos.y))
	{
		if (m_Explosive)
		{
			GameServer()->CreateExplosion(m_Pos, m_Owner, m_Type, m_Owner == -1, pOwner ? pOwner->Team() : -1, m_TeamMask);
			GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE, m_TeamMask);
		}

		if (m_Bloody)
		{
			if (m_Ghost && m_CollisionState == COLLIDED_ONCE)
			{
				if (Server()->Tick() % 5 == 0)
					GameServer()->CreateDeath(m_PrevPos, m_Owner);
			}
			else
				GameServer()->CreateDeath(m_PrevPos, m_Owner);
		}

		if (m_CollisionState == COLLIDED_NOT)
			m_CollisionState = COLLIDED_ONCE;

		if (m_CollisionState == COLLIDED_TWICE || !m_Ghost)
			Reset();
	}
	else if(m_CollisionState == COLLIDED_ONCE)
		m_CollisionState = COLLIDED_TWICE;

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
		m_EvalTick = Server()->Tick();
	}

	m_PrevPos = m_Pos;
}

void CCustomProjectile::Move()
{
	m_Pos += m_Core;
	m_Core *= m_Accel;
}

void CCustomProjectile::HitCharacter()
{
	vec2 NewPos = m_Pos + m_Core;
	CCharacter* pHit = GameWorld()->IntersectCharacter(m_PrevPos, NewPos, 6.0f, NewPos, pOwner, m_Owner);
	if (!pHit)
		return;

	if (
		pHit->GetPlayer()->GetCID() == m_Owner
		|| pHit->m_Passive
		|| pOwner->m_Passive
		|| pHit->Team() != pOwner->Team()
		)
		return;

	if (m_Spooky)
	{
		pHit->SetEmote(EMOTE_SURPRISE, Server()->Tick() + 2 * Server()->TickSpeed());
		GameServer()->SendEmoticon(pHit->GetPlayer()->GetCID(), EMOTICON_GHOST);
	}

	if (m_Bloody)
		GameServer()->CreateDeath(m_PrevPos, pHit->GetPlayer()->GetCID());

	if (m_Freeze)
		pHit->Freeze();
	else if (m_Unfreeze)
		pHit->UnFreeze();

	if (m_Explosive)
	{
		GameServer()->CreateExplosion(m_Pos, m_Owner, m_Type, m_Owner == -1, pHit->Team(), m_TeamMask);
		GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE, m_TeamMask);
	}
	else
		pHit->TakeDamage(m_Direction * max(0.001f, 0.0f), g_pData->m_Weapons.m_aId[GameServer()->GetRealWeapon(m_Type)].m_Damage, m_Owner, m_Type);

	Reset();
}

void CCustomProjectile::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient))
		return;

	if (GameServer()->GetPlayerChar(SnappingClient))
	{
		if (!CmaskIsSet(m_TeamMask, SnappingClient))
			return;
	}

	if (m_Type == WEAPON_PLASMA_RIFLE)
	{
		CNetObj_Laser *pLaser = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
		if (!pLaser)
			return;

		pLaser->m_X = (int)m_Pos.x;
		pLaser->m_Y = (int)m_Pos.y;
		pLaser->m_FromX = (int)m_Pos.x;
		pLaser->m_FromY = (int)m_Pos.y;
		pLaser->m_StartTick = m_EvalTick;
	}
	else if (m_Type == WEAPON_HEART_GUN)
	{
		CNetObj_Pickup *pPickup = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
		if (!pPickup)
			return;

		pPickup->m_X = (int)m_Pos.x;
		pPickup->m_Y = (int)m_Pos.y;
		pPickup->m_Type = POWERUP_HEALTH;
	}
}
