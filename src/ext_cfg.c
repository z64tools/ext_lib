#include <ext_lib.h>

// # # # # # # # # # # # # # # # # # # # #
// # CONFIG                              #
// # # # # # # # # # # # # # # # # # # # #

static ThreadLocal char* sCfgSection;

static const char* __Config_GotoSection(const char* str) {
    if (sCfgSection == NULL)
        return str;
    
    const char* line = str;
    
    Log("GoTo \"%s\"", sCfgSection);
    
    do {
        if (*line == '\0')
            return NULL;
        while (*line == ' ' || *line == '\t') line++;
        if (!strncmp(line, sCfgSection, strlen(sCfgSection) - 1))
            return Line(line, 1);
    } while ( (line = Line(line, 1)) );
    
    Log("Return NULL;", sCfgSection);
    
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
    } while ((s = Line(s, 1)));
    
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
        
        while (StrEnd(r, " "))
            StrEnd(r, " ")[0] = '\0';
    }
    
    char varbuf[512];
    
    snprintf(varbuf, 512, "[%s]", name);
    Log("%-16s = [%s]", name, r);
    
    return r;
}

static ThreadLocal bool sCfgError;

static char* Config_GetIncludeName(const char* line) {
    u32 size;
    
    line += strcspn(line, "<\n\r");
    if (*line != '<') return NULL;
    line++;
    size = strcspn(line, ">\n\r");
    if (line[size] != '>') return NULL;
    
    return x_strndup(line, size);
}

static void Config_RecurseIncludes(MemFile* dst, const char* file, StrNode** nodeHead) {
    MemFile cfg = MemFile_Initialize();
    StrNode* strNode;
    
    if (*nodeHead) {
        strNode = *nodeHead;
        
        for (; strNode != NULL; strNode = strNode->next) {
            if (!strcmp(file, strNode->txt))
                printf_error("onfigInclude: Recursive Include! [%s] -> [%s]", dst->info.name, file);
        }
    }
    
    strNode = Calloc(sizeof(*strNode));
    strNode->txt = (void*)file;
    Node_Add(*nodeHead, strNode);
    
    Log("Load File [%s]", file);
    MemFile_LoadFile(&cfg, file);
    MemFile_Realloc(dst, dst->memSize + cfg.memSize * 2);
    
    forline(line, cfg.str) {
        if (StrStart(line, "include ")) {
            const char* include;
            
            Assert((include = Config_GetIncludeName(line)) != NULL);
            
            Log("ConfigInclude: From [%s] open [%s]", file, include);
            
            Config_RecurseIncludes(dst, x_fmt("%s%s", x_path(file), include), nodeHead);
        } else {
            u32 size = strcspn(line, "\n\r");
            
            if (size) MemFile_Write(dst, line, size);
            MemFile_Printf(dst, "\n");
        }
    }
    
    MemFile_Free(&cfg);
};

void Config_ProcessIncludes(MemFile* mem) {
    StrNode* strNodeHead = NULL;
    MemFile dst = MemFile_Initialize();
    
    dst.info.name = mem->info.name;
    Config_RecurseIncludes(&dst, mem->info.name, &strNodeHead);
    dst.str[dst.size] = '\0';
    
    Free(mem->data);
    mem->data = dst.data;
    mem->memSize = dst.memSize;
    mem->size = dst.size;
    
    while (strNodeHead)
        Node_Kill(strNodeHead, strNodeHead);
    Log("Done");
}

s32 Config_GetErrorState(void) {
    s32 ret = 0;
    
    if (sCfgError)
        ret = 1;
    sCfgError = 0;
    
    return ret;
}

static inline void Config_Warning(MemFile* mem, const char* variable, const char* function) {
    printf_warning(
        "[%s]: Variable "PRNT_BLUE "\"%s\""PRNT_RSET " not found from " PRNT_BLUE "[%s]" PRNT_RSET " %s",
        function + strlen("Config_"),
        variable, mem->info.name,
        sCfgSection ? x_fmt("from section " PRNT_BLUE "%s", sCfgSection) : ""
    );
}

void Config_GetArray(MemFile* mem, const char* variable, ItemList* list) {
    char* array;
    char* tmp;
    u32 size = 0;
    
    Log("Build List from [%s]", variable);
    array = Config_Variable(mem->str, variable);
    
    if (array == NULL) {
        *list = ItemList_Initialize();
        
        sCfgError = true;
        Config_Warning(mem, variable, __FUNCTION__);
        
        return;
    }
    
    if ((array[0] != '[' && array[0] != '{')) {
        char* str = Config_GetVariable(mem->str, variable);
        
        ItemList_Alloc(list, 1);
        ItemList_AddItem(list, str);
        
        return;
    }
    
    while (array[size] != ']' && array[size] != '}') size++;
    
    if (size > 1) {
        tmp = array;
        array = Calloc(size);
        memcpy(array, tmp + 1, size - 1);
        
        ItemList_Separated(list, array, ',');
        Free(array);
    } else
        *list = ItemList_Initialize();
    
    for (s32 i = 0; i < list->num; i++)
        StrRep(list->item[i], "\"", "");
}

s32 Config_GetBool(MemFile* mem, const char* variable) {
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

s32 Config_GetOption(MemFile* mem, const char* variable, char* strList[]) {
    char* ptr;
    char* word;
    s32 i = 0;
    
    ptr = Config_GetVariable(mem->str, variable);
    if (ptr) {
        word = ptr;
        while (strList[i] != NULL && !StrStr(word, strList[i]))
            i++;
        
        if (strList != NULL)
            return i;
    }
    
    sCfgError = true;
    Config_Warning(mem, variable, __FUNCTION__);
    
    return 0;
}

s32 Config_GetInt(MemFile* mem, const char* variable) {
    char* ptr;
    
    ptr = Config_GetVariable(mem->str, variable);
    if (ptr) {
        return Value_Int(ptr);
    }
    
    sCfgError = true;
    Config_Warning(mem, variable, __FUNCTION__);
    
    return 0;
}

char* Config_GetStr(MemFile* mem, const char* variable) {
    char* ptr;
    
    ptr = Config_GetVariable(mem->str, variable);
    if (ptr)
        return ptr;
    
    sCfgError = true;
    Config_Warning(mem, variable, __FUNCTION__);
    
    return NULL;
}

f32 Config_GetFloat(MemFile* mem, const char* variable) {
    char* ptr;
    
    ptr = Config_GetVariable(mem->str, variable);
    if (ptr) {
        return Value_Float(ptr);
    }
    
    sCfgError = true;
    Config_Warning(mem, variable, __FUNCTION__);
    
    return 0.0f;
}

void Config_GotoSection(const char* section) {
    Free(sCfgSection);
    if (section) {
        if (section[0] == '[')
            sCfgSection = StrDup(section);
        
        else
            asprintf(&sCfgSection, "[%s]", section);
    }
}

void Config_ListVariables(MemFile* mem, ItemList* list, const char* section) {
    char* line = mem->str;
    char* wordA = x_alloc(64);
    
    ItemList_Validate(list);
    ItemList_Alloc(list, 256);
    
    if (section)
        line = (void*)__Config_GotoSection(line);
    if (line == NULL) return;
    
    for (; line != NULL; line = Line(line, 1)) {
        while (*line == ' ' || *line == '\t')
            line++;
        if (*line == '#' || *line == '\n')
            continue;
        if (*line == '\0' || *line == '[')
            break;
        
        if (StrStr(CopyLine(line, 0), "=")) {
            u32 strlen = 0;
            
            while (isalnum(line[strlen]) || line[strlen] == '_' || line[strlen] == '-')
                strlen++;
            
            if (strlen) {
                memcpy(wordA, line, strlen);
                wordA[strlen] = '\0';
                
                ItemList_AddItem(list, wordA);
            }
        }
    }
}

void Config_ListSections(MemFile* cfg, ItemList* list) {
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
            printf_error("Missing ']' from section name at line %d for [%s]", line, cfg->info.name);
    }
    
    if (sctCount == 0) {
        *list = ItemList_Initialize();
        return;
    }
    
    ItemList_Alloc(list, sctCount);
    
    forline(p, cfg->str) {
        p += strspn(p, " \t");
        if (*p != '[') continue;
        sctCount++;
        
        ItemList_AddItem(list, x_strndup(p + 1, strcspn(p, "]\n")));
    }
}

// # # # # # # # # # # # # # # # # # # # #
// # .                                   #
// # # # # # # # # # # # # # # # # # # # #

static void Config_FollowUpComment(MemFile* mem, const char* comment) {
    if (comment)
        MemFile_Printf(mem, x_fmt(" # %s", comment));
    MemFile_Printf(mem, "\n");
}

s32 Config_ReplaceVariable(MemFile* mem, const char* variable, const char* fmt, ...) {
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
        StrRem(p, strlen(Config_GetVariable(mem->str, variable)));
        StrIns(p, replacement);
        
        mem->size = strlen(mem->str);
        
        return 0;
    }
    
    return 1;
}

void Config_WriteComment(MemFile* mem, const char* comment) {
    if (comment)
        MemFile_Printf(mem, x_fmt("# %s", comment));
    MemFile_Printf(mem, "\n");
}

void Config_WriteArray(MemFile* mem, const char* variable, ItemList* list, bool quote, const char* comment) {
    const char* q[2] = {
        "",
        "\"",
    };
    
    MemFile_Printf(mem, "%-15s = [ ", variable);
    for (s32 i = 0; i < list->num; i++) {
        MemFile_Printf(mem, "%s%s%s, ", q[quote], list->item[i], q[quote]);
    }
    if (list->num)
        mem->seekPoint -= 2;
    MemFile_Printf(mem, " ]");
    Config_FollowUpComment(mem, comment);
}

void Config_WriteInt(MemFile* mem, const char* variable, const s64 integer, const char* comment) {
    MemFile_Printf(mem, "%-15s = %ld", variable, integer);
    Config_FollowUpComment(mem, comment);
}

void Config_WriteHex(MemFile* mem, const char* variable, const s64 integer, const char* comment) {
    MemFile_Printf(mem, "%-15s = 0x%lX", variable, integer);
    Config_FollowUpComment(mem, comment);
}

void Config_WriteStr(MemFile* mem, const char* variable, const char* str, bool quote, const char* comment) {
    const char* q[2] = {
        "",
        "\"",
    };
    
    MemFile_Printf(mem, "%-15s = %s%s%s", variable, q[quote], str, q[quote]);
    Config_FollowUpComment(mem, comment);
}

void Config_WriteFloat(MemFile* mem, const char* variable, const f64 flo, const char* comment) {
    MemFile_Printf(mem, "%-15s = %lf", variable, flo);
    Config_FollowUpComment(mem, comment);
}

void Config_WriteBool(MemFile* mem, const char* variable, const s32 val, const char* comment) {
    MemFile_Printf(mem, "%-15s = %s", variable, val ? "true" : "false");
    Config_FollowUpComment(mem, comment);
}

void Config_WriteSection(MemFile* mem, const char* variable, const char* comment) {
    MemFile_Printf(mem, "[%s]", variable);
    if (comment)
        MemFile_Printf(mem, " # %s", comment);
    MemFile_Printf(mem, "\n");
}
