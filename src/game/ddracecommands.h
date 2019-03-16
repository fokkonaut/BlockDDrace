/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */

// This file can be included several times.

#ifndef CONSOLE_COMMAND
#define CONSOLE_COMMAND(name, params, flags, callback, userdata, help)
#endif

CONSOLE_COMMAND("kill_pl", "v[id]", CFGFLAG_SERVER, ConKillPlayer, this, "Kills player v and announces the kill")
CONSOLE_COMMAND("totele", "i[number]", CFGFLAG_SERVER|CMDFLAG_TEST, ConToTeleporter, this, "Teleports you to teleporter v")
CONSOLE_COMMAND("totelecp", "i[number]", CFGFLAG_SERVER|CMDFLAG_TEST, ConToCheckTeleporter, this, "Teleports you to checkpoint teleporter v")
CONSOLE_COMMAND("tele", "?i[id] ?i[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConTeleport, this, "Teleports player i (or you) to player i (or you to where you look at)")
CONSOLE_COMMAND("addweapon", "i[weapon-id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConAddWeapon, this, "Gives weapon with id i to you (all = -1, hammer = 0, gun = 1, shotgun = 2, grenade = 3, rifle = 4, ninja = 5)")
CONSOLE_COMMAND("removeweapon", "i[weapon-id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConRemoveWeapon, this, "removes weapon with id i from you (all = -1, hammer = 0, gun = 1, shotgun = 2, grenade = 3, rifle = 4, ninja = 5)")
CONSOLE_COMMAND("shotgun", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConShotgun, this, "Gives a shotgun to you")
CONSOLE_COMMAND("grenade", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConGrenade, this, "Gives a grenade launcher to you")
CONSOLE_COMMAND("rifle", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConRifle, this, "Gives a rifle to you")
CONSOLE_COMMAND("jetpack","?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConJetpack, this, "Gives jetpack to player v")
CONSOLE_COMMAND("weapons", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConWeapons, this, "Gives all weapons to you")
CONSOLE_COMMAND("unshotgun", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnShotgun, this, "Takes the shotgun from you")
CONSOLE_COMMAND("ungrenade", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnGrenade, this, "Takes the grenade launcher you")
CONSOLE_COMMAND("unrifle", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnRifle, this, "Takes the rifle from you")
CONSOLE_COMMAND("unjetpack", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnJetpack, this, "Takes the jetpack from player v")
CONSOLE_COMMAND("unweapons", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnWeapons, this, "Takes all weapons from you")
CONSOLE_COMMAND("ninja", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConNinja, this, "Makes you a ninja")
CONSOLE_COMMAND("super", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConSuper, this, "Makes player v super")
CONSOLE_COMMAND("unsuper", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnSuper, this, "Removes super from player v")
CONSOLE_COMMAND("unsolo", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnSolo, this, "Puts player v out of solo part")
CONSOLE_COMMAND("undeep", "?v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnDeep, this, "Puts player v out of deep freeze")
CONSOLE_COMMAND("left", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConGoLeft, this, "Makes you move 1 tile left")
CONSOLE_COMMAND("right", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConGoRight, this, "Makes you move 1 tile right")
CONSOLE_COMMAND("up", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConGoUp, this, "Makes you move 1 tile up")
CONSOLE_COMMAND("down", "", CFGFLAG_SERVER|CMDFLAG_TEST, ConGoDown, this, "Makes you move 1 tile down")

CONSOLE_COMMAND("plasmagun", "?v[id]", CFGFLAG_SERVER, ConPlasmaGun, this, "Gives plasmagun to player v")
CONSOLE_COMMAND("unplasmagun", "?v[id]", CFGFLAG_SERVER, ConUnPlasmaGun, this, "Takes the plasmagun from player v")
CONSOLE_COMMAND("heartgun", "?v[id]", CFGFLAG_SERVER, ConHeartGun, this, "Gives heartgun to player v")
CONSOLE_COMMAND("unheartgun", "?v[id]", CFGFLAG_SERVER, ConUnHeartGun, this, "Takes the heartgun from player v")
CONSOLE_COMMAND("connectdummy", "?s[amount]", CFGFLAG_SERVER, ConConnectDummy, this, "Connects i dummies")
CONSOLE_COMMAND("disconnectdummy", "s[id/all]", CFGFLAG_SERVER, ConDisconnectDummy, this, "Disconnects dummy s/all")

CONSOLE_COMMAND("move", "i[x] i[y]", CFGFLAG_SERVER|CMDFLAG_TEST, ConMove, this, "Moves to the tile with x/y-number ii")
CONSOLE_COMMAND("move_raw", "i[x] i[y]", CFGFLAG_SERVER|CMDFLAG_TEST, ConMoveRaw, this, "Moves to the point with x/y-coordinates ii")
CONSOLE_COMMAND("force_pause", "v[id] i[seconds]", CFGFLAG_SERVER, ConForcePause, this, "Force i to pause for i seconds")
CONSOLE_COMMAND("force_unpause", "v[id]", CFGFLAG_SERVER, ConForcePause, this, "Set force-pause timer of i to 0.")

CONSOLE_COMMAND("list", "?s[filter]", CFGFLAG_CHAT, ConList, this, "List connected players with optional case-insensitive substring matching filter")
CONSOLE_COMMAND("set_team_ddr", "v[id] ?i[team]", CFGFLAG_SERVER, ConSetDDRTeam, this, "Set ddrace team of a player")
CONSOLE_COMMAND("uninvite", "v[id] ?i[team]", CFGFLAG_SERVER, ConUninvite, this, "Uninvite player from team")

CONSOLE_COMMAND("vote_mute", "v[id] i[seconds]", CFGFLAG_SERVER, ConVoteMute, this, "Remove v's right to vote for i seconds")
CONSOLE_COMMAND("mute", "", CFGFLAG_SERVER, ConMute, this, "")
CONSOLE_COMMAND("muteid", "v[id] i[seconds]", CFGFLAG_SERVER, ConMuteID, this, "")
CONSOLE_COMMAND("muteip", "s[ip] i[seconds]", CFGFLAG_SERVER, ConMuteIP, this, "")
CONSOLE_COMMAND("unmute", "v[id]", CFGFLAG_SERVER, ConUnmute, this, "")
CONSOLE_COMMAND("mutes", "", CFGFLAG_SERVER, ConMutes, this, "")
CONSOLE_COMMAND("moderate", "", CFGFLAG_SERVER, ConModerate, this, "Enables/disables active moderator mode for the player")
CONSOLE_COMMAND("vote_no", "", CFGFLAG_SERVER, ConVoteNo, this, "Same as \"vote no\"")

CONSOLE_COMMAND("freezehammer", "v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConFreezeHammer, this, "Gives a player Freeze Hammer")
CONSOLE_COMMAND("unfreezehammer", "v[id]", CFGFLAG_SERVER|CMDFLAG_TEST, ConUnFreezeHammer, this, "Removes Freeze Hammer from a player")
#undef CONSOLE_COMMAND
