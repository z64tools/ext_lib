#include <ext_lib.h>

#undef memfile_alloc

// # # # # # # # # # # # # # # # # # # # #
// # MEMFILE                             #
// # # # # # # # # # # # # # # # # # # # #

static void memfile_throw_error(memfile_t* this, const char* msg, const char* info) {
    Log_Print();
    
    print_warn_align("" PRNT_REDD "Work Directory" PRNT_RSET ":", PRNT_YELW "%s", sys_workdir());
    print_warn_align("" PRNT_REDD "File" PRNT_RSET ":", PRNT_YELW "%s", this->info.name);
    print_warn_align("" PRNT_REDD "Error" PRNT_RSET ":", "%s", msg);
    if (info) {
        print_warn_align("" PRNT_REDD "Info" PRNT_RSET ":", "%s", info);
        print_warn_align("" PRNT_REDD "size_t" PRNT_RSET ":", "%f kB", kb_to_bin(this->size));
        print_warn_align("" PRNT_REDD "MemSize" PRNT_RSET ":", "%f kB", kb_to_bin(this->memSize));
    }
    print_error("Stopping Process");
}

static FILE* memfile_fopen(const char* name, const char* mode) {
    FILE* file;
    
#if _WIN32
    wchar* name16 = calloc(strlen(name) * 4);
    wchar* mode16 = calloc(strlen(mode) * 4);
    strto16(name16, name);
    strto16(mode16, mode);
    
    file = _wfopen(name16, mode16);
    
    free(name16);
    free(mode16);
#else
    file = fopen(name, mode);
#endif
    
    return file;
}

static void memfile_validate(memfile_t* this) {
    if (this->param.initKey == 0xD0E0A0D0B0E0E0F0) {
        
        return;
    }
    
    *this = memfile_new();
}

memfile_t memfile_new() {
    return (memfile_t) {
               .param.initKey = 0xD0E0A0D0B0E0E0F0,
               .param.throwError = true,
    };
}

void memfile_set(memfile_t* this, ...) {
    va_list args;
    size_t cmd = 0;
    size_t arg = 0;
    
    if (this->param.initKey != 0xD0E0A0D0B0E0E0F0)
        *this = memfile_new();
    
    va_start(args, this);
    for (;;) {
        cmd = va_arg(args, uaddr_t);
        
        if (cmd == MEM_END)
            break;
        
        switch (cmd) {
            case MEM_ALIGN:
            case MEM_CRC32:
            case MEM_REALLOC:
                arg = va_arg(args, uaddr_t);
                break;
        }
        
        if (cmd == MEM_CLEAR) {
            cmd = arg;
            arg = 0;
        }
        
        switch (cmd) {
            case MEM_THROW_ERROR:
                this->param.throwError = true;
            case MEM_ALIGN:
                this->param.align = arg;
                break;
            case MEM_CRC32:
                _log("memfile_set: deprecated feature [MEM_CRC32], [%s]", this->info.name);
                break;
            case MEM_REALLOC:
                _log("memfile_set: deprecated feature [MEM_REALLOC], [%s]", this->info.name);
                break;
        }
    }
    va_end(args);
}

void memfile_alloc(memfile_t* this, size_t size) {
    if (this->param.initKey != 0xD0E0A0D0B0E0E0F0) {
        *this = memfile_new();
    } else if (this->data) {
        _log("memfile_alloc: Mallocing already allocated memfile_t [%s], freeing and reallocating!", this->info.name);
        memfile_free(this);
    }
    
    Assert ((this->data = calloc(size)) != NULL);
    this->memSize = size;
}

void memfile_realloc(memfile_t* this, size_t size) {
    if (this->param.initKey != 0xD0E0A0D0B0E0E0F0)
        *this = memfile_new();
    if (this->memSize > size)
        return;
    
    Assert((this->data = realloc(this->data, size)) != NULL);
    this->memSize = size;
}

void memfile_rewind(memfile_t* this) {
    this->seekPoint = 0;
}

int memfile_write(memfile_t* this, const void* src, size_t size) {
    const size_t osize = size;
    
    if (!this->memSize)
        memfile_alloc(this, size * 4);
    
    if (src == NULL) {
        _log("Trying to write 0 size");
        
        return 0;
    }
    
    size = clamp_max(size, clamp_min(this->memSize - this->seekPoint, 0));
    
    if (size != osize) {
        memfile_realloc(this, this->seekPoint + (this->memSize * 2) + (osize * 2));
        size = osize;
    }
    
    memcpy(&this->cast.u8[this->seekPoint], src, size);
    this->seekPoint += size;
    this->size = Max(this->size, this->seekPoint);
    
    if (this->param.align)
        memfile_align(this, this->param.align);
    
    return size;
}

int memfile_append_file(memfile_t* this, const char* source) {
    FILE* f;
    char buffer[256];
    size_t size;
    size_t amount = 0;
    
    if (!(f = fopen(source, "rb")))
        return false;
    
    while ((size = fread(buffer, 1, sizeof(buffer), f))) {
        if (memfile_write(this, buffer, size) != size)
            print_error("memfile_t: Could not successfully write from fread [%s]", source);
        amount += size;
    }
    
    fclose(f);
    
    return amount;
}

int memfile_insert(memfile_t* this, const void* src, size_t size) {
    size_t remasize = this->size - this->seekPoint;
    
    if (this->size + size + 1 >= this->memSize)
        memfile_realloc(this, this->memSize * 2 + size * 2 + this->seekPoint);
    
    memmove(&this->cast.u8[this->seekPoint + size], &this->cast.u8[this->seekPoint], remasize + 1);
    memcpy(&this->cast.u8[this->seekPoint], src, size);
    this->size += size;
    
    return 0;
}

int memfile_append(memfile_t* this, memfile_t* src) {
    return memfile_write(this, src->data, src->size);
}

void memfile_align(memfile_t* this, size_t align) {
    if ((this->seekPoint % align) != 0) {
        const u64 wow[32] = {};
        size_t size = align - (this->seekPoint % align);
        
        memfile_write(this, wow, size);
    }
}

int memfile_fmt(memfile_t* this, const char* fmt, ...) {
    char buffer[8192];
    va_list args;
    size_t size;
    
    va_start(args, fmt);
    size = vsnprintf(
        buffer,
        8192,
        fmt,
        args
    );
    va_end(args);
    
    size = memfile_write(this, buffer, size + 1);
    this->seekPoint--;
    this->size--;
    this->cast.u8[this->seekPoint] = '\0';
    
    return size;
}

int memfile_read(memfile_t* this, void* dest, size_t size) {
    size_t nsize = clamp_max(size, clamp_min(this->size - this->seekPoint, 0));
    
    if (nsize != size)
        _log("%d == src->seekPoint = %d / %d", nsize, this->seekPoint, this->seekPoint);
    
    if (nsize < 1)
        return 0;
    
    memcpy(dest, &this->cast.u8[this->seekPoint], nsize);
    this->seekPoint += nsize;
    
    return nsize;
}

void* memfile_seek(memfile_t* this, size_t seek) {
    if (seek == MEMFILE_SEEK_END)
        seek = this->size;
    
    if (seek > this->size)
        return NULL;
    
    this->seekPoint = seek;
    
    return (void*)&this->cast.u8[seek];
}

void memfile_load_mem(memfile_t* this, void* data, size_t size) {
    memfile_validate(this);
    memfile_null(this);
    this->size = this->memSize = size;
    this->data = data;
}

int memfile_load_bin(memfile_t* this, const char* filepath) {
    FILE* file = memfile_fopen(filepath, "rb");
    size_t tempSize;
    
    if (file == NULL) {
        _log("Could not fopen file [%s]", filepath);
        
        if (this->param.throwError) {
            if (sys_stat(this->info.name))
                memfile_throw_error(this, "Could not load file!", x_fmt("Arg: [%s]", filepath));
            memfile_throw_error(this, "Can't load because file does not exist!", x_fmt("Arg: [%s]", filepath));
        }
        
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    tempSize = ftell(file);
    
    memfile_validate(this);
    memfile_null(this);
    
    if (this->data == NULL)
        memfile_alloc(this, tempSize + 0x10);
    
    else if (this->memSize < tempSize)
        memfile_realloc(this, tempSize * 2);
    
    this->size = tempSize;
    
    rewind(file);
    if (fread(this->data, 1, this->size, file)) {
    }
    fclose(file);
    
    this->info.age = sys_stat(filepath);
    free(this->info.name);
    this->info.name = strdup(filepath);
    
    return 0;
}

int memfile_load_str(memfile_t* this, const char* filepath) {
    FILE* file = memfile_fopen(filepath, "r");
    size_t tempSize;
    
    if (file == NULL) {
        _log("Could not fopen file [%s]", filepath);
        
        if (this->param.throwError) {
            if (sys_stat(this->info.name))
                memfile_throw_error(this, "Could not load file!", x_fmt("Arg: [%s]", filepath));
            memfile_throw_error(this, "Can't load because file does not exist!", x_fmt("Arg: [%s]", filepath));
        }
        
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    tempSize = ftell(file);
    
    memfile_validate(this);
    memfile_null(this);
    
    if (this->data == NULL)
        memfile_alloc(this, tempSize + 0x10);
    
    else if (this->memSize < tempSize)
        memfile_realloc(this, tempSize * 2);
    
    this->size = tempSize;
    
    rewind(file);
    this->size = fread(this->data, 1, this->size, file);
    fclose(file);
    this->cast.u8[this->size] = '\0';
    
    this->info.age = sys_stat(filepath);
    free(this->info.name);
    this->info.name = strdup(filepath);
    
    return 0;
}

int memfile_save_bin(memfile_t* this, const char* filepath) {
    FILE* file = memfile_fopen(filepath, "wb");
    
    if (file == NULL) {
        _log("Could not fopen file [%s]", filepath);
        
        if (this->param.throwError) {
            if (sys_stat(this->info.name))
                memfile_throw_error(this, "Can't save over file!", x_fmt("Arg: [%s]", filepath));
            memfile_throw_error(this, "Can't save file!", x_fmt("Arg: [%s]", filepath));
        }
        
        return 1;
    }
    
    if (this->size)
        fwrite(this->data, sizeof(char), this->size, file);
    fclose(file);
    
    return 0;
}

int memfile_save_str(memfile_t* this, const char* filepath) {
    FILE* file = memfile_fopen(filepath, "w");
    
    if (file == NULL) {
        _log("Could not fopen file [%s]", filepath);
        
        if (this->param.throwError) {
            if (sys_stat(this->info.name))
                memfile_throw_error(this, "Can't save over file!", x_fmt("Arg: [%s]", filepath));
            memfile_throw_error(this, "Can't save file!", x_fmt("Arg: [%s]", filepath));
        }
        
        return 1;
    }
    
    if (this->size)
        fwrite(this->data, sizeof(char), this->size, file);
    fclose(file);
    
    return 0;
}

void memfile_free(memfile_t* this) {
    if (this->param.initKey == 0xD0E0A0D0B0E0E0F0)
        free(this->data, this->info.name);
    
    *this = memfile_new();
}

void memfile_null(memfile_t* this) {
    this->size = 0;
    this->seekPoint = 0;
    if (this->data)
        this->str[0] = '\0';
}

void memfile_clear(memfile_t* this) {
    memset(this->data, 0, this->memSize);
    memfile_null(this);
}
