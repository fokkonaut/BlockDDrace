#include "character.h"
#include <game/server/player.h>

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
	else if (m_pPlayer->m_Dummymode == -6)  //ChillBlock5 blmapv3 1o1 mode // made by chillerdragon
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
	else if (m_pPlayer->m_Dummymode == 29) //mode 18 (tryhardwayblocker cb5) //made by chillerdragon, cleaned up as much as possible by fokkonaut
	{
		if (m_DummyBoredCounter > 2)
		{
			CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this, 2);
			if (pChr && pChr->IsAlive())
			{
				//
			}
			else //no ruler alive
				m_DummyLockBored = true;
		}
		else
			m_DummyLockBored = false;

		if (m_DummyLockBored)
		{
			if (m_Core.m_Pos.x < 429 * 32 && IsGrounded())
				m_DummyBored = true;
		}

		if (m_Core.m_Pos.y > 214 * 32 && m_Core.m_Pos.x > 424 * 32)
		{
			CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this, 3);
			if (pChr && pChr->IsAlive())
				m_DummyBlockMode = 1;
		}
		else if (m_DummyBored)
			m_DummyBlockMode = 2;
		else if (m_DummySpecialDefend) //Check mode 3 [Attack from tunnel wayblocker]
			m_DummyBlockMode = 3;
		else
			m_DummyBlockMode = 0; //change to main mode

		//Modes:
		if (m_DummyBlockMode == 3) //special defend mode
		{
			//testy wenn der dummy in den special defend mode gesetzt wird pusht das sein adrenalin und ihm is nicht mehr lw
			m_DummyBoredCounter = 0;

			CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this, 2);
			if (pChr && pChr->IsAlive())
			{
				m_Input.m_TargetX = pChr->m_Pos.x - m_Pos.x;
				m_Input.m_TargetY = pChr->m_Pos.y - m_Pos.y;
				m_LatestInput.m_TargetX = pChr->m_Pos.x - m_Pos.x;
				m_LatestInput.m_TargetY = pChr->m_Pos.y - m_Pos.y;

				//rest on tick
				m_Input.m_Hook = 0;
				m_Input.m_Jump = 0;
				m_Input.m_Direction = 0;
				m_LatestInput.m_Fire = 0;
				m_Input.m_Fire = 0;
				SetWeapon(1); //gun verwenden damit auch erkannt wird wann der mode getriggert wird

				if (pChr->m_FreezeTime == 0)
				{
					//wenn der gegner doch irgendwie unfreeze wird übergib an den main mode und lass den notstand das regeln
					m_DummySpecialDefend = false;
					m_DummySpecialDefendAttack = false;
				}

				if (pChr->m_Core.m_Vel.y > -0.9f && pChr->m_Pos.y > 211 * 32)
				{
					//wenn der gegner am boden liegt starte angriff
					m_DummySpecialDefendAttack = true;

					//start jump
					m_Input.m_Jump = 1;
				}

				if (m_DummySpecialDefendAttack)
				{
					if (m_Core.m_Pos.x - pChr->m_Pos.x < 50) //wenn der gegner nah genung is mach dj
						m_Input.m_Jump = 1;

					if (pChr->m_Pos.x < m_Core.m_Pos.x)
						m_Input.m_Hook = 1;
					else //wenn der gegner weiter rechts als der bot is lass los und übergib an main deine arbeit ist hier getahen
					{    //main mode wird evenetuell noch korrigieren mit schieben
						m_DummySpecialDefend = false;
						m_DummySpecialDefendAttack = false;
					}

					//Der bot sollte möglichst weit nach rechts gehen aber natürlich nicht ins freeze
					if (m_Core.m_Pos.x < 427 * 32 + 15)
						m_Input.m_Direction = 1;
					else
						m_Input.m_Direction = -1;
				}
			}
			else //wenn kein gegner mehr im Ruler bereich is
			{
				m_DummySpecialDefend = false;
				m_DummySpecialDefendAttack = false;
			}
		}
		else if (m_DummyBlockMode == 2) //different wayblock mode
		{
			//rest on tick
			m_Input.m_Hook = 0;
			m_Input.m_Jump = 0;
			m_Input.m_Direction = 0;
			m_LatestInput.m_Fire = 0;
			m_Input.m_Fire = 0;
			if (Server()->Tick() % 30 == 0)
				SetWeapon(0);

			//Selfkills (bit random but they work)
			if (isFreezed)
			{
				//wenn der bot freeze is warte erstmal n paar sekunden und dann kill dich
				if (Server()->Tick() % 300 == 0)
					Die(m_pPlayer->GetCID(), WEAPON_SELF);
			}

			CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this, 4);
			if (pChr && pChr->IsAlive())
			{
				//Check ob an notstand mode18 = 0 übergeben
				if (pChr->m_FreezeTime == 0)
				{
					m_DummyBored = false;
					m_DummyBoredCounter = 0;
					m_DummyBlockMode = 0;
				}

				m_Input.m_TargetX = pChr->m_Pos.x - m_Pos.x;
				m_Input.m_TargetY = pChr->m_Pos.y - m_Pos.y;
				m_LatestInput.m_TargetX = pChr->m_Pos.x - m_Pos.x;
				m_LatestInput.m_TargetY = pChr->m_Pos.y - m_Pos.y;

				m_Input.m_Jump = 1;

				if (pChr->m_Pos.y > m_Core.m_Pos.y && pChr->m_Pos.x > m_Core.m_Pos.x + 20) //solange der bot über dem gegner ist (damit er wenn er ihn weg hammert nicht weiter hookt)
					m_Input.m_Hook = 1;

				if (m_Core.m_Pos.x > 420 * 32)

					m_Input.m_Direction = -1;
				if (pChr->m_Pos.y < m_Core.m_Pos.y + 15)
				{
					m_LatestInput.m_Fire++;
					m_Input.m_Fire++;
				}
			}
			else //lieblings position finden wenn nichts abgeht
			{
				if (m_Core.m_Pos.x < 423 * 32)
					m_Input.m_Direction = 1;
				else if (m_Core.m_Pos.x > 424 * 32 + 30)
					m_Input.m_Direction = -1;
			}
		}
		else if (m_DummyBlockMode == 1) //attack in tunnel
		{
			//Selfkills (bit random but they work)
			if (isFreezed)
			{
				//wenn der bot freeze is warte erstmal n paar sekunden und dann kill dich
				if (Server()->Tick() % 300 == 0)
					Die(m_pPlayer->GetCID(), WEAPON_SELF);
			}

			//stay on position
			if (m_Core.m_Pos.x < 426 * 32 + 10) // zu weit links
				m_Input.m_Direction = 1; //geh rechts
			else if (m_Core.m_Pos.x > 428 * 32 - 10) //zu weit rechts
				m_Input.m_Direction = -1; // geh links
			else if (m_Core.m_Pos.x > 428 * 32 + 10) // viel zu weit rechts
			{
				m_Input.m_Direction = -1; // geh links
				m_Input.m_Jump = 1;
			}
			else
			{
				CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this, 3);
				if (pChr && pChr->IsAlive())
				{
					if (pChr->m_Pos.x < 436 * 32) //wenn er ganz weit über dem freeze auf der kante ist (hooke direkt)
					{
						m_Input.m_TargetX = pChr->m_Pos.x - m_Pos.x;
						m_Input.m_TargetY = pChr->m_Pos.y - m_Pos.y;
						m_LatestInput.m_TargetX = pChr->m_Pos.x - m_Pos.x;
						m_LatestInput.m_TargetY = pChr->m_Pos.y - m_Pos.y;
					}
					else //wenn der Gegner weiter hinter dem unhook ist (hook über den Gegner um ihn trozdem zu treffen und das unhook zu umgehen)
					{
						m_Input.m_TargetX = pChr->m_Pos.x - m_Pos.x - 50;
						m_Input.m_TargetY = pChr->m_Pos.y - m_Pos.y;
						m_LatestInput.m_TargetX = pChr->m_Pos.x - m_Pos.x - 50;
						m_LatestInput.m_TargetY = pChr->m_Pos.y - m_Pos.y;
					}

					CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this, 5);
					if (pChr && pChr->IsAlive())
					{
						//wenn jemand im tunnel is check ob du nicht ausversehen den hookst anstatt des ziels in der WB area
						if (pChr->m_Pos.x < m_Core.m_Pos.x) //hooke nur wenn kein Gegner rechts von dem bot im tunnel is (da er sonst ziemlich wahrscheinlich den hooken würde)
							m_Input.m_Hook = 1;
					}
					else
						m_Input.m_Hook = 1;

					//schau ob sich der gegner bewegt und der bot grad nicht mehr am angreifen iss dann resette falls er davor halt misshookt hat
					//geht nich -.-
					if (m_Core.m_HookState != HOOK_FLYING && m_Core.m_HookState != HOOK_GRABBED)
					{
						if (Server()->Tick() % 10 == 0)
							m_Input.m_Hook = 0;
					}
					if (m_Core.m_Vel.x > 3.0f)
						m_Input.m_Direction = -1;
					else
						m_Input.m_Direction = 0;
				}
				else
					m_DummyBlockMode = 0;
			}
		}
		else if (m_DummyBlockMode == 0) //main mode
		{
			m_Input.m_Jump = 0;
			m_Input.m_Direction = 0;
			m_LatestInput.m_Fire = 0;
			m_Input.m_Fire = 0;

			//Check ob jemand in der linken freeze wand is
			CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this, 6);
			if (pChr && pChr->IsAlive()) // wenn ein spieler rechts im freeze lebt //----> versuche im notstand nicht den gegner auch da rein zu hauen da ist ja jetzt voll
				m_DummyLeftFreezeFull = true;
			else // wenn da keiner is fülle diesen spot (linke freeze wand im ruler spot)
				m_DummyLeftFreezeFull = false;

			if (m_Core.m_Pos.x > 241 * 32 && m_Core.m_Pos.x < 418 * 32 && m_Core.m_Pos.y > 121 * 32 && m_Core.m_Pos.y < 192 * 32) //new spawn ChillBlock5 (tourna edition (the on with the gores stuff))
			{
				//dieser code wird also nur ausgeführt wenn der bot gerade im neuen bereich ist
				if (m_Core.m_Pos.x > 319 * 32 && m_Core.m_Pos.y < 161 * 32) //top right spawn
				{
					//look up left
					if (m_Core.m_Pos.x < 372 * 32 && m_Core.m_Vel.y > 3.1f)
					{
						m_Input.m_TargetX = -30;
						m_Input.m_TargetY = -80;
					}
					else
					{
						m_Input.m_TargetX = -100;
						m_Input.m_TargetY = -80;
					}

					if (m_Core.m_Pos.x > 331 * 32 && isFreezed)
						Die(m_pPlayer->GetCID(), WEAPON_SELF);

					if (m_Core.m_Pos.x < 327 * 32) //dont klatsch in ze wand
						m_Input.m_Direction = 1; //nach rechts laufen
					else
						m_Input.m_Direction = -1;

					if (IsGrounded() && m_Core.m_Pos.x < 408 * 32) //initial jump from spawnplatform
						m_Input.m_Jump = 1;

					if (m_Core.m_Pos.x > 330 * 32) //only hook in tunnel and let fall at the end
					{
						if (m_Core.m_Pos.y > 147 * 32 || (m_Core.m_Pos.y > 143 * 32 && m_Core.m_Vel.y > 3.3f)) //gores pro hook up
							m_Input.m_Hook = 1;
						else if (m_Core.m_Pos.y < 143 * 32 && m_Core.m_Pos.x < 372 * 32) //hook down (if too high and in tunnel)
						{
							m_Input.m_TargetX = -42;
							m_Input.m_TargetY = 100;
							m_Input.m_Hook = 1;
						}
					}
				}
				else if (m_Core.m_Pos.x < 317 * 32) //top left spawn
				{
					if (m_Core.m_Pos.y < 158 * 32) //spawn area find down
					{
						//selfkill
						if (isFreezed)
							Die(m_pPlayer->GetCID(), WEAPON_SELF);

						if (m_Core.m_Pos.x < 276 * 32 + 20) //is die mitte von beiden linken spawns also da wo es runter geht
						{
							m_Input.m_TargetX = 57;
							m_Input.m_TargetY = -100;
							m_Input.m_Direction = 1;
						}
						else
						{
							m_Input.m_TargetX = -57;
							m_Input.m_TargetY = -100;
							m_Input.m_Direction = -1;
						}

						if (m_Core.m_Pos.y > 147 * 32)
						{
							m_Input.m_Hook = 1;
							if (m_Core.m_Pos.x > 272 * 32 && m_Core.m_Pos.x < 279 * 32) //let fall in the hole
							{
								m_Input.m_Hook = 0;
								m_Input.m_TargetX = 2;
								m_Input.m_TargetY = 100;
							}
						}
					}
					else if (m_Core.m_Pos.y > 162 * 32) //managed it to go down --> go left
					{
						//selfkill
						if (isFreezed)
							Die(m_pPlayer->GetCID(), WEAPON_SELF);

						if (m_Core.m_Pos.x < 283 * 32)
						{
							m_Input.m_TargetX = 200;
							m_Input.m_TargetY = -136;
							if (m_Core.m_Pos.y > 164 * 32 + 10)
								m_Input.m_Hook = 1;
						}
						else
						{
							m_Input.m_TargetX = 80;
							m_Input.m_TargetY = -100;
							if (m_Core.m_Pos.y > 171 * 32 - 10)
								m_Input.m_Hook = 1;
						}

						m_Input.m_Direction = 1;
					}
				}
				else //lower end area of new spawn --> entering old spawn by walling and walking right
				{
					m_Input.m_Direction = 1;
					m_Input.m_TargetX = 200;
					m_Input.m_TargetY = -84;

					//Selfkills
					if (isFreezed && IsGrounded()) //should never lie in freeze at the ground
						Die(m_pPlayer->GetCID(), WEAPON_SELF);

					if (m_Core.m_Pos.y < 166 * 32 - 20)
						m_Input.m_Hook = 1;

					if (m_Core.m_Pos.x > 332 * 32 && m_Core.m_Pos.x < 337 * 32 && m_Core.m_Pos.y > 182 * 32) //wont hit the wall --> jump
						m_Input.m_Jump = 1;

					if (m_Core.m_Pos.x > 336 * 32 + 20 && m_Core.m_Pos.y > 180 * 32) //stop moving if walled
						m_Input.m_Direction = 0;
				}
			}
			else
			{
				if (m_Core.m_Pos.x < 390 * 32 && m_Core.m_Pos.x > 325 * 32 && m_Core.m_Pos.y > 215 * 32)  //Links am spawn runter
					Die(m_pPlayer->GetCID(), WEAPON_SELF);

				else if (m_Core.m_Pos.y < 215 * 32 && m_Core.m_Pos.y > 213 * 32 && m_Core.m_Pos.x > 415 * 32 && m_Core.m_Pos.x < 428 * 32) //freeze decke im tunnel
					Die(m_pPlayer->GetCID(), WEAPON_SELF);

				else if (m_Core.m_Pos.y > 222 * 32) //freeze becken unter area
					Die(m_pPlayer->GetCID(), WEAPON_SELF);

				if (m_Core.m_Pos.y < 220 * 32 && m_Core.m_Pos.x < 415 * 32 && m_FreezeTime > 1 && m_Core.m_Pos.x > 352 * 32) //always suicide on freeze if not reached teh block area yet AND dont suicide in spawn area because new spawn sys can get pretty freezy
					Die(m_pPlayer->GetCID(), WEAPON_SELF);

				//new spawn do something agianst hookers 
				if (m_Core.m_Pos.x < 380 * 32 && m_Core.m_Pos.x > 322 * 32 && m_Core.m_Vel.x < -0.001f)
				{
					m_Input.m_Hook = 1;
					if ((m_Core.m_Pos.x < 362 * 32 && IsGrounded()) || m_Core.m_Pos.x < 350 * 32)
					{
						if (Server()->Tick() % 10 == 0)
						{
							m_Input.m_Jump = 1;
							SetWeapon(0);
						}
					}
				}
				if (m_Core.m_Pos.x < 415 * 32)
				{
					CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this);
					if (pChr && pChr->IsAlive())
					{
						if (pChr->m_Core.m_Pos.x > m_Core.m_Pos.x - 100 && pChr->m_Core.m_Pos.x < m_Core.m_Pos.x + 100 && pChr->m_Core.m_Pos.y > m_Core.m_Pos.y - 100 && pChr->m_Core.m_Pos.y < m_Core.m_Pos.y + 100)
						{
							if (pChr->m_Core.m_Vel.y < -1.5f) //only boost and use existing up speed
							{
								m_Input.m_Fire++;
								m_LatestInput.m_Fire++;
							}
							if (Server()->Tick() % 3 == 0)
								SetWeapon(0);
						}
						//old spawn do something agianst way blockers (roof protection)
						if (m_Core.m_Pos.y > 206 * 32 + 4 && m_Core.m_Pos.y < 208 * 32 && m_Core.m_Vel.y < -4.4f)
							m_Input.m_Jump = 1;

						//old spawn roof protection / attack hook
						if (pChr->m_Core.m_Pos.y > m_Core.m_Pos.y + 50)
							m_Input.m_Hook = 1;
					}
				}

				if (m_Core.m_Pos.x < 388 * 32 && m_Core.m_Pos.y > 213 * 32) //jump to old spawn
				{
					m_Input.m_Jump = 1;
					m_Input.m_Fire++;
					m_LatestInput.m_Fire++;
					m_Input.m_Hook = 1;
					m_Input.m_TargetX = -200;
					m_Input.m_TargetY = 0;
				}

				if (!m_DummyPlannedMovement)
				{
					CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this, 5);
					if (pChr && pChr->IsAlive())
					{
						if (pChr->m_Core.m_Vel.x < 3.3f) //found a slow bob in tunnel
							m_DummyMovementToBlockArea = true;
					}

					m_DummyPlannedMovement = true;
				}

				if (m_DummyMovementToBlockArea)
				{
					if (m_Core.m_Pos.x < 415 * 32)
					{
						m_Input.m_Direction = 1;

						if (m_Core.m_Pos.x > 404 * 32 && IsGrounded())
							m_Input.m_Jump = 1;

						if (m_Core.m_Pos.y < 208 * 32)
							m_Input.m_Jump = 1;

						if (m_Core.m_Pos.x > 410 * 32)
						{
							m_Input.m_TargetX = 200;
							m_Input.m_TargetY = 70;
							if (m_Core.m_Pos.x > 413 * 32)
								m_Input.m_Hook = 1;
						}
					}
					else //not needed but safty xD when the bot managed it to get into the ruler area change to old movement
						m_DummyMovementToBlockArea = false;

					//something went wrong:
					if (m_Core.m_Pos.y > 214 * 32)
					{
						m_Input.m_Jump = 1;
						m_DummyMovementToBlockArea = false;
					}
				}
				else //down way
				{
					//CheckFatsOnSpawn
					if (m_Core.m_Pos.x < 406 * 32)
					{
						CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this);
						if (pChr && pChr->IsAlive())
						{
							m_Input.m_TargetX = pChr->m_Pos.x - m_Pos.x;
							m_Input.m_TargetY = pChr->m_Pos.y - m_Pos.y;
							m_LatestInput.m_TargetX = pChr->m_Pos.x - m_Pos.x;
							m_LatestInput.m_TargetY = pChr->m_Pos.y - m_Pos.y;


							if (pChr->m_Pos.x < 407 * 32 && pChr->m_Pos.y > 212 * 32 && pChr->m_Pos.y < 215 * 32 && pChr->m_Pos.x > m_Core.m_Pos.x) //wenn ein im weg stehender tee auf der spawn plattform gefunden wurde
							{
								SetWeapon(0); //hol den hammer raus!
								if (pChr->m_Pos.x - m_Core.m_Pos.x < 30) //wenn der typ nahe bei dem bot ist
								{
									if (m_FreezeTick == 0) //nicht rum schrein
									{
										m_LatestInput.m_Fire++;
										m_Input.m_Fire++;
									}

									if (Server()->Tick() % 10 == 0)
										GameServer()->SendEmoticon(m_pPlayer->GetCID(), 9); //angry
								}
							}
							else
							{
								if (Server()->Tick() % 20 == 0)
									SetWeapon(1); //pack den hammer weg
							}
						}
					}

					//Check attacked on spawn
					if (m_Core.m_Pos.x < 412 * 32 && m_Core.m_Pos.y > 217 * 32 && m_Core.m_Vel.x < -0.5f)
					{
						m_Input.m_Jump = 1;
						m_DummyAttackedOnSpawn = true;
					}
					if (IsGrounded())
						m_DummyAttackedOnSpawn = false;

					if (m_DummyAttackedOnSpawn)
					{
						if (Server()->Tick() % 100 == 0) //this shitty stuff can set it right after activation to false but i dont care
							m_DummyAttackedOnSpawn = false;
					}

					if (m_DummyAttackedOnSpawn)
					{
						int r = rand() % 88;

						if (r > 44)
							m_Input.m_Fire++;

						int rr = rand() % 1337;
						if (rr > 420)
							SetWeapon(0);

						CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this);
						if (pChr && pChr->IsAlive())
						{
							int r = rand() % 10 - 10;

							m_Input.m_TargetX = pChr->m_Pos.x - m_Pos.x;
							m_Input.m_TargetY = pChr->m_Pos.y - m_Pos.y - r;

							if (Server()->Tick() % 13 == 0)
								GameServer()->SendEmoticon(m_pPlayer->GetCID(), 9);

							if (m_Core.m_HookState == HOOK_GRABBED || (m_Core.m_Pos.y < 216 * 32 && pChr->m_Pos.x > 404 * 32) || (pChr->m_Pos.x > 405 * 32 && m_Core.m_Pos.x > 404 * 32 + 20))
							{
								m_Input.m_Hook = 1;
								if (Server()->Tick() % 10 == 0)
								{
									int x = rand() % 20;
									int y = rand() % 20 - 10;
									m_Input.m_TargetX = x;
									m_Input.m_TargetY = y;
								}
							}
						}
					}

					//CheckSlowDudesInTunnel
					if (m_Core.m_Pos.x > 415 * 32 && m_Core.m_Pos.y > 214 * 32) //wenn bot im tunnel ist
					{
						CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this, 5);
						if (pChr && pChr->IsAlive())
						{
							if (pChr->m_Core.m_Vel.x < 7.8f) //wenn der nächste spieler im tunnel ein slowdude is 
							{
								//HauDenBau
								SetWeapon(0); //hol den hammer raus!

								m_Input.m_TargetX = pChr->m_Pos.x - m_Pos.x;
								m_Input.m_TargetY = pChr->m_Pos.y - m_Pos.y;
								m_LatestInput.m_TargetX = pChr->m_Pos.x - m_Pos.x;
								m_LatestInput.m_TargetY = pChr->m_Pos.y - m_Pos.y;

								if (m_FreezeTick == 0) //nicht rum schrein
								{
									m_LatestInput.m_Fire++;
									m_Input.m_Fire++;
								}

								if (Server()->Tick() % 10 == 0)  //angry emotes machen
									GameServer()->SendEmoticon(m_pPlayer->GetCID(), 9);
							}
						}
					}

					//CheckSpeedInTunnel
					if (m_Core.m_Pos.x > 425 * 32 && m_Core.m_Pos.y > 214 * 32 && m_Core.m_Vel.x < 9.4f) //wenn nich genung speed zum wb spot springen
						m_DummyGetSpeed = true;

					if (m_DummyGetSpeed) //wenn schwung holen == true (tunnel)
					{
						if (m_Core.m_Pos.x > 422 * 32) //zu weit rechts
						{
							//---> hol schwung für den jump
							m_Input.m_Direction = -1;

							//new hammer agressive in the walkdirection to free the way
							if (m_FreezeTime == 0)
							{
								m_Input.m_TargetX = -200;
								m_Input.m_TargetY = -2;
								if (Server()->Tick() % 20 == 0)
									SetWeapon(0);
								if (Server()->Tick() % 25 == 0)
								{
									m_Input.m_Fire++;
									m_LatestInput.m_Fire++;
								}
							}
						}
						else //wenn weit genung links
							m_DummyGetSpeed = false;
					}
					else
					{
						if (m_Core.m_Pos.x < 415 * 32) //bis zum tunnel laufen
							m_Input.m_Direction = 1;
						else if (m_Core.m_Pos.x < 440 * 32 && m_Core.m_Pos.y > 213 * 32) //im tunnel laufen
							m_Input.m_Direction = 1;

						//externe if abfrage weil laufen während sprinegn xD
						if (m_Core.m_Pos.x > 413 * 32 && m_Core.m_Pos.x < 415 * 32) // in den tunnel springen
							m_Input.m_Jump = 1;
						else if (m_Core.m_Pos.x > 428 * 32 - 20 && m_Core.m_Pos.y > 213 * 32) // im tunnel springen
							m_Input.m_Jump = 1;

						if (m_Core.m_Pos.x > 428 * 32 && m_Core.m_Pos.y > 213 * 32) // im tunnel springen nicht mehr springen
							m_Input.m_Jump = 0;

						//nochmal extern weil springen während springen
						if (m_Core.m_Pos.x > 430 * 32 && m_Core.m_Pos.y > 213 * 32) // im tunnel springen springen
							m_Input.m_Jump = 1;

						if (m_Core.m_Pos.x > 431 * 32 && m_Core.m_Pos.y > 213 * 32) //jump refillen für wayblock spot
							m_Input.m_Jump = 0;
					}
				}

				if (!isFreezed || m_Core.m_Vel.x < -0.5f || m_Core.m_Vel.x > 0.5f || m_Core.m_Vel.y != 0.000000f)
				{
					//mach nichts
				}
				else
				{
					if (Server()->Tick() % 150 == 0)
						Die(m_pPlayer->GetCID(), WEAPON_SELF);
				}

				m_DummyEmergency = false;

				//normaler internen wb spot stuff
				//if (m_Core.m_Pos.y < 213 * 32) //old new added a x check idk why the was no
				if (m_Core.m_Pos.y < 213 * 32 && m_Core.m_Pos.x > 415 * 32)
				{
					CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this, 2);
					if (pChr && pChr->IsAlive())
					{
						//sometimes walk to enemys.   to push them in freeze or super hammer them away
						if (pChr->m_Pos.y < m_Core.m_Pos.y + 2 && pChr->m_Pos.y > m_Core.m_Pos.y - 9)
						{
							if (pChr->m_Pos.x > m_Core.m_Pos.x)
								m_Input.m_Direction = 1;
							else
								m_Input.m_Direction = -1;
						}

						if (pChr->m_FreezeTime == 0) //if enemy in ruler spot is unfreeze -->notstand panic
						{
							if (Server()->Tick() % 30 == 0)  //angry emotes machen
								GameServer()->SendEmoticon(m_pPlayer->GetCID(), 9);

							if (Server()->Tick() % 20 == 0)
								SetWeapon(0);

							m_Input.m_TargetX = pChr->m_Pos.x - m_Pos.x;
							m_Input.m_TargetY = pChr->m_Pos.y - m_Pos.y;
							m_LatestInput.m_TargetX = pChr->m_Pos.x - m_Pos.x;
							m_LatestInput.m_TargetY = pChr->m_Pos.y - m_Pos.y;

							if (m_FreezeTime == 0)
							{
								m_LatestInput.m_Fire++;
								m_Input.m_Fire++;
							}

							m_DummyEmergency = true;

							if (!m_DummyLeftFreezeFull)
							{
								if (!m_DummyJumped)
								{
									m_Input.m_Jump = 1;
									m_DummyJumped = true;
								}
								else
									m_Input.m_Jump = 0;

								if (!m_DummyHooked)
								{
									if (Server()->Tick() % 30 == 0)
										m_DummyHookDelay = true;

									if (m_DummyHookDelay)
									{
										if (Server()->Tick() % 200 == 0)
										{
											m_DummyHooked = true;
											m_Input.m_Hook = 0;
										}
									}
								}

								if (!m_DummyMovedLeft)
								{
									if (m_Core.m_Pos.x > 419 * 32 + 20)
										m_Input.m_Direction = -1;
									else
										m_Input.m_Direction = 1;

									if (Server()->Tick() % 200 == 0)
									{
										m_DummyMovedLeft = true;
										m_Input.m_Direction = 0;
									}
								}
							}

							//Blocke gefreezte gegner für immer 
							if (pChr->m_FreezeTime > 0 && pChr->m_Pos.y > 204 * 32 && pChr->m_Pos.x > 422 * 32) //wenn ein gegner weit genung rechts freeze am boden liegt
							{
								if (m_Core.m_Pos.x + (5 * 32 + 40) < pChr->m_Pos.x) // er versucht 5 tiles links des gefreezten gegner zu kommen
								{
									m_Input.m_Direction = -1;

									if (m_Core.m_Pos.x > pChr->m_Pos.x && m_Core.m_Pos.x < pChr->m_Pos.x + (4 * 32)) // wenn er 4 tiles rechts des gefreezten gegners is
										m_Input.m_Jump = 1;
								}
								else //wenn der bot links des gefreezten spielers is
								{
									m_Input.m_Jump = 1;
									m_Input.m_Direction = -1;

									if (m_Core.m_Pos.x < pChr->m_Pos.x) //solange der bot weiter links is
										m_Input.m_Hook = 1;
									else
										m_Input.m_Hook = 0;
								}
							}

							if ((pChr->m_Pos.x + 10 < m_Core.m_Pos.x && pChr->m_Pos.y > 211 * 32 && pChr->m_Pos.x < 418 * 32)
								|| (pChr->m_FreezeTime > 0 && pChr->m_Pos.y > 210 * 32 && pChr->m_Pos.x < m_Core.m_Pos.x && pChr->m_Pos.x > 417 * 32 - 60)) // wenn der spieler neben der linken wand linken freeze wand liegt schiebt ihn der bot rein
							{
								if (!m_DummyLeftFreezeFull) //wenn da niemand is schieb den rein
								{
									// HIER TESTY TESTY CHANGES  211 * 32 + 40 stand hier
									if (pChr->m_Pos.y > 211 * 32 + 40) // wenn der gegner wirklich ganz tief genung is
									{
										if (m_Core.m_Pos.x > 418 * 32) // aber nicht selber ins freeze laufen
										{
											m_Input.m_Direction = -1;

											//Check ob der gegener freeze is
											if (pChr->m_FreezeTime > 0)
											{
												m_LatestInput.m_Fire = 0; //nicht schiessen ofc xD (doch is schon besser xD)
												m_Input.m_Fire = 0;
											}

											//letzten stupser geben (sonst gibs bugs kb zu fixen)
											if (pChr->isFreezed) //wenn er schon im freeze is
											{
												m_LatestInput.m_Fire = 1; //hau ihn an die wand
												m_Input.m_Fire = 1;
											}
										}
										else
										{
											m_Input.m_Direction = 1;
											if (pChr->m_FreezeTime > 0)
											{
												m_LatestInput.m_Fire = 0; //nicht schiessen ofc xD (doch is schon besser xD)
												m_Input.m_Fire = 0;
											}
										}
									}
									else //wenn der gegner nicht tief genung ist
									{
										m_Input.m_Direction = 1;

										if (pChr->m_FreezeTime > 0)
										{
											m_LatestInput.m_Fire = 0; //nicht schiessen ofc xD (doch is schon besser xD)
											m_Input.m_Fire = 0;
										}
									}
								}
							}
							else if (m_Core.m_Pos.x < 419 * 32 + 10) //sonst mehr abstand halten
								m_Input.m_Direction = 1;

							if (m_Core.m_Vel.y < 20.0f && m_Core.m_Pos.y < 207 * 32)  // wenn der tee nach oben gehammert wird
							{
								if (m_Core.m_Pos.y > 206 * 32) //ab 206 würd er so oder so ins freeze springen
									m_Input.m_Jump = 1;

								if (m_Core.m_Pos.y < pChr->m_Pos.y) //wenn der bot über dem spieler is soll er hooken
									m_Input.m_Hook = 1;
								else
									m_Input.m_Hook = 0;
							}

							if (m_Core.m_Pos.y < 207 * 32 && m_Core.m_Pos.y < pChr->m_Pos.y)
							{
								//in hammer position bewegen
								if (m_Core.m_Pos.x > 418 * 32 + 20) //nicht ins freeze laufen
								{
									if (m_Core.m_Pos.x > pChr->m_Pos.x - 45) //zu weit rechts von hammer position
										m_Input.m_Direction = -1; //gehe links
									else if (m_Core.m_Pos.x < pChr->m_Pos.x - 39) // zu weit links von hammer position
										m_Input.m_Direction = 1;
									else  //im hammer bereich
										m_Input.m_Direction = 0; //bleib da
								}
							}

							//Check ob der gegener freeze is
							if (pChr->m_FreezeTime > 0 && pChr->m_Pos.y > 208 * 32 && !pChr->isFreezed) //wenn der Gegner tief und freeze is macht es wenig sinn den frei zu hammern
							{
								m_LatestInput.m_Fire = 0; //nicht schiessen 
								m_Input.m_Fire = 0;
							}

							//Hau den weg (wie dummymode 21)
							if (pChr->m_Pos.x > 418 * 32 && pChr->m_Pos.y > 209 * 32)  //das ganze findet nur im bereich statt wo sonst eh nichts passiert
							{
								//wenn der bot den gegner nicht boosten würde hammer den auch nich weg
								m_LatestInput.m_Fire = 0;
								m_Input.m_Fire = 0;

								if (pChr->m_Core.m_Vel.y < -0.5f && m_Core.m_Pos.y + 15 > pChr->m_Pos.y) //wenn der dude speed nach oben hat
								{
									m_Input.m_Jump = 1;
									if (m_FreezeTime == 0)
									{
										m_LatestInput.m_Fire++;
										m_Input.m_Fire++;
									}
								}
							}

							if (pChr->m_Pos.x > 421 * 32 && pChr->m_FreezeTick > 0 && pChr->m_Pos.x < m_Core.m_Pos.x)
							{
								m_Input.m_Jump = 1;
								m_Input.m_Hook = 1;
							}

							//during the fight dodge the freezefloor on the left with brain
							if (m_Core.m_Pos.x > 428 * 32 + 20 && m_Core.m_Pos.x < 438 * 32 - 20)
							{
								//very nobrain directions decision
								if (m_Core.m_Pos.x < 432 * 32) //left --> go left
									m_Input.m_Direction = -1;
								else //right --> go right
									m_Input.m_Direction = 1;

								//high speed left goto speed
								if (m_Core.m_Vel.x < -6.4f && m_Core.m_Pos.x < 434 * 32)
									m_Input.m_Direction = -1;

								//low speed to the right
								if (m_Core.m_Pos.x > 431 * 32 + 20 && m_Core.m_Vel.x > 3.3f)
									m_Input.m_Direction = 1;
							}
						}
						else //sonst normal wayblocken und 427 aufsuchen
						{
							SetWeapon(1);
							m_Input.m_Jump = 0;

							if (m_Core.m_HookState == HOOK_FLYING)
								m_Input.m_Hook = 1;
							else if (m_Core.m_HookState == HOOK_GRABBED)
								m_Input.m_Hook = 1;
							else
								m_Input.m_Hook = 0;

							m_DummyJumped = false;
							m_DummyHooked = false;
							m_DummyMovedLeft = false;

							if (m_Core.m_Pos.x > 428 * 32 + 15 && m_DummyRuled) //wenn viel zu weit ausserhalb der ruler area wo der bot nur hingehookt werden kann
							{
								m_Input.m_Jump = 1;
								m_Input.m_Hook = 1;
							}

							//GameServer()->SendChat(m_pPlayer->GetCID(), CGameContext::CHAT_ALL, "Prüfe ob zu weit rechts");
							if (m_Core.m_Pos.x < (418 * 32) - 10) // zu weit links -> geh rechts
								m_Input.m_Direction = 1;
							else if (m_Core.m_Pos.x >(428 * 32) + 10) // zu weit rechts -> geh links
								m_Input.m_Direction = -1;
							else // im toleranz bereich -> stehen bleiben
							{
								m_DummyRuled = true;
								m_Input.m_Direction = 0;

								// normal wayblock
								CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this, 2);
								if (pChr && pChr->IsAlive())
								{
									//Trick[4] clears the left freeze
									if (pChr->m_Pos.x < 418 * 32 - 10 && pChr->m_Pos.y > 210 * 32 && pChr->m_Pos.y < 213 * 32 && pChr->isFreezed && pChr->m_Core.m_Vel.y == 0.00f)
										m_DummyFreezeBlockTrick = 4;

									if (pChr->m_Pos.y < 213 * 32 + 10 && pChr->m_Pos.x < 430 * 32 && pChr->m_Pos.y > 210 * 32 && pChr->m_Pos.x > 417 * 32) // wenn ein spieler auf der linken seite in der ruler area is 
									{
										//wenn ein gegner rechts des bots is prepare für trick[1]
										if (m_Core.m_Pos.x < pChr->m_Pos.x && pChr->m_Pos.x < 429 * 32 + 6)
										{
											m_Input.m_Direction = -1;
											m_Input.m_Jump = 0;

											if (m_Core.m_Pos.x < 422 * 32)
											{
												m_Input.m_Jump = 1;
												m_DummyFreezeBlockTrick = 1;
											}
										}
										//wenn ein gegner links des bots is prepare für tick[3]
										if (m_Core.m_Pos.x > pChr->m_Pos.x && pChr->m_Pos.x < 429 * 32)
										{
											m_Input.m_Direction = 1;
											m_Input.m_Jump = 0;

											if (m_Core.m_Pos.x > 427 * 32 || m_Core.m_Pos.x > pChr->m_Pos.x + (5 * 32))
											{
												m_Input.m_Jump = 1;
												m_DummyFreezeBlockTrick = 3;
											}
										}
									}
									else // wenn spieler irgendwo anders is
									{
										//wenn ein spieler rechts des freezes is und freeze is Tric[2]
										if (pChr->m_Pos.x > 433 * 32 && pChr->m_FreezeTime > 0 && IsGrounded())
											m_DummyFreezeBlockTrick = 2;
									}
								}
							}
						}
					}
					else // wenn kein lebender spieler im ruler spot ist
					{
						//Suche trozdem 427 auf
						if (m_Core.m_Pos.x < (427 * 32) - 20) // zu weit links -> geh rechts
							m_Input.m_Direction = 1;
						else if (m_Core.m_Pos.x >(427 * 32) + 40) // zu weit rechts -> geh links
							m_Input.m_Direction = -1;
					}

					// über das freeze springen wenn rechts der bevorzugenten camp stelle
					if (m_Core.m_Pos.x > 434 * 32)
						m_Input.m_Jump = 1;
					else if (m_Core.m_Pos.x > (434 * 32) - 20 && m_Core.m_Pos.x < (434 * 32) + 20) // bei flug über freeze jump wd holen
						m_Input.m_Jump = 0;

					//new testy moved tricks into Wayblocker area (y < 213 && x > 215) (was external)
					//TRICKS
					if (1 == 1)
					{
						CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this, 2);
						if (pChr && pChr->IsAlive())
						{
							if (!m_DummyEmergency && m_Core.m_Pos.x > 415 && m_Core.m_Pos.y < 213 * 32 && m_DummyFreezeBlockTrick != 0) //as long as no enemy is unfreeze in base --->  do some trickzz
							{
								//off tricks when not gud to make tricks#
								if (pChr->m_FreezeTime == 0)
								{
									m_DummyFreezeBlockTrick = 0;
									m_DummyPanicDelay = 0;
									m_DummyTrick3Check = false;
									m_DummyTrick3StartCount = false;
									m_DummyTrick3Panic = false;
									m_DummyTrick4HasStartPos = false;
								}

								if (m_DummyFreezeBlockTrick == 1) //Tick[1] enemy on the right
								{
									if (pChr->isFreezed)
										m_DummyFreezeBlockTrick = 0; //stop trick if enemy is in freeze

									m_Input.m_TargetX = pChr->m_Pos.x - m_Pos.x;
									m_Input.m_TargetY = pChr->m_Pos.y - m_Pos.y;
									m_LatestInput.m_TargetX = pChr->m_Pos.x - m_Pos.x;
									m_LatestInput.m_TargetY = pChr->m_Pos.y - m_Pos.y;

									if (Server()->Tick() % 40 == 0)
										SetWeapon(0);

									if (pChr->m_Pos.x < m_Core.m_Pos.x && pChr->m_Pos.x > m_Core.m_Pos.x - 180) //if enemy is on the left in hammer distance
									{
										m_Input.m_Fire++;
										m_LatestInput.m_Fire++;
									}

									if (m_Core.m_Pos.y < 210 * 32 + 10)
										m_DummyStartHook = true;

									if (m_DummyStartHook)
									{
										if (Server()->Tick() % 80 == 0 || pChr->m_Pos.x < m_Core.m_Pos.x + 22)
											m_DummyStartHook = false;
									}


									if (m_DummyStartHook)
										m_Input.m_Hook = 1;
									else
										m_Input.m_Hook = 0;
								}
								else if (m_DummyFreezeBlockTrick == 2) //enemy on the right plattform --> swing him away
								{
									m_Input.m_Hook = 0;
									m_Input.m_Jump = 0;
									m_Input.m_Direction = 0;
									m_LatestInput.m_Fire = 0;
									m_Input.m_Fire = 0;

									if (Server()->Tick() % 50 == 0)
									{
										m_DummyBoredCounter++;
										GameServer()->SendEmoticon(m_pPlayer->GetCID(), 7);
									}

									if (m_Core.m_Pos.x < 438 * 32) //first go right
										m_Input.m_Direction = 1;

									if (m_Core.m_Pos.x > 428 * 32 && m_Core.m_Pos.x < 430 * 32) //first jump
										m_Input.m_Jump = 1;

									if (m_Core.m_Pos.x > 427 * 32) //aim and swing
									{
										m_Input.m_TargetX = pChr->m_Pos.x - m_Pos.x;
										m_Input.m_TargetY = pChr->m_Pos.y - m_Pos.y;
										m_LatestInput.m_TargetX = pChr->m_Pos.x - m_Pos.x;
										m_LatestInput.m_TargetY = pChr->m_Pos.y - m_Pos.y;

										if (m_Core.m_Pos.x > 427 * 32 + 30 && pChr->m_Pos.y < 214 * 32)
										{
											m_Input.m_Hook = 1;
											if (Server()->Tick() % 10 == 0)
											{
												int x = rand() % 100 - 50;
												int y = rand() % 100 - 50;

												m_Input.m_TargetX = x;
												m_Input.m_TargetY = y;
											}
											//random shooting xD
											int r = rand() % 200 + 10;
											if (Server()->Tick() % r == 0 && m_FreezeTime == 0)
											{
												m_Input.m_Fire++;
												m_LatestInput.m_Fire++;
											}
										}
									}

									//also this trick needs some freeze dogining because sometime huge speed fucks the bot
									//and NOW THIS CODE is here to fuck the high speed
									if (m_Core.m_Pos.x > 440 * 32)
										m_Input.m_Direction = -1;
									if (m_Core.m_Pos.x > 439 * 32 + 20 && m_Core.m_Pos.x < 440 * 32)
										m_Input.m_Direction = 0;
								}
								else if (m_DummyFreezeBlockTrick == 3) //enemy on the left swing him to the right
								{
									m_Input.m_TargetX = pChr->m_Pos.x - m_Pos.x;
									m_Input.m_TargetY = pChr->m_Pos.y - m_Pos.y;
									m_LatestInput.m_TargetX = pChr->m_Pos.x - m_Pos.x;
									m_LatestInput.m_TargetY = pChr->m_Pos.y - m_Pos.y;

									if (m_Core.m_Pos.y < 210 * 32 + 10)
									{
										m_DummyStartHook = true;
										m_DummyTrick3StartCount = true;
									}

									if (m_DummyStartHook)
									{
										if (Server()->Tick() % 80 == 0 || pChr->m_Pos.x > m_Core.m_Pos.x - 22)
											m_DummyStartHook = false;
									}

									if (m_DummyStartHook)
										m_Input.m_Hook = 1;
									else
										m_Input.m_Hook = 0;
									if (m_Core.m_Pos.x < 429 * 32)
										m_Input.m_Direction = 1;
									else
										m_Input.m_Direction = -1;

									//STOPPER hook:
									//hook the tee if he flys to much to the right
									if (pChr->m_Pos.x > 433 * 32 + 20)
										m_Input.m_Hook = 1;

									//Hook the tee agian and go to the left -> drag him under block area
									//-->Trick 5
									if (pChr->m_Core.m_Vel.y > 8.1f && pChr->m_Pos.x > 429 * 32 + 1 && pChr->m_Pos.y > 209 * 32)
									{
										m_DummyFreezeBlockTrick = 5;
										m_Input.m_Hook = 1;
									}

									//if he lands on the right plattform switch trick xD
									//doesnt work anysways (now fixed by the stopper hook)
									if (pChr->m_Pos.x > 433 * 32 && pChr->m_Core.m_Vel.y == 0.0f)
										m_DummyFreezeBlockTrick = 2;

									//Check for trick went wrong --> trick3 panic activation
									if (m_DummyTrick3StartCount)
										m_DummyPanicDelay++;
									if (m_DummyPanicDelay > 52)
										m_DummyTrick3Check = true;
									if (m_DummyTrick3Check)
									{
										if (pChr->m_Pos.x < 430 * 32 && pChr->m_Pos.x > 426 * 32 + 10 && pChr->IsGrounded())
										{
											m_DummyTrick3Panic = true;
											m_DummyTrick3PanicLeft = true;
										}
									}
									if (m_DummyTrick3Panic)
									{
										//stuck --> go left and swing him down
										m_Input.m_Direction = 1;
										if (m_Core.m_Pos.x < 425 * 32)
											m_DummyTrick3PanicLeft = false;

										if (m_DummyTrick3PanicLeft)
											m_Input.m_Direction = -1;
										else
										{
											if (m_Core.m_Pos.y < 212 * 32 + 10)
												m_Input.m_Hook = 1;
										}
									}
								}
								else if (m_DummyFreezeBlockTrick == 4) //clear left freeze
								{
									m_Input.m_Hook = 0;
									m_Input.m_Jump = 0;
									m_Input.m_Direction = 0;
									m_LatestInput.m_Fire = 0;
									m_Input.m_Fire = 0;

									if (!m_DummyTrick4HasStartPos)
									{
										if (m_Core.m_Pos.x < 423 * 32 - 10)
											m_Input.m_Direction = 1;
										else if (m_Core.m_Pos.x > 424 * 32 - 20)
											m_Input.m_Direction = -1;
										else
											m_DummyTrick4HasStartPos = true;
									}
									else //has start pos
									{
										m_Input.m_TargetX = -200;
										m_Input.m_TargetY = -2;
										if (pChr->isFreezed)
											m_Input.m_Hook = 1;
										else
										{
											m_Input.m_Hook = 0;
											m_DummyFreezeBlockTrick = 0; //fuck it too lazy normal stuff shoudl do the rest xD
										}
										if (Server()->Tick() % 7 == 0)
											m_Input.m_Hook = 0;
									}
								}
								else if (m_DummyFreezeBlockTrick == 5) //Hook under blockarea to the left (mostly the end of a trick)
								{
									m_Input.m_Hook = 1;

									if (m_Core.m_HookState == HOOK_GRABBED)
										m_Input.m_Direction = -1;
									else
									{
										if (m_Core.m_Pos.x < 428 * 32 + 20)
											m_Input.m_Direction = 1;
									}
								}
							}
						}
						else //nobody alive in ruler area --> stop tricks
						{
							m_DummyTrick4HasStartPos = false;
							m_DummyTrick3Panic = false;
							m_DummyTrick3StartCount = false;
							m_DummyTrick3Check = false;
							m_DummyPanicDelay = 0;
							m_DummyFreezeBlockTrick = 0;
						}
					}
				}

				if (m_Core.m_Pos.x > 429 * 32 && m_Core.m_Pos.x < 436 * 32 && m_Core.m_Pos.y < 214 * 32) //dangerous area over the freeze
				{
					//first check! too low?
					if (m_Core.m_Pos.y > 211 * 32 + 10 && IsGrounded() == false)
					{
						m_Input.m_Jump = 1;
						m_Input.m_Hook = 1;
						if (Server()->Tick() % 4 == 0)
							m_Input.m_Jump = 0;
					}

					//second check! too speedy?
					if (m_Core.m_Pos.y > 209 * 32 && m_Core.m_Vel.y > 5.6f)
					{
						m_Input.m_Jump = 1;
						if (Server()->Tick() % 4 == 0)
							m_Input.m_Jump = 0;
					}
				}

				if (m_Core.m_Pos.x > 439 * 32 && m_Core.m_Pos.x < 449 * 32)
				{
					//low left lowspeed --> go left
					if (m_Core.m_Pos.x > 439 * 32 && m_Core.m_Pos.y > 209 * 32 && m_Core.m_Vel.x < 3.0f)
						m_Input.m_Direction = -1;
					//low left highrightspeed --> go right with the speed and activate some random modes to keep the speed xD
					if (m_Core.m_Pos.x > 439 * 32 && m_Core.m_Pos.y > 209 * 32 && m_Core.m_Vel.x > 6.0f && m_Core.m_Jumped < 2)
					{
						m_Input.m_Direction = 1;
						m_Input.m_Jump = 1;
						if (Server()->Tick() % 5 == 0)
							m_Input.m_Jump = 0;
						m_DummySpeedRight = true;
					}

					if (isFreezed || m_Core.m_Vel.x < 4.3f)
						m_DummySpeedRight = false;

					if (m_DummySpeedRight)
					{
						m_Input.m_Direction = 1;
						m_Input.m_TargetX = 200;
						int r = rand() % 200 - 100;
						m_Input.m_TargetY = r;
						m_Input.m_Hook = 1;
						if (Server()->Tick() % 30 == 0 && m_Core.m_HookState != HOOK_GRABBED)
							m_Input.m_Hook = 0;
					}
				}
				else //out of the freeze area resett bools
					m_DummySpeedRight = false;

				if (m_Core.m_Pos.x > 433 * 32 + 20 && m_Core.m_Pos.x < 437 * 32 && m_Core.m_Jumped > 2)
					m_Input.m_Direction = 1;

				if (m_Core.m_Pos.x > 448 * 32)
				{
					m_Input.m_Hook = 0;
					m_Input.m_Jump = 0;
					m_Input.m_Direction = 0;
					m_LatestInput.m_Fire = 0;
					m_Input.m_Fire = 0;

					if (m_Core.m_Pos.x < 451 * 32 + 20 && IsGrounded() == false && m_Core.m_Jumped > 2)
						m_Input.m_Direction = 1;
					if (m_Core.m_Vel.x < -0.8f && m_Core.m_Pos.x < 450 * 32 && IsGrounded())
						m_Input.m_Jump = 1;
					CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this);
					if (pChr && pChr->IsAlive())
					{
						if (m_Core.m_Pos.x < 451 * 32)
							m_Input.m_Direction = 1;

						if (pChr->m_Pos.x < m_Core.m_Pos.x - 7 * 32 && m_Core.m_Jumped < 2) //if enemy is on the left & bot has jump --> go left too
							m_Input.m_Direction = -1;
						if (m_Core.m_Pos.x > 454 * 32)
						{
							m_Input.m_Direction = -1;
							m_Input.m_TargetX = pChr->m_Pos.x - m_Pos.x;
							m_Input.m_TargetY = pChr->m_Pos.y - m_Pos.y;
							m_LatestInput.m_TargetX = pChr->m_Pos.x - m_Pos.x;
							m_LatestInput.m_TargetY = pChr->m_Pos.y - m_Pos.y;

							if (m_Core.m_Pos.y + 40 > pChr->m_Pos.y)
							{
								m_Input.m_Hook = 1;
								if (Server()->Tick() % 50 == 0)
									m_Input.m_Hook = 0;
							}
						}

						//second freeze from the right --> goto singel palttform
						if (m_Core.m_Pos.x > 464 * 32 && m_Core.m_Jumped > 2 && m_Core.m_Pos.x < 468 * 32)
							m_Input.m_Direction = 1;
						//go back
						if (m_Core.m_Pos.x < 468 * 32 && IsGrounded() && m_Core.m_Pos.x > 464 * 32)
							m_Input.m_Jump = 1;
						//balance
						if (m_Core.m_Pos.x > 460 * 32 && m_Core.m_Pos.x < 464 * 32 && m_Core.m_Pos.y > 210 * 32 + 10)
							m_DummyDoBalance = true;
						if (IsGrounded() && isFreezed)
							m_DummyDoBalance = false;

						if (m_DummyDoBalance)
						{
							if (m_Core.m_Pos.x > 463 * 32) //go right
							{
								if (m_Core.m_Pos.x > pChr->m_Pos.x - 4)
									m_Input.m_Direction = -1;
								else if (m_Core.m_Pos.x > pChr->m_Pos.x)
									m_Input.m_Direction = 1;

								if (m_Core.m_Pos.x < pChr->m_Pos.x)
								{
									m_Input.m_TargetX = 5;
									m_Input.m_TargetY = 200;
									if (m_Core.m_Pos.x - 1 < pChr->m_Pos.x)
									{
										m_Input.m_Fire++;
										m_LatestInput.m_Fire++;
									}
								}
								else
								{
									//do the random flick
									int r = rand() % 100 - 50;
									m_Input.m_TargetX = r;
									m_Input.m_TargetY = -200;
								}
								if (pChr->m_Pos.x > 465 * 32 - 10 && pChr->m_Pos.x < 469 * 32) //right enough go out
									m_Input.m_Direction = 1;
							}
							else //go left
							{
								if (m_Core.m_Pos.x > pChr->m_Pos.x + 4)
									m_Input.m_Direction = -1;
								else if (m_Core.m_Pos.x < pChr->m_Pos.x)
									m_Input.m_Direction = 1;

								if (m_Core.m_Pos.x > pChr->m_Pos.x)
								{
									m_Input.m_TargetX = 5;
									m_Input.m_TargetY = 200;
									if (m_Core.m_Pos.x + 1 > pChr->m_Pos.x)
									{
										m_Input.m_Fire++;
										m_LatestInput.m_Fire++;
									}
								}
								else
								{
									//do the random flick
									int r = rand() % 100 - 50;
									m_Input.m_TargetX = r;
									m_Input.m_TargetY = -200;
								}
								if (pChr->m_Pos.x < 459 * 32) //left enough go out
									m_Input.m_Direction = -1;
							}
						}

					}
					else //no close enemy alive
					{
						if (m_Core.m_Jumped < 2)
							m_Input.m_Direction = -1;
					}

					if (m_Core.m_Pos.x > 458 * 32 && m_Core.m_Pos.x < 466 * 32)
					{
						if (m_Core.m_Pos.y > 211 * 32 + 26)
							m_Input.m_Jump = 1;
						if (m_Core.m_Pos.y > 210 * 32 && m_Core.m_Vel.y > 5.4f)
							m_Input.m_Jump = 1;
					}

					//go home if its oky, oky?
					if ((m_Core.m_Pos.x < 458 * 32 && IsGrounded() && pChr->isFreezed) || (m_Core.m_Pos.x < 458 * 32 && IsGrounded() && pChr->m_Pos.x > m_Core.m_Pos.x + (10 * 32)))
						m_Input.m_Direction = -1;
					//keep going also in the air xD
					if (m_Core.m_Pos.x < 450 * 32 && m_Core.m_Vel.x < 1.1f && m_Core.m_Jumped < 2)
						m_Input.m_Direction = -1;
					//go back if too far
					if (m_Core.m_Pos.x > 468 * 32 + 20)
						m_Input.m_Direction = -1;

				}
			}
		}
		else
			m_DummyBlockMode = 0;
	}
	else if (m_pPlayer->m_Dummymode == 32) //BlmapChill police bot // made by fokkonaut
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
				m_DummyReachedCinemaEntrance = true;
				m_Input.m_Jump = 1;
			}
			else if (m_Core.m_Pos.y < 313 * 32 && m_Core.m_Pos.y > 312 * 32 && m_Core.m_Pos.x < 367 * 32)
				m_Input.m_Direction = 1;
			else if (m_Core.m_Pos.x > 367 * 32)
				m_Input.m_Direction = -1;
			if (!m_DummyReachedCinemaEntrance && m_Core.m_Pos.x < 370 * 32)
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
				if (!m_DummyLowerPanic && m_Core.m_Pos.y > 437 * 32 && m_Core.m_Pos.y > m_DummyLovedY)
				{
					m_DummyLowerPanic = 1;
					GameServer()->SendEmoticon(m_pPlayer->GetCID(), 9); //angry emote
				}

				if (m_DummyLowerPanic)
				{
					//Check for end panic
					if (m_Core.m_Pos.y < 434 * 32)
					{
						if (IsGrounded())
							m_DummyLowerPanic = 0; //made it up yay
					}

					if (m_DummyLowerPanic == 1)//position to jump on stairs
					{
						if (m_Core.m_Pos.x < 400 * 32)
							m_Input.m_Direction = 1;
						else if (m_Core.m_Pos.x > 401 * 32)
							m_Input.m_Direction = -1;
						else
							m_DummyLowerPanic = 2;
					}
					else if (m_DummyLowerPanic == 2) //jump on the left starblock element
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
							m_DummyLowerPanic = 1; //lower panic mode to reposition
					}
				}
				else //no dummy lower panic
				{
					m_DummyHelpMode = 0;
					//check if officer needs help
					CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this, 1);
					if (pChr && pChr->IsAlive())
					{
						if (m_Core.m_Pos.y > 435 * 32) // setting the destination of dummy to top left police entry bcs otherwise bot fails when trying to help --> walks into jail wall xd
						{
							m_DummyLovedX = (392 + rand() % 2) * 32;
							m_DummyLovedY = 430 * 32;
						}
						//aimbot on heuzeueu
						m_Input.m_TargetX = pChr->m_Pos.x - m_Pos.x;
						m_Input.m_TargetY = pChr->m_Pos.y - m_Pos.y;
						m_LatestInput.m_TargetX = pChr->m_Pos.x - m_Pos.x;
						m_LatestInput.m_TargetY = pChr->m_Pos.y - m_Pos.y;

						m_DummyClosestPolice = false;

						if (pChr->m_pPlayer->m_aHasItem[POLICE] || pChr->m_PoliceHelper)
							m_DummyClosestPolice = true;

						if (pChr->m_Pos.x > 444 * 32 - 10) //police dude failed too far --> to be reached by hook (set too help mode extream to leave save area)
						{
							m_DummyHelpMode = 2;
							if (m_Core.m_Jumped > 1 && m_Core.m_Pos.x > 431 * 32) //double jumped and above the freeze
								m_Input.m_Direction = -1;
							else
								m_Input.m_Direction = 1;
							//doublejump before falling in freeze
							if ((m_Core.m_Pos.x > 432 * 32 && m_Core.m_Pos.y > 432 * 32) || m_Core.m_Pos.x > 437 * 32) //over the freeze and too low
							{
								m_Input.m_Jump = 1;
								m_DummyHelpHook = true;
							}
							if (IsGrounded() && m_Core.m_Pos.x > 430 * 32 && Server()->Tick() % 60 == 0)
								m_Input.m_Jump = 1;
						}
						else
							m_DummyHelpMode = 1;

						if (m_DummyHelpMode == 1 && m_Core.m_Pos.x > 431 * 32 + 10)
							m_Input.m_Direction = -1;
						else if (m_DummyHelpMode == 1 && m_Core.m_Pos.x < 430 * 32)
							m_Input.m_Direction = 1;
						else
						{
							if (!m_DummyHelpHook && m_DummyClosestPolice)
							{
								if (m_DummyHelpMode == 2) //police dude failed too far --> to be reached by hook
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
										m_DummyHelpHook = true;
										m_Input.m_Jump = 1;
										m_Input.m_Hook = 1;
									}
								}
								else //police dude failed too near to hook from ground
								{
									if (m_Core.m_Vel.y < -4.20f && m_Core.m_Pos.y < 431 * 32)
									{
										m_DummyHelpHook = true;
										m_Input.m_Jump = 1;
										m_Input.m_Hook = 1;
									}
								}
							}
							if (Server()->Tick() % 8 == 0)
								m_Input.m_Direction = 1;
						}

						if (m_DummyHelpHook)
						{
							m_Input.m_Hook = 1;
							if (Server()->Tick() % 200 == 0)
							{
								m_DummyHelpHook = false; //timeout hook maybe failed
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

					if (!m_DummyHelpMode)
					{
						//==============
						//NOTHING TO DO
						//==============
						//basic walk to destination
						if (m_Core.m_Pos.x < m_DummyLovedX - 32)
							m_Input.m_Direction = 1;
						else if (m_Core.m_Pos.x > m_DummyLovedX + 32 && m_Core.m_Pos.x > 384 * 32)
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
									m_DummyLovedX = 420 * 32 + rand() % 69;
									m_DummyLovedY = 430 * 32;
									GameServer()->SendEmoticon(m_pPlayer->GetCID(), 7);
								}
								else
								{
									m_DummyLovedX = (392 + rand() % 2) * 32;
									m_DummyLovedY = 430 * 32;
								}
								if ((rand() % 2) == 0)
								{
									m_DummyLovedX = 384 * 32 + rand() % 128;
									m_DummyLovedY = 430 * 32;
									GameServer()->SendEmoticon(m_pPlayer->GetCID(), 5);
								}
								else
								{
									if (rand() % 3 == 0)
									{
										m_DummyLovedX = 420 * 32 + rand() % 128;
										m_DummyLovedY = 430 * 32;
										GameServer()->SendEmoticon(m_pPlayer->GetCID(), 8);
									}
									else if (rand() % 4 == 0)
									{
										m_DummyLovedX = 429 * 32 + rand() % 64;
										m_DummyLovedY = 430 * 32;
										GameServer()->SendEmoticon(m_pPlayer->GetCID(), 8);
									}
								}
								if (rand() % 5 == 0) //lower middel base
								{
									m_DummyLovedX = 410 * 32 + rand() % 64;
									m_DummyLovedY = 443 * 32;
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
				if (m_Core.m_Pos.y > m_DummyLovedY) //only if beloved place is an upper one
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