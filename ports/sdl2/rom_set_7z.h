/**
 *    Name:        rom_set_7z.h
 *    Description: the player's ROM set packed by 7-Zip (simpsons.7z): its files out into a folder (rom_set_7z.c)
 *    Version:     0.1.0
 *    Created:     15-11-2026
 *    Modified:    15-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 */
#ifndef SIMPSONS_SDL2_ROM_SET_7Z_H
#define SIMPSONS_SDL2_ROM_SET_7Z_H

#include <stddef.h>

#define ROM_SET_7Z_FILES 64    /* (MAME's simpsons set has 13) */

/* What rom_set_7z_unpack wrote: the folder (with its separator) and its files' names */
typedef struct
{
	char folder[1100];
	int count;
	char names[ROM_SET_7Z_FILES][64];
} rom_set_7z_unpacked;

/* Whether 'path' names a .7z (by its name's end, any case) */
int rom_set_is_7z(const char *path);

/* The files of the .7z 'archive' written into 'folder' (made if it is not there; with its separator), each by its
   name alone (no folders of the archive's); what was written into 'unpacked' (rom_set_7z_remove takes it away).
   NULL, or what went wrong: the archive's files are only taken as they are, the ROM set's checks are the loader's */
const char *rom_set_7z_unpack(const char *archive, const char *folder, rom_set_7z_unpacked *unpacked);

/* The files rom_set_7z_unpack wrote, and its folder when it is left empty, removed */
void rom_set_7z_remove(const rom_set_7z_unpacked *unpacked);

#endif
