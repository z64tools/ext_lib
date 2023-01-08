#include <ext_lib.h>

// # # # # # # # # # # # # # # # # # # # #
// # CONFIG                              #
// # # # # # # # # # # # # # # # # # # # #

static thread_local char* sCfgSection;
static thread_local bool sCfgError;

static const char* Ini_GotoImpl(const char* str) {
    if (sCfgSection == NULL)
        return str;
    
    const char* line = str;
    
    _log("GoTo \"%s\"", sCfgSection);
    
    do {
        if (*line == '\0')
            return NULL;
        while (*line == ' ' || *line == '\t') line++;
        if (!strncmp(line, sCfgSection, strlen(sCfgSection) - 1))
            return strline(line, 1);
    } while ( (line = strline(line, 1)) );
    
    _log("Return NULL;", sCfgSection);
    
    return NULL;
}

char* Ini_Var(const char* str, const char* name) {
    char* s;
    
    str = Ini_GotoImpl(str);
    if (str == NULL) return NULL;
    s = (char*)str;
    
    do {
        s += strspn(s, " \t");
        
        switch (*s) {
            case '#':
            case '[':
                continue;
        }
        
        if (strcspn(s, "=") > strcspn(s, "\n"))
            continue;
        if (strncmp(name, s, strlen(name)))
            continue;
        if (strlen(name) != strcspn(s, " \t="))
            continue;
        
        s += strcspn(s, "=") + 1;
        s += strspn(s, " \t");
        if (*s == '\n' || *s == '#' || *s == '\0')
            return NULL;
        
        break;
    } while ((s = strline(s, 1)));
    
    return s;
}

char* Ini_GetVar(const char* str, const char* name) {
    char* s = Ini_Var(str, name);
    char* r;
    
    if (!s) return NULL;
    
    if (*s == '\"') {
        s++;
        if (*s == '\"')
            return x_alloc(2);
        
        r = x_strndup(s, strcspn(s, "\""));
    } else {
        r = x_strndup(s, strcspn(s, "\n#"));
        
        while (strend(r, " "))
            strend(r, " ")[0] = '\0';
    }
    
    char varbuf[512];
    
    snprintf(varbuf, 512, "[%s]", name);
    _log("%-16s = [%s]", name, r);
    
    return r;
}

static char* Ini_GetIncludeName(const char* line) {
    u32 size;
    
    line += strcspn(line, "<\n\r");
    if (*line != '<') return NULL;
    line++;
    size = strcspn(line, ">\n\r");
    if (line[size] != '>') return NULL;
    
    return x_strndup(line, size);
}

static void Ini_RecurseInclude(Memfile* dst, const char* file, strnode_t** nodeHead) {
    Memfile cfg = Memfile_New();
    strnode_t* strNode;
    
    if (*nodeHead) {
        strNode = *nodeHead;
        
        for (; strNode != NULL; strNode = strNode->next) {
            if (!strcmp(file, strNode->txt))
                errr("onfigInclude: Recursive Include! [%s] -> [%s]", dst->info.name, file);
        }
    }
    
    strNode = new(*strNode);
    strNode->txt = (void*)file;
    Node_Add(*nodeHead, strNode);
    
    _log("Load File [%s]", file);
    Memfile_LoadStr(&cfg, file);
    Memfile_Realloc(dst, dst->memSize + cfg.memSize * 2);
    
    forline(line, cfg.str) {
        if (strstart(line, "include ")) {
            const char* include;
            
            _assert((include = Ini_GetIncludeName(line)) != NULL);
            
            _log("ConfigInclude: From [%s] open [%s]", file, include);
            
            Ini_RecurseInclude(dst, x_fmt("%s%s", x_path(file), include), nodeHead);
        } else {
            u32 size = strcspn(line, "\n\r");
            
            if (size) Memfile_Write(dst, line, size);
            Memfile_Fmt(dst, "\n");
        }
    }
    
    Memfile_Free(&cfg);
};

void Ini_ParseIncludes(Memfile* mem) {
    strnode_t* strNodeHead = NULL;
    Memfile dst = Memfile_New();
    
    dst.info.name = mem->info.name;
    Ini_RecurseInclude(&dst, mem->info.name, &strNodeHead);
    dst.str[dst.size] = '\0';
    
    vfree(mem->data);
    mem->data = dst.data;
    mem->memSize = dst.memSize;
    mem->size = dst.size;
    
    while (strNodeHead)
        Node_Kill(strNodeHead, strNodeHead);
    _log("Done");
}

s32 Ini_GetError(void) {
    s32 ret = 0;
    
    if (sCfgError)
        ret = 1;
    sCfgError = 0;
    
    return ret;
}

static void Ini_WarnImpl(Memfile* mem, const char* variable, const char* function) {
    warn(
        "[%s]: Variable "PRNT_BLUE "\"%s\""PRNT_RSET " not found from " PRNT_BLUE "[%s]" PRNT_RSET " %s",
        function + strlen("Config_"),
        variable, mem->info.name,
        sCfgSection ? x_fmt("from section " PRNT_BLUE "%s", sCfgSection) : ""
    );
}

void Ini_GetArr(Memfile* mem, const char* variable, List* list) {
    char* array;
    char* tmp;
    u32 size = 0;
    
    _log("Build List from [%s]", variable);
    array = Ini_Var(mem->str, variable);
    
    if (array == NULL) {
        *list = List_New();
        
        sCfgError = true;
        Ini_WarnImpl(mem, variable, __FUNCTION__);
        
        return;
    }
    
    if ((array[0] != '[' && array[0] != '{')) {
        char* str = Ini_GetVar(mem->str, variable);
        
        List_Alloc(list, 1);
        List_Add(list, str);
        
        return;
    }
    
    while (array[size] != ']' && array[size] != '}') size++;
    
    if (size > 1) {
        tmp = array;
        array = calloc(size);
        memcpy(array, tmp + 1, size - 1);
        
        List_Tokenize2(list, array, ',');
        vfree(array);
    } else
        *list = List_New();
    
    for (s32 i = 0; i < list->num; i++)
        strrep(list->item[i], "\"", "");
}

s32 Ini_GetBool(Memfile* mem, const char* variable) {
    char* ptr;
    
    ptr = Ini_GetVar(mem->str, variable);
    if (ptr) {
        char* word = ptr;
        if (!strcmp(word, "true")) {
            return true;
        }
        if (!strcmp(word, "false")) {
            return false;
        }
    }
    
    sCfgError = true;
    Ini_WarnImpl(mem, variable, __FUNCTION__);
    
    return 0;
}

s32 Ini_GetOpt(Memfile* mem, const char* variable, char* strList[]) {
    char* ptr;
    char* word;
    s32 i = 0;
    
    ptr = Ini_GetVar(mem->str, variable);
    if (ptr) {
        word = ptr;
        while (strList[i] != NULL && !strstr(word, strList[i]))
            i++;
        
        if (strList != NULL)
            return i;
    }
    
    sCfgError = true;
    Ini_WarnImpl(mem, variable, __FUNCTION__);
    
    return 0;
}

s32 Ini_GetInt(Memfile* mem, const char* variable) {
    char* ptr;
    
    ptr = Ini_GetVar(mem->str, variable);
    if (ptr) {
        return sint(ptr);
    }
    
    sCfgError = true;
    Ini_WarnImpl(mem, variable, __FUNCTION__);
    
    return 0;
}

char* Ini_GetStr(Memfile* mem, const char* variable) {
    char* ptr;
    
    ptr = Ini_GetVar(mem->str, variable);
    if (ptr)
        return ptr;
    
    sCfgError = true;
    Ini_WarnImpl(mem, variable, __FUNCTION__);
    
    return NULL;
}

f32 Ini_GetFloat(Memfile* mem, const char* variable) {
    char* ptr;
    
    ptr = Ini_GetVar(mem->str, variable);
    if (ptr) {
        return sfloat(ptr);
    }
    
    sCfgError = true;
    Ini_WarnImpl(mem, variable, __FUNCTION__);
    
    return 0.0f;
}

void Ini_GotoTab(const char* section) {
    int ignore_result = 0;
    
    vfree(sCfgSection);
    if (section) {
        if (section[0] == '[')
            sCfgSection = strdup(section);
        
        else
            ignore_result = asprintf(&sCfgSection, "[%s]", section);
    }
    
    // Fool the compiler
    ignore_result = ignore_result * 2;
}

void Ini_ListVars(Memfile* mem, List* list, const char* section) {
    char* line = mem->str;
    char* wordA = x_alloc(64);
    
    List_Alloc(list, 256);
    
    if (section)
        line = (void*)Ini_GotoImpl(line);
    if (line == NULL) return;
    
    for (; line != NULL; line = strline(line, 1)) {
        while (*line == ' ' || *line == '\t')
            line++;
        if (*line == '#' || *line == '\n')
            continue;
        if (*line == '\0' || *line == '[')
            break;
        
        if (strstr(x_cpyline(line, 0), "=")) {
            u32 strlen = 0;
            
            while (isalnum(line[strlen]) || line[strlen] == '_' || line[strlen] == '-')
                strlen++;
            
            if (strlen) {
                memcpy(wordA, line, strlen);
                wordA[strlen] = '\0';
                
                List_Add(list, wordA);
            }
        }
    }
}

void Ini_ListTabs(Memfile* cfg, List* list) {
    s32 sctCount = 0;
    s32 line = -1;
    
    forline(p, cfg->str) {
        s32 sz = 0;
        
        line++;
        p += strspn(p, " \t");
        if (*p != '[') continue;
        sctCount++;
        
        sz = strcspn(p, "]\n");
        
        if (p[sz] == '\n' || p[sz] == '\0')
            errr("Missing ']' from section name at line %d for [%s]", line, cfg->info.name);
    }
    
    if (sctCount == 0) {
        *list = List_New();
        return;
    }
    
    List_Alloc(list, sctCount);
    
    forline(p, cfg->str) {
        p += strspn(p, " \t");
        if (*p != '[') continue;
        sctCount++;
        
        List_Add(list, x_strndup(p + 1, strcspn(p, "]\n")));
    }
}

// # # # # # # # # # # # # # # # # # # # #
// # .                                   #
// # # # # # # # # # # # # # # # # # # # #

static void Config_FollowUpComment(Memfile* mem, const char* comment) {
    if (comment)
        Memfile_Fmt(mem, x_fmt(" # %s", comment));
    Memfile_Fmt(mem, "\n");
}

s32 Ini_RepVar(Memfile* mem, const char* variable, const char* fmt, ...) {
    char replacement[4096];
    va_list va;
    char* p;
    
    va_start(va, fmt);
    vsnprintf(replacement, 4096, fmt, va);
    va_end(va);
    
    p = Ini_Var(mem->str, variable);
    
    if (p) {
        if (p[0] == '"')
            p++;
        strrem(p, strlen(Ini_GetVar(mem->str, variable)));
        strinsat(p, replacement);
        
        mem->size = strlen(mem->str);
        
        return 0;
    }
    
    return 1;
}

void Ini_WriteComment(Memfile* mem, const char* comment) {
    if (comment)
        Memfile_Fmt(mem, x_fmt("# %s", comment));
    Memfile_Fmt(mem, "\n");
}

void Ini_WriteArr(Memfile* mem, const char* variable, List* list, bool quote, const char* comment) {
    const char* q[2] = {
        "",
        "\"",
    };
    
    Memfile_Fmt(mem, "%-15s = [ ", variable);
    for (s32 i = 0; i < list->num; i++) {
        Memfile_Fmt(mem, "%s%s%s, ", q[quote], list->item[i], q[quote]);
    }
    if (list->num)
        mem->seekPoint -= 2;
    Memfile_Fmt(mem, " ]");
    Config_FollowUpComment(mem, comment);
}

void Ini_WriteInt(Memfile* mem, const char* variable, s64 integer, const char* comment) {
    Memfile_Fmt(mem, "%-15s = %ld", variable, integer);
    Config_FollowUpComment(mem, comment);
}

void Ini_WriteHex(Memfile* mem, const char* variable, s64 integer, const char* comment) {
    Memfile_Fmt(mem, "%-15s = 0x%lX", variable, integer);
    Config_FollowUpComment(mem, comment);
}

void Ini_WriteStr(Memfile* mem, const char* variable, const char* str, bool quote, const char* comment) {
    const char* q[2] = {
        "",
        "\"",
    };
    
    Memfile_Fmt(mem, "%-15s = %s%s%s", variable, q[quote], str, q[quote]);
    Config_FollowUpComment(mem, comment);
}

void Ini_WriteFloat(Memfile* mem, const char* variable, f64 flo, const char* comment) {
    Memfile_Fmt(mem, "%-15s = %lf", variable, flo);
    Config_FollowUpComment(mem, comment);
}

void Ini_WriteBool(Memfile* mem, const char* variable, s32 val, const char* comment) {
    Memfile_Fmt(mem, "%-15s = %s", variable, val ? "true" : "false");
    Config_FollowUpComment(mem, comment);
}

void Ini_WriteTab(Memfile* mem, const char* variable, const char* comment) {
    Memfile_Fmt(mem, "[%s]", variable);
    if (comment)
        Memfile_Fmt(mem, " # %s", comment);
    Memfile_Fmt(mem, "\n");
}
