#include "msdllheaders.h"
#include "global.h"
#include "logger.h"
#include "time.h"

#ifdef VALVE_DLL
#include "svglobals.h"
#else
#include "hud.h"
#include "cl_util.h"
#include "player/player.h"
#include "SDL2/SDL_messagebox.h"
extern CBasePlayer player;
#endif

CBaseEntity *MSInstance(edict_t *pent);
Logger logfile;
#ifdef VALVE_DLL
Logger chatlog;
#endif
Logger NullFile;
bool g_log_initialized = false;
void MSErrorConsoleText(const msstring_ref pszLabel, const msstring_ref Progress)
{
	//Print("%s, %s\n", pszLabel, Progress);
#ifndef TURN_OFF_ALERT
	if (g_log_initialized)
	{
		msstring Output = "Error ";
#ifdef VALVE_DLL
		Output += "(SERVER): ";
#else
		Output += "(CLIENT): ";
#endif
		Output += pszLabel;
		Output += " --> ";
		Output += Progress;
		Output += "\r\n";
		if (logfile.is_open())
		{
			logfile << Output;
		}
		Print("%s", Output);
	}
	else
	{
		//This is prety fatal - We got an error before the logs were initialized
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, msstring(pszLabel) + msstring(" (Logs not yet initialized)"), Progress, NULL);
	}
#endif
}

void OpenLogFiles()
{
	char cLogfile[MAX_PATH];
	
#ifdef VALVE_DLL
	char cChatfile[MAX_PATH];
	
	time_t curTime;
	time(&curTime);
	struct tm* TheTime = localtime(&curTime);
	int month = TheTime->tm_mon + 1;
	int year = TheTime->tm_year + 1900;
	char pChatname[20];
	_snprintf(pChatname, sizeof(pChatname), "msr_chatlog_%02d_%i", month, year);
	_snprintf(cChatfile, MAX_PATH, "%s/%s.log", MSGlobals::AbsGamePath.c_str(), pChatname);
	
	_snprintf(cLogfile, MAX_PATH, "%s/%s.log", MSGlobals::AbsGamePath.c_str(), "log_msdll");
	
	logfile.open(cLogfile);
	chatlog.open(cChatfile, 1);
#else
	_snprintf(cLogfile, MAX_PATH, "%s/%s.log", MSGlobals::AbsGamePath.c_str(), "log_cldll");
	logfile.open(cLogfile);
#endif
	g_log_initialized = true;
}

#define ENT_FORMAT ENT_PREFIX "(%i,%u)"
msstring EntToString(class CBaseEntity *pEntity) // Converts an entity to a string of format "PentP(idx,addr)"
{
	if (!pEntity)
		return "";

	static char RetString[32];
	_snprintf(RetString, sizeof(RetString), ENT_FORMAT, pEntity->entindex(), (int)pEntity);

	return RetString;
}
CBaseEntity *StringToEnt(msstring_ref EntString) // Converts an string of format "PentP(idx,addr)" to an entity
{
	int Idx = -1;
	unsigned int Addr = ~0;

	if (sscanf(EntString, ENT_FORMAT, &Idx, &Addr) < 2)
		return NULL;

	CBaseEntity *pEntity = MSInstance(INDEXENT(Idx));
	if (!pEntity || (uint)pEntity != Addr)
		return NULL;

	return pEntity;
}

msstring_ref VecToString(const Vector& Vec, bool bAs2D)
{
	static char RetString[128];
	if (bAs2D)
		_snprintf(RetString, sizeof(RetString), "(%.2f,%.2f)", Vec.x, Vec.y);
	else
		_snprintf(RetString, sizeof(RetString), "(%.2f,%.2f,%.2f)", Vec.x, Vec.y, Vec.z);
	return RetString;
}
Vector StringToVec(msstring_ref String)
{
	Vector Vec;

	//This allows you to specify less than all three coordinates... in case you need a 2D vector
	if (sscanf(String, "(%f,%f,%f)", &Vec.x, &Vec.y, &Vec.z) < 3)
		if (sscanf(String, "(%f,%f)", &Vec.x, &Vec.y) < 2)
			return g_vecZero;
	return Vec;
}
Color4F StringToColor(msstring_ref String) //Converts a string of the format "(r,g,b,a)" Color class
{
	Color4F Color(0, 0, 0, 0);

	sscanf(String, "(%f,%f,%f,%f)", &Color.r, &Color.g, &Color.b, &Color.a);
	return Color;
}

//Uses Dir.x for right-left, Dir.y for forward-back, and Dir.z as up-down, relative to the angle
Vector GetRelativePos(Vector &Ang, Vector &Dir)
{
	Vector vForward, vRight, vUp, vPosition;

	EngineFunc::MakeVectors(Ang, vForward, vRight, vUp); //Use the mutal client/server friendly version of this

	vPosition = vRight * Dir.x;
	vPosition += vForward * Dir.y;
	vPosition += vUp * Dir.z;

	return vPosition;
}
//Adds models/ or sprites/ to a model or sprite filename
char *GetFullResourceName(msstring_ref pszPartialName)
{
	msstring PartialName = pszPartialName;
	if (PartialName.len() < 4)
		return (char *)PartialName;

	static msstring sReturn;

	sReturn = PartialName;
	msstring Extension = &PartialName[PartialName.len() - 4];
	if (Extension == ".spr")
		sReturn = msstring("sprites/") + PartialName;
	else if (Extension == ".mdl")
		sReturn = msstring("models/") + PartialName;

	return sReturn;
}

inline CBaseEntity *PrivData(entvars_t *pev) { return (CBaseEntity *)pev->pContainingEntity; }

CBaseEntity *MSInstance(entvars_t *pev)
{
	if (!pev)
		return NULL;
#ifdef VALVE_DLL
	CBaseEntity *pEnt = GetClassPtr((CBaseEntity *)pev);
#else
	//In the client DLL, edict() (our parameter) == pev
	CBaseEntity *pEnt = PrivData((entvars_t *)pev);
#endif
	return pEnt;
}
CBaseEntity *MSInstance(edict_t *pent)
{
	if (!pent)
		return NULL;
#ifdef VALVE_DLL
	CBaseEntity *pEnt = (CBaseEntity *)GET_PRIVATE(pent);
#else
	//In the client DLL, edict() (our parameter) == pev
	CBaseEntity *pEnt = PrivData((entvars_t *)pent);
#endif
	return pEnt;
}

int iBeam;

void BeamEffect(float SRCx, float SRCy, float SRCz, float DESTx,
				float DESTy, float DESTz, int sprite, int startframe,
				int framerate, int life, int width, int noise,
				int r, int g, int b, int brightness, int ispeed)
{
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, NULL);
	WRITE_BYTE(TE_BEAMPOINTS);
	WRITE_COORD(SRCx);
	WRITE_COORD(SRCy);
	WRITE_COORD(SRCz);
	WRITE_COORD(DESTx);
	WRITE_COORD(DESTy);
	WRITE_COORD(DESTz);
	WRITE_SHORT(sprite);
	WRITE_BYTE(startframe); // startframe
	WRITE_BYTE(framerate);	// framerate
	WRITE_BYTE(life);		//life
	WRITE_BYTE(width);		// width
	WRITE_BYTE(noise);		// noise
	WRITE_BYTE(r);
	WRITE_BYTE(g);
	WRITE_BYTE(b);
	WRITE_BYTE(brightness); // brightness
	WRITE_BYTE(ispeed);		// speed
	MESSAGE_END();
}
void BeamEffect(Vector vStart, Vector vEnd, int sprite, int startframe,
				int framerate, int life, int width, int noise,
				int r, int g, int b, int brightness, int ispeed)
{
	BeamEffect(vStart.x, vStart.y, vStart.z, vEnd.x, vEnd.y, vEnd.z,
			   sprite, startframe, framerate, life, width, noise, r, g, b,
			   brightness, ispeed);
}

int numofdigits(int x)
{
	int idigits = 1;
	while (x >= pow(10, idigits) && idigits < 256)
		idigits++;
	return idigits;
}

#ifndef _WIN32
extern "C" char *strlwr(char *str)
{
	char *orig = str;
	// process the string
	for (; *str != '\0'; str++)
		*str = tolower(*str);
	return orig;
}
#endif

void ErrorPrint(msstring vsUnqeTag, int vFlags, const char *szFmt, ...)
{
	static char	string[1024];

  	va_list argptr;
  	va_start(argptr, szFmt);
  	vsprintf(string, szFmt,argptr);
  	va_end(argptr);

    msstring vsShortTitle = "[";
    vsShortTitle += vsUnqeTag + "] ";

    #ifdef VALVE_DLL
        vsShortTitle += "Server";
    #else
        vsShortTitle += "Client";
    #endif

    msstring vsTitle = "[MSC_ERROR]";
    vsTitle += vsShortTitle;

    msstring vsAsOne = vsTitle + ": " + string + "\n";
    if (vFlags & ERRORPRINT_LOG)
    {
        logfile << vsAsOne << "\n";
    }
    if (vFlags & ERRORPRINT_CONSOLE)
    {
#ifdef VALVE_DLL
		ALERT(at_console, "%s", vsAsOne.c_str());
#else
		ConsolePrint(vsAsOne.c_str());
#endif
    }
    if (vFlags & ERRORPRINT_INFOMSG)
    {
      #ifdef VALVE_DLL
		    SendHUDMsgAll(vsShortTitle, string);
	    #else
		    player.SendHUDMsg(vsShortTitle, string);
	    #endif
    }
    if (vFlags & ERRORPRINT_CVAR)
    {
        CVAR_SET_STRING("ms_error_tracker", vsShortTitle + string);
    }
    if (vFlags & ERRORPRINT_POPUP)
    {
#ifndef VALVE_DLL
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, vsShortTitle.c_str(), string, NULL);
#endif
    }
}

//Needed to support sound.cpp
//#include "talkmonster.h"
//float	CTalkMonster::g_talkWaitTime = 0;		// time delay until it's ok to speak: used so that two NPCs don't talk at once

msscriptarray * GetScriptedArrayFromHashMap(msscriptarrayhash &vArrayHashMap, msstring &vsName, bool bAllowCreate, bool *pbExisted)
{
    msscriptarrayhash::iterator iArray = vArrayHashMap.find( vsName );

    if (iArray == vArrayHashMap.end())
    {
        if (pbExisted)
        {
            (*pbExisted) = false;
        }
        if (bAllowCreate)
        {
            msscriptarray vArray;
            vArrayHashMap[vsName] = vArray;
            return &vArrayHashMap[vsName];
        }
    }
    else
    {
        if (pbExisted)
        {
            (*pbExisted) = true;
        }
        return &iArray->second;
    }

    return NULL;
}

msscripthash * GetScriptedHashMapFromHashMap(msscripthashhash &vHashMapHashMap, msstring &vsName, bool bAllowCreate, bool *pbExisted)
{
    msscripthashhash::iterator iHash = vHashMapHashMap.find( vsName );
    if (iHash == vHashMapHashMap.end())
    {
        if (pbExisted)
        {
            (*pbExisted) = false;
        }
        if (bAllowCreate)
        {
            msscripthash vHashMap;
            vHashMapHashMap[vsName] = vHashMap;
            return &vHashMapHashMap[vsName];
        }
    }
    else
    {
        if (pbExisted)
        {
            (*pbExisted) = true;
        }
        return &iHash->second;
    }

    return NULL;
}

msscriptset * GetScriptedSetFromHashMap(msscriptsethash &vScriptHashMap, msstring &vsName, bool bAllowCreate, bool *pbExisted)
{
    msscriptsethash::iterator iSet = vScriptHashMap.find(vsName);
    if (iSet == vScriptHashMap.end())
    {
        if (pbExisted)
        {
            (*pbExisted) = false;
        }
        if (bAllowCreate)
        {
            msscriptset vSet;
            vScriptHashMap[vsName] = vSet;
            return &vScriptHashMap[vsName];
        }
    }
    else
    {
        if (pbExisted)
        {
            (*pbExisted) = true;
        }
        return &iSet->second;
    }

    return NULL;
}