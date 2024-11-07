/*
	Master Sword - A Half-life Total Conversion
	Script.cpp - Parses & Executes script event files for NPCs or weapons
*/

#include "inc_weapondefs.h"
#include "animation.h"
#include "soundent.h"
#include "script.h"
#include "stats/races.h"
#include "stats/statdefs.h"
#include "titles.h"
#include "scriptedeffects.h"
#include "logger.h"
#include "pm_defs.h"

#ifndef VALVE_DLL
#include "render/clrender.h"
#include "ms/clglobal.h"
#include "SDL2/SDL_messagebox.h"

// MiB MAR2015_01 [LOCAL_PANEL] - Includes for VGUI elements
#include "hud.h"
#include "cl_util.h"
#include "vgui_teamfortressviewport.h"
#include "ms/vgui_mscontrols.h"
#include "ms/vgui_localizedpanel.h"
#else
#include "svglobals.h"
#include "global.h"
#endif

#undef SCRIPTVAR
#define SCRIPTVAR GetVar								//A script-wide or global variable
#define ERROR_MISSING_PARMS MSErrorConsoleText( "ExecuteScriptCmd", UTIL_VarArgs("Script: %s, %s - not enough parameters!\n", m.ScriptFile.c_str(), Cmd.Name().c_str()) )

#define VecMultiply( a, b ) Vector( a[0] * b[0], a[1] * b[1], a[2] * b[2] )
void Player_UseStamina(float flAddAmt);
extern "C" playermove_t *pmove;

void CScript::Script_Setup()
{
	if (m_GlobalCmdHash.empty())
	{
		m_GlobalCmdHash["if"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_If, true);        //The old if
		m_GlobalCmdHash["if()"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_If, true);    //The new if
		//m_GlobalCmdHash["else"] = scriptcmdscpp_cmdfunc_t(NULL, true );    //Really just part of if()
		m_GlobalCmdHash["exitevent"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_exitevent, false);    //Thothie FEB2008a - tired if !EXIT_SUB -but this is pointless - I need one that exits the whole event

		//Zap dbg/debugprint in public releases
		m_GlobalCmdHash["debugprint"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Debug);
#if !TURN_OFF_ALERT
		m_GlobalCmdHash["dbg"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Debug);
#endif
		m_GlobalCmdHash["name"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Name);
		m_GlobalCmdHash["name_prefix"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_NamePrefix);
		m_GlobalCmdHash["name_unique"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_NameUnique);
		m_GlobalCmdHash["kill"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Kill); //Thothie FEB2009_21 - slay player without XP loss
		m_GlobalCmdHash["hitmulti"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_HitMulti); //Thothie FEB2009_18
		m_GlobalCmdHash["hud.addstatusicon"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_HudIcon); //Drigien APR2008
		m_GlobalCmdHash["hud.addimgicon"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_HudIcon);    //Drigien MAY2008
		m_GlobalCmdHash["hud.killicons"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_HudIcon);    //Drigien MAY2008
		m_GlobalCmdHash["hud.killstatusicon"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_HudIcon);//Drigien MAY2008
		m_GlobalCmdHash["hud.killimgicon"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_HudIcon);    //Drigien MAY2008
		m_GlobalCmdHash["hud.setfnconnection"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_HudIcon);    //Drigien MAY2008
		m_GlobalCmdHash["tospawn"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_ToSpawn); //Thothie APR2008b desperate hacks to fix transition BS
		m_GlobalCmdHash["settrans"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetTrans); //Thothie APR2008b set character transition
		m_GlobalCmdHash["setviewmodelprop"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetViewModelProp); //Shuriken MAR2008
		m_GlobalCmdHash["gagplayer"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_GagPlayer); //Thothie FEB2008b - for admin_gag function
		m_GlobalCmdHash["setpvp"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetPVP); //Thothie FEB2008a - direct setting of PVP for Votepvp
		m_GlobalCmdHash["strconc"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_StrConc); //Thothie FEB2008a - simpler string concatenation
		m_GlobalCmdHash["messageall"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_MessageAll); //playermessage for all huds
		m_GlobalCmdHash["erasefile"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_EraseFile); //MiB FEB2008a - File I/O
		m_GlobalCmdHash["writeline"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_WriteLine); //MiB FEB2008a - File I/O
		m_GlobalCmdHash["chatlog"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_ChatLog); //Thothie FEB2008a - File I/O
		m_GlobalCmdHash["clientcmd"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_ClientCmd); //Thoth FEB2008a - send commands direct to client
		m_GlobalCmdHash["servercmd"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_ServerCmd); //Thoth FEB2008a - server commands for script<->server management
		m_GlobalCmdHash["splayviewanim"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SPlayViewAnim); //MiB DEC2007a - server-side initiated view anim
		m_GlobalCmdHash["wipespell"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_WipeSpell); //MiB DEC2007a - Remove an individual spell
		m_GlobalCmdHash["overwritespell"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_OverwriteSpell); //MiB DEC2007a - Overwrite an existing spell
		m_GlobalCmdHash["xdodamage"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_XDoDamage); //Thothie DEC2007a - new, more dynamic dodamage
		m_GlobalCmdHash["attackprop"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_AttackProp); //MiB DEC2007a - change attack properties
		m_GlobalCmdHash["changelevel"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_ChangeLevel); //Thoth DEC2007a - changelevel command for scripts
		m_GlobalCmdHash["playername"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_PlayerName); //MiB/Thoth DEC2007a sets a players name
		m_GlobalCmdHash["playertitle"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_PlayerTitle); //MiB/Thoth DEC2007a sets a players title
		m_GlobalCmdHash["desc"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Desc);
		m_GlobalCmdHash["weight"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Weight);
		m_GlobalCmdHash["projectilesize"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_ProjectileSize); //Thoth MAR2012 - hasn't been used for inventory volume for years, but is still used as projectile bounding box, so renamed
		m_GlobalCmdHash["setbbox"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetBBox);
		m_GlobalCmdHash["gravity"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Gravity);
		m_GlobalCmdHash["setangle"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetAngle);
		m_GlobalCmdHash["movetype"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_MoveType);
		m_GlobalCmdHash["callevent"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_CallEvent);
		m_GlobalCmdHash["callexternal"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_CallEvent);
		m_GlobalCmdHash["calleventtimed"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_CallEvent);
		m_GlobalCmdHash["calleventloop"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_CallEvent);
		m_GlobalCmdHash["incvar"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_MathSet);
		m_GlobalCmdHash["decvar"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_MathSet);
		m_GlobalCmdHash["inc"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_MathSet);
		m_GlobalCmdHash["dec"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_MathSet);
		m_GlobalCmdHash["add"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_MathSet);
		m_GlobalCmdHash["subtract"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_MathSet);
		m_GlobalCmdHash["multiply"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_MathSet);
		m_GlobalCmdHash["divide"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_MathSet);
		m_GlobalCmdHash["mod"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_MathSet);
		m_GlobalCmdHash["vectoradd"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_VectorAdd);
		m_GlobalCmdHash["vectormultiply"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_VectorMultiply);
		m_GlobalCmdHash["vectorscale"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_VectorMultiply);
		m_GlobalCmdHash["vectorset"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_VectorSet);
		m_GlobalCmdHash["stradd"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_StrAdd);
		m_GlobalCmdHash["capvar"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_CapVar);
		m_GlobalCmdHash["volume"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Volume);
		m_GlobalCmdHash["usetrigger"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_UseTrigger);
		m_GlobalCmdHash["setmodelbody"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetModelBody);
		m_GlobalCmdHash["setmodel"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetModel);
		m_GlobalCmdHash["setsolid"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetSolid);
		m_GlobalCmdHash["setalive"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetAlive);
		m_GlobalCmdHash["setprop"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetProp);
		m_GlobalCmdHash["setfollow"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetFollow);
		m_GlobalCmdHash["setvar"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetVar);        //Exectuted at loadtime and runtime
		m_GlobalCmdHash["setvarg"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetVar);
		m_GlobalCmdHash["local"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetVar);
		m_GlobalCmdHash["token.add"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_TokenAdd);
		m_GlobalCmdHash["token.del"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_TokenDel);
		m_GlobalCmdHash["token.set"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_TokenSet); //MiB DEC2007a - edit an existing token in a string
		m_GlobalCmdHash["token.scramble"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_TokenScramble); //Thothie MAR2008a - randomize a token list
		m_GlobalCmdHash["repeatdelay"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_RepeatDelay);
		m_GlobalCmdHash["givehp"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_GiveHPMP);
		m_GlobalCmdHash["givemp"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_GiveHPMP);
		m_GlobalCmdHash["drainhp"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_DrainHP); //Thothie MAR2008b
		m_GlobalCmdHash["createnpc"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Create);
		m_GlobalCmdHash["createitem"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Create);
		m_GlobalCmdHash["registerrace"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_RegisterRace);
		m_GlobalCmdHash["registertitle"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_RegisterTitle);
		m_GlobalCmdHash["registerdefaults"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_RegisterDefaults);
		m_GlobalCmdHash["registereffect"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_RegisterEffect);
		m_GlobalCmdHash["registertexture"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_RegisterTexture);
		m_GlobalCmdHash["deleteme"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_DeleteEntity);
		m_GlobalCmdHash["deleteent"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_DeleteEntity);
		m_GlobalCmdHash["effect"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Effect);
		m_GlobalCmdHash["cleffect"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_ClEffect);
		m_GlobalCmdHash["setvelocity"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Velocity);
		m_GlobalCmdHash["addvelocity"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Velocity);
		m_GlobalCmdHash["setorigin"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Origin);
		m_GlobalCmdHash["addorigin"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Origin);
		m_GlobalCmdHash["clientevent"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_ClientEvent);
		m_GlobalCmdHash["removescript"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_RemoveScript);
		m_GlobalCmdHash["playrandomsound"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_PlaySound);
		m_GlobalCmdHash["playsound"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_PlaySound); //Thothie MAR2012_27 - These play on client now
		m_GlobalCmdHash["svplaysound"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_PlaySound); //Thothie MAR2012_27 - for those few that must be tracked server side
		m_GlobalCmdHash["svplayrandomsound"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_PlaySound); //Thothie MAR2012_27 - for those few that must be tracked server side
		m_GlobalCmdHash["sound.play3d"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SoundPlay3D);
		m_GlobalCmdHash["svsound.play3d"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SoundPlay3D); //Thothie APR2012_04 - same diff, but precaches for server
		m_GlobalCmdHash["sound.pm_play"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SoundPMPlay);
		m_GlobalCmdHash["sound.setvolume"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SoundSetVolume);
		m_GlobalCmdHash["applyeffect"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_ApplyEffect);
		m_GlobalCmdHash["storeentity"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_StoreEntity);
		m_GlobalCmdHash["precachefile"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_PrecacheFile);
		m_GlobalCmdHash["playermessage"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Message);
		m_GlobalCmdHash["rplayermessage"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Message); //Thothie - MAR2007
		m_GlobalCmdHash["gplayermessage"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Message); //Thothie - MAR2007
		m_GlobalCmdHash["bplayermessage"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Message); //Thothie - MAR2007
		m_GlobalCmdHash["yplayermessage"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Message); //Thothie - MAR2007
		m_GlobalCmdHash["dplayermessage"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Message); //Thothie - MAR2007
		m_GlobalCmdHash["consolemsg"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Message);
		m_GlobalCmdHash["infomsg"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_InfoMessage);
		m_GlobalCmdHash["return"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Return); //Thothie - AUG2013_27 - alias
		m_GlobalCmdHash["emitsound"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_EmitSound);
		m_GlobalCmdHash["companion"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Companion);
		m_GlobalCmdHash["helptip"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_HelpTip);
		m_GlobalCmdHash["quest"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Quest);
		m_GlobalCmdHash["setcallback"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetCallBack);
		m_GlobalCmdHash["drop_to_floor"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_DropToFloor);
		m_GlobalCmdHash["setwearpos"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetWearPos);
		m_GlobalCmdHash["setgaitspeed"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetGaitSpeed);
		m_GlobalCmdHash["setexpstat"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetExpStat);
		m_GlobalCmdHash["drainstamina"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_DrainStamina);
		m_GlobalCmdHash["getents"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_GetEnts); //Thothie - MAY2007a
		m_GlobalCmdHash["getplayer"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_GetPlayer); //Thothie - FEB2008a - pull a single player by slot#
		m_GlobalCmdHash["getplayers"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_GetPlayers); //Thothie - JUN2007a
		m_GlobalCmdHash["getplayersnb"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_GetPlayersNB); //Thothie - JUN2007a
		m_GlobalCmdHash["playmp3"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_PlayMP3); //Thothie - MAY2007a
		m_GlobalCmdHash["errormessage"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_ErrorMessage); //Thothie - JUN2007a
		m_GlobalCmdHash["popup"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_ErrorMessage); //Thothie - JUN2007a
		m_GlobalCmdHash["setenv"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetEnv);
		m_GlobalCmdHash["setcvar"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetCVar); //Thothie - JUN2007a
		m_GlobalCmdHash["respawn"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Respawn);
		m_GlobalCmdHash["npcmove"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_NpcMove);
		m_GlobalCmdHash["giveexp"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_GiveExp); //MiB JAN2010_18
		m_GlobalCmdHash["torandomspawn"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_ToRandomSpawn); //Thothie JAN2010_20
		m_GlobalCmdHash["scriptflags"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_ScriptFlags); //Thothie JAN2013_02
		m_GlobalCmdHash["array.add"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Array); //Mib JAN2010_27
		m_GlobalCmdHash["array.add_unique"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Array); //Thothie SEP2019_28
		m_GlobalCmdHash["array.create"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Array); //Mib JAN2010_27
		m_GlobalCmdHash["array.del"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Array); //Mib JAN2010_27
		m_GlobalCmdHash["array.copy"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Array); //Thothie NOV2016_09 pull array from remote entity
		m_GlobalCmdHash["array.clear"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Array); //Thothie OCT2016_23 - somehow we didn't get a commanddef for this
		m_GlobalCmdHash["array.erase"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Array); //Mib JAN2010_27
		m_GlobalCmdHash["array.set"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Array); //Mib JAN2010_27
		m_GlobalCmdHash["g_array.add"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Array); //Mib JUN2010_25
		m_GlobalCmdHash["g_array.add_unique"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Array); //Thothie SEP2019_28
		m_GlobalCmdHash["g_array.create"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Array); //Mib JUN2010_25
		m_GlobalCmdHash["g_array.del"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Array); //Mib JUN2010_25
		m_GlobalCmdHash["g_array.clear"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Array); //Thothie OCT2016_23 - somehow we didn't get a commanddef for this
		m_GlobalCmdHash["g_array.erase"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Array); //Mib JUN2010_25
		m_GlobalCmdHash["g_array.set"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Array); //Mib JUN2010_25
		m_GlobalCmdHash["callclitemevent"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_CallClItemEvent); //Mib FEB2010_03
		m_GlobalCmdHash["itemrestrict"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_ItemRestrict); //MiB FEB2010_13
		m_GlobalCmdHash["bleed"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_bleed); //Thothie DEC2014_13
		m_GlobalCmdHash["removeeffect"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_RemoveEffect); //MiB FEB2010_28
#if !TURN_OFF_ALERT
		m_GlobalCmdHash["conflictcheck"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_ConflictCheck); //Thothie JAN2013_08
#endif
		m_GlobalCmdHash["getplayersarray"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_GetPlayersArray); //Thothie MAR2010_16
		m_GlobalCmdHash["getitemarray"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_GetItemArray); //MiB MAY2010_23
		m_GlobalCmdHash["clearplayerhits"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_ClearPlayerHits); //MiB MAY2010_23
		m_GlobalCmdHash["darkenbloom"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_DarkenBloom); //MiB DEC2010_31
		m_GlobalCmdHash["setquality"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetQuality); //Thothie MAR2015_15 - somehow this did not get added here
		m_GlobalCmdHash["setquantity"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_setquantity); //Thothie MAR2015_15 - sets an item's quantity
		m_GlobalCmdHash["teleportdest"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_teleportdest); //Thothie OCT2015_18 - teleport to specific info_teleport_destination
		m_GlobalCmdHash["syncitem"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_syncitem); //Thothie OCT2016_04 - sync item from server to client
		m_GlobalCmdHash["displaydesc"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_syncitem); //Thothie OCT2016_04 - invoke item desc display on client
		//m_GlobalCmdHash["moditem"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_moditem); //Thothie OCT2016_18 Item Mods
		m_GlobalCmdHash["endgame"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_endgame); //Thothie OCT2016_26 - exit command to deal with the fact we can't send the "quit" command anymore

		m_GlobalCmdHash["localmenu.reset"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_LocalPanel); // MiB MAR2015_01 [LOCAL_PANEL] - Resets local menu
		m_GlobalCmdHash["localmenu.open"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_LocalPanel); // MiB MAR2015_01 [LOCAL_PANEL] - Open local menu
		m_GlobalCmdHash["localmenu.close"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_LocalPanel); // MiB MAR2015_01 [LOCAL_PANEL] - Close local menu
		m_GlobalCmdHash["registerlocal.menu"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_LocalPanel); // MiB MAR2015_01 [LOCAL_PANEL] - Register local menu
		m_GlobalCmdHash["registerlocal.button"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_LocalPanel); // MiB MAR2015_01 [LOCAL_PANEL] - Register local menu button
		m_GlobalCmdHash["registerlocal.paragraph"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_LocalPanel); // MiB MAR2015_01 [LOCAL_PANEL] - Register local menu paragraph
		m_GlobalCmdHash["registerlocal.image"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_LocalPanel); // MiB FEB2019_25 [LOCAL_PANEL] - Register local menu image
		m_GlobalCmdHash["resetglobals"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_resetglobals); // Thothie FEB2017_07 - reset globals
		m_GlobalCmdHash["breakloop"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_breakloop); // Thothie DEC2017_19 - breakloop cancels a calleventloop in progress
		m_GlobalCmdHash["resetloop"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_resetloop); // Thothie SEP2019_08 - resets the iteration to repeat a loop from 0

		//MiB MAR2019_28 Internal Script Debug
		m_GlobalCmdHash["dbg_all"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_DebugEntities);
		m_GlobalCmdHash["dbg_npcs"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_DebugEntities);
		m_GlobalCmdHash["dbg_target"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_DebugEntities);
		m_GlobalCmdHash["dbg_items"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_DebugEntities);
		m_GlobalCmdHash["dbg_hand_active"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_DebugEntities);
		m_GlobalCmdHash["dbg_hand_off"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_DebugEntities);
		m_GlobalCmdHash["dbg_players"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_DebugEntities);
		m_GlobalCmdHash["dbg_world"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_DebugEntities);
		m_GlobalCmdHash["dbg_gm"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_DebugEntities);
		m_GlobalCmdHash["dbg_index"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_DebugEntities);
		m_GlobalCmdHash["dbg_scriptname"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_DebugEntities);

		m_GlobalCmdHash["markdmg"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_MarkDmg); // MiB MAY2019_10

		m_GlobalCmdHash["setatkspeed"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetAtkSpeed); // MiB AUG2019_28

		m_GlobalCmdHash["hashmap.create"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_HashMap);
		m_GlobalCmdHash["hashmap.set"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_HashMap);
		m_GlobalCmdHash["hashmap.del"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_HashMap);
		m_GlobalCmdHash["hashmap.erase"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_HashMap);
		m_GlobalCmdHash["hashmap.clear"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_HashMap);
		m_GlobalCmdHash["hashmap.copy"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_HashMap);
		m_GlobalCmdHash["g_hashmap.create"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_HashMap);
		m_GlobalCmdHash["g_hashmap.set"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_HashMap);
		m_GlobalCmdHash["g_hashmap.del"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_HashMap);
		m_GlobalCmdHash["g_hashmap.erase"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_HashMap);
		m_GlobalCmdHash["g_hashmap.clear"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_HashMap);
		m_GlobalCmdHash["g_hashmap.copy"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_HashMap);

		m_GlobalCmdHash["set.create"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Set);
		m_GlobalCmdHash["set.add"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Set);
		m_GlobalCmdHash["set.del"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Set);
		m_GlobalCmdHash["set.erase"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Set);
		m_GlobalCmdHash["set.clear"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Set);
		m_GlobalCmdHash["set.copy"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Set);
		m_GlobalCmdHash["g_set.create"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Set);
		m_GlobalCmdHash["g_set.add"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Set);
		m_GlobalCmdHash["g_set.del"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Set);
		m_GlobalCmdHash["g_set.erase"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Set);
		m_GlobalCmdHash["g_set.clear"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Set);
		m_GlobalCmdHash["g_set.copy"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_Set);
		
		m_GlobalCmdHash["forcesend"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetEntForceSend);
		m_GlobalCmdHash["nosend"] = scriptcmdscpp_cmdfunc_t(&CScript::ScriptCmd_SetEntNoSend);
	}
}

//MIB MAR2019_31 Internal Script Debug
#ifdef VALVE_DLL
#define DBGALL          (1 << 0)
#define DBGTARGET       (1 << 1)
#define DBGITEMS        (1 << 2)
#define DBGHANDACTIVE   (1 << 3)
#define DBGHANDOFF      (1 << 4)
#define DBGPLAYERS      (1 << 5)
#define DBGWORLD        (1 << 6)
#define DBGGM           (1 << 7)
#define DBGINDEX        (1 << 8)
#define DBGSCRIPTNAME   (1 << 9)
#define DBGNPCS         (1 << 10)

struct SDebugInfo
{
	// Info for finding entities
	int                                 mTargetType;
	int                                 mTimesLooked;
	int                                 mStartIndex;
	msstring                            msScriptName;
	CBaseEntity *                       mpFoundEntity;

	// Info for printing variables
	bool                                mbIsExists;
	bool                                mbIsExplicitVar;
	msstring                            msVarName;
	int                                 mNumProps;
	msstringlist                        mSubParams;

	// Info for printing arrays
	bool                                mbGlobalArray;
	bool                                mbArray;
	bool                                mbDumpArray;
	msstring                            msArrayName;
	int                                 mArrayIndex;

	SDebugInfo()
		: mTargetType(0)
		, mTimesLooked(0)
		, mStartIndex(0)
		, mpFoundEntity(NULL)
		, mbIsExists(false)
		, mbIsExplicitVar(false)
		, mNumProps(0)
		, mbGlobalArray(false)
		, mbArray(false)
		, mbDumpArray(false)
		, mArrayIndex(0)
	{}
};

bool GetNextDebugEntity(
	CBasePlayer *                         pCaller
	, SDebugInfo &                          rDebugInfo
	)
{
	++rDebugInfo.mTimesLooked;
	rDebugInfo.mpFoundEntity = NULL;
	if ( rDebugInfo.mTargetType == DBGTARGET )
	{
		if ( rDebugInfo.mTimesLooked == 1 )
		{
			rDebugInfo.mpFoundEntity = pCaller->RetrieveEntity( ENT_TARGET );
			if ( !rDebugInfo.mpFoundEntity )
			{
				ClientPrint( pCaller->pev, at_console, "No player target, using insphere...\n" );

				CBaseEntity * vList[30];
				int vNumFound = UTIL_MonstersInSphere( vList, 30, pCaller->pev->origin, 128 );
				for(int i = 0; i < vNumFound; i++ )
				{
					rDebugInfo.mpFoundEntity = vList[i];
					if (  pCaller->entindex() == rDebugInfo.mpFoundEntity->entindex()
						|| !rDebugInfo.mpFoundEntity->IsAlive()
						)
					{
						rDebugInfo.mpFoundEntity = NULL;
					}
					else
					{
						break;
					}
				}
				if ( !rDebugInfo.mpFoundEntity )
				{
					ClientPrint( pCaller->pev, at_console, "No target in range?\n" );
				}
			}
		}
	}
	else
	if ( rDebugInfo.mTargetType == DBGWORLD )
	{
		if ( rDebugInfo.mTimesLooked == 1 )
		{
			rDebugInfo.mpFoundEntity = CBaseEntity::Instance( INDEXENT(0) );
		}
	}
	else
	if ( rDebugInfo.mTargetType == DBGGM )
	{
		if ( rDebugInfo.mTimesLooked == 1 )
		{
			rDebugInfo.mpFoundEntity = UTIL_FindEntityByString(NULL, "netname", msstring("¯") + "game_master");
		}
	}
	else
	if ( rDebugInfo.mTargetType == DBGHANDACTIVE )
	{
		if ( rDebugInfo.mTimesLooked == 1 )
		{
			rDebugInfo.mpFoundEntity = pCaller->ActiveItem();
		}
	}
	else
	if ( rDebugInfo.mTargetType == DBGHANDOFF )
	{
		if ( rDebugInfo.mTimesLooked == 1 )
		{
			if ( pCaller->m_CurrentHand == 0 )
			{
				rDebugInfo.mpFoundEntity = pCaller->Hand(1);
			}
			else
			{
				rDebugInfo.mpFoundEntity = pCaller->Hand(0);
			}
		}
	}
	else
	if ( rDebugInfo.mTargetType == DBGINDEX && rDebugInfo.mTimesLooked != 1 )
	{
		// Skip
	}
	else
	{
		while ( !rDebugInfo.mpFoundEntity && rDebugInfo.mStartIndex < gpGlobals->maxEntities )
		{
			edict_t *                   pEdict = g_engfuncs.pfnPEntityOfEntIndex( rDebugInfo.mStartIndex );
			if (  pEdict
				&& pEdict->pvPrivateData
				&& !pEdict->free
				)
			{
				IScripted *             pScripted;
				rDebugInfo.mpFoundEntity = CBaseEntity::Instance( pEdict );
				if ( !rDebugInfo.mpFoundEntity )
				{
					// Skip
				}
				else
				if ( !( pScripted = rDebugInfo.mpFoundEntity->GetScripted() ) || pScripted->m_Scripts.size() == 0 )
				{
					rDebugInfo.mpFoundEntity = NULL;
				}
				else
				if ( rDebugInfo.mTargetType == DBGNPCS          && ( !rDebugInfo.mpFoundEntity->IsMSMonster() || rDebugInfo.mpFoundEntity->IsPlayer() ) )
				{
					rDebugInfo.mpFoundEntity = NULL;
				}
				else
				if ( rDebugInfo.mTargetType == DBGPLAYERS       && !rDebugInfo.mpFoundEntity->IsPlayer() )
				{
					rDebugInfo.mpFoundEntity = NULL;
				}
				else
				if ( rDebugInfo.mTargetType == DBGITEMS         && !rDebugInfo.mpFoundEntity->IsMSItem() )
				{
					rDebugInfo.mpFoundEntity = NULL;
				}
				else
				if ( rDebugInfo.mTargetType == DBGSCRIPTNAME )
				{
					if (  !pScripted->m_Scripts.size()
						|| rDebugInfo.msScriptName != pScripted->m_Scripts[0]->m.ScriptFile
						)
					{
						rDebugInfo.mpFoundEntity = NULL;
					}
				}
			}
			if ( rDebugInfo.mTargetType == DBGINDEX ) break; // Stop even if we didn't find one
			++rDebugInfo.mStartIndex;
		}
	}

	if ( rDebugInfo.mpFoundEntity )
	{
		return true;
	}
	else
	{
		return false;
	}
}
#endif

// Get debug information for the scripted entities
// dbg_all          -   All entities
// dbg_npcs         -   NPCs (Rather, Non-items, Non-players)
// dbg_target       -   Current target
// dbg_items        -   All items
// dbg_hand_active  -   Active item
// dbg_hand_off     -   Offhand item
// dbg_players      -   Players
// dbg_world        -   World script
// dbg_gm           -   Game master
// dbg_index        -   Entity at the given index
// dbg_scriptname   -   Scripts with the given name
//
// Parameters
// <Command> <Player>                               - Prints "exists" for each matching entity that exists
// <Command> <Player> [var] <name> [prop] [prop]    - Prints the variable's value, using the sub-props as provided.
//                                                      If $get(this,name) is an entity, $get( $get(this,name), prop[, prop] )
//                                                      If $get($get(this,name),prop) is an entity, $get($get( $get(this,name), prop), prop] )
// <Command> <Player> [g]array <name> <index>       - Prints the element of the [global] array with the given name at the given index
// <Command> <Player> d[g]array <name>              - Dumps the contents of the [global] array with the given name to console
//
bool DoDebugEntities(
	CBasePlayer *                         pCallerPlayer
	, msstring                              vsCmdName
	, msstringlist                          vFilteredParams
	, const char *                          pszPrepend
	);
bool CScript::ScriptCmd_DebugEntities(
	SCRIPT_EVENT &                        Event
	, scriptcmd_t &                         Cmd
	, msstringlist &                        Params
	)
{
	if (!Params.size())
	{
		ERROR_MISSING_PARMS;
		return true;
	}

	msstringlist vFilteredParams;
	CBaseEntity * pCallerEntity = RetrieveEntity(Params[0]);
	CBasePlayer * pCallerPlayer = pCallerEntity && pCallerEntity->IsPlayer() ? (CBasePlayer *)pCallerEntity : NULL;
	if (!pCallerPlayer)
	{
		Print("Player must be first parameter\n");
		return true;
	}

	for(int i = 0; i < Params.size() - 1; i++ )
	{
		if (Params[i + 1].starts_with("PARAM"))
			break;

		vFilteredParams.add(Params[i + 1]);
	}

	//
	//  Doing this is a sub-function so I can use recursion
	//  to deal with some awkward cases while still keeping
	//  the calling entity's debug output
	//
	return DoDebugEntities(pCallerPlayer, Cmd.Name(), vFilteredParams, NULL);
}

#define ERROR_MISSING_PARMS_HACK MSErrorConsoleText( "ExecuteScriptCmd", UTIL_VarArgs("Script: %s - not enough parameters!\n", vsCmdName.c_str()) )
bool DoDebugEntities(
	CBasePlayer *                         pCallerPlayer
	, msstring                              vsCmdName
	, msstringlist                          vFilteredParams
	, const char *                          pszPrepend
	)
{
	bool                                bResult = true;
#ifdef VALVE_DLL
	SDebugInfo                          vDebugInfo;
	bool                                bIsSub          = (pszPrepend != NULL);
	msstring                            vsMsgTemplate   = "";
	size_t                              vParamIndex     = 0;
	static const char *                 ksPrintEvent    = "ext_debug_que";

	if      ( vsCmdName == "dbg_all"            ) vDebugInfo.mTargetType = DBGALL           ;
	else if ( vsCmdName == "dbg_npcs"           ) vDebugInfo.mTargetType = DBGNPCS          ;
	else if ( vsCmdName == "dbg_target"         ) vDebugInfo.mTargetType = DBGTARGET        ;
	else if ( vsCmdName == "dbg_items"          ) vDebugInfo.mTargetType = DBGITEMS         ;
	else if ( vsCmdName == "dbg_hand_active"    ) vDebugInfo.mTargetType = DBGHANDACTIVE    ;
	else if ( vsCmdName == "dbg_hand_off"       ) vDebugInfo.mTargetType = DBGHANDOFF       ;
	else if ( vsCmdName == "dbg_players"        ) vDebugInfo.mTargetType = DBGPLAYERS       ;
	else if ( vsCmdName == "dbg_world"          ) vDebugInfo.mTargetType = DBGWORLD         ;
	else if ( vsCmdName == "dbg_gm"             ) vDebugInfo.mTargetType = DBGGM            ;
	else if ( vsCmdName == "dbg_index"          )
	{
		vDebugInfo.mTargetType = DBGINDEX;
		if ( vFilteredParams.size() <= vParamIndex )
		{
			ERROR_MISSING_PARMS_HACK;
			return bResult;
		}
		vDebugInfo.mStartIndex = atoi( vFilteredParams[vParamIndex++] );
	}
	else if ( vsCmdName == "dbg_scriptname" )
	{
		vDebugInfo.mTargetType = DBGSCRIPTNAME;
		if ( vFilteredParams.size() <= vParamIndex )
		{
			ERROR_MISSING_PARMS_HACK;
			return bResult;
		}
		vDebugInfo.msScriptName = vFilteredParams[vParamIndex++];
	}
	else
	{
		Print( "You didn't add %s to the mTargetType sets :)\n", vsCmdName.c_str() );
		return bResult;
	}

	if ( vFilteredParams.size() <= vParamIndex )
	{
		vDebugInfo.mbIsExists = true;
		vsMsgTemplate += "exists";
	}
	else
	if (  vFilteredParams[vParamIndex] == "array"  || vFilteredParams[vParamIndex] == "garray"
		|| vFilteredParams[vParamIndex] == "darray" || vFilteredParams[vParamIndex] == "dgarray"
		)
	{
		vDebugInfo.mbArray              = ( vFilteredParams[vParamIndex] == "array"  || vFilteredParams[vParamIndex] == "darray"  );
		vDebugInfo.mbGlobalArray        = !vDebugInfo.mbArray;
		vDebugInfo.mbDumpArray          = ( vFilteredParams[vParamIndex] == "darray" || vFilteredParams[vParamIndex] == "dgarray" );
		vParamIndex++;

		if ( vFilteredParams.size() < ( vParamIndex + ( vDebugInfo.mbDumpArray ? 1 : 2 )) )
		{
			ERROR_MISSING_PARMS_HACK;
			return bResult;
		}

		vDebugInfo.msVarName            = vFilteredParams[vParamIndex++];
		if ( vDebugInfo.mbDumpArray )
		{
			vsMsgTemplate += "Dumping ";
			vsMsgTemplate += vDebugInfo.mbArray ? "Array " : "Global Array ";
			vsMsgTemplate += vDebugInfo.msVarName;
		}
		else
		{
			vDebugInfo.mArrayIndex      = atoi( vFilteredParams[vParamIndex++] );

			vsMsgTemplate += vDebugInfo.mbArray ? "Array " : "g_Array ";
			vsMsgTemplate += vDebugInfo.msVarName;
			vsMsgTemplate += "#";
			vsMsgTemplate += vDebugInfo.mArrayIndex;
			vsMsgTemplate += " is %s";
		}
	}
	else
	{
		if ( vFilteredParams[vParamIndex] == "var" )
		{
			vDebugInfo.mbIsExplicitVar = true;
			++vParamIndex; // Explicit, just ignore it
			if ( vFilteredParams.size() <= vParamIndex )
			{
				// Can't have JUST var, though
				ERROR_MISSING_PARMS_HACK;
				return bResult;
			}
		}

		vDebugInfo.msVarName = vFilteredParams[vParamIndex++];
		while ( vParamIndex < vFilteredParams.size() ) vDebugInfo.mSubParams.add( vFilteredParams[vParamIndex++] );
		vsMsgTemplate += "%s";
	}

	while( GetNextDebugEntity(pCallerPlayer, vDebugInfo) )
	{

		msstringlist                    vFakeParams;
		msstring                        vsNameProp = vDebugInfo.mpFoundEntity->IsMSItem() ? "itemname" : "name";
		msstring                        vsName = vDebugInfo.mpFoundEntity->GetProp(NULL,vsNameProp,vFakeParams);
		char                            pszMsg[512];
		int                             vPrependLength;
		msscriptarray *                 pScriptArray = NULL;
		pszMsg[0] = 0;

		if ( bIsSub )
		{
			vPrependLength = strlen( pszPrepend );
			_snprintf(pszMsg, sizeof(pszMsg), "%s", pszPrepend);
		}
		else
		{
			msstring                    vsTemp;
			vsTemp += vsName;
			vsTemp += "[";
			vsTemp += vDebugInfo.mpFoundEntity->entindex();
			vsTemp += "]: ";
			vPrependLength = vsTemp.len();
			_snprintf(pszMsg, sizeof(pszMsg), "%s", vsTemp.c_str());
		}

		if ( vDebugInfo.mbArray || vDebugInfo.mbGlobalArray )
		{
			bool bExisted;
			msscriptarray *pScriptArray = GetScriptedArrayFromHashMap(vDebugInfo.mbArray ? vDebugInfo.mpFoundEntity->scriptedArrays : CScript::GlobalScriptArrays, vDebugInfo.msVarName, false, &bExisted);
			if ( vDebugInfo.mbDumpArray )
			{
				if ( bExisted )
				{
					_snprintf(pszMsg+vPrependLength, sizeof(pszMsg), vsMsgTemplate.c_str(), pScriptArray->size() > (size_t)vDebugInfo.mArrayIndex ? (*pScriptArray)[vDebugInfo.mArrayIndex].c_str() : "[BAD_INDEX]");
				}
				else
				{
					_snprintf(pszMsg+vPrependLength, sizeof(pszMsg), vsMsgTemplate.c_str(), "[ERROR_NO_ARRAY]");
				}
			}
			else
			{
				_snprintf(pszMsg+vPrependLength, sizeof(pszMsg), "%s", vsMsgTemplate.c_str());
			}
		}
		else
		if ( vDebugInfo.mbIsExists )
		{
			_snprintf(pszMsg+vPrependLength, sizeof(pszMsg), "%s", vsMsgTemplate.c_str());
		}
		else
		{
			CBaseEntity *               pSubEntity = vDebugInfo.mpFoundEntity->RetrieveEntity( vDebugInfo.msVarName );
			msstring                    vsOutAppend = vDebugInfo.msVarName;
			msstringlist                vGetPropParams;
			msstring                    vsPropRslt;
			if ( !vDebugInfo.mbIsExplicitVar && !pSubEntity && vDebugInfo.mSubParams.size() )
			{
				vGetPropParams.clear();
				vGetPropParams.add( EntToString(vDebugInfo.mpFoundEntity) );
				vGetPropParams.add( vDebugInfo.msVarName );

				for(int i = 0; i < 2; i++)
				{
					if ( i )
					{
						if ( vDebugInfo.mSubParams.size() < 2 ) continue;
						vGetPropParams.add( vDebugInfo.mSubParams[0] );
					}
					vsPropRslt = vDebugInfo.mpFoundEntity->GetProp(NULL,vDebugInfo.msVarName,vGetPropParams);
					pSubEntity = pCallerPlayer->RetrieveEntity( vsPropRslt );
					if ( pSubEntity )
					{
						if ( i )
						{
							vsOutAppend += "[";
							vsOutAppend += vDebugInfo.mSubParams[0];
							vsOutAppend += "]";
							vDebugInfo.mSubParams.erase(0);
						}
						vsOutAppend += "(";
						vsOutAppend += EntToString(pSubEntity);
						vsOutAppend += ")";
						break;
					}
				}
			}
			if ( pSubEntity )
			{
				if ( !vDebugInfo.mbIsExplicitVar && vDebugInfo.mSubParams.size() )
				{
					// Awkward and inefficient, but no other way if it's an entity param
					// with subparams like array
					msstringlist        vSubParams;
					vSubParams.add( msstring() + pSubEntity->entindex() );

					for( int i = 0; i < vDebugInfo.mSubParams.size(); i++ )
						vSubParams.add( vDebugInfo.mSubParams[i] );

					vsOutAppend += "->";
					_snprintf(pszMsg+vPrependLength, sizeof(pszMsg), vsMsgTemplate.c_str(), vsOutAppend.c_str());
					bResult = DoDebugEntities(pCallerPlayer, "dbg_index", vSubParams, pszMsg) && bResult;
					continue;
				}
				if ( vDebugInfo.mSubParams.size() )
				{
					vsOutAppend += "->";
					vDebugInfo.msVarName = vDebugInfo.mSubParams[0];

					vGetPropParams.clear();
					vGetPropParams.add( EntToString(pSubEntity) );
					vGetPropParams.add( vDebugInfo.msVarName );
					if ( vDebugInfo.mSubParams.size() > 1 ) vGetPropParams.add( vDebugInfo.mSubParams[1] );
					vsOutAppend += pSubEntity->GetProp(NULL,vDebugInfo.msVarName,vGetPropParams);
				}
				else
				{
					vsOutAppend += "=";
					vsOutAppend += EntToString( pSubEntity );
				}
			}
			else
			{
				vGetPropParams.clear();
				vGetPropParams.add( EntToString(vDebugInfo.mpFoundEntity) );
				vGetPropParams.add( vDebugInfo.msVarName );
				if ( vDebugInfo.mSubParams.size() )
				{
					if ( bIsSub )
					{
						vsOutAppend += "[";
						vsOutAppend += vDebugInfo.mSubParams[0];
						vsOutAppend += "]";
					}
					else
					{
						vsOutAppend += "->";
						vsOutAppend += vDebugInfo.mSubParams[0];
					}

					vGetPropParams.add( vDebugInfo.mSubParams[0] );
					if ( vDebugInfo.mSubParams.size() > 1 )
					{
						if ( !bIsSub )
						{
							vsOutAppend += "[";
							vsOutAppend += vDebugInfo.mSubParams[1];
							vsOutAppend += "]";
						}

						vGetPropParams.add( vDebugInfo.mSubParams[1] );
						if ( vDebugInfo.mSubParams.size() > 2 ) vGetPropParams.add( vDebugInfo.mSubParams[2] );
					}
				}
				vsOutAppend += "=";

				msstring                vsPropRslt = vDebugInfo.mpFoundEntity->GetProp(NULL,vDebugInfo.msVarName,vGetPropParams);
				if (   vDebugInfo.mSubParams.size() > 2
					|| (vDebugInfo.mSubParams.size() > 1 && bIsSub)
					)
				{
					vsOutAppend += EntToString( vDebugInfo.mpFoundEntity );
					vsOutAppend += ",";
				}
				vsOutAppend += vsPropRslt;
			}

			_snprintf(pszMsg+vPrependLength, sizeof(pszMsg), vsMsgTemplate.c_str(), vsOutAppend.c_str());
		}

		msstringlist                    vPrintParams;
		vPrintParams.add( pszMsg );
		pCallerPlayer->CallScriptEvent( ksPrintEvent, &vPrintParams );

		if ( vDebugInfo.mbDumpArray && pScriptArray )
		{
			for( int i = 0; i < pScriptArray->size(); i++ )
			{
				vPrintParams.clear();
				vPrintParams.add( msstring( "#" )
					+ i
					+ " "
					+ (*pScriptArray)[i]
					);
				pCallerPlayer->CallScriptEvent( ksPrintEvent, &vPrintParams );
				Print("DEBUG: DumpArray - %i\n",i);
			}
		}
	}
#endif
	return bResult;
}
#undef ERROR_MISSING_PARMS_HACK

//MiB FEB2019_23 - item table getter
//$get_item_table(<item_name>,<prop>)
//- Pull item properties direct from the master item table.
msstring CScript::ScriptGetter_GetItemTable(msstring& FullName, msstring& ParserName, msstringlist& Params)
{
	if (Params.size() > 1)
	{
		CGenericItem *pItem = CGenericItemMgr::GetGlobalGenericItemByName(Params[0], true);
		if (pItem)
		{
			msstring vProp = Params[1];
			msstringlist vSubParams;
			for(int i = 0; i < Params.size() - 2; i++)
			{
				vSubParams.add(Params[i + 2]);
			}
			return pItem->GetProp(NULL, vProp, vSubParams);
		}
	}
	return FullName;
}

//Thothie DEC2017_19 - breakloop - cancels a calleventloop in progress by setting setvard MSC_BREAK_LOOP 1
//calleventloop resets the var and exits on detection
//scope: shared, priority: very low
bool CScript::ScriptCmd_breakloop(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	SetVar("MSC_BREAK_LOOP", "1");
	return true;
}

//Thothie SEP2019_08 - resetloop - sets the interation to repeat all or part of a loop
//resetloop [desired_iteration-1]
//scope: shared, priority: very low
bool CScript::ScriptCmd_resetloop(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	msstring reset_loop_stage = "-1";
	if (Params.size() > 0) reset_loop_stage = Params[0];
	//m.m_Iteration = reset_loop_stage; //not gonna work
	SetVar("MSC_RESET_LOOP", reset_loop_stage);
	return true;
}


//resetglobals - clears msglobals not intended to propigate between maps
//Thothie FEB2017_07 - just couldn't find a dependable place to do this, so we're making it part of the GM's map change sequence
bool CScript::ScriptCmd_resetglobals(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	MSGlobals::map_addparams = "";
	MSGlobals::map_flags = "";
	MSGlobals::MapTitle = "";
	MSGlobals::MapDesc = "";
	MSGlobals::MapWeather = "clear;clear;clear";
	MSGlobals::HPWarn = 0;
	MSGlobals::SpawnLimit = 0;
	MSGlobals::maxviewdistance = 0;
	Print("DEBUG: MSGlobals Reset\n");
	return true;
}

//endgame - quits server or client (depending on which end it is executed)
//Thothie OCT2016_26 - exit command to deal with the fact we can't send the "quit" command anymore
bool CScript::ScriptCmd_endgame(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	exit(0);
	return true;
}

//Param1 = Property name
//Param2 = Extra data
msstring_ref CBaseEntity::GetProp(CBaseEntity *pTarget, msstring &FullParams, msstringlist &Params)
{
	if (!pTarget) pTarget = this;
	CGenericItem *pItem = pTarget->IsMSItem() ? (CGenericItem *)pTarget : NULL;
	CMSMonster *pMonster = pTarget->IsMSMonster() ? (CMSMonster *)pTarget : NULL;
	CBasePlayer *pPlayer = pTarget->IsPlayer() ? (CBasePlayer *)pTarget : NULL;
	IScripted *pScripted = pTarget->GetScripted();

	bool fSuccess = false;
	int Stat = -1;
	static msstring Return;
	msstring &Prop = FullParams;

	if (Prop == "name")				return pTarget->DisplayName();
	else if (Prop == "id")				return EntToString(pTarget);
	else if (Prop == "atkspeed")
	{
#ifdef VALVE_DLL
		RETURN_FLOAT( pPlayer ? pPlayer->m_AnimSpeedAdj : 1 );
#else
		RETURN_FLOAT(player.m_AnimSpeedAdj);
#endif
	}
	/* thothie - this failed, I *guess* because it's trying to pull a var for the player
	//- instead made a few new $get comands in script.cpp
	else if( Prop == "lastmap" )
	{
	char last_map[32];
	strcpy( last_map, pPlayer->m_cEnterMap );
	ALERT( at_aiconsole, "String1(%s) String2(%s) String3(%s) \n", pPlayer->m_cEnterMap, pPlayer->m_OldTransition, last_map );
	return last_map; //even this returns 0 //pPlayer->m_cEnterMap; //returned 0
	}*/
#ifdef VALVE_DLL
	//Client can't use entity.index.  Only player.index (handled later under player)
	else if( Prop == "index" ) RETURN_INT( pTarget->entindex() )
#endif
	else if (Prop == "exists")			fSuccess = true;
	else if (Prop == "alive" || Prop == "isalive")			fSuccess = pTarget->IsAlive() ? true : false;
	else if (Prop == "hp")				RETURN_FLOAT(pTarget->pev->health)
#ifdef VALVE_DLL
	else if( Prop == "xp" || Prop == "skilllevel" )
	{
		//Thothie JUN2008a - return monsters XP value
		//NOV2014_21 - moving to top, as this is one we'll have to use at mob spawn repeatedly
		RETURN_FLOAT( pMonster->m_SkillLevel )
	}
	else if ( Prop == "blood" ) //greatguys1 August2018
	{
		msstring blood;
		if ( pTarget->BloodColor() == BLOOD_COLOR_RED )
		{
			blood.append("red");
		}
		else if ( pTarget->BloodColor() == BLOOD_COLOR_GREEN )
		{
			blood.append("green");
		}
		else if ( pTarget->BloodColor() == BLOOD_COLOR_YELLOW )
		{
			blood.append("yellow");
		}
		else
		{
			blood.append("none");
		}

		return blood.c_str();
	}
	else if( Prop == "renderprops" ) //Thothie MAR2015_17)
	{
		msstring renderprops;
		renderprops.append(FloatToString(pTarget->pev->scale));
		renderprops.append(";");
		renderprops.append(IntToString(pTarget->pev->rendermode));
		renderprops.append(";");
		renderprops.append(IntToString(pTarget->pev->renderamt));
		renderprops.append(";");
		renderprops.append(IntToString(pTarget->pev->body));
		renderprops.append(";");
		renderprops.append(IntToString(pTarget->pev->skin));
		return renderprops.c_str();
	}
#endif
	else if (Prop == "scriptname")
	{
		//Thothie DEC2008a - return the full scriptname, with path
		msstring msScriptNameReturn = pScripted->m_Scripts.size() ? pScripted->m_Scripts[0]->m.ScriptFile.c_str() : "0";
		return msScriptNameReturn.c_str();
		//MiB's attempt:
		//static msstring Return = (msstring_ref) pTarget->ScriptFName;
		//return Return.thru_substr( SCRIPT_EXT );
	}
	else if (Prop == "itemname")
	{
		//Thothie DEC2008a - return the truncated script name (mostly for items)
		msstring msScriptNameReturn = pScripted->m_Scripts.size() ? pScripted->m_Scripts[0]->m.ScriptFile.c_str() : "0";
		bool found_last_slash = false;
		int last_slash = msScriptNameReturn.len();
		while (!found_last_slash)
		{
			if (msScriptNameReturn[last_slash] == 47 || last_slash == 0)
			{
				found_last_slash = true;
			}
			else
			{
				last_slash = last_slash - 1;
			}
		}
		//thoth_return_scriptname = thoth_return_scriptname.findchar_str("/",0);
		msScriptNameReturn = msScriptNameReturn.substr(last_slash + 1);
		//thoth_return_scriptname = thoth_return_scriptname.substr( thoth_return_scriptname.len() - ( thoth_return_scriptname.len() -1 ) );
		return msScriptNameReturn.c_str();
	}
	else if (Prop == "gravity")		RETURN_FLOAT(pTarget->pev->gravity)
	else if (Prop == "height")			RETURN_FLOAT(pTarget->pev->maxs.z - pTarget->pev->mins.z)
	else if (Prop == "speed")			RETURN_FLOAT(pTarget->pev->velocity.Length())
	else if (Prop == "speed2D")		RETURN_FLOAT(pTarget->pev->velocity.Length2D())
	else if (Prop == "forwardspeed")
	{
		Vector vForward;
		EngineFunc::MakeVectors(pTarget->pev->v_angle, vForward, NULL, NULL);
		RETURN_FLOAT(DotProduct(pTarget->pev->velocity, vForward));
	}
	else if (Prop == "absmin") //Thothie DEC2014_12
	{
		return VecToString(pTarget->pev->absmin);
	}
	else if (Prop == "absmax") //Thothie DEC2014_12
	{
		return VecToString(pTarget->pev->absmax);
	}
	else if (Prop == "ducking")		fSuccess = FBitSet(pTarget->pev->flags, FL_DUCKING) ? true : false;
	else if (Prop == "onground")		fSuccess = FBitSet(pTarget->pev->flags, FL_ONGROUND) ? true : false;
#ifdef VALVE_DLL
	else if( Prop == "canattack" ) //Thothie AUG2007a
	{
		if ( pTarget->IsPlayer() )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pTarget;
			fSuccess = FBitSet(pPlayer->m_StatusFlags, PLAYER_MOVE_NOATTACK) ? false : true;
		}
	}
	else if( Prop == "canmove" ) //Thothie AUG2007a
	{
		if ( pTarget->IsPlayer() )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pTarget;
			fSuccess = FBitSet(pPlayer->m_StatusFlags, PLAYER_MOVE_NOMOVE) ? false : true;
		}
	}
	else if( Prop == "canjump" ) //Thothie AUG2007a
	{
		if ( pTarget->IsPlayer() )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pTarget;
			fSuccess = FBitSet(pPlayer->m_StatusFlags, PLAYER_MOVE_NOJUMP) ? false : true;
		}
	}
	else if( Prop == "canrun" ) //Thothie AUG2007a
	{
		if ( pTarget->IsPlayer() )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pTarget;
			fSuccess = FBitSet(pPlayer->m_StatusFlags, PLAYER_MOVE_NORUN) ? false : true;
		}
	}
	else if( Prop == "canduck" ) //Thothie AUG2007a
	{
		if ( pTarget->IsPlayer() )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pTarget;
			fSuccess = FBitSet(pPlayer->m_StatusFlags, PLAYER_MOVE_NODUCK) ? false : true;
		}
	}
#endif
	/*
	//MIB JUL2010_31 - debuggary
	else if ( Prop == "deadflag" )
	{
	RETURN_INT(pTarget->pev->deadflag)
	}
	*/
	else if (Prop == "sitting") //Thothie JAN2010_09 - spariments
	{
		if (pTarget->IsPlayer())
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pTarget;
			fSuccess = FBitSet(pPlayer->m_StatusFlags, PLAYER_MOVE_SITTING) ? true : false;
		}
	}
	else if (Prop == "inwater")		fSuccess = FBitSet(pTarget->pev->flags, FL_INWATER) ? true : false;
	else if (Prop == "value")
	{
		//Thothie FEB2008a - return item values
		if (pItem && pItem->m_Value)
		{
			int iRealCost = int(pItem->m_Value);
			RETURN_INT(iRealCost)
		}
		else return "unset";
	}
	else if (Prop == "invincible")
	{
		//Thothie AUG2007a (previous version not working)
		//fSuccess = FBitSet(pTarget->pev->flags,FL_GODMODE) ? true : false;
		//(still not workign) :/
		//Thothie FEB2015_18 - yet another attempt:
		RETURN_INT((pTarget->pev->flags & FL_GODMODE) ? 1 : 0);
	}
	else if (Prop == "waterlevel")		RETURN_INT(pTarget->pev->waterlevel)
	else if (Prop == "anim.current_frame")		RETURN_FLOAT(pTarget->pev->frame)
	else if (Prop == "anim.max_frames")		RETURN_FLOAT(255.0f)
	//else if( Prop == "anim.lastset" ) RETURN_FLOAT ( pTarget->pev->animtime ) //Thothie - JUN2007b - useless, just keeps increasing regardless of anims
	else if (Prop == "anim.index")  RETURN_INT(pTarget->pev->sequence)  //Thothie - JUN2007b
	else if (Prop == "anim.name")
	{
		//Thothie - JUN2007b
		//return animation sequence name
		//- meh, fail, just going to set animation indexes in the scripts for now
		//return LookupSequenceName(pTarget->pev->model,pTarget->pev->sequence);
	}
	else if (Prop.starts_with("origin"))			RETURN_POSITION("origin", pTarget->pev->origin)
	else if (Prop.starts_with("origin_center"))	RETURN_POSITION("origin_center", pTarget->Center())
	else if (Prop == "dist" || Prop == "dist2D" || Prop == "range" || Prop == "range2D")
	{
		//MIB JAN2010_20 - range check take model widths into account
		float Dist;
		CMSMonster *pMonsterMe = IsMSMonster() ? (CMSMonster *)this : NULL;
		if (Prop == "range" || Prop == "dist")
		{
			if (pMonster && pMonsterMe)
				Dist = (pTarget->pev->origin - pev->origin).Length() - ((pMonster->m_Width + pMonsterMe->m_Width) / 2);
			else
				Dist = (pTarget->pev->origin - pev->origin).Length();
		}
		else
			Dist = (pTarget->pev->origin - pev->origin).Length2D();

		RETURN_FLOAT(Dist)
			//original
			//float Dist = (Prop == "range" || Prop == "dist" ) ? (pTarget->pev->origin - pev->origin).Length() : (pTarget->pev->origin - pev->origin).Length2D();
			//RETURN_FLOAT( Dist )
	}
	else if (Prop.starts_with("eyepos"))	RETURN_POSITION("eyepos", pTarget->EyePosition())
	else if (Prop.starts_with("velocity"))	RETURN_POSITION("velocity", pTarget->pev->velocity)
	else if (Prop.starts_with("angles"))
	{
		RETURN_ANGLE("angles", pTarget->pev->angles)
			RETURN_POSITION("angles", pTarget->pev->angles)
	}
	else if (Prop.starts_with("viewangles"))
	{
		RETURN_ANGLE("viewangles", pTarget->pev->v_angle)
			RETURN_POSITION("viewangles", pTarget->pev->v_angle)
	}
	else if (Prop == "target")
	{
		CBaseEntity *pPlayerTarget = pTarget->RetrieveEntity(ENT_TARGET);
		return pPlayerTarget ? EntToString(pPlayerTarget) : "0";
	}
	//Thothie MAR2011_10 - return bolt type
	//- $get(<player>,bolt,[remove])
	else if (Prop == "findbolt")
	{
		bool remove_on_find = false;

		if (Params.size() > 1) remove_on_find = true;

		if (!pPlayer->m_ChosenArrow || pPlayer->m_ChosenArrow->iQuantity <= 0 || !msstring(pPlayer->m_ChosenArrow->m_Name).starts_with("proj_bolt"))
		{
			//return "unset";
			CGenericItem *pProjInPack = NULL, *pPack = NULL;
			pProjInPack = pPlayer->GetItem("bolt", &pPack);

			if (pProjInPack)
			{
				if (FBitSet(pProjInPack->MSProperties(), ITEM_GROUPABLE))
				{
					if ((pProjInPack->iQuantity) && pProjInPack->iQuantity >= 1)
					{
						pPlayer->m_ChosenArrow = pProjInPack;
					}
				}
			}
			else
			{
				return "proj_bolt_generic";
			}
		}

		msstring msScriptNameReturn = pPlayer->m_ChosenArrow->m_Scripts[0]->m.ScriptFile.c_str();
		msScriptNameReturn = msScriptNameReturn.findchar_str("/", 0);
		msScriptNameReturn = msScriptNameReturn.substr(msScriptNameReturn.len() - (msScriptNameReturn.len() - 1));

		if (remove_on_find && pPlayer->m_ChosenArrow && !msstring(pPlayer->m_ChosenArrow->m_Name).ends_with("_generic"))
		{
			pPlayer->m_ChosenArrow->iQuantity -= 1;
#ifdef VALVE_DLL
			//Remove the item after it has been depleted
			if( pPlayer->m_ChosenArrow->iQuantity <= 0 )
			{
				pPlayer->RemoveItem( pPlayer->m_ChosenArrow );
				pPlayer->m_ChosenArrow->SUB_Remove( );
			}
#endif
		}

		return msScriptNameReturn.c_str();
	}
	else if (Prop == "steamid")
	{
		//Thothie JUN2007a - return steamID
		CBasePlayer *pPlayer = (CBasePlayer *)pTarget;
		return pPlayer ? pPlayer->AuthID() : "0";
	}
	else if (Prop == "playerspawn")
	{
		return pPlayer ? pPlayer->m_SpawnTransition : "0";
	}
	else if (Prop == "nextplayerspawn")
	{
		return pPlayer ? pPlayer->m_NextTransition : "0";
	}
	else if (Prop == "spawner")
	{
		//DEC2007a - Return msmonster spawn ID to verify still exists
		return (pMonster ? pMonster->m_spawnedby : "0");
	}
	else if (Prop == "roam")
	{
		//Thothie AUG2010_25 - return roam condition
		//- seems all these should be under an if (pMonster) somewherez. :(
		return ((pMonster && pMonster->HasConditions(MONSTER_ROAM)) ? "0" : "1");
	}
	else if (Prop == "lastmap")
	{
		//Thothie JUN2007a - alternate lastmap return
		CBasePlayer *pPlayer = (CBasePlayer *)pTarget;
		return pPlayer ? pPlayer->m_cEnterMap : "0";
	}
	else if (Prop == "gold")
	{
		//Thothie JUN2007a - return Gold (attempt)
		CBasePlayer *pPlayer = (CBasePlayer *)pTarget;
		int iPlayerGoldCount = pPlayer->m_Gold;
		msstring msPlayerGoldCount = STRING(iPlayerGoldCount);
		return msPlayerGoldCount;
	}
	else if (Prop == "isbot")
	{
		//Thothie JUN2007a - check if bot
		if (!pTarget->IsPlayer()) return "1";
		CBasePlayer *pPlayer = (CBasePlayer *)pTarget;
		if (!pTarget->IsNetClient()) return "1";

		msstring msSteamID = pPlayer->AuthID();
		if (msSteamID.contains("UNKNOWN")) return "1";
		if (msSteamID.contains("BOT")) return "1";
		//add more dependants
		return "0";
	}
	else if (Prop == "isplayer") fSuccess = pTarget->IsPlayer() ? true : false;
#ifdef VALVE_DLL
	else if( Prop == "slot" )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)pTarget; //MAR2010_08
		if ( pPlayer ) RETURN_INT( pPlayer->m_CharacterNum )
	}
#endif

	else if (pScripted)
	{
		if (Prop == "scriptvar") return pScripted->GetFirstScriptVar(Params.size() >= 3 ? Params[2] : "");
		//Thothie JAN2013_15 - has effect
		else if (Prop == "haseffect")
		{
			if (Params.size() >= 3)
			{
				for(int i = 0; i < pScripted->m_Scripts.size(); i++) // Check each
				{
					if (pScripted->m_Scripts[i]->VarExists("game.effect.id")) //This is an effect
					if (strcmp(pScripted->m_Scripts[i]->GetVar("game.effect.id"), Params[2].c_str()) == 0) return "1";
				}
				return "0";
			}
			return "0";
		}
		if (pItem)
		{
			if (Prop == "name.full") return SPEECH::ItemName(pItem);
			else if (Prop == "name.full.capital")	return SPEECH::ItemName(pItem, true);
			else if (Prop == "name.prefix") return pItem->DisplayPrefix.c_str(); //Thothie OCT2016_04
			else if (Prop == "is_item")			fSuccess = true;
			else if (Prop == "is_projectile")		fSuccess = pItem->ProjectileData ? true : false;
			else if (Prop == "is_drinkable")		fSuccess = pItem->DrinkData ? true : false;
			else if (Prop == "is_spell")			fSuccess = pItem->SpellData ? true : false;
			else if (Prop == "attacking")			fSuccess = pItem->CurrentAttack ? true : false;
			else if (Prop == "inhand")			fSuccess = (pItem->m_Location == ITEMPOS_HANDS) ? true : false;
			else if (Prop == "is_worn")		fSuccess = pItem->IsWorn() ? true : false;
			else if (Prop == "inworld") fSuccess = pItem->IsInWorld() ? true : false;
			else if (Prop == "drink_amt")	RETURN_INT(pItem->DrinkGetAmt()) //Thothie JUN2007
			else if (Prop == "quality") RETURN_INT(pItem->Quality ? pItem->Quality : 0) //Thothie NOV2014_14
			else if (Prop == "maxquality") RETURN_INT(pItem->MaxQuality ? pItem->MaxQuality : 0) //Thothie NOV2014_14
			else if (Prop == "quantity") RETURN_INT(pItem->iQuantity ? pItem->iQuantity : 0) //Thothie NOV2014_14
			else if (Prop == "hand_index") RETURN_INT(pItem->m_Hand)
			else if (Prop == "wielded") //Thothie OCT2015_31 - need a reliable way to be sure if item is active or not
			{
				if (pItem->IsWorn()) RETURN_INT(0); //reliable, item is worn thus not wielded (eg. bow/shield on back)
				if (pItem->Owner())
				{
					CBasePlayer *pPlayerOwner = pItem->Owner()->IsPlayer() ? (CBasePlayer *)pItem->Owner() : NULL;
					//CBasePlayer *pPlayer = pItem->Owner();
					if (pPlayerOwner)
					{
						if (pItem == pPlayerOwner->Hand(0)) RETURN_INT(1)
						if (pItem == pPlayerOwner->Hand(1)) RETURN_INT(1)
					}
				}
				RETURN_INT(0)
			}
			else if (Prop == "handpref") RETURN_INT(pItem->m_PrefHand) //Thothie DEC2010_04
			else if (Prop == "owner")
			{
				//Thothie FEB2016_17 - this somehow got switched to no longer returning owner ID
				//original: fSuccess = pItem->m_pOwner ? true : false;
#ifndef VALVE_DLL
				//client can't determine owner ID, so just return if owned or not
				fSuccess = pItem->m_pOwner ? true : false;
#else
				if ( pItem->RetrieveEntity(ENT_OWNER) )
				{
					//on server, this will return proper for any item not on the ground
					return EntToString(pItem->RetrieveEntity(ENT_OWNER));
				}
				else
				if ( pItem->Owner() && pItem->Owner()->GetScripted() )
				{
					//this will handle items on ground or undeterminable
					return EntToString(pItem->Owner());
					//return pItem->RetrieveEntity(ENT_OWNER) ? EntToString(pItem->RetrieveEntity(ENT_OWNER)) : "0";
				}
#endif
			}
			else if (Prop == "container.open") fSuccess = pItem->Container_IsOpen() ? true : false;
			else if (Prop == "container.items") RETURN_INT(pItem->Container_ItemCount())
#ifndef VALVE_DLL
			else if (Prop == "viewmodel")		return pItem->m_ViewModel;
			else if (Prop == "viewmodel.id")  RETURN_INT(pItem->GetViewModelID());
#endif
			//Thothie OCT2016_18 Item Mods
#ifdef VALVE_DLL
			/*else if( Prop == "modded" ) return FBitSet(pItem->Properties,ITEM_MODDED) ? "1" : "0";
			else if( Prop == "mods" )
			{
				if ( FBitSet(pItem->Properties,ITEM_MODDED) )
				{
					return pItem->Mods.c_str();
				}
				else
				{
					return "0";
				}
			}*/
#endif
		} //end if ( pItem )
		else if (pMonster)
		{
			if (Prop == "race")		return _strlwr(pMonster->m_Race);
			else if (Prop == "maxhp")	RETURN_FLOAT(pMonster->MaxHP())
			else if (Prop == "relationship" && Params.size() >= 3)
			{
				CBaseEntity *pOtherEntity = RetrieveEntity(Params[2]);
				int RelationShip = pMonster->IRelationship(pOtherEntity);
				switch(RelationShip)
				{
				case RELATIONSHIP_NO:
					return "neutral";
				case RELATIONSHIP_AL:
					return "ally";
				case RELATIONSHIP_NE:
					return "neutral";
				case RELATIONSHIP_FR:
					return "enemy";
				case RELATIONSHIP_WA:
					return "wary";
				case RELATIONSHIP_DL:
					return "enemy";
				case RELATIONSHIP_HT:
					return "enemy";
				case RELATIONSHIP_NM:
					return "enemy";
				default:
					return "neutral";
				}
			}
			else if (Prop == "height")
			{
				RETURN_FLOAT(pMonster->m_Height)
			}
			else if (Prop == "nopush") return (pMonster->m_nopush) ? "1" : "0";
			else if (Prop == "width")
			{
				RETURN_FLOAT(pMonster->m_Width) //MIBJAN2010_20
			}
			else if (Prop == "mp")		RETURN_FLOAT(pMonster->m_MP)
			else if (Prop == "maxmp")	RETURN_FLOAT(pMonster->MaxMP())
			else if (Prop == "movedir") VecToString(pMonster->pev->movedir); //Thothie APR2016_07
#ifdef VALVE_DLL
			else if( Prop == "svbonepos" )
			{
				//Thothie MAR2008a - get positions of bones from server side
				//$get(<target>,svbonepos,<bone_idx>)
				Vector vBoneVec;
				Vector vBoneAngle;
				pMonster->GetBonePosition( atoi(Params[2]), vBoneVec, vBoneAngle );
				return VecToString( vBoneVec );
				//if need be ye can return vBoneAngle by making a seperate command svboneang)
			}
			else if( Prop == "attachpos" )
			{
				//Thothie MAR2008a - get position of an attachment
				//$get(<target>,attachpos,<attchment_idx>)
				Vector vAttVec;
				Vector vAttAngle;
				pMonster->GetAttachment(atoi(Params[2]), vAttVec, vAttAngle );
				return VecToString( vAttVec );
			}
			/*
			else if( Prop == "attachang" )
			{
			//Thothie MAR2008a - get angle of an attachment (failed)
			//$get(<target>,attachang,<attchment_idx>)
			Vector vAttVec;
			Vector vAttAngle;
			pMonster->GetAttachment(atoi(Params[2]), vAttVec, vAttAngle );
			return VecToString( vAttAngle );
			}
			*/
			//AUG2013_21 Thothie - attempting to return last hit group (for headshots)
			//kinda working for players (always returns 2, lest not hit), but not monsters
			else if( Prop == "lasthitgroup" )
			{
				RETURN_INT( pMonster->m_LastHitGroup );
			}
#endif
			else if (Prop == "movedest.origin")	return VecToString(pMonster->m_MoveDest.Origin);
			else if (Prop == "movedest.prox")		RETURN_FLOAT(pMonster->m_MoveDest.Proximity)
			else if (Prop == "moveprox")			RETURN_FLOAT(pMonster->GetDefaultMoveProximity())
			else if (Prop == "movedest.id")		return EntToString(pMonster->m_MoveDest.MoveTarget.Entity());
			else if (Prop == "anim_end") RETURN_FLOAT(gpGlobals->time + ((256 / pMonster->m_flFrameRate) * pMonster->pev->framerate))
			else if (Prop == "stepsize") RETURN_FLOAT(pMonster->m_StepSize)	//MiB DEC2007a
			else if (Prop == "movetype") RETURN_INT(pTarget->pev->movetype) //Thothie JAN2013_20 (post patch)
			else if (Prop == "name.full") return SPEECH::NPCName(pMonster);
			else if (Prop == "name.prefix") return pMonster->DisplayPrefix.len() ? (pMonster->DisplayPrefix) : (""); //Thothie JAN2011_30
			else if (Prop == "name.full.capital")	return SPEECH::NPCName(pMonster, true);
			else if (Prop == "dmgmulti") RETURN_FLOAT(pMonster->m_DMGMulti) //APR2008a
			else if (Prop == "hpmulti") RETURN_FLOAT(pMonster->m_HPMulti) //APR2008a
			else if (Prop == "hitmulti") RETURN_FLOAT(pMonster->m_HITMulti) //FEB2009_18
			else if (Prop == "fly")
			{
				//NOV2014_13 - needed for anti-stuck
				return (pMonster->pev->movetype == MOVETYPE_FLY) ? "1" : "0";
			}
			else if (Prop == "model") //Thothie MAY2018_06
			{
				return STRING(pTarget->pev->model);
			}


			//Thothie FEB2013_06 check if touching other (without the overhead of touch enabled every frame)
			//(fail)
			/*
						else if( Prop == "touching" )
						{
						CBaseEntity *pOther = NULL;
						CBaseEntity::Touch( pOther );
						if ( pOther )
						{
						if ( pOther->IsAlive() )
						{
						return EntToString(pOther);
						}
						else
						{
						return "0";
						}
						}
						else
						{
						return "0";
						}
						}
						*/
			else if (pPlayer)
			{
				if (Prop == "curhand") //Thothie JUN2018_13 return current active hand index
				{
					RETURN_INT(pPlayer->m_CurrentHand); //0=Left Hand Active, 1=Right Hand Active (active hand=left click, off hand=right click)
				}
				else if (Prop == "glowcolor") RETURN_VECTOR(pPlayer->mGlowColor) // MiB APR2019_10 [GLOW_COLOR] - Glow color
				else if (Prop == "gender")	return (pPlayer->m_Gender == GENDER_MALE) ? "male" : "female";
				else if (Prop == "ip") //MiB Dec2007a Returns the ip address of the player NOTE: This COULD be loopback.
				{
#ifdef VALVE_DLL
					clientaddr_t &ClientInfo = g_NewClients[pPlayer->entindex()-1];
					return ClientInfo.Addr;
#endif
				}
#ifdef VALVE_DLL
				else if( Prop == "spellname" ) //MiB Dec2007a - returns the name of the spell on target player at slot 'idx'
				{
					int idx = atoi( Params[2] );
					msstring msSpellNameReturn = pPlayer->m_SpellList[idx].c_str();
					return msSpellNameReturn.c_str();
				}
				else if( Prop == "jumping" )	fSuccess = FBitSet( pPlayer->m_StatusFlags, PLAYER_MOVE_JUMPING ) ? true : false;
				else if( Prop == "companions" )
				{
					//Thothie JUN2008a - return # of companions
					//$get(<target>,companions)
					RETURN_INT( pPlayer->m_Companions.size() )
				}
#endif
				//Thothie JUL2010_29 - get keydown value
				//$get(<player>,keydown,<attack1|attack2|use|jump|moveleft|moveright|back|forward>)
				else if (Prop == "keydown")
				{
					msstring msKeyCheck = (Params.size() >= 3 ? Params[2].c_str() : "");
					if (msKeyCheck.starts_with("attack1")) return ((FBitSet(pPlayer->pbs.ButtonsDown, IN_ATTACK)) ? "1" : "0");
					else if (msKeyCheck.starts_with("attack2")) return ((FBitSet(pPlayer->pbs.ButtonsDown, IN_ATTACK2)) ? "1" : "0");
					else if (msKeyCheck.starts_with("use")) return ((FBitSet(pPlayer->pbs.ButtonsDown, IN_USE)) ? "1" : "0");
					else if (msKeyCheck.starts_with("jump")) return ((FBitSet(pPlayer->pbs.ButtonsDown, IN_JUMP)) ? "1" : "0");
					else if (msKeyCheck.starts_with("moveleft")) return ((FBitSet(pPlayer->pbs.ButtonsDown, IN_MOVELEFT)) ? "1" : "0");
					else if (msKeyCheck.starts_with("moveright")) return ((FBitSet(pPlayer->pbs.ButtonsDown, IN_MOVERIGHT)) ? "1" : "0");
					else if (msKeyCheck.starts_with("forward")) return ((FBitSet(pPlayer->pbs.ButtonsDown, IN_FORWARD)) ? "1" : "0");
					else if (msKeyCheck.starts_with("back")) return ((FBitSet(pPlayer->pbs.ButtonsDown, IN_BACK)) ? "1" : "0");
					else if (msKeyCheck.starts_with("speed")) return ((FBitSet(pPlayer->pbs.ButtonsDown, IN_RUN)) ? "1" : "0");
					else if (msKeyCheck.starts_with("duck")) return ((FBitSet(pPlayer->pbs.ButtonsDown, IN_DUCK)) ? "1" : "0"); //APR2014_03 - Thothie - forgot about duck
				}
#ifndef VALVE_DLL
				else if (Prop == "stamina")		RETURN_FLOAT(pPlayer->Stamina)
				else if (Prop == "stamina.ratio")	RETURN_FLOAT(pPlayer->Stamina / pPlayer->MaxStamina())
#endif
				else if (Prop == "stamina.max")	RETURN_FLOAT(pPlayer->MaxStamina())
				//else if( pPlayer && Prop == "sitting" )	fSuccess = FBitSet( pPlayer->m_StatusFlags, PLAYER_MOVE_SITTING ) ? true : false;
				else if (Prop == "anim.type") { if (pPlayer->m_pAnimHandler) RETURN_INT(pPlayer->m_pAnimHandler->GetID()) else fSuccess = false; }
				else if (Prop == "anim.uselegs") { if (pPlayer->m_pAnimHandler) fSuccess = pPlayer->m_pAnimHandler->UseGait; }
				else if (Prop == "torso_anim") return pPlayer->m_szAnimTorso;
				else if (Prop == "legs_anim") return pPlayer->m_szAnimLegs;
#ifdef VALVE_DLL
				else if( Prop == "in_attack_stance" ) fSuccess = pPlayer->IsInAttackStance();
				else if( Prop == "active_item" ) return EntToString( pPlayer->ActiveItem() );
				else if( Prop == "offhand_item" ) //Thothie JUN2018_13
				{
					if ( pPlayer->m_CurrentHand == 0 )
					{
						return EntToString( pPlayer->Hand(1) );
					}
					else
					{
						return EntToString( pPlayer->Hand(0) );
					}
				}
				else if( Prop == "hand0" )
				{
					return EntToString( pPlayer->Hand(0) );
				}
				else if( Prop == "hand1" )
				{
					return EntToString( pPlayer->Hand(1) );
				}
#endif
				else if (Prop == "currentitem") fSuccess = pPlayer->ActiveItem() ? true : false;
				else if (Prop == "currentitem.anim_torso") { if (pPlayer->ActiveItem()) return pPlayer->ActiveItem()->m_AnimExt; else fSuccess = false; }
				else if (Prop == "currentitem.anim_legs")  { if (pPlayer->ActiveItem()) return pPlayer->ActiveItem()->m_AnimExtLegs; else fSuccess = false; }
#ifdef VALVE_DLL
				else if( Prop.starts_with("skillavg") ) pPlayer->m_ScoreInfoCache.SkillLevel;  //Use the cached value for speed.  skillavg might get checked
				//quite a few times per frame.  Don't waste time re-computing with SkillAvg()
				else if( Prop.starts_with("title") ) return pPlayer->GetTitle( ); //MiB DEC2007a new title system - originally: GetPlayerTitle( pPlayer->m_ScoreInfoCache.TitleIndex );
#else
				else if (Prop.starts_with("skillavg")) pPlayer->SkillAvg();  //use the current one for the client
				else if (Prop.starts_with("title")) return pPlayer->GetTitle();
#endif
				else if (Prop.starts_with("stat."))
				{
					int Max = 100;

					if (Prop.contains(".max")) RETURN_INT(Max)
					else
					{
						msstring Stat = Prop.substr(5).thru_char(".");
						int iStat = GetNatStatByName(Stat);
						if (iStat > -1)
						{
							int Amount = pPlayer->GetNatStat(iStat);

							if (Prop.contains(".ratio")) RETURN_FLOAT_PRECISION(Amount / (float)Max)
							else RETURN_INT(Amount);
						}
						else ALERT(at_console, "Player stat %s doesn't exist!\n", Stat.c_str());
					}
				}
#ifdef VALVE_DLL
				else if( Prop == "numitems" )
				{
					//Thothie APR2011_28 - need to snag current # of items to avoid sploit / fix scroll loss
					/*
					CItemList Gear;
					int TotalItems = pPlayer->Gear.size() - 1;
					foreach( i, pPlayer->Gear.size() )
					{
					CGenericItem *pPack = pPlayer->Gear[i];
					foreach( n, pPack->Container_ItemCount() )
					{
					++TotalItems;
					}
					}
					*/
					RETURN_INT( pPlayer->NumItems() );
				}
				//Thothie SEP2011_17 - This failed because of differences in the two CBasePlayer Defines (I guess)
				/*
				else if( Prop == "lastpush" )
				{
				RETURN_FLOAT( pPlayer->LastPush );
				}
				*/
#endif
				else if (Prop.starts_with("skill."))
				{
					int SubSkill = -1;
					if (Prop.contains(".prof"))			SubSkill = 0;
					else if (Prop.contains(".balance"))	SubSkill = 1;
					else if (Prop.contains(".power"))		SubSkill = 2;

					else if (Prop.contains(".fire"))			SubSkill = 0;	//Magic
					else if (Prop.contains(".ice"))			SubSkill = 1;
					else if (Prop.contains(".lightning"))		SubSkill = 2;
					else if (Prop.contains(".divination"))		SubSkill = 3;
					else if (Prop.contains(".affliction"))		SubSkill = 4;

					int Max = (SubSkill > -1) ? (int)STATPROP_MAX_VALUE : (int)STAT_MAX_VALUE;

					if (Prop.contains(".max")) RETURN_INT(Max)
					else
					{
						msstring Skill = Prop.substr(6).thru_char(".");
						int Stat = GetSkillStatByName(Skill);
						if (Stat > -1)
						{
							int Amount;
							if (SubSkill > -1) Amount = pPlayer->GetSkillStat(Skill, SubSkill);
							else Amount = pPlayer->GetSkillStat(Stat);

							if (Prop.contains(".ratio")) RETURN_FLOAT_PRECISION(Amount / (float)Max)
							else RETURN_INT(Amount);
						}
						else ALERT(at_console, "Player skill %s doesn't exist!\n", Skill.c_str());
					}
				}
			}
		}
	}
	else return "¯NA¯";

	return fSuccess ? "1" : "0";
}

// MiB MAR2015_01 [LOCAL_PANEL] - Commands for local panel
// localmenu.reset
// localmenu.open
// localmenu.close
// registerlocal.menu
// registerlocal.button
// registerlocal.paragraph
// registerlocal.image
//- scope: Server
//- Creates buttons, theme, and source of localized menu
bool CScript::ScriptCmd_LocalPanel(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	if ( Params.size() < 1 ) return true;
	CBaseEntity *pTarget = RetrieveEntity( Params[0] );
	CBasePlayer *pPlayer = pTarget && pTarget->IsPlayer() ? (CBasePlayer *)pTarget : NULL;
	if ( !pPlayer ) return true;
#else
	if (!gViewPort || !gViewPort->m_pLocalizedMenu) return true;
	CLocalizedPanel *pLocal = gViewPort->m_pLocalizedMenu;
#endif

	if (Cmd.Name() == "localmenu.reset")
	{
#ifdef VALVE_DLL
		MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_LOCALPANEL], NULL, pPlayer->pev );
		WRITE_BYTE( 0 );
		MESSAGE_END();
#else
		pLocal->Reset();
#endif
	}
	else if (Cmd.Name() == "localmenu.open")
	{
#ifdef VALVE_DLL
		MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_LOCALPANEL], NULL, pPlayer->pev );
		WRITE_BYTE( 1 );
		WRITE_STRING_LIMIT(EntToString(m.pScriptedEnt), WRITE_STRING_MAX);
		MESSAGE_END();
#else
		pLocal->Show();
#endif
	}
	else if (Cmd.Name() == "localmenu.close")
	{
#ifdef VALVE_DLL
		MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_LOCALPANEL], NULL, pPlayer->pev );
		WRITE_BYTE( 2 );
		MESSAGE_END();
#else
		pLocal->Hide();
#endif
	}
	else if (Cmd.Name() == "registerlocal.menu")
	{
		msstring sTitle = SCRIPTVAR("reg.local.menu.title");
#ifdef VALVE_DLL
		MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_LOCALPANEL], NULL, pPlayer->pev );
		WRITE_BYTE( 3 );
		WRITE_STRING_LIMIT(sTitle, WRITE_STRING_MAX);
		MESSAGE_END();
#else
		pLocal->Reset();
		pLocal->SetPanelTitle(sTitle);
#endif
	}
	else if (Cmd.Name() == "registerlocal.button")
	{
		msstring sText = SCRIPTVAR("reg.local.button.text");
		bool bCloseOnClick = atoi(SCRIPTVAR("reg.local.button.closeonclick")) ? true : false;
		bool bEnabled = atoi(SCRIPTVAR("reg.local.button.enabled")) ? true : false;
		int	cbType = atoi(SCRIPTVAR("reg.local.button.docallback")) ?
#ifdef VALVE_DLL
			1  // Server
#else
			2  // Client
#endif
			: 0; // None
		msstring sCallBack = cbType ? SCRIPTVAR("reg.local.button.callback") : "";
		msstring sCallBackData = SCRIPTVAR("reg.local.button.data");
		if (sCallBackData == "reg.local.button.callback")
		{
			sCallBackData = "";
		}

#ifdef VALVE_DLL
		MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_LOCALPANEL], NULL, pPlayer->pev );
		WRITE_BYTE(4);
		WRITE_STRING_LIMIT(sText, 60);
		WRITE_BOOL(bEnabled);
		WRITE_BOOL(bCloseOnClick);
		WRITE_BYTE(cbType);
		WRITE_STRING_LIMIT(sCallBack, 60);
		WRITE_STRING_LIMIT(sCallBackData, 60);
		MESSAGE_END();
#else
		pLocal->AddButton(sText, bEnabled, bCloseOnClick, cbType, sCallBack, sCallBackData);
#endif
	}
	else if (Cmd.Name() == "registerlocal.paragraph")
	{
		msstring srcType = SCRIPTVAR("reg.local.paragraph.source.type");
		msstring src = SCRIPTVAR("reg.local.paragraph.source");

#ifdef VALVE_DLL
		MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_LOCALPANEL], NULL, pPlayer->pev );
		WRITE_BYTE( 5 );
#endif

		if (srcType == "file")
		{
#ifdef VALVE_DLL
			WRITE_BYTE(0);
#else
			pLocal->ReadParagraphsFromFile(src);
#endif
		}
		else if (srcType == "local")
		{
#ifdef VALVE_DLL
			WRITE_BYTE(1);
#else
			pLocal->ReadParagraphsFromLocalized(src);
#endif
		}
		else
		{
#ifdef VALVE_DLL
			WRITE_BYTE(2);
#else
			pLocal->AddParagraph(src);
#endif
		}

#ifdef VALVE_DLL
		WRITE_STRING_LIMIT(src, WRITE_STRING_MAX);
		MESSAGE_END();
#endif
	}
	else if (Cmd.Name() == "registerlocal.image")
	{
		// MIB FEB2019_22 [LOCAL_PANEL_IMAGE]
		msstring	vName = SCRIPTVAR("reg.local.image.name");
		bool        bIsTga = atoi(SCRIPTVAR("reg.local.image.tga")) == 1;

		int			vFrame = atoi(SCRIPTVAR("reg.local.image.frame"));
		bool		bBorder = atoi(SCRIPTVAR("reg.local.image.border")) == 1;

#ifdef VALVE_DLL
		MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_LOCALPANEL], NULL, pPlayer->pev );
		WRITE_BYTE( 6 );
		WRITE_STRING_LIMIT(vName, WRITE_STRING_MAX);
		if ( bIsTga )
		{
			WRITE_BOOL( true );
		}
		else
		{
			WRITE_BOOL( false );
			WRITE_SHORT( vFrame );
		}
		WRITE_BOOL( bBorder );
		MESSAGE_END();
#else
		pLocal->AddImage(vName.c_str(), bIsTga, bBorder, vFrame);
#endif
	}

	return true;
}

//applyeffect <target> <script> [params...]
//- scope: server
//- applies an affect script to <target>
bool CScript::ScriptCmd_ApplyEffect(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	//<target> <effect script> [param1] [param2]...
	if( Params.size() >= 2 )
	{
		CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;

		if( pEntity && pEntity->GetScripted() && ( pEntity->IsAlive() || pEntity->IsMSItem() ) )
		{
			IScripted *pScripted = pEntity->GetScripted();

			//[begin] DEC2014_11 - allow passing of applyeffects to another mob
			/*
			msstring passeffectid = pScripted->GetFirstScriptVar("NPC_PASS_APPLYEFFECTS");
			if ( passeffectid != "NPC_PASS_APPLYEFFECTS" )
			{
			pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( passeffectid ) : pEntity;
			pScripted = pEntity->GetScripted();
			}
			*/
			//[end] DEC2014_11 - allow passing of applyeffects to another mob
			//Mib came up with better suggestion, see below:

			//[begin] DEC2014_11 - allow script control of applyeffects
			static msstringlist Parameters;
			Parameters.clearitems();
			Parameters.add( Cmd.Name() );
			Parameters.add( EntToString(m.pScriptedEnt) );
			for(int i = 0; i < Params.size(); i++)
			{
				if ( i > 0 ) Parameters.add( Params[i] );
			}
			pScripted->CallScriptEvent( "game_applyeffect", &Parameters );
			if( pScripted->m_ReturnData.len() )
			{
				msstring rdata = pScripted->m_ReturnData;
				if ( rdata.contains(";") )
				{
					static msstringlist Tokens;
					Tokens.clearitems();
					TokenizeString( rdata, Tokens );
					for(int i = 0; i < Tokens.size(); i++)
					{
						if ( Tokens[i] == "redirect" )
						{
							pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Tokens[i+1] ) : pEntity;
							pScripted = pEntity->GetScripted();
						}
					}
				}
				if ( rdata.contains("abort") ) return true;
			}
			//[end] DEC2014_11 - allow script control of applyeffects

			Parameters.clearitems();
			for( int i = 0; i < Params.size() - 2; i++ )
				Parameters.add( Params[i+2] );
			CGlobalScriptedEffects::ApplyEffect( Params[1], pScripted, pEntity, &Parameters);
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}


//array commands
//- scope: uncertain?
//array.create [Entity] <array_name>
//array.add [Entity] <array_name> <value>
//array.erase [Entity] <array_name> - potentially buggy
//array.clear [Entity] <array_name> - empties array name without erasing (safer)
//array.del [Entity] <array_name> <index>
//array.set [Entity] <array_name> <index> <new_value>
//array.copy [Entity] <src_array_name> <dest_array_name>
//g_array.xxx - global array, same options and formatting as described above, but without entity targetting
//- see $get_array, $get_array_amt, $get_array_exists for retrieval functions.
//- see Util_ScriptArray and Util_ScriptArrayGetProps shortcuts for dealing with arrays code side.
bool CScript::ScriptCmd_Array(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//MiB JAN2010_26
	//MiB JUN2010_25 Added global functionality
	/*
		(g_)array.create <name>				Creates an array (does nothing if a same-name array already exists)
		(g_)array.add <name> <val>			Add <val> to the end of array <name>
		(g_)array.set <name> <idx> <val>		Sets <name>[idx] to <val>
		(g_)array.del <name> <idx>			Deletes <name>[idx]
		(g_)array.erase <name>				Erases the array <name>
		*/

	size_t                              vParam = 0;
	CBaseEntity *                       pEnt = NULL;
	if (Params.size() > vParam)
	{
		bool bExisted;
		bool GLOBAL = Cmd.Name().starts_with("g_");
		msstring SubCmd = Cmd.Name().substr(GLOBAL ? 8 : 6);
		msstring ArrName = Params[vParam++];

		if (!GLOBAL && Params.size() > vParam)
		{
			pEnt = RetrieveEntity(ArrName);
			if (pEnt)
			{
				ArrName = Params[vParam++];
			}
		}
		if (!pEnt)
		{
			pEnt = m.pScriptedEnt;
		}

		msscriptarrayhash &             vArrayHashMap = GLOBAL ? GlobalScriptArrays : pEnt->scriptedArrays;
		msscriptarray *                 pArray = GetScriptedArrayFromHashMap(vArrayHashMap
			, ArrName
			, SubCmd == "create"
			, &bExisted
			);

		if (SubCmd != "create") // Create already done
		{
			if (!bExisted)
			{
				Print("Error: Attempting %s on non-existant array (%s)\n", SubCmd.c_str(), ArrName.c_str());
				return true;
			}

			if (SubCmd == "copy")
			{
				if (Params.size() > vParam)
				{
					msstring vsDest = Params[vParam++];
					msscriptarray * pLocalArray = m.pScriptedEnt->GetScriptedArray(vsDest, true);

					for(int i = 0; i < pArray->size(); i++)
					{
						pLocalArray->add((*pArray)[i]);
					}
				}
				else ERROR_MISSING_PARMS;
			}
			else
			if (SubCmd == "add")
			{
				if (Params.size() > vParam)
					pArray->add(Params[vParam++]);
				else ERROR_MISSING_PARMS;
			}
			else
			if (SubCmd == "add_unique") //Thothie SEP2019_29 - array.add_unique
			{
				if (Params.size() > vParam)
				{
					msstring sTemp = Params[vParam++];
					bool bFoundEntry = false;
					for(int i = 0; i < pArray->size(); i++)
					{
						msstring vsCur = (*pArray)[i];
						if (vsCur == sTemp)
						{
							bFoundEntry = true;
							break;
						}
					}
					if (!bFoundEntry) pArray->add(sTemp);
				}
				else ERROR_MISSING_PARMS;
			}
			else
			if (SubCmd == "set")
			{
				if (Params.size() > vParam + 1)
				{
					int                 vSubIndx = atoi(Params[vParam++]);
					if (vSubIndx < 0
						|| (unsigned)vSubIndx >= pArray->size()
						)
					{
						Print("Error: array.set - index %i does not exist in array %s\n", vSubIndx, ArrName.c_str());
					}
					else
					{
						(*pArray)[vSubIndx] = Params[vParam++];
					}
				}
				else ERROR_MISSING_PARMS;
			}
			else
			if (SubCmd == "del")
			{
				if (Params.size() > vParam)
				{
					int                 vSubIndx = atoi(Params[vParam++]);
					if (vSubIndx < 0
						|| (unsigned)vSubIndx >= pArray->size()
						)
					{
						Print("Error: array.del - index %i does not exist in array %s\n", vSubIndx, ArrName.c_str());
					}
					else
					{
						pArray->erase(vSubIndx);
					}
				}
				else ERROR_MISSING_PARMS;
			}
			else
			if (SubCmd == "erase")
			{
				vArrayHashMap.erase(ArrName);
			}
			else
			if (SubCmd == "clear")
			{
				pArray->clear();
			}
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//attackprop <target_item> <attack_idx> <property> <value>
//- scope: server
//- changes the property of a registerattack
bool CScript::ScriptCmd_AttackProp(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	if( Params.size() >= 4 )
	{
		CBaseEntity * pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;
		if (pEntity && pEntity->IsMSItem())
		{
			CGenericItem *pItem = (CGenericItem *) pEntity;
			int attackNum = atoi(Params[1]);
			attackdata_t* AttData = &pItem->m_Attacks[attackNum];
			msstring &PropName = Params[2];
			msstring &PropValue = Params[3];

			//Convert the value to any possibly data type for easy use later
			int Int = atoi(PropValue);
			float Float = atof(PropValue);
			Vector Vec = StringToVec(PropValue);
			bool Bool = PropValue == "true" || PropValue == "1";

			bool SETAPROP = false;

			if (!AttData) return true;

			if( PropName == "type" ) { AttData->sDamageType = PropValue; SETAPROP = true; }
			if( PropName == "range" ) { AttData->flRange = Float; SETAPROP = true; }
			if( PropName == "dmg" ) { AttData->flDamage = Float; SETAPROP = true; }
			if( PropName == "dmg.range" ) { AttData->flDamageRange = Float; SETAPROP = true; }
			if( PropName == "dmg.type" ) { AttData->sDamageType = PropValue; SETAPROP = true; }
			if( PropName == "dmg.multi" ) { AttData->f1DmgMulti = Float; SETAPROP = true; }
			if( PropName == "aoe.range" ) { AttData->flDamageAOERange = Float; SETAPROP = true; }
			if( PropName == "aoe.falloff" ) { AttData->flDamageAOEAttn = Float; SETAPROP = true; }
			if( PropName == "energydrain" ) { AttData->flEnergy = Float; SETAPROP = true; }
			if( PropName == "mpdrain" ) { AttData->flMPDrain = Float; SETAPROP = true; }
			if( PropName == "stat.prof" ) { AttData->StatProf = Int; SETAPROP = true; }
			if( PropName == "stat.balance" ) { AttData->StatBalance = Int; SETAPROP = true; }
			if( PropName == "stat.power" ) { AttData->StatPower = Int; SETAPROP = true; }
			if( PropName == "stat.exp" ) { AttData->StatExp = Int; SETAPROP = true; }
			if( PropName == "noise" ) { AttData->flNoise = Float; SETAPROP = true; }
			if( PropName == "callback" ) { AttData->CallbackName = PropValue; SETAPROP = true; }
			if( PropName == "ofs.startpos" ) { AttData->StartOffset = Vec; SETAPROP = true; }
			if( PropName == "ofs.aimang" ) { AttData->AimOffset = Vec; SETAPROP = true; }
			if( PropName == "priority" ) { AttData->iPriority = Int; SETAPROP = true; }
			if( PropName == "dmg.ignore" ) { AttData->NoDamage = Bool; SETAPROP = true; }
			if( PropName == "hitchance" ) { AttData->flAccuracyDefault = Float; SETAPROP = true; }
			if( PropName == "delay.end" ) { AttData->tDuration = Float; SETAPROP = true; }
			if( PropName == "delay.strike" ) { AttData->tLandDelay = Float; SETAPROP = true; }
			if( PropName == "dmg.noautoaim" ) { AttData->NoAutoAim = Bool; SETAPROP = true; }
			if( PropName == "ammodrain" ) { AttData->iAmmoDrain = Int; SETAPROP = true; }
			if( PropName == "hold_min" ) { AttData->tProjMinHold = Float; SETAPROP = true; }
			if( PropName == "hold_max" ) { AttData->tMaxHold = Float; SETAPROP = true; }
			if( PropName == "projectile" ) { AttData->sProjectileType = PropValue; SETAPROP = true; }
			if( PropName == "COF.l" ) { AttData->flAccuracyDefault = Float; SETAPROP = true; }
			if( PropName == "COF.r" ) { AttData->flAccBest = Float; SETAPROP = true; }
			if( PropName == "delay.end" ) { AttData->tDuration = Float; SETAPROP = true; }
			if( PropName == "reqskill" ) { AttData->RequiredSkill = Int; SETAPROP = true; }

			if( !SETAPROP ) {} //Do whatever ya'd like for when someone puts in a bad command

			if( SETAPROP && pItem->Owner()->IsPlayer() )
			{
				CBasePlayer *pPlayer = (CBasePlayer *)pItem->Owner();

				MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_ITEM], NULL, pPlayer->pev );
				WRITE_BYTE( 6 );
				// Params: ID, Attacknum, prop, value
				WRITE_LONG( pItem->m_iId );
				WRITE_BYTE( attackNum );
				WRITE_STRING_LIMIT(PropName.c_str(), 70);
				WRITE_STRING_LIMIT(PropValue.c_str(), 70);
				SendGenericItem( pPlayer, pItem, false );
				MESSAGE_END();
			}
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}
//callclitemevent <item_id> <event_name> [params...]
//- scope: server
//- calls an event on the client side instance of an item or weapon
bool CScript::ScriptCmd_CallClItemEvent(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//Female inspired clit event... (Really! Needed for female model updates!)
	//calls client event on specific item (affects owner only)
	//callclitemevent <item_id> <event_name> <params...>
#ifdef VALVE_DLL
	if ( Params.size() >= 2 )
	{
		CBaseEntity *pTmp = RetrieveEntity( Params[0] );
		if (!pTmp)
		{
			Print("callclitemevent - pTmp is illegal! \n");
			return false;
		}
		CGenericItem *pItem = pTmp->IsMSItem() ? (CGenericItem *)pTmp : NULL;
		if ( pItem && pItem->Owner() && pItem->Owner()->IsPlayer() )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pItem->Owner();
			if ( !pPlayer ) Print( "callclitemevent - Owner is illegal!\n" );
			msstring EventName = Params[1];

			int NumParams = Params.size() - 2;
			MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_ITEM], NULL, pPlayer->pev );
			WRITE_BYTE( 8 );
			WRITE_LONG( pItem->m_iId );
			WRITE_STRING( EventName.c_str() );
			WRITE_BYTE( NumParams );

			for( int i = 0; i < NumParams; i++ )
				WRITE_STRING( Params[2+i].c_str() );

			MESSAGE_END();
		}
	}
#endif

	return true;
}

//callevent <eventname> [params...]
//callevent <delay> <eventname>
//- scope: shared, flow control
//- Calls a script event by name.
//- Note that one cannot pass params via delayed callevent method, use a setvard instead
bool CScript::ScriptCmd_CallEvent(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 1)
	{
		float Delay = 0;
		IScripted *pScripted = NULL;
		msstring_ref EventName = "<none>";
		size_t NextParm = 0;
		int Loops = 1;
		msstringlist Parameters;
		enum calleventype_e { CE_NORMAL, CE_EXTERNAL, CE_EXTERNAL_ALL, CE_EXTERNAL_PLAYERS, CE_LOOP } Type = CE_NORMAL; //Thothie JUN2007a - added CE_EXTERNAL_PLAYERS

		if (Cmd.Name() == "callexternal")
		{
			if (Params[0] == "all")
			{
				Type = CE_EXTERNAL_ALL;
			}
			else if (Params[0] == "players") //Thothie JUN2007a
			{
				Type = CE_EXTERNAL_PLAYERS;
			}
			else
			{
				CBaseEntity *pEntity = m.pScriptedEnt->RetrieveEntity(Params[NextParm]);
				if (pEntity) pScripted = pEntity->GetScripted();
				Type = CE_EXTERNAL;
			}
			NextParm++;
		}
		else if (Cmd.Name() == "calleventloop")
		{
			Loops = atoi(Params[NextParm]);
			NextParm++;
			Type = CE_LOOP;
		}

		if (Params.size() == NextParm + 1)		//callevent [entity] <eventname>
			EventName = Params[NextParm];

		else if (Params.size() > NextParm + 1)
		{
			if (isdigit(Params[NextParm][0]) && Type != CE_EXTERNAL_ALL) //CE_EXTERNAL_ALL doesn't allow timed calls
			{
				Delay = atof(Params[NextParm]);
				EventName = Params[(++NextParm)++]; //callevent [entity] <delay> <eventname> x x x
			}
			else EventName = Params[NextParm++];		//callevent [entity] <eventname> x x x

			if (Params.size() > NextParm)
			{
				for(int i = 0; i < (Params.size() - NextParm); i++)		//Parameters to pass
					Parameters.add(Params[i + NextParm]);
			}

		}

		if (Delay)
		{
			if (Type == CE_EXTERNAL) { if (pScripted) pScripted->CallScriptEventTimed(EventName, Delay); }	//Need the braces for logic
			else
			{
				SCRIPT_EVENT *seEvent = EventByName(EventName);
				if (seEvent) CallEventTimed(EventName, Delay);
				else MSErrorConsoleText("ExecuteScriptCmd", UTIL_VarArgs("Script: %s, callevent (timed) - event %s NOT FOUND!\n", m.ScriptFile.c_str(), EventName));
			}
		}
		else
		{
			if (Type == CE_EXTERNAL_ALL)
				CallScriptEventAll(EventName, Parameters.size() ? &Parameters : NULL);
			else if (Type == CE_EXTERNAL_PLAYERS) //Thothie JUN2007a
				CallScriptPlayers(EventName, Parameters.size() ? &Parameters : NULL);
			else if (Type == CE_EXTERNAL)
			{
				if (pScripted) pScripted->CallScriptEvent(EventName, Parameters.size() ? &Parameters : NULL);
			}	//Need the braces for logic
			else if (strcmp(EventName, Event.Name))	//Can't call myself recursively
			{
				SetVar("MSC_RESET_LOOP", "-5"); //Thothie SEP2019_08 - resetloop
				int SaveIteration = m.m_Iteration;
				for(int i = 0; i < Loops; i++)
				{
					m.m_Iteration = i;
					RunScriptEventByName(EventName, Parameters.size() ? &Parameters : NULL);
					//Thothie DEC2017_19 - allow breaking loops by setting a var
					if (atoi(GetScriptVar("MSC_BREAK_LOOP")) == 1)
					{
						SetVar("MSC_BREAK_LOOP", "0");
						break;
					}
					if (atoi(GetScriptVar("MSC_RESET_LOOP")) != -5) //Thothie SEP2019_08 - resetloop
					{
						//might not work without changing this loop to a while
						int reset_to_iteration = atoi(GetScriptVar("MSC_RESET_LOOP"));
						m.m_Iteration = static_cast<unsigned long>(reset_to_iteration);
						i = reset_to_iteration;
						SetVar("MSC_RESET_LOOP", "-5");
					}
				}
				m.m_Iteration = SaveIteration;
			}
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//capvar <var> <min> <max>
//- scope: shared, math
//- cap a variable between <min> and <max>
bool CScript::ScriptCmd_CapVar(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 3)
	{
		float flValue = atof(Params[0]);
		if (flValue < atof(Params[1]))	SetVar(Cmd.m_Params[1], Params[1], Event);
		if (flValue > atof(Params[2]))	SetVar(Cmd.m_Params[1], Params[2], Event);
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//changelevel <mapname>
//- scope: server
//- Resets relevant globals and changes map.
bool CScript::ScriptCmd_ChangeLevel(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//changelevel <mapname> - 3 guesses what this does.
	msstring sTemp;
	if (Params.size() > 0)
	{
		sTemp = Params[0];
		//CHANGE_LEVEL( (char *)STRING(sDestMap), NULL );
		CHANGE_LEVEL(sTemp.c_str(), NULL);
		//clear music/weather for next map
		MSGlobals::map_addparams = ""; //DEC2014_17 Thothie - global addparams
		MSGlobals::map_flags = ""; //DEC2014_17 Thothie - map flags
		MSGlobals::MapWeather = "";
		MSGlobals::SpawnLimit = 0;
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//chatlog <text> - writes a line to the current chat log (eg. msc_chatlog_12_2014.log)
//- scope: server
//- check game.cvar.ms_chatlog before writing to chatlog
//- chatlog entries are normally prefixed with $timestamp(>)
bool CScript::ScriptCmd_ChatLog(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//Thothie FEB2008a - Scriptside File I/O
	//fuck dynamic I/O - trying for static
	//chatlog [line] - append a file without loading it in (saves lag for chat log)
#ifdef VALVE_DLL
	msstring msTemp;
	if( Params.size() >= 1 )
	{
		for(int i = 0; i < Params.size(); i++ )
		{
			if( i ) msTemp += " ";
			msTemp += Params[i];
		}

		//Print ( "chatlog: %s \n", sTemp.c_str() );
		chatlog << msTemp.c_str() << std::endl;
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//clearplayerhits <monster> <player|"all">
//- scope: server
//- Removes XP hit history from a monster.
//- Called on all mobs when a player dies. Boss flagged mobs ignore.
//- Prevents exploiting when immune to XP loss from death.
bool CScript::ScriptCmd_ClearPlayerHits(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//MiB JUL2010_31
	//clearplayerhits <monster> <player|"all">
	//Clears hit recordings on <monster> for single <player> or all players.
#ifdef VALVE_DLL
	if ( Params.size() >= 2 )
	{
		CBaseEntity *pEnt = RetrieveEntity( Params[0].c_str() );
		CMSMonster *pMons = pEnt && pEnt->IsMSMonster() ? (CMSMonster *)pEnt : NULL;

		if ( pMons )
		{
			if ( Params[1] == "all" )
			{
				for(int p = 0; p < MAXPLAYERS; p++)
				{
					pMons->m_PlayerDamage[p].Clear(); // MiB MAR2019_22 [SLOT_EXP] - Use new clear function
				}
			}
			else
			{
				pEnt = RetrieveEntity( Params[1].c_str() );
				CBasePlayer *pPlayer = pEnt && pEnt->IsPlayer() ? (CBasePlayer *)pEnt : NULL;
				if ( pPlayer )
				{
					int p = pPlayer->entindex()-1;
					pMons->m_PlayerDamage[p].Clear(); // MiB MAR2019_22 [SLOT_EXP] - Use new clear function
				}
			}
		}
	} else ERROR_MISSING_PARMS;
#endif

	return true;
}

//cleffect
//- scope: client
//- Client side, invokes effect. Uses too numerous to list, but include:
//- cleffect light <new|lightid> <origin> <radius> <(r,g,b)> <duration> ["entity"|"dark"]
//-- creates or updates a light, stores light index in game.script.last_light_id
//- cleffect beam_points <startpos> <endpos> <spritename> <life> <width> <amplitude> <brightness> <speed> <framerate> <color>
//- cleffect beam_points <startidx> <attachment#> <endidx> <attachment#> <spritename> <life> <width> <amplitude> <brightness> <speed> <framerate> <color>
//- cleffect decal <decal_index> <vectrace_start> <vectrace_end> - paints a decal at the collision point
//- cleffect ce <player|GM> <event_name> [params...] - calls script event on the server instance of player idx or GM. Be sure to encase vectors to be returned in $quote(), may also need an extra param at the end.
//- cleffect spark <origin|model_idx> [attachment_idx] - creates a spark effect at origin or on model
//- cleffect clientcmd <cmds> - executes <cmd> as if through client's console, bypassing Steam Pipe "security" restrictions
//- cleffect <"tempent"|"frameent"> <"sprite"|"model"> <sprite_file|model_file> <origin> <setup_event> [update_event] [collide_event]
//-- sets up a tempent or frameent with properties defined in <setup_event>
//- cleffect <"tempent"|"frameent"> set_current_prop <property> <value> [value] - sets or updates properties of tempent or frameup created by the above command
//-- origin <vec>
//-- angles <vec>
//-- velocity <vec>
//-- body <idx> - submodel index (models only) at the moment this is the internal index, not the two part smart index
//-- death_delay <secs> - seconds until tempent vanishes, changing in update has no affect
//-- follow <idx|none> [attachment] - follow another ent by attachment
//-- frame - set specific sprite or animation frame
//-- framerate <fps|ratio> - flat fps for sprites, ratio for models
//-- gravity <ratio> - can be set negative
//-- mins/maxs <vec> - bounding box for collisions
//-- model/sprite <filename> - can be used to change model/sprite in update
//-- owner <idx> - for following tempents
//-- prevstate.fuser1-4 - state of fuser1-4 before the last change
//-- prevstate.iuser1-4 - state of iuser1-4 before the last change
//-- properties include:
//-- renderamt <0-255> - amount to use for rendermode
//-- rendercolor <vec> - (R,G,B) value to use if rendermode is add or color
//-- renderfx <normal|glow|player>
//-- rendermode <normal|add|alpha|color|texture|glow>
//-- scale <ratio>
//-- scaleHD <ratio> - scale *1000 - for fine tuning scale ratios (scripts have 2 dec precision limitations_
//-- sequence <animation_index> (models only)
//-- skin <idx> - skin index (models only)
//-- update - tempents with an update event will automatically run their updates, you can set this to 0 to stop that behavior
//-- iuser1 - store integer, can be retrieved in update/collision events via game.tempent.iuser1
//-- iuser2 - as above
//-- iuser3 - as above
//-- iuser4 - as above, but reserved for use with some internal interactions
//-- fuser1 - store float, can be retrieved in update/collision events via game.tempent.fuser1
//-- fuser2 - as above
//-- fuser3 - as above
//-- fuser4 - as above, but reserved for use with some internal interactions
//-- fuser1HD <float> - fuser1 *1000 - for dealing with the script's 2 dec precision limitations
//-- baseline.fuser1-4 - initial state of fuser1-4
//-- baseline.iuser1-4 - initial state of iuser1-4
//-- most properties can be retrieved via game.tempent.xxx in any update/collision event
bool CScript::ScriptCmd_ClEffect(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifndef VALVE_DLL
	CLScriptedEffect(Params);
#endif
	return true;
}

//clientcmd <player|all> <cmds...>
//- scope: server
//- Sends commands as if to client's console
//- Beware that new Steam Pipe "security" features prevent some commands from being sent
//- See the client side version of this in cleffect for a work around.
bool CScript::ScriptCmd_ClientCmd(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//Thothie FEB2008a - client<->script management
	//- clientcmd <target|all> <sting>
	//- sends command direct to client
	//Thothie JAN2011_04 - udpated for use with 'all' option
#ifdef VALVE_DLL
	msstring msTemp;
	if( Params.size() >= 2 )
	{
		for( int i = 0; i < Params.size()-1; i++ )
		{
			if( i ) msTemp += " ";
			msTemp += Params[i+1];
		}
		msTemp += "\n";

		if ( Params[0] != "all" )
		{
			CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;
			if( pEntity->IsPlayer() )
			{
				CBasePlayer* pPlayer = (CBasePlayer *)pEntity;

				if ( pPlayer )
				{
					CLIENT_COMMAND( pPlayer->edict(), "%s", msTemp.c_str() );
				}
			}
			else ALERT( at_aiconsole, "Attempting to send client command to non-player!");
		}
		else
		{
			for( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBaseEntity *pEntity = UTIL_PlayerByIndex( i );
				CBasePlayer *pPlayer = (CBasePlayer *)pEntity;;
				if ( pPlayer ) CLIENT_COMMAND( pPlayer->edict(), "%s", msTemp.c_str() );
			}
		}
	}
	else ERROR_MISSING_PARMS;
#endif
	return true;
}

//clientevent <new||persists> <player|all> <script_file> [params...]
//clientevent <update> <player|all> <script_idx> <event_name> [params...]
//- scope: server
//- Creates or updates a client side script on the client.
//- On creation, params are passed to { client_activate event.
bool CScript::ScriptCmd_ClientEvent(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	//<new/persist> <target> <scriptname> [param1] [param2] etc..
	//<update> <target> <scriptid> <eventname> [param1] [param2] etc..
	//<remove> <target> <scriptid>

	if( Params.size() >= 3 )
	{
		//Thothie NOV2006 need a global that stores the # of client side events
		//prevent more than 10 running at once
		//Thothie - Revised JUN2007a - this isn't helping, removing
		/*CBasePlayer *pOtherPlayer = (CBasePlayer *)UTIL_PlayerByIndex( 1 );
		int fxlimiter = 0;
		fxlimiter = CVAR_GET_FLOAT("ms_fxlimit");
		if ( fxlimiter > 0 ) pOtherPlayer->CallScriptEvent( "game_fx_spawn" );
		int fxfloodlevel = 0;
		fxfloodlevel = atoi(GetVar( "G_TOTAL_FX" ));
		if ( fxfloodlevel > fxlimiter && fxlimiter > 0 )
		{
		ALERT( at_console, "Some FX not displayed - Total Flood Level: %i/%f", fxfloodlevel, fxlimiter );
		return false;
		}
		else
		{
		*/
		bool Persistent = false;
		msstring MsgType = Params[0];
		if( Params[0] == "persist" )
		{
			MsgType = "new";
			Persistent = true;
		}

		scriptsendcmd_t SendCmd;

		SendCmd.ScriptName = Params[2];
		SendCmd.UniqueID = (MsgType == "new") ? ++m_gLastSendID : atoi(SendCmd.ScriptName);
		SendCmd.MsgType = MsgType;
		SendCmd.MsgTarget = Params[1];
		if( Params.size() >= 4 )
		{
			for( int p = 0; p < Params.size() - 3; p++ )
				SendCmd.Params.add( Params[p+3] );
		}

		SendScript( SendCmd );
		if( Persistent ) m.PersistentSendCmds.add( SendCmd );
		else if( Params[0] == "remove" )
		{
			//Stop event from persisting after "clientevent remove" is called on it
			for(int e = 0; e < m.PersistentSendCmds.size( ); e++)
			{
					if( m.PersistentSendCmds[e].UniqueID == SendCmd.UniqueID )
					{
						m.PersistentSendCmds.erase( e );
						break;
					}
			}
		}
		//}  //end if fxfloodlevel > 20 else
	} //end if  Params.size() >= 3
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//companion <add|remove> <target_companion> <player>
//- scope: server
//- defunct - once used to manage player companions
bool CScript::ScriptCmd_Companion(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//probably defunct, saving companions this way causes corruption issues
#ifdef VALVE_DLL
	//<add/remove> <target companion> <target player>
	if( Params.size() >= 3 )
	{
		CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[2] ) : NULL;
		if( pEntity && pEntity->IsPlayer() )
		{
			CBaseEntity *pCompanionEnt = m.pScriptedEnt->RetrieveEntity( Params[1] );
			if( m.pScriptedEnt && pCompanionEnt->IsMSMonster() )
			{
				CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
				CMSMonster *pCompanion = (CMSMonster *)pCompanionEnt;

				if( Params[0] == "add" )
				{
					bool CanAdd = true;
					for( int i = 0; i < pPlayer->m_Companions.size(); i++ )
					{
							if( pPlayer->m_Companions[i].Entity.Entity() == pCompanion )
							{
								CanAdd = false;	//Already exist on player's list
								break;
							}
					}

					if( CanAdd )
					{
						companion_t NewCompanion;
						NewCompanion.Active = true;
						NewCompanion.Entity = m.pScriptedEnt;
						NewCompanion.ScriptName = pCompanion->m_ScriptName;
						pPlayer->m_Companions.add( NewCompanion );

						pCompanion->StoreEntity( pPlayer, ENT_OWNER );
						pPlayer->CallScriptEvent( "game_companion_added" );
						pCompanion->CallScriptEvent( "game_companion_added" );
					}
				}
				else //"remove"
				{
					for( int i = 0; i < pPlayer->m_Companions.size(); i++ )
					{
						if( pPlayer->m_Companions[i].Entity.Entity() == pCompanion )
						{
							pPlayer->m_Companions.erase( i );
							break;
						}
					}

					pCompanion->StoreEntity( NULL, ENT_OWNER );
					pCompanion->CallScriptEvent( "game_companion_removed" );
					pPlayer->CallScriptEvent( "game_companion_removed" );
				}
			}
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//conflictcheck
//- scope: shared
//- checks all vars for const/setvard/local conflicts (developer builds only)
bool CScript::ScriptCmd_ConflictCheck(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//Thothie JAN2013_08 - Alert Const/Setvard conflicts
	//so far fail, only picking up two vars on the test script
	//conflictcheck [dumpvars]
	//if dumpvars is used, just dumps all the vars to console instead (this can take time)
#if !TURN_OFF_ALERT
	CBaseEntity *pEntity = m.pScriptedEnt->RetrieveEntity("ent_me");
	IScripted *pScripted = pEntity->GetScripted();

	bool cc_dumpvars = (Params.size() >= 1);
	bool cc_noglobals = (Params.size() >= 2);

	if (cc_dumpvars)
	{
		Print("Conflictcheck: Dumping Vars for %s:\n", pScripted->m_Scripts[0]->m.ScriptFile.c_str());
		if (!cc_noglobals)
		{
			Print("GLOBALS in %s:\n", pScripted->m_Scripts[0]->m.ScriptFile.c_str());
			for(int i = 0; i < m_gVariables.size(); i++)
			{
				Print("setvarg: %s %s\n", m_gVariables[i].Name.c_str(), m_gVariables[i].Value.c_str());
			}
		}
		Print("CONSTANTS in %s:\n", pScripted->m_Scripts[0]->m.ScriptFile.c_str());
		for(int s = 0; s < pScripted->m_Scripts.size(); s++)
		{
			for(int i = 0; i < pScripted->m_Scripts[s]->m_Constants.size(); i++)
			{
				Print("const: %s %s\n", pScripted->m_Scripts[s]->m_Constants[i].Name.c_str(), pScripted->m_Scripts[s]->m_Constants[i].Value.c_str());
			}
		}
		Print("SETVARDS in %s:\n", pScripted->m_Scripts[0]->m.ScriptFile.c_str());
		for(int s = 0; s < pScripted->m_Scripts.size(); s++)
		{
			for(int i = 0; i < pScripted->m_Scripts[s]->m_Constants.size(); i++)
			{
				Print("setvard: %s %s\n", pScripted->m_Scripts[s]->m_Variables[i].Name.c_str(), pScripted->m_Scripts[s]->m_Variables[i].Value.c_str());
			}
		}
	}
	else
	{
		bool cc_found = false;

		//This will never find a conflict, as the variables cross reference is broken when they are set

		for(int s = 0; s < pScripted->m_Scripts.size(); s++)
		{
			for(int i = 0; i < pScripted->m_Scripts[s]->m_Variables.size(); i++)
			{
				for(int c = 0; c < pScripted->m_Scripts[s]->m_Constants.size(); c++)
				{
					if (pScripted->m_Scripts[s]->m_Constants[c].Name == pScripted->m_Scripts[s]->m_Variables[i].Name)
					{
						cc_found = true;
						Print("Conflictcheck: conflict with var %s in %s:\n", pScripted->m_Scripts[s]->m_Constants[c].Name.c_str(), pScripted->m_Scripts[0]->m.ScriptFile.c_str());
						MSErrorConsoleText("", UTIL_VarArgs("Script: %s, %s const/setvard confict!\n", m.ScriptFile.c_str(), pScripted->m_Scripts[s]->m_Variables[i].Name.c_str()));
					}
				} //consts
			} //vars
		} //scripts
		if (!cc_found) Print("Conflictcheck: No conflicts found in %s\n", pScripted->m_Scripts[0]->m.ScriptFile.c_str());
	}
#endif

	return true;
}

//creatnpc <script_name> <origin> [params...]
//creatitem <item_name> <origin> [params...]
//- scope: server
//- Creates npc script or item at <origin>
//- Params are passed to game_dynamically_created, which initiates as if at load time.
//- Any media used by createnpc <script_name> must be precached beforehand.
bool CScript::ScriptCmd_Create(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{	//Parameters: <script name> <position> [PARAM1] [PARAM2] etc...
#ifdef VALVE_DLL
	if( Params.size() >= 2 )
	{
		Vector Position = StringToVec( Params[1] );

		CBaseEntity *pEntity = NULL;
		IScripted *pScript = NULL;

		if( Cmd.Name() == "createnpc" )
		{
			CMSMonster *NewMonster = (CMSMonster *)GET_PRIVATE(CREATE_NAMED_ENTITY(MAKE_STRING("ms_npc")));
			pEntity = NewMonster;
			if( NewMonster ) {
				NewMonster->pev->origin = Position;
				NewMonster->Spawn( Params[0] );
				pScript = NewMonster;
			}
		}
		else
		{
			CGenericItem *pNewItem = NewGenericItem( Params[0] );
			pEntity = pNewItem; pScript = pNewItem;
			if( pNewItem )
				pNewItem->pev->origin = Position;
		}

		if( pEntity )
		{
			pEntity->StoreEntity( m.pScriptedEnt, ENT_CREATIONOWNER );
			if( m.pScriptedEnt )
				m.pScriptedEnt->StoreEntity( pEntity, ENT_LASTCREATED );

			if( pScript )
			{
				//Eveything starting from param 3 is passed to the created entity as PARAM1 PARAM2, etc.
				static msstringlist Params2;
				Params2.clearitems( );
				for( int i = 0; i < Params.size() - 2; i++ )
					Params2.add( Params[i+2] );

				pScript->CallScriptEvent( "game_dynamically_created", &Params2 );
			}
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//darkenbloom <level>
//- scope: client
//- client side, reduces bloom effect for use with white maps
//(unless overriden by client setting ms_bloom_darken other than -1
bool CScript::ScriptCmd_DarkenBloom(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//darkenbloom <level> - client side, reduces bloom effect for use with white maps
	//(unless overriden by client setting ms_bloom_darken other than -1
#ifndef VALVE_DLL
	if (Params.size() >= 1)
	{
		MSCLGlobals::mapDarkenLevel = atoi(Params[0]);
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//dbg [params...]
//- scope: shared
//- debug returns params to console.
//- developer builds only.
bool CScript::ScriptCmd_Debug(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#if !TURN_OFF_ALERT
	msstring sTemp;
	for(int i = 0; i < Params.size(); i++)
		sTemp += (i ? msstring(" ") : msstring("")) + Params[i];

	//Thothie MAR2008a - prevent dbg overflows
	if (sTemp.len() > 140)
	{
		msstring stupid_string = sTemp.substr(0, 140);
		sTemp = stupid_string;
		sTemp += "*\n";
	}

	msstring_ref LocationString = "Server";
#ifndef VALVE_DLL
	LocationString = "Client";
#endif
	Print("* Script Debug (%s): %s - %s\n", LocationString, m.pScriptedEnt ? m.pScriptedEnt->DisplayName() : "(No Entity)", sTemp.c_str());
#endif

	return true;
}

//deleteme
//deleteent <target> [fade]
//- scope: server
//- Remove self, or another entity, with optional fade effect
//- If used on a script tied to a msmonster_spawn, it will not realize the monster was removed.
//- (Thinking maybe we should fix that, not sure)
bool CScript::ScriptCmd_DeleteEntity(SCRIPT_EVENT& Event, scriptcmd_t& Cmd, msstringlist& Params)
{
	if (m.pScriptedEnt)
	{
		if (Cmd.Name() == "deleteme")
		{
			if (!m.pScriptedEnt->IsPlayer())	//Don't allow a crash by deleting players
			{
				//Thothie OCT2016_12 - notification of removal
				IScripted* pScripted = m.pScriptedEnt->GetScripted();
				pScripted->CallScriptEvent("game_deleted");
				m.pScriptedEnt->DelayedRemove();
			}
		}
		else if (Params.size() >= 1)
		{
			CBaseEntity* pEntity = m.pScriptedEnt->RetrieveEntity(Params[0]);
			if (pEntity && !pEntity->IsPlayer())
			{
#ifdef VALVE_DLL
				if (Params.size() >= 2)
				{
					if (Params[1] == "fade")
					{
						if (Params.size() >= 3)
							pEntity->SUB_FadeOut(atoi(Params[2]));
						else
							pEntity->SUB_StartFadeOut();
					}
					if (Params[1] == "remove")
					{
						//Thothie FEB2015_19 - allow removing of map entities
						UTIL_Remove(pEntity);
					}
				}
				else
				{
					//Thothie OCT2016_12 - notification of removal
					IScripted* pScripted = pEntity->GetScripted();
					pScripted->CallScriptEvent("game_deleted");
					pEntity->DelayedRemove();
				}
#endif
			}
		}
	}

	return true;
}

//desc
//- scope: shared, items
//- Descriptions for items

const int SCRIPT_DESC_MAX_LENGTH = 96;

bool CScript::ScriptCmd_Desc(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	msstring sTemp;
	if (Params.size() >= 1)
	{
		for(int i = 0; i < Params.size(); i++)
		{
			if (i) sTemp += " ";  sTemp += Params[i];
		}
		if (sTemp.len() > SCRIPT_DESC_MAX_LENGTH)
		{
			sTemp = sTemp.substr(0, SCRIPT_DESC_MAX_LENGTH); //APR2011_28 - Thothie buffer overrun
			IScripted *pScripted = m.pScriptedEnt->GetScripted();
			Print("WARNING: Description too long [%s]!\n", pScripted->m_Scripts[0]->m.ScriptFile.c_str());
		}
		if (m.pScriptedEnt) m.pScriptedEnt->DisplayDesc = sTemp;
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//drainhp <target> <drainer> <amt> [dmg_type]
//- scope: server
//- Drains HP directly instead of via dodamage.
//- Useful for when rapid special attacks need to reduce overhead.
//- [dmg_type] is future use, for now read the proper immune var ($get_takedmg) and adjust damage
bool CScript::ScriptCmd_DrainHP(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//Drainhp <target> <drainer> <amt> [dmg_type]
	//- sometimes do damage spams ents, so this can be used, optionally
	//- [dmg_type] is future use, for now read the proper immune var ($get_takedmg) and adjust damage
#ifdef VALVE_DLL
	if( Params.size() >= 3 )
	{
		CBaseEntity *pTarget = m.pScriptedEnt->RetrieveEntity( Params[0] );
		CBaseEntity *pAttacker = m.pScriptedEnt->RetrieveEntity( Params[1] );
		float Amt = -atof(Params[2]);
		pTarget->Give( GIVE_HP, Amt );
		float targ_health = pTarget->pev->health;
		targ_health += Amt;
		if ( targ_health <= 0 )
		{
			//FINISH HIM! :) (otherwise target wont actually die)
			static msstringlist Params2;
			Params2.clearitems( );
			Params2.add( EntToString(pAttacker) );
			IScripted *pScripted = pTarget->GetScripted();
			pScripted->CallScriptEvent( "game_drain_death", &Params2 ); //dodamage direct here
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//drainstamina <amt>
//- scope: shared, players
//- Reduces player's stamina by amount.
bool CScript::ScriptCmd_DrainStamina(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 2)
	{
		float Amt = atof(Params[1]);
#ifdef VALVE_DLL
		CBaseEntity *pTarget = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;
		if( pTarget && pTarget->IsPlayer( ) )
		{
			MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_CLDLLFUNC], NULL, pTarget->pev );
			WRITE_BYTE( 5 );
			WRITE_LONG( Amt );
			MESSAGE_END();

		}
#else
		Player_UseStamina(Amt);
#endif
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//drop_to_floor [target]
//- scope: server
//- Moves self or target to ground.
bool CScript::ScriptCmd_DropToFloor(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	CBaseEntity *pTarget = (Params.size() >= 1) ? RetrieveEntity( Params[0] ) : m.pScriptedEnt;

	if( pTarget ) DROP_TO_FLOOR( pTarget->edict() );
#endif

	return true;
}
//effect
//- scope: server
//- initiates various server side effects, including:
//- effect beam point <spritename> <width> <startpos> <endpos> <(r,g,b)> <brightness> <noise> <duration>
//- effect beam end <spritename> <width> <startpos> <target> <attachment index> <(r,g,b)> <brightness> <noise> <duration>
//- effect beam ents <spritename> <width> <target1> <attach#> <target2> <attach#> <(r,g,b)> <brightness> <noise> <duration> - stores in ent_lastcreated
//- effect beam vector <spritename> <width> <startpos> <endpos> <(r,g,b)> <brightness> <noise> <duration> - stores in ent_lastcreated
//- effect beam update <beam_id> <property> <value> [option] - see Scripting_AllDocs.txt for available properties
//- effect beam follow <model> <target> <attach> <width> <life> <brightness> <color> - hornet style beam follow (does not make a server side entity - but can't be tracked/updated either)
//- effect tempent gibs <modelname/spritename> <position> <size> <velocity> <randomness> <amount> <duration>
//- effect tempent spray <modelname/spritename> <position> <direction> <count> <speed> <noise>
//- effect tempent trail <modelname/spritename> <start> <dest> <count> <life> <scale> <speed> <randomness>
//- effect glow <target> <(r,g,b)> <amount> <duration> <fadeduration> - glow shell
//- effect screenshake <position> <amplitude> <frequency> <duration> <radius>
//- effect screenfade <target/"all"> <duration> <holdtime> <(r,g,b)> <alpha> <fadein|fadeout|noblend|perm>
bool CScript::ScriptCmd_Effect(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	ScriptedEffect( Params );
#endif

	return true;
}

//emitsound <srcentity> <origin> <volume> <duration> <type> [danger radius]
//- scope: server
//- various types of utility "sounds" for NPCs to respond to.
//- common usage is "combat" and "danger", both managed through game_heardsound
bool CScript::ScriptCmd_EmitSound(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	//<srcentity> <origin> <volume> <duration> <type> [danger radius]
	if( Params.size( ) >= 5 )
	{
		if( m.pScriptedEnt )
		{
			CBaseEntity *pEntity = m.pScriptedEnt->RetrieveEntity( Params[0] );
			if( pEntity )
			{
				Vector Origin = StringToVec(Params[1]);
				float Volume = atof(Params[2]);
				float Duration = atof(Params[3]);
				msstring &SoundType = Params[4];
				float DangerRadius = atof(Params[5]);

				CSoundEnt::InsertSound( pEntity, SoundType, Origin, Volume, Duration, DangerRadius );
			}
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//erasefile <filename>
//- scope: uncertain?
//- erases a file
bool CScript::ScriptCmd_EraseFile(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//MiB FEB2008a - Scriptside File I/O
	//erasefile <filename>
#ifdef VALVE_DLL
	if( Params.size() > 0 )
	{
		char cFileName[MAX_PATH];
		msstring fname = Params[0];
		bool clearFromHere = Params.size() >= 2 ? Params[1] != "no_clear" : true;
		_snprintf(cFileName, sizeof(cFileName), "%s/%s", EngineFunc::GetGameDir(), fname.c_str());


		if( clearFromHere ) //Remove from the filesOpen list unless specified otherwise.
			for( int i = 0; i < m.pScriptedEnt->filesOpen.size(); i++ )
		{
				if( m.pScriptedEnt->filesOpen[i].fileName == fname )
				{
					m.pScriptedEnt->filesOpen.erase( i );
					break;
				}
			}

		std::remove(cFileName);
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//errormessage <msg>
//popup <msg>
//- scope: shared
//- Creates an error dialogue.
//- errormessage also closes the server or client.
bool CScript::ScriptCmd_ErrorMessage(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//Thothie JUN2007a - send pop up message and exit
	//Thothie FEB2009_21 - merging "errormessage" and "popup" to save elseif blocks
	msstring sTemp;
	for(int i = 0; i < Params.size(); i++)
		sTemp += (i ? msstring(" ") : msstring("")) + Params[i];

	//Print( "* Script Debug (%s): %s - %s\n", LocationString, m.pScriptedEnt ? m.pScriptedEnt->DisplayName() : "(No Entity)", sTemp.c_str() );
	if (Cmd.Name() == "errormessage")
	{
#ifndef VALVE_DLL
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Script Error", sTemp.c_str(), NULL);
#endif
		exit(-1);
	}
	if (Cmd.Name() == "popup")
	{
#ifndef VALVE_DLL
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Script Error", sTemp.c_str(), NULL);
#endif
	}

	return true;
}

//exit
//- scope: shared
//- (not yet funcitoning, intent is to exit an event.)
bool CScript::ScriptCmd_exitevent(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//Thothie FEB2008a
	//exit - flow control - exit current code level (same as open if that returned negative)
	Event.bFullStop = true;
	return false;
}

//gagplayer <target> <0|1>
//- scope: server
//- mutes a player - not entirely functional
bool CScript::ScriptCmd_GagPlayer(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//Thothie FEB2008b - flag the player as gagged via admin_gag
	//gagplayer <target> <value>
#ifdef VALVE_DLL
	CBaseEntity *pEntity = RetrieveEntity( Params[0] );
	if ( pEntity )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
		if ( pPlayer )
		{
			pPlayer->m_Gagged = atoi(Params[1].c_str()) > 0 ? true : false; //MIB JUL2010_06 add a c_str() here
		}
	}
#endif

	return true;
}

//getents <type> <radius>
//- scope: server
//- Stores found entities in GET_ENT1-9, and the number in GET_COUNT
//- Depreciated - use $get_tsphere or $get_tbox instead.
//- Client version is $getcl_tsphere, which stores entity indexes, instead of pentids.
bool CScript::ScriptCmd_GetEnts(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//getents <type> <radius>
	//types are player, monster, or npc (both)
	//stores up to nine ents in script side array GET_ENTx (x=1-9)
	//number of ents stored in GET_COUNT
	//hopefully can be used in place of dodamage scans
#ifdef VALVE_DLL
	msstring &Name = Params[0];
	float flAreaSize = atof(Params[1]);

	CBaseEntity *pList[255], *pEnt = NULL;
	Vector StartPos = Params.size() == 3 ? StringToVec(Params[2]) : m.pScriptedEnt->pev->origin;
	int count = UTIL_MonstersInSphere( pList, 255, StartPos, flAreaSize);
	int iCurrentStore = 0;
	for(int i = 0; i < count; i++)
	{
		pEnt = pList[i];
		if ( !pEnt->IsAlive() ) continue;

		if ( m.pScriptedEnt->entindex() == pEnt->entindex() ) continue; //dont count self
		if ( !pEnt->IsMSMonster() && !pEnt->IsPlayer() ) continue;

		bool bStoreEnt = false;
		if( !_stricmp("player",Name) && pEnt->IsPlayer() ) bStoreEnt = true;
		if( !_stricmp("monster",Name) && pEnt->IsMSMonster() && !pEnt->IsPlayer() ) bStoreEnt = true;
		if( !_stricmp("any",Name) ) bStoreEnt = true;

		if ( bStoreEnt )
		{
			msstring msEntStoreReturn = msstring("GET_ENT") + ++iCurrentStore;
			SetVar( msEntStoreReturn , EntToString(pEnt), Event );
			if ( iCurrentStore >= 9 ) break;
		}

	}
	SetVar( "GET_COUNT", UTIL_VarArgs("%d",iCurrentStore), Event );
#endif

	return true;
}

//getitemarray <player> <array_name> [gear_only:1|0]
//- scope: server
//- Stores player item entity id's into an array.
//- This maybe defunct, or in need of modification, after we get player's undrawn items out of the world.
bool CScript::ScriptCmd_GetItemArray(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	// getitemarray <player> <array_name> [gear_only:1|0]
	// if <array_name> doesn't exist, it creates it. If it does, it overwrites it
	// Creates an array of ent-strings for all items worn on the target player
	// MIB ITEM TODO
#ifdef VALVE_DLL
	msscriptarrayhash::iterator         iArray;
	msscriptarray *                     pArray;
	bool                                bExisted;
	if ( Params.size() >= 2 )
	{
		CBaseEntity *pEnt = RetrieveEntity( Params[0] );
		if ( pEnt && pEnt->IsPlayer() )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pEnt;
			msstring ArrayName = Params[1];
			pArray = pPlayer->GetScriptedArray( ArrayName, true, &bExisted );

			pArray->clearitems();
			for(int i = 0; i < pPlayer->Gear.size(); i++)
			{
				CGenericItem *cur_item = pPlayer->Gear[i];
				if ( cur_item->IsWorn() )
				{
					pArray->add( EntToString( cur_item ) );
					//NOV2015 - dump all inv
					if ( pPlayer->Gear[i]->PackData && !( Params.size()>=3 && Params[2] == "1" ) )
					{
						CGenericItem *pPack = pPlayer->Gear[i];
						if ( pPack->Container_ItemCount() )
						{
							for(int n = 0; n < pPack->Container_ItemCount(); n++)
							{
								pArray->add( EntToString(pPack->Container_GetItem(n)) );
							}
						}
					}
				}
			}
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//getplayer <var> <idx>
//- scope: server
//- Grabs a player by index, and stores in <var>
bool CScript::ScriptCmd_GetPlayer(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//Thothie JUN2007a
	//getplayer <store_string> <idx>
	//- pull a single player by idx
#ifdef VALVE_DLL
	if( Params.size() >= 2 )
	{
		int player_idx = atoi(Params[1]);
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( player_idx );
		if ( pPlayer ) SetVar( Cmd.m_Params[1], EntToString(pPlayer), Event );
		else SetVar( Cmd.m_Params[1], "0", Event );
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//getplayers <var>
//- scope: server
//- Tokenizes player ID's into a var
bool CScript::ScriptCmd_GetPlayers(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//Thothie JUN2007a
	//getplayers <store_string>
	//- get all players, store in token string
#ifdef VALVE_DLL
	msstring msStorePlayers;
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pOtherPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
		if ( !pOtherPlayer ) continue;

		msStorePlayers += EntToString(pOtherPlayer);
		msStorePlayers += ";";
	}
	//ALERT(at_console,"stored players %s", thoth_storeplayers.c_str());
	SetVar( Cmd.m_Params[1], msStorePlayers, Event );
#endif

	return true;
}

//getplayersarray <arrayname>
//- scope: server
//- Tokenizes player ID's into an array, excluding bots, HLTV clients, and AFK's
bool CScript::ScriptCmd_GetPlayersArray(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//getplayersarray <arrayname>
	//Thothie MAR2010_16 - store players into an array
#ifdef VALVE_DLL
	msstring ArrName = Params[0];
	msscriptarray *                     pArray = m.pScriptedEnt->GetScriptedArray( ArrName, true );
	pArray->clearitems();

	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pOtherPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
		if ( !pOtherPlayer ) continue;
		if ( !pOtherPlayer->IsActive() ) continue;
		pArray->add( EntToString(pOtherPlayer) );
	}
#endif

	return true;
}

//getplayersnb <var>
//- scope: server
//- Tokenizes player ID's into a var, excluding bots, HLTV clients, and AFK's
bool CScript::ScriptCmd_GetPlayersNB(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//Thothie JUN2007a
	//- same as getplayers <token_string> - but doesn't return bots
	//- saves a step in the bot mess, but for better security, use the script side verifications too (see base_treasurechest)
#ifdef VALVE_DLL
	msstring msStorePlayers;
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pOtherPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
		if ( !pOtherPlayer )
		{
			//MessageBox(NULL,"NOT OPLAYER", "DEBUG POPUP",MB_OK|MB_ICONEXCLAMATION);
			continue;
		}

		if ( !pOtherPlayer->IsPlayer() )
		{
			//MessageBox(NULL,"NOT ISPLAYER", "DEBUG POPUP",MB_OK|MB_ICONEXCLAMATION);
			continue;
		}

		if ( !pOtherPlayer->IsActive() )
		{
			//MessageBox(NULL,"NOT CLIENT", "DEBUG POPUP",MB_OK|MB_ICONEXCLAMATION);
			continue;
		}

		msStorePlayers += EntToString(pOtherPlayer);
		msStorePlayers += ";";
	}
	//ALERT(at_console,"stored players %s", thoth_storeplayers.c_str());
	SetVar( Cmd.m_Params[1], msStorePlayers, Event );
#endif

	return true;
}

//giveexp <player> <skill> <amt>
//- scope: server
//- gives player <amt> XP in <skill>
bool CScript::ScriptCmd_GiveExp(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	// Ex:    giveexp PLAYER axehandling.prof 200
	//		  giveexp PLAYER bluntarms 100
	if ( Params.size() >= 3 )
	{
		CBaseEntity *pEnt = RetrieveEntity( Params[0] );
		CBasePlayer *pPlayer = pEnt->IsPlayer() ? (CBasePlayer *)pEnt : NULL;
		if ( pPlayer )
		{
			msstring Prop = Params[1];
			int amt = atoi( Params[2].c_str() );
			int Skill = -1, SubSkill = -1;
			if ( Prop.contains( "." ) )
			{
				msstring skill_name = Prop.thru_char( "." );
				Skill = GetSkillStatByName( skill_name );
				//Thothie NOV2017_30 - Greatguys 1 reports that .skip action is being problematic
				//SubSkill = GetSubSkillByName( Prop.skip( skill_name + "." ) );
				//new way:
				Print( "DEBUG: givexp %s %s\n", skill_name.c_str(), Prop.substr( skill_name.len()+1 ).c_str() );
				SubSkill = GetSubSkillByName( Prop.substr( skill_name.len()+1 ) );
			}
			else
				Skill = GetSkillStatByName( Prop );

			if( Skill > -1 )
			{
				if( SubSkill > -1 )
					pPlayer->LearnSkill( Skill , SubSkill , amt );
				else
					pPlayer->LearnSkill( Skill , amt );
			}
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//givehp [target] <amt>
//givemp [target] <amt>
//- scope: server
//- Give specifed amount of hp/mp to target
//- <amt> can be negative, but reducing HP below zero in this fashion may not trigger death events.
bool CScript::ScriptCmd_GiveHPMP(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	if( Params.size() >= 1 )
		//Must be in braces for logic
	{
		CBaseEntity *pTarget = m.pScriptedEnt;
		msstring_ref Amt = Params[0];
		if( Params.size() >= 2 ) { pTarget = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL; Amt = Params[1]; }

		if( pTarget ) pTarget->Give( Cmd.Name() == "givehp" ? GIVE_HP : GIVE_MP, atof(Amt) );
	}

	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//gravity <ratio>
//- scope: uncertain?
//- sets gravity for npc or item
bool CScript::ScriptCmd_Gravity(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 1)
		m.pScriptedEnt->pev->gravity = V_max(atof(Params[0]), 0.001f);
	else ERROR_MISSING_PARMS;
	return true;
}

// [g_]hashmap.create [Entity] <map_name>
// [g_]hashmap.set    [Entity] <map_name> <key> <value>
// [g_]hashmap.del    [Entity] <map_name> <key>
// [g_]hashmap.erase  [Entity] <map_name>
// [g_]hashmap.clear  [Entity] <map_name>
// [g_]hashmap.copy   [Entity] <src_map_name> <dest_map_name>
bool CScript::ScriptCmd_HashMap(
	SCRIPT_EVENT &                        Event
	, scriptcmd_t &                         Cmd
	, msstringlist &                        Params
	)
{
	size_t                              vParam = 0;
	msstring                            vsSubCmd;
	bool                                bGlobal = false;
	CBaseEntity *                       pEntity = NULL;
	msstring                            vsHashMapName;
	bool                                bExisted;
	if (Params.size())
	{
		vsHashMapName = Params[vParam++];
		if (Cmd.Name().starts_with("g_"))
		{
			bGlobal = true;
			vsSubCmd = Cmd.Name().substr(10);
		}
		else
		{
			vsSubCmd = Cmd.Name().substr(8);
			pEntity = RetrieveEntity(vsHashMapName);
			if (pEntity)
			{
				if (Params.size() > vParam)
				{
					vsHashMapName = Params[vParam++];
				}
				else
				{
					ERROR_MISSING_PARMS;
					return true;
				}
			}
		}

		if (!pEntity)
		{
			pEntity = m.pScriptedEnt;
		}

		msscripthashhash &              vHashMapHashMap = bGlobal ? mGlobalScriptHashes : pEntity->mScriptedHashes;
		msscripthash *                  pHashMap = GetScriptedHashMapFromHashMap(vHashMapHashMap
			, vsHashMapName
			, vsSubCmd == "create"
			, &bExisted
			);

		if (vsSubCmd != "create") // Create already done
		{
			if (!bExisted)
			{
				ErrorPrint("NO_HASHMAP"
					, ERRORPRINT_CONSOLE | ERRORPRINT_LOG | ERRORPRINT_CVAR
					, "HashMap %s doesn't exist for command %s"
					, vsHashMapName.c_str()
					, Cmd.Name().c_str()
					);
				return true;
			}

			if (vsSubCmd == "copy")
			{
				if (Params.size() > vParam)
				{
					msstring            vsDestHashMap = Params[vParam++];
					msscripthash *      pDestHashMap = m.pScriptedEnt->GetScriptedHashMap(vsDestHashMap
						, true
						);
					msscripthash::iterator
						iHashMap;
					for (iHashMap = pHashMap->begin()
						; iHashMap != pHashMap->end()
						; iHashMap++
						)
					{
						(*pDestHashMap)[iHashMap->first] = iHashMap->second;
					}
				}
				else ERROR_MISSING_PARMS;
			}
			else
			if (vsSubCmd == "set")
			{
				if (Params.size() > vParam + 1)
				{
					msstring            vsKey = Params[vParam++];
					msstring            vsValue = Params[vParam++];
					(*pHashMap)[vsKey] = vsValue;
				}
				else ERROR_MISSING_PARMS;
			}
			else
			if (vsSubCmd == "del")
			{
				if (Params.size() > vParam)
				{
					pHashMap->erase(Params[vParam++]);
				}
				else ERROR_MISSING_PARMS;
			}
			else
			if (vsSubCmd == "erase")
			{
				vHashMapHashMap.erase(vsHashMapName);
			}
			else
			if (vsSubCmd == "clear")
			{
				pHashMap->clear();
			}
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//helptip <player|all> <tipname|generic> <title> <text>
//- scope: server
//- Creates a helptip pop-up with green title text.
//- Each helptip is only supplied once, unless helptip data is cleared (unimplemented), or "generic" is used as type.
//- helptips can have multiple lines by parsing <text> with "|".
bool CScript::ScriptCmd_HelpTip(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	//<target|all> <tipname> <title> <text>
	//Thothie JAN2011_04 - modded to allow sending of helptip to all players
	if( Params.size() >= 4 )
	{
		MSGlobals::Buffer[0] = 0;
		for(int i = 0; i < Params.size() - 3; i++)
			strncat(MSGlobals::Buffer, Params[i + 3], Params[i + 3].len());
		if ( Params[0] != "all" )
		{
			CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity(Params[0]) : NULL;
			CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
			if ( pPlayer ) pPlayer->SendHelpMsg(Params[1], Params[2], MSGlobals::Buffer);
		}
		else
		{
			for( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBaseEntity *pEntity = UTIL_PlayerByIndex(i);
				CBasePlayer *pPlayer = (CBasePlayer *)pEntity;;
				if ( pPlayer ) pPlayer->SendHelpMsg(Params[1], Params[2], MSGlobals::Buffer);
			}
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//bleed <target> [color:red|green|yellow] [amt]
//- scope: server
//- cause target to bleed
//- Thothie DEC2014_13
bool CScript::ScriptCmd_bleed(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	if ( Params.size() >= 1 )
	{
		CBaseEntity *pEntity = m.pScriptedEnt->RetrieveEntity( Params[0] );

		Vector blood_origin = pEntity->pev->origin;
		blood_origin += Vector( RANDOM_FLOAT(-10,10), RANDOM_FLOAT(-10,10), RANDOM_FLOAT(-10,10) );

		Vector blood_direction = UTIL_RandomBloodVector();

		int blood_color = pEntity->BloodColor();
		if ( Params.size() >= 2 )
		{
			if( Params[1] == "red" ) blood_color = BLOOD_COLOR_RED;
			else if( Params[1] == "green" ) blood_color = BLOOD_COLOR_GREEN;
			else if( Params[1] == "yellow" ) blood_color = BLOOD_COLOR_YELLOW;
		}

		int blood_amt = 100;
		if ( Params.size() >= 3 ) blood_amt = atoi(Params[2]);

		//Thothie AUG2018_25, fixed red blood issues
		UTIL_BloodStream( blood_origin, blood_direction, (blood_color == BLOOD_COLOR_RED) ? 70 : blood_color, blood_amt );

		TraceResult tr;

		UTIL_TraceLine( blood_origin, blood_origin + Vector(0,RANDOM_FLOAT(-100,100),0) * blood_amt * 2, ignore_monsters, NULL, &tr );
		if ( tr.flFraction != 1.0 )
			UTIL_BloodDecalTrace( &tr, blood_color );
	}
	else ERROR_MISSING_PARMS;
#endif
	return true;
}

//hitmulti <target> <ratio>
//- changes <target>'s accuracy by <ratio> in dodamage events
bool CScript::ScriptCmd_HitMulti(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//Thothie FEB2008_18 - attempting to implement hit penalties for monsters/players
	//hitmulti <targ> <ratio>
#ifdef VALVE_DLL
	if( Params.size() >= 2 )
	{
		CBaseEntity *pEntity = m.pScriptedEnt->RetrieveEntity( Params[0] );
		if ( pEntity )
		{
			CMSMonster *pMonster = (CMSMonster *)pEntity;
			pMonster->m_HITMulti = atof(Params[1]);
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}
//hud. icon/image display commands
//- scope: server (client version needed/pending)
//hud.addstatusicon <player> <icon> <name> <duration> [isTGA:true|false] - adds a status icon with timer bar in corner of player's screen
//hud.addimgicon <player> <icon> <name> <%x> <%y> <%width> <%height> <duration> - adds a tga image to the player's hud
//hud.killicons <player> - removes all images and icons from player's hud
//hud.killstatusicon <player> <name> - removes specified status icon
//hud.killimgicon <player> <name> - removes specified image
//sprites must be in the msc/sprites directory, TGA's in the msc/gfx/vgui folder
//TGA's can have alpha layers, but odd ratio TGA's may not display proper
bool CScript::ScriptCmd_HudIcon(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//***************************** ADDSTATUSICON *************************
	//addstatusicon <target|all> <icon> <name> <duration> [isTGA]
	//Drigien APR2008
	if (Cmd.Name() == "hud.addstatusicon")
	{
#ifdef VALVE_DLL
		if( Params.size( ) >= 3 )
		{
			bool isTGA = false;
			if( Params.size() >= 5 ) isTGA = (Params[4] == "1") ? true : false;
			//Thothie JAN2013_08 note: isTGA is not working for status icons!
			//isTGA = (Params[3] == "true");
			//isTGA = strcmp(Params[3], "false");

			if ( Params[0] != "all" )
			{
				CBaseEntity *pEnt = m.pScriptedEnt->RetrieveEntity( Params[0] );
				CBasePlayer *pPlayer = pEnt && pEnt->IsPlayer() ? (CBasePlayer *)pEnt : NULL;

				if( pPlayer )
				{
					msstring &Icon = Params[1];
					msstring& Name = Params[2];
					MESSAGE_BEGIN(MSG_ONE, g_netmsg[NETMSG_STATUSICONS], NULL, pPlayer->pev);
					WRITE_SHORT(1);
					WRITE_STRING_LIMIT(Icon.c_str(), 85);  //Icon
					WRITE_STRING_LIMIT(Name.c_str(), 85);  //ID Name
					WRITE_FLOAT(atof(Params[3].c_str()));	//Duration
					WRITE_BYTE(isTGA);				//isTGA
					MESSAGE_END();
				}
			}
			else
			{
				for( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
					if ( !pPlayer ) continue;
					msstring &Icon = Params[1];
					msstring &Name = Params[2];
					MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_STATUSICONS], NULL, pPlayer->pev );
					WRITE_SHORT( 1 );
					WRITE_STRING_LIMIT(Icon.c_str(), 85);  //Icon
					WRITE_STRING_LIMIT(Name.c_str(), 85);  //ID Name
					WRITE_FLOAT( atof(Params[3].c_str()) );	//Duration
					WRITE_BYTE( isTGA );				//isTGA
					MESSAGE_END();
				}
			}
		}
		else ERROR_MISSING_PARMS;
#endif
	}
	//***************************** ADDIMGICON *************************
	//addimgicon <target> <icon> <name> <x> <y> <width> <height> <duration>
	//NOTE: USES TGA FILES ONLY!! Path Starts: msc/gfx/vgui/
	//Drigien MAY2008
	else if (Cmd.Name() == "hud.addimgicon")
	{
#ifdef VALVE_DLL
		if( Params.size( ) >= 8 )
		{
			CBaseEntity *pEnt = m.pScriptedEnt->RetrieveEntity( Params[0] );
			CBasePlayer *pPlayer = pEnt && pEnt->IsPlayer() ? (CBasePlayer *)pEnt : NULL;
			if( pPlayer )
			{
				msstring &Img = Params[1];
				msstring &Name = Params[2];
				MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_STATUSICONS], NULL, pPlayer->pev );
				WRITE_SHORT( 2 );
				WRITE_STRING_LIMIT(Img.c_str(), 80);  //Icon
				WRITE_STRING_LIMIT(Name.c_str(), 80);  //ID Name
				WRITE_SHORT( atoi(Params[3].c_str()) ); //X Pos
				WRITE_SHORT( atoi(Params[4].c_str()) ); //Y Pos
				WRITE_SHORT( atoi(Params[5].c_str()) ); //Width
				WRITE_SHORT( atoi(Params[6].c_str()) ); //Height
				WRITE_FLOAT( atof(Params[7].c_str()) );	//Duration
				MESSAGE_END();
			}
		}
		else ERROR_MISSING_PARMS;
#endif
	}
	//***************************** KILLICONS *************************
	//killicons <target|all>
	//Drigien MAY2008
	else if (Cmd.Name() == "hud.killicons")
	{
#ifdef VALVE_DLL
		if( Params.size( ) >= 1 )
		{
			if ( Params[0] != "all" )
			{
				CBaseEntity *pEnt = m.pScriptedEnt->RetrieveEntity( Params[0] );
				CBasePlayer *pPlayer = pEnt && pEnt->IsPlayer() ? (CBasePlayer *)pEnt : NULL;
				if( pPlayer )
				{
					MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_STATUSICONS], NULL, pPlayer->pev );
					WRITE_SHORT( 0 );
					MESSAGE_END();
				}
			}
			else
			{
				for( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
					if ( !pPlayer ) continue;

					MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_STATUSICONS], NULL, pPlayer->pev );
					WRITE_SHORT( 0 );
					MESSAGE_END();
				}
			}
		}
		else ERROR_MISSING_PARMS;
#endif
	}
	//***************************** KILLSTAUTSICONS *************************
	//killstatusicon <target|all> [icon id]
	//Drigien MAY2008
	else if (Cmd.Name() == "hud.killstatusicon")
	{
#ifdef VALVE_DLL
		if( Params.size( ) >= 1 )
		{
			CBaseEntity *pEnt = m.pScriptedEnt->RetrieveEntity( Params[0] );
			CBasePlayer *pPlayer = pEnt && pEnt->IsPlayer() ? (CBasePlayer *)pEnt : NULL;
			if( pPlayer )
			{

				MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_STATUSICONS], NULL, pPlayer->pev );
				WRITE_SHORT( -1 );
				if (Params.size() == 1)
					WRITE_STRING("all");
				else if (Params.size() >= 2)
					WRITE_STRING_LIMIT(Params[1].c_str(), WRITE_STRING_MAX);
				MESSAGE_END();
			}
		}
		else ERROR_MISSING_PARMS;
#endif
	}
	//***************************** KILLIMGICON *************************
	//killimgicon <target> [img id]
	//Drigien MAY2008
	else if (Cmd.Name() == "hud.killimgicon")
	{
#ifdef VALVE_DLL
		if( Params.size( ) >= 1 )
		{
			CBaseEntity *pEnt = m.pScriptedEnt->RetrieveEntity( Params[0] );
			CBasePlayer *pPlayer = pEnt && pEnt->IsPlayer() ? (CBasePlayer *)pEnt : NULL;
			if( pPlayer )
			{

				MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_STATUSICONS], NULL, pPlayer->pev );
				WRITE_SHORT( -2 );
				if (Params.size() == 1)
					WRITE_STRING("all");
				else if (Params.size() >= 2)
					WRITE_STRING_LIMIT(Params[1].c_str(), WRITE_STRING_MAX);
				MESSAGE_END();
			}
		}
		else ERROR_MISSING_PARMS;
#endif
	}
	/* FEB2009 - this can cause "msg '91' has not been sent yet" error
	//***************************** SETFNCONNECTION *************************
	//hud.setfnconnection <bool up>
	//Drigien MAY2008
	else if( Cmd.Name() == "hud.setfnconnection" )
	{
	#ifdef VALVE_DLL
	if( Params.size( ) >= 1 )
	{
	CBasePlayer *pPlayer = 0;

	for ( int i=1; i<=gpGlobals->maxClients; i++ )
	{
	pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
	if( pPlayer )
	{
	MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_STATUSICONS], NULL, pPlayer->pev );
	if( !strcmp( Params[1].c_str(),"true") )
	WRITE_SHORT( 3 );
	else
	WRITE_SHORT( -3 );
	MESSAGE_END();
	}
	}
	}
	else ERROR_MISSING_PARMS;
	#endif
	}
	*/
	return true;
}

//if
//- scope: shared, conditional, flow control
//- if [(] <conditional> [)] [command|newline]
//- <{>
//-		[commands]
//- <}>
//- [else] [<if> <(> <conditional> <)>]
//- <{>
//-		[commands]
//- <}>
//- [...else if repeatable...]
//-
//- also valid:
//- if <(> <conditional> <)> [command]
//- else [if <(> <conditional> <)>] [command]
//- [...else if repeatable...]
//-
//- If <conditional> is not in parantheses, and found false, script execution will exit one level at that point (or exit the event, if not embedded within another conditional).
//- else cannot be used with such terminating conditionals
//-
//- Conditional comparisons can be mathmatical or string based as follows:
//-- math:
//-- if x == y
//-- if x != y
//-- if x >= y
//-- if x <= y
//-- ( note that =< and => are not recognized.)
//-- string:
//-- if x equals y
//-- if x !equals y
//-- if x startswith y
//-- if x !startswith y
//-- if x contains y
//-- if x !contains y
//- Note that reserved vars are treated as literal strings, so:
//- if ent_laststruck equals ent_lastseen
//- will never be true, instead use:
//- if $get(ent_laststruck,id) equals $get(ent_lastseen,id)
//- Note that vectors are treated as strings, unless you break them to their component parts, eg.
//- if $vec.z(MY_POS) > $vec.z(TARG_POS) - is legit
//- if $get(ent_me,origin) == $get(ent_laststruck,origin) - is not legit (use 'equals')
//- uninitialized vars always return their litteral strings, so to find if a var is empty, one would use:
//- if THIS_VAR_IS_UNSET equals 'THIS_VAR_IS_UNSET'
//- all vars are strings until processed, so:
//- local L_TEST .123
//- if L_TEST == 0.123			true
//- if L_TEST equals 0.123		false
//- if L_TEST contains 123		true
//- if L_TEST startswith 0.		false
bool CScript::ScriptCmd_If(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//******************************* IF (cond) ***************************
	//Usage: if <varname> equals <value>
	//Usage: if <varname> isnot <value>

	bool ConditionsMet = false;
	if (Cmd.Params() == 1)
	{
		msstring Value = Params[0];

		bool Opposite = false;
		if (Params[0][0] == '!')
		{
			Opposite = true;
			Value = SCRIPTVAR(Params[0].substr(1));	//The '!' interferes with the default variable resolution, so remove it and resolve the variable again
		}

		ConditionsMet = atoi(Value) ? true : false;
		ConditionsMet = ConditionsMet ^ Opposite;
	}
	else if (Cmd.Params() >= 3)
	{
		int iCompareType = 0;

		msstring &CompareParam = Params[1];

		if (CompareParam == "equals") iCompareType = 0;
		else if (CompareParam == "isnot" || CompareParam == "!equals") iCompareType = 1; //AUG2013_15 Thothie - for consistancy with additions below
		else if (CompareParam == "<") iCompareType = 2;
		else if (CompareParam == ">") iCompareType = 3;
		else if (CompareParam == "<=") iCompareType = 4;
		else if (CompareParam == ">=") iCompareType = 5;
		else if (CompareParam == "==") iCompareType = 6;
		else if (CompareParam == "!=") iCompareType = 7;
		else if (CompareParam == "startswith") iCompareType = 8;
		else if (CompareParam == "contains") iCompareType = 9;
		else if (CompareParam == "!startswith") iCompareType = 10; //AUG2013_15 Thothie - sick of doing this with h4x
		else if (CompareParam == "!contains") iCompareType = 11; //AUG2013_15 Thothie - ditto

		msstring &CompareFromParam = Params[0], &CompareToParam = Params[2];
		if (iCompareType == 0)
			ConditionsMet = FStrEq(CompareFromParam, CompareToParam) ? true : false;
		else if (iCompareType == 1)
			ConditionsMet = !FStrEq(CompareFromParam, CompareToParam) ? true : false;
		else
		{
			float flFromValue = GetNumeric(CompareFromParam);
			float flToValue = GetNumeric(CompareToParam);
			if (iCompareType == 2)
				ConditionsMet = (flFromValue < flToValue);
			else if (iCompareType == 3)
				ConditionsMet = (flFromValue > flToValue);
			else if (iCompareType == 4)
				ConditionsMet = (flFromValue <= flToValue);
			else if (iCompareType == 5)
				ConditionsMet = (flFromValue >= flToValue);
			else if (iCompareType == 6)
				ConditionsMet = (flFromValue == flToValue);
			else if (iCompareType == 7)
				ConditionsMet = (flFromValue != flToValue);
			else if (iCompareType == 8)
			{
				//Thothie JUN2007a
				//$left(str,n) is being screwy, so seeing if can make:
				//if ( str startswith str )
				ConditionsMet = (CompareFromParam.starts_with(CompareToParam));
			}
			else if (iCompareType == 9)
			{
				//Thothie JUN2007a
				//if ( str contains str )
				ConditionsMet = (CompareFromParam.contains(CompareToParam));
			}
			else if (iCompareType == 10)
			{
				//Thothie JUN2007a
				//$left(str,n) is being screwy, so seeing if can make:
				//if ( str startswith str )
				ConditionsMet = (!CompareFromParam.starts_with(CompareToParam));
			}
			else if (iCompareType == 11)
			{
				//Thothie JUN2007a
				//if ( str contains str )
				ConditionsMet = (!CompareFromParam.contains(CompareToParam));
			}
		}
	}
	else
	{
		ConditionsMet = true;	//If I get a parameter error, just fall through like a normal command
		ERROR_MISSING_PARMS;
	}

	return ConditionsMet;
}

//infomsg <player|all> <title> <text>
//- scope: uncertain?
//- Creates a pop-up with red title text on the player's HUD
bool CScript::ScriptCmd_InfoMessage(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//NOV2014_11 comment: this looks as though it's setup to work on client or server, but doesn't look as though it'll work
	if (Params.size() >= 3)
	{
		msstring sTemp;
		bool SendToAll = false;
		if (Params[0] == "all") SendToAll = true;

		CBaseEntity *pEntity = NULL;
		if (!SendToAll)
#ifdef VALVE_DLL
			pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;
#else
			pEntity = (CBaseEntity *)&player;
#endif

		if (SendToAll || (pEntity && pEntity->IsPlayer()))
		{
			msstring Title = Params[1];
			if (Title.len() > 120)
			{
				Title = Title.substr(0, 120); //Thothie DEC2010_04 - prevent overflows
				Title += "*\n";
			}
			for(int i = 0; i < Params.size() - 2; i++)
			{
				if (i) sTemp += " ";
				sTemp += Params[i + 2];
			}
			if (sTemp.len() > 120)
			{
				sTemp = sTemp.substr(0, 120); //Thothie DEC2010_04 - prevent overflows
				sTemp += "*\n";
			}

#ifdef VALVE_DLL
			if( SendToAll )
				SendHUDMsgAll( Title, sTemp );
			else
#endif
				((CBasePlayer *)pEntity)->SendHUDMsg(Title, sTemp);
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//itemrestrict 0|1
//- scope: server, item
//- if enabled, item can only be picked up by players in item's PICKUP_ALLOW_LIST array
//- uncertain if this is used this way, seems an item simply having an array called "PICKUP_ALLOW_LIST" does the job.
bool CScript::ScriptCmd_ItemRestrict(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	//itemrestrict 0|1
	//- if enabled, item can only be picked up by players in item's PICKUP_ALLOW_LIST
	if ( Params.size() >= 1 )
	{
		CGenericItem *pItem = m.pScriptedEnt->IsMSItem() ? (CGenericItem *)m.pScriptedEnt : NULL;
		if ( pItem )
		{
			if ( Params[0] == "1" )
				SetBits(pItem->Properties,ITEM_NOPICKUP);
			else
				ClearBits(pItem->Properties,ITEM_NOPICKUP);
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//kill <player>
//- scope: server
//- kills a player without XP penalty (for hazards and the like)
bool CScript::ScriptCmd_Kill(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	//Thothie FEB2008_21 - Slay player without XP loss (for scripted hazards)
	if( Params.size() >= 1 )
	{
		CBaseEntity *pEntity = RetrieveEntity( Params[0] );
		CBasePlayer *pPlayer = pEntity->IsPlayer() ? (CBasePlayer *)pEntity : NULL;
		if ( pPlayer )
		{
			pPlayer->m_TimeTillSuicide = gpGlobals->time + 0.1;
			pPlayer->m_fNextSuicideTime = pPlayer->m_TimeTillSuicide + 0.5;
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

// MiB MAY2019_10
// markdmg <monster> <player> <stat[.prop]> <amount>
// Marks (fake) damage done to the monster by the player so they can receive exp
// Use for making summons work and receiving exp from support spells
bool CScript::ScriptCmd_MarkDmg(
	SCRIPT_EVENT &                        Event
	, scriptcmd_t &                         Cmd
	, msstringlist &                        Params
	)
{
	if (Params.size() > 3)
	{
		CBaseEntity *                   pTarget = RetrieveEntity(Params[0]);
		CBaseEntity *                   pSource = RetrieveEntity(Params[1]);
		msstring                        vsStat = Params[2];
		float                           vAmount = atof(Params[3]);

		CMSMonster *                    pTargetMonster = pTarget && pTarget->IsMSMonster() ? (CMSMonster *)pTarget : NULL;
		CBasePlayer *                   pSourcePlayer = pSource && pSource->IsPlayer() ? (CBasePlayer *)pSource : NULL;

		if (pTargetMonster && pSourcePlayer)
		{
			pTargetMonster->MarkDamage(pSourcePlayer
				, vsStat
				, vAmount
				);
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//simple math:
//- scope: shared
//decvar/dec <var> - decrease <var> by 1
//incvar/inc <var> - increase <var> by 1
//multiply <var> <amt> [full] - multiply <var> by <amt>
//divide <var> <amt> [full] - divide <var> by <amt>
//add <var> <amt> [full] - add <amt> to <var>
//subtract <var> <amt> [full] - subtract <amt> from <var>
//mod <var> <amt> [full] - mod <var> by <amt>
//- if "full" is added, as indicated, will attempt to return the full precision of the resulting float, otherwise, two places.
//- see also $math() function
bool CScript::ScriptCmd_MathSet(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 2)
	{
		int Operation = 0;
		if (Cmd.Name() == "decvar" || Cmd.Name() == "dec" || Cmd.Name() == "subtract") Operation = 1;
		else if (Cmd.Name() == "multiply") Operation = 2;
		else if (Cmd.Name() == "divide") Operation = 3;
		else if (Cmd.Name() == "mod") Operation = 4;

		float flValue = 0;
		flValue = atof(Params[0]);

		float Amount = atof(Params[1]);
		if (!Operation) flValue += Amount;
		else if (Operation == 1) flValue -= Amount;
		else if (Operation == 2) flValue *= Amount;
		else if (Operation == 3 && Amount) flValue /= Amount;
		else if (Operation == 4 && Amount) flValue = (int)flValue % (int)Amount;

		if (Params.size() <= 2)
			//No precision specified, assume 2 digits
			SetVar(Cmd.m_Params[1], UTIL_VarArgs("%.2f", flValue), Event);
		else
			SetVar(Cmd.m_Params[1], UTIL_VarArgs("%f", flValue), Event);	//used to be %.2f, but I decided to keep the precision
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//playermessage <player> <msg>
//- scope: server
//- Shows a white text message in the players event HUD (ie. the scrolling text box in the lower right).
//- note items have their own version of this, which assumes player owner is the target, and is also usable client side
//- (meaning, if you use playermessage ent_owner "I'm a sweet-ass item!" within an item script, the player will also get the "ent_owner" as part of the message.
//- these variants are not shared by items in the same way, but should be available to them, server side:
//rplayermessage <player> <msg> - red player message, as if damaged
//gplayermessage <player> <msg> - green player message, as if recieved XP
//bplayermessage <player> <msg> - blue player message (usually MP related)
//yplayermessage <player> <msg> - yellow(ish) player message, as if delt damage
//dplayermessage <player> <msg> - gray player message, as if disabled/failed
//...also...
//consolemsg <player> <msg> - sends message to player's console
bool CScript::ScriptCmd_Message(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	msstring sTemp;
	if (Params.size() >= 2)
	{
		CBaseEntity *pEntity = RetrieveEntity(Params[0]);
		if (pEntity && pEntity->IsPlayer())
		{
			for(int i = 0; i < Params.size() - 1; i++)
			{
				if (i) sTemp += " ";
				sTemp += Params[i + 1];
			}

			//Thothie MAR2008a - client message limit is 192
			if (sTemp.len() > 140)
			{
				msstring stupid_string = sTemp.substr(0, 140);
				sTemp = stupid_string;
				sTemp += "*\n";
			}

			if (Cmd.Name() == "playermessage")
			{
				((CBasePlayer*)pEntity)->SendInfoMsg("%s\n", sTemp.c_str());
			}
			//Thothie playermessage red
			else if (Cmd.Name() == "rplayermessage")
			{
				if (sTemp.len()) sTemp += "\n";
				((CBasePlayer *)pEntity)->SendEventMsg(HUDEVENT_ATTACKED, sTemp);
			}
			//Thothie playermessage green
			else if (Cmd.Name() == "gplayermessage")
			{
				if (sTemp.len()) sTemp += "\n";
				((CBasePlayer *)pEntity)->SendEventMsg(HUDEVENT_GREEN, sTemp);
			}
			//Thothie playermessage blue
			else if (Cmd.Name() == "bplayermessage")
			{
				if (sTemp.len()) sTemp += "\n";
				((CBasePlayer *)pEntity)->SendEventMsg(HUDEVENT_BLUE, sTemp);
			}
			//Thothie playermessage yellow'ish
			else if (Cmd.Name() == "yplayermessage")
			{
				if (sTemp.len()) sTemp += "\n";
				((CBasePlayer *)pEntity)->SendEventMsg(HUDEVENT_ATTACK, sTemp);
			}
			//Thothie playermessage darkgray
			else if (Cmd.Name() == "dplayermessage")
			{
				if (sTemp.len()) sTemp += "\n";
				((CBasePlayer *)pEntity)->SendEventMsg(HUDEVENT_UNABLE, sTemp);
			}
			else if (Cmd.Name() == "consolemsg")
			{
				if (sTemp.len()) sTemp += "\n"; //Thothie - console messages not adding car returns
				ClientPrint(pEntity->pev, at_console, sTemp);
			}
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//messageall <color:w|r|b|g|y|d> <message>
//- scope: server
//- as Xplayermessage above, but sends to all player's HUDs.
bool CScript::ScriptCmd_MessageAll(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//messageall <color:w|r|b|g|y|d> <message>
	//Thothie FEB2008a
	msstring msTemp;
#ifdef VALVE_DLL
	if( Params.size( ) >= 2 )
	{
		msstring msColorString = Params[0];

		for(int i = 0; i < Params.size()-1; i++)
		{
			if( i ) msTemp += " ";
			msTemp += Params[i+1];
		}

		for(int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );

			if ( !pPlayer ) continue;

			if ( msColorString.starts_with("r") ) pPlayer->SendEventMsg( HUDEVENT_ATTACKED, msTemp );
			else if ( msColorString.starts_with("b") ) pPlayer->SendEventMsg( HUDEVENT_BLUE, msTemp );
			else if ( msColorString.starts_with("g") ) pPlayer->SendEventMsg( HUDEVENT_GREEN, msTemp );
			else if ( msColorString.starts_with("y") ) pPlayer->SendEventMsg( HUDEVENT_ATTACK, msTemp );
			else if ( msColorString.starts_with("d") ) pPlayer->SendEventMsg( HUDEVENT_UNABLE, msTemp );
			else pPlayer->SendEventMsg( HUDEVENT_NORMAL, msTemp );
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//movetype <projectile|normal>
//- scope: server
//- Depreciated, only used by projectiles (and thus should probably be moved to base item commands).
//- use setprop <target> movetype <#>
bool CScript::ScriptCmd_MoveType(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	if( Params.size() >= 1 )
	{
		if( Params[0] == "projectile" )
			m.pScriptedEnt->MSMoveType = MOVETYPE_ARROW;
		else
			m.pScriptedEnt->MSMoveType = MOVETYPE_NORMAL;
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//name <string>
//- scope: shared
//- if file pipe is used, becomes the name prefix, (eg. "a|bat" "some|apples" "a murder of|crows")
bool CScript::ScriptCmd_Name(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	msstring sTemp;
	if (Params.size() >= 1)
	{
		if (m.pScriptedEnt && !m.pScriptedEnt->IsPlayer())	//Don't rename players
		{
			for(int i = 0; i < Params.size(); i++)
				sTemp += (i ? msstring(" ") : msstring("")) + Params[i];

			int barloc = 0;
			msstring Prefix, Name = sTemp;
			if ((barloc = sTemp.find("|")) != msstring_error)
			{
				Prefix = sTemp.substr(0, barloc);
				Name = sTemp.substr(barloc + 1);
				//Thothie APR2011_28 - buffer overrun prevention
				if ((Name.len() + barloc) > 39)
				{
					Name = Name.substr(0, 39 - barloc);
					IScripted *pScripted = m.pScriptedEnt->GetScripted();
					Print("WARNING: Name too long [%s]!\n", pScripted->m_Scripts[0]->m.ScriptFile.c_str());
				}
				//Thothie OCT2016_22 - add indicator to modded items (fail)
				/*
				if ( m.pScriptedEnt->IsMSItem() )
				{
				CBaseEntity *pEnt = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;
				if ( pEnt )
				{
				CGenericItem *pItem = pEnt->IsMSItem() ? (CGenericItem *)pEnt : NULL;
				if ( pItem )
				{
				if ( FBitSet(pItem->Properties,ITEM_MODDED) ) Name.append("*");
				}
				}
				}
				*/
				m.pScriptedEnt->DisplayPrefix = GetScriptVar(Prefix);
			}

			if (m.pScriptedEnt->pev && !m.pScriptedEnt->pev->netname)
			{
				//m.pScriptedEnt->pev->netname = ALLOC_STRING( msstring("¯") + Name );
				m.pScriptedEnt->m_NetName = msstring("¯") + Name;
				m.pScriptedEnt->pev->netname = MAKE_STRING(m.pScriptedEnt->m_NetName.c_str());
			}
			m.pScriptedEnt->m_DisplayName = GetScriptVar(Name);
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//name_prefix <string>
//- scope: shared
//- Depreciated. Changes the name prefix - now set by name, above.
bool CScript::ScriptCmd_NamePrefix(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 1)
	{
		if (m.pScriptedEnt) m.pScriptedEnt->DisplayPrefix = Params[0];
	}

	else ERROR_MISSING_PARMS;

	return true;
}

//name_unique <string>
//- scope: server
//- Provides a unique name for use with $get_by_name(<name>,<property>)
//- If this entity is removed, and a new one created with the same unique name, it may not reassign.
//- Similar issues occur, if duplicate NPC's are created with the same unique name.
bool CScript::ScriptCmd_NameUnique(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 1)
	{
		if (m.pScriptedEnt)
		{
			//m.pScriptedEnt->pev->netname = ALLOC_STRING( msstring("¯") + Params[0] );
			m.pScriptedEnt->m_NetName = msstring("¯") + Params[0];
			m.pScriptedEnt->pev->netname = MAKE_STRING(m.pScriptedEnt->m_NetName.c_str());
		}
	}	//Need braces

	else ERROR_MISSING_PARMS;

	return true;
}

//npcmove <target> [movetarget]
//- scope: server
//- Attempts to move <target> to scriptvar reg.npcmove.endpos
//- Setting reg.npcmove.testonly 1 causes the move to be virtual (just tests)
//- Returns: game.ret.npcmove.success (0|1) game.ret.npcmove.dist (distance moved before stopped) game.ret.npcmove.hitstep (had to step up during movement)
//- Mostly used for seeing of mobs are stuck after teleporting - not *quite* 100% fail safe, due to client movement prediction, but fairly close.
bool CScript::ScriptCmd_NpcMove(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	if( Params.size() >= 1 )
	{
		CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;
		if( pEntity && pEntity->IsMSMonster() )
		{
			CMSMonster *pMonster = (CMSMonster *)pEntity;

			moveexec_t MoveExec;
			clrmem( MoveExec );

			MoveExec.vecStart = pMonster->pev->origin;
			MoveExec.vecEnd = StringToVec(GetVar( "reg.npcmove.endpos" ));
			MoveExec.fTestMove = atoi(GetVar( "reg.npcmove.testonly" )) ? true : false;
			if( Params.size() >= 2 ) MoveExec.pTarget = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[1] ) : RetrieveEntity( Params[1] );


			bool Success = pMonster->MoveExecute( MoveExec );

			Event.SetVar( "game.ret.npcmove.success", Success ? "1" : "0"  );
			Event.SetVar( "game.ret.npcmove.dist", UTIL_VarArgs("%.2f",MoveExec.Return_DistMoved) );
			Event.SetVar( "game.ret.npcmove.hitstep", MoveExec.Return_HitStep ? "1" : "0" );
		}
	}
	else ERROR_MISSING_PARMS;

#endif
	return true;
}

//setorigin <target> <vec>
//addorigin <target> <vec>
//- scope: server
//- teleports <target> to <vec>
bool CScript::ScriptCmd_Origin(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//Parameters: <target> <origin>
	if (Params.size() >= 2)
	{
		if (m.pScriptedEnt)
		{
			CBaseEntity* pEntity = m.pScriptedEnt->RetrieveEntity(Params[0]);
			if (pEntity)
			{
				if (Cmd.Name() == "setorigin")
					pEntity->pev->origin = StringToVec(Params[1]);
				else
					pEntity->pev->origin += StringToVec(Params[1]);
			}
		}
	}
	else ERROR_MISSING_PARMS;

	return false;
}

//overwritespell <player> <spellidx> <new_spell_script>
//- scope: server
//- Overwrites an existing spell with <new_spell_script> (eg. magic_hand_poison)
bool CScript::ScriptCmd_OverwriteSpell(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//MiB Dec2007a - for overwriting a learned spell with another
	//Easily made into a system where NPCs upgrade your spells for a quest/fee
#ifdef VALVE_DLL
	//overwritespell <player> <spellidx> <spell>
	// <spell> should be in the format magic_hand_poison
	if( Params.size() >= 3 )
	{
		CBaseEntity *pEnt = RetrieveEntity( Params[0] );
		CBasePlayer *pPlayer = pEnt->IsPlayer() ? (CBasePlayer *)pEnt : NULL;

		if( pPlayer )
		{
			int idx = atoi( Params[1] );
			pPlayer->m_SpellList[idx] = Params[2];

			MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_SETPROP], NULL, pPlayer->pev );
			WRITE_BYTE( PROP_SPELL );
			WRITE_BYTE( 1 );
			WRITE_BYTE( idx );
			WRITE_STRING_LIMIT(Params[2].c_str(), WRITE_STRING_MAX);
			MESSAGE_END();
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//playername <player> <newname>
//- scope: uncertain?
//- Changes player's name.
bool CScript::ScriptCmd_PlayerName(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//playername <target> <string> - sets a player's name
	//NOV2014_11 - Runs both client and server, but looks like it wouldn't work client side
	if (Params.size() > 1)
	{
		msstring sTemp;
		CBaseEntity *pEntity = RetrieveEntity(Params[0]);

		if (pEntity->IsPlayer())
		{
			sTemp = Params[1];
			//Print("Setting name %s on %s\n", sTemp.c_str(), pEntity->m_DisplayName.c_str() );
			pEntity->m_DisplayName = sTemp;
			g_engfuncs.pfnSetClientKeyValue(pEntity->entindex(), g_engfuncs.pfnGetInfoKeyBuffer(pEntity->edict()), "name", (char *)pEntity->m_DisplayName);
			(pEntity->DisplayName());
			pEntity->m_NetName = pEntity->DisplayName();
			pEntity->pev->netname = MAKE_STRING(pEntity->m_NetName.c_str());
#ifdef VALVE_DLL
			MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_SETPROP], NULL, pEntity->pev );
			WRITE_BYTE(PROP_NAME); // 0 for title, 1 for name. More possibly to come.
			WRITE_STRING_LIMIT(pEntity->DisplayName(), WRITE_STRING_MAX);
			MESSAGE_END();
#endif
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//playertitle <player> <title>
//- scope: uncertain?
//- Changes player's title. (May not stick as of DEC2014 - need 2 fix.)
bool CScript::ScriptCmd_PlayerTitle(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//playertitle <target> <string> - sets a player's title
	//NOV2014_11 comment: runs both client and server but doesn't look like it'd work client side
	if (Params.size() > 1)
	{
		CBaseEntity *pEntity = RetrieveEntity(Params[0]);
		if (pEntity->IsPlayer())
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
			pPlayer->CustomTitle = Params[1];
#ifdef VALVE_DLL
			MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_SETPROP], NULL, pPlayer->pev );
			WRITE_BYTE(PROP_TITLE); // 0 for title, 1 for name. More possibly to come.
			WRITE_STRING_LIMIT(pPlayer->CustomTitle, WRITE_STRING_MAX);
			MESSAGE_END();
#endif
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}


//playmp3 <playerId|all> <system|combat|stop> <file>
//- scope: server
bool CScript::ScriptCmd_PlayMP3(SCRIPT_EVENT& Event, scriptcmd_t& Cmd, msstringlist& Params)
{
#ifdef VALVE_DLL

	msstring& Target = Params[0];
	msstring sMode = Params[1];
	msstring SFile = Params[2];

	int iMode;
	if (sMode == "system") iMode = MUSIC_SYSTEM;
	else if (sMode == "combat") iMode = MUSIC_COMBAT;
	else iMode = MUSIC_STOP_COMBAT;

	int i,max;
	if (Target.starts_with("all"))
	{
		i = 1;
		max = gpGlobals->maxClients;

		if (iMode != MUSIC_STOP_COMBAT)
		{
			MSGlobals::AllMusicMode = iMode;
			MSGlobals::AllMusic = SFile;
		}
	}
	else
	{
		MSGlobals::AllMusic.clear();
		i = RetrieveEntity(Target)->entindex();
		max = i;
	}

	do
	{
		CBaseEntity* targ = UTIL_PlayerByIndex(i);
		if (targ && targ->IsPlayer())
		{
			((CBasePlayer*)targ)->SwapMusic(-1, iMode, static_cast<std::string>(SFile));
		}

		i++;
	} while (i <= max);
#endif

	return true;
}

//playsound <chan> <volume> <sound> [attenuation:0.8] [pitch:100]
//playrandomsound <chan> <volume> [sounds...]
//svplaysound <chan> <volume> <sound> [attenuation:0.8] [pitch:100]
//svplayrandomsound <chan> <volume> [sounds...]
//- scope: server
//- Though initiated by server, playsound commands not prefixed by sv are actually played by the client and thus do not need to be precached
//- Sounds intended to follow an entity, or loop, must be played server side via svplaysound/svplayrandomsound.
//- Server side sounds can be "fake precached" by playing them at zero volume within the script's load phase, or inside { game_precache.
//- Looping sounds can be stopped by playing them on the same channel at 0 volume.
//- Sound labeled "none" are ignored.
//- These commands also accept older formats, where <volume> is ommited.
bool CScript::ScriptCmd_PlaySound(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 2)
	{
		//NOV2014_18 Thothie - cleaned this up a bit
		//Old way: playsound <chan> <sound>
		//New way: playsound <chan> <volume> <sound>
		if (m.pScriptedEnt && m.pScriptedEnt->edict())
		{
			//Thoth AUG2007a something about cavebat death causes crash
			//- many attempts to fix the calruin bug here
			//- double redundant fix, here, and in util.h::EMIT_SOUND
			if (m.pScriptedEnt->SndVolume < 0) m.pScriptedEnt->SndVolume = 0;
			if (m.pScriptedEnt->SndVolume > 1) m.pScriptedEnt->SndVolume = 1;

			int iChannel = atoi(Params[0]);
			int NextParm = 1;
			float Volume = -1;

			if (isdigit(Params[1][0]))
			{
				Volume = atof(Params[1]) / 10.0f;
				if (Volume > 1) Volume = 1.0; //Thothie - AUG2007a - Weirdness causing volume to be > 10 sometimes
				if (Volume < 0) Volume = 0.0; //- precautionary
				NextParm++;
			}

			msstring_ref pszSound = ((signed)Params.size() > NextParm) ? Params[NextParm].c_str() : "common/null.wav";

			//Thothie debugary
			/*
			if ( Volume > 1 )
			{
			foreach( i, Params.size( ) )
			sTemp += (i ? msstring(" ") : msstring("")) + Params[i];
			logfile << "PLAYSOUND_TOO_LOUD [" << STRING(m.pScriptedEnt ? m.pScriptedEnt->DisplayName() : "(No Entity)") << "|" << sTemp.c_str() << "]" << endl;
			}
			*/
			bool bServerSideSound = false;
			if (Cmd.Name().starts_with("sv"))
			{
				bServerSideSound = true;
			}

			if (Cmd.Name() == "playrandomsound" || Cmd.Name() == "svplayrandomsound")
			{
				pszSound = Params[NextParm + RANDOM_LONG(0, Params.size() - (Volume > -1 ? 3 : 2))];
			}

			//Todo: Allow changing pitch/attenuation with additional parameters on playsound/svplaysound
			float sAttn = ATTN_NORM;
			float sPitch = PITCH_NORM;

			if (!Cmd.Name().contains("random"))
			{
				if (Params.size() > 3)
				{
					sAttn = atof(Params[3]);
				}

				if (Params.size() > 4)
				{
					sPitch = atof(Params[4]);
				}
			}

			if (!FStrEq(pszSound, "none")) //skip over 'none'
			{
				if (Volume > -1)
				{
					if (Volume)
					{
						if (bServerSideSound)
						{
							EMIT_SOUND2(m.pScriptedEnt->edict(), iChannel, pszSound, Volume, sAttn, sPitch);
						}
						else
						{
							/*
							msstringlist Parameters;
							Parameters.clearitems();
							Parameters.add(UTIL_VarArgs("%i",iChannel));
							Parameters.add(UTIL_VarArgs("%f",Volume*10));
							Parameters.add(pszSound);
							Parameters.add(VecToString(m.pScriptedEnt->pev->origin));
							ClCallScriptPlayers( "game_cl_playsound", Parameters.size() ? &Parameters : NULL );
							*/
							ClXPlaySoundAll(pszSound, m.pScriptedEnt->pev->origin, iChannel, Volume, sAttn, sPitch);
						}
					}
					else
					{
						//volume 0 = stop the channel
						if (bServerSideSound)
						{
							EMIT_SOUND(m.pScriptedEnt->edict(), iChannel, "common/null.wav", 0.001f, sAttn);
						}
						else
						{
							/*
							msstringlist Parameters;
							Parameters.clearitems();
							Parameters.add(UTIL_VarArgs("%i",iChannel));
							Parameters.add("0");
							Parameters.add(pszSound);
							Parameters.add(VecToString(m.pScriptedEnt->pev->origin));
							ClCallScriptPlayers( "game_cl_playsound", Parameters.size() ? &Parameters : NULL );
							*/
							ClXPlaySoundAll(pszSound, m.pScriptedEnt->pev->origin, iChannel, 0.001f, sAttn, sPitch);
						}
					} //end if vol 0
				} //end if vol > -1
			} //end if sound is none
		} //end if scriptedent
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//precachefile <scriptname>
//- scope: uncertain?
//- Precache all media within another script at load time.
//- This command should only appear inside the { game_precache event.
bool CScript::ScriptCmd_PrecacheFile(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 1)
	{
		msstring &ScriptName = Params[0];
		bool PrecacheOnly = true;
		if (Params.size() >= 2)
		{
			if (Params[0] == "[full]") PrecacheOnly = false;
			ScriptName = Params[1];
		}

		CScript TempScript;
		bool fSuccess = TempScript.Spawn(ScriptName, NULL, NULL, PrecacheOnly);
		if (fSuccess) { if (!PrecacheOnly) TempScript.RunScriptEventByName("game_precache"); }
		else ALERT(at_console, "Script Erorr (%s), %s - '%s' possible file not found!\n", m.ScriptFile.c_str(), Cmd.Name().c_str(), ScriptName.c_str());
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//projectilesize <units>
//- scope: uncertain?
//- Should define the size of a projectile (and thus should be moved to generic item commands).
//- Uncertain if functional, however.
bool CScript::ScriptCmd_ProjectileSize(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 1)
		m.pScriptedEnt->m_Volume = atof(Params[0]);
	else ERROR_MISSING_PARMS;
	return true;
}

//quest <set|unset|dump> <player> <quest_name> <value>
//- scope: server
//- Sets or removes quest data on a player
//- Quest data saves with characters, and can be retrieved via $get_questdata(<player>,<quest_name>)
//- This can thus be used to write most any information to the character for later use.
//- For a list of used quest data, see the comments at the top of the script, player/player_main.script
//- dump sends all quest data to console (may overflow)
bool CScript::ScriptCmd_Quest(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	//<set/unset> <target> <name> <data>
	if( Params.size() >= 3 )
	{
		CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[1] ) : NULL;
		if( pEntity && pEntity->IsPlayer() )
		{
			//Thothie attempting to fix quest data in hopes it can be used for map security
			CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
			msstring &Action = Params[0];
			msstring &Name = Params[2];
			msstring &Data = Params[3]; //Params.size() >= 4 ? Params[3] : NULL;
			//ALERT( at_aiconsole, "Got Quest String1(%s) String2(%s) String3(%s) \n", Params[0], Params[2], Params[3]);
			bool SetData = true;
			if( Action == "unset" ) SetData = false;
			if( Action == "clear" )
			{
				pPlayer->m_Quests.clearitems();
				pPlayer->m_Quests.clear();
			}

			if( !SetData || Params.size() >= 4 )
			if( Action != "clear" ) pPlayer->SetQuest( SetData, Name, Data );
		}
	}
	else if ( Params.size() == 2 )
	{
		//Thothie JAN2013_20 (post JAN2013 patch) - Allow easy dumping of quest data
		if ( Params[0] == "dump" )
		{
			CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[1] ) : NULL;
			if( pEntity && pEntity->IsPlayer() )
			{
				CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
				msstring qd_outline;
				qd_outline = UTIL_VarArgs("Dumping Quest Data for %s:\n",pEntity->m_DisplayName.c_str());
				Print ("%s",qd_outline.c_str());
				logfile << qd_outline.c_str();
				for(int i = 0; i < pPlayer->m_Quests.size(); i++)
				{
					qd_outline = UTIL_VarArgs("#%i name: %s data: %s\n",i,pPlayer->m_Quests[i].Name.c_str(),pPlayer->m_Quests[i].Data.c_str());
					Print ("%s",qd_outline.c_str());
					logfile << qd_outline.c_str();
				}
			}
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//registerdefaults
//- scope: uncertain
//- Sets various player defaults. Runs once at game start in global.script
bool CScript::ScriptCmd_RegisterDefaults(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//Reset old
	MSGlobals::DefaultWeapons.clearitems();
	MSGlobals::DefaultFreeItems.clearitems();

	TokenizeString(SCRIPTVAR("reg.newchar.weaponlist"), MSGlobals::DefaultWeapons);
	TokenizeString(SCRIPTVAR("reg.newchar.freeitems"), MSGlobals::DefaultFreeItems);
	MSGlobals::DefaultGold = atoi(SCRIPTVAR("reg.newchar.gold"));
	MSGlobals::DefaultSpawnBoxModel = SCRIPTVAR("reg.hud.spawnbox");
#ifdef VALVE_DLL
	PRECACHE_MODEL( MSGlobals::DefaultSpawnBoxModel );
#else
	MSCLGlobals::DefaultHUDCharAnims.Idle_Weapon = SCRIPTVAR("reg.hud.char.active_weapon");
	MSCLGlobals::DefaultHUDCharAnims.Idle_NoWeapon = SCRIPTVAR("reg.hud.char.active_noweap");
	MSCLGlobals::DefaultHUDCharAnims.Fidget = SCRIPTVAR("reg.hud.char.figet");
	MSCLGlobals::DefaultHUDCharAnims.Highlighted = SCRIPTVAR("reg.hud.char.highlight");
	MSCLGlobals::DefaultHUDCharAnims.Uploading = SCRIPTVAR("reg.hud.char.upload");
	MSCLGlobals::DefaultHUDCharAnims.Inactive = SCRIPTVAR("reg.hud.char.inactive");

	MSCLGlobals::DefaultHUDSounds.QuickSlot_Select = SCRIPTVAR("reg.hud.quickslot.select");
	MSCLGlobals::DefaultHUDSounds.QuickSlot_Confirm = SCRIPTVAR("reg.hud.quickslot.confirm");
	MSCLGlobals::DefaultHUDSounds.QuickSlot_Assign = SCRIPTVAR("reg.hud.quickslot.assign");

	MSCLGlobals::DefaultHUDCoords.ItemDesc_X = atof(SCRIPTVAR("reg.hud.desctext.x"));
	MSCLGlobals::DefaultHUDCoords.ItemDesc_Y = atof(SCRIPTVAR("reg.hud.desctext.y"));
#endif

	return true;
}

//registereffect
//- scope: uncertain?
//- registers an applyeffect's properties
bool CScript::ScriptCmd_RegisterEffect(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	globalscripteffect_t Effect;

	Effect.m_Name = SCRIPTVAR("reg.effect.name");
	Effect.m_ScriptName = SCRIPTVAR("reg.effect.script");

	msstring Flags = SCRIPTVAR("reg.effect.flags");

	Effect.m_Flags = SCRIPTEFFECT_NORMAL;
	if( Flags.contains("player_action") )	SetBits( Effect.m_Flags, SCRIPTEFFECT_PLAYERACTION );
	if( Flags.contains("nostack") )			SetBits( Effect.m_Flags, SCRIPTEFFECT_NOSTACK );

	CGlobalScriptedEffects::RegisterEffect( Effect );
#endif

	return true;
}

//registerrace
//- scope: uncertain?
//- registers a race, used repeatedly in races.script
bool CScript::ScriptCmd_RegisterRace(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	race_t NewRace;
	NewRace.Name = SCRIPTVAR("reg.race.name");
	TokenizeString( SCRIPTVAR("reg.race.enemies"), NewRace.Enemies );
	TokenizeString( SCRIPTVAR("reg.race.allies"), NewRace.Allies );
	TokenizeString( SCRIPTVAR("reg.race.wary"), NewRace.Wary );

	CRaceManager::AddRace( NewRace );
#endif

	return true;
}

//registertexture
//- scope: shared
//- registers reflective textures and their properties, usually in <mapname>/map_startup
bool CScript::ScriptCmd_RegisterTexture(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifndef VALVE_DLL
	mstexture_t NewTexture;
	clrmem(NewTexture);

	//Load settings
	NewTexture.Name = SCRIPTVAR("reg.texture.name");

	msstringlist ColorParts;
	NewTexture.IsReflective = atoi(SCRIPTVAR("reg.texture.reflect")) ? true : false;
	NewTexture.IsWater = atoi(SCRIPTVAR("reg.texture.water")) ? true : false;

	//Reflection settings
	NewTexture.Mirror.Blending = atoi(SCRIPTVAR("reg.texture.reflect.blend")) ? true : false;
	TokenizeString(SCRIPTVAR("reg.texture.reflect.color"), ColorParts);
	for(int i = 0; i < ColorParts.size(); i++)
	{
		if (i == 4) break;	//Too many elements specified - a color only has 4 elements
		NewTexture.Mirror.Color[i] = atof(ColorParts[i]);
	}
	NewTexture.Mirror.Blending = atoi(SCRIPTVAR("reg.texture.reflect.blend")) ? true : false;
	NewTexture.Mirror.Range = atof(SCRIPTVAR("reg.texture.reflect.range"));
	if (VarExists("reg.texture.reflect.world"))	//Check var existence, because the default is "1"
		NewTexture.Mirror.NoWorld = !atoi(SCRIPTVAR("reg.texture.reflect.world"));
	if (VarExists("reg.texture.reflect.ents"))
		NewTexture.Mirror.NoEnts = !atoi(SCRIPTVAR("reg.texture.reflect.ents"));

	MSCLGlobals::Textures.add(NewTexture);
#endif

	return true;
}

//registertitle
//- scope: uncertain?
//- registers possible player titles based on weapon skills under the default title system
bool CScript::ScriptCmd_RegisterTitle(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 1)
	{
		if (Params.size() < 2)
		{	//Default title
			CTitleManager::DefaultTitle.Name = Params[0];
			CTitleManager::DefaultTitle.MinLevel = 0;
		}
		else
		{	//Specific title
			title_t NewTitle;
			NewTitle.Name = Params[0];
			NewTitle.MinLevel = atoi(GetScriptVar("TITLE_MINSKILL"));
			static msstringlist Skills;
			Skills.clearitems();
			TokenizeString(Params[1], Skills);
			bool SkillSuccess = true;
			for(int s = 0; s < Skills.size(); s++)
			{
				int Skill = GetSkillStatByName(SCRIPTVAR(Skills[s]));
				if (Skill == -1)
				{
					SkillSuccess = false;
					break;
				}
				NewTitle.SkillsReq.add(Skill);
			}

			if (SkillSuccess)
				CTitleManager::AddTitle(NewTitle);
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//removeeffect <target> <game.effect.id>
//- scope: server
//- removes an effect on a target based on its EFFECT_ID.
//- See individual effect scripts to get EFFECT_ID (for instance, effects/effect_ice_shield's is "effect_iceshield").
//- Beware removing certain effects prematurely may cause issues
bool CScript::ScriptCmd_RemoveEffect(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	//removeeffect <target> <game.effect.id>
	//MiB FEB2010_28
	if ( Params.size() >= 2 )
	{
		//Thothie JAN2013_05 - Not sure why this doesn't work, going to try a variant
		/*
		(CBaseEntity *pTarget = RetrieveEntity( Params[0] );
		if ( pTarget )
		{
		IScripted *pTargetScript = pTarget->GetScripted();
		foreach ( i, pTargetScript->m_Scripts.size() )
		{
		if ( pTargetScript->m_Scripts[i]->GetVar("game.effect.id") == Params[1] )
		{
		pTargetScript->m_Scripts[i]->RunScriptEventByName( "effect_die" );
		pTargetScript->m_Scripts[i]->m.RemoveNextFrame = true;
		}
		}
		}
		*/
		//new... (works) - suspect the .c_str() in strcmp is all it really needed
		CBaseEntity *pTarget = RetrieveEntity( Params[0] );
		IScripted *pScripted = pTarget ? pTarget->GetScripted() : NULL; // UScripted? IScripted.
		if ( pScripted )
		{
			for(int i = 0; i < pScripted->m_Scripts.size(); i++) // Check each
			if( pScripted->m_Scripts[i]->VarExists("game.effect.id") ) //This is an effect
			if( strcmp( pScripted->m_Scripts[i]->GetVar("game.effect.id"), Params[1].c_str() ) == 0) //If the effect is SUPPOSED to be removed
			{
				pScripted->m_Scripts[i]->RunScriptEventByName( "effect_die" ); //Call this effect's die function
				pScripted->m_Scripts[i]->m.RemoveNextFrame = true;
			}
		}
	} else ERROR_MISSING_PARMS;
#endif

	return true;
}

//removescript
//- scope: shared
//- Removes script on next frame.
//- This is usually the best way to remove client side scripts, but be sure any sprites or lights they are using have a chance to expire.
bool CScript::ScriptCmd_RemoveScript(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (strcmp(GetVar("game.effect.id"), "game.effect.id") != 0) RunScriptEventByName("effect_die");
	m.RemoveNextFrame = true;
	return true;
}

//repeatdelay <secs>
//- scope: shared, flow control
//- Causes the event to repeat every <secs>.
//- Beware that whether the event is called or not, or the NPC is alive or not, has no baring on whether it'll repeat.
bool CScript::ScriptCmd_RepeatDelay(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 1)
	{
		Event.fRepeatDelay = atof(Params[0]);
		Event.fNextExecutionTime = gpGlobals->time + Event.fRepeatDelay;
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//respawn <player|all>
//- scope: server
//Sends players to their current ms_player_spawn.
bool CScript::ScriptCmd_Respawn(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	if( Params.size( ) >= 1 )
	{
		static mslist<CBasePlayer *> PlayerList;
		PlayerList.clearitems( );

		if( Params[0] == "all" )
		{
			for( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBaseEntity *pEntity = UTIL_PlayerByIndex( i );
				if( pEntity ) PlayerList.add( (CBasePlayer *)pEntity );
			}
		}
		else
		{
			CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;
			if( pEntity && pEntity->IsPlayer( ) )
				PlayerList.add( (CBasePlayer *)pEntity );
		}

		for(int i = 0; i < PlayerList.size(); i++)
			PlayerList[i]->MoveToSpawnSpot( );
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//returndata <value>
//return <value>
//- scope: shared
//- Various usages... When used in game_damaged_other or game_damaged, changes the damage to a ratio of <value>
//- When used by $func(<event>,[params]), causes the function to be replaced by <value>
//- Does not stop code execution - if multiple instances of return are encountered in the same event, the results are tokenized.
bool CScript::ScriptCmd_Return(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 1)
	{
		if (m.pScriptedInterface)
		{
			//Thothie APR2016_08 - allow clearing of return data
			if (Params[0] == "**clear")
			{
				m.pScriptedInterface->m_ReturnData = "";
				return true; //handled
			}
			
			if (m.pScriptedInterface->m_ReturnData[0]) m.pScriptedInterface->m_ReturnData += ";";
			
			for(int i = 0; i < Params.size(); i++)
			{
				if (i) m.pScriptedInterface->m_ReturnData += " ";
				m.pScriptedInterface->m_ReturnData += Params[i];
			}
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//Disabled since scripts shouldn't be handling character saving - Solokiller 5/10/2017
/*
//saveallnow
//- scope: server
//- Force all characters to save now.
bool CScript::ScriptCmd_SaveAllNow( SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params )
{
//saveallnow
//- force all players data to save now
#ifdef VALVE_DLL
for( int i = 1; i <= gpGlobals->maxClients; i++ )
{
CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
if( pPlayer )
{
pPlayer->SaveChar( );
if( !MSGlobals::ServerSideChar ) pPlayer->m_TimeCharLastSent = 0;
}
}
#endif

return true;
}

//savenow <player>
//- scope: server
//- Force target character to save now.
bool CScript::ScriptCmd_SaveNow( SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params )
{
//savenow <target>
//- force player data to save now
#ifdef VALVE_DLL
CBaseEntity *pEntity = RetrieveEntity( Params[0] );
CBasePlayer *pPlayer = pEntity->IsPlayer() ? (CBasePlayer *)pEntity : NULL;
if( pPlayer )
{
pPlayer->SaveChar( );
if( !MSGlobals::ServerSideChar ) pPlayer->m_TimeCharLastSent = 0;
}
else ERROR_MISSING_PARMS;
#endif

return true;
}
*/

//scriptflags
//- scope: server
//- flag tracking system, has several formats, depending on desired action:
//- scriptflags <target> add <name> <type> [value:1] [expiretime:-1] [expiremsg:none]
//-- Adding a scriptflag with a duplicate name resets its value and its expire time,
//-- unless the name contains "stack", in which case another instance will be added.
//-- Stack names should always have an expire time set.
//- scriptflags <target> remove <name> - removes all instances of flags with <name> from <target>
//- scriptflags <target> cleartype <type> - removes all flags of <type> from <target>
//- scriptflags <target> clearall - removes all scriptflags from <target>
//- scriptflags <target> remove_expired - removes expired flags from <target>
//- scriptflags <target> edit <name> <type> <value> [expiretime] [msg] - edits properties of first flag found with this name
//- [expiretime] should be in seconds - actual end time is calculated from the time it is set (-1 indicates never expires, which is the default behavior)
//- Scriptflags are handled script side by: player and monsters/externals->game_scriptflag_update, check_flags_expired
bool CScript::ScriptCmd_ScriptFlags(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	//Thothie JAN2013_02
	//			  0/1      1/2 2/3    3/4    4/5     5/6          6/7
	//scriptflags <target> add <name> <type> [value] [expiretime] [expiremsg]
	//scriptflags <target> edit <name> <type> [value] [expiretime] [expiremsg]
	//scriptflags <target> remove <name>
	//scriptflags <target> cleartype <type> - removes all of <type>
	//scriptflags <target> clearall - removes all
	//scriptflags <target> remove_expired - removes expired flags
	if ( Params.size() >= 2 )
	{
		CBaseEntity *pEntity = RetrieveEntity(Params[0]);
		CBasePlayer *pPlayer = pEntity && pEntity->IsPlayer() ? ((CBasePlayer *)pEntity) : NULL;
		IScripted *pScripted = pEntity ? pEntity->GetScripted() : NULL;

		if ( pEntity && pScripted )
		{
			//Print("scriptflags [%i] %s[1-2] %s[2-3] %s[3-4] %s[4-5] %s[5-6] %s[6-7] %s[8/7]\n",Params.size(),Params[1].c_str(),(Params.size()>=3) ? Params[2].c_str() : "none",(Params.size())>=4 ? Params[3].c_str() : "none", (Params.size()>=5) ? Params[4].c_str() : "none", (Params.size()>=6) ? Params[5].c_str() : "none", (Params.size()>=7) ? Params[6].c_str() : "none", (Params.size()>=7) ? Params[7].c_str() : "none");
			if ( Params[1] == "add" && Params.size() >= 3 )
			{
				//check if name exists, remove it for replacement, if it does
				//this check is not done for names that contains "stack"
				//such flags should have an expire time
				if ( !Params[2].contains("stack") ) //Thothie SEP2019_11 - switching this to contains for ease of use
				{
					for(int i = 0; i < pEntity->m_scriptflags.names.size(); i++)
					{
						if ( pEntity->m_scriptflags.names[i] == Params[2] )
						{
							pEntity->m_scriptflags.names.erase(i);
							pEntity->m_scriptflags.types.erase(i);
							pEntity->m_scriptflags.values.erase(i);
							pEntity->m_scriptflags.expiretimes.erase(i);
							pEntity->m_scriptflags.exp_messages.erase(i);
							i -= 1; //Thothie SEP2019_11 - keep forgetting...
						}
					}
				}
				msstring sf_value = "1";
				msstring sf_expire = "-1";
				msstring sf_message = "none";
				if ( Params.size() >= 5 ) sf_value = Params[4];
				if ( Params.size() >= 6 )
				{
					if ( atof(Params[5]) > -1 )	sf_expire = UTIL_VarArgs( "%f", gpGlobals->time + atof(Params[5]) );
				}
				if ( Params.size() >= 7 ) sf_message = Params[6];
				pEntity->m_scriptflags.names.add(Params[2]);
				pEntity->m_scriptflags.types.add(Params[3]);
				pEntity->m_scriptflags.values.add(sf_value);
				pEntity->m_scriptflags.expiretimes.add(sf_expire);
				pEntity->m_scriptflags.exp_messages.add(sf_message);
			}
			if ( Params[1] == "edit" && Params.size() >= 4 )
			{
				//change values of existing script flag by name
				//though, looking back on it, adding one of the same name should have the same effect - meh.
				//scriptflags <target> edit <name> <type> [value] [expiretime] [expiremsg]
				int sfidx = -1;
				for(int i = 0; i < pEntity->m_scriptflags.names.size(); i++)
				{
					if ( pEntity->m_scriptflags.names[i] == Params[2] ) sfidx = i;
				}
				if ( sfidx > -1 )
				{
					pEntity->m_scriptflags.types[sfidx] = Params[3]; //type
					if ( Params.size() >= 5 ) pEntity->m_scriptflags.values[sfidx] = Params[4];
					if ( Params.size() >= 6 ) pEntity->m_scriptflags.expiretimes[sfidx] = (atof(Params[5])>0) ? UTIL_VarArgs( "%f", gpGlobals->time + atof(Params[5]) ) : "-1"; //Thothie SEP2019_16 - hacky edit fix
					if ( Params.size() >= 7 ) pEntity->m_scriptflags.exp_messages[sfidx] = Params[6];
				}
				else
				{
					//couldn't find it
					MSErrorConsoleText( "ExecuteScriptCmd", UTIL_VarArgs("Script: %s, %s - scriptflags edit - couldn't find name %s.\n", m.ScriptFile.c_str(), Cmd.Name().c_str(), Params[2].c_str() ) );
				}
			}
			else
			{
				if ( Params[1] == "edit" ) MSErrorConsoleText( "ExecuteScriptCmd", UTIL_VarArgs("Script: %s, %s - scriptflags edit - not enough parameters.\n", m.ScriptFile.c_str(), Cmd.Name().c_str() ) );
			}

			if ( Params[1] == "remove" )
			{
				for(int i = 0; i < pEntity->m_scriptflags.names.size(); i++)
				{
					if ( pEntity->m_scriptflags.names[i] == Params[2] )
					{
						if( pPlayer )
						{
							if ( !pEntity->m_scriptflags.exp_messages[i].starts_with("none") ) pPlayer->SendEventMsg( HUDEVENT_UNABLE, pEntity->m_scriptflags.exp_messages[i].c_str() );
						}
						pEntity->m_scriptflags.names.erase(i);
						pEntity->m_scriptflags.types.erase(i);
						pEntity->m_scriptflags.values.erase(i);
						pEntity->m_scriptflags.expiretimes.erase(i);
						pEntity->m_scriptflags.exp_messages.erase(i);
						break; //if there's a duplicate name, you'll have to run this again
					}
				}
			}

			if ( Params[1] == "remove_expired" )
			{
				for(int i = 0; i < pEntity->m_scriptflags.names.size(); i++)
				{
					float sf_time_to_expire = atof(pEntity->m_scriptflags.expiretimes[i]);

					if ( sf_time_to_expire > 0 ) //Thothie SEP2019_11 was "> -1" - MiB thinks maybe a precision thing.
					{
						if ( float(gpGlobals->time) > sf_time_to_expire ) //Thothie SEP2019_16 - trying to defeat this bug by adding float()
						{
							if( pPlayer )
							{
								if ( !pEntity->m_scriptflags.exp_messages[i].starts_with("none") ) pPlayer->SendEventMsg( HUDEVENT_UNABLE, pEntity->m_scriptflags.exp_messages[i].c_str() );
							}
							msstringlist formerprops;
							formerprops.add(pEntity->m_scriptflags.names[i]);
							formerprops.add(pEntity->m_scriptflags.types[i]);
							formerprops.add(pEntity->m_scriptflags.values[i]);
							formerprops.add(pEntity->m_scriptflags.expiretimes[i]);
							formerprops.add(pEntity->m_scriptflags.exp_messages[i]);

							pEntity->m_scriptflags.names.erase(i);
							pEntity->m_scriptflags.types.erase(i);
							pEntity->m_scriptflags.values.erase(i);
							pEntity->m_scriptflags.expiretimes.erase(i);
							pEntity->m_scriptflags.exp_messages.erase(i);

							pScripted->CallScriptEvent( "game_scriptflag_expired", &formerprops );
							i -= 1; //Thothie SEP2019_11 - never forget to decrement - though maybe I should use i--
						}
					}
				}
			}

			if ( Params[1] == "cleartype" )
			{
				for(int i = 0; i < pEntity->m_scriptflags.names.size(); i++)
				{
					if ( pEntity->m_scriptflags.types[i] == Params[2] )
					{
						if( pPlayer )
						{
							if ( !pEntity->m_scriptflags.exp_messages[i].starts_with("none") ) pPlayer->SendEventMsg( HUDEVENT_UNABLE, pEntity->m_scriptflags.exp_messages[i].c_str() );
						}
						pEntity->m_scriptflags.names.erase(i);
						pEntity->m_scriptflags.types.erase(i);
						pEntity->m_scriptflags.values.erase(i);
						pEntity->m_scriptflags.expiretimes.erase(i);
						pEntity->m_scriptflags.exp_messages.erase(i);
						i -= 1; //Thothie SEP2019_11 - never forget to decrement
					}
				}
			}

			if ( Params[1] == "clearall" )
			{
				for(int i = 0; i < pEntity->m_scriptflags.names.size(); i++ )
				{
					if( pPlayer )
					{
						if ( !pEntity->m_scriptflags.exp_messages[0].starts_with("none") ) pPlayer->SendEventMsg( HUDEVENT_UNABLE, pEntity->m_scriptflags.exp_messages[0].c_str() );
					}
					pEntity->m_scriptflags.names.erase(0);
					pEntity->m_scriptflags.types.erase(0);
					pEntity->m_scriptflags.values.erase(0);
					pEntity->m_scriptflags.expiretimes.erase(0);
					pEntity->m_scriptflags.exp_messages.erase(0);
				}
			}

			msstringlist Parameters;
			Parameters.add( Params[1].c_str() ); //action
			if ( Params.size() >= 3 )Parameters.add( Params[2].c_str() ); //name
			if ( Params.size() >= 4 )Parameters.add( Params[3].c_str() ); //type
			if ( Params.size() >= 5 ) Parameters.add( Params[4].c_str() ); //value
			if ( Params.size() >= 6 ) Parameters.add( Params[5].c_str() ); //base expire time
			pScripted->CallScriptEvent("game_scriptflag_update",&Parameters);
		}
		else
		{
			MSErrorConsoleText( "scriptflags", UTIL_VarArgs("target entity not found in %s\n",m.ScriptFile.c_str()) );
		}
	}
	else ERROR_MISSING_PARMS;
#endif
	return true;
}


//servercmd <cmds>
//- scope: server
//- Sends commands to server console
//- Steam Pipe's new "security" measures may prevent certain commands from being sent.
bool CScript::ScriptCmd_ServerCmd(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//Thothie FEB2008a - client<->script management
#ifdef VALVE_DLL
	msstring msTemp;
	if( Params.size() >= 1 )
	{
		for(int i = 0; i < Params.size(); i++)
		{
			if( i ) msTemp += " ";
			msTemp += Params[i];
		}

		msTemp += "\n";
		SERVER_COMMAND( msTemp.c_str() );
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

// [g_]set.create [Entity] <set_name>
// - Creates the set with the given name. If one already existed, does nothing
// [g_]set.add    [Entity] <set_name> <value>
// - Adds the value to the set
// [g_]set.del    [Entity] <set_name> <value>
// - Deletes the value from the set
// [g_]set.erase  [Entity] <set_name>
// - Erases the set
// [g_]set.clear  [Entity] <set_name>
// - Clears the set values
// [g_]set.copy   [Entity] <src_set_name> <dest_map_name>
// - Copies the set to this entity with the given name
bool CScript::ScriptCmd_Set(
	SCRIPT_EVENT &                        Event
	, scriptcmd_t &                         Cmd
	, msstringlist &                        Params
	)
{
	size_t                              vParam = 0;
	msstring                            vsSubCmd;
	bool                                bGlobal = false;
	CBaseEntity *                       pEntity = NULL;
	msstring                            vsSetName;
	bool                                bExisted;

	if (Params.size())
	{
		vsSetName = Params[vParam++];
		if (Cmd.Name().starts_with("g_"))
		{
			bGlobal = true;
			vsSubCmd = Cmd.Name().substr(6);
		}
		else
		{
			vsSubCmd = Cmd.Name().substr(4);
			pEntity = RetrieveEntity(vsSetName);
			if (pEntity)
			{
				if (Params.size() > vParam)
				{
					vsSetName = Params[vParam++];
				}
				else
				{
					ERROR_MISSING_PARMS;
					return true;
				}
			}
		}

		if (!pEntity)
		{
			pEntity = m.pScriptedEnt;
		}

		msscriptsethash &               vSetHashMap = bGlobal ? mGlobalScriptSets : pEntity->mScriptedSets;
		msscriptset *                   pSet = GetScriptedSetFromHashMap(vSetHashMap
			, vsSetName
			, vsSubCmd == "create"
			, &bExisted
			);
		if (vsSubCmd != "create") // create already done
		{
			if (!bExisted)
			{
				ErrorPrint("NO_SET"
					, ERRORPRINT_CONSOLE | ERRORPRINT_LOG | ERRORPRINT_CVAR
					, "Set %s doesn't exist for command %s"
					, vsSetName.c_str()
					, Cmd.Name().c_str()
				);
				return true;
			}

			if (vsSubCmd == "add")
			{
				if (Params.size() > vParam)
				{
					pSet->insert(Params[vParam++]);
				}
				else ERROR_MISSING_PARMS;
			}
			else
			if (vsSubCmd == "del")
			{
				if (Params.size() > vParam)
				{
					pSet->erase(Params[vParam++]);
				}
				else ERROR_MISSING_PARMS;
			}
			else
			if (vsSubCmd == "erase")
			{
				vSetHashMap.erase(vsSetName);
			}
			else
			if (vsSubCmd == "clear")
			{
				pSet->clear();
			}
			else
			if (vsSubCmd == "copy")
			{
				if (Params.size() > vParam)
				{
					msstring            vsDestSetName = Params[vParam++];
					msscriptset *       pDest = GetScriptedSetFromHashMap(m.pScriptedEnt->mScriptedSets
						, vsDestSetName
						, true
						);
					msscriptset::iterator
						iVal;
					for (iVal = pSet->begin()
						; !(iVal == pSet->end())
						; ++iVal
						)
					{
						pDest->insert(*iVal);
					}
				}
				else
					ERROR_MISSING_PARMS;
			}
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

// forcesend <entity> <bool>
//- scope: server
//- set the entity to be force sent to the client.
bool CScript::ScriptCmd_SetEntForceSend(SCRIPT_EVENT& Event, scriptcmd_t& Cmd, msstringlist& Params)
{
#ifdef VALVE_DLL
	if (Params.size() >= 2)
	{
		int iVal = atoi(Params[1]);
		CBaseEntity* pEntity = (m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity(Params[0]) : NULL);
		if (pEntity && !pEntity->NOSEND)
		{
			pEntity->FORCESEND = (iVal > 0);
			return true;
		}
	}
#endif
	return false;
}

// nosend <entity> <bool>
//- scope: server
//- set the entity not to be sent to via client.
bool CScript::ScriptCmd_SetEntNoSend(SCRIPT_EVENT& Event, scriptcmd_t& Cmd, msstringlist& Params)
{
#ifdef VALVE_DLL
	if (Params.size() >= 2)
	{
		int iVal = atoi(Params[1]);
		CBaseEntity* pEntity = (m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity(Params[0]) : NULL);
		if (pEntity && !pEntity->FORCESEND)
		{
			pEntity->NOSEND = (iVal > 0);
			return true;
		}
	}
#endif
	return false;
}

//setalive <0|1>
//- scope: server
//- Can be used to cause an NPC to "come back to life" in game_death.
//- Or mark an NPC as dead, prematurely.
//- Once marked dead, subsequent delayed events will not be called, as the script will stop "thinking".
//- Beware that monster spawners will still count resurrected monster as dead, and spawn another one, if it is set to do so.
//- Not certain if using this to set an NPC as dead will cause the monster spawner to count it as such, but I doubt it.
bool CScript::ScriptCmd_SetAlive(SCRIPT_EVENT& Event, scriptcmd_t& Cmd, msstringlist& Params)
{
	if (Params.size() >= 1)
	{
		if (m.pScriptedEnt)
		{
			if (atoi(Params[0]))
			{
				m.pScriptedEnt->pev->deadflag = DEAD_NO;
				m.pScriptedEnt->pev->solid = SOLID_BBOX;
				m.pScriptedEnt->pev->movetype = MOVETYPE_STEP;
				UTIL_SetOrigin(m.pScriptedEnt->pev, m.pScriptedEnt->pev->origin);
				m.pScriptedEnt->SetThink(NULL);
				RunScriptEventByName("game_fake_death"); //Thothie DEC2010_06 - handle fake deaths globally
			}
			else m.pScriptedEnt->pev->deadflag = DEAD_DEAD;
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//setatkspeed <ratio>
//- scope: shared
//- Sets the ratio the player uses to adjust attack speed
bool CScript::ScriptCmd_SetAtkSpeed(
	SCRIPT_EVENT &                        Event
	, scriptcmd_t &                         Cmd
	, msstringlist &                        Params
	)
{
	if (Params.size() >= 1)
	{
		float                           vRatio = atof(Params[0]);
		if (vRatio > 0)
		{
#ifdef VALVE_DLL
			CBasePlayer *               pPlayer = m.pScriptedEnt && m.pScriptedEnt->IsPlayer() ? (CBasePlayer *)m.pScriptedEnt : NULL;
			if ( pPlayer )
			{
				pPlayer->m_AnimSpeedAdj = vRatio;
				MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_SETPROP], NULL, pPlayer->pev );
				WRITE_BYTE( PROP_ANIMSPEEDADJ );
				WRITE_FLOAT( vRatio );
				MESSAGE_END();
			}
#else
			player.m_AnimSpeedAdj = vRatio;
			ClientCmd(UTIL_VarArgs("dosetanimatkadj %f\n", vRatio));
#endif
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//setangle <face[.x|.y|.z]> <ang|vec>
//setangle <face_origin> <vec>
//setangle <view[.x|.y|.z]> <ang|vec>
//setangle <add_view.x|add_view.y|add_view.z> <ang>
//setangle <view_origin> <vec>
//- scope: server, NPC
//- Sets NPC's angles or viewangles.
//- Glitchy under some circumstances.
bool CScript::ScriptCmd_SetAngle(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 1)
	{
		if (m.pScriptedEnt)
		{
			msstring AngToModify = Params[0];
			bool SetFace = false;

			if (AngToModify == "velocity")
				m.pScriptedEnt->pev->angles = UTIL_VecToAngles(m.pScriptedEnt->pev->velocity);

			else if (Params.size() >= 2)
			{
				if ((AngToModify == "add_view.x" || AngToModify == "add_view.pitch"))
					m.pScriptedEnt->pev->v_angle.x += -atof(Params[1]);
				else if ((AngToModify == "add_view.y" || AngToModify == "add_view.yaw"))
					m.pScriptedEnt->pev->v_angle.y += atof(Params[1]);
				else if ((AngToModify == "add_view.z" || AngToModify == "add_view.roll"))
					m.pScriptedEnt->pev->v_angle.z += atof(Params[1]);

				else if ((AngToModify == "view.x" || AngToModify == "view.pitch"))
					m.pScriptedEnt->pev->v_angle.x = -atof(Params[1]);
				else if ((AngToModify == "view.y" || AngToModify == "view.yaw"))
					m.pScriptedEnt->pev->v_angle.y = atof(Params[1]);
				else if ((AngToModify == "view.z" || AngToModify == "view.roll"))
					m.pScriptedEnt->pev->v_angle.z = atof(Params[1]);
				else if (AngToModify == "view")
				{
					CBaseEntity *pTarget = m.pScriptedEnt->RetrieveEntity(Params[1]);
					if (pTarget) m.pScriptedEnt->pev->v_angle = UTIL_VecToAngles((pTarget->EyePosition() - m.pScriptedEnt->Center()).Normalize());
					else m.pScriptedEnt->pev->v_angle = StringToVec(Params[1]);
					m.pScriptedEnt->pev->v_angle *= -1;
				}
				else if (AngToModify == "view_origin")
				{
					m.pScriptedEnt->pev->v_angle = UTIL_VecToAngles((StringToVec(Params[1]) - m.pScriptedEnt->Center()).Normalize());
					m.pScriptedEnt->pev->v_angle.x *= -1; //Thothie JUL2010_23 - inverse pitch, not the whole angle, I think
				}


				else if ((AngToModify == "face.x" || AngToModify == "face.pitch"))
				{
					m.pScriptedEnt->pev->angles.x = atof(Params[1]); SetFace = true;
				}
				else if ((AngToModify == "face.y" || AngToModify == "face.yaw"))
				{
					m.pScriptedEnt->pev->angles.y = atof(Params[1]); SetFace = true;
				}
				else if ((AngToModify == "face.z" || AngToModify == "face.roll"))
				{
					m.pScriptedEnt->pev->angles.z = atof(Params[1]); SetFace = true;
				}

				else if (AngToModify == "face")
				{
					CBaseEntity *pTarget = m.pScriptedEnt->RetrieveEntity(Params[1]);
					if (pTarget) m.pScriptedEnt->pev->angles = UTIL_VecToAngles((pTarget->EyePosition() - m.pScriptedEnt->Center()).Normalize());
					else m.pScriptedEnt->pev->angles = StringToVec(Params[1]);
					SetFace = true;
				}
				else if (AngToModify == "face_origin")
				{
					m.pScriptedEnt->pev->angles = UTIL_VecToAngles((StringToVec(Params[1]) - m.pScriptedEnt->Center()).Normalize());
					m.pScriptedEnt->pev->angles.x *= -1; //Thothie JUL2010_23 - inverse pitch
					SetFace = true;
				}
			}

			if (SetFace)		//If I set face angles, make the view angles match up with the face angles
			{
				m.pScriptedEnt->pev->v_angle = m.pScriptedEnt->pev->angles;
				m.pScriptedEnt->pev->v_angle.x *= -1;
			}

		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//setbbox <npcsize|animsize>
//setbbox <vec_mins> <vec_maxs>
//- scope: server
//- Changes the caller's bounding box.
//- May cause odd behavior when used post load, including crashes, when using animsize.
bool CScript::ScriptCmd_SetBBox(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	//Thothie - THIS COMMAND DOES NOT WORK POST-SPAWN AND I WANNA KNOW WHY!!! :...(
	//"npcsize" - Uses width and height
	//[mins] [maxs] - Uses specified mins and maxs
	if( Params.size() >= 1 )
	{
		if( m.pScriptedEnt )
		{
			if( Params.size() >= 2 )
			{
				m.pScriptedEnt->pev->mins = StringToVec(Params[0]);
				m.pScriptedEnt->pev->maxs = StringToVec(Params[1]);
				UTIL_SetSize( m.pScriptedEnt->pev, m.pScriptedEnt->pev->mins, m.pScriptedEnt->pev->maxs );
			}
			else {
				CMSMonster *pMonster = m.pScriptedEnt->IsMSMonster() ? (CMSMonster *)m.pScriptedEnt : NULL;
				if( pMonster )
				{
					if( Params[0] == "animsize" )
					{
						if( pMonster->pev->model )
						{
							Vector v_Mins, v_Maxs;
							pMonster->ExtractBbox( 0, v_Mins, v_Maxs );
							UTIL_SetSize( pMonster->pev, v_Mins, v_Maxs );
						}
					}
					else if( Params[0] == "npcsize" )
						UTIL_SetSize( pMonster->pev, Vector(-pMonster->m_Width/2,-pMonster->m_Width/2,0), Vector(pMonster->m_Width/2,pMonster->m_Width/2,pMonster->m_Height) );
				}
			}
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//setcallback <touch|think|blocked|render> <enable|disable>
//- scope: shared
//- Enables certian resource intensive callbacks, including:
//- touch: game_touch <toucher>
//- blocked: game_blocked <blocker>
//- think: game_think (called every think frame)
//- render: game_prerender (called before every render frame - client only)
bool CScript::ScriptCmd_SetCallBack(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//<type> <enable/disable>
	if (Params.size() >= 2)
	{
		if (m.pScriptedEnt)
		{
			if (m.pScriptedEnt->GetScripted())
			{
				CScriptedEnt *pScriptedEnt = (CScriptedEnt *)m.pScriptedEnt;
				msstring &Type = Params[0];
				msstring &Setting = Params[1];

				bool NewSetting = false;
				if (Setting == "enable") NewSetting = true;

				if (Type.contains("touch")) pScriptedEnt->m_HandleTouch = NewSetting;
				if (Type.contains("think")) pScriptedEnt->m_HandleThink = NewSetting;
				if (Type.contains("blocked")) pScriptedEnt->m_HandleBlocked = NewSetting;
				if (Type.contains("render")) m.m_HandleRender = NewSetting;
			}
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//setcvar <cvar> <val>
//- scope: shared
//- Sets a console var, client or server, depending on scope
bool CScript::ScriptCmd_SetCVar(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//Thothie JUN2007a - set a server-side cvar (MAYBE clientside too)
	//setcvar <CVAR> <VALUE>
	msstring &msCVARtoSetName = Params[0];
	msstring &msCVARSetValue = Params[1];
	CVAR_SET_STRING(msCVARtoSetName.c_str(), msCVARSetValue.c_str());

	return true;
}

//setenv <property> <value>
//- scope: client
//- Sets various client side sky and weather effects
//- sky.texture does not seem to work
bool CScript::ScriptCmd_SetEnv(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifndef VALVE_DLL
	//<prop> <value>
	if (Params.size() >= 2)
	{
		msstring &Prop = Params[0];
		msstring &Value = Params[1];
		if (Prop == "sky.texture")			CEnvMgr::ChangeSkyTexture(Value);
		else if (Prop == "lightgamma")		CEnvMgr::SetLightGamma(atof(Value));
		else if (Prop == "maxviewdist")	CEnvMgr::m_MaxViewDistance = atof(Value);
		else if (Prop == "fog.enabled")	CEnvMgr::m_Fog.Enabled = atoi(Value) ? true : false;
		else if (Prop == "fog.color")		CEnvMgr::m_Fog.Color = StringToVec(Value);
		else if (Prop == "fog.density")	CEnvMgr::m_Fog.Density = atof(Value);
		else if (Prop == "fog.start")		CEnvMgr::m_Fog.Start = atof(Value);
		else if (Prop == "fog.end")		CEnvMgr::m_Fog.End = atof(Value);
		else if (Prop == "fog.type")
		{
			if (Value == "exp")		CEnvMgr::m_Fog.Type = 0x0800;	//GL_EXP
			else if (Value == "exp2")	CEnvMgr::m_Fog.Type = 0x0801;	//GL_EXP2
			else						CEnvMgr::m_Fog.Type = 0x2601;	//GL_LINEAR
		}
		else if (Prop == "screen.tint")	CEnvMgr::ChangeTint(StringToColor(Value));
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//setexpstat <target> <skillname> <value>
//- scope: server
//- sets an NPCs skill level, usually used to set parry
bool CScript::ScriptCmd_SetExpStat(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//NOV2014_11 comment: runs on client too, but looks like wouldn't work there
	if (Params.size() >= 2)
	{
		CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity(Params[0]) : NULL;
		if (pEntity && pEntity->IsMSMonster())
		{
			CMSMonster *pMonster = (CMSMonster *)pEntity;

			int StatVar = -1, PropVar = -1;
			GetStatIndices(Params[1], StatVar, PropVar);
			if (StatVar > -1)
			{
				pMonster->m_UseExpStat = true;
				pMonster->m_ExpStat = StatVar;
				pMonster->m_ExpProp = PropVar;
			}
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//setfollow <target|none> ["align_bottom"]
//- scope: server
//- Causes current script's model origin to follow target, or to cease doing so, if <target>="none".
bool CScript::ScriptCmd_SetFollow(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 1)
	{
		if (m.pScriptedEnt)
		{
			if (Params[0] == "none")
			{
				m.pScriptedEnt->pev->movetype = MOVETYPE_STEP;
				m.pScriptedEnt->SetFollow(NULL);
			}
			else if (Params.size() >= 2)
			{
				CBaseEntity *pTarget = m.pScriptedEnt->RetrieveEntity(Params[0]);
				if (pTarget)
				{
					int FollowFlags = 0;
					if (Params[1].find("align_bottom") != msstring_error) SetBits(FollowFlags, ENT_EFFECT_FOLLOW_ALIGN_BOTTOM);
					//if( Params[1].find("face_host") != msstring_error ) SetBits( FollowFlags, ENT_EFFECT_FOLLOW_FACE_HOST );

					m.pScriptedEnt->SetFollow(pTarget, FollowFlags);
					m.pScriptedEnt->pev->movetype = MOVETYPE_NONE;
				}
			}
			else ERROR_MISSING_PARMS;
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//setgaitspeed <float>
//- scope: uncertain, player
//- sets player's leg movement speed
bool CScript::ScriptCmd_SetGaitSpeed(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 1)
	{
		if (m.pScriptedEnt && m.pScriptedEnt->IsPlayer())
		{
			CBasePlayer *pPlayer = (CBasePlayer *)m.pScriptedEnt;
			pPlayer->m_GaitFramerateGauge = atof(Params[0]);
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//setmodel <modelname|none>
//- scope: server
//- Defines model for server side script to use.
bool CScript::ScriptCmd_SetModel(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	msstring msTemp;
	if( Params.size() >= 1 )
	{
		if( Params[0] == "none" )
			SetBits( m.pScriptedEnt->pev->effects, EF_NODRAW );
		else
		{
			msTemp = "models/";
			msTemp += Params[0];
			m.pScriptedEnt->m_ModelName = msTemp;
			m.pScriptedEnt->pev->model = MAKE_STRING( m.pScriptedEnt->m_ModelName.c_str() );
			if ( m.pScriptedEnt->edict() )
			{
				SET_MODEL( m.pScriptedEnt->edict(), STRING(m.pScriptedEnt->pev->model) );
				ClearBits( m.pScriptedEnt->pev->effects, EF_NODRAW );
			}
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//setmodelbody <group> <submodel>
//- scope: server
//- Sets submodels on current model.
//- Unlike HLMV, indexes begin at 0.
bool CScript::ScriptCmd_SetModelBody(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//MiB JAN2010_27 - Char Selection Fix
#ifdef VALVE_DLL

	//Old way. Consolidated it and made some changes to make it easier for what comes after
	/*		if( Params.size() == 2 )
		SetBodygroup( GET_MODEL_PTR( ENT(m.pScriptedEnt->pev) ), m.pScriptedEnt->pev, atoi(Params[0]), atoi(Params[1]) );

		if( Params.size() == 3 )
		{
		CBaseEntity *pOtherEntity = RetrieveEntity( Params[0] );
		SetBodygroup( GET_MODEL_PTR( ENT(pOtherEntity->pev) ), pOtherEntity->pev, atoi(Params[1]), atoi(Params[2]) );
		}*/

	if ( Params.size() >= 2 )
	{
		CBaseEntity *pTarget = Params.size() >= 3 ? RetrieveEntity( Params[0] ) : m.pScriptedEnt;

		if ( !pTarget->pev ) return true;

		int ofs = Params.size() >= 3 ? 1 : 0;
		SetBodygroup ( GET_MODEL_PTR( ENT( pTarget->pev ) ), pTarget->pev, atoi( Params[ofs] ), atoi( Params[ofs+1] ) );

		//MiB FEB2010a - Keep the player's quest flag updated with what body is in use
		//This is used so the character selection screen knows what body to use for the mini-player.
		if ( pTarget->IsPlayer() )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pTarget;

			//MiB MAR2010_12 - Armor Fix FINAL
			MESSAGE_BEGIN( MSG_ALL, g_netmsg[NETMSG_CLDLLFUNC], NULL );
			WRITE_BYTE( 27 );
			WRITE_BYTE( pPlayer->entindex() );
			WRITE_BYTE( pPlayer->pev->body );
			MESSAGE_END();

			// MiB MAY2019_08 - Only update if alive. Stops
			// the temporary body-set to make corpse look right
			// from screwing up char select screen on reconnect
			if ( pPlayer->pev->deadflag == DEAD_NO )
			{
				bool found = false;
				for(int i = 0; i < pPlayer->m_Quests.size(); i++)
				{
					if ( pPlayer->m_Quests[i].Name == "BODY" )
					{
						pPlayer->m_Quests[i].Data = pPlayer->pev->body;
						found = true;
						break;
					}
				}

				if ( !found )
				{
					quest_t q;
					q.Name = "BODY";
					q.Data = pPlayer->pev->body;
					pPlayer->m_Quests.add( q );
				}
			}
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//setprop <target> <property> <value>
//- scope: server
//- Available properties include:
//- speed <ratio>
//- skin <idx> - model skins idx from 0
//- modelindex <idx> - change global model index - will likely crash
//- model <modelname> - change models
//- movetype <type> - 0=none,3=player_walk,4=npc_walk,5=fly,6=toss,7=push,8=noclip,9=missile,10=bounce,11=bounce_nograv,12=follow,13=BSP
//- solid <type> - 0=none,1=trigger,2=bbox,3=slidebox,4=BSP
//- frame <idx> - animation frame index
//- framerate <ratio> - animation frame rate
//- classname <newclass> - eg. func_wall - will likely crash
//- basevelocity <vec>
//- avelocity <vec> - angles velocity - for spinning
//- velocity <vec> - same as setvelocity
//- movedir <vec> - uncertain
//- ltime <float> - uncertain
//- nextthink <float> - time to next think
//- friction <float> - step friction
//- animtime <float> - uncertain
//- sequence <int> - set current animation sequence by idx
//- playerclass <int> - uncertain
//- target <string> - entity's target
//- targetname <string> - entity's targetname
//- netname <string> - entity's unique name
//- view_ofs <vec> - view offset - may not work on players
//- deadflag <int> - uncertain
//- health <float> - entity "health" - may or may not corrispond with hp
//- aiment <target> - target for setfollow
//- controller0-3 <int> - bone controller positions
//- blending0-2 <int> - uncertain
//- rendermode <mode> - 0=normal,1=color,2=texture,3=glow,4=solid,5=additive
//- renderamt <0-255> - intensity of rendermode
//- renderfx <0-16> - as follows:
//-- 0=Normal
//-- 1=Slow Pulse
//-- 2=Fast Pulse
//-- 3=Slow Wide Pulse
//-- 4=Fast Wide Pulse
//-- 5=Slow Fade Away
//-- 6=Fast Fade Away
//-- 7=Slow Become Solid
//-- 8=Fast Become Solid
//-- 9=Slow Strobe
//-- 10=Fast Strobe
//-- 11=Faster Strobe
//-- 12=Slow Flicker
//-- 13=Fast Flicker
//-- 14=Constant Glow
//-- 15=Distort
//-- 16=Hologram (Distort + fade)
//- rendercolor <RRR,GGG,BBB>
//- scale <ratio>
//- setbody <group> <submodel> - as setmodelbody
//- angles <vec>
bool CScript::ScriptCmd_SetProp(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	msstring msTemp;
	if( Params.size() >= 3 )
	{
		CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;
		if( pEntity )
		{
			msstring &PropName = Params[1];
			msstring &PropValue = Params[2];
			int Int = atoi(PropValue);

			float Float = atof(PropValue);
			Vector vVector = StringToVec(PropValue);

			if( PropName == "speed" )			pEntity->pev->speed = Float;
			else if( PropName == "skin" )		pEntity->pev->skin = Int;
			else if( PropName == "modelindex" )	pEntity->pev->modelindex = Int;
			else if( PropName == "model" )
			{
				msTemp = "models/";
				msTemp += Params[2];
				pEntity->m_ModelName = msTemp;
				if ( pEntity->edict() )
				{
					pEntity->pev->model = MAKE_STRING( pEntity->m_ModelName.c_str() );
					SET_MODEL( pEntity->edict(), STRING(pEntity->pev->model) );
				}
			}
			else if( PropName == "movetype" )	pEntity->pev->movetype = Int;
			else if( PropName == "solid" )		pEntity->pev->solid = Int;
			else if( PropName == "frame" )		pEntity->pev->frame = Float;
			else if( PropName == "framerate" )	pEntity->pev->framerate = Float;
			else if( PropName == "classname" )	pEntity->pev->classname = ALLOC_STRING(PropValue);
			else if( PropName == "basevelocity" )	pEntity->pev->basevelocity = vVector;
			else if( PropName == "avelocity" )	pEntity->pev->avelocity = vVector;
			else if( PropName == "velocity" )	pEntity->pev->velocity = vVector;
			else if( PropName == "movedir" )	pEntity->pev->movedir = vVector;
			else if( PropName == "ltime" )		pEntity->pev->ltime = Float;
			else if( PropName == "nextthink" )	pEntity->pev->nextthink = Float;
			else if( PropName == "friction" )	pEntity->pev->friction = Float;
			//else if( PropName == "frame" )	pEntity->pev->frame = Float;
			else if( PropName == "animtime" )	pEntity->pev->animtime = Float;
			else if( PropName == "sequence" )	pEntity->pev->sequence = Int;
			else if( PropName == "playerclass" )	pEntity->pev->playerclass = Int;
			else if( PropName == "target" )		pEntity->pev->target = ALLOC_STRING(PropValue);
			else if( PropName == "targetname" )	pEntity->pev->targetname = ALLOC_STRING(PropValue);
			else if( PropName == "netname" )	pEntity->pev->netname = ALLOC_STRING(PropValue);
			else if( PropName == "view_ofs" )	pEntity->pev->view_ofs = vVector;
			else if( PropName == "deadflag" )	pEntity->pev->deadflag = Int;
			else if( PropName == "health" )		pEntity->pev->health = Float;
			//else if( PropName == "friction" )	pEntity->pev->friction = Float; //Thothie MAY2011_04
			else if( PropName == "aiment" )
			{
				CBaseEntity *pEntity = StringToEnt(PropValue);
				if( pEntity && pEntity->edict() )
					pEntity->pev->aiment = pEntity->edict();
			}
			else if( PropName == "controller0" ) pEntity->pev->controller[0] = Int;
			else if( PropName == "controller1" ) pEntity->pev->controller[1] = Int;
			else if( PropName == "controller2" ) pEntity->pev->controller[2] = Int;
			else if( PropName == "controller3" ) pEntity->pev->controller[3] = Int;
			else if( PropName == "blending0" ) pEntity->pev->blending[0] = Int;
			else if( PropName == "blending1" ) pEntity->pev->blending[1] = Int;
			else if (PropName == "blending2") { pEntity->pev->blending[0] = Int; pEntity->pev->blending[1] = Int; }
			else if( PropName == "renderamt" ) pEntity->pev->renderamt = Int; //added by Thothie
			else if( PropName == "rendermode" ) pEntity->pev->rendermode = Int;  //added by Thothie
			else if( PropName == "renderfx" ) pEntity->pev->renderfx = Int;  //added by Thothie
			else if( PropName == "rendercolor" ) pEntity->pev->rendercolor = vVector;  //added by Thothie
			else if( PropName == "scale" ) pEntity->pev->scale = Float;  //added by Thothie
			else if( PropName == "angles" ) pEntity->pev->angles = vVector;  //added by Thothie
			else if( PropName == "setbody" )
			{
				SetBodygroup( GET_MODEL_PTR( ENT(pEntity->pev) ), pEntity->pev, atoi(Params[2]), atoi(Params[3]) );
			}
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//setpvp <1|0>
//- scope: server
//- Enables/disables PVP mode
bool CScript::ScriptCmd_SetPVP(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//-  setpvp <1|0>
	//- For setting PVP mode in global votes and the like
	if (Params.size() >= 1)
	{
		MSGlobals::PKAllowed = atoi(Params[0]) > 0 ? true : false;
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//setquality <item> <quality> [maxquality]
//- scope: server
//- Sets quality, and optionally, max quality, on an item.
bool CScript::ScriptCmd_SetQuality(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//setquality <item> <quality> [maxquality]
	//Thothie NOV2014_14 - sets quality property on an item

#ifdef VALVE_DLL
	if ( Params.size() >= 2 )
	{
		CBaseEntity *pEnt = m.pScriptedEnt->RetrieveEntity( Params[0] );
		CGenericItem *pItem = pEnt->IsMSItem() ? (CGenericItem *)pEnt : NULL;
		if ( pItem )
		{
			pItem->Quality = atof(Params[1]);
			if ( Params.size() >= 3 ) pItem->MaxQuality = atof(Params[2]);
		}
		else MSErrorConsoleText( "ScriptCmd_SetQuality", UTIL_VarArgs("Script: %s, %s - Warning: attempted to use setquality on non-item.\n", m.ScriptFile.c_str(), Cmd.Name().c_str()));
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//setquantity <item> <quantity>
//- scope: server
//- Sets quantity of an item.
bool CScript::ScriptCmd_setquantity(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//setquantity <item> <quantity>
	//Thothie MAR2015_15 - sets quantity property on an item

#ifdef VALVE_DLL
	if ( Params.size() >= 2 )
	{
		CBaseEntity *pEnt = m.pScriptedEnt->RetrieveEntity( Params[0] );
		CGenericItem *pItem = pEnt->IsMSItem() ? (CGenericItem *)pEnt : NULL;
		if ( pItem )
		{
			pItem->iQuantity = atof(Params[1]);
		}
		else MSErrorConsoleText( "ScriptCmd_setquantity", UTIL_VarArgs("Script: %s, %s - Warning: attempted to use setquantity on non-item.\n", m.ScriptFile.c_str(), Cmd.Name().c_str()));
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//moditem <item> <mod_str>
//- Stores item mods, and flags item as modded
//Thothie OCT2016_18 Item Mods
/*
bool CScript::ScriptCmd_moditem(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	if ( Params.size() > 1 )
	{
		CBaseEntity *pEnt = m.pScriptedEnt->RetrieveEntity( Params[0] );
		CGenericItem *pItem = pEnt->IsMSItem() ? (CGenericItem *)pEnt : NULL;
		if ( pItem )
		{
			SetBits( pItem->Properties, ITEM_MODDED );
			pItem->Mods = Params[1];
		}
		else
		{
			ALERT( at_aiconsole, "ScriptError: moditem used on non-item\n" );
		}
	}
	else ERROR_MISSING_PARMS;
#endif
	return true;
}*/


//syncitem <target_item> <target_player>
//displaydesc <target_item> <target_player>
//Syncronizes server side item data with client
//(compiles, but not yet working)
//Thothie OCT2016_04
bool CScript::ScriptCmd_syncitem(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	CBaseEntity *pEnt = m.pScriptedEnt->RetrieveEntity( Params[0] );
	CGenericItem *pItem = pEnt->IsMSItem() ? (CGenericItem *)pEnt : NULL;
	if ( pItem )
	{
		CBaseEntity *pTemp = m.pScriptedEnt->RetrieveEntity( Params[1] );
		CBasePlayer *pPlayer = pTemp->IsPlayer() ? (CBasePlayer *)pTemp : NULL;
		if ( pPlayer )
		{
			if ( Cmd.Name() == "syncitem" )
			{
				MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_UPDATEITEM], NULL, pPlayer->pev );
				//WRITE_BYTE( 1 );							//0 == New | 1 == Update existing
				//SendGenericItem( pPlayer, pItem, false );
				WRITE_LONG( pItem->m_iId );
				WRITE_STRING_LIMIT(pItem->m_DisplayName.c_str(), 90);
				WRITE_STRING_LIMIT(pItem->DisplayDesc.c_str(), 90);
				MESSAGE_END();
			}
			else if ( Cmd.Name() == "displaydesc" )
			{
				MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_ITEM], NULL, pPlayer->pev );
				WRITE_BYTE( 9 );
				WRITE_LONG( pItem->m_iId );
				MESSAGE_END();
			}
		}
		else
		{
			ALERT( at_aiconsole, "ScriptError: syncitem could not find client from %s\n",Params[1].c_str());
		}
	}
	else
	{
		ALERT( at_aiconsole, "ScriptError: syncitem used on non-item\n");
	}
#endif
	return true;
}

//teleportdest <info_teleport_destination targetname>
//Thothie OCT2015_18 - teleport to specific info_teleport_destination
bool CScript::ScriptCmd_teleportdest(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	msstring destname = Params[0];
	CBaseEntity *pTeleporter = m.pScriptedEnt;
	CBaseEntity *pTeleportDest = UTIL_FindEntityByString( NULL, "targetname", destname.c_str() );
	entvars_t* pevToucher = pTeleporter->pev;
	edict_t	*pentTarget = NULL;

	if ( !pTeleportDest )
	{
		ALERT( at_aiconsole, "ScriptError: teleport - info_teleport_destination %s not found", destname.c_str());
		return true;
	}

	pentTarget = pTeleportDest->edict();

	Vector tmp = VARS( pentTarget )->origin;

	if ( pTeleporter->IsPlayer() )
	{
		tmp.z -= pTeleporter->pev->mins.z;// make origin adjustments in case the teleportee is a player. (origin in center, not at feet)
	}
	tmp.z++;

	pevToucher->flags &= ~FL_ONGROUND;

	UTIL_SetOrigin( pevToucher, tmp );


	/*
	//This method didn't work - prob some simple string conversion error, but meh, simplified above (sadly, no indexing)
	CBaseEntity *pOther = m.pScriptedEnt;
	entvars_t* pevToucher = pOther->pev;
	edict_t	*pentTarget = NULL;

	mslist <CBaseEntity *> TeleLocs;
	CBaseEntity *pLoc = NULL;
	while ( pLoc = UTIL_FindEntityByClassname( pLoc, "info_teleport_destination" ) )
	{
	if ( FNullEnt(pLoc) )
	continue;

	//Print("XEBUG: teleport %s vs %s\n", msstring( STRING(pLoc->pev->targetname) ), msstring( STRING(destname.c_str()) ) );

	if ( msstring( STRING(destname.c_str()) ) == msstring( STRING(pLoc->pev->targetname) ) )
	TeleLocs.add( pLoc );
	}

	if ( !TeleLocs.size() )
	{
	ALERT( at_aiconsole, "ScriptError: teleport - info_teleport_destination %s not found", destname.c_str());
	return true;
	}

	int iTeleIdx = 0;
	if ( Params.size() > 1 ) iTeleIdx = atoi(Params[1].c_str());

	pentTarget = TeleLocs[ iTeleIdx ]->edict();

	Vector tmp = VARS( pentTarget )->origin;

	if ( pOther->IsPlayer() )
	{
	tmp.z -= pOther->pev->mins.z;// make origin adjustments in case the teleportee is a player. (origin in center, not at feet)
	}
	tmp.z++;

	pevToucher->flags &= ~FL_ONGROUND;

	UTIL_SetOrigin( pevToucher, tmp );
	*/
#endif
	return true;
}

//setsolid <none|box|slidebox|trigger>
//- scope: server
//- Sets solidity of a server side script
//- none = non solid
//- box = standard solidity by bounding box
//- slidebox = similar to box, but ignores ground
//- trigger = non-solid, but still detects edge collisions such as those from game_touch, and still accepts +use requests
bool CScript::ScriptCmd_SetSolid(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	CBaseEntity *pEntity = m.pScriptedEnt;
	msstring Setting = Params[0];
	if (Params.size() >= 2)
	{
		pEntity = RetrieveEntity(Params[0]);
		Setting = Params[1];
	}

	if (Params.size() >= 1)
	{
		if (pEntity)
		{
			if (Setting == "none")			pEntity->pev->solid = SOLID_NOT;
			else if (Setting == "box")		pEntity->pev->solid = SOLID_BBOX;
			else if (Setting == "slidebox")	pEntity->pev->solid = SOLID_SLIDEBOX;
			else if (Setting == "trigger")	pEntity->pev->solid = SOLID_TRIGGER;
			UTIL_SetOrigin(pEntity->pev, pEntity->pev->origin);
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//settrans <player> <trans_name>
//- scope: server
//- sets the player's next respawn point, based on ms_player_spawn message property.
bool CScript::ScriptCmd_SetTrans(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//- settrans <target> <trans_name>
	//- tie a player to a spawn area
#ifdef VALVE_DLL
	/* notes from msarea_transition:
	strcpy( pPlayer->m_OldTransition, STRING(sName) );
	strcpy( pPlayer->m_NextMap, STRING(sDestMap) );
	strcpy( pPlayer->m_NextTransition, STRING(sDestTrans) );
	pPlayer->CurrentTransArea = this;
	strcpy( pPlayer->m_SpawnTransition, STRING(sName) );*/
	if ( Params.size() >= 2 )
	{
		CBaseEntity *pEntity = RetrieveEntity( Params[0] );
		if(pEntity->IsPlayer()) //Should never be null if player is in world
		{
			CBasePlayer* pPlayer = (CBasePlayer*)pEntity;
			if (pPlayer->m_SpawnTransition != NULL)
				strncpy(pPlayer->m_SpawnTransition, Params[1], 32);
		}
		else
		{
			MSErrorConsoleText( "ExecuteScriptCmd", UTIL_VarArgs("Script: %s, %s - settrans tried to affect non-player!\n", m.ScriptFile.c_str(), Cmd.Name().c_str() ) );
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//setvar <var> <data...> - set a variable prior to load
//setvard <var> <data...> - set a variable at run time
//setvarg <var> <data...> - set a global variable (server wide)
//local <var> <data...> - set a run time local variable (lives only within the event it was created)
//- scope: shared
bool CScript::ScriptCmd_SetVar(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	msstring sTemp;
	if (Params.size() >= 2)
	{
		msstring_ref VarName = Cmd.m_Params[1];
		msstring_ref VarValue = Params[1];

#if !TURN_OFF_ALERT
		//Thothie JUN2013_08 - check for conflicts in developer builds as we go
		msstring testvar = Cmd.m_Params[1];
		msstring testvar_type = Cmd.m_Params[0];
		msstring testvar_scope = "runtime";
		conflict_check(testvar, testvar_type, testvar_scope, 0);
#endif

		if (Params.size() >= 3)	//Add strings together
		{
			sTemp = "";
			for(int i = 0; i < Params.size() - 1; i++)
				sTemp += Params[i + 1];
			VarValue = sTemp;
		}

		if (Cmd.Name() == "local")
			Event.SetLocal(VarName, VarValue);
		else if (Cmd.Name() == "setvarg")
			SetVar(VarName, VarValue, true);
		else
			SetVar(VarName, VarValue, Event);
	}
	else
	{
		UTIL_VarArgs("Missing Param for: [%s] [%s] in [%s]\n", Cmd.Name().c_str(), Params[0].c_str(), m.ScriptFile.c_str());
		//MSErrorConsoleText( "CGenericItem::ExecuteScriptCmd", UTIL_VarArgs("Script: %s, %s - not enough parameters!\n", Script->m.ScriptFile.c_str(), Cmd.Name().c_str()) )
		//ERROR_MISSING_PARMS;
	}

	return true;
}

//setviewmodelprop <item> <property> <value>
//- scope: server
//- several properties available, including:
//- rendermode <0-5>
//- renderamt <0-255>
//- renderfx <0-16>
//- animspeed <ratio>
//- model <file>
//- submodel <group> <submodel>
bool CScript::ScriptCmd_SetViewModelProp(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//setviewmodelprop <target> <rendermode|renderamt|skin|submodel|model> <value> [value]
	//Shuriken MAR2008
	//edits/additions by MIB APR2008a
#ifdef VALVE_DLL
	if( Params.size( ) >= 3 )
	{
		CBaseEntity *pEnt = m.pScriptedEnt->RetrieveEntity( Params[0] );
		CGenericItem *pItem = pEnt && pEnt->IsMSItem() ? (CGenericItem *)pEnt : NULL;
		CBasePlayer *pPlayer = pItem ? (CBasePlayer *)pItem->Owner() : NULL;

		if( pPlayer && pItem->m_Location == ITEMPOS_HANDS )
		{
			MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_ITEM], NULL, pPlayer->pev );
			WRITE_BYTE( 7 );
			WRITE_STRING( Params[1].c_str() );
			WRITE_SHORT( pItem->m_Hand );

			if( Params[1] == "model" )
			{
				msstring Model = "models/";
				Model += Params[2];
				WRITE_STRING( Model );
			}
			else
			{
				//OCT2011_30 Thothie - adjustable viewanim speed
				if( Params[1] == "animspeed" )
				{
					WRITE_FLOAT( atof(Params[2].c_str()) );
				}
				else
				{
					WRITE_BYTE( atoi(Params[2]) );

					if( Params[1] == "submodel" || Params[1] == "submodelbody" ) //Thothie DEC2017_10 - aliasing this to the more common usage
						WRITE_BYTE( atoi(Params[3]) );
				}
			}

			MESSAGE_END();
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//setwearpos <clearall|position> <amt>
//- scope: uncertain?, player
//- Defines wear slots on player
//- Runs once on player spawn in player/player_sh_stats
bool CScript::ScriptCmd_SetWearPos(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 1)
	{
		if (m.pScriptedEnt && m.pScriptedEnt->IsPlayer())
		{
			CBasePlayer *pPlayer = (CBasePlayer *)m.pScriptedEnt;
			if (Params[0] == "clearall")
			{
				pPlayer->m_WearPositions.clearitems();
			}
			else if (Params.size() >= 2)
			{
				wearpos_t *pWearPos = NULL;
				for(int i = 0; i < pPlayer->m_WearPositions.size(); i++)
				{
					if (Params[0] != pPlayer->m_WearPositions[i].Name)
						continue;

					pWearPos = &pPlayer->m_WearPositions[i];
					break;
				}

				if (!pWearPos)	//Create new position
				{
					pPlayer->m_WearPositions.add(wearpos_t(Params[0], atoi(Params[1])));
				}
				else	//Change max slots in position
					pWearPos->MaxAmt = atoi(Params[1]);
			}
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//sound.play3d <sound> <volume> <origin> [attenuation:0.8] [pitch:100]
//svsound.play3d <sound> <volume> <origin> [attenuation:0.8] [pitch:100]
//- scope shared
//- plays sound at specific origin
bool CScript::ScriptCmd_SoundPlay3D(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 3)
	{	//<sound> <volume> <origin> [attenuation]
		float Attn = Params.size() >= 4 ? atof(Params[3]) : ATTN_NORM;
		int chan = Params.size() >= 5 ? atoi(Params[4]) : 0;
		float sPitch = Params.size() >= 6 ? atof(Params[5]) : PITCH_NORM;
		float Volume = atof(Params[1]) / 10;
		//gEngfuncs.pEventAPI->EV_PlaySound( 0, *(Vector *)&Origin, chan, Sound, Volume, Attn, 0, 100 );
		//EngineFunc::Shared_PlaySound3D( Params[0], Volume, StringToVec(Params[2]), Attn );
		//Thothie MAR2012 - functions to control the channel (so we can end looping sounds)
		//- also, this may work server side now
		EngineFunc::Shared_PlaySound3D2(Params[0], Volume, StringToVec(Params[2]), Attn, chan, sPitch);
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//sound.pm_play <sound> <volume> <channel>
//- scope: client
//- Plays player footsteps - can only be used in game_playermove
bool CScript::ScriptCmd_SoundPMPlay(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifndef VALVE_DLL
	//Can only be called from game_playermove
	//Plays a PM sound - On client, plays sound locally
	//                   On server, plays sound for all OTHER players

	if (Params.size() >= 3)
	{	//<sound> <volume> <channel>
		pmove->PM_PlaySound(atoi(Params[2]), Params[0], atof(Params[1]), ATTN_NORM, 0, PITCH_NORM);
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//sound.setvolume <chan> <sound> <volume>
//- scope: client
//- For altering sound on the gobal client channels.
bool CScript::ScriptCmd_SoundSetVolume(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifndef VALVE_DLL
	if (Params.size() >= 3)
	{	//<chan> <sound> <volume>
		if (m.pScriptedEnt)
		{
			EMIT_SOUND_DYN(m.pScriptedEnt->edict(), atoi(Params[0]), Params[1], atof(Params[2]) / 10.0f, ATTN_NORM, SND_CHANGE_PITCH | SND_CHANGE_VOL, PITCH_NORM);
			//EMIT_SOUND_DYN( m.pScriptedEnt->edict(), CHAN_STATIC, Params[1],
			//	atof(Params[2])/ 10.0f, ATTN_STATIC, SND_CHANGE_PITCH | SND_CHANGE_VOL, PITCH_NORM);
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//storeentity <type> <target>
//- scope: server
//- Intent seems to be to set one of reserved entity types (eg. ent_expowner), but doesn't function.
bool CScript::ScriptCmd_StoreEntity(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 1)
	{
		if (m.pScriptedEnt)
		{
			int EntType = EntityNameToType(Params[0]);
			if (EntType > -1)
			{
				CBaseEntity *pEntity = m.pScriptedEnt->RetrieveEntity(Params[1]);
				if (pEntity)
					m.pScriptedEnt->StoreEntity(pEntity, (enttype_e)EntType);
			}
			else ALERT(at_console, "Error  (%s), %s - '%s' not a valid entity type!\n", m.ScriptFile.c_str(), Cmd.Name().c_str(), Params[0].c_str());

		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//stradd <var> <string1> <string2>
//stradd <var> <string>
//- scope: shared
//- Concatenates strings
bool CScript::ScriptCmd_StrAdd(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	// MiB SEP2019_18 Rewrote to not use Params[0]
	//  This was causing screwiness with $eval when assembling a command
	//  string (e.g., stradd L_CMD ',L_PARAM') because the parent was
	//  resolving the value of L_CMD, which was an incomplete $func call,
	//  and causing it to return bad data
	if (Params.size() > 1)
	{
		msstring                        vsVarName = Cmd.m_Params[1];
		msstring                        vsOrgl = GetVar(vsVarName);
		if (vsVarName == vsOrgl)
		{
			vsOrgl = "";
		}
		if (Params.size() >= 3) SetVar(vsVarName, Params[1] + Params[2], Event);
		else if (Params.size() >= 2) SetVar(vsVarName, vsOrgl + Params[1], Event);
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//strconc <var> <string... string... string...>
//- scope; shared
//- concatenates strings
bool CScript::ScriptCmd_StrConc(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	// Thothie FEB2008a - concatenate strings
	// strconc <var> <string... string... string...>
	// concatenates both strings and litterals (eg. strconc OUT_STR I have $get(ent_me,hp) remaining! )
	msstring sTemp;
	if (Params.size() >= 2)
	{
		sTemp += Params[0];
		for(int i = 0; i < Params.size() - 1; i++)
		{
			if (i) sTemp += " ";
			sTemp += Params[i + 1];
		}
		SetVar(Cmd.m_Params[1], sTemp, Event);
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//splayviewanim <item> <idx>
//- scope: server
//- forces a player's view animation to play
bool CScript::ScriptCmd_SPlayViewAnim(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//MiB DEC2007a - server side view anim playing for more dynamic weapon control
	//splayviewanim <target_item> <index>
#ifdef VALVE_DLL
	if( Params.size() >= 2 )
	{
		CBaseEntity *pEnt = RetrieveEntity( Params[0] );
		CGenericItem *pItem = pEnt->IsMSItem() ? (CGenericItem *)pEnt : NULL;
		CBasePlayer *pPlayer = (CBasePlayer *)pItem->Owner();
		int iAnim = atoi(Params[1]);

		if( pPlayer && pItem->m_Location == ITEMPOS_HANDS )
		{
			MESSAGE_BEGIN( MSG_ONE , g_netmsg[NETMSG_ANIM], NULL, pPlayer->pev );
			WRITE_BYTE( iAnim );
			WRITE_BYTE( pItem->m_Hand );
			MESSAGE_END();
		}
	}
#endif

	return true;
}

//token.add <token_string> <value>
//- scope: shared
//- adds an entry to a token string
bool CScript::ScriptCmd_TokenAdd(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 2)
	{
		msstring &Token = Params[0];
		msstring &TokenAdd = Params[1];

		if (Token.len())	Token += ";";

		Token += TokenAdd;
		SetVar(Cmd.m_Params[1], Token, Event);
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//token.del <token_string> <idx>
//- scope: shared
//- removes an entry to a token string
bool CScript::ScriptCmd_TokenDel(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 2)
	{
		static msstringlist Tokens;
		Tokens.clearitems();
		int DelItem = atoi(Params[1]);
		msstring TokenStr;

		TokenizeString(Params[0], Tokens);

		if (DelItem >= 0 && DelItem < (signed)Tokens.size())
		{
			Tokens.erase(DelItem);

			for(int i = 0; i < Tokens.size(); i++)
			{
				if (TokenStr.len())	TokenStr += ";";
				TokenStr += Tokens[i];
			}

			SetVar(Cmd.m_Params[1], TokenStr, Event);
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//token.scramble <token_string>
//- scope: shared
//- randomizes entries in a token string
bool CScript::ScriptCmd_TokenScramble(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 1)
	{
		//token.scramble <token_string> - randomize entries
		static msstringlist Tokens;
		msstring new_tokens;

		Tokens.clearitems();
		TokenizeString(Params[0], Tokens);

		int n_loops = Tokens.size();

		for(int i = 0; i < n_loops; i++)
		{
			int r = RANDOM_LONG(0, Tokens.size() - 1);
			new_tokens += Tokens[r];
			new_tokens += ";";
			Tokens.erase(r);
		}
		SetVar(Cmd.m_Params[1], new_tokens, Event);
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//token.set <token_string> <idx> <newvalue>
//- scope: shared
//- sets an exiting entry in a token string
bool CScript::ScriptCmd_TokenSet(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//MiB DEC2007a - Alter a token in a string
	//token.set <token_string> <idx> <replacement_string>
	//Thothie - this is bugging out, going to try making it one word - I notice the odd Cmd.m_Params[1] thang we have going above
	if (Params.size() >= 2)
	{
		static msstringlist Tokens;
		Tokens.clearitems();
		int ChgItem = atoi(Params[1]);
		msstring TokenStr;

		msstring &TokenAdd = Params[2];

		TokenizeString(Params[0], Tokens);

		if (ChgItem >= 0 && ChgItem < (signed)Tokens.size())
		{
			for(int i = 0; i < Tokens.size(); i++)
			{
				if (TokenStr.len()) TokenStr += ";";

				if (i == ChgItem)
				{
					TokenStr += TokenAdd;
				}
				else
				{
					TokenStr += Tokens[i];
				}

				//TokenStr += ";";
			}
			SetVar(Cmd.m_Params[1], TokenStr, Event);
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//torandomspawn <player>
//- scope: server
//- Moves a player to a random spawn point within 256 units (for /stuck(
bool CScript::ScriptCmd_ToRandomSpawn(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//MIB JAN2010_20
	//torandomspawn <player>
	//Moves player to a random spawn withing 256 units
#ifdef VALVE_DLL
	if ( Params.size() >= 1 )
	{
		CBaseEntity *pTarget = RetrieveEntity( Params[0] );
		if ( pTarget )
		{
			mslist<CBaseEntity *> Spawnpoints;

			CBaseEntity *pSpot = NULL;
			while( pSpot = UTIL_FindEntityByClassname( pSpot, SPAWN_GENERIC ) )
			{
				if ( ( pSpot->pev->origin - pTarget->pev->origin ).Length() > 256 )
					continue;

				Spawnpoints.add( pSpot );
			}

			if ( Spawnpoints.size() )
			{
				int loc = RANDOM_LONG( 0 , Spawnpoints.size()-1 );
				pTarget->pev->origin = Spawnpoints[ loc ]->pev->origin;
			}
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//tospawn <player> [spawn]
//- scope: server
//- Moves a player to the specified [spawn], or to his default spawn point.
bool CScript::ScriptCmd_ToSpawn(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//tospawn <target> [tras_name] - return individual player to spawn
#ifdef VALVE_DLL
	if ( Params.size() >= 1 )
	{
		CBaseEntity *pEntity = RetrieveEntity( Params[0] );
		CBasePlayer *pPlayer = pEntity->IsPlayer() ? (CBasePlayer *)pEntity : NULL;
		if( pPlayer )
		{
			if ( Params.size() >= 2 ) strncpy(pPlayer->m_SpawnTransition, Params[1], 32);
			pPlayer->m_JoinType = 2;
			CBaseEntity *pSpawnSpot = pPlayer->FindSpawnSpot();
			UTIL_SetOrigin( pPlayer->pev, pSpawnSpot->pev->origin );
			pPlayer->pev->angles = pSpawnSpot->pev->angles;
		}
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//usetrigger <map_triggers...>
//- scope: server
//- fire one or more map triggers
bool CScript::ScriptCmd_UseTrigger(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
#ifdef VALVE_DLL
	if( Params.size() >= 1 )
	{
		for(int i = 0; i < Params.size(); i++)
			FireTargets( Params[i], m.pScriptedEnt, m.pScriptedEnt, USE_TOGGLE, 0 );
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

//vectoradd <vec> <vec>
//vectoradd <vec> <x|y|z> <amt>
//- scope: shared, math
//- add to vector or vector component
bool CScript::ScriptCmd_VectorAdd(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 2)			// <vec> <changevec> <addvec>
	{									// <vec> <x/y/z> <addvalue>
		Vector Result;
		if (Params[1] == "x")		Result = StringToVec(Params[0]) + Vector(atof(Params[2]), 0, 0);
		else if (Params[1] == "y") Result = StringToVec(Params[0]) + Vector(0, atof(Params[2]), 0);
		else if (Params[1] == "z") Result = StringToVec(Params[0]) + Vector(0, 0, atof(Params[2]));
		else
		{
			if (Params.size() < 3) Result = StringToVec(Params[0]) + StringToVec(Params[1]);
			else Result = StringToVec(Params[1]) + StringToVec(Params[2]);
		}

		SetVar(Cmd.m_Params[1], VecToString(Result), Event);
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//vectormultiply <vec> <vec>
//vectormultiply <vec> <x|y|z> <ratio>
//- scope: shared, math
//- multiply vector or vector component
bool CScript::ScriptCmd_VectorMultiply(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 2)				// <vec> <x/y/z> <multvalue>
	{
		Vector Result;
		if (Params[1] == "x")		Result = VecMultiply(StringToVec(Params[0]), Vector(atof(Params[2]), 0, 0));
		else if (Params[1] == "y") Result = VecMultiply(StringToVec(Params[0]), Vector(0, atof(Params[2]), 0));
		else if (Params[1] == "z") Result = VecMultiply(StringToVec(Params[0]), Vector(0, 0, atof(Params[2])));
		else
		{
			if (Params.size() < 3)
			{
				if (isdigit(Params[1][0])) Result = StringToVec(Params[0]) * atof(Params[1]);
				else Result = VecMultiply(StringToVec(Params[0]), StringToVec(Params[1]));
			}
			else
			{
				if (isdigit(Params[1][0])) Result = StringToVec(Params[1]) * atof(Params[2]);
				else Result = VecMultiply(StringToVec(Params[1]), StringToVec(Params[2]));
			}
		}

		SetVar(Cmd.m_Params[1], VecToString(Result), Event);
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//vectorset <vec> <x|y|z> <value>
//- scope: shared, math
//- set a vector component
bool CScript::ScriptCmd_VectorSet(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 2)
	{
		Vector ModifyVec = StringToVec(Params[0]);
		float *Prop = NULL;
		if (Params[1] == "x")		Prop = &ModifyVec.x;
		else if (Params[1] == "y") Prop = &ModifyVec.y;
		else if (Params[1] == "z") Prop = &ModifyVec.z;
		else MSErrorConsoleText("CScript::ScriptCmd_VectorSet", UTIL_VarArgs("Script: %s, %s - '%s' not a valid vector coordinate!\n", m.ScriptFile.c_str(), Cmd.Name().c_str(), Params[1].c_str()));

		if (Prop) *Prop = atof(Params[2]);

		SetVar(Cmd.m_Params[1], VecToString(ModifyVec), Event);
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//setvelocity <target> <vec> ["override"]
//addvelocity <target> <vec> ["override"]
//- scope: server
//- set or add to current velocity
//- override causes this to ignore push resistance/immunity
bool CScript::ScriptCmd_Velocity(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//Parameters: <target> <velocity> [override] (override flag means push even if immune to push)
	if (Params.size() >= 2)
	{
		//Thothie JAN2020_12 - got a EXEC_CMD, noticed tabbing was odd here, doing possibly redundant checks
		if (m.pScriptedEnt)
		{
			//APR2012_01 - Thothie - Add Push Reduction via Var
			Vector lVelAdjust = StringToVec(Params[1]);
			CBaseEntity *pEntity = m.pScriptedEnt->RetrieveEntity(Params[0]);

			//Thothie APR2012_01 - fix bit where item owner's push val affects ability to push
			CBaseEntity *pCaller = NULL;
			if (m.pScriptedEnt->IsMSItem())
			{
				CGenericItem* pItem = (CGenericItem*)m.pScriptedEnt;
				if (pItem) // Thothie JAN2020_12 - EXEC_CMD redundancy
				{
					if (pItem->m_pOwner)	
						pCaller = pItem->m_pOwner;
				}
			}
			else
				pCaller = m.pScriptedEnt;

			if (pEntity != NULL)
			{
				bool abort_push = false;
				if ((pEntity != pCaller) && Params.size() >= 3 && (Params[2] != "override"))
				{
					CMSMonster* pMonster = (pEntity->IsMSMonster() ? (CMSMonster*)pEntity : NULL);
					if (pMonster)
					{
						for (int i = 0; i < pMonster->m.TakeDamageModifiers.size(); i++)
						{
							CMSMonster::takedamagemodifier_t& TDM = pMonster->m.TakeDamageModifiers[i];
							msstring read_dmgtype = TDM.DamageType;
							if (read_dmgtype.contains("stun"))
							{
								lVelAdjust.x *= TDM.modifier;
								lVelAdjust.y *= TDM.modifier;
								lVelAdjust.z *= TDM.modifier;
							}
						}

						if (pMonster->m_nopush) 
							abort_push = true; //Thothie JAN2020_12 - EXEC_CMD redundancy
					}
				}
				if (!pEntity->IsAlive()) abort_push = true;
				if (!abort_push && pEntity->pev) //[/thothie]
				{
					if (Cmd.Name() == "setvelocity") pEntity->pev->velocity = lVelAdjust; //StringToVec( Params[1] );
					else pEntity->pev->velocity += lVelAdjust; //StringToVec( Params[1] );
				}
			}
		}
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//volume <0-10>
//- scope: shared
//- Deperciated. Sets volume for next playsound command.
bool CScript::ScriptCmd_Volume(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 1)
	{
		m.pScriptedEnt->SndVolume = atof(Params[0]) * 0.1;
		if (m.pScriptedEnt->SndVolume < 0) m.pScriptedEnt->SndVolume = 0;
		if (m.pScriptedEnt->SndVolume > 1) m.pScriptedEnt->SndVolume = 1;
	}
	else ERROR_MISSING_PARMS;

	return true;
}

//weight <float>
//- scope: uncertain?
//- Sets weight of items (and should likely be moved to generic item commands.)
bool CScript::ScriptCmd_Weight(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	if (Params.size() >= 1)
		m.pScriptedEnt->m_Weight = atof(Params[0]);
	else ERROR_MISSING_PARMS;
	return true;
}

//wipespell <player> <spell_idx>
//- scope: server
//- Erases spell data slot.
bool CScript::ScriptCmd_WipeSpell(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//MiB Dec2007a - for wiping single spells.
#ifdef VALVE_DLL
	// wipespell <target> <spell_idx>
	if( Params.size() >= 2 )
	{
		CBaseEntity *pEnt = RetrieveEntity( Params[0] );
		CBasePlayer *pPlayer = pEnt->IsPlayer() ? (CBasePlayer *)pEnt : NULL;

		int spellidx = atoi( Params[1] );
		if( pPlayer && spellidx < (signed)pPlayer->m_SpellList.size() )
		{
			pPlayer->m_SpellList.erase( spellidx );
			MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_SETPROP], NULL, pPlayer->pev );
			WRITE_BYTE( PROP_SPELL );
			WRITE_BYTE( 0 ); // 1 - change spell, 0 - erase spell, -1 - erase all spells
			WRITE_BYTE( spellidx );
			MESSAGE_END();
		}
	}
	else ERROR_MISSING_PARMS;
#endif
	return true;
}

//writeline <filename> <line> ["overwrite"]
//- scope: server
//- creates, writes, or appends to a file on the server
bool CScript::ScriptCmd_WriteLine(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//MiB FEB2008a - Scriptside File I/O
	//writeline <filename> <line>
	//#ifdef VALVE_DLL //Thothie MAR2019_02 - MiB says removing this will allow client file access
	if (Params.size() >= 2)
	{
		msstring fileName = Params[0];
		msstring lineToWrite = Params[1];

		int ref = -1;
		for(int i = 0; i < m.pScriptedEnt->filesOpen.size(); i++)
		{
			if (fileName == m.pScriptedEnt->filesOpen[i].fileName)
			{
				ref = i;
				break;
			}
		}

		if (ref == -1)
		{
			ref = m.pScriptedEnt->filesOpen.size();
			m.pScriptedEnt->filesOpen.add(scriptfile_t() = fileName);
		}

		if (Params.size() >= 3)
		{
			int lineNum = atoi(Params[2]);
			bool overwrite = false;
			if (Params.size() >= 4)
				overwrite = Params[3] == "overwrite";

			m.pScriptedEnt->filesOpen[ref].ScriptFile_WriteLine(lineToWrite, lineNum, overwrite);
		}
		else
			m.pScriptedEnt->filesOpen[ref].ScriptFile_WriteLine(lineToWrite);
	}
	//#endif

	return true;
}

//xdodamage <target|(origin)> <range|aoe|(destination)|"direct"> <damage> <cth|fall_off> <attacker> <inflciter> <skill|none> <dmg_type> [flag_string]
//- scope: server
//- external dynamic dodamage command
//- if first and second parameters are both origins, traceline damage between the two points is assumed (sadly, stops at the first impact)
//- if first parameter is an origin, and second is a number, AOE (area of effect), chance to hit is 100%, and fall_off is used (0.1 = 10% dmg reduct per 10% distance from center.)
//- if second parameter is 'direct' then direct damage occurs to <target> from <attacker>
//- if first param is an entity and the second is not 'direct', then auto-aimed hitscan damage, the second param defining the max range of the attack.
//- <inflictor> and <attacker> should usually match, save when used in weapons or projectiles, in which case <inflicter> should indicate said items.
//- [flag_string] - multiple flags can be added, if seperated by semi-colons, flags follow:
//-- � "dmgevent:<prefix>"
//-- You can use this to setup seperate _dodamage processing events for each attack
//-- This will call <prefix>_dodamage, in addition to the usual game_dodamage, on the <attacker>
//-- If prefix begins with * - <prefix>_dodamage will be called on <inflictor> instead of <attacker>, sans the * (for weapons)
//� "nodecal"
//-- Causes trace damage events not to decal walls (note that they still fire hitwall/game_hitworld when calling from an item/weapon script)
bool CScript::ScriptCmd_XDoDamage(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	//Future replacement for dodamage
	//XDODAMAGE <target|(origin)> <range|aoe|(destination)|direct> <damage> <cth|fall_off> <attacker> <inflciter> <skill|none> <dmg_type> [flag_string]
#ifdef VALVE_DLL
	if( Params.size() >= 7 )
	{
		damage_t Damage;
		SetBits( Damage.iDamageType, DMG_GENERIC|DMG_SIMPLEBBOX );
		Damage.flDamage = atof(Params[2]);
		CBaseEntity *pAttacker = RetrieveEntity( Params[4] );
		CMSMonster *pMonster = (CMSMonster *)pAttacker;
		CBaseEntity *pTarget = NULL;
		Damage.pInflictor  = RetrieveEntity( Params[5] );
		Damage.pAttacker = pAttacker;
		//Thothie JUN2010_14 - make sure dead players don't do damage (may crash server)
		if ( !Damage.pInflictor ) return 0;
		if ( !Damage.pAttacker ) return 0;
		if ( Damage.pInflictor->IsPlayer() )
		{
			if ( !Damage.pInflictor->IsAlive() ) return 0;
		}
		if ( Damage.pAttacker->IsPlayer() )
		{
			if ( !Damage.pAttacker->IsAlive() ) return 0;
		}

		Damage.sDamageType = Params[7];
		if (  pMonster->m_DMGMulti > 0 ) Damage.flDamage *= pMonster->m_DMGMulti;
		if (  pMonster->m_HITMulti > 0 ) Damage.flHitPercentage *= pMonster->m_HITMulti; //FEB2009_18

		if( Params.size() >= 8 )
		{
			//Thothie OCT2011_04
			//process flags (currently only flag is dmgevent:<prefix>)
			msstringlist dflags;
			TokenizeString(Params[8],dflags);
			for(int i = 0; i < dflags.size(); i++)
			{
				msstring dflag = dflags[i].c_str();
				if ( dflag.starts_with("dmgevent:") )
				{
					Damage.dodamage_event = dflag.substr( 9 );
				}

				if ( dflag.starts_with("nodecal") )
				{
					Damage.nodecal = true;
				}
			}
		}

		//future use
		//- use this so damage from summon creatures report proper "Your skeleton does xx" or "Your lightning storm does xx" etc.
		//if ( Damage.pInflictor != Damage.pAttacker ) Damage.isSummon

		//AOE attack
		if ( Params[0][0] == '(' && Params[1][0] != '(' )
		{
			Damage.flRange = Damage.flDamageRange = atof(Params[1]);
			Vector Location;
			Location = StringToVec( Params[0] );
			Damage.vecSrc = Location;
			Damage.vecEnd = Damage.vecSrc;
			Damage.flDamageRange = atof(Params[1]);
			Damage.flAOEAttn = atof(Params[3]);
			Damage.flAOERange = Damage.flDamageRange;
			Damage.flHitPercentage = 100.0;
			Damage.vecEnd = Damage.vecSrc;
			SetBits( Damage.iDamageType, DMG_REFLECTIVE );
		}
		//vector to vector attack
		else if ( Params[0][0] == '(' && Params[1][0] == '(' )
		{
			//Damage.flRange = Damage.flDamageRange = atof(Params[1]);
			//pTarget = RetrieveEntity( Params[0] );
			Damage.vecSrc = StringToVec(Params[0]);
			Damage.vecEnd = StringToVec(Params[1]);
			Damage.flDamageRange = (Damage.vecSrc-Damage.vecEnd).Length( );
			Damage.flHitPercentage = atof(Params[3]);
		}
		//Direct Attack
		else if ( Params[1] == "direct" )
		{
			Damage.flAOERange = 0;
			Damage.pDirectDmgTarget = RetrieveEntity( Params[0] );
			Damage.flHitPercentage = atof(Params[3]);
			SetBits( Damage.iDamageType, DMG_DIRECT );
		}
		else
			//Hitscan
		{
			//FEB2010_28 - Thothie, compensate for monster width when dealing with ranges
			//- be good to add target width too, but did that for regular dodamage, xdodamage is rarely used this way, and lazy
			float flDamageRangeParam = atof(Params[1]);
			flDamageRangeParam += pMonster->m_Width / 2;
			Damage.flRange = Damage.flDamageRange = flDamageRangeParam;
			Vector vForward;
			//pTarget = RetrieveEntity( Params[0] );
			Damage.vecSrc = pMonster->EyePosition();
			Damage.flDamageRange = atof(Params[1]);
			UTIL_MakeVectorsPrivate( pMonster->pev->v_angle, vForward, NULL, NULL );
			Damage.vecEnd = pMonster->EyePosition() + vForward * Damage.flDamageRange;
			Damage.flHitPercentage = atof(Params[3]);
		}

		//set xp skill (if any)
		if ( Params[6] != "none" )
		{
			if ( Damage.pAttacker->IsPlayer() )
			{
				int Stat = 0, Prop = -1;
				GetStatIndices( Params[6], Stat, Prop );
				Damage.ExpUseProps = true;
				Damage.ExpProp = Prop;
				Damage.ExpStat = Stat;
				//ALERT( at_aiconsole, "XEBUG: Skill/prop: %i/%i \n", Stat,Prop );
			}
		}

		hitent_list Hits;
		DoDamage( Damage, Hits );
	}
	else ERROR_MISSING_PARMS;
#endif

	return true;
}

bool CScript::Script_ExecuteCmd(SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params)
{
	bool fParentSuccess = false;
	if (m.pScriptedEnt)
		fParentSuccess = m.pScriptedInterface->Script_ExecuteCmd(this, Event, Cmd, Params);

	if (fParentSuccess || Event.bFullStop) // MiB 07DEC_2014 - "exit" command
		return fParentSuccess;

	bool bFound = false;
	msfunchash_t::iterator iCmd = m_GlobalCmdHash.find(Cmd.Name());
	if (iCmd != m_GlobalCmdHash.end())
	{
		return (this->*(iCmd->second.GetFunc()))(Event, Cmd, Params);
	}

	return true;
}

//MiB Feb2008a (This goes on for a ways)

//InFile class
//==================================
//Class for inputting a file with minimal handle time
class InFile : public std::ifstream
{
public:
	msstring buffered;

	void Open(msstring_ref FileName)
	{
		std::ifstream::open(FileName);
		buffered = "";
	};

	void Delete(msstring fileName)
	{
		if (is_open())
		{
#ifdef _WIN32
			::DeleteFile(fileName);
#else
			remove(fileName);
#endif
		}			
	};

	void Close() { std::ifstream::close(); };

	msstring ReadLine()
	{
		if (!is_open()) return "[FILE_NOT_FOUND]";
		if (std::ifstream::eof()) return "[EOF]";

		char line[256];
		std::ifstream::getline(line, 256);

		msstring theReturn = line;
		if (std::ifstream::eof()) //If this is the end of file
		if (theReturn == "") //And there was nothing on this line
			theReturn = "[EOF];"; //Mark it as the end of the file

		return theReturn;
	}
};
//==================================


//scriptfile_t Reading
//==================================

//Read the next line of the file
msstring scriptfile_t::ScriptFile_ReadLine()
{

	if (nofile) return "[FILE_NOT_FOUND]";

	if (endoffile) //If you reached the end of file last read, reset
	{
		Reset();
		if (endoffile) //If it's still eof after the reset, there's nothing to read at all.
			return "[EOF]";
	}

	msstring result = ScriptFile_ReadLine(curline++);

	if (result == "[EOF]")
		endoffile = true;

	return result;
}

//Read a specified line from the file
msstring scriptfile_t::ScriptFile_ReadLine(int lineNum)
{
	if (nofile) return "[FILE_NOT_FOUND]";

	msstring theReturn;
	if (lineNum < (signed)Lines.size())
		theReturn = Lines[lineNum];
	else
		theReturn = "[EOF]";
	return theReturn;
}
//==================================


//scriptfile_t Writing

//==================================
//Append a line
void scriptfile_t::ScriptFile_WriteLine(msstring line)
{
	//ScriptFile_WriteLine( line , Lines.size() );
	AddLine(line, -1, false);

	char cFileName[MAX_PATH];
	_snprintf(cFileName, sizeof(cFileName), "%s/%s", EngineFunc::GetGameDir(), fileName.c_str());
	Logger mibfile;
	mibfile.open(cFileName, 1);
	mibfile << line << "\n";

	mibfile.close();
}
//Write a line at X, possibly overwriting or just inserting
void scriptfile_t::ScriptFile_WriteLine(msstring line, int lineNum, bool overwrite)
{
	char cFileName[MAX_PATH];
	_snprintf(cFileName, sizeof(cFileName), "%s/%s", EngineFunc::GetGameDir(), fileName.c_str());
	Logger mibfile;
	mibfile.open(cFileName, 0);

	AddLine(line, lineNum, overwrite);

	//Write all the lines to the specified file
	for(int i = 0; i < Lines.size(); i++)
	{
		mibfile << Lines[i]; //<< ";"; //Add the necessary ';' for rereads
		if (i != (signed)Lines.size() - 1) //If this isn't the last line
			mibfile << "\n"; //Add a line break
	}

	mibfile.close(); //Close the file, if not for making sure that changes save, then for making sure we don't get overlapping handles
}
//==================================


//Easy way to open files
scriptfile_t &scriptfile_t::operator=(const msstring_ref a)
{
	/* Example:
		scriptfile_t file;
		file = "test.txt";
		or
		file = "logs/log_msdll.txt";
		*/
	Open(a);
	return *this;
}

//Open a specified file and input its lines for later "reading"
void scriptfile_t::Open(msstring_ref a)
{
	char cFileName[MAX_PATH];
#ifdef VALVE_DLL
	GET_GAME_DIR( cFileName );
#else
	strncpy(cFileName, gEngfuncs.pfnGetGameDirectory(), sizeof(cFileName));
#endif
	msstring vsFullPath = cFileName;
	vsFullPath += "/";
	vsFullPath += a;

	fileName = a;
	InFile file;
	file.Open(vsFullPath);
	int count = -1; //Has to be -1 for 0 on the first pass - done like this so that...
	do{
		count++; //Keep the Lines' last element index even with the conditional statements' count
		Lines.add(file.ReadLine()); //Read each line into the list
	} while (Lines[count] != "[EOF]" && Lines[count] != "[FILE_NOT_FOUND]");

	file.Close(); //Prevents overlapping handles.

	curline = 0; //Set the current line to 0 - this is for the ReadLine() function that goes line-by-line through the file

	nofile = Lines[0] == "[FILE_NOT_FOUND]";
	Lines.erase(Lines.size() - 1); //Get rid of the "[EOF]" or "[FILE_NOT_FOUND]" token

	endoffile = Lines.size() == 0 || (Lines.size() == 1 && Lines[0] == ""); //It's possible that the file exists, but has no text, so let the scriptfile know if it's already at EOF

}

//Reset the file for reading again
void scriptfile_t::Reset()
{
	curline = 0;
	endoffile = Lines.size() == 0 || (Lines.size() == 1 && Lines[0] == "");
}

void scriptfile_t::AddLine(msstring line, int lineNum, bool overwrite)
{
	if (lineNum == -1)
	{
		Lines.add(line);
		return;
	}


	if (Lines[0] == "" && Lines.size() == 1) //Don't ask...
		Lines[0] = line;
	else if (lineNum < (signed)Lines.size())
	{
		if (overwrite)
			Lines[lineNum] = line;
		else
		{
			//Insert the line
			int lineNumTemp = lineNum;
			msstringlist LinesTemp;
			for(int i = 0; i < Lines.size(); i++)
			{
				if (i == lineNumTemp) //You've found the correct line
				{
					LinesTemp.add(line); //Add it in the proper place
					i--; //Go back so you will write the original line next
					lineNumTemp = -1; //Don't find the line infinitely
				}
				else
				{
					//Not the right line - add it back normally
					LinesTemp.add(Lines[i]);
				}
			}
			Lines = LinesTemp; //Overwrite the old list with the properly generated one
		}
	}
	else
		Lines.add(line);
}
//Finally, end of MiB Feb2008a file handling
