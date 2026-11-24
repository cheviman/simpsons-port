/**
 *    Name:        console.c
 *    Description: the console version's own versions of the game functions console/replaced.txt lists, the
 *                 players' controls on their characters' slots, START's coins and the cabinet's settings
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    The console version (The Simpsons) runs the arcade game's program with these in place of the game's own (whose
 *    definitions tools/decompile.py writes as CONSOLE_ORIGINAL(name): original_name here, for these to call), so the
 *    arcade build (The Simpsons Arcade Game) stays the program it is. In the attract mode the console's front end runs
 *    (attract): the Konami presentation at power-on (presentation.c), then the menus on the TV (tv.c, menu.c); ARCADE
 *    starts the game, whose start shows its portraits, where each player chooses his character (characters.c). As
 *    tmnt-port's console.c.
 *
 *    No coins at home: a player's START puts the coin and the attack in (console_controls), on the slot of the
 *    character he chose (each slot is a character: 1 Marge, 2 Homer, 3 Bart, 4 Lisa), and every box that would show
 *    INSERT COIN shows the game's own PRESS START (gauges.c). The cabinet's settings OPTIONS changes (the difficulty,
 *    the lives a credit gives, the attract mode's sound, the screen flipped) go into the EEPROM's settings, 68-7F,
 *    with their sum, before the boot reads them, and from OPTIONS into the work RAM's copy of them (59F0-) and the
 *    game's variables, as the boot's apply_settings takes them (symbols.txt: apply_settings 078A1A,
 *    eeprom_load_settings 05E00F, eeprom_write_sum 05EA57).
 *
 *    The lives and continues (guideline 10): ARCADE's game keeps the cabinet's lives (OPTIONS' LIVES a credit) and
 *    OPTIONS' CONTINUES (0 to 9, or a START a continue as long as one likes, UNLIMITED; MR. PLOW MODE's FREE PLAY has
 *    those always); a game of the console's own modes sets its own as it starts
 *    (console_set_mode_lives: MODE_LIVES a credit and MODE_CONTINUES continues, TMNT's Splinter Way's), each player's
 *    credits counted, and his START opens nothing once his continues are over. The Konami code's game is one
 *    (konami.c), with as many as one likes when its LIVES are INFINITE.
 */
#include "console_internal.h"

#include "game.h"
#include "simpsons/machine.h"

#include <stdio.h>
#include <string.h>

/* A join: the coin held 2 frames, then (its credit counted) the attack pressed 2 frames in every 8 until the slot
   plays (player_join takes a slot's credits when its player presses a button: ATTACK, JUMP or START, 0xB0), for at
   most JOIN_FRAMES frames; on the portraits a coin alone first (its character comes alive, characters.c), the attack
   once everyone has chosen */
#define JOIN_COIN_FRAMES    2
#define JOIN_ATTACK_FROM    5
#define JOIN_ATTACK_FRAMES  2
#define JOIN_ATTACK_EVERY   8
#define JOIN_FRAMES         240
#define LENT_CREDIT         1u         /* (start_game) */
#define SOUND_SILENCE       0x0000u    /* the sound program's command 00: everything silenced (game_start_clear) */

/* The EEPROM's settings (assets/eeprom.txt: 68-7F; the boot copies 68-7D into the work RAM at 59F0) */
#define EEPROM_DIFFICULTY   0x6Cu      /* 59F4 difficulty_option: 0-3 */
#define EEPROM_LIVES        0x6Du      /* 59F5 lives_option: the lives a credit gives, less one */
#define EEPROM_DEMO_SOUND   0x6Eu      /* 59F6 attract_sound_option */
#define EEPROM_SCREEN_FLIP  0x6Fu      /* 59F7 screen_flip_option */
#define EEPROM_SUM          0x7Eu      /* the sum of 00-7D, big endian (eeprom_write_sum) */

console_state console = { .last_presser = 0 };
int console_slot_of[CONSOLE_PLAYERS];    /* (-1 each from console_attach on) */

/* The factory's (assets/eeprom.txt): NORMAL, 2 lives, the attract mode's sound on, the screen the right way up */
console_settings_t console_settings = { .window_scale = 3, .pixels = PIXELS_SHARP, .volume = 5, .vsync = true,
	.wide = WIDE_AUTO, .game_difficulty = 1, .game_lives = 2, .game_continues = CONSOLE_CONTINUES_DEFAULT,
	.demo_sound = true, .upside_down = false, .bart_shirt = true, .stereo = true };    /* (STEREO at every start:
	   Sebas, "que siempre inicie en stereo"; OPTIONS > SOUND > SOUND makes it MONO till the game closes) */

const char *const console_pixel_names[PIXELS_KINDS] = { "sharp", "smooth", "bilinear", "crt" };
const char *const console_wide_names[WIDE_KINDS] = { "off", "16:9", "20:9", "16:10", "auto" };
const char *const console_difficulty_names[CONSOLE_DIFFICULTIES] = { "easy", "normal", "difficult", "very difficult" };

static bool arcade_test;    /* console_arcade_test: the arcade's attract mode and controls, for checks */

void console_arcade_test(void)
{
	arcade_test = true;
	console.booted = true;
	console.page = PAGE_MENU;
}

bool console_arcade_testing(void)
{
	return arcade_test;
}

/* Each slot's join: its frames since it started plus one (0: none; zero-initialized, no table of the executable's: a
   table of -1 and 0 words is a stretch the sprite ROM has too, tools/rom_in_executables.py), whether a coin goes in
   first, and whether the attack follows */
static struct
{
	int frames;
	bool coin;
	bool attack;
} joins[CONSOLE_PLAYERS];

static uint8_t controls_last[CONSOLE_PLAYERS];    /* each player's controls at the last frame */

/* The scenes a button skips (check_skip: the stories, the stages' title cards, the stages' and the bonus games' ends,
   the bonus games' instructions, the ending): no pause in them (Sebas: "en las cinematicas que no se pause"), and a
   START of a player in the game skips them as his attack does, SCENE_SKIP_FRAMES frames of it */
#define STEP_STORY_1        1u         /* game_step: the stories, */
#define STEP_STORY_2        5u
#define STEP_STORY_3        10u
#define STEP_STORY_4        14u
#define STEP_ENDING         16u        /* and the ending */
#define PHASE_STEPS         1u         /* mode_phase: the game's steps (game_run_step) */
#define SCENE_GAP           30u        /* the frames between a scene's parts (check_skip not called) still its */
#define SCENE_SKIP_FRAMES   3          /* (one let go, two pressed) */
static uint64_t scene_frame;           /* the board's frame check_skip last ran at, plus one (0: none yet) */
static uint8_t scene_skips[CONSOLE_PLAYERS];    /* each slot's frames of attack left for a START's skip */

/* The story before stage 1 skipped whole (Sebas: "te deja adelantar la cinematica pero el audio sigue corriendo
   normal y hasta que no se termine no te avanza"): it is the one scene of several parts, each with its own check_skip
   (story_1_street, story_1_redraw, story_1_rise, story_1_end), which ends that part alone at a button, after the
   part's own wait (40 frames), the story's sounds going on; one START took a second off its fifteen. Here the first
   skip takes the story with it: a START at once, the attack or the jump as the cabinet takes them (after the part's
   wait: the presses that chose the character skip nothing). Until the story's step ends the playing slots' attack is
   pressed SCENE_SKIP_PRESSED frames in SCENE_SKIP_EVERY, under black and silence and with the frames hurried
   (console_volume, console_hurried), as the Konami game's story before its stage (konami.c). The other scenes end
   whole at their one check_skip (steps 5, 10 and 14, "the stories", last a frame: theirs play in the next stage's
   opening). Not the intro's story (the menu left alone: its scenes are there to be seen), nor a LAN game's (its frames'
   pace is the network's) */
#define SCENE_SKIP_EVERY    8
#define SCENE_SKIP_PRESSED  2
#define SCENE_QUIET_FRAMES  3          /* the black and the silence kept after it: the story's last picture stays a
                                          frame, and its tune sounds on until the next step's first sound command
                                          takes its place, these frames later */
static uint8_t scene_skipped;          /* the story's step being skipped whole (0: none) */
static uint8_t scene_quiet;            /* the frames of black and silence left after a story skipped whole */

/* The name entry after a game (mode_phase 4, name_entry_phase): a column for each slot whose score made the table, each
   typing its initials with its own slot's controls, its entry (name_entry_records: 3842 + 4 x slot) done at bit 7. A
   player who continued as another character played two slots, and his controls are on the last one alone: the other
   column waited for controls nobody had (the players' issue 36). Here a player's controls go to the first of the slots
   he played this game whose entry is not done, left to right as the columns are, then to the next */
#define PHASE_NAME_ENTRY    4u
#define NAME_ENTRY_RECORDS  0x3842u
#define NAME_ENTRY_DONE     0x80u
static uint8_t slots_played[CONSOLE_PLAYERS];    /* each player's slots in this game, a bit each (from the choice on) */

static int name_entry_slot(int player, int slot)
{
	if (game_mode != GAME_MODE_GAME || mode_phase != PHASE_NAME_ENTRY)
		return slot;
	for (int played = 0; played < CONSOLE_PLAYERS; played++)
		if ((slots_played[player] & (1u << played)) && !(mem_read8((uint16_t)(NAME_ENTRY_RECORDS + 4u * played)) & NAME_ENTRY_DONE))
			return played;
	return slot;
}

void console_slots_played_forget(void)
{
	memset(slots_played, 0, sizeof(slots_played));
}

/* ---- the slots ---- */

/* The slot's record of the players' (player_variable_records, 07E821: four of 27 bytes, the slots' order) */
static const struct player *slot_record(int slot)
{
	return PLAYER(player_variable_records_ADDRESS + (uint32_t)slot * (uint32_t)sizeof(struct player));
}

bool console_slot_plays(int slot)
{
	return slot >= 0 && slot < CONSOLE_PLAYERS && (players_playing & slot_record(slot)->bit) != 0u;
}

bool console_slot_joining(int slot)
{
	return slot >= 0 && slot < CONSOLE_PLAYERS && joins[slot].frames > 0 && joins[slot].attack;
}

static uint8_t slot_lives(int slot)
{
	return RAM8(slot_record(slot)->lives);
}

bool console_join(int slot, bool coin, bool attack)
{
	if (slot < 0 || slot >= CONSOLE_PLAYERS || joins[slot].frames > 0)
		return false;
	joins[slot].frames = 1;
	joins[slot].coin = coin;
	joins[slot].attack = attack;
	return true;
}

bool console_join_going_on(int slot)
{
	return slot >= 0 && slot < CONSOLE_PLAYERS && joins[slot].frames > 0;
}

/* Whether the game shows the story whose skip takes all of it (scene_skipped) */
static bool story_skipped_whole(void)
{
	return console.page == PAGE_GAME && game_mode == GAME_MODE_GAME && mode_phase == PHASE_STEPS
		&& game_step == STEP_STORY_1 && console.mode != MODE_INTRO && !console_lan_playing();
}

bool console_scene_skipping(void)
{
	return scene_skipped != 0u && game_step == scene_skipped && story_skipped_whole();
}

bool console_scene_quiet(void)
{
	return console_scene_skipping() || scene_quiet > 0u;
}

/* A START in a game: a player whose character plays (or is coming in) pauses it (the port's); any other chooses his
   character, then comes in on it (characters.c) */
static void start_pressed(int player)
{
	const int slot = console_slot_of[player];

	if (game_mode != GAME_MODE_GAME || console.quitting_game || !console_player_may_join(player))
		return;    /* (FRANK GRIMES LIFE: its one player alone) */
	if (slot >= 0 && (console_slot_plays(slot) || console_slot_joining(slot)))
	{
		if (console_slot_plays(slot) && story_skipped_whole())
			scene_skipped = game_step;    /* (the story before stage 1: all of it) */
		else if (console_slot_plays(slot) && console_in_scene())
			scene_skips[slot] = SCENE_SKIP_FRAMES;    /* (a scene: skipped as by his attack, check_skip) */
		return;
	}
	character_change_open(player);
}

void console_controls(const uint8_t pressed[CONSOLE_PLAYERS], uint8_t starts)
{
	uint8_t slots[CONSOLE_PLAYERS] = { 0, 0, 0, 0 };
	uint8_t fresh_of[CONSOLE_PLAYERS];
	uint8_t coins = 0;

	if (arcade_test)
	{
		/* (the board's inputs as the script left them: its coins, and each player's buttons on his own slot) */
		console.page = game_mode == GAME_MODE_GAME ? PAGE_GAME : PAGE_MENU;
		for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
			simpsons_board.players[slot] = (uint8_t)~pressed[slot];
		return;
	}
	for (int player = 0; player < CONSOLE_PLAYERS; player++)
	{
		const uint8_t fresh = (uint8_t)(pressed[player] & ~controls_last[player]);

		fresh_of[player] = fresh;
		if ((starts & (1u << player)) || (fresh & (SIMPSONS_ATTACK | SIMPSONS_JUMP)))
			console.last_presser = player;
		controls_last[player] = pressed[player];
		/* out of a game, a player's controls newly pressed are the menus' actions too (whatever the port reads into
		   them: its keys and pads as each player's), besides the ones the port sends (console_actions) */
		if (console.page != PAGE_GAME)
		{
			const uint32_t actions = ((fresh & SIMPSONS_UP) ? ACTION_BIT(ACTION_UP) : 0u)
				| ((fresh & SIMPSONS_DOWN) ? ACTION_BIT(ACTION_DOWN) : 0u)
				| ((fresh & SIMPSONS_LEFT) ? ACTION_BIT(ACTION_LEFT) : 0u)
				| ((fresh & SIMPSONS_RIGHT) ? ACTION_BIT(ACTION_RIGHT) : 0u)
				| ((fresh & SIMPSONS_ATTACK) ? ACTION_BIT(ACTION_ACCEPT) : 0u)
				| ((fresh & SIMPSONS_JUMP) ? ACTION_BIT(ACTION_BACK) : 0u);

			console.pressed |= actions;
			console.player_pressed[player] |= actions | ((starts & (1u << player)) ? ACTION_BIT(ACTION_START) : 0u);
		}
	}
	if (console.page != PAGE_GAME && starts)
		console.pressed |= ACTION_BIT(ACTION_START);    /* (any player's START, in the menus) */
	if (console.page != PAGE_GAME)
		memset(console.character_change, 0, sizeof(console.character_change));
	else
		for (int player = 0; player < CONSOLE_PLAYERS; player++)
		{
			const bool start = (starts & (1u << player)) != 0;

			if (console.character_change[player])
				character_change_frame(player, fresh_of[player], start);
			else if (start)
				start_pressed(player);
			if (console_slot_of[player] >= 0)
				slots_played[player] |= (uint8_t)(1u << console_slot_of[player]);
			if (console_slot_of[player] >= 0 && !console.character_change[player])
				slots[name_entry_slot(player, console_slot_of[player])] |= pressed[player];    /* (choosing a character:
					none, so that his buttons do not hurry the continue's countdown; the name entry: name_entry_slot) */
		}
	if (boss_rush_crossing())
		memset(slots, 0, sizeof(slots));    /* (BOSS RUSH's crossing: its own controls, modes.c) */
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
		slots[slot] |= konami_presses(slot) | modes_presses(slot);    /* (the Konami game: the story before stage 1
			skipped; BOSS RUSH: a stage crossed to its boss) */
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
		if (scene_skips[slot] > 0u)
		{
			/* (a START's skip of a scene: the attack let go at its first frame, for its press to be new) */
			if (scene_skips[slot] == SCENE_SKIP_FRAMES)
				slots[slot] &= (uint8_t)~(SIMPSONS_ATTACK | SIMPSONS_JUMP);
			else
				slots[slot] |= SIMPSONS_ATTACK;
			scene_skips[slot]--;
		}
	if (scene_quiet > 0u)
		scene_quiet--;
	if (!console_scene_skipping())
	{
		if (scene_skipped != 0u && console.page == PAGE_GAME && game_mode == GAME_MODE_GAME)
			scene_quiet = SCENE_QUIET_FRAMES;    /* (the story over: its tune's last frames unheard) */
		scene_skipped = 0u;    /* (or its game over) */
	}
	else
		for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
			if (console_slot_plays(slot))    /* (his own controls not on the board: a button held is no new press) */
				slots[slot] = simpsons_board.frame % SCENE_SKIP_EVERY < SCENE_SKIP_PRESSED ? SIMPSONS_ATTACK : 0u;
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
	{
		const int frame = joins[slot].frames - 1;

		if (frame < 0)
			continue;
		if (joins[slot].coin && frame < JOIN_COIN_FRAMES)
			coins |= (uint8_t)(SIMPSONS_COIN1 << slot);
		if (!joins[slot].attack)
		{
			if (frame >= JOIN_ATTACK_FROM)
			{
				joins[slot].frames = 0;    /* (a coin alone: done) */
				continue;
			}
		}
		else if (frame < JOIN_ATTACK_FROM)
			slots[slot] &= (uint8_t)~(SIMPSONS_ATTACK | SIMPSONS_JUMP);    /* (let go, for the attack's press to be new) */
		else if (console_slot_plays(slot))
		{
			joins[slot].frames = 0;    /* (in: the player's own controls from now on) */
			continue;
		}
		else if ((frame - JOIN_ATTACK_FROM) % JOIN_ATTACK_EVERY < JOIN_ATTACK_FRAMES)
			slots[slot] |= SIMPSONS_ATTACK;
		else
			slots[slot] &= (uint8_t)~SIMPSONS_ATTACK;
		if (++joins[slot].frames > JOIN_FRAMES)
			joins[slot].frames = 0;
	}
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
		simpsons_board.players[slot] = (uint8_t)~slots[slot];
	simpsons_board.coins = (uint8_t)~coins;
}

void console_actions(uint32_t pressed)
{
	console.pressed |= pressed;
	console.player_pressed[0] |= pressed;    /* (the port's fixed keys: player 1's, as his keys are by default) */
}

void console_controls_held(const uint8_t pressed[CONSOLE_PLAYERS])
{
	for (int player = 0; player < CONSOLE_PLAYERS; player++)
		controls_last[player] = pressed[player];
}

/* A game over and the attract mode again: the players off their slots and back to their own characters, no joins
   going on, no choices open, no credits counted, the cabinet's lives */
static void slots_game_over(void)
{
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
	{
		console_slot_of[slot] = -1;
		joins[slot].frames = 0;
		console.character_change[slot] = 0;
		console.credits_put[slot] = 0;
		scene_skips[slot] = 0u;
	}
	console_characters_reset();
	console_set_mode_lives(LIVES_CABINET, CONTINUES_UNLIMITED);
}

/* ---- the lives and continues ---- */

void console_set_mode_lives(int lives, int continues)
{
	console.mode_lives = lives;
	console.mode_continues = continues;
	/* (OPTIONS' as apply_settings takes them: lives_option plus one) */
	lives_per_credit = (uint8_t)(lives != LIVES_CABINET ? lives : console_settings.game_lives);
}

bool console_credit_allowed(int player)
{
	return player >= 0 && player < CONSOLE_PLAYERS && (console.mode_continues == CONTINUES_UNLIMITED
		|| console.credits_put[player] <= console.mode_continues);
}

void console_credit_put(int player)
{
	if (player >= 0 && player < CONSOLE_PLAYERS)
		console.credits_put[player]++;
}

/* A game started at the attract mode's next frame, the choice on its portraits: its lives and continues, its first
   step (konami.c sets it once stage 1 is played) */
void console_start_game(int player, int step, int lives, int continues)
{
	console_settings_into_game();    /* (OPTIONS' settings, as the game starts) */
	slots_game_over();
	console_set_mode_lives(lives, continues);
	console.start_step = step;
	console.start_pending = step != STEP_FIRST_STAGE;
	console.chooser = player;
	console.page = PAGE_STARTING;
	console.choice = 0;
	console.page_frames = 0;
	modes_game_starting();    /* (MR. PLOW MODE's: modes.c) */
}

/* ARCADE: the cabinet's game, its lives and continues OPTIONS' (Sebas: "en arcade, agrega en options el limite de
   continues"; MR. PLOW MODE's FREE PLAY has as many as one likes), from its beginning */
void console_start_arcade(int player)
{
	console.konami = false;
	console.mode = MODE_NONE;
	console_start_game(player, STEP_FIRST_STAGE, LIVES_CABINET,
		console_settings.game_continues == CONSOLE_CONTINUES_UNLIMITED ? CONTINUES_UNLIMITED
		: console_settings.game_continues);
}

/* The attract mode's own start of a game, with no player in it yet: it starts the game when a slot has a credit, so
   one is lent to the first slot for its frame (the factory's EEPROM has a credit for each slot of its own,
   separate_coin_slots: a slot's credit would bring its character in on the portraits) and taken back; no slot's
   credit is left, and nobody has joined (players_joined is the credit count's high byte, 0) */
static void start_game(uint16_t u)
{
	const uint16_t kept_credits = credits, kept_slot_credits = slot_credits;

	credits = LENT_CREDIT;
	slot_credits = LENT_CREDIT;
	original_attract(u);
	credits = kept_credits;
	slot_credits = kept_slot_credits;
}

/* ---- the cabinet's settings ---- */

void console_settings_into_eeprom(void)
{
	uint8_t *data = simpsons_board.eeprom.data;
	uint16_t sum = 0;

	data[EEPROM_DIFFICULTY] = (uint8_t)console_settings.game_difficulty;
	data[EEPROM_LIVES] = (uint8_t)(console_settings.game_lives - 1);
	data[EEPROM_DEMO_SOUND] = console_settings.demo_sound ? 1u : 0u;
	data[EEPROM_SCREEN_FLIP] = console_settings.upside_down ? 1u : 0u;
	for (unsigned address = 0; address < EEPROM_SUM; address++)
		sum = (uint16_t)(sum + data[address]);
	data[EEPROM_SUM] = (uint8_t)(sum >> 8);
	data[EEPROM_SUM + 1u] = (uint8_t)sum;
}

/* (as the boot takes them: eeprom_load_settings copies the EEPROM's 68-7D to 59F0-, apply_settings sets the game's
   variables and the chips' flip from them) */
void console_settings_into_game(void)
{
	console_settings_into_eeprom();
	difficulty_option = (uint8_t)console_settings.game_difficulty;
	lives_option = (uint8_t)(console_settings.game_lives - 1);
	attract_sound_option = console_settings.demo_sound ? 1u : 0u;
	screen_flip_option = console_settings.upside_down ? 1u : 0u;
	apply_settings();
}

/* ---- the game's functions in the console's versions ---- */

/* The game ended where it is, as its own end takes it back to the attract mode (game_finish, end_game): the sprites
   and the layers cleared, the sound silenced, the scores and the game's variables cleared, no credits nor lives left */
static void end_game_now(void)
{
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
		RAM8(slot_record(slot)->lives) = 0u;
	io_write8(tile_chip_scroll_control, 0u);
	clear_sprites();
	clear_tile_layers();
	queue_sound_always(SOUND_SILENCE);
	reset_for_game();
	credits = 0u;
	slot_credits = 0u;
	slot_2_credits = 0u;
	slot_3_credits = 0u;
	slot_4_credits = 0u;
	players_playing = 0u;
	players_going_on = 0u;
	mode_step = 0u;
	mode_phase = 0u;
	game_step = 0u;
	step_phase = 0u;
	step_subphase = 0u;
	step_countdown = 0u;
	game_mode = GAME_MODE_ATTRACT;
	marge_state = 0u;
	homer_state = 0u;
	bart_state = 0u;
	lisa_state = 0u;
}

void console_end_game(void)
{
	console.quitting_game = true;
}

/* The game mode's frame: the game's own, the character choice over its portraits, or its end where the pause's QUIT
   GAME or RESTART (or the choice left) asked for it */
void game_mode_frame(uint16_t u)
{
	if (console.quitting_game)
	{
		console.quitting_game = false;
		end_game_now();
		return;
	}
	if (console.page == PAGE_CHOICE)
	{
		uint32_t player_pressed[CONSOLE_PLAYERS];

		memcpy(player_pressed, console.player_pressed, sizeof(player_pressed));
		memset(console.player_pressed, 0, sizeof(console.player_pressed));
		console.pressed = 0;
		choice_frame(player_pressed);
		if (console.quitting_game)
			return;    /* (the choice left: the game ends at its next frame) */
	}
	konami_game_frame();    /* (the Konami game: its stage, its lives and health) */
	if (modes_game_frame())
		return;    /* (MR. PLOW MODE's game over: it ends at its next frame) */
	same_character_marks();
	wide_screen_sides();    /* (the game's screen limits by WIDE's sides: wide.c) */
	original_game_mode_frame(u);
	if (console.page == PAGE_CHOICE)
		same_character_portraits();
}

/* The cabinet's own attract mode (original_attract: its opening, the title, the demo, the score ranking, round
   again) from its opening, as a game's end starts it (end_game: its steps at 0), the menu's objects and its song off;
   the attract's sound as OPTIONS' DEMO SOUND has it, as on the cabinet. After the presentation at power-on left
   untouched (the players' issue 23, "Have the attract mode play after the Konami logo", as tmnt-port), and after the
   intro's story the menu left alone plays (the players' issue 22, "The attract Movie watchable"; Sebas: "si no hacés
   nada ... el juego arranque solo como pasa con el arcade, el modo demo"). Any button ends it (Sebas: "si apretás
   algo, que lo saque"): the screen and the sound cleared, the TV drawn again (tv.c), the menu */
static void attract_watch(void)
{
	uint16_t place = 0, sprite_place = 0;

	vector_clear_objects(&place, &sprite_place);
	clear_sprites();
	queue_sound_always(SOUND_SILENCE);
	mode_step = 0u;
	mode_phase = 0u;
	game_step = 0u;
	step_phase = 0u;
	step_subphase = 0u;
	step_countdown = 0u;
	console.page = PAGE_ATTRACT;
	console.page_frames = 0;
}

static bool attract_watch_frame(uint16_t u, uint32_t pressed)
{
	uint16_t place = 0, sprite_place = 0;

	if (!pressed)
	{
		console.page_frames++;
		original_attract(u);
		return true;
	}
	vector_clear_objects(&place, &sprite_place);
	clear_sprites();
	clear_tile_layers();
	io_write8(tile_chip_scroll_control, 0u);
	queue_sound_always(SOUND_SILENCE);
	mode_step = 0u;
	mode_phase = 0u;
	game_step = 0u;
	step_phase = 0u;
	step_subphase = 0u;
	step_countdown = 0u;
	tv_forget();
	console.page = PAGE_MENU;
	console.choice = 0;
	console.page_frames = 0;
	return false;
}

/* The attract mode's frame: the presentation at power-on, the cabinet's attract mode after it when it was left
   untouched, then the menus on the TV; ARCADE's coin, and the attract's own start of the game once its credit has
   gone in */
void attract(uint16_t u)
{
	uint32_t pressed = console.pressed;
	uint32_t player_pressed[CONSOLE_PLAYERS];

	memcpy(player_pressed, console.player_pressed, sizeof(player_pressed));
	memset(console.player_pressed, 0, sizeof(console.player_pressed));
	console.pressed = 0;
	console.booted = true;
	same_character_marks();    /* (the game's own marks out of a game) */
	if (arcade_test)
	{
		original_attract(u);    /* (the arcade's own: a script's coins start its game) */
		return;
	}
	if (console.page == PAGE_GAME || console.page == PAGE_CHOICE)
	{
		/* (back from a game: the menu, the actions pressed during the game let go; after the pause's RESTART, the
		   same game started again, each player on the character he played) */
		const bool restarting = console.restarting;
		const int lives = console.mode_lives, continues = console.mode_continues, mode = console.mode;

		slots_game_over();
		if (!restarting)
		{
			console.konami = false;    /* (RESTART: the same game again, at its step, its lives and continues) */
			console.mode = MODE_NONE;
		}
		tv_forget();
		console.page = PAGE_MENU;
		console.choice = 0;
		if (console_lan_playing())
			console_lan_game_over();    /* (a LAN game's game over: this device's own settings, the page) */
		if (console_lan_back_to_page())
			console.page = PAGE_LAN;    /* (a LAN game over, left, or its host gone: MULTIPLAYER again) */
		else if (!restarting && mode == MODE_INTRO)
		{
			if (intro_played_through())
				attract_watch();    /* (the intro's story played whole: the cabinet's attract mode; a button, the
				                       menu) */
		}
		else if (!restarting && mode != MODE_NONE)
		{
			console.page = PAGE_MR_PLOW;    /* (a mode's game over: MR. PLOW MODE's page at its line, its lines in */
			console.choice = mode - MODE_FREE_PLAY;    /* the modes' order) */
		}
		console.page_frames = 0;
		pressed = 0;
		memset(player_pressed, 0, sizeof(player_pressed));
		if (restarting)
		{
			console_start_game(console.chooser, console.start_step, lives, continues);
			console.restarting = true;    /* (the choice picks again at once) */
		}
	}
	if (console.lan_start_due)
	{
		/* (a LAN game, the boot over: ARCADE's game for the host, the room's first device, on every device alike) */
		console.lan_start_due = false;
		console_start_arcade(0);
		pressed = 0;
		memset(player_pressed, 0, sizeof(player_pressed));
	}
	if (console.page == PAGE_PRESENTATION)
	{
		if (presentation_frame(pressed))
			return;
		console.page = PAGE_MENU;
		console.choice = 0;
		console.page_frames = 0;
		pressed = 0;
		memset(player_pressed, 0, sizeof(player_pressed));
		if (!presentation_skipped() && !console.lan_start_due)
		{
			attract_watch();    /* (left untouched: the cabinet's attract mode, the players' issue 23) */
		}
	}
	if (console.page == PAGE_ATTRACT)
	{
		if (attract_watch_frame(u, pressed))
			return;
		pressed = 0;    /* (the button that ended it picks nothing on the menu) */
		memset(player_pressed, 0, sizeof(player_pressed));
	}
	menu_frame(pressed, player_pressed);
	if (console.page == PAGE_STARTING)
	{
		start_game(u);
		if (game_mode != GAME_MODE_ATTRACT)
		{
			tv_forget();
			choice_open();
		}
		return;
	}
	tv_frame();
}

/* ---- MULTIPLAYER: the console as at power-on ---- */

void console_lan_power_on(void)
{
	const console_state fresh = { .last_presser = 0 };

	console = fresh;
	console.mode_continues = CONTINUES_UNLIMITED;    /* (console_attach's) */
	console.infinite_lives = true;
	console.repeats = true;
	console.page = PAGE_MENU;    /* (no presentation: black over the boot, then the game's start) */
	console.lan_start_due = true;
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
	{
		console_slot_of[slot] = -1;
		joins[slot].frames = 0;
		joins[slot].coin = false;
		joins[slot].attack = false;
		controls_last[slot] = 0;
		scene_skips[slot] = 0u;
	}
	scene_frame = 0u;
	scene_skipped = 0u;
	scene_quiet = 0u;
	arcade_test = false;
	console_slots_played_forget();
	console_characters_reset();
	characters_power_on();
	wide_power_on();
	modes_game_starting();
	tv_forget();
	console_settings_into_eeprom();    /* (the host's, over the factory's the board's reset put) */
	console_scores_lan(true);
	console_bart_shirt_apply();    /* (the video's recolouring, which the video's reset let go) */
}

/* ---- the port ---- */

bool console_attach(const char *folder)
{
	for (int player = 0; player < CONSOLE_PLAYERS; player++)
		console_slot_of[player] = -1;
	console_characters_reset();
	console.mode_continues = CONTINUES_UNLIMITED;
	/* (the Konami code's page at first: LIVES INFINITE, REPEAT CHARACTERS ON; set here, not in the data, as
	   tmnt-port's) */
	console.infinite_lives = true;
	console.repeats = true;
	console_settings_into_eeprom();    /* (before the boot reads them) */
	console_bart_shirt_apply();
	return draw_load_font(folder);
}

void console_overlay(console_picture *frame)
{
	if (arcade_test)
	{
		wide_picture_sides(frame);    /* (the arcade's screens as they are, WIDE's sides) */
		wide_hud_closed(frame);
		return;
	}
	if (!console.booted)
	{
		/* (the board's boot, its RAM and ROM checks, under black) */
		draw_fill(frame, PICTURE_FIRST(frame), 0, PICTURE_END(frame) - PICTURE_FIRST(frame), SIMPSONS_VIDEO_HEIGHT,
			0x000000u);
		return;
	}
	if (console.page == PAGE_PRESENTATION)
	{
		presentation_overlay(frame);
		return;
	}
	wide_picture_sides(frame);    /* (WIDE: a picture's sides, black or its backdrop's colour) */
	wide_hud_closed(frame);    /* (WIDE: Lisa's box closed at its right) */
	menu_overlay(frame);
	choice_overlay(frame);
	character_change_overlay(frame);
	if (console_scene_quiet())    /* (a story skipped whole: black till its step ends, and over the picture it left
	                                 until the next step draws its own) */
		draw_fill(frame, PICTURE_FIRST(frame), 0, PICTURE_END(frame) - PICTURE_FIRST(frame), SIMPSONS_VIDEO_HEIGHT,
			0x000000u);
	konami_overlay(frame);
	modes_overlay(frame);    /* (BOSS RUSH's crossing: its card on black) */
	if (console_lan_waiting())
	{
		char text[40];

		snprintf(text, sizeof(text), "WAITING FOR %s", console_lan_waiting());
		draw_game_letters_centred(frame, SIMPSONS_VIDEO_HEIGHT / 2, 1, 0xFFD840u, text);
	}
}

bool console_quit_requested(void)
{
	return console.quit;
}

/* The game's check_skip (game/skip.c): the frame a scene waits for a button at */
void check_skip(uint16_t wait, uint16_t *p_presses_addresses, uint32_t *p_skipped)
{
	scene_frame = simpsons_board.frame + 1u;
	original_check_skip(wait, p_presses_addresses, p_skipped);
	if (*p_skipped != 0u && story_skipped_whole())
		scene_skipped = game_step;    /* (the story's part skipped by a button: the rest of it too) */
}

bool console_in_scene(void)
{
	if (game_mode != GAME_MODE_GAME || mode_phase != PHASE_STEPS)
		return false;
	if (game_step == STEP_STORY_1 || game_step == STEP_STORY_2 || game_step == STEP_STORY_3
		|| game_step == STEP_STORY_4 || game_step == STEP_ENDING)
		return true;    /* (the stories and the ending, all of them: their parts before a button skips them too) */
	return scene_frame != 0u && simpsons_board.frame < scene_frame + SCENE_GAP;
}

bool console_can_pause(void)
{
	/* (the character choice too, Sebas: "en el menu de elegir personaje, no va a la pausa": its QUIT GAME back to the
	   menu; not in a scene, "en las cinematicas que no se pause") */
	return (console.page == PAGE_GAME || console.page == PAGE_CHOICE) && game_mode == GAME_MODE_GAME
		&& !console.quitting_game && !(console.page == PAGE_GAME && console_in_scene());
}

/* Whether a START of 'player' pauses the game: his character plays, or is coming in (a START of one who does not
   play chooses his character, start_pressed) */
bool console_start_pauses(int player)
{
	const int slot = player >= 0 && player < CONSOLE_PLAYERS ? console_slot_of[player] : -1;

	return slot >= 0 && console_can_pause() && !console.character_change[player]
		&& (console_slot_plays(slot) || console_slot_joining(slot));
}

void console_quit_game(void)
{
	console.restarting = false;
	console.quitting_game = true;
}

/* RESTART: the same game again, every player on the character he plays (those who came in during the game too) */
void console_restart_game(void)
{
	for (int player = 0; player < CONSOLE_PLAYERS; player++)
	{
		const int slot = console_slot_of[player];

		console.choice_character[player] = slot >= 0 && (console_slot_plays(slot) || console_slot_joining(slot))
			? console_slot_character[slot] : -1;
	}
	console.quitting_game = true;
	console.restarting = true;
}

void console_describe(char *text, size_t size)
{
	static const char *const pages[] = { "presentation", "menu", "mr. plow mode", "options", "video", "controls",
		"starting", "choice", "game", "lan", "lan name", "lan rooms", "lan room", "konami", "attract", "sound" };
	char konami[160] = "";
	const uint8_t *eeprom = simpsons_board.eeprom.data;

	snprintf(text, size, "page %s, game_mode %u, mode_phase %u, game_step %u, slots %d %d %d %d, choice %d%s %d%s %d%s "
		"%d%s, change %d %d %d %d, playing %02X, lives %u %u %u %u, credits %u; eeprom 6C-6F %02X %02X %02X %02X sum "
		"%02X%02X; difficulty_setting %u, lives_per_credit %u", pages[console.page], (unsigned)game_mode,
		(unsigned)mode_phase, (unsigned)game_step, console_slot_of[0], console_slot_of[1], console_slot_of[2],
		console_slot_of[3], console.choice_character[0], console.choice_picked[0] ? "*" : "",
		console.choice_character[1], console.choice_picked[1] ? "*" : "", console.choice_character[2],
		console.choice_picked[2] ? "*" : "", console.choice_character[3], console.choice_picked[3] ? "*" : "",
		console.character_change[0] - 1, console.character_change[1] - 1, console.character_change[2] - 1,
		console.character_change[3] - 1, (unsigned)players_playing, (unsigned)slot_lives(0), (unsigned)slot_lives(1),
		(unsigned)slot_lives(2), (unsigned)slot_lives(3), (unsigned)credits, eeprom[EEPROM_DIFFICULTY],
		eeprom[EEPROM_LIVES], eeprom[EEPROM_DEMO_SOUND], eeprom[EEPROM_SCREEN_FLIP], eeprom[EEPROM_SUM],
		eeprom[EEPROM_SUM + 1u], (unsigned)difficulty_setting, (unsigned)lives_per_credit);
	if (console.mode != MODE_NONE)
	{
		const size_t used = strlen(text);

		snprintf(text + used, size > used ? size - used : 0, "; %s", modes_describe());
	}
	if (console.konami)
		snprintf(konami, sizeof(konami), "; konami stage %s%s%s%s%s", konami_stage_name(console.konami_stage),
			console.start_pending ? " (pending)" : "", console.infinite_lives ? ", infinite lives" : "",
			console.infinite_health ? ", infinite health" : "", console.repeats ? ", repeats" : "");
	{
		const size_t used = strlen(text);

		snprintf(text + used, size > used ? size - used : 0, "; characters %d %d %d %d, credits %d %d %d %d, "
			"continues %d%s; step_phase %u; vitality %02X %02X %02X %02X; x %04X %04X %04X %04X, flags %02X %02X %02X %02X, kinds %02X %02X %02X %02X, teamed %02X %02X %02X %02X, camera %04X",
			console_slot_character[0], console_slot_character[1], console_slot_character[2], console_slot_character[3],
			console.credits_put[0], console.credits_put[1], console.credits_put[2], console.credits_put[3],
			console.mode_continues, konami, (unsigned)step_phase, (unsigned)OBJECT(OBJECT_ADDRESS(0))->vitality,
			(unsigned)OBJECT(OBJECT_ADDRESS(2))->vitality, (unsigned)OBJECT(OBJECT_ADDRESS(4))->vitality,
			(unsigned)OBJECT(OBJECT_ADDRESS(6))->vitality, (unsigned)OBJECT(OBJECT_ADDRESS(0))->x,
			(unsigned)OBJECT(OBJECT_ADDRESS(2))->x, (unsigned)OBJECT(OBJECT_ADDRESS(4))->x,
			(unsigned)OBJECT(OBJECT_ADDRESS(6))->x, (unsigned)OBJECT(OBJECT_ADDRESS(0))->flags,
			(unsigned)OBJECT(OBJECT_ADDRESS(2))->flags, (unsigned)OBJECT(OBJECT_ADDRESS(4))->flags,
			(unsigned)OBJECT(OBJECT_ADDRESS(6))->flags, (unsigned)OBJECT(OBJECT_ADDRESS(0))->kind,
			(unsigned)OBJECT(OBJECT_ADDRESS(2))->kind, (unsigned)OBJECT(OBJECT_ADDRESS(4))->kind,
			(unsigned)OBJECT(OBJECT_ADDRESS(6))->kind, (unsigned)OBJECT(OBJECT_ADDRESS(0))->player.teamed,
			(unsigned)OBJECT(OBJECT_ADDRESS(2))->player.teamed, (unsigned)OBJECT(OBJECT_ADDRESS(4))->player.teamed,
			(unsigned)OBJECT(OBJECT_ADDRESS(6))->player.teamed, (unsigned)camera_x);
	}
	{
		/* (the players' boxes' colours, pen 11 of the palettes 20-23, which a player's hurt flash animates) */
		size_t at = strlen(text);

		snprintf(text + at, size > at ? size - at : 0, "; boxes");
		for (unsigned palette = 0x20u; palette < 0x24u; palette++)
		{
			at = strlen(text);
			snprintf(text + at, size > at ? size - at : 0, " %02X%02X",
				(unsigned)simpsons_board.palette[(palette * 16u + 11u) * 2u],
				(unsigned)simpsons_board.palette[(palette * 16u + 11u) * 2u + 1u]);
		}
	}
	{
		/* (the high score table's first five: initials and the character, high_scores' records of 8 bytes) */
		size_t used = strlen(text);

		snprintf(text + used, size > used ? size - used : 0, "; table");
		for (unsigned entry = 0; entry < 5u; entry++)
		{
			char initials[4];

			for (unsigned letter = 0; letter < 3u; letter++)
			{
				const uint8_t value = RAM8(high_scores_ADDRESS + entry * 8u + letter);

				initials[letter] = value >= 0x20u && value < 0x7Fu ? (char)value : '?';
			}
			initials[3] = 0;
			used = strlen(text);
			snprintf(text + used, size > used ? size - used : 0, " %s%u", initials,
				(unsigned)RAM8(high_scores_ADDRESS + entry * 8u + 5u));
		}
	}
}

/* A save state's part (states.c): the joins under way and each player's controls at the last frame */
void console_joins_state(simpsons_state *state)
{
	simpsons_state_part(state, "joins");
	simpsons_state_bytes(state, joins, sizeof(joins));
	simpsons_state_bytes(state, controls_last, sizeof(controls_last));
}
