/*************************************************
*                                                *
*              B L O C K D D R A C E             *
*                                                *
**************************************************/

// This file is included in engine/shared/protocol.h, so basically every other file should be able to get these enum entries.
// These are weapons, created by fokkonaut for BlockDDrace.
// NUM_VANILLA_WEAPONS is the old NUM_WEAPONS

#include <game/generated/protocol.h>

#ifndef ENGINE_SHARED_WEAPONS_EX_H
#define ENGINE_SHARED_WEAPONS_EX_H

enum
{
	WEAPON_PLASMA_RIFLE = NUM_VANILLA_WEAPONS,
	WEAPON_HEART_GUN,
	WEAPON_STRAIGHT_GRENADE,
	NUM_WEAPONS
};

#endif // ENGINE_SHARED_WEAPONS_EX_H
