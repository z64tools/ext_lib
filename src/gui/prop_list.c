#include <ext_interface.h>

// # # # # # # # # # # # # # # # # # # # #
// # PropList                            #
// # # # # # # # # # # # # # # # # # # # #

#undef PropList_InitList
#define PROP_ONCHANGE(type, id) this->onChange && !this->onChange(this, type, id)

const char* PropList_Get(PropList* this, s32 i) {
    char** list = this->list;
    
    if (PROP_ONCHANGE(PROP_GET, i))
        return NULL;
    
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
    i = Clamp(i, 0, this->num - 1);
    
    printf_info("Set: %d", i);
    
    if (PROP_ONCHANGE(PROP_SET, i))
        return;
    
    this->key = i;
    
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
    
    for (var i = 0; i < num; i++)
        PropList_Add(&prop, va_arg(va, char*));
    
    va_end(va);
    
    return prop;
}

void PropList_SetOnChangeCallback(PropList* this, PropOnChange onChange, void* udata1, void* udata2) {
    this->onChange = onChange;
    this->udata1 = udata1;
    this->udata2 = udata2;
}

void PropList_Add(PropList* this, const char* item) {
    if (this->max && this->num >= this->max) return;
    if (PROP_ONCHANGE(PROP_ADD, 0))
        return;
    
    this->list = Realloc(this->list, sizeof(char*) * (this->num + 2));
    this->list[this->num++] = StrDup(item);
    this->list[this->num] = NULL;
}

void PropList_Insert(PropList* this, const char* item, s32 slot) {
    if (this->max && this->num >= this->max) return;
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

void PropList_Detach(PropList* this, s32 slot, bool copy) {
    if (PROP_ONCHANGE(PROP_DETACH, slot))
        return;
    
    this->detachKey = slot;
    this->detach = this->list[slot];
    
    if (copy) {
        this->copy = true;
        this->detachKey = this->num;
        this->copyKey = slot;
    }
}

void PropList_Retach(PropList* this, s32 slot) {
    if (!this->detach) {
        printf_warning("Retach on PropList that is not Detached!\a");
        return;
    }
    
    if (this->copy) {
        var in = 0;
        const char* name;
        
        while (true) {
            var found = false;
            name = x_fmt("%s.%03d", this->detach, in++);
            
            for (var i = 0; i < this->num; i++) {
                if (!strcmp(this->list[i], name))
                    found = true;
            }
            
            if (!found)
                break;
        }
        
        PropList_Insert(this, name, slot);
        this->detach = NULL;
        this->copy = false;
        return;
    }
    
    if (PROP_ONCHANGE(PROP_RETACH, slot))
        return;
    
    u8* f = Stalloc(u8[this->num]);
    f[this->key] = true;
    
    printf_hex("Pre", f, sizeof(u8[this->num]), 0);
    ArrMoveL(f, this->detachKey, this->num - this->detachKey);
    ArrMoveR(f, slot, this->num - slot);
    printf_hex("Post", f, sizeof(u8[this->num]), 0);
    
    ArrMoveL(this->list, this->detachKey, this->num - this->detachKey);
    ArrMoveR(this->list, slot, this->num - slot);
    this->detach = NULL;
    
    for (var i = 0; i < this->num; i++) {
        if (f[i] == true) {
            PropList_Set(this, i);
            break;
        }
    }
}

void PropList_DestroyDetach(PropList* this) {
    if (this->detach) {
        if (this->copy) {
            this->detach = NULL;
            this->copy = false;
            return;
        }
        
        if (PROP_ONCHANGE(PROP_DESTROY_DETACH, this->detachKey))
            return;
        
        this->list[this->detachKey] = NULL;
        ArrMoveL(this->list, this->detachKey, this->num - this->detachKey);
        this->num--;
        Free(this->detach);
        
        if (this->key >= this->detachKey)
            PropList_Set(this, this->key - 1);
    }
}

void PropList_Free(PropList* this) {
    for (var i = 0; i < this->num; i++)
        Free(this->list[i]);
    Free(this->list);
    Free(this->detach);
}
