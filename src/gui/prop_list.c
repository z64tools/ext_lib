#include <ext_geogrid.h>

// # # # # # # # # # # # # # # # # # # # #
// # PropList                            #
// # # # # # # # # # # # # # # # # # # # #

#undef PropList_InitList

static char* PropList_Get(PropList* this, s32 i) {
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

static void PropList_Set(PropList* this, s32 i) {
    this->key = Clamp(i, 0, this->num - 1);
    
    if (i != this->key)
        Log("Out of range set!");
}

PropList* PropList_Init(s32 defaultVal) {
    PropList* this = Calloc(sizeof(PropList));
    
    this->get = PropList_Get;
    this->set = PropList_Set;
    this->key = defaultVal;
    
    return this;
}

PropList* PropList_InitList(s32 def, s32 num, ...) {
    PropList* prop = PropList_Init(def);
    va_list va;
    
    va_start(va, num);
    
    for (s32 i = 0; i < num; i++)
        PropList_Add(prop, va_arg(va, char*));
    
    va_end(va);
    
    return prop;
}

void PropList_Add(PropList* this, char* item) {
    this->list = Realloc(this->list, sizeof(char*) * (this->num + 1));
    this->list[this->num++] = StrDup(item);
}

void PropList_Insert(PropList* this, char* item, s32 slot) {
    this->list = Realloc(this->list, sizeof(char*) * (this->num + 1));
    ArrMoveR(this->list, slot, this->num - slot);
    this->list[slot] = StrDup(item);
}

void PropList_Remove(PropList* this, s32 key) {
    Assert(this->list);
    
    this->num--;
    Free(this->list[key]);
    for (s32 i = key; i < this->num; i++)
        this->list[i] = this->list[i + 1];
}

void PropList_Free(PropList* this) {
    if (!this)
        return;
    for (s32 i = 0; i < this->num; i++)
        Free(this->list[i]);
    Free(this->list);
    Free(this);
}
