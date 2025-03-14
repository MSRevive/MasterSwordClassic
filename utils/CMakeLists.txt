cmake_minimum_required( VERSION 3.23 )

if (MSVC)
	set(CMAKE_SYSTEM_VERSION 10.0)
endif()

project(msr LANGUAGES C CXX)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

# Disable the use of absolute paths in library paths even in development builds.
set(CMAKE_SKIP_BUILD_RPATH ON)

# Link statically with the runtime
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Because we have 2 libraries that use the same symbols we have to default visibility to hidden so there are no collisions,
# and so the symbols don't merge and cause problems like server code calling the client version of a function.
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# For some reason, checking if CMAKE_BUILD_TYPE is defined is unreliable
# So simply check if it's empty instead
if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Modern VS versions default to C++14 anyway, so make it consistent
# But in the future we may want so support C++20
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(BASEDIR "${CMAKE_CURRENT_SOURCE_DIR}")

function(set_common_properties TARGET_NAME)
	# Compile Flags and such.
	set_target_properties(${TARGET_NAME} PROPERTIES PREFIX "")

	target_compile_definitions(${TARGET_NAME} PRIVATE
			_CRT_SECURE_NO_WARNINGS
			_MSR_UTILS
			$<$<CONFIG:DEBUG>:_DEBUG>
			$<$<PLATFORM_ID:Linux>:POSIX _POSIX LINUX _LINUX GNUC>)

	target_compile_options(${TARGET_NAME} PRIVATE
			# force 387 for FP math so the precision between win32 and linux and osx match
			# Note: the pentium-m arch setting is not used for AMD systems in the original makefile
			# Since the arch settings used are i686 this means including the setting ensures original settings are used,
			# but it could cause problems for AMD targets
			$<$<CXX_COMPILER_ID:GNU>:-fpermissive -fno-strict-aliasing -Wno-invalid-offsetof -march=pentium-m -mfpmath=387>
			
			# flifetime-dse=1 is needed to disable a compiler optimization that optimizes out std::memset calls in CBaseEntity::operator new
			# See https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html#index-flifetime-dse for more information about this flag
			# fno-gnu-unique is needed to disable a compiler optimization that prevents dlclose from unloading mod dlls,
			# causing them to retain state and crash when the engine tries to access memory in an invalid way
			# See https://gcc.gnu.org/onlinedocs/gcc/Code-Gen-Options.html#index-fno-gnu-unique for more information about this flag
			$<$<CXX_COMPILER_ID:GNU>:-flifetime-dse=1 -fno-gnu-unique>
			
			# Can't set warning level to W4 until the compiler can deal with std::visit containing static_assert, which currently causes false positive "Unreachable code" warnings
			$<$<CXX_COMPILER_ID:MSVC>:/W3 /MP>)

	# Disable certain compiler warnings.
	if (MSVC)
		target_compile_options(${TARGET_NAME} PRIVATE
			# int or float down-conversion
			/wd4244
			# int or float data truncation
			/wd4305
			# unreferenced inline function removed
			/wd4514
			# unreferenced formal parameter
			/wd4100
			# Variable is uninitialized
			/wd26495
			# Arithmetic overflow
			/wd26451
			# The enum type is unscoped
			/wd26812)
	endif()

	if (UNIX)
		target_link_options(${TARGET_NAME} PRIVATE
			$<$<CXX_COMPILER_ID:GNU>:-Wl,--no-undefined>
			$<$<PLATFORM_ID:Linux>:-static-libstdc++ -Wl,-Map,${TARGET_NAME}_map.txt>)
	endif()
endfunction(set_common_properties)

add_compile_definitions($<$<CONFIG:Debug>:DEBUG> $<$<CONFIG:Debug>:_DEBUG>)
add_compile_definitions($<$<CONFIG:Release>:NDEBUG>)

# TODO: charimport is quite broken
# add_subdirectory(charimport)
add_subdirectory(maphash)
add_subdirectory(scriptpack)

# Set Visual Studio starting project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT maphash)