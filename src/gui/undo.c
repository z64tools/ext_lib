#include <ext_undo.h>

typedef struct {
    UndoEvent** nodes;
    int max;
    int num;
} Undo;

typedef struct UndoEvent {
    struct UndoEvent* next;
    void*  origin;
    u8*    data;
    size_t size;
    int*   response;
} UndoEvent;

static Undo __instance__;
static Undo* this = &__instance__;

void Undo_Init(u32 max) {
    this->nodes = new(UndoEvent*[max]);
    this->max = max;
}

static void Undo_FreeEvent(UndoEvent* event) {
    if (event->next) Undo_FreeEvent(event->next);
    vfree(event->data, event);
}

static void Undo_Apply(UndoEvent* event) {
    memcpy(event->origin, event->data, event->size);
    
    if (event->next)
        Undo_Apply(event->next);
}

void Undo_Update(Input* input) {
    bool undo = false;
    
    if (Input_GetKey(input, KEY_LEFT_CONTROL)->hold)
        if (Input_GetKey(input, KEY_Z)->press)
            undo = true;
    
    if (this->num && undo) {
        UndoEvent* event = this->nodes[--this->num];
        
        if (event->response)
            *event->response = true;
        Undo_Apply(event);
        Undo_FreeEvent(event);
        this->nodes[this->num] = NULL;
    }
}

void Undo_Destroy() {
    for (int i = 0; i < this->num; i++)
        Undo_FreeEvent(this->nodes[i]);
    
    vfree(this->nodes);
}

UndoEvent* Undo_New() {
    UndoEvent* node = NULL;
    
    if (this->num == this->max) {
        Undo_FreeEvent(this->nodes[0]);
        arrmove_l(this->nodes, 0, this->max);
        this->nodes[this->num - 1] = node = new(UndoEvent);
    } else {
        this->nodes[this->num++] = node = new(UndoEvent);
    }
    
    return node;
}

void Undo_Register(UndoEvent* event, void* origin, size_t size) {
    if (event->data) {
        if (!event->next)
            event->next = new(UndoEvent);
        Undo_Register(event->next, origin, size);
    } else {
        event->origin = origin;
        event->data = memdup(origin, size);
        event->size = size;
    }
}

void Undo_Response(UndoEvent* event, int* dst) {
    event->response = dst;
}
