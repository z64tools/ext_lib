#include <ext_interface.h>
#include <nano_grid.h>
#include <ext_theme.h>

////////////////////////////////////////////////////////////////////////////////

typedef struct {
	rgba8_t* c;
	s32      id;
} ImgMap;

static struct {
	ImgMap imgLumSat;
	ImgMap imgHue;
} sColorContext;

static void ContextProp_Color_Init(NanoGrid* nano, ContextMenu* this) {
	PropColor* prop = this->udata;
	
	this->rect.h = this->rect.w = Max(this->rect.w, 128 + 64);
	this->temp = new(ElTextbox);
	
	if (prop->rgb8) {
		hsl_t hsl = ColorHSL(UnfoldRGB(*prop->rgb8));
		
		prop->hue = hsl.h;
		prop->pos.x = hsl.s;
		prop->pos.y = 1.0f - hsl.l;
		
		rgb8_t rgb = ColorRGB8(UnfoldHSL(hsl));
		
		if (memcmp(prop->rgb8, &rgb, sizeof(rgb8_t))) {
			warn("%d %d %d\a\n%d %d %d", UnfoldRGB(*prop->rgb8), UnfoldRGB(rgb));
		}
	}
}

static void ContextProp_Color_Draw(NanoGrid* nano, ContextMenu* this) {
	PropColor* prop = this->udata;
	void* vg = nano->vg;
	Rect r = this->rect;
	Input* input = nano->input;
	Cursor* cursor = &nano->input->cursor;
	
	r.x += 2;
	r.y += 2;
	r.w -= 4;
	r.h -= 4;
	
	Rect rectLumSat = r;
	Rect rectHue;
	hsl_t color;
	
	rectLumSat.h -= SPLIT_ELEM_Y_PADDING + SPLIT_ELEM_X_PADDING + SPLIT_ELEM_Y_PADDING + SPLIT_ELEM_X_PADDING;
	rectHue = rectLumSat;
	rectHue.y += rectHue.h + SPLIT_ELEM_X_PADDING;
	rectHue.h = SPLIT_ELEM_Y_PADDING;
	
	if (this->state.init) {
		color = ColorHSL(UnfoldRGB(*prop->rgb8));
		
		prop->hue = color.h;
		prop->pos.x = color.s;
		prop->pos.y = invertf(color.l);
	} else {
		if ((
				Rect_PointIntersect(&rectLumSat, cursor->pos.x, cursor->pos.y) &&
				Input_GetCursor(input, CLICK_L)->press
			) || ( prop->holdLumSat )) {
			Vec2s relPos = Vec2s_Sub(cursor->pos, (Vec2s) { rectLumSat.x, rectLumSat.y });
			
			prop->pos.x = clamp((f32)relPos.x / rectLumSat.w, 0, 1);
			prop->pos.y = clamp((f32)relPos.y / rectLumSat.h, 0, 1);
			prop->holdLumSat =  Input_GetCursor(input, CLICK_L)->hold;
		}
		if ((
				Rect_PointIntersect(&rectHue, cursor->pos.x, cursor->pos.y) &&
				Input_GetCursor(input, CLICK_L)->press
			) || ( prop->holdHue )) {
			Vec2s relPos = Vec2s_Sub(cursor->pos, (Vec2s) { rectHue.x, rectHue.y });
			
			prop->hue = clamp((f32)relPos.x / rectHue.w, 0, 1);
			prop->holdHue =  Input_GetCursor(input, CLICK_L)->hold;
		}
	}
	
	color = (hsl_t) { prop->hue, prop->pos.x, invertf(prop->pos.y) };
	*prop->rgb8 = ColorRGB8(UnfoldHSL(color));
	
	nested(void, UpdateImg, ()) {
		for (s32 y = 0; y < rectLumSat.h; y++) {
			for (s32 x = 0; x < rectLumSat.w; x++) {
				sColorContext.imgLumSat.c[(y * rectLumSat.w) + x] = ColorRGBA8(
					color.h,
					(f32)x / rectLumSat.w,
					(f32)y / rectLumSat.h
				);
			}
		}
		
		nvgUpdateImage(vg, sColorContext.imgLumSat.id, (void*)sColorContext.imgLumSat.c);
	};
	
	if (!sColorContext.imgLumSat.c) {
		sColorContext.imgLumSat.c = new(rgba8_t[rectLumSat.w * rectLumSat.h]);
		UpdateImg();
		sColorContext.imgLumSat.id = nvgCreateImageRGBA(vg, rectLumSat.w, rectLumSat.h, NVG_IMAGE_NEAREST | NVG_IMAGE_FLIPY, (void*)sColorContext.imgLumSat.c);
	}
	
	UpdateImg();
	
	Vec2f pos = Vec2f_Mul(prop->pos, Vec2f_New(rectLumSat.w, rectLumSat.h));
	pos = Vec2f_Add(pos, Vec2f_New(rectLumSat.x, rectLumSat.y));
	
	nvgBeginPath(vg);
	nvgFillPaint(vg, nvgImagePattern(vg, UnfoldRect(rectLumSat), 0, sColorContext.imgLumSat.id, 1.0f));
	nvgRoundedRect(vg, UnfoldRect(rectLumSat), SPLIT_ROUND_R * 2);
	nvgFill(vg);
	
	for (int i = 0; i < 2; i++) {
		nvgBeginPath(vg);
		if (!i) nvgFillColor(vg, Theme_GetColor(THEME_SHADOW, 255, 1.0f));
		else nvgFillColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 255, 1.0f));
		nvgCircle(vg, pos.x, pos.y, 5 - i);
		nvgFill(vg);
	}
	
	if (!sColorContext.imgHue.c) {
		sColorContext.imgHue.c = qxf(new(rgba8_t[rectHue.w * rectHue.h]));
		
		for (s32 y = 0; y < rectHue.h; y++) {
			for (s32 x = 0; x < rectHue.w; x++) {
				sColorContext.imgHue.c[(y * rectHue.w) + x] = ColorRGBA8(
					(f32)x / rectHue.w,
					1.0f,
					0.5f
				);
			}
		}
		
		sColorContext.imgHue.id = nvgCreateImageRGBA(vg, rectHue.w, rectHue.h, NVG_IMAGE_NEAREST | NVG_IMAGE_FLIPY, (void*)sColorContext.imgHue.c);
	}
	
	nvgBeginPath(vg);
	nvgFillPaint(vg, nvgImagePattern(vg, UnfoldRect(rectHue), 0, sColorContext.imgHue.id, 0.9f));
	nvgRoundedRect(vg, UnfoldRect(rectHue), SPLIT_ROUND_R);
	nvgFill(vg);
	
	{
		Rect r = rectHue;
		
		r.x += r.w * color.h - 1;
		r.w = 2;
		Gfx_DrawRounderRect(vg, r, nvgHSL(color.h, 0.8f, 0.8f));
		Gfx_DrawRounderOutline(vg, r, Theme_GetColor(THEME_SHADOW, 255, 1.0f));
	}
	
	DummyGrid_Push(nano);
	DummySplit_Push(nano, &this->split, nano->workRect);
	Element_Textbox(this->temp);
	DummySplit_Pop(nano, &this->split);
	DummyGrid_Pop(nano);
}

////////////////////////////////////////////////////////////////////////////////

static void ContextProp_Arli_Init(NanoGrid* nano, ContextMenu* this) {
	Arli* list = this->udata;
	ElCombo* combo = (this->element && this->element->type == ELEM_TYPE_COMBO) ? (void*)this->element : NULL;
	
	this->rect.h = 0;
	this->rect.w = 0;
	this->visualKey = list->cur;
	
	info("" PRNT_YELW "%s", __FUNCTION__);
	info("list->cur = %d", list->cur);
	
	for (int i = 0; i < list->num; i++) {
		this->rect.w = Max(this->rect.w, Gfx_TextWidth(nano->vg, list->elemName(list, i)));
		this->rect.h += SPLIT_TEXT_H;
	}
	
	this->rect.w += SPLIT_ELEM_X_PADDING * 8;
	this->rect.h += SPLIT_ELEM_X_PADDING * 2;
	
	ScrollBar_Init(&this->scroll, list->num, SPLIT_TEXT_H);
	
	if (combo && combo->controller) {
		combo->prevIndex = -1;
		this->visualKey = -1;
		this->state.rectClamp = false;
	}
	
	this->quickSelect = new(int[list->num]);
	
	for (int i = 0; i < list->num; i++) {
		const char* s = list->elemName(list, i);
		const char* p = strchr(s, '#');
		
		if (p) this->quickSelect[i] = tolower(p[1]);
		else this->quickSelect[i] = -1;
	}
}

static void ContextProp_Arli_Draw(NanoGrid* nano, ContextMenu* this) {
	Rect scrollRect = Rect_Scale(this->rect, -SPLIT_ELEM_X_PADDING, -SPLIT_ELEM_X_PADDING);
	Input* input = nano->input;
	Arli* list = this->udata;
	void* vg = nano->vg;
	bool hold = ScrollBar_Update(&this->scroll, input, input->cursor.pos, scrollRect, this->rect);
	ElCombo* combo = (this->element && this->element->type == ELEM_TYPE_COMBO) ? (void*)this->element : NULL;
	NVGcolor highlight;
	
	if (combo && combo->controller) {
		this->visualKey = -1;
		highlight = Theme_Mix(0.25f, Theme_GetColor(THEME_ELEMENT_BASE, 255, 1.0f), Theme_GetColor(THEME_ELEMENT_LIGHT, 255, 1.0f));
	} else
		highlight = Theme_GetColor(THEME_PRIM, 255, 1.0f);
	
	nvgScissor(vg, UnfoldRect(this->rect));
	
	for (int i = 0; i < list->num; i++) {
		Rect r = ScrollBar_GetRect(&this->scroll, i);
		
		if (!Rect_RectIntersect(r, this->rect))
			continue;
		
		if (Rect_PointIntersect(&this->rect, UnfoldVec2(input->cursor.pos))) {
			if (!hold && Rect_PointIntersect(&r, UnfoldVec2(input->cursor.pos))) {
				this->visualKey = i;
				
				if (Input_SelectClick(input, CLICK_L))
					this->state.setCondition = true;
			}
		}
		
		if (this->visualKey == i) {
			Gfx_DrawRounderRect(vg, r, highlight);
			Gfx_TextShadow(vg);
		}
		
		Gfx_Text(vg, r, NVG_ALIGN_MIDDLE | NVG_ALIGN_LEFT, Theme_GetColor(THEME_TEXT, 255, 1.0f), list->elemName(list, i));
	}
	
	char* p = (void*)-1UL;
	
	for (int i = 0; i < list->num; i++) {
		char* n = strchr(input->buffer, this->quickSelect[i]);
		
		if (n && n < p) {
			p = n;
			this->visualKey = i;
			this->state.setCondition = true;
		}
	}
	
	nvgResetScissor(vg);
	
	ScrollBar_Draw(&this->scroll, vg);
}

////////////////////////////////////////////////////////////////////////////////

#define FUNC_INIT 0
#define FUNC_DRAW 1
#define FUNC_DEST 2

void (*sContextMenuFuncs[][3])(NanoGrid*, ContextMenu*) = {
	[CONTEXT_PROP_COLOR] =   { ContextProp_Color_Init, ContextProp_Color_Draw },
	[CONTEXT_ARLI] =         { ContextProp_Arli_Init,  ContextProp_Arli_Draw  },
	[CONTEXT_CUSTOM] =       {},
};

static void ContextMenu_ClampRect(NanoGrid* nano, ContextMenu* this) {
	Rect cr = {
		1,
		SPLIT_BAR_HEIGHT + 1,
		nano->wdim->x - 2,
		nano->wdim->y - (SPLIT_BAR_HEIGHT + 1) * 2
	};
	
	if (this->state.rectClamp)
		this->rect = Rect_Clamp(this->rect, cr);
}

void ContextMenu_Init(NanoGrid* nano, void* uprop, void* element, ContextDataType type, Rect rect) {
	ContextMenu* this = &nano->contextMenu;
	
	osLog("context init");
	osAssert(uprop != NULL);
	osAssert(type < ArrCount(sContextMenuFuncs));
	
	this->magic = 'ctxt';
	this->element = element;
	this->udata = uprop;
	this->type = type;
	this->rect = this->rectOrigin = rect;
	this->pos = nano->input->cursor.pressPos;
	
	nano->state.blockElemInput++;
	nano->state.blockSplitting++;
	this->pos = nano->input->cursor.pos;
	this->state.up = -1;
	this->state.side = 1;
	this->state.init = true;
	this->state.widthAdjustment = true;
	this->state.offsetOriginRect = true;
	this->state.distanceCheck = true;
	this->visualKey = -1;
	this->state.rectClamp = true;
	
	osLog("type %d init func", type);
	sContextMenuFuncs[type][FUNC_INIT](nano, this);
	
	if (this->state.offsetOriginRect) {
		this->rect.y = this->rectOrigin.y + this->rectOrigin.h;
		this->rect.x = this->rectOrigin.x;
	}
	
	if (this->state.widthAdjustment)
		this->rect.w = Max(this->rect.w, this->rectOrigin.w);
	
	if (this->pos.y > nano->wdim->y * 0.5) {
		this->rect.y -= this->rect.h + this->rectOrigin.h;
		this->state.up = 1;
	}
	
	if (this->pos.x > nano->wdim->x * 0.5f) {
		this->rect.x -= this->rect.w - this->rectOrigin.w;
		this->state.side = -1;
	}
	
	if (this->state.maximize) {
		this->rect = Rect_New(0, 0, UnfoldVec2(*nano->wdim));
	}
	
	ContextMenu_ClampRect(nano, this);
	
	osLog("ok", type);
}

void ContextMenu_Custom(NanoGrid* nano, void* context, void* element, void init(NanoGrid*, ContextMenu*), void draw(NanoGrid*, ContextMenu*), void dest(NanoGrid*, ContextMenu*), Rect rect) {
	sContextMenuFuncs[CONTEXT_CUSTOM][0] = init;
	sContextMenuFuncs[CONTEXT_CUSTOM][1] = draw;
	sContextMenuFuncs[CONTEXT_CUSTOM][2] = dest;
	ContextMenu_Init(nano, context, element, CONTEXT_CUSTOM, rect);
}

void ContextMenu_InitSub(NanoGrid* nano, u8 index, Arli* list) {
	ContextMenu* this = &nano->contextMenu;
	
	if (!this->subMenu)
		this->subMenu = new(Arli*[index + 1]);
	else if (index >= this->numSubMenu)
		renew(this->subMenu, Arli*[index + 1]);
	
	this->numSubMenu = index + 1;
	this->subMenu[index] = list;
}

void ContextMenu_Draw(NanoGrid* nano, ContextMenu* this) {
	Cursor* cursor = &nano->input->cursor;
	void* vg = nano->vg;
	
	if (this->udata == NULL)
		return;
	
	if (this->state.init == true) {
		this->state.init = false;
		
		return;
	}
	
	if (this->state.maximize) {
		this->rect = Rect_New(0, 0, UnfoldVec2(*nano->wdim));
		ContextMenu_ClampRect(nano, this);
	}
	
	nvgBeginFrame(nano->vg, nano->wdim->x, nano->wdim->y, gPixelRatio); {
		Rect r = this->rect;
		Gfx_DrawRounderRect(vg, r, Theme_GetColor(THEME_ELEMENT_DARK, 255, 1.0f));
		sContextMenuFuncs[this->type][FUNC_DRAW](nano, this);
		Gfx_DrawRounderOutline(vg, r, Theme_GetColor(THEME_ELEMENT_BASE, 255, 1.5f));
	} nvgEndFrame(nano->vg);
	
	if (Rect_PointDistance(&this->rect, cursor->pos.x, cursor->pos.y) > 0 && Input_GetCursor(nano->input, CLICK_ANY)->press) {
		ContextMenu_Close(nano);
		
		return;
	}
	
	bool terminate0 = Rect_PointDistance(&this->rect, cursor->pos.x, cursor->pos.y) > 0 && Input_GetCursor(nano->input, CLICK_ANY)->press;
	bool terminate1 = this->state.distanceCheck && Rect_PointDistance(&this->rect, cursor->pos.x, cursor->pos.y) > 128 * gPixelScale;
	
	if (this->state.setCondition || terminate0 || terminate1)
		ContextMenu_Close(nano);
}

void ContextMenu_Close(NanoGrid* nano) {
	ContextMenu* this = &nano->contextMenu;
	
	if (!this->magic)
		return;
	
	if (this->state.setCondition > 0) {
		if (this->element)
			this->element->contextSet = true;
		
		switch (this->type) {
			case CONTEXT_PROP_COLOR: (void)0;
				break;
			case CONTEXT_ARLI: (void)0;
				Arli_Set(this->udata, this->visualKey);
				break;
			default:
				break;
		}
	} else {
		if (this->type == CONTEXT_ARLI) {
			ElCombo* combo = (this->element && this->element->type == ELEM_TYPE_COMBO) ? (void*)this->element : NULL;
			
			if (combo && combo->controller)
				combo->prevIndex = combo->list->cur;
		}
	}
	
	if (this->type == CONTEXT_CUSTOM)
		if (sContextMenuFuncs[CONTEXT_CUSTOM][FUNC_DEST])
			sContextMenuFuncs[CONTEXT_CUSTOM][FUNC_DEST](nano, this);
	
	if (sColorContext.imgHue.c) {
		nvgDeleteImage(nano->vg, sColorContext.imgHue.id);
		delete(sColorContext.imgHue.c);
	}
	
	if (sColorContext.imgLumSat.c) {
		nvgDeleteImage(nano->vg, sColorContext.imgLumSat.id);
		delete(sColorContext.imgLumSat.c);
	}
	
	delete(this->quickSelect, this->subMenu);
	
	nano->state.blockElemInput--;
	nano->state.blockSplitting--;
	
	memset(this, 0, sizeof(*this));
}
