/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */

// This file can be included several times.

#ifndef CHAT_COMMAND
#define CHAT_COMMAND(name, params, flags, callback, userdata, help, accesslevel)
#endif

CHAT_COMMAND("credits", "", CFGFLAG_CHAT, ConCredits, this, "Shows the credits of the DDRace mod", AUTHED_NO)
CHAT_COMMAND("emote", "?s[emote name] i[duration in seconds]", CFGFLAG_CHAT, ConEyeEmote, this, "Sets your tee's eye emote", AUTHED_NO)
CHAT_COMMAND("eyeemote", "?s['on'|'off'|'toggle']", CFGFLAG_CHAT, ConSetEyeEmote, this, "Toggles use of standard eye-emotes on/off, eyeemote s, where s = on for on, off for off, toggle for toggle and nothing to show current status", AUTHED_NO)
CHAT_COMMAND("settings", "?s[configname]", CFGFLAG_CHAT, ConSettings, this, "Shows gameplay information for this server", AUTHED_NO)
CHAT_COMMAND("help", "?r[command]", CFGFLAG_CHAT, ConHelp, this, "Shows help to command r, general help if left blank", AUTHED_NO)
CHAT_COMMAND("info", "", CFGFLAG_CHAT, ConInfo, this, "Shows info about this server", AUTHED_NO)
CHAT_COMMAND("me", "r[message]", CFGFLAG_CHAT|CFGFLAG_NONTEEHISTORIC, ConMe, this, "Like the famous irc command '/me says hi' will display '<yourname> says hi'", AUTHED_NO)
CHAT_COMMAND("w", "s[player name] r[message]", CFGFLAG_CHAT|CFGFLAG_NONTEEHISTORIC, ConWhisper, this, "Whisper something to someone (private message)", AUTHED_NO)
CHAT_COMMAND("whisper", "s[player name] r[message]", CFGFLAG_CHAT|CFGFLAG_NONTEEHISTORIC, ConWhisper, this, "Whisper something to someone (private message)", AUTHED_NO)
CHAT_COMMAND("c", "r[message]", CFGFLAG_CHAT|CFGFLAG_NONTEEHISTORIC, ConConverse, this, "Converse with the last person you whispered to (private message)", AUTHED_NO)
CHAT_COMMAND("converse", "r[message]", CFGFLAG_CHAT|CFGFLAG_NONTEEHISTORIC, ConConverse, this, "Converse with the last person you whispered to (private message)", AUTHED_NO)
CHAT_COMMAND("pause", "?r[player name]", CFGFLAG_CHAT, ConTogglePause, this, "Toggles pause", AUTHED_NO)
CHAT_COMMAND("spec", "?r[player name]", CFGFLAG_CHAT, ConToggleSpec, this, "Toggles spec (if not available behaves as /pause)", AUTHED_NO)
CHAT_COMMAND("pausevoted", "", CFGFLAG_CHAT, ConTogglePauseVoted, this, "Toggles pause on the currently voted player", AUTHED_NO)
CHAT_COMMAND("specvoted", "", CFGFLAG_CHAT, ConToggleSpecVoted, this, "Toggles spec on the currently voted player", AUTHED_NO)
CHAT_COMMAND("dnd", "", CFGFLAG_CHAT|CFGFLAG_NONTEEHISTORIC, ConDND, this, "Toggle Do Not Disturb (no chat and server messages)", AUTHED_NO)
CHAT_COMMAND("timeout", "?s[code]", CFGFLAG_CHAT, ConTimeout, this, "Set timeout protection code s", AUTHED_NO)
CHAT_COMMAND("save", "r[code]", CFGFLAG_CHAT, ConSave, this, "Save team with code r to current server. To save to another server, use '/save s r' where s = server (case-sensitive: GER, RUS, etc) and r = code", AUTHED_NO)
CHAT_COMMAND("load", "r[code]", CFGFLAG_CHAT, ConLoad, this, "Load with code r", AUTHED_NO)
CHAT_COMMAND("map", "?r[map]", CFGFLAG_CHAT|CFGFLAG_NONTEEHISTORIC, ConMap, this, "Vote a map by name", AUTHED_NO)
CHAT_COMMAND("rankteam", "?r[player name]", CFGFLAG_CHAT, ConTeamRank, this, "Shows the team rank of player with name r (your team rank by default)", AUTHED_NO)
CHAT_COMMAND("teamrank", "?r[player name]", CFGFLAG_CHAT, ConTeamRank, this, "Shows the team rank of player with name r (your team rank by default)", AUTHED_NO)
CHAT_COMMAND("rank", "?r[player name]", CFGFLAG_CHAT, ConRank, this, "Shows the rank of player with name r (your rank by default)", AUTHED_NO)
CHAT_COMMAND("rules", "", CFGFLAG_CHAT, ConRules, this, "Shows the server rules", AUTHED_NO)
CHAT_COMMAND("team", "?i[id]", CFGFLAG_CHAT, ConJoinTeam, this, "Lets you join team i (shows your team if left blank)", AUTHED_NO)
CHAT_COMMAND("lock", "?i['0'|'1']", CFGFLAG_CHAT, ConLockTeam, this, "Lock team so no-one else can join it", AUTHED_NO)
CHAT_COMMAND("invite", "r[player name]", CFGFLAG_CHAT, ConInviteTeam, this, "Invite a person to a locked team", AUTHED_NO)
CHAT_COMMAND("top5team", "?i[rank to start with]", CFGFLAG_CHAT, ConTeamTop5, this, "Shows five team ranks of the ladder beginning with rank i (1 by default)", AUTHED_NO)
CHAT_COMMAND("teamtop5", "?i[rank to start with]", CFGFLAG_CHAT, ConTeamTop5, this, "Shows five team ranks of the ladder beginning with rank i (1 by default)", AUTHED_NO)
CHAT_COMMAND("top5", "?i[rank to start with]", CFGFLAG_CHAT, ConTop5, this, "Shows five ranks of the ladder beginning with rank i (1 by default)", AUTHED_NO)
CHAT_COMMAND("showothers", "?i['0'|'1']", CFGFLAG_CHAT, ConShowOthers, this, "Whether to show players from other teams or not (off by default), optional i = 0 for off else for on", AUTHED_NO)
CHAT_COMMAND("showall", "?i['0'|'1']", CFGFLAG_CHAT, ConShowAll, this, "Whether to show players at any distance (off by default), optional i = 0 for off else for on", AUTHED_NO)
CHAT_COMMAND("specteam", "?i['0'|'1']", CFGFLAG_CHAT, ConSpecTeam, this, "Whether to show players from other teams when spectating (on by default), optional i = 0 for off else for on", AUTHED_NO)
CHAT_COMMAND("ninjajetpack", "?i['0'|'1']", CFGFLAG_CHAT, ConNinjaJetpack, this, "Whether to use ninja jetpack or not. Makes jetpack look more awesome", AUTHED_NO)
CHAT_COMMAND("saytime", "?r[player name]", CFGFLAG_CHAT|CFGFLAG_NONTEEHISTORIC, ConSayTime, this, "Privately messages someone's current time in this current running race (your time by default)", AUTHED_NO)
CHAT_COMMAND("saytimeall", "", CFGFLAG_CHAT|CFGFLAG_NONTEEHISTORIC, ConSayTimeAll, this, "Publicly messages everyone your current time in this current running race", AUTHED_NO)
CHAT_COMMAND("time", "", CFGFLAG_CHAT, ConTime, this, "Privately shows you your current time in this current running race in the broadcast message", AUTHED_NO)
CHAT_COMMAND("r", "", CFGFLAG_CHAT, ConRescue, this, "Teleport yourself out of freeze (use sv_rescue 1 to enable this feature)", AUTHED_NO)
CHAT_COMMAND("rescue", "", CFGFLAG_CHAT, ConRescue, this, "Teleport yourself out of freeze (use sv_rescue 1 to enable this feature)", AUTHED_NO)

CHAT_COMMAND("kill", "", CFGFLAG_CHAT, ConProtectedKill, this, "Kill yourself", AUTHED_NO)


/*************************************************
*                                                *
*              B L O C K D D R A C E             *
*                                                *
**************************************************/

//score
CHAT_COMMAND("score", "?s", CFGFLAG_CHAT, ConScore, this, "Changes the displayed score in scoreboard", AUTHED_NO)

//info
CHAT_COMMAND("spookyghost", "", CFGFLAG_CHAT, ConSpookyGhostInfo, this, "Shows information about the spooky ghost", AUTHED_NO)
CHAT_COMMAND("policeinfo", "?i", CFGFLAG_CHAT, ConPoliceInfo, this, "Shows information about police", AUTHED_NO)

//account
CHAT_COMMAND("register", "?s[name] ?s[password] ?s[password]", CFGFLAG_CHAT, ConRegister, this, "Register an account", AUTHED_NO)
CHAT_COMMAND("login", "?s[name] ?s[password]", CFGFLAG_CHAT, ConLogin, this, "Log into an account", AUTHED_NO)
CHAT_COMMAND("logout", "", CFGFLAG_CHAT, ConLogout, this, "Log out of an account", AUTHED_NO)

//extras
CHAT_COMMAND("weaponindicator", "", CFGFLAG_CHAT, ConWeaponIndicator, this, "Tells you which weapon you are holding under the heart and armor bar", AUTHED_NO)

//other
CHAT_COMMAND("resumemoved", "?i['0'|'1']", CFGFLAG_CHAT, ConResumeMoved, this, "Whether to resume from pause when someone moved your tee (off by default), optional i = 0 for off else for on", AUTHED_NO)

//minigames
CHAT_COMMAND("minigames", "", CFGFLAG_CHAT, ConMinigames, this, "Shows a list of all available minigames", AUTHED_NO)
CHAT_COMMAND("leave", "", CFGFLAG_CHAT, ConLeaveMinigame, this, "Leaves the current minigame", AUTHED_NO)
CHAT_COMMAND("block", "?s[enable/disable]", CFGFLAG_CHAT, ConJoinBlock, this, "Joins the block minigame", AUTHED_NO)
CHAT_COMMAND("survival", "?s[enable/disable]", CFGFLAG_CHAT, ConJoinSurvival, this, "Joins the survival minigame", AUTHED_NO)
CHAT_COMMAND("boomfng", "?s[enable/disable]", CFGFLAG_CHAT, ConJoinBoomFNG, this, "Joins the boom fng minigame", AUTHED_NO)
CHAT_COMMAND("fng", "?s[enable/disable]", CFGFLAG_CHAT, ConJoinFNG, this, "Joins the fng minigame", AUTHED_NO)
#undef CHAT_COMMAND
