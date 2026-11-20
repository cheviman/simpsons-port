# The Simpsons — how the port is made

The port's own pieces, for those who build or change it: how it runs the decompiled game, its first run, the
packages and the console version's tools. How each device is built and packed is in `ports/PLAN.md`, the console
version's plan and what was done in `console/PLAN.md`, and the decompilation itself (its status, the tools, the
assets' formats, the checks against MAME) in [simpsons-decomp](https://github.com/cheviman/simpsons-decomp)'s README.

## How the port works

- **Game code.** The Konami 053248's and the Z80's machine code is decompiled to C (`src/`, `src/sound/`), so the
  game runs natively with no CPU emulation. `runtime/src/game_program.c` runs it: the program calls the port back at
  each frame's end (its frame waits), the IRQ and the sprite DMA's FIRQ are functions, the sound board runs on its
  own timing (`runtime/src/sound_cpu.c`). The decompiled program needs less than 32 KB of stack: no thread of its
  own on any platform.
- **Data.** The game loads its assets from a pack, `simpsons.assets` (`runtime/src/assets_pack.c`: "SMAP", a format,
  a version, each region's spans and bytes), into the video and sound chips' memories and the programs' data piece
  by piece. It opens no ROM.
- **Video and sound.** The K052109, K053246/K053247, K053251 and the palette are a library
  (`runtime/src/video.c`) that draws each frame into a 320 x 224 picture (wider in WIDE); the YM2151 and K053260
  make the samples a platform plays (`runtime/src/sound_board.c`; STEREO takes the YM2151's second output).
- **Platforms.** A port provides a window, input, audio output and the frame's pace through the board's hooks
  (`runtime/include/simpsons/machine.h`, `runtime/include/game/game.h`); `ports/sdl2/` serves the computers and
  the devices, each with its own branches and CMake file (`ports/switch`, `ports/android`, `ports/ios`,
  `ports/anbernic`).

## The first run

A player has no Python: with no assets, the game takes them out of the ROM set itself, once
(`ports/sdl2/first_run.c`).

- **Where.** It looks for `--assets`, then `simpsons.assets` beside the executable (or the .app), then in the folder
  where the game keeps its files (beside the executable on Windows; `~/.local/share/The Simpsons` on Linux,
  `~/Library/Application Support/The Simpsons` on macOS), then the repository's `assets/` folder (development).
- **With none**, it looks for `simpsons.zip` (or `rom/`) beside the game, the .app or that folder, checks every file
  of MAME's `simpsons` set, writes `simpsons.assets` there (well under a second) and plays from it. Without a set, a
  window asks for it (or for it dropped on the window) and says what went wrong. A pack of another version of the
  game is made again from the set.
- **Devices.** The Switch, the Anbernic and iOS make the pack from `simpsons.zip` in their folder the first time;
  Android's APK carries it, made on a computer by Simpsons APK. The makers (Simpsons NSP, Simpsons APK, Simpsons
  Anbernic) use the same code.

## Options for the checks

Both games take `--script FILE` (a control script, `runtime/include/simpsons/script.h`) with `--fast`, and
`--hashes`, `--video-hashes` and `--sound-hashes FILE` (each frame's state, picture and sound, as the reference
writes them), `--video-dump FRAME ... --video-dir DIR` (frames as BMP), `--assets FOLDER|PACK` and `--eeprom FILE`.
The Simpsons adds `--actions FILE` (the menus' and the players' presses by frame), `--wide off|16:10|16:9|20:9|auto`,
`--wide-check`, `--arcade-start` (the arcade's scripts in the console's build, its picture the cabinet's),
`--stereo`, `--scores FILE` and `--state FILE` with `--state-save FRAME` and `--state-load`.
`simpsons_console_headless` (built beside them on a computer) is The Simpsons Mr. Plow Edition with no window, for
quick checks: `--frames N` ends it (600 unless given, or the script's length), with `--script`, `--actions`,
`--wide`, `--wide-check`, `--wide-hashes FILE` (each frame's hash of the picture WIDE draws: two builds' videos compared
in every frame), `--arcade-start`, `--video-dump FRAME ... --video-dir DIR`, the hashes and `--dump FRAME ...
--dump-dir DIR` (the board's memories); some 2000 frames a second.

- `py tools/check_scenarios.py --native-only --video --sound --jobs 16 --bin build/bin --out build/scenarios`: the
  25 scenarios against their references (always `--native-only`: without it the references are written again).
- `py tools/rom_in_executables.py [EXECUTABLE ...]`: no stretch of the ROM set in any of the games.
- `simpsons_assets_check`: the pack written, read back and made from the ROM set, the same bytes.

## The console version's checks

Every change to The Simpsons is seen in its frames (`--actions`, `--video-dump`, contact sheets) and The Simpsons
Arcade Game stays identical to the reference in its 25 scenarios. The console replaces a few of the game's functions
(`console/replaced.txt`): `tools/decompile.py` writes them as `CONSOLE_ORIGINAL(name)`, the original kept as
`original_name` in the console's build and the game's own everywhere else. Bart's shirt and the players' boxes in
WIDE are drawn by the port's side of the video (nothing unless the console asks), so the game's state is the
arcade's.
