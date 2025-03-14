add_executable(charimport)

################################################################################
# Target Properties
################################################################################
add_library(curl SHARED IMPORTED)

target_include_directories(curl INTERFACE ${BASEDIR}/thirdparty/curl)

if (WIN32)
	set_target_properties(curl PROPERTIES IMPORTED_IMPLIB "${BASEDIR}/lib/public/libcurl.lib")
endif(WIN32)

set_common_properties(charimport)

target_link_libraries(charimport PRIVATE curl)

target_include_directories(charimport PRIVATE
	${BASEDIR}/src/game/shared/ms
	${BASEDIR}/thirdparty/base64
	${CMAKE_CURRENT_SOURCE_DIR})

################################################################################
# Source groups
################################################################################
set(Source_Files
	"charimport.cpp"
	"../../src/game/shared/ms/msfileio.cpp"
	"../../src/game/shared/ms/stackstring.cpp"
	"../../thirdparty/base64/base64.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
	${Source_Files}
)

target_sources(charimport PRIVATE ${ALL_FILES})