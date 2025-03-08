#include "msdllheaders.h"
#include "stats/stats.h"
#include "stats/races.h"
#include "svglobals.h"
#include "logger.h"
#include "global.h"
#include "weapons/genericitem.h"
#include "gamerules/gamerules.h"
#include "store.h"
#include "cstringpool.h"
#include "crc/crchash.h"
#include "filesystem_shared.h"
#include "SteamServerHelper.h"
#include "fn/FNSharedDefs.h"
#include "fn/RequestManager.h"
#include "fn/HTTPRequest.h"

std::ofstream modelout;
int HighestPrecache = -1;
int TotalModelPrecaches = 1;
int PreCount = 0; //Thothie OCT2007a Precache map verification
bool gFNInitialized = false;

bool CSVGlobals::LogScripts = true;
mslist<CSVGlobals::scriptlistitem_t> CSVGlobals::ScriptList[SCRIPT_TYPES];

//Thothie MAR2012_27 - duplicate precache trackers
int gModelPrecacheCount = 0;
int gSoundPrecacheCount = 0;
mslist<modelprecachelist_t> gModelPrecacheList;
mslist<modelprecachelist_t> gSoundPrecacheList;

CStringPool g_StringPool;
CRequestManager g_FNRequestManager;

//Master Sword CVARs
/*
cvar_t	debug			= {"ms_debug","0", FCVAR_SERVER };
cvar_t	betakey			= {"ms_key","", FCVAR_PROTECTED|FCVAR_EXTDLL };*/
cvar_t ms_dev_mode = {"ms_dev_mode", "0", FCVAR_SERVER}; //MiB JUL2010_13 - Dev mode..
cvar_t ms_dynamicnpc = {"ms_dynamicnpc", "", 0};
cvar_t msallowkickvote = {"ms_allowkickvote", "1", FCVAR_SERVER};
cvar_t msallowtimevote = {"ms_allowtimevote", "1", FCVAR_SERVER};
cvar_t ms_reset_time = {"ms_reset_time", "10", FCVAR_SERVER};
cvar_t ms_reset_map = {"ms_reset_map", "edana", FCVAR_SERVER};
cvar_t ms_version = {"ms_version", __DATE__, FCVAR_EXTDLL};
cvar_t ms_pklevel = {"ms_pklevel", "0", FCVAR_SERVER};
//cvar_t	ms_trans_req	= {"ms_trans_req","0", FCVAR_SERVER }; //Thothie JUN2007 - max players required to activate a transition (0 = all on server) - nvm, changed method - nvm, changed method
cvar_t ms_fxlimit = {"ms_fxlimit", "0", FCVAR_SERVER};
//cvar_t	ms_currentfx	= {"ms_currentfx","0", 0 }; //Thothie - want to make FX control total ms.dll, but can't figure how
cvar_t ms_serverchar = {"ms_serverchar", "1", FCVAR_SERVER};
cvar_t ms_joinreset = {"ms_joinreset", "1", FCVAR_SERVER};
cvar_t ms_hp_limit = {"ms_hp_limit", "0", FCVAR_SERVER};
cvar_t msvote_farm_all_day = {"msvote_farm_all_day", "0", FCVAR_SERVER};		 //Thothie FEB2008a - Allow voting for the map the players are on
cvar_t msvote_map_type = {"msvote_map_type", "all", FCVAR_SERVER};				 //Thothie FEB2008a - Map vote type (current: all, and root)
cvar_t msvote_map_enable = {"msvote_map_enable", "1", FCVAR_SERVER};			 //Thothie FEB2008a - Map vote enable
cvar_t msvote_kick_enable = {"msvote_kick_enable", "1", FCVAR_SERVER};			 //Thothie FEB2008a - Vote kick at chat/console enable
cvar_t msvote_ban_enable = {"msvote_ban_enable", "1", FCVAR_SERVER};			 //Thothie FEB2008a - Vote ban at chat/console enable
cvar_t msvote_ban_time = {"msvote_ban_time", "60", FCVAR_SERVER};				 //Thothie FEB2008a - Time to ban a person voted banned
cvar_t msvote_pvp_enable = {"msvote_pvp_enable", "60", FCVAR_SERVER};			 //Thothie FEB2008a - Allow voting for PVP
cvar_t msvote_lock_enable = {"msvote_lock_enable", "1", FCVAR_SERVER};			 //Thothie JAN2011_04 - Allow voting to password lock server
cvar_t ms_chatlog = {"ms_chatlog", "1", FCVAR_SERVER};							 //Thothie FEB2008a - write chatlog
cvar_t ms_admin_contact = {"ms_admin_contact", "[none provided]", FCVAR_SERVER}; //Thothie FEB2008a - server host contact info
cvar_t ms_ban_to_cfg = {"ms_ban_to_cfg", "1", FCVAR_SERVER};					 //Thothie FEB2008a - write scriptside perm bans to server/listenserver.cfg
cvar_t ms_central_enabled = {"ms_central_enabled", "0", FCVAR_SERVER};
cvar_t ms_fake_hp = {"ms_fake_hp", "0", FCVAR_SERVER};			 //Thothie AUG2011_17 - moving Fakehp to cvar for use with triggers
cvar_t ms_fake_players = {"ms_fake_players", "0", FCVAR_SERVER}; //Thothie DEC2013_07 - for returning false # of players for some functions during testing
cvar_t ms_central_addr = {"ms_central_addr", "0", FCVAR_PROTECTED};
cvar_t ms_debug_mem = {"ms_debug_mem", "0", 0};
//cvar_t ms_crashcfg = {"ms_crashcfg", "crashed", FCVAR_SERVER};

#ifdef DEV_BUILD
cvar_t ms_devlog = {"ms_devlog", "1", 0};
cvar_t ms_allowdev = {"ms_allowdev", "1", 0};
#endif

//------------

//SOCKET g_PingSock;
void UnBanAll();

bool MSGlobalInit() //Called upon DLL Initialization
{
	//Master Sword CVARs
	CVAR_REGISTER(&ms_version);
	CVAR_REGISTER(&ms_pklevel);
	//CVAR_REGISTER (&ms_trans_req); //Thothie JUN2007 - max players required to activate a transition (0 = all on server)
	CVAR_REGISTER(&ms_fxlimit);
	//CVAR_REGISTER (&debug);
	//CVAR_REGISTER (&betakey);
	CVAR_REGISTER(&ms_dev_mode); //MiB "Mapper Scripts.rtf"
	CVAR_REGISTER(&ms_dynamicnpc);
	CVAR_REGISTER(&msallowkickvote);
	CVAR_REGISTER(&msallowtimevote);
	CVAR_REGISTER(&ms_serverchar);
	CVAR_REGISTER(&ms_joinreset);
	CVAR_REGISTER(&ms_reset_time);
	CVAR_REGISTER(&ms_reset_map);
	CVAR_REGISTER(&ms_hp_limit);
	CVAR_REGISTER(&msvote_farm_all_day);
	CVAR_REGISTER(&msvote_map_type);
	CVAR_REGISTER(&msvote_map_enable);
	CVAR_REGISTER(&msvote_kick_enable);
	CVAR_REGISTER(&msvote_ban_enable);
	CVAR_REGISTER(&msvote_ban_time);
	CVAR_REGISTER(&msvote_pvp_enable);
	CVAR_REGISTER(&msvote_lock_enable);
	CVAR_REGISTER(&ms_chatlog);
	CVAR_REGISTER(&ms_admin_contact);
	CVAR_REGISTER(&ms_ban_to_cfg);
	CVAR_REGISTER(&ms_central_enabled);
	CVAR_REGISTER(&ms_central_addr);
	CVAR_REGISTER(&ms_debug_mem);
	CVAR_REGISTER(&ms_fake_hp);		 //AUG2011_17 Thothie - moving fakehp functions to cvar
	CVAR_REGISTER(&ms_fake_players); //DEC2013_07 Thothie - fake players cvar

#ifdef DEV_BUILD
	CVAR_REGISTER(&ms_devlog);
	CVAR_REGISTER(&ms_allowdev);
#endif
	
	g_log_initialized = true;

	SERVER_COMMAND("exec msstartup.cfg\n");

	return true;
}

void WriteCrashCfg()
{
	char fileName[MAX_PATH], content[128];
	const char* cfgName = CVAR_GET_STRING("sv_crashcfg");

	_snprintf(fileName, MAX_PATH, "%s.cfg", (cfgName && cfgName[0]) ? cfgName : "crashed");
	_snprintf(content, sizeof(content), "map %s", MSGlobals::MapName.c_str());

	FileSystem_WriteTextToFile(fileName, content);
}

//Called from CWorld::Spawn() each map change
void MSWorldSpawn()
{
	//Setup global variables that can't be changed during a game
	MSGlobals::PKAllowed = ms_pklevel.value > 0 ? true : false;
	//Thothie attemptitng to remove FN upload sploit (Thanx to MiB)
	MSGlobals::CentralEnabled = CVAR_GET_FLOAT("ms_central_enabled") > 0.0f ? true : false;
	MSGlobals::DevModeEnabled = ms_dev_mode.value > 0 && !MSGlobals::CentralEnabled ? true : false;
	//return MSGlobals::CentralEnabled && !MSGlobals::IsLanGame && MSGlobals::ServerSideChar;
	//MSGlobals::FXLimit = CVAR_GET_FLOAT("ms_fxlimit");
	MSGlobals::PKAllowedinTown = ms_pklevel.value > 1 ? true : false;
	MSGlobals::IsLanGame = CVAR_GET_FLOAT("sv_lan") ? true : false;
	MSGlobals::CanCreateCharOnMap = false;
	MSGlobals::ServerSideChar = ms_serverchar.value ? true : false;
	MSGlobals::MapName = STRING(gpGlobals->mapname);
	
	//Force the client to use the same client lib as the server. - Solokiller
	//This ensures that clients don't replace their client and send exploit commands.
#if !defined(_DEBUG)
	ENGINE_FORCE_UNMODIFIED(force_exactfile, NULL, NULL, "cl_dlls/client.dll");
	ENGINE_FORCE_UNMODIFIED(force_exactfile, NULL, NULL, "cl_dlls/client.so");
	ENGINE_FORCE_UNMODIFIED(force_exactfile, NULL, NULL, "cl_dlls/client.dylib");
	ENGINE_FORCE_UNMODIFIED(force_exactfile, NULL, NULL, "pak0.pak");
#endif

	HTTPRequest::SetBaseURL(CVAR_GET_STRING("ms_central_addr"));

	//TODO: move to MSGlobalInit so it's not called every map change.
	//g_SteamServerHelper.Init();

	// if (MSGlobals::CentralEnabled)
	// {
	// 	// Initialize FN Request Manager
	// 	g_FNRequestManager.Init();

	// 	bool fail = true;

	// 	for (int retry = 0; retry < 5; retry++)
	// 	{
	// 		if (FNShared::IsValidConnection())
	// 		{
	// 			fail = false;
	// 			g_engfuncs.pfnServerPrint("FuzzNet connected!\n");
	// 			logfile << Logger::LOG_INFO << "FuzzNet connected\n";
	// 			break;
	// 		}
	// 		else if (retry != 5)
	// 		{
	// 			g_engfuncs.pfnServerPrint("FuzzNet connection failed! Retrying...\n");
	// 		}
	// 	}

	// 	if (fail == true)
	// 	{
	// 		g_engfuncs.pfnServerPrint("FuzzNet connection failed. Turning off FN.\n");
	// 		logfile << Logger::LOG_INFO << "FuzzNet connection failed.\n";
	// 		MSGlobals::CentralEnabled = false;
	// 	}
	// }

	if (FNShared::IsEnabled())
	{	
		g_engfuncs.pfnServerPrint("\nInitalize FN Request Manager\n");
		g_FNRequestManager.Init();
		//FNShared::ValidateFN();
	}

	WriteCrashCfg();
}

//Called every frame
void MSGameThink()
{
	g_SteamServerHelper->Think();
	g_FNRequestManager.Think();

	if(!gFNInitialized && FNShared::IsEnabled())
	{
		MSConnectFN();
		gFNInitialized = true;
	}
}

void MSConnectFN()
{
	FNShared::Validate();
}

//Called when the map changes or server is shutdown from ServerDeactivate
//Note that ClientDisconnect is called after this, and the player is deallocated again!
#define WORLD_MAX 6000
void MSGameEnd()
{
	if(MSGlobals::GameScript)
	{
		//Moved here from MSGlobals::EndMap because commands can access entities that are freed below - Solokiller 3/10/2017
		MSGlobals::GameScript->CallScriptEvent( "game_end" );
	}
	
	//Save all characters now - Solokiller 5/10/2017
	for(int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		CBasePlayer *pPlayer = static_cast<CBasePlayer*>(UTIL_PlayerByIndex(i));
		//TODO: make sure player is actually connected and in valid state (i.e. not missing inventory) - Solokiller
		if(pPlayer)
		{
			pPlayer->SaveChar();
			if(!MSGlobals::ServerSideChar) pPlayer->m_TimeCharLastSent = 0;
		}
	}
	
	//Thothie MAR2012_27 - clear duplicate precaches for next map
	gSoundPrecacheList.clearitems();
	gModelPrecacheList.clearitems();
	gModelPrecacheCount = 0;
	gSoundPrecacheCount = 0;
	
	//Deallocate any 'extra' memory the mod allocated for any entity
	edict_t *pEdict = g_engfuncs.pfnPEntityOfEntIndex(0);
	if (pEdict)
		for (int i = 0; i < gpGlobals->maxEntities; i++, pEdict++)
		{
			if (pEdict->free)
				continue;

			CBaseEntity *pEntity = MSInstance(pEdict);
			if (!pEntity)
				continue;

			msstring dbgstr_classname = STRING(pEntity->pev->classname);

			pEntity->Deactivate();
			REMOVE_ENTITY(pEntity->edict());
		}

	//Delete global items
	CGenericItemMgr::DeleteItems();

	//Delete global stores
	CStore::RemoveAllStores();

	//Delete global script commands -- UNDONE: Keep these through level changes
	//CScript::Globals.Deactivate();

	CRaceManager::DeleteAllRaces();

	//Delete gamerules
	if (g_pGameRules)
	{
		delete g_pGameRules;
		g_pGameRules = nullptr;
	}

	//We handle all remaining requests and shutdown.
	g_FNRequestManager.SendAndWait();

	//Thothie - I've not added anything here but there's a game error that generates here
	//MSGameEnd --> Call MSGlobals::EndMap
	MSGlobals::EndMap();

	//Model precache dumpfile
	if (modelout.is_open())
		modelout.close();
	HighestPrecache = -1;
	TotalModelPrecaches = 1;
	CSVGlobals::LogScripts = true;

	g_SteamServerHelper->Shutdown();

	gFNInitialized = false;
	
	//Clear the string pool now, after any references to its strings have been released.
	//Note: any attempts to access allocated strings between now and the next map start will fail and probably cause crashes.
	ClearStringPool();
}

void SendHUDMsgAll(msstring_ref Title, msstring_ref Text)
{
	for (int p = 0; p < gpGlobals->maxClients; p++)
		if (MSInstance(INDEXENT(p + 1)))
			((CBasePlayer *)MSInstance(INDEXENT(p + 1)))->SendHUDMsg(Title, Text);
}

//Server versions of these functions
const char *EngineFunc::GetGameDir()
{
	static char cGameDir[MAX_PATH];
	cGameDir[0] = 0;
	GET_GAME_DIR(cGameDir);
	return cGameDir;
}

extern "C" void LogText(char *szFmt, ...)
{
	va_list argptr;
	static char string[1024];
	va_start(argptr, szFmt);
	vsnprintf(string, sizeof(string), szFmt, argptr);
	va_end(argptr);

	logfile << string;
}

void WRITE_FLOAT(float Float)
{
	byte *pData = (byte *)&Float;
	for (int i = 0; i < sizeof(float); i++)
		WRITE_BYTE(pData[i]);
}

int PRECACHE_SOUND(const char *pszSound)
{
	//Thothie tracking model precaches, avoiding duplicates
	bool bNoLog = false;
	for (int i = 0; i < gSoundPrecacheList.size(); i++)
	{
		msstring msPrecacheSoundName = pszSound;
		if (strcmp(msPrecacheSoundName.c_str(), gSoundPrecacheList[i].PrecacheName.c_str()) == 0)
		{
			//logfile << "(Precache Duplicate Avoided)" << "\n"; //temporary
			//return 0;
			bNoLog = true;
			break;
		}
	}
	if (!bNoLog)
	{
		gSoundPrecacheList.add_blank();
		gSoundPrecacheList[gSoundPrecacheCount].PrecacheName = pszSound;
		gSoundPrecacheCount++;
		logfile << "Precache_Sound(" << gSoundPrecacheCount << "):" << pszSound << "\n";
	}
	return (*g_engfuncs.pfnPrecacheSound)((char *)pszSound);
}

int PRECACHE_MODEL(const char *pszModelname)
{
	//Thothie tracking model precaches, avoiding duplicates
	bool bNoLog = false;
	for (int i = 0; i < gModelPrecacheList.size(); i++)
	{
		msstring msPrecacheModelName = pszModelname;
		if (strcmp(msPrecacheModelName.c_str(), gModelPrecacheList[i].PrecacheName.c_str()) == 0)
		{
			//logfile << "(Precache Duplicate Avoided)" << "\n"; //temporary
			//return 0;
			bNoLog = true;
			break;
		}
	}
	if (!bNoLog)
	{
		gModelPrecacheList.add_blank();
		gModelPrecacheList[gModelPrecacheCount].PrecacheName = pszModelname;
		gModelPrecacheCount++;
		logfile << "Precache_Model(" << gModelPrecacheCount << "):" << pszModelname << "\n";
	}

#ifdef DEV_BUILD
	if (!ms_devlog.value)
		return (*g_engfuncs.pfnPrecacheModel)((char *)pszModelname);

	if (!modelout.is_open())
		modelout.open(msstring(EngineFunc::GetGameDir()) + "/log_models.txt");
#endif

	int LastModel = (*g_engfuncs.pfnPrecacheModel)((char *)pszModelname);

#ifdef DEV_BUILD
	if (LastModel > HighestPrecache)
	{
		if (TotalModelPrecaches == 1)
		{
			modelout << "Brush entities: " << LastModel << endl;
			modelout << "Num\tIndex in Engine" << endl;
		}
		char NumStr[512];
		_snprintf(NumStr, sizeof(NumStr), "%.3i\t#%.3i - %s", TotalModelPrecaches, LastModel, pszModelname);
		modelout << NumStr << endl;
		HighestPrecache = LastModel;
		TotalModelPrecaches++;
	}
#endif

	return LastModel;
}

int ALLOC_STRING(const char *szValue) //Master Sword - Keep track of all string allocations
{
	return MAKE_STRING(g_StringPool.FindOrAllocate(szValue));
}

void ClearStringPool()
{
	g_StringPool.Clear();
}

void CSVGlobals::LogScript(msstring_ref ScriptName, CBaseEntity *pOwner, int includelevel, bool PrecacheOnly, bool Sucess)
{
#ifdef DEV_BUILD
	if (!LogScripts ||
		!ms_devlog.value)
		return;

	int idx = 0;
	if (PrecacheOnly)
		idx = 3;
	else if (!pOwner)
		idx = 0;
	else if (pOwner->IsMSMonster())
		idx = 1;
	else
		idx = 2;

	scriptlistitem_t Item;
	Item.FileName = ScriptName;
	Item.Included = (includelevel > 1) ? true : false;

	ScriptList[idx].add(Item);
#endif
}
void CSVGlobals::WriteScriptLog()
{
#ifdef DEV_BUILD
	if (!ms_devlog.value)
		return;

	LogScripts = false; //Stop logging scripts

	ofstream scriptout;

	scriptout.open(msstring(EngineFunc::GetGameDir()) + "/log_scripts.txt");
	scriptout << "Scripts loaded for " << STRING(gpGlobals->mapname) << endl;

	int Total = 0;
	for (int i = 0; i < SCRIPT_TYPES; i++)
		Total += ScriptList[i].size();

	scriptout << "Total: " << Total << endl;

	for (int i = 0; i < SCRIPT_TYPES; i++)
	{
		scriptout << endl;

		msstring_ref Name = "Global:";
		if (i == 1)
			Name = "Monsters:";
		else if (i == 2)
			Name = "Items:";
		else if (i == 3)
			Name = "Precache only:";

		scriptout << Name << endl;
		scriptout << "------------" << endl;
		scriptout << "Total: " << ScriptList[i].size() << endl;

		for (int s = 0; s < ScriptList[i].size(); s++)
		{
			CSVGlobals::scriptlistitem_t &ScriptListItem = ScriptList[i][s];
			if (ScriptListItem.Included)
				if (ms_devlog.value > 1)
					scriptout << "   ";
				else
					continue; // Admin chose not to log #included scripts

			scriptout << ScriptListItem.FileName.c_str() << endl;
		}

		ScriptList[i].clear(); //save ourselves some memory
	}
#endif
}
