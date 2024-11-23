//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: TFC Class Menu 
//
// $Workfile:     $
// $Date: 2005/01/17 13:16:49 $
//
//-----------------------------------------------------------------------------
// $Log: vgui_Store.cpp,v $
// Revision 1.3  2005/01/17 13:16:49  dogg
// Brand new inventory VGUI, revised item system, Magic, Mp3 support and item storage
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

#include "../hud.h"
#include "../cl_util.h"
#include "../parsemsg.h"
#include "../vgui_teamfortressviewport.h"
#include <VGUI_TextImage.h>

//Master Sword
#include "sharedutil.h"
#include "vgui_storemainwin.h"
#include "vgui_menudefsshared.h"

#define STORE_BUTTONS 3

struct storetext_t
{
	char *ButtonTitle,
		 *Title,
		 *Desc;
}
g_StoreText[] =
{
	"#STORE_BTN_BUY", "#BUY", "#STORE_DESC_BUY",
	"#STORE_BTN_SELL", "#SELL", "#STORE_DESC_SELL",
	"#STORE_BTN_CANCEL", "#CANCEL", "#STORE_DESC_CANCEL",
};

int iMenuToShow[] =
{
	MENU_STOREBUY,
	MENU_STORESELL
};

//Menu dimensions
#define STORE_WINDOW_X					XRES(0)
#define STORE_WINDOW_Y					YRES(80)
#define STORE_WINDOW_SIZE_X				XRES(424)
#define STORE_WINDOW_SIZE_Y				YRES(312)

#define STORE_SIDEBUTTON_TOPLEFT_X		XRES(40)
#define STORE_SIDEBUTTON_TOPLEFT_Y		YRES(80)
#define STORE_SIDEBUTTON_SIZE_X			XRES(124)
#define STORE_SIDEBUTTON_SIZE_Y			YRES(24)
#define STORE_SIDEBUTTON_SPACER_Y		YRES(8)

#define STORE_SCROLLPANEL_X				STORE_SIDEBUTTON_TOPLEFT_X + STORE_SIDEBUTTON_SIZE_X + XRES(12)
#define STORE_SCROLLPANEL_SIZE_Y		YRES(312)

class CStoreBtn : public CMenuHandler_TextWindow
{
private:
	int	m_iState;
public:
	CStoreBtn( int iState ) : CMenuHandler_TextWindow( iState )
	{
		m_iState = iState;
	}
	
	virtual void actionPerformed(Panel* panel)
	{
		if (m_iState != HIDE_TEXTWINDOW)
		{
			gViewPort->HideCommandMenu();
			gViewPort->ShowVGUIMenu( m_iState );
		}
		else gViewPort->HideTopMenu( );
	}
};

//------------

// Class Menu Dimensions
#define CLASSMENU_TITLE_X				XRES(40)
#define CLASSMENU_TITLE_Y				YRES(32)
#define CLASSMENU_WINDOW_NAME_Y			YRES(8)

// Creation
CStoreMenuPanel::CStoreMenuPanel(int iTrans, int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
	char sz[256]; 

	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
	SchemeHandle_t hClassWindowText = pSchemes->getSchemeHandle( "Briefing Text" );

	// color schemes
	int r, g, b, a;

	// Create the title
	pTitleLabel = new Label( "", CLASSMENU_TITLE_X, CLASSMENU_TITLE_Y );
	pTitleLabel->setParent( this );
	pTitleLabel->setFont( pSchemes->getFont(hTitleScheme) );
	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	pTitleLabel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	pTitleLabel->setBgColor( r, g, b, a );
	pTitleLabel->setContentAlignment( vgui::Label::a_west );
	pTitleLabel->setText( "NPC's Shop" );

	// Create the Scroll panel
	m_pScrollPanel = new CTFScrollPanel( STORE_SCROLLPANEL_X, STORE_WINDOW_Y, STORE_WINDOW_SIZE_X, STORE_SCROLLPANEL_SIZE_Y );
	m_pScrollPanel->setParent(this);
	//force the scrollbars on, so after the validate clientClip will be smaller
	m_pScrollPanel->setScrollBarAutoVisible(false, false);
	m_pScrollPanel->setScrollBarVisible(true, true);
	m_pScrollPanel->setBorder( new LineBorder( Color(255 * 0.7,170 * 0.7,0,0) ) );
	m_pScrollPanel->validate();

	int clientWide=m_pScrollPanel->getClient()->getWide();

	//turn scrollpanel back into auto show scrollbar mode and validate
	m_pScrollPanel->setScrollBarAutoVisible(false,true);
	m_pScrollPanel->setScrollBarVisible(false,false);
	m_pScrollPanel->validate();

	// Create the Store buttons
	for (int i = 0; i < STORE_BUTTONS; i++)
	{
		int iYPos = STORE_SIDEBUTTON_TOPLEFT_Y + ( (STORE_SIDEBUTTON_SIZE_Y + STORE_SIDEBUTTON_SPACER_Y) * i );

		// Class button
		m_pButtons[i] = new ClassButton( i, Localized(g_StoreText[i].ButtonTitle), STORE_SIDEBUTTON_TOPLEFT_X, iYPos, STORE_SIDEBUTTON_SIZE_X, STORE_SIDEBUTTON_SIZE_Y, true);

		m_pButtons[i]->setContentAlignment( vgui::Label::a_west );
		if( i != 2 ) m_pButtons[i]->addActionSignal( new CStoreBtn(iMenuToShow[i]) );
		else m_pButtons[i]->addActionSignal( new CMenuHandler_StringCommand( "trade sell 0", TRUE ) );
		m_pButtons[i]->addInputSignal( new CHandler_MenuButtonOver(this, i) );
		m_pButtons[i]->setParent( this );

		// Create the Class Info Window
		//m_pClassInfoPanel[i] = new CTransparentPanel( 255, CLASSMENU_WINDOW_X, CLASSMENU_WINDOW_Y, CLASSMENU_WINDOW_SIZE_X, CLASSMENU_WINDOW_SIZE_Y );
		m_pClassInfoPanel[i] = new CTransparentPanel( 255, 0, 0, clientWide, STORE_SCROLLPANEL_SIZE_Y );
		m_pClassInfoPanel[i]->setParent( m_pScrollPanel->getClient() );
		//m_pClassInfoPanel[i]->setVisible( false );

		// Create the Panel title
		Label *pNameLabel = new Label( "", XRES(4), CLASSMENU_WINDOW_NAME_Y );
		pNameLabel->setFont( pSchemes->getFont(hTitleScheme) ); 
		pNameLabel->setParent( m_pClassInfoPanel[i] );
		pSchemes->getFgColor( hTitleScheme, r, g, b, a );
		pNameLabel->setFgColor( r, g, b, a );
		pSchemes->getBgColor( hTitleScheme, r, g, b, a );
		pNameLabel->setBgColor( r, g, b, a );
		pNameLabel->setContentAlignment( vgui::Label::a_west );
		pNameLabel->setText(Localized(g_StoreText[i].Title));

		 _snprintf(sz, sizeof(sz),  Localized(g_StoreText[i].Desc),  "NPC" );

		#define CLASSMENU_WINDOW_TEXT_X			XRES(150)
		#define CLASSMENU_WINDOW_TEXT_Y			YRES(80)

		// Create the Text info window
		pTextWindow[i] = new TextPanel( sz, XRES(8), CLASSMENU_WINDOW_TEXT_Y, (STORE_WINDOW_SIZE_X - XRES(4))-5, STORE_WINDOW_SIZE_Y - CLASSMENU_WINDOW_TEXT_Y);
		pTextWindow[i]->setParent( m_pClassInfoPanel[i] );
		pTextWindow[i]->setFont( pSchemes->getFont(hClassWindowText) );
		pSchemes->getFgColor( hClassWindowText, r, g, b, a );
		pTextWindow[i]->setFgColor( r, g, b, a );
		pSchemes->getBgColor( hClassWindowText, r, g, b, a );
		pTextWindow[i]->setBgColor( r, g, b, a );

		m_pClassInfoPanel[i]->setSize( STORE_WINDOW_SIZE_X, CLASSMENU_WINDOW_TEXT_Y + YRES(128) );

		//m_pClassInfoPanel[i]->setSize( clientWide - 10, CLASSMENU_WINDOW_SIZE_Y - 10 );
	}

	SetBits( m_Flags, MENUFLAG_TRAPNUMINPUT );
	m_iCurrentInfo = 0;
}


// Update
void CStoreMenuPanel::Update()
{
	char sz[256];
	int width, height;
	pTitleLabel->setText( "%s's Shop", CStorePanel::StoreVendorName.c_str() );
	 for (int i = 0; i < STORE_BUTTONS; i++) 
	{
		//Don't show buttons if not activated
		m_pButtons[i]->setVisible( false );
		if( !i && !(CStorePanel::iStoreBuyFlags & STORE_BUY) ) continue;
		else if( (i==1) && !(CStorePanel::iStoreBuyFlags & STORE_SELL) ) continue;

		 _snprintf(sz, sizeof(sz),  Localized(g_StoreText[i].Desc),  CStorePanel::StoreVendorName.c_str() );
		pTextWindow[i]->setText( sz );
		pTextWindow[i]->getTextImage()->getTextSizeWrapped( width, height );
		pTextWindow[i]->setSize( width, height );
		m_pButtons[i]->setVisible( true );
	}
}

//======================================
// Key inputs for the Class Menu
bool CStoreMenuPanel::SlotInput( int iSlot )
{
	if ( (iSlot < 0) || (iSlot > STORE_BUTTONS-1) )
		return false;


	if( !m_pButtons[ iSlot ] || !m_pButtons[ iSlot ]->isVisible() ) return false;

	m_pButtons[ iSlot ]->fireActionSignal();
	return true;
}

//======================================
// Update the Class menu before opening it
void CStoreMenuPanel::Open( void )
{
	Update();
	CMenuPanel::Open();
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void CStoreMenuPanel::Initialize( void )
{
	setVisible( false );
	m_pScrollPanel->setScrollValue( 0, 0 );
}

//======================================
// Mouse is over a class button, bring up the class info
void CStoreMenuPanel::SetActiveInfo( int iInput )
{
	// Remove all the Info panels and bring up the specified one
	for (int i = 0; i < STORE_BUTTONS; i++)
	{
		m_pButtons[i]->setArmed( false );
		m_pClassInfoPanel[i]->setVisible( false );
	}

	if ( iInput >= STORE_BUTTONS || iInput < 0 )
		iInput = 0;

	m_pButtons[iInput]->setArmed( true );
	m_pClassInfoPanel[iInput]->setVisible( true );
	m_iCurrentInfo = iInput;

	m_pScrollPanel->setScrollValue(0,0);
	m_pScrollPanel->validate();
}

void CStoreMenuPanel::Reset( void )
{
	CMenuPanel::Reset();
	SetActiveInfo( 0 );
	m_iCurrentInfo = 0;
}

storeitem_t::storeitem_t( CGenericItem *pItem ) : containeritem_t( pItem )
{
	Quantity = 1;
	iCost = 0;
	flSellRatio = 0.0f;
	iBundleAmt = 0;
}

void ContainerWindowUpdate( );
void Storage_Update( );
void SellWindow_Update( );

void UpdateActiveMenus( )
{
	ContainerWindowUpdate( );
	Storage_Update( );
	SellWindow_Update( );
}
