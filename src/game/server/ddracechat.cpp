/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
#include "gamecontext.h"
#include <engine/engine.h>
#include <engine/shared/config.h>
#include <engine/shared/protocol.h>
#include <game/server/teams.h>
#include <game/server/gamemodes/blockddrace.h>
#include <game/version.h>
#if defined(CONF_SQL)
#include <game/server/score/sql_score.h>
#endif

bool CheckClientID(int ClientID);

void CGameContext::ConCredits(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credits",
		"BlockDDrace is a mod by fokkonaut");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "credits",
		"Based on DDNet 11.9 based on DDRace by the DDRace developers, which is a mod of Teeworlds by the Teeworlds developers.");
}

void CGameContext::ConInfo(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"BlockDDrace Mod. Version: " GAME_VERSION);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"BlockDDrace is a mod by fokkonaut, based on DDNet 11.9.");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
		"It contains many new featurs, including block and city elements.");
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "info",
			"For more info, say '/cmdlist'");
}

void CGameContext::ConHelp(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;

	if (pResult->NumArguments() == 0)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "help",
				"/cmdlist will show a list of all chat commands");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "help",
				"/help + any command will show you the help for this command");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "help",
				"Example /help settings will display the help about /settings");
	}
	else
	{
		const char *pArg = pResult->GetString(0);
		const IConsole::CCommandInfo *pCmdInfo =
				pSelf->Console()->GetCommandInfo(pArg, CFGFLAG_SERVER, false);
		if (pCmdInfo)
		{
			if (pCmdInfo->m_pParams)
			{
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "Usage: %s %s", pCmdInfo->m_pName, pCmdInfo->m_pParams);
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "help", aBuf);
			}

			if (pCmdInfo->m_pHelp)
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "help", pCmdInfo->m_pHelp);
		}
		else
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"help",
					"Command is either unknown or you have given a blank command without any parameters.");
	}
}

void CGameContext::ConSettings(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;

	if (pResult->NumArguments() == 0)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings",
				"to check a server setting say /settings and setting's name, setting names are:");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings",
				"teams, cheats, collision, hooking, endlesshooking, me, ");
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings",
				"hitting, oldlaser, timeout, votes, pause and scores");
	}
	else
	{
		const char *pArg = pResult->GetString(0);
		char aBuf[256];
		float ColTemp;
		float HookTemp;
		pSelf->m_Tuning.Get("player_collision", &ColTemp);
		pSelf->m_Tuning.Get("player_hooking", &HookTemp);
		if (str_comp(pArg, "teams") == 0)
		{
			str_format(aBuf, sizeof(aBuf), "%s %s",
					g_Config.m_SvTeam == 1 ?
							"Teams are available on this server" :
							(g_Config.m_SvTeam == 0 || g_Config.m_SvTeam == 3) ?
									"Teams are not available on this server" :
									"You have to be in a team to play on this server", /*g_Config.m_SvTeamStrict ? "and if you die in a team all of you die" : */
									"and all of your team will die if the team is locked");
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings", aBuf);
		}
		else if (str_comp(pArg, "cheats") == 0)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings",
				g_Config.m_SvTestingCommands ?
						"Cheats are enabled on this server" :
						"Cheats are disabled on this server");
		}
		else if (str_comp(pArg, "collision") == 0)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings",
				ColTemp ?
						"Players can collide on this server" :
						"Players can't collide on this server");
		}
		else if (str_comp(pArg, "hooking") == 0)
		{
			pSelf->Console()->Print( IConsole::OUTPUT_LEVEL_STANDARD, "settings",
				HookTemp ?
						"Players can hook each other on this server" :
						"Players can't hook each other on this server");
		}
		else if (str_comp(pArg, "endlesshooking") == 0)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings",
				g_Config.m_SvEndlessDrag ?
						"Players hook time is unlimited" :
						"Players hook time is limited");
		}
		else if (str_comp(pArg, "hitting") == 0)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings",
				g_Config.m_SvHit ?
						"Players weapons affect others" :
						"Players weapons has no affect on others");
		}
		else if (str_comp(pArg, "oldlaser") == 0)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings",
				g_Config.m_SvOldLaser ?
						"Lasers can hit you if you shot them and they pull you towards the bounce origin (Like DDRace Beta)" :
						"Lasers can't hit you if you shot them, and they pull others towards the shooter");
		}
		else if (str_comp(pArg, "me") == 0)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings",
				g_Config.m_SvSlashMe ?
						"Players can use /me commands the famous IRC Command" :
						"Players can't use the /me command");
		}
		else if (str_comp(pArg, "timeout") == 0)
		{
			str_format(aBuf, sizeof(aBuf), "The Server Timeout is currently set to %d seconds", g_Config.m_ConnTimeout);
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings", aBuf);
		}
		else if (str_comp(pArg, "votes") == 0)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings",
				g_Config.m_SvVoteKick ?
						"Players can use Callvote menu tab to kick offenders" :
						"Players can't use the Callvote menu tab to kick offenders");
			if (g_Config.m_SvVoteKick)
			{
				str_format(aBuf, sizeof(aBuf),
						"Players are banned for %d minute(s) if they get voted off", g_Config.m_SvVoteKickBantime);

				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings",
						g_Config.m_SvVoteKickBantime ?
								aBuf :
								"Players are just kicked and not banned if they get voted off");
			}
		}
		else if (str_comp(pArg, "pause") == 0)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings",
				g_Config.m_SvPauseable ?
						"/spec will pause you and your tee will vanish" :
						"/spec will pause you but your tee will not vanish");
		}
		else if (str_comp(pArg, "scores") == 0)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings",
				g_Config.m_SvHideScore ?
						"Scores are private on this server" :
						"Scores are public on this server");
		}
		else
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "settings",
			"no matching settings found, type /settings to view them");
		}
	}
}

void CGameContext::ConRules(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	bool Printed = false;
	if (g_Config.m_SvDDRaceRules)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				"There are no rules.");
		Printed = true;
	}
	#define _RL(n) g_Config.m_SvRulesLine ## n
	char *pRuleLines[] = {
		_RL(1), _RL(2), _RL(3), _RL(4), _RL(5),
		_RL(6), _RL(7), _RL(8), _RL(9), _RL(10),
	};
	for(unsigned i = 0; i < sizeof(pRuleLines) / sizeof(pRuleLines[0]); i++)
	{
		if(pRuleLines[i][0])
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD,
				"rules", pRuleLines[i]);
			Printed = true;
		}
	}
	if (!Printed)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "rules",
				"No Rules Defined, Kill em all!!");
	}
}

void ToggleSpecPause(IConsole::IResult *pResult, void *pUserData, int PauseType)
{
	if(!CheckClientID(pResult->m_ClientID))
		return;

	CGameContext *pSelf = (CGameContext *) pUserData;
	IServer* pServ = pSelf->Server();
	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if(!pPlayer)
		return;

	if (pPlayer->m_Minigame == MINIGAME_SURVIVAL && pPlayer->m_SurvivalState > SURVIVAL_LOBBY && !pPlayer->IsSpectator())
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "You can't join the spectators while you are in survival");
		return;
	}

	int PauseState = pPlayer->IsPaused();
	if(PauseState > 0)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "You are force-paused for %d seconds.", (PauseState - pServ->Tick()) / pServ->TickSpeed());
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "spec", aBuf);
	}
	else if(pResult->NumArguments() > 0)
	{
		if(-PauseState == PauseType && pPlayer->m_SpectatorID != pResult->m_ClientID && pServ->ClientIngame(pPlayer->m_SpectatorID) && !str_comp(pServ->ClientName(pPlayer->m_SpectatorID), pResult->GetString(0)))
		{
			pPlayer->Pause(CPlayer::PAUSE_NONE, false);
		}
		else
		{
			pPlayer->Pause(PauseType, false);
			pPlayer->SpectatePlayerName(pResult->GetString(0));
		}
	}
	else if(-PauseState == PauseType)
	{
		pPlayer->Pause(CPlayer::PAUSE_NONE, false);
	}
	else if(-PauseState != PauseType)
	{
		pPlayer->Pause(PauseType, false);
	}
}

void ToggleSpecPauseVoted(IConsole::IResult *pResult, void *pUserData, int PauseType)
{
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CGameContext *pSelf = (CGameContext *)pUserData;
	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	if (pPlayer->m_Minigame == MINIGAME_SURVIVAL && pPlayer->m_SurvivalState > SURVIVAL_LOBBY && !pPlayer->IsSpectator())
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "You can't join the spectators while you are in survival");
		return;
	}

	int PauseState = pPlayer->IsPaused();
	if (PauseState > 0)
	{
		IServer* pServ = pSelf->Server();
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "You are force-paused for %d seconds.", (PauseState - pServ->Tick()) / pServ->TickSpeed());
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "spec", aBuf);
		return;
	}

	bool IsPlayerBeingVoted = pSelf->m_VoteCloseTime &&
		(pSelf->m_VoteKick || pSelf->m_VoteSpec) &&
		pResult->m_ClientID != pSelf->m_VoteVictim;
	if ((!IsPlayerBeingVoted && -PauseState == PauseType) ||
		(IsPlayerBeingVoted && PauseState && pPlayer->m_SpectatorID == pSelf->m_VoteVictim))
	{
		pPlayer->Pause(CPlayer::PAUSE_NONE, false);
	}
	else
	{
		pPlayer->Pause(PauseType, false);
		if (IsPlayerBeingVoted)
			pPlayer->m_SpectatorID = pSelf->m_VoteVictim;
	}
}

void CGameContext::ConToggleSpec(IConsole::IResult *pResult, void *pUserData)
{
	ToggleSpecPause(pResult, pUserData, g_Config.m_SvPauseable ? CPlayer::PAUSE_SPEC : CPlayer::PAUSE_PAUSED);
}

void CGameContext::ConToggleSpecVoted(IConsole::IResult *pResult, void *pUserData)
{
	ToggleSpecPauseVoted(pResult, pUserData, g_Config.m_SvPauseable ? CPlayer::PAUSE_SPEC : CPlayer::PAUSE_PAUSED);
}

void CGameContext::ConTogglePause(IConsole::IResult *pResult, void *pUserData)
{
	ToggleSpecPause(pResult, pUserData, CPlayer::PAUSE_PAUSED);
}

void CGameContext::ConTogglePauseVoted(IConsole::IResult *pResult, void *pUserData)
{
	ToggleSpecPauseVoted(pResult, pUserData, CPlayer::PAUSE_PAUSED);
}

void CGameContext::ConTeamTop5(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

#if defined(CONF_SQL)
	if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
		if(pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery + g_Config.m_SvSqlQueriesDelay * pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;
#endif

	if (g_Config.m_SvHideScore)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "teamtop5",
				"Showing the team top 5 is not allowed on this server.");
		return;
	}

	if (pResult->NumArguments() > 0)
		pSelf->Score()->ShowTeamTop5(pResult, pResult->m_ClientID, pUserData,
				pResult->GetInteger(0));
	else
		pSelf->Score()->ShowTeamTop5(pResult, pResult->m_ClientID, pUserData);

#if defined(CONF_SQL)
	if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
		pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
}

void CGameContext::ConTop5(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

#if defined(CONF_SQL)
	if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
		if(pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery + g_Config.m_SvSqlQueriesDelay * pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;
#endif

	if (g_Config.m_SvHideScore)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "top5",
				"Showing the top 5 is not allowed on this server.");
		return;
	}

	if (pResult->NumArguments() > 0)
		pSelf->Score()->ShowTop5(pResult, pResult->m_ClientID, pUserData,
				pResult->GetInteger(0));
	else
		pSelf->Score()->ShowTop5(pResult, pResult->m_ClientID, pUserData);

#if defined(CONF_SQL)
	if(pSelf->m_apPlayers[pResult->m_ClientID] && g_Config.m_SvUseSQL)
		pSelf->m_apPlayers[pResult->m_ClientID]->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
}

void CGameContext::ConDND(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientID(pResult->m_ClientID)) return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if(!pPlayer)
		return;

	if(pPlayer->m_DND)
	{
		pPlayer->m_DND = false;
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "dnd", "You will receive global chat and server messages");
	}
	else
	{
		pPlayer->m_DND = true;
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "dnd", "You will not receive any further global chat and server messages");
	}
}

void CGameContext::ConMap(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	if (g_Config.m_SvMapVote == 0)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "map",
				"/map is disabled");
		return;
	}

	if (pResult->NumArguments() <= 0)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "map", "Example: /map adr3 to call vote for Adrenaline 3. This means that the map name must start with 'a' and contain the characters 'd', 'r' and '3' in that order");
		return;
	}

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

#if defined(CONF_SQL)
	if(g_Config.m_SvUseSQL)
		if(pPlayer->m_LastSQLQuery + g_Config.m_SvSqlQueriesDelay * pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;
#endif

	pSelf->Score()->MapVote(&pSelf->m_pMapVoteResult, pResult->m_ClientID, pResult->GetString(0));

#if defined(CONF_SQL)
	if(g_Config.m_SvUseSQL)
		pPlayer->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
}

void CGameContext::ConTimeout(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	const char* pTimeout = pResult->NumArguments() > 0 ? pResult->GetString(0) : pPlayer->m_TimeoutCode;

	for(int i = 0; i < pSelf->Server()->MaxClients(); i++)
	{
		if (i == pResult->m_ClientID) continue;
		if (!pSelf->m_apPlayers[i]) continue;
		if (str_comp(pSelf->m_apPlayers[i]->m_TimeoutCode, pTimeout)) continue;
		if (pSelf->Server()->SetTimedOut(i, pResult->m_ClientID)) {
			if (pSelf->m_apPlayers[i]->GetCharacter())
				pSelf->SendTuningParams(i, pSelf->m_apPlayers[i]->GetCharacter()->m_TuneZone);
			return;
		}
	}

	pSelf->Server()->SetTimeoutProtected(pResult->m_ClientID);
	str_copy(pPlayer->m_TimeoutCode, pResult->GetString(0), sizeof(pPlayer->m_TimeoutCode));
}

void CGameContext::ConSave(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

#if defined(CONF_SQL)
	if(!g_Config.m_SvSaveGames)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Save-function is disabled on this server");
		return;
	}

	if(g_Config.m_SvUseSQL)
		if(pPlayer->m_LastSQLQuery + g_Config.m_SvSqlQueriesDelay * pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;

	int Team = ((CGameControllerBlockDDrace*) pSelf->m_pController)->m_Teams.m_Core.Team(pResult->m_ClientID);

	const char* pCode = pResult->GetString(0);
	char aCountry[5];
	if(str_length(pCode) > 3 && pCode[0] >= 'A' && pCode[0] <= 'Z' && pCode[1] >= 'A'
		&& pCode[1] <= 'Z' && pCode[2] >= 'A' && pCode[2] <= 'Z')
	{
		if(pCode[3] == ' ')
		{
			str_copy(aCountry, pCode, 4);
			pCode = pCode + 4;
		}
		else if(str_length(pCode) > 4 && pCode[4] == ' ')
		{
			str_copy(aCountry, pCode, 5);
			pCode = pCode + 5;
		}
		else
		{
			str_copy(aCountry, g_Config.m_SvSqlServerName, sizeof(aCountry));
		}
	}
	else
	{
		str_copy(aCountry, g_Config.m_SvSqlServerName, sizeof(aCountry));
	}

	if(str_in_list(g_Config.m_SvSqlValidServerNames, ",", aCountry))
	{
		pSelf->Score()->SaveTeam(Team, pCode, pResult->m_ClientID, aCountry);

		if(g_Config.m_SvUseSQL)
			pPlayer->m_LastSQLQuery = pSelf->Server()->Tick();
	}
	else
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "Unknown server name '%s'.", aCountry);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}

#endif
}

void CGameContext::ConLoad(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

#if defined(CONF_SQL)
	if(!g_Config.m_SvSaveGames)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Save-function is disabled on this server");
		return;
	}

	if(g_Config.m_SvUseSQL)
		if(pPlayer->m_LastSQLQuery + g_Config.m_SvSqlQueriesDelay * pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;
#endif

	if (pResult->NumArguments() > 0)
		pSelf->Score()->LoadTeam(pResult->GetString(0), pResult->m_ClientID);
	else
		return;

#if defined(CONF_SQL)
	if(g_Config.m_SvUseSQL)
		pPlayer->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
}

void CGameContext::ConTeamRank(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

#if defined(CONF_SQL)
	if(g_Config.m_SvUseSQL)
		if(pPlayer->m_LastSQLQuery + g_Config.m_SvSqlQueriesDelay * pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;
#endif

	if (pResult->NumArguments() > 0)
		if (!g_Config.m_SvHideScore)
			pSelf->Score()->ShowTeamRank(pResult->m_ClientID, pResult->GetString(0),
					true);
		else
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"teamrank",
					"Showing the team rank of other players is not allowed on this server.");
	else
		pSelf->Score()->ShowTeamRank(pResult->m_ClientID,
				pSelf->Server()->ClientName(pResult->m_ClientID));

#if defined(CONF_SQL)
	if(g_Config.m_SvUseSQL)
		pPlayer->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
}

void CGameContext::ConRank(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

#if defined(CONF_SQL)
	if(g_Config.m_SvUseSQL)
		if(pPlayer->m_LastSQLQuery + g_Config.m_SvSqlQueriesDelay * pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
			return;
#endif

	if (pResult->NumArguments() > 0)
		if (!g_Config.m_SvHideScore)
			pSelf->Score()->ShowRank(pResult->m_ClientID, pResult->GetString(0),
					true);
		else
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"rank",
					"Showing the rank of other players is not allowed on this server.");
	else
		pSelf->Score()->ShowRank(pResult->m_ClientID,
				pSelf->Server()->ClientName(pResult->m_ClientID));

#if defined(CONF_SQL)
	if(g_Config.m_SvUseSQL)
		pPlayer->m_LastSQLQuery = pSelf->Server()->Tick();
#endif
}

void CGameContext::ConLockTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	int Team = ((CGameControllerBlockDDrace*) pSelf->m_pController)->m_Teams.m_Core.Team(pResult->m_ClientID);

	bool Lock = ((CGameControllerBlockDDrace*) pSelf->m_pController)->m_Teams.TeamLocked(Team);

	if (pResult->NumArguments() > 0)
		Lock = !pResult->GetInteger(0);

	if(Team <= TEAM_FLOCK || Team >= TEAM_SUPER)
	{
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"print",
				"This team can't be locked");
		return;
	}

	if(pSelf->ProcessSpamProtection(pResult->m_ClientID))
		return;

	char aBuf[512];
	if(Lock)
	{
		((CGameControllerBlockDDrace*) pSelf->m_pController)->m_Teams.SetTeamLock(Team, false);

		str_format(aBuf, sizeof(aBuf), "'%s' unlocked your team.", pSelf->Server()->ClientName(pResult->m_ClientID));

		for (int i = 0; i < MAX_CLIENTS; i++)
			if (((CGameControllerBlockDDrace*) pSelf->m_pController)->m_Teams.m_Core.Team(i) == Team)
				pSelf->SendChatTarget(i, aBuf);
	}
	else if(!g_Config.m_SvTeamLock)
	{
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"print",
				"Team locking is disabled on this server");
	}
	else
	{
		((CGameControllerBlockDDrace*) pSelf->m_pController)->m_Teams.SetTeamLock(Team, true);

		str_format(aBuf, sizeof(aBuf), "'%s' locked your team. After the race started killing will kill everyone in your team.", pSelf->Server()->ClientName(pResult->m_ClientID));

		for (int i = 0; i < MAX_CLIENTS; i++)
			if (((CGameControllerBlockDDrace*) pSelf->m_pController)->m_Teams.m_Core.Team(i) == Team)
				pSelf->SendChatTarget(i, aBuf);
	}
}

void CGameContext::ConInviteTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CGameControllerBlockDDrace *pController = (CGameControllerBlockDDrace *)pSelf->m_pController;
	const char *pName = pResult->GetString(0);

	if(g_Config.m_SvTeam == 0 || g_Config.m_SvTeam == 3)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
				"Teams are disabled");
		return;
	}

	if(!g_Config.m_SvInvite)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "invite", "Invites are disabled");
		return;
	}

	int Team = pController->m_Teams.m_Core.Team(pResult->m_ClientID);
	if(Team > TEAM_FLOCK && Team < TEAM_SUPER)
	{
		int Target = -1;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!str_comp(pName, pSelf->Server()->ClientName(i)))
			{
				Target = i;
				break;
			}
		}

		if(Target < 0)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "invite", "Player not found");
			return;
		}

		if(pController->m_Teams.IsInvited(Team, Target))
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "invite", "Player already invited");
			return;
		}

		if(pSelf->m_apPlayers[pResult->m_ClientID] && pSelf->m_apPlayers[pResult->m_ClientID]->m_LastInvited + g_Config.m_SvInviteFrequency * pSelf->Server()->TickSpeed() > pSelf->Server()->Tick())
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "invite", "Can't invite this quickly");
			return;
		}

		pController->m_Teams.SetClientInvited(Team, Target, true);
		pSelf->m_apPlayers[pResult->m_ClientID]->m_LastInvited = pSelf->Server()->Tick();

		char aBuf[512];
		str_format(aBuf, sizeof aBuf, "'%s' invited you to team %d.", pSelf->Server()->ClientName(pResult->m_ClientID), Team);
		pSelf->SendChatTarget(Target, aBuf);

		str_format(aBuf, sizeof aBuf, "'%s' invited '%s' to your team.", pSelf->Server()->ClientName(pResult->m_ClientID), pSelf->Server()->ClientName(Target));;
		for (int i = 0; i < MAX_CLIENTS; i++)
			if (((CGameControllerBlockDDrace*) pSelf->m_pController)->m_Teams.m_Core.Team(i) == Team)
				pSelf->SendChatTarget(i, aBuf);
	}
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "invite", "Can't invite players to this team");
}

void CGameContext::ConJoinTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	CGameControllerBlockDDrace *pController = (CGameControllerBlockDDrace *)pSelf->m_pController;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	if (pSelf->m_VoteCloseTime && pSelf->m_VoteCreator == pResult->m_ClientID && (pSelf->m_VoteKick || pSelf->m_VoteSpec))
	{
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"join",
				"You are running a vote please try again after the vote is done!");
		return;
	}
	else if (g_Config.m_SvTeam == 0 || g_Config.m_SvTeam == 3)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
				"Teams are disabled");
		return;
	}
	else if (g_Config.m_SvTeam == 2 && pResult->GetInteger(0) == 0 && pPlayer->GetCharacter() && pPlayer->GetCharacter()->m_LastStartWarning < pSelf->Server()->Tick() - 3 * pSelf->Server()->TickSpeed())
	{
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"join",
				"You must join a team and play with somebody or else you can\'t play");
		pPlayer->GetCharacter()->m_LastStartWarning = pSelf->Server()->Tick();
	}

	if (pResult->NumArguments() > 0)
	{
		if (pPlayer->GetCharacter() == 0)
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
					"You can't change teams while you are dead/a spectator.");
		}
		else
		{
			if (pPlayer->m_Last_Team
					+ pSelf->Server()->TickSpeed()
					* g_Config.m_SvTeamChangeDelay
					> pSelf->Server()->Tick())
			{
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
						"You can\'t change teams that fast!");
			}
			else if (pPlayer->m_Minigame == MINIGAME_SURVIVAL && pResult->NumArguments() > 0)
			{
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
					"You can\'t join teams in survival");
			}
			else if(pResult->GetInteger(0) > 0 && pResult->GetInteger(0) < DDRACE_MAX_CLIENTS && pController->m_Teams.TeamLocked(pResult->GetInteger(0)) && !pController->m_Teams.IsInvited(pResult->GetInteger(0), pResult->m_ClientID))
			{
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
						g_Config.m_SvInvite ?
						"This team is locked using /lock. Only members of the team can unlock it using /lock." :
						"This team is locked using /lock. Only members of the team can invite you or unlock it using /lock.");
			}
			else if(pResult->GetInteger(0) > 0 && pResult->GetInteger(0) < DDRACE_MAX_CLIENTS && pController->m_Teams.Count(pResult->GetInteger(0)) >= g_Config.m_SvTeamMaxSize)
			{
				char aBuf[512];
				str_format(aBuf, sizeof(aBuf), "This team already has the maximum allowed size of %d players", g_Config.m_SvTeamMaxSize);
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join", aBuf);
			}
			else if (((CGameControllerBlockDDrace*) pSelf->m_pController)->m_Teams.SetCharacterTeam(
					pPlayer->GetCID(), pResult->GetInteger(0)))
			{
				char aBuf[512];
				str_format(aBuf, sizeof(aBuf), "%s joined team %d",
						pSelf->Server()->ClientName(pPlayer->GetCID()),
						pResult->GetInteger(0));
				pSelf->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
				pPlayer->m_Last_Team = pSelf->Server()->Tick();
			}
			else
			{
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
						"You cannot join this team at this time");
			}
		}
	}
	else
	{
		char aBuf[512];
		if (!pPlayer->IsPlaying())
		{
			pSelf->Console()->Print(
					IConsole::OUTPUT_LEVEL_STANDARD,
					"join",
					"You can't check your team while you are dead/a spectator.");
		}
		else
		{
			str_format(
					aBuf,
					sizeof(aBuf),
					"You are in team %d",
					((CGameControllerBlockDDrace*) pSelf->m_pController)->m_Teams.m_Core.Team(
							pResult->m_ClientID));
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "join",
					aBuf);
		}
	}
}

void CGameContext::ConMe(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	char aBuf[256 + 24];

	str_format(aBuf, 256 + 24, "'%s' %s",
			pSelf->Server()->ClientName(pResult->m_ClientID),
			pResult->GetString(0));
	if (g_Config.m_SvSlashMe)
		pSelf->SendChat(-2, CGameContext::CHAT_ALL, aBuf, pResult->m_ClientID);
	else
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"me",
				"/me is disabled on this server");
}

void CGameContext::ConConverse(IConsole::IResult *pResult, void *pUserData)
{
	// This will never be called
}

void CGameContext::ConWhisper(IConsole::IResult *pResult, void *pUserData)
{
	// This will never be called
}

void CGameContext::ConSetEyeEmote(IConsole::IResult *pResult,
		void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	if(pResult->NumArguments() == 0) {
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"emote",
				(pPlayer->m_EyeEmote) ?
						"You can now use the preset eye emotes." :
						"You don't have any eye emotes, remember to bind some. (until you die)");
		return;
	}
	else if(str_comp_nocase(pResult->GetString(0), "on") == 0)
		pPlayer->m_EyeEmote = true;
	else if(str_comp_nocase(pResult->GetString(0), "off") == 0)
		pPlayer->m_EyeEmote = false;
	else if(str_comp_nocase(pResult->GetString(0), "toggle") == 0)
		pPlayer->m_EyeEmote = !pPlayer->m_EyeEmote;
	pSelf->Console()->Print(
			IConsole::OUTPUT_LEVEL_STANDARD,
			"emote",
			(pPlayer->m_EyeEmote) ?
					"You can now use the preset eye emotes." :
					"You don't have any eye emotes, remember to bind some. (until you die)");
}

void CGameContext::ConEyeEmote(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (g_Config.m_SvEmotionalTees == -1)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "emote",
				"Emotes are disabled.");
		return;
	}

	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	if (pResult->NumArguments() == 0)
	{
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"emote",
				"Emote commands are: /emote surprise /emote blink /emote close /emote angry /emote happy /emote pain");
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"emote",
				"Example: /emote surprise 10 for 10 seconds or /emote surprise (default 1 second)");
	}
	else
	{
			if(pPlayer->m_LastEyeEmote + g_Config.m_SvEyeEmoteChangeDelay * pSelf->Server()->TickSpeed() >= pSelf->Server()->Tick())
				return;

			if (!str_comp(pResult->GetString(0), "angry"))
				pPlayer->m_DefEmote = EMOTE_ANGRY;
			else if (!str_comp(pResult->GetString(0), "blink"))
				pPlayer->m_DefEmote = EMOTE_BLINK;
			else if (!str_comp(pResult->GetString(0), "close"))
				pPlayer->m_DefEmote = EMOTE_BLINK;
			else if (!str_comp(pResult->GetString(0), "happy"))
				pPlayer->m_DefEmote = EMOTE_HAPPY;
			else if (!str_comp(pResult->GetString(0), "pain"))
				pPlayer->m_DefEmote = EMOTE_PAIN;
			else if (!str_comp(pResult->GetString(0), "surprise"))
				pPlayer->m_DefEmote = EMOTE_SURPRISE;
			else if (!str_comp(pResult->GetString(0), "normal"))
				pPlayer->m_DefEmote = EMOTE_NORMAL;
			else
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD,
						"emote", "Unknown emote... Say /emote");

			int Duration = 1;
			if (pResult->NumArguments() > 1)
				Duration = pResult->GetInteger(1);

			pPlayer->m_DefEmoteReset = pSelf->Server()->Tick()
							+ Duration * pSelf->Server()->TickSpeed();
			pPlayer->m_LastEyeEmote = pSelf->Server()->Tick();
	}
}

void CGameContext::ConNinjaJetpack(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	if (pResult->NumArguments())
		pPlayer->m_NinjaJetpack = pResult->GetInteger(0);
	else
		pPlayer->m_NinjaJetpack = !pPlayer->m_NinjaJetpack;
}

void CGameContext::ConShowOthers(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	if (g_Config.m_SvShowOthers)
	{
		if (pResult->NumArguments())
			pPlayer->m_ShowOthers = pResult->GetInteger(0);
		else
			pPlayer->m_ShowOthers = !pPlayer->m_ShowOthers;
	}
	else
		pSelf->Console()->Print(
				IConsole::OUTPUT_LEVEL_STANDARD,
				"showotherschat",
				"Showing players from other teams is disabled");
}

void CGameContext::ConShowAll(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	if (pResult->NumArguments())
	{
		if (pPlayer->m_ShowAll == (bool)pResult->GetInteger(0))
			return;

		pPlayer->m_ShowAll = pResult->GetInteger(0);
	}
	else
	{
		pPlayer->m_ShowAll = !pPlayer->m_ShowAll;
	}

	if (pPlayer->m_ShowAll)
		pSelf->SendChatTarget(pResult->m_ClientID, "You will now see all tees on this server, no matter the distance");
	else
		pSelf->SendChatTarget(pResult->m_ClientID, "You will no longer see all tees on this server");
}

void CGameContext::ConSpecTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	if (pResult->NumArguments())
		pPlayer->m_SpecTeam = pResult->GetInteger(0);
	else
		pPlayer->m_SpecTeam = !pPlayer->m_SpecTeam;
}

bool CheckClientID(int ClientID)
{
	if (ClientID < 0 || ClientID >= MAX_CLIENTS)
		return false;
	return true;
}

void CGameContext::ConSayTime(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	int ClientID;
	char aBufname[MAX_NAME_LENGTH];

	if (pResult->NumArguments() > 0)
	{
		for(ClientID = 0; ClientID < MAX_CLIENTS; ClientID++)
			if (str_comp(pResult->GetString(0), pSelf->Server()->ClientName(ClientID)) == 0)
				break;

		if(ClientID == MAX_CLIENTS)
			return;

		str_format(aBufname, sizeof(aBufname), "%s's", pSelf->Server()->ClientName(ClientID));
	}
	else
	{
		str_copy(aBufname, "Your", sizeof(aBufname));
		ClientID = pResult->m_ClientID;
	}

	CPlayer *pPlayer = pSelf->m_apPlayers[ClientID];
	if (!pPlayer)
		return;
	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;
	if(pChr->m_DDRaceState != DDRACE_STARTED)
		return;

	char aBuftime[64];
	int IntTime = (int)((float)(pSelf->Server()->Tick() - pChr->m_StartTime)
			/ ((float)pSelf->Server()->TickSpeed()));
	str_format(aBuftime, sizeof(aBuftime), "%s time is %s%d:%s%d",
			aBufname,
			((IntTime / 60) > 9) ? "" : "0", IntTime / 60,
			((IntTime % 60) > 9) ? "" : "0", IntTime % 60);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "time", aBuftime);
}

void CGameContext::ConSayTimeAll(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;
	if(pChr->m_DDRaceState != DDRACE_STARTED)
		return;

	char aBuftime[64];
	int IntTime = (int)((float)(pSelf->Server()->Tick() - pChr->m_StartTime)
			/ ((float)pSelf->Server()->TickSpeed()));
	str_format(aBuftime, sizeof(aBuftime),
			"%s\'s current race time is %s%d:%s%d",
			pSelf->Server()->ClientName(pResult->m_ClientID),
			((IntTime / 60) > 9) ? "" : "0", IntTime / 60,
			((IntTime % 60) > 9) ? "" : "0", IntTime % 60);
	pSelf->SendChat(-1, CGameContext::CHAT_ALL, aBuftime, pResult->m_ClientID);
}

void CGameContext::ConTime(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	char aBuftime[64];
	int IntTime = (int)((float)(pSelf->Server()->Tick() - pChr->m_StartTime)
			/ ((float)pSelf->Server()->TickSpeed()));
	str_format(aBuftime, sizeof(aBuftime), "Your time is %s%d:%s%d",
				((IntTime / 60) > 9) ? "" : "0", IntTime / 60,
				((IntTime % 60) > 9) ? "" : "0", IntTime % 60);
	pSelf->SendBroadcast(aBuftime, pResult->m_ClientID);
}

void CGameContext::ConRescue(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	if (!g_Config.m_SvRescue) {
		pSelf->SendChatTarget(pPlayer->GetCID(), "Rescue is not enabled on this server");
		return;
	}

	pChr->Rescue();
}

void CGameContext::ConProtectedKill(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *) pUserData;
	if (!CheckClientID(pResult->m_ClientID))
		return;
	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	CCharacter* pChr = pPlayer->GetCharacter();
	if (!pChr)
		return;

	int CurrTime = (pSelf->Server()->Tick() - pChr->m_StartTime) / pSelf->Server()->TickSpeed();
	if(g_Config.m_SvKillProtection != 0 && CurrTime >= (60 * g_Config.m_SvKillProtection) && pChr->m_DDRaceState == DDRACE_STARTED)
	{
			pPlayer->KillCharacter(WEAPON_SELF);

			//char aBuf[64];
			//str_format(aBuf, sizeof(aBuf), "You killed yourself in: %s%d:%s%d",
			//		((CurrTime / 60) > 9) ? "" : "0", CurrTime / 60,
			//		((CurrTime % 60) > 9) ? "" : "0", CurrTime % 60);
			//pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
}


/*************************************************
*                                                *
*              B L O C K D D R A C E             *
*                                                *
**************************************************/

void CGameContext::ConScore(IConsole::IResult * pResult, void * pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;
	char aFormat[32];
	str_copy(aFormat, pResult->GetString(0), sizeof(aFormat));
	bool Changed = true;

	if (!str_comp_nocase(aFormat, "time"))
		pPlayer->m_DisplayScore = SCORE_TIME;
	else if (!str_comp_nocase(aFormat, "level"))
		pPlayer->m_DisplayScore = SCORE_LEVEL;
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "~~~ Score Format ~~~");
		pSelf->SendChatTarget(pResult->m_ClientID, "You can choose what the player score will display:");
		pSelf->SendChatTarget(pResult->m_ClientID, "time, level");
		Changed = false;
	}

	if (Changed)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "Changed displayed score to '%s'.", aFormat);
		pSelf->SendChatTarget(pResult->m_ClientID, aBuf);
	}
}

void CGameContext::ConSpookyGhostInfo(IConsole::IResult * pResult, void * pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SendChatTarget(pResult->m_ClientID, "~~~ Spooky Ghost ~~~");
	pSelf->SendChatTarget(pResult->m_ClientID, "The Spooky Ghost is an extra, that can be toggled like this:");
	pSelf->SendChatTarget(pResult->m_ClientID, "Hold TAB (or other scoreboard key) and shoot two times with your gun.");
}

void CGameContext::ConWeaponIndicator(IConsole::IResult * pResult, void * pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	pPlayer->m_WeaponIndicator ^= true;

	if (pPlayer->m_WeaponIndicator)
		pSelf->SendChatTarget(pResult->m_ClientID, "Weapon indicator enabled");
	else
		pSelf->SendChatTarget(pResult->m_ClientID, "Weapon indicator disabled");
}

void CGameContext::ConRegister(IConsole::IResult * pResult, void * pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if (!g_Config.m_SvAccounts)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Accounts are not supported on this server");
		return;
	}

	if (pResult->NumArguments() != 3)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Usage: register s[name] s[password] s[password]");
		return;
	}

	char aUsername[32];
	char aPassword[32];
	char aPassword2[32];
	str_copy(aUsername, pResult->GetString(0), sizeof(aUsername));
	str_copy(aPassword, pResult->GetString(1), sizeof(aPassword));
	str_copy(aPassword2, pResult->GetString(2), sizeof(aPassword2));

	if (str_length(aUsername) > 20 || str_length(aUsername) < 3)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "The username is too long or too short");
		return;
	}

	char aAllowedCharSet[64] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	bool UnallowedChar = false;

	for (int i = 0; i < str_length(aUsername); i++)
	{
		bool NoUnallowedChars = false;

		for (int j = 0; j < str_length(aAllowedCharSet); j++)
		{
			if (aUsername[i] == aAllowedCharSet[j])
			{
				NoUnallowedChars = true;
				break;
			}
		}

		if (!NoUnallowedChars)
		{
			UnallowedChar = true;
			break;
		}
	}
	if (UnallowedChar)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Your username can only consist of letters and numbers");
		return;
	}

	if (str_comp_nocase(aPassword, aPassword2) != 0)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "The passwords need to be identical");
		return;
	}

	if (str_length(aPassword) > 20 || str_length(aPassword) < 3 || str_length(aPassword2) > 20 || str_length(aPassword2) < 3)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "The password is too long or too short");
		return;
	}

	for (unsigned int i = ACC_START; i < pSelf->m_Accounts.size(); i++)
		if (!str_comp_nocase(pSelf->m_Accounts[i].m_Username, aUsername))
		{
			pSelf->SendChatTarget(pResult->m_ClientID, "Username already exsists");
			return;
		}

	int ID = pSelf->AddAccount();
	str_copy(pSelf->m_Accounts[ID].m_Password, aPassword, sizeof(pSelf->m_Accounts[ID].m_Password));
	str_copy(pSelf->m_Accounts[ID].m_Username, aUsername, sizeof(pSelf->m_Accounts[ID].m_Username));
	pSelf->Logout(ID);

	pSelf->SendChatTarget(pResult->m_ClientID, "Successfully registered an account, you can login now");
	dbg_msg("acc", "account created, file '%s/%s.acc'", g_Config.m_SvAccFilePath, aUsername);
}

void CGameContext::ConLogin(IConsole::IResult * pResult, void * pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	if (!g_Config.m_SvAccounts)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Accounts are not supported on this server");
		return;
	}

	char aUsername[32];
	char aPassword[128];
	str_copy(aUsername, pResult->GetString(0), sizeof(aUsername));
	str_copy(aPassword, pResult->GetString(1), sizeof(aPassword));

	if (pPlayer->GetAccID() >= ACC_START)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "You are already logged in");
		return;
	}

	if (pResult->NumArguments() != 2)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Usage: login s[name] s[password]");
		return;
	}

	int ID = pSelf->AddAccount();
	pSelf->ReadAccountStats(ID, aUsername);

	if (pSelf->m_Accounts[ID].m_Username[0] == 0)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "That account doesn't exist, please register first");
		pSelf->m_Accounts.erase(pSelf->m_Accounts.begin() + ID);
		return;
	}

	if (pSelf->m_Accounts[ID].m_LoggedIn)
	{
		if (pSelf->m_Accounts[ID].m_Port == g_Config.m_SvPort)
			pSelf->SendChatTarget(pResult->m_ClientID, "This account is already logged in");
		else
			pSelf->SendChatTarget(pResult->m_ClientID, "This account is already logged in on another server");
		pSelf->m_Accounts.erase(pSelf->m_Accounts.begin() + ID);
		return;
	}

	if (pSelf->m_Accounts[ID].m_Disabled)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "This account is disabled");
		pSelf->m_Accounts.erase(pSelf->m_Accounts.begin() + ID);
		return;
	}

	if (str_comp(pSelf->m_Accounts[ID].m_Password, aPassword))
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Wrong password");
		pSelf->m_Accounts.erase(pSelf->m_Accounts.begin() + ID);
		return;
	}

	pSelf->m_Accounts[ID].m_Port = g_Config.m_SvPort;
	pSelf->m_Accounts[ID].m_LoggedIn = true;
	pSelf->m_Accounts[ID].m_ClientID = pResult->m_ClientID;
	pSelf->WriteAccountStats(ID);

	pSelf->SendChatTarget(pResult->m_ClientID, "Successfully logged in");
}

void CGameContext::ConLogout(IConsole::IResult * pResult, void * pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	if (!g_Config.m_SvAccounts && pPlayer->GetAccID() < ACC_START)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "Accounts are not supported on this server");
		return;
	}

	if (pPlayer->GetAccID() < ACC_START)
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "You are not logged in");
		return;
	}

	pSelf->Logout(pPlayer->GetAccID());
}

void CGameContext::ConPoliceInfo(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	int Page = pResult->GetInteger(0);
	int MaxPages = 4;	//////UPDATE THIS WITH EVERY PAGE YOU ADD
	if (!Page || Page > MaxPages)
		Page = 1;

	char aInfo[128];
	char aPage[128];
	str_format(aInfo, sizeof(aInfo), "Use '/policeinfo <page>' to check out what other police ranks can do.");
	str_format(aPage, sizeof(aPage), "-- Page %d/%d --", Page, MaxPages);

	pSelf->SendChatTarget(pResult->m_ClientID, "~~~ Police Info ~~~");
	if (Page >= 2 && Page <= MaxPages)
	{
		int Level = 0;
		int Policelevel = Page - 1;
		char aPolice[32];

		if (Policelevel == 1)
			Level = 18;
		else if (Policelevel == 2)
			Level = 25;
		else if (Policelevel == 3)
			Level = 30;

		str_format(aPolice, sizeof(aPolice), "[POLICE %d]", Policelevel);
		pSelf->SendChatTarget(pResult->m_ClientID, aPolice);

		str_format(aPolice, sizeof(aPolice), "Level needed to buy: [LVL %d]", Level);
		pSelf->SendChatTarget(pResult->m_ClientID, aPolice);

		pSelf->SendChatTarget(pResult->m_ClientID, "Benefits:");
		if (Policelevel == 1)
			pSelf->SendChatTarget(pResult->m_ClientID, "- The police bot will help you");
		else if (Policelevel == 2)
			pSelf->SendChatTarget(pResult->m_ClientID, "- '/policehelper'");
		else if (Policelevel == 3)
			pSelf->SendChatTarget(pResult->m_ClientID, "- taser license ('/taser')");
	}
	else
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "[GENERAL INFORMATION]");
		pSelf->SendChatTarget(pResult->m_ClientID, "Police can be bought in shop using '/buy police'.");
		pSelf->SendChatTarget(pResult->m_ClientID, "There are multiple police ranks, each cost 100.000 money.");
		pSelf->SendChatTarget(pResult->m_ClientID, "The policebot will help every police officer.");
		pSelf->SendChatTarget(pResult->m_ClientID, "Every police rank will give you more benefits.");
	}
	pSelf->SendChatTarget(pResult->m_ClientID, "------------------------");
	pSelf->SendChatTarget(pResult->m_ClientID, "Use '/policeinfo <page>' for information about other ranks");
	pSelf->SendChatTarget(pResult->m_ClientID, aPage);
}

void CGameContext::ConMinigames(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	char aMinigames[256];
	char aTemp2[256];
	aMinigames[0] = 0;
	aTemp2[0] = 0;
	for (int i = MINIGAME_BLOCK; i < NUM_MINIGAMES; i++)
	{
		if (i != MINIGAME_BLOCK)
			str_format(aTemp2, sizeof(aTemp2), "%s, ", aMinigames);
		str_format(aMinigames, sizeof(aMinigames), "%s%s", aTemp2, pSelf->GetMinigameName(i));
	}

	pSelf->SendChatTarget(pResult->m_ClientID, "~~~ Minigames ~~~");
	pSelf->SendChatTarget(pResult->m_ClientID, "You can join any minigame using '/<minigame>'");
	pSelf->SendChatTarget(pResult->m_ClientID, "To leave a minigame, just type '/leave'");
	pSelf->SendChatTarget(pResult->m_ClientID, "Here is a list of all minigames:");
	pSelf->SendChatTarget(pResult->m_ClientID, aMinigames);
}

void CGameContext::SetMinigame(IConsole::IResult *pResult, void *pUserData, int Minigame)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	CPlayer* pPlayer = m_apPlayers[pResult->m_ClientID];
	if (!pPlayer)
		return;

	char aMsg[64];

	// admins can enable or disable minigames with /<minigame> <enable/disable>
	if (pResult->NumArguments() && pSelf->Server()->GetAuthedState(pResult->m_ClientID) == AUTHED_ADMIN && Minigame != MINIGAME_NONE)
	{
		bool Disable;
		if (!str_comp_nocase(pResult->GetString(0), "enable"))
			Disable = false;
		else if (!str_comp_nocase(pResult->GetString(0), "disable"))
			Disable = true;
		else
			return;

		str_format(aMsg, sizeof(aMsg), "Minigame '%s' %s%sd", pSelf->GetMinigameName(Minigame), (pSelf->m_aMinigameDisabled[Minigame] == Disable ? "is already " : ""), pResult->GetString(0));
		pSelf->SendChatTarget(pResult->m_ClientID, aMsg);

		if (!Disable)
			pSelf->m_aMinigameDisabled[Minigame] = false;
		if (Disable)
			pSelf->m_aMinigameDisabled[Minigame] = true;
		return;
	}

	// check whether minigame is disabled
	if (Minigame != MINIGAME_NONE && pSelf->m_aMinigameDisabled[Minigame])
	{
		pSelf->SendChatTarget(pResult->m_ClientID, "This minigame is disabled");
		return;
	}

	// check if we are already in a minigame
	if (pPlayer->m_Minigame == Minigame)
	{
		// you can't leave when you're not in a minigame
		if (Minigame == MINIGAME_NONE)
			pSelf->SendChatTarget(pResult->m_ClientID, "You are not in a minigame");
		else
		{
			str_format(aMsg, sizeof(aMsg), "You are already in minigame '%s'", pSelf->GetMinigameName(Minigame));
			pSelf->SendChatTarget(pResult->m_ClientID, aMsg);
		}
		return;
	}

	// leave minigame
	if (Minigame == MINIGAME_NONE)
	{
		str_format(aMsg, sizeof(aMsg), "You left the minigame '%s'", pSelf->GetMinigameName(pPlayer->m_Minigame));
		pSelf->SendChatTarget(pResult->m_ClientID, aMsg);

		//reset everything
		if (pPlayer->m_Minigame == MINIGAME_SURVIVAL)
		{
			pPlayer->m_Gamemode = g_Config.m_SvVanillaModeStart ? GAMEMODE_VANILLA : GAMEMODE_DDRACE;
			pPlayer->m_SurvivalState = SURVIVAL_OFFLINE;
		}
	}
	// join minigame
	else if (pPlayer->m_Minigame == MINIGAME_NONE)
	{
		str_format(aMsg, sizeof(aMsg), "You joined the minigame '%s'", pSelf->GetMinigameName(Minigame));
		pSelf->SendChatTarget(pResult->m_ClientID, aMsg);
		pSelf->SendChatTarget(pResult->m_ClientID, "Say '/leave' to join the normal area again");

		//set minigame required stuff
		((CGameControllerBlockDDrace*)pSelf->m_pController)->m_Teams.SetCharacterTeam(pPlayer->GetCID(), 0);

		if (Minigame == MINIGAME_SURVIVAL)
		{
			pPlayer->m_Gamemode = GAMEMODE_VANILLA;
			pPlayer->m_SurvivalState = SURVIVAL_LOBBY;
		}
	}
	else
	{
		// you can't join minigames if you are already in another mingame
		pSelf->SendChatTarget(pResult->m_ClientID, "You have to leave first in order to join another minigame");
		return;
	}

	pPlayer->KillCharacter(WEAPON_GAME);
	pPlayer->m_Minigame = Minigame;
}

void CGameContext::ConLeaveMinigame(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SetMinigame(pResult, pUserData, MINIGAME_NONE);
}

void CGameContext::ConJoinBlock(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SetMinigame(pResult, pUserData, MINIGAME_BLOCK);
}

void CGameContext::ConJoinSurvival(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SetMinigame(pResult, pUserData, MINIGAME_SURVIVAL);
}

void CGameContext::ConJoinBoomFNG(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SetMinigame(pResult, pUserData, MINIGAME_INSTAGIB_BOOMFNG);
}

void CGameContext::ConJoinFNG(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->SetMinigame(pResult, pUserData, MINIGAME_INSTAGIB_FNG);
}