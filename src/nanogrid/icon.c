#include <nano_grid.h>
#include <ext_interface.h>
#include <nanovg/src/nanovg.h>

#define GET_LITVAL(val) \
		((val) - 'a')
#define ICON_INDEX(row, column) \
		( \
			(column - 1) + 26 * ( \
				(row > 'z' ? GET_LITVAL(row >> 8) : GET_LITVAL(row)) \
				+ (row > 'z' ? 26 : 0) \
			) \
		)

#define MAX_X 26
#define MAX_Y (ICON_INDEX('da', MAX_X + 1) / MAX_X)

static const int sIconIdTbl[MAX_X * MAX_Y] = {
	#define DEFINE_ICON(icon, bank, val, set) [ICON_INDEX(bank, val)] = icon,
#include "tbl_icon.h"
};
static u8* sIconData[ICON_MAX];

static void Icon_Init() {
	extern DataFile gBlenderIcons;
	
	Svg* vgicon = Svg_New(gBlenderIcons.data, gBlenderIcons.size);
	f32 scale = SPLIT_ICON / 16.0;
	
	for (int y = 0; y < MAX_Y; y++) {
		for (int x = 0; x < MAX_X; x++) {
			int id = x + MAX_X * y;
			
			if (sIconIdTbl[id] == ICON_NONE)
				continue;
			
			Rect* rect = new(Rect);
			*rect = Rect_New(
				5 + (16 + 5) * x,
				10 + (16 + 5) * y,
				16, id);
			
			nested(int, process, (Rect * r)) {
				int id = sIconIdTbl[r->h];
				
				r->h = r->w;
				sIconData[id] = Svg_Rasterize(vgicon, scale, r);
				delete(r);
				
				return 0;
			};
			
			Parallel_Add((void*)process, rect);
		}
	}
	
	Parallel_Exec(sys_getcorenum() * 1.5);
	
	Svg_Delete(vgicon);
}

static void Icon_NanoInit(void* vg) {
	for (int y = 0; y < MAX_Y; y++) {
		for (int x = 0; x < MAX_X; x++) {
			int id = sIconIdTbl[x + MAX_X * y];
			
			if (id == ICON_NONE)
				continue;
			
			int imgid = nvgCreateImageRGBA(vg, SPLIT_ICON, SPLIT_ICON, 0, sIconData[id]);
			osLog("%d -> %d", id, imgid);
			osAssert(id == imgid);
		}
	}
}

static void Icon_Dest() {
	for (int i = 0; i < ArrCount(sIconData); i++)
		delete(sIconData[i]);
}

onlaunch_func_t Icon_Construct() {
	gUiInitFunc = Icon_Init;
	gUiNanoFunc = Icon_NanoInit;
	gUiDestFunc = Icon_Dest;
}
