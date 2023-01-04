#include <ext_lib.h>
#include <ext_sound.h>
#include "xm.h"

void* sXmSound;

void tracker_xm_play(const void* data, u32 size) {
    xm_context_t* xm;
    
    if (xm_create_context_safe(&xm, data, size, 48000)) print_error("Could not initialize XmPlayer");
    xm_set_max_loop_count(xm, 0);
    
    sXmSound = audiodev_init(SOUND_F32, 48000, 2, (void*)xm_generate_samples, xm);
}

void tracker_xm_stop() {
    audiodev_free(sXmSound);
}
