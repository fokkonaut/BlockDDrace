
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_VARIABLES_H
#define GAME_VARIABLES_H
#undef GAME_VARIABLES_H // this file will be included several times

// dummy
MACRO_CONFIG_INT(ClDummy, cl_dummy, 0, 0, 1, CFGFLAG_CLIENT, "0 - player / 1 - dummy", AUTHED_NO)

// server
MACRO_CONFIG_INT(SvWarmup, sv_warmup, 0, 0, 0, CFGFLAG_SERVER, "Number of seconds to do warmup before round starts", AUTHED_ADMIN)
MACRO_CONFIG_STR(SvMotd, sv_motd, 900, "", CFGFLAG_SERVER, "Message of the day to display for the clients", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvTournamentMode, sv_tournament_mode, 0, 0, 1, CFGFLAG_SERVER, "Tournament mode. When enabled, players joins the server as spectator", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvSpamprotection, sv_spamprotection, 1, 0, 1, CFGFLAG_SERVER, "Spam protection", AUTHED_ADMIN)

MACRO_CONFIG_INT(SvSpectatorSlots, sv_spectator_slots, 0, 0, MAX_CLIENTS, CFGFLAG_SERVER, "Number of slots to reserve for spectators", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvInactiveKickTime, sv_inactivekick_time, 0, 0, 1000, CFGFLAG_SERVER, "How many minutes to wait before taking care of inactive players", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvInactiveKick, sv_inactivekick, 0, 0, 2, CFGFLAG_SERVER, "How to deal with inactive players (0=move to spectator, 1=move to free spectator slot/kick, 2=kick)", AUTHED_ADMIN)

MACRO_CONFIG_INT(SvStrictSpectateMode, sv_strict_spectate_mode, 0, 0, 1, CFGFLAG_SERVER, "Restricts information in spectator mode", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvVoteSpectate, sv_vote_spectate, 0, 0, 1, CFGFLAG_SERVER, "Allow voting to move players to spectators", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvVoteSpectateRejoindelay, sv_vote_spectate_rejoindelay, 3, 0, 1000, CFGFLAG_SERVER, "How many minutes to wait before a player can rejoin after being moved to spectators by vote", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvVoteKick, sv_vote_kick, 0, 0, 1, CFGFLAG_SERVER, "Allow voting to kick players", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvVoteKickMin, sv_vote_kick_min, 0, 0, MAX_CLIENTS, CFGFLAG_SERVER, "Minimum number of players required to start a kick vote", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvVoteKickBantime, sv_vote_kick_bantime, 15, 0, 1440, CFGFLAG_SERVER, "The time in seconds to ban a player if kicked by vote. 0 makes it just use kick", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvJoinVoteDelay, sv_join_vote_delay, 300, 0, 1000, CFGFLAG_SERVER, "Add a delay before recently joined players can call any vote or participate in a kick/spec vote (in seconds)", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvOldTeleportWeapons, sv_old_teleport_weapons, 0, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Teleporting of all weapons (deprecated, use special entities instead)", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvOldTeleportHook, sv_old_teleport_hook, 0, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Hook through teleporter (deprecated, use special entities instead)", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvTeleportHoldHook, sv_teleport_hold_hook, 0, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Hold hook when teleported", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvTeleportLoseWeapons, sv_teleport_lose_weapons, 0, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Lose weapons when teleported (useful for some race maps)", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvDeepfly, sv_deepfly, 1, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Allow fire non auto weapons when deep", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvDestroyBulletsOnDeath, sv_destroy_bullets_on_death, 1, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Destroy bullets when their owner dies", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvDestroyLasersOnDeath, sv_destroy_lasers_on_death, 0, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Destroy lasers when their owner dies", AUTHED_ADMIN)

MACRO_CONFIG_INT(SvMapUpdateRate, sv_mapupdaterate, 5, 1, 100, CFGFLAG_SERVER, "64 player id <-> vanilla id players map update rate", AUTHED_ADMIN)

MACRO_CONFIG_STR(SvServerType, sv_server_type, 64, "none", CFGFLAG_SERVER, "Type of the server (novice, moderate, ...)", AUTHED_ADMIN)

MACRO_CONFIG_INT(SvSendVotesPerTick, sv_send_votes_per_tick, 5, 1, 15, CFGFLAG_SERVER, "Number of vote options being send per tick", AUTHED_ADMIN)

MACRO_CONFIG_INT(SvRescue, sv_rescue, 0, 0, 1, CFGFLAG_SERVER, "Allow /rescue command so players can teleport themselves out of freeze", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvRescueDelay, sv_rescue_delay, 5, 0, 1000, CFGFLAG_SERVER, "Number of seconds between two rescues", AUTHED_ADMIN)

// debug
#ifdef CONF_DEBUG // this one can crash the server if not used correctly
	MACRO_CONFIG_INT(DbgDummies, dbg_dummies, 0, 0, 15, CFGFLAG_SERVER, "", AUTHED_ADMIN)
#endif


/*************************************************
*                                                *
*              B L O C K D D R A C E             *
*                                                *
**************************************************/

// account
MACRO_CONFIG_INT(SvAccounts, sv_accounts, 0, 0, 1, CFGFLAG_SERVER, "Whether accounts are activated or deactivated", AUTHED_ADMIN)
MACRO_CONFIG_STR(SvAccFilePath, sv_acc_file_path, 128, "data/accounts", CFGFLAG_SERVER, "The path were the server searches the .acc files", AUTHED_ADMIN)

// flags
MACRO_CONFIG_INT(SvFlagSounds, sv_flag_sounds, 0, 0, 1, CFGFLAG_SERVER, "Whether flags create a public sound on drop/pickup/respawn", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvFlagHooking, sv_flag_hooking, 1, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Whether flags are hookable", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvFlagSpectating, sv_flag_spectating, 0, 0, 1, CFGFLAG_SERVER, "Whether you can spectate flags", AUTHED_ADMIN)

// dummy
MACRO_CONFIG_INT(SvHideDummies, sv_hide_dummies, 1, 0, 2, CFGFLAG_SERVER, "Whether to hide server-side dummies (0=show dummies, 1=show in scoreboard, 2=hide in browser and scoreboard)", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvHideDummiesStatus, sv_hide_dummies_status, 1, 0, 1, CFGFLAG_SERVER, "Whether to hide server-side dummies from status command", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvDefaultDummies, sv_default_dummies, 0, 0, 1, CFGFLAG_SERVER, "Whether to create default dummies for specific maps when the server starts", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvFakeDummyPing, sv_fake_dummy_ping, 0, 0, 1, CFGFLAG_SERVER, "Whether ping of server-side dummies are more natural or 1000", AUTHED_ADMIN)
MACRO_CONFIG_INT(V3OffsetX, v3_offset_x, -1, 0, 9999, CFGFLAG_SERVER|CFGFLAG_GAME, "Offset X for the blmapV3 dummy", AUTHED_ADMIN)
MACRO_CONFIG_INT(V3OffsetY, v3_offset_y, -1, 0, 9999, CFGFLAG_SERVER|CFGFLAG_GAME, "Offset Y for the blmapV3 dummy", AUTHED_ADMIN)

MACRO_CONFIG_INT(SvHideServerInfo, sv_hide_server_info, 0, 0, 1, CFGFLAG_SERVER, "Whether to hide server info like map and gametype from the browser", AUTHED_ADMIN)

// weapon indicator
MACRO_CONFIG_INT(SvWeaponIndicatorDefault, sv_weapon_indicator_default, 0, 0, 1, CFGFLAG_SERVER, "Whether the weapon names are displayed under the health and armor bars", AUTHED_ADMIN)

// drops
MACRO_CONFIG_INT(SvDropWeapons, sv_drop_weapons, 1, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Whether to allow dropping weapons with f4", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvDropsOnDeath, sv_drops_on_death, 0, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Whether there is a chance of dropping weapons on death (health and armor in survival)", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvDestroyDropsOnLeave, sv_destroy_drops_on_leave, 1, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Destroy dropped weapons (hearts, shields) when their owner disconnects", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvMaxWeaponDrops, sv_max_weapon_drops, 5, 0, 10, CFGFLAG_SERVER, "Maximum amount of dropped weapons per player", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvMaxPickupDrops, sv_max_pickup_drops, 500, 0, 600, CFGFLAG_SERVER, "Maximum amount of dropped hearts and shields", AUTHED_ADMIN)

// spread weapons
MACRO_CONFIG_INT(SvNumSpreadShots, sv_num_spread_shots, 3, 3, 9, CFGFLAG_SERVER, "Number of shots for the spread weapons", AUTHED_ADMIN)

// vanilla
MACRO_CONFIG_INT(SvVanillaModeStart, sv_vanilla_mode_start, 0, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Whether to set the players mode to vanilla on spawn or ddrace", AUTHED_ADMIN)

//survival
MACRO_CONFIG_INT(SvSurvivalMinPlayers, sv_survival_min_players, 4, 2, MAX_CLIENTS, CFGFLAG_SERVER|CFGFLAG_GAME, "Minimum players to start a survival round", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvSurvivalLobbyCountdown, sv_survival_lobby_countdown, 30, 5, 120, CFGFLAG_SERVER|CFGFLAG_GAME, "Number in seconds until the survival round starts", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvSurvivalRoundTime, sv_survival_round_time, 5, 1, 20, CFGFLAG_SERVER|CFGFLAG_GAME, "Time in minutes until deathmatch starts", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvSurvivalDeathmatchTime, sv_survival_deathmatch_time, 2, 1, 5, CFGFLAG_SERVER|CFGFLAG_GAME, "Length of the deathmatch in minutes", AUTHED_ADMIN)

// other
MACRO_CONFIG_INT(SvAuthedPlayersColored, sv_authed_players_colored, 1, 0, 1, CFGFLAG_SERVER, "Whether authed players have a colored name in scoreboard", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvHideMinigamePlayers, sv_hide_minigame_players, 0, 0, 1, CFGFLAG_SERVER, "Whether players in different minigames are shown in the scoreboard", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvRainbowSpeedDefault, sv_rainbow_speed_default, 1, 1, 50, CFGFLAG_SERVER, "Whether players in different minigames are shown in the scoreboard", AUTHED_ADMIN)
MACRO_CONFIG_INT(SvDefaultScore, sv_default_score, 0, 0, 1, CFGFLAG_SERVER, "Default score (0 = time, 1 = level)", AUTHED_ADMIN)

MACRO_CONFIG_INT(SvOldJetpackSound, sv_old_jetpack_sound, 1, 0, 1, CFGFLAG_SERVER, "Whether to use the default gun sound for jetpack or another sound", AUTHED_ADMIN)

MACRO_CONFIG_INT(SvFakeBlocks, sv_fake_blocks, 1, 0, 1, CFGFLAG_SERVER, "Whether fake blocks are activated", AUTHED_ADMIN)
#endif
