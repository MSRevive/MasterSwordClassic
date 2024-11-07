//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// Client side entity management functions

#include <memory.h>

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_types.h"
#include "studio_event.h" // def. of mstudioevent_t
#include "r_efx.h"
#include "event_api.h"
#include "pm_defs.h"
//#include "pmtrace.h"
#include "pm_shared.h"
#include "com_model.h"
#undef CACHE_SIZE

//Master Sword
#include "inc_weapondefs.h"
#include "ms/hudscript.h"
#include "ms/clglobal.h"
#include "script.h"
#include "logger.h"

#undef DLLEXPORT //Master Sword
#define DLLEXPORT EXPORT
#define MAX_TEMPENT_EXTRA 4096

void Game_AddObjects(void);
void SetClEntityProp(cl_entity_t &Ent, msstring &Cmd, mslist<msstring *> &ValueParams);

int g_iAlive = 1;

mslist<BEAM *> m_Beams; //DEC2014_09 Thothie - beam_update

extern "C"
{
	int DLLEXPORT HUD_AddEntity(int type, struct cl_entity_s *ent, const char *modelname);
	void DLLEXPORT HUD_CreateEntities(void);
	void DLLEXPORT HUD_StudioEvent(const struct mstudioevent_s *event, const struct cl_entity_s *entity);
	void DLLEXPORT HUD_TxferLocalOverrides(struct entity_state_s *state, const struct clientdata_s *client);
	void DLLEXPORT HUD_ProcessPlayerState(struct entity_state_s *dst, const struct entity_state_s *src);
	void DLLEXPORT HUD_TxferPredictionData(struct entity_state_s *ps, const struct entity_state_s *pps, struct clientdata_s *pcd, const struct clientdata_s *ppcd, struct weapon_data_s *wd, const struct weapon_data_s *pwd);
	void DLLEXPORT HUD_TempEntUpdate(double frametime, double client_time, double cl_gravity, struct tempent_s **ppTempEntFree, struct tempent_s **ppTempEntActive, int (*Callback_AddVisibleEntity)(struct cl_entity_s *pEntity), void (*Callback_TempEntPlaySound)(struct tempent_s *pTemp, float damp));
	struct cl_entity_s DLLEXPORT *HUD_GetUserEntity(int index);
}

struct tempentextra_t
{
	bool Active;
	bool CBTimer_Enabled; //Timer callback
	float CBTimer_TimeCallback;
	msstring CBTimer_CallbackEvent;

	bool CBCollide_Enabled; //Collision callback
	msstring CBCollide_CallbackEvent;

	bool CBWater_Enabled; //Water collision callback
	msstring CBWater_CallbackEvent;

	bool Fadeout;
	float FadeDuration;
	float FadeStart;

	bool DieWithEntActive; //When the other ent ceases to exist, delete me
	int DieWithEnt;
};

static tempentextra_t g_TempEntExtra[MAX_TEMPENT_EXTRA]; //Extra info about tempents
extern bool g_TempEntNewLevel;

/*
========================
HUD_AddEntity
	Return 0 to filter entity from visible list for rendering
========================
*/
int DLLEXPORT HUD_AddEntity(int type, struct cl_entity_s *ent, const char *modelname)
{
	DBG_INPUT;
	startdbg;

	dbg("Begin");
	/*switch ( type )
	{
	case ET_NORMAL:
	case ET_PLAYER:
	case ET_BEAM:
	case ET_TEMPENTITY:
	case ET_FRAGMENTED:
	default:
		break;
	}*/
	// each frame every entity passes this function, so the overview hooks it to filter the overview entities
	// in spectator mode:
	// each frame every entity passes this function, so the overview hooks
	// it to filter the overview entities

	//Master Sword - Store the other players' pitch for later.  I need it to rotate the back bones (for look up/down),
	//				 but I'm going to have to set angles[PITCH] to zero during render.
	if (ent->player && ent->index != gEngfuncs.GetLocalPlayer()->index)
		ent->curstate.fuser2 = ent->angles.x;

	if (g_iUser1)
	{
		gHUD.m_Spectator.AddOverviewEntity(type, ent, modelname);

		if ((g_iUser1 == OBS_IN_EYE || gHUD.m_Spectator.m_pip->value == INSET_IN_EYE) &&
			ent->index == g_iUser2)
			return 0; // don't draw the player we are following in eye
	}

	enddbg;
	return 1;
}

/*
=========================
HUD_TxferLocalOverrides

The server sends us our origin with extra precision as part of the clientdata structure, not during the normal
playerstate update in entity_state_t.  In order for these overrides to eventually get to the appropriate playerstate
structure, we need to copy them into the state structure at this point.
Dogg: Called on Local Player Only
=========================
*/
void DLLEXPORT HUD_TxferLocalOverrides(struct entity_state_s *state, const struct clientdata_s *client)
{
	DBG_INPUT;
	startdbg;

	dbg("Begin");
	VectorCopy(client->origin, state->origin);

	// Spectator
	state->iuser1 = client->iuser1;
	state->iuser2 = client->iuser2;

	// Duck prevention
	state->iuser3 = client->iuser3;

	// Fire prevention
	state->iuser4 = client->iuser4;
	enddbg;
}

/*
=========================
HUD_ProcessPlayerState

We have received entity_state_t for this player over the network.  We need to copy appropriate fields to the
playerstate structure
//Dogg: Players only
=========================
*/
void DLLEXPORT HUD_ProcessPlayerState(struct entity_state_s *dst, const struct entity_state_s *src)
{
	DBG_INPUT;
	startdbg;

	dbg("Begin");
	// Copy in network data
	VectorCopy(src->origin, dst->origin);
	VectorCopy(src->angles, dst->angles);

	/*if( m_pCurrentEntity->player && pplayer->number != gEngfuncs.GetLocalPlayer()->index )
	{
		m_pCurrentEntity->angles.x *= -1;
		m_pCurrentEntity->curstate.angles = m_pCurrentEntity->angles;
	}*/

	VectorCopy(src->velocity, dst->velocity);

	dst->frame = src->frame;
	dst->modelindex = src->modelindex;
	dst->skin = src->skin;
	dst->effects = src->effects;
	dst->weaponmodel = src->weaponmodel;
	dst->movetype = src->movetype;
	dst->sequence = src->sequence;
	dst->animtime = src->animtime;

	dst->solid = src->solid;

	dst->rendermode = src->rendermode;
	dst->renderamt = src->renderamt;
	dst->rendercolor.r = src->rendercolor.r;
	dst->rendercolor.g = src->rendercolor.g;
	dst->rendercolor.b = src->rendercolor.b;
	dst->renderfx = src->renderfx;

	//Old MiB Armor Fix - Undone
	/*
	dst->renderfx				= src->renderfx / 256; //MIB JAN2010_27 Armor Fix Fix - Shift it back

	//Unload the greeks!
	dst->body					= src->renderfx & 255;
	cl_entity_t *tmp = gEngfuncs.GetEntityByIndex( dst->number ); //MIB JAN2010_27 Armor Fix Fix - Sick of this body-not passing shit. You're going to propagate, dammit!d
	tmp->curstate.body = dst->body;
	*/

	dst->framerate = src->framerate;
	//dst->body					= src->body;

	memcpy(&dst->controller[0], &src->controller[0], 4 * sizeof(byte));
	memcpy(&dst->blending[0], &src->blending[0], 2 * sizeof(byte));

	VectorCopy(src->basevelocity, dst->basevelocity);

	dst->friction = src->friction;
	dst->gravity = src->gravity;
	dst->gaitsequence = src->gaitsequence;
	dst->spectator = src->spectator;
	dst->usehull = src->usehull;
	dst->playerclass = src->playerclass;
	dst->team = src->team;
	dst->colormap = src->colormap;
	dst->fuser1 = src->fuser1;

	// Save off some data so other areas of the Client DLL can get to it
	cl_entity_t *player = gEngfuncs.GetLocalPlayer(); // Get the local player's index
	if (dst->number == player->index)
	{
		g_iPlayerClass = dst->playerclass;
		g_iTeamNumber = dst->team;

		g_iUser1 = src->iuser1;
		g_iUser2 = src->iuser2;
		g_iUser3 = src->iuser3;
	}
	enddbg;
}

/*
=========================
HUD_TxferPredictionData

Because we can predict an arbitrary number of frames before the server responds with an update, we need to be able to copy client side prediction data in
 from the state that the server ack'd receiving, which can be anywhere along the predicted frame path ( i.e., we could predict 20 frames into the future and the server ack's
 up through 10 of those frames, so we need to copy persistent client-side only state from the 10th predicted frame to the slot the server
 update is occupying.

Dogg- the extra p in ppxx stands for 'predicted'
Dogg: Called on Players Only
=========================
*/
void DLLEXPORT HUD_TxferPredictionData(struct entity_state_s *ps, const struct entity_state_s *pps, struct clientdata_s *pcd, const struct clientdata_s *ppcd, struct weapon_data_s *wd, const struct weapon_data_s *pwd)
{
	DBG_INPUT;
	startdbg;

	dbg("Begin");
	if (player.m_CharacterState == CHARSTATE_LOADED)
	{
		//Master Sword: Always use the client-side viewmodel
		pcd->viewmodel = player.pev->viewmodel;
	}
	pcd->health = 1.0f; //Master Sword: Keep health at 1.0f so engine doesn't do wierd things (like hide the view model)
						//when health falls below 1

	ps->oldbuttons = pps->oldbuttons;
	ps->flFallVelocity = pps->flFallVelocity;
	ps->iStepLeft = pps->iStepLeft;
	ps->playerclass = pps->playerclass;

	//pcd->viewmodel				= ppcd->viewmodel;
	pcd->m_iId = ppcd->m_iId;
	pcd->ammo_shells = ppcd->ammo_shells;
	pcd->ammo_nails = ppcd->ammo_nails;
	pcd->ammo_cells = ppcd->ammo_cells;
	pcd->ammo_rockets = ppcd->ammo_rockets;
	pcd->m_flNextAttack = ppcd->m_flNextAttack;
	pcd->fov = ppcd->fov;
	pcd->weaponanim = ppcd->weaponanim;
	pcd->tfstate = ppcd->tfstate;
	pcd->maxspeed = ppcd->maxspeed;

	pcd->deadflag = ppcd->deadflag;
	pcd->fuser3 = ppcd->fuser3;

	// Spectating or not dead == get control over view angles.
	g_iAlive = (ppcd->iuser1 || (pcd->deadflag == DEAD_NO)) ? 1 : 0;

	// Spectator
	pcd->iuser1 = ppcd->iuser1;
	pcd->iuser2 = ppcd->iuser2;

	// Duck prevention
	pcd->iuser3 = ppcd->iuser3;

	if (gEngfuncs.IsSpectateOnly())
	{
		// in specator mode we tell the engine who we want to spectate and how
		// iuser3 is not used for duck prevention (since the spectator can't duck at all)
		pcd->iuser1 = g_iUser1; // observer mode
		pcd->iuser2 = g_iUser2; // first target
		pcd->iuser3 = g_iUser3; // second target
	}

	pcd->fuser1 = ppcd->fuser1;
	pcd->flTimeStepSound = ppcd->flTimeStepSound;

	/*VectorCopy( ppcd->vuser1, pcd->vuser1 );
	VectorCopy( ppcd->vuser2, pcd->vuser2 );
	VectorCopy( ppcd->vuser3, pcd->vuser3 );
	VectorCopy( ppcd->vuser4, pcd->vuser4 );*/
	enddbg;
}

/*
//#define TEST_IT
#if defined( TEST_IT )

cl_entity_t mymodel[9];

void MoveModel( void )
{
	cl_entity_t *player;
	int i, j;
	int modelindex;
	struct model_s *mod;

	// Load it up with some bogus data
	player = gEngfuncs.GetLocalPlayer();
	if ( !player )
		return;

	mod = gEngfuncs.CL_LoadModel( "models/sentry3.mdl", &modelindex );
	for ( i = 0; i < 3; i++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			// Don't draw over ourself...
			if ( ( i == 1 ) && ( j == 1 ) )
				continue;

			mymodel[ i * 3 + j ] = *player;

			mymodel[ i * 3 + j ].player = 0;

			mymodel[ i * 3 + j ].model = mod;
			mymodel[ i * 3 + j ].curstate.modelindex = modelindex;
		
				// Move it out a bit
			mymodel[ i * 3 + j ].origin[0] = player->origin[0] + 50 * ( 1 - i );
			mymodel[ i * 3 + j ].origin[1] = player->origin[1] + 50 * ( 1 - j );

			gEngfuncs.CL_CreateVisibleEntity( ET_NORMAL, &mymodel[i*3+j] );
		}
	}

}

#endif

//#define TRACE_TEST
#if defined( TRACE_TEST )

extern int hitent;

cl_entity_t hit;

void TraceModel( void )
{
	cl_entity_t *ent;

	if ( hitent <= 0 )
		return;

	// Load it up with some bogus data
	ent = gEngfuncs.GetEntityByIndex( hitent );
	if ( !ent )
		return;

	hit = *ent;
	//hit.curstate.rendermode = kRenderTransTexture;
	//hit.curstate.renderfx = kRenderFxGlowShell;
	//hit.curstate.renderamt = 100;

	hit.origin[2] += 40;

	gEngfuncs.CL_CreateVisibleEntity( ET_NORMAL, &hit );
}

#endif
*/

/*
void ParticleCallback( struct particle_s *particle, float frametime )
{
	int i;

	for ( i = 0; i < 3; i++ )
	{
		particle->org[ i ] += particle->vel[ i ] * frametime;
	}
}

cvar_t *color = NULL;
void Particles( void )
{
	static float lasttime;
	float curtime;
	
	curtime = gEngfuncs.GetClientTime();

	if ( ( curtime - lasttime ) < 2.0 )
		return;

	if ( !color )
	{
		color = gEngfuncs.pfnRegisterVariable ( "color","255 0 0", 0 );
	}

	lasttime = curtime;

	// Create a few particles
	particle_t *p;
	int i, j;

	for ( i = 0; i < 1000; i++ )
	{
		int r, g, b;
		p = gEngfuncs.pEfxAPI->R_AllocParticle( ParticleCallback );
		if ( !p )
			break;

		for ( j = 0; j < 3; j++ )
		{
			p->org[ j ] = v_origin[ j ] + gEngfuncs.pfnRandomFloat( -32.0, 32.0 );;
			p->vel[ j ] = gEngfuncs.pfnRandomFloat( -100.0, 100.0 );
		}

		if ( color )
		{
			sscanf( color->string, "%i %i %i", &r, &g, &b );
		}
		else
		{
			r = 192;
			g = 0;
			b = 0;
		}

		p->color = 	gEngfuncs.pEfxAPI->R_LookupColor( r, g, b );
		gEngfuncs.pEfxAPI->R_GetPackedColor( &p->packedColor, p->color );

		// p->die is set to current time so all you have to do is add an additional time to it
		p->die += 3.0;
	}
}
*/

/*
void TempEntCallback ( struct tempent_s *ent, float frametime, float currenttime )
{
	int i;

	for ( i = 0; i < 3; i++ )
	{
		ent->entity.curstate.origin[ i ] += ent->entity.baseline.origin[ i ] * frametime;
	}
}

void TempEnts( void )
{
	static float lasttime;
	float curtime;
	
	curtime = gEngfuncs.GetClientTime();

	if ( ( curtime - lasttime ) < 10.0 )
		return;

	lasttime = curtime;

	TEMPENTITY *p;
	int i, j;
	struct model_s *mod;
	vec3_t origin;
	int index;

	mod = gEngfuncs.CL_LoadModel( "sprites/laserdot.spr", &index );

	for ( i = 0; i < 100; i++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			origin[ j ] = v_origin[ j ];
			if ( j != 2 )
			{
				origin[ j ] += 75;
			}
		}

		p = gEngfuncs.pEfxAPI->CL_TentEntAllocCustom( (float *)&origin, mod, 0, TempEntCallback );
		if ( !p )
			break;

		for ( j = 0; j < 3; j++ )
		{
			p->entity.curstate.origin[ j ] = origin[ j ];

			// Store velocity in baseline origin
			p->entity.baseline.origin[ j ] = gEngfuncs.pfnRandomFloat( -100, 100 );
		}

		// p->die is set to current time so all you have to do is add an additional time to it
		p->die += 10.0;
	}
}
*/

/*
=========================
HUD_CreateEntities
	
Gives us a chance to add additional entities to the render this frame
=========================
*/

void DLLEXPORT HUD_CreateEntities(void)
{
	// e.g., create a persistent cl_entity_t somewhere.
	// Load an appropriate model into it ( gEngfuncs.CL_LoadModel )
	// Call gEngfuncs.CL_CreateVisibleEntity to add it to the visedicts list
	/*
#if defined( TEST_IT )
	MoveModel();
#endif

#if defined( TRACE_TEST )
	TraceModel();
#endif
*/

	//Particles();
	//TempEnts();

	DBG_INPUT;
	startdbg;

	dbg("Begin");
#if defined(BEAM_TEST)
	Beams();
#endif

	dbg("Call Game_AddObjects");
	// Add in any game specific objects
	Game_AddObjects();

	//dbg( "Call HUDScript->Effects_TempEnts" );

	dbg("Call GetClientVoiceMgr()->CreateEntities");
	GetClientVoiceMgr()->CreateEntities();

	dbg("Call gHUD.m_HUDScript->Effects_PreRender");
	if (gHUD.m_HUDScript)
		gHUD.m_HUDScript->Effects_PreRender();

	dbg("Call player.Render()");
	player.Render();

	enddbg;
}

//Put here because all the cool headers are already defined here
TEMPENTITY *g_CurrentTempEnt = NULL;
cl_entity_t *g_CurrentEnt = NULL;
#define MSTEMPENT_ID (1 << 0)
#define MSTEMPENT_CALLBACK (1 << 1)
#define MSTEMPENT_GRAVITY (1 << 2)
#define MSTEMPENT_FOLLOWENT (1 << 3)

void TempEntCallback(struct tempent_s *ent, float frametime, float currenttime)
{
	if (!FBitSet(ent->entity.curstate.iuser4, MSTEMPENT_CALLBACK))
		return;

	g_CurrentTempEnt = ent;
	HUDScript->Effects_UpdateTempEnt(STRING(ent->entity.curstate.iuser3));
	g_CurrentTempEnt = nullptr;
}

void TempEntHitCallback(struct tempent_s *ent, struct pmtrace_s *ptr)
{
	startdbg;
	if (!ent->entity.curstate.weaponanim)
		return;

	g_CurrentTempEnt = ent;
	tempentextra_t &TempEntExtra = g_TempEntExtra[ent->entity.curstate.weaponanim];
	if (TempEntExtra.CBCollide_Enabled)
	{
		//AUG2013_24 Thothie - get tempent to return model idx of model struck
		static msstringlist Params;
		Params.clearitems();
		if (ptr->ent)
		{
			Params.add(UTIL_VarArgs("%i", ptr->ent));
		}
		else
		{
			Params.add("world");
		}
		HUDScript->Effects_UpdateTempEnt(TempEntExtra.CBCollide_CallbackEvent, &Params);
	}
	g_CurrentTempEnt = NULL;

	enddbg;
}

void CHudScript::Effects_UpdateTempEnt(msstring_ref EventName, msstringlist *Parameters)
{
	startdbg;
	//Update tempents
	TEMPENTITY *pTempEnt = g_CurrentTempEnt; //Save a copy, because this could get set to NULL during RunScriptEventByName
	for (int i = 0; i < m_Scripts.size(); i++)
	{
		CScript *Script = m_Scripts[i];
		if (pTempEnt->entity.curstate.iuser2 != (int)Script)
			continue;
		if (Parameters)
		{
			Script->RunScriptEventByName(EventName, Parameters);
		}
		else
		{
			Script->RunScriptEventByName(EventName);
		}
	}
	enddbg;
}
msstring_ref CScript::CLGetCurrentTempEntProp(msstring &Prop)
{
	if (!g_CurrentTempEnt)
		RETURN_NOTHING;
	TEMPENTITY *p = g_CurrentTempEnt;

	static msstring Return;
	if (Prop == "deathtime")
		RETURN_FLOAT(p->die)
	else if (Prop == "bouncefactor")
		RETURN_FLOAT(p->bounceFactor)
	else
	{
		static msstringlist Params;
		Params.clearitems();
		Params.add("dummy");
		Params.add(Prop);
		return CLGetEntProp(&p->entity, Params);
	}

	RETURN_NOTHING;
}

#define RETURN_COLOR(name, color)                             \
	{                                                         \
		if (Prop == name)                                     \
		{                                                     \
			Vector vTemp = Vector(color.r, color.g, color.b); \
			return (Return = VecToString(vTemp));             \
		}                                                     \
		else if (Prop == name ".r")                           \
			RETURN_FLOAT(color.r)                             \
		else if (Prop == name ".g")                           \
			RETURN_FLOAT(color.g)                             \
		else if (Prop == name ".b")                           \
			RETURN_FLOAT(color.b)                             \
	}

//[begin] DEC2014_09 Thothie - beam_update
//$getcl_beam(<idx|all>,<property>)
msstring_ref CScript::CLGetBeamProp(int beamid, msstringlist &Params)
{
	bool found_beam = (beamid <= (int)m_Beams.size() - 1) ? true : false;
	BEAM *pBeam = found_beam ? m_Beams[beamid] : NULL;
	/*
	bool found_beam = false;
	 for (int i = 0; i < m_Beams.size(); i++) 
	{
		Print("DEBUG: $get_clbeam checking %i for %i\n",i,beamid);
		if ( m_Beams[i]->id == beamid )
		{
			pBeam = m_Beams[i];
			found_beam = true;
			break;
		}
	}
	*/

	if (found_beam)
	{
		//Print("DEBUG: $get_clbeam returning value of property %s\n",Params[1].c_str());
		msstring beamret = "0";
		msstring &Prop = Params[1];
		if (Prop == "start")
			beamret = VecToString(pBeam->source);
		else if (Prop == "end")
			beamret = VecToString(pBeam->target);
		else if (Prop == "delta")
			beamret = VecToString(pBeam->delta);
		else if (Prop == "width")
			beamret = FloatToString(pBeam->width);
		else if (Prop == "amplitude")
			beamret = FloatToString(pBeam->amplitude);
		else if (Prop == "color")
		{
			beamret = UTIL_VarArgs("(%f,%f,%f)", pBeam->r, pBeam->g, pBeam->b);
		}
		else if (Prop == "brightness")
			beamret = FloatToString(pBeam->brightness);
		else if (Prop == "speed")
			beamret = FloatToString(pBeam->speed);
		else if (Prop == "life")
			beamret = FloatToString(pBeam->die);
		else if (Prop == "segments")
			beamret = IntToString(pBeam->segments);
		else if (Prop == "framerate")
			beamret = FloatToString(pBeam->frameRate);
		else if (Prop == "framecount")
			beamret = IntToString(pBeam->frameCount);
		else if (Prop == "sprite")
			beamret = IntToString(pBeam->modelIndex);
		else if (Prop == "startent")
			beamret = IntToString(pBeam->startEntity);
		else if (Prop == "endent")
			beamret = IntToString(pBeam->endEntity);
		else if (Prop == "flags")
			beamret = IntToString(pBeam->flags);
		else if (Prop == "t")
			beamret = FloatToString(pBeam->t);
		return beamret;
	}
	else
	{
		return "0";
	}
}
//[end] DEC2014_09 Thothie - beam_update

msstring_ref CScript::CLGetEntProp(cl_entity_t *pclEntity, msstringlist &Params)
{
	if (!pclEntity)
		RETURN_NOTHING;
	cl_entity_t &ent = *pclEntity;
	static msstring Return;
	msstring &Prop = Params[1];
	if (Prop.starts_with("origin"))
		RETURN_POSITION("origin", ent.origin)
	else if (Prop.starts_with("center"))
	{
		Vector vecCenter = ent.curstate.origin + ((ent.curstate.mins + ent.curstate.maxs) * 0.5);
		RETURN_POSITION("center", vecCenter)
	}
	else if (Prop.starts_with("angles"))
		RETURN_ANGLE("angles", ent.angles)
	else if (Prop.starts_with("velocity"))
		RETURN_POSITION("velocity", ent.baseline.vuser1) //can't use baseline.origin - it's used by networked ents
	else if (Prop.starts_with("inwater"))
		return EngineFunc::Shared_PointContents(Vector(ent.origin.x, ent.origin.y, ent.origin.z + ent.curstate.mins.z)) == CONTENTS_WATER ? "1" : "0";
	else if (Prop.starts_with("underwater"))
		return (EngineFunc::Shared_PointContents(Vector(ent.origin.x, ent.origin.y, ent.origin.z + ent.curstate.mins.z)) == CONTENTS_WATER && EngineFunc::Shared_PointContents(Vector(ent.origin.x, ent.origin.y, ent.origin.z + ent.curstate.maxs.z)) == CONTENTS_WATER) ? "1" : "0";
	else if (Prop.starts_with("waterorigin"))
	{
		Vector Origin;
		if ((Origin.z = EngineFunc::Shared_GetWaterHeight(ent.origin, ent.curstate.mins.z, ent.curstate.maxs.z)) > ent.origin.z + ent.curstate.mins.z)
		{
			Origin.x = ent.origin.x;
			Origin.y = ent.origin.y;
			RETURN_POSITION("waterorigin", Origin)
		}
	}
	else if (Prop == "model")
	{
		if (ent.model)
		{
			Return = ent.model->name;
			if (Return.starts_with("models/"))
				Return = Return.substr(7);
			if (Return.starts_with("sprites/"))
				Return = Return.substr(8);
			return Return;
		}
		return ent.model ? ent.model->name : "0";
	}
	//these two both fail
	//however you can store the time the tempent began on fuserX in the creation event
	//then guess how far along the anim has gotten in that time based on the frame rate
	/*
	else if( Prop == "animtime" ) RETURN_FLOAT( ent.curstate.animtime ) //FEB2009_19 - experimenting
	else if( Prop == "starttime" ) RETURN_FLOAT( ent.curstate.starttime ) //FEB2009_19 - experimenting
	*/
	else if (Prop == "ducking")
		return FBitSet(player.pev->flags, FL_DUCKING) ? "1" : "0"; //Thothie JUN2010_25
	else if (Prop == "modelidx")
		RETURN_INT(ent.curstate.modelindex)
	else if (Prop == "anim")
		RETURN_INT(ent.curstate.sequence)
	else if (Prop == "height")
		RETURN_FLOAT((ent.curstate.maxs.z) - (ent.curstate.mins.z)) //Thothie SEP2018_15 - return height/width for $getcl
	else if (Prop == "width")
		RETURN_FLOAT((ent.curstate.maxs.x) - (ent.curstate.mins.x)) //Thothie SEP2018_15 - return height/width for $getcl
	else if (Prop == "mins")
		RETURN_VECTOR(ent.curstate.mins)
	else if (Prop == "maxs")
		RETURN_VECTOR(ent.curstate.maxs)
	else if (Prop == "frame")
		RETURN_INT(ent.curstate.frame)
	else if (Prop == "framerate")
		RETURN_INT(ent.curstate.framerate)
	else if (Prop == "exists")
		return pclEntity->Exists() ? "1" : "0";
	else if (Prop == "gravity")
		RETURN_FLOAT(ent.curstate.gravity)
	else if (Prop == "scale")
		RETURN_FLOAT(ent.curstate.scale)
	//NOV2014_16 dealing with the fact that MSC scripts can't hanle percision more than 2f
	else if (Prop == "scaleHD")
		RETURN_FLOAT(ent.curstate.scale * 1000)
	else if (Prop == "fuser1HD")
		RETURN_FLOAT(ent.curstate.fuser1 * 1000)
	else if (Prop == "renderfx")
		RETURN_INT(ent.curstate.renderfx)
	else if (Prop == "rendermode")
		RETURN_INT(ent.curstate.rendermode)
	else if (Prop == "renderamt")
		RETURN_FLOAT(ent.curstate.renderamt)
	else if (Prop.starts_with("rendercolor"))
		RETURN_COLOR("rendercolor", ent.curstate.rendercolor)
	else if (Prop == "visible")
		return FBitSet(ent.curstate.effects, EF_NODRAW) ? "0" : "1";
	else if (Prop == "isplayer")
		return ent.player ? "1" : "0";
	else if (Prop.starts_with("attachment0"))
		RETURN_POSITION("attachment0", ent.attachment[0])
	else if (Prop.starts_with("attachment1"))
		RETURN_POSITION("attachment1", ent.attachment[1])
	else if (Prop.starts_with("attachment2"))
		RETURN_POSITION("attachment2", ent.attachment[2])
	else if (Prop.starts_with("attachment3"))
		RETURN_POSITION("attachment3", ent.attachment[3])
	else if (Prop == "iuser1")
		RETURN_INT(ent.curstate.iuser1)
	else if (Prop == "iuser2")
		RETURN_INT(ent.curstate.iuser2)
	else if (Prop == "iuser3")
		RETURN_INT(ent.curstate.iuser3)
	else if (Prop == "iuser4")
		RETURN_INT(ent.curstate.iuser4)
	else if (Prop == "fuser1")
		RETURN_FLOAT(ent.curstate.fuser1)
	else if (Prop == "fuser2")
		RETURN_FLOAT(ent.curstate.fuser2)
	else if (Prop == "fuser3")
		RETURN_FLOAT(ent.curstate.fuser3)
	else if (Prop == "fuser4")
		RETURN_FLOAT(ent.curstate.fuser4)
	else if (Prop == "prevstate.iuser1")
		RETURN_INT(ent.prevstate.iuser1)
	else if (Prop == "prevstate.iuser2")
		RETURN_INT(ent.prevstate.iuser2)
	else if (Prop == "prevstate.iuser3")
		RETURN_INT(ent.prevstate.iuser3)
	else if (Prop == "prevstate.iuser4")
		RETURN_INT(ent.prevstate.iuser4)
	else if (Prop == "prevstate.fuser1")
		RETURN_FLOAT(ent.prevstate.fuser1)
	else if (Prop == "prevstate.fuser2")
		RETURN_FLOAT(ent.prevstate.fuser2)
	else if (Prop == "prevstate.fuser3")
		RETURN_FLOAT(ent.prevstate.fuser3)
	else if (Prop == "prevstate.fuser4")
		RETURN_FLOAT(ent.prevstate.fuser4)
	else if (Prop == "baseline.iuser1")
		RETURN_INT(ent.baseline.iuser1) //Baseline props are not overriden by
	else if (Prop == "baseline.iuser2")
		RETURN_INT(ent.baseline.iuser2) //incoming server packets
	else if (Prop == "baseline.iuser3")
		RETURN_INT(ent.baseline.iuser3) //Stuff stored here is persistent
	else if (Prop == "baseline.iuser4")
		RETURN_INT(ent.baseline.iuser4)
	else if (Prop == "baseline.fuser1")
		RETURN_FLOAT(ent.baseline.fuser1)
	else if (Prop == "baseline.fuser2")
		RETURN_FLOAT(ent.baseline.fuser2)
	else if (Prop == "baseline.fuser3")
		RETURN_FLOAT(ent.baseline.fuser3)
	else if (Prop == "baseline.fuser4")
		RETURN_FLOAT(ent.baseline.fuser4)
	else if (Prop == "bonepos")
	{
		//Thothie (Comment Only) - We may want to move this up, as it is sometimes used every frame
		//- might want to reorginize some of these others accordingly as well
		//Vector Pos;
		/*
		if( !pclEntity->GetBonePos( atoi(Params[2]), Pos ) )
			RETURN_ZERO;
		*/

		//AUG2013_25 Thothie - attempting to fix client side bonepos function
		Vector Pos;
		ent.GetBonePosVec(atoi(Params[2]), Pos); //GetBonePos is boolean, wtf?
		//pclEntity->GetBonePosVec( atoi(Params[2]), Pos );
		RETURN_VECTOR(Pos);
	}
	else if (Prop.starts_with("bonecount"))
	{
		//AUG2013_25 - enabling getting bone count (be good to have this serverside too)
		RETURN_INT(ent.GetBoneCount())
	}
	else if (Prop.starts_with("viewangles"))
	{
		//MIB AUG2010_05
		vec3_t viewangles;
		gEngfuncs.GetViewAngles((float *)viewangles);
		RETURN_ANGLE("viewangles", viewangles)
	}
	/*
	else if ( Prop.starts_with("eyepos") )
	{
		//Thoth APR2012_05 - attempting to return eye position (fail)
		vec3_t eyepos = player.pev->view_ofs;
		RETURN_ANGLE( "eyepos", eyepos )
	}
	*/

	//MIB FEB2010_01 - $getcl gender identification - fail
	/*
	else if ( Prop == "gender" || Prop == "race" )
	{
		//Only works for the client-player
		CBasePlayer *pPlayer = gEngfuncs.GetLocalPlayer();
		if ( Prop == "gender" ) return pPlayer->m_Gender ? "female" : "male";
		else if ( Prop == "race" ) return pPlayer->m_Race;
	}
	*/
	RETURN_ZERO;
}

int CL_LoadModel(msstring_ref RelativePathname, model_s **ppModel = NULL)
{
	int modelindex = 0;
	msstring FullName = GetFullResourceName(RelativePathname);
	model_s *pModel = gEngfuncs.CL_LoadModel(FullName, &modelindex);
	if (ppModel)
		*ppModel = pModel;
	return modelindex;
}

void CScript::CLScriptedEffect(msstringlist &Params)
{
	if (!Params.size())
		return;

	if (Params[0] == "tempent")
	{
		if (Params.size() < 2)
			return;

		if (Params[1] == "sprite" || Params[1] == "model")
		{
			bool IsSprite = (Params[1] == "sprite") ? true : false;
			if (Params.size() < 4)
				return;

			int index;
			msstring FullName = GetFullResourceName(Params[2]);
			model_s *Model = gEngfuncs.CL_LoadModel(FullName, &index);
			Vector Origin = StringToVec(Params[3]);

			TEMPENTITY *p = gEngfuncs.pEfxAPI->CL_TentEntAllocCustom(Origin, Model, 0, TempEntCallback);
			if (!p)
				return;

			p->hitcallback = *TempEntHitCallback;
			SetBits(p->entity.curstate.iuser4, MSTEMPENT_ID);
			SetBits(p->flags, FTENT_GRAVITY);
			if (IsSprite)
			{
				p->entity.curstate.rendermode = kRenderTransAdd;
				p->entity.curstate.rendercolor.r = 0;
				p->entity.curstate.rendercolor.g = 0;
				p->entity.curstate.rendercolor.b = 0;
				p->entity.curstate.renderamt = 255;
				SetBits(p->flags, FTENT_SPRANIMATELOOP); //Default to looping, if animated
			}
			p->entity.curstate.iuser2 = (int)this;

			p->entity.curstate.origin = Origin;
			p->die += 1.0;

			int Spot = 0;
			for (int i = 0; i < MAX_TEMPENT_EXTRA; i++)
				if (!g_TempEntExtra[i].Active)
				{
					Spot = i;
					break;
				}

			tempentextra_t &TempExtra = g_TempEntExtra[Spot];
			clrmem(TempExtra);
			p->entity.curstate.weaponanim = Spot;
			TempExtra.Active = true;

			if (Params.size() >= 5)
			{
				if (Params.size() >= 6) //Callback event name
					if (Params[5] != "none")
					{
						p->entity.curstate.iuser3 = ALLOC_STRING(Params[5]);
						SetBits(p->entity.curstate.iuser4, MSTEMPENT_CALLBACK);
					}
				if (Params.size() >= 7) //Collision eventname
					if (Params[6] != "none")
					{
						TempExtra.CBCollide_Enabled = true;
						TempExtra.CBCollide_CallbackEvent = Params[6];
					}

				//Update the first time
				if (Params[4] != "none")
				{
					g_CurrentTempEnt = p;
					gHUD.m_HUDScript->Effects_UpdateTempEnt(Params[4]);
					g_CurrentTempEnt = NULL;
				}
			}
		}

		else if (Params[1] == "set_current_prop")
		{
			TEMPENTITY *p = g_CurrentTempEnt;
			if (!p)
				return;
			if (Params.size() < 4)
				return;

			tempentextra_t TempExtraDummy;
			tempentextra_t &TempExtra = p->entity.curstate.weaponanim ? g_TempEntExtra[p->entity.curstate.weaponanim] : TempExtraDummy;

			msstring &Cmd = Params[2];
			msstring &Value = Params[3];

			bool FallBack = false;
			if (Cmd == "death_delay")
			{
				if (Value == "last_frame")
					ClearBits(p->flags, FTENT_SPRANIMATELOOP);
				else if (Value == "die_with_ent")
				{
					if (Params.size() >= 5)
					{
						TempExtra.DieWithEntActive = true;
						TempExtra.DieWithEnt = atoi(Params[4]);
						p->die = gEngfuncs.GetClientTime() + 10;
					}
				}
				else
					p->die = gEngfuncs.GetClientTime() + atof(Value);
			}
			else if (Cmd == "bouncefactor")
				p->bounceFactor = atof(Value);
			else if (Cmd == "framerate")
			{
				SetBits(p->flags, FTENT_SPRANIMATE);
				p->entity.curstate.framerate = atof(Value);
			}
			else if (Cmd == "frames")
				p->frameMax = atof(Value);
			else if (Cmd == "collide")
			{
				if (Value.contains("all"))
				{
					SetBits(p->flags, FTENT_COLLIDEALL);
					//AUG2013_25 Thothie - skip collision checks for specific ent index
					if (Params.size() >= 5)
					{
						p->entity.curstate.iuser1 = atoi(Params[4]);
						SetBits(p->flags, FTENT_SKIPENT);
					}
				}
				else if (Value.contains("world"))
				{
					ClearBits(p->flags, FTENT_COLLIDEALL);
					SetBits(p->flags, FTENT_COLLIDEWORLD);
				}
				else if (Value.contains("none"))
					ClearBits(p->flags, FTENT_COLLIDEALL | FTENT_COLLIDEWORLD);

				//Not else
				if (Value.contains("die"))
					SetBits(p->flags, FTENT_COLLIDEKILL);
			}
			//Thothie MAR2011_08 - add callback on collide
			else if (Cmd == "cb_collide")
			{
				TempExtra.CBCollide_Enabled = true;
				TempExtra.CBCollide_CallbackEvent = Value;
			}
			else if (Cmd == "gravity")
			{
				p->entity.curstate.gravity = atof(Value);
				if (p->entity.curstate.gravity)
					SetBits(p->entity.curstate.iuser4, MSTEMPENT_GRAVITY);
				else
				{
					ClearBits(p->entity.curstate.iuser4, MSTEMPENT_GRAVITY);
					ClearBits(p->flags, FTENT_GRAVITY);
				}
			}
			else if (Cmd == "sequence")
			{
				p->entity.curstate.sequence = atoi(Value);
			}
			else if (p->entity.curstate.weaponanim)
			{
				if (Cmd == "timer")
				{
					TempExtra.CBTimer_Enabled = true;
					TempExtra.CBTimer_TimeCallback = gEngfuncs.GetClientTime() + atof(Value);
					TempExtra.CBTimer_CallbackEvent = Params[4];
				}
				else if (Cmd == "fadeout")
				{
					TempExtra.Fadeout = true;
					TempExtra.FadeDuration = Value == "lifetime" ? (p->die - gEngfuncs.GetClientTime()) : atof(Value);
					TempExtra.FadeStart = gEngfuncs.GetClientTime();
				}
				else if (Cmd == "cb_hitwater")
				{
					TempExtra.CBWater_Enabled = true;
					TempExtra.CBWater_CallbackEvent = Value;
				}
				else
					FallBack = true;
			}
			else
				FallBack = true;

			if (FallBack)
			{
				static mslist<msstring *> ValueParams;
				ValueParams.clearitems();

				for (int i = 0; i < Params.size() - 3; i++)
					ValueParams.add(&(Params[i + 3]));

				SetClEntityProp(p->entity, Cmd, ValueParams);
			}
		}
	}
	else if (Params[0] == "frameent")
	{
		//Visent
		msstring &Options = Params[1];
		if (Options.contains("model") || Options.contains("sprite"))
		{
			if (Params.size() < 4)
				return;

			int ModelIdx = 0;
			msstring FullName = GetFullResourceName(Params[2]);
			model_s *Model = gEngfuncs.CL_LoadModel(FullName, &ModelIdx);
			if (Model)
			{
				bool IsSprite = Options.contains("sprite") ? true : false;
				bool IsPerm = Options.contains("perm") ? true : false;

				static cl_entity_t FrameEnts[256];
				static int CurrFrameEnt = 0;
				cl_entity_t &RenderEnt = IsPerm ? MSCLGlobals::m_ClModels.add(cl_entity_t()) : FrameEnts[CurrFrameEnt++];
				clrmem(RenderEnt);

				if (CurrFrameEnt >= 256)
					CurrFrameEnt = 0;
				if (!IsPerm)
					if (!gEngfuncs.CL_CreateVisibleEntity(ET_NORMAL, &RenderEnt))
						return; //Exit if can't create the frame entity

				if (IsSprite)
				{
					RenderEnt.curstate.rendermode = kRenderTransAdd;
					RenderEnt.curstate.rendercolor.r = 0;
					RenderEnt.curstate.rendercolor.g = 0;
					RenderEnt.curstate.rendercolor.b = 0;
					RenderEnt.curstate.renderamt = 255;
				}
				RenderEnt.model = Model;
				RenderEnt.curstate.modelindex = ModelIdx;
				RenderEnt.origin = StringToVec(Params[3]);
				RenderEnt.curstate.starttime = (MSCLGlobals::m_ClModels.size() - 1);

				g_CurrentEnt = &RenderEnt;
				if (Params.size() >= 5)
					RunScriptEventByName(Params[4]);
				g_CurrentEnt = NULL;
			}
		}
		else if (Params[1] == "set_current_prop")
		{
			if (!g_CurrentEnt)
				return;
			if (Params.size() < 2)
				return;

			msstring &Cmd = Params[2];

			static mslist<msstring *> ValueParams;
			ValueParams.clearitems();

			for (int i = 0; i < Params.size() - 3; i++)
				ValueParams.add(&Params[i + 3]);

			SetClEntityProp(*g_CurrentEnt, Cmd, ValueParams);
		}
		else if (Params[1] == "kill_current_ent")
		{
			if (!g_CurrentEnt)
				return;

			MSCLGlobals::m_ClModels.erase(g_CurrentEnt->curstate.starttime);
		}
	}
	else if (Params[0] == "setprop")
	{
		if (Params.size() < 4)
			return;

		cl_entity_t *pEnt = gEngfuncs.GetEntityByIndex(atoi(Params[1]));
		if (pEnt)
		{
			msstring &Cmd = Params[2];

			static mslist<msstring *> ValueParams;
			ValueParams.clearitems();

			for (int i = 0; i < Params.size() - 3; i++)
				ValueParams.add(&Params[i + 3]);

			SetClEntityProp(*pEnt, Cmd, ValueParams);
		}
	}
	else if (Params[0] == "light")
	{
		//<origin> <radius> <(r,g,b)> <duration> ["entity"|"dark"]
		if (Params[1] == "new" && atoi(EngineFunc::CVAR_GetString("r_dynamic")) == 0)
			return;

		dlight_t NewLight;
		clrmem(NewLight);
		bool EntityLight = false;
		int NextParm = 2;

		NewLight.origin = StringToVec(Params[NextParm++]);
		NewLight.radius = atof(Params[NextParm++]);
		Vector Color = StringToVec(Params[NextParm++]);
		NewLight.color.r = Color.x;
		NewLight.color.g = Color.y;
		NewLight.color.b = Color.z;
		NewLight.die = gEngfuncs.GetClientTime() + atof(Params[NextParm++]);
		msstring &Flags = Params[NextParm++];
		if (Flags.contains("entity"))
			EntityLight = true;
		if (Flags.contains("dark"))
			NewLight.dark = true;

		dlight_t *pLight = Params[1] == "new" ? (EntityLight ? gEngfuncs.pEfxAPI->CL_AllocElight(0) : gEngfuncs.pEfxAPI->CL_AllocDlight(0)) : (dlight_t *)atoi(Params[1]);

		//AUG2013_19 - Thothie - extended conditional to encase m_gLastLightID line for saftey
		if (pLight)
		{
			memcpy(pLight, &NewLight, sizeof(dlight_t));
			m_gLastLightID = (int)pLight;
		}
	}
	else if (Params[0] == "beam_points")
	{
		//<startpos> <endpos> <spritename> <life> <width> <amplitude> <brightness> <speed> <framerate> <color>
		if (Params.size() >= 10)
		{
			int p = 1;
			Vector StartPos = StringToVec(Params[p++]);
			Vector EndPos = StringToVec(Params[p++]);
			msstring_ref ModelName = Params[p++];
			int ModelIdx = CL_LoadModel(ModelName);
			if (ModelIdx <= 0)
			{
				Print("cleffect %s: model '%s' not precached.\n", Params[0].c_str(), ModelName);
				return;
			}
			float Life = atof(Params[p++]);
			float Width = atof(Params[p++]);
			float Amplitude = atof(Params[p++]);
			float Brightness = atof(Params[p++]);
			float Speed = atof(Params[p++]);
			float Framerate = atof(Params[p++]);
			Vector Color = StringToVec(Params[p++]);

			//BEAM *pBeam = gEngfuncs.pEfxAPI->R_BeamLightning( StartPos, EndPos, ModelIdx, Life, Width, Amplitude, Brightness, Speed );
			BEAM *pBeam = gEngfuncs.pEfxAPI->R_BeamPoints(StartPos, EndPos, ModelIdx, Life, Width, Amplitude, Brightness, Speed, 0, Framerate, Color.x, Color.y, Color.z);

			//[begin] DEC2014_09 Thothie - beam_update - clear dead beams, add new
			m_Beams.add(pBeam);
			m_gLastLightID = (int)m_Beams.size() - 1;
			//[end] DEC2014_09 Thothie - beam_update - clear dead beams, add new
		}
	}
	//cleffect beam_ents <startIdx> <attachidx> <endIdx> <attachidx> <spritename> <life> <width> <amplitude> <brightness> <speed> <framerate> <color>
	//- Thothie MAR2012_15
	//- Setup a cl beam between two entities
	else if (Params[0] == "beam_ents")
	{
		if (Params.size() >= 10)
		{
			int p = 1;
			int startIdx = (atoi(Params[p++]) & 0x0FFF) | ((atoi(Params[p++]) & 0xF) << 12);
			int endIdx = (atoi(Params[p++]) & 0x0FFF) | ((atoi(Params[p++]) & 0xF) << 12);
			msstring_ref ModelName = Params[p++];
			int ModelIdx = CL_LoadModel(ModelName);
			if (ModelIdx <= 0)
			{
				Print("cleffect %s: model '%s' not precached.\n", Params[0].c_str(), ModelName);
				return;
			}
			float Life = atof(Params[p++]);
			float Width = atof(Params[p++]);
			float Amplitude = atof(Params[p++]);
			float Brightness = atof(Params[p++]);
			float Speed = atof(Params[p++]);
			float Framerate = atof(Params[p++]);
			Vector Color = StringToVec(Params[p++]);

			//BEAM *pBeam = gEngfuncs.pEfxAPI->R_BeamLightning( StartPos, EndPos, ModelIdx, Life, Width, Amplitude, Brightness, Speed );
			BEAM *pBeam = gEngfuncs.pEfxAPI->R_BeamEnts(startIdx, endIdx, ModelIdx, Life, Width, Amplitude, Brightness, Speed, 0, Framerate, Color.x, Color.y, Color.z);
			//[begin] DEC2014_09 Thothie - beam_update - clear dead beams, add new
			m_Beams.add(pBeam);
			m_gLastLightID = (int)m_Beams.size() - 1;
			//[end] DEC2014_09 Thothie - beam_update - clear dead beams, add new
		}
	}
	//cleffect beam_end <startIdx> <attachidx> <end_vec> <spritename> <life> <width> <amplitude> <brightness> <speed> <framerate> <color>
	//- Thothie MAR2012_15 - beam from ent to vec
	else if (Params[0] == "beam_end")
	{
		if (Params.size() >= 10)
		{
			int startIdx = (atoi(Params[1]) & 0x0FFF) | ((atoi(Params[2]) & 0xF) << 12);
			Vector EndPos = StringToVec(Params[3]);
			msstring_ref ModelName = Params[4];
			int ModelIdx = CL_LoadModel(ModelName);
			if (ModelIdx <= 0)
			{
				Print("cleffect %s: model '%s' not precached.\n", Params[0].c_str(), ModelName);
				return;
			}
			float Life = atof(Params[5]);
			float Width = atof(Params[6]);
			float Amplitude = atof(Params[7]);
			float Brightness = atof(Params[8]);
			float Speed = atof(Params[9]);
			float Framerate = atof(Params[10]);
			Vector Color = StringToVec(Params[11]);

			//BEAM *pBeam = gEngfuncs.pEfxAPI->R_BeamLightning( StartPos, EndPos, ModelIdx, Life, Width, Amplitude, Brightness, Speed );
			BEAM *pBeam = gEngfuncs.pEfxAPI->R_BeamEntPoint(startIdx, EndPos, ModelIdx, Life, Width, Amplitude, Brightness, Speed, 0, Framerate, Color.x, Color.y, Color.z);
			//[begin] DEC2014_09 Thothie - beam_update - clear dead beams, add new
			m_Beams.add(pBeam);
			m_gLastLightID = (int)m_Beams.size() - 1;
			//[end] DEC2014_09 Thothie - beam_update - clear dead beams, add new
		}
	}
	//cleffect beam_follow <idx> <attach> <sprite> <life> <width> <color> <brightness>
	//- Thothie DEC2014_05 - beam follow, trails a model
	else if (Params[0] == "beam_follow")
	{
		if (Params.size() >= 8)
		{
			int p = 1;
			int startIdx = (atoi(Params[p++]) & 0x0FFF) | ((atoi(Params[p++]) & 0xF) << 12);
			msstring_ref ModelName = Params[p++];
			int ModelIdx = CL_LoadModel(ModelName);
			if (ModelIdx <= 0)
			{
				Print("cleffect %s: model '%s' not precached.\n", Params[0].c_str(), ModelName);
				return;
			}
			float Life = atof(Params[p++]);
			float Width = atof(Params[p++]);
			Vector Color = StringToVec(Params[p++]);
			float Brightness = atof(Params[p++]);

			//Print("DEBUG: beam_follow idx %i mdl %i lif %f wid %f col (%f,%f,%f) bri %f\n",startIdx,ModelIdx,Life,Width,Color.x,Color.y,Color.z,Brightness);

			//BEAM *pBeam = gEngfuncs.pEfxAPI->R_BeamLightning( StartPos, EndPos, ModelIdx, Life, Width, Amplitude, Brightness, Speed );
			//( int startEnt, int modelIndex, float life, float width, float r, float g, float b, float brightness );
			BEAM *pBeam = gEngfuncs.pEfxAPI->R_BeamFollow(startIdx, ModelIdx, Life, Width, Color.x, Color.y, Color.z, Brightness);
			//these bits are here to make this behave a bit more like the server variant
			//still not completely up to par, and still permanent
			pBeam->speed = 100;
			pBeam->frameRate = 30;
			pBeam->amplitude = 5;
			float gtime = (float)gEngfuncs.GetClientTime();
			pBeam->die = gtime + Life;
			//[begin] DEC2014_09 Thothie - beam_update - clear dead beams, add new
			/*
			//this was viable with pBeam->id, but not anymore
			//didn't actually seem to be working proper anyways (though hard to tell, as pBeam->id caused odd behavior
			//checking for flag FBEAM_ISACTIVE doesn't seem to work either
			if ( m_Beams.size() > 0 )
			{
				float gcltime = gEngfuncs.GetClientTime();
				 for (int i = 0; i < m_Beams.size(); i++) 
					if ( m_Beams[i]->die > 0 && m_Beams[i]->die < gcltime )
						m_Beams.erase(i);
			}
			*/
			m_Beams.add(pBeam);
			m_gLastLightID = (int)m_Beams.size() - 1;
			//[end] DEC2014_09 Thothie - beam_update - clear dead beams, add new
		}
	}
	//cleffect beam_update <beam_idx|removeall> <property> <value>
	//- Thothie DEC2014_07 - update existing client side beams
	//- properties include:
	//-- start <vec>
	//-- end <vec>
	//-- delta <vec>
	//-- width <units>
	//-- amplitude <float>
	//-- color <(R,B,G)> - note that R,B,G are relative in this clentity (>0=on 0=off)
	//-- brightness <ratio> - this ratio multiplies the color above
	//-- life <float>
	//-- speed <float>
	//-- framerate <float>
	//-- startent <idx> [attachment]
	//-- endent <idx> [attachment]
	//-- sprite <spritename>
	//-- remove
	//- removeall in place of <beam_idx> removes all beams generated by the script and clears the beam tracking array
	else if (Params[0] == "beam_update")
	{
		if (Params.size() >= 2 && Params[1] == "removeall")
		{
			size_t size = m_Beams.size();
			for (int i = 0; i < size; i++)
			{
				BEAM* pBeam = m_Beams[i];
				if ( pBeam ) pBeam->die = 0;
			}
			m_Beams.clearitems();
		}
		else if (Params.size() >= 3)
		{
			int beamid = atoi(Params[1]);
			bool found_beam = (beamid <= (int)m_Beams.size() - 1) ? true : false;
			BEAM *pBeam = found_beam ? m_Beams[beamid] : NULL;

			/*
			bool found_beam =false;
			 for (int i = 0; i < m_Beams.size(); i++) 
			{
				Print("DEBUG: beam_update checking %i for %i\n",i,beamid);
				if ( m_Beams[i]->id == beamid )
				{
					pBeam = m_Beams[i];
					found_beam = true;
					break;
				}
			}
			*/
			if (found_beam && Params.size() >= 4)
			{
				//Print("DEBUG: beam_update applying %s %s\n",Params[2].c_str(),Params[3].c_str());
				//Print("DEBUG: beam %s %f %s\n",(!FBitSet(pBeam->flags,FBEAM_ISACTIVE)) ? "alive" : "dead",pBeam->die,Params[3].c_str());
				if (Params[2] == "start")
					pBeam->source = StringToVec(Params[3]);
				else if (Params[2] == "end")
					pBeam->target = StringToVec(Params[3]);
				else if (Params[2] == "delta")
					pBeam->delta = StringToVec(Params[3]); //whatever "delta" is
				else if (Params[2] == "width")
					pBeam->width = atof(Params[3]);
				else if (Params[2] == "amplitude")
					pBeam->amplitude = atof(Params[3]);
				else if (Params[2] == "color")
				{
					Vector col = StringToVec(Params[3]);
					pBeam->r = col.x;
					pBeam->g = col.y;
					pBeam->b = col.z;
				}
				else if (Params[2] == "brightness")
					pBeam->brightness = atof(Params[3]);
				else if (Params[2] == "speed")
					pBeam->speed = atof(Params[3]);
				else if (Params[2] == "life")
					pBeam->die = atof(Params[3]);
				else if (Params[2] == "framerate")
					pBeam->frameRate = atof(Params[3]);
				else if (Params[2] == "framecount")
					pBeam->frameCount = atoi(Params[3]); //need this if you change sprites
				//else if ( Params[2] == "segments" ) pBeam->segments = atoi(Params[3]); //no affect
				//else if ( Params[2] == "t" ) pBeam->t = atof(Params[3]); //no affect
				else if (Params[2] == "sprite")
					pBeam->modelIndex = CL_LoadModel(Params[3]);
				else if (Params[2] == "startent")
					(Params.size() >= 5) ? pBeam->startEntity = (atoi(Params[3]) & 0x0FFF) | ((atoi(Params[4]) & 0xF) << 12) : pBeam->startEntity = atoi(Params[3]);
				else if (Params[2] == "endent")
					(Params.size() >= 5) ? pBeam->endEntity = (atoi(Params[3]) & 0x0FFF) | ((atoi(Params[4]) & 0xF) << 12) : pBeam->endEntity = atoi(Params[3]);
				else if (Params[2] == "remove")
					pBeam->die = 0; //case we got extra pars due to pass params
			}
			else if (found_beam && Params.size() >= 3)
			{
				//Print("DEBUG: beam_update applying sing remove\n",Params[2].c_str());
				if (Params[2] == "remove")
				{
					pBeam->die = 0;
					//pBeam->flags == !(pBeam->flags & FBEAM_ISACTIVE);
				}
			}
			else
			{
				if (!found_beam)
				{
					Print("cleffect %s could not find beam #%i.\n", Params[0].c_str(), atoi(Params[1].c_str()));
				}
				else
				{
					Print("cleffect %s %s: not enough parameters.\n", Params[0].c_str(), Params[1].c_str());
				}
				return;
			}
		}
		else
		{
			Print("cleffect %s: not enough parameters.\n", Params[0].c_str());
			return;
		}
	}
	//Thothie JUN2011_30
	//ce <player|GM> <eventname> [params] x
	//(x on the end, as last paramter gets intermitently skewed by odd carriage return)
	//causes client to execute command on server (either through server side player script or GM)
	//this is recieved in client.cpp on server side.
	//(Note that client.cpp commands are not console commands, so you cannot use this to generate console commands.)
	//(kept short to save on lag / msg size)
	//example:
	else if (Params[0] == "ce")
	{
		//Thothie SEP2019_26 - cleffect ce security effort
		if ( Params.size() >= 3 )
		{
			/*if(Params[2].contains("clmsg"))
			{
				msstring sTemp = "ce";
				for (int i = 0; i < Params.size(); i++) //Thothie SEP2019_03 - fix need for "x" param (was Params.size()-1 )
				{
					if (i > 0)
					{
						if (i)
							sTemp += " ";
						sTemp += Params[i];
					}
				}
				sTemp += "\n";
				ServerCmd(sTemp.c_str());
			}*/
		}
	}
	//Thothie AUG2013_09 - Clientcmd work around
	//cleffect clientcmd <command_string>
	else if (Params[0] == "clientcmd")
	{
		//some commands getting filtered out from clientcmd in Steam Pipe HLDS
		//(work around, client side variant)
		msstring sTemp;
		sTemp = Params[1].c_str();
		sTemp += "\n";
		gEngfuncs.pfnClientCmd(sTemp.c_str());
	}

	//Thothie DEC2010_04 - lazy sparx
	//cleffect sparks <origin|modelidx> [attachment_idx]
	else if (Params[0] == "spark")
	{
		if (Params.size() < 2)
			return;
		//Print ("Effect %s %s\n",Params[0].c_str(),Params[1].c_str());
		if (Params[1].starts_with("("))
			gEngfuncs.pEfxAPI->R_SparkEffect(StringToVec(Params[1]), 8, -200, 200);
		else
		{
			cl_entity_t *clentity = gEngfuncs.GetEntityByIndex(atoi(Params[1]));
			int clattach = (Params.size() >= 3) ? atoi(Params[2]) : 0;
			gEngfuncs.pEfxAPI->R_SparkEffect((float *)&clentity->attachment[clattach], 8, -200, 200);
		}
	}
	//Thothie SEP2011_07 - Client Side Decals (Attempt2)
	else if (Params[0] == "decal")
	{
		if(Params[1] == "clearall") //Thothie OCT2019_10 - clear all decal option
		{
			//gEngfuncs.pEfxAPI->R_DecalRemoveAll( atoi(Params[2]) ); //prob needs to be local player
			//eff it, borrowing form NS:
			for ( int har = 0; har < 300; har++ )
			{
				gEngfuncs.pEfxAPI->R_DecalRemoveAll( har );
			}
		}else{
			//cleffect decal <index> <origin>
			int iDecalIndex = atoi(Params[1]);
			Vector vTraceStart = StringToVec(Params[2]);
			Vector vTraceEnd = StringToVec(Params[3]);

			int iFlags = 0;
			SetBits(iFlags, PM_WORLD_ONLY);
			pmtrace_s &pmtr = *gEngfuncs.PM_TraceLine((float *)&vTraceStart[0], (float *)&vTraceEnd[0], iFlags, 2, iFlags);
	
			gEngfuncs.pEfxAPI->R_DecalShoot(
			gEngfuncs.pEfxAPI->Draw_DecalIndex(iDecalIndex),
			gEngfuncs.pEventAPI->EV_IndexFromTrace(&pmtr), 0, vTraceEnd, 0);
		}
	}
	else if (Params[0] == "bleed")
	{
		if (Params.size() < 4)
			return;

		//cleffect bleed <red|green|yellow> <origin> <amt>
		int bld_col = BLOOD_COLOR_RED;
		int bld_dec = 13 + RANDOM_LONG(0, 5);
		if (Params[1] == "green")
		{
			bld_col = BLOOD_COLOR_GREEN;
			bld_dec = 19 + RANDOM_LONG(0, 5);
		}
		else if (Params[1] == "yellow")
		{
			bld_col = BLOOD_COLOR_YELLOW;
			bld_dec = 19 + RANDOM_LONG(0, 5);
		}

		Vector bld_org = StringToVec(Params[2]);
		bld_org.x += RANDOM_FLOAT(-10, 10);
		bld_org.y += RANDOM_FLOAT(-10, 10);
		bld_org.z += RANDOM_FLOAT(-10, 10);
		Vector bld_dir; // = StringToVec(Params[3]);
		bld_dir.x = RANDOM_FLOAT(-1, 1);
		bld_dir.y = RANDOM_FLOAT(-1, 1);
		bld_dir.z = RANDOM_FLOAT(0, 1);
		int bld_amt = atoi(Params[3]);
		gEngfuncs.pEfxAPI->R_BloodStream(bld_org, bld_dir, (bld_col == BLOOD_COLOR_RED) ? 70 : bld_col, V_min(bld_amt, 255));

		int iFlags = 0;
		SetBits(iFlags, PM_WORLD_ONLY);
		Vector bld_trc_end = bld_org + bld_dir * bld_amt * 2;
		pmtrace_s& pmtr = *gEngfuncs.PM_TraceLine((float*)&bld_org[0], (float*)&bld_trc_end[0], iFlags, 2, iFlags);
		gEngfuncs.pEfxAPI->R_DecalShoot(gEngfuncs.pEfxAPI->Draw_DecalIndex(bld_dec), gEngfuncs.pEventAPI->EV_IndexFromTrace(&pmtr), 0, bld_trc_end, 0);
	}
}

void SetClEntityProp(cl_entity_t &Ent, msstring &Cmd, mslist<msstring *> &Params)
{
	if (!Params.size())
		return;
	msstring &Value = *Params[0];
	if (Cmd == "origin")
		Ent.origin = Ent.curstate.origin = StringToVec(Value);
	else if (Cmd == "origin.x")
		Ent.origin.x = atof(Value);
	else if (Cmd == "origin.y")
		Ent.origin.y = atof(Value);
	else if (Cmd == "origin.z")
		Ent.origin.z = atof(Value);
	else if (Cmd == "velocity")
		Ent.baseline.origin = StringToVec(Value); //Velocity is stored in entity.baseline.origin... why? ASK VALVE
	else if (Cmd == "velocity.x")
		Ent.baseline.origin.x = atof(Value);
	else if (Cmd == "velocity.y")
		Ent.baseline.origin.y = atof(Value);
	else if (Cmd == "velocity.z")
		Ent.baseline.origin.z = atof(Value);
	else if (Cmd == "angles")
		Ent.angles = StringToVec(Value);
	else if (Cmd == "angles.pitch")
		Ent.angles.x = atof(Value);
	else if (Cmd == "angles.yaw")
		Ent.angles.y = atof(Value);
	else if (Cmd == "angles.roll")
		Ent.angles.z = atof(Value);
	else if (Cmd == "model" || Cmd == "sprite")
	{
		int modelindex = 0;
		msstring FullName = GetFullResourceName(Value);
		Ent.model = gEngfuncs.CL_LoadModel(FullName, &modelindex);
	}
	else if (Cmd == "maxs")
		Ent.curstate.maxs = StringToVec(Value);
	else if (Cmd == "mins")
		Ent.curstate.mins = StringToVec(Value);
	else if (Cmd == "gravity")
		Ent.curstate.gravity = atof(Value);
	else if (Cmd == "owner")
		Ent.curstate.owner = atoi(Value);
	else if (Cmd == "follow")
	{
		if (Value != "none")
		{
			int Parent = atoi(Value);
			Ent.curstate.skin = Parent;
			Ent.curstate.owner = Parent;
			Ent.curstate.aiment = Parent;
			if (Params.size() >= 2)
				Ent.curstate.body = atoi(*Params[1]) + 1;
			else
				Ent.curstate.body = 1; //First attachment is 1
			Ent.curstate.movetype = MOVETYPE_FOLLOW;
			ClearBits(Ent.curstate.iuser4, MSTEMPENT_GRAVITY);
			SetBits(Ent.curstate.iuser4, MSTEMPENT_FOLLOWENT);
		}
		else
		{
			Ent.curstate.skin = 0;
			Ent.curstate.owner = 0;
			Ent.curstate.aiment = 0;
			Ent.curstate.body = 0;
			Ent.curstate.movetype = MOVETYPE_NONE;
			ClearBits(Ent.curstate.iuser4, MSTEMPENT_GRAVITY);
		}
	}
	else if (Cmd == "body")
		Ent.curstate.body = atoi(Value);
	else if (Cmd == "skin")
		Ent.curstate.skin = atoi(Value);
	else if (Cmd == "aiment")
		Ent.curstate.aiment = atoi(Value);
	else if (Cmd == "movetype")
		Ent.curstate.movetype = atoi(Value);
	else if (Cmd == "scale")
		Ent.curstate.scale = atof(Value);
	//dealing with the fact that MSC can't handle floats more than 2f
	else if (Cmd == "scaleHD")
		Ent.curstate.scale = atof(Value) * 0.001;
	else if (Cmd == "fuser1HD")
		Ent.curstate.fuser1 = atof(Value) * 0.001;
	else if (Cmd == "framerate")
		Ent.curstate.framerate = atof(Value);
	else if (Cmd == "frame")
		Ent.curstate.frame = atof(Value);
	else if (Cmd == "update")
		Ent.curstate.iuser4 = atoi(Value) ? SetBits(Ent.curstate.iuser4, MSTEMPENT_CALLBACK) : ClearBits(Ent.curstate.iuser4, MSTEMPENT_CALLBACK);
	else if (Cmd == "rendermode")
	{
		if (Value.find("normal") != msstring_error)
			Ent.curstate.rendermode = kRenderNormal;
		else if (Value.find("color") != msstring_error)
			Ent.curstate.rendermode = kRenderTransColor;
		else if (Value.find("texture") != msstring_error)
			Ent.curstate.rendermode = kRenderTransTexture;
		else if (Value.find("glow") != msstring_error)
			Ent.curstate.rendermode = kRenderGlow;
		else if (Value.find("alpha") != msstring_error)
			Ent.curstate.rendermode = kRenderTransAlpha;
		else if (Value.find("add") != msstring_error)
			Ent.curstate.rendermode = kRenderTransAdd;
	}
	else if (Cmd == "renderamt")
		Ent.curstate.renderamt = atof(Value);
	else if (Cmd == "renderfx")
	{
		if (Value.find("normal") != msstring_error)
			Ent.curstate.renderfx = kRenderFxNone;
		else if (Value.find("glow") != msstring_error)
			Ent.curstate.renderfx = kRenderFxGlowShell;
		else if (Value.find("player") != msstring_error)
			Ent.curstate.rendermode = kRenderFxDeadPlayer;
	}
	else if (Cmd == "rendercolor")
	{
		Vector Color = StringToVec(Value);
		Ent.curstate.rendercolor.r = Color.x;
		Ent.curstate.rendercolor.g = Color.y;
		Ent.curstate.rendercolor.b = Color.z;
	}
	else if (Cmd == "sequence")
		Ent.curstate.sequence = atoi(Value); //FEB2009_19
	else if (Cmd == "iuser1")
		Ent.curstate.iuser1 = atoi(Value);
	else if (Cmd == "iuser2")
		Ent.curstate.iuser2 = atoi(Value);
	else if (Cmd == "iuser3")
		Ent.curstate.iuser3 = atoi(Value);
	else if (Cmd == "iuser4")
		Ent.curstate.iuser4 = atoi(Value);
	else if (Cmd == "fuser1")
		Ent.curstate.fuser1 = atof(Value);
	else if (Cmd == "fuser2")
		Ent.curstate.fuser2 = atof(Value);
	else if (Cmd == "fuser3")
		Ent.curstate.fuser3 = atof(Value);
	else if (Cmd == "fuser4")
		Ent.curstate.fuser4 = atof(Value);
	else if (Cmd == "prevstate.iuser1")
		Ent.prevstate.iuser1 = atoi(Value);
	else if (Cmd == "prevstate.iuser2")
		Ent.prevstate.iuser2 = atoi(Value);
	else if (Cmd == "prevstate.iuser3")
		Ent.prevstate.iuser3 = atoi(Value);
	else if (Cmd == "prevstate.iuser4")
		Ent.prevstate.iuser4 = atoi(Value);
	else if (Cmd == "prevstate.fuser1")
		Ent.prevstate.fuser1 = atof(Value);
	else if (Cmd == "prevstate.fuser2")
		Ent.prevstate.fuser2 = atof(Value);
	else if (Cmd == "prevstate.fuser3")
		Ent.prevstate.fuser3 = atof(Value);
	else if (Cmd == "prevstate.fuser4")
		Ent.prevstate.fuser4 = atof(Value);
	else if (Cmd == "baseline.iuser1")
		Ent.baseline.iuser1 = atoi(Value);
	else if (Cmd == "baseline.iuser2")
		Ent.baseline.iuser2 = atoi(Value);
	else if (Cmd == "baseline.iuser3")
		Ent.baseline.iuser3 = atoi(Value);
	else if (Cmd == "baseline.iuser4")
		Ent.baseline.iuser4 = atoi(Value);
	else if (Cmd == "baseline.fuser1")
		Ent.baseline.fuser1 = atof(Value);
	else if (Cmd == "baseline.fuser2")
		Ent.baseline.fuser2 = atof(Value);
	else if (Cmd == "baseline.fuser3")
		Ent.baseline.fuser3 = atof(Value);
	else if (Cmd == "baseline.fuser4")
		Ent.baseline.fuser4 = atof(Value);
}

/*
=========================
HUD_StudioEvent

The entity's studio model description indicated an event was
fired during this frame, handle the event by it's tag ( e.g., muzzleflash, sound )
=========================
*/
void ViewModel_InactiveModelVisible(bool fVisible, const cl_entity_s *ActiveEntity);
void DLLEXPORT HUD_StudioEvent(const struct mstudioevent_s *event, const struct cl_entity_s *entity)
{
	DBG_INPUT;
	startdbg;

	dbg("Begin");
	switch (event->event)
	{
	case 5001:
		gEngfuncs.pEfxAPI->R_MuzzleFlash((float *)&entity->attachment[0], atoi(event->options));
		break;
	case 5011:
		gEngfuncs.pEfxAPI->R_MuzzleFlash((float *)&entity->attachment[1], atoi(event->options));
		break;
	case 5021:
		gEngfuncs.pEfxAPI->R_MuzzleFlash((float *)&entity->attachment[2], atoi(event->options));
		break;
	case 5031:
		gEngfuncs.pEfxAPI->R_MuzzleFlash((float *)&entity->attachment[3], atoi(event->options));
		break;
	case 5002:
		gEngfuncs.pEfxAPI->R_SparkEffect((float *)&entity->attachment[0], atoi(event->options), -100, 100);
		break;
	// Client side sound
	case 5004:
		gEngfuncs.pfnPlaySoundByNameAtLocation((char *)event->options, 1.0, (float *)&entity->attachment[0]);
		break;
	case 8000:
		if (gHUD.m_HUDScript)
			gHUD.m_HUDScript->HandleAnimEvent(event->options, entity, HAE_EITHER);
		break;
	case 8001:
		if (gHUD.m_HUDScript)
			gHUD.m_HUDScript->HandleAnimEvent(event->options, entity, HAE_NEW);
		break;
	case 8002:
		if (gHUD.m_HUDScript)
			gHUD.m_HUDScript->HandleAnimEvent(event->options, entity, HAE_ATTACH);
		break;
	case 8500:
		if (event->options)
		{
			if (!_stricmp(event->options, "twohandanim"))
				ViewModel_InactiveModelVisible(true, entity);
			else if (!_stricmp(event->options, "onehandanim"))
				ViewModel_InactiveModelVisible(false, entity);
		}
		break;
	default:
		break;
	}
	enddbg;
}

/*
=================
CL_UpdateTEnts

Simulation and cleanup of temporary entities
=================
*/
int g_TempEntCount = 0;
void DLLEXPORT HUD_TempEntUpdate(
	double frametime,			  // Simulation time
	double client_time,			  // Absolute time on client
	double cl_gravity,			  // True gravity on client
	TEMPENTITY **ppTempEntFree,	  // List of freed temporary ents
	TEMPENTITY **ppTempEntActive, // List
	int (*Callback_AddVisibleEntity)(cl_entity_t *pEntity),
	void (*Callback_TempEntPlaySound)(TEMPENTITY *pTemp, float damp))
{
	DBG_INPUT;

	static int gTempEntFrame = 0;
	int i;
	TEMPENTITY *pTemp, *pnext, *pprev;
	float freq, CommonGravity, gravitySlow, life, fastFreq;

	startdbg;

	dbg("Begin");

	// Nothing to simulate
	if (!*ppTempEntActive)
	{
		if (g_TempEntNewLevel)
		{
			for (int i = 0; i < MAX_TEMPENT_EXTRA; i++) //On level change, this is called.  Clear all tempent extra data from last level
				clrmem(g_TempEntExtra[i]);

			g_TempEntNewLevel = false;
		}
		return;
	}

	// in order to have tents collide with players, we have to run the player prediction code so
	// that the client has the player list. We run this code once when we detect any COLLIDEALL
	// tent, then set this BOOL to true so the code doesn't get run again if there's more than
	// one COLLIDEALL ent for this update. (often are).
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);

	// !!!BUGBUG	-- This needs to be time based
	gTempEntFrame = (gTempEntFrame + 1) & 31;

	pTemp = *ppTempEntActive;

	// !!! Don't simulate while paused....  This is sort of a hack, revisit.
	if (frametime <= 0)
	{
		while (pTemp)
		{
			if (!(pTemp->flags & FTENT_NOMODEL))
			{
				Callback_AddVisibleEntity(&pTemp->entity);
			}
			pTemp = pTemp->next;
		}
		goto finish;
	}

	pprev = NULL;
	freq = client_time * 0.01;
	fastFreq = client_time * 5.5;
	CommonGravity = -frametime * cl_gravity;
	gravitySlow = CommonGravity * 0.5;

	g_TempEntCount = 0;
	while (pTemp)
	{
		g_TempEntCount++;
		int active;
		float gravity = CommonGravity;
		if (FBitSet(pTemp->entity.curstate.iuser4, MSTEMPENT_GRAVITY)) // Script altered this tempent's gravity
			gravity *= pTemp->entity.curstate.gravity;

		active = 1;
		life = pTemp->die - client_time;

		pnext = pTemp->next;

		try
		{
			if (life < 0)
			{
				if (pTemp->flags & FTENT_FADEOUT)
				{
					if (pTemp->entity.curstate.rendermode == kRenderNormal)
						pTemp->entity.curstate.rendermode = kRenderTransTexture;
					pTemp->entity.curstate.renderamt = pTemp->entity.baseline.renderamt * (1 + life * pTemp->fadeSpeed);
					if (pTemp->entity.curstate.renderamt <= 0)
						active = 0;
				}
				else
					active = 0;
			}

			if (pTemp->entity.curstate.weaponanim)
			{
				tempentextra_t &TempEntExtra = g_TempEntExtra[pTemp->entity.curstate.weaponanim];
				if (TempEntExtra.DieWithEntActive)
				{
					pTemp->die = gEngfuncs.GetClientTime() + 10;
					cl_entity_t *pOtherEnt = gEngfuncs.GetEntityByIndex(TempEntExtra.DieWithEnt);
					if (!pOtherEnt || !pOtherEnt->Exists())
					{
						active = 0;
						TempEntExtra.DieWithEntActive = false;
					}
				}
			}

			if (!active) // Kill it
			{
				pTemp->next = *ppTempEntFree;
				*ppTempEntFree = pTemp;
				if (!pprev) // Deleting at head of list
					*ppTempEntActive = pnext;
				else
					pprev->next = pnext;

				if (pTemp->entity.curstate.weaponanim)
				{
					tempentextra_t &TempEntExtra = g_TempEntExtra[pTemp->entity.curstate.weaponanim];
					TempEntExtra.Active = false;
				}
			}
			else
			{
				pprev = pTemp;

				//Save old origin
				VectorCopy(pTemp->entity.origin, pTemp->entity.prevstate.origin);

				//if Following, origin always equals attachment point
				if (FBitSet(pTemp->entity.curstate.iuser4, MSTEMPENT_FOLLOWENT))
				{
					cl_entity_t *pOtherEnt = gEngfuncs.GetEntityByIndex(pTemp->entity.curstate.aiment);
					if (pOtherEnt)
						pTemp->entity.origin = pOtherEnt->origin;
				}

				//Handle timer callback
				//=====================
				if (pTemp->entity.curstate.weaponanim)
				{
					tempentextra_t &TempExtra = g_TempEntExtra[pTemp->entity.curstate.weaponanim];
					if (TempExtra.CBTimer_Enabled)
						if (client_time >= TempExtra.CBTimer_TimeCallback)
						{
							TempExtra.CBTimer_Enabled = false;

							g_CurrentTempEnt = pTemp;
							HUDScript->Effects_UpdateTempEnt(TempExtra.CBTimer_CallbackEvent);
							g_CurrentTempEnt = NULL;
						}

					//Handle Fade
					//===========
					if (TempExtra.Fadeout)
					{
						float Elapsed = client_time - TempExtra.FadeStart;
						float Ratio = Elapsed / (TempExtra.FadeDuration ? TempExtra.FadeDuration : 1);
						if (pTemp->entity.curstate.rendermode == kRenderNormal)
							pTemp->entity.curstate.rendermode = kRenderTransTexture;
						pTemp->entity.curstate.renderamt = 255 * (1 - Ratio);
					}

					//Check water touch
					if (TempExtra.CBWater_Enabled)
						if (EngineFunc::Shared_PointContents(Vector(pTemp->entity.origin.x, pTemp->entity.origin.y, pTemp->entity.origin.z + pTemp->entity.curstate.mins.z)) == CONTENTS_WATER)
						{
							TempExtra.CBWater_Enabled = false;
							g_CurrentTempEnt = pTemp;
							HUDScript->Effects_UpdateTempEnt(TempExtra.CBWater_CallbackEvent);
							g_CurrentTempEnt = NULL;
						}
				}

				if (pTemp->flags & FTENT_SPARKSHOWER)
				{
					// Adjust speed if it's time
					// Scale is next think time
					if (client_time > pTemp->entity.baseline.scale)
					{
						// Show Sparks
						gEngfuncs.pEfxAPI->R_SparkEffect(pTemp->entity.origin, 8, -200, 200);

						// Reduce life
						pTemp->entity.baseline.framerate -= 0.1;

						if (pTemp->entity.baseline.framerate <= 0.0)
						{
							pTemp->die = client_time;
						}
						else
						{
							// So it will die no matter what
							pTemp->die = client_time + 0.5;

							// Next think
							pTemp->entity.baseline.scale = client_time + 0.1;
						}
					}
				}
				else if (pTemp->flags & FTENT_PLYRATTACHMENT)
				{
					cl_entity_t *pClient;

					pClient = gEngfuncs.GetEntityByIndex(pTemp->clientIndex);

					VectorAdd(pClient->origin, pTemp->tentOffset, pTemp->entity.origin);
				}
				else if (pTemp->flags & FTENT_SINEWAVE)
				{
					pTemp->x += pTemp->entity.baseline.origin[0] * frametime;
					pTemp->y += pTemp->entity.baseline.origin[1] * frametime;

					pTemp->entity.origin[0] = pTemp->x + sin(pTemp->entity.baseline.origin[2] + client_time * pTemp->entity.prevstate.frame) * (10 * pTemp->entity.curstate.framerate);
					pTemp->entity.origin[1] = pTemp->y + sin(pTemp->entity.baseline.origin[2] + fastFreq + 0.7) * (8 * pTemp->entity.curstate.framerate);
					pTemp->entity.origin[2] += pTemp->entity.baseline.origin[2] * frametime;
				}
				else if (pTemp->flags & FTENT_SPIRAL)
				{
					float s, c;
					s = sin(pTemp->entity.baseline.origin[2] + fastFreq);
					c = cos(pTemp->entity.baseline.origin[2] + fastFreq);

					pTemp->entity.origin[0] += pTemp->entity.baseline.origin[0] * frametime + 8 * sin(client_time * 20 + (int)pTemp);
					pTemp->entity.origin[1] += pTemp->entity.baseline.origin[1] * frametime + 4 * sin(client_time * 30 + (int)pTemp);
					pTemp->entity.origin[2] += pTemp->entity.baseline.origin[2] * frametime;
				}

				else
				{
					for (i = 0; i < 3; i++)
						pTemp->entity.origin[i] += pTemp->entity.baseline.origin[i] * frametime;
				}

				if (pTemp->flags & FTENT_SPRANIMATE)
				{
					pTemp->entity.curstate.frame += frametime * pTemp->entity.curstate.framerate;
					if (pTemp->entity.curstate.frame >= pTemp->frameMax)
					{
						pTemp->entity.curstate.frame = pTemp->entity.curstate.frame - (int)(pTemp->entity.curstate.frame);

						if (!(pTemp->flags & FTENT_SPRANIMATELOOP))
						{
							// this animating sprite isn't set to loop, so destroy it.
							pTemp->die = client_time;
							pTemp = pnext;
							continue;
						}
					}
				}
				else if (pTemp->flags & FTENT_SPRCYCLE)
				{
					pTemp->entity.curstate.frame += frametime * 10;
					if (pTemp->entity.curstate.frame >= pTemp->frameMax)
					{
						pTemp->entity.curstate.frame = pTemp->entity.curstate.frame - (int)(pTemp->entity.curstate.frame);
					}
				}
// Experiment
#if 0
			if ( pTemp->flags & FTENT_SCALE )
				pTemp->entity.curstate.framerate += 20.0 * (frametime / pTemp->entity.curstate.framerate);
#endif

				if (pTemp->flags & FTENT_ROTATE)
				{
					pTemp->entity.angles[0] += pTemp->entity.baseline.angles[0] * frametime;
					pTemp->entity.angles[1] += pTemp->entity.baseline.angles[1] * frametime;
					pTemp->entity.angles[2] += pTemp->entity.baseline.angles[2] * frametime;

					VectorCopy(pTemp->entity.angles, pTemp->entity.latched.prevangles);
				}

				if (pTemp->flags & (FTENT_COLLIDEALL | FTENT_COLLIDEWORLD))
				{
					vec3_t traceNormal;
					float traceFraction = 1;

					if (pTemp->flags & FTENT_COLLIDEALL)
					{
						pmtrace_t pmtrace;
						physent_t *pe;

						gEngfuncs.pEventAPI->EV_SetTraceHull(2);

						gEngfuncs.pEventAPI->EV_PlayerTrace(pTemp->entity.prevstate.origin, pTemp->entity.origin, PM_STUDIO_BOX, -1, &pmtrace);

						if (pmtrace.fraction != 1)
						{
							pe = gEngfuncs.pEventAPI->EV_GetPhysent(pmtrace.ent);
							//AUG2013_25 - seeing if we can make cl projectile skip its owner
							bool skip_ent = false;
							if (pTemp->flags & (FTENT_SKIPENT))
							{
								int iSkipent = pTemp->entity.curstate.iuser1;
								if (iSkipent && pe->info == iSkipent)
									skip_ent = true;
							}
							if (!skip_ent)
							{
								if (!pmtrace.ent || (pe->info != pTemp->clientIndex))
								{
									traceFraction = pmtrace.fraction;
									VectorCopy(pmtrace.plane.normal, traceNormal);

									if (pTemp->hitcallback)
									{
										pmtrace.ent = pe->info; //AUG2013_24 Thothie - make collide callback return model (bit of a hack to store the info here, but works.)
										(*pTemp->hitcallback)(pTemp, &pmtrace);
									}
								}
							}
						}
					}
					else if (pTemp->flags & FTENT_COLLIDEWORLD)
					{
						pmtrace_t pmtrace;

						gEngfuncs.pEventAPI->EV_SetTraceHull(2);

						gEngfuncs.pEventAPI->EV_PlayerTrace(pTemp->entity.prevstate.origin, pTemp->entity.origin, PM_STUDIO_BOX | PM_WORLD_ONLY, -1, &pmtrace);

						if (pmtrace.fraction != 1)
						{
							traceFraction = pmtrace.fraction;
							VectorCopy(pmtrace.plane.normal, traceNormal);

							if (pTemp->flags & FTENT_SPARKSHOWER)
							{
								// Chop spark speeds a bit more
								//
								VectorScale(pTemp->entity.baseline.origin, 0.6, pTemp->entity.baseline.origin);

								if (Length(pTemp->entity.baseline.origin) < 10)
								{
									pTemp->entity.baseline.framerate = 0.0;
								}
							}

							if (pTemp->hitcallback)
							{
								(*pTemp->hitcallback)(pTemp, &pmtrace);
							}
						}
					}

					if (traceFraction != 1) // Decent collision now, and damping works
					{
						float proj, damp;

						// Place at contact point
						VectorMA(pTemp->entity.prevstate.origin, traceFraction * frametime, pTemp->entity.baseline.origin, pTemp->entity.origin);
						// Damp velocity
						damp = pTemp->bounceFactor;
						if (pTemp->flags & (FTENT_GRAVITY | FTENT_SLOWGRAVITY))
						{
							damp *= 0.5;
							if (traceNormal[2] > 0.9) // Hit floor?
							{
								if (pTemp->entity.baseline.origin[2] <= 0 && pTemp->entity.baseline.origin[2] >= gravity * 3)
								{
									damp = 0; // Stop
									pTemp->flags &= ~(FTENT_ROTATE | FTENT_GRAVITY | FTENT_SLOWGRAVITY | FTENT_COLLIDEWORLD | FTENT_SMOKETRAIL);
									pTemp->entity.angles[0] = 0;
									pTemp->entity.angles[2] = 0;
								}
							}
						}

						if (pTemp->hitSound)
						{
							Callback_TempEntPlaySound(pTemp, damp);
						}

						if (pTemp->flags & FTENT_COLLIDEKILL)
						{
							// die on impact
							pTemp->flags &= ~FTENT_FADEOUT;
							pTemp->die = client_time;
						}
						else
						{
							// Reflect velocity
							if (damp != 0)
							{
								proj = DotProduct(pTemp->entity.baseline.origin, traceNormal);
								VectorMA(pTemp->entity.baseline.origin, -proj * 2, traceNormal, pTemp->entity.baseline.origin);
								// Reflect rotation (fake)

								pTemp->entity.angles[1] = -pTemp->entity.angles[1];
							}

							if (damp != 1)
							{

								VectorScale(pTemp->entity.baseline.origin, damp, pTemp->entity.baseline.origin);
								VectorScale(pTemp->entity.angles, 0.9, pTemp->entity.angles);
							}
						}
					}
				}

				if ((pTemp->flags & FTENT_FLICKER) && gTempEntFrame == pTemp->entity.curstate.effects)
				{
					dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
					VectorCopy(pTemp->entity.origin, dl->origin);
					dl->radius = 60;
					dl->color.r = 255;
					dl->color.g = 120;
					dl->color.b = 0;
					dl->die = client_time + 0.01;
				}

				if (pTemp->flags & FTENT_SMOKETRAIL)
				{
					gEngfuncs.pEfxAPI->R_RocketTrail(pTemp->entity.prevstate.origin, pTemp->entity.origin, 1);
				}

				if (pTemp->flags & FTENT_GRAVITY)
					pTemp->entity.baseline.origin[2] += gravity;
				else if (pTemp->flags & FTENT_SLOWGRAVITY)
					pTemp->entity.baseline.origin[2] += gravitySlow;

				if (pTemp->flags & FTENT_CLIENTCUSTOM)
				{
					if (pTemp->callback)
					{
						(*pTemp->callback)(pTemp, frametime, client_time);
					}
				}

				// Cull to PVS (not frustum cull, just PVS)
				if (!(pTemp->flags & FTENT_NOMODEL))
				{
					if (!Callback_AddVisibleEntity(&pTemp->entity))
					{
						if (!(pTemp->flags & FTENT_PERSIST))
						{
							pTemp->die = client_time;		// If we can't draw it this frame, just dump it.
							pTemp->flags &= ~FTENT_FADEOUT; // Don't fade out, just die
						}
					}
				}
			}
			pTemp->entity.baseline.vuser1 = pTemp->entity.baseline.origin;
		}
		catch (...)
		{
		}

		pTemp = pnext;
	}

finish:
	// Restore state info
	gEngfuncs.pEventAPI->EV_PopPMStates();
	enddbg;
}

/*
=================
HUD_GetUserEntity

If you specify negative numbers for beam start and end point entities, then
  the engine will call back into this function requesting a pointer to a cl_entity_t 
  object that describes the entity to attach the beam onto.

Indices must start at 1, not zero.
=================
*/
cl_entity_t DLLEXPORT *HUD_GetUserEntity(int index)
{
	DBG_INPUT;
	startdbg;

	dbg("Begin");
#if defined(BEAM_TEST)
	// None by default, you would return a valic pointer if you create a client side
	//  beam and attach it to a client side entity.
	if (index > 0 && index <= 1)
	{
		return &beams[index];
	}
#endif
	enddbg;
	return NULL;
}
