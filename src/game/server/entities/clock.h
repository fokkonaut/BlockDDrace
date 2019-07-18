/*************************************************
*                                                *
*              B L O C K D D R A C E             *
*                                                *
**************************************************/

// made by fokkonaut

#ifndef GAME_SERVER_ENTITIES_CLOCK_H
#define GAME_SERVER_ENTITIES_CLOCK_H

#include <game/server/entity.h>

enum
{
	SECOND = 0,
	MINUTE,
	HOUR
};

class CClock : public CEntity
{
	struct HandInfo
	{
		int m_Length;
		float m_Rotation;
		vec2 m_To;
	} m_Hand[3];

	int m_ID2;
	int m_ID3;

	void SetHandRotations();
	void Step();
	int GetID(int Hand);
public:
	CClock(CGameWorld *pGameWorld, vec2 Pos);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
};

#endif // GAME_SERVER_ENTITIES_CLOCK_H
