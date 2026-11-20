/**
 *    Name:        presentation.c
 *    Description: the console's Konami presentation, before the menu at power-on: the game's big Konami logo on
 *                 black, unseen until the family walks across it and lights it up, then one of Bart's voices
 *    Version:     0.1.0
 *    Created:     13-11-2026
 *    Modified:    13-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    As tmnt-port's console/presentation.c. The logo is the game's biggest Konami logo: the one the attract mode's
 *    last screen shows on its TV after the family on the couch (attract_last_screen_start), layer A's map of the
 *    attract's scene 0, area 3, with palette set 12's colours for layer A, drawn by the game itself into layer A
 *    over a black background (the TV left out). It is the size the game has it (about 112 x 128 pixels, the KONAMI
 *    under the mark): the console hides the part the family has not passed yet (Sebas: "si hay un konami grande,
 *    usamos ese"). The family are the game's objects as story 1 walks them in (story_1_marge_start...): Homer, Bart
 *    and Lisa on their walks (animation 1 of their kinds, the slots' characters), and Marge with Maggie in her arms
 *    (the maggie set, kind 11, its animation 2), walking from the left, drawn and moved by the game's sprite engine
 *    (update_sprites) with palette set 0's sprite colours (the players' in a game). The sound is one of Bart's voices
 *    (Sebas: "el sonido usemos uno de bart"), the one he says when he has stood still too long (3F12: the PCM effect
 *    of the K053260's sample 34), which Sebas chose by ear among them (13-11-2026: "3f12"); the others are listed in
 *    console/PLAN.md. Left untouched, the cabinet's own attract mode follows it, as in tmnt-port (console.c, the
 *    players' issue 23); skipped, the menu.
 */
#include "console_internal.h"

#include "game.h"

#define SCREEN_HEIGHT       224
#define LOGO_SCENE          0u         /* the attract's maps */
#define LOGO_AREA           3u         /* its Konami logo, in layer A (attract_last_screen_start) */
#define LOGO_TOP            28         /* the logo's rows on the screen (with layer A's scroll the last screen's) */
#define LOGO_BOTTOM         164
#define LAYERS_SCROLL_X     0xFFF0u    /* the layers' x scroll the last screen draws them with */
#define BACKGROUND          3u         /* palette requests: the background colour (black), */
#define PALETTE_SETS        1u         /* and loads of palette sets: */
#define FAMILY_PALETTES     0x0900u    /* set 0's loads 0 and 3, the fixed layer's and the sprites' (the players') */
#define LOGO_PALETTES       0x020Cu    /* set 12's load 1, layer A's: the logo's (the last screen's 020C) */
#define CLEAR_LAYERS        0u         /* the job that clears the three layers */
#define BART_VOICE          0x3F12u    /* 3F 12: Bart standing still too long (sample 34, 0.90 s) */
#define FAMILY              4
#define FIRST_OBJECT        14         /* the objects the attract's scenes use */
#define OBJECT_ON           0x80u
#define OVER_LAYERS         0x10u      /* attributes: their sprites' priority over the layers (the title's logo's), facing
                                          right */
#define FROM_FIRST_FRAME    0x80u      /* an animation's bit 7: from its first frame */
#define MARGE_WITH_MAGGIE   0x11u      /* the maggie set: Marge with Maggie in her arms (story 1's) */
#define FAMILY_SPEED        0x0140     /* the x speed: 1.25 pixels a frame (story 1's family walks at 1) */
#define OBJECT_X_LEFT       58         /* an object's x at the screen's column 0 (test_players' portraits) */
#define GROUND              0xB8u      /* their floor line: their feet near the screen's bottom, across KONAMI */
#define LIGHT               40         /* the logo lit up to this far past the leading one's place */
#define FADE_FRAMES         16

/* The walkers, from the leading one: each an object's kind and its walk, a gap behind the one before, its floor line
   (as story 1's family walks in: story_1_marge_start... Marge with Maggie in her arms is the maggie set's walk, kind
   11 animation 2) */
static const struct
{
	uint8_t kind;
	uint8_t animation;
	int behind;                /* pixels behind the one before */
	int depth;                 /* its floor line, from GROUND */
} family[FAMILY] = {
	{ CHARACTER_HOMER, 1u, 0, 0 }, { MARGE_WITH_MAGGIE, 2u, 44, -6 }, { CHARACTER_BART, 1u, 44, 4 },
	{ CHARACTER_LISA, 1u, 34, 8 },
};

/* The scene's frames (the game's 60 a second) */
#define AT_FAMILY           40
#define AT_VOICE_AFTER      10         /* frames after the last one is off the screen */
#define HOLD_AFTER_VOICE    100
#define AT_END_MOST         900        /* (at the latest) */

static struct
{
	int frame;
	int part;                  /* 0 setting up, 1 walking, 2 lit, 3 fading out, 4 over */
	int part_frame;
	int lit_to;                /* the logo is lit left of here */
	int brightness;            /* 0-256 */
	bool skipped;              /* START or attack pressed during it */
} show;

static uint16_t walker(int index)
{
	return (uint16_t)OBJECT_ADDRESS(FIRST_OBJECT + index);
}

static void start_family(void)
{
	int x = -32;

	for (int index = 0; index < FAMILY; index++)
	{
		const uint16_t object = walker(index);

		x -= family[index].behind;
		OBJECT(object)->kind = family[index].kind;
		OBJECT(object)->animation = (uint8_t)(family[index].animation | FROM_FIRST_FRAME);
		OBJECT(object)->animation_flags = 0u;
		OBJECT(object)->frame_time = 0u;
		OBJECT(object)->attributes = OVER_LAYERS;
		OBJECT(object)->x = (uint16_t)(x + OBJECT_X_LEFT);
		OBJECT(object)->y = (uint16_t)(GROUND + family[index].depth);
		OBJECT(object)->z = 0u;
		OBJECT(object)->x_speed = FAMILY_SPEED;
		OBJECT(object)->y_speed = 0;
		OBJECT(object)->z_speed = 0;
		OBJECT(object)->flags = OBJECT_ON;
	}
}

/* The logo lit as far as the leading one has gone; false once they are all past the screen's right */
static bool family_walking(void)
{
	bool walking = false;

	for (int index = 0; index < FAMILY; index++)
	{
		const uint16_t object = walker(index);
		const int x = (int16_t)OBJECT(object)->x - OBJECT_X_LEFT;

		if (OBJECT(object)->flags == 0u)
			continue;
		if (x + LIGHT > show.lit_to)
			show.lit_to = x + LIGHT;
		if (x < SIMPSONS_VIDEO_WIDTH + 48)
			walking = true;
		else
			OBJECT(object)->flags = 0u;
	}
	return walking;
}

/* As the attract's screens start: the objects and their sprites cleared, the layers' clear queued, the background
   black, the family's colours and the logo's */
static void set_up(void)
{
	uint16_t place = 0, sprite_place = 0, job = 0;

	vector_clear_objects(&place, &sprite_place);
	clear_sprites();
	queue_job(CLEAR_LAYERS);
	palette_request_kind = BACKGROUND;
	job = palette_request(job, 0u);
	palette_request_kind = PALETTE_SETS;
	job = palette_request(job, FAMILY_PALETTES);
	palette_request(job, LOGO_PALETTES);
}

/* Once the layers are clear: the logo drawn into layer A as the last screen draws it (the tile banks of the attract's
   maps, the layers' scroll) */
static void draw_logo(void)
{
	map_scene = LOGO_SCENE;
	map_area = 0u;
	set_area_tile_banks();
	attract_reset_layers();
	layer_a_scroll_x = LAYERS_SCROLL_X;
	layer_b_scroll_x = LAYERS_SCROLL_X;
	map_area = LOGO_AREA;
	attract_draw_layer_a();
}

static void next_part(int part)
{
	show.part = part;
	show.part_frame = 0;
}

bool presentation_skipped(void)
{
	return show.skipped;
}

bool presentation_frame(uint32_t pressed)
{
	if (show.part == 4)
		return false;
	show.frame++;
	show.part_frame++;
	if (show.frame == 1)
		set_up();
	if ((pressed & (ACTION_BIT(ACTION_START) | ACTION_BIT(ACTION_ACCEPT))) && show.part < 3)
	{
		show.skipped = true;
		next_part(3);    /* (skipped: the fade out at once) */
	}
	switch (show.part)
	{
		case 0:
			show.brightness = 0;
			if (show.frame >= AT_FAMILY && layer_clear_pending == 0u)
			{
				draw_logo();
				start_family();
				show.lit_to = -SIMPSONS_VIDEO_WIDTH;
				next_part(1);
			}
			break;
		case 1:
			show.brightness = show.part_frame >= FADE_FRAMES ? 256 : show.part_frame * 256 / FADE_FRAMES;
			if (!family_walking())
			{
				show.lit_to = SIMPSONS_VIDEO_WIDTH;
				next_part(2);
			}
			break;
		case 2:
			show.brightness = 256;
			if (show.part_frame == AT_VOICE_AFTER)
				queue_sound_always(BART_VOICE);
			if (show.part_frame >= AT_VOICE_AFTER + HOLD_AFTER_VOICE)
				next_part(3);
			break;
		case 3:
			show.brightness = show.part_frame >= FADE_FRAMES ? 0 : 256 - show.part_frame * 256 / FADE_FRAMES;
			if (show.part_frame >= FADE_FRAMES)
			{
				uint16_t place = 0, sprite_place = 0;

				vector_clear_objects(&place, &sprite_place);
				next_part(4);
				return false;
			}
			break;
	}
	if (show.frame >= AT_END_MOST && show.part < 3)
		next_part(3);
	return true;
}

/* Black where the family has not passed yet (the logo's part right of them: they are all left of it), the whole
   picture faded in and out */
void presentation_overlay(console_picture *frame)
{
	if (show.part == 0)
	{
		draw_fill(frame, PICTURE_FIRST(frame), 0, PICTURE_END(frame) - PICTURE_FIRST(frame), SCREEN_HEIGHT, 0x000000u);
		return;
	}
	if (show.lit_to < PICTURE_END(frame))
	{
		const int from = show.lit_to > PICTURE_FIRST(frame) ? show.lit_to : PICTURE_FIRST(frame);

		draw_fill(frame, from, LOGO_TOP, PICTURE_END(frame) - from, LOGO_BOTTOM - LOGO_TOP, 0x000000u);
	}
	if (show.brightness < 256)
		draw_shade(frame, PICTURE_FIRST(frame), 0, PICTURE_END(frame) - PICTURE_FIRST(frame), SCREEN_HEIGHT,
			show.brightness);
}
