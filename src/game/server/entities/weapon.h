#ifndef GAME_SERVER_ENTITIES_WEAPON_H
#define GAME_SERVER_ENTITIES_WEAPON_H

class CWeapon : public CEntity
{
public:
	CWeapon(CGameWorld *pGameWorld, int Weapon, int Lifetime, int Owner, int Direction, int Bullets, bool Jetpack = false);

	virtual void Reset(bool EreaseWeapon = true, bool Picked = false);
	virtual void Tick();
	virtual void Snap(int SnappingClient);

	static int const ms_PhysSize = 14;

private:
	int IsCharacterNear();
	void IsShieldNear();
	void Pickup();
	bool IsGrounded(bool SetVel = false);

	vec2 m_Vel;

	int64_t m_TeamMask;
	CCharacter* pOwner;
	int m_Owner;

	int m_Type;
	int m_Lifetime;
	int m_Bullets;
	int m_PickupDelay;
	bool m_Jetpack;
	vec2 m_PrevPos;

	// have to define a new ID variable for the bullet
	int m_ID2;

	void HandleTiles(int Index);
	int MapIndexL;
	int MapIndexR;
	int MapIndexT;
	int MapIndexB;
	int m_TileIndex;
	int m_TileFlags;
	int m_TileFIndex;
	int m_TileFFlags;
	int m_TileSIndex;
	int m_TileSFlags;
	int m_TileIndexL;
	int m_TileFlagsL;
	int m_TileFIndexL;
	int m_TileFFlagsL;
	int m_TileSIndexL;
	int m_TileSFlagsL;
	int m_TileIndexR;
	int m_TileFlagsR;
	int m_TileFIndexR;
	int m_TileFFlagsR;
	int m_TileSIndexR;
	int m_TileSFlagsR;
	int m_TileIndexT;
	int m_TileFlagsT;
	int m_TileFIndexT;
	int m_TileFFlagsT;
	int m_TileIndexB;
	int m_TileFlagsB;
	int m_TileFIndexB;
	int m_TileFFlagsB;
};

#endif
