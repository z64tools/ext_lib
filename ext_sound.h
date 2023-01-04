#include <ext_lib.h>

void* audiodev_init(sound_fmt_t fmt, u32 sampleRate, u32 channelNum, sound_callback_t callback, void* uCtx);
void audiodev_free(void* sound);