#ifndef __EXTLIB_HPP__
#define __EXTLIB_HPP__

#include <ExtTypes.h>

class MemFile {
public:
	union {
		void* data;
		PointerCast cast;
		char* str;
	};
	u32 memSize;
	u32 size;
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
	
	s32 loadFile(const char* fileName);
	s32 loadFileString(const char* fileName);
	void alloc(u32 size);
	void realloc(u32 size);
	void rewind();
	s32 write(void* src, u32 size);
	s32 insert(void* src, u32 size);
	s32 insert(void* src, u32 size, u32 pos);
	s32 append(MemFile* src);
	s32 printf(const char* fmt, ...);
	s32 read(void* dst, u32 size);
	void* seek(u32 seek);
	void loadMem(void* src, u32 size);
	void free();
	void reset();
	void clear();
};

#include <ExtLib.h>

#endif