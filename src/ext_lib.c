#pragma GCC diagnostic ignored "-Wunused-result"
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"

//crustify
#ifndef _WIN32
    #define _XOPEN_SOURCE 500
    #define _DEFAULT_SOURCE
#endif

#include "ext_lib.h"
#include <ftw.h>

#ifndef EXTLIB_PERMISSIVE
	#ifdef EXTLIB
		#if EXTLIB < THIS_EXTLIB_VERSION
			#warning ExtLib copy is newer than the project its used with
		#endif
	#endif
#endif

#ifdef _WIN32
    #include <windows.h>
#endif
//uncrustify

const f32 EPSILON = 0.0000001f;
f32 gDeltaTime = 1.0f;

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

typedef struct freelist_node_t {
    struct freelist_node_t* next;
    void* ptr;
    void (*dest)(void*);
} freelist_node_t;

static freelist_node_t* s_freelist_dest;
static freelist_node_t* s_freelist_temp;
static mutex_t s_freelist_mutex;

void* qxf(const void* ptr) {
    pthread_mutex_lock(&s_freelist_mutex); {
        freelist_node_t* node;
        
        node = calloc(sizeof(struct freelist_node_t));
        node->ptr = (void*)ptr;
        
        Node_Add(s_freelist_dest, node);
    } pthread_mutex_unlock(&s_freelist_mutex);
    
    return (void*)ptr;
}

void* FreeList_Que(void* ptr) {
    freelist_node_t* n = new(freelist_node_t);
    
    thd_lock();
    Node_Add(s_freelist_temp, n);
    thd_unlock();
    n->ptr = ptr;
    
    return ptr;
}

void* FreeList_QueCall(void* callback, void* ptr) {
    freelist_node_t* n = new(freelist_node_t);
    
    thd_lock();
    Node_Add(s_freelist_temp, n);
    thd_unlock();
    n->ptr = ptr;
    n->dest = callback;
    
    return ptr;
}

void FreeList_Free(void) {
    while (s_freelist_temp) {
        if (s_freelist_temp->dest)
            s_freelist_temp->dest(s_freelist_temp->ptr);
        else
            vfree(s_freelist_temp->ptr);
        
        Node_Kill(s_freelist_temp, s_freelist_temp);
    }
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

extern void _log_init();
extern void _log_dest();

const_func extlib_init(void) {
    _log_init();
    mutex_init(&gSegmentMutex);
    mutex_init(&gThreadMutex);
    
    {
        _log("bitfield assert:");
        u8 d[2] = { 0b00000001, 0b10000000 };
        
        _assert(bitfield_get(d, 7, 2) == 0b11);
        bitfield_set(d, 0, 7, 2);
        _assert(bitfield_get(d, 7, 2) == 0);
    }
    
    srand(sys_time());
    rand();
    
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    IO_FixWin32();
#endif
}

dest_func extlib_dest(void) {
    mutex_dest(&gSegmentMutex);
    mutex_dest(&gThreadMutex);
    
    while (s_freelist_dest) {
        vfree(s_freelist_dest->ptr);
        Node_Kill(s_freelist_dest, s_freelist_dest);
    }
    
    _log_dest();
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

void bswap(void* src, int size) {
    u32 buffer[64] = { 0 };
    u8* temp = (u8*)buffer;
    u8* srcp = src;
    
    for (int i = 0; i < size; i++) {
        temp[size - i - 1] = srcp[i];
    }
    
    for (int i = 0; i < size; i++) {
        srcp[i] = temp[i];
    }
}

void* alloc(int size) {
    return malloc(size);
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

// Calculate the sha256 digest of some data
// Author: Vitor Henrique Andrade Helfensteller Straggiotti Silva
// date_t: 26/06/2021 (DD/MM/YYYY)

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
    thread_local static u8 SetEndOnNextBlockFlag = 0;
    
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

static Hash Sha_ExtractDigest(u32* hash) {
    Hash Digest = { .hashed = true };
    
    for (u32 i = 0; i < 32; i += 4) {
        Digest.hash[i] = (u8)((hash[i / 4] >> 24) & 0x000000FF);
        Digest.hash[i + 1] = (u8)((hash[i / 4] >> 16) & 0x000000FF);
        Digest.hash[i + 2] = (u8)((hash[i / 4] >> 8) & 0x000000FF);
        Digest.hash[i + 3] = (u8)(hash[i / 4] & 0x000000FF);
    }
    
    return Digest;
}

Hash HashNew(void) {
    return (Hash) {};
}

Hash HashMem(const void* data, size_t size) {
    u32 W[64];
    u32 hash[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    u64 RemainingDataSizeByte = size;
    u8* d = (u8*)data;
    
    while (Sha_CreateCompleteScheduleArray(d, size, &RemainingDataSizeByte, W) == 1) {
        Sha_CompleteScheduleArray(W);
        Sha_Compression(hash, W);
    }
    Sha_CompleteScheduleArray(W);
    Sha_Compression(hash, W);
    
    Hash dst = Sha_ExtractDigest(hash);
    
    return dst;
}

Hash HashFile(const char* file) {
    Memfile mem = Memfile_New();
    
    Memfile_LoadBin(&mem, file);
    Hash h = HashMem(mem.data, mem.size);
    Memfile_Free(&mem);
    
    return h;
}

bool HashCmp(Hash* a, Hash* b) {
    return !!memcmp(a->hash, b->hash, sizeof(a->hash));
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

static u8** sSegment;
mutex_t gSegmentMutex;

void SegmentSet(u8 id, void* segment) {
    _log("%-4d%08X", id, segment);
    if (!sSegment) {
        sSegment = qxf(new(char*[255]));
        _assert(sSegment);
    }
    
    sSegment[id] = segment;
}

void* SegmentToVirtual(u8 id, u32 ptr) {
    if (sSegment[id] == NULL)
        errr("Segment 0x%X == NULL", id);
    
    return &sSegment[id][ptr];
}

u32 VirtualToSegment(u8 id, void* ptr) {
    return (uaddr_t)ptr - (uaddr_t)sSegment[id];
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

static struct {
    char*       head;
    size_t      offset;
    size_t      max;
    size_t      f;
    const char* header;
} gBufX = {
    .max    = 8000000,
    .f      =  100000,
    
    .header = "xbuf\xDE\xAD\xBE\xEF"
};

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

const_func x_init() {
    gBufX.head = calloc(gBufX.max);
}

dest_func x_dest() {
    vfree(gBufX.head);
}

void* x_alloc(size_t size) {
    static mutex_t xmutex;
    char* ret;
    
    if (size <= 0)
        return NULL;
    
    // if (size >= gBufX.f)
    //     errr("Biggest Failure");
    
    pthread_mutex_lock(&xmutex);
    if (gBufX.offset + size + 10 >= gBufX.max)
        gBufX.offset = 0;
    
    // Write Header
    ret = &gBufX.head[gBufX.offset];
    memcpy(ret, gBufX.header, 8);
    gBufX.offset += 8;
    
    // Write Data
    ret = &gBufX.head[gBufX.offset];
    gBufX.offset += size + 1;
    pthread_mutex_unlock(&xmutex);
    
    return memset(ret, 0, size + 1);
}

static void* m_alloc(size_t s) {
    void* addr = calloc(s);
    
    _assert(addr != NULL);
    
    return addr;
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

static void slash_n_point (const char* src, int* slash, int* point) {
    int strSize = strlen(src);
    
    *slash = 0;
    *point = 0;
    
    for (int i = strSize; i > 0; i--) {
        if (*point == 0 && src[i] == '.') {
            *point = i;
        }
        if (src[i] == '/' || src[i] == '\\') {
            *slash = i;
            break;
        }
    }
};

static char* __impl_memdup(void* (*falloc)(size_t), const char* data, size_t size) {
    if (!data || !size)
        return NULL;
    
    return memcpy(falloc(size), data, size);
}

static char* __impl_strdup(void* (*falloc)(size_t), const char* s) {
    return __impl_memdup(falloc, s, strlen(s) + 1);
}

static char* __impl_strndup(void* (*falloc)(size_t), const char* s, size_t n) {
    if (!n || !s) return NULL;
    size_t csz = strnlen(s, n);
    char* new = falloc(n + 1);
    char* res = memcpy (new, s, csz);
    
    return res;
}

static char* __impl_fmt(void* (*falloc)(size_t), const char* fmt, va_list va) {
    char buf[8192];
    int len;
    
    len = vsnprintf(buf, 8192, fmt, va);
    
    return __impl_memdup(falloc, buf, len + 1);
}

static char* __impl_rep(void* (*falloc)(size_t), const char* s, const char* a, const char* b) {
    char* r = falloc(strlen(s) * 4 + strlen(b) * 8);
    
    strcpy(r, s);
    strrep(r, a, b);
    
    return r;
}

static char* __impl_cpyline(void* (*falloc)(size_t), const char* s, size_t cpyline) {
    if (cpyline && !(s = strline(s, cpyline))) return NULL;
    
    return __impl_strndup(falloc, s, linelen(s));
}

static char* __impl_cpyword(void* (*falloc)(size_t), const char* s, size_t word) {
    if (word && !(s = strword(s, word))) return NULL;
    
    return __impl_strndup(falloc, s, wordlen(s));
}

static char* __impl_path(void* (*falloc)(size_t), const char* s) {
    char* buffer;
    int point;
    int slash;
    
    if (s == NULL)
        return NULL;
    
    slash_n_point(s, &slash, &point);
    
    if (slash == 0)
        slash = -1;
    
    buffer = falloc(slash + 1 + 1);
    memcpy(buffer, s, slash + 1);
    buffer[slash + 1] = '\0';
    
    return buffer;
}

static char* __impl_pathslot(char* (*fstrndup)(const char*, size_t), const char* src, int num) {
    const char* slot = src;
    
    if (src == NULL)
        return NULL;
    
    if (num < 0)
        num = dirnum(src) - 1;
    
    for (; num; num--) {
        slot = strchr(slot, '/');
        if (slot) slot++;
    }
    if (!slot) return NULL;
    
    int s = strcspn(slot, "/");
    char* r = fstrndup(slot, s ? s + 1 : 0);
    
    return r;
}

static inline const char* __impl_basename_last_slash(const char* s) {
    char* la = strrchr(s, '/');
    char* lb = strrchr(s, '\\');
    char* ls = (char*)Max((uaddr_t)la, (uaddr_t)lb);
    
    if (!ls++)
        ls = (char*)s;
    
    return ls;
}

static char* __impl_basename(void* (*falloc)(size_t), const char* s) {
    const char* ls = __impl_basename_last_slash(s);
    
    return __impl_strndup(falloc, ls, strcspn(ls, "."));
}

static char* __impl_filename(void* (*falloc)(size_t), const char* s) {
    const char* ls = __impl_basename_last_slash(s);
    
    return __impl_strdup(falloc, ls);
}

static char* __impl_randstr(void* (*falloc)(size_t), size_t size, const char* charset) {
    const char defcharset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const char* set = charset ? charset : defcharset;
    const int setz = (charset ? strlen(charset) : sizeof(defcharset)) - 1;
    char* str = falloc(size);
    
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

static char* __impl_strunq(void* (*falloc)(size_t), const char* s) {
    char* new = __impl_strdup(falloc, s);
    
    strrep(new, "\"", "");
    strrep(new, "'", "");
    
    return new;
}

static inline char* ifyize(char* new, const char* str, int (*tocase)(int)) {
    size_t w = 0;
    const size_t len = strlen(str);
    
    enum {
        NONE = 0,
        IS_LOWER,
        IS_UPPER,
        NONALNUM,
    } prev = NONE, this = NONE;
    
    for (size_t i = 0; i < len; i++) {
        const char chr = str[i];
        
        if (isalnum(chr)) {
            
            if (isupper(chr))
                this = IS_UPPER;
            else
                this = IS_LOWER;
            
            if (this == IS_UPPER && prev == IS_LOWER)
                new[w++] = '_';
            else if (prev == NONALNUM)
                new[w++] = '_';
            new[w++] = tocase(chr);
            
        } else
            this = NONALNUM;
        
        prev = this;
    }
    
    return new;
}

static char* __impl_enumify(void* (*falloc)(size_t), const char* s) {
    char* new = falloc(strlen(s) * 2);
    
    return ifyize(new, s, toupper);
}

static char* __impl_canitize(void* (*falloc)(size_t), const char* s) {
    char* new = falloc(strlen(s) * 2);
    
    return ifyize(new, s, tolower);
}

static char* __impl_dirrel_f(void* (*falloc)(size_t), const char* from, const char* item) {
    if (from[0] != item[0])
        return strflipslash(x_strdup(item));
    
    item = strflipslash(x_strunq(item));
    char* work = strflipslash(x_strdup(from));
    int lenCom = strstrlen(work, item);
    int subCnt = 0;
    char* sub = (char*)&work[lenCom];
    char* fol = (char*)&item[lenCom];
    char* buffer = falloc(strlen(work) + strlen(item));
    
    forstr(i, sub) {
        if (sub[i] == '/' || sub[i] == '\\')
            subCnt++;
    }
    
    for (int i = 0; i < subCnt; i++)
        strcat(buffer, "../");
    
    strcat(buffer, fol);
    
    return buffer;
}

static char* __impl_dirabs_f(char* (*ffmt)(const char*, ...), const char* from, const char* item) {
    item = strflipslash(x_strunq(item));
    char* path = strflipslash(x_strdup(from));
    char* t = strstr(item, "../");
    char* f = (char*)item;
    int subCnt = 0;
    
    while (t) {
        f = &f[strlen("../")];
        subCnt++;
        t = strstr(t + 1, "../");
    }
    
    for (int i = 0; i < subCnt; i++) {
        path[strlen(path) - 1] = '\0';
        path = x_path(path);
    }
    
    return ffmt("%s%s", path, f);
}

static char* __impl_dirrel(void* (*falloc)(size_t), const char* item) {
    return __impl_dirrel_f(falloc, sys_workdir(), item);
}

static char* __impl_dirabs(char* (*ffmt)(const char*, ...), const char* item) {
    return __impl_dirabs_f(ffmt, sys_workdir(), item);
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

char* x_strdup(const char* s) {
    return __impl_strdup(x_alloc, s);
}
char* x_strndup(const char* s, size_t n) {
    return __impl_strndup(x_alloc, s, n);
}
void* x_memdup(const void* d, size_t n) {
    return __impl_memdup(x_alloc, d, n);
}
char* x_fmt(const char* fmt, ...) {
    va_list va; va_start(va, fmt);
    char* r = __impl_fmt(x_alloc, fmt, va);
    
    va_end(va);
    return r;
}
char* x_rep(const char* s, const char* a, const char* b) {
    return __impl_rep(x_alloc, s, a, b);
}
char* x_cpyline(const char* s, size_t i) {
    return __impl_cpyline(x_alloc, s, i);
}
char* x_cpyword(const char* s, size_t i) {
    return __impl_cpyword(x_alloc, s, i);
}
char* x_path(const char* s) {
    return __impl_path(x_alloc, s);
}
char* x_pathslot(const char* s, int i) {
    return __impl_pathslot(x_strndup, s, i);
}
char* x_basename(const char* s) {
    return __impl_basename(x_alloc, s);
}
char* x_filename(const char* s) {
    return __impl_filename(x_alloc, s);
}
char* x_randstr(size_t size, const char* charset) {
    return __impl_randstr(x_alloc, size, charset);
}
char* x_strunq(const char* s) {
    return __impl_strunq(x_alloc, s);
}
char* x_enumify(const char* s) {
    return __impl_enumify(x_alloc, s);
}
char* x_canitize(const char* s) {
    return __impl_canitize(x_alloc, s);
}
char* x_dirrel_f(const char* from, const char* item) {
    return __impl_dirrel_f(x_alloc, from, item);
}
char* x_dirabs_f(const char* from, const char* item) {
    return __impl_dirabs_f(x_fmt, from, item);
}
char* x_dirrel( const char* item) {
    return __impl_dirrel(x_alloc, item);
}
char* x_dirabs(const char* item) {
    return __impl_dirabs(x_fmt, item);
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

char* strdup(const char* s) {
    return __impl_strdup(m_alloc, s);
}
char* strndup(const char* s, size_t n) {
    return __impl_strndup(m_alloc, s, n);
}
void* memdup(const void* d, size_t n) {
    return __impl_memdup(m_alloc, d, n);
}
char* fmt(const char* fmt, ...) {
    va_list va; va_start(va, fmt);
    char* r = __impl_fmt(m_alloc, fmt, va);
    
    va_end(va);
    return r;
}
char* rep(const char* s, const char* a, const char* b) {
    return __impl_rep(m_alloc, s, a, b);
}
char* cpyline(const char* s, size_t i) {
    return __impl_cpyline(m_alloc, s, i);
}
char* cpyword(const char* s, size_t i) {
    return __impl_cpyword(m_alloc, s, i);
}
char* path(const char* s) {
    return __impl_path(m_alloc, s);
}
char* pathslot(const char* s, int i) {
    return __impl_pathslot(strndup, s, i);
}
char* basename(const char* s) {
    return __impl_basename(m_alloc, s);
}
char* filename(const char* s) {
    return __impl_filename(m_alloc, s);
}
char* randstr(size_t size, const char* charset) {
    return __impl_randstr(m_alloc, size, charset);
}
char* strunq(const char* s) {
    return __impl_strunq(m_alloc, s);
}
char* enumify(const char* s) {
    return __impl_enumify(m_alloc, s);
}
char* canitize(const char* s) {
    return __impl_canitize(m_alloc, s);
}
char* dirrel_f(const char* from, const char* item) {
    return __impl_dirrel_f(m_alloc, from, item);
}
char* dirabs_f(const char* from, const char* item) {
    return __impl_dirabs_f(fmt, from, item);
}
char* dirrel( const char* item) {
    return __impl_dirrel(m_alloc, item);
}
char* dirabs(const char* item) {
    return __impl_dirabs(fmt, item);
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

int shex(const char* string) {
    return strtoul(string, NULL, 16);
}

int sint(const char* string) {
    if (strnlen(string, 3) >= 2 && !memcmp(string, "0x", 2)) {
        return strtoul(string, NULL, 16);
    } else {
        return strtol(string, NULL, 10);
    }
}

f32 sfloat(const char* string) {
    f32 fl;
    void* str = NULL;
    
    if (strstr(string, ",")) {
        string = strdup(string);
        str = (void*)string;
        strrep((void*)string, ",", ".");
    }
    
    fl = strtod(string, NULL);
    vfree(str);
    
    return fl;
}

int sbool(const char* string) {
    if (string == NULL)
        return -1;
    
    if (!stricmp(string, "true"))
        return true;
    else if (!stricmp(string, "false"))
        return false;
    
    return -1;
}

int vldt_hex(const char* str) {
    int isOk = false;
    
    for (int i = 0;; i++) {
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

int vldt_int(const char* str) {
    int isOk = false;
    
    for (int i = 0;; i++) {
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

int vldt_float(const char* str) {
    int isOk = false;
    
    for (int i = 0;; i++) {
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

int digint(int i) {
    int d = 0;
    
    for (; i; d++)
        i /= 10;
    
    return clamp_min(d, 1);
}

int digbit(int i) {
    int d = 0;
    
    for (; i; d++)
        i /= 2;
    
    return clamp_min(d, 1);
}

int valdig(int val, int digid) {
    
    for (; digid > 1; digid--)
        val /= 10;
    
    return val % 10;
}

int dighex(int i) {
    u32 d = 0;
    
    for (d = 7; d > 0 && !(i & (0xF << (d * 4)));)
        d--;
    
    return clamp_min(d + 1, 1);
}

int valhex(int val, int digid) {
    
    for (; digid > 1; digid--)
        val >>= 4;
    
    return val & 0xF;
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

static f32 hue2rgb(f32 p, f32 q, f32 t) {
    if (t < 0.0) t += 1;
    if (t > 1.0) t -= 1;
    if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
    if (t < 1.0 / 2.0) return q;
    if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
    
    return p;
}

hsl_t Color_hsl(f32 r, f32 g, f32 b) {
    hsl_t hsl;
    hsl_t* dest = &hsl;
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

rgb8_t Color_rgb8(f32 h, f32 s, f32 l) {
    rgb8_t rgb = { };
    
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

rgba8_t Color_rgba8(f32 h, f32 s, f32 l) {
    rgba8_t rgb = { .a = 0xFF };
    rgb8_t* d = (void*)&rgb;
    
    *d = Color_rgb8(h, s, l);
    
    return rgb;
}

void Color_Convert2hsl(hsl_t* dest, rgb8_t* src) {
    *dest = Color_hsl(unfold_rgb(*src));
}

void Color_Convert2rgb(rgb8_t* dest, hsl_t* src) {
    rgba8_t c = Color_rgba8(src->h, src->s, src->l);
    
    dest->r = c.r;
    dest->g = c.g;
    dest->b = c.b;
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

static const char* sNoteName[12] = {
    "C", "C#",
    "D", "D#",
    "E",
    "F", "F#",
    "G", "G#",
    "A", "A#",
    "B",
};

int Note_Index(const char* note) {
    int id = 0;
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
    
    octave = 12 * (sint(note));
    
    return id + octave;
}

const char* Note_Name(int note) {
    f32 octave = (f32)note / 12;
    
    note %= 12;
    
    return x_fmt("%s%d", sNoteName[note], (int)floorf(octave));
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

f32 randf() {
    f64 r = rand() / (f64)RAND_MAX;
    
    return r;
}

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

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

void* memmem(const void* hay, size_t haylen, const void* nee, size_t neelen) {
    char* bf = (char*) hay, * pt = (char*) nee, * p = bf;
    
    if (haylen < neelen || !bf || !pt || !haylen || !neelen)
        return NULL;
    
    while ((haylen - (p - bf)) >= neelen) {
        if (NULL != (p = memchr(p, *pt, haylen - (p - bf)))) {
            if (!memcmp(p, nee, neelen))
                return p;
            ++p;
        } else
            break;
    }
    
    return NULL;
}

void* memmem_align(u32 val, const void* haystack, size_t haystacklen, const void* needle, size_t needlelen) {
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

char* stristr(const char* haystack, const char* needle) {
    char* bf = (char*) haystack, * pt = (char*) needle, * p = bf;
    
    if (!haystack || !needle)
        return NULL;
    
    u32 haystacklen = strlen(haystack);
    u32 needlelen = strlen(needle);
    
    while (needlelen <= (haystacklen - (p - bf))) {
        char* a, * b;
        
        a = memchr(p, tolower((int)(*pt)), haystacklen - (p - bf));
        b = memchr(p, toupper((int)(*pt)), haystacklen - (p - bf));
        
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

char* strwstr(const char* hay, const char* nee) {
    char* p = strstr(hay, nee);
    
    while (p) {
        if (!isgraph(p[-1]) && !isgraph(p[strlen(nee)]))
            return p;
        
        p = strstr(p + 1, nee);
    }
    
    return NULL;
}

char* strnstr(const char* hay, const char* nee, size_t n) {
    return memmem(hay, strnlen(hay, n), nee, strlen(nee));
}

char* strend(const char* src, const char* ext) {
    const size_t xlen = strlen(ext);
    const size_t slen = strlen(src);
    char* fP;
    
    if (slen < xlen) return NULL;
    fP = (char*)(src + slen - xlen);
    if (!strcmp(fP, ext)) return fP;
    
    return NULL;
}

char* striend(const char* src, const char* ext) {
    char* fP;
    const int xlen = strlen(ext);
    const int slen = strlen(src);
    
    if (slen < xlen) return NULL;
    fP = (char*)(src + slen - xlen);
    if (!stricmp(fP, ext)) return fP;
    
    return NULL;
}

char* strstart(const char* src, const char* ext) {
    size_t extlen = strlen(ext);
    
    if (strnlen(src, extlen) < extlen)
        return NULL;
    
    if (!strncmp(src, ext, extlen))
        return (char*)src;
    
    return NULL;
}

char* stristart(const char* src, const char* ext) {
    if (!strnicmp(src, ext, strlen(ext)))
        return (char*)src;
    
    return NULL;
}

int strarg(const char* args[], const char* arg) {
    
    for (int i = 1; args[i] != NULL; i++) {
        const char* this = args[i];
        
        if (this[0] != '-')
            continue;
        this += strspn(this, "-");
        if (!strcmp(this, arg))
            return i + 1;
    }
    
    return 0;
}

char* strracpt(const char* str, const char* c) {
    char* v = (char*)str;
    u32 an = strlen(c);
    
    if (!v) return NULL;
    else v = strchr(str, '\0');
    
    while (--v >= str)
        for (int i = 0; i < an; i++)
            if (*v == c[i])
                return v;
    
    return NULL;
}

char* linehead(const char* str, const char* head) {
    if (str == NULL) return NULL;
    
    for (int i = 0;; i--) {
        if (str[i - 1] == '\n' || &str[i] == head)
            return (char*)&str[i];
    }
}

char* strline(const char* str, int line) {
    const char* ln = str;
    
    nested(char*, revstrchr, (const char* s, int c)) {
        do {
            if (*s == c) {
                return (char*)s;
            }
        } while (*s--);
        
        return (0);
    };
    
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

char* strword(const char* str, int word) {
    if (!str)
        return NULL;
    
    while (!isgraph(*str)) str++;
    while (word--) {
        if (!(str = strpbrk(str, " \t\n\r"))) return NULL;
        while (!isgraph(*str)) str++;
    }
    
    return (char*)str;
}

size_t linelen(const char* str) {
    return strcspn(str, "\n\r");
}

size_t wordlen(const char* str) {
    return strcspn(str, " \t\n\r");
}

bool chrspn(int c, const char* accept) {
    for (; *accept; accept++)
        if (c == *accept)
            return true;
    return false;
}

bool strconsist(const char* str, const char* accept) {
    for (; *str; str++)
        if (!chrspn(*str, accept))
            return false;
    return true;
}

static int __impl_strnocc(const char* s, int n, const char* accept) {
    int c = 0;
    
    for (; n && *s; n--, s++)
        if (chrspn(*s, accept))
            c++;
    return c;
}

int strnocc(const char* s, size_t n, const char* accept) {
    return __impl_strnocc(s, Max(n, 0), accept);
}

int strocc(const char* s, const char* accept) {
    return __impl_strnocc(s, -1, accept);
}

static int __impl_memnins(char* dst, size_t __dst_size, const char* src, size_t src_size, int pos, size_t __max_size) {
    int dst_size = __dst_size - pos;
    int max_size = __max_size - pos;
    
    if (dst_size <= 0 || max_size <= 0)
        return 0;
    
    dst += pos;
    
    // Check if there is room to move
    if (src_size == clamp_max(src_size, max_size)) {
        int move_size = clamp_max(src_size + dst_size, max_size) - src_size;
        
        memmove(dst + src_size, dst, move_size);
    }
    
    src_size = clamp_max(src_size, max_size);
    memcpy(dst, src, src_size);
    
    return src_size;
}

int strnins(char* dst, const char* src, size_t pos, int n) {
    return __impl_memnins(dst, strlen(dst) + 1, src, strlen(src), pos, n - 1);
}

int strins(char* dst, const char* src, size_t pos) {
    return __impl_memnins(dst, strlen(dst) + 1, src, strlen(src), pos, __INT_MAX__);
}

int strinsat(char* str, const char* ins) {
    return __impl_memnins(str, strlen(str) + 1, ins, strlen(ins), 0, __INT_MAX__);
}

static int __impl_strnrep(char* str, int len, char* (*__strstr)(const char*, const char*), const char* mtch, const char* rep) {
    char* o = str;
    int mtch_len = strlen(mtch);
    int rep_len = strlen(rep);
    int max_len;
    int nlen;
    
    if (!str || !*str)
        return 0;
    
    switch (len) {
        case 0:
            max_len = 0;
            nlen = len = strlen(str);
            break;
            
        default:
            max_len = len;
            nlen = len = strnlen(str, max_len);
            break;
    }
    
    while ( (str = __strstr(str, mtch)) ) {
        int point = (int)((uaddr_t)str - (uaddr_t)o);
        int mve_len = nlen - (point + mtch_len);
        int oins_len = rep_len;
        int ins_len = rep_len;
        
        if (max_len) {
            oins_len = ins_len;
            
            mve_len = clamp(mve_len + point, 0, max_len) - point;
            ins_len = clamp(ins_len + point, 0, max_len) - point;
        }
        
        if (oins_len == ins_len && mve_len > 0)
            memmove(str + rep_len, str + mtch_len, mve_len );
        
        if (ins_len > 0)
            memcpy(str, rep, ins_len);
        
        nlen += (ins_len - mtch_len);
        if (max_len) nlen = clamp_max(nlen, max_len);
        o[nlen] = '\0';
        
        if (max_len) {
            if (oins_len != ins_len)
                break;
        }
        
        str += ins_len;
    }
    
    return nlen - len;
}

int strrep(char* src, const char* mtch, const char* rep) {
    return __impl_strnrep(src, 0, strstr, mtch, rep);
}

int strnrep(char* src, int len, const char* mtch, const char* rep) {
    return __impl_strnrep(src, len - 1, strstr, mtch, rep);
}

int strwrep(char* src, const char* mtch, const char* rep) {
    return __impl_strnrep(src, 0, strwstr, mtch, rep);
}

int strwnrep(char* src, int len, const char* mtch, const char* rep) {
    return __impl_strnrep(src, len - 1, strwstr, mtch, rep);
}

char* strfext(const char* str) {
    int slash;
    int point;
    
    slash_n_point(str, &slash, &point);
    
    return (void*)&str[point];
}

char* strtoup(char* str) {
    forstr(i, str)
    str[i] = toupper(str[i]);
    
    return str;
}

char* strtolo(char* str) {
    forstr(i, str)
    str[i] = tolower(str[i]);
    
    return str;
}

void strntolo(char* s, int i) {
    if (i <= 0)
        i = strlen(s);
    
    for (int k = 0; k < i; k++) {
        if (s[k] >= 'A' && s[k] <= 'Z') {
            s[k] = s[k] + 32;
        }
    }
}

void strntoup(char* s, int i) {
    if (i <= 0)
        i = strlen(s);
    
    for (int k = 0; k < i; k++) {
        if (s[k] >= 'a' && s[k] <= 'z') {
            s[k] = s[k] - 32;
        }
    }
}

void strrem(char* point, int amount) {
    char* get = point + amount;
    int len = strlen(get);
    
    if (len)
        memcpy(point, get, strlen(get));
    point[len] = 0;
}

char* strflipslash(char* t) {
    strrep(t, "\\", "/");
    
    return t;
}

char* strfssani(char* t) {
    strrep(t, "\\", "");
    strrep(t, "/", "");
    strrep(t, ":", "");
    strrep(t, "*", "");
    strrep(t, "?", "");
    strrep(t, "\"", "");
    strrep(t, "<", "");
    strrep(t, ">", "");
    strrep(t, "|", "");
    
    return t;
}

int strstrlen(const char* a, const char* b) {
    int s = 0;
    const u32 m = strlen(b);
    
    for (; s < m; s++) {
        if (b[s] != a[s])
            return s;
    }
    
    return s;
}

__attribute__ ((deprecated))
char* String_GetSpacedArg(const char** args, int cur) {
    char tempBuf[512];
    int i = cur + 1;
    
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

void strswapext(char* dest, const char* src, const char* ext) {
    strcpy(dest, x_path(src));
    strcat(dest, x_basename(src));
    strcat(dest, ext);
}

static char* __impl_strarrcat(const char** list, size_t size, const char* separator) {
    char* s = alloc(size + 1);
    
    s[0] = '\0';
    
    for (int i = 0; list[i] != NULL; i++) {
        strcat(s, list[i]);
        
        if (list[i + 1])
            strcat(s, separator);
    }
    
    return s;
}

char* strarrcat(const char** list, const char* separator) {
    u32 i = 0;
    u32 ln = 0;
    
    for (; list[i] != NULL; i++) {
        ln += strlen(list[i]);
    }
    ln += strlen(separator) * (i - 1);
    
    return __impl_strarrcat(list, ln, separator);
}

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

static int __wcharconv_calclen8(u32 codepoint) {
    if (codepoint <= UTF8_1_MAX)
        return 1;
    
    if (codepoint <= UTF8_2_MAX)
        return 2;
    
    if (codepoint <= UTF8_3_MAX)
        return 3;
    
    return 4;
}

static size_t __wcharconv_encode8(u32 codepoint, char* utf8, size_t index) {
    int size = __wcharconv_calclen8(codepoint);
    
    // Write the continuation bytes in reverse order first
    for (int cont_index = size - 1; cont_index > 0; cont_index--) {
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

static size_t __wcharconv_encode16(u32 codepoint, wchar* utf16, size_t index) {
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

static u32 __wcharconv_decode8(const char* utf8, size_t len, size_t* index) {
    u8 leading = utf8[*index];
    int encoding_len = 0;
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
    
    for (int i = 0; i < encoding_len - 1; i++) {
        if (*index + 1 >= len)
            return INVALID_CODEPOINT;
        
        u8 continuation = utf8[*index + 1];
        if ((continuation & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_VALUE)
            return INVALID_CODEPOINT;
        
        codepoint <<= UTF8_CONTINUATION_CODEPOINT_BITS;
        codepoint |= continuation & ~UTF8_CONTINUATION_MASK;
        
        (*index)++;
    }
    
    int proper_len = __wcharconv_calclen8(codepoint);
    
    if (proper_len != encoding_len)
        return INVALID_CODEPOINT;
    if (codepoint < BMP_END && (codepoint & GENERIC_SURROGATE_MASK) == GENERIC_SURROGATE_VALUE)
        return INVALID_CODEPOINT;
    if (codepoint > UNICODE_MAX)
        return INVALID_CODEPOINT;
    
    return codepoint;
}

static u32 __wcharconv_decode16(const wchar* utf16, size_t len, size_t* index) {
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

size_t strwlen(const wchar* s) {
    size_t len = 0;
    
    while (s[len] != 0)
        len++;
    
    return len;
}

size_t str8nlen(const char* str, size_t n) {
    const char* t = str;
    size_t length = 0;
    
    while ((size_t)(str - t) < n && '\0' != *str) {
        if (0xf0 == (0xf8 & *str))
            str += 4;
        else if (0xe0 == (0xf0 & *str))
            str += 3;
        else if (0xc0 == (0xe0 & *str))
            str += 2;
        else
            str += 1;
        
        length++;
    }
    
    if ((size_t)(str - t) > n)
        length--;
    return length;
}

size_t str8len(const char* str) {
    return str8nlen(str, SIZE_MAX);
}

size_t strvnlen(const char* str, size_t n) {
    const char* t = str;
    size_t length = 0;
    
    while ((size_t)(str - t) < n && '\0' != *str) {
        
        if (0xf0 == (0xf8 & *str))
            str += 3;
        else if (0xe0 == (0xf0 & *str))
            str += 2;
        else if (0xc0 == (0xe0 & *str))
            str += 2;
        else {
            if (*str == '\e') {
                while (*str != 'm') str++;
                str += 1;
                continue;
            }
            str += 1;
        }
        
        length++;
    }
    
    if ((size_t)(str - t) > n)
        length--;
    return length;
}

size_t strvlen(const char* str) {
    return strvnlen(str, SIZE_MAX);
}

bool streq(const char* a, const char* b) {
    while (true) {
        if (*a != *b) return false;
        if (!*a) return true;
        a++, b++;
    }
}

bool strieq(const char* a, const char* b) {
    while (true) {
        if (tolower(*a) != tolower(*b)) return false;
        if (!*a) return true;
        a++, b++;
    }
}

char* strto8(char* dst, const wchar* src) {
    size_t dstIndex = 0;
    size_t len = strwlen(src) + 1;
    
    if (!dst)
        dst = x_alloc(len);
    
    for (size_t indexSrc = 0; indexSrc < len; indexSrc++)
        dstIndex += __wcharconv_encode8(__wcharconv_decode16(src, len, &indexSrc), dst, dstIndex);
    
    return dst;
}

wchar* strto16(wchar* dst, const char* src) {
    size_t dstIndex = 0;
    size_t len = strlen(src) + 1;
    
    if (!dst)
        dst = x_alloc(len * 3);
    
    for (size_t srcIndex = 0; srcIndex < len; srcIndex++)
        dstIndex += __wcharconv_encode16(__wcharconv_decode8(src, len, &srcIndex), dst, dstIndex);
    
    return dst;
}

int linenum(const char* str) {
    int num = 1;
    
    while (*str != '\0') {
        if (*str == '\n' || *str == '\r')
            num++;
        str++;
    }
    
    return num;
}

int dirnum(const char* src) {
    int dir = -1;
    const u32 m = strlen(src);
    
    for (int i = 0; i < m; i++) {
        if (src[i] == '/')
            dir++;
    }
    
    return dir + 1;
}

int dir_isabs(const char* item) {
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

int dir_isrel(const char* item) {
    return !dir_isabs(item);
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

#define _bitnummask(bitNum)          ((1 << (bitNum)) - 1)
#define _bitsizemask(size)           _bitnummask(size * 8)
#define _bitfield(data, shift, size) (((data) >> (shift))&_bitnummask(size))

u32 bitfield_get(const void* data, int shift, int size) {
    const u8* b = data;
    u32 r = 0;
    int szm = size - 1;
    
    for (int i = 0; i < size; i++) {
        int id = (shift + i);
        int sf = id % 8;
        
        id -= sf;
        id /= 8;
        sf = 7 - sf;
        
        r |= ((b[id] >> sf) & 1) << (szm - i);
    }
    
    return r;
}

void bitfield_set(void* data, u32 val, int shift, int size) {
    u8* b = data;
    int szm = size - 1;
    
    for (int i = 0; i < size; i++) {
        int id = (shift + i);
        int sf = id % 8;
        
        id -= sf;
        id /= 8;
        sf = 7 - sf;
        
        b[id] &= ~( 1 << sf );
        b[id] |= ((val >> (szm - i)) & 1) << sf;
    }
}

int bitfield_lzeronum(u32 v) {
    return __builtin_ctz(v);
}

int bitfield_num(int val) {
    return __builtin_popcount(val);
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

bool sys_isdir(const char* path) {
    struct stat st = { 0 };
    
    stat(path, &st);
    if (S_ISDIR(st.st_mode))
        return true;
    
    if (strend(path, "/")) {
        char* tmp = x_strdup(path);
        strend(tmp, "/")[0] = '\0';
        
        stat(tmp, &st);
        
        if (S_ISDIR(st.st_mode))
            return true;
    }
    
    return false;
}

time_t sys_stat(const char* item) {
    struct stat st = { 0 };
    time_t t = 0;
    int f = 0;
    
    if (item == NULL)
        return 0;
    
    if (strend(item, "/") || strend(item, "\\")) {
        f = 1;
        item = strdup(item);
        ((char*)item)[strlen(item) - 1] = 0;
    }
    
    if (stat(item, &st) == -1) {
        if (f) vfree(item);
        
        return 0;
    }
    
    // No access time
    // t = Max(st.st_atime, t);
    t = Max(st.st_mtime, t);
    t = Max(st.st_ctime, t);
    
    if (f) vfree(item);
    
    return t;
}

const char* sys_app(void) {
    static char buf[512];
    
#ifdef _WIN32
    GetModuleFileName(NULL, buf, 512);
#else
    (void)readlink("/proc/self/exe", buf, 512);
#endif
    
    return buf;
}

const char* sys_appdata(void) {
    const char* app = x_basename(sys_app());
    static char buf[512];
    
    snprintf(buf, 512, "%s/%s/", sys_env(ENV_APPDATA), app);
    sys_mkdir("%s", buf);
    
    return buf;
}

time_t sys_time(void) {
    time_t tme;
    
    time(&tme);
    
    return tme;
}

void sys_sleep(f64 sec) {
    struct timespec ts = { 0 };
    
    if (sec <= 0)
        return;
    
    ts.tv_sec = floor(sec);
    ts.tv_nsec = (sec - floor(sec)) * 1000000000;
    
    nanosleep(&ts, NULL);
}

static void __impl_makedir_recurse(const char* s) {
    char tmp[261];
    char* p = NULL;
    size_t len;
    
    snprintf(tmp, sizeof(tmp), "%s", s);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
        if (*p == '/') {
            *p = 0;
            if (!sys_stat(tmp))
                mkdir(
                    tmp
#ifndef _WIN32
                    ,
                    S_IRWXU
#endif
                );
            *p = '/';
        }
    if (!sys_stat(tmp))
        mkdir(
            tmp
#ifndef _WIN32
            ,
            S_IRWXU
#endif
        );
}

static void __impl_makedir(const char* s) {
    if (sys_stat(s))
        return;
    
    __impl_makedir_recurse(s);
    if (!sys_stat(s))
        _log("mkdir error: [%s]", s);
}

void sys_mkdir(const char* dir, ...) {
    char buffer[261];
    va_list args;
    
    va_start(args, dir);
    vsnprintf(buffer, 261, dir, args);
    va_end(args);
    
    const int m = strlen(buffer);
    
#ifdef _WIN32
    int i = 0;
    if (dir_isabs(buffer))
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
                errr("MakeDir: Can't make folder with illegal character! '%s'", buffer);
                break;
            default:
                break;
        }
    }
#endif
    
    if (!sys_isdir(dir)) {
        for (int i = m - 1; i >= 0; i--) {
            if (buffer[i] == '/' || buffer[i] == '\\')
                break;
            buffer[i] = '\0';
        }
    }
    
    __impl_makedir(buffer);
}

const char* sys_workdir(void) {
    static char buf[512];
    
    if (getcwd(buf, sizeof(buf)) == NULL)
        errr("Could not get sys_workdir");
    
    const u32 m = strlen(buf);
    for (int i = 0; i < m; i++) {
        if (buf[i] == '\\')
            buf[i] = '/';
    }
    
    strcat(buf, "/");
    
    return buf;
}

const char* sys_appdir(void) {
    static char* appdir;
    
    if (!appdir) {
        appdir = qxf(strndup(x_path(sys_app()), 512));
        strrep(appdir, "\\", "/");
        
        if (!strend(appdir, "/"))
            strcat(appdir, "/");
    }
    
    return appdir;
}

int sys_mv(const char* input, const char* output) {
    if (sys_stat(output))
        sys_rm(output);
    
    return rename(input, output);
}

static int __impl_rmdir(const char* item, const struct stat* bug, int type, struct FTW* ftw) {
    if (sys_rm(item))
        errr_align("rm failed:", "%s", item);
    
    return 0;
}

int sys_rm(const char* item) {
    if (sys_isdir(item))
        return rmdir(item);
    else
        return remove(item);
}

int sys_rmdir(const char* item) {
    if (!sys_isdir(item))
        return 1;
    if (!sys_stat(item))
        return 0;
    if (nftw(item, __impl_rmdir, 10, FTW_DEPTH | FTW_MOUNT | FTW_PHYS))
        errr("sys_rmdir: %s", item);
    
    return 0;
}

void sys_setworkdir(const char* txt) {
    if (dir_isrel(txt))
        txt = x_dirabs(txt);
    
    chdir(txt);
}

int sys_touch(const char* file) {
    if (!sys_stat(file)) {
        FILE* f = fopen(file, "w");
        _assert(f != NULL);
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

int sys_cp(const char* src, const char* dest) {
    if (sys_isdir(src)) {
        List list = List_New();
        
        List_Walk(&list, src, -1, LIST_FILES);
        
        for (int i = 0; i < list.num; i++) {
            char* dfile = x_fmt(
                "%s%s%s",
                dest,
                strend(dest, "/") ? "" : "/",
                list.item[i] + strlen(src)
            );
            
            _log("Copy: %s -> %s", list.item[i], dfile);
            
            sys_mkdir(x_path(dfile));
            if (sys_cp(list.item[i], dfile))
                return 1;
        }
        
        List_Free(&list);
    } else {
        Memfile a = Memfile_New();
        
        a.param.throwError = false;
        
        if (Memfile_LoadBin(&a, src))
            return -1;
        if (Memfile_SaveBin(&a, dest))
            return 1;
        Memfile_Free(&a);
    }
    
    return 0;
}

date_t sys_timedate(time_t time) {
    date_t date = { 0 };
    
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

int sys_getcorenum(void) {
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

size_t sys_statsize(const char* file) {
    FILE* f = fopen(file, "r");
    u32 result = -1;
    
    if (!f) return result;
    
    fseek(f, 0, SEEK_END);
    result = ftell(f);
    fclose(f);
    
    return result;
}

const char* sys_env(env_index_t env) {
    switch (env) {
#ifdef _WIN32
        case ENV_USERNAME:
            return x_strdup(getenv("USERNAME"));
        case ENV_APPDATA:
            return strflipslash(x_strdup(getenv("APPDATA")));
        case ENV_HOME:
            return strflipslash(x_strdup(getenv("USERPROFILE")));
        case ENV_TEMP:
            return strflipslash(x_strdup(getenv("TMP")));
            
#else // UNIX
        case ENV_USERNAME:
            return x_strdup(getenv("USER"));
        case ENV_APPDATA:
            return x_fmt("/home/%s/.local", getenv("USER"));
        case ENV_HOME:
            return x_strdup(getenv("HOME"));
        case ENV_TEMP:
            return x_strdup("/tmp");
            
#endif
    }
    
    return NULL;
}

#include <dirent.h>

int sys_emptydir(const char* path) {
    DIR* dir;
    struct dirent* entry;
    int is_empty = true;
    
    dir = opendir(path);
    
    if (dir == NULL) {
        perror("opendir");
        return false;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
            is_empty = false;
            
            break;
        }
    }
    
    if (closedir(dir) == -1) {
        perror("closedir");
        is_empty = false;
    }
    
    return is_empty;
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

static int sSysIgnore;
static int sSysReturn;

static const char* sys_exe_s(const char* str) {
#ifdef _WIN32
    char* n = x_strdup(str);
    char* e = strstr(n, ".exe");
    
    _assert(e != NULL);
    for (; (uaddr_t)e >= (uaddr_t)n; e--)
        if (*e == '/') *e = '\\';
    
    return n;
#endif
    return str;
}

int sys_exe(const char* cmd) {
    int ret;
    
    ret = system(sys_exe_s(cmd));
    
    if (ret != 0)
        _log(PRNT_REDD "[%d] " PRNT_GRAY "sys_exe(" PRNT_REDD "%s" PRNT_GRAY ");", ret, cmd);
    
    return ret;
}

void sys_exed(const char* cmd) {
#ifdef _WIN32
    sys_exe(x_fmt("start %s", sys_exe_s(cmd)));
#else
    sys_exe(x_fmt("nohup %s", cmd));
#endif
}

int sys_exel(const char* cmd, int (*callback)(void*, const char*), void* arg) {
    char* s;
    FILE* file;
    
    if ((file = popen(sys_exe_s(cmd), "r")) == NULL) {
        _log(PRNT_REDD "sys_exes(%s);", cmd);
        _log("popen failed!");
        
        return -1;
    }
    
    s = alloc(1025);
    while (fgets(s, 1024, file))
        if (callback)
            if (callback(arg, s))
                break;
    vfree(s);
    
    return pclose(file);
}

void sys_exes_noerr() {
    sSysIgnore = true;
}

int sys_exes_return() {
    return sSysReturn;
}

char* sys_exes(const char* cmd) {
    char* sExeBuffer;
    char result[1025];
    FILE* file = NULL;
    u32 size = 0;
    
    if ((file = popen(sys_exe_s(cmd), "r")) == NULL) {
        _log(PRNT_REDD "sys_exes(%s);", cmd);
        _log("popen failed!");
        
        return NULL;
    }
    
    sExeBuffer = alloc(MbToBin(4));
    _assert(sExeBuffer != NULL);
    sExeBuffer[0] = '\0';
    
    while (fgets(result, 1024, file)) {
        size += strlen(result);
        _assert (size < MbToBin(4));
        strcat(sExeBuffer, result);
    }
    
    if ((sSysReturn = pclose(file)) != 0) {
        if (sSysIgnore == 0) {
            printf("%s\n", sExeBuffer);
            _log(PRNT_REDD "[%d] " PRNT_GRAY "sys_exes(" PRNT_REDD "%s" PRNT_GRAY ");", sSysReturn, cmd);
            errr("sys_exes [%s]", sys_exe_s(cmd));
        }
    }
    
    return sExeBuffer;
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

thread_local struct {
    bool makeDir;
    char path[261];
} sFileSys;

void fs_mkflag(bool flag) {
    sFileSys.makeDir = flag;
}

void fs_set(const char* fmt, ...) {
    va_list va;
    
    va_start(va, fmt);
    vsnprintf(sFileSys.path, 261, fmt, va);
    va_end(va);
    
    _log("Path [%s]", sFileSys.path);
    
    if (sFileSys.makeDir)
        sys_mkdir(sFileSys.path);
}

char* fs_item(const char* str, ...) {
    char buffer[261];
    char* ret;
    va_list va;
    
    va_start(va, str);
    vsnprintf(buffer, 261, str, va);
    va_end(va);
    
    _log("%s", str);
    
    if (buffer[0] != '/' && buffer[0] != '\\')
        ret = x_fmt("%s%s", sFileSys.path, buffer);
    else
        ret = x_strdup(buffer + 1);
    
    return ret;
}

char* fs_find(const char* str) {
    char* file = NULL;
    time_t stat = 0;
    List list = List_New();
    
    if (*str == '*') str++;
    
    _log("Find: [%s] in [%s]", str, sFileSys.path);
    
    List_Walk(&list, sFileSys.path, 0, LIST_FILES | LIST_NO_DOT);
    
    for (int i = 0; i < list.num; i++) {
        if (striend(list.item[i], str)) {
            time_t s = sys_stat(list.item[i]);
            
            if (s > stat) {
                file = list.item[i];
                stat = s;
            }
        }
    }
    
    if (file) {
        file = x_strdup(file);
        _log("Found: %s", file);
    }
    
    List_Free(&list);
    
    return file;
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

bool cli_yesno(void) {
    char line[16] = {};
    int clear = 0;
    bool ret = true;
    
    while (stricmp(line, "y") && stricmp(line, "n")) {
        if (clear++)
            cli_clearln(2);
        
        printf("\r" PRNT_GRAY "<" PRNT_GRAY ": " PRNT_BLUE);
        fgets(line, 16, stdin);
        
        while (strend(line, "\n"))
            strend(line, "\n")[0] = '\0';
    }
    
    if (!stricmp(line, "n"))
        ret = false;
    
    cli_clearln(2);
    
    return ret;
}

void cli_clear(void) {
    printf("\033[2J");
    fflush(stdout);
}

void cli_clearln(int i) {
    printf("\x1b[2K");
    for (int j = 1; j < i; j++) {
        cli_gotoprevln();
        printf("\x1b[2K");
    }
    fflush(stdout);
}

void cli_gotoprevln(void) {
    printf("\x1b[1F");
    fflush(stdout);
}

const char* cli_gets(void) {
    static char line[512] = { 0 };
    
    printf("\r" PRNT_GRAY "<" PRNT_GRAY " " PRNT_RSET);
    fgets(line, 511, stdin);
    
    while (strend(line, "\n"))
        strend(line, "\n")[0] = '\0';
    
    _log("[%s]", line);
    
    return line;
}

char cli_getc() {
    char line[16] = {};
    
    printf("\r" PRNT_GRAY "<" PRNT_GRAY " " PRNT_BLUE);
    fgets(line, 16, stdin);
    
    cli_clearln(2);
    
    return 0;
}

void cli_hide(void) {
#ifdef _WIN32
    HWND window = GetConsoleWindow();
    ShowWindow(window, SW_HIDE);
    
#else
#endif
}

void cli_show(void) {
#ifdef _WIN32
    HWND window = GetConsoleWindow();
    ShowWindow(window, SW_SHOW);
#else
#endif
}

void cli_getSize(int* r) {
    int x = 0;
    int y = 0;
    
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

void cli_getPos(int* r) {
    int x = 0;
    int y = 0;
    
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO cbsi;
    
    _assert (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cbsi));
    
    x = cbsi.dwCursorPosition.X + 1;
    y = cbsi.dwCursorPosition.Y + 1;
#else
#endif // _WIN32
    
    r[0] = x;
    r[1] = y;
}

void cli_setPos(int x, int y) {
    fprintf(stdout, "\033[%d;%dH", y, x);
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

int qsort_numhex(const void* arg_a, const void* arg_b) {
    const char* a = *((char**)arg_a);
    const char* b = *((char**)arg_b);
    
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
            return qsort_numhex(&remainderA, &remainderB);
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
            return qsort_numhex(&remainderA, &remainderB);
    }
    if (isdigit(*a) || isdigit(*b)) {
        return *a - *b;
    }
    while (*a && *b) {
        if (isdigit(*a) || isdigit(*b))
            return qsort_numhex(&a, &b);
        if (tolower(*a) != tolower(*b))
            return tolower(*a) - tolower(*b);
        a++;
        b++;
    }
    return *a ? 1 : *b ? -1 : 0;
}

int qsort_u32(const void* arg_a, const void* arg_b) {
    const u32* a = arg_a;
    const u32* b = arg_b;
    
    return *a - *b;
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

#undef free
void* __vfree(const void* data) {
    if (data) free((void*)data);
    
    return NULL;
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
