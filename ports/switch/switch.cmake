# Name:        switch.cmake
# Description: the Nintendo Switch build (devkitPro's toolchain): The Simpsons and The Simpsons Arcade Game, each as the
#              pieces of its .nsp (the NSO, main.npdm, the NACP and the icon) and as an .nro (with the assets pack
#              where this build has it)
# Version:     0.1.0
# Created:     13-11-2026
# Modified:    13-11-2026
# Author:      Sebastian Toyos
#
# Included by CMakeLists.txt when the toolchain is the Switch's (aarch64-none-elf-cmake), as tmnt-port's. Both games
# are the computers' builds with SDL2 for the Switch (switch-sdl2, devkitPro's portlibs): the picture fixed
# (SIMPSONS_HANDHELD: the Switch's 1280 x 720, whole; SIMPSONS_SCREEN_WIDE: The Simpsons' WIDE 16:9), each Joy-Con
# or controller a player, up to four, the cabinet's four slots, its buttons as The Simpsons' OPTIONS > CONTROLS binds
# them (SIMPSONS_DEVICE_CONTROLS: the page with no keys, the Switch's buttons' pictures built in), and no EXIT in The
# Simpsons' menu (SIMPSONS_SYSTEM_EXIT: HOME leaves it). A game reads the assets pack (simpsons.assets) from its own romfs,
# which a computer fills with the player's (the .nsp, made there with the player's keys by Simpsons NSP; or, where
# this build has the pack beside the executables, SIMPSONS_BIN_DIR/simpsons.assets, the .nro: the player's own,
# never shared); with none there, from sdmc:/switch/simpsons-arcade/, where it takes the assets out of the player's
# simpsons.zip at its first start (main.c's device_assets: the release's .nro carries none). Each game its own
# title, in homebrew's range (0x05...), apart from the other ports' (tmnt-port's 0500544d4e540000 and
# 0500544d4e541000, outrunners-port's 05005e6a4f520000, segasonic-port's 05005e6a50510000): "SIMP" in ASCII, then
# the game.
#
# Usage (Windows, devkitPro's MSYS2 shell; its make cannot read the dependency files its gcc writes with D:/ paths):
#   /opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-cmake -S . -B build/switch -DSIMPSONS_REFERENCE_TOOLS=OFF
#       -DCMAKE_DEPENDS_USE_COMPILER=FALSE [-DSIMPSONS_BIN_DIR=...]
#   cmake --build build/switch --target simpsons_switch_games --parallel 16

set(SIMPSONS_SWITCH_TITLE_ID 050053494d500000)           # The Simpsons Mr. Plow Edition
set(SIMPSONS_ARCADE_SWITCH_TITLE_ID 050053494d510000)    # The Simpsons Arcade Game

# (SIMPSONS_DEVICE_CONTROLS: OPTIONS > CONTROLS, each player's own controller's buttons, the players' issue 21)
target_compile_definitions(simpsons_console_program PUBLIC SIMPSONS_HANDHELD SIMPSONS_SCREEN_WIDE
	SIMPSONS_SYSTEM_EXIT SIMPSONS_DEVICE_CONTROLS)

find_package(SDL2 CONFIG QUIET)
if(TARGET SDL2::SDL2)
	set(SIMPSONS_SWITCH_SDL2 SDL2::SDL2)
else()
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(SIMPSONS_SWITCH_SDL2_PKG REQUIRED IMPORTED_TARGET sdl2)
	set(SIMPSONS_SWITCH_SDL2 PkgConfig::SIMPSONS_SWITCH_SDL2_PKG)
endif()

# devkitPro's tools: the NACP (the title's name, author and version), the NSO, main.npdm and the .nro
set(SIMPSONS_SWITCH_TOOLS $ENV{DEVKITPRO}/tools/bin)
find_program(SIMPSONS_NACPTOOL nacptool HINTS ${SIMPSONS_SWITCH_TOOLS} REQUIRED)
find_program(SIMPSONS_ELF2NSO elf2nso HINTS ${SIMPSONS_SWITCH_TOOLS} REQUIRED)
find_program(SIMPSONS_NPDMTOOL npdmtool HINTS ${SIMPSONS_SWITCH_TOOLS} REQUIRED)
find_program(SIMPSONS_ELF2NRO elf2nro HINTS ${SIMPSONS_SWITCH_TOOLS} REQUIRED)

# the HOME menu's icon, 256 x 256: the project's (ports/switch/icon.jpg, the arcade game's cover on its green, the
# picture Sebas chose for the Switch), or another of the player's where this computer has one (local/icon.jpg);
# Simpsons NSP takes a player's own instead
if(EXISTS ${CMAKE_SOURCE_DIR}/local/icon.jpg)
	set(SIMPSONS_SWITCH_ICON ${CMAKE_SOURCE_DIR}/local/icon.jpg)
else()
	set(SIMPSONS_SWITCH_ICON ${CMAKE_SOURCE_DIR}/ports/switch/icon.jpg)
endif()
message(STATUS "The Switch's icon: ${SIMPSONS_SWITCH_ICON}")
set(SIMPSONS_SWITCH_PACK ${SIMPSONS_BIN_DIR}/simpsons.assets)
set(SIMPSONS_SWITCH_OUTPUTS "")

# A game of the Switch: its executable (the window port's main.c and controls.c), and in SIMPSONS_BIN_DIR/switch/
# 'folder' its .nsp's pieces, which the computer's Simpsons NSP puts together with the player's assets and keys; its
# .nro, with the pack where this build has it. main.npdm's name is 'folder' (sixteen bytes at most there), the
# HOME menu's 'name' (control.nacp's)
function(simpsons_switch_game target name folder title_id program)
	add_executable(${target} ports/sdl2/main.c ports/sdl2/controls.c)
	target_include_directories(${target} PRIVATE ports/sdl2)
	target_link_libraries(${target} PRIVATE ${program} ${SIMPSONS_SWITCH_SDL2})
	target_compile_definitions(${target} PRIVATE SIMPSONS_HANDHELD SIMPSONS_SCREEN_WIDE SIMPSONS_SYSTEM_EXIT)
	set_target_properties(${target} PROPERTIES SUFFIX ".elf")

	set(out ${SIMPSONS_BIN_DIR}/switch/${folder})
	set(SIMPSONS_SWITCH_NAME ${folder})
	set(SIMPSONS_SWITCH_TITLE_ID ${title_id})
	configure_file(${CMAKE_SOURCE_DIR}/ports/switch/npdm.json.in ${CMAKE_BINARY_DIR}/switch/${folder}.json @ONLY)
	add_custom_command(OUTPUT ${out}/main ${out}/main.npdm ${out}/control.nacp ${out}/icon.jpg
		COMMAND ${CMAKE_COMMAND} -E make_directory ${out}
		COMMAND ${SIMPSONS_NACPTOOL} --create ${name} "Sebastian Toyos" ${PROJECT_VERSION} ${out}/control.nacp
			--titleid=${title_id}
		COMMAND ${SIMPSONS_ELF2NSO} $<TARGET_FILE:${target}> ${out}/main
		COMMAND ${SIMPSONS_NPDMTOOL} ${CMAKE_BINARY_DIR}/switch/${folder}.json ${out}/main.npdm
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SIMPSONS_SWITCH_ICON} ${out}/icon.jpg
		DEPENDS ${target} ${SIMPSONS_SWITCH_ICON} ${CMAKE_SOURCE_DIR}/ports/switch/npdm.json.in
		COMMENT "${name}: the .nsp's pieces (main, main.npdm, control.nacp, icon.jpg)"
		VERBATIM)
	set(outputs ${out}/main ${out}/main.npdm)
	if(NOT EXISTS ${SIMPSONS_SWITCH_PACK})
		# the .nro without a pack (the release's: the game takes the assets out of simpsons.zip on the SD card), in
		# SIMPSONS_BIN_DIR/switch-nro, apart from the .nsp's pieces the computers' packages carry
		set(nro ${SIMPSONS_BIN_DIR}/switch-nro/${folder}.nro)
		add_custom_command(OUTPUT ${nro}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${SIMPSONS_BIN_DIR}/switch-nro
			COMMAND ${SIMPSONS_ELF2NRO} $<TARGET_FILE:${target}> ${nro} --icon=${SIMPSONS_SWITCH_ICON}
				--nacp=${out}/control.nacp
			DEPENDS ${target} ${out}/control.nacp ${SIMPSONS_SWITCH_ICON}
			COMMENT "${folder}.nro (${name}) without an assets pack: no space in its name, which a forwarder passes in argv"
			VERBATIM)
		list(APPEND outputs ${nro})
	else()
		set(romfs ${CMAKE_BINARY_DIR}/switch/${folder}_romfs)
		add_custom_command(OUTPUT ${out}/${folder}.nro
			COMMAND ${CMAKE_COMMAND} -E make_directory ${romfs}
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SIMPSONS_SWITCH_PACK} ${romfs}
			COMMAND ${SIMPSONS_ELF2NRO} $<TARGET_FILE:${target}> ${out}/${folder}.nro --icon=${SIMPSONS_SWITCH_ICON}
				--nacp=${out}/control.nacp --romfsdir=${romfs}
			DEPENDS ${target} ${SIMPSONS_SWITCH_PACK} ${out}/control.nacp ${SIMPSONS_SWITCH_ICON}
			COMMENT "${name}.nro with the assets pack in its romfs (the player's own: never shared)"
			VERBATIM)
		list(APPEND outputs ${out}/${folder}.nro)
	endif()
	set(SIMPSONS_SWITCH_OUTPUTS ${SIMPSONS_SWITCH_OUTPUTS} ${outputs} PARENT_SCOPE)
endfunction()

simpsons_switch_game(simpsons_switch "The Simpsons Mr. Plow Edition" simpsons ${SIMPSONS_SWITCH_TITLE_ID} simpsons_console_game)
# (CONTROLS' pictures in the executable: neither the .nro nor the .nsp has a file beside it)
target_link_libraries(simpsons_switch PRIVATE simpsons_prompts)
simpsons_switch_game(simpsons_arcade_switch "The Simpsons Arcade Game" simpsons-arcade ${SIMPSONS_ARCADE_SWITCH_TITLE_ID}
	${SIMPSONS_WINDOW_PROGRAM})
if(NOT EXISTS ${SIMPSONS_SWITCH_PACK})
	message(STATUS "No assets pack at ${SIMPSONS_SWITCH_PACK}: the .nro without one (SIMPSONS_BIN_DIR/switch-nro)")
endif()
add_custom_target(simpsons_switch_games ALL DEPENDS ${SIMPSONS_SWITCH_OUTPUTS})
