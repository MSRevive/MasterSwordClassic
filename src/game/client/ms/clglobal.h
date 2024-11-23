#include "global.h"
#include "cl_entity.h"
#include <vector>

class CBaseEntity;

inline CBaseEntity *PrivData(entvars_t *pev) { return (CBaseEntity *)pev->pContainingEntity; }

//Client-side Globals
//===================

struct mstexture_t
{
	string_i Name;
	bool IsReflective, //Reflect world
		IsWater;	   //Do splashes, refractions
	//Color4F Color;
	//bool Blending;								//Blend surface texture -- UNDONE - map does this
	struct mirrorsettings_t
	{
		Color4F Color;
		bool Blending; //Blend reflection with the original surface texture
		float Range;   //Only reflects when eye is within this range
		bool NoWorld;  //Don't reflect the world
		bool NoEnts;   //Don't reflect studio ents
	} Mirror;
};

struct hudcharanims_t
{
	string_i Idle_NoWeapon,
		Idle_Weapon,
		Fidget,
		Highlighted,
		Inactive,
		Uploading;
};

struct hudsounds_t
{
	string_i QuickSlot_Select,
		QuickSlot_Confirm,
		QuickSlot_Assign;
};

struct hudcoords_t
{
	float ItemDesc_X, ItemDesc_Y;
};

class MSCLGlobals
{
public:
	static msstring AuthID;					   //Steam ID for this client
	static bool OnMyOwnListenServer;		   //Did I join my own listen server?
	static bool CharPanelActive;			   //Choosing a character to play with?
	static bool CreatingCharacter;			   //In the process of creating a character?
	static bool CamThirdPerson;				   //Camera is in thirdperson?
	static bool OtherPlayers;				   //Other players who can legally play this map are on the server
	static mslist<mstexture_t> Textures;	   //Custom textures, to be rendered in a unique way (reflective, blended, etc)
	static hudcharanims_t DefaultHUDCharAnims; //Anims for the char select VGUI
	static hudsounds_t DefaultHUDSounds;	   //HUD sounds
	static hudcoords_t DefaultHUDCoords;	   //HUD placement coordinates

	static void Initialize();							//Global one-time Initialization - called from CHud :: Init()
	static void Think();								//Global think - sure to be called every frame
	static void InitializePlayer();						//Initialize the player each level
	static void JoinedServer();							//I just joined a server
	static void DLLDetach();							//Client dll is shutting down
	static string_t AllocString(const char *pszString); //Allocate a new string, if it doesn't already exists
	static int GetLocalPlayerIndex();					//1-based index of the local player
	static void EndMap();								//Map ended.  Do client-side specific cleanup

	//Entity-based
	static std::vector<std::string> m_Strings;
	//static mslist<char *> m_Strings;								  //All client-side globally allocated strings
	static mslist<CBaseEntity *> m_ClEntites;						  //All client-side entities
	static mslist<cl_entity_t> m_ClModels;							  //Extra models to be updated/animated client-side
	static cl_entity_t CLViewEntities[CLPERMENT_TOTAL];				  //All View entity models
	static void AddEnt(CBaseEntity *pEntity);						  //Add ent to the list
	static void RemoveEnt(CBaseEntity *pEntity, bool fDelete = true); //Remove from the list
	static void PrintAllEntites();									  //Print all client entities to console
	static void RemoveAllEntities();								  //Delete all client entites
	static void SetupGlobalEngFuncRedirects(void);					  //Set up engine functions at dll load
	static void SpawnIntoServer();									  //Called each map change
	static void Cleanup();											  //Clean up all the stuff used from the previous map

	static int mapDarkenLevel; //Map's custom bloom darkening MiB 31_DEC2010
};

void DLLAttach(HINSTANCE hinstDLL);
void DLLDetach();
