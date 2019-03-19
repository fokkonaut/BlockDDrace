#include <game/server/gamecontext.h>
#include <engine/shared/config.h>
#include "meteor.h"

CMeteor::CMeteor(CGameWorld *pGameWorld, vec2 Pos, int Owner, bool Infinite)
: CStableProjectile(pGameWorld, WEAPON_SHOTGUN, Pos)
{
	m_Vel = vec2(0.1f, 0.1f);
	m_Owner = Owner;
	m_Infinite = Infinite;
}

void CMeteor::Reset()
{
	CCharacter* pChr = GameServer()->GetPlayerChar(m_Owner);
	if (m_Infinite && pChr)
		pChr->GetPlayer()->m_InfMeteors--;
	else if (!m_Infinite && pChr)
		pChr->m_Meteors--;
	GameServer()->m_World.DestroyEntity(this);
}

void CMeteor::Tick()
{
	CCharacter* pChr = GameServer()->GetPlayerChar(m_Owner);
	if (
		GameServer()->m_ClientLeftServer[m_Owner]
		|| (!pChr && !m_Infinite)
		|| (m_Infinite && pChr && !pChr->GetPlayer()->m_InfMeteors)
		|| (!m_Infinite && pChr && !pChr->m_Meteors)
		)
	{
		Reset();
	}

	float Friction = g_Config.m_SvMeteorFriction / 1000000.f;
	float MaxAccel = g_Config.m_SvMeteorMaxAccel / 1000.f;
	float AccelPreserve = g_Config.m_SvMeteorAccelPreserve / 1000.f;

	if(pChr)
	{
		vec2 CharPos = pChr->m_Pos;
		m_Vel += normalize(CharPos - m_Pos) * (MaxAccel*AccelPreserve / (distance(CharPos, m_Pos) + AccelPreserve));
	}
	m_Pos += m_Vel;
	m_Vel *= 1.f - Friction;
}
