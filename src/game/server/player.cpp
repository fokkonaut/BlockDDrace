/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <engine/shared/config.h>
#include "player.h"

#include <engine/server.h>
#include "gamecontext.h"
#include <game/gamecore.h>
#include <game/version.h>
#include <game/server/teams.h>
#include "gamemodes/blockddrace.h"
#include <time.h>


MACRO_ALLOC_POOL_ID_IMPL(CPlayer, MAX_CLIENTS)

IServer *CPlayer::Server() const { return m_pGameServer->Server(); }

CPlayer::CPlayer(CGameContext *pGameServer, int ClientID, int Team)
{
	m_pGameServer = pGameServer;
	m_ClientID = ClientID;
	m_Team = GameServer()->m_pController->ClampTeam(Team);
	m_NumInputs = 0;
	Reset();
}

CPlayer::~CPlayer()
{
	delete m_pCharacter;
	m_pCharacter = 0;
}

void CPlayer::Reset()
{
	m_RespawnTick = Server()->Tick();
	m_DieTick = Server()->Tick();
	m_JoinTick = Server()->Tick();
	delete m_pCharacter;
	m_pCharacter = 0;
	m_KillMe = 0;
	m_SpectatorID = SPEC_FREEVIEW;
	m_LastActionTick = Server()->Tick();
	m_TeamChangeTick = Server()->Tick();
	m_LastInvited = 0;
	m_WeakHookSpawn = false;

	// BlockDDrace // used for 256p support
	m_SnapFixDDNet = true;

	int *pIdMap = Server()->GetIdMap(m_ClientID);
	for (int i = 1;i < DDRACE_MAX_CLIENTS;i++)
	{
		pIdMap[i] = -1;
	}
	pIdMap[0] = m_ClientID;

	// DDRace

	m_LastCommandPos = 0;
	m_LastPlaytime = time_get();
	m_Sent1stAfkWarning = 0;
	m_Sent2ndAfkWarning = 0;
	m_ChatScore = 0;
	m_Moderating = false;
	m_EyeEmote = true;
	m_TimerType = (g_Config.m_SvDefaultTimerType == CPlayer::TIMERTYPE_GAMETIMER || g_Config.m_SvDefaultTimerType == CPlayer::TIMERTYPE_GAMETIMER_AND_BROADCAST) ? CPlayer::TIMERTYPE_BROADCAST : g_Config.m_SvDefaultTimerType;
	m_DefEmote = EMOTE_NORMAL;
	m_Afk = false;
	m_LastWhisperTo = -1;
	m_LastSetSpectatorMode = 0;
	m_TimeoutCode[0] = '\0';

	m_TuneZone = 0;
	m_TuneZoneOld = m_TuneZone;
	m_Halloween = false;
	m_FirstPacket = true;

	m_SendVoteIndex = -1;

	if(g_Config.m_Events)
	{
		time_t rawtime;
		struct tm* timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		if ((timeinfo->tm_mon == 11 && timeinfo->tm_mday == 31) || (timeinfo->tm_mon == 0 && timeinfo->tm_mday == 1))
		{ // New Year
			m_DefEmote = EMOTE_HAPPY;
		}
		else if ((timeinfo->tm_mon == 9 && timeinfo->tm_mday == 31) || (timeinfo->tm_mon == 10 && timeinfo->tm_mday == 1))
		{ // Halloween
			m_DefEmote = EMOTE_ANGRY;
			m_Halloween = true;
		}
		else
		{
			m_DefEmote = EMOTE_NORMAL;
		}
	}
	m_DefEmoteReset = -1;

	GameServer()->Score()->PlayerData(m_ClientID)->Reset();

	m_ClientVersion = VERSION_VANILLA;
	m_ShowOthers = g_Config.m_SvShowOthersDefault;
	m_ShowAll = g_Config.m_SvShowAllDefault;
	m_SpecTeam = false;
	m_ResumeMoved = false;
	m_NinjaJetpack = false;

	m_Paused = PAUSE_NONE;
	m_DND = false;

	m_LastPause = 0;
	m_Score = -9999;
	m_HasFinishScore = false;

	// Variable initialized:
	m_Last_Team = 0;
#if defined(CONF_SQL)
	m_LastSQLQuery = 0;
#endif

	int64 Now = Server()->Tick();
	int64 TickSpeed = Server()->TickSpeed();
	// If the player joins within ten seconds of the server becoming
	// non-empty, allow them to vote immediately. This allows players to
	// vote after map changes or when they join an empty server.
	//
	// Otherwise, block voting in the begnning after joining.
	if(Now > GameServer()->m_NonEmptySince + 10 * TickSpeed)
		m_FirstVoteTick = Now + g_Config.m_SvJoinVoteDelay * TickSpeed;
	else
		m_FirstVoteTick = Now;

	m_NotEligibleForFinish = false;
	m_EligibleForFinishCheck = 0;

	// BlockDDrace
	m_IsDummy = false;
	m_Dummymode = DUMMYMODE_IDLE;
	m_FakePing = 0;

	m_vWeaponLimit.resize(NUM_WEAPONS);

	m_Gamemode = g_Config.m_SvVanillaModeStart ? GAMEMODE_VANILLA : GAMEMODE_DDRACE;
	m_SpookyGhost = false;
	m_RealUseCustomColor = 0;
	m_RealSkinName[0] = '\0';

	m_FixNameID = -1;
	m_ShowName = true;
	m_SetRealName = false;
	m_SetRealNameTick = Now;
	m_ChatTeam = CGameContext::CHAT_ALL;
	m_ChatText[0] = '\0';
	m_MsgKiller = -1;
	m_MsgWeapon = -1;
	m_MsgModeSpecial = 0;

	m_RainbowSpeed = g_Config.m_SvRainbowSpeedDefault;
	m_RainbowColor = 0;
	m_InfRainbow = false;
	m_InfMeteors = 0;
	m_HasSpookyGhost = false;

	m_DisplayScore = g_Config.m_SvDefaultScore;
	m_InstagibScore = 0;

	m_ForceSpawnPos = vec2(-1, -1);
	m_WeaponIndicator = g_Config.m_SvWeaponIndicatorDefault;

	m_Minigame = MINIGAME_NONE;
	m_SurvivalState = SURVIVAL_OFFLINE;
	m_ForceKilled = false;

	m_SpectatorFlag = SPEC_FREEVIEW;
	m_FlagPlayer = -1;
}

void CPlayer::Tick()
{
#ifdef CONF_DEBUG
	if(!g_Config.m_DbgDummies || m_ClientID < MAX_CLIENTS-g_Config.m_DbgDummies)
#endif
	if(!Server()->ClientIngame(m_ClientID))
		return;

	if(m_KillMe != 0)
	{
		KillCharacter(m_KillMe);
		m_KillMe = 0;
		return;
	}

	if (m_ChatScore > 0)
		m_ChatScore--;

	Server()->SetClientScore(m_ClientID, 0);

	if (m_Moderating && m_Afk)
	{
		m_Moderating = false;
		GameServer()->SendChatTarget(m_ClientID, "Active moderator mode disabled because you are afk.");

		if (!GameServer()->PlayerModerating())
			GameServer()->SendChat(-1, CGameContext::CHAT_ALL, "Server kick/spec votes are no longer actively moderated.");
	}

	// do latency stuff
	if (!m_IsDummy)
	{
		IServer::CClientInfo Info;
		if(Server()->GetClientInfo(m_ClientID, &Info))
		{
			m_Latency.m_Accum += Info.m_Latency;
			m_Latency.m_AccumMax = maximum(m_Latency.m_AccumMax, Info.m_Latency);
			m_Latency.m_AccumMin = minimum(m_Latency.m_AccumMin, Info.m_Latency);
		}
		// each second
		if(Server()->Tick()%Server()->TickSpeed() == 0)
		{
			m_Latency.m_Avg = m_Latency.m_Accum/Server()->TickSpeed();
			m_Latency.m_Max = m_Latency.m_AccumMax;
			m_Latency.m_Min = m_Latency.m_AccumMin;
			m_Latency.m_Accum = 0;
			m_Latency.m_AccumMin = 1000;
			m_Latency.m_AccumMax = 0;
		}
	}

	if(Server()->GetNetErrorString(m_ClientID)[0])
	{
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "'%s' would have timed out, but can use timeout protection now", Server()->ClientName(m_ClientID));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
		Server()->ResetNetErrorString(m_ClientID);
	}

	if(!GameServer()->m_World.m_Paused)
	{
		if(!m_pCharacter && m_DieTick+Server()->TickSpeed()*3 <= Server()->Tick())
			m_Spawning = true;

		if(m_pCharacter)
		{
			if(m_pCharacter->IsAlive())
			{
				ProcessPause();
				if(!m_Paused)
					m_ViewPos = m_pCharacter->m_Pos;
			}
			else if(!m_pCharacter->IsPaused())
			{
				delete m_pCharacter;
				m_pCharacter = 0;
			}
		}
		else if(m_Spawning && !m_WeakHookSpawn && m_RespawnTick <= Server()->Tick())
			TryRespawn();
	}
	else
	{
		++m_RespawnTick;
		++m_DieTick;
		++m_JoinTick;
		++m_LastActionTick;
		++m_TeamChangeTick;
	}

	m_TuneZoneOld = m_TuneZone; // determine needed tunings with viewpos
	int CurrentIndex = GameServer()->Collision()->GetMapIndex(m_ViewPos);
	m_TuneZone = GameServer()->Collision()->IsTune(CurrentIndex);

	if (m_TuneZone != m_TuneZoneOld) // don't send tunigs all the time
	{
		GameServer()->SendTuningParams(m_ClientID, m_TuneZone);
	}


	/*************************************************
	*                                                *
	*              B L O C K D D R A C E             *
	*                                                *
	**************************************************/

	// checking account level
	CheckLevel();

	// checking whether scoreboard is activated or not
	if (m_pCharacter)
	{
		if (m_PlayerFlags&PLAYERFLAG_SCOREBOARD)
			m_pCharacter->m_ShopMotdTick = 0;
		else
			m_pCharacter->m_NumGhostShots = 0;
	}

	// fixing messages if name is hidden
	if (m_SetRealName)
	{
		if (m_SetRealNameTick < Server()->Tick())
		{
			if (m_FixNameID == FIX_CHAT_MSG)
				GameServer()->SendChat(m_ClientID, m_ChatTeam, m_ChatText, m_ClientID);
			else if (m_FixNameID == FIX_KILL_MSG)
			{
				CNetMsg_Sv_KillMsg Msg;
				Msg.m_Killer = m_MsgKiller;
				Msg.m_Victim = GetCID();
				Msg.m_Weapon = m_MsgWeapon;
				Msg.m_ModeSpecial = m_MsgModeSpecial;
				for (int i = 0; i < MAX_CLIENTS; i++)
					if (!g_Config.m_SvHideMinigamePlayers || (GameServer()->m_apPlayers[i] && m_Minigame == GameServer()->m_apPlayers[i]->m_Minigame))
						Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, i);
			}

			m_SetRealName = false;
		}
	}
}

void CPlayer::PostTick()
{
	// update latency value
	if(m_PlayerFlags&PLAYERFLAG_SCOREBOARD)
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
				m_aActLatency[i] = GameServer()->m_apPlayers[i]->m_Latency.m_Min;
		}
	}

	// BlockDDrace
	CFlag *Flag = m_SpectatorFlag == SPEC_FREEVIEW ? 0 : ((CGameControllerBlockDDrace*)GameServer()->m_pController)->m_apFlags[m_SpectatorFlag];
	if (m_SpectatorFlag != SPEC_FREEVIEW)
	{
		if (!Flag)
		{
			m_SpectatorID = SPEC_FREEVIEW;
			m_SpectatorFlag = SPEC_FREEVIEW;
		}
		else if (Flag->GetCarrier())
			m_SpectatorID = Flag->GetCarrier()->GetPlayer()->GetCID();
		else
			m_SpectatorID = GameServer()->GetFlagPlayer(m_SpectatorFlag);
	}

	// update view pos for spectators
	if(m_Team == TEAM_SPECTATORS || m_Paused)
	{
		if (Flag && m_SpectatorID == GameServer()->GetFlagPlayer(m_SpectatorFlag))
			m_ViewPos = Flag->m_Pos;
		else if (m_SpectatorID != SPEC_FREEVIEW && GameServer()->m_apPlayers[m_SpectatorID] && GameServer()->m_apPlayers[m_SpectatorID]->GetCharacter())
			m_ViewPos = GameServer()->m_apPlayers[m_SpectatorID]->GetCharacter()->m_Pos;
	}
}

void CPlayer::PostPostTick()
{
	#ifdef CONF_DEBUG
		if(!g_Config.m_DbgDummies || m_ClientID < MAX_CLIENTS-g_Config.m_DbgDummies)
	#endif
		if(!Server()->ClientIngame(m_ClientID))
			return;

	if(!GameServer()->m_World.m_Paused && !m_pCharacter && m_Spawning && m_WeakHookSpawn)
		TryRespawn();
}

void CPlayer::Snap(int SnappingClient)
{
#ifdef CONF_DEBUG
	if(!g_Config.m_DbgDummies || m_ClientID < MAX_CLIENTS-g_Config.m_DbgDummies)
#endif
	if(!Server()->ClientIngame(m_ClientID))
		return;

	int id = m_ClientID;
	if (SnappingClient > -1 && !Server()->Translate(id, SnappingClient))
		return;

	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, id, sizeof(CNetObj_ClientInfo)));
	if(!pClientInfo)
		return;

	CPlayer *pSnapping = GameServer()->m_apPlayers[SnappingClient];


	/*************************************************
	*                                                *
	*              B L O C K D D R A C E             *
	*                                                *
	**************************************************/

	// for name and clan modifying
	const char *pName = Server()->ClientName(m_ClientID);
	const char *pClan = Server()->ClientClan(m_ClientID);

	// show name and clan by default
	m_ShowName = true;
	bool ShowClan = true;

	// hide name when spooky ghost is activated
	if (m_SpookyGhost)
		m_ShowName = false;

	// hide names and clans if in survival, and not in the lobby
	if (m_Minigame == MINIGAME_SURVIVAL && m_SurvivalState > SURVIVAL_LOBBY)
	{
		// hide clan aswell, clients can show clans over tees like names
		m_ShowName = false;
		ShowClan = false;
	}

	// always show name if...
	if (pSnapping->GetTeam() == TEAM_SPECTATORS || (m_Minigame != pSnapping->m_Minigame && !m_SpookyGhost))
		m_ShowName = true;

	// get the name
	if (!m_SetRealName && !m_ShowName)
		pName = " ";

	// get the clan
	if (m_SpookyGhost)
		pClan = Server()->ClientName(m_ClientID);
	else if (!ShowClan)
		pClan = "";

	// set info
	StrToInts(&pClientInfo->m_Clan0, 3, pClan);
	StrToInts(&pClientInfo->m_Name0, 4, pName);
	pClientInfo->m_Country = Server()->ClientCountry(m_ClientID);

	if (m_FlagPlayer != -1)
	{
		StrToInts(&pClientInfo->m_Skin0, 6, "default");
		pClientInfo->m_UseCustomColor = 1;
		pClientInfo->m_ColorBody = m_FlagPlayer == TEAM_RED ? 65387 : 10223467;
		pClientInfo->m_ColorFeet = m_FlagPlayer == TEAM_RED ? 65387 : 10223467;
	}
	else if ((m_pCharacter && m_pCharacter->m_Rainbow) || m_InfRainbow || IsHooked(RAINBOW))
	{
		// setting rainbow
		StrToInts(&pClientInfo->m_Skin0, 6, m_TeeInfos.m_SkinName);
		pClientInfo->m_UseCustomColor = 1;
		m_RainbowColor = (m_RainbowColor + m_RainbowSpeed) % 256;
		pClientInfo->m_ColorBody = m_RainbowColor * 0x010000 + 0xff00;
		pClientInfo->m_ColorFeet = m_RainbowColor * 0x010000 + 0xff00;
	}
	else
	{
		StrToInts(&pClientInfo->m_Skin0, 6, m_TeeInfos.m_SkinName);
		pClientInfo->m_UseCustomColor = m_TeeInfos.m_UseCustomColor;
		pClientInfo->m_ColorBody = m_TeeInfos.m_ColorBody;
		pClientInfo->m_ColorFeet = m_TeeInfos.m_ColorFeet;
	}

	CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, id, sizeof(CNetObj_PlayerInfo)));
	if(!pPlayerInfo)
		return;

	/*************************************************
	*                                                *
	*              B L O C K D D R A C E             *
	*                                                *
	**************************************************/

	m_SnapFixDDNet = false;
	if (m_ClientVersion >= VERSION_DDNET_OLD)
	{
		if (GameServer()->CountConnectedPlayers() > DDRACE_MAX_CLIENTS || m_ClientID > DDRACE_MAX_CLIENTS)
			m_SnapFixDDNet = true;
		else for (int i = 0; i < MAX_CLIENTS; i++)
			if (i >= DDRACE_MAX_CLIENTS && GameServer()->m_apPlayers[i])
				m_SnapFixDDNet = true;
	}

	// hide dummies from scoreboard and other players if they are in different minigames
	int Team = m_Team;
	if (Team != TEAM_SPECTATORS && (
		(g_Config.m_SvHideMinigamePlayers && pSnapping->m_Minigame != m_Minigame)
		|| (g_Config.m_SvHideDummies == 2 && m_IsDummy)
		))
		Team = TEAM_BLUE;

	// realistic ping for dummies
	if (m_IsDummy && g_Config.m_SvFakeDummyPing)
	{
		if (Server()->Tick() % 200 == 0)
			m_FakePing = 32 + rand() % 11;
		pPlayerInfo->m_Latency = m_FakePing;
	}
	else
		pPlayerInfo->m_Latency = SnappingClient == -1 ? m_Latency.m_Min : GameServer()->m_apPlayers[SnappingClient]->m_aActLatency[m_ClientID];

	pPlayerInfo->m_Local = (int)(m_ClientID == SnappingClient && (m_Paused != PAUSE_PAUSED || m_ClientVersion >= VERSION_DDNET_OLD));
	pPlayerInfo->m_ClientID = id;
	pPlayerInfo->m_Team = (m_SnapFixDDNet || m_ClientVersion < VERSION_DDNET_OLD || m_Paused != PAUSE_PAUSED || m_ClientID != SnappingClient) && m_Paused < PAUSE_SPEC ? Team : TEAM_SPECTATORS;

	if(m_ClientID == SnappingClient && m_Paused == PAUSE_PAUSED && (m_ClientVersion < VERSION_DDNET_OLD || m_SnapFixDDNet))
		pPlayerInfo->m_Team = TEAM_SPECTATORS;

	if (m_FlagPlayer != -1)
		pPlayerInfo->m_Team = TEAM_BLUE;

	// score
	int FlagPlayerScore = -1;
	int Score = 0;
	bool TimeFormat = false;

	// send 0 if times of others are not shown
	if(SnappingClient != m_ClientID && g_Config.m_SvHideScore)
	{
		FlagPlayerScore = -9999;
		Score = -9999;
		TimeFormat = true;
	}
	else if (pSnapping->m_Minigame == MINIGAME_BLOCK)
	{
		Score = GameServer()->m_Accounts[GetAccID()].m_Kills;
	}
	else if (pSnapping->m_Minigame == MINIGAME_SURVIVAL)
	{
		Score = GameServer()->m_Accounts[GetAccID()].m_SurvivalKills;
	}
	else if (pSnapping->m_Minigame == MINIGAME_INSTAGIB_BOOMFNG || pSnapping->m_Minigame == MINIGAME_INSTAGIB_FNG)
	{
		Score = m_InstagibScore;
	}
	else if (pSnapping->m_DisplayScore != SCORE_TIME)
	{
		if (pSnapping->m_DisplayScore == SCORE_LEVEL)
			Score = GameServer()->m_Accounts[GetAccID()].m_Level;
	}
	else
	{
		FlagPlayerScore = -9999;
		Score = abs(m_Score) * -1;
		TimeFormat = true;
	}
	if (!TimeFormat && GetAccID() < ACC_START)
		Score = 0;
	pPlayerInfo->m_Score = m_FlagPlayer != -1 ? FlagPlayerScore : Score;


	if (m_ClientID == SnappingClient && (m_Team == TEAM_SPECTATORS || m_Paused))
	{
		CNetObj_SpectatorInfo *pSpectatorInfo = static_cast<CNetObj_SpectatorInfo *>(Server()->SnapNewItem(NETOBJTYPE_SPECTATORINFO, m_ClientID, sizeof(CNetObj_SpectatorInfo)));
		if (!pSpectatorInfo)
			return;

		pSpectatorInfo->m_SpectatorID = m_SpectatorID;
		pSpectatorInfo->m_X = m_ViewPos.x;
		pSpectatorInfo->m_Y = m_ViewPos.y;
	}

	CNetObj_DDNetPlayer *pDDNetPlayer = static_cast<CNetObj_DDNetPlayer *>(Server()->SnapNewItem(NETOBJTYPE_DDNETPLAYER, id, sizeof(CNetObj_DDNetPlayer)));
	if (!pDDNetPlayer)
		return;

	pDDNetPlayer->m_AuthLevel = g_Config.m_SvAuthedPlayersColored ? Server()->GetAuthedState(id) : AUTHED_NO;
	pDDNetPlayer->m_Flags = m_Afk ? EXPLAYERFLAG_AFK : 0;
}

void CPlayer::FakeSnap()
{
	// This is problematic when it's sent before we know whether it's a non-64-player-client
	// Then we can't spectate players at the start

	// BlockDDrace
	if (m_ClientVersion >= VERSION_DDNET_OLD && !m_SnapFixDDNet)
		return;

	int FakeID = (m_SnapFixDDNet ? DDRACE_MAX_CLIENTS : VANILLA_MAX_CLIENTS) - 1;
	// BlockDDrace

	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, FakeID, sizeof(CNetObj_ClientInfo)));

	if(!pClientInfo)
		return;

	StrToInts(&pClientInfo->m_Name0, 4, " ");
	StrToInts(&pClientInfo->m_Clan0, 3, "");
	StrToInts(&pClientInfo->m_Skin0, 6, "default");

	if(m_Paused != PAUSE_PAUSED)
		return;

	CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, FakeID, sizeof(CNetObj_PlayerInfo)));
	if(!pPlayerInfo)
		return;

	pPlayerInfo->m_Latency = m_Latency.m_Min;
	pPlayerInfo->m_Local = 1;
	pPlayerInfo->m_ClientID = FakeID;
	pPlayerInfo->m_Score = -9999;
	pPlayerInfo->m_Team = TEAM_SPECTATORS;

	CNetObj_SpectatorInfo *pSpectatorInfo = static_cast<CNetObj_SpectatorInfo *>(Server()->SnapNewItem(NETOBJTYPE_SPECTATORINFO, FakeID, sizeof(CNetObj_SpectatorInfo)));
	if(!pSpectatorInfo)
		return;

	pSpectatorInfo->m_SpectatorID = m_SpectatorID;
	pSpectatorInfo->m_X = m_ViewPos.x;
	pSpectatorInfo->m_Y = m_ViewPos.y;
}

void CPlayer::OnDisconnect(const char *pReason)
{
	KillCharacter();

	if(Server()->ClientIngame(m_ClientID))
	{
		char aBuf[512];
		if(pReason && *pReason)
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game (%s)", Server()->ClientName(m_ClientID), pReason);
		else
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game", Server()->ClientName(m_ClientID));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);

		str_format(aBuf, sizeof(aBuf), "leave player='%d:%s'", m_ClientID, Server()->ClientName(m_ClientID));
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);

		bool WasModerator = m_Moderating;

		// Set this to false, otherwise PlayerModerating() will return true.
		m_Moderating = false;

		if (!GameServer()->PlayerModerating() && WasModerator)
			GameServer()->SendChat(-1, CGameContext::CHAT_ALL, "Server kick/spec votes are no longer actively moderated.");
	}

	CGameControllerBlockDDrace* Controller = (CGameControllerBlockDDrace*)GameServer()->m_pController;
	Controller->m_Teams.SetForceCharacterTeam(m_ClientID, 0);
}

void CPlayer::OnPredictedInput(CNetObj_PlayerInput *NewInput)
{
	// skip the input if chat is active
	if((m_PlayerFlags&PLAYERFLAG_CHATTING) && (NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING))
		return;

	AfkVoteTimer(NewInput);

	m_NumInputs++;

	if(m_pCharacter && !m_Paused)
		m_pCharacter->OnPredictedInput(NewInput);

	// Magic number when we can hope that client has successfully identified itself
	if(m_NumInputs == 20)
	{
		if(g_Config.m_SvClientSuggestion[0] != '\0' && m_ClientVersion <= VERSION_DDNET_OLD)
			GameServer()->SendBroadcast(g_Config.m_SvClientSuggestion, m_ClientID);
	}
}

void CPlayer::OnDirectInput(CNetObj_PlayerInput *NewInput)
{
	if (NewInput->m_PlayerFlags)
		Server()->SetClientFlags(m_ClientID, NewInput->m_PlayerFlags);

	if (AfkTimer(NewInput->m_TargetX, NewInput->m_TargetY))
		return; // we must return if kicked, as player struct is already deleted
	AfkVoteTimer(NewInput);

	if(((!m_pCharacter && m_Team == TEAM_SPECTATORS) || m_Paused) && m_SpectatorID == SPEC_FREEVIEW)
		m_ViewPos = vec2(NewInput->m_TargetX, NewInput->m_TargetY);

	if(NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING)
	{
	// skip the input if chat is active
		if(m_PlayerFlags&PLAYERFLAG_CHATTING)
			return;

		// reset input
		if(m_pCharacter)
			m_pCharacter->ResetInput();

		m_PlayerFlags = NewInput->m_PlayerFlags;
		return;
	}

	m_PlayerFlags = NewInput->m_PlayerFlags;

	if(m_pCharacter && m_Paused)
		m_pCharacter->ResetInput();

	if(!m_pCharacter && m_Team != TEAM_SPECTATORS && (NewInput->m_Fire&1))
		m_Spawning = true;

	// check for activity
	if(NewInput->m_Direction || m_LatestActivity.m_TargetX != NewInput->m_TargetX ||
		m_LatestActivity.m_TargetY != NewInput->m_TargetY || NewInput->m_Jump ||
		NewInput->m_Fire&1 || NewInput->m_Hook)
	{
		m_LatestActivity.m_TargetX = NewInput->m_TargetX;
		m_LatestActivity.m_TargetY = NewInput->m_TargetY;
		m_LastActionTick = Server()->Tick();
	}
}

void CPlayer::OnPredictedEarlyInput(CNetObj_PlayerInput *NewInput)
{
	// skip the input if chat is active
	if((m_PlayerFlags&PLAYERFLAG_CHATTING) && (NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING))
		return;

	if(m_pCharacter && !m_Paused)
		m_pCharacter->OnDirectInput(NewInput);
}

CCharacter *CPlayer::GetCharacter()
{
	if(m_pCharacter && m_pCharacter->IsAlive())
		return m_pCharacter;
	return 0;
}

void CPlayer::ThreadKillCharacter(int Weapon)
{
	m_KillMe = Weapon;
}

void CPlayer::KillCharacter(int Weapon)
{
	if(m_pCharacter)
	{
		if (m_RespawnTick > Server()->Tick())
			return;

		m_pCharacter->Die(m_ClientID, Weapon);

		delete m_pCharacter;
		m_pCharacter = 0;
	}
}

void CPlayer::Respawn(bool WeakHook)
{
	if(m_Team != TEAM_SPECTATORS)
	{
		m_WeakHookSpawn = WeakHook;
		m_Spawning = true;
	}
}

CCharacter* CPlayer::ForceSpawn(vec2 Pos)
{
	m_Spawning = false;
	m_pCharacter = new(m_ClientID) CCharacter(&GameServer()->m_World);
	m_pCharacter->Spawn(this, Pos);
	m_Team = TEAM_RED;
	return m_pCharacter;
}

void CPlayer::SetTeam(int Team, bool DoChatMsg)
{
	Team = GameServer()->m_pController->ClampTeam(Team);
	if(m_Team == Team)
		return;

	char aBuf[512];
	if(DoChatMsg)
	{
		str_format(aBuf, sizeof(aBuf), "'%s' joined the %s", Server()->ClientName(m_ClientID), GameServer()->m_pController->GetTeamName(Team));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
	}

	if(Team == TEAM_SPECTATORS)
	{
		CGameControllerBlockDDrace* Controller = (CGameControllerBlockDDrace*)GameServer()->m_pController;
		Controller->m_Teams.SetForceCharacterTeam(m_ClientID, 0);
	}

	KillCharacter();

	m_Team = Team;
	m_LastSetTeam = Server()->Tick();
	m_LastActionTick = Server()->Tick();
	m_SpectatorID = SPEC_FREEVIEW;
	m_RespawnTick = Server()->Tick();
	str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' m_Team=%d", m_ClientID, Server()->ClientName(m_ClientID), m_Team);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	if(Team == TEAM_SPECTATORS)
	{
		// update spectator modes
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_SpectatorID == m_ClientID)
				GameServer()->m_apPlayers[i]->m_SpectatorID = SPEC_FREEVIEW;
		}
	}
}

void CPlayer::TryRespawn()
{
	if (m_Team == TEAM_SPECTATORS || m_FlagPlayer != -1)
		return;

	vec2 SpawnPos;

	// BlockDDrace
	int Index = ENTITY_SPAWN;

	if (m_ForceSpawnPos != vec2(-1, -1))
	{
		SpawnPos = m_ForceSpawnPos;
	}
	else if (m_Dummymode == DUMMYMODE_SHOP_DUMMY)
	{
		if (GameServer()->Collision()->GetRandomTile(ENTITY_SHOP_DUMMY_SPAWN) != vec2(-1, -1))
			Index = ENTITY_SHOP_DUMMY_SPAWN;
		else
			Index = TILE_SHOP;
	}
	else if (m_Minigame == MINIGAME_BLOCK || m_Dummymode == DUMMYMODE_V3_BLOCKER)
	{
		Index = TILE_MINIGAME_BLOCK;
	}
	else if (m_Minigame == MINIGAME_SURVIVAL)
	{
		if (m_SurvivalState == SURVIVAL_LOBBY)
			Index = TILE_SURVIVAL_LOBBY;
		else if (m_SurvivalState == SURVIVAL_PLAYING)
			Index = TILE_SURVIVAL_SPAWN;
		else if (m_SurvivalState == SURVIVAL_DEATHMATCH)
			Index = TILE_SURVIVAL_DEATHMATCH;
	}
	else if (m_Minigame == MINIGAME_INSTAGIB_BOOMFNG)
	{
		Index = ENTITY_SPAWN_RED;
	}
	else if (m_Minigame == MINIGAME_INSTAGIB_FNG)
	{
		Index = ENTITY_SPAWN_BLUE;
	}

	if (GameServer()->Collision()->GetRandomTile(Index) == vec2(-1, -1))
		Index = ENTITY_SPAWN;

	if (m_ForceSpawnPos == vec2(-1, -1) && !GameServer()->m_pController->CanSpawn(&SpawnPos, Index))
		return;

	CGameControllerBlockDDrace* Controller = (CGameControllerBlockDDrace*)GameServer()->m_pController;

	m_WeakHookSpawn = false;
	m_Spawning = false;
	m_pCharacter = new(m_ClientID) CCharacter(&GameServer()->m_World);
	m_pCharacter->Spawn(this, SpawnPos);
	GameServer()->CreatePlayerSpawn(SpawnPos, m_pCharacter->Teams()->TeamMask(m_pCharacter->Team(), -1, m_ClientID));

	if(g_Config.m_SvTeam == 3)
	{
		int NewTeam = 0;
		for(; NewTeam < TEAM_SUPER; NewTeam++)
			if(Controller->m_Teams.Count(NewTeam) == 0)
				break;

		if(NewTeam == TEAM_SUPER)
			NewTeam = 0;

		Controller->m_Teams.SetForceCharacterTeam(GetCID(), NewTeam);
		m_pCharacter->SetSolo(true);
	}
}

bool CPlayer::AfkTimer(int NewTargetX, int NewTargetY)
{
	/*
		afk timer (x, y = mouse coordinates)
		Since a player has to move the mouse to play, this is a better method than checking
		the player's position in the game world, because it can easily be bypassed by just locking a key.
		Frozen players could be kicked as well, because they can't move.
		It also works for spectators.
		returns true if kicked
	*/

	if(Server()->GetAuthedState(m_ClientID))
		return false; // don't kick admins
	if(g_Config.m_SvMaxAfkTime == 0)
		return false; // 0 = disabled

	if(NewTargetX != m_LastTarget_x || NewTargetY != m_LastTarget_y)
	{
		m_LastPlaytime = time_get();
		m_LastTarget_x = NewTargetX;
		m_LastTarget_y = NewTargetY;
		m_Sent1stAfkWarning = 0; // afk timer's 1st warning after 50% of sv_max_afk_time
		m_Sent2ndAfkWarning = 0;

	}
	else
	{
		if(!m_Paused)
		{
			// not playing, check how long
			if(m_Sent1stAfkWarning == 0 && m_LastPlaytime < time_get()-time_freq()*(int)(g_Config.m_SvMaxAfkTime*0.5))
			{
				str_format(m_pAfkMsg, sizeof(m_pAfkMsg),
					"You have been afk for %d seconds now. Please note that you get kicked after not playing for %d seconds.",
					(int)(g_Config.m_SvMaxAfkTime*0.5),
					g_Config.m_SvMaxAfkTime
				);
				m_pGameServer->SendChatTarget(m_ClientID, m_pAfkMsg);
				m_Sent1stAfkWarning = 1;
			}
			else if(m_Sent2ndAfkWarning == 0 && m_LastPlaytime < time_get()-time_freq()*(int)(g_Config.m_SvMaxAfkTime*0.9))
			{
				str_format(m_pAfkMsg, sizeof(m_pAfkMsg),
					"You have been afk for %d seconds now. Please note that you get kicked after not playing for %d seconds.",
					(int)(g_Config.m_SvMaxAfkTime*0.9),
					g_Config.m_SvMaxAfkTime
				);
				m_pGameServer->SendChatTarget(m_ClientID, m_pAfkMsg);
				m_Sent2ndAfkWarning = 1;
			}
			else if(m_LastPlaytime < time_get()-time_freq()*g_Config.m_SvMaxAfkTime)
			{
				m_pGameServer->Server()->Kick(m_ClientID, "Away from keyboard");
				return true;
			}
		}
	}
	return false;
}

void CPlayer::AfkVoteTimer(CNetObj_PlayerInput *NewTarget)
{
	if(g_Config.m_SvMaxAfkVoteTime == 0)
		return;

	if(mem_comp(NewTarget, &m_LastTarget, sizeof(CNetObj_PlayerInput)) != 0)
	{
		m_LastPlaytime = time_get();
		mem_copy(&m_LastTarget, NewTarget, sizeof(CNetObj_PlayerInput));
	}
	else if(m_LastPlaytime < time_get()-time_freq()*g_Config.m_SvMaxAfkVoteTime)
	{
		m_Afk = true;
		return;
	}

	m_Afk = false;
}

void CPlayer::ProcessPause()
{
	if(m_ForcePauseTime && m_ForcePauseTime < Server()->Tick())
	{
		m_ForcePauseTime = 0;
		Pause(PAUSE_NONE, true);
	}

	if(m_Paused == PAUSE_SPEC && !m_pCharacter->IsPaused() && m_pCharacter->IsGrounded() && m_pCharacter->m_Pos == m_pCharacter->m_PrevPos)
	{
		m_pCharacter->Pause(true);
		GameServer()->CreateDeath(m_pCharacter->m_Pos, m_ClientID, m_pCharacter->Teams()->TeamMask(m_pCharacter->Team(), -1, m_ClientID));
		GameServer()->CreateSound(m_pCharacter->m_Pos, SOUND_PLAYER_DIE, m_pCharacter->Teams()->TeamMask(m_pCharacter->Team(), -1, m_ClientID));
	}
}

int CPlayer::Pause(int State, bool Force)
{
	if(State < PAUSE_NONE || State > PAUSE_SPEC) // Invalid pause state passed
		return 0;

	if(!m_pCharacter)
		return 0;

	char aBuf[128];
	if(State != m_Paused)
	{
		// Get to wanted state
		switch(State){
		case PAUSE_PAUSED:
		case PAUSE_NONE:
			if(m_pCharacter->IsPaused()) // First condition might be unnecessary
			{
				if(!Force && m_LastPause && m_LastPause + g_Config.m_SvSpecFrequency * Server()->TickSpeed() > Server()->Tick())
				{
					GameServer()->SendChatTarget(m_ClientID, "Can't /spec that quickly.");
					return m_Paused; // Do not update state. Do not collect $200
				}
				m_pCharacter->Pause(false);
				GameServer()->CreatePlayerSpawn(m_pCharacter->m_Pos, m_pCharacter->Teams()->TeamMask(m_pCharacter->Team(), -1, m_ClientID));
			}
			// fall-thru
		case PAUSE_SPEC:
			if(g_Config.m_SvPauseMessages)
			{
				str_format(aBuf, sizeof(aBuf), (State > PAUSE_NONE) ? "'%s' speced" : "'%s' resumed", Server()->ClientName(m_ClientID));
				GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
			}
			break;
		}

		// Update state
		m_Paused = State;
		m_LastPause = Server()->Tick();
	}

	return m_Paused;
}

int CPlayer::ForcePause(int Time)
{
	m_ForcePauseTime = Server()->Tick() + Server()->TickSpeed() * Time;

	if(g_Config.m_SvPauseMessages)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "'%s' was force-paused for %ds", Server()->ClientName(m_ClientID), Time);
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
	}

	return Pause(PAUSE_SPEC, true);
}

int CPlayer::IsPaused()
{
	return m_ForcePauseTime ? m_ForcePauseTime : -1 * m_Paused;
}

bool CPlayer::IsPlaying()
{
	if(m_pCharacter && m_pCharacter->IsAlive())
		return true;
	return false;
}

void CPlayer::SpectatePlayerName(const char *pName)
{
	if(!pName)
		return;

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(i != m_ClientID && Server()->ClientIngame(i) && !str_comp(pName, Server()->ClientName(i)))
		{
			m_SpectatorID = i;
			return;
		}
	}
}


/*************************************************
*                                                *
*              B L O C K D D R A C E             *
*                                                *
**************************************************/

void CPlayer::FixForNoName(int ID)
{
	m_FixNameID = ID;
	m_SetRealName = true;
	m_SetRealNameTick = Server()->Tick() + Server()->TickSpeed() / 20;
}

int CPlayer::GetAccID()
{
	for (unsigned int i = ACC_START; i < GameServer()->m_Accounts.size(); i++)
		if (GameServer()->m_Accounts[i].m_ClientID == m_ClientID)
			return i;
	return 0;
}

void CPlayer::CheckLevel()
{
	if (GetAccID() < ACC_START)
		return;

	CGameContext::AccountInfo Account = GameServer()->m_Accounts[GetAccID()];

	// TODO: make something here, add acc sys and stuff, this is just placeholder
	Account.m_NeededXP = 1;

	if (Account.m_XP >= Account.m_NeededXP)
	{
		Account.m_Level++;

		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "You are now Level %d!", Account.m_Level);
		GameServer()->SendChatTarget(m_ClientID, aBuf);

		Account.m_NeededXP += 2;

		dbg_msg("acc", "Level: %d, NeededXP: %d", Account.m_Level, Account.m_NeededXP);
	}
}

void CPlayer::MoneyTransaction(int Amount, const char *Description)
{
	if (GetAccID() < ACC_START)
		return;

	CGameContext::AccountInfo Account = GameServer()->m_Accounts[GetAccID()];

	Account.m_Money += Amount;

	str_format(Account.m_aLastMoneyTransaction[4], sizeof(Account.m_aLastMoneyTransaction[4]), "%s", Account.m_aLastMoneyTransaction[3]);
	str_format(Account.m_aLastMoneyTransaction[3], sizeof(Account.m_aLastMoneyTransaction[3]), "%s", Account.m_aLastMoneyTransaction[2]);
	str_format(Account.m_aLastMoneyTransaction[2], sizeof(Account.m_aLastMoneyTransaction[2]), "%s", Account.m_aLastMoneyTransaction[1]);
	str_format(Account.m_aLastMoneyTransaction[1], sizeof(Account.m_aLastMoneyTransaction[1]), "%s", Account.m_aLastMoneyTransaction[0]);
	str_format(Account.m_aLastMoneyTransaction[0], sizeof(Account.m_aLastMoneyTransaction[0]), "%s", Description);
}

bool CPlayer::IsHooked(int Power)
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		CCharacter* pChr = GameServer()->GetPlayerChar(i);
		if (!pChr)
			continue;

		if (pChr->Core()->m_HookedPlayer == m_ClientID)
		{
			if (Power == -2 && m_pCharacter)
				m_pCharacter->Core()->m_Killer.m_ClientID = i;
			return Power >= 0 ? pChr->m_HookPower == Power : true;
		}
	}
	return false;
}

bool CPlayer::IsSpectator()
{
	return m_Paused != PAUSE_NONE || m_Team == TEAM_SPECTATORS || (m_pCharacter && m_pCharacter->IsPaused());
}

void CPlayer::SetPlaying()
{
	m_SpectatorID = SPEC_FREEVIEW;
	Pause(PAUSE_NONE, true);
	SetTeam(TEAM_RED);
	if (m_pCharacter && m_pCharacter->IsPaused())
		m_pCharacter->Pause(false);
}
