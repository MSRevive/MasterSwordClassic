find_package(CURL REQUIRED)

# target_include_directories(libcurl INTERFACE ${BASEDIR}/thirdparty/libcurl)

# if (WIN32)
# 	set_target_properties(libcurl PROPERTIES IMPORTED_IMPLIB "${BASEDIR}/lib/public/libcurl.lib")
# else()
# 	set_target_properties(libcurl PROPERTIES IMPORTED_IMPLIB "${BASEDIR}/lib/public/linux-x86/libcurl.lib")
# endif()

add_library(server SHARED $<$<PLATFORM_ID:Windows>:ms.def>)

set_target_properties(server PROPERTIES OUTPUT_NAME "ms")

set_common_properties(server)

target_link_libraries(server PRIVATE
	$<$<PLATFORM_ID:Windows>:Ws2_32>
	$<$<PLATFORM_ID:Windows>:Wldap32>
	$<$<PLATFORM_ID:Windows>:Crypt32>
	$<$<PLATFORM_ID:Windows>:Normaliz>
	CURL::libcurl)

# Include directories
target_include_directories(server PRIVATE 
	${BASEDIR}/src/public
	${BASEDIR}/src/public/engine
	${BASEDIR}/src/common
	${BASEDIR}/src/game/client #defeats the purpose of shared, but that's MSR for ya.
	${BASEDIR}/src/game/shared
	${BASEDIR}/src/game/shared/movement
	${BASEDIR}/src/game/shared/ms
	${BASEDIR}/src/game/shared/stats
	${BASEDIR}/src/game/shared/syntax
	${BASEDIR}/src/game/shared/weapons
	${BASEDIR}/thirdparty
	${CMAKE_CURRENT_SOURCE_DIR}
	effects
	gamerules
	hl
	monsters
	player
	fn)

target_compile_definitions(server PRIVATE
	VERSION_SAFE_STEAM_API_INTERFACES
	VALVE_DLL
	CURL_STATICLIB)

if (UNIX)
	target_compile_definitions(server PRIVATE
		SWDS)
endif(UNIX)

if (MSVC)
	set_target_properties(server PROPERTIES 
		VS_DEBUGGER_WORKING_DIRECTORY "${VS_DEBUG_WORK_DIR}"
		VS_DEBUGGER_COMMAND "${VS_DEBUG_WORK_DIR}/hlds.exe"
		VS_DEBUGGER_COMMAND_ARGUMENTS "${VS_DEBUG_CMD_ARGS}")
endif()



################################################################################
# Source groups
################################################################################
set(Common
	"../../common/const.h"
	"../../common/cstringpool.cpp"
	"../../common/cstringpool.h"
	"../../common/Platform.h"
	"../../common/PlatformWin.h"
)
source_group("Common" FILES ${Common})

set(Engine
	"../../public/engine/progdefs.h"
)
source_group("Engine" FILES ${Engine})

set(SDK
	"../../public/FileSystem.h"
	"../../public/interface.cpp"
	"../../public/interface.h"
)
source_group("SDK" FILES ${SDK})

set(HL1
	"basemonster.h"
	"bmodels.cpp"
	"cdll_dll.h"
	"client.cpp"
	"effects.cpp"
	"enginecallback.h"
	"func_break.cpp"
	"game.cpp"
	"gamerules/gamerules.cpp"
	"gamerules/gamerules.h"
	"gamerules/multiplay_gamerules.cpp"
	"ggrenade.cpp"
	"h_export.cpp"
	"hl/animating.cpp"
	"hl/animation.cpp"
	"hl/buttons.cpp"
	"hl/cbase.cpp"
	"hl/cbase.h"
	"hl/defaultai.cpp"
	"hl/doors.cpp"
	"hl/explode.cpp"
	"hl/extdll.h"
	"hl/func_tank.cpp"
	"hl/game.h"
	"hl/globals.cpp"
	"hl/lights.cpp"
	"hl/monsters.cpp"
	"hl/monsters.h"
	"hl/monsterstate.cpp"
	"hl/nodes.cpp"
	"hl/pathcorner.cpp"
	"hl/plane.cpp"
	"hl/plats.cpp"
	"hl/schedule.cpp"
	"hl/scripted.cpp"
	"hl/skill.cpp"
	"hl/sound.cpp"
	"hl/soundent.cpp"
	"hl/soundent.h"
	"hl/spectator.cpp"
	"hl/subs.cpp"
	"hl/util.cpp"
	"hl/util.h"
	"monsters/combat.cpp"
	"serverfindentities.cpp"
	"serverfindentities.h"
	"shurispritedefs.h"
	"triggers.cpp"
	"world.cpp"
	"../shared/movement/pm_defs.h"
	"../shared/movement/pm_math.cpp"
	"../shared/movement/pm_movevars.h"
	"../shared/voice_gamemgr.cpp"
)
source_group("HL1" FILES ${HL1})

set(MS
	"effects/mseffects.cpp"
	"entity.cpp"
	"gamerules/team.cpp"
	"magic.cpp"
	"monsters/bodyparts/bodyparts.cpp"
	"monsters/bodyparts/bodyparts_human.cpp"
	"monsters/corpse.cpp"
	"monsters/monsteranimation.cpp"
	"monsters/msmonsterserver.cpp"
	"monsters/npcact.cpp"
	"monsters/npcglobal.cpp"
	"monsters/npcscript.cpp"
	"msmapents.cpp"
	"player/player.cpp"
	"player/playerstats.cpp"
	"shield.cpp"
	"store.cpp"
	"sv_character.cpp"
	"svglobals.cpp"
	#"SteamServerHelper.cpp"
	"../shared/movement/pm_shared.cpp"
	"../shared/movement/pm_shared.h"
	"../shared/ms/msmonstershared.cpp"
	"../shared/ms/netcodeshared.cpp"
	"../shared/stats/races.cpp"
	"../shared/syntax/syntax.cpp"
)
source_group("MS" FILES ${MS})

set(MS_Headers
	"effects/mseffects.h"
	"gamerules/teams.h"
	"monsters/bodyparts/bodyparts.h"
	"monsters/bodyparts/bodyparts_human.h"
	"monsters/corpse.h"
	"monsters/monsteranimation.h"
	"monsters/msmonster.h"
	"msdllheaders.h"
	"msgamerules.h"
	"msitemdefs.h"
	"player/modeldefs.h"
	"player/player.h"
	"shield.h"
	"store.h"
	"svglobals.h"
	#"SteamServerHelper.h"
	"../shared/ms/groupfile.h"
	"../shared/ms/mscharacter.h"
	"../shared/ms/mscharacterheader.h"
	"../shared/ms/titles.h"
	"../shared/ms/votedefs.h"
	"../shared/stats/races.h"
	"../shared/stats/statdefs.h"
	"../shared/stats/stats.h"
)
source_group("MS\\Headers" FILES ${MS_Headers})

set(FN
	"fn/CreateCharacterReq.cpp"
	"fn/CreateCharacterReq.h"
	"fn/DeleteCharacterReq.cpp"
	"fn/DeleteCharacterReq.h"
	"fn/FNSharedDefs.cpp"
	"fn/FNSharedDefs.h"
	"fn/LoadCharacterReq.cpp"
	"fn/LoadCharacterReq.h"
	"fn/UpdateCharacterReq.cpp"
	"fn/UpdateCharacterReq.h"
	"fn/ValidateConReq.cpp"
	"fn/ValidateConReq.h"
	"fn/ValidateMapReq.cpp"
	"fn/ValidateMapReq.h"
	"fn/ValidateScriptsReq.cpp"
	"fn/ValidateScriptsReq.h"
	"fn/RequestManager.cpp"
	"fn/RequestManager.h"
	"fn/HTTPRequest.cpp"
	"fn/HTTPRequest.h"
)
source_group("FN" FILES ${FN})

set(Shared
	"player/playershared.cpp"
	"../shared/filesystem_shared.cpp"
	"../shared/filesystem_shared.h"
	"../shared/ms/buildcontrol.h"
	"../shared/ms/dbginputs.cpp"
	"../shared/ms/findentities.cpp"
	"../shared/ms/findentities.h"
	"../shared/ms/global.cpp"
	"../shared/ms/global.h"
	"../shared/ms/globalmem.cpp"
	"../shared/ms/groupfile.cpp"
	"../shared/ms/iscript.h"
	"../shared/ms/logger.h"
	"../shared/ms/magic.h"
	"../shared/ms/msbasic.h"
	"../shared/ms/mscharacter.cpp"
	"../shared/ms/msdebug.h"
	"../shared/ms/msfileio.cpp"
	"../shared/ms/msfileio.h"
	"../shared/ms/saytext.h"
	"../shared/ms/script.cpp"
	"../shared/ms/script.h"
	"../shared/ms/scriptcmds.cpp"
	"../shared/ms/scriptedeffects.cpp"
	"../shared/ms/scriptedeffects.h"
	"../shared/ms/scriptmgr.cpp"
	"../shared/ms/scriptmgr.h"
	"../shared/ms/sharedutil.cpp"
	"../shared/ms/sharedutil.h"
	"../shared/ms/stackstring.cpp"
	"../shared/ms/stackstring.h"
	"../shared/ms/storeshared.h"
	"../shared/ms/vgui_menudefsshared.h"
	"../shared/stats/stats.cpp"
	"../shared/strhelper.cpp"
	"../shared/strhelper.h"
)
source_group("Shared" FILES ${Shared})

set(Shared_CRC
	"../shared/ms/crc/checksum_crc.cpp"
	"../shared/ms/crc/checksum_crc.h"
	"../shared/ms/crc/crchash.cpp"
	"../shared/ms/crc/crchash.h"
)
source_group("Shared\\CRC" FILES ${Shared_CRC})

set(Shared_Generic_Item
	"../shared/genitemlist.cpp"
	"../shared/genitemlist.h"
	"../shared/weapons/genericitem.cpp"
	"../shared/weapons/genericitem.h"
	"../shared/weapons/giarmor.cpp"
	"../shared/weapons/giattack.cpp"
	"../shared/weapons/gidrinkable.cpp"
	"../shared/weapons/gipack.cpp"
	"../shared/weapons/giprojectile.cpp"
	"../shared/weapons/gispawn.cpp"
	"../shared/weapons/gispell.cpp"
	"../shared/weapons/weapons.cpp"
	"../shared/weapons/weapons.h"
)
source_group("Shared\\Generic Item" FILES ${Shared_Generic_Item})

set(Shared_base64
	"../../../thirdparty/base64/base64.cpp"
	"../../../thirdparty/base64/base64.h"
)
source_group("Shared\\base64" FILES ${Shared__base64})

set(ALL_FILES
	${Common}
	${Engine}
	${SDK}
	${HL1}
	${MS}
	${MS_Headers}
	${FN}
	${Shared}
	${Shared_CRC}
	${Shared_Generic_Item}
	${Shared_base64}
)

target_sources(server PRIVATE ${ALL_FILES})

install(TARGETS server
	DESTINATION "msr/dlls"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
	GROUP_READ GROUP_EXECUTE
	WORLD_READ WORLD_EXECUTE)
