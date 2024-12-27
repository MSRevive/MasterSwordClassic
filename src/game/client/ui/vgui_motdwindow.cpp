//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:
//
// $Workfile:     $
// $Date: 2004/09/08 03:20:54 $
//
//-----------------------------------------------------------------------------
// $Log: vgui_MOTDWindow.cpp,v $
// Revision 1.2  2004/09/08 03:20:54  dogg
// no message
//
//
// $NoKeywords: $
//=============================================================================

#include "VGUI_Font.h"
#include "VGUI_ScrollPanel.h"
#include "VGUI_TextImage.h"

#include <VGUI_StackLayout.h>

#include "hud.h"
#include "cl_util.h"
#include "camera.h"
#include "kbutton.h"
#include "const.h"

#include "vgui_int.h"
#include "vgui_teamfortressviewport.h"

#define MOTD_TITLE_X XRES(16)
#define MOTD_TITLE_Y YRES(16)

#define MOTD_WINDOW_X XRES(112)
#define MOTD_WINDOW_Y YRES(80)
#define MOTD_WINDOW_SIZE_X XRES(424)
#define MOTD_WINDOW_SIZE_Y YRES(312)

//-----------------------------------------------------------------------------
// Purpose: Displays the MOTD and basic server information
//-----------------------------------------------------------------------------
class CMessageWindowPanel : public CMenuPanel
{
public:
	CMessageWindowPanel(const char *szMOTD, const char *szTitle, int iShadeFullScreen, int iRemoveMe, int x, int y, int wide, int tall);

private:
	CTransparentPanel *m_pBackgroundPanel;
};

//-----------------------------------------------------------------------------
// Purpose: Creates a new CMessageWindowPanel
// Output : CMenuPanel - interface to the panel
//-----------------------------------------------------------------------------
CMenuPanel *CMessageWindowPanel_Create(const char *szMOTD, const char *szTitle, int iShadeFullscreen, int iRemoveMe, int x, int y, int wide, int tall)
{
	return new CMessageWindowPanel(szMOTD, szTitle, iShadeFullscreen, iRemoveMe, x, y, wide, tall);
}

//-----------------------------------------------------------------------------
// Purpose: Constructs a message panel
//-----------------------------------------------------------------------------
CMessageWindowPanel::CMessageWindowPanel(const char *szMOTD, const char *szTitle, int iShadeFullscreen, int iRemoveMe, int x, int y, int wide, int tall) : CMenuPanel(iShadeFullscreen ? 100 : 255, iRemoveMe, x, y, wide, tall)
{
	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle("Title Font");
	SchemeHandle_t hMOTDText = pSchemes->getSchemeHandle("Briefing Text");

	// color schemes
	int r, g, b, a;

	// Create the window
	m_pBackgroundPanel = new CTransparentPanel(iShadeFullscreen ? 255 : 100, MOTD_WINDOW_X, MOTD_WINDOW_Y, MOTD_WINDOW_SIZE_X, MOTD_WINDOW_SIZE_Y);
	m_pBackgroundPanel->setParent(this);
	m_pBackgroundPanel->setBorder(new LineBorder(Color(255 * 0.7, 170 * 0.7, 0, 0)));
	m_pBackgroundPanel->setVisible(true);

	int iXSize, iYSize, iXPos, iYPos;
	m_pBackgroundPanel->getPos(iXPos, iYPos);
	m_pBackgroundPanel->getSize(iXSize, iYSize);

	// Create the title
	Label *pLabel = new Label("", iXPos + MOTD_TITLE_X, iYPos + MOTD_TITLE_Y);
	pLabel->setParent(this);
	pLabel->setFont(pSchemes->getFont(hTitleScheme));
	pLabel->setFont(Scheme::sf_primary1);

	pSchemes->getFgColor(hTitleScheme, r, g, b, a);
	pLabel->setFgColor(r, g, b, a);
	pLabel->setFgColor(Scheme::sc_primary1);
	pSchemes->getBgColor(hTitleScheme, r, g, b, a);
	pLabel->setBgColor(r, g, b, a);
	pLabel->setContentAlignment(vgui::Label::a_west);
	pLabel->setText(szTitle);

	// Create the Scroll panel
	ScrollPanel *pScrollPanel = new CTFScrollPanel(iXPos + XRES(16), iYPos + MOTD_TITLE_Y * 2 + YRES(16), iXSize - XRES(32), iYSize - (YRES(48) + BUTTON_SIZE_Y * 2));
	pScrollPanel->setParent(this);
	//force the scrollbars on so clientClip will take them in account after the validate
	pScrollPanel->setScrollBarAutoVisible(false, false);
	pScrollPanel->setScrollBarVisible(true, true);
	pScrollPanel->validate();

	// Create the text panel
	TextPanel *pText = new TextPanel("", 0, 0, 64, 64);
	pText->setParent(pScrollPanel->getClient());

	// get the font and colors from the scheme
	pText->setFont(pSchemes->getFont(hMOTDText));
	pSchemes->getFgColor(hMOTDText, r, g, b, a);
	pText->setFgColor(r, g, b, a);
	pSchemes->getBgColor(hMOTDText, r, g, b, a);
	pText->setBgColor(r, g, b, a);
	pText->setText(szMOTD);
	pText->setSize(pScrollPanel->getClientClip()->getWide() - 2, 5000);

	// Get the total size of the MOTD text and resize the text panel
	int iScrollSizeX, iScrollSizeY;
	pText->getTextImage()->getTextSizeWrapped(iScrollSizeX, iScrollSizeY);
	pText->setSize(iScrollSizeX, iScrollSizeY);
	//pText->setBorder(new LineBorder());

	//turn the scrollbars back into automode
	pScrollPanel->setScrollBarAutoVisible(true, true);
	pScrollPanel->setScrollBarVisible(false, false);

	pScrollPanel->validate();

	//Center button along X axis
	int ix = iXPos + (iXSize / 2) - (CMENU_SIZE_X / 2);

	CommandButton *pButton = new CommandButton(CHudTextMessage::BufferedLocaliseTextString("#Menu_OK"),
											   ix, iYPos + iYSize - YRES(16) - BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y);
	pButton->addActionSignal(new CMenuHandler_TextWindow(HIDE_TEXTWINDOW));
	pButton->setParent(this);
}
