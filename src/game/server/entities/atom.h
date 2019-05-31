/*************************************************
*                                                *
*              B L O C K D D R A C E             *
*                                                *
**************************************************/

#ifndef GAME_SERVER_ENTITIES_ATOM_H
#define GAME_SERVER_ENTITIES_ATOM_H

#include <game/server/entity.h>

#define NUM_ATOMS 6

class CAtom : public CEntity
{
	std::vector<CStableProjectile *> m_AtomProjs;
	int m_AtomPosition;

	int m_Owner;
	bool m_Infinite;

public:
	CAtom(CGameWorld *pGameWorld, vec2 Pos, int Owner, bool Infinite);
	
	void Clear();
	virtual void Reset();
	virtual void Tick();
};

#endif
