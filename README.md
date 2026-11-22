# The Simpsons — port

<a href="https://recomplauncher.com"><picture><source media="(prefers-color-scheme: dark)" srcset="https://recomplauncher.com/assets/press/launcher/badge-light.svg"><img alt="Get it on RecompLauncher" src="https://recomplauncher.com/assets/press/launcher/badge-dark.svg" height="48"></picture></a>

Konami's 1991 arcade game *The Simpsons*, for up to four players, as a game of its own on your computer (Windows,
Linux, macOS, the Steam Deck), the Nintendo Switch, Android, iPhone and iPad, and Anbernic's handhelds. Not an
emulator: the arcade's own program, decompiled into C ([how it's made](#how-its-made)). Every download has two games:

- **The Simpsons Mr. Plow Edition**: the arcade game made for playing at home: a menu, options, widescreen, save
  states, new modes and play over your local network ([what's in it](#the-simpsons-mr-plow-edition)).
- **The Simpsons Arcade Game**: the arcade game as it was: coins, the test mode, four players.

**You bring the game itself**: your own `simpsons.zip`, MAME's ROM set of *The Simpsons* (4 Players World, set 1).
Nothing of the game is in this repository or in its downloads.

<p align="center">
  <img src="docs/screenshots/presentation.png" width="32%" alt="The family walks across the game's big Konami logo">
  <img src="docs/screenshots/menu.png" width="32%" alt="The menu on the family's TV: ARCADE, MR. PLOW MODE, OPTIONS, EXIT">
  <img src="docs/screenshots/character_choice.png" width="32%" alt="Each player picks a character on the game's own portraits">
  <br>
  <img src="docs/screenshots/four_players_wide.png" width="32%" alt="Four players downtown in 16:9, Bart in his orange shirt">
  <img src="docs/screenshots/boss_rush_krusty.png" width="32%" alt="BOSS RUSH: Marge against the Krusty balloon, in 16:9">
  <img src="docs/screenshots/konami_code.png" width="32%" alt="The Konami code's page: stage select, lives, health, repeated characters">
  <br>
  <img src="docs/screenshots/boss_rush_card.png" width="32%" alt="BOSS RUSH's card before each boss">
  <img src="docs/screenshots/pause.png" width="32%" alt="The pause: CONTINUE, SAVE STATE, LOAD STATE, RESTART, QUIT GAME">
</p>

## Playing it

There is nothing to install, and no Python or command line: those are only for
[building it from source](#building-it-from-source).

1. Download the package for your system from the
   [latest release](https://github.com/cheviman/simpsons-port/releases/latest): `simpsons-windows-x64.zip`,
   `simpsons-linux-x64.tar.gz` or `simpsons-macos-universal.zip` (on macOS The Simpsons Arcade Game comes in a
   package of its own, `simpsons-arcade-macos-universal.zip`).
2. Unzip it anywhere.
3. Put your `simpsons.zip` beside the games, or drop it on the game's window when it asks for it.
4. Double-click **The Simpsons Mr. Plow Edition**.

The first time, the game takes what it needs out of your ROM set in under a second; from then on it just plays.

- **macOS**: the games are not signed with a certificate. The first time, open one with a right click and Open (on
  macOS 15 and later: System Settings, Privacy & Security, Open Anyway).
- **Steam Deck, or any Linux**: in desktop mode, put `simpsons.zip` in the folder and run `install.sh`: both games
  go to the applications menu and to Steam, so they show up in Game Mode. `uninstall.sh` takes them out again.
- **A launcher**: [RecompLauncher](https://recomplauncher.com) sets The Simpsons Mr. Plow Edition up for you.

Only that ROM set works: MAME's other sets of the game (the 2 Players ones, the Japanese and Asian ones) have
program ROMs of their own, which the decompilation does not cover, and the game refuses them. `simpsons.7z`, the set
packed by 7-Zip, works too.

### On other devices

Each device has a download of its own. On most of them a small tool, run once on your computer (Windows, macOS or
Linux), makes the game with your ROM set inside, and you copy what it makes to the device. The `README.txt` in each
download walks you through it.

- **Android** (`simpsons-android.zip`): phones, tablets, foldables, and TVs with Android TV or Google TV (the
  Chromecast with Google TV too). Put your `simpsons.zip` beside **Simpsons APK** and double-click it: it makes
  `The Simpsons Mr. Plow Edition.apk` and `The Simpsons Arcade Game.apk`, signed with a key of your own. Copy them
  to the device and open them there. Make the next updates on the same computer: each one then installs over the
  last and keeps your settings and scores ([more](ports/android/apk/README.md)).
- **Nintendo Switch** (`simpsons-switch.zip`, a console with homebrew): `simpsons.nro` (Mr. Plow Edition) and
  `simpsons-arcade.nro` (Arcade Game) for the Homebrew Menu or a forwarder, with `simpsons.zip` in `/switch/simpsons-arcade/` on the SD
  card; or **Simpsons NSP** makes `The Simpsons Mr. Plow Edition.nsp` and `The Simpsons Arcade Game.nsp` for the
  HOME menu.
- **iPhone and iPad** (`simpsons-ios.zip`): unsigned `.ipa` files to install with AltStore, SideStore or
  Sideloadly, then `simpsons.zip` in the game's folder with the Files app or the Finder. They are not, and will not
  be, on the App Store.
- **Anbernic's H700 handhelds** (`simpsons-anbernic.zip`, the stock firmware): the folder for PORTS, with
  `simpsons.zip` in `PORTS/simpsons/`; or **Simpsons Anbernic** makes it with the game inside.

What a tool makes has the game in it: it is yours alone, never share it.

## The Simpsons Mr. Plow Edition

- **The Konami presentation**: the family walks across the game's own big Konami logo, then Bart has a word.
- **The menu on the TV**: the family's TV from the attract mode, the title on its screen.
- **ARCADE**: the arcade's game with no coins: each player picks a character on the game's own portraits, one each,
  and START brings a player in at any time, up to four. After a continue, or for a new player, START picks again.
  OPTIONS sets its lives and its continues (0 to 9, or UNLIMITED).
- **MR. PLOW MODE**:
  - **FREE PLAY**: the arcade game from stage 1, as many continues as you like.
  - **BOSS RUSH**: the bosses one after another, from the wrestler to Mr. Burns, each stage crossed to its boss
    out of sight.
  - **BONUS GAMES**: the two bonus stages in a row, for one to four players, with their results.
  - **FRANK GRIMES LIFE**: one player, the whole game, against what four players would face.
- **INTRO**: the story before stage 1, Smithers taking the diamond and Maggie with it; it plays by itself too when
  the menu is left alone half a minute. START skips its scenes, Escape or Back leaves it.
- **MULTIPLAYER**: up to four devices on the same local network play one game, a character each: one hosts a room
  (HOST), the others find it (JOIN), and the host's START begins ARCADE's game on all of them with the host's
  settings. Built on [arcade-lan](https://github.com/cheviman/arcade-lan); Escape or Back leaves the game.
- **OPTIONS**: controls for four players, difficulty, lives, ARCADE's continues, demo sound, screen, video (window,
  full screen, sharp, smooth, bilinear or CRT pixels, integer scale, WIDE AUTO, 16:10, 16:9 or 20:9, VSYNC, FPS),
  volume, and SOUND: MONO, as the cabinet wires it, or STEREO, the music's channels where the program pans them. The
  version is under the menu and in OPTIONS.
- **WIDE**: the picture as wide as your screen (AUTO follows its shape), the stages drawn past the cabinet's 320
  columns, the players' boxes in the middle.
- **The pause** in a game: CONTINUE, SAVE STATE, LOAD STATE, RESTART, QUIT GAME. In the story scenes, the stages'
  title cards and ends and the ending, START skips the scene instead (as the attack does), and nothing pauses there.
- **The high scores** are kept between sessions (the cabinet forgets them at power-off).
- **Bart's shirt** is orange, as on the show (the cabinet dresses him in light blue), and **Barney's** a salmon (the
  cabinet's is green).
- And the Konami code on the menu: a stage select, infinite lives or health, and two or more players with the same
  character.

## Controls

- **Keyboard**: player 1's arrows move, Z or left Ctrl attacks, X jumps, Enter or 1 is START; player 2 is R F D G,
  A, S and 2; player 3 I K J L, right Ctrl, right Shift and 3; player 4 the keypad's 8 2 4 6, 0, its point and 4.
  Escape pauses (and goes back in the menus). F11 or Alt+Enter is the full screen, F3 the next kind of pixels.
  OPTIONS > CONTROLS changes each player's keys and buttons (`controls.ini`).
- **Game controller**: the pad or the left stick moves, A attacks, B jumps, Start is START; Back or Start pauses.
  Each controller is a player, in the order they are plugged in (a TV's remote after them). One the game does not
  know, or knows wrong, takes its line of [SDL_GameControllerDB](https://github.com/mdqinc/SDL_GameControllerDB)'s
  format in `gamecontrollerdb.txt`, beside the game on Windows (in `~/.local/share/The Simpsons` on Linux,
  `~/Library/Application Support/The Simpsons` on macOS).
- **The Simpsons Arcade Game**: 5 to 8 put a coin in each slot (a controller's Back, its player's), F2 is the test
  switch, and a coin and the attack bring a player in. Escape quits, or a controller's Back and Start held together two
  seconds.
- **Touch screens** (Android, iPhone, iPad): a stick where your left thumb lands, ATTACK and JUMP at the right,
  START (COIN in The Simpsons Arcade Game); a controller hides them.

## Shortcuts and launchers

A shortcut or a launcher can name the ROM set instead of keeping it beside the games: `--game PATH` (the .zip, the
.7z, or its folder). `--prepare` takes the game's graphics and sounds out of it (`simpsons.assets`) with no window
and ends. RecompLauncher sets The Simpsons Mr. Plow Edition up this way, from the release's `recomplauncher.json`,
and shows OPTIONS' settings on the game's page.

## How it's made

The games are built on the game's decompilation, [simpsons-decomp](https://github.com/cheviman/simpsons-decomp):
both of the arcade board's programs (the Konami 053248's and the Z80's) in C, checked frame by frame against MAME in
state, picture and sound. The Simpsons Arcade Game is that program as it is; The Simpsons Mr. Plow Edition replaces
some of its parts with the console version's (`console/`).

## Building it from source

You don't need this to play: it is for developers. Python 3.10 or newer, CMake 3.20 or newer, a C11 / C++17 compiler and SDL2 2.30 or newer. The simplest SDL2 is the
releases' own: `-DSIMPSONS_STATIC_SDL2=ON` builds it from its source into the games (CMake fetches it). Otherwise
the system's (`libsdl2-dev` on Debian and Ubuntu). Clone it with its submodule and build it:

```bash
git clone --recursive https://github.com/cheviman/simpsons-port
```

```bash
cmake -S . -B build/cmake -DSIMPSONS_STATIC_SDL2=ON -DSIMPSONS_REFERENCE_TOOLS=OFF
```

```bash
cmake --build build/cmake --config Release
```

The games are `The Simpsons Arcade Game` and `The Simpsons Mr. Plow Edition`, in `build/bin/`: they take their assets out of
`simpsons.zip` the first time (beside them, or in `rom/`), and on Windows
`cmake --build build/cmake --config Release --target simpsons_package` packs them as the releases do. Each device's
build is in `ports/PLAN.md`; the console version's plan and what is done in `console/PLAN.md`. The decompilation
itself (its status, the tools, the assets' formats, the checks against MAME) is
[simpsons-decomp](https://github.com/cheviman/simpsons-decomp)'s README.

## Supporting it

The games are free, and stay free. If you want to support the decompilations and the ports, there is
[Patreon](https://www.patreon.com/c/ChevimanPorts), where supporters also have a say in what comes next, and
[GitHub Sponsors](https://github.com/sponsors/cheviman): entirely optional, nothing is locked behind either. If
anyone asks you to pay for these games, it is not this project.

## Legal

- This project is licensed under the BSD 3-Clause License; see `LICENSE`.
- The C in `src/` is the game's programs decompiled into a new form: logic only, with names and comments written
  for this project. No ROM contents (graphics, sound, data) and no extracted assets are distributed by this
  project; they come from your own ROM set. The screenshots in `docs/screenshots/` show the game running from this
  code.
- `runtime/` follows the behaviour of MAME's `simpsons.cpp`, `k052109.cpp`, `k053246_k053247_k055673.cpp`,
  `k053251.cpp` and `k053260.cpp` and of ymfm's OPM (all BSD-3-Clause).
- The game controllers' mappings are [SDL_GameControllerDB](https://github.com/mdqinc/SDL_GameControllerDB)'s
  (`ports/sdl2/controllerdb`, zlib License), built into the games on the computers, Android and iOS.
- A ROM set packed as .7z is read with the [LZMA SDK](https://www.7-zip.org/sdk.html)'s 7z decoder
  (`ports/sdl2/lzma`, public domain), built into the games on the computers.
- The first run's window writes with Kenney Mini and CONTROLS shows Kenney's Input Prompts (www.kenney.nl), CC0.
  The window port links SDL2 (zlib License); the packages carry it inside the executables, with its terms and the
  ones above in `licenses/`.
- The icons are the arcade game's cover (the Switch) and Homer at his window from the show (the computers, Android,
  iOS): Konami's and the show's art.
- *The Simpsons* is a trademark of its owner, and the arcade game is Konami's. This project is not affiliated with
  or endorsed by either, nor by Nintendo, Apple, Google or Anbernic.
