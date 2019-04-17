/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
#include <engine/server.h>
#include <engine/config.h>
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/teams.h>
#include <game/server/gamemodes/DDRace.h>
#include "custom_projectile.h"

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
	GameWorld()->InsertEntity(this);
}

bool CCustomProjectile::HitCharacter()
{
	vec2 To2;
	CCharacter *Hit = GameServer()->m_World.IntersectCharacter(m_Pos, m_Pos + m_Core, 0.0f, To2);
	if (!Hit)
		return false;

	if (Hit->GetPlayer()->GetCID() == m_Owner) // dont hit yourself
		return false;
	else if (Hit->m_Passive || (GameServer()->GetPlayerChar(m_Owner) && GameServer()->GetPlayerChar(m_Owner)->m_Passive))
		return false;

	if (m_Spooky)
	{
		Hit->SetEmote(3, Server()->Tick() + 2 * Server()->TickSpeed()); // eyeemote surprise
		GameServer()->SendEmoticon(Hit->GetPlayer()->GetCID(), 7);		//emoticon ghost
	}

	if (m_Bloody)
		GameServer()->CreateDeath(m_Pos, Hit->GetPlayer()->GetCID());

	if (m_Freeze)
		Hit->Freeze();

	if (m_Unfreeze)
		Hit->UnFreeze();
	
	if (m_Explosive)
		GameServer()->CreateExplosion(m_Pos, m_Owner, m_Type, m_Owner == -1, (!Hit ? -1 : Hit->Team()), -1LL);
	else
		Hit->TakeDamage(m_Direction * max(0.001f, 0.0f), g_pData->m_Weapons.m_aId[GameServer()->GetRealWeapon(m_Type)].m_Damage, m_Owner, m_Type);

	GameServer()->m_World.DestroyEntity(this);
	return true;
}

void CCustomProjectile::Move()
{
	m_Pos += m_Core;
	m_Core *= m_Accel;
}

void CCustomProjectile::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CCustomProjectile::Tick()
{
	if (m_LifeTime == 0)
	{
		Reset();
		return;
	}
	m_LifeTime--;
	Move();
	HitCharacter();

	bool Res;
	Res = GameServer()->Collision()->IsSolid(m_Pos.x, m_Pos.y);
	if (Res)
	{
		if (m_Explosive)
			GameServer()->CreateExplosion(m_Pos, m_Owner, m_Type, m_Owner == -1, -1, -1LL);

		if (m_Bloody)
		{
			if (m_IsInsideWall == 1 && Server()->Tick() % 5 == 0)
				GameServer()->CreateDeath(m_Pos, m_Owner);
			else
				GameServer()->CreateDeath(m_Pos, m_Owner);
		}

		if (m_Ghost && m_IsInsideWall == 0)
			m_IsInsideWall = 1; // enteres the wall, collides the first time

		if (m_IsInsideWall == 2 || !m_Ghost) // collides second time with a wall
			Reset();
	}
	else if(m_Ghost && m_IsInsideWall == 1)
		m_IsInsideWall = 2; // leaves the wall
}

void CCustomProjectile::Snap(int SnappingClient)
{
	if (NetworkClipped(SnappingClient))
		return;

	if (m_Type == WEAPON_PLASMA_RIFLE)
	{
		CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
		if (!pObj)
			return;

		pObj->m_X = (int)m_Pos.x;
		pObj->m_Y = (int)m_Pos.y;
		pObj->m_FromX = (int)m_Pos.x;
		pObj->m_FromY = (int)m_Pos.y;
		pObj->m_StartTick = m_EvalTick;
	}
	else if (m_Type == WEAPON_HEART_GUN)
	{
		CNetObj_Pickup *pObj = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
		if (!pObj)
			return;

		pObj->m_X = (int)m_Pos.x;
		pObj->m_Y = (int)m_Pos.y;
		pObj->m_Type = POWERUP_HEALTH;
	}
}
