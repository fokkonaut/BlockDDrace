#include "character.h"
#include <game\server\player.h>

void CCharacter::DummyTick()
{
	if (!m_pPlayer->m_IsDummy)
		return;

	ResetInput();
	m_Input.m_Hook = 0;

	if (m_pPlayer->m_Dummymode == 0)
	{
		// do nothing
	}
	else if (m_pPlayer->m_Dummymode == 99) // shop bot
	{
		CCharacter *pChr = GameServer()->m_World.ClosestCharacter(m_Pos, this);
		if (pChr && pChr->m_InShop)
		{
			m_Input.m_TargetX = pChr->m_Pos.x - m_Pos.x;
			m_Input.m_TargetY = pChr->m_Pos.y - m_Pos.y;
			m_LatestInput.m_TargetX = pChr->m_Pos.x - m_Pos.x;
			m_LatestInput.m_TargetY = pChr->m_Pos.y - m_Pos.y;
		}
	}
}