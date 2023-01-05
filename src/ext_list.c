#include <ext_lib.h>
#include <dirent.h>

#undef ItemList_GetWildItem
#undef list_set_filters

#undef list_walk
#undef list_tokenize2
#undef list_free
#undef list_alloc
#undef list_add

// # # # # # # # # # # # # # # # # # # # #
// # ITEM LIST                           #
// # # # # # # # # # # # # # # # # # # # #

list_t gList_SortError;

static void list_validate(list_t* itemList) {
    if (itemList->initKey == 0xDEFABEBACECAFAFF)
        return;
    
    *itemList = list_new();
}

list_t list_new(void) {
    return (list_t) { .initKey = 0xDEFABEBACECAFAFF, .p.alnum = 0 };
}

void list_set_filters(list_t* list, u32 filterNum, ...) {
    va_list va;
    
    list_validate(list);
    list_free_filters(list);
    
    if (filterNum % 2 == 1)
        print_warn("Odd filterNum! [%d]", --filterNum);
    
    va_start(va, filterNum);
    
    for (s32 i = 0; i < filterNum * 0.5; i++) {
        FilterNode* node;
        
        node = calloc(sizeof(FilterNode));
        node->type = va_arg(va, ListFilter);
        node->txt = strdup(va_arg(va, char*));
        _assert(node->txt != NULL);
        
        Node_Add(list->filterNode, node);
    }
    
    va_end(va);
}

void list_free_filters(list_t* this) {
    if (this->initKey == 0xDEFABEBACECAFAFF) {
        while (this->filterNode) {
            free(this->filterNode->txt);
            
            Node_Kill(this->filterNode, this->filterNode);
        }
    } else
        *this = list_new();
}

typedef struct {
    str_node_t*       node;
    u32            num;
    const ListFlag flags;
} WalkInfo;

static void __list_walk(const list_t* list, const char* base, const char* parent, const s32 level, const s32 max, WalkInfo* info) {
    const char* entryPath = parent;
    
    if (info->flags & LIST_RELATIVE)
        entryPath = parent + strlen(base);
    
    DIR* dir;
    if (strlen(parent) == 0) dir = opendir("./");
    else dir = opendir(parent);
    
    if (dir) {
        const struct dirent* entry;
        
        while ((entry = readdir(dir))) {
            str_node_t* node;
            char path[261];
            bool skip = 0;
            bool filterContainUse = false;
            bool filterContainMatch = false;
            
            if (!memcmp(".\0", entry->d_name, 2) || !memcmp("..\0", entry->d_name, 3))
                continue;
            
            for (const FilterNode* filterNode = list->filterNode; filterNode != NULL; filterNode = filterNode->next) {
                switch (filterNode->type) {
                    case FILTER_SEARCH:
                        if (strstr(entry->d_name, filterNode->txt))
                            skip = true;
                        break;
                    case FILTER_START:
                        if (!memcmp(entry->d_name, filterNode->txt,
                            strlen(filterNode->txt)))
                            skip = true;
                        break;
                    case FILTER_END:
                        if (strend(entry->d_name, filterNode->txt))
                            skip = true;
                        break;
                    case FILTER_WORD:
                        if (!strcmp(entry->d_name, filterNode->txt))
                            skip = true;
                        break;
                        
                    case CONTAIN_SEARCH:
                        filterContainUse = true;
                        if (strstr(entry->d_name, filterNode->txt))
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
                        if (strend(entry->d_name, filterNode->txt))
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
            
            if (sys_isdir(path)) {
                strncat(path, "/", 261);
                
                if (max == -1 || level < max)
                    __list_walk(list, base, path, level + 1, max, info);
                
                if ((info->flags & 0xF) == LIST_FOLDERS) {
                    node = new(str_node_t);
                    node->txt = fmt("%s%s/", entryPath, entry->d_name);
                    info->num++;
                    
                    Node_Add(info->node, node);
                }
            } else {
                if (!filterContainUse || (filterContainUse && filterContainMatch)) {
                    if ((info->flags & 0xF) == LIST_FILES) {
                        node = new(str_node_t);
                        node->txt = fmt("%s%s", entryPath, entry->d_name);
                        info->num++;
                        
                        Node_Add(info->node, node);
                    }
                }
            }
        }
        
        closedir(dir);
    } else
        print_error("Could not open dir [%s]", dir);
}

void list_walk(list_t* this, const char* path, s32 depth, ListFlag flags) {
    char buf[261] = "";
    
    list_validate(this);
    
    _assert(path != NULL);
    if (strlen(path) > 0 ) {
        
        if (!sys_isdir(path))
            print_error("Can't walk path that does not exist! [%s]", path);
        
        strcpy(buf, path);
        if (!strend(buf, "/")) strcat(buf, "/");
    }
    
    WalkInfo info = { .flags = flags };
    
    _log("Walk: %s", buf);
    __list_walk(this, buf, buf, 0, depth, &info);
    
    if (info.num) {
        this->num = info.num;
        this->item = new(char*[info.num]);
        
        for (var_t i = 0; i < info.num; i++) {
            this->item[i] = info.node->txt;
            Node_Kill(info.node, info.node);
        }
    }
}

char* list_concat(list_t* this, const char* separator) {
    u32 len = 0;
    u32 seplen = separator ? strlen(separator) : 0;
    
    for (var_t i = 0; i < this->num; i++) {
        if (!this->item[i]) continue;
        len += strlen(this->item[i]) + seplen;
    }
    
    if (!len) return NULL;
    
    char* r = new(char[len + 1]);
    
    for (var_t i = 0; i < this->num; i++) {
        if (!this->item[i]) continue;
        
        strcat(r, this->item[i]);
        if (seplen) strcat(r, separator);
    }
    
    if (separator)
        strend(r, separator)[0] = '\0';
    
    return r;
}

void list_tokenize2(list_t* list, const char* str, const char separator) {
    s32 a = 0;
    s32 b = 0;
    str_node_t* nodeHead = NULL;
    
    list_free(list);
    
    while (str[a] == ' ' || str[a] == '\t' || str[a] == '\n' || str[a] == '\r') a++;
    
    while (true) {
        str_node_t* node = NULL;
        s32 isString = 0;
        s32 strcompns = 0;
        s32 brk = true;
        
        if (str[a] == '\"' || str[a] == '\'') {
            isString = 1;
            a++;
        }
        
        b = a;
        
        if (isString && (str[b] == '\"' || str[b] == '\'')) {
            node = calloc(sizeof(str_node_t));
            node->txt = calloc(2);
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
        
        node = calloc(sizeof(str_node_t));
        node->txt = calloc(b - a + strcompns + 1);
        memcpy(node->txt, &str[a], b - a + strcompns);
        Node_Add(nodeHead, node);
        
write:
        list->num++;
        
        a = b;
        
        while (str[a] == separator || str[a] == ' ' || str[a] == '\t' || str[a] == '\n' || str[a] == '\r') a++;
        if (str[a] == '\0')
            break;
    }
    
    list->item = new(char*[list->num]);
    
    for (s32 i = 0; i < list->num; i++) {
        list->item[i] = nodeHead->txt;
        Node_Kill(nodeHead, nodeHead);
    }
}

void list_print(list_t* target) {
    for (s32 i = 0; i < target->num; i++)
        printf("[#]: %4d: \"%s\"\n", i, target->item[i]);
}

time_t list_stat_max(list_t* list) {
    time_t val = 0;
    
    for (s32 i = 0; i < list->num; i++)
        val = max(val, sys_stat(list->item[i]));
    
    return val;
}

time_t list_stat_min(list_t* list) {
    time_t val = list_stat_max(list);
    
    for (s32 i = 0; i < list->num; i++)
        val = min(val, sys_stat(list->item[i]));
    
    return val;
}

s32 list_sort_slot(list_t* this, bool checkOverlaps) {
    bool error = false;
    s32 max = -1;
    list_t new = list_new();
    
    if (this->num == 0)
        return 0;
    
    list_sort(this);
    
    for (s32 i = this->num - 1; i >= 0; i--) {
        if (!this->item[i]) continue;
        if (!isdigit(this->item[i][0])) continue;
        
        max = sint(this->item[i]) + 1;
        break;
    }
    if (max <= 0) {
        print_error("List with [%s] item didn't contain max!", this->item[0]);
    }
    _log("max Num: %d", max);
    
    new.item = new(char*[max]);
    new.num = max;
    
    u32 j = 0;
    
    for (; j < this->num; j++) {
        if (!this->item[j]) continue;
        if (isdigit(this->item[j][0]))
            break;
    }
    _log("J: %d / %d", j, this->num - 1);
    
    if (checkOverlaps) {
        _log("Check Overlap");
        for (var_t i = 0; i < this->num - 1; i++) {
            if (isdigit(this->item[i][0]) && isdigit(this->item[i + 1][0])) {
                if (sint(this->item[i]) == sint(this->item[i + 1])) {
                    _log("" PRNT_REDD "![%s]!", this->item[i]);
                    list_add(&gList_SortError, this->item[i]);
                    list_add(&gList_SortError, this->item[i + 1]);
                    error = true;
                }
            }
        }
    }
    
    for (var_t i = 0; i < max; i++) {
        while (j < this->num && (!this->item[j] || !isdigit(this->item[j][0])))
            j++;
        
        if (j < this->num && i == sint(this->item[j])) {
            _log("Insert Entry: [%s]", this->item[j]);
            new.item[i] = this->item[j];
            this->item[j++] = NULL;
        } else
            new.item[i] = NULL;
    }
    
    _log("Swap Tables");
    list_free(this);
    *this = new;
    
    _log("OK");
    
    if (checkOverlaps && gList_SortError.num) {
        print_warn("Index overlap");
        for (var_t i = 0; i < gList_SortError.num; i += 2)
            print_warn("[" PRNT_YELW "%s" PRNT_RSET "] vs "
                "[" PRNT_YELW "%s" PRNT_RSET "]",
                gList_SortError.item[i], gList_SortError.item[i + 1]);
        
        list_free(&gList_SortError);
    }
    
    return error;
}

void list_free_items(list_t* this) {
    if (this->initKey == 0xDEFABEBACECAFAFF) {
        if (this->item) {
            for (; this->num > 0; this->num--)
                free(this->item[this->num - 1]);
            free(this->item);
        }
        this->p.alnum = 0;
    } else
        *this = list_new();
}

void list_free(list_t* this) {
    _assert(this != NULL);
    
    list_free_items(this);
    list_free_filters(this);
    
    *this = list_new();
}

void list_alloc(list_t* this, u32 num) {
    list_free(this);
    
    this->p.alnum = num;
    this->item = new(char*[num]);
}

static void list_realloc(list_t* this, u32 num) {
    this->p.alnum = num;
    this->item = realloc(this->item, sizeof(char*[num]));
}

void list_add(list_t* this, const char* item) {
    if (this->p.alnum && this->p.alnum < this->num + 1)
        list_realloc(this, this->p.alnum * 2 + 10);
    else
        this->item = realloc(this->item, sizeof(char*[this->num + 1]));
    this->item[this->num++] = item ? strdup(item) : NULL;
}

void list_combine(list_t* out, list_t* a, list_t* b) {
    list_free(out);
    
    list_alloc(out, a->num + b->num);
    
    for (var_t i = 0; i < a->num; i++)
        list_add(out, a->item[i]);
    
    for (var_t i = 0; i < b->num; i++)
        list_add(out, b->item[i]);
}

void list_tokenize(list_t* this, const char* s, char r) {
    char* token;
    char sep[2] = { r };
    char* buf = strdup(s);
    
    list_free(this);
    
    _log("Tokenize: [%s]", s);
    _log("Separator: [%c]", r);
    
    token = strtok(buf, sep);
    while (token) {
        this->num++;
        _log("%d: %s", this->num - 1, token);
        token = strtok(NULL, sep);
    }
    
    if (!this->num) {
        free(buf);
        return;
    }
    
    this->item = calloc(sizeof(char*) * this->num);
    
    token = buf;
    for (s32 i = 0; i < this->num; i++) {
        char* end;
        
        this->item[i] = strdup(token + strspn(token, " "));
        token += strlen(token) + 1;
        while (( end = strend(this->item[i], " ") ))
            *end = '\0';
    }
    
    free(buf);
}

void list_sort(list_t* this) {
    qsort(this->item, this->num, sizeof(char*), qsort_method_numhex);
}
