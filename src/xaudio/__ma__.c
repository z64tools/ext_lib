#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_WAV
#define MA_NO_FLAC
#define MA_NO_MP3
#define MA_NO_GENERATION

// #ifdef _WIN32
// #define MA_ENABLE_ONLY_SPECIFIC_BACKENDS MA_ENABLE_WINMM
// #else
// #define MA_ENABLE_ONLY_SPECIFIC_BACKENDS MA_ENABLE_ALSA
// #endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#include <miniaudio/miniaudio.h>

#pragma GCC diagnostic pop