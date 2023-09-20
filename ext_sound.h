#include <ext_lib.h>

typedef void (*sound_callback_t)(void*, void*, u32);

typedef enum {
	SOUND_S16 = 2,
	SOUND_S24,
	SOUND_S32,
	SOUND_F32,
} sound_fmt_t;

void* SoundDevice_Init(sound_fmt_t fmt, u32 sampleRate, u32 channelNum, sound_callback_t callback, void* uCtx);
void SoundDevice_Free(void* sound);