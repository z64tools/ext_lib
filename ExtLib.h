#ifndef __EXTLIB_H__
#define __EXTLIB_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

// __attribute__((scalar_storage_order("big-endian")))

typedef signed char s8;
typedef unsigned char u8;
typedef signed short int s16;
typedef unsigned short int u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long int s64;
typedef unsigned long long int u64;
typedef float f32;
typedef double f64;
typedef uintptr_t uPtr;
typedef intptr_t sPtr;
typedef u32 void32;
typedef time_t Time;
typedef pthread_t Thread;

extern pthread_mutex_t gMutexLock;

#ifdef __IDE_FLAG__
#define Nested(name, args) (^ name) args = ^ args
#define NestedVar(vars)    vars
#else
#define Nested(name, args) name args
#define NestedVar(vars)    while (0) { (void)0; }
#endif

typedef struct {
	f32 h;
	f32 s;
	f32 l;
} HSL8;

typedef struct {
	f32 h;
	f32 s;
	f32 l;
	union {
		u8 alpha;
		u8 a;
	};
} HSLA8;

typedef struct {
	union {
		struct {
			u8 r;
			u8 g;
			u8 b;
		};
		u8 c[3];
	};
} RGB8;

typedef struct {
	union {
		struct {
			u8 r;
			u8 g;
			u8 b;
			u8 a;
		};
		u8 c[4];
	};
} RGBA8;

typedef struct {
	f32 r;
	f32 g;
	f32 b;
} RGB32;

typedef struct {
	f32 r;
	f32 g;
	f32 b;
	f32 a;
} RGBA32;

typedef enum {
	PSL_DEBUG = -1,
	PSL_NONE,
	PSL_NO_INFO,
	PSL_NO_WARNING,
	PSL_NO_ERROR,
} PrintfSuppressLevel;

typedef enum {
	SWAP_U8  = 1,
	SWAP_U16 = 2,
	SWAP_U32 = 4,
	SWAP_U64 = 8,
	SWAP_F80 = 10
} SwapSize;

typedef union {
	void* p;
	u8*   u8;
	u16*  u16;
	u32*  u32;
	u64*  u64;
	s8*   s8;
	s16*  s16;
	s32*  s32;
	s64*  s64;
	f32*  f32;
	f64*  f64;
} PointerCast;

typedef struct Node {
	struct Node* prev;
	struct Node* next;
} Node;

typedef enum {
	MEM_FILENAME = 1 << 16,
	MEM_CRC32    = 1 << 17,
	MEM_ALIGN    = 1 << 18,
	MEM_REALLOC  = 1 << 19,
	
	MEM_CLEAR    = 1 << 30,
	MEM_END      = 1 << 31,
} MemInit;

typedef struct MemFile {
	union {
		void* data;
		PointerCast cast;
		char* str;
	};
	u32 memSize;
	u32 dataSize;
	u32 seekPoint;
	struct {
		Time age;
		char name[512];
		u32  crc32;
	} info;
	struct {
		u32 align   : 8;
		u32 realloc : 1;
		u32 getCrc  : 1;
		u32 getName : 1;
		u64 initKey;
	} param;
} MemFile;

typedef struct ItemList {
	char*  buffer;
	u32    writePoint;
	char** item;
	u32    num;
	struct {
		u64 initKey;
	} __private;
} ItemList;

typedef enum {
	DIR__MAKE_ON_ENTER = (1) << 0,
} DirParam;

typedef struct {
	char curPath[2048];
	s32  enterCount[512];
	s32  pos;
	DirParam param;
} DirCtx;

typedef void (* SoundCallback)(void*, void*, u32);

typedef enum {
	SOUND_S16 = 2,
	SOUND_S24,
	SOUND_S32,
	SOUND_F32,
} SoundFormat;

extern u8 gPrintfProgressing;

void Thread_Init(void);
void Thread_Free(void);
void Thread_Lock(void);
void Thread_Unlock(void);
void Thread_Create(Thread* thread, void* func, void* arg);
s32 Thread_Join(Thread* thread);

void SetSegment(const u8 id, void* segment);
void* SegmentedToVirtual(const u8 id, void32 ptr);
void32 VirtualToSegmented(const u8 id, void* ptr);

void* Tmp_Alloc(u32 size);
char* Tmp_String(const char* str);
char* Tmp_Printf(const char* fmt, ...);

void Time_Start(void);
f32 Time_Get(void);

typedef enum {
	SORT_NO        = 0,
	SORT_NUMERICAL = 1,
	IS_FILE        = 0,
	IS_DIR         = 1,
} DirEnum;

typedef enum {
	PATH_RELATIVE,
	PATH_ABSOLUTE
} PathType;

void Dir_SetParam(DirCtx* ctx, DirParam w);
void Dir_UnsetParam(DirCtx* ctx, DirParam w);
void Dir_Set(DirCtx* ctx, char* path, ...);
void Dir_Enter(DirCtx* ctx, char* fmt, ...);
void Dir_Leave(DirCtx* ctx);
void Dir_Make(DirCtx* ctx, char* dir, ...);
void Dir_MakeCurrent(DirCtx* ctx);
char* Dir_Current(DirCtx* ctx);
char* Dir_File(DirCtx* ctx, char* fmt, ...);
Time Dir_Stat(DirCtx* ctx, const char* item);
char* Dir_GetWildcard(DirCtx* ctx, char* x);
void Dir_ItemList(DirCtx* ctx, ItemList* itemList, bool isPath);
void Dir_ItemList_Recursive(DirCtx* ctx, ItemList* target, char* keyword);
void Dir_ItemList_Not(DirCtx* ctx, ItemList* itemList, bool isPath, char* not);
void Dir_ItemList_Keyword(DirCtx* ctx, ItemList* itemList, char* ext);

typedef enum {
	STAT_ACCS = (1) << 0,
	STAT_MODF = (1) << 1,
	STAT_CREA = (1) << 2,
} StatFlag;

bool Sys_IsDir(const char* path);
void Sys_MakeDir(const char* dir, ...);
Time Sys_Stat(const char* item);
Time Sys_StatF(const char* item, StatFlag flag);
Time Sys_StatSelf(void);
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
s32 Sys_Copy(const char* src, const char* dest, bool isStr);
u8* Sys_Sha256(u8* data, u64 size);
void Sys_Sleep(f64 sec);

s32 SysExe(const char* cmd);
char* SysExeO(const char* cmd);

typedef enum {
	CLR_WHITE = 1,
	CLR_RED,
	CLR_GREEN,
	CLR_YELLOW,
	CLR_BLUE,
	CLR_MAGENTA,
	CLR_CYAN,
	CLR_BLACK,
	
	ATTR_LGHT = (1) << 5,
	ATTR_BOLD = (1) << 6,
	ATTR_DIM  = (1) << 7,
} TerminalAttribute;

typedef struct {
	void (* move)(int, int);
	void (* print)(const char* fmt, ...);
	void (* clear)(void);
	void (* refresh)(void);
	void (* attribute)(TerminalAttribute);
} Terminal;

s32 Terminal_YesOrNo(void);
void Terminal_ClearScreen(void);
void Terminal_ClearLines(u32 i);
void Terminal_Move_PrevLine(void);
void Terminal_Move(s32 x, s32 y);
const char* Terminal_GetStr(void);
char Terminal_GetChar();
void Terminal_Window(s32 (*func) (Terminal*, void*, void*, int), void* pass, void* pass2);

typedef enum {
	LIST_FILES   = 0x0,
	LIST_FOLDERS = 0x1,
} ListFlags;

void ItemList_List(ItemList* target, const char* path, s32 depth, ListFlags flags);
void ItemList_SpacedStr(ItemList* list, const char* str);
void ItemList_CommaStr(ItemList* list, const char* str);
void ItemList_Print(ItemList* target);
Time ItemList_StatMax(ItemList* list);
s32 ItemList_Stat(ItemList* list);
s32 ItemList_SaveList(ItemList* target, const char* output);
void ItemList_NumericalSort(ItemList* list);
ItemList ItemList_Initialize(void);
void ItemList_Free(ItemList* itemList);

void printf_SetSuppressLevel(PrintfSuppressLevel lvl);
void printf_SetPrefix(char* fmt);
void printf_SetPrintfTypes(const char* d, const char* w, const char* e, const char* i);
void printf_toolinfo(const char* toolname, const char* fmt, ...);
void printf_debug(const char* fmt, ...);
void printf_debug_align(const char* info, const char* fmt, ...);
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
void printf_WinFix(void);
void printf_clearMessages(void);

f32 RandF();
void* MemMem(const void* haystack, size_t haystackSize, const void* needle, size_t needleSize);
void* MemMemCase(const void* haystack, size_t haystackSize, const void* needle, size_t needleSize);
void* MemMemAlign(u32 val, const void* haystack, size_t haySize, const void* needle, size_t needleSize);
void* MemMemU16(void* haystack, size_t haySize, const void* needle, size_t needleSize);
void* MemMemU32(void* haystack, size_t haySize, const void* needle, size_t needleSize);
void* MemMemU64(void* haystack, size_t haySize, const void* needle, size_t needleSize);
char* StrEnd(const char* src, const char* ext);
char* StrEndCase(const char* src, const char* ext);
void* Malloc(void* data, s32 size);
void* Calloc(void* data, s32 size);
void* Realloc(void* data, s32 size);
void* Free(void* data);
void ByteSwap(void* src, s32 size);
s32 ParseArgs(char* argv[], char* arg, u32* parArg);
u32 Crc32(u8* s, u32 n);

void Color_ToHSL(HSL8* hsl, RGB8* rgb);
void Color_ToRGB(RGB8* rgb, HSL8* hsl);

MemFile MemFile_Initialize();
void MemFile_Params(MemFile* memFile, ...);
void MemFile_Malloc(MemFile* memFile, u32 size);
void MemFile_Realloc(MemFile* memFile, u32 size);
void MemFile_Rewind(MemFile* memFile);
s32 MemFile_Write(MemFile* dest, void* src, u32 size);
s32 MemFile_Append(MemFile* dest, MemFile* src);
void MemFile_Align(MemFile* src, u32 align);
s32 MemFile_Printf(MemFile* dest, const char* fmt, ...);
s32 MemFile_Read(MemFile* src, void* dest, u32 size);
void* MemFile_Seek(MemFile* src, u32 seek);
s32 MemFile_LoadFile(MemFile* memFile, const char* filepath);
s32 MemFile_LoadFile_String(MemFile* memFile, const char* filepath);
s32 MemFile_SaveFile(MemFile* memFile, const char* filepath);
s32 MemFile_SaveFile_String(MemFile* memFile, const char* filepath);
s32 MemFile_LoadFile_ReqExt(MemFile* memFile, const char* filepath, const char* ext);
s32 MemFile_SaveFile_ReqExt(MemFile* memFile, const char* filepath, s32 size, const char* ext);
void MemFile_Free(MemFile* memFile);
void MemFile_Reset(MemFile* memFile);
void MemFile_Clear(MemFile* memFile);

#define StrStr(src, comp)       MemMem(src, strlen(src), comp, strlen(comp))
#define StrStrNum(src, comp, n) MemMem(src, n, comp, n)
#define StrStrCase(src, comp)   MemMemCase(src, strlen(src), comp, strlen(comp))
#define StrMtch(a, b)           (!strncmp(a, b, strlen(b)))
#define catprintf(dest, ...)    sprintf(dest + strlen(dest), __VA_ARGS__)

u32 String_GetHexInt(const char* string);
s32 String_GetInt(const char* string);
f32 String_GetFloat(const char* string);
s32 String_GetLineCount(const char* str);
s32 String_CaseComp(char* a, char* b, u32 compSize);
char* String_Line(char* str, s32 line);
char* String_LineHead(char* str);
char* String_Word(char* str, s32 word);
char* String_Extension(const char* str);
char* String_GetLine(const char* str, s32 line);
char* String_GetWord(const char* str, s32 word);
void String_CaseToLow(char* s, s32 i);
void String_CaseToUp(char* s, s32 i);
char* String_GetPath(const char* src);
char* String_GetBasename(const char* src);
char* String_GetFilename(const char* src);
s32 String_GetPathNum(const char* src);
char* String_GetFolder(const char* src, s32 num);
void String_Insert(char* point, const char* insert);
void String_InsertExt(char* origin, const char* insert, s32 pos, s32 size);
void String_Remove(char* point, s32 amount);
s32 String_Replace(char* src, const char* word, const char* replacement);
void String_SwapExtension(char* dest, char* src, const char* ext);
char* String_GetSpacedArg(char* argv[], s32 cur);
s32 String_Validate_Hex(const char* str);
s32 String_Validate_Dec(const char* str);
s32 String_Validate_Float(const char* str);
char* String_Unquote(const char* str);

void Config_SuppressNext(void);
char* Config_Variable(const char* str, const char* name);
char* Config_GetVariable(const char* str, const char* name);
void Config_GetArray(ItemList* list, const char* str, const char* name);
s32 Config_GetBool(MemFile* memFile, const char* boolName);
s32 Config_GetOption(MemFile* memFile, const char* stringName, char* strList[]);
s32 Config_GetInt(MemFile* memFile, const char* intName);
char* Config_GetString(MemFile* memFile, const char* stringName);
f32 Config_GetFloat(MemFile* memFile, const char* floatName);
s32 Config_Replace(MemFile* mem, const char* variable, const char* fmt, ...);

void Log_Init();
void Log_Free();
void Log_Print();
void Log_Unlocked(const char* func, u32 line, const char* txt, ...);
void Log(const char* func, u32 line, const char* txt, ...);
#ifndef __EXTLIB_C__
#define Log(...) Log(__FUNCTION__, __LINE__, __VA_ARGS__)
#endif

f32 Math_SmoothStepToF(f32* pValue, f32 target, f32 fraction, f32 step, f32 minStep);
f32 Math_SplineFloat(f32 u, f32* res, f32* point0, f32* point1, f32* point2, f32* point3);
void Math_ApproachF(f32* pValue, f32 target, f32 fraction, f32 step);
void Math_ApproachS(s32* pValue, s32 target, s32 scale, s32 step);
s32 WrapS(s32 x, s32 min, s32 max);
f32 WrapF(f32 x, f32 min, f32 max);

void* Sound_Init(SoundFormat fmt, u32 sampleRate, u32 channelNum, SoundCallback callback, void* uCtx);
void Sound_Free(void* sound);
void Sound_Xm_Play(const void* data, u32 size);
void Sound_Xm_Stop();

extern PrintfSuppressLevel gPrintfSuppress;

#define PRNT_DGRY "\e[90;2m"
#define PRNT_DRED "\e[91;2m"
#define PRNT_GRAY "\e[0;90m"
#define PRNT_REDD "\e[0;91m"
#define PRNT_GREN "\e[0;92m"
#define PRNT_YELW "\e[0;93m"
#define PRNT_BLUE "\e[0;94m"
#define PRNT_PRPL "\e[0;95m"
#define PRNT_CYAN "\e[0;96m"
#define PRNT_RSET "\e[m"
#define PRNT_NL   "\n"
#define PRNT_RNL  PRNT_RSET PRNT_NL
#define PRNT_TODO "\e[91;2m" "TODO"

#define str2cmp(a, b) strncmp(a, b, strlen(b))

#define Node_Add(head, node) { \
		typeof(node) lastNode = head; \
		if (lastNode == NULL) { \
			head = node; \
		} else { \
			while (lastNode->next) { \
				lastNode = lastNode->next; \
			} \
			lastNode->next = node; \
			node->prev = lastNode; \
		} \
}

#define Node_Kill(head, node) { \
		typeof(node) kill = node; \
		if (node->next) { \
			node->next->prev = node->prev; \
		} \
		if (node->prev) { \
			node->prev->next = node->next; \
		} else { \
			head = node->next; \
		} \
		free(kill); \
		kill = NULL; \
}

#define Node_Remove(head, node) { \
		if (node->next) { \
			node->next->prev = node->prev; \
		} \
		if (node->prev) { \
			node->prev->next = node->next; \
		} else { \
			head = node->next; \
		} \
		node = NULL; \
}

#define Swap(a, b) { \
		typeof(a) y = a; \
		a = b; \
		b = y; \
}

// Checks endianess with tst & tstP
#define ReadBE(in) ({ \
		typeof(in) out; \
		s32 tst = 1; \
		u8* tstP = (u8*)&tst; \
		if (tstP[0] != 0) { \
			s32 size = sizeof(in); \
			if (size == 2) { \
				out = __builtin_bswap16(in); \
			} else if (size == 4) { \
				out = __builtin_bswap32(in); \
			} else if (size == 8) { \
				out = __builtin_bswap64(in); \
			} else { \
				out = in; \
			} \
		} else { \
			out = in; \
		} \
		out; \
	} \
)

#define WriteBE(dest, set) { \
		typeof(dest) get = set; \
		dest = ReadBE(get); \
}

#define SwapBE(in) WriteBE(in, in)

#define Decr(x) (x -= (x > 0) ? 1 : 0)
#define Incr(x) (x += (x < 0) ? 1 : 0)

#define Intersect(a, aend, b, bend) ((Max(a, b) < Min(aend, bend)))

#define Max(a, b)            ((a) > (b) ? (a) : (b))
#define Min(a, b)            ((a) < (b) ? (a) : (b))
#define MaxAbs(a, b)         (Abs(a) > Abs(b) ? (a) : (b))
#define MinAbs(a, b)         (Abs(a) <= Abs(b) ? (a) : (b))
#define Abs(val)             ((val) < 0 ? -(val) : (val))
#define Clamp(val, min, max) ((val) < (min) ? (min) : (val) > (max) ? (max) : (val))
#define ClampMin(val, min)   ((val) < (min) ? (min) : (val))
#define ClampMax(val, max)   ((val) > (max) ? (max) : (val))

#define ClampS8(val)  (s8)Clamp(((f32)val), -__INT8_MAX__ - 1, __INT8_MAX__)
#define ClampS16(val) (s16)Clamp(((f32)val), -__INT16_MAX__ - 1, __INT16_MAX__)
#define ClampS32(val) (s32)Clamp(((f32)val), (-__INT32_MAX__ - 1), (f32)__INT32_MAX__)

#define ArrayCount(arr)   (u32)(sizeof(arr) / sizeof(arr[0]))
#define Lerp(x, min, max) ((min) + ((max) - (min)) * (x))

#define BinToMb(x)        ((f32)(x) / (f32)0x100000)
#define BinToKb(x)        ((f32)(x) / (f32)0x400)
#define MbToBin(x)        (u32)(0x100000 * (x))
#define KbToBin(x)        (u32)(0x400 * (x))
#define Align(var, align) ((((var) % (align)) != 0) ? (var) + (align) - ((var) % (align)) : (var))

// #define strcpy(dst, src)   strcpy(dst, src)
// #define strcat(dst, src)  strcat(dst, src)
#define String_SMerge(dst, ...) sprintf(dst + strlen(dst), __VA_ARGS__);
#define String_Generate(string) strdup(string)

#define Config_WriteTitle(title) MemFile_Printf( \
		config, \
		title \
)

#define Config_WriteTitle_Str(title) MemFile_Printf( \
		config, \
		"# %s\n", \
		title \
)

#define Config_WriteVar(com1, name, defval, com2) MemFile_Printf( \
		config, \
		com1 \
		"%-15s = %-10s # %s\n\n", \
		name, \
		# defval, \
		com2 \
)

#define Config_WriteVar_Hex(name, defval) MemFile_Printf( \
		config, \
		"%-15s = 0x%X\n", \
		name, \
		defval \
)

#define Config_WriteVar_Int(name, defval) MemFile_Printf( \
		config, \
		"%-15s = %d\n", \
		name, \
		defval \
)

#define Config_WriteVar_Flo(name, defval) MemFile_Printf( \
		config, \
		"%-15s = %f\n", \
		name, \
		defval \
)

#define Config_WriteVar_Str(name, defval) MemFile_Printf( \
		config, \
		"%-15s = %s\n", \
		name, \
		defval \
)

#define Config_SPrintf(...) MemFile_Printf( \
		config, \
		__VA_ARGS__ \
)

#ifndef NDEBUG
#define printf_debugExt(...) if (gPrintfSuppress <= PSL_DEBUG) { \
		if (gPrintfProgressing) { printf("\n"); gPrintfProgressing = 0; } \
		printf(PRNT_PRPL "[X]: " PRNT_CYAN "%-16s " PRNT_REDD "%s" PRNT_GRAY ": " PRNT_YELW "%d" PRNT_RSET "\n", __FUNCTION__, __FILE__, __LINE__); \
		printf_debug(__VA_ARGS__); \
}

#define printf_debugExt_align(title, ...) if (gPrintfSuppress <= PSL_DEBUG) { \
		if (gPrintfProgressing) { printf("\n"); gPrintfProgressing = 0; } \
		printf(PRNT_PRPL "[X]: " PRNT_CYAN "%-16s " PRNT_REDD "%s" PRNT_GRAY ": " PRNT_YELW "%d" PRNT_RSET "\n", __FUNCTION__, __FILE__, __LINE__); \
		printf_debug_align(title, __VA_ARGS__); \
}

#define Assert(exp) if (!(exp)) { \
		if (gPrintfProgressing) { printf("\n"); gPrintfProgressing = 0; } \
		printf(PRNT_PRPL "[X]: " PRNT_CYAN "%-16s " PRNT_REDD "%s" PRNT_GRAY ": " PRNT_YELW "%d" PRNT_RSET "\n", __FUNCTION__, __FILE__, __LINE__); \
		printf_debug(PRNT_YELW "Assert(\a " PRNT_RSET # exp PRNT_YELW " );"); \
		exit(EXIT_FAILURE); \
}

#ifndef __EXTLIB_C__

#define Malloc(data, size) \
	Malloc(data, size); \
	Log("Malloc(%s); %.2f Kb", #data, BinToKb(size));

#define Realloc(data, size) \
	Realloc(data, size); \
	Log("Realloc(%s); %.2f Kb", #data, BinToKb(size));

#define Calloc(data, size) \
	Calloc(data, size); \
	Log("Calloc(%s); %.2f Kb", #data, BinToKb(size));

#define Free(data) \
	Free(data); \
	Log("Free(%s);", #data );

#endif

#else
#define printf_debugExt(...)       if (0) {}
#define printf_debugExt_align(...) if (0) {}
#define Assert(exp)                if (!(exp)) printf_error(PRNT_YELW "Assert(\a " PRNT_RSET # exp PRNT_YELW " );");
#ifndef __EXTLIB_C__
#define printf_debug(...)       if (0) {}
#define printf_debug_align(...) if (0) {}
#endif
#endif

#define Main(y1, y2) main(y1, y2)
#define Arg(arg)     ParseArgs(argv, arg, &parArg)
#define SEG_FAULT ((u32*)0)[0] = 0

#define AttPacked __attribute__ ((packed))
#define AttAligned(x) __attribute__((aligned(x)))

#define SleepF(sec)            usleep((u32)((f32)(sec) * 1000 * 1000))
#define SleepS(sec)            sleep(sec)
#define ParseArg(xarg)         ParseArgs(argv, xarg, &parArg)
#define EXT_INFO_TITLE(xtitle) PRNT_YELW xtitle PRNT_RNL
#define EXT_INFO(A, indent, B) PRNT_GRAY "[>]: " PRNT_RSET A "\r\033[" #indent "C" PRNT_GRAY "# " B PRNT_NL

#define renamer_remove(old, new) \
	if (rename(old, new)) { \
		if (remove(new)) { \
			printf_error_align( \
				"Delete failed", \
				"[%s]", \
				new \
			); \
		} \
		if (rename(old, new)) { \
			printf_error_align( \
				"Rename failed", \
				"[%s] -> [%s]", \
				old, \
				new \
			); \
		} \
	}

#endif /* __EXTLIB_H__ */
