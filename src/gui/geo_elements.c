#include "ext_geogrid.h"
#include "ext_interface.h"

#undef Element_Row
#undef Element_Disable
#undef Element_Enable
#undef Element_Condition
#undef Element_Name
#undef Element_SetNameLerp
#undef Element_Header
#undef Element_Box
#undef Element_Operatable

struct ElementQueCall;

typedef void (*ElementFunc)(struct ElementQueCall*);

static void Textbox_Set(ElTextbox*, Split*);

typedef struct ElementQueCall {
    struct ElementQueCall* next;
    
    void*       arg;
    Split*      split;
    ElementFunc func;
    GeoGrid*    geo;
    
    const char* elemFunc;
    u32 update : 1;
} ElementQueCall;

////////////////////////////////////////////////////////////////////////////////

typedef struct {
    const char* item;
    f32   swing;
    f32   alpha;
    f32   colorLerp;
    Vec2f pos;
    Vec2f vel;
    Rect  rect;
    s32   hold;
    void* src;
    Arli* list;
    bool  copy;
} DragItem;

typedef struct {
    GeoGrid* geo;
    Split*   split;
    ElementQueCall* head;
    ElTextbox*      textbox;
    
    s16 breathYaw;
    f32 breath;
    
    f32 rowY;
    f32 rowX;
    f32 shiftX;
    
    s32 timerTextbox;
    s32 blockerTextbox;
    
    bool pushToHeader : 1;
    bool forceDisable : 1;
    
    DragItem dragItem;
} ElementState;

thread_local ElementState* sElemState = NULL;

////////////////////////////////////////////////////////////////////////////////

void* ElementState_New(void) {
    ElementState* elemState;
    
    _assert((elemState = new(ElementState)) != NULL);
    
    return elemState;
}

void ElementState_SetElemState(void* elemState) {
    sElemState = elemState;
}

static const char* sFmt[] = {
    "%.3f",
    "%d"
};

static void DragItem_Init(GeoGrid* geo, void* src, Rect rect, const char* item, Arli* list) {
    DragItem* this = &sElemState->dragItem;
    
    this->src = src;
    this->item = strdup(item);
    this->rect = rect;
    
    this->rect.x = -(this->rect.w * 0.5f);
    this->rect.y = -(this->rect.h * 0.5f);
    
    this->alpha = this->swing = 0;
    this->pos = Math_Vec2f_New(UnfoldVec2(geo->input->cursor.pos));
    this->hold = true;
    this->colorLerp = 0.0f;
    this->list = list;
}

static bool DragItem_Release(GeoGrid* geo, void* src) {
    DragItem* this = &sElemState->dragItem;
    
    if (src != this->src || this->hold == false) return false;
    
    this->hold = false;
    this->src = NULL;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

void ScrollBar_Init(ScrollBar* this, int max, f32 height) {
    this->max = max;
    this->slotHeight = height;
}

Rect ScrollBar_GetRect(ScrollBar* this, int slot) {
    Rect r = {
        this->rect.x,
        this->rect.y + this->slotHeight * slot,
        this->rect.w,
        this->rect.h,
    };
    
    return r;
}

bool ScrollBar_Update(ScrollBar* this, Input* input, Vec2s cursorPos, Rect r) {
    const f32 visibleSlots = (f32)r.h / this->slotHeight;
    InputType* click = NULL;
    
    this->disabled = input == NULL;
    
    if (input) {
        click =  Input_GetCursor(input, CLICK_L);
        
        if (Rect_PointIntersect(&r, UnfoldVec2(cursorPos)))
            this->cur -= Input_GetScroll(input);
    }
    
    this->visMax = clamp_min(this->max - visibleSlots, 0);
    
    if (this->hold) {
        if (!click || !click->hold)
            this->hold--;
        
        else {
            f32 y = cursorPos.y + this->holdOffset;
            
            this->cur = ((y - r.y) / (r.h * (this->visMax / this->max))) * (this->max - visibleSlots);
        }
    }
    
    this->cur = clamp(this->cur, 0, this->visMax);
    Math_SmoothStepToF(&this->vcur, this->cur, 0.25f, 500.0f, 0.001f);
    
    this->mrect = this->rect = r;
    this->rect.h = this->slotHeight;
    this->rect.y -= this->vcur * this->slotHeight;
    this->cursorPos = cursorPos;
    
    this->srect = Rect_New(
        r.x + r.w - 14,
        lerpf(this->vcur / this->max, r.y, r.y + r.h),
        12,
        lerpf((this->vcur + visibleSlots) / this->max, r.y, r.y + r.h));
    this->srect.y = clamp(this->srect.y, r.y, r.y + r.h - 8);
    this->srect.h = clamp(this->srect.h - this->srect.y, 16, r.h);
    this->srect = Rect_Clamp(this->srect, this->mrect);
    
    if (Rect_PointIntersect(&this->srect, UnfoldVec2(cursorPos))) {
        if (click && click->press) {
            this->hold = 2;
            this->holdOffset = this->srect.y - cursorPos.y;
        }
    }
    
    return this->hold;
}

bool ScrollBar_Draw(ScrollBar* this, void* vg) {
    Vec2s cursorPos = this->cursorPos;
    Rect r = this->mrect;
    const f32 visibleSlots = (f32)r.h / this->slotHeight;
    
    if (this->disabled)
        Theme_SmoothStepToCol(&this->color, Theme_GetColor(THEME_HIGHLIGHT, 0, 1.0f), 0.5f, 1.0f, 0.01f);
    
    else if (this->hold)
        Theme_SmoothStepToCol(&this->color, Theme_GetColor(THEME_PRIM, 220, 1.0f), 0.5f, 1.0f, 0.01f);
    
    else {
        if (Rect_PointIntersect(&this->mrect, UnfoldVec2(cursorPos))) {
            if (Rect_PointIntersect(&this->srect, UnfoldVec2(cursorPos))) {
                Theme_SmoothStepToCol(&this->color, Theme_GetColor(THEME_HIGHLIGHT, 220, 1.0f), 0.5f, 1.0f, 0.01f);
                
            } else
                Theme_SmoothStepToCol(&this->color, Theme_GetColor(THEME_HIGHLIGHT, 125, 1.0f), 0.5f, 1.0f, 0.01f);
        } else
            Theme_SmoothStepToCol(&this->color, Theme_GetColor(THEME_HIGHLIGHT, 0, 1.0f), 0.5f, 1.0f, 0.01f);
    }
    
    NVGcolor color = this->color;
    
    if (visibleSlots > this->max) {
        NVGcolor al = color;
        f32 remap = remapf(visibleSlots / this->max, 1.0f, 1.0f + (2.0f / this->max), 0.0f, 1.0f);
        
        remap = clamp(remap, 0.0f, 1.0f);
        al.a = 0;
        
        color = Theme_Mix(remap, color, al);
    }
    
    Gfx_DrawRounderRect(vg, this->srect, color);
    
    return this->hold;
}

void ScrollBar_FocusSlot(ScrollBar* this, Rect r, int slot) {
    const f32 visibleSlots = (f32)r.h / this->slotHeight;
    
    this->visMax = clamp_min(this->max - visibleSlots, 0);
    this->cur = slot - visibleSlots * 0.5f;
    this->cur = clamp(this->cur, 0, this->visMax);
    this->vcur = this->cur;
}

////////////////////////////////////////////////////////////////////////////////

void Gfx_SetDefaultTextParams(void* vg) {
    nvgFontFace(vg, "default");
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
        f32 dist = Math_Vec2f_DistXZ(zero, pos);
        s16 yaw = Math_Vec2f_Yaw(zero, pos);
        
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

void Gfx_DrawRounderOutlineWidth(void* vg, Rect rect, NVGcolor color, int width) {
    nvgBeginPath(vg);
    nvgFillColor(vg, color);
    nvgRoundedRect(
        vg,
        rect.x - width,
        rect.y - width,
        rect.w + width * 2,
        rect.h + width * 2,
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

void Gfx_DrawStripes(void* vg, Rect rect) {
    Vec2f shape[] = {
        { 0,    0    }, { 0.6f, 1 },
        { 0.8f, 1    }, { 0.2f, 0 },
        { 0,    0    },
    };
    
    nvgScissor(vg, UnfoldRect(rect));
    nvgFillColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 5, 1.f));
    
    nvgBeginPath(vg);
    for (int i = -8; i < rect.w; i += 8)
        Gfx_Shape(vg, Math_Vec2f_New(rect.x + i, rect.y), 20.0f, 0, shape, ArrCount(shape));
    
    nvgFill(vg);
    nvgResetScissor(vg);
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

void Gfx_Text(void* vg, Rect r, enum NVGalign align, NVGcolor col, const char* txt) {
    Rect nr = Gfx_TextRect(vg, r, align, txt);
    
    r = Rect_Clamp(nr, r);
    
    align = NVG_ALIGN_MIDDLE | NVG_ALIGN_LEFT;
    
    Gfx_SetDefaultTextParams(vg);
    nvgTextAlign(vg, align);
    nvgFillColor(vg, col);
    nvgText(
        vg,
        r.x,
        r.y + r.h * 0.5 + 1,
        txt,
        NULL
    );
}

////////////////////////////////////////////////////////////////////////////////

static ElementQueCall* Element_QueueElement(GeoGrid* geo, Split* split, ElementFunc func, void* arg, const char* elemFunc) {
    ElementQueCall* node;
    
    node = new(ElementQueCall);
    Node_Add(sElemState->head, node);
    node->geo = geo;
    node->split = split;
    node->func = func;
    node->arg = arg;
    node->update = true;
    
    if (sElemState->forceDisable)
        ((Element*)arg)->disableTemp = true;
    
    node->elemFunc = elemFunc;
    
    return node;
}

static s32 Element_PressCondition(GeoGrid* geo, Split* split, Element* this) {
    if (
        !geo->state.blockElemInput &
        (split->cursorInSplit || this->header) &&
        !split->blockCursor &&
        !split->elemBlockCursor &&
        fabsf(split->scroll.voffset - split->scroll.offset) < 5
    ) {
        if (this->header) {
            Rect r = Rect_AddPos(this->rect, split->headRect);
            
            return Rect_PointIntersect(&r, geo->input->cursor.pos.x, geo->input->cursor.pos.y);
        } else
            return Split_CursorInRect(split, &this->rect);
    }
    
    return false;
}

#define ELEM_PRESS_CONDITION(this) Element_PressCondition(sElemState->geo, sElemState->split, &(this)->element)

void Element_SetContext(GeoGrid* setGeo, Split* setSplit) {
    sElemState->split = setSplit;
    sElemState->geo = setGeo;
}

////////////////////////////////////////////////////////////////////////////////

#define ELEMENT_QUEUE_CHECK(...) if (this->element.disabled || GEO->state.blockElemInput) { \
        __VA_ARGS__                                                                         \
        goto queue_element;                                                                 \
}
#define ELEMENT_QUEUE(draw)      goto queue_element; queue_element: \
    Element_QueueElement(sElemState->geo, sElemState->split,        \
        draw, this, __FUNCTION__);

#define SPLIT sElemState->split
#define GEO   sElemState->geo

#define Element_Action(func) if (this->element.func) this->element.func()

////////////////////////////////////////////////////////////////////////////////

static void Element_ButtonDraw(ElementQueCall* call) {
    void* vg = call->geo->vg;
    ElButton* this = call->arg;
    Rect r = this->element.rect;
    bool hasText = this->text != NULL;
    
    Gfx_DrawRounderOutline(vg, r, this->element.light);
    Gfx_DrawRounderRect(vg, r, Theme_Mix(0.10, this->element.base, this->element.light));
    
    if (this->icon) {
        if (hasText) {
            r.x += SPLIT_ELEM_X_PADDING;
            r.w -= SPLIT_ELEM_X_PADDING;
        }
        nvgBeginPath(vg);
        nvgFillColor(vg, this->element.texcol);
        Gfx_Vector(vg, Math_Vec2f_New(r.x, r.y + 1), 1.0f, 0, this->icon);
        nvgFill(vg);
        if (hasText) {
            r.x += 14;
            r.w -= 14;
        }
    }
    
    if (hasText && this->element.rect.w > 8) {
        nvgScissor(vg, UnfoldRect(this->element.rect));
        Gfx_SetDefaultTextParams(vg);
        Gfx_Text(vg, r, this->align, this->element.texcol, this->text);
        nvgResetScissor(vg);
    }
}

s32 Element_Button(ElButton* this) {
    int change = 0;
    
    _assert(GEO && SPLIT);
    
    if (!this->element.toggle)
        this->state = 0;
    
    ELEMENT_QUEUE_CHECK();
    
    if (ELEM_PRESS_CONDITION(this)) {
        if (Input_GetCursor(GEO->input, CLICK_L)->press) {
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
    void* vg = call->geo->vg;
    ElColor* this = call->arg;
    rgb8_t color = {
        0x20, 0x20, 0x20,
    };
    
    if (this->prop.rgb8) {
        color = *this->prop.rgb8;
        Gfx_DrawRounderRect(vg, this->element.rect, Theme_Mix(0.10, nvgRGBA(unfold_rgb(color), 0xFF), this->element.light));
    } else {
        Gfx_DrawRounderRect(vg, this->element.rect, Theme_Mix(0.10, nvgRGBA(unfold_rgb(color), 0xFF), this->element.light));
        Gfx_DrawStripes(vg, this->element.rect);
    }
    
    Gfx_DrawRounderOutline(vg, this->element.rect, this->element.light);
}

void Element_Color(ElColor* this) {
    this->element.disabled = (this->prop.rgb8 == NULL);
    
    ELEMENT_QUEUE_CHECK();
    
    if (this->prop.rgb8) {
        if (ELEM_PRESS_CONDITION(this)) {
            if ( Input_GetCursor(GEO->input, CLICK_L)->press) {
                Rect* rect[] = { &SPLIT->rect, &SPLIT->headRect };
                
                ContextMenu_Init(GEO, &this->prop, this, CONTEXT_PROP_COLOR,
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

void Element_ClearActiveTextbox(GeoGrid* geo) {
    ElementState* state = geo->elemState;
    ElTextbox* this = state->textbox;
    
    if (!this)
        return;
    
    Textbox_SetValue(this);
    state->textbox = NULL;
    
    if (this->doBlock) {
        if (Input_ClearState(geo->input, INPUT_BLOCK)) {
            warn("Duplicate Clear on INPUT_BLOCK, press enter");
            cli_getc();
        }
        
        state->blockerTextbox--;
        this->doBlock = false;
        this->ret = true;
    }
    
    warn("Textbox: Clear");
}

static void Textbox_Set(ElTextbox* this, Split* split) {
    if (!sElemState->textbox) {
        Textbox_SetValue(this);
        this->selA = 0;
        this->selB = strlen(this->txt);
    } else if (sElemState->textbox != this)
        return;
    sElemState->textbox = this;
    sElemState->textbox->split = split;
    
    warn("Textbox: Set [%s]", this->txt);
}

static InputType* Textbox_GetKey(GeoGrid* geo, KeyMap key) {
    return &geo->input->key[key];
}

static InputType* Textbox_GetMouse(GeoGrid* geo, CursorClick key) {
    return &geo->input->cursor.clickList[key];
}

void Element_SetActiveTextbox(GeoGrid* geo, Split* split, ElTextbox* this) {
    Element_ClearActiveTextbox(geo);
    Textbox_Set(this, split);
}

void Element_UpdateTextbox(GeoGrid* geo) {
    ElTextbox* this = sElemState->textbox;
    bool cPress = Textbox_GetMouse(geo, CLICK_L)->press;
    bool cHold = Textbox_GetMouse(geo, CLICK_L)->hold;
    
    sElemState->breathYaw += DegToBin(3);
    sElemState->breath = (SinS(sElemState->breathYaw) + 1.0f) * 0.5;
    
    if (this) {
        #define HOLDREPKEY(key) (Textbox_GetKey(geo, key)->press || Textbox_GetKey(geo, key)->dual)
        Split* split = this->split;
        this->modified = false;
        this->editing = true;
        
        if (this->clearIcon) {
            if (Textbox_GetMouse(geo, CLICK_L)->press) {
                if (Split_CursorInRect(split, &this->clearRect)) {
                    arrzero(this->txt);
                    this->modified = true;
                    Element_ClearActiveTextbox(geo);
                    return;
                }
            }
        }
        
        if (Textbox_GetKey(geo, KEY_ESCAPE)->press) {
            arrzero(this->txt);
            this->modified = true;
            Element_ClearActiveTextbox(geo);
            this->ret = -1;
            
            return;
        }
        
        if (!this->doBlock) {
            if (Input_SetState(geo->input, INPUT_BLOCK)) {
                warn("Duplicate Set on INPUT_BLOCK, press enter");
                cli_getc();
            }
            
            sElemState->blockerTextbox++;
            this->doBlock = true;
            return;
        } else if (Textbox_GetKey(geo, KEY_ENTER)->press || (!Rect_PointIntersect(&this->element.rect, UnfoldVec2(split->cursorPos)) && cPress)) {
            Element_ClearActiveTextbox(geo);
            return;
        }
        
        void* vg = geo->vg;
        Input* input = geo->input;
        bool ctrl = Textbox_GetKey(geo, KEY_LEFT_CONTROL)->hold;
        bool paste = Textbox_GetKey(geo, KEY_LEFT_CONTROL)->hold && Textbox_GetKey(geo, KEY_V)->press;
        bool copy = Textbox_GetKey(geo, KEY_LEFT_CONTROL)->hold && Textbox_GetKey(geo, KEY_C)->press;
        bool kShift = Textbox_GetKey(geo, KEY_LEFT_SHIFT)->hold;
        bool kEnd = Textbox_GetKey(geo, KEY_END)->press;
        bool kHome = Textbox_GetKey(geo, KEY_HOME)->press;
        bool kRem = Textbox_GetKey(geo, KEY_BACKSPACE)->press || Textbox_GetKey(geo, KEY_BACKSPACE)->dual;
        s32 dir = HOLDREPKEY(KEY_RIGHT) - HOLDREPKEY(KEY_LEFT);
        s32 maxSize = this->size ? this->size : sizeof(this->txt);
        const s32 LEN = strlen(this->txt);
        
        if (ctrl && Textbox_GetKey(geo, KEY_A)->press) {
            this->selPivot = this->selA = 0;
            this->selPos = this->selB = LEN;
            
            return;
        }
        
        if (cPress || cHold) {
            Rect r = this->element.rect;
            s32 id = 0;
            f32 dist = FLT_MAX;
            
            if (this->isClicked && Math_Vec2s_DistXZ(split->cursorPressPos, split->cursorPos) < 2)
                return;
            
            for (char* end = this->txt; ; end++) {
                Vec2s p;
                f32 ndist;
                
                Rect tr = Gfx_TextRectMinMax(vg, r, this->align, this->txt, 0, end - this->txt);
                
                p.x = RectW(tr) - 2;
                p.y = split->cursorPos.y;
                
                if ((ndist = Math_Vec2s_DistXZ(p, split->cursorPos)) < dist) {
                    dist = ndist;
                    
                    id = end - this->txt;
                }
                
                if (*end == '\0')
                    break;
            }
            
            if (cPress && Rect_PointIntersect(&r, UnfoldVec2(split->cursorPos))) {
                if (Textbox_GetMouse(geo, CLICK_L)->dual) {
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
            } else if (cHold && this->isClicked && split->cursorPos.x >= r.x && split->cursorPos.x < r.x + r.w) {
                this->selA = Min(this->selPivot, id );
                this->selPos = this->selB = Max(this->selPivot, id );
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
                        for (var i = cur + dir; i > 0 && i < LEN; i += dir) {
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
            }
            
            if (input->buffer[0] || paste) {
                const char* origin = input->buffer;
                
                if (paste) origin = Input_GetClipboardStr(geo->input);
                if (this->selA != this->selB) Remove();
                
                strnins(this->txt, origin, this->selA, maxSize + 1);
                this->modified = true;
                
                this->selPos = this->selPivot = this->selB = this->selA = clamp_max(this->selA + 1, maxSize);
            }
            
            if (copy)
                Input_SetClipboardStr(geo->input, x_strndup(&this->txt[this->selA], this->selB - this->selA));
            
        }
    }
#undef HOLDREPKEY
}

static void Element_TextboxDraw(ElementQueCall* call) {
    void* vg = call->geo->vg;
    ElTextbox* this = call->arg;
    
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
            
            r = Gfx_TextRectMinMax(vg, r, this->align, this->txt, min, max);
            r.y++;
            r.h -= 2;
            r.w = 2;
            
            Gfx_DrawRounderRect(vg, r, Theme_GetColor(THEME_NEW, 255, 1.0f));
        }
    }
    
    Gfx_Text(vg, this->element.rect, this->align, this->element.texcol, this->txt);
    nvgResetScissor(vg);
    
    if (this->element.disableTemp || this->element.disabled)
        Gfx_DrawStripes(vg, this->element.rect);
    
    if (sElemState->textbox == this) {
        
        if (this->selA != this->selB) {
            Rect r = this->element.rect;
            s32 min = this->selA;
            s32 max = this->selB;
            
            r = Gfx_TextRectMinMax(vg, r, this->align, this->txt, min, max);
            
            nvgScissor(vg, UnfoldRect(r));
            Gfx_DrawRounderRect(vg, r, Theme_GetColor(THEME_TEXT, 255, 1.0f));
            Gfx_Text(vg, this->element.rect, this->align, this->element.shadow, this->txt);
            nvgResetScissor(vg);
        }
    }
    
    if (this->clearIcon) {
        Rect r = this->clearRect;
        
        Gfx_DrawRounderRect(vg, r, Theme_Mix(0.25f, this->element.base, this->element.light));
        Gfx_DrawRounderOutline(vg, r, this->element.light);
        
        nvgBeginPath(vg);
        nvgFillColor(vg, Theme_Mix(0.25f, this->element.light, Theme_GetColor(THEME_HIGHLIGHT, 255, 1.0f)));
        Gfx_Vector(vg, Math_Vec2f_New(UnfoldVec2(r)), r.h / 16.0f, 0, gAssets.cross);
        nvgFill(vg);
    }
}

s32 Element_Textbox(ElTextbox* this) {
    int ret = 0;
    
    _assert(GEO && SPLIT);
    
    if (this != sElemState->textbox) {
        if (this->editing)
            ret = true;
        else
            this->modified = false;
        this->editing = false;
    } else
        
        ELEMENT_QUEUE_CHECK();
    
    if (this->clearIcon) {
        this->clearRect.x = this->element.rect.x + this->element.rect.w - this->element.rect.h;
        this->clearRect.w = this->element.rect.h;
        this->clearRect.y = this->element.rect.y;
        this->clearRect.h = this->element.rect.h;
        
        this->element.rect.w -= this->clearRect.w;
    }
    
    if (ELEM_PRESS_CONDITION(this)) {
        if ( Input_GetCursor(GEO->input, CLICK_L)->press) {
            if (this->clearIcon) {
                if (Split_CursorInRect(SPLIT, &this->clearRect)) {
                    arrzero(this->txt);
                    this->modified = true;
                    
                    if (this == sElemState->textbox)
                        Element_ClearActiveTextbox(GEO);
                    
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
    void* vg = call->geo->vg;
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
    
    _assert(GEO && SPLIT);
    this->element.name = txt;
    this->element.doFree = true;
    
    ELEMENT_QUEUE(Element_TextDraw);
    
    return this;
}

////////////////////////////////////////////////////////////////////////////////

static void Element_CheckboxDraw(ElementQueCall* call) {
    void* vg = call->geo->vg;
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
        s32 wi = wrapi(i, 0, ArrCount(sVector_Cross) - 1);
        Vec2f zero = { 0 };
        Vec2f pos = {
            sVector_Cross[wi].x * 0.75,
            sVector_Cross[wi].y * 0.75,
        };
        f32 dist = Math_Vec2f_DistXZ(zero, pos);
        s16 yaw = Math_Vec2f_Yaw(zero, pos);
        
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
    
    _assert(GEO && SPLIT);
    
    ELEMENT_QUEUE_CHECK();
    
    if (ELEM_PRESS_CONDITION(this)) {
        if ( Input_GetCursor(GEO->input, CLICK_L)->press) {
            this->element.toggle ^= 1;
        }
    }
    
    ELEMENT_QUEUE(Element_CheckboxDraw);
    
    return tog - this->element.toggle;
}

////////////////////////////////////////////////////////////////////////////////

static void Element_Slider_SetCursorToVal(GeoGrid* geo, Split* split, ElSlider* this) {
    Rect r = split->dispRect;
    
    if (this->element.header)
        r = split->headRect;
    
    f32 x = r.x + this->element.rect.x + this->element.rect.w * this->value;
    f32 y = r.y + this->element.rect.y + this->element.rect.h * 0.5;
    
    Input_SetMousePos(geo->input, x, y);
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
    void* vg = call->geo->vg;
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
    
    _assert(GEO && SPLIT);
    
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
        Input* input = GEO->input;
        u32 pos = false;
        
        SPLIT->splitBlockScroll++;
        
        if (!this->isSliding && !this->isTextbox) {
            if (Input_GetCursor(input, CLICK_L)->hold || Input_SelectClick(input, CLICK_L)) {
                if (input->cursor.dragDist > 1) {
                    BlockInput();
                    
                    this->isSliding = true;
                    Element_Slider_SetCursorToVal(GEO, SPLIT, this);
                    
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
            
            if (Input_GetScroll(GEO->input)) {
                if (this->isInt) {
                    f32 scrollDir = clamp(Input_GetScroll(GEO->input), -1, 1);
                    f32 valueIncrement = 1.0f / (this->max - this->min);
                    s32 value = rint(remapf(this->value, 0.0f, 1.0f, this->min, this->max));
                    
                    if (Input_GetKey(GEO->input, KEY_LEFT_SHIFT)->hold)
                        this->value = valueIncrement * value + valueIncrement * 5 * scrollDir;
                    
                    else
                        this->value = valueIncrement * value + valueIncrement * scrollDir;
                } else {
                    if (Input_GetKey(GEO->input, KEY_LEFT_SHIFT)->hold)
                        this->value += Input_GetScroll(GEO->input) * 0.1;
                    
                    else if (Input_GetKey(GEO->input, KEY_LEFT_ALT)->hold)
                        this->value += Input_GetScroll(GEO->input) * 0.001;
                    
                    else
                        this->value += Input_GetScroll(GEO->input) * 0.01;
                }
                
                ret = true;
            }
        }
        
        if (pos) Element_Slider_SetCursorToVal(GEO, SPLIT, this);
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
    GeoGrid* geo = call->geo;
    void* vg = geo->vg;
    Arli* list = this->arlist;
    Rect r = this->element.rect;
    Vec2f center;
    
    Gfx_DrawRounderOutline(vg, r, this->element.light);
    Gfx_DrawRounderRect(vg, r, this->element.shadow);
    
    r = this->element.rect;
    
    if (list && list->num) {
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
        
        if (geo->dropMenu.element == (void*)this)
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

s32 Element_Combo(ElCombo* this) {
    Arli* list = this->arlist;
    int index = 0;
    
    _assert(GEO && SPLIT);
    
    if (list) index = list->cur;
    
    ELEMENT_QUEUE_CHECK();
    
    if (list && list->num) {
        if (ELEM_PRESS_CONDITION(this)) {
            SPLIT->splitBlockScroll++;
            s32 scrollY = clamp(Input_GetScroll(GEO->input), -1, 1);
            
            if ( Input_GetCursor(GEO->input, CLICK_L)->release) {
                Rect* rect[] = { &SPLIT->rect, &SPLIT->headRect };
                
                ContextMenu_Init(GEO, this->arlist, this, CONTEXT_ARLI, Rect_AddPos(this->element.rect, *rect[this->element.header]));
                ScrollBar_FocusSlot(&GEO->dropMenu.scroll, GEO->dropMenu.rect, list->cur);
            } else if (scrollY)
                Arli_Set(list, list->cur - scrollY);
        }
    } else
        this->element.disableTemp = true;
    
    ELEMENT_QUEUE(Element_ComboDraw);
    
    if (list) {
        this->element.faulty = (list->cur < 0 || list->cur >= list->num);
        
        return (index - list->cur) | this->element.contextSet;
    }
    
    this->element.faulty = false;
    
    return 0;
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
    ElTab* this = call->arg;
    void* vg = GEO->vg;
}

int Element_Tab(ElTab* this) {
    int id = this->index;
    
    _assert(GEO && SPLIT);
    
    ELEMENT_QUEUE_CHECK();
    
    if (ELEM_PRESS_CONDITION(this)) {
        
        if (!Input_SelectClick(GEO->input, CLICK_L))
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

static void Element_ContainerDraw(ElementQueCall* call) {
    ElContainer* this = call->arg;
    GeoGrid* geo = call->geo;
    void* vg = geo->vg;
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
    
    nvgScissor(vg, UnfoldRect(scissor));
    
    if (Input_GetKey(GEO->input, KEY_LEFT_SHIFT)->hold)
        Math_SmoothStepToF(&this->copyLerp, 1.0f, 0.25f, 0.25f, 0.01f);
    else
        Math_SmoothStepToF(&this->copyLerp, 0.0f, 0.25f, 0.25f, 0.01f);
    
    for (int i = 0; i < list->num; i++) {
        Rect tr = Element_Container_GetListElemRect(this, i);
        
        if (this != sElemState->dragItem.src) {
            if (list->cur == i) {
                Rect vt = tr;
                NVGcolor col = this->element.prim;
                
                vt.x += 1; vt.w -= 2;
                vt.y += 1; vt.h -= 2;
                col.a = 0.85f;
                
                Gfx_DrawRounderRect(
                    vg, vt, col
                );
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
        
        nvgScissor(vg, UnfoldRect(tr));
        Gfx_Text(
            vg, tr, this->align,
            this->element.texcol,
            list->elemName(list, i)
        );
        nvgResetScissor(vg);
    }
    nvgResetScissor(vg);
    
    DragItem* drag = &sElemState->dragItem;
    
    if (drag->src == this) {
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

s32 Element_Container(ElContainer* this) {
    ScrollBar* scroll = &this->scroll;
    int set = -1;
    
    _assert(GEO && SPLIT);
    
    if (this->list) {
        Arli* list = this->list;
        Input* input = GEO->input;
        Cursor* cursor = &GEO->input->cursor;
        DragItem* drag = &sElemState->dragItem;
        
        if (this->text) {
            if (&this->textBox == sElemState->textbox)
                goto queue_element;
            
            this->text = false;
        }
        
        ScrollBar_Init(scroll, list->num, SPLIT_TEXT_H);
        
        ELEMENT_QUEUE_CHECK(
            ScrollBar_Update(scroll, NULL, SPLIT->cursorPos, this->element.rect);
        );
        
        if (ScrollBar_Update(scroll, input, SPLIT->cursorPos, this->element.rect))
            goto queue_element;
        
        if (this->drag && this->heldKey > 0 && Math_Vec2s_DistXZ(cursor->pos, cursor->pressPos) > 8) {
            _log("Drag Item Init");
            
            DragItem_Init(GEO, this, this->grabRect, list->elemName(list, this->heldKey - 1), this->list);
            
            if (Input_GetKey(input, KEY_LEFT_SHIFT)->hold) {
                Arli_CopyToBuf(list, this->heldKey - 1);
                // this->detach.key = list->num;
            } else
                Arli_RemoveToBuf(list, this->heldKey - 1);
            
            this->heldKey = 0;
        }
        
        if (!ELEM_PRESS_CONDITION(this) && drag->src != this) {
            this->pressed = false;
            
            goto queue_element;
        }
        
        SPLIT->splitBlockScroll++;
        
        if (Input_GetKey(input, KEY_UP)->press) {
            Arli_Set(list, list->cur - 1);
            set = list->cur;
            
            goto queue_element;
        }
        
        if (Input_GetKey(input, KEY_DOWN)->press) {
            Arli_Set(list, list->cur + 1);
            set = list->cur;
            
            goto queue_element;
        }
        
        if ( Input_GetCursor(input, CLICK_R)->press) {
            if (this->contextList) {
                Rect cr;
                this->contextKey = -1;
                
                for (int i = 0; i < list->num; i++) {
                    Rect r = Element_Container_GetListElemRect(this, i);
                    
                    if (Rect_PointIntersect(&r, UnfoldVec2(SPLIT->cursorPos))) {
                        cr = r;
                        this->contextKey = i;
                        break;
                    }
                }
                
                if (this->contextKey > -1) {
                    cr = Rect_AddPos(cr, SPLIT->rect);
                    cr.x = GEO->input->cursor.pressPos.x;
                    cr.w = 0;
                    ContextMenu_Init(GEO, this->contextList, this, CONTEXT_ARLI, cr);
                }
            }
            
            goto queue_element;
        }
        
        if (!Input_GetCursor(input, CLICK_L)->press) {
            if (!this->pressed)
                goto queue_element;
        }
        
        if (ELEM_PRESS_CONDITION(this)) {
            if ( Input_GetCursor(input, CLICK_L)->dual && list->num) {
                _log("Rename");
                this->text = true;
                strncpy(this->textBox.txt, list->elemName(list, list->cur), sizeof(this->textBox.txt));
                
                goto queue_element;
            }
            
            if (!this->pressed) {
                for (int i = 0; i < list->num; i++) {
                    Rect r = Element_Container_GetListElemRect(this, i);
                    
                    if (Rect_PointIntersect(&r, UnfoldVec2(SPLIT->cursorPos))) {
                        this->heldKey = i + 1;
                        this->grabRect = r;
                    }
                }
            }
        }
        
        this->pressed = true;
        
        if (!Input_GetCursor(input, CLICK_L)->release)
            goto queue_element;
        
        this->pressed = false;
        this->heldKey = 0;
        
        if (DragItem_Release(GEO, this)) {
            Rect r = this->element.rect;
            
            r.y -= 8;
            r.h += 16;
            
            if (Rect_PointIntersect(&r, UnfoldVec2(SPLIT->cursorPos))) {
                s32 i = 0;
                s32 id = 0;
                
                for (; i < list->num; i++) {
                    Rect r = Element_Container_GetDragRect(this, i);
                    
                    // if (i == this->detach.key)
                    // continue;
                    
                    if (Rect_PointIntersect(&r, UnfoldVec2(SPLIT->cursorPos)))
                        break;
                    id++;
                }
                
                Arli_Insert(list, id, 1, list->copybuf);
                Arli_Set(list, id);
            }
        } else {
            for (int i = 0; i < list->num; i++) {
                Rect r = Element_Container_GetListElemRect(this, i);
                
                if (Rect_PointIntersect(&r, UnfoldVec2(SPLIT->cursorPos))) {
                    Arli_Set(list, i);
                    set = i;
                }
            }
        }
    }
    
    ELEMENT_QUEUE(Element_ContainerDraw);
    
    if (this->text) {
        (void)0;
        Arli* list = this->list;
        
        this->textBox.element.rect = Element_Container_GetListElemRect(this, list->cur);
        Element_Textbox(&this->textBox);
        this->beingSet = true;
        
        return -2;
    }
    
    if (this->beingSet) {
        this->beingSet = false;
        
        return this->list->cur;
    }
    
    return set;
}

////////////////////////////////////////////////////////////////////////////////

static void Element_SeparatorDraw(ElementQueCall* call) {
    Element* this = call->arg;
    void* vg = call->geo->vg;
    
    nvgBeginPath(vg);
    nvgStrokeWidth(vg, 1.0f);
    nvgFillColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 125, 1.0f));
    nvgMoveTo(vg, this->rect.x, this->rect.y);
    nvgLineTo(vg, this->rect.x + this->rect.w, this->rect.y);
    nvgFill(vg);
    
    vfree(this);
}

void Element_Separator(bool drawLine) {
    Element* this;
    
    _assert(GEO && SPLIT);
    
    if (drawLine) {
        this = calloc(sizeof(*this));
        
        sElemState->rowY += SPLIT_ELEM_X_PADDING;
        
        this->rect.x = SPLIT_ELEM_X_PADDING * 2 + sElemState->shiftX;
        this->rect.w = SPLIT->rect.w - SPLIT_ELEM_X_PADDING * 4 - sElemState->shiftX;
        this->rect.y = sElemState->rowY - SPLIT_ELEM_X_PADDING * 0.5;
        this->rect.h = 1;
        
        sElemState->rowY += SPLIT_ELEM_X_PADDING;
        
        ELEMENT_QUEUE(Element_SeparatorDraw);
    } else {
        sElemState->rowY += SPLIT_ELEM_X_PADDING * 2;
    }
}

////////////////////////////////////////////////////////////////////////////////

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

void* Element_AllocBoxContext() {
    return new(BoxContext);
}

static void Element_BoxDraw(ElementQueCall* call) {
    ElBox* this = call->arg;
    Rect r = this->element.rect;
    void* vg = GEO->vg;
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
        
        Vec2f center = {
            r.x + SPLIT_TEXT_H * 0.5,
            r.y + SPLIT_TEXT_H * 0.5,
        };
        
        Math_SmoothStepToS(
            &panel->yaw,
            panel->state ? DegToBin(90) : 0,
            4, DegToBin(25), 1);
        
        nvgBeginPath(vg);
        nvgFillColor(vg, this->element.light);
        Gfx_Shape(vg, center, 0.95, panel->yaw, sShapeArrow, ArrCount(sShapeArrow));
        nvgFill(vg);
    }
}

static ElBox* BoxPush() {
    BoxContext* ctx = SPLIT->boxContext;
    
    return ctx->list[ctx->index++] = new(ElBox);
}

static ElBox* BoxPop() {
    BoxContext* ctx = SPLIT->boxContext;
    
    return ctx->list[--ctx->index];
}

int Element_Box(BoxState state, ...) {
    BoxContext* ctx = SPLIT->boxContext;
    va_list va;
    
    va_start(va, state);
    ElPanel* panel = va_arg(va, ElPanel*);
    const char* name = va_arg(va, char*);
    va_end(va);
    
    if (state & BOX_INDEX)
        return ctx->index;
    
    if ((state & BOX_MASK_IO) == BOX_START) {
        ElBox* this = BoxPush();
        
        this->element.instantColor = true;
        this->element.doFree = true;
        this->rowY = sElemState->rowY;
        this->panel = panel;
        
        Element_Row(1, this, 1.0);
        
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
        
        Element_QueueElement(GEO, SPLIT, Element_BoxDraw, this, __FUNCTION__);
        
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
            
            if (!GEO->state.blockElemInput && ELEM_PRESS_CONDITION(this)) {
                Input* input = GEO->input;
                
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

void Element_Button_SetProperties(ElButton* this, const char* text, bool toggle, bool state) {
    this->element.toggle = toggle | (state == true ? 2 : 0);
    this->state = state;
    if (text)
        this->text = text;
}

void Element_Combo_SetArli(ElCombo* this, Arli* arlist) {
    this->arlist = arlist;
}

void Element_Color_SetColor(ElColor* this, void* color) {
    this->prop.rgb8 = color;
}

void Element_Container_SetArli(ElContainer* this, Arli* list, u32 num) {
    this->list = list;
    this->element.heightAdd = SPLIT_TEXT_H * clamp_min(num - 1, 0);
}

bool Element_Textbox_SetText(ElTextbox* this, const char* txt) {
    if (!this->doBlock && !this->modified) {
        strncpy(this->txt, txt, sizeof(this->txt));
        
        return 1;
    }
    
    return false;
}

void Element_Name(Element* this, const char* name) {
    this->name = name;
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
    
    _assert(GEO && SPLIT);
    
    this->posTxt.x = this->rect.x;
    this->posTxt.y = this->rect.y + SPLIT_TEXT_PADDING - 1;
    
    if (this->nameLerp > 0.0f && this->nameLerp < 1.0f) {
        this->rect.x += w + SPLIT_ELEM_X_PADDING * this->nameLerp;
        this->rect.w -= w + SPLIT_ELEM_X_PADDING * this->nameLerp;
    } else {
        void* vg = sElemState->geo->vg;
        f32 w = Gfx_TextWidth(vg, this->name);
        
        this->rect.x += w + SPLIT_ELEM_X_PADDING;
        this->rect.w -= w + SPLIT_ELEM_X_PADDING;
    }
    
    this->dispText = true;
    
    node = calloc(sizeof(ElementQueCall));
    Node_Add(sElemState->head, node);
    node->geo = GEO;
    node->split = SPLIT;
    node->func = Element_TextDraw;
    node->arg = this;
    node->elemFunc = "Element_DisplayName";
    node->update = false;
}

void Element_Row(s32 rectNum, ...) {
    f32 x = SPLIT_ELEM_X_PADDING + sElemState->rowX + sElemState->shiftX;
    f32 yadd = 0;
    f32 width;
    va_list va;
    
    va_start(va, rectNum);
    
    for (int i = 0; i < rectNum; i++) {
        Element* this = va_arg(va, void*);
        f64 a = va_arg(va, f64);
        
        width = (f32)((SPLIT->rect.w - sElemState->rowX * 2 - sElemState->shiftX) - SPLIT_ELEM_X_PADDING * 3) * a;
        
        if (this) {
            Rect* rect = &this->rect;
            
            if (this->heightAdd && yadd == 0)
                yadd = this->heightAdd;
            
            if (rect) {
                Element_SetRectImpl(
                    rect,
                    x + SPLIT_ELEM_X_PADDING, rint(sElemState->rowY - SPLIT->scroll.voffset),
                    width - SPLIT_ELEM_X_PADDING, yadd
                );
                if (this->name)
                    Element_DisplayName(this);
            }
        }
        
        x += width;
    }
    
    sElemState->rowY += SPLIT_ELEM_Y_PADDING + yadd;
    
    if (sElemState->rowY >= SPLIT->rect.h) {
        SPLIT->scroll.enabled = true;
        SPLIT->scroll.max = sElemState->rowY - SPLIT->rect.h;
    } else
        SPLIT->scroll.enabled = false;
    
    va_end(va);
}

void Element_Header(s32 num, ...) {
    f32 x = SPLIT_ELEM_X_PADDING;
    va_list va;
    
    va_start(va, num);
    
    for (int i = 0; i < num; i++) {
        Element* this = va_arg(va, Element*);
        Rect* rect = &this->rect;
        s32 w = va_arg(va, s32);
        
        this->header = true;
        
        if (rect) {
            Element_SetRectImpl(rect, x + SPLIT_ELEM_X_PADDING, 4, w, 0);
            if (this->name)
                Element_DisplayName(this);
        }
        x += w + SPLIT_ELEM_X_PADDING * 2;
    }
    
    va_end(va);
}

int Element_Operatable(Element* this) {
    if (this->disabled || GEO->state.blockElemInput)
        return 0;
    
    if (!Element_PressCondition(GEO, SPLIT, this))
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
    GeoGrid* geo = call->geo;
    Split* split = call->split;
    Input* input = geo->input;
    f32 toggle = this->toggle == 3 ? 0.50f : 0.0f;
    f32 press = 1.0f;
    bool disabled = (this->disabled || this->disableTemp);
    
    this->disableTemp = false;
    this->hover = false;
    this->press = false;
    this->contextSet = false;
    
    if (Element_PressCondition(geo, split, this)) {
        this->hover = true;
        
        if (Input_GetCursor(input, CLICK_L)->hold)
            this->press = true;
    }
    
    if (this == (void*)sElemState->textbox || this == geo->dropMenu.element)
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
        NVGcolor base = Theme_GetColor(THEME_ELEMENT_BASE, 255, 1.25f);
        
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

void Element_Draw(GeoGrid* geo, Split* split, bool header) {
    ElementQueCall* elem = sElemState->head;
    Element* this;
    
    while (elem) {
        ElementQueCall* next = elem->next;
        this = elem->arg;
        
        if (this->header == header && elem->split == split) {
            _log("ElemDraw%s: " PRNT_PRPL "%sDraw", header ? "Header" : "Split", elem->elemFunc);
            
            if (elem->update)
                Element_UpdateElement(elem);
            
            if (split->dummy || header || !Element_DisableDraw(elem->arg, elem->split))
                Element_DrawInstance(elem, this);
            
            if (this->doFree)
                vfree(this);
            
            Node_Kill(sElemState->head, elem);
        }
        
        elem = next;
    }
    
    _log("ElemDraw%s: Done", header ? "Header" : "Split");
}

void Element_Flush() {
    while (sElemState->head) {
        Element* this = sElemState->head->arg;
        if (this->doFree) vfree(this);
        Node_Kill(sElemState->head, sElemState->head);
    }
}

void DragItem_Draw(GeoGrid* geo) {
    DragItem* this = &sElemState->dragItem;
    void* vg = geo->vg;
    Rect r = this->rect;
    Vec2f prevPos = this->pos;
    
    if (!this->item) return;
    
    if (this->hold) {
        Cursor* cursor = &geo->input->cursor;
        Vec2f target = Math_Vec2f_New(UnfoldVec2(cursor->pos));
        f32 dist = Math_Vec2f_DistXZ(this->pos, target);
        if (dist > EPSILON) {
            Vec2f seg = Math_Vec2f_LineSegDir(this->pos, target);
            f32 pdist = dist;
            
            Math_SmoothStepToF(&dist, 0.0f, 0.25f, 80.0f, 0.1f);
            this->vel = Math_Vec2f_MulVal(
                seg,
                pdist - dist
            );
            
        }
    }
    
    this->pos = Math_Vec2f_Add(
        this->pos,
        this->vel
    );
    
    this->swing += (this->pos.x - prevPos.x) * 0.01f;
    
    this->swing = clamp(this->swing, -1.25f, 1.25f);
    Math_SmoothStepToF(&this->swing, 0.0f, 0.25f, 0.2f, 0.001f);
    Math_SmoothStepToF(&this->alpha, this->hold ? 200.0f : 0.0f, 0.25f, 80.0f, 0.01f);
    
    nvgBeginFrame(vg, geo->wdim->x, geo->wdim->y, gPixelRatio); {
        NVGcolor outc = Theme_Mix(this->colorLerp, Theme_GetColor(THEME_ELEMENT_LIGHT, this->alpha, 1.0f), nvgHSLA(0, 1.0f, 0.5f, this->alpha));
        NVGcolor inc = Theme_Mix(this->colorLerp, Theme_GetColor(THEME_ELEMENT_BASE, this->alpha, 1.0f), nvgHSLA(0, 1.0f, 0.5f, this->alpha));
        
        if (this->copy) {
            outc = Theme_Mix(this->colorLerp + 0.5f, nvgHSLA(0.33f, 1.0f, 0.5f, this->alpha), outc);
            inc = Theme_Mix(this->colorLerp + 0.5f, nvgHSLA(0.33f, 1.0f, 0.5f, this->alpha), inc);
        }
        nvgSave(vg);
        
        f32 scale = lerpf(0.5f, this->alpha / 200.0f, 1.0f - (fabsf(this->swing) / 1.85f));
        nvgTranslate(vg,
            this->pos.x + this->rect.w * 0.5f,
            this->pos.y + this->rect.h * 0.5f);
        nvgScale(vg, scale, scale);
        nvgRotate(vg, this->swing);
        Gfx_DrawRounderOutline(vg, r, outc);
        Gfx_DrawRounderRect(vg, r, inc);
        Gfx_Text(
            vg, r, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE,
            Theme_GetColor(THEME_TEXT, this->alpha, 1.0f),
            this->item
        );
        nvgRestore(vg);
    } nvgEndFrame(vg);
    
    if (this->alpha <= EPSILON && this->item) {
        this->alpha = 0.0f;
        vfree(this->item);
    }
    
}
