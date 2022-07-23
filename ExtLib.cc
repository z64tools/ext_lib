#include <ExtLib.hpp>

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

s32 MemFile::loadFile(const char* fileName) {
	return MemFile_LoadFile(this, fileName);
}
s32 MemFile::loadFileString(const char* fileName) {
	return MemFile_LoadFile_String(this, fileName);
}
void MemFile::alloc(u32 size) {
	MemFile_Alloc(this, size);
}
void MemFile::realloc(u32 size) {
	MemFile_Realloc(this, size);
}
void MemFile::rewind() {
	MemFile_Rewind(this);
}
s32 MemFile::write(void* src, u32 size) {
	return MemFile_Write(this, src, size);
}
s32 MemFile::insert(void* src, u32 size) {
	return MemFile_Insert(this, src, size, -1);
}
s32 MemFile::insert(void* src, u32 size, u32 pos) {
	return MemFile_Insert(this, src, size, pos);
}
s32 MemFile::append(MemFile* src) {
	return MemFile_Append(this, src);
}
s32 MemFile::printf(const char* fmt, ...) {
	va_list va;
	char* pr;
	s32 r;
	
	va_start(va, fmt);
	
	vasprintf(&pr, va, fmt);
	r = this->write(pr, strlen(pr));
	
	va_end(va);
	::free(pr);
	
	return r;
}
s32 MemFile::read(void* dst, u32 size) {
	return MemFile_Read(this, dst, size);
}
void* MemFile::seek(u32 seek) {
	return MemFile_Seek(this, seek);
}
void MemFile::loadMem(void* src, u32 size) {
	MemFile_LoadMem(this, src, size);
}
void MemFile::free() {
	MemFile_Free(this);
}
void MemFile::reset() {
	MemFile_Reset(this);
}
void MemFile::clear() {
	MemFile_Clear(this);
}