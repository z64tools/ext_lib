#include <ext_interface.h>

// # # # # # # # # # # # # # # # # # # # #
// # PropList                            #
// # # # # # # # # # # # # # # # # # # # #

#define PROP_ONCHANGE(type, id) this->onChange && !this->onChange(this, type, id)

const char* PropList_Get(PropList* this, s32 i) {
    char** list = this->list;
    
    if (PROP_ONCHANGE(PROP_GET, i))
        return NULL;
    
    if (i >= this->num) {
        _log("OOB Access %d / %d", i, this->num);
        
        return NULL;
    }
    
    if (list == NULL) {
        _log("NULL List");
        
        return NULL;
    }
    
    return list[i];
}

void PropList_Set(PropList* this, s32 i) {
    i = clamp(i, 0, this->num - 1);
    
    info("Set: %d", i);
    
    if (PROP_ONCHANGE(PROP_SET, i))
        return;
    
    this->key = i;
    
    if (i != this->key)
        _log("Out of range set!");
}

PropList PropList_Init(s32 defaultVal) {
    PropList this = {};
    
    this.key = defaultVal;
    
    return this;
}

PropList __PropList_InitList(s32 def, s32 num, ...) {
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
    info("" PRNT_GRAY "PropList.Add(" PRNT_RSET "" PRNT_RSET "%s" PRNT_GRAY ");", item);
    
    if (this->max && this->num >= this->max) return;
    if (PROP_ONCHANGE(PROP_ADD, 0))
        return;
    
    this->list = realloc(this->list, sizeof(char*) * (this->num + 2));
    
    void* new = item ? strdup(item) : NULL;
    this->list[this->num++] = new;
    this->list[this->num] = NULL;
}

void PropList_Insert(PropList* this, const char* item, s32 slot) {
    info("" PRNT_GRAY "PropList.Insert(" PRNT_RSET "%d, " PRNT_RSET "%s" PRNT_GRAY ")", slot, item);
    
    if (this->max && this->num >= this->max) return;
    if (PROP_ONCHANGE(PROP_INSERT, slot))
        return;
    
    this->list = realloc(this->list, sizeof(char*) * (this->num + 1));
    this->list[this->num++] = strdup(item);
    arrmove_r(this->list, slot, this->num - slot);
}

void PropList_Remove(PropList* this, s32 slot) {
    info("" PRNT_GRAY "PropList.Remove(" PRNT_RSET "%d"PRNT_GRAY ")", slot);
    if (PROP_ONCHANGE(PROP_REMOVE, slot))
        return;
    
    vfree(this->list[slot]);
    arrmove_l(this->list, slot, this->num - slot);
    this->num--;
}

void PropList_Detach(PropList* this, s32 slot, bool copy) {
    info("" PRNT_GRAY "PropList.Detach(" PRNT_RSET "%d, %B"PRNT_GRAY ")", slot, copy);
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
        warn("Retach on PropList that is not Detached!\a");
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
    
    u8* f = stalloc(u8[this->num]);
    f[this->key] = true;
    
    info_hex("Pre", f, sizeof(u8[this->num]), 0);
    arrmove_l(f, this->detachKey, this->num - this->detachKey);
    arrmove_r(f, slot, this->num - slot);
    info_hex("Post", f, sizeof(u8[this->num]), 0);
    
    arrmove_l(this->list, this->detachKey, this->num - this->detachKey);
    arrmove_r(this->list, slot, this->num - slot);
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
        arrmove_l(this->list, this->detachKey, this->num - this->detachKey);
        this->num--;
        vfree(this->detach);
        
        if (this->key >= this->detachKey)
            PropList_Set(this, this->key - 1);
    }
}

void PropList_Free(PropList* this) {
    for (var i = 0; i < this->num; i++)
        vfree(this->list[i]);
    vfree(this->list);
    vfree(this->detach);
}
