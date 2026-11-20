/**
 *    Name:        bart_shirt.c
 *    Description: Bart's shirt in the show's orange, and Barney's in a salmon: the colours the video draws their
 *                 shirts' pens in, the board's palette and every other pixel as the cabinet has them
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    15-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    Sebas: "vamos a cambiarle el color de la remera a bart por la naranja" (console/PLAN.md, guideline 18), and on
 *    seeing it, "hay que agregar un tono de naranja mas oscuro a las mangas" and "cuando lo agarran, es azul". The
 *    cabinet dresses Bart in light blue (xBGR 7F00: 0, 24, 31), shaded with his shorts' blue (pen 8, 7A40: 0, 18, 30);
 *    the one place the game draws him in the show's colours is the attract mode's opening, Bart skating out of school
 *    in an orange shirt (115F: 31, 10, 4, the palette's pen 8 there; his shorts there are the light blue). That orange
 *    is the one drawn here, and its shade the orange as much darker as the cabinet's shade is than its light blue.
 *
 *    The four characters share one palette (the players' sprites' colour 0: pen 7 is Bart's shirt and also the colour
 *    of Maggie's onesie in the pictures, pen 5 Lisa's dress, pen 8 Bart's shorts and the shirt's shade...), so a palette
 *    swap would dress others too. The palette RAM stays the board's (the game's state, its hashes, the same) and the
 *    video draws the shirt of Bart's sprite cells and tiles in the orange instead (simpsons_video_set_recolour):
 *    - Bart's own sprites (his object's kind, 02, as draw_object reads it: animation_sets' bank and index, its
 *      animations, their frames and sprite lists), in colour 0 (and 6, his tiny figure), on any slot (the cells are
 *      Bart's whichever slot plays him: same_character.c), in the stages, the stories, the presentation, the team moves
 *      (his cells there; Homer's, Marge's and Lisa's there have no pen 7), the ending's reunion;
 *    - his figure in the bonus games (kind 0C, 0A plus his kind: the first one's pumping, running, holding the
 *      balloon; his portrait's heads have no pen 7, his "3P" mark is colour 1), in colour 0;
 *    - Bart held by the goons that grab a player (kinds 80, 81 and 97: their lists of Bart, the only ones of theirs in
 *      colour 0 with pen 7, Bart drawn by the goon's object) and his far figure of kind A4, in colour 0;
 *    - the ending's family walking away under the stars (kind 14, colour 6): his sprites of it, the lists' entries
 *      5988-5A5F of 2 x 2 to 4 x 4 cells (Maggie's there, crawling with the same pen, are 1 x 2; the diamond's are
 *      past 5A5F), and his turn as Homer throws the diamond, past 5A5F too (family_turn_codes, the players' issue 25);
 *    - Bart falling into the gorge at stage 5's end and in its water (the scenery's kind, 10, colour 6: its six
 *      sprites of him, gorge_bart_codes; its other lists with the pen are the water's splashes), the players' issue 14;
 *    - the pictures drawn in the tile layers, by their map's cells and the picture's tiles (the rules below): Bart's
 *      profile and the family on the couch in the attract mode, Maggie and the diamond in the first story, the
 *      family's hug at the ending, and the family walking to Channel 6 after the second bonus game.
 *    In all of those sprites pen 7 is the shirt, but in Maggie standing (sprite 5974, 1 x 2 cells), which Bart's own
 *    lists draw too (02's 6130 in colour 6, the far family's 7F76 in colour 0, the same list as kind 39's): her onesie,
 *    left light blue (the players' issue 14: Maggie orange when Bart picks her up), and in Bart carrying her at the
 *    ending, after the diamond goes into Burns' mouth (his sprite 0870, 4 x 4 cells, when he is the one who picks her
 *    up): her onesie and his shirt share three of its cells, so her pixels there are kept as the board draws them
 *    (maggie_carried, the video's keep masks; painted by Sebas, the players' issue 27). Pen 8 is the shirt's shade and
 *    the shorts: each frame of them is assembled as the sprite engine lays it out, and a component of pen 8 is the
 *    shirt's shade when it touches the shirt (pen 7) on half its border or more that is not the outline (pen 3), and
 *    not the shorts' own shade (pen 12);
 *    in a component that touches it (the shorts), a pixel is the shirt's shade where its run in its row is three or
 *    fewer and ends at the shirt (the back's edge going down to the shorts). A cell's pixel is the same in every
 *    frame that uses it: the frames vote, and the shade's pixels are those most of them say. Those pixels are drawn in
 *    the shade of the orange, from the palette's colour of the shirt's pen.
 *    The attract mode's opening already dresses him in orange (its cells are the title's pieces, kind 0E: not his),
 *    the portraits, the boxes' faces and the stage cards show no shirt. Left as the cabinet has it: the first story's
 *    picture of the family with Smithers (a few pixels of Bart's sleeve behind Marge's hair, its tiles the picture's
 *    background's too), and the darkest blue (pen 12) at the back's lower edge, which is the shorts' shade too.
 *
 *    The orange follows the palette as the game changes it: a fade of the light blue (palette_job_fade: a step off or
 *    on each component not at its end) is the same fade of the orange, and any other colour there turns from blue to
 *    orange by how blue it is. For checks, SIMPSONS_BART_SHIRT=arcade in the environment draws it as the cabinet does
 *    (a scripted run takes no settings file).
 *
 *    Barney, stage 4's bystander at Moe's (kind 8C; 21 has the same lists, colour 9 always; Sebas, 15-11-2026: "dejá
 *    el pelo amarillo y cambiale el color de la remera y listo", the colour #D46F63, after his hair in brown was
 *    tried and left as the cabinet has it): his green shirt (pen 4, 2E00) and its shade (pen 5, 1D40) in that salmon
 *    and a salmon as much darker. Pen 4 is his shoes' stripes too: his 23 frames are assembled as Bart's, and a cell
 *    is the shirt's where most of its pen 4 and 5 pixels are at the body's height (BARNEY_FEET_ROWS above his feet or
 *    more), the shoes' cells left green. A fade of the green is the same fade of the salmon. The same setting as
 *    Bart's shirt (the show's colours), the same check's switch.
 */
#include "console_internal.h"

#include "game.h"
#include "game/data.h"
#include "simpsons/machine.h"
#include "simpsons/video.h"

#include <stdlib.h>
#include <string.h>

#define FIXED_PROGRAM       0x78000u    /* the fixed program's place in the program ROM (8000-FFFF) */
#define BANK_BYTES          0x2000u
#define BANK_WINDOW_END     0x8000u
#define CELLS               SIMPSONS_VIDEO_SPRITE_CELLS

#define KIND_BART           ((unsigned)CHARACTER_BART)    /* his object's kind (draw_object's set of animations) */
#define KIND_BART_FIGURE    0x0Cu       /* his figure in the bonus games and his portraits (0A plus his kind) */
#define KIND_FAMILY         0x14u       /* the ending's family walking away */
#define KIND_GOON           0x80u       /* the goons that grab a player, */
#define KIND_FAT_GOON       0x81u
#define KIND_GOON_2         0x97u
#define KIND_FAR_FAMILY     0xA4u       /* and the family far away */
#define KIND_SCENERY        0x10u       /* Moe's and the gorge's scenery: the family falling into the gorge */
#define MAGGIE_FIGURE       0x5974u     /* Maggie standing (1 x 2 cells), in Bart's lists too (02's 6130, A4's and
                                           39's 7F76): her onesie is the shirt's pen, never his */
#define KIND_BARNEY         0x8Cu       /* stage 4's bystander at Moe's, Barney (barney_bystander_update) */
#define BARNEY_COLOUR       9u          /* his colour code: pen 4 his shirt's green (and his shoes' stripes), */
#define BARNEY_SHIRT_PEN    4
#define BARNEY_SHADE_PEN    5           /* 5 its shade */
#define BARNEY_FEET_ROWS    16          /* (a pixel this far above his feet or more is the body's, else the shoes') */
#define PLAYERS_COLOUR      0u          /* the players' palette */
#define FAMILY_COLOUR       6u          /* the family's (the same pens) */
#define FAMILY_BART_FIRST   0x5988u     /* his sprites in the family's lists */
#define FAMILY_BART_LAST    0x5A5Fu
#define FAMILY_BART_CELLS   4u          /* (at least: Maggie's are 1 x 2 cells) */

#define SHIRT_PEN           7           /* the shirt's pen in the players' palette, */
#define SHADE_PEN           8           /* its shade's and the shorts', */
#define SHORTS_SHADE_PEN    12          /* the shorts' shade, */
#define OUTLINE_PEN         3           /* the outline */
#define SHADE_RUN           3           /* (a run of the shorts' blob at the shirt that is the back's edge) */
#define SLEEVE_MOST         8           /* (a bit of pen 8 alone at the shirt's height: the far arm's sleeve) */
#define PEN(n)              ((uint16_t)(1u << (n)))
#define LAYER_A             1u
#define LAYER_B             2u

/* The cabinet's light blue and its shade, and the orange, xBGR 555's components */
static const int arcade_shirt[3] = { 0, 24, 31 };
static const int arcade_shade[3] = { 0, 18, 30 };
static const int orange_shirt[3] = { 31, 10, 4 };    /* (the attract mode's opening's Bart) */

/* Barney's shirt's green and its shade, and Sebas's colour for it, #D46F63 (212, 111, 99) */
static const int barney_green[3] = { 0, 16, 11 };
static const int barney_shade[3] = { 0, 10, 7 };
static const int barney_salmon[3] = { 26, 14, 12 };

/* Bart turning to watch Homer throw the diamond, in the family's lists past 5A5F (692A, 6954 and 6984: his shirt
   there, beside Homer's turn, 5AEC-5B10, and the diamond's, 5AFD-5B4C; the players' issue 25) */
static const uint16_t family_turn_codes[] = { 0x5AF8, 0x5AF9, 0x5B14 };

/* Bart's sprites in the gorge (the scenery's lists 7C9E, falling, and 7DE2, in the water, in the family's colour;
   its other lists there with the shirt's pen are the water's splashes) */
static const uint16_t gorge_bart_codes[] = { 0x76D1, 0x76D4, 0x76D9, 0x7740, 0x7750, 0x7790 };

/* Maggie in Bart's arms at the ending (his sprite 0870): her onesie's pixels in the cells it shares with his shirt,
   each row's bit n its pixel n, kept light blue (Sebas painted the shirt on frame 32880 of a game of Bart alone; the
   players' issue 27, "Maggie's dress is still red when Bart picks her up") */
static const struct
{
	uint16_t cell;
	uint16_t rows[16];
} maggie_carried[] = {
	{ 0x0876, { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0028,
		0x0038, 0x0008, 0x0000 } },
	{ 0x0879, { 0x0000, 0x0000, 0x0000, 0x0000, 0x8000, 0x8000, 0x0000, 0x2000, 0x6000, 0xE200, 0xE000, 0xE000, 0xC000,
		0xC000, 0x0000, 0x0000 } },
	{ 0x087C, { 0x0000, 0x0000, 0x0000, 0x0000, 0x0003, 0x003F, 0x0070, 0x0000, 0x0000, 0x0000, 0x0003, 0x003F, 0x007F,
		0x007B, 0x007F, 0x001A } },
};
#define MAGGIE_CARRIED      (sizeof(maggie_carried) / sizeof(maggie_carried[0]))

/* The pictures in the tile layers: Bart's shirt's map cells (columns, rows), the picture's tiles there, its pen */
static const simpsons_video_tile_rule shirt_tiles[] = {
	{ LAYER_A, 11, 23, 30, 27, 0x5B9C, 0x5BE9, PEN(7) },    /* the attract mode's profile of Bart ("YO, MAN!") */
	{ LAYER_A, 57, 12, 61, 15, 0x5CD2, 0x5CEA, PEN(7) },    /* the attract mode's family on the couch */
	{ LAYER_B, 40, 23, 44, 25, 0x41E8, 0x4225, PEN(7) },    /* story 1: Maggie with the diamond, the family left */
	{ LAYER_B, 21, 21, 28, 27, 0x452A, 0x4591, PEN(7) },    /* the ending's hug (Maggie's onesie, the same tiles' */
	{ LAYER_B, 23, 20, 28, 20, 0x452A, 0x4591, PEN(7) },    /* pen, is above and left of these) */
	{ LAYER_B, 9, 25, 16, 31, 0x6280, 0x62CF, PEN(4) },     /* the family walking to Channel 6 (bonus 2's end), */
	{ LAYER_B, 9, 25, 16, 31, 0x6201, 0x6201, PEN(4) },     /* his shirt's lower part there a plain tile (the
	                                                           players' issue 14: blocks of light blue left) */
};

static uint16_t cell_pens[CELLS];
static uint16_t barney_body[CELLS], barney_feet[CELLS];    /* (his cells' pen 4 and 5 pixels: body, feet) */
static uint32_t cell_colours[CELLS];
static uint16_t cell_shades[CELLS];
static uint8_t (*shade_masks)[32];
static uint16_t cell_keeps[CELLS];
static uint8_t keep_masks[MAGGIE_CARRIED][32];
static int shade_mask_count;
static uint16_t shirt_colours[0x8000];    /* (each palette colour's, worked out once) */
static bool tables_made;

/* ---- the program's data, through a bank, and the sprite ROM ---- */

static int rom8(unsigned bank, uint16_t address)
{
	uint32_t place;
	const uint8_t *byte;

	if (address >= BANK_WINDOW_END)
		place = FIXED_PROGRAM + (address - BANK_WINDOW_END);
	else if (address >= BANK_WINDOW)
		place = bank * BANK_BYTES + (uint32_t)(address - BANK_WINDOW);
	else
		return -1;
	byte = simpsons_board.program_at ? simpsons_board.program_at(place) : NULL;
	return byte ? *byte : -1;
}

static int rom16(unsigned bank, uint16_t address)
{
	const int high = rom8(bank, address), low = rom8(bank, (uint16_t)(address + 1u));

	return high >= 0 && low >= 0 ? (high << 8 | low) : -1;
}

/* A 16 x 16 sprite cell's pen at x, y (the video's: 8 bytes a row, two pixels a byte, each word's bytes swapped) */
static int cell_pixel(unsigned cell, int x, int y)
{
	static const uint8_t nibbles[16] = { 2, 3, 0, 1, 6, 7, 4, 5, 10, 11, 8, 9, 14, 15, 12, 13 };
	const uint8_t data = game_sprites[(size_t)(cell % CELLS) * 128u + (size_t)y * 8u + (size_t)(nibbles[x] >> 1)];

	return (nibbles[x] & 1) ? data & 0x0F : data >> 4;
}

/* ---- a kind's sprites: their cells, and the frames they make ---- */

/* A sprite of a list: its code (the first cell's, its 6 low bits the place in a block of 8 x 8), flipped, colour,
   size (2 bits each the columns and rows as powers of two) and place (x, y from the object's) */
typedef struct
{
	unsigned code, colour, size;
	bool flip;
	int x, y;
} sprite;

static unsigned sprite_columns(const sprite *s)
{
	return 1u << (s->size & 3u);
}

static unsigned sprite_rows(const sprite *s)
{
	return 1u << ((s->size >> 2) & 3u);
}

/* The cell of a sprite's column and row, as the K053247 lays them out */
static unsigned sprite_cell(const sprite *s, unsigned column, unsigned row)
{
	static const unsigned x_offset[8] = { 0, 1, 4, 5, 16, 17, 20, 21 };
	static const unsigned y_offset[8] = { 0, 2, 8, 10, 32, 34, 40, 42 };
	const unsigned xa = (s->code & 1u) + ((s->code >> 1) & 2u) + ((s->code >> 2) & 4u);
	const unsigned ya = ((s->code >> 1) & 1u) + ((s->code >> 2) & 2u) + ((s->code >> 3) & 4u);

	return ((s->code & ~0x3Fu) + x_offset[(column + xa) & 7u] + y_offset[(row + ya) & 7u]) % CELLS;
}

static bool sprite_has_pen(const sprite *s, int pen)
{
	for (unsigned row = 0; row < sprite_rows(s); row++)
		for (unsigned column = 0; column < sprite_columns(s); column++)
			for (int y = 0; y < 16; y++)
				for (int x = 0; x < 16; x++)
					if (cell_pixel(sprite_cell(s, column, row), x, y) == pen)
						return true;
	return false;
}

/* A frame assembled as the sprite engine lays it out (each sprite centred on its place, y up, flipped whole; the
   list's first sprite over the others), on a canvas of CANVAS x CANVAS pixels round the object's place: each pixel's
   cell, its pixel in the cell, its pen (0: none); and the pen-8 components' labels */
#define CANVAS      512
#define ORIGIN      (CANVAS / 2)

typedef struct
{
	uint16_t cell[CANVAS * CANVAS];
	uint8_t pixel[CANVAS * CANVAS];
	uint8_t pen[CANVAS * CANVAS];
	int label[CANVAS * CANVAS];
	int stack[CANVAS * CANVAS];
	int left, top, right, bottom;    /* (what the frame covers) */
} canvas;

/* The frames' votes on each pen-8 pixel of the cells: shade, or not */
typedef struct
{
	uint16_t shade[256], other[256];
} votes;

static votes *vote_pool;
static int vote_count;
static uint16_t vote_index[CELLS];    /* (0: none; else one more than its votes' place) */

static votes *votes_of(unsigned cell)
{
	if (!vote_index[cell])
	{
		votes *grown = realloc(vote_pool, (size_t)(vote_count + 1) * sizeof(votes));

		if (!grown)
			return NULL;
		vote_pool = grown;
		memset(&vote_pool[vote_count], 0, sizeof(votes));
		vote_index[cell] = (uint16_t)++vote_count;
	}
	return &vote_pool[vote_index[cell] - 1];
}

static void assemble(canvas *frame, const sprite *sprites, int count)
{
	frame->left = frame->top = CANVAS;
	frame->right = frame->bottom = -1;
	for (int index = 0; index < count; index++)
	{
		const sprite *s = &sprites[index];
		const int columns = (int)sprite_columns(s), rows = (int)sprite_rows(s);

		for (int row = 0; row < rows; row++)
			for (int column = 0; column < columns; column++)
			{
				const unsigned cell = sprite_cell(s, (unsigned)column, (unsigned)row);
				const int left = ORIGIN + s->x - columns * 8 + (s->flip ? columns - 1 - column : column) * 16;
				const int top = ORIGIN - s->y - rows * 8 + row * 16;

				for (int y = 0; y < 16; y++)
					for (int x = 0; x < 16; x++)
					{
						const int pen = cell_pixel(cell, x, y);
						const int at_x = left + (s->flip ? 15 - x : x), at_y = top + y;
						const int at = at_y * CANVAS + at_x;

						if (!pen || at_x < 1 || at_y < 1 || at_x >= CANVAS - 1 || at_y >= CANVAS - 1 || frame->pen[at])
							continue;
						frame->cell[at] = (uint16_t)cell;
						frame->pixel[at] = (uint8_t)(y * 16 + x);
						frame->pen[at] = (uint8_t)pen;
						frame->label[at] = 0;
						frame->left = at_x < frame->left ? at_x : frame->left;
						frame->right = at_x > frame->right ? at_x : frame->right;
						frame->top = at_y < frame->top ? at_y : frame->top;
						frame->bottom = at_y > frame->bottom ? at_y : frame->bottom;
					}
			}
	}
}

/* The frame's pen-8 components, each the shirt's shade or the shorts (see the top), into the votes; the canvas
   cleared after */
static void classify(canvas *frame)
{
	static const int steps[4] = { 1, -1, CANVAS, -CANVAS };
	int label = 0, shirt_top = CANVAS, shirt_bottom = -1;

	/* (the rows the shirt's own pen covers: a small bit of pen 8 alone within them is the far arm's sleeve, Sebas:
	   "la mano que queda detras, se le ve el celeste de la remera"; the shoes, also pen 8, are below them) */
	for (int y = frame->top; y <= frame->bottom; y++)
		for (int x = frame->left; x <= frame->right; x++)
			if (frame->pen[y * CANVAS + x] == SHIRT_PEN)
			{
				shirt_top = y < shirt_top ? y : shirt_top;
				shirt_bottom = y > shirt_bottom ? y : shirt_bottom;
			}

	for (int y = frame->top; y <= frame->bottom; y++)
		for (int x = frame->left; x <= frame->right; x++)
		{
			const int first = y * CANVAS + x;
			int border[16] = { 0 }, size = 0, depth = 0, edge = 0, lowest = y, highest = y;

			if (frame->pen[first] != SHADE_PEN || frame->label[first])
				continue;
			frame->label[first] = ++label;
			frame->stack[depth++] = first;
			while (depth)
			{
				const int at = frame->stack[--depth];

				frame->stack[CANVAS * CANVAS - 1 - size++] = at;    /* (the component's pixels, from the far end) */
				lowest = at / CANVAS > lowest ? at / CANVAS : lowest;
				highest = at / CANVAS < highest ? at / CANVAS : highest;
				for (int step = 0; step < 4; step++)
				{
					const int next = at + steps[step];

					if (frame->pen[next] == SHADE_PEN)
					{
						if (!frame->label[next])
						{
							frame->label[next] = label;
							frame->stack[depth++] = next;
						}
					}
					else if (frame->pen[next])
						border[frame->pen[next]]++;
				}
			}
			for (int pen = 1; pen < 16; pen++)
				edge += pen == OUTLINE_PEN ? 0 : border[pen];
			const bool shade = (border[SHIRT_PEN] > 0 && border[SHORTS_SHADE_PEN] == 0 && 2 * border[SHIRT_PEN] >= edge)
				|| (size <= SLEEVE_MOST && border[SHORTS_SHADE_PEN] == 0 && highest >= shirt_top && lowest <= shirt_bottom);

			for (int index = 0; index < size; index++)
			{
				const int at = frame->stack[CANVAS * CANVAS - 1 - index];
				bool in_shade = shade;
				votes *v;

				if (!shade)
				{
					int left = at, right = at;

					while (frame->label[left - 1] == label && frame->pen[left - 1] == SHADE_PEN)
						left--;
					while (frame->label[right + 1] == label && frame->pen[right + 1] == SHADE_PEN)
						right++;
					in_shade = right - left + 1 <= SHADE_RUN
						&& (frame->pen[left - 1] == SHIRT_PEN || frame->pen[right + 1] == SHIRT_PEN);
				}
				if ((v = votes_of(frame->cell[at])) != NULL)
				{
					if (in_shade)
						v->shade[frame->pixel[at]]++;
					else
						v->other[frame->pixel[at]]++;
				}
			}
		}
	for (int y = frame->top; y <= frame->bottom; y++)
		for (int x = frame->left; x <= frame->right; x++)
		{
			frame->pen[y * CANVAS + x] = 0;
			frame->label[y * CANVAS + x] = 0;
		}
}

/* Kind 'kind''s animations (draw_object: animation_sets', or animation_sets_high's from 80, bank and index, the
   animations' table there, each animation's frames of 4 bytes up to its end, a frame's sprite list of its count plus
   one sprites of 6 bytes: code, colour, size, x, y). The table's animations are those before the first one's frames
   (the table comes first). Of each list, the sprites 'wanted' takes (when 'bart_lists' says, only of lists with a
   sprite in the players' colour with the shirt's pen: Bart's), given to 'use' */
static void walk_kind(canvas *frame, unsigned kind, bool (*wanted)(const sprite *s), bool bart_lists,
	void (*use)(canvas *frame, const sprite *sprites, int count))
{
	const uint16_t set = (uint16_t)(kind & 0x80u ? animation_sets_high_ADDRESS + 2u * (kind & 0x7Fu)
		: animation_sets_ADDRESS + 2u * kind);
	const int bank = rom8(0, set), index = rom8(0, (uint16_t)(set + 1u));
	const int table = bank >= 0 && index >= 0 ? rom16((unsigned)bank, (uint16_t)(BANK_WINDOW + (unsigned)index)) : -1;
	int first_frames = BANK_WINDOW_END;

	if (table < (int)BANK_WINDOW)
		return;
	for (int animation = 0; table + 2 * animation < first_frames; animation++)
	{
		int at = rom16((unsigned)bank, (uint16_t)(table + 2 * animation));

		if (at < (int)BANK_WINDOW || at >= (int)BANK_WINDOW_END)
			break;
		if (at < first_frames)
			first_frames = at;
		for (int head; (head = rom8((unsigned)bank, (uint16_t)at)) >= 0 && !(head & 0x80); at += 4)
		{
			const int list = rom16((unsigned)bank, (uint16_t)(at + 2));
			sprite sprites[16];
			int count = 0;
			bool barts = !bart_lists;

			for (int number = 0; list >= 0 && number <= (head & 0x0F); number++)
			{
				const uint16_t entry = (uint16_t)(list + 6 * number);
				const int code = rom16((unsigned)bank, entry), colour = rom8((unsigned)bank, (uint16_t)(entry + 2u));
				const int size = rom8((unsigned)bank, (uint16_t)(entry + 3u));
				const int x = rom8((unsigned)bank, (uint16_t)(entry + 4u)), y = rom8((unsigned)bank, (uint16_t)(entry + 5u));
				sprite s;

				if (code < 0 || colour < 0 || size < 0 || x < 0 || y < 0)
					continue;
				s.code = (unsigned)code & 0x7FFFu;
				s.flip = (code & 0x8000) != 0;
				s.colour = (unsigned)colour & 0x1Fu;
				s.size = (unsigned)size;
				s.x = (int8_t)x;
				s.y = (int8_t)y;
				if (s.code == MAGGIE_FIGURE || !wanted(&s))
					continue;    /* (Maggie's onesie stays light blue, whoever's list draws her) */
				sprites[count++] = s;
				barts = barts || (s.colour == PLAYERS_COLOUR && sprite_has_pen(&s, SHIRT_PEN));
			}
			if (barts && count)
				use(frame, sprites, count);
		}
	}
}

/* A frame of Bart's: its cells' shirt pen recoloured in their colour, and the frame classified (its shade) */
static void mark_shirt(canvas *frame, const sprite *sprites, int count)
{
	for (int number = 0; number < count; number++)
		for (unsigned row = 0; row < sprite_rows(&sprites[number]); row++)
			for (unsigned column = 0; column < sprite_columns(&sprites[number]); column++)
			{
				const unsigned cell = sprite_cell(&sprites[number], column, row);

				cell_pens[cell] |= PEN(SHIRT_PEN);
				cell_colours[cell] |= 1u << sprites[number].colour;
			}
	assemble(frame, sprites, count);
	classify(frame);
}

/* A frame of Barney's: each pixel of his shirt's pens counted for its cell, at the body or at the feet (see the top);
   the canvas cleared after */
static void count_barney(canvas *frame, const sprite *sprites, int count)
{
	assemble(frame, sprites, count);
	for (int y = frame->top; y <= frame->bottom; y++)
		for (int x = frame->left; x <= frame->right; x++)
		{
			const int at = y * CANVAS + x, pen = frame->pen[at];

			if (pen == BARNEY_SHIRT_PEN || pen == BARNEY_SHADE_PEN)
			{
				uint16_t *counted = frame->bottom - y >= BARNEY_FEET_ROWS ? barney_body : barney_feet;

				if (counted[frame->cell[at]] < UINT16_MAX)
					counted[frame->cell[at]]++;
			}
			frame->pen[at] = 0;
			frame->label[at] = 0;
		}
}

static bool bart_colours(const sprite *s)
{
	return s->colour == PLAYERS_COLOUR || s->colour == FAMILY_COLOUR;    /* (his devil is colour 5, the mop 1) */
}

static bool players_colour(const sprite *s)
{
	return s->colour == PLAYERS_COLOUR;
}

static bool family_bart(const sprite *s)
{
	if (s->colour != FAMILY_COLOUR)
		return false;
	for (size_t index = 0; index < sizeof(family_turn_codes) / sizeof(family_turn_codes[0]); index++)
		if (s->code == family_turn_codes[index])
			return true;
	return s->code >= FAMILY_BART_FIRST && s->code <= FAMILY_BART_LAST
		&& sprite_columns(s) * sprite_rows(s) >= FAMILY_BART_CELLS;
}

static bool barney_colour(const sprite *s)
{
	return s->colour == BARNEY_COLOUR;
}

static bool gorge_bart(const sprite *s)
{
	for (size_t index = 0; index < sizeof(gorge_bart_codes) / sizeof(gorge_bart_codes[0]); index++)
		if (s->code == gorge_bart_codes[index])
			return s->colour == FAMILY_COLOUR;
	return false;
}

/* The shade's pixels of each cell: those most frames say */
static void make_shade_masks(void)
{
	free(shade_masks);
	shade_masks = calloc((size_t)(vote_count ? vote_count : 1), sizeof(*shade_masks));
	shade_mask_count = 0;
	memset(cell_shades, 0, sizeof(cell_shades));
	for (unsigned cell = 0; shade_masks && cell < CELLS; cell++)
	{
		const votes *v = vote_index[cell] ? &vote_pool[vote_index[cell] - 1] : NULL;
		bool any = false;

		for (int pixel = 0; v && pixel < 256; pixel++)
			if (v->shade[pixel] > v->other[pixel])
			{
				shade_masks[shade_mask_count][pixel >> 3] |= (uint8_t)(1u << (pixel & 7));
				any = true;
			}
		if (any)
			cell_shades[cell] = (uint16_t)++shade_mask_count;
	}
}

/* ---- the orange ---- */

/* A colour 'steps' fades away from 'from' (palette_job_fade: darken_colour takes one off each component not at 0,
   brighten_colour adds one to each not at 31) */
static uint16_t faded(const int from[3], int steps)
{
	int component[3];

	for (int index = 0; index < 3; index++)
	{
		component[index] = from[index] + steps;
		component[index] = component[index] < 0 ? 0 : component[index] > 31 ? 31 : component[index];
	}
	return (uint16_t)(component[0] | component[1] << 5 | component[2] << 10);
}

static int brightness(const int component[3]);

/* The orange for a colour the palette gives Bart's shirt's pen (and the salmon for Barney's shirt's green, its
   shade) */
static uint16_t shirt_colour(uint16_t colour)
{
	const int red = colour & 31, green = (colour >> 5) & 31, blue = (colour >> 10) & 31;
	const int least = red < green ? (red < blue ? red : blue) : (green < blue ? green : blue);
	int component[3];

	for (int steps = -31; steps <= 31; steps++)
		if (faded(arcade_shirt, steps) == (colour & 0x7FFFu))
			return faded(orange_shirt, steps);
	for (int steps = -31; steps <= 31; steps++)
	{
		/* (Barney's green, its shade: the salmon, and a salmon as much darker as the shade is than the green) */
		if (faded(barney_green, steps) == (colour & 0x7FFFu))
			return faded(barney_salmon, steps);
		if (faded(barney_shade, steps) == (colour & 0x7FFFu))
		{
			const int lit = brightness(barney_green), dark = brightness(barney_shade);
			int darker[3];

			for (int index = 0; index < 3; index++)
				darker[index] = (barney_salmon[index] * dark + lit / 2) / lit;
			return faded(darker, steps);
		}
	}
	/* (another colour: as blue as it is over its grey, that much orange over it) */
	for (int index = 0; index < 3; index++)
	{
		component[index] = least + (blue - least) * orange_shirt[index] / arcade_shirt[2];
		component[index] = component[index] > 31 ? 31 : component[index];
	}
	return (uint16_t)(component[0] | component[1] << 5 | component[2] << 10);
}

/* A colour's brightness (Rec. 601's weights, in thousandths) */
static int brightness(const int component[3])
{
	return 299 * component[0] + 587 * component[1] + 114 * component[2];
}

static uint16_t recolour(uint16_t colour)
{
	return shirt_colours[colour & 0x7FFFu];
}

/* The shade for the palette's colour of the shirt's pen: the orange for it, as much darker as the cabinet's shade is
   than its light blue */
static uint16_t shade(uint16_t colour)
{
	const uint16_t orange = shirt_colours[colour & 0x7FFFu];
	const int lit = brightness(arcade_shirt), dark = brightness(arcade_shade);
	int component[3] = { orange & 31, (orange >> 5) & 31, (orange >> 10) & 31 };

	for (int index = 0; index < 3; index++)
		component[index] = (component[index] * dark + lit / 2) / lit;
	return (uint16_t)(component[0] | component[1] << 5 | component[2] << 10);
}

static void make_tables(void)
{
	canvas *frame = calloc(1, sizeof(canvas));

	memset(cell_pens, 0, sizeof(cell_pens));
	memset(cell_colours, 0, sizeof(cell_colours));
	memset(vote_index, 0, sizeof(vote_index));
	vote_count = 0;
	if (frame)
	{
		walk_kind(frame, KIND_BART, bart_colours, false, mark_shirt);
		walk_kind(frame, KIND_BART_FIGURE, players_colour, false, mark_shirt);
		walk_kind(frame, KIND_FAMILY, family_bart, false, mark_shirt);
		walk_kind(frame, KIND_GOON, players_colour, true, mark_shirt);
		walk_kind(frame, KIND_FAT_GOON, players_colour, true, mark_shirt);
		walk_kind(frame, KIND_GOON_2, players_colour, true, mark_shirt);
		walk_kind(frame, KIND_FAR_FAMILY, players_colour, true, mark_shirt);
		walk_kind(frame, KIND_SCENERY, gorge_bart, false, mark_shirt);
		memset(barney_body, 0, sizeof(barney_body));
		memset(barney_feet, 0, sizeof(barney_feet));
		walk_kind(frame, KIND_BARNEY, barney_colour, false, count_barney);
		for (unsigned cell = 0; cell < CELLS; cell++)
			if (barney_body[cell] > barney_feet[cell])
			{
				/* (his shirt's cells: their two pens in his colour; his shoes' keep their stripes) */
				cell_pens[cell] |= PEN(BARNEY_SHIRT_PEN) | PEN(BARNEY_SHADE_PEN);
				cell_colours[cell] |= 1u << BARNEY_COLOUR;
			}
		make_shade_masks();
		free(frame);
	}
	memset(cell_keeps, 0, sizeof(cell_keeps));
	for (size_t index = 0; index < MAGGIE_CARRIED; index++)
	{
		for (int row = 0; row < 16; row++)
		{
			keep_masks[index][2 * row] = (uint8_t)(maggie_carried[index].rows[row] & 0xFFu);
			keep_masks[index][2 * row + 1] = (uint8_t)(maggie_carried[index].rows[row] >> 8);
		}
		cell_keeps[maggie_carried[index].cell % CELLS] = (uint16_t)(index + 1u);
	}
	free(vote_pool);
	vote_pool = NULL;
	for (unsigned colour = 0; colour < 0x8000u; colour++)
		shirt_colours[colour] = shirt_colour((uint16_t)colour);
	tables_made = true;
}

void console_bart_shirt_apply(void)
{
	const char *check = getenv("SIMPSONS_BART_SHIRT");    /* (a check's: "arcade" or "orange") */
	const bool orange = check ? strcmp(check, "arcade") != 0 : console_settings.bart_shirt;
	simpsons_video_recolouring recolouring;

	if (!orange)
	{
		simpsons_video_set_recolour(NULL);
		return;
	}
	if (!tables_made)
		make_tables();
	memset(&recolouring, 0, sizeof(recolouring));
	recolouring.colour = recolour;
	recolouring.cell_pens = cell_pens;
	recolouring.cell_colours = cell_colours;
	recolouring.cell_shades = cell_shades;
	recolouring.shade_masks = (const uint8_t (*)[32])shade_masks;
	recolouring.cell_keeps = cell_keeps;
	recolouring.keep_masks = (const uint8_t (*)[32])keep_masks;
	recolouring.shade_pen = SHIRT_PEN;
	recolouring.shade = shade;
	recolouring.tiles = shirt_tiles;
	recolouring.tile_rules = (int)(sizeof(shirt_tiles) / sizeof(shirt_tiles[0]));
	simpsons_video_set_recolour(&recolouring);
}
