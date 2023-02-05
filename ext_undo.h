#ifndef EXT_UNDO_H
#define EXT_UNDO_H

#include <ext_lib.h>
#include <ext_input.h>

typedef struct UndoEvent UndoEvent;

void Undo_Init(u32 max);
void Undo_Update(Input* input);
void Undo_Destroy();

UndoEvent* Undo_New();
void Undo_Register(UndoEvent* this, void* origin, size_t size);
void Undo_Response(UndoEvent* event, int* dst);

#ifndef __clang__
#define Undo_RegVar(this, data) \
    Undo_Register(this, data, sizeof(*(data)))
#else
void Undo_RegVar(UndoEvent* this, void* origin);
#endif

#endif