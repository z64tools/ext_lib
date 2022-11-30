#include "ext_interface.h"

CursorIcon* gCursor;

void Cursor_CreateCursor(CursorIndex id,  const u8* data, s32 size, s32 xcent, s32 ycent) {
    CursorBitmap* dest = &gCursor->cursor[id];
    
    gCursor->cursor[id].bitmap = Alloc(sizeof(RGBA8) * size * size);
    
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

void Cursor_Init(CursorIcon* cursor, AppInfo* app) {
    gCursor = cursor;
    cursor->app = app;
    cursor->cursor[CURSOR_DEFAULT].glfwCur = glfwCreateStandardCursor(0);
    cursor->cursorForce = CURSOR_NONE;
}

void Cursor_Update() {
    if (gCursor->cursorForce != CURSOR_NONE) {
        if (gCursor->cursorNow != gCursor->cursorForce) {
            glfwSetCursor(gCursor->app->window, gCursor->cursor[gCursor->cursorForce].glfwCur);
            gCursor->cursorNow = gCursor->cursorForce;
        }
        gCursor->cursorForce = CURSOR_NONE;
        
        return;
    }
    
    if (gCursor->cursorSet != gCursor->cursorNow) {
        glfwSetCursor(gCursor->app->window, gCursor->cursor[gCursor->cursorSet].glfwCur);
        gCursor->cursorNow = gCursor->cursorSet;
    }
}

void Cursor_SetCursor(CursorIndex id) {
    gCursor->cursorSet = id;
}

void Cursor_ForceCursor(CursorIndex id) {
    gCursor->cursorForce = id;
}
