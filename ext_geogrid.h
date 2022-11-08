#ifndef EXT_GEOGRID_H
#define EXT_GEOGRID_H
#include "ext_lib.h"
#include <GLFW/glfw3.h>
#include <nanovg/src/nanovg.h>
#include "ext_math.h"
#include "ext_vector.h"
#include "ext_input.h"

extern f32 gPixelRatio;

#define SPLIT_GRAB_DIST  4
#define SPLIT_CTXM_DIST  32
#define SPLIT_BAR_HEIGHT 26
#define SPLIT_SPLIT_W    2.0
#define SPLIT_ROUND_R    2.0
#define SPLIT_CLAMP      ((SPLIT_BAR_HEIGHT + SPLIT_SPLIT_W * 1.25) * 2)

#define SPLIT_TEXT_PADDING 4
#define SPLIT_TEXT         12

#define SPLIT_TEXT_H         (SPLIT_TEXT_PADDING + 2 + SPLIT_TEXT)
#define SPLIT_ELEM_X_PADDING (SPLIT_TEXT * 0.5f)
#define SPLIT_ELEM_Y_PADDING (SPLIT_TEXT_H + SPLIT_ELEM_X_PADDING)

typedef enum {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT
} TextAlign;

typedef enum {
    BAR_TOP,
    BAR_BOT
} GuiBarIndex;

typedef enum {
    VTX_BOT_L = 0,
    VTX_TOP_L,
    VTX_TOP_R,
    VTX_BOT_R,
    EDGE_L    = 0,
    EDGE_T,
    EDGE_R,
    EDGE_B
} SplitPos;

typedef enum {
    DIR_L = 0,
    DIR_T,
    DIR_R,
    DIR_B,
} SplitDir;

typedef enum {
    SPLIT_POINT_NONE  = 0,
    SPLIT_POINT_BL    = (1 << 0),
    SPLIT_POINT_TL    = (1 << 1),
    SPLIT_POINT_TR    = (1 << 2),
    SPLIT_POINT_BR    = (1 << 3),
    
    SPLIT_POINTS      = (SPLIT_POINT_TL | SPLIT_POINT_TR | SPLIT_POINT_BL | SPLIT_POINT_BR),
    
    SPLIT_SIDE_L      = (1 << 4),
    SPLIT_SIDE_T      = (1 << 5),
    SPLIT_SIDE_R      = (1 << 6),
    SPLIT_SIDE_B      = (1 << 7),
    
    SPLIT_SIDE_H      = (SPLIT_SIDE_L | SPLIT_SIDE_R),
    SPLIT_SIDE_V      = (SPLIT_SIDE_T | SPLIT_SIDE_B),
    SPLIT_SIDES       = (SPLIT_SIDE_H | SPLIT_SIDE_V),
    
    SPLIT_KILL_DIR_L  = (1 << 4),
    SPLIT_KILL_DIR_T  = (1 << 5),
    SPLIT_KILL_DIR_R  = (1 << 6),
    SPLIT_KILL_DIR_B  = (1 << 7),
    
    SPLIT_KILL_TARGET = SPLIT_KILL_DIR_L | SPLIT_KILL_DIR_T | SPLIT_KILL_DIR_R | SPLIT_KILL_DIR_B,
} SplitState;

typedef enum {
    EDGE_STATE_NONE = 0,
    EDGE_HORIZONTAL = (1 << 0),
    EDGE_VERTICAL   = (1 << 1),
    
    EDGE_ALIGN      = (EDGE_HORIZONTAL | EDGE_VERTICAL),
    
    EDGE_STICK_L    = (1 << 2),
    EDGE_STICK_T    = (1 << 3),
    EDGE_STICK_R    = (1 << 4),
    EDGE_STICK_B    = (1 << 5),
    
    EDGE_STICK      = (EDGE_STICK_L | EDGE_STICK_T | EDGE_STICK_R | EDGE_STICK_B),
    
    EDGE_EDIT       = (1 << 6),
} EdgeState;

struct GeoGrid;
struct Split;
typedef void (*SplitFunc)(void* a, void* b, struct Split* c);

typedef struct SplitVtx {
    struct SplitVtx* next;
    Vec2f pos;
    u8    killFlag;
} SplitVtx;

typedef struct SplitEdge {
    struct SplitEdge* next;
    SplitVtx* vtx[2];
    f64       pos;
    EdgeState state;
    u8 killFlag;
} SplitEdge;

/*
 * Splits are like quad, it's made out of
 * vertices and edges. This enables to keep
 * the Splits and SplitEdges connected for splitting
 * and resizing of Splits
 *
 *  @────────────@ SplitVtx
 *  |            |
 *  |   Split    | SplitEdge
 *  |            |
 *  @────────────@
 *
 */

typedef struct {
    bool enabled;
    f32  voffset;
    s32  offset;
    s32  max;
} SplitScroll;

typedef struct Split {
    struct Split*    next;
    struct PropEnum* taskEnum;
    struct ElCombo*  taskCombo;
    
    u32 id;
    u32 prevId;
    SplitState  state;
    SplitScroll scroll;
    SplitEdge*  edge[4];
    SplitVtx*   vtx[4];
    
    Rect  rect;                // Absolute XY, relative WH
    Rect  headRect;
    Rect  dispRect;
    Vec2s mousePos;            // relative
    Vec2s mousePressPos;
    
    struct {
        u32  useCustomBG : 1;
        RGB8 color;
    } bg;
    
    void* instance;
    
    // Incrementable blocker
    s32 elemBlockMouse;
    struct {
        bool mouseInSplit    : 1;
        bool mouseInHeader   : 1;
        bool mouseInDispRect : 1;
        bool inputAccess     : 1;
        bool blockMouse      : 1;
    };
} Split;

typedef struct {
    Rect rect;
} StatusBar;

typedef struct {
    char*     taskName;
    SplitFunc init;
    SplitFunc destroy;
    SplitFunc update;
    SplitFunc draw;
    s32       size;
} SplitTask;

typedef struct {
    u32 noSplit;
    u32 noClickInput;
    u32 cleanVtx : 1;
} GeoState;

typedef enum {
    PROP_ENUM,
    PROP_COLOR,
} PropType;

typedef struct {
    struct Element* element;
    void*    prop;
    PropType type;
    Rect     rectOrigin;
    Rect     rect;
    Vec2s    pos;
    s32      key;
    struct {
        s32 init         : 1;
        s32 setCondition : 1;
        s32 up           : 2;
    } state;
} DropMenu;

typedef struct GeoGrid {
    StatusBar bar[2];
    Rect      prevWorkRect;
    Rect      workRect;
    
    struct {
        f64 clampMax;
        f64 clampMin;
    } slide;
    
    Split*     actionSplit;
    SplitEdge* actionEdge;
    
    SplitTask** taskTable;
    u32 numTaskTable;
    
    Split*     splitHead;
    SplitVtx*  vtxHead;
    SplitEdge* edgeHead;
    
    Input* input;
    Vec2s* wdim;
    void*  vg;
    void*  passArg;
    
    GeoState state;
    DropMenu dropMenu;
} GeoGrid;

// # # # # # # # # # # # # # # # # # # # #
// # Elements                            #
// # # # # # # # # # # # # # # # # # # # #

struct PropEnum;

typedef struct PropEnum {
    void*  argument;
    char** list;
    char* (*get)(struct PropEnum*, s32);
    void (*set)(struct PropEnum*, s32);
    s32 num;
    s32 key;
} PropEnum;

typedef struct PropColor {
    void* argument;
    void* color;
    char* (*get)(struct PropColor*);
    void (*set)(struct PropColor*);
} PropColor;

typedef struct Element {
    Rect        rect;
    Vec2f       posTxt;
    const char* name;
    NVGcolor    prim;
    NVGcolor    shadow;
    NVGcolor    base;
    NVGcolor    light;
    NVGcolor    texcol;
    u32 heightAdd;
    u32 disabled : 1;
    u32 hover    : 1;
    u32 press    : 1;
    u32 toggle   : 2;
    u32 dispText : 1;
    u32 header   : 1;
    
    u32 __pad;
} Element;

typedef struct {
    Element element;
    u8      state;
    u8      autoWidth;
} ElButton;

typedef struct {
    Element   element;
    char      txt[128];
    s32       size;
    u8        isNumBox   : 1;
    u8        isHintText : 2;
    TextAlign align;
    struct {
        u8  isInt : 1;
        u8  updt  : 1;
        f32 value;
        f32 min;
        f32 max;
    } nbx;
} ElTextbox;

typedef struct {
    Element element;
} ElText;

typedef struct {
    Element element;
    f32     lerp;
} ElCheckbox;

typedef struct {
    Element element;
    
    f32 vValue; /* Visual Value */
    f32 value;
    f32 min;
    f32 max;
    
    u8 isSliding : 1;
    u8 isInt     : 1;
    u8 holdState : 1;
    
    s32       isTextbox;
    ElTextbox textBox;
} ElSlider;

typedef struct ElCombo {
    Element   element;
    PropEnum* prop;
} ElCombo;

typedef struct {
    Element     element;
    PropEnum*   prop;
    SplitScroll scroll;
    
    struct {
        bool pressed : 1;
    };
} ElContainer;

extern Vec2f gZeroVec2f;
extern Vec2s gZeroVec2s;
extern Rect gZeroRect;

void Gfx_Shape(void* vg, Vec2f center, f32 scale, s16 rot, Vec2f* p, u32 num);
void Gfx_DrawRounderOutline(void* vg, Rect rect, NVGcolor color);
void Gfx_DrawRounderRect(void* vg, Rect rect, NVGcolor color);
void Gfx_Text(void* vg, Rect r, enum NVGalign align, NVGcolor col, const char* txt);
f32 Gfx_TextWidth(void* vg, const char* txt);

bool Split_CursorInRect(Split* split, Rect* rect);
bool Split_CursorInSplit(Split* split);
Split* GeoGrid_AddSplit(GeoGrid* geo, Rectf32* rect, s32 id);

s32 Split_GetCursor(GeoGrid* geo, Split* split, s32 result);

void GeoGrid_Debug(bool b);
void GeoGrid_TaskTable(GeoGrid* geo, SplitTask** taskTable, u32 num);
void GeoGrid_Init(GeoGrid* geo, Vec2s* wdim, Input* input, void* vg);
void GeoGrid_Update(GeoGrid* geo);
void GeoGrid_Draw(GeoGrid* geo);

void DropMenu_Init(GeoGrid* geo, void* uprop, PropType type, Rect rect);
void DropMenu_Update(GeoGrid* geo);
void DropMenu_Draw(GeoGrid* geo);

s32 Element_Button(ElButton * this);
void Element_Textbox(ElTextbox * this);
ElText* Element_Text(const char* txt);
s32 Element_Checkbox(ElCheckbox * this);
f32 Element_Slider(ElSlider * this);
s32 Element_Combo(ElCombo * this);
s32 Element_Container(ElContainer * this);

void Element_Separator(bool drawLine);
typedef enum {
    BOX_START,
    BOX_END,
} BoxInit;
void Element_Box(BoxInit io);
void Element_DisplayName(Element * this);

void Element_Slider_SetParams(ElSlider * this, f32 min, f32 max, char* type);
void Element_Slider_SetValue(ElSlider * this, f64 val);
void Element_Button_SetValue(ElButton * this, bool toggle, bool state);
void Element_Combo_SetPropEnum(ElCombo * this, PropEnum * prop);
void Element_Container_SetPropEnumAndHeight(ElContainer * this, PropEnum * prop, u32 num);

void Element_Name(Element * this, const char* name);
void Element_Disable(Element* element);
void Element_Enable(Element* element);
void Element_Condition(Element* element, s32 condition);
void Element_RowY(f32 y);
void Element_Row(Split* split, s32 rectNum, ...);
void Element_Header(Split* split, s32 num, ...);

void Element_Update(GeoGrid* geo);
void Element_Draw(GeoGrid* geo, Split* split, bool header);

#define Element_Name(el, name)      Element_Name(el.element, name)
#define Element_Disable(el)         Element_Disable(el.element)
#define Element_Enable(el)          Element_Enable(el.element)
#define Element_Condition(el, cond) Element_Condition(el.element, cond)
#define Element_DisplayName(this)   Element_DisplayName(this.element)
#define Element_Row(split, ...)     Element_Row(split, NARGS(__VA_ARGS__) / 2, __VA_ARGS__)
#define Element_Header(split, ...)  Element_Header(split, NARGS(__VA_ARGS__) / 2, __VA_ARGS__)

PropEnum* PropEnum_Init(s32 defaultVal);
PropEnum* PropEnum_InitList(s32 def, s32 num, ...);
void PropEnum_Add(PropEnum * this, char* item);
void PropEnum_Insert(PropEnum * this, char* item, s32 slot);
void PropEnum_Remove(PropEnum * this, s32 key);
void PropEnum_Free(PropEnum * this);

#define PropEnum_InitList(default, ...) PropEnum_InitList(default, NARGS(__VA_ARGS__), __VA_ARGS__)

#endif