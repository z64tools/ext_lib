#define __GEO_ELEM_C__

#include "GeoGrid.h"
#include "Interface.h"

struct ElementCallInfo;

typedef void (* ElementFunc)(struct ElementCallInfo*);

typedef struct ElementCallInfo {
	void*       arg;
	Split*      split;
	ElementFunc func;
	GeoGrid*    geo;
} ElementCallInfo;

/* ───────────────────────────────────────────────────────────────────────── */

static ElementCallInfo pElementStack[1024 * 2];
static ElementCallInfo* sCurrentElement;
static u32 sElemNum;

static ElTextbox* sCurTextbox;
static Split* sCurSplitTextbox;
static s32 sTextPos;
static s32 sSelectPos = -1;
static char* sStoreA;
static s32 sCtrlA = 0;

static s32 sFlickTimer;
static s32 sFlickFlag = 1;

static s16 sBreatheYaw;
static f32 sBreathe;
static const char* sFmt[] = {
	"%.3f",
	"%d"
};

/* ───────────────────────────────────────────────────────────────────────── */

static void Element_QueueElement(GeoGrid* geo, Split* split, ElementFunc func, void* arg) {
	sCurrentElement->geo = geo;
	sCurrentElement->split = split;
	sCurrentElement->func = func;
	sCurrentElement->arg = arg;
	sCurrentElement++;
	sElemNum++;
}

static void Element_Draw_RoundedOutline(void* vg, Rect* rect, NVGcolor color) {
	nvgBeginPath(vg);
	nvgFillColor(vg, color);
	nvgRoundedRect(
		vg,
		rect->x - 1,
		rect->y - 1,
		rect->w + 2,
		rect->h + 2,
		SPLIT_ROUND_R
	);
	nvgFill(vg);
}

static void Element_Draw_RoundedRect(void* vg, Rect* rect, NVGcolor color) {
	nvgBeginPath(vg);
	nvgFillColor(vg, color);
	nvgRoundedRect(
		vg,
		rect->x,
		rect->y,
		rect->w,
		rect->h,
		SPLIT_ROUND_R
	);
	nvgFill(vg);
}

static s32 Element_PressCondition(GeoGrid* geo, Split* split, Rect* rect) {
	return (!geo->state.noClickInput &&
	       split->mouseInSplit &&
	       !split->blockMouse &&
	       !split->elemBlockMouse &&
	       GeoGrid_Cursor_InRect(split, rect));
}

static void Element_Draw_TextOutline(void* vg, f32 x, f32 y, char* txt) {
	nvgFillColor(vg, Theme_GetColor(THEME_SHADOW, 255, 1.0f));
	nvgFontBlur(vg, 1.0);
	
	nvgText(
		vg,
		x,
		y,
		txt,
		NULL
	);
	nvgText(
		vg,
		x,
		y,
		txt,
		NULL
	);
}

static void Element_Slider_SetCursorToVal(GeoGrid* geo, Split* split, ElSlider* this) {
	f32 x = split->rect.x + this->element.rect.x + this->element.rect.w * this->value;
	f32 y = split->rect.y + this->element.rect.y + this->element.rect.h * 0.5;
	
	Input_SetMousePos(geo->input, x, y);
}

static void Element_Slider_SetTextbox(Split* split, ElSlider* this) {
	sCurTextbox = &this->textBox;
	sCurSplitTextbox = split;
	sCtrlA = 1;
	
	this->isTextbox = true;
	
	this->textBox.isNumBox = true;
	this->textBox.element.rect = this->element.rect;
	this->textBox.align = ALIGN_CENTER;
	this->textBox.size = 32;
	
	this->textBox.nbx.isInt = this->isInt;
	this->textBox.nbx.max = this->max;
	this->textBox.nbx.min = this->min;
	
	this->textBox.isHintText = 2;
	sTextPos = 0;
	sSelectPos = strlen(this->textBox.txt);
}

/* ───────────────────────────────────────────────────────────────────────── */

static void Element_Draw_Button(ElementCallInfo* info) {
	void* vg = info->geo->vg;
	ElButton* this = info->arg;
	f32 dsbleMul = (this->isDisabled ? 0.5 : 1.0);
	
	if (this->element.rect.w < 16) {
		return;
	}
	
	Element_Draw_RoundedOutline(vg, &this->element.rect, this->element.light);
	
	nvgBeginPath(vg);
	nvgFillColor(vg, this->element.base);
	nvgRoundedRect(vg, this->element.rect.x, this->element.rect.y, this->element.rect.w, this->element.rect.h, SPLIT_ROUND_R);
	nvgFill(vg);
	
	if (this->element.name) {
		char* txt = (char*)this->element.name;
		char ftxt[512];
		f32 bounds[4];
		
		nvgFontFace(vg, "dejavu");
		nvgFontSize(vg, SPLIT_TEXT);
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgTextLetterSpacing(vg, 0.0);
		nvgTextBounds(vg, 0, 0, txt, NULL, bounds);
		
		if (bounds[2] > this->element.rect.w - SPLIT_TEXT_PADDING * 2) {
			strcpy(ftxt, txt);
			txt = ftxt;
			
			while (bounds[2] > this->element.rect.w - SPLIT_TEXT_PADDING * 2) {
				txt[strlen(txt) - 1] = '\0';
				nvgTextBounds(vg, 0, 0, txt, NULL, bounds);
			}
		}
		
		nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		
		nvgFillColor(vg, Theme_GetColor(THEME_TEXT, 255, 1.0f - 0.25f * this->isDisabled));
		nvgFontBlur(vg, 0.0);
		nvgText(
			vg,
			this->element.rect.x + this->element.rect.w * 0.5,
			this->element.rect.y + this->element.rect.h * 0.5,
			txt,
			NULL
		);
	}
}

static void Element_Draw_Textbox(ElementCallInfo* info) {
	void* vg = info->geo->vg;
	GeoGrid* geo = info->geo;
	Split* split = info->split;
	ElTextbox* this = info->arg;
	
	Assert(this->txt != NULL);
	static char buffer[512]; strcpy(buffer, this->txt);
	Rectf32 bound = { 0 };
	char* txtA = buffer;
	char* txtB = &buffer[strlen(buffer)];
	f32 ccx = 0;
	
	if (this->element.rect.w < 16) {
		return;
	}
	
	if (this->isNumBox) {
		if (this == sCurTextbox) {
			this->nbx.updt = true;
		} else {
			if (this->nbx.updt) {
				this->nbx.value = this->nbx.isInt ? Value_Int(this->txt) : Value_Float(this->txt);
				if (this->nbx.min != 0 || this->nbx.max != 0)
					this->nbx.value = Clamp(this->nbx.value, this->nbx.min, this->nbx.max);
				snprintf(
					this->txt,
					31,
					sFmt[this->nbx.isInt],
					this->nbx.isInt ? Value_Int(this->txt) : Value_Float(this->txt)
				);
			}
		}
	}
	
	nvgFontFace(vg, "dejavu");
	nvgFontSize(vg, SPLIT_TEXT);
	nvgFontBlur(vg, 0.0);
	nvgTextLetterSpacing(vg, 0.1);
	
	switch (this->align) {
		case ALIGN_LEFT:
			break;
		case ALIGN_CENTER:
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
			nvgTextBounds(vg, 0, 0, txtA, txtB, (f32*)&bound);
			ccx = this->element.rect.w * 0.5 + bound.x - SPLIT_TEXT_PADDING;
			break;
		case ALIGN_RIGHT:
			nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
			nvgTextBounds(vg, 0, 0, txtA, txtB, (f32*)&bound);
			ccx = this->element.rect.w + bound.x - SPLIT_TEXT_PADDING * 2;
			break;
	}
	
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgTextBounds(vg, 0, 0, txtA, txtB, (f32*)&bound);
	
	if (sStoreA != NULL && this == sCurTextbox) {
		if (bound.w < this->element.rect.w) {
			sStoreA = buffer;
		} else {
			txtA = sStoreA;
			nvgTextBounds(vg, ccx, 0, txtA, txtB, (f32*)&bound);
			while (bound.w > this->element.rect.w - SPLIT_TEXT_PADDING * 2) {
				txtB--;
				nvgTextBounds(vg, ccx, 0, txtA, txtB, (f32*)&bound);
			}
			
			if (strlen(buffer) > 4) {
				s32 posA = (uptr)txtA - (uptr)buffer;
				s32 posB = (uptr)txtB - (uptr)buffer;
				
				if (sTextPos < posA || sTextPos > posB + 1 || posB - posA <= 4) {
					if (sTextPos < posA) {
						txtA -= posA - sTextPos;
					}
					if (sTextPos > posB + 1) {
						txtA += sTextPos - posB + 1;
					}
					if (posB - posA <= 4)
						txtA += (posB - posA) - 4;
					
					txtB = &buffer[strlen(buffer)];
					sStoreA = txtA;
					
					nvgTextBounds(vg, ccx, 0, txtA, txtB, (f32*)&bound);
					while (bound.w > this->element.rect.w - SPLIT_TEXT_PADDING * 2) {
						txtB--;
						nvgTextBounds(vg, ccx, 0, txtA, txtB, (f32*)&bound);
					}
				}
			}
		}
	} else {
		while (bound.w > this->element.rect.w - SPLIT_TEXT_PADDING * 2) {
			txtB--;
			nvgTextBounds(vg, 0, 0, txtA, txtB, (f32*)&bound);
		}
		
		if (this == sCurTextbox)
			sStoreA = txtA;
	}
	
	Input* inputCtx = info->geo->input;
	
	if (GeoGrid_Cursor_InRect(split, &this->element.rect) && inputCtx->mouse.clickL.hold) {
		if (this->isHintText) {
			this->isHintText = 2;
			sTextPos = 0;
			sSelectPos = strlen(this->txt);
		} else {
			if (Input_GetMouse(geo->input, MOUSE_L)->press) {
				f32 dist = 400;
				for (char* tempB = txtA; tempB <= txtB; tempB++) {
					Vec2s glyphPos;
					f32 res;
					nvgTextBounds(vg, ccx, 0, txtA, tempB, (f32*)&bound);
					glyphPos.x = this->element.rect.x + bound.w + SPLIT_TEXT_PADDING - 1;
					glyphPos.y = this->element.rect.y + bound.h - 1 + SPLIT_TEXT * 0.5;
					
					res = Math_Vec2s_DistXZ(&split->mousePos, &glyphPos);
					
					if (res < dist) {
						dist = res;
						
						sTextPos = (uptr)tempB - (uptr)buffer;
					}
				}
			} else {
				f32 dist = 400;
				uptr wow = 0;
				for (char* tempB = txtA; tempB <= txtB; tempB++) {
					Vec2s glyphPos;
					f32 res;
					nvgTextBounds(vg, ccx, 0, txtA, tempB, (f32*)&bound);
					glyphPos.x = this->element.rect.x + bound.w + SPLIT_TEXT_PADDING - 1;
					glyphPos.y = this->element.rect.y + bound.h - 1 + SPLIT_TEXT * 0.5;
					
					res = Math_Vec2s_DistXZ(&split->mousePos, &glyphPos);
					
					if (res < dist) {
						dist = res;
						wow = (uptr)tempB - (uptr)buffer;
					}
				}
				
				if (wow != sTextPos) {
					sSelectPos = wow;
				}
			}
		}
	} else if (this->isHintText == 2) this->isHintText = 0;
	
	Element_Draw_RoundedOutline(vg, &this->element.rect, this->element.light);
	
	nvgBeginPath(vg);
	nvgFillColor(vg, Theme_GetColor(THEME_ELEMENT_BASE, 255, 0.75f));
	nvgRoundedRect(vg, this->element.rect.x, this->element.rect.y, this->element.rect.w, this->element.rect.h, SPLIT_ROUND_R);
	nvgFill(vg);
	
	if (this == sCurTextbox) {
		if (sSelectPos != -1) {
			s32 min = fmin(sSelectPos, sTextPos);
			s32 max = fmax(sSelectPos, sTextPos);
			f32 x, xmax;
			
			if (txtA < &buffer[min]) {
				nvgTextBounds(vg, 0, 0, txtA, &buffer[min], (f32*)&bound);
				x = this->element.rect.x + bound.w + SPLIT_TEXT_PADDING - 1;
			} else {
				x = this->element.rect.x + SPLIT_TEXT_PADDING;
			}
			
			nvgTextBounds(vg, 0, 0, txtA, &buffer[max], (f32*)&bound);
			xmax = this->element.rect.x + bound.w + SPLIT_TEXT_PADDING - 1 - x;
			
			nvgBeginPath(vg);
			nvgFillColor(vg, Theme_GetColor(THEME_PRIM, 255, 1.0f));
			nvgRoundedRect(vg, x + ccx, this->element.rect.y + 2, ClampMax(xmax, this->element.rect.w - SPLIT_TEXT_PADDING - 2), this->element.rect.h - 4, SPLIT_ROUND_R);
			nvgFill(vg);
		} else {
			sFlickTimer++;
			
			if (sFlickTimer % 30 == 0)
				sFlickFlag ^= 1;
			
			if (sFlickFlag) {
				nvgBeginPath(vg);
				nvgTextBounds(vg, 0, 0, txtA, &buffer[sTextPos], (f32*)&bound);
				nvgBeginPath(vg);
				nvgFillColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 255, 1.0f));
				nvgRoundedRect(vg, this->element.rect.x + bound.w + SPLIT_TEXT_PADDING - 1 + ccx, this->element.rect.y + bound.h - 3, 2, SPLIT_TEXT, SPLIT_ROUND_R);
				nvgFill(vg);
			}
		}
	} else {
		if (&buffer[strlen(buffer)] != txtB)
			for (s32 i = 0; i < 3; i++)
				txtB[-i] = '.';
	}
	
	if (this->isHintText)
		nvgFillColor(vg, Theme_GetColor(THEME_TEXT, 125, 1.0f));
	else
		nvgFillColor(vg, Theme_GetColor(THEME_TEXT, 255, 1.0f));
	nvgText(
		vg,
		this->element.rect.x + SPLIT_TEXT_PADDING + ccx,
		this->element.rect.y + this->element.rect.h * 0.5,
		txtA,
		txtB
	);
}

static void Element_Draw_Text(ElementCallInfo* info) {
	void* vg = info->geo->vg;
	// Split* split = info->split;
	ElText* this = info->arg;
	f32 bounds[4];
	char tempText[512];
	
	strcpy(tempText, this->txt);
	
	nvgFontFace(vg, "dejavu");
	nvgFontSize(vg, SPLIT_TEXT);
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgTextLetterSpacing(vg, 0.1);
	
	nvgTextBounds(vg, 0, 0, this->txt, 0, bounds);
	
	while (bounds[2] > this->element.rect.w) {
		strcpy(tempText + ClampMin(strlen(tempText) - 3, 0), "..");
		
		nvgTextBounds(vg, 0, 0, tempText, 0, bounds);
	}
	
	nvgFontBlur(vg, 0.0);
	nvgFillColor(vg, this->element.texcol);
	nvgText(
		vg,
		this->element.rect.x,
		this->element.rect.y + this->element.rect.h * 0.5,
		tempText,
		NULL
	);
}

static void Element_Draw_Checkbox(ElementCallInfo* info) {
	void* vg = info->geo->vg;
	// Split* split = info->split;
	ElCheckbox* this = info->arg;
	Vec2f center;
	const Vec2f sVector_Cross[] = {
		{ .x = -10, .y =  10 }, { .x =  -7, .y =  10 },
		{ .x =   0, .y =   3 }, { .x =   7, .y =  10 },
		{ .x =  10, .y =  10 }, { .x =  10, .y =   7 },
		{ .x =   3, .y =   0 }, { .x =  10, .y =  -7 },
		{ .x =  10, .y = -10 }, { .x =   7, .y = -10 },
		{ .x =   0, .y =  -3 }, { .x =  -7, .y = -10 },
		{ .x = -10, .y = -10 }, { .x = -10, .y =  -7 },
		{ .x =  -3, .y =   0 }, { .x = -10, .y =   7 },
	};
	
	if (this->element.toggle)
		Math_DelSmoothStepToF(&this->lerp, 0.8f - sBreathe * 0.08, 0.178f, 0.1f, 0.0f);
	else
		Math_DelSmoothStepToF(&this->lerp, 0.0f, 0.268f, 0.1f, 0.0f);
	
	Element_Draw_RoundedOutline(vg, &this->element.rect, this->element.light);
	Element_Draw_RoundedRect(vg, &this->element.rect, this->element.base);
	
	NVGcolor col = this->element.base;
	f32 flipLerp = 1.0f - this->lerp;
	
	flipLerp = (1.0f - powf(flipLerp, 1.6));
	center.x = this->element.rect.x + this->element.rect.w * 0.5;
	center.y = this->element.rect.y + this->element.rect.h * 0.5;
	
	col.a = flipLerp * 1.67;
	col.a = ClampMin(col.a, 0.80);
	
	nvgBeginPath(vg);
	nvgFillColor(vg, col);
	
	for (s32 i = 0; i < ArrayCount(sVector_Cross); i++) {
		s32 wi = WrapS(i, 0, ArrayCount(sVector_Cross) - 1);
		Vec2f zero = { 0 };
		Vec2f pos = {
			sVector_Cross[wi].x * 0.75,
			sVector_Cross[wi].y * 0.75,
		};
		f32 dist = Math_Vec2f_DistXZ(&zero, &pos);
		s16 yaw = Math_Vec2f_Yaw(&zero, &pos);
		
		dist = Lerp(flipLerp, 4, dist);
		dist = Lerp((this->lerp > 0.5 ? 1.0 - this->lerp : this->lerp), dist, powf((dist * 0.1), 0.15) * 3);
		
		pos.x = center.x + SinS(yaw) * dist;
		pos.y = center.y + CosS(yaw) * dist;
		
		if ( i == 0 )
			nvgMoveTo(vg, pos.x, pos.y);
		else
			nvgLineTo(vg, pos.x, pos.y);
	}
	nvgFill(vg);
}

static void Element_Draw_Slider(ElementCallInfo* info) {
	void* vg = info->geo->vg;
	// Split* split = info->split;
	ElSlider* this = info->arg;
	Rectf32 rect;
	
	Math_DelSmoothStepToF(&this->vValue, this->value, 0.5f, (this->max - this->min) * 0.5f, 0.0f);
	
	rect.x = this->element.rect.x;
	rect.y = this->element.rect.y;
	rect.w = this->element.rect.w;
	rect.h = this->element.rect.h;
	rect.w = ClampMin(rect.w * this->vValue, 0);
	
	Element_Draw_RoundedOutline(vg, &this->element.rect, this->element.light);
	Element_Draw_RoundedRect(vg, &this->element.rect, this->element.shadow);
	
	if (this->vValue <= 0.01) {
		this->element.base.a = Lerp(ClampMin(this->vValue * 100.0f, 0.0f), 0.0, this->element.base.a);
	}
	
	nvgBeginPath(vg);
	nvgFillColor(vg, this->element.prim);
	nvgRoundedRect(
		vg,
		rect.x + 1,
		rect.y + 1,
		rect.w - 2,
		rect.h - 2,
		SPLIT_ROUND_R
	);
	nvgFill(vg);
	
	if (this->isInt) {
		snprintf(
			this->textBox.txt,
			31,
			sFmt[this->isInt],
			(s32)rint(Lerp(this->value, this->min, this->max))
		);
	} else {
		snprintf(
			this->textBox.txt,
			31,
			sFmt[this->isInt],
			Lerp(this->value, this->min, this->max)
		);
	}
	
	nvgFontFace(vg, "dejavu");
	nvgFontSize(vg, SPLIT_TEXT);
	nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
	
	nvgFillColor(vg, this->element.texcol);
	nvgFontBlur(vg, 0.0);
	nvgText(
		vg,
		this->element.rect.x + this->element.rect.w * 0.5,
		this->element.rect.y + this->element.rect.h * 0.5,
		this->textBox.txt,
		NULL
	);
}

/* ───────────────────────────────────────────────────────────────────────── */

// Returns button state, 0bXY, X == toggle, Y == pressed
s32 Element_Button(GeoGrid* geo, Split* split, ElButton* this) {
	void* vg = geo->vg;
	
	this->hover = 0;
	this->state = 0;
	
	if (this->autoWidth) {
		f32 bounds[4] = { 0.0f };
		
		nvgFontFace(vg, "dejavu");
		nvgFontSize(vg, SPLIT_TEXT);
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgTextLetterSpacing(vg, 0.0);
		nvgTextBounds(vg, 0, 0, this->element.name, NULL, bounds);
		
		this->element.rect.w = bounds[2] + SPLIT_TEXT_PADDING * 2;
	}
	
	if (Element_PressCondition(geo, split, &this->element.rect) && this->isDisabled == false) {
		if (Input_GetMouse(geo->input, MOUSE_L)->press) {
			this->state++;
			
			if (this->element.toggle) {
				u8 t = (this->element.toggle - 1) == 0; // Invert
				
				this->element.toggle = t + 1;
			}
		}
		
		if (Input_GetMouse(geo->input, MOUSE_L)->hold) {
			this->state++;
		}
		
		this->hover = 1;
	}
	
	Element_QueueElement(
		geo,
		split,
		Element_Draw_Button,
		this
	);
	
	return (this->state == 2) | (ClampMin(this->element.toggle - 1, 0)) << 4;
}

void Element_Textbox(GeoGrid* geo, Split* split, ElTextbox* this) {
	this->hover = 0;
	if (Element_PressCondition(geo, split, &this->element.rect)) {
		this->hover = 1;
		if (Input_GetMouse(geo->input, MOUSE_L)->press) {
			if (this != sCurTextbox) {
				sCtrlA = 1;
				this->isHintText = 2;
				sTextPos = 0;
				sSelectPos = strlen(this->txt);
			}
			
			sCurTextbox = this;
			sCurSplitTextbox = split;
		}
	}
	
	Element_QueueElement(
		geo,
		split,
		Element_Draw_Textbox,
		this
	);
}

// Returns text width
f32 Element_Text(GeoGrid* geo, Split* split, ElText* this) {
	f32 bounds[4] = { 0 };
	void* vg = geo->vg;
	
	nvgFontFace(vg, "dejavu");
	nvgFontSize(vg, SPLIT_TEXT);
	nvgFontBlur(vg, 0.0);
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgTextLetterSpacing(vg, 0.1);
	nvgFillColor(vg, Theme_GetColor(THEME_TEXT, 255, 1.0f));
	nvgTextBounds(vg, 0, 0, this->txt, NULL, bounds);
	
	Element_QueueElement(
		geo,
		split,
		Element_Draw_Text,
		this
	);
	
	return bounds[2];
}

s32 Element_Checkbox(GeoGrid* geo, Split* split, ElCheckbox* this) {
	this->element.rect.w = this->element.rect.h;
	
	this->hover = 0;
	if (Element_PressCondition(geo, split, &this->element.rect)) {
		this->hover = 1;
		if (Input_GetMouse(geo->input, MOUSE_L)->press) {
			this->element.toggle ^= 1;
		}
	}
	
	Element_QueueElement(
		geo,
		split,
		Element_Draw_Checkbox,
		this
	);
	
	return this->element.toggle;
}

f32 Element_Slider(GeoGrid* geo, Split* split, ElSlider* this) {
	if (this->min == 0.0f && this->max == 0.0f)
		this->max = 1.0f;
	
	this->hover = false;
	
	if (this->locked)
		goto queue_element;
	
	if (Element_PressCondition(geo, split, &this->element.rect) || this->holdState) {
		u32 pos = false;
		this->hover = true;
		
		if (this->isTextbox) {
			this->isTextbox = false;
			if (sCurTextbox == &this->textBox) {
				this->isTextbox = true;
				this->holdState = true;
				
				Element_Textbox(geo, split, &this->textBox);
				
				return Lerp(this->value, this->min, this->max);
			} else {
				Element_Slider_SetValue(this, this->isInt ? Value_Int(this->textBox.txt) : Value_Float(this->textBox.txt));
				
				goto queue_element;
			}
		}
		
		if (Input_GetMouse(geo->input, MOUSE_L)->press) {
			this->holdState = true;
			split->elemBlockMouse++;
		} else if (Input_GetMouse(geo->input, MOUSE_L)->hold && this->holdState) {
			if (geo->input->mouse.vel.x) {
				if (this->isSliding == false) {
					Element_Slider_SetCursorToVal(geo, split, this);
				} else {
					if (Input_GetKey(geo->input, KEY_LEFT_SHIFT)->hold)
						this->value += (f32)geo->input->mouse.vel.x * 0.0001f;
					else
						this->value += (f32)geo->input->mouse.vel.x * 0.001f;
					if (this->min || this->max)
						this->value = Clamp(this->value, 0.0f, 1.0f);
					
					pos = true;
				}
				
				this->isSliding = true;
			}
		} else if (Input_GetMouse(geo->input, MOUSE_L)->release && this->holdState) {
			if (this->isSliding == false && GeoGrid_Cursor_InRect(split, &this->element.rect)) {
				Element_Slider_SetTextbox(split, this);
			}
			this->isSliding = false;
		} else {
			if (this->holdState)
				split->elemBlockMouse--;
			this->holdState = false;
		}
		
		if (geo->input->mouse.scrollY) {
			if (Input_GetKey(geo->input, KEY_LEFT_SHIFT)->hold) {
				this->value += geo->input->mouse.scrollY * 0.1;
			} else if (Input_GetKey(geo->input, KEY_LEFT_ALT)->hold) {
				this->value += geo->input->mouse.scrollY * 0.001;
			} else {
				this->value += geo->input->mouse.scrollY * 0.01;
			}
		}
		
		if (pos) Element_Slider_SetCursorToVal(geo, split, this);
	}
	
queue_element:
	this->value = Clamp(this->value, 0.0f, 1.0f);
	
	if (this->isSliding)
		Cursor_SetCursor(CURSOR_EMPTY);
	
	Element_QueueElement(
		geo,
		split,
		Element_Draw_Slider,
		this
	);
	
	if (this->isInt)
		return (s32)rint(Lerp(this->value, this->min, this->max));
	else
		return Lerp(this->value, this->min, this->max);
}

/* ───────────────────────────────────────────────────────────────────────── */

static f32 sRowY;

void Element_Slider_SetValue(ElSlider* this, f64 val) {
	this->value = val;
	this->value -= this->min;
	this->value *= 1.0 / (this->max - this->min);
	this->vValue = this->value = Clamp(this->value, 0.0f, 1.0f);
}

static void Element_SetRectImpl(Rect* rect, f32 x, f32 y, f32 w) {
	rect->x = x;
	rect->y = y;
	rect->w = w;
	rect->h = SPLIT_TEXT_H;
}

void Element_RowY(f32 y) {
	sRowY = ClampMin(y, SPLIT_ELEM_X_PADDING * 2);
}

void Element_Row(Split* split, s32 rectNum, ...) {
	f32 x = SPLIT_ELEM_X_PADDING;
	f32 width;
	va_list va;
	
	va_start(va, rectNum);
	
	Log("Setting [%d] Elements", rectNum);
	
	for (s32 i = 0; i < rectNum; i++) {
		Rect* rect = va_arg(va, Rect*);
		f64 a = va_arg(va, f64);
		
		width = (f32)(split->rect.w - SPLIT_ELEM_X_PADDING * 3) * a;
		if (rect)
			Element_SetRectImpl(rect, x + SPLIT_ELEM_X_PADDING, sRowY, width - SPLIT_ELEM_X_PADDING);
		x += width;
	}
	
	sRowY += SPLIT_ELEM_Y_PADDING;
	
	va_end(va);
}

/* ───────────────────────────────────────────────────────────────────────── */

void Element_Init(GeoGrid* geo) {
	sCurrentElement = pElementStack;
}

static InputType* Textbox_GetKey(GeoGrid* geo, KeyMap key) {
	return &geo->input->key[key];
}

static InputType* Textbox_GetMouse(GeoGrid* geo, MouseMap key) {
	return &geo->input->mouse.clickArray[key];
}

void Element_Update(GeoGrid* geo) {
	static s32 timer = 0;
	static s32 blocker;
	
	sCurrentElement = pElementStack;
	sElemNum = 0;
	
	sBreatheYaw += DegToBin(3);
	sBreathe = (SinS(sBreatheYaw) + 1.0f) * 0.5;
	
	if (sCurTextbox) {
		char* txt = geo->input->buffer;
		s32 prevTextPos = sTextPos;
		s32 press = 0;
		
		if (blocker == 0) {
			blocker++;
			geo->input->state.keyBlock++;
		}
		
		if (Textbox_GetMouse(geo, MOUSE_ANY)->press || Textbox_GetKey(geo, KEY_ENTER)->press) {
			sSelectPos = -1;
			sCtrlA = 0;
			
			Assert(sCurSplitTextbox != NULL);
			if (!GeoGrid_Cursor_InRect(sCurSplitTextbox, &sCurTextbox->element.rect) || Textbox_GetKey(geo, KEY_ENTER)->press) {
				sCurTextbox = NULL;
				sCurTextbox = NULL;
				sStoreA = NULL;
				sFlickFlag = 1;
				sFlickTimer = 0;
				
				return;
			}
		}
		
		if (Textbox_GetKey(geo, KEY_LEFT_CONTROL)->hold) {
			if (Textbox_GetKey(geo, KEY_A)->press) {
				prevTextPos = strlen(sCurTextbox->txt);
				sTextPos = 0;
				sCtrlA = 1;
			}
			
			if (Textbox_GetKey(geo, KEY_V)->press) {
				txt = (char*)Input_GetClipboardStr(geo->input);
			}
			
			if (Textbox_GetKey(geo, KEY_C)->press) {
				s32 max = fmax(sSelectPos, sTextPos);
				s32 min = fmin(sSelectPos, sTextPos);
				char* copy = xAlloc(512);
				
				memcpy(copy, &sCurTextbox->txt[min], max - min);
				Input_SetClipboardStr(geo->input, copy);
			}
			
			if (Textbox_GetKey(geo, KEY_X)->press) {
				s32 max = fmax(sSelectPos, sTextPos);
				s32 min = fmin(sSelectPos, sTextPos);
				char* copy = xAlloc(512);
				
				memcpy(copy, &sCurTextbox->txt[min], max - min);
				Input_SetClipboardStr(geo->input, copy);
			}
			
			if (Textbox_GetKey(geo, KEY_LEFT)->press) {
				sFlickFlag = 1;
				sFlickTimer = 0;
				while (sTextPos > 0 && isalnum(sCurTextbox->txt[sTextPos - 1]))
					sTextPos--;
				if (sTextPos == prevTextPos)
					sTextPos--;
			}
			if (Textbox_GetKey(geo, KEY_RIGHT)->press) {
				sFlickFlag = 1;
				sFlickTimer = 0;
				while (isalnum(sCurTextbox->txt[sTextPos]))
					sTextPos++;
				if (sTextPos == prevTextPos)
					sTextPos++;
			}
		} else {
			if (Textbox_GetKey(geo, KEY_LEFT)->press) {
				if (sCtrlA == 0) {
					sTextPos--;
					press++;
					sFlickFlag = 1;
					sFlickTimer = 0;
				} else {
					sTextPos = fmin(sTextPos, sSelectPos);
					sCtrlA = 0;
					sSelectPos = -1;
				}
			}
			
			if (Textbox_GetKey(geo, KEY_RIGHT)->press) {
				if (sCtrlA == 0) {
					sTextPos++;
					press++;
					sFlickFlag = 1;
					sFlickTimer = 0;
				} else {
					sTextPos = fmax(sTextPos, sSelectPos);
					sCtrlA = 0;
					sSelectPos = -1;
				}
			}
			
			if (Textbox_GetKey(geo, KEY_HOME)->press) {
				sTextPos = 0;
			}
			
			if (Textbox_GetKey(geo, KEY_END)->press) {
				sTextPos = strlen(sCurTextbox->txt);
			}
			
			if (Textbox_GetKey(geo, KEY_LEFT)->hold || Textbox_GetKey(geo, KEY_RIGHT)->hold) {
				timer++;
			} else {
				timer = 0;
			}
			
			if (timer >= 30 && timer % 2 == 0) {
				if (Textbox_GetKey(geo, KEY_LEFT)->hold) {
					sTextPos--;
				}
				
				if (Textbox_GetKey(geo, KEY_RIGHT)->hold) {
					sTextPos++;
				}
				sFlickFlag = 1;
				sFlickTimer = 0;
			}
		}
		
		sTextPos = Clamp(sTextPos, 0, strlen(sCurTextbox->txt));
		
		if (sTextPos != prevTextPos) {
			if (Textbox_GetKey(geo, KEY_LEFT_SHIFT)->hold || Input_GetShortcut(geo->input, KEY_LEFT_CONTROL, KEY_A)) {
				if (sSelectPos == -1)
					sSelectPos = prevTextPos;
			} else
				sSelectPos = -1;
		} else if (press) {
			sSelectPos = -1;
		}
		
		if (Textbox_GetKey(geo, KEY_BACKSPACE)->press || Input_GetShortcut(geo->input, KEY_LEFT_CONTROL, KEY_X)) {
			if (sSelectPos != -1) {
				s32 max = fmax(sTextPos, sSelectPos);
				s32 min = fmin(sTextPos, sSelectPos);
				
				StrRem(&sCurTextbox->txt[min], max - min);
				sTextPos = min;
				sSelectPos = -1;
			} else if (sTextPos != 0) {
				StrRem(&sCurTextbox->txt[sTextPos - 1], 1);
				sTextPos--;
			}
		}
		
		if (txt[0] != '\0') {
			if (sSelectPos != -1) {
				s32 max = fmax(sTextPos, sSelectPos);
				s32 min = fmin(sTextPos, sSelectPos);
				
				StrRem(&sCurTextbox->txt[min], max - min);
				sTextPos = min;
				sSelectPos = -1;
			}
			
			if (strlen(sCurTextbox->txt) == 0)
				snprintf(sCurTextbox->txt, sCurTextbox->size, "%s", txt);
			else {
				StrIns2(sCurTextbox->txt, txt, sTextPos, sCurTextbox->size);
			}
			
			sTextPos += strlen(txt);
		}
		
		sTextPos = Clamp(sTextPos, 0, strlen(sCurTextbox->txt));
	} else {
		if (blocker) {
			blocker--;
			geo->input->state.keyBlock--;
		}
	}
}

static void Element_UpdateElement(ElementCallInfo* info) {
	GeoGrid* geo = info->geo;
	Element* this = info->arg;
	Split* split = info->split;
	f32 toggle = this->toggle == 2 ? 0.50f : 0.0f;
	
	this->hover = false;
	this->press = false;
	
	if (GeoGrid_Cursor_InRect(split, &this->rect)) {
		this->hover = true;
		
		if (Input_GetMouse(geo->input, MOUSE_L)->hold)
			this->press = true;
	}
	
	if (this->disabled) {
		Theme_SmoothStepToCol(&this->prim,   Theme_GetColor(THEME_PRIM,          255, 0.75f), 0.25f, 0.35f, 0.001f);
		Theme_SmoothStepToCol(&this->shadow, Theme_GetColor(THEME_ELEMENT_DARK,  255, 1.50f + toggle), 0.25f, 0.35f, 0.001f);
		Theme_SmoothStepToCol(&this->light,  Theme_GetColor(THEME_ELEMENT_LIGHT, 255, 0.25f + toggle), 0.25f, 0.35f, 0.001f);
		Theme_SmoothStepToCol(&this->texcol, Theme_GetColor(THEME_TEXT,          255, 0.75f), 0.25f, 0.35f, 0.001f);
		
		if (this->toggle < 2)
			Theme_SmoothStepToCol(&this->base,   Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.25f + toggle), 0.25f, 0.35f, 0.001f);
	} else if (this->hover) {
		Theme_SmoothStepToCol(&this->prim,   Theme_GetColor(THEME_PRIM,          255, 1.10f), 0.25f, 0.35f, 0.001f);
		Theme_SmoothStepToCol(&this->shadow, Theme_GetColor(THEME_ELEMENT_DARK,  255, 1.07f + toggle), 0.25f, 0.35f, 0.001f);
		Theme_SmoothStepToCol(&this->light,  Theme_GetColor(THEME_ELEMENT_LIGHT, 255, 1.07f + toggle), 0.25f, 0.35f, 0.001f);
		Theme_SmoothStepToCol(&this->texcol, Theme_GetColor(THEME_TEXT,          255, 1.15f), 0.25f, 0.35f, 0.001f);
		
		if (this->toggle < 2)
			Theme_SmoothStepToCol(&this->base,   Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.07f + toggle), 0.25f, 0.35f, 0.001f);
	} else {
		Theme_SmoothStepToCol(&this->prim,   Theme_GetColor(THEME_PRIM,          255, 1.00f), 0.25f, 0.35f, 0.001f);
		Theme_SmoothStepToCol(&this->shadow, Theme_GetColor(THEME_ELEMENT_DARK,  255, 1.00f + toggle), 0.25f, 0.35f, 0.001f);
		Theme_SmoothStepToCol(&this->light,  Theme_GetColor(THEME_ELEMENT_LIGHT, 255, 0.50f + toggle), 0.25f, 0.35f, 0.001f);
		Theme_SmoothStepToCol(&this->texcol, Theme_GetColor(THEME_TEXT,          255, 1.00f), 0.25f, 0.35f, 0.001f);
		
		if (this->toggle < 2)
			Theme_SmoothStepToCol(&this->base,   Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f + toggle), 0.25f, 0.35f, 0.001f);
	}
	
	if (this->toggle == 2)
		Theme_SmoothStepToCol(&this->base,   Theme_GetColor(THEME_PRIM,  255, 0.95f), 0.25f, 8.85f, 0.001f);
}

void Element_Draw(GeoGrid* geo, Split* split) {
	for (s32 i = 0; i < sElemNum; i++) {
		if (pElementStack[i].split == split) {
			Element_UpdateElement(&pElementStack[i]);
			pElementStack[i].func(&pElementStack[i]);
		}
	}
}