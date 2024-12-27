/***
*
*	Created by Kenneth "Dogg" Early.
*
*	Email kene@maverickdev.com or
*		  l33tdogg@hotmail.com
*
****/
#include "weapons/weapons.h"
#include "msitemdefs.h"
#ifndef VALVE_DLL
#include "cl_entity.h"
#endif

enum hand_e
{
	LEFT_HAND,
	RIGHT_HAND,
	HAND_PLAYERHANDS, //Special undroppable item -- Player hands

	ANY_HAND,
	BOTH_HANDS,
};
enum //Position on body (Used for m_Location and m_Position)
{
	ITEMPOS_NONE = (0 << 0),
	ITEMPOS_HANDS = (1 << 0),
	ITEMPOS_BODY = (2 << 0), //Somewhere on the body (head/back/arms/hands/legs/etc.)
							 /*ITEMPOS_BACK   = (1<<1),
	ITEMPOS_SIDE   = (1<<2),
	ITEMPOS_BELT   = (1<<3),
	ITEMPOS_HEAD   = (1<<4),
	ITEMPOS_CHEST  = (1<<5),
	ITEMPOS_ARMS   = (1<<6),
	ITEMPOS_LEGS   = (1<<7),
	ITEMPOS_SHOULDER  = (1<<8),
	ITEMPOS_HIP	   = (1<<9)*/
};

struct wearablemodelinfo_t
{
	wearablemodelinfo_t(int BodyPart) { wearablemodelinfo_t::BodyPart = BodyPart; }
	int BodyPart;
};

enum //Attack types
{
	ATT_STRIKE_LAND,
	ATT_STRIKE_HOLD,
	//ATT_CHARGE_STRIKE,
	ATT_CHARGE_THROW_PROJ
};

//Attackdata struct
struct attackdata_t
{
	float flRange, flDamage, flDamageRange, flEnergy, flMPDrain,
		flDamageAOERange, flDamageAOEAttn,
		flAccuracyDefault, flAccBest, flCritThreshold, flCritMulti,
		flNoise, flChargeAmt, f1DmgMulti, //f1DmgMulti is OCT2007a
		tDuration, tLandDelay, tProjMinHold,
		tMaxHold;
	int iPriority, //The priority over other attacks (like right+attack1 has priority over just attack1)
		Type,
		RequiredSkill, //Proficiency required to use this attack
		iAmmoDrain,	   //How much ammo is required to use the attack
		StatBalance, PropBalance,
		StatPower, PropPower,
		StatProf, PropProf,
		StatExp, PropExp;
	string_i sDamageType, //A custom damage type, specified in the script
		sProjectileType,  //Name (substring) of the desired projectile type
		sSkillType;		  //Thothie OCT2007a - need dodamage to be sent to selectable skills (not used yet)
	Vector StartOffset;	  //Offset from the viewangles that the attack should start
	Vector AimOffset;	  //Relative angle offset that the attack is aimed
	//Vector vAlignBase, vAlignHilt;
	//char cProjectileType[128],		//Name (substring) of the desired projectile type
	// cCallbackEvent[128],
	//cCallbackLandEvent[128],
	//cCallbackDoneEvent[128];
	msstring CallbackName; //Prefix for callbacks
	msstringlist ComboKeys;
	bool NoAutoAim, //Auto-target enemies in the viewcone (for strike-land)
		NoArmor,	//Thothie OCT2007a - Ignores Armor & Parry (unapplied)
		//InfAmmo,					//Thothie OCT2007a - Spawns own ammo
		NoDamage; //Target, but don't do damage - Usually the script will do something else instead

	//Not set by scripts - Realtime
	float tStart,		  //Time the attack began
		tTrueStart;		  //For projectiles, since tStart gets modified
	bool fAttackReleased, //Has the attack button been released yet?
		fAttackLanded,	  //Has the attack tracelined yet? Or has the projectile been tossed yet?
		fAttackThisFrame, //Used by monsters to align attacks w/ anims
		fCanCancel;		  //Helps control when attacks can end
	int iLoadedAmmo;	  //Amount of ammo currently loaded
	string_i sProjectile; //Name of the next projectile item to be fired
};

#define GET_CHARGE_FROM_TIME(a) (a + V_max(a - 1, 0) * .5)

// Global GenericItem
class CGenericItem;
class CScript;

typedef struct GenItem_s
{
	msstring Name;
	CGenericItem *pItem;
	bool bCalledSpawn; // MiB MAR2019_03 - [GET_ITEM_TABLE] Added to check if game_spawn has been called already
	GenItem_s()
	{
		Name = "";
		pItem = NULL;
		bCalledSpawn = false;
	}
	void operator=(GenItem_s t)
	{
		Name = t.Name;
		pItem = t.pItem;
		bCalledSpawn = t.bCalledSpawn;
	}
} GenItem_t;

//MiB MAR2012 Anti-Overflow - see previous archives for original
//Handles access to the global items list and can copy a global item to a usable item.
class CGenericItemMgr
{
public:
	// MiB MAR2019_03 - [GET_ITEM_TABLE] Added bCallSpawnIfNeeded to call spawn event if it hasn't already been done
	static CGenericItem *GetGlobalGenericItemByName(const char *pszItemName, bool bCallSpawnIfNeeded = false);
	static CGenericItem *NewGenericItem(CGenericItem *pGlobalItem);										 //Copy a global item to a usable item
	static CGenericItem *NewGenericItem(int idx);														 //[PackSwap]
	static void AddGlobalItem(GenItem_t &NewGlobalItem);												 //Add a new global item
	static int ItemCount();																				 //Number of global items
	static GenItem_t *Item(int idx);																	 //Retreive item
	static void DeleteItem(CGenericItem *pItem);														 //Delete one item by pointer
	static void DeleteItem(int idx);																	 //Delete one item by index
	static void DeleteItems();																			 //On map startup, delete all the previous map's items
	static void GenericItemPrecache(void);																 //On DLL load, precache all scripts
	static msstring_ref GetItemDisplayName(msstring_ref ItemName, bool Capital, bool Fullname, int Amt); //Temporarily creates an item to determine it's name
	static int LookUpItemIdx(msstring item_name);														 // [PackSwap]
	static const CGenericItem *SampleItem(int idx);														 // [PackSwap]

	static ulong m_LastDestroyedItemID;
	static scriptcmdname_list m_ScriptCommands;
	
	static msstringstringhash mItemAlias; // MiB APR2019_25 - Items that are hotswapping scripts

private:
	static mslist<GenItem_t> m_Items;
};

//CGenericItem *NewGenericItem( int Type );
CGenericItem *NewGenericItem(const char *pszItemName);
CGenericItem *MSUtil_GetItemByID(ulong m_iId);					   //Retreives an item in the world by unique ID
CGenericItem *MSUtil_GetItemByID(ulong m_iId, CMSMonster *pOwner); //Retreives an item in the world by unique ID - also checks if pOwner owns it

//SendAttackCmd is used in case I have to change the attack command
//- Thothie, attack verfication notice (no changes):
//- We need to have the client send another var that the server and client always agree upon
//- to prevent people from simply learning the attack command, and using it to fire any attack they please
#define ATTACK_COMMAND "atx"
#ifndef VALVE_DLL
#define SendCancelAttackCmd() \
	ClientCmd(UTIL_VarArgs("%s %i -1\n", ATTACK_COMMAND, m_iId)) //MiB JUN2010_19 - m_iId is needed, plus solves the above :) DualWield.rtf
#define SendAttackCmd(AttackType) \
	ClientCmd(UTIL_VarArgs("%s %i %i %f\n", ATTACK_COMMAND, m_iId, AttackType, m_LastChargedAmt)) // MiB MAR2012_06 - For allowing partial charge on the first OLD - ClientCmd( UTIL_VarArgs( "%s %i %i \n", ATTACK_COMMAND, m_iId, AttackType ) )
#endif

//Various properties
#define GI_JUSTSPAWNED (1 << 0)
#define GI_INPACK (1 << 1)

struct drinkdata_t {
	int Type, Intensity, IdleAnim;
	float flEffectDelay, flDrinkDelay;
	//Dynamic Data
	bool fDrinking;
	bool fGulped;
	float TimeDrinkStart;
	~drinkdata_t() {}
};

struct armordata_t
{
	int SwapBodyParts; //bodyparts to make invisible when this armor is worn
	float Protection;
	string_i Type;
	mslist<int> m_ProtectionAreas; //bodyparts that the armor protects
	~armordata_t() {}
};

struct projectiledata_t
{
	float Damage;
	float flDamageAOERange;
	float flDamageAOEAttn;
	string_i sDamageType;
	bool CollideHitBox; //Collide with hitboxes instead of just bboxes
	//Dynamic Data
	int StatPower, PropPower;
	int StatExp, PropExp; //Stat and prop that receives exp from this attack
	entityinfo_t OriginalOwner;
	float Speed;
	bool IgnoreNPC;	  //Ignore NPCs, collide with world only
	bool IgnoreWorld; //Float through walls
	~projectiledata_t() {}
};

class CGenericItem : public CBasePlayerItem
{
public:
	float flNextThink;				//replacement for pev->nextthink
	string_i m_AnimExt,				//Local copy of the anim extention
		m_AnimExtLegs,				//Local copy of the legs anim extention
		m_ViewModel,				//Local copy of the view anim
		m_PlayerHoldModel;			//p_model name for players
	long lProperties;				//Properties held over a frame, like JUSTSPAWNED
	char m_Name[64];				//Only for debugging... so I can see the item's name
	mslist<attackdata_t> m_Attacks; //Actions (mostly attacks) that the owner can activate with this item
	attackdata_t *CurrentAttack = nullptr;
	struct packdata_t *PackData = nullptr;
	struct drinkdata_t *DrinkData = nullptr;
	struct armordata_t *ArmorData = nullptr;
	struct projectiledata_t *ProjectileData = nullptr;
	struct spelldata_t *SpellData = nullptr;

	int m_Location,																	 //Location of the item on the monster (Hands, back, etc)
		m_Hand;																		 //The hand I'm in... if I'm in a hand
	float m_TimeExpire;																 //Time the item should be deleted
	int m_ViewModelAnim;															 //The current view model anim
	float m_ViewModelAnimSpeed;														 //OCT2011_30 - Thothie - Attempting to make viewmodel anim speed adjustable
	int m_ViewModelPart, m_ViewModelSubmodel, m_RenderMode, m_RenderAmt, m_RenderFx; //Shuriken MAR2008 - setviewmodelprop
																					 //MiB - Updated Apr2008a - Models are done by Part and Submodel
	short m_Skin;																	 //Shuriken MAR2008 - setviewmodelprop

	bool ClientAttacking; //Is the client attacking with me? DualWield.RTF
	int AttackNum;		  // What attack number should I be using? DualWield.RTF

	bool m_ReleaseAttack; //For attacks that require the button to stay held, this indicates the button is released

	int m_OldID; //The last save file ID of this item (not the current one)

	//Perishables
	short Quality, MaxQuality;

	//Groupables
	int iQuantity, iMaxGroupable, m_MaxGroupable; //MiB FEB2010_13 - Stackable Stacks added m_MaxGroupable

	//Wearbles
	mslist<wearpos_t> m_WearPositions; //Where the item goes when worn. Can use up multiple locations
	mslist<int> m_WearModelPositions;  //bodyparts that the model overrides

	hand_e m_PrefHand; //Which hand(s) can this weapon be held in

	void Wearable_WearOnOwner();
	void Wearable_RemoveFromOwner();
	void Wearable_ResetClientUpdate();
	inline bool IsWorn() { return m_Location > ITEMPOS_HANDS; }

	//Attacks
	virtual bool StartAttack(int ForceAttackNum = -1);
	virtual void Attack();
	virtual bool CheckKeys(attackdata_t *pAttData);
	virtual void RegisterAttack();
	virtual void CancelAttack();
	virtual void StrikeLand();
	virtual void StrikeHold();
	virtual void ChargeThrowProj();
	virtual bool UseAmmo(int iAmt);
	virtual int ActivateButton();
	virtual bool Attack_IsCharging();
	virtual bool Attack_CanAttack();
	virtual float Attack_Charge();
	virtual float GetHighestAttackCharge();

	float m_TimeChargeStart; //Time I started charging for a special attack.  0 - not charging
	float m_LastChargedAmt;	 //Last amount charged up
	damage_t *m_CurrentDamage;

	//Containers
	/*enum { //Pack Types
		PACK_NORMAL,
		PACK_QUIVER,
		PACK_SHEATH
	};*/

	void SetLockStrength(int iNewStrength);
	void RegisterContainer();
	void Container_SendContents();
	void Container_Open();
	void Container_UnListContents();
	bool Container_CanAcceptItem(CGenericItem *pItem);
	int Container_AddItem(CGenericItem *pItem);
	CGenericItem *Container_GetItem(int iIndex);
	bool Container_RemoveItem(CGenericItem *pItem);
	void Container_RemoveAllItems();
	void Container_Deactivate();
	void Container_StackItems(); //FEB2011_16 Thothie
	void Container_SendItem(CGenericItem *pItem, bool fAddItem);
	//int Container_Type( );
	int Container_ItemCount();
	float Container_Weight();
	bool Container_IsOpen();
	float Volume();

	CGenericItem *m_pParentContainer = nullptr;

	//Drinkables
	void RegisterDrinkable();
	void Drink();
	void DrinkThink();
	void DrinkCancel();
	void DrinkSetAmt(int iAmount);
	int DrinkGetAmt();

	void RegisterArmor();
	inline bool IsArmor() { return FBitSet(MSProperties(), ITEM_ARMOR) ? true : false; }
	int Armor_GetBody(int Bodypart);
#ifdef VALVE_DLL
	float Armor_Protect(damage_t Damage);
#endif

	//Spells
	float Spell_TimePrepare; //Time it takes to prepare this spell
							 //Dynamic
	float Spell_TimeCast;	 //The time I cast this spell
	bool Spell_CastSuccess;	 //Whether the cast was a success

#ifdef VALVE_DLL
	void RegisterSpell();
	bool Spell_LearnSpell(const char *pszSpellName);
	void Spell_Think();
	bool Spell_Prepare();
	void Spell_Deactivate();
#endif
	bool Spell_CanAttack();

#ifndef VALVE_DLL
	enum {
		ITEMENT_NORMAL,	   //Normal thirdperson player.  Change this one, and the rest copy it at rendertime
		ITEMENT_3DINSET,   //0 - 3D Inset
		ITEMENT_INVENTORY, //1 - Inventory floating item
		ITEMENT_TOTAL,
	};
	cl_entity_t m_ClEntity[ITEMENT_TOTAL];
#endif

#ifndef VALVE_DLL
	void Idle(void);
	int GetViewModelID();
#endif

#ifdef VALVE_DLL
	virtual void Move(void);
	virtual void Fall(void);
	virtual bool IsInAttackStance();
	float Give(enum givetype_e Type, float Amt);
	void OwnerTakeDamage(damage_t &Damage); //Modifies Damage through the item script

	//Misc
	virtual void ActivateShield(char *pszShieldAreaModel);

	//Projectiles
	virtual void RegisterProjectile();
	virtual void TossProjectile(CBaseEntity *pTossDevice, Vector &vOrigin, Vector &vVelocity, float flDamage = -99999.0f, int iStat = 0, int iProp = 0);
	virtual void ProjectileTouch(CBaseEntity *pOther);
	virtual void Projectile_Move();
	virtual void Projectile_Solidify();
	virtual void Projectile_CheckHit();
	virtual void Projectile_TouchEnable(bool touchon); //Thothie DEC2012_12 - Enable game_touch for projectiles
	virtual void Touch(CBaseEntity *pOther);		   //DEC2012_12 - Thothie - re-add climb arrows in non-sploity fashion
													   //virtual void Used( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ); //DEC2012_12 - Thothie - re-add climb arrows in non-sploity fashion
#endif

	~CGenericItem();
	CMSMonster *Owner();
	virtual float Weight();
	virtual void AddToOwner(CMSMonster *pMonster);
	virtual bool PutInAnyPack(CBasePlayer *pPlayer, bool fVerbose);
	virtual CGenericItem *FindPackForItem(CBasePlayer *pPlayer, bool fVerbose);
	virtual void FinishAttack(void);
	virtual void DelayedRemove();
	virtual bool GiveTo(CMSMonster *pReciever, bool fSound = true, bool fAllowPutInPack = true, bool fPutItemsAway = false);
	virtual void CounterEffect(CBaseEntity *pInflictor, int iEffect, void *pExtraData);
	virtual void Drop(/*int ParamsFilled = 0, const Vector &Velocity = NULL, const Vector &Angles = NULL, const Vector &Origin = NULL*/);
	virtual void Wield(void);
	virtual void UnWield(void);
	virtual bool UseItem(bool fVerbose = true);
	virtual bool PutAway(bool fVerbose = true); //Wears a wearable, puts an item in pack, or fizzles a spell
	virtual bool CanPutinInventory();			//Can be worn OR put in a pack
	virtual bool CanWearItem(void);
	virtual bool WearItem(void);
	virtual bool CanPutInPack(CGenericItem *pContainer);
	virtual bool PutInPack(CGenericItem *pPack);
	virtual void RemoveFromOwner();
	virtual void RemoveFromContainer();
	virtual void ResetAttack(void);
	virtual bool ActivatedByOwner(void); //Whether the owner is currently 'using' this item (trying to attack, drink, hold shield up, etc)
	virtual void ItemPostFrame();
	virtual bool CanReload(void) { return true; }
	virtual void ListContents();

	//Overridden
	void Deactivate(void);
	void Spawn(void);
	int iItemSlot(void) { return 4; }
	int GetItemInfo(ItemInfo *p);
	void ActivateButtonDown(void);
	void ActivateButtonUp(void);
	void Attack2ButtonDown(void);
	void Attack2ButtonUp(void);
	void AllButtonsReleased(void);
	bool ShouldIdle(void);
	bool Deploy(void);
	bool CanHolster(void);
	bool CanDrop();
	void Holster();
	void FallInit();
	void FallThink();
	void Think(void);
	void SUB_Remove(void);

	//IScripted
	void Script_Setup();																				  //Ties m_pScriptActions and m_pScriptConditions to
	bool Script_ExecuteCmd(CScript *Script, SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params); //Runs a single command

	//Drop double checking
	bool bDropAttempted = false;
	int iDropTickCounter = 0;
};
#ifdef VALVE_DLL
void SendGenericItem(CBasePlayer *pPlayer, CGenericItem *pItem, bool fNewMessage);
//void SendGenericItem(CBasePlayer *pPlayer, CGenericItem *pItem, bool fNewMessage);
#else
CGenericItem *ReadGenericItem(bool fAllowCreateNew);
#endif

void BeamEffect(Vector vStart, Vector vEnd, int sprite, int startframe,
				int framerate, int life, int width, int noise,
				int r, int g, int b, int brightness, int ispeed);
void BeamEffect(float SRCx, float SRCy, float SRCz, float DESTx,
				float DESTy, float DESTz, int sprite, int startframe,
				int framerate, int life, int width, int noise,
				int r, int g, int b, int brightness, int ispeed);
