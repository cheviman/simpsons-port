# Name:        android.cmake
# Description: the Android build (the NDK's toolchain): The Simpsons and The Simpsons Arcade Game, each its libmain.so beside
#              SDL2's, both from their source, and the APKs around them: each game's for Simpsons APK, and this
#              computer's own
# Version:     0.1.0
# Created:     13-11-2026
# Modified:    13-11-2026
# Author:      Sebastian Toyos
#
# Included by CMakeLists.txt when the toolchain is the NDK's (build/cmake/android.toolchain.cmake, with ANDROID_ABI and
# ANDROID_PLATFORM), as tmnt-port's. Both games are the computers' builds on the phone's whole screen
# (SIMPSONS_HANDHELD), The Simpsons' WIDE chosen by the screen's shape (main.c's follow_screen, again as it changes: a
# Fold opened), no EXIT in its menu (SIMPSONS_SYSTEM_EXIT: Android's home leaves it) and the touch screen's controls
# (SIMPSONS_TOUCH, ports/sdl2/touch.c), each in a libmain.so, which SDL's activity starts (its Java, from SDL's
# source). The APKs are put together without Gradle, by the SDK's own tools: javac and d8 (the Java), aapt2 (the
# manifest, the icon, the assets), aapt (the libraries and the Java in), zipalign and apksigner. An APK carries the
# assets pack made of the player's ROM set on a computer (Simpsons APK), so it is its player's alone and never shared;
# one without a pack makes it on the device, of the simpsons.zip the player puts in the app's folder (main.c's
# device_assets). Of each game, two:
#   SIMPSONS_BIN_DIR/android/<game>-base.apk    the game without a pack or a signature: Simpsons APK
#                                               (ports/android/apk) makes the player's APK of it, on their computer
#   SIMPSONS_BIN_DIR/android/<Game>-debug.apk   this computer's own, with its pack (SIMPSONS_BIN_DIR/simpsons.assets,
#                                               where there is one), signed with a debug key of this computer's
#                                               (SIMPSONS_ANDROID_KEYSTORE, by default local/android-debug.keystore,
#                                               which keytool makes when there is none: local/ is never in the
#                                               repository; the same key each time, so a new APK installs over the last
#                                               one and keeps what the game wrote)
# Each APK has the libraries of two ABIs, and Android installs the one its device runs: this build's (ANDROID_ABI,
# arm64-v8a: the phones, the tablets, the Shield) and 32-bit ARM's (armeabi-v7a: the Chromecast with Google TV, whose
# Android is 32-bit on a 64-bit chip, and older phones), built by a build of its own of the same source (the NDK builds
# one ABI at a time) in <this build>/abi/armeabi-v7a, with SIMPSONS_ANDROID_LIBRARIES_ONLY: its libraries alone.
#
# Usage (Windows; the NDK, the SDK and Java 17 of build/downloads/android, Ninja from Visual Studio):
#   cmake -S . -B build/android -G Ninja -DCMAKE_BUILD_TYPE=Release -DSIMPSONS_REFERENCE_TOOLS=OFF
#       -DCMAKE_TOOLCHAIN_FILE=build/downloads/android/sdk/ndk/<version>/build/cmake/android.toolchain.cmake
#       -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 -DSIMPSONS_BIN_DIR=build/bin-android
#       -DJava_JAVAC_EXECUTABLE=build/downloads/android/jdk-<version>/bin/javac.exe
#   cmake --build build/android --target simpsons_apks --parallel 16

set(SIMPSONS_ANDROID_DEFINITIONS SIMPSONS_HANDHELD SIMPSONS_SYSTEM_EXIT SIMPSONS_TOUCH)
# (SIMPSONS_DEVICE_CONTROLS: OPTIONS > CONTROLS, each player's own game controller's buttons, as the Switch's: the
#  players' issue 41)
target_compile_definitions(simpsons_console_program PUBLIC ${SIMPSONS_ANDROID_DEFINITIONS} SIMPSONS_DEVICE_CONTROLS)

set(SIMPSONS_ANDROID_VERSION ${PROJECT_VERSION})
math(EXPR SIMPSONS_ANDROID_VERSION_CODE    # (major * 10000 + minor * 100 + patch)
	"${PROJECT_VERSION_MAJOR} * 10000 + ${PROJECT_VERSION_MINOR} * 100 + ${PROJECT_VERSION_PATCH}")
set(SIMPSONS_ANDROID_TARGET_SDK 35)
set(SIMPSONS_ANDROID_MIN_SDK 24)    # (Android 7: the APK's signature is v2's alone, as Simpsons APK signs it)
string(REGEX MATCH "[0-9]+" SIMPSONS_ANDROID_PLATFORM_LEVEL "${ANDROID_PLATFORM}")
if(SIMPSONS_ANDROID_PLATFORM_LEVEL GREATER SIMPSONS_ANDROID_MIN_SDK)
	set(SIMPSONS_ANDROID_MIN_SDK ${SIMPSONS_ANDROID_PLATFORM_LEVEL})
endif()
set(SIMPSONS_ANDROID_ICON "" CACHE FILEPATH
	"The launcher's icon: a square picture of the player's own (JPEG or PNG), instead of the project's")
set(SIMPSONS_ANDROID_KEYSTORE "" CACHE FILEPATH
	"The key this computer's APKs are signed with (by default local/android-debug.keystore, made once)")
if(NOT SIMPSONS_ANDROID_ICON AND EXISTS ${CMAKE_SOURCE_DIR}/local/icon_android.png)
	set(SIMPSONS_ANDROID_ICON ${CMAKE_SOURCE_DIR}/local/icon_android.png)    # (local/: the player's own pictures, the
endif()                                                                      #  show's art, never in the repository)

# SDL2 from its source, as the releases': this computer's copy of release-2.32.10 (SIMPSONS_SDL2_SOURCE,
# build/downloads/SDL2-src), else SDL's repository at that tag; its library, libSDL2.so, and its Java
include(FetchContent)
if(EXISTS ${SIMPSONS_SDL2_SOURCE}/CMakeLists.txt AND NOT FETCHCONTENT_SOURCE_DIR_SDL2)
	set(FETCHCONTENT_SOURCE_DIR_SDL2 ${SIMPSONS_SDL2_SOURCE})
endif()
FetchContent_Declare(SDL2 GIT_REPOSITORY https://github.com/libsdl-org/SDL.git GIT_TAG release-2.32.10 GIT_SHALLOW TRUE)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL2)

# A game's libmain.so: 'target', the window port's main.c, controls.c, letters.c and touch.c, on 'program' (whose
# library reads the assets pack, and the player's ROM set on the device: main.c's device_assets)
function(simpsons_android_library target program)
	add_library(${target} SHARED ports/sdl2/main.c ports/sdl2/controls.c ports/sdl2/letters.c ports/sdl2/touch.c)
	target_link_libraries(${target} PRIVATE ${program} simpsons_controller_db SDL2::SDL2)
	target_compile_definitions(${target} PRIVATE ${SIMPSONS_ANDROID_DEFINITIONS})
	target_include_directories(${target} PRIVATE ports/sdl2)
	set_target_properties(${target} PROPERTIES OUTPUT_NAME main LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${target})
endfunction()

# The build of one more ABI's libraries, for the APKs of the build that starts it (below): the games' libraries alone,
# no SDK, no Java, no APK
option(SIMPSONS_ANDROID_LIBRARIES_ONLY "Build the games' libraries alone: one more ABI for another build's APKs" OFF)
if(SIMPSONS_ANDROID_LIBRARIES_ONLY)
	simpsons_android_library(simpsons_android simpsons_console_game)
	simpsons_android_library(simpsons_arcade_android ${SIMPSONS_WINDOW_PROGRAM})
	return()
endif()

# The SDK around the NDK (sdk/ndk/<version>), else ANDROID_HOME: the platform's android.jar, the newest build-tools
if(DEFINED ENV{ANDROID_HOME})
	set(SIMPSONS_ANDROID_SDK $ENV{ANDROID_HOME})
else()
	get_filename_component(SIMPSONS_ANDROID_SDK ${ANDROID_NDK}/../.. ABSOLUTE)
endif()
set(SIMPSONS_ANDROID_JAR ${SIMPSONS_ANDROID_SDK}/platforms/android-${SIMPSONS_ANDROID_TARGET_SDK}/android.jar)
if(NOT EXISTS ${SIMPSONS_ANDROID_JAR})
	message(FATAL_ERROR "No android.jar of Android ${SIMPSONS_ANDROID_TARGET_SDK} in the SDK (${SIMPSONS_ANDROID_SDK})")
endif()
file(GLOB SIMPSONS_BUILD_TOOLS LIST_DIRECTORIES true ${SIMPSONS_ANDROID_SDK}/build-tools/*)
list(SORT SIMPSONS_BUILD_TOOLS COMPARE NATURAL)
list(POP_BACK SIMPSONS_BUILD_TOOLS SIMPSONS_BUILD_TOOLS_DIR)
foreach(tool IN ITEMS aapt2 aapt zipalign d8 apksigner)
	string(TOUPPER ${tool} name)
	find_program(SIMPSONS_${name} NAMES ${tool} ${tool}.bat HINTS ${SIMPSONS_BUILD_TOOLS_DIR} NO_DEFAULT_PATH REQUIRED)
endforeach()

# Java 17 (JAVA_HOME, or Java_JAVAC_EXECUTABLE): javac and jar for SDL's Java, keytool for the debug key; d8 and
# apksigner run with it
find_package(Java 17 COMPONENTS Development REQUIRED)
get_filename_component(SIMPSONS_JAVA_BIN ${Java_JAVAC_EXECUTABLE} REALPATH)
get_filename_component(SIMPSONS_JAVA_BIN ${SIMPSONS_JAVA_BIN} DIRECTORY)
get_filename_component(SIMPSONS_JAVA_HOME ${SIMPSONS_JAVA_BIN} DIRECTORY)
find_program(SIMPSONS_KEYTOOL keytool HINTS ${SIMPSONS_JAVA_BIN} REQUIRED)
set(SIMPSONS_WITH_JAVA ${CMAKE_COMMAND} -E env JAVA_HOME=${SIMPSONS_JAVA_HOME})

set(SIMPSONS_APK_WORK ${CMAKE_BINARY_DIR}/apk)

# SDL's Java, the activity that starts libmain.so: classes.dex, the same in both games
file(GLOB SIMPSONS_SDL_JAVA ${sdl2_SOURCE_DIR}/android-project/app/src/main/java/org/libsdl/app/*.java)
add_custom_command(OUTPUT ${SIMPSONS_APK_WORK}/dex/classes.dex
	COMMAND ${CMAKE_COMMAND} -E rm -rf ${SIMPSONS_APK_WORK}/classes ${SIMPSONS_APK_WORK}/dex
	COMMAND ${CMAKE_COMMAND} -E make_directory ${SIMPSONS_APK_WORK}/classes ${SIMPSONS_APK_WORK}/dex
	COMMAND ${Java_JAVAC_EXECUTABLE} -source 8 -target 8 -Xlint:-options -nowarn -encoding UTF-8
		-bootclasspath ${SIMPSONS_ANDROID_JAR} -classpath ${SIMPSONS_BUILD_TOOLS_DIR}/core-lambda-stubs.jar
		-d ${SIMPSONS_APK_WORK}/classes ${SIMPSONS_SDL_JAVA}    # (the stubs: the lambdas' LambdaMetafactory)
	COMMAND ${Java_JAR_EXECUTABLE} cf ${SIMPSONS_APK_WORK}/classes.jar -C ${SIMPSONS_APK_WORK}/classes .
	COMMAND ${SIMPSONS_WITH_JAVA} ${SIMPSONS_D8} --release --min-api ${SIMPSONS_ANDROID_MIN_SDK}
		--lib ${SIMPSONS_ANDROID_JAR} --output ${SIMPSONS_APK_WORK}/dex ${SIMPSONS_APK_WORK}/classes.jar
	DEPENDS ${SIMPSONS_SDL_JAVA}
	COMMENT "SDL's Java: classes.dex"
	VERBATIM)

file(GLOB_RECURSE SIMPSONS_ANDROID_RES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/ports/android/res/*)

# The ABIs in the APKs: this build's and SIMPSONS_ANDROID_MORE_ABIS', each of those a build of its own
# (ExternalProject, its libraries alone), always run: its own build tells what changed
set(SIMPSONS_ANDROID_MORE_ABIS armeabi-v7a CACHE STRING "The ABIs whose libraries go into the APKs beside this build's")
set(SIMPSONS_MORE_ABIS ${SIMPSONS_ANDROID_MORE_ABIS})
list(REMOVE_ITEM SIMPSONS_MORE_ABIS ${ANDROID_ABI})
set(SIMPSONS_APK_LIBRARIES "")    # (as the APK has them: lib/<ABI>/<library>)
foreach(abi IN LISTS ANDROID_ABI SIMPSONS_MORE_ABIS)
	list(APPEND SIMPSONS_APK_LIBRARIES lib/${abi}/libmain.so lib/${abi}/libSDL2.so)
endforeach()
include(ExternalProject)
foreach(abi IN LISTS SIMPSONS_MORE_ABIS)
	set(dir ${CMAKE_BINARY_DIR}/abi/${abi})
	ExternalProject_Add(simpsons_android_${abi}
		SOURCE_DIR ${CMAKE_SOURCE_DIR}
		BINARY_DIR ${dir}
		CMAKE_GENERATOR ${CMAKE_GENERATOR}
		CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DANDROID_ABI=${abi}
			-DANDROID_PLATFORM=${ANDROID_PLATFORM} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
			-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM} -DSIMPSONS_REFERENCE_TOOLS=OFF
			-DSIMPSONS_ANDROID_LIBRARIES_ONLY=ON -DSIMPSONS_BIN_DIR=${dir}/bin
			-DSIMPSONS_DECOMPILED_DIR=${SIMPSONS_DECOMPILED_DIR} -DFETCHCONTENT_SOURCE_DIR_SDL2=${sdl2_SOURCE_DIR}
		BUILD_COMMAND ${CMAKE_COMMAND} --build ${dir} --target simpsons_android simpsons_arcade_android
		INSTALL_COMMAND ""
		BUILD_ALWAYS TRUE
		BUILD_BYPRODUCTS ${dir}/simpsons_android/libmain.so ${dir}/simpsons_arcade_android/libmain.so
			${dir}/_deps/sdl2-build/libSDL2.so)
endforeach()

# The key this computer's APKs are signed with: local/android-debug.keystore unless one is given, made once with
# keytool (Android's debug key's form: alias androiddebugkey, password android). local/ is the player's own and never
# in the repository; a release's base APKs are signed by the CI with the project's key, the player's by Simpsons APK
if(SIMPSONS_ANDROID_KEYSTORE)
	set(SIMPSONS_KEYSTORE ${SIMPSONS_ANDROID_KEYSTORE})
else()
	set(SIMPSONS_KEYSTORE ${CMAKE_SOURCE_DIR}/local/android-debug.keystore)
endif()
get_filename_component(SIMPSONS_KEYSTORE_DIR ${SIMPSONS_KEYSTORE} DIRECTORY)
set(SIMPSONS_KEYSTORE_MADE "")
if(NOT EXISTS ${SIMPSONS_KEYSTORE})    # (one made before is kept as it is)
	add_custom_command(OUTPUT ${SIMPSONS_KEYSTORE}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${SIMPSONS_KEYSTORE_DIR}
		COMMAND ${SIMPSONS_KEYTOOL} -genkeypair -keystore ${SIMPSONS_KEYSTORE} -storepass android
			-alias androiddebugkey -keypass android -keyalg RSA -keysize 2048 -validity 10000
			-dname "CN=Android Debug,O=Android,C=US"
		COMMENT "The debug key: ${SIMPSONS_KEYSTORE}"
		VERBATIM)
	set(SIMPSONS_KEYSTORE_MADE ${SIMPSONS_KEYSTORE})
endif()

set(SIMPSONS_ANDROID_PACK ${SIMPSONS_BIN_DIR}/simpsons.assets)
if(NOT EXISTS ${SIMPSONS_ANDROID_PACK})
	message(STATUS "No assets pack at ${SIMPSONS_ANDROID_PACK}: this computer's APKs get the games alone (each makes "
		"its pack of the simpsons.zip in its folder)")
endif()
set(SIMPSONS_ANDROID_OUTPUTS "")

# An APK's manifest, icon and assets, as aapt2 links them from the folder 'stage' (its res/ and assets/, which
# 'commands' fill), then the game's files (in 'files') in and aligned: 'apk'. Its assets: the letters of the touch
# controls and the menus (font.bmp) and CONTROLS' pictures (input_prompts.bmp)
function(simpsons_apk apk stage manifest files commands depends)
	add_custom_command(OUTPUT ${apk}
		COMMAND ${CMAKE_COMMAND} -E rm -rf ${stage}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${stage}/assets
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/ports/android/res ${stage}/res
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/ports/sdl2/font/font.bmp
			${CMAKE_SOURCE_DIR}/ports/sdl2/prompts/input_prompts.bmp ${stage}/assets
		${commands}
		COMMAND ${SIMPSONS_AAPT2} compile --dir ${stage}/res -o ${stage}/res.zip
		COMMAND ${SIMPSONS_AAPT2} link -o ${stage}/linked.apk -I ${SIMPSONS_ANDROID_JAR} --manifest ${manifest}
			--min-sdk-version ${SIMPSONS_ANDROID_MIN_SDK} --target-sdk-version ${SIMPSONS_ANDROID_TARGET_SDK}
			--version-code ${SIMPSONS_ANDROID_VERSION_CODE} --version-name ${SIMPSONS_ANDROID_VERSION}
			-A ${stage}/assets -0 assets ${stage}/res.zip    # (the pack stored: read in place, never unpacked)
		COMMAND ${CMAKE_COMMAND} -E chdir ${files} ${SIMPSONS_AAPT} add ${stage}/linked.apk classes.dex
			${SIMPSONS_APK_LIBRARIES}
		COMMAND ${SIMPSONS_ZIPALIGN} -p -f 4 ${stage}/linked.apk ${apk}
		DEPENDS ${manifest} ${SIMPSONS_ANDROID_RES} ${CMAKE_SOURCE_DIR}/ports/sdl2/font/font.bmp
			${CMAKE_SOURCE_DIR}/ports/sdl2/prompts/input_prompts.bmp ${files}/classes.dex
			${SIMPSONS_APK_LIBRARIES_IN_FILES} ${depends}
		COMMENT "The APK: ${apk}"
		VERBATIM)
endfunction()

# A game of Android: its libmain.so ('target' on 'program'), its manifest ('package', 'label'), its base APK
# (SIMPSONS_BIN_DIR/android/'base'-base.apk) and this computer's ('debug'-debug.apk)
function(simpsons_android_game target program base debug package label)
	simpsons_android_library(${target} ${program})

	set(SIMPSONS_ANDROID_PACKAGE ${package})
	set(SIMPSONS_ANDROID_LABEL ${label})
	set(manifest ${SIMPSONS_APK_WORK}/${base}/AndroidManifest.xml)
	configure_file(${CMAKE_SOURCE_DIR}/ports/android/AndroidManifest.xml.in ${manifest} @ONLY)

	# the game's files in its APKs: the libraries of every ABI (without their symbols: llvm-strip takes any) and the
	# Java, named as the APK has them
	set(files ${SIMPSONS_APK_WORK}/${base}/files)
	set(this_abi ${files}/lib/${ANDROID_ABI})
	set(more_libraries "")
	set(more_depends "")
	foreach(abi IN LISTS SIMPSONS_MORE_ABIS)
		set(dir ${CMAKE_BINARY_DIR}/abi/${abi})
		list(APPEND more_libraries
			COMMAND ${CMAKE_COMMAND} -E make_directory ${files}/lib/${abi}
			COMMAND ${CMAKE_STRIP} --strip-unneeded -o ${files}/lib/${abi}/libmain.so ${dir}/${target}/libmain.so
			COMMAND ${CMAKE_STRIP} --strip-unneeded -o ${files}/lib/${abi}/libSDL2.so ${dir}/_deps/sdl2-build/libSDL2.so)
		list(APPEND more_depends simpsons_android_${abi} ${dir}/${target}/libmain.so ${dir}/_deps/sdl2-build/libSDL2.so)
	endforeach()
	list(TRANSFORM SIMPSONS_APK_LIBRARIES PREPEND ${files}/ OUTPUT_VARIABLE SIMPSONS_APK_LIBRARIES_IN_FILES)
	add_custom_command(OUTPUT ${files}/classes.dex ${SIMPSONS_APK_LIBRARIES_IN_FILES}
		COMMAND ${CMAKE_COMMAND} -E rm -rf ${files}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${this_abi}
		COMMAND ${CMAKE_STRIP} --strip-unneeded -o ${this_abi}/libmain.so $<TARGET_FILE:${target}>
		COMMAND ${CMAKE_STRIP} --strip-unneeded -o ${this_abi}/libSDL2.so $<TARGET_FILE:SDL2::SDL2>
		${more_libraries}
		COMMAND ${CMAKE_COMMAND} -E copy ${SIMPSONS_APK_WORK}/dex/classes.dex ${files}
		DEPENDS ${target} SDL2 ${SIMPSONS_APK_WORK}/dex/classes.dex ${more_depends}
		COMMENT "${label}'s libraries (${ANDROID_ABI} ${SIMPSONS_MORE_ABIS}) and Java"
		VERBATIM)

	# its base APK, for Simpsons APK (ports/android/apk), which puts the player's assets pack in it (and their icon,
	# ic_launcher_own.xml and the picture in ic_launcher_art's place, and their TV banner), and signs it: no pack, no
	# signature
	set(base_apk ${SIMPSONS_BIN_DIR}/android/${base}-base.apk)
	simpsons_apk(${base_apk} ${SIMPSONS_APK_WORK}/${base}/base ${manifest} ${files} "" "")

	# this computer's own: its assets pack, its icon (SIMPSONS_ANDROID_ICON, as Simpsons APK puts it) and the debug key
	set(stage ${SIMPSONS_APK_WORK}/${base}/player)
	set(extra "")
	set(extra_files "")
	if(EXISTS ${SIMPSONS_ANDROID_PACK})
		list(APPEND extra COMMAND ${CMAKE_COMMAND} -E copy ${SIMPSONS_ANDROID_PACK} ${stage}/assets)
		list(APPEND extra_files ${SIMPSONS_ANDROID_PACK})
	endif()
	if(SIMPSONS_ANDROID_ICON)
		get_filename_component(type ${SIMPSONS_ANDROID_ICON} LAST_EXT)
		string(TOLOWER ${type} type)
		set(mipmap ${stage}/res/mipmap-xxxhdpi)
		list(APPEND extra
			COMMAND ${CMAKE_COMMAND} -E rm -f ${mipmap}/ic_launcher.png ${mipmap}/ic_launcher_art.png
			COMMAND ${CMAKE_COMMAND} -E copy ${SIMPSONS_ANDROID_ICON} ${mipmap}/ic_launcher${type}
			COMMAND ${CMAKE_COMMAND} -E copy ${SIMPSONS_ANDROID_ICON} ${mipmap}/ic_launcher_art${type}
			COMMAND ${CMAKE_COMMAND} -E copy ${stage}/res/mipmap-anydpi-v26/ic_launcher_own.xml
				${stage}/res/mipmap-anydpi-v26/ic_launcher.xml)
		list(APPEND extra_files ${SIMPSONS_ANDROID_ICON})
	endif()
	simpsons_apk(${SIMPSONS_APK_WORK}/${base}/player.apk ${stage} ${manifest} ${files} "${extra}" "${extra_files}")
	set(debug_apk ${SIMPSONS_BIN_DIR}/android/${debug}-debug.apk)    # (not Simpsons APK's own)
	add_custom_command(OUTPUT ${debug_apk}
		COMMAND ${SIMPSONS_WITH_JAVA} ${SIMPSONS_APKSIGNER} sign --ks ${SIMPSONS_KEYSTORE} --ks-pass pass:android
			--ks-key-alias androiddebugkey --key-pass pass:android --out ${debug_apk}
			${SIMPSONS_APK_WORK}/${base}/player.apk
		DEPENDS ${SIMPSONS_APK_WORK}/${base}/player.apk ${SIMPSONS_KEYSTORE_MADE}
		COMMENT "This computer's ${label}, signed: ${debug_apk}"
		VERBATIM)
	set(SIMPSONS_ANDROID_OUTPUTS ${SIMPSONS_ANDROID_OUTPUTS} ${base_apk} ${debug_apk} PARENT_SCOPE)
endfunction()

simpsons_android_game(simpsons_android simpsons_console_game simpsons The-Simpsons io.github.cheviman.simpsons
	"The Simpsons Mr. Plow Edition")
simpsons_android_game(simpsons_arcade_android ${SIMPSONS_WINDOW_PROGRAM} simpsons-arcade The-Simpsons-Arcade
	io.github.cheviman.simpsons.arcade "The Simpsons Arcade Game")
add_custom_target(simpsons_apks ALL DEPENDS ${SIMPSONS_ANDROID_OUTPUTS})
