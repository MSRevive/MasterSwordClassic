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
//
// teamplay_gamerules.cpp
//

//Master Sword
#include	"inc_weapondefs.h"
#include	"teams.h"
#include	"game.h"
#include	"items.h"
#include	"voice_gamemgr.h"
#include	"hltv.h"
#include	"logger.h"
#include	"msgamerules.h"
#include	"global.h"
#include	"svglobals.h"
#include	"mscharacter.h"

#include <climits>

extern DLL_GLOBAL CGameRules	*g_pGameRules;
extern DLL_GLOBAL BOOL	g_fGameOver;
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgScoreInfo;
extern int gmsgMOTD;
//int CountPlayers( void );

bool CheckBanned( msstring_ref SteamID );

//float g_TimeTryValidate = 0.0f;
#define VALIDATE_DELAY (60 * 30)// 30 mins
float g_ServerResetTimer = 0.0f;

#define ITEM_RESPAWN_TIME	30
#define WEAPON_RESPAWN_TIME	20
#define AMMO_RESPAWN_TIME	20

CVoiceGameMgr	g_VoiceGameMgr;

class CMultiplayGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker )
	{
		if( pTalker->m_SayType == SPEECH_GLOBAL )		//Global - Speak to all
			return true;
		else if( pTalker->m_SayType == SPEECH_PARTY )	//Party - Speak to party only
		{
			if( SameTeam( pTalker, pListener ) )
				return true;
			else
				return false;
		}
		else if( pTalker->m_SayType == SPEECH_LOCAL )	//Local - Speak to people around you
		{
			if( (pListener->Center() - pTalker->Center()).Length() < SPEECH_LOCAL_RANGE )
				return true;
			else
				return false;
		}

		return true;
	}
};
static CMultiplayGameMgrHelper g_GameMgrHelper;

//*********************************************************
// Rules for the half-life multiplayer game.
//*********************************************************

CHalfLifeMultiplay::CHalfLifeMultiplay()
{
	g_VoiceGameMgr.Init( &g_GameMgrHelper, gpGlobals->maxClients );

	RefreshSkillData();
	m_flIntermissionEndTime = 0;
	
	// 11/8/98
	// Modified by YWB:  Server .cfg file is now a cvar, so that 
	//  server ops can run multiple game servers, with different server .cfg files,
	//  from a single installed directory.
	// Mapcyclefile is already a cvar.

	// 3/31/99
	// Added lservercfg file cvar, since listen and dedicated servers should not
	// share a single config file. (sjb)
	if ( IS_DEDICATED_SERVER() )
	{
		// dedicated server
		char *servercfgfile = (char *)CVAR_GET_STRING( "servercfgfile" );

		if ( servercfgfile && servercfgfile[0] )
		{
			char szCommand[256];
			
			ALERT(at_console, "Executing dedicated server config file\n");
			_snprintf(szCommand, sizeof(szCommand), "exec %s\n", servercfgfile);
			SERVER_COMMAND( szCommand );
		}
	}
	else
	{
		// listen server
		char *lservercfgfile = (char *)CVAR_GET_STRING( "lservercfgfile" );

		if ( lservercfgfile && lservercfgfile[0] )
		{
			char szCommand[256];
			
			ALERT(at_console, "Executing listen server config file\n");
			_snprintf(szCommand, sizeof(szCommand), "exec %s\n", lservercfgfile);
			SERVER_COMMAND( szCommand );
		}
	}

	SERVER_EXECUTE( );
	m_CurrentVote.fActive = false;

	for (int i = 0; i < CLPERMENT_TOTAL; i++) 
	{
		CBaseEntity *pInvEntity = GetClassPtr( (CBaseEntity *)NULL );
		SetBits( pInvEntity->pev->flags, FL_DORMANT );
		MSGlobals::ClEntities[i] = pInvEntity->entindex( );
	}
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::RefreshSkillData( void )
{
// load all default values
	CGameRules::RefreshSkillData();
}

// longest the intermission can last, in seconds
#define MAX_INTERMISSION_TIME		120

//Master Sword
//bool CheckData( );
//------------
//=========================================================
//Think function
//=========================================================
void CHalfLifeMultiplay::Think( void )
{
	//dbg( "Call g_VoiceGameMgr.Update" );
	g_VoiceGameMgr.Update( gpGlobals->frametime );

	//dbg( "Call MSGameThink" );
	MSGameThink( );

	//dbg( "Call UpdateVote" );
	UpdateVote( );

	g_psv_gravity->value = 800.0;
	g_maxspeed->value = 600;
	g_accelerate->value = 10.0;
	g_airaccelerate->value = 10.0;
	g_wateraccelerate->value = 10.0;
	g_stepsize->value = 18.0;
	g_friction->value = 4.0;
	g_stopspeed->value = 100.0;
	g_waterfriction->value = 1.0;

	//Delete empty teams here, so they won't be deleted in the middle of a frame
	//dbg( "Delete empty teams" );
	for( int i = CTeam::Teams.size()-1; i >= 0; i-- )
	{
		CTeam *pTeam = CTeam::Teams[i];
		if( !pTeam->MemberList.size() )
			{ CTeam::Teams.erase( i ); delete pTeam; }
	}

	//if player joins and timer was started than reset timer.
	if (UTIL_NumPlayers() && g_ServerResetTimer > 0.0f)
		g_ServerResetTimer = 0.0f;

	if ((CVAR_GET_FLOAT("ms_reset_time") > 0) && !UTIL_NumPlayers())
	{
		ALERT(at_console, "Server empty checking for reset.\n");
		if (!g_ServerResetTimer)
			g_ServerResetTimer = gpGlobals->time + (CVAR_GET_FLOAT("ms_reset_time")*60);

		//reset server once timer expires
		if (gpGlobals->time >= g_ServerResetTimer)
		{
			ALERT(at_console, "Resetting server.\n");
			g_ServerResetTimer = 0.0f;
			std::string resetMap = CVAR_GET_STRING("ms_reset_map");
			std::string mapCmd = "map " + resetMap + "\n";
			SERVER_COMMAND((char*)mapCmd.c_str());
		}
	}

	///// Check game rules /////
	/*static int last_frags;
	static int last_time;

	int frags_remaining = 0;
	int time_remaining = 0;

	if ( g_fGameOver )   // someone else quit the game already
	{
		if ( m_flIntermissionEndTime < gpGlobals->time )
		{
			if ( m_iEndIntermissionButtonHit  // check that someone has pressed a key, or the max intermission time is over
				|| ((m_flIntermissionEndTime + MAX_INTERMISSION_TIME) < gpGlobals->time) ) 
				ChangeLevel(); // intermission is over
		}
		return;
	}
*/

	// Updates when frags change
/*	if ( frags_remaining != last_frags )
	{
		g_engfuncs.pfnCvar_DirectSet( &fragsleft, UTIL_VarArgs( "%i", frags_remaining ) );
	}*/


	//Master Sword stuff
	/*if( !g_TimeTryValidate ) g_TimeTryValidate = gpGlobals->time;

	if( gpGlobals->time >= g_TimeTryValidate && strlen(CVAR_GET_STRING("ms_key")) )
	{
		g_TimeTryValidate = gpGlobals->time + VALIDATE_DELAY;
		CheckValidation( );
	}*/

	//Check for clients sending item or stat info
//	while( CheckData( ) );

	//dbg( "Check switch to start map" );
	//If players join the server, but their characters aren't on the map, switch the map to a start map
	if( ms_joinreset.value )
		if( m_TimeCheckSwitchToStartMap && gpGlobals->time > m_TimeCheckSwitchToStartMap )
		{
			bool Switch = !IsAnyPlayerAllowedInMap( );
			m_TimeCheckSwitchToStartMap = 0;

			if( Switch )
			{
				m_TimeSwitchToNewMap = gpGlobals->time + 5.0f;
				m_NewMapName = "edana";
				SendHUDMsgAll( "#JOINRESET_TITLE", "#JOINRESET_TEXT" );
			}
		}

	if( m_TimeSwitchToNewMap && gpGlobals->time >= m_TimeSwitchToNewMap )
	{
		m_TimeSwitchToNewMap = 0;
		if( !IsAnyPlayerAllowedInMap( ) )
			SERVER_COMMAND( msstring("changelevel ") + m_NewMapName + "\n" );
	}
}

bool CHalfLifeMultiplay::IsAnyPlayerAllowedInMap()
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex(i);
		if (!pPlayer) continue;

		//Somebody is still loading a character from the Central Server.  Wait for success or an error
		for (int c = 0; c < MAX_CHARSLOTS; c++)
		{
			if (pPlayer->m_CharInfo[c].Status == CDS_LOADING)
				return true;
		}

		// If one person has a character that can join the map, then don't switch the map
		if (pPlayer->m_CanJoin) return true;
	}

	return false;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsMultiplayer( void )
{
	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsDeathmatch( void )
{
	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsCoOp( void )
{
	return gpGlobals->coop;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	if ( !pWeapon->CanDeploy() )
	{
		// that weapon can't deploy anyway.
		return FALSE;
	}

	if ( !pPlayer->m_pActiveItem )
	{
		// player doesn't have an active item!
		return TRUE;
	}

	if ( !pPlayer->m_pActiveItem->CanHolster() )
	{
		// can't put away the active item.
		return FALSE;
	}

	if ( pWeapon->iWeight() > pPlayer->m_pActiveItem->iWeight() )
	{
		return TRUE;
	}

	return FALSE;
}

BOOL CHalfLifeMultiplay::GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon )
{
	//Function unused
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128])
{
	g_VoiceGameMgr.ClientConnected( pEntity );

	const char *AuthID = GETPLAYERAUTHID(pEntity);

	//Check if the player has been banned by vote
	if( CheckBanned( AuthID ) )
	{
		strncpy(szRejectReason, "You are BANNED\n", 128);
		return FALSE;
	}

	g_ServerResetTimer = NULL;

	return TRUE;
}

extern int gmsgSayText;
extern int gmsgGameMode;

void CHalfLifeMultiplay::UpdateGameMode( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgGameMode, NULL, pPlayer->edict() );
		WRITE_BYTE( 0 );  // game mode (use client non-teamplay mode)
	MESSAGE_END();
}

void CHalfLifeMultiplay::InitHUD( CBasePlayer *pPlayer )
{
	// notify other clients of player joining the game
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs("%s has joined the game\n", (pPlayer->pev->netname && STRING(pPlayer->pev->netname)[0] != 0) ? STRING(pPlayer->pev->netname) : "unconnected" ));

	UTIL_LogPrintf( "\"%s<%i>\" has entered the game\n",  STRING( pPlayer->pev->netname ), GETPLAYERUSERID( pPlayer->edict() ) );

	UpdateGameMode( pPlayer );

	//Notify the global script
	if( MSGlobals::GameScript )
	{
		msstringlist Parameters;
		Parameters.add( EntToString(pPlayer) );
		MSGlobals::GameScript->CallScriptEvent( "game_playerjoin", &Parameters );
	}

	//Notify all the entities with scripts
	CBaseEntity *pEntity = NULL;
	edict_t		*pEdict = NULL;
	for( int i = 1; i < gpGlobals->maxEntities; i++ )
	{
		pEdict = g_engfuncs.pfnPEntityOfEntIndex( i );

		if( !pEdict || pEdict->free )	// Not in use
			continue;
		
		CBaseEntity *pEntity = MSInstance( pEdict );
		if( !pEntity ) continue;

		IScripted *pScripted = pEntity->GetScripted( );
		if( pScripted )
			pScripted->Script_InitHUD( pPlayer );
	}

	if( MSGlobals::GameScript )
		MSGlobals::GameScript->Script_InitHUD( pPlayer );

	// sending just one score makes the hud scoreboard active;  otherwise
	// it is just disabled for single play
	/*MESSAGE_BEGIN( MSG_ONE, gmsgScoreInfo, NULL, pl->edict() );
		WRITE_BYTE( ENTINDEX(pl->edict()) );
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );
	MESSAGE_END();*/

	//No motd
	//SendMOTDToClient( pl->edict() );

	// loop through all active players and send their score info to the new client
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pOtherPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
		if( !pOtherPlayer ) continue;

		if( i != pPlayer->entindex() )
		{
			//Send Player info
			MSGSend_PlayerInfo( pPlayer, pOtherPlayer );

			//Send Party info
			MSGSend_PlayerTeam( pPlayer, pOtherPlayer );
		}
	}

	if ( g_fGameOver )
	{
		MESSAGE_BEGIN( MSG_ONE, SVC_INTERMISSION, NULL, pPlayer->edict() );
		MESSAGE_END();
	}

	//Player has no characters that can join this map.
	//After a short delay, check if there are any other players who can join, and if not, switch to the start map
	if( !pPlayer->m_CanJoin )
	{
		if( ms_joinreset.value > 0 )
			m_TimeCheckSwitchToStartMap = gpGlobals->time + ms_joinreset.value;
	}
}

//=========================================================
//=========================================================

void CHalfLifeMultiplay::ClientDisconnected( edict_t *pClient )
{
	if( !pClient )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );

	if( pPlayer && pPlayer->IsPlayer() )
	{
		pPlayer->SaveChar(); // Save data on disconnect! TODO: should this count for non-FN as well?

		FireTargets( "game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0 );

		//Thothie JUN2007a - attempting to make sure next client takes slot #1
		//- reconnecting clients are assigned to their old slots somehow.
		//- hoping this will undo that
		int iPlayerOfs = pPlayer->entindex() -1;
		strncpy(g_NewClients[iPlayerOfs].Addr, "127.0.0.1", 128);
	
		//Thothie JUN2007a - Capturing game player leave scriptside
		//this is used to remove his items from world to prevent duplication
		msstringlist Parameters;
		Parameters.add( EntToString(pPlayer) );
		MSGlobals::GameScript->CallScriptEvent( "game_playerleave", &Parameters );

		UTIL_LogPrintf( "\"%s<%i><%s>\" disconnected\n",  pPlayer->DisplayName(), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()) );

		msstring LeaveMsg = msstring("- ") + pPlayer->DisplayName() + " has departed from the lands\n";
		MESSAGE_BEGIN( MSG_ALL, gmsgSayText, NULL );
			WRITE_BYTE( pPlayer->entindex() );
			WRITE_STRING_LIMIT(LeaveMsg.c_str(), 128);
		MESSAGE_END();

		pPlayer->SUB_Remove( );
	}
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	int iFallDamage = (int)falldamage.value;

	switch ( iFallDamage )
	{
	case 1://progressive
		pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
		return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
		break;
	default:
	case 0:// fixed
		return 10;
		break;
	}
} 

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerThink( CBasePlayer *pPlayer )
{	
	if ( g_fGameOver )
	{
		// check for button presses
		if ( pPlayer->m_afButtonPressed & ( IN_DUCK | IN_ATTACK | IN_ATTACK2 | IN_USE | IN_JUMP ) )
			m_iEndIntermissionButtonHit = TRUE;

		// clear attack/use commands from player
		pPlayer->m_afButtonPressed = 0;
		pPlayer->pev->button = 0;
		pPlayer->m_afButtonReleased = 0;
	}
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerSpawn( CBasePlayer *pPlayer )
{
	//Master Sword - Nothing here
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	return TRUE;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlPlayerSpawnTime( CBasePlayer *pPlayer )
{
	return gpGlobals->time;//now!
}

BOOL CHalfLifeMultiplay::AllowAutoTargetCrosshair( void )
{
	return FALSE;
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CHalfLifeMultiplay::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	return 1;
}


//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CHalfLifeMultiplay::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	DeathNotice( pVictim, pKiller, pInflictor );

	pVictim->m_iDeaths += 1;


	FireTargets( "game_playerdie", pVictim, pVictim, USE_TOGGLE, 0 );
	CBasePlayer *peKiller = NULL;
	CBaseEntity *ktmp = CBaseEntity::Instance( pKiller );
	if ( ktmp && (ktmp->Classify() == CLASS_PLAYER) )
		peKiller = (CBasePlayer*)ktmp;

	if ( pVictim->pev == pKiller )  
	{  // killed self
		pKiller->frags -= 1;
	}
	else if ( ktmp && ktmp->IsPlayer() )
	{
		// if a player dies in a deathmatch game and the killer is a client, award the killer some points
		pKiller->frags += IPointsForKill( peKiller, pVictim );
		
		FireTargets( "game_playerkill", ktmp, ktmp, USE_TOGGLE, 0 );
	}
	else if ( ktmp && FBitSet( ktmp->pev->flags, FL_MONSTER ) )
	{
	}
	else
	{  // killed by the world
		pKiller->frags -= 1;
	}

	// update the scores
	// killed scores
	/*MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX(pVictim->edict()) );
		WRITE_SHORT( pVictim->pev->frags );
		WRITE_SHORT( pVictim->m_iDeaths );
	MESSAGE_END();

	// killers score, if it's a player
	CBaseEntity *ep = CBaseEntity::Instance( pKiller );
	if ( ep && ep->Classify() == CLASS_PLAYER )
	{
		CBasePlayer *PK = (CBasePlayer*)ep;

		MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
			WRITE_BYTE( ENTINDEX(PK->edict()) );
			WRITE_SHORT( PK->pev->frags );
			WRITE_SHORT( PK->m_iDeaths );
		MESSAGE_END();

		// let the killer paint another decal as soon as he'd like.
		PK->m_flNextDecalTime = gpGlobals->time;
	}*/

}

//=========================================================
// Deathnotice. 
//=========================================================
void CHalfLifeMultiplay::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor )
{
	// Work out what killed the player, and send a message to all clients about it
	CBaseEntity *Killer = CBaseEntity::Instance( pKiller );

	const char *killer_weapon_name = "world";		// by default, the player is killed by the world
	int killer_index = 0;
	
	// Hack to fix name change
	char *tau = "tau_cannon";
	char *gluon = "gluon gun";

	if ( pKiller->flags & FL_CLIENT )
	{
		killer_index = ENTINDEX(ENT(pKiller));
		
		if ( pevInflictor )
		{
			if ( pevInflictor == pKiller )
			{
				// If the inflictor is the killer,  then it must be their current weapon doing the damage
				CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance( pKiller );
				
				if ( pPlayer->m_pActiveItem )
				{
					killer_weapon_name = pPlayer->m_pActiveItem->pszName();
				}
			}
			else
			{
				killer_weapon_name = STRING( pevInflictor->classname );  // it's just that easy
			}
		}
	}
	else
	{
		killer_weapon_name = STRING( pevInflictor->classname );
	}

	// strip the monster_* or weapon_* from the inflictor's classname
	if ( strncmp( killer_weapon_name, "weapon_", 7 ) == 0 )
		killer_weapon_name += 7;
	else if ( strncmp( killer_weapon_name, "monster_", 8 ) == 0 )
		killer_weapon_name += 8;
	else if ( strncmp( killer_weapon_name, "func_", 5 ) == 0 )
		killer_weapon_name += 5;

	MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
		WRITE_BYTE( killer_index );						// the killer
		WRITE_BYTE( ENTINDEX(pVictim->edict()) );		// the victim
		WRITE_STRING_LIMIT(killer_weapon_name, 128);		// what they were killed by (should this be a string?)
	MESSAGE_END();

	// replace the code names with the 'real' names
	if ( !strcmp( killer_weapon_name, "egon" ) )
		killer_weapon_name = gluon;
	else if ( !strcmp( killer_weapon_name, "gauss" ) )
		killer_weapon_name = tau;

	if ( pVictim->pev == pKiller )  
	{  // killed self
		UTIL_LogPrintf( "\"%s<%i>\" killed self with %s\n",  STRING( pVictim->pev->netname ), GETPLAYERUSERID( pVictim->edict() ), killer_weapon_name );
	}
	else if ( pKiller->flags & FL_CLIENT )
	{
		UTIL_LogPrintf( "\"%s<%i>\" killed \"%s<%i>\" with %s\n",  STRING( pKiller->netname ),
			GETPLAYERUSERID( ENT(pKiller) ),
			STRING( pVictim->pev->netname ),
			GETPLAYERUSERID( pVictim->edict() ),
			killer_weapon_name );
	}
	else
	{  // killed by the world
		UTIL_LogPrintf( "\"%s<%i>\" killed by world with %s\n",  STRING( pVictim->pev->netname ), GETPLAYERUSERID( pVictim->edict() ), killer_weapon_name );
	}

//  Print a standard message
	// TODO: make this go direct to console
	return; // just remove for now
/*
	char	szText[ 128 ];

	if ( pKiller->flags & FL_MONSTER )
	{
		// killed by a monster
		 strncpy(strcpy(szText,  STRING( pVictim->pev->netname ), sizeof(strcpy(szText) );
		strcat ( szText, " was killed by a monster.\n" );
		return;
	}

	if ( pKiller == pVictim->pev )
	{
		 strncpy(strcpy(szText,  STRING( pVictim->pev->netname ), sizeof(strcpy(szText) );
		strcat ( szText, " commited suicide.\n" );
	}
	else if ( pKiller->flags & FL_CLIENT )
	{
		 strncpy(strcpy(szText,  STRING( pKiller->netname ), sizeof(strcpy(szText) );

		strcat( szText, " : " );
		strcat( szText, killer_weapon_name );
		strcat( szText, " : " );

		strcat ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, "\n" );
	}
	else if ( FClassnameIs ( pKiller, "worldspawn" ) )
	{
		 strncpy(strcpy(szText,  STRING( pVictim->pev->netname ), sizeof(strcpy(szText) );
		strcat ( szText, " fell or drowned or something.\n" );
	}
	else if ( pKiller->solid == SOLID_BSP )
	{
		 strncpy(strcpy(szText,  STRING( pVictim->pev->netname ), sizeof(strcpy(szText) );
		strcat ( szText, " was mooshed.\n" );
	}
	else
	{
		 strncpy(strcpy(szText,  STRING( pVictim->pev->netname ), sizeof(strcpy(szText) );
		strcat ( szText, " died mysteriously.\n" );
	}

	UTIL_ClientPrintAll( szText );
*/
}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CHalfLifeMultiplay::PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHalfLifeMultiplay::FlWeaponRespawnTime( CBasePlayerItem *pWeapon )
{
	if ( CVAR_GET_FLOAT("mp_weaponstay") > 0 )
	{
		// make sure it's only certain weapons
		if ( !(pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD) )
		{
			return gpGlobals->time + 0;		// weapon respawns almost instantly
		}
	}

	return gpGlobals->time + WEAPON_RESPAWN_TIME;
}

// when we are within this close to running out of entities,  items 
// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
#define ENTITY_INTOLERANCE 100

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHalfLifeMultiplay::FlWeaponTryRespawn( CBasePlayerItem *pWeapon )
{
	if ( pWeapon && pWeapon->m_iId && (pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD) )
	{
		if ( NUMBER_OF_ENTITIES() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE) )
			return 0;

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime( pWeapon );
	}

	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeMultiplay::VecWeaponRespawnSpot( CBasePlayerItem *pWeapon )
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHalfLifeMultiplay::WeaponShouldRespawn( CBasePlayerItem *pWeapon )
{
	if ( pWeapon->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_WEAPON_RESPAWN_NO;
	}

	return GR_WEAPON_RESPAWN_YES;
}

//=========================================================
// CanHaveWeapon - returns FALSE if the player is not allowed
// to pick up this weapon
//=========================================================
/*BOOL CHalfLifeMultiplay::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pItem )
{
	if ( CVAR_GET_FLOAT("mp_weaponstay") > 0 )
	{
		if ( pItem->iFlags() & ITEM_FLAG_LIMITINWORLD )
			return CGameRules::CanHavePlayerItem( pPlayer, pItem );

		// check if the player already has this weapon
		for ( int i = 0 ; i < MAX_ITEM_TYPES ; i++ )
		{
			CBasePlayerItem *it = pPlayer->m_rgpPlayerItems[i];

			while ( it != NULL )
			{
				if ( it->m_iId == pItem->m_iId )
				{
					return FALSE;
				}

				it = it->m_pNext;
			}
		}
	}

	return CGameRules::CanHavePlayerItem( pPlayer, pItem );
}*/

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
{
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::ItemShouldRespawn( CItem *pItem )
{
	if ( pItem->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_ITEM_RESPAWN_NO;
	}

	return GR_ITEM_RESPAWN_YES;
}


//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHalfLifeMultiplay::FlItemRespawnTime( CItem *pItem )
{
	return gpGlobals->time + ITEM_RESPAWN_TIME;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeMultiplay::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount )
{
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsAllowedToSpawn( CBaseEntity *pEntity )
{
	return TRUE;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::AmmoShouldRespawn( CBasePlayerAmmo *pAmmo )
{
	if ( pAmmo->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_AMMO_RESPAWN_NO;
	}

	return GR_AMMO_RESPAWN_YES;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo )
{
	return gpGlobals->time + AMMO_RESPAWN_TIME;
}

//=========================================================
//=========================================================
Vector CHalfLifeMultiplay::VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo )
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlHealthChargerRechargeTime( void )
{
	return 60;
}


float CHalfLifeMultiplay::FlHEVChargerRechargeTime( void )
{
	return 30;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_GUN_ACTIVE;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_AMMO_ACTIVE;
}

edict_t *CHalfLifeMultiplay::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	return NULL;
}


//=========================================================
//=========================================================
int CHalfLifeMultiplay::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// half life deathmatch has only enemies
	return GR_NOTTEAMMATE;
}

BOOL CHalfLifeMultiplay::PlayFootstepSounds( CBasePlayer *pl, float fvol )
{
	if ( g_footsteps && g_footsteps->value == 0 )
		return FALSE;

	if ( pl->IsOnLadder() || pl->pev->velocity.Length2D() > 220 )
		return TRUE;  // only make step sounds in multiplayer if the player is moving fast enough

	return FALSE;
}

BOOL CHalfLifeMultiplay::FAllowFlashlight( void ) 
{ 
	return FALSE; 
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::FAllowMonsters( void )
{
	return TRUE;
	//return ( CVAR_GET_FLOAT( "mp_allowmonsters" ) != 0 );
}

//=========================================================
//======== CHalfLifeMultiplay private functions ===========
#define INTERMISSION_TIME 6

void CHalfLifeMultiplay::GoToIntermission( void )
{
	if ( g_fGameOver )
		return;  // intermission has already been triggered, so ignore.

	MESSAGE_BEGIN(MSG_ALL, SVC_INTERMISSION);
	MESSAGE_END();

	m_flIntermissionEndTime = gpGlobals->time + INTERMISSION_TIME;
	g_fGameOver = TRUE;
	m_iEndIntermissionButtonHit = FALSE;
}

#define MAX_RULE_BUFFER 1024

typedef struct mapcycle_item_s
{
	struct mapcycle_item_s *next;

	char mapname[ 32 ];
	int  minplayers, maxplayers;
	char rulebuffer[ MAX_RULE_BUFFER ];
} mapcycle_item_t;

typedef struct mapcycle_s
{
	struct mapcycle_item_s *items;
	struct mapcycle_item_s *next_item;
} mapcycle_t;

/*
==============
DestroyMapCycle

Clean up memory used by mapcycle when switching it
==============
*/
void DestroyMapCycle( mapcycle_t *cycle )
{
	mapcycle_item_t *p, *n, *start;
	p = cycle->items;
	if ( p )
	{
		start = p;
		p = p->next;
		while ( p != start )
		{
			n = p->next;
			delete p;
			p = n;
		}
		
		delete cycle->items;
	}
	cycle->items = NULL;
	cycle->next_item = NULL;
}

static char com_token[ 1500 ];

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *COM_Parse (char *data)
{
	int             c;
	int             len;
	
	len = 0;
	com_token[0] = 0;
	
	if (!data)
		return NULL;
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;                    // end of file;
		data++;
	}
	
// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
	

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
	}

// parse single characters
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
			break;
	} while (c>32);
	
	com_token[len] = 0;
	return data;
}

/*
==============
COM_TokenWaiting

Returns 1 if additional data is waiting to be processed on this line
==============
*/
int COM_TokenWaiting( char *buffer )
{
	char *p;

	p = buffer;
	while ( *p && *p!='\n')
	{
		if ( !isspace( *p ) || isalnum( *p ) )
			return 1;

		p++;
	}

	return 0;
}

/*
==============
ReloadMapCycleFile


Parses mapcycle.txt file into mapcycle_t structure
==============
*/
int ReloadMapCycleFile( char *filename, mapcycle_t *cycle )
{
	char szBuffer[ MAX_RULE_BUFFER ];
	char szMap[ 32 ];
	int length;
	char *pFileList;
	char *aFileList = pFileList = (char*)LOAD_FILE_FOR_ME( filename, &length );
	int hasbuffer;

	mapcycle_item_s *item, *newlist = NULL, *next;

	if ( pFileList && length )
	{
		// the first map name in the file becomes the default
		while ( 1 )
		{
			hasbuffer = 0;
			memset( szBuffer, 0, MAX_RULE_BUFFER );

			pFileList = COM_Parse( pFileList );
			if ( strlen( com_token ) <= 0 )
				break;

			 strncpy(szMap,  com_token, sizeof(szMap) );

			// Any more tokens on this line?
			if ( COM_TokenWaiting( pFileList ) )
			{
				pFileList = COM_Parse( pFileList );
				if ( strlen( com_token ) > 0 )
				{
					hasbuffer = 1;
					 strncpy(szBuffer,  com_token, sizeof(szBuffer) );
				}
			}

			// Check map
			if ( IS_MAP_VALID( szMap ) )
			{
				// Create entry
				char *s;

				item = new mapcycle_item_s;

				 strncpy(item->mapname,  szMap, sizeof(item->mapname) );

				item->minplayers = 0;
				item->maxplayers = 0;

				memset( item->rulebuffer, 0, MAX_RULE_BUFFER );

				if ( hasbuffer )
				{
					s = g_engfuncs.pfnInfoKeyValue( szBuffer, "minplayers" );
					if ( s && s[0] )
					{
						item->minplayers = atoi( s );
						item->minplayers = V_max( item->minplayers, 0 );
						item->minplayers = V_min( item->minplayers, gpGlobals->maxClients );
					}
					s = g_engfuncs.pfnInfoKeyValue( szBuffer, "maxplayers" );
					if ( s && s[0] )
					{
						item->maxplayers = atoi( s );
						item->maxplayers = V_max( item->maxplayers, 0 );
						item->maxplayers = V_min( item->maxplayers, gpGlobals->maxClients );
					}

					// Remove keys
					//
					g_engfuncs.pfnInfo_RemoveKey( szBuffer, "minplayers" );
					g_engfuncs.pfnInfo_RemoveKey( szBuffer, "maxplayers" );

					 strncpy(item->rulebuffer,  szBuffer, sizeof(item->rulebuffer) );
				}

				item->next = cycle->items;
				cycle->items = item;
			}
			else
			{
				ALERT( at_console, "Skipping %s from mapcycle, not a valid map\n", szMap );
			}


		}

		FREE_FILE( aFileList );
	}

	// Fixup circular list pointer
	item = cycle->items;

	// Reverse it to get original order
	while ( item )
	{
		next = item->next;
		item->next = newlist;
		newlist = item;
		item = next;
	}
	cycle->items = newlist;
	item = cycle->items;

	// Didn't parse anything
	if ( !item )
	{
		return 0;
	}

	while ( item->next )
	{
		item = item->next;
	}
	item->next = cycle->items;
	
	cycle->next_item = item->next;

	return 1;
}

/*
==============
CountPlayers

Determine the current # of active players on the server for map cycling logic
==============
*/

/*
int CountPlayers( void )
{
	//Thothie NOV2014_07 - this function now checks AFK/bot
	int	num = 0;
	bool bvalid_player;

	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pEntity = UTIL_PlayerByIndex( i );

		if( pEntity )
		{
			IScripted *iScripted = pEntity->GetScripted();
			bvalid_player = false;
			if ( iScripted )
			{
				bvalid_player = (atoi( iScripted->GetFirstScriptVar("IS_AFK")) == 1) ? false : true;
				if ( bvalid_player ) bvalid_player = (atoi( iScripted->GetFirstScriptVar("PLR_IN_WORLD")) == 1) ? true : false;
			}
			if ( bvalid_player ) num = num + 1;
		}
	}
	return num;
}
*/

mslist<msstring> g_BanList;

void BanID( const msstring &SteamID )
{
	g_BanList.push_back( SteamID );
}
void UnBanAll( )
{
	g_BanList.clear( );
}
bool CheckBanned( msstring_ref SteamID )
{
	if( g_BanList.size() <= 0 ) return false;

	 for (int b = 0; b < g_BanList.size(); b++) 
		if( g_BanList[b] == (const char *)SteamID )
			return true;

	return false;
}

/*
==============
ExtractCommandString

Parse commands/key value pairs to issue right after map xxx command is issued on server
 level transition
==============
*/
void ExtractCommandString( char *s, char *szCommand )
{
	// Now make rules happen
	char	pkey[512];
	char	value[512];	// use two buffers so compares
								// work without stomping on each other
	char	*o;
	
	if ( *s == '\\' )
		s++;

	while (1)
	{
		o = pkey;
		while ( *s != '\\' )
		{
			if ( !*s )
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;

		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		strcat( szCommand, pkey );
		if ( strlen( value ) > 0 )
		{
			strcat( szCommand, " " );
			strcat( szCommand, value );
		}
		strcat( szCommand, "\n" );

		if (!*s)
			return;
		s++;
	}
}

/*
==============
ChangeLevel

Server is changing to a new level, check mapcycle.txt for map name and setup info
==============
*/
void CHalfLifeMultiplay :: ChangeLevel( void )
{
	static char szPreviousMapCycleFile[ 256 ];
	static mapcycle_t mapcycle;

	char szNextMap[32];
	char szFirstMapInList[32];
	char szCommands[ 1500 ];
	char szRules[ 1500 ];
	int minplayers = 0, maxplayers = 0;
	strncpy(szFirstMapInList, "edana", sizeof(szFirstMapInList));  // the absolute default level is edana

	int	curplayers;
	BOOL do_cycle = TRUE;

	// find the map to change to
	char *mapcfile = (char*)CVAR_GET_STRING( "mapcyclefile" );
	ASSERT( mapcfile != NULL );

	szCommands[ 0 ] = '\0';
	szRules[ 0 ] = '\0';

	curplayers = UTIL_NumPlayers();

	// Has the map cycle filename changed?
	if ( _stricmp( mapcfile, szPreviousMapCycleFile ) )
	{
		 strncpy(szPreviousMapCycleFile,  mapcfile, sizeof(szPreviousMapCycleFile) );

		DestroyMapCycle( &mapcycle );

		if ( !ReloadMapCycleFile( mapcfile, &mapcycle ) || ( !mapcycle.items ) )
		{
			ALERT( at_console, "Unable to load map cycle file %s\n", mapcfile );
			do_cycle = FALSE;
		}
	}

	if ( do_cycle && mapcycle.items )
	{
		BOOL keeplooking = FALSE;
		BOOL found = FALSE;
		mapcycle_item_s *item;

		// Assume current map
		 strncpy(szNextMap,  STRING(gpGlobals->mapname), sizeof(szNextMap) );
		 strncpy(szFirstMapInList,  STRING(gpGlobals->mapname), sizeof(szFirstMapInList) );

		// Traverse list
		for ( item = mapcycle.next_item; item->next != mapcycle.next_item; item = item->next )
		{
			keeplooking = FALSE;

			ASSERT( item != NULL );

			if ( item->minplayers != 0 )
			{
				if ( curplayers >= item->minplayers )
				{
					found = TRUE;
					minplayers = item->minplayers;
				}
				else
				{
					keeplooking = TRUE;
				}
			}

			if ( item->maxplayers != 0 )
			{
				if ( curplayers <= item->maxplayers )
				{
					found = TRUE;
					maxplayers = item->maxplayers;
				}
				else
				{
					keeplooking = TRUE;
				}
			}

			if ( keeplooking )
				continue;

			found = TRUE;
			break;
		}

		if ( !found )
		{
			item = mapcycle.next_item;
		}			
		
		// Increment next item pointer
		mapcycle.next_item = item->next;

		// Perform logic on current item
		 strncpy(szNextMap,  item->mapname, sizeof(szNextMap) );

		ExtractCommandString( item->rulebuffer, szCommands );
		 strncpy(szRules,  item->rulebuffer, sizeof(szRules) );
	}

	if ( !IS_MAP_VALID(szNextMap) )
	{
		 strncpy(szNextMap,  szFirstMapInList, sizeof(szNextMap) );
	}

	g_fGameOver = TRUE;

	ALERT( at_console, "CHANGE LEVEL: %s\n", szNextMap );
	if ( minplayers || maxplayers )
	{
		ALERT( at_console, "PLAYER COUNT:  min %i max %i current %i\n", minplayers, maxplayers, curplayers );
	}
	if ( strlen( szRules ) > 0 )
	{
		ALERT( at_console, "RULES:  %s\n", szRules );
	}
	
	CHANGE_LEVEL( szNextMap, NULL );
	if ( strlen( szCommands ) > 0 )
	{
		SERVER_COMMAND( szCommands );
	}
}

#define MAX_MOTD_CHUNK	  60
#define MAX_MOTD_LENGTH   (MAX_MOTD_CHUNK * 4)

void CHalfLifeMultiplay :: SendMOTDToClient( edict_t *client )
{
	// read from the MOTD.txt file
	int length, char_count = 0;
	char *pFileList;
	char *aFileList = pFileList = (char*)LOAD_FILE_FOR_ME( (char *)CVAR_GET_STRING( "motdfile" ), &length );

	// send the server name
	/*MESSAGE_BEGIN( MSG_ONE, gmsgServerName, NULL, client );
		WRITE_STRING( CVAR_GET_STRING("hostname") );
	MESSAGE_END();*/

	// Send the message of the day
	// read it chunk-by-chunk,  and send it in parts

	while ( pFileList && *pFileList && char_count < MAX_MOTD_LENGTH )
	{
		char chunk[MAX_MOTD_CHUNK+1];
		
		if ( strlen( pFileList ) < MAX_MOTD_CHUNK )
		{
			 strncpy(chunk,  pFileList, sizeof(chunk) );
		}
		else
		{
			strncpy( chunk, pFileList, MAX_MOTD_CHUNK );
			chunk[MAX_MOTD_CHUNK] = 0;		// strncpy doesn't always append the null terminator
		}

		char_count += strlen( chunk );
		if ( char_count < MAX_MOTD_LENGTH )
			pFileList = aFileList + char_count; 
		else
			*pFileList = 0;

		MESSAGE_BEGIN( MSG_ONE, gmsgMOTD, NULL, client );
			WRITE_BYTE( *pFileList ? FALSE : TRUE );	// FALSE means there is still more message to come
			WRITE_STRING_LIMIT(chunk, sizeof(chunk));
		MESSAGE_END();
	}

	FREE_FILE( aFileList );
}
	
//Master Sword ----------------------------------------------------
//=========================================================
// ClientCommand
// the user has typed a command which is unrecognized by everything else;
// this check to see if the gamerules knows anything about the command
//=========================================================
BOOL CHalfLifeMultiplay :: ClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{
	if(g_VoiceGameMgr.ClientCommand(pPlayer, pcmd))
		return TRUE;

	int OldMenu;
	if( FStrEq( pcmd, "menuselect" ) )
	{
		if ( CMD_ARGC() < 2 )
			return FALSE;

		int slot = atoi( CMD_ARGV(1) );//Starts at 1

		OldMenu = pPlayer->CurrentMenu;
		pPlayer->ParseMenu( OldMenu, slot );
		//Check if you've set a different menu first
		if( pPlayer->CurrentMenu == OldMenu
			&& !pPlayer->CurrentCallbackMenu ) pPlayer->CurrentMenu = NULL;
		return TRUE;
	}
	else if( FStrEq( pcmd, "forgive" ) )
	{
		if( pPlayer->m_LastPlayerToKillMe > 0 )
		{
			CBasePlayer *pPlayerKiller = (CBasePlayer *)MSInstance(INDEXENT(pPlayer->m_LastPlayerToKillMe));
			if( !pPlayerKiller || pPlayerKiller != pPlayer->m_pLastPlayerToKillMe )
			{
				pPlayer->SendInfoMsg( "The player that killed you has since departed\n" );
			}
			else {
				pPlayer->SendHUDMsg( "Player Kill", msstring("You report your accidental death to the Guard...\n") + pPlayerKiller->DisplayName() + "'s murder charges against you have been dropped.\n" );
				pPlayer->SendHUDMsg( "Player Kill", msstring("*** Your murder charges against ") + pPlayer->DisplayName() + " have been dropped ***\n" );
				//pPlayer->SendInfoMsg( "You report your accidental death to the Guard...\n" );
				//pPlayer->SendInfoMsg( "%s's murder charges against you have been dropped.\n", pPlayerKiller->DisplayName() );
				//pPlayerKiller->SendInfoMsg( "*** Your murder charges against %s have been dropped ***\n", pPlayer->DisplayName() );
				pPlayerKiller->m_PlayersKilled = V_max(pPlayerKiller->m_PlayersKilled-1,0);
				pPlayerKiller->m_TimeWaitedToForgetKill = 0;
			}
			pPlayer->m_LastPlayerToKillMe = 0;
			pPlayer->m_pLastPlayerToKillMe = NULL;
		}
		else {
			pPlayer->SendInfoMsg( "Forgive: Use this command to remove your accidental death from the killer's record\n" );
		}
		return TRUE;
	}
	else if( FStrEq( pcmd, "joinparty" ) )
	{
		if( CMD_ARGC() >= 2 /*&& isalpha(CMD_ARGV(1)[0])*/ && !isspace(CMD_ARGV(1)[0]) )
		{
			if( pPlayer->m_pTeam )
			{
				pPlayer->SendInfoMsg( "You are already in a party!  Type 'leaveparty' to leave it\n" );
				return TRUE;
			}
			//Get the party name
			char Name[MAX_TEAMNAME_LEN+1];
			strncpy( Name, CMD_ARGV(1), MAX_TEAMNAME_LEN );
			Name[MAX_TEAMNAME_LEN] = 0;

			if( !strcmp(Name,"???") ) //Auto-name the party
			{

				/*#define PTYNAME_EXT_MAX 8
				#define PTYNAME_EXT_SML 4
				int iNameLen = pPlayer->DisplayName();
				if( iNameLen + PTYNAME_EXT_MAX <= MAX_TEAMNAME_LEN )
					 _snprintf(Name, sizeof(Name),  "%s's party",  pPlayer->DisplayName() );
				else if( iNameLen + PTYNAME_EXT_SML <= MAX_TEAMNAME_LEN )
					 _snprintf(Name, sizeof(Name),  "%s pty",  pPlayer->DisplayName() );
				else
					_snprintf( Name, sizeof(Name), "pty %s", pPlayer->DisplayName() );*/
				msstring PartyName = msstring(pPlayer->DisplayName()) + "'s party";

				strncpy( Name, PartyName.c_str(), MAX_TEAMNAME_LEN );		//Truncate the final name
				Name[MAX_TEAMNAME_LEN] = 0;
			}

			//Check for a current party of this name
			CTeam *pTeam = CTeam::GetTeam( Name );
			if( pTeam )
			{
				pTeam->ValidateUnits( );
				//Request entry into the party
				CBasePlayer *pLeader = (CBasePlayer *)UTIL_PlayerByIndex( pTeam->MemberList[0].idx );
				if( pLeader )
				{
					pLeader->SendHUDMsg( "Party", msstring(pPlayer->DisplayName()) + " wants to join your party.\nType 'accept' to allow " + pPlayer->DisplayName() + " into your party" );
					//pLeader->SendInfoMsg( "%s wants to join your party. Type 'accept' to allow %s into your party\n", pPlayer->DisplayName(), STRING(pPlayer->DisplayName) );
					pPlayer->m_pJoinTeam = pTeam;
					pPlayer->SendInfoMsg( "The party leader must now accept your request\n" );
				}
			}
			else {
				pTeam = CTeam::CreateTeam( Name, RANDOM_LONG(0, LONG_MAX) );
				//pPlayer->SendInfoMsg( "You are now the leader of %s\n", Name );
				msstring CreateMsg = msstring("You are now the leader of ") + pTeam->m_TeamName;
				pPlayer->SendHUDMsg( "Party", CreateMsg );
				pPlayer->SendInfoMsg("%s", CreateMsg.c_str());
				pPlayer->SetTeam( pTeam );
			}
		}
		return TRUE;
	}
	else if( FStrEq( pcmd, "leaveparty" ) )
	{
		if( pPlayer->m_pTeam )
		{
			CTeam *pTeam = pPlayer->m_pTeam;
			pTeam->ValidateUnits( );
			pPlayer->SendInfoMsg( "You leave the %s party\n", pTeam->TeamName() );
			int iTeamPlayers = pTeam->MemberList.size();
			if( iTeamPlayers )
				 for (int i = 0; i < pTeam->MemberList.size(); i++) 
				{
					CBasePlayer *pOtherPlayer = pTeam->GetPlayer( i );
					//Send to all other players, but not myself
					if( !pOtherPlayer || pOtherPlayer == pPlayer )
						continue;
					msstring LeaveMsg = msstring(pPlayer->DisplayName()) + " left your party";
					pOtherPlayer->SendHUDMsg( "Party", LeaveMsg );
					pOtherPlayer->SendInfoMsg("%s", LeaveMsg.c_str());
					//pOtherPlayer->SendInfoMsg( "%s left your party\n", STRING(pPlayer->DisplayName) );
				}
			pPlayer->SetTeam( NULL ); //Deletes Team if no players left
		}
		else {
			pPlayer->SendInfoMsg( "You aren't in a party!\n" );
		}
		return TRUE;
	}
	else if (FStrEq(pcmd, "startvote") && CMD_ARGC() >= 3 )
	{
		if( FStrEq(CMD_ARGV(1),"kick") )
		{
			if( CVAR_GET_FLOAT("ms_allowkickvote") )
				StartVote( pPlayer, CMD_ARGV(1), CMD_ARGV(2) );
			else 
				pPlayer->SendInfoMsg( "This server doesn't allow kick voting\n" );
		}
		else if( FStrEq(CMD_ARGV(1),"advtime") )
		{
			if( CVAR_GET_FLOAT("ms_allowtimevote") )
				StartVote( pPlayer, CMD_ARGV(1), CMD_ARGV(2) );
			else 
				pPlayer->SendInfoMsg( "This server doesn't allow time voting\n" );
		}
		else return FALSE;
		return TRUE;
	}
	else if (FStrEq(pcmd, "vote") && CMD_ARGV(1))
	{
		TallyVote( pPlayer, atoi(CMD_ARGV(1)) ? true : false );
		return TRUE;
	}
	else if( FStrEq( pcmd, "accept" ) )
	{
		int AcceptType = 0;
		//Vote to changelevel
		if( !AcceptType )
		{
			if( pPlayer->CurrentTransArea && ((CBaseEntity *)pPlayer->CurrentTransArea)->MSQuery( ENTINDEX(pPlayer->edict()) ) )
				AcceptType = 1;
		}

		//Accept a member into your party
		//Must be the party leader
		if( pPlayer->m_pTeam && pPlayer->m_pTeam->GetPlayer( 0 ) == pPlayer )
		{
			CBasePlayer *pCheckPlayer = NULL;
			for( int n = 1; n <= gpGlobals->maxClients; n++ )
			{
				pCheckPlayer = (CBasePlayer *)UTIL_PlayerByIndex( n );
				if( pCheckPlayer && pCheckPlayer->m_pJoinTeam == pPlayer->m_pTeam ) 
					break;

				pCheckPlayer = NULL;
			}

			if( pCheckPlayer )
			{
				AcceptType = 2;

				CTeam *pTeam = pPlayer->m_pTeam;

				msstring JoinMsg = msstring(pCheckPlayer->DisplayName()) + " joins your party";
				pTeam->ValidateUnits( );
				 for (int i = 0; i < pTeam->MemberList.size(); i++) 
				{
					CBasePlayer *pOtherPlayer = pTeam->GetPlayer( i );
					//if( pOtherPlayer ) pOtherPlayer->SendInfoMsg( "%s joins your party\n", STRING(pCheckPlayer->DisplayName) );
					if( pOtherPlayer )
					{
						pOtherPlayer->SendHUDMsg( "Party", JoinMsg );
						pOtherPlayer->SendInfoMsg( "%s", JoinMsg.c_str());
					}

				}

				pCheckPlayer->SetTeam( pTeam );
//				pCheckPlayer->SendInfoMsg( "%s accepted your request, you join the %s party.\n", STRING(pPlayer->DisplayName), pTeam->TeamName() );
				pCheckPlayer->SendHUDMsg( "Party", msstring(pPlayer->DisplayName()) + " accepted your request.\nYou join the " + pTeam->TeamName() + " party." );
				pCheckPlayer->SendInfoMsg("You join the %s party.", pTeam->TeamName());
				pCheckPlayer->m_pJoinTeam = NULL;
			}
		}

		//Accept an offer
		if( !AcceptType )
		{
			if( pPlayer->m_OfferInfo.SrcMonsterIDX )
			{
				pPlayer->AcceptOffer( );
				AcceptType = 3;
			}
		}
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// ClientUserInfoChanged
//=========================================================

//#define PLAYERMODEL_HUMAN_MALE1 "../../human/male1/male1"
#define PLAYERMODEL_HUMAN_MALE1 ""

void CHalfLifeMultiplay::ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer )
{
	// prevent skin/color/model changes
	char *mdls = g_engfuncs.pfnInfoKeyValue( infobuffer, "model" );

	if ( _stricmp( mdls, PLAYERMODEL_HUMAN_MALE1 ) )
	{
		g_engfuncs.pfnSetClientKeyValue( pPlayer->entindex(), g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", PLAYERMODEL_HUMAN_MALE1 );
		//_snprintf( text, sizeof(text), "* Unkwnown forces prevent you from shapeshifting...\n" );
		//UTIL_SayText( text, pPlayer );
		return;
	}

	//Only allow a name change the first time a client joins
	if( pPlayer->m_DisplayName.len() )	//Name already set.  Keep that name
	{
		g_engfuncs.pfnSetClientKeyValue( pPlayer->entindex(), g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "name", (char *)pPlayer->m_DisplayName );
		//pPlayer->pev->netname = ALLOC_STRING(pPlayer->DisplayName());
		pPlayer->m_NetName = pPlayer->DisplayName(); 
		pPlayer->pev->netname = MAKE_STRING(pPlayer->m_NetName.c_str());
	}
/*
	if( !FStrEq( pPlayer->DisplayName(), g_engfuncs.pfnInfoKeyValue( infobuffer, "name" )) )
		//if Tried to change name
		if( !pPlayer->pev->netname || !STRING(pPlayer->pev->netname)[0] )
			//and name is blank, change
			pPlayer->pev->netname = ALLOC_STRING(g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ));
		else
			//and name exists, keep old name
			g_engfuncs.pfnSetClientKeyValue( pPlayer->entindex(), g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "name", (char *)pPlayer->DisplayName() );
*/
}
//----------------------------------------------------------------

void CHalfLifeMultiplay::EndMultiplayerGame( void )
{
	//Delete all the teams
	 for (int i = 0; i < CTeam::Teams.size(); i++) 
		delete CTeam::Teams[0];
	CTeam::Teams.clear();
	
	m_CurrentVote.fActive = false;
}

void CHalfLifeMultiplay	:: StartVote( CBasePlayer *pPlayer, msstring VoteType, msstring VoteInfo )
{
	//Vote already in progress?
	if( m_CurrentVote.fActive )
		return;

	//Tried to vote too often?
	if( gpGlobals->time < pPlayer->m_TimeCanVote )
	{
		pPlayer->SendInfoMsg( "You cannot vote again for %i seconds\n", int(pPlayer->m_TimeCanVote - gpGlobals->time) );
		return;
	}

	msstring SendVoteInfo;

	//Validate vote.  Must have enough information to start
	CBasePlayer *TargetPlayer = NULL;
	if( VoteType == "kick" )
	{
		//Target player doesn't exist?
		TargetPlayer = (CBasePlayer *)MSInstance(INDEXENT(atoi(VoteInfo) + 1));		//+1 to get a 1-based player index
		if( !TargetPlayer )
			return;
	}


	//Initiate vote
	m_CurrentVote.fActive = true;
	m_CurrentVote.Type = VoteType;
	m_CurrentVote.SourcePlayer = pPlayer->entindex();
	m_CurrentVote.TargetPlayer = 0;
	m_CurrentVote.TargetPlayerWonID = "";
	m_CurrentVote.TargetPlayerName = "";
	m_CurrentVote.EndTime = gpGlobals->time + 30.0f;
	m_CurrentVote.VoteTally = 0;
	m_CurrentVote.Info = VoteInfo;

	SendVoteInfo = msstring() + (int)m_CurrentVote.SourcePlayer;

	//Set up vote info to send, based on vote type
	if( VoteType == "kick" )
	{
		m_CurrentVote.TargetPlayer = TargetPlayer->entindex();
		m_CurrentVote.TargetPlayerWonID = GETPLAYERAUTHID( TargetPlayer->edict() );
		m_CurrentVote.TargetPlayerName = TargetPlayer->DisplayName();

		SendVoteInfo += msstring(";") + (int)m_CurrentVote.TargetPlayer;
	}
	else if( VoteType == "advtime" )
	{
		 SendVoteInfo += msstring(";") + VoteInfo;
	}


	TallyVote( pPlayer, true ); // Tally this player's vote

	//Send vote to all players
	MESSAGE_BEGIN( MSG_ALL, g_netmsg[NETMSG_VOTE], NULL );
		WRITE_BYTE( 1 ); //Vote has started
		WRITE_STRING_LIMIT(m_CurrentVote.Type + (SendVoteInfo.len() ? msstring(";") + SendVoteInfo : ""), 128);
	MESSAGE_END();

	msstringlist Params;
	Params.add( m_CurrentVote.Type );
	Params.add( m_CurrentVote.Type );

	if( MSGlobals::GameScript )
		MSGlobals::GameScript->CallScriptEvent( "game_vote_start", &Params );

	pPlayer->m_TimeCanVote = m_CurrentVote.EndTime + 10.0f;
}

void CHalfLifeMultiplay	:: TallyVote( CBasePlayer *pPlayer, bool fYesVote )
{
	//Is a vote in progress?
	if( !m_CurrentVote.fActive )
		return;

	//Did this player already vote?
	//Also, don't count 'no' votes
	byte PlayerBit = (1 << (pPlayer->entindex()-1));
	if( !fYesVote || FBitSet(m_CurrentVote.VoteTally,PlayerBit) )
		return;

	SetBits( m_CurrentVote.VoteTally, PlayerBit );

	MESSAGE_BEGIN( MSG_BROADCAST, g_netmsg[NETMSG_VOTE], NULL );
		WRITE_BYTE( 2 ); //Tell who voted yes
		WRITE_LONG( m_CurrentVote.VoteTally );
	MESSAGE_END();
}
void CHalfLifeMultiplay	:: UpdateVote( )
{
	//Is a vote in progress?
	if( !m_CurrentVote.fActive )
		return;
	
	//Vote time is up, check the results
	
	//Count SPAWNED players (Connecting players don't count)
	int	iTotalPlayers = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
		if( pPlayer && pPlayer->m_fGameHUDInitialized )
			iTotalPlayers++;
	}

	if( gpGlobals->time < m_CurrentVote.EndTime 
		&& iTotalPlayers > 1 )					//If only one player is on the server, finish the vote immediately
		return;

	//Count votes
	int iYesVotes = 0;
	for(int i = 0; i < gpGlobals->maxClients; i++ )
	{
		if( FBitSet( m_CurrentVote.VoteTally,(1<<i) ) )
			iYesVotes++;
	}

	//If yes ratio is over 50%, take action
	msstringlist Params;
	Params.add( m_CurrentVote.Type );

	CBaseEntity *pSourcePlayer = MSInstance( INDEXENT(m_CurrentVote.SourcePlayer) );
	Params.add( pSourcePlayer ? EntToString(pSourcePlayer) : "<unknown>" );

	if( iYesVotes / (float)iTotalPlayers > 0.50 )
	{
		Params.add( "success" );

		if( m_CurrentVote.Type == "kick" )
		{
			SendHUDMsgAll( "Player Kick", m_CurrentVote.TargetPlayerName + " was kicked by vote!" );

			CBaseEntity *pTargetPlayer = CBaseEntity::Instance( INDEXENT(m_CurrentVote.TargetPlayer) );
			//Check to make sure the player is still here and that it's the same player.  If the player leaves
			//and a new player joins, don't kick/ban the new player.
			if( pTargetPlayer && !strcmp(GETPLAYERAUTHID(pTargetPlayer->edict()),m_CurrentVote.TargetPlayerWonID) )
			{
				Params.add( EntToString(pTargetPlayer) );

				SERVER_COMMAND( UTIL_VarArgs("kick #%s\n", m_CurrentVote.TargetPlayerWonID.c_str()) );
			}
			else
				Params.add( "unknown" );

			//But do ban the STEAM ID regardless
			BanID( m_CurrentVote.TargetPlayerWonID );
		}
		else if( m_CurrentVote.Type == "advtime" )
		{
			Params.add( m_CurrentVote.Info );
		}
	}
	else
	{
		Params.add( "failed" );
		//UTIL_ClientPrintAll( HUD_PRINTCENTER, "Vote Failed!\n" );
		msstring VoteTitle = "Player Vote";
		if( m_CurrentVote.Type == "kick" ) VoteTitle = "Player Kick";
		else VoteTitle = "Advance Time";

		SendHUDMsgAll( VoteTitle, "Vote Failed!" );
	}
 
	if( MSGlobals::GameScript )
		MSGlobals::GameScript->CallScriptEvent( "game_vote_end", &Params );

	//Turn off the vote
	m_CurrentVote.fActive = false;

	MESSAGE_BEGIN( MSG_ALL, g_netmsg[NETMSG_VOTE], NULL );
		WRITE_BYTE( 0 ); //Vote has ended
	MESSAGE_END();
}
