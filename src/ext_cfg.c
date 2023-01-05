#include <ext_lib.h>

// # # # # # # # # # # # # # # # # # # # #
// # CONFIG                              #
// # # # # # # # # # # # # # # # # # # # #

static thread_local char* sCfgSection;

static const char* __Config_GotoSection(const char* str) {
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

char* Config_Variable(const char* str, const char* name) {
    char* s;
    
    str = __Config_GotoSection(str);
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

char* Config_GetVariable(const char* str, const char* name) {
    char* s = Config_Variable(str, name);
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

static thread_local bool sCfgError;

static char* Config_GetIncludeName(const char* line) {
    u32 size;
    
    line += strcspn(line, "<\n\r");
    if (*line != '<') return NULL;
    line++;
    size = strcspn(line, ">\n\r");
    if (line[size] != '>') return NULL;
    
    return x_strndup(line, size);
}

static void Config_RecurseIncludes(memfile_t* dst, const char* file, str_node_t** nodeHead) {
    memfile_t cfg = memfile_new();
    str_node_t* strNode;
    
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
    memfile_load_str(&cfg, file);
    memfile_realloc(dst, dst->memSize + cfg.memSize * 2);
    
    forline(line, cfg.str) {
        if (strstart(line, "include ")) {
            const char* include;
            
            _assert((include = Config_GetIncludeName(line)) != NULL);
            
            _log("ConfigInclude: From [%s] open [%s]", file, include);
            
            Config_RecurseIncludes(dst, x_fmt("%s%s", x_path(file), include), nodeHead);
        } else {
            u32 size = strcspn(line, "\n\r");
            
            if (size) memfile_write(dst, line, size);
            memfile_fmt(dst, "\n");
        }
    }
    
    memfile_free(&cfg);
};

void Config_ProcessIncludes(memfile_t* mem) {
    str_node_t* strNodeHead = NULL;
    memfile_t dst = memfile_new();
    
    dst.info.name = mem->info.name;
    Config_RecurseIncludes(&dst, mem->info.name, &strNodeHead);
    dst.str[dst.size] = '\0';
    
    free(mem->data);
    mem->data = dst.data;
    mem->memSize = dst.memSize;
    mem->size = dst.size;
    
    while (strNodeHead)
        Node_Kill(strNodeHead, strNodeHead);
    _log("Done");
}

s32 Config_GetErrorState(void) {
    s32 ret = 0;
    
    if (sCfgError)
        ret = 1;
    sCfgError = 0;
    
    return ret;
}

static inline void Config_Warning(memfile_t* mem, const char* variable, const char* function) {
    warn(
        "[%s]: Variable "PRNT_BLUE "\"%s\""PRNT_RSET " not found from " PRNT_BLUE "[%s]" PRNT_RSET " %s",
        function + strlen("Config_"),
        variable, mem->info.name,
        sCfgSection ? x_fmt("from section " PRNT_BLUE "%s", sCfgSection) : ""
    );
}

void Config_GetArray(memfile_t* mem, const char* variable, list_t* list) {
    char* array;
    char* tmp;
    u32 size = 0;
    
    _log("Build List from [%s]", variable);
    array = Config_Variable(mem->str, variable);
    
    if (array == NULL) {
        *list = list_new();
        
        sCfgError = true;
        Config_Warning(mem, variable, __FUNCTION__);
        
        return;
    }
    
    if ((array[0] != '[' && array[0] != '{')) {
        char* str = Config_GetVariable(mem->str, variable);
        
        list_alloc(list, 1);
        list_add(list, str);
        
        return;
    }
    
    while (array[size] != ']' && array[size] != '}') size++;
    
    if (size > 1) {
        tmp = array;
        array = calloc(size);
        memcpy(array, tmp + 1, size - 1);
        
        list_tokenize2(list, array, ',');
        free(array);
    } else
        *list = list_new();
    
    for (s32 i = 0; i < list->num; i++)
        strrep(list->item[i], "\"", "");
}

s32 Config_GetBool(memfile_t* mem, const char* variable) {
    char* ptr;
    
    ptr = Config_GetVariable(mem->str, variable);
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
    Config_Warning(mem, variable, __FUNCTION__);
    
    return 0;
}

s32 Config_GetOption(memfile_t* mem, const char* variable, char* strList[]) {
    char* ptr;
    char* word;
    s32 i = 0;
    
    ptr = Config_GetVariable(mem->str, variable);
    if (ptr) {
        word = ptr;
        while (strList[i] != NULL && !strstr(word, strList[i]))
            i++;
        
        if (strList != NULL)
            return i;
    }
    
    sCfgError = true;
    Config_Warning(mem, variable, __FUNCTION__);
    
    return 0;
}

s32 Config_GetInt(memfile_t* mem, const char* variable) {
    char* ptr;
    
    ptr = Config_GetVariable(mem->str, variable);
    if (ptr) {
        return sint(ptr);
    }
    
    sCfgError = true;
    Config_Warning(mem, variable, __FUNCTION__);
    
    return 0;
}

char* Config_GetStr(memfile_t* mem, const char* variable) {
    char* ptr;
    
    ptr = Config_GetVariable(mem->str, variable);
    if (ptr)
        return ptr;
    
    sCfgError = true;
    Config_Warning(mem, variable, __FUNCTION__);
    
    return NULL;
}

f32 Config_GetFloat(memfile_t* mem, const char* variable) {
    char* ptr;
    
    ptr = Config_GetVariable(mem->str, variable);
    if (ptr) {
        return sfloat(ptr);
    }
    
    sCfgError = true;
    Config_Warning(mem, variable, __FUNCTION__);
    
    return 0.0f;
}

void Config_GotoSection(const char* section) {
    int ignore_result = 0;
    
    free(sCfgSection);
    if (section) {
        if (section[0] == '[')
            sCfgSection = strdup(section);
        
        else
            ignore_result = asprintf(&sCfgSection, "[%s]", section);
    }
    
    // Fool the compiler
    ignore_result = ignore_result * 2;
}

void Config_ListVariables(memfile_t* mem, list_t* list, const char* section) {
    char* line = mem->str;
    char* wordA = x_alloc(64);
    
    list_alloc(list, 256);
    
    if (section)
        line = (void*)__Config_GotoSection(line);
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
                
                list_add(list, wordA);
            }
        }
    }
}

void Config_ListSections(memfile_t* cfg, list_t* list) {
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
        *list = list_new();
        return;
    }
    
    list_alloc(list, sctCount);
    
    forline(p, cfg->str) {
        p += strspn(p, " \t");
        if (*p != '[') continue;
        sctCount++;
        
        list_add(list, x_strndup(p + 1, strcspn(p, "]\n")));
    }
}

// # # # # # # # # # # # # # # # # # # # #
// # .                                   #
// # # # # # # # # # # # # # # # # # # # #

static void Config_FollowUpComment(memfile_t* mem, const char* comment) {
    if (comment)
        memfile_fmt(mem, x_fmt(" # %s", comment));
    memfile_fmt(mem, "\n");
}

s32 Config_ReplaceVariable(memfile_t* mem, const char* variable, const char* fmt, ...) {
    char replacement[4096];
    va_list va;
    char* p;
    
    va_start(va, fmt);
    vsnprintf(replacement, 4096, fmt, va);
    va_end(va);
    
    p = Config_Variable(mem->str, variable);
    
    if (p) {
        if (p[0] == '"')
            p++;
        strrem(p, strlen(Config_GetVariable(mem->str, variable)));
        strinsat(p, replacement);
        
        mem->size = strlen(mem->str);
        
        return 0;
    }
    
    return 1;
}

void Config_WriteComment(memfile_t* mem, const char* comment) {
    if (comment)
        memfile_fmt(mem, x_fmt("# %s", comment));
    memfile_fmt(mem, "\n");
}

void Config_WriteArray(memfile_t* mem, const char* variable, list_t* list, bool quote, const char* comment) {
    const char* q[2] = {
        "",
        "\"",
    };
    
    memfile_fmt(mem, "%-15s = [ ", variable);
    for (s32 i = 0; i < list->num; i++) {
        memfile_fmt(mem, "%s%s%s, ", q[quote], list->item[i], q[quote]);
    }
    if (list->num)
        mem->seekPoint -= 2;
    memfile_fmt(mem, " ]");
    Config_FollowUpComment(mem, comment);
}

void Config_WriteInt(memfile_t* mem, const char* variable, const s64 integer, const char* comment) {
    memfile_fmt(mem, "%-15s = %ld", variable, integer);
    Config_FollowUpComment(mem, comment);
}

void Config_WriteHex(memfile_t* mem, const char* variable, const s64 integer, const char* comment) {
    memfile_fmt(mem, "%-15s = 0x%lX", variable, integer);
    Config_FollowUpComment(mem, comment);
}

void Config_WriteStr(memfile_t* mem, const char* variable, const char* str, bool quote, const char* comment) {
    const char* q[2] = {
        "",
        "\"",
    };
    
    memfile_fmt(mem, "%-15s = %s%s%s", variable, q[quote], str, q[quote]);
    Config_FollowUpComment(mem, comment);
}

void Config_WriteFloat(memfile_t* mem, const char* variable, const f64 flo, const char* comment) {
    memfile_fmt(mem, "%-15s = %lf", variable, flo);
    Config_FollowUpComment(mem, comment);
}

void Config_WriteBool(memfile_t* mem, const char* variable, const s32 val, const char* comment) {
    memfile_fmt(mem, "%-15s = %s", variable, val ? "true" : "false");
    Config_FollowUpComment(mem, comment);
}

void Config_WriteSection(memfile_t* mem, const char* variable, const char* comment) {
    memfile_fmt(mem, "[%s]", variable);
    if (comment)
        memfile_fmt(mem, " # %s", comment);
    memfile_fmt(mem, "\n");
}
