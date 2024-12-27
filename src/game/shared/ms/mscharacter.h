#ifndef MSCHARACTER_H
#define MSCHARACTER_H

#include "msfileio.h"
#include "stats/statdefs.h"
#include "gamerules/teams.h"
#include "mscharacterheader.h"
#include "buildcontrol.h"
#include "monsters/msmonster.h"

// Char Files
enum chardatastatus_e
{
	CDS_UNLOADED,
	CDS_LOADING,
	CDS_LOADED,
	CDS_NOTFOUND
};

enum charloc_e
{
	LOC_CLIENT,
	LOC_SERVER,
	LOC_CENTRAL
};

enum jointype_e
{
	JN_NOTALLOWED,
	JN_TRAVEL,
	JN_STARTMAP,
	JN_VISITED,
	JN_ELITE
};

enum charsendstatus_e
{
	CSS_DORMANT,
	CSS_SENDING,
	CSS_RECEIVING,
	CSS_SENT,
	CSS_RECEIVED
};

enum charClientType
{
	CHAR_TYPE_BASE = 0,
	CHAR_TYPE_HEADER,
	CHAR_TYPE_ITEMS,
};

struct charinfo_base_t
{
	int Index; // Keep track of index, because the characters might not be loaded in order	
	int DataLen;
	char* Data;

	charinfo_base_t()
	{
		Index = 0;
		DataLen = 0;
		Data = NULL;
	}
};

#define GEARFL_COVER_HEAD (1 << 0)
#define GEARFL_COVER_TORSO (1 << 1)
#define GEARFL_COVER_ARMS (1 << 2)
#define GEARFL_COVER_LEGS (1 << 3)
#define GEARFL_WEARING (1 << 4)

struct gearinfo_t
{
	byte Flags;
	ushort Model, Body, Skin, Anim;
};

struct charinfo_t : charinfo_base_t
{
	chardatastatus_e Status, m_CachedStatus;
	charsendstatus_e m_SendStatus; //Client uses this to determine whether the char has been uploaded
	jointype_e JoinType;
	charloc_e Location;

	//Char current Game Status, loaded from file header or sent from server
	int body; //MiB FEB2010a (JAN2010_27) - For sending what 'body' the char-selection model should use.
	bool IsElite;
	enum gender_e Gender;
	msstring Name, MapName, OldTrans, NextMap, NewTrans;
	char Guid[MSSTRING_SIZE];
	int Flags = 0;
	mslist<gearinfo_t> GearInfo;

	charinfo_t() : charinfo_base_t() { Data = NULL; Guid[0] = 0; }
	~charinfo_t();

	void Destroy();
	void AssignChar(int CharIndex, charloc_e Location, const char* Data, int DataLen, class CBasePlayer* pPlayer);
};

struct charsendinfo_t : charinfo_base_t
{
	charsendstatus_e Status; //Whether this character is being sent or receieving
	float TimeDataLastSent;
	uint DataSent;

	charsendinfo_t() : charinfo_base_t() {}
};

struct natstat_t
{
	short Value[STATPROP_TOTAL];
};
struct skillstat_t
{
	short Value[STATPROP_TOTAL];
	ulong Exp[STATPROP_TOTAL];
};
struct spellskillstat_t
{
	short Value[STATPROP_TOTAL];
	long Exp[STATPROP_TOTAL];
};

#define SAVECHAR_VERSION_MSC 11 // Legacy MS: Classic
#define SAVECHAR_VERSION_MSR 12 // MS Rebirth and up.

#define SAVECHAR_VERSION SAVECHAR_VERSION_MSR

//The types of headers.  Each time the save file is revised, a new header is added.
//The old headers are kept so the game knows when it is encountering an old save file
//and can call the legacy code for converison to the new format.

enum
{
	CHARDATA_HEADER1 = 0,
	CHARDATA_MAPSVISITED1,
	CHARDATA_SKILLS1,
	CHARDATA_SPELLS1,
	CHARDATA_ITEMS1,
	CHARDATA_STORAGE1,
	CHARDATA_COMPANIONS1,
	CHARDATA_HELPTIPS1,
	CHARDATA_QUESTS1,
	CHARDATA_QUICKSLOTS1,
	CHARDATA_ITEMS2,
	CHARDATA_UNKNOWN, //If >= CHARDATA_UNKNOWN, then skip it?
};

class MSChar_Interface
{
public:
	//static Vector LastGoodPos, LastGoodAng;
	static void AutoSave(class CBasePlayer *pPlayer);								//Client & Server
	static bool ReadCharData(void *pData, ulong Size, struct chardata_t *CharData); //Client & Server

	static enum jointype_e CanJoinThisMap(savedata_t &Data, msstringlist &VisitedMaps);		//Client & Server
	static enum jointype_e CanJoinThisMap(charinfo_t &CharData, msstringlist &VisitedMaps); //Client & Server
	static bool HasVisited(msstring_ref MapName, msstringlist &VisitedMaps);				//Client & Server

	static void PacketAcknowledged(int PacketIdx);			//Client & Server
	static void Think_SendChar(class CBasePlayer *pPlayer); //Client & Server

#ifdef VALVE_DLL
		//Server
	static void HL_SVNewIncomingChar(class CBasePlayer *pPlayer, int CharIdx, uint UUEncodeLen, uint DataLen);
	static void HL_SVReadCharData(class CBasePlayer *pPlayer, const char *UUEncodedData); //Server
	static void SaveChar(class CBasePlayer *pPlayer, savedata_t *pData = NULL);			  //Server
#else
		//Client
	static void CLInit();
	static void HL_CLNewIncomingChar(int CharIdx, uint DataLen);
	static void HL_CLReadCharData();
#endif
};

bool DeleteChar(int iCharacter);											 //Client version
bool DeleteChar(CBasePlayer *pPlayer, int iCharacter);						 //Server version
const char *GetSaveFileName(int iCharacter, CBasePlayer *pPlayer = NULL);	 //Client & Server
bool IsValidCharVersion(int Version);										 //Client & Server
savedata_t *GetCharInfo(const char *pszFileName, msstringlist &VisitedMaps); //Client & Server

#define MAX_CHARSLOTS 3 //Max number of characters one person can have. This is the max the game supports.  A server operator can set less for his server via CVAR "ms_serverchar"

struct charslot_t
{
	bool Active, //Whether this character exists and is loaded
		CanJoin, //Whether this character can enter this map
		Pending; //Central Server is currently trying to download this char

	savedata_t Data; //Character's data.  For server-side characters, only a few fields here are valid
};

class ChooseChar_Interface
{
public:
	static int ServerCharNum; //Max number of characters the server will allow (if server-side characters)
	static bool CentralServer;
	static void UpdateCharScreen();
	static void UpdateCharScreenUpload();
};
#endif //MSCHARACTER_H
