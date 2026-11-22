The Simpsons Arcade Game and The Simpsons Mr. Plow Edition for the Nintendo Switch
==================================================================================

Konami's The Simpsons (1991), the arcade game for four players, as a native game: decompiled to C and checked
frame by frame against MAME, github.com/cheviman/simpsons-port. The Simpsons Arcade Game is the arcade game as
it is; The Simpsons Mr. Plow Edition, the console version (menus of its own, no coins: START brings a player
in). This is the Switch's package: the games without their assets, ready for the Homebrew Menu, and the tool
that makes your .nsp of them on your computer. Nothing of the game comes in this package: the games take their
graphics and sounds out of your own ROM set.

What you need
-------------
- simpsons.zip, MAME's ROM set of The Simpsons (4 Players World, set 1), as MAME has it;
- a Switch with Atmosphere.

From the Homebrew Menu (the .nro)
---------------------------------
Copy what is in sd/ to the root of your SD card: switch/simpsons-arcade/simpsons.nro (The Simpsons
Mr. Plow Edition) and switch/simpsons-arcade/simpsons-arcade.nro (The Simpsons Arcade Game). Put simpsons.zip in switch/simpsons-arcade/ beside them,
and start either game from the Homebrew Menu. The first start takes the assets out of simpsons.zip into one
file, switch/simpsons-arcade/simpsons.assets (some seconds, the screen black); both games start from it from
then on. Without simpsons.zip the game says where to put it (press + to leave).

On the HOME menu (the .nsp)
---------------------------
Start the tool for your computer, with a double click:

  Windows   Simpsons NSP.exe
  macOS     Simpsons NSP (macOS)    (the first time: right click, Open)
  Linux     Simpsons NSP (Linux)

Leave it where it is, beside the folder switch (the games' Switch build) and font.bmp. Drop simpsons.zip and
your console's prod.keys on its window (or put them beside the tool), and a 256 x 256 JPEG of yours if you
want your own HOME menu icon. It writes The Simpsons Mr. Plow Edition.nsp and The Simpsons Arcade Game.nsp
with the assets in them; install them with an installer (DBI, Tinfoil, Goldleaf) on a console with Atmosphere
and signature patches.

What either way makes from your ROM set is yours alone: never share it.

Playing
-------
Each controller is a player, up to four (player 1 is Marge, 2 Homer, 3 Bart, 4 Lisa): the stick or the d-pad
moves, B (the lower button) attacks, A (the right one) jumps, + is START (and pauses The Simpsons Mr. Plow
Edition); on The Simpsons Arcade Game, minus puts a coin in, and a coin and the attack bring the player in.
HOME leaves the game. The Simpsons Mr. Plow Edition's OPTIONS > CONTROLS changes each player's buttons: choose
the player, then a control, and press the button for it on that player's own controller (DEFAULTS puts them
back); The Simpsons Arcade Game plays with the same ones. The games keep their settings, the controls
(controls.ini) and their logs (simpsons.log, simpsons-arcade.log) on the SD card, in switch/simpsons-arcade/.

From 1.0.0: 1.0.0 used switch/simpsons/, which The Simpsons: Hit & Run's port uses too. At their first start
the games copy their own files from there (simpsons.zip, simpsons.assets, the settings, the high scores, the
saved state, the controls) into switch/simpsons-arcade/ and leave the old folder as it is: take 1.0.0's two
.nro out of it yourself (a ROM set kept there as the folder rom has to be moved by hand).

Licenses
--------
LICENSE.txt is the project's (BSD-3-Clause). licenses/ holds MAME's and ymfm's (the board's video and
sound follow their behavior and tables), SDL2's and SDL_GameControllerDB's (zlib) and Kenney's (CC0: the
letters, the controls' pictures).

The Simpsons is a trademark of its owner, and the arcade game is Konami's; Nintendo Switch is a trademark
of Nintendo. This project is not affiliated with or endorsed by them.
