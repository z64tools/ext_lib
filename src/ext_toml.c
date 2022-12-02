#define EXT_TOML_C
#include "xtoml/x0.h"
#include <ext_lib.h>

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_STRING,
} Type;

void Toml_ParseFile(Toml* this, const char* file) {
    MemFile mem = MemFile_Initialize();
    char errbuf[200];
    
    Log("Parse File: [%s]", file);
    MemFile_LoadFile_String(&mem, file);
    this->config = toml_parse(mem.str, errbuf, 200);
    if (!this->config) {
        printf_warning("[Toml Praser Error!]");
        printf_warning("File: %s", file);
        printf_error("%s", errbuf);
    }
    MemFile_Free(&mem);
}

void Toml_Table(Toml* this, const char* path) {
    u32 lvl = 2;
    
    if (!path || strlen(path) == 0) {
        this->field = this->config;
        Free(this->path);
        return;
    }
    
    Free(this->path);
    this->path = strdup(path);
    
    if (StrEnd(path, "]")) {
        const char* errorMsgs[] = {
            "PrePath",
            "Elem",
            "Table",
        };
        s32 idx = Value_Int(StrStr(path, "[") + 1);
        char* npath = strndup(path, strcspn(path, "["));
        char* elem = strdup(strrchr(npath, '.') + 1);
        
        lvl = 0;
        
        *strrchr(npath, '.') = '\0';
        Log("Table: " PRNT_GRAY "[" PRNT_YELW "%s.%s" PRNT_GRAY "][" PRNT_YELW "%d" PRNT_GRAY "]", npath, elem, idx);
        
        this->field = toml_table_in(this->config, npath);
        if (!this->field) goto error; else lvl++;
        toml_array_t* arr = toml_array_in(this->field, elem);
        if (!this->field) goto error; else lvl++;
        this->field = toml_table_at(arr, idx);
        if (!this->field) goto error; else lvl++;
        
        Free(npath);
        Free(elem);
        
        return;
error:
        printf_error("%s: " PRNT_GRAY "[" PRNT_YELW "%s.%s" PRNT_GRAY "][" PRNT_YELW "%d" PRNT_GRAY "]", errorMsgs[lvl], npath, elem, idx);
    }
    
    Log("Table: " PRNT_GRAY "[" PRNT_YELW "%s" PRNT_GRAY "]", path);
    
    this->field = toml_table_in(this->config, path);
    if (!this->field) goto error;
}

static toml_datum_t Toml_GetValue(Toml* this, const char* item, Type type) {
    toml_datum_t (*func[])(const toml_table_t*, const char*) = {
        toml_int_in,
        toml_double_in,
        toml_bool_in,
        toml_string_in,
    };
    
    if (StrEnd(item, "]")) {
        toml_datum_t (*func[])(const toml_array_t*, s32) = {
            toml_int_at,
            toml_double_at,
            toml_bool_at,
            toml_string_at,
        };
        
        u32 idx = Value_Int(StrStr(item, "["));
        char* nitem = strndup(item, strcspn(item, "["));
        
        toml_array_t* arr = toml_array_in(this->field, nitem);
        
        return func[type](arr, idx);
    }
    
    return func[type](this->field, item);
}

s32 Toml_GetInt(Toml* this, const char* item) {
    Log("Get: [%s." PRNT_YELW "%s" PRNT_RSET "]", this->path, item);
    toml_datum_t t = Toml_GetValue(this, item, TYPE_INT);
    
    return t.u.i;
}

f32 Toml_GetFloat(Toml* this, const char* item) {
    Log("Get: [%s." PRNT_YELW "%s" PRNT_RSET "]", this->path, item);
    toml_datum_t t = Toml_GetValue(this, item, TYPE_FLOAT);
    
    return t.u.d;
}

bool Toml_GetBool(Toml* this, const char* item) {
    Log("Get: [%s." PRNT_YELW "%s" PRNT_RSET "]", this->path, item);
    toml_datum_t t = Toml_GetValue(this, item, TYPE_BOOL);
    
    return t.u.b;
}

char* Toml_GetString(Toml* this, const char* item) {
    Log("Get: [%s." PRNT_YELW "%s" PRNT_RSET "]", this->path, item);
    toml_datum_t t = Toml_GetValue(this, item, TYPE_STRING);
    char* r = xStrDup(t.u.s);
    
    Free(r);
    
    return r;
}

#include "xtoml/x0impl.h"