#include <ext_geogrid.h>
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

static void ContextProp_Color_Init(GeoGrid* geo, ContextMenu* this) {
    this->rect.h = this->rect.w = Max(this->rect.w, 128 + 64);
    this->data = new(ElTextbox);
}

static void ContextProp_Color_Draw(GeoGrid* geo, ContextMenu* this) {
    PropColor* prop = this->prop;
    void* vg = geo->vg;
    Rect r = this->rect;
    Input* input = geo->input;
    Cursor* cursor = &geo->input->cursor;
    
    r.x += 2;
    r.y += 2;
    r.w -= 4;
    r.h -= 4;
    
    Rect rectLumSat = r;
    Rect rectHue;
    Rect rectButton;
    hsl_t color;
    
    rectLumSat.h -= SPLIT_ELEM_Y_PADDING + SPLIT_ELEM_X_PADDING + SPLIT_ELEM_Y_PADDING + SPLIT_ELEM_X_PADDING;
    rectHue = rectLumSat;
    rectHue.y += rectHue.h + SPLIT_ELEM_X_PADDING;
    rectHue.h = SPLIT_ELEM_Y_PADDING;
    
    rectButton = r;
    rectButton.h = SPLIT_ELEM_Y_PADDING;
    
    if (this->state.init) {
        color = Color_hsl(unfold_rgb(*prop->rgb8));
        
        prop->hue = color.h;
        prop->pos.x = color.s;
        prop->pos.y = invertf(color.l);
    } else {
        if ((
                Rect_PointIntersect(&rectLumSat, cursor->pos.x, cursor->pos.y) &&
                Input_GetMouse(input, CLICK_L)->press
            ) || ( prop->holdLumSat )) {
            Vec2s relPos = Math_Vec2s_Sub(cursor->pos, (Vec2s) { rectLumSat.x, rectLumSat.y });
            
            prop->pos.x = clamp((f32)relPos.x / rectLumSat.w, 0, 1);
            prop->pos.y = clamp((f32)relPos.y / rectLumSat.h, 0, 1);
            prop->holdLumSat = Input_GetMouse(input, CLICK_L)->hold;
        }
        if ((
                Rect_PointIntersect(&rectHue, cursor->pos.x, cursor->pos.y) &&
                Input_GetMouse(input, CLICK_L)->press
            ) || ( prop->holdHue )) {
            Vec2s relPos = Math_Vec2s_Sub(cursor->pos, (Vec2s) { rectHue.x, rectHue.y });
            
            prop->hue = clamp((f32)relPos.x / rectHue.w, 0, 1);
            prop->holdHue = Input_GetMouse(input, CLICK_L)->hold;
        }
    }
    
    color = (hsl_t) { prop->hue, prop->pos.x, invertf(prop->pos.y) };
    *prop->rgb8 = Color_rgb8(unfold_hsl(color));
    
    nested(void, UpdateImg, ()) {
        for (s32 y = 0; y < rectLumSat.h; y++) {
            for (s32 x = 0; x < rectLumSat.w; x++) {
                sColorContext.imgLumSat.c[(y * rectLumSat.w) + x] = Color_rgba8(
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
    
    Vec2f pos = Math_Vec2f_Mul(prop->pos, Math_Vec2f_New(rectLumSat.w, rectLumSat.h));
    pos = Math_Vec2f_Add(pos, Math_Vec2f_New(rectLumSat.x, rectLumSat.y));
    
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
                sColorContext.imgHue.c[(y * rectHue.w) + x] = Color_rgba8(
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
    
    DummySplit_Push(geo, &this->split, geo->workRect);
    Element_Textbox(this->data);
    DummySplit_Pop(geo, &this->split);
}

////////////////////////////////////////////////////////////////////////////////

static void ContextProp_Arli_Init(GeoGrid* geo, ContextMenu* this) {
    Arli* list = this->prop;
    int nullCombo = 0;
    
    nvgFontFace(geo->vg, "default");
    nvgFontSize(geo->vg, SPLIT_TEXT);
    nvgTextAlign(geo->vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    
    this->col = new(ContextColumn);
    this->numCol = 1;
    
    warn("new column");
    for (int i = 0; i < list->num; i++) {
        ContextColumn* col = &this->col[this->numCol - 1];
        
        renew(col->row, ContextRow[++col->numRow]);
        col->row[col->numRow - 1] = (ContextRow) {};
        
        ContextRow* row = &col->row[col->numRow - 1];
        
        row->index = i;
        row->item = list->elemName(list, i);
        warn("new row: %s", row->item);
        if (!row->item) nullCombo++;
        else nullCombo = 0;
        
        if (nullCombo == 2) {
            warn("new column");
            col->numRow -= 2;
            renew(this->col, ContextColumn[++this->numCol]);
            this->col[this->numCol - 1] = (ContextColumn) {};
        }
    }
    
    this->rect.h = SPLIT_ELEM_X_PADDING;
    this->rect.w = SPLIT_ELEM_X_PADDING * 4;
    this->visualKey = list->cur;
    
    warn("calc cols and rows");
    for (int i = 0; i < this->numCol; i++) {
        int width = 0;
        int height = 0;
        
        for (int j = 0; j < this->col[i].numRow; j++) {
            warn("column %d num row %d", i, this->col[i].numRow);
            if (this->col[i].row[j].item) {
                width = Max(width, Gfx_TextWidth(geo->vg, this->col[i].row[j].item));
                height += SPLIT_TEXT_H;
            }
        }
        
        this->col[i].width = width;
        this->rect.w += width + SPLIT_ELEM_X_PADDING;
        this->rect.h = Max(this->rect.h, height + SPLIT_ELEM_X_PADDING * 2);
        
        if (i + 1 < this->numCol)
            this->rect.w += SPLIT_ELEM_X_PADDING * 2;
    }
    
    info("" PRNT_YELW "%s", __FUNCTION__);
    info("list->cur = %d", list->cur);
    info("list->num = %d", list->num);
    
    this->state.setCondition = false;
};

static void ContextProp_Arli_Draw(GeoGrid* geo, ContextMenu* this) {
    Input* input = geo->input;
    Cursor* cursor = &input->cursor;
    void* vg = geo->vg;
    Rect r = this->rect;
    int x = SPLIT_ELEM_X_PADDING * 2;
    
    for (int i = 0; i < this->numCol; i++) {
        ContextColumn* col = &this->col[i];
        r.x = this->rect.x + x;
        r.y = this->rect.y + SPLIT_ELEM_X_PADDING;
        r.w = col->width;
        r.h = SPLIT_TEXT_H;
        
        for (int j = 0; j < col->numRow; j++) {
            if (col->row[j].item) {
                if (Rect_PointIntersect(&r, cursor->pos.x, cursor->pos.y)) {
                    this->visualKey = col->row[j].index;
                    
                    r.x -= SPLIT_ELEM_X_PADDING;
                    r.w += SPLIT_ELEM_X_PADDING * 2;
                    Gfx_DrawRounderRect(vg, r, Theme_GetColor(THEME_PRIM, 215, 1.0f));
                    r.x += SPLIT_ELEM_X_PADDING;
                    r.w -= SPLIT_ELEM_X_PADDING * 2;
                    
                    if (Input_GetMouse(input, CLICK_L)->release)
                        this->state.setCondition = true;
                }
                
                nvgScissor(vg, UnfoldRect(r));
                nvgFillColor(vg, Theme_GetColor(THEME_TEXT, 255, 1.0f));
                nvgFontFace(vg, "default");
                nvgFontSize(vg, SPLIT_TEXT);
                nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
                nvgText(vg, r.x, r.y + SPLIT_ELEM_X_PADDING * 0.5f + 1, col->row[j].item, NULL);
                nvgResetScissor(vg);
                
                r.y += SPLIT_TEXT_H;
            } else {
                Rect rr = { r.x, r.y, r.w, 1 };
                Gfx_DrawRounderRect(vg, rr, Theme_GetColor(THEME_TEXT, 120, 1.0f));
            }
        }
        
        x += col->width + SPLIT_ELEM_X_PADDING;
        
        if (i + 1 < this->numCol) {
            x += SPLIT_ELEM_X_PADDING;
            Rect rr = { this->rect.x + x, this->rect.y, 1, this->rect.h };
            Gfx_DrawRounderRect(vg, rr, Theme_GetColor(THEME_TEXT, 120, 1.0f));
            x += SPLIT_ELEM_X_PADDING * 2;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

#define FUNC_INIT 0
#define FUNC_DRAW 1

void (*sContextMenuFuncs[][2])(GeoGrid*, ContextMenu*) = {
    [CONTEXT_PROP_COLOR] = { ContextProp_Color_Init, ContextProp_Color_Draw },
    [CONTEXT_ARLI] =       { ContextProp_Arli_Init,  ContextProp_Arli_Draw  },
};

void ContextMenu_Init(GeoGrid* geo, void* uprop, void* element, ContextDataType type, Rect rect) {
    ContextMenu* this = &geo->dropMenu;
    
    _log("context init");
    _assert(uprop != NULL);
    _assert(type < ArrCount(sContextMenuFuncs));
    
    this->element = element;
    this->prop = uprop;
    this->type = type;
    this->rect = this->rectOrigin = rect;
    this->pos = geo->input->cursor.pressPos;
    
    geo->state.blockElemInput++;
    geo->state.blockSplitting++;
    this->pos = geo->input->cursor.pos;
    this->state.up = -1;
    this->state.side = 1;
    this->state.init = true;
    
    _log("type %d init func", type);
    sContextMenuFuncs[type][FUNC_INIT](geo, this);
    
    this->rect.y = this->rectOrigin.y + this->rectOrigin.h;
    this->rect.x = this->rectOrigin.x;
    if (!this->state.blockWidthAdjustment)
        this->rect.w = Max(this->rect.w, this->rectOrigin.w);
    
    if (this->pos.y > geo->wdim->y * 0.5) {
        this->rect.y -= this->rect.h + this->rectOrigin.h;
        this->state.up = 1;
    }
    
    if (this->pos.x > geo->wdim->x * 0.5f) {
        this->rect.x -= this->rect.w - this->rectOrigin.w;
        this->state.side = -1;
    }
    
    _log("ok", type);
}

void ContextMenu_Draw(GeoGrid* geo) {
    ContextMenu* this = &geo->dropMenu;
    Cursor* cursor = &geo->input->cursor;
    void* vg = geo->vg;
    
    if (this->prop == NULL)
        return;
    
    nvgBeginFrame(geo->vg, geo->wdim->x, geo->wdim->y, gPixelRatio); {
        Rect r = this->rect;
        Gfx_DrawRounderOutline(vg, r, Theme_GetColor(THEME_ELEMENT_BASE, 255, 1.5f));
        Gfx_DrawRounderRect(vg, r, Theme_GetColor(THEME_ELEMENT_DARK, 255, 1.0f));
        
        sContextMenuFuncs[this->type][FUNC_DRAW](geo, this);
    } nvgEndFrame(geo->vg);
    
    if (this->state.init == true) {
        this->state.init = false;
        
        return;
    }
    
    if (Rect_PointDistance(&this->rect, cursor->pos.x, cursor->pos.y) > 0 && Input_GetMouse(geo->input, CLICK_ANY)->press) {
        ContextMenu_Close(geo);
        
        return;
    }
    
    if (this->state.setCondition || Rect_PointDistance(&this->rect, cursor->pos.x, cursor->pos.y) > 64) {
        if (this->state.setCondition) {
            this->element->contextSet = true;
            
            switch (this->type) {
                case CONTEXT_PROP_COLOR: (void)0;
                    break;
                case CONTEXT_ARLI: (void)0;
                    Arli_Set(this->prop, this->visualKey);
                    break;
            }
        }
        
        ContextMenu_Close(geo);
    }
}

void ContextMenu_Close(GeoGrid* geo) {
    ContextMenu* this = &geo->dropMenu;
    
    warn("free: %s", addr_name(this->col));
    for (int i = 0; i < this->numCol; i++)
        vfree(this->col[i].row);
    vfree(this->col, this->data);
    
    if (sColorContext.imgHue.c) {
        nvgDeleteImage(geo->vg, sColorContext.imgHue.id);
        vfree(sColorContext.imgHue.c);
    }
    if (sColorContext.imgLumSat.c) {
        nvgDeleteImage(geo->vg, sColorContext.imgLumSat.id);
        vfree(sColorContext.imgLumSat.c);
    }
    
    geo->state.blockElemInput--;
    geo->state.blockSplitting--;
    
    memset(this, 0, sizeof(*this));
}
