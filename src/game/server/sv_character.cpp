//
//  Character functions for the Server
//

#include "inc_weapondefs.h"
#include "stats/stats.h"
#include "global.h"
#include "logger.h"
#include "mscharacter.h"
#include "magic.h"
#include "script.h"
#include "fn/FNSharedDefs.h"

#ifndef _WIN32
#include "sys/io.h"
#endif

//NOTENOTE: remove this when char corruption bug is fixed - Solokiller 5/10/2017
#include "game.h"
//END NOTE

void CBasePlayer::CreateChar(createchar_t &CharData)
{
	//Create a new character, using the options the user specified.
	//A custom savedata_t struct can be created for the new
	//character and passed to SaveAll, but any items the character
	//starts with need to be added to Gear

	savedata_t Data;
	memset(&Data, 0, sizeof(savedata_t));

	strncpy(Data.Name, CharData.Name, sizeof(Data.Name));
	strncpy(Data.Race, "Human", sizeof(Data.Race)); // LEGACY
	strncpy(Data.MapName, MSGlobals::MapName, sizeof(Data.MapName));
	Data.Gender = CharData.Gender;
	Data.Gold = MSGlobals::DefaultGold;

#ifndef VALVE_DLL
	MSCLGlobals::RemoveAllEntities();
#else
	RemoveAllItems(false, true);
#endif
	//Give 1 skill to each stat
	for (int i = 0; i < m_Stats.size(); i++)
	{
		CStat &Stat = m_Stats[i];
		if (Stat.m_SubStats.size() == 1) //Parry - Only give 1 to proficiency
			Stat.m_SubStats[0].Value = 1;
		else if (Stat.m_SubStats.size() <= STATPROP_TOTAL) //Weapon Skills - Give 1 to power
			Stat.m_SubStats[STATPROP_POWER].Value = 1;
		else if (Stat.m_SubStats.size() > STATPROP_TOTAL) //Spellcasting - Give 1 to each spell category
			for (int r = 0; r < Stat.m_SubStats.size(); r++)
				Stat.m_SubStats[r].Value = 1;
	}

	//Give free items.  Wear any packs
	for (int i = 0; i < MSGlobals::DefaultFreeItems.size(); i++)
	{
		CGenericItem *pStartingItem = NewGenericItem(MSGlobals::DefaultFreeItems[i]);
		if (!pStartingItem)
			continue;

		if (FBitSet(pStartingItem->MSProperties(), ITEM_WEARABLE))
		{
			//Wear packs
			//m_fClientInitiated = true;				//Dont send a client update
			if (!AddItem(pStartingItem, false, true) || !pStartingItem->WearItem())
				pStartingItem->SUB_Remove();
			//m_fClientInitiated = false;
		}
		else if (FBitSet(pStartingItem->MSProperties(), ITEM_SPELL))
		{
			LearnSpell(pStartingItem->ItemName, true);
			pStartingItem->SUB_Remove();
		}
		else
		{
			//Normal item
			if (!AddItem(pStartingItem, true, true))
				pStartingItem->SUB_Remove();
		}
	}

	//Put the chosen weapon in the player's right hand
	CGenericItem *pStartingItem = NewGenericItem(CharData.Weapon);

	if (FBitSet(pStartingItem->MSProperties(), ITEM_SPELL))
		//It's a spell
	{
		LearnSpell(pStartingItem->ItemName, false);
		pStartingItem->SUB_Remove();
	}
	else
	{
		if (!AddItem(pStartingItem, true, true))
			pStartingItem->SUB_Remove();
	}

	m_CharacterNum = CharData.iChar;
	m_CharacterState = CHARSTATE_LOADED;	 //Temporary, so the save will succeed
	MSChar_Interface::SaveChar(this, &Data); //Save the new character

	m_CharacterState = CHARSTATE_UNLOADED; //Created a new character... this one is no longer valid

	//Clean up
	RemoveAllItems(false, true);

#ifdef VALVE_DLL
	//Update player's character list, so the new char is sent down to client
	if (!FNShared::IsEnabled())
		PreLoadChars();
#endif
}

bool DeleteChar(CBasePlayer *pPlayer, int iCharacter)
{
#ifdef VALVE_DLL
	if (FNShared::IsEnabled())
	{
		FNShared::DeleteCharacter(pPlayer, iCharacter);
		return true;
	}
#endif

	const char *pszCharFileName = GetSaveFileName(iCharacter, pPlayer);
	int ret = remove(pszCharFileName);	  //Delete savefile
	remove(BACKUP_NAME(pszCharFileName)); //Delete backup

	//Update player's char list
	if (iCharacter < MAX_CHARSLOTS)
		pPlayer->m_CharInfo[iCharacter].Status = CDS_NOTFOUND;

	return (!ret) ? true : false;
}

void MSChar_Interface::AutoSave(CBasePlayer* pPlayer)
{
	if (gpGlobals->time <= pPlayer->m_TimeNextSave) return;

	SaveChar(pPlayer, NULL); // Don't auto save too often when using FN.
	pPlayer->m_TimeNextSave = gpGlobals->time + (FNShared::IsEnabled() ? RANDOM_FLOAT(4.0f, 8.0f) : 3.0f);
}

//
//  Read chunks from raw char data
//

static char cTemp[MSSTRING_SIZE];

void chardata_t::ReadMaps1(byte DataID, CPlayer_DataBuffer &m_File)
{
	startdbg;
	if (DataID == CHARDATA_MAPSVISITED1)
	{
		//Read Maps Visited
		//Must come DIRECTLY after reading the savedata_t Data
		dbg("Read Stats");
		int Maps = 0;
		m_File.ReadInt(Maps); //[INT]
		m_VisitedMaps.clear();
		for (int m = 0; m < Maps; m++)
		{
			m_File.ReadString(cTemp, MSSTRING_SIZE); //[STRING]
			m_VisitedMaps.add(cTemp);
		}
	}
	enddbg;
}

void chardata_t::ReadSkills1(byte DataID, CPlayer_DataBuffer &m_File)
{
	if (DataID == CHARDATA_SKILLS1)
	{
		// Read skills
		CStat::InitStatList(m_Stats);

		CStat* pStat = NULL;
		CSubStat* pSubStat = NULL;

		byte Stats = 0;
		m_File.ReadByte(Stats);

		for (int i = 0; i < Stats; i++)
		{
			pStat = GetStat(i);
			byte SubStats = 0;
			m_File.ReadByte(SubStats);

			for (int r = 0; r < SubStats; r++)
			{
				short Value = 0;
				int Exp = 0;
				m_File.ReadShort(Value); //[SHORT]
				m_File.ReadInt(Exp);	 //[INT]

				pSubStat = (pStat ? pStat->GetSubStat(r) : NULL);

				// Handle legacy loading, hacky & messy:
				if ((Version == SAVECHAR_VERSION_MSC) && (i == SKILL_SPELLCASTING))
				{
					switch (r)
					{
					case 5: // Used to be DIVINATION
						pSubStat = (pStat ? pStat->GetSubStat(STAT_MAGIC_DIVINATION) : NULL);
						break;

					case 6: // Used to be AFFLICTION
						pSubStat = (pStat ? pStat->GetSubStat(STAT_MAGIC_AFFLICTION) : NULL);
						break;
					}
				}

				if (pStat && pSubStat)
				{
					pSubStat->Value = Value;
					pSubStat->Exp = Exp;
				}
			}
		}

		// MiB JUL2010_02 - Hacky, but if we add more stats and we load a character that doesn't have said stat in the file
		//		we set it to the default new stat value (level 0 in Prof and Balance, 1 in Power)
		int StatsRemaining = (m_Stats.size() - Stats);
		for (int i = 0; i < StatsRemaining; i++)
		{
			CStat& Stat = m_Stats[i + Stats];
			Stat.m_SubStats[Stat.m_SubStats.size() - 1].Value = 1;
		}
	}
}

void chardata_t::ReadSpells1(byte DataID, CPlayer_DataBuffer &m_File)
{
	if (DataID == CHARDATA_SPELLS1)
	{
		//Read Magic spells
		byte Spells = 0;
		m_File.ReadByte(Spells); //[BYTE]
		for (int s = 0; s < Spells; s++)
		{
			m_File.ReadString(cTemp, MSSTRING_SIZE); //[STRING]
			m_Spells.add(cTemp);
		}
	}
}

void chardata_t::ReadItems1(byte DataID, CPlayer_DataBuffer &m_File)
{
	if (DataID == CHARDATA_ITEMS1 || DataID == CHARDATA_ITEMS2)
	{
		//Read Items
		byte GearItems = 0;
		m_File.ReadByte(GearItems); //[SHORT]
		for (int i = 0; i < GearItems; i++)
		{
			genericitem_full_t Item;
			if (!ReadItem1(DataID, m_File, Item)) //[X ITEMS]
				continue;

			m_Items.add(Item);
		}
	}
}

void chardata_t::ReadStorageItems1(byte DataID, CPlayer_DataBuffer &m_File)
{
	if (DataID == CHARDATA_STORAGE1)
	{
		//Read storage items

		short Storages = 0;
		m_File.ReadShort(Storages); //[SHORT]
		for (int i = 0; i < Storages; i++)
		{
			storage_t Storage;
			m_File.ReadString(Storage.Name, MSSTRING_SIZE); //[X STRINGS]

			short Items = 0;
			m_File.ReadShort(Items); //[X SHORTS]
			for (int i = 0; i < Items; i++)
			{
				genericitem_full_t Item;
				if (!ReadItem1(DataID, m_File, Item)) //[Y ITEMS]
					continue;

				Storage.Items.add(Item);
			}

			m_Storages.add(Storage);
		}
	}
}

void chardata_t::ReadCompanions1(byte DataID, CPlayer_DataBuffer &m_File)
{
	if (DataID == CHARDATA_COMPANIONS1)
	{
		//Read Companions
		short Companions = 0;
		m_File.ReadShort(Companions); //[SHORT]
		for (int c = 0; c < Companions; c++)
		{
			companion_t &Companion = m_Companions.add(companion_t());
			m_File.ReadString(cTemp, MSSTRING_SIZE); //[STRING]
			Companion.ScriptName = cTemp;
			Companion.Active = false;

			//Read the saved variables
			short Vars = 0;
			m_File.ReadShort(Vars); //[SHORT]
			for (int var = 0; var < Vars; var++)
			{
				m_File.ReadString(cTemp, MSSTRING_SIZE); //[STRING]
				Companion.SaveVarName.add(cTemp);
				m_File.ReadString(cTemp, MSSTRING_SIZE); //[STRING]
				Companion.SaveVarValue.add(cTemp);
			}
		}
	}
}

void chardata_t::ReadHelpTips1(byte DataID, CPlayer_DataBuffer &m_File)
{
	if (DataID == CHARDATA_HELPTIPS1)
	{
		//Read Help tips
		short HelpTips = 0;
		m_File.ReadShort(HelpTips); //[SHORT]
		m_ViewedHelpTips.clear();
		for (int t = 0; t < HelpTips; t++)
		{
			m_File.ReadString(cTemp, MSSTRING_SIZE); //[STRING]
			m_ViewedHelpTips.add(cTemp);
		}
	}
}

void chardata_t::ReadQuests1(byte DataID, CPlayer_DataBuffer &m_File)
{
	if (DataID == CHARDATA_QUESTS1)
	{
		//Read Quests
		int Quests = 0;
		m_File.ReadInt(Quests); //[INT]
		m_Quests.clear();
		for (int q = 0; q < Quests; q++)
		{
			quest_t Quest;
			m_File.ReadString(cTemp, MSSTRING_SIZE); //[STRING]
			Quest.Name = cTemp;
			m_File.ReadString(cTemp, MSSTRING_SIZE); //[STRING]
			Quest.Data = cTemp;
			m_Quests.add(Quest);
		}
	}
}

void chardata_t::ReadQuickSlots1(byte DataID, CPlayer_DataBuffer &m_File)
{
	if (DataID == CHARDATA_QUICKSLOTS1)
	{
		//Read Quickslots
		byte QuickSlots = 0;
		m_File.ReadByte(QuickSlots); //[BYTE]
		m_QuickSlots.clear();
		for (int q = 0; q < QuickSlots; q++)
		{
			quickslot_t QuickSlot;
			byte Type = 0;
			m_File.ReadByte(Type); //[BYTE]

			if (Type) //Something in this slot
			{
				int OldID = -1;
				m_File.ReadInt(OldID); //[INT]

				QuickSlot.Active = true;
				QuickSlot.Type = (quickslottype_e)(Type - 1); //Slot type is offset by 1
				QuickSlot.ID = OldID;
			}
			else //Nothing in this slot
			{
				QuickSlot.Active = false;
			}

			m_QuickSlots.add(QuickSlot);
		}
	}
}

bool chardata_t::ReadItem1(byte DataID, CPlayer_DataBuffer &Data, genericitem_full_t &outItem)
{
	clrmem(outItem);

	char cTemp[128];
	Data.ReadString(cTemp, sizeof(cTemp)); //[STRING]
	if (!cTemp || !cTemp[0])
		return false;

	outItem.Name = cTemp;	
	msstringstringhash::iterator iAlias = CGenericItemMgr::mItemAlias.find(outItem.Name);
	if (iAlias != CGenericItemMgr::mItemAlias.end())	
		outItem.Name = iAlias->second;	

	//It is now possible to read an item from file correctly, but not be able to spawn that item.
	//Be sure that, even if the item can't be created, all of the item's data is read from file properly

	short Properties = 0;
	Data.ReadShort(Properties); //[SHORT]

	outItem.Properties = Properties;

	short Location = 0;
	Data.ReadShort(Location); //[SHORT]
	outItem.Location = Location;

	byte Hand = 0;
	Data.ReadByte(Hand); //[BYTE]
	outItem.Hand = Hand;

	if (DataID == CHARDATA_ITEMS2)
	{
		int LastID = 0;
		Data.ReadInt(LastID); //[INT]
		outItem.ID = LastID;
	}

	if (FBitSet(Properties, ITEM_PERISHABLE) ||
		FBitSet(Properties, ITEM_DRINKABLE))
	{
		short Quality = 0, MaxQuality = 0;
		Data.ReadShort(Quality);	//[SHORT]
		Data.ReadShort(MaxQuality); //[SHORT]

		outItem.Quality = Quality;
		outItem.MaxQuality = MaxQuality;
	}

	if (FBitSet(Properties, ITEM_GROUPABLE))
	{
		short GroupAmt = 0;
		Data.ReadShort(GroupAmt); //[SHORT]

		outItem.Quantity = GroupAmt;
	}

	if (FBitSet(Properties, ITEM_CONTAINER))
	{
		short iItemCount = 0;
		Data.ReadShort(iItemCount); //[SHORT]

		genericitem_full_t PackItem;
		for (int i = 0; i < iItemCount; i++)
		{
			bool Success = ReadItem1(DataID, Data, PackItem);
			if (!Success)
			{
				MSErrorConsoleText("ReadItem()", "Bad item in container, skipping...");
				continue;
			}
			outItem.ContainerItems.add(PackItem);

			/*if( pItem )
				pPackItem->PutInPack( pItem );
				else
				//If I couldn't spawn this pack, drop the item that's supposed to be inside it
				pPackItem->Drop( 3, (Vector &)g_vecZero, (Vector &)g_vecZero, (Vector &)g_vecZero );*/
		}
	}

	return true;
}

//
// Save Character
// ==============

void WriteItem(CPlayer_DataBuffer &gFile, genericitem_full_t &Item);

//If pData != NULL, then this is a new char
void MSChar_Interface::SaveChar(CBasePlayer *pPlayer, savedata_t *pData)
{
	//#ifndef VALVE_DLL
	//	if( MSGlobals::ServerSideChar )
	//		return;
	//#endif

	//Can I save right now?
	if (pPlayer->m_CharacterState == CHARSTATE_UNLOADED /*||	//Can't save if no character is created
		MSGlobals::GameType != GAMETYPE_ADVENTURE*/
		)												//If the gametype isn't adventure, we can't save no matter what.
	{
		return;
	}

	//MiB JUL2010_13 - Don't save in dev mode
	if (MSGlobals::DevModeEnabled)
		return;

	//FEB2015_25 Thothie - don't save if <15 hp (char delete bug workaround)
	if (pPlayer->MaxHP() < 15)
		return;

	//Add this map to the list of maps visited
	if (!HasVisited(MSGlobals::MapName, pPlayer->m_Maps))
		pPlayer->m_Maps.add(MSGlobals::MapName);

	const char *pszFileName;
	//#ifdef VALVE_DLL
	pszFileName = GetSaveFileName(pPlayer->m_CharacterNum, pPlayer);
	//#else
	//	pszFileName = GetSaveFileName( pPlayer->m_CharacterNum );
	//#endif

	//if( fVerbose ) Print( "Saving to file: %s\n", pszFileName );

	CPlayer_DataBuffer gFile(1 << 16);

	//Initialize
	savedata_t Data;
	memset(&Data, 0, sizeof(savedata_t));
	if (pData)
		memcpy(&Data, pData, sizeof(savedata_t));

	//Copy the data
	Data.Version = SAVECHAR_VERSION;
	//strcpy( Data.MapName, (pPlayer->iHP > 0) ? g_MapName : "" );

	if (!pData)
	{
		strncpy(Data.Name, pPlayer->m_DisplayName, sizeof(Data.Name)); // Store actual character name (DisplayName() is servername, and will have a (#) at the end if there are duplicates on the server)
		strncpy(Data.Race, "Human", sizeof(Data.Race)); // LEGACY
		strncpy(Data.Party, pPlayer->GetPartyName(), sizeof(Data.Party));
		Data.PartyID = pPlayer->GetPartyID();

		strncpy(Data.MapName, MSGlobals::MapName, sizeof(Data.MapName));

		strncpy(Data.OldTrans, pPlayer->m_OldTransition, 32);
		strncpy(Data.NextMap, pPlayer->m_NextMap, 32);
		strncpy(Data.NewTrans, pPlayer->m_NextTransition, 32);

		Data.IsElite = pPlayer->m_fIsElite;
		Data.Gold = pPlayer->m_Gold;
		Data.MaxHP = pPlayer->m_MaxHP;
		Data.MaxMP = pPlayer->m_MaxMP;
		if (pPlayer->pev->deadflag == DEAD_NO)
		{
			Data.HP = pPlayer->m_HP;
			Data.MP = pPlayer->m_MP;
		}
		else
		{
			//Tried to save while dead.
			//Set Health/Mana to 0 to indicate this
			Data.HP = Data.MP = 0.0;
			//fDead = true;
			//pPlayer->iHP = 1.0;
		}

		//Data.Origin = LastGoodPos;
		//Data.Angles = LastGoodAng;
		//Print( "Save: Data.Origin: %f %f %f\n", Data.Origin.x, Data.Origin.y, Data.Origin.z );
		//Data.SayType = pPlayer->m_SayType;
		Data.Gender = pPlayer->m_Gender;
		Data.PlayerKills = pPlayer->m_PlayersKilled;
		Data.TimeWaitedToForgetKill = pPlayer->m_TimeWaitedToForgetKill;
		//#ifdef VALVE_DLL
		strncpy(Data.SteamID, GETPLAYERAUTHID(pPlayer->edict()), 32);
		//#else
		//	strncpy( Data.SteamID, MSCLGlobals::AuthID, 32 );
		//#endif
	}

	gFile.WriteByte(CHARDATA_HEADER1);
	gFile.Write(&Data, sizeof(savedata_t)); //[VAR] Player Info

	//Save Maps Visited
	//Must come just after writing savedata_t Data
	gFile.WriteByte(CHARDATA_MAPSVISITED1); //[BYTE - CHUNK - MAPS VISITED]
	gFile.WriteInt(pPlayer->m_Maps.size()); //[INT]
	for (int m = 0; m < pPlayer->m_Maps.size(); m++)
		gFile.WriteString(pPlayer->m_Maps[m]); //[STRING]

	//Save skills
	gFile.WriteByte(CHARDATA_SKILLS1); //[BYTE - CHUNK - STATS]
	statlist &StatList = pPlayer->m_Stats;
	gFile.WriteByte(StatList.size()); //[BYTE]
	for (int i = 0; i < StatList.size(); i++)
	{
		CStat &Stat = StatList[i];
		gFile.WriteByte(Stat.m_SubStats.size()); //[BYTE]
		for (int r = 0; r < Stat.m_SubStats.size(); r++)
		{
			CSubStat &SubStat = Stat.m_SubStats[r];
			gFile.WriteShort(SubStat.Value); //[SHORT]
			gFile.WriteInt(SubStat.Exp);	 //[INT]
		}
	}

	//Save magic spells
	spellgroup_v &SpellList = pPlayer->m_SpellList;
	gFile.WriteByte(CHARDATA_SPELLS1); //[BYTE - CHUNK - SPELLS]
	gFile.WriteByte(SpellList.size()); //[BYTE]

	for (int s = 0; s < SpellList.size(); s++)
		gFile.WriteString(SpellList[s]); //[X STRINGS]

	//Save Items
	gFile.WriteByte(CHARDATA_ITEMS2); //[BYTE - CHUNK - ITEMS]

	static mslist<CGenericItem *> WriteList;
	WriteList.clearitems();

	for (int i = 0; i < pPlayer->Gear.size(); i++)
		if (pPlayer->Gear[i] != pPlayer->PlayerHands) //Skip player hands
			WriteList.add(pPlayer->Gear[i]);

	gFile.WriteByte(WriteList.size()); //[BYTE]

	for (int i = 0; i < WriteList.size(); i++)
	{
		genericitem_full_t charItem = genericitem_full_t(WriteList[i]);
		WriteItem(gFile, charItem); //[X ITEMS]
	}

	//Save storage items
	gFile.WriteByte(CHARDATA_STORAGE1);			  //[BYTE - CHUNK - STORAGE ITEMS]
	gFile.WriteShort(pPlayer->m_Storages.size()); //[SHORT]

	for (int s = 0; s < pPlayer->m_Storages.size(); s++)
	{
		storage_t &Storage = pPlayer->m_Storages[s];

		gFile.WriteString(Storage.Name);		//[STRING]
		gFile.WriteShort(Storage.Items.size()); //[SHORT]
		for (int i = 0; i < Storage.Items.size(); i++)
			WriteItem(gFile, Storage.Items[i]); //[X ITEMS]
	}

	//Save Companions
	gFile.WriteByte(CHARDATA_COMPANIONS1);			//[BYTE - CHUNK - COMPANIONS]
	gFile.WriteShort(pPlayer->m_Companions.size()); //[SHORT]
	static msstringlist SaveVarName, SaveVarValue;
	SaveVarName.clearitems();
	SaveVarValue.clearitems();

	for (int c = 0; c < pPlayer->m_Companions.size(); c++)
	{
		companion_t &Companion = pPlayer->m_Companions[c];
		gFile.WriteString(Companion.ScriptName); //[STRING]

		//Save any variables that start with "companion.save."
		CBaseEntity *pEntity = Companion.Entity.Entity();
		if (!pEntity)
			continue;
		IScripted *pScripted = pEntity->GetScripted();
		if (!pScripted || !pScripted->m_Scripts.size())
			continue;

		pScripted->CallScriptEvent("game_companion_save");

		CScript *Script = pScripted->m_Scripts[0];
		for (int v = 0; v < Script->m_Variables.size(); v++)
			if (Script->m_Variables[v].Name.starts_with("companion.save."))
			{
				SaveVarName.add(Script->m_Variables[v].Name);
				SaveVarValue.add(Script->m_Variables[v].Value);
			}

		gFile.WriteShort(SaveVarName.size()); //[SHORT]
		for (int var = 0; var < SaveVarName.size(); var++)
		{
			gFile.WriteString(SaveVarName[var]);  //[STRING]
			gFile.WriteString(SaveVarValue[var]); //[STRING]
		}
		SaveVarName.clearitems(); //Reset these for the next companion
		SaveVarValue.clearitems();
	}

	//Save Help tips
	gFile.WriteByte(CHARDATA_HELPTIPS1);				//[BYTE - CHUNK - HELPTIPS]
	gFile.WriteShort(pPlayer->m_ViewedHelpTips.size()); //[SHORT]
	for (int t = 0; t < pPlayer->m_ViewedHelpTips.size(); t++)
		gFile.WriteString(pPlayer->m_ViewedHelpTips[t]); //[STRING]

	//Save Quests
	gFile.WriteByte(CHARDATA_QUESTS1);		  //[BYTE - CHUNK - QUESTS]
	gFile.WriteInt(pPlayer->m_Quests.size()); //[INT]
	for (int q = 0; q < pPlayer->m_Quests.size(); q++)
	{
		gFile.WriteString(pPlayer->m_Quests[q].Name); //[STRING]
		gFile.WriteString(pPlayer->m_Quests[q].Data); //[STRING]
	}

	//Save Quickslots
	gFile.WriteByte(CHARDATA_QUICKSLOTS1); //[BYTE - CHUNK - QUICKSLOTS]
	gFile.WriteByte(MAX_QUICKSLOTS);	   //[INT]
	for (int q = 0; q < MAX_QUICKSLOTS; q++)
	{
		quickslot_t &QuickSlot = pPlayer->m_QuickSlots[q];
		if (QuickSlot.Active)
		{
			gFile.WriteByte(((byte)QuickSlot.Type) + 1); //[BYTE]
			gFile.WriteInt(QuickSlot.ID);				 //[INT]
		}
		else
			gFile.WriteByte(0); //[BYTE]
	}
	//-------------

	gFile.m_BufferSize = gFile.GetWritePtr();
	
	if (FNShared::IsEnabled())
	{
		// If Central Server is enabled, save to the Central Server instead of locally
		FNShared::CreateOrUpdateCharacter(pPlayer, pPlayer->m_CharacterNum, gFile.m_Buffer, gFile.GetFileSize(), (pData == NULL));
		gFile.Close();
		return;
	}
	else if (!MSGlobals::ServerSideChar)
	{
		charinfo_t &CharInfo = pPlayer->m_CharInfo[pPlayer->m_CharacterNum];
		CharInfo.AssignChar(pPlayer->m_CharacterNum, LOC_CLIENT, (char *)gFile.m_Buffer, gFile.GetFileSize(), pPlayer);
		gFile.Close();
		return;
	}

	gFile.WriteToFile(pszFileName, "wb", true);
	gFile.Close();
}