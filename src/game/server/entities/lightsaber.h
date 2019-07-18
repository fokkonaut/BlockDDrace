/*************************************************
*                                                *
*              B L O C K D D R A C E             *
*                                                *
**************************************************/

// made by fokkonaut

#ifndef GAME_SERVER_ENTITIES_LIGHTSABER_H
#define GAME_SERVER_ENTITIES_LIGHTSABER_H

#include <game/server/entity.h>

#define SPEED 15
#define RETRACTED_LENGTH 0
#define EXTENDED_LENGTH 200

class CLightsaber : public CEntity
{
	struct LightsaberInfo
	{
		int m_Length;
		float m_Rotation;
		vec2 m_To;
		bool m_Extending;
		bool m_Retracting;
	} m_Lightsaber;

	int m_Owner;
	CCharacter *m_pOwner;
	int m_EvalTick;
	int m_SoundTick;
	int64_t m_TeamMask;

	int m_LastHit[MAX_CLIENTS];

	void PlaySound();
	bool HitCharacter();
	void Step();
	int GetID(int Hand);
public:
	CLightsaber(CGameWorld *pGameWorld, int Owner);

	void Extend();
	void Retract();
	virtual void TickDefered();
	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
};

#endif // GAME_SERVER_ENTITIES_LIGHTSABER_H
