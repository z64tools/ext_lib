#include <ext_lib.h>
#include <ext_sound.h>
#include "xm.h"

void* sXmSound;

typedef struct {
    size_t _0;
    struct {
        u16   _0[5];
        int   _1;
        u8    _2[256];
        void* _3[2];
    } _1;
    u32 rate;
    u16 tempo;
    u16 bpm;
    f32 volume;
    f32 amp;
} FastTracker;

vbool sPlayFlag;
vbool sPlayStop;
static f32 sVolume = 1.0f;

void FastTracker_Player(void* ctx, void* output, size_t num) {
    FastTracker* xm = ctx;
    static u8 stopper;
    static u16 tempo;
    
    if (!sPlayFlag) {
        if (stopper != 35)
            stopper++;
        
        xm->volume = lerpf(invertf(powf(stopper / 35.f, 4.0f)), 0.0f, sVolume);
        xm->rate = lerpf(invertf(powf(stopper / 35.f, 4.0f)), 22050 * 32, 22050);
        xm->bpm = lerpf(invertf(powf(stopper / 35.f, 4.0f)), tempo * 32, tempo);
        
        if (stopper == 35)
            sPlayStop = true;
    } else {
        xm->volume = sVolume;
        tempo = xm->bpm;
    }
    
    xm_generate_samples(ctx, output, num);
}

void FastTracker_Play(const void* data, u32 size) {
    xm_context_t* xm;
    
    if (xm_create_context_safe(&xm, data, size, 22050)) errr("Could not initialize XmPlayer");
    xm_set_max_loop_count(xm, 0);
    
    sPlayFlag = true;
    sXmSound = SoundDevice_Init(SOUND_F32, 22050, 2, FastTracker_Player, xm);
}

void FastTracker_SetVolume(f32 vol) {
    sVolume = vol;
}

void FastTracker_Stop() {
    
    if (sPlayFlag)
        while (!sPlayStop)
            sPlayFlag = false;
    
    SoundDevice_Free(sXmSound);
}