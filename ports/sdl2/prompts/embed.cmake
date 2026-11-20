# Name:        embed.cmake
# Description: CONTROLS' pictures (input_prompts.bmp: Kenney's Input Prompts Pixel, CC0) as a C array, to build into
#              the games that have no file beside them
# Version:     0.1.0
# Created:     19-11-2026
# Modified:    19-11-2026
# Author:      Sebastian Toyos
#
# Usage: cmake -DSHEET=input_prompts.bmp -DOUT=prompts_sheet.c -P embed.cmake
# The array is the .bmp as it is: main.c reads it with SDL as it reads the file elsewhere (load_prompts,
# SIMPSONS_PROMPTS_EMBEDDED). Bytes, not a string: MSVC takes no string literal longer than 64 KB. As
# ports/sdl2/controllerdb/embed.cmake.

file(READ "${SHEET}" hex HEX)
string(LENGTH "${hex}" digits)
math(EXPR size "${digits} / 2")
string(REPEAT "[0-9a-f]" 48 row)    # (24 bytes a row: CMake's expressions have no {n})
string(REGEX REPLACE "(${row})" "\\1\n" hex "${hex}")
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," bytes "${hex}")
string(REPLACE "\n" "\n\t" bytes "${bytes}")
file(WRITE "${OUT}" "/* CONTROLS' pictures (ports/sdl2/prompts/input_prompts.bmp: Kenney's Input Prompts Pixel, CC0), written by
   ports/sdl2/prompts/embed.cmake: the .bmp's ${size} bytes */
#include <stddef.h>

const unsigned char simpsons_prompts_bmp[] = {
\t${bytes}
};
const size_t simpsons_prompts_bmp_size = ${size};
")
