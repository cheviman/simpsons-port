/**
 *    Name:        wide.c
 *    Description: the console's WIDE (OPTIONS > VIDEO): the picture wider than the board's 320 pixels in a stage's
 *                 play and on every other screen, the layers' columns the game does not draw that far drawn from the
 *                 same maps, the players' boxes kept in the middle, and the objects coming in, going out and walking
 *                 past the wider screen's sides
 *    Version:     0.2.0
 *    Created:     13-11-2026
 *    Modified:    14-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    As tmnt-port's WIDE (the same K052109 and screen): 16:10 32 columns more at each side, 16:9 48, 20:9 88 (the
 *    K052109's maps are rings of 512 pixels: 496 of them shown in 20:9). The video draws the layers' maps and the
 *    sprites there (simpsons_video_set_wide, from console_wide_frame at each frame's end); the console draws its own
 *    picture over the whole of it (console_picture).
 *
 *    The cabinet showed 288 of its 320 columns: the fixed layer covers the first 16 and the last 16 with black tiles
 *    (tile 3F, its ring's columns 12, 13, 50 and 51). With WIDE the fixed layer is drawn without them and without its
 *    ring's other columns (tile 00 there, a pattern of circles), the board's 320 columns of layers A and B shown whole.
 *
 *    In a stage's play (a stage step, its maps drawn: draw_layer_columns) the sides show the layers' maps: the game
 *    draws a map's column as its scroll brings it 352 pixels past the screen's left edge (camera_scroll_layers'
 *    offset 0160), and a whole layer 352 pixels from it (its 44 columns: a stage's start); the console draws the
 *    columns the wider screen shows that the layers' rings do not hold (draw_side_columns), from the same map and its
 *    blocks: their tiles, and the floor map's cells for the layer the game takes them from (the play's walls and
 *    floors, so the players and the enemies in the sides walk them as on the board: the layer the game last drew whole
 *    with them, as a stage's start draws its layer B while its camera is on layer A for its opening scene, or the
 *    camera's once that changes), and that layer's past the right side as far as the ring has room (DRAWN_AHEAD, as the
 *    board's floor is valid past its own right edge), never past the map's ends. Where a side has no
 *    map past the board's screen (a stage's start at its left, its end at its right), or no more of it than the
 *    cabinet ever shows (past where the stage's record stops the camera for its boss: stage 1's map goes on in a wall
 *    of blue bricks), the picture shows what there is and the rest at the other side, back to the middle as the scroll
 *    brings the map, moving by WIDE_PAN pixels a frame (tmnt-port's, Sebas: "correr todo para la derecha"); the
 *    players' boxes (the fixed layer) kept in the middle of the picture. The continue's countdown and GAME OVER over
 *    the play keep its sides. The other screens (the menus' TV, the portraits, the stage cards, the stories, the bonus
 *    stages, the name entry, the ending, and stage 3's climb, whose map is black past the cabinet's 288 columns) are
 *    pictures of the board's width: WIDE's sides there are their edges' colour (the TV's orange), or black where
 *    their edges are not of one colour; the presentation keeps the board's 320.
 *
 *    The objects (the console's versions of the game's functions, console/replaced.txt): a stage's spawns come in at
 *    their place past the wider screen's right side (spawn_stage_objects: due when the camera is that much closer;
 *    those that come from the left, past its left side), and an object goes out, stops, counts as on the screen and
 *    a player walks to the screen's edges as on the board, but by the wider screen's sides (enemy_remove_off_screen,
 *    remove_off_screen, stop_beyond_screen_edge, enemy_on_screen, player_check_motion, player_check_screen_edges:
 *    the same C with their limits moved), and the enemies' moves 9 and 3 stop them past the wider screen's sides, not
 *    in them (enemy_wait_while_target_down, enemy_back_off: 16 pixels past its sight, as the board's 40 and 17F are
 *    past the cabinet's black edges). A spawn list read at once (a stage's or an area's start, the play's first frame
 *    too) puts in where the board does what the cabinet shows (a scene's), and the goons that come in by themselves
 *    from where the cabinet did not show them (under its black edges or past them: stage 1's first goon, waiting for
 *    its opening scene to end, a player's report) as far past the wider screen's edges as they were past the
 *    cabinet's. An object's x is the screen's: 40 at the board's left edge, 17F at its right.
 *
 *    For development, SIMPSONS_WIDE_DEBUG in the environment writes each frame's sides, the game's step, the layers'
 *    scrolls and rings, the stage record's entry and the objects in use to the standard error; --wide-check (the
 *    executables') says each frame which side columns are not their map's and what the spawn list put in that the
 *    wider screen showed and the cabinet did not.
 */
#include "console_internal.h"

#include "game.h"
#include "simpsons/machine.h"
#include "simpsons/video.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LEAST(a, b)         ((a) < (b) ? (a) : (b))
#define MOST(a, b)          ((a) > (b) ? (a) : (b))

static const int k_sides[WIDE_AUTO] = { 0, 48, 88, 32 };
static const int k_views[WIDE_AUTO] = { SIMPSONS_VIDEO_WIDTH, 398, 496, 358 };    /* (the window's middle of them:
                                                                                     the screen's height times 16 / 9,
                                                                                     20 / 9, 16 / 10) */

#define RING                512          /* a layer's map in the K052109: a ring of 512 x 256 pixels */
#define TILE                8
#define BLOCK               32           /* a map's block: 4 x 4 tiles, 18 bytes (colour, floor pattern, 16 tiles) */
#define BLOCK_BYTES         18u
#define STRIP               256          /* a map's strip: 8 blocks across, rows of 16 bytes down it */
#define STRIPS_MOST         64           /* (the strip's index is a signed byte of twice it) */
#define GAME_AHEAD          0x160        /* the game's new column, past the screen's left edge (camera_scroll_layers) */
#define ROW_BLOCKS          12           /* the game's new row: 12 blocks from the screen's first (draw_map_row) */
#define FLOOR_MAP           0x4000u      /* the floor map: 64 cells of 8 pixels a row, 32 rows */
#define FLOOR_MAP_END       0x4800u
#define BLOCK_FLAGS         0xF67Cu      /* block_tile_flags: a floor pattern's 16 cells */
#define BANK_WINDOW_END     0x8000u
#define FIXED_PROGRAM       0x78000u     /* the fixed program's place in the program ROM (8000-FFFF) */
#define BANK_BYTES          0x2000u
#define STAGE_RECORDS_BANK  0x13u        /* the stages' records (stage_slots[n].record, stage_record_entry) */
#define WIDE_PAN            4            /* the sides' move a frame toward where they come to be */
#define EDGE                16           /* the cabinet's black edges, the fixed layer's: the board's first and last 16 */
#define MASK_TILE           0x3Fu
#define OBJECT_LEFT         0x40         /* an object's x at the board's left edge */
#define DRAWN_AHEAD         0x28         /* the floor drawn past the wider screen's right side, as far as the board's is
                                            past its own (its new column 0160 past the screen's left edge: 0x1A0-0x1A7) */

/* The game's steps that are a stage's play (4803: 2 stage 1, 4 stage 2, 6-7 stage 3, 8 stage 4, 9 stage 5, 11 stage
   6, 13 stage 7, 15 stage 8; the others: the stories, the two bonus stages, the ending) */
static bool stage_step(unsigned step)
{
	return step == 2u || step == 4u || step == 6u || step == 7u || step == 8u || step == 9u || step == 11u
		|| step == 13u || step == 15u;
}

#define PHASE_STEPS         1u           /* mode_phase: the game's steps (game_run_step), */
#define PHASE_CONTINUE      2u           /* the continue's countdown over the play, */
#define PHASE_GAME_OVER     3u           /* GAME OVER over it (then the name entry: a screen of its own) */

/* ---- the program's data, through a bank ---- */

static const uint8_t *rom(unsigned bank, uint16_t address)
{
	uint32_t place;

	if (address >= BANK_WINDOW_END)
		place = FIXED_PROGRAM + (address - BANK_WINDOW_END);
	else if (address >= 0x6000u)
		place = bank * BANK_BYTES + (uint32_t)(address - 0x6000u);
	else
		return NULL;
	return simpsons_board.program_at ? simpsons_board.program_at(place) : NULL;
}

static int rom8(unsigned bank, uint16_t address)
{
	const uint8_t *byte = rom(bank, address);

	return byte ? *byte : -1;
}

static int rom16(unsigned bank, uint16_t address)
{
	const uint8_t *high = rom(bank, address), *low = rom(bank, (uint16_t)(address + 1u));

	return high && low ? (*high << 8 | *low) : -1;
}

/* ---- the layers' maps ---- */

/* A tile layer's record (layer_maps: 0 A, 1 B): its tiles' place in the K052109 and its variables' addresses */
typedef struct
{
	uint16_t tiles, scroll_x, scroll_y, offset_y2, maps, maps_high;
} layer_record;

static layer_record record_of(int layer)
{
	const struct layer_map *map = LAYER_MAP(layer_maps[layer]);
	layer_record record;

	record.tiles = map->tiles;
	record.scroll_x = map->scroll_x;
	record.scroll_y = map->scroll_y;
	record.offset_y2 = map->offset_y2;
	record.maps = map->maps;
	record.maps_high = map->maps_high;
	return record;
}

/* The map of the scene and area the game draws (map_scene, map_area) for a layer: its strips and blocks */
typedef struct
{
	bool known;
	uint8_t bank, block_bank;
	uint16_t strips, blocks;
	int width;              /* its pixels across: its strips' */
} layer_map_area;

static bool scene_area(uint16_t lists, uint16_t lists_high, uint8_t *bank, uint16_t *address)
{
	const uint8_t scene_byte = map_scene;
	const uint16_t list = (int8_t)scene_byte < 0 ? lists_high : lists;
	const int area_list = rom16(0, (uint16_t)(list + (int8_t)(uint8_t)((scene_byte & 0x7Fu) << 1)));
	int area_bank, area_address;
	uint16_t area;

	if (area_list < 0)
		return false;
	area = (uint16_t)(area_list + (int8_t)(uint8_t)(map_area << 2));
	area_address = rom16(0, area);
	area_bank = rom8(0, (uint16_t)(area + 2u));
	if (area_address < 0 || area_bank < 0)
		return false;
	*bank = (uint8_t)area_bank;
	*address = (uint16_t)area_address;
	return true;
}

static layer_map_area map_of(int layer)
{
	const layer_record record = record_of(layer);
	layer_map_area map = { false, 0, 0, 0, 0, 0 };
	int previous = -1;

	if (!scene_area(record.maps, record.maps_high, &map.bank, &map.strips)
		|| !scene_area(layer ? block_scenes_b_ADDRESS : block_scenes_a_ADDRESS,
			layer ? block_scenes_b_high_ADDRESS : block_scenes_a_high_ADDRESS, &map.block_bank, &map.blocks))
		return map;
	/* (its strips' table: their places in the bank's window, one after the other; past the last a word of 0000) */
	for (int strip = 0; strip < STRIPS_MOST; strip++)
	{
		const int place = rom16(map.bank, (uint16_t)(map.strips + 2 * strip));

		if (place < 0x6000 || place >= (int)BANK_WINDOW_END || place <= previous)
			break;
		previous = place;
		map.width = (strip + 1) * STRIP;
	}
	map.known = map.width > 0;
	return map;
}

/* Where the drawing goes: the K052109's RAM (colour at the tile's place less 2000, its code there), and the floor
   map's cells; or, for --wide-check, the cells that differ counted */
static int checking = -1;
static int checked;

static void put_tile(uint16_t place, uint8_t colour, uint8_t code)
{
	if (place >= 0x4000u || place < 0x2000u)
		return;
	if (checking < 0)
	{
		simpsons_board.tile_ram[place - 0x2000u] = colour;
		simpsons_board.tile_ram[place] = code;
	}
	else
	{
		checked++;
		checking += simpsons_board.tile_ram[place] != code || simpsons_board.tile_ram[place - 0x2000u] != colour;
	}
}

static void put_floor(uint16_t place, uint8_t value)
{
	if (checking < 0 && place >= FLOOR_MAP && place < FLOOR_MAP_END)
		RAM8(place) = value;
}

/* The map's column at map pixel 'x' (a multiple of 8) of a layer, as draw_map_column draws it at the layer's
   scroll y 'scroll_y' (the ring's: layer_ring's y): 8 blocks down from the block row of that y (with its second y
   offset), the ring's 32 tile rows; and
   the floor map's column (fill_floor_map_column) when it is the camera's layer. false where the map's data ends */
static bool draw_column(int layer, const layer_map_area *map, int x, bool floor, uint16_t scroll_y)
{
	const layer_record record = record_of(layer);
	const uint16_t edge_y = (uint16_t)(scroll_y + mem_read16(record.offset_y2));
	const int strip = rom16(map->bank, (uint16_t)(map->strips + (int8_t)(uint8_t)((x >> 8) << 1)));
	const unsigned column = ((unsigned)x >> 3) & 3u;
	uint16_t cell, tile_place, floor_place, scroll = scroll_y;

	if (strip < 0 || x < 0 || x >= map->width)
		return false;
	cell = (uint16_t)(strip + ((x >> 4) & 0xE) + (uint16_t)(((int16_t)edge_y >> 1) & ~0xF));
	tile_place = (uint16_t)(record.tiles + 0x40u * ((scroll_y & 0xE0u) >> 3) + (((unsigned)x >> 3) & 0x3Fu));
	floor_place = (uint16_t)(FLOOR_MAP + (((unsigned)x >> 3) & 0x3Fu));
	for (int block = 0; block < 8; block++)
	{
		const int word = rom16(map->bank, cell);
		uint16_t at;
		int colour;
		uint16_t next;

		if (word < 0)
			return false;
		at = (uint16_t)(map->blocks + BLOCK_BYTES * (uint16_t)((((unsigned)word >> 8) & 3u) << 8 | ((unsigned)word & 0xFFu)));
		colour = rom8(map->block_bank, at);
		for (unsigned row = 0; row < 4u; row++)
		{
			const int tile = rom8(map->block_bank, (uint16_t)(at + 2u + column + 4u * row));

			if (colour < 0 || tile < 0)
				return false;
			put_tile((uint16_t)(tile_place + 0x40u * row), (uint8_t)colour, (uint8_t)tile);
		}
		tile_place = (uint16_t)(tile_place + 0x100u);
		if (floor)
		{
			const uint16_t floor_block = (uint16_t)(map->blocks + BLOCK_BYTES * (uint16_t)word);
			const int pattern = rom8(map->block_bank, (uint16_t)(floor_block + 1u));

			for (unsigned row = 0; row < 4u && pattern >= 0; row++)
			{
				const int value = rom8(0, (uint16_t)(BLOCK_FLAGS + (unsigned)pattern * 16u + column + 4u * row));

				if (value >= 0)
					put_floor((uint16_t)(floor_place + 0x40u * row), (uint8_t)value);
			}
			floor_place = (uint16_t)(floor_place + 0x100u);
		}
		next = (uint16_t)(scroll + 0x20u);
		if ((next ^ scroll) & 0x100u)
		{
			tile_place = (uint16_t)(tile_place - 0x800u);
			floor_place = (uint16_t)(floor_place - 0x800u);
		}
		scroll = next;
		cell = (uint16_t)(cell + 0x10u);
	}
	return true;
}

/* ---- the columns each layer's ring holds ----

   Map pixels, from 'first' to 'last' (a column's first to a column's last): what the game drew whole (draw_layer_columns:
   its columns from its scroll x and offset), what the board's screen shows (the game's scroll draws each column before
   it comes in), what the console has drawn; at most the ring's 512, at the layer's scroll y they were drawn at (a row the
   game draws as its scroll y goes spans its 12 blocks: the columns past them are drawn again) */
typedef struct
{
	bool known;             /* a map in it: the game drew it whole since the step began */
	int first, last;
	int x;                  /* (its scroll x at the last frame's end) */
	uint16_t y;
	uint8_t drawn_scene, drawn_area;
} layer_ring;

static layer_ring rings[2];

/* The fixed layer's columns drawn (the board's 16 to 303 under its black edges: edges_masked), and so the cabinet's */
static int fixed_first = 0, fixed_last = SIMPSONS_VIDEO_WIDTH - 1;

static int layer_scroll_x(int layer)
{
	return (int)(uint16_t)mem_read16(record_of(layer).scroll_x);
}

static int layer_scroll_y(int layer)
{
	return (int)(uint16_t)mem_read16(record_of(layer).scroll_y);
}

/* The game's whole layer (video/background_maps.c): what its ring holds from then */
static void whole_layer_drawn(int layer, int x, int columns)
{
	layer_ring *ring = &rings[layer & 1];

	ring->known = true;
	ring->first = x & ~(TILE - 1);
	ring->last = ring->first + TILE * columns - 1;
	ring->x = layer_scroll_x(layer & 1);
	ring->y = (uint16_t)layer_scroll_y(layer & 1);
	ring->drawn_scene = map_scene;
	ring->drawn_area = map_area;
}

void draw_layer_columns(uint16_t layer_and_columns, uint16_t *p_layer_map, uint16_t *p_tile_place)
{
	const int layer = (layer_and_columns >> 8) & 1;
	const int columns = (layer_and_columns & 0xFFu) ? (int)(layer_and_columns & 0xFFu) : 256;
	const uint16_t offset_x = (uint16_t)mem_read16(LAYER_MAP(layer_maps[layer])->offset_x);
	const int x = (uint16_t)(layer_scroll_x(layer) + offset_x);

	original_draw_layer_columns(layer_and_columns, p_layer_map, p_tile_place);
	whole_layer_drawn(layer, x, columns);
}

/* The layer the floor map's cells are of: the one the game last drew whole with them (a stage's start: its layer B,
   while its camera is on layer A for its opening scene, stage 1's), or the camera's once that changes (the game fills
   them from the camera's layer as its scroll goes: draw_floor_map_column); -1, the camera's */
static int floor_layer = -1;
static int floor_camera_seen = -1;    /* (camera_main_layer as last seen) */

static int floor_layer_now(void)
{
	return floor_layer >= 0 ? floor_layer : (camera_main_layer ? 1 : 0);
}

uint16_t draw_layer_columns_with_floor_map(uint16_t layer_and_columns)
{
	const int layer = (layer_and_columns >> 8) & 1;
	const int columns = (layer_and_columns & 0xFFu) ? (int)(layer_and_columns & 0xFFu) : 256;
	const int x = layer_scroll_x(layer);    /* (its offset set to 0 first) */
	const uint16_t u = original_draw_layer_columns_with_floor_map(layer_and_columns);

	whole_layer_drawn(layer, x, columns);
	floor_layer = layer;
	return u;
}

/* Whether the game draws a layer's new columns and rows as it scrolls (camera_update: the stage record's flags) */
static bool edges_on(int layer)
{
	return (layer ? layer_b_edges_on : layer_a_edges_on) != 0u;
}

/* The ring's columns kept as the game's scroll goes (at each frame's end, the game's frame done) */
static void follow_ring(int layer)
{
	layer_ring *ring = &rings[layer];
	const int x = layer_scroll_x(layer);
	const uint16_t y = (uint16_t)layer_scroll_y(layer);
	/* (the board's columns the cabinet showed: those under its black edges the game may leave as they were) */
	const int board_first = (x + fixed_first) & ~(TILE - 1), board_last = x + SIMPSONS_VIDEO_WIDTH - 1 - fixed_first;

	if (ring->drawn_scene != map_scene || ring->drawn_area != map_area)
	{
		/* (another map's edges from now on: the board's columns alone are of it) */
		ring->first = board_first;
		ring->last = board_last | (TILE - 1);
		ring->drawn_scene = map_scene;
		ring->drawn_area = map_area;
	}
	if (edges_on(layer) && layer_rows_on)
	{
		/* (the game draws its rows as its scroll y goes: 12 blocks from the screen's first. Where it does not, the
		   ring keeps the rows drawn at its y, round as the scroll goes, a map no taller than it: stage 6's sky) */
		if ((y ^ ring->y) & ~(uint16_t)(TILE - 1))
		{
			const int row_first = x & ~(BLOCK - 1);

			ring->first = MOST(ring->first, row_first);
			ring->last = LEAST(ring->last, row_first + ROW_BLOCKS * BLOCK - 1);
		}
		ring->y = y;
	}
	if (board_first > ring->last + 1 || board_last < ring->first - 1 || ring->first > ring->last)
	{
		ring->first = board_first;    /* (a jump: what the board shows) */
		ring->last = board_last | (TILE - 1);
	}
	ring->first = LEAST(ring->first, board_first);
	ring->last = MOST(ring->last, board_last | (TILE - 1));
	if (ring->last - ring->first >= RING)
		ring->first = ring->last - (RING - 1);
	{
		/* the game's new column (draw_scrolled_layer_edges: as its scroll x passes 8 pixels, or a frame or more later
		   when a column and a row are both due, one a frame), at its offset (GAME_AHEAD past the screen's left edge going
		   right, at it going left): the ring holds it when it comes this frame, and the column 512 pixels before it
		   may be gone whenever (the console draws that one again where the wider screen shows it) */
		const int column = (uint16_t)(x + mem_read16(LAYER_MAP(layer_maps[layer])->offset_x)) & ~(TILE - 1);

		if (((x ^ ring->x) & ~(TILE - 1)) && edges_on(layer) && layer_columns_on)
		{
			if (column > ring->last)
			{
				ring->last = column + TILE - 1;
				ring->first = MOST(ring->first, ring->last - (RING - 1));
			}
			else if (column < ring->first)
			{
				ring->first = column;
				ring->last = LEAST(ring->last, ring->first + RING - 1);
			}
		}
		if (column > ring->last)
			ring->first = MOST(ring->first, column + TILE - RING);
	}
	ring->x = x;
}

/* The columns the wider screen shows, from 'left' columns before the board's to 'right' past it, that the ring does
   not hold: drawn from the map, never past its ends */
static void draw_side_columns(int layer, const layer_map_area *map, int left, int right)
{
	layer_ring *ring = &rings[layer];
	const int x = layer_scroll_x(layer);
	const int shown_first = x - left, shown_last = x + SIMPSONS_VIDEO_WIDTH - 1 + right;
	const bool floor = layer == floor_layer_now();

	for (int column = ring->last + 1; column <= shown_last && column + TILE <= map->width; column += TILE)
	{
		if (!draw_column(layer, map, column, floor, ring->y))
			break;
		ring->last = column + TILE - 1;
		if (ring->last - ring->first >= RING)
			ring->first = ring->last - (RING - 1);
	}
	for (int column = ring->first - TILE; column + TILE - 1 >= shown_first && column >= 0; column -= TILE)
	{
		if (!draw_column(layer, map, column, floor, ring->y))
			break;
		ring->first = column;
		if (ring->last - ring->first >= RING)
			ring->last = ring->first + RING - 1;
	}
	if (floor)
	{
		/* (and the camera's layer past the right side, as far as the ring has room for it with all the picture shows:
		   the floor map's cells there, where the objects put in past the side walk in from. Past them the floor map
		   holds another place's, which an enemy goes round as ground it cannot walk on: stage 1's first goon stayed
		   there) */
		const int room_last = (shown_first & ~(TILE - 1)) + RING - 1;

		for (int column = ring->last + 1; column <= shown_last + DRAWN_AHEAD && column + TILE - 1 <= room_last
			&& column + TILE <= map->width; column += TILE)
		{
			if (!draw_column(layer, map, column, floor, ring->y))
				break;
			ring->last = column + TILE - 1;
			if (ring->last - ring->first >= RING)
				ring->first = ring->last - (RING - 1);
		}
	}
}

/* A map no wider than the ring (stage 6's layer A: 512 pixels, which its scroll does not move): drawn whole in it, its
   columns where the ring holds them, as the game drew its first 352 (no ends: the ring shows it round) */
static void draw_whole_ring(int layer, const layer_map_area *map)
{
	layer_ring *ring = &rings[layer];
	const uint16_t y = (uint16_t)layer_scroll_y(layer);
	const bool floor = layer == floor_layer_now();

	if (edges_on(layer) && layer_rows_on)
	{
		if ((y ^ ring->y) & ~(uint16_t)(TILE - 1))
			ring->first = ring->last = -1;    /* (scrolled up or down with the game's rows: its rows again) */
		ring->y = y;
	}
	if (ring->first <= 0 && ring->last >= map->width - 1)
		return;
	for (int column = 0; column + TILE <= map->width; column += TILE)
		if ((column < ring->first || column > ring->last) && !draw_column(layer, map, column, floor, ring->y))
			break;
	ring->first = 0;
	ring->last = map->width - 1;
}

/* ---- where WIDE is, and how wide ---- */

enum { SHOW_BOARD, SHOW_PICTURE, SHOW_PLAY };    /* the board's 320 alone, a picture of its width, a stage's play */

static unsigned last_step = 0xFFu, last_mode = 0xFFu;
static int shown_now = SHOW_BOARD, shown_drawn = SHOW_BOARD;    /* (the next frame's, and the frame shown's) */
static int wide_left_now, wide_right_now;

static int screen_wide;    /* WIDE AUTO's, as the port says the screen is (WIDE_OFF till it does) */

void console_set_screen_wide(int kind)
{
	screen_wide = kind > WIDE_OFF && kind < WIDE_AUTO ? kind : WIDE_OFF;
}

int console_wide_in_effect(void)
{
	const int wide = console_settings.wide;

	if (wide == WIDE_AUTO)
		return console_arcade_testing() ? WIDE_OFF : screen_wide;    /* (the arcade's scenarios: WIDE changes the
		                                                                play, the spawns past its sides) */
	return wide > WIDE_OFF && wide < WIDE_AUTO ? wide : WIDE_OFF;
}

static int wide_kind(void)
{
	return console_wide_in_effect();
}

int console_wide_view(void)
{
	return k_views[wide_kind()];
}

/* The cabinet's black edges, the fixed layer's tile 3F in its ring's columns 12-13 and 50-51 (the board's 0-15 and
   304-319), on most of its rows */
static bool edges_masked(void)
{
	int masked = 0;

	for (unsigned row = 0; row < 32u; row++)
		for (unsigned column = 12; column < 52; column += 38)
			masked += simpsons_board.tile_ram[0x2000u + row * 64u + column] == MASK_TILE
				&& simpsons_board.tile_ram[row * 64u + column] == 0u;
	return masked >= 48;
}

/* A stage's play: its maps drawn, the game's steps going on */
static bool in_play(void)
{
	const int layer = camera_main_layer ? 1 : 0;

	return console.page == PAGE_GAME && game_mode == GAME_MODE_GAME
		&& (mode_phase == PHASE_STEPS || mode_phase == PHASE_CONTINUE || mode_phase == PHASE_GAME_OVER)
		&& stage_step(game_step) && rings[layer].known && (rings[layer].drawn_scene & 0x80u);    /* (a stage's map: the
		                                                                                             stage card's TV
		                                                                                             is scene 02) */
}

static int what_shows(void)
{
	if (wide_kind() == WIDE_OFF || console.page == PAGE_PRESENTATION || !console.booted)
		return SHOW_BOARD;
	if (in_play() && !(layer_rows_on && !layer_columns_on))
		return SHOW_PLAY;
	return SHOW_PICTURE;    /* (and a stage's climb, which scrolls up and not across: stage 3's cemetery, its map black
	                           past the cabinet's 288 columns; the game draws its rows there with offsets of their own) */
}

/* The camera's room in its stage's record (stage_record_entry: its entries' checks, camera_check_x): at the right as far
   as a check stops it (stop at x), past the entries it goes on to (next entry at x). A stage's map goes on past where
   its camera stops for its boss (stage 1's: a wall of blue bricks), which the arcade never shows. At the left the
   picture shows what the camera has passed (the game's camera does not go back: its checks drop the players' push to
   the left, or stop it where it came in) */
#define CHECK_STOP          0u           /* camera_stop_at_x, and moving left camera_stop_at_x_left */
#define CHECK_NEXT_ENTRY    2u           /* moving right: camera_next_entry_at_x */
#define ENTRIES_AHEAD       16

static void camera_room(int *room_left, int *room_right)
{
	uint16_t entry = stage_record_entry;
	const int camera = (int)(uint16_t)camera_x;

	for (int entries = 0; entries < ENTRIES_AHEAD; entries++)
	{
		const int data = rom16(STAGE_RECORDS_BANK, (uint16_t)(entry + 2u));
		const int kind = data < 0 ? -1 : rom8(STAGE_RECORDS_BANK, (uint16_t)data);
		const int place = data < 0 ? -1 : rom16(STAGE_RECORDS_BANK, (uint16_t)(data + 1));

		if (kind == (int)CHECK_STOP && place >= 0)
			*room_right = LEAST(*room_right, MOST(place - camera, 0) - fixed_first);    /* (the board's last 16 columns
				were black: the picture ends there at the stop, its side less than none) */
		if (kind != (int)CHECK_NEXT_ENTRY)
			break;
		entry = (uint16_t)(entry + 4u);
	}
	(void)room_left;
}

/* The play's sides as they come to be: as wide as WIDE's at each side, but no further than a layer's map goes (the
   rest to the other side) */
static void play_sides(int side, int *left, int *right)
{
	int room_left = 0x10000, room_right = 0x10000, more;

	for (int layer = 0; layer < 2; layer++)
	{
		const layer_map_area map = rings[layer].known ? map_of(layer) : (layer_map_area){ 0 };
		const int x = layer_scroll_x(layer);

		if (!map.known || map.width <= RING)
			continue;    /* (a map no wider than the ring is in it whole: draw_whole_ring) */
		room_left = LEAST(room_left, MOST(x, 0));
		room_right = LEAST(room_right, MOST(map.width - (x + SIMPSONS_VIDEO_WIDTH), 0));
	}
	camera_room(&room_left, &room_right);
	*left = LEAST(side, room_left);
	*right = LEAST(side, room_right);
	more = 2 * side - *left - *right;
	*right += LEAST(more, room_right - *right);
	more = 2 * side - *left - *right;
	*left += LEAST(more, room_left - *left);
}

static int pan_to(int now, int target)
{
	return now + (target > now ? LEAST(WIDE_PAN, target - now) : -LEAST(WIDE_PAN, now - target));
}

static void forget_rings(void)
{
	memset(rings, 0, sizeof(rings));
}

static bool debug_on(void)
{
	static int on = -1;

	if (on < 0)
		on = getenv("SIMPSONS_WIDE_DEBUG") != NULL;
	return on != 0;
}

static void settle_moved_spawns(void);

void console_wide_frame(void)
{
	const int side = k_sides[wide_kind()];
	int left = 0, right = 0;

	if (game_step != last_step || game_mode != last_mode)
		forget_rings();    /* (another step: its maps to come) */
	settle_moved_spawns();    /* (the spawns moved out of sight that are not enemies: back at their places) */
	if (floor_camera_seen >= 0 && (int)camera_main_layer != floor_camera_seen)
		floor_layer = -1;    /* (the camera on another layer: the game's floor columns of it from now on) */
	floor_camera_seen = camera_main_layer;
	last_step = game_step;
	last_mode = game_mode;
	shown_drawn = shown_now;
	shown_now = what_shows();
	fixed_first = edges_masked() ? EDGE : 0;
	fixed_last = SIMPSONS_VIDEO_WIDTH - 1 - fixed_first;
	if (shown_now == SHOW_PLAY)
	{
		play_sides(side, &left, &right);
		if ((!wide_left_now && !wide_right_now) || shown_drawn != SHOW_PLAY)
		{
			wide_left_now = left;    /* (the sides come, or the play does: at once) */
			wide_right_now = right;
		}
		else
		{
			wide_left_now = pan_to(wide_left_now, left);
			wide_right_now = pan_to(wide_right_now, right);
		}
		for (int layer = 0; layer < 2; layer++)
		{
			const layer_map_area map = rings[layer].known ? map_of(layer) : (layer_map_area){ 0 };
			const int x = layer_scroll_x(layer);

			if (!map.known || map.width <= RING)
			{
				if (map.known)
					draw_whole_ring(layer, &map);
				simpsons_video_set_layer_window(1 + layer, -0x10000, 0x10000);
				continue;
			}
			follow_ring(layer);
			draw_side_columns(layer, &map, wide_left_now, wide_right_now);
			simpsons_video_set_layer_window(1 + layer, -x, map.width - 1 - x);    /* (nothing of it past its ends) */
		}
		simpsons_video_set_fixed((wide_right_now - wide_left_now) / 2, fixed_first, fixed_last);
	}
	else
	{
		/* (a stage's play drawn as a picture, stage 3's climb, keeps the picture where the play had it: the board's part
		   in the same place and the players' boxes too, no jump as the climb begins and ends; Sebas: "El nivel del
		   ascensor empieza mal alineado") */
		const bool climb = shown_now == SHOW_PICTURE && in_play();

		if (!climb || shown_drawn == SHOW_BOARD)
			wide_left_now = wide_right_now = shown_now == SHOW_PICTURE ? side : 0;
		for (int layer = 0; layer < 2; layer++)
		{
			const layer_map_area map = rings[layer].known && in_play() ? map_of(layer) : (layer_map_area){ 0 };

			if (map.known && map.width > RING)
				follow_ring(layer);    /* (stage 3's climb, a picture: its rings kept as its scroll goes, for the play
				                          after it) */
			simpsons_video_set_layer_window(1 + layer, -0x10000, 0x10000);
		}
		simpsons_video_set_fixed(climb ? (wide_right_now - wide_left_now) / 2 : 0, fixed_first, fixed_last);
	}
	simpsons_video_set_wide(wide_left_now, wide_right_now);
	if (debug_on())
	{
		const int data = rom16(STAGE_RECORDS_BANK, (uint16_t)(stage_record_entry + 2u));
		const layer_map_area map_a = map_of(0), map_b = map_of(1);

		fprintf(stderr, "frame %llu: show %d sides %d %d step %u phase %u/%u scene %02X/%u main %u camera %04X,%04X "
			"A %04X,%04X ring %d %d..%d w %d  B %04X,%04X ring %d %d..%d w %d edges %u%u%u%u entry %04X x %04X right %d %04X "
			"left %d %04X\n",
			(unsigned long long)simpsons_board.frame, shown_now, wide_left_now, wide_right_now, (unsigned)game_step,
			(unsigned)step_phase, (unsigned)mode_phase, (unsigned)map_scene, (unsigned)map_area,
			(unsigned)camera_main_layer, (unsigned)camera_x, (unsigned)camera_y, (unsigned)layer_a_scroll_x,
			(unsigned)layer_a_scroll_y, rings[0].known, rings[0].first, rings[0].last, map_a.width,
			(unsigned)layer_b_scroll_x, (unsigned)layer_b_scroll_y, rings[1].known, rings[1].first, rings[1].last,
			map_b.width, (unsigned)layer_a_edges_on, (unsigned)layer_b_edges_on, (unsigned)layer_columns_on,
			(unsigned)layer_rows_on, (unsigned)stage_record_entry,
			(unsigned)rom16(STAGE_RECORDS_BANK, stage_record_entry), data < 0 ? -1 : rom8(STAGE_RECORDS_BANK, (uint16_t)data),
			(unsigned)(data < 0 ? 0 : rom16(STAGE_RECORDS_BANK, (uint16_t)(data + 1))),
			data < 0 ? -1 : rom8(STAGE_RECORDS_BANK, (uint16_t)(data + 3)),
			(unsigned)(data < 0 ? 0 : rom16(STAGE_RECORDS_BANK, (uint16_t)(data + 4))));
		for (int number = 0; number < 33; number++)
		{
			const uint16_t object = (uint16_t)OBJECT_ADDRESS(number);

			if (OBJECT(object)->flags)
				fprintf(stderr, "  object %d: type %02X kind %02X x %04X y %04X x_speed %d behaviour %04X action %02X step %02X "
					"vitality %02X class %d\n", number, (unsigned)OBJECT(object)->type, (unsigned)OBJECT(object)->kind,
					(unsigned)OBJECT(object)->x, (unsigned)OBJECT(object)->y, (int)OBJECT(object)->x_speed,
					(unsigned)OBJECT(object)->behaviour, (unsigned)OBJECT(object)->action,
					(unsigned)OBJECT(object)->action_step, (unsigned)OBJECT(object)->vitality,
					(int)OBJECT(object)->enemy_class);
		}
	}
}

/* ---- the pictures' sides ---- */

/* A column's colour, when nearly all of its rows have it (else black) */
static uint32_t edge_colour(const console_picture *frame, int x)
{
	uint32_t colours[8];
	int counts[8], kinds = 0, best = 0;

	for (int y = 0; y < SIMPSONS_VIDEO_HEIGHT; y++)
	{
		const uint32_t colour = frame->pixels[y * frame->stride + frame->left + x] & 0xFFFFFFu;
		int kind = 0;

		while (kind < kinds && colours[kind] != colour)
			kind++;
		if (kind == kinds)
		{
			if (kinds == 8)
				continue;
			colours[kinds] = colour;
			counts[kinds++] = 0;
		}
		counts[kind]++;
	}
	for (int kind = 1; kind < kinds; kind++)
		if (counts[kind] > counts[best])
			best = kind;
	return kinds && counts[best] * 10 >= SIMPSONS_VIDEO_HEIGHT * 9 ? colours[best] : 0x000000u;
}

/* Over the frame the video drew: a picture's sides (and the board's edges the cabinet showed black) in its edges'
   colour, or black */
void wide_picture_sides(console_picture *frame)
{
	const int first = fixed_first ? fixed_first : 0, last = SIMPSONS_VIDEO_WIDTH - 1 - first;

	if (shown_drawn != SHOW_PICTURE || (!frame->left && !frame->right))
		return;
	{
		const uint32_t left = edge_colour(frame, first), right = edge_colour(frame, last);

		draw_fill(frame, PICTURE_FIRST(frame), 0, first - PICTURE_FIRST(frame), SIMPSONS_VIDEO_HEIGHT, left);
		draw_fill(frame, last + 1, 0, PICTURE_END(frame) - last - 1, SIMPSONS_VIDEO_HEIGHT, right);
	}
}

/* The picture's columns the window leaves out at each side (its middle: console_wide_view), for what is written at
   the picture's edges */
int wide_crop(const console_picture *frame)
{
	const int width = SIMPSONS_VIDEO_WIDTH + frame->left + frame->right, view = console_wide_view();

	return width > view ? (width - view) / 2 : 0;
}

/* ---- the objects past the wider screen's sides ---- */

/* Whether the play begins in this frame (its step's maps drawn whole: what_shows says so before the frame's end, where
   console_wide_frame sets its sides), and the sides it comes with ('left', 'right': at once). Its spawns are the
   board's, those that come in by themselves moved out of the wider screen's sight (spawn_as_play_begins) */
static bool play_begins(int *left, int *right)
{
	if (shown_now == SHOW_PLAY || game_step != last_step || game_mode != last_mode || what_shows() != SHOW_PLAY)
		return false;
	play_sides(k_sides[wide_kind()], left, right);
	return *left > 0 || *right > 0;
}

/* The picture's sides in the game's play now (the frame to be drawn next; a side may be less than none), 0 out of a
   stage's play; in the frame the play begins, the sides it comes with (a goon moved out of the wider screen's sight
   as it was put in was taken off at its first step there, past the board's limits) */
static void play_sides_now(int *left, int *right)
{
	if (shown_now == SHOW_PLAY)
	{
		*left = wide_left_now;
		*right = wide_right_now;
	}
	else if (!play_begins(left, right))
		*left = *right = 0;
}

static void play_now(int *left, int *right)
{
	play_sides_now(left, right);
	*left = MOST(*left, 0);    /* (a side less than none ends where the board's black edge did: the arcade's own
	                              limits there) */
	*right = MOST(*right, 0);
}

/* For checks (--wide-check): what the spawn list put in this frame, where it was put */
#define SPAWNS_NOTED        16
#define SIGHT_HALF          16           /* (an object's sprite, at each side of its x) */

static struct
{
	uint8_t type, state;
	int16_t x;
	int side_left, side_right;
} spawns_noted[SPAWNS_NOTED];
static int spawns_noted_count;

static void note_spawn(uint16_t object, int picture_left, int picture_right)
{
	if (spawns_noted_count >= SPAWNS_NOTED)
		return;
	spawns_noted[spawns_noted_count].type = OBJECT(object)->type;
	spawns_noted[spawns_noted_count].state = OBJECT(object)->parameter;
	spawns_noted[spawns_noted_count].x = (int16_t)OBJECT(object)->x;
	spawns_noted[spawns_noted_count].side_left = picture_left;    /* (the picture's: a side may be less than none) */
	spawns_noted[spawns_noted_count].side_right = picture_right;
	spawns_noted_count++;
}

/* The spawns that come in by themselves from where they are put (the enemies' types, spawn_stage_objects' kind): the
   goons that walk, run or float in and fight. Not what drives in or walks in for a time (the police car, E9: it would
   stop past the side), nor what puts itself at the board's edge when it comes (stage 6's donuts, FD), nor what waits
   at its place on the map (stage 3's ghost on its rope, 11; the bosses) */
static bool comes_in_type(uint8_t type)
{
	switch (type)
	{
	case 0x00:    /* the plain goon (stage 1's at its start: Sebas, "hay un enemigo ahí fijo") */
	case 0x01:    /* the goon with a hat */
	case 0x02:    /* the fireman goon */
	case 0x03:    /* the goon that grabs close */
	case 0x0A:    /* the goon with an umbrella (not 08 nor 2A: stage 1's figures at its shops take them, kinds 87 and 93,
	                 the restaurant's cook put in by the first at its place) */
	case 0x0C:    /* the striker goon */
	case 0x0D:    /* the goon with a shovel */
	case 0x0E:    /* the fat goon */
	case 0x12:    /* the fat goon that throws */
	case 0x15:    /* the goon walking in */
	case 0x17:    /* the goon running in */
	case 0x25:    /* the goon rushing in */
	case 0x28:    /* the fat goon walking in */
	case 0x29:    /* the goon with a shovel rushing in */
	case 0x2E:    /* the goon with a bomb running in */
	case 0x34:    /* the striker goon rushing in */
	case 0xE6:    /* the fat goon that grabs and body-slams */
	case 0xEB:    /* the goon that keeps away */
		return true;
	}
	return false;
}

#define OBJECT_RIGHT        0x17F        /* an object's x at the board's right edge */
#define SPAWN_EDGE          0x190        /* where the game puts a spawn in as its scroll comes (the camera's 0150) */
#define SPAWN_SLACK         8            /* (the scroll's steps past it in a frame) */

/* A spawn put in at the right that the cabinet did not show (under its black edge or past it) but the wider screen
   does: as far past the wider screen's edge as it was past the cabinet's, and no further than one the game puts in as
   its scroll comes. That is a spawn list read at once (a stage's start, an area's): stage 1's goon waiting for its
   opening scene to end just past the cabinet's edge, standing in WIDE's right side (a player's report) */
static uint16_t past_right_side(uint16_t x, int side_right, int picture_left, int picture_right)
{
	const int screen_x = (int16_t)x, more = picture_right + fixed_first;

	/* (and on the floor drawn past the side: draw_side_columns, as far as the ring has room; in 20:9 at a stage's start
	   a few pixels of it may show at the picture's edge) */
	const int floor_last = LEAST(OBJECT_RIGHT + picture_right + DRAWN_AHEAD, OBJECT_LEFT - picture_left + RING - 1 - TILE)
		- TILE;

	if (screen_x - SIGHT_HALF < OBJECT_RIGHT - fixed_first || screen_x >= SPAWN_EDGE + side_right - SPAWN_SLACK
		|| more <= 0)
		return x;    /* (in the cabinet's sight, or put in as the scroll comes: past the side already) */
	return (uint16_t)MOST(screen_x, LEAST(LEAST(screen_x, SPAWN_EDGE) + more, floor_last));
}

#define SPAWN_LEFT          0x160        /* how far left of its place a spawn of the left is put (its record's bit 7) */

/* A spawn put in at the left: one due before the board's edge would bring it (by the wider right side, as the scroll
   comes) as far past the wider screen's left side as the board's past its own; one the board puts in too (a spawn
   list read at once: an area's start), where the board has it if the cabinet shows it there (a scene's), else as far
   past the wider screen's edge as it was past the cabinet's */
static uint16_t past_left_side(uint16_t placed, int side_left, int side_right, int picture_left)
{
	const int placed_x = (int16_t)placed, board_x = placed_x - SPAWN_LEFT, more = picture_left + fixed_first;

	if (in_cemetery != 0u)
		return (uint16_t)(board_x - side_left);
	if (placed_x > SPAWN_EDGE)
		return (uint16_t)(board_x - side_left - side_right);
	if (board_x + SIGHT_HALF > OBJECT_LEFT + fixed_first || more <= 0)
		return (uint16_t)board_x;
	return (uint16_t)(board_x - more);
}

#define OBJECTS             33
#define FIRST_ENEMY         14           /* (the objects the enemies' lists run: object_lists.c) */
#define MOVED_MOST          16
#define MOVED_FRAMES        2            /* (the frames a moved spawn has to show it is an enemy) */

#define MOVED_DRIFT         8            /* (a moved spawn's own steps meanwhile: farther, it put itself somewhere) */

/* The spawns moved out of the wider screen's sight, till they show what they are: the type of a spawn does not say it
   (stage 1's figures at its shops take types the goons that come in take elsewhere: moved, the restaurant's cook
   stood at the video arcade's in 20:9; Sebas, "tiene q ir en el otro local"). One that is not an enemy after its first
   frames (its enemy_class: the goons' 1) goes back to its place, the board's, and so does what it put in at its own
   place meanwhile (the cook, by the figure beside her); one that has put itself somewhere stays there */
static struct
{
	uint16_t object;
	int16_t moved, x;    /* (how far, and where it was put) */
	uint8_t frames;
} moved_spawns[MOVED_MOST];
static int moved_count;
static uint32_t objects_in_use;    /* (the enemies' objects in use at the last frame's end, bit n object n) */

static void note_moved(uint16_t object, int moved)
{
	if (moved && moved_count < MOVED_MOST)
	{
		moved_spawns[moved_count].object = object;
		moved_spawns[moved_count].moved = (int16_t)moved;
		moved_spawns[moved_count].x = (int16_t)OBJECT(object)->x;
		moved_spawns[moved_count].frames = 0;
		moved_count++;
	}
}

/* At each frame's end: what a moved spawn put in at its place this frame, moved with it; a moved spawn that is an
   enemy kept where it was moved, one that is not back at its place after MOVED_FRAMES (or gone: forgotten) */
static void settle_moved_spawns(void)
{
	uint32_t in_use = 0;
	int kept = 0, count = moved_count;

	for (int number = FIRST_ENEMY; number < OBJECTS; number++)
	{
		const uint16_t object = (uint16_t)OBJECT_ADDRESS(number);

		if (OBJECT(object)->flags == 0u)
			continue;
		in_use |= 1u << number;
		if (objects_in_use & (1u << number))
			continue;
		for (int index = 0; index < count; index++)
			if (moved_spawns[index].object != object && moved_spawns[index].x == (int16_t)OBJECT(object)->x
				&& moved_count < MOVED_MOST)
			{
				moved_spawns[moved_count] = moved_spawns[index];
				moved_spawns[moved_count].object = object;
				moved_count++;
				break;
			}
	}
	objects_in_use = in_use;
	for (int index = 0; index < moved_count; index++)
	{
		const uint16_t object = moved_spawns[index].object;
		const int x = (int16_t)OBJECT(object)->x;

		if (OBJECT(object)->flags == 0u || (int8_t)OBJECT(object)->enemy_class > 0)
			continue;
		if (++moved_spawns[index].frames < MOVED_FRAMES)
		{
			moved_spawns[kept++] = moved_spawns[index];
			continue;
		}
		if (x - moved_spawns[index].x <= MOVED_DRIFT && moved_spawns[index].x - x <= MOVED_DRIFT)
			OBJECT(object)->x = (uint16_t)(x - moved_spawns[index].moved);
	}
	moved_count = kept;
}

/* The play's first frame (play_begins): the board's spawns, then those that come in by themselves that the cabinet
   does not show (under its black edges or past them) moved out of the wider screen's sight, as its sides come at once
   (stage 1's goon, waiting for its opening scene to end just past the cabinet's right edge: a player's report, Sebas:
   "La posición inicial fuera del 4:3"). The scene's own, the cabinet's to see, where the board has them */
static void spawn_as_play_begins(int left, int right)
{
	bool used[OBJECTS];

	for (int number = FIRST_ENEMY; number < OBJECTS; number++)
		used[number] = OBJECT(OBJECT_ADDRESS(number))->flags != 0u;
	original_spawn_stage_objects();
	for (int number = FIRST_ENEMY; number < OBJECTS; number++)
	{
		const uint16_t object = (uint16_t)OBJECT_ADDRESS(number);
		const int x = (int16_t)OBJECT(object)->x;

		if (used[number] || OBJECT(object)->flags == 0u)
			continue;
		if (comes_in_type(OBJECT(object)->type))
		{
			if (x - SIGHT_HALF >= OBJECT_RIGHT - fixed_first)
				OBJECT(object)->x = past_right_side((uint16_t)x, MOST(right, 0), left, right);
			else if (x + SIGHT_HALF <= OBJECT_LEFT + fixed_first && left + fixed_first > 0)
				OBJECT(object)->x = (uint16_t)(x - left - fixed_first);
			note_moved(object, (int16_t)OBJECT(object)->x - x);
		}
		note_spawn(object, left, right);
	}
}

/* For checks (--wide-check): what the spawn list put in since the last look that the wider screen showed where it
   was put and the cabinet did not (past its black edges: the board's first and last 16 columns); how many */
int console_wide_spawn_check(char *text, size_t size)
{
	int shown = 0, used = 0;

	if (size)
		text[0] = 0;
	for (int index = 0; index < spawns_noted_count; index++)
	{
		const int x = spawns_noted[index].x;
		const bool cabinet = x + SIGHT_HALF > OBJECT_LEFT + fixed_first
			&& x - SIGHT_HALF < OBJECT_LEFT + SIMPSONS_VIDEO_WIDTH - 1 - fixed_first;
		const bool wide = x + SIGHT_HALF > OBJECT_LEFT - spawns_noted[index].side_left
			&& x - SIGHT_HALF < OBJECT_LEFT + SIMPSONS_VIDEO_WIDTH - 1 + spawns_noted[index].side_right;

		if (wide && !cabinet)
		{
			shown++;
			if (used >= 0 && (size_t)used < size)
				used += snprintf(text + used, size - (size_t)used, "type %02X (state %02X) at x %04X, sides %d %d; ",
					(unsigned)spawns_noted[index].type, (unsigned)spawns_noted[index].state, (unsigned)(uint16_t)x,
					spawns_noted[index].side_left, spawns_noted[index].side_right);
		}
	}
	spawns_noted_count = 0;
	return shown;
}

/* The game's spawn_stage_objects (objects/spawning.c), the spawns due as far past the wider screen's right side as
   past the board's, put in at their place; those that come in from the left as far past its left side */
void spawn_stage_objects(void)
{
	uint32_t carry = 0;
	uint8_t enemy_speed = 0, spawn_flags = 0, spawn_height = 0;
	uint16_t spawn = 0, spawned = 0;
	uint8_t flags = 0, level = 0, flags_2 = 0, kind = 0, state = 0, flags_3 = 0, height_steps = 0, depth = 0;
	uint16_t enemies_speed = 0, camera_position = 0, spawn_edge = 0, scroll_position = 0, spawn_place = 0, compared_scroll = 0, spawn_stage_x = 0, camera_offset = 0, spawn_screen_x = 0, placed_x = 0, left_side_x = 0, signed_steps = 0, height_pixels = 0, negated_height = 0, height_on_screen = 0, spawn_y = 0;
	int side_left, side_right, picture_left, picture_right;

	if (in_cemetery == 0u && play_begins(&picture_left, &picture_right))
	{
		spawn_as_play_begins(picture_left, picture_right);    /* (the play's first frame: the board's spawns) */
		return;
	}
	play_sides_now(&picture_left, &picture_right);
	side_left = MOST(picture_left, 0);
	side_right = MOST(picture_right, 0);
	if (!side_left && !side_right)
	{
		original_spawn_stage_objects();
		return;
	}
	enemy_speed = enemy_speeds_by_rank[(int8_t)rank];
	enemies_speed = (uint32_t)enemy_speed << 1;
	carry = 0u;
	io_write16(enemy_walk_speed, enemies_speed);
	io_write8(stage_player_count, player_count);
	if (in_cemetery != 0u)
	{
		window_bank = 0x1Bu;
		set_lines(window_bank);
	}
	else
	{
		carry = check_enemy_limit(carry);
		if (carry)
			return;
		window_bank = 0x1Bu;
		set_lines(window_bank);
	}
	for (;;)
	{
		spawn = spawn_next;
		if ((uint8_t)(SPAWN(spawn)->x >> 8u) == 0xFFu)
			break;
		if (in_cemetery == 0u)
		{
			camera_position = camera_x;
			spawn_edge = (uint16_t)(camera_position + 0x150u + (unsigned)side_right);    /* (WIDE: its right side) */
			if (spawn_edge < SPAWN(spawn)->x)
				break;
		}
		else
		{
			scroll_position = cemetery_scroll;
			spawn_place = SPAWN(spawn)->x;
			compared_scroll = scroll_position;
			carry = compared_scroll < spawn_place;
			if (compared_scroll < spawn_place)
				break;
			flags = SPAWN(spawn)->flags;
			if ((flags & 0xCu) == 0u)
			{
				carry = check_enemy_limit(carry);
				if (carry)
				{
					spawn_next = spawn + 8u;
					continue;
				}
			}
		}
		level = SPAWN(spawn)->level;
		if (difficulty < level)
		{
			spawn_next = spawn + 8u;
			continue;
		}
		flags_2 = SPAWN(spawn)->flags;
		spawn_flags = flags_2;
		take_free_object_for_spawn(spawn_flags, &spawned, &carry);
		if (carry == 0u)
		{
			spawn_next = spawn + 8u;
			continue;
		}
		kind = SPAWN(spawn)->kind;
		OBJECT(spawned)->type = kind;
		AT16(OBJECT(spawned)->state, -1) = 0u;
		state = SPAWN(spawn)->state;
		OBJECT(spawned)->parameter = state;
		spawn_stage_x = SPAWN(spawn)->x;
		camera_offset = spawn_stage_x - camera_x;
		spawn_screen_x = camera_offset + 0x40u;
		OBJECT(spawned)->x = spawn_screen_x;
		flags_3 = SPAWN(spawn)->flags;
		if ((int8_t)flags_3 < 0)
		{
			placed_x = OBJECT(spawned)->x;
			left_side_x = past_left_side(placed_x, side_left, side_right, picture_left);    /* (WIDE: past its left
				side, from the right side's distance; where the board has it if the cabinet shows it there) */
			OBJECT(spawned)->x = left_side_x;
		}
		else if (in_cemetery == 0u && comes_in_type(kind))
		{
			OBJECT(spawned)->x = past_right_side(spawn_screen_x, side_right, picture_left, picture_right);    /* (WIDE: out of its sight) */
			note_moved(spawned, (int16_t)OBJECT(spawned)->x - (int16_t)spawn_screen_x);
		}
		OBJECT(spawned)->x_fraction = 0u;
		height_steps = SPAWN(spawn)->y;
		signed_steps = (int8_t)height_steps;
		height_pixels = (uint32_t)signed_steps << 3;
		negated_height = 0u - height_pixels;
		height_on_screen = negated_height + camera_y;
		spawn_y = height_on_screen + 0x190u;
		OBJECT(spawned)->y = spawn_y;
		OBJECT(spawned)->y_fraction = 0u;
		depth = SPAWN(spawn)->depth;
		spawn_height = depth;
		OBJECT(spawned)->z = spawn_height;
		OBJECT(spawned)->z_fraction = 0u;
		OBJECT(spawned)->floor_z = spawn_height;
		OBJECT(spawned)->carried = 0u;
		OBJECT(spawned)->blocked = 0u;
		note_spawn(spawned, picture_left, picture_right);
		spawn_next = spawn + 8u;
	}
}

/* The game's enemy_remove_off_screen (objects/object_setup.c): the object removed 64 pixels past the wider screen's
   sides */
uint16_t enemy_remove_off_screen(uint16_t object, uint16_t target)
{
	uint8_t vitality = 0;
	int16_t screen_x = 0;
	int side_left, side_right;

	play_now(&side_left, &side_right);
	if (!side_left && !side_right)
		return original_enemy_remove_off_screen(object, target);
	if (OBJECT(object)->flags == 0u)
		return target;
	screen_x = (int16_t)OBJECT(object)->x;
	if (screen_x >= -side_left && screen_x < 0x1C0 + side_right)
		return target;
	OBJECT(object)->flags = 0u;
	if (object >= OBJECT_ADDRESS(22))
		return target;
	vitality = OBJECT(object)->vitality;
	if (vitality == 0u)
		return enemy_mark_defeated(object, target);
	return enemy_release_target(object, target);
}

/* The game's remove_off_screen (system/jump_table.c): the object removed past the wider screen's left side or 64
   pixels past its right one */
uint32_t remove_off_screen(uint16_t object, uint32_t carry)
{
	int16_t object_x = 0;
	int side_left, side_right;

	play_now(&side_left, &side_right);
	if (!side_left && !side_right)
		return original_remove_off_screen(object, carry);
	if (OBJECT(object)->flags != 0u)
	{
		object_x = (int16_t)OBJECT(object)->x;
		carry = object_x >= OBJECT_LEFT - side_left && object_x < 0x1C0 + side_right;
		if (!carry)
			OBJECT(object)->flags = 0u;
	}
	return carry;
}

/* The game's stop_beyond_screen_edge (objects/object_setup.c): stopped 48 pixels past the wider screen's sides */
void stop_beyond_screen_edge(uint16_t object)
{
	int16_t screen_x = 0;
	int side_left, side_right;

	play_now(&side_left, &side_right);
	if (!side_left && !side_right)
	{
		original_stop_beyond_screen_edge(object);
		return;
	}
	if (OBJECT(object)->flags != 0u)
	{
		screen_x = (int16_t)OBJECT(object)->x;
		if (screen_x < 0x10 - side_left || screen_x > 0x1AF + side_right)
			OBJECT(object)->x_speed = 0;
	}
}

/* The game's enemy_on_screen (objects/object_setup.c): 16 pixels in from the wider screen's edges */
uint32_t enemy_on_screen(uint16_t object)
{
	int16_t screen_x = 0;
	int side_left, side_right;

	play_now(&side_left, &side_right);
	if (!side_left && !side_right)
		return original_enemy_on_screen(object);
	screen_x = (int16_t)OBJECT(object)->x;
	return screen_x >= 0x50 - side_left && screen_x <= 0x16F + side_right;
}

/* Whether an object at screen x 'x' is past a pair of the game's limits near the board's edges ('left' and 'right',
   as the enemies' moves' 40 and 17F, 16 pixels past the cabinet's sight, its black edges): those moved as far past the
   wider screen's sides (where the picture has none, the board's). beyond_move_limits: the enemies' moves 9 and 3 */
static bool beyond_limits(int x, int left, int right)
{
	int picture_left, picture_right;

	play_sides_now(&picture_left, &picture_right);
	return x < LEAST(left, left - picture_left - fixed_first) || x > MOST(right, right + picture_right + fixed_first);
}

static bool beyond_move_limits(int x)
{
	return beyond_limits(x, OBJECT_LEFT, OBJECT_RIGHT);
}

/* The game's enemy_wait_while_target_down (enemies/enemy_moves.c, Move 9): stopped past the wider screen's sides,
   not in them (a player's report: an enemy standing still in a side till it came to the board's 320) */
void enemy_wait_while_target_down(uint16_t object, uint8_t step, uint16_t *p_step_handlers, uint32_t *p_left_caller)
{
	uint16_t step_handlers = 0;
	int16_t enemy_x = 0;
	int side_left, side_right;

	play_now(&side_left, &side_right);
	if (!side_left && !side_right)
	{
		original_enemy_wait_while_target_down(object, step, p_step_handlers, p_left_caller);
		return;
	}
	step_handlers = enemy_wait_while_target_down_steps_ADDRESS;
	step = (uint8_t)((uint32_t)step << 1);
	switch ((int8_t)step)
	{
	case 0:
		enemy_wait_start(object);
		break;
	case 2:
	case 8:
		step_handlers = enemy_wait_for_target_to_stand(object);
		break;
	case 4:
		enemy_back_off_start(object);
		break;
	case 6:
		step_handlers = enemy_back_off_step(object, step_handlers);
		break;
	case 10:
		enemy_step_toward_target_start(object);
		break;
	case 12:
		enemy_step_toward_target_step(object);
		break;
	case 14:
		step_handlers = enemy_circle_target_start(object, step_handlers);
		break;
	case 16:
		step_handlers = enemy_circle_target_step(object, step_handlers);
		break;
	case 18:
		step_handlers = enemy_wander_start(object, step_handlers);
		break;
	case 20:
		step_handlers = enemy_wander_walk(object, step_handlers);
		break;
	case 22:
		step_handlers = enemy_wander_rest(object, step_handlers);
		break;
	case 24:
		enemy_rush_start(object);
		break;
	case 26:
		enemy_rush_step(object);
		break;
	case 28:
		enemy_rush_slow_down(object);
		break;
	case -14:
	case 30:
		enemy_line_up_start(object);
		break;
	case -12:
	case 32:
		enemy_line_up_in_depth(object);
		break;
	case 34:
		enemy_pace_start(object);
		break;
	case 36:
		enemy_pace_step(object);
		break;
	case 38:
		enemy_pace_end(object);
		break;
	case -98:
		enemy_climb_up_front(object, step, &step_handlers, p_left_caller);
		break;
	case -96:
		enemy_climb_up_back(object, step, &step_handlers, p_left_caller);
		break;
	case -94:
		enemy_climb_up_left(object, step, &step_handlers, p_left_caller);
		break;
	case -92:
		enemy_climb_up_right(object, step, &step_handlers, p_left_caller);
		break;
	case -90:
		enemy_hop_down_front(object, step, &step_handlers, p_left_caller);
		break;
	case -88:
		enemy_hop_down_back(object, step, &step_handlers, p_left_caller);
		break;
	case -86:
		enemy_hop_down_left(object, step, &step_handlers, p_left_caller);
		break;
	case -84:
		enemy_hop_down_right(object, step, &step_handlers, p_left_caller);
		break;
	case -82:
		step_handlers = enemy_climb_up_front_start(object);
		break;
	case -80:
	case -56:
		enemy_move_front_until_landed(object);
		break;
	case -78:
	case -72:
	case -66:
	case -60:
	case -54:
	case -48:
	case -42:
	case -36:
		enemy_back_to_thinking_after_wait(object);
		break;
	case -76:
		step_handlers = enemy_climb_up_back_start(object);
		break;
	case -74:
	case -50:
		enemy_move_back_until_landed(object);
		break;
	case -70:
		step_handlers = enemy_climb_up_left_start(object);
		break;
	case -68:
	case -44:
		enemy_move_left_until_landed(object);
		break;
	case -64:
		step_handlers = enemy_climb_up_right_start(object);
		break;
	case -62:
	case -38:
		enemy_move_right_until_landed(object);
		break;
	case -58:
	case -46:
	case -40:
		enemy_hop_start(object);
		break;
	case -52:
		enemy_hop_back_start(object);
		break;
	case -34:
		step_handlers = enemy_get_around_start(object, step_handlers);
		break;
	case -32:
		step_handlers = enemy_get_around_step(object, step_handlers);
		break;
	case -30:
		enemy_go_straight(object);
		break;
	case -28:
	case -18:
		enemy_walk_start_in_turn(object);
		break;
	case -26:
		step_handlers = enemy_walk_to_target_side_step(object);
		break;
	case -24:
		enemy_line_up_near_start(object);
		break;
	case -22:
		enemy_line_up_near_in_depth(object);
		break;
	case -20:
		step_handlers = enemy_line_up_near_in_x(object, step_handlers);
		break;
	case -16:
		step_handlers = enemy_walk_to_target_step(object);
		break;
	case -10:
		step_handlers = enemy_line_up_in_x(object, step_handlers);
		break;
	case -8:
		enemy_keep_distance_start(object);
		break;
	case -6:
		step_handlers = enemy_keep_distance_step(object, step_handlers);
		break;
	case -4:
		enemy_keep_distance_back_off(object);
		break;
	case -2:
		step_handlers = enemy_keep_distance_close_in(object, step_handlers);
		break;
	}
	enemy_x = (int16_t)OBJECT(object)->x;
	if (beyond_move_limits(enemy_x))    /* (WIDE: its sides) */
	{
		OBJECT(object)->x_speed = 0;
		OBJECT(object)->y_speed = 0;
	}
	*p_step_handlers = step_handlers;
}

/* The game's enemy_back_off (enemies/enemy_moves.c, Move 3): stopped past the wider screen's sides, not in them */
void enemy_back_off(uint16_t object, uint8_t step, uint16_t *p_step_handlers, uint32_t *p_left_caller)
{
	uint16_t step_handlers = 0;
	int16_t enemy_x = 0;
	int side_left, side_right;

	play_now(&side_left, &side_right);
	if (!side_left && !side_right)
	{
		original_enemy_back_off(object, step, p_step_handlers, p_left_caller);
		return;
	}
	step_handlers = enemy_back_off_steps_ADDRESS;
	step = (uint8_t)((uint32_t)step << 1);
	switch ((int8_t)step)
	{
	case 0:
		enemy_back_off_start(object);
		break;
	case 2:
		step_handlers = enemy_back_off_step(object, step_handlers);
		break;
	case -2:
	case 4:
		step_handlers = enemy_wait_for_target_to_stand(object);
		break;
	case 6:
		enemy_step_toward_target_start(object);
		break;
	case 8:
		enemy_step_toward_target_step(object);
		break;
	case 10:
		step_handlers = enemy_circle_target_start(object, step_handlers);
		break;
	case 12:
		step_handlers = enemy_circle_target_step(object, step_handlers);
		break;
	case 14:
		step_handlers = enemy_wander_start(object, step_handlers);
		break;
	case 16:
		step_handlers = enemy_wander_walk(object, step_handlers);
		break;
	case 18:
		step_handlers = enemy_wander_rest(object, step_handlers);
		break;
	case 20:
		enemy_rush_start(object);
		break;
	case 22:
		enemy_rush_step(object);
		break;
	case 24:
		enemy_rush_slow_down(object);
		break;
	case -18:
	case 26:
		enemy_line_up_start(object);
		break;
	case -16:
	case 28:
		enemy_line_up_in_depth(object);
		break;
	case 30:
		enemy_pace_start(object);
		break;
	case 32:
		enemy_pace_step(object);
		break;
	case 34:
		enemy_pace_end(object);
		break;
	case -102:
		enemy_climb_up_front(object, step, &step_handlers, p_left_caller);
		break;
	case -100:
		enemy_climb_up_back(object, step, &step_handlers, p_left_caller);
		break;
	case -98:
		enemy_climb_up_left(object, step, &step_handlers, p_left_caller);
		break;
	case -96:
		enemy_climb_up_right(object, step, &step_handlers, p_left_caller);
		break;
	case -94:
		enemy_hop_down_front(object, step, &step_handlers, p_left_caller);
		break;
	case -92:
		enemy_hop_down_back(object, step, &step_handlers, p_left_caller);
		break;
	case -90:
		enemy_hop_down_left(object, step, &step_handlers, p_left_caller);
		break;
	case -88:
		enemy_hop_down_right(object, step, &step_handlers, p_left_caller);
		break;
	case -86:
		step_handlers = enemy_climb_up_front_start(object);
		break;
	case -84:
	case -60:
		enemy_move_front_until_landed(object);
		break;
	case -82:
	case -76:
	case -70:
	case -64:
	case -58:
	case -52:
	case -46:
	case -40:
		enemy_back_to_thinking_after_wait(object);
		break;
	case -80:
		step_handlers = enemy_climb_up_back_start(object);
		break;
	case -78:
	case -54:
		enemy_move_back_until_landed(object);
		break;
	case -74:
		step_handlers = enemy_climb_up_left_start(object);
		break;
	case -72:
	case -48:
		enemy_move_left_until_landed(object);
		break;
	case -68:
		step_handlers = enemy_climb_up_right_start(object);
		break;
	case -66:
	case -42:
		enemy_move_right_until_landed(object);
		break;
	case -62:
	case -50:
	case -44:
		enemy_hop_start(object);
		break;
	case -56:
		enemy_hop_back_start(object);
		break;
	case -38:
		step_handlers = enemy_get_around_start(object, step_handlers);
		break;
	case -36:
		step_handlers = enemy_get_around_step(object, step_handlers);
		break;
	case -34:
		enemy_go_straight(object);
		break;
	case -32:
	case -22:
		enemy_walk_start_in_turn(object);
		break;
	case -30:
		step_handlers = enemy_walk_to_target_side_step(object);
		break;
	case -28:
		enemy_line_up_near_start(object);
		break;
	case -26:
		enemy_line_up_near_in_depth(object);
		break;
	case -24:
		step_handlers = enemy_line_up_near_in_x(object, step_handlers);
		break;
	case -20:
		step_handlers = enemy_walk_to_target_step(object);
		break;
	case -14:
		step_handlers = enemy_line_up_in_x(object, step_handlers);
		break;
	case -12:
		enemy_keep_distance_start(object);
		break;
	case -10:
		step_handlers = enemy_keep_distance_step(object, step_handlers);
		break;
	case -8:
		enemy_keep_distance_back_off(object);
		break;
	case -6:
		step_handlers = enemy_keep_distance_close_in(object, step_handlers);
		break;
	case -4:
		enemy_wait_start(object);
		break;
	}
	enemy_x = (int16_t)OBJECT(object)->x;
	if (beyond_move_limits(enemy_x))    /* (WIDE: its sides) */
	{
		OBJECT(object)->x_speed = 0;
		OBJECT(object)->y_speed = 0;
	}
	*p_step_handlers = step_handlers;
}

/* The game's ninja_rush_leap (enemies/ninja.c): the leaping ninja bounces once off the wider screen's sides, not off
   the board's edges in their sight (stage 7's ninjas bounced 48 pixels in from the picture's edge in 16:9) */
void ninja_rush_leap(uint16_t object)
{
	uint32_t landed = 0;
	uint8_t bounced = 0, bounced_2 = 0;
	uint16_t z_speed = 0, x_speed = 0;
	int16_t ninja_x = 0;
	int side_left, side_right;

	play_now(&side_left, &side_right);
	if (!side_left && !side_right)
	{
		original_ninja_rush_leap(object);
		return;
	}
	z_speed = OBJECT(object)->z_speed;
	OBJECT(object)->z_speed = z_speed - 0x20u;
	landed = vector_check_landing(object);
	if (landed)
	{
		OBJECT(object)->x_speed = 0;
		OBJECT(object)->action = 0x2Bu;
		OBJECT(object)->action_step = 0u;
		leftover_y = ninja_body_box_ADDRESS;
		vector_set_hit_box(object);
		return;
	}
	bounced = OBJECT(object)->ninja.bounced;
	if (bounced == 0u)
	{
		ninja_x = (int16_t)OBJECT(object)->x;
		if (beyond_limits(ninja_x, 0x48, 0x177))    /* (WIDE: its sides) */
		{
			x_speed = OBJECT(object)->x_speed;
			OBJECT(object)->x_speed = 0u - x_speed;
			OBJECT(object)->z_speed = 0x200;
			OBJECT(object)->animation = 0x84u;
			leftover_y = ninja_jump_attack_dive_box_ADDRESS;
			vector_set_hit_box(object);
			bounced_2 = OBJECT(object)->ninja.bounced;
			OBJECT(object)->ninja.bounced = bounced_2 + 1u;
			face_movement(object);
			return;
		}
	}
}

/* The game's clown_on_ball_hop (objects/clown_on_ball.c): the clown on his ball hops on in the wider screen's sides,
   stopped past them (stage 2's stopped in the picture's right side, hopping where he was) */
void clown_on_ball_hop(uint16_t object)
{
	uint32_t landed = 0;
	uint16_t ignored16 = 0;
	uint8_t hops_left = 0;
	int16_t clown_x = 0;
	int side_left, side_right;

	play_now(&side_left, &side_right);
	if (!side_left && !side_right)
	{
		original_clown_on_ball_hop(object);
		return;
	}
	vector_apply_gravity(object, &ignored16, &landed);
	if (landed == 0u)
	{
		clown_x = (int16_t)OBJECT(object)->x;
		if (beyond_move_limits(clown_x))    /* (WIDE: its sides) */
			OBJECT(object)->x_speed = 0;
		return;
	}
	hops_left = OBJECT(object)->clown_on_ball.hops_left;
	OBJECT(object)->clown_on_ball.hops_left = hops_left - 1u;
	if (hops_left == 1u)
	{
		OBJECT(object)->state = 2u;
		OBJECT(object)->timer = 0u;
		return;
	}
	OBJECT(object)->z_speed = 0x400;
	clown_x = (int16_t)OBJECT(object)->x;
	if (beyond_move_limits(clown_x))    /* (WIDE: its sides) */
		OBJECT(object)->x_speed = 0;
}

/* The game's krusty_balloon_fly_up (bosses/krusty_balloon.c): Krusty's balloon, knocked up, bounces off the wider
   screen's sides (stage 2's boss bounced off the board's left edge, 112 pixels in from the picture's in 16:9) */
void krusty_balloon_fly_up(uint16_t object)
{
	uint8_t vitality = 0, step = 0, shadow_player = 0;
	uint16_t z = 0, x_speed = 0, reversed_x_speed = 0, hit_by = 0;
	int16_t screen_x = 0;
	int side_left, side_right;

	play_now(&side_left, &side_right);
	if (!side_left && !side_right)
	{
		original_krusty_balloon_fly_up(object);
		return;
	}
	z = OBJECT(object)->z;
	if ((int16_t)z < 0x80)
	{
		screen_x = (int16_t)OBJECT(object)->x;
		if (beyond_move_limits(screen_x))    /* (WIDE: its sides) */
		{
			x_speed = OBJECT(object)->x_speed;
			reversed_x_speed = 0u - x_speed;
			OBJECT(object)->x_speed = reversed_x_speed;
			return;
		}
		return;
	}
	OBJECT(object)->x_speed = 0;
	OBJECT(object)->z_speed = 0;
	vitality = OBJECT(object)->vitality;
	if (krusty_balloon_beaten_health_by_players[(int8_t)((uint8_t)(player_count - 1u))] < vitality)
	{
		step = OBJECT(object)->krusty_balloon.step;
		OBJECT(object)->krusty_balloon.step = step + 1u;
		screen_x = (int16_t)OBJECT(object)->x;
		if (beyond_move_limits(screen_x))    /* (WIDE: its sides) */
		{
			x_speed = OBJECT(object)->x_speed;
			reversed_x_speed = 0u - x_speed;
			OBJECT(object)->x_speed = reversed_x_speed;
			return;
		}
		return;
	}
	OBJECT(object)->x_speed = 0;
	OBJECT(object)->y_speed = 0;
	OBJECT(object)->z_speed = 0;
	hit_by = OBJECT(object)->hit_by;
	shadow_player = OBJECT(hit_by)->shadow_player;
	OBJECT(object)->points = shadow_player & ~0xFu;
	OBJECT(object)->vitality = 0u;
	OBJECT(object)->state = 0xFDu;
	OBJECT(object)->krusty_balloon.step = 0u;
	OBJECT(object)->krusty_balloon.squash_step = 0u;
}

#define STORY_OBJECT        32           /* the scene's own actor (5640): its end takes every actor off */
#define STORY_END_MOST      120          /* (the frames a scene's end waits at most) */

static int story_end_waits;    /* (the frames the scene's end has waited) */

/* Whether an actor of the scene is leaving through the wider screen's sides: out of the cabinet's sight, in the
   picture's, and going out (then the scene's end waits a frame, STORY_END_MOST at most) */
static bool story_actors_leaving(void)
{
	int picture_left, picture_right;

	play_sides_now(&picture_left, &picture_right);
	if (story_end_waits < STORY_END_MOST)
		for (int number = 0; number < STORY_OBJECT; number++)
		{
			const uint16_t object = (uint16_t)OBJECT_ADDRESS(number);
			const int x = (int16_t)OBJECT(object)->x, speed = OBJECT(object)->x_speed;

			if (OBJECT(object)->flags == 0u)
				continue;
			if ((speed > 0 && x - SIGHT_HALF >= OBJECT_RIGHT - fixed_first && x - SIGHT_HALF < OBJECT_RIGHT + picture_right)
				|| (speed < 0 && x + SIGHT_HALF <= OBJECT_LEFT + fixed_first && x + SIGHT_HALF > OBJECT_LEFT - picture_left))
			{
				story_end_waits++;
				return true;
			}
		}
	story_end_waits = 0;
	return false;
}

/* The game's story_actor_finish_frame (scenes/story_actor.c): a scene's actor taken off the screen past the wider
   screen's sides, not in them (stage 1's opening: Smithers' goons gone where the cabinet's screen ended; Sebas,
   "Desaparece en 4:3") */
uint16_t story_actor_finish_frame(uint16_t object, uint16_t u)
{
	uint8_t actor_script = 0, count = 0;
	uint16_t child_slot = 0, x_now = 0, z_now = 0;
	uint8_t child_slots_left = 0;
	uint16_t saved_actor = 0, saved_child_slot = 0;
	uint8_t children_left = 0, players_signal = 0, parameter = 0;
	uint16_t child_slot_at = 0, slot_child = 0, actor_x = 0, z = 0;
	int side_left, side_right;

	play_now(&side_left, &side_right);
	if (!side_left && !side_right)
		return original_story_actor_finish_frame(object, u);
	if ((int16_t)object < 0x4A40 || (int16_t)object > 0x4CE0)
	{
		count = 0xBu;
		saved_actor = object;
		child_slot = object + 0x43u;
		do
		{
			child_slots_left = count;
			child_slot_at = child_slot;
			child_slot = child_slot_at + 2u;
			slot_child = mem_read16(child_slot_at);
			object = slot_child;
			if (slot_child != 0u)
			{
				saved_child_slot = child_slot;
				story_actor_run(&object, &u);
				child_slot = saved_child_slot;
			}
			children_left = child_slots_left;
			count = children_left - 1u;
		} while (children_left != 1u);
		object = saved_actor;
	}
	if (object == OBJECT_ADDRESS(32))
	{
		players_signal = io_read8(players_all_idle);
		if (players_signal != 0u)
		{
			saved_actor = object;
			object = OBJECT_ADDRESS(0);
			story_actor_run(&object, &u);
			object = OBJECT_ADDRESS(1);
			story_actor_run(&object, &u);
			object = OBJECT_ADDRESS(2);
			story_actor_run(&object, &u);
			object = OBJECT_ADDRESS(3);
			story_actor_run(&object, &u);
			object = OBJECT_ADDRESS(4);
			story_actor_run(&object, &u);
			object = OBJECT_ADDRESS(5);
			story_actor_run(&object, &u);
			object = OBJECT_ADDRESS(6);
			story_actor_run(&object, &u);
			object = OBJECT_ADDRESS(7);
			story_actor_run(&object, &u);
			object = saved_actor;
		}
	}
	if (object == OBJECT_ADDRESS(32))
		return u;
	parameter = OBJECT(object)->parameter;
	actor_script = parameter;
	if (actor_script == 0x21u || actor_script == 0xDu)
		return u;
	actor_x = OBJECT(object)->x;
	x_now = actor_x;
	if (!beyond_limits((int16_t)x_now, 0, 0x18F))    /* (WIDE: its sides) */
	{
		z = OBJECT(object)->z;
		z_now = z;
		if ((int16_t)z_now < 0x280 && (int16_t)z_now > -0x200)
			return u;
	}
	return story_actor_remove(object, u);
}

/* The game's story_actor_next_operation (scenes/story_actor.c): a scene's end (the script's FF of its own actor,
   object 32, which takes every actor off) waits while one of its actors leaves through the wider screen's
   sides, as the cabinet's had left its screen by then (stage 1's opening: Smithers' goons running off with
   Maggie, gone in the picture's right side; Sebas, "Desaparece en 4:3") */
void story_actor_next_operation(uint16_t script, uint16_t *p_object, uint16_t *p_operations)
{
	uint16_t object = (uint16_t)*p_object;
	uint16_t operations = (uint16_t)*p_operations;
	uint8_t operation = 0;
	uint16_t operation_at = 0;
	int side_left, side_right;

	play_now(&side_left, &side_right);
	if (!side_left && !side_right)
	{
		story_end_waits = 0;
		original_story_actor_next_operation(script, p_object, p_operations);
		return;
	}
	operation_at = script;
	script = operation_at + 1u;
	operation = mem_read8(operation_at);
	if (operation == 0xFFu)
	{
		if (object == OBJECT_ADDRESS(STORY_OBJECT) && story_actors_leaving())
		{
			OBJECT(object)->story_actor.wait = 1u;    /* (WIDE: the scene's end again at the next frame) */
			OBJECT(object)->story_actor.script = operation_at;
			operations = story_actor_finish_frame(object, operations);
		}
		else
			operations = story_actor_remove(object, operations);
	}
	else
	{
		operations = story_actor_operations_ADDRESS;
		switch ((int8_t)(uint8_t)((uint32_t)operation << 1))
		{
		case 0:
			story_actor_set_kind(script, &object, &operations);
			break;
		case 2:
			story_actor_set_animation(script, &object, &operations);
			break;
		case 4:
			story_actor_set_place(script, &object, &operations);
			break;
		case 6:
			story_actor_set_speeds(script, &object, &operations);
			break;
		case 8:
			story_actor_set_accelerations(script, &object, &operations);
			break;
		case 10:
			story_actor_walk_to(script, &object, &operations);
			break;
		case 12:
			leftover_y = script;
			story_actor_put_in(&object, &operations);
			break;
		case 14:
			operations = story_actor_wait(object, script, operations);
			break;
		case 16:
			story_actor_set_accelerations_xy(script, &object, &operations);
			break;
		case 18:
			story_actor_line_up_players(script, &object, &operations);
			break;
		case 20:
			story_actor_set_hit_box(script, &object, &operations);
			break;
		case 22:
			story_actor_play_sound(script, &object, &operations);
			break;
		case 24:
			operations = story_actor_halt(object, script, operations);
			break;
		case 26:
			story_actor_set_attributes(script, &object, &operations);
			break;
		case 28:
			story_actor_toggle_airborne(script, &object, &operations);
			break;
		case 30:
			story_actor_walk_to_partner(script, &object, &operations);
			break;
		case 32:
			operations = story_actor_wait_for_object(object, script);
			break;
		case 34:
			story_actor_wait_for_players(script, &object, &operations);
			break;
		case 36:
			story_actor_random_player_faces(script, &object, &operations);
			break;
		case 38:
			story_actor_set_players_animation(script, &object, &operations);
			break;
		case 40:
			story_actor_face_object(script, &object, &operations);
			break;
		case 42:
			operations = story_actor_wait_for_height(object, script, operations);
			break;
		case 44:
			leftover_y = script;
			story_actor_put_in_child(&object, &operations);
			break;
		case 46:
			operations = story_actor_wait_for_camera(object, script, operations);
			break;
		case 48:
			story_actor_set_variable(script, &object, &operations);
			break;
		case 50:
			story_actor_drop_players_held(script, &object, &operations);
			break;
		case 52:
			story_actor_walk_to_without_turning(script, &object, &operations);
			break;
		case 54:
			operations = story_actor_wait_for_x(object, script, operations);
			break;
		case 56:
			story_actor_place_players(script, &object, &operations);
			break;
		case 58:
			story_actor_start_absent_players(script, &object, &operations);
			break;
		case 60:
			story_actor_play_kind_sound(script, &object, &operations);
			break;
		case 62:
			operations = story_actor_wait_for_floor(object, script, operations);
			break;
		case 64:
			story_actor_play_sound_if_player(script, &object, &operations);
			break;
		case 66:
			story_actor_player_speaks(script, &object, &operations);
			break;
		case 68:
			story_actor_toggle_repeat_sound(script, &object, &operations);
			break;
		case 70:
			story_actor_toggle_flag_08(script, &object, &operations);
			break;
		case 72:
			operations = story_actor_wait_or_presses(object, script, operations);
			break;
		case 74:
			story_actor_stop(script, &object, &operations);
			break;
		}
	}
	*p_object = object;
	*p_operations = operations;
}

/* ---- the players at the wider screen's edges ---- */

/* The game's player_check_motion (players/motion.c), its limits at the screen's edges moved by WIDE's sides: the same
   distance from the picture's edges */
void player_check_motion(uint16_t player)
{
	uint32_t floor_at_height = 0, floor_below_height = 0, floor_compare_overflow = 0, carry = 0;
	uint8_t floor_high = 0, floor_low = 0, floor_value = 0, next_x_high = 0, next_x_low = 0;
	uint16_t floor_cell = 0, mover = 0, object = 0, player_vars = 0, right_limit = 0;
	uint8_t next_position_byte = 0;
	uint8_t teamed = 0, y_speed_2 = 0, y_high = 0, x_speed_2 = 0, x_high = 0, vitality = 0, vitality_2 = 0, scene_number = 0, attributes = 0, vitality_3 = 0;
	uint16_t partner = 0, y_and_fraction = 0, y_speed = 0, y_low_and_fraction = 0, x_and_fraction = 0, x_speed = 0, x_low_and_fraction = 0, x_speed_3 = 0, running_values = 0;
	int side_left, side_right;

	play_now(&side_left, &side_right);
	if (!side_left && !side_right)
	{
		original_player_check_motion(player);
		return;
	}
	teamed = OBJECT(player)->player.teamed;
	if ((int8_t)teamed >= 0)
		mover = player;
	else
	{
		partner = OBJECT(player)->player.partner;
		mover = partner;
	}
	y_and_fraction = (((uint32_t)(OBJECT(player)->y & 0xFFu) << 8u) | OBJECT(player)->y_fraction);
	y_speed = OBJECT(player)->y_speed;
	y_low_and_fraction = y_and_fraction;
	carry = (uint16_t)(y_low_and_fraction + y_speed) < y_low_and_fraction;
	next_position_byte = (uint16_t)(y_low_and_fraction + y_speed) >> 8;
	y_speed_2 = (uint8_t)(OBJECT(player)->y_speed >> 8u);
	y_high = (uint8_t)(OBJECT(player)->y >> 8u);
	if ((int16_t)(((uint32_t)(uint8_t)(((uint16_t)(int8_t)y_speed_2 >> 8) + y_high + carry) << 8) | next_position_byte) < 0x98)
	{
		OBJECT(player)->y_speed = 0;
		OBJECT(mover)->y_speed = 0;
	}
	x_and_fraction = (((uint32_t)(OBJECT(player)->x & 0xFFu) << 8u) | OBJECT(player)->x_fraction);
	x_speed = OBJECT(player)->x_speed;
	x_low_and_fraction = x_and_fraction;
	carry = (uint16_t)(x_low_and_fraction + x_speed) < x_low_and_fraction;
	next_position_byte = (uint16_t)(x_low_and_fraction + x_speed) >> 8;
	x_speed_2 = (uint8_t)(OBJECT(player)->x_speed >> 8u);
	x_high = (uint8_t)(OBJECT(player)->x >> 8u);
	next_x_high = ((uint16_t)(int8_t)x_speed_2 >> 8) + x_high + carry;
	next_x_low = next_position_byte;
	vitality = OBJECT(player)->vitality;
	if ((int16_t)(((uint32_t)next_x_high << 8) | next_x_low) >= (vitality == 0u ? 0x5A : 0x70) - side_left)
	{
		right_limit = 0x140u;
		vitality_2 = OBJECT(player)->vitality;
		if (vitality_2 != 0u)
		{
			next_position_byte = next_x_high;
			scene_number = scene;
			next_x_high = next_position_byte;
			if (scene_number == 4u)
				right_limit = 0x150u;
		}
		else
			right_limit += 0x26u;
		player_x_limit = (uint16_t)(right_limit + side_right);    /* (WIDE: its right side) */
		if ((int16_t)player_x_limit < (int16_t)(((uint32_t)next_x_high << 8) | next_x_low))
			OBJECT(player)->x_speed = 0;
	}
	else
		OBJECT(player)->x_speed = 0;
	object = player;
	vector_floor_ahead(object, &floor_cell, &floor_below_height);
	floor_value = (uint8_t)(floor_cell >> 8);
	if ((int8_t)floor_value >= 0)
	{
		player_floor_against_height(player, &floor_cell, &floor_at_height, &floor_below_height, &floor_compare_overflow);
		floor_high = (uint8_t)(floor_cell >> 8);
		floor_low = (uint8_t)floor_cell;
		if ((floor_below_height ^ floor_compare_overflow) | floor_at_height)
		{
			OBJECT(player)->floor_z = ((uint32_t)floor_high << 8) | floor_low;
			attributes = OBJECT(player)->attributes;
			OBJECT(player)->attributes = (attributes & 0xCFu) | player_floor_priority;
			x_speed_3 = OBJECT(player)->x_speed;
			OBJECT(mover)->x_speed = x_speed_3;
			vitality_3 = OBJECT(player)->vitality;
			running_values = io_read16(running_player_values);
			player_vars = running_values;
			PLAYER_VALUES(player_vars)->kept_life = vitality_3;
			return;
		}
	}
	object = player;
	vector_floor_ahead_x(object, &floor_cell, &floor_below_height);
	floor_value = (uint8_t)(floor_cell >> 8);
	if ((int8_t)floor_value >= 0)
	{
		player_floor_against_height(player, &floor_cell, &floor_at_height, &floor_below_height, &floor_compare_overflow);
		floor_high = (uint8_t)(floor_cell >> 8);
		floor_low = (uint8_t)floor_cell;
		if ((floor_below_height ^ floor_compare_overflow) | floor_at_height)
		{
			OBJECT(player)->y_speed = 0;
			OBJECT(mover)->y_speed = 0;
			OBJECT(player)->floor_z = ((uint32_t)floor_high << 8) | floor_low;
			attributes = OBJECT(player)->attributes;
			OBJECT(player)->attributes = (attributes & 0xCFu) | player_floor_priority;
			x_speed_3 = OBJECT(player)->x_speed;
			OBJECT(mover)->x_speed = x_speed_3;
			vitality_3 = OBJECT(player)->vitality;
			running_values = io_read16(running_player_values);
			player_vars = running_values;
			PLAYER_VALUES(player_vars)->kept_life = vitality_3;
			return;
		}
	}
	OBJECT(player)->x_speed = 0;
	object = player;
	vector_floor_ahead_y(object, &floor_cell, &floor_below_height);
	floor_value = (uint8_t)(floor_cell >> 8);
	if ((int8_t)floor_value < 0)
	{
		OBJECT(player)->y_speed = 0;
		OBJECT(mover)->y_speed = 0;
		x_speed_3 = OBJECT(player)->x_speed;
		OBJECT(mover)->x_speed = x_speed_3;
		vitality_3 = OBJECT(player)->vitality;
		running_values = io_read16(running_player_values);
		player_vars = running_values;
		PLAYER_VALUES(player_vars)->kept_life = vitality_3;
		return;
	}
	player_floor_against_height(player, &floor_cell, &floor_at_height, &floor_below_height, &floor_compare_overflow);
	floor_high = (uint8_t)(floor_cell >> 8);
	floor_low = (uint8_t)floor_cell;
	if ((floor_below_height ^ floor_compare_overflow) | floor_at_height)
	{
		OBJECT(player)->floor_z = ((uint32_t)floor_high << 8) | floor_low;
		attributes = OBJECT(player)->attributes;
		OBJECT(player)->attributes = (attributes & 0xCFu) | player_floor_priority;
		x_speed_3 = OBJECT(player)->x_speed;
		OBJECT(mover)->x_speed = x_speed_3;
		vitality_3 = OBJECT(player)->vitality;
		running_values = io_read16(running_player_values);
		player_vars = running_values;
		PLAYER_VALUES(player_vars)->kept_life = vitality_3;
		return;
	}
	OBJECT(player)->y_speed = 0;
	OBJECT(mover)->y_speed = 0;
	x_speed_3 = OBJECT(player)->x_speed;
	OBJECT(mover)->x_speed = x_speed_3;
	vitality_3 = OBJECT(player)->vitality;
	running_values = io_read16(running_player_values);
	player_vars = running_values;
	PLAYER_VALUES(player_vars)->kept_life = vitality_3;
}

/* The game's player_check_screen_edges (players/motion.c), its screen's edges moved by WIDE's sides */
void player_check_screen_edges(uint16_t player, uint32_t *p_no_crash, uint32_t *p_carry)
{
	uint32_t no_crash = *p_no_crash;
	uint32_t carry = *p_carry;
	uint32_t negative = 0;
	uint8_t floor_value = 0, height_high = 0, height_low = 0, next_x_high = 0, next_x_low = 0, stopped_flag = 0;
	uint16_t floor_cell = 0, floor_z = 0, left_limit = 0, object = 0, right_limit = 0;
	uint8_t floor_or_next_x_low = 0, floor_z_high = 0;
	uint8_t vitality = 0, vitality_2 = 0, attributes = 0, x_speed_2 = 0, x_high = 0;
	uint16_t z = 0, floor_height_byte = 0, z_2 = 0, x_speed = 0, x_and_fraction = 0, speed_word = 0;
	int side_left, side_right;

	play_now(&side_left, &side_right);
	if (!side_left && !side_right)
	{
		original_player_check_screen_edges(player, p_no_crash, p_carry);
		return;
	}
	left_limit = (uint16_t)(0x70 - side_left);    /* (WIDE: its sides) */
	right_limit = (uint16_t)(0x140 + side_right);
	vitality = OBJECT(player)->vitality;
	if (vitality == 0u)
	{
		z = OBJECT(player)->z;
		if (z != 0u)
		{
			left_limit -= 0x16u;
			right_limit += 0x26u;
		}
	}
	screen_left_edge = left_limit;
	screen_right_edge = right_limit;
	object = player;
	vector_floor_ahead_x(object, &floor_cell, &negative);
	floor_value = (uint8_t)(floor_cell >> 8);
	if ((int8_t)floor_value < 0)
	{
		OBJECT(player)->x_speed = 0;
		OBJECT(player)->y_speed = 0;
		vitality_2 = OBJECT(player)->vitality;
		if (vitality_2 != 0u)
			stopped_flag = 0xFFu;
		else
			stopped_flag = 0u;
		carry = 1u;
		no_crash = (uint8_t)~stopped_flag == 0u;
	}
	else
	{
		edge_floor_priority = floor_value & 0x30u;
		attributes = OBJECT(player)->attributes;
		OBJECT(player)->attributes = (attributes & 0xCFu) | edge_floor_priority;
		floor_height_byte = (uint8_t)((((floor_value & 0xFu) << 1) << 1) << 1);
		height_high = (uint8_t)(floor_height_byte >> 8);
		height_low = (uint8_t)floor_height_byte;
		z_2 = OBJECT(player)->z;
		if ((int16_t)z_2 < (int16_t)(((uint32_t)height_high << 8) | height_low))
		{
			OBJECT(player)->x_speed = 0;
			OBJECT(player)->y_speed = 0;
			vitality_2 = OBJECT(player)->vitality;
			if (vitality_2 != 0u)
				stopped_flag = 0xFFu;
			else
				stopped_flag = 0u;
			carry = 1u;
			no_crash = (uint8_t)~stopped_flag == 0u;
		}
		else
		{
			floor_or_next_x_low = height_low;
			floor_z_high = height_high;
			floor_z = ((uint32_t)floor_z_high << 8) | floor_or_next_x_low;
			x_speed = OBJECT(player)->x_speed;
			x_and_fraction = (((uint32_t)(OBJECT(player)->x & 0xFFu) << 8u) | OBJECT(player)->x_fraction);
			speed_word = x_speed;
			carry = (uint16_t)(speed_word + x_and_fraction) < speed_word;
			floor_or_next_x_low = (uint16_t)(speed_word + x_and_fraction) >> 8;
			x_speed_2 = (uint8_t)(OBJECT(player)->x_speed >> 8u);
			x_high = (uint8_t)(OBJECT(player)->x >> 8u);
			next_x_high = ((uint16_t)(int8_t)x_speed_2 >> 8) + x_high + carry;
			next_x_low = floor_or_next_x_low;
			if ((int16_t)screen_left_edge <= (int16_t)(((uint32_t)next_x_high << 8) | next_x_low) && (int16_t)(((uint32_t)next_x_high << 8) | next_x_low) <= (int16_t)screen_right_edge)
			{
				OBJECT(player)->floor_z = floor_z;
				no_crash = 1u;
				carry = 0u;
			}
			else
			{
				OBJECT(player)->x_speed = 0;
				OBJECT(player)->y_speed = 0;
				vitality_2 = OBJECT(player)->vitality;
				if (vitality_2 != 0u)
					stopped_flag = 0xFFu;
				else
					stopped_flag = 0u;
				carry = 1u;
				no_crash = (uint8_t)~stopped_flag == 0u;
			}
		}
	}
	*p_no_crash = no_crash;
	*p_carry = carry;
}

/* ---- checks ---- */

/* For checks (--wide-check): the columns of WIDE's sides in a stage's play (whole in the wider screen, past the board's)
   whose tiles are not their map's, of each layer; 0 when all are */
int console_wide_check(char *text, size_t size)
{
	int bad = 0, used = 0;

	if (size)
		text[0] = 0;
	if (shown_now != SHOW_PLAY || (!wide_left_now && !wide_right_now))
		return 0;
	for (int layer = 0; layer < 2; layer++)
	{
		const layer_map_area map = rings[layer].known ? map_of(layer) : (layer_map_area){ 0 };
		const int x = layer_scroll_x(layer);
		const int shown_first = x - wide_left_now, shown_last = x + SIMPSONS_VIDEO_WIDTH - 1 + wide_right_now;
		int bad_left = 0, bad_right = 0, first_bad = -1;

		if (!map.known)
			continue;
		for (int column = (shown_first + TILE - 1) & ~(TILE - 1); column + TILE - 1 <= shown_last; column += TILE)
		{
			if (column >= (x & ~(TILE - 1)) && column + TILE - 1 <= x + SIMPSONS_VIDEO_WIDTH - 1)
				continue;    /* (the board's) */
			if (column < 0 || column + TILE > map.width)
				continue;
			checking = 0;
			checked = 0;
			draw_column(layer, &map, column, false, rings[layer].y);
			if (checking * 4 > checked * 3)    /* (most of the column: not the game's own tiles changed in place) */
			{
				if (column < x)
					bad_left++;
				else
					bad_right++;
				if (first_bad < 0)
					first_bad = column;
			}
			checking = -1;
		}
		if (bad_left || bad_right)
		{
			used += snprintf(text + used, size - (size_t)used, "layer %c: %d columns at the left, %d at the right (map %d, "
				"board's first %d, sides %d %d, step %u) ", 'A' + layer, bad_left, bad_right, first_bad, x, wide_left_now,
				wide_right_now, (unsigned)game_step);
			bad += bad_left + bad_right;
		}
	}
	return bad;
}

/* ---- a LAN game's power-on (console_lan_power_on) ---- */

/* What WIDE keeps from frame to frame as at power-on, as the board's reset leaves the tile RAM: the rings' account
   (the columns drawn there are the board's state, which every device of the game hashes alike) */
void wide_power_on(void)
{
	memset(rings, 0, sizeof(rings));
	fixed_first = 0;
	fixed_last = SIMPSONS_VIDEO_WIDTH - 1;
	last_step = last_mode = 0xFFu;
	shown_now = shown_drawn = SHOW_BOARD;
	wide_left_now = wide_right_now = 0;
	floor_layer = floor_camera_seen = -1;
	story_end_waits = 0;
	moved_count = 0;
	objects_in_use = 0;
}

/* ---- a save state's part (states.c) ---- */

/* What WIDE keeps from frame to frame: its account of the layers' rings (what the game and the console have drawn in
   them, which the board's tile RAM holds), the step it is for, what it showed and its sides, and the WIDE in effect.
   A state of another WIDE than the one in effect: the next frame drawn is the board's 320 and its sides come at once
   after it */
void wide_state(simpsons_state *state)
{
	int kind = wide_kind();

	simpsons_state_part(state, "wide");
	simpsons_state_bytes(state, rings, sizeof(rings));
	simpsons_state_bytes(state, &floor_layer, sizeof(floor_layer));
	simpsons_state_bytes(state, &floor_camera_seen, sizeof(floor_camera_seen));
	simpsons_state_bytes(state, &story_end_waits, sizeof(story_end_waits));
	simpsons_state_bytes(state, moved_spawns, sizeof(moved_spawns));
	simpsons_state_bytes(state, &moved_count, sizeof(moved_count));
	simpsons_state_bytes(state, &objects_in_use, sizeof(objects_in_use));
	simpsons_state_bytes(state, &fixed_first, sizeof(fixed_first));
	simpsons_state_bytes(state, &fixed_last, sizeof(fixed_last));
	simpsons_state_bytes(state, &last_step, sizeof(last_step));
	simpsons_state_bytes(state, &last_mode, sizeof(last_mode));
	simpsons_state_bytes(state, &shown_now, sizeof(shown_now));
	simpsons_state_bytes(state, &shown_drawn, sizeof(shown_drawn));
	simpsons_state_bytes(state, &wide_left_now, sizeof(wide_left_now));
	simpsons_state_bytes(state, &wide_right_now, sizeof(wide_right_now));
	simpsons_state_bytes(state, &kind, sizeof(kind));
	if (simpsons_state_loading(state) && kind != wide_kind())
	{
		wide_left_now = wide_right_now = 0;
		shown_now = SHOW_BOARD;
		simpsons_video_set_wide(0, 0);
		simpsons_video_set_fixed(0, fixed_first, fixed_last);
		for (int layer = 0; layer < 2; layer++)
			simpsons_video_set_layer_window(1 + layer, -0x10000, 0x10000);
	}
}
