/**
 *    Name:        console_internal.h
 *    Description: what the console version's files share among themselves
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 */
#ifndef SIMPSONS_CONSOLE_INTERNAL_H
#define SIMPSONS_CONSOLE_INTERNAL_H

#include "console.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* The game's modes (game_mode, 4800: the IRQ's table E42B) */
#define GAME_MODE_TEST          0u
#define GAME_MODE_ATTRACT       1u
#define GAME_MODE_GAME          2u

/* The characters, each a slot of the cabinet's (0-3: 1 Marge, 2 Homer, 3 Bart, 4 Lisa) and the kind of its object */
#define CHARACTERS              4
#define CHARACTER_MARGE         0
#define CHARACTER_HOMER         1
#define CHARACTER_BART          2
#define CHARACTER_LISA          3

/* The console's pages */
enum
{
	PAGE_PRESENTATION,     /* the Konami presentation at power-on (presentation.c) */
	PAGE_MENU,             /* ARCADE, MR. PLOW MODE, OPTIONS, EXIT, on the TV (tv.c) */
	PAGE_MR_PLOW,          /* MR. PLOW MODE: the console's own modes (modes.c) */
	PAGE_OPTIONS,          /* CONTROLS, the cabinet's settings, VIDEO, VOLUME, the version */
	PAGE_VIDEO,            /* OPTIONS > VIDEO: the window's size, full screen, the pixels, VSYNC, FPS */
	PAGE_CONTROLS,         /* OPTIONS > CONTROLS: each player's keys and game controller buttons */
	PAGE_STARTING,         /* ARCADE's coin going in, the game starting */
	PAGE_CHOICE,           /* the game's portraits: each player chooses his character (characters.c) */
	PAGE_GAME,             /* the game's own, from the characters chosen to the attract mode */
	PAGE_LAN,              /* MULTIPLAYER: HOST, JOIN, NAME, BACK (lan.c), */
	PAGE_LAN_NAME,         /* its NAME, */
	PAGE_LAN_ROOMS,        /* JOIN's rooms found, */
	PAGE_LAN_ROOM,         /* and the room's devices */
	PAGE_KONAMI,           /* the Konami code's page, from the menu: its stage, lives, health and characters
	                          (konami.c) */
	PAGE_INTRO,            /* INTRO: STORY, ATTRACT, BACK */
	PAGE_ATTRACT           /* INTRO > ATTRACT: the cabinet's own attract mode, no menu over it (console.c) */
};

/* The lives a credit gives and the continues after the first credit in a game of the console's own modes (TMNT's
   Splinter Way's: guideline 10): ARCADE keeps the cabinet's lives (OPTIONS' LIVES) and OPTIONS' CONTINUES; FREE PLAY
   the same lives and as many continues as STARTs */
#define MODE_LIVES              5
#define MODE_CONTINUES          3
#define CONTINUES_UNLIMITED     (-1)
#define LIVES_CABINET           0      /* (a mode's lives: OPTIONS' LIVES) */

/* The Konami code's page's stages: the steps (game_step, 4803) that are a stage or a bonus game */
#define KONAMI_STAGES           10
#define STEP_FIRST_STAGE        2      /* game_step: stage 1, after the story the game begins with */

typedef struct
{
	uint32_t pressed;          /* the actions pressed since the game's last frame */
	uint32_t player_pressed[CONSOLE_PLAYERS];    /* each player's actions since the game's last frame, out of a
	                                                game: his stick, buttons and START (the port's fixed keys go
	                                                with player 1's) */
	int page;
	int choice;                /* the line chosen on the page */
	int page_frames;           /* the game's frames on the page */
	int last_presser;          /* the last player who pressed a button or START (the one who picks) */
	bool booted;               /* the attract mode has run once: the boot is over (black over it till then) */
	bool quit;                 /* EXIT picked */
	bool quitting_game;        /* QUIT GAME or RESTART: the game ends at its next frame (game_mode_frame) */
	bool restarting;           /* RESTART: the same game again, each player on the character he played */
	int chooser;               /* the player who started the game: in the character choice from its start */
	int choice_character[CONSOLE_PLAYERS];    /* the character choice: each player's (-1: not in) */
	bool choice_picked[CONSOLE_PLAYERS];      /* and whether he picked it (its slot's coin went in) */
	int character_change[CONSOLE_PLAYERS];   /* in a game: the character a player chooses to come in on, plus one
	                                            (0: no choice open) */
	int mode_lives;            /* the game's lives a credit (LIVES_CABINET: OPTIONS'), */
	int mode_continues;        /* and its continues after a player's first credit (CONTINUES_UNLIMITED: any) */
	int credits_put[CONSOLE_PLAYERS];    /* each player's credits in the game (his first and his continues) */
	bool konami;               /* the game being started or played is the Konami code page's: */
	int konami_stage;          /* its stage (0 to KONAMI_STAGES - 1), */
	bool infinite_lives;       /* its lives kept up, */
	bool infinite_health;      /* its players' health kept full, */
	bool repeats;              /* and its characters chosen by more than one player (same_character.c) */
	int start_step;            /* the step the game being started or played began in (game_step: STEP_FIRST_STAGE,
	                              the game's own beginning, or a mode's: konami.c), */
	bool start_pending;        /* and whether it is not yet set in the game started */
	int mode;                  /* the game being started or played is MR. PLOW MODE's (MODE_*: modes.c) */
	bool code_entered;         /* the Konami code was entered on the menu: its page's lives, health and characters
	                              hold in MR. PLOW MODE's games too */
	bool lan_start_due;        /* a LAN game's board reset: ARCADE's game started for the host as the attract mode
	                              comes (console_lan_power_on) */
} console_state;

/* MR. PLOW MODE's modes (modes.c) */
enum
{
	MODE_NONE,                 /* ARCADE's game, the Konami code page's */
	MODE_FREE_PLAY,            /* FREE PLAY: the game as ARCADE's, OPTIONS' lives, as many continues as STARTs */
	MODE_BOSS_RUSH,            /* the bosses one after another */
	MODE_BONUS_GAMES,          /* the two bonus stages in a row */
	MODE_FRANK_GRIMES,         /* FRANK GRIMES LIFE: one player against what four would have */
	MODE_INTRO                 /* the menu's INTRO: the story before stage 1, then the menu */
};

extern console_state console;

/* Each player's slot (0-3, the cabinet's: 1 Marge, 2 Homer, 3 Bart, 4 Lisa), -1 for none */
extern int console_slot_of[CONSOLE_PLAYERS];

/* A player starts on 'slot': its coin when 'coin' says (each slot has credits of its own: the factory's EEPROM), then,
   when 'attack' says, its attack until the slot plays; false when a join of the slot goes on already. Whether one does */
bool console_join(int slot, bool coin, bool attack);
bool console_join_going_on(int slot);

/* Whether 'slot''s character plays, or is coming in (a join under way) */
bool console_slot_plays(int slot);
bool console_slot_joining(int slot);

/* The game ended where it is (QUIT GAME, RESTART, the character choice left): the attract mode at its next frame */
void console_end_game(void);

/* ---- MULTIPLAYER (lan.c) ---- */

/* The menus' side of the LAN game: HOST, JOIN (the rooms found, one joined), the room's devices, the host's START,
   LEAVE; what went wrong last (NULL: nothing); console_lan_update each frame on its pages */
bool console_lan_host(void);
void console_lan_search(void);
void console_lan_leave(void);
void console_lan_name_changed(void);
bool console_lan_searching(void);
bool console_lan_in_room(void);
bool console_lan_joining(void);
bool console_lan_hosting(void);
int console_lan_room_count(void);
void console_lan_room_text(int index, char *text, size_t size);
bool console_lan_join(int index);
int console_lan_member_count(void);
void console_lan_member_text(int index, char *text, size_t size);
bool console_lan_can_start(void);
void console_lan_start(void);
const char *console_lan_problem(void);
void console_lan_problem_seen(void);
void console_lan_update(void);
/* The LAN game's end for this device (its game over, as the attract mode comes back); whether the MULTIPLAYER page is
   to open as the attract mode comes back (once); the device the game holds for, by its name (NULL: none) */
void console_lan_game_over(void);
bool console_lan_back_to_page(void);
const char *console_lan_waiting(void);

/* The console as at power-on for a LAN game, after the board's reset (console.c): every file's state the game sees
   (the console's, the joins, the characters, WIDE's rings, the scenes' skips), the settings into the EEPROM, the high
   scores of the boot's (console_scores_lan), ARCADE's game started for the host as the attract mode comes */
void console_lan_power_on(void);
void characters_power_on(void);
void wide_power_on(void);
/* A LAN game's: the boot's table on every device, nothing of it kept; once over, the kept table back in the game */
void console_scores_lan(bool playing);

/* Whether the game shows a scene a button skips (check_skip's: a story, a title card, a stage's or a bonus game's
   end, the ending): no pause there, a START skips it */
bool console_in_scene(void);
/* Whether a story is being skipped whole, a player's button having skipped a part of it: black and silence over it,
   its frames hurried, till its step ends */
bool console_scene_skipping(void);
/* Whether its black and silence are kept: while it is skipped and the few frames after, its last picture's and its
   tune's */
bool console_scene_quiet(void);

/* Whether the console runs the arcade's own attract mode and controls (--arcade-start, console_arcade_test): the
   game's functions in the console's versions then do what the arcade's do, but WIDE's */
bool console_arcade_testing(void);

/* OPTIONS' cabinet settings into the EEPROM (the boot's) and, once the game has booted, into the work RAM's copy of
   the EEPROM's settings and the game's variables (apply_settings) */
void console_settings_into_eeprom(void);
void console_settings_into_game(void);

/* A game of the console's started by 'player' (the console's own modes, guideline 10: MR. PLOW MODE's, the Konami
   code's): the character choice on the game's portraits, then the game as the cabinet plays it with 'lives' a credit
   and 'continues' after each player's first credit (LIVES_CABINET, CONTINUES_UNLIMITED: ARCADE's), begun at 'step'
   (game_step: a stage or bonus game, 2 to 15; STEP_FIRST_STAGE its own beginning, the story before stage 1): the story
   skipped, and once stage 1 is played the step set as the program does at a step's end, black and silent meanwhile
   (konami.c). From there the game goes on step by step as the cabinet's, to its ending; it ends at the game over of
   every player (the continues over: the attract mode, the menu) or at the pause's QUIT GAME, and RESTART starts it
   again the same. What a mode does besides (its own rules each frame) goes in game_mode_frame's (console.c) */
void console_start_game(int player, int step, int lives, int continues);
/* ARCADE: the cabinet's game from its beginning, OPTIONS' lives and continues */
void console_start_arcade(int player);

/* The game's lives and continues (guideline 10): a mode sets its own as its game starts (MODE_LIVES, MODE_CONTINUES;
   LIVES_CABINET and CONTINUES_UNLIMITED: FREE PLAY's), and the lives a credit gives go into the game (lives_per_credit)
   until the game is over */
void console_set_mode_lives(int lives, int continues);
/* Whether 'player' may put a credit in: his first, or a continue while the mode has some left for him; and his credit
   counted */
bool console_credit_allowed(int player);
void console_credit_put(int player);

/* ---- the Konami code (its page: menu.c; its game: konami.c) ---- */

/* A stage's name on the page (its title card's), and its step */
const char *konami_stage_name(int stage);
int konami_stage_step(int stage);

/* On the menu: whether this frame's presses complete the code (up, up, down, down, left, right, left, right, B, A:
   jump, attack), from any player's controls or the keyboard */
bool konami_code_entered(uint32_t pressed);
/* Its page opened with the coin's sound, from the menu's line 'menu_line' (BACK returns there); its frame and picture */
void konami_page_open(int menu_line);
void konami_page_frame(uint32_t pressed);
void konami_page_overlay(console_picture *frame);
/* The Konami game's frame before the game's (its stage set once stage 1 is played, its lives and health kept up); its
   presses on the board (the story before stage 1 skipped by a player's attack: console_controls); black over the game
   until its stage starts */
void konami_game_frame(void);
uint8_t konami_presses(int slot);
void konami_overlay(console_picture *frame);
/* Whether the game started is not yet at its first step (the picture under black, the sound silent) */
bool konami_covering(void);

/* ---- MR. PLOW MODE's modes (modes.c) ---- */

/* Each starts its game for 'player' (the character choice first, as ARCADE's): FREE PLAY and FRANK GRIMES LIFE from
   the game's beginning, BOSS RUSH from stage 1, BONUS GAMES at bonus 1 */
void free_play_start(int player);
void boss_rush_start(int player);
void bonus_games_start(int player);
void frank_grimes_start(int player);
/* The menu's INTRO (or the menu left alone a while), started for 'player': the story before stage 1 alone, the choice
   made at once under black, then the menu again */
void intro_start(int player);
/* Whether the character choice is made at once, with no one choosing (INTRO's) */
bool modes_choice_made(void);
/* INTRO's story over (menu.c): INTRO's page at STORY when it was picked there, else the menu at its INTRO line (the
   menu left alone) */
void menu_after_intro(void);
/* INTRO > ATTRACT (console.c): the cabinet's attract mode from its opening, as after a game, with no menu over it,
   till a button brings INTRO's page back at ATTRACT */
void console_attract_watch(void);
/* The per-game state as a mode's game starts (console_start_game; RESTART's too) */
void modes_game_starting(void);
/* Whether the Konami code's page's rules (its lives, health and characters) hold in the game: a mode's, the code
   entered */
bool modes_code_rules(void);
/* Whether 'player' may come in (FRANK GRIMES LIFE: the one who started it alone) */
bool console_player_may_join(int player);
/* The mode's frame before the game's (game_mode_frame's): true when the game's frame is not to run (its end) */
bool modes_game_frame(void);
/* BOSS RUSH: whether a stage is crossed to its boss out of sight (the players' controls the crossing's, black, no
   sound, the frames as fast as they go), and the crossing's controls of 'slot' */
bool boss_rush_crossing(void);
bool modes_covering(void);
uint8_t modes_presses(int slot);
/* The crossing's card over the black */
void modes_overlay(console_picture *frame);
/* For scripted checks: the mode, BOSS RUSH's boss, the game's count of players, rank and difficulty */
const char *modes_describe(void);

/* ---- characters chosen more than once (same_character.c) ---- */

/* The character each slot plays (the slot's own, unless the Konami code's game lets two players choose the same one:
   then the second plays on another slot, its object the chosen one's moves and looks) */
extern int console_slot_character[CONSOLE_PLAYERS];
/* Whether characters may be chosen more than once (the Konami game with REPEAT CHARACTERS) */
bool console_repeats_allowed(void);
/* The slot 'player' plays 'character' on: the character's own while no other player has it, else the first slot no
   other player has; -1 for none. The slot's character set to it */
int console_slot_for_character(int player, int character);
/* The slots back to their own characters (a game's end) */
void console_characters_reset(void);
/* The choice on the portraits: a slot another character plays on keeps its portrait from coming alive (the
   character's own box is the one picked) */
void same_character_portraits(void);
/* Each frame: the join marks naming the characters the slots play in a game (the game's own out of one) */
void same_character_marks(void);

/* ---- the presentation (presentation.c) ---- */

/* A frame of it, with the menus' actions (START or attack skips it); false once it is over */
bool presentation_frame(uint32_t pressed);
void presentation_overlay(console_picture *frame);

/* ---- the menu's TV (tv.c) ---- */

/* The TV's screen, where the menu goes: its middle, and the rows a line of letters fits in across it */
#define TV_MIDDLE_X             160
#define TV_SCREEN_TOP           24
#define TV_SCREEN_BOTTOM        176

/* A frame of the menu's screen: the TV set up (once, over a cleared screen) and held there, the menus' song */
void tv_frame(void);
/* Whether it shows (set up and drawn) */
bool tv_shown(void);
/* The TV set up again the next time it is asked for (a game cleared the screen) */
void tv_forget(void);

/* ---- the character choice (characters.c) ---- */

/* The choice opened as the game starts: the one who started it in it, the others coming in with START (RESTART:
   each player on the character he played, at once) */
void choice_open(void);
/* Its frame in the game mode (game_mode_frame's), with each player's actions: it holds the portraits meanwhile */
void choice_frame(const uint32_t player_pressed[CONSOLE_PLAYERS]);
void choice_overlay(console_picture *frame);

/* In a game: a START of a player whose character does not play opens his character change (his character chosen
   again, as after a continue, on a line over the game); each frame his controls newly pressed and his START move it */
void character_change_open(int player);
void character_change_frame(int player, uint8_t fresh, bool start);
void character_change_overlay(console_picture *frame);
/* Whether 'player' may come in on 'slot': it does not play nor is coming in, and no other player chooses it */
bool console_slot_free_for(int player, int slot);

/* The characters' names, as the portraits' banners write them */
extern const char *const console_character_names[CHARACTERS];

/* Each player's tag and colour (the choice's frames, the change's line) */
extern const char *const console_player_tags[CONSOLE_PLAYERS];
extern const uint32_t console_player_colours[CONSOLE_PLAYERS];

/* The menus (menu.c): their frame in the attract mode, with the actions pressed (any player's) and each player's
   own; their picture */
void menu_frame(uint32_t pressed, const uint32_t player_pressed[CONSOLE_PLAYERS]);
void menu_overlay(console_picture *frame);

/* ---- WIDE (wide.c) ---- */

/* Over the frame the video drew: a picture's sides (a screen of the board's width: the menus' TV, the stage cards, the
   stories...): its backdrop's colour where both edges are the same one colour, else black */
void wide_picture_sides(console_picture *frame);
/* The picture's columns the window leaves out at each side (it shows its middle, console_wide_view) */
int wide_crop(const console_picture *frame);

/* Bart's shirt (bart_shirt.c): the video's recolouring by console_settings.bart_shirt, at once */
void console_bart_shirt_apply(void);

/* ---- drawing over the frame (draw.c) ---- */

/* The picture's first column and the one past its last, in the board's x; the middle, where centred lines go */
#define PICTURE_FIRST(picture)  (-(picture)->left)
#define PICTURE_END(picture)    (SIMPSONS_VIDEO_WIDTH + (picture)->right)
int draw_centred_x(const console_picture *frame, int width);

/* Kenney Mini (font.bmp): loaded from 'folder'; a line of it at x, y (its top), 'scale' times, in 0x00RRGGBB with a
   shadow under it; its width */
bool draw_load_font(const char *folder);
void draw_text(console_picture *frame, int x, int y, int scale, uint32_t colour, const char *text);
int draw_text_width(const char *text, int scale);

/* The game's own letters (the fixed layer's 8 x 8 font as the game's texts write them, from the player's tile ROM:
   capitals, digits and . , - / ( ) ' ? +, its copyright sign for '[' as the game's texts write it, and the console's
   own @ : < >), 'scale' times, at x, y (the top left), in one colour, their shadow darker; their width */
#define GAME_LETTER             8
void draw_game_letters(console_picture *frame, int x, int y, int scale, uint32_t colour, const char *text);
void draw_game_letters_centred(console_picture *frame, int y, int scale, uint32_t colour, const char *text);
int draw_game_letters_width(const char *text, int scale);

/* The signature's small letters (5 pixels high, the console's own: "@cheviman"), centred */
void draw_small_letters_centred(console_picture *frame, int y, uint32_t colour, const char *text);

/* A rectangle darkened ('keep': 0-256 of its light kept), filled, or its outline 'thickness' pixels wide */
void draw_shade(console_picture *frame, int x, int y, int width, int height, int keep);
void draw_fill(console_picture *frame, int x, int y, int width, int height, uint32_t colour);
void draw_outline(console_picture *frame, int x, int y, int width, int height, int thickness, uint32_t colour);

/* ---- save states (states.c) and the high scores kept (scores.c) ---- */

/* Each file's part of a save state (simpsons/state.h), what it keeps from frame to frame: console.c's joins and the
   players' last controls, characters.c's last pick, wide.c's account of the layers' rings and its sides, modes.c's
   mode under way (a file that keeps more from frame to frame in a game visits it there too) */
void console_joins_state(simpsons_state *state);
void characters_state(simpsons_state *state);
void wide_state(simpsons_state *state);
void modes_state(simpsons_state *state);
/* A save state was loaded: the high scores kept go back into the game's table once the game is out of a game (the
   state's table may be older than the kept one) */
void console_scores_state_loaded(void);

#endif
