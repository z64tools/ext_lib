#include <ext_geogrid.h>
#include <ext_theme.h>

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
        
        if (this->pos.y > geo->wdim->y * 0.5) {
            this->rect.y -= this->rect.h + this->rectOrigin.h;
            this->state.up = 1;
        }
    } else {
        this->rect.w = 0;
        nvgFontFace(geo->vg, "default");
        nvgFontSize(geo->vg, SPLIT_TEXT);
        nvgTextAlign(geo->vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        for (s32 i = 0; i < prop->num; i++)
            this->rect.w = Max(this->rect.w, Gfx_TextWidth(geo->vg, prop->get(prop, i)));
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
            nvgFontFace(vg, "default");
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
        geo->wdim->x,
        geo->wdim->y
    );
    nvgBeginFrame(geo->vg, geo->wdim->x, geo->wdim->y, gPixelRatio); {
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
