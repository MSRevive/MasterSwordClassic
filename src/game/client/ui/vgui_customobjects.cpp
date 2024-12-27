//=========== (C) Copyright 1996-2002 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Contains implementation of various VGUI-derived objects
//
// $Workfile:     $
// $Date: 2005/01/17 13:16:49 $
//
//-----------------------------------------------------------------------------
// $Log: vgui_CustomObjects.cpp,v $
// Revision 1.3  2005/01/17 13:16:49  dogg
// Brand new inventory VGUI, revised item system, Magic, Mp3 support and item storage
//
//
// $NoKeywords: $
//=============================================================================

#include "VGUI_Font.h"

#include "hud.h"
#include "cl_util.h"
#include "camera.h"
#include "kbutton.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"
#include "camera.h"
#include "in_defs.h"
#include "parsemsg.h"

#include "vgui_int.h"
#include "vgui_teamfortressviewport.h"
#include "vgui_loadtga.h"
#include "sharedutil.h"
#include "ms/vgui_mscontrols.h"

// Arrow filenames
char *sArrowFilenames[] =
	{
		"arrowup",
		"arrowdn",
		"arrowlt",
		"arrowrt",
};

// Get the name of TGA file, without a gamedir
char *GetTGANameForRes(const char *pszName)
{
	char sz[256];
	static char gd[256];

	int i = 640;
	if (ScreenWidth < 640)
		i = 320;

	snprintf(sz, sizeof(sz), pszName, i);
	snprintf(gd, sizeof(gd), "gfx/vgui/%s.tga", sz);

	return gd;
}

//-----------------------------------------------------------------------------
// Purpose: Loads a .tga file and returns a pointer to the VGUI tga object
//-----------------------------------------------------------------------------
BitmapTGA *LoadTGAForRes(const char *pImageName)
{
	// This is a super hacky way to prevent attempts to load null
	// or zero-length name files. The best solution would be to 
	// prevent such attempts, but alas... --thesupersoup, 2023/07/09
	if (!pImageName || strlen(pImageName) <= 0)
		return nullptr;

	BitmapTGA *pTGA = nullptr;

	char sz[256];
	_snprintf(sz, sizeof(sz), "%%d_%s", pImageName);
	pTGA = vgui_LoadTGA(GetTGANameForRes(sz));

	return pTGA;
}

//===========================================================
// All TFC Hud buttons are derived from this one.
CommandButton::CommandButton(const char *text, int x, int y, int wide, int tall, bool bNoHighlight) : Button("", x, y, wide, tall)
{
	m_iPlayerClass = 0;
	m_bNoHighlight = bNoHighlight;
	m_bFlat = false;
	Init();
	setText(text);
}

CommandButton::CommandButton(int iPlayerClass, const char *text, int x, int y, int wide, int tall, bool bFlat) : Button("", x, y, wide, tall)
{
	m_iPlayerClass = iPlayerClass;
	m_bNoHighlight = false;
	m_bFlat = bFlat;
	Init();
	setText(text);
}

CommandButton::CommandButton(const char *text, int x, int y, int wide, int tall, bool bNoHighlight, bool bFlat) : Button("", x, y, wide, tall)
{
	m_iPlayerClass = 0;
	m_bFlat = bFlat;
	m_bNoHighlight = bNoHighlight;
	Init();
	setText(text);
}

void CommandButton::Init(void)
{
	m_pSubMenu = NULL;
	m_pSubLabel = NULL;
	m_pParentMenu = NULL;

	// Set text color to orange
	setFgColor(Scheme::sc_primary1);

	// left align
	setContentAlignment(vgui::Label::a_west);

	// Add the Highlight signal
	if (!m_bNoHighlight)
		addInputSignal(new CHandler_CommandButtonHighlight(this));

	// not bound to any button yet
	m_cBoundKey = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Prepends the button text with the current bound key
//			if no bound key, then a clear space ' ' instead
//-----------------------------------------------------------------------------
void CommandButton::RecalculateText(void)
{
	/*char szBuf[128];

	if ( m_cBoundKey != 0 )
	{
		if ( m_cBoundKey == (char)255 )
		{
			 strncpy(szBuf,  m_sMainText, sizeof(szBuf) );
		}
		else
		{
			 _snprintf(szBuf, sizeof(szBuf),  "  %c  %s",  m_cBoundKey,  m_sMainText );
		}
		szBuf[MAX_BUTTON_SIZE-1] = 0;
	}
	else
	{
		// just draw a space if no key bound
		 _snprintf(szBuf, sizeof(szBuf),  "     %s",  m_sMainText );
		szBuf[MAX_BUTTON_SIZE-1] = 0;
	}*/

	Button::setText(m_sMainText);
}

void CommandButton::setText(const char *text)
{
	strncpy(m_sMainText, text, MAX_BUTTON_SIZE);
	m_sMainText[MAX_BUTTON_SIZE - 1] = 0;

	RecalculateText();
}

void CommandButton::setBoundKey(char boundKey)
{
	m_cBoundKey = boundKey;
	RecalculateText();
}

char CommandButton::getBoundKey(void)
{
	return m_cBoundKey;
}

void CommandButton::AddSubMenu(CCommandMenu *pNewMenu)
{
	m_pSubMenu = pNewMenu;

	// Prevent this button from being pushed
	setMouseClickEnabled(MOUSE_LEFT, false);
}

void CommandButton::UpdateSubMenus(int iAdjustment)
{
	if (m_pSubMenu)
		m_pSubMenu->RecalculatePositions(iAdjustment);
}

void CommandButton::paint()
{
	// Make the sub label paint the same as the button
	if (m_pSubLabel)
	{
		if (isSelected())
			m_pSubLabel->PushDown();
		else
			m_pSubLabel->PushUp();
	}

	// draw armed button text in white
	if (isArmed())
	{
		setFgColor(Scheme::sc_secondary2);
	}
	else
	{
		setFgColor(Scheme::sc_primary1);
	}

	Button::paint();
}

void CommandButton::paintBackground()
{
	if (m_bFlat)
	{
		if (isArmed())
		{
			// Orange Border
			drawSetColor(Scheme::sc_secondary1);
			drawOutlinedRect(0, 0, _size[0], _size[1]);
		}
	}
	else
	{
		if (isArmed())
		{
			// Orange highlight background
			drawSetColor(Scheme::sc_primary2);
			drawFilledRect(0, 0, _size[0], _size[1]);
		}

		// Orange Border
		drawSetColor(Scheme::sc_secondary1);
		drawOutlinedRect(0, 0, _size[0], _size[1]);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Highlights the current button, and all it's parent menus
//-----------------------------------------------------------------------------
void CommandButton::cursorEntered(void)
{
	// unarm all the other buttons in this menu
	CCommandMenu *containingMenu = getParentMenu();
	if (containingMenu)
	{
		containingMenu->ClearButtonsOfArmedState();

		// make all our higher buttons armed
		CCommandMenu *pCParent = containingMenu->GetParentMenu();
		if (pCParent)
		{
			CommandButton *pParentButton = pCParent->FindButtonWithSubmenu(containingMenu);

			pParentButton->cursorEntered();
		}
	}

	//Master Sword
	if (isEnabled())
		PlaySound("ui/buttonrollover.wav", 0.8f);

	// arm ourselves
	setArmed(true);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CommandButton::cursorExited(void)
{
	// only clear ourselves if we have do not have a containing menu
	// only stay armed if we have a sub menu
	// the buttons only unarm themselves when another button is armed instead
	if (!getParentMenu() || !GetSubMenu())
	{
		setArmed(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the command menu that the button is part of, if any
// Output : CCommandMenu *
//-----------------------------------------------------------------------------
CCommandMenu *CommandButton::getParentMenu(void)
{
	return m_pParentMenu;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the menu that contains this button
// Input  : *pParentMenu -
//-----------------------------------------------------------------------------
void CommandButton::setParentMenu(CCommandMenu *pParentMenu)
{
	m_pParentMenu = pParentMenu;
}

//===========================================================
int ClassButton::IsNotValid()
{
	// If this is the main ChangeClass button, remove it if the player's only able to be civilians
	if (m_iPlayerClass == -1)
	{
		if (gViewPort->GetValidClasses(g_iTeamNumber) == -1)
			return true;

		return false;
	}

	// Is it an illegal class?
	if ((gViewPort->GetValidClasses(0) & sTFValidClassInts[m_iPlayerClass]) || (gViewPort->GetValidClasses(g_iTeamNumber) & sTFValidClassInts[m_iPlayerClass]))
		return true;

	// Only check current class if they've got autokill on
	bool bAutoKill = CVAR_GET_FLOAT("hud_classautokill") != 0;
	if (bAutoKill)
	{
		// Is it the player's current class?
		if ((gViewPort->IsRandomPC() && m_iPlayerClass == PC_RANDOM) || (!gViewPort->IsRandomPC() && (m_iPlayerClass == g_iPlayerClass)))
			return true;
	}

	return false;
}

//===========================================================
// Button with Class image beneath it
CImageLabel::CImageLabel(const char *pImageName, int x, int y) : Label("", x, y)
{
	setContentFitted(true);
	m_pTGA = LoadTGAForRes(pImageName);
	setImage(m_pTGA);
}

CImageLabel::CImageLabel(const char *pImageName, int x, int y, int wide, int tall) : Label("", x, y, wide, tall)
{
	setContentFitted(true);
	m_pTGA = LoadTGAForRes(pImageName);
	setImage(m_pTGA);
}

//===========================================================
// Image size
int CImageLabel::getImageWide(void)
{
	if (m_pTGA)
	{
		int iXSize, iYSize;
		m_pTGA->getSize(iXSize, iYSize);
		return iXSize;
	}
	else
	{
		return 1;
	}
}

int CImageLabel::getImageTall(void)
{
	if (m_pTGA)
	{
		int iXSize, iYSize;
		m_pTGA->getSize(iXSize, iYSize);
		return iYSize;
	}
	else
	{
		return 1;
	}
}

void CImageLabel::LoadImage(const char *pImageName)
{
	if (m_pTGA)
		delete m_pTGA;

	// Load the Image
	//m_pTGA = LoadTGAForRes(pImageName);
	m_pTGA = MSBitmap::GetTGA(pImageName);

	if (m_pTGA == NULL)
	{
		// we didn't find a matching image file for this resolution
		// try to load file resolution independent

		char sz[256];
		 _snprintf(sz, sizeof(sz), "%s/%s", gEngfuncs.pfnGetGameDirectory(), pImageName);
		FileInputStream fis(sz, false);
		m_pTGA = new BitmapTGA(&fis, true);
		fis.close();
	}

	if (m_pTGA == NULL)
		return; // unable to load image

	int w, t;

	m_pTGA->getSize(w, t);

	setSize(XRES(w), YRES(t));
	setImage(m_pTGA);
}

//===========================================================
// Various overloaded paint functions for Custom VGUI objects
void CCommandMenu::paintBackground()
{
	// Transparent black background

	if (m_iSpectCmdMenu)
		drawSetColor(0, 0, 0, 64);
	else
		drawSetColor(Scheme::sc_primary3);

	drawFilledRect(0, 0, _size[0], _size[1]);
}

//=================================================================================
// CUSTOM SCROLLPANEL
//=================================================================================

CTFScrollButton::CTFScrollButton(int iArrow, const char *text, int x, int y, int wide, int tall) : CommandButton(text, x, y, wide, tall)
{
	// Set text color to orange
	setFgColor(Scheme::sc_primary1);

	// Load in the arrow
	m_pTGA = MSBitmap::GetTGA(sArrowFilenames[iArrow]);
	setImage(m_pTGA);

	// Highlight signal
	InputSignal *pISignal = new CHandler_CommandButtonHighlight(this);
	addInputSignal(pISignal);
}

void CTFScrollButton::paint(void)
{
	if (!m_pTGA)
		return;

	// draw armed button text in white
	if (isArmed())
	{
		m_pTGA->setColor(Color(255, 255, 255, 0));
	}
	else
	{
		m_pTGA->setColor(Color(255, 255, 255, 128));
	}

	m_pTGA->doPaint(this);
}

void CTFScrollButton::paintBackground(void)
{
	/*
	if ( isArmed() )
	{
		// Orange highlight background
		drawSetColor( Scheme::sc_primary2 );
		drawFilledRect(0,0,_size[0],_size[1]);
	}

	// Orange Border
	drawSetColor( Scheme::sc_secondary1 );
	drawOutlinedRect(0,0,_size[0]-1,_size[1]);
*/
}

void CTFSlider::paintBackground(void)
{
	int wide, tall, nobx, noby;
	getPaintSize(wide, tall);
	getNobPos(nobx, noby);

	// Border
	drawSetColor(Scheme::sc_secondary1);
	drawOutlinedRect(0, 0, wide, tall);

	if (isVertical())
	{
		// Nob Fill
		drawSetColor(Scheme::sc_primary2);
		drawFilledRect(0, nobx, wide, noby);

		// Nob Outline
		drawSetColor(Scheme::sc_primary1);
		drawOutlinedRect(0, nobx, wide, noby);
	}
	else
	{
		// Nob Fill
		drawSetColor(Scheme::sc_primary2);
		drawFilledRect(nobx, 0, noby, tall);

		// Nob Outline
		drawSetColor(Scheme::sc_primary1);
		drawOutlinedRect(nobx, 0, noby, tall);
	}
}

CTFScrollPanel::CTFScrollPanel(int x, int y, int wide, int tall) : ScrollPanel(x, y, wide, tall)
{
	ScrollBar *pScrollBar = getVerticalScrollBar();
	pScrollBar->setButton(new CTFScrollButton(ARROW_UP, "", 0, 0, 16, 16), 0);
	pScrollBar->setButton(new CTFScrollButton(ARROW_DOWN, "", 0, 0, 16, 16), 1);
	pScrollBar->setSlider(new CTFSlider(0, wide - 1, wide, (tall - (wide * 2)) + 2, true));
	pScrollBar->setPaintBorderEnabled(false);
	pScrollBar->setPaintBackgroundEnabled(false);
	pScrollBar->setPaintEnabled(false);

	pScrollBar = getHorizontalScrollBar();
	pScrollBar->setButton(new CTFScrollButton(ARROW_LEFT, "", 0, 0, 16, 16), 0);
	pScrollBar->setButton(new CTFScrollButton(ARROW_RIGHT, "", 0, 0, 16, 16), 1);
	pScrollBar->setSlider(new CTFSlider(tall, 0, wide - (tall * 2), tall, false));
	pScrollBar->setPaintBorderEnabled(false);
	pScrollBar->setPaintBackgroundEnabled(false);
	pScrollBar->setPaintEnabled(false);
}

//=================================================================================
// CUSTOM HANDLERS
//=================================================================================
void CHandler_MenuButtonOver::cursorEntered(Panel *panel)
{
	if (gViewPort && m_pMenuPanel)
	{
		m_pMenuPanel->SetActiveInfo(m_iButton);
	}
}

void CMenuHandler_StringCommandClassSelect::actionPerformed(Panel *panel)
{
	CMenuHandler_StringCommand::actionPerformed(panel);

	bool bAutoKill = CVAR_GET_FLOAT("hud_classautokill") != 0;
	if (bAutoKill && g_iPlayerClass != 0)
		gEngfuncs.pfnClientCmd("kill");
}
