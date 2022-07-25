#include "GeoGrid.h"
#include "Interface.h"

#undef Element_Row
#undef Element_Disable
#undef Element_Enable
#undef Element_Name
#undef Element_DisplayName

struct ElementCallInfo;

typedef void (* ElementFunc)(struct ElementCallInfo*);

typedef struct ElementCallInfo {
	struct ElementCallInfo* prev;
	struct ElementCallInfo* next;
	
	void*       arg;
	Split*      split;
	ElementFunc func;
	GeoGrid*    geo;
	
	const char* elemFunc;
	s32 update;
} ElementCallInfo;

/* ───────────────────────────────────────────────────────────────────────── */

static ElementCallInfo* sElemHead;

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

static void Element_QueueElement(GeoGrid* geo, Split* split, ElementFunc func, void* arg, const char* elemFunc) {
	ElementCallInfo* node;
	
	Calloc(node, sizeof(ElementCallInfo));
	Node_Add(sElemHead, node);
	node->geo = geo;
	node->split = split;
	node->func = func;
	node->arg = arg;
	node->update = true;
	
	node->elemFunc = elemFunc;
}

#define Element_QueueElement(geo, split, func, arg) Element_QueueElement(geo, split, func, arg, __FUNCTION__)

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
	nvgRoundedRect(
		vg,
		rect->x,
		rect->y,
		rect->w,
		rect->h,
		SPLIT_ROUND_R
	);
	
	nvgPathWinding(vg, NVG_HOLE);
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
	       Split_CursorInRect(split, rect));
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

static bool Element_DisableDraw(Element* element, Split* split) {
	if (element->rect.w <= 0)
		return true;
	if (element->rect.x >= split->rect.w || element->rect.y >= split->rect.h)
		return true;
	
	return false;
}
#define Element_DisableDraw(elem, split) Element_DisableDraw(&elem->element, split)

static f32 Element_TextWidth(void* vg, const char* txt) {
	f32 bounds[4];
	
	nvgTextBounds(vg, 0, 0, txt, 0, bounds);
	
	return bounds[2];
}

/* ───────────────────────────────────────────────────────────────────────── */

#undef PropEnum_AssignList

static char* PropEnum_Get(PropEnum* this, s32 i) {
	char** list = this->list;
	
	return list[i];
}

static void PropEnum_Set(PropEnum* this, s32 i) {
	char** list = this->list;
	
	this->key = Clamp(i, 0, this->num - 1);
	printf_info("" PRNT_YELW "%s", __FUNCTION__);
	printf_info("this->key = %d", this->key);
	printf_info("\"%s\"", list[this->key]);
	
	if (i != this->key)
		Log("Out of range set!");
}

PropEnum* PropEnum_Init(s32 def, s32 num) {
	PropEnum* this = SysCalloc(sizeof(PropEnum));
	
	memset(this, 0, sizeof(*this));
	this->get = PropEnum_Get;
	this->set = PropEnum_Set;
	this->key = def;
	this->num = num;
	
	return this;
}

PropEnum* PropEnum_AssignList(s32 def, s32 num, ...) {
	PropEnum* prop = PropEnum_Init(def, num);
	va_list va;
	
	va_start(va, num);
	
	prop->list = SysCalloc(sizeof(char*) * num);
	for (s32 i = 0; i < num; i++)
		prop->list[i] = StrDup(va_arg(va, char*));
	
	va_end(va);
	
	printf_info("" PRNT_YELW "%s", __FUNCTION__);
	printf_info("prop->key = %d", prop->key);
	printf_info("prop->num = %d", prop->num);
	
	return prop;
}

void PropEnum_Add(PropEnum* this, char* item) {
	char** list = NULL;
	
	list = SysCalloc(sizeof(char*) * (this->num + 1));
	if (this->num)
		memcpy(list, this->list, sizeof(char*) * this->num);
	
	list[this->num++] = StrDup(item);
	Free(this->list);
	this->list = list;
	
	printf_info("" PRNT_YELW "%s", __FUNCTION__);
	printf_info("\"%s\"", this->list[this->num - 1]);
	printf_info("prop->num = %d", this->num);
}

void PropEnum_Remove(PropEnum* this, s32 key) {
	char** list = NULL;
	s32 w = 0;
	
	Assert(this->list);
	
	if (this->num - 1 > 0)
		list = SysCalloc(sizeof(char*) * (this->num - 1));
	
	printf_info("" PRNT_YELW "%s", __FUNCTION__);
	
	for (s32 i = 0; i < this->num; i++) {
		if (i == key) {
			printf_info("\"%s\"", this->list[i]);
			Free(this->list[i]);
			continue;
		}
		
		list[w++] = this->list[i];
	}
	
	Free(this->list);
	this->list = list;
	this->num--;
	printf_info("prop->num = %d", this->num);
}

void PropEnum_Free(PropEnum* this) {
	for (s32 i = 0; i < this->num; i++)
		Free(this->list[i]);
	Free(this->list);
	Free(this);
}

/* ───────────────────────────────────────────────────────────────────────── */

void DropMenu_Init(GeoGrid* geo, bool useOriginRect, bool highlighCurrentKey) {
	DropMenu* this = &geo->dropMenu;
	PropEnum* prop = this->prop;
	s32 height;
	
	Assert(prop != NULL);
	
	geo->state.noClickInput++;
	geo->state.noSplit++;
	this->pos = geo->input->mouse.pos;
	this->state.useOriginRect = useOriginRect;
	this->key = -1;
	if (highlighCurrentKey)
		this->key = this->prop->key;
	
	height = SPLIT_ELEM_X_PADDING * 2 + SPLIT_TEXT_H * prop->num;
	
	if (useOriginRect) {
		this->rect.y = this->rectOrigin.y + this->rectOrigin.h;
		this->rect.h = height;
		
		this->rect.x = this->rectOrigin.x;
		this->rect.w = this->rectOrigin.w;
	} else {
		this->rect.w = 0;
		nvgFontFace(geo->vg, "dejavu");
		nvgFontSize(geo->vg, SPLIT_TEXT);
		nvgTextAlign(geo->vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
		for (s32 i = 0; i < prop->num; i++)
			this->rect.w = Max(this->rect.w, Element_TextWidth(geo->vg, prop->get(prop, i)));
		this->rect.w += SPLIT_ELEM_X_PADDING * 2;
	}
	
	printf_info("" PRNT_YELW "%s", __FUNCTION__);
	printf_info("prop->key = %d", prop->key);
	printf_info("prop->num = %d", prop->num);
}

void DropMenu_Close(GeoGrid* geo) {
	DropMenu* this = &geo->dropMenu;
	
	geo->state.noClickInput--;
	geo->state.noSplit--;
	this->prop = 0;
	this->state.useOriginRect = 0;
	this->state.init = 0;
	this->element = 0;
}

void DropMenu_Draw(GeoGrid* geo) {
	DropMenu* this = &geo->dropMenu;
	PropEnum* prop = this->prop;
	void* vg = geo->vg;
	f32 height = SPLIT_ELEM_X_PADDING;
	MouseInput* mouse = &geo->input->mouse;
	s32 in = false;
	
	if (prop == NULL)
		return;
	
	glViewport(
		0,
		0,
		geo->winDim->x,
		geo->winDim->y
	);
	nvgBeginFrame(geo->vg, geo->winDim->x, geo->winDim->y, gPixelRatio); {
		Rect r = this->rect;
		Element_Draw_RoundedOutline(vg, &this->rect, Theme_GetColor(THEME_ELEMENT_LIGHT, 215, 1.0f));
		
		r.y -= 1;
		r.h += 1;
		Element_Draw_RoundedRect(vg, &r, Theme_GetColor(THEME_ELEMENT_DARK, 215, 1.0f));
		
		for (s32 i = 0; i < prop->num; i++) {
			r = this->rect;
			
			r.y = this->rect.y + height;
			r.h = SPLIT_TEXT_H;
			
			if (Rect_PointIntersect(&r, mouse->pos.x, mouse->pos.y)) {
				this->key = i;
				in = true;
			}
			
			if (i == this->key) {
				r.x += 4;
				r.w -= 8;
				Element_Draw_RoundedRect(vg, &r, Theme_GetColor(THEME_PRIM, 215, 1.0f));
			}
			
			nvgFillColor(vg, Theme_GetColor(THEME_TEXT, 255, 1.0f));
			nvgFontFace(vg, "dejavu");
			nvgFontSize(vg, SPLIT_TEXT);
			nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
			nvgText(vg, this->rect.x + SPLIT_ELEM_X_PADDING, this->rect.y + height + SPLIT_ELEM_X_PADDING * 0.5f, prop->get(prop, i), NULL);
			
			height += SPLIT_TEXT_H;
		}
	} nvgEndFrame(geo->vg);
	
	if (this->state.init == false) {
		this->state.init = true;
		
		return;
	}
	
	if (Input_GetMouse(geo->input, MOUSE_L)->press) {
		if (this->key > -1 && in)
			prop->set(prop, this->key);
		
		DropMenu_Close(geo);
	}
}

/* ───────────────────────────────────────────────────────────────────────── */

static void Element_Draw_Button(ElementCallInfo* info) {
	void* vg = info->geo->vg;
	ElButton* this = info->arg;
	
	if (Element_DisableDraw(this, info->split))
		return;
	
	Element_Draw_RoundedOutline(vg, &this->element.rect, this->element.light);
	
	nvgBeginPath(vg);
	nvgFillColor(vg, this->element.base);
	nvgRoundedRect(vg, this->element.rect.x, this->element.rect.y, this->element.rect.w, this->element.rect.h, SPLIT_ROUND_R);
	nvgFill(vg);
	
	if (this->element.name && this->element.rect.w > 8) {
		char* txt = (char*)this->element.name;
		char ftxt[512];
		f32 bounds[4];
		
		nvgFontFace(vg, "dejavu");
		nvgFontSize(vg, SPLIT_TEXT);
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
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
		nvgFontBlur(vg, 0.0);
		nvgFillColor(vg, this->element.texcol);
		nvgText(
			vg,
			this->element.rect.x + this->element.rect.w * 0.5,
			this->element.rect.y + this->element.rect.h * 0.5 + 1,
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
	
	if (Element_DisableDraw(this, info->split))
		return;
	
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
	
	if (Split_CursorInRect(split, &this->element.rect) && inputCtx->mouse.clickL.hold) {
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
		this->element.rect.y + this->element.rect.h * 0.5 + 1,
		txtA,
		txtB
	);
}

static void Element_Draw_Text(ElementCallInfo* info) {
	void* vg = info->geo->vg;
	// Split* split = info->split;
	ElText* this = info->arg;
	f32 width;
	f32 max = this->element.rect.w;
	char tempText[512];
	
	if (Element_DisableDraw(this, info->split))
		return;
	
	if (this->element.dispText)
		max *= 0.333333f;
	
	strcpy(tempText, this->element.name);
	
	nvgFontFace(vg, "dejavu");
	nvgFontSize(vg, SPLIT_TEXT);
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	
	width = Element_TextWidth(vg, this->element.name);
	while (width > max) {
		Log("%.2f > %.2f", width, max);
		strcpy(tempText + ClampMin(strlen(tempText) - 3, 0), "..");
		
		width = Element_TextWidth(vg, tempText);
	}
	
	nvgFontBlur(vg, 0.0);
	nvgFillColor(vg, this->element.texcol);
	
	if (this->element.dispText == true) {
		nvgText(
			vg,
			this->element.posTxt.x,
			this->element.posTxt.y,
			tempText,
			NULL
		);
	} else {
		nvgText(
			vg,
			this->element.rect.x,
			this->element.rect.y + this->element.rect.h * 0.5 + 1,
			tempText,
			NULL
		);
	}
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
	
	if (Element_DisableDraw(this, info->split))
		return;
	
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
	f32 mul = this->vValue;
	
	if (Element_DisableDraw(this, info->split))
		return;
	
	Math_DelSmoothStepToF(&this->vValue, this->value, 0.5f, (this->max - this->min) * 0.5f, 0.0f);
	
	if (this->isInt) {
		mul = rint(Lerp(this->value, this->min, this->max));
		
		mul -= this->min;
		mul /= this->max;
	}
	
	rect.x = this->element.rect.x;
	rect.y = this->element.rect.y;
	rect.w = this->element.rect.w;
	rect.h = this->element.rect.h;
	rect.w = ClampMin(rect.w * mul, 0);
	
	rect.x = rect.x + 2;
	rect.y = rect.y + 2;
	rect.w = ClampMin(rect.w - 4, 0);
	rect.h = rect.h - 4;
	
	Element_Draw_RoundedOutline(vg, &this->element.rect, this->element.light);
	Element_Draw_RoundedRect(vg, &this->element.rect, this->element.shadow);
	
	if (rect.w != 0) {
		nvgBeginPath(vg);
		nvgFillColor(vg, this->element.prim);
		nvgRoundedRect(
			vg,
			rect.x,
			rect.y,
			rect.w,
			rect.h,
			SPLIT_ROUND_R
		);
		nvgFill(vg);
	}
	
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
		this->element.rect.y + this->element.rect.h * 0.5 + 1,
		this->textBox.txt,
		NULL
	);
}

static void Element_Draw_Combo(ElementCallInfo* info) {
	ElCombo* this = info->arg;
	GeoGrid* geo = info->geo;
	void* vg = geo->vg;
	PropEnum* prop = this->prop;
	Rect r = this->element.rect;
	Vec2f center;
	Vec2f arrow[] = {
		{ -5.0f, -2.5f },
		{ 0.0f,   2.5f },
		{ 5.0f,  -2.5f },
		{ -5.0f, -2.5f },
	};
	
	nvgFontFace(vg, "dejavu");
	
	nvgFontSize(vg, SPLIT_TEXT);
	nvgFontBlur(vg, 0.0);
	
	Element_Draw_RoundedOutline(vg, &r, this->element.light);
	
	if (&this->element == geo->dropMenu.element)
		r.h += 2;
	
	Element_Draw_RoundedRect(vg, &r, this->element.shadow);
	
	if (&this->element == geo->dropMenu.element)
		r.h -= 2;
	
	if (prop) {
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgFillColor(vg, this->element.texcol);
		nvgText(
			vg,
			r.x + SPLIT_ELEM_X_PADDING,
			r.y + r.h * 0.5 + 1,
			prop->get(prop, prop->key),
			NULL
		);
	}
	
	center.x = r.x + r.w - 5 - 5;
	center.y = r.y + r.h * 0.5f;
	
	nvgBeginPath(vg);
	nvgFillColor(vg, this->element.light);
	for (s32 i = 0; i < ArrayCount(arrow); i++) {
		s32 wi = WrapS(i, 0, ArrayCount(arrow) - 1);
		Vec2f zero = { 0 };
		Vec2f pos = {
			arrow[wi].x,
			arrow[wi].y,
		};
		f32 dist = Math_Vec2f_DistXZ(&zero, &pos);
		s16 yaw = Math_Vec2f_Yaw(&zero, &pos);
		
		pos.x = center.x + SinS(yaw) * dist;
		pos.y = center.y + CosS(yaw) * dist;
		
		if ( i == 0 )
			nvgMoveTo(vg, pos.x, pos.y);
		else
			nvgLineTo(vg, pos.x, pos.y);
	}
	nvgFill(vg);
}

static void Element_Draw_Line(ElementCallInfo* info) {
	Element* this = info->arg;
	
	Element_Draw_RoundedOutline(info->geo->vg, &this->rect, Theme_GetColor(THEME_SHADOW, 255, 1.0f));
	Element_Draw_RoundedRect(info->geo->vg, &this->rect, Theme_GetColor(THEME_HIGHLIGHT, 175, 1.0f));
	
	Free(this);
}

static void Element_Draw_Box(ElementCallInfo* info) {
	Element* this = info->arg;
	
	Element_Draw_RoundedOutline(info->geo->vg, &this->rect, Theme_GetColor(THEME_HIGHLIGHT, 45, 1.0f));
	Element_Draw_RoundedRect(info->geo->vg, &this->rect, Theme_GetColor(THEME_SHADOW, 45, 1.0f));
	
	Free(this);
}

/* ───────────────────────────────────────────────────────────────────────── */

static GeoGrid* sGeo;
static Split* sSplit;
static f32 sRowY;
static f32 sRowX;

void Element_SetContext(GeoGrid* setGeo, Split* setSplit) {
	sSplit = setSplit;
	sGeo = setGeo;
}

// Returns button state, 0bXY, X == toggle, Y == pressed
s32 Element_Button(ElButton* this) {
	void* vg = sGeo->vg;
	
	Assert(sGeo && sSplit);
	
	this->state = 0;
	
	if (this->autoWidth) {
		f32 bounds[4] = { 0.0f };
		
		nvgFontFace(vg, "dejavu");
		nvgFontSize(vg, SPLIT_TEXT);
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgTextBounds(vg, 0, 0, this->element.name, NULL, bounds);
		
		this->element.rect.w = bounds[2] + SPLIT_TEXT_PADDING * 2;
	}
	
	if (this->element.disabled || sGeo->state.noClickInput)
		goto queue_element;
	
	if (Element_PressCondition(sGeo, sSplit, &this->element.rect)) {
		if (Input_GetMouse(sGeo->input, MOUSE_L)->press) {
			this->state++;
			
			if (this->element.toggle) {
				u8 t = (this->element.toggle - 1) == 0; // Invert
				
				this->element.toggle = t + 1;
			}
		}
		
		if (Input_GetMouse(sGeo->input, MOUSE_L)->hold) {
			this->state++;
		}
	}
	
queue_element:
	Element_QueueElement(
		sGeo,
		sSplit,
		Element_Draw_Button,
		this
	);
	
	return (this->state == 2) | (ClampMin(this->element.toggle - 1, 0)) << 4;
}

void Element_Textbox(ElTextbox* this) {
	Assert(sGeo && sSplit);
	if (this->element.disabled || sGeo->state.noClickInput)
		goto queue_element;
	
	if (Element_PressCondition(sGeo, sSplit, &this->element.rect)) {
		if (Input_GetMouse(sGeo->input, MOUSE_L)->press) {
			if (this != sCurTextbox) {
				sCtrlA = 1;
				this->isHintText = 2;
				sTextPos = 0;
				sSelectPos = strlen(this->txt);
			}
			
			sCurTextbox = this;
			sCurSplitTextbox = sSplit;
		}
	}
	
queue_element:
	Element_QueueElement(
		sGeo,
		sSplit,
		Element_Draw_Textbox,
		this
	);
}

// Returns text width
f32 Element_Text(ElText* this) {
	f32 bounds[4] = { 0 };
	void* vg = sGeo->vg;
	
	Assert(sGeo && sSplit);
	
	nvgFontFace(vg, "dejavu");
	nvgFontSize(vg, SPLIT_TEXT);
	nvgFontBlur(vg, 0.0);
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	nvgFillColor(vg, Theme_GetColor(THEME_TEXT, 255, 1.0f));
	nvgTextBounds(vg, 0, 0, this->element.name, NULL, bounds);
	
	Element_QueueElement(
		sGeo,
		sSplit,
		Element_Draw_Text,
		this
	);
	
	return bounds[2];
}

s32 Element_Checkbox(ElCheckbox* this) {
	Assert(sGeo && sSplit);
	this->element.rect.w = this->element.rect.h;
	
	if (this->element.disabled || sGeo->state.noClickInput)
		goto queue_element;
	
	if (Element_PressCondition(sGeo, sSplit, &this->element.rect)) {
		if (Input_GetMouse(sGeo->input, MOUSE_L)->press) {
			this->element.toggle ^= 1;
		}
	}
	
queue_element:
	Element_QueueElement(
		sGeo,
		sSplit,
		Element_Draw_Checkbox,
		this
	);
	
	return this->element.toggle;
}

f32 Element_Slider(ElSlider* this) {
	Assert(sGeo && sSplit);
	if (this->min == 0.0f && this->max == 0.0f)
		this->max = 1.0f;
	
	if (this->element.disabled || sGeo->state.noClickInput)
		goto queue_element;
	
	if (Element_PressCondition(sGeo, sSplit, &this->element.rect) || this->holdState) {
		u32 pos = false;
		
		if (this->isTextbox) {
			this->isTextbox = false;
			if (sCurTextbox == &this->textBox) {
				this->isTextbox = true;
				this->holdState = true;
				
				Element_Textbox(&this->textBox);
				
				return Lerp(this->value, this->min, this->max);
			} else {
				Element_Slider_SetValue(this, this->isInt ? Value_Int(this->textBox.txt) : Value_Float(this->textBox.txt));
				
				goto queue_element;
			}
		}
		
		if (Input_GetMouse(sGeo->input, MOUSE_L)->press) {
			this->holdState = true;
			sSplit->elemBlockMouse++;
		} else if (Input_GetMouse(sGeo->input, MOUSE_L)->hold && this->holdState) {
			if (sGeo->input->mouse.vel.x) {
				if (this->isSliding == false) {
					Element_Slider_SetCursorToVal(sGeo, sSplit, this);
				} else {
					if (Input_GetKey(sGeo->input, KEY_LEFT_SHIFT)->hold)
						this->value += (f32)sGeo->input->mouse.vel.x * 0.0001f;
					else
						this->value += (f32)sGeo->input->mouse.vel.x * 0.001f;
					if (this->min || this->max)
						this->value = Clamp(this->value, 0.0f, 1.0f);
					
					pos = true;
				}
				
				this->isSliding = true;
			}
		} else if (Input_GetMouse(sGeo->input, MOUSE_L)->release && this->holdState) {
			if (this->isSliding == false && Split_CursorInRect(sSplit, &this->element.rect)) {
				Element_Slider_SetTextbox(sSplit, this);
			}
			this->isSliding = false;
		} else {
			if (this->holdState)
				sSplit->elemBlockMouse--;
			this->holdState = false;
		}
		
		if (sGeo->input->mouse.scrollY) {
			if (Input_GetKey(sGeo->input, KEY_LEFT_SHIFT)->hold) {
				this->value += sGeo->input->mouse.scrollY * 0.1;
			} else if (Input_GetKey(sGeo->input, KEY_LEFT_ALT)->hold) {
				this->value += sGeo->input->mouse.scrollY * 0.001;
			} else {
				this->value += sGeo->input->mouse.scrollY * 0.01;
			}
		}
		
		if (pos) Element_Slider_SetCursorToVal(sGeo, sSplit, this);
	}
	
queue_element:
	this->value = Clamp(this->value, 0.0f, 1.0f);
	
	if (this->isSliding)
		Cursor_SetCursor(CURSOR_EMPTY);
	
	Element_QueueElement(
		sGeo,
		sSplit,
		Element_Draw_Slider,
		this
	);
	
	if (this->isInt)
		return (s32)rint(Lerp(this->value, this->min, this->max));
	else
		return Lerp(this->value, this->min, this->max);
}

s32 Element_Combo(ElCombo* this) {
	Assert(sGeo && sSplit);
	if (this->element.disabled || sGeo->state.noClickInput)
		goto queue_element;
	
	if (this->prop) {
		if (Element_PressCondition(sGeo, sSplit, &this->element.rect)) {
			s32 scrollY = Clamp(sGeo->input->mouse.scrollY, -1, 1);
			
			if (Input_GetMouse(sGeo->input, MOUSE_L)->press) {
				sGeo->dropMenu.prop = this->prop;
				sGeo->dropMenu.element = &this->element;
				sGeo->dropMenu.rectOrigin = Rect_AddPos(&this->element.rect, &sSplit->rect);
				DropMenu_Init(sGeo, true, true);
			}
			
			if (scrollY)
				this->prop->set(this->prop, this->prop->key - scrollY);
		}
	}
	
queue_element:
	Element_QueueElement(
		sGeo,
		sSplit,
		Element_Draw_Combo,
		this
	);
	
	if (this->prop)
		return this->prop->key;
	else
		return 0;
}

void Element_Separator(bool drawLine) {
	Element* this;
	
	Assert(sGeo && sSplit);
	
	if (drawLine) {
		CallocX(this);
		
		sRowY += SPLIT_ELEM_X_PADDING;
		
		this->rect.x = SPLIT_ELEM_X_PADDING * 2;
		this->rect.w = sSplit->rect.w - SPLIT_ELEM_X_PADDING * 4 - 1;
		this->rect.y = sRowY - SPLIT_ELEM_X_PADDING * 0.5;
		this->rect.h = 2;
		
		sRowY += SPLIT_ELEM_X_PADDING;
		
		Element_QueueElement(sGeo, sSplit, Element_Draw_Line, this);
	} else {
		sRowY += SPLIT_ELEM_X_PADDING * 2;
	}
}

void Element_Box(BoxInit io) {
	#define BOX_PUSH() r++
	#define BOX_POP()  r--
	#define THIS  this[r]
	#define ROW_Y rowY[r]
	
	static Element * this[16];
	static f32 rowY[16];
	static s32 r;
	
	Assert(sGeo && sSplit);
	
	if (io == BOX_START) {
		ROW_Y = sRowY;
		
		CallocX(THIS);
		Element_Row(sSplit, 1, THIS, 1.0);
		sRowY = ROW_Y + SPLIT_ELEM_X_PADDING;
		
		// Shrink Split Rect
		sRowX += SPLIT_ELEM_X_PADDING;
		sSplit->rect.w -= SPLIT_ELEM_X_PADDING * 2;
		
		Element_QueueElement(sGeo, sSplit, Element_Draw_Box, THIS);
		
		BOX_PUSH();
	}
	
	if (io == BOX_END) {
		BOX_POP();
		
		THIS->rect.h = sRowY - ROW_Y;
		sRowY += SPLIT_ELEM_X_PADDING;
		
		// Expand Split Rect
		sSplit->rect.w += SPLIT_ELEM_X_PADDING * 2;
		sRowX -= SPLIT_ELEM_X_PADDING;
	}
#undef BoxPush
#undef BoxPop
}

void Element_DisplayName(Element* this) {
	ElementCallInfo* node;
	s32 w = this->rect.w * 0.25f;
	
	Assert(sGeo && sSplit);
	
	if (w < 32)
		return;
	
	this->posTxt.x = this->rect.x;
	this->posTxt.y = this->rect.y + this->rect.h * 0.5 + 1;
	
	this->rect.x += w;
	this->rect.w -= w;
	
	this->dispText = true;
	
	Calloc(node, sizeof(ElementCallInfo));
	Node_Add(sElemHead, node);
	node->geo = sGeo;
	node->split = sSplit;
	node->func = Element_Draw_Text;
	node->arg = this;
	node->elemFunc = "Element_DisplayName";
	node->update = false;
}

/* ───────────────────────────────────────────────────────────────────────── */

void Element_Slider_SetParams(ElSlider* this, f32 min, f32 max, char* type) {
	this->min = min;
	this->max = max;
	if (!stricmp(type, "int"))
		this->isInt = true;
	if (!stricmp(type, "float"))
		this->isInt = false;
}

void Element_Slider_SetValue(ElSlider* this, f64 val) {
	this->value = val;
	this->value -= this->min;
	this->value *= 1.0 / (this->max - this->min);
	this->vValue = this->value = Clamp(this->value, 0.0f, 1.0f);
}

void Element_Button_SetValue(ElButton* this, bool toggle, bool state) {
	this->element.toggle = toggle ? toggle + state : 0;
	this->state = state;
}

void Element_Combo_SetPropEnum(ElCombo* this, PropEnum* prop) {
	this->prop = prop;
}

void Element_Name(Element* this, const char* name) {
	this->name = name;
}

void Element_Disable(Element* element) {
	element->disabled = true;
}

void Element_Enable(Element* element) {
	element->disabled = false;
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
	f32 x = SPLIT_ELEM_X_PADDING + sRowX;
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

static InputType* Textbox_GetKey(GeoGrid* geo, KeyMap key) {
	return &geo->input->key[key];
}

static InputType* Textbox_GetMouse(GeoGrid* geo, MouseMap key) {
	return &geo->input->mouse.clickArray[key];
}

void Element_Update(GeoGrid* geo) {
	static s32 timer = 0;
	static s32 blocker;
	
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
			if (!Split_CursorInRect(sCurSplitTextbox, &sCurTextbox->element.rect) || Textbox_GetKey(geo, KEY_ENTER)->press) {
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
	f32 press = 1.0f;
	
	this->hover = false;
	this->press = false;
	
	if (Element_PressCondition(geo, split, &this->rect)) {
		this->hover = true;
		
		if (Input_GetMouse(geo->input, MOUSE_L)->hold)
			this->press = true;
	}
	
	if (this == (void*)sCurTextbox || this == geo->dropMenu.element)
		this->hover = true;
	
	if (this->press && this->disabled == false)
		press = 1.2f;
	
	if (this->hover) {
		Theme_SmoothStepToCol(&this->prim,   Theme_GetColor(THEME_PRIM,          255, 1.10f * press),            0.25f, 0.35f, 0.001f);
		Theme_SmoothStepToCol(&this->shadow, Theme_GetColor(THEME_ELEMENT_DARK,  255, (1.07f + toggle) * press), 0.25f, 0.35f, 0.001f);
		Theme_SmoothStepToCol(&this->light,  Theme_GetColor(THEME_ELEMENT_LIGHT, 255, (1.07f + toggle) * press), 0.25f, 0.35f, 0.001f);
		Theme_SmoothStepToCol(&this->texcol, Theme_GetColor(THEME_TEXT,          255, 1.15f * press),            0.25f, 0.35f, 0.001f);
		
		if (this->toggle < 2)
			Theme_SmoothStepToCol(&this->base,   Theme_GetColor(THEME_ELEMENT_BASE,  255, (1.07f + toggle) * press), 0.25f, 0.35f, 0.001f);
	} else {
		Theme_SmoothStepToCol(&this->prim,   Theme_GetColor(THEME_PRIM,          255, 1.00f * press),            0.25f, 0.35f, 0.001f);
		Theme_SmoothStepToCol(&this->shadow, Theme_GetColor(THEME_ELEMENT_DARK,  255, (1.00f + toggle) * press), 0.25f, 0.35f, 0.001f);
		Theme_SmoothStepToCol(&this->light,  Theme_GetColor(THEME_ELEMENT_LIGHT, 255, (0.50f + toggle) * press), 0.25f, 0.35f, 0.001f);
		Theme_SmoothStepToCol(&this->texcol, Theme_GetColor(THEME_TEXT,          255, 1.00f * press),            0.25f, 0.35f, 0.001f);
		
		if (this->toggle < 2)
			Theme_SmoothStepToCol(&this->base,   Theme_GetColor(THEME_ELEMENT_BASE,  255, (1.00f + toggle) * press), 0.25f, 0.35f, 0.001f);
	}
	
	if (this->toggle == 2)
		Theme_SmoothStepToCol(&this->base,   Theme_GetColor(THEME_PRIM,  255, 0.95f * press), 0.25f, 8.85f, 0.001f);
	
	if (this->disabled) {
		Theme_SmoothStepToCol(&this->prim,   Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), 0.25f, 0.35f, 0.001f);
		Theme_SmoothStepToCol(&this->shadow, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), 0.25f, 0.35f, 0.001f);
		Theme_SmoothStepToCol(&this->base,   Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), 0.25f, 0.35f, 0.001f);
		Theme_SmoothStepToCol(&this->light,  Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), 1.00f, 1.00f, 0.001f);
		Theme_SmoothStepToCol(&this->texcol, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), 0.25f, 0.35f, 0.001f);
	}
}

void Element_Draw(GeoGrid* geo, Split* split) {
	ElementCallInfo* elem = sElemHead;
	
	sSplit = NULL;
	sGeo = NULL;
	
	while (elem) {
		ElementCallInfo* next = elem->next;
		
		if (elem->split == split) {
			Log("ElemFunc: " PRNT_PRPL "%s", elem->elemFunc);
			
			if (elem->update)
				Element_UpdateElement(elem);
			elem->func(elem);
			Node_Kill(sElemHead, elem);
		}
		
		elem = next;
	}
}
