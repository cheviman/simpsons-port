# The Simpsons on every platform: what it takes, and in what order

This repository is the port of the decompilation (github.com/cheviman/simpsons-decomp): a clone of it with its
history, as tmnt-port is of tmntarcade-decomp. What changes the decompilation (src/, symbols.txt, tools/, runtime/
and the core) is done there and merged here ("Merge simpsons-decomp: ..."); this repository adds console/, the
ports, their documents and the CI.

It builds two games from the same C:

- **The Simpsons Arcade**: the decompiled game as it is, the same frame by frame as the reference (the coins, the
  test mode, the four players, the EEPROM), on every platform.
- **The Simpsons**: the console version, the game made for playing at home (console/PLAN.md).

tmnt-port is the model for nearly everything: the same board family (the K052109, the YM2151, four players on one
board), the same scheme (one ports/sdl2/main.c with #ifdefs and feature macros, a .cmake for each device, the makers
the player runs on a computer, the console in console/), and its players' reports already answered. Its folders are
read, never run or changed from here.

## What the decompilation gives today (0f03164, 11-11-2026)

- `simpsons` (ports/sdl2/main.c): the game in a window on Windows, SDL2 2.32.10 linked in, the keyboard and four game
  controllers, `--script`, `--hashes`; the factory EEPROM at every start (nothing is kept).
- `simpsons_game_headless` (the decompiled program), `simpsons_headless` (the translated scaffold), `kref` (the
  reference on MAME's CPU and chips).
- The assets come from `tools/extract_assets.py` (Python) and the game loads the folder (runtime/src/assets.c); no
  executable holds a byte of the ROM set (`tools/rom_in_executables.py`).
- The 25 scenarios identical in state, picture and sound.

Where it differs from TMNT, which decides what is copied as it is:

- **No tasks.** The 053248 program runs as one chain of calls and calls the port back at each frame (`frame_wait` ->
  `program_frame`, runtime/src/game_program.c). tmnt-port's runtime/src/tasks.c and its aarch64 and 32-bit ARM paths
  have nothing to do here: the port's frame is the hook, as in the window today, and the console's menus and pause
  run inside it too.
- **The stack.** The game's thread has 64 MB on Windows (CMakeLists.txt, `simpsons_game_stack`) and has only been run
  with that; on Linux and macOS it takes the system's (8 MB by default), on the devices whatever the thread SDL starts
  has. How much it really needs is not measured yet.
- **The EEPROM instead of DIP switches.** The settings (the coinage, the lives, the difficulty, the attract's sound,
  the flipped screen) are in the ER5911 (68-7F), written by the test mode and copied to 59F1 at the boot
  (`apply_settings`, 078A1A). The high score table is in the work RAM alone (the EEPROM's 00-67 are its test pattern):
  lost at power-off, as on the cabinet.
- **The sound.** The board wires only the YM2151's left output (MAME's simpsons.cpp; the mix puts it on both sides).
- **The console's replacements.** The core's generic decompile.py writes the functions of console/replaced.txt as
  `CONSOLE_ORIGINAL(name)`, but konami_cpu/decompile.py reads runtime/replaced.txt alone (its `replaced_functions`
  call): the decompilation needs that before the console build can replace anything.

## The order (proposed, 13-11-2026: Sebas decides)

As TMNT's went (Sebas, 04-11-2026: "primero una version completa y dps port"):

- **A.** The base on Windows: the clone checked, the port's shell from tmnt-port, the first run in C, The Simpsons
  Arcade's package.
- **B.** The console version on Windows, complete (console/PLAN.md).
- **C.** The computers: Linux (and with it the Steam Deck) and macOS.
- **D.** The devices: Nintendo Switch, Android (phones, tablets, TVs), iOS and iPadOS, Anbernic.
- **E.** The release.
- **F.** Last: installing on the Steam Deck, the same installer in the four ports (Sebas, 13-11-2026).
- **G.** At the end too: the distribution maker, a project of its own for the four ports (Sebas, 13-11-2026: "al
  final"; D's makers are copied from tmnt-port meanwhile, as the other ports did).

The decompilation goes on to its 100 % alongside (as with OutRunners), merged in here as it lands.

Every item says how it is checked without the device; what only a device shows is said so in the release's notes,
never taken as tried.

## A. The base

### A0. The clone

- [x] The clone, its remotes (origin = github.com/cheviman/simpsons-port, upstream = simpsons-decomp on GitHub,
      local-decomp = D:/vs-cheviman/simpsons-decomp), the core on its `simpsons` branch, the commits' identity, the
      ROM set in rom/ and SDL2 in build/downloads. Check: `py tools/verify_rom.py` 14/14 (13-11-2026).
- [x] It builds and runs here as in the decompilation (13-11-2026). The assets extracted (27 s) and `--check` the
      same; the Z80's timing table (`decompile_sound.py`, whose C came out the same but for the dates); build/msvc in
      20 s. The 25 scenarios identical in state, picture and sound, the reference's results written here (4 min 29 s
      with `--jobs 16`), the static analysis 43056 of 43056; `rom_in_executables` 0 (simpsons, simpsons_game_headless);
      the window's attract with the three hashes the same as the reference's (7201 frames).
- [x] `.gitignore`: `local/` (the player's own pictures for the icons, the Android key) (13-11-2026). The kits when
      the CI makes them.

### A1. What the decompilation does for the port (in simpsons-decomp, merged here)

- [x] **CONSOLE_ORIGINAL in konami_cpu/decompile.py**: console/replaced.txt read beside runtime/replaced.txt, as TMNT's
      did (tmntarcade-decomp 7ad99cf). Check: a console/replaced.txt with one name writes its definition as
      `CONSOLE_ORIGINAL(name)`; with none, the C byte for byte as before; 25/25. (13-11-2026, done here in the port's
      tree, tools/core and support.h: attract, game_mode_frame and gauge_draw_picture written so; to take to
      simpsons-decomp and its core with the next commits there.)
- [x] **The stack measured** (13-11-2026): simpsons_game_headless (Release, MSVC) with its stack cut by `editbin
      /STACK` from the 64 MB the build gives it: the 25 scenarios identical with 256 KB (3 min 4 s), and stage8 (50101
      frames, the ending), test_rom, test_music and four_players identical with 128, 64 and even 32 KB. The decompiled
      program needs less than 32 KB (the 64 MB was the translation scaffold's, with its many locals): every platform's
      own thread has room (Linux and macOS 8 MB, Android's SDL thread, the Switch's and iOS's main threads); no thread
      of the game's own. A debug build (larger frames) is to be run once on Linux with the system's 8 MB. The
      CMakeLists.txt comment ("run with 64 MB only") goes in the decompilation with these numbers.
- [x] **The first run in C, as the pack** (13-11-2026; "primero que ande": the pack straight from the ROM set, not
      the editable folder, which comes later from C if at all — the Python extractor writes it): runtime/src/
      assets_layout.c (generated and kept in the repository: places and lengths only, 10377 program pieces of 421301
      bytes, 1243 sound pieces of 112680, the four regions whole), written by `simpsons_assets_check --write-layout`
      from the folder as assets.c loads it; `game_assets_rom_set_load_into` reads the player's set through it.
      Check: `simpsons_assets_check`: the folder's pack, that pack loaded and written again, and the ROM set's pack,
      7180740 bytes, the same (run by the agent and again by me); a changed EEPROM or data byte is caught at its place;
      attract, stage1 and test_rom from the folder, the folder's pack, the ROM's pack and `--rom`: 27 comparisons of
      the three hashes, identical; the 25 scenarios identical; `rom_in_executables` 0; `extract_assets.py --check` the
      same.

### A2. The port's shell, from tmnt-port (`TMNT_` -> `SIMPSONS_`)

- [x] **ports/sdl2/main.c** around Simpsons' game.h and board (13-11-2026, an agent; checked by me): the folder of
      what the game keeps, `find_assets`, the log (simpsons.log, simpsons-arcade.log), the frame's pace
      (`wait_for_frame`, `frame_behind`), VSYNC on by default, Direct3D 11 on Windows, the full screen, the pixels
      (sharp, smooth, bilinear, CRT), INTEGER SCALE, the volume, FPS, the frame hook the console and the pause hang from;
      `--video-dump F... --video-dir`, `--actions` (the console's), `--eeprom FILE`. Check: "The Simpsons Arcade.exe"
      `--script attract --fast` with the three hashes the same as the reference's (7201 frames, by the agent before and
      after src/ was written again, and by me); the 25 scenarios from build/bin-shell identical; the pause waits for
      the frame.
- [x] **Controls** (ports/sdl2/controls.c): CONTROLS for each of the four players (keys, buttons, triggers, with the
      input prompts' pictures), controls.ini (the arcade game reads it too); the test switch. Check: CONTROLS seen in
      the dumped frames; a key changed in the ini moving player 2 on the test mode's IO page needs a real keyboard
      (SDL's dummy driver has none): to try in the window.
- [x] **Game controllers' mappings**: SDL_GameControllerDB built in (ports/sdl2/controllerdb, embed.cmake, its
      LICENSE), one text given as `SDL_HINT_GAMECONTROLLERCONFIG` with `SDL_HINT_OVERRIDE` before `SDL_Init`, with the
      environment's `SDL_GAMECONTROLLERCONFIG` and the player's gamecontrollerdb.txt; the log names each joystick and
      the first 48 keys and buttons. Check (SDL's dummy video): 885 mappings, 886 with a file of one Windows and one
      Linux line, 887 with a line in the environment too.
- [x] **A game controller before a TV's remote** (`is_remote`, `controller_added`, from tmnt-port's).
- [x] **The pack** (`simpsons.assets`, runtime/src/assets_pack.c: "SMAP", its format, a version, then each region's
      spans and bytes; `--write-pack`, `--assets` a folder or a pack, `--rom`), from a file or from bytes in memory
      for the devices; the version an FNV-1a of the layout's spans and the tables both programs' C reads by name
      (9BE7DBD6 on 13-11-2026). Check: as above; a pack of another version, of another format, cut short, and a ROM set
      that is not one, each refused with its own message.
- [x] **The window's first run** (ports/sdl2/first_run.c, letters.c, the font, from tmnt-port's): no assets found, a
      window that asks for the ROM set (simpsons.zip beside the game, or dropped on it) and says what it does and what
      went wrong; it writes simpsons.assets in the folder of what the game keeps (a pack of another version made
      again) and the game starts from it; `game_load` takes a pack as well as a folder. Check: an empty folder with
      simpsons.zip beside the executable: the pack written and the attract on; a wrong zip: its message.
      (13-11-2026, an agent; checked by me: the package unzipped in a new folder with simpsons.zip beside it writes
      the pack, 7180740 bytes, version 9BE7DBD6, in 0.24 s, and the attract's three hashes are the reference's, 7201
      frames. The agent also: the second run from the pack without simpsons.zip, the pack not written again; The
      Simpsons first and The Simpsons Arcade from its pack, one pack for both; no set nor pack: the window and the log
      say so; a pack of version 04030201: its message, or made again with the set beside it; a zip that is not the
      set: "Not the set: no 072-g02.16c". Search order: `--assets`, the pack beside the executable or the .app, in the
      folder of what the game keeps, the repository's assets/ folder, the first run.)
- [x] **The EEPROM kept** (The Simpsons Arcade): simpsons.nv (128 bytes) in the folder of what the game keeps, read at
      the start and written when the game writes it (`simpsons_eeprom_written`, a hook of the port's beside the board
      in runtime/src/machine.c: what the board does is the same); the factory one from the assets where there is none;
      `--script` and the hash options start from the factory one unless `--eeprom FILE`. Check: started from a .nv with
      VERY DIFFICULT, the test mode's options page shows it; FACTORY SETTING writes the file back to the factory's (sum
      1A18) and the next start shows NORMAL; the scenarios unchanged.
- [x] **The project's icons are the pictures Sebas chose** (13-11-2026, Sebas: "usa el arte que te mande para el icono,
      no hagamos de nuevo chanchadas": no placeholders), in the repository and in what the CI makes; local/ only for a
      player's own picture in their place (and the makers take a player's picture the same way). Made from his
      pictures: ports/switch/icon.jpg (the arcade's cover, 256 x 256); ports/android/res/ (Homer at the window: the
      launcher's 288 and 192, the TV banner 320 x 180 from the whole picture); ports/sdl2/icon/simpsons.png (256) and
      simpsons.icns (the computers: Windows' .ico through make_ico.py, which CMakeLists.txt now takes by default;
      Linux beside the game; macOS's bundle). They are the show's and Konami's art: published with the repository
      once Sebas pushes it.
- [x] **Sebas's pictures**, kept in local/ as he gave them:
      - [x] The Switch: the arcade game's cover (Sebas, 13-11-2026: "esta foto para el nsp de switch"),
            local/switch_cover.webp, made local/icon.jpg: 256 x 256, baseline JPEG, the whole cover in the middle on
            its own green.
      - [x] Android and the Android handhelds: Homer at the window (Sebas, 13-11-2026: "esta para el icono de android
            y portatiles"), local/android_homer.png (457 x 333), made local/icon_android.png (300 x 300, x 120-420:
            his face a little above the middle, the waving hand and the shoe at the sides), which android.cmake takes
            as tmnt-port's does; local/icon_android_preview.png shows it square and in a round launcher's mask.
- [x] **CMake**: Release by default where nothing says (`GENERATOR_IS_MULTI_CONFIG`); `simpsons` ("The Simpsons
      Arcade") and `simpsons_console` ("The Simpsons", its program library built with SIMPSONS_CONSOLE);
      `simpsons_package` (SDL2 inside with `SIMPSONS_STATIC_SDL2`, built from a copy of SDL 2.32.10's source in
      build/downloads/SDL2-src, else SDL2.dll beside them: both tried); version 0.1.0 and `SIMPSONS_VERSION_SUFFIX`.
- [x] **The headless for the port**: `--actions`, `--script`, `--video-dump`, each WIDE, so any option's frames are
      drawn without a device. (13-11-2026: "The Simpsons" itself, with SDL's dummy drivers: with `--fast` it draws
      nothing in its window, and the pictures `--video-dump` writes are the window's whole picture, as wide as WIDE
      draws it; `--wide off|16:10|16:9|20:9` (or 0-3), `--wide-check` (each frame, the columns of WIDE's sides whose
      tiles are not their map's) and `--arcade-start` (the arcade's attract and controls, so tests/controls' scenarios
      run in the console's build, WIDE in their games). Check: the stages' frames in the three widths, the menus and
      the pause by `--actions`: console/PLAN.md, 8.)
- [x] **The documents**: the port's README.md (short, for the player: play, the console version, controls,
      multiplayer, build, legal), docs/port.md (the port's technical side), docs/package/README.txt, licenses/ (SDL2,
      SDL_GameControllerDB, the menu's font, arcade-lan).

### A3. The Windows package

- [x] `simpsons_package`: both games alone (SDL2 inside), README.txt and the licenses; the CI's windows job
      (.github/workflows/build.yml). Check: the package unzipped in a new folder with simpsons.zip beside it: the first
      run, then the attract; `rom_in_executables` 0 over the package. (13-11-2026, an agent; checked by me:
      build/msvc-package/package/simpsons-windows-x64.zip, 2.9 MB: the two games, font.bmp, input_prompts.bmp,
      README.txt, LICENSE.txt, licenses/ (MAME, ymfm, SDL2, SDL_GameControllerDB, Kenney-Mini, Kenney-Input-Prompts);
      the first run and the attract as above; `rom_in_executables` 0 in both games of 358735 stretches, and 1 KB of
      the fixed program's code planted in a copy found, 32 and 32. The tool leaves out a stretch made of two runs each
      one value over and over (a byte, a 16-bit or a 32-bit word): MSVC's static runtime's character types gave 3
      false finds, kref's MAME masks one more; 6240 of 364975 stretches left out. The CI's windows job is written and
      has not run: the repository on GitHub is Sebas's to make.)

## C. The computers

- [x] **Linux** (WSL's Ubuntu 24.04, gcc, the system's SDL2; `-DSIMPSONS_BIN_DIR=build/bin-linux`). Check: the 25
      scenarios identical from build/bin-linux, with the game's thread of A1's stack and not `ulimit`'s; the console's
      tests the same frame by frame as on Windows. (13-11-2026, an agent: nothing to change for Linux. The 25 scenarios
      identical in state, picture and sound on the main thread's 8 MB, `ulimit -s` untouched (115 s, `--jobs 16`); a
      Debug build's attract and stage8 too. The window's first run from a fresh folder: the pack byte for byte
      Windows', the attract's three hashes the reference's. The console's 34 frames of six `--actions` runs the same
      pixels and state lines as an MSVC build of the same tree (the files apart only in the BMP header's image size,
      which MSVC 19.50's /O2 writes 0). The CI's linux-x64 package made here as its job makes it: its first run and
      attract the same; `rom_in_executables` 0; it needs glibc 2.38 or later.)
- [ ] **The Steam Deck**: Linux's package; WIDE 16:10 (B); its controller through the mappings. Check: on Linux only;
      the Deck itself is said untried until someone plays it there.
- [x] **macOS**: both games as universal .apps (arm64 and x86_64, macOS 11 on, signed ad hoc,
      io.github.cheviman.simpsons-arcade and .simpsons), each in a package of its own (TMNT's issue 7); the letters and
      the ROM set found from a translocated .app (`app_folder`, `SecTranslocate...`); macOS's own full screen
      (`mac_full_screen`). Check: `zig cc -target aarch64-macos` and `x86_64-macos` of main.c and first_run.c with
      `-Wall -Wextra`; the CI's job.
      - [x] The zig check (13-11-2026, an agent): main.c, first_run.c, controls.c and letters.c, arcade and console,
            for x86_64-linux-gnu, aarch64-macos and x86_64-macos with `-Wall -Wextra`: 24 compiles, 0 warnings, 0
            errors (to an object with `-c`: zig 0.17's `-fsyntax-only` fails renaming an object it does not write; a
            stand-in CoreFoundation.h, as tmnt-port's, and without it the `__APPLE__` branch fails, so it is
            compiled); the whole game (runtime and both programs, 181 files) for the three: 0 errors. CMake makes
            each game a `MACOSX_BUNDLE` with its identifier, the pictures in Resources, CoreFoundation linked.
      - [x] The CI's job run (the repository on GitHub first).
- [x] **The CI**: windows-x64, linux-x64 (.tar.gz: the artifacts' zip loses the executable bits), macos-universal
      (ditto .zip); on a tag v* the release with docs/releases/<tag>.md; the branch testing's pre-release
      (testing-build). (13-11-2026: .github/workflows/build.yml written from tmnt-port's, computers only, with
      docs/releases/testing.md; parsed and read again; not run until the repository is on GitHub.)

## D. The devices

### Every device

- [ ] Feature macros as tmnt-port's (`SIMPSONS_HANDHELD`, `SIMPSONS_TOUCH`, `SIMPSONS_SYSTEM_EXIT`,
      `SIMPSONS_SCREEN_WIDE`, `SIMPSONS_HANDHELD_KEYS`); the game reads the pack a maker put in, or takes the assets out
      of the player's simpsons.zip on the device (`device_folder`, `device_assets`: sdmc:/switch/simpsons/,
      PORTS/simpsons/, Android/data/<the app>/files/); with no ROM set, a message that says where to put it.
      - [x] main.c's (13-11-2026, the Switch and the Anbernic): `SIMPSONS_HANDHELD` (no first run's window, no
            VSYNC, the whole screen, The Simpsons' full_screen always on, no gamecontrollerdb.txt of the player's),
            `SIMPSONS_SCREEN_WIDE` (WIDE 16:9), `SIMPSONS_HANDHELD_KEYS` (the Anbernic's mapping, MENU leaves, the
            volume keys VOLUME's); `device_folder` and `device_assets` for sdmc:/switch/simpsons/ and PORTS/simpsons/
            (both games', and what they keep and their logs there): the pack there, else, or when it is another
            version's, made there from simpsons.zip (or rom/ with the .zip or the set's files) with the first run's
            steps (`game_load_rom_set`, `game_assets_pack_write`); none, the message with the folder (the Switch's by
            libnx's console till +). Check, a Linux build with `SIMPSONS_HANDHELD` and `SIMPSONS_HANDHELD_KEYS` in WSL
            (gcc, SDL's dummy video): no set, the message with the folder (exit 1); simpsons.zip there, the pack made
            (identical to build/assets_rom.assets) and The Simpsons Arcade's attract the reference's in its three
            hashes (7201 frames); again from the pack, the same; a pack of another version with the set there, made
            again and the same; without the set, both messages; rom/simpsons.zip, the same; The Simpsons from that
            pack to the script's end. main.c with `-Wall -Wextra` for both games: 0 warnings by Zig (aarch64 glibc 2.35)
            and devkitA64's gcc (the Switch).
      - [x] The console's menus (console/menu.c, 13-11-2026, as tmnt-port's): `SIMPSONS_SYSTEM_EXIT` takes out EXIT
            (the Switch's HOME, Android's), `SIMPSONS_HANDHELD` CONTROLS and VIDEO's WINDOW, FULL SCREEN, WIDE and
            VSYNC (VIDEO: PIXELS, INTEGER SCALE, FPS; WIDE AUTO follows the screen). The macros reach
            simpsons_console_program on the Switch, the Anbernic and Android. Check: an MSVC build with both
            (build/msvc-device): the menu without EXIT, OPTIONS without CONTROLS, VIDEO's three lines (console/PLAN.md).
      - [x] Android's (13-11-2026): `SIMPSONS_HANDHELD`, `SIMPSONS_SYSTEM_EXIT` and `SIMPSONS_TOUCH` (no
            `SIMPSONS_SCREEN_WIDE`: WIDE by the screen's shape, `follow_screen`); `device_folder` is the app's folder on
            the storage (`SDL_AndroidGetExternalStoragePath`: Android/data/<the app>/files/, each game's own, the log
            there too); the APK's own pack first (`load_apk_pack`: read whole through SDL's files, as tmnt-port's),
            else `device_assets` there; what the games keep in the app's own folder (`SDL_GetPrefPath`); the console's
            letters copied out of the APK into it (`android_letters`: console/draw.c reads font.bmp with the C
            library's files, which do not reach an APK's assets; tmnt-port's console has the same gap). Check: main.c,
            touch.c, controls.c and letters.c with `-Wall -Wextra` for both games by the NDK's clang, arm64-v8a and
            armeabi-v7a: 0 warnings; the first start's steps are the Every-device ones above, tried on Linux. On a
            phone, not tried.
- [x] The game's thread: none of its own (A1: the program needs less than 32 KB; every device's thread has more).
- [x] **32-bit ARM** (Android's armeabi-v7a: the Chromecast with Google TV, old phones): 4-byte pointers, char without
      a sign, the alignment of LDRD/LDM/VLDM. Check: ports/android/zig_arm_check.cmake (Zig, musl, Thumb-2, VFPv3-D16)
      and the 25 scenarios identical under WSL's qemu-arm (three at a time, no time limit for the long ones).
      (13-11-2026: simpsons_game_headless for thumb-linux-musleabihf with Zig 0.17.0, build/arm32 and build/bin-arm32,
      ELF 32-bit ARM EABI5, static; the reference's results copied into build/scenarios_arm32 and
      `python3 tools/check_scenarios.py --native-only --video --sound --jobs 13 --bin build/bin-arm32 --out
      build/scenarios_arm32` in WSL: the 25 identical in state, picture and sound, 343270 frames, in 16 min 46 s, the
      longest (stage8, 50101 frames) inside the tool's 15 minutes. No tasks here to switch, so nothing of TMNT's
      tasks.c. The NDK's own armeabi-v7a libraries are built in the APKs (android.cmake), not run here.)

### Nintendo Switch

- [ ] ports/switch/switch.cmake (devkitPro): both games, two titles in homebrew's range, npdm, control.nacp, the icon;
      the Switch's 1280 x 720, WIDE 16:9 in the console; each Joy-Con or controller a player; no EXIT (HOME), no
      window's VIDEO lines; CONTROLS since 19-11-2026 (What was done). The .nro of each without a pack for the
      package's sd/.
      - [x] The build (13-11-2026): devkitPro's MSYS2 shell, `/opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-cmake
            -S . -B build/switch -DSIMPSONS_REFERENCE_TOOLS=OFF -DCMAKE_DEPENDS_USE_COMPILER=FALSE
            -DSIMPSONS_BIN_DIR=build/bin-switch`, target `simpsons_switch_games` (26 s, 0 warnings): The Simpsons
            **050053494D500000** and The Simpsons Arcade **050053494D510000** ("SIMP" in ASCII; apart from tmnt-port's
            0500544d4e540000/...541000, outrunners-port's 05005e6a4f520000 and segasonic-port's 05005e6a50510000; the
            add-on bases nacptool writes, ...501000 and ...511000, fall on neither), each with main (NSO0), main.npdm
            (its name the folder, simpsons and simpsons-arcade: sixteen bytes there; ACI0 and ACID the title, main
            thread 1 MB), control.nacp (the name, Sebastian Toyos, 0.1.0, presence group and save owner the title) and
            icon.jpg; the .nro of each without a pack (build/bin-switch/switch-nro, 8.7 MB), and with
            build/bin-switch/simpsons.assets there, each .nro with it in its romfs (read back: the pack identical).
            The icon: local/icon.jpg on this computer (Sebas's), else ports/switch/icon.jpg, a placeholder of plain
            text (Kenney Mini on a flat blue) until the project has an icon of its own (A2); that branch not built here.
            `rom_in_executables`: the NSOs 0; the .nro 15 stretches, all the same 32 bytes (FFFFFFFF00000000 four
            times), Mesa's `override.0` in the ELF's .data (devkitPro's libGL under SDL), and the ELF one more in its
            .debug_loclists (not in the .nro nor the NSO): no byte of the ROM set (as tmnt-port's .nro of 09-11).
      - [ ] Not tried on a Switch: the Joy-Cons and controllers as players, HOME, the picture at 1280 x 720, the
            first start's pack on the SD card, the message on libnx's console.
      - [x] The CI's switch job (devkitPro's image, `simpsons_switch_games`: the .nsp pieces and the .nro without a
            pack, the icon the repository's ports/switch/icon.jpg). (13-11-2026: written as tmnt-port's, read against
            switch.cmake; not run: the repository on GitHub first. E.)
- [x] Simpsons NSP (ports/switch/nsp, tmnt-port's with this game's extraction and pack): the player's .nsps on a
      computer. Check: `tools/check_nsp.py` over .nsps made with made-up keys: every hash holds, the program and
      control data the build's, the pack the first run's. (13-11-2026: rom_pack.c is the first run's steps,
      game_load_rom_set and game_assets_pack_write, the decompiled game linked in; built with MSVC and SDL2 inside
      (build/nsp-maker, build/bin-nsp-maker) and on Linux (WSL, gcc). Run in a new folder with the ROM set and
      made-up keys: The Simpsons.nsp and The Simpsons Arcade.nsp, 12.1 MB each, in 0.9 s; check_nsp.py (which now
      reads main.npdm's program and name and control.nacp's name, publisher, version and presence group too, and
      compares control.nacp with the build's but for what the tool sets): every hash holds, the program and control
      data the build's, simpsons.assets identical to build/assets_rom.assets, both titles. The Linux maker's two .nsps
      the same bytes as the Windows maker's. Caught: other keys ("does not decipher to NCA3"), the placeholder icon
      given with --icon against the build's, keys missing, a ROM set that is not one. Zig for aarch64-macos and
      x86_64-macos with `-Wall -Wextra`: the 22 compiles of both makers, 0 warnings. `rom_in_executables` 0.)

### Android

- [x] ports/android/android.cmake (the NDK, no Gradle: javac, d8, aapt2, zipalign, apksigner): both games
      (io.github.cheviman.simpsons and .simpsons.arcade), arm64-v8a and armeabi-v7a in each APK, targetSdk 35, the whole
      screen (`SDL_WINDOW_FULLSCREEN_DESKTOP`, `full_screen` always on), back and the background a pause, the log in
      the app's folder. Simpsons APK (ports/android/apk), the player's APKs signed with the player's key; the base APKs
      signed by the CI with the project's key. Check: `apksigner verify` (v2, v3), both ABIs, the pack the first
      run's. (13-11-2026, tmnt-port's with this game's names: the NDK 28.2, build-tools 35.0.1 and JDK 17 of
      build/downloads/android, SDL2 from build/downloads/SDL2-src, Ninja from Visual Studio; `cmake -S . -B
      build/android -G Ninja -DCMAKE_TOOLCHAIN_FILE=<the NDK>/build/cmake/android.toolchain.cmake
      -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 -DSIMPSONS_REFERENCE_TOOLS=OFF -DSIMPSONS_BIN_DIR=build/bin-android
      -DJava_JAVAC_EXECUTABLE=<the JDK>/bin/javac.exe`, target `simpsons_apks` (68 s; armeabi-v7a a build of its own,
      build/android/abi/armeabi-v7a). build/bin-android/android: simpsons-base.apk and simpsons-arcade-base.apk (no
      pack, no signature, 2.9 MB) and this computer's The-Simpsons-debug.apk and The-Simpsons-Arcade-debug.apk (with
      build/bin-android/simpsons.assets, the icon local/icon_android.png, the debug key local/android-debug.keystore,
      made once by keytool: local/ is never in the repository). aapt2 dump badging: the packages, versionCode 100,
      0.1.0, minSdk 24, targetSdk 35, native-code arm64-v8a and armeabi-v7a, leanback-launchable-activity, banner=,
      touchscreen, gamepad and leanback not required; zipalign -c -p 4 OK; apksigner verify of the debug APKs: v2 and
      v3; read back, the pack stored and identical to build/assets_rom.assets, the libraries ELF AArch64 and ARM.
      Simpsons APK (MSVC, SDL2 inside: build/apk-maker, build/bin-apk-maker) run in a new folder with simpsons.zip and
      the two base APKs in android/: The Simpsons.apk and The Simpsons Arcade.apk in 0.8 s, apksigner verify v2 (the
      tool's, as tmnt-port's: Android 7 on), CN=The Simpsons, zipalign OK, the pack in each identical to
      build/assets_rom.assets; again with the same key, the same bytes; with --icon and --banner, the entries the
      player's pictures. `rom_in_executables` 0 in the four libmain.so, both libSDL2.so, classes.dex and Simpsons APK.
      No permission in the manifest: no LAN game yet.)
      - [x] The CI's android job: the base APKs, and signed with the project's key (secrets) as tmnt-port's.
            (13-11-2026: written, the runner's NDK and SDK, build-tools 35.0.1, Java 17; the secrets ANDROID_KEYSTORE
            and ANDROID_KEYSTORE_PASSWORD, its key's alias simpsons, without them no ready/; no key in the repository,
            the build's own debug APKs stay on the runner. Read against android.cmake; not run. E.)
      - [ ] Not tried on a phone: installing, the immersive full screen, back, the background's pause, the first start
            making the pack of simpsons.zip in Android/data/<the app>/files/ (Android 11 on keeps file managers out of
            Android/data: a computer's USB, or Simpsons APK's pack in the APK).
- [x] **The touch controls** (ports/sdl2/touch.c): a stick, ATTACK and JUMP, START (COIN in The Simpsons Arcade), player
      1's; their size by the screen's DPI (at most 0.35 inch), the picture at the top on a tall screen; the labels in
      8/10 of a button; the phone's vibration with an option to turn it off. Check: a drawing of the same formulas
      over the headless's frames for an open Fold (2176 x 1812), a 20:9 phone and a 16:10 tablet. (13-11-2026,
      tmnt-port's touch.c: START pauses a player in the game and brings one in, the pause worked by the fingers too
      (pause_events, players_pressed); a key or a game controller hides them, but Android's back and the volume keys.
      No vibration: the cabinet has no motor to shake, as TMNT's. Checked with the real code instead of a drawing: a
      desktop build with `-DSIMPSONS_TOUCH_CHECK=ON` (build/android-check: touch.c in both games, main.c's --screen
      WIDTHxHEIGHT and the whole screen presented written at --video-dump's frames, the density from
      SIMPSONS_TOUCH_DPI) under SDL's dummy video, at 2400 x 1080 and 2340 x 1080 (400 dpi), an open Fold's 2176 x
      1812 and 2000 x 1830 (373 dpi), a 16:10 tablet 2560 x 1600 (274 dpi) and a TV's 1920 x 1080 (no touch): The
      Simpsons' menu and pause and a stage of both games seen (build/android_work/shapes_*.png): on the phones the
      controls on the picture's sides; on the open Folds (The Simpsons' 16:9, The Simpsons Arcade's 4:3 on the 2000 x
      1830 one) the picture at the top and the controls under it, else over its corners (the arcade's 4:3 on 2176 x
      1812 leaves less than a fifth under it, as tmnt-port's drawing); on the tablet small over the corners; ATTACK,
      JUMP, START and COIN inside their buttons. That build's attract the reference's in its three hashes.)
- [x] **WIDE by the screen's shape**, chosen again when its size changes (`follow_screen`: a Fold opened mid-game).
      (13-11-2026, main.c's, for a touch screen's build: 20:9 from twice as wide as high (tmnt-port's comment says so,
      its code asks for 20:9 itself, which left a 19.5:9 phone at 16:9 with a tenth of the screen black at each side),
      16:9 from 16:9 and on a screen 4 inches or more on its short side, 16:10 from 3:2, else OFF. Seen in the shapes
      above: 2400 x 1080 and 2340 x 1080 20:9, both Folds, the tablet and the TV 16:9, each with its log line.)
- [x] **Android TV and Google TV**: LEANBACK_LAUNCHER, leanback and the touch screen not required, a 320 x 180 banner;
      the remote known by its buttons, back as JUMP in the console, OK the START of a player out of the game. Check:
      `aapt dump badging` says `leanback-launchable-activity` and `banner=`. (13-11-2026: both; the banner and the
      launcher's icons placeholders of plain text as the Switch's icon (ports/android/res), Simpsons APK taking the
      player's own; the remote's back is not the pause (`remote_back`), home pauses (SDL_APP_WILLENTERBACKGROUND).
      Not tried on a TV.)

### iOS and iPadOS (Sebas, 13-11-2026: "sumemos una version de ios")

The first of the ports on iOS: none of the others has it. The App Store is not the way (the game is Konami's, and the
player brings the ROM set), so the player installs it by sideloading.

- [ ] **The build**: both games on SDL2's UIKit backend (SDL from its source, as the other platforms' releases), arm64,
      iPhone and iPad, from Xcode's toolchain on macOS (CMake's Xcode generator or the iOS toolchain), by the CI's
      macos-15 runner (the program's stack is the main thread's: A1). The whole screen (status bar and home indicator hidden,
      SDL_HINT_IOS_HIDE_HOME_INDICATOR), the picture inside the screen's safe area (the notch and the island), WIDE by
      the screen's shape as on Android (`follow_screen`), the sound by AVAudioSession's playback category.
      - [x] Written and compiled without a Mac (13-11-2026): ports/ios/ios.cmake (CMake's Xcode generator, target
            `simpsons_ios_games`; SDL2 2.32.10 from its source, static, with SDL2main, no HIDAPI: its Bluetooth would
            ask a permission), Info.plist.in, LaunchScreen.storyboard (black), Assets.xcassets (the app icon, Homer at
            the window: the 1024 x 1024 of ports/sdl2/icon/simpsons.icns), not signed unless SIMPSONS_IOS_TEAM;
            CMakeLists.txt's iOS lines (SIMPSONS_IOS and SIMPSONS_DEVICE, SDL_GameControllerDB's iOS lines, the include);
            main.c's `__APPLE__ && TARGET_OS_IOS` branches: the window (FULLSCREEN_DESKTOP, BORDERLESS, ALLOW_HIGHDPI),
            the hints (landscape, the home indicator dimmed, AVAudioSession's playback, the idle timer off, no
            accelerometer joystick, no mouse from the fingers), the picture and WIDE between the sides the notch or the
            island takes (`ios_side_inset`: the UIWindow's safeAreaInsets through objc_msgSend), nothing drawn from the
            background (an event watch). Check: Zig 0.17 for `aarch64-ios` and `aarch64-ios-simulator` with Zig's own
            Darwin headers (`-isystem lib/libc/include/any-darwin-any`: the real TargetConditionals.h and objc/), main.c,
            controls.c, letters.c and touch.c of both games with `-Wall -Wextra`: 16 compiles, 0 warnings, Mach-O arm64
            for iOS (platform 2) and its Simulator (7), the iOS branches in them, main renamed SDL_main; the whole game
            (runtime, src/, console/: 181 and 192 files) for aarch64-ios, 0 errors; `rom_in_executables` 0 over all
            those objects; ios.cmake parsed by CMake. The desktop the same: build/msvc-shell's two games rebuilt, The
            Simpsons Arcade's attract (7201 frames) identical to the reference in its three hashes.
      - [x] The CI's ios job (built for an iPhone or iPad; read back: the Info.plist's keys, the executable arm64 for
            iOS, the launch screen and the icon compiled into the bundle, the .ipa's entries) and ios-simulator job
            (built for the Simulator, both apps started there with no ROM set: each must write its log in Documents and
            say where to put simpsons.zip; a screenshot each). Written, parsed, not run: only they show Xcode building
            the project (the storyboard, the asset catalog, SDL's CMake under the Xcode generator, the link).
- [ ] **The controls**: the touch controls (touch.c, by the screen's size in points), the game controllers SDL finds
      (MFi, Xbox, PlayStation) as on Android, START pausing; the background a pause.
      - [x] main.c (13-11-2026): the touch controls' size by the screen's density as on Android, the renderer at the
            screen's pixels (ALLOW_HIGHDPI) and SDL_GetDisplayDPI's density in the same pixels (SDL 2.32's UIKit: its
            table of models, else the screen's points times nativeScale at 163 a inch on an iPhone, 132 on an iPad);
            the background pauses The Simpsons (SDL_APP_WILLENTERBACKGROUND, as Android); game controllers by SDL's MFi
            driver with SDL_GameControllerDB's 42 iOS lines. Compiled as above; not tried on a device.
- [ ] **The assets**: as on the other devices, from the player's simpsons.zip on the device: the app's folder shown in
      Files (UIFileSharingEnabled and LSSupportsOpeningDocumentsInPlace), the game taking them out on its first run
      there (`device_folder`, `device_assets`); with no ROM set, a message saying where to put it.
      - [x] main.c (13-11-2026): `device_folder` and the folder of what the games keep are the app's Documents
            ($HOME/Documents/, the Files app's On My iPhone > the game), the log there too; `device_assets` as on the
            other devices; with no ROM set, the message names the Files app's folder and a computer's Finder. Compiled
            as above; the message is the ios-simulator job's check; the pack made on a device untried.
- [ ] **The packages**: an unsigned .ipa of each game, with no assets, in the release (simpsons-ios.zip), which the
      player installs with AltStore or SideStore (signed with the player's own Apple ID; a free one's last 7 days, then
      they sign it again), or TrollStore where the iOS version allows it; its README says how, and that the Simpsons
      .ipa is never on the App Store.
      - [x] simpsons-ios.zip (13-11-2026): ports/pack_devices.py `--ios` (README, LICENSE, licenses, ios/ with the two
            .ipa), tried here with stand-in .ipa files; docs/package/README-ios.txt (AltStore, SideStore, Sideloadly,
            TrollStore, the 7 days, Developer Mode, the Files app, never the App Store). The .ipa themselves (ios.cmake:
            the unsigned app in Payload/, by ditto) only from the CI.
- [ ] Check without an iPhone: built by the CI for the device and for the Simulator; the console's headless and the
      scenarios built for the Simulator and run in it on the CI (`xcrun simctl`), the same frames as on the computers;
      the .ipa's Info.plist read back (the bundle IDs, the file sharing keys, the orientations); `rom_in_executables`
      0 over the .ipa. On an iPhone or iPad, said untried until someone installs it (Sebas's Mac builds it for one).
      (13-11-2026: the CI has no ROM set, so no scenario can run in its Simulator: the ios-simulator job starts both
      apps there with none instead. The scenarios in the Simulator are for Sebas's Mac; `rom_in_executables` over the
      .ipa when the CI's are downloaded.)

### Anbernic (H700, the stock firmware)

- [x] ports/anbernic/anbernic.cmake with zig_aarch64.cmake (glibc 2.35), linked against ports/anbernic/sdl2_stub.c:
      nothing newer than SDL 2.0.12 (no `SDL_RenderSetVSync`, no rumble query); the handheld's buttons as a game
      controller, MENU leaves, the volume keys, 640 x 480 at 4:3. Simpsons Anbernic (ports/anbernic/maker) writes the
      card's PORTS. Check: readelf says aarch64, glibc at most the firmware's, NEEDED libSDL2-2.0.so.0 and the C
      library alone; every SDL function in the stub. (13-11-2026: Zig 0.17.0 of build/downloads, `cmake -S . -B
      build/anbernic -G Ninja -DCMAKE_TOOLCHAIN_FILE=ports/anbernic/zig_aarch64.cmake -DSIMPSONS_ANBERNIC=ON
      -DSIMPSONS_REFERENCE_TOOLS=OFF -DSIMPSONS_BIN_DIR=build/bin-anbernic`, target `simpsons_anbernic_games` (12 s):
      build/bin-anbernic/anbernic is PORTS without the pack, The Simpsons.sh, The Simpsons Arcade.sh, and simpsons/
      with both games, font.bmp and input_prompts.bmp. readelf (WSL): ELF aarch64, /lib/ld-linux-aarch64.so.1, glibc
      2.34 at most, NEEDED libSDL2-2.0.so.0, libm.so.6 and libc.so.6, no RPATH; linked against the stub, so every SDL
      function in it; their 70 SDL functions none newer than 2.0.12 by SDL 2.32's headers. Simpsons Anbernic (MSVC,
      and on Linux in WSL) run in a new folder with the ROM set: PORTS with the build's six files and simpsons.assets
      identical to build/assets_rom.assets, the Linux maker's the same files. The handheld's first start making the
      pack in PORTS/simpsons/ checked in a Linux build of the same branches (Every device). `rom_in_executables` 0.
      .gitattributes as tmnt-port's: the .sh with LF wherever they are checked out. Not tried on a handheld: its
      buttons, MENU, the volume keys, the picture, the firmware's SDL 2.0.12 running it.)
      - [x] The CI's anbernic job (Zig 0.17.0 and SDL 2.32.10's headers downloaded on the runner,
            `simpsons_anbernic_games`). (13-11-2026: written as tmnt-port's, read against anbernic.cmake and
            zig_aarch64.cmake; not run. E.)

## E. The release

- [ ] A package for each computer (Windows; Linux; macOS's two), and one for each device (simpsons-switch.zip,
      -android.zip, -anbernic.zip by ports/pack_devices.py): the device's games without assets, ready where they can be
      (sd/, ready/, anbernic/), its tool for Windows, macOS and Linux beside them, font.bmp, the licenses, its README.
      Check: the release's packages downloaded and gone through: nothing of the ROM set (no .nro with a pack,
      -debug.apk, .idsig, .assets), each tool finds its device's folder beside it.
      - [x] ports/pack_devices.py (13-11-2026, tmnt-port's with this game's names, and simpsons-ios.zip): run here on
            the local builds (build/bin-switch, bin-android, bin-anbernic; the Windows makers of build/bin-nsp-maker and
            bin-apk-maker; stand-ins for the macOS and Linux makers and the .ipa): simpsons-switch.zip (22 files: the
            .nsp pieces and sd/switch/simpsons/ with the two .nro without a pack; this computer's .nro with the pack
            left out), simpsons-android.zip (14: the base APKs; the debug APKs and .idsig left out; no ready/ without
            the key), simpsons-anbernic.zip (18, the programs 755), simpsons-ios.zip (10); none with an .assets,
            -debug.apk, .zip or assets/. Their READMEs: docs/package/README-switch, -android, -anbernic and -ios.txt.
      - [x] The CI's device jobs: switch, android, anbernic, ios and ios-simulator, devices (the computers' kits:
            Simpsons NSP, Simpsons APK and Simpsons Anbernic with font.bmp and the licenses, which the computers'
            packages now leave out), and release and testing with the eight packages. (13-11-2026: written from
            tmnt-port's; parsed by PyYAML, every bash step through `bash -n`, the artifacts taken all made, the
            targets and paths read against CMakeLists.txt and the ports' cmake files. Not run: the repository on GitHub
            first.)
- [ ] The release's notes: what each change answers, and what was not tried on a device.
- [x] The repository on GitHub (cheviman/simpsons-port, made by Sebas); support by GitHub Sponsors; commit, push and
      tag only when Sebas says.

## F. Last: installing on the Steam Deck (the four ports)

Sebas, 13-11-2026: "los scripts para que se instale en el steamdeck", "vamos primero con el de los simpsons, pero es
extensivo, tmnt, outrunners y segasonic", and then "eso va al fondo": after everything above. outrunners-port's issue
27 (12-11-2026) asked for "an appimage (which is the preference) or a shell script install"; its player closed it
when 1.2.2's Linux executable ran on the Deck.

- [x] **One installer for the four ports**, the same files in each (ports/linux/: install.sh, uninstall.sh and a
      Python module for Steam's shortcuts, standard library only: SteamOS has python3), in each Linux package beside
      the games. It needs nothing of a port's own: the games are the ELF executables in its folder ("The Simpsons
      Arcade", "TMNT", "OutRunners"...), the icon the folder's .png, the ROM set the folder's .zip, and the saves are
      already in ~/.local/share/<the game> (SDL_GetPrefPath). It copies the folder to one of the player's
      (~/Games/<the folder>), with the ROM set if it is beside the script or in ~/Downloads; writes each game's
      .desktop entry (~/.local/share/applications, the icon by its path); and adds each game to Steam as a non-Steam
      game in every Steam user's config/shortcuts.vdf (~/.local/share/Steam/userdata/*, and Flatpak's Steam's), with
      Steam closed (it offers to close it and starts it again): Steam ROM Manager's way, which leaves the shortcut's
      appid, name, executable, folder and icon ours. Not SteamOS's `steamos-add-to-steam`: nothing of Valve's
      documents it, and the community's notes have its steam://addnonsteamgame URL changing. Its messages in a dialog
      (zenity or kdialog, both on SteamOS) when it was double-clicked rather than run in a terminal. uninstall.sh takes
      out what install.sh put in (the shortcuts by their executable's folder, the .desktop entries, the folder after
      asking), never the saves.
- [x] Check without a Deck: in WSL, a stand-in package (two ELF executables, a .png, a .zip) and a stand-in HOME with
      a shortcuts.vdf holding another game: after install, the file read back by a reader apart from the writer has
      both games and the other one untouched; a second install adds nothing; uninstall leaves the other one alone;
      `desktop-file-validate` passes; Steam running is closed and started again (a stand-in steam). The writer
      first given a real shortcuts.vdf to read and write back byte for byte (this computer's Steam has one, 13 bytes:
      no shortcuts). On a Deck, said untried until a player runs it.
- [x] Each port's CI packs the three files into its Linux package; its README and docs/package/README.txt say how
      (desktop mode, the folder with the ROM set, install.sh); the release's notes say it.
- [ ] An AppImage (the issue's first choice): weighed after the script.

## G. The distribution maker: a project of its own (proposed)

Sebas, 13-11-2026: "en carpetita aparte, es codigo fuente de win, linux, mac, de los generadores del proyecto, de los
apk, nsp para que se pueda usar en linux, se puedan compilar libremente, por ahi hacer un pseudo framework que sea
'generador de distribuidor' y le agregamos la parte del codigo, instrucciones basicas y lo haga, podria ser un
proyecto aparte".

What there is today: each port has its own copy of the three makers the player runs on a computer with the ROM set:
the NSP's (ports/switch/nsp: the NCA and PFS0 formats, AES and SHA-256, the window and its folder dialog), the APK's
(ports/android/apk: the zip, APK signatures v2 and v3, ECDSA, the player's key) and the Anbernic's
(ports/anbernic/maker: the card's PORTS). tmnt-port's are about 6,700 lines; outrunners-port's and segasonic-port's are
the same files with other names in them (line endings aside, a handful of lines a file), but for make_apk.c, 194 lines
apart: TMNT added the TV banner and its two APKs later, and a fix in one port reaches the others only by hand. Each is
built inside its port's CMake, linked to the game's extractor (tmnt_extract, rom_pack.c), and for the three computers
only by that port's CI.

- [ ] **The generic part**, with no game in it: the three makers' code (formats, crypto, zip, signatures, the window,
      the dialogs, the letters), in a repository of its own taken by each port as a submodule, as arcade-lan is (its
      name to choose). It builds alone, with its own CMake, on Windows (MSVC, MinGW), Linux (gcc, clang) and macOS
      (universal), so that anyone can build the makers from their source and run them on Linux as on Windows.
- [ ] **The game's part**, what a port gives it: a manifest (the games' names, the Switch's title IDs, Android's
      packages, the version, the icons and the TV banner, what each device's folder holds) and its code through a small
      C interface: the player's ROM set into the assets' pack (the port's extractor, as rom_pack.c is today). The basic
      instructions: the manifest's fields and a page on how a new port fills them.
- [ ] **What it makes** from a port's CMake (one call with the manifest) and in the CI's kits as today: "<game> NSP",
      "<game> APK", "<game> Anbernic" for the three computers; the devices' packages (ports/pack_devices.py) there too.
- [ ] **Check**: for TMNT, its makers given the same inputs as tmnt-port's make the same outputs: the NSP's files and
      hashes (tools/check_nsp.py), the APK's entries byte for byte and `apksigner verify` (v2, v3), PORTS's files; built
      and run on Windows and on Linux (WSL), built for macOS with `zig cc`; `rom_in_executables` 0 over each maker.
- [ ] **The ports move to it**, one at a time, each taking its own copy out: The Simpsons first, then tmnt-port,
      outrunners-port and segasonic-port, each in a session of its own, with its release's notes.

## H. The project's quality: the Recomp Standards still open

Sebas, 15-11-2026, on RecompLauncher's report: "lo de sdl3 y retroachievements, ponelos en plan, porque es calidad de
proyecto". What RecompLauncher's Recomp Standards ask that the port does not do yet (the small ones were done the same
day: What was done, 15-11-2026).

- [ ] **SDL3.** The window port on SDL3 instead of SDL2 (SDL3's game controllers: more of them known and named alike,
      gyro, touchpads, lights; what RecompLauncher builds on). Every platform's: ports/sdl2 (the window, the sound, the
      controls, the touch screen), the devices' builds (the Switch's devkitPro SDL, Android's, iOS's) and the Anbernic's
      stub (its firmware's SDL is 2.0.12: it keeps SDL2, or a layer between). Large: the four ports share the shell, so
      it is one move for all of them, tmnt-port first as the model. Check: every device's build, the scenarios through
      the window's headless runs, the controllers' mappings counted as in ports-dispositivos point 3.
- [ ] **Recomp Achievements** (github.com/elripalda/RecompAchievements): achievements, Beaten and Mastered, the
      leaderboards, through its small C library, which reports unlocks to RecompLauncher. The rules on the board's work
      RAM (the stages cleared, each boss, the ending: Beaten; the game on one credit, the bonus games' best, BOSS RUSH
      and FRANK GRIMES LIFE in The Simpsons Mr. Plow Edition: Mastered), read after each frame as the LAN reads its
      state. Medium. Check: a scripted run through each rule's moment unlocks it once, and the scenarios stay identical
      (it reads, it never writes).
- [ ] **Button prompts for PlayStation controllers** (RecompLauncher's RECOMPLAUNCHER_PAD, and SDL's controller type
      for everyone): CONTROLS shows Xbox's letters only; Kenney's Input Prompts have the PlayStation set, a second sheet
      chosen by the controller's type. Medium.
- [ ] **A ROM set as .7z on the devices and in the makers**: the computers' games read simpsons.7z (rom_set_7z.c); the
      Switch, Android, iOS and Anbernic games look for simpsons.zip in their folder, and Simpsons NSP, APK and Anbernic
      take a .zip. Small to medium (the same reader, in their builds).
- [ ] **The Simpsons Arcade Game's own tile in RecompLauncher**: its recomplauncher.json's `games` adds arguments to one
      program, and the two games are two executables. Waits on the launcher (a program for each game), or a choice of
      the game by an argument.

## What was done

- **13-11-2026**: the clone and this plan (Sebas: "arranquemos el port").
- **13-11-2026**: the Switch's icon from Sebas's picture (local/icon.jpg); the Steam Deck's installer planned for the
  end (F).
- **WSL did not start for a while** (13-11-2026: HCS_E_HYPERV_NOT_INSTALLED after the processor's change); later the
  same day it runs again: Ubuntu 24.04, kernel 6.6.87.2, gcc 13.3, SDL2 2.30.0, CMake 3.28.3, 32 processors.
- **13-11-2026**: the window's first run, the Windows package, the CI's workflow and the zig checks (an agent; the
  package's first run, attract and `rom_in_executables` checked again by me).
- **13-11-2026**: Linux in WSL (an agent): both games and the headless tools built with gcc 13.3 and the system's SDL2
  with nothing changed; the 25 scenarios, a Debug build, the first run, the console's frames and the CI's package
  checked (C, Linux).
- **13-11-2026**: the Nintendo Switch and the Anbernic from tmnt-port's (an agent): ports/switch (switch.cmake,
  npdm.json.in, the placeholder icon.jpg, Simpsons NSP), ports/anbernic (anbernic.cmake, zig_aarch64.cmake, the stub,
  the launchers, Simpsons Anbernic), tools/check_nsp.py, main.c's device branches, .gitattributes; both devices built,
  the makers built and run on the ROM set, the .nsps checked, the device's first start tried on Linux (D). The
  console's menus for the devices (console/menu.c) and the CI's switch and anbernic jobs are still to do; nothing
  tried on a device.
- **13-11-2026**: Android from tmnt-port's (an agent): ports/android (android.cmake, AndroidManifest.xml.in, the
  placeholder icons and TV banner in res/, zig_arm_check.cmake, Simpsons APK), ports/sdl2/touch.c, main.c's Android
  and touch branches (the APK's pack, the app's folders, the log, the hints, the immersive full screen, WIDE by the
  screen's shape, the TV's remote, the background's pause), CMakeLists.txt's Android, simpsons_apk_maker and
  SIMPSONS_TOUCH_CHECK; both APKs built for both ABIs and checked, Simpsons APK run on the ROM set, the touch controls
  and WIDE seen at six screens' shapes with the real code, the 25 scenarios identical on 32-bit ARM under qemu-arm (D).
  The CI's android job is still to do; nothing tried on a phone, a Fold, a tablet or a TV.
- **13-11-2026**: the CI's device jobs and iOS (an agent): .github/workflows/build.yml with the switch, android,
  anbernic, ios, ios-simulator and devices jobs and the computers' kits (release and testing with the eight packages),
  ports/pack_devices.py, docs/package/README-switch, -android, -anbernic and -ios.txt; ports/ios (ios.cmake,
  Info.plist.in, LaunchScreen.storyboard, Assets.xcassets with Homer's icon), CMakeLists.txt's iOS lines and main.c's
  iOS branches (D, E). Checked here: the workflow parsed and its bash steps, pack_devices.py on the local builds, the
  iOS C compiled by Zig for iOS and its Simulator, the desktop's attract identical. Nothing of it run on GitHub, built
  by Xcode, or tried on an iPhone or iPad.
- **13-11-2026, the release's ground** (the lead, Sebas: "subile el readme lindo, con screens"): github.com/cheviman/
  simpsons-port made by Sebas and pushed (c7a15d9; the core's CONSOLE_ORIGINAL 789290a on arcade-decomp's simpsons);
  README.md for the player with eight screenshots (docs/screenshots/, the console's frames at 2x), docs/port.md. The
  CI's first run: windows-x64, linux-x64, macos-universal, switch, android, ios (the device's build), anbernic and
  devices all green; ios-simulator running. The Steam Deck's installer (F) for this port: ports/linux/install.sh,
  uninstall.sh and steam_shortcuts.py, in the Linux package; checked in WSL with a stand-in package, HOME and Steam
  (Steam running closed and started again; both games added after another game's shortcut, its bytes as they were;
  a second install adds nothing; uninstall leaves shortcuts.vdf the same bytes as before; Steam's empty 13-byte file
  read and written back the same). desktop-file-validate is not installed in WSL (nothing downloaded): the entries
  checked by hand. tmnt-port, outrunners-port and segasonic-port take the same three files in their own sessions.
- **13-11-2026, the plan closed here** (Sebas): the console's LAN in a session of its own (done the same day:
  console/PLAN.md, 11); G (the distribution maker)
  and the other ports' Steam Deck installers later, each in its own session. b05bf45 stays local until LAN is in
  (Sebas: "No hagas el ci si falta el lan").
- **14-11-2026, the players' issue 2 (the Switch)** (Sebas: "acomodá lo de switch"): both games' folder on the SD card
  is sdmc:/switch/simpsons-arcade/ (1.0.0's sdmc:/switch/simpsons/ is The Simpsons: Hit & Run's port's too; Sebas: "le
  pondría the simpsons arcade"). At the first start without it, the games copy their own files from the old folder
  (switch_take_old_folder: simpsons.zip, simpsons.assets, simpsons.scores, simpsons.state, simpsons.nv by their names;
  simpsons.ini and controls.ini only with their own key and first line) and leave it as it was. The Simpsons on the
  Switch no longer goes full screen (apply_settings, toggle_full_screen): devkitPro's SDL has its desktop mode
  1920 x 1080 always (SWITCH_VideoInit, read in libSDL2.a's SDL_switchvideo.o), so SDL_WINDOW_FULLSCREEN_DESKTOP made the
  window 1080p and the 720p screen showed part of the picture (the .nsp's screenshot in the issue). The pack, the
  package's README-switch.txt and the README say the new folder. tmnt-port, outrunners-port and segasonic-port make
  their Switch window full screen the same way: to look at in their sessions.
- **14-11-2026, the console version's name** (Sebas: "en los nombres del apk y los exe, que sea the arcade game"): The
  Simpsons is The Simpsons Arcade Game in every build and package (the .exe, the .app, the Linux executable, the .apk,
  the .ipa, the .nro and the .nsp, the Anbernic's launcher, the window's title, the HOME menu's and the launchers'
  names, the makers' defaults), The Simpsons Arcade as it was. Kept, so nothing a player has is lost: the folder of
  what the games keep (~/.local/share/The Simpsons, ~/Library/Application Support/The Simpsons), Simpsons APK's key
  folder, the Android and iOS identifiers, the Switch's title ids. The makers' buttons say "Make the .nsp files" and
  "Make the .apk files" (the new name is 304 pixels in their letters, the buttons 300). Checked: the Windows games and
  the three makers built (MSVC), the Switch's two .nro (devkitPro), rom_in_executables none in the games.
- **14-11-2026, the names again** (Sebas: "que sea la de consola mr plow edition", Mr. Plow being Don Barredora; "The
  Simpsons Arcade" beside "The Simpsons Arcade Game" said arcade twice): the console version is The Simpsons Mr. Plow
  Edition and the board's as it is The Simpsons Arcade Game, in every build and package as above; the identifiers, the
  folder of what the games keep and the Switch's folder simpsons-arcade as they were. Checked as above: the Windows
  games and the makers, the Switch's two .nro, rom_in_executables none.
- **14-11-2026, 1.0.1** (Sebas: "Comité, pushea y hace versión y cerramos x hoy"): the players' issues 2 and 3, WIDE's
  stage 1 goon (a player's report), the sides' floor, the bounces and stage 3's climb (console/PLAN.md); version
  1.0.1 (CMakeLists.txt), docs/releases/v1.0.1.md, testing.md's list emptied; main fast-forwarded to testing, the
  tag v1.0.1 once the CI is green on that commit. Not tried on a device.
- **15-11-2026, RecompLauncher** (Sebas: "Add RecompLauncher support to this recomp", with recomplauncher.com's
  agent guide): docs/package/recomplauncher.json in the computers' packages (simpsons_package, the workflow's Linux
  and macOS step), with which RecompLauncher installs The Simpsons Mr. Plow Edition from a release, asks for the ROM
  set (simpsons.zip or its folder), makes the pack with no window (`--prepare --game=`), starts the game with
  `--game=`, shows 14 of OPTIONS' settings (simpsons.ini) on the game's page and backs up the high scores, the state,
  the settings, the controls and the EEPROM. The computers' games take `--game ROMSET` (first_run_found: as one
  dropped on the window, or a folder with simpsons.zip in it) and `--prepare` (the pack alone, its progress on the
  standard output, 0 or 2); the console's simpsons.ini reader takes "key = value" too. The README's badge. Checked:
  the schema (0 errors), the Windows games (MSVC, no warnings), `--prepare` with the set (0), another zip (2), the
  folder (0), no set with and without the pack (0, 2); the game with `--game=` and no pack (the pack made, started)
  and with no arguments; "volume = 0" silent and "volume = 10" as "volume=10" (SDL's disk audio); the 25 scenarios
  identical. Not tried in RecompLauncher itself: the ROM set is asked for with its schema's `inputs` and
  `{input:rom}`, whose guide (SETUP-INPUTS.md) is not public.
- **15-11-2026, the Recomp Standards and two players' issues** (Sebas: "preparemos esas recomendaciones, y tenemos un
  par de bugs que arreglar", the issues 14 and 12; "ponelos en plan" for SDL3 and Recomp Achievements: H above):
  - **Issue 14, Bart's shirt** (console/bart_shirt.c): Bart falling into the gorge at stage 5's end and in its water
    (the scenery's kind 10 in the family's colour: its six sprites of him; its splashes left light blue), the walk to
    Channel 6's lower shirt (a plain tile, 6201, outside its rule's range), and Maggie standing (sprite 5974), which
    Bart's own lists draw too (02's 6130, the far family's 7F76, 39's): never marked, her onesie light blue. Found by
    scanning the console's frames of stages 4-8, the stories, the attract mode and bonus 2 for the light blue left in
    the play area, and the pixels the shirt changes, cropped and seen. Checked: the gorge and Channel 6's frames have no
    light blue left, and every pixel that differs from the shirt off is the light blue turned orange (1191 of 1191);
    Maggie's two cells are drawn by those three lists alone (all the kinds' lists searched). Not seen: Bart picking
    Maggie up in a run (the scenarios' rescuer is Marge).
  - **Issue 12, high-DPI Windows** (main.c): SDL_HINT_WINDOWS_DPI_SCALING, the game DPI-aware, its windows in points and
    its pictures in the screen's pixels (present draws in the renderer's output); the first run's window keeps its
    layout (a logical size, whole times). The log says the window and its picture's pixels, and what the settings made
    of it. Checked at 100 % only (the same sizes as before); not on a scaled desktop.
  - **Logs** (main.c, log_into): the last three runs kept (FILE_NAME.log, .1.log, .2.log), on Linux and macOS too when
    the standard error is not a terminal. Checked on Windows: four starts from no console leave three logs.
  - **portable.txt** on Linux and macOS: what the game keeps beside it (beside the .app). Not run there (WSL's build
    below compiles it).
  - **simpsons.ini commented**: each key with what it takes (the reader skips the comments, and takes "key = value").
  - **RecompLauncher's Big Picture** (RECOMPLAUNCHER_MODE=bigpicture): the game full screen that time, simpsons.ini's
    window kept. Checked with SDL's dummy video: full screen 1024 x 768, the file unchanged; a window in desktop mode.
  - **The Simpsons Arcade Game's quit with a controller**: Back and Start held together two seconds. Not tried with a
    controller.
  - **A ROM set as .7z** (rom_set_7z.c, the LZMA SDK 26.06's 7z decoder in ports/sdl2/lzma, public domain, its notice
    in the packages' licenses): dropped, beside the game, in a folder given, or with --game; its files put beside the
    pack, read as a set's folder, taken away. Checked: LZMA2 and LZMA archives make the zip's pack byte for byte, a
    folder with simpsons.7z and one beside the game too, another 7z is refused (2) and leaves nothing.
  - recomplauncher.json: the .7z, portable.txt and the logs' folder. Not done: the diamond Sebas saw changed (not
    found in the scans).
  - **Barney's colours** (Sebas: "hay que arreglar tmb el tema de los colores de barney, que estan los de season1"):
    his hair in brown tried and left yellow ("dejá el pelo amarillo y cambiale el color de la remera y listo"), his
    shirt in #D46F63: console/PLAN.md's What was done.
- **19-11-2026, the players' issues 18 and 21** (Sebas: "vamos con tanda de errores, revisate el 21, 18, 16"; 16,
  Otto at his stall, was in testing already: seen again in this build, stage 2 from the Konami code's page in 20:9,
  Otto there through the opening's frames 2600 to 2820, the play from 2876).
  - **18, BOSS RUSH's card not shown** ("it freezes when you select a character then it skips straight to the
    boss"): the crossing's frames are hurried (console_hurried: not waited for, one picture in 16 shown, the card on
    black over them), and with VSYNC present() leaves out a picture behind the pace (frame_behind: past next_frame
    plus a frame's time); next_frame moves in wait_for_frame alone, which hurried frames do not call, so every hurried
    picture after the first frame's time was behind. Measured (Windows, Direct3D 11, VSYNC ON, stage 1's crossing,
    frames 1056 to 2704): 104 hurried pictures, 1 shown; here that one was the card, so it looked right; on a slower
    machine none, the character choice left on the screen till the boss. In the game since 1.0.0.
    The fix (on_frame_done): a hurried frame sets next_frame to now before its picture (no pace to be behind; the
    pace goes on from the last one). After: 104 of 104 shown; the story skipped whole's 19 of 19 (console/PLAN.md).
  - **21, the Switch's speed and sound** ("unstable FPS that bounces around between the high 30's and low 50's.
    Animation speed matches the frame rate, and the audio suffers"): measured here (Ryzen 9 5900XT, --fast, SDL's
    dummy video, stage 1's play, 4000 frames) a frame drawn takes 2.8 ms in 4:3 and 3.4 ms in WIDE 16:9, the
    Switch's; one not drawn (BOSS RUSH's crossing, 1800 frames) 1.0 ms. A core of the Switch's, some 6 to 8 times
    slower: 20 to 27 ms a frame drawn against the board's 16.9, the player's 37 to 50 a second. A frame longer than
    its time made the game late, and past four frames the pace started again from there (wait_for_frame): the game
    and its sound as slow as the pictures, the sound's queue empty at every frame.
    The fix (main.c, both games): a frame that ends behind its time, by half a frame's time or by a whole one with
    VSYNC, leaves the next one undrawn (late_skip, game_set_draw: the board's state the same either way), three in a
    row at most; the game and its sound keep the board's pace with fewer pictures. With VSYNC a machine that slow
    showed no picture at all, every one behind: never two left out in a row now (left_out), and none while frames go
    undrawn within 30 frames of one another, for 120 frames (late_pace: each frame drawn is shown).
  - **--slow MS and --no-vsync** (both games, for checks: a slow device's frames on a computer): each frame drawn
    takes MS milliseconds more, and every 600 frames the standard error says the frames a second, how many were drawn
    and shown and how often the sound's queue was empty; VSYNC off whatever the settings say, as the devices' builds.
  - Checked (Windows, a window, stage 1's play, frames 1800 to 3000). --slow 20 --no-vsync, the device's case, before:
    37 frames a second, 600 of 600 drawn and shown, the sound out 600 times in 600 frames; after: 59.1 to 59.3 (the
    board's 59.19), 394 to 396 drawn and shown, the sound out 0 times. --slow 20 with VSYNC, before: 38.8 a second, 0
    shown of 600 drawn, the sound out 600 times; after: 59.1 to 59.2, 402 drawn and shown, 0 times. --slow 40: 220
    drawn and shown, 59.0 to 59.3; --slow 12: 600 drawn, 563 to 570 shown; --slow 1: 600 drawn and shown, 59.19, as
    without it. The Simpsons Arcade Game, --slow 20 --no-vsync: 59.2, 399 of 600 drawn. The Switch's two games built
    (devkitPro), no warnings (MSVC; GCC -Wall -Wextra on the changed files); the 25 scenarios identical in state,
    picture and sound.
  - Not tried on a Switch. Not done: the drawing's own speed (70 % of a frame; the runtime's, in the
    decompilation's repository), the Switch's CPU boost, and the player's other request, custom button mapping on the
    Switch (OPTIONS has no CONTROLS on the devices).
- **19-11-2026, CONTROLS on the Switch** (Sebas: "hagamos el mapeo de los botones tmb en switch"; the players' issue
  21's other request, "custom button mapping"). The Switch's games already read controls.ini from the SD card and
  held each player's controls by it (controls.c, controls_holds), and took a pressed button for the control CONTROLS
  waits for (read_events, bind_button): the page alone was left out of OPTIONS with the devices' fixed controls
  (SIMPSONS_HANDHELD), and its pictures had no file to come from.
  - **SIMPSONS_DEVICE_CONTROLS** (switch.cmake, on the console's program): OPTIONS has CONTROLS (menu.c:
    FIXED_CONTROLS is a device without it, the Anbernic and Android as before), "PRESS A BUTTON", and the page draws
    no keys, each control's buttons side by side (draw_controls, up to three, CONSOLE_CONTROLS_NEXT_X apart). While it
    waits, a button of another player's controller leaves the control as it was (bind_button): a device has no Escape
    for it, and with the shown player's controller not there the page waited for ever.
  - **The Switch's buttons by their letters** (button_prompts, `__SWITCH__`): devkitPro's SDL has the controller by
    the buttons' places, an Xbox controller's (its mapping in libSDL2.a: "a:b1,b:b0,x:b3,y:b2", libnx's buttons in
    their order, A B X Y, the rest as their bits: b6 L, b8 ZL, b10 +, b11 -), so SDL's A, the lower button, is the
    Switch's B, its B the A, its X the Y and its Y the X; the sheet's white A B X Y, L, R, ZL, ZR, - and + (Kenney's
    Input Prompts Pixel has them). So the default ATTACK is the Switch's B and JUMP its A: README-switch.txt said A
    and B, put right.
  - **The sheet in the executable** (ports/sdl2/prompts/embed.cmake, simpsons_prompts, SIMPSONS_PROMPTS_EMBEDDED:
    load_prompts reads it with SDL_LoadBMP_RW): an .nro or an .nsp's executable has no file beside it; 627 KB more.
  - **Two checks on a computer** (CMake options, never a player's build): SIMPSONS_PAD_CHECK, the games with
    `--pads N` (SDL's virtual joysticks as game controllers, plugged in as the game starts) and --actions' `FRAME pad
    BUTTON PLAYER` (SDL's names; a scripted run then reads the players' controls), and SIMPSONS_SWITCH_CHECK, The
    Simpsons Mr. Plow Edition with the Switch's definitions, its buttons' pictures out of the executable, and --pads
    (build/msvc-switch-check, build/bin-switch-check).
  - Checked (the check build, SDL's dummy video, one virtual controller, the pictures written): OPTIONS with CONTROLS
    first; the page with the d-pad's four, ATTACK B, JUMP A, START +, no keys; ATTACK chosen, "PRESS A BUTTON", SDL's
    x pressed: ATTACK Y and B, and that button then opens CONTROLS from OPTIONS as the attack does (a opens it too,
    b goes back); DOWN and the left shoulder: L and the d-pad; UP and the right trigger: ZR and the d-pad; START and
    Back: - and +; DEFAULTS: as at first. PLAYER 2, who has no controller, a control chosen, then a button and a
    trigger of player 1's: the page back, nothing bound. Built: MSVC (the games as before, PAD_CHECK off), the
    Switch's two games (devkitPro, no warnings; simpsons_prompts_bmp in the ELF), GCC -Wall -Wextra on main.c and
    menu.c in each configuration (the computers', the Switch's, the Anbernic's, with and without the checks).
  - Not tried on a Switch: the page itself, whether the letters shown are the buttons' own (the mapping's reading
    above), a controller plugged in while the page waits, a single Joy-Con held sideways. Not done: CONTROLS on the
    Anbernic and Android (their buttons' pictures); Back (the Switch's -) still pauses whatever is bound to it. The
    25 scenarios not run again: nothing of theirs changed here (the window's main.c, the console's menu.c, CMake),
    and the checkout had the video's changes of another session in progress, with its own runs going.
- **The video's own speed, issue 21's root cause** (19-11-2026, a session of its own; Sebas, as it was measured: "si
  es una tecnica que sirve, tengamosla en cuenta para los otros ports"): runtime/src/video.c drew a pixel at a time
  (the three tile layers 68 % of a drawn frame, the two passes from pens to colours 17 %, the sprites 6 %). Now a
  tile's row and a sprite cell's row are taken from the ROM whole and an empty one passed at once, a layer with one
  scroll is drawn a tile at a time, the palette's pens are made again only for the colours that changed, a line
  becomes colours in one pass and WIDE's frame copies the board's columns. The video: 2.09 -> 0.25 ms a frame in WIDE
  16:9, 1.62 -> 0.19 in 4:3; a frame drawn, the way issue 21 was measured (--fast, SDL's dummy video, stage 1's play,
  4000 frames): 2.82 -> 0.94 ms in WIDE 16:9, 2.28 -> 0.77 in 4:3. Not a pixel changed: the 25 scenarios identical
  (state, picture, sound); every frame of every script in WIDE off, 16:10, 16:9 and 20:9 and of two games of The
  Simpsons' own the same as before (`simpsons_console_headless --wide-hashes`, new: 216 runs' files, 2,914,160
  hashes); the video before and the new one drawing the same random boards (flips, every scroll, zoom, shadows, WIDE's
  negative sides, the recolouring, saved states), 800,000 frames the same, three planted bugs caught. MSVC and GCC
  -Wall -Wextra, no warnings; nothing of the ROM in the executables. Made in the decompilation (runtime/src/video.c
  there, its commit 78b39f4, its 25 scenarios identical) and brought by the merge (20-11-2026: 23d02c5, the conflict
  resolved with the port's tested file); video.h says the recolouring's functions are of the colour alone. Not tried
  on a Switch. The technique for the other ports: the skill ports-dispositivos, point 36.
- **20-11-2026, 1.0.2** (Sebas: "cuando tengas todo, responde los tickets 21,22, commit, push, y version nueva"): the
  players' issues 21 (the video three times faster, CONTROLS on the Switch, the late frames left out) and 22 (INTRO >
  ATTRACT), WIDE at stage 2's end and stages 3's and 5's starts (console/PLAN.md), and the rest of testing's list;
  version 1.0.2 (CMakeLists.txt), docs/releases/v1.0.2.md, testing.md's list emptied; main fast-forwarded to testing,
  the tag v1.0.2 once the CI is green on that commit. The 25 scenarios identical after the merge. Not tried on a
  device.
- **20-11-2026, 1.0.3** (Sebas: "Compita y push a teating y saca version"): the players' issues 23 (the arcade's
  attract mode after the Konami logo), 24 (WIDE: a scene as 4:3 with black sides), 25 and 27 (Bart's shirt as he turns
  at the ending; Maggie light blue in his arms, painted by Sebas, the video's keep masks), stage 2's teacup ride in
  WIDE, a LAN game's end and its settings, and the README for players (console/PLAN.md); version 1.0.3
  (CMakeLists.txt), docs/releases/v1.0.3.md, testing.md's list emptied; main fast-forwarded to testing, the tag v1.0.3
  once the CI is green on that commit. The 25 scenarios identical. Issue 26 (stage 6 closing) not reproduced in 35
  runs: waiting for the player's log. Not tried on a device.
- **23-11-2026, 1.0.4** (Sebas: "ok, comiteemo, hagamos version nueva y push"): the players' issues 29 (WIDE: stage
  2's mailboxes, its fat goons' obstacles, Krusty's balloon fight, Lisa's box closed at its right, the screen's edges by
  name in every stage), 30 (the Switch's .nro with no space in their names, for a forwarder), 31 (stage 8: Smithers
  stuck off the screen), 32 (the ending in Mr. Burns' office as 4:3, Lisa's box's top corner and her box on the
  character choice; the hat throwers' hat is the cabinet's own), 34 (stage 7's end: one Smithers, the lift-off, the
  scene before stage 8), 35 (stage 3's elevator) and 36 (the name entry with two characters), and the menu without
  INTRO with the Konami code kept on it (console/PLAN.md); version 1.0.4 (CMakeLists.txt), docs/releases/v1.0.4.md,
  testing.md's list emptied; main fast-forwarded to testing, the tag v1.0.4 once the CI is green on that commit. The
  25 scenarios identical. Not tried on a device.
