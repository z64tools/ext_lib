#ifndef __Z64GEOGUI_H__
#define __Z64GEOGUI_H__
#include <GLFW/glfw3.h>
#include <nanovg/src/nanovg.h>
#include <ExtLib.h>
#include <ExtGui/Math.h>
#include <ExtGui/Input.h>

extern f32 gPixelRatio;

#define DefineTask(name, x) { \
		name, \
		(void*)x ## _Init, \
		(void*)x ## _Destroy, \
		(void*)x ## _Update, \
		(void*)x ## _Draw, \
		sizeof(x), \
		NULL }

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
	SPLIT_POINT_NONE = 0,
	SPLIT_POINT_BL   = (1 << 0),
	SPLIT_POINT_TL   = (1 << 1),
	SPLIT_POINT_TR   = (1 << 2),
	SPLIT_POINT_BR   = (1 << 3),
	
	SPLIT_POINTS     = (SPLIT_POINT_TL | SPLIT_POINT_TR | SPLIT_POINT_BL | SPLIT_POINT_BR),
	
	SPLIT_SIDE_L     = (1 << 4),
	SPLIT_SIDE_T     = (1 << 5),
	SPLIT_SIDE_R     = (1 << 6),
	SPLIT_SIDE_B     = (1 << 7),
	
	SPLIT_SIDE_H     = (SPLIT_SIDE_L | SPLIT_SIDE_R),
	SPLIT_SIDE_V     = (SPLIT_SIDE_T | SPLIT_SIDE_B),
	SPLIT_SIDES      = (SPLIT_SIDE_H | SPLIT_SIDE_V),
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
typedef void (* SplitFunc)(void* a, void* b, struct Split* c);

typedef struct SplitVtx {
	struct SplitVtx* prev;
	struct SplitVtx* next;
	Vec2f pos;
	u8    killFlag;
} SplitVtx;

typedef struct SplitEdge {
	struct SplitEdge* prev;
	struct SplitEdge* next;
	SplitVtx* vtx[2];
	f64 pos;
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

typedef struct Split {
	struct Split* prev;
	struct Split* next;
	SplitState    stateFlag;
	SplitEdge*    edge[4];
	SplitVtx* vtx[4];
	Rect  rect; // Absolute XY, relative WH
	Vec2s center;
	Vec2s mousePos; // relative
	Vec2s mousePressPos;
	bool  mouseInSplit;
	bool  mouseInHeader;
	u32   id;
	u32   prevId;
	bool  blockMouse;
	s32   elemBlockMouse;
	struct {
		bool useCustomBG;
		RGB8 color;
	} bg;
	
	const char* name;
} Split;

typedef struct {
	Rect rect;
} StatusBar;

typedef struct {
	char* taskName;
	SplitFunc init;
	SplitFunc destroy;
	SplitFunc update;
	SplitFunc draw;
	s32   size;
	void* instance;
} SplitTask;

typedef struct {
	u32 noSplit;
	u32 noClickInput;
} GeoState;

typedef struct {
	struct Element*  element;
	struct PropEnum* prop;
	Rect  rectOrigin;
	Rect  rect;
	Vec2s pos;
	s32   key;
	struct {
		s32 useOriginRect : 1;
		s32 dirH          : 4;
		s32 dirV          : 4;
		s32 init          : 1;
	} state;
} DropMenu;

typedef struct GeoGrid {
	StatusBar bar[2];
	Rect prevWorkRect;
	Rect workRect;
	
	struct {
		f64 clampMax;
		f64 clampMin;
	} slide;
	
	Split* actionSplit;
	SplitEdge*  actionEdge;
	
	SplitTask** taskTable;
	
	Split*     splitHead;
	SplitVtx*  vtxHead;
	SplitEdge* edgeHead;
	
	Input*     input;
	Vec2s*     winDim;
	void*      vg;
	void*      passArg;
	
	GeoState   state;
	DropMenu   dropMenu;
} GeoGrid;

// # # # # # # # # # # # # # # # # # # # #
// # Elements                            #
// # # # # # # # # # # # # # # # # # # # #

struct PropEnum;

typedef char* (* EnumGet)(struct PropEnum*, s32);
typedef void (* EnumSet)(struct PropEnum*, s32);
typedef void (* EnumFree)(struct PropEnum*);

typedef struct PropEnum {
	void*    argument;
	void*    list;
	EnumGet  get;
	EnumSet  set;
	EnumFree free;
	s32 num;
	s32 key;
} PropEnum;

typedef struct Element {
	Rect        rect;
	Vec2f       posTxt;
	const char* name;
	NVGcolor    prim;
	NVGcolor    shadow;
	NVGcolor    base;
	NVGcolor    light;
	NVGcolor    texcol;
	u32 disabled : 1;
	u32 hover    : 1;
	u32 press    : 1;
	u32 toggle   : 2;
	u32 dispText : 1;
} Element;

typedef struct ElButton {
	Element element;
	u8 state;
	u8 autoWidth;
} ElButton;

typedef struct {
	Element element;
	char    txt[128];
	s32 size;
	u8  isNumBox   : 1;
	u8  isHintText : 2;
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
	f32 lerp;
} ElCheckbox;

typedef struct {
	Element element;
	
	f32 vValue; /* Visual Value */
	f32 value;
	f32 min;
	f32 max;
	
	u8  isSliding : 1;
	u8  isInt     : 1;
	u8  holdState : 1;
	
	s32 isTextbox;
	ElTextbox textBox;
} ElSlider;

typedef struct {
	Element   element;
	PropEnum* prop;
} ElCombo;

bool Split_CursorInRect(Split* split, Rect* rect);
bool Split_CursorInSplit(Split* split);
Split* GeoGrid_AddSplit(GeoGrid* geo, const char* name, Rectf32* rect);

s32 Split_GetCursor(GeoGrid* geo, Split* split, s32 result);

void GeoGrid_Init(GeoGrid* geo, Vec2s* winDim, Input* input, void* vg);
void GeoGrid_Update(GeoGrid* geo);
void GeoGrid_Draw(GeoGrid* geo);

void DropMenu_Init(GeoGrid* geo, bool useOriginRect);
void DropMenu_Update(GeoGrid* geo);
void DropMenu_Draw(GeoGrid* geo);

s32 Element_Button(GeoGrid* geo, Split* split, ElButton* this);
void Element_Textbox(GeoGrid* geo, Split* split, ElTextbox* this);
f32 Element_Text(GeoGrid* geo, Split* split, ElText* this);
s32 Element_Checkbox(GeoGrid* geo, Split* split, ElCheckbox* this);
f32 Element_Slider(GeoGrid* geo, Split* split, ElSlider* this);
void Element_Combo(GeoGrid* geo, Split* split, ElCombo* this);

void Element_Slider_SetParams(ElSlider* this, f32 min, f32 max, char* type);
void Element_Slider_SetValue(ElSlider* this, f64 val);
void Element_Button_SetValue(ElButton* this, bool toggle, bool state);
void Element_Combo_SetPropEnum(ElCombo* this, PropEnum* prop);
void Element_Name(Element* this, const char* name);
void Element_Disable(Element* element);
void Element_Enable(Element* element);
void Element_RowY(f32 y);
void Element_Row(Split* split, s32 rectNum, ...);
void Element_DisplayName(GeoGrid* geo, Split* split, Element* this);

void Element_Update(GeoGrid* geo);
void Element_Draw(GeoGrid* geo, Split* split);

#define Element_Name(el, name)                Element_Name(el.element, name)
#define Element_Disable(el)                   Element_Disable(el.element)
#define Element_Enable(el)                    Element_Enable(el.element)
#define Element_DisplayName(geo, split, this) Element_DisplayName(geo, split, this.element)
#define Element_Row(split, ...)               Element_Row(split, NARGS(__VA_ARGS__) / 2, __VA_ARGS__)

PropEnum* PropEnum_Alloc(
	void* list,
	s32 num,
	s32 def
);

void PropEnum_Free(PropEnum* this);

void PropEnum_ArgCallback(PropEnum* this, void* arg);
void PropEnum_GetCallback(PropEnum* this, EnumGet func);
void PropEnum_SetCallback(PropEnum* this, EnumSet func);
void PropEnum_FreeCallback(PropEnum* this, EnumFree func);

#endif