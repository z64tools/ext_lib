#include "ext_lib.h"
#include <miniaudio/miniaudio.h>

typedef struct {
    sound_callback_t    callback;
    ma_device_config deviceConfig;
    ma_device device;
    void*     uctx;
} SoundCtx;

static void __audiodev_callback(ma_device* dev, void* output, const void* input, ma_uint32 frameCount) {
    SoundCtx* sndCtx = dev->pUserData;
    
    sndCtx->callback(sndCtx->uctx, output, frameCount);
}

void* audiodev_init(sound_fmt_t fmt, u32 sampleRate, u32 channelNum, sound_callback_t callback, void* uCtx) {
    SoundCtx* soundCtx;
    
    soundCtx = calloc(sizeof(SoundCtx));
    soundCtx->uctx = uCtx;
    
    soundCtx->deviceConfig = ma_device_config_init(ma_device_type_playback);
    soundCtx->deviceConfig.playback.format = (ma_format)fmt;
    soundCtx->deviceConfig.playback.channels = channelNum;
    soundCtx->deviceConfig.sampleRate = sampleRate;
    soundCtx->deviceConfig.dataCallback = __audiodev_callback;
    soundCtx->deviceConfig.pUserData = soundCtx;
    soundCtx->deviceConfig.periodSizeInFrames = 512;
    soundCtx->callback = callback;
    
    ma_device_init(NULL, &soundCtx->deviceConfig, &soundCtx->device);
    ma_device_start(&soundCtx->device);
    
    return soundCtx;
}

void audiodev_free(void* sound) {
    SoundCtx* soundCtx = sound;
    
    if (sound) {
        ma_device_stop(&soundCtx->device);
        ma_device_uninit(&soundCtx->device);
    }
}
