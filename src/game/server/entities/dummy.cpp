#include "character.h"
#include <game/server/player.h>
#include "dummy.h"

void CCharacter::DummyTick()
{
	if (!m_pPlayer->m_IsDummy)
		return;

	ResetInput();
	m_Input.m_Hook = 0;

	if (m_pPlayer->m_Dummymode == 0)
	{
		// do nothing
	}
	else if (m_pPlayer->m_Dummymode == -6)  //ChillBlock5 blmapv3 1o1 mode
	{
		CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this);
		if (pChr && pChr->IsAlive())
		{
			m_Input.m_TargetX = pChr->m_Pos.x - m_Pos.x;
			m_Input.m_TargetY = pChr->m_Pos.y - m_Pos.y;
			m_LatestInput.m_TargetX = pChr->m_Pos.x - m_Pos.x;
			m_LatestInput.m_TargetY = pChr->m_Pos.y - m_Pos.y;

			/*************************************************
			*                                                *
			*                A T T A C K                     *
			*                                                *
			**************************************************/

			//swing enemy up
			if (m_Core.m_Pos.y < pChr->m_Pos.y - 20 && !IsGrounded() && !pChr->isFreezed)
			{
				m_Input.m_Hook = 1;
				float dist = distance(pChr->m_Pos, m_Core.m_Pos);
				if (dist < 250.f)
				{
					if (m_Core.m_Pos.x < pChr->m_Pos.x)
						m_Input.m_Direction = -1;
					else
						m_Input.m_Direction = 1;
					if (dist < 80.f) // hammer dist
					{
						if (absolute(pChr->m_Core.m_Vel.x) > 2.6f)
						{
							if (m_FreezeTime == 0)
							{
								m_LatestInput.m_Fire++;
								m_Input.m_Fire++;
							}
						}
					}
				}
			}

			//attack in mid
			if (pChr->m_Pos.x > 393 * 32 - 7 + V3_OFFSET_X && pChr->m_Pos.x < 396 * 32 + 7 + V3_OFFSET_X)
			{
				if (pChr->m_Pos.x < m_Core.m_Pos.x) // bot on the left
				{
					if (pChr->m_Core.m_Vel.x < 0.0f)
						m_Input.m_Hook = 1;
					else
						m_Input.m_Hook = 0;
				}
				else // bot on the right
				{
					if (pChr->m_Core.m_Vel.x < 0.0f)
						m_Input.m_Hook = 0;
					else
						m_Input.m_Hook = 1;
				}
				if (pChr->isFreezed)
					m_Input.m_Hook = 0;
			}

			//attack bot in the middle and enemy in the air -> try to hook down
			if (m_Core.m_Pos.y < 78 * 32 + V3_OFFSET_Y && m_Core.m_Pos.y > 70 * 32 + V3_OFFSET_Y && IsGrounded()) // if bot is in position
			{
				if (pChr->m_Pos.x < 389 * 32 + V3_OFFSET_X || pChr->m_Pos.x > 400 * 32 + V3_OFFSET_X) //enemy on the left side
				{
					if (pChr->m_Pos.y < 76 * 32 + V3_OFFSET_Y && pChr->m_Core.m_Vel.y > 4.2f)
						m_Input.m_Hook = 1;
				}

				if (m_Core.m_HookState == HOOK_FLYING)
					m_Input.m_Hook = 1;
				else if (m_Core.m_HookState == HOOK_GRABBED)
				{
					m_Input.m_Hook = 1;
					//stay strong and walk agianst hook pull
					if (m_Core.m_Pos.x < 392 * 32 + V3_OFFSET_X) //left side
						m_Input.m_Direction = 1;
					else if (m_Core.m_Pos.x > 397 * 32 + V3_OFFSET_X) //right side
						m_Input.m_Direction = -1;
				}
			}

			// attack throw into left freeze wall
			if (m_Core.m_Pos.x < 383 * 32 + V3_OFFSET_X)
			{
				if (pChr->m_Pos.y > m_Core.m_Pos.y + 190)
					m_Input.m_Hook = 1;
				else if (pChr->m_Pos.y < m_Core.m_Pos.y - 190)
					m_Input.m_Hook = 1;
				else
				{
					if (pChr->m_Core.m_Vel.x < -1.6f)
					{
						if (pChr->m_Pos.x < m_Core.m_Pos.x - 7 && pChr->m_Pos.x > m_Core.m_Pos.x - 90) //enemy on the left side
						{
							if (pChr->m_Pos.y < m_Core.m_Pos.y + 90 && pChr->m_Pos.y > m_Core.m_Pos.y - 90)
							{
								if (m_FreezeTime == 0)
								{
									m_LatestInput.m_Fire++;
									m_Input.m_Fire++;
								}
							}
						}
					}
				}
			}

			// attack throw into right freeze wall
			if (m_Core.m_Pos.x > 404 * 32 + V3_OFFSET_X)
			{
				if (pChr->m_Pos.y > m_Core.m_Pos.y + 190)
					m_Input.m_Hook = 1;
				else if (pChr->m_Pos.y < m_Core.m_Pos.y - 190)
					m_Input.m_Hook = 1;
				else
				{
					if (pChr->m_Core.m_Vel.x > 1.6f)
					{
						if (pChr->m_Pos.x > m_Core.m_Pos.x + 7 && pChr->m_Pos.x < m_Core.m_Pos.x + 90) //enemy on the right side
						{
							if (pChr->m_Pos.y > m_Core.m_Pos.y - 90 && pChr->m_Pos.y < m_Core.m_Pos.y + 90)
							{
								if (m_FreezeTime == 0)
								{
									m_LatestInput.m_Fire++;
									m_Input.m_Fire++;
								}
							}
						}
					}
				}
			}

			/*************************************************
			*                                                *
			*                D E F E N D (move)              *
			*                                                *
			**************************************************/

			//########################################
			//Worst hammer switch code eu west rofl! #
			//########################################
			//switch to hammer if enemy is near enough
			if ((pChr->m_Pos.x > m_Core.m_Pos.x + 130) || (pChr->m_Pos.x < m_Core.m_Pos.x - 130)) //default is gun
				SetWeapon(1);
			else
			{
				//switch to hammer if enemy is near enough
				if ((pChr->m_Pos.y > m_Core.m_Pos.y + 130) || (pChr->m_Pos.y < m_Core.m_Pos.y - 130)) //default is gun
					SetWeapon(1);
				else //near --> hammer
					SetWeapon(0);
			}

			//Starty movement
			if (m_Core.m_Pos.x < 389 * 32 + V3_OFFSET_X && m_Core.m_Pos.y > 79 * 32 + V3_OFFSET_Y && pChr->m_Pos.y > 79 * 32 + V3_OFFSET_Y && pChr->m_Pos.x > 398 * 32 + V3_OFFSET_X && IsGrounded() && pChr->IsGrounded())
				m_Input.m_Jump = 1;
			if (m_Core.m_Pos.x < 389 * 32 + V3_OFFSET_X && pChr->m_Pos.x > 307 * 32 + V3_OFFSET_X && pChr->m_Pos.x > 398 * 32 + V3_OFFSET_X)
				m_Input.m_Direction = 1;

			//important freeze doges leave them last!:

			//freeze prevention mainpart down right
			if (m_Core.m_Pos.x > 397 * 32 + V3_OFFSET_X && m_Core.m_Pos.x < 401 * 32 + V3_OFFSET_X && m_Core.m_Pos.y > 78 * 32 + V3_OFFSET_Y)
			{
				m_Input.m_Jump = 1;
				m_Input.m_Hook = 1;
				if (Server()->Tick() % 20 == 0)
				{
					m_Input.m_Hook = 0;
					m_Input.m_Jump = 0;
				}
				m_Input.m_Direction = 1;
				m_Input.m_TargetX = 200;
				m_Input.m_TargetY = 80;
			}

			//freeze prevention mainpart down left
			if (m_Core.m_Pos.x > 387 * 32 + V3_OFFSET_X && m_Core.m_Pos.x < 391 * 32 + V3_OFFSET_X && m_Core.m_Pos.y > 78 * 32 + V3_OFFSET_Y)
			{
				m_Input.m_Jump = 1;
				m_Input.m_Hook = 1;
				if (Server()->Tick() % 20 == 0)
				{
					m_Input.m_Hook = 0;
					m_Input.m_Jump = 0;
				}
				m_Input.m_Direction = -1;
				m_Input.m_TargetX = -200;
				m_Input.m_TargetY = 80;
			}

			//Freeze prevention top left
			if (m_Core.m_Pos.x < 391 * 32 + V3_OFFSET_X && m_Core.m_Pos.y < 71 * 32 + V3_OFFSET_Y && m_Core.m_Pos.x > 387 * 32 - 10 + V3_OFFSET_X)
			{
				m_Input.m_Direction = -1;
				m_Input.m_Hook = 1;
				if (Server()->Tick() % 20 == 0)
					m_Input.m_Hook = 0;
				m_Input.m_TargetX = -200;
				m_Input.m_TargetY = -87;
				if (m_Core.m_Pos.y > 19 * 32 + 20)
				{
					m_Input.m_TargetX = -200;
					m_Input.m_TargetY = -210;
				}
			}

			//Freeze prevention top right
			if (m_Core.m_Pos.x < 402 * 32 + 10 + V3_OFFSET_X && m_Core.m_Pos.y < 71 * 32 + V3_OFFSET_Y && m_Core.m_Pos.x > 397 * 32 + V3_OFFSET_X)
			{
				m_Input.m_Direction = 1;
				m_Input.m_Hook = 1;
				if (Server()->Tick() % 20 == 0)
					m_Input.m_Hook = 0;
				m_Input.m_TargetX = 200;
				m_Input.m_TargetY = -87;
				if (m_Core.m_Pos.y > 67 * 32 + 20 + V3_OFFSET_Y)
				{
					m_Input.m_TargetX = 200;
					m_Input.m_TargetY = -210;
				}
			}

			//Freeze prevention mid
			if (m_Core.m_Pos.x > 393 * 32 - 7 + V3_OFFSET_X && m_Core.m_Pos.x < 396 * 32 + 7 + V3_OFFSET_X)
			{
				if (m_Core.m_Vel.x < 0.0f)
					m_Input.m_Direction = -1;
				else
					m_Input.m_Direction = 1;

				if (m_Core.m_Pos.y > 77 * 32 - 1 + V3_OFFSET_Y && IsGrounded() == false)
				{
					m_Input.m_Jump = 1;
					if (m_Core.m_Jumped > 2) //no jumps == rip   --> panic hook
					{
						m_Input.m_Hook = 1;
						if (Server()->Tick() % 15 == 0)
							m_Input.m_Hook = 0;
					}
				}
			}

			//Freeze prevention left 
			if (m_Core.m_Pos.x < 380 * 32 + V3_OFFSET_X || (m_Core.m_Pos.x < 382 * 32 + V3_OFFSET_X && m_Core.m_Vel.x < -8.4f))
				m_Input.m_Direction = 1;
			//Freeze prevention right
			if (m_Core.m_Pos.x > 408 * 32 + V3_OFFSET_X || (m_Core.m_Pos.x > 406 * 32 + V3_OFFSET_X && m_Core.m_Vel.x > 8.4f))
				m_Input.m_Direction = -1;
		}

	}
	else if (m_pPlayer->m_Dummymode == 32) //BlmapChill police bot
	{
		if (m_Core.m_Pos.x > 451 * 32 && m_Core.m_Pos.x < 472 * 32 && m_Core.m_Pos.y > 74 * 32 && m_Core.m_Pos.y < 85 * 32) //spawn area, walk into the left SPAWN teleporter
		{
			m_DummySpeed = 70; // fix for crashbug (DONT REMOVE)

			m_Input.m_Direction = -1;
			if (m_Core.m_Pos.x > 454 * 32 && m_Core.m_Pos.x < 458 * 32) //left side of spawn area
			{
				m_Input.m_Jump = 1;
				if (Server()->Tick() % 10 == 0)
					m_Input.m_Jump = 0;
			}
		}
		else if (m_Core.m_Pos.x < 240 * 32 && m_Core.m_Pos.y < 36 * 32) // the complete zone in the map intselfs. its for resetting the dummy when he is back in spawn using tp
		{
			if (isFreezed && m_Core.m_Pos.x > 32 * 32)
			{
				if (Server()->Tick() % 60 == 0)
					GameServer()->SendEmoticon(m_pPlayer->GetCID(), 3); // tear emote before killing
				if (Server()->Tick() % 500 == 0 && IsGrounded()) //kill when freeze
					Die(m_pPlayer->GetCID(), WEAPON_SELF);
			}
			if (m_Core.m_Pos.x < 24 * 32 && m_Core.m_Pos.y < 14 * 32 && m_Core.m_Pos.x > 23 * 32) // looking for tp and setting different aims for the swing
				m_DummySpawnTeleporter = 1;
			if (m_Core.m_Pos.x < 25 * 32 && m_Core.m_Pos.y < 14 * 32 && m_Core.m_Pos.x > 24 * 32) // looking for tp and setting different aims for the swing
				m_DummySpawnTeleporter = 2;
			if (m_Core.m_Pos.x < 26 * 32 && m_Core.m_Pos.y < 14 * 32 && m_Core.m_Pos.x > 25 * 32) // looking for tp and setting different aims for the swing
				m_DummySpawnTeleporter = 3;
			if (m_Core.m_Pos.y > 21 * 32 && m_Core.m_Pos.x > 43 * 32 && m_Core.m_Pos.y < 35 * 32) // kill 
				Die(m_pPlayer->GetCID(), WEAPON_SELF);
			if (m_Core.m_Pos.y > 35 * 32 && m_Core.m_Pos.x < 43 * 32) // area bottom right from spawn, if he fall, he will kill
				Die(m_pPlayer->GetCID(), WEAPON_SELF);
			if (m_Core.m_Pos.x < 16 * 32) // area left of old spawn, he will kill too
				Die(m_pPlayer->GetCID(), WEAPON_SELF);
			else if (m_Core.m_Pos.y > 25 * 32) // after unfreeze hold hook to the right and walk right.
			{
				m_Input.m_TargetX = 100;
				m_Input.m_TargetY = 1;
				m_Input.m_Direction = 1;
				m_Input.m_Hook = 1;
				if (m_Core.m_Pos.x > 25 * 32 && m_Core.m_Pos.x < 33 * 32 && m_Core.m_Pos.y > 30 * 32) //set weapon and aim down
				{
					if (Server()->Tick() % 5 == 0)
					{
						SetWeapon(3);
					}
					if (m_DummySpawnTeleporter == 1)
					{
						m_Input.m_TargetX = 190;
						m_Input.m_TargetY = 100;
						m_LatestInput.m_TargetX = 190;
						m_LatestInput.m_TargetY = 100;
					}
					else if (m_DummySpawnTeleporter == 2)
					{
						m_Input.m_TargetX = 205;
						m_Input.m_TargetY = 100;
						m_LatestInput.m_TargetX = 205;
						m_LatestInput.m_TargetY = 100;
					}
					else if (m_DummySpawnTeleporter == 3)
					{
						m_Input.m_TargetX = 190;
						m_Input.m_TargetY = 100;
						m_LatestInput.m_TargetX = 190;
						m_LatestInput.m_TargetY = 100;
					}
					if (m_Core.m_Pos.x > 31 * 32)
					{
						m_Input.m_Jump = 1;
						m_LatestInput.m_Fire++;
						m_Input.m_Fire++;
					}
				}
			}
			else if (m_Core.m_Pos.x > 33 * 32 && m_Core.m_Pos.x < 50 * 32 && m_Core.m_Pos.y > 18 * 32)
			{
				m_Input.m_Direction = 1;
				if (m_Core.m_Pos.x > 33 * 32 && m_Core.m_Pos.x < 42 * 32 && m_Core.m_Pos.y > 20 * 32 && m_Core.m_Pos.y < 25 * 32)
				{
					GameServer()->SendEmoticon(m_pPlayer->GetCID(), 14); //happy emote when successfully did the grenaede jump
					if (Server()->Tick() % 1 == 0) //change to gun
						SetWeapon(1);
				}
				if (m_Core.m_Pos.x > 47 * 32 && m_Core.m_Pos.x < 48 * 32) // jumping up on the plateu
				{
					m_Input.m_Direction = 0;
					m_Input.m_Jump = 1;
				}
			}
			else if (m_Core.m_Pos.y < 16 * 32 && m_Core.m_Pos.x < 75 * 32 && m_Core.m_Pos.x > 40 * 32) // walking right on it
			{
				m_Input.m_Direction = 1;
				m_Input.m_Jump = 0;
				if (m_Core.m_Pos.x > 55 * 32 && m_Core.m_Pos.x < 56 * 32) //jumping over gap
					m_Input.m_Jump = 1;
			}
			if (m_Core.m_Pos.y > 15 * 32 && m_Core.m_Pos.x > 55 * 32 && m_Core.m_Pos.x < 65 * 32)
			{
				m_Input.m_Direction = 1;
				if (m_Core.m_Pos.x > 63 * 32)
					m_Input.m_Jump = 1;
			}
			else if (m_Core.m_Pos.x > 75 * 32 && m_Core.m_Pos.x < 135 * 32) //gores stuff (the thign with freeze spikes from top and bottom)
			{
				m_Input.m_Jump = 0;
				m_Input.m_Direction = 1;
				if (m_Core.m_Pos.x > 77 * 32) //start jump into gores
					m_Input.m_Jump = 1;
				if (m_Core.m_Pos.x > 92 * 32 && m_Core.m_Pos.y > 12.5 * 32) // hooking stuff from now on
				{
					m_Input.m_TargetX = 100;
					m_Input.m_TargetY = -100;
					m_Input.m_Hook = 1;
					if (m_Core.m_Pos.y < 14 * 32 && m_Core.m_Pos.x > 100 * 32 && m_Core.m_Pos.x < 110 * 32)
						m_Input.m_Hook = 0;
				}
				if (m_Core.m_Pos.x > 120 * 32 && m_Core.m_Pos.y < 13 * 32)
					m_Input.m_Hook = 0;
			}
			else if (m_Core.m_Pos.x > 135 * 32)
			{
				m_Input.m_Direction = 1;
				if (m_Core.m_Pos.y > 12 * 32) // gores (the long way to 5 jumps)
				{
					m_Input.m_TargetX = 100;
					m_Input.m_TargetY = -200;
					m_Input.m_Hook = 1;
					if (m_Core.m_Pos.y < 12 * 32)
						m_Input.m_Hook = 0;
					if (m_Core.m_Pos.x > 212 * 32)
					{
						m_Input.m_Hook = 1;
						m_Input.m_TargetX = 100;
						m_Input.m_TargetY = -75;
						m_Input.m_Jump = 1;
					}
				}
			}
		}
		if (isFreezed && m_Core.m_Pos.y < 410 * 32) // kills when in freeze and not in policebase
		{
			if (Server()->Tick() % 60 == 0)
				GameServer()->SendEmoticon(m_pPlayer->GetCID(), 3); // tear emote before killing
			if (Server()->Tick() % 500 == 0 && IsGrounded()) // kill when freeze
				Die(m_pPlayer->GetCID(), WEAPON_SELF);
		}
		if (isFreezed && m_Core.m_Pos.x < 41 * 32 && m_Core.m_Pos.x > 33 * 32 && m_Core.m_Pos.y < 10 * 32) // kills when on speedup right next to the newtee spawn to prevent infinite flappy blocking
		{
			if (Server()->Tick() % 500 == 0) // kill when freeze
				Die(m_pPlayer->GetCID(), WEAPON_SELF);
		}
		if (m_Core.m_Pos.x > 368 * 32 && m_Core.m_Pos.y < 340 * 32) //new spawn going left and hopping over the gap under the CFRM.  (the jump over the freeze gap before falling down is not here, its in line 13647)
		{
			m_Input.m_Direction = -1;
			if (m_Core.m_Pos.x > 509 * 32 && m_Core.m_Pos.y > 62 * 32) // if bot gets under the table he will go right and jump out of the gap under the table
			{
				m_Input.m_Direction = 1;
				if (m_Core.m_Pos.x > 511.5 * 32)
				{
					if (Server()->Tick() % 10 == 0)
						m_Input.m_Jump = 1;
				}
			}
			else if (Server()->Tick() % 10 == 0 && m_Core.m_Pos.x > 505 * 32)
				m_Input.m_Jump = 1;
			if (m_Core.m_Pos.x < 497 * 32 && m_Core.m_Pos.x > 496 * 32)
				m_Input.m_Jump = 1;
			if (m_Core.m_Pos.x < 480 * 32 && m_Core.m_Pos.x > 479 * 32)
				m_Input.m_Jump = 1;
		}
		if (m_Core.m_Pos.x > 370 * 32 && m_Core.m_Pos.y < 340 * 32 && m_Core.m_Pos.y > 310 * 32) // bottom going left to the grenade jump
		{
			m_Input.m_Direction = -1;
			if (m_Core.m_Pos.x < 422 * 32 && m_Core.m_Pos.x > 421 * 32) // bottom jump over the hole to police station
				m_Input.m_Jump = 1;
			if (m_Core.m_Pos.x < 406 * 32 && m_Core.m_Pos.x > 405 * 32) // using 5jump from now on
				m_Input.m_Jump = 1;
			if (m_Core.m_Pos.x < 397 * 32 && m_Core.m_Pos.x > 396 * 32)
				m_Input.m_Jump = 1;
			if (m_Core.m_Pos.x < 387 * 32 && m_Core.m_Pos.x > 386 * 32)
				m_Input.m_Jump = 1;
			if (m_Core.m_Pos.x < 377 * 32 && m_Core.m_Pos.x > 376 * 32) // last jump from the 5 jump
				m_Input.m_Jump = 1;
			if (m_Core.m_Pos.y > 339 * 32) // if he falls into the hole to police station he will kill
				Die(m_pPlayer->GetCID(), WEAPON_SELF);
		}
		else if (m_Core.m_Pos.y > 296 * 32 && m_Core.m_Pos.x < 370 * 32 && m_Core.m_Pos.x > 350 * 32 && m_Core.m_Pos.y < 418 * 32) // getting up to the grenade jump part
		{
			if (IsGrounded())
			{
				m_ReachedCinemaEntrance = true;
				m_Input.m_Jump = 1;
			}
			else if (m_Core.m_Pos.y < 313 * 32 && m_Core.m_Pos.y > 312 * 32 && m_Core.m_Pos.x < 367 * 32)
				m_Input.m_Direction = 1;
			else if (m_Core.m_Pos.x > 367 * 32)
				m_Input.m_Direction = -1;
			if (!m_ReachedCinemaEntrance && m_Core.m_Pos.x < 370 * 32)
				m_Input.m_Direction = 0;
			if (m_Core.m_Vel.y > 0.0000001f && m_Core.m_Pos.y < 310 * 32)
				m_Input.m_Jump = 1;
		}
		else if (m_Core.m_Vel.y > 0.001f && m_Core.m_Pos.y < 293 * 32 && m_Core.m_Pos.x > 366.4 * 32 && m_Core.m_Pos.x < 370 * 32)
		{
			m_Input.m_Direction = -1;
			if (Server()->Tick() % 1 == 0)
				SetWeapon(3);
		}
		else if (m_Core.m_Pos.x > 325 * 32 && m_Core.m_Pos.x < 366 * 32 && m_Core.m_Pos.y < 295 * 32 && m_Core.m_Pos.y > 59 * 32) // insane grenade jump
		{
			if (IsGrounded() && m_DummyGrenadeJump == 0) // shoot up
			{
				m_Input.m_Jump = 1;
				m_Input.m_TargetX = 0;
				m_Input.m_TargetY = -100;
				m_LatestInput.m_TargetX = 0;
				m_LatestInput.m_TargetY = -100;
				m_LatestInput.m_Fire++;
				m_Input.m_Fire++;
				m_DummyGrenadeJump = 1;
			}
			else if (m_Core.m_Vel.y > -7.6f && m_DummyGrenadeJump == 1) // jump in air // basically a timer for when the grenade comes down
			{
				m_Input.m_Jump = 1;
				m_DummyGrenadeJump = 2;
			}
			if (m_DummyGrenadeJump == 2 || m_DummyGrenadeJump == 3) // double grenade
			{
				if (m_Core.m_Pos.y > 58 * 32)
				{
					if (IsGrounded())
						m_DummyTouchedGround = true;
					if (m_DummyTouchedGround == true)
						m_Input.m_Direction = -1;
					if (m_Core.m_Vel.y > 0.1f && IsGrounded())
					{
						m_Input.m_Jump = 1;
						m_Input.m_TargetX = 100;
						m_Input.m_TargetY = 150;
						m_LatestInput.m_TargetX = 100;
						m_LatestInput.m_TargetY = 150;
						m_LatestInput.m_Fire++;
						m_Input.m_Fire++;
						m_DummyGrenadeJump = 3;
					}
					if (m_DummyGrenadeJump == 3)
					{
						if (m_Core.m_Pos.x < 344 * 32 && m_Core.m_Pos.x > 343 * 32 && m_Core.m_Pos.y > 250 * 32) // air grenade for double wall grnade
						{
							m_Input.m_TargetX = -100;
							m_Input.m_TargetY = -100;
							m_LatestInput.m_TargetX = -100;
							m_LatestInput.m_TargetY = -100;
							m_LatestInput.m_Fire++;
							m_Input.m_Fire++;
						}
					}
				}
			}
			if (m_Core.m_Pos.x < 330 * 32 && m_Core.m_Vel.x == 0.0f && m_Core.m_Pos.y > 59 * 32) // if on wall jump and shoot
			{
				if (m_Core.m_Pos.y > 250 * 32 && m_Core.m_Vel.y > 6.0f)
				{
					m_Input.m_Jump = 1;
					m_DummyStartGrenade = true;
				}
				if (m_DummyStartGrenade == true)
				{
					m_Input.m_TargetX = -100;
					m_Input.m_TargetY = 170;
					m_LatestInput.m_TargetX = -100;
					m_LatestInput.m_TargetY = 170;
					m_LatestInput.m_Fire++;
					m_Input.m_Fire++;
				}
				if (m_Core.m_Pos.y < 130 * 32 && m_Core.m_Pos.y > 131 * 32)
					m_Input.m_Jump = 1;
				if (m_Core.m_Vel.y > 0.001f && m_Core.m_Pos.y < 150 * 32)
					m_DummyStartGrenade = false;
				if (m_Core.m_Vel.y > 2.0f && m_Core.m_Pos.y < 150 * 32)
				{
					m_Input.m_Jump = 1;
					m_DummyStartGrenade = true;
				}
			}
		}
		if (m_Core.m_Pos.y < 60 * 32 && m_Core.m_Pos.x < 337 * 32 && m_Core.m_Pos.x > 325 * 32 && m_Core.m_Pos.y > 53 * 32) // top of the grenade jump // shoot left to get to the right 
		{
			m_Input.m_Direction = 1;
			m_Input.m_TargetX = -100;
			m_Input.m_TargetY = 0;
			m_LatestInput.m_TargetX = -100;
			m_LatestInput.m_TargetY = 0;
			m_LatestInput.m_Fire++;
			m_Input.m_Fire++;
			m_Input.m_Jump = 0;
			if (m_Core.m_Pos.x > 333 * 32 && m_Core.m_Pos.x < 335 * 32) // hook up and get into the tunnel thingy
			{
				m_Input.m_Jump = 1;
				if (m_Core.m_Pos.y < 55 * 32)
					m_Input.m_Direction = 1;
			}
		}
		if (m_Core.m_Pos.x > 337 * 32 && m_Core.m_Pos.x < 400 * 32 && m_Core.m_Pos.y < 60 * 32 && m_Core.m_Pos.y > 40 * 32) // hook thru the hookthru
		{
			m_Input.m_TargetX = 0;
			m_Input.m_TargetY = -1;
			m_LatestInput.m_TargetX = 0;
			m_LatestInput.m_TargetY = -1;
			m_Input.m_Hook = 1;
		}
		if (m_Core.m_Pos.x > 339.5 * 32 && m_Core.m_Pos.x < 345 * 32 && m_Core.m_Pos.y < 51 * 32)
			m_Input.m_Direction = -1;
		if (m_Core.m_Pos.x < 339 * 32 && m_Core.m_Pos.x > 315 * 32 && m_Core.m_Pos.y > 40 * 32 && m_Core.m_Pos.y < 53 * 32) // top of grenade jump the thing to go through the wall
		{
			m_Input.m_Hook = 0;
			m_Input.m_TargetX = 100;
			m_Input.m_TargetY = 50;
			m_LatestInput.m_TargetX = 100;
			m_LatestInput.m_TargetY = 50;
			if (m_DummyAlreadyBeenHere == false)
			{
				if (m_Core.m_Pos.x < 339 * 32)
				{
					m_Input.m_Direction = 1;
					if (m_Core.m_Pos.x > 338 * 32 && m_Core.m_Pos.x < 339 * 32 && m_Core.m_Pos.y > 51 * 32)
						m_DummyAlreadyBeenHere = true;
				}
			}
			if (m_DummyAlreadyBeenHere == true) //using grenade to get throug the freeze in this tunnel thingy
			{
				m_Input.m_Direction = -1;
				if (m_Core.m_Pos.x < 338 * 32)
				{
					m_LatestInput.m_Fire++;
					m_Input.m_Fire++;
				}
			}
			if (m_Core.m_Pos.x < 328 * 32 && m_Core.m_Pos.y < 60 * 32)
				m_Input.m_Jump = 1;
		}
		else if (m_Core.m_Pos.y > 260 * 32 && m_Core.m_Pos.x < 325 * 32 && m_Core.m_Pos.y < 328 * 32 && m_Core.m_Pos.x > 275 * 32) // jumping over the big freeze to get into the tunnel
		{
			m_Input.m_Direction = -1;
			m_Input.m_Jump = 0;
			if (m_Core.m_Pos.y > 280 * 32 && m_Core.m_Pos.y < 285 * 32)
				m_Input.m_Jump = 1;
			if (Server()->Tick() % 5 == 0)
				SetWeapon(1);
		}
		else if (m_Core.m_Pos.y > 328 * 32 && m_Core.m_Pos.y < 345 * 32 && m_Core.m_Pos.x > 236 * 32 && m_Core.m_Pos.x < 365 * 32) // after grenade jump and being down going into the tunnel to police staion
		{
			m_Input.m_Direction = 1;
			if (m_Core.m_Pos.x > 265 * 32 && m_Core.m_Pos.x < 267 * 32)
				m_Input.m_Jump = 1;
			if (m_Core.m_Pos.x > 282 * 32 && m_Core.m_Pos.x < 284 * 32)
				m_Input.m_Jump = 1;
		}
		if (m_Core.m_Pos.y > 337.4 * 32 && m_Core.m_Pos.y < 345 * 32 && m_Core.m_Pos.x > 295 * 32 && m_Core.m_Pos.x < 365 * 32) // walkking left in air to get on the little block
			m_Input.m_Direction = -1;
		if (m_Core.m_Pos.y < 355 * 32 && m_Core.m_Pos.y > 346 * 32)
		{
			m_Input.m_Direction = 1;
			m_Input.m_Jump = 0;
			if (m_Core.m_Vel.y > 0.0000001f && m_Core.m_Pos.y > 352.6 * 32 && m_Core.m_Pos.x < 315 * 32) // jump in air to get to the right
				m_Input.m_Jump = 1;
		}
		if (m_Core.m_Pos.x > 315.5 * 32 && m_Core.m_Pos.x < 327 * 32 && m_Core.m_Pos.y > 344 * 32 && m_Core.m_Pos.y < 418 * 32) // stop moving in air to get into the hole
			m_Input.m_Direction = 0;
		if (m_Core.m_Pos.x > 290 * 32 && m_Core.m_Pos.x < 450 * 32 && m_Core.m_Pos.y < 450 * 32 && m_Core.m_Pos.y > 380 * 32) //police area
		{
			if (m_Core.m_Pos.x > 380 * 32 && m_Core.m_Pos.x < 450 * 32 && m_Core.m_Pos.y < 450 * 32 && m_Core.m_Pos.y > 380 * 32) //police area
			{
				if (m_Core.m_Pos.x < 397 * 32 && m_Core.m_Pos.y > 436 * 32 && m_Core.m_Pos.x > 388 * 32) // on the money tile jump loop, to prevent blocking flappy there
				{
					m_Input.m_Jump = 0;
					if (Server()->Tick() % 20 == 0)
						m_Input.m_Jump = 1;
				}
				//detect lower panic (accedentally fall into the lower police base 
				if (!m_LowerPanic && m_Core.m_Pos.y > 437 * 32 && m_Core.m_Pos.y > m_LovedY)
				{
					m_LowerPanic = 1;
					GameServer()->SendEmoticon(m_pPlayer->GetCID(), 9); //angry emote
				}

				if (m_LowerPanic)
				{
					//Check for end panic
					if (m_Core.m_Pos.y < 434 * 32)
					{
						if (IsGrounded())
							m_LowerPanic = 0; //made it up yay
					}

					if (m_LowerPanic == 1)//position to jump on stairs
					{
						if (m_Core.m_Pos.x < 400 * 32)
							m_Input.m_Direction = 1;
						else if (m_Core.m_Pos.x > 401 * 32)
							m_Input.m_Direction = -1;
						else
							m_LowerPanic = 2;
					}
					else if (m_LowerPanic == 2) //jump on the left starblock element
					{
						if (IsGrounded())
						{
							m_Input.m_Jump = 1;
							if (Server()->Tick() % 20 == 0)
								m_Input.m_Jump = 0;
						}

						//navigate to platform
						if (m_Core.m_Pos.y < 435 * 32 - 10)
						{
							m_Input.m_Direction = -1;
							if (m_Core.m_Pos.y < 433 * 32)
							{
								if (m_Core.m_Vel.y > 0.01f && m_DummyUsedDJ == false)
								{
									m_Input.m_Jump = 1; //double jump
									if (!IsGrounded()) // this dummyuseddj is for only using default 2 jumps even if 5 jump is on
										m_DummyUsedDJ = true;
								}
							}
							if (m_DummyUsedDJ == true && IsGrounded())
								m_DummyUsedDJ = false;
						}

						else if (m_Core.m_Pos.y < 438 * 32) //only if high enough focus on the first lower platform
						{
							if (m_Core.m_Pos.x < 403 * 32)
								m_Input.m_Direction = 1;
							else if (m_Core.m_Pos.x > 404 * 32 + 20)
								m_Input.m_Direction = -1;
						}

						if ((m_Core.m_Pos.y > 441 * 32 + 10 && (m_Core.m_Pos.x > 402 * 32 || m_Core.m_Pos.x < 399 * 32 + 10)) || isFreezed) //check for fail position
							m_LowerPanic = 1; //lower panic mode to reposition
					}
				}
				else //no dummy lower panic
				{
					m_HelpMode = 0;
					//check if officer needs help
					CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this, 1);
					if (pChr && pChr->IsAlive())
					{
						if (m_Core.m_Pos.y > 435 * 32) // setting the destination of dummy to top left police entry bcs otherwise bot fails when trying to help --> walks into jail wall xd
						{
							m_LovedX = (392 + rand() % 2) * 32;
							m_LovedY = 430 * 32;
						}
						//aimbot on heuzeueu
						m_Input.m_TargetX = pChr->m_Pos.x - m_Pos.x;
						m_Input.m_TargetY = pChr->m_Pos.y - m_Pos.y;
						m_LatestInput.m_TargetX = pChr->m_Pos.x - m_Pos.x;
						m_LatestInput.m_TargetY = pChr->m_Pos.y - m_Pos.y;

						m_ClosestPolice = false;
						//If a policerank escapes from jail he is treated like a non police
						if (pChr->m_pPlayer->m_aHasItem[POLICE] || pChr->m_PoliceHelper)
							m_ClosestPolice = true;

						if (pChr->m_Pos.x > 444 * 32 - 10) //police dude failed too far --> to be reached by hook (set too help mode extream to leave save area)
						{
							m_HelpMode = 2;
							if (m_Core.m_Jumped > 1 && m_Core.m_Pos.x > 431 * 32) //double jumped and above the freeze
								m_Input.m_Direction = -1;
							else
								m_Input.m_Direction = 1;
							//doublejump before falling in freeze
							if ((m_Core.m_Pos.x > 432 * 32 && m_Core.m_Pos.y > 432 * 32) || m_Core.m_Pos.x > 437 * 32) //over the freeze and too low
							{
								m_Input.m_Jump = 1;
								m_HelpHook = true;
							}
							if (IsGrounded() && m_Core.m_Pos.x > 430 * 32 && Server()->Tick() % 60 == 0)
								m_Input.m_Jump = 1;
						}
						else
							m_HelpMode = 1;

						if (m_HelpMode == 1 && m_Core.m_Pos.x > 431 * 32 + 10)
							m_Input.m_Direction = -1;
						else if (m_HelpMode == 1 && m_Core.m_Pos.x < 430 * 32)
							m_Input.m_Direction = 1;
						else
						{
							if (!m_HelpHook && m_ClosestPolice)
							{
								if (m_HelpMode == 2) //police dude failed too far --> to be reached by hook
								{
									//if (m_Core.m_Pos.x > 435 * 32) //moved too double jump
									//{
									//	m_HelpHook = true;
									//}
								}
								else if (pChr->m_Pos.x > 439 * 32) //police dude in the middle
								{
									if (IsGrounded())
									{
										m_HelpHook = true;
										m_Input.m_Jump = 1;
										m_Input.m_Hook = 1;
									}
								}
								else //police dude failed too near to hook from ground
								{
									if (m_Core.m_Vel.y < -4.20f && m_Core.m_Pos.y < 431 * 32)
									{
										m_HelpHook = true;
										m_Input.m_Jump = 1;
										m_Input.m_Hook = 1;
									}
								}
							}
							if (Server()->Tick() % 8 == 0)
								m_Input.m_Direction = 1;
						}

						if (m_HelpHook)
						{
							m_Input.m_Hook = 1;
							if (Server()->Tick() % 200 == 0)
							{
								m_HelpHook = false; //timeout hook maybe failed
								m_Input.m_Hook = 0;
								m_Input.m_Direction = 1;
							}
						}

						//dont wait on ground
						if (IsGrounded() && Server()->Tick() % 900 == 0)
							m_Input.m_Jump = 1;
						//backup reset jump
						if (Server()->Tick() % 1337 == 0)
							m_Input.m_Jump = 0;
					}

					if (!m_HelpMode)
					{
						//==============
						//NOTHING TO DO
						//==============
						//basic walk to destination
						if (m_Core.m_Pos.x < m_LovedX - 32)
							m_Input.m_Direction = 1;
						else if (m_Core.m_Pos.x > m_LovedX + 32 && m_Core.m_Pos.x > 384 * 32)
							m_Input.m_Direction = -1;

						//change changing speed
						if (Server()->Tick() % m_DummySpeed == 0)
						{
							if (rand() % 2 == 0)
								m_DummySpeed = rand() % 10000 + 420;
						}

						//choose beloved destination
						if (Server()->Tick() % m_DummySpeed == 0)
						{
							if ((rand() % 2) == 0)
							{
								if ((rand() % 3) == 0)
								{
									m_LovedX = 420 * 32 + rand() % 69;
									m_LovedY = 430 * 32;
									GameServer()->SendEmoticon(m_pPlayer->GetCID(), 7);
								}
								else
								{
									m_LovedX = (392 + rand() % 2) * 32;
									m_LovedY = 430 * 32;
								}
								if ((rand() % 2) == 0)
								{
									m_LovedX = 384 * 32 + rand() % 128;
									m_LovedY = 430 * 32;
									GameServer()->SendEmoticon(m_pPlayer->GetCID(), 5);
								}
								else
								{
									if (rand() % 3 == 0)
									{
										m_LovedX = 420 * 32 + rand() % 128;
										m_LovedY = 430 * 32;
										GameServer()->SendEmoticon(m_pPlayer->GetCID(), 8);
									}
									else if (rand() % 4 == 0)
									{
										m_LovedX = 429 * 32 + rand() % 64;
										m_LovedY = 430 * 32;
										GameServer()->SendEmoticon(m_pPlayer->GetCID(), 8);
									}
								}
								if (rand() % 5 == 0) //lower middel base
								{
									m_LovedX = 410 * 32 + rand() % 64;
									m_LovedY = 443 * 32;
								}
							}
							else
								GameServer()->SendEmoticon(m_pPlayer->GetCID(), 1);
						}
					}
				}

				//dont walk into the lower police base entry freeze
				if (m_Core.m_Pos.x > 425 * 32 && m_Core.m_Pos.x < 429 * 32) //right side
				{
					if (m_Core.m_Vel.x < -0.02f && IsGrounded())
						m_Input.m_Jump = 1;
				}
				else if (m_Core.m_Pos.x > 389 * 32 && m_Core.m_Pos.x < 391 * 32) //left side
				{
					if (m_Core.m_Vel.x > 0.02f && IsGrounded())
						m_Input.m_Jump = 1;
				}

				//jump over the police underground from entry to enty
				if (m_Core.m_Pos.y > m_LovedY) //only if beloved place is an upper one
				{
					if (m_Core.m_Pos.x > 415 * 32 && m_Core.m_Pos.x < 418 * 32) //right side
					{
						if (m_Core.m_Vel.x < -0.02f && IsGrounded())
						{
							m_Input.m_Jump = 1;
							if (Server()->Tick() % 5 == 0)
								m_Input.m_Jump = 0;
						}
					}
					else if (m_Core.m_Pos.x > 398 * 32 && m_Core.m_Pos.x < 401 * 32) //left side
					{
						if (m_Core.m_Vel.x > 0.02f && IsGrounded())
						{
							m_Input.m_Jump = 1;
							if (Server()->Tick() % 5 == 0)
								m_Input.m_Jump = 0;
						}
					}

					//do the doublejump
					if (m_Core.m_Vel.y > 6.9f && m_Core.m_Pos.y > 430 * 32 && m_Core.m_Pos.x < 433 * 32 && m_DummyUsedDJ == false) //falling and not too high to hit roof with head
					{
						m_Input.m_Jump = 1;
						if (!IsGrounded()) // this dummyuseddj is for only using default 2 jumps even if 5 jump is on
							m_DummyUsedDJ = true;
					}
					if (m_DummyUsedDJ == true && IsGrounded())
						m_DummyUsedDJ = false;
				}
			}
			if (m_Core.m_Pos.y > 380 * 32 && m_Core.m_Pos.x < 363 * 32) // walking right again to get into the tunnel at the bottom
			{
				m_Input.m_Direction = 1;
				if (IsGrounded())
					m_Input.m_Jump = 1;
			}
			if (m_Core.m_Pos.y > 380 * 32 && m_Core.m_Pos.x < 381 * 32 && m_Core.m_Pos.x > 363 * 32)
			{
				m_Input.m_Direction = 1;
				if (m_Core.m_Pos.x > 367 * 32 && m_Core.m_Pos.x < 368 * 32 && IsGrounded())
					m_Input.m_Jump = 1;
				if (m_Core.m_Pos.y > 433.7 * 32)
					m_Input.m_Jump = 1;
			}
			if (m_Core.m_Pos.x > 290 * 32 && m_Core.m_Pos.x < 450 * 32 && m_Core.m_Pos.y > 415 * 32 && m_Core.m_Pos.y < 450 * 32)
			{
				if (isFreezed) // kills when in freeze in policebase or left of it (takes longer that he kills bcs the way is so long he wait a bit longer for help)
				{
					if (Server()->Tick() % 60 == 0)
						GameServer()->SendEmoticon(m_pPlayer->GetCID(), 3); // tear emote before killing
					if (Server()->Tick() % 3000 == 0 && (IsGrounded() || m_Core.m_Pos.x > 430 * 32)) // kill when freeze
						Die(m_pPlayer->GetCID(), WEAPON_SELF);
				}
			}
		}
	}
	else if (m_pPlayer->m_Dummymode == 99) // shop bot
	{
		CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this);
		if (pChr && pChr->m_InShop)
		{
			m_Input.m_TargetX = pChr->m_Pos.x - m_Pos.x;
			m_Input.m_TargetY = pChr->m_Pos.y - m_Pos.y;
			m_LatestInput.m_TargetX = pChr->m_Pos.x - m_Pos.x;
			m_LatestInput.m_TargetY = pChr->m_Pos.y - m_Pos.y;
		}
	}
	else
		m_pPlayer->m_Dummymode = 0;
}