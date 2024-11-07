/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
// cl_entity.h
#if !defined(CL_ENTITYH)
#define CL_ENTITYH
#ifdef _WIN32
#pragma once
#endif

typedef struct efrag_s
{
	struct mleaf_s *leaf;
	struct efrag_s *leafnext;
	struct cl_entity_s *entity;
	struct efrag_s *entnext;
} efrag_t;

typedef struct
{
	byte mouthopen; // 0 = mouth closed, 255 = mouth agape
	byte sndcount;	// counter for running average
	int sndavg;		// running average
} mouth_t;

typedef struct
{
	float prevanimtime;
	float sequencetime;
	byte prevseqblending[2];
	vec3_t prevorigin;
	vec3_t prevangles;

	int prevsequence;
	float prevframe;

	byte prevcontroller[4];
	byte prevblending[2];
} latchedvars_t;

typedef struct
{
	// Time stamp for this movement
	float animtime;

	vec3_t origin;
	vec3_t angles;
} position_history_t;

typedef struct cl_entity_s cl_entity_t;

#define HISTORY_MAX 64 // Must be power of 2
#define HISTORY_MASK (HISTORY_MAX - 1)

#if !defined(ENTITY_STATEH)
#include "entity_state.h"
#endif

#if !defined(PROGS_H)
#include "progs.h"
#endif

struct cl_entity_s
{
	int index = 0; // Index into cl_entities ( should match actual slot, but not necessarily )

	qboolean player = false; // True if this entity is a "player"

	entity_state_t baseline{ 0 };  // The original state from which to delta during an uncompressed message
	entity_state_t prevstate{ 0 }; // The state information from the penultimate message received from the server
	entity_state_t curstate{ 0 };  // The state information from the last message received from server

	int current_position;				// Last received history update index
	position_history_t ph[HISTORY_MAX]; // History of position and angle updates for this player

	mouth_t mouth; // For synchronizing mouth movements.

	latchedvars_t latched; // Variables used by studio model rendering routines

	// Information based on interplocation, extrapolation, prediction, or just copied from last msg received.
	//
	float lastmove;

	// Actual render position and angles
	vec3_t origin;
	vec3_t angles;

	// Attachment points
	vec3_t attachment[4];

	// Other entity local information
	int trivial_accept;

	struct model_s *model = nullptr;	 // cl.model_precache[ curstate.modelindes ];  all visible entities have a model
	struct efrag_s *efrag;	 // linked list of efrags
	struct mnode_s *topnode; // for bmodels, first world node that splits bmodel, or NULL if not split

	float syncbase; // for client-side animations -- used by obsolete alias animation system, remove?
	int visframe;	// last frame this entity was found in an active leaf
	colorVec cvFloorColor;

	bool SetModel(const char *pszModelName);

	void AttachTo(cl_entity_t &Owner)
	{
		origin = curstate.origin = Owner.origin;
		curstate.aiment = Owner.index;
		curstate.movetype = MOVETYPE_FOLLOW;
	}
	bool HasExtra(); //Extra data for this model is already initialized
	void PlayAnim(int Anim);
	bool PlayAnim(const char *Anim);
	void SetBody(int Group, int Value);
	bool GetBonePos(int BoneNum, Vector &BonePos);
	Vector GetBonePosVec(int BoneNum, Vector &BonePos); //AUG2013_25 Thothie - fixing cl bonepos acquisition
	int GetBoneCount();									//AUG2013_25 Thothie - getting bone count
	bool Exists();
	Vector Origin(); //The final origin, taking my attachment to a parent into account

	bool FullRotate(); //Full rotation is when a player model is allowed to rotate its pitch
					   //This happens while swimming
};

#endif // !CL_ENTITYH
