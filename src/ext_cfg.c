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
    
    Log("Variable: %s", name);
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
            return xAlloc(2);
        
        r = xStrNDup(s, strcspn(s, "\""));
    } else {
        r = xStrNDup(s, strcspn(s, "\n#"));
        
        while (StrEnd(r, " "))
            StrEnd(r, " ")[0] = '\0';
    }
    
    return r;
}

static ThreadLocal bool sCfgError;

void Config_ProcessIncludes(MemFile* mem) {
    StrNode* strNodeHead = NULL;
    StrNode* strNode = NULL;
    
    strNode = Calloc(sizeof(StrNode));
    strNode->txt = xStrDup(mem->info.name);
    Node_Add(strNodeHead, strNode);
    
reprocess:
    (void)0;
    char* line = mem->str;
    
    do {
        if (memcmp(line, "include ", 8))
            continue;
        char* head = CopyLine(line, 0);
        
        while (line[-1] != '<') {
            line++;
            if (*line == '\n' || *line == '\r' || *line == '\0')
                goto free;
        }
        
        s32 ln = 0;
        
        for (;; ln++) {
            if (line[ln] == '>')
                break;
            if (*line == '\0')
                goto free;
        }
        
        char* name;
        MemFile in = MemFile_Initialize();
        
        name = Calloc(ln + 1);
        memcpy(name, line, ln);
        
        FileSys_Path(Path(mem->info.name));
        MemFile_LoadFile_String(&in, FileSys_File(name));
        
        strNode = strNodeHead;
        while (strNode) {
            if (!strcmp(in.info.name, strNode->txt))
                printf_error("Recursive inclusion in patch [%s] including [%s]", mem->info.name, in.info.name);
            
            strNode = strNode->next;
        }
        
        if (!StrRep(mem->str, head, in.str))
            printf_error("Replacing Failed: [%s] [%X]", head, (u32) * head);
        
        strNode = Calloc(sizeof(StrNode));
        strNode->txt = FileSys_File(name);
        Node_Add(strNodeHead, strNode);
        
        MemFile_Free(&in);
        Free(name);
        goto reprocess;
    } while ( (line = Line(line, 1)) );
    
free:
    while (strNodeHead)
        Node_Kill(strNodeHead, strNodeHead);
    mem->size = strlen(mem->str);
    Log("Done");
}

s32 Config_GetErrorState(void) {
    s32 ret = 0;
    
    if (sCfgError)
        ret = 1;
    sCfgError = 0;
    
    return ret;
}

void Config_GetArray(MemFile* mem, const char* variable, ItemList* list) {
    char* array;
    char* tmp;
    u32 size = 0;
    
    Log("Build List from [%s]", variable);
    array = Config_Variable(mem->str, variable);
    
    if (array == NULL || (array[0] != '[' && array[0] != '{')) {
        *list = ItemList_Initialize();
        
        sCfgError = true;
        printf_warning("[%s] Variable [%s] not found", __FUNCTION__, variable);
        
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
    printf_warning("[%s] Variable [%s] not found", __FUNCTION__, variable);
    
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
    printf_warning("[%s] Variable [%s] not found", __FUNCTION__, variable);
    
    return 0;
}

s32 Config_GetInt(MemFile* mem, const char* variable) {
    char* ptr;
    
    ptr = Config_GetVariable(mem->str, variable);
    if (ptr) {
        return Value_Int(ptr);
    }
    
    sCfgError = true;
    printf_warning("[%s] Variable [%s] not found", __FUNCTION__, variable);
    
    return 0;
}

char* Config_GetStr(MemFile* mem, const char* variable) {
    char* ptr;
    
    ptr = Config_GetVariable(mem->str, variable);
    if (ptr) {
        Log("\"%s\" [%s]", ptr, mem->info.name);
        
        return ptr;
    }
    
    sCfgError = true;
    printf_warning("[%s] Variable [%s] not found", __FUNCTION__, variable);
    
    return NULL;
}

f32 Config_GetFloat(MemFile* mem, const char* variable) {
    char* ptr;
    
    ptr = Config_GetVariable(mem->str, variable);
    if (ptr) {
        return Value_Float(ptr);
    }
    
    sCfgError = true;
    printf_warning("[%s] Variable [%s] not found", __FUNCTION__, variable);
    
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
    u32 lineNum;
    char* wordA = xAlloc(64);
    
    ItemList_Validate(list);
    ItemList_Alloc(list, 256, 256 * 64);
    
    if (section)
        line = (void*)__Config_GotoSection(line);
    if (line == NULL) return;
    
    lineNum = LineNum(line);
    
    for (s32 i = 0; i < lineNum; i++, line = Line(line, 1)) {
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
    char* p = cfg->str;
    s32 ln = LineNum(p);
    s32 sctCount = 0;
    s32 sctSize = 0;
    
    for (s32 i = 0; i < ln; i++, p = Line(p, 1)) {
        s32 sz = 0;
        
        while (!isgraph(*p)) p++;
        if (*p != '[')
            continue;
        sctCount++;
        
        while (p[sz] != ']') {
            sz++;
            sctSize++;
            
            if (p[sz] == '\0')
                printf_error("Missing ']' from section name at line %d for [%s]", i, cfg->info.name);
        }
    }
    
    ItemList_Alloc(list, sctCount, sctSize + sctCount);
    
    p = cfg->str;
    for (s32 i = 0; i < ln; i++, p = Line(p, 1)) {
        char* word;
        
        while (!isgraph(*p)) p++;
        if (*p != '[')
            continue;
        sctCount++;
        
        word = StrDup(p + 1);
        
        for (s32 j = 0; ; j++) {
            if (word[j] == ']') {
                word[j] = '\0';
                break;
            }
        }
        
        ItemList_AddItem(list, word);
        Free(word);
    }
}

// # # # # # # # # # # # # # # # # # # # #
// # TOML                                #
// # # # # # # # # # # # # # # # # # # # #

static void Config_FollowUpComment(MemFile* mem, const char* comment) {
    if (comment)
        MemFile_Printf(mem, xFmt(" # %s", comment));
    MemFile_Printf(mem, "\n");
}

s32 Config_ReplaceVariable(MemFile* mem, const char* variable, const char* fmt, ...) {
    char* replacement;
    va_list va;
    char* p;
    
    va_start(va, fmt);
    vasprintf(&replacement, fmt, va);
    va_end(va);
    
    p = Config_Variable(mem->str, variable);
    
    if (p) {
        if (p[0] == '"')
            p++;
        StrRem(p, strlen(Config_GetVariable(mem->str, variable)));
        StrIns(p, replacement);
        
        mem->size = strlen(mem->str);
        Free(replacement);
        
        return 0;
    }
    
    Free(replacement);
    
    return 1;
}

void Config_WriteComment(MemFile* mem, const char* comment) {
    if (comment)
        MemFile_Printf(mem, xFmt("# %s", comment));
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
