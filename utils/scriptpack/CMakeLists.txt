add_executable(scriptpack)

################################################################################
# Target Properties
################################################################################
set_common_properties(scriptpack)

target_include_directories(scriptpack PRIVATE
	${BASEDIR}/src/game/server
	${BASEDIR}/src/game/shared
	${BASEDIR}/src/game/shared/ms
	${BASEDIR}/thirdparty
	${CMAKE_CURRENT_SOURCE_DIR}
	tclap)

################################################################################
# Source groups
################################################################################
set(Source_Files
	"cbase.h"
	"compat.h"
	"dirent.h"
	"packer.cpp"
	"packer.h"
	"parser.h"
	"scriptpack.cpp"
	"../../src/game/shared/ms/crc/checksum_crc.cpp"
	"../../src/game/shared/ms/crc/checksum_crc.h"
	"../../src/game/shared/ms/crc/crchash.cpp"
	"../../src/game/shared/ms/crc/crchash.h"
	"../../src/game/shared/ms/groupfile.cpp"
	"../../src/game/shared/ms/groupfile.h"
	"../../src/game/shared/ms/msfileio.cpp"
	"../../src/game/shared/ms/stackstring.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
	${Source_Files}
)

target_sources(scriptpack PRIVATE ${ALL_FILES})