#ifndef __Z64CURSOR_H__
#define __Z64CURSOR_H__
#include <GLFW/glfw3.h>
#include <ExtLib.h>
#include <nanovg/src/nanovg.h>

typedef struct {
	RGBA8*    bitmap;
	GLFWimage img;
	void* glfwCur;
} CursorBitmap;

typedef enum {
	CURSOR_NONE    = -1,
	CURSOR_DEFAULT = 0,
	CURSOR_ARROW_L = 1,
	CURSOR_ARROW_U,
	CURSOR_ARROW_R,
	CURSOR_ARROW_D,
	
	CURSOR_ARROW_H,
	CURSOR_ARROW_V,
	
	CURSOR_CROSSHAIR,
	
	CURSOR_EMPTY,
	CURSOR_MAX
} CursorIndex;

typedef struct {
	struct AppInfo* app;
	void* _p;
	CursorBitmap    cursor[64];
	CursorIndex cursorNow;
	CursorIndex cursorSet;
	CursorIndex cursorForce;
} Cursor;

void Cursor_CreateCursor(CursorIndex id,  const u8* data, s32 size, s32 xcent, s32 ycent);
void Cursor_Init(Cursor* cursor, struct AppInfo* app);
void Cursor_Update();
void Cursor_SetCursor(CursorIndex id);
void Cursor_ForceCursor(CursorIndex id);

#endif