#ifndef EXT_LIB_H
#define EXT_LIB_H

#define THIS_EXTLIB_VERSION 220

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

#ifdef __clang__
#define free __hidden_free
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

#undef free

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

void* dfree(const void* ptr);
void* freelist_que(void* ptr);
void* freelist_quecall(void* callback, void* ptr);
void freelist_free(void);

void bswap(void* src, int size);
void* alloc(int size);

hash_t hash_get(const void* data, size_t size);
bool hash_cmp(hash_t* a, hash_t* b);

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

void _log_print();

#ifdef __clang__
void _assert(bool);
void _log(const char* fmt, ...);
#else
void __log__(const char* func, u32 line, const char* txt, ...);
#define _log(...) __log__(__FUNCTION__, __LINE__, __VA_ARGS__)

#define _assert(v) do {                                         \
        if (!(v)) {                                             \
            _log("_assert( "PRNT_YELW "%s"PRNT_GRAY " )", # v); \
            _log_print();                                       \
            exit(1);                                            \
        }                                                       \
} while (0)

#endif

// __attribute__((deprecated))
char* Config_Variable(const char* str, const char* name);
// __attribute__((deprecated))
char* Config_GetVariable(const char* str, const char* name);
// __attribute__((deprecated))
void Config_ProcessIncludes(memfile_t* mem);
// __attribute__((deprecated))
s32 Config_GetErrorState(void);
// __attribute__((deprecated))
void Config_GetArray(memfile_t* mem, const char* variable, list_t* list);
// __attribute__((deprecated))
s32 Config_GetBool(memfile_t* mem, const char* variable);
// __attribute__((deprecated))
s32 Config_GetOption(memfile_t* mem, const char* variable, char* strList[]);
// __attribute__((deprecated))
s32 Config_GetInt(memfile_t* mem, const char* variable);
// __attribute__((deprecated))
char* Config_GetStr(memfile_t* mem, const char* variable);
// __attribute__((deprecated))
f32 Config_GetFloat(memfile_t* mem, const char* variable);
// __attribute__((deprecated))
void Config_GotoSection(const char* section);
// __attribute__((deprecated))
void Config_ListVariables(memfile_t* mem, list_t* list, const char* section);
// __attribute__((deprecated))
void Config_ListSections(memfile_t* cfg, list_t* list);
// __attribute__((deprecated))
s32 Config_ReplaceVariable(memfile_t* mem, const char* variable, const char* fmt, ...);
// __attribute__((deprecated))
void Config_WriteComment(memfile_t* mem, const char* comment);
// __attribute__((deprecated))
void Config_WriteArray(memfile_t* mem, const char* variable, list_t* list, bool quote, const char* comment);
// __attribute__((deprecated))
void Config_WriteInt(memfile_t* mem, const char* variable, const s64 integer, const char* comment);
// __attribute__((deprecated))
void Config_WriteHex(memfile_t* mem, const char* variable, const s64 integer, const char* comment);
// __attribute__((deprecated))
void Config_WriteStr(memfile_t* mem, const char* variable, const char* str, bool quote, const char* comment);
// __attribute__((deprecated))
void Config_WriteFloat(memfile_t* mem, const char* variable, const f64 flo, const char* comment);
// __attribute__((deprecated))
void Config_WriteBool(memfile_t* mem, const char* variable, const s32 val, const char* comment);
// __attribute__((deprecated))
void Config_WriteSection(memfile_t* mem, const char* variable, const char* comment);
#define Config_Print(mem, ...) memfile_fmt(mem, __VA_ARGS__)
#define NO_COMMENT NULL
#define QUOTES     1
#define NO_QUOTES  0

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

char* regex(const char* str, const char* pattern, regx_flag_t flag);

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

void print_lvl(PrintfSuppressLevel lvl);
void print_title(const char* toolname, const char* fmt, ...);
void print_warn(const char* fmt, ...);
void print_warn_align(const char* info, const char* fmt, ...);
void print_kill(FILE* output);
void print_error(const char* fmt, ...);
void print_error_align(const char* info, const char* fmt, ...);
void print_info(const char* fmt, ...);
void print_info_align(const char* info, const char* fmt, ...);
void print_prog_align(const char* info, const char* fmt, const char* color);
void print_prog_fast(const char* info, u32 a, u32 b);
void print_prog(const char* info, u32 a, u32 b);
void print_getc(const char* txt);
void print_volatile(const char* fmt, ...);
void print_win32_fix(void);
void print_hex(const char* txt, const void* data, u32 size, u32 dispOffset);
void print_bit(const char* txt, const void* data, u32 size, u32 dispOffset);
void print_nl(void);

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

list_t list_new(void);
void list_set_filters(list_t* list, u32 filterNum, ...);
void list_free_filters(list_t* this);
void list_walk(list_t* this, const char* path, s32 depth, ListFlag flags);
char* list_concat(list_t* this, const char* separator);
void list_tokenize2(list_t* list, const char* str, const char separator);
void list_print(list_t* target);
time_t list_stat_max(list_t* list);
time_t list_stat_min(list_t* list);
s32 list_sort_slot(list_t* this, bool checkOverlaps);
void list_free_items(list_t* this);
void list_free(list_t* this);
void list_alloc(list_t* this, u32 num);
void list_add(list_t* this, const char* item);
void list_combine(list_t* out, list_t* a, list_t* b);
void list_tokenize(list_t* this, const char* s, char r);
void list_sort(list_t* this);

#ifndef __clang__
#define list_set_filters(list, ...) list_set_filters(list, NARGS(__VA_ARGS__), __VA_ARGS__)
#endif

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

toml_t toml_new();
void toml_set_var(toml_t* this, const char* item, const char* fmt, ...);
void toml_set_tbl(toml_t* this, const char* item, ...);
bool toml_rm_var(toml_t* this, const char* fmt, ...);
bool toml_rm_arr(toml_t* this, const char* fmt, ...);
bool toml_rm_tbl(toml_t* this, const char* fmt, ...);
void toml_load(toml_t* this, const char* file);
void toml_free(toml_t* this);
void toml_print(toml_t* this, void* d, void (*PRINT)(void*, const char*, ...));
void toml_write_mem(toml_t* this, memfile_t* mem);
void toml_save(toml_t* this, const char* file);
int toml_get_int(toml_t* this, const char* item, ...);
f32 toml_get_float(toml_t* this, const char* item, ...);
bool toml_get_bool(toml_t* this, const char* item, ...);
char* toml_get_str(toml_t* this, const char* item, ...);
char* toml_get_var(toml_t* this, const char* item, ...);
int toml_arr_item_num(toml_t* this, const char* arr, ...);
int toml_tbl_item_num(toml_t* this, const char* item, ...);

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

memfile_t memfile_new();
void memfile_set(memfile_t* this, ...);
void memfile_alloc(memfile_t* this, size_t size);
void memfile_realloc(memfile_t* this, size_t size);
void memfile_rewind(memfile_t* this);
int memfile_write(memfile_t* this, const void* src, size_t size);
int memfile_append_file(memfile_t* this, const char* source);
int memfile_insert(memfile_t* this, const void* src, size_t size);
int memfile_append(memfile_t* this, memfile_t* src);
void memfile_align(memfile_t* this, size_t align);
int memfile_fmt(memfile_t* this, const char* fmt, ...);
int memfile_read(memfile_t* this, void* dest, size_t size);
void* memfile_seek(memfile_t* this, size_t seek);
void memfile_load_mem(memfile_t* this, void* data, size_t size);
int memfile_load_bin(memfile_t* this, const char* filepath);
int memfile_load_str(memfile_t* this, const char* filepath);
int memfile_save_bin(memfile_t* this, const char* filepath);
int memfile_save_str(memfile_t* this, const char* filepath);
void memfile_free(memfile_t* this);
void memfile_null(memfile_t* this);
void memfile_clear(memfile_t* this);

#ifndef __clang__
#define memfile_set(this, ...) memfile_set(this, __VA_ARGS__, MEM_END)
#endif

#define MEMFILE_SEEK_END 0xDEFEBABE

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

extern vbool gThreadMode;
extern mutex_t gThreadMutex;
extern const char* gThdPool_ProgressMessage;
void* threadpool_add(void* function, void* arg);
void threadpool_exec(u32 max);
void threadpool_set_id(void* __this, int id);
void threadpool_set_dep(void* __this, int id);

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

void SegmentSet(const u8 id, void* segment);
void* SegmentedToVirtual(const u8 id, void32 ptr);
void32 VirtualToSegmented(const u8 id, void* ptr);

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

void* x_alloc(size_t size);

char* x_strdup(const char* s);
char* x_strndup(const char* s, size_t n);
void* x_memdup(const void* d, size_t n);
char* x_fmt(const char* fmt, ...);
char* x_rep(const char* s, const char* a, const char* b);
char* x_cpyline(const char* s, size_t i);
char* x_cpyword(const char* s, size_t i);
char* x_path(const char* s);
char* x_pathslot(const char* s, int i);
char* x_basename(const char* s);
char* x_filename(const char* s);
char* x_randstr(size_t size, const char* charset);
char* x_strunq(const char* s);
char* x_enumify(const char* s);
char* x_canitize(const char* s);
char* x_dirrel_f(const char* from, const char* item);
char* x_dirabs_f(const char* from, const char* item);
char* x_dirrel( const char* item);
char* x_dirabs(const char* item);

char* strdup(const char* s);
char* strndup(const char* s, size_t n);
void* memdup(const void* d, size_t n);
char* fmt(const char* fmt, ...);
char* rep(const char* s, const char* a, const char* b);
char* cpyline(const char* s, size_t i);
char* cpyword(const char* s, size_t i);
char* path(const char* s);
char* pathslot(const char* s, int i);
char* basename(const char* s);
char* filename(const char* s);
char* randstr(size_t size, const char* charset);
char* strunq(const char* s);
char* enumify(const char* s);
char* canitize(const char* s);
char* dirrel_f(const char* from, const char* item);
char* dirabs_f(const char* from, const char* item);
char* dirrel( const char* item);
char* dirabs(const char* item);

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

#define calloc(size) calloc(1, size)

#ifdef __clang__
void* free(const void*, ...);

#else

void* __free(const void* data);
#define __impl_for_each_free(a) a = __free(a);
#define free(...)               ({                      \
        VA_ARG_MANIP(__impl_for_each_free, __VA_ARGS__) \
        NULL;                                           \
    })

#endif

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

int shex(const char* string);
int sint(const char* string);
f32 sfloat(const char* string);
int sbool(const char* string);
int vldt_hex(const char* str);
int vldt_int(const char* str);
int vldt_float(const char* str);
int digint(int i);
int dighex(int i);

hsl_t color_hsl(u8 r, u8 g, u8 b);
rgb8_t color_rgb8(f32 h, f32 s, f32 l);
rgba8_t color_rgba8(f32 h, f32 s, f32 l);
void color_cnvtohsl(hsl_t* dest, rgb8_t* src);
void color_cnvtorgb(rgb8_t* dest, hsl_t* src);

int Music_NoteIndex(const char* note);
const char* Music_NoteWord(int note);

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

f32 randf();
f32 Math_SmoothStepToF(f32* pValue, f32 target, f32 fraction, f32 step, f32 minStep);
f32 Math_Spline(f32 k, f32 xm1, f32 x0, f32 x1, f32 x2);
void Math_ApproachF(f32* pValue, f32 target, f32 fraction, f32 step);
void Math_ApproachS(s16* pValue, s16 target, s16 scale, s16 step);

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

void* memmem(const void* hay, size_t haylen, const void* nee, size_t neelen);
void* memmem_align(u32 val, const void* haystack, size_t haystacklen, const void* needle, size_t needlelen);
char* stristr(const char* haystack, const char* needle);
char* strwstr(const char* hay, const char* nee);
char* strnstr(const char* hay, const char* nee, size_t n);
char* strend(const char* src, const char* ext);
char* striend(const char* src, const char* ext);
char* strstart(const char* src, const char* ext);
char* stristart(const char* src, const char* ext);
int strarg(const char* args[], const char* arg);
char* strracpt(const char* str, const char* c);
char* strlnhead(const char* str, const char* head);
char* strline(const char* str, int line);
char* strword(const char* str, int word);
size_t linelen(const char* str);
size_t wordlen(const char* str);
bool chrspn(int c, const char* accept);
int strnocc(const char* s, size_t n, const char* accept);
int strocc(const char* s, const char* accept);
int strnins(char* dst, const char* src, size_t pos, int n);
int strins(char* dst, const char* src, size_t pos);
int strinsat(char* str, const char* ins);
int strrep(char* src, const char* mtch, const char* rep);
int strnrep(char* src, int len, const char* mtch, const char* rep);
int strwrep(char* src, const char* mtch, const char* rep);
int strwnrep(char* src, int len, const char* mtch, const char* rep);
char* strfext(const char* str);
char* strtoup(char* str);
char* strtolo(char* str);
void strntolo(char* s, int i);
void strntoup(char* s, int i);
void strrem(char* point, int amount);
char* strflipslash(char* t);
char* strfssani(char* t);
int strstrlen(const char* a, const char* b);
char* String_GetSpacedArg(const char** args, int cur);
void strswapext(char* dest, const char* src, const char* ext);
char* strarrcat(const char** list, const char* separator);
size_t strwlen(const wchar* s);
char* strto8(char* dst, const wchar* src);
wchar* strto16(wchar* dst, const char* src);
int linenum(const char* str);
int dirnum(const char* src);
int dir_isabs(const char* item);
int dir_isrel(const char* item);

#ifndef _WIN32
#ifndef __clang__
#define stricmp(a, b)        strcasecmp(a, b)
#define strnicmp(a, b, size) strncasecmp(a, b, size)
#endif
#endif

int stricmp(const char* a, const char* b);
int strnicmp(const char* a, const char* b, size_t size);

bool sys_isdir(const char* path);
time_t sys_stat(const char* item);
const char* sys_app(void);
const char* sys_appdata(void);
time_t sys_time(void);
void sys_sleep(f64 sec);
void sys_mkdir(const char* dir, ...);
const char* sys_workdir(void);
const char* sys_appdir(void);
int sys_mv(const char* input, const char* output);
int sys_rm(const char* item);
int sys_rmdir(const char* item);
void sys_setworkpath(const char* txt);
int sys_touch(const char* file);
int sys_cp(const char* src, const char* dest);
date_t sys_timedate(time_t time);
int sys_getcorenum(void);
size_t sys_statsize(const char* file);
const char* sys_env(env_index_t env);

int sys_exe(const char* cmd);
void sys_exed(const char* cmd);
int sys_exel(const char* cmd, int (*callback)(void*, const char*), void* arg);
void sys_exes_noerr();
int sys_exes_return();
char* sys_exes(const char* cmd);

void fs_mkflag(bool flag);
void fs_set(const char* fmt, ...);
char* fs_item(const char* str, ...);
char* fs_find(const char* str);

bool cli_yesno(void);
void cli_clear(void);
void cli_clearln(int i);
void cli_gotoprevln(void);
const char* cli_gets(void);
char cli_getc();
void cli_hide(void);
void cli_show(void);
void cli_getSize(int* r);
void cli_getPos(int* r);

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

int qsort_method_numhex(const void* ptrA, const void* ptrB);

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

static inline void thd_lock(void) {
    pthread_mutex_lock(&gThreadMutex);
}

static inline void thd_unlock(void) {
    
    pthread_mutex_unlock(&gThreadMutex);
}

static inline int thd_create(thread_t* thread, void* func, void* arg) {
    return pthread_create(thread, NULL, (void*)func, (void*)(arg));
}

static inline int thd_join(thread_t* thread) {
    return pthread_join(*thread, NULL);
}

#pragma GCC diagnostic pop

#ifndef __clang__
#define strncat(str, cat, size) strncat(str, cat, (size) - 1)
#define strncpy(str, cpy, size) strncpy(str, cpy, (size) - 1)
#endif

#endif /* EXT_LIB_H */
