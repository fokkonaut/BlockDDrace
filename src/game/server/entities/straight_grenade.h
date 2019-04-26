#ifndef GAME_SERVER_ENTITIES_STRAIGHT_GRENADE_H
#define GAME_SERVER_ENTITIES_STRAIGHT_GRENADE_H

class CStraightGrenade : public CEntity
{
public:
	CStraightGrenade(CGameWorld *pGameWorld, int Lifetime, int Owner, float Force, vec2 Dir);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

private:

	void CalculateVel();
	virtual void TickDefered();
	void HitCharacter();
	void Move();

	int m_StartTick;
	vec2 m_LastResetPos;
	int m_LastResetTick;
	vec2 m_PrevPos;
	vec2 m_Core;

	bool m_CalculatedVel;
	int m_VelX;
	int m_VelY;

	int64_t m_TeamMask;
	CCharacter* pOwner;
	int m_Owner;

	float m_Force;
	vec2 m_Direction;
	int m_Lifetime;
};

#endif
