//=========== (C) Copyright 1996-2002 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: VGUI scoreboard
//
// $Workfile:     $
// $Date: 2005/01/17 13:16:49 $
//
//-----------------------------------------------------------------------------
// $Log: vgui_ScorePanel.cpp,v $
// Revision 1.7  2005/01/17 13:16:49  dogg
// Brand new inventory VGUI, revised item system, Magic, Mp3 support and item storage
//
//
// $NoKeywords: $
//=============================================================================

#include <VGUI_LineBorder.h>

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "vgui_teamfortressviewport.h"
#include "vgui_scorepanel.h"
#include "vgui_helpers.h"
#include "vgui_loadtga.h"
#include "vgui_spectatorpanel.h"

//Master Sword
#include "parsemsg.h"
#include "msdllheaders.h"
#include "player/player.h"
#include "stats/statdefs.h"
#include "global.h"

hud_player_info_t g_PlayerInfoList[MAX_PLAYERS + 1];	// player info from the engine
extra_player_info_t g_PlayerExtraInfo[MAX_PLAYERS + 1]; // additional player info sent directly to the client dll
team_info_t g_TeamInfo[MAX_TEAMS + 1];
int g_IsSpectator[MAX_PLAYERS + 1];

int HUD_IsGame(const char *game);
int EV_TFC_IsAllyTeam(int iTeam1, int iTeam2);

// Scoreboard dimensions
#define SBOARD_TITLE_SIZE_Y YRES(22)

#define X_BORDER XRES(4)

// Column sizes
class SBColumnInfo
{
public:
	char *m_pTitle; // If null, ignore, if starts with #, it's localized, otherwise use the string directly.
	int m_Width;	// Based on 640 width. Scaled to fit other resolutions.
	Label::Alignment m_Alignment;
};

// grid size is marked out for 640x480 screen

SBColumnInfo g_ColumnInfo[NUM_COLUMNS] =
	{
		{NULL, 24, Label::a_center}, //Icon
		{NULL, 90, Label::a_west},	 //Name
		//{"#SKILLLEVEL",	50,			Label::a_center},		//Skill
		{"#TITLE", 100, Label::a_center},  //Title
		{"#PARTY", 80, Label::a_center},   //Party
		{"#HEALTH", 50, Label::a_center},  //FEB2008a -- Shuriken, Health Cur/Max
		{"#LATENCY", 30, Label::a_center}, //Ping
		{NULL, 40, Label::a_center},	   //Voice Icon
		{NULL, 2, Label::a_east},		   // blank column to take up the slack
};

#define TEAM_NO 0
#define TEAM_YES 1
#define TEAM_SPECTATORS 2
#define TEAM_BLANK 3

//-----------------------------------------------------------------------------
// ScorePanel::HitTestPanel.
//-----------------------------------------------------------------------------

void ScorePanel::HitTestPanel::internalMousePressed(MouseCode code)
{
	for (int i = 0; i < _inputSignalDar.getCount(); i++)
	{
		_inputSignalDar[i]->mousePressed(code, this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create the ScoreBoard panel
//-----------------------------------------------------------------------------
ScorePanel::ScorePanel(int x, int y, int wide, int tall) : Panel(x, y, wide, tall)
{
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle("Scoreboard Title Text");
	SchemeHandle_t hSmallScheme = pSchemes->getSchemeHandle("Scoreboard Small Text");
	Font *tfont = pSchemes->getFont(hTitleScheme);
	Font *smallfont = pSchemes->getFont(hSmallScheme);

	setBgColor(0, 0, 0, 96);
	m_pCurrentHighlightLabel = NULL;
	m_iHighlightRow = -1;

	// Initialize the top title.
	m_TitleLabel.setFont(tfont);
	m_TitleLabel.setText(Localized("#SCOREBOARD_TITLE"));
	m_TitleLabel.setBgColor(0, 0, 0, 255);
	m_TitleLabel.setFgColor(Scheme::sc_primary1);
	m_TitleLabel.setContentAlignment(vgui::Label::a_west);

	LineBorder *border = new LineBorder(Color(60, 60, 60, 128));
	setBorder(border);
	setPaintBorderEnabled(true);

	int xpos = g_ColumnInfo[0].m_Width + 3;
	if (ScreenWidth >= 640)
	{
		// only expand column size for res greater than 640
		xpos = XRES(xpos);
	}
	m_TitleLabel.setBounds(xpos, 4, wide, SBOARD_TITLE_SIZE_Y);
	m_TitleLabel.setContentFitted(false);
	m_TitleLabel.setParent(this);

	// Setup the header (labels like "name", "class", etc..).
	m_HeaderGrid.SetDimensions(NUM_COLUMNS, 1);
	m_HeaderGrid.SetSpacing(0, 0);

	for (int i = 0; i < NUM_COLUMNS; i++)
	{
		if (g_ColumnInfo[i].m_pTitle && g_ColumnInfo[i].m_pTitle[0] == '#')
			m_HeaderLabels[i].setText(CHudTextMessage::BufferedLocaliseTextString(g_ColumnInfo[i].m_pTitle));
		else if (g_ColumnInfo[i].m_pTitle)
			m_HeaderLabels[i].setText(g_ColumnInfo[i].m_pTitle);

		int xwide = g_ColumnInfo[i].m_Width;
		if (ScreenWidth >= 640)
		{
			xwide = XRES(xwide);
		}
		else if (ScreenWidth == 400)
		{
			// hack to make 400x300 resolution scoreboard fit
			if (i == 1)
			{
				// reduces size of player name cell
				xwide -= 28;
			}
			else if (i == 0)
			{
				xwide -= 8;
			}
		}

		m_HeaderGrid.SetColumnWidth(i, xwide);
		m_HeaderGrid.SetEntry(i, 0, &m_HeaderLabels[i]);

		m_HeaderLabels[i].setBgColor(0, 0, 0, 255);
		m_HeaderLabels[i].setFgColor(Scheme::sc_primary1);
		m_HeaderLabels[i].setFont(smallfont);
		m_HeaderLabels[i].setContentAlignment(g_ColumnInfo[i].m_Alignment);

		int yres = 12;
		if (ScreenHeight >= 480)
		{
			yres = YRES(yres);
		}
		m_HeaderLabels[i].setSize(50, yres);
	}

	// Set the width of the last column to be the remaining space.
	int ex, ey, ew, eh;
	m_HeaderGrid.GetEntryBox(NUM_COLUMNS - 2, 0, ex, ey, ew, eh);
	m_HeaderGrid.SetColumnWidth(NUM_COLUMNS - 1, (wide - X_BORDER) - (ex + ew));

	m_HeaderGrid.AutoSetRowHeights();
	m_HeaderGrid.setBounds(X_BORDER, SBOARD_TITLE_SIZE_Y, wide - X_BORDER * 2, m_HeaderGrid.GetRowHeight(0));
	m_HeaderGrid.setParent(this);
	m_HeaderGrid.setBgColor(0, 0, 0, 255);

	// Now setup the listbox with the actual player data in it.
	int headerX, headerY, headerWidth, headerHeight;
	m_HeaderGrid.getBounds(headerX, headerY, headerWidth, headerHeight);
	m_PlayerList.setBounds(headerX, headerY + headerHeight, headerWidth, tall - headerY - headerHeight - 6);
	m_PlayerList.setBgColor(0, 0, 0, 255);
	m_PlayerList.setParent(this);

	for (int row = 0; row < NUM_ROWS; row++)
	{
		CGrid *pGridRow = &m_PlayerGrids[row];

		pGridRow->SetDimensions(NUM_COLUMNS, 1);

		for (int col = 0; col < NUM_COLUMNS; col++)
		{
			m_PlayerEntries[col][row].setContentFitted(false);
			m_PlayerEntries[col][row].setRow(row);
			m_PlayerEntries[col][row].addInputSignal(this);
			pGridRow->SetEntry(col, 0, &m_PlayerEntries[col][row]);
		}

		pGridRow->setBgColor(0, 0, 0, 255);
		//		pGridRow->SetSpacing(2, 0);
		pGridRow->SetSpacing(0, 0);
		pGridRow->CopyColumnWidths(&m_HeaderGrid);
		pGridRow->AutoSetRowHeights();
		pGridRow->setSize(PanelWidth(pGridRow), pGridRow->CalcDrawHeight());
		pGridRow->RepositionContents();

		m_PlayerList.AddItem(pGridRow);
	}

	// Add the hit test panel. It is invisible and traps mouse clicks so we can go into squelch mode.
	m_HitTestPanel.setBgColor(0, 0, 0, 255);
	m_HitTestPanel.setParent(this);
	m_HitTestPanel.setBounds(0, 0, wide, tall);
	m_HitTestPanel.addInputSignal(this);

	m_pCloseButton = new CommandButton("x", wide - XRES(12 + 4), YRES(2), XRES(12), YRES(12));
	m_pCloseButton->setParent(this);
	m_pCloseButton->addActionSignal(new CMenuHandler_StringCommandWatch("-showscores", true));
	m_pCloseButton->setBgColor(0, 0, 0, 255);
	m_pCloseButton->setFgColor(255, 255, 255, 0);
	m_pCloseButton->setFont(tfont);
	m_pCloseButton->setBoundKey((char)255);
	m_pCloseButton->setContentAlignment(Label::a_center);

	Initialize();
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void ScorePanel::Initialize(void)
{
	// Clear out scoreboard data
	m_iLastKilledBy = 0;
	m_fLastKillTime = 0;
	m_iPlayerNum = 0;
	m_iNumTeams = 0;
	memset(g_PlayerExtraInfo, 0, sizeof g_PlayerExtraInfo);
	memset(g_TeamInfo, 0, sizeof g_TeamInfo);

	//m_pIcons[0] = new CImageDelayed( "hud_trans", false, false, 0, 0, 16, 16 );
	m_pIcons[0] = vgui_LoadTGA("gfx/vgui/640_hud_trans.tga", false);
}

bool HACK_GetPlayerUniqueID(int iPlayer, char playerID[16])
{
	return !!gEngfuncs.GetPlayerUniqueID(iPlayer, playerID);
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate the internal scoreboard data
//-----------------------------------------------------------------------------
void ScorePanel::Update()
{
	// Set the title
	if (MSGlobals::ServerName.len())
		m_TitleLabel.setText(MSGlobals::ServerName + " (" + MSGlobals::MapName + ")");
	else
		m_TitleLabel.setText(Localized("#SCOREBOARD_TITLE"));

	m_iRows = 0;
	gViewPort->GetAllPlayersInfo();

	// Clear out sorts
	std::fill(std::begin(m_iSortedRows), std::end(m_iSortedRows), 0);
	std::fill(std::begin(m_iIsATeam), std::end(m_iIsATeam), TEAM_NO);
	std::fill(std::begin(m_bHasBeenSorted), std::end(m_bHasBeenSorted), false);

	// If it's not teamplay, sort all the players. Otherwise, sort the teams.
	if (!gHUD.m_Teamplay)
		SortPlayers(0, NULL);
	else
		SortTeams();

	// set scrollbar range
	m_PlayerList.SetScrollRange(m_iRows);

	FillGrid();

	if (gViewPort->m_pSpectatorPanel->m_menuVisible)
	{
		m_pCloseButton->setVisible(true);
	}
	else
	{
		m_pCloseButton->setVisible(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sort all the teams
//-----------------------------------------------------------------------------
void ScorePanel::SortTeams()
{
	int i = 0;

	// clear out team scores
	for (i = 1; i <= m_iNumTeams; i++)
	{
		if (!g_TeamInfo[i].scores_overriden)
			g_TeamInfo[i].frags = g_TeamInfo[i].deaths = 0;
		g_TeamInfo[i].ping = g_TeamInfo[i].packetloss = 0;
	}

	// recalc the team scores, then draw them
	for (i = 1; i < MAX_PLAYERS; i++)
	{
		if (g_PlayerInfoList[i].name == NULL)
			continue; // empty player slot, skip

		if (g_PlayerExtraInfo[i].teamname[0] == 0)
			continue; // skip over players who are not in a team

		// find what team this player is in
		int j = 0;
		for (j = 1; j <= m_iNumTeams; j++)
		{
			if (!_stricmp(g_PlayerExtraInfo[i].teamname, g_TeamInfo[j].name))
				break;
		}
		if (j > m_iNumTeams) // player is not in a team, skip to the next guy
			continue;

		if (!g_TeamInfo[j].scores_overriden)
		{
			//g_TeamInfo[j].frags += g_PlayerExtraInfo[i].frags;
			//g_TeamInfo[j].deaths += g_PlayerExtraInfo[i].deaths;
		}

		g_TeamInfo[j].ping += g_PlayerInfoList[i].ping;
		g_TeamInfo[j].packetloss += g_PlayerInfoList[i].packetloss;

		if (g_PlayerInfoList[i].thisplayer)
			g_TeamInfo[j].ownteam = TRUE;
		else
			g_TeamInfo[j].ownteam = FALSE;

		// Set the team's number (used for team colors)
		g_TeamInfo[j].teamnumber = g_PlayerExtraInfo[i].teamnumber;
	}

	// find team ping/packetloss averages
	for (i = 1; i <= m_iNumTeams; i++)
	{
		g_TeamInfo[i].already_drawn = FALSE;

		if (g_TeamInfo[i].players > 0)
		{
			g_TeamInfo[i].ping /= g_TeamInfo[i].players;	   // use the average ping of all the players in the team as the teams ping
			g_TeamInfo[i].packetloss /= g_TeamInfo[i].players; // use the average ping of all the players in the team as the teams ping
		}
	}

	// Draw the teams
	while (1)
	{
		int highest_frags = -99999;
		int lowest_deaths = 99999;
		int best_team = 0;

		for (i = 1; i <= m_iNumTeams; i++)
		{
			if (g_TeamInfo[i].players < 1)
				continue;

			if (!g_TeamInfo[i].already_drawn && g_TeamInfo[i].frags >= highest_frags)
			{
				if (g_TeamInfo[i].frags > highest_frags || g_TeamInfo[i].deaths < lowest_deaths)
				{
					best_team = i;
					lowest_deaths = g_TeamInfo[i].deaths;
					highest_frags = g_TeamInfo[i].frags;
				}
			}
		}

		// draw the best team on the scoreboard
		if (!best_team)
			break;

		// Put this team in the sorted list
		m_iSortedRows[m_iRows] = best_team;
		m_iIsATeam[m_iRows] = TEAM_YES;
		g_TeamInfo[best_team].already_drawn = TRUE; // set the already_drawn to be TRUE, so this team won't get sorted again
		m_iRows++;

		// Now sort all the players on this team
		SortPlayers(0, g_TeamInfo[best_team].name);
	}

	// Add all the players who aren't in a team yet into spectators
	SortPlayers(TEAM_SPECTATORS, NULL);
}

//-----------------------------------------------------------------------------
// Purpose: Sort a list of players
//-----------------------------------------------------------------------------

void ScorePanel::SortPlayers(int iTeam, char *team)
{
	bool bCreatedTeam = false;

	// Sort the players
	for (int n = 0; n < MAX_PLAYERS; n++)
	{
		// Find the top ranking player
		//Bubble Sort
		int best_player = 0;

		for (int i = 1; i < MAX_PLAYERS; i++)
		{
			extra_player_info_t &ExtraInfo = g_PlayerExtraInfo[i];
			if (m_bHasBeenSorted[i] || !g_PlayerInfoList[i].name ||
				(best_player && _stricmp(g_PlayerExtraInfo[best_player].Title, ExtraInfo.Title) < 0))
				continue;

			cl_entity_t *ent = gEngfuncs.GetEntityByIndex(i);
			if (!ent)
				continue;

			best_player = i;
		}

		if (!best_player)
			break;

		// If we haven't created the Team yet, do it first
		if (!bCreatedTeam && iTeam)
		{
			m_iIsATeam[m_iRows] = iTeam;
			m_iRows++;

			bCreatedTeam = true;
		}

		// Put this player in the sorted list
		m_iSortedRows[m_iRows] = best_player;
		m_bHasBeenSorted[best_player] = true;
		m_iRows++;
	}

	if (team)
	{
		m_iIsATeam[m_iRows++] = TEAM_BLANK;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate the existing teams in the match
//-----------------------------------------------------------------------------
void ScorePanel::RebuildTeams()
{
	int i = 0;

	// clear out player counts from teams
	for (i = 1; i <= m_iNumTeams; i++)
	{
		g_TeamInfo[i].players = 0;
	}

	// rebuild the team list
	gViewPort->GetAllPlayersInfo();
	m_iNumTeams = 0;
	for (i = 1; i < MAX_PLAYERS; i++)
	{
		if (g_PlayerInfoList[i].name == NULL)
			continue;

		if (g_PlayerExtraInfo[i].teamname[0] == 0)
			continue; // skip over players who are not in a team

		int j = 0;
		// is this player in an existing team?
		for (j = 1; j <= m_iNumTeams; j++)
		{
			if (g_TeamInfo[j].name[0] == '\0')
				break;

			if (!_stricmp(g_PlayerExtraInfo[i].teamname, g_TeamInfo[j].name))
				break;
		}

		if (j > m_iNumTeams)
		{ // they aren't in a listed team, so make a new one
			// search through for an empty team slot
			for (j = 1; j <= m_iNumTeams; j++)
			{
				if (g_TeamInfo[j].name[0] == '\0')
					break;
			}
			m_iNumTeams = V_max(j, m_iNumTeams);

			strncpy(g_TeamInfo[j].name, g_PlayerExtraInfo[i].teamname, MAX_TEAM_NAME);
			g_TeamInfo[j].players = 0;
		}

		g_TeamInfo[j].players++;
	}

	// clear out any empty teams
	for (i = 1; i <= m_iNumTeams; i++)
	{
		if (g_TeamInfo[i].players < 1)
			memset(&g_TeamInfo[i], 0, sizeof(team_info_t));
	}

	// Update the scoreboard
	Update();
}

void ScorePanel::FillGrid()
{
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hScheme = pSchemes->getSchemeHandle("Scoreboard Text");
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle("Scoreboard Title Text");
	SchemeHandle_t hSmallScheme = pSchemes->getSchemeHandle("Scoreboard Small Text");

	Font *sfont = pSchemes->getFont(hScheme);
	Font *tfont = pSchemes->getFont(hTitleScheme);
	Font *smallfont = pSchemes->getFont(hSmallScheme);

	// update highlight position
	int x, y;
	getApp()->getCursorPos(x, y);
	cursorMoved(x, y, this);

	// remove highlight row if we're not in squelch mode
	if (!GetClientVoiceMgr()->IsInSquelchMode())
	{
		m_iHighlightRow = -1;
	}

	bool bNextRowIsGap = false;

	for (int row = 0; row < NUM_ROWS; row++)
	{
		CGrid *pGridRow = &m_PlayerGrids[row];
		pGridRow->SetRowUnderline(0, false, 0, 0, 0, 0, 0);

		if (row >= m_iRows)
		{
			for (int col = 0; col < NUM_COLUMNS; col++)
				m_PlayerEntries[col][row].setVisible(false);

			continue;
		}

		bool bRowIsGap = false;
		if (bNextRowIsGap)
		{
			bNextRowIsGap = false;
			bRowIsGap = true;
		}

		for (int col = 0; col < NUM_COLUMNS; col++)
		{
			CLabelHeader *pLabel = &m_PlayerEntries[col][row];

			pLabel->setVisible(true);
			pLabel->setText2("");
			pLabel->setImage(NULL);
			pLabel->setFont(sfont);
			pLabel->setTextOffset(0, 0);

			int rowheight = 13;
			if (ScreenHeight > 480)
			{
				rowheight = YRES(rowheight);
			}
			else
			{
				// more tweaking, make sure icons fit at low res
				rowheight = 15;
			}
			pLabel->setSize(pLabel->getWide(), rowheight);
			pLabel->setBgColor(0, 0, 0, 255);

			char sz[128];
			hud_player_info_t *pl_info = NULL;
			team_info_t *team_info = NULL;

			if (m_iIsATeam[row] == TEAM_BLANK)
			{
				pLabel->setText(" ");
				continue;
			}
			else if (m_iIsATeam[row] == TEAM_YES)
			{

				// Get the team's data
				team_info = &g_TeamInfo[m_iSortedRows[row]];

				// team color text for team names
				/*pLabel->setFgColor(	iTeamColors[team_info->teamnumber % iNumberOfTeamColors][0],
									iTeamColors[team_info->teamnumber % iNumberOfTeamColors][1],
									iTeamColors[team_info->teamnumber % iNumberOfTeamColors][2],
									0 );*/

				// different height for team header rows
				rowheight = 20;
				if (ScreenHeight >= 480)
				{
					rowheight = YRES(rowheight);
				}
				pLabel->setSize(pLabel->getWide(), rowheight);
				pLabel->setFont(tfont);

				/*pGridRow->SetRowUnderline(	0,
											true,
											YRES(3),
											iTeamColors[team_info->teamnumber % iNumberOfTeamColors][0],
											iTeamColors[team_info->teamnumber % iNumberOfTeamColors][1],
											iTeamColors[team_info->teamnumber % iNumberOfTeamColors][2],
											0 );*/
			}
			else if (m_iIsATeam[row] == TEAM_SPECTATORS)
			{
				// grey text for spectators
				pLabel->setFgColor(100, 100, 100, 0);

				// different height for team header rows
				rowheight = 20;
				if (ScreenHeight >= 480)
				{
					rowheight = YRES(rowheight);
				}
				pLabel->setSize(pLabel->getWide(), rowheight);
				pLabel->setFont(tfont);

				pGridRow->SetRowUnderline(0, true, YRES(3), 100, 100, 100, 0);
			}
			else
			{
				pLabel->setFgColor(100, 100, 100, 0);
				// team color text for player names
				/*pLabel->setFgColor(	iTeamColors[ g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber % iNumberOfTeamColors ][0],
									iTeamColors[ g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber % iNumberOfTeamColors ][1],
									iTeamColors[ g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber % iNumberOfTeamColors ][2],
									0 );*/

				// Get the player's data
				pl_info = &g_PlayerInfoList[m_iSortedRows[row]];

				// Set background color
				if (pl_info->thisplayer) // if it is their name, draw it a different color
				{
					// Highlight this player
					pLabel->setFgColor(Scheme::sc_white);
					/*pLabel->setBgColor(	iTeamColors[ g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber % iNumberOfTeamColors ][0],
										iTeamColors[ g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber % iNumberOfTeamColors ][1],
										iTeamColors[ g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber % iNumberOfTeamColors ][2],
										196 );*/
				}
				else if (m_iSortedRows[row] == m_iLastKilledBy && m_fLastKillTime && m_fLastKillTime > gHUD.m_flTime)
				{
					// Killer's name
					pLabel->setBgColor(255, 0, 0, 255 - ((float)15 * (float)(m_fLastKillTime - gHUD.m_flTime)));
				}
			}

			// Align
			pLabel->setContentAlignment(g_ColumnInfo[col].m_Alignment);
			/*if (col == COLUMN_NAME || col == COLUMN_CLASS)
			{
				pLabel->setContentAlignment( vgui::Label::a_west );
			}
			else if (col == COLUMN_TRACKER)
			{
				pLabel->setContentAlignment( vgui::Label::a_center );
			}
			else
			{
				pLabel->setContentAlignment( vgui::Label::a_east );
			}*/

			// Fill out with the correct data
			 strncpy(sz,  "", sizeof(sz) );
			if (m_iIsATeam[row])
			{
				/*char sz2[128];

				switch (col)
				{
				case COLUMN_NAME:
					if ( m_iIsATeam[row] == TEAM_SPECTATORS )
					{
						 strncpy(sz2,  CHudTextMessage::BufferedLocaliseTextString( "#Spectators" ), sizeof(sz2) );
					}
					else
					{
						 strncpy(sz2,  gViewPort->GetTeamName(team_info->teamnumber), sizeof(sz2) );
					}

					 strncpy(sz,  sz2, sizeof(sz) );

					// Append the number of players
					if ( m_iIsATeam[row] == TEAM_YES )
					{
						if (team_info->players == 1)
						{
							 _snprintf(sz2, sizeof(sz2),  "(%d %s)",  team_info->players,  CHudTextMessage::BufferedLocaliseTextString( "#Player" ) );
						}
						else
						{
							 _snprintf(sz2, sizeof(sz2),  "(%d %s)",  team_info->players,  CHudTextMessage::BufferedLocaliseTextString( "#Player_plural" ) );
						}

						pLabel->setText2(sz2);
						pLabel->setFont2(smallfont);
					}
					break;
				default:
					break;
				}*/
			}
			else
			{
				hud_player_info_t &Info = *pl_info;
				extra_player_info_t &ExtraInfo = g_PlayerExtraInfo[m_iSortedRows[row]];

				if (MSGlobals::CurrentVote.fActive && PlayerVotedYes(m_iSortedRows[row]))
				{
					//Player voted yes to a current vote.  Change the background color to green
					pLabel->setBgColor(0, 90, 0, 255 - 90);
				}

				//Has the player picked a character yet?
				bool fCharLoaded = FBitSet(ExtraInfo.Flags, (1 << 2)) ? true : false;

				switch (col)
				{
				case COLUMN_ICON:
					if (FBitSet(ExtraInfo.Flags, (1 << 1)))
					{
						pLabel->setImage(m_pIcons[0]); //Standing in transition
						continue;
					}
					else
						break;
				case COLUMN_NAME:
					//UNDONE -- Don't use the displayname, because the actual name on the server
					//might have been changed due to duplicate names
					_snprintf(sz, sizeof(sz), "%s", Info.name);
					break;
				/*case COLUMN_SKILL:
					{
						if( fLoading ) strcpy( sz, "Loading..." );
						else
							//strcpy( sz, "" );
							 _snprintf(sz, sizeof(sz),  "%d",  ExtraInfo.SkillAve );
					}
					break;*/
				case COLUMN_TITLE:
					if (fCharLoaded)
						 strncpy(sz,  ExtraInfo.Title, sizeof(sz) );
					else
						 strncpy(sz,  "Loading...", sizeof(sz) );
					break;
				case COLUMN_PARTY:
					if (fCharLoaded && ExtraInfo.teamname[0])
						_snprintf(sz, 128, "%s", &ExtraInfo.teamname);
					else
						 strncpy(sz,  "-", sizeof(sz) );
					break;
				case COLUMN_HEALTH: //FEB2008 -- Shuriken
					if (fCharLoaded)
					{
						if (ExtraInfo.HP > 0)
						{
							 _snprintf(sz, sizeof(sz),  "%i/%i",  ExtraInfo.HP,  ExtraInfo.MaxHP );
						}
						else
							 strncpy(sz,  "Dead", sizeof(sz) );
					}
					else
						 strncpy(sz,  "-", sizeof(sz) );
					break;
				case COLUMN_PING:
					 _snprintf(sz, sizeof(sz),  "%d",  g_PlayerInfoList[m_iSortedRows[row]].ping );
					break;
				case COLUMN_VOICE:
					sz[0] = 0;
					// in HLTV mode allow spectator to turn on/off commentator voice
					if (!pl_info->thisplayer || gEngfuncs.IsSpectateOnly())
					{
						GetClientVoiceMgr()->UpdateSpeakerImage(pLabel, m_iSortedRows[row]);
					}
					break;
				default:
					break;
				}
			}

			pLabel->setText(sz);
		}
	}

	for (int row = 0; row < NUM_ROWS; row++)
	{
		CGrid *pGridRow = &m_PlayerGrids[row];

		pGridRow->AutoSetRowHeights();
		pGridRow->setSize(PanelWidth(pGridRow), pGridRow->CalcDrawHeight());
		pGridRow->RepositionContents();
	}

	// hack, for the thing to resize
	m_PlayerList.getSize(x, y);
	m_PlayerList.setSize(x, y);
}

//-----------------------------------------------------------------------------
// Purpose: Setup highlights for player names in scoreboard
//-----------------------------------------------------------------------------
void ScorePanel::DeathMsg(int killer, int victim)
{
	// if we were the one killed,  or the world killed us, set the scoreboard to indicate suicide
	if (victim == m_iPlayerNum || killer == 0)
	{
		m_iLastKilledBy = killer ? killer : m_iPlayerNum;
		m_fLastKillTime = gHUD.m_flTime + 10; // display who we were killed by for 10 seconds

		if (killer == m_iPlayerNum)
			m_iLastKilledBy = m_iPlayerNum;
	}
}

void ScorePanel::Open(void)
{
	RebuildTeams();
	setVisible(true);
	m_HitTestPanel.setVisible(true);
}

void ScorePanel::mousePressed(MouseCode code, Panel *panel)
{
	if (gHUD.m_iIntermission)
		return;

	if (!GetClientVoiceMgr()->IsInSquelchMode())
	{
		GetClientVoiceMgr()->StartSquelchMode();
		m_HitTestPanel.setVisible(false);
	}
	else if (m_iHighlightRow >= 0)
	{
		// mouse has been pressed, toggle mute state
		int iPlayer = m_iSortedRows[m_iHighlightRow];
		if (iPlayer > 0)
		{
			// print text message
			hud_player_info_t *pl_info = &g_PlayerInfoList[iPlayer];

			if (pl_info && pl_info->name && pl_info->name[0])
			{
				char string[256];
				if (GetClientVoiceMgr()->IsPlayerBlocked(iPlayer))
				{
					char string1[1024];

					// remove mute
					GetClientVoiceMgr()->SetPlayerBlockedState(iPlayer, false);

					 _snprintf(string1, sizeof(string1),  CHudTextMessage::BufferedLocaliseTextString("#Unmuted"),  pl_info->name );
					 _snprintf(string, sizeof(string),  "%c** %s\n",  HUD_PRINTTALK,  string1 );

					gHUD.m_TextMessage.MsgFunc_TextMsg(NULL, strlen(string) + 1, string);
				}
				else
				{
					char string1[1024];
					char string2[1024];

					// mute the player
					GetClientVoiceMgr()->SetPlayerBlockedState(iPlayer, true);

					 _snprintf(string1, sizeof(string1),  CHudTextMessage::BufferedLocaliseTextString("#Muted"),  pl_info->name );
					 strncpy(string2,  CHudTextMessage::BufferedLocaliseTextString("#No_longer_hear_that_player"), sizeof(string2) );
					 _snprintf(string, sizeof(string),  "%c** %s %s\n",  HUD_PRINTTALK,  string1,  string2 );

					gHUD.m_TextMessage.MsgFunc_TextMsg(NULL, strlen(string) + 1, string);
				}
			}
		}
	}
}

void ScorePanel::cursorMoved(int x, int y, Panel *panel)
{
	if (GetClientVoiceMgr()->IsInSquelchMode())
	{
		// look for which cell the mouse is currently over
		for (int i = 0; i < NUM_ROWS; i++)
		{
			int row, col;
			if (m_PlayerGrids[i].getCellAtPoint(x, y, row, col))
			{
				MouseOverCell(i, col);
				return;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles mouse movement over a cell
// Input  : row -
//			col -
//-----------------------------------------------------------------------------
void ScorePanel::MouseOverCell(int row, int col)
{
	CLabelHeader *label = &m_PlayerEntries[col][row];

	// clear the previously highlighted label
	if (m_pCurrentHighlightLabel != label)
	{
		m_pCurrentHighlightLabel = NULL;
		m_iHighlightRow = -1;
	}
	if (!label)
		return;

	// don't act on teams
	if (m_iIsATeam[row] != TEAM_NO)
		return;

	// don't act on disconnected players or ourselves
	hud_player_info_t *pl_info = &g_PlayerInfoList[m_iSortedRows[row]];
	if (!pl_info->name || !pl_info->name[0])
		return;

	if (pl_info->thisplayer && !gEngfuncs.IsSpectateOnly())
		return;

	// setup the new highlight
	m_pCurrentHighlightLabel = label;
	m_iHighlightRow = row;
}

//-----------------------------------------------------------------------------
// Purpose: Label paint functions - take into account current highligh status
//-----------------------------------------------------------------------------
void CLabelHeader::paintBackground()
{
	Color oldBg;
	getBgColor(oldBg);

	if (gViewPort->GetScoreBoard()->m_iHighlightRow == _row)
	{
		setBgColor(134, 91, 19, 0);
	}

	Panel::paintBackground();

	setBgColor(oldBg);
}

//-----------------------------------------------------------------------------
// Purpose: Label paint functions - take into account current highligh status
//-----------------------------------------------------------------------------
void CLabelHeader::paint()
{
	Color oldFg;
	getFgColor(oldFg);

	if (gViewPort->GetScoreBoard()->m_iHighlightRow == _row)
	{
		setFgColor(255, 255, 255, 0);
	}

	// draw text
	int x, y, iwide, itall;
	getTextSize(iwide, itall);
	calcAlignment(iwide, itall, x, y);
	_dualImage->setPos(x, y);

	int x1, y1;
	_dualImage->GetImage(1)->getPos(x1, y1);
	_dualImage->GetImage(1)->setPos(_gap, y1);

	_dualImage->doPaint(this);

	// get size of the panel and the image
	if (_image)
	{
		Color imgColor;
		getFgColor(imgColor);
		if (_useFgColorAsImageColor)
		{
			_image->setColor(imgColor);
		}

		_image->getSize(iwide, itall);
		calcAlignment(iwide, itall, x, y);
		_image->setPos(x, y);
		_image->doPaint(this);
	}

	setFgColor(oldFg[0], oldFg[1], oldFg[2], oldFg[3]);
}

void CLabelHeader::calcAlignment(int iwide, int itall, int &x, int &y)
{
	// calculate alignment ourselves, since vgui is so broken
	int wide, tall;
	getSize(wide, tall);

	x = 0, y = 0;

	// align left/right
	switch (_contentAlignment)
	{
	// left
	case Label::a_northwest:
	case Label::a_west:
	case Label::a_southwest:
	{
		x = 0;
		break;
	}

	// center
	case Label::a_north:
	case Label::a_center:
	case Label::a_south:
	{
		x = (wide - iwide) / 2;
		break;
	}

	// right
	case Label::a_northeast:
	case Label::a_east:
	case Label::a_southeast:
	{
		x = wide - iwide;
		break;
	}
	}

	// top/down
	switch (_contentAlignment)
	{
	// top
	case Label::a_northwest:
	case Label::a_north:
	case Label::a_northeast:
	{
		y = 0;
		break;
	}

	// center
	case Label::a_west:
	case Label::a_center:
	case Label::a_east:
	{
		y = (tall - itall) / 2;
		break;
	}

	// south
	case Label::a_southwest:
	case Label::a_south:
	case Label::a_southeast:
	{
		y = tall - itall;
		break;
	}
	}

	// don't clip to Y
	//	if (y < 0)
	//	{
	//		y = 0;
	//	}
	if (x < 0)
	{
		x = 0;
	}

	x += _offset[0];
	y += _offset[1];
}

void CLabelHeader::setText(int textBufferLen, const char *text)
{
	_dualImage->GetImage(0)->setText(text);

	// calculate the text size
	Font *font = _dualImage->GetImage(0)->getFont();
	_gap = 0;
	for (const char *ch = text; *ch != 0; ch++)
	{
		int a, b, c;
		font->getCharABCwide(*ch, a, b, c);
		_gap += (a + b + c);
	}

	_gap += XRES(5);
}

void MSG_ScoreInfo(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	short cl = READ_BYTE();
	byte Flags = READ_BYTE();
	// (1<<0) - Is Elite
	// (1<<1) - Standing in transition
	// (1<<2) - Character is loaded
	msstring_ref sTitle = "Pending...";
	if (FBitSet(Flags, (1 << 2)))
		sTitle = READ_STRING();
	//byte	iTitle = READ_BYTE();
	//short	iSkillave = READ_SHORT();
	short iMaxHP = READ_SHORT();
	short iHP = READ_SHORT();
	if (cl > 0 && cl <= MAX_PLAYERS)
	{
		if (FBitSet(Flags, (1 << 2)))
			strncpy(g_PlayerExtraInfo[cl].Title, sTitle, sizeof(g_PlayerExtraInfo[cl].Title));
		//g_PlayerExtraInfo[cl].SkillAve = iSkillave;
		g_PlayerExtraInfo[cl].teamnumber = 0; //teamnumber;
		g_PlayerExtraInfo[cl].Flags = Flags;
		g_PlayerExtraInfo[cl].MaxHP = iMaxHP; //Shuriken FEB2008a
		g_PlayerExtraInfo[cl].HP = iHP;		  //Shuriken FEB2008a
		gViewPort->UpdateOnPlayerInfo();
	}
}
