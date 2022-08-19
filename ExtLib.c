#define __EXTLIB_C__

#define THIS_EXTLIB_VERSION 201

#ifndef EXTLIB
#error ExtLib Version not defined
#else
#if EXTLIB > THIS_EXTLIB_VERSION
#error Your local ExtLib copy seems to be old. Please update it!
#endif
#endif

#ifdef __IDE_FLAG__

#ifdef _WIN32
#undef _WIN32
#endif
void readlink(char*, char*, int);
void chdir(const char*);
void gettimeofday(void*, void*);
#endif

#ifndef _WIN32
#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE
#endif

#include "ExtLib.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <ftw.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <libloaderapi.h>
#endif

// # # # # # # # # # # # # # # # # # # # #
// # ExtLib Construct Destruct           #
// # # # # # # # # # # # # # # # # # # # #

__attribute__ ((constructor)) void ExtLib_Init(void) {
	Log_Init();
	
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
#endif
}

__attribute__ ((destructor)) void ExtLib_Destroy(void) {
	Log_Free();
}

// # # # # # # # # # # # # # # # # # # # #
// # THREAD                              #
// # # # # # # # # # # # # # # # # # # # #

static pthread_mutex_t ___sExt_ThreadLock;
static volatile bool ___sExt_ThreadInit;
static s32 sEXIT;

void ThreadLock_Init(void) {
	pthread_mutex_init(&___sExt_ThreadLock, NULL);
	___sExt_ThreadInit = true;
}

void ThreadLock_Free(void) {
	pthread_mutex_destroy(&___sExt_ThreadLock);
	___sExt_ThreadInit = false;
}

void ThreadLock_Lock(void) {
	if (___sExt_ThreadInit) {
		pthread_mutex_lock(&___sExt_ThreadLock);
	}
}

void ThreadLock_Unlock(void) {
	if (___sExt_ThreadInit) {
		pthread_mutex_unlock(&___sExt_ThreadLock);
	}
}

void ThreadLock_Create(Thread* thread, void* func, void* arg) {
	if (___sExt_ThreadInit == false) printf_error("Thread not Initialized");
	pthread_create(thread, NULL, (void*)func, (void*)(arg));
}

s32 ThreadLock_Join(Thread* thread) {
	u32 r = pthread_join(*thread, NULL);
	
	memset(thread, 0, sizeof(Thread));
	
	return r;
}

void Thread_Create(Thread* thread, void* func, void* arg) {
	pthread_create(thread, NULL, (void*)func, (void*)(arg));
}

s32 Thread_Join(Thread* thread) {
	u32 r = pthread_join(*thread, NULL);
	
	memset(thread, 0, sizeof(Thread));
	
	return r;
}

// # # # # # # # # # # # # # # # # # # # #
// # SEGMENT                             #
// # # # # # # # # # # # # # # # # # # # #

static u8* sSegment[255];

void SetSegment(const u8 id, void* segment) {
	sSegment[id] = segment;
}

void* SegmentedToVirtual(const u8 id, void32 ptr) {
	if (sSegment[id] == NULL)
		printf_error("Segment 0x%X == NULL", id);
	
	return &sSegment[id][ptr];
}

void32 VirtualToSegmented(const u8 id, void* ptr) {
	return (uptr)ptr - (uptr)sSegment[id];
}

// # # # # # # # # # # # # # # # # # # # #
// # TMP                                 #
// # # # # # # # # # # # # # # # # # # # #

static ThreadLocal struct {
	u8*  head;
	Size offset;
	
	// Maybe expandable in the future?
	Size max;
} sBufferX = {
	.max = MbToBin(4)
};

void* xAlloc(Size size) {
#pragma GCC diagnostic push
#ifndef __IDE_FLAG__
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
	u8* ret;
	
	if (size <= 0)
		return NULL;
	
	if (!sBufferX.head)
		qFree(sBufferX.head = malloc(sBufferX.max));
	
	if (sBufferX.offset + size + 1 >= sBufferX.max)
		sBufferX.offset = 0;
	
	ret = &sBufferX.head[sBufferX.offset];
	sBufferX.offset += size + 1;
	
	return memset(ret, 0, size + 1);
#pragma GCC diagnostic pop
}

char* xStrDup(const char* str) {
	return xMemDup(str, strlen(str) + 1);
}

char* xMemDup(const char* data, Size size) {
	if (!data || !size)
		return NULL;
	
	return memcpy(xAlloc(size), data, size);
}

char* xFmt(const char* fmt, ...) {
	char* tempBuf;
	char* r;
	
	va_list args;
	
	va_start(args, fmt);
	vasprintf(&tempBuf, fmt, args);
	va_end(args);
	
	r = xStrDup(tempBuf);
	tempBuf = Free(tempBuf);
	
	return r;
}

char* xRep(const char* str, const char* a, const char* b) {
	char* r = xAlloc(strlen(str) + (strlen(b) - strlen(a)) + 1 );
	
	strcpy(r, str);
	StrRep(r, a, b);
	
	return r;
}

// # # # # # # # # # # # # # # # # # # # #
// # TIME                                #
// # # # # # # # # # # # # # # # # # # # #

static struct timeval sTimeStart[255], sTimeStop[255];

void Time_Start(u32 slot) {
	gettimeofday(&sTimeStart[slot], NULL);
}

f64 Time_Get(u32 slot) {
	gettimeofday(&sTimeStop[slot], NULL);
	
	return (sTimeStop[slot].tv_sec - sTimeStart[slot].tv_sec) + (f32)(sTimeStop[slot].tv_usec - sTimeStart[slot].tv_usec) / 1000000;
}

typedef struct {
	struct timeval t;
	f32 ring[20];
	s8  k;
} ProfilerSlot;

struct {
	ProfilerSlot s[255];
} gProfilerCtx;

void Profiler_I(u8 s) {
	ProfilerSlot* p = &gProfilerCtx.s[s];
	
	gettimeofday(&p->t, 0);
}

void Profiler_O(u8 s) {
	struct timeval t;
	ProfilerSlot* p = &gProfilerCtx.s[s];
	
	gettimeofday(&t, 0);
	
	p->ring[p->k] = t.tv_sec - p->t.tv_sec + (f32)(t.tv_usec - p->t.tv_usec) * 0.000001f;
}

f32 Profiler_Time(u8 s) {
	ProfilerSlot* p = &gProfilerCtx.s[s];
	f32 sec = 0.0f;
	
	for (s32 i = 0; i < 20; i++)
		sec += p->ring[i];
	sec /= 20;
	p->k++;
	if (p->k >= 20)
		p->k = 0;
	
	return sec;
}

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

void ItemList_FreeFilters(ItemList* list) {
	while (list->filterNode) {
		Free(list->filterNode->txt);
		
		Node_Kill(list->filterNode, list->filterNode);
	}
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
			char* path;
			u32 cont = 0;
			u32 containFlag = false;
			u32 contains = false;
			
			if (!strcmp(".", entry->d_name) || !strcmp("..", entry->d_name))
				continue;
			
			fornode(FilterNode, filterNode, list->filterNode) {
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
			
			asprintf(&path, "%s%s/", parent, entry->d_name);
			
			if (Sys_IsDir(path)) {
				if (max == -1 || level < max)
					ItemList_Walk(list, base, path, level + 1, max, info);
				
				if ((info->flags & 0xF) == LIST_FOLDERS) {
					node = Calloc(sizeof(StrNode));
					asprintf(&node->txt, "%s%s/", entryPath, entry->d_name);
					info->len += strlen(node->txt) + 1;
					info->num++;
					
					Node_Add(info->node, node);
				}
			} else {
				if (!containFlag || (containFlag && contains)) {
					if ((info->flags & 0xF) == LIST_FILES) {
						StrNode* node2;
						
						node2 = Calloc(sizeof(StrNode));
						asprintf(&node2->txt, "%s%s", entryPath, entry->d_name);
						info->len += strlen(node2->txt) + 1;
						info->num++;
						
						Node_Add(info->node, node2);
					}
				}
			}
			
			Free(path);
		}
		
		closedir(dir);
	} else
		printf_error("Could not open dir [%s]", dir);
}

void ItemList_List(ItemList* target, const char* path, s32 depth, ListFlag flags) {
	WalkInfo info = { 0 };
	
	ItemList_Validate(target);
	
	if (strlen(path) > 0 && !Sys_Stat(path))
		printf_error("Can't walk path that does not exist! [%s]", path);
	
	info.flags = flags;
	
	ItemList_Walk(target, path, path, 0, depth, &info);
	
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
	
	Log("OK, %d [%s]", target->num, path);
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
			Log("String? %c%c", str[a], str[a + 1]);
			Log("a%d - b%d / %d", a, b, strlen(str));
			isString = 1;
			a++;
		}
		
		b = a;
		
		if (isString && (str[b] == '\"' || str[b] == '\'')) {
			Log("Empty String");
			node = Calloc(sizeof(StrNode));
			node->txt = Calloc(2);
			Node_Add(nodeHead, node);
			
			b++;
			
			goto write;
		}
		
		Log("char %02X", str[b], isString);
		Log("a%d - b%d / %d", a, b, strlen(str));
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
		Log("%d [%s]", b - a + 1, node->txt);
		Node_Add(nodeHead, node);
		
write:
		list->num++;
		list->writePoint += strlen(node->txt) + 1;
		
		a = b;
		Log("" PRNT_REDD "CONTINUE" PRNT_RSET ": [%s]", &str[a]);
		
		while (str[a] == separator || str[a] == ' ' || str[a] == '\t' || str[a] == '\n' || str[a] == '\r') a++;
		if (str[a] == '\0')
			break;
	}
	
	Log("Building List");
	
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
	
	Log("OK, %d [%s]", list->num, str);
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
		Log("Nothing to sort!");
		
		return;
	}
	
	Log("Sorting!");
	for (s32 i = 0; i < list->num; i++)
		highestNum = Max(highestNum, Value_Int(list->item[i]));
	
	Log("Num Max %d From %d Items", highestNum, list->num);
	
	if (highestNum == 0) {
		Log("Aborting Sorting");
		
		return;
	}
	
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
	
	Log("Sorted");
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

void ItemList_Free(ItemList* itemList) {
	if (itemList->initKey == 0xDEFABEBACECAFAFF) {
		Free(itemList->buffer);
		Free(itemList->item);
		ItemList_FreeFilters(itemList);
	}
	itemList[0] = ItemList_Initialize();
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
	
	forlist(i, *a)
	ItemList_AddItem(out, a->item[i]);
	forlist(i, *b)
	ItemList_AddItem(out, b->item[i]);
}

// # # # # # # # # # # # # # # # # # # # #
// # SYS                                 #
// # # # # # # # # # # # # # # # # # # # #

static ThreadLocal char* __sPath;
static ThreadLocal s32 __sMakeDir;

void FileSys_MakePath(s32 flag) {
	__sMakeDir = flag;
}

void FileSys_Path(const char* fmt, ...) {
	va_list va;
	
	Free(__sPath);
	va_start(va, fmt);
	vasprintf(&__sPath, fmt, va);
	va_end(va);
	
	if (__sMakeDir)
		Sys_MakeDir(__sPath);
}

char* FileSys_File(const char* str, ...) {
	char* buffer;
	char* ret;
	va_list va;
	
	va_start(va, str);
	vasprintf(&buffer, str, va);
	va_end(va);
	
	Log("%s", str);
	Assert(buffer != NULL);
	if (buffer[0] != '/' && buffer[0] != '\\')
		ret = xFmt("%s%s", __sPath, buffer);
	else
		ret = xStrDup(buffer + 1);
	Free(buffer);
	
	return ret;
}

char* FileSys_FindFile(const char* str) {
	ItemList list = ItemList_Initialize();
	char* file = NULL;
	Time stat = 0;
	
	if (*str == '*') str++;
	
	ItemList_List(&list, __sPath, 0, LIST_FILES | LIST_NO_DOT);
	for (s32 i = 0; i < list.num; i++) {
		if (StrEndCase(list.item[i], str) && Sys_Stat(list.item[i]) > stat) {
			file = list.item[i];
			stat = Sys_Stat(file);
		}
	}
	
	if (file)
		file = xStrDup(file);
	
	ItemList_Free(&list);
	
	return file;
}

void FileSys_Free() {
	Free(__sPath);
}

// # # # # # # # # # # # # # # # # # # # #
// # SYS                                 #
// # # # # # # # # # # # # # # # # # # # #

static s32 sSysIgnore;
static s32 sSysReturn;

bool Sys_IsDir(const char* path) {
	struct stat st = { 0 };
	
	stat(path, &st);
	
	if (S_ISDIR(st.st_mode)) {
		return true;
	}
	
	return false;
}

Time Sys_Stat(const char* item) {
	struct stat st = { 0 };
	Time t = 0;
	s32 free = 0;
	
	if (item == NULL)
		return 0;
	
	if (StrEnd(item, "/") || StrEnd(item, "\\")) {
		free = 1;
		item = StrDup(item);
		((char*)item)[strlen(item) - 1] = 0;
	}
	
	if (stat(item, &st) == -1) {
		if (free) Free(item);
		
		return 0;
	}
	
	// No access time
	// t = Max(st.st_atime, t);
	t = Max(st.st_mtime, t);
	t = Max(st.st_ctime, t);
	
	if (free) Free(item);
	
	return t;
}

Time Sys_StatF(const char* item, StatFlag flag) {
	struct stat st = { 0 };
	Time t = 0;
	
	if (item == NULL)
		return 0;
	
	if (stat(item, &st) == -1)
		return 0;
	
	if (flag & STAT_ACCS)
		t = Max(st.st_atime, t);
	
	if (flag & STAT_MODF)
		t = Max(st.st_mtime, t);
	
	if (flag & STAT_CREA)
		t = Max(st.st_ctime, t);
	
	return t;
}

char* Sys_ThisApp(void) {
	static char buf[512];
	
#ifdef _WIN32
	GetModuleFileName(NULL, buf, 512);
#else
	readlink("/proc/self/exe", buf, 512);
#endif
	
	return buf;
}

Time Sys_Time(void) {
	Time tme;
	
	time(&tme);
	
	return tme;
}

void Sys_Sleep(f64 sec) {
	struct timespec ts = { 0 };
	
	if (sec <= 0)
		return;
	
	ts.tv_sec = floor(sec);
	ts.tv_nsec = (sec - floor(sec)) * 1000000000;
	
	nanosleep(&ts, NULL);
}

static void __recursive_mkdir(const char* dir) {
	char tmp[256];
	char* p = NULL;
	Size len;
	
	snprintf(tmp, sizeof(tmp), "%s", dir);
	len = strlen(tmp);
	if (tmp[len - 1] == '/')
		tmp[len - 1] = 0;
	for (p = tmp + 1; *p; p++)
		if (*p == '/') {
			*p = 0;
			if (!Sys_Stat(tmp))
				mkdir(
					tmp
#ifndef _WIN32
					,
					S_IRWXU
#endif
				);
			*p = '/';
		}
	if (!Sys_Stat(tmp))
		mkdir(
			tmp
#ifndef _WIN32
			,
			S_IRWXU
#endif
		);
}

void __MakeDir(const char* buffer) {
	if (Sys_Stat(buffer))
		return;
	
	__recursive_mkdir(buffer);
	if (!Sys_Stat(buffer))
		Log("mkdir error: [%s]", buffer);
}

void Sys_MakeDir(const char* dir, ...) {
	char* buffer;
	va_list args;
	
	va_start(args, dir);
	vasprintf(&buffer, dir, args);
	va_end(args);
	
#ifdef _WIN32
	s32 i = 0;
	if (PathIsAbs(buffer))
		i = 3;
	
	for (; i < strlen(buffer); i++) {
		switch (buffer[i]) {
			case ':':
			case '*':
			case '?':
			case '"':
			case '<':
			case '>':
			case '|':
				printf_error("MakeDir: Can't make folder with illegal character! '%s'", buffer);
				break;
			default:
				break;
		}
	}
#endif
	
	if (!Sys_IsDir(dir)) {
		for (s32 i = strlen(buffer) - 1; i >= 0; i--) {
			if (buffer[i] == '/' || buffer[i] == '\\')
				break;
			buffer[i] = '\0';
		}
	}
	
	Log("[%s]", buffer);
	
	__MakeDir(buffer);
	
	Free(buffer);
}

const char* Sys_WorkDir(void) {
	static char buf[512];
	
	if (getcwd(buf, sizeof(buf)) == NULL) {
		printf_error("Could not get Sys_WorkDir");
	}
	
	for (s32 i = 0; i < strlen(buf); i++) {
		if (buf[i] == '\\')
			buf[i] = '/';
	}
	
	strcat(buf, "/");
	
	return buf;
}

const char* Sys_AppDir(void) {
	return Path(Sys_ThisApp());
}

s32 Sys_Rename(const char* input, const char* output) {
	if (Sys_Stat(output))
		Sys_Delete(output);
	
	return rename(input, output);
}

static s32 __rm_func(const char* item, const struct stat* bug, s32 type, struct FTW* ftw) {
	if (Sys_Delete(item))
		printf_error_align("Delete", "%s", item);
	
	return 0;
}

s32 Sys_Delete(const char* item) {
	if (Sys_IsDir(item))
		return rmdir(item);
	else
		return remove(item);
}

s32 Sys_Delete_Recursive(const char* item) {
	if (!Sys_IsDir(item))
		return 1;
	if (!Sys_Stat(item))
		return 0;
	if (nftw(item, __rm_func, 10, FTW_DEPTH | FTW_MOUNT | FTW_PHYS))
		printf_error("nftw error: %s %s", item, __FUNCTION__);
	
	return 0;
}

void Sys_SetWorkDir(const char* txt) {
	if (PathIsRel(txt))
		txt = PathAbs(txt);
	
	chdir(txt);
}

void SysExe_IgnoreError() {
	sSysIgnore = true;
}

s32 SysExe_GetError() {
	return sSysReturn;
}

s32 SysExe(const char* cmd) {
	s32 ret;
	
	ret = system(cmd);
	
	if (ret != 0)
		Log(PRNT_REDD "[%d] " PRNT_GRAY "SysExe(" PRNT_REDD "%s" PRNT_GRAY ");", ret, cmd);
	
	return ret;
}

char* SysExeO(const char* cmd) {
	char* result;
	MemFile mem = MemFile_Initialize();
	FILE* file;
	
	if ((file = popen(cmd, "r")) == NULL) {
		Log(PRNT_REDD "SysExeO(%s);", cmd);
		Log("popen failed!");
		
		return NULL;
	}
	
	MemFile_Params(&mem, MEM_REALLOC, true, MEM_END);
	MemFile_Alloc(&mem, MbToBin(1.0));
	
	result = Alloc(1025);
	while (fgets(result, 1024, file))
		MemFile_Write(&mem, result, strlen(result));
	Free(result);
	
	if ((sSysReturn = pclose(file)) != 0) {
		if (sSysIgnore == 0) {
			printf("%s\n", mem.str);
			Log(PRNT_REDD "[%d] " PRNT_GRAY "SysExeO(" PRNT_REDD "%s" PRNT_GRAY ");", sSysReturn, cmd);
			printf_error("SysExeO");
		}
	}
	
	sSysIgnore = 0;
	
	return mem.data;
}

void Sys_TerminalSize(s32* r) {
	s32 x = 0;
	s32 y = 0;
	
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	x = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
#ifndef __IDE_FLAG__
#include <sys/ioctl.h>
	struct winsize w;
	
	ioctl(0, TIOCGWINSZ, &w);
	
	x = w.ws_col;
	y = w.ws_row;
#endif
#endif
	
	r[0] = x;
	r[1] = y;
}

s32 Sys_Touch(const char* file) {
#include <utime.h>
	struct stat st;
	struct utimbuf nTime;
	
	stat(file, &st);
	nTime.actime = st.st_atime;
	nTime.modtime = time(NULL);
	utime(file, &nTime);
	
	return 0;
}

s32 Sys_Copy(const char* src, const char* dest) {
	MemFile a = MemFile_Initialize();
	
	if (MemFile_LoadFile(&a, src))
		return -1;
	if (MemFile_SaveFile(&a, dest))
		return 1;
	MemFile_Free(&a);
	
	return 0;
}

Date Sys_Date(Time time) {
	Date date = { 0 };
	
#ifndef __IDE_FLAG__
	struct tm* tistr = localtime(&time);
	
	date.year = tistr->tm_year + 1900;
	date.month = tistr->tm_mon + 1;
	date.day = tistr->tm_mday;
	date.hour = tistr->tm_hour;
	date.minute = tistr->tm_min;
	date.second = tistr->tm_sec;
#endif
	
	return date;
}

// # # # # # # # # # # # # # # # # # # # #
// # TERMINAL                            #
// # # # # # # # # # # # # # # # # # # # #

s32 Terminal_YesOrNo(void) {
	char ans[512] = { 0 };
	u32 clear = 0;
	
	while (strcmp(ans, "y\n") && strcmp(ans, "n\n") && strcmp(ans, "Y\n") && strcmp(ans, "N\n")) {
		if (clear) {
			Terminal_ClearLines(2);
		}
		
		printf("\r" PRNT_GRAY "[" PRNT_DGRY "<" PRNT_GRAY "]: " PRNT_BLUE);
		fgets(ans, 511, stdin);
		clear = 1;
		
		if (sEXIT)
			exit(1);
	}
	
	if (ans[0] == 'N' || ans[0] == 'n') {
		Terminal_ClearLines(2);
		
		return false;
	}
	Terminal_ClearLines(2);
	
	return true;
}

void Terminal_ClearScreen(void) {
	printf("\033[2J");
}

void Terminal_ClearLines(u32 i) {
	printf("\x1b[2K");
	for (s32 j = 1; j < i; j++) {
		Terminal_Move_PrevLine();
		printf("\x1b[2K");
	}
}

void Terminal_Move_PrevLine(void) {
	printf("\x1b[1F");
}

void Terminal_Move(s32 x, s32 y) {
	if (y < 0)
		printf("\033[%dA", -y);
	else
		printf("\033[%dB", y);
	if (x < 0)
		printf("\033[%dD", -x);
	else
		printf("\033[%dC", x);
}

const char* Terminal_GetStr(void) {
	static char str[512] = { 0 };
	
	printf("\r" PRNT_GRAY "[" PRNT_DGRY "<" PRNT_GRAY "]: " PRNT_RSET);
	fgets(str, 511, stdin);
	str[strlen(str) - 1] = '\0'; // remove newline
	
	Log("[%s]", str);
	
	return str;
}

char Terminal_GetChar() {
	printf("\r" PRNT_GRAY "[" PRNT_DGRY "<" PRNT_GRAY "]: " PRNT_RSET);
	
	return getchar();
}

// # # # # # # # # # # # # # # # # # # # #
// # VARIOUS                             #
// # # # # # # # # # # # # # # # # # # # #

static s32 sRandInit;

void __Assert(s32 expression, const char* msg, ...) {
	if (!expression) {
		char* buf;
		va_list va;
		
		va_start(va, msg);
		vasprintf(&buf, msg, va);
		va_end(va);
		
		printf_error("%s", buf);
	}
}

f32 RandF() {
	if (sRandInit == 0) {
		sRandInit++;
		srand(time(NULL));
	}
	
	for (s32 i = 0; i < 128; i++)
		srand(rand());
	
	f64 r = rand() / (f32)__INT16_MAX__;
	
	return fmod(r, 1.0f);
}

void* MemMem(const void* haystack, Size haystacklen, const void* needle, Size needlelen) {
	char* bf = (char*) haystack, * pt = (char*) needle, * p = bf;
	
	if (haystacklen < needlelen || !haystack || !needle || !haystacklen || !needlelen)
		return NULL;
	
	while ((haystacklen - (p - bf)) >= needlelen) {
		if (NULL != (p = memchr(p, (s32)(*pt), haystacklen - (p - bf)))) {
			if (!memcmp(p, needle, needlelen))
				return p;
			++p;
		} else
			break;
	}
	
	return NULL;
}

void* StrStr(const char* haystack, const char* needle) {
	if (!haystack || !needle)
		return NULL;
	
	return MemMem(haystack, strlen(haystack), needle, strlen(needle));
}

void* StrStrWhole(const char* haystack, const char* needle) {
	char* p = StrStr(haystack, needle);
	
	while (p) {
		if (!isgraph(p[-1]) && !isgraph(p[strlen(needle)]))
			return p;
		
		p = StrStr(p + 1, needle);
	}
	
	return NULL;
}

void* StrStrCase(const char* haystack, const char* needle) {
	char* bf = (char*) haystack, * pt = (char*) needle, * p = bf;
	
	if (!haystack || !needle)
		return NULL;
	
	u32 haystacklen = strlen(haystack);
	u32 needlelen = strlen(needle);
	
	while (needlelen <= (haystacklen - (p - bf))) {
		char* a, * b;
		
		a = memchr(p, tolower((s32)(*pt)), haystacklen - (p - bf));
		b = memchr(p, toupper((s32)(*pt)), haystacklen - (p - bf));
		
		if (a == NULL)
			p = b;
		else if (b == NULL)
			p = a;
		else
			p = Min(a, b);
		
		if (p) {
			if (0 == strnicmp(p, needle, needlelen))
				return p;
			++p;
		} else
			break;
	}
	
	return NULL;
}

void* MemStrCase(const char* haystack, u32 haystacklen, const char* needle) {
	char* bf = (char*) haystack, * pt = (char*) needle, * p = bf;
	
	if (!haystack || !needle)
		return NULL;
	
	u32 needlelen = strlen(needle);
	
	while (needlelen <= (haystacklen - (p - bf))) {
		char* a, * b;
		
		a = memchr(p, tolower((s32)(*pt)), haystacklen - (p - bf));
		b = memchr(p, toupper((s32)(*pt)), haystacklen - (p - bf));
		
		if (a == NULL)
			p = b;
		else if (b == NULL)
			p = a;
		else
			p = Min(a, b);
		
		if (p) {
			if (0 == strnicmp(p, needle, needlelen))
				return p;
			++p;
		} else
			break;
	}
	
	return NULL;
}

void* MemMemAlign(u32 val, const void* haystack, Size haystacklen, const void* needle, Size needlelen) {
	char* s = (char*)needle;
	char* bf = (char*)haystack;
	char* p = (char*)haystack;
	
	if (haystacklen < needlelen || !haystack || !needle || !haystacklen || !needlelen || !val)
		return NULL;
	
	while (haystacklen - (p - bf) >= needlelen) {
		if (p[0] == s[0] && !memcmp(p, needle, needlelen))
			return p;
		p += val;
	}
	
	return NULL;
}

char* StrEnd(const char* src, const char* ext) {
	char* fP;
	
	if (strlen(src) < strlen(ext))
		return NULL;
	
	fP = (char*)(src + strlen(src) - strlen(ext));
	
	if (!strcmp(fP, ext))
		return fP;
	
	return NULL;
}

char* StrEndCase(const char* src, const char* ext) {
	char* fP;
	
	if (strlen(src) < strlen(ext))
		return NULL;
	
	fP = (char*)(src + strlen(src) - strlen(ext));
	
	if (!stricmp(fP, ext))
		return fP;
	
	return NULL;
}

void ByteSwap(void* src, s32 size) {
	u32 buffer[64] = { 0 };
	u8* temp = (u8*)buffer;
	u8* srcp = src;
	
	for (s32 i = 0; i < size; i++) {
		temp[size - i - 1] = srcp[i];
	}
	
	for (s32 i = 0; i < size; i++) {
		srcp[i] = temp[i];
	}
}

void* Alloc(s32 size) {
	return malloc(size);
}

void* Calloc(s32 size) {
	return calloc(1, size);
}

void* Realloc(const void* data, s32 size) {
	return realloc((void*)data, size);
}

void* Free(const void* data) {
	if (data)
		free((void*)data);
	
	return NULL;
}

void* MemDup(const void* src, Size size) {
	if (src == NULL)
		return NULL;
	
	return memcpy(Alloc(size), src, size);
}

char* StrDup(const char* src) {
	if (src == NULL)
		return NULL;
	
	return MemDup(src, strlen(src) + 1);
}

char* StrDupX(const char* src, Size size) {
	if (src == NULL)
		return NULL;
	
	return strcpy(Alloc(Max(size, strlen(src) + 1)), src);
}

char* StrDupClp(const char* str, u32 max) {
	char* r = MemDup(str, max + 1);
	
	r[max] = '\0';
	
	return r;
}

char* Fmt(const char* fmt, ...) {
	char* s;
	va_list va;
	
	va_start(va, fmt);
	vasprintf(&s, fmt, va);
	va_end(va);
	
	return s;
}

s32 ParseArgs(char* argv[], char* arg, u32* parArg) {
	char* s = xFmt("%s", arg);
	char* ss = xFmt("-%s", arg);
	char* sss = xFmt("--%s", arg);
	char* tst[] = {
		s, ss, sss
	};
	
	for (s32 i = 1; argv[i] != NULL; i++) {
		for (s32 j = 0; j < ArrayCount(tst); j++) {
			if (strlen(argv[i]) == strlen(tst[j]))
				if (!strcmp(argv[i], tst[j])) {
					if (parArg != NULL)
						*parArg = i + 1;
					
					return i + 1;
				}
		}
	}
	
	return 0;
}

void SlashAndPoint(const char* src, s32* slash, s32* point) {
	s32 strSize = strlen(src);
	
	*slash = 0;
	*point = 0;
	
	for (s32 i = strSize; i > 0; i--) {
		if (*point == 0 && src[i] == '.') {
			*point = i;
		}
		if (src[i] == '/' || src[i] == '\\') {
			*slash = i;
			break;
		}
	}
}

char* Path(const char* src) {
	char* buffer;
	s32 point;
	s32 slash;
	
	if (src == NULL)
		return NULL;
	
	SlashAndPoint(src, &slash, &point);
	
	if (slash == 0)
		slash = -1;
	
	buffer = xAlloc(slash + 1 + 1);
	memcpy(buffer, src, slash + 1);
	buffer[slash + 1] = '\0';
	
	return buffer;
}

char* PathSlot(const char* src, s32 num) {
	char* buffer;
	s32 start = -1;
	s32 end;
	
	if (src == NULL)
		return NULL;
	
	if (num < 0) {
		num = PathNum(src) - 1;
	}
	
	for (s32 temp = 0;;) {
		if (temp >= num)
			break;
		if (src[start + 1] == '/')
			temp++;
		start++;
	}
	start++;
	end = start + 1;
	
	while (src[end] != '/') {
		if (src[end] == '\0')
			printf_error("Could not solve folder for [%s]", src);
		end++;
	}
	end++;
	
	buffer = xAlloc(end - start + 1);
	
	memcpy(buffer, &src[start], end - start);
	buffer[end - start] = '\0';
	
	return buffer;
}

char* StrChrAcpt(const char* str, char* c) {
	char* v = (char*)str;
	u32 an = strlen(c);
	
	if (!v) return NULL;
	else v = strchr(str, '\0');
	
	while (--v >= str)
		for (s32 i = 0; i < an; i++)
			if (*v == c[i])
				return v;
	
	return NULL;
}

char* Basename(const char* src) {
	char* ls = StrChrAcpt(src, "\\/");
	
	if (!ls++)
		ls = (char*)src;
	
	return xMemDup(ls, strcspn(ls, "."));
}

char* Filename(const char* src) {
	char* ls = StrChrAcpt(src, "\\/");
	
	if (!ls++)
		ls = (char*)src;
	
	return xStrDup(ls);
}

char* LineHead(const char* str, const char* head) {
	s32 i = 1;
	
	for (;; i--) {
		if (&str[i] == head)
			return (char*)&str[i];
		if (str[i - 1] == '\n' || str[i - 1] == '\r')
			return (char*)&str[i];
	}
}

char* Line(const char* str, s32 line) {
	const char* ln = str;
	
	if (!str)
		return NULL;
	
	while (line--) {
		ln = strpbrk(ln, "\n\r");
		
		if (!ln++)
			return NULL;
	}
	
	return (char*)ln;
}

char* Word(const char* str, s32 word) {
	if (!str)
		return NULL;
	
	while (!isgraph(*str)) str++;
	while (word--) {
		if (!(str = strpbrk(str, " \t\n\r"))) return NULL;
		while (!isgraph(*str)) str++;
	}
	
	return (char*)str;
}

Size LineLen(const char* str) {
	return strcspn(str, "\n\r");
}

Size WordLen(const char* str) {
	return strcspn(str, " \t\n\r");
}

char* FileExtension(const char* str) {
	s32 slash;
	s32 point;
	
	SlashAndPoint(str, &slash, &point);
	
	return (void*)&str[point];
}

void CaseToLow(char* s, s32 i) {
	if (i <= 0)
		i = strlen(s);
	
	for (s32 k = 0; k < i; k++) {
		if (s[k] >= 'A' && s[k] <= 'Z') {
			s[k] = s[k] + 32;
		}
	}
}

void CaseToUp(char* s, s32 i) {
	if (i <= 0)
		i = strlen(s);
	
	for (s32 k = 0; k < i; k++) {
		if (s[k] >= 'a' && s[k] <= 'z') {
			s[k] = s[k] - 32;
		}
	}
}

s32 LineNum(const char* str) {
	s32 num = 1;
	
	while (*str != '\0') {
		if (*str == '\n' || *str == '\r')
			num++;
		str++;
	}
	
	return num;
}

s32 PathNum(const char* src) {
	s32 dir = -1;
	
	for (s32 i = 0; i < strlen(src); i++) {
		if (src[i] == '/')
			dir++;
	}
	
	return dir + 1;
}

char* CopyLine(const char* str, s32 line) {
	if (!(str = Line(str, line))) return NULL;
	
	return xMemDup(str, LineLen(str));
}

char* CopyWord(const char* str, s32 word) {
	if (!(str = Word(str, word))) return NULL;
	
	return xMemDup(str, WordLen(str));
}

char* PathRel_From(const char* from, const char* item) {
	item = StrSlash(StrUnq(item));
	char* work = StrSlash(StrDup(from));
	s32 lenCom = StrComLen(work, item);
	s32 subCnt = 0;
	char* sub = (char*)&work[lenCom];
	char* fol = (char*)&item[lenCom];
	char* buffer = xAlloc(strlen(work) + strlen(item));
	
	forstr(i, sub) {
		if (sub[i] == '/' || sub[i] == '\\')
			subCnt++;
	}
	
	for (s32 i = 0; i < subCnt; i++)
		strcat(buffer, "../");
	
	strcat(buffer, fol);
	
	return buffer;
}

char* PathAbs_From(const char* from, const char* item) {
	item = StrSlash(StrUnq(item));
	char* path = StrSlash(xStrDup(from));
	char* t = StrStr(item, "../");
	char* f = (char*)item;
	s32 subCnt = 0;
	
	while (t) {
		f = &f[strlen("../")];
		subCnt++;
		t = StrStr(t + 1, "../");
	}
	
	for (s32 i = 0; i < subCnt; i++) {
		path[strlen(path) - 1] = '\0';
		path = Path(path);
	}
	
	return xFmt("%s%s", path, f);
}

char* PathRel(const char* item) {
	return PathRel_From(Sys_WorkDir(), item);
}

char* PathAbs(const char* item) {
	return PathAbs_From(Sys_WorkDir(), item);
}

s32 PathIsAbs(const char* item) {
	while (item[0] == '\'' || item[0] == '\"')
		item++;
	
	if (isalpha(item[0]) && item[1] == ':' && (item[2] == '/' || item[2] == '\\')) {
		
		return 1;
	}
	if (item[0] == '/' || item[0] == '\\') {
		
		return 1;
	}
	
	return 0;
}

s32 PathIsRel(const char* item) {
	return !PathIsAbs(item);
}

// # # # # # # # # # # # # # # # # # # # #
// # On Exit                             #
// # # # # # # # # # # # # # # # # # # # #

typedef struct PostFreeNode {
	struct PostFreeNode* prev;
	struct PostFreeNode* next;
	void* ptr;
} PostFreeNode;

static PostFreeNode* sPostFreeHead;
static s32 sOnExitFlag;

static void __PostFree(s32 i, void* arg) {
	while (sPostFreeHead) {
		Free(sPostFreeHead->ptr);
		Node_Kill(sPostFreeHead, sPostFreeHead);
	}
}

void* qFree(const void* ptr) {
	PostFreeNode* node;
	
	if (!sOnExitFlag) {
		sOnExitFlag++;
#ifdef _WIN32
		if (!_onexit((void*)__PostFree))
#else
		if (on_exit(__PostFree, NULL))
#endif
			printf_error("Could not init OnExit");
	}
	
	node = Calloc(sizeof(struct PostFreeNode));
	node->ptr = (void*)ptr;
	
	Node_Add(sPostFreeHead, node);
	
	return (void*)ptr;
}

// # # # # # # # # # # # # # # # # # # # #
// # COLOR                               #
// # # # # # # # # # # # # # # # # # # # #

void Color_ToHSL(HSL8* dest, RGB8* src) {
	f32 r, g, b;
	f32 cmax, cmin, d;
	
	r = (f32)src->r / 255;
	g = (f32)src->g / 255;
	b = (f32)src->b / 255;
	
	cmax = fmax(r, (fmax(g, b)));
	cmin = fmin(r, (fmin(g, b)));
	dest->l = (cmax + cmin) / 2;
	d = cmax - cmin;
	
	if (cmax == cmin)
		dest->h = dest->s = 0;
	else {
		dest->s = dest->l > 0.5 ? d / (2 - cmax - cmin) : d / (cmax + cmin);
		
		if (cmax == r) {
			dest->h = (g - b) / d + (g < b ? 6 : 0);
		} else if (cmax == g) {
			dest->h = (b - r) / d + 2;
		} else if (cmax == b) {
			dest->h = (r - g) / d + 4;
		}
		dest->h /= 6.0;
	}
}

static f32 hue2rgb(f32 p, f32 q, f32 t) {
	if (t < 0.0) t += 1;
	if (t > 1.0) t -= 1;
	if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
	if (t < 1.0 / 2.0) return q;
	if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
	
	return p;
}

void Color_ToRGB(RGB8* dest, HSL8* src) {
	if (src->s == 0) {
		dest->r = dest->g = dest->b = src->l;
	} else {
		f32 q = src->l < 0.5 ? src->l * (1 + src->s) : src->l + src->s - src->l * src->s;
		f32 p = 2.0 * src->l - q;
		dest->r = hue2rgb(p, q, src->h + 1.0 / 3.0) * 255;
		dest->g = hue2rgb(p, q, src->h) * 255;
		dest->b = hue2rgb(p, q, src->h - 1.0 / 3.0) * 255;
	}
}

// # # # # # # # # # # # # # # # # # # # #
// # MEMFILE                             #
// # # # # # # # # # # # # # # # # # # # #

static FILE* MemFOpen(const char* name, const char* mode) {
	FILE* file;
	
#if _WIN32
	file = _wfopen(StrU16(0, name), StrU16(0, mode));
#else
	file = fopen(name, mode);
#endif
	
	return file;
}

void MemFile_Validate(MemFile* mem) {
	if (mem->param.initKey == 0xD0E0A0D0B0E0E0F0) {
		MemFile_Reset(mem);
		
		return;
	}
	
	*mem = MemFile_Initialize();
}

MemFile MemFile_Initialize() {
	return (MemFile) { .param.initKey = 0xD0E0A0D0B0E0E0F0 };
}

void MemFile_Params(MemFile* memFile, ...) {
	va_list args;
	u32 cmd;
	u32 arg;
	
	va_start(args, memFile);
	for (;;) {
		cmd = va_arg(args, uptr);
		
		if (cmd == MEM_END) {
			break;
		}
		
		arg = va_arg(args, uptr);
		
		if (cmd == MEM_CLEAR) {
			cmd = arg;
			arg = 0;
		}
		
		switch (cmd) {
			case MEM_ALIGN:
				memFile->param.align = arg;
				break;
			case MEM_CRC32:
				memFile->param.getCrc = arg != 0 ? true : false;
				break;
			case MEM_REALLOC:
				memFile->param.realloc = arg != 0 ? true : false;
				break;
		}
	}
	va_end(args);
}

void MemFile_Alloc(MemFile* memFile, u32 size) {
	if (memFile->param.initKey != 0xD0E0A0D0B0E0E0F0) {
		*memFile = MemFile_Initialize();
	} else if (memFile->data) {
		Log("MemFile_Alloc: Mallocing already allocated MemFile [%s], freeing and reallocating!", memFile->info.name);
		MemFile_Free(memFile);
	}
	
	memFile->data = Calloc(size);
	
	if (memFile->data == NULL)
		printf_error("Failed to malloc [0x%X] bytes.", size);
	
	memFile->memSize = size;
}

void MemFile_Realloc(MemFile* memFile, u32 size) {
	if (memFile->param.initKey != 0xD0E0A0D0B0E0E0F0) {
		*memFile = MemFile_Initialize();
		MemFile_Alloc(memFile, size);
		
		return;
	}
	
	if (memFile->memSize > size)
		return;
	
	memFile->data = Realloc(memFile->data, size);
	memFile->memSize = size;
}

void MemFile_Rewind(MemFile* memFile) {
	memFile->seekPoint = 0;
}

s32 MemFile_Write(MemFile* dest, void* src, u32 size) {
	u32 osize = size;
	
	if (src == NULL) {
		printf_warning("MemFile: src is NULL");
		
		return -1;
	}
	
	size = ClampMax(size, ClampMin(dest->memSize - dest->seekPoint, 0));
	
	if (size != osize) {
		if (dest->param.realloc) {
			MemFile_Realloc(dest, dest->memSize + dest->memSize);
			size = osize;
		} else {
			printf_warning("MemFile: Wrote %.2fkB instead of %.2fkB", BinToKb(size), BinToKb(osize));
		}
	}
	
	memcpy(&dest->cast.u8[dest->seekPoint], src, size);
	dest->seekPoint += size;
	dest->size = Max(dest->size, dest->seekPoint);
	
	if (dest->param.align)
		if ((dest->seekPoint % dest->param.align) != 0)
			MemFile_Align(dest, dest->param.align);
	
	return size;
}

/*
 * If pos is 0 or bigger: override seekPoint
 */
s32 MemFile_Insert(MemFile* mem, void* src, u32 size, s64 pos) {
	u32 p = pos < 0 ? mem->seekPoint : pos;
	u32 remasize = mem->size - p;
	
	if (p + size + remasize >= mem->memSize) {
		if (mem->param.realloc) {
			MemFile_Realloc(mem, mem->memSize * 2);
		} else {
			printf_error("MemFile ran out of space");
		}
	}
	
	memmove(&mem->cast.u8[p + remasize], &mem->cast.u8[p], remasize);
	
	return MemFile_Write(mem, src, size);
}

s32 MemFile_Append(MemFile* dest, MemFile* src) {
	return MemFile_Write(dest, src->data, src->size);
}

void MemFile_Align(MemFile* src, u32 align) {
	if ((src->seekPoint % align) != 0) {
		u64 wow[2] = { 0 };
		u32 size = align - (src->seekPoint % align);
		
		MemFile_Write(src, wow, size);
	}
}

s32 MemFile_Printf(MemFile* dest, const char* fmt, ...) {
	char* buffer;
	va_list args;
	
	va_start(args, fmt);
	vasprintf(
		&buffer,
		fmt,
		args
	);
	va_end(args);
	
	return ({
		s32 ret = MemFile_Write(dest, buffer, strlen(buffer));
		Free(buffer);
		ret;
	});
}

s32 MemFile_Read(MemFile* src, void* dest, Size size) {
	Size nsize = ClampMax(size, ClampMin(src->size - src->seekPoint, 0));
	
	if (nsize != size)
		Log("%d == src->seekPoint = %d / %d", nsize, src->seekPoint, src->seekPoint);
	
	if (nsize < 1)
		return 0;
	
	memcpy(dest, &src->cast.u8[src->seekPoint], nsize);
	src->seekPoint += nsize;
	
	return nsize;
}

void* MemFile_Seek(MemFile* src, u32 seek) {
	if (seek == MEMFILE_SEEK_END)
		seek = src->size;
	
	if (seek > src->memSize) {
		return NULL;
	}
	src->seekPoint = seek;
	
	return (void*)&src->cast.u8[seek];
}

void MemFile_LoadMem(MemFile* mem, void* data, Size size) {
	MemFile_Validate(mem);
	mem->size = mem->memSize = size;
	mem->data = data;
}

s32 MemFile_LoadFile(MemFile* memFile, const char* filepath) {
	FILE* file = MemFOpen(filepath, "rb");
	u32 tempSize;
	
	if (file == NULL) {
		printf_warning("Failed to open file [%s]", filepath);
		
		return 1;
	}
	
	fseek(file, 0, SEEK_END);
	tempSize = ftell(file);
	
	MemFile_Validate(memFile);
	if (memFile->data == NULL) {
		MemFile_Alloc(memFile, tempSize);
		memFile->memSize = memFile->size = tempSize;
		if (memFile->data == NULL) {
			printf_warning("Failed to malloc MemFile.\n\tAttempted size is [0x%X] bytes to store data from [%s].", tempSize, filepath);
			
			return 1;
		}
	} else {
		if (memFile->memSize < tempSize)
			MemFile_Realloc(memFile, tempSize * 2);
		memFile->size = tempSize;
	}
	
	rewind(file);
	if (fread(memFile->data, 1, memFile->size, file)) {
	}
	fclose(file);
	
	memFile->info.age = Sys_Stat(filepath);
	strcpy(memFile->info.name, filepath);
	
	return 0;
}

s32 MemFile_LoadFile_String(MemFile* memFile, const char* filepath) {
	FILE* file = MemFOpen(filepath, "r");
	u32 tempSize;
	
	if (file == NULL) {
		printf_warning("Failed to open file [%s]", filepath);
		
		return 1;
	}
	
	fseek(file, 0, SEEK_END);
	tempSize = ftell(file);
	
	MemFile_Validate(memFile);
	if (memFile->data == NULL) {
		MemFile_Alloc(memFile, tempSize + 0x10);
		memFile->memSize = tempSize + 0x10;
		memFile->size = tempSize;
		if (memFile->data == NULL) {
			printf_warning("Failed to malloc MemFile.\n\tAttempted size is [0x%X] bytes to store data from [%s].", tempSize, filepath);
			
			return 1;
		}
	} else {
		if (memFile->memSize < tempSize)
			MemFile_Realloc(memFile, tempSize * 2);
		memFile->size = tempSize;
	}
	
	rewind(file);
	memFile->size = fread(memFile->data, 1, memFile->size, file);
	fclose(file);
	memFile->cast.u8[memFile->size] = '\0';
	
	memFile->info.age = Sys_Stat(filepath);
	strcpy(memFile->info.name, filepath);
	
	return 0;
}

s32 MemFile_SaveFile(MemFile* memFile, const char* filepath) {
	FILE* file = MemFOpen(filepath, "wb");
	
	if (file == NULL) {
		printf_warning("Failed to fopen file [%s] for writing.", filepath);
		
		return 1;
	}
	
	if (memFile->size)
		fwrite(memFile->data, sizeof(char), memFile->size, file);
	fclose(file);
	
	return 0;
}

s32 MemFile_SaveFile_String(MemFile* memFile, const char* filepath) {
	FILE* file = MemFOpen(filepath, "w");
	
	if (file == NULL) {
		printf_warning("Failed to fopen file [%s] for writing.", filepath);
		
		return 1;
	}
	
	if (memFile->size)
		fwrite(memFile->data, sizeof(char), memFile->size, file);
	fclose(file);
	
	return 0;
}

void MemFile_Free(MemFile* memFile) {
	if (memFile->param.initKey == 0xD0E0A0D0B0E0E0F0) {
		Free(memFile->data);
	}
	
	*memFile = MemFile_Initialize();
}

void MemFile_Reset(MemFile* memFile) {
	memFile->size = 0;
	memFile->seekPoint = 0;
}

void MemFile_Clear(MemFile* memFile) {
	memset(memFile->data, 0, memFile->memSize);
	MemFile_Reset(memFile);
}

// # # # # # # # # # # # # # # # # # # # #
// # VALUE                               #
// # # # # # # # # # # # # # # # # # # # #

u32 Value_Hex(const char* string) {
	return strtoul(string, NULL, 16);
}

s32 Value_Int(const char* string) {
	if (!memcmp(string, "0x", 2)) {
		return strtoul(string, NULL, 16);
	} else {
		return strtol(string, NULL, 10);
	}
}

f32 Value_Float(const char* string) {
	f32 fl;
	void* str = NULL;
	
	if (StrStr(string, ",")) {
		string = strdup(string);
		str = (void*)string;
		StrRep((void*)string, ",", ".");
	}
	
	fl = strtod(string, NULL);
	Free(str);
	
	return fl;
}

s32 Value_Bool(const char* string) {
	if (string == NULL)
		return -1;
	
	if (!stricmp(string, "true"))
		return true;
	else if (!stricmp(string, "false"))
		return false;
	
	return -1;
}

s32 Value_ValidateHex(const char* str) {
	s32 isOk = false;
	
	for (s32 i = 0; i < strlen(str); i++) {
		if (ispunct(str[i]))
			break;
		if (
			(str[i] >= 'A' && str[i] <= 'F') ||
			(str[i] >= 'a' && str[i] <= 'f') ||
			(str[i] >= '0' && str[i] <= '9') ||
			str[i] == 'x' || str[i] == 'X' ||
			str[i] == ' ' || str[i] == '\t'
		) {
			isOk = true;
			continue;
		}
		
		return false;
	}
	
	return isOk;
}

s32 Value_ValidateInt(const char* str) {
	s32 isOk = false;
	
	for (s32 i = 0; i < strlen(str); i++) {
		if (ispunct(str[i]))
			break;
		if (
			(str[i] >= '0' && str[i] <= '9')
		) {
			isOk = true;
			continue;
		}
		
		return false;
	}
	
	return isOk;
}

s32 Value_ValidateFloat(const char* str) {
	s32 isOk = false;
	
	for (s32 i = 0; i < strlen(str); i++) {
		if (ispunct(str[i]) && str[i] != '.')
			break;
		if (
			(str[i] >= '0' && str[i] <= '9') || str[i] == '.'
		) {
			isOk = true;
			continue;
		}
		
		return false;
	}
	
	return isOk;
}

ValueType Value_Type(const char* variable) {
	ValueType type = {
		.isFloat = true,
		.isHex   = true,
		.isDec   = true,
		.isBool  = false,
	};
	
	if (!strcmp(variable, "true") || !strcmp(variable, "false")) {
		type = (ValueType) {
			.isBool = true,
		};
		
		return type;
	}
	
	for (s32 i = 0; i < strlen(variable); i++) {
		if (variable[i] <= ' ') {
			type.isFloat = false;
			type.isDec = false;
			type.isHex = false;
		}
		
		if (isalpha(variable[i])) {
			type.isFloat = false;
			type.isDec = false;
		}
		
		if (variable[i] == '.')
			type.isDec = false;
		
		switch (variable[i]) {
			case 'a' ... 'f':
			case 'A' ... 'F':
			case '0' ... '9':
			case 'x':
				break;
			default:
				type.isHex = false;
				break;
		}
	}
	
	if (type.isDec) {
		type.isFloat = type.isHex = false;
	}
	
	return type;
}

s32 Digits_Int(s32 i) {
	s32 d = 0;
	
	for (; i != 0; d++)
		i *= 0.1;
	
	return ClampMin(d, 1);
}

s32 Digits_Hex(s32 i) {
	s32 d = 0;
	
	for (; i != 0 && d != 0; d++)
		i = (i >> 4);
	
	return ClampMin(d, 1);
}

// # # # # # # # # # # # # # # # # # # # #
// # MUSIC                               #
// # # # # # # # # # # # # # # # # # # # #

static const char* sNoteName[12] = {
	"C", "C#",
	"D", "D#",
	"E",
	"F", "F#",
	"G", "G#",
	"A", "A#",
	"B",
};

s32 Music_NoteIndex(const char* note) {
	s32 id = 0;
	u32 octave;
	
	foreach(i, sNoteName) {
		if (sNoteName[i][1] == '#')
			continue;
		if (note[0] == sNoteName[i][0]) {
			id = i;
			
			if (note[1] == '#')
				id++;
			
			break;
		}
	}
	
	while (!isdigit(note[0]) && note[0] != '-') note++;
	
	octave = 12 * (Value_Int(note));
	
	return id + octave;
}

const char* Music_NoteWord(s32 note) {
	f32 octave = (f32)note / 12;
	
	note %= 12;
	
	return xFmt("%s%d", sNoteName[note], (s32)floorf(octave));
}

// # # # # # # # # # # # # # # # # # # # #
// # STRING                              #
// # # # # # # # # # # # # # # # # # # # #

// Insert
void StrIns(char* point, const char* insert) {
	s32 insLen = strlen(insert);
	char* insEnd = point + insLen;
	s32 remLen = strlen(point);
	
	memmove(insEnd, point, remLen + 1);
	insEnd[remLen] = 0;
	memcpy(point, insert, insLen);
}

// Insert
void StrIns2(char* origin, const char* insert, s32 pos, s32 size) {
	s32 inslen = strlen(insert);
	
	if (pos >= size)
		return;
	
	if (size - pos - inslen > 0)
		memmove(&origin[pos + inslen], &origin[pos], size - pos - inslen);
	
	for (s32 j = 0; j < inslen; pos++, j++) {
		origin[pos] = insert[j];
	}
}

// Remove
void StrRem(char* point, s32 amount) {
	char* get = point + amount;
	s32 len = strlen(get);
	
	if (len)
		memcpy(point, get, strlen(get));
	point[len] = 0;
}

// Replace
s32 StrRep(char* src, const char* word, const char* replacement) {
	u32 repLen = strlen(replacement);
	u32 wordLen = strlen(word);
	s32 diff = 0;
	char* ptr;
	
	if ((uptr)word >= (uptr)src && (uptr)word < (uptr)src + strlen(src)) {
		printf("[%s] [%s]", word, replacement);
		printf_error("Replacing with self!");
	}
	
	ptr = StrStr(src, word);
	
	while (ptr != NULL) {
		u32 remLen = strlen(ptr + wordLen);
		memmove(ptr + repLen, ptr + wordLen, remLen + 1);
		memcpy(ptr, replacement, repLen);
		ptr = StrStr(ptr + repLen, word);
		diff = true;
	}
	
	return diff;
}

s32 StrRepWhole(char* src, const char* word, const char* replacement) {
	u32 repLen = strlen(replacement);
	u32 wordLen = strlen(word);
	s32 diff = 0;
	char* ptr;
	
	if ((uptr)word >= (uptr)src && (uptr)word < (uptr)src + strlen(src)) {
		printf("[%s] [%s]", word, replacement);
		printf_error("Replacing with self!");
	}
	
	ptr = StrStrWhole(src, word);
	
	while (ptr != NULL) {
		u32 remLen = strlen(ptr + wordLen);
		memmove(ptr + repLen, ptr + wordLen, remLen + 1);
		memcpy(ptr, replacement, repLen);
		ptr = StrStrWhole(ptr  + repLen, word);
		diff = true;
	}
	
	return diff;
}

// Unquote
char* StrUnq(const char* str) {
	char* new = xStrDup(str);
	
	StrRep(new, "\"", "");
	StrRep(new, "'", "");
	
	return new;
}

char* StrSlash(char* t) {
	StrRep(t, "\\", "/");
	
	return t;
}

char* StrStripIllegalChar(char* t) {
	StrRep(t, "\\", "");
	StrRep(t, "/", "");
	StrRep(t, ":", "");
	StrRep(t, "*", "");
	StrRep(t, "?", "");
	StrRep(t, "\"", "");
	StrRep(t, "<", "");
	StrRep(t, ">", "");
	StrRep(t, "|", "");
	
	return t;
}

// Common length
s32 StrComLen(const char* a, const char* b) {
	s32 s = 0;
	
	for (; s < strlen(b); s++) {
		if (b[s] != a[s])
			return s;
	}
	
	return s;
}

char* String_GetSpacedArg(char* argv[], s32 cur) {
	char tempBuf[512];
	s32 i = cur + 1;
	
	if (argv[i] && argv[i][0] != '-' && argv[i][1] != '-') {
		strcpy(tempBuf, argv[cur]);
		
		while (argv[i] && argv[i][0] != '-' && argv[i][1] != '-') {
			strcat(tempBuf, " ");
			strcat(tempBuf, argv[i++]);
		}
		
		return xStrDup(tempBuf);
	}
	
	return argv[cur];
}

void String_SwapExtension(char* dest, char* src, const char* ext) {
	strcpy(dest, Path(src));
	strcat(dest, Basename(src));
	strcat(dest, ext);
}

char* StrUpper(char* str) {
	forstr(i, str)
	str[i] = toupper(str[i]);
	
	return str;
}

char* StrLower(char* str) {
	forstr(i, str)
	str[i] = tolower(str[i]);
	
	return str;
}

// # # # # # # # # # # # # # # # # # # # #
// # utf8 <-> utf16                      #
// # # # # # # # # # # # # # # # # # # # #
// https://github.com/Davipb/utf8-utf16-converter

typedef enum {
	BMP_END                          = 0xFFFF,
	UNICODE_MAX                      = 0x10FFFF,
	INVALID_CODEPOINT                = 0xFFFD,
	GENERIC_SURROGATE_VALUE          = 0xD800,
	GENERIC_SURROGATE_MASK           = 0xF800,
	HIGH_SURROGATE_VALUE             = 0xD800,
	LOW_SURROGATE_VALUE              = 0xDC00,
	SURROGATE_MASK                   = 0xFC00,
	SURROGATE_CODEPOINT_OFFSET       = 0x10000,
	SURROGATE_CODEPOINT_MASK         = 0x03FF,
	SURROGATE_CODEPOINT_BITS         = 10,
	UTF8_1_MAX                       = 0x7F,
	UTF8_2_MAX                       = 0x7FF,
	UTF8_3_MAX                       = 0xFFFF,
	UTF8_4_MAX                       = 0x10FFFF,
	UTF8_CONTINUATION_VALUE          = 0x80,
	UTF8_CONTINUATION_MASK           = 0xC0,
	UTF8_CONTINUATION_CODEPOINT_BITS = 6,
	UTF8_LEADING_BYTES_LEN           = 4,
} __utf8_define_t;

typedef struct {
	u8 mask;
	u8 value;
} utf8_pattern;

static const utf8_pattern utf8_leading_bytes[] = {
	{ 0x80, 0x00 }, // 0xxxxxxx
	{ 0xE0, 0xC0 }, // 110xxxxx
	{ 0xF0, 0xE0 }, // 1110xxxx
	{ 0xF8, 0xF0 }, // 11110xxx
};

static s32 CalcLenU8(u32 codepoint) {
	if (codepoint <= UTF8_1_MAX)
		return 1;
	
	if (codepoint <= UTF8_2_MAX)
		return 2;
	
	if (codepoint <= UTF8_3_MAX)
		return 3;
	
	return 4;
}

#if 0
static s32 CalcLenU16(u32 codepoint) {
	if (codepoint <= BMP_END)
		return 1;
	
	return 2;
}
#endif

static Size EncodeU8(u32 codepoint, char* utf8, Size index) {
	s32 size = CalcLenU8(codepoint);
	
	// Write the continuation bytes in reverse order first
	for (s32 cont_index = size - 1; cont_index > 0; cont_index--) {
		u8 cont = codepoint & ~UTF8_CONTINUATION_MASK;
		cont |= UTF8_CONTINUATION_VALUE;
		
		utf8[index + cont_index] = cont;
		codepoint >>= UTF8_CONTINUATION_CODEPOINT_BITS;
	}
	
	utf8_pattern pattern = utf8_leading_bytes[size - 1];
	
	u8 lead = codepoint & ~(pattern.mask);
	
	lead |= pattern.value;
	
	utf8[index] = lead;
	
	return size;
}

static Size EncodeU16(u32 codepoint, wchar* utf16, Size index) {
	if (codepoint <= BMP_END) {
		utf16[index] = codepoint;
		
		return 1;
	}
	
	codepoint -= SURROGATE_CODEPOINT_OFFSET;
	
	u16 low = LOW_SURROGATE_VALUE;
	low |= codepoint & SURROGATE_CODEPOINT_MASK;
	
	codepoint >>= SURROGATE_CODEPOINT_BITS;
	
	u16 high = HIGH_SURROGATE_VALUE;
	high |= codepoint & SURROGATE_CODEPOINT_MASK;
	
	utf16[index] = high;
	utf16[index + 1] = low;
	
	return 2;
}

static u32 DecodeU8(const char* utf8, Size len, Size* index) {
	u8 leading = utf8[*index];
	s32 encoding_len = 0;
	utf8_pattern leading_pattern;
	bool matches = false;
	
	do {
		encoding_len++;
		leading_pattern = utf8_leading_bytes[encoding_len - 1];
		
		matches = (leading & leading_pattern.mask) == leading_pattern.value;
		
	} while (!matches && encoding_len < UTF8_LEADING_BYTES_LEN);
	
	if (!matches)
		return INVALID_CODEPOINT;
	
	u32 codepoint = leading & ~leading_pattern.mask;
	
	for (s32 i = 0; i < encoding_len - 1; i++) {
		if (*index + 1 >= len)
			return INVALID_CODEPOINT;
		
		u8 continuation = utf8[*index + 1];
		if ((continuation & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_VALUE)
			return INVALID_CODEPOINT;
		
		codepoint <<= UTF8_CONTINUATION_CODEPOINT_BITS;
		codepoint |= continuation & ~UTF8_CONTINUATION_MASK;
		
		(*index)++;
	}
	
	s32 proper_len = CalcLenU8(codepoint);
	
	if (proper_len != encoding_len)
		return INVALID_CODEPOINT;
	if (codepoint < BMP_END && (codepoint & GENERIC_SURROGATE_MASK) == GENERIC_SURROGATE_VALUE)
		return INVALID_CODEPOINT;
	if (codepoint > UNICODE_MAX)
		return INVALID_CODEPOINT;
	
	return codepoint;
}

static u32 DecodeU16(const wchar* utf16, Size len, Size* index) {
	u16 high = utf16[*index];
	
	if ((high & GENERIC_SURROGATE_MASK) != GENERIC_SURROGATE_VALUE)
		return high;
	if ((high & SURROGATE_MASK) != HIGH_SURROGATE_VALUE)
		return INVALID_CODEPOINT;
	if (*index == len - 1)
		return INVALID_CODEPOINT;
	
	u16 low = utf16[*index + 1];
	
	if ((low & SURROGATE_MASK) != LOW_SURROGATE_VALUE)
		return INVALID_CODEPOINT;
	(*index)++;
	u32 result = high & SURROGATE_CODEPOINT_MASK;
	
	result <<= SURROGATE_CODEPOINT_BITS;
	result |= low & SURROGATE_CODEPOINT_MASK;
	result += SURROGATE_CODEPOINT_OFFSET;
	
	return result;
}

char* StrU8(char* dst, const wchar* src) {
	Size dstIndex = 0;
	Size len = strwlen(src) + 1;
	
	if (!dst)
		dst = xAlloc(len);
	
	for (Size indexSrc = 0; indexSrc < len; indexSrc++)
		dstIndex += EncodeU8(DecodeU16(src, len, &indexSrc), dst, dstIndex);
	
	return dst;
}

wchar* StrU16(wchar* dst, const char* src) {
	Size dstIndex = 0;
	Size len = strlen(src) + 1;
	
	if (!dst)
		dst = xAlloc(len * 3);
	
	for (Size srcIndex = 0; srcIndex < len; srcIndex++)
		dstIndex += EncodeU16(DecodeU8(src, len, &srcIndex), dst, dstIndex);
	
	return dst;
}

Size strwlen(const wchar* s) {
	Size len = 0;
	
	while (s[len] != L'\0')
		len++;
	
	return len;
}

// # # # # # # # # # # # # # # # # # # # #
// # CONFIG                              #
// # # # # # # # # # # # # # # # # # # # #

static ThreadLocal char* sCfgSection;

static const char* __Config_GotoSection(const char* str) {
	if (sCfgSection == NULL)
		return str;
	
	s32 lineNum = LineNum(str);
	const char* line = str;
	
	Log("GoTo \"%s\"", sCfgSection);
	
	for (s32 i = 0; i < lineNum; i++, line = Line(line, 1)) {
		if (*line == '\0')
			return NULL;
		while (*line == ' ' || *line == '\t') line++;
		if (!strncmp(line, sCfgSection, strlen(sCfgSection) - 1))
			return Line(line, 1);
	}
	
	Log("Return NULL;", sCfgSection);
	
	return NULL;
}

char* Config_Variable(const char* str, const char* name) {
	u32 lineCount;
	char* line;
	char* ret = NULL;
	
	str = __Config_GotoSection(str);
	if (str == NULL) return NULL;
	
	lineCount = LineNum(str);
	line = (char*)str;
	
	Log("Var [%s]", name);
	for (s32 i = 0; i < lineCount; i++, line = Line(line, 1)) {
		if (line == NULL)
			break;
		while (line[0] == ' ' || line[0] == '\t') line++;
		if (line[0] == '#' || line[0] == ';' || line[0] <= ' ')
			continue;
		if (line[0] == '[')
			break;
		if (StrMtch(line, name)) {
			char* p = line + strlen(name);
			
			while (p[0] == ' ' || p[0] == '\t') {
				p++;
				if (p[0] == '\0')
					return NULL;
			}
			
			if (p[0] != '=')
				return NULL;
			
			while (p[0] == '=' || p[0] == ' ' || p[0] == '\t') {
				p++;
				if (p[0] == '\0')
					return NULL;
			}
			ret = p;
			break;
		}
	}
	
	return ret;
}

char* Config_GetVariable(const char* str, const char* name) {
	u32 lineCount;
	char* line;
	char* ret = NULL;
	
	str = __Config_GotoSection(str);
	if (str == NULL) return NULL;
	
	lineCount = LineNum(str);
	line = (char*)str;
	
	Log("Var [%s]", name);
	for (s32 i = 0; i < lineCount; i++, line = Line(line, 1)) {
		if (line == NULL)
			break;
		while (line[0] == ' ' || line[0] == '\t') line++;
		if (line[0] == '#' || line[0] == ';' || line[0] <= ' ')
			continue;
		if (line[0] == '[')
			break;
		if (StrMtch(line, name)) {
			s32 isString = 0;
			char* buf;
			char* p = line + strlen(name);
			u32 size = 0;
			
			while (p[0] == ' ' || p[0] == '\t') {
				p++;
				
				if (p[0] == '\0' || p[0] == '\n')
					return NULL;
			}
			
			if (p[0] != '=')
				return NULL;
			
			while (p[0] == '=' || p[0] == ' ' || p[0] == '\t') {
				p++;
				
				if (p[0] == '\0' || p[0] == '\n')
					return NULL;
			}
			
			while (p[size] != ';' && (p[size] != '#' || isString == true) && p[size] != '\n' && (isString == false || p[size] != '\"') && p[size] != '\0') {
				if (p[size] == '\"')
					isString = 1;
				size++;
			}
			
			if (isString == false)
				while (p[size - 1] <= ' ')
					size--;
			
			buf = xAlloc(size + 1);
			memcpy(buf, p + isString, size - isString);
			
			ret = buf;
			break;
		}
	}
	
	return ret;
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
	u32 lineNum = LineNum(mem->str);
	char* line = mem->str;
	
	for (s32 i = 0; i < lineNum; i++, line = Line(line, 1)) {
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
	}
	
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

// # # # # # # # # # # # # # # # # # # # #
// # TSV                                 #
// # # # # # # # # # # # # # # # # # # # #

char* String_Tsv(char* str, s32 rowNum, s32 lineNum) {
	char* line = Line(str, lineNum);
	u32 size = 0;
	char* r;
	
	for (s32 i = 0; i < rowNum; i++) {
		while (*line != '\t') {
			line++;
			
			if (*line == '\0' || *line == '\n')
				return NULL;
		}
		
		line++;
		
		if (*line == '\0' || *line == '\n')
			return NULL;
	}
	
	if (*line == '\t') return NULL;
	while (line[size] != '\t' && line[size] != '\0' && line[size] != '\n') size++;
	
	r = xAlloc(size + 1);
	memcpy(r, line, size);
	
	return r;
}

// # # # # # # # # # # # # # # # # # # # #
// # LOG                                 #
// # # # # # # # # # # # # # # # # # # # #

#include <signal.h>

#define FAULT_BUFFER_SIZE (1024)
#define FAULT_LOG_NUM     8

static char* sLogMsg[FAULT_LOG_NUM];
static char* sLogFunc[FAULT_LOG_NUM];
static u32 sLogLine[FAULT_LOG_NUM];
static s32 sLogInit;
static s32 sLogOutput = true;

void Log_NoOutput(void) {
	sLogOutput = false;
}

static void Log_Signal_PrintTitle(s32 arg, FILE* file) {
	const char* errorMsg[] = {
		"\a0",
		"\a1 - Hang Up",
		"\a2 - Interrupted", // SIGINT
		"\a3 - Quit",
		"\a4 - Illegal Instruction",
		"\a5 - Trap",
		"\a6 - Abort()",
		"\a7 - Illegal Memory Access",
		"\a8 - Floating Point Exception",
		"\a9 - Killed",
		"\a10 - Programmer Error",
		"\a11 - Segmentation Fault",
		"\a12 - Programmer Error",
		"\a13 - Pipe Death",
		"\a14 - Alarm",
		"\a15 - Killed",
		
		"\aERROR",
	};
	
	printf_WinFix();
	
	if (arg == 16 && sLogOutput == true)
		fprintf(file, "\n[!]: [ ERROR ]");
	else if (arg != 0xDEADBEEF)
		fprintf(file, "\n" PRNT_DGRY "[" PRNT_REDD "!" PRNT_DGRY "]:" PRNT_DGRY " [ " PRNT_REDD "%s " PRNT_DGRY "]", errorMsg[ClampMax(arg, 16)]);
	else
		fprintf(file, "\n" PRNT_DGRY "[" PRNT_REDD "!" PRNT_DGRY "]:" PRNT_DGRY " [ " PRNT_REDD "LOG " PRNT_DGRY "]");
}

static void Log_Printinf(s32 arg, FILE* file) {
	u32 msgsNum = 0;
	u32 repeat = 0;
	
	if (arg != 16 || sLogOutput == false) {
		for (s32 i = FAULT_LOG_NUM - 1; i >= 0; i--) {
			if (strlen(sLogMsg[i]) == 0)
				continue;
			
			if ((msgsNum > 0 && strcmp(sLogMsg[i], sLogMsg[i + 1])) )
				if (repeat)
					fprintf(file, PRNT_PRPL " x %d" PRNT_RSET, repeat + 1);
			
			if (msgsNum == 0 || (msgsNum > 0 && strcmp(sLogFunc[i], sLogFunc[i + 1])) )
				fprintf(file, "\n" PRNT_DGRY "[" PRNT_REDD "!" PRNT_DGRY "]:" PRNT_YELW " %s" PRNT_DGRY "();" PRNT_RSET, sLogFunc[i]);
			
			if (msgsNum == 0 || (msgsNum > 0 && strcmp(sLogMsg[i], sLogMsg[i + 1])) ) {
				fprintf(
					file,
					"\n" PRNT_DGRY "[" PRNT_REDD "!" PRNT_DGRY "]:" PRNT_DGRY " [ %4d ]" PRNT_RSET " %s" PRNT_RSET,
					sLogLine[i],
					sLogMsg[i]
				);
				
				repeat = 0;
			} else {
				repeat++;
			}
			
			msgsNum++;
		}
		fprintf(file, "\n");
		
		fprintf(
			file,
			"\n" PRNT_DGRY "[" PRNT_REDD "!" PRNT_DGRY "]:" PRNT_YELW " Provide this log to the developer." PRNT_RSET "\n"
		);
		fprintf(file, "\n");
	} else {
		for (s32 i = FAULT_LOG_NUM - 1; i >= 0; i--) {
			if (strlen(sLogMsg[i]) == 0)
				continue;
			
			if ((msgsNum > 0 && strcmp(sLogMsg[i], sLogMsg[i + 1])) )
				if (repeat)
					fprintf(file, " x %d", repeat + 1);
			
			if (msgsNum == 0 || (msgsNum > 0 && strcmp(sLogFunc[i], sLogFunc[i + 1])) )
				fprintf(file, "\n[!]: %s();", sLogFunc[i]);
			
			if (msgsNum == 0 || (msgsNum > 0 && strcmp(sLogMsg[i], sLogMsg[i + 1])) ) {
				fprintf(
					file,
					"\n[!]: [ %4d ] %s",
					sLogLine[i],
					sLogMsg[i]
				);
				
				repeat = 0;
			} else {
				repeat++;
			}
			
			msgsNum++;
		}
		fprintf(file, "\n");
	}
}

static void Log_Signal(s32 arg) {
	static volatile bool ran = 0;
	FILE* file;
	
	if (!sLogInit)
		return;
	
	if (ran) return;
	sLogInit = false;
	sEXIT = true;
	ran = ___sExt_ThreadInit != 0;
	
	if (arg == 16 && sLogOutput)
		file = fopen(".log", "w");
	else
		file = stdout;
	
	Log_Signal_PrintTitle(arg, file);
	Log_Printinf(arg, file);
	
	if (arg != 16) {
		printf_getchar("Press enter to exit");
		exit(1);
	}
}

void Log_Init() {
	if (sLogInit)
		return;
	for (s32 i = 1; i < 16; i++)
		signal(i, Log_Signal);
	
	for (s32 i = 0; i < FAULT_LOG_NUM; i++) {
		sLogMsg[i] = Calloc(FAULT_BUFFER_SIZE);
		sLogFunc[i] = Calloc(FAULT_BUFFER_SIZE * 0.25);
	}
	
	sLogInit = true;
}

void Log_Free() {
	if (!sLogInit)
		return;
	sLogInit = 0;
	for (s32 i = 0; i < FAULT_LOG_NUM; i++) {
		Free(sLogMsg[i]);
		Free(sLogFunc[i]);
	}
}

void Log_Print() {
	if (!sLogInit)
		return;
	if (sLogMsg[0] == NULL)
		return;
	if (sLogMsg[0][0] != 0)
		Log_Signal(0xDEADBEEF);
}

void __Log_ItemList(ItemList* list, const char* function, s32 line) {
	if (!sLogInit)
		return;
	forlist(i, *list) {
		__Log(function, line, "%d - [%s]", i, list->item[i]);
	}
}

void __Log(const char* func, u32 line, const char* txt, ...) {
	if (!sLogInit)
		return;
	
	ThreadLock_Lock();
	va_list args;
	
	if (sLogMsg[0] == NULL)
		return;
	
	for (s32 i = FAULT_LOG_NUM - 1; i > 0; i--) {
		strcpy(sLogMsg[i], sLogMsg[i - 1]);
		strcpy(sLogFunc[i], sLogFunc[i - 1]);
		sLogLine[i] = sLogLine[i - 1];
	}
	
	va_start(args, txt);
	vsnprintf(sLogMsg[0], FAULT_BUFFER_SIZE, txt, args);
	va_end(args);
	
	strcpy(sLogFunc[0], func);
	sLogLine[0] = line;
	ThreadLock_Unlock();
}

// # # # # # # # # # # # # # # # # # # # #
// # PRINTF                              #
// # # # # # # # # # # # # # # # # # # # #

static char* sPrintfPrefix = "";
static u8 sPrintfType = 1;
static const char* sPrintfPreType[][4] = {
	{
		NULL,
		NULL,
		NULL,
		NULL,
	},
	{
		">",
		">",
		">",
		">"
	}
};
PrintfSuppressLevel gPrintfSuppress = 0;
u8 gPrintfProgressing;

void printf_SetSuppressLevel(PrintfSuppressLevel lvl) {
	gPrintfSuppress = lvl;
}

void printf_SetPrefix(char* fmt) {
	sPrintfPrefix = fmt;
}

void printf_SetPrintfTypes(const char* d, const char* w, const char* e, const char* i) {
	sPrintfType = 0;
	sPrintfPreType[sPrintfType][0] = d;
	sPrintfPreType[sPrintfType][1] = w;
	sPrintfPreType[sPrintfType][2] = e;
	sPrintfPreType[sPrintfType][3] = i;
}

void printf_toolinfo(const char* toolname, const char* fmt, ...) {
	static u32 printed = 0;
	
	if (gPrintfSuppress >= PSL_NO_INFO)
		return;
	
	if (printed != 0) return;
	printed++;
	
	u32 strln = strlen(toolname);
	u32 rmv = 0;
	u32 tmp = strln;
	va_list args;
	
	for (s32 i = 0; i < strln; i++) {
		if (rmv) {
			if (toolname[i] != 'm') {
				tmp--;
			} else {
				tmp -= 2;
				rmv = false;
			}
		} else {
			if (toolname[i] == '\e' && toolname[i + 1] == '[') {
				rmv = true;
				strln--;
			}
		}
	}
	
	strln = tmp;
	
	printf(PRNT_GRAY "[>]--");
	for (s32 i = 0; i < strln; i++)
		printf("-");
	printf("------[>]\n");
	
	printf(" |   ");
	printf(PRNT_CYAN "%s" PRNT_GRAY, toolname);
	printf("       |\n");
	
	printf("[>]--");
	for (s32 i = 0; i < strln; i++)
		printf("-");
	printf("------[>]\n" PRNT_RSET);
	printf("     ");
	
	if (fmt) {
		va_start(args, fmt);
		vprintf(
			fmt,
			args
		);
		va_end(args);
		if (strlen(fmt) > 1)
			printf("\n");
	}
	printf("\n" PRNT_RSET);
	
}

static void __printf_call(u32 type, FILE* file) {
	char* color[4] = {
		PRNT_PRPL,
		PRNT_REDD,
		PRNT_REDD,
		PRNT_BLUE
	};
	
	fprintf(
		file,
		"%s"
		PRNT_GRAY "["
		"%s%s"
		PRNT_GRAY "]: "
		PRNT_RSET,
		sPrintfPrefix,
		color[type],
		sPrintfPreType[sPrintfType][type]
	);
}

void printf_warning(const char* fmt, ...) {
	if (gPrintfSuppress >= PSL_NO_WARNING)
		return;
	
	if (sEXIT)
		return;
	
	if (gPrintfProgressing) {
		printf("\n");
		gPrintfProgressing = false;
	}
	
	va_list args;
	
	va_start(args, fmt);
	__printf_call(1, stdout);
	vprintf(
		fmt,
		args
	);
	printf("\n");
	va_end(args);
}

void printf_warning_align(const char* info, const char* fmt, ...) {
	if (gPrintfSuppress >= PSL_NO_WARNING)
		return;
	
	if (sEXIT)
		return;
	
	if (gPrintfProgressing) {
		printf("\n");
		gPrintfProgressing = false;
	}
	
	va_list args;
	
	va_start(args, fmt);
	__printf_call(1, stdout);
	printf(
		"%-16s " PRNT_RSET,
		info
	);
	vprintf(
		fmt,
		args
	);
	printf(PRNT_RSET "\n");
	va_end(args);
}

static void printf_MuteOutput(FILE* output) {
#ifdef _WIN32
	freopen ("NUL", "w", output);
#else
	freopen ("/dev/null", "w", output);
#endif
}

void printf_error(const char* fmt, ...) {
	char* msg;
	
	sEXIT = 1;
	if (___sExt_ThreadInit)
		printf_MuteOutput(stdout);
	
	Log_Signal(16);
	Log_Free();
	if (gPrintfSuppress < PSL_NO_ERROR) {
		if (gPrintfProgressing) {
			fprintf(stderr, "\n");
			gPrintfProgressing = false;
		}
		
		va_list args;
		
		va_start(args, fmt);
		__printf_call(2, stderr);
		if (___sExt_ThreadInit)
			fprintf(stderr, "[>]: ");
		vasprintf(
			&msg,
			fmt,
			args
		);
		
		fprintf(stderr, "%s\n", msg);
		
		va_end(args);
	}
	
#ifdef _WIN32
	if (___sExt_ThreadInit)
		fprintf(stderr, "[<]: Press enter to exit");
	Terminal_GetChar();
#endif
	
	exit(EXIT_FAILURE);
}

void printf_error_align(const char* info, const char* fmt, ...) {
	char* msg[2];
	
	sEXIT = 1;
	if (___sExt_ThreadInit)
		printf_MuteOutput(stdout);
	
	Log_Signal(16);
	Log_Free();
	if (gPrintfSuppress < PSL_NO_ERROR) {
		if (gPrintfProgressing) {
			fprintf(stderr, "\n");
			gPrintfProgressing = false;
		}
		
		va_list args;
		
		va_start(args, fmt);
		__printf_call(2, stderr);
		if (___sExt_ThreadInit)
			fprintf(stderr, "[>]: ");
		asprintf(
			&msg[0],
			"%-16s " PRNT_RSET,
			info
		);
		vasprintf(
			&msg[1],
			fmt,
			args
		);
		
		fprintf(stderr, "%s%s\n", msg[0], msg[1]);
		
		va_end(args);
	}
	
#ifdef _WIN32
	if (___sExt_ThreadInit)
		fprintf(stderr, "[<]: Press enter to exit");
	Terminal_GetChar();
#endif
	
	exit(EXIT_FAILURE);
}

void printf_info(const char* fmt, ...) {
	char* buf = NULL;
	
	if (sEXIT)
		return;
	
	if (gPrintfSuppress >= PSL_NO_INFO)
		return;
	
	if (gPrintfProgressing) {
		printf("\n");
		gPrintfProgressing = false;
	}
	va_list args;
	
	va_start(args, fmt);
	__printf_call(3, stdout);
	vasprintf(
		&buf,
		fmt,
		args
	);
	va_end(args);
	
	printf("%s" PRNT_RSET "\n", buf);
	Free(buf);
}

void printf_info_align(const char* info, const char* fmt, ...) {
	char* buf = NULL;
	
	if (sEXIT)
		return;
	
	if (gPrintfSuppress >= PSL_NO_INFO)
		return;
	
	if (gPrintfProgressing) {
		printf("\n");
		gPrintfProgressing = false;
	}
	va_list args;
	
	va_start(args, fmt);
	__printf_call(3, stdout);
	vasprintf(
		&buf,
		fmt,
		args
	);
	va_end(args);
	
	printf("%-16s%s" PRNT_RSET "\n", info, buf);
	Free(buf);
}

void printf_prog_align(const char* info, const char* fmt, const char* color) {
	if (sEXIT)
		return;
	
	if (gPrintfSuppress >= PSL_NO_INFO)
		return;
	
	if (gPrintfProgressing) {
		printf("\n");
		gPrintfProgressing = false;
	}
	
	printf("\n");
	Terminal_ClearLines(2);
	__printf_call(3, stdout);
	printf("%-16s%s%s", info, color ? color : "", fmt);
}

void printf_progressFst(const char* info, u32 a, u32 b) {
	if (gPrintfSuppress >= PSL_NO_INFO) {
		return;
	}
	
	if (sEXIT)
		return;
	
	printf("\r");
	__printf_call(3, stdout);
	printf(
		// "%-16s" PRNT_RSET "[%4d / %-4d]",
		xFmt("%c-16s" PRNT_RSET "[ %c%dd / %c-%dd ]", '%', '%', Digits_Int(b), '%', Digits_Int(b)),
		info,
		a,
		b
	);
	gPrintfProgressing = true;
	
	if (a == b) {
		gPrintfProgressing = false;
		printf("\n");
	}
	fflush(stdout);
}

void printf_progress(const char* info, u32 a, u32 b) {
	if (gPrintfSuppress >= PSL_NO_INFO) {
		return;
	}
	
	if (sEXIT)
		return;
	
	static f32 lstPrcnt;
	f32 prcnt = (f32)a / (f32)b;
	
	if (lstPrcnt > prcnt)
		lstPrcnt = 0;
	
	if (prcnt - lstPrcnt > 0.125) {
		lstPrcnt = prcnt;
	} else {
		if (a != b && a > 1) {
			return;
		}
	}
	
	printf("\r");
	__printf_call(3, stdout);
	printf(
		// "%-16s" PRNT_RSET "[%4d / %-4d]",
		xFmt("%c-16s" PRNT_RSET "[ %c%dd / %c-%dd ]", '%', '%', Digits_Int(b), '%', Digits_Int(b)),
		info,
		a,
		b
	);
	gPrintfProgressing = true;
	
	if (a == b) {
		gPrintfProgressing = false;
		printf("\n");
	}
	fflush(stdout);
}

void printf_getchar(const char* txt) {
	printf_info("%s", txt);
	Terminal_GetChar();
}

void printf_lock(const char* fmt, ...) {
	va_list va;
	
	if (sEXIT)
		return;
	
	va_start(va, fmt);
	ThreadLock_Lock();
	vprintf(fmt, va);
	ThreadLock_Unlock();
	va_end(va);
}

void printf_WinFix(void) {
#ifdef _WIN32
	system("\0");
#endif
}

void printf_hex(const char* txt, const void* data, u32 size, u32 dispOffset) {
	const u8* d = data;
	u32 num = 8;
	char* digit;
	s32 i = 0;
	
	if (sEXIT)
		return;
	
	if (gPrintfSuppress >= PSL_NO_INFO)
		return;
	
	for (;; num--)
		if ((size + dispOffset) >> (num * 4))
			break;
	
	digit = xFmt("" PRNT_GRAY "%c0%dX: " PRNT_RSET, '%', num + 1);
	
	if (txt)
		printf_info("%s", txt);
	for (; i < size; i++) {
		if (i % 16 == 0)
			printf(digit, i + dispOffset);
		
		printf("%02X", d[i]);
		if ((i + 1) % 4 == 0)
			printf(" ");
		if ((i + 1) % 16 == 0)
			printf("\n");
	}
	
	if (i % 16 != 0)
		printf("\n");
}

void printf_bit(const char* txt, const void* data, u32 size, u32 dispOffset) {
	const u8* d = data;
	s32 s = 0;
	u32 num = 8;
	char* digit;
	
	if (sEXIT)
		return;
	
	if (gPrintfSuppress >= PSL_NO_INFO)
		return;
	
	for (;; num--)
		if ((size + dispOffset) >> (num * 4))
			break;
	
	digit = xFmt("" PRNT_GRAY "%c0%dX: " PRNT_RSET, '%', num + 1);
	
	if (txt)
		printf_info("%s", txt);
	for (s32 i = 0; i < size; i++) {
		if (s % 4 == 0)
			printf(digit, s + dispOffset);
		
		for (s32 j = 7; j >= 0; j--)
			printf("%d", (d[i] >> j) & 1);
		
		printf(" ");
		
		if ((s + 1) % 4 == 0)
			printf("\n");
		
		s++;
	}
	
	if (s % 4 != 0)
		printf("\n");
}

void printf_nl(void) {
	if (sEXIT)
		return;
	
	if (gPrintfSuppress >= PSL_NO_INFO)
		return;
	
	printf("\n");
}

// # # # # # # # # # # # # # # # # # # # #
// # MATH                                #
// # # # # # # # # # # # # # # # # # # # #

f32 Math_SmoothStepToF(f32* pValue, f32 target, f32 fraction, f32 step, f32 minStep) {
	if (*pValue != target) {
		f32 stepSize = (target - *pValue) * fraction;
		
		if ((stepSize >= minStep) || (stepSize <= -minStep)) {
			if (stepSize > step) {
				stepSize = step;
			}
			
			if (stepSize < -step) {
				stepSize = -step;
			}
			
			*pValue += stepSize;
		} else {
			if (stepSize < minStep) {
				*pValue += minStep;
				stepSize = minStep;
				
				if (target < *pValue) {
					*pValue = target;
				}
			}
			if (stepSize > -minStep) {
				*pValue += -minStep;
				
				if (*pValue < target) {
					*pValue = target;
				}
			}
		}
	}
	
	return fabsf(target - *pValue);
}

f32 Math_Spline_Audio(f32 k, f32 xm1, f32 x0, f32 x1, f32 x2) {
	f32 a = (3.0f * (x0 - x1) - xm1 + x2) * 0.5f;
	f32 b = 2.0f * x1 + xm1 - (5.0f * x0 + x2) * 0.5f;
	f32 c = (x1 - xm1) * 0.5f;
	
	return (((((a * k) + b) * k) + c) * k) + x0;
}

f32 Math_Spline(f32 k, f32 xm1, f32 x0, f32 x1, f32 x2) {
	f32 coeff[4];
	
	coeff[0] = (1.0f - k) * (1.0f - k) * (1.0f - k) / 6.0f;
	coeff[1] = k * k * k / 2.0f - k * k + 2.0f / 3.0f;
	coeff[2] = -k * k * k / 2.0f + k * k / 2.0f + k / 2.0f + 1.0f / 6.0f;
	coeff[3] = k * k * k / 6.0f;
	
	return (coeff[0] * xm1) + (coeff[1] * x0) + (coeff[2] * x1) + (coeff[3] * x2);
}

void Math_ApproachF(f32* pValue, f32 target, f32 fraction, f32 step) {
	if (*pValue != target) {
		f32 stepSize = (target - *pValue) * fraction;
		
		if (stepSize > step) {
			stepSize = step;
		} else if (stepSize < -step) {
			stepSize = -step;
		}
		
		*pValue += stepSize;
	}
}

void Math_ApproachS(s16* pValue, s16 target, s16 scale, s16 step) {
	s16 diff = target - *pValue;
	
	diff /= scale;
	
	if (diff > step) {
		*pValue += step;
	} else if (diff < -step) {
		*pValue -= step;
	} else {
		*pValue += diff;
	}
}

// # # # # # # # # # # # # # # # # # # # #
// # SHA                                 #
// # # # # # # # # # # # # # # # # # # # #

/*
   Calculate the sha256 digest of some data
   Author: Vitor Henrique Andrade Helfensteller Straggiotti Silva
   Date: 26/06/2021 (DD/MM/YYYY)
 */

static u32 Sha_Sgima1(u32 x) {
	u32 RotateRight17, RotateRight19, ShiftRight10;
	
	RotateRight17 = (x >> 17) | (x << 15);
	RotateRight19 = (x >> 19) | (x << 13);
	ShiftRight10 = x >> 10;
	
	return RotateRight17 ^ RotateRight19 ^ ShiftRight10;
}

static u32 Sha_Sgima0(u32 x) {
	u32 RotateRight7, RotateRight18, ShiftRight3;
	
	RotateRight7 = (x >> 7) | (x << 25);
	RotateRight18 = (x >> 18) | (x << 14);
	ShiftRight3 = x >> 3;
	
	return RotateRight7 ^ RotateRight18 ^ ShiftRight3;
}

static u32 Sha_Choice(u32 x, u32 y, u32 z) {
	return (x & y) ^ ((~x) & z);
}

static u32 Sha_BigSigma1(u32 x) {
	u32 RotateRight6, RotateRight11, RotateRight25;
	
	RotateRight6 = (x >> 6) | (x << 26);
	RotateRight11 = (x >> 11) | (x << 21);
	RotateRight25 = (x >> 25) | (x << 7);
	
	return RotateRight6 ^ RotateRight11 ^ RotateRight25;
}

static u32 Sha_BigSigma0(u32 x) {
	u32 RotateRight2, RotateRight13, RotateRight22;
	
	RotateRight2 = (x >> 2) | (x << 30);
	RotateRight13 = (x >> 13) | (x << 19);
	RotateRight22 = (x >> 22) | (x << 10);
	
	return RotateRight2 ^ RotateRight13 ^ RotateRight22;
}

static u32 Sha_Major(u32 x, u32 y, u32 z) {
	return (x & y) ^ (x & z) ^ (y & z);
}

static u8 Sha_CreateCompleteScheduleArray(u8* Data, u64 DataSizeByte, u64* RemainingDataSizeByte, u32* W) {
	u8 TmpBlock[64];
	u8 IsFinishedFlag = 0;
	static u8 SetEndOnNextBlockFlag = 0;
	
	if ((0xFFFFFFFFFFFFFFFF / 8) < DataSizeByte) {
		printf("Error! File/Data exceeds size limit of 20097152 TiB");
		exit(EXIT_FAILURE);
	}
	
	for (u8 i = 0; i < 64; i++) {
		W[i] = 0x0;
		TmpBlock[i] = 0x0;
	}
	
	for (u8 i = 0; i < 64; i++) {
		if (*RemainingDataSizeByte > 0) {
			TmpBlock[i] = Data[DataSizeByte - *RemainingDataSizeByte];
			*RemainingDataSizeByte = *RemainingDataSizeByte - 1;
			
			if (*RemainingDataSizeByte == 0) {
				if (i < 63) {
					i++;
					TmpBlock[i] = 0x80;
					if (i < 56) {
						u64 DataSizeBits = DataSizeByte * 8;
						TmpBlock[56] = (DataSizeBits >> 56) & 0x00000000000000FF;
						TmpBlock[57] = (DataSizeBits >> 48) & 0x00000000000000FF;
						TmpBlock[58] = (DataSizeBits >> 40) & 0x00000000000000FF;
						TmpBlock[59] = (DataSizeBits >> 32) & 0x00000000000000FF;
						TmpBlock[60] = (DataSizeBits >> 24) & 0x00000000000000FF;
						TmpBlock[61] = (DataSizeBits >> 16) & 0x00000000000000FF;
						TmpBlock[62] = (DataSizeBits >> 8) & 0x00000000000000FF;
						TmpBlock[63] = DataSizeBits & 0x00000000000000FF;
						IsFinishedFlag = 1;
						goto outside1;
						
					} else
						goto outside1;
					
				} else {
					SetEndOnNextBlockFlag = 1;
				}
			}
		} else {
			if ((SetEndOnNextBlockFlag == 1) && (i == 0)) {
				TmpBlock[i] = 0x80;
				SetEndOnNextBlockFlag = 0;
			}
			u64 DataSizeBits = DataSizeByte * 8;
			TmpBlock[56] = (DataSizeBits >> 56) & 0x00000000000000FF;
			TmpBlock[57] = (DataSizeBits >> 48) & 0x00000000000000FF;
			TmpBlock[58] = (DataSizeBits >> 40) & 0x00000000000000FF;
			TmpBlock[59] = (DataSizeBits >> 32) & 0x00000000000000FF;
			TmpBlock[60] = (DataSizeBits >> 24) & 0x00000000000000FF;
			TmpBlock[61] = (DataSizeBits >> 16) & 0x00000000000000FF;
			TmpBlock[62] = (DataSizeBits >> 8) & 0x00000000000000FF;
			TmpBlock[63] = DataSizeBits & 0x00000000000000FF;
			IsFinishedFlag = 1;
			goto outside1;
		}
	}
outside1:
	
	for (u8 i = 0; i < 64; i += 4) {
		W[i / 4] = (((u32)TmpBlock[i]) << 24) |
			(((u32)TmpBlock[i + 1]) << 16) |
			(((u32)TmpBlock[i + 2]) << 8) |
			((u32)TmpBlock[i + 3]);
	}
	
	if (IsFinishedFlag == 1)
		return 0;
	else
		return 1;
}

static void Sha_CompleteScheduleArray(u32* W) {
	for (u8 i = 16; i < 64; i++)
		W[i] = Sha_Sgima1(W[i - 2]) + W[i - 7] + Sha_Sgima0(W[i - 15]) + W[i - 16];
}

static void Sha_Compression(u32* Hash, u32* W) {
	enum TmpH {a, b, c, d, e, f, g, h};
	u32 TmpHash[8] = { 0 };
	u32 Temp1 = 0, Temp2 = 0;
	const u32 K_const[64] = {
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
		0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
		0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
		0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
		0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
		0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
		0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
		0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
		0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
		0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
		0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
		0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
		0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
		0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
		0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
	};
	
	TmpHash[a] = Hash[0];
	TmpHash[b] = Hash[1];
	TmpHash[c] = Hash[2];
	TmpHash[d] = Hash[3];
	TmpHash[e] = Hash[4];
	TmpHash[f] = Hash[5];
	TmpHash[g] = Hash[6];
	TmpHash[h] = Hash[7];
	
	for (u32 i = 0; i < 64; i++) {
		Temp1 = Sha_BigSigma1(TmpHash[e]) + Sha_Choice(TmpHash[e], TmpHash[f], TmpHash[g]) +
			K_const[i] + W[i] + TmpHash[h];
		Temp2 = Sha_BigSigma0(TmpHash[a]) + Sha_Major(TmpHash[a], TmpHash[b], TmpHash[c]);
		
		TmpHash[h] = TmpHash[g];
		TmpHash[g] = TmpHash[f];
		TmpHash[f] = TmpHash[e];
		TmpHash[e] = TmpHash[d] + Temp1;
		TmpHash[d] = TmpHash[c];
		TmpHash[c] = TmpHash[b];
		TmpHash[b] = TmpHash[a];
		TmpHash[a] = Temp1 + Temp2;
	}
	
	Hash[0] += TmpHash[a];
	Hash[1] += TmpHash[b];
	Hash[2] += TmpHash[c];
	Hash[3] += TmpHash[d];
	Hash[4] += TmpHash[e];
	Hash[5] += TmpHash[f];
	Hash[6] += TmpHash[g];
	Hash[7] += TmpHash[h];
}

static u8* Sha_ExtractDigest(u32* Hash) {
	u8* Digest = (u8*)malloc(32 * sizeof(u8));
	
	for (u32 i = 0; i < 32; i += 4) {
		Digest[i] = (u8)((Hash[i / 4] >> 24) & 0x000000FF);
		Digest[i + 1] = (u8)((Hash[i / 4] >> 16) & 0x000000FF);
		Digest[i + 2] = (u8)((Hash[i / 4] >> 8) & 0x000000FF);
		Digest[i + 3] = (u8)(Hash[i / 4] & 0x000000FF);
	}
	
	return Digest;
}

u8* Sys_Sha256(u8* data, u64 size) {
	u32 W[64];
	u32 Hash[8] = {
		0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
		0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
	};
	u8* Digest;
	u64 RemainingDataSizeByte = size;
	
	while (Sha_CreateCompleteScheduleArray(data, size, &RemainingDataSizeByte, W) == 1) {
		Sha_CompleteScheduleArray(W);
		Sha_Compression(Hash, W);
	}
	Sha_CompleteScheduleArray(W);
	Sha_Compression(Hash, W);
	
	Digest = Sha_ExtractDigest(Hash);
	
	return Digest;
}
