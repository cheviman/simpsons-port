# LZMA SDK's 7z decoder

The files here are the C decoder of the [LZMA SDK](https://www.7-zip.org/sdk.html) 26.06 (2026-11-06), unchanged,
from its release `lzma2606.7z` (github.com/ip7z/7zip, release 26.06): what the 7z reader needs to read an archive and
decode its LZMA, LZMA2, BCJ, BCJ2 and Delta streams (the PPMd method and the encoders left out). The games read a ROM
set packed as `.7z` with it (`ports/sdl2/rom_set_7z.c`).

The LZMA SDK is written and placed in the public domain by Igor Pavlov:

> Anyone is free to copy, modify, publish, use, compile, sell, or distribute the original LZMA SDK code, either in
> source code form or as a compiled binary, for any purpose, commercial or non-commercial, and by any means.
