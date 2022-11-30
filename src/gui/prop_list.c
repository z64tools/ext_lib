#include <ext_geogrid.h>

// # # # # # # # # # # # # # # # # # # # #
// # PropList                            #
// # # # # # # # # # # # # # # # # # # # #

#undef PropList_InitList

const char* PropList_Get(PropList* this, s32 i) {
    char** list = this->list;
    
    if (i >= this->num) {
        Log("OOB Access %d / %d", i, this->num);
        
        return NULL;
    }
    
    if (list == NULL) {
        Log("NULL List");
        
        return NULL;
    }
    
    return list[i];
}

void PropList_Set(PropList* this, s32 i) {
    this->key = Clamp(i, 0, this->num - 1);
    
    if (i != this->key)
        Log("Out of range set!");
}

PropList PropList_Init(s32 defaultVal) {
    PropList this = {};
    
    this.key = defaultVal;
    
    return this;
}

PropList PropList_InitList(s32 def, s32 num, ...) {
    PropList prop = PropList_Init(def);
    va_list va;
    
    va_start(va, num);
    
    for (s32 i = 0; i < num; i++)
        PropList_Add(&prop, va_arg(va, char*));
    
    va_end(va);
    
    return prop;
}

void PropList_SetOnChangeCallback(PropList* this, PropOnChange onChange, void* udata1, void* udata2) {
    this->onChange = onChange;
    this->udata1 = udata1;
    this->udata2 = udata2;
}

#define PROP_ONCHANGE(type, id) this->onChange && !this->onChange(this, type, id)

void PropList_Add(PropList* this, const char* item) {
    if (PROP_ONCHANGE(PROP_ADD, 0))
        return;
    
    this->list = Realloc(this->list, sizeof(char*) * (this->num + 1));
    this->list[this->num++] = StrDup(item);
}

void PropList_Insert(PropList* this, const char* item, s32 slot) {
    if (PROP_ONCHANGE(PROP_INSERT, slot))
        return;
    
    this->list = Realloc(this->list, sizeof(char*) * (this->num + 1));
    this->list[this->num++] = StrDup(item);
    ArrMoveR(this->list, slot, this->num - slot);
}

void PropList_Remove(PropList* this, s32 slot) {
    if (PROP_ONCHANGE(PROP_REMOVE, slot))
        return;
    
    Free(this->list[slot]);
    ArrMoveL(this->list, slot, this->num - slot);
    this->num--;
}

void PropList_Detach(PropList* this, s32 slot) {
    if (PROP_ONCHANGE(PROP_DETACH, slot))
        return;
    
    this->detach = this->list[slot];
    this->list[slot] = NULL;
    ArrMoveL(this->list, slot, this->num - slot);
    this->num--;
}

void PropList_Retach(PropList* this, s32 slot) {
    if (!this->detach) {
        printf_warning("Retach on PropList that is not Detached!\a");
        return;
    }
    
    if (PROP_ONCHANGE(PROP_RETACH, slot))
        return;
    
    this->num++;
    ArrMoveR(this->list, slot, this->num - slot);
    this->list[slot] = this->detach;
    this->detach = NULL;
}

void PropList_DestroyDetach(PropList* this) {
    if (this->detach) {
        PROP_ONCHANGE(PROP_DESTROY_DETACH, 0);
        Free(this->detach);
    }
}

void PropList_Free(PropList* this) {
    for (s32 i = 0; i < this->num; i++)
        Free(this->list[i]);
    Free(this->list);
    Free(this->detach);
}
