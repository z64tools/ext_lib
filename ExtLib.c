#define __EXTLIB_C__

#define THIS_EXTLIB_VERSION 116

#ifndef EXTLIB
#warning ExtLib Version not defined
#else
#if EXTLIB > THIS_EXTLIB_VERSION
#warning Your local ExtLib copy seems to be old. Please update it!
#endif
#endif

#ifdef __IDE_FLAG__

#ifdef _WIN32
#undef _WIN32
#endif
void readlink(char*, char*, int);
void chdir(const char*);
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

#ifdef _WIN32
#include <windows.h>
#include <libloaderapi.h>
#endif

PrintfSuppressLevel gPrintfSuppress = 0;
char* sPrintfPrefix = "ExtLib";
u8 sPrintfType = 1;
u8 gPrintfProgressing;
u8* sSegment[255];
char* sPrintfPreType[][4] = {
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
u8 sBuffer_Temp[MbToBin(16)];
u32 sSeek_Temp = 0;
time_t sTime;
MemFile sLog;

pthread_mutex_t ___gExt_ThreadLock;
volatile bool ___gExt_ThreadInit;

// # # # # # # # # # # # # # # # # # # # #
// # THREAD                              #
// # # # # # # # # # # # # # # # # # # # #

void Thread_Init(void) {
	pthread_mutex_init(&___gExt_ThreadLock, NULL);
	___gExt_ThreadInit = true;
}

void Thread_Free(void) {
	pthread_mutex_destroy(&___gExt_ThreadLock);
	___gExt_ThreadInit = false;
}

void Thread_Lock(void) {
	if (___gExt_ThreadInit) {
		pthread_mutex_lock(&___gExt_ThreadLock);
	}
}

void Thread_Unlock(void) {
	if (___gExt_ThreadInit) {
		pthread_mutex_unlock(&___gExt_ThreadLock);
	}
}

void Thread_Create(Thread* thread, void* func, void* arg) {
	if (___gExt_ThreadInit == false) printf_error("Thread not Initialized");
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

void SetSegment(const u8 id, void* segment) {
	sSegment[id] = segment;
}

void* SegmentedToVirtual(const u8 id, void32 ptr) {
	if (sSegment[id] == NULL)
		printf_error("Segment 0x%X == NULL", id);
	
	return &sSegment[id][ptr];
}

void32 VirtualToSegmented(const u8 id, void* ptr) {
	return (uPtr)ptr - (uPtr)sSegment[id];
}

// # # # # # # # # # # # # # # # # # # # #
// # TMP                                 #
// # # # # # # # # # # # # # # # # # # # #

void* Tmp_Alloc(u32 size) {
	Thread_Lock();
	u8* ret;
	
	if (size >= sizeof(sBuffer_Temp) / 2)
		printf_error("Can't fit %fMb into the GraphBuffer", BinToMb(size));
	
	if (size == 0)
		return NULL;
	
	if (sSeek_Temp + size + 0x10 > sizeof(sBuffer_Temp)) {
		Log(__FUNCTION__, __LINE__, "" PRNT_YELW "Tmp_Alloc: rewind\a");
		sSeek_Temp = 0;
	}
	
	ret = &sBuffer_Temp[sSeek_Temp];
	memset(ret, 0, size + 1);
	sSeek_Temp = sSeek_Temp + size + 1;
	
	Thread_Unlock();
	
	return ret;
}

char* Tmp_String(char* str) {
	char* ret = Tmp_Alloc(strlen(str));
	
	strcpy(ret, str);
	
	return ret;
}

char* Tmp_Printf(char* fmt, ...) {
	char tempBuf[512 * 2];
	
	va_list args;
	
	va_start(args, fmt);
	vsnprintf(tempBuf, ArrayCount(tempBuf), fmt, args);
	va_end(args);
	
	return Tmp_String(tempBuf);
}

// # # # # # # # # # # # # # # # # # # # #
// # TIME                                #
// # # # # # # # # # # # # # # # # # # # #

struct timeval sTimeStart, sTimeStop;

void Time_Start(void) {
	gettimeofday(&sTimeStart, NULL);
}

f32 Time_Get(void) {
	gettimeofday(&sTimeStop, NULL);
	
	return (sTimeStop.tv_sec - sTimeStart.tv_sec) + (f32)(sTimeStop.tv_usec - sTimeStart.tv_usec) / 1000000;
}

// # # # # # # # # # # # # # # # # # # # #
// # DIR                                 #
// # # # # # # # # # # # # # # # # # # # #

void Dir_SetParam(DirCtx* ctx, DirParam w) {
	ctx->param |= w;
}

void Dir_UnsetParam(DirCtx* ctx, DirParam w) {
	ctx->param &= ~(w);
}

void Dir_Set(DirCtx* ctx, char* path, ...) {
	s32 firstSet = 0;
	va_list args;
	
	if (ctx->curPath[0] == '\0')
		firstSet++;
	
	memset(ctx->curPath, 0, 512);
	va_start(args, path);
	vsnprintf(ctx->curPath, ArrayCount(ctx->curPath), path, args);
	va_end(args);
	
	if (firstSet) {
		for (s32 i = 0; i < 512; i++) {
			ctx->enterCount[i] = 0;
		}
		for (s32 i = 0; i < strlen(ctx->curPath); i++) {
			if (ctx->curPath[i] == '/' || ctx->curPath[i] == '\\')
				ctx->enterCount[++ctx->pos] = 1;
		}
	}
}

void Dir_Enter(DirCtx* ctx, char* fmt, ...) {
	va_list args;
	char buffer[512];
	char spacing[512] = { 0 };
	
	va_start(args, fmt);
	vsnprintf(buffer, ArrayCount(buffer), fmt, args);
	va_end(args);
	
	if (!(ctx->param & DIR__MAKE_ON_ENTER)) {
		if (!Dir_Stat(ctx, buffer)) {
			printf_error("Could not enter folder [%s]", ctx->curPath, buffer);
		}
	}
	
	ctx->pos++;
	ctx->enterCount[ctx->pos] = 0;
	
	for (s32 i = 0;; i++) {
		if (buffer[i] == '\0')
			break;
		if (buffer[i] == '/' || buffer[i] == '\\')
			ctx->enterCount[ctx->pos]++;
	}
	
	for (s32 i = 0; i < ctx->pos; i++)
		strcat(spacing, "  ");
	Log("Dir_Enter/Leave", __LINE__, PRNT_BLUE "--> %s%s", spacing, buffer);
	
	strcat(ctx->curPath, buffer);
	
	if (ctx->param & DIR__MAKE_ON_ENTER) {
		Dir_MakeCurrent(ctx);
	}
}

void Dir_Leave(DirCtx* ctx) {
	char buf[512];
	s32 count = ctx->enterCount[ctx->pos];
	char spacing[512] = { 0 };
	
	for (s32 i = 0; i < count; i++) {
		ctx->curPath[strlen(ctx->curPath) - 1] = '\0';
		strcpy(buf, ctx->curPath);
		strcpy(ctx->curPath, String_GetPath(ctx->curPath));
	}
	
	ctx->enterCount[ctx->pos] = 0;
	ctx->pos--;
	
	for (s32 i = 0; i < ctx->pos; i++)
		strcat(spacing, "  ");
	
	Log("Dir_Enter/Leave", __LINE__, PRNT_REDD "<-- %s%s/", spacing, buf + strlen(ctx->curPath));
}

void Dir_Make(DirCtx* ctx, char* dir, ...) {
	char argBuf[512];
	char buf[512];
	va_list args;
	
	va_start(args, dir);
	vsnprintf(argBuf, ArrayCount(argBuf), dir, args);
	va_end(args);
	
	snprintf(buf, 512, "%s%s", ctx->curPath, argBuf);
	
	Sys_MakeDir(buf);
}

void Dir_MakeCurrent(DirCtx* ctx) {
	Sys_MakeDir(ctx->curPath);
}

char* Dir_Current(DirCtx* ctx) {
	return ctx->curPath;
}

char* Dir_File(DirCtx* ctx, char* fmt, ...) {
	char* buffer;
	char argBuf[512];
	va_list args;
	
	va_start(args, fmt);
	vsnprintf(argBuf, ArrayCount(argBuf), fmt, args);
	va_end(args);
	
	if (StrStr(argBuf, "*")) {
		return Dir_GetWildcard(ctx, argBuf);
	}
	
	buffer = Tmp_Printf("%s%s", ctx->curPath, argBuf);
	
	return buffer;
}

Time Dir_Stat(DirCtx* ctx, const char* item) {
	struct stat st = { 0 };
	char buf[512];
	
	snprintf(buf, 512, "%s%s", ctx->curPath, item);
	
	if (stat(buf, &st) == -1)
		return 0;
	
	if (st.st_mtime == 0)
		printf_error("Sys_Stat: [%s] time is zero?!", item);
	
	return st.st_mtime;
}

// @bug: SegFaults on empty ctx->curPath
char* Dir_GetWildcard(DirCtx* ctx, char* x) {
	ItemList list;
	char* sEnd;
	char* sStart = NULL;
	char* restorePath;
	char* search = StrStr(x, "*");
	char* posPath;
	
	if (search == NULL)
		return NULL;
	
	sEnd = Tmp_String(&search[1]);
	posPath = String_GetPath(Tmp_Printf("%s%s", ctx->curPath, x));
	
	if ((uPtr)search - (uPtr)x > 0) {
		sStart = Tmp_Alloc((uPtr)search - (uPtr)x + 2);
		memcpy(sStart, x, (uPtr)search - (uPtr)x);
	}
	
	restorePath = Tmp_String(ctx->curPath);
	
	if (strcmp(posPath, restorePath)) {
		Dir_Set(ctx, posPath);
	}
	
	Dir_ItemList(ctx, &list, false);
	
	if (strcmp(posPath, restorePath)) {
		Dir_Set(ctx, restorePath);
	}
	
	for (s32 i = 0; i < list.num; i++) {
		if (StrStr(list.item[i], sEnd) && (sStart == NULL || StrStr(list.item[i], sStart))) {
			return Tmp_Printf("%s%s", posPath, list.item[i]);
		}
	}
	
	return NULL;
}

void Dir_ItemList(DirCtx* ctx, ItemList* itemList, bool isPath) {
	DIR* dir = opendir(ctx->curPath);
	u32 bufSize = 0;
	struct dirent* entry;
	
	*itemList = ItemList_Initialize();
	
	if (dir == NULL)
		printf_error_align("Dir_ItemList", "Could not open: %s", ctx->curPath);
	
	while ((entry = readdir(dir)) != NULL) {
		if (isPath) {
			if (Sys_IsDir(Dir_File(ctx, entry->d_name))) {
				if (entry->d_name[0] == '.')
					continue;
				itemList->num++;
				bufSize += strlen(entry->d_name) + 2;
			}
		} else {
			if (!Sys_IsDir(Dir_File(ctx, entry->d_name))) {
				itemList->num++;
				bufSize += strlen(entry->d_name) + 2;
			}
		}
	}
	
	closedir(dir);
	
	if (itemList->num) {
		u32 i = 0;
		dir = opendir(ctx->curPath);
		itemList->buffer = Calloc(0, bufSize);
		itemList->item = Calloc(0, sizeof(char*) * itemList->num);
		
		while ((entry = readdir(dir)) != NULL) {
			if (isPath) {
				if (Sys_IsDir(Dir_File(ctx, entry->d_name))) {
					if (entry->d_name[0] == '.')
						continue;
					strcpy(&itemList->buffer[itemList->writePoint], entry->d_name);
					strcat(&itemList->buffer[itemList->writePoint], "/");
					itemList->item[i] = &itemList->buffer[itemList->writePoint];
					itemList->writePoint += strlen(itemList->item[i]) + 1;
					i++;
				}
			} else {
				if (!Sys_IsDir(Dir_File(ctx, entry->d_name))) {
					strcpy(&itemList->buffer[itemList->writePoint], entry->d_name);
					itemList->item[i] = &itemList->buffer[itemList->writePoint];
					itemList->writePoint += strlen(itemList->item[i]) + 1;
					i++;
				}
			}
		}
		closedir(dir);
	}
}

static void Dir_ItemList_Recursive_ChildCount(DirCtx* ctx, ItemList* target, char* pathTo, char* keyword) {
	ItemList folder = { 0 };
	ItemList file = { 0 };
	char* path;
	
	Dir_ItemList(ctx, &folder, true);
	Dir_ItemList(ctx, &file, false);
	
	for (s32 i = 0; i < folder.num; i++) {
		Dir_Enter(ctx, folder.item[i]); {
			path = Calloc(path, 0x128);
			sprintf(path, "%s%s", pathTo, folder.item[i]);
			
			Dir_ItemList_Recursive_ChildCount(ctx, target, path, keyword);
			Dir_Leave(ctx);
			Free(path);
		}
	}
	
	for (s32 i = 0; i < file.num; i++) {
		if (keyword && !StrStr(file.item[i], keyword) && !StrStr(pathTo, keyword))
			continue;
		target->num++;
		target->writePoint += strlen(pathTo) + strlen(file.item[i]) + 1;
	}
	
	ItemList_Free(&folder);
	ItemList_Free(&file);
}

static void Dir_ItemList_Recursive_ChildWrite(DirCtx* ctx, ItemList* target, char* pathTo, char* keyword) {
	ItemList folder = { 0 };
	ItemList file = { 0 };
	char* path;
	
	Dir_ItemList(ctx, &folder, true);
	Dir_ItemList(ctx, &file, false);
	
	for (s32 i = 0; i < folder.num; i++) {
		Dir_Enter(ctx, folder.item[i]); {
			path = Calloc(path, 0x128);
			sprintf(path, "%s%s", pathTo, folder.item[i]);
			
			Dir_ItemList_Recursive_ChildWrite(ctx, target, path, keyword);
			Dir_Leave(ctx);
			Free(path);
		}
	}
	
	for (s32 i = 0; i < file.num; i++) {
		if (keyword && !StrStr(file.item[i], keyword) && !StrStr(pathTo, keyword))
			continue;
		target->item[target->num] = &target->buffer[target->writePoint];
		sprintf(target->item[target->num], "%s%s", pathTo, file.item[i]);
		target->writePoint += strlen(target->item[target->num]) + 1;
		target->num++;
	}
	
	ItemList_Free(&folder);
	ItemList_Free(&file);
}

void Dir_ItemList_Recursive(DirCtx* ctx, ItemList* target, char* keyword) {
	Dir_ItemList_Recursive_ChildCount(ctx, target, "", keyword);
	if (target->num == 0) {
		Log(__FUNCTION__, __LINE__, "target->num == 0");
		memset(target, 0, sizeof(*target));
		
		return;
	}
	Log(__FUNCTION__, __LINE__, "target->num == %d", target->num);
	target->item = Calloc(0, sizeof(char*) * target->num);
	target->buffer = Calloc(0, target->writePoint);
	target->writePoint = 0;
	target->num = 0;
	Dir_ItemList_Recursive_ChildWrite(ctx, target, "", keyword);
}

void Dir_ItemList_Not(DirCtx* ctx, ItemList* itemList, bool isPath, char* not) {
	DIR* dir = opendir(ctx->curPath);
	u32 bufSize = 0;
	struct dirent* entry;
	
	*itemList = (ItemList) { 0 };
	
	if (dir == NULL)
		printf_error_align("Could not opendir()", "%s", ctx->curPath);
	
	while ((entry = readdir(dir)) != NULL) {
		if (isPath) {
			if (Sys_IsDir(Dir_File(ctx, entry->d_name))) {
				if (entry->d_name[0] == '.')
					continue;
				if (strcmp(entry->d_name, not) == 0)
					continue;
				itemList->num++;
				bufSize += strlen(entry->d_name) + 2;
			}
		} else {
			if (!Sys_IsDir(Dir_File(ctx, entry->d_name))) {
				itemList->num++;
				bufSize += strlen(entry->d_name) + 2;
			}
		}
	}
	
	closedir(dir);
	
	if (itemList->num) {
		u32 i = 0;
		dir = opendir(ctx->curPath);
		itemList->buffer = Calloc(0, bufSize);
		itemList->item = Calloc(0, sizeof(char*) * itemList->num);
		
		while ((entry = readdir(dir)) != NULL) {
			if (isPath) {
				if (Sys_IsDir(Dir_File(ctx, entry->d_name))) {
					if (entry->d_name[0] == '.')
						continue;
					if (strcmp(entry->d_name, not) == 0)
						continue;
					strcpy(&itemList->buffer[itemList->writePoint], entry->d_name);
					strcat(&itemList->buffer[itemList->writePoint], "/");
					itemList->item[i] = &itemList->buffer[itemList->writePoint];
					itemList->writePoint += strlen(itemList->item[i]) + 1;
					i++;
				}
			} else {
				if (!Sys_IsDir(Dir_File(ctx, entry->d_name))) {
					strcpy(&itemList->buffer[itemList->writePoint], entry->d_name);
					itemList->item[i] = &itemList->buffer[itemList->writePoint];
					itemList->writePoint += strlen(itemList->item[i]) + 1;
					i++;
				}
			}
		}
		closedir(dir);
	}
}

void Dir_ItemList_Keyword(DirCtx* ctx, ItemList* itemList, char* ext) {
	DIR* dir = opendir(ctx->curPath);
	u32 bufSize = 0;
	struct dirent* entry;
	
	*itemList = (ItemList) { 0 };
	
	if (dir == NULL)
		printf_error_align("Could not opendir()", "%s", ctx->curPath);
	
	while ((entry = readdir(dir)) != NULL) {
		if (!Sys_IsDir(Dir_File(ctx, entry->d_name))) {
			if (!StrStr(entry->d_name, ext))
				continue;
			itemList->num++;
			bufSize += strlen(entry->d_name) + 2;
		}
	}
	
	closedir(dir);
	
	if (itemList->num) {
		u32 i = 0;
		dir = opendir(ctx->curPath);
		itemList->buffer = Calloc(0, bufSize);
		itemList->item = Calloc(0, sizeof(char*) * itemList->num);
		
		while ((entry = readdir(dir)) != NULL) {
			if (!Sys_IsDir(Dir_File(ctx, entry->d_name))) {
				if (!StrStr(entry->d_name, ext))
					continue;
				strcpy(&itemList->buffer[itemList->writePoint], entry->d_name);
				itemList->item[i] = &itemList->buffer[itemList->writePoint];
				itemList->writePoint += strlen(itemList->item[i]) + 1;
				i++;
			}
		}
		closedir(dir);
	}
}

static void ItemList_Recursive_ChildCount(DirCtx* ctx, ItemList* target, const char* oriPath, char* pathTo, char* keyword) {
	ItemList* folder = Calloc(0, sizeof(ItemList));
	ItemList* file = Calloc(0, sizeof(ItemList));
	char* path;
	
	Dir_ItemList(ctx, folder, true);
	Dir_ItemList(ctx, file, false);
	
	for (s32 i = 0; i < folder->num; i++) {
		Dir_Enter(ctx, folder->item[i]); {
			path = Calloc(path, 0x128);
			sprintf(path, "%s%s", pathTo, folder->item[i]);
			
			ItemList_Recursive_ChildCount(ctx, target, oriPath, path, keyword);
			Dir_Leave(ctx);
			Free(path);
		}
	}
	
	for (s32 i = 0; i < file->num; i++) {
		if (keyword && !StrStr(file->item[i], keyword) && !StrStr(pathTo, keyword))
			continue;
		target->num++;
		target->writePoint += strlen(oriPath) + strlen(pathTo) + strlen(file->item[i]) + 1;
	}
	
	ItemList_Free(folder);
	ItemList_Free(file);
	Free(folder);
	Free(file);
}

static void ItemList_Recursive_ChildWrite(DirCtx* ctx, ItemList* target, const char* oriPath, char* pathTo, char* keyword) {
	ItemList* folder = Calloc(0, sizeof(ItemList));
	ItemList* file = Calloc(0, sizeof(ItemList));
	char* path;
	
	Dir_ItemList(ctx, folder, true);
	Dir_ItemList(ctx, file, false);
	
	for (s32 i = 0; i < folder->num; i++) {
		Dir_Enter(ctx, folder->item[i]); {
			path = Calloc(path, 0x128);
			sprintf(path, "%s%s", pathTo, folder->item[i]);
			
			ItemList_Recursive_ChildWrite(ctx, target, oriPath, path, keyword);
			Dir_Leave(ctx);
			Free(path);
		}
	}
	
	for (s32 i = 0; i < file->num; i++) {
		if (keyword && !StrStr(file->item[i], keyword) && !StrStr(pathTo, keyword))
			continue;
		target->item[target->num] = &target->buffer[target->writePoint];
		sprintf(target->item[target->num], "%s%s%s", oriPath, pathTo, file->item[i]);
		target->writePoint += strlen(target->item[target->num]) + 1;
		target->num++;
	}
	
	ItemList_Free(folder);
	ItemList_Free(file);
	Free(folder);
	Free(file);
}

void ItemList_Recursive(ItemList* target, const char* path, char* keyword, PathType fullPath) {
	DirCtx* dirCtx = Calloc(0, sizeof(DirCtx));
	char* oriPath = Calloc(0, 512);
	
	Dir_Set(dirCtx, "%s%s", Sys_WorkDir(), path);
	
	Log(__FUNCTION__, __LINE__, "Path: [%s]", dirCtx->curPath);
	
	if (fullPath) strcpy(oriPath, dirCtx->curPath);
	else strcpy(oriPath, path);
	
	ItemList_Recursive_ChildCount(dirCtx, target, oriPath, "", keyword);
	if (target->num == 0) {
		Log(__FUNCTION__, __LINE__, "target->num == 0");
		memset(target, 0, sizeof(*target));
		
		goto free;
	}
	Log(__FUNCTION__, __LINE__, "target->num == %d", target->num);
	target->item = Calloc(0, sizeof(char*) * target->num);
	target->buffer = Calloc(0, target->writePoint);
	target->writePoint = 0;
	target->num = 0;
	ItemList_Recursive_ChildWrite(dirCtx, target, oriPath, "", keyword);
	
free:
	Free(dirCtx);
	Free(oriPath);
}

static void ItemList_Validate(ItemList* itemList) {
	if (itemList->__private.initKey != 0xDEFABEBACECAFAFF)
		*itemList = ItemList_Initialize();
}

void ItemList_List(ItemList* target, const char* path, s32 depth, ListFlags flags) {
	bool isWordDir = false;
	u32 wplen;
	char** nftwList;
	u32 nftwBufLen;
	u32 nftwNum;
	
	ItemList_Validate(target);
	
	nftwList = Malloc(0, sizeof(char*) * 1024 * 16);
	nftwBufLen = 0;
	nftwNum = 0;
	
	int Nested(__list_item, (const char* item, const struct stat* bug, int type, struct FTW* ftw) ) {
		NestedVar(
			u32 nftwBufLen;
			u32 nftwNum;
		);
		u32 typeFlag = flags & 0xF;
		
		if (typeFlag == LIST_FILES) {
			if (type != FTW_F)
				return 0;
			
			if (depth > -1 && ftw->level > depth + 1)
				return 0;
			
			nftwList[nftwNum] = strdup(item);
			
			if (nftwList[nftwNum] == NULL) {
				Log(__FUNCTION__, __LINE__, "strdup(item);");
				
				return 1;
			}
		} else if (typeFlag == LIST_FOLDERS) {
			if (type != FTW_DP)
				return 0;
			
			if (depth > -1 && ftw->level != depth + 1)
				return 0;
			
			nftwList[nftwNum] = Calloc(0, strlen(item) + 3);
			
			if (nftwList[nftwNum] == NULL) {
				Log(__FUNCTION__, __LINE__, "strdup(item);");
				
				return 1;
			}
			
			strcpy(nftwList[nftwNum], item);
			strcat(nftwList[nftwNum], "/");
		} else return 0;
		
		Log(__FUNCTION__, __LINE__, "%3d/%-3d base:%-3d type:%-3d - %s", ftw->level, depth, ftw->base, type, item);
		
		nftwBufLen += strlen(nftwList[nftwNum]) + 1;
		nftwNum++;
		
		return 0;
	};
	
	if (strlen(path) == 0) {
		isWordDir = true;
		path = Sys_WorkDir();
		wplen = strlen(path);
	}
	
	if (nftw(path, (void*)__list_item, 80, FTW_DEPTH | FTW_MOUNT | FTW_PHYS))
		printf_error("nftw error: %s %d", __FUNCTION__, __LINE__);
	
	target->buffer = Malloc(0, nftwBufLen);
	target->item = Malloc(0, sizeof(char*) * nftwNum);
	target->num = nftwNum;
	
	for (s32 i = 0; i < nftwNum; i++) {
		target->item[i] = &target->buffer[target->writePoint];
		
		if (isWordDir)
			strcpy(target->item[i], &nftwList[i][wplen]);
		else
			strcpy(target->item[i], nftwList[i]);
		
		target->writePoint += strlen(target->item[i]) + 1;
		
		Free(nftwList[i]);
	}
	
	nftwList = Free(nftwList);
	
	Log(__FUNCTION__, __LINE__, "OK");
}

void ItemList_Print(ItemList* target) {
	for (s32 i = 0; i < target->num; i++)
		printf("[#]: %4d: %s\n", i, target->item[i]);
}

s32 ItemList_SaveList(ItemList* target, const char* output) {
	MemFile mem = MemFile_Initialize();
	
	MemFile_Malloc(&mem, 512 * target->num);
	
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
	
	for (s32 i = 0; i < list->num; i++) {
		if (String_GetInt(list->item[i]) > highestNum)
			highestNum = String_GetInt(list->item[i]);
	}
	
	sorted.buffer = Calloc(0, list->writePoint * 4);
	sorted.item = Calloc(0, sizeof(char*) * (highestNum + 1));
	
	for (s32 i = 0; i <= highestNum; i++) {
		u32 null = true;
		
		for (s32 j = 0; j < list->num; j++) {
			if (String_GetInt(list->item[j]) == i) {
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
	
	ItemList_Free(list);
	
	*list = sorted;
}

ItemList ItemList_Initialize(void) {
	return (ItemList) { .__private = { .initKey = 0xDEFABEBACECAFAFF } };
}

void ItemList_Free(ItemList* itemList) {
	Free(itemList->buffer);
	Free(itemList->item);
	itemList[0] = ItemList_Initialize();
}

// # # # # # # # # # # # # # # # # # # # #
// # SYS                                 #
// # # # # # # # # # # # # # # # # # # # #

bool Sys_IsDir(const char* path) {
	struct stat st = { 0 };
	
	stat(path, &st);
	
	if (S_ISDIR(st.st_mode)) {
		return true;
	}
	
	return false;
}

Time Sys_Stat_Ex(const char* item) {
	struct stat st = { 0 };
	Time t;
	
	if (stat(item, &st) == -1)
		return 0;
	
	if (st.st_atime > t)
		t = st.st_atime;
	if (st.st_mtime > t)
		t = st.st_mtime;
	if (st.st_ctime > t)
		t = st.st_ctime;
	
	return st.st_ctime;
}

Time Sys_Stat(const char* item) {
	struct stat st = { 0 };
	
	if (stat(item, &st) == -1)
		return 0;
	
	if (st.st_mtime == 0)
		printf_error("Sys_Stat: [%s] time is zero?!", item);
	
	return st.st_mtime;
}

Time Sys_StatSelf(void) {
	static char buf[512];
	
#ifdef _WIN32
	GetModuleFileName(NULL, buf, 512);
#else
	readlink("/proc/self/exe", buf, 512);
#endif
	
	return Sys_Stat(buf);
}

Time Sys_Time(void) {
	Time tme;
	
	time(&tme);
	
	return tme;
}

volatile bool sThreadOverlapFlag;

static void __MakeDir(const char* buffer) {
	if (Sys_Stat(buffer))
		return;
	
	struct stat st = { 0 };
	
	if (stat(buffer, &st) == -1) {
#ifdef _WIN32
		if (mkdir(buffer)) {
			if (!Sys_Stat(buffer))
				Log(__FUNCTION__, __LINE__, "mkdir error: [%s]", buffer);
		}
#else
		if (mkdir(buffer, 0700)) {
			if (!Sys_Stat(buffer))
				Log(__FUNCTION__, __LINE__, "mkdir error: [%s]", buffer);
		}
#endif
	}
}

void Sys_MakeDir(const char* dir, ...) {
	char buffer[512];
	s32 pathNum;
	va_list args;
	
	va_start(args, dir);
	vsnprintf(buffer, ArrayCount(buffer), dir, args);
	va_end(args);
	
	pathNum = String_GetPathNum(buffer);
	
	if (pathNum == 1) {
		__MakeDir(buffer);
	} else {
		for (s32 i = 0; i < pathNum; i++) {
			char tempBuff[512];
			char* folder = String_GetFolder(buffer, 0);
			
			strcpy(tempBuff, folder);
			for (s32 j = 1; j < i + 1; j++) {
				folder = String_GetFolder(buffer, j);
				strcat(tempBuff, folder);
			}
			
			__MakeDir(tempBuff);
		}
	}
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
	static char buf[512];
	
#ifdef _WIN32
	GetModuleFileName(NULL, buf, 512);
#else
	readlink("/proc/self/exe", buf, 512);
#endif
	
	return String_GetPath(buf);
}

s32 Sys_Rename(const char* input, const char* output) {
	s32 r = rename(input, output);
	
	return r;
}

static int __rm_func(const char* item, const struct stat* bug, int type, struct FTW* ftw) {
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
	if (nftw(item, __rm_func, 10, FTW_DEPTH | FTW_MOUNT | FTW_PHYS))
		printf_error("nftw error: %s %s", item, __FUNCTION__);
	
	return 0;
}

void Sys_SetWorkDir(const char* txt) {
	chdir(txt);
}

bool Sys_Command(const char* cmd) {
	if (system(cmd)) {
		Log(__FUNCTION__, __LINE__, PRNT_REDD "%s", cmd);
		
		return 1;
	}
	
	return 0;
}

char* Sys_CommandOut(const char* cmd) {
	FILE* file = popen(cmd, "r");
	char result[257] = { 0x0 };
	char* out;
	MemFile mem = MemFile_Initialize();
	
	if (file == NULL) {
		Log(__FUNCTION__, __LINE__, PRNT_REDD "%s", cmd);
		printf_error_align("Sys_CommandOut", "popen Failed");
	}
	
	MemFile_Malloc(&mem, MbToBin(32.0));
	while (fgets(result, 256, file) != NULL)
		MemFile_Printf(&mem, "%s", result);
	
	out = Calloc(0, mem.dataSize);
	strcpy(out, mem.data);
	
	Log(__FUNCTION__, __LINE__, "Size: %.2f", BinToKb(mem.dataSize));
	
	if (pclose(file)) {
		Log(__FUNCTION__, __LINE__, PRNT_REDD "%s", mem.str);
		Log(__FUNCTION__, __LINE__, PRNT_REDD "%s", cmd);
		printf_error_align("Sys_CommandOut", "Exit Status Failed");
	}
	
	return out;
}

void Sys_TerminalSize(s32* r) {
	s32 x;
	s32 y;
	
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
	MemFile mem = MemFile_Initialize();
	
	if (Sys_Stat(file)) {
		if (MemFile_LoadFile(&mem, file))
			return 1;
	}
	
	if (MemFile_SaveFile(&mem, file)) {
		MemFile_Free(&mem);
		
		return 1;
	}
	
	MemFile_Free(&mem);
	
	return 0;
}

s32 Sys_Copy(const char* src, const char* dest, bool isStr) {
	MemFile a = MemFile_Initialize();
	
	if (isStr) {
		if (MemFile_LoadFile_String(&a, src))
			return -1;
		if (MemFile_SaveFile_String(&a, dest))
			return 1;
		MemFile_Free(&a);
	} else {
		if (MemFile_LoadFile(&a, src))
			return -1;
		if (MemFile_SaveFile(&a, dest))
			return 1;
		MemFile_Free(&a);
	}
	
	return 0;
}

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
	}
	
	if (ans[0] == 'N' || ans[0] == 'n')
		return false;
	
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
	
	printf("\r" PRNT_GRAY "[" PRNT_DGRY "<" PRNT_GRAY "]: " PRNT_GRAY);
	fgets(str, 511, stdin);
	
	return str;
}

// # # # # # # # # # # # # # # # # # # # #
// # PRINTF                              #
// # # # # # # # # # # # # # # # # # # # #

void printf_SetSuppressLevel(PrintfSuppressLevel lvl) {
	gPrintfSuppress = lvl;
}

void printf_SetPrefix(char* fmt) {
	sPrintfPrefix = fmt;
}

void printf_SetPrintfTypes(const char* d, const char* w, const char* e, const char* i) {
	sPrintfType = 0;
	sPrintfPreType[sPrintfType][0] = String_Generate(d);
	sPrintfPreType[sPrintfType][1] = String_Generate(w);
	sPrintfPreType[sPrintfType][2] = String_Generate(e);
	sPrintfPreType[sPrintfType][3] = String_Generate(i);
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
	
	va_start(args, fmt);
	
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
	vprintf(
		fmt,
		args
	);
	va_end(args);
}

static void __printf_call(u32 type, char* dest) {
	char* color[4] = {
		PRNT_PRPL,
		PRNT_REDD,
		PRNT_REDD,
		PRNT_BLUE
	};
	
	if (dest) {
		sprintf(
			dest,
			"\r%s"
			PRNT_GRAY "["
			"%s%s"
			PRNT_GRAY "]: "
			PRNT_RSET,
			sPrintfPrefix,
			color[type],
			sPrintfPreType[sPrintfType][type]
		);
		
		return;
	}
	
	printf(
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

void printf_debug(const char* fmt, ...) {
	if (gPrintfSuppress > PSL_DEBUG)
		return;
	
	if (gPrintfProgressing) {
		printf("\n");
		gPrintfProgressing = false;
	}
	
	va_list args;
	
	va_start(args, fmt);
	__printf_call(0, 0);
	vprintf(
		fmt,
		args
	);
	printf(PRNT_RSET "\n");
	va_end(args);
}

void printf_debug_align(const char* info, const char* fmt, ...) {
	if (gPrintfSuppress > PSL_DEBUG)
		return;
	
	if (gPrintfProgressing) {
		printf("\n");
		gPrintfProgressing = false;
	}
	
	va_list args;
	
	va_start(args, fmt);
	__printf_call(0, 0);
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

void printf_warning(const char* fmt, ...) {
	if (gPrintfSuppress >= PSL_NO_WARNING)
		return;
	
	if (gPrintfProgressing) {
		printf("\n");
		gPrintfProgressing = false;
	}
	
	va_list args;
	
	va_start(args, fmt);
	__printf_call(1, 0);
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
	
	if (gPrintfProgressing) {
		printf("\n");
		gPrintfProgressing = false;
	}
	
	va_list args;
	
	va_start(args, fmt);
	__printf_call(1, 0);
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

void printf_error(const char* fmt, ...) {
	Log_Print();
	Log_Free();
	if (gPrintfSuppress < PSL_NO_ERROR) {
		if (gPrintfProgressing) {
			printf("\n");
			gPrintfProgressing = false;
		}
		
		va_list args;
		
		printf("\n");
		va_start(args, fmt);
		__printf_call(2, 0);
		vprintf(
			fmt,
			args
		);
		printf("\n");
		va_end(args);
	}
	
	exit(EXIT_FAILURE);
}

void printf_error_align(const char* info, const char* fmt, ...) {
	Log_Print();
	Log_Free();
	if (gPrintfSuppress < PSL_NO_ERROR) {
		if (gPrintfProgressing) {
			printf("\n");
			gPrintfProgressing = false;
		}
		
		va_list args;
		
		va_start(args, fmt);
		__printf_call(2, 0);
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
	
	exit(EXIT_FAILURE);
}

void printf_info(const char* fmt, ...) {
	char printfBuf[512];
	char buf[512];
	
	if (gPrintfSuppress >= PSL_NO_INFO)
		return;
	
	if (gPrintfProgressing) {
		printf("\n");
		gPrintfProgressing = false;
	}
	va_list args;
	
	va_start(args, fmt);
	__printf_call(3, printfBuf);
	vsprintf(
		buf,
		fmt,
		args
	); strcat(printfBuf, buf);
	va_end(args);
	
	strcat(printfBuf, "" PRNT_RSET "\n");
	printf("%s", printfBuf);
}

void printf_info_align(const char* info, const char* fmt, ...) {
	char printfBuf[512];
	char buf[512];
	
	if (gPrintfSuppress >= PSL_NO_INFO)
		return;
	
	if (gPrintfProgressing) {
		printf("\n");
		gPrintfProgressing = false;
	}
	va_list args;
	
	va_start(args, fmt);
	__printf_call(3, printfBuf);
	sprintf(
		buf,
		"%-16s " PRNT_RSET,
		info
	); strcat(printfBuf, buf);
	vsprintf(
		buf,
		fmt,
		args
	); strcat(printfBuf, buf);
	va_end(args);
	
	strcat(printfBuf, "" PRNT_RSET "\n");
	
	printf("%s", printfBuf);
}

void printf_prog_align(const char* info, const char* fmt, const char* color) {
	char printfBuf[512];
	char buf[512];
	
	if (gPrintfSuppress >= PSL_NO_INFO)
		return;
	
	if (gPrintfProgressing) {
		printf("\n");
		gPrintfProgressing = false;
	}
	
	__printf_call(3, printfBuf);
	sprintf(
		buf,
		"%-16s " PRNT_RSET,
		info
	);
	strcat(printfBuf, buf);
	if (color)
		strcat(printfBuf, color);
	strcat(printfBuf, fmt);
	
	Terminal_ClearLines(1);
	printf("%s", printfBuf);
}

void printf_progress(const char* info, u32 a, u32 b) {
	if (gPrintfSuppress >= PSL_NO_INFO) {
		return;
	}
	
	static f32 lstPrcnt;
	f32 prcnt = (f32)a / (f32)b;
	
	if (lstPrcnt > prcnt)
		lstPrcnt = 0;
	
	if (prcnt - lstPrcnt > 0.125) {
		lstPrcnt = prcnt;
	} else {
		if (a != b) {
			return;
		}
	}
	
	printf("\r");
	__printf_call(3, 0);
	printf(
		"%-16s " PRNT_RSET,
		info
	);
	printf("[%d / %d]", a, b);
	gPrintfProgressing = true;
	
	if (a == b) {
		gPrintfProgressing = false;
		printf("\n");
	}
}

void printf_getchar(const char* txt) {
	printf_info("%s", txt);
	printf("\r" PRNT_GRAY "[" PRNT_DGRY "<" PRNT_GRAY "]: " PRNT_RSET);
	getchar();
}

void printf_WinFix(void) {
#ifdef _WIN32
	system("\0");
#endif
}

// # # # # # # # # # # # # # # # # # # # #
// # VARIOUS                             #
// # # # # # # # # # # # # # # # # # # # #

void* MemMem(const void* haystack, size_t haystackSize, const void* needle, size_t needleSize) {
	if (haystack == NULL || needle == NULL)
		return NULL;
	register char* cur, * last;
	const char* cl = (const char*)haystack;
	const char* cs = (const char*)needle;
	
	/* we need something to compare */
	if (haystackSize == 0 || needleSize == 0)
		return NULL;
	
	/* "s" must be smaller or equal to "l" */
	if (haystackSize < needleSize)
		return NULL;
	
	/* special case where s_len == 1 */
	if (needleSize == 1)
		return memchr(haystack, (int)*cs, haystackSize);
	
	/* the last position where its possible to find "s" in "l" */
	last = (char*)cl + haystackSize - needleSize;
	
	for (cur = (char*)cl; cur <= last; cur++) {
		if (*cur == *cs && memcmp(cur, cs, needleSize) == 0)
			return cur;
	}
	
	return NULL;
}

void* MemMemCase(const void* haystack, size_t haystackSize, const void* needle, size_t needleSize) {
	if (haystack == NULL || needle == NULL)
		return NULL;
	register char* cur, * last;
	char* cl = (char*)haystack;
	char* cs = (char*)needle;
	
	/* we need something to compare */
	if (haystackSize == 0 || needleSize == 0)
		return NULL;
	
	/* "s" must be smaller or equal to "l" */
	if (haystackSize < needleSize)
		return NULL;
	
	/* special case where s_len == 1 */
	if (needleSize == 1)
		return memchr(haystack, (int)*cs, haystackSize);
	
	/* the last position where its possible to find "s" in "l" */
	last = (char*)cl + haystackSize - needleSize;
	
	for (cur = (char*)cl; cur <= last; cur++) {
		if (tolower(*cur) == tolower(*cs) && String_CaseComp(cur, cs, needleSize) == 0)
			return cur;
	}
	
	return NULL;
}

void* MemMemAlign(u32 val, const void* haystack, size_t haySize, const void* needle, size_t needleSize) {
	const u8* hay = haystack;
	const u8* nee = needle;
	
	if (haySize == 0 || needleSize == 0)
		return NULL;
	
	if (haystack == NULL || needle == NULL)
		return NULL;
	
	if (haySize < needleSize) {
		return NULL;
	}
	
	for (s32 i = 0;; i++) {
		if (i * val > haySize - needleSize)
			return NULL;
		
		if (hay[i * val] == nee[0]) {
			if (memcmp(&hay[i * val], nee, needleSize) == 0) {
				return (void*)&hay[i * val];
			}
		}
	}
}

void* MemMemU16(void* haystack, size_t haySize, const void* needle, size_t needleSize) {
	u16* hay = haystack;
	const u16* nee = needle;
	const u16* neeEnd = nee + needleSize / sizeof(*nee);
	const u16* hayEnd = hay + ((haySize - needleSize) / sizeof(*hay));
	
	/* guarantee alignment */
	assert((((uPtr)haystack) & 0x1) == 0);
	assert((((uPtr)needle) & 0x1) == 0);
	assert((haySize & 0x1) == 0);
	assert((needleSize & 0x1) == 0);
	
	if (haySize == 0 || needleSize == 0)
		return NULL;
	
	if (haystack == NULL || needle == NULL)
		return NULL;
	
	if (haySize < needleSize)
		return NULL;
	
	while (hay <= hayEnd) {
		const u16* neeNow = nee;
		const u16* hayNow = hay;
		
		while (neeNow != neeEnd) {
			if (*neeNow != *hayNow)
				goto L_next;
			++neeNow;
			++hayNow;
		}
		
		return hay;
L_next:
		++hay;
	}
	
	return 0;
}

void* MemMemU32(void* haystack, size_t haySize, const void* needle, size_t needleSize) {
	u32* hay = haystack;
	const u32* nee = needle;
	const u32* neeEnd = nee + needleSize / sizeof(*nee);
	const u32* hayEnd = hay + ((haySize - needleSize) / sizeof(*hay));
	
	/* guarantee alignment */
	assert((((uPtr)haystack) & 0x3) == 0);
	assert((((uPtr)needle) & 0x3) == 0);
	assert((haySize & 0x3) == 0);
	assert((needleSize & 0x3) == 0);
	
	if (haySize == 0 || needleSize == 0)
		return NULL;
	
	if (haystack == NULL || needle == NULL)
		return NULL;
	
	if (haySize < needleSize)
		return NULL;
	
	while (hay <= hayEnd) {
		const u32* neeNow = nee;
		const u32* hayNow = hay;
		
		while (neeNow != neeEnd) {
			if (*neeNow != *hayNow)
				goto L_next;
			++neeNow;
			++hayNow;
		}
		
		return hay;
L_next:
		++hay;
	}
	
	return 0;
}

void* MemMemU64(void* haystack, size_t haySize, const void* needle, size_t needleSize) {
	u64* hay = haystack;
	const u64* nee = needle;
	const u64* neeEnd = nee + needleSize / sizeof(*nee);
	const u64* hayEnd = hay + ((haySize - needleSize) / sizeof(*hay));
	
	/* guarantee alignment */
	assert((((uPtr)haystack) & 0xf) == 0);
	assert((((uPtr)needle) & 0xf) == 0);
	assert((haySize & 0xf) == 0);
	assert((needleSize & 0xf) == 0);
	
	if (haySize == 0 || needleSize == 0)
		return NULL;
	
	if (haystack == NULL || needle == NULL)
		return NULL;
	
	if (haySize < needleSize)
		return NULL;
	
	while (hay <= hayEnd) {
		const u64* neeNow = nee;
		const u64* hayNow = hay;
		
		while (neeNow != neeEnd) {
			if (*neeNow != *hayNow)
				goto L_next;
			++neeNow;
			++hayNow;
		}
		
		return hay;
L_next:
		++hay;
	}
	
	return 0;
}

const char* StrEnd(const char* src, const char* ext) {
	const char* fP;
	
	if ((fP = StrStr(src, ext)) != NULL && strlen(fP) == strlen(ext))
		return fP;
	
	return NULL;
}

const char* StrEndCase(const char* src, const char* ext) {
	const char* fP;
	
	if ((fP = StrStrCase(src, ext)) != NULL && strlen(fP) == strlen(ext))
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

void* Malloc(void* data, s32 size) {
	data = malloc(size);
	
	if (data == NULL) {
		printf_error("Could not allocate [0x%X] bytes.", size);
	}
	
	return data;
}

void* Calloc(void* data, s32 size) {
	data = Malloc(data, size);
	if (data != NULL) {
		memset(data, 0, size);
	}
	
	return data;
}

void* Realloc(void* data, s32 size) {
	data = realloc(data, size);
	
	if (data == NULL) {
		printf_error("Could not reallocate to [0x%X] bytes.", size);
	}
	
	return data;
}

void* Free(void* data) {
	if (data)
		free(data);
	
	return NULL;
}

s32 ParseArgs(char* argv[], char* arg, u32* parArg) {
	char* s = Tmp_Printf("%s", arg);
	char* ss = Tmp_Printf("-%s", arg);
	char* sss = Tmp_Printf("--%s", arg);
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

u32 Crc32(u8* s, u32 n) {
	u32 crc = 0xFFFFFFFF;
	
	for (u32 i = 0; i < n; i++) {
		u8 ch = s[i];
		for (s32 j = 0; j < 8; j++) {
			u32 b = (ch ^ crc) & 1;
			crc >>= 1;
			if (b) crc = crc ^ 0xEDB88320;
			ch >>= 1;
		}
	}
	
	return ~crc;
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

MemFile MemFile_Initialize() {
	return (MemFile) { .param.initKey = 0xD0E0A0D0B0E0E0F0 };
}

void MemFile_Params(MemFile* memFile, ...) {
	va_list args;
	u32 cmd;
	u32 arg;
	
	va_start(args, memFile);
	for (;;) {
		cmd = va_arg(args, uPtr);
		
		if (cmd == MEM_END) {
			break;
		}
		
		arg = va_arg(args, uPtr);
		
		if (cmd == MEM_CLEAR) {
			cmd = arg;
			arg = 0;
		}
		
		if (cmd == MEM_FILENAME) {
			memFile->param.getName = arg > 0 ? true : false;
		} else if (cmd == MEM_CRC32) {
			memFile->param.getCrc = arg > 0 ? true : false;
		} else if (cmd == MEM_ALIGN) {
			memFile->param.align = arg;
		} else if (cmd == MEM_REALLOC) {
			memFile->param.realloc = arg > 0 ? true : false;
		}
	}
	va_end(args);
}

void MemFile_Malloc(MemFile* memFile, u32 size) {
	if (memFile->param.initKey != 0xD0E0A0D0B0E0E0F0)
		*memFile = MemFile_Initialize();
	memFile->data = malloc(size);
	memset(memFile->data, 0, size);
	
	if (memFile->data == NULL) {
		printf_warning("Failed to malloc [0x%X] bytes.", size);
	}
	
	memFile->memSize = size;
}

void MemFile_Realloc(MemFile* memFile, u32 size) {
	if (memFile->memSize > size)
		return;
	
	// Make sure to have enough space
	if (size < memFile->memSize + 0x10000) {
		size += 0x10000;
	}
	
	memFile->data = realloc(memFile->data, size);
	memFile->memSize = size;
}

void MemFile_Rewind(MemFile* memFile) {
	memFile->seekPoint = 0;
}

s32 MemFile_Write(MemFile* dest, void* src, u32 size) {
	if (dest->seekPoint + size > dest->memSize) {
		if (!dest->param.realloc) {
			printf_warning_align(
				"MemSize exceeded",
				"%.2fkB / %.2fkB",
				BinToKb(dest->dataSize),
				BinToKb(dest->memSize)
			);
			
			return 1;
		}
		
		MemFile_Realloc(dest, dest->memSize * 2);
	}
	
	if (dest->seekPoint + size > dest->dataSize) {
		dest->dataSize = dest->seekPoint + size;
	}
	memcpy(&dest->cast.u8[dest->seekPoint], src, size);
	dest->seekPoint += size;
	
	if (dest->param.align) {
		if ((dest->seekPoint % dest->param.align) != 0) {
			MemFile_Align(dest, dest->param.align);
		}
	}
	
	return 0;
}

s32 MemFile_Append(MemFile* dest, MemFile* src) {
	return MemFile_Write(dest, src->data, src->dataSize);
}

void MemFile_Align(MemFile* src, u32 align) {
	if ((src->seekPoint % align) != 0) {
		u64 wow[2] = { 0 };
		u32 size = align - (src->seekPoint % align);
		
		MemFile_Write(src, wow, size);
	}
}

s32 MemFile_Printf(MemFile* dest, const char* fmt, ...) {
	char buffer[512];
	va_list args;
	
	va_start(args, fmt);
	vsnprintf(
		buffer,
		ArrayCount(buffer),
		fmt,
		args
	);
	va_end(args);
	
	return MemFile_Write(dest, buffer, strlen(buffer));
}

s32 MemFile_Read(MemFile* src, void* dest, u32 size) {
	if (src->seekPoint + size > src->dataSize) {
		return 1;
	}
	
	memcpy(dest, &src->cast.u8[src->seekPoint], size);
	src->seekPoint += size;
	
	return 0;
}

void* MemFile_Seek(MemFile* src, u32 seek) {
	if (seek > src->memSize) {
		printf_error("!");
	}
	src->seekPoint = seek;
	
	return (void*)&src->cast.u8[seek];
}

s32 MemFile_LoadFile(MemFile* memFile, const char* filepath) {
	u32 tempSize;
	FILE* file = fopen(filepath, "rb");
	struct stat sta;
	
	if (file == NULL) {
		printf_warning("Failed to open file [%s]", filepath);
		
		return 1;
	}
	
	fseek(file, 0, SEEK_END);
	tempSize = ftell(file);
	
	if (memFile->data == NULL) {
		MemFile_Malloc(memFile, tempSize);
		memFile->memSize = memFile->dataSize = tempSize;
		if (memFile->data == NULL) {
			printf_warning("Failed to malloc MemFile.\n\tAttempted size is [0x%X] bytes to store data from [%s].", tempSize, filepath);
			
			return 1;
		}
	} else {
		if (memFile->memSize < tempSize)
			MemFile_Realloc(memFile, tempSize * 2);
		memFile->dataSize = tempSize;
	}
	
	rewind(file);
	if (fread(memFile->data, 1, memFile->dataSize, file)) {
	}
	fclose(file);
	stat(filepath, &sta);
	memFile->info.age = sta.st_mtime;
	strcpy(memFile->info.name, filepath);
	
	if (memFile->param.getCrc) {
		memFile->info.crc32 = Crc32(memFile->data, memFile->dataSize);
	}
	
	return 0;
}

s32 MemFile_LoadFile_String(MemFile* memFile, const char* filepath) {
	u32 tempSize;
	FILE* file = fopen(filepath, "r");
	struct stat sta;
	
	if (file == NULL) {
		printf_warning("Failed to open file [%s]", filepath);
		
		return 1;
	}
	
	fseek(file, 0, SEEK_END);
	tempSize = ftell(file);
	
	if (memFile->data == NULL) {
		MemFile_Malloc(memFile, tempSize + 0x10);
		memFile->memSize = tempSize + 0x10;
		memFile->dataSize = tempSize;
		if (memFile->data == NULL) {
			printf_warning("Failed to malloc MemFile.\n\tAttempted size is [0x%X] bytes to store data from [%s].", tempSize, filepath);
			
			return 1;
		}
	} else {
		if (memFile->memSize < tempSize)
			MemFile_Realloc(memFile, tempSize * 2);
		memFile->dataSize = tempSize;
	}
	
	rewind(file);
	memFile->dataSize = fread(memFile->data, 1, memFile->dataSize, file);
	fclose(file);
	memFile->cast.u8[memFile->dataSize] = '\0';
	
	stat(filepath, &sta);
	memFile->info.age = sta.st_mtime;
	strcpy(memFile->info.name, filepath);
	
	if (memFile->param.getCrc) {
		memFile->info.crc32 = Crc32(memFile->data, memFile->dataSize);
	}
	
	return 0;
}

s32 MemFile_SaveFile(MemFile* memFile, const char* filepath) {
	FILE* file = fopen(filepath, "wb");
	
	if (file == NULL) {
		printf_warning("Failed to fopen file [%s] for writing.", filepath);
		
		return 1;
	}
	
	fwrite(memFile->data, sizeof(char), memFile->dataSize, file);
	fclose(file);
	
	return 0;
}

s32 MemFile_SaveFile_String(MemFile* memFile, const char* filepath) {
	FILE* file = fopen(filepath, "w");
	
	if (file == NULL) {
		printf_warning("Failed to fopen file [%s] for writing.", filepath);
		
		return 1;
	}
	
	fwrite(memFile->data, sizeof(char), memFile->dataSize, file);
	fclose(file);
	
	return 0;
}

s32 MemFile_LoadFile_ReqExt(MemFile* memFile, const char* filepath, const char* ext) {
	if (StrStrCase(filepath, ext)) {
		return MemFile_LoadFile(memFile, filepath);
	}
	printf_warning("[%s] does not match extension [%s]", filepath, ext);
	
	return 1;
}

s32 MemFile_SaveFile_ReqExt(MemFile* memFile, const char* filepath, s32 size, const char* ext) {
	if (StrStrCase(filepath, ext)) {
		return MemFile_SaveFile(memFile, filepath);
	}
	
	printf_warning("[%s] does not match extension [%s]", filepath, ext);
	
	return 1;
}

void MemFile_Free(MemFile* memFile) {
	if (memFile->data) {
		free(memFile->data);
		
		*memFile = MemFile_Initialize();
	}
}

void MemFile_Reset(MemFile* memFile) {
	memFile->dataSize = 0;
	memFile->seekPoint = 0;
}

void MemFile_Clear(MemFile* memFile) {
	memset(memFile->data, 0, memFile->memSize);
	MemFile_Reset(memFile);
}

// # # # # # # # # # # # # # # # # # # # #
// # STRING                              #
// # # # # # # # # # # # # # # # # # # # #

u32 String_GetHexInt(char* string) {
	return strtoul(string, NULL, 16);
}

s32 String_GetInt(char* string) {
	if (!memcmp(string, "0x", 2)) {
		return strtoul(string, NULL, 16);
	} else {
		return strtol(string, NULL, 10);
	}
}

f32 String_GetFloat(char* string) {
	f32 fl;
	u32 mal = 0;
	
	if (StrStr(string, ",")) {
		mal = true;
		string = strdup(string);
		String_Replace(string, ",", ".");
	}
	
	fl = strtod(string, NULL);
	if (mal)
		Free(string);
	
	return fl;
}

s32 String_GetLineCount(const char* str) {
	s32 line = 1;
	s32 i = 0;
	
	while (str[i++] != '\0') {
		if (str[i] == '\n' && str[i + 1] != '\0')
			line++;
	}
	
	return line;
}

s32 String_CaseComp(char* a, char* b, u32 compSize) {
	u32 wow = 0;
	
	for (s32 i = 0; i < compSize; i++) {
		wow = tolower(a[i]) - tolower(b[i]);
		if (wow)
			return 1;
	}
	
	return 0;
}

static void __GetSlashAndPoint(const char* src, s32* slash, s32* point) {
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

char* String_GetLine(const char* str, s32 line) {
	char* buffer;
	s32 iLine = -1;
	s32 i = 0;
	s32 j = 0;
	
	if (str == NULL)
		return NULL;
	
	while (str[i] != '\0') {
		j = 0;
		if (str[i] != '\n') {
			while (str[i + j] != '\n' && str[i + j] != '\0') {
				j++;
			}
			
			iLine++;
			
			if (iLine == line) {
				break;
			}
			
			i += j;
		} else {
			i++;
		}
	}
	
	buffer = Tmp_Alloc(j + 1);
	
	memcpy(buffer, &str[i], j);
	buffer[j] = '\0';
	
	return buffer;
}

char* String_GetWord(const char* str, s32 word) {
	char* buffer;
	s32 iWord = -1;
	s32 i = 0;
	s32 j = 0;
	
	if (str == NULL)
		return NULL;
	
	while (str[i] != '\0') {
		j = 0;
		if (str[i + j] > ' ') {
			while (str[i + j] > ' ') {
				j++;
			}
			
			iWord++;
			
			if (iWord == word) {
				break;
			}
			
			i += j;
		} else {
			i++;
		}
	}
	
	buffer = Tmp_Alloc(j + 1);
	
	memcpy(buffer, &str[i], j);
	buffer[j] = '\0';
	
	return buffer;
}

char* String_GetPath(const char* src) {
	char* buffer;
	s32 point;
	s32 slash;
	
	if (src == NULL)
		return NULL;
	
	__GetSlashAndPoint(src, &slash, &point);
	
	if (slash == 0)
		slash = -1;
	
	buffer = Tmp_Alloc(slash + 1 + 1);
	
	memcpy(buffer, src, slash + 1);
	buffer[slash + 1] = '\0';
	
	return buffer;
}

char* String_GetBasename(const char* src) {
	char* buffer;
	s32 point;
	s32 slash;
	
	if (src == NULL)
		return NULL;
	
	__GetSlashAndPoint(src, &slash, &point);
	
	// Offset away from the slash
	if (slash > 0)
		slash++;
	
	if (point < slash || slash + point == 0) {
		point = slash + 1;
		while (src[point] > ' ') point++;
	}
	
	buffer = Tmp_Alloc(point - slash + 1);
	
	memcpy(buffer, &src[slash], point - slash);
	buffer[point - slash] = '\0';
	
	return buffer;
}

char* String_GetFilename(const char* src) {
	char* buffer;
	s32 point;
	s32 slash;
	s32 ext = 0;
	
	if (src == NULL)
		return NULL;
	
	__GetSlashAndPoint(src, &slash, &point);
	
	// Offset away from the slash
	if (slash > 0)
		slash++;
	
	if (src[point + ext] == '.') {
		ext++;
		while (isalnum(src[point + ext])) ext++;
	}
	
	if (point < slash || slash + point == 0) {
		point = slash + 1;
		while (src[point] > ' ') point++;
	}
	
	buffer = Tmp_Alloc(point - slash + ext + 1);
	
	memcpy(buffer, &src[slash], point - slash + ext);
	buffer[point - slash + ext] = '\0';
	
	return buffer;
}

s32 String_GetPathNum(const char* src) {
	s32 dir = -1;
	
	for (s32 i = 0; i < strlen(src); i++) {
		if (src[i] == '/')
			dir++;
	}
	
	return dir + 1;
}

char* String_GetFolder(const char* src, s32 num) {
	char* buffer;
	s32 start = -1;
	s32 end;
	
	if (src == NULL)
		return NULL;
	
	if (num < 0) {
		num = String_GetPathNum(src) - 1;
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
	
	buffer = Tmp_Alloc(end - start + 1);
	
	memcpy(buffer, &src[start], end - start);
	buffer[end - start] = '\0';
	
	return buffer;
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
		
		return Tmp_String(tempBuf);
	}
	
	return argv[cur];
}

char* String_Line(char* str, s32 line) {
	s32 iLine = -1;
	s32 i = 0;
	s32 j = 0;
	
	if (str == NULL)
		return NULL;
	
	while (str[i] != '\0') {
		j = 0;
		if (str[i] != '\n') {
			while (str[i + j] != '\n' && str[i + j] != '\0') {
				j++;
			}
			
			iLine++;
			
			if (iLine == line) {
				break;
			}
			
			i += j;
		} else {
			i++;
		}
	}
	
	return &str[i];
}

char* String_LineHead(char* str) {
	s32 i = 1;
	
	if (str == NULL)
		return NULL;
	
	for (;; i--) {
		if (str[i - 1] == '\0')
			return NULL;
		if (str[i - 1] == '\n')
			return &str[i];
	}
}

char* String_Word(char* str, s32 word) {
	s32 iWord = -1;
	s32 i = 0;
	s32 j = 0;
	
	if (str == NULL)
		return NULL;
	
	while (str[i] != '\0') {
		j = 0;
		if (str[i + j] > ' ') {
			while (str[i + j] > ' ') {
				j++;
			}
			
			iWord++;
			
			if (iWord == word) {
				break;
			}
			
			i += j;
		} else {
			i++;
		}
	}
	
	return &str[i];
}

char* String_Extension(const char* str) {
	s32 slash;
	s32 point;
	
	__GetSlashAndPoint(str, &slash, &point);
	
	return (void*)&str[point];
}

void String_CaseToLow(char* s, s32 i) {
	for (s32 k = 0; k < i; k++) {
		if (s[k] >= 'A' && s[k] <= 'Z') {
			s[k] = s[k] + 32;
		}
	}
}

void String_CaseToUp(char* s, s32 i) {
	for (s32 k = 0; k < i; k++) {
		if (s[k] >= 'a' && s[k] <= 'z') {
			s[k] = s[k] - 32;
		}
	}
}

void String_Insert(char* point, const char* insert) {
	s32 insLen = strlen(insert);
	char* insEnd = point + insLen;
	s32 remLen = strlen(point);
	
	memmove(insEnd, point, remLen + 1);
	insEnd[remLen] = 0;
	memcpy(point, insert, insLen);
}

void String_InsertExt(char* origin, const char* insert, s32 pos, s32 size) {
	s32 inslen = strlen(insert);
	
	if (pos >= size)
		return;
	
	if (size - pos - inslen > 0)
		memmove(&origin[pos + inslen], &origin[pos], size - pos - inslen);
	
	for (s32 j = 0; j < inslen; pos++, j++) {
		origin[pos] = insert[j];
	}
}

void String_Remove(char* point, s32 amount) {
	char* get = point + amount;
	s32 len = strlen(get);
	
	if (len)
		memcpy(point, get, strlen(get));
	point[len] = 0;
}

s32 String_Replace(char* src, const char* word, const char* replacement) {
	s32 diff = 0;
	char* ptr;
	
	if (!StrStr(src, word)) return 0;
	
	if (strlen(word) == 1 && strlen(replacement) == 1) {
		for (s32 i = 0; i < strlen(src); i++) {
			if (src[i] == word[0]) {
				src[i] = replacement[0];
				break;
			}
		}
		
		return true;
	}
	
	ptr = StrStr(src, word);
	
	while (ptr != NULL) {
		String_Remove(ptr, strlen(word));
		String_Insert(ptr, replacement);
		ptr = StrStr(ptr + 1, word);
		diff = true;
	}
	
	return diff;
}

void String_SwapExtension(char* dest, char* src, const char* ext) {
	strcpy(dest, String_GetPath(src));
	strcat(dest, String_GetBasename(src));
	strcat(dest, ext);
}

s32 String_Validate_Hex(const char* str) {
	s32 isOk = false;
	
	for (s32 i = 0; i < strlen(str); i++) {
		if (
			(str[i] >= 'A' && str[i] <= 'F') ||
			(str[i] >= 'a' && str[i] <= 'f') ||
			(str[i] >= '0' && str[i] <= '9') ||
			str[i] == 'x' || str[i] == 'X'
		) {
			isOk = true;
			continue;
		}
		
		return false;
	}
	
	return isOk;
}

s32 String_Validate_Hex_Spaced(const char* str) {
	s32 isOk = false;
	
	for (s32 i = 0; i < strlen(str); i++) {
		if (
			(str[i] >= 'A' && str[i] <= 'F') ||
			(str[i] >= 'a' && str[i] <= 'f') ||
			(str[i] >= '0' && str[i] <= '9') ||
			str[i] == 'x' || str[i] == 'X' || str[i] == ' '
		) {
			isOk = true;
			continue;
		}
		
		return false;
	}
	
	return isOk;
}

// # # # # # # # # # # # # # # # # # # # #
// # CONFIG                              #
// # # # # # # # # # # # # # # # # # # # #

s32 sConfigSuppression;

void Config_SuppressNext(void) {
	sConfigSuppression = 1;
}

char* Config_Variable(const char* str, const char* name) {
	u32 lineCount = String_GetLineCount(str);
	char* line = (char*)str;
	
	for (s32 i = 0; i < lineCount; i++, line = String_Line(line, 1)) {
		if (line[0] == '#' || line[0] == ';' || line[0] <= ' ')
			continue;
		if (!strncmp(line, name, strlen(name))) {
			s32 isString = 0;
			char* p = line + strlen(name);
			u32 size = 0;
			
			while (p[0] == ' ' || p[0] == '\t')
				p++;
			
			if (p[0] != '=')
				return NULL;
			
			while (p[0] == '=' || p[0] == ' ' || p[0] == '\t')
				p++;
			
			while (p[size + 1] != ';' && p[size + 1] != '#' && p[size] != '\n' && (isString == false || p[size] != '\"') && p[size] != '\0') {
				if (p[size] == '\"')
					isString = 1;
				size++;
			}
			
			if (isString == false)
				while (p[size - 1] == ' ')
					size--;
			
			return p;
		}
	}
	
	return NULL;
}

char* Config_GetVariable(const char* str, const char* name) {
	u32 lineCount = String_GetLineCount(str);
	char* line = (char*)str;
	
	for (s32 i = 0; i < lineCount; i++, line = String_Line(line, 1)) {
		if (line[0] == '#' || line[0] == ';' || line[0] <= ' ')
			continue;
		if (!strncmp(line, name, strlen(name))) {
			s32 isString = 0;
			char* buf;
			char* p = line + strlen(name);
			u32 size = 0;
			
			while (p[0] == ' ' || p[0] == '\t')
				p++;
			
			if (p[0] != '=')
				return NULL;
			
			while (p[0] == '=' || p[0] == ' ' || p[0] == '\t')
				p++;
			
			while (p[size + 1] != ';' && p[size + 1] != '#' && p[size] != '\n' && (isString == false || p[size] != '\"') && p[size] != '\0') {
				if (p[size] == '\"')
					isString = 1;
				size++;
			}
			
			if (isString == false)
				while (p[size - 1] == ' ')
					size--;
			
			buf = Tmp_Alloc(size + 1);
			memcpy(buf, p + isString, size - isString);
			
			return buf;
		}
	}
	
	return NULL;
}

s32 Config_GetBool(MemFile* memFile, const char* boolName) {
	char* ptr;
	
	ptr = Config_GetVariable(memFile->str, boolName);
	if (ptr) {
		char* word = ptr;
		if (!strcmp(word, "true")) {
			return true;
		}
		if (!strcmp(word, "false")) {
			return false;
		}
	}
	
	if (sConfigSuppression == 0)
		printf_warning("[%s] is missing bool [%s]", memFile->info.name, boolName);
	else sConfigSuppression++;
	
	return 0;
}

s32 Config_GetOption(MemFile* memFile, const char* stringName, char* strList[]) {
	char* ptr;
	char* word;
	s32 i = 0;
	
	ptr = Config_GetVariable(memFile->str, stringName);
	if (ptr) {
		word = ptr;
		while (strList[i] != NULL && !StrStr(word, strList[i]))
			i++;
		
		if (strList != NULL)
			return i;
	}
	
	if (sConfigSuppression == 0)
		printf_warning("[%s] is missing option [%s]", memFile->info.name, stringName);
	else sConfigSuppression++;
	
	return 0;
}

s32 Config_GetInt(MemFile* memFile, const char* intName) {
	char* ptr;
	
	ptr = Config_GetVariable(memFile->str, intName);
	if (ptr) {
		return String_GetInt(ptr);
	}
	
	if (sConfigSuppression == 0)
		printf_warning("[%s] is missing integer [%s]", memFile->info.name, intName);
	else sConfigSuppression++;
	
	return 0;
}

char* Config_GetString(MemFile* memFile, const char* stringName) {
	char* ptr;
	
	ptr = Config_GetVariable(memFile->str, stringName);
	if (ptr) {
		return ptr;
	}
	
	if (sConfigSuppression == 0)
		printf_warning("[%s] is missing string [%s]", memFile->info.name, stringName);
	else sConfigSuppression++;
	
	return NULL;
}

f32 Config_GetFloat(MemFile* memFile, const char* floatName) {
	char* ptr;
	
	ptr = Config_GetVariable(memFile->str, floatName);
	if (ptr) {
		return String_GetFloat(ptr);
	}
	
	if (sConfigSuppression == 0)
		printf_warning("[%s] is missing float [%s]", memFile->info.name, floatName);
	else sConfigSuppression++;
	
	return 0.0f;
}

s32 Config_Replace(MemFile* mem, const char* variable, const char* fmt, ...) {
	char* replacement = Malloc(0, 0x10000);
	va_list va;
	char* p;
	
	va_start(va, fmt);
	vsprintf(replacement, fmt, va);
	va_end(va);
	
	p = Config_Variable(mem->str, variable);
	
	if (p) {
		if (p[0] == '"')
			p++;
		String_Remove(p, strlen(Config_GetVariable(mem->str, variable)));
		String_Insert(p, replacement);
		
		mem->dataSize = strlen(mem->str);
		Free(replacement);
		
		return 0;
	}
	
	Free(replacement);
	
	return 1;
}

// # # # # # # # # # # # # # # # # # # # #
// # LOG                                 #
// # # # # # # # # # # # # # # # # # # # #

#include <signal.h>

#define FAULT_BUFFER_SIZE (1024)
#define FAULT_LOG_NUM     64

char* sLogMsg[FAULT_LOG_NUM];
char* sLogFunc[FAULT_LOG_NUM];
u32 sLogLine[FAULT_LOG_NUM];

static void Log_Signal(int arg) {
	volatile static bool ran = 0;
	const char* errorMsg[] = {
		"\a0",
		"\a1",
		"\aForced Close", // SIGINT
		"\a3",
		"\aSIGILL",
		"\a5",
		"\aSIGABRT_COMPAT",
		"\a7",
		"\aSIGFPE",
		"\a9",
		"\a10",
		"\aSegmentation Fault",
		"\a12",
		"\a13",
		"\a14",
		"\aSIGTERM",
		"\a16",
		"\a17",
		"\a18",
		"\a19",
		"\a20",
		"\aSIGBREAK",
		"\aSIGABRT",
		
		"\aUNDEFINED",
	};
	u32 msgsNum = 0;
	u32 repeat = 0;
	s32 errorID = ClampMax(arg, 23);
	
	if (errorID < 1)
		errorID = 23;
	
	if (ran) return;
	ran = ___gExt_ThreadInit != 0;
	
	printf_WinFix();
	
	printf("\n");
	if (arg != 0xDEADBEEF)
		printf("\n" PRNT_DGRY "[" PRNT_REDD "!" PRNT_DGRY "]:" PRNT_DGRY " [ " PRNT_REDD "%s " PRNT_DGRY "]", errorMsg[ClampMax(arg, 23)]);
	else
		printf("\n" PRNT_DGRY "[" PRNT_REDD "!" PRNT_DGRY "]:" PRNT_DGRY " [ " PRNT_REDD "LOG " PRNT_DGRY "]");
	
	for (s32 i = FAULT_LOG_NUM - 1; i >= 0; i--) {
		if (strlen(sLogMsg[i]) > 0) {
			if (msgsNum == 0 || strcmp(sLogFunc[i], sLogFunc[i + 1]))
				printf("\n" PRNT_DGRY "[" PRNT_REDD "!" PRNT_DGRY "]:" PRNT_YELW " %s" PRNT_DGRY "();" PRNT_RSET, sLogFunc[i]);
			if (msgsNum == 0 || strcmp(sLogMsg[i], sLogMsg[i + 1])) {
				printf(
					"\n" PRNT_DGRY "[" PRNT_REDD "!" PRNT_DGRY "]:" PRNT_DGRY " [ %4d ]" PRNT_RSET " %s" PRNT_RSET,
					sLogLine[i],
					sLogMsg[i]
				);
				if (repeat) printf( PRNT_PRPL " x %d" PRNT_RSET, repeat + 1);
				repeat = 0;
			} else {
				repeat++;
			}
			msgsNum++;
		}
	}
	printf("\n");
	
	if (arg != 0xDEADBEEF && arg != 21) {
		printf(
			"\n" PRNT_DGRY "[" PRNT_REDD "!" PRNT_DGRY "]:" PRNT_YELW " Provide this log to the developer." PRNT_RSET "\n"
		);
#ifdef _WIN32
		Terminal_GetStr();
#endif
		exit(EXIT_FAILURE);
	}
}

void Log_Init() {
	for (s32 i = -100; i < 100; i++)
		if (i != 0)
			signal(i, Log_Signal);
	
	for (s32 i = 0; i < FAULT_LOG_NUM; i++) {
		sLogMsg[i] = Calloc(0, FAULT_BUFFER_SIZE);
		sLogFunc[i] = Calloc(0, FAULT_BUFFER_SIZE * 0.25);
	}
}

void Log_Free() {
	for (s32 i = 0; i < FAULT_LOG_NUM; i++) {
		Free(sLogMsg[i]);
		Free(sLogFunc[i]);
	}
}

void Log_Print() {
	if (sLogMsg[0][0] != 0)
		Log_Signal(0xDEADBEEF);
}

void Log_Unlocked(const char* func, u32 line, const char* txt, ...) {
	va_list args;
	
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
}

void Log(const char* func, u32 line, const char* txt, ...) {
	Thread_Lock();
	va_list args;
	
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
	Thread_Unlock();
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

f32 Math_SplineFloat(f32 u, f32* res, f32* point0, f32* point1, f32* point2, f32* point3) {
	f32 coeff[4];
	
	coeff[0] = (1.0f - u) * (1.0f - u) * (1.0f - u) / 6.0f;
	coeff[1] = u * u * u / 2.0f - u * u + 2.0f / 3.0f;
	coeff[2] = -u * u * u / 2.0f + u * u / 2.0f + u / 2.0f + 1.0f / 6.0f;
	coeff[3] = u * u * u / 6.0f;
	
	if (res) {
		*res = (coeff[0] * *point0) + (coeff[1] * *point1) + (coeff[2] * *point2) + (coeff[3] * *point3);
		
		return *res;
	}
	
	return (coeff[0] * *point0) + (coeff[1] * *point1) + (coeff[2] * *point2) + (coeff[3] * *point3);
}

// # # # # # # # # # # # # # # # # # # # #
// # SOUND                               #
// # # # # # # # # # # # # # # # # # # # #

#include "miniaudio.h"
#include "xm.h"

static struct {
	SoundCallback    callback;
	ma_device_config deviceConfig;
	ma_device device;
} sSoundCtx;

static void __Sound_Callback(ma_device* dev, void* output, const void* input, ma_uint32 frameCount) {
	sSoundCtx.callback(dev->pUserData, output, frameCount);
}

void Sound_Init(SoundFormat fmt, u32 sampleRate, u32 channelNum, SoundCallback callback, void* uCtx) {
	sSoundCtx.deviceConfig = ma_device_config_init(ma_device_type_playback);
	
	sSoundCtx.deviceConfig.playback.format = (ma_format)fmt;
	sSoundCtx.deviceConfig.playback.channels = channelNum;
	sSoundCtx.deviceConfig.sampleRate = sampleRate;
	sSoundCtx.deviceConfig.dataCallback = __Sound_Callback;
	sSoundCtx.deviceConfig.pUserData = uCtx;
	sSoundCtx.deviceConfig.periodSizeInFrames = 128;
	sSoundCtx.callback = callback;
	
	ma_device_init(NULL, &sSoundCtx.deviceConfig, &sSoundCtx.device);
	ma_device_start(&sSoundCtx.device);
}

void Sound_Free() {
	ma_device_stop(&sSoundCtx.device);
	ma_device_uninit(&sSoundCtx.device);
}

static void __Sound_Xm_Play(xm_context_t* xm, void* output, u32 frameCount) {
	xm_generate_samples(xm, output, frameCount);
}

void Sound_Xm_Play(const void* data, u32 size) {
	xm_context_t* xm;
	
	if (xm_create_context_safe(&xm, data, size, 48000)) printf_error("Could not initialize XmPlayer");
	xm_set_max_loop_count(xm, 0);
	
	Sound_Init(SOUND_F32, 48000, 2, (void*)__Sound_Xm_Play, xm);
}

void Sound_Xm_Stop() {
	Sound_Free();
}

// # # # # # # # # # # # # # # # # # # # #
// # SHA                                 #
// # # # # # # # # # # # # # # # # # # # #

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
        Calculate the sha256 digest of some data
        Author: Vitor Henrique Andrade Helfensteller Straggiotti Silva
        Date: 26/06/2021 (DD/MM/YYYY)
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#define DEBUG_FLAG      0
#define BLOCK_SIZE_BYTE 64

static uint32_t sigma1(uint32_t x) {
	uint32_t RotateRight17, RotateRight19, ShiftRight10;
	
	RotateRight17 = (x >> 17) | (x << 15);
	RotateRight19 = (x >> 19) | (x << 13);
	ShiftRight10 = x >> 10;
	
	return RotateRight17 ^ RotateRight19 ^ ShiftRight10;
}

static uint32_t sigma0(uint32_t x) {
	uint32_t RotateRight7, RotateRight18, ShiftRight3;
	
	RotateRight7 = (x >> 7) | (x << 25);
	RotateRight18 = (x >> 18) | (x << 14);
	ShiftRight3 = x >> 3;
	
	return RotateRight7 ^ RotateRight18 ^ ShiftRight3;
}

static uint32_t choice(uint32_t x, uint32_t y, uint32_t z) {
	return (x & y) ^ ((~x) & z);
}

static uint32_t BigSigma1(uint32_t x) {
	uint32_t RotateRight6, RotateRight11, RotateRight25;
	
	RotateRight6 = (x >> 6) | (x << 26);
	RotateRight11 = (x >> 11) | (x << 21);
	RotateRight25 = (x >> 25) | (x << 7);
	
	return RotateRight6 ^ RotateRight11 ^ RotateRight25;
}

static uint32_t BigSigma0(uint32_t x) {
	uint32_t RotateRight2, RotateRight13, RotateRight22;
	
	RotateRight2 = (x >> 2) | (x << 30);
	RotateRight13 = (x >> 13) | (x << 19);
	RotateRight22 = (x >> 22) | (x << 10);
	
	return RotateRight2 ^ RotateRight13 ^ RotateRight22;
}

static uint32_t major(uint32_t x, uint32_t y, uint32_t z) {
	return (x & y) ^ (x & z) ^ (y & z);
}

static uint8_t create_schedule_array_data(uint8_t* Data, uint64_t DataSizeByte, uint64_t* RemainingDataSizeByte, uint32_t* W) {
	//Checking for file/data size limit
	if ((0xFFFFFFFFFFFFFFFF / 8) < DataSizeByte) {
		printf("Error! File/Data exceeds size limit of 20097152 TiB");
		exit(EXIT_FAILURE);
	}
	
	//Starting with all data + 1 ending byte + 8 size byte
	uint8_t TmpBlock[64];
	uint8_t IsFinishedFlag = 0;
	static uint8_t SetEndOnNextBlockFlag = 0;
	
	//Clear schedule array before use
	for (uint8_t i = 0; i < 64; i++) {
		W[i] = 0x0;
		TmpBlock[i] = 0x0; //Necessary for 0 padding on last block
	}
	
	//Creating 512 bits (64 bytes, 16 uint32_t) block with ending byte, padding
	// and data size
	for (uint8_t i = 0; i < 64; i++) {
		if (*RemainingDataSizeByte > 0) {
			TmpBlock[i] = Data[DataSizeByte - *RemainingDataSizeByte];
			*RemainingDataSizeByte = *RemainingDataSizeByte - 1;
			
			if (*RemainingDataSizeByte == 0) { //Data ends before the end of the block
				if (i < 63) {
					i++;
					TmpBlock[i] = 0x80;
					if (i < 56) {
						//64 bits data size in bits with big endian representation
						uint64_t DataSizeBits = DataSizeByte * 8;
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
					} else //Block canot hold 64 bits data size value
						goto outside1;
				} else {       //Last element of data is the last element on block
					SetEndOnNextBlockFlag = 1;
				}
			}
		} else {
			if ((SetEndOnNextBlockFlag == 1) && (i == 0)) {
				TmpBlock[i] = 0x80;
				SetEndOnNextBlockFlag = 0;
			}
			uint64_t DataSizeBits = DataSizeByte * 8;
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
	
	//Filling the schedule array
	for (uint8_t i = 0; i < 64; i += 4) {
		W[i / 4] = (((uint32_t)TmpBlock[i]) << 24) |
			(((uint32_t)TmpBlock[i + 1]) << 16) |
			(((uint32_t)TmpBlock[i + 2]) << 8) |
			((uint32_t)TmpBlock[i + 3]);
	}
	
	if (IsFinishedFlag == 1)
		return 0;
	else
		return 1;
}

static void complete_schedule_array(uint32_t* W) {
	//add more 48 words of 32bit [w16 to w63]
	for (uint8_t i = 16; i < 64; i++) {
		W[i] = sigma1(W[i - 2]) + W[i - 7] + sigma0(W[i - 15]) + W[i - 16];
	}
}

static void compression(uint32_t* Hash, uint32_t* W) {
	enum TmpH {a, b, c, d, e, f, g, h};
	//create round constants (K)
	const uint32_t K_const[64] = { 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
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
				       0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };
	
	uint32_t TmpHash[8] = { 0 };
	uint32_t Temp1 = 0, Temp2 = 0;
	
	//inicialize variables a, b, c, d, e, f, g, h to h[0::7] respectively
	TmpHash[a] = Hash[0];
	TmpHash[b] = Hash[1];
	TmpHash[c] = Hash[2];
	TmpHash[d] = Hash[3];
	TmpHash[e] = Hash[4];
	TmpHash[f] = Hash[5];
	TmpHash[g] = Hash[6];
	TmpHash[h] = Hash[7];
	
	//Compression of the message schedule (W[0::63]) -----------------------
	for (uint32_t i = 0; i < 64; i++) {
		Temp1 = BigSigma1(TmpHash[e]) + choice(TmpHash[e], TmpHash[f], TmpHash[g]) +
			K_const[i] + W[i] + TmpHash[h];
		Temp2 = BigSigma0(TmpHash[a]) + major(TmpHash[a], TmpHash[b], TmpHash[c]);
		
		TmpHash[h] = TmpHash[g];
		TmpHash[g] = TmpHash[f];
		TmpHash[f] = TmpHash[e];
		TmpHash[e] = TmpHash[d] + Temp1;
		TmpHash[d] = TmpHash[c];
		TmpHash[c] = TmpHash[b];
		TmpHash[b] = TmpHash[a];
		TmpHash[a] = Temp1 + Temp2;
	}
	//Update hash values for current block
	Hash[0] += TmpHash[a];
	Hash[1] += TmpHash[b];
	Hash[2] += TmpHash[c];
	Hash[3] += TmpHash[d];
	Hash[4] += TmpHash[e];
	Hash[5] += TmpHash[f];
	Hash[6] += TmpHash[g];
	Hash[7] += TmpHash[h];
}

static uint8_t*extract_digest(uint32_t* Hash) {
	uint8_t* Digest;
	
	//Allocate memory for digest pointer
	Digest = (uint8_t*)malloc(32 * sizeof(uint8_t));
	
	//Prepare digest for return
	for (uint32_t i = 0; i < 32; i += 4) {
		Digest[i] = (uint8_t)((Hash[i / 4] >> 24) & 0x000000FF);
		Digest[i + 1] = (uint8_t)((Hash[i / 4] >> 16) & 0x000000FF);
		Digest[i + 2] = (uint8_t)((Hash[i / 4] >> 8) & 0x000000FF);
		Digest[i + 3] = (uint8_t)(Hash[i / 4] & 0x000000FF);
	}
	
	return Digest;
}

u8* Sys_Sha256(u8* data, u64 size) {
	//schedule array
	uint32_t W[64];
	
	//H -> Block hash ; TmpH -> temporary hash in compression loop
	//Temp1 and Temp2 are auxiliar variable to calculate TmpH[]
	uint32_t Hash[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
			     0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
	
	//Hashed data
	uint8_t* Digest;
	
	uint64_t RemainingDataSizeByte = size;
	
	while (create_schedule_array_data(data, size, &RemainingDataSizeByte, W) == 1) {
		complete_schedule_array(W);
		compression(Hash, W);
	}
	complete_schedule_array(W);
	compression(Hash, W);
	
	Digest = extract_digest(Hash);
	
	return Digest;
}
