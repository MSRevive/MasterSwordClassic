#include "buildcontrol.h"
#include "hl/extdll.h"
#include "hl/util.h"
#include "iscript.h"	//MS scripted interface
#include "sharedutil.h" //MS generic utility functions
#include "cbase.h"

#pragma warning(disable : 4995) // allow use of old headers (such as string.h, iostream.h, etc.)
#pragma warning(disable : 4288) // loop control variable declared in the for-loop is used outside the for-loop scope

CBaseEntity *MSInstance(edict_t *pent);
CBaseEntity *MSInstance(entvars_t *pev);

#ifndef VALVE_DLL
#undef DLLEXPORT
#endif