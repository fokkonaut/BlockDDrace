#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "weapon.h"
#include "pickup.h"

#include <game/server/teams.h>

CWeapon::CWeapon(CGameWorld *pGameWorld, int Weapon, int Lifetime, int Owner, int Direction, int Bullets, bool Jetpack)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
	m_Type = Weapon;
	m_Lifetime = Server()->TickSpeed() * Lifetime;
	m_Pos = GameServer()->GetPlayerChar(Owner)->m_Pos;
	m_Jetpack = Jetpack;
	m_Bullets = Bullets;
	m_Owner = Owner;
	m_Vel = vec2(5*Direction, -5);
	m_PickupDelay = Server()->TickSpeed() * 2;
	m_PrevPos = m_Pos;

	m_ID2 = Server()->SnapNewID();

	GameWorld()->InsertEntity(this);
}

void CWeapon::Reset(bool EreaseWeapon, bool Picked)
{
	if (EreaseWeapon)
	{
		CPlayer* pOwner = GameServer()->m_apPlayers[m_Owner];
		if (pOwner)
		{
			for (unsigned i = 0; i < pOwner->m_vWeaponLimit[m_Type].size(); i++)
			{
				if (pOwner->m_vWeaponLimit[m_Type][i] == this)
				{
					pOwner->m_vWeaponLimit[m_Type].erase(pOwner->m_vWeaponLimit[m_Type].begin() + i);
				}
			}
		}
	}

	if (!Picked)
		GameServer()->CreateDeath(m_Pos, -1);

	Server()->SnapFreeID(m_ID2);
	GameServer()->m_World.DestroyEntity(this);
}

void CWeapon::Tick()
{
	pOwner = 0;
	if (m_Owner != -1 && GameServer()->GetPlayerChar(m_Owner))
		pOwner = GameServer()->GetPlayerChar(m_Owner);

	if (m_Owner >= 0 && !GameServer()->m_apPlayers[m_Owner])
		Reset();

	m_TeamMask = pOwner ? pOwner->Teams()->TeamMask(pOwner->Team(), -1, m_Owner) : -1LL;

	// weapon hits death-tile or left the game layer, reset it
	if (GameServer()->Collision()->GetCollisionAt(m_Pos.x, m_Pos.y) == TILE_DEATH || GameServer()->Collision()->GetFCollisionAt(m_Pos.x, m_Pos.y) == TILE_DEATH || GameLayerClipped(m_Pos))
		Reset();

	m_Lifetime--;
	if (m_Lifetime <= 0)
		Reset();

	if (m_PickupDelay > 0)
		m_PickupDelay--;

	Pickup();
	IsShieldNear();
	HandleDropped();

	m_PrevPos = m_Pos;
}

void CWeapon::Pickup()
{
	int ID = IsCharacterNear();
	if (ID != -1)
	{
		CCharacter* pChr = GameServer()->GetPlayerChar(ID);

		int Ammo = pChr->GetPlayer()->m_Gamemode == MODE_VANILLA ? m_Bullets : -1;
		pChr->GiveWeapon(m_Type, false, Ammo);
		GameServer()->SendWeaponPickup(pChr->GetPlayer()->GetCID(), m_Type);

		if (m_Jetpack)
			pChr->Jetpack();

		if (m_Type == WEAPON_SHOTGUN || m_Type == WEAPON_RIFLE || m_Type == WEAPON_PLASMA_RIFLE)
			GameServer()->CreateSound(m_Pos, SOUND_PICKUP_SHOTGUN, pChr->Teams()->TeamMask(pChr->Team()));
		else if (m_Type == WEAPON_GRENADE || m_Type == WEAPON_STRAIGHT_GRENADE)
			GameServer()->CreateSound(m_Pos, SOUND_PICKUP_GRENADE, pChr->Teams()->TeamMask(pChr->Team()));
		else if (m_Type == WEAPON_HAMMER || m_Type == WEAPON_GUN || m_Type == WEAPON_HEART_GUN)
			GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR, pChr->Teams()->TeamMask(pChr->Team()));

		Reset(true, true);
	}
}

int CWeapon::IsCharacterNear()
{
	CCharacter *apEnts[MAX_CLIENTS];
	int Num = GameWorld()->FindEntities(m_Pos, 20.0f, (CEntity**)apEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

	for (int i = 0; i < Num; i++)
	{
		CCharacter* pChr = apEnts[i];

		if (
			(m_PickupDelay > 0 && pChr == GameServer()->GetPlayerChar(m_Owner))
			|| (!pChr->CanCollide(m_Owner))
			|| (pChr->GetPlayer()->m_SpookyGhost && m_Type != WEAPON_GUN)
			|| (pChr->GetWeaponGot(m_Type) && !m_Jetpack && pChr->GetPlayer()->m_Gamemode == MODE_DDRACE)
			|| (m_Jetpack && !pChr->GetWeaponGot(WEAPON_GUN))
			|| (m_Jetpack && pChr->m_Jetpack)
			|| (pChr->GetPlayer()->m_Gamemode == MODE_VANILLA && pChr->GetWeaponGot(m_Type) && pChr->GetWeaponAmmo(m_Type) >= m_Bullets)
			)
			continue;

		return pChr->GetPlayer()->GetCID();
	}

	return -1;
}

void CWeapon::IsShieldNear()
{
	CPickup *apEnts[9];
	int Num = GameWorld()->FindEntities(m_Pos, 20.0f, (CEntity**)apEnts, 9, CGameWorld::ENTTYPE_PICKUP);

	for (int i = 0; i < Num; i++)
	{
		CPickup *pShield = apEnts[i];

		if (pShield->GetType() == POWERUP_ARMOR)
		{
			if (GameServer()->m_apPlayers[m_Owner]->m_Gamemode == MODE_DDRACE)
			{
				GameServer()->CreateSound(m_Pos, SOUND_PICKUP_ARMOR);
				Reset();
			}
		}
	}
}

bool CWeapon::IsGrounded(bool SetVel)
{
	if ((GameServer()->Collision()->CheckPoint(m_Pos.x + ms_PhysSize, m_Pos.y + ms_PhysSize + 5))
		|| (GameServer()->Collision()->CheckPoint(m_Pos.x - ms_PhysSize, m_Pos.y + ms_PhysSize + 5)))
	{
		if (SetVel)
			m_Vel.x *= 0.75f;
		return true;
	}

	if ((m_Vel.y < 0 && ((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_180) || (m_TileIndexB == TILE_STOP && m_TileFlagsB == ROTATION_180) || (m_TileIndexB == TILE_STOPS && (m_TileFlagsB == ROTATION_0 || m_TileFlagsB == ROTATION_180)) || (m_TileIndexB == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_180) || (m_TileFIndexB == TILE_STOP && m_TileFFlagsB == ROTATION_180) || (m_TileFIndexB == TILE_STOPS && (m_TileFFlagsB == ROTATION_0 || m_TileFFlagsB == ROTATION_180)) || (m_TileFIndexB == TILE_STOPA)))
		|| (m_Vel.y > 0 && ((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_0) || (m_TileIndexT == TILE_STOP && m_TileFlagsT == ROTATION_0) || (m_TileIndexT == TILE_STOPS && (m_TileFlagsT == ROTATION_0 || m_TileFlagsT == ROTATION_180)) || (m_TileIndexT == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_0) || (m_TileFIndexT == TILE_STOP && m_TileFFlagsT == ROTATION_0) || (m_TileFIndexT == TILE_STOPS && (m_TileFFlagsT == ROTATION_0 || m_TileFFlagsT == ROTATION_180)) || (m_TileFIndexT == TILE_STOPA))))
	{
		m_Vel.y = 0;
		if (SetVel)
			m_Vel.x *= 0.925f;
		return true;
	}

	if (SetVel)
		m_Vel.x *= 0.98f;
	return false;
}

void CWeapon::HandleDropped()
{
	//Gravity
	m_Vel.y += GameServer()->Tuning()->m_Gravity;

	//Friction
	vec2 TempVel = m_Vel;

	//Speedups
	if (GameServer()->Collision()->IsSpeedup(GameServer()->Collision()->GetMapIndex(m_Pos)))
	{
		int Force, MaxSpeed = 0;
		vec2 Direction, MaxVel;
		float TeeAngle, SpeederAngle, DiffAngle, SpeedLeft, TeeSpeed;
		GameServer()->Collision()->GetSpeedup(GameServer()->Collision()->GetMapIndex(m_Pos), &Direction, &Force, &MaxSpeed);

		if (Force == 255 && MaxSpeed)
		{
			m_Vel = Direction * (MaxSpeed / 5);
		}

		else
		{
			if (MaxSpeed > 0 && MaxSpeed < 5) MaxSpeed = 5;
			if (MaxSpeed > 0)
			{
				if (Direction.x > 0.0000001f)
					SpeederAngle = -atan(Direction.y / Direction.x);
				else if (Direction.x < 0.0000001f)
					SpeederAngle = atan(Direction.y / Direction.x) + 2.0f * asin(1.0f);
				else if (Direction.y > 0.0000001f)
					SpeederAngle = asin(1.0f);
				else
					SpeederAngle = asin(-1.0f);

				if (SpeederAngle < 0)
					SpeederAngle = 4.0f * asin(1.0f) + SpeederAngle;

				if (TempVel.x > 0.0000001f)
					TeeAngle = -atan(TempVel.y / TempVel.x);
				else if (TempVel.x < 0.0000001f)
					TeeAngle = atan(TempVel.y / TempVel.x) + 2.0f * asin(1.0f);
				else if (TempVel.y > 0.0000001f)
					TeeAngle = asin(1.0f);
				else
					TeeAngle = asin(-1.0f);

				if (TeeAngle < 0)
					TeeAngle = 4.0f * asin(1.0f) + TeeAngle;

				TeeSpeed = sqrt(pow(TempVel.x, 2) + pow(TempVel.y, 2));

				DiffAngle = SpeederAngle - TeeAngle;
				SpeedLeft = MaxSpeed / 5.0f - cos(DiffAngle) * TeeSpeed;
				if (abs(SpeedLeft) > Force && SpeedLeft > 0.0000001f)
					TempVel += Direction * Force;
				else if (abs(SpeedLeft) > Force)
					TempVel += Direction * -Force;
				else
					TempVel += Direction * SpeedLeft;
			}
			else
				TempVel += Direction * Force;
		}
	}

	//stopper
	int CurrentIndex = GameServer()->Collision()->GetMapIndex(m_Pos);
	std::list < int > Indices = GameServer()->Collision()->GetMapIndices(m_PrevPos, m_Pos);
	if (!Indices.empty())
		for (std::list < int >::iterator i = Indices.begin(); i != Indices.end(); i++)
			HandleTiles(*i);
	else
	{
		HandleTiles(CurrentIndex);
	}

	if (TempVel.x > 0 && ((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_270) || (m_TileIndexL == TILE_STOP && m_TileFlagsL == ROTATION_270) || (m_TileIndexL == TILE_STOPS && (m_TileFlagsL == ROTATION_90 || m_TileFlagsL == ROTATION_270)) || (m_TileIndexL == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_270) || (m_TileFIndexL == TILE_STOP && m_TileFFlagsL == ROTATION_270) || (m_TileFIndexL == TILE_STOPS && (m_TileFFlagsL == ROTATION_90 || m_TileFFlagsL == ROTATION_270)) || (m_TileFIndexL == TILE_STOPA)))
		TempVel.x = 0;
	if (TempVel.x < 0 && ((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_90) || (m_TileIndexR == TILE_STOP && m_TileFlagsR == ROTATION_90) || (m_TileIndexR == TILE_STOPS && (m_TileFlagsR == ROTATION_90 || m_TileFlagsR == ROTATION_270)) || (m_TileIndexR == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_90) || (m_TileFIndexR == TILE_STOP && m_TileFFlagsR == ROTATION_90) || (m_TileFIndexR == TILE_STOPS && (m_TileFFlagsR == ROTATION_90 || m_TileFFlagsR == ROTATION_270)) || (m_TileFIndexR == TILE_STOPA)))
		TempVel.x = 0;
	m_Vel = TempVel;
	IsGrounded(true);

	GameServer()->Collision()->MoveBox(&m_Pos, &m_Vel, vec2(ms_PhysSize, ms_PhysSize), 0.5f);
}

void CWeapon::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	if (GameServer()->GetPlayerChar(SnappingClient))
	{
		if (!CmaskIsSet(m_TeamMask, SnappingClient))
			return;
	}

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	pP->m_Type = POWERUP_WEAPON;
	pP->m_Subtype = GameServer()->GetRealWeapon(m_Type);

	if (m_Jetpack)
	{
		CNetObj_Projectile *pJetpackIndicator = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID2, sizeof(CNetObj_Projectile)));
		if (pJetpackIndicator)
		{
			pJetpackIndicator->m_X = pP->m_X;
			pJetpackIndicator->m_Y = pP->m_Y - 25;
			pJetpackIndicator->m_Type = WEAPON_SHOTGUN;
			pJetpackIndicator->m_StartTick = Server()->Tick();
		}
	}
}

void CWeapon::HandleTiles(int Index)
{
	int MapIndex = Index;
	float Offset = 4.0f;
	MapIndexL = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x + ms_PhysSize + Offset, m_Pos.y));
	MapIndexR = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x - ms_PhysSize - Offset, m_Pos.y));
	MapIndexT = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x, m_Pos.y + ms_PhysSize + Offset));
	MapIndexB = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x, m_Pos.y - ms_PhysSize - Offset));
	m_TileIndex = GameServer()->Collision()->GetTileIndex(MapIndex);
	m_TileFlags = GameServer()->Collision()->GetTileFlags(MapIndex);
	m_TileIndexL = GameServer()->Collision()->GetTileIndex(MapIndexL);
	m_TileFlagsL = GameServer()->Collision()->GetTileFlags(MapIndexL);
	m_TileIndexR = GameServer()->Collision()->GetTileIndex(MapIndexR);
	m_TileFlagsR = GameServer()->Collision()->GetTileFlags(MapIndexR);
	m_TileIndexB = GameServer()->Collision()->GetTileIndex(MapIndexB);
	m_TileFlagsB = GameServer()->Collision()->GetTileFlags(MapIndexB);
	m_TileIndexT = GameServer()->Collision()->GetTileIndex(MapIndexT);
	m_TileFlagsT = GameServer()->Collision()->GetTileFlags(MapIndexT);
	m_TileFIndex = GameServer()->Collision()->GetFTileIndex(MapIndex);
	m_TileFFlags = GameServer()->Collision()->GetFTileFlags(MapIndex);
	m_TileFIndexL = GameServer()->Collision()->GetFTileIndex(MapIndexL);
	m_TileFFlagsL = GameServer()->Collision()->GetFTileFlags(MapIndexL);
	m_TileFIndexR = GameServer()->Collision()->GetFTileIndex(MapIndexR);
	m_TileFFlagsR = GameServer()->Collision()->GetFTileFlags(MapIndexR);
	m_TileFIndexB = GameServer()->Collision()->GetFTileIndex(MapIndexB);
	m_TileFFlagsB = GameServer()->Collision()->GetFTileFlags(MapIndexB);
	m_TileFIndexT = GameServer()->Collision()->GetFTileIndex(MapIndexT);
	m_TileFFlagsT = GameServer()->Collision()->GetFTileFlags(MapIndexT);
}