/**
 *    Name:        modes.c
 *    Description: MR. PLOW MODE's modes of their own: BOSS RUSH (the bosses one after another), BONUS GAMES (the two
 *                 bonus stages in a row) and FRANK GRIMES LIFE (one player against what four would have)
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    MR. PLOW MODE's page (menu.c; the heading for now, Sebas: "le ponemos de titulo mr plow mode, y dps lo cambiamos")
 *    starts a game of the console's own modes (console/PLAN.md, guidelines 10, 13, 14 and 16; tmnt-port's
 *    console/modes.c): the character choice on the game's portraits as ARCADE's, then the game with MODE_LIVES a credit
 *    and MODE_CONTINUES continues after each player's first; with the Konami code entered on the menu, its page's LIVES
 *    INFINITE, HEALTH INFINITE and REPEAT CHARACTERS hold in the modes too (konami.c). A mode's game over (its
 *    continues over) and its end are the game's own: GAME OVER, the high scores' name entry when a score is in the
 *    table, the attract mode, and the menu at MR. PLOW MODE's line of the mode.
 *
 *    BOSS RUSH (guideline 13): from stage 1, each stage crossed to its boss by the game's own play, out of sight and as
 *    fast as the machine goes (console_hurried: the frames not waited for, a picture in a few; no sound), the card
 *    "BOSS RUSH", the boss's number, its stage and its name on black; as tmnt-port's crossing. While a stage is
 *    crossed the players walk right on their own (and down and up the street in turn, attacking now and then), their
 *    health and lives kept up, and every object of the enemies' list (4F80, eight of them: the goons, the zombies, the
 *    stage's other bosses) but the stage's boss goes out of the game as the game takes an enemy off past the screen's
 *    edge (enemy_remove_off_screen: its slot free, its target let go), the frame after it came; the stage's spawns and
 *    events go on as they are (its doors, its scenery, the camera's stops), and the camera goes on as the game moves
 *    it, its stops let go once no enemy holds them. The rank's time (rank_frames, rank_time_step: the time a scene has
 *    been played raises the enemies' speed and the bosses' moves, rank_update) is kept as it was through the crossing.
 *    The boss's object in the enemies' list ends the crossing: the players play from his entrance on. Once he is
 *    beaten (the stage's play over: its step_phase past the play's) the stage's end is crossed too, and the steps with
 *    no boss (the bonus stages, the stories) are passed as the program goes on to a step (game_step set at its first
 *    phase, A75E): stage 1's wrestler, stage 2's Krusty balloon, stage 3's gangsters (its forest crossed, the
 *    cemetery's climb, they come once the enemies are gone), stage 4's football player, stage 5's bear, stage 6's
 *    Dreamland boss, stage 7's kabuki warrior, stage 8's Smithers and, after the game's own scene, Mr. Burns' robot;
 *    then the game's own end of stage 8 and its ending, the high scores, the menu.
 *
 *    BONUS GAMES (guideline 14): the two bonus stages in a row, for one to four players (the others' characters play
 *    them as the game plays a character nobody plays): blow up the balloon (step 3, src/bonus/balloon.c), its results,
 *    then wake up (step 12, src/bonus/wake_up.c), its results, and the menu. Both, because each is a contest of under
 *    a minute between the four characters and the game's results screen after each says who won it: one alone is over
 *    almost as it starts, and the two make a game with a beginning and an end. The game begins at the first as the
 *    Konami code's stage select does (konami.c: under black until it starts), and the step after its results (stage
 *    2's) is set to the second's as the program goes on to a step; the second's end after its results (the walk to
 *    Channel 6, which leads to stage 7) ends the game.
 *
 *    FRANK GRIMES LIFE (guideline 16, Sebas: "el modo ronin seria 'frank grimes life'"; TMNT's RONIN): one player, the
 *    whole game, against what four would have. The game scales by player_count (4857, count_players_in_game: the slots
 *    playing, every frame of the game mode): the rank (rank_by_players), the spawns' level (set_spawn_level: the
 *    players less one plus the difficulty, which spawn records come), how many enemies hold the scroll
 *    (enemy_limits_by_difficulty, cemetery_enemy_limits_by_players), the doors' and the timed waves' enemies by the
 *    players (stage_player_count), and every boss's health and moves (..._by_players: the health a gauge's piece
 *    stands for, the health they are beaten at, their attacks' chances, waits and speeds, the bear's and the football
 *    player's waits for a player who is down). The console's count_players_in_game counts four while the one player
 *    plays. No other player comes in (the portraits, a START in the game), and the other boxes are blank.
 */
#include "console_internal.h"

#include "game.h"
#include "simpsons/machine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PHASE_STEPS         1u         /* mode_phase: the game's steps (game_run_step) */
#define STEP_STORY_1        1u         /* game_step: the story before stage 1 */
#define STEP_BONUS_1        3u         /* bonus 1, blow up the balloon, */
#define STEP_AFTER_BONUS_1  4u         /* the step after it (stage 2's), */
#define STEP_BONUS_2        12u        /* bonus 2, wake up, */
#define BONUS_2_END_PHASE   5u         /* and its end (bonus_2_end_phase: the walk to Channel 6 that stage 7 follows) */
#define STEP_ENDING         16u        /* and the ending */
#define FULL_HEALTH         0x28u      /* a player's vitality, full (player_join, player_run) */
#define ENEMIES_FIRST       14         /* the enemies' list, 4F80: objects 14-21 (run_enemies) */
#define ENEMIES_LAST        21
#define PLAYERS_FIRST       OBJECT_ADDRESS(0)    /* the players' objects and their partners: 4A40-4D3F */
#define PLAYERS_END         OBJECT_ADDRESS(8)
#define FRANK_PLAYERS       4u         /* the players FRANK GRIMES LIFE's game counts */

/* The lives of 'slot' (its player values, 49D0 + 8 x the slot: the lives' byte, 49D1) */
#define SLOT_LIVES(slot)    RAM8(player_variables_ADDRESS + 1u + 8u * (uint32_t)(slot))

/* ---- BOSS RUSH's bosses ---- */

/* Each boss: the step he is fought in and the step its crossing begins at (stage 3's forest before its cemetery),
   the step_phase of the play he comes in (stage_N_play; stage 8's second fight is its final_fight), his object's type
   in the enemies' list (the 4F80 list's handlers, symbols.txt), the stage's title card's name and his own (the
   decompilation's, by his sprites' set: SET_NAMES) */
#define RUSH_BOSSES         9
#define RUSH_ROBOT          8          /* (Mr. Burns' robot: after the game's own scene, no crossing) */
static const struct
{
	uint8_t step, enter_step, play_phase, type;
	const char *stage, *name;
} rush_bosses[RUSH_BOSSES] = {
	{ 2, 2, 4, 0x1Fu, "STAGE 1 DOWNTOWN SPRINGFIELD", "THE WRESTLER" },
	{ 4, 4, 4, 0x05u, "STAGE 2 KRUSTYLAND", "THE KRUSTY BALLOON" },
	{ 7, 6, 3, 0x3Eu, "STAGE 3 DISCOUNT CEMETERY", "THE GANGSTERS" },
	{ 8, 8, 4, 0x19u, "STAGE 4 MOE'S TAVERN", "THE FOOTBALL PLAYER" },
	{ 9, 9, 4, 0x20u, "STAGE 5 SPRINGFIELD BUTTE", "THE BEAR" },
	{ 11, 11, 4, 0xF8u, "STAGE 6 DREAMLAND", "THE DREAMLAND BOSS" },
	{ 13, 13, 4, 0x37u, "STAGE 7 CHANNEL 6", "THE KABUKI WARRIOR" },
	{ 15, 15, 5, 0xF6u, "STAGE 8 NUCLEAR POWER PLANT", "SMITHERS" },
	{ 15, 15, 9, 0xECu, "STAGE 8 NUCLEAR POWER PLANT", "MR. BURNS" },
};

/* The crossing's players: right all along, down for WANDER_STEP frames and up as long in every WANDER_EVERY (each
   player a quarter of it later: the blind scenarios' players, tools/make_scenarios.py, whom no stage held), the
   attack 2 frames in every ATTACK_EVERY; through the stories and the scenes, the attack 2 frames in every SKIP_EVERY
   (what a button skips, check_skip) */
#define WANDER_EVERY        480
#define WANDER_STEP         60
#define ATTACK_EVERY        64
#define SKIP_EVERY          8
#define PRESS_FRAMES        2
#define RUSH_MOST_FRAMES    12000      /* a stage crossed this long without its boss: the players' from there (none
                                          seen to: a safety, so that the black never stays) */

static int rush_boss;                  /* the boss being crossed to or fought (RUSH_BOSSES: all beaten) */
static bool rush_crossing;             /* the stage crossed out of sight */
static uint32_t rush_frames;           /* the frames of this crossing */
static uint16_t rush_rank_frames;      /* the rank's time as the crossing began */
static uint8_t rush_rank_time_step;
static uint8_t rush_lives[CONSOLE_PLAYERS];    /* each slot's lives as the crossing began */
static bool ending_seen;               /* a mode's game came to the ending: once it goes round, the game is over */
static bool debug_checked, debug;      /* SIMPSONS_MODES_DEBUG: each frame's state on the standard error, and with
                                          "uncovered" the crossing's frames without the card over them */
static bool debug_uncovered;

static bool slot_playing(int slot)
{
	return (players_playing & (1u << slot)) != 0u;
}

static bool playing(void)
{
	return console.page == PAGE_GAME && game_mode == GAME_MODE_GAME && !console.quitting_game;
}

static int mode_continues(void)
{
	return console.code_entered && console.infinite_lives ? CONTINUES_UNLIMITED : MODE_CONTINUES;
}

/* The per-game state, as a mode's game starts (and again at RESTART). For checks, SIMPSONS_BOSS_RUSH_FROM in the
   environment (1-8) starts BOSS RUSH at that boss's crossing: the step after the story set to his */
void modes_game_starting(void)
{
	const char *from = getenv("SIMPSONS_BOSS_RUSH_FROM");

	rush_boss = from && atoi(from) >= 1 && atoi(from) <= RUSH_ROBOT ? atoi(from) - 1 : 0;
	rush_crossing = console.mode == MODE_BOSS_RUSH;
	rush_frames = 0u;
	ending_seen = false;
}

/* FREE PLAY (Sebas: "en mr plow, agreguemos un modo normal de juego, pero sin limite de continues"): ARCADE's game,
   OPTIONS' lives, as many continues as STARTs whatever OPTIONS' CONTINUES says; after its ending, the menu */
void free_play_start(int player)
{
	console.konami = false;
	console.mode = MODE_FREE_PLAY;
	console_start_game(player, STEP_FIRST_STAGE, LIVES_CABINET, CONTINUES_UNLIMITED);
}

void boss_rush_start(int player)
{
	console.konami = false;
	console.mode = MODE_BOSS_RUSH;
	console_start_game(player, STEP_FIRST_STAGE, MODE_LIVES, mode_continues());
}

void bonus_games_start(int player)
{
	console.konami = false;
	console.mode = MODE_BONUS_GAMES;
	console_start_game(player, STEP_BONUS_1, MODE_LIVES, mode_continues());    /* (konami.c's start at a step) */
}

/* INTRO (the players' issue 8: "Intro Movie watchable within The Simpsons Console Version"; Sebas: "Solo INTRO en el
   menú", and "si esta inactivo, que meta una intro"): the story before stage 1, Smithers taking the diamond and
   Maggie with it. A game started as ARCADE's, its choice made at once (the player's Marge) and everything under
   black and silence (modes_covering) but the story; as the story ends (stage 1 next: its own end, or START or the
   attack skipping it, as in any scene) or BACK leaves it, the game ends and the menu comes back at INTRO */
void intro_start(int player)
{
	console.konami = false;
	console.mode = MODE_INTRO;
	console_start_game(player, STEP_FIRST_STAGE, LIVES_CABINET, CONTINUES_UNLIMITED);
}

bool modes_choice_made(void)
{
	return console.mode == MODE_INTRO;
}

/* (INTRO's game showing its story: not before it, the start and the choice under black) */
static bool intro_story(void)
{
	return console.page == PAGE_GAME && game_mode == GAME_MODE_GAME && mode_phase == PHASE_STEPS
		&& game_step == STEP_STORY_1;
}

static bool intro_frame(void)
{
	const bool leaving = (console.pressed & ACTION_BIT(ACTION_BACK)) != 0;

	console.pressed = 0;    /* (the game's own controls stay theirs: START and the attack skip the story) */
	if (leaving || (mode_phase == PHASE_STEPS && game_step >= STEP_FIRST_STAGE))
	{
		console_end_game();
		return true;
	}
	return false;
}

void frank_grimes_start(int player)
{
	console.konami = false;
	console.mode = MODE_FRANK_GRIMES;
	console_start_game(player, STEP_FIRST_STAGE, MODE_LIVES, mode_continues());
}

bool modes_code_rules(void)
{
	return console.mode != MODE_NONE && console.code_entered;
}

bool console_player_may_join(int player)
{
	return console.mode != MODE_FRANK_GRIMES || player == console.chooser;
}

/* ---- FRANK GRIMES LIFE: the game counts four players ---- */

/* The game's count_players_in_game (players/players.c), four while FRANK GRIMES LIFE's one player plays */
void count_players_in_game(void)
{
	original_count_players_in_game();
	if (console.mode == MODE_FRANK_GRIMES && console.page == PAGE_GAME && player_count < FRANK_PLAYERS)
		player_count = FRANK_PLAYERS;
}

/* ---- BOSS RUSH ---- */

/* The current boss's object in the enemies' list */
static bool boss_in(void)
{
	for (int number = ENEMIES_FIRST; number <= ENEMIES_LAST; number++)
		if (OBJECT(OBJECT_ADDRESS(number))->flags != 0u
			&& (uint8_t)OBJECT(OBJECT_ADDRESS(number))->type == rush_bosses[rush_boss].type)
			return true;    /* (the type a signed byte in struct object: the boss types from 80 on) */
	return false;
}

/* Whether 'type' is a boss of the step being crossed (stage 8's Smithers and Mr. Burns' robot come in together, the
   robot waiting for stage_cue: 03AD6B) */
static bool step_boss(uint8_t type)
{
	for (int boss = rush_boss; boss < RUSH_BOSSES && rush_bosses[boss].step == rush_bosses[rush_boss].step; boss++)
		if (rush_bosses[boss].type == type)
			return true;
	return false;
}

/* Krustyland's ride (teacup_goon_ride.c) counts its riders, type 1A of the enemies' list: off its arm as each one
   lands (teacup_goon_leap_land, state 5), off its hub as each one is beaten (teacup_goon_beaten), and it holds the
   scroll till both counts are 0. A rider is left on the ride, and beaten as by a player once it has landed */
#define TEACUP_GOON_TYPE    0x1Au
#define TEACUP_GOON_LANDED  6u         /* (its states from teacup_goon_ready on) */
#define TEACUP_GOON_BEATEN  0x80u      /* (its beaten and hit states, FE and FF) */
#define NO_POINTS           0x40u

/* Every object of the enemies' list but the step's bosses off the game, as the game takes an enemy off past the
   screen's edge (enemy_remove_off_screen): its slot free and, once it has a player as its target, the player's
   attacker it was let go (a new one has none yet: its target is 0) */
static void enemies_off(void)
{
	for (int number = ENEMIES_FIRST; number <= ENEMIES_LAST; number++)
	{
		const uint16_t object = (uint16_t)OBJECT_ADDRESS(number);
		const uint16_t target = OBJECT(object)->target;

		if (OBJECT(object)->flags == 0u || step_boss((uint8_t)OBJECT(object)->type))
			continue;
		if ((uint8_t)OBJECT(object)->type == TEACUP_GOON_TYPE)
		{
			if (OBJECT(object)->state >= TEACUP_GOON_LANDED && OBJECT(object)->state < TEACUP_GOON_BEATEN)
			{
				OBJECT(object)->points = NO_POINTS;    /* (no score for the crossing's: award_enemy_points passes
				                                          the beaten ones with bit 6) */
				teacup_goon_beaten(object);
			}
			continue;
		}
		OBJECT(object)->flags = 0u;
		if (target >= PLAYERS_FIRST && target < PLAYERS_END)
			enemy_release_target(object, target);
	}
}

/* Whether a stage the crossing goes through is played: the boss's step at its play's phase, or stage 3's forest before
   its cemetery (step_stage_3_forest's play, its phase 4) */
#define FOREST_STEP         6u
#define FOREST_PLAY_PHASE   4u

static bool crossing_play(void)
{
	if (game_step == rush_bosses[rush_boss].step)
		return step_phase == rush_bosses[rush_boss].play_phase;
	return game_step == FOREST_STEP && game_step == rush_bosses[rush_boss].enter_step && step_phase == FOREST_PLAY_PHASE;
}

static void crossing_begins(void)
{
	rush_crossing = true;
	rush_frames = 0u;
	rush_rank_frames = rank_frames;
	rush_rank_time_step = rank_time_step;
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
		rush_lives[slot] = SLOT_LIVES(slot);
}

static void boss_rush_frame(void)
{
	if (mode_phase != PHASE_STEPS || rush_boss >= RUSH_BOSSES)
		return;
	/* (the steps with no boss passed, at their first phase, to the next boss's: bonus 1, story 2, story 3, bonus 2,
	   story 4) */
	if (step_phase == 0u && game_step > STEP_STORY_1 && game_step < rush_bosses[rush_boss].enter_step)
	{
		game_step = rush_bosses[rush_boss].enter_step;
		step_subphase = 0u;
		step_countdown = 0u;
	}
	if (game_step == rush_bosses[rush_boss].step && step_phase > rush_bosses[rush_boss].play_phase)
	{
		/* (beaten: the next one's crossing, the stage's end crossed too; after Smithers the game's own scene and
		   the robot's entrance; after the robot the game's own end) */
		rush_boss++;
		if (rush_boss < RUSH_ROBOT)
			crossing_begins();
		else
			rush_crossing = false;
		return;
	}
	if (!rush_crossing)
		return;
	if (rush_frames == 0u)
		crossing_begins();
	rush_frames++;
	if (game_step == rush_bosses[rush_boss].step && step_phase == rush_bosses[rush_boss].play_phase && boss_in())
	{
		rush_crossing = false;    /* (he comes: the players' from his entrance) */
		return;
	}
	if (rush_frames >= RUSH_MOST_FRAMES)
	{
		rush_crossing = false;
		return;
	}
	if (crossing_play())
		enemies_off();    /* (in a stage's play alone: the portraits and the stories have objects in that list) */
	rank_frames = rush_rank_frames;
	rank_time_step = rush_rank_time_step;
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
	{
		const uint16_t object = OBJECT_ADDRESS(2u * (uint32_t)slot);

		if (!slot_playing(slot))
			continue;
		if (SLOT_LIVES(slot) < rush_lives[slot])
			SLOT_LIVES(slot) = rush_lives[slot];
		rush_lives[slot] = SLOT_LIVES(slot);    /* (a player in during the crossing: his) */
		if (OBJECT(object)->flags != 0u && OBJECT(object)->vitality != 0u && OBJECT(object)->vitality < FULL_HEALTH)
			OBJECT(object)->vitality = FULL_HEALTH;
	}
}

bool boss_rush_crossing(void)
{
	return console.mode == MODE_BOSS_RUSH && rush_crossing && playing();
}

/* The crossing's controls of 'slot' (its player's own are not on the board meanwhile): the attack to skip the stories
   and the scenes, and through a stage's play the walk right */
uint8_t modes_presses(int slot)
{
	const uint64_t frame = simpsons_board.frame;
	const uint64_t wander = (frame + (uint64_t)slot * (WANDER_EVERY / 4)) % WANDER_EVERY;
	uint8_t controls = SIMPSONS_RIGHT;

	if (!boss_rush_crossing() || !slot_playing(slot) || mode_phase != PHASE_STEPS)
		return 0u;
	if (!crossing_play())
		return frame % SKIP_EVERY < PRESS_FRAMES ? SIMPSONS_ATTACK : 0u;
	if (wander < WANDER_STEP)
		controls |= SIMPSONS_DOWN;
	else if (wander >= WANDER_EVERY / 2 && wander < WANDER_EVERY / 2 + WANDER_STEP)
		controls |= SIMPSONS_UP;
	if (frame % ATTACK_EVERY < PRESS_FRAMES)
		controls |= SIMPSONS_ATTACK;
	return controls;
}

/* ---- BONUS GAMES ---- */

/* The step after bonus 1's results set to bonus 2's, and bonus 2's end after its results (the walk to Channel 6, the
   story stage 7 follows) ends the game: true when the game's frame is not to run (its end, at the next frame) */
static bool bonus_games_frame(void)
{
	if (mode_phase != PHASE_STEPS)
		return false;
	if (game_step == STEP_AFTER_BONUS_1 && step_phase == 0u)
	{
		game_step = STEP_BONUS_2;
		step_subphase = 0u;
		step_countdown = 0u;
	}
	else if (game_step == STEP_BONUS_2 && step_phase == BONUS_2_END_PHASE)
	{
		console_end_game();
		return true;
	}
	return false;
}

/* ---- each frame ---- */

static void debug_line(void)
{
	char enemies[64];
	size_t used = 0;

	if (!debug_checked)
	{
		const char *setting = getenv("SIMPSONS_MODES_DEBUG");

		debug = setting != NULL;
		debug_uncovered = setting != NULL && strcmp(setting, "uncovered") == 0;
		debug_checked = true;
	}
	if (!debug)
		return;
	enemies[0] = 0;
	for (int number = ENEMIES_FIRST; number <= ENEMIES_LAST && used + 4 < sizeof(enemies); number++)
		if (OBJECT(OBJECT_ADDRESS(number))->flags != 0u)
			used += (size_t)snprintf(enemies + used, sizeof(enemies) - used, " %02X",
				(unsigned)(uint8_t)OBJECT(OBJECT_ADDRESS(number))->type);
	fprintf(stderr, "modes %llu: mode %d page %d phase %u step %u/%u boss %d crossing %d frames %u camera %04X,%04X "
		"rank %u difficulty %u players %u allowed %u lives %u %u %u %u enemies%s\n",
		(unsigned long long)simpsons_board.frame, console.mode, console.page, (unsigned)mode_phase, (unsigned)game_step,
		(unsigned)step_phase, rush_boss, rush_crossing, (unsigned)rush_frames, (unsigned)camera_x, (unsigned)camera_y,
		(unsigned)rank, (unsigned)difficulty, (unsigned)player_count, (unsigned)io_read8(enemies_allowed),
		(unsigned)SLOT_LIVES(0), (unsigned)SLOT_LIVES(1), (unsigned)SLOT_LIVES(2), (unsigned)SLOT_LIVES(3), enemies);
}

bool modes_game_frame(void)
{
	bool stop = false;

	if (playing())
		debug_line();    /* (any game's: the modes' counts against ARCADE's) */
	if (console.mode == MODE_NONE || !playing())
		return false;
	if (ending_seen && mode_phase == PHASE_STEPS && game_step != STEP_ENDING)
	{
		/* (after the ending and the high scores the game would go round for the players still in: the menu) */
		console_end_game();
		return true;
	}
	if (mode_phase == PHASE_STEPS && game_step == STEP_ENDING)
		ending_seen = true;
	if (console.mode == MODE_BOSS_RUSH)
		boss_rush_frame();
	else if (console.mode == MODE_BONUS_GAMES)
		stop = bonus_games_frame();
	else if (console.mode == MODE_INTRO)
		stop = intro_frame();
	return stop;
}

bool modes_covering(void)
{
	return boss_rush_crossing() || (console.mode == MODE_INTRO && !intro_story());
}

/* The crossing's card on black: BOSS RUSH, the boss's number, his stage and his name */
#define CARD_COLOUR         0xFFD840u
#define CARD_WHITE          0xFFFFFFu
#define CARD_HEADING_Y      64
#define CARD_NUMBER_Y       92
#define CARD_STAGE_Y        112
#define CARD_NAME_Y         136

void modes_overlay(console_picture *frame)
{
	char text[24];
	const int boss = rush_boss < RUSH_BOSSES ? rush_boss : RUSH_BOSSES - 1;

	if (!modes_covering() || debug_uncovered)
		return;
	draw_fill(frame, PICTURE_FIRST(frame), 0, PICTURE_END(frame) - PICTURE_FIRST(frame), SIMPSONS_VIDEO_HEIGHT,
		0x000000u);
	if (console.mode != MODE_BOSS_RUSH)
		return;    /* (INTRO's: black alone till its story) */
	draw_game_letters_centred(frame, CARD_HEADING_Y, 2, CARD_COLOUR, "BOSS RUSH");
	snprintf(text, sizeof(text), "BOSS %d OF %d", boss + 1, RUSH_BOSSES);
	draw_game_letters_centred(frame, CARD_NUMBER_Y, 1, CARD_WHITE, text);
	draw_game_letters_centred(frame, CARD_STAGE_Y, 1, CARD_WHITE, rush_bosses[boss].stage);
	draw_game_letters_centred(frame, CARD_NAME_Y, 2, CARD_COLOUR, rush_bosses[boss].name);
}

const char *modes_describe(void)
{
	static char text[96];
	static const char *const names[] = { "none", "free play", "boss rush", "bonus games", "frank grimes life",
		"intro" };

	snprintf(text, sizeof(text), "mode %s%s, boss %d%s, player_count %u, rank %u, difficulty %u",
		names[console.mode >= 0 && console.mode <= MODE_INTRO ? console.mode : 0],
		modes_code_rules() ? " (code)" : "", rush_boss, rush_crossing ? " crossing" : "", (unsigned)player_count,
		(unsigned)rank, (unsigned)difficulty);
	return text;
}

/* A save state's part (states.c): the mode under way, what this file keeps from frame to frame (the mode itself is
   console's) */
void modes_state(simpsons_state *state)
{
	simpsons_state_part(state, "modes");
	simpsons_state_bytes(state, &rush_boss, sizeof(rush_boss));
	simpsons_state_bytes(state, &rush_crossing, sizeof(rush_crossing));
	simpsons_state_bytes(state, &rush_frames, sizeof(rush_frames));
	simpsons_state_bytes(state, &rush_rank_frames, sizeof(rush_rank_frames));
	simpsons_state_bytes(state, &rush_rank_time_step, sizeof(rush_rank_time_step));
	simpsons_state_bytes(state, rush_lives, sizeof(rush_lives));
	simpsons_state_bytes(state, &ending_seen, sizeof(ending_seen));
}
