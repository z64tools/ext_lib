#ifndef __Z64GUI_H__
#define __Z64GUI_H__
#include <ExtLib.h>
#include <nanovg/src/nanovg.h>

typedef enum {
	THEME_BASE,
	THEME_SHADOW,
	THEME_HIGHLIGHT,
	
	THEME_ELEMENT_LIGHT,
	THEME_ELEMENT_BASE,
	THEME_ELEMENT_DARK,
	
	THEME_PRIM,
	THEME_TEXT,
	
	THEME_MAX,
	
	THEME_DISABLED = THEME_BASE,
} ThemeColor;

void Theme_Init(u32 themeId);
void Theme_SmoothStepToCol(NVGcolor* src, NVGcolor target, f32 a, f32 b, f32 c);
NVGcolor Theme_GetColor(ThemeColor, s32, f32);
NVGcolor Theme_Mix(f32 v, NVGcolor a, NVGcolor b);

#define UnfoldNVGcolor(color) (color.r * 255), (color.g * 255), (color.b * 255)

#endif