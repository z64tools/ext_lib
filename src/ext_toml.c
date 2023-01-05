#define EXT_TOML_C

#ifdef __clang__
#define free __hidden_free
#endif

#include "xtoml/x0.h"
#include "xtoml/x0impl.h"
#include <ext_lib.h>

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_STRING,
} type_t;

typedef struct {
    toml_table_t* tbl;
    toml_array_t* arr;
    const char*   item;
    s32 idx;
} travel_t;

#define BUFFER_SIZE 1024

toml_t toml_new() {
    toml_t this = {};
    
    return this;
}

static const char* Toml_GetPathStr(const char* path, const char* elem, const char* next) {
    return x_fmt("" PRNT_GRAY "[ %s"PRNT_YELW "%s"PRNT_GRAY "%s]",
               path ? path : "", elem ? elem : "", next ? next : "");
}

static travel_t Toml_Travel(toml_t* this, const char* field, toml_table_t* tbl, char* path) {
    const char* elem = x_strndup(field, strcspn(field, "."));
    const char* next = elem ? &field[strlen(elem)] : NULL;
    travel_t t = {};
    
    if (!tbl) return t;
    
    strcat(path, elem);
    
    nested(void, NewTable, (toml_table_t * tbl, const char* elem)) {
        _log("NewTable: %s", elem);
        
        tbl->tab = realloc(tbl->tab, sizeof(void*) * ++tbl->ntab);
        tbl->tab[tbl->ntab - 1] = new(toml_table_t);
        tbl->tab[tbl->ntab - 1]->key = strdup(elem);
    };
    
    nested(void, NewTblArray, (toml_table_t * tbl, const char* elem)) {
        _log("NewTableArray: %s", elem);
        
        tbl->arr = realloc(tbl->arr, sizeof(void*) * ++tbl->narr);
        tbl->arr[tbl->narr - 1] = new(toml_array_t);
        tbl->arr[tbl->narr - 1]->key = strdup(elem);
        tbl->arr[tbl->narr - 1]->kind = 't';
        tbl->arr[tbl->narr - 1]->type = 'm';
    };
    
    nested(void, NewTblArrayIndex, (toml_array_t * arr, const char* elem, s32 idx)) {
        idx += 1;
        arr->item = realloc(arr->item, sizeof(toml_arritem_t) * idx);
        
        for (var_t i = arr->nitem; i < idx; i++) {
            _log("NewTableArrayIdx: %s [%d]", elem, i);
            arr->item[i] = (toml_arritem_t) { .tab = new(toml_table_t) };
        }
        
        arr->nitem = idx;
    };
    
    nested(void, NewArray, (toml_table_t * tbl, const char* elem)) {
        _log("NewArray: %s", elem);
        tbl->arr = realloc(tbl->arr, sizeof(void*) * ++tbl->narr);
        tbl->arr[tbl->narr - 1] = new(toml_array_t);
        tbl->arr[tbl->narr - 1]->key = strdup(elem);
        tbl->arr[tbl->narr - 1]->kind = 'v';
        tbl->arr[tbl->narr - 1]->type = 'm';
        tbl->arr[tbl->narr - 1]->nitem = 1;
        tbl->arr[tbl->narr - 1]->item = new(toml_arritem_t);
    };
    
    nested(travel_t, Error, (const char* msg)) {
        travel_t null = {};
        
        if (!this->silence) {
            warn("" PRNT_REDD "%s" PRNT_RSET " does not exist!", msg);
            errr("%s", Toml_GetPathStr(path, elem, next));
        }
        this->success = false;
        
        return null;
    };
    
    if (!next || *next == '\0') {
        if (strend(elem, "]")) {
            const char* arv = strstr(elem, "[") + 1;
            s32 idx = sint(arv);
            s32 sidx = -1;
            
            if ((arv = strstr(arv, "["))) {
                arv++;
                sidx = sint(arv);
                
                swapvar(idx, sidx);
            }
            
            strstr(elem, "[")[0] = '\0';
            
            t = (travel_t) {
                .arr = toml_array_in(tbl, elem),
                .idx = idx,
            };
            
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
                
                _log("%d", arr ? arr->nitem : -1);
                if (!arr) {
                    if (this->write) {
                        toml_array_t* varr = t.arr;
                        u32 num = varr->nitem;
                        
                        varr->item = realloc(varr->item, sizeof(toml_arritem_t) * (sidx + 1));
                        for (var_t n = num; n < sidx + 1; n++) {
                            varr->item[n] = (toml_arritem_t) { 0 };
                            varr->item[n].arr = new(toml_array_t);
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
        
        t =  (travel_t) {
            .tbl = tbl,
            .item = elem,
        };
        
        if (!t.tbl)
            return Error("Variable");
        
        return t;
    } else next++;
    
    strcat(path, ".");
    toml_table_t* new_tbl;
    
    if (strend(elem, "]")) {
        s32 idx = sint(strstr(elem, "[") + 1);
        strstr(elem, "[")[0] = '\0';
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

static toml_datum_t Toml_GetValue(toml_t* this, const char* item, type_t type) {
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
    
    char path[BUFFER_SIZE] = {};
    travel_t t = Toml_Travel(this, item, this->root, path);
    
    if (t.tbl)
        return getVar[type](t.tbl, t.item);
    else if (t.arr)
        return getArr[type](t.arr, t.idx);
    
    return (toml_datum_t) {};
}

void toml_set_var(toml_t* this, const char* item, const char* fmt, ...) {
    char path[BUFFER_SIZE] = {};
    char value[BUFFER_SIZE];
    va_list va;
    
    va_start(va, fmt);
    vsnprintf(value, BUFFER_SIZE, fmt, va);
    va_end(va);
    
    if (this->root == NULL)
        this->root = new(toml_table_t);
    
    this->write = true;
    travel_t t = Toml_Travel(this, item, this->root, path);
    this->write = false;
    
    if (t.tbl) {
        toml_table_t* tbl = t.tbl;
        
        _log("SetValue(Tbl): %s = %s", item, value);
        
        if (!toml_key_exists(t.tbl, t.item)) {
            tbl->kval = realloc(tbl->kval, sizeof(void*) * ++tbl->nkval);
            tbl->kval[tbl->nkval - 1] = calloc(sizeof(toml_keyval_t));
            
            tbl->kval[tbl->nkval - 1]->key = strdup(t.item);
            this->changed = true;
        }
        
        for (var_t i = 0; i < tbl->nkval; i++) {
            if (!strcmp(tbl->kval[i]->key, t.item)) {
                if (!tbl->kval[i]->val || strcmp(tbl->kval[i]->val, value)) {
                    free(tbl->kval[i]->val);
                    tbl->kval[i]->val = strdup(value);
                    this->changed = true;
                }
            }
        }
    }
    
    if (t.arr) {
        toml_array_t* arr = t.arr;
        
        _log("SetValue(Arr): %s = %s", item, value);
        
        while (arr->nitem <= t.idx) {
            arr->item = realloc(arr->item, sizeof(toml_arritem_t) * ++arr->nitem);
            arr->item[arr->nitem - 1] = (toml_arritem_t) {};
            arr->item[arr->nitem - 1].val = strdup("0");
            arr->item[arr->nitem - 1].valtype = valtype("0");
            arr->item[arr->nitem - 1].tab = 0;
            arr->item[arr->nitem - 1].arr = 0;
            this->changed = true;
        }
        
        if (!arr->item[t.idx].val || strcmp(arr->item[t.idx].val, value)) {
            free(arr->item[t.idx].val);
            arr->item[t.idx].val = strdup(value);
            arr->item[t.idx].valtype = valtype(value);
            arr->item[t.idx].tab = 0;
            arr->item[t.idx].arr = 0;
            this->changed = true;
        }
    }
}

void toml_set_tbl(toml_t* this, const char* item, ...) {
    char path[BUFFER_SIZE] = {};
    char table[BUFFER_SIZE];
    va_list va;
    
    va_start(va, item);
    vsnprintf(table, BUFFER_SIZE, item, va);
    strncat(table, ".temp", BUFFER_SIZE);
    va_end(va);
    
    if (this->root == NULL)
        this->root = new(toml_table_t);
    
    this->write = true;
    Toml_Travel(this, table, this->root, path);
    this->write = false;
    this->changed = true;
}

enum Remove {
    REMOVE_NONE,
    REMOVE_VAR,
    REMOVE_ARR,
    REMOVE_TBL
};

static bool Toml_Remove(toml_t* this, enum Remove rem, const char* item, va_list va) {
    char path[BUFFER_SIZE] = {};
    char value[BUFFER_SIZE];
    
    vsnprintf(value, BUFFER_SIZE, item, va);
    
    if (this->root == NULL)
        this->root = new(toml_table_t);
    
    switch (rem) {
        case REMOVE_ARR:
            if (!strend(value, "]"))
                strcat(value, "[]");
            break;
            
        case REMOVE_TBL:
            strcat(value, ".t");
            break;
            
        default:
            break;
    }
    
    this->silence = true;
    travel_t t = Toml_Travel(this, item, this->root, path);
    this->silence = false;
    
    toml_table_t* tbl = t.tbl;
    
    if (!tbl) return false;
    
    #define TOML_REMOVE(TYPE)                                 \
        for (var_t i = 0; i < tbl->n ## TYPE; i++) {          \
            if (!strcmp(tbl->TYPE[i]->key, t.item)) {         \
                xfree_ ## TYPE(tbl->TYPE[i]);                 \
                arrmve_l(tbl->TYPE, i, (tbl->n ## TYPE) - i); \
                tbl->n ## TYPE--;                             \
                if (!tbl->n ## TYPE) { free(tbl->TYPE); }     \
                this->changed = true;                         \
                return true;                                  \
            }                                                 \
        }
    
    switch (rem) {
        case REMOVE_VAR:
            TOML_REMOVE(kval);
            break;
            
        case REMOVE_ARR:
            TOML_REMOVE(arr);
            break;
            
        case REMOVE_TBL:
            TOML_REMOVE(tab);
            break;
            
        default:
            break;
    }
    
    return false;
}

bool toml_rm_var(toml_t* this, const char* fmt, ...) {
    va_list va;
    
    va_start(va, fmt);
    bool r = Toml_Remove(this, REMOVE_VAR, fmt, va);
    va_end(va);
    return r;
}

bool toml_rm_arr(toml_t* this, const char* fmt, ...) {
    va_list va;
    
    va_start(va, fmt);
    bool r = Toml_Remove(this, REMOVE_ARR, fmt, va);
    va_end(va);
    return r;
}

bool toml_rm_tbl(toml_t* this, const char* fmt, ...) {
    va_list va;
    
    va_start(va, fmt);
    bool r = Toml_Remove(this, REMOVE_TBL, fmt, va);
    va_end(va);
    return r;
}

// # # # # # # # # # # # # # # # # # # # #
// # Base                                #
// # # # # # # # # # # # # # # # # # # # #

void toml_load(toml_t* this, const char* file) {
    memfile_t mem = memfile_new();
    char errbuf[200];
    
    _log("Parse File: [%s]", file);
    memfile_load_str(&mem, file);
    if (!(this->root = toml_parse(mem.str, errbuf, 200))) {
        warn("[toml_t Praser Error!]");
        warn("File: %s", file);
        errr("%s", errbuf);
    }
    memfile_free(&mem);
}

void toml_free(toml_t* this) {
    if (!this) return;
    __toml_free(this->root);
    memset(this, 0, sizeof(*this));
}

void toml_print(toml_t* this, void* d, void (*PRINT)(void*, const char*, ...)) {
    nested(void, Parse, (toml_table_t * tbl, char* path, u32 indent)) {
        int nkval = toml_table_nkval(tbl);
        int narr = toml_table_narr(tbl);
        int ntab = toml_table_ntab(tbl);
        char* nd = "";
        
        _log("%08X [%s]: %d / %d / %d", tbl, path, tbl->nkval, tbl->narr, tbl->ntab);
        
        if (indent) {
            nd = x_alloc(indent + 1);
            memset(nd, '\t', indent);
        }
        
        for (var_t i = 0; i < nkval; i++) {
            _log("Value: %s", toml_key_in(tbl, i));
            
            PRINT(d, "%s%-16s = ", nd, toml_key_in(tbl, i));
            PRINT(d, "%s\n", toml_raw_in(tbl, toml_key_in(tbl, i)));
        }
        
        for (var_t i = nkval; i < narr + nkval; i++) {
            _log("Array: %s", toml_key_in(tbl, i));
            
            toml_array_t* arr = toml_array_in(tbl, toml_key_in(tbl, i));
            const char* val;
            var_t j = 0;
            var_t k = 0;
            toml_array_t* ar = arr;
            
            switch (toml_array_kind(arr)) {
                case 'a':
                    PRINT(d, "%s%-16s = [\n", nd, toml_key_in(tbl, i));
                    
                    ar = toml_array_at(arr, k++);
                    while (ar) {
                        j = 0;
                        PRINT(d, "\t%s[ ", nd);
                        
                        val = toml_raw_at(ar, j++);
                        while (val) {
                            PRINT(d, "%s", val);
                            
                            val = toml_raw_at(ar, j++);
                            if (val)
                                PRINT(d, ", ");
                        }
                        PRINT(d, " ]");
                        ar = toml_array_at(arr, k++);
                        if (ar)
                            PRINT(d, ",");
                        PRINT(d, "\n");
                    }
                    PRINT(d, "%s]\n", nd);
                    
                    break;
                case 'v':
                    PRINT(d, "%s%-16s = [ ", nd, toml_key_in(tbl, i));
                    
                    j = k = 0;
                    
                    val = toml_raw_at(arr, j++);
                    while (val) {
                        PRINT(d, "%s", val);
                        val = toml_raw_at(arr, j++);
                        if (val)
                            PRINT(d, ", ");
                    }
                    PRINT(d, " ]\n");
                    break;
            }
        }
        
        for (var_t i = nkval; i < narr + nkval; i++) {
            _log("Array: %s", toml_key_in(tbl, i));
            
            toml_array_t* arr = toml_array_in(tbl, toml_key_in(tbl, i));
            var_t k = 0;
            toml_table_t* tb;
            
            switch (toml_array_kind(arr)) {
                case 't':
                
                    while ((tb = toml_table_at(arr, k++))) {
                        PRINT(d, "%s[[%s%s]]\n", nd, path, toml_key_in(tbl, i));
                        Parse(tb, x_fmt("%s%s.", path, toml_key_in(tbl, i)), indent + 1);
                    }
                    
                    break;
            }
        }
        
        for (var_t i = nkval + narr; i < nkval + narr + ntab; i++) {
            _log("Table: %s", toml_key_in(tbl, i));
            PRINT(d, "%s[%s%s]\n", nd, path, toml_key_in(tbl, i));
            
            Parse(toml_table_in(tbl, toml_key_in(tbl, i)), x_fmt("%s%s.", path, toml_key_in(tbl, i)), indent + 1);
        }
    };
    
    Parse(this->root, "", 0);
}

void toml_write_mem(toml_t* this, memfile_t* mem) {
    memfile_free(mem);
    toml_print(this, mem, (void*)memfile_fmt);
}

void toml_save(toml_t* this, const char* file) {
    memfile_t mem = memfile_new();
    
    toml_print(this, &mem, (void*)memfile_fmt);
    memfile_save_str(&mem, file);
    memfile_free(&mem);
}

// # # # # # # # # # # # # # # # # # # # #
// # GetValue                            #
// # # # # # # # # # # # # # # # # # # # #

int toml_get_int(toml_t* this, const char* item, ...) {
    char buffer[BUFFER_SIZE];
    va_list va;
    
    va_start(va, item);
    vsnprintf(buffer, BUFFER_SIZE, item, va);
    toml_datum_t t = Toml_GetValue(this, buffer, TYPE_INT);
    va_end(va);
    
    return t.u.i;
}

f32 toml_get_float(toml_t* this, const char* item, ...) {
    char buffer[BUFFER_SIZE];
    va_list va;
    
    va_start(va, item);
    vsnprintf(buffer, BUFFER_SIZE, item, va);
    toml_datum_t t = Toml_GetValue(this, buffer, TYPE_FLOAT);
    va_end(va);
    
    return t.u.d;
}

bool toml_get_bool(toml_t* this, const char* item, ...) {
    char buffer[BUFFER_SIZE];
    va_list va;
    
    va_start(va, item);
    vsnprintf(buffer, BUFFER_SIZE, item, va);
    toml_datum_t t = Toml_GetValue(this, buffer, TYPE_BOOL);
    va_end(va);
    
    return t.u.b;
}

char* toml_get_str(toml_t* this, const char* item, ...) {
    char buffer[BUFFER_SIZE];
    va_list va;
    
    va_start(va, item);
    vsnprintf(buffer, BUFFER_SIZE, item, va);
    toml_datum_t t = Toml_GetValue(this, buffer, TYPE_STRING);
    va_end(va);
    
    if (!t.ok) return NULL;
    
    return t.u.s;
}

char* toml_get_var(toml_t* this, const char* item, ...) {
    char buffer[BUFFER_SIZE];
    va_list va;
    
    va_start(va, item);
    vsnprintf(buffer, BUFFER_SIZE, item, va);
    va_end(va);
    
    this->silence = true;
    char path[BUFFER_SIZE] = {};
    travel_t t = Toml_Travel(this, buffer, this->root, path);
    this->silence = false;
    
    if (t.arr && t.arr->nitem > t.idx)
        return x_strdup(t.arr->item[t.idx].val);
    if (t.tbl && toml_key_exists(t.tbl, t.item))
        for (var_t i = 0; i < t.tbl->nkval; i++)
            if (!strcmp(t.tbl->kval[i]->key, t.item))
                return x_strdup(t.tbl->kval[i]->val);
    return NULL;
}

// # # # # # # # # # # # # # # # # # # # #
// # NumArray                            #
// # # # # # # # # # # # # # # # # # # # #

int toml_arr_item_num(toml_t* this, const char* arr, ...) {
    char buf[BUFFER_SIZE];
    char path[BUFFER_SIZE] = {};
    va_list va;
    
    va_start(va, arr);
    vsnprintf(buf, BUFFER_SIZE, arr, va);
    va_end(va);
    
    if (!strend(buf, "]")) strcat(buf, "[]");
    
    this->silence = true;
    travel_t t = Toml_Travel(this, buf, this->root, path);
    this->silence = false;
    
    if (t.arr) return t.arr->nitem;
    
    return 0;
}

int toml_tbl_item_num(toml_t* this, const char* item, ...) {
    char buf[BUFFER_SIZE];
    char path[BUFFER_SIZE] = {};
    va_list va;
    
    va_start(va, item);
    vsnprintf(buf, BUFFER_SIZE, item, va);
    strcat(buf, ".temp");
    va_end(va);
    
    this->silence = true;
    travel_t t = Toml_Travel(this, buf, this->root, path);
    this->silence = false;
    
    if (t.tbl)
        return toml_table_narr(t.tbl) + toml_table_nkval(t.tbl);
    
    return 0;
}

// # # # # # # # # # # # # # # # # # # # #
// # Impl                                #
// # # # # # # # # # # # # # # # # # # # #
