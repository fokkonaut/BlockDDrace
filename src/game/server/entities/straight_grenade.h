/*************************************************
*                                                *
*              B L O C K D D R A C E             *
*                                                *
**************************************************/

// made by fokkonaut

#ifndef GAME_SERVER_ENTITIES_STRAIGHT_GRENADE_H
#define GAME_SERVER_ENTITIES_STRAIGHT_GRENADE_H

#include "stable_projectile.h"

class CStraightGrenade : public CStableProjectile
{
public:
	CStraightGrenade(CGameWorld *pGameWorld, int Lifetime, int Owner, vec2 Pos, vec2 Dir);

	virtual void Reset();
	virtual void Tick();

private:

	void HitCharacter();
	void Move();

	int m_StartTick;
	vec2 m_PrevPos;
	vec2 m_Core;

	int64_t m_TeamMask;
	CCharacter* pOwner;
	int m_Owner;

	vec2 m_Direction;
	int m_Lifetime;
};

#endif
