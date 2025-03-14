add_executable(maphash)

################################################################################
# Target Properties
################################################################################
set_common_properties(maphash)

target_include_directories(maphash PRIVATE
	${BASEDIR}/../src/common
	${BASEDIR}/../src/game/server
	${BASEDIR}/../src/game/server/hl
	${BASEDIR}/../src/game/shared
	${BASEDIR}/../src/game/shared/ms
	${BASEDIR}/../src/game/shared/ms/crc
	${BASEDIR}/../src/public
	${BASEDIR}/../src/public/engine
	${BASEDIR}/../thirdparty
	${CMAKE_CURRENT_SOURCE_DIR})

################################################################################
# Source groups
################################################################################
set(Source_Files
	"maphash.cpp"
	"../../src/game/shared/ms/crc/checksum_crc.cpp"
	"../../src/game/shared/ms/crc/checksum_crc.h"
	"../../src/game/shared/ms/crc/crchash.cpp"
	"../../src/game/shared/ms/crc/crchash.h"
	"../../src/game/shared/ms/msfileio.cpp"
	"../../src/game/shared/ms/stackstring.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
	${Source_Files}
)

target_sources(maphash PRIVATE ${ALL_FILES})