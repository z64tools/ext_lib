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
} SplitScrollBar;

struct BoxContext;

typedef struct Split {
    struct Split*   next;
    struct ElCombo* taskCombo;
    Arli* taskList;
    
    u32 id;
    u32 prevId;
    SplitState     state;
    SplitScrollBar scroll;
    SplitEdge*     edge[4];
    SplitVtx*      vtx[4];
    
    Rect  rect; // Absolute XY, relative WH
    Rect  headRect;
    Rect  dispRect;
    Vec2s cursorPos; // relative
    Vec2s cursorPressPos;
    
    struct {
        bool     useCustomColor : 1;
        bool     useCustomPaint : 1;
        rgb8_t   color;
        NVGpaint paint;
    } bg;
    
    void* instance;
    
    // Incrementable blocker
    s32 elemBlockCursor;
    s32 splitBlockScroll;
    struct {
        bool cursorInSplit  : 1;
        bool cursorInHeader : 1;
        bool cursorInDisp   : 1;
        bool inputAccess    : 1;
        bool blockCursor    : 1;
    };
    
    u32       isHeader;
    SplitFunc headerFunc;
    struct BoxContext* boxContext;
} Split;

typedef struct {
    char*     taskName;
    SplitFunc init;
    SplitFunc destroy;
    SplitFunc update;
    SplitFunc draw;
    void (*saveConfig)(void*, Split*, Toml*, const char*);
    void (*loadConfig)(void*, Split*, Toml*, const char*);
    s32 size;
} SplitTask;

typedef struct {
    f32      cur;
    f32      vcur;
    NVGcolor color;
    Rect     mrect;
    Rect     rect;
    Rect     srect;
    Vec2s    cursorPos;
    
    f32  slotHeight;
    f32  max;
    f32  visMax;
    s32  holdOffset;
    int  hold;
    bool disabled;
} ScrollBar;

void ScrollBar_Init(ScrollBar* this, int max, f32 height);
Rect ScrollBar_GetRect(ScrollBar* this, int slot);
bool ScrollBar_Update(ScrollBar* this, Input* input, Vec2s cursorPos, Rect r);
bool ScrollBar_Draw(ScrollBar* this, void* vg);
void ScrollBar_FocusSlot(ScrollBar* this, Rect r, int slot);

typedef enum {
    CONTEXT_PROP_COLOR,
    CONTEXT_ARLI,
} ContextDataType;

typedef struct ContextMenu {
    struct Element* element;
    void* prop;
    ContextDataType type;
    Rect  rectOrigin;
    Rect  rect;
    Vec2s pos;
    struct {
        bool init                 : 1;
        bool setCondition         : 1;
        bool blockWidthAdjustment : 1;
        s32  up                   : 2;
        s32  side                 : 2;
    } state;
    
    void* data;
    int   visualKey;
    
    Split     split;
    ScrollBar scroll;
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
        bool cleanVtx     : 1;
        bool first        : 1;
        bool splittedHold : 1;
    } state;
    ContextMenu dropMenu;
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
    NVGcolor    constlight; // no hover light
    NVGcolor    texcol;
    u32 heightAdd;
    f32 visFaultMix;
    struct {
        bool disabled     : 1;
        bool disableTemp  : 1;
        bool hover        : 1;
        bool press        : 1;
        bool dispText     : 1;
        bool header       : 1;
        bool doFree       : 1;
        bool contextSet   : 1;
        bool faulty       : 1;
        u32  toggle       : 2;
        bool instantColor : 1;
    };
    
    int colOvrdPrim;
    int colOvrdShadow;
    int colOvrdBase;
    int colOvrdLight;
    int colOvrdTexcol;
} Element;

/*============================================================================*/

typedef struct {
    const char* name;
    Rect rect;
    bool state;
    s16  yaw;
} ElPanel;

typedef struct {
    Element    element;
    VectorGfx* icon;
    enum NVGalign align;
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
    Element       element;
    char          txt[256];
    s32           size;
    TextboxType   type;
    enum NVGalign align;
    
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
    // Rect boxRect;
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
    Element element;
    enum NVGalign align;
    Arli* arlist;
    bool  showDecID : 1;
    bool  showHexID : 1;
} ElCombo;

typedef struct {
    Element   element;
    Arli*     list;
    Arli*     contextList;
    ScrollBar scroll;
    
    struct {
        bool pressed   : 1;
        bool text      : 1;
        bool beingSet  : 1;
        bool drag      : 1;
        bool showDecID : 1;
        bool showHexID : 1;
    };
    
    Rect      grabRect;
    ElTextbox textBox;
    
    s32 contextKey;
    s32 heldKey;
    f32 detachMul;
    f32 copyLerp;
    
    bool detached;
    enum NVGalign align;
} ElContainer;

typedef struct {
    Element      element;
    int          index;
    const char** name;
    int          num;
} ElTab;

extern Vec2f gZeroVec2f;
extern Vec2s gZeroVec2s;
extern Rect gZeroRect;

VectorGfx VectorGfx_New(VectorGfx* this, const void* data, f32 fidelity);
void VectorGfx_Free(VectorGfx* this);

void Gfx_SetDefaultTextParams(void* vg);
void Gfx_Shape(void* vg, Vec2f center, f32 scale, s16 rot, const Vec2f* p, u32 num);
void Gfx_Vector(void* vg, Vec2f center, f32 scale, s16 rot, const VectorGfx* gfx);
void Gfx_DrawRounderOutline(void* vg, Rect rect, NVGcolor color);
void Gfx_DrawRounderOutlineWidth(void* vg, Rect rect, NVGcolor color, int width);
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

void DummySplit_Push(GeoGrid* geo, Split* split, Rect r);
void DummySplit_Pop(GeoGrid* geo, Split* split);

void GeoGrid_SaveLayout(GeoGrid* geo, Toml* toml, const char* file);
int GeoGrid_LoadLayout(GeoGrid* this, Toml* toml);

void ContextMenu_Init(GeoGrid* geo, void* uprop, void* element, ContextDataType type, Rect rect);
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
    BOX_START   = 0,
    BOX_END     = 1,
    BOX_MASK_IO = 1 << 0,
    BOX_INDEX   = 1 << 1,
} BoxState;

int Element_Box(BoxState io, ...);
void Element_DisplayName(Element* this, f32 lerp);

#ifndef __clang__
#define Element_Box(...) Element_Box(__VA_ARGS__, NULL, NULL)
#endif

void Element_Slider_SetParams(ElSlider* this, f32 min, f32 max, char* type);
void Element_Slider_SetValue(ElSlider* this, f64 val);
void Element_Button_SetValue(ElButton* this, bool toggle, bool state);
void Element_Combo_SetArli(ElCombo* this, Arli* arlist);
void Element_Color_SetColor(ElColor* this, void* color);
void Element_Container_SetArli(ElContainer* this, Arli* prop, u32 num);
bool Element_Textbox_SetText(ElTextbox* this, const char* txt);

void Element_Name(Element* this, const char* name);
Element* Element_Disable(Element* element);
Element* Element_Enable(Element* element);
Element* Element_Condition(Element* element, s32 condition);
void Element_RowY(f32 y);
void Element_ShiftX(f32 x);
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

#endif