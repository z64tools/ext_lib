#include "ext_interface.h"
#include "nano_grid.h"

#undef Element_Box

#undef Element_SetName
#undef Element_SetIcon
#undef Element_Disable
#undef Element_Enable
#undef Element_Condition
#undef Element_SetNameLerp
#undef Element_Operatable
#undef Element_Row
#undef Element_Header

struct ElementQueCall;

typedef void (*ElementFunc)(struct ElementQueCall*);

static void Textbox_Set(ElTextbox*, Split*);

typedef struct ElementQueCall {
	struct ElementQueCall* next;
	
	void*       arg;
	Split*      split;
	ElementFunc func;
	NanoGrid*   nano;
	
	const char* elemFunc;
	u32 update : 1;
} ElementQueCall;

////////////////////////////////////////////////////////////////////////////////

typedef struct {
	const char* item;
	f32      swing;
	f32      alpha;
	f32      colorLerp;
	Vec2s    anchor;
	Vec2f    pos;
	Vec2f    vel;
	Rect     rect;
	s32      hold;
	Element* src;
	Arli*    list;
	bool     copy;
} DragItem;

typedef struct ElBox {
	Element  element;
	ElPanel* panel;
	Rect     headRect;
	f32      rowY;
} ElBox;

typedef struct BoxContext {
	ElBox* list[16];
	int    index;
} BoxContext;

typedef struct {
	NanoGrid*       nano;
	Split*          split;
	ElementQueCall* head;
	char         textStoreBuf[TEXTBOX_BUFFER_SIZE];
	ElTextbox*   textbox;
	BoxContext   boxCtx;
	ElContainer* interactContainer;
	
	s16 breathYaw;
	f32 breath;
	
	f32 rowY;
	f32 rowX;
	f32 shiftX;
	f32 abs;
	
	int timerTextbox;
	int blockerTextbox;
	int blockerTemp;
	
	bool pushToHeader : 1;
	bool forceDisable : 1;
	
	DragItem dragItem;
} ElementState;

thread_local ElementState* sElemState = NULL;

////////////////////////////////////////////////////////////////////////////////

void* ElementState_New(void) {
	ElementState* elemState;
	
	osAssert((elemState = new(ElementState)) != NULL);
	
	return elemState;
}

void ElementState_Set(void* elemState) {
	sElemState = elemState;
}

void* ElementState_Get() {
	return sElemState;
}

static const char* sFmt[] = {
	"%.3f",
	"%d"
};

static void DragItem_Init(NanoGrid* nano, Element* src, Rect rect, const char* item, Arli* list) {
	DragItem* this = &sElemState->dragItem;
	
	this->src = src;
	this->item = strdup(item);
	this->rect = rect;
	this->rect.x = 0;
	this->rect.y = 0;
	
	this->alpha = this->swing = 0;
	this->pos = Vec2f_New(UnfoldVec2(nano->input->cursor.pos));
	this->anchor = nano->input->cursor.pos;
	this->anchor.x -= rect.x + sElemState->split->rect.x;
	this->anchor.y -= rect.y + sElemState->split->rect.y;
	this->hold = true;
	this->colorLerp = 0.0f;
	this->list = list;
}

static bool DragItem_Release(NanoGrid* nano, void* src) {
	DragItem* this = &sElemState->dragItem;
	
	if (this->hold == false || src != this->src) return false;
	
	this->hold = false;
	this->src = NULL;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

void ScrollBar_Init(ScrollBar* this, int max, int height) {
	this->max = max;
	this->slotHeight = height;
}

Rect ScrollBar_GetRect(ScrollBar* this, int slot) {
	Rect r = {
		this->workRect.x,
		this->workRect.y + this->slotHeight * slot,
		this->workRect.w - (SPLIT_SCROLL_WIDTH + SPLIT_SCROLL_PAD) * this->mod,
		this->workRect.h,
	};
	
	return r;
}

bool ScrollBar_Update(ScrollBar* this, Input* input, Vec2s cursorPos, Rect scrollRect, Rect displayRect) {
	InputType* click = NULL;
	f64 barHeight;
	Rect prevRect = this->baseRect;
	
	this->baseRect = this->workRect = scrollRect;
	this->cursorPos = cursorPos;
	this->disabled = !!!input;
	
	if (input) {
		click =  Input_GetCursor(input, CLICK_L);
		
		if (Rect_PointIntersect(&this->baseRect, UnfoldVec2(cursorPos))) {
			this->cur -= Input_GetScroll(input);
		}
	}
	
	this->visNum = this->baseRect.h / this->slotHeight;
	this->visMax = clamp_min(this->max - this->visNum, 0);
	
	barHeight = displayRect.h * clamp(this->visNum / this->max, 0, 1);
	if (barHeight < SPLIT_SCROLL_WIDTH * 2)
		barHeight = SPLIT_SCROLL_WIDTH * 2;
	if (barHeight > displayRect.h)
		barHeight = displayRect.h * 0.75f;
	
	if (this->hold) {
		if (!click || !click->hold)
			this->hold--;
		
		else {
			int heldPosY = this->cursorPos.y - this->holdOffset;
			f64 cur = (heldPosY - displayRect.y) / (displayRect.h - barHeight);
			
			this->cur = rint(cur * this->visMax);
		}
		
	} else if (this->focusSlot > -1) {
		this->cur = clamp(this->focusSlot - this->visNum / 2, 0, this->visMax);
		this->focusSlot = -1;
		
		if (!this->focusSmooth)
			this->vcur = this->cur;
	} else if (this->baseRect.h != prevRect.h) {
		this->vcur = clamp(this->cur, 0, this->visMax);
	}
	
	this->cur = clamp(this->cur, 0, this->visMax);
	this->vcur = clamp(this->vcur, 0, this->visMax);
	Math_DelSmoothStepToD(&this->vcur, this->cur, 0.25, (f32)this->visMax / 4, 0.01);
	
	f64 vslot = this->vcur;
	f64 vmod = normd(this->vcur, 0, this->visMax);
	
	this->barOutlineRect = Rect_New(
		RectW(displayRect) - (SPLIT_SCROLL_WIDTH + SPLIT_SCROLL_PAD),
		displayRect.y,
		SPLIT_SCROLL_WIDTH,
		displayRect.h);
	this->barRect = Rect_New(
		RectW(displayRect) - (SPLIT_SCROLL_WIDTH + SPLIT_SCROLL_PAD),
		ceilf(lerpd(vmod, displayRect.y, RectH(displayRect) - barHeight)),
		SPLIT_SCROLL_WIDTH,
		barHeight);
	
	f32 target = (this->visNum >= this->max) ? 0.0f : 1.0f;
	Math_SmoothStepToF(&this->mod, target, 0.25f, 0.25f, 0.01f);
	
	this->workRect.y -= rint(vslot * this->slotHeight);
	this->workRect.h = this->slotHeight;
	
	if (Rect_PointIntersect(&this->barRect, UnfoldVec2(this->cursorPos))) {
		if (click && click->press) {
			this->hold = 2;
			this->holdOffset = this->cursorPos.y - this->barRect.y;
		}
	}
	
	return this->hold;
}

bool ScrollBar_Draw(ScrollBar* this, void* vg) {
	Vec2s cursorPos = this->cursorPos;
	
	if (this->hold)
		Theme_SmoothStepToCol(&this->color, Theme_GetColor(THEME_PRIM, 220, 1.0f), 0.5f, 1.0f, 0.01f);
	
	else {
		if (Rect_PointIntersect(&this->barRect, UnfoldVec2(cursorPos))) {
			Theme_SmoothStepToCol(&this->color, Theme_GetColor(THEME_HIGHLIGHT, 220, 1.0f), 0.5f, 1.0f, 0.01f);
			
		} else
			Theme_SmoothStepToCol(&this->color, Theme_GetColor(THEME_HIGHLIGHT, 125, 1.0f), 0.5f, 1.0f, 0.01f);
	}
	
	if (this->mod < EPSILON)
		return this->hold;
	
	NVGcolor color = this->color;
	NVGcolor base = Theme_GetColor(THEME_ELEMENT_BASE, 255, 1.0f);
	
	color.a *= this->mod;
	base.a *= this->mod;
	Gfx_DrawRounderRect(vg, Rect_Scale(this->barOutlineRect, -2, -4), base);
	Gfx_DrawRounderRect(vg, Rect_Scale(this->barRect, -2, -4), color);
	color.a /= 4;
	Gfx_DrawRounderOutline(vg, Rect_Scale(this->barOutlineRect, -2, -4), color);
	
	return this->hold;
}

void ScrollBar_FocusSlot(ScrollBar* this, int slot, bool smooth) {
	this->focusSlot = slot;
	this->focusSmooth = smooth;
}

////////////////////////////////////////////////////////////////////////////////

const char* sDefaultFont = "default";

void Gfx_SetDefaultTextParams(void* vg) {
	nvgFontFace(vg, sDefaultFont);
	nvgFontSize(vg, SPLIT_TEXT);
	nvgFontBlur(vg, 0.0);
	nvgTextLetterSpacing(vg, -0.5f);
}

void Gfx_Shape(void* vg, Vec2f center, f32 scale, s16 rot, const Vec2f* p, u32 num) {
	bool move = true;
	
	for (int i = num - 1; !(i < 0); i--) {
		
		if (p[i].x == FLT_MAX || p[i].y == FLT_MAX) {
			move = true;
			continue;
		}
		
		Vec2f zero = { 0 };
		Vec2f pos = {
			p[i].x * scale,
			p[i].y * scale,
		};
		f32 dist = Vec2f_DistXZ(zero, pos);
		s16 yaw = Vec2f_Yaw(zero, pos);
		
		pos.x = center.x + SinS((s32)(yaw + rot)) * dist;
		pos.y = center.y + CosS((s32)(yaw + rot)) * dist;
		
		if ( move ) {
			nvgMoveTo(vg, pos.x, pos.y);
			move = false;
			i++;
			continue;
		} else
			nvgLineTo(vg, pos.x, pos.y);
	}
}

void Gfx_Vector(void* vg, Vec2f center, f32 scale, s16 rot, const VectorGfx* gfx) {
	Gfx_Shape(vg, center, scale, rot, gfx->pos, gfx->num);
}

void Gfx_DrawRounderOutline(void* vg, Rect rect, NVGcolor color) {
	nvgBeginPath(vg);
	nvgFillColor(vg, color);
	nvgRoundedRect(vg, rect.x - 1, rect.y - 1, rect.w + 2, rect.h + 2, SPLIT_ROUND_R);
	nvgRoundedRect(vg, rect.x, rect.y, rect.w, rect.h, SPLIT_ROUND_R);
	
	nvgPathWinding(vg, NVG_HOLE);
	nvgFill(vg);
}

void Gfx_DrawRounderOutlineWidth(void* vg, Rect rect, NVGcolor color, int width) {
	nvgBeginPath(vg);
	nvgFillColor(vg, color);
	nvgRoundedRect(vg, rect.x - width, rect.y - width, rect.w + width * 2, rect.h + width * 2, SPLIT_ROUND_R);
	nvgRoundedRect(vg, rect.x, rect.y, rect.w, rect.h, SPLIT_ROUND_R);
	nvgPathWinding(vg, NVG_HOLE);
	nvgFill(vg);
}

void Gfx_DrawRounderRect(void* vg, Rect rect, NVGcolor color) {
	nvgBeginPath(vg);
	nvgFillColor(vg, color);
	nvgRoundedRect(vg, rect.x, rect.y, rect.w, rect.h, SPLIT_ROUND_R);
	nvgFill(vg);
}

void Gfx_DrawStripes(void* vg, Rect rect) {
	Vec2f shape[] = {
		{ 0,    0    }, { 0.6f, 1 },
		{ 0.8f, 1    }, { 0.2f, 0 },
		{ 0,    0    },
	};
	
	nvgSave(vg);
	nvgScissor(vg, UnfoldRect(rect));
	nvgFillColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 5, 1.f));
	
	nvgBeginPath(vg);
	for (int i = -8; i < rect.w; i += 8)
		Gfx_Shape(vg, Vec2f_New(rect.x + i, rect.y), 20.0f, 0, shape, ArrCount(shape));
	
	nvgFill(vg);
	nvgResetScissor(vg);
	nvgRestore(vg);
}

f32 Gfx_TextWidth(void* vg, const char* txt) {
	f32 bounds[4];
	
	nvgSave(vg);
	Gfx_SetDefaultTextParams(vg);
	nvgTextAlign(vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_LEFT);
	nvgTextBounds(vg, 0, 0, txt, 0, bounds);
	nvgRestore(vg);
	
	return bounds[2];
}

Rect Gfx_TextRect(void* vg, Rect r, enum NVGalign align, const char* txt) {
	f32 width = Gfx_TextWidth(vg, txt);
	
#if 0
	if (align & NVG_ALIGN_CENTER) {
		r.x += r.w * 0.5f - width * 0.5f;
		r.w = width;
	} else if (align & NVG_ALIGN_LEFT) {
		r.x += SPLIT_ELEM_X_PADDING;
		r.w = width;
	} else {
		Rect c = r;
		
		c.x += SPLIT_ELEM_X_PADDING;
		c.w -= SPLIT_ELEM_X_PADDING * 2;
		
		r.x += r.w - SPLIT_ELEM_X_PADDING - width;
		r.w = width;
		
		r = Rect_Clamp(r, c);
	}
#else
	if (align & NVG_ALIGN_LEFT) {
		r.x += SPLIT_ELEM_X_PADDING;
		r.w = width;
	} else if (align & NVG_ALIGN_RIGHT) {
		Rect c = r;
		
		c.x += SPLIT_ELEM_X_PADDING;
		c.w -= SPLIT_ELEM_X_PADDING * 2;
		
		r.x += r.w - SPLIT_ELEM_X_PADDING - width;
		r.w = width;
		
		r = Rect_Clamp(r, c);
	} else {
		r.x += r.w * 0.5f - width * 0.5f;
		r.w = width;
	}
#endif
	
	return r;
}

Rect Gfx_TextRectMinMax(void* vg, Rect r, enum NVGalign align, const char* txt, int min, int max) {
	r = Gfx_TextRect(vg, r, align, txt);
	
	f32 wmin = 0;
	f32 wmax = 0;
	
	if (min) wmin = Gfx_TextWidth(vg, x_strndup(txt, min));
	if (max) wmax = Gfx_TextWidth(vg, x_strndup(txt, max));
	
	r.x += wmin;
	r.w = wmax - wmin;
	
	return r;
}

static bool sShadow;

static Vec2f* Gfx_Underline(void* vg, const char** txt) {
	if (strchr(*txt, '#')) {
		static Vec2f w;
		char* p;
		char* t;
		
		t = x_rep(*txt, " ", "_");
		p = strchr(t, '#');
		*p = '\0';
		
		w.x = Gfx_TextWidth(vg, t);
		p++; p[1] = '\0';
		w.y = Gfx_TextWidth(vg, p);
		
		*txt = x_rep(*txt, "#", "");
		
		return &w;
	}
	
	return NULL;
}

void Gfx_Text(void* vg, Rect r, enum NVGalign align, NVGcolor col, const char* txt) {
	Vec2f* undline = Gfx_Underline(vg, &txt);
	Rect nr = Gfx_TextRect(vg, r, align, txt);
	
	r = Rect_Clamp(nr, r);
	align = NVG_ALIGN_MIDDLE | NVG_ALIGN_LEFT;
	
	Gfx_SetDefaultTextParams(vg);
	nvgTextAlign(vg, align);
	
	if (sShadow) {
		sShadow = false;
		nvgSave(vg);
		nvgFontBlur(vg, 1.0f);
		nvgFillColor(vg, Theme_GetColor(THEME_SHADOW, 255, 1.0f));
		nvgText(vg, r.x, r.y + r.h * 0.5 + 1, txt, NULL);
		nvgText(vg, r.x, r.y + r.h * 0.5 + 1, txt, NULL);
		nvgRestore(vg);
	}
	
	nvgFillColor(vg, col);
	nvgText(vg, r.x, r.y + r.h * 0.5 + 1, txt, NULL);
	
	if (undline) {
		nvgBeginPath(vg);
		nvgStrokeWidth(vg, GetSplitPixel() * 0.5f);
		nvgMoveTo(vg, r.x + undline->x, r.y + r.h - GetSplitPixel() * 2.0f);
		nvgLineTo(vg, r.x + undline->x + undline->y, r.y + r.h - GetSplitPixel() * 2.0f);
		nvgFill(vg);
	}
}

void Gfx_Icon(void* vg, Rect r, NVGcolor col, int icon) {
	int scale = SPLIT_ICON - r.h;
	
	r = Rect_Scale(r, 0, scale / 2);
	r.w = SPLIT_ICON;
	
	NVGpaint paint = nvgImagePattern(vg, UnfoldRect(r), 0, icon, 1.0f);
	
	paint.innerColor = col;
	paint.outerColor = col;
	
	nvgBeginPath(vg);
	nvgRect(vg, UnfoldRect(r));
	
	nvgFillPaint(vg, paint);
	nvgFill(vg);
	
}

void Gfx_TextShadow(void* vg) {
	sShadow = true;
}

////////////////////////////////////////////////////////////////////////////////

static ElementQueCall* Element_QueueElement(NanoGrid* nano, Split* split, ElementFunc func, void* arg, const char* elemFunc) {
	ElementQueCall* node;
	
	node = new(ElementQueCall);
	Node_Add(sElemState->head, node);
	node->nano = nano;
	node->split = split;
	node->func = func;
	node->arg = arg;
	node->update = true;
	
	if (sElemState->forceDisable)
		((Element*)arg)->disableTemp = true;
	
	node->elemFunc = elemFunc;
	
	return node;
}

static s32 Element_PressCondition(NanoGrid* nano, Split* split, Element* this) {
	if (
		!nano->state.blockElemInput &
		(split->cursorInSplit || this->header) &&
		!split->blockCursor &&
		!split->elemBlockCursor &&
		(!sElemState->blockerTemp || this->type != ELEM_TYPE_BOX) &&
		fabsf(split->scroll.voffset - split->scroll.offset) < 5
	) {
		if (this->header) {
			Rect r = Rect_AddPos(this->rect, split->headRect);
			
			return Rect_PointIntersect(&r, nano->input->cursor.pos.x, nano->input->cursor.pos.y);
		} else {
			if (Input_GetCursor(nano->input, CLICK_ANY)->hold)
				if (!Rect_PointIntersect(&this->rect, UnfoldVec2(split->cursorPressPos)))
					return false;
			
			return Split_CursorInRect(split, &this->rect);
		}
	}
	
	return false;
}

int DummySplit_InputAcces(NanoGrid* nano, Split* split, Rect* r) {
	if (
		!nano->state.blockElemInput &
		split->cursorInSplit &&
		!split->blockCursor &&
		!split->elemBlockCursor &&
		!sElemState->textbox
	) {
		if (r) {
			if (Input_GetCursor(nano->input, CLICK_ANY)->hold)
				if (!Rect_PointIntersect(r, UnfoldVec2(split->cursorPressPos)))
					return false;
			
			return Split_CursorInRect(split, r);
		}
		
		return true;
	}
	
	return false;
}

int DummyGrid_InputCheck(NanoGrid* nano) {
	if (
		!nano->state.blockElemInput &
		!sElemState->textbox
	)
		return true;
	
	return false;
}

#define ELEM_PRESS_CONDITION(this) Element_PressCondition(sElemState->nano, sElemState->split, &(this)->element)

void Element_SetContext(NanoGrid* setGeo, Split* setSplit) {
	sElemState->split = setSplit;
	sElemState->nano = setGeo;
}

////////////////////////////////////////////////////////////////////////////////

#define ELEMENT_QUEUE_CHECK(...) if (this->element.disabled || this->element.disableTemp) { \
			__VA_ARGS__ \
			goto queue_element; \
}
#define ELEMENT_QUEUE(draw)      goto queue_element; queue_element: \
		Element_QueueElement(sElemState->nano, sElemState->split, \
			draw, this, __FUNCTION__);

#define SPLIT sElemState->split
#define NANO  sElemState->nano

#define Element_Action(func) if (this->element.func) this->element.func()

////////////////////////////////////////////////////////////////////////////////

static void Element_ButtonDraw(ElementQueCall* call) {
	void* vg = call->nano->vg;
	ElButton* this = call->arg;
	Rect r = this->element.rect;
	NVGcolor light = Theme_Mix(0.10f, this->element.base, this->element.light);
	NVGcolor text = Theme_Mix(0.20f, this->element.light, this->element.texcol);
	
	Gfx_DrawRounderOutline(vg, r, this->element.light);
	Gfx_DrawRounderRect(vg, r, light);
	
	nvgScissor(vg, UnfoldRect(this->element.rect));
	
	if (this->element.iconId) {
		Gfx_Icon(vg, Rect_ShrinkX(r, SPLIT_TEXT_PADDING), text, this->element.iconId);
		r = Rect_ShrinkX(r, r.h);
	}
	
	if (this->element.name)
		Gfx_Text(vg, r, this->align, this->element.texcol, this->element.name);
	
	nvgResetScissor(vg);
}

s32 Element_Button(ElButton* this) {
	int change = 0;
	
	osAssert(NANO && SPLIT);
	
	if (this->element.type == ELEM_TYPE_NONE) {
		this->element.type = ELEM_TYPE_BUTTON;
		
		if (this->element.nameState == ELEM_NAME_STATE_INIT)
			this->element.nameState = ELEM_NAME_STATE_HIDE;
	}
	
	if (!this->element.toggle)
		this->state = 0;
	
	ELEMENT_QUEUE_CHECK();
	
	if (ELEM_PRESS_CONDITION(this)) {
		if (Input_SelectClick(NANO->input, CLICK_L)) {
			change = true;
			
			this->state ^= 1;
			if (this->element.toggle)
				this->element.toggle ^= 0b10;
		}
	}
	
	ELEMENT_QUEUE(Element_ButtonDraw);
	
	return change;
}

////////////////////////////////////////////////////////////////////////////////

static void Element_ColorBoxDraw(ElementQueCall* call) {
	void* vg = call->nano->vg;
	ElColor* this = call->arg;
	rgb8_t color = {
		0x20, 0x20, 0x20,
	};
	
	if (this->prop.rgb8) {
		color = *this->prop.rgb8;
		Gfx_DrawRounderRect(vg, this->element.rect, Theme_Mix(0.10, nvgRGBA(UnfoldRGB(color), 0xFF), this->element.light));
	} else {
		Gfx_DrawRounderRect(vg, this->element.rect, Theme_Mix(0.10, nvgRGBA(UnfoldRGB(color), 0xFF), this->element.light));
		Gfx_DrawStripes(vg, this->element.rect);
	}
	
	Gfx_DrawRounderOutline(vg, this->element.rect, this->element.light);
}

void Element_Color(ElColor* this) {
	this->element.disabled = (this->prop.rgb8 == NULL);
	
	if (this->element.type == ELEM_TYPE_NONE)
		this->element.type = ELEM_TYPE_COLOR;
	
	ELEMENT_QUEUE_CHECK();
	
	if (this->prop.rgb8) {
		if (ELEM_PRESS_CONDITION(this)) {
			if ( Input_GetCursor(NANO->input, CLICK_L)->press) {
				Rect* rect[] = { &SPLIT->rect, &SPLIT->headRect };
				
				ContextMenu_Init(NANO, &this->prop, this, CONTEXT_PROP_COLOR,
					Rect_AddPos(this->element.rect, *rect[this->element.header]));
			}
		}
	}
	
	ELEMENT_QUEUE(Element_ColorBoxDraw);
}

////////////////////////////////////////////////////////////////////////////////

static void Textbox_SetValue(ElTextbox* this) {
	if (this->type != TEXTBOX_STR) {
		if (this == sElemState->textbox) {
			this->val.update = true;
		} else {
			if (this->val.update) {
				switch (this->type) {
					case TEXTBOX_INT:
						this->val.value = sint(this->txt);
						break;
					case TEXTBOX_HEX:
						this->val.value = shex(this->txt);
						break;
					case TEXTBOX_F32:
						this->val.value = sfloat(this->txt);
						break;
						
					case TEXTBOX_STR:
						break;
				}
				
				if (this->val.min != 0 || this->val.max != 0)
					this->val.value = clamp(this->val.value, this->val.min, this->val.max);
				
				s32 size = this->size ? this->size : sizeof(this->txt);
				
				switch (this->type) {
					case TEXTBOX_INT:
						snprintf(this->txt, size, "%d", (s32)this->val.value);
						
						break;
					case TEXTBOX_HEX:
						snprintf(this->txt, size, "0x%X", (s32)this->val.value);
						
						break;
					case TEXTBOX_F32:
						snprintf(this->txt, size, "%.4f", (f32)this->val.value);
						
						break;
						
					case TEXTBOX_STR:
						break;
				}
				
				this->val.update = false;
			}
		}
	}
}

void Element_ClearActiveTextbox(NanoGrid* nano) {
	ElementState* state = nano->elemState;
	ElTextbox* this = state->textbox;
	
	if (!this) return;
	
	Textbox_SetValue(this);
	state->textbox = NULL;
	
	if (this->doBlock) {
		if (Input_ClearState(nano->input, INPUT_BLOCK)) {
			warn("Duplicate Clear on INPUT_BLOCK, press enter");
			cli_getc();
		}
		
		state->blockerTextbox--;
		state->blockerTemp = 2;
		this->ret = true;
		this->doBlock = false;
		this->charoff = 0;
	}
	
	warn("Textbox: Clear");
}

static void CharOffClamp(ElTextbox* this, void* vg) {
	this->charoff = clamp(
		this->charoff,
		0,
		clamp_min(
			(Gfx_TextWidth(vg, this->txt) + Gfx_TextWidth(vg, "--")) - this->element.rect.w,
			0));
}

static void Textbox_Set(ElTextbox* this, Split* split) {
	if (!sElemState->textbox) {
		Textbox_SetValue(this);
		this->selA = 0;
		this->selB = strlen(this->txt);
		this->charoff = __INT32_MAX__;
		CharOffClamp(this, sElemState->nano->vg);
		
		strncpy(sElemState->textStoreBuf, this->txt, TEXTBOX_BUFFER_SIZE);
	} else if (sElemState->textbox != this)
		return;
	
	sElemState->textbox = this;
	sElemState->textbox->split = split;
	
	warn("Textbox: Set [%s]", this->txt);
}

static InputType* Textbox_GetKey(NanoGrid* nano, int key) {
	return &nano->input->key[key];
}

static InputType* Textbox_GetMouse(NanoGrid* nano, CursorClick key) {
	return &nano->input->cursor.clickList[key];
}

void Element_SetActiveTextbox(NanoGrid* nano, Split* split, ElTextbox* this) {
	if (sElemState->textbox != this) {
		Element_ClearActiveTextbox(nano);
		Textbox_Set(this, split);
	}
}

void Element_UpdateTextbox(NanoGrid* nano) {
	ElTextbox* this = sElemState->textbox;
	bool cPress = Textbox_GetMouse(nano, CLICK_L)->press;
	bool cHold = Textbox_GetMouse(nano, CLICK_L)->hold;
	
	if (!cHold) Decr(sElemState->blockerTemp);
	sElemState->breathYaw += DegToBin(3);
	sElemState->breath = (SinS(sElemState->breathYaw) + 1.0f) * 0.5;
	
	if (!this) return;
	
	#define HOLDREPKEY(key) (Textbox_GetKey(nano, key)->press || Textbox_GetKey(nano, key)->dual)
	Split* split = this->split;
	this->modified = false;
	this->editing = true;
	
	if (this->clearIcon) {
		if (Textbox_GetMouse(nano, CLICK_L)->press) {
			if (Split_CursorInRect(split, &this->clearRect)) {
				arrzero(this->txt);
				this->modified = true;
				Element_ClearActiveTextbox(nano);
				return;
			}
		}
	}
	
	if (Textbox_GetKey(nano, KEY_ESCAPE)->press) {
		strncpy(this->txt, sElemState->textStoreBuf, TEXTBOX_BUFFER_SIZE);
		this->modified = true;
		Element_ClearActiveTextbox(nano);
		this->ret = -1;
		
		return;
	}
	
	if (!this->doBlock) {
		if (Input_SetState(nano->input, INPUT_BLOCK)) {
			warn("Duplicate Set on INPUT_BLOCK, press enter");
			cli_getc();
		}
		
		sElemState->blockerTextbox++;
		this->doBlock = true;
		return;
	} else if (Textbox_GetKey(nano, KEY_ENTER)->press || (!Rect_PointIntersect(&this->element.rect, UnfoldVec2(split->cursorPos)) && cPress)) {
		Element_ClearActiveTextbox(nano);
		return;
	}
	
	void* vg = nano->vg;
	Input* input = nano->input;
	bool ctrl = Textbox_GetKey(nano, KEY_LEFT_CONTROL)->hold;
	bool paste = Textbox_GetKey(nano, KEY_LEFT_CONTROL)->hold && Textbox_GetKey(nano, KEY_V)->press;
	bool copy = Textbox_GetKey(nano, KEY_LEFT_CONTROL)->hold && Textbox_GetKey(nano, KEY_C)->press;
	bool kShift = Textbox_GetKey(nano, KEY_LEFT_SHIFT)->hold;
	bool kEnd = Textbox_GetKey(nano, KEY_END)->press;
	bool kHome = Textbox_GetKey(nano, KEY_HOME)->press;
	bool kRem = Textbox_GetKey(nano, KEY_BACKSPACE)->press || Textbox_GetKey(nano, KEY_BACKSPACE)->dual;
	s32 dir = HOLDREPKEY(KEY_RIGHT) - HOLDREPKEY(KEY_LEFT);
	s32 maxSize = this->size ? this->size : sizeof(this->txt);
	const s32 LEN = strlen(this->txt);
	
	if (ctrl && Textbox_GetKey(nano, KEY_A)->press) {
		this->selPivot = this->selA = 0;
		this->selPos = this->selB = LEN;
		this->charoff = __INT32_MAX__;
		CharOffClamp(this, vg);
		
		return;
	}
	
	if (cPress || cHold) {
		Rect r = this->element.rect;
		s32 id = 0;
		f32 dist = FLT_MAX;
		
		r.x -= this->charoff;
		
		if (this->isClicked && input->cursor.dragDist < 4)
			return;
		
		for (char* end = this->txt; ; end++) {
			Vec2s p;
			f32 ndist;
			
			Rect tr = Gfx_TextRectMinMax(vg, r, this->align, this->txt, 0, end - this->txt);
			
			p.x = RectW(tr) - 2;
			p.y = split->cursorPos.y;
			
			if ((ndist = Vec2s_DistXZ(p, split->cursorPos)) < dist) {
				dist = ndist;
				
				id = end - this->txt;
			}
			
			if (*end == '\0')
				break;
		}
		
		if (cPress && Rect_PointIntersect(&this->element.rect, UnfoldVec2(split->cursorPos))) {
			if (Textbox_GetMouse(nano, CLICK_L)->dual) {
				if (this->selA == this->selB) {
					for (; this->selA > 0; this->selA--)
						if (ispunct(this->txt[this->selA - 1]))
							break;
					for (; this->selB < strlen(this->txt); this->selB++)
						if (ispunct(this->txt[this->selB]))
							break;
				} else {
					this->selA = 0;
					this->selB = strlen(this->txt);
				}
				return;
			}
			this->selPos = this->selPivot = this->selA = this->selB = id;
			this->isClicked = true;
			warn("Textbox: Select %d", id);
		} else if (cHold && this->isClicked) {
			this->selA = Min(this->selPivot, id );
			this->selPos = this->selB = Max(this->selPivot, id );
			
			if (split->cursorPos.x < this->element.rect.x)
				this->charoff -= Gfx_TextWidth(vg, "-");
			if (split->cursorPos.x > RectW(this->element.rect))
				this->charoff += Gfx_TextWidth(vg, "-");
			CharOffClamp(this, vg);
		}
	} else {
		this->isClicked = false;
		
		nested(void, Remove, ()) {
			s32 min = this->selA;
			s32 max = this->selB - min;
			
			if (this->selA == this->selB) {
				min = clamp_min(min - 1, 0);
				max = 1;
			}
			
			strrem(&this->txt[min], max);
			this->selA = this->selB = min;
			this->modified = true;
		};
		
		if (kRem)
			Remove();
		
		if (dir || kEnd || kHome) {
			nested(s32, Shift, (s32 cur, s32 dir)) {
				if (ctrl && dir) {
					for (var_t i = cur + dir; i > 0 && i < LEN; i += dir) {
						if (ispunct(this->txt[i]) != ispunct(this->txt[i + dir]))
							return i + Max(0, dir);
					}
				}
				
				return cur + dir;
			};
			
			s32 val = 0;
			s32 dirPoint[] = { this->selA, this->selB };
			bool shiftMul = this->selA == this->selB;
			
			//crustify
            if (kHome)       val = 0;
            else if (kEnd)   val = LEN;
            else if (kShift) val = Shift(this->selPos, dir);
            else             val = Shift(dirPoint[clamp_min(dir, 0)], dir * shiftMul);
			//uncrustify
			val = clamp(val, 0, LEN);
			
			if (kShift) {
				this->selPos = val;
				this->selA = Min(this->selPos, this->selPivot);
				this->selB = Max(this->selPos, this->selPivot);
			} else
				this->selPos = this->selPivot = this->selA = this->selB = val;
			
			Rect r = this->element.rect;
			r.x -= this->charoff;
			
			r = Gfx_TextRectMinMax(vg, r, this->align, this->txt, this->selA, this->selB);
			
			int minx = (this->element.rect.x + SPLIT_ELEM_X_PADDING * 4);
			int maxx = (RectW(this->element.rect) - SPLIT_ELEM_X_PADDING * 4);
			
			if (dir < 0 || kHome) {
				if (r.x < minx)
					this->charoff -= minx - r.x;
			} else if (dir > 0 || kEnd) {
				if (RectW(r) > maxx)
					this->charoff -= maxx - RectW(r);
			}
			
			CharOffClamp(this, vg);
		}
		
		if (input->buffer[0] || paste) {
			const char* origin = input->buffer;
			
			if (paste) origin = Input_GetClipboardStr(nano->input);
			if (this->selA != this->selB) Remove();
			
			strnins(this->txt, origin, this->selA, maxSize + 1);
			this->modified = true;
			
			this->selPos = this->selPivot = this->selB = this->selA = clamp_max(this->selA + 1, maxSize);
		}
		
		if (copy)
			Input_SetClipboardStr(nano->input, x_strndup(&this->txt[this->selA], this->selB - this->selA));
		
	}
	
#undef HOLDREPKEY
}

static void Element_TextboxDraw(ElementQueCall* call) {
	void* vg = call->nano->vg;
	ElTextbox* this = call->arg;
	
	if (sElemState->textbox == this)
		Gfx_DrawRounderRect(vg, this->element.rect, this->element.shadow);
	else
		Gfx_DrawRounderRect(vg, this->element.rect, this->element.base);
	
	if (sElemState->textbox == this)
		Gfx_DrawRounderOutline(vg, this->element.rect, Theme_GetColor(THEME_ELEMENT_LIGHT, 255, 1.0f));
	else
		Gfx_DrawRounderOutline(vg, this->element.rect, this->element.light);
	
	nvgFillColor(vg, this->element.texcol);
	nvgScissor(vg, UnfoldRect(this->element.rect));
	
	if (sElemState->textbox == this) {
		if (this->selA == this->selB) {
			Rect r = this->element.rect;
			s32 min = this->selA;
			s32 max = this->selB;
			
			r.x -= this->charoff;
			
			r = Gfx_TextRectMinMax(vg, r, this->align, this->txt, min, max);
			r.y++;
			r.h -= 2;
			r.w = 2;
			
			Gfx_DrawRounderRect(vg, r, Theme_GetColor(THEME_TEXT, 255, 1.0f));
		}
	}
	
	Rect r = this->element.rect;
	
	r.x -= this->charoff;
	Gfx_Text(vg, r, this->align, this->element.texcol, this->txt);
	nvgResetScissor(vg);
	
	if (this->element.disableTemp || this->element.disabled)
		Gfx_DrawStripes(vg, this->element.rect);
	
	if (sElemState->textbox == this) {
		if (this->selA != this->selB) {
			Rect r = this->element.rect;
			s32 min = this->selA;
			s32 max = this->selB;
			
			r.x -= this->charoff;
			r = Gfx_TextRectMinMax(vg, r, this->align, this->txt, min, max);
			r = Rect_Clamp(r, this->element.rect);
			
			nvgScissor(vg, UnfoldRect(r));
			Gfx_DrawRounderRect(vg, r, Theme_GetColor(THEME_TEXT, 255, 1.0f));
			
			r = this->element.rect;
			r.x -= this->charoff;
			Gfx_Text(vg, r, this->align, this->element.shadow, this->txt);
			nvgResetScissor(vg);
		}
	}
	
	if (this->clearIcon) {
		Rect r = this->clearRect;
		
		Gfx_DrawRounderRect(vg, r, Theme_Mix(0.25f, this->element.base, this->element.light));
		Gfx_DrawRounderOutline(vg, r, this->element.light);
		
		nvgBeginPath(vg);
		nvgFillColor(vg, Theme_Mix(0.25f, this->element.light, Theme_GetColor(THEME_HIGHLIGHT, 255, 1.0f)));
		Gfx_Vector(vg, Vec2f_New(UnfoldVec2(r)), r.h / 16.0f, 0, gAssets.cross);
		nvgFill(vg);
	}
}

s32 Element_Textbox(ElTextbox* this) {
	int ret = 0;
	
	this->element.type = ELEM_TYPE_TEXTBOX;
	
	osAssert(NANO && SPLIT);
	
	if (this != sElemState->textbox) {
		if (this->editing)
			ret = true;
		else
			this->modified = false;
		this->editing = false;
		
		ELEMENT_QUEUE_CHECK();
	}
	
	if (this->clearIcon) {
		this->clearRect.x = this->element.rect.x + this->element.rect.w - this->element.rect.h;
		this->clearRect.w = this->element.rect.h;
		this->clearRect.y = this->element.rect.y;
		this->clearRect.h = this->element.rect.h;
		
		this->element.rect.w -= this->clearRect.w;
	}
	
	if (ELEM_PRESS_CONDITION(this)) {
		if ( Input_GetCursor(NANO->input, CLICK_L)->press) {
			if (this->clearIcon) {
				if (Split_CursorInRect(SPLIT, &this->clearRect)) {
					arrzero(this->txt);
					this->modified = true;
					
					if (this == sElemState->textbox)
						Element_ClearActiveTextbox(NANO);
					
					goto queue_element;
				}
			}
			
			Textbox_Set(this, SPLIT);
		}
	}
	
	ELEMENT_QUEUE(Element_TextboxDraw);
	
	return ret;
}

////////////////////////////////////////////////////////////////////////////////

static void Element_TextDraw(ElementQueCall* call) {
	void* vg = call->nano->vg;
	// Split* split = call->split;
	ElText* this = call->arg;
	
	Gfx_SetDefaultTextParams(vg);
	
	if (!this->element.disabled)
		nvgFillColor(vg, Theme_GetColor(THEME_TEXT, 255, 1.0f));
	else
		nvgFillColor(vg, Theme_Mix(0.45, Theme_GetColor(THEME_ELEMENT_BASE, 255, 1.00f), Theme_GetColor(THEME_TEXT, 255, 1.15f)));
	
	if (this->element.dispText) {
		Rect r = this->element.rect;
		
		r.x = this->element.posTxt.x;
		r.w = this->element.rect.x - r.x - SPLIT_ELEM_X_PADDING;
		
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
		nvgText(vg, this->element.posTxt.x, this->element.posTxt.y, this->element.name, NULL);
	} else {
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgScissor(vg, UnfoldRect(this->element.rect));
		nvgText(vg, this->element.rect.x, this->element.rect.y + this->element.rect.h * 0.5 + 1, this->element.name, NULL);
		nvgResetScissor(vg);
	}
}

ElText* Element_Text(const char* txt) {
	ElText* this = new(ElText);
	
	osAssert(NANO && SPLIT);
	this->element.name = txt;
	this->element.doFree = true;
	
	ELEMENT_QUEUE(Element_TextDraw);
	
	return this;
}

////////////////////////////////////////////////////////////////////////////////

static void Element_CheckboxDraw(ElementQueCall* call) {
	void* vg = call->nano->vg;
	// Split* split = call->split;
	ElCheckbox* this = call->arg;
	Vec2f center;
	const Vec2f sVector_Cross[] = {
		//crustify
        { .x = -10, .y = 10  }, { .x = -7,  .y = 10  },
        { .x = 0,   .y = 3   }, { .x = 7,   .y = 10  },
        { .x = 10,  .y = 10  }, { .x = 10,  .y = 7   },
        { .x = 3,   .y = 0   }, { .x = 10,  .y = -7  },
        { .x = 10,  .y = -10 }, { .x = 7,   .y = -10 },
        { .x = 0,   .y = -3  }, { .x = -7,  .y = -10 },
        { .x = -10, .y = -10 }, { .x = -10, .y = -7  },
        { .x = -3,  .y = 0   }, { .x = -10, .y = 7   },
		//uncrustify
	};
	Rect r = this->element.rect;
	
	r.w = r.h;
	
	if (this->element.toggle)
		Math_DelSmoothStepToF(&this->lerp, 0.8f - sElemState->breath * 0.08, 0.178f, 0.1f, 0.0f);
	else
		Math_DelSmoothStepToF(&this->lerp, 0.0f, 0.268f, 0.1f, 0.0f);
	
	Gfx_DrawRounderOutline(vg, r, this->element.light);
	Gfx_DrawRounderRect(vg, r, this->element.base);
	
	NVGcolor col = Theme_Mix(this->lerp, this->element.shadow, this->element.prim);
	f32 flipLerp = 1.0f - this->lerp;
	
	flipLerp = (1.0f - powf(flipLerp, 1.6));
	center.x = r.x + r.w * 0.5;
	center.y = r.y + r.h * 0.5;
	
	col.a = flipLerp * 1.67;
	col.a = clamp_min(col.a, 0.80);
	
	nvgBeginPath(vg);
	nvgFillColor(vg, col);
	
	for (int i = 0; i < ArrCount(sVector_Cross); i++) {
		s32 wi = wrap(i, 0, ArrCount(sVector_Cross) - 1);
		Vec2f zero = { 0 };
		Vec2f pos = {
			sVector_Cross[wi].x * 0.75,
			sVector_Cross[wi].y * 0.75,
		};
		f32 dist = Vec2f_DistXZ(zero, pos);
		s16 yaw = Vec2f_Yaw(zero, pos);
		
		dist = lerpf(flipLerp, 4, dist);
		dist = lerpf((this->lerp > 0.5 ? 1.0 - this->lerp : this->lerp), dist, powf((dist * 0.1), 0.15) * 3);
		
		pos.x = center.x + SinS(yaw) * dist;
		pos.y = center.y + CosS(yaw) * dist;
		
		if ( i == 0 )
			nvgMoveTo(vg, pos.x, pos.y);
		else
			nvgLineTo(vg, pos.x, pos.y);
	}
	nvgFill(vg);
}

s32 Element_Checkbox(ElCheckbox* this) {
	int tog = this->element.toggle;
	
	if (this->element.type == ELEM_TYPE_NONE)
		this->element.type = ELEM_TYPE_CHECKBOX;
	
	osAssert(NANO && SPLIT);
	
	ELEMENT_QUEUE_CHECK();
	
	if (ELEM_PRESS_CONDITION(this)) {
		if ( Input_GetCursor(NANO->input, CLICK_L)->press) {
			this->element.toggle ^= 1;
		}
	}
	
	ELEMENT_QUEUE(Element_CheckboxDraw);
	
	return tog - this->element.toggle;
}

////////////////////////////////////////////////////////////////////////////////

static void Element_Slider_SetCursorToVal(NanoGrid* nano, Split* split, ElSlider* this) {
	Rect r = split->dispRect;
	
	if (this->element.header)
		r = split->headRect;
	
	f32 x = r.x + this->element.rect.x + this->element.rect.w * this->value;
	f32 y = r.y + this->element.rect.y + this->element.rect.h * 0.5;
	
	Input_SetMousePos(nano->input, x, y);
}

static void Element_Slider_SetTextbox(Split* split, ElSlider* this) {
	if (!sElemState->textbox) {
		this->isTextbox = true;
		
		this->textBox.element.rect = this->element.rect;
		this->textBox.align = NVG_ALIGN_CENTER;
		this->textBox.size = 32;
		this->textBox.element.header = this->element.header;
		
		this->textBox.type = this->isInt ? TEXTBOX_INT : TEXTBOX_F32;
		this->textBox.val.value = lerpf(this->value, this->min, this->max);
		this->textBox.val.max = this->max;
		this->textBox.val.min = this->min;
		this->textBox.val.update = true;
		Textbox_Set(&this->textBox, split);
	}
}

static void Element_SliderDraw(ElementQueCall* call) {
	void* vg = call->nano->vg;
	// Split* split = call->split;
	ElSlider* this = call->arg;
	Rectf32 rect;
	f32 step = (this->max - this->min) * 0.5f;
	
	Math_DelSmoothStepToF(&this->vValue, this->value, 0.5f, step, 0.0f);
	
	Rect cr = this->element.rect;
	
	cr.x += 2;
	cr.y += 2;
	cr.w -= 4;
	cr.h -= 4;
	
	rect.x = cr.x;
	rect.y = cr.y;
	rect.h = cr.h;
	rect.w = clamp_min(cr.w * this->vValue, 0);
	
	Gfx_DrawRounderOutline(vg, this->element.rect, this->element.light);
	Gfx_DrawRounderRect(vg, this->element.rect, this->element.shadow);
	
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
	
	if (this->isInt)
		snprintf(this->textBox.txt, 31, sFmt[this->isInt], (s32)rint(lerpf(this->value, this->min, this->max)));
	
	else
		snprintf(this->textBox.txt, 31, sFmt[this->isInt], lerpf(this->value, this->min, this->max));
	
	nvgScissor(vg, UnfoldRect(this->element.rect));
	Gfx_Text(vg, this->element.rect, NVG_ALIGN_CENTER, this->element.texcol, this->textBox.txt);
	nvgResetScissor(vg);
}

int Element_Slider(ElSlider* this) {
	int ret = 0;
	
	osAssert(NANO && SPLIT);
	
	if (this->element.type == ELEM_TYPE_NONE)
		this->element.type = ELEM_TYPE_SLIDER;
	
	if (this->min == 0.0f && this->max == 0.0f)
		this->max = 1.0f;
	
	ELEMENT_QUEUE_CHECK();
	
	nested(void, BlockInput, ()) {
		if (!this->holdState)
			SPLIT->elemBlockCursor++;
		this->holdState = true;
	};
	
	nested(void, UnblockInput, ()) {
		if (this->holdState)
			SPLIT->elemBlockCursor--;
		this->holdState = false;
	};
	
	if (ELEM_PRESS_CONDITION(this) || this->holdState) {
		Input* input = NANO->input;
		u32 pos = false;
		
		SPLIT->splitBlockScroll++;
		
		if (!this->isSliding && !this->isTextbox) {
			if (Input_GetCursor(input, CLICK_L)->hold || Input_SelectClick(input, CLICK_L)) {
				if (input->cursor.dragDist > 1) {
					BlockInput();
					
					this->isSliding = true;
					Element_Slider_SetCursorToVal(NANO, SPLIT, this);
					
				} else if (Input_SelectClick(input, CLICK_L)) {
					BlockInput();
					
					this->isTextbox = true;
					Element_Slider_SetTextbox(SPLIT, this);
				}
			}
		}
		
		if (this->isSliding) {
			if (Input_GetCursor(input, CLICK_L)->release || !Input_GetCursor(input, CLICK_L)->hold) {
				this->isSliding = false;
				ret = true;
				
			} else {
				if (input->cursor.vel.x) {
					if (Input_GetKey(input, KEY_LEFT_SHIFT)->hold)
						this->value += (f32)input->cursor.vel.x * 0.0001f;
					
					else
						this->value += (f32)input->cursor.vel.x * 0.001f;
					
					if (this->min || this->max)
						this->value = clamp(this->value, 0.0f, 1.0f);
					
					pos = true;
				}
			}
		}
		
		if (this->isTextbox) {
			this->isTextbox = false;
			
			if (sElemState->textbox == &this->textBox) {
				this->isTextbox = true;
				this->holdState = true;
				
				Element_Textbox(&this->textBox);
				
				return lerpf(this->value, this->min, this->max);
			} else {
				UnblockInput();
				this->holdState = false;
				this->isSliding = false;
				this->isTextbox = false;
				Element_Slider_SetValue(this, this->isInt ? sint(this->textBox.txt) : sfloat(this->textBox.txt));
				ret = true;
				
				goto queue_element;
			}
		}
		
		if (!this->isSliding && !this->isTextbox) {
			UnblockInput();
			f32 scroll = Input_GetScrollRaw(NANO->input);
			
			if (scroll) {
				if (this->isInt) {
					f32 valueIncrement = 1.0f / (this->max - this->min);
					
					if (Input_GetKey(NANO->input, KEY_LEFT_SHIFT)->hold)
						this->value += valueIncrement * scroll * 5;
					
					else
						this->value += valueIncrement * scroll;
				} else {
					if (Input_GetKey(NANO->input, KEY_LEFT_SHIFT)->hold)
						this->value += scroll * 0.1;
					
					else if (Input_GetKey(NANO->input, KEY_LEFT_ALT)->hold)
						this->value += scroll * 0.001;
					
					else
						this->value += scroll * 0.01;
				}
				
				ret = true;
			}
		}
		
		if (pos) Element_Slider_SetCursorToVal(NANO, SPLIT, this);
	}
	
	ELEMENT_QUEUE(Element_SliderDraw);
	this->value = clamp(this->value, 0.0f, 1.0f);
	
	if (this->isSliding)
		Cursor_SetCursor(CURSOR_EMPTY);
	
	return ret + this->holdState;
}

////////////////////////////////////////////////////////////////////////////////

Vec2f sShapeArrow[] = {
	{ -5.0f, -2.5f  },
	{ 0.0f,  2.5f   },
	{ 5.0f,  -2.5f  },
	{ -5.0f, -2.5f  },
};

static void Element_ComboDraw(ElementQueCall* call) {
	ElCombo* this = call->arg;
	NanoGrid* nano = call->nano;
	void* vg = nano->vg;
	Arli* list = this->list;
	Rect r = this->element.rect;
	Vec2f center;
	
	Gfx_DrawRounderOutline(vg, r, this->element.light);
	Gfx_DrawRounderRect(vg, r, this->element.shadow);
	
	r = this->element.rect;
	
	if (this->controller) {
		const char* text = "(null)";
		
		if (this->title)
			text = this->title;
		else if (list)
			text = list->elemName(list, list->cur);
		
		nvgScissor(vg, UnfoldRect(r));
		Gfx_Text(vg, r, this->align, this->element.texcol, text);
		nvgResetScissor(vg);
		
		return;
	} else if (list && list->num) {
		Rect sr = r;
		NVGcolor color;
		
		f32 width = 0;
		const char* text = NULL;
		
		if (this->showDecID)
			text = x_fmt("%d:--", list->cur);
		
		else if (this->showHexID)
			text = x_fmt("%X:--", list->cur);
		
		if (text) {
			width += Gfx_TextWidth(vg, text);
			while (strend(text, "-"))
				strend(text, "-")[0] = '\0';
			Gfx_Text(
				vg, r, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE,
				this->element.texcol,
				text
			);
		}
		
		sr.x += width;
		sr.w -= 10 + SPLIT_ELEM_X_PADDING + width;
		nvgScissor(vg, UnfoldRect(sr));
		
		if (nano->contextMenu.element == (void*)this)
			color = this->element.light;
		
		else
			color = this->element.texcol;
		
		Gfx_Text(vg, sr, this->align, color, list->elemName(list, list->cur));
		nvgResetScissor(vg);
	} else
		Gfx_DrawStripes(vg, this->element.rect);
	
	center.x = r.x + r.w - 10;
	center.y = r.y + r.h * 0.5f;
	
	nvgBeginPath(vg);
	nvgFillColor(vg, this->element.light);
	Gfx_Shape(vg, center, 0.95, 0, sShapeArrow, ArrCount(sShapeArrow));
	nvgFill(vg);
}

int Element_Combo(ElCombo* this) {
	Arli* list = this->list;
	Set* set = &this->set;
	
	memzero(set);
	osAssert(NANO && SPLIT);
	
	if (this->element.type == ELEM_TYPE_NONE) {
		this->element.type = ELEM_TYPE_COMBO;
		
		if (this->align == 0)
			this->align = NVG_ALIGN_RIGHT;
		
		this->prevIndex = this->list ? this->list->cur : -16;
	}
	
	if (this->title)
		this->controller = true;
	
	ELEMENT_QUEUE_CHECK();
	
	if (list && list->num) {
		if (ELEM_PRESS_CONDITION(this)) {
			SPLIT->splitBlockScroll++;
			s32 scrollY = Input_GetScrollRaw(NANO->input);
			
			if (Input_GetCursor(NANO->input, CLICK_L)->release) {
				Rect* rect[] = { &SPLIT->rect, &SPLIT->headRect };
				
				ContextMenu_Init(NANO, list, this, CONTEXT_ARLI, Rect_AddPos(this->element.rect, *rect[this->element.header]));
				if (!this->controller)
					ScrollBar_FocusSlot(&NANO->contextMenu.scroll, list->cur, false);
			} else if (scrollY && !this->controller)
				Arli_Set(list, list->cur - scrollY);
		}
	} else
		this->element.disableTemp = true;
	
	ELEMENT_QUEUE(Element_ComboDraw);
	
	if (list && NANO->contextMenu.element != &this->element) {
		int prevIndex = this->prevIndex;
		int curIndex = list->cur;
		
		this->prevIndex = curIndex;
		
		if (!this->controller)
			this->element.faulty = (curIndex < 0 || curIndex >= list->num);
		
		if (curIndex - prevIndex)
			return curIndex;
	} else {
		this->element.faulty = false;
	}
	
	return -1;
}

////////////////////////////////////////////////////////////////////////////////

Rect Element_Tab_GetRect(ElTab* this, int index) {
	f32 w = this->element.rect.w / (f32)this->num;
	
	return Rect_New(
		this->element.rect.x + w * index,
		this->element.rect.y,
		w,
		SPLIT_TEXT_H
	);
}

static void Element_TabDraw(ElementQueCall* call) {
	// ElTab* this = call->arg;
	// void* vg = NANO->vg;
}

int Element_Tab(ElTab* this) {
	int id = this->index;
	
	osAssert(NANO && SPLIT);
	
	ELEMENT_QUEUE_CHECK();
	
	if (ELEM_PRESS_CONDITION(this)) {
		
		if (!Input_SelectClick(NANO->input, CLICK_L))
			goto queue_element;
		
		for (int i = 0; i < this->num; i++) {
			Rect r = Element_Tab_GetRect(this, i);
			
			if (Split_CursorInRect(SPLIT, &r)) {
				this->index = i;
				
				break;
			}
		}
	}
	
	ELEMENT_QUEUE(Element_TabDraw);
	
	return id - this->index;
}

////////////////////////////////////////////////////////////////////////////////

static Rect Element_Container_GetListElemRect(ElContainer* this, s32 i) {
	return ScrollBar_GetRect(&this->scroll, i);
}

static Rect Element_Container_GetDragRect(ElContainer* this, s32 i) {
	Rect r = Element_Container_GetListElemRect(this, i);
	
	r.y -= SPLIT_TEXT_H * 0.5f;
	
	return r;
}

static void Element_Container_NewInteract(ElContainer* this) {
	if (!this->controller) return;
	
	sElemState->interactContainer = this;
}

static void Element_ContainerDraw(ElementQueCall* call) {
	ElContainer* this = call->arg;
	NanoGrid* nano = call->nano;
	void* vg = nano->vg;
	Arli* list = this->list;
	Rect r = this->element.rect;
	Rect scissor = this->element.rect;
	NVGcolor cornerCol = this->element.shadow;
	
	cornerCol.a = 2.5f;
	Gfx_DrawRounderOutline(vg, r, cornerCol);
	Gfx_DrawRounderRect(vg, r, this->element.shadow);
	
	if (!list)
		return;
	
	scissor.x += 1;
	scissor.y += 1;
	scissor.w -= 2;
	scissor.h -= 2;
	
	if (scissor.w <= 0)
		return;
	if (scissor.h <= 0)
		return;
	
	nvgScissor(vg, UnfoldRect(scissor));
	
	if (Input_GetKey(NANO->input, KEY_LEFT_SHIFT)->hold)
		Math_SmoothStepToF(&this->copyLerp, 1.0f, 0.25f, 0.25f, 0.01f);
	else
		Math_SmoothStepToF(&this->copyLerp, 0.0f, 0.25f, 0.25f, 0.01f);
	
	if (this->controller && this != sElemState->interactContainer)
		if (this->list) this->list->cur = -1;
	
	for (int i = 0; i < list->num; i++) {
		Rect tr = Element_Container_GetListElemRect(this, i);
		
		if (this != (void*)sElemState->dragItem.src) {
			if (list->cur == i) {
				Rect vt = tr;
				NVGcolor col = this->element.prim;
				
				vt.x += 1; vt.w -= 2;
				vt.y += 1; vt.h -= 2;
				col.a = 0.85f;
				
				if (this->controller == true)
					col = Theme_Mix(0.25f, this->element.base, this->element.light);
				
				Gfx_DrawRounderRect(vg, vt, col);
			}
			
			if (Split_CursorInRect(SPLIT, &tr) && this->copyLerp > EPSILON) {
				Rect vt = tr;
				vt.x += 1; vt.w -= 2;
				vt.y += 1; vt.h -= 2;
				
				Gfx_DrawRounderRect(vg, vt, nvgHSLA(0.33f, 1.0f, 0.5f, 125 * this->copyLerp));
			}
		}
		
		f32 width = 0;
		const char* text = NULL;
		
		if (this->showDecID)
			text = x_fmt("%d:--", i);
		
		else if (this->showHexID)
			text = x_fmt("%X:--", i);
		
		if (text) {
			width += Gfx_TextWidth(vg, text);
			while (strend(text, "-"))
				strend(text, "-")[0] = '\0';
			Gfx_Text(
				vg, tr, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE,
				this->element.texcol,
				text
			);
		}
		
		tr.x += width;
		tr.w -= width;
		
		Gfx_Text(
			vg, tr, this->align,
			this->element.texcol,
			list->elemName(list, i)
		);
	}
	nvgResetScissor(vg);
	
	DragItem* drag = &sElemState->dragItem;
	
	if (this == (void*)drag->src) {
		Rect r = this->element.rect;
		
		r.y -= 8;
		r.h += 16;
		
		if (Rect_PointIntersect(&r, UnfoldVec2(SPLIT->cursorPos))) {
			Math_SmoothStepToF(&drag->colorLerp, 0.0f, 0.25f, 0.25f, 0.001f);
			
			for (int i = 0; i < list->num + 1; i++) {
				Rect r = Element_Container_GetDragRect(this, i);
				
				// if (i == this->detach.key)
				// continue;
				
				if (Rect_PointIntersect(&r, UnfoldVec2(SPLIT->cursorPos)) || list->num == i) {
					r.y += r.h / 2;
					r.y--;
					r.h = 2;
					
					Gfx_DrawRounderRect(vg, r, Theme_GetColor(THEME_PRIM, 255, 1.0f));
					break;
				}
			}
		} else
			Math_SmoothStepToF(&drag->colorLerp, 0.5f, 0.25f, 0.25f, 0.001f);
	}
	
	ScrollBar_Draw(&this->scroll, vg);
}

Set* Element_Container(ElContainer* this) {
	ScrollBar* scroll = &this->scroll;
	Arli* list = this->list;
	Input* input = NANO->input;
	Set* set = &this->set;
	
	memzero(set);
	osAssert(NANO && SPLIT);
	
	if (list) {
		if (this->controller)
			if (Input_GetCursor(input, CLICK_ANY)->press)
				if (!ELEM_PRESS_CONDITION(this))
					list->cur = -1;
		
		if (this->prevIndex != list->cur) {
			set->state = SET_EXTERN;
			set->index = list->cur;
			this->prevIndex = list->cur;
		}
	}
	
	if (this->element.type == ELEM_TYPE_NONE) {
		this->element.type = ELEM_TYPE_CONTAINER;
		
		if (this->align == 0)
			this->align = NVG_ALIGN_RIGHT;
		
		this->addButton.element.name = "+";
		this->addButton.align = NVG_ALIGN_CENTER;
		this->remButton.element.name = "-";
		this->remButton.align = NVG_ALIGN_CENTER;
		
		if (list)
			this->prevIndex = list->cur;
		else
			this->prevIndex = -16;
	}
	
	if (this->mutable) {
		this->addButton.element.rect = Rect_ShrinkX(this->element.rect, this->element.rect.w - SPLIT_TEXT_H);
		this->remButton.element.rect = Rect_ShrinkX(this->element.rect, this->element.rect.w - SPLIT_TEXT_H);
		this->element.rect = Rect_ShrinkX(this->element.rect, -(SPLIT_TEXT_H + SPLIT_ELEM_X_PADDING));
		
		this->remButton.element.rect = Rect_ShrinkY(this->remButton.element.rect, -(SPLIT_TEXT_H + SPLIT_ELEM_X_PADDING));
		this->addButton.element.rect.h = SPLIT_TEXT_H;
		this->remButton.element.rect.h = SPLIT_TEXT_H;
	}
	
	ScrollBar_Init(scroll, list ? list->num : 0, SPLIT_TEXT_H);
	Rect dispRect = Rect_Scale(this->element.rect, -SPLIT_ELEM_X_PADDING, -SPLIT_ELEM_X_PADDING);
	
	if (this->text) {
		if (&this->textBox == sElemState->textbox)
			goto queue_element;
		
		this->text = false;
		
		set->index = list->cur;
		set->state = SET_RENAME;
	}
	
	if (this->holdStretch) {
		Cursor_ForceCursor(CURSOR_ARROW_V);
		f32 dist = FLT_MAX;
		
		for (int i = 1; i < 16; i++) {
			f32 ndist;
			Vec2s p = {
				SPLIT->cursorPos.x,
				this->element.rect.y + (SPLIT_TEXT_H * clamp_min(i, 0) + SPLIT_ELEM_X_PADDING * 2)
			};
			
			if ((ndist = Vec2s_DistXZ(SPLIT->cursorPos, p)) < dist) {
				dist = ndist;
				this->element.slotNum = i;
			}
		}
		
		if (!Input_GetCursor(input, CLICK_L)->release) {
			ScrollBar_Update(scroll, NULL, SPLIT->cursorPos, dispRect, this->element.rect);
			this->element.disableTemp = 1;
			goto queue_element;
		}
		
		this->holdStretch = false;
	}
	
	osLog("scroll update");
	if (ScrollBar_Update(scroll, input, SPLIT->cursorPos, dispRect, this->element.rect))
		goto queue_element;
	
	if (this->list) {
		ELEMENT_QUEUE_CHECK();
		
		if (Input_GetCursor(input, CLICK_L)->release) {
			if (DragItem_Release(NANO, this)) {
				Rect r = this->element.rect;
				
				if (Rect_PointIntersect(&r, UnfoldVec2(SPLIT->cursorPos))) {
					s32 i = 0;
					s32 id = 0;
					
					for (; i < list->num; i++) {
						Rect r = Element_Container_GetDragRect(this, i);
						
						if (Rect_PointIntersect(&r, UnfoldVec2(SPLIT->cursorPos)))
							break;
						id++;
					}
					
					Arli_Insert(list, id, 1, list->copybuf);
					Arli_Set(list, id);
				}
			}
		}
		
		if (!ELEM_PRESS_CONDITION(this))
			goto queue_element;
		
		DragItem* drag = &sElemState->dragItem;
		int hoverKey = -1;
		Rect hoverRect;
		Rect stretchr = this->element.rect;
		
		stretchr.y = RectH(this->element.rect) - 6;
		stretchr.h = 6;
		
		if (this->stretch && Rect_PointIntersect(&stretchr, UnfoldVec2(SPLIT->cursorPos))) {
			if (Input_GetCursor(input, CLICK_L)->press)
				this->holdStretch = true;
			Cursor_ForceCursor(CURSOR_ARROW_V);
		} else {
			for (int i = 0; i < list->num; i++) {
				Rect r = Element_Container_GetListElemRect(this, i);
				
				if (Rect_PointIntersect(&r, UnfoldVec2(SPLIT->cursorPos))) {
					hoverKey = i;
					hoverRect = r;
				}
			}
		}
		
		if (Input_GetKey(input, KEY_UP)->press) {
			Arli_Set(list, list->cur - 1);
			Element_Container_NewInteract(this);
			ScrollBar_FocusSlot(&this->scroll, list->cur, true);
			
			goto queue_element;
		}
		
		if (Input_GetKey(input, KEY_DOWN)->press) {
			Arli_Set(list, list->cur + 1);
			Element_Container_NewInteract(this);
			ScrollBar_FocusSlot(&this->scroll, list->cur, true);
			
			goto queue_element;
		}
		
		if (Input_GetCursor(input, CLICK_L)->press) {
			this->pressKey = hoverKey;
			this->pressRect = hoverRect;
			
			if (hoverKey > -1) {
				Arli_Set(list, hoverKey);
				Element_Container_NewInteract(this);
			}
			
			goto queue_element;
		}
		
		if (Input_GetCursor(input, CLICK_L)->dual && this->renamable) {
			strncpy(this->textBox.txt, list->elemName(list, hoverKey), sizeof(this->textBox.txt));
			this->text = true;
			Element_SetActiveTextbox(NANO, SPLIT, &this->textBox);
			
			goto queue_element;
		}
		
		if (!this->mutable)
			goto queue_element;
		
		if (Input_GetKey(input, KEY_DELETE)->press)
			if (list->cur > -1 && list->cur < list->num)
				Arli_Remove(list, list->cur, 1);
		
		if (this->pressKey == -1)
			goto queue_element;
		if (drag->item)
			goto queue_element;
		
		if (Input_GetCursor(input, CLICK_L)->hold && input->cursor.dragDist > 8) {
			DragItem_Init(NANO, &this->element, this->pressRect, list->elemName(list, list->cur), this->list);
			
			if (Input_GetKey(input, KEY_LEFT_SHIFT)->hold)
				Arli_CopyToBuf(list, list->cur);
			else
				Arli_RemoveToBuf(list, list->cur);
		}
	}
	
	ELEMENT_QUEUE(Element_ContainerDraw);
	
	if (this->mutable) {
		Element_Condition(&this->addButton.element, list != NULL);
		Element_Condition(&this->remButton.element, list != NULL && list->num > 0 && list->cur > -1);
		
		if (Element_Button(&this->addButton)) {
			u8* temp = x_alloc(list->elemSize);
			int64_t index = list->cur > -1 ? list->cur + 1 : list->num;
			
			if (list->num == 0)
				index = 0;
			
			Arli_Insert(list, index, 1, temp);
			Arli_Set(list, index);
			set->index = index;
			set->state = SET_NEW;
		}
		
		if (Element_Button(&this->remButton)) {
			Arli_Remove(list, list->cur, 1);
			Arli_Set(list, list->cur);
		}
	}
	
	if (list) {
		if (this->prevIndex != list->cur) {
			if (set->state == SET_NONE) {
				set->state = SET_CHANGE;
				set->index = list->cur;
				
				if (set->index < 0)
					set->state = SET_UNSET;
			}
			
			this->prevIndex = list->cur;
		}
	}
	
	if (this->text) {
		this->textBox.element.rect = this->pressRect;
		Element_Textbox(&this->textBox);
	}
	
	return set;
}

////////////////////////////////////////////////////////////////////////////////

static void Element_SeparatorDraw(ElementQueCall* call) {
	Element* this = call->arg;
	void* vg = call->nano->vg;
	
	nvgBeginPath(vg);
	nvgStrokeWidth(vg, 1.0f);
	nvgFillColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 125, 1.0f));
	nvgMoveTo(vg, this->rect.x, this->rect.y);
	nvgLineTo(vg, this->rect.x + this->rect.w, this->rect.y);
	nvgFill(vg);
}

void Element_Separator(bool drawLine) {
	Element* this;
	
	osAssert(NANO && SPLIT);
	
	if (drawLine) {
		this = new(Element);
		
		sElemState->rowY += SPLIT_ELEM_X_PADDING;
		
		this->rect.x = SPLIT_ELEM_X_PADDING * 2 + sElemState->shiftX;
		this->rect.w = SPLIT->rect.w - SPLIT_ELEM_X_PADDING * 4 - sElemState->shiftX;
		this->rect.y = sElemState->rowY - SPLIT_ELEM_X_PADDING * 0.5;
		this->rect.h = 1;
		this->doFree = true;
		
		sElemState->rowY += SPLIT_ELEM_X_PADDING;
		
		ELEMENT_QUEUE(Element_SeparatorDraw);
	} else {
		sElemState->rowY += SPLIT_ELEM_X_PADDING * 2;
	}
}

////////////////////////////////////////////////////////////////////////////////

#include <ext_math.h>

static void Element_BoxDraw(ElementQueCall* call) {
	ElBox* this = call->arg;
	Rect r = this->element.rect;
	void* vg = NANO->vg;
	ElPanel* panel = this->panel;
	
	if (panel)
		r = panel->rect;
	
	this->element.shadow.a = 0.5f;
	
	Gfx_DrawRounderOutline(vg, r, this->element.constlight);
	Gfx_DrawRounderRect(vg, r, this->element.shadow);
	
	if (panel) {
		r.h = SPLIT_TEXT_H;
		if (panel->name)
			Gfx_Text(vg, r, NVG_ALIGN_CENTER, this->element.texcol, panel->name);
		
		Math_SmoothStepToS(
			&panel->yaw,
			panel->state ? DegToBin(90) : 0,
			4, DegToBin(25), 1);
		
		nvgSave(vg);
		
		f32 magic = SPLIT_ICON * 0.5f;
		
		nvgTranslate(vg, r.x + magic, r.y + magic);
		nvgRotate(vg, -BinToRad(panel->yaw));
		
		const Rect ir = {
			rint(-SPLIT_ICON / 2 + gPixelScale),
			rint(-SPLIT_ICON / 2),
			rint(SPLIT_ICON),
			rint(SPLIT_ICON)
		};
		
		Gfx_Icon(vg, ir, this->element.light, ICON_ARROW_D);
		nvgRestore(vg);
	}
}

static ElBox* BoxPush() {
	BoxContext* ctx = &sElemState->boxCtx;
	
	return ctx->list[ctx->index++] = new(ElBox);
}

static ElBox* BoxPop() {
	BoxContext* ctx = &sElemState->boxCtx;
	
	return ctx->list[--ctx->index];
}

int Element_Box(BoxState state, ...) {
	BoxContext* ctx = &sElemState->boxCtx;
	va_list va;
	
	va_start(va, state);
	ElPanel* panel = va_arg(va, ElPanel*);
	const char* name = va_arg(va, char*);
	va_end(va);
	
	if (state & BOX_INDEX)
		return ctx->index;
	
	if ((state & BOX_MASK_IO) == BOX_START) {
		ElBox* this = BoxPush();
		
		this->element.type = ELEM_TYPE_BOX;
		this->element.instantColor = true;
		this->element.doFree = true;
		this->rowY = sElemState->rowY;
		this->panel = panel;
		
		Element_Row(1, this, 1.0f);
		
		sElemState->rowY = this->rowY + SPLIT_ELEM_X_PADDING;
		sElemState->rowX += SPLIT_ELEM_X_PADDING;
		
		if (panel) {
			sElemState->rowY += SPLIT_TEXT_H;
			this->headRect = this->element.rect;
			this->headRect.h = SPLIT_TEXT_H;
			panel->name = name;
			panel->rect.x = this->element.rect.x;
			panel->rect.y = this->element.rect.y;
			panel->rect.w = this->element.rect.w;
		}
		
		Element_QueueElement(NANO, SPLIT, Element_BoxDraw, this, __FUNCTION__);
		
		return panel ? !panel->state : 1;
	}
	
	if ((state & BOX_MASK_IO) == BOX_END) {
		ElBox* this = BoxPop();
		
		if (panel && RectH(this->element.rect) == sElemState->rowY - SPLIT_ELEM_X_PADDING)
			sElemState->rowY -= SPLIT_ELEM_X_PADDING;
		
		if (!panel || (panel && !panel->state)) {
			this->element.rect.h = sElemState->rowY - this->rowY;
			sElemState->rowY += SPLIT_ELEM_X_PADDING;
			
			if (panel)
				panel->rect = this->element.rect;
		} else {
			Math_SmoothStepToI(&panel->rect.h, SPLIT_TEXT_H, 2, SPLIT_TEXT_H, 1);
			sElemState->rowY = RectH(panel->rect) + SPLIT_ELEM_X_PADDING;
		}
		
		if (panel) {
			this->element.rect.h = SPLIT_TEXT_H;
			
			if (!NANO->state.blockElemInput && ELEM_PRESS_CONDITION(this)) {
				Input* input = NANO->input;
				
				if (Split_CursorInRect(SPLIT, &this->element.rect))
					if (Input_SelectClick(input, CLICK_L))
						panel->state ^= 1;
			}
		}
		
		sElemState->rowX -= SPLIT_ELEM_X_PADDING;
	}
	
	return ctx->index;
}

////////////////////////////////////////////////////////////////////////////////

void Element_Slider_SetParams(ElSlider* this, f32 min, f32 max, char* type) {
	this->min = min;
	this->max = max;
	if (!stricmp(type, "int") || strstart(type, "i") || strstart(type, "s") || strstart(type, "u"))
		this->isInt = true;
	else if (!stricmp(type, "float") || strstart(type, "f"))
		this->isInt = false;
}

void Element_Slider_SetValue(ElSlider* this, f32 val) {
	val = clamp(val, this->min, this->max);
	this->vValue = this->value = normf(val, this->min, this->max);
}

f32 Element_Slider_GetValue(ElSlider* this) {
	if (this->isInt)
		return (s32)rint(lerpf(this->value, this->min, this->max));
	else
		return lerpf(this->value, this->min, this->max);
}

void Element_Button_SetProperties(ElButton* this, bool toggle, bool state) {
	this->element.toggle = toggle | (state == true ? 2 : 0);
	this->state = state;
}

void Element_Combo_SetArli(ElCombo* this, Arli* arlist) {
	this->list = arlist;
}

void Element_Color_SetColor(ElColor* this, void* color) {
	this->prop.rgb8 = color;
}

void Element_Container_SetArli(ElContainer* this, Arli* list, u32 num) {
	this->list = list;
	this->element.slotNum = num;
}

bool Element_Textbox_SetText(ElTextbox* this, const char* txt) {
	if (!this->doBlock && !this->modified) {
		strncpy(this->txt, txt, sizeof(this->txt));
		
		return 1;
	}
	
	return false;
}

void Element_SetName(Element* this, const char* name) {
	this->name = name;
}

void Element_SetIcon(Element* this, int icon) {
	this->iconId = icon;
}

Element* Element_Disable(Element* element) {
	element->disabled = true;
	
	return element;
}

Element* Element_Enable(Element* element) {
	element->disabled = false;
	
	return element;
}

Element* Element_Condition(Element* element, s32 condition) {
	element->disabled = !condition;
	
	return element;
}

static void Element_SetRectImpl(Rect* rect, f32 x, f32 y, f32 w, f32 hAdd) {
	rect->x = x;
	rect->y = y;
	rect->w = w;
	rect->h = SPLIT_TEXT_H + hAdd;
}

void Element_RowY(f32 y) {
	sElemState->rowY = y;
}

void Element_ShiftX(f32 x) {
	sElemState->shiftX = x;
}

void Element_SetNameLerp(Element* this, f32 lerp) {
	this->nameLerp = lerp;
}

static void Element_DisplayName(Element* this) {
	ElementQueCall* node;
	s32 w = this->rect.w * this->nameLerp;
	
	osAssert(NANO && SPLIT);
	
	this->posTxt.x = this->rect.x;
	this->posTxt.y = this->rect.y + SPLIT_TEXT_PADDING - 1;
	
	if (this->nameLerp > 0.0f && this->nameLerp < 1.0f) {
		this->rect.x += w + SPLIT_ELEM_X_PADDING * this->nameLerp;
		this->rect.w -= w + SPLIT_ELEM_X_PADDING * this->nameLerp;
	} else {
		void* vg = sElemState->nano->vg;
		f32 w = Gfx_TextWidth(vg, this->name);
		
		this->rect.x += w + SPLIT_ELEM_X_PADDING;
		this->rect.w -= w + SPLIT_ELEM_X_PADDING;
	}
	
	this->dispText = true;
	
	node = calloc(sizeof(ElementQueCall));
	Node_Add(sElemState->head, node);
	node->nano = NANO;
	node->split = SPLIT;
	node->func = Element_TextDraw;
	node->arg = this;
	node->elemFunc = "Element_DisplayName";
	node->update = false;
}

f32 ElAbs(int value) {
	sElemState->abs -= value + SPLIT_ELEM_X_PADDING;
	
	return -(value + SPLIT_ELEM_X_PADDING);
}

static void Element_HandleRowName(Element* this) {
	if (this->type != ELEM_TYPE_NONE && this->nameState == ELEM_NAME_STATE_INIT)
		this->nameState = ELEM_NAME_STATE_SHOW;
	
	if (this->nameState == ELEM_NAME_STATE_SHOW && this->name)
		Element_DisplayName(this);
}

void Element_Row(s32 rectNum, ...) {
	f32 shiftX = clamp_min(sElemState->shiftX, 0);
	f32 widthX = clamp_min(-sElemState->shiftX, 0);
	f32 x = SPLIT_ELEM_X_PADDING + sElemState->rowX + shiftX + (SPLIT->dummy ? SPLIT->rect.x : 0) - 1;
	f32 yadd = 0;
	f32 width;
	va_list va;
	int srw = SPLIT->rect.w + sElemState->abs;
	
	va_start(va, rectNum);
	for (int i = 0; i < rectNum; i++) {
		Element* this = va_arg(va, void*);
		f64 a = va_arg(va, f64);
		
		if (a >= 0)
			width = (f32)(((srw - widthX) - sElemState->rowX * 2 - shiftX) - SPLIT_ELEM_X_PADDING * 3) * a;
		else
			width = -a;
		
		if (this) {
			Rect* rect = &this->rect;
			
			if (this->slotNum && yadd == 0)
				yadd = SPLIT_TEXT_H * clamp_min(this->slotNum - 1, 0) + SPLIT_ELEM_X_PADDING * 2;
			
			if (rect) {
				Element_SetRectImpl(
					rect,
					x + SPLIT_ELEM_X_PADDING,
					rint(sElemState->rowY - SPLIT->scroll.voffset),
					width - SPLIT_ELEM_X_PADDING, yadd);
				
				Element_HandleRowName(this);
			}
		}
		
		x += width;
	}
	va_end(va);
	
	sElemState->abs = 0;
	sElemState->rowY += SPLIT_ELEM_Y_PADDING + yadd;
	
	if (sElemState->rowY >= SPLIT->rect.h) {
		SPLIT->scroll.enabled = true;
		SPLIT->scroll.max = sElemState->rowY - SPLIT->rect.h;
	} else
		SPLIT->scroll.enabled = false;
	
}

void Element_Header(s32 num, ...) {
	f32 x = SPLIT_ELEM_X_PADDING;
	va_list va;
	
	va_start(va, num);
	
	for (int i = 0; i < num; i++) {
		Element* this = va_arg(va, Element*);
		Rect* rect = &this->rect;
		s32 w = va_arg(va, s32) * gPixelScale;
		
		if (this) {
			this->header = true;
			
			if (rect) {
				Element_SetRectImpl(rect, x + SPLIT_ELEM_X_PADDING, (SPLIT_BAR_HEIGHT - SPLIT_TEXT_H) / 2, w, 0);
				Element_HandleRowName(this);
			}
		}
		x += w + SPLIT_ELEM_X_PADDING;
	}
	
	va_end(va);
}

int Element_Operatable(Element* this) {
	if (this->disabled || NANO->state.blockElemInput)
		return 0;
	
	if (!Element_PressCondition(NANO, SPLIT, this))
		return 0;
	
	return 1;
}

////////////////////////////////////////////////////////////////////////////////

static void Element_SetColor(Element* this, NVGcolor* source, NVGcolor target) {
	if (this->instantColor)
		*source = target;
	else
		Theme_SmoothStepToCol(source, target, 0.25f, 0.35f, 0.001f);
}

static void Element_UpdateElement(ElementQueCall* call) {
	Element* this = call->arg;
	NanoGrid* nano = call->nano;
	Split* split = call->split;
	Input* input = nano->input;
	f32 toggle = this->toggle == 3 ? 0.50f : 0.0f;
	f32 press = 1.0f;
	bool disabled = (this->disabled || this->disableTemp);
	
	this->disableTemp = false;
	this->hover = false;
	this->press = false;
	this->contextSet = false;
	
	if (Element_PressCondition(nano, split, this)) {
		this->hover = true;
		
		if (Input_GetCursor(input, CLICK_L)->hold)
			this->press = true;
	}
	
	if (this == (void*)sElemState->textbox || this == nano->contextMenu.element)
		this->hover = true;
	
	if (this->press && !disabled)
		press = 1.2f;
	
	int colPrim = this->colOvrdPrim ? this->colOvrdPrim : THEME_PRIM;
	int colShadow = this->colOvrdShadow ? this->colOvrdShadow : THEME_ELEMENT_DARK;
	int colBase = this->colOvrdBase ? this->colOvrdBase : THEME_ELEMENT_BASE;
	int colLight = this->colOvrdLight ? this->colOvrdLight : THEME_ELEMENT_LIGHT;
	int colTexcol = this->colOvrdTexcol ? this->colOvrdTexcol : THEME_TEXT;
	
	if (disabled) {
		const f32 mix = 0.5f;
		NVGcolor base = Theme_GetColor(THEME_ELEMENT_DARK, 255, 1.0f);
		
		this->prim = Theme_Mix(mix, base, Theme_GetColor(colPrim, 255, 1.00f * press));
		this->shadow = Theme_Mix(mix, base, Theme_GetColor(colShadow, 255, (1.00f + toggle) * press));
		this->light = Theme_Mix(mix, base, Theme_GetColor(colLight, 255, (0.50f + toggle) * press));
		this->constlight = Theme_Mix(mix, base, Theme_GetColor(colLight, 255, (0.50f + toggle)));
		this->texcol = Theme_Mix(mix, base, Theme_GetColor(colTexcol, 255, 1.00f * press));
		
		if (this->toggle < 3) this->base = Theme_Mix(mix, base, Theme_GetColor(colBase, 255, (1.00f + toggle) * press));
		else this->base = Theme_Mix(0.75, base, Theme_GetColor(colPrim, 255, 0.95f * press));
	} else {
		if (this->hover) {
			Element_SetColor(this, &this->prim, Theme_GetColor(colPrim, 255, 1.10f * press));
			Element_SetColor(this, &this->shadow, Theme_GetColor(colShadow, 255, (1.07f + toggle) * press));
			Element_SetColor(this, &this->light, Theme_GetColor(colLight, 255, (1.07f + toggle) * press));
			Element_SetColor(this, &this->texcol, Theme_GetColor(colTexcol, 255, 1.15f * press));
			
			if (this->toggle < 3)
				Theme_SmoothStepToCol(&this->base, Theme_GetColor(colBase, 255, (1.07f + toggle) * press), 0.25f, 0.35f, 0.001f);
		} else {
			Element_SetColor(this, &this->prim, Theme_GetColor(colPrim, 255, 1.00f * press));
			Element_SetColor(this, &this->shadow, Theme_GetColor(colShadow, 255, (1.00f + toggle) * press));
			Element_SetColor(this, &this->light, Theme_GetColor(colLight, 255, (0.50f + toggle) * press));
			Element_SetColor(this, &this->texcol, Theme_GetColor(colTexcol, 255, 1.00f * press));
			
			if (this->toggle < 3)
				Element_SetColor(this, &this->base, Theme_GetColor(colBase, 255, (1.00f + toggle) * press));
		}
		
		Element_SetColor(this, &this->constlight, Theme_GetColor(colLight, 255, (0.50f + toggle)));
		
		if (this->toggle == 3)
			Element_SetColor(this, &this->base, Theme_GetColor(colPrim, 255, 0.95f * press));
	}
	
	if (this->faulty || this->visFaultMix > EPSILON) {
		Math_SmoothStepToF(&this->visFaultMix, this->faulty ? 1.0f : 0.0f, 0.25f, 0.25f, 0.01f);
		
		this->light = Theme_Mix(this->visFaultMix, this->light, Theme_GetColor(THEME_DELETE, 255, 1.0f));
		this->constlight = Theme_Mix(this->visFaultMix, this->constlight, Theme_GetColor(THEME_DELETE, 255, 1.0f));
	}
}

static bool Element_DisableDraw(Element* element, Split* split) {
	if (element->rect.w < 1)
		return true;
	
	if (
		element->rect.x >= split->rect.w
		|| element->rect.x + element->rect.w < 0
		|| element->rect.y >= split->rect.h
		|| element->rect.y + element->rect.h < 0
	)
		return true;
	
	return false;
}

static void Element_DrawInstance(ElementQueCall* elem, Element* this) {
	elem->func(elem);
	
	if (this)
		this->dispText = false;
}

void Element_Draw(NanoGrid* nano, Split* split, bool header) {
	ElementQueCall* elem = sElemState->head;
	
	while (elem) {
		ElementQueCall* next = elem->next;
		Element* this = elem->arg;
		
		if (elem->split == nano->killSplit) {
			elem = next;
			continue;
		}
		
		if (this && this->header == header && elem->split == split) {
			osLog("ElemDraw%s: " PRNT_PRPL "%sDraw", header ? "Header" : "Split", elem->elemFunc);
			
			osLog("Update Element");
			if (elem->update)
				Element_UpdateElement(elem);
			
			osLog("Draw Instance");
			if (split->dummy || header || !Element_DisableDraw(elem->arg, elem->split))
				Element_DrawInstance(elem, this);
			
			osLog("Free");
			if (this->doFree)
				delete(elem->arg);
			
			osLog("Kill Node");
			Node_Kill(sElemState->head, elem);
		}
		
		elem = next;
	}
	
	osLog("ElemDraw%s: Done", header ? "Header" : "Split");
}

void Element_Flush(NanoGrid* nano) {
	while (sElemState->head) {
		Element* this = sElemState->head->arg;
		
		if (sElemState->head->split != nano->killSplit)
			if (this->doFree) delete(this);
		
		Node_Kill(sElemState->head, sElemState->head);
	}
	
	nano->killSplit = NULL;
}

void DragItem_Draw(NanoGrid* nano) {
	DragItem* this = &sElemState->dragItem;
	void* vg = nano->vg;
	Rect r = this->rect;
	Vec2f prevPos = this->pos;
	
	if (!this->item) return;
	
	if (this->hold) {
		Cursor* cursor = &nano->input->cursor;
		Vec2f target = Vec2f_New(UnfoldVec2(cursor->pos));
		f32 dist = Vec2f_DistXZ(this->pos, target);
		if (dist > EPSILON) {
			Vec2f seg = Vec2f_LineSegDir(this->pos, target);
			f32 pdist = dist;
			
			Math_SmoothStepToF(&dist, 0.0f, 0.25f, 80.0f, 0.1f);
			this->vel = Vec2f_MulVal(
				seg,
				pdist - dist
			);
			
		}
	}
	
	this->pos = Vec2f_Add(this->pos, this->vel);
	
	this->swing += (this->pos.x - prevPos.x) * 0.01f;
	
	this->swing = clamp(this->swing, -1.25f, 1.25f);
	Math_SmoothStepToF(&this->swing, 0.0f, 0.25f, 0.2f, 0.001f);
	Math_SmoothStepToF(&this->alpha, this->hold ? 200.0f : 0.0f, 0.25f, 80.0f, 0.01f);
	
	enum NVGalign align = NVG_ALIGN_CENTER;
	
	if (this->src && this->src->type == ELEM_TYPE_CONTAINER)
		align = ((ElContainer*)this->src)->align;
	
	nvgBeginFrame(vg, nano->wdim->x, nano->wdim->y, gPixelRatio); {
		NVGcolor outc = Theme_Mix(this->colorLerp, Theme_GetColor(THEME_ELEMENT_LIGHT, this->alpha, 1.0f), nvgHSLA(0, 1.0f, 0.5f, this->alpha));
		NVGcolor inc = Theme_Mix(this->colorLerp, Theme_GetColor(THEME_ELEMENT_BASE, this->alpha, 1.0f), nvgHSLA(0, 1.0f, 0.5f, this->alpha));
		
		if (this->copy) {
			outc = Theme_Mix(this->colorLerp + 0.5f, nvgHSLA(0.33f, 1.0f, 0.5f, this->alpha), outc);
			inc = Theme_Mix(this->colorLerp + 0.5f, nvgHSLA(0.33f, 1.0f, 0.5f, this->alpha), inc);
		}
		nvgSave(vg);
		
		f32 scale = lerpf(0.5f, this->alpha / 200.0f, 1.0f - (fabsf(this->swing) / 1.85f));
		nvgTranslate(vg,
			this->pos.x,
			this->pos.y);
		nvgScale(vg, scale, scale);
		nvgRotate(vg, this->swing);
		r.x -= this->anchor.x;
		r.y -= this->anchor.y;
		Gfx_DrawRounderOutline(vg, r, outc);
		Gfx_DrawRounderRect(vg, r, inc);
		Gfx_Text(
			vg, r, align,
			Theme_GetColor(THEME_TEXT, this->alpha, 1.0f),
			this->item
		);
		nvgRestore(vg);
	} nvgEndFrame(vg);
	
	if (this->alpha <= EPSILON && this->item) {
		this->alpha = 0.0f;
		delete(this->item);
	}
	
}

typedef struct osCtx {
	Arli msg;
	int  index;
} osCtx;

struct osMsg {
	char  msg[258];
	Timer timer;
	int   icon;
};

static osCtx __this;
static osCtx* this = &__this;

onlaunch_func_t osMsg_Init() {
	this->msg = Arli_New(struct osMsg);
}

onexit_func_t osMsg_Dest() {
	Arli_Free(&this->msg);
}

void osMsg_Draw(NanoGrid* nano) {
	Input* input = nano->input;
	void* vg = nano->vg;
	struct osMsg* msg = Arli_Head(&this->msg);
	struct osMsg* end = msg + this->msg.num;
	
	if (msg >= end)
		return;
	
	Rect r = {
		SPLIT_ELEM_X_PADDING,
		nano->wdim->y - (SPLIT_ELEM_X_PADDING + SPLIT_BAR_HEIGHT * 2),
		(SPLIT_TEXT * 32),
	};
	
	nvgBeginFrame(vg, nano->wdim->x, nano->wdim->y, gPixelRatio); {
		
		nvgSave(vg);
		nvgReset(vg);
		nvgFontSize(vg, SPLIT_TEXT);
		nvgFontFace(vg, "default");
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
		
		for (; msg < end; msg++) {
			Rectf32 bounds;
			
			nvgTextBoxBounds(vg,
				0, 0,
				r.w - SPLIT_ELEM_Y_PADDING * 2,
				msg->msg, NULL,
				(f32*)&bounds);
			
			r = Rect_ExpandY(r, (bounds.h + SPLIT_ELEM_Y_PADDING * 2));
			
			f32 alpha = remapf(TimerElapsed(&msg->timer), 4, 6, 1.0f, 0.0f);
			
			alpha = clamp(alpha, 0.0f, 1.0f);
			
			int shinrkX = r.w - SPLIT_ICON;
			int shinrkY = r.h - SPLIT_ICON;
			Rect cr = r;
			cr = Rect_ShrinkX(cr, shinrkX);
			cr = Rect_ShrinkY(cr, shinrkY);
			
			cr.x -= SPLIT_TEXT_PADDING;
			cr.y += SPLIT_TEXT_PADDING;
			
			if (Rect_PointIntersect(&r, UnfoldVec2(input->cursor.pos))) {
				alpha *= 1.22f;
				msg->timer = TimerSet(0);
				
				if (Rect_PointIntersect(&cr, UnfoldVec2(input->cursor.pos)))
					if (Input_SelectClick(input, CLICK_L))
						alpha = 0; // KILL
			}
			
			int colorID = THEME_ELEMENT_LIGHT;
			
			switch (msg->icon) {
				case ICON_INFO:
					colorID = THEME_PRIM;
					break;
					
				case ICON_WARNING:
					colorID = THEME_WARNING;
					break;
					
				case ICON_ERROR:
					colorID = THEME_DELETE;
					break;
			}
			
			NVGcolor base = Theme_GetColor(THEME_ELEMENT_BASE, 200 * alpha, 1.25f);
			NVGcolor dark = Theme_GetColor(THEME_ELEMENT_DARK, 200 * alpha, 1.0f);
			NVGcolor text = Theme_GetColor(colorID, 200 * alpha, 1.0f);
			
			Gfx_DrawRounderOutline(vg, r, base);
			Gfx_DrawRounderRect(vg, r, dark);
			nvgFillColor(vg, text);
			nvgTextBox(vg,
				r.x + SPLIT_ELEM_Y_PADDING,
				r.y + SPLIT_ELEM_Y_PADDING,
				r.w - SPLIT_ELEM_Y_PADDING * 2,
				msg->msg, 0);
			
			if (msg->icon) {
				Rect ir = r;
				
				ir.x += SPLIT_TEXT_PADDING;
				ir.y = r.y + r.h * 0.5f - SPLIT_ICON * 0.5f;
				ir.h = SPLIT_ICON;
				ir.w = SPLIT_ICON;
				
				Gfx_Icon(vg, ir, text, msg->icon);
			}
			
			Gfx_DrawRounderRect(vg, cr, base);
			Gfx_Icon(vg, cr, text, ICON_CLOSE);
			
			r = Rect_ShrinkY(r, r.h);
			r.y -= SPLIT_ELEM_X_PADDING;
			
			if (alpha == 0.0f) {
				Arli_Remove(&this->msg, Arli_IndexOf(&this->msg, msg), 1);
				msg--;
				end--;
			}
		}
		
		nvgRestore(vg);
	} nvgEndFrame(vg);
	
}

void osMsg(int icon, const char* fmt, ...) {
	struct osMsg* msg = Arli_Insert(&this->msg, 0, 1, NULL);
	va_list va;
	
	va_start(va, fmt);
	xl_vsnprintf(msg->msg, 256, fmt, va);
	va_end(va);
	
	info("%s", msg->msg);
	
	msg->timer = TimerSet(0);
	msg->icon = icon;
}
