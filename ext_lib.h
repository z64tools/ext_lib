#ifndef EXT_LIB_H
#define EXT_LIB_H

#define THIS_EXTLIB_VERSION 210

/**
 * Do not get impression that this library is clang compatible.
 * Far from it.
 *
 * I just use clang IDE, that's all.
 */
#ifdef __clang__
	#ifdef _WIN32
		#undef _WIN32
	#endif
#endif

#ifndef EXTLIB_PERMISSIVE
	#ifndef EXTLIB
		#error EXTLIB not defined
	#else
		#if EXTLIB > THIS_EXTLIB_VERSION
			#error ExtLib copy is older than the project its used with
		#endif
	#endif
#endif

#define _GNU_SOURCE
#ifndef __CRT__NO_INLINE
	#define __CRT__NO_INLINE
#endif

#include "ext_type.h"
#include "ext_macros.h"
#include "ext_math.h"
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <float.h>

extern PrintfSuppressLevel gPrintfSuppress;
extern u8 gPrintfProgressing;

void* Object_Method(Object* self, void* target, int argCount);
void Object_Prepare(Object* self);
void* Object_Create(size_t size, int funcCount);

void* qFree(const void* ptr);
void* PostFree_Queue(void* ptr);
void* PostFree_QueueCallback(void* callback, void* ptr);
void PostFree_Free(void);

extern const char* gThdPool_ProgressMessage;
void ThdPool_Add(void* function, void* arg, u32 n, ...);
void ThdPool_Exec(u32 max, bool mutex);
#define ThdPool_Add(function, arg, ...) ThdPool_Add(function, arg, NARGS(__VA_ARGS__), __VA_ARGS__)

void SetSegment(const u8 id, void* segment);
void* SegmentedToVirtual(const u8 id, void32 ptr);
void32 VirtualToSegmented(const u8 id, void* ptr);

// Temporary variable alloc, won't last forever
void* xAlloc(Size size);
char* xStrDup(const char* str);
char* xStrNDup(const char* s, size_t n);
char* xMemDup(const char* data, Size size);
char* xFmt(const char* fmt, ...);
char* xRep(const char* str, const char* a, const char* b);

void Time_Start(u32 slot);
f64 Time_Get(u32 slot);
void Profiler_I(u8 s);
void Profiler_O(u8 s);
f32 Profiler_Time(u8 s);

void FileSys_MakePath(s32 flag);
void FileSys_Path(const char* fmt, ...);
char* FileSys_File(const char* str, ...);
char* FileSys_FindFile(const char* str);
void FileSys_Free();

bool Sys_IsDir(const char* path);
void Sys_MakeDir(const char* dir, ...);
Time Sys_Stat(const char* item);
Time Sys_StatF(const char* item, StatFlag flag);
char* Sys_ThisApp(void);
Time Sys_Time(void);
s32 Sys_Rename(const char* input, const char* output);
s32 Sys_Delete(const char* item);
s32 Sys_Delete_Recursive(const char* item);
const char* Sys_WorkDir(void);
const char* Sys_AppDir(void);
void Sys_SetWorkDir(const char* txt);
#define cliprintf(dest, tool, args, ...) sprintf(dest, "%s " args, tool, __VA_ARGS__)
void Sys_TerminalSize(s32* r);
s32 Sys_Touch(const char* file);
s32 Sys_Copy(const char* src, const char* dest);
u8* Sys_Sha256(u8* data, u64 size);
void Sys_Sleep(f64 sec);
Date Sys_Date(Time time);
s32 Sys_GetCoreCount(void);
Size Sys_GetFileSize(const char* file);

void SysExe_IgnoreError();
s32 SysExe_GetError();
s32 SysExe(const char* cmd);
char* SysExeO(const char* cmd);
s32 SysExeC(const char* cmd, s32 (*callback)(void*, const char*), void* arg);

s32 Terminal_YesOrNo(void);
void Terminal_ClearScreen(void);
void Terminal_ClearLines(u32 i);
void Terminal_Move_PrevLine(void);
const char* Terminal_GetStr(void);
char Terminal_GetChar();

extern ItemList gList_SortError;

void ItemList_Validate(ItemList* itemList);
ItemList ItemList_Initialize(void);
void ItemList_SetFilter(ItemList* list, u32 filterNum, ...);
void ItemList_FreeFilters(ItemList* list);
void ItemList_List(ItemList* target, const char* path, s32 depth, ListFlag flags);
char* ItemList_GetWildItem(ItemList* list, const char* end, const char* error, ...);
void ItemList_Separated(ItemList* list, const char* str, const char separator);
void ItemList_Print(ItemList* target);
Time ItemList_StatMax(ItemList* list);
Time ItemList_StatMin(ItemList* list);
s32 ItemList_SaveList(ItemList* target, const char* output);
void ItemList_NumericalSort(ItemList* list);
s32 ItemList_NumericalSlotSort(ItemList* list, bool checkOverlaps);
void ItemList_Free(ItemList* itemList);
void ItemList_Alloc(ItemList* list, u32 num, Size size);
void ItemList_AddItem(ItemList* list, const char* item);
void ItemList_Combine(ItemList* out, ItemList* a, ItemList* b);

#define ItemList_GetWildItem(list, ...) ItemList_GetWildItem(list, __VA_ARGS__, NULL)
#define ItemList_SetFilter(list, ...)   ItemList_SetFilter(list, NARGS(__VA_ARGS__), __VA_ARGS__)

#define Assert(expression) do {										   \
		if (!(expression)) {										   \
			fprintf(												   \
				stdout,												   \
				"ASSERT: " PRNT_GRAY "[" PRNT_REDD "%s" PRNT_GRAY "]:" \
				"[" PRNT_YELW "%s" PRNT_GRAY "]:"					   \
				"[" PRNT_BLUE "%d" PRNT_GRAY "]",					   \
				#expression, __FUNCTION__, __LINE__					   \
			); }													   \
} while (0)

f32 RandF();
void* MemMem(const void* haystack, Size haystacklen, const void* needle, Size needlelen);
void* StrStr(const char* haystack, const char* needle);
void* StrStrWhole(const char* haystack, const char* needle);
void* StrStrCase(const char* haystack, const char* needle);
void* MemStrCase(const char* haystack, u32 haystacklen, const char* needle);
void* MemMemAlign(u32 val, const void* haystack, Size haystacklen, const void* needle, Size needlelen);
char* StrEnd(const char* src, const char* ext);
char* StrEndCase(const char* src, const char* ext);
char* StrStart(const char* src, const char* ext);
char* StrStartCase(const char* src, const char* ext);
void ByteSwap(void* src, s32 size);
__attribute__ ((warn_unused_result))
void* Alloc(s32 size);
__attribute__ ((warn_unused_result))
void* Calloc(s32 size);
__attribute__ ((warn_unused_result))
void* Realloc(const void* data, s32 size);
void* Free(const void* data);
#define Free(data) ({ data = Free(data); })
void* MemDup(const void* src, Size size);
char* StrDup(const char* src);
char* StrDupX(const char* src, Size size);
char* StrDupClp(const char* str, u32 max);
char* Fmt(const char* fmt, ...);
s32 ParseArgs(char* argv[], char* arg, u32* parArg);
void SlashAndPoint(const char* src, s32* slash, s32* point);
char* Path(const char* src);
char* PathSlot(const char* src, s32 num);
char* StrChrAcpt(const char* str, char* c);
char* Basename(const char* src);
char* Filename(const char* src);
char* LineHead(const char* str, const char* head);
char* Line(const char* str, s32 line);
char* Word(const char* str, s32 word);
Size LineLen(const char* str);
Size WordLen(const char* str);
char* FileExtension(const char* str);
void CaseToLow(char* s, s32 i);
void CaseToUp(char* s, s32 i);
s32 LineNum(const char* str);
s32 PathNum(const char* src);
char* CopyLine(const char* str, s32 line);
char* CopyWord(const char* str, s32 word);
char* PathRel_From(const char* from, const char* item);
char* PathAbs_From(const char* from, const char* item);
char* PathRel(const char* item);
char* PathAbs(const char* item);
s32 PathIsAbs(const char* item);
s32 PathIsRel(const char* item);

void Color_ToHSL(HSL8* hsl, RGB8* rgb);
void Color_ToRGB(RGB8* rgb, HSL8* hsl);

void MemFile_Validate(MemFile* mem);
MemFile MemFile_Initialize();
void MemFile_Params(MemFile* memFile, ...);
void MemFile_Alloc(MemFile* memFile, u32 size);
void MemFile_Realloc(MemFile* memFile, u32 size);
void MemFile_Rewind(MemFile* memFile);
s32 MemFile_Write(MemFile* dest, void* src, u32 size);
s32 MemFile_Insert(MemFile* mem, void* src, u32 size, s64 pos);
s32 MemFile_Append(MemFile* dest, MemFile* src);
void MemFile_Align(MemFile* src, u32 align);
s32 MemFile_Printf(MemFile* dest, const char* fmt, ...);
s32 MemFile_Read(MemFile* src, void* dest, Size size);
void* MemFile_Seek(MemFile* src, u32 seek);
void MemFile_LoadMem(MemFile* mem, void* data, Size size);
s32 MemFile_LoadFile(MemFile* memFile, const char* filepath);
s32 MemFile_LoadFile_String(MemFile* memFile, const char* filepath);
s32 MemFile_SaveFile(MemFile* memFile, const char* filepath);
s32 MemFile_SaveFile_String(MemFile* memFile, const char* filepath);
void MemFile_Free(MemFile* memFile);
void MemFile_Reset(MemFile* memFile);
void MemFile_Clear(MemFile* memFile);

#define MemFile_Alloc(this, size) do {				 \
		Log("MemFile_Alloc(%s, %s);", #this, #size); \
		MemFile_Alloc(this, size);					 \
} while (0)

#define MEMFILE_SEEK_END 0xFFFFFFFF

u32 Value_Hex(const char* string);
s32 Value_Int(const char* string);
f32 Value_Float(const char* string);
s32 Value_Bool(const char* string);
s32 Value_ValidateHex(const char* str);
s32 Value_ValidateInt(const char* str);
s32 Value_ValidateFloat(const char* str);
ValueType Value_Type(const char* variable);
s32 Digits_Int(s32 i);
s32 Digits_Hex(s32 i);

s32 Music_NoteIndex(const char* note);
const char* Music_NoteWord(s32 note);

void StrIns(char* point, const char* insert);
void StrIns2(char* origin, const char* insert, s32 pos, s32 size);
void StrRem(char* point, s32 amount);
s32 StrRep(char* src, const char* word, const char* replacement);
s32 StrRepWhole(char* src, const char* word, const char* replacement);
char* StrUnq(const char* str);
s32 StrComLen(const char* a, const char* b);
char* StrSlash(char* str);
char* StrStripIllegalChar(char* str);
void String_SwapExtension(char* dest, char* src, const char* ext);
char* String_GetSpacedArg(char* argv[], s32 cur);
char* StrUpper(char* str);
char* StrLower(char* str);
bool ChrPool(const char c, const char* pool);
bool StrPool(const char* s, const char* pool);

char* StrU8(char* dst, const wchar* src);
wchar* StrU16(wchar* dst, const char* src);
Size strwlen(const wchar* s);

char* Config_Variable(const char* str, const char* name);
char* Config_GetVariable(const char* str, const char* name);
void Config_ProcessIncludes(MemFile* mem);

s32 Config_GetErrorState(void);
void Config_GetArray(MemFile* mem, const char* name, ItemList* list);
s32 Config_GetBool(MemFile* mem, const char* boolName);
s32 Config_GetOption(MemFile* mem, const char* stringName, char* strList[]);
s32 Config_GetInt(MemFile* mem, const char* intName);
char* Config_GetStr(MemFile* mem, const char* stringName);
f32 Config_GetFloat(MemFile* mem, const char* floatName);
void Config_GotoSection(const char* section);
void Config_ListVariables(MemFile* mem, ItemList* list, const char* section);
void Config_ListSections(MemFile* cfg, ItemList* list);

s32 Config_ReplaceVariable(MemFile* mem, const char* variable, const char* fmt, ...);
void Config_WriteComment(MemFile* mem, const char* comment);
void Config_WriteArray(MemFile* mem, const char* variable, ItemList* list, bool quote, const char* comment);
void Config_WriteInt(MemFile* mem, const char* variable, const s64 integer, const char* comment);
void Config_WriteHex(MemFile* mem, const char* variable, const s64 integer, const char* comment);
void Config_WriteStr(MemFile* mem, const char* variable, const char* str, bool quote, const char* comment);
void Config_WriteFloat(MemFile* mem, const char* variable, const f64 flo, const char* comment);
void Config_WriteBool(MemFile* mem, const char* variable, const s32 val, const char* comment);
void Config_WriteSection(MemFile* mem, const char* variable, const char* comment);
#define Config_Print(mem, ...) MemFile_Printf(mem, __VA_ARGS__)
#define NO_COMMENT NULL
#define QUOTES     1
#define NO_QUOTES  0

char* String_Tsv(char* str, s32 rowNum, s32 lineNum);

void Log_NoOutput(void);
void Log_Init();
void Log_Free();
void Log_Print();
void __Log(const char* func, u32 line, const char* txt, ...);
#define Log(...) __Log(__FUNCTION__, __LINE__, __VA_ARGS__)

void printf_SetSuppressLevel(PrintfSuppressLevel lvl);
void __attribute__ ((deprecated)) printf_SetPrefix(char* fmt);
void printf_toolinfo(const char* toolname, const char* fmt, ...);
void printf_warning(const char* fmt, ...);
void printf_warning_align(const char* info, const char* fmt, ...);
void printf_error(const char* fmt, ...);
void printf_error_align(const char* info, const char* fmt, ...);
void printf_info(const char* fmt, ...);
void printf_info_align(const char* info, const char* fmt, ...);
void printf_prog_align(const char* info, const char* fmt, const char* color);
void printf_progressFst(const char* info, u32 a, u32 b);
void printf_progress(const char* info, u32 a, u32 b);
void printf_getchar(const char* txt);
void printf_lock(const char* fmt, ...);
void printf_WinFix(void);
void printf_hex(const char* txt, const void* data, u32 size, u32 dispOffset);
void printf_bit(const char* txt, const void* data, u32 size, u32 dispOffset);
void printf_nl(void);

f32 Math_SmoothStepToF(f32* pValue, f32 target, f32 fraction, f32 step, f32 minStep);
f32 Math_Spline_Audio(float k, float xm1, float x0, float x1, float x2);
f32 Math_Spline(f32 k, f32 xm1, f32 x0, f32 x1, f32 x2);
void Math_ApproachF(f32* pValue, f32 target, f32 fraction, f32 step);
void Math_ApproachS(s16* pValue, s16 target, s16 scale, s16 step);

void* Sound_Init(SoundFormat fmt, u32 sampleRate, u32 channelNum, SoundCallback callback, void* uCtx);
void Sound_Free(void* sound);
void Sound_Xm_Play(const void* data, u32 size);
void Sound_Xm_Stop();

char* Regex(const char* str, const char* pattern, RegexFlag flag);

#ifndef _WIN32
	#ifndef __clang__
		#define stricmp(a, b)        strcasecmp(a, b)
		#define strnicmp(a, b, size) strncasecmp(a, b, size)
	#endif
#endif

int stricmp(const char* a, const char* b);
int strnicmp(const char* a, const char* b, Size size);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#ifdef _WIN32
	static char* strndup(const char* s, size_t n) {
		size_t len = strlen(s);
		
		len = len > n ? n : len;
		char* new = (char*) malloc (len + 1);
		if (new == NULL)
			return NULL;
		new[len] = '\0';
		
		return (char*) memcpy (new, s, len);
	}
#endif

extern vbool gThreadMode;
extern Mutex gThreadMutex;

static inline void Mutex_Enable(void) {
	if (!gThreadMode)
		pthread_mutex_init(&gThreadMutex, NULL);
	gThreadMode = true;
	
}

static inline void Mutex_Disable(void) {
	if (gThreadMode)
		pthread_mutex_destroy(&gThreadMutex);
	gThreadMode = false;
	
}

static inline void Mutex_Lock(void) {
	if (gThreadMode)
		pthread_mutex_lock(&gThreadMutex);
}

static inline void Mutex_Unlock(void) {
	if (gThreadMode)
		pthread_mutex_unlock(&gThreadMutex);
}

static inline void Thread_Create(Thread* thread, void* func, void* arg) {
	pthread_create(thread, NULL, (void*)func, (void*)(arg));
}

static inline s32 Thread_Join(Thread* thread) {
	u32 r = pthread_join(*thread, NULL);
	
	memset(thread, 0, sizeof(Thread));
	
	return r;
}

#pragma GCC diagnostic pop

#endif /* EXT_LIB_H */
