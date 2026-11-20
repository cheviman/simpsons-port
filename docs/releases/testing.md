The Simpsons testing: the next release's changes, for the players to try before it comes out. It is not a release:
it is made again at each change, and something in it may not work yet. Tell what you find in the issues
(github.com/cheviman/simpsons-port/issues), with the version The Simpsons Mr. Plow Edition's OPTIONS shows at the bottom right
(V...-TEST...). As in a release, nothing of the game comes in these packages: keep your own `simpsons.zip` (MAME's
ROM set, 4 Players World) beside the game, and it makes its assets out of it the first time it starts.

**To try**

- **INTRO > ATTRACT** (the players' issue 22): INTRO on the menu has its page now: STORY, the story before stage 1
  as before, and ATTRACT, the arcade's own attract mode with no menu over it (its opening, the title, the demo and
  the high scores, round again), till a button (A, Back or Start) brings the page back. Its sound follows OPTIONS'
  DEMO SOUND, as on the arcade machine.
- **The picture drawn three times faster** (the players' issue 21, the Switch): a frame takes some 0.9 ms to draw on
  a computer where it took 2.8 (WIDE 16:9), with the same pixels, so a slow device has the game at its speed with
  far fewer pictures left out. Not tried on a Switch: tell how it goes there.
- **WIDE, stage 2's end** (Sebas, from a player's video): when the Krusty balloon falls and the floor gives way, the
  hole goes on past the picture's sides as one, where the sides showed pieces of its wall again. The fall off the
  cliff and Homer's fall down the gorge are 4:3, black at both sides, as they have no more picture than the cabinet's.
- **WIDE, the goons at stage 3's and stage 5's starts** (Sebas, from a player's video): they wait past the wider
  picture's edge and walk in, instead of standing in sight at the 4:3 picture's edge. Stage 3's and stage 5's cards
  are centred as the others.
- **Slow devices** (the players' issue 21, the Switch): where a frame takes longer to draw than its time, the game
  and its sound keep their speed and some pictures are left out, instead of all of it slowing down with the sound
  breaking up. Not tried on a Switch: tell how it goes there, docked and handheld.
- **CONTROLS on the Switch** (the players' issue 21): OPTIONS > CONTROLS is there, to choose each player's buttons
  on their own controller: pick the player, a control, and press the button for it (a button of another controller
  leaves it as it was); DEFAULTS puts them back. The Simpsons Arcade Game plays with the same ones. Not tried on a
  Switch: tell whether the page shows the buttons you press by their own letters.
- **BOSS RUSH's card** (the players' issue 18): BOSS RUSH, the boss's number, his stage and his name show on black
  while the game goes to each boss; with VSYNC ON the picture stood still on the character choice until the boss.
- **The story before stage 1** (Sebas): one START skips all of it, straight to stage 1's card, its sound with it
  (the attack and the jump too, once the story has run a second). It skipped one scene at a time, the sound going
  on. INTRO on the menu still skips scene by scene.
- **WIDE, stage 1's opening** (Sebas, on his phone): Smithers' goons running off with Maggie no longer vanish in
  the picture's right side; they run out of the picture, and the scene ends a moment later than in 4:3.
- **WIDE, stage 1's shops** (Sebas, on his phone): the restaurant's cook stands at its door again, no longer at
  the video arcade's in 20:9.
- **WIDE, stage 3's elevator** (the players' issue 5): centred, the same black at both sides, from the platform's
  standing until the play goes on at the shaft's top (it stood to the left: 16 pixels in 16:10, the Steam Deck's,
  up to 72 in 20:9), with no jump as it begins or ends; the players stay on the platform as on the cabinet.
- **WIDE, stage 2's Otto** (the players' issue 16): Otto is at his stall from the stage's start, while the family
  comes down in the balloon, instead of appearing as they drop from it (20:9, and at 16:9's edge). Stage 1's restaurant
  door, with its OPEN sign, is there from the start too.
- **RecompLauncher** (Sebas): the computers' packages carry `recomplauncher.json`, so RecompLauncher can install The
  Simpsons Mr. Plow Edition, ask for your `simpsons.zip`, make the game's assets of it, and show OPTIONS' settings
  on the game's page. A shortcut can name the ROM set too, with `--game PATH`. Without them, nothing changes.
- **Bart's shirt** (the players' issue 14): orange too as he falls into the gorge at stage 5's end and in its water,
  and all of it on the walk to Channel 6; Maggie's onesie stays light blue when Bart picks her up.
- **Barney's shirt** (Sebas): a salmon instead of the cabinet's green (stage 4, at Moe's); his hair stays yellow.
- **INTRO** on the menu (the players' issue 8): the story before stage 1, Smithers taking the diamond and Maggie with
  it; it plays by itself when the menu is left alone half a minute, as an arcade's attract mode does. START skips its
  scenes, Escape or Back leaves it.
- **High-DPI screens on Windows** (the players' issue 12): the game draws at the screen's own pixels where Windows
  scales the desktop (225 % on a 4K screen gave it 1707 x 960, stretched), its windows as large as before.
- **simpsons.7z**: MAME's set packed by 7-Zip works as the .zip does.
- **The Simpsons Arcade Game with a controller**: Back and Start held together two seconds quit it.
- **The logs of the last three runs** are kept (simpsons.log, .1.log, .2.log), on Linux and macOS too; there, a
  portable.txt beside the games keeps everything in their folder. simpsons.ini explains each setting.

The game is free, and stays free. To support the decompilations and the ports, and have a say in what comes
next: [patreon.com/c/ChevimanPorts](https://www.patreon.com/c/ChevimanPorts). Entirely optional.

The Simpsons is a trademark of its owner, and the arcade game is Konami's; this project is not affiliated with or
endorsed by either, nor by Apple.
