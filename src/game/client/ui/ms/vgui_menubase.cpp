//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Character creation menu
//
// $Workfile:     $
// $Date: 2004/11/07 01:06:28 $
//
//-----------------------------------------------------------------------------
// $Log: vgui_MainMenu.cpp,v $
// Revision 1.4  2004/11/07 01:06:28  dogg
// New Event Console
//
// Revision 1.3  2004/10/13 20:21:51  dogg
// Big update
// Netcode re-arranged
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

#include "msdllheaders.h"
#include "player/player.h"
#include "global.h"

#undef DLLEXPORT

#include <VGUI_Font.h>
//#include <VGUI_TextImage.h>

#include "../hud.h"
#include "../cl_util.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"
#include "../parsemsg.h"

#include "../vgui_int.h"
#include "../vgui_teamfortressviewport.h"

//Master Sword
#include "vgui_options.h"
#include "vgui_menudefsshared.h"

#include "vgui_menubase.h"
#include "vgui_menu_main.h"
#include "vgui_menu_interact.h"

//------------

float g_fMenuLastClosed = 0.0f;

CAction_SelectMainOption::CAction_SelectMainOption(class VGUI_MenuBase *pPanel, int iValue, msvariant Data)
{
	m_pPanel = pPanel;
	m_Value = iValue;
	m_Data = Data;
}
void CAction_SelectMainOption::actionPerformed(Panel *panel)
{
	m_pPanel->Select(m_Value, m_Data);
}

// Menu Dimensions
#define MAINWIN_SIZE_X XRES(120)
#define MAINWIN_SIZE_Y YRES(170)
#define MAINWIN_X XRES(320) - MAINWIN_SIZE_X / 2
#define MAINWIN_Y YRES(240) - MAINWIN_SIZE_Y / 2

#define BTN_SPACER_X XRES(15)
#define BTN_SPACER_Y YRES(10)
#define BTN_SIZE_X (MAINWIN_SIZE_X - BTN_SPACER_X * 2.0f)
#define BTN_SIZE_Y YRES(12)
#define BTN_X MAINWIN_SIZE_X / 2.0f - BTN_SIZE_X / 2.0f
#define BTN_START_Y YRES(50)

#define MAINMENU_FADETIME 0.5f //Thothie DEC2012_24 - reduce menu fade time

#define MAINLABEL_TOP_Y YRES(0)
int GetCenteredItemX(int WorkSpaceSizeX, int ItemSizeX, int Items, int SpaceBewteenItems);

// Creation
VGUI_MenuBase::VGUI_MenuBase(Panel *myParent) : CMenuPanel(255, 0, 0, 0, ScreenWidth, ScreenHeight)
{
	m_AllowKeys = true; // MiB NOV2014_25, block number shortcuts: NpcInteractMenus.rft
	setParent(myParent);
}

void VGUI_MenuBase::Init()
{
	startdbg;

	SetBits(m_Flags, MENUFLAG_TRAPNUMINPUT);

	m_pMainPanel = new CTransparentPanel(128, MAINWIN_X, MAINWIN_Y, MAINWIN_SIZE_X, MAINWIN_SIZE_Y);
	m_pMainPanel->setBorder(m_Border = new LineBorder(XRES(2), Color(Color_Border.r, Color_Border.g, Color_Border.b, Color_Border.a)));
	m_pMainPanel->setParent(this);

	MSLabel *pTitle = m_Title = new MSLabel(m_pMainPanel, "", 0, YRES(10), m_pMainPanel->getWide(), YRES(14));
	pTitle->setFont(g_FontTitle);
	pTitle->setContentAlignment(vgui::Label::a_center);
	pTitle->setFgColor(255, 255, 255, 0);
	pTitle->setText(Localized("#MAIN_MENU")); //Must set the text here, not at initializion
	int textw, texth;
	pTitle->getTextSize(textw, texth);
	int titlex = GetCenteredItemX(m_pMainPanel->getWide(), textw, 1, 0);

	// MiB NOV2014_25, center the title and separator NpcInteractMenus.rft [begin]
	// MiB 25NOV_2014, for centering the separator
	m_TitleSep = new CTransparentPanel(0, titlex, YRES(30), textw, YRES(3));
	m_TitleSep->setBorder(m_Spacer = new LineBorder(2, Color(0, 128, 0, 128)));
	m_TitleSep->setParent(m_pMainPanel);
	// MiB NOV2014_25, center the title and separator NpcInteractMenus.rft [end]

	//Orignal Code:
	/*
	CTransparentPanel *pSpacer = new CTransparentPanel( 0, titlex, YRES(30), textw, YRES(3) );
	pSpacer->setBorder( m_Spacer = new LineBorder( 2, Color(0, 128, 0, 128) ) );
	pSpacer->setParent( m_pMainPanel );
	*/

	enddbg;
}

MSButton *VGUI_MenuBase::AddButton(msstring_ref Name, int Width, msvariant ID)
{
	int w, h;
	g_FontSml->getTextSize(Name, w, h);
	MSButton *pButton = m_Buttons.add(new MSButton(m_pMainPanel, Name, (m_pMainPanel->getWide() / 2.0) - (w / 2.0), m_ButtonY, w, BTN_SIZE_Y, Color_BtnArmed, Color_BtnUnarmed));
	pButton->m_AutoFitText = true;
	pButton->setTextAlignment(Label::a_west);
	pButton->setContentAlignment(Label::a_center);
	pButton->addActionSignal(m_Actions.add(new CAction_SelectMainOption(this, m_Buttons.size() - 1, ID)));
	pButton->SetDisabledColor(Color_BtnDisabled);
	pButton->setText(Name);

	m_ButtonY += pButton->getTall() + BTN_SPACER_Y;

	return pButton;
}

void VGUI_MenuBase::Reset(void)
{
	m_pMainPanel->setVisible(true);
}

// Update
void VGUI_MenuBase::Update()
{
}

// Key inputs for the menu
bool VGUI_MenuBase::SlotInput(int iSlot)
{
	// MiB NOV2014_25, disable number shortcuts: NpcInteractMenus.rft
	if (iSlot < 0 || iSlot >= (signed)m_Buttons.size() || !m_AllowKeys || !m_Buttons[iSlot]->isEnabled())
		return false;

	//Original Code:
	/*
	if( iSlot < 0 || iSlot >= (signed)m_Buttons.size() )
		return false;
	*/

	m_Buttons[iSlot]->doClick();

	return true;
}

// Update the menu before opening it
void VGUI_MenuBase::Open(void)
{
	for (int i = 0; i < m_Buttons.size(); i++)
		m_Buttons[i]->setArmed(false);

	Reset();
	CMenuPanel::Open();
	m_OpenTime = gpGlobals->time;
	UpdateFade();
}

void VGUI_MenuBase::UpdateFade(void)
{
	float FadeTime = gpGlobals->time - m_OpenTime;
	FadeTime = V_max(V_min(FadeTime, MAINMENU_FADETIME), 0);
	m_FadeAmt = int(255 * FadeTime / MAINMENU_FADETIME);
	float InveserdFade = 255 - m_FadeAmt;

	Color color;

	m_pMainPanel->m_iTransparency = (InveserdFade / 2 + 128);

	m_Title->getFgColor(color);
	m_Title->setFgColor(color[0], color[1], color[2], InveserdFade);

	m_Border->getLineColor(color);
	m_Border->setLineColor(color[0], color[1], color[2], InveserdFade);

	m_Spacer->getLineColor(color);
	m_Spacer->setLineColor(color[0], color[1], color[2], InveserdFade);

	//m_pMainPanel->setBorder( NULL );
	//m_pMainPanel->setBorder( m_Border );

	for (int i = 0; i < m_Buttons.size(); i++)
	{
		m_Buttons[i]->m_ArmedColor.a = InveserdFade;
		m_Buttons[i]->m_UnArmedColor.a = InveserdFade;
		m_Buttons[i]->m_DisabledColor.a = InveserdFade;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void VGUI_MenuBase::Initialize(void)
{
	setVisible(false);
}

void __CmdFunc_ToggleMenu(void)
{
	if (gEngfuncs.Cmd_Argc() < 2 || !gViewPort)
		return;

	msstring MenuName = gEngfuncs.Cmd_Argv(1);

	if (MenuName == INTERACT_MENU_NAME)
	{
		VGUI_MainPanel *pPanel = VGUI::FindPanel(INTERACT_MENU_NAME);
		if (pPanel)
		{
			VGUI_MenuInteract *pInteractMenu = (VGUI_MenuInteract *)pPanel;
			pInteractMenu->QueryNPC();
		}
	}
	else
		VGUI::ToggleMenuVisible(MenuName);
}
