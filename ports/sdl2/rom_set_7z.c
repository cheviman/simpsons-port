/**
 *    Name:        rom_set_7z.c
 *    Description: the player's ROM set packed by 7-Zip (simpsons.7z): its files out into a folder, for the assets'
 *                 loader, which reads a set's .zip or its folder of files
 *    Version:     0.1.0
 *    Created:     15-11-2026
 *    Modified:    15-11-2026
 *    Author:      Sebastian Toyos
 *
 *    license:BSD-3-Clause
 *    MAME's sets are passed around as .7z as much as .zip (the Recomp Standards: the formats players have). The
 *    archive is read with the LZMA SDK's 7z decoder (ports/sdl2/lzma, public domain): each file, solid blocks and
 *    all, decoded in memory and written by its name alone into the folder the first run gives (the folder of what the
 *    game keeps), which the assets' loader then reads as a set's folder (game_load_rom_set), and which is taken away
 *    after (rom_set_7z_remove). The files are taken as they are: whether they are the set is the loader's to say.
 */
#include "rom_set_7z.h"

#include "lzma/7z.h"
#include "lzma/7zAlloc.h"
#include "lzma/7zCrc.h"
#include "lzma/7zFile.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>    /* (_mkdir, _rmdir) */
#define make_folder(path)   _mkdir(path)
#define remove_folder(path) _rmdir(path)
#else
#include <sys/stat.h>
#include <unistd.h>
#define make_folder(path)   mkdir(path, 0755)
#define remove_folder(path) rmdir(path)
#endif

#define LOOK_BUFFER (1u << 18)    /* the archive's reads, at a time */

static const ISzAlloc g_alloc = { SzAlloc, SzFree };
static const ISzAlloc g_alloc_temp = { SzAllocTemp, SzFreeTemp };

int rom_set_is_7z(const char *path)
{
	const size_t length = path ? strlen(path) : 0;

	return length > 3 && path[length - 3] == '.' && path[length - 2] == '7'
		&& tolower((unsigned char)path[length - 1]) == 'z';
}

/* The file 'index''s name, its last part (the archive's folders dropped), into 'name': "" when it is not plain ASCII
   (no set's file is), too long, or a folder's name of the archive's own */
static void file_name(const CSzArEx *db, UInt32 index, char *name, size_t size)
{
	UInt16 wide[256];
	size_t length = SzArEx_GetFileNameUtf16(db, index, NULL), start = 0, at = 0;

	name[0] = '\0';
	if (length == 0 || length > sizeof(wide) / sizeof(wide[0]))
		return;
	SzArEx_GetFileNameUtf16(db, index, wide);
	for (size_t place = 0; place + 1 < length; place++)
		if (wide[place] == '/' || wide[place] == '\\')
			start = place + 1;
	for (size_t place = start; place + 1 < length; place++)
	{
		if (wide[place] < 0x20 || wide[place] >= 0x7F || wide[place] == ':' || at + 1 >= size)
		{
			name[0] = '\0';
			return;
		}
		name[at++] = (char)wide[place];
	}
	name[at] = '\0';
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		name[0] = '\0';
}

static const char *problem_of(SRes result)
{
	switch (result)
	{
	case SZ_ERROR_NO_ARCHIVE: return "not a 7z archive";
	case SZ_ERROR_UNSUPPORTED: return "the 7z archive's method is not one the game reads (LZMA, LZMA2)";
	case SZ_ERROR_MEM: return "no memory for the 7z archive";
	case SZ_ERROR_CRC: return "the 7z archive is damaged (a CRC differs)";
	case SZ_ERROR_INPUT_EOF: return "the 7z archive is not whole";
	default: return "the 7z archive cannot be read";
	}
}

const char *rom_set_7z_unpack(const char *archive, const char *folder, rom_set_7z_unpacked *unpacked)
{
	static int crc_ready;
	CFileInStream archive_stream;
	CLookToRead2 look;
	CSzArEx db;
	SRes result;
	UInt32 block_index = 0xFFFFFFFFu;
	Byte *out_buffer = NULL;
	size_t out_buffer_size = 0;
	const char *problem = NULL;

	unpacked->count = 0;
	snprintf(unpacked->folder, sizeof(unpacked->folder), "%s", folder);
	if (!crc_ready)
	{
		CrcGenerateTable();
		crc_ready = 1;
	}
	if (InFile_Open(&archive_stream.file, archive) != 0)
		return "the 7z archive cannot be opened";
	FileInStream_CreateVTable(&archive_stream);
	archive_stream.wres = 0;
	LookToRead2_CreateVTable(&look, False);
	look.buf = (Byte *)ISzAlloc_Alloc(&g_alloc, LOOK_BUFFER);
	if (!look.buf)
	{
		File_Close(&archive_stream.file);
		return "no memory for the 7z archive";
	}
	look.bufSize = LOOK_BUFFER;
	look.realStream = &archive_stream.vt;
	LookToRead2_INIT(&look);
	SzArEx_Init(&db);
	result = SzArEx_Open(&db, &look.vt, &g_alloc, &g_alloc_temp);
	if (result != SZ_OK)
		problem = problem_of(result);
	else
	{
		char directory[1100];
		size_t length;

		/* (the folder made, without its separator) */
		snprintf(directory, sizeof(directory), "%s", folder);
		length = strlen(directory);
		while (length > 1 && (directory[length - 1] == '/' || directory[length - 1] == '\\'))
			directory[--length] = '\0';
		make_folder(directory);
	}
	for (UInt32 index = 0; !problem && index < db.NumFiles; index++)
	{
		char name[64], path[1200];
		size_t offset = 0, processed = 0;
		FILE *output;

		if (SzArEx_IsDir(&db, index))
			continue;
		file_name(&db, index, name, sizeof(name));
		if (!name[0])
			continue;    /* (no file of a set's) */
		if (unpacked->count >= ROM_SET_7Z_FILES)
		{
			problem = "the 7z archive has too many files for a ROM set";
			break;
		}
		result = SzArEx_Extract(&db, &look.vt, index, &block_index, &out_buffer, &out_buffer_size, &offset,
			&processed, &g_alloc, &g_alloc_temp);
		if (result != SZ_OK)
		{
			problem = problem_of(result);
			break;
		}
		snprintf(path, sizeof(path), "%s%s", unpacked->folder, name);
		if ((output = fopen(path, "wb")) == NULL)
		{
			problem = "the 7z archive's files cannot be written";
			break;
		}
		snprintf(unpacked->names[unpacked->count++], sizeof(unpacked->names[0]), "%s", name);
		if (fwrite(out_buffer + offset, 1, processed, output) != processed)
			problem = "the 7z archive's files cannot be written";
		if (fclose(output) != 0)
			problem = "the 7z archive's files cannot be written";
	}
	ISzAlloc_Free(&g_alloc, out_buffer);
	SzArEx_Free(&db, &g_alloc);
	ISzAlloc_Free(&g_alloc, look.buf);
	File_Close(&archive_stream.file);
	if (problem)
		rom_set_7z_remove(unpacked);
	return problem;
}

void rom_set_7z_remove(const rom_set_7z_unpacked *unpacked)
{
	char directory[1100];
	size_t length;

	for (int index = 0; index < unpacked->count; index++)
	{
		char path[1200];

		snprintf(path, sizeof(path), "%s%s", unpacked->folder, unpacked->names[index]);
		remove(path);
	}
	snprintf(directory, sizeof(directory), "%s", unpacked->folder);
	length = strlen(directory);
	while (length > 1 && (directory[length - 1] == '/' || directory[length - 1] == '\\'))
		directory[--length] = '\0';
	remove_folder(directory);    /* (only when empty: a folder the player had stays) */
}
