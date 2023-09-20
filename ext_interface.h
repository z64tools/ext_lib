#ifndef EXT_INTERFACE_H
#define EXT_INTERFACE_H

#include "ext_lib.h"
#include "ext_cursor.h"
#include "ext_input.h"
#include "ext_matrix.h"
#include "ext_view.h"
#include "ext_theme.h"
#include "ext_collision.h"

extern f32 gPixelRatio;
extern f32 gPixelScale;
extern void (*gUiInitFunc)();
extern void (*gUiNanoFunc)(void*);
extern void (*gUiDestFunc)();

typedef enum {
	WIN_MAXIMIZE,
	WIN_MINIMIZE,
} WindowParam;

typedef void (*WindowFunction)(void*);
typedef void (*DropCallback)(GLFWwindow*, s32, char* item[]);

typedef enum {
	APP_RESIZE_CALLBACK = 1 << 0,
	APP_CLOSED          = 1 << 3,
} WindowState;

typedef struct Window {
	char title[64];
	GLFWwindow*    glfw;
	WindowFunction updateCall;
	WindowFunction drawCall;
	Input* input;
	void*  context;
	void*  vg;
	
	Vec2s dim;
	Vec2s bufDim;
	Vec2s prevDim;
	
	WindowState state;
	
	bool tick;
	struct {
		f32 tickMod;
	} private;
} Window;

GLFWwindow* GET_GLFWWINDOW(void);
Window* GET_WINDOW();
void* GET_CONTEXT();
Input* GET_INPUT();

void glViewportRect(s32 x, s32 y, s32 w, s32 h);
void* Window_Init(
	const char* title,
	Window* window,
	Input* input,
	void* context,
	WindowFunction
	updateCall,
	WindowFunction drawCall,
	DropCallback dropCallback,
	u32 x, u32 y,
	u32 samples
);
void Window_Close(Window* window);
void Window_Update(Window* window);
void Window_SetParam(Window* window, u32 num, ...);

#define GUI_INITIALIZE( \
			mainContext, title, \
			width, height, antialias, \
			Update, Draw, DropCallback) \
 \
		(mainContext)->vg = Window_Init( \
			title, \
			&(mainContext)->window, &(mainContext)->input, \
			(mainContext), (void*)Update, \
			(void*)Draw, DropCallback, \
			width, height, antialias \
			);
#define Window_SetParam(window, ...) do { \
			ctassert((NARGS(__VA_ARGS__) % 2) == 0); \
			Window_SetParam(app, NARGS(__VA_ARGS__), __VA_ARGS__); \
} while (0)

#endif
