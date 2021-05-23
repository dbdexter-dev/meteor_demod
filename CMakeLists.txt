cmake_minimum_required(VERSION 3.10)

option(ENABLE_TUI "Enable Ncurses TUI" ON)

project(meteor_demod
	VERSION 1.0
	DESCRIPTION "QPSK demodulator"
	LANGUAGES C)

add_definitions(-DVERSION="${CMAKE_PROJECT_VERSION}")

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -Wextra -Wimplicit-fallthrough")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -march=native -ftree-vectorize")
# ARM architectures need -mfpu=auto in order to enable NEON when available,
# but that option is unrecognized by x86 gcc (and possibly others): only
# add it to the release flags when the compiler's target is arm
# This is not a problem for arm64, as NEON support is mandatory for that arch
execute_process(COMMAND "${CMAKE_C_COMPILER}" "-dumpmachine" COMMAND "grep" "arm" OUTPUT_QUIET RESULT_VARIABLE is_arm)
if (is_arm EQUAL "0")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -mfpu=auto")
endif()


set(COMMON_SOURCES
	dsp/agc.c dsp/agc.h
	dsp/filter.c dsp/filter.h
	dsp/pll.c dsp/pll.h
	dsp/timing.c dsp/timing.h
	dsp/sincos.c dsp/sincos.h

	demod.c demod.h
	utils.c utils.h
)

if (ENABLE_TUI)
	find_library(NCURSES_LIBRARY NAMES ncurses ncursesw)
	if (NCURSES_LIBRARY)
		add_definitions(-DENABLE_TUI)
		set(COMMON_SOURCES ${COMMON_SOURCES} tui.c tui.h)
	else()
		message(WARNING "ncurses not found, fancy TUI will not be available")
	endif()
endif()

set(COMMON_INC_DIRS
	${PROJECT_SOURCE_DIR}
)

# Main executable target
add_executable(meteor_demod main.c wavfile.c ${COMMON_SOURCES})
target_include_directories(meteor_demod PUBLIC ${COMMON_INC_DIRS})
target_link_libraries(meteor_demod PUBLIC m pthread)

# Add links to ncurses if enabled
if (ENABLE_TUI AND NCURSES_LIBRARY)
	target_link_libraries(meteor_demod PUBLIC ${NCURSES_LIBRARY})
endif()

install(TARGETS meteor_demod DESTINATION bin)

# uninstall target
if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()

