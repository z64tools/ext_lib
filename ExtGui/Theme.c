#include "Interface.h"

u32 sIndex;
HSLA8 sDarkTheme[THEME_MAX] = { 0 };
HSLA8 sLightTheme[THEME_MAX] = { 0 };
HSLA8* sTheme[] = {
	sDarkTheme,
	sLightTheme,
};

NVGcolor Theme_GetColor(ThemeColor pal, s32 alpha, f32 mult) {
	HSLA8 col = sTheme[sIndex][pal];
	
	if (sIndex == 1)
		mult = 1 / mult;
	
	return nvgHSLA(col.h, col.s * mult, col.l * mult, ClampMax(alpha, 255));
}

void Theme_SmoothStepToCol(NVGcolor* src, NVGcolor target, f32 a, f32 b, f32 c) {
	Math_DelSmoothStepToF(&src->r, target.r, a, b, c);
	Math_DelSmoothStepToF(&src->g, target.g, a, b, c);
	Math_DelSmoothStepToF(&src->b, target.b, a, b, c);
	Math_DelSmoothStepToF(&src->a, target.a, a, b, c);
}

void Theme_Init(u32 themeId) {
	sIndex = themeId;
	{
		sDarkTheme[THEME_BASE] =          (HSLA8) { 000.0 / 360, 0.0, 0.15 };
		sDarkTheme[THEME_SHADOW] =        (HSLA8) { 000.0 / 360, 0.0, 0.07 };
		sDarkTheme[THEME_HIGHLIGHT] =     (HSLA8) { 000.0 / 360, 0.0, 0.80 };
		sDarkTheme[THEME_ELEMENT_LIGHT] = (HSLA8) { 000.0 / 360, 0.0, 0.60 };
		sDarkTheme[THEME_ELEMENT_BASE] =  (HSLA8) { 000.0 / 360, 0.0, 0.16 };
		sDarkTheme[THEME_ELEMENT_DARK] =  (HSLA8) { 000.0 / 360, 0.0, 0.10 };
		sDarkTheme[THEME_PRIM] =          (HSLA8) { 220.0 / 360, 0.7, 0.60 };
		sDarkTheme[THEME_TEXT] =          (HSLA8) { 000.0 / 360, 0.0, 0.90 };
	}
	{
	}
}