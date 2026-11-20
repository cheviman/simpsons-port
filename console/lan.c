/**
 *    Name:        lan.c
 *    Description: MULTIPLAYER, the LAN game (console/PLAN.md, 11): the menu's MULTIPLAYER, its rooms through
 *                 arcade-lan, and its game played in lockstep, every device the same frames from the same start
 *    Version:     1.0.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    As tmnt-port's console/lan.c. A room's devices, up to four, each with its player: HOST makes one, JOIN finds them
 *    on the network, the host's START begins the game. Every device then starts from the same state: the board as at
 *    power-on (the port leaves the program, game_reset, and runs it again from its start: console_lan_reset_wanted,
 *    console_lan_begin), the console as at power-on (console_lan_power_on: every file's own state the game sees), the
 *    host's settings that change the play (DIFFICULTY, LIVES, CONTINUES, DEMO SOUND, SCREEN into the EEPROM's
 *    settings, and the WIDE in effect on the host, which moves the spawns and the players' limits), no high scores of
 *    a device's own (the boot's table on every one), and ARCADE's game started for the host, its character choice on
 *    the game's portraits. Each frame on, every device gives its player's controls (his stick and buttons, and START
 *    newly pressed) and runs the frame with every slot's: the device in the room's list order is the console's player
 *    in that order (the host the first), who comes in with START and picks a character as at home. The state's hash
 *    after each frame goes to the host, which compares them. The game over (the attract mode again), the host gone, a
 *    device out of step or its own Escape: the MULTIPLAYER page again, the device's own settings back. Without the
 *    library (SIMPSONS_LAN not built) the page says NO NETWORK.
 */
#include "console_internal.h"

#ifdef SIMPSONS_LAN
#include "arcade_lan.h"    /* (before game.h: ram.h's names are macros) */
#endif
#include "game.h"
#include "simpsons/machine.h"

#include <stdio.h>
#include <string.h>

#ifdef SIMPSONS_LAN

#define LAN_GAME_VERSION    1          /* the game's datagrams and play: another version's devices are not heard */
#define LAN_BEACON_PORT     27780      /* (OutRunners' 27720, TMNT's 27740, SegaSonic's 27760) */
#define LAN_DELAY           3          /* frames: a player's controls play 3 frames after they are given */
#define LAN_BYTES           2          /* a player's input: his stick and buttons (SIMPSONS_* bits), and START */
#define CONTINUES_ANY       0xFFu      /* (the settings' CONTINUES UNLIMITED) */

/* The host's settings that change the play, as its START sends them */
enum { SET_DIFFICULTY, SET_LIVES, SET_CONTINUES, SET_DEMO_SOUND, SET_SCREEN, SET_WIDE, SETTINGS_BYTES };

static struct
{
	lan_device *device;
	const char *problem;       /* what went wrong last, shown on the MULTIPLAYER page until a line is moved to */
	bool reset_wanted;         /* the game started: the board to be reset for it */
	bool playing;              /* its game, from the reset */
	bool frame_given;          /* a frame's inputs on the board: its state's hash after it */
	bool back_to_page;         /* the game over for this device: the MULTIPLAYER page as the attract mode comes */
	uint32_t frame;
	uint8_t settings[LAN_SETTINGS_MOST];
	size_t settings_size;
	int first_slot;
	console_settings_t own;    /* this device's settings, back after the game */
	char waiting[LAN_NAME_LENGTH + 1];
} lan;

/* The device, opened when first wanted (its name the settings' NAME) */
static lan_device *device_of(void)
{
	static const lan_config config = { { 'S', 'I', 'M', 'P' }, LAN_GAME_VERSION, LAN_BEACON_PORT, LAN_BYTES,
		LAN_DELAY, 0, LAN_LOCKSTEP, 0 };

	if (!lan.device)
	{
		lan.device = lan_open(&config, console_settings.name[0] ? console_settings.name : CONSOLE_DEFAULT_NAME, 1);
		if (!lan.device)
			lan.problem = "NO NETWORK";
	}
	return lan.device;
}

static void take_problem(void)
{
	const char *problem = lan.device ? lan_problem(lan.device) : NULL;

	if (problem)
		lan.problem = problem;
}

bool console_lan_host(void)
{
	lan.problem = NULL;
	return device_of() && lan_host(lan.device);
}

void console_lan_search(void)
{
	lan.problem = NULL;
	if (device_of())
		lan_search(lan.device);
	take_problem();
}

void console_lan_leave(void)
{
	if (lan.device)
		lan_leave(lan.device);
}

/* NAME changed: the device opened again with it, when in no room */
void console_lan_name_changed(void)
{
	if (lan.device && lan_role_of(lan.device) == LAN_OFF)
	{
		lan_close(lan.device);
		lan.device = NULL;
	}
}

bool console_lan_searching(void)
{
	return lan.device && lan_role_of(lan.device) == LAN_SEARCHING;
}

bool console_lan_in_room(void)
{
	return lan.device && (lan_role_of(lan.device) == LAN_IN_ROOM || lan_role_of(lan.device) == LAN_JOINING);
}

bool console_lan_joining(void)
{
	return lan.device && lan_role_of(lan.device) == LAN_JOINING;
}

bool console_lan_hosting(void)
{
	return lan.device && lan_hosting(lan.device);
}

int console_lan_room_count(void)
{
	lan_room rooms[LAN_ROOMS];

	return lan.device ? lan_rooms(lan.device, rooms) : 0;
}

/* A room found, as JOIN lists it: its host's name, its players, PLAYING when its game goes on */
void console_lan_room_text(int index, char *text, size_t size)
{
	lan_room rooms[LAN_ROOMS];
	const int count = lan.device ? lan_rooms(lan.device, rooms) : 0;

	if (index < 0 || index >= count)
	{
		snprintf(text, size, "?");
		return;
	}
	snprintf(text, size, "%-10s %d OF %d%s", rooms[index].name, rooms[index].players, LAN_PLAYERS,
		rooms[index].playing ? " PLAYING" : "");
}

bool console_lan_join(int index)
{
	lan_room rooms[LAN_ROOMS];
	const int count = lan.device ? lan_rooms(lan.device, rooms) : 0;

	if (index < 0 || index >= count || rooms[index].playing || rooms[index].players >= LAN_PLAYERS)
		return false;
	lan.problem = NULL;
	return lan_join(lan.device, &rooms[index]);
}

int console_lan_member_count(void)
{
	lan_member members[LAN_DEVICES];

	return lan.device ? lan_members(lan.device, members) : 0;
}

/* A device of the room, as the room's page lists it: its player's number, its name, HOST, YOU, WAITING when unheard */
void console_lan_member_text(int index, char *text, size_t size)
{
	lan_member members[LAN_DEVICES];
	const int count = lan.device ? lan_members(lan.device, members) : 0;

	if (index < 0 || index >= count)
	{
		snprintf(text, size, "?");
		return;
	}
	snprintf(text, size, "%dP %-10s%s%s%s", members[index].first_slot + 1, members[index].name,
		index == 0 ? " HOST" : "", members[index].own ? " YOU" : "", members[index].heard ? "" : " WAITING");
}

bool console_lan_can_start(void)
{
	return console_lan_hosting() && lan_role_of(lan.device) == LAN_IN_ROOM && console_lan_member_count() > 1;
}

/* The host's START: its settings that change the play, sent to every device */
void console_lan_start(void)
{
	uint8_t settings[SETTINGS_BYTES];

	settings[SET_DIFFICULTY] = (uint8_t)console_settings.game_difficulty;
	settings[SET_LIVES] = (uint8_t)console_settings.game_lives;
	settings[SET_CONTINUES] = console_settings.game_continues == CONSOLE_CONTINUES_UNLIMITED ? CONTINUES_ANY
		: (uint8_t)console_settings.game_continues;
	settings[SET_DEMO_SOUND] = console_settings.demo_sound ? 1u : 0u;
	settings[SET_SCREEN] = console_settings.upside_down ? 1u : 0u;
	settings[SET_WIDE] = (uint8_t)console_wide_in_effect();    /* (AUTO as the host's screen has it) */
	if (console_lan_can_start())
		lan_start(lan.device, settings, sizeof(settings));
}

const char *console_lan_problem(void)
{
	take_problem();
	return lan.problem;
}

void console_lan_problem_seen(void)
{
	lan.problem = NULL;
}

/* Each frame on the MULTIPLAYER pages: the datagrams, and the game's start */
void console_lan_update(void)
{
	if (!lan.device)
		return;
	lan_update(lan.device);
	take_problem();
	if (lan_game_started(lan.device, lan.settings, &lan.settings_size, &lan.first_slot))
		lan.reset_wanted = true;
}

/* ---- the game ---- */

bool console_lan_reset_wanted(void)
{
	return lan.reset_wanted;
}

bool console_lan_playing(void)
{
	return lan.playing;
}

void console_lan_begin(void)
{
	console_settings_t *settings = &console_settings;

	lan.reset_wanted = false;
	lan.playing = true;
	lan.frame_given = false;
	lan.back_to_page = false;
	lan.own = *settings;
	if (lan.settings_size >= SETTINGS_BYTES)
	{
		settings->game_difficulty = lan.settings[SET_DIFFICULTY] < CONSOLE_DIFFICULTIES ? lan.settings[SET_DIFFICULTY] : 1;
		settings->game_lives = lan.settings[SET_LIVES] >= CONSOLE_LIVES_FEWEST
			&& lan.settings[SET_LIVES] <= CONSOLE_LIVES_MOST ? lan.settings[SET_LIVES] : 2;
		settings->game_continues = lan.settings[SET_CONTINUES] <= CONSOLE_CONTINUES_MOST ? lan.settings[SET_CONTINUES]
			: CONSOLE_CONTINUES_UNLIMITED;
		settings->demo_sound = lan.settings[SET_DEMO_SOUND] != 0u;
		settings->upside_down = lan.settings[SET_SCREEN] != 0u;
		settings->wide = lan.settings[SET_WIDE] < WIDE_AUTO ? lan.settings[SET_WIDE] : WIDE_OFF;
	}
	settings->changed = true;    /* (the window as WIDE has it: the port applies them, and saves none in the game) */
	console_lan_power_on();
	fprintf(stderr, "MULTIPLAYER: the game starts, player %d\n", lan.first_slot + 1);
}

/* The game ended for this device: its own settings back, the MULTIPLAYER page as the attract mode comes */
static void lan_game_end(const char *problem)
{
	console_settings_t *settings = &console_settings;

	if (lan.device)
		lan_leave(lan.device);
	lan.playing = false;
	lan.frame_given = false;
	lan.back_to_page = true;
	settings->game_difficulty = lan.own.game_difficulty;
	settings->game_lives = lan.own.game_lives;
	settings->game_continues = lan.own.game_continues;
	settings->demo_sound = lan.own.demo_sound;
	settings->upside_down = lan.own.upside_down;
	settings->wide = lan.own.wide;
	console_settings_into_game();    /* (into the EEPROM and the game's copies, which held the host's: the attract mode's
	                                    DEMO SOUND until the next game took OPTIONS' again) */
	settings->changed = true;
	console_scores_lan(false);
	if (problem)
		lan.problem = problem;
	fprintf(stderr, "MULTIPLAYER: the game is over%s%s\n", problem ? ": " : "", problem ? problem : "");
}

void console_lan_game_over(void)
{
	lan_game_end(NULL);
}

void console_lan_quit(void)
{
	console_quit_game();
	lan_game_end(NULL);
}

bool console_lan_back_to_page(void)
{
	const bool back = lan.back_to_page;

	lan.back_to_page = false;
	return back;
}

bool console_lan_frame(uint8_t pressed, bool start)
{
	uint8_t own[LAN_BYTES], all[LAN_PLAYERS * LAN_BYTES], players[CONSOLE_PLAYERS], starts = 0;
	const char *problem;

	if (!lan.playing)
		return true;
	if (lan.frame_given)
	{
		lan_frame_hash(lan.device, lan.frame, simpsons_state_hash());
		lan.frame_given = false;
	}
	lan_update(lan.device);
	problem = lan_problem(lan.device);
	if (lan_role_of(lan.device) != LAN_PLAYING || (problem && strcmp(problem, "OUT OF STEP") == 0))
	{
		/* (the host gone, or a device out of step: the game ended where it is, for this device) */
		console_quit_game();
		lan_game_end(problem ? problem : "HOST GONE");
		return true;
	}
	own[0] = pressed;
	own[1] = start ? 1u : 0u;
	lan_give_inputs(lan.device, own);
	if (!lan_next_frame(lan.device, all))
	{
		if (!lan_waiting(lan.device, lan.waiting))
			lan.waiting[0] = '\0';
		return false;
	}
	lan.waiting[0] = '\0';
	lan.frame = lan_frame_number(lan.device) - 1u;
	lan.frame_given = true;
	for (int slot = 0; slot < CONSOLE_PLAYERS; slot++)
	{
		players[slot] = all[slot * LAN_BYTES];
		if (all[slot * LAN_BYTES + 1])
			starts |= (uint8_t)(1u << slot);
	}
	console_controls(players, starts);
	return true;
}

uint32_t console_lan_frame_number(void)
{
	return lan.frame;
}

const char *console_lan_waiting(void)
{
	return lan.playing && lan.waiting[0] ? lan.waiting : NULL;
}

#else

/* (no LAN in this build: the page says so) */
bool console_lan_host(void) { return false; }
void console_lan_search(void) { }
void console_lan_leave(void) { }
void console_lan_name_changed(void) { }
bool console_lan_searching(void) { return false; }
bool console_lan_in_room(void) { return false; }
bool console_lan_joining(void) { return false; }
bool console_lan_hosting(void) { return false; }
int console_lan_room_count(void) { return 0; }
void console_lan_room_text(int index, char *text, size_t size) { (void)index; snprintf(text, size, "?"); }
bool console_lan_join(int index) { (void)index; return false; }
int console_lan_member_count(void) { return 0; }
void console_lan_member_text(int index, char *text, size_t size) { (void)index; snprintf(text, size, "?"); }
bool console_lan_can_start(void) { return false; }
void console_lan_start(void) { }
const char *console_lan_problem(void) { return "NO NETWORK"; }
void console_lan_problem_seen(void) { }
void console_lan_update(void) { }
bool console_lan_reset_wanted(void) { return false; }
bool console_lan_playing(void) { return false; }
void console_lan_begin(void) { }
void console_lan_game_over(void) { }
void console_lan_quit(void) { }
bool console_lan_back_to_page(void) { return false; }
bool console_lan_frame(uint8_t pressed, bool start) { (void)pressed; (void)start; return true; }
uint32_t console_lan_frame_number(void) { return 0; }
const char *console_lan_waiting(void) { return NULL; }

#endif
