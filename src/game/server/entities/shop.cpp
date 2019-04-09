#include "character.h"
#include <game\server\player.h>

void CCharacter::ShopWindow(int Dir)
{
	m_ShopMotdTick = 0;
	int m_MaxShopPage = 3; // UPDATE THIS WITH EVERY PAGE YOU ADD!!!!!

	if (Dir == 0)
		m_ShopWindowPage = 0;
	else if (Dir == 1)
	{
		m_ShopWindowPage++;
		if (m_ShopWindowPage > m_MaxShopPage)
			m_ShopWindowPage = 0;
	}
	else if (Dir == -1)
	{
		m_ShopWindowPage--;
		if (m_ShopWindowPage < 0)
			m_ShopWindowPage = m_MaxShopPage;
	}

	char aItem[256];
	char aLevelTmp[128];
	char aPriceTmp[16];
	char aTimeTmp[256];
	char aInfo[1028];

	if (m_ShopWindowPage == 0)
	{
		str_format(aItem, sizeof(aItem), "Welcome to the shop!\n\n"
			"By shooting to the right you go one site forward,\n"
			"and by shooting left you go one site backwards.");
	}
	else if (m_ShopWindowPage == 1)
	{
		str_format(aItem, sizeof(aItem), "        ~  R A I N B O W  ~      ");
		str_format(aLevelTmp, sizeof(aLevelTmp), "5");
		str_format(aPriceTmp, sizeof(aPriceTmp), "1.500");
		str_format(aTimeTmp, sizeof(aTimeTmp), "You own this item until you're dead.");
		str_format(aInfo, sizeof(aInfo), "Rainbow will make your tee change the color very fast.");
	}
	else if (m_ShopWindowPage == 2)
	{
		str_format(aItem, sizeof(aItem), "        ~  B L O O D Y  ~      ");
		str_format(aLevelTmp, sizeof(aLevelTmp), "15");
		str_format(aPriceTmp, sizeof(aPriceTmp), "3.500");
		str_format(aTimeTmp, sizeof(aTimeTmp), "You own this item until you're dead.");
		str_format(aInfo, sizeof(aInfo), "Bloody will give your tee a permanent kill effect.");
	}
	else if (m_ShopWindowPage == 3)
	{
		str_format(aItem, sizeof(aItem), "       ~  S P O O K Y G H O S T  ~     ");
		str_format(aLevelTmp, sizeof(aLevelTmp), "1");
		str_format(aPriceTmp, sizeof(aPriceTmp), "1.000.000");
		str_format(aTimeTmp, sizeof(aTimeTmp), "You own this item forever.");
		str_format(aInfo, sizeof(aInfo), "Using this item you can hide from other players behind bushes.\n"
			"If your ghost is activated you won't have a name\n"
			"For more information please visit '/spookyghost help'.");
	}
	else
	{
		str_format(aItem, sizeof(aItem), "");
	}
	//////////////////// UPDATE m_MaxShopPage ON TOP OF THIS FUNCTION!!! /////////////////////////

	char aLevel[128];
	str_format(aLevel, sizeof(aLevel), "Needed level: %s", aLevelTmp);
	char aPrice[16];
	str_format(aPrice, sizeof(aPrice), "Price: %s", aPriceTmp);
	char aTime[256];
	str_format(aTime, sizeof(aTime), "Time: %s", aTimeTmp);

	char aBase[512];
	if (m_ShopWindowPage > 0)
	{
		str_format(aBase, sizeof(aBase),
			"***************************\n"
			"        ~  S H O P  ~      \n"
			"***************************\n\n"
			"%s\n\n"
			"%s\n"
			"%s\n"
			"%s\n\n"
			"%s\n\n"
			"***************************\n"
			"If you want to buy an item press f3.\n\n\n"
			"              ~ %d ~              ", aItem, aLevel, aPrice, aTime, aInfo, m_ShopWindowPage);
	}
	else
	{
		str_format(aBase, sizeof(aBase),
			"***************************\n"
			"        ~  S H O P  ~      \n"
			"***************************\n\n"
			"%s\n\n"
			"***************************\n"
			"If you want to buy an item press f3.", aItem);
	}

	GameServer()->SendMotd(aBase, GetPlayer()->GetCID());
	m_ShopMotdTick = Server()->Tick() + Server()->TickSpeed() * 10; // motd is there for 10 sec
}

void CCharacter::ConfirmPurchase()
{
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf),
		"***************************\n"
		"        ~  S H O P  ~      \n"
		"***************************\n\n"
		"Are you sure you want to buy this item?\n\n"
		"f3 - yes\n"
		"f4 - no\n\n"
		"***************************\n");

	GameServer()->SendMotd(aBuf, GetPlayer()->GetCID());
	m_PurchaseState = 2;
}

void CCharacter::PurchaseEnd(bool canceled)
{
	char aResult[256];
	if (canceled)
	{
		char aBuf[256];
		str_format(aResult, sizeof(aResult), "You canceled the purchase.");
		str_format(aBuf, sizeof(aBuf),
			"***************************\n"
			"        ~  S H O P  ~      \n"
			"***************************\n\n"
			"%s\n\n"
			"***************************\n", aResult);

		GameServer()->SendMotd(aBuf, GetPlayer()->GetCID());
	}
	else
	{
		BuyItem(m_ShopWindowPage);
		ShopWindow(0);
	}

	m_PurchaseState = 1;
}

void CCharacter::BuyItem(int ItemID)
{
	if (!m_InShop)
	{
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "You have to be in the shop to buy some items.");
		return;
	}

	if (ItemID == 1)
	{
		if (m_Rainbow || m_pPlayer->m_InfRainbow)
		{
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "You already own rainbow.");
			return;
		}

		if (m_pPlayer->m_Level < 5)
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Your level is too low! You need to be level 5 to buy rainbow.");
		else
		{
			if (m_pPlayer->m_Money >= 1500)
			{
				m_pPlayer->MoneyTransaction(-1500, "-1.500 money. (bought 'rainbow')");
				SetExtra(RAINBOW, m_pPlayer->GetCID(), false, false, -1, true);
				GameServer()->SendChatTarget(m_pPlayer->GetCID(), "You bought rainbow until death.");
			}
			else
				GameServer()->SendChatTarget(m_pPlayer->GetCID(), "You don't have enough money! You need 1.500 money.");
		}
	}
	else if (ItemID == 2)
	{
		if (m_Bloody)
		{
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "You already own bloody.");
			return;
		}

		if (m_pPlayer->m_Level < 15)
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Your level is too low! You need to be level 15 to buy bloody.");
		else
		{
			if (m_pPlayer->m_Money >= 3500)
			{
				m_pPlayer->MoneyTransaction(-3500, "-3.500 money. (bought 'bloody')");
				SetExtra(BLOODY, m_pPlayer->GetCID(), false, false, -1, true);
				GameServer()->SendChatTarget(m_pPlayer->GetCID(), "You bought bloody until death.");
			}
			else
				GameServer()->SendChatTarget(m_pPlayer->GetCID(), "You don't have enough money! You need 3 500.");
		}
	}
	else if (ItemID == 3)
	{
		if (m_pPlayer->m_Level < 1)
		{
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Your level is too low! You need to be level 1 to buy the spooky ghost.");
			return;
		}
		else if (m_pPlayer->m_aHasItem[SPOOKY_GHOST])
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "You already have the spooky ghost.");
		else if (m_pPlayer->m_Money >= 1000000)
		{
			m_pPlayer->MoneyTransaction(-1000000, "-1000000 money. (bought 'spooky_ghost')");
			m_pPlayer->m_aHasItem[SPOOKY_GHOST] = true;
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "You bought the spooky ghost. For more infos check '/spookyghostinfo'.");
		}
		else
			GameServer()->SendChatTarget(m_pPlayer->GetCID(), "You don't have enough money!");
	}
	else
		GameServer()->SendChatTarget(m_pPlayer->GetCID(), "Invalid shop item. Choose another one.");
}