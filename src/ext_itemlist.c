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

typedef struct {
    StrNode* node;
    u32      len;
    u32      num;
    ListFlag flags;
} WalkInfo;

void ItemList_Validate(ItemList* itemList) {
    if (itemList->initKey == 0xDEFABEBACECAFAFF)
        return;
    
    *itemList = ItemList_Initialize();
}

ItemList ItemList_Initialize(void) {
    return (ItemList) { .initKey = 0xDEFABEBACECAFAFF };
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
        node->txt = StrDup(va_arg(va, char*));
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

static void ItemList_Walk(ItemList* list, const char* base, const char* parent, s32 level, s32 max, WalkInfo* info) {
    DIR* dir;
    const char* entryPath = parent;
    struct dirent* entry;
    
    if (info->flags & LIST_RELATIVE)
        entryPath = parent + strlen(base);
    
    if (strlen(parent) == 0)
        dir = opendir("./");
    else
        dir = opendir(parent);
    
    if (dir) {
        while ((entry = readdir(dir))) {
            StrNode* node;
            char path[261];
            u32 cont = 0;
            u32 containFlag = false;
            u32 contains = false;
            
            if (!strcmp(".", entry->d_name) || !strcmp("..", entry->d_name))
                continue;
            
            fornode(filterNode, list->filterNode) {
                switch (filterNode->type) {
                    case FILTER_SEARCH:
                        if (StrStr(entry->d_name, filterNode->txt))
                            cont = true;
                        break;
                    case FILTER_START:
                        if (!memcmp(entry->d_name, filterNode->txt, strlen(filterNode->txt)))
                            cont = true;
                        break;
                    case FILTER_END:
                        if (StrEnd(entry->d_name, filterNode->txt))
                            cont = true;
                        break;
                    case FILTER_WORD:
                        if (!strcmp(entry->d_name, filterNode->txt))
                            cont = true;
                        break;
                        
                    case CONTAIN_SEARCH:
                        containFlag = true;
                        if (StrStr(entry->d_name, filterNode->txt))
                            contains = true;
                        break;
                    case CONTAIN_START:
                        containFlag = true;
                        if (!memcmp(entry->d_name, filterNode->txt, strlen(filterNode->txt)))
                            contains = true;
                        break;
                    case CONTAIN_END:
                        containFlag = true;
                        if (StrEnd(entry->d_name, filterNode->txt))
                            contains = true;
                        break;
                    case CONTAIN_WORD:
                        containFlag = true;
                        if (!strcmp(entry->d_name, filterNode->txt))
                            contains = true;
                        break;
                }
                
                if (cont)
                    break;
            }
            
            if (((info->flags & LIST_NO_DOT) && entry->d_name[0] == '.') || cont)
                continue;
            
            snprintf(path, 261, "%s%s/", parent, entry->d_name);
            
            if (Sys_IsDir(path)) {
                if (max == -1 || level < max)
                    ItemList_Walk(list, base, path, level + 1, max, info);
                
                if ((info->flags & 0xF) == LIST_FOLDERS) {
                    node = Calloc(sizeof(StrNode));
                    node->txt = Alloc(261);
                    snprintf(node->txt, 261, "%s%s/", entryPath, entry->d_name);
                    info->len += strlen(node->txt) + 1;
                    info->num++;
                    
                    Node_Add(info->node, node);
                }
            } else {
                if (!containFlag || (containFlag && contains)) {
                    if ((info->flags & 0xF) == LIST_FILES) {
                        StrNode* node2;
                        
                        node2 = Calloc(sizeof(StrNode));
                        node2->txt = Alloc(261);
                        snprintf(node2->txt, 261, "%s%s", entryPath, entry->d_name);
                        info->len += strlen(node2->txt) + 1;
                        info->num++;
                        
                        Node_Add(info->node, node2);
                    }
                }
            }
        }
        
        closedir(dir);
    } else
        printf_error("Could not open dir [%s]", dir);
}

void ItemList_List(ItemList* target, const char* path, s32 depth, ListFlag flags) {
    WalkInfo info = { 0 };
    char* buf;
    
    ItemList_Validate(target);
    
    Assert(path != NULL);
    if (strlen(path) > 0 && !Sys_IsDir(path))
        printf_error("Can't walk path that does not exist! [%s]", path);
    buf = strndup(path, strlen(path) + 2);
    if (!StrEnd(buf, "/")) strcat(buf, "/");
    
    info.flags = flags;
    
    Log("Walk: %s", buf);
    ItemList_Walk(target, buf, buf, 0, depth, &info);
    
    if (info.num) {
        target->buffer = Alloc(info.len);
        target->item = Alloc(sizeof(char*) * info.num);
        target->num = info.num;
        
        for (s32 i = 0; i < info.num; i++) {
            StrNode* node = info.node;
            target->item[i] = &target->buffer[target->writePoint];
            strcpy(target->item[i], node->txt);
            target->writePoint += strlen(target->item[i]) + 1;
            
            Free(node->txt);
            Node_Kill(info.node, info.node);
        }
        Log("OK: %d", target->num);
    }
    
    Free(buf);
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
    
    ItemList_Validate(list);
    
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
        list->writePoint += strlen(node->txt) + 1;
        
        a = b;
        
        while (str[a] == separator || str[a] == ' ' || str[a] == '\t' || str[a] == '\n' || str[a] == '\r') a++;
        if (str[a] == '\0')
            break;
    }
    
    list->buffer = Calloc(list->writePoint);
    list->item = Calloc(sizeof(char*) * list->num);
    list->writePoint = 0;
    
    for (s32 i = 0; i < list->num; i++) {
        list->item[i] = &list->buffer[list->writePoint];
        strcpy(list->item[i], nodeHead->txt);
        
        list->writePoint += strlen(list->item[i]) + 1;
        
        Free(nodeHead->txt);
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

void ItemList_NumericalSort(ItemList* list) {
    ItemList sorted = ItemList_Initialize();
    u32 highestNum = 0;
    
    if (list->num < 2) {
        
        return;
    }
    
    for (s32 i = 0; i < list->num; i++)
        highestNum = Max(highestNum, Value_Int(list->item[i]));
    
    if (highestNum == 0)
        return;
    
    sorted.buffer = Calloc(list->writePoint * 4);
    sorted.item = Calloc(sizeof(char*) * (highestNum + 1));
    
    for (s32 i = 0; i <= highestNum; i++) {
        u32 null = true;
        
        for (s32 j = 0; j < list->num; j++) {
            if (isdigit(list->item[j][0]) && Value_Int(list->item[j]) == i) {
                sorted.item[sorted.num] = &sorted.buffer[sorted.writePoint];
                strcpy(sorted.item[sorted.num], list->item[j]);
                sorted.writePoint += strlen(sorted.item[sorted.num]) + 1;
                sorted.num++;
                null = false;
                break;
            }
        }
        
        if (null == true) {
            sorted.item[sorted.num++] = NULL;
        }
    }
    
    sorted.filterNode = list->filterNode;
    list->filterNode = NULL;
    ItemList_Free(list);
    
    *list = sorted;
}

static s32 SlotSort_GetNum(char* item, u32* ret) {
    u32 nonNum = false;
    char* num = item;
    
    while (!isdigit(*num)) {
        num++;
        if (*num == '\0') {
            nonNum = true;
            break;
        }
    }
    
    if (!nonNum) {
        *ret = Value_Int(num);
        
        return 0;
    }
    
    return 1;
}

s32 ItemList_NumericalSlotSort(ItemList* list, bool checkOverlaps) {
    s32 error = false;
    ItemList new = ItemList_Initialize();
    u32 max = 0;
    
    forlist(i, *list) {
        u32 num;
        
        if (!SlotSort_GetNum(list->item[i], &num))
            max = Max(num + 1, max);
    }
    
    if (max == 0)
        return error;
    
    ItemList_Alloc(&new, max, list->writePoint);
    
    for (s32 i = 0; i < max; i++) {
        u32 write = false;
        u32 num;
        forlist(j, *list) {
            if (list->item[j] && !SlotSort_GetNum(list->item[j], &num) && i == num) {
                ItemList_AddItem(&new, list->item[j]);
                list->item[j] = NULL;
                write = true;
                
                break;
            }
        }
        
        if (!write)
            ItemList_AddItem(&new, NULL);
    }
    
    if (checkOverlaps) {
        forlist(i, new) {
            u32 newNum = 0;
            u32 listNum = 0;
            
            if (new.item[i] == NULL)
                continue;
            
            SlotSort_GetNum(new.item[i], &newNum);
            
            forlist(j, *list) {
                if (list->item[j] == NULL)
                    continue;
                
                SlotSort_GetNum(list->item[j], &listNum);
                
                if (newNum != listNum)
                    continue;
                
                if (!strcmp(list->item[j], new.item[i]))
                    continue;
                
                if (error == 0)
                    ItemList_Alloc(&gList_SortError, new.num, list->writePoint);
                ItemList_AddItem(&gList_SortError, list->item[j]);
                error = true;
            }
        }
    }
    
    ItemList_Free(list);
    *list = new;
    
    return error;
}

void ItemList_FreeItems(ItemList* this) {
    if (this->initKey == 0xDEFABEBACECAFAFF) {
        Free(this->buffer);
        Free(this->item);
        this->num = 0;
        this->writePoint = 0;
    } else
        *this = ItemList_Initialize();
}

void ItemList_Free(ItemList* this) {
    Assert(this != NULL);
    
    ItemList_FreeItems(this);
    ItemList_FreeFilters(this);
    
    *this = ItemList_Initialize();
}

void ItemList_Alloc(ItemList* list, u32 num, Size size) {
    ItemList_Validate(list);
    ItemList_Free(list);
    list->num = 0;
    list->writePoint = 0;
    
    if (num == 0 || size == 0)
        return;
    
    list->item = Calloc(sizeof(char*) * num);
    list->buffer = Calloc(size);
    list->alnum = num;
}

void ItemList_AddItem(ItemList* list, const char* item) {
    if (item == NULL) {
        list->item[list->num++] = NULL;
        
        return;
    }
    
    u32 len = strlen(item);
    
    list->item[list->num] = &list->buffer[list->writePoint];
    strcpy(list->item[list->num], item);
    list->writePoint += len + 1;
    list->num++;
}

void ItemList_Combine(ItemList* out, ItemList* a, ItemList* b) {
    ItemList_Alloc(out, a->num + b->num, a->writePoint + b->writePoint);
    
    forlist(i, *a) {
        ItemList_AddItem(out, a->item[i]);
    }
    
    forlist(i, *b) {
        ItemList_AddItem(out, b->item[i]);
    }
}

void ItemList_Tokenize(ItemList* this, const char* s, char r) {
    char* token;
    char sep[2] = { r };
    
    ItemList_Free(this);
    
    Log("Tokenize: [%s]", s);
    Log("Separator: [%c]", r);
    
    Assert(( this->buffer = StrDup(s) ) != NULL);
    this->writePoint = strlen(this->buffer) + 1;
    
    token = strtok(this->buffer, sep);
    while (token) {
        this->num++;
        Log("%d: %s", this->num - 1, token);
        token = strtok(NULL, sep);
    }
    
    if (!this->num) return;
    this->item = Calloc(sizeof(char*) * this->num);
    
    token = this->buffer;
    for (s32 i = 0; i < this->num; i++) {
        char* end;
        
        this->item[i] = token + strspn(token, " ");
        token += strlen(token) + 1;
        while (( end = StrEnd(this->item[i], " ") ))
            *end = '\0';
    }
}

void ItemList_SortNatural(ItemList* this) {
    qsort(this->item, this->num, sizeof(char*), QSortCallback_Str_NumHex);
}
