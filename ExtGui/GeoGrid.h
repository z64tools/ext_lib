#ifndef __Z64GEOGUI_H__
#define __Z64GEOGUI_H__
#include <GLFW/glfw3.h>
#include <nanovg/src/nanovg.h>
#include <ExtLib.h>
#include <ExtGui/Math.h>
#include <ExtGui/Input.h>

extern f32 gPixelRatio;

#define DefineTask(x) (void*)x ## _Init, \
	(void*)x ## _Destroy, \
	(void*)x ## _Update, \
	(void*)x ## _Draw, \
	sizeof(x)

#define SPLIT_GRAB_DIST  4
#define SPLIT_CTXM_DIST  32
#define SPLIT_BAR_HEIGHT 26
#define SPLIT_SPLIT_W    2.0
#define SPLIT_ROUND_R    2.0
#define SPLIT_CLAMP      ((SPLIT_BAR_HEIGHT + SPLIT_SPLIT_W * 1.25) * 2)

#define SPLIT_TEXT_PADDING 4
#define SPLIT_TEXT         12

#define SPLIT_TEXT_H         (SPLIT_TEXT_PADDING * 2 + SPLIT_TEXT)
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
	void* instance;
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
	s32 size;
} SplitTask;

typedef struct {
	u32 noSplit      : 1;
	u32 noClickInput : 1;
} GeoState;

typedef struct GeoGrid {
	StatusBar bar[2];
	Rect prevWorkRect;
	Rect workRect;
	
	struct {
		f64 clampMax;
		f64 clampMin;
	} slide;
	
	Split* actionSplit;
	SplitEdge* actionEdge;
	
	SplitTask* taskTable;
	s32        taskTableNum;
	
	Split*     splitHead;
	SplitVtx*  vtxHead;
	SplitEdge* edgeHead;
	
	Input*     input;
	Vec2s*     winDim;
	void*      vg;
	void*      passArg;
	
	GeoState   state;
} GeoGrid;

// # # # # # # # # # # # # # # # # # # # #
// # Elements                            #
// # # # # # # # # # # # # # # # # # # # #

typedef struct PropEnum {
	void* argument;
	void (* update)(void*);
	struct {
		s32 key;
		const char* name;
	} item[];
} PropEnum;

typedef struct {
	Rect rect;
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
} Element;

typedef struct ElButton {
	Element element;
	u8 isDisabled;
	u8 state;
	u8 hover;
	u8 autoWidth;
} ElButton;

typedef struct {
	Element element;
	char    txt[128];
	s32 size;
	u8  hover      : 1;
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
	char*   txt;
} ElText;

typedef struct {
	Element element;
	f32 lerp;
	u8  hover;
} ElCheckbox;

typedef struct {
	Element element;
	
	f32 vValue; /* Visual Value */
	f32 value;
	f32 min;
	f32 max;
	
	u8  isSliding : 1;
	u8  isInt     : 1;
	u8  hover     : 1;
	u8  holdState : 1;
	u8  locked    : 1;
	
	s32 isTextbox;
	ElTextbox textBox;
} ElSlider;

typedef struct {
	PropEnum*   prop;
	Rect rect;
	const char* name;
} ElCombo;

bool GeoGrid_Cursor_InRect(Split* split, Rect* rect);
bool GeoGrid_Cursor_InSplit(Split* split);
Split* GeoGrid_AddSplit(GeoGrid* geo, const char* name, Rectf32* rect);

s32 Split_Cursor(GeoGrid* geo, Split* split, s32 result);

void GeoGrid_Init(GeoGrid* geo, Vec2s* winDim, Input* input, void* vg);
void GeoGrid_Update(GeoGrid* geo);
void GeoGrid_Draw(GeoGrid* geo);

void Element_Init(GeoGrid* geo);
void Element_Update(GeoGrid* geo);
void Element_Draw(GeoGrid* geo, Split* split);

s32 Element_Button(GeoGrid*, Split*, ElButton*);
void Element_Textbox(GeoGrid*, Split*, ElTextbox*);
f32 Element_Text(GeoGrid* geo, Split* split, ElText* txt);
s32 Element_Checkbox(GeoGrid* geo, Split* split, ElCheckbox* this);
void Element_Slider_SetValue(ElSlider* this, f64 val);
f32 Element_Slider(GeoGrid* geo, Split* split, ElSlider* this);

void Element_RowY(f32 y);
void Element_Row(Split* split, s32 rectNum, ...);

#ifndef __GEO_ELEM_C__
#define Element_Row(split, ...) Element_Row(split, NARGS(__VA_ARGS__) / 2, __VA_ARGS__)
#endif

#endif