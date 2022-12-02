#include "ext_geogrid.h"
#include "ext_interface.h"

#undef Element_Row
#undef Element_Disable
#undef Element_Enable
#undef Element_Condition
#undef Element_Name
#undef Element_DisplayName
#undef Element_Header

struct ElementCallInfo;

typedef void (*ElementFunc)(struct ElementCallInfo*);

typedef struct ElementCallInfo {
    struct ElementCallInfo* next;
    
    void*       arg;
    Split*      split;
    ElementFunc func;
    GeoGrid*    geo;
    
    const char* elemFunc;
    u32 update : 1;
} ElementCallInfo;

/* ───────────────────────────────────────────────────────────────────────── */

typedef struct {
    const char* item;
    f32   swing;
    f32   alpha;
    Vec2f pos;
    Vec2f vel;
    Rect  rect;
    s32   hold;
    void* src;
} DragItem;

ThreadLocal struct {
    ElementCallInfo* head;
    
    ElTextbox* curTextbox;
    Split*     curSplitTextbox;
    s32   posText;
    s32   posSel;
    char* storeA;
    s32   ctrlA;
    
    s32 flickTimer;
    s32 flickFlag;
    
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
} gElementState = {
    .posSel    = -1,
    .flickFlag = 1,
};

static const char* sFmt[] = {
    "%.3f",
    "%d"
};

static void DragItem_Init(GeoGrid* geo, void* src, Rect rect, const char* item) {
    DragItem* this = &gElementState.dragItem;
    
    this->src = src;
    this->item = StrDup(item);
    this->rect = rect;
    
    this->rect.x = -(this->rect.w * 0.5f);
    this->rect.y = -(this->rect.h * 0.5f);
    
    this->alpha = this->swing = 0;
    this->pos = Math_Vec2f_New(UnfoldVec2(geo->input->cursor.pos));
    this->hold = true;
}

static bool DragItem_Release(GeoGrid* geo, void* src) {
    DragItem* this = &gElementState.dragItem;
    
    if (src != this->src || this->hold == false) return false;
    
    this->hold = false;
    this->src = NULL;
    
    return true;
}

// # # # # # # # # # # # # # # # # # # # #
// #                                     #
// # # # # # # # # # # # # # # # # # # # #

void Gfx_SetDefaultTextParams(void* vg) {
    nvgFontFace(vg, "default");
    nvgFontSize(vg, SPLIT_TEXT);
    nvgFontBlur(vg, 0.0);
    nvgTextLetterSpacing(vg, -0.5f);
}

void Gfx_Shape(void* vg, Vec2f center, f32 scale, s16 rot, const Vec2f* p, u32 num) {
    bool move = true;
    
    for (s32 i = num - 1; !(i < 0); i--) {
        
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
    for (s32 i = -8; i < rect.w; i += 8)
        Gfx_Shape(vg, Math_Vec2f_New(rect.x + i, rect.y), 20.0f, 0, shape, ArrayCount(shape));
    
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

// # # # # # # # # # # # # # # # # # # # #
// #                                     #
// # # # # # # # # # # # # # # # # # # # #

static void Element_QueueElement(GeoGrid* geo, Split* split, ElementFunc func, void* arg, const char* elemFunc) {
    ElementCallInfo* node;
    
    node = Calloc(sizeof(ElementCallInfo));
    Node_Add(gElementState.head, node);
    node->geo = geo;
    node->split = split;
    node->func = func;
    node->arg = arg;
    node->update = true;
    
    if (gElementState.forceDisable)
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

#define ELEM_PRESS_CONDITION(this) Element_PressCondition(gElementState.geo, gElementState.split, &(this)->element)

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

void Element_SetContext(GeoGrid* setGeo, Split* setSplit) {
    gElementState.split = setSplit;
    gElementState.geo = setGeo;
}

// # # # # # # # # # # # # # # # # # # # #
// #                                     #
// # # # # # # # # # # # # # # # # # # # #

#define ELEMENT_QUEUE_CHECK() if (this->element.disabled || gElementState.geo->state.blockElemInput) \
    goto queue_element;
#define ELEMENT_QUEUE(draw)   goto queue_element; queue_element: \
    Element_QueueElement(gElementState.geo, gElementState.split, \
        draw, this, __FUNCTION__);

// # # # # # # # # # # # # # # # # # # # #
// #                                     #
// # # # # # # # # # # # # # # # # # # # #

static void Element_ButtonDraw(ElementCallInfo* info) {
    void* vg = info->geo->vg;
    ElButton* this = info->arg;
    Rect r = this->element.rect;
    
    Gfx_DrawRounderOutline(vg, r, this->element.light);
    Gfx_DrawRounderRect(vg, r, Theme_Mix(0.10, this->element.base, this->element.light));
    
    if (this->icon) {
        r.x += SPLIT_ELEM_X_PADDING;
        r.w -= SPLIT_ELEM_X_PADDING;
        nvgBeginPath(vg);
        nvgFillColor(vg, this->element.texcol);
        Gfx_Vector(vg, Math_Vec2f_New(r.x, r.y + 2), 1.0f, 0, this->icon);
        nvgFill(vg);
        r.x += 14;
        r.w -= 14;
    }
    
    if (this->element.name && this->element.rect.w > 8) {
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
                pos.x = r.x + ClampMin((r.w - width) * 0.5, SPLIT_ELEM_X_PADDING);
                break;
        }
        
        nvgFillColor(vg, this->element.texcol);
        nvgText(vg, pos.x, pos.y, txt, NULL);
        nvgResetScissor(vg);
    }
}

s32 Element_Button(ElButton* this) {
    void* vg = gElementState.geo->vg;
    
    Assert(gElementState.geo && gElementState.split);
    
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
        if (Input_GetMouse(gElementState.geo->input, CLICK_L)->press) {
            this->state ^= 1;
            if (this->element.toggle)
                this->element.toggle ^= 0b10;
        }
    }
    
    ELEMENT_QUEUE(Element_ButtonDraw);
    
    return this->state;
}

// # # # # # # # # # # # # # # # # # # # #

static void Element_ColorBoxDraw(ElementCallInfo* info) {
    void* vg = info->geo->vg;
    ElColor* this = info->arg;
    RGB8 color = {
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
    
    ELEMENT_QUEUE_CHECK();
    
    if (this->prop.rgb8) {
        if (ELEM_PRESS_CONDITION(this)) {
            if (Input_GetMouse(gElementState.geo->input, CLICK_L)->press) {
                Rect* rect[] = { &gElementState.split->rect, &gElementState.split->headRect };
                
                ContextMenu_Init(gElementState.geo, &this->prop, this, PROP_COLOR, Rect_AddPos(this->element.rect, *rect[this->element.header]));
            }
        }
    }
    
    ELEMENT_QUEUE(Element_ColorBoxDraw);
}

// # # # # # # # # # # # # # # # # # # # #

static void Element_TextboxDraw(ElementCallInfo* info) {
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
    
    nvgFontFace(vg, "default");
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
    
    if (Split_CursorInRect(split, &this->element.rect) && inputCtx->cursor.clickL.hold) {
        if (this->isHintText) {
            this->isHintText = 2;
            gElementState.posText = 0;
            gElementState.posSel = strlen(this->txt);
        } else {
            if (Input_GetMouse(geo->input, CLICK_L)->press) {
                f32 dist = 400;
                for (char* tempB = txtA; tempB <= txtB; tempB++) {
                    Vec2s glyphPos;
                    f32 res;
                    nvgTextBounds(vg, ccx, 0, txtA, tempB, (f32*)&bound);
                    glyphPos.x = this->element.rect.x + bound.w + SPLIT_TEXT_PADDING - 1;
                    glyphPos.y = this->element.rect.y + bound.h - 1 + SPLIT_TEXT * 0.5;
                    
                    res = Math_Vec2s_DistXZ(split->cursorPos, glyphPos);
                    
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
                    
                    res = Math_Vec2s_DistXZ(split->cursorPos, glyphPos);
                    
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
            gElementState.flickTimer += gTick;
            
            if (gTick && gElementState.flickTimer % 30 == 0)
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

void Element_Textbox(ElTextbox* this) {
    Assert(gElementState.geo && gElementState.split);
    
    ELEMENT_QUEUE_CHECK();
    
    if (ELEM_PRESS_CONDITION(this)) {
        if (Input_GetMouse(gElementState.geo->input, CLICK_L)->press) {
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
    
    ELEMENT_QUEUE(Element_TextboxDraw);
}

// # # # # # # # # # # # # # # # # # # # #

static void Element_TextDraw(ElementCallInfo* info) {
    void* vg = info->geo->vg;
    // Split* split = info->split;
    ElText* this = info->arg;
    
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
    ElText* this = Calloc(sizeof(ElText));
    
    Assert(gElementState.geo && gElementState.split);
    this->element.name = txt;
    this->element.doFree = true;
    
    ELEMENT_QUEUE(Element_TextDraw);
    
    return this;
}

// # # # # # # # # # # # # # # # # # # # #

static void Element_CheckboxDraw(ElementCallInfo* info) {
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

s32 Element_Checkbox(ElCheckbox* this) {
    Assert(gElementState.geo && gElementState.split);
    this->element.rect.w = this->element.rect.h;
    
    ELEMENT_QUEUE_CHECK();
    
    if (ELEM_PRESS_CONDITION(this)) {
        if (Input_GetMouse(gElementState.geo->input, CLICK_L)->press) {
            this->element.toggle ^= 1;
        }
    }
    
    ELEMENT_QUEUE(Element_CheckboxDraw);
    
    return this->element.toggle;
}

// # # # # # # # # # # # # # # # # # # # #

static void Element_SliderDraw(ElementCallInfo* info) {
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
    Assert(gElementState.geo && gElementState.split);
    
    if (this->min == 0.0f && this->max == 0.0f)
        this->max = 1.0f;
    
    ELEMENT_QUEUE_CHECK();
    
    Block(void, SetHold, ()) {
        this->holdState = true;
        gElementState.split->elemBlockMouse++;
    };
    
    Block(void, UnsetHold, ()) {
        if (this->holdState)
            gElementState.split->elemBlockMouse--;
        this->holdState = false;
    };
    
    if (ELEM_PRESS_CONDITION(this) || this->holdState) {
        u32 pos = false;
        
        if (this->isTextbox) {
            this->isTextbox = false;
            if (gElementState.curTextbox == &this->textBox) {
                this->isTextbox = true;
                this->holdState = true;
                
                Element_Textbox(&this->textBox);
                
                return Lerp(this->value, this->min, this->max);
            } else {
                UnsetHold();
                this->isSliding = false;
                this->isTextbox = false;
                Element_Slider_SetValue(this, this->isInt ? Value_Int(this->textBox.txt) : Value_Float(this->textBox.txt));
                
                goto queue_element;
            }
        }
        
        if (Input_GetMouse(gElementState.geo->input, CLICK_L)->press)
            SetHold();
        
        else if (Input_GetMouse(gElementState.geo->input, CLICK_L)->hold && this->holdState) {
            if (gElementState.geo->input->cursor.vel.x) {
                if (this->isSliding == false) {
                    Element_Slider_SetCursorToVal(gElementState.geo, gElementState.split, this);
                } else {
                    if (Input_GetKey(gElementState.geo->input, KEY_LEFT_SHIFT)->hold)
                        this->value += (f32)gElementState.geo->input->cursor.vel.x * 0.0001f;
                    else
                        this->value += (f32)gElementState.geo->input->cursor.vel.x * 0.001f;
                    if (this->min || this->max)
                        this->value = Clamp(this->value, 0.0f, 1.0f);
                    
                    pos = true;
                }
                
                this->isSliding = true;
            }
        } else if (Input_GetMouse(gElementState.geo->input, CLICK_L)->release && this->holdState) {
            if (this->isSliding == false && Split_CursorInRect(gElementState.split, &this->element.rect))
                Element_Slider_SetTextbox(gElementState.split, this);
            
            this->isSliding = false;
        } else
            UnsetHold();
        
        if (Input_GetScroll(gElementState.geo->input)) {
            if (this->isInt) {
                f32 scrollDir = Clamp(Input_GetScroll(gElementState.geo->input), -1, 1);
                f32 valueIncrement = 1.0f / (this->max - this->min);
                s32 value = rint(Remap(this->value, 0.0f, 1.0f, this->min, this->max));
                
                if (Input_GetKey(gElementState.geo->input, KEY_LEFT_SHIFT)->hold)
                    this->value = valueIncrement * value + valueIncrement * 5 * scrollDir;
                
                else
                    this->value = valueIncrement * value + valueIncrement * scrollDir;
            } else {
                if (Input_GetKey(gElementState.geo->input, KEY_LEFT_SHIFT)->hold)
                    this->value += Input_GetScroll(gElementState.geo->input) * 0.1;
                
                else if (Input_GetKey(gElementState.geo->input, KEY_LEFT_ALT)->hold)
                    this->value += Input_GetScroll(gElementState.geo->input) * 0.001;
                
                else
                    this->value += Input_GetScroll(gElementState.geo->input) * 0.01;
            }
        }
        
        if (pos) Element_Slider_SetCursorToVal(gElementState.geo, gElementState.split, this);
    }
    
    ELEMENT_QUEUE(Element_SliderDraw);
    this->value = Clamp(this->value, 0.0f, 1.0f);
    
    if (this->isSliding)
        Cursor_SetCursor(CURSOR_EMPTY);
    
    if (this->isInt)
        return (s32)rint(Lerp(this->value, this->min, this->max));
    else
        return Lerp(this->value, this->min, this->max);
}

// # # # # # # # # # # # # # # # # # # # #

static void Element_ComboDraw(ElementCallInfo* info) {
    ElCombo* this = info->arg;
    GeoGrid* geo = info->geo;
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
    Gfx_Shape(vg, center, 0.95, 0, arrow, ArrayCount(arrow));
    nvgFill(vg);
}

s32 Element_Combo(ElCombo* this) {
    Assert(gElementState.geo && gElementState.split);
    
    ELEMENT_QUEUE_CHECK();
    
    Log("PROP %X", this->prop);
    
    if (this->prop && this->prop->num) {
        if (ELEM_PRESS_CONDITION(this)) {
            s32 scrollY = Clamp(Input_GetScroll(gElementState.geo->input), -1, 1);
            
            if (Input_GetMouse(gElementState.geo->input, CLICK_L)->press) {
                Rect* rect[] = {
                    &gElementState.split->rect,
                    &gElementState.split->headRect
                };
                
                ContextMenu_Init(gElementState.geo, this->prop, this, PROP_ENUM, Rect_AddPos(this->element.rect, *rect[this->element.header]));
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

// # # # # # # # # # # # # # # # # # # # #

static Rect Element_Container_GetPropRect(ElContainer* this, s32 i) {
    Rect r = this->element.rect;
    f32 yOffset = 0;
    
    if (this->prop && this->prop->detach
        && i >= this->detachID) {
        yOffset = SPLIT_TEXT_H * this->detachMul;
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

static void Element_ContainerDraw(ElementCallInfo* info) {
    ElContainer* this = info->arg;
    GeoGrid* geo = info->geo;
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
    for (s32 i = 0; i < prop->num; i++) {
        Rect tr = Element_Container_GetPropRect(this, i);
        
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
        
        Gfx_Text(
            vg, tr, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE,
            this->element.texcol,
            PropList_Get(prop, i)
        );
    }
    nvgResetScissor(vg);
    
    DragItem* drag = &gElementState.dragItem;
    
    if (drag->src == this) {
        Rect r = this->element.rect;
        
        r.y -= 8;
        r.h += 16;
        
        if (Rect_PointIntersect(&r, UnfoldVec2(gElementState.split->cursorPos))) {
            for (s32 i = 0;; i++) {
                Rect r = Element_Container_GetDragRect(this, i);
                
                if (Rect_PointIntersect(&r, UnfoldVec2(gElementState.split->cursorPos))
                    || prop->num == i) {
                    r.y += r.h / 2;
                    r.y--;
                    r.h = 2;
                    
                    Gfx_DrawRounderRect(vg, r, Theme_GetColor(THEME_PRIM, 255, 1.0f));
                    break;
                }
            }
        }
    }
    
}

s32 Element_Container(ElContainer* this) {
    SplitScroll* scroll = &this->scroll;
    
    Assert(gElementState.geo && gElementState.split);
    
    if (this->prop) {
        PropList* prop = this->prop;
        Input* input = gElementState.geo->input;
        Cursor* cursor = &gElementState.geo->input->cursor;
        s32 val = Clamp(cursor->scrollY, -1, 1);
        DragItem* drag = &gElementState.dragItem;
        
        if (this->text) {
            if (&this->textBox == gElementState.curTextbox)
                goto queue_element;
            
            this->text = false;
        }
        
        scroll->max = SPLIT_TEXT_H * this->prop->num - this->element.rect.h;
        scroll->max = ClampMin(scroll->max, 0);
        
        ELEMENT_QUEUE_CHECK();
        
        if (this->heldKey > 0 && Math_Vec2s_DistXZ(cursor->pos, cursor->pressPos) > 8) {
            Log("Drag Item Init");
            DragItem_Init(gElementState.geo, this, this->grabRect, PropList_Get(prop, this->heldKey - 1));
            this->detachID = this->heldKey - 1;
            this->detachMul = 1.0f;
            PropList_Detach(prop, this->heldKey - 1);
            this->heldKey = 0;
        }
        
        if (!ELEM_PRESS_CONDITION(this) &&
            drag->src != this) {
            this->pressed = false;
            
            goto queue_element;
        }
        
        scroll->offset += (SPLIT_TEXT_H) *-val;
        
        if (!Input_GetMouse(input, CLICK_L)->press)
            if (!this->pressed)
                goto queue_element;
        
        if (ELEM_PRESS_CONDITION(this)) {
            if (Input_GetMouse(input, CLICK_L)->dual && prop->num) {
                Log("Rename");
                this->text = true;
                this->textBox.size = 32;
                strcpy(this->textBox.txt, PropList_Get(prop, prop->key));
                prop->list[prop->key] = Realloc(prop->list[prop->key], 32);
                
                goto queue_element;
            }
            
            if (!this->pressed) {
                for (s32 i = 0; i < prop->num; i++) {
                    Rect r = Element_Container_GetPropRect(this, i);
                    
                    if (Rect_PointIntersect(&r, UnfoldVec2(gElementState.split->cursorPos))) {
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
        
        if (DragItem_Release(gElementState.geo, this)) {
            Rect r = this->element.rect;
            
            r.y -= 8;
            r.h += 16;
            
            if (Rect_PointIntersect(&r, UnfoldVec2(gElementState.split->cursorPos))) {
                s32 i = 0;
                
                for (; i < prop->num; i++) {
                    Rect r = Element_Container_GetDragRect(this, i);
                    
                    if (Rect_PointIntersect(&r, UnfoldVec2(gElementState.split->cursorPos)))
                        break;
                }
                
                PropList_Retach(prop, i);
            }
            
            PropList_DestroyDetach(prop);
        } else {
            for (s32 i = 0; i < prop->num; i++) {
                Rect r = Element_Container_GetPropRect(this, i);
                
                if (Rect_PointIntersect(&r, UnfoldVec2(gElementState.split->cursorPos)))
                    PropList_Set(prop, i);
            }
        }
        
    }
    
    ELEMENT_QUEUE(Element_ContainerDraw);
    scroll->offset = Clamp(scroll->offset, 0, scroll->max);
    
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

// # # # # # # # # # # # # # # # # # # # #

static void Element_SeparatorDraw(ElementCallInfo* info) {
    Element* this = info->arg;
    
    Gfx_DrawRounderOutline(info->geo->vg, this->rect, Theme_GetColor(THEME_SHADOW, 255, 1.0f));
    Gfx_DrawRounderRect(info->geo->vg, this->rect, Theme_GetColor(THEME_HIGHLIGHT, 175, 1.0f));
    
    Free(this);
}

void Element_Separator(bool drawLine) {
    Element* this;
    
    Assert(gElementState.geo && gElementState.split);
    
    if (drawLine) {
        this = Calloc(sizeof(*this));
        
        gElementState.rowY += SPLIT_ELEM_X_PADDING;
        
        this->rect.x = SPLIT_ELEM_X_PADDING * 2;
        this->rect.w = gElementState.split->rect.w - SPLIT_ELEM_X_PADDING * 4 - 1;
        this->rect.y = gElementState.rowY - SPLIT_ELEM_X_PADDING * 0.5;
        this->rect.h = 2;
        
        gElementState.rowY += SPLIT_ELEM_X_PADDING;
        
        ELEMENT_QUEUE(Element_SeparatorDraw);
    } else {
        gElementState.rowY += SPLIT_ELEM_X_PADDING * 2;
    }
}

// # # # # # # # # # # # # # # # # # # # #

static void Element_BoxDraw(ElementCallInfo* info) {
    Element* this = info->arg;
    
    Gfx_DrawRounderOutline(info->geo->vg, this->rect, Theme_GetColor(THEME_HIGHLIGHT, 45, 1.0f));
    Gfx_DrawRounderRect(info->geo->vg, this->rect, Theme_GetColor(THEME_SHADOW, 45, 1.0f));
    
    Free(this);
}

s32 Element_Box(BoxInit io) {
    ThreadLocal static Element* boxElemList[42];
    ThreadLocal static f32 boxRowY[42];
    ThreadLocal static s32 r;
    
    if (io == BOX_GET_NUM)
        return r;
    
    #define BOX_PUSH() r++
    #define BOX_POP()  r--
    #define THIS  boxElemList[r]
    #define ROW_Y boxRowY[r]
    
    Assert(gElementState.geo && gElementState.split);
    Assert(r < ArrayCount(boxElemList));
    
    if (io == BOX_START) {
        ROW_Y = gElementState.rowY;
        
        THIS = Calloc(sizeof(*THIS));
        Element_Row(gElementState.split, 1, THIS, 1.0);
        gElementState.rowY = ROW_Y + SPLIT_ELEM_X_PADDING;
        
        // Shrink Split Rect
        gElementState.rowX += SPLIT_ELEM_X_PADDING;
        gElementState.split->rect.w -= SPLIT_ELEM_X_PADDING * 2;
        
        Element_QueueElement(gElementState.geo, gElementState.split,
            Element_BoxDraw, THIS, __FUNCTION__);
        
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
#undef BOX_PUSH
#undef BOX_POP
#undef THIS
#undef ROW_Y
    return 0;
}

// # # # # # # # # # # # # # # # # # # # #

void Element_DisplayName(Element* this, f32 lerp) {
    ElementCallInfo* node;
    s32 w = this->rect.w * lerp;
    
    Assert(gElementState.geo && gElementState.split);
    
    this->posTxt.x = this->rect.x;
    this->posTxt.y = this->rect.y + SPLIT_TEXT_PADDING - 1;
    
    this->rect.x += w + SPLIT_ELEM_X_PADDING * lerp;
    this->rect.w -= w + SPLIT_ELEM_X_PADDING * lerp;
    
    this->dispText = true;
    
    node = Calloc(sizeof(ElementCallInfo));
    Node_Add(gElementState.head, node);
    node->geo = gElementState.geo;
    node->split = gElementState.split;
    node->func = Element_TextDraw;
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
    if (!stricmp(type, "int") || StrStart(type, "s") || StrStart(type, "u"))
        this->isInt = true;
    else if (!stricmp(type, "float") || StrStart(type, "f"))
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

void Element_Combo_SetPropList(ElCombo* this, PropList* prop) {
    this->prop = prop;
}

void Element_Color_SetColor(ElColor* this, void* color) {
    this->prop.rgb8 = color;
}

void Element_Container_SetPropList(ElContainer* this, PropList* prop, u32 num) {
    this->prop = prop;
    this->element.heightAdd = SPLIT_TEXT_H * ClampMin(num - 3, 0);
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
    gElementState.rowY = y;
}

void Element_Row(Split* split, s32 rectNum, ...) {
    f32 x = SPLIT_ELEM_X_PADDING + gElementState.rowX;
    f32 yadd = 0;
    f32 width;
    va_list va;
    
    va_start(va, rectNum);
    
    Log("Setting [%d] Elements for Split ID %d", rectNum, split->id);
    
    for (s32 i = 0; i < rectNum; i++) {
        Element* this = va_arg(va, void*);
        f64 a = va_arg(va, f64);
        
        width = (f32)(split->rect.w - SPLIT_ELEM_X_PADDING * 3) * a;
        
        if (this) {
            Rect* rect = &this->rect;
            Log("[%d]: %s", i, this->name);
            
            if (this->heightAdd && yadd == 0)
                yadd = this->heightAdd;
            
            if (rect) {
                Element_SetRectImpl(
                    rect,
                    x + SPLIT_ELEM_X_PADDING, rint(gElementState.rowY - split->scroll.voffset),
                    width - SPLIT_ELEM_X_PADDING, yadd
                );
            }
        }
        
        x += width;
    }
    
    gElementState.rowY += SPLIT_ELEM_Y_PADDING + yadd;
    
    if (gElementState.rowY >= split->rect.h) {
        split->scroll.enabled = true;
        split->scroll.max = gElementState.rowY - split->rect.h;
    } else
        split->scroll.enabled = false;
    
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
            Element_SetRectImpl(rect, x + SPLIT_ELEM_X_PADDING, 4, w, 0);
        x += w + SPLIT_ELEM_X_PADDING;
    }
    
    va_end(va);
}

// # # # # # # # # # # # # # # # # # # # #
// # Element Main                        #
// # # # # # # # # # # # # # # # # # # # #

static InputType* Textbox_GetKey(GeoGrid* geo, KeyMap key) {
    return &geo->input->key[key];
}

static InputType* Textbox_GetMouse(GeoGrid* geo, CursorClick key) {
    return &geo->input->cursor.clickList[key];
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
            geo->input->state.block++;
        }
        
        if (Textbox_GetMouse(geo, CLICK_ANY)->press || Textbox_GetKey(geo, KEY_ENTER)->press) {
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
            
            if (Textbox_GetKey(geo, KEY_HOME)->press)
                gElementState.posText = 0;
            
            if (Textbox_GetKey(geo, KEY_END)->press)
                gElementState.posText = strlen(gElementState.curTextbox->txt);
            
            if (Textbox_GetKey(geo, KEY_LEFT)->hold || Textbox_GetKey(geo, KEY_RIGHT)->hold)
                gElementState.timerTextbox += gTick;
            else
                gElementState.timerTextbox = 0;
            
            if (gTick && gElementState.timerTextbox >= 30 && (s32)gElementState.timerTextbox % 2 == 0) {
                if (Textbox_GetKey(geo, KEY_LEFT)->hold)
                    gElementState.posText--;
                
                if (Textbox_GetKey(geo, KEY_RIGHT)->hold)
                    gElementState.posText++;
                
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
        } else if (press)
            gElementState.posSel = -1;
        
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
            
            else
                StrIns2(gElementState.curTextbox->txt, txt, gElementState.posText, gElementState.curTextbox->size);
            
            gElementState.posText += strlen(txt);
        }
        
        gElementState.posText = Clamp(gElementState.posText, 0, strlen(gElementState.curTextbox->txt));
    } else {
        if (gElementState.blockerTextbox) {
            gElementState.blockerTextbox--;
            geo->input->state.block--;
        }
    }
}

static void Element_UpdateElement(ElementCallInfo* info) {
    GeoGrid* geo = info->geo;
    Element* this = info->arg;
    Split* split = info->split;
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
    
    if (this == (void*)gElementState.curTextbox || this == geo->dropMenu.element)
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
    ElementCallInfo* elem = gElementState.head;
    Element* this;
    
    while (elem) {
        ElementCallInfo* next = elem->next;
        this = elem->arg;
        
        if (this->header == header && elem->split == split) {
            Log("[%d][Split ID %d] ElemFunc: " PRNT_PRPL "%s", header, split->id, elem->elemFunc);
            
            if (elem->update)
                Element_UpdateElement(elem);
            if (header || !Element_DisableDraw(elem->arg, elem->split))
                elem->func(elem);
            if (this->doFree)
                Free(this);
            Node_Remove(gElementState.head, elem);
            Free(elem);
        }
        
        elem = next;
    }
}

void DragItem_Draw(GeoGrid* geo) {
    DragItem* this = &gElementState.dragItem;
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
    
    this->swing = Clamp(this->swing, -1.25f, 1.25f);
    Math_SmoothStepToF(&this->swing, 0.0f, 0.25f, 0.2f, 0.001f);
    Math_SmoothStepToF(&this->alpha, this->hold ? 200.0f : 0.0f, 0.25f, 80.0f, 0.01f);
    
    nvgBeginFrame(vg, geo->wdim->x, geo->wdim->y, gPixelRatio); {
        f32 scale = Lerp(0.5f, this->alpha / 200.0f, 1.0f - (fabsf(this->swing) / 1.85f));
        nvgTranslate(vg,
            this->pos.x + this->rect.w * 0.5f,
            this->pos.y + this->rect.h * 0.5f);
        nvgScale(vg, scale, scale);
        nvgRotate(vg, this->swing);
        Gfx_DrawRounderOutline(vg, r, Theme_GetColor(THEME_ELEMENT_LIGHT, this->alpha, 1.0f));
        Gfx_DrawRounderRect(vg, r, Theme_GetColor(THEME_ELEMENT_BASE, this->alpha, 1.0f));
        Gfx_Text(
            vg, r, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE,
            Theme_GetColor(THEME_TEXT, this->alpha, 1.0f),
            this->item
        );
    } nvgEndFrame(vg);
    
    if (this->alpha <= EPSILON && this->item) {
        this->alpha = 0.0f;
        Free(this->item);
    }
}
