#include <ext_lib.h>

#undef MemFile_Alloc

// # # # # # # # # # # # # # # # # # # # #
// # MEMFILE                             #
// # # # # # # # # # # # # # # # # # # # #

static FILE* MemFOpen(const char* name, const char* mode) {
	FILE* file;
	
#if _WIN32
		wchar* name16 = Calloc(strlen(name) * 4);
		wchar* mode16 = Calloc(strlen(mode) * 4);
		StrU16(name16, name);
		StrU16(mode16, mode);
		
		file = _wfopen(name16, mode16);
		
		Free(name16);
		Free(mode16);
#else
		file = fopen(name, mode);
#endif
	
	return file;
}

void MemFile_Validate(MemFile* mem) {
	if (mem->param.initKey == 0xD0E0A0D0B0E0E0F0) {
		MemFile_Reset(mem);
		
		return;
	}
	
	*mem = MemFile_Initialize();
}

MemFile MemFile_Initialize() {
	return (MemFile) { .param.initKey = 0xD0E0A0D0B0E0E0F0 };
}

void MemFile_Params(MemFile* memFile, ...) {
	va_list args;
	u32 cmd;
	u32 arg;
	
	if (memFile->param.initKey == 0xD0E0A0D0B0E0E0F0)
		*memFile = MemFile_Initialize();
	
	va_start(args, memFile);
	for (;;) {
		cmd = va_arg(args, uptr);
		
		if (cmd == MEM_END) {
			break;
		}
		
		arg = va_arg(args, uptr);
		
		if (cmd == MEM_CLEAR) {
			cmd = arg;
			arg = 0;
		}
		
		switch (cmd) {
			case MEM_ALIGN:
				memFile->param.align = arg;
				break;
			case MEM_CRC32:
				memFile->param.getCrc = arg != 0 ? true : false;
				break;
			case MEM_REALLOC:
				memFile->param.realloc = arg != 0 ? true : false;
				break;
		}
	}
	va_end(args);
}

void MemFile_Alloc(MemFile* memFile, u32 size) {
	
	if (memFile->param.initKey != 0xD0E0A0D0B0E0E0F0) {
		*memFile = MemFile_Initialize();
	} else if (memFile->data) {
		Log("MemFile_Alloc: Mallocing already allocated MemFile [%s], freeing and reallocating!", memFile->info.name);
		MemFile_Free(memFile);
	}
	
	Assert ((memFile->data = Calloc(size)) != NULL);
	memFile->memSize = size;
}

void MemFile_Realloc(MemFile* memFile, u32 size) {
	if (memFile->param.initKey != 0xD0E0A0D0B0E0E0F0)
		*memFile = MemFile_Initialize();
	if (memFile->memSize > size)
		return;
	
	Assert((memFile->data = Realloc(memFile->data, size)) != NULL);
	memFile->memSize = size;
}

void MemFile_Rewind(MemFile* memFile) {
	memFile->seekPoint = 0;
}

s32 MemFile_Write(MemFile* dest, void* src, u32 size) {
	u32 osize = size;
	
	if (src == NULL) {
		Log("Trying to write 0 size");
		
		return 0;
	}
	
	size = ClampMax(size, ClampMin(dest->memSize - dest->seekPoint, 0));
	
	if (size != osize)
		MemFile_Realloc(dest, dest->memSize * 2 + size);
	
	memcpy(&dest->cast.u8[dest->seekPoint], src, size);
	dest->seekPoint += size;
	dest->size = Max(dest->size, dest->seekPoint);
	
	if (dest->param.align)
		if ((dest->seekPoint % dest->param.align) != 0)
			MemFile_Align(dest, dest->param.align);
	
	return size;
}

/*
 * If pos is 0 or bigger: override seekPoint
 */
s32 MemFile_Insert(MemFile* mem, void* src, u32 size, s64 pos) {
	u32 p = pos < 0 ? mem->seekPoint : pos;
	u32 remasize = mem->size - p;
	
	if (p + size + remasize >= mem->memSize)
		MemFile_Realloc(mem, mem->memSize * 2);
	
	memmove(&mem->cast.u8[p + remasize], &mem->cast.u8[p], remasize);
	
	return MemFile_Write(mem, src, size);
}

s32 MemFile_Append(MemFile* dest, MemFile* src) {
	return MemFile_Write(dest, src->data, src->size);
}

void MemFile_Align(MemFile* src, u32 align) {
	if ((src->seekPoint % align) != 0) {
		u64 wow[2] = { 0 };
		u32 size = align - (src->seekPoint % align);
		
		MemFile_Write(src, wow, size);
	}
}

s32 MemFile_Printf(MemFile* dest, const char* fmt, ...) {
	char* buffer;
	va_list args;
	
	va_start(args, fmt);
	vasprintf(
		&buffer,
		fmt,
		args
	);
	va_end(args);
	
	return ({
		s32 ret = MemFile_Write(dest, buffer, strlen(buffer));
		Free(buffer);
		ret;
	});
}

s32 MemFile_Read(MemFile* src, void* dest, Size size) {
	Size nsize = ClampMax(size, ClampMin(src->size - src->seekPoint, 0));
	
	if (nsize != size)
		Log("%d == src->seekPoint = %d / %d", nsize, src->seekPoint, src->seekPoint);
	
	if (nsize < 1)
		return 0;
	
	memcpy(dest, &src->cast.u8[src->seekPoint], nsize);
	src->seekPoint += nsize;
	
	return nsize;
}

void* MemFile_Seek(MemFile* src, u32 seek) {
	if (seek == MEMFILE_SEEK_END)
		seek = src->size;
	
	if (seek > src->memSize) {
		return NULL;
	}
	src->seekPoint = seek;
	
	return (void*)&src->cast.u8[seek];
}

void MemFile_LoadMem(MemFile* mem, void* data, Size size) {
	MemFile_Validate(mem);
	mem->size = mem->memSize = size;
	mem->data = data;
}

s32 MemFile_LoadFile(MemFile* memFile, const char* filepath) {
	FILE* file = MemFOpen(filepath, "rb");
	u32 tempSize;
	
	if (file == NULL) {
		Log("Could not fopen file [%s]", filepath);
		
		return 1;
	}
	
	fseek(file, 0, SEEK_END);
	tempSize = ftell(file);
	
	MemFile_Validate(memFile);
	
	if (memFile->data == NULL)
		MemFile_Alloc(memFile, tempSize + 0x10);
	
	else if (memFile->memSize < tempSize)
		MemFile_Realloc(memFile, tempSize * 2);
	
	memFile->size = tempSize;
	
	rewind(file);
	if (fread(memFile->data, 1, memFile->size, file)) {
	}
	fclose(file);
	
	memFile->info.age = Sys_Stat(filepath);
	memFile->info.name = strdup(filepath);
	
	return 0;
}

s32 MemFile_LoadFile_String(MemFile* memFile, const char* filepath) {
	FILE* file = MemFOpen(filepath, "r");
	u32 tempSize;
	
	if (file == NULL) {
		Log("Could not fopen file [%s]", filepath);
		
		return 1;
	}
	
	fseek(file, 0, SEEK_END);
	tempSize = ftell(file);
	
	MemFile_Validate(memFile);
	
	if (memFile->data == NULL)
		MemFile_Alloc(memFile, tempSize + 0x10);
	
	else if (memFile->memSize < tempSize)
		MemFile_Realloc(memFile, tempSize * 2);
	
	memFile->size = tempSize;
	
	rewind(file);
	memFile->size = fread(memFile->data, 1, memFile->size, file);
	fclose(file);
	memFile->cast.u8[memFile->size] = '\0';
	
	memFile->info.age = Sys_Stat(filepath);
	memFile->info.name = strdup(filepath);
	
	return 0;
}

s32 MemFile_SaveFile(MemFile* memFile, const char* filepath) {
	FILE* file = MemFOpen(filepath, "wb");
	
	if (file == NULL) {
		Log("Could not fopen file [%s]", filepath);
		
		return 1;
	}
	
	if (memFile->size)
		fwrite(memFile->data, sizeof(char), memFile->size, file);
	fclose(file);
	
	return 0;
}

s32 MemFile_SaveFile_String(MemFile* memFile, const char* filepath) {
	FILE* file = MemFOpen(filepath, "w");
	
	if (file == NULL) {
		Log("Could not fopen file [%s]", filepath);
		
		return 1;
	}
	
	if (memFile->size)
		fwrite(memFile->data, sizeof(char), memFile->size, file);
	fclose(file);
	
	return 0;
}

void MemFile_Free(MemFile* memFile) {
	if (memFile->param.initKey == 0xD0E0A0D0B0E0E0F0) {
		Free(memFile->data);
		Free(memFile->info.name);
	}
	
	*memFile = MemFile_Initialize();
}

void MemFile_Reset(MemFile* memFile) {
	memFile->size = 0;
	memFile->seekPoint = 0;
}

void MemFile_Clear(MemFile* memFile) {
	memset(memFile->data, 0, memFile->memSize);
	MemFile_Reset(memFile);
}
