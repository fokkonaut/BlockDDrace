/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_CHARACTER_H
#define GAME_SERVER_ENTITIES_CHARACTER_H

#include <deque>
#include <game/server/entity.h>
#include <game/server/entities/stable_projectile.h>
#include <game/generated/server_data.h>
#include <game/generated/protocol.h>

#include <game/gamecore.h>

class CGameTeams;

enum
{
	WEAPON_GAME = -3, // team switching etc
	WEAPON_SELF = -2, // console kill command
	WEAPON_WORLD = -1, // death tiles etc
};

enum
{
	FAKETUNE_FREEZE = 1,
	FAKETUNE_SOLO = 2,
	FAKETUNE_NOJUMP = 4,
	FAKETUNE_NOCOLL = 8,
	FAKETUNE_NOHOOK = 16,
	FAKETUNE_JETPACK = 32,
	FAKETUNE_NOHAMMER = 64,
};


/*************************************************
*                                                *
*              B L O C K D D R A C E             *
*                                                *
**************************************************/

enum Extra
{
	HOOK_NORMAL = 0,
	JETPACK,
	RAINBOW,
	INF_RAINBOW,
	ATOM,
	TRAIL,
	EXTRA_SPOOKY_GHOST,
	METEOR,
	INF_METEOR,
	PASSIVE,
	VANILLA_MODE,
	DDRACE_MODE,
	BLOODY,
	STRONG_BLOODY,
	POLICE_HELPER,
	SCROLL_NINJA,
	HOOK_POWER,
	ENDLESS_HOOK,
	INFINITE_JUMPS,
	SPREAD_WEAPON,
	FREEZE_HAMMER,
	INVISIBLE,
	TELEKINESIS,
	NUM_EXTRAS
};

enum Backup
{
	BACKUP_FREEZE = 0,
	BACKUP_SPOOKY_GHOST,
	NUM_BACKUPS,

	BACKUP_HEALTH = NUM_WEAPONS,
	BACKUP_ARMOR,
	NUM_WEAPONS_BACKUP
};

/*************************************************
*                                                *
*              B L O C K D D R A C E             *
*                                                *
**************************************************/


class CCharacter : public CEntity
{
	MACRO_ALLOC_POOL_ID()

	friend class CSaveTee; // need to use core

public:
	//character's size
	static const int ms_PhysSize = 28;

	CCharacter(CGameWorld *pWorld);

	virtual void Reset();
	virtual void Destroy();
	virtual void Tick();
	virtual void TickDefered();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);
	virtual int NetworkClipped(int SnappingClient);
	virtual int NetworkClipped(int SnappingClient, vec2 CheckPos);

	bool IsGrounded();

	void SetWeapon(int W);
	void SetSolo(bool Solo);
	void HandleWeaponSwitch();
	void DoWeaponSwitch();

	void HandleWeapons();
	void HandleNinja();
	void HandleJetpack();

	void OnPredictedInput(CNetObj_PlayerInput *pNewInput);
	void OnDirectInput(CNetObj_PlayerInput *pNewInput);
	void ResetInput();
	void FireWeapon();

	void Die(int Killer, int Weapon);
	bool TakeDamage(vec2 Force, int Dmg, int From, int Weapon);

	bool Spawn(class CPlayer *pPlayer, vec2 Pos);
	bool Remove();

	bool IncreaseHealth(int Amount);
	bool IncreaseArmor(int Amount);

	void GiveWeapon(int Weapon, bool Remove = false, int Ammo = -1);
	void GiveNinja();
	void RemoveNinja();

	void SetEmote(int Emote, int Tick);

	void Rescue();

	int NeededFaketuning() {return m_NeededFaketuning;}
	bool IsAlive() const { return m_Alive; }
	bool IsPaused() const { return m_Paused; }
	class CPlayer *GetPlayer() { return m_pPlayer; }


	/*************************************************
	*                                                *
	*              B L O C K D D R A C E             *
	*                                                *
	**************************************************/

	//drops
	void DropWeapon(int WeaponID, float Dir = -3, bool Forced = false);
	void DropPickup(int Type, int Amount = 1);
	void DropLoot();

	void SetAvailableWeapon(int PreferedWeapon = WEAPON_GUN);
	int GetAimDir() { return m_Input.m_TargetX < 0 ? -1 : 1; };

	void PassiveCollision(bool Set);

	void Jetpack(bool Set = true, int FromID = -1, bool Silent = false);
	void Rainbow(bool Set = true, int FromID = -1, bool Silent = false);
	void InfRainbow(bool Set = true, int FromID = -1, bool Silent = false);
	void Atom(bool Set = true, int FromID = -1, bool Silent = false);
	void Trail(bool Set = true, int FromID = -1, bool Silent = false);
	void SpookyGhost(bool Set = true, int FromID = -1, bool Silent = false);
	void Meteor(bool Set = true, int FromID = -1, bool Infinite = false, bool Silent = false);
	void Passive(bool Set = true, int FromID = -1, bool Silent = false);
	void VanillaMode(int FromID = -1, bool Silent = false);
	void DDraceMode(int FromID = -1, bool Silent = false);
	void Bloody(bool Set = true, int FromID = -1, bool Silent = false);
	void StrongBloody(bool Set = true, int FromID = -1, bool Silent = false);
	void PoliceHelper(bool Set = true, int FromID = -1, bool Silent = false);
	void ScrollNinja(bool Set = true, int FromID = -1, bool Silent = false);
	void HookPower(int Extra, int FromID = -1, bool Silent = false);
	void EndlessHook(bool Set = true, int FromID = -1, bool Silent = false);
	void InfiniteJumps(bool Set = true, int FromID = -1, bool Silent = false);
	void SpreadWeapon(int Type, bool Set = true, int FromID = -1, bool Silent = false);
	void FreezeHammer(bool Set = true, int FromID = -1, bool Silent = false);
	void Invisible(bool Set = true, int FromID = -1, bool Silent = false);
	void Telekinesis(bool Set = true, int FromID = -1, bool Silent = false);

	/*************************************************
	*                                                *
	*              B L O C K D D R A C E             *
	*                                                *
	**************************************************/


private:
	// player controlling this character
	class CPlayer *m_pPlayer;

	bool m_Alive;
	bool m_Paused;
	int m_NeededFaketuning;

	// weapon info
	CEntity *m_apHitObjects[10];
	int m_NumObjectsHit;

	struct WeaponStat
	{
		int m_AmmoRegenStart;
		int m_Ammo;
		int m_Ammocost;
		bool m_Got;

	} m_aWeapons[NUM_WEAPONS];

	// BlockDDrace
	int m_ActiveWeapon;
	// BlockDDrace

	int m_LastWeapon;
	int m_QueuedWeapon;

	int m_ReloadTimer;
	int m_AttackTick;

	int m_DamageTaken;

	int m_EmoteType;
	int m_EmoteStop;

	// last tick that the player took any action ie some input
	int m_LastAction;
	int m_LastNoAmmoSound;

	// these are non-heldback inputs
	CNetObj_PlayerInput m_LatestPrevInput;
	CNetObj_PlayerInput m_LatestInput;

	// input
	CNetObj_PlayerInput m_PrevInput;
	CNetObj_PlayerInput m_Input;
	CNetObj_PlayerInput m_SavedInput;
	int m_NumInputs;
	int m_Jumped;

	int m_DamageTakenTick;

	int m_Health;
	int m_Armor;

	// ninja
	struct
	{
		vec2 m_ActivationDir;
		int m_ActivationTick;
		int m_CurrentMoveTime;
		int m_OldVelAmount;
	} m_Ninja;

	// the player core for the physics
	CCharacterCore m_Core;

	// info for dead reckoning
	int m_ReckoningTick; // tick that we are performing dead reckoning From
	CCharacterCore m_SendCore; // core that we should send
	CCharacterCore m_ReckoningCore; // the dead reckoning core

	// DDRace


	void HandleTiles(int Index);
	float m_Time;
	int m_LastBroadcast;
	void DDRaceInit();
	void HandleSkippableTiles(int Index);
	void DDRaceTick();
	void DDRacePostCoreTick();
	void HandleBroadcast();
	void HandleTuneLayer();
	void SendZoneMsgs();

	bool m_SetSavePos;
	vec2 m_PrevSavePos;
	bool m_Solo;

	// BlockDDrace
	void BlockDDraceTick();
	void DummyTick();
	void BlockDDraceInit();

public:
	CGameTeams* Teams();
	void Pause(bool Pause);
	bool Freeze(int Time);
	bool Freeze();
	bool UnFreeze();
	void GiveAllWeapons();
	int m_DDRaceState;
	int Team();
	bool CanCollide(int ClientID, bool CheckPassive = true);
	bool SameTeam(int ClientID);
	bool m_Super;
	bool m_SuperJump;
	bool m_Jetpack;
	bool m_NinjaJetpack;
	int m_TeamBeforeSuper;
	int m_FreezeTime;
	int64 m_FirstFreezeTick;
	int m_FreezeTick;
	bool m_DeepFreeze;
	bool m_EndlessHook;
	bool m_FreezeHammer;
	enum
	{
		HIT_ALL=0,
		DISABLE_HIT_HAMMER=1,
		DISABLE_HIT_SHOTGUN=2,
		DISABLE_HIT_GRENADE=4,
		DISABLE_HIT_RIFLE=8
	};
	int m_Hit;
	int m_TuneZone;
	int m_TuneZoneOld;
	int m_PainSoundTimer;
	int m_LastMove;
	int m_StartTime;
	vec2 m_PrevPos;
	int m_TeleCheckpoint;
	int m_CpTick;
	int m_CpActive;
	int m_CpLastBroadcast;
	float m_CpCurrent[25];
	int m_TileIndex;
	int m_TileFlags;
	int m_TileFIndex;
	int m_TileFFlags;
	int m_TileSIndex;
	int m_TileSFlags;
	int m_TileIndexL;
	int m_TileFlagsL;
	int m_TileFIndexL;
	int m_TileFFlagsL;
	int m_TileSIndexL;
	int m_TileSFlagsL;
	int m_TileIndexR;
	int m_TileFlagsR;
	int m_TileFIndexR;
	int m_TileFFlagsR;
	int m_TileSIndexR;
	int m_TileSFlagsR;
	int m_TileIndexT;
	int m_TileFlagsT;
	int m_TileFIndexT;
	int m_TileFFlagsT;
	int m_TileSIndexT;
	int m_TileSFlagsT;
	int m_TileIndexB;
	int m_TileFlagsB;
	int m_TileFIndexB;
	int m_TileFFlagsB;
	int m_TileSIndexB;
	int m_TileSFlagsB;
	vec2 m_Intersection;
	int64 m_LastStartWarning;
	int64 m_LastRescue;
	bool m_LastRefillJumps;
	bool m_LastPenalty;
	bool m_LastBonus;
	bool m_HasTeleGun;
	bool m_HasTeleGrenade;
	bool m_HasTeleLaser;
	vec2 m_TeleGunPos;
	bool m_TeleGunTeleport;
	bool m_IsBlueTeleGunTeleport;
	int m_StrongWeakID;

	// Setters/Getters because i don't want to modify vanilla vars access modifiers
	int GetLastWeapon() { return m_LastWeapon; };
	void SetLastWeapon(int LastWeap) {m_LastWeapon = LastWeap; };
	int GetActiveWeapon() { return m_ActiveWeapon; };
	void SetActiveWeapon(int Weapon);
	void SetLastAction(int LastAction) {m_LastAction = LastAction; };
	int GetArmor() { return m_Armor; };
	void SetArmor(int Armor) {m_Armor = Armor; };
	CCharacterCore GetCore() { return m_Core; };
	void SetCore(CCharacterCore Core) {m_Core = Core; };
	CCharacterCore* Core() { return &m_Core; };
	bool GetWeaponGot(int Type) { return m_aWeapons[Type].m_Got; };
	void SetWeaponGot(int Type, bool Value) { m_aWeapons[Type].m_Got = Value; };
	int GetWeaponAmmo(int Type);
	void SetWeaponAmmo(int Type, int Value) { m_aWeapons[Type].m_Ammo = Value; };
	bool IsAlive() { return m_Alive; };
	void SetEmoteType(int EmoteType) { m_EmoteType = EmoteType; };
	void SetEmoteStop(int EmoteStop) { m_EmoteStop = EmoteStop; };
	void SetNinjaActivationDir(vec2 ActivationDir) { m_Ninja.m_ActivationDir = ActivationDir; };
	void SetNinjaActivationTick(int ActivationTick) { m_Ninja.m_ActivationTick = ActivationTick; };
	void SetNinjaCurrentMoveTime(int CurrentMoveTime) { m_Ninja.m_CurrentMoveTime = CurrentMoveTime; };


	/*************************************************
	*                                                *
	*              B L O C K D D R A C E             *
	*                                                *
	**************************************************/

	//last tile
	int m_LastIndexTile;
	int m_LastIndexFrontTile;

	//last toucher
	int m_LastHitWeapon;
	int m_LastTouchedTee;
	int m_OldLastHookedPlayer;

	//backups
	void BackupWeapons(int Type);
	void LoadWeaponBackup(int Type);
	int m_aWeaponsBackup[NUM_WEAPONS_BACKUP][NUM_BACKUPS];
	bool m_WeaponsBackupped[NUM_BACKUPS];
	bool m_aWeaponsBackupGot[NUM_WEAPONS_BACKUP][NUM_BACKUPS];

	//spooky ghost
	void SetSpookyGhost();
	void UnsetSpookyGhost();
	void SaveRealInfos();
	int m_NumGhostShots;

	//extras
	bool m_Rainbow;
	bool m_Atom;
	bool m_Trail;
	int m_Meteors;
	bool m_Bloody;
	bool m_StrongBloody;
	bool m_Invisible;
	bool m_ScrollNinja;
	int m_HookPower;
	bool m_aSpreadWeapon[NUM_WEAPONS];
	int m_TelekinesisTee;

	bool m_Passive;
	bool m_PoliceHelper;

	bool m_AtomHooked;
	bool m_TrailHooked;

	// shop
	bool m_InShop;
	bool m_EnteredShop;

	int64 m_ShopBotAntiSpamTick;

	void ShopWindow(int Dir);
	int m_ShopWindowPage;
	int64 m_ShopMotdTick;
	void BuyItem(int ItemID);
	void ConfirmPurchase();
	int m_PurchaseState;
	void PurchaseEnd(bool canceled);

	//weapon indicator
	void UpdateWeaponIndicator();

	//others
	int HasFlag();
	void CheckMoved();

	//this means the character is directly on a freezetile
	bool m_IsFrozen;


	/////////dummymode variables

	void Fire(bool Fire = true);

	//dummymode 32 vars (BlmapChill police guard)
	int m_DummyLovedX;
	int m_DummyLovedY;
	int m_DummyLowerPanic;
	int m_DummySpeed;
	int m_DummyHelpMode;
	bool m_DummyHelpHook;
	bool m_DummyClosestPolice;

	int m_DummyGrenadeJump;
	bool m_DummyTouchedGround;
	bool m_DummyAlreadyBeenHere;
	bool m_DummyStartGrenade;
	bool m_DummyUsedDJ;
	int m_DummySpawnTeleporter;
	bool m_DummyReachedCinemaEntrance;


	//dummymode 29 vars (ChillBlock5 blocker)
	int m_DummyFreezeBlockTrick;
	int m_DummyPanicDelay;
	bool m_DummyStartHook;
	bool m_DummySpeedRight;
	bool m_DummyTrick3Check;
	bool m_DummyTrick3Panic;
	bool m_DummyTrick3StartCount;
	bool m_DummyTrick3PanicLeft;
	bool m_DummyTrick4HasStartPos;
	bool m_DummyLockBored;
	bool m_DummyDoBalance;
	bool m_DummyAttackedOnSpawn;
	bool m_DummyMovementToBlockArea;
	bool m_DummyPlannedMovement;
	bool m_DummyJumped;
	bool m_DummyHooked;
	bool m_DummyMovedLeft;
	bool m_DummyHookDelay;
	bool m_DummyRuled;
	bool m_DummyEmergency;
	bool m_DummyLeftFreezeFull;
	bool m_DummyGetSpeed;
	bool m_DummyBored;
	bool m_DummySpecialDefend;
	bool m_DummySpecialDefendAttack;
	int m_DummyBoredCounter;
	int m_DummyBlockMode;

	//dummymode 31 vars (ChillBlock5 police guard)
	bool m_DummySpawnAnimation;
	int m_DummySpawnAnimationDelay;
	bool m_DummyPoliceGetSpeed;
	bool m_DummyGotStuck;
	bool m_DummyChillClosestPolice;
	int m_DummyPoliceMode;
	int m_DummyAttackMode;

	//dummymode 23 vars
	int m_DummyHelpBeforeHammerfly;
	bool m_DummyHelpEmergency;
	bool m_DummyHelpNoEmergency;
	bool m_DummyHookAfterHammer;
	bool m_DummyHelpBeforeFly;
	bool m_DummyPanicWhileHelping;
	bool m_DummyPanicBalance;
	bool m_DummyMateFailed;
	bool m_DummyHook;
	bool m_DummyCollectedWeapons;
	bool m_DummyMateCollectedWeapons;
	bool m_DummyRocketJumped;
	bool m_DummyRaceHelpHook;
	bool m_DummyRaceHook;
	int m_DummyRaceState;
	int m_DummyRaceMode;
	int m_DummyNothingHappensCounter;
	int m_DummyPanicWeapon;
	int m_DummyMateHelpMode;
	int m_DummyMovementMode;
	bool m_DummyFreezed;
	int m_DummyEmoteTickNext;

	/////////dummymode variables

	/*************************************************
	*                                                *
	*              B L O C K D D R A C E             *
	*                                                *
	**************************************************/
};

enum
{
	DDRACE_NONE = 0,
	DDRACE_STARTED,
	DDRACE_CHEAT, // no time and won't start again unless ordered by a mod or death
	DDRACE_FINISHED
};

#endif
