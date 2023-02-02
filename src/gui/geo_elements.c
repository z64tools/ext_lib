#include "ext_geogrid.h"
#include "ext_interface.h"

#undef Element_Row
#undef Element_Disable
#undef Element_Enable
#undef Element_Condition
#undef Element_Name
#undef Element_DisplayName
#undef Element_Header

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

/*============================================================================*/

typedef struct {
    const char* item;
    f32       swing;
    f32       alpha;
    f32       colorLerp;
    Vec2f     pos;
    Vec2f     vel;
    Rect      rect;
    s32       hold;
    void*     src;
    PropList* prop;
} DragItem;

typedef struct {
    ElementQueCall* head;
    ElTextbox*      textbox;
    
    s16 breathYaw;
    f32 breath;
    
    GeoGrid* geo;
    Split*   split;
    
    f32 rowY;
    f32 rowX;
    
    s32 timerTextbox;
    s32 blockerTextbox;
    
    bool pushToHeader : 1;
    bool forceDisable : 1;
    
    DragItem dragItem;
} ElementState;

thread_local ElementState* gElemState = NULL;

void* ElementState_New(void) {
    ElementState* elemState;
    
    _assert((elemState = new(ElementState)) != NULL);
    
    return elemState;
}

void ElementState_SetElemState(void* elemState) {
    gElemState = elemState;
}

static const char* sFmt[] = {
    "%.3f",
    "%d"
};

static void DragItem_Init(GeoGrid* geo, void* src, Rect rect, const char* item, PropList* prop) {
    DragItem* this = &gElemState->dragItem;
    
    this->src = src;
    this->item = strdup(item);
    this->rect = rect;
    
    this->rect.x = -(this->rect.w * 0.5f);
    this->rect.y = -(this->rect.h * 0.5f);
    
    this->alpha = this->swing = 0;
    this->pos = Math_Vec2f_New(UnfoldVec2(geo->input->cursor.pos));
    this->hold = true;
    this->colorLerp = 0.0f;
    this->prop = prop;
}

static bool DragItem_Release(GeoGrid* geo, void* src) {
    DragItem* this = &gElemState->dragItem;
    
    if (src != this->src || this->hold == false) return false;
    
    this->hold = false;
    this->src = NULL;
    
    return true;
}

/*============================================================================*/

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

void Gfx_Text(void* vg, Rect r, enum NVGalign align, NVGcolor col, const char* txt) {
    Gfx_SetDefaultTextParams(vg);
    nvgTextAlign(vg, align);
    nvgFillColor(vg, col);
    nvgText(
        vg,
        r.x + SPLIT_ELEM_X_PADDING,
        r.y + r.h * 0.5 + 1,
        txt,
        NULL
    );
}

f32 Gfx_TextWidth(void* vg, const char* txt) {
    f32 bounds[4];
    
    Gfx_SetDefaultTextParams(vg);
    nvgTextBounds(vg, 0, 0, txt, 0, bounds);
    
    return bounds[2];
}

/*============================================================================*/

static void Element_QueueElement(GeoGrid* geo, Split* split, ElementFunc func, void* arg, const char* elemFunc) {
    ElementQueCall* node;
    
    node = new(ElementQueCall);
    Node_Add(gElemState->head, node);
    node->geo = geo;
    node->split = split;
    node->func = func;
    node->arg = arg;
    node->update = true;
    
    if (gElemState->forceDisable)
        ((Element*)arg)->disabled = true;
    
    node->elemFunc = elemFunc;
}

static s32 Element_PressCondition(GeoGrid* geo, Split* split, Element* this) {
    if (
        !geo->state.blockElemInput &
        (split->mouseInSplit || this->header) &&
        !split->blockMouse &&
        !split->elemBlockMouse
    ) {
        if (this->header) {
            Rect r = Rect_AddPos(this->rect, split->headRect);
            
            return Rect_PointIntersect(&r, geo->input->cursor.pos.x, geo->input->cursor.pos.y);
        } else {
            return Split_CursorInRect(split, &this->rect);
        }
    }
    
    return false;
}

#define ELEM_PRESS_CONDITION(this) Element_PressCondition(gElemState->geo, gElemState->split, &(this)->element)

static void Element_Slider_SetCursorToVal(GeoGrid* geo, Split* split, ElSlider* this) {
    f32 x = split->rect.x + this->element.rect.x + this->element.rect.w * this->value;
    f32 y = split->rect.y + this->element.rect.y + this->element.rect.h * 0.5;
    
    Input_SetMousePos(geo->input, x, y);
}

static void Element_Slider_SetTextbox(Split* split, ElSlider* this) {
    if (!gElemState->textbox) {
        this->isTextbox = true;
        
        this->textBox.element.rect = this->element.rect;
        this->textBox.align = ALIGN_CENTER;
        this->textBox.size = 32;
        
        this->textBox.type = this->isInt ? TEXTBOX_INT : TEXTBOX_F32;
        this->textBox.val.value = lerpf(this->value, this->min, this->max);
        this->textBox.val.max = this->max;
        this->textBox.val.min = this->min;
        this->textBox.val.update = true;
        Textbox_Set(&this->textBox, split);
    }
}

void Element_SetContext(GeoGrid* setGeo, Split* setSplit) {
    gElemState->split = setSplit;
    gElemState->geo = setGeo;
}

/*============================================================================*/

#define ELEMENT_QUEUE_CHECK() if (this->element.disabled || gElemState->geo->state.blockElemInput) \
    goto queue_element;
#define ELEMENT_QUEUE(draw)   goto queue_element; queue_element: \
    Element_QueueElement(gElemState->geo, gElemState->split,     \
        draw, this, __FUNCTION__);

#define SPLIT gElemState->split
#define GEO   gElemState->geo

/*============================================================================*/

static void Element_ButtonDraw(ElementQueCall* call) {
    void* vg = call->geo->vg;
    ElButton* this = call->arg;
    Rect r = this->element.rect;
    bool name = this->element.name != NULL;
    
    Gfx_DrawRounderOutline(vg, r, this->element.light);
    Gfx_DrawRounderRect(vg, r, Theme_Mix(0.10, this->element.base, this->element.light));
    
    if (this->icon) {
        if (name) {
            r.x += SPLIT_ELEM_X_PADDING;
            r.w -= SPLIT_ELEM_X_PADDING;
        }
        nvgBeginPath(vg);
        nvgFillColor(vg, this->element.texcol);
        Gfx_Vector(vg, Math_Vec2f_New(r.x, r.y + 1), 1.0f, 0, this->icon);
        nvgFill(vg);
        if (name) {
            r.x += 14;
            r.w -= 14;
        }
    }
    
    if (name && this->element.rect.w > 8) {
        char* txt = (char*)this->element.name;
        f32 width;
        Vec2f pos;
        
        nvgScissor(vg, UnfoldRect(this->element.rect));
        Gfx_SetDefaultTextParams(vg);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        
        width = Gfx_TextWidth(vg, txt);
        pos.y = r.y + r.h * 0.5 + 1;
        
        switch (this->align) {
            case ALIGN_LEFT:
                pos.x = r.x + SPLIT_ELEM_X_PADDING;
                break;
            case ALIGN_RIGHT:
                pos.x = (r.x + r.w) - width - SPLIT_ELEM_X_PADDING;
                break;
            case ALIGN_CENTER:
                pos.x = r.x + clamp_min((r.w - width) * 0.5, SPLIT_ELEM_X_PADDING);
                break;
        }
        
        nvgFillColor(vg, this->element.texcol);
        nvgText(vg, pos.x, pos.y, txt, NULL);
        nvgResetScissor(vg);
    }
}

s32 Element_Button(ElButton* this) {
    void* vg = gElemState->geo->vg;
    
    _assert(GEO && SPLIT);
    
    if (!this->element.toggle)
        this->state = 0;
    
    if (this->autoWidth) {
        f32 bounds[4] = { 0.0f };
        
        nvgFontFace(vg, "default");
        nvgFontSize(vg, SPLIT_TEXT);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgTextBounds(vg, 0, 0, this->element.name, NULL, bounds);
        
        this->element.rect.w = bounds[2] + SPLIT_TEXT_PADDING * 2;
    }
    
    ELEMENT_QUEUE_CHECK();
    
    if (ELEM_PRESS_CONDITION(this)) {
        if (Input_GetMouse(gElemState->geo->input, CLICK_L)->press) {
            this->state ^= 1;
            if (this->element.toggle)
                this->element.toggle ^= 0b10;
        }
    }
    
    ELEMENT_QUEUE(Element_ButtonDraw);
    
    return this->state;
}

/*============================================================================*/

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
            if (Input_GetMouse(gElemState->geo->input, CLICK_L)->press) {
                Rect* rect[] = { &gElemState->split->rect, &gElemState->split->headRect };
                
                ContextMenu_Init(gElemState->geo, &this->prop, this, PROP_COLOR, Rect_AddPos(this->element.rect, *rect[this->element.header]));
            }
        }
    }
    
    ELEMENT_QUEUE(Element_ColorBoxDraw);
}

/*============================================================================*/

static void Textbox_SetValue(ElTextbox* this) {
    if (this->type != TEXTBOX_STR) {
        if (this == gElemState->textbox) {
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

static void Textbox_Clear(GeoGrid* geo) {
    ElTextbox* this = gElemState->textbox;
    
    if (!this)
        return;
    
    Textbox_SetValue(this);
    gElemState->textbox = NULL;
    
    if (this->doBlock) {
        gElemState->blockerTextbox--;
        geo->input->state.block--;
        this->doBlock = false;
        this->ret = true;
    }
    
    warn("Textbox: Clear");
}

static void Textbox_Set(ElTextbox* this, Split* split) {
    if (!gElemState->textbox) {
        Textbox_SetValue(this);
        this->selA = 0;
        this->selB = strlen(this->txt);
    } else if (gElemState->textbox != this)
        return;
    gElemState->textbox = this;
    gElemState->textbox->split = split;
    
    warn("Textbox: Set [%s]", this->txt);
}

static InputType* Textbox_GetKey(GeoGrid* geo, KeyMap key) {
    return &geo->input->key[key];
}

static InputType* Textbox_GetMouse(GeoGrid* geo, CursorClick key) {
    return &geo->input->cursor.clickList[key];
}

void Element_SetActiveTextbox(GeoGrid* geo, Split* split, ElTextbox* this) {
    Textbox_Clear(geo);
    Textbox_Set(this, split ? split : &geo->private.dummySplit);
}

void Element_UpdateTextbox(GeoGrid* geo) {
    ElTextbox* this = gElemState->textbox;
    bool cPress = Textbox_GetMouse(geo, CLICK_L)->press;
    bool cHold = Textbox_GetMouse(geo, CLICK_L)->hold;
    
    gElemState->breathYaw += DegToBin(3);
    gElemState->breath = (SinS(gElemState->breathYaw) + 1.0f) * 0.5;
    
    if (this) {
        #define HOLDREPKEY(key) (Textbox_GetKey(geo, key)->press || Textbox_GetKey(geo, key)->dual)
        Split* split = this->split;
        this->modified = false;
        
        if (this->clearIcon) {
            if (Textbox_GetMouse(geo, CLICK_L)->press) {
                if (Split_CursorInRect(split, &this->clearRect)) {
                    arrzero(this->txt);
                    this->modified = true;
                    Textbox_Clear(geo);
                    return;
                }
            }
        }
        
        if (Textbox_GetKey(geo, KEY_ESCAPE)->press) {
            arrzero(this->txt);
            this->modified = true;
            Textbox_Clear(geo);
            this->ret = -1;
            
            return;
        }
        
        if (!this->doBlock) {
            gElemState->blockerTextbox++, geo->input->state.block++, this->doBlock = true;
            return;
        } else if (Textbox_GetKey(geo, KEY_ENTER)->press
            || (!Rect_PointIntersect(&this->element.rect, UnfoldVec2(split->cursorPos)) && cPress)) {
            Textbox_Clear(geo);
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
            Rect r = this->boxRect;
            s32 id = 0;
            f32 dist = FLT_MAX;
            
            if (this->isClicked && Math_Vec2s_DistXZ(split->mousePressPos, split->cursorPos) < 2)
                return;
            
            Gfx_SetDefaultTextParams(vg);
            nvgTextAlign(vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_LEFT);
            
            for (char* end = this->txt; ; end++) {
                f32 b[4];
                Vec2s p;
                f32 ndist;
                
                nvgTextBounds(vg, 0, 0, this->txt, end, b);
                p.x = r.x + SPLIT_ELEM_X_PADDING + b[2] - 2;
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
                if (kHome)        val = 0;
                else if (kEnd)    val = LEN;
                else if (kShift)  val = Shift(this->selPos, dir);
                else              val = Shift(dirPoint[clamp_min(dir, 0)], dir * shiftMul);
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
                
                strnins(this->txt, origin, this->selA, maxSize);
                
                this->selPos = this->selPivot = this->selB = this->selA = clamp_max(this->selA + 1, maxSize);
                this->modified = true;
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
    enum NVGalign align = NVG_ALIGN_MIDDLE + this->align;
    
    Gfx_DrawRounderRect(vg, this->boxRect, this->element.base);
    
    if (gElemState->textbox == this)
        Gfx_DrawRounderOutline(vg, this->boxRect, Theme_GetColor(THEME_ELEMENT_LIGHT, 255, 1.0f));
    else
        Gfx_DrawRounderOutline(vg, this->boxRect, this->element.light);
    
    nvgFillColor(vg, this->element.texcol);
    nvgScissor(vg, UnfoldRect(this->boxRect));
    
    if (gElemState->textbox == this) {
        Rect r = this->boxRect;
        char str[sizeof(this->txt)];
        
        align = NVG_ALIGN_MIDDLE | ALIGN_LEFT;
        
        r.y++;
        r.h -= 2;
        
        if (this->selA == this->selB) {
            memset(str, 0, sizeof(str));
            strncpy(str, this->txt, gElemState->textbox->selA + 1);
            strrep(str, " ", "-");
            r.x += Gfx_TextWidth(vg, str) + SPLIT_ELEM_X_PADDING;
            
            r.x -= 2;
            r.w = 2;
            Gfx_DrawRounderRect(vg, r, Theme_GetColor(THEME_DELETE, 255, 1.0f));
        }
        
    }
    
    Gfx_Text(vg, this->boxRect, align, this->element.texcol, this->txt);
    nvgResetScissor(vg);
    
    if (gElemState->textbox == this) {
        Rect r = this->boxRect;
        char str[sizeof(this->txt)];
        
        if (this->selA != this->selB) {
            s32 min = this->selA;
            s32 max = this->selB;
            s32 start;
            
            memset(str, 0, sizeof(str));
            _log("cpymin: %d", min);
            strncpy(str, this->txt, min + 1);
            strrep(str, " ", "-");
            r.x += clamp((start = Gfx_TextWidth(vg, str)) + SPLIT_ELEM_X_PADDING, 0, this->boxRect.w);
            
            memset(str, 0, sizeof(str));
            _log("cpymax: %d", max);
            strncpy(str, this->txt, max + 1);
            strrep(str, " ", "-");
            r.w = clamp(Gfx_TextWidth(vg, str) - start, 0, this->boxRect.w);
            
            nvgScissor(vg, UnfoldRect(r));
            Gfx_DrawRounderRect(vg, r, Theme_GetColor(THEME_TEXT, 255, 1.0f));
            Gfx_Text(vg, this->boxRect, align, this->element.shadow, this->txt);
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
    _assert(GEO && SPLIT);
    
    ELEMENT_QUEUE_CHECK();
    
    if (this != gElemState->textbox)
        this->modified = false;
    
    this->boxRect = this->element.rect;
    
    if (this->clearIcon) {
        this->clearRect.x = this->element.rect.x + this->element.rect.w - this->element.rect.h;
        this->clearRect.w = this->element.rect.h;
        this->clearRect.y = this->element.rect.y;
        this->clearRect.h = this->element.rect.h;
        
        this->boxRect.w -= this->clearRect.w;
    }
    
    if (ELEM_PRESS_CONDITION(this)) {
        if (Input_GetMouse(gElemState->geo->input, CLICK_L)->press) {
            if (this->clearIcon) {
                if (Split_CursorInRect(SPLIT, &this->clearRect)) {
                    arrzero(this->txt);
                    this->modified = true;
                    
                    if (this == gElemState->textbox)
                        Textbox_Clear(GEO);
                    
                    goto queue_element;
                }
            }
            
            Textbox_Set(this, SPLIT);
        }
    }
    
    ELEMENT_QUEUE(Element_TextboxDraw);
    
    s32 ret = this->ret;
    
    return (this->ret = 0, ret);
}

/*============================================================================*/

static void Element_TextDraw(ElementQueCall* call) {
    void* vg = call->geo->vg;
    // Split* split = call->split;
    ElText* this = call->arg;
    
    Gfx_SetDefaultTextParams(vg);
    
    if (this->element.disabled == false)
        nvgFillColor(vg, Theme_GetColor(THEME_TEXT, 255, 1.0f));
    else
        nvgFillColor(vg, Theme_Mix(0.45, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), Theme_GetColor(THEME_TEXT, 255, 1.15f)));
    
    if (this->element.dispText == true) {
        Rect r = this->element.rect;
        
        r.x = this->element.posTxt.x;
        r.w = this->element.rect.x - r.x - SPLIT_ELEM_X_PADDING;
        
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgScissor(vg, UnfoldRect(r));
        nvgText(vg, this->element.posTxt.x, this->element.posTxt.y, this->element.name, NULL);
    } else {
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgScissor(vg, UnfoldRect(this->element.rect));
        nvgText(vg, this->element.rect.x, this->element.rect.y + this->element.rect.h * 0.5 + 1, this->element.name, NULL);
    }
    nvgResetScissor(vg);
}

ElText* Element_Text(const char* txt) {
    ElText* this = new(ElText);
    
    _assert(GEO && SPLIT);
    this->element.name = txt;
    this->element.doFree = true;
    
    ELEMENT_QUEUE(Element_TextDraw);
    
    return this;
}

/*============================================================================*/

static void Element_CheckboxDraw(ElementQueCall* call) {
    void* vg = call->geo->vg;
    // Split* split = call->split;
    ElCheckbox* this = call->arg;
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
    Rect r = this->element.rect;
    
    r.w = r.h;
    
    if (this->element.toggle)
        Math_DelSmoothStepToF(&this->lerp, 0.8f - gElemState->breath * 0.08, 0.178f, 0.1f, 0.0f);
    else
        Math_DelSmoothStepToF(&this->lerp, 0.0f, 0.268f, 0.1f, 0.0f);
    
    Gfx_DrawRounderOutline(vg, r, this->element.light);
    Gfx_DrawRounderRect(vg, r, this->element.base);
    
    NVGcolor col = this->element.prim;
    f32 flipLerp = 1.0f - this->lerp;
    
    flipLerp = (1.0f - powf(flipLerp, 1.6));
    center.x = r.x + r.w * 0.5;
    center.y = r.y + r.h * 0.5;
    
    col.a = flipLerp * 1.67;
    col.a = clamp_min(col.a, 0.80);
    
    if (this->element.disabled)
        col = this->element.light;
    
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
    _assert(GEO && SPLIT);
    
    ELEMENT_QUEUE_CHECK();
    
    if (ELEM_PRESS_CONDITION(this)) {
        if (Input_GetMouse(gElemState->geo->input, CLICK_L)->press) {
            this->element.toggle ^= 1;
        }
    }
    
    ELEMENT_QUEUE(Element_CheckboxDraw);
    
    return this->element.toggle;
}

/*============================================================================*/

static void Element_SliderDraw(ElementQueCall* call) {
    void* vg = call->geo->vg;
    // Split* split = call->split;
    ElSlider* this = call->arg;
    Rectf32 rect;
    f32 step = (this->max - this->min) * 0.5f;
    
    if (this->isInt) {
        s32 mul = rint(remapf(this->value, 0, 1, this->min, this->max));
        
        Math_DelSmoothStepToF(&this->vValue, remapf(mul, this->min, this->max, 0, 1), 0.5f, step, 0.0f);
    } else
        Math_DelSmoothStepToF(&this->vValue, this->value, 0.5f, step, 0.0f);
    
    rect.x = this->element.rect.x;
    rect.y = this->element.rect.y;
    rect.w = this->element.rect.w;
    rect.h = this->element.rect.h;
    rect.w = clamp_min(rect.w * this->vValue, 0);
    
    rect.x = rect.x + 2;
    rect.y = rect.y + 2;
    rect.w = clamp_min(rect.w - 4, 0);
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
            (s32)rint(lerpf(this->value, this->min, this->max))
        );
    } else {
        snprintf(
            this->textBox.txt,
            31,
            sFmt[this->isInt],
            lerpf(this->value, this->min, this->max)
        );
    }
    
    nvgScissor(vg, UnfoldRect(this->element.rect));
    Gfx_SetDefaultTextParams(vg);
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

f32 Element_Slider(ElSlider* this) {
    _assert(GEO && SPLIT);
    
    if (this->min == 0.0f && this->max == 0.0f)
        this->max = 1.0f;
    
    ELEMENT_QUEUE_CHECK();
    
    nested(void, SetHold, ()) {
        this->holdState = true;
        gElemState->split->elemBlockMouse++;
    };
    
    nested(void, UnsetHold, ()) {
        if (this->holdState)
            gElemState->split->elemBlockMouse--;
        this->holdState = false;
    };
    
    if (ELEM_PRESS_CONDITION(this) || this->holdState) {
        u32 pos = false;
        
        SPLIT->splitBlockScroll++;
        
        if (this->isTextbox) {
            this->isTextbox = false;
            
            if (gElemState->textbox == &this->textBox) {
                this->isTextbox = true;
                this->holdState = true;
                
                Element_Textbox(&this->textBox);
                
                return lerpf(this->value, this->min, this->max);
            } else {
                UnsetHold();
                this->isSliding = false;
                this->isTextbox = false;
                Element_Slider_SetValue(this, this->isInt ? sint(this->textBox.txt) : sfloat(this->textBox.txt));
                
                goto queue_element;
            }
        }
        
        if (Input_GetMouse(gElemState->geo->input, CLICK_L)->press)
            SetHold();
        
        else if (Input_GetMouse(gElemState->geo->input, CLICK_L)->hold && this->holdState) {
            if (gElemState->geo->input->cursor.vel.x) {
                if (this->isSliding == false) {
                    Element_Slider_SetCursorToVal(gElemState->geo, gElemState->split, this);
                } else {
                    if (Input_GetKey(gElemState->geo->input, KEY_LEFT_SHIFT)->hold)
                        this->value += (f32)gElemState->geo->input->cursor.vel.x * 0.0001f;
                    else
                        this->value += (f32)gElemState->geo->input->cursor.vel.x * 0.001f;
                    if (this->min || this->max)
                        this->value = clamp(this->value, 0.0f, 1.0f);
                    
                    pos = true;
                }
                
                this->isSliding = true;
            }
        } else if (Input_GetMouse(gElemState->geo->input, CLICK_L)->release && this->holdState) {
            if (this->isSliding == false && Split_CursorInRect(gElemState->split, &this->element.rect))
                Element_Slider_SetTextbox(gElemState->split, this);
            
            this->isSliding = false;
        } else
            UnsetHold();
        
        if (Input_GetScroll(gElemState->geo->input)) {
            if (this->isInt) {
                f32 scrollDir = clamp(Input_GetScroll(gElemState->geo->input), -1, 1);
                f32 valueIncrement = 1.0f / (this->max - this->min);
                s32 value = rint(remapf(this->value, 0.0f, 1.0f, this->min, this->max));
                
                if (Input_GetKey(gElemState->geo->input, KEY_LEFT_SHIFT)->hold)
                    this->value = valueIncrement * value + valueIncrement * 5 * scrollDir;
                
                else
                    this->value = valueIncrement * value + valueIncrement * scrollDir;
            } else {
                if (Input_GetKey(gElemState->geo->input, KEY_LEFT_SHIFT)->hold)
                    this->value += Input_GetScroll(gElemState->geo->input) * 0.1;
                
                else if (Input_GetKey(gElemState->geo->input, KEY_LEFT_ALT)->hold)
                    this->value += Input_GetScroll(gElemState->geo->input) * 0.001;
                
                else
                    this->value += Input_GetScroll(gElemState->geo->input) * 0.01;
            }
        }
        
        if (pos) Element_Slider_SetCursorToVal(gElemState->geo, gElemState->split, this);
    }
    
    ELEMENT_QUEUE(Element_SliderDraw);
    this->value = clamp(this->value, 0.0f, 1.0f);
    
    if (this->isSliding)
        Cursor_SetCursor(CURSOR_EMPTY);
    
    if (this->isInt)
        return (s32)rint(lerpf(this->value, this->min, this->max));
    else
        return lerpf(this->value, this->min, this->max);
}

/*============================================================================*/

static void Element_ComboDraw(ElementQueCall* call) {
    ElCombo* this = call->arg;
    GeoGrid* geo = call->geo;
    void* vg = geo->vg;
    PropList* prop = this->prop;
    Rect r = this->element.rect;
    Vec2f center;
    Vec2f arrow[] = {
        { -5.0f, -2.5f  },
        { 0.0f,  2.5f   },
        { 5.0f,  -2.5f  },
        { -5.0f, -2.5f  },
    };
    
    Gfx_DrawRounderOutline(vg, r, this->element.light);
    Gfx_DrawRounderRect(vg, r, this->element.shadow);
    
    r = this->element.rect;
    
    if (prop && prop->num) {
        nvgScissor(vg, UnfoldRect(this->element.rect));
        Gfx_SetDefaultTextParams(vg);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        
        if (geo->dropMenu.element == (void*)this)
            nvgFillColor(vg, this->element.light);
        
        else
            nvgFillColor(vg, this->element.texcol);
        
        nvgText(
            vg,
            r.x + SPLIT_ELEM_X_PADDING,
            r.y + r.h * 0.5 + 1,
            PropList_Get(prop, prop->key),
            NULL
        );
        nvgResetScissor(vg);
    } else
        Gfx_DrawStripes(vg, this->element.rect);
    
    center.x = r.x + r.w - 5 - 5;
    center.y = r.y + r.h * 0.5f;
    
    nvgBeginPath(vg);
    nvgFillColor(vg, this->element.light);
    Gfx_Shape(vg, center, 0.95, 0, arrow, ArrCount(arrow));
    nvgFill(vg);
}

s32 Element_Combo(ElCombo* this) {
    _assert(GEO && SPLIT);
    
    ELEMENT_QUEUE_CHECK();
    
    _log("PROP %X", this->prop);
    
    if (this->prop && this->prop->num) {
        if (ELEM_PRESS_CONDITION(this)) {
            SPLIT->splitBlockScroll++;
            s32 scrollY = clamp(Input_GetScroll(gElemState->geo->input), -1, 1);
            
            if (Input_GetMouse(gElemState->geo->input, CLICK_L)->press) {
                Rect* rect[] = {
                    &gElemState->split->rect,
                    &gElemState->split->headRect
                };
                
                ContextMenu_Init(gElemState->geo, this->prop, this, PROP_ENUM, Rect_AddPos(this->element.rect, *rect[this->element.header]));
            } else if (scrollY)
                PropList_Set(this->prop, this->prop->key - scrollY);
        }
    } else
        this->element.disableTemp = true;
    
    ELEMENT_QUEUE(Element_ComboDraw);
    
    if (this->prop)
        return this->prop->key;
    
    else
        return 0;
}

/*============================================================================*/

static Rect Element_Container_GetPropRect(ElContainer* this, s32 i) {
    Rect r = this->element.rect;
    f32 yOffset = 0;
    PropList* prop = this->prop;
    
    if (prop && prop->detach && i >= prop->detachKey) {
        yOffset = SPLIT_TEXT_H * this->detachMul;
        yOffset -= SPLIT_TEXT_H;
    }
    
    return Rect_New(
        r.x,
        r.y + SPLIT_TEXT_H * i - rint(this->scroll.voffset) + yOffset,
        r.w, SPLIT_TEXT_H
    );
}

static Rect Element_Container_GetDragRect(ElContainer* this, s32 i) {
    Rect r = Element_Container_GetPropRect(this, i);
    
    r.y -= SPLIT_TEXT_H * 0.5f;
    
    return r;
}

static void Element_ContainerDraw(ElementQueCall* call) {
    ElContainer* this = call->arg;
    GeoGrid* geo = call->geo;
    void* vg = geo->vg;
    PropList* prop = this->prop;
    Rect r = this->element.rect;
    Rect scissor = this->element.rect;
    NVGcolor cornerCol = this->element.shadow;
    SplitScroll* scroll = &this->scroll;
    
    cornerCol.a = 2.5f;
    Gfx_DrawRounderOutline(vg, r, cornerCol);
    Gfx_DrawRounderRect(vg, r, this->element.shadow);
    
    Math_SmoothStepToF(
        &scroll->voffset, scroll->offset,
        0.25f, fabsf(scroll->offset - scroll->voffset) * 0.5f, 0.1f
    );
    
    Math_SmoothStepToF(&this->detachMul, 0.0f,
        0.25f,
        0.75f, 0.01f);
    
    if (!prop)
        return;
    
    scissor.x += 1;
    scissor.y += 1;
    scissor.w -= 2;
    scissor.h -= 2;
    
    nvgScissor(vg, UnfoldRect(scissor));
    
    if (Input_GetKey(gElemState->geo->input, KEY_LEFT_SHIFT)->hold)
        Math_SmoothStepToF(&this->copyLerp, 1.0f, 0.25f, 0.25f, 0.01f);
    else
        Math_SmoothStepToF(&this->copyLerp, 0.0f, 0.25f, 0.25f, 0.01f);
    
    for (int i = 0; i < prop->num; i++) {
        Rect tr = Element_Container_GetPropRect(this, i);
        
        if (prop->detach && prop->detachKey == i)
            continue;
        
        if (prop->key == i) {
            Rect vt = tr;
            NVGcolor col = this->element.prim;
            
            vt.x += 1; vt.w -= 2;
            vt.y += 1; vt.h -= 2;
            col.a = 0.85f;
            
            Gfx_DrawRounderRect(
                vg, vt, col
            );
        }
        
        if (!prop->detach) {
            if (Split_CursorInRect(gElemState->split, &tr) && this->copyLerp > EPSILON) {
                Rect vt = tr;
                vt.x += 1; vt.w -= 2;
                vt.y += 1; vt.h -= 2;
                
                Gfx_DrawRounderRect(vg, vt, nvgHSLA(0.33f, 1.0f, 0.5f, 125 * this->copyLerp));
            }
        }
        
        Gfx_Text(
            vg, tr, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE,
            this->element.texcol,
            PropList_Get(prop, i)
        );
    }
    nvgResetScissor(vg);
    
    DragItem* drag = &gElemState->dragItem;
    
    if (drag->src == this) {
        Rect r = this->element.rect;
        
        r.y -= 8;
        r.h += 16;
        
        if (Rect_PointIntersect(&r, UnfoldVec2(gElemState->split->cursorPos))) {
            Math_SmoothStepToF(&drag->colorLerp, 0.0f, 0.25f, 0.25f, 0.001f);
            
            for (int i = 0; i < prop->num + 1; i++) {
                Rect r = Element_Container_GetDragRect(this, i);
                
                if (i == this->prop->detachKey)
                    continue;
                
                if (Rect_PointIntersect(&r, UnfoldVec2(gElemState->split->cursorPos))
                    || prop->num == i) {
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
    
}

s32 Element_Container(ElContainer* this) {
    SplitScroll* scroll = &this->scroll;
    
    _assert(GEO && SPLIT);
    
    if (this->prop) {
        PropList* prop = this->prop;
        Input* input = gElemState->geo->input;
        Cursor* cursor = &gElemState->geo->input->cursor;
        s32 val = clamp(cursor->scrollY, -1, 1);
        DragItem* drag = &gElemState->dragItem;
        
        if (this->text) {
            if (&this->textBox == gElemState->textbox)
                goto queue_element;
            
            this->text = false;
        }
        
        scroll->max = SPLIT_TEXT_H * this->prop->num - this->element.rect.h;
        scroll->max = clamp_min(scroll->max, 0);
        
        ELEMENT_QUEUE_CHECK();
        
        if (this->heldKey > 0 && Math_Vec2s_DistXZ(cursor->pos, cursor->pressPos) > 8) {
            _log("Drag Item Init");
            DragItem_Init(gElemState->geo, this, this->grabRect, PropList_Get(prop, this->heldKey - 1), this->prop);
            this->detachID = this->heldKey - 1;
            this->detachMul = 1.0f;
            PropList_Detach(prop, this->heldKey - 1, Input_GetKey(input, KEY_LEFT_SHIFT)->hold);
            this->heldKey = 0;
        }
        
        if (!ELEM_PRESS_CONDITION(this) &&
            drag->src != this) {
            this->pressed = false;
            
            goto queue_element;
        }
        
        SPLIT->splitBlockScroll++;
        scroll->offset += (SPLIT_TEXT_H) *-val;
        
        if (!Input_GetMouse(input, CLICK_L)->press)
            if (!this->pressed)
                goto queue_element;
        
        if (ELEM_PRESS_CONDITION(this)) {
            if (Input_GetMouse(input, CLICK_L)->dual && prop->num) {
                _log("Rename");
                this->text = true;
                this->textBox.size = 32;
                strcpy(this->textBox.txt, PropList_Get(prop, prop->key));
                prop->list[prop->key] = realloc(prop->list[prop->key], 32);
                
                goto queue_element;
            }
            
            if (!this->pressed) {
                for (int i = 0; i < prop->num; i++) {
                    Rect r = Element_Container_GetPropRect(this, i);
                    
                    if (Rect_PointIntersect(&r, UnfoldVec2(gElemState->split->cursorPos))) {
                        this->heldKey = i + 1;
                        this->grabRect = r;
                    }
                }
            }
        }
        
        this->pressed = true;
        
        if (!Input_GetMouse(input, CLICK_L)->release)
            goto queue_element;
        
        this->pressed = false;
        this->heldKey = 0;
        
        if (DragItem_Release(gElemState->geo, this)) {
            Rect r = this->element.rect;
            
            r.y -= 8;
            r.h += 16;
            
            if (Rect_PointIntersect(&r, UnfoldVec2(gElemState->split->cursorPos))) {
                s32 i = 0;
                s32 id = 0;
                
                for (; i < prop->num; i++) {
                    Rect r = Element_Container_GetDragRect(this, i);
                    
                    if (i == prop->detachKey)
                        continue;
                    
                    if (Rect_PointIntersect(&r, UnfoldVec2(gElemState->split->cursorPos)))
                        break;
                    id++;
                }
                
                PropList_Retach(prop, id);
            }
            
            PropList_DestroyDetach(prop);
        } else {
            for (int i = 0; i < prop->num; i++) {
                Rect r = Element_Container_GetPropRect(this, i);
                
                if (Rect_PointIntersect(&r, UnfoldVec2(gElemState->split->cursorPos)))
                    PropList_Set(prop, i);
            }
        }
        
    }
    
    ELEMENT_QUEUE(Element_ContainerDraw);
    scroll->offset = clamp(scroll->offset, 0, scroll->max);
    
    if (this->text) {
        (void)0;
        PropList* prop = this->prop;
        this->textBox.element.rect = Element_Container_GetPropRect(this, prop->key);
        strcpy(prop->list[prop->key], this->textBox.txt);
        Element_Textbox(&this->textBox);
        
        return prop->key;
    }
    
    if (this->prop)
        return this->prop->key;
    
    else
        return 0;
}

/*============================================================================*/

static void Element_SeparatorDraw(ElementQueCall* call) {
    Element* this = call->arg;
    
    Gfx_DrawRounderOutline(call->geo->vg, this->rect, Theme_GetColor(THEME_SHADOW, 255, 1.0f));
    Gfx_DrawRounderRect(call->geo->vg, this->rect, Theme_GetColor(THEME_HIGHLIGHT, 175, 1.0f));
    
    vfree(this);
}

void Element_Separator(bool drawLine) {
    Element* this;
    
    _assert(GEO && SPLIT);
    
    if (drawLine) {
        this = calloc(sizeof(*this));
        
        gElemState->rowY += SPLIT_ELEM_X_PADDING;
        
        this->rect.x = SPLIT_ELEM_X_PADDING * 2;
        this->rect.w = gElemState->split->rect.w - SPLIT_ELEM_X_PADDING * 4 - 1;
        this->rect.y = gElemState->rowY - SPLIT_ELEM_X_PADDING * 0.5;
        this->rect.h = 2;
        
        gElemState->rowY += SPLIT_ELEM_X_PADDING;
        
        ELEMENT_QUEUE(Element_SeparatorDraw);
    } else {
        gElemState->rowY += SPLIT_ELEM_X_PADDING * 2;
    }
}

/*============================================================================*/

static void Element_BoxDraw(ElementQueCall* call) {
    Element* this = call->arg;
    
    Gfx_DrawRounderOutline(call->geo->vg, this->rect, Theme_GetColor(THEME_HIGHLIGHT, 45, 1.0f));
    Gfx_DrawRounderRect(call->geo->vg, this->rect, Theme_GetColor(THEME_SHADOW, 45, 1.0f));
}

s32 Element_Box(BoxInit io) {
    thread_local static Element* boxElemList[42];
    thread_local static f32 boxRowY[42];
    thread_local static s32 r;
    
    if (io == BOX_GET_NUM)
        return r;
    
    #define BOX_PUSH() r++
    #define BOX_POP()  r--
    #define THIS  boxElemList[r]
    #define ROW_Y boxRowY[r]
    
    _assert(GEO && SPLIT);
    _assert(r < ArrCount(boxElemList));
    
    if (io == BOX_START) {
        ROW_Y = gElemState->rowY;
        
        THIS = new(*THIS);
        THIS->doFree = true;
        Element_Row(1, THIS, 1.0);
        gElemState->rowY = ROW_Y + SPLIT_ELEM_X_PADDING;
        
        // Shrink Split Rect
        gElemState->rowX += SPLIT_ELEM_X_PADDING;
        gElemState->split->rect.w -= SPLIT_ELEM_X_PADDING * 2;
        
        Element_QueueElement(gElemState->geo, gElemState->split,
            Element_BoxDraw, THIS, __FUNCTION__);
        
        BOX_PUSH();
    }
    
    if (io == BOX_END) {
        BOX_POP();
        
        THIS->rect.h = gElemState->rowY - ROW_Y;
        gElemState->rowY += SPLIT_ELEM_X_PADDING;
        
        // Expand Split Rect
        gElemState->split->rect.w += SPLIT_ELEM_X_PADDING * 2;
        gElemState->rowX -= SPLIT_ELEM_X_PADDING;
    }
#undef BOX_PUSH
#undef BOX_POP
#undef THIS
#undef ROW_Y
    
    return 0;
}

/*============================================================================*/

void Element_DisplayName(Element* this, f32 lerp) {
    ElementQueCall* node;
    s32 w = this->rect.w * lerp;
    
    _assert(GEO && SPLIT);
    
    this->posTxt.x = this->rect.x;
    this->posTxt.y = this->rect.y + SPLIT_TEXT_PADDING - 1;
    
    this->rect.x += w + SPLIT_ELEM_X_PADDING * lerp;
    this->rect.w -= w + SPLIT_ELEM_X_PADDING * lerp;
    
    this->dispText = true;
    
    node = calloc(sizeof(ElementQueCall));
    Node_Add(gElemState->head, node);
    node->geo = gElemState->geo;
    node->split = gElemState->split;
    node->func = Element_TextDraw;
    node->arg = this;
    node->elemFunc = "Element_DisplayName";
    node->update = false;
}

/*============================================================================*/

void Element_Slider_SetParams(ElSlider* this, f32 min, f32 max, char* type) {
    this->min = min;
    this->max = max;
    if (!stricmp(type, "int") || strstart(type, "s") || strstart(type, "u"))
        this->isInt = true;
    else if (!stricmp(type, "float") || strstart(type, "f"))
        this->isInt = false;
}

void Element_Slider_SetValue(ElSlider* this, f64 val) {
    val = clamp(val, this->min, this->max);
    this->vValue = this->value = normf(val, this->min, this->max);
}

void Element_Button_SetValue(ElButton* this, bool toggle, bool state) {
    this->element.toggle = toggle | (state == true ? 2 : 0);
    this->state = state;
}

void Element_Combo_SetPropList(ElCombo* this, PropList* prop) {
    this->prop = prop;
}

void Element_Color_SetColor(ElColor* this, void* color) {
    this->prop.rgb8 = color;
}

void Element_Container_SetPropList(ElContainer* this, PropList* prop, u32 num) {
    this->prop = prop;
    this->element.heightAdd = SPLIT_TEXT_H * clamp_min(num - 1, 0);
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
    gElemState->rowY = y;
}

void Element_Row(s32 rectNum, ...) {
    f32 x = SPLIT_ELEM_X_PADDING + gElemState->rowX;
    f32 yadd = 0;
    f32 width;
    va_list va;
    
    va_start(va, rectNum);
    
    _log("Setting [%d] Elements for Split ID %d", rectNum, gElemState->split->id);
    
    for (int i = 0; i < rectNum; i++) {
        Element* this = va_arg(va, void*);
        f64 a = va_arg(va, f64);
        
        width = (f32)(gElemState->split->rect.w - SPLIT_ELEM_X_PADDING * 3) * a;
        
        if (this) {
            Rect* rect = &this->rect;
            _log("[%d]: %s", i, this->name);
            
            if (this->heightAdd && yadd == 0)
                yadd = this->heightAdd;
            
            if (rect) {
                Element_SetRectImpl(
                    rect,
                    x + SPLIT_ELEM_X_PADDING, rint(gElemState->rowY - gElemState->split->scroll.voffset),
                    width - SPLIT_ELEM_X_PADDING, yadd
                );
            }
        }
        
        x += width;
    }
    
    gElemState->rowY += SPLIT_ELEM_Y_PADDING + yadd;
    
    if (gElemState->rowY >= gElemState->split->rect.h) {
        gElemState->split->scroll.enabled = true;
        gElemState->split->scroll.max = gElemState->rowY - gElemState->split->rect.h;
    } else
        gElemState->split->scroll.enabled = false;
    
    va_end(va);
}

void Element_Header(s32 num, ...) {
    f32 x = SPLIT_ELEM_X_PADDING;
    va_list va;
    
    va_start(va, num);
    
    _log("Setting [%d] Header Elements", num);
    
    for (int i = 0; i < num; i++) {
        Element* this = va_arg(va, Element*);
        Rect* rect = &this->rect;
        s32 w = va_arg(va, s32);
        
        this->header = true;
        
        if (rect)
            Element_SetRectImpl(rect, x + SPLIT_ELEM_X_PADDING, 4, w, 0);
        x += w + SPLIT_ELEM_X_PADDING;
    }
    
    va_end(va);
}

/*============================================================================*/

static void Element_UpdateElement(ElementQueCall* call) {
    GeoGrid* geo = call->geo;
    Element* this = call->arg;
    Split* split = call->split;
    f32 toggle = this->toggle == 3 ? 0.50f : 0.0f;
    f32 press = 1.0f;
    bool disabled = (this->disabled || this->disableTemp);
    
    this->disableTemp = false;
    this->hover = false;
    this->press = false;
    
    if (Element_PressCondition(geo, split, this)) {
        this->hover = true;
        
        if (Input_GetMouse(geo->input, CLICK_L)->hold)
            this->press = true;
    }
    
    if (this == (void*)gElemState->textbox || this == geo->dropMenu.element)
        this->hover = true;
    
    if (this->press && !disabled)
        press = 1.2f;
    
    if (disabled) {
        const f32 mix = 0.5;
        
        this->prim = Theme_Mix(mix, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), Theme_GetColor(THEME_PRIM,          255, 1.00f * press));
        this->shadow = Theme_Mix(mix, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), Theme_GetColor(THEME_ELEMENT_DARK,  255, (1.00f + toggle) * press));
        this->light = Theme_Mix(mix, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), Theme_GetColor(THEME_ELEMENT_LIGHT, 255, (0.50f + toggle) * press));
        this->texcol = Theme_Mix(mix, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), Theme_GetColor(THEME_TEXT,          255, 1.00f * press));
        
        if (this->toggle < 3)
            this->base = Theme_Mix(mix, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), Theme_GetColor(THEME_ELEMENT_BASE,  255, (1.00f + toggle) * press));
        
        if (this->toggle == 3)
            this->base = Theme_Mix(0.75, Theme_GetColor(THEME_ELEMENT_BASE,  255, 1.00f), Theme_GetColor(THEME_PRIM,  255, 0.95f * press));
    } else {
        if (this->hover) {
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

void Element_Draw(GeoGrid* geo, Split* split, bool header) {
    ElementQueCall* elem = gElemState->head;
    Element* this;
    
    while (elem) {
        ElementQueCall* next = elem->next;
        this = elem->arg;
        
        if (this->header == header && elem->split == split) {
            _log("[%d][Split ID %d] ElemFunc: " PRNT_PRPL "%sDraw", header, split->id, elem->elemFunc);
            
            if (elem->update)
                Element_UpdateElement(elem);
            
            if (header || !Element_DisableDraw(elem->arg, elem->split))
                elem->func(elem);
            
            if (this->doFree)
                vfree(this);
            
            Node_Kill(gElemState->head, elem);
        }
        
        elem = next;
    }
}

void Element_Flush() {
    while (gElemState->head) {
        Element* this = gElemState->head->arg;
        if (this->doFree) vfree(this);
        Node_Kill(gElemState->head, gElemState->head);
    }
}

void DragItem_Draw(GeoGrid* geo) {
    DragItem* this = &gElemState->dragItem;
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
        
        if (this->prop->copy) {
            outc = Theme_Mix(this->colorLerp + 0.5f, nvgHSLA(0.33f, 1.0f, 0.5f, this->alpha), outc);
            inc = Theme_Mix(this->colorLerp + 0.5f, nvgHSLA(0.33f, 1.0f, 0.5f, this->alpha), inc);
        }
        
        f32 scale = lerpf(0.5f, this->alpha / 200.0f, 1.0f - (fabsf(this->swing) / 1.85f));
        nvgTranslate(vg,
            this->pos.x + this->rect.w * 0.5f,
            this->pos.y + this->rect.h * 0.5f);
        nvgScale(vg, scale, scale);
        nvgRotate(vg, this->swing);
        Gfx_DrawRounderOutline(vg, r, outc);
        Gfx_DrawRounderRect(vg, r, inc);
        Gfx_Text(
            vg, r, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE,
            Theme_GetColor(THEME_TEXT, this->alpha, 1.0f),
            this->item
        );
    } nvgEndFrame(vg);
    
    if (this->alpha <= EPSILON && this->item) {
        this->alpha = 0.0f;
        vfree(this->item);
    }
}
