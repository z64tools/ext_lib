
// # # # # # # # # # # # # # # # # # # # #
// # ZIPFILE                             #
// # # # # # # # # # # # # # # # # # # # #

#include "ext_zip.h"
#include <zip/src/zip.h>

void* Zip_Load(Zip* zip, const char* file, char mode) {
    zip->filename = strdup(file);
    
    switch (mode) {
        case ZIP_READ:
            return zip->pkg = zip_open(file, 0, mode);
            
        case ZIP_WRITE:
            return zip->pkg = zip_open(file, 9, mode);
            
        case ZIP_APPEND:
            return zip->pkg = zip_open(file, 0, mode);
    }
    
    errr("Unknown Zip Load Mode: [%c] [%s]", mode, file);
    return NULL;
}

int Zip_GetEntryNum(Zip* zip) {
    return zip_entries_total(zip->pkg);
}

int Zip_ReadByName(Zip* zip, const char* entry, Memfile* mem) {
    void* data = NULL;
    size_t sz;
    
    if (zip_entry_open(zip->pkg, entry))
        return ZIP_ERROR_OPEN_ENTRY;
    if (zip_entry_read(zip->pkg, &data, &sz) < 0) {
        if (zip_entry_close(zip->pkg))
            errr("Fatal Error: Entry Read & Entry Close failed for Zip \"%s\"", zip->filename);
        
        return ZIP_ERROR_RW_ENTRY;
    }
    
    mem->info.name = strdup(entry);
    
    if (zip_entry_close(zip->pkg))
        return ZIP_ERROR_CLOSE;
    
    Memfile_LoadMem(mem, data, sz);
    
    return 0;
}

int Zip_ReadByID(Zip* zip, size_t index, Memfile* mem) {
    void* data = NULL;
    size_t sz;
    
    if (zip_entry_openbyindex(zip->pkg, index))
        return ZIP_ERROR_OPEN_ENTRY;
    _log("Reading Entry \"%s\"", mem->info.name);
    
    if (zip_entry_isdir(zip->pkg)) {
        if (zip_entry_close(zip->pkg))
            return ZIP_ERROR_CLOSE;
        
        return 0;
    }
    
    if (zip_entry_read(zip->pkg, &data, &sz) < 0) {
        if (zip_entry_close(zip->pkg))
            errr("Fatal Error: Entry Read & Entry Close failed for Zip \"%s\"", zip->filename);
        
        return ZIP_ERROR_RW_ENTRY;
    }
    
    Memfile_LoadMem(mem, data, sz);
    mem->info.name = strdup(zip_entry_name(zip->pkg));
    
    if (zip_entry_close(zip->pkg))
        return ZIP_ERROR_CLOSE;
    
    return 0;
}

int Zip_ReadPath(Zip* zip, const char* path, int (*callback)(const char* name, Memfile* mem)) {
    u32 ent = zip_entries_total(zip->pkg);
    
    if (callback == NULL)
        errr("[Zip_ReadPath]: Please provide a callback function!");
    
    for (u32 i = 0; i < ent; i++) {
        const char* name;
        int ret;
        int brk = 0;
        
        if (zip_entry_openbyindex(zip->pkg, i))
            return ZIP_ERROR_OPEN_ENTRY;
        if (zip_entry_isdir(zip->pkg)) {
            zip_entry_close(zip->pkg);
            continue;
        }
        name = strdup(zip_entry_name(zip->pkg));
        
        if (name == NULL)
            return -6;
        
        if (memcmp(path, name, strlen(path))) {
            zip_entry_close(zip->pkg);
            vfree(name);
            continue;
        }
        zip_entry_close(zip->pkg);
        
        Memfile mem = Memfile_New();
        
        if ((ret = Zip_ReadByID(zip, i, &mem)))
            return ret;
        
        if (callback(name, &mem))
            brk = true;
        
        Memfile_Free(&mem);
        vfree(name);
        
        if (brk)
            break;
    }
    
    return 0;
}

int Zip_Write(Zip* zip, Memfile* mem, const char* entry) {
    if (zip_entry_open(zip->pkg, entry))
        return ZIP_ERROR_OPEN_ENTRY;
    if (zip_entry_write(zip->pkg, mem->data, mem->size))
        return ZIP_ERROR_RW_ENTRY;
    if (zip_entry_close(zip->pkg))
        return ZIP_ERROR_CLOSE;
    
    return 0;
}

static int Zip_DumpNoCall(Zip* zip, Memfile* mem, u32 i) {
    int ret;
    
    _log("ReadEntryIndex");
    if ((ret = Zip_ReadByID(zip, i, mem)))
        return ret;
    
    _log("SaveFile \"%s\"", fs_item(mem->info.name));
    if (Memfile_SaveBin(mem, fs_item(mem->info.name)))
        return ZIP_ERROR_RW_ENTRY;
    
    Memfile_Free(mem);
    
    return 0;
}

static int Zip_DumpCall(Zip* zip, Memfile* mem, u32 i, f32 prcnt, int (*callback)(const char* name, f32 prcnt)) {
    char* name;
    int isDir;
    
    if (!callback)
        return Zip_DumpNoCall(zip, mem, i);
    
    _log("Get Name");
    if (zip_entry_openbyindex(zip->pkg, i))
        return ZIP_ERROR_OPEN_ENTRY;
    name = strdup(zip_entry_name(zip->pkg));
    isDir = zip_entry_isdir(zip->pkg);
    if (zip_entry_close(zip->pkg))
        return ZIP_ERROR_CLOSE;
    
    if (!callback(name, prcnt)) {
        if (isDir) {
            sys_mkdir(fs_item(name));
            vfree(name);
            
            return 0;
        }
        
        vfree(name);
        return Zip_DumpNoCall(zip, mem, i);
    }
    
    vfree(name);
    
    return 0;
}

int Zip_Dump(Zip* zip, const char* path, int (*callback)(const char* name, f32 prcnt)) {
    Memfile mem = Memfile_New();
    u32 ent = zip_entries_total(zip->pkg);
    int ret;
    
    _log("Entries: %d", ent);
    for (u32 i = 0; i < ent; i++) {
        fs_set(path);
        if ((ret = Zip_DumpCall(zip, &mem, i, ((f32)(i + 1) / ent) * 100.0f, callback)))
            return ret;
    }
    
    return 0;
}

void Zip_Free(Zip* zip) {
    vfree(zip->filename);
    zip_close(zip->pkg);
}
