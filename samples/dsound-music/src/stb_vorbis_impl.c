//------------------------------------------------------------------------------
// stb_vorbis implementation TU for the dsound-music sample.
//
// We decode from memory only (read the .ogg with the Win32 file API, then
// stb_vorbis_decode_memory), so disable stb_vorbis's stdio/file paths. The
// pushdata (streaming) API isn't used either.
//------------------------------------------------------------------------------
#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_NO_PUSHDATA_API

#include "stb_vorbis.c"
