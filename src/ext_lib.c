#pragma GCC diagnostic ignored "-Wunused-result"
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"

#ifndef _WIN32
#define _XOPEN_SOURCE 500
#define _DEFAULT_SOURCE
#endif

#include "ext_lib.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <ftw.h>
#include <time.h>

//crustify
#ifndef EXTLIB_PERMISSIVE
	#ifdef EXTLIB
		#if EXTLIB < THIS_EXTLIB_VERSION
			#warning ExtLib copy is newer than the project its used with
		#endif
	#endif
#endif
//uncrustify

#ifdef _WIN32
#include <windows.h>
#include <libloaderapi.h>
#endif

#define stdlog stderr

f32 EPSILON = 0.0000001f;
f32 gDeltaTime = 1.0f;

// # # # # # # # # # # # # # # # # # # # #
// # THREAD                              #
// # # # # # # # # # # # # # # # # # # # #

vbool gThreadMode;
Mutex gThreadMutex;
static vbool gKillFlag;

// # # # # # # # # # # # # # # # # # # # #
// # ext_lib Construct Destruct           #
// # # # # # # # # # # # # # # # # # # # #

typedef struct PostFreeNode {
    struct PostFreeNode* next;
    void* ptr;
    void (*free)(void*);
} PostFreeNode;

static PostFreeNode* sPostFreeHead;
static PostFreeNode* sPostFreeHead2;
static Mutex sQMutex;

void* qFree(const void* ptr) {
    pthread_mutex_lock(&sQMutex); {
        PostFreeNode* node;
        
        node = Calloc(sizeof(struct PostFreeNode));
        node->ptr = (void*)ptr;
        
        Node_Add(sPostFreeHead, node);
    } pthread_mutex_unlock(&sQMutex);
    
    return (void*)ptr;
}

void* PostFree_Queue(void* ptr) {
    PostFreeNode* n = New(PostFreeNode);
    
    Mutex_Lock();
    Node_Add(sPostFreeHead2, n);
    Mutex_Unlock();
    n->ptr = ptr;
    
    return ptr;
}

void* PostFree_QueueCallback(void* callback, void* ptr) {
    PostFreeNode* n = New(PostFreeNode);
    
    Mutex_Lock();
    Node_Add(sPostFreeHead2, n);
    Mutex_Unlock();
    n->ptr = ptr;
    n->free = callback;
    
    return ptr;
}

void PostFree_Free(void) {
    while (sPostFreeHead2) {
        if (sPostFreeHead2->free)
            sPostFreeHead2->free(sPostFreeHead2->ptr);
        else
            Free(sPostFreeHead2->ptr);
        
        Node_Kill(sPostFreeHead2, sPostFreeHead2);
    }
}

static struct timeval sProfiTime;

__attribute__ ((constructor)) void ExtLib_Init(void) {
    gettimeofday(&sProfiTime, NULL);
    Log_Init();
    
    srand((u32)(clock() + time(0)));
    
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    printf_WinFix();
#endif
}

static void* sDestructorArg;
static void (*sDestructor)(void*);

void Sys_SetDestFunc(void* func, void* arg) {
    sDestructor = func;
    sDestructorArg = arg;
}

__attribute__ ((destructor)) void ExtLib_Destroy(void) {
    while (sPostFreeHead) {
        Free(sPostFreeHead->ptr);
        Node_Kill(sPostFreeHead, sPostFreeHead);
    }
    
    if (sDestructor)
        sDestructor(sDestructorArg);
    
    Log_Free();
}

void profilog(const char* msg) {
    struct timeval next;
    f32 time;
    
    gettimeofday(&next, NULL);
    
    time = (next.tv_sec - sProfiTime.tv_sec) + (next.tv_usec - sProfiTime.tv_usec) / 1000000.0;
    sProfiTime = next;
    
    printf("" PRNT_PRPL "-" PRNT_GRAY ": " PRNT_RSET "%-16s " PRNT_YELW "%.2f " PRNT_RSET "ms\n", msg,  time * 1000.0f);
}

void profilogdiv(const char* msg, f32 div) {
    struct timeval next;
    f32 time;
    
    gettimeofday(&next, NULL);
    time = (next.tv_sec - sProfiTime.tv_sec) + (next.tv_usec - sProfiTime.tv_usec) / 1000000.0;
    time /= div;
    sProfiTime = next;
    
    printf("" PRNT_PRPL "~" PRNT_GRAY ": " PRNT_RSET "%-16s " PRNT_YELW "%.2f " PRNT_RSET "ms\n", msg,  time * 1000.0f);
}

// # # # # # # # # # # # # # # # # # # # #
// # ThreadPool                          #
// # # # # # # # # # # # # # # # # # # # #

typedef enum {
    T_IDLE,
    T_RUN,
    T_DONE,
} Tstate;

typedef struct ThdItem {
    struct ThdItem* next;
    volatile Tstate state;
    Thread thd;
    void (*function)(void*);
    void* arg;
    vu8   nid;
    vu8   numDep;
    vs32  dep[16];
} ThdItem;

typedef struct {
    ThdItem* head;
    vu32     num;
    vu16     dep[__UINT8_MAX__];
    struct {
        vbool mutex : 1;
        vbool on    : 1;
    };
} ThdPool;

static ThdPool* gTPool;

static __attribute__((noinline)) void ThdPool_AddToHead(ThdItem* t) {
    if (t->nid)
        gTPool->dep[t->nid]++;
    
    Node_Add(gTPool->head, t);
    gTPool->num++;
}

#undef ThdPool_Add
void ThdPool_Add(void* function, void* arg, u32 n, ...) {
    ThdItem* t = New(ThdItem);
    va_list va;
    
    if (!gTPool) {
        gTPool = qFree(New(ThdPool));
        Assert(gTPool);
    }
    
    Assert(gTPool->on == false);
    
    t->function = function;
    t->arg = arg;
    
    va_start(va, n);
    for (s32 i = 0; i < n; i++) {
        s32 val = va_arg(va, s32);
        
        if (val > 0)
            t->nid = val;
        
        if (val < 0)
            t->dep[t->numDep++] = -val;
    }
    va_end(va);
    
    Mutex_Lock();
    ThdPool_AddToHead(t);
    Mutex_Unlock();
    Log("%d", gTPool->num);
}

static void ThdPool_RunThd(ThdItem* thd) {
    thd->function(thd->arg);
    thd->state = T_DONE;
}

static bool ThdPool_CheckDep(ThdItem* t) {
    if (!t->numDep)
        return true;
    
    for (s32 i = 0; i < t->numDep; i++)
        if (gTPool->dep[t->dep[i]] != 0)
            return false;
    
    return true;
}

const char* gThdPool_ProgressMessage;

void ThdPool_Exec(u32 max, bool mutex) {
    u32 amount = gTPool->num;
    u32 prev = 1;
    u32 prog = 0;
    u32 cur = 0;
    ThdItem* t;
    bool msg = gThdPool_ProgressMessage != NULL;
    
    if (max == 0)
        max = 1;
    
    if (mutex)
        Mutex_Enable();
    
    gTPool->on = true;
    
    Log("Num: %d", gTPool->num);
    Log("Max: %d", max);
    while (gTPool->num) {
        
        if (msg && prev != prog)
            printf_progressFst(gThdPool_ProgressMessage, prog + 1, amount);
        
        max = Min(max, gTPool->num);
        
        if (cur < max) { /* Assign */
            t = gTPool->head;
            
            while (t) {
                if (t->state == T_IDLE)
                    if (ThdPool_CheckDep(t))
                        break;
                
                t = t->next;
            }
            
            if (t) {
                while (t->state != T_RUN)
                    t->state = T_RUN;
                
                Log("Create Thread");
                if (Thread_Create(&t->thd, ThdPool_RunThd, t))
                    printf_error("ThdPool: Could not create thread");
                
                cur++;
            }
            
        }
        
        prev = prog;
        if (true) { /* Clean */
            t = gTPool->head;
            while (t) {
                if (t->state == T_DONE) {
                    Thread_Join(&t->thd);
                    break;
                }
                
                t = t->next;
            }
            
            if (t) {
                if (t->nid)
                    gTPool->dep[t->nid]--;
                
                Node_Kill(gTPool->head, t);
                
                cur--;
                gTPool->num--;
                prog++;
            }
        }
    }
    
    if (mutex)
        Mutex_Disable();
    
    gThdPool_ProgressMessage = NULL;
    gTPool->on = false;
}

// # # # # # # # # # # # # # # # # # # # #
// # SEGMENT                             #
// # # # # # # # # # # # # # # # # # # # #

static u8** sSegment;

void SetSegment(const u8 id, void* segment) {
    Log("%-4d%08X", id, segment);
    if (!sSegment) {
        sSegment = qFree(New(char*[255]));
        Assert(sSegment);
    }
    
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

typedef struct {
    u8*  head;
    Size offset;
    Size max;
    Size f;
} BufferX;

void* x_alloc(Size size) {
    static Mutex xmutex;
    static BufferX this = {
        .max = MbToBin(8),
        .f   = MbToBin(2),
    };
    u8* ret;
    
    if (size <= 0)
        return NULL;
    
    if (size >= this.f)
        printf_error("Biggest Failure");
    
    pthread_mutex_lock(&xmutex);
    if (this.head == NULL) {
        this.head = qFree(Alloc(this.max));
        Assert(this.head != NULL);
    }
    
    if (this.offset + size + 10 >= this.max) {
        Log("Rewind: [ %.3fkB / %.3fkB ]", BinToKb(this.offset), BinToKb(this.max));
        this.offset = 0;
    }
    
    ret = &this.head[this.offset];
    this.offset += size + 1;
    pthread_mutex_unlock(&xmutex);
    
    return memset(ret, 0, size + 1);
}

char* x_strdup(const char* str) {
    return x_memdup(str, strlen(str) + 1);
}

char* x_strndup(const char* s, Size n) {
    if (!n || !s) return NULL;
    Size csz = strnlen(s, n);
    char* new = x_alloc(n + 1);
    char* res = memcpy (new, s, csz);
    
    return res;
}

char* x_memdup(const char* data, Size size) {
    if (!data || !size)
        return NULL;
    
    return memcpy(x_alloc(size), data, size);
}

char* x_fmt(const char* fmt, ...) {
    char buf[4096];
    s32 len;
    va_list va;
    
    va_start(va, fmt);
    Assert((len = vsnprintf(buf, 4096, fmt, va)) < 4096);
    va_end(va);
    
    return x_memdup(buf, len + 1);
}

char* x_rep(const char* str, const char* a, const char* b) {
    char* r = x_alloc(strlen(str) * 4 + strlen(b) * 8);
    
    strcpy(r, str);
    StrRep(r, a, b);
    
    return r;
}

char* x_line(const char* str, s32 line) {
    if (!(str = Line(str, line))) return NULL;
    
    return x_strndup(str, LineLen(str));
}

char* x_word(const char* str, s32 word) {
    if (!(str = Word(str, word))) return NULL;
    
    return x_strndup(str, WordLen(str));
}

static void SlashAndPoint(const char* src, s32* slash, s32* point) {
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

char* x_path(const char* src) {
    char* buffer;
    s32 point;
    s32 slash;
    
    if (src == NULL)
        return NULL;
    
    SlashAndPoint(src, &slash, &point);
    
    if (slash == 0)
        slash = -1;
    
    buffer = x_alloc(slash + 1 + 1);
    memcpy(buffer, src, slash + 1);
    buffer[slash + 1] = '\0';
    
    return buffer;
}

char* x_pathslot(const char* src, s32 num) {
    const char* slot = src;
    
    if (src == NULL)
        return NULL;
    
    if (num < 0)
        num = PathNum(src) - 1;
    
    for (; num; num--) {
        slot = strchr(slot, '/');
        if (slot) slot++;
    }
    if (!slot) return NULL;
    
    const s32 s = strcspn(slot, "/");
    char* r = x_strndup(slot, s ? s + 1 : 0);
    Log("[%s]", r);
    return r;
}

char* x_basename(const char* src) {
    char* ls = StrChrAcpt(src, "\\/");
    
    if (!ls++)
        ls = (char*)src;
    
    return x_strndup(ls, strcspn(ls, "."));
}

char* x_filename(const char* src) {
    char* ls = StrChrAcpt(src, "\\/");
    
    if (!ls++)
        ls = (char*)src;
    
    return x_strdup(ls);
}

const char sDefCharSet[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

char* x_randstr(Size size, const char* charset) {
    const char* set = charset ? charset : sDefCharSet;
    const s32 setz = (charset ? strlen(charset) : sizeof(sDefCharSet)) - 1;
    char* str = x_alloc(size);
    
    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int key = rand() % setz;
            str[n] = set[key];
        }
        str[size] = '\0';
    }
    return str;
}

char* basename(const char* src) {
    char* ls = StrChrAcpt(src, "\\/");
    
    if (!ls++)
        ls = (char*)src;
    
    return strndup(ls, strcspn(ls, "."));
}

char* filename(const char* src) {
    char* ls = StrChrAcpt(src, "\\/");
    
    if (!ls++)
        ls = (char*)src;
    
    return strdup(ls);
}

// # # # # # # # # # # # # # # # # # # # #
// # TIME                                #
// # # # # # # # # # # # # # # # # # # # #

ThreadLocal static struct timeval sTimeStart[255];

void Time_Start(u32 slot) {
    gettimeofday(&sTimeStart[slot], NULL);
}

f32 Time_Get(u32 slot) {
    struct timeval sTimeStop;
    
    gettimeofday(&sTimeStop, NULL);
    
    return (sTimeStop.tv_sec - sTimeStart[slot].tv_sec) + (f32)(sTimeStop.tv_usec - sTimeStart[slot].tv_usec) / 1000000;
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
// # SYS                                 #
// # # # # # # # # # # # # # # # # # # # #

ThreadLocal static char __sPath[261];
ThreadLocal static bool __sMakeDir;

void FileSys_MakePath(bool flag) {
    __sMakeDir = flag;
}

void FileSys_Path(const char* fmt, ...) {
    va_list va;
    
    va_start(va, fmt);
    vsnprintf(__sPath, 261, fmt, va);
    va_end(va);
    
    if (__sMakeDir)
        Sys_MakeDir(__sPath);
}

char* FileSys_File(const char* str, ...) {
    char buffer[261];
    char* ret;
    va_list va;
    
    va_start(va, str);
    vsnprintf(buffer, 261, str, va);
    va_end(va);
    
    Log("%s", str);
    Assert(buffer != NULL);
    if (buffer[0] != '/' && buffer[0] != '\\')
        ret = x_fmt("%s%s", __sPath, buffer);
    else
        ret = x_strdup(buffer + 1);
    
    return ret;
}

char* FileSys_FindFile(const char* str) {
    ItemList list = ItemList_Initialize();
    char* file = NULL;
    Time stat = 0;
    
    if (*str == '*') str++;
    
    Log("Find: [%s] in [%s]", str, __sPath);
    
    ItemList_List(&list, __sPath, 0, LIST_FILES | LIST_NO_DOT);
    for (s32 i = 0; i < list.num; i++) {
        if (StrEndCase(list.item[i], str) && Sys_Stat(list.item[i]) > stat) {
            file = list.item[i];
            stat = Sys_Stat(file);
        }
    }
    
    if (file) {
        file = x_strdup(file);
        Log("Found: %s", file);
    }
    
    ItemList_Free(&list);
    
    return file;
}

// # # # # # # # # # # # # # # # # # # # #
// # SYS                                 #
// # # # # # # # # # # # # # # # # # # # #

static s32 sSysIgnore;
static s32 sSysReturn;

bool Sys_IsDir(const char* path) {
    struct stat st = { 0 };
    
    stat(path, &st);
    if (S_ISDIR(st.st_mode))
        return true;
    
    if (StrEnd(path, "/")) {
        char* tmp = x_strdup(path);
        StrEnd(tmp, "/")[0] = '\0';
        
        stat(tmp, &st);
        
        if (S_ISDIR(st.st_mode))
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
        item = strdup(item);
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

const char* Sys_ThisApp(void) {
    static char buf[512];
    
#ifdef _WIN32
    GetModuleFileName(NULL, buf, 512);
#else
    (void)readlink("/proc/self/exe", buf, 512);
#endif
    
    return buf;
}

const char* Sys_ThisAppData(void) {
    const char* app = x_basename(Sys_ThisApp());
    static char buf[512];
    
    snprintf(buf, 512, "%s/%s/", Sys_GetEnv(ENV_APPDATA), app);
    Sys_MakeDir("%s", buf);
    
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
    char buffer[261];
    va_list args;
    
    va_start(args, dir);
    vsnprintf(buffer, 261, dir, args);
    va_end(args);
    
    const s32 m = strlen(buffer);
    
#ifdef _WIN32
    s32 i = 0;
    if (PathIsAbs(buffer))
        i = 3;
    
    for (; i < m; i++) {
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
        for (s32 i = m - 1; i >= 0; i--) {
            if (buffer[i] == '/' || buffer[i] == '\\')
                break;
            buffer[i] = '\0';
        }
    }
    
    __MakeDir(buffer);
}

const char* Sys_WorkDir(void) {
    static char buf[512];
    
    if (getcwd(buf, sizeof(buf)) == NULL)
        printf_error("Could not get Sys_WorkDir");
    
    const u32 m = strlen(buf);
    for (s32 i = 0; i < m; i++) {
        if (buf[i] == '\\')
            buf[i] = '/';
    }
    
    strcat(buf, "/");
    
    return buf;
}

char* sAppDir;

const char* Sys_AppDir(void) {
    if (!sAppDir) {
        sAppDir = qFree(strndup(x_path(Sys_ThisApp()), 512));
        StrRep(sAppDir, "\\", "/");
        
        if (!StrEnd(sAppDir, "/"))
            strcat(sAppDir, "/");
    }
    
    return sAppDir;
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

void SysExeD(const char* cmd) {
#ifdef _WIN32
    SysExe(x_fmt("start %s", cmd));
#else
    SysExe(x_fmt("nohup %s", cmd));
#endif
}

char* SysExeO(const char* cmd) {
    char* sExeBuffer;
    char result[1025];
    FILE* file = NULL;
    u32 size = 0;
    
    if ((file = popen(cmd, "r")) == NULL) {
        Log(PRNT_REDD "SysExeO(%s);", cmd);
        Log("popen failed!");
        
        return NULL;
    }
    
    sExeBuffer = Alloc(MbToBin(4));
    Assert(sExeBuffer != NULL);
    sExeBuffer[0] = '\0';
    
    while (fgets(result, 1024, file)) {
        size += strlen(result);
        Assert (size < MbToBin(4));
        strcat(sExeBuffer, result);
    }
    
    if ((sSysReturn = pclose(file)) != 0) {
        if (sSysIgnore == 0) {
            printf("%s\n", sExeBuffer);
            Log(PRNT_REDD "[%d] " PRNT_GRAY "SysExeO(" PRNT_REDD "%s" PRNT_GRAY ");", sSysReturn, cmd);
            printf_error("SysExeO");
        }
    }
    
    return sExeBuffer;
}

s32 SysExeC(const char* cmd, s32 (*callback)(void*, const char*), void* arg) {
    char* s;
    FILE* file;
    
    if ((file = popen(cmd, "r")) == NULL) {
        Log(PRNT_REDD "SysExeO(%s);", cmd);
        Log("popen failed!");
        
        return -1;
    }
    
    s = Alloc(1025);
    while (fgets(s, 1024, file))
        if (callback)
            if (callback(arg, s))
                break;
    Free(s);
    
    return pclose(file);
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
#ifndef __clang__
#include <sys/ioctl.h>
    struct winsize w;
    
    ioctl(0, TIOCGWINSZ, &w);
    
    x = w.ws_col;
    y = w.ws_row;
#endif // __clang__
#endif // _WIN32
    
    r[0] = x;
    r[1] = y;
}

void Sys_TerminalCursorPos(s32* r) {
    s32 x = 0;
    s32 y = 0;
    
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO cbsi;
    
    Assert (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cbsi));
    
    x = cbsi.dwCursorPosition.X;
    y = cbsi.dwCursorPosition.Y;
#else
#endif // _WIN32
    
    r[0] = x;
    r[1] = y;
}

s32 Sys_Touch(const char* file) {
    if (!Sys_Stat(file)) {
        FILE* f = fopen(file, "w");
        Assert(f != NULL);
        fclose(f);
        
        return 0;
    }
    
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
    if (Sys_IsDir(src)) {
        ItemList list = ItemList_Initialize();
        
        ItemList_List(&list, src, -1, LIST_FILES);
        
        forlist(i, list) {
            char* dfile = x_fmt(
                "%s%s%s",
                dest,
                StrEnd(dest, "/") ? "" : "/",
                list.item[i] + strlen(src)
            );
            
            Log("Copy: %s -> %s", list.item[i], dfile);
            
            Sys_MakeDir(x_path(dfile));
            if (Sys_Copy(list.item[i], dfile))
                return 1;
        }
        
        ItemList_Free(&list);
    } else {
        MemFile a = MemFile_Initialize();
        
        a.param.throwError = false;
        
        if (MemFile_LoadFile(&a, src))
            return -1;
        if (MemFile_SaveFile(&a, dest))
            return 1;
        MemFile_Free(&a);
    }
    
    return 0;
}

Date Sys_Date(Time time) {
    Date date = { 0 };
    
#ifndef __clang__
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

s32 Sys_GetCoreCount(void) {
    {
#ifndef __clang__
#ifdef _WIN32
        #define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
        s64 nprocs = -1;
        s64 nprocs_max = -1;
        
#ifdef _WIN32
#ifndef _SC_NPROCESSORS_ONLN
        SYSTEM_INFO info;
        GetSystemInfo(&info);
        #define sysconf(a) info.dwNumberOfProcessors
        #define _SC_NPROCESSORS_ONLN
#endif
#endif
#ifdef _SC_NPROCESSORS_ONLN
        nprocs = sysconf(_SC_NPROCESSORS_ONLN);
        if (nprocs < 1) {
            return 0;
        }
        nprocs_max = sysconf(_SC_NPROCESSORS_CONF);
        if (nprocs_max < 1) {
            return 0;
        }
        
        return nprocs;
#else
        
#endif
#endif
    }
    
    return 0;
}

Size Sys_GetFileSize(const char* file) {
    FILE* f = fopen(file, "r");
    u32 result = -1;
    
    if (!f) return result;
    
    fseek(f, 0, SEEK_END);
    result = ftell(f);
    fclose(f);
    
    return result;
}

const char* Sys_GetEnv(SysEnv env) {
    switch (env) {
#ifdef _WIN32
        case ENV_USERNAME:
            return x_strdup(getenv("USERNAME"));
        case ENV_APPDATA:
            return StrSlash(x_strdup(getenv("APPDATA")));
        case ENV_HOME:
            return StrSlash(x_strdup(getenv("USERPROFILE")));
        case ENV_TEMP:
            return StrSlash(x_strdup(getenv("TMP")));
            
#else // UNIX
        case ENV_USERNAME:
            return x_strdup(getenv("USER"));
        case ENV_APPDATA:
            return x_strdup("~/.local");
        case ENV_HOME:
            return x_strdup(getenv("HOME"));
        case ENV_TEMP:
            return x_strdup("/tmp");
            
#endif
    }
    
    return NULL;
}

const char* Sys_TmpFile(const char* path) {
    u64 l;
    
redo:
    l = RandF() * (f64)__UINT64_MAX__;
    
    Sys_MakeDir("%s/ext_lib/%s%s",
        Sys_GetEnv(ENV_TEMP),
        path ? path : "",
        path ? (StrEnd(path, "/") ? "" : "/") : "");
    
    const char* mk = x_fmt("%s/ext_lib/%s%s%02X%02X%02X%02X-%016X.tmp",
            Sys_GetEnv(ENV_TEMP),
            path ? path : "",
            StrEnd(path, "/") ? "" : "/",
            Sys_Date(Sys_Time()).month,
            Sys_Date(Sys_Time()).day,
            Sys_Date(Sys_Time()).minute,
            Sys_Date(Sys_Time()).second,
            l);
    
    if (Sys_Stat(mk))
        goto redo;
    
    return mk;
}

#ifdef EXT_LIB_SYS_AUDIO

void Sys_Beep(s32 freq, s32 time) {
    Beep(freq, time);
}

void Sys_Sound(const char* type) {
#ifdef _WIN32
    if (!stricmp(type, "error"))
        PlaySound("SystemHand", NULL, SND_ASYNC);
    if (!stricmp(type, "warning"))
        PlaySound("SystemExclamation", NULL, SND_ASYNC);
#endif
}

static volatile bool sKill;
static volatile bool sRunning;

static s32 FatalJingle(void* this) {
    struct {
        s32 freq;
        s32 time;
    } jingle[] = {
        { 349, 125 }, { 392, 125 }, { 466, 125 }, { 392, 125  },
        { 587, 375 }, { 587, 375 }, { 523, 750 }, { 349, 125  },
        { 392, 125 }, { 466, 125 }, { 392, 125 }, { 523, 375  },
        { 523, 375 }, { 466, 500 }, { 440, 125 }, { 392, 250  },
        { 349, 125 }, { 392, 125 }, { 466, 125 }, { 392, 125  },
        { 466, 500 }, { 523, 250 }, { 440, 375 }, { 392, 125  },
        { 349, 500 }, { 349, 250 }, { 523, 500 }, { 466, 1000 },
        { 349, 125 }, { 392, 125 }, { 466, 125 }, { 392, 125  },
        { 587, 375 }, { 587, 375 }, { 523, 750 }, { 349, 125  },
        { 392, 125 }, { 466, 125 }, { 392, 125 }, { 698, 500  },
        { 440, 250 }, { 466, 500 }, { 440, 125 }, { 392, 250  },
        { 349, 125 }, { 392, 125 }, { 466, 125 }, { 392, 125  },
        { 466, 500 }, { 523, 250 }, { 440, 375 }, { 392, 125  },
        { 349, 500 }, { 349, 250 }, { 523, 500 }, { 466, 1000 }
    };
    
    foreach(i, jingle) {
        Sys_Beep(jingle[i].freq, jingle[i].time);
        if (sKill) {
            sRunning = false;
            return 0;
        }
    }
    
    sRunning = false;
    return 0;
}

void Sys_FatalJingle(void) {
    static Thread t;
    
    while (sRunning) {
        sKill = true;
    }
    sKill = false;
    
    sRunning = true;
    Thread_Create(&t, FatalJingle, NULL);
}

#endif

// # # # # # # # # # # # # # # # # # # # #
// # TERMINAL                            #
// # # # # # # # # # # # # # # # # # # # #

bool Terminal_YesOrNo(void) {
    char line[16] = {};
    u32 clear = 0;
    bool ret = true;
    
    while (stricmp(line, "y") && stricmp(line, "n")) {
        if (gKillFlag)
            return -1;
        if (clear++)
            Terminal_ClearLines(2);
        
        printf("\r" PRNT_GRAY "<" PRNT_GRAY ": " PRNT_BLUE);
        fgets(line, 16, stdin);
        
        while (StrEnd(line, "\n"))
            StrEnd(line, "\n")[0] = '\0';
    }
    
    if (!stricmp(line, "n"))
        ret = false;
    
    Terminal_ClearLines(2);
    
    return ret;
}

void Terminal_ClearScreen(void) {
    printf("\033[2J");
    fflush(stdout);
}

void Terminal_ClearLines(u32 i) {
    printf("\x1b[2K");
    for (s32 j = 1; j < i; j++) {
        Terminal_Move_PrevLine();
        printf("\x1b[2K");
    }
    fflush(stdout);
}

void Terminal_Move_PrevLine(void) {
    printf("\x1b[1F");
    fflush(stdout);
}

const char* Terminal_GetStr(void) {
    static char line[512] = { 0 };
    
    printf("\r" PRNT_GRAY "<" PRNT_GRAY ": " PRNT_RSET);
    fgets(line, 511, stdin);
    
    while (StrEnd(line, "\n"))
        StrEnd(line, "\n")[0] = '\0';
    
    Log("[%s]", line);
    
    return line;
}

char Terminal_GetChar() {
    char line[16] = {};
    
    printf("\r" PRNT_GRAY "<" PRNT_GRAY ": " PRNT_BLUE);
    fgets(line, 16, stdin);
    
    Terminal_ClearLines(2);
    
    return 0;
}

void Terminal_Hide(void) {
#ifdef _WIN32
    HWND window = GetConsoleWindow();
    ShowWindow(window, SW_HIDE);
    
#else
#endif
}

void Terminal_Show(void) {
#ifdef _WIN32
    HWND window = GetConsoleWindow();
    ShowWindow(window, SW_SHOW);
#else
#endif
}

// # # # # # # # # # # # # # # # # # # # #
// # VARIOUS                             #
// # # # # # # # # # # # # # # # # # # # #

f32 RandF() {
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

char* StrStr(const char* haystack, const char* needle) {
    if (!haystack || !needle)
        return NULL;
    
    return MemMem(haystack, strlen(haystack), needle, strlen(needle));
}

char* StrStrWhole(const char* haystack, const char* needle) {
    char* p = StrStr(haystack, needle);
    
    while (p) {
        if (!isgraph(p[-1]) && !isgraph(p[strlen(needle)]))
            return p;
        
        p = StrStr(p + 1, needle);
    }
    
    return NULL;
}

char* StrStrCase(const char* haystack, const char* needle) {
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
    const s32 xlen = strlen(ext);
    const s32 slen = strlen(src);
    
    if (slen < xlen) return NULL;
    fP = (char*)(src + slen - xlen);
    if (!strcmp(fP, ext)) return fP;
    
    return NULL;
}

char* StrEndCase(const char* src, const char* ext) {
    char* fP;
    const s32 xlen = strlen(ext);
    const s32 slen = strlen(src);
    
    if (slen < xlen) return NULL;
    fP = (char*)(src + slen - xlen);
    if (!stricmp(fP, ext)) return fP;
    
    return NULL;
}

char* StrStart(const char* src, const char* ext) {
    Size extlen = strlen(ext);
    
    if (strnlen(src, extlen) < extlen)
        return NULL;
    
    if (!strncmp(src, ext, extlen))
        return (char*)src;
    
    return NULL;
}

char* StrStartCase(const char* src, const char* ext) {
    if (!strnicmp(src, ext, strlen(ext)))
        return (char*)src;
    
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
    return calloc(2, size );
}

void* Realloc(const void* data, s32 size) {
    return realloc((void*)data, size);
}

void* memdup(const void* src, Size size) {
    if (src == NULL)
        return NULL;
    
    return memcpy(Alloc(size), src, size);
}

char* Fmt(const char* fmt, ...) {
    char s[4096];
    s32 len;
    va_list va;
    
    va_start(va, fmt);
    Assert((len = vsnprintf(s, 4096, fmt, va)) < 4096);
    va_end(va);
    
    char* r = memdup(s, len + 1);
    Assert(r != NULL);
    
    return r;
}

s32 ArgStr(const char* args[], const char* arg) {
    
    for (s32 i = 1; args[i] != NULL; i++) {
        const char* this = args[i];
        
        if (this[0] != '-')
            continue;
        this += strspn(this, "-");
        if (!strcmp(this, arg))
            return i + 1;
    }
    
    return 0;
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

char* LineHead(const char* str, const char* head) {
    if (str == NULL) return NULL;
    
    for (s32 i = 0;; i--) {
        if (str[i - 1] == '\n' || &str[i] == head)
            return (char*)&str[i];
    }
}

static char* revstrchr (register const char* s, int c) {
    do {
        if (*s == c) {
            return (char*)s;
        }
    } while (*s--);
    
    return (0);
}

char* Line(const char* str, s32 line) {
    const char* ln = str;
    
    if (!str)
        return NULL;
    
    if (line) {
        if (line > 0) {
            while (line--) {
                ln = strchr(ln, '\n');
                
                if (!ln++)
                    return NULL;
                
                if (*ln == '\r')
                    ln++;
            }
        } else {
            while (line++) {
                ln = revstrchr(ln, '\n');
                
                if (!ln--)
                    return NULL;
            }
            
            if (ln)
                ln += 2;
        }
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
    return strcspn(str, "\n");
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
    const u32 m = strlen(src);
    
    for (s32 i = 0; i < m; i++) {
        if (src[i] == '/')
            dir++;
    }
    
    return dir + 1;
}

char* CopyLine(const char* str, s32 line) {
    if (!(str = Line(str, line))) return NULL;
    
    return x_strndup(str, LineLen(str));
}

char* CopyWord(const char* str, s32 word) {
    if (!(str = Word(str, word))) return NULL;
    
    return x_strndup(str, WordLen(str));
}

char* PathRel_From(const char* from, const char* item) {
    item = StrSlash(StrUnq(item));
    char* work = StrSlash(strdup(from));
    s32 lenCom = StrComLen(work, item);
    s32 subCnt = 0;
    char* sub = (char*)&work[lenCom];
    char* fol = (char*)&item[lenCom];
    char* buffer = x_alloc(strlen(work) + strlen(item));
    
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
    char* path = StrSlash(x_strdup(from));
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
        path = x_path(path);
    }
    
    return x_fmt("%s%s", path, f);
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

static char* StrConvert(const char* str, int (*tocase)(int)) {
    char* new = x_alloc(strlen(str) * 8);
    u32 write = 0;
    u32 len = strlen(str);
    u32 prev = 0;
    
    for (s32 i = 0; i < len; i++) {
        
        if (isalnum(str[i])) {
            prev = write;
            
            if (i != 0) {
                if (isalpha(str[i]) && isalpha(str[prev]))
                    if (isupper(str[i]) && !isupper(str[prev]))
                        new[write++] = '_';
            }
            
            new[write++] = tocase(str[i]);
        } else if (str[i] == ' ')
            new[write++] = '_';
    }
    
    return new;
}

char* Enumify(const char* str) {
    return StrConvert(str, toupper);
}

char* Canitize(const char* str) {
    return StrConvert(str, tolower);
}

// # # # # # # # # # # # # # # # # # # # #
// # COLOR                               #
// # # # # # # # # # # # # # # # # # # # #

static f32 hue2rgb(f32 p, f32 q, f32 t) {
    if (t < 0.0) t += 1;
    if (t > 1.0) t -= 1;
    if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
    if (t < 1.0 / 2.0) return q;
    if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
    
    return p;
}

HSL8 Color_GetHSL(f32 r, f32 g, f32 b) {
    HSL8 hsl;
    HSL8* dest = &hsl;
    f32 cmax, cmin, d;
    
    r /= 255;
    g /= 255;
    b /= 255;
    
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
    
    return hsl;
}

RGB8 Color_GetRGB8(f32 h, f32 s, f32 l) {
    RGB8 rgb = { };
    
    if (s == 0) {
        rgb.r = rgb.g = rgb.b = l * 255;
    } else {
        f32 q = l < 0.5f ? l * (1 + s) : l + s - l * s;
        f32 p = 2.0f * l - q;
        rgb.r = hue2rgb(p, q, h + 1.0f / 3.0f) * 255;
        rgb.g = hue2rgb(p, q, h) * 255;
        rgb.b = hue2rgb(p, q, h - 1.0f / 3.0f) * 255;
    }
    
    return rgb;
}

RGBA8 Color_GetRGBA8(f32 h, f32 s, f32 l) {
    RGBA8 rgb = { .a = 0xFF };
    RGB8* d = (void*)&rgb;
    
    *d = Color_GetRGB8(h, s, l);
    
    return rgb;
}

void Color_ToHSL(HSL8* dest, RGB8* src) {
    *dest = Color_GetHSL(UnfoldRGB(*src));
}

void Color_ToRGB(RGB8* dest, HSL8* src) {
    RGBA8 c = Color_GetRGBA8(src->h, src->s, src->l);
    
    dest->r = c.r;
    dest->g = c.g;
    dest->b = c.b;
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
    
    for (s32 i = 0;; i++) {
        if (ispunct(str[i]) || str[i] == '\0')
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
    
    for (s32 i = 0;; i++) {
        if (ispunct(str[i]) || str[i] == '\0')
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
    
    for (s32 i = 0;; i++) {
        if (ispunct(str[i]) || str[i] == '\0')
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
    
    return x_fmt("%s%d", sNoteName[note], (s32)floorf(octave));
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
    char* new = x_alloc(strlen(origin) + strlen(insert) + 2);
    
    strncpy(new, origin, pos);
    strcat(new, insert);
    strcat(new, &origin[pos]);
    
    strncpy(origin, new, size);
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
    char* new = x_strdup(str);
    
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
    const u32 m = strlen(b);
    
    for (; s < m; s++) {
        if (b[s] != a[s])
            return s;
    }
    
    return s;
}

char* String_GetSpacedArg(const char** args, s32 cur) {
    char tempBuf[512];
    s32 i = cur + 1;
    
    if (args[i] && args[i][0] != '-' && args[i][1] != '-') {
        strcpy(tempBuf, args[cur]);
        
        while (args[i] && args[i][0] != '-' && args[i][1] != '-') {
            strcat(tempBuf, " ");
            strcat(tempBuf, args[i++]);
        }
        
        return x_strdup(tempBuf);
    }
    
    return x_strdup(args[cur]);
}

void String_SwapExtension(char* dest, const char* src, const char* ext) {
    strcpy(dest, x_path(src));
    strcat(dest, x_basename(src));
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

bool ChrPool(const char c, const char* pool) {
    u32 m = strlen(pool);
    
    for (u32 i = 0; i < m; i++)
        if (c == pool[i])
            return true;
    
    return false;
}

bool StrPool(const char* s, const char* pool) {
    if (!s) return false;
    
    do {
        if (!ChrPool(*s, pool))
            return false;
    } while (*(++s) != '\0');
    
    return true;
}

static char* StrCatAlloc(const char** list, Size size, char* separator) {
    char* s = Alloc(size + 1);
    
    s[0] = '\0';
    
    for (s32 i = 0; list[i] != NULL; i++) {
        strcat(s, list[i]);
        
        if (list[i + 1])
            strcat(s, separator);
    }
    
    return s;
}

char* StrCatArg(const char** list, char separator) {
    char sep[2] = { separator, '\0' };
    u32 i = 0;
    u32 ln = 0;
    
    for (; list[i] != NULL; i++) {
        ln += strlen(list[i]);
    }
    ln += strlen(sep) * (i - 1);
    
    return StrCatAlloc(list, ln, sep);
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
        dst = x_alloc(len);
    
    for (Size indexSrc = 0; indexSrc < len; indexSrc++)
        dstIndex += EncodeU8(DecodeU16(src, len, &indexSrc), dst, dstIndex);
    
    return dst;
}

wchar* StrU16(wchar* dst, const char* src) {
    Size dstIndex = 0;
    Size len = strlen(src) + 1;
    
    if (!dst)
        dst = x_alloc(len * 3);
    
    for (Size srcIndex = 0; srcIndex < len; srcIndex++)
        dstIndex += EncodeU16(DecodeU8(src, len, &srcIndex), dst, dstIndex);
    
    return dst;
}

Size strwlen(const wchar* s) {
    Size len = 0;
    
    while (s[len] != 0)
        len++;
    
    return len;
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
    
    r = x_alloc(size + 1);
    memcpy(r, line, size);
    
    return r;
}

// # # # # # # # # # # # # # # # # # # # #
// # LOG                                 #
// # # # # # # # # # # # # # # # # # # # #

#include <signal.h>

#define FAULT_BUFFER_SIZE (1024)
#define FAULT_LOG_NUM     14

static char* sLogMsg[FAULT_LOG_NUM];
static char* sLogFunc[FAULT_LOG_NUM];
static u32 sLogLine[FAULT_LOG_NUM];
static vs32 sLogInit;
static vs32 sLogOutput = true;

void Log_NoOutput(void) {
    sLogOutput = false;
}

static void Log_Signal_PrintTitle(s32 arg, FILE* file) {
    const char* errorMsg[] = {
        "\a0",
        "\a1 - Hang Up",
        "\a2 - Interrupted",   // SIGINT
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
        
        "\aLog List",
    };
    
    if (gPrintfProgressing)
        fprintf(file, "\n");
    
    if (arg != 0xDEADBEEF)
        fprintf(file, "" PRNT_GRAY "[ " PRNT_REDD "%s " PRNT_GRAY "]\n", errorMsg[ClampMax(arg, 16)]);
    else
        fprintf(file, "" PRNT_GRAY "[ " PRNT_REDD "LOG " PRNT_GRAY "]\n");
    fprintf(file, "\n");
}

static void Log_Printinf(s32 arg, FILE* file) {
    
    for (s32 i = FAULT_LOG_NUM - 1, j = 0; i >= 0; i--, j++) {
        char* pfunc = j == 0 ? "__log_none__" : sLogFunc[i + 1];
        char fmt[16];
        
        snprintf(fmt, 16, "%d:", sLogLine[i]);
        if (strcmp(sLogFunc[i], pfunc))
            fprintf(file, "" PRNT_YELW "%s" PRNT_GRAY "();\n", sLogFunc[i]);
        fprintf(file, "" PRNT_GRAY "%-8s" PRNT_RSET "%s\n", fmt, sLogMsg[i]);
    }
    
    if (arg == 16)
        fprintf(file, "\n");
}

static void Log_Signal(s32 arg) {
    static volatile bool ran = 0;
    
    if (!sLogInit)
        return;
    if (ran) return;
    ran = true;
    sLogInit = false;
    gKillFlag = true;
    
    Log_Signal_PrintTitle(arg, stdlog);
    Log_Printinf(arg, stdlog);
    
#ifdef _WIN32
    printf_getchar("Press enter to exit");
#endif
    exit(arg);
}

void Log_Init() {
    if (sLogInit)
        return;
    for (s32 i = 1; i < 16; i++)
        signal(i, Log_Signal);
    for (s32 i = 0; i < FAULT_LOG_NUM; i++) {
        sLogMsg[i] = Calloc(FAULT_BUFFER_SIZE);
        sLogFunc[i] = Calloc(128);
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

void __Log(const char* func, u32 line, const char* txt, ...) {
    
#if 0
    va_list va;
    char buf[512];
    
    va_start(va, txt);
    vsnprintf(buf, 512, txt, va);
    printf_info("" PRNT_GRAY "[%s::%d]" PRNT_RSET " %s", func, line, buf);
    va_end(va);
    
    return;
#endif
    
    if (!sLogInit)
        return;
    
    if (sLogMsg[0] == NULL)
        return;
    
    Mutex_Lock();
    {
        ArrMoveR(sLogMsg, 0, FAULT_LOG_NUM);
        ArrMoveR(sLogFunc, 0, FAULT_LOG_NUM);
        ArrMoveR(sLogLine, 0, FAULT_LOG_NUM);
        
        va_list args;
        va_start(args, txt);
        vsnprintf(sLogMsg[0], FAULT_BUFFER_SIZE, txt, args);
        va_end(args);
        
        strcpy(sLogFunc[0], func);
        sLogLine[0] = line;
        
#if 0
        if (strcmp(sLogFunc[0], sLogFunc[1]))
            fprintf(stdlog, "" PRNT_REDD "%s" PRNT_GRAY "();\n", sLogFunc[0]);
        fprintf(stdlog, "" PRNT_GRAY "%-8d" PRNT_RSET "%s\n", sLogLine[0], sLogMsg[0]);
#endif
    }
    Mutex_Unlock();
}

// # # # # # # # # # # # # # # # # # # # #
// # PRINTF                              #
// # # # # # # # # # # # # # # # # # # # #

PrintfSuppressLevel gPrintfSuppress = 0;
u8 gPrintfProgressing;

void printf_SetSuppressLevel(PrintfSuppressLevel lvl) {
    gPrintfSuppress = lvl;
}

void printf_SetPrefix(char* fmt) {
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
    const char* color[4] = {
        PRNT_PRPL,
        PRNT_REDD,
        PRNT_REDD,
        PRNT_BLUE
    };
    
    fprintf(
        file,
        "" PRNT_GRAY "%s>" PRNT_GRAY ": " PRNT_RSET,
        color[type]
    );
}

void printf_warning(const char* fmt, ...) {
    if (gPrintfSuppress >= PSL_NO_WARNING)
        return;
    
    if (gKillFlag)
        return;
    
    if (gPrintfProgressing) {
        printf("\n");
        gPrintfProgressing = false;
    }
    
    va_list args;
    
    va_start(args, fmt);
    __printf_call(1, stdlog);
    vfprintf(
        stdlog,
        fmt,
        args
    );
    fputs("\n", stdlog);
    va_end(args);
}

void printf_warning_align(const char* info, const char* fmt, ...) {
    if (gPrintfSuppress >= PSL_NO_WARNING)
        return;
    
    if (gKillFlag)
        return;
    
    if (gPrintfProgressing) {
        printf("\n");
        gPrintfProgressing = false;
    }
    
    va_list args;
    
    va_start(args, fmt);
    __printf_call(1, stdout);
    fprintf(
        stdout,
        "%-16s " PRNT_RSET,
        info
    );
    vfprintf(
        stdout,
        fmt,
        args
    );
    fputs("\n", stdout);
    va_end(args);
}

void printf_MuteOutput(FILE* output) {
#ifdef _WIN32
    freopen ("NUL", "w", output);
#else
    freopen ("/dev/null", "w", output);
#endif
}

void printf_error(const char* fmt, ...) {
    gKillFlag = 1;
    
    printf_MuteOutput(stdout);
    
    if (gPrintfSuppress < PSL_NO_ERROR) {
        if (gPrintfProgressing) {
            fputs("\n", stdlog);
            gPrintfProgressing = false;
        }
        
        va_list args;
        
        va_start(args, fmt);
        
        __printf_call(2, stdlog);
        vfprintf(stdlog, fmt, args);
        fputs("\n\a", stdlog);
        fflush(stdlog);
        
        va_end(args);
    }
    
#ifdef _WIN32
    Terminal_GetChar();
#endif
    
    exit(EXIT_FAILURE);
}

void printf_error_align(const char* info, const char* fmt, ...) {
    gKillFlag = 1;
    
    printf_MuteOutput(stdout);
    
    if (gPrintfSuppress < PSL_NO_ERROR) {
        if (gPrintfProgressing) {
            fputs("\n", stdlog);
            gPrintfProgressing = false;
        }
        
        va_list args;
        
        va_start(args, fmt);
        __printf_call(2, stdlog);
        fprintf(
            stdlog,
            "%-16s " PRNT_RSET,
            info
        );
        vfprintf(
            stdlog,
            fmt,
            args
        );
        fputs("\n\a", stdlog);
        
        va_end(args);
    }
    
#ifdef _WIN32
    Terminal_GetChar();
#endif
    
    exit(EXIT_FAILURE);
}

void printf_info(const char* fmt, ...) {
    
    if (gKillFlag)
        return;
    
    if (gPrintfSuppress >= PSL_NO_INFO)
        return;
    
    if (gPrintfProgressing) {
        fputs("\n", stdout);
        gPrintfProgressing = false;
    }
    va_list args;
    
    va_start(args, fmt);
    __printf_call(3, stdout);
    vfprintf(
        stdout,
        fmt,
        args
    );
    fputs("\n", stdout);
    va_end(args);
}

void printf_info_align(const char* info, const char* fmt, ...) {
    if (gKillFlag)
        return;
    
    if (gPrintfSuppress >= PSL_NO_INFO)
        return;
    
    if (gPrintfProgressing) {
        fputs("\n", stdout);
        gPrintfProgressing = false;
    }
    va_list args;
    
    va_start(args, fmt);
    __printf_call(3, stdout);
    fprintf(
        stdout,
        "%-16s " PRNT_RSET,
        info
    );
    vfprintf(
        stdout,
        fmt,
        args
    );
    fputs("\n", stdout);
    va_end(args);
}

void printf_prog_align(const char* info, const char* fmt, const char* color) {
    if (gKillFlag)
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
    
    if (gKillFlag)
        return;
    
    printf(
        // "%-16s" PRNT_RSET "[%4d / %-4d]",
        x_fmt("\r" PRNT_BLUE ">" PRNT_GRAY ": " PRNT_RSET "%c-16s" PRNT_RSET "[ %c%dd / %c-%dd ]", '%', '%', Digits_Int(b), '%', Digits_Int(b)),
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
    
    if (gKillFlag)
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
        x_fmt("%c-16s" PRNT_RSET "[ %c%dd / %c-%dd ]", '%', '%', Digits_Int(b), '%', Digits_Int(b)),
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
    
    if (gKillFlag)
        return;
    
    va_start(va, fmt);
    Mutex_Lock();
    printf_WinFix();
    vprintf(fmt, va);
    fflush(NULL);
    Mutex_Unlock();
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
    
    if (gKillFlag)
        return;
    
    if (gPrintfSuppress >= PSL_NO_INFO)
        return;
    
    for (;; num--)
        if ((size + dispOffset) >> (num * 4))
            break;
    
    digit = x_fmt("" PRNT_GRAY "%c0%dX: " PRNT_RSET, '%', num + 1);
    
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
    
    if (gKillFlag)
        return;
    
    if (gPrintfSuppress >= PSL_NO_INFO)
        return;
    
    for (;; num--)
        if ((size + dispOffset) >> (num * 4))
            break;
    
    digit = x_fmt("" PRNT_GRAY "%c0%dX: " PRNT_RSET, '%', num + 1);
    
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
    if (gKillFlag)
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

f32 Math_Spline(f32 k, f32 xm1, f32 x0, f32 x1, f32 x2) {
    f32 a = (3.0f * (x0 - x1) - xm1 + x2) * 0.5f;
    f32 b = 2.0f * x1 + xm1 - (5.0f * x0 + x2) * 0.5f;
    f32 c = (x1 - xm1) * 0.5f;
    
    return (((((a * k) + b) * k) + c) * k) + x0;
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
    ThreadLocal static u8 SetEndOnNextBlockFlag = 0;
    
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

static Checksum Sha_ExtractDigest(u32* Hash) {
    Checksum Digest;
    
    for (u32 i = 0; i < 32; i += 4) {
        Digest.hash[i] = (u8)((Hash[i / 4] >> 24) & 0x000000FF);
        Digest.hash[i + 1] = (u8)((Hash[i / 4] >> 16) & 0x000000FF);
        Digest.hash[i + 2] = (u8)((Hash[i / 4] >> 8) & 0x000000FF);
        Digest.hash[i + 3] = (u8)(Hash[i / 4] & 0x000000FF);
    }
    
    return Digest;
}

Checksum Checksum_Get(const void* data, u64 size) {
    u32 W[64];
    u32 Hash[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    u64 RemainingDataSizeByte = size;
    Checksum dst;
    u8* d = (u8*)data;
    
    while (Sha_CreateCompleteScheduleArray(d, size, &RemainingDataSizeByte, W) == 1) {
        Sha_CompleteScheduleArray(W);
        Sha_Compression(Hash, W);
    }
    Sha_CompleteScheduleArray(W);
    Sha_Compression(Hash, W);
    
    dst = Sha_ExtractDigest(Hash);
    dst.data = d;
    dst.size = size;
    
    return dst;
}

bool Checksum_IsMatch(Checksum* a, Checksum* b) {
    return !memcmp(a->hash, b->hash, sizeof(a->hash));
}

#undef Free
void* Free(const void* data) {
    if (data != NULL)
        free((void*)data);
    
    return NULL;
}

// # # # # # # # # # # # # # # # # # # # #
// # QSort                               #
// # # # # # # # # # # # # # # # # # # # #

int QSortCallback_Str_NumHex(const void* ptrA, const void* ptrB) {
    const char* a = *((char**)ptrA);
    const char* b = *((char**)ptrB);
    
    if (!a || !b)
        return a ? 1 : b ? -1 : 0;
    
    if (!memcmp(a, "0x", 2) && !memcmp(b, "0x", 2)) {
        char* remainderA;
        char* remainderB;
        long valA = strtoul(a, &remainderA, 16);
        long valB = strtoul(b, &remainderB, 16);
        if (valA != valB)
            return valA - valB;
        
        else if (remainderB - b != remainderA - a)
            return (remainderB - b) - (remainderA - a);
        else
            return QSortCallback_Str_NumHex(&remainderA, &remainderB);
    }
    if (!memcmp(a, "0x", 2) || !memcmp(b, "0x", 2)) {
        return *a - *b;
    }
    if (isdigit(*a) && isdigit(*b)) {
        char* remainderA;
        char* remainderB;
        long valA = strtol(a, &remainderA, 10);
        long valB = strtol(b, &remainderB, 10);
        if (valA != valB)
            return valA - valB;
        
        else if (remainderB - b != remainderA - a)
            return (remainderB - b) - (remainderA - a);
        else
            return QSortCallback_Str_NumHex(&remainderA, &remainderB);
    }
    if (isdigit(*a) || isdigit(*b)) {
        return *a - *b;
    }
    while (*a && *b) {
        if (isdigit(*a) || isdigit(*b))
            return QSortCallback_Str_NumHex(&a, &b);
        if (tolower(*a) != tolower(*b))
            return tolower(*a) - tolower(*b);
        a++;
        b++;
    }
    return *a ? 1 : *b ? -1 : 0;
}
