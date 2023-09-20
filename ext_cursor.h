#ifndef EXT_CURSOR_H
#define EXT_CURSOR_H
#include <GLFW/glfw3.h>
#include <nanovg/src/nanovg.h>
#include "ext_type.h"

typedef struct {
	rgba8_t*  bitmap;
	GLFWimage img;
	void*     glfwCur;
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
	struct Window* window;
	CursorBitmap   cursor[CURSOR_MAX];
	CursorIndex    cursorNow;
	CursorIndex    cursorSet;
	CursorIndex    cursorForce;
} CursorIcon;

void Cursor_CreateCursor(CursorIndex id,  const u8* data, s32 size, s32 xcent, s32 ycent);
void Cursor_Init(CursorIcon* cursor, struct Window*);
void Cursor_Free();
void Cursor_Update();
void Cursor_SetCursor(CursorIndex id);
void Cursor_ForceCursor(CursorIndex id);

#endif
