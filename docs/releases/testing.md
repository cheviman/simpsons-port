The Simpsons testing: the next release's changes, for the players to try before it comes out. It is not a release:
it is made again at each change, and something in it may not work yet. Tell what you find in the issues
(github.com/cheviman/simpsons-port/issues), with the version The Simpsons Mr. Plow Edition's OPTIONS shows at the bottom right
(V...-TEST...). As in a release, nothing of the game comes in these packages: keep your own `simpsons.zip` (MAME's
ROM set, 4 Players World) beside the game, and it makes its assets out of it the first time it starts.

- **WIDE, stage 2's mailbox by Otto's stall** (the players' issue 29): seen again before it is hit. It was put in past
  the wider side as the stage began and kept that place's priority when it came back to its own, so it stood behind
  the background: unseen, yet hiding the players and goons that walked behind it.
- **WIDE, Lisa's box at the top closed at its right** (the players' issue 29): the players' boxes each have their white
  side at the left, and the fourth's right one fell under the cabinet's black edge, so the game never drew it; in WIDE's
  play it is drawn now, as the others are.
- **WIDE, stage 2's mailbox as the play begins** (the players' issue 29): it waited for the board's right side to put
  itself up, so in WIDE's wider right side it stayed unseen, with no floor raised under it, till the scroll brought
  it in. It now puts itself up once in WIDE's sight. (During the walk-in before the play the game has not set it up
  yet, on the cabinet too, where it is out of sight.)
- **WIDE, stage 2's fat goons holding nothing** (the players' issue 29): coming in from the left past WIDE's left
  side, a fat goon picks up the obstacle he carries on his head some pixels past WIDE's left limit, where the cabinet
  has it right at its own; it was taken off at once, and he walked in holding nothing. It is now kept while he holds it.
  (The first try of this covered the wrong check: the obstacle has a check of its own, at the cabinet's sides, which
  now goes by WIDE's. Its place freed, the next object took it, stage 2's mailbox by the cages, and the goon carried
  that one to his head and threw it: the mailbox flying across from the top. Gone with it.)
- **WIDE, stage 8: Smithers stuck off the screen** (the players' issue 31): his fight's left wall is at the wider
  screen's left side, a column the cabinet never draws, so on the cabinet he runs well past it and back. WIDE had put
  the wall's floor cells there, and a hit or a jump could leave him inside them, turning at each frame off the screen.
  In his fight the floor map is now the cabinet's, cell for cell.
- **Switch, the .nro from a forwarder** (the players' issue 30): the two .nro are now `simpsons.nro` (Mr. Plow
  Edition) and `simpsons-arcade.nro` (Arcade Game), with no space in their names. A forwarder (Sphaira's) hands the
  .nro's path to the Homebrew loader, which cut it at the first space and stopped with error 2002-6005. Replace the old
  `The Simpsons ... .nro` files on the SD card with these, and make the forwarder again.
- **WIDE, the screen's edges in every stage** (the players' issue 29, reported again and again: things that vanished or
  never showed at WIDE's sides): the game decides when an object leaves the screen with numbers written in each of
  its routines, the cabinet's edges; WIDE moved them one routine at a time. They are now named in one place, and WIDE
  moves them all by its sides, left and right apart (the cemetery's enemies come in from the right). Among those it
  moved now: stage 4's figures behind the bar, which vanished in the left side, the stages' props and posters, doors,
  birds, Sideshow Bob, the woman in the apron, Marge's cloud. A check that plays every stage in 16:9, 16:10 and 20:9
  finds nothing vanishing at WIDE's sides now but what vanishes in the cabinet's part too (enemies beaten there,
  obstacles breaking).
- **WIDE, stage 2: the fight with Krusty's balloon** (the players' issue 29): the camera stops at the map's right end
  for it, so all of WIDE's sides are at the left, over map the cabinet never shows in the fight: the fun house's white
  wall with its green porthole beside the deck, and, once the deck gives way, its broken floor in black and lines.
  Through the whole fight, wherever the camera is in it (a player walking left brings it back some way), that side now
  goes on with the deck, its rail, the trees and the sky the cabinet shows, each row repeated leftwards from the first
  column the cabinet can show there, and the broken deck the same way. In 16:9, 16:10 and 20:9.
- **The menu without INTRO, the game showing itself when left alone**: INTRO and its page (STORY, ATTRACT) are gone
  from Mr. Plow Edition's menu. Left alone 30 seconds, the menu plays the story before stage 1, and once the story is
  over the arcade's own attract mode, its characters and its demo, as the cabinet does; any button, a direction too,
  brings the menu back.
- **The Konami code kept on the menu**: entered once, it puts a KONAMI CODE line on the menu, after OPTIONS, which
  stays there from then on (`konami_code=1` in simpsons.ini) and opens the code's page.

The game is free, and stays free. To support the decompilations and the ports, and have a say in what comes
next: [patreon.com/c/ChevimanPorts](https://www.patreon.com/c/ChevimanPorts). Entirely optional.

The Simpsons is a trademark of its owner, and the arcade game is Konami's; this project is not affiliated with or
endorsed by either, nor by Apple.
