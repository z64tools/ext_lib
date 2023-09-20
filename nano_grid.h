#ifndef NANO_GRID_H
#define NANO_GRID_H
#include <GLFW/glfw3.h>
#include <nanovg/src/nanovg.h>
#include "ext_lib.h"
#include "ext_math.h"
#include "ext_vector.h"
#include "ext_input.h"

extern f32 gPixelRatio;
extern f32 gPixelScale;

typedef struct Svg Svg;
Svg* Svg_New(const void* mem, size_t size);
void* Svg_Rasterize(Svg* this, f32 scale, Rect* ovrr);
void Svg_Delete(Svg* this);

f32 GetSplitPixel();
f32 GetSplitGrabDist();
f32 GetSplitCtxmDist();
f32 GetSplitBarHeight();
f32 GetSplitSplitW();
f32 GetSplitRoundR();
f32 GetSplitClamp();
f32 GetSplitTextPadding();
f32 GetSplitText();
f32 GetSplitTextH();
f32 GetSplitIconH();
f32 GetSplitElemXPadding();
f32 GetSplitElemYPadding();
f32 GetSplitScrollWidth();
f32 GetSplitScrollPad();
#define SPLIT_PIXEL          GetSplitPixel()
#define SPLIT_GRAB_DIST      GetSplitGrabDist()
#define SPLIT_CTXM_DIST      GetSplitCtxmDist()
#define SPLIT_BAR_HEIGHT     GetSplitBarHeight()
#define SPLIT_SPLIT_W        GetSplitSplitW()
#define SPLIT_ROUND_R        GetSplitRoundR()
#define SPLIT_CLAMP          GetSplitClamp()
#define SPLIT_TEXT_PADDING   GetSplitTextPadding()
#define SPLIT_TEXT           GetSplitText()
#define SPLIT_TEXT_H         GetSplitTextH()
#define SPLIT_ICON           GetSplitIconH()
#define SPLIT_ELEM_X_PADDING GetSplitElemXPadding()
#define SPLIT_ELEM_Y_PADDING GetSplitElemYPadding()
#define SPLIT_SCROLL_WIDTH   GetSplitScrollWidth()
#define SPLIT_SCROLL_PAD     GetSplitScrollPad()

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

struct NanoGrid;
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
		bool dummy          : 1;
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
	void (*saveConfig)(void*, Split*, Toml*, const char*);
	void (*loadConfig)(void*, Split*, Toml*, const char*);
	s32 size;
} SplitTask;

typedef struct {
	f64  cur;
	f64  slotHeight;
	f64  vcur;
	f64  max;
	f64  visMax;
	f64  visNum;
	int  focusSlot;
	bool focusSmooth;
	
	NVGcolor color;
	Rect     baseRect;
	Rect     barOutlineRect;
	Rect     barRect;
	Rect     workRect;
	Vec2s    cursorPos;
	
	s32  holdOffset;
	int  hold;
	bool disabled;
	
	f32 mod;
} ScrollBar;

void ScrollBar_Init(ScrollBar* this, int max, int height);
Rect ScrollBar_GetRect(ScrollBar* this, int slot);
bool ScrollBar_Update(ScrollBar* this, Input* input, Vec2s cursorPos, Rect scrollRect, Rect displayRect);
bool ScrollBar_Draw(ScrollBar* this, void* vg);
void ScrollBar_FocusSlot(ScrollBar* this, int slot, bool smooth);

typedef enum {
	CONTEXT_PROP_COLOR,
	CONTEXT_ARLI,
	CONTEXT_CUSTOM,
} ContextDataType;

typedef struct ContextMenu {
	struct Element* element;
	void* udata;
	ContextDataType type;
	Rect  rectOrigin;
	Rect  rect;
	Vec2s pos;
	
	int magic;
	struct {
		bool init             : 1;
		bool widthAdjustment  : 1;
		bool offsetOriginRect : 1;
		bool rectClamp        : 1;
		bool distanceCheck    : 1;
		bool maximize         : 1;
		int  setCondition     : 2;
		int  up               : 2;
		int  side             : 2;
	} state;
	
	void* temp;
	int   visualKey;
	
	Split     split;
	ScrollBar scroll;
	
	int* quickSelect;
	
	Arli** subMenu;
	int    numSubMenu;
} ContextMenu;

typedef struct NanoGrid {
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
	
	Split*     killSplit;
	Split*     splitHead;
	SplitVtx*  vtxHead;
	SplitEdge* edgeHead;
	
	Input*      input;
	Vec2s*      wdim;
	void*       vg;
	void*       passArg;
	void*       elemState;
	void*       swapState;
	ContextMenu contextMenu;
	
	struct {
		s32  blockSplitting;
		s32  blockElemInput;
		int  cullSplits;
		bool cleanVtx     : 1;
		bool splittedHold : 1;
	} state;
} NanoGrid;

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

typedef struct FileDialog FileDialog;

#pragma GCC diagnostic error "-Wunused-result"

/**
 * action:
 *   'o' - open single file
 *   'm' - open multi file
 *   's' - save single file
 *   'f' - open folder
 */

__attribute__ ((warn_unused_result))
FileDialog* FileDialog_Open  (NanoGrid* nano, char action, const char* basename, const char* filter);
bool FileDialog_Poll(FileDialog* this);
__attribute__ ((warn_unused_result))
List* FileDialog_GetResult(FileDialog** this);

/*============================================================================*/

#ifndef GEO_VECTORGFX_C
extern const ElemAssets gAssets;
#endif

typedef struct Element {
	Rect        rect;
	Vec2s       posTxt;
	const char* name;
	int iconId;
	
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
		bool instantColor : 1;
		
		u32 toggle : 2;
	};
	
	enum {
		ELEM_NAME_STATE_INIT,
		ELEM_NAME_STATE_HIDE,
		ELEM_NAME_STATE_SHOW,
	} nameState;
	
	u8 colOvrdPrim;
	u8 colOvrdShadow;
	u8 colOvrdBase;
	u8 colOvrdLight;
	u8 colOvrdTexcol;
	u8 slotNum;
	enum {
		ELEM_TYPE_NONE,
		ELEM_TYPE_BOX,
		ELEM_TYPE_BUTTON,
		ELEM_TYPE_COLOR,
		ELEM_TYPE_TEXTBOX,
		ELEM_TYPE_CHECKBOX,
		ELEM_TYPE_SLIDER,
		ELEM_TYPE_COMBO,
		ELEM_TYPE_CONTAINER,
	} type;
	
	f32 visFaultMix;
	f32 nameLerp;
	
	NVGcolor prim;
	NVGcolor shadow;
	NVGcolor base;
	NVGcolor light;
	NVGcolor constlight;       // no hover light
	NVGcolor texcol;
} Element;

typedef struct {
	int64_t index;
	enum {
		SET_NONE = 0,
		SET_UNSET,
		SET_CHANGE,
		SET_RENAME,
		SET_EXTERN,
		SET_NEW,
	} state;
} Set;

/*============================================================================*/

typedef struct {
	const char* name;
	Rect rect;
	bool state;
	s16  yaw;
} ElPanel;

typedef struct {
	Element element;
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

#define TEXTBOX_BUFFER_SIZE 512

typedef struct {
	Element       element;
	char          txt[TEXTBOX_BUFFER_SIZE];
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
		bool editing   : 1;
		bool clearIcon : 1;
	};
	int  ret;
	int  selA;
	int  selB;
	int  selPivot;
	int  selPos;
	int  runVal;
	int  charoff;
	int  offset;
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
	Element       element;
	Set           set;
	enum NVGalign align;
	Arli*         list;
	struct {
		bool showDecID  : 1;
		bool showHexID  : 1;
		bool controller : 1;
	};
	const char* title;
	int prevIndex;
} ElCombo;

typedef struct {
	Element   element;
	Set       set;
	Arli*     list;
	ScrollBar scroll;
	
	struct {
		bool text        : 1;
		bool mutable     : 1;
		bool renamable   : 1;
		bool showDecID   : 1;
		bool showHexID   : 1;
		bool controller  : 1;
		bool stretch     : 1;
		bool holdStretch : 1;
	};
	
	ElTextbox textBox;
	ElButton  addButton;
	ElButton  remButton;
	
	Rect  pressRect;
	int   pressKey;
	int64_t prevIndex;
	f32   copyLerp;
	
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
void Gfx_Icon(void* vg, Rect r, NVGcolor col, int icon);
void Gfx_TextShadow(void* vg);
f32 Gfx_TextWidth(void* vg, const char* txt);

bool Split_CursorInRect(Split* split, Rect* rect);
bool Split_CursorInSplit(Split* split);
Split* NanoGrid_AddSplit(NanoGrid* nano, Rectf32* rect, s32 id);

s32 Split_GetCursor(NanoGrid* nano, Split* split, s32 result);

void NanoGrid_Debug(bool b);
void NanoGrid_TaskTable(NanoGrid* nano, SplitTask** taskTable, u32 num);
void NanoGrid_Init(NanoGrid* this, struct Window* app, void* passArg);
void NanoGrid_Destroy(NanoGrid* this);
void NanoGrid_Update(NanoGrid* nano);
void NanoGrid_Draw(NanoGrid* nano);

void DummyGrid_Push(NanoGrid* this);
void DummyGrid_Pop(NanoGrid* this);
void DummySplit_Push(NanoGrid* nano, Split* split, Rect r);
void DummySplit_Pop(NanoGrid* nano, Split* split);
int DummySplit_InputAcces(NanoGrid* nano, Split* split, Rect* r);
int DummyGrid_InputCheck(NanoGrid* nano);

void NanoGrid_SaveLayout(NanoGrid* nano, Toml* toml, const char* file);
int NanoGrid_LoadLayout(NanoGrid* this, Toml* toml);

void ContextMenu_Init(NanoGrid* nano, void* uprop, void* element, ContextDataType type, Rect rect);
void ContextMenu_Custom(NanoGrid* nano, void* context, void* element, void init(NanoGrid*, ContextMenu*), void draw(NanoGrid*, ContextMenu*), void dest(NanoGrid*, ContextMenu*), Rect rect);
void ContextMenu_Draw(NanoGrid* nano, ContextMenu* this);
void ContextMenu_Close(NanoGrid* nano);

void Element_SetActiveTextbox(NanoGrid* nano, Split* split, ElTextbox* this);
void Element_ClearActiveTextbox(NanoGrid* nano);

s32 Element_Button(ElButton* this);
void Element_Color(ElColor* this);
s32 Element_Textbox(ElTextbox* this);
ElText* Element_Text(const char* txt);
s32 Element_Checkbox(ElCheckbox* this);
int Element_Slider(ElSlider* this);
void Element_Separator(bool drawLine);

int Element_Combo(ElCombo* this);
Set* Element_Container(ElContainer* this);

typedef enum {
	BOX_START   = 0,
	BOX_END     = 1,
	BOX_MASK_IO = 1 << 0,
	BOX_INDEX   = 1 << 1,
} BoxState;

int Element_Box(BoxState io, ...);

void Element_Slider_SetParams(ElSlider* this, f32 min, f32 max, char* type);
void Element_Slider_SetValue(ElSlider* this, f32 val);
f32 Element_Slider_GetValue(ElSlider* this);
void Element_Button_SetProperties(ElButton* this, bool toggle, bool state);
void Element_Combo_SetArli(ElCombo* this, Arli* arlist);
void Element_Color_SetColor(ElColor* this, void* color);
void Element_Container_SetArli(ElContainer* this, Arli* prop, u32 num);
bool Element_Textbox_SetText(ElTextbox* this, const char* txt);

void Element_SetName(Element* this, const char* name);
void Element_SetIcon(Element* this, int icon);
Element* Element_Disable(Element* element);
Element* Element_Enable(Element* element);
Element* Element_Condition(Element* element, s32 condition);
void Element_SetNameLerp(Element* this, f32 lerp);
f32 ElAbs(int value);
void Element_RowY(f32 y);
void Element_ShiftX(f32 x);
void Element_Row(s32 rectNum, ...);
void Element_Header(s32 num, ...);
int Element_Operatable(Element* this);

void Element_UpdateTextbox(NanoGrid* nano);
void Element_Draw(NanoGrid* nano, Split* split, bool header);

#ifndef __clang__
#define Element_Box(...) Element_Box(__VA_ARGS__, NULL, NULL)
#endif
#define Element_SetName(this, name)     Element_SetName(&(this)->element, name)
#define Element_SetIcon(this, icon)     Element_SetIcon(&(this)->element, icon)
#define Element_Disable(this)           Element_Disable(&(this)->element)
#define Element_Enable(this)            Element_Enable(&(this)->element)
#define Element_Condition(this, cond)   Element_Condition(&(this)->element, cond)
#define Element_SetNameLerp(this, lerp) Element_SetNameLerp(&(this)->element, lerp)
#define Element_Operatable(this)        Element_Operatable(&(this)->element)
#define Element_Row(...)                Element_Row(NARGS(__VA_ARGS__) / 2, __VA_ARGS__)
#define Element_Header(...)             Element_Header(NARGS(__VA_ARGS__) / 2, __VA_ARGS__)

#include "src/nanogrid/tbl_icon.h"

void osMsg(int icon, const char* fmt, ...);

#endif
