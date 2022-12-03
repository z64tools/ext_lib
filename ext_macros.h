#ifndef EXT_MACROS_H
#define EXT_MACROS_H

#define PRNT_DGRY "\e[90;2m"
#define PRNT_DRED "\e[91;2m"
#define PRNT_GRAY "\e[90m"
#define PRNT_REDD "\e[91m"
#define PRNT_GREN "\e[92m"
#define PRNT_YELW "\e[93m"
#define PRNT_BLUE "\e[94m"
#define PRNT_PRPL "\e[95m"
#define PRNT_CYAN "\e[96m"
#define PRNT_DARK "\e[2m"
#define PRNT_ITLN "\e[3m"
#define PRNT_UNDL "\e[4m"
#define PRNT_BLNK "\e[5m"
#define PRNT_RSET "\e[m"
#define PRNT_NL   "\n"
#define PRNT_RNL  PRNT_RSET PRNT_NL
#define PRNT_TODO "\e[91;2m" "TODO"

#define catprintf(dest, ...) sprintf(dest + strlen(dest), __VA_ARGS__)

#define Node_Add(head, node) do {               \
        typeof(node) * __n__ = &head;           \
        while (*__n__) __n__ = &(*__n__)->next; \
        *__n__ = node;                          \
} while (0)

#define Node_Remove(head, node) do {                    \
        typeof(node) * __n__ = &head;                   \
        while (*__n__ != node) __n__ = &(*__n__)->next; \
        *__n__ = node->next;                            \
} while (0)

#define Node_Kill(head, node) do {    \
        typeof(node) killNode = node; \
        Node_Remove(head, node);      \
        Free(killNode);               \
} while (0)

#define Swap(a, b) do { \
        var y = a;      \
        a = b;          \
        b = y;          \
} while (0)

// Checks endianess with tst & tstP
#define ReadBE(in) ({                        \
        typeof(in) out;                      \
        s32 tst = 1;                         \
        u8* tstP = (u8*)&tst;                \
        if (tstP[0] != 0) {                  \
            s32 size = sizeof(in);           \
            if (size == 2) {                 \
                out = __builtin_bswap16(in); \
            } else if (size == 4) {          \
                out = __builtin_bswap32(in); \
            } else if (size == 8) {          \
                out = __builtin_bswap64(in); \
            } else {                         \
                out = in;                    \
            }                                \
        } else {                             \
            out = in;                        \
        }                                    \
        out;                                 \
    }                                        \
)

#define WriteBE(dest, set) {    \
        typeof(dest) get = set; \
        dest = ReadBE(get);     \
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

#define ArrayCount(arr)                  (u32)(sizeof(arr) / sizeof(arr[0]))
#define BitNumMask(bitNum)               ((1 << (bitNum)) - 1)
#define SizeMask(size)                   BitNumMask(size * 8)
#define FieldData(data, bitCount, shift) (((data) >> (shift))&BitNumMask(bitCount))

#define BinToMb(x)        ((f32)(x) / (f32)0x100000)
#define BinToKb(x)        ((f32)(x) / (f32)0x400)
#define MbToBin(x)        (u32)(0x100000 * (x))
#define KbToBin(x)        (u32)(0x400 * (x))
#define Align(val, align) ((((val) % (align)) != 0) ? (val) + (align) - ((val) % (align)) : (val))

#define bitscan32(u32) __builtin_ctz(u32)

#define VA1(NAME, ...)                                 NAME
#define VA2(_1, NAME, ...)                             NAME
#define VA3(_1, _2, NAME, ...)                         NAME
#define VA4(_1, _2, _3, NAME, ...)                     NAME
#define VA5(_1, _2, _3, _4, NAME, ...)                 NAME
#define VA6(_1, _2, _3, _4, _5, NAME, ...)             NAME
#define VA7(_1, _2, _3, _4, _5, _6, NAME, ...)         NAME
#define VA8(_1, _2, _3, _4, _5, _6, _7, NAME, ...)     NAME
#define VA9(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME

#define NARGS_SEQ(                                      \
        _1, _2, _3, _4, _5, _6, _7, _8,                 \
        _9, _10, _11, _12, _13, _14, _15, _16,          \
        _17, _18, _19, _20, _21, _22, _23, _24,         \
        _25, _26, _27, _28, _29, _30, _31, _32,         \
        _33, _34, _35, _36, _37, _38, _39, _40,         \
        _41, _42, _43, _44, _45, _46, _47, _48,         \
        _49, _50, _51, _52, _53, _54, _55, _56,         \
        _57, _58, _59, _60, _61, _62, _63, _64,         \
        _65, _66, _67, _68, _69, _70, _71, _72,         \
        _73, _74, _75, _76, _77, _78, _79, _80,         \
        _81, _82, _83, _84, _85, _86, _87, _88,         \
        _89, _90, _91, _92, _93, _94, _95, _96,         \
        _97, _98, _99, _100, _101, _102, _103, _104,    \
        _105, _106, _107, _108, _109, _110, _111, _112, \
        _113, _114, _115, _116, _117, _118, _119, _120, \
        _121, _122, _123, _124, _125, _126, _127, _128, N, ...) N
#define NARGS(...)                               \
    NARGS_SEQ(                                   \
        __VA_ARGS__                              \
        , 128, 127, 126, 125, 124, 123, 122, 121 \
        , 120, 119, 118, 117, 116, 115, 114, 113 \
        , 112, 111, 110, 109, 108, 107, 106, 105 \
        , 104, 103, 102, 101, 100, 99, 98, 97    \
        , 96, 95, 94, 93, 92, 91, 90, 89         \
        , 88, 87, 86, 85, 84, 83, 82, 81         \
        , 80, 79, 78, 77, 76, 75, 74, 73         \
        , 72, 71, 70, 69, 68, 67, 66, 65         \
        , 64, 63, 62, 61, 60, 59, 58, 57         \
        , 56, 55, 54, 53, 52, 51, 50, 49         \
        , 48, 47, 46, 45, 44, 43, 42, 41         \
        , 40, 39, 38, 37, 36, 35, 34, 33         \
        , 32, 31, 30, 29, 28, 27, 26, 25         \
        , 24, 23, 22, 21, 20, 19, 18, 17         \
        , 16, 15, 14, 13, 12, 11, 10, 9          \
        , 8, 7, 6, 5, 4, 3, 2, 1                 \
    )

#define Main(y1, y2) main(y1, y2)
#define SEG_FAULT ((u32*)0)[0] = 0

#if defined(_WIN32) && defined(UNICODE)
#define UnicodeMain(count, args)                     \
    __x_main(int count, char** args);                \
    int wmain(int count, wchar * *args) {            \
        char** nargv = Alloc(sizeof(char*) * count); \
        for (s32 i = 0; i < count; i++) {            \
            nargv[i] = Calloc(strwlen(args[i]));     \
            StrU8(nargv[i], args[i]);                \
        }                                            \
        Log("run " PRNT_YELW "main");                \
        return __x_main(count, nargv);               \
    }                                                \
    int __x_main(int count, char** args)
#else
#define UnicodeMain(count, args) main(int count, char** args)
#endif

#define TIME_I() Time_Start(45)
#define TIME_O() printf_info("%s %d: %.3fs", __FUNCTION__, __LINE__, Time_Get(45))

#define UnfoldRGB(color)  (color).r, (color).g, (color).b
#define UnfoldRGBA(color) (color).r, (color).g, (color).b, (color).a
#define UnfoldHSL(color)  (color).h, (color).s, (color).l

#define New(type)              Calloc(sizeof(type))
#define SleepF(sec)            usleep((u32)((f32)(sec) * 1000 * 1000))
#define SleepS(sec)            sleep(sec)
#define EXT_INFO_TITLE(xtitle) PRNT_YELW xtitle PRNT_RNL
#define EXT_INFO(A, indent, B) PRNT_GRAY "[>]: " PRNT_RSET A "\r\033[" #indent "C" PRNT_GRAY "# " B PRNT_NL

#define foreach(val, arr)        for (int val = 0; val < ArrayCount(arr); val++)
#define forlist(val, list)       for (int val = 0; val < (list).num; val++)
#define fornode(type, val, head) for (type* val = head; val != NULL; val = val->next)
#define forstr(val, str)         for (int val = 0; val < strlen(str); val++)
#define forline(val, str)        for (const char* val = str; val; val = Line(val, 1))

#define ArrMoveR(arr, start, count) do {                      \
        var v = (arr)[(start) + (count) - 1];                 \
        for (int i = (count) + (start) - 1; i > (start); i--) \
        (arr)[i] = (arr)[i - 1];                              \
        (arr)[(start)] = v;                                   \
} while (0)

#define ArrMoveL(arr, start, count) do {                  \
        var v = (arr)[(start)];                           \
        for (int i = (start); i < (count) + (start); i++) \
        (arr)[i] = (arr)[i + 1];                          \
        (arr)[(count) + (start) - 1] = v;                 \
} while (0)

/**
 * These are only to satisfy clang IDE. These won't work
 * as expected if compiled with clang.
 */
#ifdef __clang__
#define Block(type, name, args) \
    type (^ name) args = NULL;  \
    name = ^ type args
#define BlockDecl(type, name, args) \
    type (^ name) args
#define BlockVar(var) typeof(var) var

#else
#define Block(type, name, args) \
    type name args
#define BlockDecl(type, name, args) (void)0
#define BlockVar(var)               (void)0
#endif

#define FOPEN(file, mode) ({                                                 \
        FILE* f = fopen(file, mode);                                         \
        if (f == NULL) {                                                     \
            printf_warning("" PRNT_YELW "%s" PRNT_GRAY "();", __FUNCTION__); \
            printf_error("fopen error: [%s]", file);                         \
        }                                                                    \
        f;                                                                   \
    })

#endif