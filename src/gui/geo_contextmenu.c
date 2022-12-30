#include <ext_geogrid.h>
#include <ext_theme.h>

// # # # # # # # # # # # # # # # # # # # #
// # PropList                            #
// # # # # # # # # # # # # # # # # # # # #

static void ContextProp_List_Init(GeoGrid* geo, ContextMenu* this) {
    PropList* prop = this->prop;
    
    nvgFontFace(geo->vg, "default");
    nvgFontSize(geo->vg, SPLIT_TEXT);
    nvgTextAlign(geo->vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    
    this->rect.h = SPLIT_ELEM_X_PADDING * 2 + SPLIT_TEXT_H * prop->num;
    this->rect.w = 0;
    prop->visualKey = prop->key;
    
    for (s32 i = 0; i < prop->num; i++) {
        this->rect.w = Max(this->rect.w, Gfx_TextWidth(geo->vg, PropList_Get(prop, i)));
        this->rect.w += SPLIT_ELEM_X_PADDING * 2;
    }
    
    printf_info("" PRNT_YELW "%s", __FUNCTION__);
    printf_info("prop->key = %d", prop->key);
    printf_info("prop->num = %d", prop->num);
    
    this->state.setCondition = false;
};

static void ContextProp_List_Draw(GeoGrid* geo, ContextMenu* this) {
    PropList* prop = this->prop;
    Input* input = geo->input;
    Cursor* cursor = &input->cursor;
    f32 height = SPLIT_ELEM_X_PADDING;
    void* vg = geo->vg;
    Rect r = this->rect;
    
    for (s32 i = 0; i < prop->num; i++) {
        const char* item = PropList_Get(prop, i);
        r = this->rect;
        
        r.y = this->rect.y + height;
        r.h = SPLIT_TEXT_H;
        
        if (Rect_PointIntersect(&r, cursor->pos.x, cursor->pos.y) && item) {
            prop->visualKey = i;
            
            if (Input_GetMouse(input, CLICK_L)->release)
                this->state.setCondition = true;
        }
        
        if (i == prop->visualKey) {
            r.x += 4;
            r.w -= 8;
            Gfx_DrawRounderRect(vg, r, Theme_GetColor(THEME_PRIM, 215, 1.0f));
        }
        
        if (item) {
            nvgScissor(vg, UnfoldRect(r));
            nvgFillColor(vg, Theme_GetColor(THEME_TEXT, 255, 1.0f));
            nvgFontFace(vg, "default");
            nvgFontSize(vg, SPLIT_TEXT);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
            nvgText(
                vg,
                this->rect.x + SPLIT_ELEM_X_PADDING,
                this->rect.y + height + SPLIT_ELEM_X_PADDING * 0.5f + 1,
                item,
                NULL
            );
            nvgResetScissor(vg);
        }
        
        height += SPLIT_TEXT_H;
    }
}

// # # # # # # # # # # # # # # # # # # # #
// # PropColor                           #
// # # # # # # # # # # # # # # # # # # # #

static void ContextProp_Init_Draw(GeoGrid* geo, ContextMenu* this) {
    this->rect.h = this->rect.w = 256;
    this->state.blockWidthAdjustment = true;
}

static void ContextProp_Color_Draw(GeoGrid* geo, ContextMenu* this) {
    typedef struct {
        RGBA8* c;
        s32    id;
    } ImgMap;
    
    thread_local static ImgMap imgLumSat;
    thread_local static ImgMap imgHue;
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
    HSL8 color;
    
    rectLumSat.h -= SPLIT_ELEM_Y_PADDING + SPLIT_ELEM_X_PADDING;
    rectHue = rectLumSat;
    rectHue.y += rectHue.h + SPLIT_ELEM_X_PADDING;
    rectHue.h = SPLIT_ELEM_Y_PADDING;
    
    if (this->state.init) {
        color = Color_GetHSL(UnfoldRGB(*prop->rgb8));
        
        prop->hue = color.h;
        prop->pos.x = color.s;
        prop->pos.y = InvF(color.l);
    } else {
        if ((
                Rect_PointIntersect(&rectLumSat, cursor->pos.x, cursor->pos.y) &&
                Input_GetMouse(input, CLICK_L)->press
            ) || ( prop->holdLumSat )) {
            Vec2s relPos = Math_Vec2s_Sub(cursor->pos, (Vec2s) { rectLumSat.x, rectLumSat.y });
            
            prop->pos.x = Clamp((f32)relPos.x / rectLumSat.w, 0, 1);
            prop->pos.y = Clamp((f32)relPos.y / rectLumSat.h, 0, 1);
            prop->holdLumSat = Input_GetMouse(input, CLICK_L)->hold;
        }
        if ((
                Rect_PointIntersect(&rectHue, cursor->pos.x, cursor->pos.y) &&
                Input_GetMouse(input, CLICK_L)->press
            ) || ( prop->holdHue )) {
            Vec2s relPos = Math_Vec2s_Sub(cursor->pos, (Vec2s) { rectHue.x, rectHue.y });
            
            prop->hue = Clamp((f32)relPos.x / rectHue.w, 0, 1);
            prop->holdHue = Input_GetMouse(input, CLICK_L)->hold;
        }
    }
    
    color = (HSL8) { prop->hue, prop->pos.x, InvF(prop->pos.y) };
    *prop->rgb8 = Color_GetRGB8(UnfoldHSL(color));
    
    Block(void, UpdateImg, ()) {
        for (s32 y = 0; y < rectLumSat.h; y++) {
            for (s32 x = 0; x < rectLumSat.w; x++) {
                imgLumSat.c[(y * rectLumSat.w) + x] = Color_GetRGBA8(
                    color.h,
                    (f32)x / rectLumSat.w,
                    (f32)y / rectLumSat.h
                );
            }
        }
        
        nvgUpdateImage(vg, imgLumSat.id, (void*)imgLumSat.c);
    };
    
    if (!imgLumSat.c) {
        imgLumSat.c = qFree(New(RGBA8[rectLumSat.w * rectLumSat.h]));
        UpdateImg();
        imgLumSat.id = nvgCreateImageRGBA(vg, rectLumSat.w, rectLumSat.h, NVG_IMAGE_NEAREST | NVG_IMAGE_FLIPY, (void*)imgLumSat.c);
    }
    
    UpdateImg();
    
    Vec2f pos = Math_Vec2f_Mul(prop->pos, Math_Vec2f_New(rectLumSat.w, rectLumSat.h));
    pos = Math_Vec2f_Add(pos, Math_Vec2f_New(rectLumSat.x, rectLumSat.y));
    
    nvgBeginPath(vg);
    nvgFillPaint(vg, nvgImagePattern(vg, UnfoldRect(rectLumSat), 0, imgLumSat.id, 1.0f));
    nvgRoundedRect(vg, UnfoldRect(rectLumSat), SPLIT_ROUND_R * 2);
    nvgFill(vg);
    
    for (s32 i = 0; i < 2; i++) {
        
        nvgBeginPath(vg);
        if (!i) nvgFillColor(vg, Theme_GetColor(THEME_SHADOW, 255, 1.0f));
        else nvgFillColor(vg, Theme_GetColor(THEME_HIGHLIGHT, 255, 1.0f));
        nvgCircle(vg, pos.x, pos.y, 5 - i);
        nvgFill(vg);
    }
    
    if (!imgHue.c) {
        imgHue.c = qFree(New(RGBA8[rectHue.w * rectHue.h]));
        
        for (s32 y = 0; y < rectHue.h; y++) {
            for (s32 x = 0; x < rectHue.w; x++) {
                imgHue.c[(y * rectHue.w) + x] = Color_GetRGBA8(
                    (f32)x / rectHue.w,
                    1.0f,
                    0.5f
                );
            }
        }
        
        imgHue.id = nvgCreateImageRGBA(vg, rectHue.w, rectHue.h, NVG_IMAGE_NEAREST | NVG_IMAGE_FLIPY, (void*)imgHue.c);
    }
    
    nvgBeginPath(vg);
    nvgFillPaint(vg, nvgImagePattern(vg, UnfoldRect(rectHue), 0, imgHue.id, 0.9f));
    nvgRoundedRect(vg, UnfoldRect(rectHue), SPLIT_ROUND_R);
    nvgFill(vg);
    
    {
        Rect r = rectHue;
        
        r.x += r.w * color.h - 1;
        r.w = 2;
        Gfx_DrawRounderRect(vg, r, nvgHSL(color.h, 0.8f, 0.8f));
        Gfx_DrawRounderOutline(vg, r, Theme_GetColor(THEME_SHADOW, 255, 1.0f));
    }
}

// # # # # # # # # # # # # # # # # # # # #
// # ContextMenu                         #
// # # # # # # # # # # # # # # # # # # # #

#define FUNC_INIT 0
#define FUNC_DRAW 1

void (*sContextMenuFuncs[][2])(GeoGrid*, ContextMenu*) = {
    [PROP_ENUM] =  { ContextProp_List_Init, ContextProp_List_Draw  },
    [PROP_COLOR] = { ContextProp_Init_Draw, ContextProp_Color_Draw },
};

void ContextMenu_Init(GeoGrid* geo, void* uprop, void* element, PropType type, Rect rect) {
    ContextMenu* this = &geo->dropMenu;
    
    Assert(uprop != NULL);
    Assert(type < ArrayCount(sContextMenuFuncs));
    
    this->element = element;
    this->prop = uprop;
    this->type = type;
    this->rectOrigin = rect;
    this->pos = geo->input->cursor.pressPos;
    
    geo->state.blockElemInput++;
    geo->state.blockSplitting++;
    this->pos = geo->input->cursor.pos;
    this->state.up = -1;
    this->state.side = 1;
    this->state.init = true;
    
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
}

void ContextMenu_Draw(GeoGrid* geo) {
    ContextMenu* this = &geo->dropMenu;
    Cursor* cursor = &geo->input->cursor;
    void* vg = geo->vg;
    
    if (this->prop == NULL)
        return;
    
    nvgBeginFrame(geo->vg, geo->wdim->x, geo->wdim->y, gPixelRatio); {
        Rect r = this->rect;
        Gfx_DrawRounderOutline(vg, r, Theme_GetColor(THEME_ELEMENT_LIGHT, 215, 1.0f));
        r.y += this->state.up; r.h++;
        Gfx_DrawRounderRect(vg, r, Theme_GetColor(THEME_ELEMENT_DARK, 215, 1.0f));
        
        sContextMenuFuncs[this->type][FUNC_DRAW](geo, this);
    } nvgEndFrame(geo->vg);
    
    if (this->state.init == true) {
        this->state.init = false;
        
        return;
    }
    
    if (Rect_PointDistance(&this->rect, cursor->pos.x, cursor->pos.y) > 0 &&
        Input_GetMouse(geo->input, CLICK_ANY)->press) {
        ContextMenu_Close(geo);
        return;
    }
    
    if (this->state.setCondition || Rect_PointDistance(&this->rect, cursor->pos.x, cursor->pos.y) > 64) {
        if (this->state.setCondition) {
            switch (this->type) {
                case PROP_ENUM: (void)0;
                    PropList* enm = this->prop;
                    
                    PropList_Set(enm, enm->visualKey);
                    break;
                case PROP_COLOR: (void)0;
                    break;
            }
        }
        
        ContextMenu_Close(geo);
    }
}

void ContextMenu_Close(GeoGrid* geo) {
    ContextMenu* this = &geo->dropMenu;
    
    geo->state.blockElemInput--;
    geo->state.blockSplitting--;
    
    memset(this, 0, sizeof(*this));
}
