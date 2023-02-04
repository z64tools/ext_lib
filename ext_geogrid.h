#ifndef EXT_GEOGRID_H
#define EXT_GEOGRID_H
#include <GLFW/glfw3.h>
#include <nanovg/src/nanovg.h>
#include "ext_lib.h"
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
    struct PropList* taskList;
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
    Vec2s cursorPos;           // relative
    Vec2s mousePressPos;
    
    struct {
        bool     useCustomColor : 1;
        bool     useCustomPaint : 1;
        rgb8_t   color;
        NVGpaint paint;
    } bg;
    
    void* instance;
    
    // Incrementable blocker
    s32 elemBlockMouse;
    s32 splitBlockScroll;
    struct {
        bool mouseInSplit    : 1;
        bool mouseInHeader   : 1;
        bool mouseInDispRect : 1;
        bool inputAccess     : 1;
        bool blockMouse      : 1;
    };
    
    u32       isHeader;
    SplitFunc headerFunc;
} Split;

typedef struct {
    char*     taskName;
    SplitFunc init;
    SplitFunc destroy;
    SplitFunc update;
    SplitFunc draw;
    s32       size;
} SplitTask;

typedef enum {
    PROP_ENUM,
    PROP_COLOR,
} PropType;

typedef struct ContextMenu {
    struct Element* element;
    void*    prop;
    PropType type;
    Rect     rectOrigin;
    Rect     rect;
    Vec2s    pos;
    struct {
        bool init                 : 1;
        bool setCondition         : 1;
        bool blockWidthAdjustment : 1;
        s32  up                   : 2;
        s32  side                 : 2;
    } state;
} ContextMenu;

typedef struct GeoGrid {
    Split bar[2];
    Rect  prevWorkRect;
    Rect  workRect;
    
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
    void*  elemState;
    
    struct {
        s32  blockSplitting;
        s32  blockElemInput;
        bool cleanVtx : 1;
    } state;
    struct ContextMenu dropMenu;
    
    struct {
        Split dummySplit;
    } private;
} GeoGrid;

// # # # # # # # # # # # # # # # # # # # #
// # Prop                                #
// # # # # # # # # # # # # # # # # # # # #

typedef enum {
    PROP_SET,
    PROP_GET,
    PROP_ADD,
    PROP_INSERT,
    PROP_REMOVE,
    PROP_DETACH,
    PROP_RETACH,
    PROP_DESTROY_DETACH,
} PropListChange;

struct PropList;
typedef bool (*PropOnChange)(struct PropList*, PropListChange, s32);

typedef struct PropList {
    void*  argument;
    char** list;
    char*  detach;
    s32    max;
    s32    num;
    s32    key;
    s32    visualKey;
    s32    detachKey;
    s32    copyKey;
    bool   copy;
    
    PropOnChange onChange;
    void* udata1;
    void* udata2;
} PropList;

typedef struct PropColor {
    void* argument;
    f32   hue;
    Vec2f pos;
    union {
        rgba8_t* rgba8;
        rgb8_t*  rgb8;
    };
    struct {
        bool updateImg  : 1;
        bool holdLumSat : 1;
        bool holdHue    : 1;
    };
} PropColor;

/*============================================================================*/

typedef struct {
    Vec2f* pos;
    u32    num;
} VectorGfx;

typedef struct {
    bool       __initialized__;
    VectorGfx* folder;
    VectorGfx* cross;
    VectorGfx* arrowParent;
} ElemAssets;

/*============================================================================*/

#ifndef GEO_VECTORGFX_C
extern const ElemAssets gAssets;
#endif

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
    struct {
        bool disabled    : 1;
        bool disableTemp : 1;
        bool hover       : 1;
        bool press       : 1;
        bool dispText    : 1;
        bool header      : 1;
        bool doFree      : 1;
        u32  toggle      : 2;
    };
} Element;

/*============================================================================*/

typedef struct {
    Element    element;
    VectorGfx* icon;
    TextAlign  align;
    s8 state;
    s8 prevResult;
} ElButton;

typedef struct {
    Element   element;
    PropColor prop;
} ElColor;

typedef enum {
    TEXTBOX_STR,
    TEXTBOX_INT,
    TEXTBOX_HEX,
    TEXTBOX_F32,
} TextboxType;

typedef struct {
    Element     element;
    char        txt[256];
    s32         size;
    TextboxType type;
    TextAlign   align;
    
    struct {
        u8  update;
        f32 value;
        f32 min;
        f32 max;
    } val;
    
    Split* split;
    
    struct {
        bool doBlock   : 1;
        bool isClicked : 1;
        bool modified  : 1;
        bool clearIcon : 1;
    };
    s32  ret;
    s32  selA;
    s32  selB;
    s32  selPivot;
    s32  selPos;
    s32  runVal;
    Rect clearRect;
    Rect boxRect;
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
    
    struct {
        bool isInt     : 1;
        bool isSliding : 1;
        bool holdState : 1;
        bool isTextbox : 1;
    };
    
    ElTextbox textBox;
} ElSlider;

typedef struct ElCombo {
    Element   element;
    PropList* prop;
} ElCombo;

typedef struct {
    Element     element;
    PropList*   prop;
    SplitScroll scroll;
    
    struct {
        bool pressed   : 1;
        bool text      : 1;
        bool drag      : 1;
        bool showIndex : 1;
    };
    
    Rect      grabRect;
    ElTextbox textBox;
    
    s32 heldKey;
    s32 detachID;
    f32 detachMul;
    f32 copyLerp;
} ElContainer;

extern Vec2f gZeroVec2f;
extern Vec2s gZeroVec2s;
extern Rect gZeroRect;

VectorGfx VectorGfx_New(VectorGfx* this, const void* data, f32 fidelity);
void VectorGfx_Free(VectorGfx* this);

void Gfx_Shape(void* vg, Vec2f center, f32 scale, s16 rot, const Vec2f* p, u32 num);
void Gfx_Vector(void* vg, Vec2f center, f32 scale, s16 rot, const VectorGfx* gfx);
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
void GeoGrid_Init(GeoGrid* this, struct AppInfo* app, void* passArg);
void GeoGrid_Destroy(GeoGrid* this);
void GeoGrid_Update(GeoGrid* geo);
void GeoGrid_Draw(GeoGrid* geo);

void GeoGrid_Splitless_Start(GeoGrid* geo, Rect r);
void GeoGrid_Splitless_End(GeoGrid* geo);

void ContextMenu_Init(GeoGrid* geo, void* uprop, void* element, PropType type, Rect rect);
void ContextMenu_Draw(GeoGrid* geo);
void ContextMenu_Close(GeoGrid* geo);

void Element_SetActiveTextbox(GeoGrid* geo, Split* split, ElTextbox* this);

s32 Element_Button(ElButton* this);
void Element_Color(ElColor* this);
s32 Element_Textbox(ElTextbox* this);
ElText* Element_Text(const char* txt);
s32 Element_Checkbox(ElCheckbox* this);
f32 Element_Slider(ElSlider* this);
s32 Element_Combo(ElCombo* this);
s32 Element_Container(ElContainer* this);

void Element_Separator(bool drawLine);
typedef enum {
    BOX_START,
    BOX_END,
    BOX_GET_NUM,
} BoxInit;
s32 Element_Box(BoxInit io);
void Element_DisplayName(Element* this, f32 lerp);

void Element_Slider_SetParams(ElSlider* this, f32 min, f32 max, char* type);
void Element_Slider_SetValue(ElSlider* this, f64 val);
void Element_Button_SetValue(ElButton* this, bool toggle, bool state);
void Element_Combo_SetPropList(ElCombo* this, PropList* prop);
void Element_Color_SetColor(ElColor* this, void* color);
void Element_Container_SetPropList(ElContainer* this, PropList* prop, u32 num);

void Element_Name(Element* this, const char* name);
Element* Element_Disable(Element* element);
Element* Element_Enable(Element* element);
Element* Element_Condition(Element* element, s32 condition);
void Element_RowY(f32 y);
void Element_Row(s32 rectNum, ...);
void Element_Header(s32 num, ...);

void Element_UpdateTextbox(GeoGrid* geo);
void Element_Draw(GeoGrid* geo, Split* split, bool header);

#define Element_Name(this, name)        Element_Name(&(this)->element, name)
#define Element_Disable(this)           Element_Disable(&(this)->element)
#define Element_Enable(this)            Element_Enable(&(this)->element)
#define Element_Condition(this, cond)   Element_Condition(&(this)->element, cond)
#define Element_DisplayName(this, lerp) Element_DisplayName(&(this)->element, lerp)
#define Element_Row(...)                Element_Row(NARGS(__VA_ARGS__) / 2, __VA_ARGS__)
#define Element_Header(...)             Element_Header(NARGS(__VA_ARGS__) / 2, __VA_ARGS__)

const char* PropList_Get(PropList* this, s32 i);
void PropList_Set(PropList* this, s32 i);
PropList PropList_Init(s32 defaultVal);
PropList PropList_InitList(s32 def, s32 num, ...);
void PropList_SetOnChangeCallback(PropList* this, PropOnChange onChange, void* udata1, void* udata2);
void PropList_Add(PropList* this, const char* item);
void PropList_Insert(PropList* this, const char* item, s32 slot);
void PropList_Remove(PropList* this, s32 key);
void PropList_Detach(PropList* this, s32 slot, bool copy);
void PropList_Retach(PropList* this, s32 slot);
void PropList_DestroyDetach(PropList* this);
void PropList_Free(PropList* this);

#define PropList_InitList(default, ...) PropList_InitList(default, NARGS(__VA_ARGS__), __VA_ARGS__)

#endif