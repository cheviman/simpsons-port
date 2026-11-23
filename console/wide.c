/**
 *    Name:        wide.c
 *    Description: the console's WIDE (OPTIONS > VIDEO): the picture wider than the board's 320 pixels in a stage's
 *                 play and on every other screen, the layers' columns the game does not draw that far drawn from the
 *                 same maps, the players' boxes kept in the middle, and the objects coming in, going out and walking
 *                 past the wider screen's sides
 *    Version:     0.4.2
 *    Created:     13-11-2026
 *    Modified:    20-11-2026
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
 *    stages, the name entry, the ending, and stage 3's elevator from the platform's standing till the play goes on at
 *    the shaft's top, its map blank past the cabinet's 288 columns, the platform's limits the cabinet's) are pictures
 *    of the board's width, centred: WIDE's sides there are their edges' colour where both edges are the same one colour
 *    (the TV's orange), else black (a scene as 4:3, the players' issue 24), and black in stage 2's closing story, its fall off the cliff and down the gorge (as
 *    4:3, Sebas); the presentation keeps the board's 320. A stage's card is a picture till the stage draws its play,
 *    stage 3's too, drawn from its own scene: the play comes with its sides at once. (The elevator's sides of the shaft's own
 *    cells, climb_fill, are off: CLIMB_SIDES.)
 *
 *    The objects (the console's versions of the game's functions, console/replaced.txt): a stage's spawns come in at
 *    their place past the wider screen's right side (spawn_stage_objects: due when the camera is that much closer;
 *    those that come from the left, past its left side; stage 2's teacup ride's hub as on the board, as it takes itself
 *    off past the board's right side: due_as_on_board), and an object goes out, stops, counts as on the screen and
 *    a player walks to the screen's edges as on the board, but by the wider screen's sides (enemy_remove_off_screen,
 *    remove_off_screen, stop_beyond_screen_edge, enemy_on_screen, player_check_motion, player_check_screen_edges:
 *    the same C with their limits moved), and the enemies' moves 9 and 3 stop them past the wider screen's sides, not
 *    in them (enemy_wait_while_target_down, enemy_back_off: 16 pixels past its sight, as the board's 40 and 17F are
 *    past the cabinet's black edges). A spawn list read at once (a stage's or an area's start, the play's first frame
 *    too) puts in where the board does what the cabinet shows (a scene's), and the goons that come in by themselves
 *    from where the cabinet did not show them (under its black edges or past them: stage 1's first goon, waiting for
 *    its opening scene to end, a player's report) as far past the wider screen's edges as they were past the
 *    cabinet's; then, ahead of the list, those due in the wider screen's right side that stand in its sight (stage 2's
 *    Otto at his stall through the opening, the players' issue 16), which the list skips as it comes to them. An
 *    object's x is the screen's: 40 at the board's left edge, 17F at its right.
 *
 *    For development, SIMPSONS_WIDE_DEBUG in the environment writes each frame's sides, the game's step, the layers'
 *    scrolls and rings, the stage record's entry and the objects in use to the standard error; --wide-check (the
 *    executables') says each frame which side columns are not their map's and what the spawn list put in that the
 *    wider screen showed and the cabinet did not.
 */
#include "console_internal.h"

#include "game.h"
#include "game/data.h"    /* (game_tiles: climb_fill's, CLIMB_SIDES) */
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
#define FLOOR_COLUMNS       64u
#define FLOOR_ROWS          32u
#define BLOCK_FLAGS         0xF67Cu      /* block_tile_flags: a floor pattern's 16 cells */
#define BANK_WINDOW_END     0x8000u
#define FIXED_PROGRAM       0x78000u     /* the fixed program's place in the program ROM (8000-FFFF) */
#define BANK_BYTES          0x2000u
#define STAGE_RECORDS_BANK  0x13u        /* the stages' records (stage_slots[n].record, stage_record_entry) */
#define WIDE_PAN            4            /* the sides' move a frame toward where they come to be */
#define OBJECTS             33           /* the object slots (OBJECT_ADDRESS 0-32) */
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

/* A stage's title card (its step's phase 1, stage_title_card; stage 3's cemetery, step 7, has none: its phase 1 begins
   its play): a picture, centred, till the stage draws its play's maps (its phase 2). Stage 3's card is drawn from its
   stage's own scene (86, area 1), not the others' scene 02, and was taken for its play, its sides the card's maps':
   the play then came in panning from them, and the goons the forest puts in at its start past the cabinet's right edge
   stood in WIDE's sight (Sebas: "los secuaces esperan en altura de 4:3") */
#define TITLE_PHASE         1u
#define CEMETERY_STEP       7u
#define STAGE_8_STEP        15u

static bool title_card(void)
{
	return step_phase == TITLE_PHASE && game_step != CEMETERY_STEP;
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
	/* (its strips' table: their places in the bank's window, one after the other or the same strip again, as stage 2's
	   hole after its boss, 8 strips of the same void and its wall: drawn at the boss's place, map pixels 680-85F; past the
	   last a word of 0000) */
	for (int strip = 0; strip < STRIPS_MOST; strip++)
	{
		const int place = rom16(map.bank, (uint16_t)(map.strips + 2 * strip));

		if (place < 0x6000 || place >= (int)BANK_WINDOW_END || place < previous)
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

/* The floor map's cells WIDE put in: what it put and what the board's had there before (while the cell still holds
   WIDE's, the game has not written it since) */
static uint8_t floor_wide[FLOOR_MAP_END - FLOOR_MAP], floor_board[FLOOR_MAP_END - FLOOR_MAP];
static bool floor_put[FLOOR_MAP_END - FLOOR_MAP];

static void put_floor(uint16_t place, uint8_t value)
{
	if (checking < 0 && place >= FLOOR_MAP && place < FLOOR_MAP_END)
	{
		const unsigned cell = place - FLOOR_MAP;

		if (!floor_put[cell] || RAM8(place) != floor_wide[cell])
			floor_board[cell] = RAM8(place);
		floor_put[cell] = true;
		floor_wide[cell] = value;
		RAM8(place) = value;
	}
}

/* The board's cells back where WIDE's are still there */
static void floor_back_to_board(void)
{
	for (unsigned cell = 0; cell < FLOOR_MAP_END - FLOOR_MAP; cell++)
		if (floor_put[cell])
		{
			if (RAM8(FLOOR_MAP + cell) == floor_wide[cell])
				RAM8(FLOOR_MAP + cell) = floor_board[cell];
			floor_put[cell] = false;
		}
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
	bool card;              /* (drawn whole by a stage's title card: title_card) */
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
	ring->card = stage_step(game_step) && title_card();
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

	/* (the floor map's columns the game filled again: none of WIDE's cells in them now, even where it put the value WIDE
	   had there; else floor_back_to_board would take the game's for WIDE's and put back what was there before: stage 3's
	   elevator, its platform's floor) */
	for (int column = 0; column < LEAST(columns, (int)FLOOR_COLUMNS); column++)
		for (unsigned row = 0; row < FLOOR_ROWS; row++)
			floor_put[row * FLOOR_COLUMNS + (((unsigned)x >> 3) + (unsigned)column) % FLOOR_COLUMNS] = false;
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
		if (game_step == STAGE_8_STEP && stage_scrolls_on == 0u)
		{
			/* (stage 8's fight with Smithers, its scroll stopped: the floor map's cells as the board's holds them. Its
			   left wall is at the wider screen's left side, a column the cabinet never draws: the board's cells there
			   are another place's floor, and Smithers runs past it some 166 pixels and back. With the wall's cells
			   WIDE drew there he could be put inside it, turn in it at each frame and stay off the screen: the
			   players' issue 31) */
			floor_back_to_board();
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
static bool black_now, black_drawn;    /* (a picture's sides black whatever its edges: closing_story's) */
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

/* Stage 2's closing story once its floor has given way (stage_2_end_phase: the players off the cliff and down the
   gorge, on maps of their own no wider than the ring, the gorge's scrolling up): a picture of the board's width,
   centred, its sides black (Sebas: "cuando cae homero que es cinematica la pondria 4:3"), not the play's sides */
#define STAGE_2_STEP        4u
#define STAGE_2_END_PHASE   7u
/* And stage 7's end (stage_7_end_phase: the helicopter flying to the power plant, a picture on stage 7's scene, its
   area 1): its map is not a play's, and WIDE drew the play's sides of it, the lift-off's left side of 112 pixels, as
   blocks of other tiles round the plant and down the sides (Sebas, with the players' issue 34: "hay un error de
   fondo") */
#define STAGE_7_STEP        0x0Du
#define STAGE_7_CLOSING     7u
#define STAGE_7_END_PHASE   8u
/* And stage 8's ending in Mr. Burns' office, from its closing scene on (stage_8_closing_scene, stage_8_objects_wait,
   stage_8_end_phase): a cutscene, 4:3 with black sides as the others (the players' issue 32: "the ending's scene has
   no black borders") */
#define STAGE_8_CLOSING     12u

static bool closing_story(void)
{
	return (game_step == STAGE_2_STEP && step_phase == STAGE_2_END_PHASE)
		|| (game_step == STAGE_7_STEP && step_phase == STAGE_7_END_PHASE)
		|| (game_step == STAGE_8_STEP && step_phase >= STAGE_8_CLOSING);
}

/* A stage's play: its maps drawn, the game's steps going on */
static bool in_play(void)
{
	const int layer = camera_main_layer ? 1 : 0;

	return console.page == PAGE_GAME && game_mode == GAME_MODE_GAME && !closing_story()
		&& (mode_phase == PHASE_STEPS || mode_phase == PHASE_CONTINUE || mode_phase == PHASE_GAME_OVER)
		&& stage_step(game_step) && rings[layer].known && (rings[layer].drawn_scene & 0x80u)    /* (a stage's map: the
		                                                                                           stage card's TV is
		                                                                                           scene 02, */
		&& !rings[layer].card;    /* or stage 3's, its own scene's: till the stage draws its play) */
}

/* Stage 3's elevator: its step (7) till its scroll follows the camera (cemetery_scroll_update's step 5, the play at the
   shaft's top), the platform standing and then going up the shaft */
#define ELEVATOR_STEP       7u
#define ELEVATOR_PLAY       5u

static bool elevator(void)
{
	return game_step == ELEVATOR_STEP && stage_scroll_step < ELEVATOR_PLAY;
}

/* Stage 7's closing scene once the helicopter lifts off (stage_7_scene_lift: layer B's y going up, a row of its cells
   blanked as it goes): the board's columns of layer B have nothing there, but its map left of them, which WIDE's left
   side shows (the kabuki stage's deck and the beams under it), would go up the side (the players' issue 34) */
static bool helicopter_lift(void)
{
	return game_step == STAGE_7_STEP && step_phase == STAGE_7_CLOSING && rings[1].known
		&& (uint16_t)layer_scroll_y(1) != rings[1].y;
}

static int what_shows(void)
{
	if (wide_kind() == WIDE_OFF || console.page == PAGE_PRESENTATION || !console.booted)
		return SHOW_BOARD;
	if (in_play() && !elevator() && !(layer_rows_on && !layer_columns_on))
		return SHOW_PLAY;
	return SHOW_PICTURE;    /* (and a stage's climb, which scrolls up and not across: stage 3's elevator, its map blank
	                           past the cabinet's 288 columns, the game's rows there with offsets of their own; a picture
	                           for the objects from the platform's standing on, the platform's limits the cabinet's) */
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

/* Stage 7's closing scene: Smithers runs off to the right with Maggie, and the helicopter he is then seen in comes in
   from the right as the camera follows him. The cabinet's screen has him gone 14 frames before it; a right side wider
   than 48 shows the helicopter, him at its window, while he still runs at the board's edge (20:9's 88: the players'
   issue 34). Its right side no more than that, the rest at the left */
#define HELICOPTER_RIGHT    48

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
	if (game_step == STAGE_7_STEP && step_phase == STAGE_7_CLOSING)
		room_right = LEAST(room_right, HELICOPTER_RIGHT);
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

/* SIMPSONS_WIDE_AUDIT (a check, set when the port runs: the headless with a stage's script): each frame of a play
   with sides, what the game does past the board's sides that WIDE shows. An object gone (its flags cleared, or its
   slot another type's) while it was in a side, and an object in a side for a second without being drawn (animation
   flag 80 off): the game's own checks at the cabinet's sides that WIDE has not moved yet (the players' issue 29:
   the fat goon's obstacle, stage 2's mailbox). Many are the game's own (an enemy beaten there, a spawn waiting):
   each one is read before it is fixed */
#define AUDIT_HIDDEN        60

static void audit_frame(int left, int right)
{
	static int on = -1;
	static uint8_t last_flags[OBJECTS], last_type[OBJECTS], last_kind[OBJECTS], last_state[OBJECTS];
	static int16_t last_x[OBJECTS];
	static int hidden[OBJECTS];

	if (on < 0)
		on = getenv("SIMPSONS_WIDE_AUDIT") != NULL;
	if (!on)
		return;
	for (int number = 0; number < OBJECTS; number++)
	{
		const uint16_t object = (uint16_t)OBJECT_ADDRESS(number);
		const uint8_t flags = OBJECT(object)->flags, type = OBJECT(object)->type;
		const int16_t x = (int16_t)OBJECT(object)->x;
		const bool was_side = (left || right) && last_flags[number]
			&& ((last_x[number] >= OBJECT_LEFT - left && last_x[number] < OBJECT_LEFT)
				|| (last_x[number] >= OBJECT_LEFT + SIMPSONS_VIDEO_WIDTH && last_x[number] < OBJECT_LEFT + SIMPSONS_VIDEO_WIDTH + right));
		const bool side = (left || right) && flags
			&& ((x >= OBJECT_LEFT - left && x < OBJECT_LEFT) || (x >= OBJECT_LEFT + SIMPSONS_VIDEO_WIDTH && x < OBJECT_LEFT + SIMPSONS_VIDEO_WIDTH + right));

		const bool was_board = (left || right) && last_flags[number] && last_x[number] >= OBJECT_LEFT
			&& last_x[number] < OBJECT_LEFT + SIMPSONS_VIDEO_WIDTH;
		const bool board = (left || right) && flags && x >= OBJECT_LEFT && x < OBJECT_LEFT + SIMPSONS_VIDEO_WIDTH;
		const bool drawn = (OBJECT(object)->animation_flags & 0x80u) != 0u;

		if ((was_side || was_board) && (!flags || type != last_type[number]))
			fprintf(stderr, "AUDIT frame %llu step %u: gone in %s: object %d type %02X kind %02X state %02X x %04X%s\n",
				(unsigned long long)simpsons_board.frame, (unsigned)game_step, was_side ? "a side" : "the board", number,
				(unsigned)last_type[number], (unsigned)last_kind[number], (unsigned)last_state[number],
				(unsigned)(uint16_t)last_x[number], flags ? " (its slot another's)" : "");
		if (board && drawn && hidden[number] >= 8)
			fprintf(stderr, "AUDIT frame %llu step %u: drawn as it came in to the board, hidden %d frames in a side: object %d "
				"type %02X kind %02X state %02X x %04X\n", (unsigned long long)simpsons_board.frame, (unsigned)game_step,
				hidden[number], number, (unsigned)type, (unsigned)OBJECT(object)->kind, (unsigned)OBJECT(object)->state,
				(unsigned)(uint16_t)x);
		hidden[number] = side && !drawn ? hidden[number] + 1 : (side ? 0 : (board ? 0 : hidden[number]));
		last_flags[number] = flags;
		last_type[number] = type;
		last_kind[number] = OBJECT(object)->kind;
		last_state[number] = OBJECT(object)->state;
		last_x[number] = x;
	}
}

/* ---- stage 3's elevator: its sides of the shaft's own cells (off) ----

   Off (CLIMB_SIDES 0): the elevator shows WIDE's sides black, the picture centred (Sebas, 15-11-2026: "no me gusta",
   "dejemos con esos bordes negros, pero bien centrado"); kept for when the sides have tiles of their own ("deja ese
   arreglo comentado porque tendriamos que hacer unos sprites para eso"). On, it draws them of the shaft's cells:

   The elevator's map has nothing past the cabinet's 288 columns: the game's rows hold the map's blank block there.
   Its sides take the shaft's own cells, as segasonic-port's sides take its map's where the map has none (Sebas:
   "hagamos como hicimos con el segasonic, que completamos con sprites del juego"): each side's blank cells outward,
   row by row from the top, take the repeat of the row that holds where the row meets the side (the bricks every 2
   cells, a beam's cells straight on, the stairs along their slope), so long as it holds for the cell beside it toward
   the picture, for the one above it and for the one under the cell beside it; then, where none does, a blank beside a
   blank, an even fill beside or above it, or the row's best repeat as it is (the K052109's tiles do not flip across
   here: no mirror). What the map draws in the sides (the walkway at the climb's top) stays. Only the picture: the
   elevator is a picture for the objects still, the platform's limits the cabinet's (Sebas: "no hay que extender los
   limites del nivel para que los jugadores no se salgan"). The cells are layer A's in the board's tile RAM, as the
   play's side columns are (the board's state, which every device of a LAN game draws alike); each frame the game's are
   put back before they are drawn again from them, and when the elevator ends. Tried in 20:9 (stage3's script): the
   bricks and the beams go on, the stairs as rows of blocks and a landing's floor as a slab down the side */
#define CLIMB_SIDES         0

static bool climb_now, climb_drawn;    /* (the next frame's picture is the elevator's, and the frame shown's) */
static int held_left = -1;    /* (the play's left side the map has room for, the picture's kept after the elevator) */

#define EARLY_MOST          8            /* the spawns put in ahead of the list in the play's first frame */

static uint16_t early_spawns[EARLY_MOST];    /* (their records: the list skips them as it comes to them,
                                                put_in_sighted_spawns) */
static uint16_t early_objects[EARLY_MOST];    /* (and their objects) */
static int early_count;

#if CLIMB_SIDES
#define CLIMB_ROWS          32           /* the ring's rows, from the one above the screen's first */
#define CLIMB_COLUMNS       64           /* (a ring's: the widest picture's 62 at most) */
#define CLIMB_REPEATS       20
#define CLIMB_PERIOD        8            /* the cells toward the picture a row's repeat is scored on */
#define CLIMB_EVIDENCE      2            /* the fewest of them a repeat holds for, to be taken */
#define TILE_SCROLL_Y_A     0x180Cu      /* the K052109's scroll y of layer A, as one (k052109's update_scroll) */
#define TILE_SCROLL_CONTROL 0x1C80u      /* its scroll by rows or columns (layer A's: bits 0-2) */
#define TILE_ADDRESS_MAP    0x1C00u
#define TILE_BANKS_01       0x1D80u      /* the tile ROM's banks */
#define TILE_BANKS_23       0x1F00u
#define SCREEN_TOP          16           /* the screen's first line in the K052109's (the video's CLIP_TOP) */

enum { CELL_OUT, CELL_SHOWN, CELL_HIDDEN, CELL_DRAWN };

/* A repeat: a cell takes the one 'across' cells toward the picture, and so many rows down (1) or up (-1): along the
   rows, along the stairs' slope, along half of it */
static const int8_t climb_repeats[CLIMB_REPEATS][2] = {
	{ 1, 0 }, { 2, 0 }, { 3, 0 }, { 4, 0 }, { 5, 0 }, { 6, 0 }, { 7, 0 }, { 8, 0 },
	{ 1, -1 }, { 1, 1 }, { 2, -2 }, { 2, 2 }, { 3, -3 }, { 3, 3 }, { 4, -4 }, { 4, 4 },
	{ 2, -1 }, { 2, 1 }, { 4, -2 }, { 4, 2 }
};

static uint8_t climb_marks[32][CLIMB_COLUMNS];    /* the console's cells in layer A's ring */
static uint8_t climb_cells[32][CLIMB_COLUMNS][4];    /* the game's colour and code there, and the console's */

/* The picture's cells: layer A's ring from the row above the screen's first, from the picture's first column */
typedef struct
{
	int columns;
	uint16_t word[CLIMB_ROWS][CLIMB_COLUMNS];    /* colour << 8 | code */
	uint8_t state[CLIMB_ROWS][CLIMB_COLUMNS];
	bool blank[CLIMB_ROWS][CLIMB_COLUMNS];
} climb_grid;

/* A cell's tile has nothing to draw (all of it pen 0): read as the video reads it (simpsons.cpp's tile callback) */
static bool tile_blank(uint16_t word)
{
	const uint8_t *ram = simpsons_board.tile_ram;
	const unsigned which = (unsigned)(word >> 10) & 3u;
	unsigned colour = word >> 8, bank = (ram[which < 2u ? TILE_BANKS_01 : TILE_BANKS_23] >> (4u * (which & 1u))) & 0x0Fu;
	const uint8_t *data;

	if (!(ram[TILE_ADDRESS_MAP] & 0x40u))
		colour = (colour & 0xF3u) | ((bank & 3u) << 2);
	bank >>= 2;
	data = &game_tiles[(((word & 0xFFu) | ((colour & 0x3Fu) << 8) | (bank << 14)) % (GAME_TILES_SIZE / 32u)) * 32u];
	for (int byte = 0; byte < 32; byte++)
		if (data[byte])
			return false;
	return true;
}

static bool climb_known(const climb_grid *grid, int column, int row)
{
	return column >= 0 && column < grid->columns && row >= 0 && row < CLIMB_ROWS
		&& (grid->state[row][column] == CELL_SHOWN || grid->state[row][column] == CELL_DRAWN);
}

/* Two cells look alike: the same, or both blank */
static bool climb_alike(const climb_grid *grid, int column, int row, int other_column, int other_row)
{
	return grid->word[row][column] == grid->word[other_row][other_column]
		|| (grid->blank[row][column] && grid->blank[other_row][other_column]);
}

/* The repeats of a row's side, best first, and their scores: how many cells each holds for where the row meets the
   side (from the cell at 'from' toward the picture, with the rows above and under) */
static void climb_rank(const climb_grid *grid, int row, int from, int step, int *ranked, int *scores)
{
	for (int repeat = 0; repeat < CLIMB_REPEATS; repeat++)
	{
		int index;

		scores[repeat] = 0;
		for (int line = row - 1; line <= row + 1; line++)
			for (int cell = 0; cell < CLIMB_PERIOD; cell++)
			{
				const int here = from - step * cell, there = from - step * (cell + climb_repeats[repeat][0]);
				const int there_line = line + climb_repeats[repeat][1];

				if (climb_known(grid, here, line) && climb_known(grid, there, there_line) && !grid->blank[line][here])
					scores[repeat] += grid->word[line][here] == grid->word[there_line][there];
			}
		for (index = repeat; index > 0 && scores[ranked[index - 1]] < scores[repeat]; index--)
			ranked[index] = ranked[index - 1];
		ranked[index] = repeat;
	}
}

/* A side's cell by the row's repeats: the best one that holds for the cell beside it toward the picture, for the one
   above it and for the one under the cell beside it (a structure's corner ends there); false when none does */
static bool climb_repeat(const climb_grid *grid, int column, int row, int step, const int *ranked, const int *scores,
	uint16_t *word)
{
	const int inner = column - step;
	const bool above_known = climb_known(grid, column, row - 1);

	if (!climb_known(grid, inner, row))
		return false;
	for (int index = 0; index < CLIMB_REPEATS && scores[ranked[index]] >= CLIMB_EVIDENCE; index++)
	{
		const int across = -step * climb_repeats[ranked[index]][0], down = climb_repeats[ranked[index]][1];

		if (!climb_known(grid, column + across, row + down) || !climb_known(grid, inner + across, row + down)
			|| !climb_alike(grid, inner, row, inner + across, row + down))
			continue;
		if (above_known && (!climb_known(grid, column + across, row - 1 + down)
				|| !climb_alike(grid, column, row - 1, column + across, row - 1 + down)))
			continue;
		if (row + 1 < CLIMB_ROWS && grid->state[row + 1][inner] == CELL_SHOWN
			&& (!climb_known(grid, inner + across, row + 1 + down)
				|| !climb_alike(grid, inner, row + 1, inner + across, row + 1 + down)))
			continue;
		*word = grid->word[row + down][column + across];
		return true;
	}
	return false;
}

/* A side's cell no repeat holds for: a blank beside a blank (a structure ends), an even fill beside it or above it,
   else the row's best repeat as it is, else the cell beside it */
static uint16_t climb_other(const climb_grid *grid, int column, int row, int step, const int *ranked)
{
	const int inner = column - step;

	if (climb_known(grid, inner, row) && grid->blank[row][inner])
		return grid->word[row][inner];
	if (climb_known(grid, inner, row) && climb_known(grid, inner, row - 1)
		&& grid->word[row - 1][inner] == grid->word[row][inner])
		return grid->word[row][inner];
	if (climb_known(grid, column, row - 1) && !grid->blank[row - 1][column]
		&& (!climb_known(grid, column, row - 2) || grid->word[row - 2][column] == grid->word[row - 1][column]))
		return grid->word[row - 1][column];
	for (int index = 0; index < CLIMB_REPEATS; index++)
	{
		const int across = -step * climb_repeats[ranked[index]][0], down = climb_repeats[ranked[index]][1];

		if (climb_known(grid, column + across, row + down))
			return grid->word[row + down][column + across];
	}
	return climb_known(grid, inner, row) ? grid->word[row][inner] : grid->word[row][column];
}

/* The game's cells back where the console's are still (the game's rows drawn over them since are the game's) */
static void climb_put_back(void)
{
	const uint16_t tiles = record_of(0).tiles;

	for (int row = 0; row < 32; row++)
		for (int column = 0; column < CLIMB_COLUMNS; column++)
		{
			const uint16_t place = (uint16_t)(tiles + 0x40u * (unsigned)row + (unsigned)column);
			uint8_t *colour = &simpsons_board.tile_ram[place - 0x2000u], *code = &simpsons_board.tile_ram[place];

			if (!climb_marks[row][column])
				continue;
			climb_marks[row][column] = 0;
			if (*colour == climb_cells[row][column][2] && *code == climb_cells[row][column][3])
			{
				*colour = climb_cells[row][column][0];
				*code = climb_cells[row][column][1];
			}
		}
}

/* The climb's sides, 'left' and 'right' of them, drawn of the shaft's own cells (its game's put back first) */
static void climb_fill(int left, int right)
{
	static climb_grid grid;
	const uint16_t tiles = record_of(0).tiles;
	const int x = layer_scroll_x(0);
	const int top = ((((SCREEN_TOP + simpsons_board.tile_ram[TILE_SCROLL_Y_A]) & 0xFF) >> 3) + 31) & 31;
	const int first = (x - left) >> 3, last = (x + SIMPSONS_VIDEO_WIDTH - 1 + right) >> 3;
	const int shown_first = (x + EDGE + TILE - 1) >> 3, shown_last = ((x + SIMPSONS_VIDEO_WIDTH - EDGE) >> 3) - 1;
	const int drawn_first = (x & ~(BLOCK - 1)) >> 3, drawn_last = drawn_first + ROW_BLOCKS * (BLOCK / TILE) - 1;
	int ranked[CLIMB_REPEATS], scores[CLIMB_REPEATS];

	climb_put_back();
	if (simpsons_board.tile_ram[TILE_SCROLL_CONTROL] & 7u)
		return;    /* (layer A scrolled by rows or columns: not the climb's) */
	grid.columns = LEAST(last - first + 1, CLIMB_COLUMNS);
	for (int row = 0; row < CLIMB_ROWS; row++)
		for (int column = 0; column < grid.columns; column++)
		{
			const int map_column = first + column;
			const uint16_t place = (uint16_t)(tiles + 0x40u * (unsigned)((top + row) & 31)
				+ (unsigned)(map_column & (CLIMB_COLUMNS - 1)));
			const uint16_t word = (uint16_t)(simpsons_board.tile_ram[place - 0x2000u] << 8 | simpsons_board.tile_ram[place]);

			grid.word[row][column] = word;
			grid.blank[row][column] = tile_blank(word);
			if (map_column >= shown_first && map_column <= shown_last)
				grid.state[row][column] = CELL_SHOWN;    /* (the cabinet's 288 columns) */
			else if (map_column >= drawn_first && map_column <= drawn_last && !grid.blank[row][column])
				grid.state[row][column] = CELL_SHOWN;    /* (what the map draws in a side: it stays) */
			else
				grid.state[row][column] = CELL_HIDDEN;
		}
	/* (twice row by row from the top: the repeats alone, so a row can take from the rows under it; then the rest) */
	for (int pass = 0; pass < 2; pass++)
		for (int row = 0; row < CLIMB_ROWS; row++)
			for (int side = 0; side < 2; side++)
			{
				const int step = side ? -1 : 1;
				int column = (side ? shown_first : shown_last) - first + step;

				while (column >= 0 && column < grid.columns && grid.state[row][column] != CELL_HIDDEN)
					column += step;
				if (column < 0 || column >= grid.columns)
					continue;
				climb_rank(&grid, row, column - step, step, ranked, scores);
				for (; column >= 0 && column < grid.columns; column += step)
				{
					uint16_t word = grid.word[row][column];

					if (grid.state[row][column] != CELL_HIDDEN)
						continue;
					if (pass == 0 && !climb_repeat(&grid, column, row, step, ranked, scores, &word))
						continue;
					if (pass == 1 && !climb_repeat(&grid, column, row, step, ranked, scores, &word))
						word = climb_other(&grid, column, row, step, ranked);
					grid.word[row][column] = word;
					grid.blank[row][column] = tile_blank(word);
					grid.state[row][column] = CELL_DRAWN;
				}
			}
	for (int row = 0; row < CLIMB_ROWS; row++)
		for (int column = 0; column < grid.columns; column++)
		{
			const int ring_row = (top + row) & 31, ring_column = (first + column) & (CLIMB_COLUMNS - 1);
			const uint16_t place = (uint16_t)(tiles + 0x40u * (unsigned)ring_row + (unsigned)ring_column);
			uint8_t *colour = &simpsons_board.tile_ram[place - 0x2000u], *code = &simpsons_board.tile_ram[place];
			const uint16_t word = grid.word[row][column];

			if (grid.state[row][column] != CELL_DRAWN || (*colour == (word >> 8) && *code == (word & 0xFFu)))
				continue;
			climb_cells[ring_row][ring_column][0] = *colour;
			climb_cells[ring_row][ring_column][1] = *code;
			climb_cells[ring_row][ring_column][2] = (uint8_t)(word >> 8);
			climb_cells[ring_row][ring_column][3] = (uint8_t)word;
			climb_marks[ring_row][ring_column] = 1;
			*colour = (uint8_t)(word >> 8);
			*code = (uint8_t)word;
		}
}
#endif

/* ---- stage 2's boss: its sides of the deck's own cells ----

   Krusty's balloon is fought on the deck at the map's right end, the camera stopped at the boss's place (its check's
   stop): WIDE's sides are all at the left, the map's columns the camera passed, which the cabinet never shows in the
   fight: the fun house's white wall and its porthole beside the deck, the trees and the Ferris wheel, and after the
   boss (stage_2_after_boss: the void's area drawn at the boss's place) the broken deck's columns past its own. Sebas
   saw them in 16:9 ("tendríamos que hacer como hicimos con el segasonic de reconstruir el margen"): while the
   camera is in the boss's room (boss_shown_from), each row of both layers' ring goes on leftwards, from the first map
   column the cabinet can show there, as the repeat of its cells (the period, of 1 to 16 cells, that most of 16 hold for;
   the smaller when alike; the ring's 64 columns room for 32 made, 20:9's 24 or 25), so the deck's planks, its rail and posts, the trees and the sky go on to the picture's left end. The cells
   are the board's tile RAM, as the play's side columns are, made again each frame from the board's columns (which the
   game draws), so a LAN game's devices draw them alike; the floor map stays the map's */
#define BOSS_PERIOD_MOST    16
#define BOSS_SCORED         16
#define RING_COLUMNS        64

static int boss_fill_end[2] = { -1, -1 };    /* (each layer's first map pixel past the cells made: console_wide_check) */

/* The camera's room for the boss: its stage entry stops it at the right within a screen of the map's end (the boss's
   place, 074F), and going back at the left (0700: a player walking left brings it there, Sebas: "te pasa porque tenés
   a Bart sobre toda la parte derecha del mapa"). What the cabinet can show of the map begins at that left stop past its
   black edge (0710): the map pixel the sides are made of the cells from, wherever the camera is in its room, so
   nothing changes as it moves; -1 out of the fight */
static int boss_shown_from(void)
{
	const int data = rom16(STAGE_RECORDS_BANK, (uint16_t)(stage_record_entry + 2u));
	const layer_map_area map = rings[camera_main_layer ? 1 : 0].known ? map_of(camera_main_layer ? 1 : 0)
		: (layer_map_area){ 0 };
	int right, left;

	if (game_step != STAGE_2_STEP || wide_left_now <= 0 || data < 0 || !map.known
		|| rom8(STAGE_RECORDS_BANK, (uint16_t)data) != (int)CHECK_STOP)
		return -1;
	right = rom16(STAGE_RECORDS_BANK, (uint16_t)(data + 1));
	if (right < 0 || map.width - (right + SIMPSONS_VIDEO_WIDTH) >= SIMPSONS_VIDEO_WIDTH)
		return -1;    /* (a stop short of the map's end: the play's, where the map left of it was shown) */
	left = rom8(STAGE_RECORDS_BANK, (uint16_t)(data + 3)) == (int)CHECK_STOP
		? rom16(STAGE_RECORDS_BANK, (uint16_t)(data + 4)) : right;
	return (left < 0 ? right : LEAST(left, right)) + fixed_first;
}

static void boss_side_fill(int shown_from)
{
	for (int layer = 0; layer < 2; layer++)
	{
		const uint16_t tiles = record_of(layer).tiles;
		const int x = layer_scroll_x(layer);
		const int board = (x + shown_from - (int)(uint16_t)camera_x + TILE - 1) >> 3, first = (x - wide_left_now) >> 3;

		boss_fill_end[layer] = -1;
		if (!rings[layer].known || first >= board || board - first > RING_COLUMNS - BOSS_SCORED - BOSS_PERIOD_MOST)
			continue;
		boss_fill_end[layer] = board * TILE;
		for (int row = 0; row < 32; row++)
		{
			uint8_t *colour = &simpsons_board.tile_ram[tiles + 0x40u * (unsigned)row - 0x2000u];
			uint8_t *code = &simpsons_board.tile_ram[tiles + 0x40u * (unsigned)row];
			int best = 1, best_score = -1;

			for (int period = 1; period <= BOSS_PERIOD_MOST; period++)
			{
				int score = 0;

				for (int cell = board; cell < board + BOSS_SCORED; cell++)
				{
					const int here = cell & (RING_COLUMNS - 1), there = (cell + period) & (RING_COLUMNS - 1);

					score += colour[here] == colour[there] && code[here] == code[there];
				}
				if (score > best_score)
				{
					best = period;
					best_score = score;
				}
			}
			for (int column = board - 1; column >= first; column--)
			{
				const int here = column & (RING_COLUMNS - 1), there = (column + best) & (RING_COLUMNS - 1);

				colour[here] = colour[there];
				code[here] = code[there];
			}
		}
	}
}

static void settle_moved_spawns(void);

void console_wide_frame(void)
{
	const int side = k_sides[wide_kind()];
	int left = 0, right = 0;

	if (game_step != last_step || game_mode != last_mode)
	{
		forget_rings();    /* (another step: its maps to come, */
		early_count = 0;    /* its spawn list) */
	}
	settle_moved_spawns();    /* (the spawns moved out of sight that are not enemies: back at their places) */
	if (floor_camera_seen >= 0 && (int)camera_main_layer != floor_camera_seen)
		floor_layer = -1;    /* (the camera on another layer: the game's floor columns of it from now on) */
	floor_camera_seen = camera_main_layer;
	last_step = game_step;
	last_mode = game_mode;
	shown_drawn = shown_now;
	climb_drawn = climb_now;
	black_drawn = black_now;
	shown_now = what_shows();
	black_now = shown_now == SHOW_PICTURE && closing_story();
	fixed_first = edges_masked() ? EDGE : 0;
	fixed_last = SIMPSONS_VIDEO_WIDTH - 1 - fixed_first;
	if (shown_now == SHOW_PLAY)
	{
#if CLIMB_SIDES
		climb_put_back();    /* (the elevator's sides gone: the game's cells, the map's) */
#endif
		climb_now = false;
		play_sides(side, &left, &right);
		if ((!wide_left_now && !wide_right_now) || (shown_drawn != SHOW_PLAY && !climb_drawn))
		{
			wide_left_now = left;    /* (the sides come, or the play does: at once) */
			wide_right_now = right;
			held_left = -1;
		}
		else if ((climb_drawn || held_left >= 0) && left < wide_left_now)
			held_left = left;    /* (after the elevator's picture, the play at the shaft's top: the picture kept centred,
			                        black past the map's left end as the elevator's sides were, till the camera brings the
			                        map that far; the objects' left side the map's, play_sides_now) */
		else
		{
			wide_left_now = pan_to(wide_left_now, left);
			wide_right_now = pan_to(wide_right_now, right);
			held_left = -1;
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
			if (layer == 1 && helicopter_lift())
				simpsons_video_set_layer_window(1 + layer, fixed_first, fixed_last);    /* (stage 7's lift-off: layer B
					in the board's columns alone) */
			else
				simpsons_video_set_layer_window(1 + layer, -x, map.width - 1 - x);    /* (nothing of it past its ends) */
		}
		const int boss_from = boss_shown_from();

		if (boss_from >= 0)
			boss_side_fill(boss_from);    /* (stage 2's boss: its sides of the deck's own cells) */
		else
			boss_fill_end[0] = boss_fill_end[1] = -1;
		simpsons_video_set_fixed((wide_right_now - wide_left_now) / 2, fixed_first, fixed_last);
	}
	else
	{
		/* (stage 3's elevator, a stage's play drawn as a picture, centred as the other pictures, its sides black, from
		   the platform's standing on: no jump as it begins, the step's change before it a picture too, and the play
		   after it held there, climb_drawn; Sebas: "dejemos con esos bordes negros, pero bien centrado". Another play
		   drawn as a picture, stage 8's start, keeps the picture where the play had it: the board's part in the same
		   place and the players' boxes too) */
		const bool climb = shown_now == SHOW_PICTURE && in_play()
			&& (elevator() || (climb_drawn && game_step == ELEVATOR_STEP));    /* (till its play comes: its scroll's step
			                                                                     5 may begin with a frame of its picture) */
		const bool kept = shown_now == SHOW_PICTURE && in_play() && !climb;

		if (!kept || shown_drawn == SHOW_BOARD)
			wide_left_now = wide_right_now = shown_now == SHOW_PICTURE ? side : 0;
		for (int layer = 0; layer < 2; layer++)
		{
			const layer_map_area map = rings[layer].known && in_play() ? map_of(layer) : (layer_map_area){ 0 };

			if (map.known && map.width > RING)
			{
				follow_ring(layer);    /* (a play drawn as a picture: its rings kept as its scroll goes, for the play
				                          after it) */
				if (climb && !(edges_on(layer) && layer_rows_on))
					draw_side_columns(layer, &map, wide_left_now, wide_right_now);    /* (and the sides of its layers that
						do not scroll up, under the black, as a play draws them: the columns under the cabinet's black edges,
						which the game leaves as they were, its map's for the play after it) */
			}
			simpsons_video_set_layer_window(1 + layer, -0x10000, 0x10000);
		}
		if (climb)
			floor_back_to_board();    /* (the elevator's floor map the board's, cell for cell: its platform alone, the
				rest walls. WIDE's cells of the cemetery's map stayed beside the platform, and a goon jumping in from the right
				landed on them, off the platform, unseen, and fell in from above: the players' issue 35) */
		climb_now = climb && (wide_left_now || wide_right_now);
#if CLIMB_SIDES
		if (climb_now)
			climb_fill(wide_left_now, wide_right_now);    /* (its sides of the shaft's own cells) */
		else
			climb_put_back();
#endif
		simpsons_video_set_fixed(kept ? (wide_right_now - wide_left_now) / 2 : 0, fixed_first, fixed_last);
	}
	simpsons_video_set_wide(wide_left_now, wide_right_now);
	wide_screen_sides();    /* (the game's limits for the next frame) */
	audit_frame(shown_now == SHOW_PLAY ? wide_left_now : 0, shown_now == SHOW_PLAY ? wide_right_now : 0);
	if (debug_on())
	{
		const int data = rom16(STAGE_RECORDS_BANK, (uint16_t)(stage_record_entry + 2u));
		const layer_map_area map_a = map_of(0), map_b = map_of(1);

		fprintf(stderr, "frame %llu: show %d sides %d %d step %u phase %u/%u scroll %u held %d scene %02X/%u main %u "
			"camera %04X,%04X "
			"A %04X,%04X ring %d %d..%d w %d  B %04X,%04X ring %d %d..%d w %d edges %u%u%u%u entry %04X x %04X right %d %04X "
			"left %d %04X\n",
			(unsigned long long)simpsons_board.frame, shown_now, wide_left_now, wide_right_now, (unsigned)game_step,
			(unsigned)step_phase, (unsigned)mode_phase, (unsigned)stage_scroll_step, held_left, (unsigned)map_scene,
			(unsigned)map_area,
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

/* A column's colour, when nearly all of its rows have it (else black); 'most', the one most of its rows have and how
   many */
static uint32_t edge_colour(const console_picture *frame, int x, uint32_t *most, int *rows)
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
	*most = kinds ? colours[best] : 0x000000u;
	*rows = kinds ? counts[best] : 0;
	return kinds && counts[best] * 10 >= SIMPSONS_VIDEO_HEIGHT * 9 ? colours[best] : 0x000000u;
}

/* The sides' colour as a picture goes on: kept while both edges are still mostly it, whatever crosses them (the name
   entry's Maggies walking along its bottom took a tenth of its edges' rows, and its brown sides went black and back as
   each one passed: the players' issue 36, "this screen still has strange flashing") */
#define SIDES_KEPT_ROWS     (SIMPSONS_VIDEO_HEIGHT * 2 / 3)
static uint32_t sides_kept;
static bool sides_kept_on;

/* Over the frame the video drew: a picture's sides (and the board's edges the cabinet showed black) in its edges'
   colour, or black */
void wide_picture_sides(console_picture *frame)
{
	const int first = fixed_first ? fixed_first : 0, last = SIMPSONS_VIDEO_WIDTH - 1 - first;

	if (shown_drawn != SHOW_PICTURE || (CLIMB_SIDES && climb_drawn) || (!frame->left && !frame->right))
	{
		sides_kept_on = false;
		return;    /* (stage 3's elevator, CLIMB_SIDES: its sides of the shaft's own cells, climb_fill) */
	}
	{
		uint32_t left_most, right_most;
		int left_rows, right_rows;
		const uint32_t left = edge_colour(frame, first, &left_most, &left_rows);
		const uint32_t right = edge_colour(frame, last, &right_most, &right_rows);
		/* (both sides one colour only when both edges are it, a backdrop: the TV's orange, the cards' lilac; a scene
		   with one edge of one colour, Bart's profile's green, as 4:3 on black, the players' issue 24) */
		uint32_t sides = left == right ? left : 0x000000u;

		if (black_drawn)
			sides = 0x000000u;
		else if (sides != 0x000000u)
		{
			sides_kept = sides;
			sides_kept_on = true;
		}
		else if (sides_kept_on && left_most == sides_kept && right_most == sides_kept && left_rows >= SIDES_KEPT_ROWS
			&& right_rows >= SIDES_KEPT_ROWS)
			sides = sides_kept;
		else
			sides_kept_on = false;

		draw_fill(frame, PICTURE_FIRST(frame), 0, first - PICTURE_FIRST(frame), SIMPSONS_VIDEO_HEIGHT, sides);
		draw_fill(frame, last + 1, 0, PICTURE_END(frame) - last - 1, SIMPSONS_VIDEO_HEIGHT, sides);
	}
}

/* The players' boxes at the top of the play (the fixed layer's): 72 pixels wide, each with its white side at its left,
   the board's 16, 88, 160 and 232, from row 8 to row 26. The fourth's right side would be the board's 304, under the
   cabinet's black edge (edges_masked): the game never draws it, as the cabinet showed no more */
#define HUD_TOP             8
#define HUD_BOTTOM          26
#define HUD_LAST_SIDE       232
#define HUD_BOX_WIDTH       72

static bool hud_white(const console_picture *frame, int x, int y)
{
	const uint32_t colour = frame->pixels[y * frame->stride + frame->left + x];

	return (colour >> 16 & 0xFFu) >= 0xF0u && (colour >> 8 & 0xFFu) >= 0xF0u && (colour & 0xFFu) >= 0xF0u;
}

/* Over the frame the video drew, in WIDE's play and its pictures (the character choice's, whose boxes are the same;
   not a cutscene shown as the cabinet's, black_drawn): the fourth player's box (Lisa's) closed at its right as the
   others are, the side the boxes share copied there, when the boxes are drawn (the players' issue 29: "the HUD for
   Lisa should have a white, single pixel-wide strip added to the right side"; issue 32, the choice's: "still missing
   the right-side pixel border") */
void wide_hud_closed(console_picture *frame)
{
	const int dx = (frame->right - frame->left) / 2;    /* (the fixed layer's, centred: console_wide_frame) */
	const int side = HUD_LAST_SIDE + dx, closing = side + HUD_BOX_WIDTH;

	if (shown_drawn == SHOW_BOARD || black_drawn || fixed_first != EDGE || side < PICTURE_FIRST(frame)
		|| closing >= PICTURE_END(frame))
		return;
	for (int y = HUD_TOP; y < HUD_BOTTOM; y++)
		if (!hud_white(frame, side, y))
			return;
	if (!hud_white(frame, closing - 1, HUD_TOP) || !hud_white(frame, closing - 1, HUD_BOTTOM))
		return;    /* (the box's top and bottom reaching its end) */
	/* (from the row under the box's top: the boxes' corners are left out, as the name's tab's are, the side under
	   each tab's corner beginning a row below it; the players' issue 32, "the top-most pixel removed". Row 26 the
	   shared side's shadow, grey: as the others') */
	for (int y = HUD_TOP + 1; y <= HUD_BOTTOM; y++)
		frame->pixels[y * frame->stride + frame->left + closing] = frame->pixels[y * frame->stride + frame->left + side];
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
		*left = held_left >= 0 ? LEAST(wide_left_now, held_left) : wide_left_now;    /* (the picture held centred after
		                                                                                 the elevator: the map's room) */
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

/* The game's screen limits (support.h's PAST_LEFT, PAST_RIGHT, SCREEN_SPAN, X_BETWEEN: the board's edges, object x
   0x40 and 0x180, and a margin) moved by these: WIDE's sides in a stage's play as play_now has them, 0 elsewhere and
   with WIDE off (the arcade's own limits). Set at each frame's end for the next one and again as the game mode's
   frame begins (console.c), so the frame a play begins in has the sides it comes with */
int screen_more_left, screen_more_right;

void wide_screen_sides(void)
{
	play_now(&screen_more_left, &screen_more_right);
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

/* An object's sprites' priority against the layers taken again from the floor map where it stands now, as the game's
   start_object_from_setup takes it where it starts: a spawn moved past WIDE's side started on a cell that is not its
   place's (stage 2's mailbox by Otto's stall, put in at 01F0, stood behind the layers back at 0180: unseen, yet hiding
   the sprites behind it, the players' issue 29). The floor's scratch the game keeps in its RAM (48E8-48EE: the
   offsets, their signs, the column and the place) is left as it was */
#define FLOOR_SCRATCH       0x48E8u
#define FLOOR_SCRATCH_SIZE  7

static void floor_priority_again(uint16_t object)
{
	uint8_t scratch[FLOOR_SCRATCH_SIZE];
	uint16_t cell = 0u;
	uint32_t no_floor = 0u;

	for (int index = 0; index < FLOOR_SCRATCH_SIZE; index++)
		scratch[index] = mem_read8((uint16_t)(FLOOR_SCRATCH + index));
	floor_at_offset(object, &cell, &no_floor);
	OBJECT(object)->attributes = (uint8_t)(((cell >> 8) & 0x30u) | (OBJECT(object)->attributes & 0xCFu));
	for (int index = 0; index < FLOOR_SCRATCH_SIZE; index++)
		mem_write8((uint16_t)(FLOOR_SCRATCH + index), scratch[index]);
}

/* At each frame's end: what a moved spawn put in at its place this frame, moved with it; a moved spawn that is an
   enemy kept where it was moved, one that is not back at its place after MOVED_FRAMES (or gone: forgotten), with its
   sprites' priority of its place's floor (floor_priority_again) */
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
		{
			OBJECT(object)->x = (uint16_t)(x - moved_spawns[index].moved);
			floor_priority_again(object);
		}
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

/* A spawn due as on the board, not as far earlier as WIDE's right side is wide: stage 2's teacup ride's hub (list 1's
   type 0F, teacup_goon_ride_update), which takes itself off past the board's right side (its x 1E0 on, and its start
   puts it 40 pixels on from its place: 1D0 on the board, 200 put in by WIDE's side), so in WIDE it was gone in its
   first frame and its arms and the goons riding them never came (a player's report: "the enemies inside the cups
   (when you get the hammer) are not showing up"). Those after it in the list wait for it, as on the board */
#define RIDE_HUB_KIND       0x0Fu
#define SPAWN_LIST          0x0Cu        /* a spawn's flags: its object's list (take_free_object_for_spawn), */
#define SPAWN_LIST_1        0x04u        /* list 1 */

static bool due_as_on_board(uint16_t spawn)
{
	return SPAWN(spawn)->kind == RIDE_HUB_KIND && (SPAWN(spawn)->flags & SPAWN_LIST) == SPAWN_LIST_1;
}

/* A spawn put in as the game's spawn_stage_objects puts it in (an object of its list free for it, or none), at its
   place moved by WIDE's sides ('wide'), else at the board's: the object, 0 if none was free */
static uint16_t put_in_spawn(uint16_t spawn, bool wide, int side_left, int side_right, int picture_left,
	int picture_right)
{
	uint32_t carry = 0;
	uint8_t spawn_flags = 0, spawn_height = 0, flags_2 = 0, kind = 0, state = 0, flags_3 = 0, height_steps = 0, depth = 0;
	uint16_t spawned = 0, spawn_stage_x = 0, camera_offset = 0, spawn_screen_x = 0, placed_x = 0, left_side_x = 0,
		signed_steps = 0, height_pixels = 0, negated_height = 0, height_on_screen = 0, spawn_y = 0;

	flags_2 = SPAWN(spawn)->flags;
	spawn_flags = flags_2;
	take_free_object_for_spawn(spawn_flags, &spawned, &carry);
	if (carry == 0u)
		return 0u;
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
		left_side_x = wide ? past_left_side(placed_x, side_left, side_right, picture_left)    /* (WIDE: past its
			left side, from the right side's distance; where the board has it if the cabinet shows it there) */
			: (uint16_t)(placed_x - 0x160u);
		OBJECT(spawned)->x = left_side_x;
	}
	else if (wide && in_cemetery == 0u && comes_in_type(kind))
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
	return spawned;
}

/* The play's first frame, after the board's spawns: the spawns due in the wider screen's right side that stand in its
   sight, put in ahead of the list (a stage's start: start_stage_objects reads the list once, the game's play frames
   come only after its opening; stage 2's Otto at his stall in 20:9 came as the players dropped from their balloon,
   the players' issue 16). Not those that come in by themselves or from the left: the list puts them in as it comes to
   them, after the opening, whose own objects take their slots meanwhile (stage 2's balloon lost two of its own when the
   goons came in with Otto) */
static void put_in_sighted_spawns(int side_left, int side_right, int picture_left, int picture_right)
{
	uint16_t spawned;

	early_count = 0;
	window_bank = 0x1Bu;    /* (the spawn lists' bank, as the game's spawn_stage_objects reads them) */
	set_lines(window_bank);
	for (uint16_t spawn = spawn_next; early_count < EARLY_MOST; spawn = (uint16_t)(spawn + 8u))
	{
		const int screen_x = (int16_t)(uint16_t)(SPAWN(spawn)->x - camera_x + 0x40u);

		if ((uint8_t)(SPAWN(spawn)->x >> 8u) == 0xFFu
			|| (uint16_t)(camera_x + 0x150u + (unsigned)side_right) < SPAWN(spawn)->x)
			break;
		if (difficulty < SPAWN(spawn)->level || (int8_t)SPAWN(spawn)->flags < 0 || comes_in_type(SPAWN(spawn)->kind)
			|| due_as_on_board(spawn) || screen_x - SIGHT_HALF >= OBJECT_RIGHT + side_right)
			continue;    /* (not one that stands in the wider screen's sight) */
		if ((spawned = put_in_spawn(spawn, true, side_left, side_right, picture_left, picture_right)) != 0u)
		{
			early_spawns[early_count] = spawn;
			early_objects[early_count++] = spawned;
			if (debug_on())
				fprintf(stderr, "frame %llu: put in ahead: object %04X type %02X x %04X\n",
					(unsigned long long)simpsons_board.frame, (unsigned)spawned, (unsigned)OBJECT(spawned)->type,
					(unsigned)OBJECT(spawned)->x);
		}
	}
}

/* A spawn put in ahead of the list (put_in_sighted_spawns), as the list comes to it (no longer ahead): the list skips
   it ('wide'), and with WIDE turned off meanwhile, while it is there: one the game has taken off past the board's
   screen the list puts in again (stage 2's Otto, taken off as the play began, at his stall as the cabinet's camera
   comes). In WIDE one that took itself off is not put in again (stage 2's E9, gone in its first frame) */
static bool put_in_early(uint16_t spawn, bool wide)
{
	for (int index = 0; index < early_count; index++)
		if (early_spawns[index] == spawn)
		{
			const uint16_t object = early_objects[index];
			const bool there = OBJECT(object)->flags != 0u && OBJECT(object)->type == SPAWN(spawn)->kind;

			early_spawns[index] = early_spawns[--early_count];
			early_objects[index] = early_objects[early_count];
			return wide || there;
		}
	return false;
}

/* The game's spawn_stage_objects (objects/spawning.c), the spawns due as far past the wider screen's right side as
   past the board's, put in at their place; those that come in from the left as far past its left side; in the play's
   first frame, the board's spawns and those that stand in the wider screen's sight (put_in_sighted_spawns) */
void spawn_stage_objects(void)
{
	uint32_t carry = 0;
	uint8_t enemy_speed = 0, flags = 0, level = 0;
	uint16_t spawn = 0, spawned = 0;
	uint16_t enemies_speed = 0, camera_position = 0, spawn_edge = 0, scroll_position = 0, spawn_place = 0, compared_scroll = 0;
	int side_left, side_right, picture_left, picture_right;

	if (in_cemetery == 0u && play_begins(&picture_left, &picture_right))
	{
		spawn_as_play_begins(picture_left, picture_right);    /* (the play's first frame: the board's spawns, */
		put_in_sighted_spawns(MOST(picture_left, 0), MOST(picture_right, 0), picture_left, picture_right);    /* then
			those that stand in the wider screen's sight) */
		return;
	}
	play_sides_now(&picture_left, &picture_right);
	side_left = MOST(picture_left, 0);
	side_right = MOST(picture_right, 0);
	if (!side_left && !side_right && !early_count)
	{
		original_spawn_stage_objects();
		return;
	}    /* (and with some put in ahead of the list, WIDE turned off meanwhile: the board's, skipping them) */
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
			spawn_edge = (uint16_t)(camera_position + 0x150u
				+ (due_as_on_board(spawn) ? 0u : (unsigned)side_right));    /* (WIDE: its right side) */
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
		if (put_in_early(spawn, side_left || side_right))
		{
			spawn_next = spawn + 8u;    /* (put in ahead of the list already: the play's first frame) */
			continue;
		}
		spawned = put_in_spawn(spawn, side_left || side_right, side_left, side_right, picture_left, picture_right);
		if (spawned && (side_left || side_right))
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
/* Whether an object is held by its carrier, still in play beside it: stage 2's fat goons coming in from the left
   past WIDE's left side pick up the obstacle they carry on their heads (fat_goon_thrower_pick_up) some pixels past
   it, where the board's has it right at its own limit; it was taken off at once and the goon walked in holding
   nothing (the players' issue 29). Thrown, it leaves its carrier and goes out as on the board */
#define HELD_REACH          0x40         /* (on its head, as it walks: some 0x10 to 0x23 apart) */

static bool held_in_sight(uint16_t object)
{
	if (OBJECT(object)->carried == 0u)
		return false;
	for (int number = 0; number < OBJECTS; number++)
	{
		/* (its carrier keeps it in its own fields, not in the obstacle's parent) */
		const uint16_t carrier = (uint16_t)OBJECT_ADDRESS(number);
		const int apart = (int16_t)OBJECT(object)->x - (int16_t)OBJECT(carrier)->x;

		if (carrier != object && OBJECT(carrier)->flags != 0u
			&& OBJECT(carrier)->fat_goon_thrower.carried_object == object && apart <= HELD_REACH && apart >= -HELD_REACH)
			return true;
	}
	return false;
}

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
		carry = (object_x >= OBJECT_LEFT - side_left && object_x < 0x1C0 + side_right) || held_in_sight(object);
		if (!carry)
			OBJECT(object)->flags = 0u;
	}
	return carry;
}

/* The game's prop_leave (objects/stage_props.c): a prop (list 1 type 3D: the stages' scenery objects) is taken away
   once the scroll brings its x short of its leave x, which its entry in the props table gives (a byte times 16: 04 is
   0040, the board's left edge), not a limit in the code (support.h's PAST_LEFT): stage 4's in 20:9 went at x 003F,
   in WIDE's left side in sight (SIMPSONS_WIDE_AUDIT). In WIDE: past the leave x less the left side, signed (the
   board's compare is unsigned, the same for the x it reaches there) */
void prop_leave(uint16_t object)
{
	int side_left, side_right;

	play_now(&side_left, &side_right);
	if (!side_left)
	{
		original_prop_leave(object);
		return;
	}
	if ((int16_t)OBJECT(object)->x < (int)OBJECT(object)->prop.leave_x - side_left)
		OBJECT(object)->flags = 0u;
}

/* The game's carried_obstacle_update (objects/bank_entries.c): the obstacle a fat goon carries on his head (list 1
   type 28) is removed past the board's sides by a check of its own (x short of 0010 or from 01B0 on), not by
   remove_off_screen. A goon coming in past WIDE's left side puts it in at x 0000 or so: it was removed in its first
   frame, and the goon walked in holding nothing; its slot free, the next object put in took it (stage 2's mailbox),
   and the goon, holding that slot still, carried the mailbox to his head and threw it across the screen (the players'
   issue 29). In WIDE: past the wider screen's sides, and not while its goon holds it (held_in_sight) */
#define CARRIED_LEFT        0x10
#define CARRIED_RIGHT       0x1B0

uint16_t carried_obstacle_update(uint16_t object, uint8_t state)
{
	uint16_t states = carried_obstacle_states_ADDRESS;
	int side_left, side_right;

	play_now(&side_left, &side_right);
	if (!side_left && !side_right)
		return original_carried_obstacle_update(object, state);
	switch ((int8_t)(uint8_t)((uint32_t)state << 1))    /* (the game's, as it is) */
	{
	case 0:
	case 18:
		states = carried_obstacle_start(object);
		break;
	case 2:
		states = carried_obstacle_held(object);
		break;
	case 4:
		carried_obstacle_thrown(object);
		break;
	case 6:
	case 24:
		carried_obstacle_fly(object);
		break;
	case 8:
	case 26:
		carried_obstacle_bounce(object);
		break;
	case 10:
	case 28:
		carried_obstacle_land(object);
		break;
	case 12:
	case 30:
		carried_obstacle_slide(object);
		break;
	case -4:
	case 14:
		states = carried_obstacle_break_steps(object);
		break;
	case -2:
	case 16:
		states = carried_obstacle_drop_steps(object);
		break;
	case 20:
		states = tossed_obstacle_held(object);
		break;
	case 22:
		tossed_obstacle_toss(object);
		break;
	}
	states = object_take_special_hit(object, states);
	carried_obstacle_check_floor(object);
	if (OBJECT(object)->flags != 0u)
	{
		const int x = (int16_t)OBJECT(object)->x;

		if ((x < CARRIED_LEFT - side_left || x >= CARRIED_RIGHT + side_right) && !held_in_sight(object))
			OBJECT(object)->flags = 0u;
	}
	return states;
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
/* An obstacle's start (list 1's type 00: stage 2's mailbox by Otto's stall), which waits to put itself up (2 pixels
   up, shown, the floor under it raised by its height) till its x is short of 0180, the board's right side's sight:
   in WIDE, short of the wider screen's right side's. As stage 2 began its mailbox stood at 0180, past the board's
   but in WIDE's sight: unseen, and its floor not raised, till the scroll brought it in (the players' issue 29) */
#define OBSTACLE_SHOWN      0x180

void obstacle_start(uint16_t object, uint16_t *p_u, uint32_t *p_left_caller)
{
	const int x = (int16_t)OBJECT(object)->x;
	int side_left, side_right;

	play_now(&side_left, &side_right);
	if (OBJECT(object)->action != 0u && x >= OBSTACLE_SHOWN && x < OBSTACLE_SHOWN + side_right)
	{
		/* (the board's own branch, short of 0180) */
		const uint16_t moved_y = (uint16_t)(OBJECT(object)->y - 2u);

		OBJECT(object)->y = moved_y;
		OBJECT(object)->animation_flags = 0x80u;
		obstacle_raise_floor(object, (uint16_t)((moved_y & 0xFFu) | ((uint16_t)OBJECT(object)->obstacle.height << 8)),
			p_u, p_left_caller);
		if (*p_left_caller != 0u)
			*p_left_caller = 0u;
		else
			OBJECT(object)->state = (uint8_t)(OBJECT(object)->state + 1u);
		return;
	}
	original_obstacle_start(object, p_u, p_left_caller);
}

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

/* A scene's actor (not a player) going out through a side leaves the picture when it left the cabinet's screen: past
   the edge the board showed, each frame's move taken further by the side's width over what the board had left to go
   (to its 0 or 018F, story_actor_finish_frame). Stage 7's end: Smithers runs off at 1 pixel a frame past the camera
   following him; the board takes him off 32 pixels past its edge, unseen, and WIDE only past its side, so he was still
   in it as the helicopter came in with him aboard (the players' issue 34) */
#define STORY_LEFT_GONE     0x0000       /* (story_actor_finish_frame's limits) */
#define STORY_RIGHT_GONE    0x018F
#define STORY_PLAYERS       8            /* (objects 0-7: the players and theirs, who walk as they do) */

static int16_t story_last_x[STORY_OBJECT];
static uint64_t story_last_frame[STORY_OBJECT];
static int story_more[STORY_OBJECT];    /* (sixteenths of a pixel carried to the next frame) */

static void story_actor_out_as_board(uint16_t object)
{
	const int number = object >= OBJECT_ADDRESS(0) ? (int)((object - OBJECT_ADDRESS(0)) / 0x60u) : -1;
	const int seen_left = OBJECT_LEFT + fixed_first + SIGHT_HALF;    /* (its sprite beginning to leave the board's sight,
		its black edges, which take 2 * SIGHT_HALF to cross; in the picture the side and those edges more) */
	const int seen_right = OBJECT_RIGHT - fixed_first - SIGHT_HALF;
	const int gone = 2 * SIGHT_HALF;
	int x, moved, picture_left, picture_right, more = 0;
	bool followed;

	if (number < STORY_PLAYERS || number >= STORY_OBJECT || (object - OBJECT_ADDRESS(0)) % 0x60u)
		return;    /* (the players, the scene's own actor and the children's slots elsewhere) */
	x = (int16_t)OBJECT(object)->x;
	followed = story_last_frame[number] + 1u == simpsons_board.frame;
	moved = x - story_last_x[number];
	story_last_frame[number] = simpsons_board.frame;
	if (!followed)
	{
		story_last_x[number] = (int16_t)x;
		story_more[number] = 0;
		return;
	}
	play_sides_now(&picture_left, &picture_right);
	if (moved > 0 && x > seen_right && picture_right > 0)
		more = story_more[number] + moved * (picture_right + fixed_first) * 16 / gone;
	else if (moved < 0 && x < seen_left && picture_left > 0)
		more = story_more[number] + moved * (picture_left + fixed_first) * 16 / gone;
	else
		story_more[number] = 0;
	if (more)
	{
		OBJECT(object)->x = (uint16_t)(x + more / 16);
		story_more[number] = more % 16;
	}
	story_last_x[number] = (int16_t)OBJECT(object)->x;
}

/* The game's story_actor_finish_frame (scenes/story_actor.c): a scene's actor taken off the screen past the wider
   screen's sides, not in them (stage 1's opening: Smithers' goons gone where the cabinet's screen ended; Sebas,
   "Desaparece en 4:3"), and going out through them as fast as it left the board's screen (story_actor_out_as_board) */
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
	story_actor_out_as_board(object);    /* (WIDE: out as on the board) */
	actor_x = OBJECT(object)->x;
	x_now = actor_x;
	if (!beyond_limits((int16_t)x_now, STORY_LEFT_GONE, STORY_RIGHT_GONE))    /* (WIDE: its sides) */
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
			if (column < 0 || column + TILE > map.width || column < boss_fill_end[layer])
				continue;    /* (and stage 2's boss's sides, of the deck's own cells: boss_side_fill) */
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
	early_count = 0;
	climb_now = climb_drawn = false;
	held_left = -1;
#if CLIMB_SIDES
	memset(climb_marks, 0, sizeof(climb_marks));
#endif
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
	simpsons_state_bytes(state, early_spawns, sizeof(early_spawns));
	simpsons_state_bytes(state, early_objects, sizeof(early_objects));
	simpsons_state_bytes(state, &early_count, sizeof(early_count));
	simpsons_state_bytes(state, &fixed_first, sizeof(fixed_first));
	simpsons_state_bytes(state, &fixed_last, sizeof(fixed_last));
	simpsons_state_bytes(state, &last_step, sizeof(last_step));
	simpsons_state_bytes(state, &last_mode, sizeof(last_mode));
	simpsons_state_bytes(state, &shown_now, sizeof(shown_now));
	simpsons_state_bytes(state, &shown_drawn, sizeof(shown_drawn));
	simpsons_state_bytes(state, &wide_left_now, sizeof(wide_left_now));
	simpsons_state_bytes(state, &wide_right_now, sizeof(wide_right_now));
	simpsons_state_bytes(state, &climb_now, sizeof(climb_now));
	simpsons_state_bytes(state, &climb_drawn, sizeof(climb_drawn));
	simpsons_state_bytes(state, &held_left, sizeof(held_left));
#if CLIMB_SIDES
	simpsons_state_bytes(state, climb_marks, sizeof(climb_marks));
	simpsons_state_bytes(state, climb_cells, sizeof(climb_cells));
#endif
	simpsons_state_bytes(state, &kind, sizeof(kind));
	if (simpsons_state_loading(state) && kind != wide_kind())
	{
		wide_left_now = wide_right_now = 0;
		held_left = -1;
		shown_now = SHOW_BOARD;
		simpsons_video_set_wide(0, 0);
		simpsons_video_set_fixed(0, fixed_first, fixed_last);
		for (int layer = 0; layer < 2; layer++)
			simpsons_video_set_layer_window(1 + layer, -0x10000, 0x10000);
	}
}
