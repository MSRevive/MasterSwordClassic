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
/*

===== player.cpp ========================================================

  functions dealing with the player

*/

#include <climits>
#include <cmath>
#include "msdllheaders.h"
#include "trains.h"
#include "nodes.h"
#include "soundent.h"
#include "monsters.h"
#include "../engine/shake.h"
#include "decals.h"
#include "gamerules/gamerules.h"
#include "decals.h"
#include "game.h"
#include "hltv.h"

//MasterSword
#include "msitemdefs.h"
#include "modeldefs.h"
#include "stats/statdefs.h"
#include "stats/stats.h"
#include "stats/races.h"
#include "player.h"
#include "weapons/genericitem.h"
#include "script.h"
//#include "monsters/bodyparts/bodyparts_Human.h"
#include "monsters/corpse.h"
#include "svglobals.h"
#include "effects/mseffects.h"
#include "vgui_menudefsshared.h"
#include "store.h" //CStore
#include "syntax/syntax.h"
#include "monsters/monsteranimation.h"
#include "gamerules/teams.h"
#include "global.h"
#include "mscharacter.h"
#include "magic.h"
#include "logger.h"
#include "fn/FNSharedDefs.h"

#define MAX_ENTITIES_TO_SEARCH 4096
static CBaseEntity* g_pEntitiesInBox[MAX_ENTITIES_TO_SEARCH];

clientitem_t::clientitem_t(class CGenericItem *pItem) : genericitem_t(pItem)
{
	Location = pItem->m_Location;
	Hand = pItem->m_Hand;
}

//------

CGenericItem *FindParryWeapon(CMSMonster *pMonster, /*out*/ int &iPlayerHand, /*out*/ int &iAttackNum);

//DLL_GLOBAL ULONG		g_ulModelRefHuman;
extern DLL_GLOBAL BOOL g_fGameOver;
extern DLL_GLOBAL BOOL g_fDrawLines;
int gEvilImpulse101;
extern DLL_GLOBAL int g_iSkillLevel, gDisplayTitle;
BOOL gInitHUD = TRUE;

extern void respawn(entvars_t *pev, BOOL fCopyCorpse);
extern Vector VecBModelOrigin(entvars_t *pevBModel);
int MapTextureTypeStepType(char chTextureType);

// the world node graph
extern CGraph WorldGraph;

#define PLAYER_WALLJUMP_SPEED 300 // how fast we can spring off walls
#define PLAYER_LONGJUMP_SPEED 350 // how fast we longjump

#define TRAIN_ACTIVE 0x80
#define TRAIN_NEW 0xc0
#define TRAIN_OFF 0x00
#define TRAIN_NEUTRAL 0x01
#define TRAIN_SLOW 0x02
#define TRAIN_MEDIUM 0x03
#define TRAIN_FAST 0x04
#define TRAIN_BACK 0x05

#define FLASH_DRAIN_TIME 1.2  //100 units/3 minutes
#define FLASH_CHARGE_TIME 0.2 // 100 units/20 seconds  (seconds per unit)

//#define PLAYER_MAX_SAFE_FALL_DIST	20// falling any farther than this many feet will inflict damage
//#define	PLAYER_FATAL_FALL_DIST		60// 100% damage inflicted if player falls this many feet
//#define	DAMAGE_PER_UNIT_FALLEN		(float)( 100 ) / ( ( PLAYER_FATAL_FALL_DIST - PLAYER_MAX_SAFE_FALL_DIST ) * 12 )
//#define MAX_SAFE_FALL_UNITS			( PLAYER_MAX_SAFE_FALL_DIST * 12 )

// Global Savedata for player
/*TYPEDESCRIPTION	CBasePlayer::m_playerSaveData[] =
{
//	DEFINE_FIELD( CBasePlayer, m_flFlashLightTime, FIELD_TIME ),
//	DEFINE_FIELD( CBasePlayer, m_iFlashBattery, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_afButtonLast, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonPressed, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonReleased, FIELD_INTEGER ),

	DEFINE_ARRAY( CBasePlayer, m_rgItems, FIELD_INTEGER, MAX_ITEMS ),
	DEFINE_FIELD( CBasePlayer, m_afPhysicsFlags, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_flTimeStepSound, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flTimeWeaponIdle, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flSwimTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flDuckTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flWallJumpTime, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_flSuitUpdate, FIELD_TIME ),
//	DEFINE_ARRAY( CBasePlayer, m_rgSuitPlayList, FIELD_INTEGER, CSUITPLAYLIST ),
	DEFINE_FIELD( CBasePlayer, m_iSuitPlayNext, FIELD_INTEGER ),
//	DEFINE_ARRAY( CBasePlayer, m_rgiSuitNoRepeat, FIELD_INTEGER, CSUITNOREPEAT ),
//	DEFINE_ARRAY( CBasePlayer, m_rgflSuitNoRepeatTime, FIELD_TIME, CSUITNOREPEAT ),
	DEFINE_FIELD( CBasePlayer, m_lastDamageAmount, FIELD_INTEGER ),

	DEFINE_ARRAY( CBasePlayer, m_rgpPlayerItems, FIELD_CLASSPTR, MAX_ITEM_TYPES ),
	DEFINE_FIELD( CBasePlayer, m_pActiveItem, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBasePlayer, m_pLastItem, FIELD_CLASSPTR ),

	DEFINE_ARRAY( CBasePlayer, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS ),
	DEFINE_FIELD( CBasePlayer, m_idrowndmg, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_idrownrestored, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_tSneaking, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_iTrain, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_bitsHUDDamage, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_flFallVelocity, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_iTargetVolume, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iWeaponVolume, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iExtraSoundTypes, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iWeaponFlash, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_fLongJump, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_fInitHUD, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_tbdPrev, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_pTank, FIELD_EHANDLE ),
	DEFINE_FIELD( CBasePlayer, m_iHideHUD, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iFOV, FIELD_INTEGER ),

	//DEFINE_FIELD( CBasePlayer, m_fDeadTime, FIELD_FLOAT ), // only used in multiplayer games
	//DEFINE_FIELD( CBasePlayer, m_fGameHUDInitialized, FIELD_INTEGER ), // only used in multiplayer games
	//DEFINE_FIELD( CBasePlayer, m_flStopExtraSoundTime, FIELD_TIME ),
	//DEFINE_FIELD( CBasePlayer, m_fKnownItem, FIELD_INTEGER ), // reset to zero on load
	//DEFINE_FIELD( CBasePlayer, m_iPlayerSound, FIELD_INTEGER ),	// Don't restore, set in Precache()
	//DEFINE_FIELD( CBasePlayer, m_pentSndLast, FIELD_EDICT ),	// Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flSndRoomtype, FIELD_FLOAT ),	// Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flSndRange, FIELD_FLOAT ),	// Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_fNewAmmo, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flgeigerRange, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_flgeigerDelay, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_igeigerRangePrev, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_iStepLeft, FIELD_INTEGER ), // Don't need to restore
	//DEFINE_ARRAY( CBasePlayer, m_szTextureName, FIELD_CHARACTER, CBTEXTURENAMEMAX ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_chTextureType, FIELD_CHARACTER ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_fNoPlayerSound, FIELD_BOOLEAN ), // Don't need to restore, debug
	//DEFINE_FIELD( CBasePlayer, m_iUpdateTime, FIELD_INTEGER ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_iClientHealth, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_iClientBattery, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_iClientHideHUD, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_fWeapon, FIELD_BOOLEAN ),  // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_nCustomSprayFrames, FIELD_INTEGER ), // Don't restore, depends on server message after spawning and only matters in multiplayer
	//DEFINE_FIELD( CBasePlayer, m_vecAutoAim, FIELD_VECTOR ), // Don't save/restore - this is recomputed
	//DEFINE_ARRAY( CBasePlayer, m_rgAmmoLast, FIELD_INTEGER, MAX_AMMO_SLOTS ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_fOnTarget, FIELD_BOOLEAN ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_nCustomSprayFrames, FIELD_INTEGER ), // Don't need to restore

};	*/

int giPrecacheGrunt = 0;
int gmsgShake = 0;
int gmsgFade = 0;
//int gmsgSelAmmo = 0;
//int gmsgFlashlight = 0;
//int gmsgFlashBattery = 0;
int gmsgResetHUD = 0;
int gmsgInitHUD = 0;
int gmsgShowGameTitle = 0;
int gmsgCurWeapon = 0;
int gmsgDamage = 0;
int gmsgTrain = 0;
int gmsgWeaponList = 0;
int gmsgHudText = 0;
int gmsgDeathMsg = 0;
int gmsgScoreInfo = 0;
int gmsgTeamInfo = 0;
int gmsgTeamScore = 0;
int gmsgGameMode = 0;
int gmsgMOTD = 0;
int gmsgAmmoPickup = 0;
int gmsgWeapPickup = 0;
int gmsgItemPickup = 0;
int gmsgHideWeapon = 0;
int gmsgSetCurWeap = 0;
int gmsgSayText = 0;
int gmsgTextMsg = 0;
int gmsgSetFOV = 0;
int gmsgShowMenu = 0;
int gmsgGeigerRange = 0;
//SDK 2.3
int gmsgStatusText = 0;
int gmsgStatusValue = 0;

//Master Sword
//These correspond to the netmsg_e enums in player.h
const char* g_PlayerMsgs[NETMSG_NUM] =
{
	"SetProp",	 // MiBNov/Dec2007 - Part of the name/title changing system.
	"StatusIcons", //Drigien MAY2008 - Status Icons
	"Anim",
	"Fatigue",
	"Music",
	"Hands",
	"SetStat",
	"Spells",
	"HP",
	"MP",
	"PlayEvent",
	"CLDllFunc",
	"VGUIMenu",
	"EntInfo",
	"StoreItem",
	"Vote",
	"HUDInfoMsg",
	"CharInfo",
	"Item",
	"Exp",		//Search shuri
	"CLXPlay",	//MAR2012_28 - client side sound
	"LocalPanel", // MiB MAR2015_01 [LOCAL_PANEL] - Message for local panel
};
int g_netmsg[NETMSG_NUM] = {0};

//-------------

void LinkUserMessages(void)
{
	// Already taken care of?
	if (gmsgGeigerRange)
	{
		return;
	}

	//gmsgSelAmmo = REG_USER_MSG("SelAmmo", sizeof(SelAmmo));
	//gmsgCurWeapon = REG_USER_MSG("CurWeapon", 3);
	gmsgGeigerRange = REG_USER_MSG("Geiger", 1);
	gmsgDamage = REG_USER_MSG("Damage", 12);
	gmsgTrain = REG_USER_MSG("Train", 1);
	gmsgHudText = REG_USER_MSG("HudText", -1);
	gmsgSayText = REG_USER_MSG("SayText", -1);
	gmsgTextMsg = REG_USER_MSG("TextMsg", -1);
	gmsgWeaponList = REG_USER_MSG("WeaponList", -1);
	gmsgResetHUD = REG_USER_MSG("ResetHUD", -1); // called every respawn
	gmsgInitHUD = REG_USER_MSG("InitHUD", -1);	 // called every time a new player joins the server
	gmsgShowGameTitle = REG_USER_MSG("GameTitle", 1);
	gmsgDeathMsg = REG_USER_MSG("DeathMsg", -1);
	gmsgScoreInfo = REG_USER_MSG("ScoreInfo", -1);
	gmsgTeamInfo = REG_USER_MSG("TeamInfo", -1);   // sets the name of a player's team
	gmsgTeamScore = REG_USER_MSG("TeamScore", -1); // sets the score of a team on the scoreboard
	gmsgGameMode = REG_USER_MSG("GameMode", 1);
	gmsgMOTD = REG_USER_MSG("MOTD", -1);
	gmsgAmmoPickup = REG_USER_MSG("AmmoPickup", 2);
	gmsgWeapPickup = REG_USER_MSG("WeapPickup", 1);
	gmsgItemPickup = REG_USER_MSG("ItemPickup", -1);
	gmsgHideWeapon = REG_USER_MSG("HideWeapon", 1);
	gmsgSetFOV = REG_USER_MSG("SetFOV", 1);
	gmsgShowMenu = REG_USER_MSG("ShowMenu", -1);
	gmsgShake = REG_USER_MSG("ScreenShake", sizeof(ScreenShake));
	gmsgFade = REG_USER_MSG("ScreenFade", sizeof(ScreenFade));

	//Master Sword
	for (int i = 0; i < NETMSG_NUM; i++)
		g_netmsg[i] = REG_USER_MSG(g_PlayerMsgs[i], -1);
	//------------

	//SDK 2.3
	gmsgStatusText = REG_USER_MSG("StatusText", -1);
	gmsgStatusValue = REG_USER_MSG("StatusValue", 3);
}

LINK_ENTITY_TO_CLASS(player, CBasePlayer);

/*void CBasePlayer :: Pain( void )
{
	float	flRndSound;//sound randomizer

	flRndSound = RANDOM_FLOAT ( 0 , 1 );

	if ( flRndSound <= 0.33 )
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM);
	else if ( flRndSound <= 0.66 )
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM);
	else
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM);
}*/

/*
 *
 */
Vector VecVelocityForDamage(float flDamage)
{
	Vector vec(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));

	if (flDamage > -50)
		vec = vec * 0.7;
	else if (flDamage > -200)
		vec = vec * 2;
	else
		vec = vec * 10;

	return vec;
}

int TrainSpeed(int iSpeed, int iMax)
{
	float fSpeed, fMax;
	int iRet = 0;

	fMax = (float)iMax;
	fSpeed = iSpeed;

	fSpeed = fSpeed / fMax;

	if (iSpeed < 0)
		iRet = TRAIN_BACK;
	else if (iSpeed == 0)
		iRet = TRAIN_NEUTRAL;
	else if (fSpeed < 0.33)
		iRet = TRAIN_SLOW;
	else if (fSpeed < 0.66)
		iRet = TRAIN_MEDIUM;
	else
		iRet = TRAIN_FAST;

	return iRet;
}

const char *CBasePlayer ::DisplayName()
{
	return g_engfuncs.pfnInfoKeyValue(g_engfuncs.pfnGetInfoKeyBuffer(edict()), "name");
}

void CBasePlayer ::DeathSound(void)
{
	//Master Sword: Don't play this if you got splattered
	//if( !FBitSet(pev->effects,EF_NODRAW) )
	//	PlaySound( CHAN_VOICE, "player/death.wav", 1, true );

	STOP_SOUND(edict(), CHAN_ITEM, "common/null.wav");
	STOP_SOUND(edict(), CHAN_BODY, "common/null.wav");
	STOP_SOUND(edict(), CHAN_WEAPON, "common/null.wav");
}

// TakeHealth - Called by a trigger_hurt with a dmg value of < 0
// bitsDamageType indicates type of damage healed.

int CBasePlayer ::TakeHealth(float flHealth, int bitsDamageType)
{
	return GiveHP(flHealth);
}

Vector CBasePlayer ::GetGunPosition()
{
	//	UTIL_MakeVectors(pev->v_angle);
	//	m_HackedGunPos = pev->view_ofs;
	Vector origin;

	origin = pev->origin + pev->view_ofs;
	return origin;
}

//=========================================================
// TraceAttack
//=========================================================
float CBasePlayer ::TraceAttack(damage_t &Damage)
{
	if (!pev->takedamage)
		return 0;

	//if( Body ) Damage.flDamage = Body->Bodypart(HitGroupToBodyPart(m_LastHitGroup))->TraceAttack( m_LastHitGroup, pInflictor, pAttacker, Damage.flDamage, Damage.iDamageType );

	Damage.iHitGroup = m_LastHitGroup = RANDOM_LONG(0, HUMAN_BODYPARTS - 1);
	if (Damage.outTraceResult.pHit)
		Damage.iHitGroup = m_LastHitGroup = HitGroupToBodyPart(Damage.outTraceResult.iHitgroup);

	//Let armor and shield do their magic
	for (int i = 0; i < Gear.size(); i++)
		Gear[i]->OwnerTakeDamage(Damage);

	if (Damage.flDamage <= 0)
		Damage.flDamage = 0;

	//flDamage = CMSMonster::TraceAttack( pevInflictor, pevAttacker, flDamage, vecDir, ptr, bitsDamageType, iAccuracyRoll );
	Damage.flDamage = CMSMonster::TraceAttack(Damage);

	if (Damage.flDamage == -1)
	{
		//THOTHIE - uncommented in attempt to restore game_parry script function
		//New parry effect - make it look like you dodged
		pev->punchangle = Vector(0, 0, RANDOM_LONG(0, 1) ? 10 : -10); //thothie was 15 : -15);
		if (Damage.pParryItem)
			Damage.pParryItem->CallScriptEvent("game_parry");

		//OLD parry
		//Parry with a weapon, if available

		/*int iAttackNum = 0, iHand = 0;;
		CGenericItem *pHandItem = FindParryWeapon( this, iHand, iAttackNum );

		if( pHandItem )
		{
			pHandItem->Script->RunScriptEventByName( "parry" );
			//Removed switching to the parry weapon and moving the player's view to face the enemy.  Too annoying
			pHandItem->CounterEffect( MSInstance(ENT(pevInflictor)), CE_SHIELDHIT, (void *)&flDamage );

			if( iCurrentHand != iHand )
			{
				if( CH && ((CGenericItem *)CH)->CurrentAttack )
					((CGenericItem *)CH)->CancelAttack( );
				SwitchHands( iHand, FALSE );
			}

			if( iCurrentHand == iHand )
			{
				m_ClientCurrentHand = iHand;
				if( !FInViewCone(&pevInflictor->origin) )
				{
					pev->angles.y = UTIL_VecToAngles(pevInflictor->origin - pev->origin).y;
					pev->fixangle = 1;
				}
				MESSAGE_BEGIN( MSG_ONE, gmsgCLDllFunc, NULL, pev );
					WRITE_BYTE( 10 );
					WRITE_BYTE( iHand );
					WRITE_BYTE( iAttackNum );
				MESSAGE_END();
			}
		}*/
	}

	return Damage.flDamage;
}

/*
	Take some damage.
	NOTE: each call to TakeDamage with bitsDamageType set to a time-based damage
	type will cause the damage time countdown to be reset.  Thus the ongoing effects of poison, radiation
	etc are implemented with subsequent calls to TakeDamage using DMG_GENERIC.
*/

int CBasePlayer ::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	// have suit diagnose the problem - ie: report damage type
	int bitsDamage = bitsDamageType;
	int ffound = TRUE;
	int fTookDamage;
	//float flHealthPrev = pev->health;

	// Already dead
	if (!IsAlive())
		return 0;
	// go take the damage first

	CBaseEntity *pAttacker = CBaseEntity::Instance(pevAttacker);

	if (!g_pGameRules->FPlayerCanTakeDamage(this, pAttacker))
	{
		// Refuse the damage
		return 0;
	}

	// keep track of amount of damage last sustained
	m_lastDamageAmount = flDamage;

	// Master Sword - int cast UNDONE!  This is checked in CBaseMonster::TakeDamage()
	// this cast to INT is critical!!! If a player ends up with 0.5 health, the engine will get that
	// as an int (zero) and think the player is dead! (this will incite a clientside screentilt, etc)
	fTookDamage = CMSMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	/*if( pev->health < 1.0f )
	{
		if( FBitSet(bitsDamageType,DMG_NOKILL) ) pev->health = HP = 1;
		else Killed( pevAttacker, GIB_NEVER );
	}*/

	// reset damage time countdown for each type of time based damage player just sustained

	{
		static int damageTypes[CDMG_TIMEBASED] = {
			DMG_PARALYZE, DMG_NERVEGAS, DMG_POISON, DMG_RADIATION, DMG_DROWNRECOVER, DMG_ACID, DMG_SLOWBURN, DMG_SLOWFREEZE
		};
		for (int i = 0; i < CDMG_TIMEBASED; i++)
		{
			if (bitsDamageType & damageTypes[i])
				m_rgbTimeBasedDamage[i] = 0;
		}
	}

	// tell director about it
	MESSAGE_BEGIN(MSG_SPEC, SVC_DIRECTOR);
	WRITE_BYTE(9);							  // command length in bytes
	WRITE_BYTE(DRC_CMD_EVENT);				  // take damage event
	WRITE_SHORT(ENTINDEX(this->edict()));	  // index number of primary entity
	WRITE_SHORT(ENTINDEX(ENT(pevInflictor))); // index number of secondary entity
	WRITE_LONG(5);							  // eventflags (priority and flags)
	MESSAGE_END();

	m_bitsDamageType |= bitsDamage; // Save this so we can report it to the client
	m_bitsHUDDamage = -1;			// make sure the damage bits get resent

	return fTookDamage;
}
void CBasePlayer::TakeDamageEffect(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	// grab the vector of the incoming attack. ( pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	/*Vector vecDir = Vector( 0, 0, 0 );

	if (pInflictor) {
		vecDir = ( pInflictor->Center() - Vector ( 0, 0, 10 ) - Center() ).Normalize();
		vecDir = g_vecAttackDir = vecDir.Normalize();
	}*/

	//Red tint and a little punch
	if (flDamage > 0 && IsAlive())
	{
		float Amt = flDamage / MaxHP() * 255;
		int alpha = V_max(Amt, 0);

		if (flDamage > 0.5) //If too small, don't even waste the bandwidth
			UTIL_ScreenFade(this, Vector(255, 0, 0), 1, 0.5, alpha, FFADE_IN);

		float flPunch = alpha * 0.25;
		pev->punchangle.x += RANDOM_FLOAT(-flPunch, flPunch) * 0.5;
		pev->punchangle.y += RANDOM_FLOAT(-flPunch, flPunch) * 0.1;
		pev->punchangle.z += RANDOM_FLOAT(-flPunch, flPunch) * 0.2;
	}
}

//=========================================================
// PackDeadPlayerItems - call this when a player dies to
// pack up the appropriate weapons and ammo items, and to
// destroy anything that shouldn't be packed.
//
// This is pretty brute force :(
//=========================================================
void CBasePlayer::PackDeadPlayerItems(void)
{
	if (m_fDropAllItems)
		RemoveAllItems(TRUE); // now strip off everything the player has
	m_fDropAllItems = false;
}

/*
 * GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
 *
 * ENTITY_METHOD(PlayerDie)
 */
entvars_t *g_pevLastInflictor; // Set in combat.cpp.  Used to pass the damage inflictor for death messages.
							   // Better solution:  Add as parameter to all Killed() functions.

void CBasePlayer::Killed(entvars_t *pevAttacker, int iGib)
{
	UTIL_ClientPrintAll(HUD_PRINTCENTER, UTIL_VarArgs("%s has fallen!", DisplayName()));

	//	if( AwardFrags )
	//		g_pGameRules->PlayerKilled( this, pevAttacker, g_pevLastInflictor );

	enum
	{
		KILLED_BY_MONSTER,
		KILLED_BY_SELF,
		KILLED_BY_TRIGGER,
		KILLED_BY_UNKNOWN
	} DeathType = KILLED_BY_UNKNOWN;
	int LoseGoldPercent = 0; //Amount of Gold to put on corpse

	CBaseEntity *pKillerEnt = (CBaseEntity *)MSInstance(pevAttacker);
	if (pKillerEnt)
	{
		if (pKillerEnt->MyMonsterPointer())
		{
			if (pKillerEnt == this)
				DeathType = KILLED_BY_SELF;
			else
				DeathType = KILLED_BY_MONSTER;
		}
		else
			DeathType = KILLED_BY_TRIGGER;
	}

	// Adrian: always make the players non-solid in multiplayer when they die
	if (g_pGameRules->IsMultiplayer())
	{
		pev->solid = SOLID_NOT;
	}

	if (DeathType == KILLED_BY_MONSTER)
	{
		//Lose about 50% of your gold
		//LoseGoldPercent = RANDOM_LONG(40,60);

		//Lose all items
		m_fDropAllItems = false;

		//Give monsters (not players) health for killing the player

		//THOTHIE: This makes high level mons neigh impossible to overwhelm with numbers
		//consider removing (compromize, reducing to 25%)
		//MAR2008b - changing system and moving to script side
		if (pKillerEnt->MyMonsterPointer() && !pKillerEnt->IsPlayer())
		{
			CMSMonster *pMonster = (CMSMonster *)pKillerEnt;
			static msstringlist Params;
			Params.clearitems();
			Params.add(EntToString(this));
			pMonster->CallScriptEvent("game_killed_player", &Params);
		}

		if (pKillerEnt->IsPlayer())
		{
			//A player killed me
			CBasePlayer *pPlayerKiller = (CBasePlayer *)pKillerEnt;
			if (!m_PlayersKilled && !m_TimeWaitedToForgetSteal)
			{
				//I'm just an innocent player
				pPlayerKiller->m_PlayersKilled++;
				pPlayerKiller->m_TimeWaitedToForgetKill = 0;
				m_LastPlayerToKillMe = pPlayerKiller->entindex();
				m_pLastPlayerToKillMe = pPlayerKiller;
				SendHUDMsg("Player Kill", msstring("*** ") + pPlayerKiller->DisplayName() + " has killed you!  If this was accidental, press F4 (forgive)\nto remove the crime from " + pPlayerKiller->DisplayName() + "'s record! ***");
			}
			else if (m_PlayersKilled > 0)
			{
				//I'm a player killer
				//int Reward = RANDOM_LONG( 10, m_PlayersKilled * 50 );
				//pPlayerKiller->SendInfoMsg( "*** You recieve a reward of %i gold for killing %s ***\n", STRING(pPlayerKiller->DisplayName), STRING(pPlayerKiller->DisplayName) );
			}
		}
	}

	//Thothie - removing XP penalty for being killed by trigger
	//Thothie APR2016_10
	//- Adding marker for specific NPCs not to cause XP loss when they kill
	//- setvar NPC_NO_XP_PENALTY 1
	bool no_xp_pen = false;
	if (pKillerEnt)
	{
		IScripted* pKillerScript = pKillerEnt->GetScripted();
		if (pKillerScript)
		{
			if (atof(pKillerScript->GetFirstScriptVar("NPC_NO_XP_PENALTY")) == 1)
				no_xp_pen = true;
		}
	}

	if ((DeathType == KILLED_BY_MONSTER) && pKillerEnt && !pKillerEnt->IsPlayer() && !no_xp_pen)
	{
		//THOTHIE wants this so you lose:
		//- 1% of gold on hand :)
		//Contribs: HobbitG

		float DeathTax = 0.01;
		int TaxOut = m_Gold;

		TaxOut *= DeathTax;
		GiveGold(-TaxOut, false);
		SendEventMsg(HUDEVENT_UNABLE, UTIL_VarArgs("Death Penalty: Lost %i gp \n", TaxOut));
	} //Endif DeathType == KILLED_BY_MONSTER

	//if( OpenPack ) OpenPack->Container_UnListContents( );
	CancelAttack();

	if (FBitSet(pev->flags, FL_DUCKING))
	{
		CinematicCamera(FALSE);
		ClearBits(pev->flags, FL_DUCKING);
		ClearBits(m_afPhysicsFlags, PFLAG_DUCKING);
		pev->origin.z = pev->origin.z + ((Size(0).z / 2) - (Size(FL_DUCKING).z / 2));
		SetSize(pev->flags);
		UTIL_SetOrigin(pev, pev->origin);
	}
	//	if( FBitSet( SkillInfo.SkillsActivated, SKILL_ROGUE_FADE ) )
	//		SkillInfo.FadeAmt = -2; //Silently deactivate skill

	CSound *pSound;

	if (m_pTank != NULL)
	{
		m_pTank->Use(this, this, USE_OFF, 0);
		m_pTank = NULL;
	}

	// this client isn't going to be thinking for a while, so reset the sound until they respawn
	pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));
	{
		if (pSound)
		{
			pSound->Reset();
		}
	}

	/*m_Activity = GetDeathActivity( );
	SetAnimation( MONSTER_ANIM_BREAK );
	SetAnimation( MONSTER_ANIM_DIE, NULL, (void *)m_Activity );*/

	//pev->modelindex = g_ulModelRefHuman;    // don't use eyes

	pev->health = 0;
	pev->deadflag = DEAD_DYING;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_TOSS;
	ClearBits(pev->flags, FL_ONGROUND);

	// reset FOV
	m_iFOV = m_iClientFOV = 0;

	MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, NULL, pev);
	WRITE_BYTE(0);
	MESSAGE_END();

	//Call the client killed function
	MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_CLDLLFUNC], NULL, pev);
	WRITE_BYTE(1);
	MESSAGE_END();

	// UNDONE: Put this in, but add FFADE_PERMANENT and make fade time 8.8 instead of 4.12
	UTIL_ScreenFade(this, Vector(255, 0, 0), 0.2, 15, 128, FFADE_IN);

	SetThink(&CBasePlayer::PlayerDeathThink);
	pev->nextthink = gpGlobals->time + 0.1;

	// UNDONE: No more gibbing bodies
	/*if ( ( pev->health < PLAYER_GIB_HEATH && iGib != GIB_NEVER ) || iGib == GIB_ALWAYS )
	{
		GibMonster(); // This clears pev->model
		pev->effects |= EF_NODRAW;
		return;
	}*/

	DeathSound();
	// Origin/Angles for the Camera position
	Vector vOrigin = pev->origin + gpGlobals->v_right * 70 + Vector(0, 0, 25);
	TraceResult tr;
	UTIL_TraceLine(pev->origin, vOrigin, dont_ignore_monsters, edict(), &tr);
	if (tr.flFraction < 1.0)
		vOrigin = tr.vecEndPos;
	Vector vAngles = UTIL_VecToAngles(vOrigin - pev->origin);

	vAngles.y += 180;
	pev->angles.x = 0;
	pev->angles.z = 0; //0

	//pev->gaitsequence = 0;

	//This already runs on CMSMonster
	/*
	//MiB Dec2007a - For removing effects on death
	if (m_Scripts.size() > 0)
	{
		IScripted *pScripted = GetScripted(); // UScripted? IScripted.
		if (pScripted)
		{
			for (int i = 0; i < pScripted->m_Scripts.size(); i++)							 // Check each
			{
				if (m_Scripts[i]->VarExists("game.effect.id"))								 //This is an effect
				{
					if (strcmp(m_Scripts[i]->GetVar("game.effect.removeondeath"), "1") == 0) //If the effect is SUPPOSED to be removed
					{
						m_Scripts[i]->RunScriptEventByName("effect_die");					 //Call this effect's die function
						m_Scripts[i]->m.RemoveNextFrame = true;
					}
				}
			}
		}
	}*/

	CallScriptEvent("game_death");
	InitBoneControllers();

	//Create a brand new corpse
	m_Corpse = (CCorpse *)GetClassPtr((CCorpse *)NULL);
	m_Corpse->CreateCorpse(this, LoseGoldPercent);

	CinematicCamera(TRUE, vOrigin, vAngles);
	pev->effects |= EF_NODRAW;
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	pev->takedamage = DAMAGE_NO;
	pev->solid = SOLID_NOT;
	SetBits(pev->flags, FL_NOTARGET);
	m_TimeTillSuicide = 0;
}

/*
===========
WaterMove
============
*/
#define AIRTIME 12 // lung full of air lasts this many seconds

void CBasePlayer::WaterMove()
{
	int air;

	if (pev->movetype == MOVETYPE_NOCLIP)
		return;

	if (pev->health < 0)
		return;

	// waterlevel 0 - not in water
	// waterlevel 1 - feet in water
	// waterlevel 2 - waist in water
	// waterlevel 3 - head in water

	if (pev->waterlevel != 3)
	{
		// not underwater

		// play 'up for air' sound
		if (pev->air_finished < gpGlobals->time)
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_wade1.wav", 1, ATTN_NORM);
		else if (pev->air_finished < gpGlobals->time + 9)
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_wade2.wav", 1, ATTN_NORM);

		pev->air_finished = gpGlobals->time + AIRTIME;
		pev->dmg = 2;

		// if we took drowning damage, give it back slowly
		if (m_idrowndmg > m_idrownrestored)
		{
			// set drowning damage bit.  hack - dmg_drownrecover actually
			// makes the time based damage code 'give back' health over time.
			// make sure counter is cleared so we start count correctly.

			// NOTE: this actually causes the count to continue restarting
			// until all drowning damage is healed.

			m_bitsDamageType |= DMG_DROWNRECOVER;
			m_bitsDamageType &= ~DMG_DROWN;
			m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
		}
	}
	else
	{ // fully under water
		// stop restoring damage while underwater
		m_bitsDamageType &= ~DMG_DROWNRECOVER;
		m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;

		if (pev->air_finished < gpGlobals->time) // drown!
		{
			if (pev->pain_finished < gpGlobals->time)
			{
				// take drowning damage
				pev->dmg += 1;
				if (pev->dmg > 5)
					pev->dmg = 5;
				if (!FBitSet(pev->flags, FL_GODMODE))
					TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), pev->dmg, DMG_DROWN);
				pev->pain_finished = gpGlobals->time + 1;

				// track drowning damage, give it back when
				// player finally takes a breath

				m_idrowndmg += pev->dmg;
			}
		}
		else
		{
			m_bitsDamageType &= ~DMG_DROWN;
		}
	}

	if (!pev->waterlevel)
	{
		if (FBitSet(pev->flags, FL_INWATER))
			ClearBits(pev->flags, FL_INWATER);
		return;
	}

	// make bubbles

	air = (int)(pev->air_finished - gpGlobals->time);
	if (!RANDOM_LONG(0, 0x1f) && RANDOM_LONG(0, AIRTIME - 1) >= air)
	{
		switch (RANDOM_LONG(0, 3))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim1.wav", 0.8, ATTN_NORM);
			break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim2.wav", 0.8, ATTN_NORM);
			break;
		case 2:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim3.wav", 0.8, ATTN_NORM);
			break;
		case 3:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim4.wav", 0.8, ATTN_NORM);
			break;
		}
	}

	if (pev->watertype == CONTENT_LAVA) // do damage
	{
		if (pev->dmgtime < gpGlobals->time)
			TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), 10 * pev->waterlevel, DMG_BURN);
	}
	else if (pev->watertype == CONTENT_SLIME) // do damage
	{
		pev->dmgtime = gpGlobals->time + 1;
		TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), 4 * pev->waterlevel, DMG_ACID);
	}

	if (!FBitSet(pev->flags, FL_INWATER))
	{
		SetBits(pev->flags, FL_INWATER);
		pev->dmgtime = 0;
	}

	//if (!FBitSet(pev->flags, FL_WATERJUMP))
	//	pev->velocity = pev->velocity - 0.8 * pev->waterlevel * gpGlobals->frametime * pev->velocity;
}

// TRUE if the player is attached to a ladder
BOOL CBasePlayer::IsOnLadder(void)
{
	return (pev->movetype == MOVETYPE_FLY);
}

void CBasePlayer::PlayerDeathThink(void)
{
	float flForward;

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		flForward = pev->velocity.Length() - 20;
		if (flForward <= 0)
			pev->velocity = g_vecZero;
		else
			pev->velocity = flForward * pev->velocity.Normalize();
	}

	// Drop the guns here because weapons that have an area effect and can kill their user
	// will sometimes crash coming back from CBasePlayer::Killed() if they kill their owner because the
	// player class sometimes is freed. It's safer to manipulate the weapons once we know
	// we aren't calling into any of their code anymore through the player pointer.

	//PackDeadPlayerItems(); //JAN2015_11 Thothie - this causes too many exploits
	//though we may need to tweak this meathod insead, if the crash issue is still there (sfaik we have no AOE weapons that can damage the user.)

	if (pev->modelindex && (!m_fSequenceFinished) && (pev->deadflag == DEAD_DYING))
	{
		StudioFrameAdvance();

		m_iRespawnFrames++;			// Note, these aren't necessarily real "frames", so behavior is dependent on # of client movement commands
		if (m_iRespawnFrames < 120) // Animations should be no longer than this
			return;
	}

	if (pev->deadflag == DEAD_DYING)
		pev->deadflag = DEAD_DEAD;

	pev->effects |= EF_NOINTERP;

	BOOL fAnyButtonDown = (pev->button & ~IN_SCORE);

	// wait for all buttons released
	if (pev->deadflag == DEAD_DEAD)
	{
		if (fAnyButtonDown)
			return;

		if (g_pGameRules->FPlayerCanRespawn(this))
		{
			m_fDeadTime = gpGlobals->time;
			pev->deadflag = DEAD_RESPAWNABLE;
		}

		return;
	}

	// wait for any button down,  or mp_forcerespawn is set and the respawn time is up
	if (!fAnyButtonDown && !(g_pGameRules->IsMultiplayer() && forcerespawn.value > 0 && (gpGlobals->time > (m_fDeadTime + 5))))
		return;

	pev->button = 0;
	m_iRespawnFrames = 0;

	respawn(pev, !(m_afPhysicsFlags & PFLAG_OBSERVER)); // don't copy a corpse if we're in deathcam.
	pev->nextthink = -1;
}

//=========================================================
// StartDeathCam - find an intermission spot and send the
// player off into observer mode
//=========================================================
void CBasePlayer::StartDeathCam(void)
{
	edict_t *pSpot, *pNewSpot;
	int iRand;

	if (pev->view_ofs == g_vecZero)
	{
		// don't accept subsequent attempts to StartDeathCam()
		return;
	}

	pSpot = FIND_ENTITY_BY_CLASSNAME(NULL, "info_intermission");

	if (!FNullEnt(pSpot))
	{
		// at least one intermission spot in the world.
		iRand = RANDOM_LONG(0, 3);

		while (iRand > 0)
		{
			pNewSpot = FIND_ENTITY_BY_CLASSNAME(pSpot, "info_intermission");

			if (pNewSpot)
			{
				pSpot = pNewSpot;
			}

			iRand--;
		}

		//		CopyToBodyQue( pev );
		StartObserver(pSpot->v.origin, pSpot->v.v_angle);
	}
	else
	{
		// no intermission spot. Push them up in the air, looking down at their corpse
		TraceResult tr;
		//		CopyToBodyQue( pev );
		UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, 128), ignore_monsters, edict(), &tr);
		StartObserver(tr.vecEndPos, UTIL_VecToAngles(tr.vecEndPos - pev->origin));
		return;
	}
}

void CBasePlayer::StartObserver(Vector vecPosition, Vector vecViewAngle)
{
	m_afPhysicsFlags |= PFLAG_OBSERVER;

	pev->view_ofs = g_vecZero;
	pev->angles = pev->v_angle = vecViewAngle;
	pev->fixangle = TRUE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	pev->modelindex = 0;
	UTIL_SetOrigin(pev, vecPosition);
}
void CBasePlayer::CinematicCamera(BOOL OnorOff, Vector vecPosition, Vector vecViewAngle, BOOL bCreateClone)
{
	if (OnorOff == TRUE)
	{
		m_afPhysicsFlags |= PFLAG_OBSERVER;
		CamEntity = GetClassPtr((CBaseEntity *)NULL);
		CamEntity->pev->classname = MAKE_STRING("camera");
		CamEntity->pev->modelindex = MODEL_INDEX("models/null.mdl");
		CamEntity->pev->renderamt = 0; // The engine won't draw this model if this is set to 0 and blending is on
		CamEntity->pev->rendermode = kRenderTransTexture;
		CamEntity->pev->solid = SOLID_NOT;

		CamEntity->pev->origin = vecPosition;
		CamEntity->pev->angles = CamEntity->pev->v_angle = vecViewAngle;
		UTIL_SetOrigin(CamEntity->pev, CamEntity->pev->origin);
		//		UTIL_SetSize(CamEntity->pev, Vector( 0, 0, 0), Vector(0, 0, 0) );
		SET_VIEW(edict(), CamEntity->edict());
	}
	else
	{
		m_afPhysicsFlags &= ~PFLAG_OBSERVER;
		SET_VIEW(edict(), edict()); // Fixes invis ents if you died b4 changelevel, disconnect, etc.
		if (CamEntity)
		{
			CamEntity->SUB_Remove();
			CamEntity = NULL;
		}
	}
}

//
// PlayerUse - handles USE keypress
//
#define PLAYER_SEARCH_RADIUS (float)64

void CBasePlayer::PlayerUse(void)
{
	// Was use pressed or released?
	if (!((pev->button | m_afButtonPressed | m_afButtonReleased) & IN_USE))
		return;

	if (HasConditions(MONSTER_OPENCONTAINER) || HasConditions(MONSTER_TRADING))
		return;

	// Hit Use on a train?
	if (m_afButtonPressed & IN_USE)
	{
		if (m_pTank != NULL)
		{
			// Stop controlling the tank
			// TODO: Send HUD Update
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = NULL;
			return;
		}
		else
		{
			if (m_afPhysicsFlags & PFLAG_ONTRAIN)
			{
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW | TRAIN_OFF;
				return;
			}
			else
			{ // Start controlling the train!
				CBaseEntity *pTrain = CBaseEntity::Instance(pev->groundentity);

				if (pTrain && !(pev->button & IN_JUMP) && FBitSet(pev->flags, FL_ONGROUND) && (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) && pTrain->OnControls(pev))
				{
					m_afPhysicsFlags |= PFLAG_ONTRAIN;
					m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
					m_iTrain |= TRAIN_NEW;
					EMIT_SOUND(ENT(pev), CHAN_ITEM, "plats/train_use1.wav", 0.8, ATTN_NORM);
					return;
				}
			}
		}
	}

	CBaseEntity *pObject = NULL;
	CBaseEntity *pClosest = NULL;
	Vector vecLOS;
	float flMaxDot = VIEW_FIELD_NARROW;
	float flDot;

	UTIL_MakeVectors(pev->v_angle); // so we know which way we are facing

	while ((pObject = UTIL_FindEntityInSphere(pObject, pev->origin, PLAYER_SEARCH_RADIUS)) != NULL)
	{

		if (pObject->ObjectCaps() & (FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE))
		{
			// !!!PERFORMANCE- should this check be done on a per case basis AFTER we've determined that
			// this object is actually usable? This dot is being done for every object within PLAYER_SEARCH_RADIUS
			// when player hits the use key. How many objects can be in that area, anyway? (sjb)
			vecLOS = (VecBModelOrigin(pObject->pev) - (pev->origin + pev->view_ofs));

			// This essentially moves the origin of the target to the corner nearest the player to test to see
			// if it's "hull" is in the view cone
			vecLOS = UTIL_ClampVectorToBox(vecLOS, pObject->pev->size * 0.5);

			flDot = DotProduct(vecLOS, gpGlobals->v_forward);
			if (flDot > flMaxDot)
			{ // only if the item is in front of the user
				pClosest = pObject;
				flMaxDot = flDot;
				//				ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
			}
			//			ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
		}
	}
	pObject = pClosest;

	// Found an object
	if (pObject)
	{
		//!!!UNDONE: traceline here to prevent USEing buttons through walls
		int caps = pObject->ObjectCaps();

		if (m_afButtonPressed & IN_USE)
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/wpn_select.wav", 0.4, ATTN_NORM);

		if (((pev->button & IN_USE) && (caps & FCAP_CONTINUOUS_USE)) ||
			((m_afButtonPressed & IN_USE) && (caps & (FCAP_IMPULSE_USE | FCAP_ONOFF_USE))))
		{
			if (caps & FCAP_CONTINUOUS_USE)
				m_afPhysicsFlags |= PFLAG_USING;

			pObject->Use(this, this, USE_SET, 1);
		}
		// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
		else if ((m_afButtonReleased & IN_USE) && (pObject->ObjectCaps() & FCAP_ONOFF_USE)) // BUGBUG This is an "off" use
		{
			pObject->Use(this, this, USE_SET, 0);
		}
	}
	else
	{
		if (m_afButtonPressed & IN_USE)
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/wpn_denyselect.wav", 0.4, ATTN_NORM);
	}
}

void CBasePlayer::Jump()
{
	//if( FBitSet(m_StatusFlags, PLAYER_MOVE_NOJUMP) ) return;

	Vector vecWallCheckDir; // direction we're tracing a line to find a wall when walljumping
	Vector vecAdjustedVelocity;
	Vector vecSpot;
	TraceResult tr;
	//BOOL bCanJump = TRUE;

	if (FBitSet(pev->flags, FL_WATERJUMP))
		return;

	if (pev->waterlevel >= 2)
	{
		return;
	}

	// jump velocity is sqrt( height * gravity * 2)

	// If this isn't the first frame pressing the jump button, break out.
	if (!FBitSet(m_afButtonPressed, IN_JUMP))
		return; // don't pogo stick

	if (!(pev->flags & FL_ONGROUND) || !pev->groundentity)
		return;

	SetBits(m_StatusFlags, PLAYER_MOVE_JUMPING);
	CallScriptEvent("game_jump");

	/*if ( FBitSet(pev->flags, FL_DUCKING ) || FBitSet(m_afPhysicsFlags, PFLAG_DUCKING) )
	{
		if ( m_fLongJump && (pev->button & IN_DUCK) && gpGlobals->time - m_flDuckTime < 1 && pev->velocity.Length() > 50 )
		{// If jump pressed within a second of duck while moving, long jump!
			SetAnimation( MONSTER_ANIM_SUPERJUMP, "longjump" );
		}
	}*/

	// If you're standing on a conveyor, add it's velocity to yours (for momentum)
	entvars_t *pevGround = VARS(pev->groundentity);
	if (pevGround && (pevGround->flags & FL_CONVEYOR))
	{
		pev->velocity = pev->velocity + pev->basevelocity;
	}
}

// This is a glorious hack to find free space when you've crouched into some solid space
// Our crouching collisions do not work correctly for some reason and this is easier
// than fixing the problem :(
void FixPlayerCrouchStuck(edict_t *pPlayer)
{
	TraceResult trace;

	// Move up as many as 18 pixels if the player is stuck.
	for (int i = 0; i < 18; i++)
	{
		UTIL_TraceHull(pPlayer->v.origin, pPlayer->v.origin, dont_ignore_monsters, head_hull, pPlayer, &trace);
		if (trace.fStartSolid)
			pPlayer->v.origin.z++;
		else
			break;
	}
}

void CBasePlayer::Duck()
{
}

void CBasePlayer::PlayerAction(msstring_ref Action)
{
	CScript *EventScript = NULL;

	for (int i = 0; i < m_Scripts.size(); i++)
		if (!strcmp(m_Scripts[i]->GetVar("game.effect.id"), Action))
		{
			EventScript = m_Scripts[i];
			break;
		}

	if (!EventScript)
		return;

	EventScript->RunScriptEventByName("game_player_activate");
}
/*void CBasePlayer::Act( int iAction )
{
	DoAction( iAction-1 );
}
void CBasePlayer::DoAction( int iAction ) {
	switch( iAction )
	{
		case ACTION_SIT:		//If standing, sit
		case ACTION_STAND:		//If sitting, stand
			if( !FBitSet(pev->flags,FL_ONGROUND) ) return;

			if( !FBitSet(m_StatusFlags,PLAYER_MOVE_SITTING) )		//Standing... try to sit
			{
				if( FBitSet(m_StatusFlags,PLAYER_MOVE_ATTACKING) )
				{
					SendEventMsg( HUDEVENT_UNABLE, "You cannot sit right now.\n" );
					return;
				}
	//			if( Wielded( 0 ) || Wielded( 1 ) ) {
	//				SendInfoMsg( "You must unwield your weapons before sitting.\n" );
	//				return;
	//			}
				//Duck
				if( !FBitSet(pev->flags,FL_DUCKING) ) {
					SetSize( FL_DUCKING );

					if ( pev->flags & FL_ONGROUND )
						pev->origin.z = pev->origin.z - ((Size(0).z/2) - (Size(FL_DUCKING).z/2));

					FixPlayerCrouchStuck( edict() );
					UTIL_SetOrigin( pev, pev->origin );

					pev->view_ofs = VEC_DUCK_VIEW;
					SetBits(pev->flags,FL_DUCKING);				// Hull is duck hull
				}

				//Sit
				SendInfoMsg( "You sit down." );
	//			strcpy( m_szAnimExtention, "sitdown" );
				SetAnimation( MONSTER_ANIM_BREAK );
				SetAnimation( MONSTER_ANIM_HOLD, "sitdown", (void *)0 );
				SetBits( m_StatusFlags, PLAYER_MOVE_SITTING );
				LockSpeed = TRUE;
				UTIL_MakeVectors( pev->angles );
				m_TimeGainHP = 0;  //Reset the gain HP time
			}
			else		//Sitting, try to stand
			{
				TraceResult trace;
				Vector newOrigin = pev->origin;

				if ( pev->flags & FL_ONGROUND )
					newOrigin.z = newOrigin.z + ((Size(0).z/2) - (Size(FL_DUCKING).z/2));

				UTIL_TraceHull( newOrigin, newOrigin, dont_ignore_monsters, human_hull, ENT(pev), &trace );

				if ( !trace.fStartSolid )
				{
					SendInfoMsg( "You stand back up." );
					//strcpy( m_szAnimExtention, m_szAnimLast );
					ClearBits( m_StatusFlags, PLAYER_MOVE_SITTING );
					CinematicCamera( FALSE );
					LockSpeed = FALSE;

					ClearBits(pev->flags,FL_DUCKING);
					ClearBits(m_afPhysicsFlags,PFLAG_DUCKING);
					m_TimeGainHP = 0;
					//pev->view_ofs = Vector( 0, 0, eyeheight );
					SetSize( pev->flags );
					pev->origin = newOrigin;
					FixPlayerCrouchStuck( edict() );
					UTIL_SetOrigin( pev, pev->origin );

					if( m_pLastAnimHandler ) m_pAnimHandler = m_pLastAnimHandler;
					else SetAnimation( MONSTER_ANIM_BREAK );
				}
				else SendEventMsg( HUDEVENT_UNABLE, "Something prevents you from standing." );
			}
			break;
	}
}*/

//
// ID's player as such.
//
int CBasePlayer::Classify(void)
{
	return CLASS_PLAYER;
}

//Player Steam ID
void CBasePlayer::InitStatusBar()
{
	m_flStatusBarDisappearDelay = 0;
	m_SbarString1[0] = m_SbarString0[0] = 0;
}

void CBasePlayer::UpdateStatusBar()
{
	int newSBarState[SBAR_END];
	char sbuf0[SBAR_STRING_SIZE];
	char sbuf1[SBAR_STRING_SIZE];

	memset(newSBarState, 0, sizeof(newSBarState));
	 strncpy(sbuf0,  m_SbarString0, sizeof(sbuf0) );
	 strncpy(sbuf1,  m_SbarString1, sizeof(sbuf1) );

	// Find an ID Target
	TraceResult tr;
	UTIL_MakeVectors(pev->v_angle + pev->punchangle);
	Vector vecSrc = EyePosition();
	Vector vecEnd = vecSrc + (gpGlobals->v_forward * MAX_ID_RANGE);
	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, edict(), &tr);

	if (tr.flFraction != 1.0)
	{
		if (!FNullEnt(tr.pHit))
		{
			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if (pEntity->Classify() == CLASS_PLAYER)
			{
				newSBarState[SBAR_ID_TARGETNAME] = ENTINDEX(pEntity->edict());
				 strncpy(sbuf1,  "1 %p1\n2 Health: %i2%%\n3 Armor: %i3%%", sizeof(sbuf1) );

				// allies and medics get to see the targets health
				if (g_pGameRules->PlayerRelationship(this, pEntity) == GR_TEAMMATE)
				{
					newSBarState[SBAR_ID_TARGETHEALTH] = 100 * (pEntity->pev->health / pEntity->pev->max_health);
					newSBarState[SBAR_ID_TARGETARMOR] = pEntity->pev->armorvalue; //No need to get it % based since 100 it's the max.
				}

				m_flStatusBarDisappearDelay = gpGlobals->time + 1.0;
			}
		}
		else if (m_flStatusBarDisappearDelay > gpGlobals->time)
		{
			// hold the values for a short amount of time after viewing the object
			newSBarState[SBAR_ID_TARGETNAME] = m_izSBarState[SBAR_ID_TARGETNAME];
			newSBarState[SBAR_ID_TARGETHEALTH] = m_izSBarState[SBAR_ID_TARGETHEALTH];
			newSBarState[SBAR_ID_TARGETARMOR] = m_izSBarState[SBAR_ID_TARGETARMOR];
		}
	}

	BOOL bForceResend = FALSE;

	if (strcmp(sbuf0, m_SbarString0))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgStatusText, NULL, pev);
		WRITE_BYTE(0);
		WRITE_STRING(sbuf0);
		MESSAGE_END();

		 strncpy(m_SbarString0,  sbuf0, sizeof(m_SbarString0) );

		// make sure everything's resent
		bForceResend = TRUE;
	}

	if (strcmp(sbuf1, m_SbarString1))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgStatusText, NULL, pev);
		WRITE_BYTE(1);
		WRITE_STRING(sbuf1);
		MESSAGE_END();

		 strncpy(m_SbarString1,  sbuf1, sizeof(m_SbarString1) );

		// make sure everything's resent
		bForceResend = TRUE;
	}

	// Check values and send if they don't match
	for (int i = 1; i < SBAR_END; i++)
	{
		if (newSBarState[i] != m_izSBarState[i] || bForceResend)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgStatusValue, NULL, pev);
			WRITE_BYTE(i);
			WRITE_SHORT(newSBarState[i]);
			MESSAGE_END();

			m_izSBarState[i] = newSBarState[i];
		}
	}
}

#define CLIMB_SHAKE_FREQUENCY 22 // how many frames in between screen shakes when climbing
#define MAX_CLIMB_SPEED 200		 // fastest vertical climbing speed possible
#define CLIMB_SPEED_DEC 15		 // climbing deceleration rate
#define CLIMB_PUNCH_X -7		 // how far to 'punch' client X axis when climbing
#define CLIMB_PUNCH_Z 7			 // how far to 'punch' client Z axis when climbing

void SetKeys(CBasePlayer *pPlayer);

void CBasePlayer::PreThink(void)
{
	startdbg;

	dbg("Begin");

	//Send char info, if character is still unloaded
	Think_SendCharData();

	dbg("Call MSChar_Interface::AutoSave");
	MSChar_Interface::AutoSave(this); //Autosave character

	dbg("Call MSChar_Interface::Think_SendChar");
	MSChar_Interface::Think_SendChar(this); //Send client-side char down to client

	int buttonsChanged = (m_afButtonLast ^ pev->button); // These buttons have changed this frame

	// Debounced button codes for pressed/released
	// UNDONE: Do we need auto-repeat?
	m_afButtonPressed = buttonsChanged & pev->button;	  // The changed ones still down are "pressed"
	m_afButtonReleased = buttonsChanged & (~pev->button); // The ones not down are "released"

	dbg("Call SetKeys");
	SetKeys();

	dbg("Call g_pGameRules->PlayerThink");
	if (g_pGameRules)
		g_pGameRules->PlayerThink(this);

	if (g_fGameOver)
		return; // intermission or finale

	dbg("Call Trade");
	Trade(); //Trade - Do this early on

	dbg("Call ItemPreFrame");
	ItemPreFrame();
	WaterMove();

	RunScriptEvents(); //RunScriptEvents

	dbg("Call UpdateClientData");
	UpdateClientData(); //UpdateClientData

	if (SpawnCheckTime > 0 && gpGlobals->time > SpawnCheckTime)
	{
		SpawnCheckTime = 0;
		dbg("Call Spawn");
		Spawn();
	}

	if (FBitSet(pev->flags, FL_SPECTATOR))
		return;

	dbg("Call CheckTimeBasedDamage");
	CheckTimeBasedDamage();

	if (pev->deadflag >= DEAD_DYING)
	{
		dbg("Call PlayerDeathThink");
		PlayerDeathThink();
		return;
	}

	// So the correct flags get sent to client asap.
	//
	if (m_afPhysicsFlags & PFLAG_ONTRAIN)
		pev->flags |= FL_ONTRAIN;
	else
		pev->flags &= ~FL_ONTRAIN;

	// Train speed control
	if (m_afPhysicsFlags & PFLAG_ONTRAIN)
	{
		CBaseEntity *pTrain = CBaseEntity::Instance(pev->groundentity);
		float vel;

		if (!pTrain)
		{
			TraceResult trainTrace;
			// Maybe this is on the other side of a level transition
			UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -38), ignore_monsters, ENT(pev), &trainTrace);

			// HACKHACK - Just look for the func_tracktrain classname
			if (trainTrace.flFraction != 1.0 && trainTrace.pHit)
				pTrain = CBaseEntity::Instance(trainTrace.pHit);

			if (!pTrain || !(pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) || !pTrain->OnControls(pev))
			{
				//ALERT( at_error, "In train mode with no train!\n" );
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW | TRAIN_OFF;
				return;
			}
		}
		else if (!FBitSet(pev->flags, FL_ONGROUND) || FBitSet(pTrain->pev->spawnflags, SF_TRACKTRAIN_NOCONTROL) || (pev->button & (IN_MOVELEFT | IN_MOVERIGHT)))
		{
			// Turn off the train if you jump, strafe, or the train controls go dead
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
			m_iTrain = TRAIN_NEW | TRAIN_OFF;
			return;
		}

		pev->velocity = g_vecZero;
		vel = 0;
		if (m_afButtonPressed & IN_FORWARD)
		{
			vel = 1;
			pTrain->Use(this, this, USE_SET, (float)vel);
		}
		else if (m_afButtonPressed & IN_BACK)
		{
			vel = -1;
			pTrain->Use(this, this, USE_SET, (float)vel);
		}

		if (vel)
		{
			m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
			m_iTrain |= TRAIN_ACTIVE | TRAIN_NEW;
		}
	}
	else if (m_iTrain & TRAIN_ACTIVE)
		m_iTrain = TRAIN_NEW; // turn off train

	if (FBitSet(m_StatusFlags, PLAYER_MOVE_JUMPING) && //Was jumping
		FBitSet(pev->flags, FL_ONGROUND))			   //Now on ground
	{
		ClearBits(m_StatusFlags, PLAYER_MOVE_JUMPING);
		CallScriptEvent("game_jump_land");
	}

	if (pev->button & IN_JUMP)
	{
		// If on a ladder, jump off the ladder
		// else Jump
		Jump();
	}

	// If trying to duck, already ducked, or in the process of ducking
	if ((pev->button & IN_DUCK) || FBitSet(pev->flags, FL_DUCKING) || (m_afPhysicsFlags & PFLAG_DUCKING))
		Duck();

	/*if( FBitSet(StatusFlags,PLAYER_MOVE_SITTING) )
	{
		Vector vForward, vUp;
		UTIL_MakeVectorsPrivate( Vector(0, pev->angles.y, 0), vForward, NULL, vUp );
		CamEntity->pev->origin = pev->origin - vForward*50 + vUp*20;
		TraceResult tr;
		UTIL_TraceLine( pev->origin, CamEntity->pev->origin, dont_ignore_monsters, edict(), &tr );
		if( tr.flFraction < 1.0 ) CamEntity->pev->origin = tr.vecEndPos;
		Vector vCamAng = UTIL_VecToAngles( (pev->origin+pev->view_ofs)-CamEntity->pev->origin );
		vCamAng.x *= -1;
		CamEntity->pev->v_angle = CamEntity->pev->angles = vCamAng;
	}*/

	// play a footstep if it's time - this will eventually be frame-based. not time based.

	//UpdateStepSound();

	if (!FBitSet(pev->flags, FL_ONGROUND))
		m_flFallVelocity = -pev->velocity.z;
	else
		m_flFallVelocity = 0;

	// StudioFrameAdvance( );//!!!HACKHACK!!! Can't be hit by traceline when not animating?

	if (m_TimeTillSuicide)
	{
		EnableControl(FALSE);
		if (gpGlobals->time >= m_TimeTillSuicide)
		{
			// have the player kill themself
			pev->health = m_HP = 0;
			Killed(pev, GIB_NEVER);
			m_TimeTillSuicide = 0;
		}
	}

	if (FBitSet(m_StatusFlags, PLAYER_MOVE_ATTACKING))
		m_TimeCanSteal = gpGlobals->time + 5.0;

	if (m_TimeCanSteal && gpGlobals->time >= m_TimeCanSteal)
		m_TimeCanSteal = 0;
		
	pev->vuser3.x = MaxHP();
	pev->vuser3.y = (IsAlive() ? pev->health : 0);
	pev->vuser3.z = 0.0f;

	enddbg;
}

/* Time based Damage works as follows:
	1) There are several types of timebased damage:

		#define DMG_PARALYZE		(1 << 14)	// slows affected creature down
		#define DMG_NERVEGAS		(1 << 15)	// nerve toxins, very bad
		#define DMG_POISON			(1 << 16)	// blood poisioning
		#define DMG_RADIATION		(1 << 17)	// radiation exposure
		#define DMG_DROWNRECOVER	(1 << 18)	// drown recovery
		#define DMG_ACID			(1 << 19)	// toxic chemicals or acid burns
		#define DMG_SLOWBURN		(1 << 20)	// in an oven
		#define DMG_SLOWFREEZE		(1 << 21)	// in a subzero freezer

	2) A new hit inflicting tbd restarts the tbd counter - each monster has an 8bit counter,
		per damage type. The counter is decremented every second, so the maximum time
		an effect will last is 255/60 = 4.25 minutes.  Of course, staying within the radius
		of a damaging effect like fire, nervegas, radiation will continually reset the counter to max.

	3) Every second that a tbd counter is running, the player takes damage.  The damage
		is determined by the type of tdb.
			Paralyze		- 1/2 movement rate, 30 second duration.
			Nervegas		- 5 points per second, 16 second duration = 80 points max dose.
			Poison			- 2 points per second, 25 second duration = 50 points max dose.
			Radiation		- 1 point per second, 50 second duration = 50 points max dose.
			Drown			- 5 points per second, 2 second duration.
			Acid/Chemical	- 5 points per second, 10 second duration = 50 points max.
			Burn			- 10 points per second, 2 second duration.
			Freeze			- 3 points per second, 10 second duration = 30 points max.

	4) Certain actions or countermeasures counteract the damaging effects of tbds:

		Armor/Heater/Cooler - Chemical(acid),burn, freeze all do damage to armor power, then to body
							- recharged by suit recharger
		Air In Lungs		- drowning damage is done to air in lungs first, then to body
							- recharged by poking head out of water
							- 10 seconds if swiming fast
		Air In SCUBA		- drowning damage is done to air in tanks first, then to body
							- 2 minutes in tanks. Need new tank once empty.
		Radiation Syringe	- Each syringe full provides protection vs one radiation dosage
		Antitoxin Syringe	- Each syringe full provides protection vs one poisoning (nervegas or poison).
		Health kit			- Immediate stop to acid/chemical, fire or freeze damage.
		Radiation Shower	- Immediate stop to radiation damage, acid/chemical or fire damage.


*/

// If player is taking time based damage, continue doing damage to player -
// this simulates the effect of being poisoned, gassed, dosed with radiation etc -
// anything that continues to do damage even after the initial contact stops.
// Update all time based damage counters, and shut off any that are done.

// The m_bitsDamageType bit MUST be set if any damage is to be taken.
// This routine will detect the initial on value of the m_bitsDamageType
// and init the appropriate counter.  Only processes damage every second.

//#define PARALYZE_DURATION	30		// number of 2 second intervals to take damage
//#define PARALYZE_DAMAGE		0.0		// damage to take each 2 second interval

//#define NERVEGAS_DURATION	16
//#define NERVEGAS_DAMAGE		5.0

//#define POISON_DURATION		25
//#define POISON_DAMAGE		2.0

//#define RADIATION_DURATION	50
//#define RADIATION_DAMAGE	1.0

//#define ACID_DURATION		10
//#define ACID_DAMAGE			5.0

//#define SLOWBURN_DURATION	2
//#define SLOWBURN_DAMAGE		1.0

//#define SLOWFREEZE_DURATION	1.0
//#define SLOWFREEZE_DAMAGE	3.0

/* */
void CBasePlayer::CheckTimeBasedDamage()
{
	int i;
	byte bDuration = 0;

	static float gtbdPrev = 0.0;

	if (!(m_bitsDamageType & DMG_TIMEBASED))
		return;

	// only check for time based damage approx. every 2 seconds
	if (fabs(gpGlobals->time - m_tbdPrev) < 2.0)
		return;

	m_tbdPrev = gpGlobals->time;

	for (i = 0; i < CDMG_TIMEBASED; i++)
	{
		// make sure bit is set for damage type
		if (m_bitsDamageType & (DMG_PARALYZE << i))
		{
			switch (i)
			{
			case itbd_Paralyze:
				// UNDONE - flag movement as half-speed
				bDuration = PARALYZE_DURATION;
				break;
			case itbd_NerveGas:
				//				TakeDamage(pev, pev, NERVEGAS_DAMAGE, DMG_GENERIC);
				bDuration = NERVEGAS_DURATION;
				break;
			case itbd_Poison:
				TakeDamage(pev, pev, POISON_DAMAGE, DMG_GENERIC);
				bDuration = POISON_DURATION;
				break;
			case itbd_Radiation:
				//				TakeDamage(pev, pev, RADIATION_DAMAGE, DMG_GENERIC);
				bDuration = RADIATION_DURATION;
				break;
			case itbd_DrownRecover:
				// NOTE: this hack is actually used to RESTORE health
				// after the player has been drowning and finally takes a breath
				if (m_idrowndmg > m_idrownrestored)
				{
					int idif = V_min(m_idrowndmg - m_idrownrestored, 10);

					GiveHP(idif);
					m_idrownrestored += idif;
				}
				bDuration = 4; // get up to 5*10 = 50 points back
				break;
			case itbd_Acid:
				//				TakeDamage(pev, pev, ACID_DAMAGE, DMG_GENERIC);
				bDuration = ACID_DURATION;
				break;
			case itbd_SlowBurn:
				//				TakeDamage(pev, pev, SLOWBURN_DAMAGE, DMG_GENERIC);
				bDuration = SLOWBURN_DURATION;
				break;
			case itbd_SlowFreeze:
				//				TakeDamage(pev, pev, SLOWFREEZE_DAMAGE, DMG_GENERIC);
				bDuration = SLOWFREEZE_DURATION;
				break;
			default:
				bDuration = 0;
			}

			if (m_rgbTimeBasedDamage[i])
			{
				// use up an antitoxin on poison or nervegas after a few seconds of damage
				if (((i == itbd_NerveGas) && (m_rgbTimeBasedDamage[i] < NERVEGAS_DURATION)) ||
					((i == itbd_Poison) && (m_rgbTimeBasedDamage[i] < POISON_DURATION)))
				{
					if (m_rgItems[ITEM_ANTIDOTE])
					{
						m_rgbTimeBasedDamage[i] = 0;
						m_rgItems[ITEM_ANTIDOTE]--;
						//						SetSuitUpdate("!HEV_HEAL4", FALSE, SUIT_REPEAT_OK);
					}
				}

				// decrement damage duration, detect when done.
				if (!m_rgbTimeBasedDamage[i] || --m_rgbTimeBasedDamage[i] == 0)
				{
					m_rgbTimeBasedDamage[i] = 0;
					// if we're done, clear damage bits
					m_bitsDamageType &= ~(DMG_PARALYZE << i);
				}
			}
			else
				// first time taking this damage type - init damage duration
				m_rgbTimeBasedDamage[i] = bDuration;
		}
	}
}

/*
THE POWER SUIT

The Suit provides 3 main functions: Protection, Notification and Augmentation.
Some functions are automatic, some require power.
The player gets the suit shortly after getting off the train in C1A0 and it stays
with him for the entire game.

Protection

	Heat/Cold
		When the player enters a hot/cold area, the heating/cooling indicator on the suit
		will come on and the battery will drain while the player stays in the area.
		After the battery is dead, the player starts to take damage.
		This feature is built into the suit and is automatically engaged.
	Radiation Syringe
		This will cause the player to be immune from the effects of radiation for N seconds. Single use item.
	Anti-Toxin Syringe
		This will cure the player from being poisoned. Single use item.
	Health
		Small (1st aid kits, food, etc.)
		Large (boxes on walls)
	Armor
		The armor works using energy to create a protective field that deflects a
		percentage of damage projectile and explosive attacks. After the armor has been deployed,
		it will attempt to recharge itself to full capacity with the energy reserves from the battery.
		It takes the armor N seconds to fully charge.

Notification (via the HUD)

x	Health
x	Ammo
x	Automatic Health Care
		Notifies the player when automatic healing has been engaged.
x	Geiger counter
		Classic Geiger counter sound and status bar at top of HUD
		alerts player to dangerous levels of radiation. This is not visible when radiation levels are normal.
x	Poison
	Armor
		Displays the current level of armor.

Augmentation

	Reanimation (w/adrenaline)
		Causes the player to come back to life after he has been dead for 3 seconds.
		Will not work if player was gibbed. Single use.
	Long Jump
		Used by hitting the ??? key(s). Caused the player to further than normal.
	SCUBA
		Used automatically after picked up and after player enters the water.
		Works for N seconds. Single use.

Things powered by the battery

	Armor
		Uses N watts for every M units of damage.
	Heat/Cool
		Uses N watts for every second in hot/cold area.
	Long Jump
		Uses N watts for every jump.
	Alien Cloak
		Uses N watts for each use. Each use lasts M seconds.
	Alien Shield
		Augments armor. Reduces Armor drain by one half

*/

// if in range of radiation source, ping geiger counter
#define GEIGERDELAY 0.25

void CBasePlayer ::UpdateGeigerCounter(void)
{
	byte range;

	// delay per update ie: don't flood net with these msgs
	if (gpGlobals->time < m_flgeigerDelay)
		return;

	m_flgeigerDelay = gpGlobals->time + GEIGERDELAY;

	// send range to radition source to client

	range = (byte)(m_flgeigerRange / 4);

	if (range != m_igeigerRangePrev)
	{
		m_igeigerRangePrev = range;

		MESSAGE_BEGIN(MSG_ONE, gmsgGeigerRange, NULL, pev);
		WRITE_BYTE(range);
		MESSAGE_END();
	}

	// reset counter and semaphore
	if (!RANDOM_LONG(0, 3))
		m_flgeigerRange = 1000;
}

//=========================================================
// UpdatePlayerSound - updates the position of the player's
// reserved sound slot in the sound list.
//=========================================================
void CBasePlayer ::UpdatePlayerSound(void)
{
	int iBodyVolume;
	int iVolume;
	CSound *pSound;

	pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt ::ClientSoundIndex(edict()));
	if (!pSound)
		return;

	pSound->m_SrcEntity = this;
	pSound->m_Type = "npc";

	if (!pSound)
	{
		ALERT(at_console, "Client lost reserved sound!\n");
		return;
	}

	pSound->m_iType = bits_SOUND_NONE;

	// now calculate the best target volume for the sound. If the player's weapon
	// is louder than his body/movement, use the weapon volume, else, use the body volume.

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		// clamp the noise that can be made by the body, in case a push trigger,
		// weapon recoil, or anything shoves the player abnormally fast.
		//iBodyVolume = min( pev->velocity.Length(), 512 );
		if (pev->velocity.Length() < 512)
			iBodyVolume = pev->velocity.Length();
		else
			iBodyVolume = 512;
	}
	else
		iBodyVolume = 0;

	if (pev->button & IN_JUMP)
	{
		iBodyVolume += 100;
	}

	// convert player move speed and actions into sound audible by monsters.
	//if( Class && Class->id == CLASS_ROGUE ) m_iTargetVolume = 0;
	//else {
	if (m_iTargetVolume > iBodyVolume)
	{
		//I'm already making more noise than my footsteps...
		// OR in the bits for COMBAT sound if the weapon is being louder than the player.
		pSound->m_iType |= bits_SOUND_COMBAT;
	}
	else
		m_iTargetVolume = iBodyVolume;
	//}

	// if target volume is greater than the player sound's current volume, we paste the new volume in
	// immediately. If target is less than the current volume, current volume is not set immediately to the
	// lower volume, rather works itself towards target volume over time. This gives monsters a much better chance
	// to hear a sound, especially if they don't listen every frame.
	iVolume = pSound->m_iVolume;

	if (m_iTargetVolume > iVolume)
	{
		iVolume = m_iTargetVolume;
	}
	else if (iVolume > m_iTargetVolume)
	{
		iVolume -= 250 * gpGlobals->frametime;

		if (iVolume < m_iTargetVolume)
			iVolume = 0;
	}

	if (m_fNoPlayerSound)
	{
		// debugging flag, lets players move around and shoot without monsters hearing.
		iVolume = 0;
	}

	if (gpGlobals->time > m_flStopExtraSoundTime)
	{
		// since the extra sound that a weapon emits only lasts for one client frame, we keep that sound around for a server frame or two
		// after actual emission to make sure it gets heard.
		m_iExtraSoundTypes = 0;
	}

	if (pSound)
	{
		pSound->m_vecOrigin = pev->origin;
		pSound->m_iType |= (bits_SOUND_PLAYER | m_iExtraSoundTypes);
		pSound->m_iVolume = iVolume;
	}

	// keep track of virtual muzzle flash
	m_iWeaponFlash -= 256 * gpGlobals->frametime;
	if (m_iWeaponFlash < 0)
		m_iWeaponFlash = 0;

	UTIL_MakeVectors(pev->angles);
	gpGlobals->v_forward.z = 0;

	// Below are a couple of useful little bits that make it easier to determine just how much noise the
	// player is making.
	//UTIL_ParticleEffect ( pev->origin + gpGlobals->v_forward * iVolume, g_vecZero, 255, 25 );
	//ALERT ( at_console, "%d/%d\n", iVolume, m_iTargetVolume );
	m_iTargetVolume = 0;
}
#define postthinkdbg(a) dbg(msstring("[") + DisplayName() + "] " + a)

void CBasePlayer::PostThink()
{
	startdbg;
	postthinkdbg("Begin");

	if (g_fGameOver)
		goto pt_end; // intermission or finale

	if (!IsAlive() || FBitSet(pev->flags, FL_SPECTATOR))
		goto pt_end;

	postthinkdbg("Handle Tank controlling");
	// Handle Tank controlling
	if (m_pTank != NULL)
	{ // if they've moved too far from the gun,  or selected a weapon, unuse the gun
		if (m_pTank->OnControls(pev) && !pev->weaponmodel)
		{
			m_pTank->Use(this, this, USE_SET, 2); // try fire the gun
		}
		else
		{ // they've moved off the platform
			m_pTank->Use(this, this, USE_OFF, 0);
			m_pTank = NULL;
		}
	}

	postthinkdbg("Call ItemPostFrame( )");
	// do weapon stuff
	ItemPostFrame();

	// check to see if player landed hard enough to make a sound
	// falling farther than half of the maximum safe distance, but not as far a max safe distance will
	// play a bootscrape sound, and no damage will be inflicted. Fallling a distance shorter than half
	// of maximum safe distance will make no sound. Falling farther than max safe distance will play a
	// fallpain sound, and damage will be inflicted based on how far the player fell

	postthinkdbg("Check player fall");
	if ((FBitSet(pev->flags, FL_ONGROUND)) && (pev->health > 0) && m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD)
	{
		// ALERT ( at_console, "%f\n", m_flFallVelocity );

		if (pev->watertype == CONTENT_WATER)
		{
			// Did he hit the world or a non-moving entity?
			// BUG - this happens all the time in water, especially when
			// BUG - water has current force
			// if ( !pev->groundentity || VARS(pev->groundentity)->velocity.z == 0 )
			// EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade1.wav", 1, ATTN_NORM);
		}
		else if (m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED)
		{ // after this point, we start doing damage

			pev->punchangle.x += 22;
			pev->punchangle.y += RANDOM_LONG(-20, 20);
			pev->punchangle.z += RANDOM_LONG(-20, 20);
			float flFallDamage = g_pGameRules->FlPlayerFallDamage(this);

			//HIT GROUND SOUNDS
			if (flFallDamage > pev->health)
			{ //splat
				// note: play on item channel because we play footstep landing on body channel
				if (flFallDamage > pev->health + PLAYER_GIB_HEATH)
					EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/bodysplat.wav", 1, ATTN_NORM);
			}
			switch (RANDOM_LONG(0, 4))
			{
				case 0:
					PlaySound(CHAN_AUTO, "player/hitground1.wav", 1.0, true);
					break;
				case 1:
					PlaySound(CHAN_AUTO, "player/hitground2.wav", 1.0, true);
					break;
				case 2:
					PlaySound(CHAN_AUTO, "common/bodydrop1.wav", 1.0, true);
					break;
				case 3:
					PlaySound(CHAN_AUTO, "common/bodydrop2.wav", 1.0, true);
					break;
				case 4:
					PlaySound(CHAN_AUTO, "common/bodydrop3.wav", 1.0, true);
					break;
			}

			//PAIN sounds and TakeDamage()
			if (flFallDamage > 0)
			{
				if (flFallDamage < pev->health)
				{ // If you didn't die
					CallScriptEvent("game_fall");
				}

				//if( Class && Class->id == CLASS_ROGUE )
				//	flFallDamage *= 0.5;

				TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), flFallDamage, DMG_FALL);
			}
		}
		else
		{
			// knock the screen around a little bit, temporary effect
			pev->punchangle.x = m_flFallVelocity * 0.018; // punch x axis

			if (RANDOM_LONG(0, 1))
			{
				pev->punchangle.z = m_flFallVelocity * 0.009;
			}
		}
	}

	postthinkdbg("Call game_animate");
	CallScriptEvent("game_animate");

	postthinkdbg("Call SetAnimation");
	if (IsAlive())
		SetAnimation(MONSTER_ANIM_WALK);

	postthinkdbg("Call StudioFrameAdvance");
	StudioFrameAdvance();

	postthinkdbg("Call UpdatePlayerSound");
	UpdatePlayerSound();

	postthinkdbg("Call SetSpeed");
	SetSpeed();

	// Track button info so we can detect 'pressed' and 'released' buttons next frame
	m_afButtonLast = pev->button;

pt_end:

	postthinkdbg("Call UpdateMiscPositions");
	UpdateMiscPositions();

	postthinkdbg("Call Body->Think");
	if (Body)
		Body->Think(this);

	postthinkdbg("Call Script Event game_think");
	CallScriptEvent("game_think");

	postthinkdbg("End PostThink");
	
	//Deactivate no-collide if not near any players.
	postthinkdbg("Check for nearby spawns and players, deactivate nocolloide");
	//do not check for dead or spectating/noclipping players
	if (pev->solid == SOLID_TRIGGER && pev->deadflag == DEAD_NO && pev->movetype != MOVETYPE_NOCLIP) {

		CBaseEntity* fEnt = NULL;
		//initialize proximity check
		bool hasProximity = false;
		int proximityDistance = 32;

		//loop through all nearby entities
		while ( (fEnt = UTIL_FindEntityInSphere(fEnt, pev->origin, proximityDistance) ) != NULL) {

			//if you find a player set check to 1
			if (fEnt->IsPlayer() && fEnt->pev != pev) {
				hasProximity = true;
				//no reason to continue.
				break;
			}

		};

		//setup for spawn point check as they are never found by find entity in sphere
		fEnt = NULL;
		while ((fEnt = UTIL_FindEntityByClassname(fEnt, SPAWN_BEGIN)) != NULL) {
			Vector spawnDistance = fEnt->pev->origin - pev->origin;

			//hardcoding vector length since it's always x y z
			for (int idx = 0; idx < 3; idx++) {
				spawnDistance[idx] = std::abs(spawnDistance[idx]);
			};

			//if more than 124 away from spawn in any direction loop to next
			if (spawnDistance[0] > proximityDistance || spawnDistance[1] > proximityDistance || spawnDistance[2] > proximityDistance) {
				continue;
			};

			//else set proximity check
			hasProximity = true;

		};

		//and same for generic spawns
		fEnt = NULL;
		while ((fEnt = UTIL_FindEntityByClassname(fEnt, SPAWN_GENERIC)) != NULL) {
			Vector spawnDistance = fEnt->pev->origin - pev->origin;

			for (int idx = 0; idx < 3; idx++) {
				spawnDistance[idx] = std::abs(spawnDistance[idx]);
			};

			//if more than 124 away from spawn in any direction loop to next
			if (spawnDistance[0] > proximityDistance || spawnDistance[1] > proximityDistance || spawnDistance[2] > proximityDistance) {
				continue;
			};

			//else set proximity check
			hasProximity = true;

		};

		//if check is not true set self solid state to slidebox
		if (!hasProximity) {
			pev->solid = SOLID_SLIDEBOX;
		}
	}

	enddbg("CBasePlayer::PostThink()");
}

// checks if the spot is clear of players
BOOL IsSpawnPointValid(CBaseEntity *pPlayer, CBaseEntity *pSpot)
{
	// CBaseEntity *ent = NULL;

	// if (!pSpot->IsTriggered(pPlayer))
	// {
	// 	return FALSE;
	// }

	// /*	while ( (ent = UTIL_FindEntityInSphere( ent, pSpot->pev->origin, 128 )) != NULL )
	// {
	// 	// if ent is a client, don't spawn on 'em
	// 	if ( ent->IsPlayer() && ent != pPlayer )
	// 		return FALSE;
	// }*/
	// bool fBlocked = false;
	// float Range = 72;
	// Vector vMinBounds = pSpot->pev->origin - Vector(Range, Range, Range);
	// Vector vMaxBounds = pSpot->pev->origin + Vector(Range, Range, Range);
	// int count = UTIL_EntitiesInBox(g_pEntitiesInBox, MAX_ENTITIES_TO_SEARCH, vMinBounds, vMaxBounds, 0);
	// for (int i = 0; i < count; i++)
	// {
	// 	CBaseEntity *pSightEnt = g_pEntitiesInBox[i];
	// 	if (!pSightEnt || pSightEnt->pev->solid == SOLID_NOT ||
	// 		pSightEnt->pev->solid == SOLID_TRIGGER || pSightEnt == pPlayer)
	// 		continue;

	// 	//We're blocked, quit
	// 	fBlocked = true;
	// 	break;
	// }

	return TRUE;
}
void CBasePlayer::AddNoise(float flNoiseAmt)
{
	m_iTargetVolume = V_min(m_iTargetVolume + flNoiseAmt, 1500);
	//if (m_iTargetVolume + flNoiseAmt<1500)
	//	m_iTargetVolume = m_iTargetVolume + flNoiseAmt;
	//else
	//	m_iTargetVolume = 1500;
}

/*
============
Am I elite?
============
*/
bool CBasePlayer::IsElite()
{
	return m_fIsElite;
	/*	if( FStrEq(STRING(pPlayer->DisplayName),"Dogg") ||
		FStrEq(STRING(pPlayer->DisplayName),"Lanethan") ||
		FStrEq(STRING(pPlayer->DisplayName),"Borbarad") ||
		FStrEq(STRING(pPlayer->DisplayName),"Lord Maz") ||
		FStrEq(STRING(pPlayer->DisplayName),"Heath") ||
		FStrEq(STRING(pPlayer->DisplayName),"Damroth") ||
		FStrEq(STRING(pPlayer->DisplayName),"Nestromo") )
			return true;*/

	//return false;
}

CBaseEntity *GetAssaultSpawnSpot(CBasePlayer *pPlayer);

enum spawnspot_e
{
	SS_NOSPOT,
	SS_FOUND,
	SS_ALLFULL
};
spawnspot_e GetRandomSpawnSpot(msstring_ref Name, msstring_ref TransitionName, CBaseEntity *pPlayer, CBaseEntity **pFoundSpot)
{
	mslist<CBaseEntity *> Spawnpoints;
	if (!Name)
		return SS_NOSPOT;

	//logfile << " " << pPlayer->DisplayName() << "Requested_Spwn_spot: " << TransitionName << "\r\n";

	CBaseEntity *pSpot = NULL;
	while (pSpot = UTIL_FindEntityByClassname(pSpot, Name))
	{
		bool fValidSpot = true;

		//if TransitionName is set, check the transition name.  If trans is not set, I don't care what the tranistion is
		if (TransitionName && !FStrEq(STRING(pSpot->pev->message), TransitionName))
		{
			fValidSpot = false;
			continue;
		}

		Spawnpoints.add(pSpot);
	}

	if (!Spawnpoints.size())
		return SS_NOSPOT;

	//Test all spots in a random order
	int RandSpot = 0;
	pSpot = NULL;

	while (Spawnpoints.size())
	{
		RandSpot = RANDOM_LONG(0, (Spawnpoints.size() - 1));

		pSpot = Spawnpoints[RandSpot];
		if (!IsSpawnPointValid(pPlayer, pSpot))
		{
			pSpot = NULL;
			Spawnpoints.erase(RandSpot);
		}
		else
			break;
	}

	//All spots filled, wait.
	if (!pSpot)
		return SS_ALLFULL; //Spots found, but all spots full

	*pFoundSpot = pSpot;
	return SS_FOUND; //Spot found
}
//Thothie NOV2014_09 - IsActive() - centralizing AFK checking
bool CBasePlayer::IsActive()
{
	IScripted *iScripted = this->GetScripted();
	if (!iScripted)
		return false;
	if (atoi(iScripted->GetFirstScriptVar("PLR_IN_WORLD")) != 1)
		return false;
	msstring CheckSteamID = this->AuthID();
	if (CheckSteamID.contains("UNKNOWN"))
		return false;
	if (CheckSteamID.contains("BOT"))
		return false;
	if (CheckSteamID.contains("HLTV"))
		return false;
	if (this->MaxHP() < 15)
		return false;
	return true;
}

CBaseEntity *CBasePlayer::FindSpawnSpot()
{
	//Compile a list of spots that you can spawn at, then randomly select one

	const char *JoinTypeText = "unknown";
	if (m_CharacterState == CHARSTATE_UNLOADED)
		JoinTypeText = "Spectate";
	else
	{
		switch (m_JoinType)
		{
		case JN_TRAVEL:
			JoinTypeText = "Travel";
			break;
		case JN_STARTMAP:
			JoinTypeText = "Start Map";
			break;
		case JN_VISITED:
			JoinTypeText = "Previously Visited";
			break;
		case JN_ELITE:
			JoinTypeText = "GM";
			break;
		case JN_NOTALLOWED:
			JoinTypeText = "Not Allowed";
			break;
		}
	}

	logfile << Logger::LOG_INFO << "Looking for valid spawn spots for " << DisplayName() << " (" << JoinTypeText << ")...\n";

	//Find all valid spots
	CBaseEntity *pSpot = NULL;
	spawnspot_e Status = SS_NOSPOT;

	if (m_CharacterState == CHARSTATE_UNLOADED)
	{
		Status = GetRandomSpawnSpot(SPAWN_SPECTATE, NULL, this, &pSpot);
		if (Status == SS_NOSPOT)
		{
			if (IsElite())
				ALERT(at_console, "WARNING: Mapper did not include spawn spot: %s\n", SPAWN_SPECTATE);
			Status = GetRandomSpawnSpot(SPAWN_GENERIC, NULL, this, &pSpot);
		}
	}
	else
	{
		logfile << Logger::LOG_INFO << "m_JoinType: " << m_JoinType << "\n";
		switch (m_JoinType)
		{
		case JN_VISITED:
			m_SpawnTransition = GetOtherPlayerTransition(this);
			//----NO BREAK---

		case JN_TRAVEL:	  //Transitioned to new map or died, and respawning at last transition
		case JN_STARTMAP: //Joined a startmap.
						  //If I joined a startmap, but I have a transition, try to use the transition first
						  //before just going straight to the char creation point
		case JN_ELITE:	  //I'm Elite - Join no matter what
			if (m_SpawnTransition)
				Status = GetRandomSpawnSpot(SPAWN_GENERIC, m_SpawnTransition, this, &pSpot);
			else
				Status = SS_NOSPOT;

			if (Status == SS_NOSPOT && m_JoinType == JN_STARTMAP)
				Status = GetRandomSpawnSpot(SPAWN_BEGIN, NULL, this, &pSpot);
			if (Status == SS_NOSPOT && m_JoinType == JN_ELITE)
				Status = GetRandomSpawnSpot(SPAWN_GENERIC, NULL, this, &pSpot);
			break;
		default:

			break;
		}
	}

	if (Status > SS_NOSPOT)
	{
		logfile << Logger::LOG_INFO << "Found useable spawn spots, testing...\n";

		if (Status == SS_ALLFULL)
		{
			//All spots filled, wait.
			SpawnCheckTime = gpGlobals->time + 2.0;
			SendEventMsg(HUDEVENT_UNABLE, "Waiting to spawn...\n");
			logfile << Logger::LOG_INFO << "All spawn spots filled!\n";
			return NULL;
		}

		//Spot was open, fall through
	}
	else
	{
		//No valid spots, kick player with message

		msstring TransitionText = m_SpawnTransition ? m_SpawnTransition : "<unknown>";
		logfile << Logger::LOG_WARN << "NO valid spawn spots for " << JoinTypeText << " (Trans: " << TransitionText.c_str() << ")!!!\n";
		bool fKickPlayer = true;

		if (m_CharacterState == CHARSTATE_LOADED)
		{
			if (m_MapStatus == FIRST_MAP)
				ClientPrint(pev, at_console, "* You cannot create a new character on this map! *\n");
			else if (m_MapStatus == NEW_MAP)
				ClientPrint(pev, at_console, "* Mapper did not include transition link: %s *\n", TransitionText.c_str());
		}

		if (IsElite())
		{
			pSpot = UTIL_FindEntityByClassname(NULL, SPAWN_GENERIC);
			if (!pSpot)
				ALERT(at_console, "Map has no %s.\n", SPAWN_GENERIC);
			fKickPlayer = false;
		}

		if (fKickPlayer)
		{
			msstring PlayerName = DisplayName();
			ALERT(at_logged, "Kicking %s:  %s's char is on map: %s Trans: %s\n", PlayerName.c_str(), PlayerName.c_str(), m_cEnterMap, TransitionText.c_str());
			char cKickString[256];
			_snprintf(cKickString, sizeof(cKickString),
				"Disconnected.\n*** Your character is not on this map. ***\n*** Your character is on the map: %s ***\n", m_cEnterMap);
			KickPlayer(cKickString);
			return NULL;
		}
	}

	logfile << Logger::LOG_INFO << "Found VALID SPOT\n";

	return pSpot;
}

//Map must have a ms_player_begin in order for people to create characters there!
class CSpawnPointBegin : public CPointEntity
{
	void Spawn()
	{
		MSGlobals::CanCreateCharOnMap = true;
		CPointEntity::Spawn();
	}
};

LINK_ENTITY_TO_CLASS(ms_player_spec, CPointEntity);
LINK_ENTITY_TO_CLASS(ms_player_spawn, CPointEntity);
LINK_ENTITY_TO_CLASS(ms_player_begin, CSpawnPointBegin);

void CBasePlayer::Spawn(void)
{
	startdbg;

	dbg("Call Precache");
	//Master Sword spawn code
	//Note: The player will sometimes have items/packs when this is called
	Precache();

	bool SpawnPlayer = m_CharacterState == CHARSTATE_LOADED;
	bool fRespawnPlayer = (pev->deadflag > DEAD_NO); //Respawn me

	//MiB Jul2008a (JAN2010) - If this is the first spawn, call 'wear' on all items worn.
	//This fixes the bug where you would have to re-equip packs (etc)
	//For them to show to other players.
	//-
	//Thothie - no good, reactivates item, causing conflicts
	//Thothie - yeah, but we really, really need it now, attempting to compensate
	if (!fRespawnPlayer)
	{
		for (int i = 0; i < Gear.size(); i++)
		{
			CGenericItem *cur_item = Gear[i];

			if (cur_item->IsWorn())
			{
				//Thothie FEB2010_01 Pass gender/race with show
				static msstringlist Params;
				Params.clearitems();
				Params.add(m_Race);
				Params.add((m_Gender == 0) ? "male" : "female");
				Params.add("game_show");
				cur_item->CallScriptEvent("game_show", &Params);
			}
		}
	}

	//Initialize if not done already
	dbg("Call InitialSpawn");
	InitialSpawn();

	pev->model = IdealModel();
	SetModel(pev->model); //Set the default model

	dbg("Call game_spawn");
	CallScriptEvent("game_spawn");

	dbg("Init player");
	m_PrefHand = RIGHT_HAND; // Right handed (unsettable for now)
	m_Framerate = 1.0f;
	CheckAreaTime = gpGlobals->time + 0.5;

	int oldStatusFlags = m_StatusFlags;
	m_StatusFlags = 0;

	m_flFieldOfView = 0.1;
	ClearConditions(MONSTER_OPENCONTAINER);

	if (fRespawnPlayer)
	{
		m_MaxHP = MaxHP();
		m_MaxMP = MaxMP();
		pev->health = m_HP = pev->max_health = m_MaxHP;
		m_MP = m_MaxMP;
		CallScriptEvent("game_respawn"); //Thothie MAR2008a
	}

	Stamina = MaxStamina();
	//-----------------------------

	pev->classname = MAKE_STRING("player");
	pev->armorvalue = 0;
	pev->takedamage = DAMAGE_AIM;
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_WALK;
	pev->flags = FL_CLIENT;
	pev->air_finished = gpGlobals->time + 12;
	pev->dmg = 2; // initial water damage
	pev->effects = 0;

	pev->deadflag = DEAD_NO;
	pev->dmg_take = 0;
	pev->dmg_save = 0;
	pev->friction = 1.0;
	pev->gravity = 1.0;
	pev->rendermode = 0;
	pev->renderamt = 0;
	m_bitsHUDDamage = -1;
	m_bitsDamageType = 0;
	m_afPhysicsFlags = 0;

	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "slj", "0");
	g_engfuncs.pfnSetPhysicsKeyValue(edict(), "hl", "1");

	m_iFOV = 0;				  // init field of view.
	m_iClientFOV = -1;		  // make sure fov reset is sent
	m_iClientHP = -1;		  // reset HP
	m_iClientMaxHP = -1;	  // reset MaxHP
	m_iClientMP = -1;		  // reset MP
	m_iClientMaxMP = -1;	  // reset MaxMP
	m_ClientCurrentHand = -1; // reset Hand
	m_ClientGender = -1;	  // reset Gender

	m_flNextDecalTime = 0; // let this player decal as soon as he spawns.

	m_flgeigerDelay = gpGlobals->time + 2.0; // wait a few seconds until user-defined message registrations
											 // are recieved by all clients

	m_flTimeStepSound = 0;
	m_iStepLeft = 0;
	m_flFieldOfView = 0.5; // some monsters use this to determine whether or not the player is looking at them.

	m_bloodColor = BLOOD_COLOR_RED;
	m_flNextAttack = UTIL_WeaponTimeBase();

	// dont let uninitialized value here hurt the player
	m_flFallVelocity = 0;

	//Find a spot for the player and create a portal

	//If m_MapStatus == OLD_MAP, then the server has recieved
	//this player's coordinates, or the player has already spawned once.

	if (!MoveToSpawnSpot())
	{
		pev->origin = Vector(0, 0, 0);
		m_iHideHUD = HIDEHUD_ALL;
	}

	UTIL_SetOrigin(pev, pev->origin);

	/*if( !pentSpawnSpot )
	{
		//logfile << "\r\nSpawn spot was invalid" << (m_MapStatus == OLD_MAP ? " (Same map, so use coordinates)" : "") << "\r\n";
		//If OLD_MAP, and first spawn, then a spawn spot wasn't selected
		//because the coords were sent from client.
		//If not the first spawn, then a spawn spot was selected and
		//the above !pentSpawnSpot skips over this
		if( m_MapStatus != OLD_MAP ) SpawnPlayer = FALSE;
	}
	else {
	}*/

	dbg("Create spawn portal");
	if (m_MapStatus == FIRST_MAP && !fRespawnPlayer)
	{
		//***!!!*** Setup some kind of cool portal***!!!***
		CPortal *pPortal = GetClassPtr((CPortal *)NULL);
		pPortal->pev->origin = pev->origin;
		pPortal->Spawn2();
	}

	dbg("Call Setsize");
	SetSize(pev->flags);

	pev->view_ofs = VEC_VIEW;
	m_HackedGunPos = Vector(0, 32, 0);

	if (m_iPlayerSound == SOUNDLIST_EMPTY)
		ALERT(at_console, "Couldn't alloc player sound slot!\n");

	m_fNoPlayerSound = FALSE; // normal sound behavior.

	m_pLastItem = NULL;
	m_fInitHUD = TRUE;
	m_iClientHideHUD = -1; // force this to be recalculated
	m_fWeapon = FALSE;
	m_pClientActiveItem = NULL;

	if (!SpawnPlayer)
	{
		//If it hasn't loaded your character yet, don't spawn
		dbg("Spawn in observer mode");
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_NOCLIP;

		//Start out in the boonies...
		//If clients sudden start missing sever-to-client messages, THIS is the problem!
		//UTIL_SetOrigin( pev, Vector( 4000, 4000, 4000 ) );

		pev->takedamage = DAMAGE_NO;
		SetBits(pev->effects, EF_NODRAW);
		SetBits(pev->flags, FL_NOTARGET);
		m_afPhysicsFlags |= PFLAG_OBSERVER;
		m_iHideHUD = HIDEHUD_ALL;
		EnableControl(FALSE); //So you can't move

		dbg("Call PreLoadChars");

		//Load the list of characters either from file or from the Central Server
		if (!m_LoadedInitialChars)
		{
			PreLoadChars();
			m_LoadedInitialChars = true;
		}
	}
	else
	{
		dbg("Spawn in regular mode");
		CallScriptEvent("game_player_putinworld"); //Thothie MAR2008a

		//See if music is playing for all players, then play for newly connected character
		if (MSGlobals::AllMusic.length() > 0) //If playing music for all players
		{
			SwapMusic(-1, MSGlobals::AllMusicMode, MSGlobals::AllMusic);
		}

		//debug
		Print(">>>>> spawn: m_NextTransition: %s m_SpawnTransition: %s m_OldTransition: %s\n", m_SpawnTransition, m_SpawnTransition, m_OldTransition);

		ClearBits(pev->flags, FL_NOTARGET);
		//SetBits( pev->effects, EF_NODRAW ); //Thothie - making char invisible for new body
		UTIL_ScreenFade(this, Vector(0, 0, 0), 1, 0, 0, FFADE_IN);
		CinematicCamera(FALSE); //Returns you to normal viewing
		LockSpeed = FALSE;
		m_SkillLevel = -1; //Force a score screen update
		//Call the client dll spawn function in UpdateClientData
		SetBits(m_MsgFlags, MSGFLAG_SPAWN);
		m_iHideHUD = 0; //Unhide HUD, if hidden
		EnableControl(TRUE);
		UTIL_SetOrigin(pev, pev->origin);

		//Shuriken - This should send an Exp message for all the players' stats.
		//I have no idea how costly it is to send this many message each time a player joins
		//so you might be better off just axing this and letting them update on monster kills.
		/*for(int i = 0; i < SKILL_MAX_ATTACK; i++) {
			CStat *pStat = FindStat( i + SKILL_FIRSTSKILL );
			for(int p = 0; p < (signed)pStat->m_SubStats.size();p++) {
				CSubStat &SubStat = pStat->m_SubStats[p];
				if(SubStat.Value > 26) {
					MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_EXP], NULL, pev );
					WRITE_BYTE( i );
					WRITE_BYTE( p );
					WRITE_LONG( SubStat.Exp );
					MESSAGE_END();
				}
			}
		}*/

		//If the map status was first map, leave it
		//Otherwise set it to old map, so I can respawn
		if (m_MapStatus != FIRST_MAP)
			m_MapStatus = OLD_MAP;

		//Display greeting
		dbg("Display server greeting");
		clientaddr_t &ClientInfo = g_NewClients[entindex() - 1];
		if (!ClientInfo.fDisplayedGreeting)
		{
			msstring InfoString;
			msstring_ref PKString = NULL, SaveString = NULL, CharString = NULL;
			if (MSGlobals::PKAllowedinTown)
				PKString = "Player killing is allowed outside of town";
			else if (MSGlobals::PKAllowed)
				PKString = "Player killing is allowed anywhere";
			else
				PKString = "This server does not allow player killing";

			if (MSGlobals::GameType != GAMETYPE_ADVENTURE)
				SaveString = "Challenge mode: Your character will not be saved!";

			if (MSGlobals::ServerSideChar != GAMETYPE_ADVENTURE)
				CharString = "Characters are stored on the server";

			InfoString = PKString;
			if (SaveString)
			{
				InfoString += "\n";
				InfoString += SaveString;
			}
			if (CharString)
			{
				InfoString += "\n";
				InfoString += CharString;
			}

			SendHUDMsg("Welcome to Master Sword", InfoString);
			ClientInfo.fDisplayedGreeting = true;
		}
	}
	m_pLastAnimHandler = NULL;				 // Set these
	m_pAnimHandler = &gAnimWalk;			 // or it crashes
	m_fSequenceFinished = TRUE;				 //So it knows to begin the first animation
	pev->sequence = LookupSequence("stand"); //LookupActivity( ACT_IDLE )
	BlockButton(IN_ATTACK);					 //Make it inconsequential if the player is still holding down the button

	dbg("Initialize Body");
	if (Body)
		Body->Set(BPS_RDRNORM, 0);

	//Give player hands
	dbg("Give player hands SpawnPlayer PlayerHands");
	if (SpawnPlayer && !PlayerHands)
	{
		dbg("Give player hands CGenericItem *pPlayerHands");
		CGenericItem *pPlayerHands = NewGenericItem("fist_bare");
		if (pPlayerHands)
		{
			dbg("Give player hands pPlayerHands GiveTo");
			pPlayerHands->GiveTo(this, false, false);
		}
		else
		{
			dbg("Give player hands MSErrorConsoleText");
			MSErrorConsoleText("CBasePlayer::Spawn()", "Couldn't find item \"fist_bare\"!");
		}
	}

	CustomTitle = " NONESET ";

	//MiB - Putting this before Shuri's message just in case the exp crap prematurely destroys the function
	//Set the player's net name to be the DisplayName. I stole this code from when a player types 'name' in console,
	//As it fixes the glitch of having "evil" instead of "evil squirrel". I figure
	//The same code here will do the same, but without the client getting involved
	g_engfuncs.pfnSetClientKeyValue(entindex(), g_engfuncs.pfnGetInfoKeyBuffer(edict()), "name", (char *)m_DisplayName);
	m_NetName = DisplayName();
	pev->netname = MAKE_STRING(m_NetName.c_str());

	dbg("Shurik3n: Send Skills on spawn");
	//Shurik3n AUG2007a - attempts to fix 100% bug
	for (int i = 0; i < SKILL_MAX_STATS; i++)
	{
		CStat *pStat = FindStat(i + SKILL_FIRSTSKILL);
		for (int p = 0; p < (signed)pStat->m_SubStats.size(); p++)
		{
			CSubStat &SubStat = pStat->m_SubStats[p];
			if (SubStat.Value > 25)
			{
				MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_EXP], NULL, pev);
				WRITE_BYTE(i);
				WRITE_BYTE(p);
				WRITE_LONG(SubStat.Exp);
				MESSAGE_END();
			}
		}
	}

	dbg("Call SwitchToBestHand");
	SwitchToBestHand();

	dbg("Call g_pGameRules->PlayerSpawn");
	g_pGameRules->PlayerSpawn(this);

	dbg("Call MSGlobals::GameScript game_playerspawn");
	if (MSGlobals::GameScript)
	{
		msstringlist Parameters;
		Parameters.add(EntToString(this));
		MSGlobals::GameScript->CallScriptEvent("game_playerspawn", &Parameters);
	}

	enddbg("CBasePlayer::Spawn()");
}

bool CBasePlayer::MoveToSpawnSpot()
{
	CBaseEntity *pSpawnSpot = NULL;
	startdbg;

	dbg("Call FindSpawnSpot");

	if (pSpawnSpot = FindSpawnSpot())
	{
		pev->origin = pSpawnSpot->pev->origin + Vector(0, 0, 1);
		pev->v_angle = g_vecZero;
		pev->velocity = g_vecZero;
		pev->angles = pSpawnSpot->pev->angles;
		pev->punchangle = g_vecZero;
		pev->fixangle = TRUE;
		if (pSpawnSpot->pev->target)
			FireTargets(STRING(pSpawnSpot->pev->target), this, this, USE_TOGGLE, 0);
	}

	//Thothie - need a loop around here, causing every wearable item the character has to execute it's "game_wear" function
	enddbg;
	return pSpawnSpot ? true : false;
}

extern int iBeam;
void PlayerPrecache()
{
	//remove?
	//iBeam = PRECACHE_MODEL( "sprites/smoke.spr" );

	// player gib sounds
	PRECACHE_SOUND("common/bodysplat.wav");

	// player pain sounds
	PRECACHE_SOUND("player/pl_pain2.wav");
	PRECACHE_SOUND("player/pl_pain4.wav");
	PRECACHE_SOUND("player/pl_pain5.wav");
	PRECACHE_SOUND("player/pl_pain6.wav");
	PRECACHE_SOUND("player/pl_pain7.wav");

	//Master Sword Precaches...
	PRECACHE_SOUND("player/death.wav");
	PRECACHE_SOUND("player/chesthit1.wav");
	PRECACHE_SOUND("player/stomachhit1.wav");
	PRECACHE_SOUND("player/armhit1.wav");
	PRECACHE_SOUND("player/leghit1.wav");
	PRECACHE_SOUND("player/fallpain1.wav");
	PRECACHE_SOUND("player/fallpain2.wav");
	PRECACHE_SOUND("player/fallpain3.wav");
	PRECACHE_SOUND("player/fallpain4.wav");
	PRECACHE_SOUND("player/hitground1.wav");
	PRECACHE_SOUND("player/hitground2.wav");
	PRECACHE_SOUND("player/shout1.wav");
	PRECACHE_SOUND("player/jab1.wav");
	PRECACHE_SOUND("player/jab2.wav");

	//Breathing, for client-side playback
	/*PRECACHE_SOUND("player/breathe_fast1.wav");
	PRECACHE_SOUND("player/breathe_fast2.wav");
	PRECACHE_SOUND("player/breathe_fast3.wav");
	PRECACHE_SOUND("player/breathe_slow1a.wav");
	PRECACHE_SOUND("player/breathe_slow1b.wav");*/

	PRECACHE_SOUND("player/femaledeath.wav");
	PRECACHE_SOUND("player/femalechesthit1.wav");
	PRECACHE_SOUND("player/femalestomachhit1.wav");
	PRECACHE_SOUND("player/femalearmhit1.wav");
	PRECACHE_SOUND("player/femaleleghit1.wav");
	PRECACHE_SOUND("player/femalefallpain1.wav");
	PRECACHE_SOUND("player/femalefallpain2.wav");
	PRECACHE_SOUND("player/femalefallpain3.wav");
	PRECACHE_SOUND("player/femalefallpain4.wav");
	PRECACHE_SOUND("player/femalehitground1.wav");
	PRECACHE_SOUND("player/femalehitground2.wav");
	PRECACHE_SOUND("player/femaleshout1.wav");
	PRECACHE_SOUND("player/femalejab1.wav");
	PRECACHE_SOUND("player/femalejab2.wav");

	//Breathing, for client-side playback
	/*	PRECACHE_SOUND("player/femalebreathe_fast1.wav");
	PRECACHE_SOUND("player/femalebreathe_fast2.wav");
	PRECACHE_SOUND("player/femalebreathe_fast3.wav");
	PRECACHE_SOUND("player/femalebreathe_slow1a.wav");
	PRECACHE_SOUND("player/femalebreathe_slow1b.wav");*/

	//PRECACHE_MODEL( "models/player.mdl"  );
	PRECACHE_MODEL(MODEL_HUMAN_REF);
	//PRECACHE_MODEL( MODEL_HUMAN_MALE1 ); //Thothie MAR2012_27 - why are we precache'ing this?

	//foreach( n, HumanModels )
	//{
	//	PRECACHE_MODEL( ModelList[n][0] );
	//PRECACHE_MODEL( ModelList[n][1] );
	//}

	//Precache the player script

	CScript PlayerScript;
	PlayerScript.Spawn(PLAYER_SCRIPT, NULL, NULL, true);
}

void CBasePlayer ::Precache(void)
{
	// in the event that the player JUST spawned, and the level node graph
	// was loaded, fix all of the node graph pointers before the game starts.

	// !!!BUGBUG - now that we have multiplayer, this needs to be moved!
	if (WorldGraph.m_fGraphPresent && !WorldGraph.m_fGraphPointersSet)
	{
		if (!WorldGraph.FSetGraphPointers())
		{
			ALERT(at_console, "**Graph pointers were not set!\n");
		}
		else
		{
			ALERT(at_console, "**Graph Pointers Set!\n");
		}
	}

	// SOUNDS / MODELS ARE PRECACHED in ClientPrecache() (game specific)
	// because they need to precache before any clients have connected

	// init geiger counter vars during spawn and each time
	// we cross a level transition

	m_flgeigerRange = 1000;
	m_igeigerRangePrev = 1000;

	m_bitsDamageType = 0;
	m_bitsHUDDamage = -1;

	m_iTrain = TRAIN_NEW;

	// Make sure any necessary user messages have been registered
	LinkUserMessages();

	m_iUpdateTime = 5; // won't update for 1/2 a second

	if (gInitHUD)
		m_fInitHUD = TRUE;
}

int CBasePlayer::Save(CSave &save)
{
	//if ( !CBaseMonster::Save(save) )
	return 0;

	//return save.WriteFields( "PLAYER", this, m_playerSaveData, ARRAYSIZE(m_playerSaveData) );
}

//
// Marks everything as new so the player will resend this to the hud.
//
void CBasePlayer::RenewItems(void)
{
}

int CBasePlayer::Restore(CRestore &restore)
{
	return 0;
	/*if ( !CBaseMonster::Restore(restore) )
		return 0;

	int status = restore.ReadFields( "PLAYER", this, m_playerSaveData, ARRAYSIZE(m_playerSaveData) );

	SAVERESTOREDATA *pSaveData = (SAVERESTOREDATA *)gpGlobals->pSaveData;
	// landmark isn't present.
	if ( !pSaveData->fUseLandmark )
	{
		ALERT( at_console, "No Landmark:%s\n", pSaveData->szLandmarkName );

		// default to normal spawn
		edict_t* pentSpawnSpot = EntSelectSpawnPoint( this );
		pev->origin = VARS(pentSpawnSpot)->origin + Vector(0,0,1);
		pev->angles = VARS(pentSpawnSpot)->angles;
	}
	pev->v_angle.z = 0;	// Clear out roll
	pev->angles = pev->v_angle;

	pev->fixangle = TRUE;           // turn this way immediately

// Copied from spawn() for now
	m_bloodColor	= BLOOD_COLOR_RED;

    g_ulModelRefHuman = pev->;

	if ( FBitSet(pev->flags, FL_DUCKING) )
	{
		// Use the crouch HACK
		// FixPlayerCrouchStuck( edict() );
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	}
	else
	{
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
	}

	RenewItems();

	return status;*/
}

const char *CBasePlayer::TeamID(void)
{
	return GetPartyName();
}
const char *CBasePlayer::GetPartyName(void)
{
	if (!pev ||	  // Not fully connected yet
		!m_pTeam) // No team
		return "";

	// return their team name
	return m_pTeam->TeamName();
}

ulong CBasePlayer::GetPartyID(void)
{
	if (!pev ||	  // Not fully connected yet
		!m_pTeam) // No team
		return 0;

	return m_pTeam->m_ID;
}

//==============================================
// !!!UNDONE:ultra temporary SprayCan entity to apply
// decal frame at a time. For PreAlpha CD
//==============================================
class CSprayCan : public CBaseEntity
{
public:
	void Spawn(entvars_t *pevOwner);
	void Think(void);

	virtual int ObjectCaps(void) { return FCAP_DONT_SAVE; }
};

void CSprayCan::Spawn(entvars_t *pevOwner)
{
	pev->origin = pevOwner->origin + Vector(0, 0, 32);
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT(pevOwner);
	pev->frame = 0;

	pev->nextthink = gpGlobals->time + 0.1;
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/sprayer.wav", 1, ATTN_NORM);
}

void CSprayCan::Think(void)
{
	TraceResult tr;
	int playernum;
	int nFrames;
	CBasePlayer *pPlayer;

	pPlayer = (CBasePlayer *)GET_PRIVATE(pev->owner);

	if (pPlayer)
		nFrames = pPlayer->GetCustomDecalFrames();
	else
		nFrames = -1;

	playernum = ENTINDEX(pev->owner);

	// ALERT(at_console, "Spray by player %i, %i of %i\n", playernum, (int)(pev->frame + 1), nFrames);

	UTIL_MakeVectors(pev->angles);
	UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, &tr);

	// No customization present.
	if (nFrames == -1)
	{
		UTIL_DecalTrace(&tr, DECAL_LAMBDA6);
		UTIL_Remove(this);
	}
	else
	{
		UTIL_PlayerDecalTrace(&tr, playernum, pev->frame, TRUE);
		// Just painted last custom frame.
		if (pev->frame++ >= (nFrames - 1))
			UTIL_Remove(this);
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

class CBloodSplat : public CBaseEntity
{
public:
	void Spawn(entvars_t *pevOwner);
	void Spray(void);
};

void CBloodSplat::Spawn(entvars_t *pevOwner)
{
	pev->origin = pevOwner->origin + Vector(0, 0, 32);
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT(pevOwner);

	SetThink(&CBloodSplat::Spray);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBloodSplat::Spray(void)
{
	TraceResult tr;

	if (g_Language != LANGUAGE_GERMAN)
	{
		UTIL_MakeVectors(pev->angles);
		UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, &tr);

		UTIL_BloodDecalTrace(&tr, BLOOD_COLOR_RED);
	}
	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.1;
}

//==============================================

CBaseEntity *CBasePlayer::GiveNamedItem(const char *pszName)
{
	//First, search through all items with pszName as a substring
	//If not found, use pszName as the full item name

	if (!pszName || !pszName[0])
		return NULL;

	msstring_ref WeaponScript = NULL;
	for (int i = 0; i < CGenericItemMgr::ItemCount(); i++)
	{
		GenItem_t &GlobalItem = *CGenericItemMgr::Item(i);
		if (strstr(GlobalItem.pItem->ItemName, pszName))
		{
			WeaponScript = GlobalItem.pItem->ItemName;
			break;
		}
	}

	if (!WeaponScript)
		return NULL;

	CGenericItem *pItem = NewGenericItem(WeaponScript);

	//Couldn't search for item, see if exact name was specified
	if (!pItem)
		pItem = NewGenericItem(pszName);

	if (!pItem)
	{
		ALERT(at_console, "Item %s doesn't exist!", pszName);
		return NULL;
	}

	pItem->GiveTo(this);
	pItem->pev->origin = pev->origin;

	return pItem;
}

CBaseEntity *FindEntityForward(CBaseEntity *pMe)
{
	TraceResult tr;

	UTIL_MakeVectors(pMe->pev->v_angle);
	UTIL_TraceLine(pMe->pev->origin + pMe->pev->view_ofs, pMe->pev->origin + pMe->pev->view_ofs + gpGlobals->v_forward * 8192, dont_ignore_monsters, pMe->edict(), &tr);
	if (tr.flFraction != 1.0 && !FNullEnt(tr.pHit))
	{
		CBaseEntity *pHit = CBaseEntity::Instance(tr.pHit);
		return pHit;
	}
	return NULL;
}

/*BOOL CBasePlayer :: FlashlightIsOn( void )
{
	return FBitSet(pev->effects, EF_DIMLIGHT);
}


void CBasePlayer :: FlashlightTurnOn( void )
{
	if ( !g_pGameRules->FAllowFlashlight() )
	{
		return;
	}

	if ( (pev->weapons & (1<<WEAPON_SUIT)) )
	{
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, SOUND_FLASHLIGHT_ON, 1.0, ATTN_NORM, 0, PITCH_NORM );
		SetBits(pev->effects, EF_DIMLIGHT);
		MESSAGE_BEGIN( MSG_ONE, gmsgFlashlight, NULL, pev );
		WRITE_BYTE(1);
		WRITE_BYTE(m_iFlashBattery);
		MESSAGE_END();

		m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->time;

	}
}


void CBasePlayer :: FlashlightTurnOff( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, SOUND_FLASHLIGHT_OFF, 1.0, ATTN_NORM, 0, PITCH_NORM );
    ClearBits(pev->effects, EF_DIMLIGHT);
	MESSAGE_BEGIN( MSG_ONE, gmsgFlashlight, NULL, pev );
	WRITE_BYTE(0);
	WRITE_BYTE(m_iFlashBattery);
	MESSAGE_END();

	m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->time;

}*/

/*
===============
ForceClientDllUpdate

When recording a demo, we need to have the server tell us the entire client state
so that the client side .dll can behave correctly.
Reset stuff so that the state is transmitted.
===============
*/
/*void CBasePlayer :: ForceClientDllUpdate( void )
{
	m_iClientHealth  = -1;
	m_iTrain |= TRAIN_NEW;  // Force new train message.
	m_fWeapon = FALSE;          // Force weapon send
	m_fKnownItem = FALSE;    // Force weaponinit messages.

	// Now force all the necessary messages
	//  to be sent.
	UpdateClientData();
}*/

/*
============
ImpulseCommands
============
*/

void CBasePlayer::ImpulseCommands()
{
	TraceResult tr; // UNDONE: kill me! This is temporary for PreAlpha CDs

	// Handle use events
	PlayerUse();

	int iImpulse = (int)pev->impulse;
	switch (iImpulse)
	{
		/*	case 100:
        // temporary flashlight for level designers
        if ( FlashlightIsOn() )
		{
			FlashlightTurnOff();
		}
        else
		{
			FlashlightTurnOn();
		}
		break;*/

	case 201: // paint decal

		if (gpGlobals->time < m_flNextDecalTime)
		{
			// too early!
			break;
		}

		UTIL_MakeVectors(pev->v_angle);
		UTIL_TraceLine(pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, ENT(pev), &tr);

		if (tr.flFraction != 1.0)
		{ // line hit something, so paint a decal
			m_flNextDecalTime = gpGlobals->time + CVAR_GET_FLOAT("decalfrequency");
			CSprayCan *pCan = GetClassPtr((CSprayCan *)NULL);
			pCan->Spawn(pev);
		}

		break;
		/*	case    204:  //  Demo recording, update client dll specific data again.
		ForceClientDllUpdate();
		break;*/
	default:
		// check all of the cheat impulse commands now
		CheatImpulseCommands(iImpulse);
		break;
	}

	pev->impulse = 0;
}

//=========================================================
//=========================================================
void CBasePlayer::CheatImpulseCommands(int iImpulse)
{
#if !defined(HLDEMO_BUILD)
	if (!IsElite())
		return;

	CBaseEntity *pEntity;
	TraceResult tr;

	switch (iImpulse)
	{
	case 76:
	{
		if (!giPrecacheGrunt)
		{
			giPrecacheGrunt = 1;
			ALERT(at_console, "You must now restart to use Grunt-o-matic.\n");
		}
		else
		{
			UTIL_MakeVectors(Vector(0, pev->v_angle.y, 0));
			Create("monster_human_grunt", pev->origin + gpGlobals->v_forward * 128, pev->angles);
		}
		break;
	}

	case 101:
		gEvilImpulse101 = TRUE;

		//GiveDefaultItems( );

		gEvilImpulse101 = FALSE;
		break;

	case 102:
		// Gibbage!!!
		CGib::SpawnRandomGibs(pev, 1, 1);
		break;

	case 103:
		// What the hell are you doing?
		pEntity = FindEntityForward(this);
		if (pEntity && pEntity->IsMSMonster())
		{
			CMSMonster *pMonster = (CMSMonster *)pEntity;
			if (pMonster)
				pMonster->ReportAIState();
		}
		break;

	case 104:
		// Dump all of the global state varaibles (and global entity names)
		gGlobalState.DumpGlobals();
		break;

	case 105: // player makes no sound for monsters to hear.
	{
		if (m_fNoPlayerSound)
		{
			ALERT(at_console, "Player is audible\n");
			m_fNoPlayerSound = FALSE;
		}
		else
		{
			ALERT(at_console, "Player is silent\n");
			m_fNoPlayerSound = TRUE;
		}
		break;
	}

	case 106:
		// Give me the classname and targetname of this entity.
		pEntity = FindEntityForward(this);
		if (pEntity)
		{
			ALERT(at_console, "Classname: %s", STRING(pEntity->pev->classname));

			if (!FStringNull(pEntity->pev->targetname))
			{
				ALERT(at_console, " - Targetname: %s\n", STRING(pEntity->pev->targetname));
			}
			else
			{
				ALERT(at_console, " - TargetName: No Targetname\n");
			}

			ALERT(at_console, "Model: %s\n", STRING(pEntity->pev->model));
			if (pEntity->pev->globalname)
				ALERT(at_console, "Globalname: %s\n", STRING(pEntity->pev->globalname));
		}
		break;

	case 107:
	{
		TraceResult tr;

		edict_t *pWorld = g_engfuncs.pfnPEntityOfEntIndex(0);

		Vector start = pev->origin + pev->view_ofs;
		Vector end = start + gpGlobals->v_forward * 1024;
		UTIL_TraceLine(start, end, ignore_monsters, edict(), &tr);
		if (tr.pHit)
			pWorld = tr.pHit;
		const char *pTextureName = TRACE_TEXTURE(pWorld, start, end);
		if (pTextureName)
			ALERT(at_console, "Texture: %s\n", pTextureName);
	}
	break;
	case 195: // show shortest paths for entire level to nearest node
	{
		Create("node_viewer_fly", pev->origin, pev->angles);
	}
	break;
	case 196: // show shortest paths for entire level to nearest node
	{
		Create("node_viewer_large", pev->origin, pev->angles);
	}
	break;
	case 197: // show shortest paths for entire level to nearest node
	{
		Create("node_viewer_human", pev->origin, pev->angles);
	}
	break;
	case 199: // show nearest node and all connections
	{
		ALERT(at_console, "%d\n", WorldGraph.FindNearestNode(pev->origin, bits_NODE_GROUP_REALM));
		WorldGraph.ShowNodeConnections(WorldGraph.FindNearestNode(pev->origin, bits_NODE_GROUP_REALM));
	}
	break;
	case 202: // Random blood splatter
		UTIL_MakeVectors(pev->v_angle);
		UTIL_TraceLine(pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, ENT(pev), &tr);

		if (tr.flFraction != 1.0)
		{ // line hit something, so paint a decal
			CBloodSplat *pBlood = GetClassPtr((CBloodSplat *)NULL);
			pBlood->Spawn(pev);
		}
		break;
	case 203: // remove creature.
		pEntity = FindEntityForward(this);
		if (pEntity)
		{
			if (pEntity->pev->takedamage)
				pEntity->SetThink(&CBasePlayer::SUB_Remove);
		}
		break;
	}
#endif // HLDEMO_BUILD
}

/*
============
ItemPreFrame

Called every frame by the player PreThink
============
*/
void CBasePlayer::ItemPreFrame()
{
}

/*
============
ItemPostFrame

Called every frame by the player PostThink
============
*/
void CBasePlayer::ItemPostFrame()
{
	if (m_afPhysicsFlags & PFLAG_OBSERVER)
		return;

	// check if the player is using a tank
	if (m_pTank != NULL)
		return;

	ImpulseCommands();

	CGenericItem* pMainHand = Hand(0);
	CGenericItem* pAltHand = Hand(1);

	if (pMainHand)
		pMainHand->ItemPostFrame();
	if (pAltHand && pMainHand != pAltHand)
		pAltHand->ItemPostFrame();
}

/*
=========================================================
	UpdateClientData

Resends any changed player HUD info to the client.
Called every frame by PlayerPreThink
Also called at start of demo recording and playback by
ForceClientDllUpdate to ensure the demo gets messages
reflecting all of the HUD state info.
=========================================================
*/

extern cvar_t msallowkickvote;
extern cvar_t msallowtimevote;
extern cvar_t ms_serverchar;

void CBasePlayer::UpdateClientData(void)
{
	bool fConnectedThisFrame = false;
	bool bShouldSaveChar = false;

	//Init HUD message MUST be first!
	if (m_fInitHUD)
	{
		//Thothie nothing changed here, but somewhere whithin we are getting:
		//Error (SERVER): Error: CBasePlayer::UpdateClientData --> Call InitHUD
		//Maybe caused by a script, not sure
		m_fInitHUD = FALSE;
		gInitHUD = FALSE;

		MESSAGE_BEGIN(MSG_ONE, gmsgResetHUD, NULL, pev);
		WRITE_BYTE(0);
		MESSAGE_END();

		if (!m_fGameHUDInitialized)
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgInitHUD, NULL, pev);
			WRITE_STRING_LIMIT(CVAR_GET_STRING("hostname"), 64);
			WRITE_STRING_LIMIT(STRING(gpGlobals->mapname), 32);

			//Find the proper IP address to send to this player.  I
			//If he matches my lan address, I'll send that one.
			//If otherwise send MS_IP
			/*msstring PlayerInterface = g_NetCode.GetServerIPForPlayer( this );
				PlayerInterface += msstring_ref(msstring(":") + (int)g_NetCode.s.FilePort);
				logfile << "Sending server IP to " << DisplayName() << ": " << PlayerInterface.c_str() << "\r\n";
				WRITE_STRING( PlayerInterface.c_str() );*/
			for (int i = 0; i < CLPERMENT_TOTAL; i++)
				WRITE_SHORT(MSGlobals::ClEntities[i]);

			byte Flags = 0;
			if (IsLocalHost())
				Flags |= (1 << 0); //Player is localhost in listen server?
			if (MSGlobals::IsLanGame)
				Flags |= (1 << 1); //Lan game?
			if (MSGlobals::CanCreateCharOnMap)
				Flags |= (1 << 2); //Can create chars on this map?
			if (MSGlobals::GameType == GAMETYPE_ADVENTURE)
				Flags |= (1 << 3); //Saving character allowed?
			if (MSGlobals::ServerSideChar)
				Flags |= (1 << 4); //Server-side characters?
			if (GetOtherPlayerTransition(this))
				Flags |= (1 << 5); //Other players are on and eligible to join?
			WRITE_BYTE(Flags);
			WRITE_STRING_LIMIT(GETPLAYERAUTHID(edict()), 32);

			//Send allowed votes
			byte VotesAllowed = 0;
			if (msallowkickvote.value)
				SetBits(VotesAllowed, (1 << 0));
			if (msallowtimevote.value)
				SetBits(VotesAllowed, (1 << 1));
			WRITE_BYTE(VotesAllowed);			  //Type of votes allowed
			WRITE_BYTE((int)ms_serverchar.value); //Number of characters allowed
			MESSAGE_END();

			if (MSGlobals::CanCreateCharOnMap)
				m_CanJoin = true; //Player can join this map

			//Central server info (on/off, name)
			MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_CLDLLFUNC], NULL, pev);
			WRITE_BYTE(6);
			WRITE_BYTE(FNShared::IsEnabled() ? 1 : 0);
			MESSAGE_END();

			m_fGameHUDInitialized = TRUE;
			InitHUD();
			fConnectedThisFrame = true;
		}

		InitStatusBar();
		FireTargets("game_playerspawn", this, this, USE_TOGGLE, 0);
	}

	if (FBitSet(m_iHideHUD, HIDEHUD_ALL))
		return;

	//pev->renderfx = pev->body; //JAN2010 - MiB Aug2008a - body doesn't pass properly, but renderfx does. We now use it for submodel work.

	if (FBitSet(m_MsgFlags, MSGFLAG_SPAWN))
	{
		//Call the client spawn function
		ClearBits(m_MsgFlags, MSGFLAG_SPAWN);
		MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_CLDLLFUNC], NULL, pev);
		WRITE_BYTE(0);
		MESSAGE_END();

		//Send all quickslots
		MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_CLDLLFUNC], NULL, pev); //Send the quickslots
		WRITE_BYTE(21);
		for (int i = 0; i < MAX_QUICKSLOTS; i++)
		{
			quickslot_t &QuickSlot = m_QuickSlots[i];
			WRITE_BYTE(QuickSlot.Active ? 1 : 0);
			if (QuickSlot.Active)
			{
				WRITE_BYTE(QuickSlot.Type);
				WRITE_LONG(QuickSlot.ID);
			}
		}
		MESSAGE_END();
	}

	/*	if ( m_iFatigue != iFatigue ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgFatigue, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_BYTE( iFatigue );
		MESSAGE_END();
		m_iFatigue = iFatigue;
	}
	if ( m_iMaxFatigue != MaxFatigue() ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgFatigue, NULL, pev );
			WRITE_BYTE( 1 );
			WRITE_BYTE( MaxFatigue() );
		MESSAGE_END();
		m_iMaxFatigue = MaxFatigue();
	}*/

	//Send new items
	for (int i = 0; i < Gear.size(); i++)
	{
		CGenericItem *pItem = Gear[i];

		clientitem_t ClientItem(pItem);

		bool fInCache = false, fCached = false;

		for (int n = 0; n < m_ClientItems.size(); n++)
			if (m_ClientItems[n].ID == pItem->m_iId)
			{
				fInCache = true;

				/*bool Same = true;
				 for (int b = 0; b < sizeof(clientitem_t); b++)
				{
					byte &SrcByte = ((byte *)&ClientItem)[b];
					byte &DstByte = ((byte *)&m_ClientItems[n])[b];
					if( SrcByte != DstByte )
						Same = false;
				}*/

				//if( Same )
				if (!memcmp(&m_ClientItems[n], &ClientItem, sizeof(clientitem_t)))
					//Found this item in the cache with the exact same properties... do not update
					fCached = true;
				else
					//Found this item, but some properties are different... resend the item and update the cache now
					memcpy(&m_ClientItems[n], &ClientItem, sizeof(clientitem_t));
				break;
			}

		if (fCached)
			continue;

		//Send the item
		MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_ITEM], NULL, pev);
		WRITE_BYTE(fInCache ? 1 : 0); //0 == New | 1 == Update existing
		SendGenericItem(this, pItem, false);
		MESSAGE_END();
		bShouldSaveChar = true; // Save the item

		if (!fInCache) //If it's a completely new item...
		{
			//Thothie FEB2011_16 Sort items
			pItem->Container_StackItems();

			//If it's a container, send its conents
			pItem->Container_SendContents();

			//Update the cache with this item
			// clientitem_t &CachedItem =
			m_ClientItems.add(ClientItem);
		}
	}

	//Update and delete old items
	for (int i = 0; i < m_ClientItems.size(); i++)
	{
		ulong ClientItemID = m_ClientItems[i].ID;

		if (Gear.GetItem(ClientItemID))
			continue;

		//Send a remove item message
		MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_ITEM], NULL, pev);
		WRITE_BYTE(2);
		WRITE_LONG(ClientItemID);
		MESSAGE_END();
		bShouldSaveChar = true; // Remove the item from save

		m_ClientItems.erase(i--); //Have to decrement i after the call so I don't skip items
	}

	if (bShouldSaveChar)
		SaveChar();

	//Active Hand
	if (m_CurrentHand != m_ClientCurrentHand)
	{
		MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_HANDS], NULL, pev);
		WRITE_BYTE(m_CurrentHand);
		MESSAGE_END();

		m_ClientCurrentHand = m_CurrentHand;
	}

	for (int i = 0; i < m_Stats.size(); i++)
	{
		CStat &Stat = m_Stats[i];
		if (Stat.Changed())
		{
			MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_SETSTAT], NULL, pev);
			WRITE_BYTE(1);
			WRITE_BYTE(i);
			for (int r = 0; r < Stat.m_SubStats.size(); r++)
			{
				WRITE_BYTE(Stat.m_SubStats[r].Value);
				if (Stat.m_Type == CStat::STAT_SKILL)
					WRITE_SHORT(Stat.m_SubStats[r].Exp);
			}
			MESSAGE_END();

			Stat.Update();
		}
	}

	//MiB Modded JAN2010_15 - Gold Change on Spawn.rtf
	if (m_OldGold != m_Gold)
	{
		MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_SETSTAT], NULL, pev);
		WRITE_BYTE(3);
		WRITE_BYTE(1);
		WRITE_LONG(m_Gold);
		MESSAGE_END();
		m_OldGold = m_Gold;
	}

	if (m_Gender != m_ClientGender)
	{
		MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_SETSTAT], NULL, pev);
		WRITE_BYTE(10);
		WRITE_BYTE(m_Gender);
		MESSAGE_END();

		m_ClientGender = m_Gender;
	}

	if (m_iHideHUD != m_iClientHideHUD)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgHideWeapon, NULL, pev);
		WRITE_BYTE(m_iHideHUD);
		MESSAGE_END();

		m_iClientHideHUD = m_iHideHUD;
	}

	if (m_iFOV != m_iClientFOV)
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgSetFOV, NULL, pev);
		WRITE_BYTE(m_iFOV);
		MESSAGE_END();

		// cache FOV change at end of function, so weapon updates can see that FOV has changed
	}

	// HACKHACK -- send the message to display the game title
	/*if (gDisplayTitle)
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgShowGameTitle, NULL, pev );
			WRITE_BYTE( 0 );
		MESSAGE_END();
		gDisplayTitle = 0;
	}*/

	int Msg = 0, Amt = 0, *pLastAmt = NULL, Type = 0;
	for (int i = 0; i < 4; i++)
	{
		switch (i)
		{
		case 0:
			Msg = g_netmsg[NETMSG_HP];
			Amt = m_HP;
			pLastAmt = &m_iClientHP;
			Type = 0;
			break;
		case 1:
			Msg = g_netmsg[NETMSG_HP];
			Amt = MaxHP();
			pLastAmt = &m_iClientMaxHP;
			Type = 1;
			break;
		case 2:
			Msg = g_netmsg[NETMSG_MP];
			Amt = m_MP;
			pLastAmt = &m_iClientMP;
			Type = 0;
			break;
		case 3:
			Msg = g_netmsg[NETMSG_MP];
			Amt = MaxMP();
			pLastAmt = &m_iClientMaxMP;
			Type = 1;
			break;
		}

		if (Amt == *pLastAmt)
			continue;

		//MIB JUN2010_14 - "MP and HP sends Fix.rtf"
		if (i == 0)
		{
			if (gpGlobals->time - m_timeLastSentHP < 0.5)
				continue;
			m_timeLastSentHP = gpGlobals->time;
		}
		else if (i == 2)
		{
			if (gpGlobals->time - m_timeLastSentMP < 0.5)
				continue;
			m_timeLastSentMP = gpGlobals->time;
		}

		MESSAGE_BEGIN(MSG_ONE, Msg, NULL, pev);
		if (Amt > (signed short)(SHRT_MAX / 2))
			Amt = (SHRT_MAX / 2);
		WRITE_SHORT(Amt);
		WRITE_BYTE(Type);
		MESSAGE_END();

		*pLastAmt = Amt;
	}

	//MAR2010_08 hacks, trying to see if undoing this resolves char slot corruption issue
	//Thothie - Yes, this is it what is causing the m_CharacterNum corruption, just no idea why
	/*
	float *pLastSent; //MiB JAN2010_26 - Slowing down how often these send

	//MiBJAN2010_17 - alterations to loop below
	 for (int i = 0; i < 4; i++)
	{
		switch( i )
		{
			case 0: Msg = g_netmsg[NETMSG_HP]; Amt = m_HP		; pLastAmt = &m_iClientHP	; pLastSent = &m_timeLastSentHP;	Type = 0; break;
			case 1: Msg = g_netmsg[NETMSG_HP]; Amt = MaxHP( )	; pLastAmt = &m_iClientMaxHP; pLastSent = &m_timeLastSentMaxHP;	Type = 1; break;
			case 2: Msg = g_netmsg[NETMSG_MP]; Amt = m_MP		; pLastAmt = &m_iClientMP	; pLastSent = &m_timeLastSentMP;	Type = 0; break;
			case 3: Msg = g_netmsg[NETMSG_MP]; Amt = MaxMP( )	; pLastAmt = &m_iClientMaxMP; pLastSent = &m_timeLastSentMaxMP; Type = 1; break;
		}

		if( Amt == *pLastAmt )
			continue;

		if ( (gpGlobals->time - *pLastSent) < 0.5 ) //MiB JAN2010_27 - alter this number to update more frequently
			continue;

		MESSAGE_BEGIN( MSG_ONE, Msg, NULL, pev );
			if( Amt > (signed short)(SHRT_MAX/2) ) Amt = (SHRT_MAX/2);
			WRITE_SHORT( Amt ); WRITE_BYTE( Type );
		MESSAGE_END();

		*pLastAmt = Amt;
		*pLastSent = gpGlobals->time;
	}
	*/
	//[/end] MiB JAN2010_26 - Slowing down how often these send

	if (m_ClPlayersKilled != m_PlayersKilled)
	{
		MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_CLDLLFUNC], NULL, pev);
		WRITE_BYTE(8);
		WRITE_SHORT(m_PlayersKilled);
		MESSAGE_END();
		m_ClPlayersKilled = m_PlayersKilled;
	}

	//Update the kill waited time every 10 secs
	if (fabs(m_TimeWaitedToForgetKill - m_ClTimeWaitedToForgetKill) > 10)
	{
		MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_CLDLLFUNC], NULL, pev);
		WRITE_BYTE(9);
		WRITE_COORD(m_TimeWaitedToForgetKill);
		MESSAGE_END();
		m_ClTimeWaitedToForgetKill = m_TimeWaitedToForgetKill;
		///ALERT( at_console, "%s - Time: %2.f/%i\n", STRING(DisplayName), m_TimeWaitedToForgetKill, m_PlayersKilled * 10 );
	}

	//Update the steal waited time every 10 secs
	if (fabs(m_TimeWaitedToForgetSteal - m_ClTimeWaitedToForgetSteal) > 10)
	{
		MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_CLDLLFUNC], NULL, pev);
		WRITE_BYTE(13);
		WRITE_COORD(m_TimeWaitedToForgetSteal);
		MESSAGE_END();
		m_ClTimeWaitedToForgetSteal = m_TimeWaitedToForgetSteal;
		///ALERT( at_console, "%s - Time: %2.f/%i\n", STRING(DisplayName), m_TimeWaitedToForgetKill, m_PlayersKilled * 10 );
	}

	//Update player actions list
	ClearBits(m_StatusFlags, PLAYER_MOVE_NORUN | PLAYER_MOVE_NOJUMP | PLAYER_MOVE_NODUCK | PLAYER_MOVE_NOATTACK | PLAYER_MOVE_NOMOVE | PLAYER_MOVE_STOPRUN);

	for (int i = 0; i < m_Scripts.size(); i++)
	{
		CScript *Script = m_Scripts[i];

		//Update stuff based on effect script variables.
		//To check for a 'true' blocking value make sure that it has explicitly set to "0".  If it was never set at all, assume false.
		int EffectFlags = 0;
		if (!strcmp(Script->GetVar("game.effect.canmove"), "0"))
			SetBits(EffectFlags, PLAYER_MOVE_NOMOVE);
		if (!strcmp(Script->GetVar("game.effect.canrun"), "0"))
			SetBits(EffectFlags, PLAYER_MOVE_NORUN);
		if (!strcmp(Script->GetVar("game.effect.canjump"), "0"))
			SetBits(EffectFlags, PLAYER_MOVE_NOJUMP);
		if (!strcmp(Script->GetVar("game.effect.canduck"), "0"))
			SetBits(EffectFlags, PLAYER_MOVE_NODUCK);
		if (!strcmp(Script->GetVar("game.effect.canattack"), "0"))
			SetBits(EffectFlags, PLAYER_MOVE_NOATTACK);

		SetBits(m_StatusFlags, EffectFlags); //Add the movement blocking flags from this effect to the player's list

		if (strcmp(Script->GetVar("game.effect.updateplayer"), "1"))
			continue;

		//logfile << "DEBUG: ApplyEffect->Client " << Script->GetVar("game.effect.displayname") << "\n";

		MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_CLDLLFUNC], NULL, pev);
		WRITE_BYTE(16);
		WRITE_BYTE(1);
		WRITE_STRING_LIMIT(Script->GetVar("game.effect.id"), 90);
		WRITE_STRING_LIMIT(Script->GetVar("game.effect.displayname"), 90);
		MESSAGE_END();

		Script->SetVar("game.effect.updateplayer", 0);
	}

	/*if (pev->dmg_take || pev->dmg_save || m_bitsHUDDamage != m_bitsDamageType)
	{
		// Comes from inside me if not set
		Vector damageOrigin = pev->origin;
		// send "damage" message
		// causes screen to flash, and pain compass to show direction of damage
		edict_t *other = pev->dmg_inflictor;
		if ( other )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance(other);
			if ( pEntity )
				damageOrigin = pEntity->Center();
		}

		// only send down damage type that have hud art
		int visibleDamageBits = m_bitsDamageType & DMG_SHOWNHUD;

		MESSAGE_BEGIN( MSG_ONE, gmsgDamage, NULL, pev );
			WRITE_BYTE( pev->dmg_save );
			WRITE_BYTE( pev->dmg_take );
			WRITE_LONG( visibleDamageBits );
			WRITE_COORD( damageOrigin.x );
			WRITE_COORD( damageOrigin.y );
			WRITE_COORD( damageOrigin.z );
		MESSAGE_END();

		pev->dmg_take = 0;
		pev->dmg_save = 0;
		m_bitsHUDDamage = m_bitsDamageType;

		// Clear off non-time-based damage indicators
		m_bitsDamageType &= DMG_TIMEBASED;
	}*/

	if (m_iTrain & TRAIN_NEW)
	{
		ASSERT(gmsgTrain > 0);
		// send "health" update message
		MESSAGE_BEGIN(MSG_ONE, gmsgTrain, NULL, pev);
		WRITE_BYTE(m_iTrain & 0xF);
		MESSAGE_END();

		m_iTrain &= ~TRAIN_NEW;
	}

	// Cache and client weapon change
	m_pClientActiveItem = m_pActiveItem;
	m_iClientFOV = m_iFOV;

	// Update Status Bar
	/*if ( m_flNextSBarUpdateTime < gpGlobals->time )
	{
		UpdateStatusBar();
		m_flNextSBarUpdateTime = gpGlobals->time + 0.2;
	}*/
}

//=========================================================
// FBecomeProne - Overridden for the player to set the proper
// physics flags when a barnacle grabs player.
//=========================================================
BOOL CBasePlayer ::FBecomeProne(void)
{
	m_afPhysicsFlags |= PFLAG_ONBARNACLE;
	return TRUE;
}

//=========================================================
// BarnacleVictimBitten - bad name for a function that is called
// by Barnacle victims when the barnacle pulls their head
// into its mouth. For the player, just die.
//=========================================================
void CBasePlayer ::BarnacleVictimBitten(entvars_t *pevBarnacle)
{
	TakeDamage(pevBarnacle, pevBarnacle, pev->health + pev->armorvalue, DMG_SLASH | DMG_ALWAYSGIB);
}

//=========================================================
// BarnacleVictimReleased - overridden for player who has
// physics flags concerns.
//=========================================================
void CBasePlayer ::BarnacleVictimReleased(void)
{
	m_afPhysicsFlags &= ~PFLAG_ONBARNACLE;
}

//=========================================================
// Illumination
// return player light level plus virtual muzzle flash
//=========================================================
int CBasePlayer ::Illumination(void)
{
	int iIllum = CBaseEntity::Illumination();

	iIllum += m_iWeaponFlash;
	if (iIllum > 255)
		return 255;
	return iIllum;
}

void CBasePlayer ::EnableControl(BOOL fControl)
{
	if (!fControl)
		pev->flags |= FL_FROZEN;
	else
		pev->flags &= ~FL_FROZEN;
}

#define DOT_1DEGREE 0.9998476951564
#define DOT_2DEGREE 0.9993908270191
#define DOT_3DEGREE 0.9986295347546
#define DOT_4DEGREE 0.9975640502598
#define DOT_5DEGREE 0.9961946980917
#define DOT_6DEGREE 0.9945218953683
#define DOT_7DEGREE 0.9925461516413
#define DOT_8DEGREE 0.9902680687416
#define DOT_9DEGREE 0.9876883405951
#define DOT_10DEGREE 0.9848077530122
#define DOT_15DEGREE 0.9659258262891
#define DOT_20DEGREE 0.9396926207859
#define DOT_25DEGREE 0.9063077870367

//=========================================================
// Autoaim
// set crosshair position to point to enemey
//=========================================================
Vector CBasePlayer ::GetAutoaimVector(float flDelta)
{
	UTIL_MakeVectors(pev->v_angle + pev->punchangle);
	return gpGlobals->v_forward;

	/*if (g_iSkillLevel == SKILL_HARD)
	{
		UTIL_MakeVectors( pev->v_angle + pev->punchangle );
		return gpGlobals->v_forward;
	}

	Vector vecSrc = GetGunPosition( );
	float flDist = 8192;

	// always use non-sticky autoaim
	// UNDONE: use sever variable to chose!
	if (1 || g_iSkillLevel == SKILL_MEDIUM)
	{
		m_vecAutoAim = Vector( 0, 0, 0 );
		// flDelta *= 0.5;
	}

	BOOL m_fOldTargeting = m_fOnTarget;
	Vector angles = AutoaimDeflection(vecSrc, flDist, flDelta );

	// update ontarget if changed
	if ( !g_pGameRules->AllowAutoTargetCrosshair() )
		m_fOnTarget = 0;
	else if (m_fOldTargeting != m_fOnTarget)
	{
		m_pActiveItem->UpdateItemInfo( );
	}

	if (angles.x > 180)
		angles.x -= 360;
	if (angles.x < -180)
		angles.x += 360;
	if (angles.y > 180)
		angles.y -= 360;
	if (angles.y < -180)
		angles.y += 360;

	if (angles.x > 25)
		angles.x = 25;
	if (angles.x < -25)
		angles.x = -25;
	if (angles.y > 12)
		angles.y = 12;
	if (angles.y < -12)
		angles.y = -12;


	// always use non-sticky autoaim
	// UNDONE: use sever variable to chose!
	if (0 || g_iSkillLevel == SKILL_EASY)
	{
		m_vecAutoAim = m_vecAutoAim * 0.67 + angles * 0.33;
	}
	else
	{
		m_vecAutoAim = angles * 0.9;
	}

	// m_vecAutoAim = m_vecAutoAim * 0.99;

	// Don't send across network if sv_aim is 0
	if ( CVAR_GET_FLOAT( "sv_aim" ) != 0 )
	{
		if ( m_vecAutoAim.x != m_lastx ||
			 m_vecAutoAim.y != m_lasty )
		{
			SET_CROSSHAIRANGLE( edict(), -m_vecAutoAim.x, m_vecAutoAim.y );

			m_lastx = m_vecAutoAim.x;
			m_lasty = m_vecAutoAim.y;
		}
	}

	// ALERT( at_console, "%f %f\n", angles.x, angles.y );

	UTIL_MakeVectors( pev->v_angle + pev->punchangle + m_vecAutoAim );
	return gpGlobals->v_forward;*/
}

/*Vector CBasePlayer :: AutoaimDeflection( Vector &vecSrc, float flDist, float flDelta  )
{
	edict_t		*pEdict = g_engfuncs.pfnPEntityOfEntIndex( 1 );
	CBaseEntity	*pEntity;
	float		bestdot;
	Vector		bestdir;
	edict_t		*bestent;
	TraceResult tr;

	if ( CVAR_GET_FLOAT("sv_aim") == 0 )
	{
		m_fOnTarget = FALSE;
		return g_vecZero;
	}

	UTIL_MakeVectors( pev->v_angle + pev->punchangle + m_vecAutoAim );

	// try all possible entities
	bestdir = gpGlobals->v_forward;
	bestdot = flDelta; // +- 10 degrees
	bestent = NULL;

	m_fOnTarget = FALSE;

	UTIL_TraceLine( vecSrc, vecSrc + bestdir * flDist, dont_ignore_monsters, edict(), &tr );


	if ( tr.pHit && tr.pHit->v.takedamage != DAMAGE_NO)
	{
		// don't look through water
		if (!((pev->waterlevel != 3 && tr.pHit->v.waterlevel == 3)
			|| (pev->waterlevel == 3 && tr.pHit->v.waterlevel == 0)))
		{
			if (tr.pHit->v.takedamage == DAMAGE_AIM)
				m_fOnTarget = TRUE;

			return m_vecAutoAim;
		}
	}

	for ( int i = 1; i < gpGlobals->maxEntities; i++, pEdict++ )
	{
		Vector center;
		Vector dir;
		float dot;

		if ( pEdict->free )	// Not in use
			continue;

		if (pEdict->v.takedamage != DAMAGE_AIM)
			continue;
		if (pEdict == edict())
			continue;
//		if (pev->team > 0 && pEdict->v.team == pev->team)
//			continue;	// don't aim at teammate
		if ( !g_pGameRules->ShouldAutoAim( this, pEdict ) )
			continue;

		pEntity = Instance( pEdict );
		if (pEntity == NULL)
			continue;

		if (!pEntity->IsAlive())
			continue;

		// don't look through water
		if ((pev->waterlevel != 3 && pEntity->pev->waterlevel == 3)
			|| (pev->waterlevel == 3 && pEntity->pev->waterlevel == 0))
			continue;

		center = pEntity->BodyTarget( vecSrc );

		dir = (center - vecSrc).Normalize( );

		// make sure it's in front of the player
		if (DotProduct (dir, gpGlobals->v_forward ) < 0)
			continue;

		dot = fabs( DotProduct (dir, gpGlobals->v_right ) )
			+ fabs( DotProduct (dir, gpGlobals->v_up ) ) * 0.5;

		// tweek for distance
		dot *= 1.0 + 0.2 * ((center - vecSrc).Length() / flDist);

		if (dot > bestdot)
			continue;	// to far to turn

		UTIL_TraceLine( vecSrc, center, dont_ignore_monsters, edict(), &tr );
		if (tr.flFraction != 1.0 && tr.pHit != pEdict)
		{
			// ALERT( at_console, "hit %s, can't see %s\n", STRING( tr.pHit->v.classname ), STRING( pEdict->v.classname ) );
			continue;
		}

		// don't shoot at friends
		if (IRelationship( pEntity ) < 0)
		{
			if ( !pEntity->IsPlayer() && !g_pGameRules->IsDeathmatch())
				// ALERT( at_console, "friend\n");
				continue;
		}

		// can shoot at this one
		bestdot = dot;
		bestent = pEdict;
		bestdir = dir;
	}

	if (bestent)
	{
		bestdir = UTIL_VecToAngles (bestdir);
		bestdir.x = -bestdir.x;
		bestdir = bestdir - pev->v_angle - pev->punchangle;

		if (bestent->v.takedamage == DAMAGE_AIM)
			m_fOnTarget = TRUE;

		return bestdir;
	}

	return Vector( 0, 0, 0 );
}*/

void CBasePlayer ::ResetAutoaim()
{
	if (m_vecAutoAim.x != 0 || m_vecAutoAim.y != 0)
	{
		m_vecAutoAim = Vector(0, 0, 0);
		SET_CROSSHAIRANGLE(edict(), 0, 0);
	}
	m_fOnTarget = FALSE;
}

/*
=============
SetCustomDecalFrames

  UNDONE:  Determine real frame limit, 8 is a placeholder.
  Note:  -1 means no custom frames present.
=============
*/
void CBasePlayer ::SetCustomDecalFrames(int nFrames)
{
	if (nFrames > 0 &&
		nFrames < 8)
		m_nCustomSprayFrames = nFrames;
	else
		m_nCustomSprayFrames = -1;
}

/*
=============
GetCustomDecalFrames

  Returns the # of custom frames this player's custom clan logo contains.
=============
*/
int CBasePlayer ::GetCustomDecalFrames(void)
{
	return m_nCustomSprayFrames;
}

class CStripWeapons : public CPointEntity
{
public:
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

private:
};

LINK_ENTITY_TO_CLASS(player_weaponstrip, CStripWeapons);

void CStripWeapons ::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBasePlayer *pPlayer = NULL;

	if (pActivator && pActivator->IsPlayer())
	{
		pPlayer = (CBasePlayer *)pActivator;
	}
	else if (!g_pGameRules->IsDeathmatch())
	{
		pPlayer = (CBasePlayer *)CBaseEntity::Instance(g_engfuncs.pfnPEntityOfEntIndex(1));
	}

	if (pPlayer)
		pPlayer->RemoveAllItems(FALSE);
}

class CRevertSaved : public CPointEntity
{
public:
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT MessageThink(void);
	void EXPORT LoadThink(void);
	void KeyValue(KeyValueData *pkvd);

	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

	inline float Duration(void) { return pev->dmg_take; }
	inline float HoldTime(void) { return pev->dmg_save; }
	inline float MessageTime(void) { return m_messageTime; }
	inline float LoadTime(void) { return m_loadTime; }

	inline void SetDuration(float duration) { pev->dmg_take = duration; }
	inline void SetHoldTime(float hold) { pev->dmg_save = hold; }
	inline void SetMessageTime(float time) { m_messageTime = time; }
	inline void SetLoadTime(float time) { m_loadTime = time; }

private:
	float m_messageTime;
	float m_loadTime;
};

LINK_ENTITY_TO_CLASS(player_loadsaved, CRevertSaved);

TYPEDESCRIPTION CRevertSaved::m_SaveData[] =
	{
		DEFINE_FIELD(CRevertSaved, m_messageTime, FIELD_FLOAT), // These are not actual times, but durations, so save as floats
		DEFINE_FIELD(CRevertSaved, m_loadTime, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CRevertSaved, CPointEntity);

void CRevertSaved ::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "duration"))
	{
		SetDuration(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "holdtime"))
	{
		SetHoldTime(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "messagetime"))
	{
		SetMessageTime(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "loadtime"))
	{
		SetLoadTime(atof(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue(pkvd);
}

void CRevertSaved ::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	UTIL_ScreenFadeAll(pev->rendercolor, Duration(), HoldTime(), pev->renderamt, FFADE_OUT);
	pev->nextthink = gpGlobals->time + MessageTime();
	SetThink(&CRevertSaved::MessageThink);
}

void CRevertSaved ::MessageThink(void)
{
	UTIL_ShowMessageAll(STRING(pev->message));
	float nextThink = LoadTime() - MessageTime();
	if (nextThink > 0)
	{
		pev->nextthink = gpGlobals->time + nextThink;
		SetThink(&CRevertSaved::LoadThink);
	}
	else
		LoadThink();
}

void CRevertSaved ::LoadThink(void)
{
	if (!gpGlobals->deathmatch)
	{
		SERVER_COMMAND("reload\n");
	}
}

//=========================================================
// Multiplayer intermission spots.
//=========================================================
class CInfoIntermission : public CPointEntity
{
	void Spawn(void);
	void Think(void);
};

void CInfoIntermission::Spawn(void)
{
	UTIL_SetOrigin(pev, pev->origin);
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->v_angle = g_vecZero;

	pev->nextthink = gpGlobals->time + 2; // let targets spawn!
}

void CInfoIntermission::Think(void)
{
	edict_t *pTarget;

	// find my target
	pTarget = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(pev->target));

	if (!FNullEnt(pTarget))
	{
		pev->v_angle = UTIL_VecToAngles((pTarget->v.origin - pev->origin).Normalize());
		pev->v_angle.x = -pev->v_angle.x;
	}
}

LINK_ENTITY_TO_CLASS(info_intermission, CInfoIntermission);

void CBasePlayer::UpdateMiscPositions(void)
{
	if (gpGlobals->time > TimeUpdateIDInfo)
	{
		TimeUpdateIDInfo = gpGlobals->time + 1.0; //0.4

		//Find my current target
		Vector vForward;
		UTIL_MakeVectorsPrivate(pev->v_angle + pev->punchangle, vForward, NULL, NULL);
		Vector vEndPos = EyePosition() + vForward * 2048;

		TraceResult tr;
		gpGlobals->trace_flags |= FTRACE_SIMPLEBOX;
		UTIL_TraceLine(EyePosition(), vEndPos, dont_ignore_monsters, edict(), &tr);
		CBaseEntity *pTarget = NULL;
		if (tr.flFraction < 1.0)
		{
			pTarget = MSInstance(tr.pHit);
			if (pTarget && pTarget->Classify() == CLASS_NONE)
				pTarget = NULL;
		}

		CBaseEntity *pOldTarget = RetrieveEntity(ENT_TARGET);
		StoreEntity(pTarget, ENT_TARGET);
		if (pTarget && pTarget != pOldTarget && pTarget->GetScripted())
		{
			static msstringlist Params;
			Params.clearitems();
			Params.add(EntToString(this));
			((CScriptedEnt *)pTarget)->CallScriptEvent("game_targeted_by_player", &Params);
		}

		//Update HUD entity info (player/monster/item names)
		int EntriesSent = 0;
		float Range = 1024;
		Vector vMinBounds = pev->origin - Vector(Range, Range, Range);
		Vector vMaxBounds = pev->origin + Vector(Range, Range, Range);

		int count = UTIL_EntitiesInBox(g_pEntitiesInBox, MAX_ENTITIES_TO_SEARCH, vMinBounds, vMaxBounds, 0);
		for (int i = 0; i < count && EntriesSent < 2; i++) //only send 2 at a time now
		{
			CBaseEntity *pSightEnt = g_pEntitiesInBox[i];

			if (!pSightEnt ||
				pSightEnt == this ||
				pSightEnt->pev->movetype == MOVETYPE_FOLLOW)
				continue;
			if (!pSightEnt->IsPlayer() &&
				!pSightEnt->MyMonsterPointer())
				continue;

			if (pSightEnt->IsMSItem() /*&& pSightEnt->MSMoveType != MOVETYPE_ARROW */)
				continue;

			entinfo_t EntData;
			EntData.entindex = ENTINDEX(pSightEnt->edict());
			EntData.Name = pSightEnt->DisplayName();
			EntData.Type = ENT_NEUTRAL;
			if (pSightEnt->IsMSMonster())
			{
				if (pSightEnt->IsPlayer())
				{
					CBasePlayer *pTestPlayer = (CBasePlayer *)pSightEnt;
					if (pTestPlayer->m_TimeWaitedToForgetSteal)
					{
						EntData.Name += " [THIEF]";
						EntData.Type = ENT_HOSTILE;
					}
				}
				if (EntData.Type == ENT_NEUTRAL)
				{
					int iRelationship = IRelationship(pSightEnt);
					switch(iRelationship)
					{
					case RELATIONSHIP_AL: //friendly
						EntData.Type = ENT_FRIENDLY;
						break;
					case RELATIONSHIP_HT: //hostile
						EntData.Type = ENT_HOSTILE;
						break;
					case RELATIONSHIP_DL: //dislike
						EntData.Type = ENT_HOSTILE;
						break;
					case RELATIONSHIP_WA: //wary
						EntData.Type = ENT_WARY;
						break;
					case RELATIONSHIP_NM: //nemesis
						EntData.Type = ENT_DEADLY;
						break;
					case RELATIONSHIP_NO: //not NPC
						EntData.Type = ENT_NOTNPC;
						break;
					}

					//Thothie DEC2012_12 - boss hud ID
					IScripted *pScripted = pSightEnt->GetScripted();
					if (atoi(pScripted->GetFirstScriptVar("NPC_IS_BOSS")) > 0)
						EntData.Type = ENT_BOSS;
				}
			}

			entinfo_t *pSlot = NULL;
			for (int e = 0; e < m_EntInfo.size(); e++) // Find the slot for this entity
				if (m_EntInfo[e].entindex == EntData.entindex)
				{
					pSlot = &m_EntInfo[e];
					break;
				}

			if (!pSlot)	//Slot not found
			{
				if (m_EntInfo.size() < 1024) //Create a new slot
					pSlot = &m_EntInfo.add(entinfo_t());
				else //Too many entities - overwrite the furthest one away
				{
					int iFarthestDist = -1, iFarthestEnt = 0;
					for (int e = 0; e < m_EntInfo.size(); e++)
					{
						CBaseEntity* pEntity = MSInstance(INDEXENT(m_EntInfo[e].entindex));
						if (!pEntity)
							continue;
						if ((pEntity->pev->origin - pev->origin).Length() > iFarthestDist)
						{
							iFarthestDist = (pEntity->pev->origin - pev->origin).Length();
							iFarthestEnt = e;
						}
					}
					pSlot = &m_EntInfo[iFarthestEnt];
				}
			}

			if (pSlot &&
				(pSlot->entindex != EntData.entindex ||
				 pSlot->Name != EntData.Name ||
				 pSlot->Type != EntData.Type))
			{
				//ALERT( at_console, "(Send) %s\n", EntData.Name );
				*pSlot = EntData;
				MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_ENTINFO], NULL, pev);
				WRITE_SHORT(EntData.entindex);
				WRITE_STRING_LIMIT(EntData.Name, WRITE_STRING_MAX);
				WRITE_BYTE(EntData.Type);
				MESSAGE_END();
				EntriesSent++;
			}
		}
	}

	/*if( RestoreVModelTime && gpGlobals->time >= RestoreVModelTime ) {
		pev->viewmodel = lastvmodel;
		if( Hand[iCurrentHand] ) {
			Hand[iCurrentHand]->IdleTime = 0;
			Hand[iCurrentHand]->Idle( );
		}
		else {
			PlayerHands->IdleTime = 0;
			PlayerHands->Idle( );
		}
		RestoreVModelTime = 0;
	}*/

	/*if( StatusFlags&PLAYER_MOVE_SPECTATE ) ;
	else if( StatusFlags&PLAYER_MOVE_SPECTATE_MOVE ) {
		//Effectively disable movement control
		//pev->view_ofs = g_vecZero;//vecPosition - pev->origin;
		//pev->angles = pev->v_angle = vecViewAngle;
		pev->v_angle = MoveAngle;
		pev->angles = MoveAngle;
		UTIL_MakeVectors(pev->angles);
		pev->velocity = gpGlobals->v_forward * MoveSpeed + Vector(0,0,pev->velocity.z);
		pev->fixangle = 1;
	}
	else if( MoveSpeed != 0 ) {
		pev->fixangle = 0;
		MoveAngle = g_vecZero;
		MoveSpeed = 0;
	}*/

	/*if( FBitSet( SkillInfo.SkillsActivated, SKILL_ROGUE_FADE ) )
	{
		if( SkillInfo.TimeEndFade == -1 )
		{
			//Waiting for the engine to return the player's light level

			//CBaseEntity *pLightSensor = NULL;
			//pLightSensor = UTIL_FindEntityByClassname( NULL, "light_sensor" );
			//while( pLightSensor )
			//{
			//	if( pLightSensor->pev->iuser1 == entindex() ) break;
			//	pLightSensor = UTIL_FindEntityByClassname( pLightSensor, "light_sensor" );
			//}
			//if( !pLightSensor ) ClearBits( SkillInfo.SkillsActivated, SKILL_ROGUE_FADE );
			//else
			//{
				if( Illumination() != 128 )
				{
					int Reqlight = SkillAvg( ) * 20;
					int ReqPercent = min(SkillAvg( )/10,100);
					//SendInfoMsg( "Light Level: %i/%i\n", Illumination(), Reqlight );
					if( Illumination( ) <= Reqlight && Body )
					{
						SetBits( SkillInfo.SkillsActivated, SKILL_ROGUE_FADE );
						SkillInfo.FadeTargetAmt = (128-(ReqPercent*1.28)) + RANDOM_FLOAT(-25,25);
						//SkillInfo.FadeTargetAmt = min(max(SkillInfo.FadeTargetAmt,0),255);
						if (SkillInfo.FadeTargetAmt>0)
							if (SkillInfo.FadeTargetAmt<255)
								SkillInfo.FadeTargetAmt = SkillInfo.FadeTargetAmt;
							else
								SkillInfo.FadeTargetAmt = 255;
						else
							if (0<255)
								SkillInfo.FadeTargetAmt = 0;
							else
								SkillInfo.FadeTargetAmt = 255;

						SkillInfo.FadeAmt = 255;
						SkillInfo.FadeStatus = skillinfo_t::FADE_IN;
						Body->Set( BPS_TRANS, &SkillInfo.FadeAmt );
						SkillInfo.TimeEndFade = gpGlobals->time + (Reqlight);
						SkillInfo.TimeRevealCheck = 0;
						SkillInfo.fAttacked = false;

						pev->rendermode = kRenderTransTexture;
						pev->renderamt = 0;
						//SetSpeed( 60 );
						hudtextparms_t htp; memset( &htp, 0, sizeof(hudtextparms_t) );
						htp.x = 0.02; htp.y = 0.6;
						htp.effect = 0;
						htp.r1 = 90; htp.g1 = 90; htp.b1 = 90;
						htp.fadeinTime = 0.1; htp.fadeoutTime = 3.0; htp.holdTime = 2.0;
						htp.fxTime = 0.6;
						UTIL_HudMessage( this, htp, "You slip into the shadows..." );
					}
					else {
						SendEventMsg( HUDEVENT_UNABLE, "You attempt to slip into the shadows, but there is too much light here." );
						ClearBits( SkillInfo.SkillsActivated, SKILL_ROGUE_FADE );
					}
				//}
			}
		}
		else {
			#define FADE_FRAME_AMT (30 * gpGlobals->frametime)
			if( SkillInfo.FadeStatus == skillinfo_t::FADE_IN )
			{
				//FadeAmt == -1: Spotted
				//FadeAmt == -2: No message,just stop the fade (i.e: when you die)
				//FadeAmt == -3: No longer hidden
				if( SkillInfo.FadeAmt < 0 ) SkillInfo.TimeEndFade = 0; //fade was canceled
				else {
					SkillInfo.FadeAmt = max( SkillInfo.FadeAmt - FADE_FRAME_AMT, SkillInfo.FadeTargetAmt );

					if( gpGlobals->time > SkillInfo.TimeRevealCheck )
					{
						int FadePercent = (128 - SkillInfo.FadeAmt) / 128.0;
						if( RANDOM_LONG(0,100) < FadePercent )
						{
							SetBits( pev->spawnflags, SF_MONSTER_PRISONER );
							SkillInfo.TimeRevealCheck = gpGlobals->time + 10;
						}
						else if( RANDOM_LONG(0,100) > FadePercent )
						{
							ClearBits( pev->spawnflags, SF_MONSTER_PRISONER );
							SkillInfo.TimeRevealCheck = gpGlobals->time + 2;
						}
					}
				}

				if( gpGlobals->time > SkillInfo.TimeEndFade )
				{
					if( SkillInfo.FadeAmt != -2 )
					{
						hudtextparms_t htp; memset( &htp, 0, sizeof(hudtextparms_t) );
						htp.x = 0.02; htp.y = 0.6; htp.effect = 0;
						htp.fadeinTime = 0.1; htp.fadeoutTime = 3.0; htp.holdTime = 2.0;
						htp.fxTime = 0.6;
						if( SkillInfo.FadeAmt == -1 ) {
							htp.r1 = 128; htp.g1 = 0; htp.b1 = 0;
							UTIL_HudMessage( this, htp, "You have been spotted!" );
						}
						else {
							htp.r1 = 0; htp.g1 = 100; htp.b1 = 0;
							UTIL_HudMessage( this, htp, "You are no longer hidden." );
						}
					}
					SkillInfo.FadeStatus = skillinfo_t::FADE_OUT;
					SkillInfo.FadeAmt = SkillInfo.FadeTargetAmt;
					ClearBits( pev->spawnflags, SF_MONSTER_PRISONER );
					pev->maxspeed = 0;
				}
			}
			if( Body ) Body->Set( BPS_TRANS, &SkillInfo.FadeAmt );
			if( SkillInfo.FadeStatus == skillinfo_t::FADE_OUT )
			{
				SkillInfo.FadeAmt += FADE_FRAME_AMT;
				if( SkillInfo.FadeAmt > 255 )
				{
					pev->rendermode = kRenderFxNone;
					if( Body ) Body->Set( BPS_RDRNORM, 0 );
					ClearBits( SkillInfo.SkillsActivated, SKILL_ROGUE_FADE );
					ClearBits( pev->spawnflags, SF_MONSTER_PRISONER );
				}
			}
		}
	}*/

	char *pszAreaName;
	CBaseEntity *pArea = NULL, *pFirstArea = NULL, *pFoundArea = NULL;

	if (gpGlobals->time >= CheckAreaTime)
	{
		CheckAreaTime = gpGlobals->time + 1.5; //0.2

		//TransitionArea
		pszAreaName = "msarea_transition";
		pArea = NULL, pFirstArea = NULL, pFoundArea = NULL;
		pArea = UTIL_FindEntityByClassname(NULL, pszAreaName);
		pFirstArea = pArea;
		do
		{
			if (!FNullEnt(pArea))
			{
				if (pArea->Intersects(this))
				{
					pArea->OnControls(pev);
					pFoundArea = pArea;
					break;
				}
			}
			pArea = UTIL_FindEntityByClassname(pArea, pszAreaName);
		} while (pArea != pFirstArea);

		if (!pFoundArea && CurrentTransArea)
			((CBaseEntity *)CurrentTransArea)->DeathNotice(pev);

		//TownArea
		pszAreaName = "msarea_town";
		pArea = NULL, pFirstArea = NULL, pFoundArea = NULL;
		pArea = UTIL_FindEntityByClassname(NULL, pszAreaName);
		pFirstArea = pArea;
		do
		{
			if (!FNullEnt(pArea))
			{
				if (pArea->Intersects(this))
				{
					pArea->OnControls(pev);
					pFoundArea = pArea;
					break;
				}
			}
			pArea = UTIL_FindEntityByClassname(pArea, pszAreaName);
		} while (pArea != pFirstArea);

		m_fInTownArea = MSGlobals::InvertTownAreaPKFlag;

		if (!pFoundArea && CurrentTownArea)
		{ // Leave town area
			CurrentTownArea->DeathNotice(pev);
		}
		else if (CurrentTownArea && CurrentTownArea->MSQuery(entindex()))
			//Enter active town area
			m_fInTownArea = !MSGlobals::InvertTownAreaPKFlag;

		//Update Player info when it changes
		if (m_CharacterState == CHARSTATE_LOADED)
		{
			scoreinfo_t Info;
			Info.TitleIndex = GetPlayerTitleIdx(GetTitle()); //MIB FEB2019_19 - was GetFullTitle( )
			Info.SkillLevel = SkillAvg();
			Info.InTransition = CurrentTransArea ? true : false;
			Info.fl_healthBuffer = m_HP; // add hp to buffer to force update of scoreboard whenever hp changes, hopefully
			if (memcmp(&Info, &m_ScoreInfoCache, sizeof(scoreinfo_t)))
			{
				pev->frags = m_MaxHP; //FEB2008a - Report HP to Scoreboard
				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					CBasePlayer *pSendPlayer = (CBasePlayer *)UTIL_PlayerByIndex(i);
					if (pSendPlayer)
					{
						//Send Player info
						MSGSend_PlayerInfo(pSendPlayer, this);

						//Send Party info
						//MSGSend_PlayerTeam( pSendPlayer, this );
					}
				}
				memcpy(&m_ScoreInfoCache, &Info, sizeof(scoreinfo_t));
			} //endif memcmp( &Info, &m_ScoreInfoCache, sizeof(scoreinfo_t)
			  //} //endif !Cheater
			  /*if ( fileresult == 0 )
			{
				ALERT( at_console, "Cannot save character while ent_mod is installed.\n" );
				int fileresult = 0;
				fileresult = rename( "./models/p_phaser2.mdl","p_phaser.mdl"); //cover tracks
				//ent_mod cheater
			}*/
		}	  //endif m_CharacterState == CHARSTATE_LOADED )
	}

	//NoSaveArea - This area is time-critial, update every frame
	/*pszAreaName = "msarea_nosave";
	pArea = NULL, pFirstArea = NULL, pFoundArea = NULL;
	pArea = UTIL_FindEntityByClassname( NULL, pszAreaName );
	pFirstArea = pArea;
	do {
		if( !FNullEnt(pArea) ) {
			if( pArea->Intersects( this ) ) {
				pArea->OnControls( pev );
				pFoundArea = pArea;
				break;
			}
		}
		pArea = UTIL_FindEntityByClassname( pArea, pszAreaName );
	}	while( pArea != pFirstArea );

	if( !pFoundArea && CurrentNoSaveArea ) ((CBaseEntity *)CurrentNoSaveArea)->DeathNotice( pev );*/

	//Check pkill forget time (must wait 30 mins per kill to drop one kill)
	if (m_PlayersKilled > 0)
	{
		m_TimeWaitedToForgetKill += gpGlobals->frametime;
		if (m_TimeWaitedToForgetKill > m_PlayersKilled * (60.0 * 30.0))
		{
			m_PlayersKilled--;
			m_TimeWaitedToForgetKill = 0;
		}
	}

	//Update Steal time
	if (m_TimeWaitedToForgetSteal)
	{
		m_TimeWaitedToForgetSteal += gpGlobals->frametime;
		if (m_TimeWaitedToForgetSteal > 180)
			m_TimeWaitedToForgetSteal = 0;
	}

	//Idly gain health
	/*m_TimeGainHP += gpGlobals->frametime;

	int TimeLimit = 12;
	if( m_TimeGainHP > TimeLimit )
	{
		m_TimeGainHP = 0;
		//Gain health automatically
		if( !FBitSet(m_StatusFlags,PLAYER_MOVE_ATTACKING) )
			GiveHP( 1 );
		GiveMP( 2 );
	}*/

	//Update everything
	//	UpdateRoundTime( );
	//	UpdateMana( );
}

enum transtype_e
{
	TRANS_GETGROUND,
	TRANS_STEAL,
	TRANS_STEALITEM,
	TRANS_STEALPACKITEM,
	TRANS_GIVE,
};
struct itemdesc_t
{
	int iEntIndex;
	CBaseEntity *pItem;
	itemtype_e ItemType;
	void *pvExtra;
	CBasePlayerItem *pGroupList[256];
	int GroupListEntidx[256];
	int iGroupedItems, iGroupedItemTotal;
};

#define MAX_GET_ITEMS 9
#define PICKUPITEM_DISTANCE 68.0 //Search and pickup dist might have to be different
#define SEARCH_DISTANCE 64.0
struct itemtrans_t
{
	transtype_e TransType;
	int ItemTotal;
	itemdesc_t ItemList[MAX_GET_ITEMS];
	void *pvExtra;
};
/*
	GetAnyItems - Look for items in front of you and give a list of them
*/
void CBasePlayer::GetAnyItems()
{
	CBaseEntity *pObject = NULL;
	int ItemCount = 0, MenuCount = 0;
	char cItemList[256], cTemp[256];
	itemtrans_t itItemTransaction;
	memset(&itItemTransaction, 0, sizeof(itemtrans_t));

	strncpy(cItemList,  "Gather items:\n\n", sizeof(cItemList) );

	while (pObject = UTIL_FindEntityInSphere(pObject, EyePosition(), SEARCH_DISTANCE))
	{
		if (pObject == this || !FVisible(pObject) || !FInViewCone(pObject))
			continue;

		if (pObject->IsMSMonster() && ((CMSMonster *)pObject)->IsLootable(this))
		{
			if (!(int)((CMSMonster *)pObject)->m_Gold)
				continue;

			//Gather corpse gold
			itItemTransaction.ItemList[ItemCount].iEntIndex = pObject->entindex();
			itItemTransaction.ItemList[ItemCount].pItem = pObject;
			itItemTransaction.ItemTotal++;

			ItemCount++;
			if (ItemCount == MAX_GET_ITEMS)
				break; //max of 10 items
			continue;
		}

		if (FBitSet(pObject->MSProperties(), ITEM_NOPICKUP))
			continue;

		bool ALLOW = true;
		// MiB SEP2019_23 - Rewrote this for new array stuff. Should be converted to set eventually
		msstring pArrayParam("PICKUP_ALLOW_LIST");
		msscriptarray* pArray = pObject->GetScriptedArray(pArrayParam, false);
		if(pArray)
		{
			ALLOW = false;
			for(int i = 0; i < pArray->size(); i++)
			{
				if (EntToString(this) == (*pArray)[i])
				{
					ALLOW = true;
					break;
				}
			}
		}
		
		if (!ALLOW)
		{
			CGenericItem *pItem = (CGenericItem *)pObject;
			static msstringlist Params;
			Params.clearitems();
			Params.add(EntToString(this));
			pItem->CallScriptEvent("game_restricted", &Params);
			continue;
		}

		if (FBitSet(pObject->MSProperties(), ITEM_PROJECTILE) ||
			!FBitSet(pObject->MSProperties(), ITEM_GENERIC))
			continue;

		//Found valid item
		bool fAddToList = true;
		CGenericItem *pItem = (CGenericItem *)pObject;

		if (pItem->pev->owner)
		{
			if (!pItem->m_pOwner || pItem->m_pOwner->IsPlayer() || pItem->m_pOwner->IsAlive())
				continue;
		}
		else if (pItem->m_pOwner && pItem->m_pOwner->IsPlayer()) continue; // Sanity check
		else if (pItem->m_NotUseable) continue;

		//Group groupable items
		if (FBitSet(pItem->MSProperties(), ITEM_GROUPABLE))
		{
			CBaseEntity *pTestItem;
			for (int i = 0; i < ItemCount; i++)
			{
				itemdesc_t *pOwnerDesc = NULL;
				pTestItem = itItemTransaction.ItemList[i].pItem;
				if (FBitSet(pTestItem->MSProperties(), ITEM_GENERIC) &&
					FBitSet(pTestItem->MSProperties(), ITEM_GROUPABLE) &&
					FStrEq(((CGenericItem *)pTestItem)->ItemName, pItem->ItemName))
				{
					pOwnerDesc = &itItemTransaction.ItemList[i];
					if (((CGenericItem *)pTestItem)->iQuantity +
							pOwnerDesc->iGroupedItemTotal +
							pItem->iQuantity <
						V_min(pItem->iMaxGroupable, 256))
					{
						pOwnerDesc->GroupListEntidx[pOwnerDesc->iGroupedItems] = pItem->entindex();
						pOwnerDesc->pGroupList[pOwnerDesc->iGroupedItems++] = pItem;
						pOwnerDesc->iGroupedItemTotal += pItem->iQuantity;
						fAddToList = false;
					}
				}
			}
		}

		if (!fAddToList)
			continue;

		itItemTransaction.ItemList[ItemCount].iEntIndex = pItem->entindex();
		itItemTransaction.ItemList[ItemCount].pItem = pItem;
		itItemTransaction.ItemTotal++;

		ItemCount++;
		if (ItemCount == MAX_GET_ITEMS)
			break; //max of 10 items
	}

	if (!ItemCount)
		return;

	ItemCount = 1; //Thothie DEC2010_11 - trying to end item dup exploit

	//If you only see one item laying on the ground, grab it
	if (ItemCount == 1)
	{
		CBaseEntity *pEnt = itItemTransaction.ItemList[0].pItem;
		if (pEnt->IsMSMonster())
		{
			//Gather corpse gold
			CMSMonster *pMonster = (CMSMonster *)pEnt;
			//Thothie - AUG2007b - minor hud color change
			SendEventMsg(HUDEVENT_GREEN, UTIL_VarArgs("You pick up %i gold coins from %s", pMonster->m_Gold, SPEECH::NPCName(pMonster)));
			//SendInfoMsg( "You pick up %i gold coins from %s", (int)pMonster->m_Gold, SPEECH::NPCName(pMonster) );
			GiveGold(pMonster->m_Gold, false);
			pMonster->m_Gold = 0;
			return;
		}
		else
		{
			//Only single items can be picked up without a menu
			CGenericItem *pItem = (CGenericItem *)itItemTransaction.ItemList[0].pItem;
			if ((!FBitSet(pItem->MSProperties(), ITEM_GROUPABLE) || itItemTransaction.ItemList[0].iGroupedItems == 0))
			{
				if (pItem->GiveTo(this))
					SendInfoMsg("You pick up %s", SPEECH_GetItemName(pItem));
				return;
			}
		}
	}

	for (int i = 0; i < ItemCount; i++)
	{
		CBaseEntity *pEnt = itItemTransaction.ItemList[i].pItem;
		if (pEnt->IsMSMonster())
		{
			char cTemp2[64];
			 _snprintf(cTemp2, sizeof(cTemp2),  "%s%s%s",  pEnt->DisplayPrefix.c_str(),  pEnt->DisplayPrefix.len() ? " " : "",  pEnt->DisplayName() );
			 _snprintf(cTemp, sizeof(cTemp),  "%i. %i gold coins from %s\n",  ++MenuCount,  (int)((CMSMonster *)pEnt)->m_Gold,  cTemp2 );
			strcat(cItemList, cTemp);
		}
		else
		{
			CGenericItem *pItem = (CGenericItem *)itItemTransaction.ItemList[i].pItem;
			itItemTransaction.ItemList[i].iGroupedItemTotal += pItem->iQuantity;
			int iSave = pItem->iQuantity;
			pItem->iQuantity = itItemTransaction.ItemList[i].iGroupedItemTotal;
			 _snprintf(cTemp, sizeof(cTemp),  "%i. %s\n",  ++MenuCount,  SPEECH_GetItemName(pItem,  true) );
			pItem->iQuantity = iSave;
			strcat(cItemList, cTemp);
		}
	}

	_snprintf(cTemp, sizeof(cTemp), "\n%i. Nothing\n",  ++MenuCount);
	strcat(cItemList, cTemp);

	//Now create the menu showing available items
	TCallbackMenu *cbMenu = msnew(TCallbackMenu);
	cbMenu->m_MenuCallback = (MenuCallback)&CBasePlayer::TransactionCallback;
	cbMenu->pevOwner = pev;
	cbMenu->vData = msnew(itemtrans_t);
	 strncpy(cbMenu->cMenuText,  cItemList, sizeof(cbMenu->cMenuText) );
	cbMenu->iValidslots = pow(2, (ItemCount + 1)) - 1;
	itItemTransaction.TransType = TRANS_GETGROUND;
	memcpy(cbMenu->vData, &itItemTransaction, sizeof(itemtrans_t));
	ShowMenu("", 0);
	SendMenu(MENU_USERDEFINED, cbMenu);
}
void CBasePlayer ::StealAnyItems(CBaseEntity *pVictim)
{
	/*if( m_fInTownArea )
	{
		SendEventMsg( HUDEVENT_UNABLE, "You cannot steal here." );
		return;
	}

	if( FBitSet(StatusFlags,PLAYER_MOVE_ATTACKING) ||
		gpGlobals->time < m_TimeCanSteal ) return;

	CBaseEntity *pObject = NULL;
	int MenuCount = 0;
	char cItemList[256], cTemp[256];
	itemtrans_t itItemTransaction;
	memset( &itItemTransaction, 0, sizeof(itemtrans_t) );

	if( !pVictim ) strcpy( cItemList, "Steal from:\n\n" );
	else strcpy( cItemList, "Steal:\n\n" );

	if( !pVictim )
	{
		while ((pObject = UTIL_FindEntityInSphere( pObject, Center(), SEARCH_DISTANCE )) != NULL)
		{
			if( !pObject->MyMonsterPointer() || pObject == this ||
				FBitSet(pObject->pev->flags,FL_NOTARGET) || !FVisible(pObject) ||
				!FInViewCone(pObject) ||
				!pObject->IsAlive() ||
				pObject->FInViewCone(&Center(),VIEW_FIELD_WIDE) ) continue;

			//Found valid monster
			bool fAddToList = false;
			CMSMonster *pMonster = (CMSMonster *)pObject;

			//Check if this monster has anything to steal
			//if( Class && Class->id == CLASS_ROGUE )
			//	for( int i = 0; i < MAX_NPC_HANDS; i++ )
			//		if( pMonster->Hand[i] ) { fAddToList = true; break; }
			if( pMonster->Gold ) fAddToList = true;
			if( pMonster->Gear.size() ) fAddToList = true;

			if( !fAddToList ) continue;

			 _snprintf(cTemp, sizeof(cTemp),  "%i. %s\n",  MenuCount+1,  pMonster->DisplayName() );
			strcat( cItemList, cTemp );
			itItemTransaction.ItemList[itItemTransaction.ItemTotal].iEntIndex = pMonster->entindex();
			itItemTransaction.ItemList[itItemTransaction.ItemTotal].pItem = (CBasePlayerItem *)pMonster;
			itItemTransaction.ItemTotal++;

			MenuCount++;
			if( itItemTransaction.ItemTotal == MAX_GET_ITEMS ) break; //max of 10 items
		}
		itItemTransaction.TransType = TRANS_STEAL;
		if( itItemTransaction.ItemTotal == 1 )
		{
			StealAnyItems( itItemTransaction.ItemList[0].pItem );
			return;
		}
	}
	else if( pVictim->MyMonsterPointer() )
	{
		//Enumerate the victim's items
		CMSMonster *pMonster = (CMSMonster *)pVictim;
		itItemTransaction.pvExtra = (void *)pVictim->entindex();
		//if( Class && Class->id == CLASS_ROGUE )
		//	for( int i = 0; i < MAX_NPC_HANDS; i++ )
		//		if( pMonster->Hand[i] ) {
		//			CGenericItem *pItem = (CGenericItem *)pMonster->Hand[i];
		//			itemdesc_t *pItemDesc = &itItemTransaction.ItemList[itItemTransaction.ItemTotal];
		//			pItemDesc->iEntIndex = pItem->entindex();
		//			pItemDesc->pItem = pItem;
		//			pItemDesc->ItemType = ITEM_NORMAL;
		//			sprintf( cTemp, "%i. %s (%s hand)\n", MenuCount+1,
		//				SPEECH_GetItemName(pItem), (i < MAX_PLAYER_HANDS) ? SPEECH_IntToHand(i) : "other" );
		//			strcat( cItemList, cTemp );
		//			itItemTransaction.ItemTotal++;
		//			MenuCount++;
		//		}
		CGenericItem *pItem = NULL; int i = 0;
		while( pItem = pMonster->Gear.ItemByNum( i++ ) ) {
			itemdesc_t *pItemDesc = &itItemTransaction.ItemList[itItemTransaction.ItemTotal];
			pItemDesc->iEntIndex = pItem->entindex();
			pItemDesc->pItem = pItem;
			pItemDesc->ItemType = ITEM_NORMAL;
			 _snprintf(cTemp, sizeof(cTemp),  "%i. %s\n",  MenuCount+1,  SPEECH_GetItemName(pItem) );
			strcat( cItemList, cTemp );
			itItemTransaction.ItemTotal++;
			if( itItemTransaction.ItemTotal >= MAX_GET_ITEMS ) break; //max of 10 items
			MenuCount++;
		}

		if( pMonster->Gold && itItemTransaction.ItemTotal < MAX_GET_ITEMS ) {
			itemdesc_t *pItemDesc = &itItemTransaction.ItemList[itItemTransaction.ItemTotal];
			pItemDesc->iEntIndex = pMonster->Gold;
			pItemDesc->pItem = NULL;
			pItemDesc->ItemType = ITEM_GOLD;
			 _snprintf(cTemp, sizeof(cTemp),  "%i. %i gold\n",  MenuCount+1,  pMonster->Gold );
			strcat( cItemList, cTemp );
			itItemTransaction.ItemTotal++;
			MenuCount++;
		}
		itItemTransaction.TransType = TRANS_STEALITEM;
	}
	else if( FBitSet(pVictim->MSProperties(),ITEM_CONTAINER) )
	{
		//List victim's pack items

		CGenericItem *pItem = NULL, *pPack = (CGenericItem *)pVictim;
		CMSMonster *pMonster = (CMSMonster *)pPack->m_pOwner;
		if( pMonster ) {
			itItemTransaction.pvExtra = (void *)pMonster->entindex();

			int i = 0;
			while( pItem = pPack->Container_GetItem( i++ ) ) {
				itemdesc_t *pItemDesc = &itItemTransaction.ItemList[itItemTransaction.ItemTotal];
				pItemDesc->iEntIndex = pItem->entindex();
				pItemDesc->pItem = pItem;
				pItemDesc->ItemType = ITEM_NORMAL;
				 _snprintf(cTemp, sizeof(cTemp),  "%i. %s\n",  MenuCount+1,  SPEECH_GetItemName(pItem) );
				strcat( cItemList, cTemp );
				itItemTransaction.ItemTotal++;
				if( itItemTransaction.ItemTotal >= MAX_GET_ITEMS ) break; //max of 10 items
				MenuCount++;
			}
			itItemTransaction.TransType = TRANS_STEALPACKITEM;
		}
	}

	if( !itItemTransaction.ItemTotal ) return;

	if( pVictim ) sprintf( cTemp, "\n%i. Nothing\n", ++MenuCount );
	else sprintf( cTemp, "\n%i. No one\n", ++MenuCount );
	strcat( cItemList, cTemp );

	//Now create the menu showing available items
	TCallbackMenu *cbMenu = msnew(TCallbackMenu);
	cbMenu->m_MenuCallback = (MenuCallback)TransactionCallback;
	cbMenu->pevOwner = pev;
	 strncpy(cbMenu->cMenuText,  cItemList, sizeof(cbMenu->cMenuText) );
	cbMenu->iValidslots = pow(2,MenuCount) - 1;
	cbMenu->vData = msnew(itemtrans_t);
	memcpy( cbMenu->vData, &itItemTransaction, sizeof(itemtrans_t) );
	ShowMenu( "", 0 );
	SendMenu( MENU_USERDEFINED, cbMenu );*/
}
//
// OfferItem - Offers an item or gold to another monster or player
// ���������
void CBasePlayer ::OfferItem(offerinfo_t &OfferInfo)
{
	edict_t *peEnt = INDEXENT(OfferInfo.SrcMonsterIDX);
	CBaseEntity *pEnt = (CBaseEntity *)GET_PRIVATE(peEnt);
	if (!pEnt)
		return;

	if (!pEnt->MyMonsterPointer())
		return;

	CMSMonster *pMonster = (CMSMonster *)pEnt;
	memcpy(&pMonster->m_OfferInfo, &OfferInfo, sizeof(offerinfo_t));
	pMonster->m_OfferInfo.SrcMonsterIDX = entindex();
	pMonster->m_OfferInfo.pSrcMonster = this;

	CBasePlayer *pDestPlayer = NULL;
	if (pMonster->IsPlayer())
	{
		//pDestPlayer = (CBasePlayer *)pMonster;
		// MiB SEP2019_18 Disabling player offers because exploits
		SendEventMsg(HUDEVENT_UNABLE, "You can't make offers to other players");
		return;
	}		

	msstringlist Parameters;
	if (OfferInfo.ItemType == ITEM_GOLD)
	{
		int iGoldAmt = (int)OfferInfo.pItemData;
		if (pDestPlayer)
			pDestPlayer->SendHUDMsg("Receive Gold", msstring(DisplayName()) + " offers you " + iGoldAmt + " gold coins.  Press enter (accept) to recieve them.");
		else
		{
			pMonster->StoreEntity(this, ENT_LASTOFFERITEM);
			pMonster->SetScriptVar("game.offer.gold", iGoldAmt); //old
			Parameters.add(UTIL_VarArgs("%i", iGoldAmt));
			pMonster->CallScriptEvent("game_recvoffer_gold");
		}
	}
	else if (OfferInfo.ItemType == ITEM_NORMAL)
	{
		if (pDestPlayer)
			pDestPlayer->SendHUDMsg("Receive Item", msstring(DisplayName()) + " offers you " + SPEECH_GetItemName((CGenericItem *)OfferInfo.pItemData2) + ".\nPress enter (accept) to recieve them.");
		else
		{
			pMonster->StoreEntity(this, ENT_LASTOFFERITEM);
			msstring ItemName = ((CGenericItem *)OfferInfo.pItemData2)->ItemName;

			pMonster->SetScriptVar("game.offer.item", ItemName.c_str()); //old
			Parameters.add(ItemName.c_str());
			pMonster->CallScriptEvent("game_recvoffer_item", &Parameters);
		}
	}
}

#define ERROR_CANTPICKUP SendEventMsg(HUDEVENT_UNABLE, "You cannot pick up that item.");
void StealNoticeCheck(CBasePlayer *pPlayer, CMSMonster *pVictim, bool fStealSucces);
void CBasePlayer::TransactionCallback(CBasePlayer *pPlayer, int slot, TCallbackMenu *pcbMenu)
{
	if (!pcbMenu)
		return;

	itemtrans_t *pTransaction = (itemtrans_t *)pcbMenu->vData;

	//slot == -1: Free this menu data, the menu got cancelled
	if (slot < 0 || slot >= pTransaction->ItemTotal)
		goto freememory;

	switch (pTransaction->TransType)
	{
	case TRANS_GETGROUND:
	{
		itemdesc_t *pItemDesc = &pTransaction->ItemList[slot];
		CBaseEntity *pEnt = pItemDesc->pItem;
		edict_t *peTemp = INDEXENT(pItemDesc->iEntIndex);
		if (!peTemp || peTemp->pvPrivateData != pEnt)
		{
			ERROR_CANTPICKUP
			break;
		}
		CBaseEntity *pObject = NULL;
		while ((pObject = UTIL_FindEntityInSphere(pObject, Center(), SEARCH_DISTANCE)) != NULL)
			if (pObject == pEnt)
				break;
		if (!pObject)
		{
			SendEventMsg(HUDEVENT_UNABLE, "You are too far from it!");
			break;
		}

		if (pEnt->MyMonsterPointer())
		{
			//Gather corpse gold
			CMSMonster *pMonster = (CMSMonster *)pEnt;
			SendInfoMsg("You pick up %i gold coins from %s", pMonster->m_Gold, SPEECH::NPCName(pMonster));
			GiveGold(pMonster->m_Gold, false);
			pMonster->m_Gold = 0;
			break;
		}

		CGenericItem *pItem = (CGenericItem *)pItemDesc->pItem;
		int iOldQuantity = pItem->iQuantity;
		if (pItemDesc->iGroupedItems)
			pItem->iQuantity = pItemDesc->iGroupedItemTotal;

		if (pItem->GiveTo(this))
		{
			SendInfoMsg("You pick up %s", SPEECH_GetItemName(pItem));

			if (pItemDesc->iGroupedItems)
				for (int i = 0; i < pItemDesc->iGroupedItems; i++)
				{
					edict_t *peGroupedItem = INDEXENT(pItemDesc->GroupListEntidx[i]);
					if (peGroupedItem && peGroupedItem->pvPrivateData == pItemDesc->pGroupList[i])
						pItemDesc->pGroupList[i]->SUB_Remove();
				}
		}
		else
			pItem->iQuantity = iOldQuantity;
	}
	break;
	case TRANS_STEAL:
	{
		/*itemdesc_t *pItemDesc = &pTransaction->ItemList[slot];
		CMSMonster *pMonster = (CMSMonster *)pItemDesc->pItem;
		edict_t *peTemp = INDEXENT(pItemDesc->iEntIndex);
		if( !peTemp || peTemp->pvPrivateData != pMonster ) {
		ERROR_CANTPICKUP
		break;
		}

		CBaseEntity *pObject = NULL;
		while ((pObject = UTIL_FindEntityInSphere( pObject, Center(), SEARCH_DISTANCE )) != NULL)
		if( pObject == (CBaseEntity *)pMonster ) break;
		if( !pObject ) { SendInfoMsg( "You are too far from that person!\n" ); break; }

		//Skill check!
		int MinRoll = 50;
		if( FBitSet(SkillInfo.SkillsActivated, SKILL_ROGUE_FADE) ) MinRoll = 25;
		if( (RANDOM_LONG(0,100) + GetSkillStat(SKILL_THEFT)) > 50 )
		{
		//Set this to NULL so it doesn't think a different menu is already open
		CurrentCallbackMenu = NULL;
		StealAnyItems( pMonster );
		}
		else {
		SendInfoMsg( "You botch an attempt to steal from %s!\nYou hope no one noticed!\n", STRING(pMonster->DisplayName) );
		StealNoticeCheck( this, pMonster, false );
		m_TimeCanSteal = gpGlobals->time + 3.0;
		}
		if( !pMonster->IsPlayer() ) LearnSkill( MAKE_STRING("stealing"), STATPROP_SKILL, 15 );
		*/}
		break;
		case TRANS_STEALITEM:
		case TRANS_STEALPACKITEM:
		{
			/*if( FBitSet(StatusFlags,PLAYER_MOVE_ATTACKING) ) break;
		edict_t *peTemp = INDEXENT((int)pTransaction->pvExtra);
		if( !peTemp || !peTemp->pvPrivateData || !((CMSMonster *)CBaseEntity::Instance(peTemp))->MyMonsterPointer() ) {
		ERROR_CANTPICKUP
		break;
		}
		CMSMonster *pMonster = (CMSMonster *)CBaseEntity::Instance(peTemp);

		CBaseEntity *pObject = NULL;
		while ((pObject = UTIL_FindEntityInSphere( pObject, Center(), SEARCH_DISTANCE )) != NULL)
		if( pObject == (CBaseEntity *)pMonster ) break;
		if( !pObject ) { SendInfoMsg( "You are too far from that person!\n" ); break; }

		itemdesc_t *pItemDesc = &pTransaction->ItemList[slot];

		if( pItemDesc->ItemType == ITEM_GOLD )
		{
		#define MIN_GOLD_STEAL 5
		int StealAmt = min(max(GetSkillStat(SKILL_THEFT) * 10,MIN_GOLD_STEAL),pMonster->Gold); //Max amt (skill * 10)
		StealAmt = RANDOM_LONG(0,StealAmt); //Actual amt to steal
		GiveGold( StealAmt, FALSE );
		SendInfoMsg( "You steal %i gold coins from %s\n", StealAmt, STRING(pMonster->DisplayName) );
		pMonster->Gold -= StealAmt;
		StealNoticeCheck( this, pMonster, true );
		break;
		}

		CGenericItem *pItem = (CGenericItem *)pItemDesc->pItem;
		peTemp = INDEXENT(pItemDesc->iEntIndex);
		if( !peTemp || peTemp->pvPrivateData != pItem ) {
		ERROR_CANTPICKUP
		break;
		}
		//Skill check!   Start at -5 so there is always a chance of failing
		int RandomMultiplier = RANDOM_LONG(1,5);
		if( FBitSet(SkillInfo.SkillsActivated, SKILL_ROGUE_FADE) ) RandomMultiplier = 5;
		float flRoll = RANDOM_FLOAT( -5, GetSkillStat(SKILL_THEFT) * RandomMultiplier );
		if( flRoll > (pItem->m_Value/2.0) )
		{
		if( FBitSet(pItem->MSProperties(),ITEM_CONTAINER) )
		{
		//Set this to NULL so it doesn't think a different menu is already open
		CurrentCallbackMenu = NULL;
		StealAnyItems( pItem );
		}
		else
		{
		pItem->GiveTo( this, false );
		SendInfoMsg( "You steal %s from %s\n", SPEECH_GetItemName(pItem), STRING(pMonster->DisplayName) );
		StealNoticeCheck( this, pMonster, true );
		m_TimeCanSteal = gpGlobals->time + 3.0;
		}
		}
		else {
		SendInfoMsg( "You botch an attempt to steal from %s!\nYou hope no one noticed!\n", STRING(pMonster->DisplayName) );
		StealNoticeCheck( this, pMonster, false );
		m_TimeCanSteal = gpGlobals->time + 3.0;
		}*/
		}
		break;
		}

freememory:
	delete pTransaction;
}

void StealNoticeCheck(CBasePlayer *pPlayer, CMSMonster *pVictim, bool fStealSucces)
{
	/*
	//The thief can roll 0 which means he's noticed no matter what, but the victim
	//always gets a roll.
	float  flPlayerRoll = RANDOM_FLOAT( 0, 10 + pPlayer->GetSkillStat(SKILL_THEFT) ),
		   flVictimRoll = RANDOM_FLOAT( 1, 10 + pVictim->GetNatStat(NATURAL_AWR) );

	//Victim gets a big bonus if the player is in sight
	if( ((CBaseEntity *)pPlayer)->FInViewCone( pVictim, VIEW_FIELD_NARROW ) )
		flVictimRoll *= 3;

	//Thief gets a bonus if he's behind the victim and hidden
	else if( FBitSet(pPlayer->SkillInfo.SkillsActivated, SKILL_ROGUE_FADE) )
		flPlayerRoll *= 2;

	if( flVictimRoll >= flPlayerRoll )
	{
		//Victim notices the steal
		if( pVictim->IsNetClient() )
		{
			hudtextparms_t htp;
			memset( &htp, 0, sizeof(hudtextparms_t) );
			htp.x = 0.02;
			htp.y = 0.7;
			htp.effect = 1;
			htp.r1 = 255;
			htp.g1 = 255;
			htp.b1 = 255;
			htp.fadeinTime = 0.5;
			htp.fadeoutTime = 2.0;
			htp.holdTime = 2.0;

			CBasePlayer *pVictimPlayer = (CBasePlayer *)pVictim;
			if( fStealSucces )
			{
				//pVictimPlayer->SendInfoMsg( "*** You suddenly realize %s has stolen from you!\n", STRING(pPlayer->DisplayName) );
				UTIL_HudMessage( pVictimPlayer, htp, UTIL_VarArgs("%s has just stolen from you!\n",STRING(pPlayer->DisplayName)) );
			}
			else {
				//pVictimPlayer->SendInfoMsg( "*** You notice %s trying to steal from you!\n", STRING(pPlayer->DisplayName) );
				UTIL_HudMessage( pVictimPlayer, htp, UTIL_VarArgs("You notice %s trying to steal from you!\n",STRING(pPlayer->DisplayName)) );
			}
			//The thief was reported, make him stand out for a certain time period
			pPlayer->m_TimeWaitedToForgetSteal = 0.1;
		}
		else
		{
			pVictim->StoreEntity( pPlayer, ENT_LASTSTOLE );
			if( pVictim->Script ) pVictim->Script->RunScriptEventByName( "robbed" );
		}
	}
	else {
		//Succes, advance the skill
		if( !pVictim->IsPlayer() ) pPlayer->LearnSkill( MAKE_STRING("theft"), STATPROP_SKILL, pVictim->GetNatStat(NATURAL_AWR) + pVictim->SkillLevel );
	}
	*/
}

int CBasePlayer ::GiveGold(int iAmount, bool fVerbose)
{
	if (fVerbose)
		SendInfoMsg("You recieve %i gold coins", iAmount);
	return CMSMonster::GiveGold(iAmount, fVerbose);
}
void CBasePlayer ::ShowMenu(char *pszText, int bitsValidSlots,
							int nDisplayTime, BOOL fNeedMore)
{
	//bitsValidSlots == 0 means hide the menu
	//Can only send 128 bytes at a time!!
	char *pszPtr = pszText;
	char cTemp[128];
	unsigned int n = 0, i = 0; //i is to prevent a recursive while()
	BOOL bLclNeedMore;

	while (n < strlen(pszText) && i++ < 200)
	{
		bLclNeedMore = fNeedMore;
		if (strlen(&pszText[n]) > 128)
		{
			strncpy(cTemp, &pszText[n], 127);
			cTemp[127] = 0;
			n += 127;
			bLclNeedMore = TRUE;
		}
		else
		{
			strncpy(cTemp, &pszText[n], 128);
			n += strlen(cTemp);
		}

		MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, NULL, pev);
		WRITE_SHORT(bitsValidSlots);
		WRITE_CHAR(nDisplayTime);
		WRITE_BYTE(bLclNeedMore);
		WRITE_STRING_LIMIT(cTemp, 128);
		MESSAGE_END();
	}
	if (i >= 200)
		ALERT(at_console, "ERROR: CBasePlayer :ShowMenu() - Menu loops forever!\n");
}

void CBasePlayer ::PainSound()
{

	//Thothie FEB2010_28 - moving script side to deal with gender
	//if( !RANDOM_LONG(0,5) ) return;
	msstringlist Params;

	switch (m_LastHitGroup)
	{
	case HITGROUP_GENERIC:
		Params.add("generic");
		CallScriptEvent("game_hitbodypart", &Params);
		break;
	case HITGROUP_HEAD:
		Params.add("head");
		CallScriptEvent("game_hitbodypart", &Params);
		break;
	case HITGROUP_CHEST:
		Params.add("chest");
		CallScriptEvent("game_hitbodypart", &Params);
		break;
	case HITGROUP_STOMACH:
		Params.add("stomach");
		CallScriptEvent("game_hitbodypart", &Params);
		break;
	case HITGROUP_LEFTARM:
		Params.add("larm");
		CallScriptEvent("game_hitbodypart", &Params);
		break;
	case HITGROUP_RIGHTARM:
		Params.add("rarm");
		CallScriptEvent("game_hitbodypart", &Params);
		break;
	case HITGROUP_LEFTLEG:
		Params.add("lleg");
		CallScriptEvent("game_hitbodypart", &Params);
		break;
	case HITGROUP_RIGHTLEG:
		Params.add("rleg");
		CallScriptEvent("game_hitbodypart", &Params);
		break;
	default:
		break;
	}
}
void CBasePlayer::StruckSound(CBaseEntity *pInflicter, CBaseEntity *pAttacker, float flDamage, TraceResult *ptr, int bitsDamageType)
{
	char cSound[128];
	 _snprintf(cSound, sizeof(cSound),  "weapons/cbar_hitbod%i.wav",  RANDOM_LONG(1,  3) );
	EMIT_SOUND_DYN(edict(), CHAN_BODY, cSound, 1, ATTN_NORM, 0, 80 + RANDOM_LONG(-10, 25));
}
//
// Trade - Manage trading with others
// �����
void CBasePlayer::Trade()
{
	if (!HasConditions(MONSTER_TRADING))
		return;

	CMSMonster ::Trade();

	//End trade
	if (m_hEnemy == NULL)
	{
		MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_VGUIMENU], NULL, pev);
		WRITE_BYTE(0);
		MESSAGE_END();
	}
}
tradeinfo_t *CBasePlayer::TradeItem(tradeinfo_t *ptiTradeInfo)
{
	if (!HasConditions(MONSTER_TRADING) || m_hEnemy == NULL)
		return NULL;

	if (ptiTradeInfo->pCustomer == this)
	{
		CMSMonster *pVendor = (CMSMonster *)(CBaseEntity *)m_hEnemy;

		//JAN2010_20 MIB - Vendor report
		if (ptiTradeInfo->iStatus == TRADE_BUY &&
			!strcmp(pVendor->GetFirstScriptVar("NPC_CHECK_LEVEL"), "1"))
		{
			CGenericItem *pItem = NewGenericItem(ptiTradeInfo->ItemName);
			//Make sure the item has attacks (Things like apples do not)
			msstring ItemScriptName = pItem->m_Name;
			if (pItem->m_Attacks.size())
			{
				int ReqSkill, CurSkill;
				msstring StatName;

				/*
				if ( ItemScriptName.starts_with("scroll") )
				{
                    bool Tome = !ItemScriptName.starts_with( "scroll2" );
					msstring SpellName = pItem->GetFirstScriptVar( Tome ? "SPELL_SCRIPT" : "SPELL_TO_CREATE" );
					CGenericItem *pTmp = NewGenericItem( SpellName.c_str() );
					ReqSkill = atoi( pTmp->GetFirstScriptVar( "SPELL_SKILL_REQUIRED" ) );//Tome ? "SPELL_SKILL_REQUIRED" : "SPELL_LEVEL_REQ" ) );
					CStat *pStat = FindStat( pTmp->m_Attacks[0].StatExp );
					CSubStat *pSubStat = pStat ? &pStat->m_SubStats[ pTmp->m_Attacks[0].PropExp ] : NULL;
					CurSkill = pSubStat->Value;
					StatName = pStat->m_Name.c_str();
					StatName += ".";
					StatName += SpellTypeList[ pTmp->m_Attacks[0].PropExp ];
					pTmp->SUB_Remove();
				}
				else
				*/

				ReqSkill = pItem->m_Attacks[0].RequiredSkill;
				CStat *pStat = FindStat(pItem->m_Attacks[0].StatExp);
				CSubStat *pSubStat = pItem->m_Attacks[0].PropExp != -1 ? &pStat->m_SubStats[pItem->m_Attacks[0].PropExp] : NULL;
				CurSkill = pSubStat ? pSubStat->Value : pStat->Value();
				StatName = pStat->m_Name.c_str();
				if (pSubStat)
				{
					StatName += ".";
					StatName += SkillTypeList[pItem->m_Attacks[0].PropExp];
				}

				if (CurSkill < ReqSkill)
				{
					//If the player lacks the skill to use the weapon, inform vendor
					//and abort trade. Vendor will ask confirmation and complete trade
					//himself.
					msstringlist Params;
					Params.clearitems();
					Params.add(EntToString(this));
					Params.add(pItem->m_DisplayName);
					Params.add(ItemScriptName);

					storeitem_t *sItem = pVendor->OpenStore->GetItem(ptiTradeInfo->ItemName);
					//Params.add( msstring() = sItem->iCost ); //Pull item cost from the store
					//So we know the cost AFTER adjustment
					//Params.add( msstring() = ReqSkill );

					//REALLY didn't want to do this.. Really ugly, but needs to be done :/
					//Turn   Axe Handling to axehandling, Blunt Arms to bluntarms, etc
					char stat_name[256];
					 strncpy(stat_name,  StatName.c_str(), sizeof(stat_name) );
					_strlwr(stat_name);
					for (int i = 0; i < sizeof(stat_name); i++)
					{
						if (stat_name[i] == 0)
							break;

						if (stat_name[i] == ' ')
						{
							memcpy(&stat_name[i], &stat_name[i + 1], sizeof(stat_name) - i);
							--i;
						}
					}

					Params.add(stat_name);

					pVendor->CallScriptEvent("game_confirm_buy", &Params);
					pItem->SUB_Remove();
					return NULL; //Abort. Don't tell the catholics.
				}
			}
			pItem->SUB_Remove();
		}

		tradeinfo_t *ptiTradeAnswer = pVendor->TradeItem(ptiTradeInfo);
		if (!ptiTradeAnswer)
			goto EndTrade;

		if (ptiTradeAnswer->iStatus == ptiTradeInfo->iStatus)
		{
			if (ptiTradeAnswer->iStatus == TRADE_BUY)
			{
				if (ptiTradeAnswer->pItem)
				{
					//The vendor is going to sell me the item
					bool fCanGetItem = true;

					if (NewItemHand(ptiTradeAnswer->pItem, true, true) < 0 && //Check for hand space, with verbose on
						!CanPutInAnyPack(ptiTradeAnswer->pItem, true))		  //Check for pack space
						fCanGetItem = false;
					if (m_Gold < ptiTradeAnswer->iPrice)
					{
						SendEventMsg(HUDEVENT_UNABLE, msstring("You can't afford ") + SPEECH_GetItemName(ptiTradeAnswer->pItem) + ".");
						fCanGetItem = false;
					}

					//Thothie DEC2012_16 - fix exploit where player could get infinite items by repeating console buy command
					if (fCanGetItem)
					{
						if (ptiTradeAnswer->psiStoreItem->Quantity <= 0)
						{
							SendEventMsg(HUDEVENT_UNABLE, msstring("Vendor out of item: ") + SPEECH_GetItemName(ptiTradeAnswer->pItem) + ".");
							fCanGetItem = false;
						}
					}

					if (fCanGetItem)
					{
						SendInfoMsg("You receive %s.", SPEECH_GetItemName(ptiTradeAnswer->pItem));
						ptiTradeAnswer->pItem->GiveTo(this);
						GiveGold(-ptiTradeAnswer->iPrice, false);
						ptiTradeAnswer->psiStoreItem->Quantity -= V_max(ptiTradeAnswer->pItem->iQuantity, 1);

						//Thothie FEB2008a - allow informing of other players of item transfer
						static msstringlist Params;
						Params.clearitems();
						Params.add(EntToString(ptiTradeAnswer->pItem));
						Params.add(EntToString(pVendor));
						CallScriptEvent("game_player_got_from_store", &Params);

						static msstringlist Params_sec;
						Params_sec.clearitems();
						Params_sec.add(EntToString(this));
						Params_sec.add(EntToString(ptiTradeAnswer->pItem));
						pVendor->CallScriptEvent("game_gave_player", &Params_sec); //Thothie JAN2013_08 - good to give vender/chest ability to know what player took

						goto EndTrade;
					}
					else
					{
						ptiTradeAnswer->pItem->SUB_Remove();
						goto EndTrade;
					}
				}
				else if (ptiTradeAnswer->iPrice)
				{
					static msstringlist Params;
					Params.clearitems();
					Params.add(EntToString(this));
					Params.add(UTIL_VarArgs("%i", ptiTradeAnswer->iPrice));
					pVendor->CallScriptEvent("game_gave_gold", &Params); //Thothie JAN2013_08 - good to give vender/chest ability to know it gave gold and amt

					GiveGold(ptiTradeAnswer->iPrice);
					pVendor->GiveGold(-ptiTradeAnswer->iPrice);
					goto EndTrade;
				}
			}
			else if (ptiTradeAnswer->iStatus == TRADE_SELL)
			{
				//The vendor will let me sell him this item
				if (!ptiTradeAnswer->pItem)
					goto EndTrade;

				SendInfoMsg("You sell %s for %i gold.", SPEECH_GetItemName(ptiTradeAnswer->pItem), ptiTradeAnswer->iPrice);
				GiveGold(ptiTradeAnswer->iPrice, false);
				ptiTradeAnswer->psiStoreItem->Quantity += (short)V_max(ptiTradeAnswer->pItem->iQuantity, 1);
				RemoveItem(ptiTradeAnswer->pItem);
				ptiTradeAnswer->pItem->SUB_Remove();

				goto EndTrade;
			}
		}
	}
	else
		; //I'm the vendor

EndTrade:
	//ClearConditions(MONSTER_TRADING); //NOV2014_12 Thothie - attempting to fix the Ike bug
	return NULL;
}
//
// AcceptOffer - Accept an offer from a player or monster
// �����������
bool CBasePlayer ::AcceptOffer()
{
	//Save the Offer info
	offerinfo_t OfferInfo;
	memcpy(&OfferInfo, &m_OfferInfo, sizeof(offerinfo_t));

	if (!CMSMonster::AcceptOffer())
		return false;

	CMSMonster *pMonster = (CMSMonster *)OfferInfo.pSrcMonster;
	CBasePlayer *pPlayer = pMonster->IsPlayer() ? (CBasePlayer *)pMonster : NULL;

	if (OfferInfo.ItemType == ITEM_NORMAL)
	{
		if (pPlayer)
			pPlayer->SendInfoMsg("%s accepts your %s", DisplayName(), SPEECH_GetItemName((CGenericItem *)OfferInfo.pItemData2));
		SendInfoMsg("You recieve %s from %s", SPEECH_GetItemName((CGenericItem *)OfferInfo.pItemData2), pMonster->DisplayName());
	}
	else if (OfferInfo.ItemType == ITEM_GOLD)
	{
		if (pPlayer)
			pPlayer->SendInfoMsg("%s accepts your %i gold coins", DisplayName(), (int)OfferInfo.pItemData);
		SendInfoMsg("You recieve %i gold coins from %s", (int)OfferInfo.pItemData, pMonster->DisplayName());
	}

	return true;
}
BOOL CBasePlayer::SkinMonster(CMSMonster *pDeadMonster)
{
	/*if( !pDeadMonster->Skin )
		return FALSE;

	CGenericItem *pSkin = NULL;
	bool fSkinIt = false;

	pSkin = NewGenericItem( (char *)STRING(pDeadMonster->Skin) );
	if( pSkin )
	{
		CGenericItem *pSkinToolPack = NULL;
		CGenericItem *pSkinTool = GetItem( "dagger", &pSkinToolPack );
		if( pSkinTool )
		{
			int dex = GetNatStat( NATURAL_DEX ) + GetSkillStat( SKILL_SMALLARMS );
			int pick = RANDOM_FLOAT(0,100);
			if( pick < dex ) {
				fSkinIt = true;
				SendInfoMsg( "You skin %s.", SPEECH::NPCName(pDeadMonster) );
			}
			else SendEventMsg( HUDEVENT_UNABLE,  msstring("You fail to skin ") + SPEECH::NPCName(pDeadMonster) + " corpse." );
		}
		else SendEventMsg( HUDEVENT_UNABLE, "Can't skin without a dagger!" );

		if( fSkinIt ) {
			pSkin->pev->origin = pDeadMonster->pev->origin;
			pSkin->pev->angles.y = RANDOM_FLOAT(0,360);
			return TRUE;
		}
		else pSkin->SUB_Remove( );
	}*/

	return FALSE;
}
void CBasePlayer::InitHUD()
{
	//Don't do anything relating to the client until now!!
	g_pGameRules->InitHUD(this);
	FireTargets("game_playerjoin", this, this, USE_TOGGLE, 0);
}
bool CBasePlayer::PrepareSpell(const char *pszName)
{
	//Thothie - make sure player actually HAS SPELL before giving it to him
	//otherwise you can summon any spell with prep command
	bool bCanGiveSpell = false;
	for (int i = 0; i < m_SpellList.size(); i++)
	{
		msstring ShortName = m_SpellList[i];
		if (ShortName == pszName)
		{
			return Magic::Prepare(pszName, this);
			bCanGiveSpell = true;
			break;
		}
	}
	return bCanGiveSpell;
}

class CLightSensor : public CBaseEntity
{
public:
	static CLightSensor *CreateLightSensor(int PlayerIndex, Vector &Origin)
	{
		CLightSensor *pNewLightSensor = GetClassPtr((CLightSensor *)NULL);
		pNewLightSensor->pev->origin = Origin;
		pNewLightSensor->pev->iuser1 = PlayerIndex;
		pNewLightSensor->Spawn();
		return pNewLightSensor;
	}

	void Spawn()
	{
		//Note:  Half-life will only give an an object a light level when:
		//- It has a model
		//- It is visible (rendered)
		//- It has a nextthink set
		//So here is the bare minimum:
		pev->classname = MAKE_STRING("light_sensor");
		//SET_MODEL(ENT(pev), "models/roach.mdl");
		SET_MODEL(ENT(pev), "models/null.mdl");
		pev->rendermode = kRenderTransTexture;
		pev->renderamt = 1;
		pev->origin = pev->origin + Vector(0, 0, 0);
		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time + 1.0;
	}
};
LINK_ENTITY_TO_CLASS(light_sensor, CLightSensor);

void CBasePlayer::UseSkill(int iSkill)
{
	/*	if( !Class ) return;

	if( Class->id == CLASS_ROGUE )
	{
		switch( iSkill )
		{
		case 0:
			if( !FBitSet(SkillInfo.SkillsActivated, SKILL_ROGUE_FADE) ||
				SkillInfo.FadeStatus == skillinfo_t::FADE_OUT )
			{
				//Only start a fade if I'm not already faded or I'm still fading out
				//from the last fade.
				//CLightSensor::CreateLightSensor( entindex(), pev->origin );
				SetBits( SkillInfo.SkillsActivated, SKILL_ROGUE_FADE );
				SkillInfo.TimeEndFade = -1; //Starts the fade
				MESSAGE_BEGIN( MSG_ONE, gmsgCLDllFunc, NULL, pev );
					WRITE_BYTE( 11 ); //Activate skill
					WRITE_BYTE( 0 );  //"Fade" skill
					WRITE_BYTE( 1 );  //Turn it on
					WRITE_COORD( 1.5 ); //Time to hide view model
				MESSAGE_END();

			}
			else SendInfoMsg( "You are already hidden.\n" );
			break;
		}
	}*/
}

void CBasePlayer ::SetModel(const char *Newmodel)
{
	SET_MODEL(ENT(pev), Newmodel);
	//	if( CanSendMessages ) g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer(edict()), "model", Newmodel );
}
void CBasePlayer ::SetModel(int Newmodel)
{
	SetModel(STRING(Newmodel));
}

void CBasePlayer ::SetTeam(CTeam *pNewTeam)
{
	//g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "team", TeamName );

	msstringlist Parameters;
	CTeam *pOldTeam = m_pTeam;
	m_pTeam = pNewTeam;

	if (pOldTeam)
		pOldTeam->RemoveFromTeam(this);
	if (pNewTeam)
	{
		pNewTeam->AddToTeam(this);
		Parameters.add(pNewTeam->TeamName());
		CallScriptEvent("game_party_join", &Parameters);
	}
	else
	{
		Parameters.add(pOldTeam ? pOldTeam->TeamName() : "");
		CallScriptEvent("game_party_leave", &Parameters);
	}
}
int CBasePlayer::IRelationship(CBaseEntity *pTarget)
{
	if (pTarget && pTarget->IsPlayer())
	{
		CBasePlayer *pPlayer = (CBasePlayer *)pTarget;
		if (SameTeam(this, pPlayer))
			return RELATIONSHIP_AL;
		if (pPlayer->m_PlayersKilled > 8)
			return RELATIONSHIP_NM;
		else if (pPlayer->m_PlayersKilled > 3)
			return RELATIONSHIP_HT;
		else if (pPlayer->m_PlayersKilled > 0)
			return RELATIONSHIP_DL;
	}
	return CMSMonster::IRelationship(pTarget);
}
bool CBasePlayer::CanDamage(CBaseEntity *pOther) //Can I damage this npc?
{
	if (pOther->IsPlayer()) //It's a player and player killing is allowed
	{
		if (MSGlobals::PKAllowed)
		{
			//MiB AUG2007a
			CBasePlayer *pOtherPlayer = (CBasePlayer *)pOther;

			//We're not party members and
			//PK is allowed everywhere or
			//The other player is not in town
			//MAR2008b Thothie - something is up with the PVP, just returning true to see if it fiexes
			return true;
			/*if( !SameTeam(this,pOther) && (MSGlobals::PKAllowedinTown || !pOtherPlayer->m_fInTownArea ) )
			{
				return true;
			}
			return false;*/
		}
		return false; //Hit a player, but PK is not allowed
	}

	return CBaseEntity::CanDamage(pOther);
}
void CBasePlayer::KickPlayer(const char *pszMessage)
{
	if (!IsElite())
	{
		if (pszMessage)
			ClientPrint(pev, HUD_PRINTNOTIFY, pszMessage);
		if (IsLocalHost())
			SERVER_COMMAND("disconnect\n");
		else
			SERVER_COMMAND(UTIL_VarArgs("kick #%i\n", GETPLAYERUSERID(edict())));
		logfile << Logger::LOG_INFO << "Kicked " << DisplayName() << " Reason: " << pszMessage << "\n";
	}
}
void CBasePlayer::Attacked(CBaseEntity *pAttacker, float flDamage, int bitsDamageType)
{
	//I was attacked by something
	if (!pAttacker || flDamage <= 0)
		return;

	/*if( FBitSet( SkillInfo.SkillsActivated, SKILL_ROGUE_FADE )
		&& SkillInfo.FadeStatus == skillinfo_t::FADE_IN )
	{
		SkillInfo.FadeAmt = -1; //kill the fade
	}*/

	if (pAttacker->IsPlayer() && pAttacker != this)
	{
		hudtextparms_t htp;
		memset(&htp, 0, sizeof(hudtextparms_t));
		htp.x = 0.02;
		htp.y = 0.6;
		htp.effect = 1;
		htp.r1 = 255;
		htp.g1 = htp.b1 = 0;
		htp.fadeinTime = 0.5;
		htp.fadeoutTime = 2.0;
		htp.holdTime = 2.0;
		UTIL_HudMessage(this, htp, UTIL_VarArgs("%s is attacking you!", pAttacker->DisplayName()));
		return;
	}
}
void CBasePlayer::Seen(CMSMonster *pMonster)
{
	//I was spotted by someone
	if (!pMonster || pMonster->IsPlayer())
		return;

	/*	if( FBitSet( SkillInfo.SkillsActivated, SKILL_ROGUE_FADE )
		&& SkillInfo.FadeStatus == skillinfo_t::FADE_IN )
	{
		SkillInfo.FadeAmt = -1; //kill the fade
	}*/
	msstringlist Parameters;
	Parameters.add(EntToString(pMonster));
	CallScriptEvent("game_seen", &Parameters);
}
void CBasePlayer::SetQuest(bool SetData, msstring_ref Name, msstring_ref Data)
{
	//if( SetData ) ALERT( at_aiconsole, UTIL_VarArgs("Got Quest String1(%s) String2(%s) \n", Name, Data));

	//thothie attempting to fix quest system
	/*if ( UTIL_VarArgs("%s",Name) == "erase" )
	{
		ALERT( at_aiconsole, "Hard_Code-erasing quest data");
		m_Quests.clearitems();
		m_Quests.clear();
		 for (int i = 0; i < m_Quests.size(); i++)
		{
			m_Quests.erase(i);
		}
	}
	else
	{*/

	int Found = -1;
	for (int i = 0; i < m_Quests.size(); i++)
	{
		if (m_Quests[i].Name == Name)
		{
			if (SetData)
				m_Quests[i].Data = Data;
			else
				m_Quests.erase(i);
			Found = i;
			break;
		}
	}

	if (Found <= -1)
	{
		if (SetData)
		{
			quest_t Quest;
			Quest.Name = Name;
			Quest.Data = Data;
			m_Quests.add(Quest);
		}
	}
}

bool CBasePlayer::RestoreAllServer(void *pData, ulong Size)
{
	startdbg;
	dbg("Begin");

	logfile << Logger::LOG_INFO << "Load Character: " << DisplayName() << "\n";

	//Thothie JAN2010_10 - flag to tell "char" function character is loaded, so no clickie
	m_CharacterState = CHARSTATE_LOADING;

	//Thothie JUL2007 - prevent loading of STEAM_ID_PENDING chars
	msstring msDisplayName = DisplayName();
	if (msDisplayName.starts_with("LOAD_FAILED-RECONNECT"))
	{
		KickPlayer("\nNo clicky means no clicky. Please reconnect.\n");
		return false;
	}

	RemoveAllItems(false, true);				  //Remove all items
	CallScriptEvent("game_reset_wear_positions"); //Re-initialize all the wearable positions for items

	chardata_t Data;
	if (!MSChar_Interface::ReadCharData(pData, Size, &Data))
	{
		KickPlayer("\nTampering with the save file results in a ban!\n");
		return false;
	}

	//Read Maps Visited

	m_Maps = Data.m_VisitedMaps;

	m_MapStatus = INVALID_MAP;
	char cCurrentMap[32];
	strncpy(cCurrentMap, STRING(gpGlobals->mapname), sizeof(cCurrentMap));
	strncpy(m_NextMap, cCurrentMap, 32);

	//Determine whether a transition took place and set the spawn transition accordingly
	if (FStrEq(Data.MapName, cCurrentMap))
	{
		m_MapStatus = OLD_MAP;
		strncpy(m_OldTransition, Data.OldTrans, 32); //Copy transition names to savable memory
	}
	else if (FStrEq(Data.NextMap, cCurrentMap))
	{
		m_MapStatus = NEW_MAP;
		strncpy(m_OldTransition, Data.NewTrans, 32); //The new transition becomes the old transition
	}

	m_SpawnTransition = m_OldTransition;
	m_NextTransition[0] = 0;
	m_NextMap[0] = 0;

	//Copy the data

	m_DisplayName = Data.Name;
	pev->netname = 0;
	//msstring String = msstring("\"name ") + Data.Name + "\"\n";

	//Force the client to do the name command
	//This is so the engine will run its name check routine and append (x) if people have the same names
	//I need that check because if two people were to forecefully get assigned the same name (with g_engfuncs.pfnSetClientKeyValue),
	//Then the whole server does the "5 minute delayed messages" bug
	CLIENT_COMMAND(edict(), "name %s\n", Data.Name);
	//g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "name", (char *)Data.Name );

	dbg("Read Stats");
	m_OldGold = m_Gold = Data.Gold;

	//MiB JAN2010_15 Gold Change on Spawn.rtf
	
	MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_SETSTAT], NULL, pev);
	WRITE_BYTE(3);
	WRITE_BYTE(0);
	WRITE_LONG(m_Gold);
	MESSAGE_END();

	if (Data.HP > 0)
	{
		float CappedHP = V_max(Data.HP, 0);
		float CappedMP = V_max(Data.MP, 0);
		pev->health = m_HP = CappedHP;
		m_MP = CappedMP;
		m_MaxHP = Data.MaxHP;
		m_MaxMP = Data.MaxMP;
	}
	//Player saved while dead
	else
		pev->deadflag = DEAD_DEAD;

	strncpy(m_cEnterMap,  Data.MapName, sizeof(m_cEnterMap) );

	SetTeam(CTeam::CreateTeam(Data.Party, Data.PartyID));
	m_Gender = Data.Gender;
	m_fIsElite = Data.IsElite ? true : false;
	m_PlayersKilled = Data.PlayerKills;
	m_TimeWaitedToForgetKill = Data.TimeWaitedToForgetKill;
	m_TimeWaitedToForgetSteal = Data.TimeWaitedToForgetSteal;

	//MiB JAN2010 - Player Kill Stickiness.rtf
	MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_CLDLLFUNC], NULL, pev);
	WRITE_BYTE(8);
	WRITE_SHORT(m_PlayersKilled);
	MESSAGE_END();

	m_JoinType = MSChar_Interface::CanJoinThisMap(Data, m_Maps);

	//Create our Human body -- Must be done here
	if (Body)
		Body->Delete(); //Moved to SUB_Remove
	Body = msnew CHumanBody;
	//Body->Initialize( this );

	//Read skills
	m_Stats = Data.m_Stats;

	//MiB Aug 2008a (JAN2010_15) - Reset tomes on client
	MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_SETPROP], NULL, pev);
	WRITE_BYTE(PROP_SPELL);
	WRITE_BYTE(-1);
	MESSAGE_END();

	//Read Magic spells
	for (int s = 0; s < Data.m_Spells.size(); s++)
		LearnSpell(Data.m_Spells[s]);

	mslist<CGenericItem *> Items; //Keep track of ALL items, for quickslot assignment later

	//Read Items
	dbg("Read Items");
	for (int i = 0; i < Data.m_Items.size(); i++)
	{
		CGenericItem *pItem = Data.m_Items[i].operator CGenericItem *();

		if (pItem->m_Location == ITEMPOS_HANDS)
			AddItem(pItem, true, false, pItem->m_Hand);
		else
			pItem->AddToOwner(this);

		Items.add(pItem);
		for (int c = 0; c < pItem->Container_ItemCount(); c++)
			Items.add(pItem->Container_GetItem(c));

		//if( FBitSet(pItem->MSProperties(), ITEM_WEARABLE) && pItem->IsWorn() )	//Wear the wearable items
		//	pItem->WearItem( );
	}

	//Read storage items
	m_Storages = Data.m_Storages;
	Storage_Send(); //Send all the items in storage

	//Read Companions
	dbg("Read Companions");

	m_Companions = Data.m_Companions;
	//Thothie JUN2008a - just read in companions, save the summoning until the script command "summonpets"
	//- scratch the above, if the player saves while his pet is not present, it corrupts the pet and character
	for (int c = 0; c < m_Companions.size(); c++)
	{
		companion_t &Companion = m_Companions[c];

		CMSMonster *pCompanion = (CMSMonster *)CREATE_ENT("ms_npc");
		if (!pCompanion)
			continue;
		Companion.Active = true;
		Companion.Entity = pCompanion;

		pCompanion->StoreEntity(this, ENT_OWNER);
		edict_t *pEdict = pCompanion->edict();
		pCompanion->Spawn(Companion.ScriptName);
		if (pEdict->free)
			continue;

		pCompanion->pev->origin = pev->origin + Vector(0, 0, 128);

		IScripted *pScripted = pCompanion->GetScripted();
		if (!pScripted || !pScripted->m_Scripts.size())
			continue;
		for (int v = 0; v < Companion.SaveVarName.size(); v++)
			pScripted->SetScriptVar(Companion.SaveVarName[v].c_str(), Companion.SaveVarValue[v].c_str());

		pScripted->CallScriptEvent("game_companion_restore");
	}

	//Read Help tips
	m_ViewedHelpTips = Data.m_ViewedHelpTips;

	//Read Quests
	m_Quests = Data.m_Quests;

	//Read QuickSlots
	for (int q = 0; q < Data.m_QuickSlots.size(); q++)
	{
		quickslot_t &QuickSlot = Data.m_QuickSlots[q];
		if (QuickSlot.Active && (QuickSlot.Type == QS_ITEM))
		{
			bool bFound = false;
			for (unsigned int i = 0; i < Items.size(); i++)
			{
				if (Items[i]->m_OldID == QuickSlot.ID)
				{
					QuickSlot.ID = Items[i]->m_iId;
					bFound = true;
					break;
				}
			}
			QuickSlot.Active = bFound;
		}
		m_QuickSlots[q] = QuickSlot;
	}

	//Make sure an update is sent from UpdateClientData ASAP
	for (int i = 0; i < m_Stats.size(); i++)
		m_Stats[i].OutDate();

	m_CharacterState = CHARSTATE_LOADED;

	//Send the character name down to client
	MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_CLDLLFUNC], NULL, pev);
	WRITE_BYTE(4);
	WRITE_STRING_LIMIT(Data.Name, 32);
	MESSAGE_END();

	//Send music data 

	dbg("Call CBasePlayer::Spawn()");
	Spawn();

	enddbg;

	return true;
}

bool CBasePlayer::LoadCharacter(int Num)
{
	if (Num >= MAX_CHARSLOTS)
		return false;

	charinfo_t &Char = m_CharInfo[Num];
	if (Char.Status != CDS_LOADED)
		return false;

	if (RestoreAllServer(Char.Data, Char.DataLen))
	{
		m_CharacterNum = Num;
		return true;
	}

	return false;
}

void CBasePlayer::SUB_Remove()
{
	CBaseMonster::SUB_Remove();
}

void CBasePlayer::Think_SendCharData()
{
	//If it's time, then send info about a character
	//Keep doing this until info about all 3 chars is sent to client
	if (m_CharacterState == CHARSTATE_LOADED || gpGlobals->time < m_TimeSendCharInfo || !m_fGameHUDInitialized)
		return;

	msstringlist VisitedMaps;

	for (int i = 0; i < MAX_CHARSLOTS; i++)
	{
		charinfo_t& CharInfo = m_CharInfo[i];

		if (CharInfo.m_CachedStatus == CharInfo.Status)
			continue;

		CharInfo.m_CachedStatus = CharInfo.Status;

		// Send base char data
		MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_CHARINFO], NULL, pev);
		WRITE_BYTE((CharInfo.Status == CDS_LOADED) ? CHAR_TYPE_HEADER : CHAR_TYPE_BASE);
		WRITE_BYTE(CharInfo.Index);	   //This character is available or pending
		WRITE_BYTE(CharInfo.Status);   //Current status
		WRITE_BYTE(CharInfo.Location); //Current storage location

		if (CharInfo.Status == CDS_LOADED)
		{
			WRITE_STRING_LIMIT(CharInfo.Name, 32);	 //Character name
			WRITE_STRING_LIMIT(CharInfo.MapName, 16);	 //Map char was last on
			WRITE_STRING_LIMIT(CharInfo.OldTrans, 32); //Last trans char hit
			WRITE_STRING_LIMIT(CharInfo.NextMap, 32);	 //New map char wants to enter
			WRITE_STRING_LIMIT(CharInfo.NewTrans, 32); //New trans at new map
			WRITE_SHORT(CharInfo.body);		 //MIB JAN2010_27 - Char Selection Fix - Body the model should use

			// jointype_e JoinType = MSChar_Interface::CanJoinThisMap( CharInfo, VisitedMaps );

			byte CharFlags = 0;
			if (CharInfo.IsElite)
				SetBits(CharFlags, (1 << 0));
			if (CharInfo.Gender == GENDER_FEMALE)
				SetBits(CharFlags, (1 << 1));
			if (CharInfo.JoinType != JN_NOTALLOWED)
				SetBits(CharFlags, (1 << 2));

			WRITE_BYTE(CharFlags); //Character flags
		}
		MESSAGE_END();

		// Send items data
		if (CharInfo.Status == CDS_LOADED)
		{
			const int maxItemsToSend = V_min(CharInfo.GearInfo.size(), 20);
			MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_CHARINFO], NULL, pev);
			WRITE_BYTE(CHAR_TYPE_ITEMS);
			WRITE_BYTE(CharInfo.Index);
			WRITE_BYTE(maxItemsToSend);
			for (int i = 0; i < maxItemsToSend; i++)
			{
				const gearinfo_t& GearInfo = CharInfo.GearInfo[i];
				WRITE_BYTE(GearInfo.Flags);
				WRITE_SHORT(GearInfo.Model);
				WRITE_SHORT(GearInfo.Body);
				WRITE_SHORT(GearInfo.Skin);
				WRITE_BYTE(GearInfo.Anim);
			}
			MESSAGE_END();
		}
	}

	m_TimeSendCharInfo = gpGlobals->time + 0.5f;
}

bool CBasePlayer::IsInAttackStance()
{
	//Server can't tell if the player is running, so just
	//check if player speed is over walkspeed
	if (pev->velocity.Length2D() >= WalkSpeed(false) * 1.3)
		return false;

	if (m_TimeResetLegs && gpGlobals->time < m_TimeResetLegs)
		return true;

	for (int i = 0; i < MAX_NPC_HANDS; i++)
		if (Hand(i) && Hand(i)->IsInAttackStance())
			return true;

	return false;
}
void CBasePlayer::Storage_Open(msstring_ref pszDisplayName, msstring_ref pszStorageName, float flFeeRatio, entityinfo_t &Entity)
{
	m_CurrentStorage.Active = true;
	m_CurrentStorage.StorageName = pszStorageName;
	m_CurrentStorage.DisplayName = pszDisplayName;
	m_CurrentStorage.flFeeRatio = flFeeRatio;
	m_CurrentStorage.ReqTeller = Entity.pvPrivData ? true : false;
	if (m_CurrentStorage.ReqTeller)
		m_CurrentStorage.Teller = Entity;

	MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_ITEM], NULL, pev);
	WRITE_BYTE(5);
	WRITE_STRING_LIMIT(m_CurrentStorage.DisplayName, 90);
	WRITE_STRING_LIMIT(m_CurrentStorage.StorageName, 90);
	WRITE_FLOAT(m_CurrentStorage.flFeeRatio);
	MESSAGE_END();
}
void CBasePlayer::Storage_Send()
{
	MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_STOREITEM], NULL, pev);
	WRITE_BYTE(1); //reset storage items
	MESSAGE_END();

	//Send all storage items.
	for (int s = 0; s < m_Storages.size(); s++)
	{
		if (!m_Storages[s].Items.size())
		{
			//If the storage has no items, just send the storage name, indicating no items
			MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_STOREITEM], NULL, pev);
			WRITE_BYTE(2); //storage item
			WRITE_STRING_LIMIT(m_Storages[s].Name, WRITE_STRING_MAX);
			WRITE_BYTE(0); //No items
			MESSAGE_END();
		}
		else
		{
			//If the storage has items, send a full message
			for (int i = 0; i < m_Storages[s].Items.size(); i++)
			{
				MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_STOREITEM], NULL, pev);
				WRITE_BYTE(2); //storage item
				WRITE_STRING_LIMIT(m_Storages[s].Name, WRITE_STRING_MAX);
				WRITE_BYTE(1); //Add this item to the storage
				SendGenericItem(this, m_Storages[s].Items[i], false);
				MESSAGE_END();
			}
		}
	}
}

void CBasePlayer::SaveChar()
{
	//Save right now
	MSChar_Interface::SaveChar(this, NULL);
}

void CBasePlayer::QuickSlot_Create(int Slot, ulong ID, bool Verbose)
{
	if (Slot < 0 || Slot >= MAX_QUICKSLOTS)
		return;

	CGenericItem *pItem = MSUtil_GetItemByID(ID, this);
	if (!pItem || pItem->m_Hand == HAND_PLAYERHANDS)
		return;

	quickslot_t &QuickSlot = m_QuickSlots[Slot];
	bool CreateSlot = false;
	int Flags = 0;

	if (ID != 0)
	{
		if (!FBitSet(pItem->MSProperties(), ITEM_SPELL))
		{
			QuickSlot.Active = true;
			QuickSlot.Type = QS_ITEM;
			QuickSlot.ID = pItem->m_iId;
			CreateSlot = true;
		}
		else
		{
			for (int i = 0; i < m_SpellList.size(); i++)
			{
				msstring LongName = pItem->m_Scripts[0]->m.ScriptFile;
				msstring ShortName = m_SpellList[i];
				msstring NameSubstr = LongName.find_str(ShortName);
				if (ShortName == NameSubstr)
				{
					QuickSlot.Active = true;
					QuickSlot.Type = QS_SPELL;
					QuickSlot.ID = i;
					CreateSlot = true;
					break;
				}
			}
		}
	}

	if (CreateSlot)
	{
		SetBits(Flags, (1 << 0));
		if (Verbose)
			SetBits(Flags, (1 << 1));
	}
	else
	{
		QuickSlot.Active = false;
	}

#ifdef VALVE_DLL
	MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_CLDLLFUNC], NULL, pev);
	WRITE_BYTE(20);
	WRITE_BYTE(Flags);
	WRITE_BYTE(Slot);
	WRITE_BYTE(QuickSlot.Type);
	WRITE_LONG(QuickSlot.Active ? QuickSlot.ID : 0);
	MESSAGE_END();
#endif
}
void CBasePlayer::QuickSlot_Use(int Slot)
{
	/*if( Slot < 0 || Slot >= MAX_QUICKSLOTS ) return;

	CGenericItem *pItem = MSUtil_GetItemByID( ID, this );
	if( !pItem || pItem->m_Hand == HAND_PLAYERHANDS )
		return;*/
}

void SendViewAnim(CBasePlayer *pPlayer, int iAnim, int body)
{
	MESSAGE_BEGIN(MSG_ONE, SVC_WEAPONANIM, NULL, pPlayer->pev);
	WRITE_BYTE(iAnim); // sequence number
	WRITE_BYTE(body);  // weaponmodel bodygroup.
	MESSAGE_END();
}

void MSGSend_PlayerInfo(CBasePlayer *pSendToPlayer, CBasePlayer *pPlayer)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgScoreInfo, NULL, pSendToPlayer->edict());
	WRITE_BYTE(pPlayer->entindex()); // client number
	int Flags = 0;
	if (pPlayer->m_fIsElite)
		SetBits(Flags, (1 << 0));
	if (pPlayer->CurrentTransArea)
		SetBits(Flags, (1 << 1));
	if (pPlayer->m_CharacterState == CHARSTATE_LOADED)
		SetBits(Flags, (1 << 2));
	WRITE_BYTE(Flags);
	if (pPlayer->m_CharacterState == CHARSTATE_LOADED)
	{
		WRITE_STRING_LIMIT(pPlayer->GetFullTitle(), WRITE_STRING_MAX);
		WRITE_SHORT((int)pPlayer->MaxHP()); //FEB2008a -- Shuriken
		WRITE_SHORT((int)pPlayer->m_HP);	//FEB2008a -- Shuriken
	}
	MESSAGE_END();
}

bool CBasePlayer::SwapMusic(int musicArea, int mode, std::string track)
{
	if (musicArea != m_iMusicArea || musicArea == -1)
	{
		m_iMusicArea = musicArea;

		MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_MUSIC], NULL, this->pev);
		WRITE_BYTE(mode);

		if (mode != MUSIC_STOP || track.length() > 0) //If playing music instead of stopping, send the music file
		{
			WRITE_STRING(track.c_str());
		}

		MESSAGE_END();

		return true;
	}
	return false;
}