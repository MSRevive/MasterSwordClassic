//========= Copyright � 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// studio_model.cpp
// routines for setting up to draw 3DStudio models

#include "inc_weapondefs.h"
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio.h"
#include "entity_state.h"
#include "entity_types.h"
#include "cl_entity.h"
#include "dlight.h"
#include "triangleapi.h"
#include "script.h"

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include "studio_util.h"
#include "r_studioint.h"

#include "studiomodelrenderer.h"
#include "gamestudiomodelrenderer.h"
#include "logger.h"
#include "clrender.h"
#include "ms/clglobal.h"
#include "ms/hudscript.h"
#include <GL/gl.h> // Header File For The OpenGL32 Library

extern "C" int nanmask;
#define IS_NAN(x) (((*(int *)&x) & nanmask) == nanmask)

// Global engine <-> studio model rendering code interface
engine_studio_api_t IEngineStudio;

modelinfo_t CModelMgr::m_ModelInfo[4096];

void Print(const char* szFmt, ...);
void VectorAngles(const float* forward, float* angles);

int ViewModel_ExclusiveViewHand = -1;
void ViewModel_InactiveModelVisible(bool fVisible, const cl_entity_s* ActiveEntity)
{
	ViewModel_ExclusiveViewHand = fVisible ? ActiveEntity->curstate.iuser2 : -1;
}
extern vec3_t v_origin, v_angles, v_cl_angles, v_sim_org, v_lastAngles;
//CStudioModelRenderer *g_StudioRender = NULL;

cl_entity_t* DrawEnt = NULL;

/////////////////////
// Implementation of CStudioModelRenderer.h

/*
====================
Init

====================
*/
void CStudioModelRenderer::Init(void)
{
	// Set up some variables shared with engine
	m_pCvarHiModels = IEngineStudio.GetCvar("cl_himodels");
	m_pCvarDeveloper = IEngineStudio.GetCvar("developer");
	m_pCvarDrawEntities = IEngineStudio.GetCvar("r_drawentities");

	m_pChromeSprite = IEngineStudio.GetChromeSprite();

	IEngineStudio.GetModelCounters(&m_pStudioModelCount, &m_pModelsDrawn);

	// Get pointers to engine data structures
	m_pbonetransform = (float(*)[MAXSTUDIOBONES][3][4])IEngineStudio.StudioGetBoneTransform();
	m_plighttransform = (float(*)[MAXSTUDIOBONES][3][4])IEngineStudio.StudioGetLightTransform();
	m_paliastransform = (float(*)[3][4])IEngineStudio.StudioGetAliasTransform();
	m_protationmatrix = (float(*)[3][4])IEngineStudio.StudioGetRotationMatrix();
}

/*
====================
CStudioModelRenderer

====================
*/
CStudioModelRenderer::CStudioModelRenderer(void)
{
	m_fDoInterp = 1;
	m_fGaitEstimation = 1;
	m_pCurrentEntity = NULL;
	m_pCvarHiModels = NULL;
	m_pCvarDeveloper = NULL;
	m_pCvarDrawEntities = NULL;
	m_pChromeSprite = NULL;
	m_pStudioModelCount = NULL;
	m_pModelsDrawn = NULL;
	m_protationmatrix = NULL;
	m_paliastransform = NULL;
	m_pbonetransform = NULL;
	m_plighttransform = NULL;
	m_pStudioHeader = NULL;
	m_pBodyPart = NULL;
	m_pSubModel = NULL;
	m_pPlayerInfo = NULL;
	m_pRenderModel = NULL;
	//m_MirrorRender		= false;
	/*AngleMatrix( g_vecZero, m_FlipMatrix );
	float scalemat[ 3 ][ 4 ] =
	{
		-1,  0, 0, 0,
		 0,  1, 0, 0,
		 0,  0, 1, 0,
	};
	ConcatTransforms( m_FlipMatrix, scalemat, m_FlipMatrix );*/
}

/*
====================
~CStudioModelRenderer

====================
*/
CStudioModelRenderer::~CStudioModelRenderer(void)
{
}

/*
====================
StudioCalcBoneAdj

====================
*/
void CStudioModelRenderer::StudioCalcBoneAdj(float dadt, float* adj, const byte* pcontroller1, const byte* pcontroller2, byte mouthopen)
{
	int i, j;
	float value;
	mstudiobonecontroller_t* pbonecontroller;

	pbonecontroller = (mstudiobonecontroller_t*)((byte*)m_pStudioHeader + m_pStudioHeader->bonecontrollerindex);

	for (j = 0; j < m_pStudioHeader->numbonecontrollers; j++)
	{
		i = pbonecontroller[j].index;
		if (i <= 3)
		{
			// check for 360% wrapping
			if (pbonecontroller[j].type & STUDIO_RLOOP)
			{
				if (abs(pcontroller1[i] - pcontroller2[i]) > 128)
				{
					int a, b;
					a = (pcontroller1[j] + 128) % 256;
					b = (pcontroller2[j] + 128) % 256;
					value = ((a * dadt) + (b * (1 - dadt)) - 128) * (360.0 / 256.0) + pbonecontroller[j].start;
				}
				else
				{
					value = ((pcontroller1[i] * dadt + (pcontroller2[i]) * (1.0 - dadt))) * (360.0 / 256.0) + pbonecontroller[j].start;
				}
			}
			else
			{
				value = (pcontroller1[i] * dadt + pcontroller2[i] * (1.0 - dadt)) / 255.0;
				if (value < 0)
					value = 0;
				if (value > 1.0)
					value = 1.0;
				value = (1.0 - value) * pbonecontroller[j].start + value * pbonecontroller[j].end;
			}
			// Con_DPrintf( "%d %d %f : %f\n", m_pCurrentEntity->curstate.controller[j], m_pCurrentEntity->latched.prevcontroller[j], value, dadt );
		}
		else
		{
			value = mouthopen / 64.0;
			if (value > 1.0)
				value = 1.0;
			value = (1.0 - value) * pbonecontroller[j].start + value * pbonecontroller[j].end;
			// Con_DPrintf("%d %f\n", mouthopen, value );
		}
		switch (pbonecontroller[j].type & STUDIO_TYPES)
		{
		case STUDIO_XR:
		case STUDIO_YR:
		case STUDIO_ZR:
			adj[j] = value * (M_PI / 180.0);
			break;
		case STUDIO_X:
		case STUDIO_Y:
		case STUDIO_Z:
			adj[j] = value;
			break;
		}
	}
}

/*
====================
StudioCalcBoneQuaterion

====================
*/
struct bonemodify_t
{
	int Bone;
	Vector AddRotation;
};
mslist<bonemodify_t> m_BoneMod;
bool g_UseBoneMods = false;
int g_CurrentBone = 0;

void CStudioModelRenderer::StudioCalcBoneQuaterion(int frame, float s, mstudiobone_t* pbone, mstudioanim_t* panim, float* adj, float* q)
{
	int j, k;
	vec4_t q1, q2;
	vec3_t angle1, angle2;
	mstudioanimvalue_t* panimvalue;

	for (j = 0; j < 3; j++)
	{
		if (panim->offset[j + 3] == 0)
		{
			angle2[j] = angle1[j] = pbone->value[j + 3]; // default;
		}
		else
		{
			panimvalue = (mstudioanimvalue_t*)((byte*)panim + panim->offset[j + 3]);
			k = frame;
			// DEBUG
			if (panimvalue->num.total < panimvalue->num.valid)
				k = 0;
			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
				// DEBUG
				if (panimvalue->num.total < panimvalue->num.valid)
					k = 0;
			}
			// Bah, missing blend!
			if (panimvalue->num.valid > k)
			{
				angle1[j] = panimvalue[k + 1].value;

				if (panimvalue->num.valid > k + 1)
				{
					angle2[j] = panimvalue[k + 2].value;
				}
				else
				{
					if (panimvalue->num.total > k + 1)
						angle2[j] = angle1[j];
					else
						angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
				}
			}
			else
			{
				angle1[j] = panimvalue[panimvalue->num.valid].value;
				if (panimvalue->num.total > k + 1)
				{
					angle2[j] = angle1[j];
				}
				else
				{
					angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
				}
			}
			/*if( FBitSet(m_pCurrentEntity->curstate.oldbuttons,MSRDR_FLIPPED) && j > 0 )
			{
				float orig1 = pbone->value[j+3];
				for( int a = 0; a < 2; a++ )
				{
					Vector &angle = !a ? angle1 : angle2;

					float animrot = angle[j] * pbone->scale[j+3];
					if( pbone->parent == -1 )
					{
						animrot += orig1;
						animrot -= M_PI/2;
					}
					animrot *= -1;

					if( pbone->parent == -1 )
					{
						animrot += M_PI/2;
						animrot -= (-orig1);
					}

					angle[j] = (-orig1) + animrot;
				}
			}
			else*/
			{
				angle1[j] = pbone->value[j + 3] + angle1[j] * pbone->scale[j + 3];
				angle2[j] = pbone->value[j + 3] + angle2[j] * pbone->scale[j + 3];
			}
		}

		if (pbone->bonecontroller[j + 3] != -1)
		{
			angle1[j] += adj[pbone->bonecontroller[j + 3]];
			angle2[j] += adj[pbone->bonecontroller[j + 3]];
		}
	}

	//Master Sword - do dynamic operations on bones
	for (int a = 0; a < 2; a++)
	{
		Vector& angle = !a ? angle1 : angle2;
		ModifyBone(pbone, angle);

		//Mirror the bone rotation for flipped models
		/*if( FBitSet(m_pCurrentEntity->curstate.oldbuttons,MSRDR_FLIPPED) )
			if( pbone->parent != -1 )
			{
				if( angle[0] > M_PI ) angle[0] -= M_PI*2;
				if( angle[0] < -M_PI ) angle[0] += M_PI*2;
				 for (int j = 0; j < 2; j++)
					if( panim->offset[j+1+3] == 0 )
					{
						angle[j+1] *= -1;
					}
			}*/
	}
	//----------

	if (!VectorCompare(angle1, angle2))
	{
		AngleQuaternion(angle1, q1);
		AngleQuaternion(angle2, q2);
		QuaternionSlerp(q1, q2, s, q);
	}
	else
	{
		AngleQuaternion(angle1, q);
	}
}

/*
====================
StudioCalcBonePosition

====================
*/
void CStudioModelRenderer::StudioCalcBonePosition(int frame, float s, mstudiobone_t* pbone, mstudioanim_t* panim, float* adj, float* pos)
{
	int j, k;
	mstudioanimvalue_t* panimvalue;

	for (j = 0; j < 3; j++)
	{
		pos[j] = pbone->value[j]; // default;

		if (panim->offset[j] != 0)
		{
			panimvalue = (mstudioanimvalue_t*)((byte*)panim + panim->offset[j]);
			/*
			if (i == 0 && j == 0)
				Con_DPrintf("%d  %d:%d  %f\n", frame, panimvalue->num.valid, panimvalue->num.total, s );
			*/

			k = frame;
			// DEBUG
			if (panimvalue->num.total < panimvalue->num.valid)
				k = 0;
			// find span of values that includes the frame we want
			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
				// DEBUG
				if (panimvalue->num.total < panimvalue->num.valid)
					k = 0;
			}
			// if we're inside the span
			if (panimvalue->num.valid > k)
			{
				// and there's more data in the span
				if (panimvalue->num.valid > k + 1)
				{
					pos[j] += (panimvalue[k + 1].value * (1.0 - s) + s * panimvalue[k + 2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[k + 1].value * pbone->scale[j];
				}
			}
			else
			{
				// are we at the end of the repeating values section and there's another section with data?
				if (panimvalue->num.total <= k + 1)
				{
					pos[j] += (panimvalue[panimvalue->num.valid].value * (1.0 - s) + s * panimvalue[panimvalue->num.valid + 2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[panimvalue->num.valid].value * pbone->scale[j];
				}
			}
		}
		if (pbone->bonecontroller[j] != -1 && adj)
		{
			pos[j] += adj[pbone->bonecontroller[j]];
		}
	}

	//Master Sword - Mirror the bone x position for flipped models
	/*if( FBitSet(m_pCurrentEntity->curstate.oldbuttons,MSRDR_FLIPPED) )
		if( pbone->parent != -1 )
			pos[0] *= -1;*/
}

/*
====================
StudioSlerpBones

====================
*/
void CStudioModelRenderer::StudioSlerpBones(vec4_t q1[], float pos1[][3], vec4_t q2[], float pos2[][3], float s)
{
	int i;
	vec4_t q3;
	float s1;

	if (s < 0)
		s = 0;
	else if (s > 1.0)
		s = 1.0;

	s1 = 1.0 - s;

	for (i = 0; i < m_pStudioHeader->numbones; i++)
	{
		QuaternionSlerp(q1[i], q2[i], s, q3);
		q1[i][0] = q3[0];
		q1[i][1] = q3[1];
		q1[i][2] = q3[2];
		q1[i][3] = q3[3];
		pos1[i][0] = pos1[i][0] * s1 + pos2[i][0] * s;
		pos1[i][1] = pos1[i][1] * s1 + pos2[i][1] * s;
		pos1[i][2] = pos1[i][2] * s1 + pos2[i][2] * s;
	}
}

/*
====================
StudioGetAnim

====================
*/
mstudioanim_t* CStudioModelRenderer::StudioGetAnim(model_t* m_pSubModel, mstudioseqdesc_t* pseqdesc)
{
	mstudioseqgroup_t* pseqgroup;
	cache_user_t* paSequences;

	pseqgroup = (mstudioseqgroup_t*)((byte*)m_pStudioHeader + m_pStudioHeader->seqgroupindex) + pseqdesc->seqgroup;

	if (pseqdesc->seqgroup == 0)
	{
		return (mstudioanim_t*)((byte*)m_pStudioHeader + pseqgroup->data + pseqdesc->animindex);
	}

	paSequences = (cache_user_t*)m_pSubModel->submodels;

	if (paSequences == NULL)
	{
		paSequences = (cache_user_t*)IEngineStudio.Mem_Calloc(16, sizeof(cache_user_t)); // UNDONE: leak!
		m_pSubModel->submodels = (dmodel_t*)paSequences;
	}

	if (!IEngineStudio.Cache_Check((struct cache_user_s*)&(paSequences[pseqdesc->seqgroup])))
	{
		gEngfuncs.Con_DPrintf("loading %s\n", pseqgroup->name);
		IEngineStudio.LoadCacheFile(pseqgroup->name, (struct cache_user_s*)&paSequences[pseqdesc->seqgroup]);
	}
	return (mstudioanim_t*)((byte*)paSequences[pseqdesc->seqgroup].data + pseqdesc->animindex);
}

/*
====================
StudioPlayerBlend

====================
*/
void CStudioModelRenderer::StudioPlayerBlend(mstudioseqdesc_t* pseqdesc, int* pBlend, float* pPitch)
{
	// calc up/down pointing
	*pBlend = (*pPitch * 3);
	if (*pBlend < pseqdesc->blendstart[0])
	{
		*pPitch -= pseqdesc->blendstart[0] / 3.0;
		*pBlend = 0;
	}
	else if (*pBlend > pseqdesc->blendend[0])
	{
		*pPitch -= pseqdesc->blendend[0] / 3.0;
		*pBlend = 255;
	}
	else
	{
		if (pseqdesc->blendend[0] - pseqdesc->blendstart[0] < 0.1) // catch qc error
			*pBlend = 127;
		else
			*pBlend = 255 * (*pBlend - pseqdesc->blendstart[0]) / (pseqdesc->blendend[0] - pseqdesc->blendstart[0]);
		*pPitch = 0;
	}
}

/*
====================
StudioSetUpTransform

Copies model origin and rotation into the transform matrix for rendering
====================
*/
void CStudioModelRenderer::StudioSetUpTransform(int trivial_accept)
{
	int i;
	vec3_t angles;
	vec3_t modelpos;

	cl_entity_t& Ent = *m_pCurrentEntity;

	if (m_pCurrentEntity->model->flags & EF_ROTATE)
	{
		float timemod;
		timemod = fmod(gEngfuncs.GetClientTime(), 2.0f);
		m_pCurrentEntity->angles[0] = 0;
		m_pCurrentEntity->angles[YAW] = timemod * 180.0 - 90.0;
		m_pCurrentEntity->angles[2] = 0;
		VectorCopy(m_pCurrentEntity->angles, m_pCurrentEntity->curstate.angles);
	}

	// tweek model origin
	//angles = m_pCurrentEntity->curstate.angles;
	angles = Ent.angles;
	modelpos = Ent.origin; //default to current position.  If this is a monster/item/player/etc,
						   //then this is changed to an interpolated position below

	//For Custom follow - Always use the owner's origin
	if (FBitSet(Ent.curstate.playerclass, ENT_EFFECT_FOLLOW_ROTATE) && Ent.curstate.owner > 0)
	{
		//Set the origin to my owner's origin
		cl_entity_t* entOwner = gEngfuncs.GetEntityByIndex(Ent.curstate.owner);
		if (entOwner)
			modelpos = entOwner->curstate.origin;
	}
	else
	{
		float frametime = (m_clTime - m_clOldTime);
		Ent.baseline.vuser1 = g_vecZero;

		if (!FBitSet(Ent.curstate.effects, EF_NOINTERP) && Ent.curstate.movetype != MOVETYPE_NONE) //Could be a non-moving server ent, or any client ent besides the player
		{
			if (Ent.curstate.movetype != MOVETYPE_WALK) //Player have their own interpolation
			{
				//Ent.origin = Ent.curstate.origin;
				float f = 0;
				float d;

				// don't do it if the goalstarttime hasn't updated in a while.

				// NOTE:  Because we need to interpolate multiplayer characters, the interpolation time limit
				//  was increased to 1.0 s., which is 2x the max lag we are accounting for.

				if ((m_clTime < Ent.curstate.animtime + 1.0f) &&
					(Ent.curstate.animtime != Ent.latched.prevanimtime))
				{
					f = (m_clTime - Ent.curstate.animtime) / (Ent.curstate.animtime - Ent.latched.prevanimtime);
					//Con_DPrintf("%4.2f %.2f %.2f\n", f, Ent.curstate.animtime, m_clTime);
				}

				if (m_fDoInterp)
					// ugly hack to interpolate angle, position. current is reached 0.1 seconds after being set
					f = f - 1.0;
				else
					f = 0;

				const auto pseqdesc = (mstudioseqdesc_t*)((byte*)m_pStudioHeader + m_pStudioHeader->seqindex) + Ent.curstate.sequence;
				if ((pseqdesc->motiontype & STUDIO_LX) != 0 || (Ent.curstate.eflags & EFLAG_SLERP) != 0)
				{
					for (i = 0; i < 3; i++)
					{
						modelpos[i] += (Ent.origin[i] - Ent.latched.prevorigin[i]) * f;
					}
				}

				//			Con_DPrintf("%.0f %.0f\n",Ent.msg_angles[0][YAW], Ent.msg_angles[1][YAW] );
				for (i = 0; i < 3; i++)
				{
					float ang1, ang2;

					ang1 = Ent.angles[i];
					ang2 = Ent.latched.prevangles[i];

					d = ang1 - ang2;
					if (d > 180)
					{
						d -= 360;
					}
					else if (d < -180)
					{
						d += 360;
					}

					angles[i] += d * f;
				}
			}

			if (Ent.HasExtra())
			{
				modelinfo_t& ModelInfo = CModelMgr::m_ModelInfo[Ent.index];
				ModelInfo.InterpOrg = modelpos;
				ModelInfo.InterpAng = angles;
			}

			//Calculate velocity, for $getcl, later
			if (frametime)
				Ent.baseline.vuser1 = (Ent.origin - Ent.latched.prevorigin) / frametime;
		}
		else
		{
			//EF_NOINTERP was set, so make the velocity 0
			Ent.baseline.vuser1 = g_vecZero;
		}
	}
	/*else if ( Ent.curstate.movetype != MOVETYPE_NONE )
	{
		VectorCopy( Ent.angles, angles );
	}*/

	angles[PITCH] = -angles[PITCH];
	AngleMatrix(angles, (*m_protationmatrix));

	if (!IEngineStudio.IsHardware())
	{
		static float viewmatrix[3][4];

		VectorCopy(m_vRight, viewmatrix[0]);
		VectorCopy(m_vUp, viewmatrix[1]);
		VectorInverse(viewmatrix[1]);
		VectorCopy(m_vNormal, viewmatrix[2]);

		(*m_protationmatrix)[0][3] = modelpos[0] - m_vRenderOrigin[0];
		(*m_protationmatrix)[1][3] = modelpos[1] - m_vRenderOrigin[1];
		(*m_protationmatrix)[2][3] = modelpos[2] - m_vRenderOrigin[2];

		ConcatTransforms(viewmatrix, (*m_protationmatrix), (*m_paliastransform));

		// do the scaling up of x and y to screen coordinates as part of the transform
		// for the unclipped case (it would mess up clipping in the clipped case).
		// Also scale down z, so 1/z is scaled 31 bits for free, and scale down x and y
		// correspondingly so the projected x and y come out right
		// FIXME: make this work for clipped case too?
		if (trivial_accept)
		{
			for (i = 0; i < 4; i++)
			{
				(*m_paliastransform)[0][i] *= m_fSoftwareXScale *
					(1.0 / (ZISCALE * 0x10000));
				(*m_paliastransform)[1][i] *= m_fSoftwareYScale *
					(1.0 / (ZISCALE * 0x10000));
				(*m_paliastransform)[2][i] *= 1.0 / (ZISCALE * 0x10000);
			}
		}
	}

	(*m_protationmatrix)[0][3] = modelpos[0];
	(*m_protationmatrix)[1][3] = modelpos[1];
	(*m_protationmatrix)[2][3] = modelpos[2];

	//Master Sword - scale the model if specified
	if (Ent.curstate.scale)
	{
		float s = Ent.curstate.scale;
		float scalemat[3][4] =
		{
			s, 0, 0, 0,
			0, s, 0, 0,
			0, 0, s, 0,
		};

		ConcatTransforms((*m_protationmatrix), scalemat, (*m_protationmatrix));
	}
}

/*
====================
StudioEstimateInterpolant

====================
*/
float CStudioModelRenderer::StudioEstimateInterpolant(void)
{
	float dadt = 1.0;

	if (m_fDoInterp && (m_pCurrentEntity->curstate.animtime >= m_pCurrentEntity->latched.prevanimtime + 0.01))
	{
		dadt = (m_clTime - m_pCurrentEntity->curstate.animtime) / 0.1;
		if (dadt > 2.0)
		{
			dadt = 2.0;
		}
	}
	return dadt;
}

/*
====================
StudioCalcRotations

====================
*/
void CStudioModelRenderer::StudioCalcRotations(float pos[][3], vec4_t* q, mstudioseqdesc_t* pseqdesc, mstudioanim_t* panim, float flFrame)
{
	int i;
	int frame;
	mstudiobone_t* pbone;

	float s;
	float adj[MAXSTUDIOCONTROLLERS];
	float dadt;

	if (flFrame > pseqdesc->numframes - 1)
	{
		flFrame = 0; // bah, fix this bug with changing sequences too fast
	}
	// BUG ( somewhere else ) but this code should validate this data.
	// This could cause a crash if the frame # is negative, so we'll go ahead
	//  and clamp it here
	else if (flFrame < -0.01)
	{
		flFrame = -0.01;
	}

	frame = (int)flFrame;

	// Con_DPrintf("%d %.4f %.4f %.4f %.4f %d\n", m_pCurrentEntity->curstate.sequence, m_clTime, m_pCurrentEntity->animtime, m_pCurrentEntity->frame, f, frame );

	// Con_DPrintf( "%f %f %f\n", m_pCurrentEntity->angles[ROLL], m_pCurrentEntity->angles[PITCH], m_pCurrentEntity->angles[YAW] );

	// Con_DPrintf("frame %d %d\n", frame1, frame2 );

	dadt = StudioEstimateInterpolant();
	s = (flFrame - frame);

	// add in programtic controllers
	pbone = (mstudiobone_t*)((byte*)m_pStudioHeader + m_pStudioHeader->boneindex);

	StudioCalcBoneAdj(dadt, adj, m_pCurrentEntity->curstate.controller, m_pCurrentEntity->latched.prevcontroller, m_pCurrentEntity->mouth.mouthopen);

	for (i = 0; i < m_pStudioHeader->numbones; i++, pbone++, panim++)
	{
		g_CurrentBone = i;
		StudioCalcBoneQuaterion(frame, s, pbone, panim, adj, q[i]);

		StudioCalcBonePosition(frame, s, pbone, panim, adj, pos[i]);
		// if (0 && i == 0)
		//	Con_DPrintf("%d %d %d %d\n", m_pCurrentEntity->curstate.sequence, frame, j, k );
	}

	if (pseqdesc->motiontype & STUDIO_X)
	{
		pos[pseqdesc->motionbone][0] = 0.0;
	}
	if (pseqdesc->motiontype & STUDIO_Y)
	{
		pos[pseqdesc->motionbone][1] = 0.0;
	}
	if (pseqdesc->motiontype & STUDIO_Z)
	{
		pos[pseqdesc->motionbone][2] = 0.0;
	}

	s = 0 * ((1.0 - (flFrame - (int)(flFrame))) / (pseqdesc->numframes)) * m_pCurrentEntity->curstate.framerate;

	if (pseqdesc->motiontype & STUDIO_LX)
	{
		pos[pseqdesc->motionbone][0] += s * pseqdesc->linearmovement[0];
	}
	if (pseqdesc->motiontype & STUDIO_LY)
	{
		pos[pseqdesc->motionbone][1] += s * pseqdesc->linearmovement[1];
	}
	if (pseqdesc->motiontype & STUDIO_LZ)
	{
		pos[pseqdesc->motionbone][2] += s * pseqdesc->linearmovement[2];
	}
}

/*
====================
Studio_FxTransform

====================
*/
void CStudioModelRenderer::StudioFxTransform(cl_entity_t* ent, float transform[3][4])
{
	switch (ent->curstate.renderfx)
	{
	case kRenderFxDistort:
	case kRenderFxHologram:
		if (gEngfuncs.pfnRandomLong(0, 49) == 0)
		{
			int axis = gEngfuncs.pfnRandomLong(0, 1);
			if (axis == 1) // Choose between x & z
				axis = 2;
			VectorScale(transform[axis], gEngfuncs.pfnRandomFloat(1, 1.484), transform[axis]);
		}
		else if (gEngfuncs.pfnRandomLong(0, 49) == 0)
		{
			float offset;
			int axis = gEngfuncs.pfnRandomLong(0, 1);
			if (axis == 1) // Choose between x & z
				axis = 2;
			offset = gEngfuncs.pfnRandomFloat(-10, 10);
			transform[gEngfuncs.pfnRandomLong(0, 2)][3] += offset;
		}
		break;
	case kRenderFxExplode:
	{
		float scale;

		scale = 1.0 + (m_clTime - ent->curstate.animtime) * 10.0;
		if (scale > 2) // Don't blow up more than 200%
			scale = 2;
		transform[0][1] *= scale;
		transform[1][1] *= scale;
		transform[2][1] *= scale;
	}
	break;
	}
	//transform[0][1] *= -1;	//testhack
	//transform[1][1] *= -1;
	//transform[2][1] *= -1;
}

/*
====================
StudioEstimateFrame

====================
*/
float CStudioModelRenderer::StudioEstimateFrame(mstudioseqdesc_t* pseqdesc)
{
	double dfdt, f;

	if (m_fDoInterp)
	{
		if (m_clTime < m_pCurrentEntity->curstate.animtime)
		{
			dfdt = 0;
		}
		else
		{
			dfdt = (m_clTime - m_pCurrentEntity->curstate.animtime) * m_pCurrentEntity->curstate.framerate * pseqdesc->fps;
		}
	}
	else
	{
		dfdt = 0;
	}

	if (pseqdesc->numframes <= 1)
	{
		f = 0;
	}
	else
	{
		f = (m_pCurrentEntity->curstate.frame * (pseqdesc->numframes - 1)) / 256.0;
	}

	f += dfdt;

	if (pseqdesc->flags & STUDIO_LOOPING)
	{
		if (pseqdesc->numframes > 1)
		{
			f -= (int)(f / (pseqdesc->numframes - 1)) * (pseqdesc->numframes - 1);
		}
		if (f < 0)
		{
			f += (pseqdesc->numframes - 1);
		}
	}
	else
	{
		if (f >= pseqdesc->numframes - 1.001)
		{
			f = pseqdesc->numframes - 1.001;
		}
		if (f < 0.0)
		{
			f = 0.0;
		}
	}
	return f;
}

/*
====================
StudioSetupBones

====================
*/
void CStudioModelRenderer::StudioSetupBones(void)
{
	int i;
	double f;

	mstudiobone_t* pbones;
	mstudioseqdesc_t* pseqdesc;
	mstudioanim_t* panim;

	static float pos[MAXSTUDIOBONES][3];
	static vec4_t q[MAXSTUDIOBONES];
	float bonematrix[3][4];

	static float pos2[MAXSTUDIOBONES][3];
	static vec4_t q2[MAXSTUDIOBONES];
	static float pos3[MAXSTUDIOBONES][3];
	static vec4_t q3[MAXSTUDIOBONES];
	static float pos4[MAXSTUDIOBONES][3];
	static vec4_t q4[MAXSTUDIOBONES];
	cl_entity_t& Ent = *m_pCurrentEntity;

	if (Ent.curstate.sequence >= m_pStudioHeader->numseq)
	{
		Ent.curstate.sequence = 0;
	}

	pseqdesc = (mstudioseqdesc_t*)((byte*)m_pStudioHeader + m_pStudioHeader->seqindex) + Ent.curstate.sequence;

	f = StudioEstimateFrame(pseqdesc);

	if (Ent.latched.prevframe > f)
	{
		//Con_DPrintf("%f %f\n", Ent.prevframe, f );
	}

	panim = StudioGetAnim(m_pRenderModel, pseqdesc);
	StudioCalcRotations(pos, q, pseqdesc, panim, f);

	if (pseqdesc->numblends > 1)
	{
		float s;
		float dadt;

		panim += m_pStudioHeader->numbones;
		StudioCalcRotations(pos2, q2, pseqdesc, panim, f);

		dadt = StudioEstimateInterpolant();
		s = (Ent.curstate.blending[0] * dadt + Ent.latched.prevblending[0] * (1.0 - dadt)) / 255.0;

		StudioSlerpBones(q, pos, q2, pos2, s);

		if (pseqdesc->numblends == 4)
		{
			panim += m_pStudioHeader->numbones;
			StudioCalcRotations(pos3, q3, pseqdesc, panim, f);

			panim += m_pStudioHeader->numbones;
			StudioCalcRotations(pos4, q4, pseqdesc, panim, f);

			s = (Ent.curstate.blending[0] * dadt + Ent.latched.prevblending[0] * (1.0 - dadt)) / 255.0;
			StudioSlerpBones(q3, pos3, q4, pos4, s);

			s = (Ent.curstate.blending[1] * dadt + Ent.latched.prevblending[1] * (1.0 - dadt)) / 255.0;
			StudioSlerpBones(q, pos, q3, pos3, s);
		}
	}

	if (m_fDoInterp &&
		Ent.latched.sequencetime &&
		(Ent.latched.sequencetime + 0.2 > m_clTime) &&
		(Ent.latched.prevsequence < m_pStudioHeader->numseq))
	{
		// blend from last sequence
		static float pos1b[MAXSTUDIOBONES][3];
		static vec4_t q1b[MAXSTUDIOBONES];
		float s;

		if (Ent.latched.prevsequence >= m_pStudioHeader->numseq)
			Ent.latched.prevsequence = 0;

		pseqdesc = (mstudioseqdesc_t*)((byte*)m_pStudioHeader + m_pStudioHeader->seqindex) + Ent.latched.prevsequence;
		panim = StudioGetAnim(m_pRenderModel, pseqdesc);
		// clip prevframe
		StudioCalcRotations(pos1b, q1b, pseqdesc, panim, Ent.latched.prevframe);

		if (pseqdesc->numblends > 1)
		{
			panim += m_pStudioHeader->numbones;
			StudioCalcRotations(pos2, q2, pseqdesc, panim, Ent.latched.prevframe);

			s = (Ent.latched.prevseqblending[0]) / 255.0;
			StudioSlerpBones(q1b, pos1b, q2, pos2, s);

			if (pseqdesc->numblends == 4)
			{
				panim += m_pStudioHeader->numbones;
				StudioCalcRotations(pos3, q3, pseqdesc, panim, Ent.latched.prevframe);

				panim += m_pStudioHeader->numbones;
				StudioCalcRotations(pos4, q4, pseqdesc, panim, Ent.latched.prevframe);

				s = (Ent.latched.prevseqblending[0]) / 255.0;
				StudioSlerpBones(q3, pos3, q4, pos4, s);

				s = (m_pCurrentEntity->latched.prevseqblending[1]) / 255.0;
				StudioSlerpBones(q1b, pos1b, q3, pos3, s);
			}
		}

		s = 1.0 - (m_clTime - m_pCurrentEntity->latched.sequencetime) / 0.2;
		StudioSlerpBones(q, pos, q1b, pos1b, s);
	}
	else
	{
		//Con_DPrintf("prevframe = %4.2f\n", f);
		m_pCurrentEntity->latched.prevframe = f;
	}

	pbones = (mstudiobone_t*)((byte*)m_pStudioHeader + m_pStudioHeader->boneindex);

	// bounds checking
	if (m_pPlayerInfo && (m_pPlayerInfo->gaitsequence >= m_pStudioHeader->numseq))
		m_pPlayerInfo->gaitsequence = 0;

	// calc gait animation
	if (m_pPlayerInfo && m_pPlayerInfo->gaitsequence != 0)
	{
		pseqdesc = (mstudioseqdesc_t*)((byte*)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pPlayerInfo->gaitsequence;

		panim = StudioGetAnim(m_pRenderModel, pseqdesc);
		StudioCalcRotations(pos2, q2, pseqdesc, panim, m_pPlayerInfo->gaitframe);

		for (i = 0; i < m_pStudioHeader->numbones; i++)
		{
			if (strcmp(pbones[i].name, "Bip01 Spine") == 0)
				break;
			memcpy(pos[i], pos2[i], sizeof(pos[i]));
			memcpy(q[i], q2[i], sizeof(q[i]));
		}
	}

	for (i = 0; i < m_pStudioHeader->numbones; i++)
	{
		QuaternionMatrix(q[i], bonematrix);

		bonematrix[0][3] = pos[i][0];
		bonematrix[1][3] = pos[i][1];
		bonematrix[2][3] = pos[i][2];

		if (pbones[i].parent == -1)
		{
			if (IEngineStudio.IsHardware())
			{
				ConcatTransforms((*m_protationmatrix), bonematrix, (*m_pbonetransform)[i]);
				MatrixCopy((*m_pbonetransform)[i], (*m_plighttransform)[i]);
				//ConcatTransforms ((*m_protationmatrix), bonematrix, (*m_plighttransform)[i]);
			}
			else
			{
				ConcatTransforms((*m_paliastransform), bonematrix, (*m_pbonetransform)[i]);
				ConcatTransforms((*m_protationmatrix), bonematrix, (*m_plighttransform)[i]);
			}

			// Apply client-side effects to the transformation matrix
			StudioFxTransform(m_pCurrentEntity, (*m_pbonetransform)[i]);
		}
		else
		{
			ConcatTransforms((*m_pbonetransform)[pbones[i].parent], bonematrix, (*m_pbonetransform)[i]);
			MatrixCopy((*m_pbonetransform)[i], (*m_plighttransform)[i]);
			//ConcatTransforms ((*m_plighttransform)[pbones[i].parent], bonematrix, (*m_plighttransform)[i]);
		}

		if (Ent.HasExtra())
		{
			modelinfo_t& ModelInfo = CModelMgr::m_ModelInfo[Ent.index];
			ModelInfo.BoneOrigins[i] = Vector(pbones[i].value[0] + pbones[i].scale[0],
				pbones[i].value[1] + pbones[i].scale[1],
				pbones[i].value[2] + pbones[i].scale[2]);
			MatrixCopy((*m_pbonetransform)[i], ModelInfo.BoneTransformations[i]);
		}
	}
}

/*
====================
StudioSaveBones

====================
*/
void CStudioModelRenderer::StudioSaveBones(void)
{
	int i;

	mstudiobone_t* pbones;
	pbones = (mstudiobone_t*)((byte*)m_pStudioHeader + m_pStudioHeader->boneindex);

	m_nCachedBones = m_pStudioHeader->numbones;

	for (i = 0; i < m_pStudioHeader->numbones; i++)
	{
		strncpy(m_nCachedBoneNames[i], pbones[i].name, 32);
		MatrixCopy((*m_pbonetransform)[i], m_rgCachedBoneTransform[i]);
		MatrixCopy((*m_plighttransform)[i], m_rgCachedLightTransform[i]);
	}
}

/*
====================
StudioMergeBones

====================
*/
#include "pm_defs.h"
extern physent_t* MSUTIL_EntityByIndex(int playerindex);
void CStudioModelRenderer::StudioMergeBones(model_t* m_pSubModel)
{
	int i, j;
	double f;
	//int					do_hunt = true;
	cl_entity_t& Ent = *m_pCurrentEntity;

	mstudiobone_t* pbones;
	mstudioseqdesc_t* pseqdesc;
	mstudioanim_t* panim;

	static float pos[MAXSTUDIOBONES][3];
	float bonematrix[3][4];
	static vec4_t q[MAXSTUDIOBONES];

	if (Ent.curstate.sequence >= m_pStudioHeader->numseq)
	{
		Ent.curstate.sequence = 0;
	}

	pseqdesc = (mstudioseqdesc_t*)((byte*)m_pStudioHeader + m_pStudioHeader->seqindex) + Ent.curstate.sequence;

	f = StudioEstimateFrame(pseqdesc);

	if (Ent.latched.prevframe > f)
	{
		//Con_DPrintf("%f %f\n", Ent.prevframe, f );
	}

	panim = StudioGetAnim(m_pSubModel, pseqdesc);
	StudioCalcRotations(pos, q, pseqdesc, panim, f);

	pbones = (mstudiobone_t*)((byte*)m_pStudioHeader + m_pStudioHeader->boneindex);

	for (i = 0; i < m_pStudioHeader->numbones; i++)
	{
		if (FBitSet(Ent.curstate.playerclass, ENT_EFFECT_FOLLOW_ROTATE))
			j = m_nCachedBones;
		else
			for (j = 0; j < m_nCachedBones; j++)
			{
				//if( strstr(pbones[i].name,"rhand") )

				if (_stricmp(pbones[i].name, m_nCachedBoneNames[j]) == 0)
				{
					MatrixCopy(m_rgCachedBoneTransform[j], (*m_pbonetransform)[i]);
					MatrixCopy(m_rgCachedLightTransform[j], (*m_plighttransform)[i]);
					break;
				}
			}
		if (j >= m_nCachedBones)
		{
			QuaternionMatrix(q[i], bonematrix);

			bonematrix[0][3] = pos[i][0];
			bonematrix[1][3] = pos[i][1];
			bonematrix[2][3] = pos[i][2];

			if (pbones[i].parent == -1)
			{
				if (FBitSet(Ent.curstate.playerclass, ENT_EFFECT_FOLLOW_ROTATE))
				{
					//Master Sword - This is a special MOVETYPE_FOLLOW entity that rotates with the host
					//(Used by arrows, spider)
					cl_entity_t* entOwner = NULL;
					bool LocalFollow = (Ent.curstate.owner == gEngfuncs.GetLocalPlayer()->index);

					if (!LocalFollow)
						entOwner = gEngfuncs.GetEntityByIndex(Ent.curstate.owner);
					else
						entOwner = gEngfuncs.GetLocalPlayer(); //When the server removes MOVETYPE_FOLLOW in AddToFullPack,
															   //it's attached to _me_ and needs to show up in my firstperson view

					Vector OwnerOrg = entOwner->origin, OwnerAng = entOwner->angles;
					if (Ent.HasExtra() && !LocalFollow)
					{
						modelinfo_t& ModelInfo = CModelMgr::m_ModelInfo[entOwner->index];
						OwnerOrg = ModelInfo.InterpOrg;
						OwnerAng = ModelInfo.InterpAng;
					}

					float HostMatrix[3][4];					   //Get the host's angles into a matrix
					Vector HostAng(0, OwnerAng.y, OwnerAng.z); //Ignore host's pitch
					AngleMatrix(HostAng, HostMatrix);
					for (int c = 0; c < 3; c++)
						HostMatrix[c][3] = OwnerOrg[c];

					if (FBitSet(Ent.curstate.playerclass, ENT_EFFECT_FOLLOW_ALIGN_BOTTOM))
						HostMatrix[2][3] += entOwner->curstate.mins.z;

					float AttachmentMatrix[3][4]; //Get my angles into a matrix

					AngleMatrix(Ent.curstate.angles, AttachmentMatrix);
					for (int c = 0; c < 3; c++)
						AttachmentMatrix[c][3] = Ent.origin[c];

					ConcatTransforms(HostMatrix, AttachmentMatrix, (*m_protationmatrix));
				}

				if (IEngineStudio.IsHardware())
				{
					ConcatTransforms((*m_protationmatrix), bonematrix, (*m_pbonetransform)[i]);
					MatrixCopy((*m_pbonetransform)[i], (*m_plighttransform)[i]);

					//ConcatTransforms ((*m_protationmatrix), bonematrix, (*m_plighttransform)[i]);
				}
				else
				{
					ConcatTransforms((*m_paliastransform), bonematrix, (*m_pbonetransform)[i]);
					ConcatTransforms((*m_protationmatrix), bonematrix, (*m_plighttransform)[i]);
				}

				// Apply client-side effects to the transformation matrix
				StudioFxTransform(m_pCurrentEntity, (*m_pbonetransform)[i]);
			}
			else
			{
				ConcatTransforms((*m_pbonetransform)[pbones[i].parent], bonematrix, (*m_pbonetransform)[i]);
				MatrixCopy((*m_pbonetransform)[i], (*m_plighttransform)[i]);
				//ConcatTransforms ((*m_plighttransform)[pbones[i].parent], bonematrix, (*m_plighttransform)[i]);
			}
		}
	}
}

/*
====================
StudioDrawModel

====================
*/

#define rdrdbg(a) dbg(msstring(a) + " Entity #" + m_pCurrentEntity->curstate.number)
//#define rdrdbg( a )

extern bool g_FirstRender;
extern CGameStudioModelRenderer g_StudioRenderer;

void RenderModel(cl_entity_t* pEntity)
{
	DrawEnt = pEntity;
	if (DrawEnt->player)
		CModelMgr::MSStudioDrawModel(STUDIO_RENDER, (IEngineStudio.GetPlayerState(pEntity->index - 1)));
	else
		CModelMgr::MSStudioDrawModel(STUDIO_RENDER, NULL);
}

//MIB APR2008a - massive changes
int CStudioModelRenderer::StudioDrawModel(int flags)
{
	startdbg;
	dbg("Begin");

	//if( !FBitSet(flags, STUDIO_RENDER) ) return 1;

	if (g_FirstRender && FBitSet(flags, STUDIO_RENDER))
	{
		//glClear( GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT|GL_ACCUM_BUFFER_BIT );
		g_FirstRender = false;
	}

	if (DrawEnt)
	{
		m_pCurrentEntity = DrawEnt;
		DrawEnt = NULL;
	}
	else
		m_pCurrentEntity = IEngineStudio.GetCurrentEntity();

	cl_entity_t& Ent = *m_pCurrentEntity;

	if (FBitSet(flags, STUDIO_RENDER))
		if (!CMirrorMgr::Render_StudioModel(m_pCurrentEntity))
			return 0; //Rendering inside of a mirror was canceled

	//Default is to only render the one 'current' entity.  The view model overrides this and renders two
	cl_entity_t* RenderEnts[MAX_PLAYER_HANDITEMS] = { m_pCurrentEntity, NULL, NULL };
	int TotalModels = 1;

	//Master Sword - Rendering the view model actually means rendering two separate models
	if (FBitSet(m_pCurrentEntity->curstate.oldbuttons, MSRDR_VIEWMODEL))
	{
		TotalModels = 0;
		bool FoundExclusiveVModel = false;
		for (int hand = 0; hand < MAX_PLAYER_HANDITEMS; hand++) //Includes playerhands view model
		{
			//Check if some view model animation requested exclusive rendering (usually because it uses both hands)
			if (ViewModel_ExclusiveViewHand >= 0 && ViewModel_ExclusiveViewHand != hand)
				continue;

			if (CMirrorMgr::m_CurrentMirror.Enabled)
				return 0;

			//Check if an item is hend in this hand
			CGenericItem* pItem = player.Hand(hand);
			if (!pItem)
				continue;
			if (hand == HAND_PLAYERHANDS && player.ActiveItem() != pItem)
				continue; //For the player hands to be rendered, it must be the active item

			//Check if the viewmodel for this hand is valid
			int NUM = pItem->m_ViewModelPart;
			model_s* pModel = gEngfuncs.CL_LoadModel(pItem->m_ViewModel, &NUM); //MiB Apr2008a - ViewModel changing system
			if (!pModel)
				continue;

			int h = TotalModels;

			//Try to create an entity for this viewmodel (only lasts this frame)
			cl_entity_t& RenderEnt = MSCLGlobals::CLViewEntities[hand];
			int Ent = gEngfuncs.CL_CreateVisibleEntity(ET_NORMAL, &RenderEnt);
			if (!Ent)
				continue;

			if (ViewModel_ExclusiveViewHand >= 0)
				FoundExclusiveVModel = true;

			RenderEnt.model = pModel;
			studiohdr_t* pStudioHeader = (studiohdr_t*)IEngineStudio.Mod_Extradata(RenderEnt.model);

			cl_entity_s* view = gEngfuncs.GetViewModel();
			view->model = NULL; //MiB JUN2010_21 - Disable the HL viewmodel. We use our own.

			RenderEnt.SetBody(pItem->m_ViewModelPart, pItem->m_ViewModelSubmodel); //Set the specified submodel on the item

			//MiB Apr 2008a NOTE:
			//These ifs may need to be removed. Skins and other things CAN be 0.
			//Untested right now.
			if (pItem->m_Skin > 0)
				RenderEnt.curstate.skin = pItem->m_Skin;
			if (pItem->m_RenderMode > 0)
				RenderEnt.curstate.rendermode = pItem->m_RenderMode;
			if (pItem->m_RenderFx > 0)
				RenderEnt.curstate.renderfx = pItem->m_RenderFx;
			if (pItem->m_RenderAmt > 0)
				RenderEnt.baseline.renderamt = RenderEnt.curstate.renderamt = pItem->m_RenderAmt;
			//[/Shuriken]

			SetBits(RenderEnt.curstate.colormap, MSRDR_HANDMODEL);
			SetBits(RenderEnt.curstate.oldbuttons, MSRDR_NOREFLECT);
			RenderEnt.curstate.iuser2 = hand;
			RenderEnt.curstate.modelindex = 0;
			//RenderEnt.current_position = m_pCurrentEntity->current_position;
			RenderEnt.origin = m_pCurrentEntity->origin;
			RenderEnt.angles = m_pCurrentEntity->angles;
			RenderEnt.curstate.angles = m_pCurrentEntity->curstate.angles;
			if (hand == LEFT_HAND)
				SetBits(RenderEnt.curstate.oldbuttons, MSRDR_FLIPPED); //If this is the left hand, set the Flipped flag
			else
				ClearBits(RenderEnt.curstate.oldbuttons, MSRDR_FLIPPED);
			if (pItem->m_ViewModelAnim >= 0) //New anim
			{
				RenderEnt.curstate.sequence = pItem->m_ViewModelAnim;
				RenderEnt.curstate.frame = 0;
				RenderEnt.curstate.framerate = 1.0f;
				if (pItem->m_ViewModelAnimSpeed)
				{
					RenderEnt.curstate.framerate *= pItem->m_ViewModelAnimSpeed;
				}

				if (player.m_AnimSpeedAdj && player.m_AnimSpeedAdj != 1 && pItem->m_Scripts.size() && pItem->m_Scripts[0]->m.ScriptFile.len())
				{
					msstring vsItemName = pItem->m_Scripts[0]->m.ScriptFile;
					if (!vsItemName.starts_with("shield") && !vsItemName.starts_with("bow") && !vsItemName.starts_with("mana") && !vsItemName.starts_with("health") && !vsItemName.starts_with("item") && !atoi(pItem->GetFirstScriptVar("ITEM_IGNORE_SPEED_ADJUST")))
					{
						RenderEnt.curstate.framerate *= player.m_AnimSpeedAdj;
					}
				}

				RenderEnt.curstate.animtime = m_clTime;
				pItem->m_ViewModelAnim = -1; //Set to -1 so I keep using the same one next frame.  If the item sets this again, I'll restart my anim
			}

			/*static msstringlist Parameters;
			Parameters.clearitems( );
			Parameters.add( UTIL_VarArgs("%i",RenderEnt.curstate.number) );
			dbg( "call game_render on viewmodel" );
			pItem->CallScriptEvent( "game_render", &Parameters );*/

			//Check if anim finished
			//			studiohdr_t *pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata( RenderEnt.model );
			mstudioseqdesc_t* pseqdesc = (mstudioseqdesc_t*)((byte*)pStudioHeader + pStudioHeader->seqindex) + RenderEnt.curstate.sequence;
			float Currentframe = (m_clTime - RenderEnt.curstate.animtime) * RenderEnt.curstate.framerate * pseqdesc->fps;
			float LastFrame = (m_clOldTime - RenderEnt.curstate.animtime) * RenderEnt.curstate.framerate * pseqdesc->fps;
			if (Currentframe >= pseqdesc->numframes && LastFrame < pseqdesc->numframes
				//&& FBitSet(flags, STUDIO_EVENTS) )
				&& FBitSet(flags, STUDIO_RENDER))
			{
				dbg("call game_viewanimdone on viewmodel");
				pItem->CallScriptEvent("game_viewanimdone");
				if (ViewModel_ExclusiveViewHand == hand)
					ViewModel_ExclusiveViewHand = -1;
			}

			//logfile << "Current anim: " << RenderEnt.curstate.sequence << endl;
			RenderEnts[h] = &RenderEnt;
			TotalModels++;
		}

		//ClearBits( flags, STUDIO_RENDER );
		//Some view model requested exclusive rendering, but then got removed before the animation ended... allow rendering for both models again
		if (ViewModel_ExclusiveViewHand >= 0 && !FoundExclusiveVModel)
			ViewModel_ExclusiveViewHand = -1;
	}

	for (int rm = 0; rm < TotalModels; rm++)
	{
		m_pCurrentEntity = RenderEnts[rm];
		if (!m_pCurrentEntity)
			continue;

		cl_entity_t& Ent = *m_pCurrentEntity;

		rdrdbg("Start Render Model");
		IEngineStudio.GetTimes(&m_nFrameCount, &m_clTime, &m_clOldTime);
		IEngineStudio.GetViewInfo(m_vRenderOrigin, m_vUp, m_vRight, m_vNormal);
		IEngineStudio.GetAliasScale(&m_fSoftwareXScale, &m_fSoftwareYScale);

		if (m_pCurrentEntity->curstate.renderfx == kRenderFxDeadPlayer)
		{
			entity_state_t deadplayer;

			int result;
			int save_interp;

			if (m_pCurrentEntity->curstate.renderamt <= 0 || m_pCurrentEntity->curstate.renderamt > gEngfuncs.GetMaxClients())
				return 0;

			// get copy of player
			deadplayer = *(IEngineStudio.GetPlayerState(m_pCurrentEntity->curstate.renderamt - 1)); //cl.frames[cl.parsecount & CL_UPDATE_MASK].playerstate[m_pCurrentEntity->curstate.renderamt-1];

			// clear weapon, movement state
			deadplayer.number = m_pCurrentEntity->curstate.renderamt;
			deadplayer.weaponmodel = 0;
			deadplayer.gaitsequence = 0;

			deadplayer.movetype = MOVETYPE_NONE;
			VectorCopy(m_pCurrentEntity->curstate.angles, deadplayer.angles);
			VectorCopy(m_pCurrentEntity->curstate.origin, deadplayer.origin);

			save_interp = m_fDoInterp;
			m_fDoInterp = 0;

			// draw as though it were a player
			result = StudioDrawPlayer(flags, &deadplayer);

			m_fDoInterp = save_interp;
			return result;
		}

		//Draw model just like a player, except angles, origin, colormap (flags)
		if (FBitSet(Ent.curstate.colormap, MSRDR_ASPLAYER))
		{
			int oldidx = m_pCurrentEntity->index;
			int oldnum = m_pCurrentEntity->curstate.number;

			cl_entity_t* clplayer = gEngfuncs.GetLocalPlayer();
			int ZeroBasedPlayerIdx = clplayer->index - 1;
			entity_state_t fakeplayer = *(IEngineStudio.GetPlayerState(ZeroBasedPlayerIdx));

			m_pPlayerInfo = IEngineStudio.PlayerInfo(ZeroBasedPlayerIdx);
			Vector PrevOrigin = m_pPlayerInfo->prevgaitorigin;
			Vector vForward;
			EngineFunc::MakeVectors(m_pCurrentEntity->angles, vForward, NULL, NULL);
			//Set the "last origin" to behind me to the walk code thinks I'm walking forward.
			//This needs to be scaled from the original velocity, incase I'm drawing a fake player of a different size

			//m_pPlayerInfo->prevgaitorigin = m_pCurrentEntity->origin - vForward * (player.pev->velocity * m_pCurrentEntity->curstate.impacttime).Length();
			m_pPlayerInfo->prevgaitorigin = m_pCurrentEntity->origin;

			//m_pCurrentEntity->index = clplayer->index;
			//m_pCurrentEntity->prevstate = clplayer->prevstate;
			//m_pCurrentEntity->curstate.number = clplayer->curstate.number;
			if (FBitSet(Ent.curstate.colormap, MSRDR_COPYPLAYER))
			{
				m_pCurrentEntity->curstate.gaitsequence = clplayer->curstate.gaitsequence;
				m_pCurrentEntity->curstate.sequence = clplayer->curstate.sequence;
				m_pCurrentEntity->curstate.animtime = clplayer->curstate.animtime;
				m_pCurrentEntity->curstate.framerate = clplayer->curstate.framerate;
				m_pCurrentEntity->curstate.frame = clplayer->curstate.frame;
				memcpy(m_pCurrentEntity->curstate.controller, clplayer->curstate.controller, 4);
				m_pCurrentEntity->curstate.movetype = clplayer->curstate.movetype;
				m_pCurrentEntity->curstate.basevelocity = clplayer->curstate.basevelocity;
				m_pCurrentEntity->curstate.eflags = clplayer->curstate.eflags;
				m_pCurrentEntity->curstate.flFallVelocity = clplayer->curstate.flFallVelocity;
				m_pCurrentEntity->curstate.onground = clplayer->curstate.onground;
				m_pCurrentEntity->curstate.msg_time = clplayer->curstate.msg_time;
				m_pCurrentEntity->curstate.velocity = clplayer->curstate.velocity;
				m_pCurrentEntity->baseline = clplayer->baseline;
				memcpy(m_pCurrentEntity->attachment, clplayer->attachment, sizeof(m_pCurrentEntity->attachment));
			}

			//m_pCurrentEntity->latched = clplayer->latched;

			Ent.player = 0;

			fakeplayer.angles = Ent.curstate.angles;
			fakeplayer.origin = Ent.curstate.origin;
			fakeplayer.gaitsequence = Ent.curstate.gaitsequence;

			//Print( "Animtime %.2f\n", clplayer->curstate.animtime );
			// draw as though it were a player
			int result = StudioDrawPlayer(flags, &fakeplayer);

			m_pPlayerInfo = IEngineStudio.PlayerInfo(ZeroBasedPlayerIdx);
			m_pPlayerInfo->prevgaitorigin = PrevOrigin;
			m_pPlayerInfo = NULL;

			m_pCurrentEntity->curstate.number = oldnum;
			m_pCurrentEntity->index = oldidx;

			return result;
		}

		rdrdbg("Mod_Extradata Entity");
		m_pRenderModel = m_pCurrentEntity->model;
		m_pStudioHeader = (studiohdr_t*)IEngineStudio.Mod_Extradata(m_pRenderModel);

		RegisterExtraData(Ent);

		IEngineStudio.StudioSetHeader(m_pStudioHeader);
		IEngineStudio.SetRenderModel(m_pRenderModel);

		rdrdbg("StudioSetUpTransform");
		StudioSetUpTransform(0);

		bool SkipVisCheck = false;
		// Master Sword - If a special follow ent is following the local player, always render
		if (FBitSet(Ent.curstate.playerclass, ENT_EFFECT_FOLLOW_ROTATE) && Ent.curstate.owner == gEngfuncs.GetLocalPlayer()->index)
			SkipVisCheck = true;

		if (CMirrorMgr::m_CurrentMirror.Enabled)
			SkipVisCheck = true; //Skip vis on entites for mirror rendering

		if (flags & STUDIO_RENDER)
		{
			// see if the bounding box lets us trivially reject, also sets
			bool Visible = true, RestoreOrigin = false;
			Vector OldOrigin;
			if (FBitSet(Ent.curstate.playerclass, ENT_EFFECT_FOLLOW_ROTATE) && Ent.curstate.owner > 0)
			{ //Set the origin to my owner's origin for the vis check, then set it back
				cl_entity_t* ent = gEngfuncs.GetEntityByIndex(m_pCurrentEntity->curstate.owner);
				if (ent)
				{
					OldOrigin = m_pCurrentEntity->curstate.origin;
					m_pCurrentEntity->origin = m_pCurrentEntity->curstate.origin = ent->curstate.origin;
					RestoreOrigin = true;
				}
			}

			rdrdbg("StudioCheckBBox");
			if (!SkipVisCheck)
			{
				/*if( CMirrorMgr::m_CurrentMirror.Enabled )
				{
					Vector Bounds[2] = { m_pCurrentEntity->origin + m_pCurrentEntity->curstate.mins, m_pCurrentEntity->origin + m_pCurrentEntity->curstate.maxs };
					Visible = CheckBBox( Bounds );					//Special vis check on mirrored entities
				}
				else*/
				Visible = IEngineStudio.StudioCheckBBox() ? true : false;
			}
			if (RestoreOrigin)
				m_pCurrentEntity->origin = m_pCurrentEntity->curstate.origin = OldOrigin;
			if (!Visible)
				return 0;

			(*m_pModelsDrawn)++;
			(*m_pStudioModelCount)++; // render data cache cookie

			if (m_pStudioHeader->numbodyparts == 0)
				return 1;
		}

		if (Ent.curstate.movetype == MOVETYPE_FOLLOW ||
			FBitSet(Ent.curstate.playerclass, ENT_EFFECT_FOLLOW_ROTATE))
		{
			rdrdbg("StudioMergeBones");
			StudioMergeBones(m_pRenderModel);
		}
		else
		{
			rdrdbg("StudioSetupBones");
			StudioSetupBones();
		}

		if (flags & STUDIO_EVENTS)
		{
			rdrdbg("StudioCalcAttachments");
			StudioCalcAttachments();

			rdrdbg("IEngineStudio.StudioClientEvents");
			IEngineStudio.StudioClientEvents();
			// copy attachments into global entity array
			if (m_pCurrentEntity->index > 0)
			{
				cl_entity_t* ent = gEngfuncs.GetEntityByIndex(m_pCurrentEntity->index);

				if (ent)
					memcpy(ent->attachment, m_pCurrentEntity->attachment, sizeof(vec3_t) * 4);
			}
		}

		if (flags & STUDIO_RENDER)
		{
			rdrdbg("Render");
			//MiB JUN2010_21 - Makes the viewmodels not stick into walls
			if (FBitSet(m_pCurrentEntity->curstate.colormap, MSRDR_HANDMODEL))
			{
				float tmp[2];
				glGetFloatv(GL_DEPTH_RANGE, tmp);
				glDepthRange(tmp[0], tmp[0] + 0.3 * (tmp[1] - tmp[0]));

				StudioRenderModel();
				glDepthRange(tmp[0], tmp[1]);
			}
			else
				StudioRenderModel();
		}
	}
	enddbg;
	return 1;
}

int CGenericItem::GetViewModelID()
{
	if (m_Location != ITEMPOS_HANDS)
		return -1;
	return MSCLGlobals::CLViewEntities[m_Hand].curstate.number;
}

/*
====================
StudioEstimateGait

====================
*/
void CStudioModelRenderer::StudioEstimateGait(entity_state_t* pplayer)
{
	float frametime = (m_clTime - m_clOldTime);
	vec3_t est_velocity;

	if (frametime < 0)
		frametime = 0;
	else if (frametime > 1.0)
		frametime = 1;

	if (frametime == 0 || m_pPlayerInfo->renderframe == m_nFrameCount)
	{
		m_flGaitMovement = 0;
		return;
	}

	// VectorAdd( pplayer->velocity, pplayer->prediction_error, est_velocity );
	if (m_fGaitEstimation)
	{
		est_velocity = m_pCurrentEntity->origin - m_pPlayerInfo->prevgaitorigin;
		m_pPlayerInfo->prevgaitorigin = m_pCurrentEntity->origin;
		//VectorSubtract( m_pCurrentEntity->origin, m_pPlayerInfo->prevgaitorigin, est_velocity );
		//VectorCopy( m_pCurrentEntity->origin, m_pPlayerInfo->prevgaitorigin );
		//m_flGaitMovement = Length( est_velocity );
		m_flGaitMovement = est_velocity.Length2D(); //Master Sword - don't count falling velocity
		if (frametime <= 0 || m_flGaitMovement / frametime < 5)
		{
			m_flGaitMovement = 0;
			est_velocity[0] = 0;
			est_velocity[1] = 0;
		}
	}
	else
	{
		VectorCopy(pplayer->velocity, est_velocity);
		est_velocity.z = 0; //Master Sword - don't count falling velocity
		m_flGaitMovement = Length(est_velocity) * frametime;
	}

	if (est_velocity[1] == 0 && est_velocity[0] == 0)
	{
		float flYawDiff = m_pCurrentEntity->angles[YAW] - m_pPlayerInfo->gaityaw;
		flYawDiff = flYawDiff - (int)(flYawDiff / 360) * 360;
		if (flYawDiff > 180)
			flYawDiff -= 360;
		if (flYawDiff < -180)
			flYawDiff += 360;

		if (frametime < 0.25)
			flYawDiff *= frametime * 4;
		else
			flYawDiff *= frametime;

		m_pPlayerInfo->gaityaw += flYawDiff;
		m_pPlayerInfo->gaityaw = m_pPlayerInfo->gaityaw - (int)(m_pPlayerInfo->gaityaw / 360) * 360;

		m_flGaitMovement = 0;
	}
	else
	{
		m_pPlayerInfo->gaityaw = (atan2(est_velocity[1], est_velocity[0]) * 180 / M_PI);
		if (m_pPlayerInfo->gaityaw > 180)
			m_pPlayerInfo->gaityaw = 180;
		if (m_pPlayerInfo->gaityaw < -180)
			m_pPlayerInfo->gaityaw = -180;

		//Master Sword
		/*float flYawDiff = m_pCurrentEntity->angles[YAW] - m_pPlayerInfo->gaityaw;
		flYawDiff = flYawDiff - (int)(flYawDiff / 360) * 360;
		if (flYawDiff > 180)
			flYawDiff -= 360;
		if (flYawDiff < -180)
			flYawDiff += 360;*/

			//flYawDiff = max(min(flYawDiff,65),-65);
			//m_pPlayerInfo->gaityaw = m_pCurrentEntity->angles[YAW] + flYawDiff;
			//Print( "%f\n", flYawDiff );
	}
}

/*
====================
StudioProcessGait

====================
*/
void CStudioModelRenderer::StudioProcessGait(entity_state_t* pplayer)
{
	mstudioseqdesc_t* pseqdesc = NULL;
	//float frametime;
	int iBlend = 0;
	float flYaw = 0; // view direction relative to movement
	float frametime = 0;

	if (m_pCurrentEntity->curstate.sequence >= m_pStudioHeader->numseq)
		m_pCurrentEntity->curstate.sequence = 0;

	pseqdesc = (mstudioseqdesc_t*)((byte*)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pCurrentEntity->curstate.sequence;

	StudioPlayerBlend(pseqdesc, &iBlend, &m_pCurrentEntity->angles[PITCH]);

	m_pCurrentEntity->latched.prevangles[PITCH] = m_pCurrentEntity->angles[PITCH];
	m_pCurrentEntity->curstate.blending[0] = iBlend;
	m_pCurrentEntity->latched.prevblending[0] = m_pCurrentEntity->curstate.blending[0];
	m_pCurrentEntity->latched.prevseqblending[0] = m_pCurrentEntity->curstate.blending[0];

	//Print("%f %d\n", m_pCurrentEntity->angles[PITCH], m_pCurrentEntity->curstate.blending[0] );

	frametime = (m_clTime - m_clOldTime);
	//Master Sword
	//Slow down the gait animation
	if (frametime < 0)
		frametime = 0;
	else if (frametime > 1.0)
		frametime = 1;

	StudioEstimateGait(pplayer);

	// Con_DPrintf("%f %f\n", m_pCurrentEntity->angles[YAW], m_pPlayerInfo->gaityaw );

	// calc side to side turning
	if (!FBitSet(m_pCurrentEntity->curstate.colormap, MSRDR_ASPLAYER))
	{
		flYaw = m_pCurrentEntity->angles[YAW] - m_pPlayerInfo->gaityaw;
		flYaw = flYaw - (int)(flYaw / 360) * 360;

		//Print( "%.2f %.2f\n", m_pCurrentEntity->angles[YAW], m_pPlayerInfo->gaityaw );

		if (flYaw < -180)
			flYaw = flYaw + 360;
		if (flYaw > 180)
			flYaw = flYaw - 360;

		if (flYaw > 120)
		{
			m_pPlayerInfo->gaityaw = m_pPlayerInfo->gaityaw - 180;
			m_flGaitMovement = -m_flGaitMovement;
			flYaw = flYaw - 180;
		}
		else if (flYaw < -120)
		{
			m_pPlayerInfo->gaityaw = m_pPlayerInfo->gaityaw + 180;
			m_flGaitMovement = -m_flGaitMovement;
			flYaw = flYaw + 180;
		}

		// adjust torso
		m_pCurrentEntity->curstate.controller[0] = ((flYaw / 4.0) + 30) / (60.0 / 255.0);
		m_pCurrentEntity->curstate.controller[1] = ((flYaw / 4.0) + 30) / (60.0 / 255.0);
		m_pCurrentEntity->curstate.controller[2] = ((flYaw / 4.0) + 30) / (60.0 / 255.0);
		m_pCurrentEntity->curstate.controller[3] = ((flYaw / 4.0) + 30) / (60.0 / 255.0);
		m_pCurrentEntity->latched.prevcontroller[0] = m_pCurrentEntity->curstate.controller[0];
		m_pCurrentEntity->latched.prevcontroller[1] = m_pCurrentEntity->curstate.controller[1];
		m_pCurrentEntity->latched.prevcontroller[2] = m_pCurrentEntity->curstate.controller[2];
		m_pCurrentEntity->latched.prevcontroller[3] = m_pCurrentEntity->curstate.controller[3];

		m_pCurrentEntity->angles[YAW] = m_pPlayerInfo->gaityaw;
		if (m_pCurrentEntity->angles[YAW] < -0)
			m_pCurrentEntity->angles[YAW] += 360;
		m_pCurrentEntity->latched.prevangles[YAW] = m_pCurrentEntity->angles[YAW];
	}
	else
	{
		m_pCurrentEntity->curstate.controller[0] = 127;
		m_pCurrentEntity->curstate.controller[1] = 127;
		m_pCurrentEntity->curstate.controller[2] = 127;
		m_pCurrentEntity->curstate.controller[3] = 127;
		m_pCurrentEntity->latched.prevcontroller[0] = m_pCurrentEntity->curstate.controller[0];
		m_pCurrentEntity->latched.prevcontroller[1] = m_pCurrentEntity->curstate.controller[1];
		m_pCurrentEntity->latched.prevcontroller[2] = m_pCurrentEntity->curstate.controller[2];
		m_pCurrentEntity->latched.prevcontroller[3] = m_pCurrentEntity->curstate.controller[3];
	}

	if (pplayer->gaitsequence >= m_pStudioHeader->numseq)
		pplayer->gaitsequence = 0;

	pseqdesc = (mstudioseqdesc_t*)((byte*)m_pStudioHeader + m_pStudioHeader->seqindex) + pplayer->gaitsequence;

	//Master Sword - Scale gait framerate to fuser1
	//Print( "Gait: %.2f - %.2f - %.2f\n", m_flGaitMovement, frametime, pplayer->fuser1 );

	/*		m_flGaitMovement *= pplayer->fuser1;	// > 0 Means multiply gait framerate
	else if( pplayer->fuser1 < 0 )
		m_flGaitMovement = -pplayer->fuser1;	// < 0 Means use this gait framerate
*/
// calc gait frame
	if (pseqdesc->linearmovement[0] > 0)
	{
		m_pPlayerInfo->gaitframe += (m_flGaitMovement / pseqdesc->linearmovement[0]) * pseqdesc->numframes;
	}
	else if (frametime > 0) //Make sure I don't divide by zero if I process the same frame twice.  It has been a problem in the past - Dogg
	{
		//m_pPlayerInfo->gaitframe += pseqdesc->fps * frametime;
		float Speed = (m_flGaitMovement / frametime);
		float WalkPercent = pplayer->fuser1 ? Speed / pplayer->fuser1 : 1;
		float AdvAmt = WalkPercent * pseqdesc->fps * frametime;
		m_pPlayerInfo->gaitframe += AdvAmt;

		//if( IS_NAN( m_pPlayerInfo->gaitframe ) )	//Should never happen because of the above frametime check
		//	m_pPlayerInfo->gaitframe = 0;

		//if( m_flGaitMovement )
		//	Print( "%.2f | %.2f | %.2f\n", Speed, AdvAmt, m_pPlayerInfo->gaitframe );
	}

	// do modulo
	m_pPlayerInfo->gaitframe = m_pPlayerInfo->gaitframe - (int)(m_pPlayerInfo->gaitframe / pseqdesc->numframes) * pseqdesc->numframes;
	if (m_pPlayerInfo->gaitframe < 0)
		m_pPlayerInfo->gaitframe += pseqdesc->numframes;
}

/*
====================
StudioDrawPlayer

====================
*/
CRenderPlayer RenderPlayer;

int CStudioModelRenderer::StudioDrawPlayer(int flags, entity_state_t* pplayer)
{
	//	if( flags & STUDIO_RENDER )
	//		dbgtxt( "" );
	m_pCurrentEntity = IEngineStudio.GetCurrentEntity();
	cl_entity_t& Ent = *m_pCurrentEntity;

	/*if( m_pCurrentEntity->player &&
		m_pCurrentEntity->player == gEngfuncs.GetLocalPlayer()->index &&
		!MSCLGlobals::CamThirdPerson &&
		!CMirrorMgr::m_CurrentMirror.Enabled )
	{
		//Manually draw the Viewmodel.  The engine doesn't draw it because MS always reports thirdperson for mirrors
		m_pPlayerInfo = NULL;
		DrawEnt = IEngineStudio.GetViewEntity();
		StudioDrawModel( flags );

		m_pCurrentEntity = IEngineStudio.GetCurrentEntity();
		SetBits( m_pCurrentEntity->curstate.oldbuttons, MSRDR_SKIP );
		//return 0;
	}*/

	if (FBitSet(flags, STUDIO_RENDER))
		if (!CMirrorMgr::Render_StudioModel(m_pCurrentEntity))
			return 0; //Rendering inside of a mirror was cancelled

	IEngineStudio.GetTimes(&m_nFrameCount, &m_clTime, &m_clOldTime);
	IEngineStudio.GetViewInfo(m_vRenderOrigin, m_vUp, m_vRight, m_vNormal);
	IEngineStudio.GetAliasScale(&m_fSoftwareXScale, &m_fSoftwareYScale);

	//Don't rotate player models up/down !! -- MOVED:  To CRenderPlayer::Render()
	//Whatever is set for m_pCurrentEntity->curstate.angles.x is used later to rotate the back bones, making the player look up/down
	//Whatever is set for m_pCurrentEntity->angles.x is killed right here, to the whole model from rotating up/down
	//m_pCurrentEntity->angles.x = 0;
	/*if( m_pCurrentEntity->player && pplayer->number != gEngfuncs.GetLocalPlayer()->index )
	{
		m_pCurrentEntity->angles.x *= -1;
		m_pCurrentEntity->curstate.angles = m_pCurrentEntity->angles;
	}*/

	// Con_DPrintf("DrawPlayer %d\n", m_pCurrentEntity->blending[0] );
	// Con_DPrintf("DrawPlayer %d %d (%d)\n", m_nFrameCount, pplayer->player_index, m_pCurrentEntity->curstate.sequence );
	// Con_DPrintf("Player %.2f %.2f %.2f\n", pplayer->velocity[0], pplayer->velocity[1], pplayer->velocity[2] );

	m_nPlayerIndex = pplayer->number - 1;

	if (m_nPlayerIndex < 0 || m_nPlayerIndex >= gEngfuncs.GetMaxClients())
		return 0;

	m_pRenderModel = IEngineStudio.SetupPlayerModel(m_nPlayerIndex);
	if (m_pRenderModel == NULL)
		return 0;

	m_pStudioHeader = (studiohdr_t*)IEngineStudio.Mod_Extradata(m_pRenderModel);
	IEngineStudio.StudioSetHeader(m_pStudioHeader);
	IEngineStudio.SetRenderModel(m_pRenderModel);

	RegisterExtraData(Ent);

	//Master Sword - Flip the pitch for *other* players (not the local one).
	//This is just countering the engine flipping the pitch for other players in the first place
	//--UNDONE - Don't flip, just remove pitch from .angles and set curstate.angles properly (it is always set way too low for some reason)
	bool InterpolatePitch = false;
	float OldPitch = 0, IdealPitch = 0, InterpStartTime = 0, InterpMaxTime = 1;
	if (Ent.player)
	{
		if (Ent.FullRotate())
		{
			InterpolatePitch = true;
			OldPitch = m_pCurrentEntity->angles.x;
			IdealPitch = m_pCurrentEntity->curstate.fuser2 * 3;
			InterpStartTime = m_pCurrentEntity->latched.sequencetime;
			InterpMaxTime = 0.2;
			if (Ent.HasExtra())
			{
				modelinfo_t& ModelInfo = CModelMgr::m_ModelInfo[Ent.index];
				ModelInfo.InterpolateFromFullRotation = true;
			}
		}
		else
		{
			modelinfo_t* pModelInfo = NULL;
			if (Ent.HasExtra())
				pModelInfo = &CModelMgr::m_ModelInfo[Ent.index];

			if (pModelInfo && pModelInfo->InterpolateFromFullRotation) //Interp pitch when coming from the swim anim
			{
				InterpolatePitch = true;
				OldPitch = m_pCurrentEntity->curstate.fuser2 * 3;
				IdealPitch = m_pCurrentEntity->angles.x;
				InterpStartTime = m_pCurrentEntity->latched.sequencetime;
				InterpMaxTime = 0.2;

				if (m_clTime - InterpStartTime >= 0.2) //Done interpolating, turn it off
					pModelInfo->InterpolateFromFullRotation = false;
			}
			else
				m_pCurrentEntity->angles.x = m_pCurrentEntity->curstate.angles.x = 0;
		}

		if (InterpolatePitch)
		{
			float slerp = (m_clTime - InterpStartTime) / InterpMaxTime;
			if (InterpStartTime + InterpMaxTime > m_clTime)
				IdealPitch = OldPitch + (IdealPitch - OldPitch) * slerp;

			m_pCurrentEntity->angles.x = IdealPitch;
		}
	}

	if (pplayer->gaitsequence)
	{
		vec3_t orig_angles;
		m_pPlayerInfo = IEngineStudio.PlayerInfo(m_nPlayerIndex);

		VectorCopy(m_pCurrentEntity->angles, orig_angles);

		StudioProcessGait(pplayer);

		m_pPlayerInfo->gaitsequence = pplayer->gaitsequence;
		m_pPlayerInfo = NULL;

		StudioSetUpTransform(0);
		VectorCopy(orig_angles, m_pCurrentEntity->angles);
	}
	else
	{
		Ent.curstate.controller[0] = 127;
		Ent.curstate.controller[1] = 127;
		Ent.curstate.controller[2] = 127;
		Ent.curstate.controller[3] = 127;
		Ent.latched.prevcontroller[0] = m_pCurrentEntity->curstate.controller[0];
		Ent.latched.prevcontroller[1] = m_pCurrentEntity->curstate.controller[1];
		Ent.latched.prevcontroller[2] = m_pCurrentEntity->curstate.controller[2];
		Ent.latched.prevcontroller[3] = m_pCurrentEntity->curstate.controller[3];

		m_pPlayerInfo = IEngineStudio.PlayerInfo(m_nPlayerIndex);
		m_pPlayerInfo->gaitsequence = 0;

		StudioSetUpTransform(0);
	}

	if (flags & STUDIO_RENDER)
	{
		// see if the bounding box lets us trivially reject, also sets
		if (!IEngineStudio.StudioCheckBBox())
			return 0;

		(*m_pModelsDrawn)++;
		(*m_pStudioModelCount)++; // render data cache cookie

		if (m_pStudioHeader->numbodyparts == 0)
			return 1;
	}

	m_pPlayerInfo = IEngineStudio.PlayerInfo(m_nPlayerIndex);

	StudioSetupBones();
	StudioSaveBones();
	m_pPlayerInfo->renderframe = m_nFrameCount;

	m_pPlayerInfo = NULL;

	if (flags & STUDIO_EVENTS)
	{
		StudioCalcAttachments();
		IEngineStudio.StudioClientEvents();
		// copy attachments into global entity array
		if (m_pCurrentEntity->index > 0)
		{
			cl_entity_t* ent = gEngfuncs.GetEntityByIndex(m_pCurrentEntity->index);

			memcpy(ent->attachment, m_pCurrentEntity->attachment, sizeof(vec3_t) * 4);
		}
	}

	if (flags & STUDIO_RENDER)
	{
		//Master Sword: 'r_himodels' removed
		/*if (m_pCvarHiModels->value && m_pRenderModel != m_pCurrentEntity->model  )
		{
			// show highest resolution multiplayer model
			m_pCurrentEntity->curstate.body = 255;
		}

		if (!(m_pCvarDeveloper->value == 0 && gEngfuncs.GetMaxClients() == 1 ) && ( m_pRenderModel == m_pCurrentEntity->model ) )
		{
			m_pCurrentEntity->curstate.body = 1; // force helmet
		}*/

		//Master Sword - Add local player gear and everything here. This makes it client-side

		//MIB JAN2010 - original code below, replacement follows
		/*
		if( pplayer->number == gEngfuncs.GetLocalPlayer()->index &&
			!FBitSet(m_pCurrentEntity->curstate.colormap,MSRDR_ASPLAYER) )
		{
			RenderPlayer.SetEntity( m_pCurrentEntity );
			RenderPlayer.SetGear( &player.Gear );
			RenderPlayer.m_Gender = (gender_e)player.m_Gender;
			RenderPlayer.SetClientEntity( false ); //Don't add to the entity list - It's already there, passed down from the server
			RenderPlayer.Render( );
		}
		*/
		//MiB - 10_MAR2010 - Armor finally fixed. Again. And once more.
		if (!FBitSet(m_pCurrentEntity->curstate.colormap, MSRDR_ASPLAYER))
		{
			int idx;
			if (pplayer->number == gEngfuncs.GetLocalPlayer()->index)
			{
				RenderPlayer.SetEntity(m_pCurrentEntity);
				RenderPlayer.SetGear(&player.Gear);
				RenderPlayer.m_Gender = (gender_e)player.m_Gender;
				RenderPlayer.SetClientEntity(false); //Don't add to the entity list - It's already there, passed down from the server
				RenderPlayer.Render();

				//m_pCurrentEntity->curstate.body = gEngfuncs.GetLocalPlayer()->curstate.body;
				idx = gEngfuncs.GetLocalPlayer()->index; //MiB AUG2010_01 - was curstate.number
			}
			else
				idx = m_pCurrentEntity->index; //MiB AUG2010_01 - was curstate.number

			m_pCurrentEntity->curstate.body = playerBodyArray[idx];
		}

		/*lighting.plightvec = dir;
		IEngineStudio.StudioDynamicLight(m_pCurrentEntity, &lighting );

		IEngineStudio.StudioEntityLight( &lighting );

		// model and frame independant
		IEngineStudio.StudioSetupLighting (&lighting);*/

		m_pPlayerInfo = IEngineStudio.PlayerInfo(m_nPlayerIndex);

		// get remap colors
		m_nTopColor = V_min(V_max(0, m_pPlayerInfo->topcolor), 360);
		m_nBottomColor = V_min(V_max(0, m_pPlayerInfo->bottomcolor), 360);

		IEngineStudio.StudioSetRemapColors(m_nTopColor, m_nBottomColor);

		//Master Sword - don't render the normal player model. -- UNDONE
		//Instead, the bodypart attachmets are rendered -- UNDONE
		StudioRenderModel();

		m_pPlayerInfo = NULL;

		/*
		Master Sword - don't render the weaponmodel
		if (pplayer->weaponmodel)
		{
			cl_entity_t saveent = *m_pCurrentEntity;

			model_t *pweaponmodel = IEngineStudio.GetModelByIndex( pplayer->weaponmodel );

			m_pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata (pweaponmodel);
			IEngineStudio.StudioSetHeader( m_pStudioHeader );

			StudioMergeBones( pweaponmodel);

			IEngineStudio.StudioSetupLighting (&lighting);

			StudioRenderModel( );

			StudioCalcAttachments( );

			*m_pCurrentEntity = saveent;
		}*/
	}

	return 1;
}

/*
====================
StudioCalcAttachments

====================
*/
void CStudioModelRenderer::StudioCalcAttachments(void)
{
	int i;
	mstudioattachment_t* pattachment;

	if (m_pStudioHeader->numattachments > 4)
	{
		gEngfuncs.Con_DPrintf("Too many attachments on %s\n", m_pCurrentEntity->model->name);
		exit(-1);
	}

	// calculate attachment points
	pattachment = (mstudioattachment_t*)((byte*)m_pStudioHeader + m_pStudioHeader->attachmentindex);
	for (i = 0; i < m_pStudioHeader->numattachments; i++)
	{
		VectorTransform(pattachment[i].org, (*m_plighttransform)[pattachment[i].bone], m_pCurrentEntity->attachment[i]);

		if (FBitSet(m_pCurrentEntity->curstate.oldbuttons, MSRDR_FLIPPED))
		{
			//flip attachment point across the plane going through the middle of the screen
			Plane FlipPlane;
			//EngineFunc::MakeVectors( m_pCurrentEntity->angles, NULL, FlipPlane.m_Normal, NULL );
			FlipPlane.m_Normal = ViewMgr.Params->right;
			FlipPlane.m_Dist = DotProduct(FlipPlane.m_Normal, ViewMgr.Params->vieworg);
			float AttachmentDist = DotProduct(FlipPlane.m_Normal, m_pCurrentEntity->attachment[i]) - FlipPlane.m_Dist;
			m_pCurrentEntity->attachment[i] += -FlipPlane.m_Normal * (AttachmentDist * 2);
		}
	}
}

/*
====================
StudioRenderModel

====================
*/

void CStudioModelRenderer::StudioRenderModel(void)
{
	alight_t lighting;
	vec3_t lightdir;

	HUDScript->Effects_Render(*m_pCurrentEntity, CMirrorMgr::m_CurrentMirror.Enabled);

	lighting.plightvec = lightdir;
	IEngineStudio.StudioDynamicLight(m_pCurrentEntity, &lighting);

	IEngineStudio.StudioEntityLight(&lighting);

	//lighting.color = Vector( 0, 1, 0 );
	if (FBitSet(m_pCurrentEntity->curstate.colormap, MSRDR_GLOW_GRN))
		lighting.color = Vector(0, 1, 0);
	if (FBitSet(m_pCurrentEntity->curstate.colormap, MSRDR_GLOW_RED))
		lighting.color = Vector(1, 0, 0);
	if (FBitSet(m_pCurrentEntity->curstate.colormap, MSRDR_DARK))
		lighting.ambientlight = 0;

	if (FBitSet(m_pCurrentEntity->curstate.colormap, MSRDR_LIGHT_NORMAL))
	{
		lighting.color = Vector(1, 1, 1);
		lighting.ambientlight = 90;
		//dir = (v_origin - m_pCurrentEntity->origin).Normalize();
		lightdir = Vector(0, 0, -1);
		lighting.shadelight = 26;
	}
	if (FBitSet(m_pCurrentEntity->curstate.colormap, MSRDR_LIGHT_DIM))
	{
		lighting.color = Vector(.5, .5, .5);
		lighting.ambientlight = 60;
		//dir = (v_origin - m_pCurrentEntity->origin).Normalize();
		lightdir = Vector(0, 0, -1);
		lighting.shadelight = 100;
	}

	// model and frame independant
	IEngineStudio.StudioSetupLighting(&lighting);

	// get remap colors
	m_nTopColor = m_pCurrentEntity->curstate.colormap & 0xFF;
	m_nBottomColor = (m_pCurrentEntity->curstate.colormap & 0xFF00) >> 8;

	IEngineStudio.StudioSetRemapColors(m_nTopColor, m_nBottomColor);

	//Now, Render
	if (!FBitSet(m_pCurrentEntity->curstate.oldbuttons, MSRDR_SKIP)) //Chance to skip rendering
	{
		IEngineStudio.SetChromeOrigin();
		IEngineStudio.SetForceFaceFlags(0);

		if (m_pCurrentEntity->curstate.renderfx == kRenderFxGlowShell)
		{
			m_pCurrentEntity->curstate.renderfx = kRenderFxNone;
			StudioRenderFinal();

			if (!IEngineStudio.IsHardware())
			{
				gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
			}

			IEngineStudio.SetForceFaceFlags(STUDIO_NF_CHROME);

			gEngfuncs.pTriAPI->SpriteTexture(m_pChromeSprite, 0);
			m_pCurrentEntity->curstate.renderfx = kRenderFxGlowShell;

			StudioRenderFinal();
			if (!IEngineStudio.IsHardware())
			{
				gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
			}
		}
		else
		{
			StudioRenderFinal();
		}
	}

	//Done rendering, reset stuff
	//if( FBitSet(m_pCurrentEntity->curstate.playerclass,ENT_EFFECT_FOLLOW_ROTATE) && m_pCurrentEntity->curstate.owner > 0 )
	//Set it back
	//	m_pCurrentEntity->origin = m_pCurrentEntity->curstate.origin = OldOrigin;
}

/*
====================
StudioRenderFinal_Software

====================
*/
void CStudioModelRenderer::StudioRenderFinal_Software(void)
{
	int i;

	// Note, rendermode set here has effect in SW
	IEngineStudio.SetupRenderer(0);

	if (m_pCvarDrawEntities->value == 2)
	{
		IEngineStudio.StudioDrawBones();
	}
	else if (m_pCvarDrawEntities->value == 3)
	{
		IEngineStudio.StudioDrawHulls();
	}
	else
	{
		for (i = 0; i < m_pStudioHeader->numbodyparts; i++)
		{
			IEngineStudio.StudioSetupModel(i, (void**)&m_pBodyPart, (void**)&m_pSubModel);
			IEngineStudio.StudioDrawPoints();
		}
	}

	if (m_pCvarDrawEntities->value == 4)
	{
		gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
		IEngineStudio.StudioDrawHulls();
		gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
	}

	if (m_pCvarDrawEntities->value == 5)
	{
		IEngineStudio.StudioDrawAbsBBox();
	}

	IEngineStudio.RestoreRenderer();
}

/*
====================
StudioRenderFinal_Hardware

====================
*/
void CStudioModelRenderer::StudioRenderFinal_Hardware(void)
{
	int i;
	int rendermode;

	rendermode = IEngineStudio.GetForceFaceFlags() ? kRenderTransAdd : m_pCurrentEntity->curstate.rendermode;
	IEngineStudio.SetupRenderer(rendermode);

	if (m_pCvarDrawEntities->value == 2)
	{
		IEngineStudio.StudioDrawBones();
	}
	else if (m_pCvarDrawEntities->value == 3)
	{
		IEngineStudio.StudioDrawHulls();
	}
	else
	{
		for (i = 0; i < m_pStudioHeader->numbodyparts; i++)
		{
			IEngineStudio.StudioSetupModel(i, (void**)&m_pBodyPart, (void**)&m_pSubModel);
			if (m_DrawStyle == DRAW_BOTHFACES)
				gEngfuncs.pTriAPI->CullFace(TRI_NONE);
			else if (m_DrawStyle == DRAW_BACKFACES)
				gEngfuncs.pTriAPI->CullFace(TRI_NONE);

			if (m_fDoInterp)
			{
				// interpolation messes up bounding boxes.
				m_pCurrentEntity->trivial_accept = 0;
			}

			IEngineStudio.GL_SetRenderMode(rendermode);

			/*if( CMirrorMgr::m_CurrentMirror.Enabled )
			{
				glColorMask( 1, 1, 1, 1 );
				CMirror &Mirror = *CMirrorMgr::m_CurrentMirror.Mirror;
				float Alpha = !Mirror.m_Texture->Mirror.NoWorld ? 1 : Mirror.m_Texture->Mirror.Color.a;
				gEngfuncs.pTriAPI->Color4f( Mirror.m_Texture->Mirror.Color.r, Mirror.m_Texture->Mirror.Color.g, Mirror.m_Texture->Mirror.Color.b, Alpha );
				//glEnable( GL_BLEND );
			}*/

			IEngineStudio.StudioDrawPoints();
			IEngineStudio.GL_StudioDrawShadow();
		}
	}

	if (m_pCvarDrawEntities->value == 4)
	{
		gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
		IEngineStudio.StudioDrawHulls();
		gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
	}

	IEngineStudio.RestoreRenderer();
}

/*
====================
StudioRenderFinal

====================
*/

//true == Setup to render a studio model
//false == undo the setup
void CStudioModelRenderer::StudioSetupRender(bool Setup)
{
	m_DrawStyle = DRAW_DEFAULT;

	if (CMirrorMgr::m_CurrentMirror.Enabled)
	{
		CMirror& Mirror = *CMirrorMgr::m_CurrentMirror.Mirror;

		if (Setup)
		{
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();

			Mirror.ApplyTransformation();
			m_DrawStyle = DRAW_BACKFACES;
		}
		else
		{
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}
	}

	//Master Sword - Flip the model (left-right) the when neccesary
	if (FBitSet(m_pCurrentEntity->curstate.oldbuttons, MSRDR_FLIPPED))
	{
		if (Setup)
		{
			float mm[4][4];
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glGetFloatv(GL_MODELVIEW_MATRIX, (float*)mm);

			glLoadIdentity();
			glScalef(-1, 1, 1);

			glMultMatrixf((float*)mm);

			m_DrawStyle = DRAW_BACKFACES;
		}
		else
		{
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}
	}
}

void CStudioModelRenderer::StudioRenderFinal(void)
{
	StudioSetupRender(true);

	if (IEngineStudio.IsHardware())
	{
		StudioRenderFinal_Hardware();
	}
	else
	{
		StudioRenderFinal_Software();
	}

	StudioSetupRender(false);
}
/*
==================
Dogg - FlipModel

Mirrors the model's X vertices across the axis and reverses the vertex order of every triangle
Incidently, this slows down rendering because I have to destruct the original triange strips and triangle fans,
replacing tem with my own triangle strips which only allows for one triangle per strip (slow).
==================
*/

/*int LeftView_OldCommands[MAXSTUDIOMESHES];
static short LeftView_Commands[60000];
void CStudioModelRenderer::FlipModel( bool Enable )
{
	//Master Sword - flip view model vertices & normals if requested by game.
	//				 The bones are flipped elsewhere
	//m_pCurrentEntity->curstate.oldbuttons - Stores whether this entity should be flipped
	//m_pRenderModel->surfaces - Store whether the model has already been flipped

	if( !FBitSet(m_pCurrentEntity->curstate.oldbuttons,MSRDR_FLIPPED) )
		return;

	//Flag for indicating whether the model vertices have been flipped
	m_pRenderModel->surfaces = Enable ? (msurface_t *)(MSRDR_FLIPPED) : NULL;

	short *newComands = LeftView_Commands;						//Pointer to my new commands, with only one triangle per command and all triangles in reverse vertex order

	for( int i = 0; i < m_pStudioHeader->numbodyparts; i++ )
	{
		mstudiobodyparts_t	*pbodypart = (mstudiobodyparts_t *)((byte *)m_pStudioHeader + m_pStudioHeader->bodypartindex) + i;

		int index = i / pbodypart->base;
		index = index % pbodypart->nummodels;

		mstudiomodel_t		*m_pmodel = (mstudiomodel_t *)((byte *)m_pStudioHeader + pbodypart->modelindex) + index;
		vec3_t *pstudioverts = (vec3_t *)((byte *)m_pStudioHeader + m_pmodel->vertindex);
		vec3_t *pstudionorms = (vec3_t *)((byte *)m_pStudioHeader + m_pmodel->normindex);

		for( int v = 0; v < m_pmodel->numverts; v++ )	pstudioverts[v][0] *= -1;	//Flip the vertices
		for( int n = 0; n < m_pmodel->numnorms; n++)	pstudionorms[n][0] *= -1;	//Flip the normals


		 for (int m = 0; m < m_pmodel->nummesh; m++)
		{
			mstudiomesh_t *pmesh = (mstudiomesh_t *)((byte *)m_pStudioHeader + m_pmodel->meshindex) + m;
			if( !Enable )
			{
				pmesh->triindex = LeftView_OldCommands[m];							//Restore the pointer, allowing the engine to once again render the model's triangle commands
				continue;
			}

			LeftView_OldCommands[m] = (int)pmesh->triindex;							//Save this pointer, then change it

			short *ptricmds = (short *)((byte *)m_pStudioHeader + pmesh->triindex);	//Get pointer to original triangle commands
			pmesh->triindex = (int)((int)newComands - (int)m_pStudioHeader);		//Set this pointer to now render our local commands, instead of the model's normal command


			while( ptricmds[0] )
			{
				short NumTriangleCmds = ptricmds[0];
				ptricmds++;
				bool IsFan = false;

				if( NumTriangleCmds < 0 )		//Triangle fan commands
				{
					NumTriangleCmds *= -1;
					IsFan = true;
				}

				{
					short *Modifycmds = ptricmds;
					Modifycmds += (2 * 4);
					 for (int Triangle = 0; Triangle < NumTriangleCmds-2; Triangle++)
					{
						newComands[0] = 3;	//Num of cmds in new triangle (each command is 4 shorts long)
						newComands++;		//Get ready to store commands

						int Ofs1 = 2, Ofs2 = 1, Ofs3 = 0;

						//Convert from triangle fans or triangle strips to Simple Triangle Strips (only one triangle per strip)
						//The vertices are copies, but in reverse order so they're flipped
						if( !IsFan )
						{
							//Handling triangle strips
							//Every other triangle must be inverted, so check that.
							if( Triangle % 2 != 0 )		{ Ofs1 = 2; Ofs2 = 1; Ofs3 = 0; }					//Vertex #0, 1, 2
							else						{ Ofs1 = 1; Ofs2 = 2; Ofs3 = 0; }					//Vertex #1, 0, 2
						}
						//Handling triangle fans
						//Start with the first vertex in the set, then go to the last, and finally to the prev one.
						//This copies the triangle in reverse order
						else							{ Ofs1 = (Triangle+2); Ofs2 = 0; Ofs3 = 1; }		//Vertex #(first vertex)

						memcpy( newComands, Modifycmds - (Ofs1 * 4), sizeof(short) * 4 ); newComands += 4;	//Copy first vertex
						memcpy( newComands, Modifycmds - (Ofs2 * 4), sizeof(short) * 4 ); newComands += 4;	//Copy second vertex
						memcpy( newComands, Modifycmds - (Ofs3 * 4), sizeof(short) * 4 ); newComands += 4;  //Copy third vertex

						if( (newComands - LeftView_Commands) > 50000 )
							MSErrorConsoleText( "CStudioModelRenderer::FlipModel", "Warning! Flipped View model Triangle Commands Approaching 60000!\n" );
						Modifycmds += 4;
					}

					ptricmds += NumTriangleCmds * 4;
					continue;
				}
			}
			newComands[0] = 0;
		}
	}
}
*/
//Master Sword - Dynamically modify a bone
void CStudioModelRenderer::ModifyBone(mstudiobone_t* pbone, Vector& angle)
{
	if (!m_pCurrentEntity->player)
		return;

	if (m_pCurrentEntity->FullRotate())
		return;

	//if( FBitSet(m_pCurrentEntity->curstate.colormap, MSRDR_ASPLAYER) )
	//	return;

	//return; //Don't work in multiplayer for some reason

	int Bone = 0;
	if (!_stricmp(pbone->name, "Bip01 Spine"))
		Bone = 0;
	else if (!_stricmp(pbone->name, "Bip01 Spine1"))
		Bone = 1;
	else if (!_stricmp(pbone->name, "Bip01 Spine2"))
		Bone = 2;
	else
		return;

	//Make players look up/down by rotating bones
	float Value = 0;
	float Pitch = m_pCurrentEntity->curstate.fuser2;
	if (m_pCurrentEntity->index == gEngfuncs.GetLocalPlayer()->index)
		Pitch *= -1;

	if (Bone == 0)
		Value = Pitch * 0.5;
	if (Bone == 1)
		Value = Pitch * 0.4f; //0.2
	if (Bone == 2)
		Value = Pitch * 0.3f; //0.2

	angle.z += -Value * (M_PI / 180.0);
}

void CStudioModelRenderer::RegisterExtraData(cl_entity_t& Ent)
{
	if (!Ent.HasExtra() && Ent.index > 0 && Ent.index < 4096)
	{
		//This is the first rendering of a new model.  Allocate an extra info entry
		modelinfo_t& ModelInfo = CModelMgr::m_ModelInfo[Ent.index];
		clrmem(ModelInfo);

		ModelInfo.Used = true;
		SetBits(Ent.mouth.sndavg, MSRDR_HASEXTRA);
		ModelInfo.Bones = m_pStudioHeader->numbones;
	}
}
