The Simpsons testing: the next release's changes, for the players to try before it comes out. It is not a release:
it is made again at each change, and something in it may not work yet. Tell what you find in the issues
(github.com/cheviman/simpsons-port/issues), with the version The Simpsons Mr. Plow Edition's OPTIONS shows at the bottom right
(V...-TEST...). As in a release, nothing of the game comes in these packages: keep your own `simpsons.zip` (MAME's
ROM set, 4 Players World) beside the game, and it makes its assets out of it the first time it starts.

**WIDE, stage 1's police car** (issues 32 and 38): the police car parked by the fire station never came. WIDE brings
each thing in a little earlier than the cabinet, and the game had no room left for the car then (WIDE's sides keep more
of the stage's figures in sight), so it was dropped. It now waits for room, at the latest till the moment the cabinet
brings it in; it may come in at the very right edge of the screen.

**WIDE, stage 2's mailbox in the balloon landing** (issues 32 and 38): it now stands by Otto's stall while the family
comes down in the balloon, instead of appearing as they land.

**WIDE, the camera stops** (issue 38): in every stage the camera now stops where the cabinet's does. In WIDE it stopped
earlier, by the width of WIDE's side (stage 7's first fight, for one), and skipped a stop or two.

**WIDE, the cemetery's closing** (issue 38): the goons and the ghosts no longer come in near the middle of the screen:
they come in where the cabinet has them, at its own left edge (on the arcade its black border hides them as they
appear).

**WIDE, the waterfall and the dream** (issue 38): the fall over the waterfall at the end of stage 5, and the family on
the grass by the lake, falling asleep into Dreamland and waking up from it, are pictures 4:3 with black sides, as the
other scenes.

**WIDE, stage 7's closing** (issue 38): from the boss's fall to the flight to the power plant, the scene is a picture
4:3 with black sides. What WIDE showed beside it was the arcade's own map: the doors' frame as a black line, the map's
end in black, the helicopter drawn as it comes in, as on the cabinet's edge.

**WIDE, stage 2: the fight with Krusty's balloon** (issue 38): the left side is now plain sky, with the trees, the
bushes, the rail and the deck going on, and no pieces of the Ferris wheel, the clouds or the roller coaster. It comes in
from the screen's left edge as the camera reaches the fight, while the fun house goes out, instead of all at once.

**The hat throwers** (issue 38): the hat still on their heads as they throw and catch it is the arcade's own: the
cabinet draws the same pictures.

**ORIGINAL COLORS** (issue 33): a new line in The Simpsons Mr. Plow Edition's OPTIONS, after CONTINUES. ON shows the
arcade's own colours (Bart's light blue shirt) instead of the show's orange; it changes at once and is kept in
simpsons.ini (bart_shirt=arcade).

Not tried on a device: everything above was checked on the PC (the console's headless); the WIDE changes are for the
players to confirm.

**OPTIONS, shorter**: SCREEN is now in OPTIONS > VIDEO, and a new OPTIONS > SOUND page holds DEMO SOUND, VOLUME and
SOUND. The lines that open a page (CONTROLS, VIDEO, SOUND) show a > at their right. SOUND starts in STEREO every time
the game starts (MONO can still be chosen, till the game closes).
