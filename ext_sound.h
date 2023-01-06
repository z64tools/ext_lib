#include <ext_lib.h>

void* SoundDevice_Init(sound_fmt_t fmt, u32 sampleRate, u32 channelNum, sound_callback_t callback, void* uCtx);
void SoundDevice_Free(void* sound);