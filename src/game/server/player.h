/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_PLAYER_H
#define GAME_SERVER_PLAYER_H

// this include should perhaps be removed
#include "entities/character.h"
#include "gamecontext.h"


/*************************************************
*                                                *
*              B L O C K D D R A C E             *
*                                                *
**************************************************/

#include "entities/pickup_drop.h"
#include <vector>

enum NoNameFix
{
	FIX_SET_NAME_ONLY = 0,
	FIX_CHAT_MSG,
	FIX_KILL_MSG
};

enum Gamemode
{
	GAMEMODE_DDRACE = 0,
	GAMEMODE_VANILLA
};

enum Scoreformat
{
	SCORE_TIME = 0,
	SCORE_LEVEL
};

enum Dummymode
{
	DUMMYMODE_IDLE = 0,
	DUMMYMODE_V3_BLOCKER = -6,
	DUMMYMODE_CHILLBLOCK5_RACER = 23,
	DUMMYMODE_CHILLBLOCK5_BLOCKER = 29,
	DUMMYMODE_CHILLBOCK5_POLICE = 31,
	DUMMYMODE_BLMAPCHILL_POLICE = 32,
	DUMMYMODE_SHOP_BOT = 99,
};

/*************************************************
*                                                *
*              B L O C K D D R A C E             *
*                                                *
**************************************************/


// player object
class CPlayer
{
	MACRO_ALLOC_POOL_ID()

	friend class CSaveTee;

public:
	CPlayer(CGameContext *pGameServer, int ClientID, int Team);
	~CPlayer();

	void Reset();

	void TryRespawn();
	void Respawn(bool WeakHook = false); // with WeakHook == true the character will be spawned after all calls of Tick from other Players
	CCharacter* ForceSpawn(vec2 Pos); // required for loading savegames
	void SetTeam(int Team, bool DoChatMsg=true);
	int GetTeam() const { return m_Team; };
	int GetCID() const { return m_ClientID; };

	void Tick();
	void PostTick();

	// will be called after all Tick and PostTick calls from other players
	void PostPostTick();
	void Snap(int SnappingClient);
	void FakeSnap();

	void OnDirectInput(CNetObj_PlayerInput *NewInput);
	void OnPredictedInput(CNetObj_PlayerInput *NewInput);
	void OnPredictedEarlyInput(CNetObj_PlayerInput *NewInput);
	void OnDisconnect(const char *pReason);

	void ThreadKillCharacter(int Weapon = WEAPON_GAME);
	void KillCharacter(int Weapon = WEAPON_GAME);
	CCharacter *GetCharacter();

	void SpectatePlayerName(const char *pName);

	//---------------------------------------------------------
	// this is used for snapping so we know how we can clip the view for the player
	vec2 m_ViewPos;
	int m_TuneZone;
	int m_TuneZoneOld;

	// states if the client is chatting, accessing a menu etc.
	int m_PlayerFlags;

	// used for snapping to just update latency if the scoreboard is active
	int m_aActLatency[MAX_CLIENTS];

	// used for spectator mode
	int m_SpectatorID;

	bool m_IsReady;

	//
	int m_Vote;
	int m_VotePos;
	//
	int m_LastVoteCall;
	int m_LastVoteTry;
	int m_LastChat;
	int m_LastSetTeam;
	int m_LastSetSpectatorMode;
	int m_LastChangeInfo;
	int m_LastEmote;
	int m_LastKill;
	int m_LastCommands[4];
	int m_LastCommandPos;
	int m_LastWhisperTo;
	int m_LastInvited;

	int m_SendVoteIndex;

	// TODO: clean this up
	struct
	{
		char m_SkinName[64];
		int m_UseCustomColor;
		int m_ColorBody;
		int m_ColorFeet;
	} m_TeeInfos;

	// BlockDDrace
	int m_RespawnTick;
	// BlockDDrace

	int m_DieTick;
	int m_Score;
	int m_JoinTick;
	bool m_ForceBalanced;
	int m_LastActionTick;
	int m_TeamChangeTick;
	bool m_SentSemicolonTip;
	struct
	{
		int m_TargetX;
		int m_TargetY;
	} m_LatestActivity;

	// network latency calculations
	struct
	{
		int m_Accum;
		int m_AccumMin;
		int m_AccumMax;
		int m_Avg;
		int m_Min;
		int m_Max;
	} m_Latency;

private:
	CCharacter *m_pCharacter;
	int m_NumInputs;
	CGameContext *m_pGameServer;

	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const;

	//
	bool m_Spawning;
	bool m_WeakHookSpawn;
	int m_ClientID;
	int m_Team;

	int m_Paused;
	int64 m_ForcePauseTime;
	int64 m_LastPause;

public:
	enum
	{
		PAUSE_NONE=0,
		PAUSE_PAUSED,
		PAUSE_SPEC
	};
	
	enum
	{
		TIMERTYPE_GAMETIMER=0,
		TIMERTYPE_BROADCAST,
		TIMERTYPE_GAMETIMER_AND_BROADCAST,
		TIMERTYPE_NONE,
	};

	bool m_DND;
	int64 m_FirstVoteTick;
	char m_TimeoutCode[64];

	void ProcessPause();
	int Pause(int State, bool Force);
	int ForcePause(int Time);
	int IsPaused();

	bool IsPlaying();
	int64 m_Last_KickVote;
	int64 m_Last_Team;
	int m_ClientVersion;
	bool m_ShowOthers;
	bool m_ShowAll;
	bool m_SpecTeam;
	bool m_NinjaJetpack;
	bool m_Afk;
	int m_KillMe;
	bool m_HasFinishScore;
	int m_ChatScore;
	bool m_Moderating;

	bool AfkTimer(int new_target_x, int new_target_y); //returns true if kicked
	void AfkVoteTimer(CNetObj_PlayerInput *NewTarget);
	int64 m_LastPlaytime;
	int64 m_LastEyeEmote;
	int64 m_LastBroadcast;
	bool m_LastBroadcastImportance;
	int m_LastTarget_x;
	int m_LastTarget_y;
	CNetObj_PlayerInput m_LastTarget;
	int m_Sent1stAfkWarning; // afk timer's 1st warning after 50% of sv_max_afk_time
	int m_Sent2ndAfkWarning; // afk timer's 2nd warning after 90% of sv_max_afk_time
	char m_pAfkMsg[160];
	bool m_EyeEmote;
	int m_TimerType;
	int m_DefEmote;
	int m_DefEmoteReset;
	bool m_Halloween;
	bool m_FirstPacket;
#if defined(CONF_SQL)
	int64 m_LastSQLQuery;
#endif
	bool m_NotEligibleForFinish;
	int64 m_EligibleForFinishCheck;


	/*************************************************
	*                                                *
	*              B L O C K D D R A C E             *
	*                                                *
	**************************************************/

	//weapon drops
	std::vector< std::vector<CPickupDrop*> > m_vWeaponLimit;

	//dummy
	bool m_IsDummy;
	int m_Dummymode;
	int m_FakePing;
	vec2 m_ForceSpawnPos;

	//snap fix for 256p support
	bool m_SnapFixDDNet;

	//gamemodes
	int m_Gamemode;

	//spooky ghost
	bool m_SpookyGhost;
	int m_RealUseCustomColor;
	char m_RealSkinName[64];

	//no name fix
	void FixForNoName(int ID);
	int m_FixNameID;
	bool m_ShowName;
	bool m_SetRealName;
	int64 m_SetRealNameTick;
	int m_ChatTeam;
	char m_ChatText[256];
	int m_MsgKiller;
	int m_MsgWeapon;
	int m_MsgModeSpecial;

	//extras
	int m_RainbowSpeed;
	int m_RainbowColor;

	bool m_InfRainbow;
	int m_InfMeteors;
	bool m_HasSpookyGhost;

	//account
	int GetAccID();
	void CheckLevel();
	void MoneyTransaction(int Amount, const char *Description);

	//score
	int m_DisplayScore;
	int m_InstagibScore;

	//weapon indicator
	bool m_WeaponIndicator;

	//others
	bool IsHooked(int Power = -1);
	bool IsSpectator();
	void SetPlaying();
	bool m_ResumeMoved;

	// used for spectating flags
	int m_SpectatorFlag;
	int m_FlagPlayer;

	//minigames
	int m_Minigame;
	int m_SurvivalState;
	bool m_ForceKilled;
};

#endif
