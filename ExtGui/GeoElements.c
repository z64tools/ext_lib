#include "GeoGrid.h"
#include "Interface.h"

#undef Element_Row
#undef Element_Disable
#undef Element_Enable
#undef Element_Condition
#undef Element_Name
#undef Element_DisplayName
#undef Element_Header

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
	u32 update : 1;
} ElementCallInfo;

/* ───────────────────────────────────────────────────────────────────────── */

ThreadLocal struct {
	ElementCallInfo* head;
	
	ElTextbox* curTextbox;
	Split*   curSplitTextbox;
	s32      posText;
	s32      posSel;
	char*    storeA;
	s32      ctrlA;
	
	s32      flickTimer;
	s32      flickFlag;
	
	s16      breathYaw;
	f32      breath;
	
	GeoGrid* geo;
	Split*   split;
	
	f32      rowY;
	f32      rowX;
	
	s32      timerTextbox;
	s32      blockerTextbox;
	
	s32      pushToHeader : 1;
} gElementState = {
	.posSel = -1,
	.flickFlag = 1,
};

static const char* sFmt[] = {
	"%.3f",
	"%d"
};

// # # # # # # # # # # # # # # # # # # # #
// #                                     #
// # # # # # # # # # # # # # # # # # # # #

void Gfx_Shape(void* vg, Vec2f center, f32 scale, s16 rot, Vec2f* p, u32 num) {
	for (s32 i = 0; i < num; i++) {
		s32 wi = WrapS(i, 0, num - 1);
		Vec2f zero = { 0 };
		Vec2f pos = {
			p[wi].x * scale,
			p[wi].y * scale,
		};
		f32 dist = Math_Vec2f_DistXZ(zero, pos);
		s16 yaw = Math_Vec2f_Yaw(zero, pos);
		
		pos.x = center.x + SinS((s32)(yaw + rot)) * dist;
		pos.y = center.y + CosS((s32)(yaw + rot)) * dist;
		
		if ( i == 0 )
			nvgMoveTo(vg, pos.x, pos.y);
		else
			nvgLineTo(vg, pos.x, pos.y);
	}
}

void Gfx_DrawRounderOutline(void* vg, Rect rect, NVGcolor color) {
	nvgBeginPath(vg);
	nvgFillColor(vg, color);
	nvgRoundedRect(
		vg,
		rect.x - 1,
		rect.y - 1,
		rect.w + 2,
		rect.h + 2,
		SPLIT_ROUND_R
	);
	nvgRoundedRect(
		vg,
		rect.x,
		rect.y,
		rect.w,
		rect.h,
		SPLIT_ROUND_R
	);
	
	nvgPathWinding(vg, NVG_HOLE);
	nvgFill(vg);
}

void Gfx_DrawRounderRect(void* vg, Rect rect, NVGcolor color) {
	nvgBeginPath(vg);
	nvgFillColor(vg, color);
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

// # # # # # # # # # # # # # # # # # # # #
// #                                     #
// # # # # # # # # # # # # # # # # # # # #

static void Element_QueueElement(GeoGrid* geo, Split* split, ElementFunc func, void* arg, const char* elemFunc) {
	ElementCallInfo* node;
	
	Calloc(node, sizeof(ElementCallInfo));
	Node_Add(gElementState.head, node);
	node->geo = geo;
	node->split = split;
	node->func = func;
	node->arg = arg;
	node->update = true;
	
	node->elemFunc = elemFunc;
}

#define Element_QueueElement(geo, split, func, arg) Element_QueueElement(geo, split, func, arg, __FUNCTION__)

static s32 Element_PressCondition(GeoGrid* geo, Split* split, Element* this) {
	if (!geo->state.noClickInput &
		(split->mouseInSplit || this->header) &&
		!split->blockMouse &&
		!split->elemBlockMouse) {
		if (this->header) {
			Rect r = Rect_AddPos(this->rect, split->headRect);
			
			return Rect_PointIntersect(&r, geo->input->mouse.pos.x, geo->input->mouse.pos.y);
		} else {
			return Split_CursorInRect(split, &this->rect);
		}
	}
	
	return false;
}

static void Element_Slider_SetCursorToVal(GeoGrid* geo, Split* split, ElSlider* this) {
	f32 x = split->rect.x + this->element.rect.x + this->element.rect.w * this->value;
	f32 y = split->rect.y + this->element.rect.y + this->element.rect.h * 0.5;
	
	Input_SetMousePos(geo->input, x, y);
}

static void Element_Slider_SetTextbox(Split* split, ElSlider* this) {
	gElementState.curTextbox = &this->textBox;
	gElementState.curSplitTextbox = split;
	gElementState.ctrlA = 1;
	
	this->isTextbox = true;
	
	this->textBox.isNumBox = true;
	this->textBox.element.rect = this->element.rect;
	this->textBox.align = ALIGN_CENTER;
	this->textBox.size = 32;
	
	this->textBox.nbx.isInt = this->isInt;
	this->textBox.nbx.max = this->max;
	this->textBox.nbx.min = this->min;
	
	this->textBox.isHintText = 2;
	gElementState.posText = 0;
	gElementState.posSel = strlen(this->textBox.txt);
}

static bool Element_DisableDraw(Element* element, Split* split) {
	if (element->rect.w < 1)
		return true;
	if (element->rect.x >= split->rect.w || element->rect.y >= split->rect.h)
		return true;
	
	return false;
}

static f32 Element_TextWidth(void* vg, const char* txt) {
	f32 bounds[4];
	
	nvgFontFace(vg, "dejavu");
	nvgFontSize(vg, SPLIT_TEXT);
	nvgFontBlur(vg, 0.0);
	nvgTextBounds(vg, 0, 0, txt, 0, bounds);
	
	return bounds[2];
}

// # # # # # # # # # # # # # # # # # # # #
// # PropEnum                            #
// # # # # # # # # # # # # # # # # # # # #

#undef PropEnum_InitList

static char* PropEnum_Get(PropEnum* this, s32 i) {
	char** list = this->list;
	
	if (i >= this->num) {
		printf_info("" PRNT_YELW "%s", __FUNCTION__);
		printf_warning("OOB Access %d / %d", i, this->num);
		
		return NULL;
	}
	
	return list[i];
}

static void PropEnum_Set(PropEnum* this, s32 i) {
	this->key = Clamp(i, 0, this->num - 1);
	
	if (i != this->key)
		Log("Out of range set!");
}

PropEnum* PropEnum_Init(s32 defaultVal) {
	PropEnum* this = SysCalloc(sizeof(PropEnum));
	
	this->get = PropEnum_Get;
	this->set = PropEnum_Set;
	this->key = defaultVal;
	
	return this;
}

PropEnum* PropEnum_InitList(s32 def, s32 num, ...) {
	PropEnum* prop = PropEnum_Init(def);
	va_list va;
	
	va_start(va, num);
	
	for (s32 i = 0; i < num; i++)
		PropEnum_Add(prop, va_arg(va, char*));
	
	va_end(va);
	
	return prop;
}

void PropEnum_Add(PropEnum* this, char* item) {
	Realloc(this->list, sizeof(char*) * (this->num + 1));
	this->list[this->num++] = StrDup(item);
}

void PropEnum_Remove(PropEnum* this, s32 key) {
	Assert(this->list);
	
	this->num--;
	Free(this->list[key]);
	for (s32 i = key; i < this->num; i++)
		this->list[i] = this->list[i + 1];
}

void PropEnum_Free(PropEnum* this) {
	for (s32 i = 0; i < this->num; i++)
		Free(this->list[i]);
	Free(this->list);
	Free(this);
}

// # # # # # # # # # # # # # # # # # # # #
// # DropMenu                            #
// # # # # # # # # # # # # # # # # # # # #

static void DropMenu_Init_PropEnum(GeoGrid* geo, DropMenu* this) {
	PropEnum* prop = this->prop;
	s32 height;
	
	geo->state.noClickInput++;
	geo->state.noSplit++;
	this->pos = geo->input->mouse.pos;
	this->key = -1;
	this->key = prop->key;
	this->state.up = -1;
	
	height = SPLIT_ELEM_X_PADDING * 2 + SPLIT_TEXT_H * prop->num;
	
	if (this->rectOrigin.w && this->rectOrigin.h) {
		this->rect.y = this->rectOrigin.y + this->rectOrigin.h;
		this->rect.h = height;
		
		this->rect.x = this->rectOrigin.x;
		this->rect.w = this->rectOrigin.w;
		
		if (this->pos.y > geo->winDim->y * 0.5) {
			this->rect.y -= this->rect.h + this->rectOrigin.h;
			this->state.up = 1;
		}
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
};

void (*sDropMenuInit[])(GeoGrid*, DropMenu*) = {
	[PROP_ENUM] = DropMenu_Init_PropEnum,
};

void DropMenu_Init(GeoGrid* geo, void* uprop, PropType type, Rect rect) {
	DropMenu* this = &geo->dropMenu;
	
	Assert(uprop != NULL);
	Assert(type < ArrayCount(sDropMenuInit));
	
	this->prop = uprop;
	this->type = type;
	this->rectOrigin = rect;
	this->pos = geo->input->mouse.pressPos;
	
	sDropMenuInit[type](geo, this);
}

void DropMenu_Close(GeoGrid* geo) {
	DropMenu* this = &geo->dropMenu;
	
	geo->state.noClickInput--;
	geo->state.noSplit--;
	
	memset(this, 0, sizeof(*this));
}

static void DropMenu_Draw_Enum(GeoGrid* geo, DropMenu* this) {
	PropEnum* prop = this->prop;
	f32 height = SPLIT_ELEM_X_PADDING;
	MouseInput* mouse = &geo->input->mouse;
	void* vg = geo->vg;
	Rect r = this->rect;
	
	this->state.setCondition = false;
	
	Gfx_DrawRounderOutline(vg, this->rect, Theme_GetColor(THEME_ELEMENT_LIGHT, 215, 1.0f));
	
	r.y += this->state.up;
	r.h++;
	Gfx_DrawRounderRect(vg, r, Theme_GetColor(THEME_ELEMENT_DARK, 215, 1.0f));
	
	for (s32 i = 0; i < prop->num; i++) {
		r = this->rect;
		
		r.y = this->rect.y + height;
		r.h = SPLIT_TEXT_H;
		
		if (Rect_PointIntersect(&r, mouse->pos.x, mouse->pos.y) && prop->get(prop, i)) {
			this->key = i;
			
			this->state.setCondition = true;
		}
		
		if (i == this->key) {
			r.x += 4;
			r.w -= 8;
			Gfx_DrawRounderRect(vg, r, Theme_GetColor(THEME_PRIM, 215, 1.0f));
		}
		
		if (prop->get(prop, i)) {
			nvgScissor(vg, UnfoldRect(r));
			nvgFillColor(vg, Theme_GetColor(THEME_TEXT, 255, 1.0f));
			nvgFontFace(vg, "dejavu");
			nvgFontSize(vg, SPLIT_TEXT);
			nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
			nvgText(
				vg,
				this->rect.x + SPLIT_ELEM_X_PADDING,
				this->rect.y + height + SPLIT_ELEM_X_PADDING * 0.5f + 1,
				prop->get(prop, i),
				NULL
			);
			nvgResetScissor(vg);
		}
		
		height += SPLIT_TEXT_H;
	}
}

void (*sDropMenuDraw[])(GeoGrid*, DropMenu*) = {
	[PROP_ENUM] = DropMenu_Draw_Enum,
};

void DropMenu_Draw(GeoGrid* geo) {
	DropMenu* this = &geo->dropMenu;
	PropEnum* prop = this->prop;
	MouseInput* mouse = &geo->input->mouse;
	
	if (prop == NULL)
		return;
	
	glViewport(
		0,
		0,
		geo->winDim->x,
		geo->winDim->y
	);
	nvgBeginFrame(geo->vg, geo->winDim->x, geo->winDim->y, gPixelRatio); {
		sDropMenuDraw[this->type](geo, this);
	} nvgEndFrame(geo->vg);
	
	if (this->state.init == false) {
		this->state.init = true;
		
		return;
	}
	
	if (Input_GetMouse(geo->input, MOUSE_L)->press || Rect_PointDistance(&this->rect, mouse->pos.x, mouse->pos.y) > 32) {
		if (this->state.setCondition)
			prop->set(prop, this->key);
		
		DropMenu_Close(geo);
	}
}

// # # # # # # # # # # # # # # # # # # # #
// # Element Draw                        #
// # # # # # # # # # # # # # # # # # # # #

static void Element_Draw_Button(ElementCallInfo* info) {
	void* vg = info->geo->vg;
	ElButton* this = info->arg;
	
	Gfx_DrawRounderOutline(vg, this->element.rect, this->element.light);
	
	nvgBeginPath(vg);
	nvgFillColor(vg, Theme_Mix(0.10, this->element.base, this->element.light));
	nvgRoundedRect(vg, this->element.rect.x, this->element.rect.y, this->element.rect.w, this->element.rect.h, SPLIT_ROUND_R);
	nvgFill(vg);
	
	if (this->element.name && this->element.rect.w > 8) {
		char* txt = (char*)this->element.name;
		f32 width;
		
		nvgScissor(vg, UnfoldRect(this->element.rect));
		nvgFontFace(vg, "dejavu");
		nvgFontSize(vg, SPLIT_TEXT);
		nvgFontBlur(vg, 0.0);
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		
		width = Element_TextWidth(vg, txt);
		
		nvgFillColor(vg, this->element.texcol);
		nvgText(
			vg,
			this->element.rect.x + ClampMin((this->element.rect.w - width) * 0.5, SPLIT_ELEM_X_PADDING),
			this->element.rect.y + this->element.rect.h * 0.5 + 1,
			txt,
			NULL
		);
		nvgResetScissor(vg);
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
		if (this == gElementState.curTextbox) {
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
	
	if (gElementState.storeA != NULL && this == gElementState.curTextbox) {
		if (bound.w < this->element.rect.w) {
			gElementState.storeA = buffer;
		} else {
			txtA = gElementState.storeA;
			nvgTextBounds(vg, ccx, 0, txtA, txtB, (f32*)&bound);
			while (bound.w > this->element.rect.w - SPLIT_TEXT_PADDING * 2) {
				txtB--;
				nvgTextBounds(vg, ccx, 0, txtA, txtB, (f32*)&bound);
			}
			
			if (strlen(buffer) > 4) {
				s32 posA = (uptr)txtA - (uptr)buffer;
				s32 posB = (uptr)txtB - (uptr)buffer;
				
				if (gElementState.posText < posA || gElementState.posText > posB + 1 || posB - posA <= 4) {
					if (gElementState.posText < posA) {
						txtA -= posA - gElementState.posText;
					}
					if (gElementState.posText > posB + 1) {
						txtA += gElementState.posText - posB + 1;
					}
					if (posB - posA <= 4)
						txtA += (posB - posA) - 4;
					
					txtB = &buffer[strlen(buffer)];
					gElementState.storeA = txtA;
					
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
		
		if (this == gElementState.curTextbox)
			gElementState.storeA = txtA;
	}
	
	Input* inputCtx = info->geo->input;
	
	if (Split_CursorInRect(split, &this->element.rect) && inputCtx->mouse.clickL.hold) {
		if (this->isHintText) {
			this->isHintText = 2;
			gElementState.posText = 0;
			gElementState.posSel = strlen(this->txt);
		} else {
			if (Input_GetMouse(geo->input, MOUSE_L)->press) {
				f32 dist = 400;
				for (char* tempB = txtA; tempB <= txtB; tempB++) {
					Vec2s glyphPos;
					f32 res;
					nvgTextBounds(vg, ccx, 0, txtA, tempB, (f32*)&bound);
					glyphPos.x = this->element.rect.x + bound.w + SPLIT_TEXT_PADDING - 1;
					glyphPos.y = this->element.rect.y + bound.h - 1 + SPLIT_TEXT * 0.5;
					
					res = Math_Vec2s_DistXZ(split->mousePos, glyphPos);
					
					if (res < dist) {
						dist = res;
						
						gElementState.posText = (uptr)tempB - (uptr)buffer;
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
					
					res = Math_Vec2s_DistXZ(split->mousePos, glyphPos);
					
					if (res < dist) {
						dist = res;
						wow = (uptr)tempB - (uptr)buffer;
					}
				}
				
				if (wow != gElementState.posText) {
					gElementState.posSel = wow;
				}
			}
		}
	} else if (this->isHintText == 2) this->isHintText = 0;
	
	Gfx_DrawRounderOutline(vg, this->element.rect, this->element.light);
	
	nvgBeginPath(vg);
	nvgFillColor(vg, Theme_GetColor(THEME_ELEMENT_BASE, 255, 0.75f));
	nvgRoundedRect(vg, this->element.rect.x, this->element.rect.y, this->element.rect.w, this->element.rect.h, SPLIT_ROUND_R);
	nvgFill(vg);
	
	if (this == gElementState.curTextbox) {
		if (gElementState.posSel != -1) {
			s32 min = fmin(gElementState.posSel, gElementState.posText);
			s32 max = fmax(gElementState.posSel, gElementState.posText);
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
			gElementState.flickTimer++;
			
			if (gElementState.flickTimer % 30 == 0)
				gElementState.flickFlag ^= 1;
			
			if (gElementState.flickFlag) {
				nvgBeginPath(vg);
				nvgTextBounds(vg, 0, 0, txtA, &buffer[gElementState.posText], (f32*)&bound);
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
	
	nvgFontFace(vg, "dejavu");
	nvgFontSize(vg, SPLIT_TEXT);
	nvgFontBlur(vg, 0.0);
	nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
	
	if (this->element.disabled == false)
		nvgFillColor(vg, Theme_GetColor(THEME_TEXT, 255, 1.0f));
	else
		nvgFillColor(vg, Theme_Mix(0.45, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), Theme_GetColor(THEME_TEXT, 255, 1.15f)));
	
	if (this->element.dispText == true) {
		Rect r = this->element.rect;
		
		r.x = this->element.posTxt.x;
		r.w = this->element.rect.x - r.x - SPLIT_ELEM_X_PADDING;
		
		nvgScissor(vg, UnfoldRect(r));
		nvgText(
			vg,
			this->element.posTxt.x,
			this->element.posTxt.y,
			this->element.name,
			NULL
		);
	} else {
		nvgScissor(vg, UnfoldRect(this->element.rect));
		nvgText(
			vg,
			this->element.rect.x,
			this->element.rect.y + this->element.rect.h * 0.5 + 1,
			this->element.name,
			NULL
		);
		Free(this);
	}
	nvgResetScissor(vg);
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
		Math_DelSmoothStepToF(&this->lerp, 0.8f - gElementState.breath * 0.08, 0.178f, 0.1f, 0.0f);
	else
		Math_DelSmoothStepToF(&this->lerp, 0.0f, 0.268f, 0.1f, 0.0f);
	
	Gfx_DrawRounderOutline(vg, this->element.rect, this->element.light);
	Gfx_DrawRounderRect(vg, this->element.rect, this->element.base);
	
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
		f32 dist = Math_Vec2f_DistXZ(zero, pos);
		s16 yaw = Math_Vec2f_Yaw(zero, pos);
		
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
	f32 step = (this->max - this->min) * 0.5f;
	
	if (this->isInt) {
		s32 mul = rint(Remap(this->value, 0, 1, this->min, this->max));
		
		Math_DelSmoothStepToF(&this->vValue, Remap(mul, this->min, this->max, 0, 1), 0.5f, step, 0.0f);
	} else
		Math_DelSmoothStepToF(&this->vValue, this->value, 0.5f, step, 0.0f);
	
	rect.x = this->element.rect.x;
	rect.y = this->element.rect.y;
	rect.w = this->element.rect.w;
	rect.h = this->element.rect.h;
	rect.w = ClampMin(rect.w * this->vValue, 0);
	
	rect.x = rect.x + 2;
	rect.y = rect.y + 2;
	rect.w = ClampMin(rect.w - 4, 0);
	rect.h = rect.h - 4;
	
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
	
	nvgScissor(vg, UnfoldRect(this->element.rect));
	nvgFontFace(vg, "dejavu");
	nvgFontSize(vg, SPLIT_TEXT);
	nvgFontBlur(vg, 0.0);
	nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
	
	nvgFillColor(vg, this->element.texcol);
	nvgText(
		vg,
		this->element.rect.x + this->element.rect.w * 0.5,
		this->element.rect.y + this->element.rect.h * 0.5 + 1,
		this->textBox.txt,
		NULL
	);
	nvgResetScissor(vg);
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
	
	Gfx_DrawRounderOutline(vg, r, this->element.light);
	
	// if (&this->element == geo->dropMenu.element) {
	// 	if (geo->dropMenu.state.up == 1)
	// 		r.y -= 2;
	// 	r.h += 2;
	// }
	
	Gfx_DrawRounderRect(vg, r, this->element.shadow);
	
	r = this->element.rect;
	
	if (prop && prop->get(prop, prop->key)) {
		nvgScissor(vg, UnfoldRect(this->element.rect));
		nvgFontFace(vg, "dejavu");
		nvgFontSize(vg, SPLIT_TEXT);
		nvgFontBlur(vg, 0.0);
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		
		if (geo->dropMenu.element == (void*)this)
			nvgFillColor(vg, this->element.light);
		
		else
			nvgFillColor(vg, this->element.texcol);
		
		nvgText(
			vg,
			r.x + SPLIT_ELEM_X_PADDING,
			r.y + r.h * 0.5 + 1,
			prop->get(prop, prop->key),
			NULL
		);
		nvgResetScissor(vg);
	}
	
	center.x = r.x + r.w - 5 - 5;
	center.y = r.y + r.h * 0.5f;
	
	nvgBeginPath(vg);
	nvgFillColor(vg, this->element.light);
	Gfx_Shape(vg, center, 0.95, 0, arrow, ArrayCount(arrow));
	nvgFill(vg);
}

static void Element_Draw_Line(ElementCallInfo* info) {
	Element* this = info->arg;
	
	Gfx_DrawRounderOutline(info->geo->vg, this->rect, Theme_GetColor(THEME_SHADOW, 255, 1.0f));
	Gfx_DrawRounderRect(info->geo->vg, this->rect, Theme_GetColor(THEME_HIGHLIGHT, 175, 1.0f));
	
	Free(this);
}

static void Element_Draw_Box(ElementCallInfo* info) {
	Element* this = info->arg;
	
	Gfx_DrawRounderOutline(info->geo->vg, this->rect, Theme_GetColor(THEME_HIGHLIGHT, 45, 1.0f));
	Gfx_DrawRounderRect(info->geo->vg, this->rect, Theme_GetColor(THEME_SHADOW, 45, 1.0f));
	
	Free(this);
}

// # # # # # # # # # # # # # # # # # # # #
// # Element Update                      #
// # # # # # # # # # # # # # # # # # # # #

void Element_SetContext(GeoGrid* setGeo, Split* setSplit) {
	gElementState.split = setSplit;
	gElementState.geo = setGeo;
}

// Returns button state, 0bXY, X == toggle, Y == pressed
s32 Element_Button(ElButton* this) {
	void* vg = gElementState.geo->vg;
	
	Assert(gElementState.geo && gElementState.split);
	
	if (!this->element.toggle)
		this->state = 0;
	
	if (this->autoWidth) {
		f32 bounds[4] = { 0.0f };
		
		nvgFontFace(vg, "dejavu");
		nvgFontSize(vg, SPLIT_TEXT);
		nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
		nvgTextBounds(vg, 0, 0, this->element.name, NULL, bounds);
		
		this->element.rect.w = bounds[2] + SPLIT_TEXT_PADDING * 2;
	}
	
	if (this->element.disabled || gElementState.geo->state.noClickInput)
		goto queue_element;
	
	if (Element_PressCondition(gElementState.geo, gElementState.split, &this->element)) {
		if (Input_GetMouse(gElementState.geo->input, MOUSE_L)->press) {
			this->state ^= 1;
			if (this->element.toggle)
				this->element.toggle ^= 0b10;
		}
	}
	
queue_element:
	Element_QueueElement(
		gElementState.geo,
		gElementState.split,
		Element_Draw_Button,
		this
	);
	
	return this->state;
}

void Element_Textbox(ElTextbox* this) {
	Assert(gElementState.geo && gElementState.split);
	if (this->element.disabled || gElementState.geo->state.noClickInput)
		goto queue_element;
	
	if (Element_PressCondition(gElementState.geo, gElementState.split, &this->element)) {
		if (Input_GetMouse(gElementState.geo->input, MOUSE_L)->press) {
			if (this != gElementState.curTextbox) {
				gElementState.ctrlA = 1;
				this->isHintText = 2;
				gElementState.posText = 0;
				gElementState.posSel = strlen(this->txt);
			}
			
			gElementState.curTextbox = this;
			gElementState.curSplitTextbox = gElementState.split;
		}
	}
	
queue_element:
	Element_QueueElement(
		gElementState.geo,
		gElementState.split,
		Element_Draw_Textbox,
		this
	);
}

// Returns text width
ElText* Element_Text(const char* txt) {
	ElText* this = SysCalloc(sizeof(ElText));
	
	Assert(gElementState.geo && gElementState.split);
	this->element.name = txt;
	this->element.disabled = false;
	
	Element_QueueElement(
		gElementState.geo,
		gElementState.split,
		Element_Draw_Text,
		this
	);
	
	return this;
}

s32 Element_Checkbox(ElCheckbox* this) {
	Assert(gElementState.geo && gElementState.split);
	this->element.rect.w = this->element.rect.h;
	
	if (this->element.disabled || gElementState.geo->state.noClickInput)
		goto queue_element;
	
	if (Element_PressCondition(gElementState.geo, gElementState.split, &this->element)) {
		if (Input_GetMouse(gElementState.geo->input, MOUSE_L)->press) {
			this->element.toggle ^= 1;
		}
	}
	
queue_element:
	Element_QueueElement(
		gElementState.geo,
		gElementState.split,
		Element_Draw_Checkbox,
		this
	);
	
	return this->element.toggle;
}

f32 Element_Slider(ElSlider* this) {
	Assert(gElementState.geo && gElementState.split);
	if (this->min == 0.0f && this->max == 0.0f)
		this->max = 1.0f;
	
	if (this->element.disabled || gElementState.geo->state.noClickInput)
		goto queue_element;
	
	if (Element_PressCondition(gElementState.geo, gElementState.split, &this->element) || this->holdState) {
		u32 pos = false;
		
		if (this->isTextbox) {
			this->isTextbox = false;
			if (gElementState.curTextbox == &this->textBox) {
				this->isTextbox = true;
				this->holdState = true;
				
				Element_Textbox(&this->textBox);
				
				return Lerp(this->value, this->min, this->max);
			} else {
				Element_Slider_SetValue(this, this->isInt ? Value_Int(this->textBox.txt) : Value_Float(this->textBox.txt));
				
				goto queue_element;
			}
		}
		
		if (Input_GetMouse(gElementState.geo->input, MOUSE_L)->press) {
			this->holdState = true;
			gElementState.split->elemBlockMouse++;
		} else if (Input_GetMouse(gElementState.geo->input, MOUSE_L)->hold && this->holdState) {
			if (gElementState.geo->input->mouse.vel.x) {
				if (this->isSliding == false) {
					Element_Slider_SetCursorToVal(gElementState.geo, gElementState.split, this);
				} else {
					if (Input_GetKey(gElementState.geo->input, KEY_LEFT_SHIFT)->hold)
						this->value += (f32)gElementState.geo->input->mouse.vel.x * 0.0001f;
					else
						this->value += (f32)gElementState.geo->input->mouse.vel.x * 0.001f;
					if (this->min || this->max)
						this->value = Clamp(this->value, 0.0f, 1.0f);
					
					pos = true;
				}
				
				this->isSliding = true;
			}
		} else if (Input_GetMouse(gElementState.geo->input, MOUSE_L)->release && this->holdState) {
			if (this->isSliding == false && Split_CursorInRect(gElementState.split, &this->element.rect)) {
				Element_Slider_SetTextbox(gElementState.split, this);
			}
			this->isSliding = false;
		} else {
			if (this->holdState)
				gElementState.split->elemBlockMouse--;
			this->holdState = false;
		}
		
		if (gElementState.geo->input->mouse.scrollY) {
			if (this->isInt) {
				f32 scrollDir = Clamp(gElementState.geo->input->mouse.scrollY, -1, 1);
				f32 valueIncrement = 1.0f / (this->max - this->min);
				s32 value = rint(Remap(this->value, 0.0f, 1.0f, this->min, this->max));
				
				if (Input_GetKey(gElementState.geo->input, KEY_LEFT_SHIFT)->hold)
					this->value = valueIncrement * value + valueIncrement * 5 * scrollDir;
				
				else
					this->value = valueIncrement * value + valueIncrement * scrollDir;
			} else {
				if (Input_GetKey(gElementState.geo->input, KEY_LEFT_SHIFT)->hold)
					this->value += gElementState.geo->input->mouse.scrollY * 0.1;
				
				else if (Input_GetKey(gElementState.geo->input, KEY_LEFT_ALT)->hold)
					this->value += gElementState.geo->input->mouse.scrollY * 0.001;
				
				else
					this->value += gElementState.geo->input->mouse.scrollY * 0.01;
			}
		}
		
		if (pos) Element_Slider_SetCursorToVal(gElementState.geo, gElementState.split, this);
	}
	
queue_element:
	this->value = Clamp(this->value, 0.0f, 1.0f);
	
	if (this->isSliding)
		Cursor_SetCursor(CURSOR_EMPTY);
	
	Element_QueueElement(
		gElementState.geo,
		gElementState.split,
		Element_Draw_Slider,
		this
	);
	
	if (this->isInt)
		return (s32)rint(Lerp(this->value, this->min, this->max));
	else
		return Lerp(this->value, this->min, this->max);
}

s32 Element_Combo(ElCombo* this) {
	Assert(gElementState.geo && gElementState.split);
	
	if (this->element.disabled || gElementState.geo->state.noClickInput)
		goto queue_element;
	
	Log("PROP %X", this->prop);
	if (this->prop && this->prop->num) {
		if (Element_PressCondition(gElementState.geo, gElementState.split, &this->element)) {
			s32 scrollY = Clamp(gElementState.geo->input->mouse.scrollY, -1, 1);
			
			if (Input_GetMouse(gElementState.geo->input, MOUSE_L)->press) {
				gElementState.geo->dropMenu.element = (void*)this;
				
				if (this->element.header)
					DropMenu_Init(
						gElementState.geo,
						this->prop,
						PROP_ENUM,
						Rect_AddPos(
							this->element.rect,
							gElementState.split->headRect
						)
					);
				else
					DropMenu_Init(
						gElementState.geo,
						this->prop,
						PROP_ENUM,
						Rect_AddPos(
							this->element.rect,
							gElementState.split->rect
						)
					);
			}
			
			if (scrollY)
				this->prop->set(this->prop, this->prop->key - scrollY);
		}
	}
	
queue_element:
	Element_QueueElement(
		gElementState.geo,
		gElementState.split,
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
	
	Assert(gElementState.geo && gElementState.split);
	
	if (drawLine) {
		CallocX(this);
		
		gElementState.rowY += SPLIT_ELEM_X_PADDING;
		
		this->rect.x = SPLIT_ELEM_X_PADDING * 2;
		this->rect.w = gElementState.split->rect.w - SPLIT_ELEM_X_PADDING * 4 - 1;
		this->rect.y = gElementState.rowY - SPLIT_ELEM_X_PADDING * 0.5;
		this->rect.h = 2;
		
		gElementState.rowY += SPLIT_ELEM_X_PADDING;
		
		Element_QueueElement(gElementState.geo, gElementState.split, Element_Draw_Line, this);
	} else {
		gElementState.rowY += SPLIT_ELEM_X_PADDING * 2;
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
	
	Assert(gElementState.geo && gElementState.split);
	
	if (io == BOX_START) {
		ROW_Y = gElementState.rowY;
		
		CallocX(THIS);
		Element_Row(gElementState.split, 1, THIS, 1.0);
		gElementState.rowY = ROW_Y + SPLIT_ELEM_X_PADDING;
		
		// Shrink Split Rect
		gElementState.rowX += SPLIT_ELEM_X_PADDING;
		gElementState.split->rect.w -= SPLIT_ELEM_X_PADDING * 2;
		
		Element_QueueElement(gElementState.geo, gElementState.split, Element_Draw_Box, THIS);
		
		BOX_PUSH();
	}
	
	if (io == BOX_END) {
		BOX_POP();
		
		THIS->rect.h = gElementState.rowY - ROW_Y;
		gElementState.rowY += SPLIT_ELEM_X_PADDING;
		
		// Expand Split Rect
		gElementState.split->rect.w += SPLIT_ELEM_X_PADDING * 2;
		gElementState.rowX -= SPLIT_ELEM_X_PADDING;
	}
#undef BoxPush
#undef BoxPop
}

void Element_DisplayName(Element* this) {
	ElementCallInfo* node;
	s32 w = this->rect.w * 0.25f;
	
	Assert(gElementState.geo && gElementState.split);
	
	this->posTxt.x = this->rect.x;
	this->posTxt.y = this->rect.y + this->rect.h * 0.5 + 1;
	
	this->rect.x += w;
	this->rect.w -= w;
	
	this->dispText = true;
	
	Calloc(node, sizeof(ElementCallInfo));
	Node_Add(gElementState.head, node);
	node->geo = gElementState.geo;
	node->split = gElementState.split;
	node->func = Element_Draw_Text;
	node->arg = this;
	node->elemFunc = "Element_DisplayName";
	node->update = false;
}

// # # # # # # # # # # # # # # # # # # # #
// # Element Property Settings           #
// # # # # # # # # # # # # # # # # # # # #

void Element_Slider_SetParams(ElSlider* this, f32 min, f32 max, char* type) {
	this->min = min;
	this->max = max;
	if (!stricmp(type, "int"))
		this->isInt = true;
	if (!stricmp(type, "float"))
		this->isInt = false;
}

void Element_Slider_SetValue(ElSlider* this, f64 val) {
	val = Clamp(val, this->min, this->max);
	this->vValue = this->value = Normalize(val, this->min, this->max);
}

void Element_Button_SetValue(ElButton* this, bool toggle, bool state) {
	this->element.toggle = toggle | (state == true ? 2 : 0);
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

void Element_Condition(Element* element, s32 condition) {
	element->disabled = !condition;
}

static void Element_SetRectImpl(Rect* rect, f32 x, f32 y, f32 w) {
	rect->x = x;
	rect->y = y;
	rect->w = w;
	rect->h = SPLIT_TEXT_H;
}

void Element_RowY(f32 y) {
	gElementState.rowY = y;
}

void Element_Row(Split* split, s32 rectNum, ...) {
	f32 x = SPLIT_ELEM_X_PADDING + gElementState.rowX;
	f32 width;
	va_list va;
	
	va_start(va, rectNum);
	
	Log("Setting [%d] Elements for Split [%s]", rectNum, split->name);
	
	for (s32 i = 0; i < rectNum; i++) {
		Element* this = va_arg(va, void*);
		f64 a = va_arg(va, f64);
		
		width = (f32)(split->rect.w - SPLIT_ELEM_X_PADDING * 3) * a;
		
		if (this) {
			Rect* rect = &this->rect;
			Log("[%d]: %s", i, this->name);
			
			if (rect)
				Element_SetRectImpl(rect, x + SPLIT_ELEM_X_PADDING, gElementState.rowY, width - SPLIT_ELEM_X_PADDING);
		}
		
		x += width;
	}
	
	gElementState.rowY += SPLIT_ELEM_Y_PADDING;
	
	va_end(va);
}

void Element_Header(Split* split, s32 num, ...) {
	f32 x = SPLIT_ELEM_X_PADDING;
	va_list va;
	
	va_start(va, num);
	
	Log("Setting [%d] Header Elements", num);
	
	for (s32 i = 0; i < num; i++) {
		Element* this = va_arg(va, Element*);
		Rect* rect = &this->rect;
		s32 w = va_arg(va, s32);
		
		this->header = true;
		
		if (rect)
			Element_SetRectImpl(rect, x + SPLIT_ELEM_X_PADDING, 4, w);
		x += w;
	}
	
	va_end(va);
}

// # # # # # # # # # # # # # # # # # # # #
// # Element Main                        #
// # # # # # # # # # # # # # # # # # # # #

static InputType* Textbox_GetKey(GeoGrid* geo, KeyMap key) {
	return &geo->input->key[key];
}

static InputType* Textbox_GetMouse(GeoGrid* geo, MouseMap key) {
	return &geo->input->mouse.clickArray[key];
}

void Element_Update(GeoGrid* geo) {
	gElementState.breathYaw += DegToBin(3);
	gElementState.breath = (SinS(gElementState.breathYaw) + 1.0f) * 0.5;
	
	if (gElementState.curTextbox) {
		char* txt = geo->input->buffer;
		s32 prevTextPos = gElementState.posText;
		s32 press = 0;
		
		if (gElementState.blockerTextbox == 0) {
			gElementState.blockerTextbox++;
			geo->input->state.keyBlock++;
		}
		
		if (Textbox_GetMouse(geo, MOUSE_ANY)->press || Textbox_GetKey(geo, KEY_ENTER)->press) {
			gElementState.posSel = -1;
			gElementState.ctrlA = 0;
			
			Assert(gElementState.curSplitTextbox != NULL);
			if (!Split_CursorInRect(gElementState.curSplitTextbox, &gElementState.curTextbox->element.rect) || Textbox_GetKey(geo, KEY_ENTER)->press) {
				gElementState.curTextbox = NULL;
				gElementState.curTextbox = NULL;
				gElementState.storeA = NULL;
				gElementState.flickFlag = 1;
				gElementState.flickTimer = 0;
				
				return;
			}
		}
		
		if (Textbox_GetKey(geo, KEY_LEFT_CONTROL)->hold) {
			if (Textbox_GetKey(geo, KEY_A)->press) {
				prevTextPos = strlen(gElementState.curTextbox->txt);
				gElementState.posText = 0;
				gElementState.ctrlA = 1;
			}
			
			if (Textbox_GetKey(geo, KEY_V)->press) {
				txt = (char*)Input_GetClipboardStr(geo->input);
			}
			
			if (Textbox_GetKey(geo, KEY_C)->press) {
				s32 max = fmax(gElementState.posSel, gElementState.posText);
				s32 min = fmin(gElementState.posSel, gElementState.posText);
				char* copy = xAlloc(512);
				
				memcpy(copy, &gElementState.curTextbox->txt[min], max - min);
				Input_SetClipboardStr(geo->input, copy);
			}
			
			if (Textbox_GetKey(geo, KEY_X)->press) {
				s32 max = fmax(gElementState.posSel, gElementState.posText);
				s32 min = fmin(gElementState.posSel, gElementState.posText);
				char* copy = xAlloc(512);
				
				memcpy(copy, &gElementState.curTextbox->txt[min], max - min);
				Input_SetClipboardStr(geo->input, copy);
			}
			
			if (Textbox_GetKey(geo, KEY_LEFT)->press) {
				gElementState.flickFlag = 1;
				gElementState.flickTimer = 0;
				while (gElementState.posText > 0 && isalnum(gElementState.curTextbox->txt[gElementState.posText - 1]))
					gElementState.posText--;
				if (gElementState.posText == prevTextPos)
					gElementState.posText--;
			}
			if (Textbox_GetKey(geo, KEY_RIGHT)->press) {
				gElementState.flickFlag = 1;
				gElementState.flickTimer = 0;
				while (isalnum(gElementState.curTextbox->txt[gElementState.posText]))
					gElementState.posText++;
				if (gElementState.posText == prevTextPos)
					gElementState.posText++;
			}
		} else {
			if (Textbox_GetKey(geo, KEY_LEFT)->press) {
				if (gElementState.ctrlA == 0) {
					gElementState.posText--;
					press++;
					gElementState.flickFlag = 1;
					gElementState.flickTimer = 0;
				} else {
					gElementState.posText = fmin(gElementState.posText, gElementState.posSel);
					gElementState.ctrlA = 0;
					gElementState.posSel = -1;
				}
			}
			
			if (Textbox_GetKey(geo, KEY_RIGHT)->press) {
				if (gElementState.ctrlA == 0) {
					gElementState.posText++;
					press++;
					gElementState.flickFlag = 1;
					gElementState.flickTimer = 0;
				} else {
					gElementState.posText = fmax(gElementState.posText, gElementState.posSel);
					gElementState.ctrlA = 0;
					gElementState.posSel = -1;
				}
			}
			
			if (Textbox_GetKey(geo, KEY_HOME)->press) {
				gElementState.posText = 0;
			}
			
			if (Textbox_GetKey(geo, KEY_END)->press) {
				gElementState.posText = strlen(gElementState.curTextbox->txt);
			}
			
			if (Textbox_GetKey(geo, KEY_LEFT)->hold || Textbox_GetKey(geo, KEY_RIGHT)->hold) {
				gElementState.timerTextbox++;
			} else {
				gElementState.timerTextbox = 0;
			}
			
			if (gElementState.timerTextbox >= 30 && gElementState.timerTextbox % 2 == 0) {
				if (Textbox_GetKey(geo, KEY_LEFT)->hold) {
					gElementState.posText--;
				}
				
				if (Textbox_GetKey(geo, KEY_RIGHT)->hold) {
					gElementState.posText++;
				}
				gElementState.flickFlag = 1;
				gElementState.flickTimer = 0;
			}
		}
		
		gElementState.posText = Clamp(gElementState.posText, 0, strlen(gElementState.curTextbox->txt));
		
		if (gElementState.posText != prevTextPos) {
			if (Textbox_GetKey(geo, KEY_LEFT_SHIFT)->hold || Input_GetShortcut(geo->input, KEY_LEFT_CONTROL, KEY_A)) {
				if (gElementState.posSel == -1)
					gElementState.posSel = prevTextPos;
			} else
				gElementState.posSel = -1;
		} else if (press) {
			gElementState.posSel = -1;
		}
		
		if (Textbox_GetKey(geo, KEY_BACKSPACE)->press || Input_GetShortcut(geo->input, KEY_LEFT_CONTROL, KEY_X)) {
			if (gElementState.posSel != -1) {
				s32 max = fmax(gElementState.posText, gElementState.posSel);
				s32 min = fmin(gElementState.posText, gElementState.posSel);
				
				StrRem(&gElementState.curTextbox->txt[min], max - min);
				gElementState.posText = min;
				gElementState.posSel = -1;
			} else if (gElementState.posText != 0) {
				StrRem(&gElementState.curTextbox->txt[gElementState.posText - 1], 1);
				gElementState.posText--;
			}
		}
		
		if (txt[0] != '\0') {
			if (gElementState.posSel != -1) {
				s32 max = fmax(gElementState.posText, gElementState.posSel);
				s32 min = fmin(gElementState.posText, gElementState.posSel);
				
				StrRem(&gElementState.curTextbox->txt[min], max - min);
				gElementState.posText = min;
				gElementState.posSel = -1;
			}
			
			if (strlen(gElementState.curTextbox->txt) == 0)
				snprintf(gElementState.curTextbox->txt, gElementState.curTextbox->size, "%s", txt);
			else {
				StrIns2(gElementState.curTextbox->txt, txt, gElementState.posText, gElementState.curTextbox->size);
			}
			
			gElementState.posText += strlen(txt);
		}
		
		gElementState.posText = Clamp(gElementState.posText, 0, strlen(gElementState.curTextbox->txt));
	} else {
		if (gElementState.blockerTextbox) {
			gElementState.blockerTextbox--;
			geo->input->state.keyBlock--;
		}
	}
}

static void Element_UpdateElement(ElementCallInfo* info) {
	GeoGrid* geo = info->geo;
	Element* this = info->arg;
	Split* split = info->split;
	f32 toggle = this->toggle == 3 ? 0.50f : 0.0f;
	f32 press = 1.0f;
	
	this->hover = false;
	this->press = false;
	
	if (Element_PressCondition(geo, split, this)) {
		this->hover = true;
		
		if (Input_GetMouse(geo->input, MOUSE_L)->hold)
			this->press = true;
	}
	
	if (this == (void*)gElementState.curTextbox || this == geo->dropMenu.element)
		this->hover = true;
	
	if (this->press && this->disabled == false)
		press = 1.2f;
	
	if (this->disabled) {
		const f32 mix = 0.5;
		if (this->hover && !this->disabled) {
			this->prim = Theme_Mix(mix, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), Theme_GetColor(THEME_PRIM,          255, 1.10f * press));
			this->shadow = Theme_Mix(mix, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), Theme_GetColor(THEME_ELEMENT_DARK,  255, (1.07f + toggle) * press));
			this->light = Theme_Mix(mix, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), Theme_GetColor(THEME_ELEMENT_LIGHT, 255, (1.07f + toggle) * press));
			this->texcol = Theme_Mix(mix, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), Theme_GetColor(THEME_TEXT,          255, 1.15f * press));
			
			if (this->toggle < 3)
				this->base = Theme_Mix(mix, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), Theme_GetColor(THEME_ELEMENT_BASE,  255, (1.07f + toggle) * press));
		} else {
			this->prim = Theme_Mix(mix, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), Theme_GetColor(THEME_PRIM,          255, 1.00f * press));
			this->shadow = Theme_Mix(mix, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), Theme_GetColor(THEME_ELEMENT_DARK,  255, (1.00f + toggle) * press));
			this->light = Theme_Mix(mix, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), Theme_GetColor(THEME_ELEMENT_LIGHT, 255, (0.50f + toggle) * press));
			this->texcol = Theme_Mix(mix, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), Theme_GetColor(THEME_TEXT,          255, 1.00f * press));
			
			if (this->toggle < 3)
				this->base = Theme_Mix(mix, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), Theme_GetColor(THEME_ELEMENT_BASE,  255, (1.00f + toggle) * press));
		}
		
		if (this->toggle == 3)
			this->base = Theme_Mix(0.75, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), Theme_GetColor(THEME_PRIM,  255, 0.95f * press));
	} else {
		if (this->hover && !this->disabled) {
			Theme_SmoothStepToCol(&this->prim,   Theme_GetColor(THEME_PRIM,          255, 1.10f * press),            0.25f, 0.35f, 0.001f);
			Theme_SmoothStepToCol(&this->shadow, Theme_GetColor(THEME_ELEMENT_DARK,  255, (1.07f + toggle) * press), 0.25f, 0.35f, 0.001f);
			Theme_SmoothStepToCol(&this->light,  Theme_GetColor(THEME_ELEMENT_LIGHT, 255, (1.07f + toggle) * press), 0.25f, 0.35f, 0.001f);
			Theme_SmoothStepToCol(&this->texcol, Theme_GetColor(THEME_TEXT,          255, 1.15f * press),            0.25f, 0.35f, 0.001f);
			
			if (this->toggle < 3)
				Theme_SmoothStepToCol(&this->base,   Theme_GetColor(THEME_ELEMENT_BASE,  255, (1.07f + toggle) * press), 0.25f, 0.35f, 0.001f);
		} else {
			Theme_SmoothStepToCol(&this->prim,   Theme_GetColor(THEME_PRIM,          255, 1.00f * press),            0.25f, 0.35f, 0.001f);
			Theme_SmoothStepToCol(&this->shadow, Theme_GetColor(THEME_ELEMENT_DARK,  255, (1.00f + toggle) * press), 0.25f, 0.35f, 0.001f);
			Theme_SmoothStepToCol(&this->light,  Theme_GetColor(THEME_ELEMENT_LIGHT, 255, (0.50f + toggle) * press), 0.25f, 0.35f, 0.001f);
			Theme_SmoothStepToCol(&this->texcol, Theme_GetColor(THEME_TEXT,          255, 1.00f * press),            0.25f, 0.35f, 0.001f);
			
			if (this->toggle < 3)
				Theme_SmoothStepToCol(&this->base,   Theme_GetColor(THEME_ELEMENT_BASE,  255, (1.00f + toggle) * press), 0.25f, 0.35f, 0.001f);
		}
		
		if (this->toggle == 3)
			Theme_SmoothStepToCol(&this->base,   Theme_GetColor(THEME_PRIM,  255, 0.95f * press), 0.25f, 8.85f, 0.001f);
	}
}

void Element_Draw(GeoGrid* geo, Split* split, bool header) {
	ElementCallInfo* elem = gElementState.head;
	Element* this;
	
	while (elem) {
		ElementCallInfo* next = elem->next;
		this = elem->arg;
		
		if (this->header == header && elem->split == split) {
			Log("ElemFunc: " PRNT_PRPL "%s", elem->elemFunc);
			
			if (elem->update)
				Element_UpdateElement(elem);
			
			if (header || !Element_DisableDraw(elem->arg, elem->split))
				elem->func(elem);
			Node_Kill(gElementState.head, elem);
		}
		
		elem = next;
	}
}
