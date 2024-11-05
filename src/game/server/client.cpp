/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
// Robin, 4-22-98: Moved set_suicide_frame() here from player.cpp to allow us to
//				   have one without a hardcoded player.mdl in tf_client.cpp

/*

===== client.cpp ========================================================

  client/server game specific stuff

*/

// Master Sword
#include "inc_weapondefs.h"
#include "stats/stats.h"
#include "stats/statdefs.h"
// ---

#include "saverestore.h"
#include "spectator.h"
#include "client.h"
#include "soundent.h"
#include "customentity.h"
#include "usercmd.h"
#include "netadr.h"
#include "com_model.h"

#include "logger.h"
#include "svglobals.h"
#include "mscharacter.h"
#include "global.h"
#include "pm_shared.h" // PM_GetHullBounds

extern void PlayerPrecache();

// Temp
#define MAX_MONSTERS 5
int g_SummonedMonsters = 0;

extern DLL_GLOBAL unsigned long g_ulModelIndexPlayer;
extern DLL_GLOBAL BOOL g_fGameOver;
extern DLL_GLOBAL int g_iSkillLevel;
extern DLL_GLOBAL unsigned long g_ulFrameCount;

extern int giPrecacheGrunt;
extern int gmsgSayText;

void LinkUserMessages(void);
/*
 * used by kill command and disconnect command
 * ROBIN: Moved here from player.cpp, to allow multiple player models
 */
void set_suicide_frame(entvars_t *pev)
{
	startdbg;
	ALERT(at_console, "SUICIDE FRAME\n");
	if (!pev->model)
		return; // allready gibbed

	//	pev->frame		= $deatha11;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_TOSS;
	pev->deadflag = DEAD_DEAD;
	pev->nextthink = -1;
	enddbg;
}

clientaddr_t g_NewClients[32];

/*
===========
ClientConnect

called when a player connects to a server
============
*/
BOOL ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128])
{
	DBG_INPUT;
	bool fSuccess = false;
	startdbg;

	logfile << Logger::LOG_INFO << "[ClientConnect]  ";
	if (g_pGameRules)
		fSuccess = g_pGameRules->ClientConnected(pEntity, pszName, pszAddress, szRejectReason) ? true : false;

	if (fSuccess)
	{
		// this has to be here otherwise on reconnect players can't select characters
		int iPlayerOfs = ENTINDEX(pEntity) - 1;
		clientaddr_t &ClientInfo = g_NewClients[iPlayerOfs];
		ClientInfo.pe = pEntity;
		ClientInfo.TimeClientConnected = gpGlobals->time;
		strncpy(ClientInfo.Addr, pszAddress, sizeof(ClientInfo.Addr) );
		ClientInfo.fDisplayedGreeting = false;
		pEntity->free = false;
		logfile << "Client Queue: [" << iPlayerOfs << "] " << pszAddress << "\n";
	}
	else
		logfile << "Client rejected: " << szRejectReason << "\n";

	logfile << Logger::LOG_INFO << "[ClientConnect: Complete]\n";

	enddbg;
	return fSuccess ? 1 : 0;
}

/*
===========
ClientDisconnect

called when a player disconnects from a server

GLOBALS ASSUMED SET:  g_fGameOver
============
*/
//int CountPlayers( void );

void ClientDisconnect(edict_t *pEntity)
{
	DBG_INPUT;
	startdbg;

	dbg("Begin");
	if (g_fGameOver)
		return;

	dbg("Call pSound->Reset");

	/*CSound *pSound;
	pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( pEntity ) );
	// since this client isn't around to think anymore, reset their sound.
	if ( pSound )
		pSound->Reset();*/

	dbg("Call g_pGameRules->ClientDisconnected");

	//When the server is shutdown, this ClientDisconnect is called after gamerules has been deleted
	if (g_pGameRules)
		g_pGameRules->ClientDisconnected(pEntity);

	enddbg;
}

// called by ClientKill and DeadThink
void respawn(entvars_t *pev, BOOL fCopyCorpse)
{
	// respawn player
	GetClassPtr((CBasePlayer *)pev)->Spawn();
}

/*
============
ClientKill

Player entered the suicide command

GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
============
*/
void ClientKill(edict_t *pEntity)
{
	startdbg;
	DBG_INPUT;
	entvars_t *pev = &pEntity->v;

	CBasePlayer *pPlayer = (CBasePlayer *)CBasePlayer::Instance(pev);

	if (!pPlayer->IsElite() && pPlayer->m_fNextSuicideTime > gpGlobals->time)
		return; // prevent suiciding too often

	if (!pPlayer->IsElite())
		pPlayer->SendInfoMsg("You will die in 5 seconds...\n");
	pPlayer->m_TimeTillSuicide = gpGlobals->time + (pPlayer->IsElite() ? 0.1f : 5.0f);
	pPlayer->m_fNextSuicideTime = pPlayer->m_TimeTillSuicide + 5;
	enddbg;
}

/*
===========
ClientPutInServer

called each time a player is spawned
============
*/
void ClientPutInServer(edict_t *pEntity)
{
	startdbg;
	DBG_INPUT;

	logfile << Logger::LOG_INFO << "[ClientPutInServer]\n";
	CBasePlayer *pPlayer;

	entvars_t *pev = &pEntity->v;

	pPlayer = GetClassPtr((CBasePlayer *)pev);
	pPlayer->SetCustomDecalFrames(-1); // Assume none;
	
	if (!pPlayer->m_ClientAddress[0]) //Just joined the server, get address
	{
		int iPlayerOfs = ENTINDEX(pEntity) - 1;
		logfile << Logger::LOG_INFO << "Client Address " << g_NewClients[iPlayerOfs].Addr << "... Slot [" << iPlayerOfs << "]\n";

		strncpy(pPlayer->m_ClientAddress, g_NewClients[iPlayerOfs].Addr, sizeof(pPlayer->m_ClientAddress));
	}else{
		MSErrorConsoleText("ClientPutInServer", "Player already has Address");
	}
	
	char msg[256];
	_snprintf(msg, sizeof(msg), "Connecting %s (%s)\n", pPlayer->DisplayName(), pPlayer->m_ClientAddress);
	g_engfuncs.pfnServerPrint(msg);
	logfile << Logger::LOG_INFO << msg;

	// Read Profile from FN, if possible.
	pPlayer->steamID64 = UTIL_ComputeSteamID64(GETPLAYERAUTHID(pEntity));

	// Allocate a CBasePlayer for pev, and call spawn
	pPlayer->Spawn();

	// Reset interpolation during first frame
	pPlayer->pev->effects |= EF_NOINTERP;
	enddbg;
}

#include "voice_gamemgr.h"
extern CVoiceGameMgr g_VoiceGameMgr;

//// HOST_SAY
// String comes in as
// say blah blah blah
// or as
// blah blah blah
//
void Host_Say(edict_t *pEntity, int teamonly)
{
	CBasePlayer *client;
	int j;
	char *p;
	char text[128];
	char szTemp[256];
	const char *cpSay = "say";
	const char *cpSayTeam = "say_team";
	const char *pcmd = CMD_ARGV(0);

	// We can get a raw string now, without the "say " prepended
	if (CMD_ARGC() == 0)
		return;

	entvars_t *pev = &pEntity->v;
	CBasePlayer *player = GetClassPtr((CBasePlayer *)pev);

	//Not yet.
	if (player->m_flNextChatTime > gpGlobals->time)
		return;

	if (!_stricmp(pcmd, cpSay) || !_stricmp(pcmd, cpSayTeam))
	{
		if (CMD_ARGC() >= 2)
		{
			p = (char *)CMD_ARGS();
		}
		else
		{
			// say with a blank message, nothing to do
			return;
		}
	}
	else // Raw text, need to prepend argv[0]
	{
		if (CMD_ARGC() >= 2)
		{
			_snprintf(szTemp, sizeof(szTemp), "%s %s",  (char *)pcmd, (char *)CMD_ARGS());
		}
		else
		{
			// Just a one word command, use the first word...sigh
			_snprintf(szTemp, sizeof(szTemp), "%s", (char *)pcmd);
		}
		p = szTemp;
	}

	// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[strlen(p) - 1] = 0;
	}

	// make sure the text has content
	char *pc = NULL;
	for (pc = p; pc != NULL && *pc != 0; pc++)
	{
		if (isprint(*pc) && !isspace(*pc))
		{
			pc = NULL; // we've found an alphanumeric character,  so text is valid
			break;
		}
	}
	if (pc != NULL)
		return; // no character found, so say nothing

	// turn on color set 2  (color on,  no sound)
	if (teamonly)
		_snprintf(text, sizeof(text), "%c(TEAM) %s: ",  2, STRING(pEntity->v.netname) );
	else
		_snprintf(text, sizeof(text), "%c%s: ",  2, STRING(pEntity->v.netname) );

	j = sizeof(text) - 2 - strlen(text); // -2 for /n and null terminator
	if ((int)strlen(p) > j)
		p[j] = 0;

	strncat(text, p, sizeof(text));
	strncat(text, "\n", sizeof(text));

	player->m_flNextChatTime = gpGlobals->time + CHAT_INTERVAL;

	// loop through all players
	// Start with the first player.
	// This may return the world in single player if the client types something between levels or during spawn
	// so check it, or it will infinite loop

	client = NULL;
	while (((client = (CBasePlayer *)UTIL_FindEntityByClassname(client, "player")) != NULL) && (!FNullEnt(client->edict())))
	{
		if (!client->pev)
			continue;

		if (client->edict() == pEntity)
			continue;

		if (!(client->IsNetClient())) // Not a client ? (should never be true)
			continue;

		// can the receiver hear the sender? or has he muted him?
		if (g_VoiceGameMgr.PlayerHasBlockedPlayer(client, player))
			continue;

		if (teamonly && g_pGameRules->PlayerRelationship(client, CBaseEntity::Instance(pEntity)) != GR_TEAMMATE)
			continue;

		MESSAGE_BEGIN(MSG_ONE, gmsgSayText, NULL, client->pev);
		WRITE_BYTE(ENTINDEX(pEntity));
		WRITE_STRING(text);
		MESSAGE_END();
	}

	// print to the sending client
	MESSAGE_BEGIN(MSG_ONE, gmsgSayText, NULL, &pEntity->v);
	WRITE_BYTE(ENTINDEX(pEntity));
	WRITE_STRING(text);
	MESSAGE_END();

	// echo to server console
	g_engfuncs.pfnServerPrint(text);

	char *temp;
	if (teamonly)
		temp = "say_team";
	else
		temp = "say";

	UTIL_LogPrintf("\"%s<%i><%s>\" %s \"%s\"\n",
				   STRING(pEntity->v.netname),
				   GETPLAYERUSERID(pEntity),
				   GETPLAYERAUTHID(pEntity),
				   temp,
				   p);
}

/*
===========
ClientCommand
called each time a player uses a "cmd" command
============
*/
extern int gmsgHideWeapon;

// Use CMD_ARGS,  CMD_ARGV, and CMD_ARGC to get pointers the character string command.
// NOTE: CMD_ARGS CANNOT BE USED IF CMD_ARGC <= 1!!!
void ClientCommand2(edict_t *pEntity);

void ClientCommand(edict_t *pEntity)
{
	if (!CMD_ARGC())
		return;
	ClientCommand2(pEntity);
}

void ClientCommand2(edict_t *pEntity)
{
	const char *pcmd = CMD_ARGV(0);

	// Is the client spawned yet?
	if (!pEntity->pvPrivateData)
		return;

	entvars_t *pev = &pEntity->v;

	CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pev);
	int iHand = -1;

	if (MSGlobals::GameScript)
	{
		CBaseEntity *pWorld = CBaseEntity::Instance(ENT(0));
		if (pWorld)
		{
			pWorld->StoreEntity(pPlayer, ENT_CURRENTPLAYER);
			static msstringlist Parameters;
			Parameters.clearitems();

			for (int i = 0; i < CMD_ARGC(); i++)
			{
				//this is so terrible to do...
				//should think of another way to handle commands from scripts altogether.
				char* argv = (char*)CMD_ARGV(i);
				Parameters.add(strutil::stripBadChars(argv));
			}
				
			MSGlobals::GameScript->CallScriptEvent("game_playercmd", &Parameters);
		}
	}

	//JAN2010_09 Thothie - Changing situations where you can/can't use inventory
	bool bCanUseInventory = true;
	if (pPlayer->InMenu)
		bCanUseInventory = false; //SEP2011_07 - prevent inv use while in menus
	if (FBitSet(pPlayer->m_StatusFlags, PLAYER_MOVE_NOATTACK))
		bCanUseInventory = false; //can't use inventory cuz can't attack
	if (FBitSet(pPlayer->m_StatusFlags, PLAYER_MOVE_SITTING))
		bCanUseInventory = true; //but can't attack cuz sitting, so it's okay
	if (!pPlayer->IsAlive())
		bCanUseInventory = false; //JAN2010_19 wait, dead, can't use inventory after all
	if (pPlayer->HasConditions(MONSTER_TRADING))
		bCanUseInventory = false; //SEP2011_10 - prevent inv use while in trade

	if (FStrEq(pcmd, "say"))
	{
		char *Args = (char *)CMD_ARGS();
		pPlayer->Speak(Args, (speech_type)pPlayer->m_SayType);
	}
	else if (FStrEq(pcmd, "say_text") && !pPlayer->m_Gagged)
	{
		if (CMD_ARGC() >= 3)
		{
			char* Args = (char*)CMD_ARGS();
			int SayType = atoi(CMD_ARGV(1));
			msstring Text = msstring(Args).find_str(" "); //skip the first parameter
			Text = Text.substr(1);

			CBaseEntity* pGameMasterEnt = UTIL_FindEntityByString(NULL, "netname", msstring("¯") + "game_master");
			IScripted* pGMScript = (pGameMasterEnt ? pGameMasterEnt->GetScripted() : NULL);
			if (pGMScript)
			{
				msstringlist Parameters;
				Parameters.add(EntToString(pPlayer));
				Parameters.add(CMD_ARGV(1));
				Parameters.add(Text);
				pGMScript->CallScriptEvent("game_playerspeak", &Parameters);
			}

			pPlayer->Speak(Text, (speech_type)SayType);
		}
	}
	else if (FStrEq(pcmd, "setsay"))
	{
		pPlayer->m_SayType = atoi(CMD_ARGV(1));
	}
	else if (FStrEq(pcmd, "localcb")) // MiB MAR2015_01 [LOCAL_PANEL] - For doing server-side callback
	{
		if (CMD_ARGC() >= 3)
		{
			CBaseEntity *pEntity = StringToEnt(CMD_ARGV(1));
			IScripted *pScript = NULL;
			if (pEntity && (pScript = pEntity->GetScripted()))
			{
				static msstringlist Params;
				Params.clear();
				Params.add(EntToString(pPlayer));
				pScript->CallScriptEvent(CMD_ARGV(2), &Params);
			}
		}
	}
	//MiB MAR2012_07 - leap checks
	else if (FStrEq(pcmd, "game_leap"))
	{
		if (CMD_ARGC() == 3)
		{
			msstring leapDir = pcmd;
			leapDir += CMD_ARGV(1);

			static msstringlist parms;
			parms.clearitems();
			parms.add(msstring(CMD_ARGV(2)));
			pPlayer->CallScriptEvent(leapDir, &parms);
		}
	}
	else if (FStrEq(pcmd, "dosetanimatkadj"))
	{
		if ( CMD_ARGC() == 2 )
		{
				pPlayer->m_AnimSpeedAdj = atof(CMD_ARGV(1));
				msstringlist parms;
				parms.add( msstring(CMD_ARGV(1)) );
				pPlayer->CallScriptEvent( "game_cl_setatkspeed", &parms );
		}
	}
	//MiB JUN2010_17 - Allow choosing of what arrow to fire
	else if (FStrEq(pcmd, "choosingarrow"))
	{
		//tell script system I'm pickin arrows (NOV2014_16 moved up)
		//still not getting it... Wondering if this is client side
		msstringlist Parameters;
		Parameters.add(FloatToString(gpGlobals->time));
		pPlayer->CallScriptEvent("game_arrowmenu", &Parameters);
		return;
	}
	else if (FStrEq(pcmd, "selectarrow"))
	{
		if (CMD_ARGC() < 2)
		{
			return;
		}

		CGenericItem *pArrow;
		msstring Param = CMD_ARGV(1);
		if (Param.starts_with("GENERIC_"))
			pArrow = NewGenericItem(Param.ends_with("ARROW") ? "proj_arrow_generic" : "proj_bolt_generic");
		else
			pArrow = MSUtil_GetItemByID(atol(CMD_ARGV(1)), pPlayer);
		pPlayer->m_ChosenArrow = pArrow;

		pPlayer->SendInfoMsg("Selected ammo: %s", pArrow->DisplayName());

		//tell scripts I'm done selecting my arrow
		msstringlist Parameters;
		Parameters.add("0");
		Parameters.add(EntToString(pArrow));
		pPlayer->CallScriptEvent("game_arrowmenu", &Parameters);
	}
	else if (FStrEq(pcmd, "say_team"))
	{
	}
	/*else if ( FStrEq(pcmd, "fullupdate" ) )
	{
		GetClassPtr((CBasePlayer *)pev)->ForceClientDllUpdate();
	}*/
	else if (FStrEq(pcmd, "char") && CMD_ARGV(1)) //JAN2010_09 Thothie - Fixed sploit here where other character could be accessed instant - may also fix dbl click bug
	{
		//Thothie MAR2010_09 - seperating delete option from unloaded/loading conditional
		if (CMD_ARGC() == 3 && atoi(CMD_ARGV(1)) == -1 && pPlayer->m_CharacterState == CHARSTATE_UNLOADED) //Delete character
		{
			DeleteChar(pPlayer, atoi(CMD_ARGV(2)));
		}
		else if (pPlayer->m_CharacterState == CHARSTATE_UNLOADED && pPlayer->m_CharacterState != CHARSTATE_LOADING)
		{
			if (CMD_ARGC() == 2) //Select character
			{
				if (!strcmp(CMD_ARGV(1), "canjoin"))
					pPlayer->m_CanJoin = true;
				else
					pPlayer->LoadCharacter(atoi(CMD_ARGV(1)));
			}
			else if (CMD_ARGC() == 5) //Create character
			{
				if (MSGlobals::CanCreateCharOnMap)
				{
					//Create a new character
					createchar_t NewChar;
					NewChar.ServerSide = true;
					NewChar.iChar = atoi(CMD_ARGV(1));
					NewChar.Name = CMD_ARGV(2);
					NewChar.Gender = atoi(CMD_ARGV(3));
					NewChar.Weapon = CMD_ARGV(4);

					//Validate default weapon spawn
					bool validItem = false;
					for (int i = 0; i < MSGlobals::DefaultWeapons.size(); i++)
					{
						if (MSGlobals::DefaultWeapons[i] == NewChar.Weapon)
						{
							validItem = true;
							break;
						}
					}

					if ( validItem ) pPlayer->CreateChar(NewChar);
				}
				else
					ClientPrint(pPlayer->pev, at_console, "*** You can't create a new character on this map! ***\n");
			}
		}
		else
			ClientPrint(pPlayer->pev, at_console, "*** Can't Access Character Menu - Character Loaded or Loading ***\n");
	}
	else if (FStrEq(pcmd, "ul") && CMD_ARGC() >= 2)
	{
		if (!MSGlobals::ServerSideChar)
			return;

		if (MSGlobals::CentralEnabled)
			return;

		char Buffer[256];
		strncpy(Buffer, CMD_ARGV(1), 3);
		Buffer[3] = 0;

		if (!strcmp(Buffer, "new") && strlen(CMD_ARGV(1)) == 3 && CMD_ARGC() == 5)
		{
			MSChar_Interface::HL_SVNewIncomingChar(pPlayer, atoi(CMD_ARGV(2)), atoi(CMD_ARGV(3)), atoi(CMD_ARGV(4)));
		}
		else if (CMD_ARGC() == 2)
		{
			MSChar_Interface::HL_SVReadCharData(pPlayer, CMD_ARGV(1));
		}
	}

	else if (FStrEq(pcmd, "+special"))
		pPlayer->pbs.MoreBTNSDown |= BTN_SPECIAL;
	else if (FStrEq(pcmd, "-special"))
		pPlayer->pbs.MoreBTNSDown &= ~BTN_SPECIAL;

	else if (FStrEq(pcmd, "resethands"))
	{
		//Thothie - deletes weapon
		//	foreach( i, MAX_NPC_HANDS )
		//	if( pPlayer->Hand(i) ) pPlayer->Hand(i)->SUB_Remove( );
	}
	else if (FStrEq(pcmd, "get") && bCanUseInventory)
		pPlayer->GetAnyItems();

	//Stealing disabled
	//else if (FStrEq(pcmd, "steal"))
	//	pPlayer->StealAnyItems( NULL );

	else if (FStrEq(pcmd, "getmenuoptions"))
	{
		CMSMonster *pMonster = NULL;

		if (CMD_ARGC() >= 2)
		{
			int EntIdx = atoi(CMD_ARGV(1));
			CBaseEntity *pEntity = MSInstance(INDEXENT(EntIdx));
			if (pEntity && pEntity->IsMSMonster())
				pMonster = (CMSMonster *)pEntity;
		}
		else
		{
			pMonster = pPlayer;
		}

		if (pMonster)
			pMonster->OpenMenu(pPlayer);
	}
	else if (FStrEq(pcmd, "menuoption"))
	{
		if (CMD_ARGC() <= 2)
			return;

		int EntIdx = atoi(CMD_ARGV(1));
		CBaseEntity *pEntity = MSInstance(INDEXENT(EntIdx));
		if (!pEntity || !pEntity->IsMSMonster())
			return;

		CMSMonster *pMonster = (CMSMonster *)pEntity;
		int Option = atoi(CMD_ARGV(2));

		pPlayer->InMenu = false;

		pMonster->UseMenuOption(pPlayer, Option);
	}
	else if (FStrEq(pcmd, "offer"))
	{
		if (CMD_ARGC() >= 3)
		{
			offerinfo_t OfferInfo;
			memset(&OfferInfo, 0, sizeof(offerinfo_t));
			OfferInfo.ItemType = (itemtype_e)255;
			//Store the dest. monster idx for now
			//OfferItem() converts it to the src idx
			OfferInfo.SrcMonsterIDX = atoi(CMD_ARGV(1));

			if (!_stricmp(CMD_ARGV(2), "gold"))
			{
				if (CMD_ARGC() >= 4)
				{
					OfferInfo.ItemType = ITEM_GOLD;
					OfferInfo.pItemData = (void *)atoi(CMD_ARGV(3));
				}
			}
			else
			{
				iHand = atoi(CMD_ARGV(2));
				CGenericItem *pItem = pPlayer->Hand(iHand);
				if (pItem)
				{
					OfferInfo.ItemType = ITEM_NORMAL;
					OfferInfo.pItemData = (void *)pItem->entindex();
					OfferInfo.pItemData2 = (void *)pItem;
				}
			}

			if (OfferInfo.pItemData || OfferInfo.pItemData2)
				pPlayer->OfferItem(OfferInfo);
		}
	}
	else if (FStrEq(pcmd, "trade"))
	{
		if (CMD_ARGC() < 2)
			return;

		bool fDoTrade = false;
		tradeinfo_t tiTradeInfo;
		clrmem(tiTradeInfo);
		if (!_stricmp(CMD_ARGV(1), "buy"))
		{
			tiTradeInfo.iStatus = TRADE_BUY;
			fDoTrade = true;
		}
		else if (!_stricmp(CMD_ARGV(1), "sell"))
		{
			tiTradeInfo.iStatus = TRADE_SELL;
			fDoTrade = true;
		}

		if (CMD_ARGC() >= 2 && fDoTrade)
		{
			for (int i = 2; i < CMD_ARGC(); i++)
			{
				tiTradeInfo.ItemName = (char *)CMD_ARGV(i);
				tiTradeInfo.pCustomer = pPlayer;
				pPlayer->TradeItem(&tiTradeInfo);
			}
		}
		pPlayer->m_hEnemy = NULL;
	}
	else if (FStrEq(pcmd, "storage"))
	{
		if (CMD_ARGC() < 2)
			return;

		if (!_stricmp(CMD_ARGV(1), "stop"))
			pPlayer->m_CurrentStorage.Active = false;

		if (CMD_ARGC() < 3)
			return;

		if (pPlayer->m_CurrentStorage.Active)
		{
			storage_t *pStorage = pPlayer->Storage_GetStorage(pPlayer->m_CurrentStorage.StorageName);
			if (pStorage)
			{
				if (!_stricmp(CMD_ARGV(1), "add"))
				{
					CGenericItem *pItem = MSUtil_GetItemByID(atol(CMD_ARGV(2)), pPlayer);
					if (pItem)
					{
						//Determine a unique ID for the item in storage
						genericitem_full_t Item(pItem);
						for (unsigned int ID = 0; ID < SHRT_MAX; ID++)
						{
							bool Duplicate = false;
							for (int i = 0; i < pStorage->Items.size(); i++)
								if (pStorage->Items[i].ID == ID)
								{
									Duplicate = true;
									break;
								}
							if (Duplicate)
								continue;
							Item.ID = ID;
							break;
						}

						pStorage->Items.add(Item);
						pPlayer->Storage_Send();
						pPlayer->RemoveItem(pItem);
						pPlayer->SendEventMsg(HUDEVENT_NORMAL, msstring("You put ") + SPEECH::ItemName(pItem) + " in storage");
						pItem->SUB_Remove();
					}
				}
				else if (!_stricmp(CMD_ARGV(1), "remove"))
				{
					if (CMD_ARGC() < 4)
						return;

					int ItemID = atoi(CMD_ARGV(2));
					genericitem_t *pStorageItem = NULL;
					int StorageItemIdx = 0;
					for (unsigned int i = 0; i < pStorage->Items.size(); i++)
						if (pStorage->Items[i].ID == ItemID)
						{
							pStorageItem = &pStorage->Items[i];
							StorageItemIdx = i;
							break;
						}

					if (pStorageItem)
					{
						CGenericItem *pItem = *pStorageItem;
						int Cost = int(pItem->m_Value * pPlayer->m_CurrentStorage.flFeeRatio);

						if (!Cost || pPlayer->m_Gold > Cost) //If no retrieval fee or I have enough to pay it...
						{
							int ContainerID = atol(CMD_ARGV(3));
							bool Success = false;

							if (!ContainerID)
								Success = pItem->GiveTo(pPlayer, true, true);
							else
							{
								CGenericItem *pContainer = MSUtil_GetItemByID(ContainerID, pPlayer);
								if (pContainer)
									Success = pPlayer->PutInPack(pItem, pContainer);
							}

							if (Success)
							{
								pPlayer->SendEventMsg(HUDEVENT_NORMAL, msstring("You take ") + SPEECH::ItemName(pItem) + " from storage");
								if (Cost)
									pPlayer->SendEventMsg(HUDEVENT_NORMAL, msstring("Retrieval fee: ") + Cost + " Gold");
								else
									pPlayer->SendEventMsg(HUDEVENT_NORMAL, msstring("Retrieval fee: None"));

								pPlayer->GiveGold(-Cost, false);
								pStorage->Items.erase(StorageItemIdx);
								pPlayer->Storage_Send();
							}
							else
								pItem->SUB_Remove();
						}
						else
							pPlayer->SendEventMsg(HUDEVENT_UNABLE, msstring("You can't afford the retrieval fee of ") + Cost + " Gold");
					}
				}
			}
		}
	}
	else if (FStrEq(pcmd, "reset_tips"))
	{
		pPlayer->m_ViewedHelpTips.clear();
		pPlayer->SendInfoMsg("Tips reset.");
	}
	else if (FStrEq(pcmd, "reset_quests"))
	{
		//Thothie JAN2010_09 spolitable
		//pPlayer->m_Quests.clear( );
		//pPlayer->SendInfoMsg( "Quests reset." );
	}
	else if (FStrEq(pcmd, "prep"))
	{
		if (!CMD_ARGV(1))
			return;
		//Thothie attempting to de-h4x this
		//char * cmholder;
		if (bCanUseInventory)
		{
			pPlayer->PrepareSpell(CMD_ARGV(1)); //Thothie - total h4x - doesnt check if you have the item or spell! YOU CAN SPAWN ANYTHING!
		}
		else
		{
			if (FBitSet(pPlayer->m_StatusFlags, PLAYER_MOVE_NOMOVE))
				pPlayer->SendInfoMsg("Can't prepare spells while frozen or stunned...");
			else
				pPlayer->SendInfoMsg("Can't prepare spells now.");
		}
	}
	else if (FStrEq(pcmd, "useskill"))
	{
		if (!CMD_ARGV(1))
			return;
		//pPlayer->UseSkill( atoi((CHAR *)CMD_ARGV(1)) ); //Thothie - smells of h4x
	}
	else if (FStrEq(pcmd, "quickslot") && bCanUseInventory)
	{
		//This is only called when creating a slot, so this wont work here
		/*
		if ( FStrEq(CMD_ARGV(1),"arrow") )
		{
			//tell script system I'm pickin arrows (NOV2014_16 moved up)
			//still not getting it... Wondering if this is client side
			msstringlist Parameters;
			Parameters.add( FloatToString(gpGlobals->time) );
			pPlayer->CallScriptEvent( "game_arrowmenu", &Parameters );
		}
		*/
		if (FStrEq(CMD_ARGV(1), "create"))
		{
			if (CMD_ARGC() < 4)
				return;

			ulong ID = atol(CMD_ARGV(3));
			if (FStrEq(CMD_ARGV(3), "current"))
				ID = pPlayer->ActiveItem() ? pPlayer->ActiveItem()->m_iId : 0;

			pPlayer->QuickSlot_Create(atoi(CMD_ARGV(2)), ID, true);
		}
		//NOV2014_16 comment - should there not be a return here?
	}
	else if (FStrEq(pcmd, "drop"))
	{
		// Player is dropping an item.
		//syntax: "drop <ID>"
		if (bCanUseInventory)
		{
			CGenericItem *pItem = NULL;
			if (CMD_ARGC() > 1)
				pItem = MSUtil_GetItemByID(atol(CMD_ARGV(1)), pPlayer);
			else
				pItem = pPlayer->ActiveItem();

			UTIL_MakeVectors(pev->angles);
			if (pItem)
				pPlayer->DropItem(pItem, false, true);
			else
				MSErrorConsoleText("ClientCommand()", msstring("'drop' cmd couldn't find item to drop"));
		}
		else
		{
			if (FBitSet(pPlayer->m_StatusFlags, PLAYER_MOVE_NOMOVE))
				pPlayer->SendInfoMsg("Can't use inventory while frozen or stunned...");
			else
				pPlayer->SendInfoMsg("Can't use inventory now.");
		}
	}
	/*
	else if ( FStrEq(pcmd, "drop" ) && FBitSet(pPlayer->m_StatusFlags, PLAYER_MOVE_NOATTACK) )
	{
		//ALERT( at_console, "Fbitset: %i\n", FBitSet(pPlayer->m_StatusFlags, PLAYER_MOVE_SITTING) );
		if ( !FBitSet(pPlayer->m_StatusFlags, PLAYER_MOVE_SITTING) )
		{
			if ( FBitSet(pPlayer->m_StatusFlags, PLAYER_MOVE_NOMOVE) ) pPlayer->SendInfoMsg("Can't use inventory while frozen or sitting...");
			else pPlayer->SendInfoMsg("Can't use inventory now.");
		}
	}
*/
	else if (FStrEq(pcmd, "reconnect") || FStrEq(pcmd, "retry"))
	{
		//Thothie JUL2007a - anti item dup
		//Player is reconnecting, alert scripts
		//syntax: "drop <ID>"
		pPlayer->m_TimeCharLastSent = 0; //Thothie SEP2011_07 - force save (attempt)
		msstringlist Parameters;
		Parameters.add(EntToString(pPlayer));
		MSGlobals::GameScript->CallScriptEvent("game_playerleave", &Parameters);
	}
	else if (FStrEq(pcmd, "use"))
	{
		// player is trying to use an item.
		//syntax: "use <hand>"
		if (bCanUseInventory)
		{
			int Hand = pPlayer->m_CurrentHand;
			if (CMD_ARGC() > 1)
				Hand = atoi(CMD_ARGV(1));

			pPlayer->UseItem(Hand, true);
		}
		else
		{
			if (FBitSet(pPlayer->m_StatusFlags, PLAYER_MOVE_NOMOVE))
				pPlayer->SendInfoMsg("Can't use inventory while frozen or stunned...");
			else
				pPlayer->SendInfoMsg("Can't use inventory now.");
		}
	}
	else if (FStrEq(pcmd, "remove") && CMD_ARGV(1))
	{
		ulong ID = atol(CMD_ARGV(1));
		CGenericItem *pItem = MSUtil_GetItemByID(ID, pPlayer);

		if (pItem && pItem->GiveTo(pPlayer, true, false))
		{
			pItem->CallScriptEvent("game_removepack");
		}

		/*if( pItem )
		{
			//pPlayer->m_fClientInitiated = true;
			if( pItem->GiveTo( pPlayer, true, false ) )
			{

				MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_PACK], NULL, pPlayer->pev );
					WRITE_BYTE( 1 );
					WRITE_LONG( pItem->m_iId );
					WRITE_BYTE( iHand + 1 );
				MESSAGE_END();
				//Ensure hand messages aren't sent
				pPlayer->m_ClientCurrentHand = pPlayer->iCurrentHand;
				pPlayer->m_ClientHandID[pItem->m_Hand] = pItem->m_iId;
			}
			//else MSErrorConsoleText( "ClientCommand()", "'remove' cmd failed" );
			//pPlayer->m_fClientInitiated = false;
		}
		else MSErrorConsoleText( "ClientCommand()", msstring("'remove' cmd couldn't find item ") + (int)ID );*/
	}
	else if (FStrEq(pcmd, "action"))
	{ //'act' is a reserved Half-life command
		if (!CMD_ARGV(1))
			return;
		pPlayer->PlayerAction(CMD_ARGV(1));
	}
	else if (FStrEq(pcmd, "savefileid") && CMD_ARGV(1))
	{
		//Thothie - smells of h4x
		//pPlayer->m_SaveFileID = atoi(CMD_ARGV(1));
	}

	//Debug commands
	/*	else if (FStrEq(pcmd, "effect") && g_flWeaponCheat) {
		UTIL_MakeVectors( pPlayer->pev->v_angle );
		Vector vOrigin = pev->origin + gpGlobals->v_forward * 20;
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, vOrigin );
			WRITE_BYTE( TE_EXPLOSION);
			WRITE_COORD( vOrigin.x );
			WRITE_COORD( vOrigin.y );
			WRITE_COORD( vOrigin.z );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( (BYTE)1 ); // scale * 10
			WRITE_BYTE( 15  ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NODLIGHTS );
		MESSAGE_END();
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, vOrigin );
			WRITE_BYTE( TE_DLIGHT );
			WRITE_COORD( vOrigin.x );
			WRITE_COORD( vOrigin.y );
			WRITE_COORD( vOrigin.z );
			WRITE_BYTE( 10 );
			WRITE_BYTE( 255 ); //R
			WRITE_BYTE( 0 ); //G
			WRITE_BYTE( 0 ); //B
			WRITE_BYTE( 50 ); // brightness
			WRITE_BYTE( 10 ); // life
			WRITE_BYTE( 1 );
		MESSAGE_END();
	}*/

	else if (FStrEq(pcmd, ATTACK_COMMAND))
	{
		if (CMD_ARGC() > 2)
		{

			//AAUG MASSIVE SPLOIT - THOTHIE FEB2008a
			//anyone who uses "atx x x" can use ANY attack!
			//we need to pass another arg to verify that the charge took place
			//but cant do this just now as it'll fubar the beta server
			//int Hand = atoi( CMD_ARGV(1) );

			//MiB - A G-G-G-G-G fixed.

			int AttackCode = atoi(CMD_ARGV(2));

			//tried to use m_HP as pass varg, but no good, as server and client lose sync
			//int AttackVerify = atoi( CMD_ARGV(3) );
			//if ( (AttackVerify != pPlayer->m_HP) && AttackCode > -1 )
			//{
			//	//report cheater here
			//	pPlayer->SendInfoMsg("Anti-cheat verification pending.");
			//}
			//else
			//{
			//MiB JUN2010_19 - Commented this out. New way of doing things to allow both hands to attack at once.
			/*
				pPlayer->m_ClientAttack = true;
				pPlayer->m_ClientAttackHand = Hand;
				pPlayer->m_ClientAttackNum = AttackCode;
				*/
			//}

			// This is how we're doing it, now. Client tells the item it's attacking.
			CGenericItem *pItem = MSUtil_GetItemByID(atol(CMD_ARGV(1)), pPlayer);
			//MiB MAR2012 - charge percent (original follows)
			if (pItem && pItem->m_Location == ITEMPOS_HANDS && !FBitSet(pPlayer->m_StatusFlags, PLAYER_MOVE_NOATTACK))
			{
				pItem->ClientAttacking = true;
				pItem->AttackNum = AttackCode;
				float chargePercent = atof(CMD_ARGV(3));
				if (AttackCode == 0 && chargePercent > 0)
				{
					msstringlist parms;
					parms.clearitems();
					parms.add(CMD_ARGV(3)); //msstring() + chargePercent );
					pItem->CallScriptEvent("game_setchargepercent", &parms);
				}
			}
			else if (pItem)
			{
				//NOV2014_15 - Think we accidentally undid this :O
				pItem->ClientAttacking = true;
				pItem->AttackNum = AttackCode;
			}
			/*
			float chargePercent = atof(CMD_ARGV(3));
			if ( chargePercent > 0 && chargePercent < 1 )
			{
				msstringlist parms;
				parms.clearitems();
				parms.add( CMD_ARGV(3) ); //msstring() + chargePercent );
				pItem->CallScriptEvent( "game_setchargepercent", &parms );
			}
			if ( pItem && pItem->m_Location == ITEMPOS_HANDS )
			{
				pItem->ClientAttacking = true;
				pItem->AttackNum = AttackCode;
			}
			*/
		}
	}
	else if (FStrEq(pcmd, "drink"))
	{
		if (CMD_ARGC() > 1)
		{
			int Hand = atoi(CMD_ARGV(1));
			CGenericItem *pItem = pPlayer->Hand(Hand);
			if (pItem)
				pItem->Drink();
		}
	}
	else if (FStrEq(pcmd, "switchhand"))
	{
		//Thothie MAR2012 - giving option for "silent" switch hand for spawn hand fix
		if (!strcmp(CMD_ARGV(1), "1"))
		{
			pPlayer->SwitchHands(!pPlayer->m_CurrentHand, false);
		}
		else
		{
			pPlayer->SwitchHands(!pPlayer->m_CurrentHand, true);
		}
		/*if( !pPlayer->SwitchHands( !pPlayer->m_CurrentHand, true ) )
			//pPlayer->m_ClientCurrentHand = pPlayer->iCurrentHand;
		//else
		{
			MSErrorConsoleText( "ClientCommand()", "'hand' cmd failed" );
			//Switch hands failed...
			//Since it succeeded on the client, there's an inconsistancy.
			//Fix it by forcing an update
			pPlayer->m_ClientCurrentHand = -1;
		}*/
	}
	//Thothie JUL2011_02 - Dynamic Client Command
	else if (FStrEq(pcmd, "ce"))
	{
		//clcmd <player|GM> <event> <params> - called by client-side script command clcmd
		/*IScripted *pScripted = NULL;
		if (!strcmp(CMD_ARGV(1), "GM"))
		{
			ALERT(at_console, "DEBUG: ce - requested GM as target\n");
			CBaseEntity *pGameMasterEnt = UTIL_FindEntityByString(NULL, "netname", msstring("¯") + "game_master");
			if (pGameMasterEnt)
			{
				pScripted = pGameMasterEnt->GetScripted();
			}
			else
			{
				ALERT(at_console, "ERROR: ce - Failed to find game master!\n");
			}
		}
		else
		{
			if (!strcmp(CMD_ARGV(1), "player"))
			{
				pScripted = pPlayer->GetScripted();
			}
		}

		if (pScripted)
		{
			static msstringlist Params;
			Params.clearitems();
			int i = 0;
			for (i = 0; i < CMD_ARGC(); i++)
			{
				if (i > 2)
				{
					Params.add(CMD_ARGV(i));
				}
			}
			Params.add(CMD_ARGV(i + 1));
			pScripted->CallScriptEvent(CMD_ARGV(2), &Params);
		}*/

		/*
		CBaseEntity *pEntity = StringToEnt( CMD_ARGV(1) );
		if( pEntity )
		{
			pScripted = pEntity->GetScripted();
			if ( pScripted )
			{
				static msstringlist Params;
				Params.clearitems( );
				 for (int i = 0; i < CMD_ARGC(); i++)
				{
					if ( i > 2 )
					{
						Params.add( CMD_ARGV(i) );
					}
				}
				pScripted->CallScriptEvent( CMD_ARGV(2), &Params );
			}
			else
			{
				MSErrorConsoleText( "clcmd - entity ", msstring(CMD_ARGV(1)) + " found, but not scripted." );
			}
		}
		else
		{
			MSErrorConsoleText( "clcmd - entity ", msstring(CMD_ARGV(1)) + " not found." );
		}
		*/
	}
	else if (FStrEq(pcmd, "inv") && CMD_ARGV(1) && bCanUseInventory) //MAY2008 - no pulling invenotry when you cant attack
	{
		if (pPlayer->pev->weaponanim > 0)
			return;

		//This event is predicted client-side, so do NOT send hand
		//update messages
		if (FStrEq(CMD_ARGV(1), "stop"))
			pPlayer->ClearConditions(MONSTER_OPENCONTAINER);
		//pPlayer->OpenPack->Container_UnListContents( );

		else if (FStrEq(CMD_ARGV(1), "get"))
		{
			CGenericItem *pContainer = MSUtil_GetItemByID(atol(CMD_ARGV(2)), pPlayer);
			if (pContainer)
			{
				CGenericItem *pItem = MSUtil_GetItemByID(atol(CMD_ARGV(3)), pPlayer);
				if (pItem)
				{
					//pPlayer->m_fClientInitiated = true;
					if (pItem->GiveTo(pPlayer, true, false))
					{
						pItem->CallScriptEvent("removefrompack"); //old
						pItem->CallScriptEvent("game_removefrompack");
						//Ensure hand messages aren't sent
						//pPlayer->m_ClientCurrentHand = pPlayer->iCurrentHand;
						//pPlayer->m_ClientHandID[pItem->m_Hand] = pItem->m_iId;
					}
					//pPlayer->m_fClientInitiated = false;
				}
				else
					MSErrorConsoleText("ClientCommand - inv get item", msstring("Item ") + atoi(CMD_ARGV(3)) + " does not exist on player!");
			}
			else
				MSErrorConsoleText("ClientCommand - inv get item", msstring("Pack ") + atoi(CMD_ARGV(2)) + " does not exist on player!");
		}
		else if (FStrEq(CMD_ARGV(1), "open"))
		{
			//Thothie MAR2011_18 - moved item stacker here to prevent overflow
			for (int i = 0; i < pPlayer->Gear.size(); i++)
			{
				CGenericItem *pItem = pPlayer->Gear[i];
				if (pItem)
				{
					if (pItem->PackData)
					{
						pItem->Container_StackItems();
						pItem->Container_SendContents();
					}
				}
			}

			CGenericItem *pPack = MSUtil_GetItemByID(atol(CMD_ARGV(2)), pPlayer);

			if (pPack)
			{
				pPack->Container_Open();
			}
		}
		else if (FStrEq(CMD_ARGV(1), "transfer"))
		{
			CGenericItem *pItem = MSUtil_GetItemByID(atol(CMD_ARGV(2)), pPlayer);
			if (pItem)
			{
				ulong ContainerID = atol(CMD_ARGV(3));
				if (ContainerID)
				{
					//Transfer to pack
					CGenericItem *pContainer = pPlayer->GetContainer(atol(CMD_ARGV(3)));
					if (pContainer)
					{
						pPlayer->PutInPack(pItem, pContainer, true);
					}
				}
				//Transfer to hand
				else if (pItem->GiveTo(pPlayer, true, false, true))
				{
					pItem->CallScriptEvent("removefrompack"); //old
					pItem->CallScriptEvent("game_removefrompack");
				}
			}
		}
	}
	else if (FStrEq(pcmd, "inv") && !bCanUseInventory)
	{
		if (FBitSet(pPlayer->m_StatusFlags, PLAYER_MOVE_NOMOVE))
			pPlayer->SendInfoMsg("Can't use inventory while frozen or stunned...");
		else
			pPlayer->SendInfoMsg("Can't use inventory now.");
	}
	else if (FStrEq(pcmd, "entcount"))
	{
		int e = 0;
		for (int i = 0; i < gpGlobals->maxEntities; i++)
		{
			edict_t *pEdict = g_engfuncs.pfnPEntityOfEntIndex(i);
			if (!pEdict)
				continue;

			if (pEdict->free)
				continue;

			// Clients aren't necessarily initialized until ClientPutInServer()
			if (!pEdict->pvPrivateData)
				continue;

			e++;
		}

		Print("Entities: %i\n", e);
	}
	else if (g_pGameRules->ClientCommand(GetClassPtr((CBasePlayer *)pev), pcmd))
	{
		// MenuSelect returns true only if the command is properly handled,  so don't print a warning
	}
// Cheats
#ifndef RELEASE_LOCKDOWN
	else if (FStrEq(pcmd, "gmcmd") && CMD_ARGC() >= 2)
	{
		//char code[4]; //god
		//code[0] = 'g'; code[1] = 'o';
		//code[2] = 'd'; code[3] = 0;
		//if( FStrEq(CMD_ARGV(1),code) )
		//{
		bool fPrevGMStatus = pPlayer->m_fIsElite;
		//#ifdef DEV_BUILD
		pPlayer->m_fIsElite = atoi(CMD_ARGV(1)) ? true : false;
		//#else
		/*if( IsGM(GETPLAYERWONID(pPlayer->edict())) )
					pPlayer->m_fIsElite = !pPlayer->m_fIsElite;
				else return FALSE;*/
		//	pPlayer->m_fIsElite = true;
		//#endif

		if (pPlayer->m_fIsElite != fPrevGMStatus)
		{
			if (pPlayer->m_fIsElite)
				pPlayer->SendInfoMsg("You are now Elite\n");
			else
				pPlayer->SendInfoMsg("You are now Normal\n");
			MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_CLDLLFUNC], NULL, pPlayer->pev);
			WRITE_BYTE(7);
			WRITE_BYTE(pPlayer->m_fIsElite);
			MESSAGE_END();

			pPlayer->SaveChar(); //Save our new status
		}
		//}
	}
	else if (pPlayer->IsElite()
#ifdef DEV_BUILD
			 && ms_allowdev.value
#endif
	)
	{
		if (FStrEq(pcmd, "listitems"))
		{
			char pszMatch[64]; pszMatch[0] = 0;
			if (CMD_ARGC() > 1)
				 strncpy(pszMatch,  CMD_ARGV(1), sizeof(pszMatch) );
			int count = 0;
			for (int i = 0; i < CGenericItemMgr::ItemCount(); i++)
			{
				GenItem_t *pGenItem = CGenericItemMgr::Item(i);
				if (strstr(pGenItem->Name, pszMatch))
				{
					ALERT(at_console, "Item: %s\n", pGenItem->Name.c_str());
					count++;
				}
			}

			if (count)
				ALERT(at_console, "Items found: %i\n", count);
			else
				ALERT(at_console, "No items found.\n");
		}
		/*	else if ( FStrEq(pcmd, "dotrick") )
		{
			hudtextparms_t htp; memset( &htp, 0, sizeof(hudtextparms_t) );
			htp.x = 0.02; htp.y = 0.6;
			htp.effect = 2;
			htp.r1 = 0; htp.g1 = 128; htp.b1 = 0;
			htp.r2 = 178; htp.g2 = 119; htp.b2 = 0;
			htp.fadeinTime = 0.02; htp.fadeoutTime = 3.0; htp.holdTime = 2.0;
			htp.fxTime = 0.6;
			UTIL_HudMessage( pPlayer, htp, "Reset PKs" );
			pPlayer->m_PlayersKilled = 0;
			pPlayer->m_TimeWaitedToForgetKill = 0;
		}*/
		else if (FStrEq(pcmd, "teleport")) //Teleport anywhere
		{
			if (CMD_ARGC() == 4)
				pPlayer->pev->origin = Vector(atof(CMD_ARGV(1)), atof(CMD_ARGV(2)), atof(CMD_ARGV(3)));
			else
				ALERT(at_console, "Position: %f %f %f\n", pPlayer->pev->origin.x, pPlayer->pev->origin.y, pPlayer->pev->origin.z);
		}
		else if (FStrEq(pcmd, "kick"))
		{ // allow kicking
			if (CMD_ARGV(1))
				SERVER_COMMAND(UTIL_VarArgs("kick %s\n", CMD_ARGV(1)));
		}
		else if (FStrEq(pcmd, "give"))
		{ // free items
			if (CMD_ARGC() >= 2)
			{
				CBaseEntity *pEnt = pPlayer->GiveNamedItem(CMD_ARGV(1));
				if (CMD_ARGC() >= 3 && pEnt && (pEnt->MSProperties() & ITEM_GENERIC))
					((CGenericItem *)pEnt)->iQuantity = atoi(CMD_ARGV(2));
			}
		}
		else if (FStrEq(pcmd, "setstat"))
		{ // free stats
			if (CMD_ARGV(3))
			{
				msstring FullName = CMD_ARGV(1);
				if (FullName.contains(".")) //New way - <stat.prop> <value>
				{
					msstring StatName = FullName.thru_char(".");
					CStat *pStat = pPlayer->FindStat(StatName);
					if (pStat)
					{
						msstring PropName = FullName.substr(StatName.len() + 1);
						int iProp = GetSubSkillByName(PropName);
						if (iProp > -1)
						{
							int value = atoi(CMD_ARGV(2));
							pStat->m_SubStats[iProp].Value = min(value, STATPROP_MAX_VALUE);
						}
					}
				}
				else //Old way.  <stat name> <substat> <value>
				{
					CStat *pStat = pPlayer->FindStat(CMD_ARGV(1));
					if (pStat)
					{
						int iSubStat = atoi(CMD_ARGV(2));
						int value = atoi(CMD_ARGV(3));
						if (iSubStat < (signed)pStat->m_SubStats.size())
							pStat->m_SubStats[iSubStat].Value = min(value, STATPROP_MAX_VALUE);
					}
				}
			}
		}
		else if (FStrEq(pcmd, "sethp"))
		{ // set hp
			if (CMD_ARGV(1))
			{
				pPlayer->m_HP = pPlayer->pev->health = 0;
				pPlayer->GiveHP(atof(CMD_ARGV(1)));
			}
		}
		else if (FStrEq(pcmd, "setmp"))
		{ // set mp
			if (CMD_ARGV(1))
			{
				pPlayer->m_MP = 0;
				pPlayer->GiveMP(atof(CMD_ARGV(1)));
			}
		}
		/*
		//Thothie AUG2010_02 - moving this script side
		else if ( FStrEq(pcmd, "setweather" ) ) { // set weather
			if( CMD_ARGV(1) )
			{
				msstringlist Parameters;
				Parameters.add( CMD_ARGV(1) );
				if( CMD_ARGC() >= 3 ) Parameters.add( CMD_ARGV(2) );
				else Parameters.add( "default" );							//Default 15 seconds to stop old weather
				if( CMD_ARGC() >= 4 ) Parameters.add( CMD_ARGV(3) );
				else Parameters.add( "default" );							//Default 15 seconds to finish new weather

				if( MSGlobals::GameScript )
					MSGlobals::GameScript->CallScriptEvent( "game_playercmd_setweather", &Parameters );
			}
		}
		*/
		else if (FStrEq(pcmd, "iamgod"))
		{ // god mode
			if (CMD_ARGV(1))
			{
				if (atoi(CMD_ARGV(1)))
					SetBits(pPlayer->pev->flags, FL_GODMODE);
				else
					ClearBits(pPlayer->pev->flags, FL_GODMODE);
			}
		}
		else if (FStrEq(pcmd, "freegold"))
			pPlayer->GiveGold(1000);
		else if (FStrEq(pcmd, "gibme"))
		{
			if (pPlayer->pev->health <= 0)
				return;
			pPlayer->pev->health = 0;
			pPlayer->Killed(pPlayer->pev, GIB_ALWAYS);
		}
		else if (FStrEq(pcmd, "buffer"))
		{
			pPlayer->SendInfoMsg("%s", g_engfuncs.pfnGetInfoKeyBuffer(pPlayer->edict()));
		}
		/*else if ( FStrEq(pcmd, "uniqueid" ) &&  pPlayer->IsElite() ) {
			char cTemp1[64];
			 _snprintf(cTemp1, sizeof(cTemp1),  "%.3f",  CVAR_GET_FLOAT("ms_key") );
			pPlayer->SendInfoMsg( "My ID: %s\n", cTemp1 );
		}*/
		else if (FStrEq(pcmd, "npc") && CMD_ARGV(1))
		{
			//Removed cap on summons
			//if( g_SummonedMonsters >= MAX_MONSTERS ) return;

			CMSMonster *NewMonster = (CMSMonster *)CREATE_ENT("ms_npc");
			if (NewMonster)
			{
				UTIL_MakeVectors(pPlayer->pev->v_angle);
				NewMonster->pev->origin = pPlayer->EyePosition() + gpGlobals->v_forward * 90;
				NewMonster->pev->origin.z = pPlayer->pev->origin.z + 8;
				NewMonster->pev->classname = MAKE_STRING("msmonster_summoned");
				g_SummonedMonsters++;

				NewMonster->Spawn(CMD_ARGV(1));
			}
		}
		else if (FStrEq(pcmd, "fov"))
		{
			if (CMD_ARGC() >= 2)
				pPlayer->m_iFOV = atoi(CMD_ARGV(1));
		}
		else if (FStrEq(pcmd, "sizemeup"))
		{
			pPlayer->SendInfoMsg("MoveType: %i\n", pPlayer->pev->movetype);
			Vector min = pev->origin + -(pPlayer->Size() / 2), max = pev->origin + (pPlayer->Size() / 2);
			//BeamEffect( min, max, iBeam, 0, 0, 100, 10, 0, 255, 255,255, 255, 20 );
		}
		else if (FStrEq(pcmd, "specme"))
		{
			int Type = CMD_ARGV(1) ? atoi(CMD_ARGV(1)) : 0;
			switch (Type)
			{
			case 0:
				if (FBitSet(pPlayer->m_afPhysicsFlags, PFLAG_OBSERVER))
				{
					pPlayer->pev->movetype = MOVETYPE_WALK;
					pPlayer->pev->solid = SOLID_BBOX;
					ClearBits(pPlayer->m_afPhysicsFlags, PFLAG_OBSERVER);
					ClearBits(pPlayer->pev->flags, FL_NOTARGET);
					//pPlayer->SetSpeed( 0 );
					MESSAGE_BEGIN(MSG_ONE, gmsgHideWeapon, NULL, pPlayer->pev);
					WRITE_BYTE(0);
					MESSAGE_END();
				}
				break;
			case 1:
				//pPlayer->RemoveAllItems( FALSE, TRUE );
				MESSAGE_BEGIN(MSG_ONE, gmsgHideWeapon, NULL, pPlayer->pev);
				WRITE_BYTE(HIDEHUD_ALL);
				MESSAGE_END();
				//--- NO BREAK ---
			case 2:
				pPlayer->pev->movetype = MOVETYPE_NOCLIP;
				pPlayer->pev->solid = SOLID_NOT;
				SetBits(pPlayer->m_afPhysicsFlags, PFLAG_OBSERVER);
				SetBits(pPlayer->pev->flags, FL_NOTARGET);
				//pPlayer->SetSpeed( 300 );
				break;
			}
		}
		else if (FStrEq(pcmd, "quest"))
		{
			if (CMD_ARGC() >= 3)
				pPlayer->SetQuest(true, CMD_ARGV(1), CMD_ARGV(2));
		}
	}
#endif
	// ------ END CHEATS -----------
	else
	{
		// tell the user they entered an unknown command
		//Thothie - buggy, counts MS commands as unknown, thus supressing
		//ClientPrint( &pEntity->v, HUD_PRINTCONSOLE, UTIL_VarArgs( "Unknown command: %s\n", pcmd ) );
	}
}

/*
========================
ClientUserInfoChanged

called after the player changes
userinfo - gives dll a chance to modify it before
it gets sent into the rest of the engine.
========================
*/
void ClientUserInfoChanged(edict_t *pEntity, char *infobuffer)
{

	/*if( !pEntity->v.netname )
	{
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX(pEntity), infobuffer, "name", (char *)"myname" );
		pEntity->v.netname = ALLOC_STRING("myname");
	}*/

	// Is the client spawned yet?
	if (!pEntity->pvPrivateData)
		return;

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	/*	if ( pEntity->v.netname && STRING(pEntity->v.netname)[0] != 0 && !FStrEq( STRING(pEntity->v.netname), g_engfuncs.pfnInfoKeyValue( infobuffer, "name" )) )
	{
		char text[256];
		 _snprintf(text, sizeof(text),  "* %s prefers the name %s\n",  STRING(pEntity->v.netname),  g_engfuncs.pfnInfoKeyValue( infobuffer,  "name" ) );
		MESSAGE_BEGIN( MSG_ALL, gmsgSayText, NULL );
			WRITE_BYTE( ENTINDEX(pEntity) );
			WRITE_STRING( text );
		MESSAGE_END();

		UTIL_LogPrintf( "\"%s<%i>\" changed name to \"%s<%i>\"\n", STRING( pEntity->v.netname ), GETPLAYERUSERID( pEntity ), g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ), GETPLAYERUSERID( pEntity ) );
	}*/

	if (g_pGameRules)
		g_pGameRules->ClientUserInfoChanged(GetClassPtr((CBasePlayer *)&pEntity->v), infobuffer);
}

static int g_serveractive = 0;

void ServerDeactivate(void)
{
	// It's possible that the engine will call this function more times than is necessary
	//  Therefore, only run it one time for each call to ServerActivate
	if (g_serveractive != 1)
	{
		return;
	}

	g_serveractive = 0;

	// Peform any shutdown operations here...
	//

	if (g_pGameRules)
		g_pGameRules->EndMultiplayerGame();

	MSGameEnd();
}

DLL_GLOBAL extern bool g_fInPrecache; //Code called from is in CWorld::Precache

void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
	DBG_INPUT;
	startdbg;
	int i;
	CBaseEntity *pClass;
	logfile << Logger::LOG_INFO << "World Activate..." << std::flush;

	// Every call to ServerActivate should be matched by a call to ServerDeactivate
	g_serveractive = 1;
	//g_fInPrecache = false;  //Monsters wont spawn if I set this here instead of the end of CWorld::Spawn

	// Clients have not been initialized yet
	for (i = 0; i < edictCount; i++)
	{
		if (pEdictList[i].free)
			continue;

		// Clients aren't necessarily initialized until ClientPutInServer()
		if ((i != 0 && i < clientMax) || !pEdictList[i].pvPrivateData)
			continue;

		pClass = CBaseEntity::Instance(&pEdictList[i]);
		// Activate this entity if it's got a class & isn't dormant
		if (pClass && !(pClass->pev->flags & FL_DORMANT))
		{
			msstring Dbgstr = "Activating ";
			Dbgstr += pClass->DisplayName();
			Dbgstr += " (";
			Dbgstr += STRING(pClass->pev->targetname);
			Dbgstr += ")";

			dbg(Dbgstr);
			try
			{
				pClass->Activate();
			}
			catch (...)
			{
				MSErrorConsoleText("ServerActivate", Dbgstr);
			}
		}
		//else
		//
		//ALERT( at_console, "Can't instance %s\n", STRING(pEdictList[i].v.classname) );
		//}
	}

	// Link user messages here to make sure first client can get them...
	LinkUserMessages();

	//If the game master hasn't been created yet, create it now - Solokiller
	CBaseEntity* pGameMasterEnt = UTIL_FindEntityByString(NULL, "netname", msstring("¯") + "game_master");
	if (!pGameMasterEnt)
	{
		logfile << Logger::LOG_INFO << "Spawning game master\n";
		//TODO: this code was lifted from CScript::ScriptCmd_Create, considering refactoring - Solokiller
		CMSMonster* NewMonster = (CMSMonster*)GET_PRIVATE(CREATE_NAMED_ENTITY(MAKE_STRING("ms_npc")));
		if (NewMonster)
		{
			NewMonster->pev->origin = Vector(20000, -10000, -20000);
			NewMonster->Spawn("game_master");

			msstringlist params;
			NewMonster->CallScriptEvent("game_dynamically_created", &params);
		}
	}

	CSVGlobals::WriteScriptLog();

	logfile << Logger::LOG_INFO << "World Activate END\n";
	enddbg;
}

/*
================
PlayerPreThink

Called every frame before physics are run
================
*/
void PlayerPreThink(edict_t *pEntity)
{
	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->PreThink();
}

/*
================
PlayerPostThink

Called every frame after physics are run
================
*/

void PlayerPostThink(edict_t *pEntity)
{
	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->PostThink();
}

void ParmsNewLevel(void)
{
}

void ParmsChangeLevel(void)
{
	// retrieve the pointer to the save data
	SAVERESTOREDATA *pSaveData = (SAVERESTOREDATA *)gpGlobals->pSaveData;

	if (pSaveData)
		pSaveData->connectionCount = BuildChangeList(pSaveData->levelList, MAX_LEVEL_CONNECTIONS);
}

//
// GLOBALS ASSUMED SET:  g_ulFrameCount
//
void StartFrame(void)
{
	MSGlobals::SharedThink();

	if (gpGlobals->time > CSoundEnt::m_gSoundEnt.m_TimeLastThink + 0.3f)
		CSoundEnt::m_gSoundEnt.Think();

	if (g_pGameRules)
		g_pGameRules->Think();

	if (g_fGameOver)
		return;

	//gpGlobals->teamplay = CVAR_GET_FLOAT("teamplay");
	//g_iSkillLevel = CVAR_GET_FLOAT("skill");
	g_ulFrameCount++;
}

void ClientPrecache(void)
{
	// setup precaches always needed
	PRECACHE_SOUND("player/sprayer.wav"); // spray paint sound for PreAlpha

	PRECACHE_SOUND("player/pl_fallpain2.wav");
	PRECACHE_SOUND("player/pl_fallpain3.wav");

	PRECACHE_SOUND("player/pl_step1.wav"); // walk on concrete
	PRECACHE_SOUND("player/pl_step2.wav");
	PRECACHE_SOUND("player/pl_step3.wav");
	PRECACHE_SOUND("player/pl_step4.wav");

	PRECACHE_SOUND("common/npc_step1.wav"); // NPC walk on concrete
	PRECACHE_SOUND("common/npc_step2.wav");
	PRECACHE_SOUND("common/npc_step3.wav");
	PRECACHE_SOUND("common/npc_step4.wav");

	PRECACHE_SOUND("player/pl_metal1.wav"); // walk on metal
	PRECACHE_SOUND("player/pl_metal2.wav");
	PRECACHE_SOUND("player/pl_metal3.wav");
	PRECACHE_SOUND("player/pl_metal4.wav");

	PRECACHE_SOUND("player/pl_dirt1.wav"); // walk on dirt
	PRECACHE_SOUND("player/pl_dirt2.wav");
	PRECACHE_SOUND("player/pl_dirt3.wav");
	PRECACHE_SOUND("player/pl_dirt4.wav");

	PRECACHE_SOUND("player/pl_duct1.wav"); // walk in duct
	PRECACHE_SOUND("player/pl_duct2.wav");
	PRECACHE_SOUND("player/pl_duct3.wav");
	PRECACHE_SOUND("player/pl_duct4.wav");

	PRECACHE_SOUND("player/pl_grate1.wav"); // walk on grate
	PRECACHE_SOUND("player/pl_grate2.wav");
	PRECACHE_SOUND("player/pl_grate3.wav");
	PRECACHE_SOUND("player/pl_grate4.wav");

	PRECACHE_SOUND("player/pl_slosh1.wav"); // walk in shallow water
	PRECACHE_SOUND("player/pl_slosh2.wav");
	PRECACHE_SOUND("player/pl_slosh3.wav");
	PRECACHE_SOUND("player/pl_slosh4.wav");

	PRECACHE_SOUND("player/pl_tile1.wav"); // walk on tile
	PRECACHE_SOUND("player/pl_tile2.wav");
	PRECACHE_SOUND("player/pl_tile3.wav");
	PRECACHE_SOUND("player/pl_tile4.wav");
	PRECACHE_SOUND("player/pl_tile5.wav");

	PRECACHE_SOUND("player/pl_snow1.wav"); // walk on snow (MAR2008a)
	PRECACHE_SOUND("player/pl_snow2.wav");
	PRECACHE_SOUND("player/pl_snow3.wav");
	PRECACHE_SOUND("player/pl_snow4.wav");
	PRECACHE_SOUND("player/pl_snow5.wav"); //suspect 5 & 6 maybe a loss
	PRECACHE_SOUND("player/pl_snow6.wav");

	PRECACHE_SOUND("player/pl_swim1.wav"); // breathe bubbles
	PRECACHE_SOUND("player/pl_swim2.wav");
	PRECACHE_SOUND("player/pl_swim3.wav");
	PRECACHE_SOUND("player/pl_swim4.wav");

	PRECACHE_SOUND("player/pl_ladder1.wav"); // climb ladder rung
	PRECACHE_SOUND("player/pl_ladder2.wav");
	PRECACHE_SOUND("player/pl_ladder3.wav");
	PRECACHE_SOUND("player/pl_ladder4.wav");

	PRECACHE_SOUND("player/pl_wade1.wav"); // wade in water
	PRECACHE_SOUND("player/pl_wade2.wav");
	PRECACHE_SOUND("player/pl_wade3.wav");
	PRECACHE_SOUND("player/pl_wade4.wav");

	PRECACHE_SOUND("debris/wood1.wav"); // hit wood texture
	PRECACHE_SOUND("debris/wood2.wav");
	PRECACHE_SOUND("debris/wood3.wav");

	PRECACHE_SOUND("plats/train_use1.wav"); // use a train

	PRECACHE_SOUND("buttons/spark5.wav"); // hit computer texture
	PRECACHE_SOUND("buttons/spark6.wav");
	PRECACHE_SOUND("debris/glass1.wav");
	PRECACHE_SOUND("debris/glass2.wav");
	PRECACHE_SOUND("debris/glass3.wav");

	if (giPrecacheGrunt)
		UTIL_PrecacheOther("monster_human_grunt");

	PlayerPrecache();
}

/*
================
Sys_Error

Engine is going to shut down, allows setting a breakpoint in game .dll to catch that occasion
================
*/
void Sys_Error(const char *error_string)
{
	DBG_INPUT;
	// Default case, do nothing.  MOD AUTHORS:  Add code ( e.g., _asm { int 3 }; here to cause a breakpoint for debugging your game .dlls
	msstring Error = msstring("SYS_ERROR: ") + (error_string ? error_string : "[NO STRING]");
	//logfile << "SYS_ERROR: " << (error_string ? error_string : "[NO STRING]") << endl;
	LogCurrentLine(Error.c_str());
}

/*
===============
GetGameDescription

Returns the descriptive name of this .dll.  E.g., Half-Life, or Team Fortress 2
===============
*/
const char *GetGameDescription()
{
#if DEBUG
	static char gameDesc[64];
	sprintf(gameDesc, "MS:R - %s", "Canary");
#else
	char build[8] = {__DATE__[0], __DATE__[1], __DATE__[2], __DATE__[7], __DATE__[8], __DATE__[9], __DATE__[10]};
	static char gameDesc[64];
	sprintf(gameDesc, "MS:R - %sa", strupr(build));
#endif

	return gameDesc;
}

/*
================
PlayerCustomization

A new player customization has been registered on the server
UNDONE:  This only sets the # of frames of the spray can logo
animation right now.
================
*/
void PlayerCustomization(edict_t *pEntity, customization_t *pCust)
{
	DBG_INPUT;
	startdbg;
	entvars_t *pev = &pEntity->v;
	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE(pEntity);

	if (!pPlayer)
	{
		ALERT(at_console, "PlayerCustomization:  Couldn't get player!\n");
		return;
	}

	if (!pCust)
	{
		ALERT(at_console, "PlayerCustomization:  NULL customization!\n");
		return;
	}

	switch (pCust->resource.type)
	{
	case t_decal:
		pPlayer->SetCustomDecalFrames(pCust->nUserData2); // Second int is max # of frames.
		break;
	case t_sound:
	case t_skin:
	case t_model:
		// Ignore for now.
		break;
	default:
		ALERT(at_console, "PlayerCustomization:  Unknown customization type!\n");
		break;
	}
	enddbg;
}

/*
================
SpectatorConnect

A spectator has joined the game
================
*/
void SpectatorConnect(edict_t *pEntity)
{
	DBG_INPUT;
	entvars_t *pev = &pEntity->v;
	CBaseSpectator *pPlayer = (CBaseSpectator *)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->SpectatorConnect();
}

/*
================
SpectatorConnect

A spectator has left the game
================
*/
void SpectatorDisconnect(edict_t *pEntity)
{
	entvars_t *pev = &pEntity->v;
	CBaseSpectator *pPlayer = (CBaseSpectator *)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->SpectatorDisconnect();
}

/*
================
SpectatorConnect

A spectator has sent a usercmd
================
*/
void SpectatorThink(edict_t *pEntity)
{
	entvars_t *pev = &pEntity->v;
	CBaseSpectator *pPlayer = (CBaseSpectator *)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->SpectatorThink();
}

////////////////////////////////////////////////////////
// PAS and PVS routines for client messaging
//

/*
================
SetupVisibility

A client can have a separate "view entity" indicating that his/her view should depend on the origin of that
view entity.  If that's the case, then pViewEntity will be non-NULL and will be used.  Otherwise, the current
entity's origin is used.  Either is offset by the view_ofs to get the eye position.

From the eye position, we set up the PAS and PVS to use for filtering network messages to the client.  At this point, we could
 override the actual PAS or PVS values, or use a different origin.

NOTE:  Do not cache the values of pas and pvs, as they depend on reusable memory in the engine, they are only good for this one frame
================
*/
void SetupVisibility(edict_t *pViewEntity, edict_t *pClient, unsigned char **pvs, unsigned char **pas)
{
	Vector org;
	edict_t *pView = pClient;

	// Find the client's PVS
	if (pViewEntity)
	{
		pView = pViewEntity;
	}

	if (pClient->v.flags & FL_PROXY)
	{
		*pvs = NULL; // the spectator proxy sees
		*pas = NULL; // and hears everything
		return;
	}

	org = pView->v.origin + pView->v.view_ofs;
	if (pView->v.flags & FL_DUCKING)
	{
		org = org + (VEC_HULL_MIN - VEC_DUCK_HULL_MIN);
	}

	*pvs = ENGINE_SET_PVS((float *)&org);
	*pas = ENGINE_SET_PAS((float *)&org);
}

#include "entity_state.h"

/*
AddToFullPack

Return 1 if the entity state has been filled in for the ent and the entity will be propagated to the client, 0 otherwise

state is the server maintained copy of the state info that is transmitted to the client
a MOD could alter values copied into state to send the "host" a different look for a particular entity update, etc.
e and ent are the entity that is being added to the update, if 1 is returned
host is the player's edict of the player whom we are sending the update to
player is 1 if the ent/e is a player and 0 otherwise
pSet is either the PAS or PVS that we previous set up.  We can use it to ask the engine to filter the entity against the PAS or PVS.
we could also use the pas/ pvs that we set in SetupVisibility, if we wanted to.  Caching the value is valid in that case, but still only for the current frame
*/
//Dogg - basically decide whether or not to send an entity
int AddToFullPack(struct entity_state_s *state, int e, edict_t *ent, edict_t *host, int hostflags, int player, unsigned char *pSet)
{
	// Entities with an index greater than this will corrupt the client's heap because 
	// the index is sent with only 11 bits of precision (2^11 == 2048).
	// So we don't send them, just like having too many entities would result
	// in the entity not being sent.
	if (e >= MAX_EDICTS)
		return 0;

	//if( FBitSet( ent->v.playerclass, ENT_EFFECT_FOLLOW_ROTATE ) )
	
	auto entity = reinterpret_cast<CBaseEntity*>(GET_PRIVATE(ent));
	CBaseEntity* pEntity = GetClassPtr((CBaseEntity*)&ent->v);
	// if entity isn't flagged for force send then go through checks.
	if (!pEntity->FORCESEND)
	{
		// don't send if flagged for NODRAW and it's not the host getting the message
		if ((ent->v.effects & EF_NODRAW) != 0 && (ent != host))
			return 0;	
		
		// Ignore ents without valid / visible models
		if (!ent->v.modelindex || !STRING(ent->v.model))
			return 0;
		
		// Don't send entities with flag NOSEND or using null.mdl
		if (pEntity->NOSEND || (strcmp(STRING(ent->v.model), "models/null.mdl") == 0))
			return 0;
	
		// Don't send spectators to other players
		if ((ent->v.flags & FL_SPECTATOR) != 0 && (ent != host))
			return 0;
	
		// Ignore if not the host and not touching a PVS/PAS leaf
		// If pSet is NULL, then the test will always succeed and the entity will be added to the update
		if (ent != host && !FBitSet(ent->v.playerclass, ENT_EFFECT_FOLLOW_ROTATE))
		{
			if (!ENGINE_CHECK_VISIBILITY((const struct edict_s *)ent, pSet))
			{
				return 0;
			}
		}
	
		// Don't send entity to local client if the client says it's predicting the entity itself.
		if ((ent->v.flags & FL_SKIPLOCALHOST) != 0)
		{
			if ((hostflags & 1) != 0 && (ent->v.owner == host))
				return 0;
		}
	}

	if (0 != host->v.groupinfo)
	{
		UTIL_SetGroupTrace(host->v.groupinfo, GROUP_OP_AND);

		// Should always be set, of course
		if (0 != ent->v.groupinfo)
		{
			if (g_groupop == GROUP_OP_AND)
			{
				if (!(ent->v.groupinfo & host->v.groupinfo))
					return 0;
			}
			else if (g_groupop == GROUP_OP_NAND)
			{
				if ((ent->v.groupinfo & host->v.groupinfo) != 0)
					return 0;
			}
		}

		UTIL_UnsetGroupTrace();
	}

	/*CBaseEntity *pEntity = MSInstance( ent );
	if( pEntity )
	{
		//Skip bodyparts when overridden by armor
		if( FBitSet(pEntity->MSProperties(),ENT_BODYPART) && !pEntity->MSQuery( 0 ) )
			return 0;
	}*/

	memset(state, 0, sizeof(*state));

	// Assign index so we can track this entity from frame to frame and
	//  delta from it.
	state->number = e;
	state->entityType = ENTITY_NORMAL;

	// Flag custom entities.
	if ((ent->v.flags & FL_CUSTOMENTITY) != 0)
	{
		state->entityType = ENTITY_BEAM;
	}

	//
	// Copy state data
	//

	// Round animtime to nearest millisecond
	state->animtime = (int)(1000.0 * ent->v.animtime) / 1000.0;

	memcpy(state->origin, ent->v.origin, 3 * sizeof(float));
	memcpy(state->angles, ent->v.angles, 3 * sizeof(float));
	memcpy(state->mins, ent->v.mins, 3 * sizeof(float));
	memcpy(state->maxs, ent->v.maxs, 3 * sizeof(float));

	memcpy(state->startpos, ent->v.startpos, 3 * sizeof(float));
	memcpy(state->endpos, ent->v.endpos, 3 * sizeof(float));

	state->impacttime = ent->v.impacttime;
	state->starttime = ent->v.starttime;

	state->modelindex = ent->v.modelindex;

	state->frame = ent->v.frame;

	state->skin = ent->v.skin;
	state->effects = ent->v.effects;

	memcpy(state->vuser3, ent->v.vuser3, 3 * sizeof(float));

	// This non-player entity is being moved by the game .dll and not the physics simulation system
	//  make sure that we interpolate it's position on the client if it moves
	/*if ( !player &&
		 ent->v.animtime &&
		 ent->v.velocity[ 0 ] == 0 &&
		 ent->v.velocity[ 1 ] == 0 &&
		 ent->v.velocity[ 2 ] == 0 )
	{
		state->eflags |= EFLAG_SLERP;
	}*/

	// This replaces the above code.
	if ((ent->v.flags & FL_FLY) != 0)
	{
		state->eflags |= EFLAG_SLERP;
	}
	else
	{
		state->eflags &= ~EFLAG_SLERP;
	}

	state->eflags |= entity->m_EFlags;

	//Master Sword - interpolate arrows in the air
	//if( ent->v.movetype == MOVETYPE_TOSS && ent->v.scale == 1 )
	//	state->eflags |= EFLAG_SLERP;
	//else if( ent->v.movetype == MOVETYPE_STEP )

	//Master Sword: New: interpolate all
	//if( !FBitSet(ent->v.effects,EF_NOINTERP) )
	//state->eflags |= EFLAG_SLERP;
	//if( player )
	//	state->angles.x = -ent->v.angles.x;	//Note that players' own angles.x is not transmitted to themselves...
	//however angles.y is, and angles.x is transmitted to everyone else

	state->scale = ent->v.scale;
	state->solid = ent->v.solid;
	state->colormap = ent->v.colormap;
	state->movetype = ent->v.movetype;
	state->sequence = ent->v.sequence;
	state->framerate = ent->v.framerate;
	state->body = ent->v.body;
	
	int i;
	for (i = 0; i < 4; i++)
		state->controller[i] = ent->v.controller[i];

	for (i = 0; i < 2; i++)
		state->blending[i] = ent->v.blending[i];

	state->rendermode = ent->v.rendermode;
	state->renderamt = ent->v.renderamt;
	state->renderfx = ent->v.renderfx;
	state->rendercolor.r = ent->v.rendercolor[0];
	state->rendercolor.g = ent->v.rendercolor[1];
	state->rendercolor.b = ent->v.rendercolor[2];

	state->aiment = 0;
	if (ent->v.aiment)
		state->aiment = ENTINDEX(ent->v.aiment);

	state->owner = 0;
	if (ent->v.owner)
	{
		int owner = ENTINDEX(ent->v.owner);

		// Only care if owned by a player or using special follow
		if ((owner >= 1 && owner <= gpGlobals->maxClients))
			state->owner = owner;
	}

	if (FBitSet(ent->v.playerclass, ENT_EFFECT_FOLLOW_ROTATE)) //For Master Sword's special follow
	{
		state->origin = ent->v.vuser1;
		if (ent->v.owner)
			state->owner = ENTINDEX(ent->v.owner);
	}

	// HACK:  Somewhat...
	// Class is overridden for non-players to signify a breakable glass object ( sort of a class? )
	if ( !player )
	{
		state->playerclass = ent->v.playerclass;
	}

	// Special stuff for players only
	if (player)
	{
		memcpy(state->basevelocity, ent->v.basevelocity, 3 * sizeof(float));

		//MiB FEB2010a - MAJOR hack - Using renderfx as my tro-an horse. (undone)
		state->renderfx = ent->v.renderfx;

		//state->weaponmodel  = MODEL_INDEX( STRING( ent->v.weaponmodel ) );
		state->gaitsequence = ent->v.gaitsequence;
		state->spectator = ent->v.flags & FL_SPECTATOR;
		state->friction = ent->v.friction;

		state->gravity = ent->v.gravity;
		//		state->team			= ent->v.team;
		//		state->playerclass  = ent->v.playerclass;
		state->usehull = (ent->v.flags & FL_DUCKING) ? 1 : 0;
		state->health = ent->v.health;

		//fuser1 = The movement speed at which the gait anim should play at normal fps
		//         If the speed is different, then play the fps at a ratio of the current speed to this normal speed
		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)&ent->v);
		state->fuser1 = pPlayer->m_GaitFramerateGauge;
	}

	if (pEntity && pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
		state->eflags |= EFLAG_FLESH_SOUND;
	else
		state->eflags &= ~EFLAG_FLESH_SOUND;

	return 1;
}

// defaults for clientinfo messages
#define DEFAULT_VIEWHEIGHT 28

/*
===================
CreateBaseline

Creates baselines used for network encoding, especially for player data since players are not spawned until connect time.
===================
*/
void CreateBaseline(int player, int eindex, struct entity_state_s *baseline, struct edict_s *entity, int playermodelindex, vec3_t player_mins, vec3_t player_maxs)
{
	DBG_INPUT;
	startdbg;
	dbg("CreateBaseline - Begin");
	baseline->origin = entity->v.origin;
	baseline->angles = entity->v.angles;
	baseline->frame = entity->v.frame;
	baseline->skin = (short)entity->v.skin;

	// render information
	baseline->rendermode = (byte)entity->v.rendermode;
	baseline->renderamt = (byte)entity->v.renderamt;
	baseline->rendercolor.r = (byte)entity->v.rendercolor[0];
	baseline->rendercolor.g = (byte)entity->v.rendercolor[1];
	baseline->rendercolor.b = (byte)entity->v.rendercolor[2];
	baseline->renderfx = (byte)entity->v.renderfx;

	if (player)
	{
		//baseline->renderfx = (entity->v.renderfx * 256) + entity->v.body; //MIB MAR2010_09 Armor Fix Fix Fix Fix (undone)
		baseline->renderfx = entity->v.renderfx;
		baseline->mins = player_mins;
		baseline->maxs = player_maxs;

		baseline->colormap = eindex;
		baseline->modelindex = playermodelindex;
		baseline->friction = 1.0;
		baseline->movetype = MOVETYPE_WALK;

		baseline->scale = entity->v.scale;
		baseline->solid = SOLID_SLIDEBOX;
		baseline->framerate = 1.0;
		baseline->gravity = 1.0;
		//baseline->gaitsequence	= 0;
	}
	else
	{
		baseline->mins = entity->v.mins;
		baseline->maxs = entity->v.maxs;

		baseline->colormap = 0;
		baseline->modelindex = entity->v.modelindex; //SV_ModelIndex(pr_strings + entity->v.model);
		baseline->movetype = entity->v.movetype;

		baseline->scale = entity->v.scale;
		baseline->solid = entity->v.solid;
		baseline->framerate = entity->v.framerate;
		baseline->gravity = entity->v.gravity;
	}

	enddbg;
}

typedef struct
{
	char name[32];
	int field;
} entity_field_alias_t;

enum
{
	FIELD_ORIGIN0 = 0,
	FIELD_ORIGIN1,
	FIELD_ORIGIN2,
	FIELD_ANGLES0,
	FIELD_ANGLES1,
	FIELD_ANGLES2,
	FIELD_MINS0,
	FIELD_MINS1,
	FIELD_MINS2,
	FIELD_MAXS0,
	FIELD_MAXS1,
	FIELD_MAXS2,
};

static entity_field_alias_t entity_field_alias[] =
	{
		{"origin[0]", 0},
		{"origin[1]", 0},
		{"origin[2]", 0},
		{"angles[0]", 0},
		{"angles[1]", 0},
		{"angles[2]", 0},
		{"mins[0]", 0},
		{"mins[1]", 0},
		{"mins[2]", 0},
		{"maxs[0]", 0},
		{"maxs[1]", 0},
		{"maxs[2]", 0},
};

void Entity_FieldInit(struct delta_s *pFields)
{
	int EntityFields = ARRAYSIZE(entity_field_alias);
	for (int i = 0; i < EntityFields; i++)
		entity_field_alias[i].field = DELTA_FINDFIELD(pFields, entity_field_alias[i].name);
}

/*
==================
Entity_Encode

Callback for sending entity_state_t info over network.
FIXME:  Move to script
==================
*/
//Dogg - decides WHAT NOT TO SEND about a generic (non-player) entity
void Entity_Encode(struct delta_s *pFields, const unsigned char *from, const unsigned char *to)
{
	entity_state_t *f, *t;

	int localplayer = 0;
	static int initialized = 0;

	if (!initialized)
	{
		Entity_FieldInit(pFields);
		initialized = 1;
	}

	f = (entity_state_t *)from;
	t = (entity_state_t *)to;

	//Unset this after the special follow is first sent... so the server's constantly updating angles don't override the client offset angles
	if (FBitSet(t->playerclass, ENT_EFFECT_FOLLOW_ROTATE) && t->owner == f->owner)
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);

		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES2].field);

		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_MINS0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_MINS1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_MINS2].field);

		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_MAXS0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_MAXS1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_MAXS2].field);
	}

	// Never send origin to local player, it's sent with more resolution in clientdata_t structure
	localplayer = (t->number - 1) == ENGINE_CURRENT_PLAYER();
	if (localplayer)
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}

	if ((t->impacttime != 0) && (t->starttime != 0))
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);

		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES2].field);
	}

	if ((t->movetype == MOVETYPE_FOLLOW) &&
		(t->aiment != 0))
	{
		//Master Sword.  Follow entities are usually packs or held items. Don't send much data
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);

		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ANGLES2].field);

		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_MINS0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_MINS1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_MINS2].field);

		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_MAXS0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_MAXS1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_MAXS2].field);
	}
	else if (t->aiment != f->aiment)
	{
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
}

static entity_field_alias_t player_field_alias[] =
	{
		{"origin[0]", 0},
		{"origin[1]", 0},
		{"origin[2]", 0},
};

void Player_FieldInit(struct delta_s *pFields)
{
	player_field_alias[FIELD_ORIGIN0].field = DELTA_FINDFIELD(pFields, player_field_alias[FIELD_ORIGIN0].name);
	player_field_alias[FIELD_ORIGIN1].field = DELTA_FINDFIELD(pFields, player_field_alias[FIELD_ORIGIN1].name);
	player_field_alias[FIELD_ORIGIN2].field = DELTA_FINDFIELD(pFields, player_field_alias[FIELD_ORIGIN2].name);
}

/*
==================
Player_Encode

Callback for sending entity_state_t for players info over network.
==================
*/
//Dogg - decides WHAT NOT to send about a player entity
void Player_Encode(struct delta_s *pFields, const unsigned char *from, const unsigned char *to)
{
	entity_state_t *f, *t;
	int localplayer = 0;
	static int initialized = 0;

	if (!initialized)
	{
		Player_FieldInit(pFields);
		initialized = 1;
	}

	f = (entity_state_t *)from;
	t = (entity_state_t *)to;

	// Never send origin to local player, it's sent with more resolution in clientdata_t structure
	localplayer = (t->number - 1) == ENGINE_CURRENT_PLAYER();
	if (localplayer)
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}

	if ((t->movetype == MOVETYPE_FOLLOW) &&
		(t->aiment != 0))
	{
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
	else if (t->aiment != f->aiment)
	{
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN0].field);
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN1].field);
		DELTA_SETBYINDEX(pFields, entity_field_alias[FIELD_ORIGIN2].field);
	}
}

#define CUSTOMFIELD_ORIGIN0 0
#define CUSTOMFIELD_ORIGIN1 1
#define CUSTOMFIELD_ORIGIN2 2
#define CUSTOMFIELD_ANGLES0 3
#define CUSTOMFIELD_ANGLES1 4
#define CUSTOMFIELD_ANGLES2 5
#define CUSTOMFIELD_SKIN 6
#define CUSTOMFIELD_SEQUENCE 7
#define CUSTOMFIELD_ANIMTIME 8

entity_field_alias_t custom_entity_field_alias[] =
	{
		{"origin[0]", 0},
		{"origin[1]", 0},
		{"origin[2]", 0},
		{"angles[0]", 0},
		{"angles[1]", 0},
		{"angles[2]", 0},
		{"skin", 0},
		{"sequence", 0},
		{"animtime", 0},
};

void Custom_Entity_FieldInit(struct delta_s *pFields)
{
	custom_entity_field_alias[CUSTOMFIELD_ORIGIN0].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN0].name);
	custom_entity_field_alias[CUSTOMFIELD_ORIGIN1].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN1].name);
	custom_entity_field_alias[CUSTOMFIELD_ORIGIN2].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN2].name);
	custom_entity_field_alias[CUSTOMFIELD_ANGLES0].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES0].name);
	custom_entity_field_alias[CUSTOMFIELD_ANGLES1].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES1].name);
	custom_entity_field_alias[CUSTOMFIELD_ANGLES2].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES2].name);
	custom_entity_field_alias[CUSTOMFIELD_SKIN].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_SKIN].name);
	custom_entity_field_alias[CUSTOMFIELD_SEQUENCE].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_SEQUENCE].name);
	custom_entity_field_alias[CUSTOMFIELD_ANIMTIME].field = DELTA_FINDFIELD(pFields, custom_entity_field_alias[CUSTOMFIELD_ANIMTIME].name);
}

/*
==================
Custom_Encode

Callback for sending entity_state_t info ( for custom entities ) over network.
FIXME:  Move to script
==================
*/
void Custom_Encode(struct delta_s *pFields, const unsigned char *from, const unsigned char *to)
{
	entity_state_t *f, *t;
	int beamType;
	static int initialized = 0;

	if (!initialized)
	{
		Custom_Entity_FieldInit(pFields);
		initialized = 1;
	}

	f = (entity_state_t *)from;
	t = (entity_state_t *)to;

	beamType = t->rendermode & 0x0f;

	if (beamType != BEAM_POINTS && beamType != BEAM_ENTPOINT)
	{
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN0].field);
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN1].field);
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ORIGIN2].field);
	}

	if (beamType != BEAM_POINTS)
	{
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES0].field);
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES1].field);
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ANGLES2].field);
	}

	if (beamType != BEAM_ENTS && beamType != BEAM_ENTPOINT)
	{
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_SKIN].field);
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_SEQUENCE].field);
	}

	// animtime is compared by rounding first
	// see if we really shouldn't actually send it
	if ((int)f->animtime == (int)t->animtime)
	{
		DELTA_UNSETBYINDEX(pFields, custom_entity_field_alias[CUSTOMFIELD_ANIMTIME].field);
	}
}

/*
=================
RegisterEncoders

Allows game .dll to override network encoding of certain types of entities and tweak values, etc.
=================
*/
void RegisterEncoders(void)
{
	DBG_INPUT;
	DELTA_ADDENCODER("Entity_Encode", Entity_Encode);
	DELTA_ADDENCODER("Custom_Encode", Custom_Encode);
	DELTA_ADDENCODER("Player_Encode", Player_Encode);
}

int GetWeaponData(struct edict_s *player, struct weapon_data_s *info)
{
	DBG_INPUT;
	memset(info, 0, 32 * sizeof(weapon_data_t));
	return 1; //1
}

/*
=================
UpdateClientData

Data sent to current client only
engine sets cd to 0 before calling.
=================
*/
void UpdateClientData(const struct edict_s *ent, int sendweapons, struct clientdata_s *cd)
{
	DBG_INPUT;
	startdbg;

	cd->flags = ent->v.flags;
	cd->health = ent->v.health;

	cd->viewmodel = MODEL_INDEX(STRING(ent->v.viewmodel)); //commenting out causes a crash?
	cd->waterlevel = ent->v.waterlevel;
	cd->watertype = ent->v.watertype;
	cd->weapons = ent->v.weapons;

	// Vectors
	cd->origin = ent->v.origin;
	cd->velocity = ent->v.velocity;
	cd->view_ofs = ent->v.view_ofs;
	cd->punchangle = ent->v.punchangle;

	cd->bInDuck = ent->v.bInDuck;
	cd->flTimeStepSound = ent->v.flTimeStepSound;
	cd->flDuckTime = ent->v.flDuckTime;
	cd->flSwimTime = ent->v.flSwimTime;
	cd->waterjumptime = ent->v.teleport_time;

	 strncpy(cd->physinfo,  ENGINE_GETPHYSINFO(ent), sizeof(cd->physinfo) );

	cd->maxspeed = ent->v.maxspeed;
	cd->fov = ent->v.fov;
	cd->weaponanim = ent->v.weaponanim;

	cd->pushmsec = ent->v.pushmsec;

	//Master Sword --

	CBasePlayer *pPlayer = (CBasePlayer *)MSInstance((edict_t *)ent);
	if (pPlayer)
		cd->iuser3 = pPlayer->m_StatusFlags;
	else
		cd->iuser3 = 0;
	//---------------

	enddbg;
}

/*
=================
CmdStart

We're about to run this usercmd for the specified player.  We can set up groupinfo and masking here, etc.
This is the time to examine the usercmd for anything extra.  This call happens even if think does not.
=================
*/
void CmdStart(const edict_t *player, const struct usercmd_s *cmd, unsigned int random_seed)
{
	DBG_INPUT;
	startdbg;
	CBasePlayer *pPlayer = (CBasePlayer *)CBasePlayer::Instance((entvars_t *)&player->v);

	if (!pPlayer)
		return;

	if (pPlayer->pev->groupinfo != 0)
	{
		UTIL_SetGroupTrace(pPlayer->pev->groupinfo, GROUP_OP_AND);
	}

	pPlayer->random_seed = random_seed;

	enddbg;
}

/*
=================
CmdEnd

Each cmdstart is exactly matched with a cmd end, clean up any group trace flags, etc. here
=================
*/
void CmdEnd(const edict_t *player)
{
	DBG_INPUT;
	startdbg;
	entvars_t *pev = (entvars_t *)&player->v;
	CBasePlayer *pl = (CBasePlayer *)CBasePlayer::Instance(pev);

	if (!pl)
		return;
	if (pl->pev->groupinfo != 0)
	{
		UTIL_UnsetGroupTrace();
	}

	enddbg;
}

/*
================================
ConnectionlessPacket

 Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
  size of the response_buffer, so you must zero it out if you choose not to respond.
================================
*/
int ConnectionlessPacket(const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size)
{
	DBG_INPUT;
	// Parse stuff from args
	int max_buffer_size = *response_buffer_size;

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}

/*
================================
GetHullBounds

  Engine calls this to enumerate player collision hulls, for prediction.  Return 0 if the hullnumber doesn't exist.
================================
*/
int GetHullBounds(int hullnumber, float *mins, float *maxs)
{
	return PM_GetHullBounds(hullnumber, mins, maxs) ? 1 : 0;
}

/*
================================
CreateInstancedBaselines

Create pseudo-baselines for items that aren't placed in the map at spawn time, but which are likely
to be created during play ( e.g., grenades, ammo packs, projectiles, corpses, etc. )
================================
*/
void CreateInstancedBaselines(void)
{
	DBG_INPUT;
	//int iret = 0;
	//entity_state_t state;

	//memset( &state, 0, sizeof( state ) );

	// Create any additional baselines here for things like grendates, etc.
	// iret = ENGINE_INSTANCE_BASELINE( pc->pev->classname, &state );

	// Destroy objects.
	//UTIL_Remove( pc );
}

/*
================================
InconsistentFile

One of the ENGINE_FORCE_UNMODIFIED files failed the consistency check for the specified player
 Return 0 to allow the client to continue, 1 to force immediate disconnection ( with an optional disconnect message of up to 256 characters )
================================
*/
int InconsistentFile(const edict_t *player, const char *filename, char *disconnect_message)
{
	DBG_INPUT;
	// Server doesn't care?
	//if ( CVAR_GET_FLOAT( "mp_consistency" ) != 1 )
	//	return 0;

	// Default behavior is to kick the player
	//	sprintf( disconnect_message, "Server is enforcing file consistency for %s\n", filename );
	sprintf(disconnect_message, "DO NOT MODIFY MASTER SWORD FILES\n"
		"EITHER YOU OR THE SERVER HAS MODIFIED AN IMPORTANT FILE\n"
		"TRY ANOTHER SERVER OR, IF YOU HAVE MODIFIED FILES, REINSTALL\n");

	// Kick now with specified disconnect message.
	return 1;
}

/*
================================
AllowLagCompensation

 The game .dll should return 1 if lag compensation should be allowed ( could also just set
  the sv_unlag cvar.
 Most games right now should return 0, until client-side weapon prediction code is written
  and tested for them ( note you can predict weapons, but not do lag compensation, too,
  if you want.
================================
*/
int AllowLagCompensation(void)
{
	DBG_INPUT;
	return 0;
}
