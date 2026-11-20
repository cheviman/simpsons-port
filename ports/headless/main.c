/**
 *    Name:        main.c
 *    Description: the game with no screen or sound: the controls a script drives, and hashes of every frame in the
 *                 formats tools/core/konami/reference/kref writes
 *    Version:     0.2.0
 *    Created:     06-11-2026
 *    Modified:    14-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    Usage: simpsons_headless [--frames N] [--hashes FILE] [--video-hashes FILE] [--sound-hashes FILE]
 *                             [--sound-wav FILE] [--stereo] [--script FILE] [--service] [--assets FOLDER|PACK]
 *                             [--rom ROM_SET] [--write-pack FILE] [--dump FRAME ... --dump-dir DIR]
 *                             [--video-dump FRAME ... --video-dir DIR]
 *                             (simpsons_console_headless: [--actions FILE] [--wide KIND] [--wide-check]
 *                             [--wide-hashes FILE] [--arcade-start])
 *    The game (runtime/src/game.c: the program on the board, its own video and sound board) from its assets
 *    (--assets: the folder, assets/ by default, or the pack, simpsons.assets); --rom loads them from the player's ROM
 *    set straight instead, through the assets' layout (for the checks: the same state as the folder's).
 *    --write-pack writes the pack of what it loaded and stops. It runs with frame timing, as kref --timing frames does
 *    (simpsons/frame.h): the script is applied when each VBLANK starts, and the hashes are kref's ("FRAME HASH":
 *    simpsons_state_hash, the frame's pixels with --video-hashes, the sound board's samples and RAM with
 *    --sound-hashes, as kref --sound-timing makes them), so the two compare line by line. --sound-wav writes the
 *    sound the ports play (game_audio: stereo, 16 bits), as kref --sound-wav writes the reference's; with --stereo,
 *    as The Simpsons' SOUND: STEREO mixes it (game_set_stereo: the YM2151's two outputs, the hashes the same). --service
 *    holds the test switch from the start. --dump writes the board's memories at the end of those frames into
 *    --dump-dir's folder (FRAME_work_ram.bin, FRAME_palette.bin...), as kref --dump writes the reference's.
 *    --video-dump writes the frames FRAME ... as pictures into DIR (frame_FRAME.bmp, as kref --video-dump).
 *
 *    The console version's build (SIMPSONS_CONSOLE: simpsons_console_headless, The Simpsons Mr. Plow Edition with no
 *    window, for quick checks: Sebas, "Hace un headless para probar rápido") runs the console's front end with its
 *    settings the defaults and ends at --frames (600 unless given, or the script's length): the script's controls are
 *    the players' (the console puts them on their characters' slots), and --actions FILE gives the menus' actions and
 *    the players' own as the window's --actions does, a line each: the frame, then up, down, left, right, accept,
 *    back, start, pause or dump, and a player (1-4) for his own stick, buttons or START (start: player 1's when none);
 *    "pause" shows the pause's picture over the frames written from then on, CONTINUE chosen (the game goes on
 *    meanwhile: the window's pause holds it), and "dump" is the window's (none here). --wide KIND (off, 16:10, 16:9,
 *    20:9, auto, or its number: auto is OFF here, no screen) plays it with OPTIONS > VIDEO > WIDE so, the pictures
 *    written as wide as WIDE draws them, the console's picture over them; --wide-check says, each frame, which columns
 *    of WIDE's sides are not their layer's map's; --wide-hashes FILE writes, each frame, the hash of the picture WIDE
 *    draws (simpsons_video_wide_frame, before the console's own picture over it: "FRAME HASH WIDTH LEFT", the hash as
 *    --video-hashes' over its columns), so a change to the video is seen in every frame of it; --arcade-start runs the arcade's own attract mode and controls (a
 *    script's coins and buttons straight onto the board). Each picture written says where the console stands, on the
 *    standard output. Its letters: font.bmp beside it.
 */
#include "game/assets.h"
#include "game/data.h"
#include "game/game.h"
#include "simpsons/machine.h"
#include "simpsons/program.h"
#include "simpsons/script.h"
#include "simpsons/video.h"
#ifdef SIMPSONS_CONSOLE
#include "console.h"
#endif

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef SIMPSONS_CONSOLE
#define USAGE "Usage: simpsons_console_headless [--frames N] [--hashes FILE] [--video-hashes FILE]" \
	" [--sound-hashes FILE] [--sound-wav FILE] [--stereo] [--script FILE] [--assets FOLDER|PACK] [--rom ROM_SET]" \
	" [--dump FRAME ... --dump-dir DIR] [--video-dump FRAME ... --video-dir DIR] [--actions FILE] [--wide KIND]" \
	" [--wide-check] [--wide-hashes FILE] [--arcade-start]\n"
#else
#define USAGE "Usage: simpsons_headless [--frames N] [--hashes FILE] [--video-hashes FILE] [--sound-hashes FILE]" \
	" [--sound-wav FILE] [--stereo] [--script FILE] [--service] [--assets FOLDER|PACK] [--rom ROM_SET]" \
	" [--write-pack FILE]" \
	" [--dump FRAME ... --dump-dir DIR] [--video-dump FRAME ... --video-dir DIR]\n"
#endif
#define DUMPS_MAX 64

static uint64_t g_frames = 600;
static uint64_t g_dumps[DUMPS_MAX];
static int g_dump_count;
static const char *g_dump_dir = ".";
static uint64_t g_video_dumps[DUMPS_MAX];
static int g_video_dump_count;
static const char *g_video_dir = ".";
static FILE *g_hashes;
static FILE *g_video_hashes;
static FILE *g_sound_hashes;
#ifdef SIMPSONS_CONSOLE
static FILE *g_wide_hashes;
#endif
static simpsons_script *g_script;
static FILE *g_wav;
static uint32_t g_wav_bytes;

static FILE *open_output(const char *path)
{
	FILE *file = fopen(path, "w");

	if (!file)
	{
		fprintf(stderr, "%s: cannot be written\n", path);
		exit(1);
	}
	return file;
}

static void put_le(FILE *file, uint32_t value, int bytes)
{
	for (int index = 0; index < bytes; index++)
		fputc((int)(value >> (8 * index)) & 0xFF, file);
}

/* The WAV's header, 16-bit stereo at the sound's rate: written first with no length, then again with it */
static void wav_header(void)
{
	const uint32_t rate = game_audio_rate();

	fseek(g_wav, 0, SEEK_SET);
	fwrite("RIFF", 1, 4, g_wav);
	put_le(g_wav, 36u + g_wav_bytes, 4);
	fwrite("WAVEfmt ", 1, 8, g_wav);
	put_le(g_wav, 16, 4);
	put_le(g_wav, 1, 2);
	put_le(g_wav, 2, 2);
	put_le(g_wav, rate, 4);
	put_le(g_wav, rate * 4u, 4);
	put_le(g_wav, 4, 2);
	put_le(g_wav, 16, 2);
	fwrite("data", 1, 4, g_wav);
	put_le(g_wav, g_wav_bytes, 4);
}

static void finish(void)
{
	if (g_wav)
	{
		wav_header();
		fclose(g_wav);
	}
	if (g_hashes)
		fclose(g_hashes);
	if (g_video_hashes)
		fclose(g_video_hashes);
	if (g_sound_hashes)
		fclose(g_sound_hashes);
#ifdef SIMPSONS_CONSOLE
	if (g_wide_hashes)
		fclose(g_wide_hashes);
#endif
	printf("frames:           %" PRIu64 "\n", simpsons_board.frame);
	printf("interrupts taken:  %u\n", program_interrupts());
	printf("sound IRQs:        %u\n", simpsons_board.sound_irqs);
	printf("watchdog resets:   %u\n", simpsons_board.watchdog_resets);
	fflush(stdout);
	exit(0);
}

/* The board's memories at the end of a frame, a file each, as kref --dump writes them (the work RAM in the order
   of its addresses: the board keeps it the other way round) */
static void dump_memories(uint64_t frame)
{
	static uint8_t work_ram[SIMPSONS_WORK_RAM_SIZE];
	const struct { const char *name; const uint8_t *data; size_t size; } memories[] = {
		{ "work_ram", work_ram, sizeof(work_ram) },
		{ "extra_ram", simpsons_board.extra_ram, sizeof(simpsons_board.extra_ram) },
		{ "palette", simpsons_board.palette, sizeof(simpsons_board.palette) },
		{ "tile_ram", simpsons_board.tile_ram, sizeof(simpsons_board.tile_ram) },
		{ "sprite_ram", simpsons_board.sprite_ram, sizeof(simpsons_board.sprite_ram) },
		{ "object_ram", simpsons_board.object_ram, sizeof(simpsons_board.object_ram) },
		{ "eeprom", simpsons_board.eeprom.data, sizeof(simpsons_board.eeprom.data) },
	};

	for (uint32_t offset = 0; offset < SIMPSONS_WORK_RAM_SIZE; offset++)
		work_ram[offset] = *simpsons_work_ram_byte((uint16_t)(SIMPSONS_WORK_RAM + offset));
	for (size_t index = 0; index < sizeof(memories) / sizeof(memories[0]); index++)
	{
		char path[1024];
		FILE *file;

		snprintf(path, sizeof(path), "%s/%" PRIu64 "_%s.bin", g_dump_dir, frame, memories[index].name);
		file = fopen(path, "wb");
		if (!file)
		{
			fprintf(stderr, "%s: cannot be written\n", path);
			exit(1);
		}
		fwrite(memories[index].data, 1, memories[index].size, file);
		fclose(file);
	}
}

#ifdef SIMPSONS_CONSOLE
/* ---- --actions: the console's menus' actions and the players' own, by frame (the window's, ports/sdl2/main.c) ---- */

#define MAX_ACTIONS     4096
#define KIND_PAUSE      ACTIONS          /* (past the menus' actions) */
#define KIND_DUMP       (ACTIONS + 1)

static struct
{
	uint64_t frame;
	int kind;        /* ACTION_*, KIND_PAUSE or KIND_DUMP */
	int player;
	bool own;        /* (a player given: up ... back are that player's stick and buttons, not the menus' keys) */
} g_actions[MAX_ACTIONS];
static size_t g_action_count;
static bool g_pause_shown;    /* "pause": the pause's picture over the frames written from then on */
static bool g_wide_check;     /* --wide-check: each frame, WIDE's sides' columns that are not their map's */

static const char *load_actions(const char *path)
{
	static const char *const names[ACTIONS + 2] = { "up", "down", "left", "right", "accept", "back", "start", "pause",
		"dump" };
	char line[256], name[32];
	unsigned long long frame;
	int player;
	FILE *input = fopen(path, "r");

	if (!input)
		return "The actions file cannot be read.";
	while (fgets(line, sizeof(line), input) && g_action_count < MAX_ACTIONS)
	{
		int kind, read;

		if (line[0] == '#' || (read = sscanf(line, "%llu %31s %d", &frame, name, &player)) < 2)
			continue;
		for (kind = 0; kind < ACTIONS + 2 && strcmp(name, names[kind]) != 0; kind++)
			;
		if (kind == ACTIONS + 2)
		{
			fclose(input);
			return "An action is not one of up, down, left, right, accept, back, start, pause, dump.";
		}
		g_actions[g_action_count].frame = frame;
		g_actions[g_action_count].kind = kind;
		g_actions[g_action_count].player = read == 3 && player >= 1 && player <= CONSOLE_PLAYERS ? player - 1 : 0;
		g_actions[g_action_count].own = read == 3;
		g_action_count++;
	}
	fclose(input);
	return NULL;
}

/* The script's controls as the players' (the board's inputs it set, player N's on slot N), and the frame's actions,
   into the console: it puts them on the board for the next frame. A START that would pause shows the pause's picture
   (the game goes on) */
static void console_inputs(void)
{
	static const uint8_t controls[ACTIONS] = { SIMPSONS_UP, SIMPSONS_DOWN, SIMPSONS_LEFT, SIMPSONS_RIGHT,
		SIMPSONS_ATTACK, SIMPSONS_JUMP, 0 };
	uint8_t pressed[CONSOLE_PLAYERS], starts = 0;
	uint32_t menu_actions = 0;

	for (int player = 0; player < CONSOLE_PLAYERS; player++)
		pressed[player] = g_script ? (uint8_t)~simpsons_board.players[player] : 0u;    /* (without a script the board
			holds what the console put there: none of it again) */
	for (size_t index = 0; index < g_action_count; index++)
	{
		const int kind = g_actions[index].kind, player = g_actions[index].player;

		if (g_actions[index].frame != simpsons_board.frame || kind == KIND_DUMP)
			continue;
		if (kind == KIND_PAUSE)
			g_pause_shown = console_can_pause();
		else if (kind == ACTION_START)
		{
			if (console_start_pauses(player))
				g_pause_shown = true;
			else
				starts |= (uint8_t)(1u << player);
		}
		else if (g_actions[index].own)
			pressed[player] |= controls[kind];
		else if (g_pause_shown && (kind == ACTION_BACK || kind == ACTION_ACCEPT))
			g_pause_shown = false;    /* (the pause's CONTINUE) */
		else
			menu_actions |= ACTION_BIT(kind);
	}
	console_actions(console_controls_waiting() >= 0 ? 0 : menu_actions);
	console_controls(pressed, starts);
}
#endif

/* --video-dump: the frame the video drew as a 24-bit BMP, as kref writes them: WIDE's with its sides, the console's
   picture over it in its build; where the console stands, on the standard output */
static void write_bmp(const char *path, const uint32_t *pixels, int width, int stride)
{
	const int height = SIMPSONS_VIDEO_HEIGHT;
	const uint32_t row_bytes = ((uint32_t)width * 3u + 3u) & ~3u, size = 54u + row_bytes * (uint32_t)height;
	uint8_t header[54] = { 'B', 'M' };
	FILE *output = fopen(path, "wb");

	if (!output)
	{
		fprintf(stderr, "%s: cannot be written\n", path);
		exit(1);
	}
	for (int byte = 0; byte < 4; byte++)
	{
		header[2 + byte] = (uint8_t)(size >> (8 * byte));
		header[18 + byte] = (uint8_t)((uint32_t)width >> (8 * byte));
		header[22 + byte] = (uint8_t)((uint32_t)height >> (8 * byte));
		header[34 + byte] = (uint8_t)((row_bytes * (uint32_t)height) >> (8 * byte));
	}
	header[10] = 54;
	header[14] = 40;
	header[26] = 1;
	header[28] = 24;
	fwrite(header, 1, sizeof(header), output);
	for (int y = height - 1; y >= 0; y--)
	{
		static const uint8_t pad[3] = { 0, 0, 0 };

		for (int x = 0; x < width; x++)
		{
			const uint32_t pixel = pixels[y * stride + x];
			const uint8_t bgr[3] = { (uint8_t)pixel, (uint8_t)(pixel >> 8), (uint8_t)(pixel >> 16) };

			fwrite(bgr, 1, 3, output);
		}
		fwrite(pad, 1, row_bytes - (uint32_t)width * 3u, output);
	}
	fclose(output);
}

static void dump_video(uint64_t frame)
{
	static uint32_t pixels[SIMPSONS_VIDEO_MAX_WIDTH * SIMPSONS_VIDEO_HEIGHT];
	int width, left = 0;
	const uint32_t *wide = simpsons_video_wide_frame(&width);
	char path[1100];

	if (width > SIMPSONS_VIDEO_WIDTH)
	{
		left = simpsons_video_wide_left();
		for (int y = 0; y < SIMPSONS_VIDEO_HEIGHT; y++)
			memcpy(&pixels[y * SIMPSONS_VIDEO_MAX_WIDTH], &wide[y * SIMPSONS_VIDEO_MAX_WIDTH],
				(size_t)width * sizeof(uint32_t));
	}
	else
	{
		width = SIMPSONS_VIDEO_WIDTH;
		for (int y = 0; y < SIMPSONS_VIDEO_HEIGHT; y++)
			memcpy(&pixels[y * SIMPSONS_VIDEO_MAX_WIDTH], &game_frame()->pixels[y * SIMPSONS_VIDEO_WIDTH],
				SIMPSONS_VIDEO_WIDTH * sizeof(uint32_t));
	}
#ifdef SIMPSONS_CONSOLE
	{
		console_picture picture = { pixels, SIMPSONS_VIDEO_MAX_WIDTH, left, width - SIMPSONS_VIDEO_WIDTH - left };
		char text[1024];

		console_overlay(&picture);
		if (g_pause_shown)
			console_pause_overlay(&picture, CONSOLE_PAUSE_CONTINUE);
		console_describe(text, sizeof(text));
		printf("frame_%" PRIu64 ": %s\n", frame, text);
	}
#else
	(void)left;
#endif
	snprintf(path, sizeof(path), "%s/frame_%" PRIu64 ".bmp", g_video_dir, frame);
	write_bmp(path, pixels, width, SIMPSONS_VIDEO_MAX_WIDTH);
}

#ifdef SIMPSONS_CONSOLE
/* --wide-hashes: FNV-1a over the picture WIDE draws, its columns' red, green and blue row by row, as
   simpsons_video_hash's over the board's */
static void write_wide_hash(uint64_t frame)
{
	int width;
	const uint32_t *wide = simpsons_video_wide_frame(&width);
	uint64_t hash = 0xcbf29ce484222325ull;

	for (int y = 0; y < SIMPSONS_VIDEO_HEIGHT; y++)
		for (int x = 0; x < width; x++)
			for (int shift = 16; shift >= 0; shift -= 8)
				hash = (hash ^ ((wide[y * SIMPSONS_VIDEO_MAX_WIDTH + x] >> shift) & 0xFFu)) * 0x100000001b3ull;
	fprintf(g_wide_hashes, "%" PRIu64 " %016" PRIx64 " %d %d\n", frame, hash, width, simpsons_video_wide_left());
}
#endif

/* A frame is done: the controls change, the frame's sound closes and its hashes are taken, as kref's */
static void frame_done(void)
{
	const uint64_t number = simpsons_board.frame;

	simpsons_script_apply(g_script, simpsons_board.frame);
#ifdef SIMPSONS_CONSOLE
	console_inputs();
	console_wide_frame();    /* (--wide: from the next frame drawn) */
	if (g_wide_check)
	{
		char text[512];

		if (console_wide_check(text, sizeof(text)))
			printf("frame %" PRIu64 ": WIDE's sides not their map's: %s\n", number, text);
		if (console_wide_spawn_check(text, sizeof(text)))
			printf("frame %" PRIu64 ": put in in WIDE's sides' sight: %s\n", number, text);
	}
#endif
	game_end_frame();
	if (g_wav)
	{
		size_t count;
		const int16_t *samples = game_audio(&count);

		for (size_t index = 0; index < 2u * count; index++)
			put_le(g_wav, (uint16_t)samples[index], 2);
		g_wav_bytes += (uint32_t)(4u * count);
	}
	if (g_sound_hashes)
		fprintf(g_sound_hashes, "%" PRIu64 " %016" PRIx64 " %016" PRIx64 "\n", number, game_sound_samples_hash(),
			game_sound_state_hash());
	if (g_hashes)
		fprintf(g_hashes, "%" PRIu64 " %016" PRIx64 "\n", number, simpsons_state_hash());
	if (g_video_hashes)
		fprintf(g_video_hashes, "%" PRIu64 " %016" PRIx64 "\n", number, simpsons_video_hash(game_frame()));
#ifdef SIMPSONS_CONSOLE
	if (g_wide_hashes)
		write_wide_hash(number);
#endif
	for (int index = 0; index < g_dump_count; index++)
		if (g_dumps[index] == number)
			dump_memories(number);
	for (int index = 0; index < g_video_dump_count; index++)
		if (g_video_dumps[index] == number)
			dump_video(number);
	if (number >= g_frames)
		finish();
}

/* The game's assets from the folder (game_load), from a pack, or from the ROM set through the layout, into the
   memories game_load loads into (the programs' data as its pieces only): NULL, or what failed */
static const char *load(const char *assets, const char *rom)
{
	const game_assets_memory into[GAME_ASSETS_REGIONS] = {
		{ NULL, NULL }, { NULL, NULL }, { game_tiles, NULL }, { game_sprites, NULL }, { game_samples, NULL },
		{ game_eeprom, NULL } };

	if (rom)
		return game_assets_rom_set_load_into(rom, into);
	if (game_assets_is_pack(assets))
		return game_assets_pack_load_into(assets, into);
	return game_load(assets);
}

int main(int argc, char **argv)
{
	const char *assets = "assets", *rom = NULL, *pack = NULL, *script_path = NULL, *problem;
	bool frames_given = false, service = false;
#ifdef SIMPSONS_CONSOLE
	bool arcade_start = false;
#endif

	for (int index = 1; index < argc; index++)
	{
		const bool has_value = index + 1 < argc;

		if (!strcmp(argv[index], "--frames") && has_value)
		{
			g_frames = strtoull(argv[++index], NULL, 10);
			frames_given = true;
		}
		else if (!strcmp(argv[index], "--hashes") && has_value)
			g_hashes = open_output(argv[++index]);
		else if (!strcmp(argv[index], "--video-hashes") && has_value)
			g_video_hashes = open_output(argv[++index]);
		else if (!strcmp(argv[index], "--sound-hashes") && has_value)
			g_sound_hashes = open_output(argv[++index]);
		else if (!strcmp(argv[index], "--sound-wav") && has_value)
		{
			if (!(g_wav = fopen(argv[++index], "wb")))
			{
				fprintf(stderr, "%s: cannot be written\n", argv[index]);
				return 1;
			}
		}
		else if (!strcmp(argv[index], "--stereo"))
			game_set_stereo(true);
		else if (!strcmp(argv[index], "--script") && has_value)
			script_path = argv[++index];
		else if (!strcmp(argv[index], "--assets") && has_value)
			assets = argv[++index];
		else if (!strcmp(argv[index], "--rom") && has_value)
			rom = argv[++index];
		else if (!strcmp(argv[index], "--write-pack") && has_value)
			pack = argv[++index];
		else if (!strcmp(argv[index], "--service"))
			service = true;
		else if (!strcmp(argv[index], "--dump") && has_value)
			while (index + 1 < argc && argv[index + 1][0] != '-' && g_dump_count < DUMPS_MAX)
				g_dumps[g_dump_count++] = strtoull(argv[++index], NULL, 10);
		else if (!strcmp(argv[index], "--dump-dir") && has_value)
			g_dump_dir = argv[++index];
		else if (!strcmp(argv[index], "--video-dump") && has_value)
			while (index + 1 < argc && argv[index + 1][0] != '-' && g_video_dump_count < DUMPS_MAX)
				g_video_dumps[g_video_dump_count++] = strtoull(argv[++index], NULL, 10);
		else if (!strcmp(argv[index], "--video-dir") && has_value)
			g_video_dir = argv[++index];
#ifdef SIMPSONS_CONSOLE
		else if (!strcmp(argv[index], "--actions") && has_value)
		{
			if ((problem = load_actions(argv[++index])) != NULL)
			{
				fprintf(stderr, "%s\n", problem);
				return 1;
			}
		}
		else if (!strcmp(argv[index], "--wide") && has_value)
		{
			const char *kind = argv[++index];
			int wide = -1;

			for (int known = 0; known < WIDE_KINDS; known++)
				if (!strcmp(kind, console_wide_names[known]) || (kind[0] == '0' + known && !kind[1]))
					wide = known;
			if (wide < 0)
			{
				fprintf(stderr, "WIDE is one of off, 16:10, 16:9, 20:9, auto (or 0-4).\n");
				return 1;
			}
			console_settings.wide = wide;    /* (OPTIONS > VIDEO > WIDE) */
		}
		else if (!strcmp(argv[index], "--wide-check"))
			g_wide_check = true;
		else if (!strcmp(argv[index], "--wide-hashes") && has_value)
			g_wide_hashes = open_output(argv[++index]);
		else if (!strcmp(argv[index], "--arcade-start"))
			arcade_start = true;
#endif
		else
		{
			fprintf(stderr, "%s", USAGE);
			return 1;
		}
	}
	if ((problem = load(assets, rom)) != NULL)
	{
		fprintf(stderr, rom ? "The ROM set %s cannot be read: %s\n" : "The game's assets in %s cannot be read: %s\n",
			rom ? rom : assets, problem);
		return 1;
	}
	if (pack)
	{
		if ((problem = game_assets_pack_write(pack)) != NULL)
		{
			fprintf(stderr, "%s: %s\n", pack, problem);
			return 1;
		}
		printf("%s: the assets pack written (version %08X)\n", pack, (unsigned)game_assets_version());
		return 0;
	}
	if ((problem = game_reset(service, g_video_hashes != NULL || g_video_dump_count > 0
#ifdef SIMPSONS_CONSOLE
		|| g_wide_hashes != NULL
#endif
		)) != NULL)
	{
		fprintf(stderr, "The game does not start: %s\n", problem);
		return 1;
	}
#ifdef SIMPSONS_CONSOLE
	{
		/* (its letters beside the executable; OPTIONS' cabinet settings into the EEPROM) */
		char folder[1024];
		const char *slash = strrchr(argv[0], '/'), *backslash = strrchr(argv[0], '\\');
		const char *last = slash > backslash ? slash : backslash;

		snprintf(folder, sizeof(folder), "%.*s", last ? (int)(last - argv[0] + 1) : 0, argv[0]);
		if (!console_attach(folder))
			fprintf(stderr, "%sfont.bmp cannot be read: the menus have no letters\n", folder);
		if (arcade_start)
			console_arcade_test();    /* (the arcade's scenarios in the console's build) */
		console_scores_open(NULL);    /* (none kept: a check's) */
	}
#endif
	if (script_path)
	{
		char error[256];

		g_script = simpsons_script_load(script_path, error, sizeof(error));
		if (!g_script)
		{
			fprintf(stderr, "%s\n", error);
			return 1;
		}
		if (!frames_given && simpsons_script_frames(g_script))
			g_frames = simpsons_script_frames(g_script);
		simpsons_script_apply(g_script, 0);
	}
	if (g_wav)
		wav_header();
	game_frame_done = frame_done;
	program_run();
	return 3;
}
