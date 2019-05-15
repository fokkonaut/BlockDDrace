/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
#include "gamecontext.h"
#include <engine/shared/config.h>
#include <game/server/teams.h>
#include <game/server/gamemodes/DDRace.h>
#include <game/version.h>
#if defined(CONF_SQL)
#include <game/server/score/sql_score.h>
#endif

bool CheckClientID(int ClientID);

void CGameContext::ConGoLeft(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	pSelf->MoveCharacter(pResult->m_ClientID, -1, 0);
}

void CGameContext::ConGoRight(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	pSelf->MoveCharacter(pResult->m_ClientID, 1, 0);
}

void CGameContext::ConGoDown(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	pSelf->MoveCharacter(pResult->m_ClientID, 0, 1);
}

void CGameContext::ConGoUp(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	pSelf->MoveCharacter(pResult->m_ClientID, 0, -1);
}

void CGameContext::ConMove(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	pSelf->MoveCharacter(pResult->m_ClientID, pResult->GetInteger(0),
			pResult->GetInteger(1));
}

void CGameContext::ConMoveRaw(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	pSelf->MoveCharacter(pResult->m_ClientID, pResult->GetInteger(0),
			pResult->GetInteger(1), true);
}

void CGameContext::MoveCharacter(int ClientID, int X, int Y, bool Raw)
{
	CCharacter* pChr = GetPlayerChar(ClientID);

	if (!pChr)
		return;

	pChr->Core()->m_Pos.x += ((Raw) ? 1 : 32) * X;
	pChr->Core()->m_Pos.y += ((Raw) ? 1 : 32) * Y;
	pChr->m_DDRaceState = DDRACE_CHEAT;
}

void CGameContext::ConKillPlayer(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	int Victim = pResult->GetVictim();

	if (pSelf->m_apPlayers[Victim])
	{
		pSelf->m_apPlayers[Victim]->KillCharacter(WEAPON_GAME);
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "%s was killed by %s",
				pSelf->Server()->ClientName(Victim),
				pSelf->Server()->ClientName(pResult->m_ClientID));
		pSelf->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
	}
}

void CGameContext::ConNinja(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_NINJA, false);
}

void CGameContext::ConSuper(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr && !pChr->m_Super)
	{
		pChr->m_Super = true;
		pChr->Core()->m_Super = true;
		pChr->UnFreeze();
		pChr->m_TeamBeforeSuper = pChr->Team();
		pChr->Teams()->SetCharacterTeam(Victim, TEAM_SUPER);
		pChr->m_DDRaceState = DDRACE_CHEAT;
		if (pChr->m_Passive)
			pChr->PassiveCollision(false);
	}
}

void CGameContext::ConUnSuper(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr && pChr->m_Super)
	{
		pChr->m_Super = false;
		pChr->Core()->m_Super = false;
		pChr->Teams()->SetForceCharacterTeam(Victim, pChr->m_TeamBeforeSuper);
		if (pChr->m_Passive)
			pChr->PassiveCollision(true);
	}
}

void CGameContext::ConUnSolo(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->SetSolo(false);
}

void CGameContext::ConUnDeep(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->m_DeepFreeze = false;
}

void CGameContext::ConShotgun(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_SHOTGUN, false);
}

void CGameContext::ConGrenade(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_GRENADE, false);
}

void CGameContext::ConRifle(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_RIFLE, false);
}

void CGameContext::ConWeapons(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, -1, false);
}

void CGameContext::ConUnShotgun(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_SHOTGUN, true);
}

void CGameContext::ConUnGrenade(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_GRENADE, true);
}

void CGameContext::ConUnRifle(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_RIFLE, true);
}

void CGameContext::ConUnWeapons(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, -1, true);
}

void CGameContext::ConAddWeapon(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, pResult->GetInteger(0), false, true);
}

void CGameContext::ConRemoveWeapon(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, pResult->GetInteger(0), true, true);
}

void CGameContext::ModifyWeapons(IConsole::IResult *pResult, void *pUserData, int Weapon, bool Remove, bool AddRemoveCommand)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Offset = AddRemoveCommand ? 1 : 0;
	int Victim = pResult->NumArguments() > Offset ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = GetPlayerChar(Victim);
	if (!pChr)
		return;

	if (clamp(Weapon, -2, NUM_WEAPONS - 1) != Weapon)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
				"invalid weapon id");
		return;
	}

	int Amount = (pChr->GetPlayer()->m_Gamemode == MODE_VANILLA && Weapon != WEAPON_HAMMER) ? 10 : -1;

	bool Spread = Remove ? false : pResult->NumArguments() > 1+Offset ? pResult->GetInteger(1+Offset) : Weapon >= 0 ? pChr->m_aSpreadWeapon[Weapon] : false;

	if (Weapon == -1)
	{
		pChr->GiveWeapon(WEAPON_SHOTGUN, Remove, Amount);
		pChr->GiveWeapon(WEAPON_GRENADE, Remove, Amount);
		pChr->GiveWeapon(WEAPON_RIFLE, Remove, Amount);
		if (!Remove)
		{
			pChr->GiveWeapon(WEAPON_HAMMER, Remove);
			pChr->GiveWeapon(WEAPON_GUN, Remove, Amount);
		}
		for (int i = 0; i < WEAPON_NINJA; i++)
			if (pChr->m_aSpreadWeapon[i] != Spread)
				pChr->SpreadWeapon(i, Spread, pResult->m_ClientID);
	}
	else if (Weapon == -2)
	{
		pChr->GiveWeapon(WEAPON_PLASMA_RIFLE, Remove, Amount);
		pChr->GiveWeapon(WEAPON_HEART_GUN, Remove, Amount);
		pChr->GiveWeapon(WEAPON_STRAIGHT_GRENADE, Remove, Amount);

		for (int i = WEAPON_NINJA; i < NUM_WEAPONS; i++)
			if (pChr->m_aSpreadWeapon[i] != Spread)
				pChr->SpreadWeapon(i, Spread, pResult->m_ClientID);
	}
	else
	{
		if (Weapon == WEAPON_NINJA && pChr->m_ScrollNinja && Remove)
			pChr->ScrollNinja(false);
		pChr->GiveWeapon(Weapon, Remove, Amount);

		if (pChr->m_aSpreadWeapon[Weapon] != Spread)
			pChr->SpreadWeapon(Weapon, Spread, pResult->m_ClientID);
	}

	pChr->m_DDRaceState = DDRACE_CHEAT;
}

void CGameContext::ConToTeleporter(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	unsigned int TeleTo = pResult->GetInteger(0);

	if (((CGameControllerDDRace*)pSelf->m_pController)->m_TeleOuts[TeleTo-1].size())
	{
		int Num = ((CGameControllerDDRace*)pSelf->m_pController)->m_TeleOuts[TeleTo-1].size();
		vec2 TelePos = ((CGameControllerDDRace*)pSelf->m_pController)->m_TeleOuts[TeleTo-1][(!Num)?Num:rand() % Num];
		CCharacter* pChr = pSelf->GetPlayerChar(pResult->m_ClientID);
		if (pChr)
		{
			pChr->Core()->m_Pos = TelePos;
			pChr->m_Pos = TelePos;
			pChr->m_PrevPos = TelePos;
			pChr->m_DDRaceState = DDRACE_CHEAT;
		}
	}
}

void CGameContext::ConToCheckTeleporter(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	unsigned int TeleTo = pResult->GetInteger(0);

	if (((CGameControllerDDRace*)pSelf->m_pController)->m_TeleCheckOuts[TeleTo-1].size())
	{
		int Num = ((CGameControllerDDRace*)pSelf->m_pController)->m_TeleCheckOuts[TeleTo-1].size();
		vec2 TelePos = ((CGameControllerDDRace*)pSelf->m_pController)->m_TeleCheckOuts[TeleTo-1][(!Num)?Num:rand() % Num];
		CCharacter* pChr = pSelf->GetPlayerChar(pResult->m_ClientID);
		if (pChr)
		{
			pChr->Core()->m_Pos = TelePos;
			pChr->m_Pos = TelePos;
			pChr->m_PrevPos = TelePos;
			pChr->m_DDRaceState = DDRACE_CHEAT;
			pChr->m_TeleCheckpoint = TeleTo;
		}
	}
}

void CGameContext::ConTeleport(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	int Tele = pResult->GetVictim();
	int TeleTo = pResult->NumArguments() == 2 ? pResult->GetInteger(1) : pResult->m_ClientID;
	if (pResult->NumArguments() < 2 && Tele != pResult->m_ClientID)
		return;

	CCharacter *pChr = pSelf->GetPlayerChar(Tele);
	if (pChr && pSelf->GetPlayerChar(TeleTo))
	{
		pChr->Core()->m_Pos = pSelf->m_apPlayers[TeleTo]->m_ViewPos;
		pChr->m_Pos = pSelf->m_apPlayers[TeleTo]->m_ViewPos;
		pChr->m_PrevPos = pSelf->m_apPlayers[TeleTo]->m_ViewPos;
		pChr->m_DDRaceState = DDRACE_CHEAT;
	}
}

void CGameContext::ConKill(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];

	if (!pPlayer
			|| (pPlayer->m_LastKill
					&& pPlayer->m_LastKill
					+ pSelf->Server()->TickSpeed()
					* g_Config.m_SvKillDelay
					> pSelf->Server()->Tick()))
		return;

	pPlayer->m_LastKill = pSelf->Server()->Tick();
	pPlayer->KillCharacter(WEAPON_SELF);
	//pPlayer->m_RespawnTick = pSelf->Server()->Tick() + pSelf->Server()->TickSpeed() * g_Config.m_SvSuicidePenalty;
}

void CGameContext::ConForcePause(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();
	int Seconds = 0;
	if (pResult->NumArguments() > 1)
		Seconds = clamp(pResult->GetInteger(1), 0, 360);

	CPlayer *pPlayer = pSelf->m_apPlayers[Victim];
	if (!pPlayer)
		return;

	pPlayer->ForcePause(Seconds);
}

bool CGameContext::VoteMute(const NETADDR *pAddr, int Secs, const char *pDisplayName, int AuthedID)
{
	char aBuf[128];
	bool Found = false;

	// find a matching vote mute for this ip, update expiration time if found
	for(int i = 0; i < m_NumVoteMutes; i++)
	{
		if(net_addr_comp_noport(&m_aVoteMutes[i].m_Addr, pAddr) == 0)
		{
			m_aVoteMutes[i].m_Expire = Server()->Tick()
				+ Secs * Server()->TickSpeed();
			Found = true;
			break;
		}
	}

	if(!Found) // nothing found so far, find a free slot..
	{
		if(m_NumVoteMutes < MAX_VOTE_BANS)
		{
			m_aVoteMutes[m_NumVoteMutes].m_Addr = *pAddr;
			m_aVoteMutes[m_NumVoteMutes].m_Expire = Server()->Tick()
				+ Secs * Server()->TickSpeed();
			m_NumVoteMutes++;
			Found = true;
		}
	}
	if(Found)
	{
		if(pDisplayName)
		{
			str_format(aBuf, sizeof aBuf, "'%s' banned '%s' for %d seconds from voting.",
				Server()->ClientName(AuthedID), pDisplayName, Secs);
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "votemute", aBuf);
		}
	}
	else // no free slot found
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "votemute", "vote mute array is full");
	return Found;
}

bool CGameContext::VoteUnmute(const NETADDR *pAddr, const char *pDisplayName, int AuthedID)
{
	for (int i = 0; i < m_NumVoteMutes; i++)
	{
		if (net_addr_comp_noport(&m_aVoteMutes[i].m_Addr, pAddr) == 0)
		{
			m_NumVoteMutes--;
			m_aVoteMutes[i] = m_aVoteMutes[m_NumVoteMutes];
			if (pDisplayName)
			{
				char aBuf[128];
				str_format(aBuf, sizeof aBuf, "'%s' unbanned '%s' from voting.",
					Server()->ClientName(AuthedID), pDisplayName);
				Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "voteunmute", aBuf);
			}
			return true;
		}
	}
	return false;
}

void CGameContext::Mute(const NETADDR *pAddr, int Secs, const char *pDisplayName)
{
	char aBuf[128];
	int Found = 0;

	// find a matching mute for this ip, update expiration time if found
	for (int i = 0; i < m_NumMutes; i++)
	{
		if (net_addr_comp_noport(&m_aMutes[i].m_Addr, pAddr) == 0)
		{
			m_aMutes[i].m_Expire = Server()->Tick()
							+ Secs * Server()->TickSpeed();
			Found = 1;
		}
	}

	if (!Found) // nothing found so far, find a free slot..
	{
		if (m_NumMutes < MAX_MUTES)
		{
			m_aMutes[m_NumMutes].m_Addr = *pAddr;
			m_aMutes[m_NumMutes].m_Expire = Server()->Tick()
							+ Secs * Server()->TickSpeed();
			m_NumMutes++;
			Found = 1;
		}
	}
	if (Found)
	{
		if (pDisplayName)
		{
			str_format(aBuf, sizeof aBuf, "'%s' has been muted for %d seconds.",
					pDisplayName, Secs);
			SendChat(-1, CHAT_ALL, aBuf);
		}
	}
	else // no free slot found
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mutes", "mute array is full");
}

void CGameContext::ConVoteMute(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	if(Victim < 0 || Victim > MAX_CLIENTS || !pSelf->m_apPlayers[Victim])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "votemute", "Client ID not found");
		return;
	}

	NETADDR Addr;
	pSelf->Server()->GetClientAddr(Victim, &Addr);

	int Seconds = clamp(pResult->GetInteger(1), 1, 86400);
	bool Found = pSelf->VoteMute(&Addr, Seconds, pSelf->Server()->ClientName(Victim), pResult->m_ClientID);

	if (Found)
	{
		char aBuf[128];
		str_format(aBuf, sizeof aBuf, "'%s' banned '%s' for %d seconds from voting.",
			pSelf->Server()->ClientName(pResult->m_ClientID), pSelf->Server()->ClientName(Victim), Seconds);
		pSelf->SendChat(-1, 0, aBuf);
	}
}

void CGameContext::ConVoteUnmute(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();

	if (Victim < 0 || Victim > MAX_CLIENTS || !pSelf->m_apPlayers[Victim])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "voteunmute", "Client ID not found");
		return;
	}

	NETADDR Addr;
	pSelf->Server()->GetClientAddr(Victim, &Addr);

	bool Found = pSelf->VoteUnmute(&Addr, pSelf->Server()->ClientName(Victim), pResult->m_ClientID);
	if (Found)
	{
		char aBuf[128];
		str_format(aBuf, sizeof aBuf, "'%s' unbanned '%s' from voting.",
			pSelf->Server()->ClientName(pResult->m_ClientID), pSelf->Server()->ClientName(Victim));
		pSelf->SendChat(-1, 0, aBuf);
	}
}

void CGameContext::ConVoteMutes(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if (pSelf->m_NumVoteMutes <= 0)
	{
		// Just to make sure.
		pSelf->m_NumVoteMutes = 0;
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "votemutes",
			"There are no active vote mutes.");
		return;
	}

	char aIpBuf[64];
	char aBuf[128];
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "votemutes",
		"Active vote mutes:");
	for (int i = 0; i < pSelf->m_NumVoteMutes; i++)
	{
		net_addr_str(&pSelf->m_aVoteMutes[i].m_Addr, aIpBuf, sizeof(aIpBuf), false);
		str_format(aBuf, sizeof aBuf, "%d: \"%s\", %d seconds left", i,
			aIpBuf, (pSelf->m_aVoteMutes[i].m_Expire - pSelf->Server()->Tick()) / pSelf->Server()->TickSpeed());
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "votemutes", aBuf);
	}
}

void CGameContext::ConMute(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->Console()->Print(
			IConsole::OUTPUT_LEVEL_STANDARD,
			"mutes",
			"Use either 'muteid <client_id> <seconds>' or 'muteip <ip> <seconds>'");
}

// mute through client id
void CGameContext::ConMuteID(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	int Victim = pResult->GetVictim();

	if (Victim < 0 || Victim > MAX_CLIENTS || !pSelf->m_apPlayers[Victim])
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "muteid", "Client id not found.");
		return;
	}

	NETADDR Addr;
	pSelf->Server()->GetClientAddr(Victim, &Addr);

	pSelf->Mute(&Addr, clamp(pResult->GetInteger(1), 1, 86400),
			pSelf->Server()->ClientName(Victim));
}

// mute through ip, arguments reversed to workaround parsing
void CGameContext::ConMuteIP(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	NETADDR Addr;
	if (net_addr_from_str(&Addr, pResult->GetString(0)))
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mutes",
				"Invalid network address to mute");
	}
	pSelf->Mute(&Addr, clamp(pResult->GetInteger(1), 1, 86400), NULL);
}

// unmute by mute list index
void CGameContext::ConUnmute(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	char aIpBuf[64];
	char aBuf[64];
	int Victim = pResult->GetVictim();

	if (Victim < 0 || Victim >= pSelf->m_NumMutes)
		return;

	pSelf->m_NumMutes--;
	pSelf->m_aMutes[Victim] = pSelf->m_aMutes[pSelf->m_NumMutes];

	net_addr_str(&pSelf->m_aMutes[Victim].m_Addr, aIpBuf, sizeof(aIpBuf), false);
	str_format(aBuf, sizeof(aBuf), "Unmuted %s", aIpBuf);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mutes", aBuf);
}

// list mutes
void CGameContext::ConMutes(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;

	if (pSelf->m_NumMutes <= 0)
	{
		// Just to make sure.
		pSelf->m_NumMutes = 0;
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mutes",
			"There are no active mutes.");
		return;
	}

	char aIpBuf[64];
	char aBuf[128];
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mutes",
			"Active mutes:");
	for (int i = 0; i < pSelf->m_NumMutes; i++)
	{
		net_addr_str(&pSelf->m_aMutes[i].m_Addr, aIpBuf, sizeof(aIpBuf), false);
		str_format(aBuf, sizeof aBuf, "%d: \"%s\", %d seconds left", i, aIpBuf,
			(pSelf->m_aMutes[i].m_Expire - pSelf->Server()->Tick()) / pSelf->Server()->TickSpeed());
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mutes", aBuf);
	}
}

void CGameContext::ConModerate(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	bool HadModerator = pSelf->PlayerModerating();

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	pPlayer->m_Moderating = !pPlayer->m_Moderating;

	char aBuf[256];

	if(!HadModerator && pPlayer->m_Moderating)
		str_format(aBuf, sizeof(aBuf), "Server kick/spec votes will now be actively moderated.");

	if(!pSelf->PlayerModerating())
		str_format(aBuf, sizeof(aBuf), "Server kick/spec votes are no longer actively moderated.");

	pSelf->SendChat(-1, CHAT_ALL, aBuf, 0);

	if(pPlayer->m_Moderating)
		pSelf->SendChatTarget(pResult->m_ClientID, "Active moderator mode enabled for you.");
	else
		pSelf->SendChatTarget(pResult->m_ClientID, "Active moderator mode disabled for you.");
}

void CGameContext::ConList(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ClientID = pResult->m_ClientID;
	if(!CheckClientID(ClientID)) return;

	char zerochar = 0;
	if(pResult->NumArguments() > 0)
		pSelf->List(ClientID, pResult->GetString(0));
	else
		pSelf->List(ClientID, &zerochar);
}

void CGameContext::ConSetDDRTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CGameControllerDDRace *pController = (CGameControllerDDRace *)pSelf->m_pController;

	int Target = pResult->GetVictim();
	int Team = pResult->GetInteger(1);

	if(pController->m_Teams.m_Core.Team(Target) && pController->m_Teams.GetDDRaceState(pSelf->m_apPlayers[Target]) == DDRACE_STARTED)
		pSelf->m_apPlayers[Target]->KillCharacter(WEAPON_SELF);

	pController->m_Teams.SetForceCharacterTeam(Target, Team);
}

void CGameContext::ConUninvite(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CGameControllerDDRace *pController = (CGameControllerDDRace *)pSelf->m_pController;

	pController->m_Teams.SetClientInvited(pResult->GetInteger(1), pResult->GetVictim(), false);
}

void CGameContext::ConVoteNo(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	pSelf->ForceVote(pResult->m_ClientID, false);
}


/*************************************************
*                                                *
*              B L O C K D D R A C E             *
*                                                *
**************************************************/

void CGameContext::ConExtraWeapons(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, -2, false);
}

void CGameContext::ConUnExtraWeapons(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, -2, true);
}

void CGameContext::ConHammer(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_HAMMER, false);
}

void CGameContext::ConUnHammer(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_HAMMER, true);
}

void CGameContext::ConGun(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_GUN, false);
}

void CGameContext::ConUnGun(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_GUN, true);
}

void CGameContext::ConPlasmaRifle(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_PLASMA_RIFLE, false);
}

void CGameContext::ConUnPlasmaRifle(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_PLASMA_RIFLE, true);
}

void CGameContext::ConHeartGun(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_HEART_GUN, false);
}

void CGameContext::ConUnHeartGun(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_HEART_GUN, true);
}

void CGameContext::ConStraightGrenade(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_STRAIGHT_GRENADE, false);
}

void CGameContext::ConUnStraightGrenade(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ModifyWeapons(pResult, pUserData, WEAPON_STRAIGHT_GRENADE, true);
}

void CGameContext::ConScrollNinja(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->ScrollNinja(!pChr->m_ScrollNinja, pResult->m_ClientID);
}

void CGameContext::ConInfiniteJumps(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->InfiniteJumps(!pChr->m_SuperJump, pResult->m_ClientID);
}

void CGameContext::ConEndlessHook(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->EndlessHook(!pChr->m_EndlessHook, pResult->m_ClientID);
}

void CGameContext::ConJetpack(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->Jetpack(!pChr->m_Jetpack, pResult->m_ClientID);
}

void CGameContext::ConRainbow(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->Rainbow(!(pChr->m_Rainbow || pChr->GetPlayer()->m_InfRainbow), pResult->m_ClientID);
}

void CGameContext::ConInfRainbow(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->InfRainbow(!(pChr->m_Rainbow || pChr->GetPlayer()->m_InfRainbow), pResult->m_ClientID);
}

void CGameContext::ConAtom(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->Atom(!(pChr->m_Atom || pChr->GetPlayer()->m_InfAtom), pResult->m_ClientID);
}

void CGameContext::ConInfAtom(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->InfAtom(!(pChr->m_Atom || pChr->GetPlayer()->m_InfAtom), pResult->m_ClientID);
}

void CGameContext::ConTrail(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->Trail(!(pChr->m_Trail || pChr->GetPlayer()->m_InfTrail), pResult->m_ClientID);
}

void CGameContext::ConInfTrail(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->InfTrail(!(pChr->m_Trail || pChr->GetPlayer()->m_InfTrail), pResult->m_ClientID);
}

void CGameContext::ConSpookyGhost(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->SpookyGhost(!pChr->GetPlayer()->m_HasSpookyGhost, pResult->m_ClientID);
}

void CGameContext::ConAddMeteor(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->Meteor(true, pResult->m_ClientID);
}

void CGameContext::ConAddInfMeteor(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->InfMeteor(true, pResult->m_ClientID);
}

void CGameContext::ConRemoveMeteors(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->Meteor(false, pResult->m_ClientID);
}

void CGameContext::ConPassive(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->Passive(!pChr->m_Passive, pResult->m_ClientID);
}

void CGameContext::ConVanillaMode(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->VanillaMode(pResult->m_ClientID);
}

void CGameContext::ConDDraceMode(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->DDraceMode(pResult->m_ClientID);
}

void CGameContext::ConBloody(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->Bloody(!(pChr->m_Bloody || pChr->m_StrongBloody), pResult->m_ClientID);
}

void CGameContext::ConStrongBloody(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->StrongBloody(!pChr->m_StrongBloody, pResult->m_ClientID);
}

void CGameContext::ConPoliceHelper(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->PoliceHelper(!pChr->m_PoliceHelper, pResult->m_ClientID);
}

void CGameContext::ConHookPower(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() > 1 ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	bool ShowInfo = false;
	if (pChr)
	{
		int Power = -1;
		for (int i = 0; i < NUM_EXTRAS; i++)
		{
			if (!str_comp_nocase(pResult->GetString(0), pSelf->GetExtraName(i)))
				if (pSelf->IsValidHookPower(i))
					Power = i;
		}
		if (Power == -1)
			ShowInfo = true;
		else
		{
			if (pChr->m_HookPower == Power)
				Power = HOOK_NORMAL;
			pChr->HookPower(Power, pResult->m_ClientID);
		}
	}
	if (!pResult->NumArguments() || ShowInfo)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "console", "Available hook powers:");
		for (int i = 0; i < NUM_EXTRAS; i++)
		{
			if (pSelf->IsValidHookPower(i))
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "console", pSelf->GetExtraName(i));
		}
	}
}

void CGameContext::ConFreezeHammer(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() ? pResult->GetVictim() : pResult->m_ClientID;
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);
	if (pChr)
		pChr->FreezeHammer(!pChr->m_FreezeHammer, pResult->m_ClientID);
}

void CGameContext::ConForceFlagOwner(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->NumArguments() > 1 ? pResult->GetInteger(1) : pResult->m_ClientID;
	((CGameControllerDDRace*)pSelf->m_pController)->ForceFlagOwner(Victim, pResult->GetInteger(0));
}

void CGameContext::ConPlayerInfo(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if (pSelf->Server()->GetAuthedState(pResult->m_ClientID) != AUTHED_ADMIN)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Missing permission");
		return;
	}

	int ID = pSelf->GetCIDByName(pResult->GetString(0));
	if (ID < 0)
		return;

	CCharacter* pChr = pSelf->GetPlayerChar(ID);
	CPlayer* pPlayer = pSelf->m_apPlayers[ID];

	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "==== [PLAYER INFO] '%s' ====", pResult->GetString(0));
	pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	if (pSelf->Server()->GetAuthedState(ID) != AUTHED_NO)
	{
		str_format(aBuf, sizeof(aBuf), "Authed: %d", pSelf->Server()->GetAuthedState(ID));
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
	str_format(aBuf, sizeof(aBuf), "ClientID: %d", ID);
	pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	if (pChr)
		pSelf->SendChatTarget(pResult->m_ClientID, "Status: Ingame");
	else if (pPlayer->GetTeam() == TEAM_SPECTATORS)
		pSelf->SendChatTarget(pResult->m_ClientID, "Status: Spectator");
	else
		pSelf->SendChatTarget(pResult->m_ClientID, "Status: Dead");
	if (pPlayer->GetAccID() > 0)
	{
		str_format(aBuf, sizeof(aBuf), "AccountName: %s", pSelf->m_Accounts[pPlayer->GetAccID()].m_Username);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
	else
		pSelf->SendChatTarget(pResult->m_ClientID, "Account: Not logged in");
	if (pPlayer->m_InfRainbow)
		pSelf->SendChatTarget(pResult->m_ClientID, "Infinite Rainbow: True");
	if (pPlayer->m_InfAtom)
		pSelf->SendChatTarget(pResult->m_ClientID, "Infinite Atom: True");
	if (pPlayer->m_InfTrail)
		pSelf->SendChatTarget(pResult->m_ClientID, "Infinite Trail: True");
	if (pPlayer->m_InfMeteors > 0)
	{
		str_format(aBuf, sizeof(aBuf), "Infinite Meteors: %d", pPlayer->m_InfMeteors);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
	if (pPlayer->m_Gamemode == MODE_DDRACE)
		pSelf->SendChatTarget(pResult->m_ClientID, "Mode: DDrace");
	else if (pPlayer->m_Gamemode == MODE_VANILLA)
		pSelf->SendChatTarget(pResult->m_ClientID, "Mode: Vanilla");

	if (pChr)
	{
		if (pChr->HasFlag() == TEAM_RED)
			pSelf->SendChatTarget(pResult->m_ClientID, "Flag: Red");
		if (pChr->HasFlag() == TEAM_BLUE)
			pSelf->SendChatTarget(pResult->m_ClientID, "Flag: Blue");
		if (pChr->m_DeepFreeze)
			pSelf->SendChatTarget(pResult->m_ClientID, "Frozen: Deep");
		else if (pChr->IsFrozen)
			pSelf->SendChatTarget(pResult->m_ClientID, "Frozen: True");
		else if (pChr->m_FreezeTime)
		{
			str_format(aBuf, sizeof(aBuf), "Frozen: Freezetime: %d", pChr->m_FreezeTime);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		}
		if (pChr->m_SuperJump)
			pSelf->SendChatTarget(pResult->m_ClientID, "SuperJump: True");
		if (pChr->m_EndlessHook)
			pSelf->SendChatTarget(pResult->m_ClientID, "Endless: True");
		if (pChr->m_Jetpack)
			pSelf->SendChatTarget(pResult->m_ClientID, "Jetpack: True");
		if (pChr->m_Rainbow)
			pSelf->SendChatTarget(pResult->m_ClientID, "Rainbow: True");
		if (pChr->m_Atom)
			pSelf->SendChatTarget(pResult->m_ClientID, "Atom: True");
		if (pChr->m_Trail)
			pSelf->SendChatTarget(pResult->m_ClientID, "Trail: True");
		if (pChr->m_Bloody)
			pSelf->SendChatTarget(pResult->m_ClientID, "Bloody: True");
		if (pChr->m_StrongBloody)
			pSelf->SendChatTarget(pResult->m_ClientID, "Strong Bloody: True");
		if (pChr->m_Meteors > 0)
		{
			str_format(aBuf, sizeof(aBuf), "Meteors: %d", pChr->m_Meteors);
			pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
		}
		if (pChr->m_Passive)
			pSelf->SendChatTarget(pResult->m_ClientID, "Passive Mode: True");
		if (pChr->m_PoliceHelper)
			pSelf->SendChatTarget(pResult->m_ClientID, "Police Helper: True");
		for (int i = 0; i < NUM_WEAPONS; i++)
		{
			if (pChr->m_aSpreadWeapon[i])
			{
				str_format(aBuf, sizeof(aBuf), "Spread %s: True", pSelf->GetWeaponName(i));
				pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
			}
		}
		str_format(aBuf, sizeof(aBuf), "Position: (%.2f/%.2f)", pChr->m_Pos.x / 32, pChr->m_Pos.y / 32);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
}

void CGameContext::ConConnectDummy(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Amount = pResult->GetInteger(0);
	int Dummymode = pResult->GetInteger(1);

	if (!Amount)
		Amount = 1;

	for (int i = 0; i < Amount; i++)
		pSelf->ConnectDummy(Dummymode);
}

void CGameContext::ConDisconnectDummy(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int ID = pResult->GetVictim();
	if (ID >= 0 && ID < MAX_CLIENTS && pSelf->m_apPlayers[ID] && pSelf->m_apPlayers[ID]->m_IsDummy)
		pSelf->Server()->BotLeave(ID);
}

void CGameContext::ConDummymode(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	int Victim = pResult->GetVictim();
	CCharacter* pChr = pSelf->GetPlayerChar(Victim);

	if (!pChr || !pChr->GetPlayer()->m_IsDummy)
		return;

	if (pResult->NumArguments() == 2)
		pChr->GetPlayer()->m_Dummymode = pResult->GetInteger(1);
	else
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "Dummymode of '%s': [%d]", pSelf->Server()->ClientName(pResult->GetInteger(0)), pChr->GetPlayer()->m_Dummymode);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "console", aBuf);
	}
}

void CGameContext::ConConnectDefaultDummies(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->ConnectDefaultBots();
}