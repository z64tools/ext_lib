#include "Global.h"

static CursorContext* __cursorCtx;

void Cursor_CreateCursor(CursorContext* cursorCtx, CursorIndex id,  const u8* data, s32 size, s32 xcent, s32 ycent) {
	CursorBitmap* dest = &cursorCtx->cursor[id];
	
	cursorCtx->cursor[id].bitmap = Malloc(0, sizeof(RGBA8) * size * size);;
	
	for (s32 i = 0, j = 0; i < size * size; i++, j += 2) {
		dest->bitmap[i].r = data[j];
		dest->bitmap[i].g = data[j];
		dest->bitmap[i].b = data[j];
		dest->bitmap[i].a = data[j + 1];
	}
	
	dest->img.height = dest->img.width = size;
	dest->img.pixels = (void*)dest->bitmap;
	dest->glfwCur = glfwCreateCursor((void*)&dest->img, xcent, ycent);
	Assert(dest->glfwCur != NULL);
}

void Cursor_Init(CursorContext* cursorCtx) {
	cursorCtx->cursor[CURSOR_DEFAULT].glfwCur = glfwCreateStandardCursor(0);
	__cursorCtx = cursorCtx;
	cursorCtx->cursorForce = CURSOR_NONE;
}

void Cursor_Update(CursorContext* cursorCtx) {
	if (cursorCtx->cursorForce != CURSOR_NONE) {
		if (cursorCtx->cursorNow != cursorCtx->cursorForce) {
			glfwSetCursor(__appInfo->mainWindow, __cursorCtx->cursor[cursorCtx->cursorForce].glfwCur);
			cursorCtx->cursorNow = cursorCtx->cursorForce;
		}
		cursorCtx->cursorForce = CURSOR_NONE;
		
		return;
	}
	
	if (cursorCtx->cursorSet != cursorCtx->cursorNow) {
		glfwSetCursor(__appInfo->mainWindow, __cursorCtx->cursor[cursorCtx->cursorSet].glfwCur);
		cursorCtx->cursorNow = cursorCtx->cursorSet;
	}
}

void Cursor_SetCursor(CursorIndex cursorId) {
	__cursorCtx->cursorSet = cursorId;
}

void Cursor_ForceCursor(CursorIndex cursorId) {
	__cursorCtx->cursorForce = cursorId;
}