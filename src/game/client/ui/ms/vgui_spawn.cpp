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
// $Date: 2005/01/17 13:16:49 $
//
//-----------------------------------------------------------------------------
// $Log: vgui_Spawn.cpp,v $
// Revision 1.6  2005/01/17 13:16:49  dogg
// Brand new inventory VGUI, revised item system, Magic, Mp3 support and item storage
//
// Revision 1.5  2004/11/07 01:06:28  dogg
// New Event Console
//
// Revision 1.4  2004/10/19 23:14:54  dogg
// BIG update
// Memory leak fixes
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
#include "stats/statdefs.h"

#undef DLLEXPORT

#include "VGUI_Font.h"
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
#include "vgui_spawn.h"
#include "hudid.h"
#include "health.h"
//------------

// Menu Dimensions
#define MAINWIN_X XRES(40)
#define MAINWIN_Y YRES(32)
#define TITLE_X MAINWIN_X
#define TITLE_Y MAINWIN_Y
#define SUBTITLE_X TITLE_X + XRES(10)
#define SUBTITLE_Y TITLE_Y + YRES(40)
#define MSG_W XRES(200)
#define MSG_H YRES(60)

// Creation
CSpawnPanel::CSpawnPanel(Panel *pParent) : CMenuPanel(1, false, 0, 0, ScreenWidth, ScreenHeight)
{
	startdbg;
	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle("Title Font");
	SchemeHandle_t hClassWindowText = pSchemes->getSchemeHandle("Briefing Text");

	// color schemes
	int r, g, b, a;
	pSchemes->getFgColor(hClassWindowText, r, g, b, a);

	//m_pScrollPanel->setPos( 0, 0 );
	//m_pScrollPanel->setScrollBarAutoVisible(false,false);
	setParent(pParent);

	m_pTitle = new TransLabel("Loading...", TITLE_X, TITLE_Y, this);
	m_pTitle->setFont(pSchemes->getFont(hTitleScheme));
	pSchemes->getFgColor(hTitleScheme, r, g, b, a);
	m_pTitle->setFgColor(r, g, b, a);

	m_pNameLabel = new TransLabel("", SUBTITLE_X, SUBTITLE_Y, this);
	m_pNameLabel->setFont(pSchemes->getFont(hClassWindowText));
	pSchemes->getFgColor(hClassWindowText, r, g, b, a);
	m_pNameLabel->setFgColor(r, g, b, a);

	m_Message = new TextPanel("", SUBTITLE_X, SUBTITLE_Y + YRES(40), MSG_W, MSG_H);
	m_Message->setParent(this);
	m_Message->setFont(pSchemes->getFont(hClassWindowText));
	m_Message->setText("The server may not be correctly configured.");
	pSchemes->getBgColor(hTitleScheme, r, g, b, a);
	m_Message->setBgColor(r, g, b, a);
	m_Message->setFgColor(255, 0, 0, 0);
	m_Message->setVisible(false);

	pStatus = new CStatusBar(this, XRES(20), YRES(70), XRES(120), YRES(15));
	enddbg;
}

// Update
void CSpawnPanel::Update()
{
	return;
}

void CSpawnPanel::Close(void)
{
	CMenuPanel::Close();
	ClearBits(gHUD.m_iHideHUDDisplay, HIDEHUD_ALL);
}
//======================================
// Key inputs for the Class Menu
bool CSpawnPanel::SlotInput(int iSlot)
{
	return false;
}

//======================================
// Update the menu before opening it
void CSpawnPanel::Open(void)
{
	Update();
	pStatus->setVisible(false);
	SetBits(gHUD.m_iHideHUDDisplay, HIDEHUD_ALL);
	CMenuPanel::Open();
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void CSpawnPanel::Initialize(void)
{
	setVisible(false);
	//m_pScrollPanel->setScrollValue( 0, 0 );
}

//======================================
// Mouse is over a button, highlight it
void CSpawnPanel::SetActiveInfo(int iInput)
{
}

void UpdateStatusVGUI()
{
	if (gViewPort && gViewPort->m_pSpawnScreen)
		gViewPort->m_pSpawnScreen->Update();
}
void UpdateRecvScriptStatus(float Percentage)
{
	if (gViewPort && gViewPort->m_pSpawnScreen)
	{
		gViewPort->m_pSpawnScreen->pStatus->setVisible(true);
		gViewPort->m_pSpawnScreen->pStatus->Percentage = Percentage;
		gViewPort->m_pSpawnScreen->Update();
	}
}
