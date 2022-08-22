#include "ext_lib.h"
#include "xm.h"

void* gXmlSound;

static void __Sound_Xm_Play(xm_context_t* xm, void* output, u32 frameCount) {
	xm_generate_samples(xm, output, frameCount);
}

void Sound_Xm_Play(const void* data, u32 size) {
	xm_context_t* xm;
	
	if (xm_create_context_safe(&xm, data, size, 48000)) printf_error("Could not initialize XmPlayer");
	xm_set_max_loop_count(xm, 0);
	
	gXmlSound = Sound_Init(SOUND_F32, 48000, 2, (void*)__Sound_Xm_Play, xm);
}

void Sound_Xm_Stop() {
	Sound_Free(gXmlSound);
}