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
#ifndef PLAYER_H
#define PLAYER_H

#include "monsters/msmonster.h"
#include "sharedutil.h"
#include "monsters/bodyparts/bodyparts_human.h"
#include "pm_materials.h"
#include "mscharacter.h"

#define MAX_ID_RANGE 2048
#define SBAR_STRING_SIZE 128
#define NUM_MAX_ITEMS 75 //Thothie APR2011_28

enum sbar_data
{
	SBAR_ID_TARGETNAME = 1,
	SBAR_ID_TARGETHEALTH,
	SBAR_ID_TARGETARMOR,
	SBAR_END,
};

#define CHAT_INTERVAL 1.0f
//Master Sword

class CStat;
class CBasePlayer;
class CCorpse;
class CTeam;
struct TCallbackMenu;
class CGenericItem;

enum EMapStatus
{
	UNDEFINED_MAP, //Just joined, and map info hasn't been sent yet
	FIRST_MAP,	   //Created a new character.  Must spawn on a ms_player_begin
	OLD_MAP,	   //Stayed on the same map
	NEW_MAP,	   //Switched to a new map (valid)
	INVALID_MAP	   //Switched to a new map (INVALID)
};
enum EntType
{
	ENT_NEUTRAL,
	ENT_FRIENDLY,
	ENT_WARY,
	ENT_HOSTILE,
	ENT_DEADLY,
	ENT_BOSS, //Thothie DEC2012_12 - boss hud ID
	ENT_NOTNPC,
};
struct entinfo_t
{
	int entindex;
	msstring Name;
	EntType Type;
};
enum hudevent_e //For use with SendEventMsg()
{
	HUDEVENT_NORMAL,   //Normal color (off white)
	HUDEVENT_UNABLE,   //Unable to do something (Pickup item, Equip, drop, attack, switch hands, etc.)
	HUDEVENT_ATTACK,   //Your attack results
	HUDEVENT_ATTACKED, //You were attacked
	HUDEVENT_GREEN,	   //Something good
	HUDEVENT_BLUE,	   //Something blue
};
enum charstate_e
{
	CHARSTATE_UNLOADED, //Character hasn't been loaded yet
	CHARSTATE_LOADED,	//Character has been loaded, and can be saved
	CHARSTATE_LOADING,	//JAN2010_10 Thothie - character still loading flag
};

enum quickslottype_e
{
	QS_ITEM,
	QS_SPELL,
	QS_ARROW, //MIB Arrow Selection JUN2010_17
};

struct quickslot_t //Quickslots for items, spells
{
	bool Active;
	quickslottype_e Type;
	uint ID;
};
#define MAX_QUICKSLOTS 24 //MiB MAR2012 - Increase quickslots

#include "sharedmenu.h"

void SendViewAnim(CBasePlayer *pPlayer, int iAnim, int body = 0);
//void SetViewModel( const char *pszViewModel );
void InitializeBodyPart(CBaseEntity *pOwner, CBaseEntity *pBodyPart, const char *ModelName);
const char *GetPlayerTitle(int Title);
int GetPlayerTitleIdx(const char *pszTitle);
void MSGSend_PlayerInfo(CBasePlayer *pSendToPlayer, CBasePlayer *pPlayer);

//macros (just to shorten things up a bit)
#define CREATE_ENT(item) (CBaseEntity *)GET_PRIVATE(CREATE_NAMED_ENTITY(MAKE_STRING(item)));

#define VAR_NPC_ANIM_TORSO "game.monster.torso_anim"
#define VAR_NPC_ANIM_LEGS "game.monster.legs_anim"

#define PLAYER_SCRIPT "player/player"
//#define mCH m_pOwner->iCurrentHand

/*//#define Wielded( iHand ) ((Hand[iHand])?Hand[iHand]->Wielded:(PlayerHands?PlayerHands->Wielded:FALSE))
#define Wielded( iHand ) ((Hand[iHand])?TRUE:(PlayerHands?PlayerHands->Wielded:FALSE))
//#define CHWielded ((Hand[iCurrentHand])?Hand[iCurrentHand]->Wielded:(PlayerHands?PlayerHands->Wielded:FALSE))
#define CHWielded Wielded(iCurrentHand)*/

#define CH Hand[iCurrentHand]
#define MAX_PLAYER_HANDS 2
#define MAX_PLAYER_HANDITEMS 3
#define MAX_KEYHISTORY 10

#define mSStat m_pOwner->GetSkillStat
#define mNStat m_pOwner->GetNatStat

#define MSGFLAG_SPAWN (1 << 0)

//-----------------

#define PLAYER_FATAL_FALL_SPEED 1024															  // approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED 580															  // approx 20 feet
#define DAMAGE_FOR_FALL_SPEED (float)100 / (PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED) // damage per unit per second.
#define PLAYER_MIN_BOUNCE_SPEED 200
#define PLAYER_FALL_PUNCH_THRESHHOLD (float)350 // won't punch player's screen/make scrape noise unless player falling at least this fast.

//
// Player PHYSICS FLAGS bits
//
#define PFLAG_ONLADDER (1 << 0)
#define PFLAG_ONSWING (1 << 0)
#define PFLAG_ONTRAIN (1 << 1)
#define PFLAG_ONBARNACLE (1 << 2)
#define PFLAG_DUCKING (1 << 3)	// In the process of ducking, but totally squatted yet
#define PFLAG_USING (1 << 4)	// Using a continuous entity
#define PFLAG_OBSERVER (1 << 5) // player is locked in stationary cam mode. Spectators can move, observers can't.

//
// generic player
//
//-----------------------------------------------------
//This is Half-Life player entity
//-----------------------------------------------------
/*#define CSUITPLAYLIST	4		// max of 4 suit sentences queued up at any time

#define SUIT_GROUP			TRUE
#define	SUIT_SENTENCE		FALSE

#define	SUIT_REPEAT_OK		0
#define SUIT_NEXT_IN_30SEC	30
#define SUIT_NEXT_IN_1MIN	60
#define SUIT_NEXT_IN_5MIN	300
#define SUIT_NEXT_IN_10MIN	600
#define SUIT_NEXT_IN_30MIN	1800
#define SUIT_NEXT_IN_1HOUR	3600

#define CSUITNOREPEAT		32

#define	SOUND_FLASHLIGHT_ON		"items/flashlight1.wav"
#define	SOUND_FLASHLIGHT_OFF	"items/flashlight1.wav"

#define TEAM_NAME_LENGTH	16*/

//---------Master Sword -----------
//---------------------------------

struct PlayerButtonStruct
{
	int ButtonsDown;			//Are these keys currently down or up?
	int MoreBTNSDown;			//To detect if buttons not defined by the engine are down
	//float fMaxForwardPressTime; //hold forward longer than this & no run next keypress
	int BlockButtons;			//Don't detect another push until user lets the button up
};
struct keysnapshot
{
	int Buttons;
	float Time;
};

struct skillinfo_t
{
	int SkillsActivated;

	//FadeSkill
	enum
	{
		FADE_IN,
		FADE_OUT
	} FadeStatus;
	float FadeAmt, FadeTargetAmt, TimeEndFade, TimeRevealCheck;
	bool fAttacked;
};

extern keysnapshot KeyHistory[10];

struct clientaddr_t
{							   //Client data.
	edict_t *pe;			   //World entity that belongs to this client
	char Addr[128];			   //Address of the client connection
	float TimeClientConnected; //Time the client connect
	bool fDisplayedGreeting;   //Only send the motd once per server join.  Dont send when the server changes maps
};
extern clientaddr_t g_NewClients[32];

struct createchar_t
{
	bool ServerSide;  //Client or server-side character (affects file naming scheme, directory, etc.)
	int iChar;		  //Index of char
	msstring Name;	  //Name
	int Gender;		  //Gender
	msstring Weapon;  //Starting Wepaon
	msstring SteamID; //SteamID (server-side characters)
};

#pragma pack( push, 1 )

struct clientitem_t : public genericitem_t
{
	clientitem_t() {}
	clientitem_t(class CGenericItem *pItem);
	ushort Location;
	byte Hand;
};

struct scoreinfo_t
{
	int TitleIndex;
	int SkillLevel;
	bool IsElite, InTransition;
	float fl_healthBuffer;
};

#pragma pack( pop )

// -------------
enum netmsg_e
{
	NETMSG_SETPROP,		//MiB Nov/Dec2007 NETMSG_ANIM, MiB DEC2007 wierdness
	NETMSG_STATUSICONS, //Drigien MAY2008
	NETMSG_ANIM,
	NETMSG_FATIGUE,
	NETMSG_MUSIC,
	NETMSG_HANDS,
	NETMSG_SETSTAT,
	NETMSG_SPELLS,
	NETMSG_HP,
	NETMSG_MP,
	NETMSG_PLAYEVENT,
	NETMSG_CLDLLFUNC,
	NETMSG_VGUIMENU,
	NETMSG_ENTINFO,
	NETMSG_STOREITEM,
	NETMSG_VOTE,
	NETMSG_HUDMSG,
	NETMSG_CHARINFO,
	NETMSG_ITEM,
	NETMSG_EXP,
	NETMSG_CLXPLAY,	   //MAR2012_28 - client side sound message
	NETMSG_LOCALPANEL, // MiB MAR2015_01 [LOCAL_PANEL] - Index of local panel message
	NETMSG_UPDATEITEM, // OCT2016_18 - Thothie Update Item
	NETMSG_NUM,
};
extern int g_netmsg[NETMSG_NUM];

//MiB Nov/Dec2007a - For use with the SetProp Message
enum propmsg_e
{
	PROP_TITLE,
	PROP_NAME,
	PROP_SPELL,
	PROP_ANIMSPEEDADJ,
};

//A genericitem struct that holds _all_ neccesary item info.
//Used for storage and saving
struct genericitem_full_t : genericitem_t
{
	genericitem_full_t() : genericitem_t() {}
	genericitem_full_t(CGenericItem *pItem);
	~genericitem_full_t() { ContainerItems.clear(); }
	operator CGenericItem *();

	unsigned short Location;
	byte Hand;
	float Spell_TimePrepare;
	byte Spell_CastSuccess;
	mslist<genericitem_full_t> ContainerItems;
};
struct storage_t
{
	msstring Name;
	mslist<genericitem_full_t> Items;
};
struct storageaccess_t
{
	bool Active;
	bool ReqTeller; //Whether we should keep checking to see if the player is out of range of the teller or if the teller has died
	msstring DisplayName;
	msstring StorageName;
	float flFeeRatio;
	entityinfo_t Teller;
};

//Companion
struct companion_t
{
	bool Active;
	entityinfo_t Entity;
	string_i ScriptName;
	msstringlist SaveVarName,
		SaveVarValue;
};

//Quests
struct quest_t
{
	msstring Name;
	msstring Data;
};

//Wear Positions
struct wearpos_t
{
	wearpos_t() {}
	wearpos_t(string_i name, int maxamt)
	{
		Name = name;
		MaxAmt = maxamt;
	}
	string_i Name;
	union
	{
		int MaxAmt; //Max slots available (player)  or
		int Slots;	//Number of slots used (item)
	};
};

struct chardata_t : savedata_t
{
	msstringlist m_VisitedMaps;			//All the maps I've visited
	statlist m_Stats;					//All stats.  Natural stats and Skill-based stats
	msstringlist m_Spells;				//All known spells
	mslist<genericitem_full_t> m_Items; //All carried items
	mslist<storage_t> m_Storages;		//Storage places where I have items
	mslist<companion_t> m_Companions;	//Companions
	msstringlist m_ViewedHelpTips;		//List of all help tips the player has already viewed
	mslist<quest_t> m_Quests;			//All the quests I've completed
	mslist<quickslot_t> m_QuickSlots;	//All the quickslots (based on the item IDs of the last save)

	bool ReadData(void *pData, ulong Size);

	//The number on the end of these functions is the version number.
	//Each time one is changed, a new function must be made with an increased version
	//This allows me to keep the old code and old legacy player files

	bool ReadHeader1(byte DataID, CPlayer_DataBuffer &m_File);
#ifdef VALVE_DLL
	void ReadMaps1(byte DataID, CPlayer_DataBuffer &m_File);
	void ReadSkills1(byte DataID, CPlayer_DataBuffer &m_File);
	void ReadSpells1(byte DataID, CPlayer_DataBuffer &m_File);
	void ReadItems1(byte DataID, CPlayer_DataBuffer &m_File);
	bool ReadItem1(byte DataID, CPlayer_DataBuffer &Data, genericitem_full_t &outItem);
	void ReadStorageItems1(byte DataID, CPlayer_DataBuffer &m_File);
	void ReadCompanions1(byte DataID, CPlayer_DataBuffer &m_File);
	void ReadHelpTips1(byte DataID, CPlayer_DataBuffer &m_File);
	void ReadQuests1(byte DataID, CPlayer_DataBuffer &m_File);
	void ReadQuickSlots1(byte DataID, CPlayer_DataBuffer &m_File);
#endif

	CStat* GetStat(int index)
	{
		if ((index < 0) || (index >= m_Stats.size()))
			return NULL;
		return &m_Stats[index];
	}
};

class CBasePlayer : public CMSMonster
{
public:
	//Master Sword
	charsendinfo_t m_CharSend; //Info about the char being transmitted (to server or to client)
	float m_TimeCharLastSent;  //Time the server last sent the char down to client
	charinfo_t m_CharInfo[MAX_CHARSLOTS];
	bool m_fDropAllItems; //Drop items upon death?
	int m_SayType;
	int m_iFatigue, m_iMaxFatigue;
	CGenericItem *PlayerHands;	 //Special item - Player hands
	CGenericItem *m_ChosenArrow; //MiB JUN2010 - Arrow selection
	byte m_PrefHand;			 //O = left 1 = right
	float TimeUpdateIDInfo, CheckAreaTime, ForWardPressedTime;
	//RoundTime: Time to wear/use/wield/cast/etc.
	//FatigueTime: Time to decrease the amount of iFatigue.
	CBaseEntity *CurrentTransArea;
	CBaseEntity *CurrentNoSaveArea;
	CBaseEntity *CurrentTownArea;
	int CurrentMenu;
	PlayerButtonStruct pbs;
	float m_SprintDelay;
	bool bSprintDelayWarned = false;
	char m_szAnimTorso[32], //The current torso anim to use
		m_szAnimLegs[32];	//The current leg anim to use
	float m_TimeResetLegs;	//Hold the attack legs anim until this time
	float SpawnCheckTime;
	CBaseEntity *CamEntity;
	CCorpse *m_Corpse; //Corpse from my last death
	TCallbackMenu *CurrentCallbackMenu;
	int m_MsgFlags;
	char m_cEnterMap[64]; //Last map player saved on

	EMapStatus m_MapStatus; //Whether this map is my first, a transition, or the same as last time
	bool m_fIsElite;
	struct itemtrans_t *m_ItemTrans;
	float m_TimeTillSuicide;
	//float				LastPush; //Thothie SEP2011_16 - last time a player was affected by a push brush (failed: wont transfer between maps version and script version of CBasePlayer)
	float m_TimeCanVote;
	bool m_fClientInitiated; //Client sent this command to the server.  Make sure to cache any item changes so they don't get sent back to client
	bool m_fInTownArea;
	mslist<storage_t> m_Storages;	  //Storage places where I have items
	storageaccess_t m_CurrentStorage; //Currently accessing this storage
	bool m_Initialized;				  //Whether stats were created, script loaded, etc
	float m_TimeNextSave;
	bool m_CanJoin;							  //Whether I have any characters on this server that can join the game
	int m_JoinType;							  //How I joined the server
	mslist<companion_t> m_Companions;		  //
	msstringlist m_ViewedHelpTips;			  //List of all help tips the player has already viewed
	mslist<quest_t> m_Quests;				  //All the quests I've completed
	msstringlist m_Maps;					  //All the maps I've visited
	mslist<wearpos_t> m_WearPositions;		  //All available positions to wear something
	bool m_ClientAttack;					  //Attack queued
	int m_ClientAttackHand;					  //Current hand the player is attacking with
	int m_ClientAttackNum;					  //Current attack the player is performing
	float m_GaitFramerateGauge;				  //If I'm moving at this speed, play my gait at normal (100%) fps. Otherwide adjust fps using a ratio
	quickslot_t m_QuickSlots[MAX_QUICKSLOTS]; //Quickslots for spells, items
	float m_AnimSpeedAdj;
	Vector mGlowColor;
#ifndef VALVE_DLL
	char mLastGlowColor[14];
#endif

	//unused at the moment:
	float m_TimeCanSteal;
	int m_PlayersKilled; //How many players I've killed
	int m_LastPlayerToKillMe;
	CBasePlayer *m_pLastPlayerToKillMe;
	float m_TimeWaitedToForgetKill;
	float m_TimeWaitedToForgetSteal;
	skillinfo_t SkillInfo;
	BOOL LockSpeed;
	int ArrowsStuckInMe;

	//Save file
	char m_ClientAddress[128];
	unsigned int m_SaveFileID;								//Client sets this ID via "savefileid", then sends the save file with
															//this ID attached so the server knows which client the file belongs to
	bool RestoreAllServer(void *pData, unsigned long Size); //pData = Save file data

	mslist<entinfo_t> m_EntInfo; // Info for client-side ID system

#ifndef VALVE_DLL
	//fLastSpeed: To check if you lost enough speed to stop running
	float fSpeed, fLastSpeed;
	float m_MaxSpeed;	//Speed the Player can move.  Client only
	int m_CharLastSent; //Index of last char uploaded to server
#endif
#ifdef VALVE_DLL
	// Client caches
	int m_ClientCurrentHand;
	int m_iClientHP;	// the health currently known by the client.  If this changes, send a new
	int m_iClientMaxHP; // the max health currently known by the client.  If this changes, send a new
	int m_iClientMP;	// the mana currently known by the client.  If this changes, send a new
	int m_iClientMaxMP; // the max mana currently known by the client.  If this changes, send a new
						/*
	//MAR2010_08 hacks, seeing if disabling this helps
	//Thothie - yup, it's this that's fuxing up the m_CharacterNum corruption, just no idea why
	float		m_timeLastSentHP;	// MiB JAN2010_26 - Changing HP/MP a lot causes lag from sending to the client so many times. Putting a limit on them.
	float		m_timeLastSentMaxHP;// " "
	float		m_timeLastSentMP;	// " "
	float		m_timeLastSentMaxMP;// " "
*/

	//MIB JUN2010_14 - "MP and HP sends Fix.rtf"
	float m_timeLastSentHP;
	float m_timeLastSentMP;

	mslist<clientitem_t> m_ClientItems; //Client-known Items I'm wearing
	int m_ClPlayersKilled;				//How many players I've killed
	float m_ClTimeWaitedToForgetKill;
	float m_ClTimeWaitedToForgetSteal;
	int m_ClientGender;
	//---------------
	scoreinfo_t m_ScoreInfoCache;
	CTeam *m_pTeam,	  //My current party
		*m_pJoinTeam; //Waiting for acceptance into this party
	float m_TimeGainHP;

	int m_iMusicArea = -1;  //Index of the last music trigger player used / was a part of
	bool SwapMusic(int musicArea, int mode, std::string track); //Attempts to swap music with a new area trigger. Returns true if successful

	float m_TimeSendCharInfo;  //Delay to send info about the next character
	bool m_LoadedInitialChars; //Whether I've tried to load my characters for display yet
#endif

#ifndef VALVE_DLL
	void Think();
	void SUB_Remove(void) {}
	void CheckSpeed();
	void DoSprint();
	bool CheckHandSwitch();
	void BeginRender();
	void Render();
	void RenderCleanup();
#endif			 //There is no #else here because it screws
#ifdef VALVE_DLL //up the Class helper built into .NET
	bool IsInAttackStance();
	const char *DisplayName();
	bool CanDamage(CBaseEntity *pOther); //Can I damage this entity?
	void Storage_Open(msstring_ref pszDisplayName, msstring_ref pszStorageName, float flFeeRatio, entityinfo_t &Entity);
	void Storage_Send();
	std::tuple<bool, int> LearnSkill(int iStat, int iStatType, int EnemySkillLevel);
	bool LearnSkill(int iStat, int EnemySkillLevel);
	void SetQuest(bool SetData, msstring_ref Name, msstring_ref Data);
	float TraceAttack(damage_t &Damage);
	void SUB_Remove(void);
	bool LoadCharacter(int Num); //Load a server-side character
	void Think_SendCharData();	 //Send info about a Char, if not already sent
	void SaveChar();
#endif
	int MSProperties()
	{
		return 400;
	}										 //NPC_PLAYER
	void CreateChar(createchar_t &CharData); //Create a character (Client or Server)
	void Deactivate();
	void SendInfoMsg(const char *msg, ...);
	void SendHUDMsg(msstring_ref Title, msstring_ref Text);						   //HUD message - top left
	void SendHelpMsg(msstring_ref Tipname, msstring_ref Title, msstring_ref Text); //Help tip message
	void SendEventMsg(msstring_ref Text);										   //Event message - VGUI console
	void SendEventMsg(COLOR &color, msstring_ref Text);							   //Event message - VGUI console
	void SendEventMsg(hudevent_e HudEvent, msstring_ref Text);					   //Event message - VGUI console
	void SetModel(const char *Newmodel);
	void SetModel(int Newmodel);
	void SetTeam(CTeam *pNewTeam);
	void UpdateSpeed();
	void UpdateMiscPositions();
	void GetAnyItems();
	void StealAnyItems(CBaseEntity *pVictim);
	void OfferItem(offerinfo_t &OfferInfo);
	void TransactionCallback(CBasePlayer *pPlayer, int slot, TCallbackMenu *pcbMenu);
	CGenericItem *CanPutInAnyPack(CGenericItem *pItem, bool bVerbose);
	//	void				SetRoundTime( float seconds = 0, float fSpeed = -1 );
	//	void				UpdateRoundTime( );
	//	void				UpdateFatigue( );
	//	void				UpdateMana( );
	//bool				SwapHands( bool bVerbose = true );
	void ShowMenu(char *pszText, int bitsValidSlots, int nDisplayTime = 0, BOOL fNeedMore = FALSE);
	int GiveGold(int iAmount, bool bVerbose = true);
	//float				TraceAttack( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType, int iAccuracyRoll);
	void AttackSound();
	void PainSound();
	void StruckSound(CBaseEntity *pInflicter, CBaseEntity *pAttacker, float flDamage, TraceResult *ptr, int bitsDamageType);
	void MovePackSelection(int iUpOrDown);
	void SendMenu(int iMenu, TCallbackMenu *cbmMenu = NULL);
	void ParseMenu(int iMenu, int slot);
	void InitHUD(); //Don't do anything relating to the client until now!!
	void PlayerAction(msstring_ref Action);
	bool PrepareSpell(const char *pszName);
	void TakeDamageEffect(CBaseEntity *pInflicter, CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	void CinematicCamera(BOOL OnorOff, Vector vecPosition = g_vecZero, Vector vecViewAngle = g_vecZero, BOOL bCreateClone = FALSE);
	CBaseEntity *GiveNamedItem(const char *szName);

	CGenericItem *ActiveItem();
	int ActiveItemHand();
#ifdef VALVE_DLL
	bool m_Gagged; //Thothie FEB2007b - attempting to allow admin_gagging of players
#endif
	bool AddItem(CGenericItem *pItem, bool ToHand, bool CheckWeight, int Hand = -1);
	bool InMenu;	//Thothie SEP2011_07 - prevent inv use while in menus
	int NumItems(); //Thothie SEP2011_07 - return number of items func
	int NewItemHand(CGenericItem *pItem, bool CheckWeight, bool bVerbose, bool FreeHands = false, char *pszErrorString = NULL);
	bool CanHold(CGenericItem *pItem, bool bVerbose = true, char *pszErrorString = NULL);
	bool SwitchHands(int iHand, bool bVerbose = true); //Switch to a held item
	bool SwitchToBestHand();
	//bool				DropItem( int Hand, bool ForceDrop = false, Vector &vDropDir = Vector(800,800,800) );
	bool DropItem(CGenericItem *pDropItem, bool ForceDrop, bool Verbose);
	bool UseItem(int Hand, bool bVerbose = false);
	bool PutInPack(int iHand, CGenericItem *pContainer, bool bVerbose = true);
	bool PutInPack(CGenericItem *pItem, CGenericItem *pContainer, bool bVerbose = true);
	bool PutInAnyPack(CGenericItem *pItem, bool bVerbose = true);
	bool RemoveItem(CGenericItem *pItem);
	void RemoveAllItems(bool fDead, bool fDeleteItems = false);

	void Trade();
	tradeinfo_t *TradeItem(tradeinfo_t *ptiTradeInfo);
	bool AcceptOffer();
	BOOL SkinMonster(CMSMonster *pDeadMonster);
	void LearnSpell(const char *pszSpellScript, bool fVerbose = false);
	void PlaySound(int channel, const char *sample, float volume, bool fGenderSpecific = false, float attenuation = ATTN_NORM);
	void KickPlayer(const char *pszMessage);
	bool IsElite();
	void AddNoise(float flNoiseAmt);
	int IRelationship(CBaseEntity *pTarget);
	void UseSkill(int iSkill);
	void Attacked(CBaseEntity *pAttacker, float flDamage, int bitsDamageType);
	void Seen(CMSMonster *pMonster);
	bool IsActive(); //Thothie NOV2014_09 - IsActive() - centralizing AFK checking
	CBaseEntity *FindSpawnSpot();
	bool MoveToSpawnSpot();
	const char *GetTitle();		//Calculate title
	const char *GetFullTitle(); //Calculate title with skill Ex. "23 berserker"
	void SetKeys();
	void BlockButton(int Button);
	storage_t *Storage_CreateAccount(storage_t &Storage);
	storage_t *Storage_GetStorage(msstring_ref pszStorageName);
	void InitialSpawn(void);
	bool Script_SetupEvent(CScript *Script, SCRIPT_EVENT &Event);
	const char *GetPartyName();
	ulong GetPartyID();
	bool IsLocalHost(); //This is a listen server and this is the host
	msstring AuthID();
	void SendChar(charinfo_base_t &CharBase);
	void PreLoadChars(int CharIdx = -1); //Preload all my available characters (client or server)
	void QuickSlot_Create(int Slot, ulong ID, bool Verbose);
	void QuickSlot_Use(int Slot);

	// Stats
	float eyeheight;
	charstate_e m_CharacterState; //Is the character loaded?
	int m_CharacterNum;			  //Number of the currently loaded character
	char m_NextMap[32], m_OldTransition[32], m_NextTransition[32];
	char *m_SpawnTransition; //Transition to spawn at, after a level transition, or after death

	bool CreateStats();
	void DeleteStats();

	int IdealModel();
	Vector Size(int flags = 0);
	void SetSize(int flags = 0);
	float Volume();

	float MaxStamina();
	float MaxHP();
	float MaxMP();
	float WalkSpeed(bool bParseSpeed = true);
	float RunSpeed(bool bParseSpeed = true);
	float ParseSpeed(float flSpeed);
	float CurrentSpeed(bool bParseSpeed = true);
	int SkillAvg();
	//-------------------------------------------

	int random_seed;		// See that is shared between client & server for shared weapons code
	msstring CustomTitle;	//MiB/Thothie DEC2007a - For new title system
	int m_iPlayerSound;		// the index of the sound list slot reserved for this player
	int m_iTargetVolume;	// ideal sound volume.
	int m_iWeaponVolume;	// how loud the player's weapon is right now.
	int m_iExtraSoundTypes; // additional classification for this weapon's sound
	int m_iWeaponFlash;		// brightness of the weapon flash
	float m_flStopExtraSoundTime;

	int m_afButtonLast;
	int m_afButtonPressed;
	int m_afButtonReleased;

	edict_t *m_pentSndLast; // last sound entity to modify player room type
	float m_flSndRoomtype;	// last roomtype set by sound entity
	float m_flSndRange;		// dist from player to sound entity

	float m_flFallVelocity;

	int m_rgItems[MAX_ITEMS];
	int m_fKnownItem; // True when a new item needs to be added
	int m_fNewAmmo;	  // True when a new item has been added

	unsigned int m_afPhysicsFlags; // physics flags - set when 'normal' physics should be revisited or overriden
	float m_fNextSuicideTime;	   // the time after which the player can next use the suicide command

	// these are time-sensitive things that we keep track of
	float m_flTimeStepSound;  // when the last stepping sound was made
	float m_flTimeWeaponIdle; // when to play another weapon idle animation.
	float m_flSwimTime;		  // how long player has been underwater
	float m_flDuckTime;		  // how long we've been ducking
	float m_flWallJumpTime;	  // how long until next walljump

	float m_flSuitUpdate;	// when to play next suit update
							//	int					m_rgSuitPlayList[CSUITPLAYLIST];// next sentencenum to play for suit update
	int m_iSuitPlayNext;	// next sentence slot for queue storage;
							//	int					m_rgiSuitNoRepeat[CSUITNOREPEAT];		// suit sentence no repeat list
							//	float				m_rgflSuitNoRepeatTime[CSUITNOREPEAT];	// how long to wait before allowing repeat
	int m_lastDamageAmount; // Last damage taken
	float m_tbdPrev;		// Time-based damage timer

	float m_flgeigerRange; // range to nearest radiation source
	float m_flgeigerDelay; // delay per update of range msg to client
	int m_igeigerRangePrev;
	int m_iStepLeft;						// alternate left/right foot stepping sound
	char m_szTextureName[CBTEXTURENAMEMAX]; // current texture name we're standing on
	char m_chTextureType;					// current texture type

	int m_idrowndmg;	  // track drowning damage taken
	int m_idrownrestored; // track drowning damage restored

	int m_bitsHUDDamage; // Damage bits for the current fame. These get sent to
						 // the hude via the DAMAGE message
	BOOL m_fInitHUD;	 // True when deferred HUD restart msg needs to be sent
	BOOL m_fGameHUDInitialized;
	int m_iTrain;	// Train control position
	BOOL m_fWeapon; // Set this to FALSE to force a reset of the current weapon HUD info

	EHANDLE m_pTank;   // the tank which the player is currently controlling,  NULL if no tank
	float m_fDeadTime; // the time at which the player died  (used in PlayerDeathThink())

	BOOL m_fNoPlayerSound; // a debugging feature. Player makes no sound if this is true.
	BOOL m_fLongJump;	   // does this player have the longjump module?

	float m_tSneaking;
	int m_iUpdateTime; // stores the number of frame ticks before sending HUD update messages
	int m_iHideHUD;	   // the players hud weapon info is to be hidden
	int m_iClientHideHUD;
	int m_iFOV;		  // field of view
	int m_iClientFOV; // client's known FOV
	// usable player items
	CBasePlayerItem *m_rgpPlayerItems[MAX_ITEM_TYPES];
	CBasePlayerItem *m_pActiveItem;
	CBasePlayerItem *m_pClientActiveItem; // client version of the active item
	CBasePlayerItem *m_pLastItem;
	// shared ammo slots
	int m_rgAmmo[MAX_AMMO_SLOTS];
	int m_rgAmmoLast[MAX_AMMO_SLOTS];

	Vector m_vecAutoAim;
	BOOL m_fOnTarget;
	int m_iDeaths;
	float m_iRespawnFrames; // used in PlayerDeathThink() to make sure players can always respawn

	int m_lastx, m_lasty; // These are the previous update's crosshair angles, DON"T SAVE/RESTORE

	int m_nCustomSprayFrames; // Custom clan logo frames for this player
	float m_flNextDecalTime;  // next time this player can spray a decal

	virtual void Spawn(void);
	//void Pain( void );

	//	virtual void Think( void );
	virtual void Jump(void);
	virtual void Duck(void);
	virtual void PreThink(void);
	virtual void PostThink(void);
	virtual Vector GetGunPosition(void);
	virtual int TakeHealth(float flHealth, int bitsDamageType);
	virtual int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	virtual void Killed(entvars_t *pevAttacker, int iGib);
	virtual Vector BodyTarget(const Vector &posSrc) { return Center() + pev->view_ofs * RANDOM_FLOAT(0.5, 1.1); }; // position to shoot at
	virtual void StartSneaking(void) { m_tSneaking = gpGlobals->time - 1; }
	virtual void StopSneaking(void) { m_tSneaking = gpGlobals->time + 30; }
	virtual BOOL IsSneaking(void) { return m_tSneaking <= gpGlobals->time; }
	virtual BOOL ShouldFadeOnDeath(void) { return FALSE; }
	virtual BOOL IsPlayer(void) { return TRUE; } // Spectators should return FALSE for this, they aren't "players" as far as game logic is concerned

	virtual BOOL IsNetClient(void) { return TRUE; } // Bots should return FALSE for this, they can't receive NET messages
													// Spectators should return TRUE for this
	virtual const char *TeamID(void);

	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	void RenewItems(void);
	void PackDeadPlayerItems(void);

	// JOHN:  sends custom messages if player HUD data has changed  (eg health, ammo)
	virtual void UpdateClientData(void);

	static TYPEDESCRIPTION m_playerSaveData[];

	// Player is moved across the transition by other means
	virtual int ObjectCaps(void) { return CBaseMonster ::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual void Precache(void);
	BOOL IsOnLadder(void);
	/*	BOOL			FlashlightIsOn( void );
	void			FlashlightTurnOn( void );
	void			FlashlightTurnOff( void );*/

	void UpdatePlayerSound(void);
	void DeathSound(void);

	int Classify(void);
	void SetWeaponAnimType(const char *szExtention);

	// custom player functions
	virtual void ImpulseCommands(void);
	void CheatImpulseCommands(int iImpulse);

	void StartDeathCam(void);
	void StartObserver(Vector vecPosition, Vector vecViewAngle);

	BOOL HasWeapons(void); // do I have ANY weapons?
	void SelectPrevItem(int iItem);
	void SelectNextItem(int iItem);
	void SelectLastItem(void);
	void SelectItem(const char *pstr);
	void ItemPreFrame(void);
	void ItemPostFrame(void);
	void EnableControl(BOOL fControl);

	void WaterMove(void);
	void EXPORT PlayerDeathThink(void);
	void PlayerUse(void);

	void UpdateGeigerCounter(void);
	void CheckTimeBasedDamage(void);
	//void UpdateStepSound( void );
	//void PlayStepSound(int step, float fvol);

	BOOL FBecomeProne(void);
	void BarnacleVictimBitten(entvars_t *pevBarnacle);
	void BarnacleVictimReleased(void);
	int AmmoInventory(int iAmmoIndex);
	int Illumination(void);

	void ResetAutoaim(void);
	Vector GetAutoaimVector(float flDelta);
	//Vector AutoaimDeflection( Vector &vecSrc, float flDist, float flDelta  );

	//void ForceClientDllUpdate( void );  // Forces all client .dll specific data to be resent to client.

	void DeathMessage(entvars_t *pevKiller);

	void SetCustomDecalFrames(int nFrames);
	int GetCustomDecalFrames(void);

	//SDK 2.3 -----------------
	float m_flStartCharge;
	float m_flAmmoStartCharge;
	float m_flPlayAftershock;
	float m_flNextAmmoBurn; // while charging, when to absorb another unit of player's ammo?

	//Player ID
	void InitStatusBar(void);
	void UpdateStatusBar(void);
	int m_izSBarState[SBAR_END];
	float m_flNextSBarUpdateTime;
	float m_flStatusBarDisappearDelay;
	char m_SbarString0[SBAR_STRING_SIZE];
	char m_SbarString1[SBAR_STRING_SIZE];

	float m_flNextChatTime;

	unsigned long long steamID64;	
};

#ifndef VALVE_DLL
extern CBasePlayer player;

//Mib MAR2010_12 - Armor Fix FINAL
extern int playerBodyArray[16];
#endif

#define AUTOAIM_2DEGREES 0.0348994967025
#define AUTOAIM_5DEGREES 0.08715574274766
#define AUTOAIM_8DEGREES 0.1391731009601
#define AUTOAIM_10DEGREES 0.1736481776669

extern int gmsgHudText;
extern BOOL gInitHUD;

char *GetOtherPlayerTransition(CBasePlayer *pPlayer);

#endif // PLAYER_H
