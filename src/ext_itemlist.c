#include <ext_lib.h>
#include <dirent.h>

#undef ItemList_GetWildItem
#undef ItemList_SetFilter

#undef ItemList_List
#undef ItemList_Separated
#undef ItemList_Free
#undef ItemList_Alloc
#undef ItemList_AddItem

// # # # # # # # # # # # # # # # # # # # #
// # ITEM LIST                           #
// # # # # # # # # # # # # # # # # # # # #

ItemList gList_SortError;

void ItemList_Validate(ItemList* itemList) {
    if (itemList->initKey == 0xDEFABEBACECAFAFF)
        return;
    
    *itemList = ItemList_Initialize();
}

ItemList ItemList_Initialize(void) {
    return (ItemList) { .initKey = 0xDEFABEBACECAFAFF, .p.alnum = 0 };
}

void ItemList_SetFilter(ItemList* list, u32 filterNum, ...) {
    va_list va;
    
    ItemList_Validate(list);
    ItemList_FreeFilters(list);
    
    if (filterNum % 2 == 1)
        printf_warning("Odd filterNum! [%d]", --filterNum);
    
    va_start(va, filterNum);
    
    for (s32 i = 0; i < filterNum * 0.5; i++) {
        FilterNode* node;
        
        node = Calloc(sizeof(FilterNode));
        node->type = va_arg(va, ListFilter);
        node->txt = strdup(va_arg(va, char*));
        Assert(node->txt != NULL);
        
        Node_Add(list->filterNode, node);
    }
    
    va_end(va);
}

void ItemList_FreeFilters(ItemList* this) {
    if (this->initKey == 0xDEFABEBACECAFAFF) {
        while (this->filterNode) {
            Free(this->filterNode->txt);
            
            Node_Kill(this->filterNode, this->filterNode);
        }
    } else
        *this = ItemList_Initialize();
}

typedef struct {
    StrNode*       node;
    u32            num;
    const ListFlag flags;
} WalkInfo;

static void ItemList_Walk(const ItemList* list, const char* base, const char* parent, const s32 level, const s32 max, WalkInfo* info) {
    const char* entryPath = parent;
    
    if (info->flags & LIST_RELATIVE)
        entryPath = parent + strlen(base);
    
    DIR* dir;
    if (strlen(parent) == 0) dir = opendir("./");
    else dir = opendir(parent);
    
    if (dir) {
        const struct dirent* entry;
        
        while ((entry = readdir(dir))) {
            StrNode* node;
            char path[261];
            bool skip = 0;
            bool filterContainUse = false;
            bool filterContainMatch = false;
            
            if (!memcmp(".\0", entry->d_name, 2) || !memcmp("..\0", entry->d_name, 3))
                continue;
            
            for (const FilterNode* filterNode = list->filterNode; filterNode != NULL; filterNode = filterNode->next) {
                switch (filterNode->type) {
                    case FILTER_SEARCH:
                        if (StrStr(entry->d_name, filterNode->txt))
                            skip = true;
                        break;
                    case FILTER_START:
                        if (!memcmp(entry->d_name, filterNode->txt,
                            strlen(filterNode->txt)))
                            skip = true;
                        break;
                    case FILTER_END:
                        if (StrEnd(entry->d_name, filterNode->txt))
                            skip = true;
                        break;
                    case FILTER_WORD:
                        if (!strcmp(entry->d_name, filterNode->txt))
                            skip = true;
                        break;
                        
                    case CONTAIN_SEARCH:
                        filterContainUse = true;
                        if (StrStr(entry->d_name, filterNode->txt))
                            filterContainMatch = true;
                        break;
                    case CONTAIN_START:
                        filterContainUse = true;
                        if (!memcmp(entry->d_name, filterNode->txt,
                            strlen(filterNode->txt)))
                            filterContainMatch = true;
                        break;
                    case CONTAIN_END:
                        filterContainUse = true;
                        if (StrEnd(entry->d_name, filterNode->txt))
                            filterContainMatch = true;
                        break;
                    case CONTAIN_WORD:
                        filterContainUse = true;
                        if (!strcmp(entry->d_name, filterNode->txt))
                            filterContainMatch = true;
                        break;
                }
                
                if (skip)
                    break;
            }
            
            if (skip) continue;
            if (((info->flags & LIST_NO_DOT) && entry->d_name[0] == '.')) continue;
            
            snprintf(path, 261, "%s%s", parent, entry->d_name);
            
            if (Sys_IsDir(path)) {
                strncat(path, "/", 261);
                
                if (max == -1 || level < max)
                    ItemList_Walk(list, base, path, level + 1, max, info);
                
                if ((info->flags & 0xF) == LIST_FOLDERS) {
                    node = New(StrNode);
                    node->txt = Fmt("%s%s/", entryPath, entry->d_name);
                    info->num++;
                    
                    Node_Add(info->node, node);
                }
            } else {
                if (!filterContainUse || (filterContainUse && filterContainMatch)) {
                    if ((info->flags & 0xF) == LIST_FILES) {
                        node = New(StrNode);
                        node->txt = Fmt("%s%s", entryPath, entry->d_name);
                        info->num++;
                        
                        Node_Add(info->node, node);
                    }
                }
            }
        }
        
        closedir(dir);
    } else
        printf_error("Could not open dir [%s]", dir);
}

void ItemList_List(ItemList* this, const char* path, s32 depth, ListFlag flags) {
    char buf[261] = "";
    
    ItemList_Validate(this);
    
    Assert(path != NULL);
    if (strlen(path) > 0 ) {
        
        if (!Sys_IsDir(path))
            printf_error("Can't walk path that does not exist! [%s]", path);
        
        strcpy(buf, path);
        if (!StrEnd(buf, "/")) strcat(buf, "/");
    }
    
    WalkInfo info = { .flags = flags };
    
    Log("Walk: %s", buf);
    ItemList_Walk(this, buf, buf, 0, depth, &info);
    
    if (info.num) {
        this->num = info.num;
        this->item = New(char*[info.num]);
        
        for (var i = 0; i < info.num; i++) {
            this->item[i] = info.node->txt;
            Node_Kill(info.node, info.node);
        }
    }
}

char* ItemList_Concat(ItemList* this, const char* separator) {
    u32 len = 0;
    u32 seplen = separator ? strlen(separator) : 0;
    
    for (var i = 0; i < this->num; i++) {
        if (!this->item[i]) continue;
        len += strlen(this->item[i]) + seplen;
    }
    
    if (!len) return NULL;
    
    char* r = New(char[len + 1]);
    
    for (var i = 0; i < this->num; i++) {
        if (!this->item[i]) continue;
        
        strcat(r, this->item[i]);
        if (seplen) strcat(r, separator);
    }
    
    if (separator)
        StrEnd(r, separator)[0] = '\0';
    
    return r;
}

char* ItemList_GetWildItem(ItemList* list, const char* end, const char* error, ...) {
    for (s32 i = 0; i < list->num; i++)
        if (StrEndCase(list->item[i], end))
            return list->item[i];
    
    if (error) {
        char* text;
        va_list va;
        
        va_start(va, error);
        vasprintf(&text, error, va);
        va_end(va);
        
        printf_error("%s", error);
    }
    
    return NULL;
}

void ItemList_Separated(ItemList* list, const char* str, const char separator) {
    s32 a = 0;
    s32 b = 0;
    StrNode* nodeHead = NULL;
    
    ItemList_Free(list);
    
    while (str[a] == ' ' || str[a] == '\t' || str[a] == '\n' || str[a] == '\r') a++;
    
    while (true) {
        StrNode* node = NULL;
        s32 isString = 0;
        s32 strcompns = 0;
        s32 brk = true;
        
        if (str[a] == '\"' || str[a] == '\'') {
            isString = 1;
            a++;
        }
        
        b = a;
        
        if (isString && (str[b] == '\"' || str[b] == '\'')) {
            node = Calloc(sizeof(StrNode));
            node->txt = Calloc(2);
            Node_Add(nodeHead, node);
            
            b++;
            
            goto write;
        }
        
        while (isString || (str[b] != separator && str[b] != '\0')) {
            b++;
            if (isString && (str[b] == '\"' || str[b] == '\'')) {
                isString = 0;
                strcompns = -1;
            }
        }
        while (!isString && a != b && separator != ' ' && str[b - 1] == ' ') b--;
        
        if (b == a)
            break;
        
        for (s32 v = 0; v < b - a + strcompns + 1; v++) {
            if (isgraph(str[a + v]))
                brk = false;
        }
        
        if (brk)
            break;
        
        node = Calloc(sizeof(StrNode));
        node->txt = Calloc(b - a + strcompns + 1);
        memcpy(node->txt, &str[a], b - a + strcompns);
        Node_Add(nodeHead, node);
        
write:
        list->num++;
        
        a = b;
        
        while (str[a] == separator || str[a] == ' ' || str[a] == '\t' || str[a] == '\n' || str[a] == '\r') a++;
        if (str[a] == '\0')
            break;
    }
    
    list->item = New(char*[list->num]);
    
    for (s32 i = 0; i < list->num; i++) {
        list->item[i] = nodeHead->txt;
        Node_Kill(nodeHead, nodeHead);
    }
}

void ItemList_Print(ItemList* target) {
    for (s32 i = 0; i < target->num; i++)
        printf("[#]: %4d: \"%s\"\n", i, target->item[i]);
}

Time ItemList_StatMax(ItemList* list) {
    Time val = 0;
    
    for (s32 i = 0; i < list->num; i++)
        val = Max(val, Sys_Stat(list->item[i]));
    
    return val;
}

Time ItemList_StatMin(ItemList* list) {
    Time val = ItemList_StatMax(list);
    
    for (s32 i = 0; i < list->num; i++)
        val = Min(val, Sys_Stat(list->item[i]));
    
    return val;
}

s32 ItemList_SaveList(ItemList* target, const char* output) {
    MemFile mem = MemFile_Initialize();
    
    MemFile_Alloc(&mem, 512 * target->num);
    
    for (s32 i = 0; i < target->num; i++)
        MemFile_Printf(&mem, "%s\n", target->item[i]);
    if (MemFile_SaveFile_String(&mem, output))
        return 1;
    
    MemFile_Free(&mem);
    
    return 0;
}

s32 ItemList_NumericalSlotSort(ItemList* this, bool checkOverlaps) {
    bool error = false;
    s32 max = -1;
    ItemList new = ItemList_Initialize();
    
    if (this->num == 0)
        return 0;
    
    ItemList_SortNatural(this);
    
    for (s32 i = this->num - 1; i >= 0; i--) {
        if (!this->item[i]) continue;
        if (!isdigit(this->item[i][0])) continue;
        
        max = Value_Int(this->item[i]) + 1;
        break;
    }
    if (max <= 0) {
        printf_error("List with [%s] item didn't contain max!", this->item[0]);
    }
    Log("Max Num: %d", max);
    
    new.item = New(char*[max]);
    new.num = max;
    
    u32 j = 0;
    
    for (; j < this->num; j++) {
        if (!this->item[j]) continue;
        if (isdigit(this->item[j][0]))
            break;
    }
    Log("J: %d / %d", j, this->num - 1);
    
    if (checkOverlaps) {
        Log("Check Overlap");
        for (var i = 0; i < this->num - 1; i++) {
            if (isdigit(this->item[i][0]) && isdigit(this->item[i + 1][0])) {
                if (Value_Int(this->item[i]) == Value_Int(this->item[i + 1])) {
                    Log("" PRNT_REDD "![%s]!", this->item[i]);
                    ItemList_AddItem(&gList_SortError, this->item[i]);
                    ItemList_AddItem(&gList_SortError, this->item[i + 1]);
                    error = true;
                }
            }
        }
    }
    
    for (var i = 0; i < max; i++) {
        while (j < this->num && (!this->item[j] || !isdigit(this->item[j][0])))
            j++;
        
        if (j < this->num && i == Value_Int(this->item[j])) {
            Log("Insert Entry: [%s]", this->item[j]);
            new.item[i] = this->item[j];
            this->item[j++] = NULL;
        } else
            new.item[i] = NULL;
    }
    
    Log("Swap Tables");
    ItemList_Free(this);
    *this = new;
    
    Log("OK");
    
    if (checkOverlaps && gList_SortError.num) {
        printf_warning("Index overlap");
        for (var i = 0; i < gList_SortError.num; i += 2)
            printf_warning("[" PRNT_YELW "%s" PRNT_RSET "] vs "
                "[" PRNT_YELW "%s" PRNT_RSET "]",
                gList_SortError.item[i], gList_SortError.item[i + 1]);
        
        ItemList_Free(&gList_SortError);
    }
    
    return error;
}

void ItemList_FreeItems(ItemList* this) {
    if (this->initKey == 0xDEFABEBACECAFAFF) {
        if (this->item) {
            for (; this->num > 0; this->num--)
                Free(this->item[this->num - 1]);
            Free(this->item);
        }
        this->p.alnum = 0;
    } else
        *this = ItemList_Initialize();
}

void ItemList_Free(ItemList* this) {
    Assert(this != NULL);
    
    ItemList_FreeItems(this);
    ItemList_FreeFilters(this);
    
    *this = ItemList_Initialize();
}

void ItemList_Alloc(ItemList* this, u32 num) {
    ItemList_Free(this);
    
    this->p.alnum = num;
    this->item = New(char*[num]);
}

static void ItemList_Realloc(ItemList* this, u32 num) {
    this->p.alnum = num;
    this->item = Realloc(this->item, sizeof(char*[num]));
}

void ItemList_AddItem(ItemList* this, const char* item) {
    if (this->p.alnum && this->p.alnum < this->num + 1)
        ItemList_Realloc(this, this->p.alnum * 2 + 10);
    else
        this->item = Realloc(this->item, sizeof(char*[this->num + 1]));
    this->item[this->num++] = item ? strdup(item) : NULL;
}

void ItemList_Combine(ItemList* out, ItemList* a, ItemList* b) {
    ItemList_Free(out);
    
    ItemList_Alloc(out, a->num + b->num);
    
    for (var i = 0; i < a->num; i++)
        ItemList_AddItem(out, a->item[i]);
    
    for (var i = 0; i < b->num; i++)
        ItemList_AddItem(out, b->item[i]);
}

void ItemList_Tokenize(ItemList* this, const char* s, char r) {
    char* token;
    char sep[2] = { r };
    char* buf = strdup(s);
    
    ItemList_Free(this);
    
    Log("Tokenize: [%s]", s);
    Log("Separator: [%c]", r);
    
    token = strtok(buf, sep);
    while (token) {
        this->num++;
        Log("%d: %s", this->num - 1, token);
        token = strtok(NULL, sep);
    }
    
    if (!this->num) {
        Free(buf);
        return;
    }
    
    this->item = Calloc(sizeof(char*) * this->num);
    
    token = buf;
    for (s32 i = 0; i < this->num; i++) {
        char* end;
        
        this->item[i] = strdup(token + strspn(token, " "));
        token += strlen(token) + 1;
        while (( end = StrEnd(this->item[i], " ") ))
            *end = '\0';
    }
    
    Free(buf);
}

void ItemList_SortNatural(ItemList* this) {
    qsort(this->item, this->num, sizeof(char*), QSortCallback_Str_NumHex);
}
