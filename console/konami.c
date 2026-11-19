/**
 *    Name:        konami.c
 *    Description: a game of the console's begun at a stage (the Konami code's page's, a mode's), and the Konami
 *                 code's game's lives and players' health kept up
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    Up, up, down, down, left, right, left, right, B, A on the menu (console/PLAN.md, guideline 9; Sebas: "tmb konami
 *    code", "No te olvides del Konami code") opens the KONAMI CODE page (menu.c): a stage or bonus game to start in,
 *    LIVES INFINITE or NORMAL, HEALTH INFINITE or NORMAL, REPEAT CHARACTERS (same_character.c). Its game is a game of
 *    the console's own modes (MODE_LIVES a credit, MODE_CONTINUES continues; with LIVES INFINITE as many as one likes),
 *    started as ARCADE starts it, the characters chosen on the portraits; as tmnt-port's level select.
 *
 *    A game begun at a step (console_start_game: the Konami page's stage, or a mode's) starts it as the scenarios start
 *    a stage (tools/make_scenarios.py): the game goes on to a step as the program does at a step's end (A75E:
 *    game_step one more, step_phase and its two bytes cleared), and a poke of those, once stage 1 is played, starts
 *    that step whole. So the story before stage 1 is skipped by a playing slot's attack (any player's skips it,
 *    check_skip; in the Konami game also before its stage 1), and once every player has come into stage 1's play the
 *    step is set (some 700 frames after the portraits: the story's least, stage 1's title card and opening).
 *    Meanwhile the stage's name on black over the game, and no sound: the port plays it at volume 0 (console_volume),
 *    the game and its sound program going on as they do (letting the game's sound commands go made the story wait
 *    some 9 seconds longer for its sounds), and the port runs those frames as fast as it can (console_hurried: no
 *    waiting for the frame's time, one picture in a few): the stage's own title card comes up in a fraction of a
 *    second.
 *    LIVES INFINITE keeps each playing slot's lives at MODE_LIVES (a life lost: one less in the box, the next frame
 *    back up), HEALTH INFINITE each player's health full before every frame (a hit takes some of it at most; one that
 *    takes all at once still knocks him out, and his life goes as the game takes it).
 */
#include "console_internal.h"

#include "game.h"
#include "simpsons/machine.h"

#include <stdio.h>

#define PHASE_STEPS         1u         /* mode_phase: the game's steps (game_run_step) */
#define STEP_STORY_1        1u         /* game_step: the story before stage 1, */
#define STEP_STAGE_1        2u         /* stage 1 */
#define PLAY_PHASE          4u         /* step_phase: a stage's play (step_stage_1's 4, stage_1_play) */
#define SKIP_EVERY          8          /* the story skipped: a playing slot's attack pressed 2 frames in 8 */
#define SKIP_FRAMES         2
#define FULL_HEALTH         0x28u      /* a player's vitality, full (player_join, player_run) */
#define ENTRY_STATUS(slot)  RAM8(marge_entry_status_ADDRESS + 8u * (uint32_t)(slot))    /* (0: not come in yet) */
#define NAME_Y              104        /* the stage's name over the black */
#define NAME_COLOUR         0xFFD840u

/* The steps a stage or a bonus game is (make_scenarios.py's STEPS), by the title cards' names (stage_N_title_text) */
static const struct
{
	uint8_t step;
	const char *name;
} stages[KONAMI_STAGES] = {
	{ 2, "1 DOWNTOWN SPRINGFIELD" },
	{ 3, "BONUS 1 BALLOON" },
	{ 4, "2 KRUSTYLAND" },
	{ 6, "3 DISCOUNT CEMETERY" },
	{ 8, "4 MOE'S TAVERN" },
	{ 9, "5 SPRINGFIELD BUTTE" },
	{ 11, "6 DREAMLAND" },
	{ 12, "BONUS 2 WAKE UP" },
	{ 13, "7 CHANNEL 6" },
	{ 15, "8 NUCLEAR POWER PLANT" },
};

const char *konami_stage_name(int stage)
{
	return stages[stage >= 0 && stage < KONAMI_STAGES ? stage : 0].name;
}

int konami_stage_step(int stage)
{
	return stages[stage >= 0 && stage < KONAMI_STAGES ? stage : 0].step;
}

/* The name of a step that is a stage or a bonus game, for the black before it */
static const char *step_name(int step)
{
	for (int stage = 0; stage < KONAMI_STAGES; stage++)
		if (stages[stage].step == step)
			return stages[stage].name;
	return "";
}

/* The lives of 'slot' (its player values, 49D0 + 8 x the slot: the lives' byte, 49D1, as the scenarios poke them) */
#define SLOT_LIVES(slot)    RAM8(player_variables_ADDRESS + 1u + 8u * (uint32_t)(slot))

static bool slot_playing(int slot)
{
	return (players_playing & (1u << slot)) != 0u;
}

/* Every playing slot has come into stage 1's play (its object in, the step can be left as the scenarios leave it) */
static bool stage_1_entered(void)
{
	bool any = false;

	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
		if (slot_playing(slot))
		{
			if (ENTRY_STATUS(slot) == 0u)
				return false;
			any = true;
		}
	return any;
}

bool konami_covering(void)
{
	return console.start_pending && console.page == PAGE_GAME && game_mode == GAME_MODE_GAME;
}

void konami_game_frame(void)
{
	if (console.page != PAGE_GAME || game_mode != GAME_MODE_GAME)
		return;
	if (console.start_pending && mode_phase == PHASE_STEPS && game_step == STEP_STAGE_1 && step_phase == PLAY_PHASE
		&& stage_1_entered())
	{
		step_phase = 0u;
		step_subphase = 0u;
		step_countdown = 0u;
		game_step = (uint8_t)console.start_step;
		console.start_pending = false;
	}
	if (!console.konami && !modes_code_rules())
		return;    /* (MR. PLOW MODE's games too, once the code was entered on the menu: modes.c) */
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
	{
		const uint16_t object = OBJECT_ADDRESS(2u * (uint32_t)slot);

		if (!slot_playing(slot))
			continue;
		if (console.infinite_lives && SLOT_LIVES(slot) < MODE_LIVES)
			SLOT_LIVES(slot) = MODE_LIVES;
		/* (alive and in: a knocked out player, his vitality 0, goes on as the game takes him) */
		if (console.infinite_health && ENTRY_STATUS(slot) != 0u && OBJECT(object)->flags != 0u
			&& OBJECT(object)->vitality != 0u && OBJECT(object)->vitality < FULL_HEALTH)
			OBJECT(object)->vitality = FULL_HEALTH;
	}
}

int console_volume(void)
{
	return konami_covering() || modes_covering() || console_scene_quiet() ? 0 : console_settings.volume;    /* (no
		sound under the black: a stage's start, BOSS RUSH's crossing, a story skipped whole) */
}

bool console_hurried(void)
{
	return konami_covering() || modes_covering() || console_scene_skipping();
}

uint8_t konami_presses(int slot)
{
	if ((!console.konami && !console.start_pending) || console.page != PAGE_GAME || game_mode != GAME_MODE_GAME
		|| mode_phase != PHASE_STEPS || game_step != STEP_STORY_1 || !slot_playing(slot))
		return 0u;
	return simpsons_board.frame % SKIP_EVERY < SKIP_FRAMES ? SIMPSONS_ATTACK : 0u;
}

void konami_overlay(console_picture *frame)
{
	const char *name = step_name(console.start_step);
	char text[40];

	if (!konami_covering())
		return;
	draw_fill(frame, PICTURE_FIRST(frame), 0, PICTURE_END(frame) - PICTURE_FIRST(frame), SIMPSONS_VIDEO_HEIGHT,
		0x000000u);
	snprintf(text, sizeof(text), "%s%s", name[0] >= '0' && name[0] <= '9' ? "STAGE " : "", name);
	draw_game_letters_centred(frame, NAME_Y, 1, NAME_COLOUR, text);
}
