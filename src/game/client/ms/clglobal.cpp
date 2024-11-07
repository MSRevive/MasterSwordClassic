#include "hud.h"
#include "cl_util.h"
#include "com_weapons.h"
#include "msdllheaders.h"
#include "clglobal.h"
#include "clenv.h"
#include "mscharacter.h"
#include "vgui_menudefsshared.h"
#include "player/player.h"
#include "hudmisc.h"
#include "hudscript.h"
#include "ms/vgui_hud.h"
#include "logger.h"
//#include "SteamClientHelper.h"
//#include "richpresence.h"

void VGUI_Think();

msstring MSCLGlobals::AuthID;								   //My steamID
bool MSCLGlobals::OnMyOwnListenServer;						   //Did I join my own listen server?
bool MSCLGlobals::CreatingCharacter;						   //Am in the process of creating a new character?
bool MSCLGlobals::CharPanelActive;							   //Choosing a character to play with?
bool MSCLGlobals::CamThirdPerson;							   //Camera is in thirdperson?
bool MSCLGlobals::OtherPlayers = false;						   //Other players who can legally play this map are on the server
std::vector<std::string> MSCLGlobals::m_Strings;			   //All client-side globally allocated strings
mslist<CBaseEntity *> MSCLGlobals::m_ClEntites;				   //All client-side entities
mslist<cl_entity_t> MSCLGlobals::m_ClModels;				   //Extra models to be updated/animated client-side
cl_entity_t MSCLGlobals::CLViewEntities[MAX_PLAYER_HANDITEMS]; //All View entity models
mslist<mstexture_t> MSCLGlobals::Textures;					   //Custom textures, to be rendered in a unique way (reflective, blended, etc)
hudcharanims_t MSCLGlobals::DefaultHUDCharAnims;			   //Anims for the char select VGUI
hudsounds_t MSCLGlobals::DefaultHUDSounds;					   //HUD sounds
hudcoords_t MSCLGlobals::DefaultHUDCoords;					   //HUD placement coordinates
int MSCLGlobals::mapDarkenLevel = 0;						   //Map's custom bloom darkening MiB 31_DEC2010

// Local version of game .dll global variables ( time, etc. )
globalvars_t Globals;

void MSCLGlobals::AddEnt(CBaseEntity *pEntity)
{
	pEntity->pev = msnew(entvars_t);
	pEntity->pev->pContainingEntity = (edict_t *)pEntity;
	pEntity->pev->nextthink = -1;

	m_ClEntites.add(pEntity);
}
void MSCLGlobals::RemoveEnt(CBaseEntity *pEntity, bool fDelete)
{
	for (int e = 0; e < m_ClEntites.size(); e++)
		if (m_ClEntites[e] == pEntity)
		{
			m_ClEntites.erase(e);
			break;
		}
	if (fDelete)
	{
		::delete pEntity->pev;
		::delete pEntity;
	}
}

//Global one-time Initialization - called from CHud :: Init()
void MSCLGlobals::Initialize()
{
	startdbg;

	dbg("Begin");
	//Set up g_engfuncs re-directs
	gpGlobals = &Globals;
	SetupGlobalEngFuncRedirects();

	// Set up pointer
	// Fill in current time
	gpGlobals->time = gEngfuncs.GetClientTime();

	// //Client CVARs
	// CVAR_CREATE("ms_status_icons", "1", FCVAR_CLIENTDLL); // Drigien MAY2008 - Shows/Hides The HUD Status Icons
	// //JAN2010_11 - not optional - used to blind player sometimes
	// //CVAR_CREATE( "ms_showhudimgs", "1", FCVAR_CLIENTDLL );		// Drigien MAY2008 - Shows/Hides The HUD Images
	// CVAR_CREATE("hud_classautokill", "1", FCVAR_ARCHIVE);				// controls whether or not to suicide immediately on TF class switch
	// CVAR_CREATE("hud_takesshots", "0", FCVAR_ARCHIVE);					// controls whether or not to automatically take screenshots at the end of a round
	// CVAR_CREATE("ms_hidehud", "0", FCVAR_ARCHIVE);						// Hides the HUD and viewmodel completely
	// CVAR_CREATE("ms_lildude", "1", FCVAR_ARCHIVE);						// Thothie MAR2007a - Hides the 3d Guy if set 0
	// CVAR_CREATE("ms_clgender", "0", FCVAR_CLIENTDLL);					// Thothie FEB2011_22 - Adding a cvar to store client gender
	// CVAR_CREATE("ms_xpdisplay", "0", FCVAR_ARCHIVE);					// Thothie AUG2007a - XP Display Options
	// CVAR_CREATE("ms_developer", "0", FCVAR_CLIENTDLL);					// Thothie MAR2007b - Hides client side developer messages when set to 0
	// CVAR_CREATE("ms_evthud_decaytime", "5", FCVAR_ARCHIVE);				// Time each line in the Event Console lasts before it shrinks
	// CVAR_CREATE("ms_evthud_history", "10", FCVAR_ARCHIVE);				// Max number of text lines to keep in the Event Console history
	// CVAR_CREATE("ms_evthud_size", "5", FCVAR_ARCHIVE);					// Max number of text lines shown at once
	// CVAR_CREATE("ms_evthud_bgtrans", "0", FCVAR_CLIENTDLL);				// Transparency of the background
	// CVAR_CREATE("ms_txthud_decaytime", "9", FCVAR_ARCHIVE);				// Time each line in the Event Console lasts before it shrinks
	// CVAR_CREATE("ms_txthud_history", "50", FCVAR_ARCHIVE);				// Max number of text lines to keep in the Event Console history
	// CVAR_CREATE("ms_txthud_size", "8", FCVAR_ARCHIVE);					// Max number of text lines shown at once
	// CVAR_CREATE("ms_txthud_bgtrans", "0", FCVAR_ARCHIVE);				// Transparency of the background
	// CVAR_CREATE("ms_txthud_width", "640", FCVAR_ARCHIVE);				// Width of console
	// CVAR_CREATE(CVAR_HELPTIPS, "1", FCVAR_ARCHIVE /*|FCVAR_USERINFO*/); // Whether help tips are shown
	// CVAR_CREATE("ms_reflect", "1", FCVAR_ARCHIVE);						// Allow reflective surfaces
	// CVAR_CREATE("ms_reflect_dbg", "0", FCVAR_ARCHIVE);					// Debug reflective surfaces
	// CVAR_CREATE("ms_bloom_darken", "-1", FCVAR_ARCHIVE);				// MiB DEC2010 - Darken bloom
	// CVAR_CREATE("ms_bloom_level", "0", FCVAR_ARCHIVE);					// Thothie DEC2010_30 - Fix ms_bloom_level stickiness
	// //CVAR_CREATE("ms_reconnect_delay", "5", FCVAR_ARCHIVE);				// Thothie AUG2017 - Make reconnect delay adjustable client side
	// CVAR_CREATE(MSCVAR_QUICKSLOT_TIMEOUT, "2.5", FCVAR_ARCHIVE);		// Timeout for the quickslots
	// CVAR_CREATE("ms_autocharge", "1", FCVAR_ARCHIVE);					// MiB MAR2012_05 - Let's you auto-charge your attack
	// CVAR_CREATE("ms_doubletapdodge", "0", FCVAR_ARCHIVE);				// MiB MAR2012_05 -Enable/Disable double tapping to dodge
	// CVAR_CREATE("ms_invtype", "1", FCVAR_ARCHIVE);						// MiB FEB2012_12 - Inventory types (added post-doc by Thothie)
	// CVAR_CREATE("ms_showotherglow", "1", FCVAR_ARCHIVE);
	// CVAR_CREATE("ms_chargebar_sound", "magic/chargebar_alt1.wav", FCVAR_ARCHIVE);
	// CVAR_CREATE("ms_chargebar_volume", "15", FCVAR_ARCHIVE);
	// CVAR_CREATE("ms_doubletap_delay", "0.45", FCVAR_ARCHIVE); // The higher the amount, the longer the player has to hit left/right/back for a second time
	// CVAR_CREATE("ms_sprint_verbose", "2", FCVAR_ARCHIVE); // 0 for no messages , 1 for only warnings , 2 for everything
	// CVAR_CREATE("ms_sprint_toggle", "1", FCVAR_ARCHIVE);
	// CVAR_CREATE("ms_sprint_doubletap", "1", FCVAR_ARCHIVE);

	//CVAR_CREATE("ms_alpha_inventory", "0", FCVAR_ARCHIVE); // MiB FEB2019_24 [ALPHABETICAL_INVENTORY]
	//CVAR_CREATE("ms_hands_display", "2", FCVAR_ARCHIVE);
	//CVAR_CREATE("ms_hands_display_y", "20", FCVAR_ARCHIVE);
	//CVAR_CREATE("ms_scrollamount", "30", FCVAR_ARCHIVE);
	//CVAR_CREATE("ms_doubleclicktime", "1", FCVAR_ARCHIVE);

	/*msstring DefaultLANID = "this_must_be_unique";
	gEngfuncs.pfnRegisterVariable( "ms_id", DefaultID, FCVAR_USERINFO|FCVAR_ARCHIVE );

	if( DefaultLANID == (const char *)gEngfuncs.pfnGetCvarString("ms_id") )
	{
		//If I'm still using the default ID, generate a random one
		char ID[8];
		 for (int i = 0; i < 6; i++)
		{
			int CharType = RANDOM_LONG(0,2);
			int Low, High;
			switch( CharType ) {
				case 0: Low = '0'; High = '9'; break;
				case 1: Low = 'a'; High = 'z'; break;
				case 2: Low = 'A'; High = 'Z'; break;
			}
			ID[i] = RANDOM_LONG( Low, High ); ID[i+1] = 0;
		}
		gEngfuncs.pfnClientCmd( msstring("ms_id ") + ID + "\n" );
	}*/

	dbg("Call InitializePlayer");
	InitializePlayer();

	dbg("Call MSGlobalItemInit");
	MSGlobalItemInit();

	enddbg;
}

//Player initialization that happens every map
void MSCLGlobals::InitializePlayer()
{
	if (!player.pev)
		AddEnt(&player);

	player.CreateStats();
	player.m_CharacterState = CHARSTATE_UNLOADED;
	player.m_DisplayName = "Adventurer";
	player.PlayerHands = NULL;
	player.ClearConditions(MONSTER_OPENCONTAINER);
	player.Gear.clear();
	player.m_HP = player.m_MP = 0;

	player.m_fGameHUDInitialized = TRUE;
	//player.pbs.fMaxForwardPressTime = 0;
	player.m_SprintDelay = gpGlobals->time;
	player.m_Initialized = false;

	for (int i = 0; i < MAX_PLAYER_HANDITEMS; i++)
	{
		MSCLGlobals::CLViewEntities[i].index = MSGlobals::ClEntities[(i != 2) ? CLPERMENT_LEFTVIEW + i : CLPERMENT_LEFTVIEW];
		MSCLGlobals::CLViewEntities[i].curstate.number = MSGlobals::ClEntities[(i != 2) ? CLPERMENT_LEFTVIEW + i : CLPERMENT_LEFTVIEW];
	}
}

//Global think - sure to be called every frame
void MSCLGlobals::Think()
{
	static float flLastThinkTime = 0;
	gpGlobals->time = gEngfuncs.GetClientTime();
	gpGlobals->frametime = gpGlobals->time - flLastThinkTime;

	//if( player.m_CharacterState == CHARSTATE_UNLOADED ) return;

	//Delete entities with |FL_KILLME
	//Count backward
	for (int e = m_ClEntites.size() - 1; e >= 0; e--)
		if (FBitSet(m_ClEntites[e]->pev->flags, FL_KILLME))
			RemoveEnt(m_ClEntites[e], m_ClEntites[e] != &player);

	//Call entity Think() functions
	for (int e = 0; e < m_ClEntites.size(); e++)
		if (flLastThinkTime <= m_ClEntites[e]->pev->nextthink && m_ClEntites[e]->pev->nextthink < gpGlobals->time)
			m_ClEntites[e]->Think();

	VGUI_Think();

	//Last task
	flLastThinkTime = gpGlobals->time;
}

void MSCLGlobals::PrintAllEntites()
{
	Print("Global Items...\n");

	int items = 0;
	for (int e = 0; e < m_ClEntites.size(); e++)
		Print("Item %i: %s", items++, m_ClEntites[e]->DisplayName());
}
void MSCLGlobals::RemoveAllEntities()
{
	//Delete all entites
	if (player.m_CharacterState == CHARSTATE_LOADED)
	{
		player.Killed(NULL, 0);
		player.RemoveAllItems(false, true);
	}

	//There shouldn't be anything left here, but the player... which is skipped.
	//If any items get deleted below, conside it an error and make sure it gets
	//deleted beforehand
	//(presumably within RemoveAllItems() somewhere)
	int killed = 0;
	for (int e = 0; e < m_ClEntites.size(); e++)
	{
		CBaseEntity *pEntity = m_ClEntites[e];
		if (!pEntity) continue;

		//Skip player
		if (pEntity == &player) continue;

		//unset player hands (should happen in CBasePlayer::RemoveAllItems() first...)
		if (pEntity == (CBaseEntity *)player.PlayerHands)
			player.PlayerHands = NULL;

		Print("Cleanup Item %i: %s\n", killed++, pEntity->DisplayName());

		if (pEntity->pev)
			SetBits(pEntity->pev->flags, FL_KILLME);
	}
	//logfile << "Global Cleanup: " << killed << " unreferenced entites.\r\n";
	MSCLGlobals::Think();

	m_ClModels.clear(); //Cleanup client-side models/sprites
	std::vector<std::string>().swap(m_Strings); //Cleanup allocated strings, hopefully this prevents com_loadfile error...

	//Re-initialize player
	InitializePlayer();
}

void MSCLGlobals::EndMap()
{
	ChooseChar_Interface::CentralServer = false; //Reset
}

void MSCLGlobals::DLLDetach()
{
	player.Deactivate();
}

int MSCLGlobals::GetLocalPlayerIndex()
{
	cl_entity_t *clPlayer = gEngfuncs.GetLocalPlayer();
	return clPlayer ? clPlayer->index : 1;
}

//Client versions of these functions
const char *EngineFunc::GetGameDir()
{
	return gEngfuncs.pfnGetGameDirectory();
}

void AlertMessage(ALERT_TYPE atype, const char *szFmt, ...)
{
	static char string[1024];

	va_list argptr;
	va_start(argptr, szFmt);
	vsnprintf(string, sizeof(string), szFmt, argptr);
	va_end(argptr);

	ConsolePrint("cl: ");
	ConsolePrint(string);
}

char* UTIL_VarArgs(const char *format, ...)
{
	static char string[1024];

	va_list argptr;	
	va_start(argptr, format);
	vsnprintf(string, sizeof(string), format, argptr);
	va_end(argptr);

	return (char *)string;
}

Vector UTIL_VecToAngles(const Vector &vec)
{
	float rgflVecOut[3];
	AngleVectors(vec, rgflVecOut, NULL, NULL);
	return Vector(rgflVecOut);
}

string_t MSCLGlobals::AllocString(const char *pszString)
{
	if (!pszString)
		return 0;

	size_t size = m_Strings.size();
	for (int s = 0; s < size; s++)
	{
		if (FStrEq(m_Strings[s].c_str(), pszString))
			return m_Strings[s].c_str() - gpGlobals->pStringBase;
	}

	size_t len = strlen(pszString) + 1;
	char *pszNewString = new(char[len]);
	strncpy(pszNewString, pszString, len);
	
	m_Strings.push_back(std::string(pszNewString));

	return pszNewString - gpGlobals->pStringBase;
}
/*char *MSCLGlobals::GetString( string_t sString )
{
	if( !sString ) return "(NULL)";
	return (char *)STRING( sString );
}*/
void MSCLGlobals::SetupGlobalEngFuncRedirects(void)
{
	// Fake functions
	g_engfuncs.pfnPrecacheModel = stub_PrecacheModel;
	g_engfuncs.pfnPrecacheSound = stub_PrecacheSound;
	g_engfuncs.pfnPrecacheEvent = stub_PrecacheEvent;
	g_engfuncs.pfnNameForFunction = stub_NameForFunction;
	g_engfuncs.pfnSetModel = stub_SetModel;
	g_engfuncs.pfnSetClientMaxspeed = HUD_SetMaxSpeed;

	// Handled locally
	g_engfuncs.pfnPlaybackEvent = HUD_PlaybackEvent;
	g_engfuncs.pfnAllocString = AllocString;
	g_engfuncs.pfnAlertMessage = AlertMessage;
	gpGlobals->pStringBase = "(NULL)";

	// Pass through to engine
	g_engfuncs.pfnPrecacheEvent = gEngfuncs.pfnPrecacheEvent;
	g_engfuncs.pfnRandomFloat = gEngfuncs.pfnRandomFloat;
	g_engfuncs.pfnRandomLong = gEngfuncs.pfnRandomLong;
}
//I've recieved all script files, I can now spawn
void CreateStoreMenus();
void ShowVGUIMenu(int iMenu);

void MSCLGlobals::SpawnIntoServer()
{
	startdbg;

	logfile << Logger::LOG_INFO << "SpawnIntoServer...";

	Cleanup(); //Clean up stuff from the previous map

	dbg("Call player.InitialSpawn");
	player.InitialSpawn();
	player.BeginRender();

	dbg("Call CreateStoreMenus");
	CreateStoreMenus();

	dbg("Call MSChar_Interface::CLInit");
	MSChar_Interface::CLInit();

	dbg("ShowVGUIMenu( MENU_NEWCHARACTER )");
	ShowVGUIMenu(MENU_NEWCHARACTER);

	logfile << "DONE\n";

	enddbg;
}

//Cleans up stuff from the previous map
void MSCLGlobals::Cleanup()
{
	startdbg;

	//Remove spell list
	player.m_SpellList.clear();

	//Kill the client-side entity list
	dbg("Call CBasePlayeR::RenderCleanup");
	player.RenderCleanup();

	// Delete all client-side entities
	dbg("Call MSCLGlobals::RemoveAllEntities( )");
	RemoveAllEntities();

	//Remove Environment Special Effects
	CRender::Cleanup();

	enddbg;
}

void DLLAttach(HINSTANCE hinstDLL)
{
	MSGlobals::DLLAttach(hinstDLL);
}

void DLLDetach()
{
	//if( logfile.is_open() ) logfile << __FILE__ << ":" << ((int)__LINE__) << " client.dll being unloaded" << endl;
	if (logfile.is_open())
		(((logfile << Logger::LOG_INFO << __FILE__) << " client.dll being unloaded\n"));
	//RichPresenceShutdown();
	MSGlobals::EndMap();
	MSCLGlobals::DLLDetach();
	MSGlobals::DLLDetach();
}

#if _WIN32
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
		DLLAttach(hinstDLL);
	else if (fdwReason == DLL_PROCESS_DETACH)
		DLLDetach();
	return TRUE;
}
#endif
