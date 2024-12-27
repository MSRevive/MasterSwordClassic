//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: NPC Store sell menu
//
// $Workfile:     $
// $Date: 2005/01/17 13:16:49 $
//
//-----------------------------------------------------------------------------
// $Log: vgui_StoreSell.cpp,v $
// Revision 1.5  2005/01/17 13:16:49  dogg
// Brand new inventory VGUI, revised item system, Magic, Mp3 support and item storage
//
// Revision 1.4  2004/11/07 01:06:28  dogg
// New Event Console
//
// Revision 1.3  2004/09/18 10:41:53  dogg
// Magic update
//
// Revision 1.2  2004/09/11 22:21:03  dogg
// fixed the <break> problems in cl_dll/MasterSword here!
//
// Revision 1.1  2004/09/07 17:06:01  reddog
// First commit! ;-)
//
//
// $NoKeywords: $
//=============================================================================

#include "inc_weapondefs.h"
#undef DLLEXPORT

#include <VGUI_Font.h>
#include <VGUI_TextImage.h>

#include "../hud.h"
#include "../cl_util.h"
#include "../camera.h"
#include "../kbutton.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"
#include "../camera.h"
#include "../in_defs.h"
#include "../parsemsg.h"

#include "../vgui_int.h"
#include "../vgui_teamfortressviewport.h"

//Master Sword
#include "vgui_storesell.h"

class CAction_Sell : public ActionSignal
{
protected:
	CStoreSellPanel *m_Panel;

public:
	CAction_Sell(CStoreSellPanel *pPanel) { m_Panel = pPanel; }
	virtual void actionPerformed(Panel *panel) { m_Panel->SellAll(); }
};

//------------

// Creation
CStoreSellPanel::CStoreSellPanel(Panel *pParent) : CStorePanel()
{
	setParent(pParent);

	m_pSubtitle->setText(Localized("#SELL_SUBTITLE"));

	m_ActButton->setText(Localized("#SELL"));
	m_ActButton->addActionSignal(new CAction_Sell(this));
}

bool CStoreSellPanel::InterestedInItem(msstring_ref pszItemName)
{
	for (int i = 0; i < CStorePanel::StoreItems.size(); i++)
		if (CStorePanel::StoreItems[i].Name == pszItemName)
			return true;

	return false;
}

//Item selected
void CStoreSellPanel::ItemCreated(void *pData)
{
	containeritem_t &Item = *(containeritem_t *)pData;
	Item.Disabled = !InterestedInItem(Item.Name); //Disabled, if vendor isn't interested in this item
	m_AllowUpdate = true;
}

void CStoreSellPanel::ItemHighlighted(void *pData)
{
	CStorePanel::ItemHighlighted(pData);

	VGUI_ItemButton &ItemButton = *(VGUI_ItemButton *)pData;
	if (ItemButton.m_Highlighted)
	{
		m_InfoPanel->m_SaleText->setText("Worthless");

		containeritem_t &Item = ItemButton.m_Data;
		for (int s = 0; s < CStorePanel::StoreItems.size(); s++)
		{
			storeitem_t &StoreItem = CStorePanel::StoreItems[s];
			if (Item.Name != StoreItem.Name)
				continue;

			int Value = int(StoreItem.iCost * StoreItem.flSellRatio);
			char cTemp[256];
			 _snprintf(cTemp, sizeof(cTemp),  Localized("#SELL_ITEM_VALUE"),  Value );
			m_InfoPanel->m_SaleText->setText(cTemp);
			break;
		}
	}
}

//Item selected
void CStoreSellPanel::ItemSelectChanged(ulong ID, bool fSelected)
{
	m_SelectedItems.clear();

	int Valuetotal = 0;

	for (int g = 0; g < m_GearPanel->GearItemButtonTotal; g++)
	{
		VGUI_Inv_GearItem &GearItem = *m_GearPanel->GearItemButtons[g];
		for (int i = 0; i < GearItem.m_ItemContainer->m_ItemButtonTotal; i++)
		{
			VGUI_ItemButton &ItemButton = *GearItem.m_ItemContainer->m_ItemButtons[i];
			if (!ItemButton.m_Selected)
				continue;

			containeritem_t &Item = ItemButton.m_Data;
			for (int s = 0; s < CStorePanel::StoreItems.size(); s++)
			{
				storeitem_t &StoreItem = CStorePanel::StoreItems[s];
				if (Item.Name != StoreItem.Name)
					continue;

				m_SelectedItems.push_back(ItemButton.m_Data);
				Valuetotal += int(StoreItem.iCost * StoreItem.flSellRatio);
			}
		}
	}

	m_SaleLabel->setText(msstring("Selling ") + (int)m_SelectedItems.size() + " items for " + Valuetotal + " gold");
}

//Sell button pressed
void CStoreSellPanel::SellAll()
{
	msstring CommandString = "trade stop"; //If no items selected

	if (m_SelectedItems.size() > 0)
	{
		CommandString = "trade sell";
		for (int i = 0; i < m_SelectedItems.size(); i++)
		{
			CommandString += " ";
			CommandString += (int)m_SelectedItems[i].ID;
		}
		CommandString += "\n";
	}

	ClientCmd(CommandString);
}

void SellWindow_Update()
{
	if (gViewPort && gViewPort->m_pStoreSellMenu)
	{
		gViewPort->m_pStoreSellMenu->m_AllowUpdate = true;
		gViewPort->m_pStoreSellMenu->Update();
	}
}
