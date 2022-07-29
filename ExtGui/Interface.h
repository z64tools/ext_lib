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
typedef void (* DropCallback)(GLFWwindow*, s32, char* item[]);

typedef struct AppInfo {
	GLFWwindow*  window;
	CallbackFunc updateCall;
	CallbackFunc drawCall;
	Input* input;
	void*  context;
	Vec2s  winDim;
	Vec2s  prevWinDim;
	bool   isResizeCallback;
} AppInfo;

AppInfo* GetAppInfo(void* window);
void* GetUserCtx(void* window);

extern f64 gFPS;

void* Interface_Init(
	const char* title,
	AppInfo* app,
	Input* input,
	void* context,
	CallbackFunc
	updateCall,
	CallbackFunc drawCall,
	DropCallback dropCallback,
	u32 x, u32 y,
	u32 samples
);
void Interface_Main(AppInfo* app);

#endif
