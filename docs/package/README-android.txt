The Simpsons Arcade Game and The Simpsons Mr. Plow Edition for Android
======================================================================

Konami's The Simpsons (1991), the arcade game for four players, as a native game: decompiled to C and checked
frame by frame against MAME, github.com/cheviman/simpsons-port. The Simpsons Arcade Game is the arcade game as
it is; The Simpsons Mr. Plow Edition, the console version (menus of its own, no coins: START brings a player
in). This is Android's package:
{ready}the games without their assets, ready to install, and the tool that makes your own APKs of them on
{ready}your computer.
{no-ready}the tool that makes your own APKs of the games on your computer.
Nothing of the game comes in this package: the games take their graphics and sounds out of your own ROM
set.

What you need
-------------
- simpsons.zip, MAME's ROM set of The Simpsons (4 Players World, set 1), as MAME has it;
- a phone, tablet or TV with Android 7 or later.

{ready}The APKs in ready/
{ready}------------------
{ready}Install ready/The Simpsons Mr. Plow Edition.apk and ready/The Simpsons Arcade Game.apk on the device (open
{ready}them there; Android asks once to allow apps from that source). Open a game once: it says it has no ROM set and
{ready}makes its folder on the device's storage, Android/data/io.github.cheviman.simpsons/files
{ready}(The Simpsons Arcade Game's: Android/data/io.github.cheviman.simpsons.arcade/files). Copy simpsons.zip into
{ready}that folder, from a computer with the device on USB (file transfer: most phones' own file managers do not open
{ready}Android/data), and open the game again: the first start takes the assets out of simpsons.zip there (some
{ready}seconds). Each game its own copy. These APKs are signed with the project's key: a new release installs over
{ready}them, but not over an APK you made with the tool (another key: uninstall that one first).
{ready}
Your own APKs, with the tool
----------------------------
Start the tool for your computer, with a double click:

  Windows   Simpsons APK.exe
  macOS     Simpsons APK (macOS)    (the first time: right click, Open)
  Linux     Simpsons APK (Linux)

Leave it where it is, beside the folder android (the games' Android build) and font.bmp. Drop simpsons.zip on
its window (or put it beside the tool), a square picture of yours for the launcher's icon and a 16:9 one for a
TV's banner if you want them. It writes The Simpsons Mr. Plow Edition.apk and The Simpsons Arcade Game.apk
with the assets in them, signed with a key of yours kept in your user folder (keep it: only an APK with the
same key installs over the last one). Copy them to the device and open them there.

{ready}What either way makes from your ROM set is yours alone: never share it.
{no-ready}What the tool makes from your ROM set is yours alone: never share it.

Playing
-------
The touch screen has a stick where your left thumb lands, ATTACK and JUMP at the right, and START (COIN on
The Simpsons Arcade Game, where ATTACK then brings Marge in); a game controller hides them, and each
controller is a player (player 1 is Marge, 2 Homer, 3 Bart, 4 Lisa). Back, or START in the game, pauses
The Simpsons Mr. Plow Edition, and so does leaving the app. Its OPTIONS > CONTROLS changes each player's
buttons: choose the player, then a control, and press the button for it on that player's own controller
(Back leaves it as it was; DEFAULTS puts them all back). On a TV with Android TV or Google TV the games are
in the TV's apps; its remote is a player after the game controllers (in The Simpsons its pad moves, OK
attacks, back jumps, home pauses). Each game writes its log (simpsons.log, simpsons-arcade.log) in its folder
on the device's storage, Android/data/io.github.cheviman.simpsons/files (The Simpsons Arcade Game's:
Android/data/io.github.cheviman.simpsons.arcade/files).

Licenses
--------
LICENSE.txt is the project's (BSD-3-Clause). licenses/ holds MAME's and ymfm's (the board's video and
sound follow their behavior and tables), SDL2's and SDL_GameControllerDB's (zlib) and Kenney's (CC0: the
letters, the controls' pictures).

The Simpsons is a trademark of its owner, and the arcade game is Konami's; Android is a trademark of
Google. This project is not affiliated with or endorsed by them.
