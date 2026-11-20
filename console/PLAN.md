# The console build: what it adds, and in what order

The port builds two games from the same decompiled program, as tmnt-port, outrunners-port and segasonic-port do:

- **The Simpsons Arcade**: the decompiled game as it is, the same frame by frame as the reference. It comes first
  (ports/PLAN.md).
- **The Simpsons**: the console version, the arcade game made for playing at home. The game's functions
  console/replaced.txt lists get the console's own versions (console/*.c); their definitions in src/ are written as
  `CONSOLE_ORIGINAL(name)`, so the arcade build runs the same program (konami_cpu/decompile.py has to read
  console/replaced.txt first: ports/PLAN.md, A1).

What the decompilation already knows is said with each item; what is left to work out, too. tmnt-port's console/ is
the model (console.c, menu.c, draw.c, presentation.c, wide.c, modes.c, lan.c): read it, never run or change it from
here.

## The guidelines (proposed 13-11-2026, from TMNT's: Sebas confirms, changes or drops each)

### What every console version has (tmnt-port's, and the players' reports on it)

1. **The names and the menu.** The executables "The Simpsons Arcade" and "The Simpsons" (TMNT Arcade and TMNT). The
   menu: ARCADE (the arcade game at home), the modes of our own under a heading of their own (TMNT's SPLINTER WAY,
   OutRunners' & KNUCKLES), OPTIONS, EXIT. The heading for now: MR. PLOW MODE (Sebas, 13-11-2026: "le ponemos de
   titulo mr plow mode, y dps lo cambiamos").
2. **No coins: PRESS START.** A player joins with START: the console puts the slot's coin and its attack in, as the
   cabinet's players do (the factory settings have no start buttons: a coin and the attack bring a player in). Where
   a player's box shows INSERT COIN (`game_texts` of bank 12, printed by `print_game_text`), PRESS START. Check: a
   control script with START alone reaches stage 1 for one, two and four players.
3. **Who plays whom** (confirmed, Sebas, 13-11-2026: "en la version consola, quiero que se pueda cambiar los
   personajes"). In this set each slot is a character of its own (1 Marge, 2 Homer, 3 Bart, 4 Lisa): one player
   alone is always Marge. As TMNT's (its guideline 5), the console puts the player's controls on the slot of the one
   chosen, and with more players each chooses one of those left; the program stays as it is. (MAME's two-player sets
   let the players choose, but theirs are other program ROMs, not this set's.) The character chosen again after a
   continue, as TMNT's turtle change (07-11-2026). One character a player, none twice (Sebas, 13-11-2026: "los
   personajes, 1 sin repetir"); with the Konami code, a character may be chosen twice (9). Check: one controller choosing
   Bart: slot 3 plays, the others stay out; after a continue, Lisa: slot 4 plays from where the game goes on.
4. **The pause**: START of a player in the game pauses (another's brings him in), CONTINUE, RESTART, QUIT; by each
   player's controls as the menus read them, by touch and by back on the devices; it waits for the frame as the game
   does (never a sleep of its own: TMNT's pause stood still with VSYNC). Check: the console's headless with `--actions`
   through the pause, and VSYNC on in the window.
5. **OPTIONS** as tmnt-port's: CONTROLS (each player's keys, buttons and triggers), VIDEO (WINDOW, FULL SCREEN, PIXELS
   sharp, smooth, bilinear or CRT, INTEGER SCALE, WIDE, VSYNC, PRESS START's blink, FPS), VOLUME, and the cabinet's
   settings by the test mode's names: DIFFICULTY, LIVES, DEMO SOUND, SCREEN (flipped); not the coinage. The game keeps
   them in the EEPROM's copy (59F1, taken into `difficulty_setting` 4814, `lives_per_credit` 485A... by
   `apply_settings`): the console writes them there before the game starts. In simpsons.ini; the enums only grow at
   the end. OPTIONS shows the game's version, as TMNT's (Sebas, 13-11-2026: "obvio tmb el @cheviman, la version en
   options"): `PROJECT_VERSION` and the testing builds' suffix (`SIMPSONS_VERSION_SUFFIX`, -TEST and the CI run's
   number). Check: each setting as the test mode's COIN/GAME OPTIONS page shows it, and in the game (the lives in a
   player's box).
6. **The Konami presentation** (confirmed, Sebas, 13-11-2026: "el logo de intro hagamos con el de las tortugas,
   buscamos un konami y que le pasen todos caminando"), as tmnt-port's console/presentation.c: before the title at
   power-on, a Konami logo of the game's own, scaled up and dark on black, until the family walks across it and lights
   it up. What is left to find: the logo among the game's graphics (TMNT's was a game text of the fixed layer, the
   ribbon and KONAMI 16 pixels high, that no screen showed; here bank 12's `game_texts` and the fixed layer's
   characters first, then the sprites), drawn from the player's assets. The family: Marge, Homer, Bart and Lisa as the
   game's own objects on their slots, each with its own walk, drawn and stepped on by the game's sprite engine, as
   TMNT's turtles (Maggie too if the game has her walking or carried). The logo: a large Konami one if the game has it,
   else the best one found (Sebas: "si hay un konami grande, usamos ese, sino bueno, busquemos"). Its sound: one of
   Bart's voices (Sebas: "el sonido usemos uno de bart"), which one to choose by ear among the K053260's samples the
   game plays when Bart speaks (the sound commands his objects send).
   Check: the console's headless frames through it, START skipping it.
7. **The menu's screen** (confirmed, Sebas, 13-11-2026: "para el menu, usemos la tele, que tiene de titulos para
   presentarlo"): the TV the game's title and attract show (the family on the couch), with the menu presented on its
   screen; the title's logo and copyright line, and "@cheviman" under them (the port's signature, as
   outrunners-port's and TMNT's). What is left to find: the scene and its layers and sprites (the attract's frames,
   its map scene number), and how the menu's lines fit the TV's screen in each WIDE. Its music to choose among the
   songs by name (`attract`, `ending`...: tools/sound_music.py's SONGS).
8. **WIDE** (confirmed, Sebas, 13-11-2026): 16:10 (the Steam Deck), 16:9 and 20:9 beside the arcade's 4:3. The K052109's maps are 512 pixels wide
   and the sprites can be drawn past 320; what the game keeps up to date beyond its 320 columns (the layers' columns
   as the scroll goes, the objects and the spawns it leaves off screen, the players' limits against the screen's
   edges) is what is left to work out, as tmnt-port's console/wide.c did. Also in the pause, the menus, the continue
   and every scene over the game, and the players' boxes in the middle (the fixed layer's band). The console's
   biggest job. Check: the headless's frames in each WIDE at each stage's start, middle and boss (the 25 scenarios'
   frames), and the scenes between them.
9. **The Konami code in the menu** (confirmed, Sebas, 13-11-2026: "tmb konami code") (up, up, down, down, left,
   right, left, right, B, A: JUMP and ATTACK): a stage select (a step starts as the scenarios start it: `step_phase`
   and its two bytes 0, `game_step` 4803 the step, from stage 1 under way), HEALTH INFINITE, unlimited lives and
   continues; and the characters chosen with repeats (Sebas: "y ahi dejamos repetir"): two or more players the same
   character, each slot playing the chosen one's moves with its looks, as TMNT's SAME TURTLE (its console/same_turtle.c:
   the chosen one's tables and palettes on every slot). What a character is to the program is what is left to work out
   (each character's object kind, its animation set, its records: `player_variable_records`, E819, four of 27 bytes,
   and its palettes).
10. **Lives and continues in our own modes**: fixed (TMNT's: 5 lives, 3 continues); unlimited with the code. The
    decompilation knows the lives (49D1 + 8 x the player), the continue's countdown (4A1C, BCD) and the continue's
    phase (`game_continue_phase`, `continue_screen`).
11. **MULTIPLAYER over the LAN, for the four** (arcade-lan as a submodule in lan/, lockstep: one board with four
    players, as TMNT): each device its players, the host's frames' inputs for all, the state's hash per frame to tell
    a device that went apart. Check: two headless `--real-time` through a stage; a client gone; the host gone; the
    library's own tests.

### What this game could add (to weigh)

12. **The high scores kept.** The table is in the work RAM alone (the EEPROM holds its test pattern and the
    settings), lost at power-off on the cabinet: the console keeps it with its settings and puts it back after the boot
    sets the table (`insert_high_score`, 079743, and what the boot writes). Check: a high score, the game quit and
    started again: it is in the table; The Simpsons Arcade as the cabinet.
13. **BOSS RUSH**: the bosses one after another (src/bosses/: the wrestler, the Krusty balloon, stage 3's two, the
    football player, the bear, the Dreamland boss, Channel 6's, stage 8's and Mr. Burns' robot), each from its fight's
    place: how a stage starts at its boss (the scroll, the spawns before it) is what is left to work out, as TMNT's
    crossing did.
14. **BONUS GAMES**: the two bonus stages (step 3, blow up the balloon, src/bonus/balloon.c; step 12, wake up,
    src/bonus/wake_up.c) as a mode of their own from the menu, for one to four players, with their results screen.
15. **STEREO**: the board wires only the YM2151's left output, so the music's FM channels the program pans right
    alone (command EF 02) are not heard on the cabinet, nor in MAME; the MIDI files already pan as the program asks.
    An option, SOUND: MONO (the board's, the default) or STEREO (the YM2151's two outputs as the program pans them), to
    be listened to first. Check: MONO's sound hashes the same as The Simpsons Arcade's; a song with right-panned
    channels has them in STEREO's WAV.
16. **FRANK GRIMES LIFE** (confirmed, Sebas, 13-11-2026: "el modo ronin seria 'frank grimes life'"), TMNT's RONIN:
    one player against what four would have. The game scales by the players in
    play (`rank_by_players` 041786 and the bosses' `..._by_players` tables): which counts to give four players' with
    one is what is left to work out.
17. **Save states** from the pause (TMNT's guideline 15, still to do there too): the whole board's state the
    scenarios already compare, with the program's globals (`leftover_y`, the bank) and the runtime's.
18. **Bart's shirt in orange** (confirmed, Sebas, 13-11-2026: "vamos a cambiarle el color de la remera a bart por la
    naranja"): in The Simpsons, Bart's shirt in the show's orange; The Simpsons Arcade as the board has it. A palette
    swap, as TMNT's ORIGINAL TURTLES' bandanas: the colours of Bart's shirt in the palettes his sprites use (the sets the
    game loads for slot 3: the palette sets E9CB/EAEB, bank 2D's palettes; the sprites' from 32 on), changed where the
    console sees them loaded, nothing else of the palette. What is left to work out: which palette and which of its
    colours are the shirt, in every scene (the stages, the bonus stages, the stories, the ending, the attract's
    profile). Whether OPTIONS gets a line to have the arcade's colour back: to ask. Check: the headless's frames with
    Bart in each stage and scene: his shirt orange, every other colour of the frame the arcade's.
19. **The region**, to look at: this set is the World 4 Players one, and the other sets differ in their program ROMs
    (17c, 13c, 15c); an option only if this program chooses its texts by a byte, as TMNT's `version` did.

### The players' requests (to do; 14-11-2026, from 1.0's thread on r/decomps)

20. **RUN-AHEAD** (OFF, 1 or 2 frames), in The Simpsons alone: at each frame the whole game's state kept in memory at
    the main loop's wait (game/state.h: `game_state_possible`, any frame after the boot), the next frames run with this
    frame's controls, without their sound or their picture but the last one's, which is shown, and the state put back.
    Off in MULTIPLAYER, where the lockstep decides the frames. What is left to find: the frames the game itself takes
    between a button and the picture that answers it (the sprite DMA's among them), the state's size and time a frame
    in memory rather than in a file, and the sound kept from the real frames alone. Check: the headless's `--actions`
    with RUN-AHEAD 1 and 2: the same pictures 1 and 2 frames earlier than without it; The Simpsons Arcade 25/25.
21. **The players' boxes at the screen's edges** in WIDE, beside the middle (TMNT's players asked for the middle, so
    both, in OPTIONS > VIDEO): the fixed layer's band (the four boxes) cut by player, the first two at the left edge
    and the other two at the right. Check: the headless's frames in each WIDE with one to four players, the boxes whole
    and nothing of the fixed layer's sides between them.
22. **A window across two screens**: borderless, as large as the displays side by side (a 32:9 setup), beside FULL
    SCREEN, which covers one display (`SDL_WINDOW_FULLSCREEN_DESKTOP`). Windows, Linux and macOS. Check: the window's
    size from SDL's display bounds; on two real screens only by a player, and the release's notes say so.
23. **WIDE 32:9** (about 796 columns at the board's 224 lines): past the K052109's maps, rings of 512 pixels (20:9 shows
    496 of them), so the sides would be drawn from the stage's maps outside the chip. After the chip-agnostic port, the
    goal of every port after the CPU-agnostic C (Sebas, 14-11-2026: "es la idea de llegar a eso").
24. **What the game does not use**, each looked at before Sebas decides, kept only if it adds something to the game:
    the 22 sheets of tiles no scene shows (the decomp's assets, `unused_XXXXX`), the 9 sheets of sprite cells nothing
    shows, the animation set nothing sets up (`kind_AD`), the four FM effects no command reaches
    (`unused_effect_group`), and `unused_dreamland_spawn_list` (67 spawns no table leads to: a cut layout?). Check: each
    drawn or played in the headless, its frames seen.
25. **New stages**: only as a complement to the game's play, never a new skin over it; the player who asked offered
    their ideas for them (a GitHub issue). Waiting for them.
26. **Scaling filters** (xBRZ, HQ2X, HQ3X, HQ4X) beside SHARP, SMOOTH, BILINEAR and CRT: once the decomp is at 100 %,
    with the picture drawn on the GPU (Sebas, 14-11-2026: "cuando lleguemos al 100% del decomp vamos a mejorar el
    motor gráfico para que sea por hardware y agregar esas cosas en todos los que vengo haciendo"), in every port.

## The order (proposed)

The build first (console/replaced.txt, `CONSOLE_ORIGINAL`, `simpsons_console`, `simpsons_console_headless`), then 1,
2, 3, 4, 5, 7 (the console playable at home), then 8 (WIDE), 9 and 10, 11 (MULTIPLAYER), and the additions Sebas
keeps from 12 to 19. Every change the console makes is checked with the console's headless (`--actions`,
`--video-dump`), and The Simpsons Arcade stays 25/25 after each. After 1.0, the players' requests, 20 to 26.

## What was done

- **13-11-2026**: this plan (Sebas: "un checklist de todo lo que hay que migrar basico, plataformas, y lo que le
  vamos a agregar").
- **13-11-2026, the skeleton** (an agent; its frames seen by me): console/console.h, console_internal.h, console.c,
  menu.c, draw.c from tmnt-port's generic parts; console/replaced.txt with `attract` and `game_mode_frame` (the
  decompiler's CONSOLE_ORIGINAL for Konami: of the whole C only those two definitions and their declarations change);
  "The Simpsons.exe". Points 1 (the menu ARCADE, MR. PLOW MODE with COMING SOON, OPTIONS, EXIT, written with the
  game's own letters over the attract, darkened), 4 (the pause: CONTINUE, RESTART, QUIT GAME; START of a player in the
  game pauses, another's brings him in) and 5 (OPTIONS: CONTROLS with the prompts' pictures, VIDEO, VOLUME, the
  cabinet's settings: DIFFICULTY to VERY DIFFICULT and LIVES to 3 leave the EEPROM's 6C-6F 03 02 01 00, sum 1A1B,
  59F4-59F7 the same, difficulty_setting 3, lives_per_credit 3, and the game starts Marge with 3 lives; the version
  V0.1.0) work; point 2's START joins (the slot's coin and attack), its PRESS START text still to do.
- **13-11-2026, points 2, 3, 6 and 7** (an agent; the frames under build/console_work/, seen by it):
  - **2, PRESS START**: the players' boxes are gauge_draw_player's pictures: 3 INSERT COIN (tiles C7-CF over D7-DF),
    5 PRESS START (E9-EF EA EB over F9-FF FA FB), which the cabinet shows where a slot's credit waits for its player.
    console/gauges.c (`gauge_draw_picture` in console/replaced.txt) draws the game's own PRESS START in place of INSERT
    COIN, in the box's colour.
  - **3, who plays whom** (console/characters.c): this EEPROM gives each slot credits of its own (separate_coin_slots),
    so a slot's credit brings its character in on the portraits. ARCADE starts the game with no credit in (the attract's
    own start, a credit lent to slot 1 for that frame and taken back) and the game's portraits are the choice: the one
    who picked ARCADE is in, the others come in with START; LEFT and RIGHT among the characters no other player picked,
    attack or START picks: the slot's coin, the portrait comes alive with its voice; once everyone in has picked (and 80
    frames for the last portrait), each one's attack brings him in. In a game, the START of a player whose character
    does not play opens "1P < BART >" over the game (his own first, the free ones with LEFT and RIGHT, attack brings him
    in with a coin and the attack, jump closes it): after a continue, or a new player. RESTART picks again at once.
  - **6, the presentation** (console/presentation.c): the game's biggest Konami logo, the one the attract's last screen
    shows on its TV (layer A's map of scene 0, area 3, palette set 12's load 1), drawn by the game on black, hidden
    right of the family until they pass; Homer, Marge with Maggie in her arms (the maggie set, kind 11, as story 1
    walks her in), Bart and Lisa walk across it as story 1's family walks (their animations, the sprite engine);
    then one of Bart's voices, and the fade. START or attack skips it. Bart's voices (commands 3F NN, Bart's kind is 2:
    each family's base + 2), as WAVs in build/console_work/bart_voices/ for Sebas to choose by ear: 3F02 sample 18
    0.68 s (flung back, squashed), 3F06 sample 22 1.37 s (stunned, blow 04), 3F0A sample 26 0.67 s (story 1's
    MAGGIE!), 3F0E sample 30 1.61 s (eating), 3F12 sample 34 0.90 s (standing still too long), 3F16 sample 38 0.78 s (a
    story's line), 3F1A sample 2 0.40 s (his portrait at a game's start), 3F22 sample 6 0.29 s (attack shout), 3F26
    sample 10 0.38 s (hurt), 3F2A sample 14 1.40 s (stunned, blow 03; stage 2's end). **Sebas chose 3F12** (13-11-2026:
    "3f12"); the first try had 3F1A.
  - **7, the menu's screen** (console/tv.c, menu.c): the TV of the attract's last screen and the score ranking (layer
    B's map of scene 2, area 0, palette set 1's load 2), its screen black as the stage cards have it, the title's logo
    on it (the title's object, kind 0E, at 0.57 of its size by the K053247's zoom, set 7's sprite colours), the menu's
    lines under it, then the title's Konami copyright line "(C) 1991 KONAMI" and "@cheviman"; the other pages over the
    TV without the logo. The menus' song is the attract's (song 80, `attract`), asked for again when it ends; others
    that would fit: 8B `ending` (the family together), 96 `area_clear` (the family walks on), 91 `game_start`.
- **13-11-2026, 8, WIDE** (an agent; its frames seen by it, contact sheets under build/wide_work/): OPTIONS > VIDEO >
  WIDE OFF, 16:10, 16:9, 20:9 (the enum OFF, 16:9, 20:9, 16:10 as tmnt-port's; simpsons.ini's `wide=` by name): 32, 48
  or 88 columns more at each side, the window 358, 398 or 496 wide (the frames' middle). The Simpsons Arcade has none.
  - The video (runtime/src/video.c, the port's part of it: `simpsons_video_set_wide`, `_set_layer_window`,
    `_set_fixed`, `_wide_frame`): the layers' rings and the sprites (the K053247's x has 10 bits) drawn past the board's
    320, nothing of it unless the console sets it. A side may be up to 32 less than none, the other the more.
  - The cabinet showed 288 of its 320 columns: the fixed layer covers the first and last 16 with black tiles (3F, its
    ring's columns 12-13 and 50-51), and its ring's other columns hold tile 00 (a pattern of circles). With WIDE the
    fixed layer is drawn on the board's 16-303 alone; layers A and B on all of it.
  - A stage's play (console/wide.c): the layers' maps (the scene's and area's: map_scene, map_area; strips of 256
    pixels, rows of 16 bytes, blocks of 32 pixels and 18 bytes) read as find_layer_column and draw_map_column read
    them. The game draws a map's column 352 pixels past the screen's left edge as its scroll goes (camera_scroll_layers'
    0160) and a whole layer 352 from it (draw_layer_columns, now in console/replaced.txt with
    draw_layer_columns_with_floor_map: what each layer's ring holds from then); at each frame's end the console draws
    the columns the wider screen shows that the ring does not hold, with the floor map's cells for the camera's layer
    (the players and the enemies walk the sides as the board). Its account of the ring: the game's new column (and the
    one 512 pixels before it gone, also when the game draws it a frame late), a row the game draws as its 12 blocks,
    the board's black edges as columns the game may leave as they were, a map no wider than the ring (stage 6's sky,
    512) drawn in it whole, and where the game draws no rows the columns drawn at the y of the rows it holds (stage
    6's sky scrolls up and down round its ring). Never past a map's ends nor past where the stage's record stops the
    camera (stage 1's map goes on in a wall of blue bricks past its boss; stages 2, 5 and 7's go on past their stops
    too): the rest at the other side, the picture panning there 4 pixels a frame; at a stop the picture ends where the
    cabinet's black edge was. The players' boxes (the whole fixed layer) in the middle of the picture. The continue's
    countdown and GAME OVER over the play keep its sides.
  - The objects (console/replaced.txt, the same C with their limits moved by the sides): spawn_stage_objects (a spawn
    due as far past the wider screen's right side as past the board's, put in at its place; those coming from the left
    past its left side), enemy_remove_off_screen, remove_off_screen, stop_beyond_screen_edge, enemy_on_screen,
    player_check_motion, player_check_screen_edges (the players walk to the same distance from the picture's edges).
  - The other screens are pictures of the board's width, their sides their edges' colour where an edge is all of one
    (the menus' TV and the stage cards' TV: orange, lilac), else black: the portraits, the stories, the bonus stages,
    the name entry, the ending, and stage 3's elevator from the platform's standing (its map black past the 288; the
    game draws its rows there with offsets of their own; 15-11-2026, centred, the platform's limits the cabinet's). The
    presentation keeps the board's 320 on black. The pause greys the whole wider frame.
    OPTIONS' version at the window's right edge.
  - Checked (the console's executable, `--arcade-start` with tests/controls' scripts, `--wide-check`, SDL's dummy
    drivers): stages 1-8, bonus 1 and 2, continue, one and four players and story_untouched in 20:9, and the stages,
    bonus stages, continue and one player in 16:9 and 16:10: no side column but the map's, but at stage 7's end (the
    helicopter's pick-up, some 130 frames in each width: up to 13 columns at the left, which the game writes there
    itself); the menus, OPTIONS, VIDEO, the portraits' choice, a story, stage 1 and the pause by `--actions`
    (build/wide_work/menus.txt). The Simpsons Arcade's 25 scenarios identical (2 min 49 s), the static analysis 43056 of
    43056, `rom_in_executables` 0 in the three games of build/bin-shell; tools/decompile.py with the new
    console/replaced.txt gives src/ as written (game.h and five files: CONSOLE_ORIGINAL).
  - Left, to see in the window: whether the spawns past the wider sides and the widened limits play well (the blind
    scripts are no players: in 20:9 stage 4's stalls at the pool tables, the scroll held by a player stuck behind
    one, while 16:9's and 16:10's reach the boss); stage 7's end's left side; the stories in WIDE (black bars today:
    story 1 scrolls a map that could be wide); the devices' VIDEO without WIDE (SIMPSONS_SCREEN_WIDE: 16:9 fixed).
- **13-11-2026, point 15, STEREO** (an agent): `simpsons_sound_stereo` (simpsons/sound_board.h; game.h's
  `game_set_stereo`), off by default: the mix takes the YM2151's second output on the right side at the board's 0.5
  (simpsons.cpp routes only the first, to both sides at 0.5), so every FM channel is on each side the program pans it to
  at the board's level; the K053260 and the left side as the board has them. MONO's WAV the same byte for byte as
  before (the attract, 7201 frames), the 25 scenarios identical. In attract (80), stage_1 (81) and continue (A1) the
  channels panned right alone are at −39.1, −37.2 and −38.6 dB, sounding in 55, 97 and 52 % of the 50 ms windows;
  STEREO's right side within 0.2 dB of MONO's level, no higher peak; most right-alone notes are twins of left-alone
  parts a few ticks off (echoes and doubles), continue has some 700 of its own; after_stage_7 (83), which pans nothing,
  the same byte for byte. WAVs in build/stereo_work/. The OPTIONS line is in (the Konami code's agent, below): OPTIONS
  > SOUND MONO | STEREO after VOLUME (console_settings.stereo, simpsons.ini's `sound=mono|stereo`, game_set_stereo at
  the start and at each apply_settings; `--stereo` sets it for a scripted run). Checked: OPTIONS' frame with SOUND in
  WIDE OFF and 16:9; a Konami game's stage 1 with and without `--stereo` gives the same state, video and sound hashes
  (5501 frames: the sound's are the chips', not the mix).
- **13-11-2026, points 9 and 10, the Konami code, the lives and continues, the devices' menu** (an agent; its frames
  seen by it, contact sheets and runs under build/konami_work/; Sebas: "No te olvides del Konami code", "En el menu"):
  - **The code** (menu.c): on the menu, up, up, down, down, left, right, left, right, B (the jump), A (the attack), from
    any player's controls or the keyboard (the last ten presses, one a frame), as tmnt-port's: the coin's sound
    (4002) and the KONAMI CODE page over the TV, as OPTIONS': STAGE (the eight stages and the two bonus games by their
    title cards' names: 1 DOWNTOWN SPRINGFIELD, BONUS 1 BALLOON, 2 KRUSTYLAND, 3 DISCOUNT CEMETERY, 4 MOE'S TAVERN,
    5 SPRINGFIELD BUTTE, 6 DREAMLAND, BONUS 2 WAKE UP, 7 CHANNEL 6, 8 NUCLEAR POWER PLANT), LIVES INFINITE | NORMAL
    (INFINITE at first), HEALTH INFINITE | NORMAL (NORMAL), REPEAT CHARACTERS ON | OFF (ON), START (or any START), BACK
    (to the menu's line the code was entered on). The menu has no line of it (the code's ups and downs take the line
    there and back, its A picks nothing); not kept in simpsons.ini (tmnt-port's).
  - **A game begun at a stage** (console_start_game, konami.c; the Konami page's START, and MR. PLOW MODE's modes
    next): the character choice on the portraits as ARCADE's, the story before stage 1 skipped by a playing slot's
    attack, and once every playing slot has come into stage 1's play (step_phase 4, each one's entry status) step_phase
    and its two bytes 0 and game_step the stage's, as the scenarios start it (the program at a step's end, A75E).
    Meanwhile (some 700 to 1200 of the board's frames) the stage's name on black and no sound (console_volume: the
    port plays it at 0; the game and the sound program go on as they do), the port running those frames as fast as
    it can (console_hurried: no waiting for the frame's time, none drawn but one in 16, game_set_draw); then the
    stage's own title card. RESTART: the same stage again.
    LIVES INFINITE keeps each playing slot's lives at 5, HEALTH INFINITE each player's vitality at 28 before every
    frame (a knocked out one, vitality 0, goes on as the game takes him).
  - **Lives and continues** (guideline 10, console.c): console_start_game(player, step, lives, continues) and
    console_set_mode_lives: the lives a credit gives (lives_per_credit) and the continues after each player's first
    credit, his credits counted (console_credit_allowed, console_credit_put: they go with him when he changes
    character), his START opening nothing once they are over; the countdown then runs out (GAME OVER), and when every
    player's has the game is over: the attract mode, the menu. ARCADE: the cabinet's (OPTIONS' LIVES, as many continues
    as STARTs; tmnt-port's ARCADE has no code's lives either). The Konami game: MODE_LIVES 5, MODE_CONTINUES 3, as many
    as one likes with LIVES INFINITE. A mode of MR. PLOW MODE only chooses its numbers and its first step, and adds its
    own rules each frame in game_mode_frame.
  - **Characters chosen more than once** (same_character.c, REPEAT CHARACTERS): a character is, to the program, its
    player object's kind, 00-03: the sprite engine's set of animations (animation_sets E591: the four share their
    animations' numbers, each draws its own moves in its own colours, the four palettes always loaded), and in
    src/players/ the attacks' reach (player_attack_boxes by state and kind), the voices (the command plus the kind),
    the palette's flash (22 plus the kind), the team-ups, what is carried and how high, the balloons and heights over
    the head. The kind is the player too (where he comes in: the kind times 32; whose blows a team-up's are:
    shadow_player's bits 4-5, the kind times 16; a story's random player), and the slot is his lives, score, box, its
    colour and his "1UP MARGE" mark. So the second player of a character plays on a slot nobody has (the character's
    own when free), console_slot_character: its object's kind the character's while player_run_all and
    players_update_second_sprites run (not at the frame he comes in: his place is his slot's) and while draw_object
    draws it (the second bonus game's players, kind 0A plus the slot, and the bonus games' results' portraits too),
    the slot's again after; a team-up's player bits written from a character back to the slot meant; two of the same
    character never team up (team_moves_by_characters' entries of a character with itself are the bytes around the
    table); his box shows the character's picture (gauge pictures 6-9, gauges.c). On the portraits the character's box
    takes all its players' frames and tags, and the portrait of a slot another character plays on does not come
    alive. Without the code, one each as before. console/replaced.txt: player_run_all, players_update_second_sprites,
    draw_object (tools/decompile.py with it: only their definitions and game.h's declarations change).
  - **The devices' menu** (menu.c, as tmnt-port's): SIMPSONS_SYSTEM_EXIT (the Switch, Android), no EXIT;
    SIMPSONS_HANDHELD (the Switch, the Anbernic, Android), no CONTROLS, and VIDEO with PIXELS, INTEGER SCALE and FPS
    alone (no WINDOW, FULL SCREEN, WIDE, VSYNC: the screen is the window, WIDE the device's or the screen's shape's).
    The macros reach simpsons_console_program on the three already (switch.cmake, anbernic.cmake, android.cmake).
  - **The version** (Sebas: "Te falta el nro de version"): on OPTIONS in its panel, on BACK's line at the right in grey,
    and under the menu beside the signature, "@cheviman  V0.1.0" (the small letters got the digits, '.', '-', V, T, E,
    S for a testing build's suffix).
  - **WIDE AUTO, the default everywhere** (Sebas: "Sí, AUTO por defecto"): WIDE_AUTO at the enum's end
    (simpsons.ini's `wide=auto`, `--wide auto`), VIDEO's WIDE AUTO, OFF, 16:10, 16:9, 20:9, AUTO showing what it
    resolved to ("AUTO 16:9"). main.c's follow_screen, for every build now: the screen's shape (a computer's: the
    desktop's mode of the window's display, or the full screen's size; a device's and a phone's: the renderer's whole
    output) at each frame shown, wide_for_screen (20:9 from 2:1, 16:9 from 16:9 or a tablet's touch screen, 16:10 from
    3:2, else OFF) into console_set_screen_wide; console_wide_in_effect is it under AUTO (OFF with --arcade-start), the
    one chosen otherwise, and wide.c and the window's size (apply_settings) take that one; the window sized again
    when it changes (the window on another monitor, a Fold opened). Android: AUTO replaces the forced WIDE; the Switch
    keeps 16:9 (SIMPSONS_SCREEN_WIDE). Checked: `--wide auto` with SDL's dummy video ("Screen 1024x768: WIDE AUTO ->
    off", VIDEO "AUTO OFF", RIGHT: OFF, 16:10); wide_for_screen as main.c has it, compiled alone: 1920x1080 16:9,
    2560x1600 16:10, 3440x1440 20:9, 1024x768 OFF, 1280x800 16:10, 2340x1080 20:9, 1280x1024 OFF.
  - **--arcade-start as the arcade again**: the PRESS START for INSERT COIN (gauges.c) changed the fixed layer's tiles,
    so the console's `--arcade-start` state differed from the scenarios' where a box showed INSERT COIN (the attract's
    demo, frames 2574-4618; stage1's 709-735); in the arcade's own scenarios the boxes are the cabinet's now
    (console_arcade_testing), and WIDE AUTO is OFF there: attract and stage1 the same state and sound hashes as
    build/scenarios, frame by frame (the video differs only by Bart's orange shirt, on by default in a scripted run).
  - Checked (build/bin-shell's The Simpsons, SDL's dummy drivers, `--actions` for the menus and `--script` for the
    players' controls in the game; build/konami_work/): the code on the menu and the page; each of the ten stages from
    the stage select (four blind players, HEALTH INFINITE) starting with its title card and going on to the next step:
    stage 1 to bonus 1, bonus 1 to stage 2, stage 2 to stage 3, stage 3 to its cemetery part, stage 4 to stage 5 (with
    the players walking down and up longer: the blind script stalls at the pool tables), stage 5 to stage 6 and stage 7
    to stage 8 (with HEALTH NORMAL: players knocked out come in again elsewhere; with their health kept they stay at
    the cave's mouth and a room's wall), stage 6 to bonus 2, bonus 2 to stage 7, stage 8 to its end and the game round
    to stage 1 (the ending skipped by the blind attack, as the scenario's); HEALTH INFINITE (an idle Marge, LIVES
    NORMAL: grabbed and hit to frame 12000, vitality 28 and 5 lives throughout); LIVES INFINITE (an idle Bart: knocked
    out again and again, 5 lives throughout, still playing); the continues (an idle Lisa, LIVES NORMAL: 4 credits, 5
    lives each, the fifth START nothing, GAME OVER, the menu); ARCADE's continues as before (6 credits, 2 lives each);
    two Barts, four Lisas, and Bart, Bart and Marge (each moving and attacking, their boxes' pictures, the portraits;
    two Barts alone stay at stage 1's start, as a lone Bart does with the blind script: the camera never pushed);
    RESTART of a Konami game at stage 4; the black and silent start (its sound samples silent to the stage's title).
    The devices' menu by a build with SIMPSONS_HANDHELD and SIMPSONS_SYSTEM_EXIT (build/msvc-device): the menu, OPTIONS
    and VIDEO as above. The Simpsons Arcade's 25 scenarios identical (2 min 57 s), the static analysis 43056 of 43056,
    rom_in_executables 0 in build/bin-shell's three games and build/bin-device's.
- **13-11-2026, 18, Bart's shirt in orange** (an agent; its frames seen by it and by me, contact sheets under
  build/bart_shirt_work/sheets/; Sebas: "la remera de bart. Que sea naranja", then "hay que agregar un tono de naranja
  mas oscuro a las mangas" and "cuando lo agarran, es azul"): the cabinet dresses Bart in light blue (xBGR 7F00),
  shaded with his shorts' blue (pen 8); the game's own orange for him is the attract mode's opening, Bart skating out
  of school (115F: 31, 10, 4). The Simpsons draws his shirt in it, and its shade in the orange as much darker as the
  cabinet's shade is than its light blue (CE4218). The four characters share one palette (the sprites' colour 0: pen 7
  Bart's shirt, the same light blue as Maggie's onesie and the diamond in the pictures), so there is no palette swap:
  the palette RAM stays the board's and the video draws Bart's pixels orange (runtime/src/video.c's port-side
  recolouring, simpsons_video_set_recolour: pens of named sprite cells in given colours, per-cell pixel masks drawn in
  a shade, and the layers' tiles by map cells and tile codes, through the console's colour functions; nothing unless
  the console sets it). console/bart_shirt.c walks, from the program's data as draw_object reads them, Bart's sprites:
  his kind (02), his figure in the bonus games (0C), his sprites of the ending's family (14), and Bart held by the
  goons that grab a player (80, 81, 97) or far away (A4). It assembles each of their frames to tell pen 8's shirt shade
  from the shorts (by component and border, voted per cell pixel over 361 frames), and has six rules for five pictures
  (the attract's profile and couch, story 1's Maggie with the diamond, the ending's hug, the walk to Channel 6). A fade
  of the light blue is the same fade of the orange. Always on in The Simpsons: console_settings.bart_shirt,
  simpsons.ini's `bart_shirt=orange|arcade` (no OPTIONS line); SIMPSONS_BART_SHIRT=arcade in the environment for checks.
  - Checked: the console's frames against the arcade's (--arcade-start) and against its own with the shirt off
    (--actions): every pixel that differs is the shirt or its shade turned orange, none other (stages 1-7, 60 frames
    each; the ending; bonus 1 and 2; story 1; the attract; the goons' grabs in stages 2 and 3; player 1 as Bart
    standing on his skateboard, Sebas's pose). The state and sound hashes of attract and stage 1 are the same with the
    shirt on and off and equal to the arcade's; The Simpsons Arcade's 25 scenarios identical, the static analysis
    43056 of 43056, rom_in_executables 0 in the three games of build/bin-shell.
  - Left as the cabinet has it: the darkest blue (pen 12) at the back's lower corner, which is the shorts' shade too;
    story 1's collage with Smithers (a few pixels of Bart's sleeve, the picture's background tiles).
- **13-11-2026, the Konami code's leftovers** (the same agent; Sebas: "no los dejes intermedios"):
  - **No wait for the stage**: the frames under the black run as fast as the machine can (main.c: console_hurried,
    no wait for the frame's time, one in 16 drawn and shown, game_set_draw: the runtime draws none of the others, the
    board's state the same either way). Measured with SDL's dummy video at the board's pace: a lone Marge's start to
    stage 5 (about 1190 covered frames, the story waiting longest with her) in 31.6 s for 3000 frames instead of
    50.7: the covered frames in about 1.05 s; four players cover some 700. The state and sound hashes of the 3000
    frames the same as `--fast`'s and as before the change, the card at frame 2288 as before.
  - **The join mark names the character** (same_character.c, same_character_marks): "1UP MARGE" is one sprite of two
    cells per character (7828 Marge, 782C Homer, 782A Bart, 782E Lisa, pens 9, 4, A, 8), the slot's number in rows
    0-9 and the name in rows 11-15; while a slot plays another character, its mark's name rows are that character's,
    in the slot's pen, in the sprite data in memory (game_sprites), and the assets' again out of such a game (nothing
    else reads those cells but the test mode's ROM check). Seen: "1UP BART" in red over a second Bart, "1UP LISA" to
    "4UP LISA" over four Lisas.
  - **The high scores** (console/replaced.txt: insert_high_score, name_entry_draw_picture): a slot played by another
    character goes in the table as it (the ranking's icon), the initials a player's last score goes in with as he
    joins again (player_join: MRG, HMR, BRT, LSA) the character's, and the name entry's column shows its face.
    Seen: a second Lisa on Marge's slot in again with 9999: the table's first entry LSA, character 3 (was MRG 0), and
    the name entry's two Lisa faces.
  - **The hurt flash** (console/replaced.txt: player_flash_palette): measured, it is no sprite's: the palette effect
    22 plus the kind (palette_effects 35-38) animates pen 11 of palettes 20-23, the four players' boxes' colours (2C1D
    Marge's red, 7E48 Homer's blue, 0300 Bart's green, 02DF Lisa's orange) in the fixed layer, and the players'
    sprites draw with the sprites' colours (theirs 00 here, the sprites' base 0). So it says whose box: with the kind
    the character's, a second Bart hit on Marge's slot flashed the first Bart's green box (the two seemed to flash
    together). Now the slot's own box flashes (the kind the slot's while player_flash_palette runs). Seen: the second
    Bart hit (frames 2380-2700, his vitality from 24 to 10, the first Bart's at 0C): before, palette 22 pen 11
    0300/4CE0/019F and palette 20 at 2C1D; after, palette 20 4411/7F00 and palette 22 at 0300; the frames before and
    after differ in the boxes' band alone (17,1 to 232,26), the game the same (the same vitality frame by frame), both
    shirts orange (flash_boxes.png). No video hook needed: the sprites have no flash of their own.
  - `--arcade-start` the arcade's again: the boxes' PRESS START (gauges.c) changed the fixed layer where a box showed
    INSERT COIN; in the arcade's own scenarios they are the cabinet's now (console_arcade_testing), and WIDE AUTO is
    OFF there: attract and stage1 the same state and sound hashes as build/scenarios.
- **13-11-2026, 12, 13, 14, 16 and 17: MR. PLOW MODE, the high scores kept, save states** (two agents, stopped by
  Sebas to save the budget once their code was in; finished and checked by the lead): console/modes.c (BOSS RUSH:
  each stage crossed to its boss out of sight and as fast as the machine goes, the card "BOSS RUSH, BOSS N OF 9",
  the stage and the boss's name; BONUS GAMES: the two bonus stages in a row, one to four players, their results,
  back to MR. PLOW MODE's page; FRANK GRIMES LIFE: one player, the game counting four, console/gauges.c putting out
  the other three boxes whole: their pictures, tabs, lines and scores, Sebas: "si no hay jugadores, no tendria que
  verse"); console/scores.c (simpsons.scores: the table into the game after the boot, written when a new score
  comes in); runtime/src/state.c, game_state.c and console/states.c (the pause's SAVE STATE and LOAD STATE, one
  state in simpsons.state, `--state FILE --state-save FRAME --state-load` for checks).
  - Checked: from the menu, BOSS RUSH reached the wrestler, then the Krusty balloon, then stage 3's gangsters (a
    blind player, 5 lives: then CONTINUE, GAME OVER and MR. PLOW MODE's page again); BONUS GAMES played both bonus
    stages to their results and went back to the page; FRANK GRIMES LIFE played with the other boxes empty. A state
    saved at stage 1's frame 3000 and loaded in a new run: the 9601 frames after it identical in state, picture and
    sound to the run that went on. A kept table with SEB first: in the game's table at the attract (frame 400). The
    Simpsons Arcade's 25 scenarios identical, the static analysis 43056 of 43056, rom_in_executables 0 in build/bin-
    shell's three games.
  - Not checked yet: BOSS RUSH past its third boss (the blind player loses there), the kept table written after a
    new score's name entry (read back only), save states in a bonus stage, a story and with four players.
- **13-11-2026, the plan closed here** (Sebas: "Cerremos el plan. Y yo activo otra sesión para el lan"): done 1-10,
  12-18 (19 looked at: the World set has no Japanese music, that is the 2-player sets' 072-g03); 11, MULTIPLAYER over
  the LAN, goes to a session of its own (arcade-lan as tmnt-port's console/lan.c). Not pushed until it is: b05bf45.
  (11 done the same day, below: the session never started, the lead did it.)
- **13-11-2026, the scenes without the pause, FREE PLAY and ARCADE's continues** (Sebas: "en las cinematicas que no se
  pause, en mr plow, agreguemos un modo normal de juego, pero sin limite de continues, en arcade, agrega en options el
  limite de continues"; the runs under build/continues_work/):
  - **The scenes**: check_skip in console/replaced.txt (game/skip.c: what a button skips, after its wait: the stories,
    the stages' title cards, the stages' and the bonus games' ends, the bonus games' instructions, the ending). While
    the game calls it (and 30 frames after, its parts' gaps), and all through the stories' and the ending's steps (1,
    5, 10, 14, 16), console_can_pause is false: no pause by START, Escape, Back, the touch screen or the pause asked
    for; a START of a player whose character plays skips the scene as his attack does (one frame let go, two of the
    attack). Checked: a pause asked for in story 1 (frame 1150) did not open, the one in stage 1's play (2600) did;
    story 1 with START every 60 frames reached stage 1 at frame 1300, without it at 1800.
  - **FREE PLAY**, MR. PLOW MODE's first line (MODE_FREE_PLAY, free_play_start in modes.c; the modes' order and
    console.c's way back to the page from it): ARCADE's game, OPTIONS' lives, as many continues as STARTs whatever
    OPTIONS' CONTINUES says, and after the ending the menu (as the modes'). The name can change (Sebas named none).
    Checked: an idle Marge with START every 400 frames put her sixth credit in (a mode's 3 continues stop at the
    fourth), "mode free play" all along.
  - **CONTINUES** in OPTIONS, after LIVES: ARCADE's continues after a player's first credit, 0 to 9 or UNLIMITED
    (console_settings.game_continues, 3 by default as the modes' and TMNT's; simpsons.ini's `continues=3|unlimited`),
    console_start_arcade's. Checked: OPTIONS' frames (3 by default, 1 after two LEFTs); ARCADE with 1: Marge's second
    credit, then her START opened nothing, GAME OVER and the menu (frame 12500).
  - The version 1.0.0 (CMakeLists.txt's project), for the release.
- **13-11-2026, 11, MULTIPLAYER over the LAN** (Sebas: "arranquemos el lan"; "La versión del tmnt es la última
  implementada, pero el repo de arcade-lan": tmnt-port's console/lan.c the model, arcade-lan at its latest, ad5dfe7, in
  lan/ as a submodule; the runs under build/lan_work/):
  - The build: arcade-lan (CMakeLists.txt's ARCADE_LAN_DIR, SIMPSONS_LAN) linked to The Simpsons alone, on every
    platform; iOS with ARCADE_LAN_NO_BROADCAST (the rooms found by the library's SEEKs and sweeps) and
    NSLocalNetworkUsageDescription, Android with INTERNET; its license in the packages (licenses/arcade-lan.txt).
  - console/lan.c (tmnt-port's, the game "SIMP", its beacons at 27780: OutRunners' 27720, TMNT's 27740, SegaSonic's
    27760): the menu's MULTIPLAYER (between MR. PLOW MODE and OPTIONS; the menu's lines 12 pixels apart from 96, the
    copyright and the signature where they were) with HOST, JOIN, NAME (simpsons.ini's `name=`), BACK; JOIN's rooms;
    the room's devices, the host's START (two or more) and LEAVE. The host's START sends its DIFFICULTY, LIVES,
    CONTINUES, DEMO SOUND, SCREEN and the WIDE in effect (AUTO as its screen has it: WIDE moves the spawns and the
    players' limits).
  - The game's start: no tasks here, so the port leaves the program by longjmp at the frame's end (segasonic-port's
    way), game_reset, console_lan_begin and the program from its start again; the console as at power-on
    (console_lan_power_on: the console's state, the joins, the scenes' skips, characters.c's last pick, wide.c's
    rings, the modes', the TV), the host's settings into the EEPROM over the factory's, the boot's high score table
    on every device (scores.c: none of a device's own, nothing of the game kept, the kept table back once it is over),
    Bart's shirt again; black over the boot, then ARCADE's game for the host (the room's first device) and the
    character choice on the portraits, where every other device's player comes in with START and picks, as at home.
  - Each frame: the device's player 1's controls and START newly pressed (2 bytes, 3 frames' delay; nothing of the
    port's own keys in the game), every device's back through console_controls, the room's devices in its list's
    order the console's players 1-4; the frame held till they come (WAITING FOR its name over the picture); the state's
    hash after each frame to the host. No pause: Escape or Back leaves the game (its character stands, the game goes
    on for the others), the game over, the host gone or OUT OF STEP: the MULTIPLAYER page again with the device's own
    settings, never saved over by the host's. --lan-name and --lan-hashes for the checks, --actions and --video-dump
    counting the run's frames across the reset.
  - Checked on this computer, two of The Simpsons with SDL's dummy drivers at the board's pace: the host's room found
    by the guest, START, Marge (host) and Homer (guest) picked, story 1 and stage 1 played with both attacking and
    walking: 4096 LAN frames with the same state hash on both (the guest's pictures the host's), the guest leaving at
    its run's frame 5000 and the host playing on alone (Homer standing), each back on MULTIPLAYER and out by EXIT; a
    second pair, the host leaving at frame 3500: 2597 frames the same, and the guest's MULTIPLAYER page with HOST GONE.
    arcade-lan's own tests (lan_test, lan_link_test) pass; rom_in_executables none in The Simpsons, The Simpsons Arcade
    and Simpsons NSP; The Simpsons Arcade's 25 scenarios identical.
  - Only two real devices show: the rooms found across a real network (a firewall's prompt on Windows the first time),
    the Switch's, Android's and iOS's sockets, and the pace with a network's delays. A device's second player is not
    in yet (the library takes them, the menu has no PLAYERS), as in tmnt-port.
- **14-11-2026**: 20 to 26, the players' requests from 1.0's thread on r/decomps (Sebas: "hagamos así, lo ponemos en
  to-do"): RUN-AHEAD, the players' boxes at the edges, a window across two screens, WIDE 32:9 after the chip-agnostic
  port, what the game does not use, new stages from a player's ideas, and the scaling filters with the GPU's picture
  once the decomp is at 100 %.
- **14-11-2026, WIDE, stage 1's goon at its start** (a player's report; Sebas: "Primer nivel, cuando empieza, hay un
  enemigo ahí fijo y después desaparecen en los 4:3", "El error parece q es al inicio. La posición inicial fuera del
  4:3"; a session of its own): in 16:9, 20:9 and 16:10 a goon stood still in the picture's right side through stage
  1's opening scene, then walked in.
  - **simpsons_console_headless** (Sebas: "Hace un headless para probar rápido"): ports/headless/main.c with
    SIMPSONS_CONSOLE, as tmnt-port's tmnt_console_headless: `--actions` (the window's names: up ... start, pause,
    dump), `--wide`, `--wide-check`, `--arcade-start`, `--video-dump` with the console's picture over the frames, the
    hashes, `--dump`, and `--frames` ending it; not on the devices. 1800 frames in some 4 s. Without a script the
    players' controls are none each frame (the board holds what the console put there: read back, the attack was held
    for ever and no story was skipped).
  - The cause: the goon (type 00) the spawn list puts in at the play's first frame (frame 1495 by the menu: the list
    read at once) at the screen's x 0180, waiting for the scene to end: on the board under the cabinet's black edge
    (its sight ends at 016F), in WIDE in the right side. It was put in before WIDE's sides came (they come at the
    frame's end), so as the board puts it. Moved past the side, it went round and round there (move 13, ground it
    cannot walk on): the console drew the sides' floor map cells from the camera's layer (A in the opening scene), but
    the game draws stage 1's floor from layer B (draw_layer_columns_with_floor_map, 012C): walls in the right side
    while the scene played, for the players too.
  - The fix (wide.c): the sides' floor from the layer the game last drew whole with it, or the camera's once that
    changes (floor_layer, in the state), and past the right side as far as the ring has room with what the picture
    shows (DRAWN_AHEAD, 0x28: the board's is valid past its own right edge, its new column 0160 past the left one).
    The play's first frame is the board's, but for the spawns it puts in (spawn_as_play_begins). A spawn list read at
    once puts in where the board does what the cabinet shows (a scene's), and the goons that come in by themselves
    (comes_in_type: 00-03, 08, 0A, 0C-0E, 12, 15, 17, 25, 28-2A, 2E, 34, E6, EB) that the cabinet did not show as far
    past the wider screen's edges as they were past its sight (past_right_side, past_left_side; no further than the
    floor drawn: in 20:9 at a stage's start some 16 pixels of them show at the picture's edge, the ring's 512 holding
    the 496 shown). Before, the WIDE path moved every spawn of the left by both sides, a scene's too: with the first
    frame in it, stage 1's opening lost Grandpa, Patty and Selma, Smithers and his goons.
  - The enemies' moves 9 and 3 (enemy_wait_while_target_down, enemy_back_off, console/replaced.txt) stop an enemy
    past the wider screen's sides, 16 pixels past its sight as the board's 40 and 17F are past the cabinet's; and, as
    the session's brief asked to look at: ninja_rush_leap (stage 7's leaping ninjas bounced at 0176-0177 and 0042,
    48 pixels in from the picture's edge in 16:9), clown_on_ball_hop (stage 2's clown stopped in the right side,
    hopping where he was) and krusty_balloon_fly_up (Krusty's balloon, knocked up, bounced off the board's left edge
    112 pixels in from the picture's) with the same limits moved. tools/decompile.py with the new replaced.txt:
    src/ as written (game.h and four files: CONSOLE_ORIGINAL).
  - Checked (the headless): stage 1's start by the menu in 16:9, 20:9 and 16:10, the opening scene whole as in 4:3
    and the right side empty till it ends, the goon (at 01F0, 022F, 01D0) walking in and fighting; the ninjas'
    bounces at 01B6 (and at 0176 where the right side is less than none, as the board), no balloon bounce at the
    board's edge. `--wide-check` (now also the spawns put in that the wider screen shows and the cabinet did not) on
    the eight stages' scripts in 16:9 and 20:9: no side column but the map's but stage 7's end (the 128 known), and
    in sight only what is left below. WIDE OFF: the 13 scripts' state hashes the same as before the change frame by
    frame (the console's window, `--arcade-start`); The Simpsons Arcade's 25 scenarios identical, the static analysis
    43056 of 43056; rom_in_executables none in the four executables (the console's headless too); the Switch's two
    games (devkitPro, 0 warnings).
  - Left, in WIDE's sight as they are put in: stage 1's police car (E9, driving in by a timer: moved it would stop past
    the side) and what comes with it (F8), stage 6's donuts (FD: they put themselves at 0180 when they walk in) and its
    flying saxophone (FC, leaping in from the front), stage 3's ghost on its rope (11, hanging above), a goon from the
    side (3F) at the board's left edge where the picture has no left side, stage 7's ninja (32) at 0180 and kind 0F;
    and a spawn put in as the scroll comes at a camera's stop (the right side 0): a few pixels of it.
- **14-11-2026, WIDE, stage 3's climb** (Sebas: "El nivel del ascensor empieza mal alineado y tendríamos q hacer una
  reconstrucción para q sea wide. Xq como se mueve la primer parte de arriba para abajo"; "Fijate lo del ascensor"):
  stage 3's second step (7, scene 87) begins with layer A scrolling up (cemetery_scroll_up: its y from 0 to 0500,
  rows drawn and no columns), which WIDE shows as a picture of the board's width (what_shows). The play before it had
  its sides at the map's left end (16 at the left, the rest at the right: 16 and 80 in 16:9), the picture centred
  them (48 and 48): the board's part jumped 32 pixels as the climb began and 30 as it ended.
  - The fix (wide.c, console_wide_frame): a stage's play drawn as a picture keeps the sides the play had, the players'
    boxes where they were; the play after it pans from there. Checked (the headless, stage3's script): no jump past
    WIDE_PAN in 16:9, 20:9 and 16:10 through the climb (16:9's step change, 14893-14895, keeps its two frames of the
    board's); `--wide-check` no side column but the map's; 4:3's state hashes (stage1, stage3) the same; The Simpsons
    Arcade's 25 scenarios identical; the Switch's two games built (0 warnings).
  - The climb wide (the reconstruction), measured and not done: layer A's map (768 pixels) has the stage's own picture
    past the cabinet's columns (blocks of many tiles at 320-767). With the climb shown as a play (sides drawn from the
    map, tried behind an environment variable), `--wide-check` with the board's columns too: the console's columns are
    the map's in the whole picture up to layer A's y 03E0 (15027-16933 of the script, about two thirds of the climb);
    past it they are not, the board's own columns too (13 to 46 of them): there cemetery_scroll_up sets layer A's y
    offset (layer_a_offset_y) by stage_3_part_2_layer_offsets as its y passes their limits, and the game's rows
    (find_layer_row: the map's cell at its scroll y plus that offset and the second one) come from rows the console's
    columns (find_layer_column's: no first y offset) do not draw; no single y offset makes a column match. To do it:
    the side columns drawn as the game's rows are, with that offset, and checked in each WIDE.
- **14-11-2026, WIDE, stage 1's opening: Smithers' goons** (Sebas, on his phone with 1.0.1: "Primer nivel. Cuando se va
  smithees con los secuaces. Desaparece en 4:3"): a scene's actor is taken off when its screen x leaves 0..018F
  (story_actor_finish_frame: 0x11 past the board's right edge, out of the cabinet's sight), in WIDE's side; and the
  scene's end (its own actor's script, object 32 at 5640, at its FF) takes every actor off, which in WIDE came while
  the goons still ran through the right side (frame 1656 by the menu, the cabinet's gone by 1645).
  - The fix (wide.c, console/replaced.txt: story_actor_finish_frame and story_actor_next_operation): an actor taken
    off past the wider screen's sides (beyond_limits, 0 and 018F moved as the others); the scene's end, while one of
    its actors is out of the cabinet's sight, in the picture's and going out, waits a frame (its script left at the
    FF, a wait of 1, as the game's waits do), STORY_END_MOST (120) frames at most, the count in the state.
    tools/decompile.py with the new replaced.txt: src/ as written (game.h and story_actor.c).
  - Checked (the headless, stage 1 by the menu): the goons taken off out of the picture's sight in 16:10, 16:9 and
    20:9 (01E0, 0201, 023F); the scene ends 14, 25 and 51 frames later than the cabinet's (the play begins that much
    later); WIDE OFF the same (0191, 0192, the goon walking in at 1659).
- **14-11-2026, WIDE, stage 1's restaurant figures** (Sebas, on his phone: "la señora ... estaba haciendo clipping en el
  salón de video y tiene q ir en el otro local"): moved with the goons that come in (comes_in_type 08), the figure
  at the restaurant (type 08, kind 87, put in at the scroll's edge with the cook, type 26 kind 0F, at its own place)
  stood past the side in 20:9, the cook with it at the video arcade's (map 495 for 352; 447 in 16:9). A spawn's type
  does not say what it is: 08 and 2A are stage 1's shops' figures (kinds 87, 93; enemy_class 0) and goons elsewhere.
  - The fix (wide.c): 08 and 2A out of comes_in_type; and a moved spawn that is not an enemy after its first two
    frames (enemy_class not above 0) goes back to its place, with what it put in at its own place meanwhile, unless
    it has put itself somewhere (settle_moved_spawns, in the state). Checked (the headless, stage 1 by the menu with
    player 1 walking right): the cook at map 352 in 4:3, 16:9 and 20:9; the start's goon still past the side.
  - On the way: the play's first frame had the board's limits for its objects (WIDE's sides come at its end), so the
    start's goon, moved past the side as it was put in, was taken off at its first step (enemy_remove_off_screen
    with the board's 01C0); that frame's objects now have the sides the play comes with (play_sides_now), its spawns
    the board's (spawn_as_play_begins).
- **15-11-2026, Barney's shirt** (Sebas: "hay que arreglar tmb el tema de los colores de barney, que estan los de
  season1"): his hair was tried in brown first (his 23 frames assembled, the hair found round his eyes, the video given
  a second colour by masks), and left as the cabinet has it, yellow ("no, quedo horrible", then "dejá el pelo
  amarillo y cambiale el color de la remera y listo"); his shirt in Sebas's #D46F63. console/bart_shirt.c: stage 4's Barney (the
  bystander at Moe's, kind 8C, colour 9) has his green shirt in pen 4 and its shade in pen 5, pen 4 his shoes'
  stripes too: his frames are assembled and a cell is the shirt's where most of its two pens' pixels are 16 rows over
  his feet or more; those cells' two pens recoloured in his colour, the green's fades to the salmon's (26, 14, 12:
  214, 115, 99) and the shade's to a salmon as much darker. Bart's shirt's setting (bart_shirt=orange) and its check's
  switch hold it too; the kinds' lists walked once for both (walk_kind). The video as it was.
  - Checked: stage 4's frames with the colours on and off differ in the two shirts alone (Bart's light blue and its
    shade; Barney's 00845A and 005239 to D67363 and 844A42), 5573 pixels in 59 frames; stage 1's as before (2979, Bart's
    alone); Barney seen 6 times his size: the shirt salmon, its folds darker, the hair yellow, the shoes' stripes green.
- **15-11-2026, INTRO** (the players' issue 8, "Intro Movie watchable within The Simpsons Console Version"; Sebas:
  "Solo INTRO en el menú", then "si esta inactivo, que meta una intro"): the menu's INTRO line (between MULTIPLAYER and
  OPTIONS; the menu's lines from row 90, 11 apart, so that six and the copyright and the signature fit the TV's
  screen) plays the story before stage 1, Smithers taking the diamond and Maggie with it, and the menu comes back at
  INTRO; the menu left alone 30 seconds (IDLE_FRAMES, nothing pressed) plays it too, as an arcade's attract mode
  does. modes.c's MODE_INTRO: a game started as ARCADE's, its choice made at once (the player's Marge: characters.c,
  as RESTART's), the start and the choice under black and silence (modes_covering, its frames hurried) till the story
  shows; as the story ends (stage 1 next), START or the attack skip its scenes to it, or BACK leaves it, the game
  ends (console_end_game). A start that never comes back to the menu with INTRO's black off.
  - Checked with the console's headless and --actions: INTRO picked (the choice at frame 940 under black, the story
    from 1100, the menu at 2300; left alone, it plays again at 3700), BACK in the story (the menu the next frames),
    START in it (the menu at 1650), and the menu left alone from power-on (the story at 2500). The story's frames with
    the shirt on and off differ in Bart's shirt alone; the diamond in Maggie's mouth is the cabinet's darker blue in
    both. The menu's six lines, the copyright and the signature drawn inside the TV's screen.
- **15-11-2026, WIDE, stage 3's elevator** (the players' issue 5, SteamOS: "wide is inconsistent: some scenes have,
  some don't"; Sebas: "vamos con el 5, que es la escena del ascensor", "no hay que extender los limites del nivel para
  que los jugadores no se salgan", then "dejemos con esos bordes negros, pero bien centrado"): the elevator kept the
  sides its standing's play had at the map's left end, 16 at the left and the rest at the right (the board's part 16
  pixels left of the middle in 16:10, 32 in 16:9, 72 in 20:9), and its standing was a play, with the wider screen's
  limits.
  - The fix (wide.c): elevator(), step 7 from the platform's standing till cemetery_scroll_update's step 5 (the play
    at the shaft's top), is a picture, centred as the others, its sides black, the platform's limits the cabinet's;
    its layers that do not scroll up draw their sides under the black, as the play did (draw_side_columns), for the
    play after it (else layer B's columns under the cabinet's right black edge stay the step before's: --wide-check,
    20:9, at the shaft's top). The play after it holds the picture centred while the map has no room at the left
    (black past its end, as the elevator's sides; the objects' left side the map's: held_left, play_sides_now), then
    goes on from there: no jump as the elevator begins or ends. Stage 8's start, a play drawn as a picture too, as it
    was (the picture where the play had it).
  - Tried and left off (CLIMB_SIDES 0, wide.c's climb_fill; Sebas: "no me gusta", "deja ese arreglo comentado porque
    tendriamos que hacer unos sprites para eso"): the sides of the shaft's own cells, as segasonic-port's sides (each
    row's repeat that holds where it meets the side; no mirror: the K052109's tiles do not flip across here), in layer
    A's cells and put back each frame. The bricks and the beams went on, but the stairs as rows of blocks and a
    landing's floor as a slab down the side: to do with tiles of their own.
  - Checked (the headless, stage3's script): sides 32/32, 48/48 and 88/88 from the step's change (14045, 14893, 14525)
    through the elevator into the play at the top, which pans once the camera has brought the map (16:10 from 17555,
    16:9 from 18347, 20:9 from 17883); `--wide-check` no side column but the map's in the three; the eight stages'
    scripts in 16:9 with the same sides frame by frame as before but stage 3's (stage7's script's 128 reports at stage
    8's end as before); 4:3 (WIDE OFF) the same state and video hashes as before (stages 3 and 8); The Simpsons
    Arcade's 25 scenarios identical (state, video, sound); both CLIMB_SIDES builds with no warnings (MSVC, GCC
    -Wall -Wextra); the executables with nothing of the ROM set.
- **15-11-2026, WIDE, stage 2's Otto** (the players' issue 16: "In stage 2 in 20:9, Otto doesn't appear at his stall
  until the player, drops down from their balloon at the start of the level"; Sebas: "Yes, #16"): a stage's start
  reads its spawn list once (start_stage_objects), and the game's play frames (stage_play_frame, which runs
  spawn_stage_objects) come only after its opening (stage 2's: the family coming down in their balloon). WIDE puts a
  spawn in as far past the wider screen's right side as past the board's, but in the play's first frame only the
  board's (spawn_as_play_begins): Otto (type F8, at x 01EC, past the cabinet's 017F, in 20:9's right side of 176 at
  the stage's start and at 16:9's edge) came in the first play frame after the balloon (stage2's script, 2865 in
  20:9), his stall empty through the opening. So did stage 1's restaurant door with its OPEN sign (type 08).
  - Tried first: in the play's first frame, the wider screen's whole due list. Otto came, but the two goons due with
    him (34, 0C) took objects 19 and 21, and the balloon, put in by the opening the frame after, lost two of its six
    objects: it was gone.
  - The fix (wide.c): put_in_sighted_spawns: in the play's first frame, after the board's spawns, those due in the
    wider screen's right side that stand in its sight (not those that come in by themselves, comes_in_type, nor those
    from the left) are put in ahead of the list (early_spawns, 8 at most, in the state, forgotten at a step's change);
    the list skips them as it comes to them (put_in_early; in WIDE also one that took itself off, stage 2's E9, gone
    in its first frame). The loop's body is put_in_spawn; with WIDE turned off meanwhile and some put in ahead, the
    console's loop puts in as the board does (x less 0160 for those from the left, no move for those that come in by
    themselves: the local worker's diff of the two, checked by hand), skipping those still there and putting in again
    those the game took off past the board's screen (their objects in early_objects).
  - Put in ahead at the stages' starts: stage 1 the restaurant's door (08; in 20:9 the video arcade's 2A and 11 too),
    stage 2 E9 and Otto (16:9, 20:9), stage 6's four donuts (FD, 20:9; not drawn till they fly), stage 7 FC (in 20:9
    also a 0F, which waits in sight and walks in as the play begins; with it the enemy limit holds two goons back
    there, waiting out of sight anyway); stages 3, 4, 5 and 8 none.
  - Checked (the console's headless, the eight stages' scripts in 16:10, 16:9 and 20:9 against the build before it):
    Otto at his stall from the play's first frame, the balloon whole (objects 18-20 and 30-32), the goons in objects
    19 and 21 at 2865 and every frame after the same; stage 1's opening with its three objects (00/CA, 00/C9) in
    other objects and its phases the same; no opening's object lost; --wide-check's columns as before; 4:3 the same
    state and video hashes as before (stages 1, 2 and 7); The Simpsons Arcade's 25 scenarios identical; no warnings
    (MSVC, GCC -Wall -Wextra). WIDE turned off with Otto put in ahead (the window build, stage 2 in 20:9 saved at its
    2800 and loaded with WIDE OFF, the players kept up by pokes): Otto taken off at 2924 as the play began, past the
    board's screen, and put in again at 018E as the camera came to 005E (3668); never two.
- **19-11-2026, the story before stage 1 skipped whole** (Sebas: "otro error heavy que encontre, cuando entras al
  principio del juego, te deja adelantar la cinematica pero el audio sigue corriendo normal y hasta que no se termine
  no te avanza"): the cabinet's own. Story 1 (game_step 1) is four parts, each with its check_skip (story_1_street,
  story_1_redraw, story_1_rise, story_1_end): a button ends that part alone, after the part's wait of 40 frames, and
  the story's sounds go on. Measured (ARCADE, Marge alone, the story from frame 989): untouched it ends at 1883 (894
  frames, 15 s); one START at 1100, at 1819 (64 frames less); a START every 10 frames, at 1272 at the earliest. The
  other scenes a button skips have one check_skip and end whole (stage 2's and 5's ends, stage 6's wake, stage 7's
  end, stage 8's start, the bonus games' instructions, the title cards, the ending); steps 5, 10 and 14, "the
  stories" 2 to 4, last a frame (story_steps.c: theirs play in the next stage's opening).
  - The change (console.c, konami.c): the first skip takes the story with it (scene_skipped): a START of a player
    in the game at once, the attack or the jump as the cabinet takes them (after the part's wait: the presses that
    chose the character skip nothing). Till the step ends the playing slots' attack is pressed 2 frames in 8, their
    own controls off the board, under black, at volume 0 and with the frames hurried (console_scene_skipping in
    console_hurried), as the Konami code's story before its stage. The black and the silence 3 frames more
    (scene_quiet, console_scene_quiet in console_overlay and console_volume): the frame the step changes at still
    has the story's last picture (Maggie with the diamond, seen a frame before this), and the story's tune sounds
    on till the title card's first sound command takes its place.
  - Not INTRO's story (its scenes one by one: it is there to be seen), nor a LAN game's (its frames' pace is the
    network's): the cabinet's there.
  - Checked (scripted; SDL's dummy video, and a window with VSYNC): one START at 1000, inside the first part's wait:
    stage 1's step at 1273; at 1100: 1321; at 1300: 1377. The attack (--script) at 1040 nothing, as on the cabinet,
    at 1100 stage 1 at 1313. Black from the START to 1276, STAGE 1 DOWNTOWN SPRINGFIELD's card from 1277. The sound
    (the samples' mean size each frame): volume 0 through the story's last three frames, then the card's as an
    unskipped run's from its first sound. The window's 19 hurried pictures all shown (ports/PLAN.md, issue 18). The
    25 scenarios identical. Not checked: a controller's START on a device, four players.
- **20-11-2026, WIDE at stage 2's end and at stages 3's and 5's starts** (Sebas, from a player's video of
  1.0.1-TEST13: "te paso capturas de escenas que el 16:9 se rompió. aca cuando le ganas al globo de krusty", "cuando
  cae homero que es cinematica la pondria 4:3", "los secuaces esperan en altura de 4:3", "en el nivel del bosque
  tambien"; the elevator's players' boxes in the same video were issue 5's, already right in testing):
  - **The hole after the Krusty balloon** (stage_2_after_boss: map area 1 on layer B, drawn at the boss's place, map
    pixels 680-85F): its strips' table is the same strip 8 times (7D7E, the void) and its wall (7DFE); map_of cut a
    table at its first repeat, took the hole for a 256-pixel map, drew it whole round the ring (draw_whole_ring), and
    the sides showed pieces of its wall again. A repeat is the same strip now: the hole a map of 2304 pixels, its sides
    the void's. (The left side's black with a light blue edge above the floor is the funhouse's Krusty face in the
    night's colours: the stage's map, the cabinet's as its camera passes it, 12044.)
  - **Stage 2's closing story** (stage_2_end_phase, step_phase 7: the fall off the cliff, area 2, a map of 512 pixels,
    and Homer's down the gorge, area 3, scrolling up): no longer the play, which kept the boss's sides (112 and -16 in
    16:9) and showed the maps round the ring past them: a picture, centred, its sides black (black_drawn), from the
    black frame its first is.
  - **A stage's title card** (step_phase 1, stage_title_card; step 7 has none): stage 3's and stage 5's are drawn from
    their stage's own scene (86 area 1, 89 area 4), not scene 02, and counted as the play: their sides the card's maps'
    (96 and 0 in 16:9), so the play began panning from them, and the forest's goons put in at its first frame past the
    cabinet's right edge (type EB at 0188) were taken as out of WIDE's sight (its right side 4 then) and stood in it
    as it opened (0 and 96). A ring drawn whole in a card's phase is the card's (ring card) till the stage draws its
    play; the play comes with its sides at once, its goons past the wider screen's edge.
  - Checked (the headless's --wide-hashes, each frame's WIDE picture, before and after, the eight stages' and the two
    bonus games' scripts, 16:9): stages 1, 4 (but stage 5's card at its end), 6, 7, 8 and the bonus games the same in
    every frame; stage 2 from the floor's break to the cards, and stages 3 and 5 from their cards on (their goons
    elsewhere: the play after them is another). --wide-check in 16:9: nothing put in in the sides' sight at stages 3's
    and 5's starts (16318's goon before); 20:9 as before (stage 1's at 1702 and 1917, at the edge). The frames seen in
    16:9 and 20:9 (build/krusty_work). WIDE OFF: stages 2, 3 and 5 the same state, video and sound hashes. Not
    checked on a device.
- **20-11-2026, INTRO > ATTRACT** (the players' issue 22, "Request: The attract Movie watchable within the intro
  option": the attract mode starts in the arcade game, not in the console's; Sebas: "y dps agregemos el 22", the way
  chosen here as he did not pick one: INTRO's page with STORY and ATTRACT): the menu's INTRO opens INTRO's page
  (menu.c: STORY, ATTRACT, BACK, with two lines under them saying what the chosen one is, as MR. PLOW MODE's). STORY
  is INTRO as before (modes.c), and its end comes back to the page at STORY (menu_after_intro; the menu left alone
  still plays it and comes back to the menu at INTRO). ATTRACT (console.c, console_attract_watch and
  attract_watch_frame): the cabinet's own attract mode, original_attract, which the console's attract never runs
  (its menus over the TV instead), from its opening as a game's end starts it (end_game: mode_step and the step's
  variables at 0), the menu's objects, sprites and song off, no menu over it (PAGE_ATTRACT: menu_overlay draws
  nothing), its sound as OPTIONS' DEMO SOUND has it (attract_sound_option, the cabinet's); A, Back or Start end it:
  the objects, sprites, layers and sound cleared, its steps at 0, the TV drawn again (tv_forget), the page at
  ATTRACT. The headless's page names (console_where) have the two new pages: without them a --video-dump of the
  page read past the table (a crash, found by the check below).
  - Checked (the console's headless, --actions, WIDE 16:9): INTRO's page at 740, ATTRACT picked at 770, the
    attract's opening, the title, Bart's, Homer's, Lisa's and Marge's pages, the demo on stage 1 (GAME OVER in its
    middle, as the arcade game's own attract has it: simpsons_game_headless's frames 2750 to 4500), the title and
    the score ranking, and round again; A at 6000: the page at ATTRACT (6010), BACK, the menu on the TV with its song
    (6080), then ARCADE and the character choice (6300) and the game from its story (6600). STORY from the page at
    760: the choice under black, the story, and INTRO's page at STORY from 2300. Not checked on a device.
- **20-11-2026, WIDE, stage 2's teacup ride** (a player's report, forwarded by Sebas: "If I am playing it in
  widescreen mode, the enemies inside the cups (when you get the hammer) are not showing up. Changing the aspect ratio
  to 4:3 fixes that"; two other players' videos showed it too): the ride's hub (list 1's type 0F,
  teacup_goon_ride_update, unseen) puts in its three arms and the six goons riding them (type 1A, kind 88), and takes
  itself off when its x is 1E0 or more (the board's right side, 0x61 past its edge). Its start puts it 40 on from where
  the spawn list puts it: 1D0 on the board (spawned at 190, due at the camera's 150 past it); in WIDE, due 48 pixels
  earlier as every spawn (spawn_stage_objects: the wider screen's right side), at 1C0, so at 200, and gone in its first
  frame (stage 2's script, 16:9: put in at 4588, flags 00 that frame; the camera never held, no arm, no rider). Its
  spawn is due as on the board now (due_as_on_board, list 1 type 0F; those after it in the list wait for it, as on
  the board), and put_in_sighted_spawns leaves it to the list. Not done the other way (the function replaced with its
  limit by WIDE's side, as krusty_balloon_fly_up): its definition is in src/ (objects/bank_entries.c), which changes
  in the decompilation.
  - Checked (stage 2's script): 16:9, the hub at 01D0 at 4661, its arms at 4662, the six goons from 4663 till 6111,
    the cups full and turning in the frames (4760 on), the goons leaping off to fight; 20:9, the hub at 01CE at 4458
    and its goons from 4460. --wide-hashes against the build before, 16:9: stage 2 the same till 4667, every other
    stage and both bonus games the same in every frame. WIDE OFF, stage 2: the same state, picture and sound hashes as
    before. Not checked on a device.
- **20-11-2026, the attract mode after the Konami logo** (the players' issue 23, "[Request] Have the attract mode
  play after the Konami logo ... (similar to TMNT)"; Sebas: "el 23 lo hicimos?"): the presentation at power-on left
  untouched is followed by the cabinet's own attract mode, as INTRO > ATTRACT plays it (console_attract_watch: its
  opening, the title, the characters' pages, the demo, the score ranking, round again), with no menu over it; A,
  Back or Start end it on the menu at ARCADE (attract_to_menu), not on INTRO's page. The presentation skipped with
  START or attack (presentation_skipped) goes to the menu as before, and so does a LAN game's power-on (no
  presentation). The menu left alone still plays INTRO (Sebas's choice, issue 8).
  - Checked (the console's headless, --actions): untouched, the presentation till about 560, the attract mode from
    then (its clouds and title at 600 and 700, the demo at 1400, the board's steps going on at 2900); A at 1500, the
    menu on the TV at 1510 at ARCADE; A at 200 (the presentation skipped), the menu at 400. Not checked on a device.
- **20-11-2026, Maggie in Bart's arms at the ending** (the players' issue 27, "Maggie's dress is still red when Bart
  picks her up after putting the diamond in Burns' mouth"; Sebas painted the shirt: "queres que te lo pinte a
  mano?"): in a game of Bart alone, he is the one who picks Maggie up after the diamond goes into Burns' mouth (in a
  four-player game Marge does), and his sprite carrying her (0870, 4 x 4 cells) has her onesie and his shirt in the
  same pen, three of its cells shared (0876, 0879, 087C). The video's recolouring takes keep masks
  (simpsons_video_recolouring's cell_keeps and keep_masks, laid out as the shade's): a cell's pixels drawn as the board
  draws them, neither recoloured nor in the shirt's shade; bart_shirt.c's maggie_carried has her 67 pixels there, from
  Sebas's painting of frame 32880 (the cabinet's colours with the shirt painted orange, local/issue27).
  - Checked (the console's headless, --arcade-start, stage 8's script with Bart alone): frame 32880 the same as the
    painting in every pixel; 32770 to 33060 (Bart carrying her) 41 pixels from the cabinet's colours, the shirt alone,
    where 108 were; the bend before (32750, 32760) Maggie light blue as before. The 25 scenarios identical. Not
    checked on a device.
