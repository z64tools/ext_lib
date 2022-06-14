#ifndef __EXTLIB_H__
#define __EXTLIB_H__

#define _GNU_SOURCE
#define __CRT__NO_INLINE

#include <ExtTypes.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <float.h>

extern PrintfSuppressLevel gPrintfSuppress;
extern pthread_mutex_t gMutexLock;
extern u8 gPrintfProgressing;

void ThreadLock_Init(void);
void ThreadLock_Free(void);
void ThreadLock_Lock(void);
void ThreadLock_Unlock(void);
void ThreadLock_Create(Thread* thread, void* func, void* arg);
s32 ThreadLock_Join(Thread* thread);

void Thread_Create(Thread* thread, void* func, void* arg);
s32 Thread_Join(Thread* thread);
s32 ThreadLock_TryJoin(Thread* thread);

void SetSegment(const u8 id, void* segment);
void* SegmentedToVirtual(const u8 id, void32 ptr);
void32 VirtualToSegmented(const u8 id, void* ptr);

void* HeapMalloc(Size size);
char* HeapStrDup(const char* str);
char* HeapMemDup(const char* data, Size size);
char* HeapPrint(const char* fmt, ...);

void Time_Start(u32 slot);
f64 Time_Get(u32 slot);

void Dir_SetParam(DirParam w);
void Dir_UnsetParam(DirParam w);
void Dir_Set(char* path, ...);
void Dir_Enter(char* fmt, ...);
void Dir_Leave(void);
void Dir_Make(char* dir, ...);
void Dir_MakeCurrent(void);
char* Dir_Current(void);
char* Dir_File(char* fmt, ...);
Time Dir_Stat(const char* item);
char* Dir_GetWildcard(char* x);
void Dir_ItemList(ItemList* itemList, bool isPath);
void Dir_ItemList_Recursive(ItemList* target, char* keyword);
void Dir_ItemList_Not(ItemList* itemList, bool isPath, char* not);
void Dir_ItemList_Keyword(ItemList* itemList, char* ext);
char* Dir_FindFile(const char* str);

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

void SysExe_IgnoreError();
s32 SysExe_GetError();
s32 SysExe(const char* cmd);
char* SysExeO(const char* cmd);

s32 Terminal_YesOrNo(void);
void Terminal_ClearScreen(void);
void Terminal_ClearLines(u32 i);
void Terminal_Move_PrevLine(void);
void Terminal_Move(s32 x, s32 y);
const char* Terminal_GetStr(void);
char Terminal_GetChar();

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
void ItemList_Free(ItemList* itemList);
void ItemList_Alloc(ItemList* list, u32 num, Size size);
void ItemList_AddItem(ItemList* list, const char* item);

#ifndef __EXTLIB_C__

#define ItemList_GetWildItem(list, ...) ItemList_GetWildItem(list, __VA_ARGS__, NULL)
#define ItemList_SetFilter(list, ...)   ItemList_SetFilter(list, NARGS(__VA_ARGS__), __VA_ARGS__)

#endif

void printf_SetSuppressLevel(PrintfSuppressLevel lvl);
void printf_SetPrefix(char* fmt);
void printf_SetPrintfTypes(const char* d, const char* w, const char* e, const char* i);
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
void printf_clearMessages(void);

void __Assert(s32 expression, const char* msg, ...);
#define Assert(expression) \
	__Assert((s32)(expression), "" PRNT_DGRY "[" PRNT_REDD "%s" PRNT_DGRY "]:[" PRNT_YELW "%s" PRNT_DGRY "]:[" PRNT_BLUE "%d" PRNT_DGRY "]", #expression, __FUNCTION__, __LINE__)
f32 RandF();
void* MemMem(const void* haystack, size_t haystackSize, const void* needle, size_t needleSize);
void* StrStr(const char* haystack, const char* needle);
void* StrStrWhole(const char* haystack, const char* needle);
void* StrStrCase(const char* strA, const char* strB);
void* MemMemAlign(u32 val, const void* haystack, size_t haySize, const void* needle, size_t needleSize);
char* StrEnd(const char* src, const char* ext);
char* StrEndCase(const char* src, const char* ext);
void* ____Malloc(void* data, s32 size);
void* ____Calloc(void* data, s32 size);
void* ____Realloc(void* data, s32 size);
void* MemDup(const void* src, Size size);
char* StrDup(const char* src);
char* StrDupX(const char* src, Size size);
void* ____Free(const void* data);
void ByteSwap(void* src, s32 size);
s32 ParseArgs(char* argv[], char* arg, u32* parArg);
u32 Crc32(u8* s, u32 n);
void SlashAndPoint(const char* src, s32* slash, s32* point);
char* Path(const char* src);
char* PathSlot(const char* src, s32 num);
char* Basename(const char* src);
char* Filename(const char* src);
char* Line(const char* str, s32 line);
char* LineHead(const char* str);
char* Word(const char* str, s32 word);
char* FileExtension(const char* str);
void CaseToLow(char* s, s32 i);
void CaseToUp(char* s, s32 i);
s32 LineNum(const char* str);
s32 PathNum(const char* src);
char* CopyLine(const char* str, s32 line);
char* CopyWord(const char* str, s32 word);
char* PathRel(const char* file);
char* PathAbs(const char* item);
s32 PathIsAbs(const char* item);
s32 PathIsRel(const char* item);

void* qFree(const void* ptr);

char* AllcPath(const char* src);
char* AllcBasename(const char* src);
char* AllcFilename(const char* src);
char* AllcLine(const char* str, s32 line);
char* AllcWord(const char* str, s32 word);

void Color_ToHSL(HSL8* hsl, RGB8* rgb);
void Color_ToRGB(RGB8* rgb, HSL8* hsl);

void MemFile_Validate(MemFile* mem);
MemFile MemFile_Initialize();
void MemFile_Params(MemFile* memFile, ...);
void MemFile_Malloc(MemFile* memFile, u32 size);
void MemFile_Realloc(MemFile* memFile, u32 size);
void MemFile_Rewind(MemFile* memFile);
s32 MemFile_Write(MemFile* dest, void* src, u32 size);
s32 MemFile_Insert(MemFile* mem, void* src, s32 size, s64 pos);
s32 MemFile_Append(MemFile* dest, MemFile* src);
void MemFile_Align(MemFile* src, u32 align);
s32 MemFile_Printf(MemFile* dest, const char* fmt, ...);
s32 MemFile_Read(MemFile* src, void* dest, Size size);
#define MEMFILE_SEEK_END 0xFFFFFFFF
void* MemFile_Seek(MemFile* src, u32 seek);
void MemFile_LoadMem(MemFile* mem, void* data, Size size);
s32 MemFile_LoadFile(MemFile* memFile, const char* filepath);
s32 MemFile_LoadFile_String(MemFile* memFile, const char* filepath);
s32 MemFile_SaveFile(MemFile* memFile, const char* filepath);
s32 MemFile_SaveFile_String(MemFile* memFile, const char* filepath);
void MemFile_Free(MemFile* memFile);
void MemFile_Reset(MemFile* memFile);
void MemFile_Clear(MemFile* memFile);

#define StrStrNum(src, comp, n) MemMem(src, n, comp, n)
#define StrMtch(a, b)           (!strncmp(a, b, strlen(b)))
#define catprintf(dest, ...)    sprintf(dest + strlen(dest), __VA_ARGS__)

u32 Value_Hex(const char* string);
s32 Value_Int(const char* string);
f32 Value_Float(const char* string);
s32 Value_Bool(const char* string);
s32 Value_ValidateHex(const char* str);
s32 Value_ValidateInt(const char* str);
s32 Value_ValidateFloat(const char* str);
ValueType Value_Type(const char* variable);

s32 Music_NoteIndex(const char* note);
const char* Music_NoteWord(s32 note);

void StrIns(char* point, const char* insert);
void StrIns2(char* origin, const char* insert, s32 pos, s32 size);
void StrRem(char* point, s32 amount);
s32 StrRep(char* src, const char* word, const char* replacement);
wchar* StrU8(const char* str);
char* StrUnq(const char* str);
s32 StrComLen(const char* a, const char* b);
char* StrSlash(char* str);
char* StrStripIllegalChar(char* str);
void String_SwapExtension(char* dest, char* src, const char* ext);
char* String_GetSpacedArg(char* argv[], s32 cur);

char* Toml_Variable(const char* str, const char* name);
char* Toml_GetVariable(const char* str, const char* name);
void Toml_SetErrorState(bool boolean);
void Toml_GetArray(MemFile* mem, ItemList* list, const char* name);
s32 Toml_GetBool(MemFile* mem, const char* boolName);
s32 Toml_GetOption(MemFile* mem, const char* stringName, char* strList[]);
s32 Toml_GetInt(MemFile* mem, const char* intName);
char* Toml_GetStr(MemFile* mem, const char* stringName);
f32 Toml_GetFloat(MemFile* mem, const char* floatName);
void Toml_GotoSection(const char* section);
void Toml_ListVariables(MemFile* mem, ItemList* list, const char* section);

s32 Toml_ReplaceVariable(MemFile* mem, const char* variable, const char* fmt, ...);
void Toml_WriteComment(MemFile* mem, const char* comment);
void Toml_WriteArray(MemFile* mem, const char* variable, ItemList* list, bool quote, const char* comment);
void Toml_WriteInt(MemFile* mem, const char* variable, const s64 integer, const char* comment);
void Toml_WriteHex(MemFile* mem, const char* variable, const s64 integer, const char* comment);
void Toml_WriteStr(MemFile* mem, const char* variable, const char* str, bool quote, const char* comment);
void Toml_WriteFloat(MemFile* mem, const char* variable, const f64 flo, const char* comment);
void Toml_WriteSection(MemFile* mem, const char* variable);
#define Toml_Print(mem, ...) MemFile_Printf(mem, __VA_ARGS__)
#define NO_COMMENT NULL
#define QUOTES     1
#define NO_QUOTES  0

char* String_Tsv(char* str, s32 rowNum, s32 lineNum);

void Log_NoOutput(void);
void Log_Init();
void Log_Free();
void Log_Print();
void Log_Unlocked(const char* func, u32 line, const char* txt, ...);
void __Log(const char* func, u32 line, const char* txt, ...);
#define Log(...) __Log(__FUNCTION__, __LINE__, __VA_ARGS__)

f32 Math_SmoothStepToF(f32* pValue, f32 target, f32 fraction, f32 step, f32 minStep);
f32 Math_Spline_Audio(float k, float xm1, float x0, float x1, float x2);
f32 Math_Spline(f32 k, f32 xm1, f32 x0, f32 x1, f32 x2);
void Math_ApproachF(f32* pValue, f32 target, f32 fraction, f32 step);
void Math_ApproachS(s32* pValue, s32 target, s32 scale, s32 step);
s32 WrapS(s32 x, s32 min, s32 max);
f32 WrapF(f32 x, f32 min, f32 max);

void* Sound_Init(SoundFormat fmt, u32 sampleRate, u32 channelNum, SoundCallback callback, void* uCtx);
void Sound_Free(void* sound);
void Sound_Xm_Play(const void* data, u32 size);
void Sound_Xm_Stop();

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
#define PRNT_BOLD "\e[1m"

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
		Free(kill); \
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
#define IsBetween(a, x, y)          ((a) >= (x) && (a) <= (y))

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

#define NARGS_SEQ(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, N, ...) N
#define NARGS(...) \
	NARGS_SEQ(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

#define Malloc(data, size) { \
		Log("Malloc(%s); %.2f Kb", #data, BinToKb(size)); \
		data = ____Malloc(0, size); \
		Assert(data != NULL); \
}

#define Realloc(data, size) { \
		Log("Realloc(%s); %.2f Kb", #data, BinToKb(size)); \
		data = ____Realloc(data, size); \
		Assert(data != NULL); \
}

#define Calloc(data, size) { \
		Log("Calloc(%s); %.2f Kb", #data, BinToKb(size)); \
		data = ____Calloc(0, size); \
		Assert(data != NULL); \
}

#define Free(data) { \
		Log("Free(%s);", #data ); \
		data = ____Free(data); \
}

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

#define foreach(var, arr)        for (s32 var = 0; var < ArrayCount(arr); var++)
#define forlist(var, list)       for (s32 var = 0; var < (list).num; var++)
#define fornode(type, var, head) for (type* var = head; var != NULL; var = var->next)
#define forstr(var, str)         for (s32 var = 0; var < strlen(str); var++)

#ifndef _WIN32
#ifndef __IDE_FLAG__
#define stricmp(a, b)        strcasecmp(a, b)
#define strnicmp(a, b, size) strncasecmp(a, b, size)
#endif
#endif

int stricmp(const char* a, const char* b);
int strnicmp(const char* a, const char* b, Size size);

#endif /* __EXTLIB_H__ */
