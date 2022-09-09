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
        
        return;
    }
    
    *mem = MemFile_Initialize();
}

MemFile MemFile_Initialize() {
    return (MemFile) { .param.initKey = 0xD0E0A0D0B0E0E0F0 };
}

void MemFile_Params(MemFile* mem, ...) {
    va_list args;
    u32 cmd;
    u32 arg;
    
    if (mem->param.initKey != 0xD0E0A0D0B0E0E0F0)
        *mem = MemFile_Initialize();
    
    va_start(args, mem);
    for (;;) {
        cmd = va_arg(args, uptr);
        
        if (cmd == MEM_END)
            break;
        
        arg = va_arg(args, uptr);
        
        if (cmd == MEM_CLEAR) {
            cmd = arg;
            arg = 0;
        }
        
        switch (cmd) {
            case MEM_ALIGN:
                mem->param.align = arg;
                break;
            case MEM_CRC32:
                Log("MemFile_Params: deprecated feature [MEM_CRC32], [%s]", mem->info.name);
                break;
            case MEM_REALLOC:
                Log("MemFile_Params: deprecated feature [MEM_REALLOC], [%s]", mem->info.name);
                break;
        }
    }
    va_end(args);
}

void MemFile_Alloc(MemFile* mem, u32 size) {
    
    if (mem->param.initKey != 0xD0E0A0D0B0E0E0F0) {
        *mem = MemFile_Initialize();
    } else if (mem->data) {
        Log("MemFile_Alloc: Mallocing already allocated MemFile [%s], freeing and reallocating!", mem->info.name);
        MemFile_Free(mem);
    }
    
    Assert ((mem->data = Calloc(size)) != NULL);
    mem->memSize = size;
}

void MemFile_Realloc(MemFile* mem, u32 size) {
    if (mem->param.initKey != 0xD0E0A0D0B0E0E0F0)
        *mem = MemFile_Initialize();
    if (mem->memSize > size)
        return;
    
    Assert((mem->data = Realloc(mem->data, size)) != NULL);
    mem->memSize = size;
}

void MemFile_Rewind(MemFile* mem) {
    mem->seekPoint = 0;
}

s32 MemFile_Write(MemFile* dest, const void* src, u32 size) {
    u32 osize = size;
    
    if (src == NULL) {
        Log("Trying to write 0 size");
        
        return 0;
    }
    
    size = ClampMax(size, ClampMin(dest->memSize - dest->seekPoint, 0));
    
    if (size != osize) {
        MemFile_Realloc(dest, dest->memSize * 2 + osize * 2);
        size = osize;
    }
    
    memcpy(&dest->cast.u8[dest->seekPoint], src, size);
    dest->seekPoint += size;
    dest->size = Max(dest->size, dest->seekPoint);
    
    if (dest->param.align)
        MemFile_Align(dest, dest->param.align);
    
    return size;
}

/*
 * If pos is 0 or bigger: override seekPoint
 */
s32 MemFile_Insert(MemFile* mem, const void* src, u32 size, s64 pos) {
    u32 p = pos < 0 ? mem->seekPoint : pos;
    u32 remasize = mem->size - p;
    
    if (p + size + remasize >= mem->memSize)
        MemFile_Realloc(mem, mem->memSize * 2 + size * 2);
    
    memmove(&mem->cast.u8[p + remasize], &mem->cast.u8[p], remasize);
    
    return MemFile_Write(mem, src, size);
}

s32 MemFile_Append(MemFile* dest, MemFile* src) {
    return MemFile_Write(dest, src->data, src->size);
}

void MemFile_Align(MemFile* src, u32 align) {
    if ((src->seekPoint % align) != 0) {
        const u64 wow[32] = {};
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
    
    if (seek > src->size)
        return NULL;
    
    src->seekPoint = seek;
    
    return (void*)&src->cast.u8[seek];
}

void MemFile_LoadMem(MemFile* mem, void* data, Size size) {
    MemFile_Validate(mem);
    MemFile_Reset(mem);
    mem->size = mem->memSize = size;
    mem->data = data;
}

s32 MemFile_LoadFile(MemFile* mem, const char* filepath) {
    FILE* file = MemFOpen(filepath, "rb");
    u32 tempSize;
    
    if (file == NULL) {
        Log("Could not fopen file [%s]", filepath);
        
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    tempSize = ftell(file);
    
    MemFile_Validate(mem);
    MemFile_Reset(mem);
    
    if (mem->data == NULL)
        MemFile_Alloc(mem, tempSize + 0x10);
    
    else if (mem->memSize < tempSize)
        MemFile_Realloc(mem, tempSize * 2);
    
    mem->size = tempSize;
    
    rewind(file);
    if (fread(mem->data, 1, mem->size, file)) {
    }
    fclose(file);
    
    mem->info.age = Sys_Stat(filepath);
    mem->info.name = strdup(filepath);
    
    return 0;
}

s32 MemFile_LoadFile_String(MemFile* mem, const char* filepath) {
    FILE* file = MemFOpen(filepath, "r");
    u32 tempSize;
    
    if (file == NULL) {
        Log("Could not fopen file [%s]", filepath);
        
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    tempSize = ftell(file);
    
    MemFile_Validate(mem);
    MemFile_Reset(mem);
    
    if (mem->data == NULL)
        MemFile_Alloc(mem, tempSize + 0x10);
    
    else if (mem->memSize < tempSize)
        MemFile_Realloc(mem, tempSize * 2);
    
    mem->size = tempSize;
    
    rewind(file);
    mem->size = fread(mem->data, 1, mem->size, file);
    fclose(file);
    mem->cast.u8[mem->size] = '\0';
    
    mem->info.age = Sys_Stat(filepath);
    mem->info.name = strdup(filepath);
    
    return 0;
}

s32 MemFile_SaveFile(MemFile* mem, const char* filepath) {
    FILE* file = MemFOpen(filepath, "wb");
    
    if (file == NULL) {
        Log("Could not fopen file [%s]", filepath);
        
        return 1;
    }
    
    if (mem->size)
        fwrite(mem->data, sizeof(char), mem->size, file);
    fclose(file);
    
    return 0;
}

s32 MemFile_SaveFile_String(MemFile* mem, const char* filepath) {
    FILE* file = MemFOpen(filepath, "w");
    
    if (file == NULL) {
        Log("Could not fopen file [%s]", filepath);
        
        return 1;
    }
    
    if (mem->size)
        fwrite(mem->data, sizeof(char), mem->size, file);
    fclose(file);
    
    return 0;
}

void MemFile_Free(MemFile* mem) {
    if (mem->param.initKey == 0xD0E0A0D0B0E0E0F0) {
        Free(mem->data);
        Free(mem->info.name);
    }
    
    *mem = MemFile_Initialize();
}

void MemFile_Reset(MemFile* mem) {
    mem->size = 0;
    mem->seekPoint = 0;
}

void MemFile_Clear(MemFile* mem) {
    memset(mem->data, 0, mem->memSize);
    MemFile_Reset(mem);
}
