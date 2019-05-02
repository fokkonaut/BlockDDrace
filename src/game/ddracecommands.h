/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */

// This file can be included several times.

#ifndef CONSOLE_COMMAND
#define CONSOLE_COMMAND(name, params, flags, callback, userdata, help)
#endif

CONSOLE_COMMAND("kill_pl", "v[id]", CFGFLAG_SERVER, ConKillPlayer, this, "Kills player v and announces the kill")
CONSOLE_COMMAND("totele", "i[number]", CFGFLAG_SERVER|CMDFLAG_TEST, ConToTeleporter, this, "Teleports you to teleporter v")
CONSOLE_COMMAND("totelecp", "i[number]", CFGFLAG_SERVER|CMDFLAG_TEST, ConToCheckTeleporter, this, "Teleports you to checkpoint teleporter v")
CONSOLE_COMMAND("tele", "?v[id] ?i[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConTeleport, this, "Teleports player v to player i")
CONSOLE_COMMAND("addweapon", "i[weapon-id] ?v[id] ?i[spread]", CFGFLAG_SERVER|CMDFLAG_TEST, ConAddWeapon, this, "Gives weapon with id i to v (all=-1, extra=-2, hammer=0, gun=1, shotgun=2, grenade=3, rifle=4, ninja=5)")
CONSOLE_COMMAND("removeweapon", "i[weapon-id] ?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConRemoveWeapon, this, "removes weapon with id i from v (all=-1, extra=-2, hammer=0, gun=1, shotgun=2, grenade=3, rifle=4, ninja=5)")
CONSOLE_COMMAND("shotgun", "?v[id] ?i[spread]", CFGFLAG_SERVER|CMDFLAG_TEST, ConShotgun, this, "Gives a shotgun to player v")
CONSOLE_COMMAND("grenade", "?v[id] ?i[spread]", CFGFLAG_SERVER|CMDFLAG_TEST, ConGrenade, this, "Gives a grenade launcher to player v")
CONSOLE_COMMAND("rifle", "?v[id] ?i[spread]", CFGFLAG_SERVER|CMDFLAG_TEST, ConRifle, this, "Gives a rifle to player v")
CONSOLE_COMMAND("weapons", "?v[id] ?i[spread]", CFGFLAG_SERVER|CMDFLAG_TEST, ConWeapons, this, "Gives all weapons to player v")
CONSOLE_COMMAND("unshotgun", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnShotgun, this, "Takes the shotgun from player v")
CONSOLE_COMMAND("ungrenade", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnGrenade, this, "Takes the grenade launcher player v")
CONSOLE_COMMAND("unrifle", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnRifle, this, "Takes the rifle from player v")
CONSOLE_COMMAND("unweapons", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnWeapons, this, "Takes all weapons from player v")
CONSOLE_COMMAND("ninja", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConNinja, this, "Makes player v a ninja")
CONSOLE_COMMAND("super", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConSuper, this, "Makes player v super")
CONSOLE_COMMAND("unsuper", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnSuper, this, "Removes super from player v")
CONSOLE_COMMAND("unsolo", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnSolo, this, "Puts player v out of solo part")
CONSOLE_COMMAND("undeep", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnDeep, this, "Puts player v out of deep freeze")
CONSOLE_COMMAND("left", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConGoLeft, this, "Makes you move 1 tile left")
CONSOLE_COMMAND("right", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConGoRight, this, "Makes you move 1 tile right")
CONSOLE_COMMAND("up", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConGoUp, this, "Makes you move 1 tile up")
CONSOLE_COMMAND("down", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConGoDown, this, "Makes you move 1 tile down")

CONSOLE_COMMAND("extraweapons", "?v[id] ?i[spread]", CFGFLAG_SERVER|CMDFLAG_TEST, ConExtraWeapons, this, "Gives all extra weapons to player v")
CONSOLE_COMMAND("unextraweapons", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnExtraWeapons, this, "Takes all extra weapons from player v")

CONSOLE_COMMAND("plasmarifle", "?v[id] ?i[spread]", CFGFLAG_SERVER|CMDFLAG_TEST, ConPlasmaRifle, this, "Gives a plasma rifle to player v")
CONSOLE_COMMAND("unplasmarifle", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnPlasmaRifle, this, "Takes the plasma rifle from player v")
CONSOLE_COMMAND("heartgun", "?v[id] ?i[spread]", CFGFLAG_SERVER, ConHeartGun, this, "Gives a heart gun to player v")
CONSOLE_COMMAND("unheartgun", "?v[id]", CFGFLAG_SERVER, ConUnHeartGun, this, "Takes the heart gun from player v")
CONSOLE_COMMAND("straightgrenade", "?v[id] ?i[spread]", CFGFLAG_SERVER, ConStraightGrenade, this, "Gives a straight grenade to player v")
CONSOLE_COMMAND("unstraightgrenade", "?v[id]", CFGFLAG_SERVER, ConUnStraightGrenade, this, "Takes the straight grenade from player v")

CONSOLE_COMMAND("hammer", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConHammer, this, "Gives a hammer to player v")
CONSOLE_COMMAND("gun", "?v[id] ?i[spread]", CFGFLAG_SERVER|CMDFLAG_TEST, ConGun, this, "Gives a gun to player v")
CONSOLE_COMMAND("unhammer", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnHammer, this, "Takes the hammer from player v")
CONSOLE_COMMAND("ungun", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnGun, this, "Takes the gun from player v")

CONSOLE_COMMAND("scrollninja", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConScrollNinja, this, "Gives a ninja to player v")

CONSOLE_COMMAND("connectdummy", "?i[amount] ?i[dummymode]", CFGFLAG_SERVER, ConConnectDummy, this, "Connects i dummies")
CONSOLE_COMMAND("disconnectdummy", "v[id]", CFGFLAG_SERVER, ConDisconnectDummy, this, "Disconnects dummy v")
CONSOLE_COMMAND("dummymode", "v[id] ?i[dummymode]", CFGFLAG_SERVER, ConDummymode, this, "Sets or shows the dummymode of dummy v")
CONSOLE_COMMAND("connectdefaultdummies", "", CFGFLAG_SERVER, ConConnectDefaultDummies, this, "Connects default dummies")

CONSOLE_COMMAND("playerinfo", "s[name]", CFGFLAG_SERVER|CFGFLAG_CHAT, ConPlayerInfo, this, "Shows information about the player s")

CONSOLE_COMMAND("hookpower", "?s[power] ?v[id]", CFGFLAG_SERVER, ConHookPower, this, "Sets hook power for player v")
CONSOLE_COMMAND("freezehammer", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConFreezeHammer, this, "Toggles freeze hammer for player v")

CONSOLE_COMMAND("infinitejumps", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConInfiniteJumps, this, "Toggles infinite jumps for player v")
CONSOLE_COMMAND("endlesshook", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConEndlessHook, this, "Toggles endlesshook for player v")
CONSOLE_COMMAND("jetpack", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConJetpack, this, "Toggles jetpack for player v")
CONSOLE_COMMAND("rainbow", "?v[id]", CFGFLAG_SERVER, ConRainbow, this, "Toggles rainbow for player v")
CONSOLE_COMMAND("infrainbow", "?v[id]", CFGFLAG_SERVER, ConInfRainbow, this, "Toggles infinite rainbow for player v")
CONSOLE_COMMAND("atom", "?v[id]", CFGFLAG_SERVER, ConAtom, this, "Toggles atom for player v")
CONSOLE_COMMAND("infatom", "?v[id]", CFGFLAG_SERVER, ConInfAtom, this, "Toggles infinite atom for player v")
CONSOLE_COMMAND("trail", "?v[id]", CFGFLAG_SERVER, ConTrail, this, "Toggles trail for player v")
CONSOLE_COMMAND("inftrail", "?v[id]", CFGFLAG_SERVER, ConInfTrail, this, "Toggles infinite for player v")
CONSOLE_COMMAND("spookyghost", "?v[id]", CFGFLAG_SERVER, ConSpookyGhost, this, "Toggles spooky ghost for player v")
CONSOLE_COMMAND("addmeteor", "?v[id]", CFGFLAG_SERVER, ConAddMeteor, this, "Adds a meteors to player v")
CONSOLE_COMMAND("addinfmeteor", "?v[id]", CFGFLAG_SERVER, ConAddInfMeteor, this, "Adds an infinite meteors to player v")
CONSOLE_COMMAND("removemeteors", "?v[id", CFGFLAG_SERVER, ConRemoveMeteors, this, "Removes all meteors from player v")
CONSOLE_COMMAND("passive", "?v[id]", CFGFLAG_SERVER, ConPassive, this, "Toggles passive mode for player v")
CONSOLE_COMMAND("vanillamode", "?v[id]", CFGFLAG_SERVER, ConVanillaMode, this, "Activates vanilla mode for player v")
CONSOLE_COMMAND("ddracemode", "?v[id]", CFGFLAG_SERVER, ConDDraceMode, this, "Deactivates vanilla mode for player v")
CONSOLE_COMMAND("bloody", "?v[id]", CFGFLAG_SERVER, ConBloody, this, "Toggles bloody for player v")
CONSOLE_COMMAND("strongbloody", "?v[id]", CFGFLAG_SERVER, ConStrongBloody, this, "Toggles strong bloody for player v")
CONSOLE_COMMAND("policehelper", "?v[id]", CFGFLAG_SERVER, ConPoliceHelper, this, "Toggles police helper for player v")

CONSOLE_COMMAND("move", "i[x] i[y]", CFGFLAG_SERVER|CMDFLAG_TEST, ConMove, this, "Moves to the tile with x/y-number ii")
CONSOLE_COMMAND("move_raw", "i[x] i[y]", CFGFLAG_SERVER|CMDFLAG_TEST, ConMoveRaw, this, "Moves to the point with x/y-coordinates ii")
CONSOLE_COMMAND("force_pause", "v[id] i[seconds]", CFGFLAG_SERVER, ConForcePause, this, "Force i to pause for i seconds")
CONSOLE_COMMAND("force_unpause", "v[id]", CFGFLAG_SERVER, ConForcePause, this, "Set force-pause timer of i to 0.")

CONSOLE_COMMAND("list", "?s[filter]", CFGFLAG_CHAT, ConList, this, "List connected players with optional case-insensitive substring matching filter")
CONSOLE_COMMAND("set_team_ddr", "v[id] ?i[team]", CFGFLAG_SERVER, ConSetDDRTeam, this, "Set ddrace team of a player")
CONSOLE_COMMAND("uninvite", "v[id] ?i[team]", CFGFLAG_SERVER, ConUninvite, this, "Uninvite player from team")

CONSOLE_COMMAND("vote_mute", "v[id] i[seconds]", CFGFLAG_SERVER, ConVoteMute, this, "Remove v's right to vote for i seconds")
CONSOLE_COMMAND("vote_unmute", "v[id]", CFGFLAG_SERVER, ConVoteUnmute, this, "Give back v's right to vote.")
CONSOLE_COMMAND("vote_mutes", "", CFGFLAG_SERVER, ConVoteMutes, this, "List the current active vote mutes.")
CONSOLE_COMMAND("mute", "", CFGFLAG_SERVER, ConMute, this, "")
CONSOLE_COMMAND("muteid", "v[id] i[seconds]", CFGFLAG_SERVER, ConMuteID, this, "")
CONSOLE_COMMAND("muteip", "s[ip] i[seconds]", CFGFLAG_SERVER, ConMuteIP, this, "")
CONSOLE_COMMAND("unmute", "v[id]", CFGFLAG_SERVER, ConUnmute, this, "")
CONSOLE_COMMAND("mutes", "", CFGFLAG_SERVER, ConMutes, this, "")
CONSOLE_COMMAND("moderate", "", CFGFLAG_SERVER, ConModerate, this, "Enables/disables active moderator mode for the player")
CONSOLE_COMMAND("vote_no", "", CFGFLAG_SERVER, ConVoteNo, this, "Same as \"vote no\"")
#undef CONSOLE_COMMAND
