#ifndef __EXTTYPES_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
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
typedef size_t Size;
typedef wchar_t wchar;

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
	MEM_CRC32    = 1 << 17,
	MEM_ALIGN    = 1 << 18,
	MEM_REALLOC  = 1 << 19,
	MEM_FILENAME = 1 << 20,
	
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
		u32 align;
		u32 realloc : 1;
		u32 getCrc  : 1;
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
		u32 alnum;
	} __private;
} ItemList;

typedef struct {
	s32 year;
	s32 month;
	s32 day;
	s32 hour;
	s32 minute;
	s32 second;
} Date;

typedef enum {
	DIR__MAKE_ON_ENTER = (1) << 0,
} DirParam;

typedef void (* SoundCallback)(void*, void*, u32);

typedef enum {
	SOUND_S16 = 2,
	SOUND_S24,
	SOUND_S32,
	SOUND_F32,
} SoundFormat;

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

typedef enum {
	STAT_ACCS = (1) << 0,
	STAT_MODF = (1) << 1,
	STAT_CREA = (1) << 2,
} StatFlag;

typedef enum {
	LIST_FILES    = 0x0,
	LIST_FOLDERS  = 0x1,
	
	LIST_RELATIVE = (1) << 4,
	LIST_NO_DOT   = (1) << 5,
} ListFlags;

#endif