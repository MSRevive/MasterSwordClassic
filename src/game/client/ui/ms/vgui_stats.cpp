//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Personal Stats display panel
//
//=============================================================================

#include "msdllheaders.h"
#include "player/player.h"
#undef DLLEXPORT

#include <VGUI_TextImage.h>

#include "../hud.h"
#include "../cl_util.h"
#include "../parsemsg.h"

#include "../vgui_teamfortressviewport.h"

//Master Sword
#include "vgui_stats.h"
#include "vgui_menudefsshared.h"
#include "stats/stats.h"

long double GetExpNeeded(int StatValue);

int TestExpArray[SKILL_MAX_ATTACK][STAT_MAGIC_TOTAL];

char *GenderList[] =
	{
		"Male",
		"Female"};

//------------

// Menu Dimensions
#define MAINWINDOW_X XRES(40)
#define MAINWINDOW_Y YRES(40)
#define MAINWINDOW_SIZE_X XRES(330)
#define MAINWINDOW_SIZE_Y YRES(270)
#define MAINLABEL_TOP_Y YRES(0)
#define MAINBUTTON_SIZE_X XRES(132)
#define MAINBUTTON_SIZE_Y YRES(16)
#define TITLE_SKILLS_X XRES(148)
#define TITLE_GENINFO_X XRES(12)
#define SKILLTYPEBUTTON_SIZE_Y YRES(20)
#define SKILLINFOPANEL_SIZE_X XRES(225)
#define SKILLINFOPANEL_TITLE_Y YRES(3)
#define SKILLINFOPANEL_TITLE_H YRES(20)
#define SKILLINFOPANEL_BTM_BORDERSPACER_H YRES(4)

#define SCROLLWIN_X XRES(0)
#define SCROLLWIN_SPACER_Y YRES(2)

static COLOR Color_TitleText = COLOR(255, 255, 255, 0),
			 Color_NormalText = COLOR(190, 190, 190, 0),
			 Color_InstructionText = COLOR(128, 128, 128, 0),
			 Color_SelectedText = COLOR(255, 0, 0, 0),
			 Color_TransparentText = COLOR(0, 0, 0, 255);

// Creation
CStatPanel::CStatPanel(Panel *pParent) : CMenuPanel(0, false, 0, 0, ScreenWidth, ScreenHeight)
{
	startdbg;
	setParent(pParent);
	setVisible(false);

	m_Name = "stats";
	m_NoMouse = true;
	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle("Title Font");
	SchemeHandle_t hClassWindowText = pSchemes->getSchemeHandle("Briefing Text");

	SetBits(m_Flags, MENUFLAG_CLOSEONESC | MENUFLAG_TRAPSTEPINPUT);

	//Main panel
	pMainPanel = new CTransparentPanel(60, MAINWINDOW_X, MAINWINDOW_Y, MAINWINDOW_SIZE_X, MAINWINDOW_SIZE_Y);
	pMainPanel->setParent(this);
	pMainPanel->setBorder(new LineBorder(2, Color(0, 128, 0, 0)));

	// Create the title
	Label *pTitleLabel = new MSLabel(pMainPanel, "", XRES(6), YRES(2));
	pTitleLabel->setFont(pSchemes->getFont(hTitleScheme));
	pTitleLabel->SetFGColorRGB(Color_TitleText);
	pTitleLabel->setText("Character Info"); //Set text here so the font gets recognized

	int TitlePosX, TitlePosY, TitleSizeX, TitleSizeY;
	pTitleLabel->getPos(TitlePosX, TitlePosY);
	pTitleLabel->getSize(TitleSizeX, TitleSizeY);

	// Create the Scroll panel
	m_pScrollPanel = new CTFScrollPanel(SCROLLWIN_X, TitlePosY + TitleSizeY, pMainPanel->getWide(), pMainPanel->getTall());
	m_pScrollPanel->setParent(pMainPanel);
	m_pScrollPanel->setScrollBarAutoVisible(false, true);
	m_pScrollPanel->setScrollBarVisible(false, false);

	MSLabel *pLabel = new MSLabel(m_pScrollPanel->getClient(), "General Information", TITLE_GENINFO_X - XRES(2), YRES(3));
	pLabel->SetFGColorRGB(Color_TitleText);

	int ix, iy;
	pLabel->getPos(ix, iy);

	int i, offset = iy + pLabel->getTall();

	for (i = 0; i < INFO_STAT_NUM; i++)
	{
		TextPanel *pTextbox = Info_StatLabel[i] =
			new TextPanel("", TITLE_GENINFO_X, offset + i * MAINBUTTON_SIZE_Y, MAINBUTTON_SIZE_X, MAINBUTTON_SIZE_Y);
		pTextbox->setParent(m_pScrollPanel->getClient());
		pTextbox->setFont(pSchemes->getFont(hClassWindowText));
		pTextbox->SetFGColorRGB(Color_NormalText);
		pTextbox->SetBGColorRGB(Color_TransparentText);
	}

	offset += INFO_STAT_NUM * MAINBUTTON_SIZE_Y + MAINBUTTON_SIZE_Y;

	for (i = 0; i < NATURAL_MAX_STATS; i++)
	{
		TextPanel *pTextbox = Nat_StatLabel[i] =
			new TextPanel("", TITLE_GENINFO_X, offset + i * MAINBUTTON_SIZE_Y, MAINBUTTON_SIZE_X, MAINBUTTON_SIZE_Y);
		pTextbox->setParent(m_pScrollPanel->getClient());
		pTextbox->setFont(pSchemes->getFont(hClassWindowText));
		pTextbox->SetFGColorRGB(Color_NormalText);
		pTextbox->SetBGColorRGB(Color_TransparentText);

#if 0
		TextPanel *pModbox = Nat_StatModsLabel[i] =
			new TextPanel( "", TITLE_GENINFO_X + XRES(80), offset + i * MAINBUTTON_SIZE_Y, MAINBUTTON_SIZE_X, MAINBUTTON_SIZE_Y );
		pModbox->setParent( m_pScrollPanel->getClient() );
		pModbox->setFont( pSchemes->getFont(hClassWindowText) );
		pModbox->SetBGColorRGB( Color_TransparentText );
#endif
	}

	pLabel = new MSLabel(m_pScrollPanel->getClient(), "Skills", TITLE_SKILLS_X - XRES(2), YRES(3));
	pLabel->SetFGColorRGB(Color_TitleText);

	pLabel->getPos(ix, iy);
	offset = iy + pLabel->getTall();

	for (int i = 0; i < SKILL_MAX_STATS + 1; i++)
	{
		TextPanel *pTextbox = Skill_StatLabel[i] =
			new TextPanel("", TITLE_SKILLS_X, offset + i * MAINBUTTON_SIZE_Y, MAINBUTTON_SIZE_X, MAINBUTTON_SIZE_Y);
		pTextbox->setParent(m_pScrollPanel->getClient());
		pTextbox->setFont(pSchemes->getFont(hClassWindowText));
		pTextbox->setBgColor(0, 0, 0, 255);
	}

	pLabel = new MSLabel(pMainPanel, "Use PGUP/PGDN to view skill info.", TITLE_GENINFO_X, MAINWINDOW_SIZE_Y - MAINBUTTON_SIZE_Y - YRES(5), MAINWINDOW_SIZE_X, MAINBUTTON_SIZE_Y);
	pLabel->SetFGColorRGB(Color_InstructionText);

	//Setup the Skill Info Pane
	pMainPanel->getPos(ix, iy);
	m_InfoPanel = new CTransparentPanel(255, 0, 0, m_pScrollPanel->getWide(), MAINWINDOW_SIZE_Y);
	m_InfoPanel->setParent(this);
	m_InfoPanel->setPos(ix + MAINWINDOW_SIZE_X + XRES(16), iy);
	m_InfoPanel->setSize(SKILLINFOPANEL_SIZE_X, YRES(96));
	m_InfoPanel->m_iTransparency = pMainPanel->m_iTransparency;
	m_InfoPanel->setBorder(new LineBorder(2, Color(0, 128, 0, 0)));

	m_SkillInfoLabel = pLabel =
		new MSLabel(m_InfoPanel, "", 0, SKILLINFOPANEL_TITLE_Y, SKILLINFOPANEL_SIZE_X, SKILLINFOPANEL_TITLE_H, MSLabel::a_center);
	pLabel->SetFGColorRGB(Color_TitleText);

	offset = SKILLINFOPANEL_TITLE_Y + SKILLINFOPANEL_TITLE_H;

	for (i = 0; i < STAT_MAGIC_TOTAL; i++)
	{
		pLabel = m_StatTypeLabel[i] =
			new MSLabel(m_InfoPanel, "", TITLE_GENINFO_X, offset + i * MAINBUTTON_SIZE_Y, SKILLINFOPANEL_SIZE_X - TITLE_GENINFO_X, SKILLTYPEBUTTON_SIZE_Y);
		pLabel->setTextAlignment(Label::a_center);
		pLabel->setContentAlignment(Label::a_northwest);
		pLabel->SetFGColorRGB(Color_NormalText);
		pLabel->setVisible(false);
	}

	//Last Label is centered.  This labe is for when there is only one stat
	//m_InfoPanel->getSize(iw,ih);
	//pLabel = m_StatTypeLabel[STAT_MAGIC_TOTAL] =
	//	new MSLabel( m_InfoPanel, "", 0, 0, iw, ih + YRES(10), MSLabel::a_center );

	//	pLabel->setPos( ix + (iw2/2), iy + (ih2/2) );
	//pLabel->setTextAlignment( Label::a_center );
	//pLabel->setContentAlignment( Label::a_center );
	//pLabel->SetFGColorRGB( Color_TitleText );

	m_ActiveStat = -1;

	m_pScrollPanel->setScrollValue(0, 0);
	m_pScrollPanel->validate();
	enddbg;
}

//Shuriken read the Exp message from the server.
int __MsgFunc_Exp(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int iStat, iStatType;
	ulong Exp;
	iStat = READ_BYTE();
	iStatType = READ_BYTE();
	Exp = READ_LONG();
	TestExpArray[iStat][iStatType] = int(Exp);
	return 1;
}

// Update
void CStatPanel::Update()
{
	pMainPanel->setVisible(true);

	int i;
	char cDisplayText[128];
	for (i = 0; i < INFO_STAT_NUM; i++)
	{
		switch (i)
		{
		case 0:
			 _snprintf(cDisplayText, sizeof(cDisplayText),  "Name: %s\n",  player.DisplayName() );
			break;
		case 1:
			 _snprintf(cDisplayText, sizeof(cDisplayText),  "Gender: %s\n",  GenderList[player.m_Gender] );
			break;
		case 2:
			 _snprintf(cDisplayText, sizeof(cDisplayText),  "%s %s\n", "human",  player.GetTitle() );
			break;
		case 3:
			cDisplayText[0] = 0;
			break;
		case 4:
			 _snprintf(cDisplayText, sizeof(cDisplayText),  "Gold: %i\n",  player.m_Gold );
			break;
		case 5:
			 _snprintf(cDisplayText, sizeof(cDisplayText),  "Player Kills: %i\n",  player.m_PlayersKilled );
			break;
		default:
			cDisplayText[0] = 0;
			break;
		}

		TextPanel *pTextbox = Info_StatLabel[i];
		pTextbox->setText(cDisplayText);
	}
	for (i = 0; i < NATURAL_MAX_STATS; i++)
	{

		 _snprintf(cDisplayText, sizeof(cDisplayText),  "%s: %i \n",  NatStatList[i].Name,  player.GetNatStat(i) );

		TextPanel *pTextbox = Nat_StatLabel[i];
		pTextbox->setText(cDisplayText);

#if 0
		TextPanel *pModbox = Nat_StatModsLabel[i];
		int iBonus = player.GetNatStatBonus( i );
		pModbox->SetFGColorRGB( (iBonus < 0 ? COLOR(255,0,0,10) : COLOR( 0,240,0,0 )) );
		 _snprintf(cDisplayText, sizeof(cDisplayText),  (iBonus < 0 ? "(%i)" : "(+%i)"),  iBonus );
		pModbox->setText( cDisplayText );
#endif
	}

	bool FOUND_PARRY = false;
	for (int i = 0; i < SKILL_MAX_STATS; i++)
	{
		FOUND_PARRY = FOUND_PARRY || msstring(SkillStatList[i].Name) == "Parry";
		bool blank = i == SKILL_MAX_STATS - 1;
		int real_idx = FOUND_PARRY ? i + 1 : i;
		if (!blank)
			 _snprintf(cDisplayText, sizeof(cDisplayText),  "%s: %i\n",  SkillStatList[real_idx].Name,  player.GetSkillStat(SKILL_FIRSTSKILL + real_idx) );
		else
			 strncpy(cDisplayText,  "\n", sizeof(cDisplayText) );

		TextPanel *pTextbox = Skill_StatLabel[i];
		pTextbox->setText(cDisplayText);
		if (m_ActiveStat == real_idx)
			pTextbox->SetFGColorRGB(Color_SelectedText);
		else
			pTextbox->SetFGColorRGB(Color_NormalText);
	}

	 _snprintf(cDisplayText, sizeof(cDisplayText),  "Parry: %i\n",  player.GetSkillStat(SKILL_PARRY) );
	TextPanel *pTextbox = Skill_StatLabel[SKILL_MAX_STATS];
	pTextbox->setText(cDisplayText);
	pTextbox->SetFGColorRGB(Color_NormalText);

	//Update the Stat Info pane

	if (m_ActiveStat < 0 || msstring(SkillStatList[m_ActiveStat].Name) == "Parry")
	{
		m_InfoPanel->setVisible(false);
		return;
	}

	m_InfoPanel->setVisible(true);
	m_SkillInfoLabel->setText(SkillStatList[m_ActiveStat].Name);

	CStat *pStat = player.FindStat(SKILL_FIRSTSKILL + m_ActiveStat);
	if (pStat)
	{
		int Height = SKILLINFOPANEL_TITLE_Y + SKILLINFOPANEL_TITLE_H + SKILLINFOPANEL_BTM_BORDERSPACER_H;
		int UnusedSlots = 0;
		for (int i = 0; i < STAT_MAGIC_TOTAL; i++)
		{
			int iSubStats = pStat->m_SubStats.size();
			if (i >= iSubStats)
			{
				m_StatTypeLabel[i]->setVisible(false);
				continue;
			}

			CSubStat &SubStat = pStat->m_SubStats[i];
			long double ExpNeeded = GetExpNeeded(SubStat.Value);
			//Changed to a float so the players can keep better track of leveling.
			float Percent = (float(SubStat.Exp) / float(ExpNeeded)) * 100;
			//Shuriken must have changed the value by killing something, and no point unless their skill is high enough to be corrupted.
			if (SubStat.Value > 25)
				Percent = (TestExpArray[m_ActiveStat][i] / ExpNeeded) * 100;
			if (Percent > 100.0)
				Percent = 100.0;
			if (Percent < 0.0)
				Percent = 0.0;
			if (SubStat.Value == 0)
				Percent = 0.0;

			msstring_ref Name = "";
			if (iSubStats <= STATPROP_TOTAL)
				Name = SkillTypeList[i];
			else
				Name = SpellTypeList[i];

			//Thothie only display is slot is not marked unused
			double ExpToLevel = (SubStat.Value > 25 ? ExpNeeded - TestExpArray[m_ActiveStat][i] : ExpNeeded - SubStat.Exp);
			if (strcmp(Name, "unused") != 0)
			{
				if (EngineFunc::CVAR_GetFloat("ms_xpdisplay") == 0)
					 _snprintf(cDisplayText, sizeof(cDisplayText),  "%s: %i (%.2f%%%%) [%i left]\n",  Name,  (int)SubStat.Value,  Percent,  (int)ceil(ExpToLevel) );
				else if (EngineFunc::CVAR_GetFloat("ms_xpdisplay") == 1)
					 _snprintf(cDisplayText, sizeof(cDisplayText),  "%s: %i,  %i/%i (%.2f%%%%)\n",  Name,  (int)SubStat.Value,  (SubStat.Value > 25 ? TestExpArray[m_ActiveStat][i] : SubStat.Exp),  (int)ceil(ExpNeeded),  Percent );
				else if (EngineFunc::CVAR_GetFloat("ms_xpdisplay") == 2)
					 _snprintf(cDisplayText, sizeof(cDisplayText),  "%s: %i (%.2f%%%%) %i XP Left\n",  Name,  (int)SubStat.Value,  Percent,  (int)ceil(ExpToLevel) );
				else if (EngineFunc::CVAR_GetFloat("ms_xpdisplay") == 3)
				{
					if ((int)SubStat.Value < 5)
						 _snprintf(cDisplayText, sizeof(cDisplayText),  "%s: %i (%.1f%%%% trained)\n",  Name,  (int)SubStat.Value,  Percent );
					if ((int)SubStat.Value >= 5 && (int)SubStat.Value < 10)
						 _snprintf(cDisplayText, sizeof(cDisplayText),  "%s: %i (%.2f%%%% trained)\n",  Name,  (int)SubStat.Value,  Percent );
					if ((int)SubStat.Value >= 10 && (int)SubStat.Value < 15)
						 _snprintf(cDisplayText, sizeof(cDisplayText),  "%s: %i (%.3f%%%% trained)\n",  Name,  (int)SubStat.Value,  Percent );
					if ((int)SubStat.Value >= 15 && (int)SubStat.Value < 20)
						 _snprintf(cDisplayText, sizeof(cDisplayText),  "%s: %i (%.4f%%%% trained)\n",  Name,  (int)SubStat.Value,  Percent );
					if ((int)SubStat.Value > 20)
						 _snprintf(cDisplayText, sizeof(cDisplayText),  "%s: %i (%.5f%%%% trained)\n",  Name,  (int)SubStat.Value,  Percent );
				}
				else if (EngineFunc::CVAR_GetFloat("ms_xpdisplay") >= 4)
					 _snprintf(cDisplayText, sizeof(cDisplayText),  "%s: %i (%.0f%%%% trained)\n",  Name,  (int)SubStat.Value,  Percent );

				Label *pLabel = m_StatTypeLabel[i - UnusedSlots];
				pLabel->setVisible(true);
				pLabel->setText(cDisplayText);
				Height += MAINBUTTON_SIZE_Y;
			}
			else
			{
				++UnusedSlots;
				 strncpy(cDisplayText,  "", sizeof(cDisplayText) );
			}
		}

		m_InfoPanel->setSize(SKILLINFOPANEL_SIZE_X, Height);
	}

	m_pScrollPanel->validate();
}

//======================================
// Key inputs for the Class Menu
bool CStatPanel::SlotInput(int iSlot)
{
	return false;
}

//======================================
// Update the Class menu before opening it
void CStatPanel::Open(void)
{
	Update();
	CMenuPanel::Open();
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void CStatPanel::Initialize(void)
{
	//setVisible( false );
	m_pScrollPanel->setScrollValue(0, 0);
}

void CStatPanel::StepInput(hudscroll_e ScrollCmd)
{
	if (!IsActive() || (ScrollCmd != HUDSCROLL_DOWN && ScrollCmd != HUDSCROLL_UP))
		return;

	if (ScrollCmd == HUDSCROLL_DOWN)
	{
		m_ActiveStat++;
		if (m_ActiveStat + SKILL_FIRSTSKILL == SKILL_PARRY)
			++m_ActiveStat;
		if (m_ActiveStat >= SKILL_MAX_STATS)
			m_ActiveStat = -1;
	}
	else
	{
		m_ActiveStat--;
		if (m_ActiveStat + SKILL_FIRSTSKILL == SKILL_PARRY)
			--m_ActiveStat;
		if (m_ActiveStat == -2)
			m_ActiveStat = SKILL_MAX_STATS - 1;
	}

	Update();
}
void CStatPanel::Close(void)
{
	CMenuPanel::Close();
}

//Bound to 'playerinfo'
void __CmdFunc_PlayerInfo(void)
{
	VGUI::ToggleMenuVisible("stats");
}
void UpdateVGUIStats()
{
	if (gViewPort->m_pStatMenu)
		gViewPort->m_pStatMenu->Update();
}

int __MsgFunc_SetStat(const char *pszName, int iSize, void *pbuf)
{
	//MiB JAN2010_15 Gold change on spawn.rtf
	BEGIN_READ(pbuf, iSize);
	bool Verbose;
	int iStatType = READ_BYTE(), iStat, value;

	if (iStatType < 3)
		iStat = READ_BYTE();

	if (iStatType == 0)
		value = READ_BYTE();
	else if (iStatType == 3)
	{
		Verbose = READ_BYTE() == 1;
		value = READ_LONG();
	}

	//Fill out the player info
	if (iStatType == 1)
	{
		if (iStat < (signed)player.m_Stats.size())
		{
			CStat &Stat = player.m_Stats[iStat];
			for (int i = 0; i < Stat.m_SubStats.size(); i++)
			{
				Stat.m_SubStats[i].Value = READ_BYTE();
				if (Stat.m_Type == CStat::STAT_SKILL)
					Stat.m_SubStats[i].Exp = READ_SHORT();
			}
		}
	}
	else if (iStatType == 3)
	{
		static char cMessage[64];
		static client_textmessage_t msg;
		int oldGold = player.m_Gold;
		player.m_Gold = value;
		bool fGoldGained = false;

		if (player.m_Gold > oldGold)
		{
			fGoldGained = true;
			msg.r1 = 0;
			msg.g1 = 255;
			msg.b1 = 0;
			msg.r2 = msg.g2 = msg.b2 = 0;
			 _snprintf(cMessage, sizeof(cMessage),  "+ %i Gold\n",  player.m_Gold - oldGold );
		}
		else if (player.m_Gold < oldGold)
		{
			msg.r1 = 128;
			msg.g1 = 0;
			msg.b1 = 0;
			msg.r2 = msg.g2 = msg.b2 = 0;
			 _snprintf(cMessage, sizeof(cMessage),  "- %i Gold",  oldGold - player.m_Gold );
		}

		msg.effect = 1;
		//msg.x = 0.18; msg.y = 0.90;
		//msg.x = 0.22; msg.y = 0.90;
		msg.x = 0.24;
		msg.y = 0.90;
		msg.fadein = 0.8;
		msg.holdtime = 1.5;
		msg.fadeout = 0.8;
		msg.pName = NULL;
		msg.pMessage = cMessage;

		if (player.IsAlive() && Verbose)
		{
			gHUD.m_Message.MessageAdd(msg);
			if (fGoldGained)
				player.PlaySound(CHAN_BODY, "misc/gold.wav", 1.0, true);
		}
		UpdateActiveMenus();
	}
	else if (iStatType == 10) //Gender msg
	{
		player.m_Gender = READ_BYTE();
	}

	UpdateVGUIStats();
	return 1;
}
