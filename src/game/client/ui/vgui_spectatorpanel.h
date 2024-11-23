//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// vgui_SpectatorPanel.h: interface for the SpectatorPanel class.
//
//////////////////////////////////////////////////////////////////////

#ifndef SPECTATORPANEL_H
#define SPECTATORPANEL_H

#include <VGUI_Panel.h>
#include <VGUI_Label.h>
#include <VGUI_Button.h>

class ColorButton : public CommandButton
{
private:
	Color *ArmedColor;
	Color *UnArmedColor;

	Color *ArmedBorderColor;
	Color *UnArmedBorderColor;

public:
	ColorButton(const char *text, int x, int y, int wide, int tall, bool bNoHighlight, bool bFlat) : CommandButton(text, x, y, wide, tall, bNoHighlight, bFlat)
	{
		ArmedColor = NULL;
		UnArmedColor = NULL;
		ArmedBorderColor = NULL;
		UnArmedBorderColor = NULL;
	}

	virtual void paintBackground()
	{
		int r, g, b, a;
		Color bgcolor;

		if (isArmed())
		{
			// Highlight background
			/*	getBgColor(bgcolor);
			bgcolor.getColor(r, g, b, a);
			drawSetColor( r,g,b,a );
			drawFilledRect(0,0,_size[0],_size[1]);*/

			if (ArmedBorderColor)
			{
				ArmedBorderColor->getColor(r, g, b, a);
				drawSetColor(r, g, b, a);
				drawOutlinedRect(0, 0, _size[0], _size[1]);
			}
		}
		else
		{
			if (UnArmedBorderColor)
			{
				UnArmedBorderColor->getColor(r, g, b, a);
				drawSetColor(r, g, b, a);
				drawOutlinedRect(0, 0, _size[0], _size[1]);
			}
		}
	}
	void paint()
	{
		int r, g, b, a;
		if (isArmed())
		{
			if (ArmedColor)
			{
				ArmedColor->getColor(r, g, b, a);
				setFgColor(r, g, b, a);
			}
			else
				setFgColor(Scheme::sc_secondary2);
		}
		else
		{
			if (UnArmedColor)
			{
				UnArmedColor->getColor(r, g, b, a);
				setFgColor(r, g, b, a);
			}
			else
				setFgColor(Scheme::sc_primary1);
		}

		Button::paint();
	}

	void setArmedColor(int r, int g, int b, int a)
	{
		ArmedColor = new Color(r, g, b, a);
	}

	void setUnArmedColor(int r, int g, int b, int a)
	{
		UnArmedColor = new Color(r, g, b, a);
	}

	void setArmedBorderColor(int r, int g, int b, int a)
	{
		ArmedBorderColor = new Color(r, g, b, a);
	}

	void setUnArmedBorderColor(int r, int g, int b, int a)
	{
		UnArmedBorderColor = new Color(r, g, b, a);
	}
};

using namespace vgui;

#define SPECTATOR_PANEL_CMD_NONE 0

#define SPECTATOR_PANEL_CMD_OPTIONS 1
#define SPECTATOR_PANEL_CMD_PREVPLAYER 2
#define SPECTATOR_PANEL_CMD_NEXTPLAYER 3
#define SPECTATOR_PANEL_CMD_HIDEMENU 4
#define SPECTATOR_PANEL_CMD_TOGGLE_INSET 5
#define SPECTATOR_PANEL_CMD_CAMERA 6

#define TEAM_NUMBER 2

class SpectatorPanel : public Panel //, public vgui::CDefaultInputSignal
{

public:
	SpectatorPanel(int x, int y, int wide, int tall);
	virtual ~SpectatorPanel();

	void ActionSignal(int cmd);

	// InputSignal overrides.
public:
	void Initialize();
	void Update();

public:
	void EnableInsetView(bool isEnabled);
	void ShowMenu(bool isVisible);

	ColorButton *m_OptionButton;
	//	CommandButton     *	m_HideButton;
	ColorButton *m_PrevPlayerButton;
	ColorButton *m_NextPlayerButton;
	ColorButton *m_CamButton;

	CTransparentPanel *m_TopBorder;
	CTransparentPanel *m_BottomBorder;

	ColorButton *m_InsetViewButton;

	Label *m_BottomMainLabel;
	CImageLabel *m_TimerImage;
	Label *m_CurrentTime;
	Label *m_ExtraInfo;
	Panel *m_Separator;

	Label *m_TeamScores[TEAM_NUMBER];

	CImageLabel *m_TopBanner;

	bool m_menuVisible;
	bool m_insetVisible;
};

class CSpectatorHandler_Command : public ActionSignal
{

private:
	SpectatorPanel *m_pFather;
	int m_cmd;

public:
	CSpectatorHandler_Command(SpectatorPanel *panel, int cmd)
	{
		m_pFather = panel;
		m_cmd = cmd;
	}

	virtual void actionPerformed(Panel *panel)
	{
		m_pFather->ActionSignal(m_cmd);
	}
};

#endif // !defined SPECTATORPANEL_H
