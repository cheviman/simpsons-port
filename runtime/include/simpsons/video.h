/**
 *    Name:        video.h
 *    Description: The Simpsons' video: the K052109's three tile layers, the K053246/K053247's sprites and the
 *                 K053251's priorities and colour bases, drawn from the board's memories as the board shows them
 *    Version:     0.1.0
 *    Created:     06-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 */
#ifndef SIMPSONS_VIDEO_H
#define SIMPSONS_VIDEO_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SIMPSONS_VIDEO_WIDTH    320
#define SIMPSONS_VIDEO_HEIGHT   224     /* the screen's lines 16-239 */

typedef struct simpsons_frame
{
	uint32_t pixels[SIMPSONS_VIDEO_WIDTH * SIMPSONS_VIDEO_HEIGHT];  /* 0x00RRGGBB */
} simpsons_frame;

/* the chips as the board's reset leaves them */
void simpsons_video_reset(void);

/* VBLANK ends (line 16): the chips take the board's memories, the K052109 its scroll, and the frame is drawn into
   'frame' (none when NULL): The Simpsons shows a frame from the end of its VBLANK */
void simpsons_video_vblank_end(simpsons_frame *frame);

/* VBLANK begins (line 240): the chips take the board's memories */
void simpsons_video_vblank_start(void);

/* FNV-1a over the frame's pixels, red, green and blue (kref's video hashes) */
uint64_t simpsons_video_hash(const simpsons_frame *frame);

/* ---- a port's WIDE (the console's; the arcade's is always none, its frames the board's byte for byte) ----

   A wider picture: 'left' and 'right' columns more at each side of the board's 320 (their sum up to
   SIMPSONS_VIDEO_MAX_MORE, and a side as much as SIMPSONS_VIDEO_MAX_LESS less than none, the other the more: a picture
   that ends where the board's black edges did), the layers' maps (512 pixels wide, a ring) and the sprites (the
   K053247's x has 10 bits) drawn there too, from the next frame drawn on. The frame above stays the board's 320
   columns; simpsons_video_wide_frame gives the wider one, its rows SIMPSONS_VIDEO_MAX_WIDTH apart, and its width (320
   + left + right) */
#define SIMPSONS_VIDEO_MAX_MORE     176
#define SIMPSONS_VIDEO_MAX_LESS     32
#define SIMPSONS_VIDEO_MAX_WIDTH    (SIMPSONS_VIDEO_WIDTH + SIMPSONS_VIDEO_MAX_MORE)
void simpsons_video_set_wide(int left, int right);
const uint32_t *simpsons_video_wide_frame(int *width);
int simpsons_video_wide_left(void);    /* (the columns of that frame left of the board's column 0) */

/* WIDE's maps' ends: layer 'map' (1 A, 2 B) shows its map from screen column 'first' to 'last' only (the board's
   0-319), nothing of it past them, where its map has no more columns (its ring of 512 pixels holds others there);
   from the next frame drawn on, and only with a wider picture */
void simpsons_video_set_layer_window(int map, int first, int last);

/* WIDE's fixed layer: its board's columns 'first' to 'last' alone (its ring's others hold what the game never shows,
   and the board's first and last 16 its black edges, which the cabinet showed), drawn 'dx' columns further right than
   the board's (the players' boxes kept in the middle of a picture whose sides differ); from the next frame drawn on,
   and only with a wider picture */
void simpsons_video_set_fixed(int dx, int first, int last);

/* ---- a port's recolouring (the console's: Bart's shirt; the arcade's sets none, its frames the board's) ----

   Some pens of some sprite cells and of some layer tiles drawn in other colours than the palette RAM gives them, and some
   pixels of some sprite cells in a shade of them, the RAM and every other pixel as the board has them. The colours are
   the caller's functions of a palette colour (xBGR 555 to xBGR 555; a shaded pen takes the colour shaded), and of it
   alone: the video asks them for a colour once and keeps the answer until the recolouring is set again. The tables
   are the caller's and stay as it leaves them; NULL turns it off. From the next frame drawn on */
#define SIMPSONS_VIDEO_SPRITE_CELLS 0x8000    /* (the sprite ROM's 16 x 16 cells) */

typedef uint16_t (*simpsons_video_recolour)(uint16_t colour);

/* A layer's tiles recoloured: in layer 'map' (1 A, 2 B), the map's cells (64 x 32 of 8 x 8 pixels) from column
   'first_column' and row 'first_row' to 'last_column' and 'last_row' whose tile (its code after the board's tile
   callback, the tile ROM's) is 'first_code' to 'last_code': their pens 'pens' */
typedef struct
{
	uint8_t map;
	uint8_t first_column, first_row, last_column, last_row;
	uint16_t first_code, last_code;
	uint16_t pens;
} simpsons_video_tile_rule;

typedef struct
{
	simpsons_video_recolour colour;     /* a recoloured pen's colour, from its palette colour */
	const uint16_t *cell_pens;          /* each sprite cell's pens recoloured (bit n: pen n; SIMPSONS_VIDEO_SPRITE_CELLS), */
	const uint32_t *cell_colours;       /* only in its sprite's colours here (bit n: its attributes' 5 bits n) */
	const uint16_t *cell_shades;        /* each sprite cell's pixels drawn in the shade, in the same colours: 0 none, else
	                                       one more than its mask's place in 'shade_masks' (16 x 16 bits, row by row, bit
	                                       n of byte n / 8 the pixel n) */
	const uint8_t (*shade_masks)[32];
	const uint16_t *cell_keeps;         /* each sprite cell's pixels left as the board draws them, its recoloured pens
	                                       not recoloured there (another figure's in the same pen): 0 none, else one
	                                       more than its mask's place in 'keep_masks' (as 'shade_masks') */
	const uint8_t (*keep_masks)[32];
	int shade_pen;                      /* the shade's colour: 'shade' of this pen's palette colour in the sprite's */
	simpsons_video_recolour shade;      /* colour code */
	const simpsons_video_tile_rule *tiles;    /* the layers' rules (none: NULL, 0) */
	int tile_rules;
} simpsons_video_recolouring;

void simpsons_video_set_recolour(const simpsons_video_recolouring *recolouring);

#ifdef __cplusplus
}
#endif

#endif
