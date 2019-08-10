/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/mapitems.h>

#include "character.h"
#include "laser.h"
#include "projectile.h"

// BlockDDrace
#include "flag.h"
#include "custom_projectile.h"
#include "meteor.h"
#include "pickup_drop.h"
#include "atom.h"
#include "trail.h"
// BlockDDrace

#include <stdio.h>
#include <string.h>
#include <game/server/gamemodes/blockddrace.h>
#include <game/server/score.h>
#include "light.h"


MACRO_ALLOC_POOL_ID_IMPL(CCharacter, MAX_CLIENTS)

// Character, "physical" player's part
CCharacter::CCharacter(CGameWorld *pWorld)
: CEntity(pWorld, CGameWorld::ENTTYPE_CHARACTER)
{
	m_ProximityRadius = ms_PhysSize;
	m_Health = 0;
	m_Armor = 0;
	m_StrongWeakID = 0;
}

void CCharacter::Reset()
{
	Destroy();
}

bool CCharacter::Spawn(CPlayer *pPlayer, vec2 Pos)
{
	m_EmoteStop = -1;
	m_LastAction = -1;
	m_LastNoAmmoSound = -1;
	m_LastWeapon = WEAPON_HAMMER;
	m_QueuedWeapon = -1;
	m_LastRefillJumps = false;
	m_LastPenalty = false;
	m_LastBonus = false;

	m_HasTeleGun = false;
	m_HasTeleLaser = false;
	m_HasTeleGrenade = false;
	m_TeleGunTeleport = false;
	m_IsBlueTeleGunTeleport = false;
	m_Solo = false;

	m_pPlayer = pPlayer;
	m_Pos = Pos;

	m_Core.Reset();
	m_Core.Init(&GameWorld()->m_Core, GameServer()->Collision(), &((CGameControllerBlockDDrace*)GameServer()->m_pController)->m_Teams.m_Core, &((CGameControllerBlockDDrace*)GameServer()->m_pController)->m_TeleOuts);
	SetActiveWeapon(WEAPON_GUN);
	m_Core.m_Pos = m_Pos;
	GameWorld()->m_Core.m_apCharacters[m_pPlayer->GetCID()] = &m_Core;

	m_ReckoningTick = 0;
	mem_zero(&m_SendCore, sizeof(m_SendCore));
	mem_zero(&m_ReckoningCore, sizeof(m_ReckoningCore));

	GameWorld()->InsertEntity(this);
	m_Alive = true;

	// BlockDDrace
	BlockDDraceInit();
	// BlockDDrace

	GameServer()->m_pController->OnCharacterSpawn(this);

	Teams()->OnCharacterSpawn(GetPlayer()->GetCID());

	DDRaceInit();

	m_TuneZone = GameServer()->Collision()->IsTune(GameServer()->Collision()->GetMapIndex(Pos));
	m_TuneZoneOld = -1; // no zone leave msg on spawn
	m_NeededFaketuning = 0; // reset fake tunings on respawn and send the client
	SendZoneMsgs(); // we want a entermessage also on spawn
	GameServer()->SendTuningParams(m_pPlayer->GetCID(), m_TuneZone);

	Server()->StartRecord(m_pPlayer->GetCID());
	return true;
}

void CCharacter::Destroy()
{
	GameWorld()->m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	m_Alive = false;
	m_Solo = false;
}

void CCharacter::SetWeapon(int W)
{
	// BlockDDrace
	if (!GetWeaponGot(W))
	{
		SetAvailableWeapon();
		return;
	}
	// BlockDDrace

	if(W == GetActiveWeapon())
		return;

	m_LastWeapon = GetActiveWeapon();
	m_QueuedWeapon = -1;
	SetActiveWeapon(W);
	GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SWITCH, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));

	if(GetActiveWeapon() < 0 || GetActiveWeapon() >= NUM_WEAPONS)
		SetActiveWeapon(WEAPON_GUN);
}

void CCharacter::SetSolo(bool Solo)
{
	m_Solo = Solo;
	m_Core.m_Solo = Solo;
	Teams()->m_Core.SetSolo(m_pPlayer->GetCID(), Solo);

	if(Solo)
		m_NeededFaketuning |= FAKETUNE_SOLO;
	else
		m_NeededFaketuning &= ~FAKETUNE_SOLO;

	GameServer()->SendTuningParams(m_pPlayer->GetCID(), m_TuneZone); // update tunings
}

bool CCharacter::IsGrounded()
{
	if(GameServer()->Collision()->CheckPoint(m_Pos.x+m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	if(GameServer()->Collision()->CheckPoint(m_Pos.x-m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;

	int index = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x, m_Pos.y+m_ProximityRadius/2+4));
	int tile = GameServer()->Collision()->GetTileIndex(index);
	int flags = GameServer()->Collision()->GetTileFlags(index);
	if(tile == TILE_STOPA || (tile == TILE_STOP && flags == ROTATION_0) || (tile ==TILE_STOPS && (flags == ROTATION_0 || flags == ROTATION_180)))
		return true;
	tile = GameServer()->Collision()->GetFTileIndex(index);
	flags = GameServer()->Collision()->GetFTileFlags(index);
	if(tile == TILE_STOPA || (tile == TILE_STOP && flags == ROTATION_0) || (tile ==TILE_STOPS && (flags == ROTATION_0 || flags == ROTATION_180)))
		return true;

	return false;
}

void CCharacter::HandleJetpack()
{
	vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));

	bool FullAuto = false;
	if (m_Jetpack && GetActiveWeapon() == WEAPON_GUN)
		FullAuto = true;

	// check if we gonna fire
	bool WillFire = false;
	if(CountInput(m_LatestPrevInput.m_Fire, m_LatestInput.m_Fire).m_Presses)
		WillFire = true;

	if(FullAuto && (m_LatestInput.m_Fire&1) && m_aWeapons[GetActiveWeapon()].m_Ammo)
		WillFire = true;

	if(!WillFire)
		return;

	// check for ammo
	if(!m_aWeapons[GetActiveWeapon()].m_Ammo)
	{
		return;
	}

	switch(GetActiveWeapon())
	{
		case WEAPON_GUN:
		{
			if (m_Jetpack)
			{
				float Strength;
				if (!m_TuneZone)
					Strength = GameServer()->Tuning()->m_JetpackStrength;
				else
					Strength = GameServer()->TuningList()[m_TuneZone].m_JetpackStrength;
				TakeDamage(Direction * -1.0f * (Strength / 100.0f / 6.11f), 0, m_pPlayer->GetCID(), GetActiveWeapon());
			}
		}
	}
}

void CCharacter::HandleNinja()
{
	if(GetActiveWeapon() != WEAPON_NINJA)
		return;

	if (!m_ScrollNinja)
	{
		if ((Server()->Tick() - m_Ninja.m_ActivationTick) > (g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000))
		{
			// time's up, return
			RemoveNinja();
			return;
		}

		int NinjaTime = m_Ninja.m_ActivationTick + (g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000) - Server()->Tick();

		if (NinjaTime % Server()->TickSpeed() == 0 && NinjaTime / Server()->TickSpeed() <= 5)
		{
			GameServer()->CreateDamageInd(m_Pos, 0, NinjaTime / Server()->TickSpeed(), Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));
		}

		//m_Armor = 10 - (NinjaTime / 15);

		// force ninja Weapon
		SetWeapon(WEAPON_NINJA);
	}

	m_Ninja.m_CurrentMoveTime--;

	if (m_Ninja.m_CurrentMoveTime == 0)
	{
		// reset velocity
		m_Core.m_Vel = m_Ninja.m_ActivationDir*m_Ninja.m_OldVelAmount;
	}

	if (m_Ninja.m_CurrentMoveTime > 0)
	{
		// Set velocity
		m_Core.m_Vel = m_Ninja.m_ActivationDir * g_pData->m_Weapons.m_Ninja.m_Velocity;
		vec2 OldPos = m_Pos;
		GameServer()->Collision()->MoveBox(&m_Core.m_Pos, &m_Core.m_Vel, vec2(m_ProximityRadius, m_ProximityRadius), 0.f);

		// reset velocity so the client doesn't predict stuff
		m_Core.m_Vel = vec2(0.f, 0.f);

		// check if we Hit anything along the way
		{
			CCharacter *aEnts[MAX_CLIENTS];
			vec2 Dir = m_Pos - OldPos;
			float Radius = m_ProximityRadius * 2.0f;
			vec2 Center = OldPos + Dir * 0.5f;
			int Num = GameWorld()->FindEntities(Center, Radius, (CEntity**)aEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

			for (int i = 0; i < Num; ++i)
			{
				if (aEnts[i] == this)
					continue;

				// check that we can collide with the other player
				if (!CanCollide(aEnts[i]->m_pPlayer->GetCID()))
					continue;

				// make sure we haven't Hit this object before
				bool bAlreadyHit = false;
				for (int j = 0; j < m_NumObjectsHit; j++)
				{
					if (m_apHitObjects[j] == aEnts[i])
						bAlreadyHit = true;
				}
				if (bAlreadyHit)
					continue;

				// check so we are sufficiently close
				if (distance(aEnts[i]->m_Pos, m_Pos) > (m_ProximityRadius * 2.0f))
					continue;

				// Hit a player, give him damage and stuffs...
				GameServer()->CreateSound(aEnts[i]->m_Pos, SOUND_NINJA_HIT, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));
				// set his velocity to fast upward (for now)
				if(m_NumObjectsHit < 10)
					m_apHitObjects[m_NumObjectsHit++] = aEnts[i];

				aEnts[i]->TakeDamage(vec2(0, -10.0f), g_pData->m_Weapons.m_Ninja.m_pBase->m_Damage, m_pPlayer->GetCID(), WEAPON_NINJA);
			}
		}

		return;
	}

	return;
}


void CCharacter::DoWeaponSwitch()
{
	// make sure we can switch
	if(m_ReloadTimer != 0 || m_QueuedWeapon == -1 || (m_aWeapons[WEAPON_NINJA].m_Got && !m_ScrollNinja) || !m_aWeapons[m_QueuedWeapon].m_Got)
		return;

	// switch Weapon
	SetWeapon(m_QueuedWeapon);
}

void CCharacter::HandleWeaponSwitch()
{
	int WantedWeapon = GetActiveWeapon();
	if(m_QueuedWeapon != -1)
		WantedWeapon = m_QueuedWeapon;

	bool Anything = false;
	 for(int i = 0; i < NUM_WEAPONS; ++i)
		 if(i != WEAPON_NINJA && m_aWeapons[i].m_Got)
			 Anything = true;
	 if(!Anything)
		 return;
	// select Weapon
	int Next = CountInput(m_LatestPrevInput.m_NextWeapon, m_LatestInput.m_NextWeapon).m_Presses;
	int Prev = CountInput(m_LatestPrevInput.m_PrevWeapon, m_LatestInput.m_PrevWeapon).m_Presses;

	if(Next < 128) // make sure we only try sane stuff
	{
		while(Next) // Next Weapon selection
		{
			WantedWeapon = (WantedWeapon+1)%NUM_WEAPONS;
			if(m_aWeapons[WantedWeapon].m_Got)
				Next--;
		}
	}

	if(Prev < 128) // make sure we only try sane stuff
	{
		while(Prev) // Prev Weapon selection
		{
			WantedWeapon = (WantedWeapon-1)<0?NUM_WEAPONS-1:WantedWeapon-1;
			if(m_aWeapons[WantedWeapon].m_Got)
				Prev--;
		}
	}

	// Direct Weapon selection
	if(m_LatestInput.m_WantedWeapon)
		WantedWeapon = m_Input.m_WantedWeapon-1;

	// check for insane values
	if(WantedWeapon >= 0 && WantedWeapon < NUM_WEAPONS && WantedWeapon != GetActiveWeapon() && m_aWeapons[WantedWeapon].m_Got)
		m_QueuedWeapon = WantedWeapon;

	DoWeaponSwitch();
}

void CCharacter::FireWeapon()
{
	if(m_ReloadTimer != 0)
		return;

	DoWeaponSwitch();
	vec2 TempDirection = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));

	bool FullAuto = false;
	if
	(
		GetActiveWeapon() == WEAPON_GRENADE
		|| GetActiveWeapon() == WEAPON_SHOTGUN
		|| GetActiveWeapon() == WEAPON_RIFLE
		|| GetActiveWeapon() == WEAPON_PLASMA_RIFLE
		|| GetActiveWeapon() == WEAPON_STRAIGHT_GRENADE
	)
		FullAuto = true;
	if (m_Jetpack && GetActiveWeapon() == WEAPON_GUN)
		FullAuto = true;

	// don't fire non auto weapons when player is deep and sv_deepfly is disabled
	if(!g_Config.m_SvDeepfly && !FullAuto && m_DeepFreeze)
		return;

	// check if we gonna fire
	bool WillFire = false;
	if(CountInput(m_LatestPrevInput.m_Fire, m_LatestInput.m_Fire).m_Presses)
		WillFire = true;

	if(FullAuto && (m_LatestInput.m_Fire&1) && m_aWeapons[GetActiveWeapon()].m_Ammo)
		WillFire = true;

	if(!WillFire)
		return;

	// shop window
	if (m_ShopWindowPage != SHOP_PAGE_NONE && m_PurchaseState == SHOP_STATE_OPENED_WINDOW)
	{
		ShopWindow(GetAimDir());
		return;
	}

	//spooky ghost
	if (m_pPlayer->m_PlayerFlags&PLAYERFLAG_SCOREBOARD && GameServer()->GetRealWeapon(GetActiveWeapon()) == WEAPON_GUN)
	{
		m_NumGhostShots++;
		if ((m_pPlayer->m_HasSpookyGhost || GameServer()->m_Accounts[m_pPlayer->GetAccID()].m_aHasItem[SPOOKY_GHOST]) && m_NumGhostShots == 2 && !m_pPlayer->m_SpookyGhost)
		{
			SetSpookyGhost();
			m_NumGhostShots = 0;
		}
		else if (m_NumGhostShots == 2 && m_pPlayer->m_SpookyGhost)
		{
			UnsetSpookyGhost();
			m_NumGhostShots = 0;
		}
	}

	// check for ammo
	if(!m_aWeapons[GetActiveWeapon()].m_Ammo)
	{
		if (m_FreezeTime)
		{
			if (m_PainSoundTimer <= 0)
			{
				m_PainSoundTimer = 1 * Server()->TickSpeed();
				GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));
			}
		}
		else
		{
			// 125ms is a magical limit of how fast a human can click
			m_ReloadTimer = 125 * Server()->TickSpeed() / 1000;
			GameServer()->CreateSound(m_Pos, SOUND_WEAPON_NOAMMO);
			// Timer stuff to avoid shrieking orchestra caused by unfreeze-plasma
		}
		return;
	}

	// BlockDDrace
	vec2 ProjStartPos = m_Pos+TempDirection*m_ProximityRadius*0.75f;
	float Spread[] = { 0, -0.1f, 0.1f, -0.2f, 0.2f, -0.3f, 0.3f, -0.4f, 0.4f };
	int NumShots = m_aSpreadWeapon[GetActiveWeapon()] ? g_Config.m_SvNumSpreadShots : 1;
	if ((GetActiveWeapon() == WEAPON_SHOTGUN && m_pPlayer->m_Gamemode == GAMEMODE_VANILLA) || GetActiveWeapon() == WEAPON_TELEKINESIS || GetActiveWeapon() == WEAPON_LIGHTSABER)
		NumShots = 1;
	bool Sound = true;

	for (int i = 0; i < NumShots; i++)
	{
		float Angle = GetAngle(TempDirection);
		Angle += Spread[i];
		vec2 Direction = vec2(cosf(Angle), sinf(Angle));

		switch (GetActiveWeapon())
		{
		case WEAPON_HAMMER:
		{
			// reset objects Hit
			m_NumObjectsHit = 0;
			if (Sound)
				GameServer()->CreateSound(m_Pos, SOUND_HAMMER_FIRE, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));

			if (m_Hit&DISABLE_HIT_HAMMER) break;

			CCharacter *apEnts[MAX_CLIENTS];
			int Hits = 0;
			int Num = GameWorld()->FindEntities(ProjStartPos, m_ProximityRadius*0.5f, (CEntity**)apEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

			for (int i = 0; i < Num; ++i)
			{
				CCharacter *pTarget = apEnts[i];

				if (pTarget == this || !CanCollide(pTarget->GetPlayer()->GetCID()))
					continue;

				// set his velocity to fast upward (for now)
				if (length(pTarget->m_Pos - ProjStartPos) > 0.0f)
					GameServer()->CreateHammerHit(pTarget->m_Pos - normalize(pTarget->m_Pos - ProjStartPos)*m_ProximityRadius*0.5f, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));
				else
					GameServer()->CreateHammerHit(ProjStartPos, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));

				vec2 Dir;
				if (length(pTarget->m_Pos - m_Pos) > 0.0f)
					Dir = normalize(pTarget->m_Pos - m_Pos);
				else
					Dir = vec2(0.f, -1.f);

				float Strength;
				if (!m_TuneZone)
					Strength = GameServer()->Tuning()->m_HammerStrength;
				else
					Strength = GameServer()->TuningList()[m_TuneZone].m_HammerStrength;

				vec2 Temp = pTarget->m_Core.m_Vel + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f;
				if (Temp.x > 0 && ((pTarget->m_TileIndex == TILE_STOP && pTarget->m_TileFlags == ROTATION_270) || (pTarget->m_TileIndexL == TILE_STOP && pTarget->m_TileFlagsL == ROTATION_270) || (pTarget->m_TileIndexL == TILE_STOPS && (pTarget->m_TileFlagsL == ROTATION_90 || pTarget->m_TileFlagsL == ROTATION_270)) || (pTarget->m_TileIndexL == TILE_STOPA) || (pTarget->m_TileFIndex == TILE_STOP && pTarget->m_TileFFlags == ROTATION_270) || (pTarget->m_TileFIndexL == TILE_STOP && pTarget->m_TileFFlagsL == ROTATION_270) || (pTarget->m_TileFIndexL == TILE_STOPS && (pTarget->m_TileFFlagsL == ROTATION_90 || pTarget->m_TileFFlagsL == ROTATION_270)) || (pTarget->m_TileFIndexL == TILE_STOPA) || (pTarget->m_TileSIndex == TILE_STOP && pTarget->m_TileSFlags == ROTATION_270) || (pTarget->m_TileSIndexL == TILE_STOP && pTarget->m_TileSFlagsL == ROTATION_270) || (pTarget->m_TileSIndexL == TILE_STOPS && (pTarget->m_TileSFlagsL == ROTATION_90 || pTarget->m_TileSFlagsL == ROTATION_270)) || (pTarget->m_TileSIndexL == TILE_STOPA)))
					Temp.x = 0;
				if (Temp.x < 0 && ((pTarget->m_TileIndex == TILE_STOP && pTarget->m_TileFlags == ROTATION_90) || (pTarget->m_TileIndexR == TILE_STOP && pTarget->m_TileFlagsR == ROTATION_90) || (pTarget->m_TileIndexR == TILE_STOPS && (pTarget->m_TileFlagsR == ROTATION_90 || pTarget->m_TileFlagsR == ROTATION_270)) || (pTarget->m_TileIndexR == TILE_STOPA) || (pTarget->m_TileFIndex == TILE_STOP && pTarget->m_TileFFlags == ROTATION_90) || (pTarget->m_TileFIndexR == TILE_STOP && pTarget->m_TileFFlagsR == ROTATION_90) || (pTarget->m_TileFIndexR == TILE_STOPS && (pTarget->m_TileFFlagsR == ROTATION_90 || pTarget->m_TileFFlagsR == ROTATION_270)) || (pTarget->m_TileFIndexR == TILE_STOPA) || (pTarget->m_TileSIndex == TILE_STOP && pTarget->m_TileSFlags == ROTATION_90) || (pTarget->m_TileSIndexR == TILE_STOP && pTarget->m_TileSFlagsR == ROTATION_90) || (pTarget->m_TileSIndexR == TILE_STOPS && (pTarget->m_TileSFlagsR == ROTATION_90 || pTarget->m_TileSFlagsR == ROTATION_270)) || (pTarget->m_TileSIndexR == TILE_STOPA)))
					Temp.x = 0;
				if (Temp.y < 0 && ((pTarget->m_TileIndex == TILE_STOP && pTarget->m_TileFlags == ROTATION_180) || (pTarget->m_TileIndexB == TILE_STOP && pTarget->m_TileFlagsB == ROTATION_180) || (pTarget->m_TileIndexB == TILE_STOPS && (pTarget->m_TileFlagsB == ROTATION_0 || pTarget->m_TileFlagsB == ROTATION_180)) || (pTarget->m_TileIndexB == TILE_STOPA) || (pTarget->m_TileFIndex == TILE_STOP && pTarget->m_TileFFlags == ROTATION_180) || (pTarget->m_TileFIndexB == TILE_STOP && pTarget->m_TileFFlagsB == ROTATION_180) || (pTarget->m_TileFIndexB == TILE_STOPS && (pTarget->m_TileFFlagsB == ROTATION_0 || pTarget->m_TileFFlagsB == ROTATION_180)) || (pTarget->m_TileFIndexB == TILE_STOPA) || (pTarget->m_TileSIndex == TILE_STOP && pTarget->m_TileSFlags == ROTATION_180) || (pTarget->m_TileSIndexB == TILE_STOP && pTarget->m_TileSFlagsB == ROTATION_180) || (pTarget->m_TileSIndexB == TILE_STOPS && (pTarget->m_TileSFlagsB == ROTATION_0 || pTarget->m_TileSFlagsB == ROTATION_180)) || (pTarget->m_TileSIndexB == TILE_STOPA)))
					Temp.y = 0;
				if (Temp.y > 0 && ((pTarget->m_TileIndex == TILE_STOP && pTarget->m_TileFlags == ROTATION_0) || (pTarget->m_TileIndexT == TILE_STOP && pTarget->m_TileFlagsT == ROTATION_0) || (pTarget->m_TileIndexT == TILE_STOPS && (pTarget->m_TileFlagsT == ROTATION_0 || pTarget->m_TileFlagsT == ROTATION_180)) || (pTarget->m_TileIndexT == TILE_STOPA) || (pTarget->m_TileFIndex == TILE_STOP && pTarget->m_TileFFlags == ROTATION_0) || (pTarget->m_TileFIndexT == TILE_STOP && pTarget->m_TileFFlagsT == ROTATION_0) || (pTarget->m_TileFIndexT == TILE_STOPS && (pTarget->m_TileFFlagsT == ROTATION_0 || pTarget->m_TileFFlagsT == ROTATION_180)) || (pTarget->m_TileFIndexT == TILE_STOPA) || (pTarget->m_TileSIndex == TILE_STOP && pTarget->m_TileSFlags == ROTATION_0) || (pTarget->m_TileSIndexT == TILE_STOP && pTarget->m_TileSFlagsT == ROTATION_0) || (pTarget->m_TileSIndexT == TILE_STOPS && (pTarget->m_TileSFlagsT == ROTATION_0 || pTarget->m_TileSFlagsT == ROTATION_180)) || (pTarget->m_TileSIndexT == TILE_STOPA)))
					Temp.y = 0;
				Temp -= pTarget->m_Core.m_Vel;

				pTarget->UnFreeze();
				pTarget->TakeDamage((vec2(0.f, -1.0f) + Temp) * Strength, g_pData->m_Weapons.m_Hammer.m_pBase->m_Damage,
					m_pPlayer->GetCID(), GetActiveWeapon());

				if (m_FreezeHammer)
					pTarget->Freeze();

				Hits++;
			}

			// if we Hit anything, we have to wait for the reload
			if (Hits)
				m_ReloadTimer = Server()->TickSpeed() / 3;

		} break;

		case WEAPON_GUN:
		{
			if (!m_Jetpack || !m_pPlayer->m_NinjaJetpack)
			{
				int Lifetime;
				if (!m_TuneZone)
					Lifetime = (int)(Server()->TickSpeed() * (m_pPlayer->m_Gamemode == GAMEMODE_VANILLA ? GameServer()->Tuning()->m_VanillaGunLifetime : GameServer()->Tuning()->m_GunLifetime));
				else
					Lifetime = (int)(Server()->TickSpeed() * (m_pPlayer->m_Gamemode == GAMEMODE_VANILLA ? GameServer()->TuningList()[m_TuneZone].m_VanillaGunLifetime : GameServer()->TuningList()[m_TuneZone].m_GunLifetime));

				CProjectile *pProj = new CProjectile
				(
					GameWorld(),
					WEAPON_GUN,//Type
					m_pPlayer->GetCID(),//Owner
					ProjStartPos,//Pos
					Direction,//Dir
					Lifetime,//Span
					0,//Freeze
					0,//Explosive
					0,//Force
					-1,//SoundImpact
					0,
					0,
					m_pPlayer->m_SpookyGhost,
					m_pPlayer->m_Gamemode == GAMEMODE_VANILLA
				);

				// pack the Projectile and send it to the client Directly
				CNetObj_Projectile p;
				pProj->FillInfo(&p);

				CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
				Msg.AddInt(1);
				for (unsigned i = 0; i < sizeof(CNetObj_Projectile) / sizeof(int); i++)
					Msg.AddInt(((int *)&p)[i]);

				Server()->SendMsg(&Msg, MSGFLAG_VITAL, m_pPlayer->GetCID());
				if (Sound)
					GameServer()->CreateSound(m_Pos, (m_Jetpack && !g_Config.m_SvOldJetpackSound) ? SOUND_HOOK_LOOP : SOUND_GUN_FIRE, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));
			}
		} break;

		case WEAPON_SHOTGUN:
		{
			if (m_pPlayer->m_Gamemode == GAMEMODE_VANILLA)
			{
				int ShotSpread = 2;

				CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
				Msg.AddInt(ShotSpread * 2 + 1);

				for (int i = -ShotSpread; i <= ShotSpread; ++i)
				{
					float Spreading[] = { -0.185f, -0.070f, 0, 0.070f, 0.185f };
					float a = GetAngle(Direction);
					a += Spreading[i + 2];
					float v = 1 - (absolute(i) / (float)ShotSpread);
					float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
					CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_SHOTGUN,
						m_pPlayer->GetCID(),
						ProjStartPos,
						vec2(cosf(a), sinf(a))*Speed,
						(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_ShotgunLifetime),
						0, 0, 0, -1);

					// pack the Projectile and send it to the client Directly
					CNetObj_Projectile p;
					pProj->FillInfo(&p);

					for (unsigned i = 0; i < sizeof(CNetObj_Projectile) / sizeof(int); i++)
						Msg.AddInt(((int *)&p)[i]);
				}

				Server()->SendMsg(&Msg, MSGFLAG_VITAL, m_pPlayer->GetCID());
			}
			else
			{
				float LaserReach;
				if (!m_TuneZone)
					LaserReach = GameServer()->Tuning()->m_LaserReach;
				else
					LaserReach = GameServer()->TuningList()[m_TuneZone].m_LaserReach;

				new CLaser(GameWorld(), m_Pos, Direction, LaserReach, m_pPlayer->GetCID(), WEAPON_SHOTGUN);
			}
			if (Sound)
				GameServer()->CreateSound(m_Pos, SOUND_SHOTGUN_FIRE, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));
		} break;

		case WEAPON_STRAIGHT_GRENADE:
		case WEAPON_GRENADE:
		{
			int Lifetime;
			if (!m_TuneZone)
				Lifetime = (int)(Server()->TickSpeed() * (GetActiveWeapon() == WEAPON_STRAIGHT_GRENADE ? GameServer()->Tuning()->m_StraightGrenadeLifetime : GameServer()->Tuning()->m_GrenadeLifetime));
			else
				Lifetime = (int)(Server()->TickSpeed() * (GetActiveWeapon() == WEAPON_STRAIGHT_GRENADE ? GameServer()->TuningList()[m_TuneZone].m_StraightGrenadeLifetime : GameServer()->TuningList()[m_TuneZone].m_GrenadeLifetime));

			CProjectile *pProj = new CProjectile
			(
				GameWorld(),
				GetActiveWeapon(),//Type
				m_pPlayer->GetCID(),//Owner
				ProjStartPos,//Pos
				Direction,//Dir
				Lifetime,//Span
				0,//Freeze
				true,//Explosive
				0,//Force
				SOUND_GRENADE_EXPLODE,//SoundImpact
				0,//Layer
				0,//Number
				false,//Spooky
				GetActiveWeapon() == WEAPON_STRAIGHT_GRENADE//FakeTuning
			);

			// pack the Projectile and send it to the client Directly
			CNetObj_Projectile p;
			pProj->FillInfo(&p);

			CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
			Msg.AddInt(1);
			for (unsigned i = 0; i < sizeof(CNetObj_Projectile) / sizeof(int); i++)
				Msg.AddInt(((int *)&p)[i]);
			Server()->SendMsg(&Msg, MSGFLAG_VITAL, m_pPlayer->GetCID());

			if (Sound)
				GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));
		} break;

		case WEAPON_RIFLE:
		{
			float LaserReach;
			if (!m_TuneZone)
				LaserReach = GameServer()->Tuning()->m_LaserReach;
			else
				LaserReach = GameServer()->TuningList()[m_TuneZone].m_LaserReach;

			new CLaser(GameWorld(), m_Pos, Direction, LaserReach, m_pPlayer->GetCID(), WEAPON_RIFLE);
			if (Sound)
				GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));
		} break;

		case WEAPON_NINJA:
		{
			// reset Hit objects
			m_NumObjectsHit = 0;

			m_Ninja.m_ActivationDir = Direction;
			m_Ninja.m_CurrentMoveTime = g_pData->m_Weapons.m_Ninja.m_Movetime * Server()->TickSpeed() / 1000;
			m_Ninja.m_OldVelAmount = length(m_Core.m_Vel);

			if (Sound)
				GameServer()->CreateSound(m_Pos, SOUND_NINJA_FIRE, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));
		} break;

		case WEAPON_PLASMA_RIFLE:
		{
			new CCustomProjectile
			(
				GameWorld(),
				m_pPlayer->GetCID(),	//owner
				ProjStartPos,			//pos
				Direction,				//dir
				0,						//freeze
				1,						//explosive
				1,						//unfreeze
				0,						//bloody
				0,						//ghost
				0,						//spooky
				WEAPON_PLASMA_RIFLE,	//type
				6,						//lifetime
				1.0f,					//accel
				10.0f					//speed
			);
			if (Sound)
				GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));
		} break;

		case WEAPON_HEART_GUN:
		{
			new CCustomProjectile
			(
				GameWorld(),
				m_pPlayer->GetCID(),	//owner
				ProjStartPos,			//pos
				Direction,				//dir
				0,						//freeze
				0,						//explosive
				0,						//unfreeze
				m_pPlayer->m_SpookyGhost,//bloody
				m_pPlayer->m_SpookyGhost,//ghost
				m_pPlayer->m_SpookyGhost,//spooky
				WEAPON_HEART_GUN,		//type
				6,						//lifetime
				1.0f,					//accel
				10.0f					//speed
			);
			if (Sound)
				GameServer()->CreateSound(m_Pos, SOUND_PICKUP_HEALTH, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));
		} break;

		case WEAPON_TELEKINESIS:
		{
			bool TelekinesisSound = false;

			if (m_TelekinesisTee == -1)
			{
				vec2 CursorPos = vec2(m_Pos.x + m_Input.m_TargetX, m_Pos.y + m_Input.m_TargetY);
				CCharacter *pChr = GameWorld()->ClosestCharacter(CursorPos, 20.f, this, m_pPlayer->GetCID());
				if (pChr && pChr->GetPlayer()->GetCID() != m_pPlayer->GetCID() && pChr->m_TelekinesisTee != m_pPlayer->GetCID())
				{
					bool IsTelekinesed = false;
					for (int i = 0; i < MAX_CLIENTS; i++)
						if (GameServer()->GetPlayerChar(i) && GameServer()->GetPlayerChar(i)->m_TelekinesisTee == pChr->GetPlayer()->GetCID())
						{
							IsTelekinesed = true;
							break;
						}

					if (!IsTelekinesed)
					{
						m_TelekinesisTee = pChr->GetPlayer()->GetCID();
						TelekinesisSound = true;
					}
				}
			}
			else if (m_TelekinesisTee != -1)
			{
				m_TelekinesisTee = -1;
				TelekinesisSound = true;
			}

			if (Sound && TelekinesisSound)
				GameServer()->CreateSound(m_Pos, SOUND_NINJA_HIT, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));
		} break;

		case WEAPON_LIGHTSABER:
		{
			if (!m_pLightsaber)
			{
				m_pLightsaber = new CLightsaber(GameWorld(), m_pPlayer->GetCID());
				m_pLightsaber->Extend();
			}
			else
			{
				m_pLightsaber->Retract();
			}
		} break;
		}

		Sound = false;
	}

	if (GetActiveWeapon() != WEAPON_LIGHTSABER) // we don't want the client to render the fire animation
		m_AttackTick = Server()->Tick();

	if(m_aWeapons[GetActiveWeapon()].m_Ammo > 0) // -1 == unlimited
		m_aWeapons[GetActiveWeapon()].m_Ammo--;

	if(!m_ReloadTimer)
	{
		float FireDelay;
		if (!m_TuneZone)
			GameServer()->Tuning()->Get(OLD_TUNES + GetActiveWeapon(), &FireDelay);
		else
			GameServer()->TuningList()[m_TuneZone].Get(OLD_TUNES + GetActiveWeapon(), &FireDelay);
		m_ReloadTimer = FireDelay * Server()->TickSpeed() / 1000;
	}
}

void CCharacter::HandleWeapons()
{
	//ninja
	HandleNinja();
	HandleJetpack();

	if(m_PainSoundTimer > 0)
		m_PainSoundTimer--;

	// check reload timer
	if(m_ReloadTimer)
	{
		m_ReloadTimer--;
		return;
	}

	// fire Weapon, if wanted
	FireWeapon();

	// ammo regen
	int AmmoRegenTime = g_pData->m_Weapons.m_aId[GetActiveWeapon()].m_Ammoregentime;
	if (GetActiveWeapon() == WEAPON_HEART_GUN)
		AmmoRegenTime = g_pData->m_Weapons.m_aId[WEAPON_GUN].m_Ammoregentime;
	if(AmmoRegenTime && m_pPlayer->m_Gamemode == GAMEMODE_VANILLA && !m_FreezeTime && m_aWeapons[GetActiveWeapon()].m_Ammo != -1)
	{
		// If equipped and not active, regen ammo?
		if (m_ReloadTimer <= 0)
		{
			if (m_aWeapons[GetActiveWeapon()].m_AmmoRegenStart < 0)
				m_aWeapons[GetActiveWeapon()].m_AmmoRegenStart = Server()->Tick();

			if ((Server()->Tick() - m_aWeapons[GetActiveWeapon()].m_AmmoRegenStart) >= AmmoRegenTime * Server()->TickSpeed() / 1000)
			{
				// Add some ammo
				m_aWeapons[GetActiveWeapon()].m_Ammo = minimum(m_aWeapons[GetActiveWeapon()].m_Ammo + 1, 10);
				m_aWeapons[GetActiveWeapon()].m_AmmoRegenStart = -1;
			}
		}
		else
		{
			m_aWeapons[GetActiveWeapon()].m_AmmoRegenStart = -1;
		}
	}

	return;
}

void CCharacter::GiveNinja()
{
	if (!m_ScrollNinja && !m_aWeapons[WEAPON_NINJA].m_Got && m_pPlayer->m_Gamemode == GAMEMODE_VANILLA)
		GameServer()->CreateSound(m_Pos, SOUND_PICKUP_NINJA, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));

	for (int i = 0; i < NUM_BACKUPS; i++)
	{
		m_aWeaponsBackupGot[WEAPON_NINJA][i] = true;
		m_aWeaponsBackup[WEAPON_NINJA][i] = -1;
	}

	if (!m_FreezeTime)
		m_aWeapons[WEAPON_NINJA].m_Ammo = -1;

	m_aWeapons[WEAPON_NINJA].m_Got = true;

	if (m_ScrollNinja)
		return;

	m_Ninja.m_ActivationTick = Server()->Tick();
	if (GetActiveWeapon() != WEAPON_NINJA)
		m_LastWeapon = GetActiveWeapon();
	SetActiveWeapon(WEAPON_NINJA);
}

void CCharacter::RemoveNinja()
{
	m_Ninja.m_CurrentMoveTime = 0;
	if (GetActiveWeapon() == WEAPON_NINJA)
		SetWeapon(m_LastWeapon);
	m_aWeapons[WEAPON_NINJA].m_Got = false;
	for (int i = 0; i < NUM_BACKUPS; i++)
		m_aWeaponsBackupGot[WEAPON_NINJA][i] = false;
}

void CCharacter::SetEmote(int Emote, int Tick)
{
	m_EmoteType = Emote;
	m_EmoteStop = Tick;
}

void CCharacter::OnPredictedInput(CNetObj_PlayerInput *pNewInput)
{
	// check for changes
	if(mem_comp(&m_SavedInput, pNewInput, sizeof(CNetObj_PlayerInput)) != 0)
		m_LastAction = Server()->Tick();

	// copy new input
	mem_copy(&m_Input, pNewInput, sizeof(m_Input));
	m_NumInputs++;

	// it is not allowed to aim in the center
	if(m_Input.m_TargetX == 0 && m_Input.m_TargetY == 0)
		m_Input.m_TargetY = -1;

	mem_copy(&m_SavedInput, &m_Input, sizeof(m_SavedInput));
}

void CCharacter::OnDirectInput(CNetObj_PlayerInput *pNewInput)
{
	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
	mem_copy(&m_LatestInput, pNewInput, sizeof(m_LatestInput));

	// it is not allowed to aim in the center
	if(m_LatestInput.m_TargetX == 0 && m_LatestInput.m_TargetY == 0)
		m_LatestInput.m_TargetY = -1;

	if(m_NumInputs > 2 && m_pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		HandleWeaponSwitch();
		FireWeapon();
	}

	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
}

void CCharacter::ResetInput()
{
	m_Input.m_Direction = 0;
	//m_Input.m_Hook = 0;
	// simulate releasing the fire button
	if((m_Input.m_Fire&1) != 0)
		m_Input.m_Fire++;
	m_Input.m_Fire &= INPUT_STATE_MASK;
	m_Input.m_Jump = 0;
	m_LatestPrevInput = m_LatestInput = m_Input;
}

void CCharacter::Tick()
{
	if (m_Paused)
		return;

	// BlockDDrace
	BlockDDraceTick();
	DummyTick();
	// BlockDDrace

	DDRaceTick();

	// BlockDDrace
	FakeBlockTick();
	// BlockDDrace

	m_Core.m_Input = m_Input;

	// BlockDDrace
	for (int i = 0; i < 2; i++)
	{
		bool Carried = true;
		CFlag *F = ((CGameControllerBlockDDrace*)GameServer()->m_pController)->m_apFlags[i];
		if (!F)
			continue;

		if (F->GetCarrier() == NULL)
			Carried = false;
		m_Core.SetFlagPos(i, F->m_Pos, F->IsAtStand(), F->GetVel(), Carried);
	}
	// BlockDDrace

	m_Core.Tick(true);

	// BlockDDrace
	if (m_Core.m_UpdateFlagVel == FLAG_RED)
		((CGameControllerBlockDDrace*)GameServer()->m_pController)->m_apFlags[TEAM_RED]->SetVel(m_Core.m_UFlagVel);
	else if (m_Core.m_UpdateFlagVel == FLAG_BLUE)
		((CGameControllerBlockDDrace*)GameServer()->m_pController)->m_apFlags[TEAM_BLUE]->SetVel(m_Core.m_UFlagVel);
	// BlockDDrace

	// handle Weapons
	HandleWeapons();

	DDRacePostCoreTick();
	
	// Previnput
	m_PrevInput = m_Input;

	m_PrevPos = m_Core.m_Pos;
	return;
}

void CCharacter::TickDefered()
{
	// advance the dummy
	{
		CWorldCore TempWorld;
		m_ReckoningCore.Init(&TempWorld, GameServer()->Collision(), &((CGameControllerBlockDDrace*)GameServer()->m_pController)->m_Teams.m_Core, &((CGameControllerBlockDDrace*)GameServer()->m_pController)->m_TeleOuts);
		m_ReckoningCore.m_Id = m_pPlayer->GetCID();
		m_ReckoningCore.Tick(false);
		m_ReckoningCore.Move();
		m_ReckoningCore.Quantize();
	}

	//lastsentcore
	vec2 StartPos = m_Core.m_Pos;
	vec2 StartVel = m_Core.m_Vel;
	bool StuckBefore = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));

	m_Core.m_Id = m_pPlayer->GetCID();
	m_Core.Move();
	bool StuckAfterMove = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	m_Core.Quantize();
	bool StuckAfterQuant = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	m_Pos = m_Core.m_Pos;

	if(!StuckBefore && (StuckAfterMove || StuckAfterQuant))
	{
		// Hackish solution to get rid of strict-aliasing warning
		union
		{
			float f;
			unsigned u;
		}StartPosX, StartPosY, StartVelX, StartVelY;

		StartPosX.f = StartPos.x;
		StartPosY.f = StartPos.y;
		StartVelX.f = StartVel.x;
		StartVelY.f = StartVel.y;

		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "STUCK!!! %d %d %d %f %f %f %f %x %x %x %x",
			StuckBefore,
			StuckAfterMove,
			StuckAfterQuant,
			StartPos.x, StartPos.y,
			StartVel.x, StartVel.y,
			StartPosX.u, StartPosY.u,
			StartVelX.u, StartVelY.u);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	}

	int Events = m_Core.m_TriggeredEvents;
	//int Mask = CmaskAllExceptOne(m_pPlayer->GetCID());

	if(Events&COREEVENT_GROUND_JUMP) GameServer()->CreateSound(m_Pos, SOUND_PLAYER_JUMP, Teams()->TeamMask(Team(), m_pPlayer->GetCID()));

	if(Events&COREEVENT_HOOK_ATTACH_PLAYER) GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_PLAYER, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));
	if(Events&COREEVENT_HOOK_ATTACH_GROUND) GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_GROUND, Teams()->TeamMask(Team(), m_pPlayer->GetCID(), m_pPlayer->GetCID()));
	if(Events&COREEVENT_HOOK_HIT_NOHOOK) GameServer()->CreateSound(m_Pos, SOUND_HOOK_NOATTACH, Teams()->TeamMask(Team(), m_pPlayer->GetCID(), m_pPlayer->GetCID()));


	if(m_pPlayer->GetTeam() == TEAM_SPECTATORS)
	{
		m_Pos.x = m_Input.m_TargetX;
		m_Pos.y = m_Input.m_TargetY;
	}

	// update the m_SendCore if needed
	{
		CNetObj_Character Predicted;
		CNetObj_Character Current;
		mem_zero(&Predicted, sizeof(Predicted));
		mem_zero(&Current, sizeof(Current));
		m_ReckoningCore.Write(&Predicted);
		m_Core.Write(&Current);

		// only allow dead reackoning for a top of 3 seconds
		if(m_Core.m_pReset || m_ReckoningTick+Server()->TickSpeed()*3 < Server()->Tick() || mem_comp(&Predicted, &Current, sizeof(CNetObj_Character)) != 0)
		{
			m_ReckoningTick = Server()->Tick();
			m_SendCore = m_Core;
			m_ReckoningCore = m_Core;
			m_Core.m_pReset = false;
		}
	}
}

void CCharacter::TickPaused()
{
	++m_AttackTick;
	++m_DamageTakenTick;
	++m_Ninja.m_ActivationTick;
	++m_ReckoningTick;
	if(m_LastAction != -1)
		++m_LastAction;
	if(m_aWeapons[GetActiveWeapon()].m_AmmoRegenStart > -1)
		++m_aWeapons[GetActiveWeapon()].m_AmmoRegenStart;
	if(m_EmoteStop > -1)
		++m_EmoteStop;
}

bool CCharacter::IncreaseHealth(int Amount)
{
	if(m_Health >= 10)
		return false;
	m_Health = clamp(m_Health+Amount, 0, 10);
	return true;
}

bool CCharacter::IncreaseArmor(int Amount)
{
	// BlockDDrace
	if (m_FreezeTime && m_aWeaponsBackup[BACKUP_ARMOR][BACKUP_FREEZE] >= 10)
		return false;
	if (m_Armor >= 10)
		return false;

	if (m_FreezeTime)
		m_aWeaponsBackup[BACKUP_ARMOR][BACKUP_FREEZE] = clamp(m_aWeaponsBackup[BACKUP_ARMOR][BACKUP_FREEZE] + Amount, 0, 10);
	else
		m_Armor = clamp(m_Armor + Amount, 0, 10);
	return true;
}

void CCharacter::Die(int Killer, int Weapon)
{
	if (Server()->IsRecording(m_pPlayer->GetCID()))
		Server()->StopRecord(m_pPlayer->GetCID());

	/*************************************************
	*                                                *
	*              B L O C K D D R A C E             *
	*                                                *
	**************************************************/

	// drop armor, hearts and weapons
	DropLoot();

	// only let unfrozen kills through if you have damage activated
	if (!m_FreezeTime && (Killer < 0 || Killer == m_pPlayer->GetCID() || Weapon < 0))
	{
		m_Killer.m_ClientID = -1;
		m_Killer.m_Weapon = -1;
	}

	// if no killer exists its a selfkill
	if (m_Killer.m_ClientID == -1)
		m_Killer.m_ClientID = m_pPlayer->GetCID();

	// set the new killer and weapon
	Killer = m_Killer.m_ClientID;
	if (Weapon >= 0)
		Weapon = m_Killer.m_Weapon;


	CPlayer *pKiller = (Killer >= 0 && Killer != m_pPlayer->GetCID()) ? GameServer()->m_apPlayers[Killer] : 0;

	// account kills and deaths
	if (pKiller)
	{
		if (pKiller->m_Minigame == MINIGAME_SURVIVAL)
			GameServer()->m_Accounts[pKiller->GetAccID()].m_SurvivalKills++;
		else
			GameServer()->m_Accounts[pKiller->GetAccID()].m_Kills++;

		GameServer()->m_Accounts[m_pPlayer->GetAccID()].m_Deaths++;
	}
	
	/*************************************************
	*                                                *
	*              B L O C K D D R A C E             *
	*                                                *
	**************************************************/

	int ModeSpecial = GameServer()->m_pController->OnCharacterDeath(this, GameServer()->m_apPlayers[Killer], Weapon);

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "kill killer='%d:%s' victim='%d:%s' weapon=%d special=%d",
		Killer, Server()->ClientName(Killer),
		m_pPlayer->GetCID(), Server()->ClientName(m_pPlayer->GetCID()), Weapon, ModeSpecial);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	// BlockDDrace

	// send the kill message
	if (!m_pPlayer->m_ShowName || (pKiller && !pKiller->m_ShowName))
	{
		if (m_pPlayer->m_SpookyGhost)
			m_pPlayer->m_RespawnTick = Server()->Tick() + Server()->TickSpeed() / 10;

		if (pKiller && !pKiller->m_ShowName)
			pKiller->FixForNoName(FIX_SET_NAME_ONLY);

		m_pPlayer->m_MsgKiller = Killer;
		m_pPlayer->m_MsgWeapon = GameServer()->GetRealWeapon(m_Killer.m_Weapon);
		m_pPlayer->m_MsgModeSpecial = ModeSpecial;
		m_pPlayer->FixForNoName(FIX_KILL_MSG);
	} // BlockDDrace
	else
	{
		CNetMsg_Sv_KillMsg Msg;
		Msg.m_Killer = Killer;
		Msg.m_Victim = m_pPlayer->GetCID();
		Msg.m_Weapon = GameServer()->GetRealWeapon(m_Killer.m_Weapon);
		Msg.m_ModeSpecial = ModeSpecial;
		// BlockDDrace // only send kill message to players in the same minigame
		for (int i = 0; i < MAX_CLIENTS; i++)
			if (!g_Config.m_SvHideMinigamePlayers || (GameServer()->m_apPlayers[i] && m_pPlayer->m_Minigame == GameServer()->m_apPlayers[i]->m_Minigame))
				Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
	}

	// BlockDDrace
	((CGameControllerBlockDDrace*)GameServer()->m_pController)->ChangeFlagOwner(this, GameServer()->GetPlayerChar(Killer));

	// character doesnt exist, print some messages and set states
	// if the player is in deathmatch mode, or simply playing
	if (GameServer()->m_SurvivalGameState > SURVIVAL_LOBBY && m_pPlayer->m_SurvivalState > SURVIVAL_LOBBY && !m_pPlayer->m_ForceKilled)
	{
		// check for players in the current game state
		if (m_pPlayer->GetCID() != GameServer()->m_SurvivalWinner)
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "You lost, you can wait for another round or leave the lobby using '/leave'");
		if (GameServer()->CountSurvivalPlayers(GameServer()->m_SurvivalGameState) > 2)
		{
			// if there are more than just two players left, you will watch your killer or a random player
			m_pPlayer->m_SpectatorID = pKiller ? Killer : GameServer()->GetRandomSurvivalPlayer(GameServer()->m_SurvivalGameState, m_pPlayer->GetCID());
			m_pPlayer->Pause(CPlayer::PAUSE_PAUSED, true);

			// printing a message that you died and informing about remaining players
			char aKillMsg[128];
			str_format(aKillMsg, sizeof(aKillMsg), "'%s' died\nAlive players: %d", Server()->ClientName(m_pPlayer->GetCID()), GameServer()->CountSurvivalPlayers(GameServer()->m_SurvivalGameState) -1 /* -1 because we have to exclude the currently dying*/);
			GameServer()->SendSurvivalBroadcast(aKillMsg);
		}
		// sending you back to lobby
		m_pPlayer->m_SurvivalState = SURVIVAL_LOBBY;
	}

	// a nice sound
	GameServer()->CreateSound(m_Pos, SOUND_PLAYER_DIE, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));

	// this is for auto respawn after 3 secs
	m_pPlayer->m_DieTick = Server()->Tick();

	m_Alive = false;
	m_Solo = false;
	m_pPlayer->m_ForceKilled = false;

	GameWorld()->RemoveEntity(this);
	GameWorld()->m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCID(), Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));
	Teams()->OnCharacterDeath(m_pPlayer->GetCID(), Weapon);
}

bool CCharacter::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	/*************************************************
	*                                                *
	*              B L O C K D D R A C E             *
	*                                                *
	**************************************************/

	if (GameServer()->m_apPlayers[From] && From != m_pPlayer->GetCID())
	{
		m_Killer.m_ClientID = From;
		m_Killer.m_Weapon = Weapon;
	}

	if (m_pPlayer->m_Gamemode == GAMEMODE_VANILLA || Weapon == WEAPON_LIGHTSABER)
	{
		// m_pPlayer only inflicts half damage on self
		if(From == m_pPlayer->GetCID())
			Dmg = maximum(1, Dmg/2);

		m_DamageTaken++;

		// create healthmod indicator
		if(Server()->Tick() < m_DamageTakenTick+25)
		{
			// make sure that the damage indicators doesn't group together
			GameServer()->CreateDamageInd(m_Pos, m_DamageTaken*0.25f, Dmg);
		}
		else
		{
			m_DamageTaken = 0;
			GameServer()->CreateDamageInd(m_Pos, 0, Dmg);
		}

		if(Dmg)
		{
			if(m_Armor && m_pPlayer->m_Gamemode == GAMEMODE_VANILLA)
			{
				if(Dmg > 1)
				{
					m_Health--;
					Dmg--;
				}

				if(Dmg > m_Armor)
				{
					Dmg -= m_Armor;
					m_Armor = 0;
				}
				else
				{
					m_Armor -= Dmg;
					Dmg = 0;
				}
			}

			m_Health -= Dmg;
		}

		m_DamageTakenTick = Server()->Tick();

		// do damage Hit sound
		if(From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
		{
			int64_t Mask = CmaskOne(From);
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_SpectatorID == From)
					Mask |= CmaskOne(i);
			}
			GameServer()->CreateSound(GameServer()->m_apPlayers[From]->m_ViewPos, SOUND_HIT, Mask);
		}

		// check for death
		if(m_Health <= 0)
		{
			Die(From, Weapon);

			// set attacker's face to happy (taunt!)
			if (From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
			{
				CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
				if (pChr)
				{
					pChr->m_EmoteType = EMOTE_HAPPY;
					pChr->m_EmoteStop = Server()->Tick() + Server()->TickSpeed();
				}
			}

			return false;
		}

		if (Dmg > 2)
			GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG);
		else
			GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT);
	}

	if (m_pPlayer->m_Gamemode == GAMEMODE_DDRACE && From != -1 && GameServer()->m_apPlayers[From] && GameServer()->m_apPlayers[From]->m_SpookyGhost)
	{
		// dont do emote pain if the shooter has spooky ghost
	}
	else if ((Dmg && m_pPlayer->m_Gamemode == GAMEMODE_VANILLA) || Weapon == WEAPON_HAMMER || Weapon == WEAPON_GRENADE || Weapon == WEAPON_STRAIGHT_GRENADE || Weapon == WEAPON_LIGHTSABER)
	{
		m_EmoteType = EMOTE_PAIN;
		m_EmoteStop = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;
	}

	vec2 Temp = m_Core.m_Vel + Force;
	if(Temp.x > 0 && ((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_270) || (m_TileIndexL == TILE_STOP && m_TileFlagsL == ROTATION_270) || (m_TileIndexL == TILE_STOPS && (m_TileFlagsL == ROTATION_90 || m_TileFlagsL ==ROTATION_270)) || (m_TileIndexL == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_270) || (m_TileFIndexL == TILE_STOP && m_TileFFlagsL == ROTATION_270) || (m_TileFIndexL == TILE_STOPS && (m_TileFFlagsL == ROTATION_90 || m_TileFFlagsL == ROTATION_270)) || (m_TileFIndexL == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_270) || (m_TileSIndexL == TILE_STOP && m_TileSFlagsL == ROTATION_270) || (m_TileSIndexL == TILE_STOPS && (m_TileSFlagsL == ROTATION_90 || m_TileSFlagsL == ROTATION_270)) || (m_TileSIndexL == TILE_STOPA)))
		Temp.x = 0;
	if(Temp.x < 0 && ((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_90) || (m_TileIndexR == TILE_STOP && m_TileFlagsR == ROTATION_90) || (m_TileIndexR == TILE_STOPS && (m_TileFlagsR == ROTATION_90 || m_TileFlagsR == ROTATION_270)) || (m_TileIndexR == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_90) || (m_TileFIndexR == TILE_STOP && m_TileFFlagsR == ROTATION_90) || (m_TileFIndexR == TILE_STOPS && (m_TileFFlagsR == ROTATION_90 || m_TileFFlagsR == ROTATION_270)) || (m_TileFIndexR == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_90) || (m_TileSIndexR == TILE_STOP && m_TileSFlagsR == ROTATION_90) || (m_TileSIndexR == TILE_STOPS && (m_TileSFlagsR == ROTATION_90 || m_TileSFlagsR == ROTATION_270)) || (m_TileSIndexR == TILE_STOPA)))
		Temp.x = 0;
	if(Temp.y < 0 && ((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_180) || (m_TileIndexB == TILE_STOP && m_TileFlagsB == ROTATION_180) || (m_TileIndexB == TILE_STOPS && (m_TileFlagsB == ROTATION_0 || m_TileFlagsB == ROTATION_180)) || (m_TileIndexB == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_180) || (m_TileFIndexB == TILE_STOP && m_TileFFlagsB == ROTATION_180) || (m_TileFIndexB == TILE_STOPS && (m_TileFFlagsB == ROTATION_0 || m_TileFFlagsB == ROTATION_180)) || (m_TileFIndexB == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_180) || (m_TileSIndexB == TILE_STOP && m_TileSFlagsB == ROTATION_180) || (m_TileSIndexB == TILE_STOPS && (m_TileSFlagsB == ROTATION_0 || m_TileSFlagsB == ROTATION_180)) || (m_TileSIndexB == TILE_STOPA)))
		Temp.y = 0;
	if(Temp.y > 0 && ((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_0) || (m_TileIndexT == TILE_STOP && m_TileFlagsT == ROTATION_0) || (m_TileIndexT == TILE_STOPS && (m_TileFlagsT == ROTATION_0 || m_TileFlagsT == ROTATION_180)) || (m_TileIndexT == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_0) || (m_TileFIndexT == TILE_STOP && m_TileFFlagsT == ROTATION_0) || (m_TileFIndexT == TILE_STOPS && (m_TileFFlagsT == ROTATION_0 || m_TileFFlagsT == ROTATION_180)) || (m_TileFIndexT == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_0) || (m_TileSIndexT == TILE_STOP && m_TileSFlagsT == ROTATION_0) || (m_TileSIndexT == TILE_STOPS && (m_TileSFlagsT == ROTATION_0 || m_TileSFlagsT == ROTATION_180)) || (m_TileSIndexT == TILE_STOPA)))
		Temp.y = 0;
	m_Core.m_Vel = Temp;

	return true;
}

void CCharacter::Snap(int SnappingClient)
{
	int id = m_pPlayer->GetCID();

	if(SnappingClient > -1 && !Server()->Translate(id, SnappingClient))
		return;

	if(NetworkClipped(SnappingClient))
		return;

	if(SnappingClient > -1)
	{
		CCharacter* SnapChar = GameServer()->GetPlayerChar(SnappingClient);
		CPlayer* SnapPlayer = GameServer()->m_apPlayers[SnappingClient];

		if((SnapPlayer->GetTeam() == TEAM_SPECTATORS || SnapPlayer->IsPaused()) && SnapPlayer->m_SpectatorID != -1
			&& !CanCollide(SnapPlayer->m_SpectatorID, false) && !SnapPlayer->m_ShowOthers)
			return;

		if( SnapPlayer->GetTeam() != TEAM_SPECTATORS && !SnapPlayer->IsPaused() && SnapChar && !SnapChar->m_Super
			&& !CanCollide(SnappingClient, false) && !SnapPlayer->m_ShowOthers)
			return;

		if((SnapPlayer->GetTeam() == TEAM_SPECTATORS || SnapPlayer->IsPaused()) && SnapPlayer->m_SpectatorID == -1
			&& !CanCollide(SnappingClient, false) && SnapPlayer->m_SpecTeam)
			return;

		// BlockDDrace
		if (m_Invisible && SnappingClient != m_pPlayer->GetCID())
			if (Server()->GetAuthedState(SnappingClient) < AUTHED_MOD || Server()->Tick() % 200 == 0)
				return;
	}

	if (m_Paused)
		return;

	CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, id, sizeof(CNetObj_Character)));
	if(!pCharacter)
		return;

	// BlockDDrace
	if (m_StrongBloody)
	{
		for (int i = 0; i < 3; i++)
			GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCID());
	}
	else if (m_Bloody || m_pPlayer->IsHooked(BLOODY))
	{
		if (Server()->Tick() % 3 == 0)
			GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCID());
	}
	// BlockDDrace

	// write down the m_Core
	if(!m_ReckoningTick || GameWorld()->m_Paused)
	{
		// no dead reckoning when paused because the client doesn't know
		// how far to perform the reckoning
		pCharacter->m_Tick = 0;
		m_Core.Write(pCharacter);
	}
	else
	{
		pCharacter->m_Tick = m_ReckoningTick;
		m_SendCore.Write(pCharacter);
	}

	// set emote
	if (m_EmoteStop < Server()->Tick())
	{
		m_EmoteType = m_pPlayer->m_DefEmote;
		m_EmoteStop = -1;
	}
	pCharacter->m_Emote = m_EmoteType;

	if (pCharacter->m_HookedPlayer != -1)
	{
		if (!Server()->Translate(pCharacter->m_HookedPlayer, SnappingClient))
			pCharacter->m_HookedPlayer = -1;
	}

	pCharacter->m_AttackTick = m_AttackTick;
	pCharacter->m_Direction = m_Input.m_Direction;
	pCharacter->m_Weapon = m_Core.m_ActiveWeapon;
	pCharacter->m_AmmoCount = 0;
	pCharacter->m_Health = 0;
	pCharacter->m_Armor = 0;

	// change eyes and use ninja graphic if player is freeze
	if (m_DeepFreeze)
	{
		if (pCharacter->m_Emote == EMOTE_NORMAL)
			pCharacter->m_Emote = EMOTE_PAIN;
		pCharacter->m_Weapon = WEAPON_NINJA;
	}
	else if (m_FreezeTime > 0 || m_FreezeTime == -1)
	{
		if (pCharacter->m_Emote == EMOTE_NORMAL)
			pCharacter->m_Emote = EMOTE_BLINK;
		pCharacter->m_Weapon = WEAPON_NINJA;
	}

	// jetpack and ninjajetpack prediction
	if (m_pPlayer->GetCID() == SnappingClient)
	{
		if (m_Jetpack && pCharacter->m_Weapon != WEAPON_NINJA)
		{
			if (!(m_NeededFaketuning & FAKETUNE_JETPACK))
			{
				m_NeededFaketuning |= FAKETUNE_JETPACK;
				GameServer()->SendTuningParams(m_pPlayer->GetCID(), m_TuneZone);
			}
		}
		else
		{
			if (m_NeededFaketuning & FAKETUNE_JETPACK)
			{
				m_NeededFaketuning &= ~FAKETUNE_JETPACK;
				GameServer()->SendTuningParams(m_pPlayer->GetCID(), m_TuneZone);
			}
		}
	}

	// change eyes, use ninja graphic and set ammo count if player has ninjajetpack
	if (m_pPlayer->m_NinjaJetpack && m_Jetpack && GetActiveWeapon() == WEAPON_GUN && !m_DeepFreeze && !(m_FreezeTime > 0 || m_FreezeTime == -1))
	{
		if (pCharacter->m_Emote == EMOTE_NORMAL)
			pCharacter->m_Emote = EMOTE_HAPPY;
		pCharacter->m_Weapon = WEAPON_NINJA;
		pCharacter->m_AmmoCount = 10;
	}

	if(m_pPlayer->GetCID() == SnappingClient || SnappingClient == -1 ||
		(!g_Config.m_SvStrictSpectateMode && m_pPlayer->GetCID() == GameServer()->m_apPlayers[SnappingClient]->m_SpectatorID))
	{
		pCharacter->m_Health = m_Health;
		pCharacter->m_Armor = m_Armor;
		if(m_aWeapons[GetActiveWeapon()].m_Ammo > 0)
			pCharacter->m_AmmoCount = (!m_FreezeTime)?m_aWeapons[GetActiveWeapon()].m_Ammo:0;
	}

	if(GetPlayer()->m_Afk || GetPlayer()->IsPaused())
	{
		if(m_FreezeTime > 0 || m_FreezeTime == -1 || m_DeepFreeze)
			pCharacter->m_Emote = EMOTE_NORMAL;
		else
			pCharacter->m_Emote = EMOTE_BLINK;
	}

	if(pCharacter->m_Emote == EMOTE_NORMAL)
	{
		if(250 - ((Server()->Tick() - m_LastAction)%(250)) < 5)
			pCharacter->m_Emote = EMOTE_BLINK;
	}

	if(m_pPlayer->m_Halloween)
	{
		if(1200 - ((Server()->Tick() - m_LastAction)%(1200)) < 5)
		{
			GameServer()->SendEmoticon(m_pPlayer->GetCID(), EMOTICON_GHOST);
		}
	}

	pCharacter->m_PlayerFlags = GetPlayer()->m_PlayerFlags;

	CNetObj_DDNetCharacter *pDDNetCharacter = static_cast<CNetObj_DDNetCharacter *>(Server()->SnapNewItem(NETOBJTYPE_DDNETCHARACTER, id, sizeof(CNetObj_DDNetCharacter)));
	pDDNetCharacter->m_Flags = 0;
	if (m_Solo)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_SOLO;
	if (m_Super)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_SUPER;
	if (m_EndlessHook)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_ENDLESS_HOOK;
	if (!m_Core.m_Collision)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_NO_COLLISION;
	if (!m_Core.m_Hook)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_NO_HOOK;
	if (m_SuperJump)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_ENDLESS_JUMP;
	if (m_Jetpack || m_NinjaJetpack)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_JETPACK;
	if (m_Hit&DISABLE_HIT_GRENADE)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_NO_GRENADE_HIT;
	if (m_Hit&DISABLE_HIT_HAMMER)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_NO_HAMMER_HIT;
	if (m_Hit&DISABLE_HIT_RIFLE)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_NO_RIFLE_HIT;
	if (m_Hit&DISABLE_HIT_SHOTGUN)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_NO_SHOTGUN_HIT;
	if (m_HasTeleGun)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_TELEGUN_GUN;
	if (m_HasTeleGrenade)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_TELEGUN_GRENADE;
	if (m_HasTeleLaser)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_TELEGUN_LASER;
	if (m_aWeapons[WEAPON_HAMMER].m_Got)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_HAMMER;
	if (m_aWeapons[WEAPON_GUN].m_Got)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_GUN;
	if (m_aWeapons[WEAPON_SHOTGUN].m_Got)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_SHOTGUN;
	if (m_aWeapons[WEAPON_GRENADE].m_Got)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_GRENADE;
	if (m_aWeapons[WEAPON_RIFLE].m_Got)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_LASER;
	if (m_Core.m_ActiveWeapon == WEAPON_NINJA)
		pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_NINJA;

	pDDNetCharacter->m_FreezeEnd = m_DeepFreeze ? -1 : m_FreezeTime == 0 ? 0 : Server()->Tick() + m_FreezeTime;
	pDDNetCharacter->m_Jumps = m_Core.m_Jumps;
	pDDNetCharacter->m_TeleCheckpoint = m_TeleCheckpoint;
	pDDNetCharacter->m_StrongWeakID = m_StrongWeakID;
}

int CCharacter::NetworkClipped(int SnappingClient)
{
	return NetworkClipped(SnappingClient, m_Pos);
}

int CCharacter::NetworkClipped(int SnappingClient, vec2 CheckPos)
{
	if(SnappingClient == -1 || GameServer()->m_apPlayers[SnappingClient]->m_ShowAll)
		return 0;

	float dx = GameServer()->m_apPlayers[SnappingClient]->m_ViewPos.x-CheckPos.x;
	float dy = GameServer()->m_apPlayers[SnappingClient]->m_ViewPos.y-CheckPos.y;

	if(absolute(dx) > 1000.0f || absolute(dy) > 800.0f)
		return 1;

	if(distance(GameServer()->m_apPlayers[SnappingClient]->m_ViewPos, CheckPos) > 4000.0f)
		return 1;
	return 0;
}

// DDRace

bool CCharacter::CanCollide(int ClientID, bool CheckPassive)
{
	return Teams()->m_Core.CanCollide(GetPlayer()->GetCID(), ClientID, CheckPassive);
}
bool CCharacter::SameTeam(int ClientID)
{
	return Teams()->m_Core.SameTeam(GetPlayer()->GetCID(), ClientID);
}

int CCharacter::Team()
{
	return Teams()->m_Core.Team(m_pPlayer->GetCID());
}

CGameTeams* CCharacter::Teams()
{
	return &((CGameControllerBlockDDrace*)GameServer()->m_pController)->m_Teams;
}

void CCharacter::HandleBroadcast()
{
	CPlayerData *pData = GameServer()->Score()->PlayerData(m_pPlayer->GetCID());

	if(m_DDRaceState == DDRACE_STARTED && m_CpLastBroadcast != m_CpActive &&
			m_CpActive > -1 && m_CpTick > Server()->Tick() && m_pPlayer->m_ClientVersion == VERSION_VANILLA &&
			pData->m_BestTime && pData->m_aBestCpTime[m_CpActive] != 0)
	{
		char aBroadcast[128];
		float Diff = m_CpCurrent[m_CpActive] - pData->m_aBestCpTime[m_CpActive];
		str_format(aBroadcast, sizeof(aBroadcast), "Checkpoint | Diff : %+5.2f", Diff);
		GameServer()->SendBroadcast(aBroadcast, m_pPlayer->GetCID());
		m_CpLastBroadcast = m_CpActive;
		m_LastBroadcast = Server()->Tick();
	}
	else if ((m_pPlayer->m_TimerType == CPlayer::TIMERTYPE_BROADCAST || m_pPlayer->m_TimerType == CPlayer::TIMERTYPE_GAMETIMER_AND_BROADCAST) && m_DDRaceState == DDRACE_STARTED && m_LastBroadcast + Server()->TickSpeed() * g_Config.m_SvTimeInBroadcastInterval <= Server()->Tick())
	{
		char aBuftime[64];
		int IntTime = (int)((float)(Server()->Tick() - m_StartTime) / ((float)Server()->TickSpeed()));
		str_format(aBuftime, sizeof(aBuftime), "%s%d:%s%d", ((IntTime/60) > 9)?"":"0", IntTime/60, ((IntTime%60) > 9)?"":"0", IntTime%60);
		GameServer()->SendBroadcast(aBuftime, m_pPlayer->GetCID(), false);
		m_CpLastBroadcast = m_CpActive;
		m_LastBroadcast = Server()->Tick();
	}
}

void CCharacter::HandleSkippableTiles(int Index)
{
	// handle death-tiles and leaving gamelayer
	if((GameServer()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f) == TILE_DEATH ||
			GameServer()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f) == TILE_DEATH ||
			GameServer()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f) == TILE_DEATH ||
			GameServer()->Collision()->GetFCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f) == TILE_DEATH||
			GameServer()->Collision()->GetFCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f) == TILE_DEATH ||
			GameServer()->Collision()->GetFCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f) == TILE_DEATH ||
			GameServer()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f) == TILE_DEATH) &&
			!m_Super && !(Team() && Teams()->TeeFinished(m_pPlayer->GetCID())))
	{
		Die(m_pPlayer->GetCID(), WEAPON_WORLD);
		return;
	}

	if (GameLayerClipped(m_Pos))
	{
		Die(m_pPlayer->GetCID(), WEAPON_WORLD);
		return;
	}

	if(Index < 0)
		return;

	// handle speedup tiles
	if(GameServer()->Collision()->IsSpeedup(Index))
	{
		vec2 Direction, MaxVel, TempVel = m_Core.m_Vel;
		int Force, MaxSpeed = 0;
		float TeeAngle, SpeederAngle, DiffAngle, SpeedLeft, TeeSpeed;
		GameServer()->Collision()->GetSpeedup(Index, &Direction, &Force, &MaxSpeed);
		if(Force == 255 && MaxSpeed)
		{
			m_Core.m_Vel = Direction * (MaxSpeed/5);
		}
		else
		{
			if(MaxSpeed > 0 && MaxSpeed < 5) MaxSpeed = 5;
			if(MaxSpeed > 0)
			{
				if(Direction.x > 0.0000001f)
					SpeederAngle = -atan(Direction.y / Direction.x);
				else if(Direction.x < 0.0000001f)
					SpeederAngle = atan(Direction.y / Direction.x) + 2.0f * asin(1.0f);
				else if(Direction.y > 0.0000001f)
					SpeederAngle = asin(1.0f);
				else
					SpeederAngle = asin(-1.0f);

				if(SpeederAngle < 0)
					SpeederAngle = 4.0f * asin(1.0f) + SpeederAngle;

				if(TempVel.x > 0.0000001f)
					TeeAngle = -atan(TempVel.y / TempVel.x);
				else if(TempVel.x < 0.0000001f)
					TeeAngle = atan(TempVel.y / TempVel.x) + 2.0f * asin(1.0f);
				else if(TempVel.y > 0.0000001f)
					TeeAngle = asin(1.0f);
				else
					TeeAngle = asin(-1.0f);

				if(TeeAngle < 0)
					TeeAngle = 4.0f * asin(1.0f) + TeeAngle;

				TeeSpeed = sqrt(pow(TempVel.x, 2) + pow(TempVel.y, 2));

				DiffAngle = SpeederAngle - TeeAngle;
				SpeedLeft = MaxSpeed / 5.0f - cos(DiffAngle) * TeeSpeed;
				if(abs((int)SpeedLeft) > Force && SpeedLeft > 0.0000001f)
					TempVel += Direction * Force;
				else if(abs((int)SpeedLeft) > Force)
					TempVel += Direction * -Force;
				else
					TempVel += Direction * SpeedLeft;
			}
			else
				TempVel += Direction * Force;

			if(TempVel.x > 0 && ((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_270) || (m_TileIndexL == TILE_STOP && m_TileFlagsL == ROTATION_270) || (m_TileIndexL == TILE_STOPS && (m_TileFlagsL == ROTATION_90 || m_TileFlagsL ==ROTATION_270)) || (m_TileIndexL == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_270) || (m_TileFIndexL == TILE_STOP && m_TileFFlagsL == ROTATION_270) || (m_TileFIndexL == TILE_STOPS && (m_TileFFlagsL == ROTATION_90 || m_TileFFlagsL == ROTATION_270)) || (m_TileFIndexL == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_270) || (m_TileSIndexL == TILE_STOP && m_TileSFlagsL == ROTATION_270) || (m_TileSIndexL == TILE_STOPS && (m_TileSFlagsL == ROTATION_90 || m_TileSFlagsL == ROTATION_270)) || (m_TileSIndexL == TILE_STOPA)))
				TempVel.x = 0;
			if(TempVel.x < 0 && ((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_90) || (m_TileIndexR == TILE_STOP && m_TileFlagsR == ROTATION_90) || (m_TileIndexR == TILE_STOPS && (m_TileFlagsR == ROTATION_90 || m_TileFlagsR == ROTATION_270)) || (m_TileIndexR == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_90) || (m_TileFIndexR == TILE_STOP && m_TileFFlagsR == ROTATION_90) || (m_TileFIndexR == TILE_STOPS && (m_TileFFlagsR == ROTATION_90 || m_TileFFlagsR == ROTATION_270)) || (m_TileFIndexR == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_90) || (m_TileSIndexR == TILE_STOP && m_TileSFlagsR == ROTATION_90) || (m_TileSIndexR == TILE_STOPS && (m_TileSFlagsR == ROTATION_90 || m_TileSFlagsR == ROTATION_270)) || (m_TileSIndexR == TILE_STOPA)))
				TempVel.x = 0;
			if(TempVel.y < 0 && ((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_180) || (m_TileIndexB == TILE_STOP && m_TileFlagsB == ROTATION_180) || (m_TileIndexB == TILE_STOPS && (m_TileFlagsB == ROTATION_0 || m_TileFlagsB == ROTATION_180)) || (m_TileIndexB == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_180) || (m_TileFIndexB == TILE_STOP && m_TileFFlagsB == ROTATION_180) || (m_TileFIndexB == TILE_STOPS && (m_TileFFlagsB == ROTATION_0 || m_TileFFlagsB == ROTATION_180)) || (m_TileFIndexB == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_180) || (m_TileSIndexB == TILE_STOP && m_TileSFlagsB == ROTATION_180) || (m_TileSIndexB == TILE_STOPS && (m_TileSFlagsB == ROTATION_0 || m_TileSFlagsB == ROTATION_180)) || (m_TileSIndexB == TILE_STOPA)))
				TempVel.y = 0;
			if(TempVel.y > 0 && ((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_0) || (m_TileIndexT == TILE_STOP && m_TileFlagsT == ROTATION_0) || (m_TileIndexT == TILE_STOPS && (m_TileFlagsT == ROTATION_0 || m_TileFlagsT == ROTATION_180)) || (m_TileIndexT == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_0) || (m_TileFIndexT == TILE_STOP && m_TileFFlagsT == ROTATION_0) || (m_TileFIndexT == TILE_STOPS && (m_TileFFlagsT == ROTATION_0 || m_TileFFlagsT == ROTATION_180)) || (m_TileFIndexT == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_0) || (m_TileSIndexT == TILE_STOP && m_TileSFlagsT == ROTATION_0) || (m_TileSIndexT == TILE_STOPS && (m_TileSFlagsT == ROTATION_0 || m_TileSFlagsT == ROTATION_180)) || (m_TileSIndexT == TILE_STOPA)))
				TempVel.y = 0;
			m_Core.m_Vel = TempVel;
		}
	}
}

void CCharacter::HandleTiles(int Index)
{
	CGameControllerBlockDDrace* Controller = (CGameControllerBlockDDrace*)GameServer()->m_pController;
	int MapIndex = Index;
	//int PureMapIndex = GameServer()->Collision()->GetPureMapIndex(m_Pos);
	float Offset = 4.0f;
	int MapIndexL = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x + (m_ProximityRadius / 2) + Offset, m_Pos.y));
	int MapIndexR = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x - (m_ProximityRadius / 2) - Offset, m_Pos.y));
	int MapIndexT = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x, m_Pos.y + (m_ProximityRadius / 2) + Offset));
	int MapIndexB = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x, m_Pos.y - (m_ProximityRadius / 2) - Offset));
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
	m_TileFFlagsT = GameServer()->Collision()->GetFTileFlags(MapIndexT);//
	m_TileSIndex = (GameServer()->Collision()->m_pSwitchers && GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetDTileNumber(MapIndex)].m_Status[Team()])?(Team() != TEAM_SUPER)? GameServer()->Collision()->GetDTileIndex(MapIndex) : 0 : 0;
	m_TileSFlags = (GameServer()->Collision()->m_pSwitchers && GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetDTileNumber(MapIndex)].m_Status[Team()])?(Team() != TEAM_SUPER)? GameServer()->Collision()->GetDTileFlags(MapIndex) : 0 : 0;
	m_TileSIndexL = (GameServer()->Collision()->m_pSwitchers && GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetDTileNumber(MapIndexL)].m_Status[Team()])?(Team() != TEAM_SUPER)? GameServer()->Collision()->GetDTileIndex(MapIndexL) : 0 : 0;
	m_TileSFlagsL = (GameServer()->Collision()->m_pSwitchers && GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetDTileNumber(MapIndexL)].m_Status[Team()])?(Team() != TEAM_SUPER)? GameServer()->Collision()->GetDTileFlags(MapIndexL) : 0 : 0;
	m_TileSIndexR = (GameServer()->Collision()->m_pSwitchers && GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetDTileNumber(MapIndexR)].m_Status[Team()])?(Team() != TEAM_SUPER)? GameServer()->Collision()->GetDTileIndex(MapIndexR) : 0 : 0;
	m_TileSFlagsR = (GameServer()->Collision()->m_pSwitchers && GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetDTileNumber(MapIndexR)].m_Status[Team()])?(Team() != TEAM_SUPER)? GameServer()->Collision()->GetDTileFlags(MapIndexR) : 0 : 0;
	m_TileSIndexB = (GameServer()->Collision()->m_pSwitchers && GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetDTileNumber(MapIndexB)].m_Status[Team()])?(Team() != TEAM_SUPER)? GameServer()->Collision()->GetDTileIndex(MapIndexB) : 0 : 0;
	m_TileSFlagsB = (GameServer()->Collision()->m_pSwitchers && GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetDTileNumber(MapIndexB)].m_Status[Team()])?(Team() != TEAM_SUPER)? GameServer()->Collision()->GetDTileFlags(MapIndexB) : 0 : 0;
	m_TileSIndexT = (GameServer()->Collision()->m_pSwitchers && GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetDTileNumber(MapIndexT)].m_Status[Team()])?(Team() != TEAM_SUPER)? GameServer()->Collision()->GetDTileIndex(MapIndexT) : 0 : 0;
	m_TileSFlagsT = (GameServer()->Collision()->m_pSwitchers && GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetDTileNumber(MapIndexT)].m_Status[Team()])?(Team() != TEAM_SUPER)? GameServer()->Collision()->GetDTileFlags(MapIndexT) : 0 : 0;
	//Sensitivity
	int S1 = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f));
	int S2 = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x + m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f));
	int S3 = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y - m_ProximityRadius / 3.f));
	int S4 = GameServer()->Collision()->GetPureMapIndex(vec2(m_Pos.x - m_ProximityRadius / 3.f, m_Pos.y + m_ProximityRadius / 3.f));
	int Tile1 = GameServer()->Collision()->GetTileIndex(S1);
	int Tile2 = GameServer()->Collision()->GetTileIndex(S2);
	int Tile3 = GameServer()->Collision()->GetTileIndex(S3);
	int Tile4 = GameServer()->Collision()->GetTileIndex(S4);
	int FTile1 = GameServer()->Collision()->GetFTileIndex(S1);
	int FTile2 = GameServer()->Collision()->GetFTileIndex(S2);
	int FTile3 = GameServer()->Collision()->GetFTileIndex(S3);
	int FTile4 = GameServer()->Collision()->GetFTileIndex(S4);
	if(Index < 0)
	{
		m_LastRefillJumps = false;
		m_LastPenalty = false;
		m_LastBonus = false;
		return;
	}
	int cp = GameServer()->Collision()->IsCheckpoint(MapIndex);
	if(cp != -1 && m_DDRaceState == DDRACE_STARTED && cp > m_CpActive)
	{
		m_CpActive = cp;
		m_CpCurrent[cp] = m_Time;
		m_CpTick = Server()->Tick() + Server()->TickSpeed() * 2;
		if(m_pPlayer->m_ClientVersion >= VERSION_DDRACE) {
			CPlayerData *pData = GameServer()->Score()->PlayerData(m_pPlayer->GetCID());
			CNetMsg_Sv_DDRaceTime Msg;
			Msg.m_Time = (int)m_Time;
			Msg.m_Check = 0;
			Msg.m_Finish = 0;

			if(m_CpActive != -1 && m_CpTick > Server()->Tick())
			{
				if(pData->m_BestTime && pData->m_aBestCpTime[m_CpActive] != 0)
				{
					float Diff = (m_CpCurrent[m_CpActive] - pData->m_aBestCpTime[m_CpActive])*100;
					Msg.m_Check = (int)Diff;
				}
			}

			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, m_pPlayer->GetCID());
		}
	}
	int cpf = GameServer()->Collision()->IsFCheckpoint(MapIndex);
	if(cpf != -1 && m_DDRaceState == DDRACE_STARTED && cpf > m_CpActive)
	{
		m_CpActive = cpf;
		m_CpCurrent[cpf] = m_Time;
		m_CpTick = Server()->Tick() + Server()->TickSpeed()*2;
		if(m_pPlayer->m_ClientVersion >= VERSION_DDRACE) {
			CPlayerData *pData = GameServer()->Score()->PlayerData(m_pPlayer->GetCID());
			CNetMsg_Sv_DDRaceTime Msg;
			Msg.m_Time = (int)m_Time;
			Msg.m_Check = 0;
			Msg.m_Finish = 0;

			if(m_CpActive != -1 && m_CpTick > Server()->Tick())
			{
				if(pData->m_BestTime && pData->m_aBestCpTime[m_CpActive] != 0)
				{
					float Diff = (m_CpCurrent[m_CpActive] - pData->m_aBestCpTime[m_CpActive])*100;
					Msg.m_Check = (int)Diff;
				}
			}

			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, m_pPlayer->GetCID());
		}
	}
	int tcp = GameServer()->Collision()->IsTCheckpoint(MapIndex);
	if(tcp)
		m_TeleCheckpoint = tcp;

	// start
	if(((m_TileIndex == TILE_BEGIN) || (m_TileFIndex == TILE_BEGIN) || FTile1 == TILE_BEGIN || FTile2 == TILE_BEGIN || FTile3 == TILE_BEGIN || FTile4 == TILE_BEGIN || Tile1 == TILE_BEGIN || Tile2 == TILE_BEGIN || Tile3 == TILE_BEGIN || Tile4 == TILE_BEGIN) && (m_DDRaceState == DDRACE_NONE || m_DDRaceState == DDRACE_FINISHED || (m_DDRaceState == DDRACE_STARTED && !Team() && g_Config.m_SvTeam != 3)))
	{
		bool CanBegin = true;
		if(g_Config.m_SvResetPickups)
		{
			for (int i = WEAPON_SHOTGUN; i < NUM_WEAPONS; ++i)
			{
				m_aWeapons[i].m_Got = false;
				if(GetActiveWeapon() == i)
					SetActiveWeapon(WEAPON_GUN);
			}
		}
		if(g_Config.m_SvTeam == 2 && (Team() == TEAM_FLOCK || Teams()->Count(Team()) <= 1))
		{
			if(m_LastStartWarning < Server()->Tick() - 3 * Server()->TickSpeed())
			{
				GameServer()->SendChatTarget(GetPlayer()->GetCID(),"You have to be in a team with other tees to start");
				m_LastStartWarning = Server()->Tick();
			}
			Die(GetPlayer()->GetCID(), WEAPON_WORLD);
			CanBegin = false;
		}
		if(CanBegin)
		{
			Teams()->OnCharacterStart(m_pPlayer->GetCID());
			m_CpActive = -2;
		}
	}

	// finish
	if(((m_TileIndex == TILE_END) || (m_TileFIndex == TILE_END) || FTile1 == TILE_END || FTile2 == TILE_END || FTile3 == TILE_END || FTile4 == TILE_END || Tile1 == TILE_END || Tile2 == TILE_END || Tile3 == TILE_END || Tile4 == TILE_END) && m_DDRaceState == DDRACE_STARTED)
	{
		Controller->m_Teams.OnCharacterFinish(m_pPlayer->GetCID());
		m_pPlayer->m_Score = GameServer()->Score()->PlayerData(m_pPlayer->GetCID())->m_BestTime;
	}

	// freeze
	if(((m_TileIndex == TILE_FREEZE) || (m_TileFIndex == TILE_FREEZE)) && !m_Super && !m_DeepFreeze)
		Freeze();
	else if(((m_TileIndex == TILE_UNFREEZE) || (m_TileFIndex == TILE_UNFREEZE)) && !m_DeepFreeze)
		UnFreeze();

	// deep freeze
	if(((m_TileIndex == TILE_DFREEZE) || (m_TileFIndex == TILE_DFREEZE)) && !m_Super && !m_DeepFreeze)
		m_DeepFreeze = true;
	else if(((m_TileIndex == TILE_DUNFREEZE) || (m_TileFIndex == TILE_DUNFREEZE)) && !m_Super && m_DeepFreeze)
		m_DeepFreeze = false;

	// endless hook
	if(((m_TileIndex == TILE_EHOOK_START) || (m_TileFIndex == TILE_EHOOK_START)) && !m_EndlessHook)
		EndlessHook();
	else if(((m_TileIndex == TILE_EHOOK_END) || (m_TileFIndex == TILE_EHOOK_END)) && m_EndlessHook)
		EndlessHook(false);

	// hit others
	if(((m_TileIndex == TILE_HIT_END) || (m_TileFIndex == TILE_HIT_END)) && m_Hit != (DISABLE_HIT_GRENADE|DISABLE_HIT_HAMMER|DISABLE_HIT_RIFLE|DISABLE_HIT_SHOTGUN))
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCID(), "You can't hit others");
		m_Hit = DISABLE_HIT_GRENADE|DISABLE_HIT_HAMMER|DISABLE_HIT_RIFLE|DISABLE_HIT_SHOTGUN;
		m_Core.m_NoShotgunHit = true;
		m_Core.m_NoGrenadeHit = true;
		m_Core.m_NoHammerHit = true;
		m_Core.m_NoRifleHit = true;
		m_NeededFaketuning |= FAKETUNE_NOHAMMER;
		GameServer()->SendTuningParams(m_pPlayer->GetCID(), m_TuneZone); // update tunings
	}
	else if(((m_TileIndex == TILE_HIT_START) || (m_TileFIndex == TILE_HIT_START)) && m_Hit != HIT_ALL)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCID(), "You can hit others");
		m_Hit = HIT_ALL;
		m_Core.m_NoShotgunHit = false;
		m_Core.m_NoGrenadeHit = false;
		m_Core.m_NoHammerHit = false;
		m_Core.m_NoRifleHit = false;
		m_NeededFaketuning &= ~FAKETUNE_NOHAMMER;
		GameServer()->SendTuningParams(m_pPlayer->GetCID(), m_TuneZone); // update tunings
	}

	// collide with others
	if(((m_TileIndex == TILE_NPC_END) || (m_TileFIndex == TILE_NPC_END)) && m_Core.m_Collision)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCID(), "You can't collide with others");
		m_Core.m_Collision = false;
		m_Core.m_NoCollision = true;
		m_NeededFaketuning |= FAKETUNE_NOCOLL;
		GameServer()->SendTuningParams(m_pPlayer->GetCID(), m_TuneZone); // update tunings
	}
	else if(((m_TileIndex == TILE_NPC_START) || (m_TileFIndex == TILE_NPC_START)) && !m_Core.m_Collision)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCID(),"You can collide with others");
		m_Core.m_Collision = true;
		m_Core.m_NoCollision = false;
		m_NeededFaketuning &= ~FAKETUNE_NOCOLL;
		GameServer()->SendTuningParams(m_pPlayer->GetCID(), m_TuneZone); // update tunings
	}

	// hook others
	if(((m_TileIndex == TILE_NPH_END) || (m_TileFIndex == TILE_NPH_END)) && m_Core.m_Hook)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCID(), "You can't hook others");
		m_Core.m_Hook = false;
		m_Core.m_NoHookHit = true;
		m_NeededFaketuning |= FAKETUNE_NOHOOK;
		GameServer()->SendTuningParams(m_pPlayer->GetCID(), m_TuneZone); // update tunings
	}
	else if(((m_TileIndex == TILE_NPH_START) || (m_TileFIndex == TILE_NPH_START)) && !m_Core.m_Hook)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCID(),"You can hook others");
		m_Core.m_Hook = true;
		m_Core.m_NoHookHit = false;
		m_NeededFaketuning &= ~FAKETUNE_NOHOOK;
		GameServer()->SendTuningParams(m_pPlayer->GetCID(), m_TuneZone); // update tunings
	}

	// unlimited air jumps
	if(((m_TileIndex == TILE_SUPER_START) || (m_TileFIndex == TILE_SUPER_START)) && !m_SuperJump)
		InfiniteJumps();
	else if(((m_TileIndex == TILE_SUPER_END) || (m_TileFIndex == TILE_SUPER_END)) && m_SuperJump)
		InfiniteJumps(false);

	// walljump
	if((m_TileIndex == TILE_WALLJUMP) || (m_TileFIndex == TILE_WALLJUMP))
	{
		if(m_Core.m_Vel.y > 0 && m_Core.m_Colliding && m_Core.m_LeftWall)
		{
			m_Core.m_LeftWall = false;
			m_Core.m_JumpedTotal = m_Core.m_Jumps - 1;
			m_Core.m_Jumped = 1;
		}
	}

	// jetpack gun
	if(((m_TileIndex == TILE_JETPACK_START) || (m_TileFIndex == TILE_JETPACK_START)) && !m_Jetpack)
		Jetpack();
	else if(((m_TileIndex == TILE_JETPACK_END) || (m_TileFIndex == TILE_JETPACK_END)) && m_Jetpack)
		Jetpack(false);

	// unlock team
	else if(((m_TileIndex == TILE_UNLOCK_TEAM) || (m_TileFIndex == TILE_UNLOCK_TEAM)) && Teams()->TeamLocked(Team()))
	{
		Teams()->SetTeamLock(Team(), false);

		for(int i = 0; i < MAX_CLIENTS; i++)
			if(Teams()->m_Core.Team(i) == Team())
				GameServer()->SendChatTarget(i, "Your team was unlocked by an unlock team tile");
	}


	/*************************************************
	*                                                *
	*              B L O C K D D R A C E             *
	*                                                *
	**************************************************/

	//jetpack toggle
	if ((m_TileIndex == TILE_JETPACK) || (m_TileFIndex == TILE_JETPACK))
	{
		if ((m_LastIndexTile == TILE_JETPACK) || (m_LastIndexFrontTile == TILE_JETPACK))
			return;
		Jetpack(!m_Jetpack);
	}

	//rainbow toggle
	if ((m_TileIndex == TILE_RAINBOW) || (m_TileFIndex == TILE_RAINBOW))
	{
		if ((m_LastIndexTile == TILE_RAINBOW) || (m_LastIndexFrontTile == TILE_RAINBOW))
			return;
		Rainbow(!(m_Rainbow || m_pPlayer->m_InfRainbow));
	}

	//atom toggle
	if ((m_TileIndex == TILE_ATOM) || (m_TileFIndex == TILE_ATOM))
	{
		if ((m_LastIndexTile == TILE_ATOM) || (m_LastIndexFrontTile == TILE_ATOM))
			return;
		Atom(!m_Atom);
	}

	//trail toggle
	if ((m_TileIndex == TILE_TRAIL) || (m_TileFIndex == TILE_TRAIL))
	{
		if ((m_LastIndexTile == TILE_TRAIL) || (m_LastIndexFrontTile == TILE_TRAIL))
			return;
		Trail(!m_Trail);
	}

	//spooky ghost toggle
	if ((m_TileIndex == TILE_SPOOKY_GHOST) || (m_TileFIndex == TILE_SPOOKY_GHOST))
	{
		if ((m_LastIndexTile == TILE_SPOOKY_GHOST) || (m_LastIndexFrontTile == TILE_SPOOKY_GHOST) || GameServer()->m_Accounts[m_pPlayer->GetAccID()].m_aHasItem[SPOOKY_GHOST])
			return;
		SpookyGhost(!m_pPlayer->m_HasSpookyGhost);
	}

	//add meteor
	if ((m_TileIndex == TILE_ADD_METEOR) || (m_TileFIndex == TILE_ADD_METEOR))
	{
		if ((m_LastIndexTile == TILE_ADD_METEOR) || (m_LastIndexFrontTile == TILE_ADD_METEOR))
			return;
		Meteor();
	}

	//remove meteors
	if ((m_TileIndex == TILE_REMOVE_METEORS) || (m_TileFIndex == TILE_REMOVE_METEORS))
	{
		if ((m_LastIndexTile == TILE_REMOVE_METEORS) || (m_LastIndexFrontTile == TILE_REMOVE_METEORS))
			return;
		Meteor(false);
	}

	//passive toggle
	if ((m_TileIndex == TILE_PASSIVE) || (m_TileFIndex == TILE_PASSIVE))
	{
		if ((m_LastIndexTile == TILE_PASSIVE) || (m_LastIndexFrontTile == TILE_PASSIVE))
			return;
		Passive(!m_Passive);
	}

	//vanilla mode
	if ((m_TileIndex == TILE_VANILLA_MODE) || (m_TileFIndex == TILE_VANILLA_MODE))
	{
		if ((m_LastIndexTile == TILE_VANILLA_MODE) || (m_LastIndexFrontTile == TILE_VANILLA_MODE))
			return;
		VanillaMode();
	}

	//ddrace mode
	if ((m_TileIndex == TILE_DDRACE_MODE) || (m_TileFIndex == TILE_DDRACE_MODE))
	{
		if ((m_LastIndexTile == TILE_DDRACE_MODE) || (m_LastIndexFrontTile == TILE_DDRACE_MODE))
			return;
		DDraceMode();
	}

	//bloody toggle
	if ((m_TileIndex == TILE_BLOODY) || (m_TileFIndex == TILE_BLOODY))
	{
		if ((m_LastIndexTile == TILE_BLOODY) || (m_LastIndexFrontTile == TILE_BLOODY))
			return;
		Bloody(!(m_Bloody || m_StrongBloody));
	}

	//shop
	if (m_TileIndex == TILE_SHOP || m_TileFIndex == TILE_SHOP)
	{
		if (!m_InShop)
		{
			m_EnteredShop = true;
			m_InShop = true;
		}
		if (m_EnteredShop)
		{
			if (m_ShopBotAntiSpamTick <= Server()->Tick())
			{
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "Welcome to the shop, %s! Press f4 to start shopping.", Server()->ClientName(m_pPlayer->GetCID()));
				GameServer()->SendChat(GameWorld()->GetClosestShopBot(m_Pos, this, m_pPlayer->GetCID()), CGameContext::CHAT_SINGLE, aBuf, -1, m_pPlayer->GetCID());
			}
			m_EnteredShop = false;
		}
		if (Server()->Tick() % 50 == 0)
			GameServer()->SendBroadcast("~ S H O P ~", m_pPlayer->GetCID(), false);
	}
	if (m_pPlayer->m_IsDummy && (m_TileIndex == ENTITY_SHOP_BOT_SPAWN || m_TileFIndex == ENTITY_SHOP_BOT_SPAWN))
		m_InShop = true;


	// helper only
	if ((m_TileIndex == TILE_HELPERS_ONLY) || (m_TileFIndex == TILE_HELPERS_ONLY))
	{
		if (Server()->GetAuthedState(m_pPlayer->GetCID()) < AUTHED_HELPER)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCID(), "This area is for helpers only");
			Die(m_pPlayer->GetCID(), WEAPON_WORLD);
			return;
		}
	}

	// moderator only
	if ((m_TileIndex == TILE_MODERATORS_ONLY) || (m_TileFIndex == TILE_MODERATORS_ONLY))
	{
		if (Server()->GetAuthedState(m_pPlayer->GetCID()) < AUTHED_MOD)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCID(), "This area is for moderators only");
			Die(m_pPlayer->GetCID(), WEAPON_WORLD);
			return;
		}
	}

	// admin only
	if ((m_TileIndex == TILE_ADMINS_ONLY) || (m_TileFIndex == TILE_ADMINS_ONLY))
	{
		if (Server()->GetAuthedState(m_pPlayer->GetCID()) < AUTHED_ADMIN)
		{
			GameServer()->SendChatTarget(GetPlayer()->GetCID(), "This area is for admins only");
			Die(m_pPlayer->GetCID(), WEAPON_WORLD);
			return;
		}
	}


	m_LastIndexTile = m_TileIndex;
	m_LastIndexFrontTile = m_TileFIndex;

	/*************************************************
	*                                                *
	*              B L O C K D D R A C E             *
	*                                                *
	**************************************************/

	// solo part
	if(((m_TileIndex == TILE_SOLO_START) || (m_TileFIndex == TILE_SOLO_START)) && !m_Solo)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCID(), "You are now in a solo part");
		SetSolo(true);
	}
	else if(((m_TileIndex == TILE_SOLO_END) || (m_TileFIndex == TILE_SOLO_END)) && m_Solo)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCID(), "You are now out of the solo part");
		SetSolo(false);
	}

	// refill jumps
	if(((m_TileIndex == TILE_REFILL_JUMPS) || (m_TileFIndex == TILE_REFILL_JUMPS)) && !m_LastRefillJumps)
	{
		m_Core.m_JumpedTotal = 0;
		m_Core.m_Jumped = 0;
		m_LastRefillJumps = true;
	}
	if((m_TileIndex != TILE_REFILL_JUMPS) && (m_TileFIndex != TILE_REFILL_JUMPS))
	{
		m_LastRefillJumps = false;
	}

	// Teleport gun
	if (((m_TileIndex == TILE_TELE_GUN_ENABLE) || (m_TileFIndex == TILE_TELE_GUN_ENABLE)) && !m_HasTeleGun)
	{
		m_HasTeleGun = true;
		m_Core.m_HasTelegunGun = true;
		GameServer()->SendChatTarget(GetPlayer()->GetCID(), "Teleport gun enabled");
	}
	else if (((m_TileIndex == TILE_TELE_GUN_DISABLE) || (m_TileFIndex == TILE_TELE_GUN_DISABLE)) && m_HasTeleGun)
	{
		m_HasTeleGun = false;
		m_Core.m_HasTelegunGun = false;
		GameServer()->SendChatTarget(GetPlayer()->GetCID(), "Teleport gun disabled");
	}

	if (((m_TileIndex == TILE_TELE_GRENADE_ENABLE) || (m_TileFIndex == TILE_TELE_GRENADE_ENABLE)) && !m_HasTeleGrenade)
	{
		m_HasTeleGrenade = true;
		m_Core.m_HasTelegunGrenade = true;
		GameServer()->SendChatTarget(GetPlayer()->GetCID(), "Teleport grenade enabled");
	}
	else if (((m_TileIndex == TILE_TELE_GRENADE_DISABLE) || (m_TileFIndex == TILE_TELE_GRENADE_DISABLE)) && m_HasTeleGrenade)
	{
		m_HasTeleGrenade = false;
		m_Core.m_HasTelegunGrenade = false;
		GameServer()->SendChatTarget(GetPlayer()->GetCID(), "Teleport grenade disabled");
	}

	if (((m_TileIndex == TILE_TELE_LASER_ENABLE) || (m_TileFIndex == TILE_TELE_LASER_ENABLE)) && !m_HasTeleLaser)
	{
		m_HasTeleLaser = true;
		m_Core.m_HasTelegunLaser = true;
		GameServer()->SendChatTarget(GetPlayer()->GetCID(), "Teleport laser enabled");
	}
	else if (((m_TileIndex == TILE_TELE_LASER_DISABLE) || (m_TileFIndex == TILE_TELE_LASER_DISABLE)) && m_HasTeleLaser)
	{
		m_HasTeleLaser = false;
		m_Core.m_HasTelegunLaser = false;
		GameServer()->SendChatTarget(GetPlayer()->GetCID(), "Teleport laser disabled");
	}

	// stopper
	if(((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_270) || (m_TileIndexL == TILE_STOP && m_TileFlagsL == ROTATION_270) || (m_TileIndexL == TILE_STOPS && (m_TileFlagsL == ROTATION_90 || m_TileFlagsL ==ROTATION_270)) || (m_TileIndexL == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_270) || (m_TileFIndexL == TILE_STOP && m_TileFFlagsL == ROTATION_270) || (m_TileFIndexL == TILE_STOPS && (m_TileFFlagsL == ROTATION_90 || m_TileFFlagsL == ROTATION_270)) || (m_TileFIndexL == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_270) || (m_TileSIndexL == TILE_STOP && m_TileSFlagsL == ROTATION_270) || (m_TileSIndexL == TILE_STOPS && (m_TileSFlagsL == ROTATION_90 || m_TileSFlagsL == ROTATION_270)) || (m_TileSIndexL == TILE_STOPA)) && m_Core.m_Vel.x > 0)
	{
		if((int)GameServer()->Collision()->GetPos(MapIndexL).x)
			if((int)GameServer()->Collision()->GetPos(MapIndexL).x < (int)m_Core.m_Pos.x)
				m_Core.m_Pos = m_PrevPos;
		m_Core.m_Vel.x = 0;
	}
	if(((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_90) || (m_TileIndexR == TILE_STOP && m_TileFlagsR == ROTATION_90) || (m_TileIndexR == TILE_STOPS && (m_TileFlagsR == ROTATION_90 || m_TileFlagsR == ROTATION_270)) || (m_TileIndexR == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_90) || (m_TileFIndexR == TILE_STOP && m_TileFFlagsR == ROTATION_90) || (m_TileFIndexR == TILE_STOPS && (m_TileFFlagsR == ROTATION_90 || m_TileFFlagsR == ROTATION_270)) || (m_TileFIndexR == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_90) || (m_TileSIndexR == TILE_STOP && m_TileSFlagsR == ROTATION_90) || (m_TileSIndexR == TILE_STOPS && (m_TileSFlagsR == ROTATION_90 || m_TileSFlagsR == ROTATION_270)) || (m_TileSIndexR == TILE_STOPA)) && m_Core.m_Vel.x < 0)
	{
		if((int)GameServer()->Collision()->GetPos(MapIndexR).x)
			if((int)GameServer()->Collision()->GetPos(MapIndexR).x > (int)m_Core.m_Pos.x)
				m_Core.m_Pos = m_PrevPos;
		m_Core.m_Vel.x = 0;
	}
	if(((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_180) || (m_TileIndexB == TILE_STOP && m_TileFlagsB == ROTATION_180) || (m_TileIndexB == TILE_STOPS && (m_TileFlagsB == ROTATION_0 || m_TileFlagsB == ROTATION_180)) || (m_TileIndexB == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_180) || (m_TileFIndexB == TILE_STOP && m_TileFFlagsB == ROTATION_180) || (m_TileFIndexB == TILE_STOPS && (m_TileFFlagsB == ROTATION_0 || m_TileFFlagsB == ROTATION_180)) || (m_TileFIndexB == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_180) || (m_TileSIndexB == TILE_STOP && m_TileSFlagsB == ROTATION_180) || (m_TileSIndexB == TILE_STOPS && (m_TileSFlagsB == ROTATION_0 || m_TileSFlagsB == ROTATION_180)) || (m_TileSIndexB == TILE_STOPA)) && m_Core.m_Vel.y < 0)
	{
		if((int)GameServer()->Collision()->GetPos(MapIndexB).y)
			if((int)GameServer()->Collision()->GetPos(MapIndexB).y > (int)m_Core.m_Pos.y)
				m_Core.m_Pos = m_PrevPos;
		m_Core.m_Vel.y = 0;
	}
	if(((m_TileIndex == TILE_STOP && m_TileFlags == ROTATION_0) || (m_TileIndexT == TILE_STOP && m_TileFlagsT == ROTATION_0) || (m_TileIndexT == TILE_STOPS && (m_TileFlagsT == ROTATION_0 || m_TileFlagsT == ROTATION_180)) || (m_TileIndexT == TILE_STOPA) || (m_TileFIndex == TILE_STOP && m_TileFFlags == ROTATION_0) || (m_TileFIndexT == TILE_STOP && m_TileFFlagsT == ROTATION_0) || (m_TileFIndexT == TILE_STOPS && (m_TileFFlagsT == ROTATION_0 || m_TileFFlagsT == ROTATION_180)) || (m_TileFIndexT == TILE_STOPA) || (m_TileSIndex == TILE_STOP && m_TileSFlags == ROTATION_0) || (m_TileSIndexT == TILE_STOP && m_TileSFlagsT == ROTATION_0) || (m_TileSIndexT == TILE_STOPS && (m_TileSFlagsT == ROTATION_0 || m_TileSFlagsT == ROTATION_180)) || (m_TileSIndexT == TILE_STOPA)) && m_Core.m_Vel.y > 0)
	{
		if((int)GameServer()->Collision()->GetPos(MapIndexT).y)
			if((int)GameServer()->Collision()->GetPos(MapIndexT).y < (int)m_Core.m_Pos.y)
				m_Core.m_Pos = m_PrevPos;
		m_Core.m_Vel.y = 0;
		m_Core.m_Jumped = 0;
		m_Core.m_JumpedTotal = 0;
	}

	// handle switch tiles
	if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_SWITCHOPEN && Team() != TEAM_SUPER && GameServer()->Collision()->GetSwitchNumber(MapIndex) > 0)
	{
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_Status[Team()] = true;
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_EndTick[Team()] = 0;
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_Type[Team()] = TILE_SWITCHOPEN;
	}
	else if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_SWITCHTIMEDOPEN && Team() != TEAM_SUPER && GameServer()->Collision()->GetSwitchNumber(MapIndex) > 0)
	{
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_Status[Team()] = true;
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_EndTick[Team()] = Server()->Tick() + 1 + GameServer()->Collision()->GetSwitchDelay(MapIndex)*Server()->TickSpeed() ;
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_Type[Team()] = TILE_SWITCHTIMEDOPEN;
	}
	else if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_SWITCHTIMEDCLOSE && Team() != TEAM_SUPER && GameServer()->Collision()->GetSwitchNumber(MapIndex) > 0)
	{
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_Status[Team()] = false;
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_EndTick[Team()] = Server()->Tick() + 1 + GameServer()->Collision()->GetSwitchDelay(MapIndex)*Server()->TickSpeed();
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_Type[Team()] = TILE_SWITCHTIMEDCLOSE;
	}
	else if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_SWITCHCLOSE && Team() != TEAM_SUPER && GameServer()->Collision()->GetSwitchNumber(MapIndex) > 0)
	{
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_Status[Team()] = false;
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_EndTick[Team()] = 0;
		GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_Type[Team()] = TILE_SWITCHCLOSE;
	}
	else if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_FREEZE && Team() != TEAM_SUPER)
	{
		if(GameServer()->Collision()->GetSwitchNumber(MapIndex) == 0 || GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_Status[Team()])
			Freeze(GameServer()->Collision()->GetSwitchDelay(MapIndex));
	}
	else if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_DFREEZE && Team() != TEAM_SUPER)
	{
		if(GameServer()->Collision()->GetSwitchNumber(MapIndex) == 0 || GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_Status[Team()])
			m_DeepFreeze = true;
	}
	else if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_DUNFREEZE && Team() != TEAM_SUPER)
	{
		if(GameServer()->Collision()->GetSwitchNumber(MapIndex) == 0 || GameServer()->Collision()->m_pSwitchers[GameServer()->Collision()->GetSwitchNumber(MapIndex)].m_Status[Team()])
			m_DeepFreeze = false;
	}
	else if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_HIT_START && m_Hit&DISABLE_HIT_HAMMER && GameServer()->Collision()->GetSwitchDelay(MapIndex) == WEAPON_HAMMER)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCID(),"You can hammer hit others");
		m_Hit &= ~DISABLE_HIT_HAMMER;
		m_NeededFaketuning &= ~FAKETUNE_NOHAMMER;
		m_Core.m_NoHammerHit = false;
		GameServer()->SendTuningParams(m_pPlayer->GetCID(), m_TuneZone); // update tunings
	}
	else if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_HIT_END && !(m_Hit&DISABLE_HIT_HAMMER) && GameServer()->Collision()->GetSwitchDelay(MapIndex) == WEAPON_HAMMER)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCID(),"You can't hammer hit others");
		m_Hit |= DISABLE_HIT_HAMMER;
		m_NeededFaketuning |= FAKETUNE_NOHAMMER;
		m_Core.m_NoHammerHit = true;
		GameServer()->SendTuningParams(m_pPlayer->GetCID(), m_TuneZone); // update tunings
	}
	else if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_HIT_START && m_Hit&DISABLE_HIT_SHOTGUN && GameServer()->Collision()->GetSwitchDelay(MapIndex) == WEAPON_SHOTGUN)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCID(),"You can shoot others with shotgun");
		m_Hit &= ~DISABLE_HIT_SHOTGUN;
		m_Core.m_NoShotgunHit = false;
	}
	else if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_HIT_END && !(m_Hit&DISABLE_HIT_SHOTGUN) && GameServer()->Collision()->GetSwitchDelay(MapIndex) == WEAPON_SHOTGUN)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCID(),"You can't shoot others with shotgun");
		m_Hit |= DISABLE_HIT_SHOTGUN;
		m_Core.m_NoShotgunHit = true;
	}
	else if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_HIT_START && m_Hit&DISABLE_HIT_GRENADE && GameServer()->Collision()->GetSwitchDelay(MapIndex) == WEAPON_GRENADE)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCID(),"You can shoot others with grenade");
		m_Hit &= ~DISABLE_HIT_GRENADE;
		m_Core.m_NoGrenadeHit = false;
	}
	else if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_HIT_END && !(m_Hit&DISABLE_HIT_GRENADE) && GameServer()->Collision()->GetSwitchDelay(MapIndex) == WEAPON_GRENADE)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCID(),"You can't shoot others with grenade");
		m_Hit |= DISABLE_HIT_GRENADE;
		m_Core.m_NoGrenadeHit = true;
	}
	else if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_HIT_START && m_Hit&DISABLE_HIT_RIFLE && GameServer()->Collision()->GetSwitchDelay(MapIndex) == WEAPON_RIFLE)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCID(),"You can shoot others with rifle");
		m_Hit &= ~DISABLE_HIT_RIFLE;
		m_Core.m_NoRifleHit = false;
	}
	else if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_HIT_END && !(m_Hit&DISABLE_HIT_RIFLE) && GameServer()->Collision()->GetSwitchDelay(MapIndex) == WEAPON_RIFLE)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCID(),"You can't shoot others with rifle");
		m_Hit |= DISABLE_HIT_RIFLE;
		m_Core.m_NoRifleHit = true;
	}
	else if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_JUMP)
	{
		int newJumps = GameServer()->Collision()->GetSwitchDelay(MapIndex);

		if (newJumps != m_Core.m_Jumps)
		{
			char aBuf[256];
			if(newJumps == 1)
				str_format(aBuf, sizeof(aBuf), "You can jump %d time", newJumps);
			else
				str_format(aBuf, sizeof(aBuf), "You can jump %d times", newJumps);
			GameServer()->SendChatTarget(GetPlayer()->GetCID(),aBuf);

			if (newJumps == 0 && !m_SuperJump)
			{
				m_NeededFaketuning |= FAKETUNE_NOJUMP;
				GameServer()->SendTuningParams(m_pPlayer->GetCID(), m_TuneZone); // update tunings
			}
			else if (m_Core.m_Jumps == 0)
			{
				m_NeededFaketuning &= ~FAKETUNE_NOJUMP;
				GameServer()->SendTuningParams(m_pPlayer->GetCID(), m_TuneZone); // update tunings
			}

			m_Core.m_Jumps = newJumps;
		}
	}
	else if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_PENALTY && !m_LastPenalty)
	{
		int min = GameServer()->Collision()->GetSwitchDelay(MapIndex);
		int sec = GameServer()->Collision()->GetSwitchNumber(MapIndex);
		int Team = Teams()->m_Core.Team(m_Core.m_Id);

		m_StartTime -= (min * 60 + sec) * Server()->TickSpeed();

		if (Team != TEAM_FLOCK && Team != TEAM_SUPER)
		{
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if(Teams()->m_Core.Team(i) == Team && i != m_Core.m_Id && GameServer()->m_apPlayers[i])
				{
					CCharacter* pChar = GameServer()->m_apPlayers[i]->GetCharacter();

					if (pChar)
						pChar->m_StartTime = m_StartTime;
				}
			}
		}

		m_LastPenalty = true;
	}
	else if(GameServer()->Collision()->IsSwitch(MapIndex) == TILE_BONUS && !m_LastBonus)
	{
		int min = GameServer()->Collision()->GetSwitchDelay(MapIndex);
		int sec = GameServer()->Collision()->GetSwitchNumber(MapIndex);
		int Team = Teams()->m_Core.Team(m_Core.m_Id);

		m_StartTime += (min * 60 + sec) * Server()->TickSpeed();
		if (m_StartTime > Server()->Tick())
			m_StartTime = Server()->Tick();

		if (Team != TEAM_FLOCK && Team != TEAM_SUPER)
		{
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if(Teams()->m_Core.Team(i) == Team && i != m_Core.m_Id && GameServer()->m_apPlayers[i])
				{
					CCharacter* pChar = GameServer()->m_apPlayers[i]->GetCharacter();

					if (pChar)
						pChar->m_StartTime = m_StartTime;
				}
			}
		}

		m_LastBonus = true;
	}

	if(GameServer()->Collision()->IsSwitch(MapIndex) != TILE_PENALTY)
	{
		m_LastPenalty = false;
	}

	if(GameServer()->Collision()->IsSwitch(MapIndex) != TILE_BONUS)
	{
		m_LastBonus = false;
	}

	int z = GameServer()->Collision()->IsTeleport(MapIndex);
	if(!g_Config.m_SvOldTeleportHook && !g_Config.m_SvOldTeleportWeapons && z && Controller->m_TeleOuts[z-1].size())
	{
		if (m_Super)
			return;
		int Num = Controller->m_TeleOuts[z-1].size();
		m_Core.m_Pos = Controller->m_TeleOuts[z-1][(!Num)?Num:rand() % Num];
		if(!g_Config.m_SvTeleportHoldHook)
		{
			m_Core.m_HookedPlayer = -1;
			m_Core.m_HookState = HOOK_RETRACTED;
			m_Core.m_TriggeredEvents |= COREEVENT_HOOK_RETRACT;
			m_Core.m_HookPos = m_Core.m_Pos;
		}
		if(g_Config.m_SvTeleportLoseWeapons)
		{
			for(int i=WEAPON_SHOTGUN;i<NUM_WEAPONS;i++)
				if (i != WEAPON_NINJA)
					m_aWeapons[i].m_Got = false;
		}
		return;
	}
	int evilz = GameServer()->Collision()->IsEvilTeleport(MapIndex);
	if(evilz && Controller->m_TeleOuts[evilz-1].size())
	{
		if (m_Super)
			return;
		int Num = Controller->m_TeleOuts[evilz-1].size();
		m_Core.m_Pos = Controller->m_TeleOuts[evilz-1][(!Num)?Num:rand() % Num];
		if (!g_Config.m_SvOldTeleportHook && !g_Config.m_SvOldTeleportWeapons)
		{
			m_Core.m_Vel = vec2(0,0);

			if(!g_Config.m_SvTeleportHoldHook)
			{
				m_Core.m_HookedPlayer = -1;
				m_Core.m_HookState = HOOK_RETRACTED;
				m_Core.m_TriggeredEvents |= COREEVENT_HOOK_RETRACT;
				GameWorld()->ReleaseHooked(GetPlayer()->GetCID());
				m_Core.m_HookPos = m_Core.m_Pos;
			}
			if(g_Config.m_SvTeleportLoseWeapons)
			{
				for(int i=WEAPON_SHOTGUN;i<NUM_WEAPONS;i++)
					if (i != WEAPON_NINJA)
						m_aWeapons[i].m_Got = false;
			}
		}
		return;
	}
	if(GameServer()->Collision()->IsCheckEvilTeleport(MapIndex))
	{
		if (m_Super)
			return;
		// first check if there is a TeleCheckOut for the current recorded checkpoint, if not check previous checkpoints
		for(int k=m_TeleCheckpoint-1; k >= 0; k--)
		{
			if(Controller->m_TeleCheckOuts[k].size())
			{
				int Num = Controller->m_TeleCheckOuts[k].size();
				m_Core.m_Pos = Controller->m_TeleCheckOuts[k][(!Num)?Num:rand() % Num];
				m_Core.m_Vel = vec2(0,0);

				if(!g_Config.m_SvTeleportHoldHook)
				{
					m_Core.m_HookedPlayer = -1;
					m_Core.m_HookState = HOOK_RETRACTED;
					m_Core.m_TriggeredEvents |= COREEVENT_HOOK_RETRACT;
					GameWorld()->ReleaseHooked(GetPlayer()->GetCID());
					m_Core.m_HookPos = m_Core.m_Pos;
				}

				return;
			}
		}
		// if no checkpointout have been found (or if there no recorded checkpoint), teleport to start
		vec2 SpawnPos;
		if(GameServer()->m_pController->CanSpawn(&SpawnPos, ENTITY_SPAWN))
		{
			m_Core.m_Pos = SpawnPos;
			m_Core.m_Vel = vec2(0,0);

			if(!g_Config.m_SvTeleportHoldHook)
			{
				m_Core.m_HookedPlayer = -1;
				m_Core.m_HookState = HOOK_RETRACTED;
				m_Core.m_TriggeredEvents |= COREEVENT_HOOK_RETRACT;
				GameWorld()->ReleaseHooked(GetPlayer()->GetCID());
				m_Core.m_HookPos = m_Core.m_Pos;
			}
		}
		return;
	}
	if(GameServer()->Collision()->IsCheckTeleport(MapIndex))
	{
		if (m_Super)
			return;
		// first check if there is a TeleCheckOut for the current recorded checkpoint, if not check previous checkpoints
		for(int k=m_TeleCheckpoint-1; k >= 0; k--)
		{
			if(Controller->m_TeleCheckOuts[k].size())
			{
				int Num = Controller->m_TeleCheckOuts[k].size();
				m_Core.m_Pos = Controller->m_TeleCheckOuts[k][(!Num)?Num:rand() % Num];

				if(!g_Config.m_SvTeleportHoldHook)
				{
					m_Core.m_HookedPlayer = -1;
					m_Core.m_HookState = HOOK_RETRACTED;
					m_Core.m_TriggeredEvents |= COREEVENT_HOOK_RETRACT;
					m_Core.m_HookPos = m_Core.m_Pos;
				}

				return;
			}
		}
		// if no checkpointout have been found (or if there no recorded checkpoint), teleport to start
		vec2 SpawnPos;
		if(GameServer()->m_pController->CanSpawn(&SpawnPos, ENTITY_SPAWN))
		{
			m_Core.m_Pos = SpawnPos;

			if(!g_Config.m_SvTeleportHoldHook)
			{
				m_Core.m_HookedPlayer = -1;
				m_Core.m_HookState = HOOK_RETRACTED;
				m_Core.m_TriggeredEvents |= COREEVENT_HOOK_RETRACT;
				m_Core.m_HookPos = m_Core.m_Pos;
			}
		}
		return;
	}
}

void CCharacter::HandleTuneLayer()
{
	m_TuneZoneOld = m_TuneZone;
	int CurrentIndex = GameServer()->Collision()->GetMapIndex(m_Pos);
	m_TuneZone = GameServer()->Collision()->IsTune(CurrentIndex);

	if(m_TuneZone)
		m_Core.m_pWorld->m_Tuning[g_Config.m_ClDummy] = GameServer()->TuningList()[m_TuneZone]; // throw tunings from specific zone into gamecore
	else
		m_Core.m_pWorld->m_Tuning[g_Config.m_ClDummy] = *GameServer()->Tuning();

	if (m_TuneZone != m_TuneZoneOld) // don't send tunigs all the time
	{
		// send zone msgs
		SendZoneMsgs();
	}
}

void CCharacter::SendZoneMsgs()
{
	// send zone leave msg
	// (m_TuneZoneOld >= 0: avoid zone leave msgs on spawn)
	if (m_TuneZoneOld >= 0 && GameServer()->m_aaZoneLeaveMsg[m_TuneZoneOld])
	{
		const char *pCur = GameServer()->m_aaZoneLeaveMsg[m_TuneZoneOld];
		const char *pPos;
		while ((pPos = str_find(pCur, "\\n")))
		{
			char aBuf[256];
			str_copy(aBuf, pCur, pPos - pCur + 1);
			aBuf[pPos - pCur + 1] = '\0';
			pCur = pPos + 2;
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		}
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), pCur);
	}
	// send zone enter msg
	if (GameServer()->m_aaZoneEnterMsg[m_TuneZone])
	{
		const char* pCur = GameServer()->m_aaZoneEnterMsg[m_TuneZone];
		const char* pPos;
		while ((pPos = str_find(pCur, "\\n")))
		{
			char aBuf[256];
			str_copy(aBuf, pCur, pPos - pCur + 1);
			aBuf[pPos - pCur + 1] = '\0';
			pCur = pPos + 2;
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), aBuf);
		}
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), pCur);
	}
}

void CCharacter::DDRaceTick()
{
	if (!m_pPlayer->m_IsDummy)
		mem_copy(&m_Input, &m_SavedInput, sizeof(m_Input));
	if(m_Input.m_Direction != 0 || m_Input.m_Jump != 0)
		m_LastMove = Server()->Tick();

	if(m_FreezeTime > 0 || m_FreezeTime == -1)
	{
		m_Armor = (m_FreezeTime >= 0) ? 10 - (m_FreezeTime / 15) : 0;
		if (m_FreezeTime % Server()->TickSpeed() == Server()->TickSpeed() - 1 || m_FreezeTime == -1)
		{
			GameServer()->CreateDamageInd(m_Pos, 0, (m_FreezeTime + 1) / Server()->TickSpeed(), Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));
		}
		if(m_FreezeTime > 0)
			m_FreezeTime--;
		else
			m_Ninja.m_ActivationTick = Server()->Tick();
		m_Input.m_Direction = 0;
		m_Input.m_Jump = 0;
		m_Input.m_Hook = 0;
		if (m_FreezeTime == 1)
			UnFreeze();
	}

	HandleTuneLayer(); // need this before coretick

	// look for save position for rescue feature
	if(g_Config.m_SvRescue) {
		int index = GameServer()->Collision()->GetPureMapIndex(m_Pos);
		int tile = GameServer()->Collision()->GetTileIndex(index);
		int ftile = GameServer()->Collision()->GetFTileIndex(index);
		if(IsGrounded() && tile != TILE_FREEZE && tile != TILE_DFREEZE && ftile != TILE_FREEZE && ftile != TILE_DFREEZE) {
			m_PrevSavePos = m_Pos;
			m_SetSavePos = true;
		}
	}

	CheckMoved();

	m_Core.m_Id = GetPlayer()->GetCID();
}


void CCharacter::DDRacePostCoreTick()
{
	// BlockDDrace
	m_IsFrozen = false;
	// BlockDDrace

	m_Time = (float)(Server()->Tick() - m_StartTime) / ((float)Server()->TickSpeed());

	if (m_pPlayer->m_DefEmoteReset >= 0 && m_pPlayer->m_DefEmoteReset <= Server()->Tick())
	{
		m_pPlayer->m_DefEmoteReset = -1;
		m_EmoteType = m_pPlayer->m_DefEmote = EMOTE_NORMAL;
		m_EmoteStop = -1;
	}

	if (m_EndlessHook || (m_Super && g_Config.m_SvEndlessSuperHook))
		m_Core.m_HookTick = 0;

	if (m_DeepFreeze && !m_Super)
		Freeze();

	if (m_Core.m_Jumps == 0 && !m_Super)
		m_Core.m_Jumped = 3;
	else if (m_Core.m_Jumps == 1 && m_Core.m_Jumped > 0)
		m_Core.m_Jumped = 3;
	else if (m_Core.m_JumpedTotal < m_Core.m_Jumps - 1 && m_Core.m_Jumped > 1)
		m_Core.m_Jumped = 1;

	if ((m_Super || m_SuperJump) && m_Core.m_Jumped > 1)
		m_Core.m_Jumped = 1;

	int CurrentIndex = GameServer()->Collision()->GetMapIndex(m_Pos);
	HandleSkippableTiles(CurrentIndex);

	// handle Anti-Skip tiles
	std::list < int > Indices = GameServer()->Collision()->GetMapIndices(m_PrevPos, m_Pos);
	if(!Indices.empty())
		for(std::list < int >::iterator i = Indices.begin(); i != Indices.end(); i++)
			HandleTiles(*i);
	else
	{
		HandleTiles(CurrentIndex);
		m_LastIndexTile = 0;
		m_LastIndexFrontTile = 0;
	}

	// teleport gun
	if (m_TeleGunTeleport)
	{
		GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCID(), Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));
		m_Core.m_Pos = m_TeleGunPos;
		if(!m_IsBlueTeleGunTeleport)
			m_Core.m_Vel = vec2(0, 0);
		GameServer()->CreateDeath(m_TeleGunPos, m_pPlayer->GetCID(), Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));
		GameServer()->CreateSound(m_TeleGunPos, SOUND_WEAPON_SPAWN, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));
		m_TeleGunTeleport = false;
		m_IsBlueTeleGunTeleport = false;
	}

	// BlockDDrace
	if (!m_IsFrozen)
		m_FirstFreezeTick = 0;
	// BlockDDrace

	HandleBroadcast();
}

bool CCharacter::Freeze(int Seconds)
{
	// BlockDDrace
	m_IsFrozen = true;

	if ((Seconds <= 0 || m_Super || m_FreezeTime == -1 || m_FreezeTime > Seconds * Server()->TickSpeed()) && Seconds != -1)
		 return false;
	if (m_FreezeTick < Server()->Tick() - Server()->TickSpeed() || Seconds == -1)
	{
		// BlockDDrace
		BackupWeapons(BACKUP_FREEZE);
		for (int i = 0; i < NUM_WEAPONS; i++)
			m_aWeapons[i].m_Ammo = 0;

		if (m_FreezeTick == 0 || m_FirstFreezeTick == 0)
			m_FirstFreezeTick = Server()->Tick();
		m_FreezeTime = Seconds == -1 ? Seconds : Seconds * Server()->TickSpeed();
		m_FreezeTick = Server()->Tick();
		return true;
	}
	return false;
}

bool CCharacter::Freeze()
{
	return Freeze(g_Config.m_SvFreezeDelay);
}

bool CCharacter::UnFreeze()
{
	if (m_FreezeTime > 0)
	{
		// BlockDDrace
		LoadWeaponBackup(BACKUP_FREEZE);

		if(!m_aWeapons[GetActiveWeapon()].m_Got)
			SetActiveWeapon(WEAPON_GUN);
		m_FreezeTime = 0;
		m_FreezeTick = 0;
		m_FirstFreezeTick = 0;
		m_Killer.m_ClientID = -1;
		m_Killer.m_Weapon = -1;
		if (GetActiveWeapon()==WEAPON_HAMMER) m_ReloadTimer = 0;
		return true;
	}
	return false;
}

void CCharacter::GiveWeapon(int Weapon, bool Remove, int Ammo)
{
	// BlockDDrace
	for (int i = 0; i < NUM_BACKUPS; i++)
	{
		m_aWeaponsBackupGot[Weapon][i] = !Remove;
		m_aWeaponsBackup[Weapon][i] = Ammo;
	}

	if (m_pPlayer->m_SpookyGhost)
		return;
	// BlockDDrace

	if (Weapon == WEAPON_NINJA)
	{
		if (Remove)
			RemoveNinja();
		else
			GiveNinja();
		return;
	}

	m_aWeapons[Weapon].m_Got = !Remove;

	if (Remove)
	{
		if (GetActiveWeapon() == Weapon)
			SetWeapon(WEAPON_GUN);
	}
	else
	{
		if (!m_FreezeTime)
			m_aWeapons[Weapon].m_Ammo = Ammo;
	}
}

void CCharacter::GiveAllWeapons()
{
	for(int i=WEAPON_GUN;i<NUM_WEAPONS;i++)
		if (i != WEAPON_NINJA)
			GiveWeapon(i);
}

void CCharacter::Pause(bool Pause)
{
	m_Paused = Pause;
	if(Pause)
	{
		GameWorld()->m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
		GameWorld()->RemoveEntity(this);

		if (m_Core.m_HookedPlayer != -1) // Keeping hook would allow cheats
		{
			m_Core.m_HookedPlayer = -1;
			m_Core.m_HookState = HOOK_RETRACTED;
			m_Core.m_TriggeredEvents |= COREEVENT_HOOK_RETRACT;
		}
	}
	else
	{
		m_Core.m_Vel = vec2(0,0);
		GameWorld()->m_Core.m_apCharacters[m_pPlayer->GetCID()] = &m_Core;
		GameWorld()->InsertEntity(this);
	}
}

void CCharacter::DDRaceInit()
{
	m_Paused = false;
	m_DDRaceState = DDRACE_NONE;
	m_PrevPos = m_Pos;
	m_SetSavePos = false;
	m_LastBroadcast = 0;
	m_TeamBeforeSuper = 0;
	m_Core.m_Id = GetPlayer()->GetCID();
	if(g_Config.m_SvTeam == 2)
	{
		GameServer()->SendChatTarget(GetPlayer()->GetCID(),"Please join a team before you start");
		m_LastStartWarning = Server()->Tick();
	}
	m_TeleCheckpoint = 0;
	m_EndlessHook = g_Config.m_SvEndlessDrag;
	m_Hit = g_Config.m_SvHit ? HIT_ALL : DISABLE_HIT_GRENADE|DISABLE_HIT_HAMMER|DISABLE_HIT_RIFLE|DISABLE_HIT_SHOTGUN;
	m_SuperJump = false;
	m_Jetpack = false;
	m_Core.m_Jumps = 2;
	m_FreezeHammer = false;

	int Team = Teams()->m_Core.Team(m_Core.m_Id);

	if(Teams()->TeamLocked(Team))
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if(Teams()->m_Core.Team(i) == Team && i != m_Core.m_Id && GameServer()->m_apPlayers[i])
			{
				CCharacter* pChar = GameServer()->m_apPlayers[i]->GetCharacter();

				if (pChar)
				{
					m_DDRaceState = pChar->m_DDRaceState;
					m_StartTime = pChar->m_StartTime;
				}
			}
		}
	}
}

void CCharacter::Rescue()
{
	if (m_SetSavePos && !m_Super && !m_DeepFreeze && IsGrounded() && m_Pos == m_PrevPos) {
		if (m_LastRescue + g_Config.m_SvRescueDelay * Server()->TickSpeed() > Server()->Tick())
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "You have to wait %d seconds until you can rescue yourself", (int)((m_LastRescue + g_Config.m_SvRescueDelay * Server()->TickSpeed() - Server()->Tick()) / Server()->TickSpeed()));
			GameServer()->SendChatTarget(GetPlayer()->GetCID(), aBuf);
			return;
		}

		int index = GameServer()->Collision()->GetPureMapIndex(m_Pos);
		if (GameServer()->Collision()->GetTileIndex(index) == TILE_FREEZE || GameServer()->Collision()->GetFTileIndex(index) == TILE_FREEZE) {
			m_LastRescue = Server()->Tick();
			m_Core.m_Pos = m_PrevSavePos;
			m_Pos = m_PrevSavePos;
			m_PrevPos = m_PrevSavePos;
			m_Core.m_Vel = vec2(0, 0);
			m_Core.m_HookedPlayer = -1;
			m_Core.m_HookState = HOOK_RETRACTED;
			m_Core.m_TriggeredEvents |= COREEVENT_HOOK_RETRACT;
			GameWorld()->ReleaseHooked(GetPlayer()->GetCID());
			m_Core.m_HookPos = m_Core.m_Pos;
			UnFreeze();
		}
	}
}


/*************************************************
*                                                *
*              B L O C K D D R A C E             *
*                                                *
**************************************************/

void CCharacter::BlockDDraceInit()
{
	m_LastIndexTile = 0;
	m_LastIndexFrontTile = 0;

	m_Killer.m_ClientID = -1;
	m_Killer.m_Weapon = -1;
	m_OldLastHookedPlayer = -1;

	m_NumGhostShots = 0;

	m_Invisible = false;
	m_Rainbow = false;
	m_Atom = false;
	m_Trail = false;
	m_Meteors = 0;
	m_Bloody = false;
	m_StrongBloody = false;
	m_ScrollNinja = false;
	m_HookPower = HOOK_NORMAL;
	for (int i = 0; i < NUM_WEAPONS; i++)
		m_aSpreadWeapon[i] = false;
	m_Passive = false;
	m_pPassiveShield = 0;
	m_PoliceHelper = false;
	m_TelekinesisTee = -1;
	m_pLightsaber = 0;
	m_Item = -3;
	m_pItem = 0;

	m_InShop = false;
	m_EnteredShop = false;

	int64 Now = Server()->Tick();
	m_ShopBotAntiSpamTick = Now;
	m_ShopWindowPage = SHOP_PAGE_NONE;
	m_ShopMotdTick = Now;
	m_PurchaseState = SHOP_STATE_NONE;

	SaveRealInfos();
	UnsetSpookyGhost();

	m_pPlayer->m_Gamemode = (g_Config.m_SvVanillaModeStart || m_pPlayer->m_Gamemode == GAMEMODE_VANILLA) ? GAMEMODE_VANILLA : GAMEMODE_DDRACE;
	m_Armor = m_pPlayer->m_Gamemode == GAMEMODE_VANILLA ? 0 : 10;

	m_FakeBlockTuning = false;
	m_FakeBlockUpState = 0;
	m_FakeBlockOldVelX = 0.f;
}

void CCharacter::BlockDDraceTick()
{
	// checking for body checks
	CCharacter *pChr = GameWorld()->ClosestCharacter(m_Pos, 20.0f, this);
	if (pChr && pChr->m_Pos.x < m_Core.m_Pos.x + 45 && pChr->m_Pos.x > m_Core.m_Pos.x - 45 && pChr->m_Pos.y < m_Core.m_Pos.y + 45 && pChr->m_Pos.y > m_Core.m_Pos.y - 45)
		if (pChr && !pChr->m_FreezeTime && CanCollide(pChr->GetPlayer()->GetCID()))
			m_Killer.m_ClientID = pChr->GetPlayer()->GetCID();

	// checking if someone hooks you, setting last touched tee
	m_pPlayer->IsHooked(-2);

	// reset last hit weapon if someone new hooks us
	if (m_Core.m_LastHookedPlayer != m_OldLastHookedPlayer)
		m_Killer.m_Weapon = -1;
	m_OldLastHookedPlayer = m_Core.m_LastHookedPlayer;

	// fix miss prediction for other players if one is in passive
	if (m_pPlayer->m_ClientVersion < VERSION_DDNET_KNOW_SOLO_PLAYERS) // the newer clients use the DDNet network character to know whether they can collide or not
	{
		CCharacter *pPas = GameWorld()->ClosestCharacter(m_Pos, 50.0f, this);
		if (pPas && (pPas->m_Passive || m_Passive) && !(pPas->m_Super || m_Super))
			m_NeededFaketuning |= FAKETUNE_NOCOLL;
		else if (!m_Passive)
			m_NeededFaketuning &= ~FAKETUNE_NOCOLL;
		GameServer()->SendTuningParams(m_pPlayer->GetCID(), m_TuneZone); // update tunings
	}

	if (m_ShopMotdTick < Server()->Tick())
	{
		m_ShopWindowPage = SHOP_PAGE_NONE;
		m_PurchaseState = SHOP_STATE_NONE;
	}

	if (m_InShop)
	{
		if (m_TileIndex != TILE_SHOP && m_TileFIndex != TILE_SHOP && m_TileIndex != ENTITY_SHOP_BOT_SPAWN && m_TileFIndex != ENTITY_SHOP_BOT_SPAWN)
		{
			if (m_ShopBotAntiSpamTick < Server()->Tick())
			{
				GameServer()->SendChat(GameWorld()->GetClosestShopBot(m_Pos, this, m_pPlayer->GetCID()), CGameContext::CHAT_SINGLE, "Bye! Come back if you need something.", -1, m_pPlayer->GetCID());
				m_ShopBotAntiSpamTick = Server()->Tick() + Server()->TickSpeed() * 5;
			}

			if (m_ShopWindowPage != SHOP_PAGE_NONE)
				GameServer()->SendMotd("", GetPlayer()->GetCID());

			GameServer()->SendBroadcast("", m_pPlayer->GetCID());

			m_PurchaseState = SHOP_STATE_NONE;
			m_ShopWindowPage = SHOP_PAGE_NONE;

			m_InShop = false;
		}
	}

	if (!m_AtomHooked && m_pPlayer->IsHooked(ATOM) && !m_Atom)
		new CAtom(GameWorld(), m_Pos, m_pPlayer->GetCID());
	if (!m_TrailHooked && m_pPlayer->IsHooked(TRAIL) && !m_Trail)
		new CTrail(GameWorld(), m_Pos, m_pPlayer->GetCID());

	m_AtomHooked = m_pPlayer->IsHooked(ATOM);
	m_TrailHooked = m_pPlayer->IsHooked(TRAIL);

	if (m_TelekinesisTee != -1)
	{
		CCharacter *pChr = GameServer()->GetPlayerChar(m_TelekinesisTee);
		if (pChr && GetActiveWeapon() == WEAPON_TELEKINESIS && !m_FreezeTime)
		{
			pChr->Core()->m_Pos = vec2(m_Pos.x+m_Input.m_TargetX, m_Pos.y+m_Input.m_TargetY);
			pChr->Core()->m_Vel = vec2(0.f, 0.f);
		}
		else
			m_TelekinesisTee = -1;
	}

	if (m_pLightsaber && (m_FreezeTime || GetActiveWeapon() != WEAPON_LIGHTSABER))
		m_pLightsaber->Retract();
}

void CCharacter::FakeBlockTick()
{
	enum
	{
		LEFT=1<<0,
		RIGHT=1<<1,
		DOWN=1<<2,
		UP=1<<3
	};

	int FakeCheckPoint = 0;
	if (GameServer()->Collision()->FakeCheckPoint(m_Pos.x-(m_ProximityRadius/2+5), m_Pos.y-m_ProximityRadius/2))
	{
		FakeCheckPoint |= LEFT;
	}
	if (GameServer()->Collision()->FakeCheckPoint(m_Pos.x+(m_ProximityRadius/2+5), m_Pos.y-m_ProximityRadius/2))
	{
		FakeCheckPoint |= RIGHT;
	}
	if (GameServer()->Collision()->FakeCheckPoint(m_Pos.x+m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5) ||
	GameServer()->Collision()->FakeCheckPoint(m_Pos.x-m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
	{
		FakeCheckPoint |= DOWN;
		m_Core.m_Pos.y -= 0.001f;
		m_Core.m_Vel.y = 0;
	}
	if (GameServer()->Collision()->FakeCheckPoint(m_Pos.x+m_ProximityRadius/2, m_Pos.y-(m_ProximityRadius/2+5)) ||
	GameServer()->Collision()->FakeCheckPoint(m_Pos.x-m_ProximityRadius/2, m_Pos.y-(m_ProximityRadius/2+5)))
	{
		FakeCheckPoint |= UP;
		if (m_FakeBlockUpState == 0)
		{
			m_FakeBlockOldVelX = m_Core.m_Vel.x;
			m_Core.m_Vel = vec2(0.f, 0.f);
			m_FakeBlockUpState = 1;
		}
		else if (m_FakeBlockUpState == 1)
		{
			m_Core.m_Vel.x = m_FakeBlockOldVelX;
			m_FakeBlockUpState = 2;
		}
	}

	if ((m_Input.m_Direction == -1 && FakeCheckPoint & LEFT) || (m_Input.m_Direction == 1 && FakeCheckPoint & RIGHT))
		m_Input.m_Direction = 0;

	if(!m_FakeBlockTuning && FakeCheckPoint != 0)
	{
		static CTuningParams FakeTuning;

		if (!m_TuneZone)
			FakeTuning = *GameServer()->Tuning();
		else
			FakeTuning = GameServer()->TuningList()[m_TuneZone];

		if (FakeCheckPoint & LEFT || FakeCheckPoint & RIGHT)
		{
			FakeTuning.m_GroundControlSpeed = 0.f;
			FakeTuning.m_GroundControlAccel = 0.f;
			FakeTuning.m_AirControlSpeed = 0.f;
			FakeTuning.m_AirControlAccel = 0.f;
		}
		if (FakeCheckPoint & DOWN)
		{
			FakeTuning.m_Gravity = 0.f;
		}

		CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
		int *pParams = (int *)&FakeTuning;
		for (unsigned i = 0; i < sizeof(FakeTuning) / sizeof(int); i++)
			Msg.AddInt(pParams[i]);
		Server()->SendMsg(&Msg, MSGFLAG_VITAL, m_pPlayer->GetCID());

		m_FakeBlockTuning = true;
	}
	else if (m_FakeBlockTuning && FakeCheckPoint == 0)
	{
		GameServer()->SendTuningParams(m_pPlayer->GetCID(), m_TuneZone);
		m_FakeBlockTuning = false;
		m_FakeBlockUpState = 0;
	}
}

void CCharacter::BackupWeapons(int Type)
{
	if (!m_WeaponsBackupped[Type])
	{
		for (int i = 0; i < NUM_WEAPONS_BACKUP; i++)
		{
			if (i == BACKUP_HEALTH)
				m_aWeaponsBackup[i][Type] = m_Health;
			else if (i == BACKUP_ARMOR)
				m_aWeaponsBackup[i][Type] = m_Armor;
			else
			{
				m_aWeaponsBackupGot[i][Type] = m_aWeapons[i].m_Got;
				m_aWeaponsBackup[i][Type] = m_aWeapons[i].m_Ammo;
			}
		}
		m_WeaponsBackupped[Type] = true;
	}
}

void CCharacter::LoadWeaponBackup(int Type)
{
	if (m_WeaponsBackupped[Type])
	{
		for (int i = 0; i < NUM_WEAPONS_BACKUP; i++)
		{
			if (i == BACKUP_HEALTH)
				m_Health = m_aWeaponsBackup[i][Type];
			else if (i == BACKUP_ARMOR)
				m_Armor = m_aWeaponsBackup[i][Type];
			else
			{
				m_aWeapons[i].m_Got = m_aWeaponsBackupGot[i][Type];
				m_aWeapons[i].m_Ammo = m_aWeaponsBackup[i][Type];
				if (i == WEAPON_NINJA)
					m_aWeapons[i].m_Ammo = -1;
			}
		}
		m_WeaponsBackupped[Type] = false;
	}
}

void CCharacter::SetAvailableWeapon(int PreferedWeapon)
{
	if (GetWeaponGot(PreferedWeapon))
		SetWeapon(PreferedWeapon);
	else for (int i = NUM_WEAPONS; i > -1; i--)
	{
		if (i != WEAPON_NINJA)
			if (GetWeaponGot(i))
				SetWeapon(i);
	}
	UpdateWeaponIndicator();
}

void CCharacter::DropFlag()
{
	for (int i = 0; i < 2; i++)
	{
		CFlag *F = ((CGameControllerBlockDDrace*)GameServer()->m_pController)->m_apFlags[i];
		if (!F)
			continue;

		if (F->GetCarrier() == this)
			F->Drop(GetAimDir());
	}
}

void CCharacter::DropWeapon(int WeaponID, float Dir, bool Forced)
{
	if ((m_FreezeTime && !Forced) || !g_Config.m_SvDropWeapons || g_Config.m_SvMaxWeaponDrops == 0 || WeaponID == WEAPON_NINJA || !m_aWeapons[WeaponID].m_Got)
		return;

	if (m_pPlayer->m_vWeaponLimit[WeaponID].size() == (unsigned)g_Config.m_SvMaxWeaponDrops)
	{
		m_pPlayer->m_vWeaponLimit[WeaponID][0]->Reset(false, false);
		m_pPlayer->m_vWeaponLimit[WeaponID].erase(m_pPlayer->m_vWeaponLimit[WeaponID].begin());
	}

	int WeaponCount = 0;
	for (int i = 0; i < NUM_WEAPONS; i++)
	{
		if (i == WEAPON_NINJA)
			continue;
		if (m_aWeapons[i].m_Got)
			WeaponCount++;
	}

	if (WeaponCount > 1 || (WeaponID == WEAPON_GUN && m_Jetpack))
	{
		GameServer()->CreateSound(m_Pos, SOUND_WEAPON_NOAMMO, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));
		CPickupDrop *Weapon = new CPickupDrop(GameWorld(), POWERUP_WEAPON, m_pPlayer->GetCID(), Dir == -3 ? GetAimDir() : Dir, WeaponID, 300, GetWeaponAmmo(WeaponID), m_aSpreadWeapon[WeaponID], (WeaponID == WEAPON_GUN && m_Jetpack));
		m_pPlayer->m_vWeaponLimit[WeaponID].push_back(Weapon);

		if (WeaponID != WEAPON_GUN || !m_Jetpack)
		{
			m_aWeapons[WeaponID].m_Got = false;
			SetWeapon(WEAPON_GUN);
		}
		if (m_aSpreadWeapon[WeaponID])
			SpreadWeapon(WeaponID, false);
		if (WeaponID == WEAPON_GUN && m_Jetpack)
			Jetpack(false);
	}
}

void CCharacter::DropPickup(int Type, int Amount)
{
	if (Type > POWERUP_ARMOR || g_Config.m_SvMaxPickupDrops == 0 || Amount <= 0)
		return;

	for (int i = 0; i < Amount; i++)
	{
		if (GameServer()->m_vPickupDropLimit.size() == (unsigned)g_Config.m_SvMaxPickupDrops)
		{
			GameServer()->m_vPickupDropLimit[0]->Reset();
			GameServer()->m_vPickupDropLimit.erase(GameServer()->m_vPickupDropLimit.begin());
		}
		float Dir = ((rand() % 50 - 25) * 0.1); // in a range of -2.5 to +2.5
		CPickupDrop *PickupDrop = new CPickupDrop(&GameServer()->m_World, Type, m_pPlayer->GetCID(), Dir);
		GameServer()->m_vPickupDropLimit.push_back(PickupDrop);
	}
	GameServer()->CreateSound(m_Pos, Type == POWERUP_HEALTH ? SOUND_PICKUP_HEALTH : SOUND_PICKUP_ARMOR, Teams()->TeamMask(Team(), -1, m_pPlayer->GetCID()));
}

void CCharacter::DropLoot()
{
	if (!g_Config.m_SvDropsOnDeath)
		return;

	if (m_pPlayer->m_Minigame == MINIGAME_SURVIVAL)
	{
		if (m_pPlayer->m_SurvivalState <= SURVIVAL_LOBBY)
			return;

		// drop 0 to 5 armor and hearts
		DropPickup(POWERUP_HEALTH, rand() % 6);
		DropPickup(POWERUP_ARMOR, rand() % 6);

		// drop all your weapons, in various directions (excluding hammer, ninja and extra weapons)
		for (int i = WEAPON_GUN; i < NUM_VANILLA_WEAPONS-1; i++)
		{
			float Dir = ((rand() % 50 - 25 + 1) * 0.1); // in a range of -2.5 to +2.5
			DropWeapon(i, Dir, true);
		}
	}
	else if (m_pPlayer->m_Minigame == MINIGAME_NONE)
	{
		for (int i = 0; i < 2; i++)
		{
			int Weapon = WEAPON_GUN;
			while (Weapon == WEAPON_NINJA || Weapon == WEAPON_GUN || Weapon == WEAPON_HAMMER)
				Weapon = rand() % NUM_WEAPONS;
			float Dir = ((rand() % 50 - 25 + 1) * 0.1); // in a range of -2.5 to +2.5
			DropWeapon(Weapon, Dir);
		}
	}
}

void CCharacter::SetSpookyGhost()
{
	BackupWeapons(BACKUP_SPOOKY_GHOST);
	for (int i = 0; i < NUM_WEAPONS; i++)
		if (GameServer()->GetRealWeapon(i) != WEAPON_GUN)
			m_aWeapons[i].m_Got = false;
	str_copy(m_pPlayer->m_TeeInfos.m_SkinName, "ghost", sizeof(m_pPlayer->m_TeeInfos.m_SkinName));
	m_pPlayer->m_TeeInfos.m_UseCustomColor = 0;
	m_pPlayer->m_SpookyGhost = true;
}

void CCharacter::UnsetSpookyGhost()
{
	LoadWeaponBackup(BACKUP_SPOOKY_GHOST);
	str_copy(m_pPlayer->m_TeeInfos.m_SkinName, m_pPlayer->m_RealSkinName, sizeof(m_pPlayer->m_TeeInfos.m_SkinName));
	m_pPlayer->m_TeeInfos.m_UseCustomColor = m_pPlayer->m_RealUseCustomColor;
	m_pPlayer->m_SpookyGhost = false;
}

void CCharacter::SaveRealInfos()
{
	if (m_pPlayer->m_SpookyGhost)
		return;

	str_copy(m_pPlayer->m_RealSkinName, m_pPlayer->m_TeeInfos.m_SkinName, sizeof(m_pPlayer->m_RealSkinName));
	m_pPlayer->m_RealUseCustomColor = m_pPlayer->m_TeeInfos.m_UseCustomColor;
}

void CCharacter::SetActiveWeapon(int Weapon)
{
	m_ActiveWeapon = Weapon;

	if (Weapon == WEAPON_PLASMA_RIFLE)
		m_Core.m_ActiveWeapon = WEAPON_RIFLE;
	else if (Weapon == WEAPON_HEART_GUN)
		m_Core.m_ActiveWeapon = WEAPON_GUN;
	else if (Weapon == WEAPON_STRAIGHT_GRENADE)
		m_Core.m_ActiveWeapon = WEAPON_GRENADE;
	else if (Weapon == WEAPON_TELEKINESIS)
		m_Core.m_ActiveWeapon = WEAPON_NINJA;
	else if (Weapon == WEAPON_LIGHTSABER)
		m_Core.m_ActiveWeapon = WEAPON_GUN;
	else
		m_Core.m_ActiveWeapon = Weapon;

	UpdateWeaponIndicator();
}

bool CCharacter::GetWeaponGot(int Type)
{
	if (m_FreezeTime)
		return m_aWeaponsBackupGot[Type][BACKUP_FREEZE];
	return m_aWeapons[Type].m_Got;
}

int CCharacter::GetWeaponAmmo(int Type)
{
	if (m_FreezeTime)
		return m_aWeaponsBackup[Type][BACKUP_FREEZE];
	return m_aWeapons[Type].m_Ammo;
}

void CCharacter::SetWeaponGot(int Type, bool Value)
{
	if (m_FreezeTime)
	{
		m_aWeaponsBackupGot[Type][BACKUP_FREEZE] = Value;
		return;
	}
	m_aWeapons[Type].m_Got = Value;
}

void CCharacter::SetWeaponAmmo(int Type, int Value)
{
	if (m_FreezeTime)
	{
		m_aWeaponsBackup[Type][BACKUP_FREEZE] = Value;
		return;
	}
	m_aWeapons[Type].m_Ammo = Value;
}

void CCharacter::UpdateWeaponIndicator()
{
	if (!m_pPlayer->m_WeaponIndicator || (m_pPlayer->m_Minigame == MINIGAME_SURVIVAL && GameServer()->m_SurvivalBackgroundState < BACKGROUND_DEATHMATCH_COUNTDOWN))
		return;

	char aBuf[256];
	char aSpaces[128];
	str_format(aSpaces, sizeof(aSpaces), "                                                                                                                               ");
	str_format(aBuf, sizeof(aBuf), "Weapon: %s%s", GameServer()->GetWeaponName(GetActiveWeapon()), aSpaces);
	GameServer()->SendBroadcast(aBuf, m_pPlayer->GetCID(), false);
}

int CCharacter::HasFlag()
{
	return ((CGameControllerBlockDDrace*)GameServer()->m_pController)->HasFlag(this);
}

void CCharacter::CheckMoved()
{
	if (!m_pPlayer->m_ResumeMoved || !m_pPlayer->IsPaused() || m_PrevPos == m_Pos)
		return;

	m_pPlayer->Pause(CPlayer::PAUSE_NONE, false);
}

void CCharacter::Jetpack(bool Set, int FromID, bool Silent)
{
	m_Jetpack = Set;
	m_Core.m_Jetpack = m_Jetpack;
	GameServer()->SendExtraMessage(JETPACK, m_pPlayer->GetCID(), Set, FromID, Silent);
}

void CCharacter::Rainbow(bool Set, int FromID, bool Silent)
{
	m_Rainbow = Set;
	m_pPlayer->m_InfRainbow = false;
	GameServer()->SendExtraMessage(RAINBOW, m_pPlayer->GetCID(), Set, FromID, Silent);
}

void CCharacter::InfRainbow(bool Set, int FromID, bool Silent)
{
	m_pPlayer->m_InfRainbow = Set;
	m_Rainbow = false;
	GameServer()->SendExtraMessage(INF_RAINBOW, m_pPlayer->GetCID(), Set, FromID, Silent);
}

void CCharacter::Atom(bool Set, int FromID, bool Silent)
{
	m_Atom = Set;
	if (Set)
		new CAtom(GameWorld(), m_Pos, m_pPlayer->GetCID());
	GameServer()->SendExtraMessage(ATOM, m_pPlayer->GetCID(), Set, FromID, Silent);
}

void CCharacter::Trail(bool Set, int FromID, bool Silent)
{
	m_Trail = Set;
	if (Set)
		new CTrail(GameWorld(), m_Pos, m_pPlayer->GetCID());
	GameServer()->SendExtraMessage(TRAIL, m_pPlayer->GetCID(), Set, FromID, Silent);
}

void CCharacter::SpookyGhost(bool Set, int FromID, bool Silent)
{
	m_pPlayer->m_HasSpookyGhost = Set;
	GameServer()->SendExtraMessage(EXTRA_SPOOKY_GHOST, m_pPlayer->GetCID(), Set, FromID, Silent);
	if (!Silent && Set)
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "For more info, say '/spookyghost'");
}

void CCharacter::Meteor(bool Set, int FromID, bool Infinite, bool Silent)
{
	if (Set)
	{
		if (m_pPlayer->m_InfMeteors + m_Meteors >= 50)
		{
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "You already have the maximum of 50 meteors");
			return;
		}

		vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));
		vec2 ProjStartPos = m_Pos + Direction * m_ProximityRadius*0.75f;

		Infinite ? m_pPlayer->m_InfMeteors++ : m_Meteors++;
		new CMeteor(GameWorld(), ProjStartPos, m_pPlayer->GetCID(), Infinite);
	}
	else
	{
		if (!m_Meteors && !m_pPlayer->m_InfMeteors)
			return;

		m_Meteors = 0;
		m_pPlayer->m_InfMeteors = 0;
	}
	GameServer()->SendExtraMessage(Infinite ? INF_METEOR : METEOR, m_pPlayer->GetCID(), Set, FromID, Silent);
}

void CCharacter::Passive(bool Set, int FromID, bool Silent)
{
	m_Passive = Set;
	m_Core.m_Passive = Set;
	Teams()->m_Core.SetPassive(m_pPlayer->GetCID(), Set);
	PassiveCollision(Set);

	m_pPassiveShield = !Set ? 0 : new CPickup(GameWorld(), POWERUP_ARMOR, 0, 0, 0, m_pPlayer->GetCID());
	GameServer()->SendExtraMessage(PASSIVE, m_pPlayer->GetCID(), Set, FromID, Silent);
}

void CCharacter::PassiveCollision(bool Set)
{
	if (Set)
	{
		m_NeededFaketuning |= FAKETUNE_NOCOLL;
		m_NeededFaketuning |= FAKETUNE_NOHAMMER;
	}
	else
	{
		m_NeededFaketuning &= ~FAKETUNE_NOCOLL;
		m_NeededFaketuning &= ~FAKETUNE_NOHAMMER;
	}

	m_Core.m_Collision = !Set;
	m_Core.m_NoCollision = Set;
	m_Hit = Set ? (DISABLE_HIT_GRENADE|DISABLE_HIT_HAMMER|DISABLE_HIT_RIFLE|DISABLE_HIT_SHOTGUN) : HIT_ALL;
	m_Core.m_NoShotgunHit = Set;
	m_Core.m_NoGrenadeHit = Set;
	m_Core.m_NoHammerHit = Set;
	m_Core.m_NoRifleHit = Set;
	m_Core.m_Hook = !Set;
	m_Core.m_NoHookHit = Set;

	GameServer()->SendTuningParams(m_pPlayer->GetCID(), m_TuneZone); // update tunings
}

void CCharacter::VanillaMode(int FromID, bool Silent)
{
	if (m_pPlayer->m_Gamemode == GAMEMODE_VANILLA)
		return;

	m_pPlayer->m_Gamemode = GAMEMODE_VANILLA;
	m_Armor = 0;
	for (int j = 0; j < NUM_BACKUPS; j++)
	{
		for (int i = 0; i < NUM_WEAPONS_BACKUP; i++)
		{
			if (i == BACKUP_HEALTH)
				m_aWeaponsBackup[i][j] = 10;
			else if (i == BACKUP_ARMOR)
				m_aWeaponsBackup[i][j] = 0;
			else if (i != WEAPON_HAMMER)
			{
				m_aWeaponsBackup[i][j] = 10;
				if (!m_FreezeTime)
					m_aWeapons[i].m_Ammo = 10;
			}
		}
	}
	GameServer()->SendExtraMessage(VANILLA_MODE, m_pPlayer->GetCID(), true, FromID, Silent);
}

void CCharacter::DDraceMode(int FromID, bool Silent)
{
	if (m_pPlayer->m_Gamemode == GAMEMODE_DDRACE)
		return;

	m_pPlayer->m_Gamemode = GAMEMODE_DDRACE;
	m_Health = 10;
	m_Armor = 10;
	for (int j = 0; j < NUM_BACKUPS; j++)
	{
		for (int i = 0; i < NUM_WEAPONS_BACKUP; i++)
		{
			if (i == BACKUP_HEALTH)
				m_aWeaponsBackup[i][j] = 10;
			else if (i == BACKUP_ARMOR)
				m_aWeaponsBackup[i][j] = 10;
			else
			{
				m_aWeaponsBackup[i][j] = -1;
				if (!m_FreezeTime)
					m_aWeapons[i].m_Ammo = -1;
			}
		}
	}
	GameServer()->SendExtraMessage(DDRACE_MODE, m_pPlayer->GetCID(), true, FromID, Silent);
}

void CCharacter::Bloody(bool Set, int FromID, bool Silent)
{
	m_Bloody = Set;
	m_StrongBloody = false;
	GameServer()->SendExtraMessage(BLOODY, m_pPlayer->GetCID(), Set, FromID, Silent);
}

void CCharacter::StrongBloody(bool Set, int FromID, bool Silent)
{
	m_StrongBloody = Set;
	m_Bloody = false;
	GameServer()->SendExtraMessage(STRONG_BLOODY, m_pPlayer->GetCID(), Set, FromID, Silent);
}

void CCharacter::PoliceHelper(bool Set, int FromID, bool Silent)
{
	m_PoliceHelper = Set;
	GameServer()->SendExtraMessage(POLICE_HELPER, m_pPlayer->GetCID(), Set, FromID, Silent);
}

void CCharacter::ScrollNinja(bool Set, int FromID, bool Silent)
{
	m_ScrollNinja = Set;
	if (Set)
		GiveNinja();
	else
		RemoveNinja();
	GameServer()->SendExtraMessage(SCROLL_NINJA, m_pPlayer->GetCID(), Set, FromID, Silent);
}

void CCharacter::HookPower(int Extra, int FromID, bool Silent)
{
	if (m_HookPower == HOOK_NORMAL && Extra == HOOK_NORMAL)
		return;
	m_HookPower = Extra;
	GameServer()->SendExtraMessage(HOOK_POWER, m_pPlayer->GetCID(), true, FromID, Silent, Extra);
}

void CCharacter::EndlessHook(bool Set, int FromID, bool Silent)
{
	m_EndlessHook = Set;
	m_Core.m_EndlessHook = Set;
	GameServer()->SendExtraMessage(ENDLESS_HOOK, m_pPlayer->GetCID(), Set, FromID, Silent);
}

void CCharacter::InfiniteJumps(bool Set, int FromID, bool Silent)
{
	m_SuperJump = Set;
	m_Core.m_EndlessJump = Set;
	if (m_Core.m_Jumps == 0)
	{
		if (Set)
			m_NeededFaketuning &= ~FAKETUNE_NOJUMP;
		else
			m_NeededFaketuning |= FAKETUNE_NOJUMP;
		GameServer()->SendTuningParams(m_pPlayer->GetCID(), m_TuneZone); // update tunings
	}
	GameServer()->SendExtraMessage(INFINITE_JUMPS, m_pPlayer->GetCID(), Set, FromID, Silent);
}

void CCharacter::SpreadWeapon(int Type, bool Set, int FromID, bool Silent)
{
	if (Type == WEAPON_HAMMER || Type == WEAPON_NINJA)
		return;
	m_aSpreadWeapon[Type] = Set;
	GameServer()->SendExtraMessage(SPREAD_WEAPON, m_pPlayer->GetCID(), Set, FromID, Silent, Type);
}

void CCharacter::FreezeHammer(bool Set, int FromID, bool Silent)
{
	m_FreezeHammer = Set;
	GameServer()->SendExtraMessage(FREEZE_HAMMER, m_pPlayer->GetCID(), Set, FromID, Silent);
}

void CCharacter::Invisible(bool Set, int FromID, bool Silent)
{
	m_Invisible = Set;
	GameServer()->SendExtraMessage(INVISIBLE, m_pPlayer->GetCID(), Set, FromID, Silent);
}

void CCharacter::Item(int Item, int FromID, bool Silent)
{
	if ((m_Item == -3 && Item == -3) || Item >= NUM_WEAPONS || Item < -3)
		return;
	int Type = Item == -2 ? POWERUP_HEALTH : Item == -1 ? POWERUP_ARMOR : Item >= 0 ? POWERUP_WEAPON : 0;
	int SubType = Item >= 0 ? Item : 0;
	m_Item = Item;
	m_pItem = Item == -3 ? 0 : new CPickup(GameWorld(), Type, SubType, 0, 0, m_pPlayer->GetCID());
	GameServer()->SendExtraMessage(ITEM, m_pPlayer->GetCID(), Item == -3 ? false : true, FromID, Silent, Item);
}
