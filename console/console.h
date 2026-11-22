/**
 *    Name:        console.h
 *    Description: what the console version (The Simpsons) tells the ports, and what they hand it
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    The console version is the arcade game's program with the functions console/replaced.txt lists in the
 *    console's own versions (console.c): a front end of its own in the attract mode (the Konami presentation, the
 *    menus on the TV: ARCADE, MR. PLOW MODE, OPTIONS, EXIT), each player's character chosen on the game's portraits
 *    and again after a continue, no coins (a player joins with START: the console puts the slot's coin and attack in,
 *    and the boxes say PRESS START), the cabinet's settings from OPTIONS written into the EEPROM, the pause's QUIT
 *    GAME and RESTART, and the Konami code's page on the menu (a stage to start in, infinite lives and health, a
 *    character for more than one player). The ports hand it each frame's controls and actions, and draw its overlay
 *    on each frame. As tmnt-port's console/console.h.
 */
#ifndef SIMPSONS_CONSOLE_H
#define SIMPSONS_CONSOLE_H

#include "simpsons/state.h"
#include "simpsons/video.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONSOLE_PLAYERS 4

/* Sets the console up, after game_reset and before the program runs: OPTIONS' cabinet settings into the board's
   EEPROM (console_settings: the boot reads them), and the overlay's letters (Kenney Mini, font.bmp in 'folder', with
   its separator; NULL: the current folder; the menus write with the game's own). false when font.bmp is not there */
bool console_attach(const char *folder);

/* The menus' actions, from any player's keys and game controller: the port hands those newly pressed at each
   frame, and the menus take them on the game's next frame */
enum
{
	ACTION_UP,
	ACTION_DOWN,
	ACTION_LEFT,
	ACTION_RIGHT,
	ACTION_ACCEPT,      /* the attack button, Enter */
	ACTION_BACK,        /* the jump button, Escape */
	ACTION_START,
	ACTIONS
};
#define ACTION_BIT(action) (1u << (action))

void console_actions(uint32_t pressed);

/* Each frame, after the port has read its controls: each player's stick and buttons (simpsons/machine.h's
   SIMPSONS_LEFT ... SIMPSONS_JUMP bits, set when pressed) and START (bit n: player n's, newly pressed). The console
   puts them on the board: a player's on his character's slot, a START in a game on a slot of its own, with the
   slot's coin and attack (the board's inputs for the next frame). Out of a game, what a player newly presses is also
   a menu action (the stick's directions, attack ACCEPT, jump BACK) */
void console_controls(const uint8_t pressed[CONSOLE_PLAYERS], uint8_t starts);

/* The picture the console draws over: the frame the video drew, with columns more at its sides ('left' and 'right':
   WIDE's, 0 without it), its rows 'stride' pixels apart, the board's column 0 of each at 'pixels' + 'left'. The
   console's x are the board's, from -left to 319 + right */
typedef struct
{
	uint32_t *pixels;
	int stride;
	int left, right;
} console_picture;

/* The console's own picture over the frame the video drew (the menus' letters, black over the boot): the port calls
   it on each frame it shows or writes */
void console_overlay(console_picture *picture);

/* True once the menu's EXIT was picked: the port closes the game */
bool console_quit_requested(void);

/* The volume the port plays the game's sound at: OPTIONS' VOLUME (0-10, 5 the game's own), 0 while the console keeps
   the game silent (the Konami code's game, under black until its stage starts) */
int console_volume(void);

/* Whether the port runs the game's frames as fast as it can, without waiting for the frame's time and showing one in
   a few: the black the console keeps over a game begun at a stage until the stage starts (some 700 of the board's
   frames: the story skipped, stage 1's title card and opening) */
bool console_hurried(void);

/* The pause (as tmnt-port's): true while a game the menus started is played, when the port may stop it (Escape, a
   game controller's Back, or START of a player whose character plays: console_start_pauses; another's START brings
   him in); its picture over the frame it stopped at, the line 'choice' chosen; SAVE STATE and LOAD STATE the game's
   whole state into its file and back (the port's, game/state.h: the pause stays, LOAD STATE's picture the state's);
   QUIT GAME ends the game at its next frame, back to the menu; RESTART ends it and starts the same game again, each
   player on the same character */
enum
{
	CONSOLE_PAUSE_CONTINUE,
	CONSOLE_PAUSE_SAVE_STATE,
	CONSOLE_PAUSE_LOAD_STATE,
	CONSOLE_PAUSE_RESTART,
	CONSOLE_PAUSE_QUIT,
	CONSOLE_PAUSE_ITEMS
};

bool console_can_pause(void);
bool console_start_pauses(int player);

/* MULTIPLAYER, the LAN game (console/lan.c, arcade-lan): the host's START has begun it, and the board is to be reset
   for it (the port: the program left, game_reset, console_lan_begin, and the program run again from its start, on
   every device alike); while it is played each frame's controls go through console_lan_frame (this device's player's
   stick and buttons, and START newly pressed), which puts every device's on the board once they have come (false:
   not yet, the frame holds and the port shows its last picture again), and there is no pause (Escape or Back leaves
   the game: console_lan_quit). The LAN frame whose controls are on the board, for checks (--lan-hashes) */
bool console_lan_reset_wanted(void);
void console_lan_begin(void);
bool console_lan_playing(void);
bool console_lan_frame(uint8_t pressed, bool start);
void console_lan_quit(void);
uint32_t console_lan_frame_number(void);
void console_pause_overlay(console_picture *picture, int choice);
void console_quit_game(void);
void console_restart_game(void);

/* ---- what the console keeps besides its settings ---- */

/* The high scores kept (scores.c): the cabinet keeps its table in the work RAM alone, lost at power-off; the console
   keeps it in 'path' (simpsons.scores in the folder of what the game keeps; NULL or "": none, a scripted run's), puts
   it into the game's table once the boot has set its own, and writes it when a new score has come in (the name
   entry's end). A file that is no such table, or none, leaves the boot's. console_scores_frame at each frame's end */
void console_scores_open(const char *path);
void console_scores_frame(void);

/* A save state's console part (states.c): the console's game as it goes (its pages and players, its slots and
   characters, the joins under way, its mode's, WIDE's account of the layers): the port visits it in its own part
   (game/state.h's 'more') */
void console_state_part(simpsons_state *state);

/* The pause's word on what SAVE STATE or LOAD STATE came to (NULL: none), under its lines */
const char *console_state_message(simpsons_state_result result, bool loading);
void console_pause_message(console_picture *picture, const char *message);

/* The players' controls as they are held now, taken as the last ones (after the pause: what was held over it is
   no new press) */
void console_controls_held(const uint8_t pressed[CONSOLE_PLAYERS]);

/* The settings OPTIONS shows and changes: the port loads them (simpsons.ini) before console_attach, and applies and
   saves them when 'changed' is set. The enums only grow at their ends (simpsons.ini keeps them by name) */
enum
{
	PIXELS_SHARP,       /* the board's pixels, square */
	PIXELS_SMOOTH,      /* sharp bilinear: square pixels with soft edges */
	PIXELS_BILINEAR,    /* plain bilinear, softer */
	PIXELS_CRT,         /* scanlines, a phosphor mask and a little vignette over smooth pixels */
	PIXELS_KINDS
};

/* OPTIONS > VIDEO > WIDE: the picture wider than the board's 320 pixels (wide.c), in VIDEO's order AUTO, OFF, 16:10,
   16:9, 20:9; simpsons.ini keeps it by name, and the enum only grows at its end (as tmnt-port's) */
enum
{
	WIDE_OFF,           /* the board's 320 pixels */
	WIDE_16_9,          /* 48 more at each side (the window shows 398) */
	WIDE_20_9,          /* 88 more at each side (the window shows 496): a phone's screen */
	WIDE_16_10,         /* 32 more at each side (the window shows 358): the Steam Deck's screen */
	WIDE_AUTO,          /* the one the screen's shape asks for (console_set_screen_wide): the default (Sebas: "Sí, AUTO
	                       por defecto") */
	WIDE_KINDS
};

/* WIDE AUTO: the port says which of OFF, 16:10, 16:9 and 20:9 the screen's shape asks for (its follow_screen: a
   computer's monitor, a full screen's size, a phone's or a Fold's screen as it changes); the one in effect is it while
   OPTIONS > VIDEO > WIDE is AUTO (OFF for the arcade's own scenarios, --arcade-start), else the one chosen */
void console_set_screen_wide(int kind);
int console_wide_in_effect(void);

/* OPTIONS > CONTROLS: each player's controls, which the port binds to keys and game controller buttons (as
   tmnt-port's) */
enum
{
	CONTROL_UP,
	CONTROL_DOWN,
	CONTROL_LEFT,
	CONTROL_RIGHT,
	CONTROL_ATTACK,
	CONTROL_JUMP,
	CONTROL_START,
	CONTROLS
};
#define CONTROL_PLAYERS 4

/* CONTROLS on screen: where, in the picture's pixels, the pictures of 'control''s keys and button go (their top
   left; false when the page is not shown): the first key's there, the second's and the game controller button's
   that many pixels to the right */
bool console_controls_place(int control, int *x, int *y);
#define CONSOLE_CONTROLS_SECOND_X   36     /* (room for a key two tiles wide, Enter or Space) */
#define CONSOLE_CONTROLS_BUTTON_X   76
#define CONSOLE_CONTROLS_NEXT_X     18     /* (a device's page, SIMPSONS_DEVICE_CONTROLS: no keys, the control's
                                              buttons side by side from the first key's place) */

/* The player whose controls CONTROLS shows (0-3) */
int console_controls_player(void);

/* The control CONTROLS waits for a new key or button for, or -1: the port takes the next one pressed, binds it
   (Escape: leaves it as it was) and says so */
int console_controls_waiting(void);
void console_controls_bound(void);

/* DEFAULTS was picked on CONTROLS (true once): the port puts the shown player's keys and buttons back */
bool console_controls_defaults(void);

/* The cabinet's settings OPTIONS changes, by the test mode's COIN/GAME OPTIONS page (the coinage stays the
   factory's: a START is a coin and a credit) */
#define CONSOLE_DIFFICULTIES    4      /* 8-3 DIFFICULTY: EASY, NORMAL, DIFFICULT, VERY DIFFICULT */
#define CONSOLE_LIVES_FEWEST    1      /* 8-2 NUMBER OF PLAYERS: the lives a credit gives, 1 to 7 */
#define CONSOLE_LIVES_MOST      7
#define CONSOLE_CONTINUES_MOST  9      /* OPTIONS' CONTINUES, the console's: ARCADE's continues after a player's first
                                          credit, 0 to 9, */
#define CONSOLE_CONTINUES_UNLIMITED (-1)    /* or as many as STARTs (the cabinet's coins) */
#define CONSOLE_CONTINUES_DEFAULT 3    /* (the modes' and TMNT's) */
#define CONSOLE_NAME_LENGTH     10     /* MULTIPLAYER > NAME: the device's name in a room (arcade-lan's LAN_NAME_LENGTH) */
#define CONSOLE_DEFAULT_NAME    "PLAYER"

typedef struct
{
	int window_scale;      /* 1-6: the window, in times the game's picture */
	bool full_screen;
	int pixels;            /* PIXELS_* */
	bool integer_scale;    /* OPTIONS > VIDEO > INTEGER SCALE: the picture a whole number of times its pixels */
	int wide;              /* OPTIONS > VIDEO > WIDE: WIDE_* */
	int volume;            /* 0-10: 5 is the game's own */
	bool fps;              /* OPTIONS > VIDEO > FPS: the frames a second at the picture's top right, and in the title */
	bool vsync;            /* OPTIONS > VIDEO > VSYNC: each frame shown at the screen's refresh (a computer's) */
	bool stereo;           /* OPTIONS > SOUND: STEREO, the YM2151's two outputs as the program pans them; MONO, the
	                          board's (its first alone) */
	int game_difficulty;   /* 0-3, as the EEPROM keeps it (68 + 4) */
	int game_lives;        /* 1-7: the EEPROM keeps it less one (68 + 5) */
	int game_continues;    /* ARCADE's continues: 0 to CONSOLE_CONTINUES_MOST, or CONSOLE_CONTINUES_UNLIMITED (the
	                          console's, not the EEPROM's) */
	bool demo_sound;       /* the sound in the attract mode (68 + 6) */
	bool upside_down;      /* the screen flipped (68 + 7) */
	bool bart_shirt;       /* Bart's shirt in the show's orange (bart_shirt.c); false: the cabinet's light blue */
	bool konami_code;      /* the Konami code entered once on the menu: its KONAMI CODE line from then on (menu.c) */
	char name[CONSOLE_NAME_LENGTH + 1];    /* MULTIPLAYER > NAME (A-Z, 0-9, '.', '-' and spaces; none yet: asked for) */
	bool changed;
} console_settings_t;

extern console_settings_t console_settings;

/* The names the settings file gives the pixels' kinds, WIDE's and the difficulties */
extern const char *const console_pixel_names[PIXELS_KINDS];
extern const char *const console_wide_names[WIDE_KINDS];
extern const char *const console_difficulty_names[CONSOLE_DIFFICULTIES];

/* ---- WIDE (wide.c) ---- */

/* The port calls it at each frame's end, before the next is drawn: WIDE's sides, the layers' maps' ends and the
   players' boxes' band onto the video (simpsons/video.h), and the layers' columns the wider screen shows that the
   game does not draw that far, from the same maps */
void console_wide_frame(void);
void wide_screen_sides(void);

/* The window's picture, in the game's pixels: 320, or WIDE's (358 for 16:10, 398 for 16:9, 496 for 20:9: the middle of
   the frames, which have 32, 48 or 88 columns more at each side) */
int console_wide_view(void);

/* For checks (--wide-check): WIDE's sides' columns whose tiles are not their layer's map's, described in 'text';
   their count, 0 when none */
int console_wide_check(char *text, size_t size);

/* For checks (--wide-check): what the spawn list put in since the last look that the wider screen showed where it was
   put and the cabinet did not, described in 'text'; their count, 0 when none */
int console_wide_spawn_check(char *text, size_t size);

/* For checks (--arcade-start): the arcade's own attract mode and controls (a script's coins and players' buttons
   straight onto the board, player N's on slot N), so the arcade's scenarios run as they are in the console's build,
   its WIDE in their games */
void console_arcade_test(void);

/* For scripted checks: where the console stands, a line of text (the page, the slots, the settings the game took) */
void console_describe(char *text, size_t size);

#ifdef __cplusplus
}
#endif

#endif
