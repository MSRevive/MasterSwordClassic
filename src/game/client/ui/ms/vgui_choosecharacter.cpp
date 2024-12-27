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
// $Log: vgui_ChooseCharacter.cpp,v $
// Revision 1.11  2005/01/17 13:16:49  dogg
// Brand new inventory VGUI, revised item system, Magic, Mp3 support and item storage
//
// Revision 1.10  2004/11/07 01:06:28  dogg
// New Event Console
//
// Revision 1.9  2004/10/19 23:14:54  dogg
// BIG update
// Memory leak fixes
//
// Revision 1.8  2004/10/16 11:47:01  dogg
// no message
//
// Revision 1.7  2004/10/14 12:15:31  dogg
// no message
//
// Revision 1.6  2004/10/13 20:21:51  dogg
// Big update
// Netcode re-arranged
//
// Revision 1.5  2004/09/23 11:54:19  dogg
// Reworked Character Encryption for linux build
//
// Revision 1.4  2004/09/19 13:39:22  dogg
// Splah damage update
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

#include "msdllheaders.h"
#include "player/player.h"
#include "weapons/genericitem.h"

#include "clglobal.h"

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "parsemsg.h"

#include "vgui_teamfortressviewport.h"
#include "vgui_mscontrols.h"

#include "vgui_menudefsshared.h"
#include "vgui_choosecharacter.h"
#include "vgui_hud.h"
#include "player/modeldefs.h"
#include "r_studioint.h"
#include "logger.h"

extern engine_studio_api_t IEngineStudio;
int			ChooseChar_Interface::ServerCharNum = 0;
bool		ChooseChar_Interface::CentralServer;

char *GenderPanel_MainBtnText[] =
{
	"#CHOOSECHAR_GENDER_MALE",
	"#CHOOSECHAR_GENDER_FEMALE"
};
char *g_CharImgName[] = 
{
	"char_male",
	"char_male"
};

static COLOR EnabledColor( 0, 255, 0, 0 ), DisabledColor( 128, 128, 128, 80 ), NewCharColor( 255, 255, 255, 0 );
static COLOR InfoColor( 192, 192, 192, 128 );
static COLOR HightlightColor( 255, 0, 0, 0 );

class CAction_SelectOption : public ActionSignal
{
//protected:
	CNewCharacterPanel *m_pPanel;
	int m_Option, m_Value;
	static createchar_t m_NewChar;
public:
	CAction_SelectOption( CNewCharacterPanel *pPanel, int iOption, int iValue )
	{
		m_pPanel = pPanel;
		m_Option = iOption;
		m_Value = iValue;
	}
	~CAction_SelectOption( )
	{
		m_pPanel = NULL;
	}

	virtual void actionPerformed(Panel* panel)
	{
		switch( m_Option )
		{
		case STG_CHOOSECHAR:
		{
			//Load and send my save character

			if( MSGlobals::ServerSideChar
				&& m_Option >= ChooseChar_Interface::ServerCharNum )
					return;

			charinfo_t &Char = player.m_CharInfo[m_Value];
			if( Char.Status != CDS_NOTFOUND )				
			{
				if( Char.Location == LOC_CLIENT &&
					Char.m_SendStatus != CSS_SENT )
				{
					//Local file

					if( Char.m_SendStatus == CSS_DORMANT )
					{
						Char.m_SendStatus = CSS_SENDING;

						player.SendChar( Char );
					}

					//RestoreAll( m_Value );
				}
				else
				{
					msstring CharCmd = msstring("char ") + m_Value + "\n";
					ServerCmd( CharCmd );
				}
				return;
			}
			else 
			{
				//New char screen
				m_NewChar.iChar = m_Value;
				MSCLGlobals::CreatingCharacter = true;
			}
			/*else if( RestoreAll( m_Value ) )			//Client chars
			{
				//gViewPort->HideTopMenu( );
				return;
			}*/

			break;
		}
		case STG_CHOOSEGENDER:
			m_NewChar.Gender = m_Value;
			m_NewChar.Name = m_pPanel->Gender_Name;
			break;
		case STG_CHOOSEWEAPON:
			//if( MSGlobals::ServerSideChar )
			{
				//Create a new character, using the options the user specified.
				msstring NewCharCmd = "char ";											//Command "char"
				NewCharCmd += m_NewChar.iChar; NewCharCmd += " \"";						//1: Char index
				NewCharCmd += m_NewChar.Name; NewCharCmd += "\" ";				//2: Char name
				NewCharCmd += m_NewChar.Gender; NewCharCmd += " ";						//3: Char Gender
				NewCharCmd += MSGlobals::DefaultWeapons[m_Value].c_str(); NewCharCmd += "\n";	//5: Char Starting weapon
				ServerCmd( NewCharCmd );
			}
			/*else
			{
				//Create a new character, using the options the user specified.
				//A custom savedata_t struct can be created for the new
				//character and passed to SaveAll, but any items the character
				//should have need to be in player.hand[]

				m_NewChar.ServerSide = false;
				m_NewChar.Name = gEngfuncs.pfnGetCvarString("name");
				m_NewChar.Weapon = MSGlobals::DefaultWeapons[m_Value];
				player.CreateChar( m_NewChar );

				MSCLGlobals::CreatingCharacter = false;
				RestoreAll( m_NewChar.iChar );
			}*/
			//gViewPort->HideVGUIMenu();
			MSCLGlobals::CreatingCharacter = false;
			m_pPanel->m_Stage = STG_FIRST;
			m_pPanel->Update( );
			return;
		}
	
		m_pPanel->m_Stage = stage_e(m_pPanel->m_Stage + 1);
		m_pPanel->Update( );
	}
};
createchar_t CAction_SelectOption::m_NewChar;

class CChoose_DeleteChar : public ActionSignal
{
protected:
	CNewCharacterPanel *m_pPanel;
	bool m_ConfirmState;
public:
	CChoose_DeleteChar( CNewCharacterPanel *pPanel, bool ConfirmState )
	{
		m_pPanel = pPanel;
		m_ConfirmState = ConfirmState;
	}

	virtual void actionPerformed( Panel* panel )
	{
		if( m_ConfirmState )
		{
			if( MSGlobals::ServerSideChar )
				ServerCmd( msstring("char -1 ") + m_pPanel->m_DeleteChar );
			else
			{
				DeleteChar( m_pPanel->m_DeleteChar );
				player.m_CharInfo[m_pPanel->m_DeleteChar].Status = CDS_NOTFOUND;
				m_pPanel->Update( );
			}
		}
		m_pPanel->m_ConfirmPanel->setVisible( false );
	}
};

class CChoose_DeleteConfirm : public ActionSignal
{
protected:
	CNewCharacterPanel *m_pPanel;
	int m_Char;
public:
	CChoose_DeleteConfirm( CNewCharacterPanel *pPanel, int i )
	{
		m_pPanel = pPanel;
		m_Char = i;
	}

	virtual void actionPerformed(Panel* panel)
	{
		m_pPanel->m_DeleteChar = m_Char;
		m_pPanel->m_ConfirmPanel->setVisible( true );
	}
};


//Clicked the 'ok' button next to the name
class GenderInput_SetName : public ActionSignal
{
public:
	CNewCharacterPanel *m_pPanel;
	GenderInput_SetName( CNewCharacterPanel *pPanel )
	{
		m_pPanel = pPanel;
	}
	virtual void actionPerformed( Panel* panel )
	{
		m_pPanel->Gender_NameSelected( );
	}
};


//Clicked the Name Label to go back and change the name
class GenderInput_ChangeName : public InputSignal
{
public:
	CNewCharacterPanel *m_pPanel;
	int m_Item;

	GenderInput_ChangeName( CNewCharacterPanel *pPanel, int Item ) { m_pPanel = pPanel; m_Item = Item; }

	virtual void mousePressed( MouseCode code,Panel* panel ) 
	{	
		m_pPanel->Gender_SelectItem( m_Item );
	}
	
	void cursorEntered( Panel *panel )					{ }
	void cursorExited( Panel* panel )					{ };
	void cursorMoved(int x,int y,Panel* panel) {};
	void mouseReleased(MouseCode code,Panel* panel) {};
	void mouseDoublePressed( MouseCode code, Panel* panel ) { };
	void mouseWheeled(int delta,Panel* panel) {};
	void keyPressed(KeyCode code,Panel* panel) {};
	void keyTyped(KeyCode code,Panel* panel) {};
	void keyReleased(KeyCode code,Panel* panel) {};
	void keyFocusTicked(Panel* panel) {};
};


class WeaponButton : public CommandButton
{
public:
	WeaponButton( int x,int y,int wide,int tall ) : CommandButton( "", x, y, wide, tall, false ) { }

	virtual void paintBackground()
	{
		if ( isArmed() )
		{
			// Orange highlight background
			drawSetColor( Scheme::sc_primary2 );
			drawFilledRect(0,0,_size[0],_size[1]);
		}
	}
};

class CGoBack : public ActionSignal
{
protected:
	CNewCharacterPanel *m_pPanel;
public:
	CGoBack( CNewCharacterPanel *pPanel )
	{
		m_pPanel = pPanel;
	}

	virtual void actionPerformed(Panel* panel)
	{
		if( m_pPanel->m_Stage == STG_FIRST + 1 ) MSCLGlobals::CreatingCharacter = false; // MIB FEB2015_21 [RACE_MENU] In case the stage order changes in the future
		m_pPanel->m_Stage = stage_e(m_pPanel->m_Stage - 1);
		m_pPanel->Update( );
	}
};

class CHandler_CharSelect : public InputSignal
{
private:
	CRenderChar *m_pChar;
public:
	CHandler_CharSelect( CRenderChar *pChar )
	{
		m_pChar = pChar;
	}
		
	void cursorEntered( Panel *panel )					{ m_pChar->Highlight( true ); }
	void cursorExited( Panel* panel )					{ m_pChar->Highlight( false ); };
	void mousePressed( MouseCode code,Panel* panel )	{ m_pChar->Select( ); };
	void cursorMoved(int x,int y,Panel* panel) {};
	void mouseReleased(MouseCode code,Panel* panel) {};
	void mouseDoublePressed( MouseCode code, Panel* panel ) { };
	void mouseWheeled(int delta,Panel* panel) {};
	void keyPressed(KeyCode code,Panel* panel) {};
	void keyTyped(KeyCode code,Panel* panel) {};
	void keyReleased(KeyCode code,Panel* panel) {};
	void keyFocusTicked(Panel* panel) {};
};
//------------

// Menu Dimensions
#define MAINWINDOW_X				XRES(0)
#define MAINWINDOW_Y				YRES(80)
#define MAINWINDOW_SIZE_X			XRES(560)
#define MAINWINDOW_SIZE_Y			YRES(320)
#define MAINLABEL_TOP_Y				YRES(0)
#define MAINBUTTON_SIZE_X			XRES(80)
#define CHOOSE_MAINLBLSIZEY			YRES(12)
#define CHOOSE_CHARHANDLING_Y		(CHOOSE_MAINLBLSIZEY + YRES(4))
#define CHOOSE_CHARHANDLING_H		YRES(28)
#define CHOOSE_BTNY					CHOOSE_CHARHANDLING_Y
//#define CHOOSE_BTNWIDTH				160
//#define CHOOSE_BTNHEIGHT			210
#define CHOOSE_BTNWIDTH				XRES(110)
#define CHOOSE_BTNHEIGHT			YRES(130)
#define CHOOSE_BTNSPACERX			XRES(16) * XRES(1)
#define CHOOSE_CHARLABELY			CHOOSE_BTNY + CHOOSE_BTNHEIGHT + YRES(8)
#define CHOOSE_CHARLABELSIZEY		YRES(13)
#define CHOOSE_DELBTNSIZEX			XRES(36)
#define CHOOSE_DELBTNSIZEY			YRES(10)
#define CHOOSE_DELBTNY				CHOOSE_BTNY + CHOOSE_BTNHEIGHT + CHOOSE_DELBTNSIZEY + YRES(21)
#define CHOOSE_DELBTNOFSX			CHOOSE_BTNWIDTH - CHOOSE_DELBTNSIZEX - XRES(0)
#define CHOOSE_Y					YRES(15)
#define CHOOSE_SIZEX				(m_pScrollPanel->getWide() - m_pScrollPanel->getVerticalScrollBar()->getWide())
#define CHOOSE_SIZEY				YRES(270)
#define GENDER_BTNY					YRES(40)
#define GENDER_SIZEY				CHOOSE_SIZEY
#define GENDER_BTNLABELY			GENDER_BTNY + CHOOSE_BTNHEIGHT + YRES(8)
#define WEAPON_BTNY					YRES(26)
#define WEAPON_BTN_SIZEX			128
#define WEAPON_BTN_SIZEY			128
#define WEAPON_BTNLABEL_SPACERY		YRES(10)
#define WEAPON_BTN_SPACERY			WEAPON_BTNLABEL_SPACERY + YRES(21)
#define BACK_BTN_X					XRES(80)
#define BACK_BTN_Y					YRES(360)
#define BACK_BTN_SIZEX				XRES(60)
#define BACK_BTN_SIZEY				YRES(20)

int GetCenteredItemX( int WorkSpaceSizeX, int ItemSizeX, int Items, int SpaceBewteenItems )
{
	//Get the X value of the FIRST item
	return (WorkSpaceSizeX/2.0f) - (ItemSizeX/2.0f * Items + (SpaceBewteenItems/2.0f * Items-1));
}

// Creation
CMenuPanel *CreateNewCharacterPanel( )
{
	return new CNewCharacterPanel( 1, true, 0, 0, ScreenWidth, ScreenHeight);
}
CNewCharacterPanel::CNewCharacterPanel( int iTrans, int iRemoveMe, int x, int y, int wide,int tall ) : CMenuPanel( 0, TRUE, x, y, wide, tall )
{
	startdbg;
	dbg( "Begin" );

	SetBits( m_Flags, MENUFLAG_TRAPNUMINPUT );

	Gender_Name = gEngfuncs.pfnGetCvarString("name");

	CTransparentPanel *pMainPanel = new CTransparentPanel( 255, MAINWINDOW_X, MAINWINDOW_Y, getWide( ), MAINWINDOW_SIZE_Y );
	pMainPanel->setParent( this );

	m_pScrollPanel = new CTFScrollPanel( 0, 0, getWide( ), MAINWINDOW_SIZE_Y );
	m_pScrollPanel->setParent( pMainPanel );
	m_pScrollPanel->setScrollBarVisible( false, false );
	m_pScrollPanel->setScrollBarAutoVisible( true, true );
	m_pScrollPanel->setFgColor( Scheme::sc_primary1 );
	m_pScrollPanel->setBgColor( Scheme::sc_primary1 );
	m_pScrollPanel->validate();

	m_pTitleLabel = new MSLabel( this, "", 0, 0, wide, YRES(80), MSLabel::a_center );
	m_pTitleLabel->setFont( g_FontTitle );
	m_pTitleLabel->SetFGColorRGB( NewCharColor );
	m_pTitleLabel->setText( Localized("#CHOOSECHAR_PANE_CHOOSE") );
	//Setup Choose Character panel
	
	m_ChoosePanel = new CTransparentPanel( 255, 0, CHOOSE_Y, getWide( ), CHOOSE_SIZEY );
	m_ChoosePanel->setParent( m_pScrollPanel->getClient() );
	//m_ChoosePanel->setParent( m_pScrollPanel->getClient() );
	//m_ChoosePanel->setBorder( new LineBorder(Color(NewCharColor.r,NewCharColor.g,NewCharColor.b,NewCharColor.a)) );
	Choose_MainLabel = new MSLabel( m_ChoosePanel, "", XRES(0), YRES(0), CHOOSE_SIZEX, CHOOSE_MAINLBLSIZEY, MSLabel::a_center );
	Choose_MainLabel->SetFGColorRGB( Color_Text_White );

	Choose_CharHandlingLabel = new MSLabel( m_ChoosePanel, "", XRES(0), CHOOSE_CHARHANDLING_Y, CHOOSE_SIZEX, CHOOSE_CHARHANDLING_H, MSLabel::a_north );
	Choose_CharHandlingLabel->SetFGColorRGB( InfoColor );
	
	dbg( "Init Main buttons" );
	float ButtonWidth = CHOOSEPANEL_MAINBTNS * CHOOSE_BTNWIDTH + (CHOOSEPANEL_MAINBTNS-1) * CHOOSE_BTNSPACERX;
	float StartX = (m_ChoosePanel->getWide()/2.0f) - (ButtonWidth/2.0f);
	for( int i = 0; i < CHOOSEPANEL_MAINBTNS; i++ )
	{
		int ix = StartX + i * CHOOSE_BTNWIDTH + i * CHOOSE_BTNSPACERX, iy = CHOOSE_BTNY + CHOOSE_CHARHANDLING_H;
		//Choose_Image[i] = new CImageLabel( "", ix, iy );
		//Choose_Image[i]->setParent( m_ChoosePanel );
		Choose_ImageCover[i] = new CTransparentPanel( 100, ix, iy, CHOOSE_BTNWIDTH, CHOOSE_BTNHEIGHT );
		Choose_ImageCover[i]->setParent( m_ChoosePanel );
		Choose_ImageCover[i]->SetBGColorRGB( DisabledColor );
		//int TextWidth, h;
		//g_FontSml->getTextSize( Localized("#CHOOSECHAR_CREATE"), TextWidth, h );
		Choose_MainBtn[i] = new MSButton( m_ChoosePanel, "", ix, iy, CHOOSE_BTNWIDTH, CHOOSE_BTNHEIGHT );
		Choose_MainBtn[i]->setContentAlignment( Label::a_center );
		//Choose_MainBtn[i]->setTextAlignment( Label::a_center );
		Choose_MainBtn[i]->SetArmedColor( NewCharColor );
		Choose_MainBtn[i]->SetDisabledColor( DisabledColor );
		Choose_MainActionSig[i] = new CAction_SelectOption( this, STG_CHOOSECHAR, i );
		Choose_MainBtn[i]->addActionSignal( Choose_MainActionSig[i] );
		Choose_MainInputSig[i] = new CHandler_CharSelect( &m_CharEnts[i] );
		Choose_MainBtn[i]->addInputSignal( Choose_MainInputSig[i] );
		Choose_MainBtn[i]->setFont( g_FontID );
		Choose_MainBtn[i]->setText( Localized("#CHOOSECHAR_CREATE") );

		iy += Choose_MainBtn[i]->getTall();
		Choose_CharLabel[i][0] = new MSLabel( m_ChoosePanel, "", ix, iy, CHOOSE_BTNWIDTH, CHOOSE_CHARLABELSIZEY, MSLabel::a_center );
		iy += Choose_CharLabel[i][0]->getTall();
		Choose_CharLabel[i][1] = new MSLabel( m_ChoosePanel, "", ix, iy, CHOOSE_BTNWIDTH, CHOOSE_CHARLABELSIZEY, MSLabel::a_center );
		iy += Choose_CharLabel[i][1]->getTall();

		 for (int n = 0; n < 2; n++) 
		{
			//Labels - Name & map info
			Choose_CharLabel[i][n]->setFont( g_FontSml );
		}

		//Delete button
		Choose_DeleteChar[i] = new MSButton( m_ChoosePanel, "", ix + CHOOSE_DELBTNOFSX, iy, CHOOSE_DELBTNSIZEX, CHOOSE_DELBTNSIZEY );
		Choose_DeleteChar[i]->setFont( g_FontSml );
		Choose_DeleteChar[i]->SetArmedColor( NewCharColor );
		Choose_DelActionSig[i] = new CChoose_DeleteConfirm( this, i );
		Choose_DeleteChar[i]->addActionSignal( Choose_DelActionSig[i] );
		Choose_DeleteChar[i]->setText( Localized("#CHOOSECHAR_DELETE") );	//For Font
	}

	Choose_UploadStatus = new MSLabel( m_ChoosePanel, "", XRES(0), m_ChoosePanel->getTall() - CHOOSE_MAINLBLSIZEY, CHOOSE_SIZEX, CHOOSE_MAINLBLSIZEY, MSLabel::a_center );
	Choose_UploadStatus->SetFGColorRGB( Color_Text_White );

	//Confirm character deletion Panel
	#define CONFIRM_SIZEX XRES(120)
	#define CONFIRM_SIZEY YRES(60)

	m_ConfirmPanel = new CTransparentPanel( 10, (CHOOSE_SIZEX/2.0f)-(CONFIRM_SIZEX/2.0f), YRES(120), CONFIRM_SIZEX, CONFIRM_SIZEY );
	m_ConfirmPanel->setParent( m_ChoosePanel );
	m_ConfirmPanel->setBorder( new LineBorder(Color(NewCharColor.r,NewCharColor.g,NewCharColor.b,NewCharColor.a)) );
	m_ConfirmPanel->setVisible( false );

	#define CONFIRM_TITLE_SIZEX XRES(70)

	MSLabel *pConfirmLabel = new MSLabel( m_ConfirmPanel, "", (CONFIRM_SIZEX/2.0f) - (CONFIRM_TITLE_SIZEX/2.0f), YRES(5), CONFIRM_TITLE_SIZEX, YRES(12) );
	pConfirmLabel->setFont( g_FontSml );
	pConfirmLabel->SetFGColorRGB( EnabledColor );
	pConfirmLabel->setText( Localized("#CHOOSECHAR_DELETEQUESTION") );

	#define CONFIRMBTN_X XRES(12)
	#define CONFIRMBTN_SIZEX XRES(20)
	#define CONFIRMBTN_SIZEY YRES(10)

	MSButton *pConfirm = new MSButton( m_ConfirmPanel, "", CONFIRMBTN_X, (CONFIRM_SIZEY/2.0f) - (CONFIRMBTN_SIZEY/2.0f), CONFIRMBTN_SIZEX, CONFIRMBTN_SIZEY );
	pConfirm->setFont( g_FontSml );
	pConfirm->SetArmedColor( NewCharColor );
	pConfirm->addActionSignal( new CChoose_DeleteChar( this, true ) );
	pConfirm->setText( Localized("#YES") );

	pConfirm = new MSButton( m_ConfirmPanel, "", (CONFIRM_SIZEX - CONFIRMBTN_SIZEX) - CONFIRMBTN_X, (CONFIRM_SIZEY/2.0f) - (CONFIRMBTN_SIZEY/2.0f), CONFIRMBTN_SIZEX, CONFIRMBTN_SIZEY );
	pConfirm->setFont( g_FontSml );
	pConfirm->SetArmedColor( NewCharColor );
	pConfirm->addActionSignal( new CChoose_DeleteChar( this, false ) );
	pConfirm->setText( Localized("#NO") );

	//Setup Gender panel
	
	#define LABEL_ITEMNAME_SIZE_X			XRES(128)
	#define LABEL_ITEMNAME_SIZE_Y			YRES(16)
	#define LABEL_ITEM_SPACER_Y				YRES(2)

	dbg( "Init Gender buttons" );

	m_GenderPanel = new CTransparentPanel( 255, 0, 0, CHOOSE_SIZEX, GENDER_SIZEY );
	m_GenderPanel->setParent( m_pScrollPanel->getClient() );
	Gender_MainLabel = new TextPanel( "", XRES(8), MAINLABEL_TOP_Y, LABEL_ITEMNAME_SIZE_X, LABEL_ITEMNAME_SIZE_Y );
	Gender_MainLabel->setParent( m_GenderPanel );
	Gender_MainLabel->setFont( g_FontSml );
	Gender_MainLabel->SetFGColorRGB( NewCharColor );
	Gender_MainLabel->SetBGColorRGB( Color_Transparent );
 
	#define GENDER_NAME_X XRES(75)
	#define GENDER_NAME_Y MAINLABEL_TOP_Y + YRES(5)

	Gender_NameLabel = new MSLabel( m_GenderPanel, Localized( "#CHOOSECHAR_NAME" ), XRES(75), GENDER_NAME_Y );
	Gender_NameLabel->setFont( g_FontID );
	Gender_NameLabel->addInputSignal( new GenderInput_ChangeName( this, 0 ) );

	Gender_NameTextPanel = new VGUI_TextPanel( m_GenderPanel, GENDER_NAME_X + Gender_NameLabel->getWide(), GENDER_NAME_Y+YRES(2), XRES(120), 20 );
	Gender_NameTextPanel->SetText( Gender_Name );
	Gender_NameTextPanel->addInputSignal( new GenderInput_ChangeName( this, 0 ) );
	Gender_NameTextPanel->m_MaxLetters = 32;

	//MiB DEC2007a (changing button locs)
	Gender_NameOK = new MSButton( m_GenderPanel, "", XRES(220), GENDER_NAME_Y - YRES(2), XRES(100), YRES(20) );
	//old: Gender_NameOK = new MSButton( m_GenderPanel, "", XRES(275), GENDER_NAME_Y, XRES(20), YRES(20) );
	Gender_NameOK->addActionSignal( new GenderInput_SetName( this ) );
	Gender_NameOK->setContentAlignment( Label::a_center );
	Gender_NameOK->SetArmedColor( NewCharColor );
	Gender_NameOK->SetDisabledColor( DisabledColor );
	Gender_NameOK->setText( Localized("Submit Name") );
	//old: Gender_NameOK->setText( Localized("#CHOOSECHAR_GENDEROK") );

	Gender_GenderLabel = new MSLabel( m_GenderPanel, Localized( "#CHOOSECHAR_GENDERLBL" ), XRES(75), YRES(30) );
	Gender_GenderLabel->SetFGColorRGB( NewCharColor );
	Gender_GenderLabel->setFont( g_FontID );
	Gender_GenderLabel->addInputSignal( new GenderInput_ChangeName( this, 1 ) );

	dbg( "Init Gender Panel" );
	StartX = GetCenteredItemX( m_ChoosePanel->getWide(), CHOOSE_BTNWIDTH, 2, XRES(32) );
	 for (int i = 0; i < GENDERPANEL_MAINBTNS; i++) 
	{
		int ix = StartX + i * CHOOSE_BTNWIDTH + i * CHOOSE_BTNSPACERX, iy = CHOOSE_BTNY + CHOOSE_CHARHANDLING_H;
		//int ix = StartX + i * CHOOSE_BTNWIDTH + i * CHOOSE_BTNSPACERX, iy = GENDER_BTNY;
		MSButton &GenderBtn  = *(Gender_MainBtn[i] = new MSButton( m_GenderPanel, "", ix, iy, CHOOSE_BTNWIDTH, CHOOSE_BTNHEIGHT ));
		Gender_MainActionSig[i] = new CAction_SelectOption( this, STG_CHOOSEGENDER, i );
		GenderBtn.addActionSignal( Gender_MainActionSig[i] );
		GenderBtn.setText( Localized(GenderPanel_MainBtnText[i]) );
		GenderBtn.setContentAlignment( Label::a_center );
		GenderBtn.SetArmedColor( NewCharColor );
		GenderBtn.SetDisabledColor( DisabledColor );
	}

	Gender_SelectItem( 0 );

	//Weapon panel setup

	dbg( "Init Weapon Panel" );
	m_WeaponPanel = new CTransparentPanel( 255, 0, 0, CHOOSE_SIZEX, GENDER_SIZEY );
	m_WeaponPanel->setParent( m_pScrollPanel->getClient() );
	Weapon_MainLabel = new TextPanel( Localized( "#CHOOSECHAR_WEAPON" ), XRES(120), MAINLABEL_TOP_Y, LABEL_ITEMNAME_SIZE_X, LABEL_ITEMNAME_SIZE_Y );
	Weapon_MainLabel->setParent( m_WeaponPanel );
	Weapon_MainLabel->setFont( g_FontSml );
	Weapon_MainLabel->SetFGColorRGB( NewCharColor );
	Weapon_MainLabel->SetBGColorRGB( Color_Transparent );

	dbg( msstring("Init Weapon Buttons (") + (int)WEAPONPANEL_MAINBTNS + ")" );
	StartX = GetCenteredItemX( m_ChoosePanel->getWide(), WEAPON_BTN_SIZEX, 3, CHOOSE_BTNSPACERX );
	int WeaponPanelSizeY = m_WeaponPanel->getTall( );
	for (int i = 0; i < WEAPONPANEL_MAINBTNS; i++) 
	{
		if( i >= WEAPONPANEL_MAINBTNMAX )
			break;							//Max of 9 starting weapon choices

		int ix = StartX + (i%3) * WEAPON_BTN_SIZEX + (i%3) * CHOOSE_BTNSPACERX,
			iy = WEAPON_BTNY + (i/3) * (WEAPON_BTN_SIZEY + WEAPON_BTN_SPACERY),
			iw = WEAPON_BTN_SIZEX,
			ih = WEAPON_BTN_SIZEY;
		dbg( msstring("Create Weapon") + MSGlobals::DefaultWeapons[i] );
		
		// MiB MAR2019_13 - Clean up of "temporary items" in favor of global table
		CGenericItem *ptmpItem = CGenericItemMgr::GetGlobalGenericItemByName( MSGlobals::DefaultWeapons[i], true );
		if( !ptmpItem )
		{
			dbg( msstring("Weapon ") + MSGlobals::DefaultWeapons[i] + " Not found" );
			Print( "Error: Starting item %s NOT FOUND!\n", MSGlobals::DefaultWeapons[i].c_str() );
			Weapon_MainBtn[i] = NULL;
			Weapon_MainActionSig[i] = NULL;
			continue;
		}
		dbg( msstring("Init Weapon") + MSGlobals::DefaultWeapons[i] );
		Weapon_MainBtnImg[i] = new CImageDelayed( msstring("items/640_") + ptmpItem->TradeSpriteName, false, true, ix, iy, iw, ih );
		msstring Name = ptmpItem->DisplayName( );

		Weapon_MainBtnImg[i]->SetFrame( ptmpItem->SpriteFrame ); //Shuriken FEB2008

		Weapon_MainBtnImg[i]->setParent( m_WeaponPanel );

		Weapon_MainBtn[i] = new WeaponButton( ix, iy, iw, ih );
		Weapon_MainBtn[i]->setParent( m_WeaponPanel );
		Weapon_MainActionSig[i] = new CAction_SelectOption( this, STG_CHOOSEWEAPON, i );
		Weapon_MainBtn[i]->addActionSignal( Weapon_MainActionSig[i] );

		dbg( msstring("Init Weapon Label (") + MSGlobals::DefaultWeapons[i] + ")" );
		//MiB DEC2007a - moving buttons:
		Weapon_BtnLabel[i] = new MSLabel( m_WeaponPanel, Name, ix - XRES(Name.len()/2), iy + ih + WEAPON_BTNLABEL_SPACERY, iw + XRES(Name.len())/*+ CHOOSE_BTNSPACERX*/, CHOOSE_CHARLABELSIZEY );
		//old: Weapon_BtnLabel[i] = new MSLabel( m_WeaponPanel, Name, ix, iy + ih + WEAPON_BTNLABEL_SPACERY, iw /*+ CHOOSE_BTNSPACERX*/, CHOOSE_CHARLABELSIZEY );
		Weapon_BtnLabel[i]->setContentAlignment( Label::a_center );
		Weapon_BtnLabel[i]->setFont( g_FontSml );
		Weapon_BtnLabel[i]->SetFGColorRGB( NewCharColor );
		Weapon_BtnLabel[i]->setBgColor( 0, 0, 0, 255 );

		int lblx, lbly;
		Weapon_BtnLabel[i]->getPos( lblx, lbly );
		int ButtonBottomY = lbly + Weapon_BtnLabel[i]->getTall( );
		if( ButtonBottomY > WeaponPanelSizeY ) WeaponPanelSizeY = ButtonBottomY;
	}

	//Resize the weapon panel
	dbg( "Resize Wepaon panel" );
	m_WeaponPanel->setSize( m_WeaponPanel->getWide(), WeaponPanelSizeY );

	#define BUTTON_CANCEL_SIZE_X			XRES(200)
	#define BUTTON_CANCEL_SIZE_Y			YRES(12)
	#define BUTTON_CANCEL_SPACER_Y			YRES(6)

	// Create the Cancel button
	dbg( "Init cancel button" );
	m_pCancelButton = new MSButton( this, Localized("#CANCEL"), MAINWINDOW_X, MAINWINDOW_Y + MAINWINDOW_SIZE_Y + BUTTON_CANCEL_SPACER_Y, BUTTON_CANCEL_SIZE_X, BUTTON_CANCEL_SIZE_Y );
	((MSButton *)m_pCancelButton)->SetArmedColor( NewCharColor );
	m_pCancelButton->addActionSignal( new CMenuHandler_TextWindow( HIDE_TEXTWINDOW ) );

	// Create the Back button
	dbg( "Init back button" );
	m_BackBtn = new MSButton( this, "", BACK_BTN_X, BACK_BTN_Y, BACK_BTN_SIZEX, BACK_BTN_SIZEY );
	m_BackBtn->SetArmedColor( NewCharColor );
	m_BackBtn->addActionSignal( new CGoBack( this ) );
	m_BackBtn->setText( Localized("#CHOOSECHAR_BACK") );
	
	m_pScrollPanel->setScrollValue( 0, 0 );
	fClosingMenu = false;
	iButtons = 0;
	m_Stage = STG_CHOOSECHAR;
	enddbg;
}

// Update
void CNewCharacterPanel::Update()
{
	//Put my character header info into a global structure, so the choose character panel knows what's what.
	//If client-side characters, load the data from file right now
	//If server-side characters, then the data is loaded via Server Msg "CharInfo" before this is ever called
 
	m_ChoosePanel->setVisible( false );
	m_GenderPanel->setVisible( false );
	m_WeaponPanel->setVisible( false );

	char cTemp[128], cTemp2[512];
	strncpy(cTemp2, MSGlobals::MapName, sizeof(cTemp2) );
	cTemp2[0] = toupper(cTemp2[0]);

	_snprintf(cTemp, sizeof(cTemp), Localized("#CHOOSECHAR_ENTERING"), cTemp2);		//Entering: <mapname>
	Choose_MainLabel->setText( cTemp );

	if( MSGlobals::ServerSideChar )
	{
		cTemp2[0] = 0;
		if (MSGlobals::IsLanGame) 
			_snprintf(cTemp2, sizeof(cTemp2), "\n%s", Localized("#CHOOSECHAR_LAN"));

		if(ChooseChar_Interface::CentralServer)
			_snprintf(cTemp2, sizeof(cTemp2), "\n%s", Localized("#CHOOSECHAR_CENTRALNETWORK"));

		_snprintf(cTemp, sizeof(cTemp), "%s%s", Localized("#CHOOSECHAR_SERVER"), cTemp2);
		Choose_CharHandlingLabel->setText( cTemp );								//Character are stored on the server
	}
	else
		Choose_CharHandlingLabel->setText(Localized("#CHOOSECHAR_LOCAL"));		//Character are stored on the client

	switch( m_Stage )
	{
	case STG_CHOOSECHAR:
		m_ChoosePanel->setVisible( true );
		iButtons = CHOOSEPANEL_MAINBTNS;

		for (int i = 0; i < iButtons; i++) 
		{
			//Only certain fields can be assumed valid in this savedata
			//If client-side characters - all savedata_t info is valid;
			//If server-side, only the Name, map, nextmap, oldtrans, and newtrans are valid
			charinfo_t &CharSlot = player.m_CharInfo[i];
			if( CharSlot.Status != CDS_LOADED )
			{
				Choose_MainBtn[i]->setText( Localized("#CHOOSECHAR_CREATE") );

				if( CharSlot.Status == CDS_NOTFOUND &&				//No char at this slot and
					MSGlobals::CanCreateCharOnMap &&				//Map allows creating characters and
					player.m_CharSend.Status == CSS_DORMANT &&		//Not currently uploading a character and
					(!MSGlobals::ServerSideChar ||					//Characters are client side or
					i < ChooseChar_Interface::ServerCharNum) )		//Server allows clients to create at least this many characters
				{		
					//Choose_MainBtn[i]->SetBGColorRGB( NewCharColor );
					//Choose_MainBtn[i]->SetFGColorRGB( NewCharColor );

					Choose_MainBtn[i]->setEnabled( true );
					Choose_ImageCover[i]->setVisible( false );
				}
				else {
					//Choose_MainBtn[i]->SetBGColorRGB( DisabledColor );
					//Choose_MainBtn[i]->SetFGColorRGB( DisabledColor );
					if (CharSlot.Status == CDS_LOADING)
						Choose_MainBtn[i]->setText(Localized("#CHOOSECHAR_LOADING"));
					Choose_MainBtn[i]->setEnabled( false );
					//Choose_ImageCover[i]->setVisible( true );
				}
				Choose_CharLabel[i][0]->setText( "" );
				Choose_CharLabel[i][1]->setText( "" );
				Choose_DeleteChar[i]->setVisible( false );
				//Choose_Image[i]->setVisible( false );
				m_CharEnts[i].SetVisible( false );
				continue;
			}
			else
			{
				//Character Status is CDS_LOADED
				Choose_MainBtn[i]->setText( "" );
				Choose_CharLabel[i][0]->setText( CharSlot.Name );
				char cMapInfo[128];
				CharSlot.MapName[0] = toupper(CharSlot.MapName[0]);
				CharSlot.NextMap[0] = toupper(CharSlot.NextMap[0]);
				if( CharSlot.NextMap[0] )
					 _snprintf(cMapInfo, sizeof(cMapInfo),  "%s -> %s",  CharSlot.MapName.c_str(),  CharSlot.NextMap.c_str() );
				else
					 _snprintf(cMapInfo, sizeof(cMapInfo),  "At %s",  CharSlot.MapName.c_str() );
				Choose_CharLabel[i][1]->setText( cMapInfo );
				Choose_DeleteChar[i]->setVisible( true );

				//Grey out the char if it can't join the map
				bool GreyedOut = CharSlot.JoinType == JN_NOTALLOWED;
				
				//Grey out all chars while uploading a char
				if( player.m_CharSend.Status != CSS_DORMANT ) GreyedOut = true;

				msstring model = MODEL_HUMAN_REF;

				//m_CharEnts[i].Init( i , model ); // MIB FEB2015_21 [RACE_MENU] - Re-init with correct race model

				if( GreyedOut )
				{
					//Character exists, but cannot be selected (can't join map, temporarily disabled, etc.)
					Choose_MainBtn[i]->setVisible( false );
					Choose_CharLabel[i][0]->SetFGColorRGB( DisabledColor );
					Choose_CharLabel[i][1]->SetFGColorRGB( DisabledColor );
					Choose_ImageCover[i]->setVisible( false );	//no image cover anymore, it's obvious
					m_CharEnts[i].SetActive( false );
				}
				else
				{
					//Character can join this map
					Choose_MainBtn[i]->SetBGColorRGB( Color_Transparent );
					Choose_MainBtn[i]->setVisible( true );
					Choose_MainBtn[i]->setEnabled( true );
					Choose_CharLabel[i][0]->SetFGColorRGB( EnabledColor );
					Choose_CharLabel[i][1]->SetFGColorRGB( EnabledColor );
					Choose_ImageCover[i]->setVisible( false );
					m_CharEnts[i].SetActive( true );
				}
				//Character picture
	//			Choose_Image[i]->setContentFitted(true);
				/*Choose_Image[i]->m_pTGA = LoadTGAForRes(g_CharImgName[Data.Gender]);
				Choose_Image[i]->setImage( Choose_Image[i]->m_pTGA );
				int iWidth = Choose_Image[i]->getImageWide();
				int iHeight = Choose_Image[i]->getImageTall();
				Choose_Image[i]->setSize( iWidth, iHeight );
				Choose_Image[i]->setVisible( true );*/
				
				m_CharEnts[i].SetVisible( true );		//Show 3D model
			}
		}
		
		m_pTitleLabel->setText( Localized("#CHOOSECHAR_PANE_CHOOSE") );
		m_BackBtn->setVisible( false );
		break;
	case STG_CHOOSEGENDER:
		m_GenderPanel->setVisible( true );
		Gender_NameTextPanel->SetText( Gender_Name );

		m_BackBtn->setVisible( true );
		m_pTitleLabel->setText( Localized("#CHOOSECHAR_PANE_NEW") );
		iButtons = GENDERPANEL_MAINBTNS;
		break;
	case STG_CHOOSEWEAPON:
		 for (int i = 0; i < WEAPONPANEL_MAINBTNS; i++) 
			Weapon_MainBtnImg[i]->LoadImg( );
		m_WeaponPanel->setVisible( true );
		m_BackBtn->setVisible( true );
		m_pTitleLabel->setText( Localized("#CHOOSECHAR_PANE_NEW") );
		iButtons = WEAPONPANEL_MAINBTNS;
		break;
	}

	//Only allow a cancel if you've already spawned
	m_pCancelButton->setVisible( true );
	if( player.m_CharacterState == CHARSTATE_UNLOADED )
		m_pCancelButton->setVisible( false );
	m_pScrollPanel->validate( );
}
void CNewCharacterPanel::Gender_SelectItem( int Btn )
{
	Gender_Item = Btn;

	if( Btn == 0 )
	{
		Gender_NameLabel->SetFGColorRGB( HightlightColor );
		Gender_GenderLabel->SetFGColorRGB( DisabledColor );
		Gender_NameTextPanel->SetActive( true );
		Gender_NameOK->setEnabled( true );
	}
	else
	{
		Gender_NameLabel->SetFGColorRGB( NewCharColor );
		Gender_GenderLabel->SetFGColorRGB( HightlightColor );
		Gender_NameTextPanel->SetActive( false );
		Gender_NameOK->setEnabled( false );
	}

	Gender_CharEnts[0].m_Gender = GENDER_MALE; //Thothie FEB2011_02 gender bender fixes
	Gender_CharEnts[1].m_Gender = GENDER_FEMALE; //Thothie FEB2011_02 gender bender fixes

	 for (int i = 0; i < GENDERPANEL_MAINBTNS; i++) 
	{
		Gender_CharEnts[i].SetActive( Btn == 1 ? true : false );
		Gender_MainBtn[i]->setEnabled( Btn == 1 ? true : false );
	}

	Gender_NameTextPanel->SetText( Gender_Name );
}

void CNewCharacterPanel::Think( )
{
	switch( m_Stage )
	{
	case STG_CHOOSEGENDER:
		Gender_NameTextPanel->Update( );
		break;
	}
}

bool CNewCharacterPanel::KeyInput( int down, int keynum, const char *pszCurrentBinding ) 
{
	startdbg;
	switch( m_Stage )
	{
	case STG_CHOOSEGENDER:
		if( keynum == '\r' )
		{
			Gender_NameSelected( );
		}
		else
			Gender_NameTextPanel->KeyInput( down, keynum, pszCurrentBinding ); 
		return true;
	}

	enddbg;

	return false;
}
void CNewCharacterPanel::Gender_NameSelected( ) 
{
	if( Gender_NameTextPanel->m_Message.len( ) )
		Gender_Name = Gender_NameTextPanel->m_Message;
	Gender_NameTextPanel->SetText( Gender_Name );
	Gender_SelectItem( 1 );
	Update( );
}

void CNewCharacterPanel::Close( void )
{

	//Note - I stopped deleting the resources... it seems to crash HL much later on

	startdbg;
	dbg( "Begin" );
	MSCLGlobals::CharPanelActive = false;

	/*if( fClosingMenu )
	{
		CMenuPanel::Close( );
		delete this;
		return;
	}*/

	 for (int i = 0; i < CHOOSEPANEL_MAINBTNS; i++) 
	{
		CRenderChar &CharEnt = m_CharEnts[i];
		CharEnt.UnRegister( );
	}

	 for (int i = 0; i < GENDERPANEL_MAINBTNS; i++) 
	{
		CRenderChar &CharEnt = Gender_CharEnts[i];
		CharEnt.UnRegister( );
	}

	m_SpawnBox.UnRegister( );

	int i;
	removeAllChildren( );

	m_ChoosePanel->removeAllChildren( );
	//delete m_ChoosePanel;
	for( i = 0; i < CHOOSEPANEL_MAINBTNS; i++ )
	{
		Choose_MainBtn[i]->removeActionSignals( );
		//if( Choose_MainActionSig[i] ) delete Choose_MainActionSig[i];
		if( Choose_MainInputSig[i] )
		{
			Choose_MainBtn[i]->removeInputSignal( Choose_MainInputSig[i] );
			//delete Choose_MainInputSig[i];
		}
		//delete Choose_MainBtn[i];
	}

	m_GenderPanel->removeAllChildren( );
	//delete m_GenderPanel;
	for( i = 0; i < GENDERPANEL_MAINBTNS; i++ )
	{
		Gender_MainBtn[i]->removeActionSignals( );
		//if( Gender_MainActionSig[i] ) delete Gender_MainActionSig[i];
		//delete Gender_MainBtn[i];
	}

	m_WeaponPanel->removeAllChildren( );
	//delete m_WeaponPanel;
	for (i = 0; i < (signed)WEAPONPANEL_MAINBTNS; i++)
	{
		if( Weapon_MainBtn[i] ) Weapon_MainBtn[i]->removeActionSignals( );
		//if( Weapon_MainActionSig[i] ) delete Weapon_MainActionSig[i];
		//delete Weapon_MainBtn[i]; -- THIS DELETE NEVER WORKS
	}

	fClosingMenu = true;
	//gViewPort->HideTopMenu( );

	//Remove from menu list
	 for (int i = 0; i < gViewPort->m_Menus.size(); i++) 
		if( gViewPort->m_Menus[i] == this ) 
			{ gViewPort->m_Menus.erase( i ); break; }

	CMenuPanel::Close( );
	enddbg;
}
//======================================
// Key inputs for the Class Menu
bool CNewCharacterPanel::SlotInput( int iSlot )
{
	//Convert iSlot to be zero-based
	iSlot--;
	if( iSlot < 0 ) iSlot = 9;

	if ( (iSlot < 0) || (iSlot > iButtons-1) )
		return false;

	switch( m_Stage )
	{
		case STG_CHOOSECHAR:
			if( !Choose_MainBtn[ iSlot ] || Choose_MainBtn[ iSlot ]->IsNotValid() )
				return false;
			else {
				Choose_MainBtn[ iSlot ]->fireActionSignal();
				return true;
			}
		case STG_CHOOSEGENDER:
			if( !Gender_MainBtn[ iSlot ] || Gender_MainBtn[ iSlot ]->IsNotValid() )
				return false;
			else {
				Gender_MainBtn[ iSlot ]->fireActionSignal();
				return true;
			}
		case STG_CHOOSEWEAPON:
			if( !Weapon_MainBtn[ iSlot ] || Weapon_MainBtn[ iSlot ]->IsNotValid() )
				return false;
			else {
				Weapon_MainBtn[ iSlot ]->fireActionSignal();
				return true;
			}
	}

	return false;
}

//======================================
// Update the menu before opening it

void CNewCharacterPanel::Open( void )
{
	startdbg;
	m_Stage = STG_CHOOSECHAR;
	MSCLGlobals::CharPanelActive = true;

	dbg( "Call CMenuPanel::Open()" );
	CMenuPanel::Open();

	dbg( "Init Main 3D models" );
	 for (int i = 0; i < CHOOSEPANEL_MAINBTNS; i++) 
	{
		CRenderChar &CharEnt = m_CharEnts[i];
		CharEnt.m_Stage = STG_CHOOSECHAR;
		CharEnt.Register( );
		CharEnt.Init( i );
	}

	dbg( "Init Gender 3D models" );
	 for (int i = 0; i < GENDERPANEL_MAINBTNS; i++) 
	{
		CRenderChar &CharEnt = Gender_CharEnts[i];
		CharEnt.m_Stage = STG_CHOOSEGENDER;
		CharEnt.Register( );
		CharEnt.Init( i );
	}

	dbg( "Call Update" );
	Update( );

	dbg( "setScrollValue" );
	m_pScrollPanel->setScrollValue( 0, 0 );

	enddbg;
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void CNewCharacterPanel::Initialize( void )
{
	startdbg;
	setVisible( false );
	m_pScrollPanel->setScrollValue( 0, 0 );

	enddbg;
}

void CNewCharacterPanel::UpdateUpload( )
{
	if( player.m_CharSend.Status == CSS_SENDING )
	{
		Choose_UploadStatus->setVisible( true );
		int Percent = ((float)player.m_CharSend.DataSent / player.m_CharSend.DataLen) * 100;
		msstring Text = msstring("Uploading (") + Percent + "%)";
		Choose_UploadStatus->setText( Text.c_str() );
	}
	else
	{
		Choose_UploadStatus->setVisible( false );
	}

}

void ShowVGUIMenu( int iMenu );
void __CmdFunc_PlayerChooseChar( )
{
#ifdef DEV_BUILD
	if( player.m_CharacterState == CHARSTATE_UNLOADED )
		ShowVGUIMenu( MENU_NEWCHARACTER );
#endif
}

int __MsgFunc_CharInfo(const char* pszName, int iSize, void* pbuf)
{
	startdbg;
	dbg("Begin");
	BEGIN_READ(pbuf, iSize);

	//Put my character header info into a global structure, so the choose character panel knows what's what.
	//This info comes from the server, if characters are server-side
	//If chars are client-side, then this message only comes with Status == CDS_LOADED after I've uploaded a char
	//Read data for one character at a time

	byte CharMsgType = READ_BYTE();
	byte CharIndex = READ_BYTE();						//Which char the info describes

	if (CharIndex >= MAX_CHARSLOTS)
		return 0;

	charinfo_t& CharSlot = player.m_CharInfo[CharIndex];

	if (CharMsgType == CHAR_TYPE_ITEMS)
	{
		int GearItems = READ_BYTE();
		for (int i = 0; i < GearItems; i++)
		{
			gearinfo_t GearInfo;
			GearInfo.Flags = READ_BYTE();
			GearInfo.Model = READ_SHORT();
			GearInfo.Body = READ_SHORT();
			GearInfo.Skin = READ_SHORT();
			GearInfo.Anim = READ_BYTE();
			CharSlot.GearInfo.add(GearInfo);
		}
	}
	else
	{
		clrmem(CharSlot);

		CharSlot.Index = CharIndex;
		CharSlot.Status = (chardatastatus_e)READ_BYTE();				//Char status (unloaded/pending/loaded)
		CharSlot.Location = (charloc_e)READ_BYTE();					//Char location (client/server/central network)

		if (CharSlot.Status == CDS_LOADED)
		{
			CharSlot.Name = READ_STRING();
			CharSlot.MapName = READ_STRING();
			CharSlot.OldTrans = READ_STRING();
			CharSlot.NextMap = READ_STRING();
			CharSlot.NewTrans = READ_STRING();
			CharSlot.body = READ_SHORT(); //MiB JAN2010_27 - Char Selection Fix
			//CharSlot.Race = READ_STRING( ); // MIB FEB2015_21 [RACE_MENU] - Read the character's race
			byte CharFlags = READ_BYTE();
			CharSlot.IsElite = FBitSet(CharFlags, (1 << 0));
			CharSlot.Gender = FBitSet(CharFlags, (1 << 1)) ? GENDER_FEMALE : GENDER_MALE;
			CharSlot.JoinType = FBitSet(CharFlags, (1 << 2)) ? JN_TRAVEL : JN_NOTALLOWED;
			//Thothie FEB2011_02 - invisible new character fix (default based on gender)
			if (CharSlot.body == 0)
			{
				CharSlot.body = 40;
				if (CharSlot.Gender == GENDER_FEMALE) CharSlot.body = 80;
			}

			if (CharSlot.Location == LOC_CLIENT)
			{
				CharSlot.m_SendStatus = CSS_SENT;	//I just finished sending this char to the server
				player.m_CharSend.Status = CSS_DORMANT;
			}
		}
	}

	if (MSCLGlobals::CharPanelActive)
		if (gViewPort && gViewPort->m_pCurrentMenu)
			((CNewCharacterPanel*)gViewPort->m_pCurrentMenu)->Update();

	enddbg;
	return 1;
}

void ChooseChar_Interface::UpdateCharScreen()
{
	if (MSCLGlobals::CharPanelActive)
		if (gViewPort && gViewPort->m_pCurrentMenu)
			((CNewCharacterPanel*)gViewPort->m_pCurrentMenu)->Update();
}
void ChooseChar_Interface::UpdateCharScreenUpload()
{
	if (MSCLGlobals::CharPanelActive)
		if (gViewPort && gViewPort->m_pCurrentMenu)
			((CNewCharacterPanel*)gViewPort->m_pCurrentMenu)->UpdateUpload();
}

//CRenderChar

#define TIME_MINIDLE 6
#define TIME_MAXIDLE 60
#define CHAR_SCALE 0.025f
void CRenderChar::Init( int Idx ) 
{ 
	Init( Idx, MODEL_HUMAN_REF ); 
} // MIB FEB2015_21 [RACE_MENU] - Changed this to pass-through, allows model as optional parm
void CRenderChar::Init( int Idx, msstring model )
{
	clrmem( m_Ent );

	m_Done = false;
	m_Idx = Idx;
	int WorldNumber = MSGlobals::ClEntities[CLPERMENT_INSET] + m_Idx;  //There just happens to be 3 model slots, and none are used at this time
	m_Ent.SetModel( model.c_str() ); // MIB FEB2015_21 [RACE_MENU] - Set the model provided
	m_Ent.index = WorldNumber;
	m_Ent.curstate.number = WorldNumber;
	m_Ent.curstate.scale = CHAR_SCALE;
	m_Ent.curstate.framerate = 1.0f;
	SetBits( m_Ent.curstate.effects, EF_NOINTERP );
	SetBits( m_Ent.curstate.oldbuttons, MSRDR_NOREFLECT );
 	SetBits( m_Ent.curstate.colormap, MSRDR_ASPLAYER );
	
	m_Gender = GENDER_MALE;

	for (int i = 0; i < 4; i++) 
		m_Ent.latched.prevcontroller[i] = m_Ent.curstate.controller[i] = 127;
		
 	m_Ent.curstate.gaitsequence = 0;
	m_TimeRandomIdle = gpGlobals->time + RANDOM_FLOAT(TIME_MINIDLE,TIME_MAXIDLE);
	SetActive( false );
	m_Highlighted = false;
	m_ReturnToAttention = false;
}

void CRenderChar::Render( )
{
	startdbg;

	if( !MSCLGlobals::CharPanelActive )
		return;
	if( !gViewPort || !gViewPort->m_pCurrentMenu )
		return;

	if( ((CNewCharacterPanel*)gViewPort->m_pCurrentMenu)->m_Stage != m_Stage )
		return;

	Vector vForward, vRight, vUp;
	EngineFunc::MakeVectors( ViewMgr.Angles, vForward, vRight, vUp );

	m_Ent.origin = ViewMgr.Origin + vForward * 5.0f + vUp * 0.4;
	if( m_Stage == STG_CHOOSECHAR )
		m_Ent.origin += (vRight * 2 * (m_Idx-1));
	else if( m_Stage == STG_CHOOSEGENDER )
		m_Ent.origin += (vRight * -1.3 + vRight * 2 * (m_Idx));

	m_Ent.angles = Vector( ViewMgr.Angles.x, ViewMgr.Angles.y + 180, 0 );
	m_Ent.curstate.angles = m_Ent.angles;
	m_Ent.curstate.origin = m_Ent.origin;
	m_Ent.curstate.body = 0;

	//Handle gear
	//Thothie FEB2011_02 - don't figure gear for gender entries, or will inherit from other slots
	if( m_Stage != STG_CHOOSEGENDER )
	{
		m_GearItems.clearitems( );
		m_Gear.clearitems( );
		uint BodyParts[HUMAN_BODYPARTS] = { 0 };
		
		for (int i = 0; i < player.m_CharInfo[m_Idx].GearInfo.size(); i++) 
		{
			gearinfo_t &GearInfo = player.m_CharInfo[m_Idx].GearInfo[i];
			CGenericItem pItem = CGenericItem();
			cl_entity_t &ItemEnt = GearItemEntity( pItem );

			//ItemEnt = m_Ent;
			ItemEnt.index = -1;
			ItemEnt.curstate.number = ItemEnt.index;
			ItemEnt.curstate.impacttime = m_Ent.curstate.impacttime;
			ItemEnt.model = IEngineStudio.GetModelByIndex( GearInfo.Model );
			ItemEnt.curstate.modelindex = GearInfo.Model;
			ItemEnt.curstate.body = GearInfo.Body;
			ItemEnt.curstate.skin = GearInfo.Skin;
			ItemEnt.PlayAnim( GearInfo.Anim );

			for (int i = 0; i < HUMAN_BODYPARTS; i++)
			{
				if( FBitSet( GearInfo.Flags, (1<<i) ) )
					BodyParts[i] = 1;
			}

			//ItemEnt.curstate.frame = 0;
			//ItemEnt.curstate.impacttime = CHAR_SCALE;
			//ItemEnt.index = MSGlobals::ClEntities[CLPERMENT_INSET];
			//ItemEnt.curstate.number = ItemEnt.index;
			//ItemEnt.origin = m_Ent.origin;
			//ItemEnt.angles = m_Ent.angles;
			//ItemEnt.curstate.origin = ItemEnt.origin;
			//ItemEnt.curstate.angles = ItemEnt.angles;
			//ItemEnt.curstate.framerate = 1.0f;


			ItemEnt.curstate.colormap = ItemEnt.curstate.oldbuttons = 0;
			SetBits( ItemEnt.curstate.oldbuttons, MSRDR_NOREFLECT );
			SetBits( ItemEnt.curstate.colormap, MSRDR_LIGHT_NORMAL );

			m_GearItems.add( pItem );

			pItem.RemoveFromOwner();
			pItem.Container_RemoveAllItems();
			pItem.Deactivate();

		}
		for (int i = 0; i < m_GearItems.size(); i++) 
			m_Gear.add( &m_GearItems[i] );
	}

	if( m_Active )
	{
		if( m_AnimState == RCS_FIDGET && m_Ent.curstate.frame >= 1 )
		{
			m_ReturnToAttention = true;
			m_Ent.curstate.frame = 1;
		}
		else if( m_AnimState == RCS_IDLE && gpGlobals->time > m_TimeRandomIdle )
		{
			m_Ent.PlayAnim( MSCLGlobals::DefaultHUDCharAnims.Fidget.c_str() );
			m_AnimState = RCS_FIDGET;
		}

		//Got un-highlighted while playing highlight anim (jump).  Wait until highlight anim is done, then go back to attention
		if( m_ReturnToAttention && m_Ent.curstate.frame >= 1 )
			PlayAttnAnim( );
		
	}

	CRenderPlayer::Render( );

	//Print("Idx: %i Body: %1 %1 %1 %1 \n",m_Idx,BodyParts[0],BodyParts[1],BodyParts[2],BodyParts[3]);

	/*m_Ent.SetBody( 0 , BodyParts[1] ); //Set the head
	m_Ent.SetBody( 1 , BodyParts[0] + BodyParts[2] ); //Set the body*/
	//MIB JAN2010_27 - Char Selection Fix
	//Thothie FEB2011_02 - fixing gender bender
	if ( m_Stage != STG_CHOOSEGENDER)
	{
		m_Ent.curstate.body = player.m_CharInfo[m_Idx].body;
	}
	else
	{
		if ( m_Gender == GENDER_MALE )
		{
			m_Ent.SetBody(0,1);
			m_Ent.SetBody(1,1);
			m_Ent.SetBody(2,1);
			m_Ent.SetBody(3,1);
		}
		else
		{
			m_Ent.SetBody(0,2);
			m_Ent.SetBody(1,2);
			m_Ent.SetBody(2,2);
			m_Ent.SetBody(3,2);
		}
		
	}


	//foreach( i, HUMAN_BODYPARTS )
	//	m_Ent.SetBody( i, BodyParts[i] );

	enddbg;
}
void CRenderChar::PlayAttnAnim( )
{
	//Determine if an item is being held
	m_ItemInHand = false;

	if( m_Stage == STG_CHOOSECHAR )
		 for (int i = 0; i < player.m_CharInfo[m_Idx].GearInfo.size(); i++) 
			if( !FBitSet( player.m_CharInfo[m_Idx].GearInfo[i].Flags,GEARFL_WEARING ) ) { m_ItemInHand = true; break; }


	m_Ent.PlayAnim( m_ItemInHand ? MSCLGlobals::DefaultHUDCharAnims.Idle_Weapon.c_str() : MSCLGlobals::DefaultHUDCharAnims.Idle_NoWeapon.c_str() );
	m_AnimState = RCS_IDLE;
	m_TimeRandomIdle = gpGlobals->time + RANDOM_FLOAT(TIME_MINIDLE,TIME_MAXIDLE);
	m_ReturnToAttention = false;
}

void CRenderChar::SetActive( bool Active )
{
	m_Active = Active;
	ClearBits( m_Ent.curstate.colormap, MSRDR_LIGHT_NORMAL|MSRDR_LIGHT_DIM );

	if( Active )
	{
		PlayAttnAnim( );
		Highlight( false );
		SetBits( m_Ent.curstate.colormap, MSRDR_LIGHT_NORMAL );
	}
	else
	{
		if( player.m_CharSend.Index == m_Idx && player.m_CharSend.Status == CSS_SENDING )
			m_Ent.PlayAnim( MSCLGlobals::DefaultHUDCharAnims.Uploading.c_str() );
		else
			m_Ent.PlayAnim( MSCLGlobals::DefaultHUDCharAnims.Inactive.c_str() );
		m_AnimState = RCS_INACTIVE;
		SetBits( m_Ent.curstate.colormap, MSRDR_LIGHT_DIM );
	}
}
void CRenderChar::Highlight( bool Highlighted )
{
	m_Highlighted = Highlighted;
	if( Highlighted )
	{
		m_Ent.PlayAnim( MSCLGlobals::DefaultHUDCharAnims.Highlighted.c_str() );
		m_AnimState = RCS_HIGHLIGHT;
		//m_Ent.PlayAnim( ANIM_JUMP );
	}
	else
	{
		if( m_AnimState == RCS_HIGHLIGHT )
			m_ReturnToAttention = true;
	}
}
void CRenderChar::Select( )
{
}
CRenderChar::~CRenderChar( )
{
	if( !m_Done )
	{
		//This is bad - the char gets deallocated before it gets unregistered
		MSErrorConsoleText( "~CRenderChar()", "Deallocated improperly" );
	}
}


//Spawn box
void CRenderSpawnbox::Init( )
{
	m_Done = false;
	clrmem( m_Ent );
	bool FoundModel = m_Ent.SetModel( MSGlobals::DefaultSpawnBoxModel );
	if( FoundModel )
	{
		m_Ent.index = m_Ent.curstate.number = -1;
		SetBits( m_Ent.curstate.effects, EF_NOINTERP );
		Register( );
	}
	else
	{
		Print("Spawnbox Model %s NOT FOUND!", MSGlobals::DefaultSpawnBoxModel.c_str());
	}
}
void CRenderSpawnbox::Render( )
{
	startdbg;
	Vector vForward, vRight, vUp;
	EngineFunc::MakeVectors( ViewMgr.Angles, vForward, vRight, vUp );
	m_Ent.origin = ViewMgr.Origin;
	m_Ent.angles = Vector( ViewMgr.Angles.x, ViewMgr.Angles.y + 180, 0 );
 	m_Ent.curstate.angles = m_Ent.angles;
	m_Ent.curstate.origin = m_Ent.origin;

	CRenderEntity::Render( );
	enddbg;
}
CRenderSpawnbox::~CRenderSpawnbox( )
{
	if( !m_Done )
	{
		//This is bad - the ent gets deallocated before it gets unregistered
		MSErrorConsoleText( "~CRenderChar()", "Deallocated improperly" );
	}
}


CNewCharacterPanel::~CNewCharacterPanel( )
{
}
