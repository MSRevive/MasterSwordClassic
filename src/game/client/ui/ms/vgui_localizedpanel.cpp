// MiB MAR2015_01 [LOCAL_PANEL] - Definition of localized panel

#include "hud.h"
#include "cl_util.h"
#include "vgui_teamfortressviewport.h"
#include "VGUI_TextImage.h"
#include "parsemsg.h"

#include "sharedutil.h"

#include "vgui_localizedpanel.h"
#include "vgui_stats.h"
#include "vgui_menudefsshared.h"
#include "iscript.h"
#include "hudscript.h"

#define LOCAL_TRANSPARENCY 60
#define LOCAL_MAINPANEL_RATIO (.875)
#define LOCAL_MAINPANEL_WIDTH (LOCAL_MAINPANEL_RATIO * ScreenWidth)
#define LOCAL_MAINPANEL_HEIGHT (LOCAL_MAINPANEL_RATIO * ScreenHeight)

#define LOCAL_SCROLLPANEL_X_SPACE_L XRES(10)
#define LOCAL_SCROLLPANEL_X_SPACE_R LOCAL_SCROLLPANEL_X_SPACE_L
#define LOCAL_SCROLLPANEL_Y_SPACE_T YRES(30)
#define LOCAL_SCROLLPANEL_Y_SPACE_B YRES(10) + LOCAL_BUTTON_HEIGHT
#define LOCAL_SCROLLPANEL_WIDTH LOCAL_MAINPANEL_WIDTH - (LOCAL_SCROLLPANEL_X_SPACE_L + LOCAL_SCROLLPANEL_X_SPACE_R)
#define LOCAL_SCROLLPANEL_HEIGHT LOCAL_MAINPANEL_HEIGHT - (LOCAL_SCROLLPANEL_Y_SPACE_T + LOCAL_SCROLLPANEL_Y_SPACE_B)

#define LOCAL_TEXTPANEL_HEIGHT_MIN LOCAL_SCROLLPANEL_HEIGHT - YRES(10)

#define LOCAL_CLOSEBUTTON_WIDTH LOCAL_MAINPANEL_WIDTH / 8
#define LOCAL_CLOSEBUTTON_HEIGHT LOCAL_MAINPANEL_HEIGHT / 12

#define LOCAL_BUTTON_WIDTH LOCAL_MAINPANEL_WIDTH / 6
#define LOCAL_BUTTON_HEIGHT LOCAL_MAINPANEL_HEIGHT / 16
#define LOCAL_BUTTON_SPACE XRES(10)
#define LOCAL_BUTTON_Y LOCAL_SCROLLPANEL_Y_SPACE_T + LOCAL_SCROLLPANEL_HEIGHT + ((LOCAL_MAINPANEL_HEIGHT - (LOCAL_SCROLLPANEL_Y_SPACE_T + LOCAL_SCROLLPANEL_HEIGHT)) / 2 - LOCAL_BUTTON_HEIGHT / 2)

// Find the starting position based on parent size, child size, total number of children, and size of space between
int GetCenteredItem(int iParent, int iChild, int iChildren, int iSpace)
{
	return (iParent / 2.0f) - (iChild / 2.0f * iChildren + (iSpace / 2.0f * iChildren - 1));
}

// Check if a localized value exists
bool LocalExists(msstring title)
{
	return msstring(Localized(title)) != title;
}

CLocalizedPanel::CLocalizedPanel(Panel *pParent)
	: CMenuPanel(LOCAL_TRANSPARENCY, false, 0, 0, ScreenWidth, ScreenHeight)
{
	setParent(pParent);
	m_Name = LOCAL_MENU_NAME;
	m_pLocalScriptedEntity = NULL;
	m_ServerEntString = "";

	m_iButtonTotal = 0;
	m_iSubPanelTotal = 0;

	int xMainPanel = GetCenteredItem(ScreenWidth, LOCAL_MAINPANEL_WIDTH, 1, 0);
	int yMainPanel = GetCenteredItem(ScreenHeight, LOCAL_MAINPANEL_HEIGHT, 1, 0);
	m_pMainPanel = new CTransparentPanel(LOCAL_TRANSPARENCY, xMainPanel, yMainPanel, LOCAL_MAINPANEL_WIDTH, LOCAL_MAINPANEL_HEIGHT);
	m_pMainPanel->setParent(this);
	m_pMainPanel->setVisible(true);

	m_pScroll = new CTFScrollPanel(LOCAL_SCROLLPANEL_X_SPACE_L, LOCAL_SCROLLPANEL_Y_SPACE_T, LOCAL_SCROLLPANEL_WIDTH, LOCAL_SCROLLPANEL_HEIGHT);
	m_pScroll->setParent(m_pMainPanel);
	m_pScroll->setScrollBarAutoVisible(false, true);
	m_pScroll->setScrollBarVisible(false, false);
	m_pScroll->setBorder(new LineBorder(2, Color(128, 128, 128, 128)));
	m_pScroll->validate();

	m_pTextPanel = new Panel(0, 0, (m_pScroll->getWide() - m_pScroll->getVerticalScrollBar()->getWide()), LOCAL_TEXTPANEL_HEIGHT_MIN);
	m_pTextPanel->setBgColor(0, 0, 0, 255);
	m_pTextPanel->setParent(m_pScroll->getClient());
	m_pTextPanel->setVisible(true);

	m_pTitle = new TextPanel("", 0, 0, 100, 100); // Automatically sets its size later, just fake it right now
	m_pTitle->setBgColor(0, 0, 0, 255);
	m_pTitle->setFgColor(Scheme::sc_primary1);
	m_pTitle->setVisible(true);
	m_pTitle->setParent(m_pMainPanel);

	m_pCloseButton = new MSButton(this, Localized("#CLOSE"), (xMainPanel + LOCAL_MAINPANEL_WIDTH) - LOCAL_CLOSEBUTTON_WIDTH, yMainPanel - LOCAL_CLOSEBUTTON_HEIGHT, LOCAL_CLOSEBUTTON_WIDTH, LOCAL_CLOSEBUTTON_HEIGHT, COLOR(255, 0, 0, 0), COLOR(100, 100, 100, 0));

	m_pCloseButton->setParent(this);
	m_pCloseButton->setFont(g_FontID);
	m_pCloseButton->addActionSignal(new CCallback_Signal(this, true, 0, ""));
	m_pCloseButton->setContentAlignment(vgui::Label::Alignment::a_center);
	m_pCloseButton->setVisible(true);
}

CLocalizedPanel::~CLocalizedPanel()
{
	Reset();
	delete m_pTextPanel;
	delete m_pScroll;
	delete m_pTitle;
	delete m_pCloseButton;
	delete m_pMainPanel;
}

void CLocalizedPanel::Show(void)
{
	ShowVGUIMenu(MENU_LOCAL);
}

void CLocalizedPanel::Open(void)
{
	UpdateCursorState();
	setVisible(true);
}

void CLocalizedPanel::Hide(void)
{
	VGUI::HideMenu(this);
}

void CLocalizedPanel::Close(void)
{
	for (int i = 0; i < m_iButtonTotal; i++)
		m_ButtonList[i]->setArmed(false);
	m_pCloseButton->setArmed(false);
	setVisible(false);

	msstring sCloseCallback = "local_panel_close";
	if (m_ServerEntString.len())
		CallbackServer(sCloseCallback);
	else if (m_pLocalScriptedEntity)
		CallbackClient(sCloseCallback);

	UpdateCursorState();
}

void CLocalizedPanel::Reset(void)
{
	Hide();
	ClearButtons();
	ClearSubPanels();
	m_pScroll->setScrollValue(0, 0);
}

int CLocalizedPanel::GetNextY(void)
{
	int	xPos = 0;
	int	yPos = 0;

	if ( m_iSubPanelTotal )
	{
		Panel	*pLast = m_SubPanelList[m_iSubPanelTotal-1];
		pLast->getPos( xPos, yPos );
		yPos += pLast->getTall();
	}

	return yPos;
}

void CLocalizedPanel::SetServerEntString(msstring sEntString)
{
	m_ServerEntString = sEntString;
}

void CLocalizedPanel::SetClientScriptedEntity(IScripted *pEntity)
{
	m_pLocalScriptedEntity = pEntity;
}

void CLocalizedPanel::SetPanelTitle(msstring title)
{
	m_pTitle->setText(Localized(title));

	int w, h;
	m_pTitle->getTextImage()->getTextSize(w, h);
	m_pTitle->setSize(w, h);
	m_pTitle->setPos(LOCAL_SCROLLPANEL_X_SPACE_L, GetCenteredItem(LOCAL_SCROLLPANEL_Y_SPACE_T, h, 1, 0));
}

void CLocalizedPanel::AddButton(msstring sText, bool bEnabled, bool bCloseOnClick, int cbType, msstring sCallback, msstring sCallbackData)
{
	ClassButton *pButton = new ClassButton(0, Localized(sText), 0, LOCAL_BUTTON_Y, LOCAL_BUTTON_WIDTH, LOCAL_BUTTON_HEIGHT, !bEnabled);
	pButton->setParent(m_pMainPanel);
	pButton->addActionSignal(new CCallback_Signal(this, bCloseOnClick, cbType, sCallback, sCallbackData));
	pButton->setContentAlignment(vgui::Label::Alignment::a_center);
	pButton->setVisible(true);
	pButton->setEnabled(bEnabled);
	if (!bEnabled)
		pButton->setFgColor(128, 128, 128, 128);
	m_ButtonList.add(pButton);
	++m_iButtonTotal;

	PositionButtons();
}

void CLocalizedPanel::PositionButtons(void)
{
	int x = GetCenteredItem(m_pMainPanel->getWide(), LOCAL_BUTTON_WIDTH, m_iButtonTotal, LOCAL_BUTTON_SPACE);
	for (int i = 0; i < m_iButtonTotal; i++)
	{
		m_ButtonList[i]->setPos(x + i * (LOCAL_BUTTON_WIDTH + LOCAL_BUTTON_SPACE), LOCAL_BUTTON_Y);
	}
}

void CLocalizedPanel::ClearButtons(void)
{
	for (int i = 0; i < m_iButtonTotal; i++)
	{
		m_ButtonList[i]->setVisible(false);
		m_pMainPanel->removeChild(m_ButtonList[i]);
	}
	m_ButtonList.clear();
	m_iButtonTotal = 0;
}

void CLocalizedPanel::DoCallback(bool bDoClose, int cbType, msstring sCallback, msstring sCallbackData)
{
	if (bDoClose)
		Hide();
	if (cbType == 1)
		CallbackServer(sCallback, sCallbackData);
	if (cbType == 2)
		CallbackClient(sCallback, sCallbackData);
}

void CLocalizedPanel::CallbackServer(msstring sCallback, msstring sCallbackData)
{
	msstring cmd = "localcb \"";
	cmd += m_ServerEntString + "\" ";
	cmd += sCallback;
	if(sCallbackData.len())
	{
		cmd += msstring( " \"" ) + sCallbackData + "\"";
	}
	cmd += "\n";

	ServerCmd(cmd.c_str());
}

void CLocalizedPanel::CallbackClient(msstring sCallback, msstring sCallbackData)
{
	msstringlist Params;
	Params.clear();
  if (sCallbackData.len())
  {
    Params.add(sCallbackData);
  }
  gHUD.m_HUDScript->CallScriptEvent( sCallback, &Params );
}

void CLocalizedPanel::AddImage(const char *pszName, bool bIsTga, bool bBorder, int vFrame)
{
	int	yPos = GetNextY();
	int vPanelW	= m_pTextPanel->getWide();
	CImageDelayed	*pImg;
	Panel	*pWrapper;

	pImg = new CImageDelayed( pszName, bIsTga, false, 0, 0 );
	if ( !bIsTga )
	{
		pImg->SetFrame( vFrame );
	}
	if ( bBorder )
	{
		pImg->setBorder( new LineBorder( 2, Color(128, 128, 128, 128) ) );
	}
	pImg->setVisible( true );
	pImg->setPos( (vPanelW - pImg->getWide()) / 2, 0 );

	pWrapper = new Panel( 0, yPos, vPanelW, pImg->getTall() );
	pWrapper->addChild( pImg );
	AddSubPanel( pWrapper );
	m_pScroll->validate();
}

void CLocalizedPanel::AddSubPanel( Panel *pPanel )
{
	pPanel->setParent( m_pTextPanel );
	pPanel->setVisible( true );

	m_SubPanelList.add( pPanel );
	++m_iSubPanelTotal;
}

void CLocalizedPanel::ClearSubPanels( void )
{
	for(int i = 0; i < m_iSubPanelTotal; i++)
	{
		m_pTextPanel->removeChild( m_SubPanelList[i] );
		delete m_SubPanelList[i];
	}

	m_pTextPanel->setSize( m_pTextPanel->getWide() , LOCAL_TEXTPANEL_HEIGHT_MIN );
	m_SubPanelList.clear();
	m_iSubPanelTotal = 0;
}

void CLocalizedPanel::AddParagraph(const char *pszText)
{
	int xPos = 0, yPos = 0, w = 0, hParagraph = 0, vStart = 0, hPanel;
	TextPanel *pParagraph;
	msstring vsFontName;

	if ( pszText[0] == '!' )
	{
		msstring vTest = msstring( pszText );

		if ( vTest.starts_with( "!TGA" ) )
		{
			vTest = vTest.substr( 4 );
			bool bBorder = false;
			if ( vTest.starts_with( "_BORDER" ) )
			{
				vTest = vTest.substr( 7 );
				bBorder = true;
			}

			msstring vImage = vTest.substr( 1 );
			AddImage(vImage, true, bBorder);
			return;
		}

		if ( vTest.starts_with( "!SPRITE" ) )
		{
			vTest = vTest.substr( 7 );
			bool bBorder	= false;
			int	vFrame = 0;
			if ( vTest.starts_with( "_BORDER" ) )
			{
				vTest = vTest.substr( 7 );
				bBorder	= true;
			}

			if ( vTest.starts_with( "_" ) )
			{
				int	vSpace = vTest.findchar( " " );
				vFrame = atoi( vTest.substr( 1, vSpace - 1 ) );
				vTest = vTest.substr( vSpace );
			}

			msstring	vImage = vTest.substr( 1 );
			AddImage( vImage, false, bBorder, vFrame );
			return;
		}

		if ( vTest.starts_with( "!FONT_" ) )
		{
			vTest = vTest.substr( 6 );
			int	vSpace = vTest.findchar( " " );
			if ( vSpace == msstring_error ) vSpace = vTest.len();
			vsFontName = vTest.substr( 0, vSpace );
			vStart += vSpace + 7;
		}
	}

	pParagraph = new TextPanel(strlen(pszText) ? pszText : "\t" // Hack, without this blank lines have no size
							   ,
							   0, yPos, w = m_pTextPanel->getWide(), LOCAL_SCROLLPANEL_HEIGHT);
	if(!vsFontName.len())
	{
		vsFontName = "Nrml";
	}

	CSchemeManager * pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t vSchemeHandle = pSchemes->getSchemeHandle( vsFontName );
	int r,g,b,a;

	pParagraph->setFont(pSchemes->getFont(vSchemeHandle));
	pSchemes->getFgColor(vSchemeHandle, r, g, b, a);
	pParagraph->setFgColor(r, g, b, a);
	pSchemes->getBgColor(vSchemeHandle, r, g, b, a);
	pParagraph->setBgColor(r, g, b, a);
	AddSubPanel(pParagraph);

	// Word-wrap and resize height
	pParagraph->getTextImage()->getTextSizeWrapped(w, hParagraph);
	pParagraph->setSize(w, hParagraph);
	hPanel = V_max(LOCAL_TEXTPANEL_HEIGHT_MIN, yPos+hParagraph);
	m_pTextPanel->setSize(m_pTextPanel->getWide(), hPanel);

	m_pScroll->validate();
}

void CLocalizedPanel::ReadParagraphsFromLocalized(msstring title)
{
	bool bDidSet = false;
	if (LocalExists(title))
	{
		AddParagraph(Localized(title));
		bDidSet = true;
	}
	else
	{
		msstring par = title + "_PAR";
		for (int i = 0; LocalExists(par + i); ++i)
		{
			bDidSet = true;
			AddParagraph(Localized(par + i));
		}
	}

	if (!bDidSet)
	{
		AddParagraph(title);
		bDidSet = true;
	}
}

void CLocalizedPanel::ReadParagraphsFromFile(msstring fname)
{
	if (!fname.ends_with(".txt"))
		fname += ".txt";
	char cFileName[256];
	_snprintf(cFileName, sizeof(cFileName),  "%s/%s",  EngineFunc::GetGameDir(),  fname.c_str() );

	std::ifstream file;
	file.open(cFileName);
	if (file.is_open())
	{
		file.seekg(0, std::ios::end);
		long len = file.tellg();
		file.seekg(0, std::ios::beg);

		char *pszText = new char[len];
		file.read(pszText, len);

		char *pszLast = pszText;
		char *pszPara = strchr(pszLast, '\n');
		while (pszPara)
		{
			pszPara[0] = 0;
			AddParagraph(pszLast);
			pszLast = pszPara + 1;
			pszPara = strchr(pszLast, '\n');
		}
		AddParagraph(pszLast);
		delete pszText;
	}
	else
	{
		msstring sNotFound = "FILE NOT FOUND: ";
		sNotFound += fname;
		AddParagraph(sNotFound.c_str());
	}
}
