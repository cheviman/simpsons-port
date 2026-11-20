/**
 *    Name:        menu.c
 *    Description: the console's menus over the attract mode: the menu (ARCADE, MR. PLOW MODE, MULTIPLAYER, INTRO,
 *                 OPTIONS, EXIT),
 *                 MR. PLOW MODE's page, OPTIONS with VIDEO and CONTROLS, the Konami code's page, and the pause's
 *                 picture
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    15-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    The menu is on the TV the attract mode shows (tv.c: the family watches it on the couch, and the Konami logo and
 *    the score ranking show on it), its lines on the TV's screen under the title's logo, with the title's copyright
 *    line and "@cheviman" under them (Sebas: "para el menu, usemos la tele, que tiene de titulos para presentarlo");
 *    the other pages write over the TV darkened. All of them with the game's own letters (draw.c). ARCADE starts the
 *    arcade game at home for the player who picks it: a coin, then the character choice on the game's portraits
 *    (characters.c); the others come in with START. INTRO plays the story before stage 1 and comes back (modes.c),
 *    and so does the menu left alone IDLE_FRAMES (the players' issue 8; Sebas: "si esta inactivo, que meta una
 *    intro"). OPTIONS changes the cabinet's settings by the test mode's names (8-3 DIFFICULTY,
 *    8-2 NUMBER OF PLAYERS as LIVES, 8-4 SOUND IN ATTRACT MODE as DEMO SOUND, 8-5 VIDEO SCREEN FLIP as SCREEN), which
 *    go into the game when OPTIONS closes; CONTINUES (ARCADE's, 0 to 9 or UNLIMITED), VIDEO, VOLUME and SOUND (MONO,
 *    the board's, or STEREO) the port's. The
 *    port's version on OPTIONS (in its panel, at BACK's right) and under the menu with "@cheviman". As tmnt-port's
 *    console/menu.c.
 *
 *    The Konami code on the menu (guideline 9: up, up, down, down, left, right, left, right, B, A: the jump and the
 *    attack, the cabinet's two buttons as a pad has them; from any player's controls or the keyboard) opens the
 *    KONAMI CODE page with the coin's sound: STAGE (each stage and bonus game, by its title card's name), LIVES
 *    INFINITE or NORMAL, HEALTH INFINITE or NORMAL, REPEAT CHARACTERS ON or OFF, START (its game: the character choice
 *    on the portraits, then konami.c), BACK (to the menu's line the code was entered on); the menu has no line of it,
 *    as tmnt-port's level select. What it sets is the console's, not kept in simpsons.ini (tmnt-port's).
 *
 *    On the devices (as tmnt-port's): no EXIT where the system leaves the game (SIMPSONS_SYSTEM_EXIT: the Switch's
 *    HOME), and no CONTROLS where the device's controls are fixed (SIMPSONS_HANDHELD), whose VIDEO has PIXELS,
 *    INTEGER SCALE and FPS alone: its screen is the window, WIDE as it is. A device whose players bind their game
 *    controllers' buttons has CONTROLS (SIMPSONS_DEVICE_CONTROLS: the Switch, the players' issue 21), its page the
 *    computers' with no keys.
 */
#include "console_internal.h"

#include "game.h"

#include <stdio.h>
#include <string.h>

#define SCREEN_WIDTH        SIMPSONS_VIDEO_WIDTH
#define ITEM_COLOUR         0xFFFFFFu
#define CHOSEN_COLOUR       0xFFD840u    /* yellow */
#define HEADING_COLOUR      0xFFD840u
#define SIGNATURE_COLOUR    0xD0D0D0u
#define COPYRIGHT_COLOUR    0xD0D0D0u
#define MENU_Y              90           /* the menu's lines, on the TV's screen under the title's logo (six: */
#define ITEM_STEP           11           /* their copyright and signature in the screen, INTRO among them) */
#define COPYRIGHT           "[ 1991 KONAMI"    /* the title's copyright line, Konami's (game text 2's: '[' its
                                                  copyright sign), under the menu */
#define COPYRIGHT_Y         (MENU_Y + MENU_ITEMS * ITEM_STEP + 4)
#define SIGNATURE           "@cheviman  V" SIMPSONS_VERSION    /* the port's author, under it (as tmnt-port's), and
                                                            its release */
#define SIGNATURE_Y         (COPYRIGHT_Y + 12)
#define VERSION_TEXT        "V" SIMPSONS_VERSION    /* OPTIONS: the port's release, on BACK's line at the right */
#define VERSION_COLOUR      0xA0A0A0u
#define CODE_SOUND          0x4002u      /* the coin's sound (coin_counter's), as tmnt-port's: the Konami code took */
#define PAGE_Y              28           /* a page's heading */
#define LINE_Y              56           /* and its first line */
#define LINE_STEP           14
#define LINE_LEFT           40
#define LINE_RIGHT          (SCREEN_WIDTH - 40)
#define STARTING_FRAMES     600          /* a game that has not started by then: back to the menu */
#define IDLE_FRAMES         1776         /* the menu left alone this long (30 seconds): INTRO plays */
#define PAUSE_Y             76           /* the pause's PAUSE, and its lines under it */
#define PAUSE_LINES_Y       108
#define PAUSE_LINE_STEP     14

/* (a device's lines past their lists, never chosen: SIMPSONS_SYSTEM_EXIT, SIMPSONS_HANDHELD) */
#ifdef SIMPSONS_SYSTEM_EXIT
enum { MENU_ARCADE, MENU_MR_PLOW, MENU_LAN, MENU_INTRO, MENU_OPTIONS, MENU_ITEMS, MENU_EXIT = 100 };
#else
enum { MENU_ARCADE, MENU_MR_PLOW, MENU_LAN, MENU_INTRO, MENU_OPTIONS, MENU_EXIT, MENU_ITEMS };
#endif
enum { PLOW_FREE_PLAY, PLOW_BOSS_RUSH, PLOW_BONUS_GAMES, PLOW_FRANK_GRIMES, PLOW_BACK, PLOW_ITEMS };    /* (the
	modes' order: MODE_FREE_PLAY on, console.c's way back to the page) */
#if defined(SIMPSONS_HANDHELD) && !defined(SIMPSONS_DEVICE_CONTROLS)
#define FIXED_CONTROLS    /* (no CONTROLS in OPTIONS) */
#endif
#ifdef FIXED_CONTROLS
enum { OPTION_DIFFICULTY, OPTION_LIVES, OPTION_CONTINUES, OPTION_DEMO_SOUND, OPTION_SCREEN, OPTION_VIDEO,
	OPTION_VOLUME, OPTION_SOUND, OPTION_BACK, OPTION_ITEMS, OPTION_CONTROLS = 100 };
#else
enum { OPTION_CONTROLS, OPTION_DIFFICULTY, OPTION_LIVES, OPTION_CONTINUES, OPTION_DEMO_SOUND, OPTION_SCREEN,
	OPTION_VIDEO, OPTION_VOLUME, OPTION_SOUND, OPTION_BACK, OPTION_ITEMS };
#endif
#ifdef SIMPSONS_HANDHELD
enum { VIDEO_PIXELS, VIDEO_INTEGER_SCALE, VIDEO_FPS, VIDEO_BACK, VIDEO_ITEMS, VIDEO_WINDOW = 100, VIDEO_FULL_SCREEN,
	VIDEO_WIDE, VIDEO_VSYNC };
#else
enum { VIDEO_WINDOW, VIDEO_FULL_SCREEN, VIDEO_PIXELS, VIDEO_INTEGER_SCALE, VIDEO_WIDE, VIDEO_VSYNC, VIDEO_FPS,
	VIDEO_BACK, VIDEO_ITEMS };
#endif
enum { KONAMI_STAGE, KONAMI_LIVES, KONAMI_HEALTH, KONAMI_REPEATS, KONAMI_START, KONAMI_BACK, KONAMI_ITEMS };
enum { INTRO_STORY, INTRO_ATTRACT, INTRO_BACK, INTRO_ITEMS };

#ifdef SIMPSONS_SYSTEM_EXIT
static const char *const menu_items[MENU_ITEMS] = { "ARCADE", "MR. PLOW MODE", "MULTIPLAYER", "INTRO", "OPTIONS" };
#else
static const char *const menu_items[MENU_ITEMS] = { "ARCADE", "MR. PLOW MODE", "MULTIPLAYER", "INTRO", "OPTIONS",
	"EXIT" };
#endif
static int idle_frames;    /* the menu's frames with nothing pressed (IDLE_FRAMES: INTRO) */
static const char *const intro_items[INTRO_ITEMS] = { "STORY", "ATTRACT", "BACK" };
static const char *const intro_about[INTRO_ITEMS][2] = {
	{ "SMITHERS TAKES THE DIAMOND", "AND MAGGIE WITH IT" },
	{ "THE ARCADE DEMO AND HIGH SCORES", "ANY BUTTON COMES BACK" },
	{ "", "" },
};
static bool story_picked;    /* INTRO's STORY picked on its page (not the menu left alone): back to the page */
static const char *const plow_items[PLOW_ITEMS] = { "FREE PLAY", "BOSS RUSH", "BONUS GAMES", "FRANK GRIMES LIFE",
	"BACK" };
/* (what the chosen one is, under the lines) */
static const char *const plow_about[PLOW_ITEMS][2] = {
	{ "THE ARCADE GAME FROM STAGE 1", "AS MANY CONTINUES AS YOU LIKE" },
	{ "THE BOSSES ONE AFTER ANOTHER", "FROM THE WRESTLER TO MR. BURNS" },
	{ "BLOW UP THE BALLOON, THEN WAKE UP", "1 TO 4 PLAYERS" },
	{ "ONE PLAYER, THE WHOLE GAME,", "AGAINST WHAT FOUR WOULD FACE" },
	{ "", "" },
};
#ifdef FIXED_CONTROLS
static const char *const option_items[OPTION_ITEMS] = { "DIFFICULTY", "LIVES", "CONTINUES", "DEMO SOUND", "SCREEN",
	"VIDEO", "VOLUME", "SOUND", "BACK" };
#else
static const char *const option_items[OPTION_ITEMS] = { "CONTROLS", "DIFFICULTY", "LIVES", "CONTINUES", "DEMO SOUND",
	"SCREEN", "VIDEO", "VOLUME", "SOUND", "BACK" };
#endif
#ifdef SIMPSONS_HANDHELD
static const char *const video_items[VIDEO_ITEMS] = { "PIXELS", "INTEGER SCALE", "FPS", "BACK" };
#else
static const char *const video_items[VIDEO_ITEMS] = { "WINDOW", "FULL SCREEN", "PIXELS", "INTEGER SCALE", "WIDE",
	"VSYNC", "FPS", "BACK" };
#endif
static const char *const konami_items[KONAMI_ITEMS] = { "STAGE", "LIVES", "HEALTH", "REPEAT CHARACTERS", "START",
	"BACK" };
/* (the test mode's own words: 8-3 DIFFICULTY's lines) */
static const char *const difficulties[CONSOLE_DIFFICULTIES] = { "EASY", "NORMAL", "DIFFICULT", "VERY DIFFICULT" };
static const char *const filters[PIXELS_KINDS] = { "SHARP", "SMOOTH", "BILINEAR", "CRT" };
#ifndef SIMPSONS_HANDHELD
static const char *const wide_kinds[WIDE_KINDS] = { "OFF", "16:9", "20:9", "16:10", "AUTO" };
#endif
static const int wide_order[WIDE_KINDS] = { WIDE_AUTO, WIDE_OFF, WIDE_16_10, WIDE_16_9, WIDE_20_9 };    /* (VIDEO's:
	AUTO, then narrow to wide; the enum's 16:10 came last, tmnt-port's) */

/* CONTROLS: PLAYER on top, the controls in two columns (each its name, and under it the pictures of its keys and
   button, the port's), DEFAULTS and BACK side by side at the bottom; as tmnt-port's */
enum { CONTROLS_PLAYER, CONTROLS_FIRST, CONTROLS_DEFAULTS = CONTROLS_FIRST + CONTROLS, CONTROLS_BACK };
#define CONTROLS_COLUMN_ROWS  4      /* UP, DOWN, LEFT, RIGHT; then ATTACK, JUMP, START */
#define CONTROLS_HEADING_Y    18
#define CONTROLS_PLAYER_Y     44
#define CONTROLS_CELL_Y       64     /* the first row of controls: their names */
#define CONTROLS_CELL_STEP    30     /* a name, its pictures under it, a gap */
#define CONTROLS_PICTURES_Y   10     /* under the name */
#define CONTROLS_BOTTOM_Y     194    /* DEFAULTS and BACK, or PRESS A KEY OR BUTTON */
#define CONTROLS_SETTLE       2      /* frames the menus take no action after a key or button is bound (it is held) */
static const int controls_column_x[2] = { 40, 176 };
static const char *const control_names[CONTROLS] = { "UP", "DOWN", "LEFT", "RIGHT", "ATTACK", "JUMP", "START" };
static int controls_player;        /* whose controls are shown */
static int controls_waiting = -1;  /* the control waiting for its key or button, or -1 */
static bool controls_defaults;     /* DEFAULTS picked, for the port */
static int controls_settle;        /* frames left without actions after a binding */
static bool settings_touched;      /* OPTIONS changed a cabinet's setting: into the game when it closes */

/* ---- the pages ---- */

static void open_page(int page, int choice)
{
	console.page = page;
	console.choice = choice;
	console.page_frames = 0;
}

static int step_value(int value, int by, int low, int high)
{
	value += by;
	return value < low ? high : (value > high ? low : value);
}

/* LEFT or RIGHT: -1, +1 or 0 */
static int change(uint32_t pressed)
{
	return (pressed & ACTION_BIT(ACTION_LEFT)) ? -1 : ((pressed & ACTION_BIT(ACTION_RIGHT)) ? 1 : 0);
}

static bool chosen_now(uint32_t pressed)
{
	return (pressed & (ACTION_BIT(ACTION_ACCEPT) | ACTION_BIT(ACTION_START))) != 0;
}

static void menu_page(uint32_t pressed)
{
	if (!chosen_now(pressed))
		return;
	switch (console.choice)
	{
		case MENU_ARCADE:
			console_start_arcade(console.last_presser);    /* (its coin: PAGE_STARTING) */
			break;
		case MENU_MR_PLOW:
			open_page(PAGE_MR_PLOW, PLOW_FREE_PLAY);
			break;
		case MENU_LAN:
			open_page(PAGE_LAN, 0);
			break;
		case MENU_INTRO:
			open_page(PAGE_INTRO, INTRO_STORY);
			break;
		case MENU_OPTIONS:
			open_page(PAGE_OPTIONS, 0);
			break;
		case MENU_EXIT:
			console.quit = true;
			break;
	}
}

/* INTRO: STORY, the story before stage 1 (modes.c), and ATTRACT, the cabinet's attract mode (console.c, the players'
   issue 22: "The attract Movie watchable within the intro option"); jump or BACK, the menu again */
static void intro_page(uint32_t pressed)
{
	if ((pressed & ACTION_BIT(ACTION_BACK)) || (chosen_now(pressed) && console.choice == INTRO_BACK))
	{
		open_page(PAGE_MENU, MENU_INTRO);
		return;
	}
	if (!chosen_now(pressed))
		return;
	if (console.choice == INTRO_STORY)
	{
		story_picked = true;
		intro_start(console.last_presser);
	}
	else if (console.choice == INTRO_ATTRACT)
		console_attract_watch();
}

void menu_after_intro(void)
{
	if (story_picked)
		open_page(PAGE_INTRO, INTRO_STORY);
	else
		open_page(PAGE_MENU, MENU_INTRO);
	story_picked = false;
}

/* MR. PLOW MODE: its modes (modes.c), each a game for the player who picks it, the character choice first; jump or
   BACK, the menu again */
static void plow_page(uint32_t pressed)
{
	if ((pressed & ACTION_BIT(ACTION_BACK)) || (chosen_now(pressed) && console.choice == PLOW_BACK))
	{
		open_page(PAGE_MENU, MENU_MR_PLOW);
		return;
	}
	if (!chosen_now(pressed))
		return;
	switch (console.choice)
	{
		case PLOW_FREE_PLAY:
			free_play_start(console.last_presser);
			break;
		case PLOW_BOSS_RUSH:
			boss_rush_start(console.last_presser);
			break;
		case PLOW_BONUS_GAMES:
			bonus_games_start(console.last_presser);
			break;
		case PLOW_FRANK_GRIMES:
			frank_grimes_start(console.last_presser);
			break;
		default:
			break;
	}
}

static void options_page(uint32_t pressed)
{
	const int by = change(pressed) ? change(pressed) : (chosen_now(pressed) ? 1 : 0);    /* (ACCEPT: the next value) */
	console_settings_t *settings = &console_settings;

	if ((pressed & ACTION_BIT(ACTION_BACK)) || (chosen_now(pressed) && console.choice == OPTION_BACK))
	{
		if (settings_touched)
			console_settings_into_game();
		settings_touched = false;
		settings->changed = true;    /* (the port keeps them) */
		open_page(PAGE_MENU, MENU_OPTIONS);
		return;
	}
	if (chosen_now(pressed))
		switch (console.choice)
		{
			case OPTION_CONTROLS:
				open_page(PAGE_CONTROLS, 0);
				return;
			case OPTION_VIDEO:
				open_page(PAGE_VIDEO, 0);
				return;
			default:
				break;
		}
	if (!by)
		return;
	switch (console.choice)
	{
		case OPTION_DIFFICULTY:
			settings->game_difficulty = step_value(settings->game_difficulty, by, 0, CONSOLE_DIFFICULTIES - 1);
			settings_touched = true;
			break;
		case OPTION_LIVES:
			settings->game_lives = step_value(settings->game_lives, by, CONSOLE_LIVES_FEWEST, CONSOLE_LIVES_MOST);
			settings_touched = true;
			break;
		case OPTION_CONTINUES:
		{
			/* (0 to 9, then UNLIMITED: the console's, ARCADE's continues as its game starts) */
			const int unlimited = CONSOLE_CONTINUES_MOST + 1;
			const int place = settings->game_continues == CONSOLE_CONTINUES_UNLIMITED ? unlimited
				: settings->game_continues;
			const int next = step_value(place, by, 0, unlimited);

			settings->game_continues = next == unlimited ? CONSOLE_CONTINUES_UNLIMITED : next;
			settings->changed = true;
			break;
		}
		case OPTION_DEMO_SOUND:
			settings->demo_sound = !settings->demo_sound;
			settings_touched = true;
			break;
		case OPTION_SCREEN:
			settings->upside_down = !settings->upside_down;
			settings_touched = true;
			break;
		case OPTION_VOLUME:
			settings->volume = step_value(settings->volume, by, 0, 10);
			settings->changed = true;
			break;
		case OPTION_SOUND:
			settings->stereo = !settings->stereo;    /* (the port's: STEREO, the YM2151's two outputs) */
			settings->changed = true;
			break;
		default:
			break;
	}
}

/* VIDEO: the window's size (1 to 6 times the picture), full screen, the pixels, INTEGER SCALE, WIDE, VSYNC, FPS; the
   port applies them */
static void video_page(uint32_t pressed)
{
	const int by = change(pressed) ? change(pressed) : (chosen_now(pressed) ? 1 : 0);
	console_settings_t *settings = &console_settings;

	if ((pressed & ACTION_BIT(ACTION_BACK)) || (chosen_now(pressed) && console.choice == VIDEO_BACK))
	{
		open_page(PAGE_OPTIONS, OPTION_VIDEO);
		return;
	}
	if (!by)
		return;
	switch (console.choice)
	{
		case VIDEO_WINDOW:
			settings->window_scale = step_value(settings->window_scale, by, 1, 6);
			break;
		case VIDEO_FULL_SCREEN:
			settings->full_screen = !settings->full_screen;
			break;
		case VIDEO_PIXELS:
			settings->pixels = step_value(settings->pixels, by, 0, PIXELS_KINDS - 1);
			break;
		case VIDEO_INTEGER_SCALE:
			settings->integer_scale = !settings->integer_scale;
			break;
		case VIDEO_WIDE:
		{
			int place = 0;

			while (place < WIDE_KINDS - 1 && wide_order[place] != settings->wide)
				place++;
			settings->wide = wide_order[step_value(place, by, 0, WIDE_KINDS - 1)];
			break;
		}
		case VIDEO_VSYNC:
			settings->vsync = !settings->vsync;
			break;
		case VIDEO_FPS:
			settings->fps = !settings->fps;
			break;
		default:
			return;
	}
	settings->changed = true;
}

/* ---- MULTIPLAYER (lan.c) ---- */

/* MULTIPLAYER: HOST, JOIN, NAME, BACK; what went wrong last under them. NAME: UP and DOWN change the letter, LEFT and
   RIGHT go to the one beside it, ACCEPT keeps the name, BACK leaves it as it was. JOIN: the rooms found, one picked;
   the room: its devices, the host's START (two devices or more) and LEAVE. As tmnt-port's */
enum { LAN_HOST, LAN_JOIN, LAN_NAME, LAN_BACK, LAN_ITEMS };
static const char *const lan_items[LAN_ITEMS] = { "HOST", "JOIN", "NAME", "BACK" };
static const char name_letters[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-";
static char name_edit[CONSOLE_NAME_LENGTH + 1];    /* NAME: the name being written */
static int name_at;
static int name_then = -1;    /* (HOST or JOIN, asked with no name yet: what NAME goes on to once kept; -1 none) */
static int lan_blink;

static const char *own_name(void)
{
	return console_settings.name[0] ? console_settings.name : CONSOLE_DEFAULT_NAME;
}

/* HOST: a room of its own; JOIN: the rooms looked for */
static void host_or_join(int item)
{
	if (item == LAN_HOST)
	{
		const bool hosting = console_lan_host();

		open_page(hosting ? PAGE_LAN_ROOM : PAGE_LAN, hosting ? 0 : LAN_HOST);
		return;
	}
	console_lan_search();
	open_page(console_lan_searching() ? PAGE_LAN_ROOMS : PAGE_LAN, console_lan_searching() ? 0 : LAN_JOIN);
}

/* NAME, its letters the name kept (blank when none yet), 'then' HOST or JOIN once kept (-1: the page) */
static void open_name(int then)
{
	snprintf(name_edit, sizeof(name_edit), "%-*s", CONSOLE_NAME_LENGTH, console_settings.name);
	name_at = 0;
	name_then = then;
	open_page(PAGE_LAN_NAME, 0);
}

static void lan_page(uint32_t pressed)
{
	if (pressed & (ACTION_BIT(ACTION_UP) | ACTION_BIT(ACTION_DOWN) | ACTION_BIT(ACTION_ACCEPT) | ACTION_BIT(ACTION_START)))
		console_lan_problem_seen();
	if ((pressed & ACTION_BIT(ACTION_BACK)) || (chosen_now(pressed) && console.choice == LAN_BACK))
	{
		console_lan_leave();
		open_page(PAGE_MENU, MENU_LAN);
		return;
	}
	if (!chosen_now(pressed))
		return;
	switch (console.choice)
	{
		case LAN_HOST:
		case LAN_JOIN:
			if (!console_settings.name[0])
				open_name(console.choice);    /* (the player's name first, the one the room shows) */
			else
				host_or_join(console.choice);
			break;
		case LAN_NAME:
			open_name(-1);
			break;
		default:
			break;
	}
}

static int letter_of(char letter)
{
	for (int index = 0; name_letters[index]; index++)
		if (name_letters[index] == letter)
			return index;
	return 0;
}

static void lan_name_page(uint32_t pressed)
{
	const int letters = (int)sizeof(name_letters) - 1;

	if (pressed & ACTION_BIT(ACTION_BACK))
	{
		open_page(PAGE_LAN, name_then >= 0 ? name_then : LAN_NAME);
		name_then = -1;
		return;
	}
	if (chosen_now(pressed))
	{
		const int then = name_then;
		int length = CONSOLE_NAME_LENGTH;

		while (length > 0 && name_edit[length - 1] == ' ')
			length--;
		if (length > 0 || !console_settings.name[0])
		{
			/* (none written: PLAYER, not asked again) */
			snprintf(console_settings.name, sizeof(console_settings.name), "%.*s",
				length ? length : (int)strlen(CONSOLE_DEFAULT_NAME), length ? name_edit : CONSOLE_DEFAULT_NAME);
			console_settings.changed = true;    /* (the port keeps it) */
			console_lan_name_changed();
		}
		name_then = -1;
		if (then >= 0)
			host_or_join(then);
		else
			open_page(PAGE_LAN, LAN_NAME);
		return;
	}
	if (pressed & (ACTION_BIT(ACTION_UP) | ACTION_BIT(ACTION_DOWN)))
		name_edit[name_at] = name_letters[step_value(letter_of(name_edit[name_at]),
			(pressed & ACTION_BIT(ACTION_UP)) ? 1 : -1, 0, letters - 1)];
	name_at = step_value(name_at, change(pressed), 0, CONSOLE_NAME_LENGTH - 1);
}

static void lan_rooms_page(uint32_t pressed)
{
	const int rooms = console_lan_room_count();

	if (!console_lan_searching())
	{
		open_page(PAGE_LAN, LAN_JOIN);
		return;
	}
	if (console.choice > rooms)
		console.choice = rooms;    /* (a room forgotten: BACK where it was) */
	if ((pressed & ACTION_BIT(ACTION_BACK)) || (chosen_now(pressed) && console.choice >= rooms))
	{
		console_lan_leave();
		open_page(PAGE_LAN, LAN_JOIN);
		return;
	}
	if (chosen_now(pressed) && console_lan_join(console.choice))
		open_page(PAGE_LAN_ROOM, 0);
}

static void lan_room_page(uint32_t pressed)
{
	const bool host = console_lan_hosting();

	if (console_lan_reset_wanted())
		return;    /* (its game started: the board reset at this frame's end) */
	if (!console_lan_in_room())
	{
		open_page(PAGE_LAN, 0);
		return;
	}
	if ((pressed & ACTION_BIT(ACTION_BACK)) || (chosen_now(pressed) && console.choice == (host ? 1 : 0)))
	{
		console_lan_leave();
		open_page(PAGE_LAN, 0);
		return;
	}
	if (chosen_now(pressed) && host && console.choice == 0 && console_lan_can_start())
		console_lan_start();
}

/* ---- the Konami code ---- */

/* The last presses, the newest last (0xFF: none, or several at once), one a frame; the menu's line it was entered on
   (BACK from its page goes back there) */
#define CODE_LENGTH         10
static const uint8_t konami_code[CODE_LENGTH] = { ACTION_UP, ACTION_UP, ACTION_DOWN, ACTION_DOWN, ACTION_LEFT,
	ACTION_RIGHT, ACTION_LEFT, ACTION_RIGHT, ACTION_BACK, ACTION_ACCEPT };
static uint8_t code_pressed[CODE_LENGTH];
static bool code_ready;    /* (none pressed at first: 0xFF set at the first press, no table of the executable's) */
static int code_choice;

bool konami_code_entered(uint32_t pressed)
{
	const uint32_t code_keys = ACTION_BIT(ACTION_UP) | ACTION_BIT(ACTION_DOWN) | ACTION_BIT(ACTION_LEFT)
		| ACTION_BIT(ACTION_RIGHT) | ACTION_BIT(ACTION_BACK) | ACTION_BIT(ACTION_ACCEPT) | ACTION_BIT(ACTION_START);
	uint8_t action = 0xFFu;

	pressed &= code_keys;
	if (!pressed)
		return false;
	if (!code_ready)
	{
		memset(code_pressed, 0xFF, sizeof(code_pressed));
		code_ready = true;
	}
	for (int bit = 0; bit < ACTIONS; bit++)
		if (pressed == ACTION_BIT(bit))
			action = (uint8_t)bit;
	memmove(code_pressed, code_pressed + 1, CODE_LENGTH - 1);
	code_pressed[CODE_LENGTH - 1] = action;
	if (memcmp(code_pressed, konami_code, CODE_LENGTH) != 0)
		return false;
	memset(code_pressed, 0xFF, sizeof(code_pressed));
	return true;
}

void konami_page_open(int menu_line)
{
	code_choice = menu_line;
	console.code_entered = true;    /* (its lives, health and characters for MR. PLOW MODE's games too: modes.c) */
	queue_sound_always(CODE_SOUND);
	open_page(PAGE_KONAMI, KONAMI_STAGE);
}

/* LEFT and RIGHT (or attack) change a line's value; START, or attack on START, starts its game (the character choice);
   jump or BACK, the menu again */
void konami_page_frame(uint32_t pressed)
{
	const int by = change(pressed) ? change(pressed)
		: ((pressed & ACTION_BIT(ACTION_ACCEPT)) ? 1 : 0);

	if ((pressed & ACTION_BIT(ACTION_BACK)) || (chosen_now(pressed) && console.choice == KONAMI_BACK))
	{
		open_page(PAGE_MENU, code_choice);
		return;
	}
	if ((pressed & ACTION_BIT(ACTION_START)) || (chosen_now(pressed) && console.choice == KONAMI_START))
	{
		console.konami = true;
		console.mode = MODE_NONE;
		console_start_game(console.last_presser, konami_stage_step(console.konami_stage), MODE_LIVES,
			console.infinite_lives ? CONTINUES_UNLIMITED : MODE_CONTINUES);
		return;
	}
	if (!by)
		return;
	switch (console.choice)
	{
		case KONAMI_STAGE:
			console.konami_stage = step_value(console.konami_stage, by, 0, KONAMI_STAGES - 1);
			break;
		case KONAMI_LIVES:
			console.infinite_lives = !console.infinite_lives;
			break;
		case KONAMI_HEALTH:
			console.infinite_health = !console.infinite_health;
			break;
		case KONAMI_REPEATS:
			console.repeats = !console.repeats;
			break;
		default:
			break;
	}
}

/* ---- CONTROLS ---- */

static void control_place(int control, int *x, int *y)
{
	*x = controls_column_x[control / CONTROLS_COLUMN_ROWS];
	*y = CONTROLS_CELL_Y + (control % CONTROLS_COLUMN_ROWS) * CONTROLS_CELL_STEP;
}

/* The arrows on the grid: up and down within a column (to PLAYER above it, to DEFAULTS or BACK under it), left and
   right across the columns (on PLAYER: the player) */
static void controls_move(uint32_t pressed)
{
	const int choice = console.choice;
	const int control = choice - CONTROLS_FIRST;
	const bool on_control = choice >= CONTROLS_FIRST && choice < CONTROLS_DEFAULTS;
	const int column = on_control ? control / CONTROLS_COLUMN_ROWS : (choice == CONTROLS_BACK ? 1 : 0);
	const int column_last = column ? CONTROLS - 1 : CONTROLS_COLUMN_ROWS - 1;

	if (pressed & ACTION_BIT(ACTION_UP))
	{
		if (choice == CONTROLS_DEFAULTS || choice == CONTROLS_BACK)
			console.choice = CONTROLS_FIRST + column_last;
		else if (on_control && control % CONTROLS_COLUMN_ROWS != 0)
			console.choice = choice - 1;
		else if (on_control)
			console.choice = CONTROLS_PLAYER;
		else
			console.choice = CONTROLS_DEFAULTS;    /* (from PLAYER round to the bottom) */
	}
	else if (pressed & ACTION_BIT(ACTION_DOWN))
	{
		if (choice == CONTROLS_PLAYER)
			console.choice = CONTROLS_FIRST;
		else if (on_control && control != column_last)
			console.choice = choice + 1;
		else if (on_control)
			console.choice = column ? CONTROLS_BACK : CONTROLS_DEFAULTS;
		else
			console.choice = CONTROLS_PLAYER;
	}
	else if (change(pressed))
	{
		if (choice == CONTROLS_PLAYER)
			controls_player = step_value(controls_player, change(pressed), 0, CONTROL_PLAYERS - 1);
		else if (on_control)
		{
			const int row = control % CONTROLS_COLUMN_ROWS;
			const int other = column ? row : CONTROLS_COLUMN_ROWS + row;

			console.choice = CONTROLS_FIRST + (other > CONTROLS - 1 ? CONTROLS - 1 : other);
		}
		else
			console.choice = choice == CONTROLS_DEFAULTS ? CONTROLS_BACK : CONTROLS_DEFAULTS;
	}
}

static void controls_page(uint32_t pressed)
{
	if (controls_waiting >= 0)
		return;    /* (the port takes the next key or button: console_controls_bound) */
	if (controls_settle > 0)
	{
		controls_settle--;
		return;
	}
	if ((pressed & ACTION_BIT(ACTION_BACK)) || (chosen_now(pressed) && console.choice == CONTROLS_BACK))
	{
		open_page(PAGE_OPTIONS, OPTION_CONTROLS);
		return;
	}
	controls_move(pressed);
	if (!chosen_now(pressed))
		return;
	if (console.choice >= CONTROLS_FIRST && console.choice < CONTROLS_DEFAULTS)
		controls_waiting = console.choice - CONTROLS_FIRST;
	else if (console.choice == CONTROLS_DEFAULTS)
		controls_defaults = true;
}

bool console_controls_place(int control, int *x, int *y)
{
	if (console.page != PAGE_CONTROLS || game_mode != GAME_MODE_ATTRACT || controls_waiting >= 0 || control < 0
		|| control >= CONTROLS)
		return false;
	control_place(control, x, y);
	*y += CONTROLS_PICTURES_Y;
	return true;
}

int console_controls_player(void)
{
	return controls_player;
}

int console_controls_waiting(void)
{
	return console.page == PAGE_CONTROLS ? controls_waiting : -1;
}

void console_controls_bound(void)
{
	controls_waiting = -1;
	controls_settle = CONTROLS_SETTLE;
}

bool console_controls_defaults(void)
{
	const bool chosen = controls_defaults;

	controls_defaults = false;
	return chosen;
}

/* ARCADE's coin going in: the game starts in the attract mode itself (console.c), or, when it has not after a while,
   the menu again */
static void starting_page(void)
{
	if (console.page_frames > STARTING_FRAMES)
	{
		if (console.mode == MODE_INTRO)
			menu_after_intro();
		else
			open_page(PAGE_MENU, MENU_ARCADE);
		console.mode = MODE_NONE;    /* (INTRO's black off) */
	}
}

void menu_frame(uint32_t pressed, const uint32_t player_pressed[CONSOLE_PLAYERS])
{
	const int page = console.page;
	int lines = 0;

	(void)player_pressed;    /* (each player's own: the character choice's, characters.c) */
	console.page_frames++;
	lan_blink++;
	if (page >= PAGE_LAN && page <= PAGE_LAN_ROOM)
		console_lan_update();    /* (the rooms' datagrams, the game's start) */
	switch (page)
	{
		case PAGE_MENU: lines = MENU_ITEMS; break;
		case PAGE_MR_PLOW: lines = PLOW_ITEMS; break;
		case PAGE_LAN: lines = LAN_ITEMS; break;
		case PAGE_LAN_ROOMS: lines = console_lan_room_count() + 1; break;
		case PAGE_LAN_ROOM: lines = console_lan_hosting() ? 2 : 1; break;
		case PAGE_OPTIONS: lines = OPTION_ITEMS; break;
		case PAGE_VIDEO: lines = VIDEO_ITEMS; break;
		case PAGE_KONAMI: lines = KONAMI_ITEMS; break;
		case PAGE_INTRO: lines = INTRO_ITEMS; break;
		default: break;
	}
	if (lines)
	{
		if (pressed & ACTION_BIT(ACTION_UP))
			console.choice = step_value(console.choice, -1, 0, lines - 1);
		if (pressed & ACTION_BIT(ACTION_DOWN))
			console.choice = step_value(console.choice, 1, 0, lines - 1);
	}
	/* (the menu left alone, nothing pressed for IDLE_FRAMES: INTRO, Sebas: "si esta inactivo, que meta una intro") */
	idle_frames = page == PAGE_MENU && !pressed ? idle_frames + 1 : 0;
	if (idle_frames >= IDLE_FRAMES)
	{
		idle_frames = 0;
		story_picked = false;
		intro_start(0);
		return;
	}
	switch (page)
	{
		case PAGE_MENU:
			if (konami_code_entered(pressed))
				konami_page_open(console.choice);    /* (its ups and downs took the line there and back, its A picks
				                                        nothing) */
			else
				menu_page(pressed);
			break;
		case PAGE_MR_PLOW:
			plow_page(pressed);
			break;
		case PAGE_INTRO:
			intro_page(pressed);
			break;
		case PAGE_KONAMI:
			konami_page_frame(pressed);
			break;
		case PAGE_LAN:
			lan_page(pressed);
			break;
		case PAGE_LAN_NAME:
			lan_name_page(pressed);
			break;
		case PAGE_LAN_ROOMS:
			lan_rooms_page(pressed);
			break;
		case PAGE_LAN_ROOM:
			lan_room_page(pressed);
			break;
		case PAGE_OPTIONS:
			options_page(pressed);
			break;
		case PAGE_VIDEO:
			video_page(pressed);
			break;
		case PAGE_CONTROLS:
			controls_page(pressed);
			break;
		case PAGE_STARTING:
			starting_page();
			break;
		default:
			break;
	}
}

/* ---- the picture ---- */

static console_picture *target;    /* the frame being drawn over */

/* The menu's lines, centred, the one chosen in yellow, 'step' rows apart */
static void list_lines(const char *const *items, int count, int y, int step)
{
	for (int index = 0; index < count; index++)
		draw_game_letters_centred(target, y + index * step, 1, index == console.choice ? CHOSEN_COLOUR : ITEM_COLOUR,
			items[index]);
}

/* A page: its heading twice the letters' size over its 'lines', the picture darkened behind them */
static void page_box(const char *heading, int lines)
{
	draw_shade(target, LINE_LEFT - 12, PAGE_Y - 8, LINE_RIGHT - LINE_LEFT + 24, LINE_Y - PAGE_Y + lines * LINE_STEP + 10,
		64);
	draw_game_letters_centred(target, PAGE_Y, 2, HEADING_COLOUR, heading);
}

/* A line of a page: its name at the left, its value (or none: a page of its own, or BACK) at the right */
static void option_line(const char *const *items, int index, const char *value)
{
	const uint32_t colour = index == console.choice ? CHOSEN_COLOUR : ITEM_COLOUR;
	const int y = LINE_Y + index * LINE_STEP;

	draw_game_letters(target, LINE_LEFT, y, 1, colour, items[index]);
	if (value)
		draw_game_letters(target, LINE_RIGHT - draw_game_letters_width(value, 1), y, 1, colour, value);
}

/* The menu on the TV's screen (the TV and the title's logo on it: tv.c) */
static void menu_screen(void)
{
	list_lines(menu_items, MENU_ITEMS, MENU_Y, ITEM_STEP);
	draw_game_letters_centred(target, COPYRIGHT_Y, 1, COPYRIGHT_COLOUR, COPYRIGHT);
	draw_small_letters_centred(target, SIGNATURE_Y, SIGNATURE_COLOUR, SIGNATURE);
}

/* INTRO: its heading, its lines centred, and under them what the chosen one is (as MR. PLOW MODE's) */
#define INTRO_ABOUT_Y       (LINE_Y + INTRO_ITEMS * LINE_STEP + 10)

static void intro_overlay(void)
{
	page_box("INTRO", INTRO_ITEMS + 3);
	list_lines(intro_items, INTRO_ITEMS, LINE_Y, LINE_STEP);
	for (int line = 0; line < 2; line++)
		draw_game_letters_centred(target, INTRO_ABOUT_Y + line * 12, 1, 0xC0C0C0u,
			intro_about[console.choice >= 0 && console.choice < INTRO_ITEMS ? console.choice : INTRO_BACK][line]);
}

/* MR. PLOW MODE: its heading, its lines centred as the menu's, and under them what the chosen one is */
#define PLOW_ABOUT_Y        (LINE_Y + PLOW_ITEMS * LINE_STEP + 10)
#define PLOW_ABOUT_STEP     12
#define PLOW_ABOUT_COLOUR   0xC0C0C0u

static void plow_overlay(void)
{
	page_box("MR. PLOW MODE", PLOW_ITEMS + 3);
	list_lines(plow_items, PLOW_ITEMS, LINE_Y, LINE_STEP);
	for (int line = 0; line < 2; line++)
		draw_game_letters_centred(target, PLOW_ABOUT_Y + line * PLOW_ABOUT_STEP, 1, PLOW_ABOUT_COLOUR,
			plow_about[console.choice >= 0 && console.choice < PLOW_ITEMS ? console.choice : PLOW_BACK][line]);
}

static void options_overlay(void)
{
	char value[40];

	page_box("OPTIONS", OPTION_ITEMS);
#ifndef FIXED_CONTROLS
	option_line(option_items, OPTION_CONTROLS, NULL);
#endif
	option_line(option_items, OPTION_DIFFICULTY, difficulties[console_settings.game_difficulty]);
	snprintf(value, sizeof(value), "%d", console_settings.game_lives);
	option_line(option_items, OPTION_LIVES, value);
	if (console_settings.game_continues == CONSOLE_CONTINUES_UNLIMITED)
		snprintf(value, sizeof(value), "UNLIMITED");
	else
		snprintf(value, sizeof(value), "%d", console_settings.game_continues);
	option_line(option_items, OPTION_CONTINUES, value);
	option_line(option_items, OPTION_DEMO_SOUND, console_settings.demo_sound ? "ON" : "OFF");
	option_line(option_items, OPTION_SCREEN, console_settings.upside_down ? "UPSIDE DOWN" : "NORMAL");
	option_line(option_items, OPTION_VIDEO, NULL);
	snprintf(value, sizeof(value), "%d", console_settings.volume);
	option_line(option_items, OPTION_VOLUME, value);
	option_line(option_items, OPTION_SOUND, console_settings.stereo ? "STEREO" : "MONO");
	option_line(option_items, OPTION_BACK, NULL);
	draw_game_letters(target, LINE_RIGHT - draw_game_letters_width(VERSION_TEXT, 1), LINE_Y + OPTION_BACK * LINE_STEP,
		1, VERSION_COLOUR, VERSION_TEXT);    /* (in the panel, at BACK's right: the board's middle, in every WIDE) */
}

static void video_overlay(void)
{
	char value[16];

	page_box("VIDEO", VIDEO_ITEMS);
#ifndef SIMPSONS_HANDHELD
	snprintf(value, sizeof(value), "%dX", console_settings.window_scale);
	option_line(video_items, VIDEO_WINDOW, value);
	option_line(video_items, VIDEO_FULL_SCREEN, console_settings.full_screen ? "ON" : "OFF");
#else
	(void)value;
#endif
	option_line(video_items, VIDEO_PIXELS, filters[console_settings.pixels]);
	option_line(video_items, VIDEO_INTEGER_SCALE, console_settings.integer_scale ? "ON" : "OFF");
#ifndef SIMPSONS_HANDHELD
	if (console_settings.wide == WIDE_AUTO)
		snprintf(value, sizeof(value), "AUTO %s", wide_kinds[console_wide_in_effect()]);    /* (what the screen's shape
			asks for) */
	else
		snprintf(value, sizeof(value), "%s", wide_kinds[console_settings.wide > 0 && console_settings.wide < WIDE_KINDS
			? console_settings.wide : WIDE_OFF]);
	option_line(video_items, VIDEO_WIDE, value);
	option_line(video_items, VIDEO_VSYNC, console_settings.vsync ? "ON" : "OFF");
#endif
	option_line(video_items, VIDEO_FPS, console_settings.fps ? "ON" : "OFF");
	option_line(video_items, VIDEO_BACK, NULL);
}

/* MULTIPLAYER's pages (as tmnt-port's) */
static void lan_overlay(void)
{
	const char *problem = console_lan_problem();

	page_box("MULTIPLAYER", LAN_ITEMS + 2);
	option_line(lan_items, LAN_HOST, NULL);
	option_line(lan_items, LAN_JOIN, NULL);
	option_line(lan_items, LAN_NAME, own_name());
	option_line(lan_items, LAN_BACK, NULL);
	if (problem)
		draw_game_letters_centred(target, LINE_Y + (LAN_ITEMS + 1) * LINE_STEP, 1, CHOSEN_COLOUR, problem);
}

static void lan_name_overlay(void)
{
	const int scale = 2, letter = draw_game_letters_width("A", scale);
	int x = draw_centred_x(target, CONSOLE_NAME_LENGTH * letter);
	char shown[2] = { 0, 0 };

	page_box(name_then >= 0 ? "YOUR NAME" : "NAME", 4);
	for (int at = 0; at < CONSOLE_NAME_LENGTH; at++, x += letter)
	{
		const uint32_t colour = at == name_at ? CHOSEN_COLOUR : ITEM_COLOUR;

		shown[0] = name_edit[at];
		if (shown[0] != ' ')
			draw_game_letters(target, x, LINE_Y + LINE_STEP, scale, colour, shown);
		draw_fill(target, x + 1, LINE_Y + LINE_STEP + 17, letter - 3, 2, at == name_at ? CHOSEN_COLOUR : 0x8080A0u);
	}
	draw_game_letters_centred(target, LINE_Y + 3 * LINE_STEP + 4, 1, ITEM_COLOUR, "UP DOWN: LETTER  ACCEPT: OK");
}

static void lan_rooms_overlay(void)
{
	const int rooms = console_lan_room_count();
	char text[40];

	page_box("JOIN", LAN_ITEMS + 2);
	for (int room = 0; room < rooms; room++)
	{
		console_lan_room_text(room, text, sizeof(text));
		draw_game_letters(target, LINE_LEFT, LINE_Y + room * LINE_STEP, 1,
			room == console.choice ? CHOSEN_COLOUR : ITEM_COLOUR, text);
	}
	draw_game_letters(target, LINE_LEFT, LINE_Y + rooms * LINE_STEP, 1,
		console.choice == rooms ? CHOSEN_COLOUR : ITEM_COLOUR, "BACK");
	if (rooms == 0 && (lan_blink & 32) == 0)
		draw_game_letters_centred(target, LINE_Y + 3 * LINE_STEP, 1, ITEM_COLOUR, "LOOKING FOR ROOMS");
}

static void lan_room_overlay(void)
{
	const bool host = console_lan_hosting();
	const int members = console_lan_member_count();
	const char *problem = console_lan_problem();
	char text[40];

	page_box(console_lan_joining() ? "JOINING" : "ROOM", LAN_ITEMS + 4);
	for (int index = 0; index < members; index++)
	{
		console_lan_member_text(index, text, sizeof(text));
		draw_game_letters(target, LINE_LEFT, LINE_Y + index * LINE_STEP, 1, ITEM_COLOUR, text);
	}
	if (host)
	{
		draw_game_letters(target, LINE_LEFT, LINE_Y + 5 * LINE_STEP, 1,
			console.choice == 0 ? CHOSEN_COLOUR : ITEM_COLOUR, console_lan_can_start() ? "START" : "START: 2 OR MORE");
		draw_game_letters(target, LINE_LEFT, LINE_Y + 6 * LINE_STEP, 1,
			console.choice == 1 ? CHOSEN_COLOUR : ITEM_COLOUR, "LEAVE");
	}
	else
	{
		if ((lan_blink & 32) == 0)
			draw_game_letters_centred(target, LINE_Y + 5 * LINE_STEP, 1, ITEM_COLOUR, "THE HOST STARTS");
		draw_game_letters(target, LINE_LEFT, LINE_Y + 6 * LINE_STEP, 1, CHOSEN_COLOUR, "LEAVE");
	}
	if (problem)
		draw_game_letters_centred(target, LINE_Y + 7 * LINE_STEP, 1, CHOSEN_COLOUR, problem);
}

/* KONAMI CODE: its heading, and its lines as OPTIONS' */
void konami_page_overlay(console_picture *frame)
{
	target = frame;
	page_box("KONAMI CODE", KONAMI_ITEMS);
	option_line(konami_items, KONAMI_STAGE, konami_stage_name(console.konami_stage));
	option_line(konami_items, KONAMI_LIVES, console.infinite_lives ? "INFINITE" : "NORMAL");
	option_line(konami_items, KONAMI_HEALTH, console.infinite_health ? "INFINITE" : "NORMAL");
	option_line(konami_items, KONAMI_REPEATS, console.repeats ? "ON" : "OFF");
	option_line(konami_items, KONAMI_START, NULL);
	option_line(konami_items, KONAMI_BACK, NULL);
}

static void controls_overlay(void)
{
	char value[16];
	int x, y;

	draw_shade(target, 16, CONTROLS_HEADING_Y - 8, SCREEN_WIDTH - 32, CONTROLS_BOTTOM_Y - CONTROLS_HEADING_Y + 18, 64);
	draw_game_letters_centred(target, CONTROLS_HEADING_Y, 2, HEADING_COLOUR, "CONTROLS");
	if (controls_waiting >= 0)
	{
		control_place(controls_waiting, &x, &y);
		draw_game_letters(target, x, y, 1, CHOSEN_COLOUR, control_names[controls_waiting]);
#ifdef SIMPSONS_DEVICE_CONTROLS
		draw_game_letters_centred(target, CONTROLS_BOTTOM_Y, 1, ITEM_COLOUR, "PRESS A BUTTON");    /* (the shown
			player's own controller's; another's leaves the control as it was: the port's) */
#else
		draw_game_letters_centred(target, CONTROLS_BOTTOM_Y, 1, ITEM_COLOUR, "PRESS A KEY OR BUTTON");
#endif
		return;
	}
	snprintf(value, sizeof(value), "< PLAYER %d >", controls_player + 1);    /* (LEFT and RIGHT: players 1-4) */
	draw_game_letters_centred(target, CONTROLS_PLAYER_Y, 1, console.choice == CONTROLS_PLAYER ? CHOSEN_COLOUR
		: ITEM_COLOUR, value);
	for (int control = 0; control < CONTROLS; control++)
	{
		control_place(control, &x, &y);
		draw_game_letters(target, x, y, 1, console.choice == CONTROLS_FIRST + control ? CHOSEN_COLOUR : ITEM_COLOUR,
			control_names[control]);
	}
	draw_game_letters(target, controls_column_x[0], CONTROLS_BOTTOM_Y, 1,
		console.choice == CONTROLS_DEFAULTS ? CHOSEN_COLOUR : ITEM_COLOUR, "DEFAULTS");
	draw_game_letters(target, controls_column_x[1], CONTROLS_BOTTOM_Y, 1,
		console.choice == CONTROLS_BACK ? CHOSEN_COLOUR : ITEM_COLOUR, "BACK");
}

/* The pause's picture: the frame it stopped at in grey, PAUSE and its lines over it in the game's letters (as
   tmnt-port's) */
void console_pause_overlay(console_picture *frame, int choice)
{
	static const char *const items[CONSOLE_PAUSE_ITEMS] = { "CONTINUE", "SAVE STATE", "LOAD STATE", "RESTART",
		"QUIT GAME" };
	const int width = SIMPSONS_VIDEO_WIDTH + frame->left + frame->right;

	for (int y = 0; y < SIMPSONS_VIDEO_HEIGHT; y++)
		for (int x = 0; x < width; x++)
		{
			uint32_t *pixel = &frame->pixels[y * frame->stride + x];
			const uint32_t colour = *pixel;
			const uint32_t level = ((((colour >> 16) & 0xFFu) * 77u + ((colour >> 8) & 0xFFu) * 150u
				+ (colour & 0xFFu) * 29u) >> 8) * 3u / 5u;

			*pixel = level << 16 | level << 8 | level;
		}
	draw_game_letters_centred(frame, PAUSE_Y, 2, HEADING_COLOUR, "PAUSE");
	for (int item = 0; item < CONSOLE_PAUSE_ITEMS; item++)
		draw_game_letters_centred(frame, PAUSE_LINES_Y + item * PAUSE_LINE_STEP, 1,
			item == choice ? CHOSEN_COLOUR : ITEM_COLOUR, items[item]);
}

void menu_overlay(console_picture *frame)
{
	target = frame;
	if (game_mode != GAME_MODE_ATTRACT || console.page == PAGE_ATTRACT)
		return;    /* (INTRO > ATTRACT: the cabinet's attract mode alone) */
	if (console.page == PAGE_STARTING || !tv_shown())
	{
		/* (the screen cleared for the TV, or for the game's start: black, not the screens the game leaves) */
		draw_fill(frame, PICTURE_FIRST(frame), 0, PICTURE_END(frame) - PICTURE_FIRST(frame), SIMPSONS_VIDEO_HEIGHT,
			0x000000u);
		return;
	}
	switch (console.page)
	{
		case PAGE_MENU:
			menu_screen();
			break;
		case PAGE_MR_PLOW:
			plow_overlay();
			break;
		case PAGE_INTRO:
			intro_overlay();
			break;
		case PAGE_OPTIONS:
			options_overlay();
			break;
		case PAGE_VIDEO:
			video_overlay();
			break;
		case PAGE_CONTROLS:
			controls_overlay();
			break;
		case PAGE_KONAMI:
			konami_page_overlay(frame);
			break;
		case PAGE_LAN:
			lan_overlay();
			break;
		case PAGE_LAN_NAME:
			lan_name_overlay();
			break;
		case PAGE_LAN_ROOMS:
			lan_rooms_overlay();
			break;
		case PAGE_LAN_ROOM:
			lan_room_overlay();
			break;
		default:
			break;
	}
}
