#include "ext_lib.h"
#include <miniaudio/miniaudio.h>

typedef struct {
    SoundCallback    callback;
    ma_device_config deviceConfig;
    ma_device    device;
    volatile u32 isPlaying;
    void* uctx;
} SoundCtx;

static void __Sound_Callback(ma_device* dev, void* output, const void* input, ma_uint32 frameCount) {
    SoundCtx* sndCtx = dev->pUserData;
    
    sndCtx->callback(sndCtx->uctx, output, frameCount);
}

void* Sound_Init(SoundFormat fmt, u32 sampleRate, u32 channelNum, SoundCallback callback, void* uCtx) {
    SoundCtx* soundCtx;
    
    soundCtx = Calloc(sizeof(SoundCtx));
    soundCtx->uctx = uCtx;
    
    soundCtx->deviceConfig = ma_device_config_init(ma_device_type_playback);
    soundCtx->deviceConfig.playback.format = (ma_format)fmt;
    soundCtx->deviceConfig.playback.channels = channelNum;
    soundCtx->deviceConfig.sampleRate = sampleRate;
    soundCtx->deviceConfig.dataCallback = __Sound_Callback;
    soundCtx->deviceConfig.pUserData = soundCtx;
    soundCtx->deviceConfig.periodSizeInFrames = 128;
    soundCtx->callback = callback;
    
    ma_device_init(NULL, &soundCtx->deviceConfig, &soundCtx->device);
    ma_device_start(&soundCtx->device);
    
    return soundCtx;
}

void Sound_Free(void* sound) {
    SoundCtx* soundCtx = sound;
    
    if (sound) {
        ma_device_stop(&soundCtx->device);
        ma_device_uninit(&soundCtx->device);
    }
}
