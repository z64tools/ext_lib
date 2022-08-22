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

#define catprintf(dest, ...)    sprintf(dest + strlen(dest), __VA_ARGS__)

#define Node_Add(head, node) do { \
		typeof(node) * n = &head; \
		while (*n) n = &(*n)->next; \
		*n = node; \
} while (0)

#define Node_Remove(head, node) do { \
		typeof(node) * n = &head; \
		while (*n != node) n = &(*n)->next; \
		*n = node->next; \
} while (0)

#define Node_Kill(head, node) do { \
		typeof(node) killNode = node; \
		Node_Remove(head, node); \
		Free(killNode); \
} while (0)

#define Swap(a, b) do { \
		var y = a; \
		a = b; \
		b = y; \
} while (0)

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

#define ArrayCount(arr)                  (u32)(sizeof(arr) / sizeof(arr[0]))
#define BitNumMask(bitNum)               ((1 << (bitNum)) - 1)
#define SizeMask(size)                   BitNumMask(size * 8)
#define FieldData(data, bitCount, shift) (((data) >> (shift)) & BitNumMask(bitCount))

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

#define NARGS_SEQ(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, N, ...) N
#define NARGS(...) \
	NARGS_SEQ(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

#define Main(y1, y2) main(y1, y2)
#define Arg(arg)     ParseArgs(argv, arg, &parArg)
#define SEG_FAULT ((u32*)0)[0] = 0

#if defined(_WIN32) && defined(UNICODE)
#define UnicodeMain(count, args) \
	__x_main(int count, char** args); \
	int wmain(int count, wchar * *args) { \
		char** nargv = Alloc(sizeof(char*) * count); \
		for (s32 i = 0; i < count; i++) { \
			nargv[i] = Calloc(strwlen(args[i])); \
			StrU8(nargv[i], args[i]); \
		} \
		Log("run " PRNT_YELW "main"); \
		return __x_main(count, nargv); \
	} \
	int __x_main(int count, char** args)
#else
#define UnicodeMain(count, args) main(int count, char** args)
#endif

#define SleepF(sec)            usleep((u32)((f32)(sec) * 1000 * 1000))
#define SleepS(sec)            sleep(sec)
#define ParseArg(xarg)         ParseArgs(argv, xarg, &parArg)
#define EXT_INFO_TITLE(xtitle) PRNT_YELW xtitle PRNT_RNL
#define EXT_INFO(A, indent, B) PRNT_GRAY "[>]: " PRNT_RSET A "\r\033[" #indent "C" PRNT_GRAY "# " B PRNT_NL

#define foreach(val, arr)        for (int val = 0; val < ArrayCount(arr); val++)
#define forlist(val, list)       for (int val = 0; val < (list).num; val++)
#define fornode(type, val, head) for (type* val = head; val != NULL; val = val->next)
#define forstr(val, str)         for (int val = 0; val < strlen(str); val++)

#define ArrMoveR(arr, start, count) do { \
		var v = (arr)[(start) + (count) - 1]; \
		for (int i = (count) + (start) - 1; i > (start); i--) \
		(arr)[i] = (arr)[i - 1]; \
		(arr)[(start)] = v; \
} while (0)

#define ArrMoveL(arr, start, count) do { \
		var v = (arr)[(start)]; \
		for (int i = (start); i < (count) + (start); i++) \
		(arr)[i] = (arr)[i + 1]; \
		(arr)[(count) + (start) - 1] = v; \
} while (0)

#endif