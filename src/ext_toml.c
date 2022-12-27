#define EXT_TOML_C
#include "xtoml/x0.h"
#include "xtoml/x0impl.h"
#include <ext_lib.h>

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_STRING,
} Type;

typedef struct {
    toml_table_t* tbl;
    const char*   item;
    toml_array_t* arr;
    s32 idx;
} Travel;

static const char* Toml_GetPathStr(const char* path, const char* elem, const char* next) {
    return x_fmt("" PRNT_GRAY "[ %s"PRNT_YELW "%s"PRNT_GRAY "%s]",
               path ? path : "", elem ? elem : "", next ? next : "");
}

static Travel Toml_Travel(Toml* this, const char* field, toml_table_t* tbl, char* path) {
    const char* elem = x_strndup(field, strcspn(field, "."));
    const char* next = elem ? &field[strlen(elem)] : NULL;
    Travel t = {};
    
    strcat(path, elem);
    
    Block(void, NewTable, (toml_table_t * tbl, const char* elem)) {
        Log("NewTable: %s", elem);
        
        tbl->tab = Realloc(tbl->tab, sizeof(void*) * ++tbl->ntab);
        tbl->tab[tbl->ntab - 1] = New(toml_table_t);
        tbl->tab[tbl->ntab - 1]->key = strdup(elem);
    };
    
    Block(void, NewTblArray, (toml_table_t * tbl, const char* elem)) {
        Log("NewTableArray: %s", elem);
        
        tbl->arr = Realloc(tbl->arr, sizeof(void*) * ++tbl->narr);
        tbl->arr[tbl->narr - 1] = New(toml_array_t);
        tbl->arr[tbl->narr - 1]->key = strdup(elem);
        tbl->arr[tbl->narr - 1]->kind = 't';
        tbl->arr[tbl->narr - 1]->type = 'm';
    };
    
    Block(void, NewTblArrayIndex, (toml_array_t * arr, const char* elem, s32 idx)) {
        idx += 1;
        arr->item = Realloc(arr->item, sizeof(toml_arritem_t) * idx);
        
        for (var i = arr->nitem; i < idx; i++) {
            Log("NewTableArrayIdx: %s [%d]", elem, i);
            arr->item[i] = (toml_arritem_t) { .tab = New(toml_table_t) };
        }
        
        arr->nitem = idx;
    };
    
    Block(void, NewArray, (toml_table_t * tbl, const char* elem)) {
        Log("NewArray: %s", elem);
        tbl->arr = Realloc(tbl->arr, sizeof(void*) * ++tbl->narr);
        tbl->arr[tbl->narr - 1] = New(toml_array_t);
        tbl->arr[tbl->narr - 1]->key = strdup(elem);
        tbl->arr[tbl->narr - 1]->kind = 'v';
        tbl->arr[tbl->narr - 1]->type = 'm';
        tbl->arr[tbl->narr - 1]->nitem = 1;
        tbl->arr[tbl->narr - 1]->item = New(toml_arritem_t);
    };
    
    Block(Travel, Error, (const char* msg)) {
        Travel null = {};
        
        if (!this->silence) {
            printf_warning("" PRNT_REDD "%s" PRNT_RSET " does not exist!", msg);
            printf_error("%s", Toml_GetPathStr(path, elem, next));
        }
        this->success = false;
        
        return null;
    };
    
    if (!next || *next == '\0') {
        if (StrEnd(elem, "]")) {
            const char* arv = StrStr(elem, "[") + 1;
            s32 idx = Value_Int(arv);
            s32 sidx = -1;
            
            if ((arv = StrStr(arv, "["))) {
                arv++;
                sidx = Value_Int(arv);
                
                Swap(idx, sidx);
            }
            
            StrStr(elem, "[")[0] = '\0';
            
            t = (Travel) { .arr = toml_array_in(tbl, elem), .idx = idx };
            
            if (!t.arr) {
                if (this->write) {
                    NewArray(tbl, elem);
                    t.arr = toml_array_in(tbl, elem);
                }
                
                if (!t.arr)
                    return Error("Variable Array");
            }
            
            if (sidx >= 0) {
                toml_array_t* arr = toml_array_at(t.arr, sidx);
                
                Log("%d", arr ? arr->nitem : -1);
                if (!arr) {
                    if (this->write) {
                        toml_array_t* varr = t.arr;
                        u32 num = varr->nitem;
                        
                        varr->item = Realloc(varr->item, sizeof(toml_arritem_t) * (sidx + 1));
                        for (var n = num; n < sidx + 1; n++) {
                            varr->item[n] = (toml_arritem_t) { 0 };
                            varr->item[n].arr = New(toml_array_t);
                            varr->item[n].arr->key = strdup(elem);
                            varr->item[n].arr->type = 'm';
                            varr->item[n].arr->kind = 'v';
                        }
                        varr->nitem = sidx + 1;
                        
                        arr = toml_array_at(t.arr, sidx);
                    }
                    
                    if (!arr)
                        return Error("Sub Array");
                }
                
                t.arr = arr;
                
                return t;
            }
            
            return t;
        }
        
        t =  (Travel) { .tbl = tbl, .item = elem };
        
        if (!t.tbl)
            return Error("Variable");
        
        return t;
    } else next++;
    
    strcat(path, ".");
    toml_table_t* new_tbl;
    
    if (StrEnd(elem, "]")) {
        s32 idx = Value_Int(StrStr(elem, "[") + 1);
        StrStr(elem, "[")[0] = '\0';
        toml_array_t* arr = toml_array_in(tbl, elem);
        
        if (!arr) {
            if (this->write) {
                NewTblArray(tbl, elem);
                arr = toml_array_in(tbl, elem);
            }
            
            if (!arr)
                return Error("Table Array");
        }
        
        new_tbl = toml_table_at(arr, idx);
        
        if (!new_tbl) {
            if (this->write) {
                NewTblArrayIndex(arr, elem, idx);
                
                new_tbl = toml_table_at(arr, idx);
            }
            
            if (!new_tbl)
                return Error("Table Array Index");
        }
        
        return Toml_Travel(this, next, new_tbl, path);
    }
    
    new_tbl = toml_table_in(tbl, elem);
    
    if (!new_tbl) {
        if (this->write) {
            NewTable(tbl, elem);
            new_tbl = toml_table_in(tbl, elem);
        }
        
        if (!new_tbl)
            return Error("Table");
    }
    
    return Toml_Travel(this, next, new_tbl, path);
}

static toml_datum_t Toml_GetValue(Toml* this, const char* item, Type type) {
    this->success = true;
    toml_datum_t (*getVar[])(const toml_table_t*, const char*) = {
        toml_int_in,
        toml_double_in,
        toml_bool_in,
        toml_string_in,
    };
    toml_datum_t (*getArr[])(const toml_array_t*, int) = {
        toml_int_at,
        toml_double_at,
        toml_bool_at,
        toml_string_at,
    };
    
    char path[256] = {};
    Travel t = Toml_Travel(this, item, this->root, path);
    
    if (t.tbl)
        return getVar[type](t.tbl, t.item);
    else if (t.arr)
        return getArr[type](t.arr, t.idx);
    
    return (toml_datum_t) {};
}

void Toml_SetValue(Toml* this, const char* item, const char* fmt, ...) {
    char path[256] = {};
    char value[256];
    va_list va;
    
    va_start(va, fmt);
    vsnprintf(value, 256, fmt, va);
    va_end(va);
    
    this->write = true;
    Travel t = Toml_Travel(this, item, this->root, path);
    this->write = false;
    
    if (t.tbl) {
        toml_table_t* tbl = t.tbl;
        
        Log("SetValue(Tbl): %s = %s", item, value);
        
        if (!toml_key_exists(t.tbl, t.item)) {
            tbl->kval = Realloc(tbl->kval, sizeof(void*) * ++tbl->nkval);
            tbl->kval[tbl->nkval - 1] = Calloc(sizeof(toml_keyval_t));
            
            tbl->kval[tbl->nkval - 1]->key = strdup(t.item);
        }
        
        for (var i = 0; i < tbl->nkval; i++) {
            if (!strcmp(tbl->kval[i]->key, t.item)) {
                Free(tbl->kval[i]->val);
                tbl->kval[i]->val = strdup(value);
            }
        }
    }
    
    if (t.arr) {
        toml_array_t* arr = t.arr;
        
        Log("SetValue(Arr): %s = %s", item, value);
        
        while (arr->nitem <= t.idx) {
            arr->item = Realloc(arr->item, sizeof(toml_arritem_t) * ++arr->nitem);
            arr->item[arr->nitem - 1] = (toml_arritem_t) {};
            arr->item[arr->nitem - 1].val = strdup("0");
            arr->item[arr->nitem - 1].valtype = valtype("0");
            arr->item[arr->nitem - 1].tab = 0;
            arr->item[arr->nitem - 1].arr = 0;
        }
        
        Free(arr->item[t.idx].val);
        arr->item[t.idx].val = strdup(value);
        arr->item[t.idx].valtype = valtype(value);
        arr->item[t.idx].tab = 0;
        arr->item[t.idx].arr = 0;
    }
}

void Toml_AddTable(Toml* this, const char* item, ...) {
    char path[256] = {};
    char table[256];
    va_list va;
    
    va_start(va, item);
    vsnprintf(table, 256, item, va);
    strncat(table, ".temp", 256);
    va_end(va);
    
    this->write = true;
    Toml_Travel(this, table, this->root, path);
    this->write = false;
}

// # # # # # # # # # # # # # # # # # # # #
// # Base                                #
// # # # # # # # # # # # # # # # # # # # #

Toml Toml_New() {
    Toml this = {};
    
    this.root = New(toml_table_t);
    
    return this;
}

void Toml_LoadFile(Toml* this, const char* file) {
    MemFile mem = MemFile_Initialize();
    char errbuf[200];
    
    Log("Parse File: [%s]", file);
    MemFile_LoadFile_String(&mem, file);
    if (!(this->root = toml_parse(mem.str, errbuf, 200))) {
        printf_warning("[Toml Praser Error!]");
        printf_warning("File: %s", file);
        printf_error("%s", errbuf);
    }
    MemFile_Free(&mem);
}

void Toml_Free(Toml* this) {
    toml_free(this->root);
}

void Toml_ToMem(Toml* this, MemFile* mem) {
    MemFile_Free(mem);
    
    Block(void, Parse, (toml_table_t * tbl, char* path, u32 indent)) {
        BlockVar(mem);
        int nkval = toml_table_nkval(tbl);
        int narr = toml_table_narr(tbl);
        int ntab = toml_table_ntab(tbl);
        char* nd = "";
        
        Log("%08X [%s]: %d / %d / %d", tbl, path, tbl->nkval, tbl->narr, tbl->ntab);
        
        if (indent) {
            nd = x_alloc(indent * 4 + 1);
            memset(nd, ' ', indent * 4);
        }
        
        for (var i = 0; i < nkval; i++) {
            Log("Value: %s", toml_key_in(tbl, i));
            
            MemFile_Printf(mem, "%s%-16s = ", nd, toml_key_in(tbl, i));
            MemFile_Printf(mem, "%s\n", toml_raw_in(tbl, toml_key_in(tbl, i)));
        }
        
        for (var i = nkval; i < narr + nkval; i++) {
            Log("Array: %s", toml_key_in(tbl, i));
            
            toml_array_t* arr = toml_array_in(tbl, toml_key_in(tbl, i));
            const char* val;
            var j = 0;
            var k = 0;
            toml_array_t* ar = arr;
            
            switch (toml_array_kind(arr)) {
                case 'a':
                    MemFile_Printf(mem, "%s%-16s = [\n", nd, toml_key_in(tbl, i));
                    
                    ar = toml_array_at(arr, k++);
                    while (ar) {
                        j = 0;
                        MemFile_Printf(mem, "    %s[ ", nd);
                        
                        val = toml_raw_at(ar, j++);
                        while (val) {
                            MemFile_Printf(mem, "%s", val);
                            
                            val = toml_raw_at(ar, j++);
                            if (val)
                                MemFile_Printf(mem, ", ");
                        }
                        MemFile_Printf(mem, " ]");
                        ar = toml_array_at(arr, k++);
                        if (ar)
                            MemFile_Printf(mem, ",");
                        MemFile_Printf(mem, "\n");
                    }
                    MemFile_Printf(mem, "%s]\n", nd);
                    
                    break;
                case 'v':
                    MemFile_Printf(mem, "%s%-16s = [ ", nd, toml_key_in(tbl, i));
                    
                    j = k = 0;
                    
                    val = toml_raw_at(arr, j++);
                    while (val) {
                        MemFile_Printf(mem, "%s", val);
                        val = toml_raw_at(arr, j++);
                        if (val)
                            MemFile_Printf(mem, ", ");
                    }
                    MemFile_Printf(mem, " ]\n");
                    break;
            }
        }
        
        for (var i = nkval; i < narr + nkval; i++) {
            Log("Array: %s", toml_key_in(tbl, i));
            
            toml_array_t* arr = toml_array_in(tbl, toml_key_in(tbl, i));
            var k = 0;
            toml_table_t* tb;
            
            switch (toml_array_kind(arr)) {
                case 't':
                
                    while ((tb = toml_table_at(arr, k++))) {
                        MemFile_Printf(mem, "%s[[%s%s]]\n", nd, path, toml_key_in(tbl, i));
                        Parse(tb, x_fmt("%s%s.", path, toml_key_in(tbl, i)), indent + 1);
                    }
                    
                    break;
            }
        }
        
        for (var i = nkval + narr; i < nkval + narr + ntab; i++) {
            Log("Table: %s", toml_key_in(tbl, i));
            MemFile_Printf(mem, "%s[%s%s]\n", nd, path, toml_key_in(tbl, i));
            
            Parse(toml_table_in(tbl, toml_key_in(tbl, i)), x_fmt("%s%s.", path, toml_key_in(tbl, i)), indent + 1);
        }
    };
    
    Parse(this->root, "", 0);
}

void Toml_SaveFile(Toml* this, const char* file) {
    MemFile mem = MemFile_Initialize();
    
    Toml_ToMem(this, &mem);
    MemFile_SaveFile_String(&mem, file);
    MemFile_Free(&mem);
}

// # # # # # # # # # # # # # # # # # # # #
// # GetValue                            #
// # # # # # # # # # # # # # # # # # # # #

s32 Toml_GetInt(Toml* this, const char* item, ...) {
    char buffer[256];
    va_list va;
    
    va_start(va, item);
    vsnprintf(buffer, 256, item, va);
    toml_datum_t t = Toml_GetValue(this, buffer, TYPE_INT);
    va_end(va);
    
    return t.u.i;
}

f32 Toml_GetFloat(Toml* this, const char* item, ...) {
    char buffer[256];
    va_list va;
    
    va_start(va, item);
    vsnprintf(buffer, 256, item, va);
    toml_datum_t t = Toml_GetValue(this, buffer, TYPE_FLOAT);
    va_end(va);
    
    return t.u.d;
}

bool Toml_GetBool(Toml* this, const char* item, ...) {
    char buffer[256];
    va_list va;
    
    va_start(va, item);
    vsnprintf(buffer, 256, item, va);
    toml_datum_t t = Toml_GetValue(this, buffer, TYPE_BOOL);
    va_end(va);
    
    return t.u.b;
}

char* Toml_GetString(Toml* this, const char* item, ...) {
    char buffer[256];
    va_list va;
    
    va_start(va, item);
    vsnprintf(buffer, 256, item, va);
    toml_datum_t t = Toml_GetValue(this, buffer, TYPE_STRING);
    va_end(va);
    
    if (!t.ok) return NULL;
    
    return t.u.s;
}

char* Toml_GetVar(Toml* this, const char* item, ...) {
    char buffer[256];
    va_list va;
    
    va_start(va, item);
    vsnprintf(buffer, 256, item, va);
    va_end(va);
    
    this->silence = true;
    char path[256] = {};
    Travel t = Toml_Travel(this, buffer, this->root, path);
    this->silence = false;
    
    if (t.arr && t.arr->nitem > t.idx)
        return x_strdup(t.arr->item[t.idx].val);
    if (t.tbl && toml_key_exists(t.tbl, t.item))
        for (var i = 0; i < t.tbl->nkval; i++)
            if (!strcmp(t.tbl->kval[i]->key, t.item))
                return x_strdup(t.tbl->kval[i]->val);
    return NULL;
}

// # # # # # # # # # # # # # # # # # # # #
// # NumArray                            #
// # # # # # # # # # # # # # # # # # # # #

s32 Toml_ArrItemNum(Toml* this, const char* arr, ...) {
    char buf[256];
    char path[256] = {};
    va_list va;
    
    va_start(va, arr);
    vsnprintf(buf, 256, arr, va);
    va_end(va);
    
    if (!StrEnd(buf, "]")) strcat(buf, "[]");
    
    this->silence = true;
    Travel t = Toml_Travel(this, buf, this->root, path);
    this->silence = false;
    
    if (t.arr) return t.arr->nitem;
    
    return 0;
}

s32 Toml_TblItemNum(Toml* this, const char* item, ...) {
    char buf[256];
    char path[256] = {};
    va_list va;
    
    va_start(va, item);
    vsnprintf(buf, 256, item, va);
    strcat(buf, ".temp");
    va_end(va);
    
    this->silence = true;
    Travel t = Toml_Travel(this, buf, this->root, path);
    this->silence = false;
    
    if (t.tbl)
        return toml_table_narr(t.tbl) + toml_table_nkval(t.tbl);
    
    return 0;
}

// # # # # # # # # # # # # # # # # # # # #
// # Impl                                #
// # # # # # # # # # # # # # # # # # # # #
