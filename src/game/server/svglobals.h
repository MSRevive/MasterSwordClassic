#ifndef INC_MSGLOBALS
#define INC_MSGLOBALS

//Global initialiazations

bool MSGlobalInit();
void MSWorldSpawn();
void MSGameThink();
void MSGameEnd();
void SendHUDMsgAll(msstring_ref Title, msstring_ref Text);

#define SCRIPT_TYPES 4

class CSVGlobals
{
public:
	static bool LogScripts;
	static void LogScript(msstring_ref ScriptName, class CBaseEntity *pOwner, int includelevel, bool PrecacheOnly, bool Sucess);
	static void WriteScriptLog();

	struct scriptlistitem_t
	{
		msstring FileName;
		bool Included;
	};
	static mslist<scriptlistitem_t> ScriptList[SCRIPT_TYPES];
};

#define SPAWN_GENERIC "ms_player_spawn"
#define SPAWN_BEGIN "ms_player_begin"
#define SPAWN_SPECTATE "ms_player_spec"
#define OLD_GENERIC_SPAWN "info_player_start"

#ifdef CVARDEF_H
extern cvar_s ms_joinreset;
extern cvar_t *g_psv_gravity;
extern cvar_t *g_psv_aim;
extern cvar_t *g_footsteps;
extern cvar_t *g_maxspeed;
extern cvar_t *g_accelerate;
extern cvar_t *g_airaccelerate;
extern cvar_t *g_wateraccelerate;
extern cvar_t *g_stepsize;
extern cvar_t *g_airaccelerate;
extern cvar_t *g_friction;
extern cvar_t *g_stopspeed;
extern cvar_t *g_waterfriction;

#ifdef DEV_BUILD
extern cvar_s ms_allowdev;
#endif
#endif

#endif //INC_MSGLOBALS