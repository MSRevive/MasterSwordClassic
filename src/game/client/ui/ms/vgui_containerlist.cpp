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
// $Log: vgui_ContainerList.cpp,v $
// Revision 1.7  2005/01/17 13:16:49  dogg
// Brand new inventory VGUI, revised item system, Magic, Mp3 support and item storage
//
// Revision 1.6  2004/11/07 01:06:28  dogg
// New Event Console
//
// Revision 1.5  2004/10/14 12:15:31  dogg
// no message
//
// Revision 1.4  2004/09/14 07:49:43  dogg
// no message
//
// Revision 1.3  2004/09/12 00:12:00  dogg
// no message
//
//
// $NoKeywords: $
//=============================================================================

//A panel containing items, complete with name, picture and highlighting.
//Used for inventory, trade and storage screens

#include "inc_weapondefs.h"
#undef DLLEXPORT

#include "hud.h"
#include "cl_util.h"

#include "vgui_teamfortressviewport.h"

//Master Sword
#include "vgui_containerlist.h"
#include "logger.h"
#include "clglobal.h"
#include "vgui_menudefsshared.h"

//#define LOG_EXTRA

#ifdef LOG_EXTRA
#define logfileopt logfile
#else
#define logfileopt NullFile
#endif

bool fBlockVGUIMouseButton1 = false;
void ContainerWindowUpdate();

class CContainerInputHandler : public InputSignal
{
public:
	virtual void mouseReleased(MouseCode code, Panel *panel)
	{
		if (code == MOUSE_LEFT)
			fBlockVGUIMouseButton1 = false;
	}

	virtual void cursorMoved(int x, int y, Panel *panel){};
	virtual void cursorEntered(Panel *panel){};
	virtual void cursorExited(Panel *Panel){};
	virtual void mousePressed(MouseCode code, Panel *panel) {}
	virtual void mouseDoublePressed(MouseCode code, Panel *panel) {}
	virtual void mouseWheeled(int delta, Panel *panel){};
	virtual void keyPressed(KeyCode code, Panel *panel){};
	virtual void keyTyped(KeyCode code, Panel *panel){};
	virtual void keyReleased(KeyCode code, Panel *panel){};
	virtual void keyFocusTicked(Panel *panel){};
};

class CAction_RemoveGear : public ActionSignal
{
protected:
	CContainerPanel *m_Panel;

public:
	CAction_RemoveGear(CContainerPanel *pPanel) { m_Panel = pPanel; }
	virtual void actionPerformed(Panel *panel) { m_Panel->RemoveGear(); }
};

//------------

// Menu Dimensions
#define CONTAINER_BUTTON_SPACER_Y ((LABEL_ITEMNAME_SIZE_Y + LABEL_ITEM_SPACER_Y) * CONTAINER_INFO_LABELS + YRES(6))
#define HANDBUTTON_SPACER_X XRES(8)
#define HANDBUTTON_X ((CLASSMENU_WINDOW_X + CLASSMENU_WINDOW_SIZE_X) - GOLDLABEL_SIZE_X - (GOLDLABEL_SIZE_X + HANDBUTTON_SPACER_X))
#define BOTTOM_BUTTON_SPACER_Y YRES(4)
#define CHECKBOX_Y MainPanelY + ih + BOTTOM_BUTTON_SPACER_Y + YRES(2)

// Creation
CContainerPanel::CContainerPanel(int iTrans, int iRemoveMe, int x, int y, int wide, int tall) : VGUI_ContainerPanel()
{
	m_pCancelButton->setText(Localized("#CLOSE"));
	m_ActButton->addActionSignal(new CAction_RemoveGear(this));
	//m_ActButton->setVisible( false );
}
void CContainerPanel::UpdateSubtitle()
{
	mslist<VGUI_ItemButton *> SelectedItems;
	GetSelectedItems(SelectedItems);

	VGUI_Inv_GearItem &GearButton = *m_GearPanel->GearItemButtons[m_GearPanel->m_Selected];
	m_ActButton->setText(Localized("#REMOVE"));
	if (!m_GearPanel->m_Selected || SelectedItems.size() > 0)
	{
		m_pSubtitle->setText("Click container to move selected item, or click again to equip");
		m_ActButton->setEnabled(false);
		//if( SelectedItems.size() == 1 )
		//	m_ActButton->setText( Localized("#USE") );
	}
	else
	{
		m_pSubtitle->setText(m_Text_DoubleClick);
		CGenericItem *pGearItem = player.GetGearItem(GearButton.m_GearItemID);
		if (pGearItem)
		{
			if (!FBitSet(pGearItem->MSProperties(), ITEM_CONTAINER))
				m_pSubtitle->setText("Remove wearable item");
		}
		m_ActButton->setEnabled(true);
	}

	GearButton.m_ItemContainer->setVisible(GearButton.m_GearItem.IsContainer);
}
void CContainerPanel::RemoveGear()
{
	msstring Cmd = msstring("remove ") + m_GearPanel->GearItemButtons[m_GearPanel->m_Selected]->m_GearItemID + "\n";
	ServerCmd(Cmd);
	gViewPort->HideTopMenu();
}
void CContainerPanel::ItemSelectChanged(ulong ID, bool fSelected)
{
	UpdateSubtitle();
}
void CContainerPanel::ItemDoubleclicked(ulong ID)
{
	if (m_GearPanel->m_Selected == 0)
		return;

	UnSelectAllItems();

	CGenericItem *pItem = MSUtil_GetItemByID(ID, &player);
	if (!pItem)
	{
		//MSErrorConsoleText( "CContainerPanel::ItemSelected", msstring("Container Item not found (") + idx + ")" );
		return;
	}

	//if( pItem->GiveTo( &player, false, false ) )
	//	{
	char sz[32];
	 _snprintf(sz, sizeof(sz),  "inv transfer %u 0",  pItem->m_iId );
	gEngfuncs.pfnClientCmd(sz);

	gViewPort->HideTopMenu();
	//	}
}
void CContainerPanel::GearItemSelected(ulong ID)
{
	//Unselect all items
	UnSelectAllItems();

	//Set the title to the name of the container
	msstring Title = "Inventory";

	if (ID > 0)
	{
		CGenericItem *pGearItem = player.GetGearItem(ID);
		if (pGearItem)
			Title = pGearItem->DisplayName();
	}
	else if (!ID)
		Title = "Player hands";

	m_pTitle->setText(Title);
	UpdateSubtitle();
}
bool CContainerPanel::GearItemClicked(ulong ID)
{
	mslist<VGUI_ItemButton *> SelectedItems;
	GetSelectedItems(SelectedItems);

	if (!SelectedItems.size())
		return false;

	ulong ContainerID = ID;
	CGenericItem *pContainer = player.GetContainer(ID);

	if (ID && !pContainer)
		return false;

	//Extract the IDs of the selected items.  I can't keep the list of selected items because the itembuttons get
	//modified when I start moving items around
	/*mslist<ulong> SelectedIDs;
	 for (int i = 0; i < SelectedItems.size(); i++) 
	{
		SelectedIDs.push_back( SelectedItems[i]->m_Data.ID );
		SelectedItems[i]->Select( false );
	}*/

	for (int i = 0; i < SelectedItems.size(); i++)
	{
		CGenericItem *pItem = MSUtil_GetItemByID(SelectedItems[i]->m_Data.ID, &player);
		if (!pItem)
			continue;

		//Move the item to the pack
		//if( player.PutInPack( pItem, pContainer, true ) )
		//{
		//Send the command
		msstring Command = msstring("inv transfer ") + (int)pItem->m_iId + " " + (int)ContainerID + "\n";
		gEngfuncs.pfnClientCmd(Command);
		//}
	}

	return true;
}
bool CContainerPanel::GearItemDoubleClicked(ulong ID)
{
	CGenericItem *pWornItem = player.GetGearItem(ID);
	if (!pWornItem)
		return false;

	if (!pWornItem->IsWorn())
		return false;

	m_ActButton->doClick(); //Remove the item

	return true;
}
// Update
void CContainerPanel::Close(void)
{
	player.ClearConditions(MONSTER_OPENCONTAINER);
	ClientCmd("inv stop");
	VGUI_ContainerPanel::Close();
}

// MIB FEB2015_21 [INV_SCROLL] - Pass to the gear panel
void CContainerPanel::StepInput(bool bDirUp)
{
	if (m_GearPanel)
		m_GearPanel->StepInput(bDirUp);
}

//======================================
// Update the Class menu before opening it
void CContainerPanel::Open(void)
{
	//If the button is down, block until its released
	bool fButtonDown = false;
#ifdef _WIN32
	int vKey = VK_LBUTTON;
	if (GetSystemMetrics(SM_SWAPBUTTON))
		vKey = VK_RBUTTON;
	if (HIWORD(GetAsyncKeyState(vKey)))
		fBlockVGUIMouseButton1 = true;
	else
#endif
		fBlockVGUIMouseButton1 = false;

	VGUI_ContainerPanel::Open();

	//Select the open pack
	for (int i = 0; i < m_GearPanel->GearItemButtonTotal; i++)
	{
		if (player.HasConditions(MONSTER_OPENCONTAINER))
		{
			if (m_OpenContainerID == m_GearPanel->GearItemButtons[i]->m_GearItemID)
			{
				m_GearPanel->Select(i);
				break;
			}
		}
		else
		{
			m_GearPanel->Select(i);
			break;
		} //If no specific containter open, just select the first one - Player hands
	}
}

void ContainerWindowOpen(ulong ContainerID)
{
	if (!gViewPort || !gViewPort->m_pContainerMenu)
		return;

	gViewPort->m_pContainerMenu->m_OpenContainerID = ContainerID;
	ShowVGUIMenu(MENU_CONTAINER);
}
void ContainerWindowUpdate()
{
	if (!gViewPort || !gViewPort->m_pContainerMenu)
		return;

	gViewPort->m_pContainerMenu->m_AllowUpdate = true;
	gViewPort->m_pContainerMenu->Update();
}
void ContainerWindowClose()
{
	gViewPort->HideTopMenu();
}
