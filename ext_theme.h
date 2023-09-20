#ifndef EXT_THEME_H
#define EXT_THEME_H
#include <nanovg/src/nanovg.h>
#include "ext_type.h"
#include "ext_macros.h"
#include "ext_math.h"
#include "ext_vector.h"

typedef enum {
	THEME_NONE,
	THEME_BASE,
	THEME_SPLIT,
	THEME_SHADOW,
	THEME_HIGHLIGHT,
	
	THEME_ELEMENT_LIGHT,
	THEME_ELEMENT_BASE,
	THEME_ELEMENT_DARK,
	
	THEME_PRIM,
	THEME_NEW,
	THEME_DELETE,
	THEME_TEXT,
	THEME_WARNING,
	
	THEME_MAX,
	
	THEME_DISABLED = THEME_BASE,
} ThemeColor;

void Theme_Init(u32 themeId);
void Theme_SmoothStepToCol(NVGcolor* src, NVGcolor target, f32 a, f32 b, f32 c);
NVGcolor Theme_GetColor(ThemeColor, s32, f32);
NVGcolor Theme_Mix(f32 v, NVGcolor a, NVGcolor b);

void Theme_SetColor(rgb8_t color, int id);

#define UnfoldNVGcolor(color) ((color).r * 255), ((color).g * 255), ((color).b * 255)

#endif