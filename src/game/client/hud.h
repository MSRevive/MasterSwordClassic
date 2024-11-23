/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
//  hud.h
//
// class CHud declaration
//
// CHud handles the message, calculation, and drawing the HUD
//

//Master Sword - Control the HUDs to be hidden, or shown
bool ShowHUD();
bool ShowHealth();
bool ShowChat();
//--------------

class CHudFatigue;
class CHudStats;
class CHudMagic;
class CHudMusic;
class CHudAction;
class CHudHealth;
class CHudMenu;
class CHudScript;
class CHudID;
class CHudMisc;

///////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\//

#define RGB_YELLOWISH 0x00FFA000 //255,160,0
#define RGB_REDISH 0x00FF1010	 //255,160,0
#define RGB_GREENISH 0x0000A000	 //0,160,0

#include "wrect.h"
#include "cl_dll.h"
#include "ammo.h"

#define DHN_DRAWZERO 1
#define DHN_2DIGITS 2
#define DHN_3DIGITS 4
#define MIN_ALPHA 100

#define HUDELEM_ACTIVE 1

enum
{
	MAX_PLAYERS = 64,
	MAX_TEAMS = 64,
	MAX_TEAM_NAME = 16,
};

typedef struct
{
	int x, y;
} POSITION;

typedef struct
{
	unsigned char r, g, b, a;
} RGBA;

typedef struct cvar_s cvar_t;

#define HUD_ACTIVE 1
#define HUD_INTERMISSION 2

#define MAX_PLAYER_NAME_LENGTH 32

#define MAX_MOTD_LENGTH 1024

//					Master Sword
//-----------------------------------------------------
//=====================================================
#include <vector>
#include "ms/hudbase.h"
#ifndef VALVE_DLL
#include "voice_status.h" //SDK 2.3
#endif
#include "hud_spectator.h" //SDK 2.3

void Print(const char *szFmt, ...);
void ShowVGUIMenu(int iMenu);

//ripped from windef.h
#ifndef BOOL
typedef int BOOL;
#endif

//Moved here from CHudStatusIcons
#define MAX_SPRITE_NAME_LENGTH 24

typedef struct
{
	char szSpriteName[MAX_SPRITE_NAME_LENGTH];
	HLSPRITE spr;
	wrect_t rc;
	unsigned char r, g, b;
	int id, index, frame;
	bool Display;
} icon_sprite_t;

#ifndef VALVE_DLL
//Bitmap functions
class MSBitmap
{
public:
	static class vgui::BitmapTGA *GetTGA(const char *pszImageName);
	static HLSPRITE GetSprite(const char *pszImageName);
	static HLSPRITE LoadSprite(const char *pszImageName);
	static void ReloadSprites();
};
//------------
#endif

//
//-----------------------------------------------------
//

class CHudAmmoSecondary : public CHudBase
{
public:
	int Init(void);
	int VidInit(void);
	void Reset(void);
	int Draw(float flTime);

	int MsgFunc_SecAmmoVal(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_SecAmmoIcon(const char *pszName, int iSize, void *pbuf);

private:
	enum
	{
		MAX_SEC_AMMO_VALUES = 4
	};

	int m_HUD_ammoicon; // sprite indices
	int m_iAmmoAmounts[MAX_SEC_AMMO_VALUES];
	float m_fFade;
};

#define FADE_TIME 100

//
//-----------------------------------------------------
//
class CHudGeiger : public CHudBase
{
public:
	int Init(void);
	int VidInit(void);
	int Draw(float flTime);
	int MsgFunc_Geiger(const char *pszName, int iSize, void *pbuf);

private:
	int m_iGeigerRange;
};

//
//-----------------------------------------------------
//
class CHudTrain : public CHudBase
{
public:
	int Init(void);
	int VidInit(void);
	int Draw(float flTime);
	int MsgFunc_Train(const char *pszName, int iSize, void *pbuf);

private:
	HLSPRITE m_hSprite;
	int m_iPos;
};

//
//-----------------------------------------------------
//
class CHudStatusBar : public CHudBase
{
public:
	int Init(void);
	int VidInit(void);
	int Draw(float flTime);
	void Reset(void);
	void ParseStatusString(int line_num);

	int MsgFunc_StatusText(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_StatusValue(const char *pszName, int iSize, void *pbuf);

protected:
	enum
	{
		MAX_STATUSTEXT_LENGTH = 128,
		MAX_STATUSBAR_VALUES = 8,
		MAX_STATUSBAR_LINES = 2,
	};

	char m_szStatusText[MAX_STATUSBAR_LINES][MAX_STATUSTEXT_LENGTH]; // a text string describing how the status bar is to be drawn
	char m_szStatusBar[MAX_STATUSBAR_LINES][MAX_STATUSTEXT_LENGTH];	 // the constructed bar that is drawn
	int m_iStatusValues[MAX_STATUSBAR_VALUES];						 // an array of values for use in the status bar

	int m_bReparseString; // set to TRUE whenever the m_szStatusBar needs to be recalculated
};

extern int g_IsSpectator[MAX_PLAYERS + 1];

//
//-----------------------------------------------------
//
class CHudDeathNotice : public CHudBase
{
public:
	int Init(void);
	void InitHUDData(void);
	int VidInit(void);
	int Draw(float flTime);
	int MsgFunc_DeathMsg(const char *pszName, int iSize, void *pbuf);

private:
	int m_HUD_d_skull; // sprite index of skull icon
};

//
//-----------------------------------------------------
//
class CHudSayText : public CHudBase
{
public:
	int Init(void);
	void InitHUDData(void);
	int VidInit(void);
	int Draw(float flTime);
	int MsgFunc_SayText(const char *pszName, int iSize, void *pbuf);
	void SayTextPrint(const char *pszBuf, int iBufSize, int clientIndex = -1);
	void EnsureTextFitsInOneLineAndWrapIfHaveTo(int line);

	friend class CHudSpectator;

private:
	struct cvar_s *m_HUD_saytext;
	struct cvar_s *m_HUD_saytext_time;
};

//
//-----------------------------------------------------
//
const int maxHUDMessages = 16;
struct message_parms_t
{
	client_textmessage_t *pMessage;
	float time;
	int x, y;
	int totalWidth, totalHeight;
	int width;
	int lines;
	int lineLength;
	int length;
	int r, g, b;
	int text;
	int fadeBlend;
	float charTime;
	float fadeTime;
};

//
//-----------------------------------------------------
//

class CHudTextMessage : public CHudBase
{
public:
	int Init(void);
	static char *LocaliseTextString(const char *msg, char *dst_buffer, int buffer_size);
	static char *BufferedLocaliseTextString(const char *msg);
	char *LookupString(const char *msg_name, int *msg_dest = NULL);
	int MsgFunc_TextMsg(const char *pszName, int iSize, void *pbuf);
};

//
//-----------------------------------------------------
//

class CHudMessage : public CHudBase
{
public:
	int Init(void);
	int VidInit(void);
	int Draw(float flTime);
	int MsgFunc_HudText(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_GameTitle(const char *pszName, int iSize, void *pbuf);

	float FadeBlend(float fadein, float fadeout, float hold, float localTime);
	int XPosition(float x, int width, int lineWidth);
	int YPosition(float y, int height);

	void MessageAdd(const char *pName, float time);
	void MessageAdd(client_textmessage_t &msg); //Master Sword
	void MessageDrawScan(client_textmessage_t *pMessage, float time);
	void MessageScanStart(void);
	void MessageScanNextChar(void);
	void Reset(void);

private:
	client_textmessage_t *m_pMessages[maxHUDMessages];
	float m_startTime[maxHUDMessages];
	message_parms_t m_parms;
	float m_gameTitleTime;
	client_textmessage_t *m_pGameTitle;

	int m_HUD_title_life;
	int m_HUD_title_half;
};

//
//-----------------------------------------------------
//

class CHudStatusIcons : public CHudBase
{
public:
	int Init(void);
	int VidInit(void);
	void Reset(void);
	int Draw(float flTime);
	int MsgFunc_StatusIcon(const char *pszName, int iSize, void *pbuf);

	enum
	{
		MAX_ICONSPRITENAME_LENGTH = MAX_SPRITE_NAME_LENGTH,
		MAX_ICONSPRITES = 4,
	};

	//had to make these public so CHud could access them (to enable concussion icon)
	//could use a friend declaration instead...
	void EnableIcon(char *pszIconName, unsigned char red, unsigned char green, unsigned char blue);
	void DisableIcon(char *pszIconName);

private:
	icon_sprite_t m_IconList[MAX_ICONSPRITES];
};

//
//-----------------------------------------------------
//

class CHud
{
private:
	struct HudSprite {
		char Name[MAX_SPRITE_NAME_LENGTH];
		char SpriteName[64];
		HLSPRITE Handle = 0;
		wrect_t Rectangle{0, 0, 0, 0};
	};

	std::vector<HudSprite> m_Sprites;
	std::vector<CHudBase*> m_HudList;

	HLSPRITE m_hsprLogo;
	int m_iLogo;

	float m_flMouseSensitivity;
	int m_iConcussionEffect;
	struct cvar_s *default_fov;

public:
	HLSPRITE m_hsprCursor;
	float m_flTime;		  // the current client time
	float m_fOldTime;	  // the time at which the HUD was last redrawn
	double m_flTimeDelta; // the difference between flTime and fOldTime
	Vector m_vecOrigin;
	Vector m_vecAngles;
	int m_iKeyBits;
	int m_iHideHUDDisplay;
	int m_iFOV;
	int m_Teamplay;
	int m_iRes;
	cvar_t *m_pCvarStealMouse;

	int m_iFontHeight;
	int DrawHudNumber(int x, int y, int iFlags, int iNumber, int r, int g, int b);
	int DrawHudString(int x, int y, int iMaxX, char *szString, int r, int g, int b);
	int DrawHudStringReverse(int xpos, int ypos, int iMinX, char *szString, int r, int g, int b);
	int DrawHudNumberString(int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b);
	int GetNumWidth(int iNumber, int iFlags);

	//Master Sword
	int DrawHudNumberSML(int x, int y, int iFlags, int iNumber, int r, int g, int b);
	int DrawHudStringSML(int x, int y, char *pcString, int r, int g, int b, int iExtraSpace = 0);
	//------------

public:
	HLSPRITE GetSprite(int index)
	{
		return (index < 0) ? 0 : m_Sprites[index].Handle;
	}

	wrect_t& GetSpriteRect(int index)
	{
		//wrect_t rect = wrect_t.Rectangle{0,0,0,0};
		//return (index < 0) ? wrect_t{} : m_Sprites[index].Rectangle;
		wrect_t rect = {0,0,0,0};
		return (index < 0) ? rect : m_Sprites[index].Rectangle;
	}

	int GetSpriteIndex(const char *SpriteName); // gets a sprite index, for use in the m_rghSprites[] array

	//CHudAmmo	m_Ammo;
	CHudTrain m_Train;
	CHudMessage m_Message;
	CHudStatusBar m_StatusBar;
	CHudDeathNotice m_DeathNotice;
	CHudSayText m_SayText;
	CHudAmmoSecondary m_AmmoSecondary;
	CHudTextMessage m_TextMessage;
	CHudStatusIcons m_StatusIcons;
	CHudSpectator m_Spectator; //SDK 2.3

	//MasterSword
	CHudHealth *m_Health;
	CHudFatigue *m_Fatigue;
	CHudMagic *m_Magic;
	CHudMusic *m_Music;
	CHudAction *m_Action;
	CHudMenu *m_Menu;
	CHudScript *m_HUDScript;
	CHudMisc *m_Misc;
	CHudID *m_HUDId;
	//-----------

	void Init(void);
	void Shutdown(void);
	void VidInit(void);
	void Think(void);
	int Redraw(float flTime, int intermission);
	void ReloadClient();
	int UpdateClientData(client_data_t *cdata, float time);

	CHud() = default;
	~CHud() = default; // destructor, frees allocated memory

	// user messages
	int MsgFunc_Damage(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf);
	//int MsgFunc_Logo(const char *pszName,  int iSize, void *pbuf);
	int MsgFunc_ResetHUD(const char *pszName, int iSize, void *pbuf);
	void MsgFunc_InitHUD(const char *pszName, int iSize, void *pbuf);
	void MsgFunc_ViewMode(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_SetFOV(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_ViewModel(const char *pszName, int iSize, void *pbuf);
	//int MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf );
	// Screen information
	SCREENINFO m_scrinfo;

	int m_iWeaponBits;
	int m_fPlayerDead;
	int m_iIntermission;

	// sprite indexes
	int m_HUD_number_0;
	//Master Sword
	int m_HUD_numberSML_0, m_HUD_char_a, m_HUD_char_A,
		m_HUD_char_slashSML, m_HUD_char_slash, m_HUD_char_colon;

	void AddHudElem(CHudBase *p);

	float GetSensitivity();
};

class TeamFortressViewport;

extern CHud gHUD;
extern TeamFortressViewport *gViewPort;

extern int g_iPlayerClass;
extern int g_iTeamNumber;
extern int g_iUser1;
extern int g_iUser2;
extern int g_iUser3;
