#ifndef __GET_INC_H__
#define __GET_INC_H__

#include <ExtLib.h>
#include <ExtGui/Cursor.h>
#include <ExtGui/Input.h>
#include <ExtGui/Matrix.h>
#include <ExtGui/View.h>
#include <ExtGui/Theme.h>
#include <ExtGui/GeoGrid.h>

struct ViewerContext;
struct Scene;

typedef void (* CallbackFunc)(void*);
typedef void (* DropCallback)(GLFWwindow*, s32, char* path[]);

typedef struct AppInfo {
	GLFWwindow*  mainWindow;
	CallbackFunc updateCall;
	CallbackFunc drawCall;
	void* context;
	Vec2s winDim;
	Vec2s prevWinDim;
	bool  isResizeCallback;
} AppInfo;

extern InputContext* __inputCtx;
extern AppInfo* __appInfo;

void UI_Init(
	const char* title,
	AppInfo* appInfo,
	InputContext* inputCtx,
	void* context,
	CallbackFunc updateCall,
	CallbackFunc drawCall,
	DropCallback dropCallback,
	u32 x,  u32 y, u32 samples
);
void UI_Draw();
void UI_Main();

#endif
