The Simpsons Arcade Game and The Simpsons Mr. Plow Edition
==========================================================

Konami's The Simpsons (1991), the arcade game for four players, as a native game: decompiled to C and
checked frame by frame against MAME, github.com/cheviman/simpsons-port. Nothing of the game comes in
this package: it takes the game's graphics and sounds out of your own ROM set.

Two games come in it, the same program:
- The Simpsons Arcade Game, the arcade game as it is: coins, the test mode, the four players, its EEPROM.
- The Simpsons Mr. Plow Edition, the console version: the game made for playing at home, with menus of its own
  and no coins (START brings a player in).

They are "The Simpsons Arcade Game.exe" and "The Simpsons Mr. Plow Edition.exe" on Windows,
"The Simpsons Arcade Game.app" and "The Simpsons Mr. Plow Edition.app" on macOS, and
"The Simpsons Arcade Game" and "The Simpsons Mr. Plow Edition" on Linux. On macOS each comes in a package of
its own: simpsons-macos-universal.zip has The Simpsons Mr. Plow Edition, and
simpsons-arcade-macos-universal.zip The Simpsons Arcade Game.

What you need
-------------
simpsons.zip, MAME's ROM set of The Simpsons (4 Players World, set 1), as MAME has it: only that set (the
other sets, 2 Players, Japan, Asia, have program ROMs of their own, which the game refuses). The set packed
by 7-Zip, simpsons.7z, works as well. Put it beside the games, or drop it on the game's window when it asks
for it; a folder rom beside the games with the set's files (or simpsons.zip) in it works too. A shortcut or
a launcher can name it instead: --game PATH (the .zip, the .7z, or its folder); --prepare takes the
graphics and sounds out of it with no window and ends. Keep the games' pictures (font.bmp, input_prompts.bmp)
beside them: the window that asks for the set and The Simpsons' menus write with them (on macOS they
are inside the .apps).

The first time a game starts it takes the game's graphics, sounds and data out of the ROM set (in
less than a second) into one file, simpsons.assets, and keeps it: beside the games on Windows, in
~/.local/share/The Simpsons on Linux, in ~/Library/Application Support/The Simpsons on macOS. From then
on both games start from it and never open the ROM set again. A new version of the games makes it
again from the ROM set when it needs to (keep simpsons.zip where it was, or drop it on the window
again). simpsons.assets comes from your ROM set, so it is yours alone: never share it.

macOS: the .apps are not signed with a certificate. The first time, open one with a right click and Open (on
macOS 15 and later: System Settings, Privacy & Security, Open Anyway). Linux: start
"./The Simpsons Arcade Game" or "./The Simpsons Mr. Plow Edition" from the folder; the games need SDL2's usual
libraries of a desktop (X11 or Wayland, ALSA or PulseAudio), which they load themselves. Steam Deck (or any
Linux): in desktop mode, put simpsons.zip in this folder (or in ~/Downloads) and run install.sh: it copies the
folder to ~/Games/simpsons, adds both games to the applications menu and to Steam as non-Steam games (closing
Steam meanwhile and starting it again), so they show in Game Mode. uninstall.sh takes them out again; the
saves stay.

What the games keep goes in the same folder as simpsons.assets: controls.ini (each player's keys and
buttons, both games), simpsons.ini (The Simpsons' OPTIONS, each one explained in it), simpsons.nv (The
Simpsons Arcade Game's EEPROM: the test mode's settings), and the logs of the last three runs when the
games were started with nowhere else for them to go (from Explorer, Steam or a launcher, not a terminal):
simpsons.log and simpsons-arcade.log, the runs before as .1.log and .2.log. On Linux and macOS a file
portable.txt beside the games (beside the .app) keeps all of it in this folder instead.

Playing The Simpsons Arcade Game
--------------------------------
The arcade game as it is: four players, each with a coin slot. A coin and the attack button bring a
player in: player 1 is Marge, 2 Homer, 3 Bart, 4 Lisa. The test mode's settings (F2, the cabinet's
test switch) are kept from one game to the next, as the cabinet's EEPROM keeps them.

  Move (player 1)            the arrow keys
  Attack, jump               Z or left Ctrl; X or left Alt
  Coins 1-4                  5, 6, 7, 8
  Test switch                F2 (held: the test mode's menu)
  Full screen                F11 or Alt+Enter
  Pixels                     F3: sharp, smooth, bilinear, CRT
  Quit                       Escape, or a controller's Back and Start held together two seconds

Players 2 to 4 have keys of their own (as MAME's), which controls.ini changes. Game controllers are
players 1 to 4 in the order they are plugged in: the pad or the left stick moves, A attacks, B jumps,
Back puts a coin in. A controller the games do not know, or know wrong, takes its line
(SDL_GameControllerDB's format) in gamecontrollerdb.txt, beside the games or in the folder of what
they keep: its line goes over their own.

Playing The Simpsons Mr. Plow Edition
-------------------------------------
The menu: ARCADE (the arcade game at home: each player picks a character on the game's portraits),
MR. PLOW MODE (FREE PLAY with as many continues as you like, BOSS RUSH, BONUS GAMES, FRANK GRIMES
LIFE), MULTIPLAYER (up to four devices of your local network in one game: one HOSTs a room, the
others JOIN it, the host's START begins it), INTRO (the story before stage 1; it plays by itself
when the menu is left alone half a minute, START skips its scenes), OPTIONS (each player's
CONTROLS; the cabinet's DIFFICULTY, LIVES, DEMO SOUND and SCREEN, and ARCADE's CONTINUES; VIDEO:
the window's size, full screen, the pixels, INTEGER SCALE, WIDE, VSYNC, FPS; the VOLUME; SOUND MONO
or STEREO; kept in simpsons.ini), EXIT. In a game, START brings in another player; Escape, a controller's Back or START
of a player in the game pauses it (CONTINUE, SAVE STATE, LOAD STATE, RESTART, QUIT GAME), but in a
story or a stage's scene, where START skips it. In a MULTIPLAYER game Escape or Back leaves it.

  Move (player 1)            the arrow keys
  Attack, jump               Z or left Ctrl; X or left Alt
  START                      Enter
  In the menus               the arrows move, Enter or attack picks, Escape goes back

Licenses
--------
LICENSE.txt is the project's (BSD-3-Clause). licenses/ holds MAME's and ymfm's (BSD-3-Clause: the
board's video and sound follow their behavior and tables), SDL2's (zlib: the window, the sound and the
controls, inside the games), SDL_GameControllerDB's (zlib: the game controllers' mappings, inside the
games) and Kenney's (CC0: Kenney Mini, the letters of the window that asks for the ROM set; Input
Prompts Pixel, the keys and buttons on The Simpsons' CONTROLS page), and arcade-lan's (BSD-3-Clause:
MULTIPLAYER's rooms and game over the local network, inside The Simpsons Mr. Plow Edition), and the LZMA
SDK's notice (public domain: the reader of a ROM set packed as .7z, inside the games).

No data of the game is in this package: no graphics, sound, music or program byte of the ROM set.
The Simpsons is a trademark of its owner, and the arcade game is Konami's; this project is not
affiliated with or endorsed by either.
