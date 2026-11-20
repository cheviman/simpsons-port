/**
 *    Name:        video.c
 *    Description: The Simpsons' video: the K052109's three tile layers, the K053246/K053247's sprites, the
 *                 K053251's priorities and colour bases and the palette, drawn from the board's memories as the
 *                 board shows them
 *    Version:     0.1.0
 *    Created:     06-11-2026
 *    Modified:    19-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    Behavior follows MAME 0.289 pixel for pixel: konami/simpsons.cpp's screen update and callbacks (Ernesto
 *    Corvi), the K052109's, the K053246/K053247's and the K053251's (konami/k052109.cpp,
 *    konami/k053246_k053247_k055673.cpp, konami/k053251.cpp), konami_helper's layer sort, and what they use of
 *    emu/tilemap.cpp and emu/drawgfx.cpp (Aaron Giles) and of the palette (util/palette.cpp).
 *    tools/core/konami/reference builds those files into kref, and kref --video-check compares its frames with
 *    these. MAME's terms are in tools/core/konami/reference/mame/LICENSE.
 *
 *    How a frame is made, in a 384 x 264 bitmap of palette pens and one of priorities (lines 16-239 and columns
 *    0-319 shown): the background colour, then the three tile layers in the K053251's order, the one of lowest
 *    priority first, each without its pen 0 and marking the priority bitmap with 1, 2 and 4, then the sprites,
 *    nearest first: a sprite's pixel is not drawn where a nearer sprite was, nor over the layers its priority
 *    puts it under. A layer is a 512 x 256 map of 8 x 8 tiles; the screen's pixel shows the map's pixel its
 *    scroll gives (one for the layer, or one a row, a column, or both, as the K052109's scroll control register
 *    says), all of it turned round when the screen is flipped. The K052109 keeps what each tile shows (its
 *    code, colour and flip) from when it was last marked changed, as MAME's tile maps do. A sprite is 1 to 8
 *    x 8 cells of 16 x 16 pixels, zoomed, flipped or mirrored; its pen 15 shades what is under it when it
 *    casts a shadow. The pens become colours through the palette RAM (xBGR 555); a shaded pen takes 0.6 of
 *    its colour.
 *
 *    The pixels are MAME's; the way to them is this file's, for the machines that are slow: a tile's row and a
 *    cell's row are taken from the ROM whole (their pens a nibble each: tile_row, cell_row) and an empty one is
 *    passed at once, a layer with one scroll is drawn a tile at a time, the palette's pens are made again only for
 *    the colours that changed, and a line becomes colours in one pass.
 *
 *    A port's WIDE (simpsons_video_set_wide, the console's): the same frame with columns more at its sides, the
 *    layers' maps and the sprites drawn there too, the bitmaps' screen at their column BITMAP_LEFT; the board's own
 *    frame is drawn as it always is.
 *
 *    A port's recolouring (simpsons_video_set_recolour, the console's): a pixel of a pen the port names, of a sprite
 *    cell or a layer's tile it names, keeps its palette pen in the bitmap marked RECOLOURED, and takes the colour the
 *    port's function gives for the palette's; a sprite cell's pixel the port puts in the shade is marked SHADE, and
 *    takes the colour the port's shade gives for its colour code's shade pen; nothing is marked unless the port names
 *    something.
 */
#include "simpsons/video.h"

#include "game/data.h"
#include "simpsons/machine.h"
#include "simpsons/state.h"

#include <stdbool.h>
#include <string.h>

#define BITMAP_LEFT     (SIMPSONS_VIDEO_MAX_MORE + SIMPSONS_VIDEO_MAX_LESS)    /* the screen's column 0 in the bitmaps:
                                                                               room for WIDE's left */
#define BITMAP_WIDTH    (BITMAP_LEFT + SIMPSONS_VIDEO_WIDTH + SIMPSONS_VIDEO_MAX_MORE + SIMPSONS_VIDEO_MAX_LESS)
#define BITMAP_HEIGHT   264
#define AT(x, y)        ((y) * BITMAP_WIDTH + BITMAP_LEFT + (x))    /* a screen pixel's place in the bitmaps */
#define CLIP_LEFT       0
#define CLIP_RIGHT      319
#define CLIP_TOP        16
#define CLIP_BOTTOM     239
#define MAP_WIDTH       512
#define MAP_HEIGHT      256
#define MAP_TILES       (64 * 32)
#define COLOURS         2048            /* the palette's; the shaded ones follow them, then the highlighted */
#define COLOUR_CODES    (COLOURS / 16)  /* the colour codes of a 4-bit tile or cell */
#define TILES           (GAME_TILES_SIZE / 32)
#define SPRITE_CELLS    (GAME_SPRITES_SIZE / 128)
#define SHADOW_FACTOR   0.6f            /* the palette's default (emu/dipalette.h) */
#define SCREEN_WIDTH    384             /* the screen's width, as the K053247 asks it */
/* tilemap.cpp's flip sums: the visible area's x0 + x1 + 1 and y0 + y1 + 1 */
#define X_EXTENT        (CLIP_RIGHT + CLIP_LEFT + 1)
#define Y_EXTENT        (CLIP_BOTTOM + CLIP_TOP + 1)

/* the K052109's RAM: colour (attribute), then code for the fixed layer, A and B, its registers among them */
#define COLOUR_RAM      0x0000
#define CODE_RAM        0x2000
#define CODE_RAM_2      0x4000
#define TILE_RAM_END    0x1800          /* (of each 8 KB) */
#define SCROLL_Y        0x1800
#define SCROLL_X        0x1A00
#define REG_ADDRESS_MAP 0x1C00
#define REG_SCROLL      0x1C80
#define REG_BANKS_01    0x1D80
#define REG_FLIP        0x1E80
#define REG_BANKS_23    0x1F00

/* drawgfx's draw modes for a pen */
#define DRAW_NONE       0
#define DRAW_SOURCE     1
#define DRAW_SHADOW     2
#define DRAW_SHADOW_PRI 3

/* What a tile of a map shows, as the tile map last read it */
typedef struct
{
	uint16_t code;
	uint8_t colour;
	bool flip_y;
} tile_info;

typedef struct
{
	int dx;                         /* set_scrolldx (the same flipped) */
	int rows, cols;                 /* its scroll: one, a row, a column... (set_scroll_rows, set_scroll_cols) */
	int rowscroll[MAP_HEIGHT];
	int colscroll[MAP_WIDTH / 8];
	bool flipped;                   /* the tile map's flip, X and Y together */
	bool all_dirty;
	bool dirty[MAP_TILES];
	tile_info tiles[MAP_TILES];
} layer;

/* the K052109: its RAM as the chip has it (the board's, at the last VBLANK edge) and the registers it keeps */
static uint8_t g_tile_ram[SIMPSONS_TILE_RAM_SIZE];
static uint8_t g_address_map;
static uint8_t g_scroll_control;
static uint8_t g_tile_flip;
static uint8_t g_char_rom_bank[4];
static layer g_layers[3];

/* simpsons.cpp's: the layers' colour bases (they last drew with), the sprites' and the layers' sorted
   priorities */
static int g_layer_colour_base[3];
static int g_sprite_colour_base;
static int g_layer_priority[3];

/* the K053247's RAM (as the DMA left it) and the K053246's registers */
static uint16_t g_sprite_ram[SIMPSONS_SPRITE_RAM_SIZE / 2];
static uint8_t g_sprite_registers[8];

static uint16_t g_bitmap[BITMAP_HEIGHT * BITMAP_WIDTH];
static uint8_t g_priority[BITMAP_HEIGHT * BITMAP_WIDTH];
static uint32_t g_pens[3 * COLOURS];

/* WIDE (a port's): the columns drawn more at each side, the maps' ends on the screen (layers A and B), the fixed
   layer's columns and place; each taken from its _next as a frame is drawn. And the wider frame */
static int g_left, g_right, g_left_next, g_right_next;
static int g_first[3] = { -0x10000, -0x10000, -0x10000 }, g_last[3] = { 0x10000, 0x10000, 0x10000 };
static int g_first_next[3] = { -0x10000, -0x10000, -0x10000 }, g_last_next[3] = { 0x10000, 0x10000, 0x10000 };
static int g_fixed_dx, g_fixed_first = -0x10000, g_fixed_last = 0x10000;
static int g_fixed_dx_next, g_fixed_first_next = -0x10000, g_fixed_last_next = 0x10000;
static uint32_t g_wide[SIMPSONS_VIDEO_MAX_WIDTH * SIMPSONS_VIDEO_HEIGHT];
static int g_wide_width = SIMPSONS_VIDEO_WIDTH, g_wide_left;

/* The recolouring (a port's): its functions and tables; each layer's map cells' pens as the rules give them for the
   frame drawn; and the pens' colours recoloured and in the shade, as g_pens */
#define RECOLOURED      0x8000          /* (a bitmap pixel recoloured, */
#define SHADE           0x4000          /* or in the shade; the pens are below 3 * COLOURS) */
#define PEN_PART        0x3FFF
_Static_assert(SIMPSONS_VIDEO_SPRITE_CELLS == SPRITE_CELLS, "a cell's pens for each cell of the sprite ROM");
static simpsons_video_recolouring g_recolouring;
static simpsons_video_recolour g_recolour;    /* (g_recolouring's colour: none, none of it) */
static uint16_t g_tile_pens[3][MAP_TILES];
static uint32_t g_recoloured_pens[3 * COLOURS];
static uint32_t g_shade_pens[3 * COLOURS];

/* The palette RAM's colours the pens were last made from: g_pens' (none made yet: the RAM's zeros, black), the
   recoloured pens', and each colour code's shade pen's for the shade's; and whether the recolouring was set since */
static uint16_t g_palette_raw[COLOURS];
static uint16_t g_recoloured_raw[COLOURS];
static uint16_t g_shade_raw[COLOUR_CODES];
static bool g_recolour_set = true;

static void mark_tile(int map, int index)
{
	g_layers[map].dirty[index] = true;
}

static void mark_all(int map)
{
	g_layers[map].all_dirty = true;
}

/* k052109.cpp's tileflip_reset: the tile maps' flip (a change marks them all) */
static void flip_reset(void)
{
	const bool flipped = (g_tile_flip & 0x01) != 0;

	for (int map = 0; map < 3; map++)
		if (g_layers[map].flipped != flipped)
		{
			g_layers[map].flipped = flipped;
			mark_all(map);
		}
}

/* k052109.cpp's write, for what the drawing uses: the tiles a write changes, the address map, the scroll
   control, the ROM banks and the flips */
static void tile_chip_write(int offset, uint8_t data)
{
	g_tile_ram[offset] = data;
	if ((offset & 0x1FFF) < TILE_RAM_END)
		mark_tile((offset & 0x1800) >> 11, offset & 0x7FF);
	else if (offset == REG_ADDRESS_MAP)
		g_address_map = data;
	else if (offset == REG_SCROLL)
		g_scroll_control = data;
	else if (offset == REG_BANKS_01 || offset == REG_BANKS_23)
	{
		const int first = offset == REG_BANKS_01 ? 0 : 2;
		int dirty = 0;

		if (g_char_rom_bank[first] != (data & 0x0F))
			dirty |= 1;
		if (g_char_rom_bank[first + 1] != ((data >> 4) & 0x0F))
			dirty |= 2;
		if (dirty)
		{
			g_char_rom_bank[first] = data & 0x0F;
			g_char_rom_bank[first + 1] = (data >> 4) & 0x0F;
			/* the tiles whose colour picks a bank that changed */
			for (int index = 0; index < TILE_RAM_END; index++)
			{
				const int bank = (g_tile_ram[index] & 0x0C) >> 2;

				if ((bank == first && (dirty & 1)) || (bank == first + 1 && (dirty & 2)))
					mark_tile((index & 0x1800) >> 11, index & 0x7FF);
			}
		}
	}
	else if (offset == REG_FLIP)
	{
		if ((g_tile_flip & 0x06) != (data & 0x06))
			for (int map = 0; map < 3; map++)
				mark_all(map);
		g_tile_flip = data & 0x07;
		flip_reset();
	}
}

/* The board's memories and registers into the chips: what the program wrote since they last took them (kref's
   video takes them the same way) */
static void load_chips(void)
{
	/* (the writes in the RAM's order; a stretch nothing wrote to is passed at once) */
	_Static_assert(SIMPSONS_TILE_RAM_SIZE % 64 == 0, "the tile RAM compared in stretches of 64 bytes");
	for (int stretch = 0; stretch < SIMPSONS_TILE_RAM_SIZE; stretch += 64)
		if (memcmp(&g_tile_ram[stretch], &simpsons_board.tile_ram[stretch], 64) != 0)
			for (int offset = stretch; offset < stretch + 64; offset++)
				if (g_tile_ram[offset] != simpsons_board.tile_ram[offset])
					tile_chip_write(offset, simpsons_board.tile_ram[offset]);
	for (int word = 0; word < SIMPSONS_SPRITE_RAM_SIZE / 2; word++)
		g_sprite_ram[word] = (uint16_t)(simpsons_board.object_ram[word * 2] << 8 | simpsons_board.object_ram[word * 2 + 1]);
	memcpy(g_sprite_registers, simpsons_board.sprite_registers, sizeof(g_sprite_registers));
}

void simpsons_video_reset(void)
{
	memset(g_tile_ram, 0, sizeof(g_tile_ram));
	g_address_map = g_scroll_control = g_tile_flip = 0;
	memset(g_char_rom_bank, 0, sizeof(g_char_rom_bank));
	memset(g_layers, 0, sizeof(g_layers));
	g_layers[0].dx = -96;
	g_layers[1].dx = -90;
	g_layers[2].dx = -90;
	for (int map = 0; map < 3; map++)
	{
		g_layers[map].rows = g_layers[map].cols = 1;
		g_layers[map].all_dirty = true;
	}
	memset(g_layer_colour_base, 0, sizeof(g_layer_colour_base));
	g_sprite_colour_base = 0;
	memset(g_layer_priority, 0, sizeof(g_layer_priority));
	memset(g_sprite_ram, 0, sizeof(g_sprite_ram));
	memset(g_sprite_registers, 0, sizeof(g_sprite_registers));
}

/* k052109.cpp's update_scroll, for layers A and B */
static void update_scroll(void)
{
	static const int rows_table[4] = { 1, 1, 32, 256 };

	for (int map = 0; map < 2; map++)
	{
		const int control = (g_scroll_control >> (map * 3)) & 7;
		const int rows = rows_table[control & 3];
		const int cols = (control & 4) ? 64 : 1;
		const uint8_t *scroll_y = &g_tile_ram[SCROLL_Y | (map ? 0x2000 : 0)];
		const uint8_t *scroll_x = &g_tile_ram[SCROLL_X | (map ? 0x2000 : 0)];
		layer *target = &g_layers[map + 1];

		if (rows == 1 && cols == 1)
		{
			target->rows = target->cols = 1;
			target->rowscroll[0] = scroll_x[0] + 256 * scroll_x[1];
			target->colscroll[0] = scroll_y[12];
		}
		else if (cols == 1)
		{
			const int y = scroll_y[12];
			const int mask = rows == 256 ? 0xFF : 0xF8;

			target->rows = 256;
			target->cols = 1;
			target->colscroll[0] = y;
			for (int offset = 0; offset < 256; offset++)
				target->rowscroll[(offset + y) & 0xFF] = scroll_x[2 * (offset & mask)] + 256 * scroll_x[2 * (offset & mask) + 1];
		}
		else if (rows == 1)
		{
			const int x = scroll_x[0] + 256 * scroll_x[1];

			target->rows = 1;
			target->cols = 64;
			target->rowscroll[0] = x;
			for (int offset = 0; offset < 64; offset++)
				target->colscroll[(offset + x / 8) & 0x3F] = scroll_y[offset];
		}
		else
		{
			const int step = rows == 256 ? 2 : 16;

			target->rows = rows;
			target->cols = 64;
			for (int offset = 0; offset < rows; offset++)
				target->rowscroll[offset] = scroll_x[step * offset] + 256 * scroll_x[step * offset + 1];
			for (int offset = 0; offset < 64; offset++)
				target->colscroll[offset] = scroll_y[offset];
		}
	}
}

/* k052109.cpp's get_tile_info with simpsons.cpp's tile callback: what tile 'index' of map 'map' shows now */
static tile_info read_tile(int map, int index)
{
	const int place = map * 0x800 + index;
	int code = g_tile_ram[CODE_RAM + place] + 256 * g_tile_ram[CODE_RAM_2 + place];
	int colour = g_tile_ram[COLOUR_RAM + place];
	int bank = g_char_rom_bank[(colour & 0x0C) >> 2];
	tile_info info;

	if (!(g_address_map & 0x40))
		colour = (colour & 0xF3) | ((bank & 0x03) << 2);
	bank >>= 2;
	info.flip_y = (colour & 0x02) && (g_tile_flip & 0x04);
	code |= ((colour & 0x3F) << 8) | (bank << 14);
	colour = g_layer_colour_base[map] + ((colour & 0xC0) >> 6);
	info.code = (uint16_t)(code % TILES);
	info.colour = (uint8_t)(colour % COLOUR_CODES);
	return info;
}

/* tilemap.cpp's pixmap_update: the tiles marked changed read again */
static void update_tiles(int map)
{
	layer *target = &g_layers[map];

	if (target->all_dirty)
	{
		for (int index = 0; index < MAP_TILES; index++)
			target->dirty[index] = true;
		target->all_dirty = false;
	}
	for (int index = 0; index < MAP_TILES; index++)
		if (target->dirty[index])
		{
			target->tiles[index] = read_tile(map, index);
			target->dirty[index] = false;
		}
}

/* tilemap.cpp's effective_rowscroll and effective_colscroll: where the map's pixmap starts on the screen */
static int effective_rowscroll(const layer *map, int index, bool flipped)
{
	int value;

	if (flipped)
		index = map->rows - 1 - index;
	value = flipped ? X_EXTENT - MAP_WIDTH - (map->dx - map->rowscroll[index]) : map->dx - map->rowscroll[index];
	return value < 0 ? MAP_WIDTH - (-value) % MAP_WIDTH : value % MAP_WIDTH;
}

static int effective_colscroll(const layer *map, int index, bool flipped)
{
	int value;

	if (flipped)
		index = map->cols - 1 - index;
	value = flipped ? Y_EXTENT - MAP_HEIGHT - (0 - map->colscroll[index]) : 0 - map->colscroll[index];
	return value < 0 ? MAP_HEIGHT - (-value) % MAP_HEIGHT : value % MAP_HEIGHT;
}

static int wrap(int value, int size)
{
	value %= size;
	return value < 0 ? value + size : value;
}

/* A byte of a tile's row (a plane's: its bit 7 the leftmost pixel) with each pixel's bit at the bottom of a nibble,
   column c's at bit 4c */
#define SPREAD_BIT(byte, column)    ((uint32_t)(((byte) >> (7 - (column))) & 1) << (4 * (column)))
#define SPREAD(byte)                (SPREAD_BIT(byte, 0) | SPREAD_BIT(byte, 1) | SPREAD_BIT(byte, 2) | SPREAD_BIT(byte, 3) \
	| SPREAD_BIT(byte, 4) | SPREAD_BIT(byte, 5) | SPREAD_BIT(byte, 6) | SPREAD_BIT(byte, 7))
#define SPREAD_4(byte)              SPREAD(byte), SPREAD((byte) + 1), SPREAD((byte) + 2), SPREAD((byte) + 3)
#define SPREAD_16(byte)             SPREAD_4(byte), SPREAD_4((byte) + 4), SPREAD_4((byte) + 8), SPREAD_4((byte) + 12)
#define SPREAD_64(byte)             SPREAD_16(byte), SPREAD_16((byte) + 16), SPREAD_16((byte) + 32), SPREAD_16((byte) + 48)
static const uint32_t k_spread[256] = { SPREAD_64(0), SPREAD_64(64), SPREAD_64(128), SPREAD_64(192) };

/* The eight pens of a row of a tile (four bytes a row, a plane each), column c's in bits 4c to 4c + 3 */
static inline uint32_t tile_row(int code, int row)
{
	const uint8_t *data = &game_tiles[(size_t)code * 32u + (size_t)row * 4u];

	if (!(data[0] | data[1] | data[2] | data[3]))
		return 0;    /* (an empty row, as most of a layer over the others are) */
	return k_spread[data[0]] | k_spread[data[1]] << 1 | k_spread[data[2]] << 2 | k_spread[data[3]] << 3;
}

/* (whether none of a row's eight pens is 0) */
#define NO_PEN_0(pens)  ((((pens) | (pens) >> 1 | (pens) >> 2 | (pens) >> 3) & 0x11111111u) == 0x11111111u)

/* The recolouring's rules for layer 'map' (a port's): its map cells' pens recoloured in the frame drawn, from the tiles
   the map shows */
static void take_tile_rules(int map)
{
	memset(g_tile_pens[map], 0, sizeof(g_tile_pens[map]));
	for (int index = 0; g_recolour && g_recolouring.tiles && index < g_recolouring.tile_rules; index++)
	{
		const simpsons_video_tile_rule *rule = &g_recolouring.tiles[index];

		if (rule->map != map)
			continue;
		for (int row = rule->first_row; row <= rule->last_row && row < MAP_TILES / 64; row++)
			for (int column = rule->first_column; column <= rule->last_column && column < 64; column++)
			{
				const int code = g_layers[map].tiles[row * 64 + column].code;

				if (code >= rule->first_code && code <= rule->last_code)
					g_tile_pens[map][row * 64 + column] |= rule->pens;
			}
	}
}

/* (whether a tile shows nothing: its eight rows empty) */
static inline bool tile_empty(int code)
{
	uint64_t rows[4];

	memcpy(rows, &game_tiles[(size_t)code * 32u], sizeof(rows));
	return !(rows[0] | rows[1] | rows[2] | rows[3]);
}

/* A run of a tile row's pixels over a line of the bitmap, without its pen 0, marked 'priority' in the priority
   bitmap: 'run' pixels from 'line' and 'line_priority' on, the row's columns from 'column' on (to the right, or to
   the left when flipped: a run of 8 not flipped is the whole row, from its column 0), in colour 'colour' (its first
   pen's place in the palette); 'recoloured', the pens a port recolours in this tile */
static inline void draw_tile_run(uint16_t *line, uint8_t *line_priority, int run, uint32_t pens, int column, bool flipped,
	int colour, unsigned recoloured, uint8_t priority)
{
	if (run == 8 && !flipped && !recoloured && NO_PEN_0(pens))
	{
		/* (a whole row with no pen 0, as most of a background's are: its eight pixels at once) */
		uint64_t under;

		for (int index = 0; index < 8; index++)
			line[index] = (uint16_t)(colour + (int)((pens >> (4 * index)) & 15));
		memcpy(&under, line_priority, sizeof(under));
		under |= priority * 0x0101010101010101ull;
		memcpy(line_priority, &under, sizeof(under));
		return;
	}
	for (int index = 0; index < run; index++, column += flipped ? -1 : 1)
	{
		const unsigned pen = (pens >> (4 * column)) & 15;

		if (pen)
		{
			line[index] = (uint16_t)((colour + (int)pen) | ((recoloured >> pen) & 1 ? RECOLOURED : 0));
			line_priority[index] |= priority;
		}
	}
}

/* k052109's tilemap_draw as simpsons.cpp calls it: the map 'map' over the bitmap without its pen 0, its pixels
   marked 'priority' in the priority bitmap */
static void draw_layer(int map, uint8_t priority)
{
	const layer *scroll = &g_layers[map];
	const bool flipped = scroll->flipped;
	int rowscroll[MAP_HEIGHT], colscroll[MAP_WIDTH / 8];
	bool same_rows = true, same_cols = true;
	int rows, cols;

	update_tiles(map);
	take_tile_rules(map);
	for (int index = 0; index < scroll->rows; index++)
	{
		rowscroll[index] = effective_rowscroll(scroll, index, flipped);
		same_rows = same_rows && rowscroll[index] == rowscroll[0];
	}
	for (int index = 0; index < scroll->cols; index++)
	{
		colscroll[index] = effective_colscroll(scroll, index, flipped);
		same_cols = same_cols && colscroll[index] == colscroll[0];
	}
	/* (k052109's tile_blitter takes the scroll of both rows and columns; else tilemap.cpp, which treats a
	   scroll whose rows, or columns, are all the same as one) */
	const bool mixed = scroll->rows > 1 && scroll->cols > 1;
	rows = same_rows ? 1 : scroll->rows;
	cols = same_cols ? 1 : scroll->cols;

	/* The screen's columns drawn (WIDE: its sides too, the fixed layer's columns alone, moved by dx, and nothing past
	   the map's end) */
	const int dx = map == 0 ? g_fixed_dx : 0;
	const int source_first = map == 0 ? g_fixed_first : -0x10000, source_last = map == 0 ? g_fixed_last : 0x10000;
	int64_t first = CLIP_LEFT - g_left, last = CLIP_RIGHT + g_right;

	if (first < (int64_t)source_first + dx)
		first = (int64_t)source_first + dx;
	if (last > (int64_t)source_last + dx)
		last = (int64_t)source_last + dx;
	if (first < g_first[map])
		first = g_first[map];
	if (last > g_last[map])
		last = g_last[map];
	if (first > last)
		return;

	const int first_x = (int)first, last_x = (int)last;    /* (some of the picture's: an int's) */

	/* px and py are the map's pixel a screen pixel shows, before the flip turns the map round; a run is a line's
	   pixels of one tile, from px to the end of the tile or of the columns drawn */
	if (!mixed && rows == 1 && cols == 1)
	{
		/* One scroll for the whole layer, as nearly every frame has: a tile at a time, its lines one after another,
		   and a tile that shows nothing passed at once */
		const int px_first = wrap(first_x - dx - rowscroll[0], MAP_WIDTH);
		int lines, run;

		for (int y = CLIP_TOP; y <= CLIP_BOTTOM; y += lines)
		{
			const int py = wrap(y - colscroll[0], MAP_HEIGHT);

			lines = 8 - (py & 7);    /* (to the end of the row of tiles) */
			if (lines > CLIP_BOTTOM - y + 1)
				lines = CLIP_BOTTOM - y + 1;
			for (int x = first_x, px = px_first; x <= last_x; x += run, px = (px + run) & (MAP_WIDTH - 1))
			{
				const int map_x = flipped ? MAP_WIDTH - 1 - px : px, map_y = flipped ? MAP_HEIGHT - 1 - py : py;
				const int cell = (map_y >> 3) * 64 + (map_x >> 3);
				const tile_info *info = &scroll->tiles[cell];

				run = 8 - (px & 7);
				if (run > last_x - x + 1)
					run = last_x - x + 1;
				if (tile_empty(info->code))
					continue;
				for (int index = 0; index < lines; index++)
				{
					/* (the line's row of the tile: the map's rows go up the screen when flipped) */
					const int row = (flipped ? map_y - index : map_y + index) & 7;
					const uint32_t pens = tile_row(info->code, info->flip_y ? 7 - row : row);

					if (pens)
						draw_tile_run(&g_bitmap[AT(x, y + index)], &g_priority[AT(x, y + index)], run, pens, map_x & 7,
							flipped, info->colour * 16, g_tile_pens[map][cell], priority);
				}
			}
		}
		return;
	}

	/* A scroll by rows, by columns or both: a line at a time (a line's scroll is one, and a column of the map has
	   one too) */
	for (int y = CLIP_TOP; y <= CLIP_BOTTOM; y++)
	{
		int line_scroll = rowscroll[0], line_py = 0, start_x = 0, run;

		if (mixed)
		{
			const int band = y / (MAP_HEIGHT / scroll->rows);

			line_scroll = rowscroll[band];
			start_x = scroll->rowscroll[flipped ? scroll->rows - 1 - band : band] & ~7;
			if (!flipped)
				start_x = -start_x;
		}
		else if (cols == 1)
		{
			line_py = wrap(y - colscroll[0], MAP_HEIGHT);
			line_scroll = rowscroll[line_py / (MAP_HEIGHT / rows)];
		}
		for (int x = first_x, px = wrap(first_x - dx - line_scroll, MAP_WIDTH); x <= last_x;
			x += run, px = (px + run) & (MAP_WIDTH - 1))
		{
			int py = line_py;

			run = 8 - (px & 7);
			if (run > last_x - x + 1)
				run = last_x - x + 1;
			if (mixed)
				py = wrap(y - colscroll[((start_x + (px & ~7)) >> 3) & 0x3F], MAP_HEIGHT);
			else if (cols != 1)
				py = wrap(y - colscroll[px / (MAP_WIDTH / cols)], MAP_HEIGHT);

			const int map_x = flipped ? MAP_WIDTH - 1 - px : px, map_y = flipped ? MAP_HEIGHT - 1 - py : py;
			const int cell = (map_y >> 3) * 64 + (map_x >> 3);
			const tile_info *info = &scroll->tiles[cell];
			const uint32_t pens = tile_row(info->code, info->flip_y ? 7 - (map_y & 7) : map_y & 7);

			if (pens)
				draw_tile_run(&g_bitmap[AT(x, y)], &g_priority[AT(x, y)], run, pens, map_x & 7, flipped, info->colour * 16,
					g_tile_pens[map][cell], priority);
		}
	}
}

/* The sixteen pens of a row of a 16 x 16 sprite cell (the K053247's sprite layout: 8 bytes a row, two pixels a byte,
   the left one its high nibble, the bytes of each word swapped), column c's in bits 4c to 4c + 3 */
static uint64_t cell_row(int code, int y)
{
	const uint8_t *data = &game_sprites[(size_t)code * 128u + (size_t)y * 8u];
	uint64_t row = 0;

	for (int byte = 0; byte < 8; byte++)
		row |= (uint64_t)data[byte] << (8 * byte);
	row = (row & 0x00FF00FF00FF00FFull) << 8 | ((row >> 8) & 0x00FF00FF00FF00FFull);
	return (row & 0x0F0F0F0F0F0F0F0Full) << 4 | ((row >> 4) & 0x0F0F0F0F0F0F0F0Full);
}

/* The sixteen pixels of that row a port's recolouring puts in the shade (none: NULL), column c's in bit c */
static unsigned shade_row(const uint8_t *shade_mask, int y)
{
	return shade_mask ? (unsigned)(shade_mask[2 * y] | shade_mask[2 * y + 1] << 8) : 0u;
}

/* drawgfx's PIXEL_OP_REBASE_TRANSTABLE16_PRIORITY: a pixel of a cell over the bitmap, by the pen's draw mode
   (never pen 0, which draws nothing), unless the priority bitmap says something nearer is there */
static inline void cell_pen(uint16_t *destination, uint8_t *priority, int pen, int colour, uint32_t mask,
	const uint8_t *modes, unsigned recoloured_pens, bool shade)
{
	const int mode = modes[pen];
	const uint8_t under = *priority;

	if (mode == DRAW_SOURCE)
	{
		if (((1u << (under & 0x1F)) & mask) == 0)
		{
			const int value = colour + pen;
			/* (a port's recolouring) */
			const int recoloured = shade ? SHADE : (recoloured_pens >> pen) & 1 ? RECOLOURED : 0;

			if ((under & 0xC0) == (DRAW_SHADOW_PRI << 6))
				*destination = (uint16_t)(((uint16_t)value < COLOURS ? value + COLOURS : value) | recoloured);
			else
				*destination = (uint16_t)(value | recoloured);
		}
		*priority = 31;
	}
	else if ((under & 0x80) == 0 && ((1u << (under & 0x1F)) & mask) == 0)
	{
		/* (the shadow table: a pen of the palette to its shaded one, the others kept; a recoloured one stays so) */
		if ((*destination & PEN_PART) < COLOURS)
			*destination = (uint16_t)(*destination + COLOURS);
		*priority = (uint8_t)(under | (mode << 6));
	}
}

/* A sprite cell drawn in colour code 'colour' (the sprites' base and the attributes' 5 bits: the base a multiple of
   32): whether a port's recolouring names it in that colour; its pens recoloured; its pixels in the shade (NULL: none) */
static bool cell_recoloured(int code, int colour)
{
	return g_recolour && g_recolouring.cell_colours
		&& ((g_recolouring.cell_colours[code % SPRITE_CELLS] >> (colour & 0x1F)) & 1u);
}

static unsigned cell_recoloured_pens(int code, int colour)
{
	return cell_recoloured(code, colour) && g_recolouring.cell_pens ? g_recolouring.cell_pens[code % SPRITE_CELLS] : 0u;
}

static const uint8_t *cell_shade_mask(int code, int colour)
{
	if (!cell_recoloured(code, colour) || !g_recolouring.cell_shades || !g_recolouring.shade_masks
		|| !g_recolouring.cell_shades[code % SPRITE_CELLS])
		return NULL;
	return g_recolouring.shade_masks[g_recolouring.cell_shades[code % SPRITE_CELLS] - 1];
}

/* drawgfx's prio_transtable: a cell at its size */
static void draw_cell(int code, int colour, bool flip_x, bool flip_y, int left, int top, uint32_t mask, const uint8_t *modes)
{
	const int clip_left = CLIP_LEFT - g_left, clip_right = CLIP_RIGHT + g_right;    /* (WIDE's sides too) */
	int right = left + 15, bottom = top + 15, source_x = 0, source_y = 0;

	if (left > clip_right || right < clip_left || top > CLIP_BOTTOM || bottom < CLIP_TOP)
		return;
	if (left < clip_left)
	{
		source_x = clip_left - left;
		left = clip_left;
	}
	if (right > clip_right)
		right = clip_right;
	if (top < CLIP_TOP)
	{
		source_y = CLIP_TOP - top;
		top = CLIP_TOP;
	}
	if (bottom > CLIP_BOTTOM)
		bottom = CLIP_BOTTOM;
	if (flip_x)
		source_x = 15 - source_x;
	if (flip_y)
		source_y = 15 - source_y;
	mask |= 1u << 31;
	const unsigned recoloured_pens = cell_recoloured_pens(code, colour);
	const uint8_t *shade_mask = cell_shade_mask(code, colour);
	colour = 16 * (colour % COLOUR_CODES);
	code %= SPRITE_CELLS;
	for (int y = top; y <= bottom; y++, source_y += flip_y ? -1 : 1)
	{
		const uint64_t pens = cell_row(code, source_y);
		const unsigned shade = shade_row(shade_mask, source_y);
		uint16_t *line = &g_bitmap[AT(left, y)];
		uint8_t *line_priority = &g_priority[AT(left, y)];

		if (!pens)
			continue;
		for (int x = 0, sx = source_x; x <= right - left; x++, sx += flip_x ? -1 : 1)
		{
			const int pen = (int)(pens >> (4 * sx)) & 15;

			if (pen)
				cell_pen(&line[x], &line_priority[x], pen, colour, mask, modes, recoloured_pens, (shade >> sx) & 1);
		}
	}
}

/* drawgfx's prio_zoom_transtable: a cell scaled (16.16 scales), at its size when they are 1 */
static void draw_cell_zoomed(int code, int colour, bool flip_x, bool flip_y, int left, int top, uint32_t scale_x,
	uint32_t scale_y, uint32_t mask, const uint8_t *modes)
{
	if (scale_x == 0x10000 && scale_y == 0x10000)
	{
		draw_cell(code, colour, flip_x, flip_y, left, top, mask, modes);
		return;
	}
	mask |= 1u << 31;
	const unsigned recoloured_pens = cell_recoloured_pens(code, colour);
	const uint8_t *shade_mask = cell_shade_mask(code, colour);
	colour = 16 * (colour % COLOUR_CODES);
	code %= SPRITE_CELLS;

	const uint32_t width = (scale_x * 16 + 0x8000) >> 16;
	const uint32_t height = (scale_y * 16 + 0x8000) >> 16;
	const int clip_left = CLIP_LEFT - g_left, clip_right = CLIP_RIGHT + g_right;    /* (WIDE's sides too) */
	int32_t dx, dy, right, bottom, source_x = 0, source_y = 0;

	if (width < 1 || height < 1)
		return;
	dx = (16 << 16) / (int32_t)width;
	dy = (16 << 16) / (int32_t)height;
	right = left + (int32_t)width - 1;
	if (left > clip_right || right < clip_left)
		return;
	if (left < clip_left)
	{
		source_x = (clip_left - left) * dx;
		left = clip_left;
	}
	if (right > clip_right)
		right = clip_right;
	bottom = top + (int32_t)height - 1;
	if (top > CLIP_BOTTOM || bottom < CLIP_TOP)
		return;
	if (top < CLIP_TOP)
	{
		source_y = (CLIP_TOP - top) * dy;
		top = CLIP_TOP;
	}
	if (bottom > CLIP_BOTTOM)
		bottom = CLIP_BOTTOM;
	if (flip_x)
	{
		source_x = (int32_t)(width - 1) * dx - source_x;
		dx = -dx;
	}
	if (flip_y)
	{
		source_y = (int32_t)(height - 1) * dy - source_y;
		dy = -dy;
	}
	for (int y = top; y <= bottom; y++, source_y += dy)
	{
		const uint64_t pens = cell_row(code, source_y >> 16);
		const unsigned shade = shade_row(shade_mask, source_y >> 16);
		uint16_t *line = &g_bitmap[AT(left, y)];
		uint8_t *line_priority = &g_priority[AT(left, y)];
		int32_t sx = source_x;

		if (!pens)
			continue;
		for (int x = 0; x <= right - left; x++, sx += dx)
		{
			const int pen = (int)(pens >> (4 * (sx >> 16))) & 15;

			if (pen)
				cell_pen(&line[x], &line_priority[x], pen, colour, mask, modes, recoloured_pens,
					(shade >> (sx >> 16)) & 1);
		}
	}
}

/* k053247_draw_yxloop_gx, the non-GX path: a sprite's cells */
static void draw_cells(int code, int colour, int height, int width, int zoom_x, int zoom_y, bool flip_x, bool flip_y,
	int ox, int oy, int xa, int ya, bool mirror_x, bool mirror_y, bool no_zoom, uint32_t mask, const uint8_t *modes)
{
	static const int x_offset[8] = { 0, 1, 4, 5, 16, 17, 20, 21 };
	static const int y_offset[8] = { 0, 2, 8, 10, 32, 34, 40, 42 };

	for (int y = 0; y < height; y++)
	{
		const int sy = oy + ((zoom_y * y + (1 << 11)) >> 12);
		const int zh = (oy + ((zoom_y * (y + 1) + (1 << 11)) >> 12)) - sy;

		for (int x = 0; x < width; x++)
		{
			const int sx = ox + ((zoom_x * x + (1 << 11)) >> 12);
			const int zw = (ox + ((zoom_x * (x + 1) + (1 << 11)) >> 12)) - sx;
			int cell = code;
			bool fx, fy;

			if (mirror_x)
			{
				if ((!flip_x) ^ ((x << 1) < width))
				{
					cell += x_offset[(width - 1 - x + xa) & 7];
					fx = true;
				}
				else
				{
					cell += x_offset[(x + xa) & 7];
					fx = false;
				}
			}
			else
			{
				cell += flip_x ? x_offset[(width - 1 - x + xa) & 7] : x_offset[(x + xa) & 7];
				fx = flip_x;
			}
			if (mirror_y)
			{
				if ((!flip_y) ^ ((y << 1) >= height))
				{
					cell += y_offset[(height - 1 - y + ya) & 7];
					fy = true;
				}
				else
				{
					cell += y_offset[(y + ya) & 7];
					fy = false;
				}
			}
			else
			{
				cell += flip_y ? y_offset[(height - 1 - y + ya) & 7] : y_offset[(y + ya) & 7];
				fy = flip_y;
			}

			if (no_zoom)
				draw_cell(cell, colour, fx, fy, sx, sy, mask, modes);
			else
				draw_cell_zoomed(cell, colour, fx, fy, sx, sy, (uint32_t)zw << 12, (uint32_t)zh << 12, mask, modes);
			/* The Simpsons' shadows: a mirrored sprite one cell high is drawn again upside down */
			if (mirror_y && height == 1)
			{
				if (no_zoom)
					draw_cell(cell, colour, fx, !fy, sx, sy, mask, modes);
				else
					draw_cell_zoomed(cell, colour, fx, !fy, sx, sy, (uint32_t)zw << 12, (uint32_t)zh << 12, mask, modes);
			}
		}
	}
}

/* k053247_draw_single_sprite_gxcore, the non-GX path: the sprite at word 'offset' of the sprite RAM */
static void draw_sprite(int code, int offset, int colour, uint32_t mask, int shadow, uint8_t *modes)
{
	const uint16_t *sprite = &g_sprite_ram[offset];
	const bool flip_screen_x = (g_sprite_registers[5] & 0x01) != 0;
	const bool flip_screen_y = (g_sprite_registers[5] & 0x02) != 0;
	const int attributes = sprite[0];
	int xa = 0, ya = 0, ox, oy, zoom_x, zoom_y, scale_x, scale_y, width, height, value;
	bool flip_x, flip_y, mirror_x, mirror_y, no_zoom;

	if (code & 0x01) xa += 1;
	if (code & 0x02) ya += 1;
	if (code & 0x04) xa += 2;
	if (code & 0x08) ya += 2;
	if (code & 0x10) xa += 4;
	if (code & 0x20) ya += 4;
	code &= ~0x3F;

	oy = sprite[2] & 0x3FF;
	ox = sprite[3] & 0x3FF;
	scale_y = zoom_y = sprite[4] & 0x3FF;
	zoom_y = zoom_y ? (0x400000 + (zoom_y >> 1)) / zoom_y : 0x800000;
	if (!(attributes & 0x4000))
	{
		scale_x = zoom_x = sprite[5] & 0x3FF;
		zoom_x = zoom_x ? (0x400000 + (zoom_x >> 1)) / zoom_x : 0x800000;
	}
	else
	{
		zoom_x = zoom_y;
		scale_x = scale_y;
	}
	no_zoom = scale_x == 0x40 && scale_y == 0x40;
	flip_x = (attributes & 0x1000) != 0;
	flip_y = (attributes & 0x2000) != 0;
	mirror_x = (sprite[6] & 0x4000) != 0;
	if (mirror_x)
		flip_x = false;
	mirror_y = (sprite[6] & 0x8000) != 0;

	/* (for Escape Kids: OBJSET1 bit 3 halves the sprites' width) */
	if (g_sprite_registers[5] & 0x08)
	{
		zoom_x >>= 1;
		ox = (ox >> 1) + 1;
		if (flip_screen_x)
			ox += SCREEN_WIDTH;
		no_zoom = false;
	}
	if (flip_screen_x)
	{
		ox = -ox;
		if (!mirror_x)
			flip_x = !flip_x;
	}
	if (flip_screen_y)
	{
		oy = -oy;
		if (!mirror_y)
			flip_y = !flip_y;
	}

	/* the display window's offsets and the wrap (the K053247's OPSET, never written here, is 0: 1024 wide) */
	const int window_x = (int16_t)((g_sprite_registers[0] << 8) | g_sprite_registers[1]);
	const int window_y = (int16_t)((g_sprite_registers[2] << 8) | g_sprite_registers[3]);

	ox = (ox - window_x) & 1023;
	oy = (-oy - window_y) & 1023;
	if (ox >= 1024 - 384)
		ox -= 1024;
	if (oy >= 1024 - 512)
		oy -= 1024;
	value = (attributes >> 8) & 0x0F;
	width = 1 << (value & 3);
	height = 1 << ((value >> 2) & 3);
	/* simpsons.cpp's set_config: the sprites' offsets */
	ox += -43;
	oy -= 23;
	/* the coordinates are the sprite's centre */
	ox -= (zoom_x * width) >> 13;
	oy -= (zoom_y * height) >> 13;

	shadow = (shadow >> 10) & 3;
	modes[15] = shadow ? DRAW_SHADOW : DRAW_SOURCE;
	draw_cells(code, colour, height, width, zoom_x, zoom_y, flip_x, flip_y, ox, oy, xa, ya, mirror_x, mirror_y, no_zoom,
		mask, modes);
}

/* k053247_sprites_draw: the sprites, sorted by their z code (the K053247's OPSET PRI clear: the biggest first),
   drawn from the end of the list (the nearest first) */
static void draw_sprites(void)
{
	int sorted[256], count = 0;
	uint8_t modes[16];

	memset(modes, DRAW_SOURCE, sizeof(modes));
	modes[0] = DRAW_NONE;
	for (int offset = 0; offset < 0x800; offset += 8)
		if (g_sprite_ram[offset] & 0x8000)
			sorted[count++] = offset;

	const int w = count;
	count--;
	const int h = count;

	for (int y = 0; y < h; y++)
	{
		int offset = sorted[y];
		int z = g_sprite_ram[offset] & 0xFF;

		for (int x = y + 1; x < w; x++)
		{
			const int other = sorted[x];
			const int code = g_sprite_ram[other] & 0xFF;

			if (z <= code)
			{
				z = code;
				sorted[x] = offset;
				sorted[y] = offset = other;
			}
		}
	}

	for (; count >= 0; count--)
	{
		const int offset = sorted[count];
		const int code = g_sprite_ram[offset + 1];
		const int attributes = g_sprite_ram[offset + 6];
		/* simpsons.cpp's sprite callback: the priority among the layers and the colour */
		const int priority = (attributes & 0x0F80) >> 6;
		uint32_t mask;

		if (priority <= g_layer_priority[2])
			mask = 0;
		else if (priority <= g_layer_priority[1])
			mask = 0xF0;
		else if (priority <= g_layer_priority[0])
			mask = 0xF0 | 0xCC;
		else
			mask = 0xF0 | 0xCC | 0xAA;
		draw_sprite(code, offset, g_sprite_colour_base + (attributes & 0x001F), mask, attributes, modes);
	}
}

/* The palette RAM's colours (2048, xBGR 555, big endian), and their shaded and highlighted pens */
static uint8_t pal5bit(int value)
{
	value &= 0x1F;
	return (uint8_t)((value << 3) | (value >> 2));
}

static uint8_t adjust(uint8_t component, float contrast)
{
	const int value = (int)((float)component * contrast + 0.0f);

	return (uint8_t)(value < 0 ? 0 : value > 255 ? 255 : value);
}

/* A palette colour's pen, its shaded one and its highlighted one, into 'pens' (g_pens' layout) */
static void put_pens(uint32_t *pens, int entry, int raw)
{
	const float highlight = 1.0f / SHADOW_FACTOR;
	const uint8_t red = pal5bit(raw), green = pal5bit(raw >> 5), blue = pal5bit(raw >> 10);

	pens[entry] = (uint32_t)red << 16 | (uint32_t)green << 8 | blue;
	pens[COLOURS + entry] = (uint32_t)adjust(red, SHADOW_FACTOR) << 16 | (uint32_t)adjust(green, SHADOW_FACTOR) << 8
		| adjust(blue, SHADOW_FACTOR);
	pens[2 * COLOURS + entry] = (uint32_t)adjust(red, highlight) << 16 | (uint32_t)adjust(green, highlight) << 8
		| adjust(blue, highlight);
}

static uint16_t palette_colour(int entry)
{
	return (uint16_t)(simpsons_board.palette[entry * 2] << 8 | simpsons_board.palette[entry * 2 + 1]);
}

/* The pens of the palette RAM's colours that changed since they were last made (g_pens is a function of the RAM's
   colour alone; a port's recoloured and shade pens, of it and of the recolouring: all of them again once it is set) */
static void load_palette(void)
{
	for (int entry = 0; entry < COLOURS; entry++)
	{
		const uint16_t raw = palette_colour(entry);

		if (raw != g_palette_raw[entry])
		{
			g_palette_raw[entry] = raw;
			put_pens(g_pens, entry, raw);
		}
		/* (a port's recolouring: the pen's colour recoloured) */
		if (g_recolour && (g_recolour_set || raw != g_recoloured_raw[entry]))
		{
			g_recoloured_raw[entry] = raw;
			put_pens(g_recoloured_pens, entry, g_recolour((uint16_t)(raw & 0x7FFF)));
		}
	}
	/* (and the shade's, the same for a colour code's sixteen pens: from the code's shade pen) */
	for (int code = 0; g_recolour && g_recolouring.shade && code < COLOUR_CODES; code++)
	{
		const uint16_t raw = palette_colour(code * 16 + (g_recolouring.shade_pen & 15));

		if (g_recolour_set || raw != g_shade_raw[code])
		{
			const uint16_t shade = g_recolouring.shade((uint16_t)(raw & 0x7FFF));

			g_shade_raw[code] = raw;
			for (int pen = 0; pen < 16; pen++)
				put_pens(g_shade_pens, code * 16 + pen, shade);
		}
	}
	if (g_recolour)
		g_recolour_set = false;
}

/* A bitmap pixel's colour: its pen's, or its pen recoloured or in the shade (a port's) */
static uint32_t pixel_colour(uint16_t value)
{
	if (value & SHADE)
		return g_shade_pens[(value & PEN_PART) % (3 * COLOURS)];
	if (value & RECOLOURED)
		return g_recoloured_pens[(value & PEN_PART) % (3 * COLOURS)];
	return g_pens[value % (3 * COLOURS)];
}

/* A stretch of a line of the bitmap as colours (nearly every pixel is a pen and nothing else) */
static void line_colours(uint32_t *colours, const uint16_t *pixels, int count)
{
	int x = 0;

	for (; x + 4 <= count; x += 4)
		if ((pixels[x] | pixels[x + 1] | pixels[x + 2] | pixels[x + 3]) < 2 * COLOURS)
		{
			/* (four pens of the palette's, shaded or not) */
			colours[x] = g_pens[pixels[x]];
			colours[x + 1] = g_pens[pixels[x + 1]];
			colours[x + 2] = g_pens[pixels[x + 2]];
			colours[x + 3] = g_pens[pixels[x + 3]];
		}
		else
			for (int index = x; index < x + 4; index++)
				colours[index] = pixel_colour(pixels[index]);
	for (; x < count; x++)
		colours[x] = pixel_colour(pixels[x]);
}
_Static_assert((2 * COLOURS & (2 * COLOURS - 1)) == 0, "pens below a power of two are so together too");

/* simpsons.cpp's screen_update */
static void draw(simpsons_frame *frame)
{
	/* the K053251's colour bases (registers 9 and 10) and its priorities of CI2-CI4 (the fixed layer, A, B) */
	const uint8_t index_bases = simpsons_board.priority[9] & 0x3F, layer_bases = simpsons_board.priority[10] & 0x3F;
	const int background = 32 * (index_bases & 0x03);
	const int colour_bases[3] = { 32 * ((index_bases >> 4) & 0x03), 16 * (layer_bases & 0x07), 16 * ((layer_bases >> 3) & 0x07) };
	int order[3] = { 0, 1, 2 };

	g_sprite_colour_base = 32 * ((index_bases >> 2) & 0x03);
	for (int map = 0; map < 3; map++)
	{
		if (g_layer_colour_base[map] != colour_bases[map])
			mark_all(map);
		g_layer_colour_base[map] = colour_bases[map];
		g_layer_priority[map] = simpsons_board.priority[2 + map] & 0x3F;
	}
	/* konami_sortlayers3: the lowest priority (the biggest number) first */
	static const int pairs[3][2] = { { 0, 1 }, { 0, 2 }, { 1, 2 } };
	for (int pair = 0; pair < 3; pair++)
	{
		const int a = pairs[pair][0], b = pairs[pair][1];

		if (g_layer_priority[a] < g_layer_priority[b])
		{
			int swap = g_layer_priority[a];
			g_layer_priority[a] = g_layer_priority[b];
			g_layer_priority[b] = swap;
			swap = order[a];
			order[a] = order[b];
			order[b] = swap;
		}
	}

	const int columns = SIMPSONS_VIDEO_WIDTH + g_left + g_right;    /* (WIDE's sides too) */

	for (int y = CLIP_TOP; columns > 0 && y <= CLIP_BOTTOM; y++)
	{
		uint16_t *line = &g_bitmap[AT(CLIP_LEFT - g_left, y)];

		for (int x = 0; x < columns; x++)
			line[x] = (uint16_t)(16 * background);
		memset(&g_priority[AT(CLIP_LEFT - g_left, y)], 0, (size_t)columns);
	}
	draw_layer(order[0], 1);
	draw_layer(order[1], 2);
	draw_layer(order[2], 4);
	draw_sprites();

	load_palette();
	for (int y = 0; y < SIMPSONS_VIDEO_HEIGHT; y++)
		line_colours(&frame->pixels[y * SIMPSONS_VIDEO_WIDTH], &g_bitmap[AT(0, CLIP_TOP + y)], SIMPSONS_VIDEO_WIDTH);
	/* WIDE's picture: the same with its sides (its columns that are the board's, from 'board' to before 'end', as the
	   frame has them) */
	g_wide_width = SIMPSONS_VIDEO_WIDTH + g_left + g_right;
	g_wide_left = g_left;
	if (g_left || g_right)
	{
		const int board = g_left > 0 ? g_left : 0;
		const int most = g_left + SIMPSONS_VIDEO_WIDTH < g_wide_width ? g_left + SIMPSONS_VIDEO_WIDTH : g_wide_width;
		const int end = most > board ? most : board;

		for (int y = 0; y < SIMPSONS_VIDEO_HEIGHT; y++)
		{
			uint32_t *wide = &g_wide[y * SIMPSONS_VIDEO_MAX_WIDTH];
			const uint16_t *line = &g_bitmap[AT(-g_left, CLIP_TOP + y)];

			line_colours(wide, line, board);
			memcpy(&wide[board], &frame->pixels[y * SIMPSONS_VIDEO_WIDTH + board - g_left],
				(size_t)(end - board) * sizeof(uint32_t));
			line_colours(&wide[end], &line[end], g_wide_width - end);
		}
	}
}

/* WIDE's settings for the frame about to be drawn (only with a wider picture: the board's own shows its maps whole) */
static void take_wide(void)
{
	const bool wide = g_left_next || g_right_next;

	g_left = g_left_next;
	g_right = g_right_next;
	g_fixed_dx = wide ? g_fixed_dx_next : 0;
	g_fixed_first = wide ? g_fixed_first_next : -0x10000;
	g_fixed_last = wide ? g_fixed_last_next : 0x10000;
	for (int map = 0; map < 3; map++)
	{
		g_first[map] = wide ? g_first_next[map] : -0x10000;
		g_last[map] = wide ? g_last_next[map] : 0x10000;
	}
}

void simpsons_video_vblank_end(simpsons_frame *frame)
{
	load_chips();
	update_scroll();
	take_wide();
	if (frame)
		draw(frame);
}

void simpsons_video_set_wide(int left, int right)
{
	const int least = -SIMPSONS_VIDEO_MAX_LESS, most = SIMPSONS_VIDEO_MAX_MORE + SIMPSONS_VIDEO_MAX_LESS;

	g_left_next = left < least ? least : (left > most ? most : left);
	g_right_next = right < least ? least : right;
	if (g_left_next + g_right_next > SIMPSONS_VIDEO_MAX_MORE)
		g_right_next = SIMPSONS_VIDEO_MAX_MORE - g_left_next;
	if (g_left_next + g_right_next < 0)
		g_left_next = g_right_next = 0;    /* (never narrower than the board's) */
}

const uint32_t *simpsons_video_wide_frame(int *width)
{
	*width = g_wide_width;
	return g_wide;
}

int simpsons_video_wide_left(void)
{
	return g_wide_width != SIMPSONS_VIDEO_WIDTH || g_wide_left ? g_wide_left : 0;
}

void simpsons_video_set_layer_window(int map, int first, int last)
{
	if (map >= 1 && map <= 2)
	{
		g_first_next[map] = first;
		g_last_next[map] = last;
	}
}

void simpsons_video_set_fixed(int dx, int first, int last)
{
	g_fixed_dx_next = dx;
	g_fixed_first_next = first;
	g_fixed_last_next = last;
}

void simpsons_video_set_recolour(const simpsons_video_recolouring *recolouring)
{
	if (recolouring && recolouring->colour)
		g_recolouring = *recolouring;
	else
		memset(&g_recolouring, 0, sizeof(g_recolouring));
	g_recolour = g_recolouring.colour;
	g_recolour_set = true;
}

void simpsons_video_vblank_start(void)
{
	load_chips();
}

uint64_t simpsons_video_hash(const simpsons_frame *frame)
{
	uint64_t hash = 0xcbf29ce484222325ull;

	for (int index = 0; index < SIMPSONS_VIDEO_WIDTH * SIMPSONS_VIDEO_HEIGHT; index++)
		for (int shift = 16; shift >= 0; shift -= 8)
			hash = (hash ^ ((frame->pixels[index] >> shift) & 0xFFu)) * 0x100000001b3ull;
	return hash;
}

/* A save state's part (simpsons/state.h): the chips as the video last took them from the board and what it keeps
   from frame to frame, the tile maps' caches with their marks (as MAME's tilemaps, a tile is read again only when
   marked: a cache is no function of the RAM alone), the colour bases they were drawn with, and WIDE's settings, the
   frame's and the next one's; and the board's 320 columns of the bitmap, which a frame of WIDE drawn with a side less
   than none (-g_right columns of the board's past a stop of the camera's) leaves as an earlier frame drew them, and the
   board's frame still takes. What each frame draws again (the rest of the bitmap, the priorities, the pens, the
   recolouring's cells, the frames drawn) is not kept */
void simpsons_video_state(simpsons_state *state)
{
	simpsons_state_part(state, "video");
	simpsons_state_bytes(state, g_tile_ram, sizeof(g_tile_ram));
	simpsons_state_bytes(state, &g_address_map, sizeof(g_address_map));
	simpsons_state_bytes(state, &g_scroll_control, sizeof(g_scroll_control));
	simpsons_state_bytes(state, &g_tile_flip, sizeof(g_tile_flip));
	simpsons_state_bytes(state, g_char_rom_bank, sizeof(g_char_rom_bank));
	simpsons_state_bytes(state, g_layers, sizeof(g_layers));
	simpsons_state_bytes(state, g_layer_colour_base, sizeof(g_layer_colour_base));
	simpsons_state_bytes(state, &g_sprite_colour_base, sizeof(g_sprite_colour_base));
	simpsons_state_bytes(state, g_layer_priority, sizeof(g_layer_priority));
	simpsons_state_bytes(state, g_sprite_ram, sizeof(g_sprite_ram));
	simpsons_state_bytes(state, g_sprite_registers, sizeof(g_sprite_registers));
	simpsons_state_part(state, "video's wide");
	simpsons_state_bytes(state, &g_left, sizeof(g_left));
	simpsons_state_bytes(state, &g_right, sizeof(g_right));
	simpsons_state_bytes(state, &g_left_next, sizeof(g_left_next));
	simpsons_state_bytes(state, &g_right_next, sizeof(g_right_next));
	simpsons_state_bytes(state, g_first, sizeof(g_first));
	simpsons_state_bytes(state, g_last, sizeof(g_last));
	simpsons_state_bytes(state, g_first_next, sizeof(g_first_next));
	simpsons_state_bytes(state, g_last_next, sizeof(g_last_next));
	simpsons_state_bytes(state, &g_fixed_dx, sizeof(g_fixed_dx));
	simpsons_state_bytes(state, &g_fixed_first, sizeof(g_fixed_first));
	simpsons_state_bytes(state, &g_fixed_last, sizeof(g_fixed_last));
	simpsons_state_bytes(state, &g_fixed_dx_next, sizeof(g_fixed_dx_next));
	simpsons_state_bytes(state, &g_fixed_first_next, sizeof(g_fixed_first_next));
	simpsons_state_bytes(state, &g_fixed_last_next, sizeof(g_fixed_last_next));
	for (int y = CLIP_TOP; y <= CLIP_BOTTOM; y++)
		simpsons_state_bytes(state, &g_bitmap[AT(0, y)], SIMPSONS_VIDEO_WIDTH * sizeof(g_bitmap[0]));
}
