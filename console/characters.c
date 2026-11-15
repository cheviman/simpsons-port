/**
 *    Name:        characters.c
 *    Description: who plays whom: the character choice on the game's portraits as a game starts, and a player's
 *                 character chosen again in a game (after a continue, or coming in)
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    In this set each slot is a character (1 Marge, 2 Homer, 3 Bart, 4 Lisa): the console puts a player's controls on
 *    the slot of the character he chooses, and the program stays as it is (console/PLAN.md, guideline 3; as
 *    tmnt-port's turtle choice). Sebas: "en la version consola, quiero que se pueda cambiar los personajes", "los
 *    personajes, 1 sin repetir".
 *
 *    The choice is the game's own portraits screen (step_portraits: the four characters' boxes, a portrait coming
 *    alive with its voice when its slot has a credit), which the game shows as ARCADE starts it with no credit in:
 *    the player who picked ARCADE is in it, the others come in with START or attack; LEFT and RIGHT go among the
 *    characters no other player has picked, attack or START picks one, and its slot's coin goes in at once (each slot
 *    has credits of its own on the factory's EEPROM), so its portrait comes alive and says its voice; jump leaves it
 *    (the one who started the game: back to the menu, while nobody has picked). The console holds the portraits while
 *    players choose (their 32 seconds), and once everyone in has picked and the last portrait has come alive, it
 *    presses each one's attack: the players join as the cabinet's do, and the portraits end as the game ends them.
 *    RESTART picks again at once what each player played. In the Konami code's game with REPEAT CHARACTERS
 *    (same_character.c) a character may be picked by more than one player: the first on its slot, the others on slots
 *    no one has, which play it; its box is theirs (their frames and tags on it), and the portrait of a slot another
 *    character plays on does not come alive.
 *
 *    In a game, the START of a player whose character does not play (his lives are over, or he has none) opens a line
 *    over the game, "1P < BART >": his own character first if it is free, LEFT and RIGHT among the free ones (their
 *    character not playing nor coming in, no other player on them), attack or START brings him in on it with a coin
 *    and the attack, jump closes it (TMNT's turtle change after a continue). The game goes on meanwhile, and his
 *    controls reach no slot while he chooses. With repeats, any character while a slot is free (on its own slot when
 *    that is free). A game of the console's own modes counts his credits: once his continues are over his START opens
 *    nothing (console_credit_allowed).
 */
#include "console_internal.h"

#include "game.h"

#include <stdio.h>
#include <string.h>

#define PHASE_STEPS         1u         /* mode_phase: the game's steps (game_run_step) */
#define PHASE_GAME_OVER     3u         /* GAME OVER and the name entry from here */
#define STEP_PORTRAITS      0u         /* game_step 0: the portraits (step_portraits) */
#define PORTRAITS_RUN       1u         /* its step_phase while they wait for the players, */
#define PORTRAITS_WAIT      2u         /* and once one has pressed (64 frames) */
#define PORTRAITS_TIME      0x0780u    /* their 32 seconds (portraits_start) */
#define PORTRAITS_WAIT_TIME 0x40u      /* the frames after a press (portraits_run) */
#define BOX_LEFT            17         /* the portraits' boxes (game_start_draw_portraits): the first's frame, */
#define BOX_STEP            72         /* the next one's */
#define BOX_WIDTH           70
#define BOX_TOP             33
#define BOX_HEIGHT          158
#define FRAME_THICKNESS     2
#define TAG_Y               38         /* the players' tags, at their boxes' top */
#define TAG_STEP            20
#define CHOICE_HEADING_Y    198        /* under the boxes */
#define CHOICE_JOIN_Y       210
#define CHANGE_Y            176        /* the change's lines over the game's lower part */
#define CHANGE_STEP         12
#define WHITE               0xFFFFFFu

#define ALIVE_FRAMES        80         /* from the last pick to the game going on: a portrait comes alive (its 32
                                          frames' wait, its 6 pictures 3 frames each) and says its voice */

const char *const console_character_names[CHARACTERS] = { "MARGE", "HOMER", "BART", "LISA" };
static int last_pick;              /* the choice's frame of its last pick */
const char *const console_player_tags[CONSOLE_PLAYERS] = { "1P", "2P", "3P", "4P" };
const uint32_t console_player_colours[CONSOLE_PLAYERS] = { 0xFF5050u, 0x50A0FFu, 0x50E050u, 0xFFD040u };

/* ---- the choice on the portraits ---- */

static bool portraits_shown(void)
{
	return game_mode == GAME_MODE_GAME && mode_phase == PHASE_STEPS && game_step == STEP_PORTRAITS
		&& step_phase >= PORTRAITS_RUN && step_subphase >= 1u && layer_clear_pending == 0u;
}

void choice_open(void)
{
	for (int player = 0; player < CONSOLE_PLAYERS; player++)
	{
		if (!console.restarting)
			console.choice_character[player] = player == console.chooser ? CHARACTER_MARGE : -1;
		console.choice_picked[player] = false;
		console_slot_of[player] = -1;
	}
	console_characters_reset();
	console.page = PAGE_CHOICE;
	console.choice = 0;
	console.page_frames = 0;
}

/* A character no other player has picked: 'character', or the next one in the direction 'by' (+1, -1); -1 for none */
static int free_character(int player, int character, int by)
{
	if (console_repeats_allowed())
		return character;    /* (any: the Konami code's REPEAT CHARACTERS) */
	for (int tries = 0; tries < CHARACTERS; tries++, character = (character + by + CHARACTERS) % CHARACTERS)
	{
		bool taken = false;

		for (int other = 0; other < CONSOLE_PLAYERS; other++)
			taken = taken || (other != player && console.choice_picked[other] && console.choice_character[other] == character);
		if (!taken)
			return character;
	}
	return -1;
}

/* 'player' picks his character: its slot's coin (its portrait comes alive with its voice) */
static void pick(int player)
{
	const int character = console.choice_character[player];
	const int slot = console_repeats_allowed() ? console_slot_for_character(player, character) : character;

	if (slot < 0)
		return;
	console.choice_picked[player] = true;
	console_slot_of[player] = slot;
	console_join(slot, true, false);
	console_credit_put(player);
	last_pick = console.page_frames;
	for (int other = 0; other < CONSOLE_PLAYERS && !console_repeats_allowed(); other++)
		if (other != player && console.choice_character[other] == character && !console.choice_picked[other])
			console.choice_character[other] = free_character(other, character, 1);    /* (off the picked one) */
}

void choice_frame(const uint32_t player_pressed[CONSOLE_PLAYERS])
{
	const uint32_t picks = ACTION_BIT(ACTION_ACCEPT) | ACTION_BIT(ACTION_START);
	int in = 0, picked = 0;
	bool coins_going_in = false;

	console.page_frames++;
	if (!portraits_shown())
		return;
	/* (the portraits held while players choose: their 32 seconds, and the wait after the first join) */
	if (step_phase == PORTRAITS_RUN)
		step_timer = PORTRAITS_TIME;
	else
		step_timer = (uint16_t)((step_timer & 0xFFu) | PORTRAITS_WAIT_TIME << 8);
	if (console.restarting || modes_choice_made())
	{
		/* (RESTART: what each one played; INTRO: the player's Marge, the choice under black) */
		console.restarting = false;
		for (int player = 0; player < CONSOLE_PLAYERS; player++)
			if (console.choice_character[player] >= 0 && !console.choice_picked[player])
				pick(player);
	}
	for (int player = 0; player < CONSOLE_PLAYERS; player++)
	{
		const uint32_t pressed = player_pressed[player];
		int *character = &console.choice_character[player];
		const int by = (pressed & ACTION_BIT(ACTION_LEFT)) ? -1 : (pressed & ACTION_BIT(ACTION_RIGHT)) ? 1 : 0;

		if (console.choice_picked[player])
			continue;    /* (his character came in) */
		if (*character < 0)
		{
			if ((pressed & picks) && console_player_may_join(player))    /* (FRANK GRIMES LIFE: one player) */
				*character = free_character(player, CHARACTER_MARGE, 1);
			continue;
		}
		if (pressed & ACTION_BIT(ACTION_BACK))
		{
			bool anyone = false;

			for (int other = 0; other < CONSOLE_PLAYERS; other++)
				anyone = anyone || console.choice_picked[other];
			if (player != console.chooser)
				*character = -1;
			else if (!anyone)
			{
				console_end_game();    /* (the menu again) */
				return;
			}
			continue;
		}
		if (by)
		{
			const int other = free_character(player, (*character + by + CHARACTERS) % CHARACTERS, by);

			*character = other >= 0 ? other : *character;
		}
		if ((pressed & picks) && free_character(player, *character, 1) == *character)
			pick(player);
	}
	for (int player = 0; player < CONSOLE_PLAYERS; player++)
	{
		in += console.choice_character[player] >= 0;
		picked += console.choice_character[player] >= 0 && console.choice_picked[player];
		coins_going_in = coins_going_in || (console.choice_picked[player] && console_join_going_on(console_slot_of[player]));
	}
	if (in > 0 && picked == in && !coins_going_in && console.page_frames - last_pick >= ALIVE_FRAMES)
	{
		/* (everyone in has picked and his coin is in: each one's attack brings him in, the portraits end as the game
		   ends them, the players' controls on their slots from now on) */
		for (int player = 0; player < CONSOLE_PLAYERS; player++)
			if (console.choice_character[player] >= 0)
				console_join(console_slot_of[player], false, true);
		console.page = PAGE_GAME;
		console.page_frames = 0;
	}
}

void choice_overlay(console_picture *frame)
{
	int in = 0, on[CHARACTERS] = { 0, 0, 0, 0 }, drawn[CHARACTERS] = { 0, 0, 0, 0 };

	if (console.page != PAGE_CHOICE || !portraits_shown())
		return;
	for (int player = 0; player < CONSOLE_PLAYERS; player++)
		if (console.choice_character[player] >= 0)
		{
			in++;
			on[console.choice_character[player]]++;
		}
	for (int player = 0; player < CONSOLE_PLAYERS; player++)
	{
		const int character = console.choice_character[player];
		const int x = BOX_LEFT + character * BOX_STEP;
		const int inset = drawn[character] * (FRAME_THICKNESS + 1);
		const uint32_t colour = console_player_colours[player];

		if (character < 0)
			continue;
		/* (his box's frame in his colour, nested inwards when two are on it; his tag at its top, blinking until he
		   picks) */
		draw_outline(frame, x + inset, BOX_TOP + inset, BOX_WIDTH - 2 * inset, BOX_HEIGHT - 2 * inset, FRAME_THICKNESS,
			colour);
		if (console.choice_picked[player] || (console.page_frames & 16) == 0)
			draw_game_letters(frame, x + (BOX_WIDTH - on[character] * TAG_STEP) / 2 + drawn[character] * TAG_STEP + 2,
				TAG_Y, 1, colour, console_player_tags[player]);
		drawn[character]++;
	}
	draw_game_letters_centred(frame, CHOICE_HEADING_Y, 1, WHITE, "CHOOSE YOUR CHARACTER");
	if (in < CONSOLE_PLAYERS && (console.page_frames & 32) == 0 && console.mode != MODE_FRANK_GRIMES)
		draw_game_letters_centred(frame, CHOICE_JOIN_Y, 1, WHITE, "PRESS START TO JOIN");    /* (FRANK GRIMES LIFE:
			one player) */
}

/* ---- a character chosen again in a game ---- */

bool console_slot_free_for(int player, int slot)
{
	if (console_slot_plays(slot) || console_slot_joining(slot))
		return false;
	for (int other = 0; other < CONSOLE_PLAYERS && !console_repeats_allowed(); other++)
		if (other != player && console.character_change[other] == slot + 1)
			return false;    /* (another player is choosing it) */
	return true;
}

/* Whether 'player' may come in as 'character': its slot free (with repeats, any slot) */
static bool character_free_for(int player, int character)
{
	if (!console_repeats_allowed())
		return console_slot_free_for(player, character);
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
		if (console_slot_free_for(player, slot))
			return true;
	return false;
}

/* 'character', or the next one 'player' may come in as in the direction 'by' (+1, -1); -1 for none */
static int free_character_for(int player, int character, int by)
{
	for (int tries = 0; tries < CHARACTERS; tries++, character = (character + by + CHARACTERS) % CHARACTERS)
		if (character_free_for(player, character))
			return character;
	return -1;
}

static bool change_allowed(void)
{
	return console.page == PAGE_GAME && game_mode == GAME_MODE_GAME && mode_phase < PHASE_GAME_OVER
		&& !console.quitting_game;
}

void character_change_open(int player)
{
	const int own = console_slot_of[player];
	const int first = change_allowed() && console_credit_allowed(player)
		? free_character_for(player, own >= 0 ? console_slot_character[own] : CHARACTER_MARGE, 1) : -1;

	console.character_change[player] = first + 1;
}

void character_change_frame(int player, uint8_t fresh, bool start)
{
	int character = console.character_change[player] - 1;

	if (!change_allowed() || (fresh & SIMPSONS_JUMP))
	{
		console.character_change[player] = 0;
		return;
	}
	if (fresh & (SIMPSONS_LEFT | SIMPSONS_RIGHT))
	{
		const int by = (fresh & SIMPSONS_LEFT) ? -1 : 1;

		character = free_character_for(player, (character + by + CHARACTERS) % CHARACTERS, by);
	}
	else if (!character_free_for(player, character))
		character = free_character_for(player, character, 1);    /* (another player came in on it meanwhile) */
	if (character < 0)
	{
		console.character_change[player] = 0;
		return;
	}
	if ((fresh & SIMPSONS_ATTACK) || start)
	{
		const int slot = console_repeats_allowed() ? console_slot_for_character(player, character) : character;

		console.character_change[player] = 0;
		if (slot < 0)
			return;
		console_slot_of[player] = slot;
		console_join(slot, true, true);
		console_credit_put(player);
		return;
	}
	console.character_change[player] = character + 1;
}

void character_change_overlay(console_picture *frame)
{
	int lines = 0, line = 0;

	for (int player = 0; player < CONSOLE_PLAYERS; player++)
		lines += console.character_change[player] != 0;
	if (!lines || !change_allowed())
		return;
	draw_shade(frame, PICTURE_FIRST(frame), CHANGE_Y - 6, PICTURE_END(frame) - PICTURE_FIRST(frame),
		lines * CHANGE_STEP + 8, 80);
	for (int player = 0; player < CONSOLE_PLAYERS; player++)
	{
		char text[32];

		if (!console.character_change[player])
			continue;
		snprintf(text, sizeof(text), "%s  < %s >", console_player_tags[player],
			console_character_names[console.character_change[player] - 1]);
		draw_game_letters_centred(frame, CHANGE_Y + line++ * CHANGE_STEP, 1, console_player_colours[player], text);
	}
}

/* A LAN game's power-on (console_lan_power_on): no pick yet */
void characters_power_on(void)
{
	last_pick = 0;
}

/* A save state's part (states.c): the choice's last pick (the rest of the choice is console's) */
void characters_state(simpsons_state *state)
{
	simpsons_state_part(state, "characters");
	simpsons_state_bytes(state, &last_pick, sizeof(last_pick));
}
