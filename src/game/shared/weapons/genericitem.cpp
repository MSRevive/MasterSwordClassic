/***
*
*	Copyright (c) 2000, Kenneth "Dogg" Early.
*
*	Email kene@maverickdev.com or
*		  l33tdogg@hotmail.com
*
****/
/*
	Generic Item: All Items are defined here
*/
#ifndef VALVE_DLL
#include "hud.h"
#include "cl_util.h"
#include "cl_dll.h"
#include "../common/const.h"
#include "../engine/studio.h"
#include "r_studioint.h"
#include "SDL2/SDL_messagebox.h"
#endif

#include "inc_weapondefs.h"
#include "script.h"
#include "effects/mseffects.h"
#include "groupfile.h"
#include "stats/statdefs.h"
#include "logger.h"

#ifndef VALVE_DLL
void ContainerWindowClose();
void ContainerWindowUpdate();
void ContainerWindowOpen(ulong ContainerID);
extern engine_studio_api_t IEngineStudio;
#include "ms/clglobal.h"
#include "render/clrender.h"
#endif

#include "soundent.h"
#include "shurispritedefs.h"

extern CSoundEnt* pSoundEnt;
extern CSoundEnt* Saved;
extern int SavedName;

//Prevents errors when SUB_Remove() is called from Think()
int g_ItemRemovalStatus;

//Have g_MSScriptInfo default to 'unknown'
globalscriptinfo_t g_MSScriptTypes[] =
{
	{MS_SCRIPT_UKNOWN, "unknown"},
	{MS_SCRIPT_LIBRARY, "sc.dll"},
	{MS_SCRIPT_DIR, "/scripts"},
},
* g_MSScriptInfo = &g_MSScriptTypes[0];

//#define LOG_ITEMHANDLING

#ifdef LOG_ITEMHANDLING
#define logfileopt logfile
#else
#define logfileopt NullFile
#endif

// Global GenericItem retrieve functions
// MiB APR2019_07 - $get_item_table
CGenericItem* GetGenericItemByName(const char* pItemName, bool bCallSpawnIfNeeded = false)
{
	return CGenericItemMgr::GetGlobalGenericItemByName(pItemName, bCallSpawnIfNeeded);
}

CGenericItem* CGenericItemMgr::GetGlobalGenericItemByName(const char* pszItemName, bool bCallSpawnIfNeeded)
{
	for (int i = 0; i < m_Items.size(); i++)
	{
		GenItem_t& GlobalItem = m_Items[i];
		if (FStrEq(GlobalItem.Name.c_str(), pszItemName))
		{
			// MiB MAR2019_03 - [GET_ITEM_TABLE] Call game_spawn if it hasn't been done already
			if (bCallSpawnIfNeeded && !GlobalItem.bCalledSpawn)
			{
				// Only create once, just a param to tell the spawn this
				// is the global item in case there's anything it shouldn't
				// do in the spawn event
				msstringlist vParams;
				vParams.add("1");
				GlobalItem.bCalledSpawn = true;

				// Create a temporary entvars. A lot of script commands expect
				// this to exist, but I don't think it should do anything horrible
				// if it's fake. If it causes any problems, turn off whatever
				// command is causing it by utilizing PARAM1 being 1
				bool bTempPev = GlobalItem.pItem->pev ? false : true;
				if (bTempPev)
					GlobalItem.pItem->pev = new entvars_t();

				try {
					GlobalItem.pItem->CallScriptEvent("game_spawn", &vParams);
				}
				catch (...)
				{
					CGenericItem* pNewItem = NULL;
					try
					{
						pNewItem = NewGenericItem(GlobalItem.pItem);
						::delete (CGenericItem*)GlobalItem.pItem;
						GlobalItem.pItem = pNewItem;
						GlobalItem.pItem->SUB_Remove();
						bTempPev = false;
					}
					catch (...)
					{
#if !TURN_OFF_ALERT
						ErrorPrint("GLBL_ITEM"
							, ERRORPRINT_LOG | ERRORPRINT_CONSOLE | ERRORPRINT_INFOMSG
							, "Error while spawning global item '%s'! %s"
							, pszItemName
							, pNewItem ? "Using a full item instead!"
							: "Creating full item also failed, exiting game"
						);
						if (!pNewItem)
						{
							exit(-1);
						}
#endif
					}
				}

				if (bTempPev)
				{
					delete GlobalItem.pItem->pev;
					GlobalItem.pItem->pev = NULL;
				}
			}

			return GlobalItem.pItem;
		}
	}
	return NULL;
}

//CGenericItem *GetGenericItemByID( int ID ) { return CGenericItemMgr::GetGlobalGenericItemByID( ID ); }
/*CGenericItem *CGenericItemMgr::GetGlobalGenericItemByID( int Type )
	{
		 for (int i = 0; i < m_Items.size(); i++)
		{
			GenItem_t &GlobalItem = m_Items[i];
			if( GlobalItem.pItem->iWeaponType == Type )
				return GlobalItem.pItem;
		}
		return NULL;
	}*/

	//Create a new CGenericItem ent from one of the global templates (GenItem_t)
CGenericItem* NewGenericItem(CGenericItem* pGlobalItem) {
	return CGenericItemMgr::NewGenericItem(pGlobalItem);
}

CGenericItem* CGenericItemMgr::NewGenericItem(CGenericItem* pGlobalItem)
{
	startdbg;

	if (!pGlobalItem)
		return NULL;

	if (pGlobalItem && (pGlobalItem->m_Scripts.size() <= 0))
	{
		ALERT(at_console, "CGenericItemMgr::NewGenericItem: No scripts for %s %s ID %llu!\n", pGlobalItem->ItemName.c_str(), pGlobalItem->m_Name, pGlobalItem->m_iId);
		return NULL;
	}

	dbg("Create Entity for Item");
#ifdef VALVE_DLL
	CGenericItem* pItem = GetClassPtr((CGenericItem*)NULL);
	if ((ulong)pItem == m_LastDestroyedItemID)
	{
		//Work around for a bug.
		//(When new item gets last del'd item memory location (which is used for ID))
		CGenericItem* NewItem = GetClassPtr((CGenericItem*)NULL);
		pItem->SUB_Remove();
		pItem = NewItem;
	}
#else
	CGenericItem* pItem = ::new CGenericItem; //Allocate from engine.  Don't use msnew
	entvars_t* tmpPev = pItem->pev;
	pItem->pev = tmpPev;
	if (pItem)
		pItem->m_pfnThink = NULL;
#endif

	if (!pItem)
		return NULL;

	//Thothie JUN2010_10 - paranoia - this maybe causing char corruption with quest items, not sure
	/*
#ifdef VALVE_DLL
		pItem->pev->origin = Vector(10000,10000,10000); //MIB FEB2010_25 - Shift verification items off map
#endif
		*/

	dbg("Set Item properties");
	//pItem->iWeaponType = pGlobalItem->iWeaponType;
	pItem->ItemName = pGlobalItem->ItemName;
	pItem->m_iId = (int)pItem; //RANDOM_LONG(0,32765);
	strncpy(pItem->m_Name, pGlobalItem->m_Name, sizeof(pItem->m_Name));
	if (pGlobalItem->m_Scripts[0]) //This should ALWAYS be true!
	{
		dbg("Copy script data from global item");
		pItem->m_Scripts.add(msnew CScript);
		pGlobalItem->m_Scripts[0]->CopyAllData(pItem->m_Scripts[0], pItem, pItem);
		pItem->SetScriptVar("IS_NEW_ITEM", "1");
	}

#ifndef VALVE_DLL
	MSCLGlobals::AddEnt(pItem);
#endif

	dbg("Call event game_precache");
	pItem->CallScriptEvent("game_precache");
	dbg("Call CGenericItem::Spawn");
	pItem->Spawn();

#ifndef VALVE_DLL
	logfileopt << "CREATE ITEM: " << pItem->DisplayName() << "\r\n";
#endif

	return pItem;

	enddbg;

	return NULL;
}

//CGenericItem *NewGenericItem( int iD ) { return NewGenericItem( CGenericItemMgr::GetGlobalGenericItemByID(iD) ); }
CGenericItem* NewGenericItem(const char* pszItemName) {
	return NewGenericItem(GetGenericItemByName(pszItemName));
}

void CGenericItemMgr::AddGlobalItem(GenItem_t& NewGlobalItem) {
	m_Items.add(NewGlobalItem);
}

int CGenericItemMgr::ItemCount() {
	return m_Items.size();
}

GenItem_t* CGenericItemMgr::Item(int idx) {
	return &m_Items[idx];
}

// MiB MAR2012_10 - Get item_name's index in the global array
int CGenericItemMgr::LookUpItemIdx(msstring item_name)
{
	for (int i = 0; i < m_Items.size(); i++)
	{
		if (item_name == m_Items[i].Name)
			return i;
	}

	return -1;
}

// MiB MAR2012_10 - Get a new item based on it's index in the global array
CGenericItem* CGenericItemMgr::NewGenericItem(int idx)
{
	return ::NewGenericItem(m_Items[idx].Name.c_str());
}

ulong CGenericItemMgr::m_LastDestroyedItemID = -1;
mslist<GenItem_t> CGenericItemMgr::m_Items;

void CGenericItemMgr::DeleteItem(CGenericItem* pItem)
{
	for (int i = 0; i < ItemCount(); i++)
		if (m_Items[i].pItem == pItem)
			DeleteItem(i);
}

void CGenericItemMgr::DeleteItem(int idx)
{
	GenItem_t& GlobalItem = m_Items[idx];
	GlobalItem.pItem->Deactivate();			   //This Global item isn't a real engine item.. but Deactivate() still deallocates the script data
	::delete (CGenericItem*)GlobalItem.pItem; //Must delete using global function
	m_Items.erase(idx);
}

void CGenericItemMgr::DeleteItems()
{
	startdbg;
	int ItemCount = m_Items.size(); //Save because this will be changing
	for (int i = 0; i < ItemCount; i++)
		DeleteItem(0); //Keep deleting the first item

	m_Items.clear();
	enddbg;
}

//Temporarily create an item to determine it's name
msstring_ref CGenericItemMgr::GetItemDisplayName(msstring_ref ItemName, bool Capital, bool Fullname, int Amt)
{
	CGenericItem* pItem = GetGlobalGenericItemByName(ItemName, true);
	static msstring DisplayName = ItemName;
	if (pItem)
	{
		if (Fullname)
		{
			pItem->iQuantity = Amt;
			DisplayName = SPEECH::ItemName(pItem, Capital);
		}
		else
			DisplayName = pItem->DisplayName();
	}
	return DisplayName.c_str();
}

//GenericItemPrecache
//Cache all items from file into memory
scriptcmdname_list CGenericItemMgr::m_ScriptCommands;
std::map<msstring, msstring> CGenericItemMgr::mItemAlias;

void CGenericItemMgr::GenericItemPrecache(void)
{
	logfile << Logger::LOG_INFO << "Precaching Items...\n";

	ALERT(at_logged, "Precaching Items...\n");

	startdbg;

	dbg("Add Script commands");

	//GenericItem script commands (only add once per game):
	if (!m_ScriptCommands.size())
	{
		m_ScriptCommands.add(scriptcmdname_t("wipespells"));
		m_ScriptCommands.add(scriptcmdname_t("callownerevent"));
		m_ScriptCommands.add(scriptcmdname_t("playanim"));
		m_ScriptCommands.add(scriptcmdname_t("playowneranim"));
		m_ScriptCommands.add(scriptcmdname_t("playviewanim"));
		m_ScriptCommands.add(scriptcmdname_t("setanimext"));
		m_ScriptCommands.add(scriptcmdname_t("sethudsprite"));
		m_ScriptCommands.add(scriptcmdname_t("setholdmodel"));
		//m_ScriptCommands.add( scriptcmdname_t( "setitemskin" ) ); //MAY2008a - failed
		m_ScriptCommands.add(scriptcmdname_t("value"));
		m_ScriptCommands.add(scriptcmdname_t("wearable"));
		m_ScriptCommands.add(scriptcmdname_t("groupable"));
		m_ScriptCommands.add(scriptcmdname_t("useable"));
		m_ScriptCommands.add(scriptcmdname_t("quality"));
		m_ScriptCommands.add(scriptcmdname_t("expiretime"));
		m_ScriptCommands.add(scriptcmdname_t("playermessage"));
		m_ScriptCommands.add(scriptcmdname_t("playermessagecl"));
		m_ScriptCommands.add(scriptcmdname_t("registerattack"));
		m_ScriptCommands.add(scriptcmdname_t("registercontainer"));
		m_ScriptCommands.add(scriptcmdname_t("registerdrink"));
		m_ScriptCommands.add(scriptcmdname_t("registerarmor"));
		m_ScriptCommands.add(scriptcmdname_t("registerprojectile"));
		m_ScriptCommands.add(scriptcmdname_t("registerspell"));
		m_ScriptCommands.add(scriptcmdname_t("cancelattack"));
		m_ScriptCommands.add(scriptcmdname_t("wield"));
		m_ScriptCommands.add(scriptcmdname_t("sethand"));
		m_ScriptCommands.add(scriptcmdname_t("attachlight"));
		m_ScriptCommands.add(scriptcmdname_t("setviewmodel"));
		m_ScriptCommands.add(scriptcmdname_t("playsound"));
		m_ScriptCommands.add(scriptcmdname_t("svpplaysound")); //Thothie MAR2012_27
		m_ScriptCommands.add(scriptcmdname_t("playsoundcl"));
		m_ScriptCommands.add(scriptcmdname_t("playrandomsound"));
		m_ScriptCommands.add(scriptcmdname_t("playrandomsound")); //Thothie MAR2012_27
		m_ScriptCommands.add(scriptcmdname_t("playrandomsoundcl"));
		m_ScriptCommands.add(scriptcmdname_t("setworldmodel"));
		m_ScriptCommands.add(scriptcmdname_t("setpmodel"));
		m_ScriptCommands.add(scriptcmdname_t("setshield"));
		m_ScriptCommands.add(scriptcmdname_t("attachsprite"));
		m_ScriptCommands.add(scriptcmdname_t("learnspell"));
		m_ScriptCommands.add(scriptcmdname_t("solidifyprojectile"));
		m_ScriptCommands.add(scriptcmdname_t("projectiletouch")); //Thothie DEC2012_12 - Enable game_touch for projectiles
		m_ScriptCommands.add(scriptcmdname_t("listcontents"));
		m_ScriptCommands.add(scriptcmdname_t("fall"));
		m_ScriptCommands.add(scriptcmdname_t("setdmg"));
		m_ScriptCommands.add(scriptcmdname_t("setmodelskin"));
		m_ScriptCommands.add(scriptcmdname_t("setviewmodelskin"));
	}

	dbg("Load Script items.txt");

	byte* pMemFile = NULL, * pStringPtr = NULL;
	int iFileSize = 65535, i = 0, n;
	char cString[128], cItemFileName[128];

#ifndef SCRIPT_LOCKDOWN
	//Dev build: Try items.txt in /scripts dir
	pMemFile = pStringPtr = LOAD_FILE_FOR_ME(FILE_DEV_ITEMLIST, &iFileSize);
	if (pMemFile)
		//Set global variable: scripts loaded from directory
		g_MSScriptInfo = &g_MSScriptTypes[MS_SCRIPT_DIR];
	else
	{
#endif

		//If Public build or /scripts/items.txt failed in the dev build, try /dlls/sc.dll
		//CGroupFile &GroupFile = *msnew CGroupFile();
		CGameGroupFile GroupFile;
		GroupFile.Open("dlls/sc.dll");
		ulong FileSize;
		if (GroupFile.ReadEntry(FILE_ITEMLIST, NULL, FileSize))
		{
			pStringPtr = msnew(byte[FileSize + 1]);
			GroupFile.ReadEntry(FILE_ITEMLIST, (byte*)pStringPtr, FileSize);
			pStringPtr[FileSize] = 0;
			//Set global variable: scripts loaded from library
			g_MSScriptInfo = &g_MSScriptTypes[MS_SCRIPT_LIBRARY];
		}
		else
		{

#ifndef SCRIPT_LOCKDOWN
			//Couldn't find items.txt in dev build... jump to end.  No items will be loaded
			ALERT(at_console, "NONEXISTANT file: \"%s\"\nTHIS FILE IS EXTREMELY IMPORTANT. WHY IS IT MISSING?\n", FILE_DEV_ITEMLIST);
#else
			//Fatal error in public build... couldn't find items.txt
			Log("FATAL ERROR: items.txt inside sc.dll NOT FOUND!");

#ifdef RELEASE_LOCKDOWN
			exit(0);
#else
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SC Error", "Missing items.txt!", NULL);
#endif
#endif
			goto end;
		}

#ifndef SCRIPT_LOCKDOWN
	}
#endif

	//Delete old global items
	dbg("Delete old global items");
	CGenericItemMgr::DeleteItems();

	/*#ifdef VALVE_DLL
	ALERT( at_console, "Loading Mastersword items from: %s...\n", g_MSScriptInfo->ContainerName );
#endif*/

	dbg("Load global items");

	//GetString(cString, min(FileSize, sizeof(cString)), (char *)pStringPtr, i, "\r\n")
	while (GetString(cString, V_min(FileSize, sizeof(cString)), (const char*)pStringPtr, i, "\r\n"))
	{
		n = i;
		i += strlen(cString) + 1;
		if (n == i + 1)
			continue; //Found nothing
		if (cString[0] == '/' && cString[1] == '/')
			continue; //comment
		if (!cString[0] || cString[0] == '\r' || cString[0] == '\n')
			continue;

		if (cString == strstr(cString, "#swap "))
		{
			msstring vsLine = msstring(&cString[6]);
			int vSpacePos = vsLine.find(" ");
			msstring vsSrcName = vsLine.substr(0, vSpacePos);
			msstring vsDestName = vsLine.substr(vSpacePos + 1);
			CGenericItemMgr::mItemAlias[vsSrcName] = vsDestName;
			continue;
		}

		_snprintf(cItemFileName, sizeof(cItemFileName), "items/%s", cString);

		logfileopt << "  (Precache) Creating item " << cString << "...";
		//Create a new Global Item

		int iD = 0;
		GenItem_t NewGlobalItem;

		NewGlobalItem.Name = cString;
		CGenericItem* pNewItem = ::msnew CGenericItem;
		NewGlobalItem.pItem = pNewItem;

		CGenericItemMgr::AddGlobalItem(NewGlobalItem);

		//NewItem.iWeaponType = 99 + CGenericItemMgr::ItemCount( ); //First must be 100
		strncpy(pNewItem->m_Name, cString, sizeof(pNewItem->m_Name)); //NewItem.m_Name
		pNewItem->ItemName = cString;

		dbg(msstring("Load script: ") + cItemFileName);
		//Log(cItemFileName);

		bool fSuccess = pNewItem->Script_Add(cItemFileName, pNewItem) ? true : false;
		//Log("try adding new item scripts");
		if (fSuccess)
		{
			pNewItem->SetScriptVar("IS_NEW_ITEM", "0");
			pNewItem->RunScriptEvents(); //Slows down game load, but needed for the precachefile command
		}

		if (!fSuccess)
		{
			//Couldn't load the item's script, don't load the item
#ifdef DEV_BUILD
			CGenericItemMgr::DeleteItem(pNewItem);
			logfileopt << " FAILED\r\n";
#else
#ifdef RELEASE_LOCKDOWN
			exit(0);
#else
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Missing Item Script!", msstring("Item script not found: ") + cItemFileName, NULL);
#endif
#endif
		}
		else
			logfileopt << " SUCCESS\r\n";
	}

	if (pMemFile)
		FREE_FILE(pMemFile);
	else
		//if( g_MSScriptInfo->Containter == MS_SCRIPT_LIBRARY )
		delete[] pStringPtr;

end: //Cleanup time
	logfile << Logger::LOG_INFO << "Done precaching items\n";
	enddbg;
}

//-----------------

extern int iBeam; //Global cuz this can be shared across players
void FindHullIntersection(const Vector& vecSrc, TraceResult& tr, float* mins, float* maxs, edict_t* pEntity);

LINK_ENTITY_TO_CLASS(ms_item, CGenericItem);

void CGenericItem::SUB_Remove()
{
	//MAR2008a - Thothie
	//alert script I'm being removed
	//- waiting until I have an actual use to test with
	//- game_removedfromowner may call along with this
	//CallScriptEvent( "game_removed" );

	//Remove from my owner, if I have one
	RemoveFromOwner();

	//Unlink all the container's items when removed
	Container_RemoveAllItems();

	//ALERT( at_console, "Delete Script: %s\n", STRING(DisplayName) );
	if (g_ItemRemovalStatus == 1)
		g_ItemRemovalStatus = 2;
	//ALERT( at_console, "END Delete Script\n" );

	CGenericItemMgr::m_LastDestroyedItemID = m_iId;

	CBaseEntity::SUB_Remove();
}

CGenericItem::~CGenericItem()
{
	Deactivate();
}

void CGenericItem::Deactivate()
{
	m_Attacks.clear();
	m_WearPositions.clear();
	m_WearModelPositions.clear();

	CurrentAttack = nullptr;

	Container_Deactivate();

	if (DrinkData != nullptr)
	{
		delete DrinkData;
		DrinkData = nullptr;
	}

	if (ArmorData != nullptr)
	{
		delete ArmorData;
		ArmorData = nullptr;
	}

	if (ProjectileData != nullptr)
	{
		delete ProjectileData;
		ProjectileData = nullptr;
	}
#ifdef VALVE_DLL
	Spell_Deactivate();
#endif

	IScripted::Deactivate();
}

void CGenericItem::Spawn()
{
	StoreEntity(this, ENT_ME);
	CurrentAttack = nullptr;
	SetBits(Properties, ITEM_GENERIC);
	m_PrefHand = ANY_HAND;
	ExpireTime = MSITEM_TIME_EXPIRE;

	CallScriptEvent("spawn"); //old
	CallScriptEvent("game_spawn");

	SetBits(lProperties, GI_JUSTSPAWNED);

	if (pev)
	{
		pev->classname = MAKE_STRING("ms_item");
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

CMSMonster* CGenericItem::Owner()
{
	if (m_pParentContainer)
		return m_pParentContainer->Owner();
	return m_pOwner;
}
float CGenericItem::Volume()
{
	//Returns only the size of this item
	return FBitSet(MSProperties(), ITEM_GROUPABLE) ? CBaseEntity::Volume() * iQuantity : CBaseEntity::Volume();
}
float CGenericItem::Weight()
{
	//Returns the size of this item, or if a pack, the weight
	//of all items inside the pack
	float MyVolume = CBaseEntity::m_Weight;
	if (FBitSet(MSProperties(), ITEM_CONTAINER))
		MyVolume += Container_Weight();
	else if FBitSet(MSProperties(), ITEM_GROUPABLE)
		MyVolume *= iQuantity;
	return MyVolume;
}
int CGenericItem::GetItemInfo(ItemInfo* p)
{
	p->pszName = ((pev != NULL) ? STRING(pev->classname) : "INVALID");
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = -1;
	p->iPosition = -1;
	p->iId = m_iId;
	p->iWeight = 0;
	return 0;
}

bool CGenericItem::Deploy()
{
	if (m_Location == ITEMPOS_HANDS)
	{
#ifdef VALVE_DLL
		if (m_pPlayer)
		{
			if (m_AnimExt)
				strncpy(m_pPlayer->m_szAnimTorso, m_AnimExt, 32);
			if (m_AnimExtLegs)
				strncpy(m_pPlayer->m_szAnimLegs, m_AnimExtLegs, 32);
			else
				strncpy(m_pPlayer->m_szAnimLegs, "", sizeof(m_pPlayer->m_szAnimLegs));
		}
#endif

		//SetViewModel( NULL ); //In case the script doesn't set a view model
	}
	UTIL_SetOrigin(pev, m_pOwner->pev->origin); //Move immediately to owner (added so 'clientevent all_in_sight' works from game_deploy)

#ifdef VALVE_DLL
	msstringlist Params;
	Params.clear();

	Params.add(EntToString(this));

	// a1ba: commented out, as m_Hand isn't a string_t anymore but an enum
	//Params.add(STRING(m_Hand));
	m_pOwner->CallScriptEvent("game_equipped", &Params);
#endif

	CallScriptEvent("game_deploy");

#ifdef VALVE_DLL
	//Inform the client that he is holding this item -- UNDONE: crashes
	if (m_pPlayer)
		m_pPlayer->m_ClientCurrentHand = -1;

	/*			MESSAGE_BEGIN( MSG_ONE, gmsgHands, NULL, m_pPlayer->pev );
		WRITE_BYTE( (m_Hand==mCH)?m_Hand+2:m_Hand );
		WRITE_BYTE( iWeaponType );
		WRITE_LONG( m_iId );
		WRITE_BYTE( iGetMethod );
		WRITE_SHORT( iQuantity );
	MESSAGE_END();*/
#else
	if (FBitSet(MSProperties(), ITEM_SPELL))
	{
		Spell_TimeCast = gpGlobals->time;

		if (Spell_CastSuccess)
			CallScriptEvent("game_prepare_success");
		else
			CallScriptEvent("game_prepare_failed");
	}
#endif

	//The client crashes if I send gmsgHands from inside this function
	//For now, I just left it in UpdateClientData.  That means I can't
	//reset iGetMethod here.
	//iGetMethod = METHOD_NONE;

	return true;
}

int CGenericItem::ActivateButton()
{
	if (!m_pPlayer)
		return IN_ATTACK;

	return (m_pPlayer->ActiveItem() == this) ? IN_ATTACK : IN_ATTACK2;
}
void CGenericItem::ActivateButtonDown()
{
	//if( m_pPlayer && FBitSet(m_pPlayer->m_StatusFlags, PLAYER_MOVE_NOATTACK) )
	//	return;

	//CallScriptEvent( "attack1" );//old
	CallScriptEvent("game_attack1_down");
	if (m_pPlayer && FBitSet(m_pPlayer->m_afButtonPressed, ActivateButton())) // First frame of button being held down
	{
		CallScriptEvent("game_attack1");
		if (((CurrentAttack												//This is a click after the first attack click (sometime during a normal attack)
			&& !CurrentAttack->flChargeAmt)							//This attack isn't a charge attack
			|| atoi(EngineFunc::CVAR_GetString("ms_autocharge")) == 1) // MiB MAR2012_05 - Allow auto-charge instead of auto-swing
			&& !m_TimeChargeStart										//Haven't already started charging
			&& GetHighestAttackCharge())								//There is some other attack which uses charging
		{
			m_TimeChargeStart = gpGlobals->time; //If already attacking, then start charge
		}
	}

	//Handle pack stuff
	//if( PackData ) Container_AttackButtonDown( );
}
void CGenericItem::ActivateButtonUp()
{
	//CallScriptEvent("-attack1");//old
	CallScriptEvent("game_-attack1");
	m_LastChargedAmt = Attack_Charge();
	m_TimeChargeStart = 0;

#ifndef VALVE_DLL
	if (!m_ReleaseAttack)
		if (CurrentAttack &&
			!CurrentAttack->fAttackReleased &&
			(CurrentAttack->Type == ATT_CHARGE_THROW_PROJ || CurrentAttack->Type == ATT_STRIKE_HOLD))
		{
			if (gpGlobals->time >= CurrentAttack->tTrueStart + CurrentAttack->tProjMinHold)
			{
				SendCancelAttackCmd();
				CurrentAttack->fAttackReleased = true;
				m_ReleaseAttack = true;
			}
		}
#endif
}
void CGenericItem::Attack2ButtonDown(void)
{
	//if( m_pPlayer && !FBitSet(m_pPlayer->m_StatusFlags, PLAYER_MOVE_CANATTACK) )
	//	return;

	//CallScriptEvent("attack2");//old
	CallScriptEvent("game_+attack2");
	if (m_pPlayer && FBitSet(m_pPlayer->m_afButtonPressed, IN_ATTACK2)) // First frame of button being held down
	{
		CallScriptEvent("game_attack2");
	}
}
void CGenericItem::Attack2ButtonUp()
{
	//CallScriptEvent("-attack2");//old
	CallScriptEvent("game_-attack2");
}

void CGenericItem::FinishAttack()
{	//unused
	/*	switch( iActionType ) {
		case WEAPON_ATTACK_SWIPE1:
			SendWeaponAnim( WEAPON_LIFT1 );
			break;
	}*/
	ResetAttack();
}
void CGenericItem::ResetAttack()
{ //unused
	//Don't set m_flNextPrimaryAttack to 0 here, let it expire by gpGlobals->time passing it\n" );
	IdleTime = 0;
}

//Called on items when they hit something
//For example: Sword hits a shield.  Shield calls countereffect on the sword so the sword can make his owner's screen shake
void CGenericItem::CounterEffect(CBaseEntity* pInflictor, int iEffect, void* pExtraData)
{
#ifdef VALVE_DLL
	switch (iEffect)
	{
	case CE_HITWORLD: //I hit a wall
	{
		//CBaseEntity *pSoundEnt = (CBaseEntity *)pExtraData;
		//if( !pSoundEnt ) break;

		CallScriptEvent("hitwall"); //old
		CallScriptEvent("game_hitworld");

		/*char cSound[128];
				 _snprintf(cSound, sizeof(cSound),  "weapons/cbar_hit%i.wav",  RANDOM_LONG(1, 2) );
				EMIT_SOUND_DYN(pSoundEnt->edict(), CHAN_AUTO, cSound, 1, ATTN_NORM, 0, 80 + RANDOM_LONG(-10,25)); */
		break;
	}
	case CE_HITMONSTER: //I hit an npc
	{
		//CallScriptEvent( "hitmonster" );//old
		CallScriptEvent("game_hitnpc");
		break;
	}
	case CE_SHIELDHIT: //I hit a shield
	{
		bool fWeaponDecay = false;
		float fDamage = *(float*)pExtraData;
		if (pExtraData)
		{
			if (Quality)
			{
				Quality -= int(fDamage * +0.5);
				if (Quality < 0)
					Quality = 0;
				fWeaponDecay = true;
			}
		}

		if (m_pOwner)
		{
			m_pOwner->pev->punchangle = Vector(RANDOM_FLOAT(-3, 3),
				RANDOM_FLOAT(-3, 3), RANDOM_FLOAT(-3, 3));
			//EMIT_SOUND_DYN( m_pOwner->edict(), CHAN_WEAPON, SOUND_IRONSHIELD_HIT1, 1, ATTN_NORM, 0, 198 + RANDOM_LONG(0,50));
			if (m_pPlayer)
			{
				MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_CLDLLFUNC], NULL, m_pPlayer->pev);
				WRITE_BYTE(14);
				WRITE_LONG(m_iId);
				WRITE_SHORT(Quality);
				MESSAGE_END();
				CancelAttack(); //too annoying
			}
			if (fWeaponDecay && !Quality)
			{
				if (m_pPlayer)
					m_pPlayer->SendInfoMsg("Your %s breaks apart!\n", DisplayName());
				m_pOwner->RemoveItem(this);
				SetThink(&CGenericItem::SUB_Remove);
				flNextThink = gpGlobals->time;
			}
		}
	}
	break;
	}
#endif
}

// no fire buttons down
void CGenericItem::AllButtonsReleased(void) {}
bool CGenericItem::ShouldIdle(void)
{
	if (IdleTime < gpGlobals->time &&
		fNextActionTime < gpGlobals->time)
		return true;
	else
		return false;
}
#ifndef VALVE_DLL
void CGenericItem::Idle(void)
{
} //add playing idle animations?
#endif

/*
	GiveTo - Called whenever an item is transferred to a player from the
			 ground, a corpse, or the hand or body of another player.
*/
bool CGenericItem::GiveTo(CMSMonster* pReciever, bool AllowPutInPack, bool fSound, bool fPutItemsAway)
{
	if (m_NotUseable)
		return false;

	CBasePlayer* pPlayer = NULL;
	bool fWentToPack = false;
	if (pReciever && pReciever->IsPlayer())
		pPlayer = (CBasePlayer*)pReciever;

	//I'm an airborne projectile, cancel out
	if (MSProperties() & ITEM_PROJECTILE)
		return false;

	bool fRemovedFromPack = (Owner() == pReciever);

	if (pPlayer)
	{
		char cErrorString[128] = "";
		int Return = pPlayer->NewItemHand(this, !fRemovedFromPack, true, fPutItemsAway, cErrorString);
		if (Return == -2)
		{
			if (!fRemovedFromPack && AllowPutInPack)
			{
				//If hands are full, try to put the item straight into a pack
				if (!pPlayer->PutInAnyPack(this, true))
				{
					if (pPlayer->Gear.size())
						return false;
				} //If there were no packs, show the hands error message below
				else
				{
					fWentToPack = true;
					Return = 100;
				}
			}
		}
		//else if( Return == -4 ) { pPlayer->SwapHands( true ); Return = 100; }

		if (Return < 0)
		{
			if (cErrorString[0])
				pPlayer->SendEventMsg(HUDEVENT_UNABLE, cErrorString);
			return false;
		}
	}

	SetThink(NULL);
	//Only add to hands if we didn't already add the item to a pack
	if (!fWentToPack)
	{
		//If I'm still attached to someone, remove me
		RemoveFromOwner();

		//iGetMethod = METHOD_PICKEDUP;
		if (!pReciever || !pReciever->AddItem(this, true, true))
			return false;
	}

	if (fSound && pReciever && pReciever->pev)
		EMIT_SOUND(ENT(pReciever->pev), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);

	return true;
}
void CGenericItem::AddToOwner(CMSMonster* pMonster)
{
	// Add item from to owner.  m_pOwner gets set here.
	m_pOwner = pMonster;
	m_pPlayer = pMonster->IsPlayer() ? (CBasePlayer*)pMonster : NULL;
	pev->owner = pMonster->edict();
	pev->aiment = pMonster->edict();
	pev->movetype = MOVETYPE_FOLLOW;
	pev->solid = SOLID_NOT;
	SetBits(pev->flags, FL_SKIPLOCALHOST); //Don't send the entity to its owner.  It's all predicted now
	m_pOwner->Gear.AddItem(this);
	StoreEntity(pMonster, ENT_OWNER);
}

void CGenericItem::Wield(void) {}
void CGenericItem::UnWield(void) {}

bool CGenericItem::UseItem(bool Verbose)
{
	startdbg;

	if (!m_pOwner)
		return false;
	if (CurrentAttack)
		return false;

	//"Drop" spells to cancel them when sheathed
	if (FBitSet(MSProperties(), ITEM_SPELL)) {
		this->Drop();
		return true;
	}

	if (Verbose && !CanPutinInventory())
		return false;

	//Try to wear the item
	if (FBitSet(MSProperties(), ITEM_WEARABLE))
		if (WearItem())
			return true;
	//MiB Jul2008a (JAN2010_13) - Changed from below. If a wearable item can't be worn,
	//				 try to put it in a pack.

	/*if( FBitSet(MSProperties(), ITEM_WEARABLE) )
		return WearItem( );*/

	dbg("PutInAnyPack");

	//Try to put the item in a pack
	if (!m_pPlayer)
		return PutInAnyPack(NULL, true);

	enddbg;
	return m_pPlayer->PutInAnyPack(this, true);
}

bool CGenericItem::PutAway(bool Verbose)
{
	if (FBitSet(MSProperties(), ITEM_SPELL))
	{
		if (m_pPlayer)
			return m_pPlayer->DropItem(this, false, false);
		return true;
	}

	UseItem(Verbose);
	return m_pParentContainer || (m_Location != ITEMPOS_HANDS);
}

//Can I put this item away?  (Either Wear it or put it inside a container)
bool CGenericItem::CanPutinInventory()
{
	if (!m_pOwner ||						 //Don't have owner
		CurrentAttack ||					 //Attacking
		FBitSet(MSProperties(), ITEM_SPELL)) //Spell
		return false;

	return CanWearItem() || (m_pPlayer && FindPackForItem(m_pPlayer, false));
}

bool CGenericItem::CanWearItem()
{
	if (!m_pOwner || !FBitSet(MSProperties(), ITEM_WEARABLE))
		return false;

	bool Verbose =
#ifdef VALVE_DLL
		true;
#else
		false;
#endif

	CGenericItem* pItemConflict = NULL;
	if (m_pPlayer)
	{
		for (int iloc = 0; iloc < m_WearPositions.size(); iloc++)
		{
			wearpos_t* pPlayerPos = NULL;
			for (int ploc = 0; ploc < m_pPlayer->m_WearPositions.size(); ploc++)
			{
				msstring_ref PlayerPosName = m_pPlayer->m_WearPositions[ploc].Name;
				if (m_WearPositions[iloc].Name != m_pPlayer->m_WearPositions[ploc].Name)
					continue;

				pPlayerPos = &m_pPlayer->m_WearPositions[ploc];
				break;
			}

			if (!pPlayerPos)
			{
				if (Verbose)
					m_pPlayer->SendInfoMsg("You can't wear %s\n", SPEECH_GetItemName(this));
				return false; //Couldn't find position on player (the position's name isn't defined)
			}

			wearpos_t& PlayerPos = *pPlayerPos;
			CGenericItem* pItemConflict = NULL;

			int iSlots = 0;
			for (int i = 0; i < m_pOwner->Gear.size(); i++)
			{
				CGenericItem* pItemWorn = m_pOwner->Gear[i];

				if (pItemWorn == this ||
					!FBitSet(pItemWorn->MSProperties(), ITEM_WEARABLE) ||
					!pItemWorn->IsWorn())
					continue;

				for (int iwloc = 0; iwloc < pItemWorn->m_WearPositions.size(); iwloc++)
				{
					if (pItemWorn->m_WearPositions[iwloc].Name != PlayerPos.Name)
						continue;

					iSlots += pItemWorn->m_WearPositions[iloc].Slots;
					pItemConflict = pItemWorn;
					break;
				}
			}

			iSlots += m_WearPositions[iloc].Slots;

			if (iSlots > PlayerPos.MaxAmt)
			{
				if (Verbose)
				{
					if (PlayerPos.MaxAmt == 0)
					{
						m_pPlayer->SendInfoMsg("You can't wear %s\n", SPEECH_GetItemName(this));
					}
					else if (PlayerPos.MaxAmt == 1)
					{
						if (pItemConflict)
							m_pPlayer->SendInfoMsg("You have no more %s slots\n", PlayerPos.Name.c_str()); //Thothie DEC2007a - was "You've are already wearing a %s\n", SPEECH_GetItemName(pItemConflict)
						else
							m_pPlayer->SendInfoMsg("You can't wear %s\n", SPEECH_GetItemName(this));
					}
					else
						m_pPlayer->SendInfoMsg("You have no more %s slots\n", PlayerPos.Name.c_str());
				}
				//Thothie DEC2007a - was m_pPlayer->SendInfoMsg( "You are wearing too many %ss\n", DisplayName() )
				return false;
			}
		}
	}

	return true;
}

bool CGenericItem::WearItem(void)
{
	if (!CanWearItem())
		return false;

	//Thothie FEB2010_01 Pass gender/race with wear
	static msstringlist Params;
	Params.clearitems();
	Params.add(m_pOwner->m_Race);
	Params.add((m_pOwner->m_Gender == GENDER_MALE) ? "male" : "female");
	Params.add("CGenericItem::WearItem");

	CallScriptEvent("game_wear", &Params);

	bool IsActiveItem = m_pOwner ? m_pOwner->ActiveItem() == this : false;

	m_Location = ITEMPOS_BODY;

	if (IsActiveItem)
		m_pOwner->SwitchToBestHand();

	return true;
}

/*
	FindPackForItem:
	Find a valid pack for an item, with space for the item.

	pPlayer is a parameter because this can be called even when the
	item doesn't have an owner (like picking up straight to pack)
*/
CGenericItem* CGenericItem::FindPackForItem(CBasePlayer* pPlayer, bool fVerbose)
{
	if (!pPlayer)
		return NULL;

	//Manual put in packs.
	bool fSuccess = false;
	CGenericItem* pPack = NULL;

	if (strstr(ItemName, "arrow"))
		pPack = pPlayer->GetContainer("quiver");
	else if (strstr(ItemName, "swords"))
		pPack = pPlayer->GetContainer("sheath");
	else if (strstr(ItemName, "blunt") ||
		strstr(ItemName, "axes"))
		pPack = pPlayer->GetContainer("holster");
		

	//Check for space, etc in desired pack
	if (pPack && CanPutInPack(pPack))
		fSuccess = true;

	if (!fSuccess)
	{
		//Last resort, try to put in any pack
		for (int i = 0; i < pPlayer->Gear.size(); i++)
		{
			CGenericItem* pNextPack = pPlayer->Gear[i];

			if (FBitSet(pNextPack->MSProperties(), ITEM_CONTAINER) &&
				//pPlayer->PutInPack( this, pNextPack, FALSE ) )
				CanPutInPack(pNextPack))
			{
				pPack = pNextPack;
				fSuccess = true;
				break;
			}
		}
	}

	if (!fSuccess)
		return NULL;

	return pPack;
}
/*
	PutInAnyPack:
	pPlayer is a parameter because this can be called even when the
	item doesn't have an owner (like picking up straight to pack)
*/
bool CGenericItem::PutInAnyPack(CBasePlayer* pPlayer, bool fVerbose)
{
	CGenericItem* pPack = FindPackForItem(pPlayer, fVerbose);

	if (!pPack)
		return false;

#/*ifdef VALVE_DLL
		int LastHand = m_pOwner ? m_Hand : -1;
		CBasePlayer *pOldPlayer = pPlayer; //m_pPlayer is unset in PutInPack()
	#endif*/

	/*#ifdef VALVE_DLL
		if( fSuccess )
		{
			//Sync client, so a server update is not sent
			if( LastHand > -1 )
			{
				pOldPlayer->m_ClientHandID[LastHand] = (pOldPlayer->Hand[LastHand] ? pOldPlayer->Hand[LastHand]->m_iId : 0);
				pOldPlayer->m_ClientCurrentHand = pOldPlayer->iCurrentHand;
			}
			return TRUE;
		}
	#endif*/
	return PutInPack(pPack);
}

bool CGenericItem::CanPutInPack(CGenericItem* pContainer)
{
	if (!pContainer)
		return false;

	if (FBitSet(MSProperties(), ITEM_SPELL))
		return false; //Can't put spells in pack

	return pContainer->Container_CanAcceptItem(this);
}
bool CGenericItem::PutInPack(CGenericItem* pContainer)
{
	if (!CanPutInPack(pContainer))
		return false;

	//SetThink( NULL ); //NOTE: My think function is set to NULL here!
	CancelAttack();

	//AUG2010_28 call BEFORE we actually remove the item
	CallScriptEvent("game_putinpack");

	if (m_pOwner)
	{
		StoreEntity(m_pOwner, ENT_OWNER); //Thothie OCT2016_22 - items not storing owner propers when in pack (not cutting it)
		m_pOwner->RemoveItem(this);
	}
	else
		RemoveFromOwner();

	int ret = pContainer->Container_AddItem(this);
	if (ret == -1)
	{
		SUB_Remove();
		return true;
	}
	//if( m_pPlayer && m_pPlayer->m_fClientInitiated )
	//	pContainer->fClientUpdated = true;

	ClearBits(lProperties, GI_JUSTSPAWNED);
	SetBits(lProperties, GI_INPACK);
	SetBits(pev->effects, EF_NODRAW);

	//CallScriptEvent( "game_putinpack" );

	pev->owner = pContainer->edict();

#ifndef VALVE_DLL
	//ContainerWindowUpdate( );
#endif

	return true;
}
bool CGenericItem::CanHolster()
{
	if (m_Hand == HAND_PLAYERHANDS)
		return true;
	if (CurrentAttack)
		return false;
	return true;
}
bool CGenericItem::CanDrop()
{
#ifdef VALVE_DLL
	if (CurrentAttack)
		return false;

	if (gpGlobals->time < fNextActionTime)
		return false;

	if (m_PrefHand == HAND_PLAYERHANDS)
		return false;

	// if (!Spell_CanAttack())
	// 	return false;
#endif

	return true;
}
void CGenericItem::Holster() {
	m_pOwner = Owner();

	CancelAttack();

	//also reset the items drop tracking if it is holstered
	bDropAttempted = false;
	iDropTickCounter = 0;
}

void CGenericItem::Drop(/*int ParamsFilled, const Vector &Velocity, const Vector &Angles, const Vector &Origin */)
{
	m_pOwner = Owner(); // Oddness happens when trying to drop straight from packs because they get their owner from the container. Explicitly set for now

	bDropAttempted = true;
	if ((bDropAttempted && m_pOwner->IsPlayer()) || !m_pOwner->IsPlayer()) {
		CallScriptEvent("game_drop");

		CancelAttack();

#ifdef VALVE_DLL
		/*if( ParamsFilled > 0 ) pev->velocity = Velocity;
			else if( m_pOwner ) pev->velocity = m_pOwner->pev->velocity;
			if( ParamsFilled > 1 ) pev->angles = Angles;
			else if( m_pOwner ) pev->angles = m_pOwner->pev->angles;
			if( ParamsFilled > 2 ) pev->origin = Origin;
			else if( m_pOwner ) pev->origin = m_pOwner->EyePosition( );*/

		pev->velocity = m_pOwner->pev->velocity;
		pev->angles = m_pOwner->pev->angles;
		pev->origin = m_pOwner->EyePosition() + gpGlobals->v_forward * 10; // Items sometimes get caught in head, move forward a bit

		//	if( m_pPlayer ) strcpy( m_pPlayer->m_szAnimLegs, "" );
		pev->sequence = 0;
		pev->aiment = NULL;
		//pev->avelocity = Vector( 0,10,0 );
		if (WorldModel.len())
		{
			ClearBits(pev->effects, EF_NODRAW); //might need to remove this
			SetBits(pev->effects, EF_NOINTERP);
			SET_MODEL(ENT(pev), WorldModel);
		}
#endif

		if (m_pOwner)
			m_pOwner->RemoveItem((CGenericItem*)this);

#ifdef VALVE_DLL
		FallInit(); //Call FallInit after RemoveItem unsets the think funcrion
#endif
		//CBasePlayerItem::Drop( ParamsFilled, Velocity, Angles, Origin );

#ifndef VALVE_DLL
		//Close the container when dropped (client-side)
		//if( m_pPlayer && m_pPlayer->OpenPack == this ) ContainerWindowClose( );

		void ContainerWindowUpdate();
		SUB_Remove();
#endif
		//reset drop trackers on successful drop
		bDropAttempted = false;
		iDropTickCounter = 0;
	}
	else if (SpellData || !m_pOwner->IsPlayer()) {
		//I'm fairly certain npcs don't use this and the player check is unnecessary but trying to avoid weird stuff.
		//Dropping spells fizzles them
		if (m_pOwner)
			m_pOwner->RemoveItem(this);
		DelayedRemove();
	}
	else if (m_pOwner->IsPlayer()) {
		bDropAttempted = true;
	}

}

void CGenericItem::FallInit()
{
	CBasePlayerItem::FallInit();

	CallScriptEvent("game_fall");

	m_TimeExpire = gpGlobals->time + ExpireTime;
	flNextThink = pev->nextthink;
}

void CGenericItem::FallThink(void)
{
	if (pev->flags & FL_ONGROUND)
	{
		float pitch = 95 + RANDOM_LONG(0, 29);
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "items/weapondrop1.wav", 1, ATTN_NORM, 0, pitch);

		// lie flat
		pev->angles.x = 0;
		pev->angles.z = 0;

		//Materialize
		pev->owner = NULL;
		pev->solid = SOLID_TRIGGER;

		UTIL_SetOrigin(pev, pev->origin); // link into world.
		m_TimeExpire = gpGlobals->time + ExpireTime;
	}
	else
		flNextThink = gpGlobals->time + 0.1;
}

//Whether the owner is currently 'using' this item (trying to attack, drink, hold shield up, etc)
bool CGenericItem::ActivatedByOwner(void)
{
	if (!m_pPlayer)
		return false;

	if (m_pPlayer->ActiveItem() == this)
	{
		if (FBitSet(m_pPlayer->pbs.ButtonsDown, IN_ATTACK))
			return true;
	}
	else if (m_Location == ITEMPOS_HANDS)
	{
		if (FBitSet(m_pPlayer->pbs.ButtonsDown, IN_ATTACK2))
			return true;
	}
	return false;
}

void CGenericItem::ListContents()
{
	startdbg;
	dbg("Begin");

	if (m_pPlayer)
	{
		m_pPlayer->SetConditions(MONSTER_OPENCONTAINER);
		m_pPlayer->BlockButton(IN_ATTACK);
	}

#ifndef VALVE_DLL
	dbg("Call ContainerWindowOpen");
	ContainerWindowOpen(m_iId);
#endif
	enddbg;
}

msstring ItemThinkProgress;
void CGenericItem::Think()
{
	startdbg;
	dbg("Remove marked items");

	if (!Owner() && m_TimeExpire && gpGlobals->time >= m_TimeExpire)
	{
		m_TimeExpire = 0;
		DelayedRemove();
	}

	if (flNextThink && flNextThink <= gpGlobals->time)
	{
		flNextThink = 0;
		g_ItemRemovalStatus = 1;
		CBaseEntity::Think();

		if (g_ItemRemovalStatus == 2)
		{
			//Special case - SUB_Remove was called
			g_ItemRemovalStatus = 0;
			return;
		}
		g_ItemRemovalStatus = 0;
	}

	dbg("ItemPostFrame");
#ifndef VALVE_DLL
	//In the server dll this is called from playerpostthink
	ItemPostFrame();
#endif

	dbg("RunScriptEvents");
	RunScriptEvents();

	dbg("Attack");
	if (Attack_CanAttack())
	{
		StartAttack();
		Attack();
	}

#ifdef VALVE_DLL
	pev->nextthink = gpGlobals->time + 0.1;
	dbg("Fall");
	Fall();

	dbg("Move");
	Move();

	dbg("Spell_Think");
	Spell_Think();
#else
	pev->nextthink = gpGlobals->time;
#endif

	//drop attempt tracking
	/*
	if (bDropAttempted && iDropTickCounter >= 100)
	{
		//if 100 ticks have passed since attempting to drop, cancel drop.
		bDropAttempted = false;
		CBasePlayer* pOwner;
		if (pOwner = dynamic_cast<CBasePlayer*>(Owner())) {
			pOwner->SendEventMsg(HUDEVENT_NORMAL, msstring("Dropping timed out."));
		}
		iDropTickCounter = 0;
	}
	else if (bDropAttempted && iDropTickCounter < 100) {
		iDropTickCounter++;
	}*/

	enddbg;
}
#ifdef VALVE_DLL
//
// Move - For special item behavior
//
void CGenericItem::Move()
{
	Projectile_Move();
}
#endif
//
// RemoveFromOwner - cleanup
// ���������������

void CGenericItem::RemoveFromOwner()
{
	startdbg;

	dbg("Call remove script event");

	CallScriptEvent("game_removefromowner");

	dbg("Call CancelAttack");
	CancelAttack();

	if (pev)
		ClearBits(pev->flags, FL_SKIPLOCALHOST); // Start sending the entity to the owner again

	m_Location = ITEMPOS_HANDS;
	Wielded = FALSE;

	if (m_pPlayer)
		m_pPlayer->m_TimeResetLegs = 0;

	dbg("Call Container_UnListContents");
	Container_UnListContents();

	dbg("Call Wearable_RemoveFromOwner");
	Wearable_RemoveFromOwner();

	dbg("Call RemoveFromContainer");
	RemoveFromContainer();

	dbg("Call Gear.RemoveItem");
	if (m_pOwner)
		m_pOwner->Gear.RemoveItem(this); //Remove me from my owner's packlist

	//Unset pointers
	m_pPlayer = NULL;
	m_pOwner = NULL;

#ifdef VALVE_DLL
	if (pev && pev->iuser2 && pev->iuser3) //Notify torch sprite I've been removed
	{
		CBaseEntity* pSprite = MSInstance(INDEXENT(pev->iuser2));
		if ((int)pSprite == pev->iuser3)
			pSprite->Think();
	}
#endif

	dbg("Call Wearable_ResetClientUpdate");
	Wearable_ResetClientUpdate();

	enddbgprt(msstring("[") + DisplayName() + "]");
}
void CGenericItem::RemoveFromContainer()
{
	if (!m_pParentContainer)
		return;

	m_pParentContainer->Container_RemoveItem(this);
}

void CGenericItem::DelayedRemove()
{
	if (m_pOwner)
		m_pOwner->RemoveItem(this);
	else
		RemoveFromOwner();

	SetThink(&CGenericItem::SUB_Remove);
	flNextThink = gpGlobals->time + 0.1;
}

//
// Fall - fall if an owner wasn't set on the same frame that I spawned
// ����
#ifdef VALVE_DLL
void CGenericItem::Fall()
{
	if (!FBitSet(lProperties, GI_JUSTSPAWNED))
		return;

	ClearBits(lProperties, GI_JUSTSPAWNED);

	if (!Owner())
	{
		//logfile << "[" << STRING(DisplayName) << "] NO OWNER (Fall)\r\n";
		FallInit();
	}
}
bool CGenericItem::IsInAttackStance()
{
	if (m_pPlayer && m_pPlayer->m_TimeResetLegs && gpGlobals->time < m_pPlayer->m_TimeResetLegs)
		return true;
	return false;
}
#endif

//*********************************************************************************
//*********************************************************************************
//****                                                                         ****
//****                             Script Functions                            ****
//****                                                                         ****
//*********************************************************************************
//*********************************************************************************

#define genitemdbg(a) dbg(msstring("[") + DisplayName() + "] " + a)
#define ERROR_MISSING_PARMS MSErrorConsoleText("CGenericItem::ExecuteScriptCmd", UTIL_VarArgs("%s: %s - not enough parameters!\n", Script->m.ScriptFile.c_str(), Cmd.Name().c_str()))

//Register my script commands
void CGenericItem::Script_Setup()
{
	IScripted::Script_Setup();
	m_pScriptCommands = &CGenericItemMgr::m_ScriptCommands;
}

bool CGenericItem::Script_ExecuteCmd(CScript* Script, SCRIPT_EVENT& Event, scriptcmd_t& Cmd, msstringlist& Params)
{
	//Parse one command
	startdbg;

	msstring sTemp;

	msstring DebugString = msstring("Action:");
	for (int i = 0; i < Cmd.m_Params.size(); i++)
	{
		DebugString += " ";
		DebugString += Cmd.m_Params[i];
	}
	genitemdbg(DebugString);

	//****************************** LISTCONTENTS ***************************
	if (Cmd.Name() == "listcontents")
	{
		ListContents();
	}
	//********************************** FALL ******************************
	else if (Cmd.Name() == "fall")
	{
#ifdef VALVE_DLL
		FallInit();
#endif
	}
	//************************** WIPESPELLS by THOTHIE **********************
	else if (Cmd.Name() == "wipespells")
	{
#ifdef VALVE_DLL
		//This bit is the main player
		m_pPlayer->m_SpellList.clear();

		MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_SETPROP], NULL, m_pPlayer->pev);
		WRITE_BYTE(PROP_SPELL);
		WRITE_BYTE(-1); // 1 - change spell, 0 - erase spell, -1 - erase all spells
		MESSAGE_END();
#endif
	}
	//****************************** REGISTERATTACK ************************
	else if (Cmd.Name() == "registerattack")
		RegisterAttack();
	//****************************** REGISTERCONTAINTER ********************
	else if (Cmd.Name() == "registercontainer")
		RegisterContainer();
	//****************************** REGISTERDRINK *************************
	else if (Cmd.Name() == "registerdrink")
		RegisterDrinkable();
	//****************************** REGISTERARMOR *************************
	else if (Cmd.Name() == "registerarmor")
		RegisterArmor();
	//****************************** REGISTERPROJECTILE ********************
	else if (Cmd.Name() == "registerprojectile")
	{
#ifdef VALVE_DLL
		RegisterProjectile();
#endif
	}
	//****************************** REGISTERSPELL ********************
	else if (Cmd.Name() == "registerspell")
	{
#ifdef VALVE_DLL
		RegisterSpell();
#endif
	}
	//****************************** CANCELATTACK **************************
	else if (Cmd.Name() == "cancelattack")
		CancelAttack();
	//****************************** SETSHIELD *****************************
	else if (Cmd.Name() == "setshield")
	{
#ifdef VALVE_DLL
		if (Params.size() >= 1)
		{
			if (Params[0] != "none")
				ActivateShield(msstring("models/") + Params[0]);
		}

		else
			ERROR_MISSING_PARMS;
#endif
	}
	//****************************** ATTACHSPRITE *************************
	else if (Cmd.Name() == "attachsprite")
	{
#ifdef VALVE_DLL
		if (Params.size() >= 4)
		{
			//Param 2 == sprite type. transparent is the only type right now
			CMSSprite* pSprite = GetClassPtr((CMSSprite*)NULL);
			if (pSprite)
			{
				pSprite->TorchInit(msstring("sprites/") + Params[0], atof(Params[2]), atof(Params[3]), edict());
				pev->iuser2 = pSprite->entindex();
				pev->iuser3 = (int)pSprite;
			}
		}
		else
			ERROR_MISSING_PARMS;
#endif
	}
	//****************************** ATTACHLIGHT ********************************
	else if (Cmd.Name() == "attachlight")
	{
#ifdef VALVE_DLL
		CTorchLight::Create(0, edict());
#endif
	}
	//****************************** SETHUDSPRITE ********************************
	//Shuriken FEB2008a, major changes to this script command
	else if (Cmd.Name() == "sethudsprite")
	{
#ifndef VALVE_DLL
		if (Params.size() >= 2)
		{
			if (Params[0] == "hand")
				HandSpriteName = INV_SPRITE;
			else if (Params[0] == "trade")
				TradeSpriteName = INV_SPRITE;
			int SpriteIndex = SpriteIsInArray(Params[1].c_str());
			if (SpriteIndex != -1)
				SpriteFrame = SpriteIndex;
			else
				SpriteFrame = atoi(Params[1]);
		}
		else
			ERROR_MISSING_PARMS;
#endif
	}
	//****************************** CALLOWNEREVENT ******************************
	//Mainly used for events that are handled by a monster scipt (if its a monster)
	//or player code if its a player
	else if (Cmd.Name() == "callownerevent")
	{
		if (Params.size() >= 1)
		{
			if (m_pOwner)
				m_pOwner->CallScriptEvent(Params[0]);
		}
		else
			ERROR_MISSING_PARMS;
	}
	//****************************** VALUE **************************
	else if (Cmd.Name() == "value")
	{
		if (Params.size() >= 1)
			m_Value = atoi(Params[0]);

		else
			ERROR_MISSING_PARMS;
	}
	//****************************** WEARABLE **************************
	else if (Cmd.Name() == "wearable")
	{
		if (Params.size() >= 1)
		{
			if (Params[0] == "0")
			{
				m_WearPositions.clear();
				ClearBits(Properties, ITEM_WEARABLE);
			}
			else if (Params.size() >= 2)
			{
				msstring& WearPos = Params[1];
				SetBits(Properties, ITEM_WEARABLE);
				msstringlist Positions;
				TokenizeString(WearPos, Positions, ";|");

				for (int i1 = 0; i1 < Positions.size(); i1++)
				{
					msstring& PosName = Positions[i1];

					bool Exists = false;
					for (int i2 = 0; i2 < m_WearPositions.size(); i2++)
					{
						if (PosName != m_WearPositions[i2].Name)
							continue;

						Exists = true;
						break;
					}

					if (!Exists)
						m_WearPositions.add(wearpos_t(PosName, atoi(Params[0])));
				}
			}
		}
		else
			ERROR_MISSING_PARMS;
	}
	//****************************** GROUPABLE **************************
	else if (Cmd.Name() == "groupable")
	{
		if (Params.size() >= 1)
		{
			iMaxGroupable = atoi(Params[0]);
			if (iMaxGroupable)
			{
				SetBits(Properties, ITEM_GROUPABLE);
				if (iQuantity <= 0)
					iQuantity = 1; //FEB2010_13 MIB Stackable Stacks
				m_MaxGroupable = iMaxGroupable;
			}
			else
				ClearBits(Properties, ITEM_GROUPABLE);
		}
		else
			ERROR_MISSING_PARMS;
	}
	//****************************** USEABLE **************************
	else if (Cmd.Name() == "useable")
	{
		if (Params.size() >= 1)
			m_NotUseable = atoi(Params[0]) ? false : true;

		else
			ERROR_MISSING_PARMS;
	}
	//****************************** QUALITY **************************
	else if (Cmd.Name() == "quality")
	{
		if (Params.size() >= 1)
		{
			Quality = MaxQuality = atoi(Params[0]);
			SetBits(Properties, ITEM_PERISHABLE);
		}
		else
			ERROR_MISSING_PARMS;
	}
	//****************************** EXPIRETIME ******************************
	//Lets you set how long until this item expires
	else if (Cmd.Name() == "expiretime")
	{
		if (Params.size() >= 1)
		{
			ExpireTime = atof(Params[0]);
			if (!m_pOwner)
				m_TimeExpire = gpGlobals->time + ExpireTime;
		}
		else
			ERROR_MISSING_PARMS;
	}
	//****************************** PLAYERMESSAGE *********************
	else if (Cmd.Name() == "playermessage" || Cmd.Name() == "playermessagecl")
	{
		if (Params.size() >= 1)
		{
			for (int i = 0; i < Params.size(); i++)
			{
				if (i)
					sTemp += " ";
				sTemp += Params[i];
			}
			if (m_pPlayer &&
#ifdef VALVE_DLL
				Cmd.Name() == "playermessage"
#else
				Cmd.Name() == "playermessagecl"
#endif
				)
				m_pPlayer->SendInfoMsg("%s", sTemp.c_str());
		}
		else
			ERROR_MISSING_PARMS;
	}
	//****************************** SETANIMEXT **************************
	else if (Cmd.Name() == "setanimext")
	{
		if (Params.size() >= 1)
		{
			m_AnimExt = Params[0];
			if (Params.size() > 1)
				m_AnimExtLegs = Params[1];
		}
		else
			ERROR_MISSING_PARMS;
	}
	//***************************** SETVIEWMODEL *************************
	else if (Cmd.Name() == "setviewmodel")
	{
#ifndef VALVE_DLL
		if (Params.size() >= 1)
		{
			if (Params[0] == "none")
				m_ViewModel = "";
			else
			{
				sTemp = "models/";
				sTemp += Params[0];
				m_ViewModel = sTemp;
				//SetViewModel( sTemp );
			}
		}
		else
			ERROR_MISSING_PARMS;
#else
		//Thothie JAN2020_04 - need method to pull viewmodel server side
		if (Params.size() >= 1) SetScriptVar("ITEM_VIEWMODEL", Params[0].c_str());
#endif
	}
	//***************************** SETWORLDMODEL *************************
	else if (Cmd.Name() == "setworldmodel")
	{
		if (Params.size() >= 1)
		{
			if (Params[0] == "none")
				WorldModel = "";
			else
				WorldModel = msstring("models/") + Params[0];
		}
		else
			ERROR_MISSING_PARMS;
	}
	//***************************** SETHOLDMODEL **************************
	else if (Cmd.Name() == "setholdmodel")
	{
#ifdef VALVE_DLL
		if (Params.size() >= 1)
		{
			if (m_pOwner)
			{
				if (m_pOwner->IsPlayer() && (m_PlayerHoldModel ? true : false))
					SET_MODEL(edict(), m_PlayerHoldModel);

				else
					pev->modelindex = 0;

				//This is for monsters
				m_pOwner->SetScriptVar("game.monster.wielded_item", m_PlayerHoldModel.c_str());
				m_pOwner->CallScriptEvent("game_wielditem");
			}
		}
		else
			ERROR_MISSING_PARMS;
#endif
	}
	//***************************** SETITEMSKIN **************************
	//setitemskin <index> MAY2008a
	//- setprop does not affect pmodel, trying to do so through here
	//fail - no more effective than setprop
	/*else if( Cmd.Name() == "setitemskin" ) {
		#ifdef VALVE_DLL
			if( Params.size( ) >= 1 )
				pev->skin = atoi(Params[0]);
			else ERROR_MISSING_PARMS;
		#endif
	}*/
	//***************************** SETPMODEL **************************
	else if (Cmd.Name() == "setpmodel")
	{
#ifdef VALVE_DLL
		if (Params.size() >= 1)
			m_PlayerHoldModel = msstring("models/") + Params[0];

		else
			ERROR_MISSING_PARMS;
#endif
	}
	//********************* PLAYVIEWANIM **************************
	else if (Cmd.Name() == "playviewanim")
	{
#ifndef VALVE_DLL
		if (Params.size() >= 1)
		{
			if (m_pPlayer && m_Location == ITEMPOS_HANDS)
			{
				int iAnim = atoi(Params[0]);
				if (m_pPlayer->pev->weaponanim == iAnim)
				{
					//cl_entity_t *view = gEngfuncs.GetViewModel();
					//view->curstate.frame = 0;
				}
				else
					m_pPlayer->pev->weaponanim = iAnim;
				//Print( "PLAY VIEW ANIM: %i!!\n", m_pPlayer->pev->weaponanim );
				//SendViewAnim( m_pPlayer, m_pPlayer->pev->weaponanim );
				m_ViewModelAnim = iAnim;
			}
		}
		else
			ERROR_MISSING_PARMS;
#endif
	}
	//***************************** PLAYANIM ***************************
	else if (Cmd.Name() == "playanim")
	{
		if (Params.size() >= 1)
		{
			pev->sequence = LookupSequence(Params[0]);
			if (pev->sequence < 0)
			{
				pev->sequence = 0;
				MSErrorConsoleText("CGenercItem::Script_ExecuteCmd()", UTIL_VarArgs("Script: %s:\n  %s - Sequence %s NOT FOUND!\n", Script->m.ScriptFile.c_str(), Cmd.Name().c_str(), Params[0].c_str()));
			}

			pev->frame = 0;
			ResetSequenceInfo();
		}
		else
			ERROR_MISSING_PARMS;
	}
	//***************************** PLAYOWNERANIM ***************************
	else if (Cmd.Name() == "playowneranim")
	{
		//Parameters
		//<AnimType>  <AnimName> [LegAnimName] [TimeHoldLegAnim]
		if (Params.size() >= 1)
		{
#ifdef VALVE_DLL
			if (m_pOwner)
			{
				Cmd.m_Params[0] = "playanim";
				if (m_pOwner->m_Scripts[0])
					m_pOwner->m_Scripts[0]->Script_ExecuteCmd(Event, Cmd, Params);
				Cmd.m_Params[0] = "playowneranim";
			}
#endif
		}
		else
			ERROR_MISSING_PARMS;
	}
	//***************************** PLAYSOUND ***************************
	else if (Cmd.Name() == "svplaysound" || Cmd.Name() == "svplayrandomsound" || Cmd.Name() == "playsound" || Cmd.Name() == "playsoundcl" || Cmd.Name() == "playrandomsound" || Cmd.Name() == "playrandomsoundcl")
	{
#ifdef VALVE_DLL
		if (Cmd.Name() == "svplaysound" || Cmd.Name() == "svplayrandomsound" || Cmd.Name() == "playsound" || Cmd.Name() == "playrandomsound")
#else
		if (Cmd.Name() == "playsoundcl" || Cmd.Name() == "playrandomsoundcl")
#endif
		{
			//If I have an owner, use his playsound.  If not, use mine
			if (m_pOwner && m_pOwner->m_Scripts[0])
				m_pOwner->m_Scripts[0]->Script_ExecuteCmd(Event, Cmd, Params);
			else
				return false; //Fallback to the CScript version of playsound/playrandomsound
		}
	}
	//***************************** WIELD ***************************
	else if (Cmd.Name() == "wield")
	{
		if (Params.size() >= 1)
		{
			if (m_pPlayer)
			{
				byte ReqHands = atoi(Params[0]), UsedHands = 0;
				for (int i = 0; i < MAX_PLAYER_HANDS; i++)
					if (m_pPlayer->Hand(i) && m_pPlayer->Hand(i) != this)
						UsedHands++;

				if (MAX_PLAYER_HANDS - (UsedHands + ReqHands) >= 0)
					Wielded = TRUE;
				else
					Wielded = FALSE;
			}
		}
		else
			ERROR_MISSING_PARMS;
	}
	//***************************** SETHAND ***************************
	else if (Cmd.Name() == "sethand")
	{
		if (Params.size() >= 1)
		{
			msstring& ReqHand = Params[0];
			if (!_stricmp(ReqHand, "left"))
				m_PrefHand = LEFT_HAND;
			else if (!_stricmp(ReqHand, "right"))
				m_PrefHand = RIGHT_HAND;
			else if (!_stricmp(ReqHand, "any"))
				m_PrefHand = ANY_HAND;
			else if (!_stricmp(ReqHand, "both"))
				m_PrefHand = BOTH_HANDS;
			else if (!_stricmp(ReqHand, "undroppable"))
				m_PrefHand = HAND_PLAYERHANDS;
		}
		else
			ERROR_MISSING_PARMS;
	}
	//***************************** LEARNSPELL ***************************
	else if (Cmd.Name() == "learnspell")
	{
#ifdef VALVE_DLL
		if (Params.size() >= 1)
		{
			if (m_pPlayer)
			{
				if (Spell_LearnSpell(Params[0])) //Cause owner to attempt to learn this spell
				{
					CallScriptEvent("game_learnspell_success");
					if (m_pOwner)
						m_pOwner->RemoveItem(this);
					SetThink(&CGenericItem::SUB_Remove);
					flNextThink = gpGlobals->time + 0.1;
				}
				else
					CallScriptEvent("game_learnspell_failed");
			}
		}
		else
			ERROR_MISSING_PARMS;
#endif
	}
	//***************************** SOLIDIFYPROJECTILE ***************************
	else if (Cmd.Name() == "solidifyprojectile")
	{
#ifdef VALVE_DLL
		Projectile_Solidify();
#endif
	}
	//***************************** PROJECTILETOUCH ***************************
	//Thothie DEC2012_12 - Enable game_touch for projectiles
#ifdef VALVE_DLL
	else if (Cmd.Name() == "projectiletouch")
	{
		Projectile_TouchEnable(atoi(Params[0]) ? true : false);
	}
#endif
	//****************************** SETDMG ******************************
	else if (Cmd.Name() == "setdmg")
	{
		if (Params.size() >= 2)
		{
			if (m_CurrentDamage)
			{
				msstring& DmgProp = Params[0];
				msstring& DmgValue = Params[1];
				if (DmgProp == "dmg")
					m_CurrentDamage->flDamage = atof(DmgValue);
				else if (DmgProp == "type")
					m_CurrentDamage->sDamageType = DmgValue;
				else if (DmgProp == "hit")
					m_CurrentDamage->AttackHit = atoi(DmgValue) ? true : false;
			}
		}
		else
			ERROR_MISSING_PARMS;
	}

#ifndef VALVE_DLL

	//This is handled here for the client-side entities.  This returns false and handles it again in CScript for the server side
	//******************************* SETMODEL ****************************
#define m_ClEntNormal m_ClEntity[ITEMENT_NORMAL]
	else if (Cmd.Name() == "setmodel")
	{
		if (Params.size() >= 1)
		{
			if (Params[0] == "none")
				m_ClEntNormal.model = NULL;
			else
			{
				sTemp = "models/";
				sTemp += Params[0];
				m_ClEntNormal.model = IEngineStudio.Mod_ForName(sTemp, 0);
			}
		}
		return false;
	}
	//****************************** SETMODELBODY ************************
	else if (Cmd.Name() == "setmodelbody")
	{
		if (Params.size() >= 2)
			if (m_ClEntNormal.model)
			{
				int iGroup = atoi(Params[0]);
				int iValue = atoi(Params[1]);
				m_ClEntNormal.SetBody(iGroup, iValue);
			}
		return false;
	}

	//****************************** SETMODELSKIN ************************
	else if (Cmd.Name() == "setmodelskin")
	{
		if (Params.size() >= 1)
			m_ClEntNormal.curstate.skin = atoi(Params[0]);
		return false;
	}
	else if (Cmd.Name() == "setviewmodelskin")
	{
		if (Params.size() >= 1)
		{
			cl_entity_t* view = gEngfuncs.GetViewModel();
			view->curstate.skin = atoi(Params[0]);
		}
		return false;
	}
#endif
	else
		return false;

	enddbg;

	return true;
}

CGenericItem* FindParryWeapon(CMSMonster* pMonster, /*out*/ int& iPlayerHand, /*out*/ int& iAttackNum)
{
	CGenericItem* pHandItem;

	//Look at current hand first, then other hand
	int iHand[MAX_PLAYER_HANDS];
	iHand[0] = pMonster->m_CurrentHand;
	iHand[1] = !iHand[0];

	for (int i = 0; i < MAX_PLAYER_HANDS; i++)
	{
		int CheckHand = iHand[i];
		pHandItem = pMonster->Hand(CheckHand);
		if (!pHandItem)
			continue;

		for (int a = 0; a < pHandItem->m_Attacks.size(); a++)
		{
			attackdata_t& Attack = pHandItem->m_Attacks[a];
			if (Attack.StatExp == SKILL_PARRY)
			{
				iAttackNum = a;
				iPlayerHand = iHand[i];
				return pHandItem;
			}
		}
	}

	return NULL;
}

//Moved here because it's shared between client and server
bool CGenericItem::Spell_CanAttack()
{
	//Can I attack with this spell right now?
	//If this item is not a spell then return yes
	if (!FBitSet(MSProperties(), ITEM_SPELL))
		return true;

	if (gpGlobals->time < Spell_TimeCast + Spell_TimePrepare)
		return false;

	return true;
}

#ifdef VALVE_DLL
float CGenericItem::Give(enum givetype_e Type, float Amt)
{
	if (m_pOwner)
		return m_pOwner->Give(Type, Amt); //Pass it to my owner
	return CBaseEntity::Give(Type, Amt);
}

CGenericItem* MSUtil_GetItemByID(ulong m_iId)
{
	CGenericItem* pItem = (CGenericItem*)UTIL_FindEntityByClassname(NULL, "ms_item");
	while (pItem)
	{
		if (pItem->m_iId == m_iId)
			return pItem;
		pItem = (CGenericItem*)UTIL_FindEntityByClassname(pItem, "ms_item");
	}
	return NULL;
}
void SendGenericItem(CBasePlayer* pPlayer, CGenericItem* pItem, bool fNewMessage)
{
	//genericitem_full_t newItem = const_cast<genericitem_full_t>(*pItem);
	//SendGenericItem(pPlayer, pItem, fNewMessage);

	// {
	genericitem_full_t Item(pItem);
	if (fNewMessage)
	{
		MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_ITEM], NULL, pPlayer->pev);
		WRITE_BYTE(0);
	}
	WRITE_LONG(Item.ID);
	//WRITE_STRING( Item.Name );
	int iList = -1;
	WRITE_LONG(CGenericItemMgr::LookUpItemIdx(Item.Name)); // MiB MAR2012_10 - Item index rather than script name
	WRITE_SHORT(Item.Location);
	WRITE_BYTE(Item.Hand);
	WRITE_SHORT(Item.Properties);
	if (FBitSet(Item.Properties, ITEM_GROUPABLE))
	{
		WRITE_SHORT(Item.Quantity);
	}
	if (FBitSet(Item.Properties, ITEM_DRINKABLE))
	{
		WRITE_BYTE(Item.Quality);
	}
	if (FBitSet(Item.Properties, ITEM_PERISHABLE))
	{
		WRITE_SHORT(Item.Quality);
		WRITE_SHORT(Item.MaxQuality);
	}
	if (FBitSet(Item.Properties, ITEM_SPELL))
	{
		WRITE_COORD(Item.Spell_TimePrepare);
		WRITE_BYTE(Item.Spell_CastSuccess);
	}
	if (fNewMessage)
		MESSAGE_END();
}

//MIB MAR2012 anti-overlow (see previous archives for original)
// void SendGenericItem(CBasePlayer* pPlayer, genericitem_full_t Item, bool fNewMessage)
// {
// 	if (fNewMessage)
// 	{
// 		MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_ITEM], NULL, pPlayer->pev);
// 		WRITE_BYTE(0);
// 	}
// 	WRITE_LONG(Item.ID);
// 	//WRITE_STRING( Item.Name );
// 	int iList = -1;
// 	WRITE_LONG(CGenericItemMgr::LookUpItemIdx(Item.Name)); // MiB MAR2012_10 - Item index rather than script name
// 	WRITE_SHORT(Item.Location);
// 	WRITE_BYTE(Item.Hand);
// 	WRITE_SHORT(Item.Properties);
// 	if (FBitSet(Item.Properties, ITEM_GROUPABLE))
// 	{
// 		WRITE_SHORT(Item.Quantity);
// 	}
// 	if (FBitSet(Item.Properties, ITEM_DRINKABLE))
// 	{
// 		WRITE_BYTE(Item.Quality);
// 	}
// 	if (FBitSet(Item.Properties, ITEM_PERISHABLE))
// 	{
// 		WRITE_SHORT(Item.Quality);
// 		WRITE_SHORT(Item.MaxQuality);
// 	}
// 	if (FBitSet(Item.Properties, ITEM_SPELL))
// 	{
// 		WRITE_COORD(Item.Spell_TimePrepare);
// 		WRITE_BYTE(Item.Spell_CastSuccess);
// 	}
// 	if (fNewMessage)
// 		MESSAGE_END();
// }
#endif

CGenericItem* MSUtil_GetItemByID(ulong m_iId, CMSMonster* pOwner)
{
	CGenericItem* pItem = MSUtil_GetItemByID(m_iId);
	if (!pItem || pItem->Owner() != pOwner)
		return NULL;

	return pItem;
}

#ifndef VALVE_DLL
#include "../parsemsg.h"
CGenericItem* MSUtil_GetItemByID(ulong lID)
{
	for (int e = 0; e < MSCLGlobals::m_ClEntites.size(); e++)
	{
		CBaseEntity* pEntity = MSCLGlobals::m_ClEntites[e];
		if (!FBitSet(pEntity->MSProperties(), ITEM_GENERIC))
			continue;

		if (((CGenericItem*)pEntity)->m_iId == lID)
			return (CGenericItem*)pEntity;
	}

	return NULL;
}
CGenericItem* ReadGenericItem(bool fAllowCreateNew)
{
	ulong lID = READ_LONG();
	//msstring ItemScript = READ_STRING( );
	int idx = READ_LONG(); // MiB MAR2012_10 - Using global index array instead of script name
	//msstring ItemScript = CGenericItemMgr::NewGenericItem( idx );

	CGenericItem* pItem = MSUtil_GetItemByID(lID);
	if (!pItem && fAllowCreateNew)
		pItem = CGenericItemMgr::NewGenericItem(idx); //ItemScript ); // MiB - See above
	if (!pItem)
	{
		MSErrorConsoleText("ReadGenericItem", msstring("Item doesn't exist and couldn't be created  ID: ") + (int)lID + " Script: " + CGenericItemMgr::Item(idx)->Name.c_str());
		return NULL;
	}

	pItem->m_iId = lID;
	pItem->m_Location = READ_SHORT();
	pItem->m_Hand = READ_BYTE();
	pItem->Properties = READ_SHORT();
	if (FBitSet(pItem->MSProperties(), ITEM_GROUPABLE))
	{
		pItem->iQuantity = READ_SHORT();
	}
	if (FBitSet(pItem->MSProperties(), ITEM_DRINKABLE))
	{
		pItem->Quality = READ_BYTE();
	}
	if (FBitSet(pItem->MSProperties(), ITEM_PERISHABLE))
	{
		pItem->Quality = READ_SHORT();
		pItem->MaxQuality = READ_SHORT();
	}
	if (FBitSet(pItem->MSProperties(), ITEM_SPELL))
	{
		pItem->Spell_TimePrepare = READ_COORD();
		pItem->Spell_CastSuccess = READ_BYTE() ? true : false;
	}
	/*logfile << "Server Caused Creation of New Item: "
		  << MSString(pItem->DisplayName) << " (" << pItem->m_iId
		 << ")\r\n";*/
	return pItem;
}
#endif

//splayviewanim
#ifndef VALVE_DLL
int __MsgFunc_Anim(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	int iAnim = READ_BYTE();
	int iHand = READ_BYTE();

	if (!player.pev->weaponanim == iAnim)
		player.pev->weaponanim = iAnim;

	CGenericItem* pItem = player.Hand(iHand);
	pItem->m_ViewModelAnim = iAnim;

	return 1;
}
#endif

//MiB FEB2008a +special
#ifndef VALVE_DLL
void __CmdFunc_SpecialDown()
{
	/*
	CGenericItem *pItem = player.ActiveItem();
	if( pItem && !pItem->Attack_IsCharging() )
	{
		pItem->m_TimeChargeStart = gpGlobals->time;
	}
	*/
}

void __CmdFunc_SpecialUp()
{
	/*
	 CGenericItem *pItem = player.ActiveItem();
	if( pItem && pItem->Attack_IsCharging() )
	{
		pItem->m_TimeChargeStart = 0;
	}
	*/
}
#endif
