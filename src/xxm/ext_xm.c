#include <ext_lib.h>
#include <ext_sound.h>
#include "xm.h"

void* sXmSound;

void FastTracker_Play(const void* data, u32 size) {
    xm_context_t* xm;
    
    if (xm_create_context_safe(&xm, data, size, 48000)) errr("Could not initialize XmPlayer");
    xm_set_max_loop_count(xm, 0);
    
    sXmSound = SoundDevice_Init(SOUND_F32, 48000, 2, (void*)xm_generate_samples, xm);
}

void FastTracker_Stop() {
    SoundDevice_Free(sXmSound);
}
