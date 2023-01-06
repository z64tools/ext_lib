#include <ext_lib.h>

#undef Memfile_Set

// # # # # # # # # # # # # # # # # # # # #
// # MEMFILE                             #
// # # # # # # # # # # # # # # # # # # # #

static void Memfile_Throw(Memfile* this, const char* msg, const char* info) {
    _log_print();
    
    warn_align("" PRNT_REDD "Work Directory" PRNT_RSET ":", PRNT_YELW "%s", sys_workdir());
    warn_align("" PRNT_REDD "File" PRNT_RSET ":", PRNT_YELW "%s", this->info.name);
    warn_align("" PRNT_REDD "Error" PRNT_RSET ":", "%s", msg);
    if (info) {
        warn_align("" PRNT_REDD "Info" PRNT_RSET ":", "%s", info);
        warn_align("" PRNT_REDD "size_t" PRNT_RSET ":", "%f kB", KbToBin(this->size));
        warn_align("" PRNT_REDD "MemSize" PRNT_RSET ":", "%f kB", KbToBin(this->memSize));
    }
    errr("Stopping Process");
}

static FILE* Memfile_Fopen(const char* name, const char* mode) {
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

static void Memfile_Validate(Memfile* this) {
    if (this->param.initKey == 0xD0E0A0D0B0E0E0F0) {
        
        return;
    }
    
    *this = Memfile_New();
}

Memfile Memfile_New() {
    return (Memfile) {
               .param.initKey = 0xD0E0A0D0B0E0E0F0,
               .param.throwError = true,
    };
}

void Memfile_Set(Memfile* this, ...) {
    va_list args;
    size_t cmd = 0;
    size_t arg = 0;
    
    if (this->param.initKey != 0xD0E0A0D0B0E0E0F0)
        *this = Memfile_New();
    
    va_start(args, this);
    for (;;) {
        cmd = va_arg(args, int);
        
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
                _log("Memfile_Set: deprecated feature [MEM_CRC32], [%s]", this->info.name);
                break;
            case MEM_REALLOC:
                _log("Memfile_Set: deprecated feature [MEM_REALLOC], [%s]", this->info.name);
                break;
        }
    }
    va_end(args);
}

void Memfile_Alloc(Memfile* this, size_t size) {
    if (this->param.initKey != 0xD0E0A0D0B0E0E0F0) {
        *this = Memfile_New();
    } else if (this->data) {
        _log("Memfile_Alloc: Mallocing already allocated Memfile [%s], freeing and reallocating!", this->info.name);
        Memfile_Free(this);
    }
    
    _assert ((this->data = calloc(size)) != NULL);
    this->memSize = size;
}

void Memfile_Realloc(Memfile* this, size_t size) {
    if (this->param.initKey != 0xD0E0A0D0B0E0E0F0)
        *this = Memfile_New();
    if (this->memSize > size)
        return;
    
    _assert((this->data = realloc(this->data, size)) != NULL);
    this->memSize = size;
}

void Memfile_Rewind(Memfile* this) {
    this->seekPoint = 0;
}

int Memfile_Write(Memfile* this, const void* src, size_t size) {
    const size_t osize = size;
    
    if (!this->memSize)
        Memfile_Alloc(this, size * 4);
    
    if (src == NULL) {
        _log("Trying to write 0 size");
        
        return 0;
    }
    
    size = clamp_max(size, clamp_min(this->memSize - this->seekPoint, 0));
    
    if (size != osize) {
        Memfile_Realloc(this, this->seekPoint + (this->memSize * 2) + (osize * 2));
        size = osize;
    }
    
    memcpy(&this->cast.u8[this->seekPoint], src, size);
    this->seekPoint += size;
    this->size = Max(this->size, this->seekPoint);
    
    if (this->param.align)
        Memfile_Align(this, this->param.align);
    
    return size;
}

int Memfile_WriteFile(Memfile* this, const char* source) {
    FILE* f;
    char buffer[256];
    size_t size;
    size_t amount = 0;
    
    if (!(f = fopen(source, "rb")))
        return false;
    
    while ((size = fread(buffer, 1, sizeof(buffer), f))) {
        if (Memfile_Write(this, buffer, size) != size)
            errr("Memfile: Could not successfully write from fread [%s]", source);
        amount += size;
    }
    
    fclose(f);
    
    return amount;
}

int Memfile_Insert(Memfile* this, const void* src, size_t size) {
    size_t remasize = this->size - this->seekPoint;
    
    if (this->size + size + 1 >= this->memSize)
        Memfile_Realloc(this, this->memSize * 2 + size * 2 + this->seekPoint);
    
    memmove(&this->cast.u8[this->seekPoint + size], &this->cast.u8[this->seekPoint], remasize + 1);
    memcpy(&this->cast.u8[this->seekPoint], src, size);
    this->size += size;
    
    return 0;
}

int Memfile_Append(Memfile* this, Memfile* src) {
    return Memfile_Write(this, src->data, src->size);
}

void Memfile_Align(Memfile* this, size_t align) {
    if ((this->seekPoint % align) != 0) {
        const u64 wow[32] = {};
        size_t size = align - (this->seekPoint % align);
        
        Memfile_Write(this, wow, size);
    }
}

int Memfile_Fmt(Memfile* this, const char* fmt, ...) {
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
    
    size = Memfile_Write(this, buffer, size + 1);
    this->seekPoint--;
    this->size--;
    this->cast.u8[this->seekPoint] = '\0';
    
    return size;
}

int Memfile_Read(Memfile* this, void* dest, size_t size) {
    size_t nsize = clamp_max(size, clamp_min(this->size - this->seekPoint, 0));
    
    if (nsize != size)
        _log("%d == src->seekPoint = %d / %d", nsize, this->seekPoint, this->seekPoint);
    
    if (nsize < 1)
        return 0;
    
    memcpy(dest, &this->cast.u8[this->seekPoint], nsize);
    this->seekPoint += nsize;
    
    return nsize;
}

void* Memfile_Seek(Memfile* this, size_t seek) {
    if (seek == MEMFILE_SEEK_END)
        seek = this->size;
    
    if (seek > this->size)
        return NULL;
    
    this->seekPoint = seek;
    
    return (void*)&this->cast.u8[seek];
}

void Memfile_LoadMem(Memfile* this, void* data, size_t size) {
    Memfile_Validate(this);
    Memfile_Null(this);
    this->size = this->memSize = size;
    this->data = data;
}

int Memfile_LoadBin(Memfile* this, const char* filepath) {
    FILE* file = Memfile_Fopen(filepath, "rb");
    size_t tempSize;
    
    if (file == NULL) {
        _log("Could not fopen file [%s]", filepath);
        
        if (this->param.throwError) {
            if (sys_stat(this->info.name))
                Memfile_Throw(this, "Could not load file!", x_fmt("Arg: [%s]", filepath));
            Memfile_Throw(this, "Can't load because file does not exist!", x_fmt("Arg: [%s]", filepath));
        }
        
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    tempSize = ftell(file);
    
    Memfile_Validate(this);
    Memfile_Null(this);
    
    if (this->data == NULL)
        Memfile_Alloc(this, tempSize + 0x10);
    
    else if (this->memSize < tempSize)
        Memfile_Realloc(this, tempSize * 2);
    
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

int Memfile_LoadStr(Memfile* this, const char* filepath) {
    FILE* file = Memfile_Fopen(filepath, "r");
    size_t tempSize;
    
    if (file == NULL) {
        _log("Could not fopen file [%s]", filepath);
        
        if (this->param.throwError) {
            if (sys_stat(this->info.name))
                Memfile_Throw(this, "Could not load file!", x_fmt("Arg: [%s]", filepath));
            Memfile_Throw(this, "Can't load because file does not exist!", x_fmt("Arg: [%s]", filepath));
        }
        
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    tempSize = ftell(file);
    
    Memfile_Validate(this);
    Memfile_Null(this);
    
    if (this->data == NULL)
        Memfile_Alloc(this, tempSize + 0x10);
    
    else if (this->memSize < tempSize)
        Memfile_Realloc(this, tempSize * 2);
    
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

int Memfile_SaveBin(Memfile* this, const char* filepath) {
    FILE* file = Memfile_Fopen(filepath, "wb");
    
    if (file == NULL) {
        _log("Could not fopen file [%s]", filepath);
        
        if (this->param.throwError) {
            if (sys_stat(this->info.name))
                Memfile_Throw(this, "Can't save over file!", x_fmt("Arg: [%s]", filepath));
            Memfile_Throw(this, "Can't save file!", x_fmt("Arg: [%s]", filepath));
        }
        
        return 1;
    }
    
    if (this->size)
        fwrite(this->data, sizeof(char), this->size, file);
    fclose(file);
    
    return 0;
}

int Memfile_SaveStr(Memfile* this, const char* filepath) {
    FILE* file = Memfile_Fopen(filepath, "w");
    
    if (file == NULL) {
        _log("Could not fopen file [%s]", filepath);
        
        if (this->param.throwError) {
            if (sys_stat(this->info.name))
                Memfile_Throw(this, "Can't save over file!", x_fmt("Arg: [%s]", filepath));
            Memfile_Throw(this, "Can't save file!", x_fmt("Arg: [%s]", filepath));
        }
        
        return 1;
    }
    
    if (this->size)
        fwrite(this->data, sizeof(char), this->size, file);
    fclose(file);
    
    return 0;
}

void Memfile_Free(Memfile* this) {
    if (this->param.initKey == 0xD0E0A0D0B0E0E0F0)
        free(this->data, this->info.name);
    
    *this = Memfile_New();
}

void Memfile_Null(Memfile* this) {
    this->size = 0;
    this->seekPoint = 0;
    if (this->data)
        this->str[0] = '\0';
}

void Memfile_Clear(Memfile* this) {
    memset(this->data, 0, this->memSize);
    Memfile_Null(this);
}
