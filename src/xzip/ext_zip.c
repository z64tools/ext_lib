
// # # # # # # # # # # # # # # # # # # # #
// # ZIPFILE                             #
// # # # # # # # # # # # # # # # # # # # #

#include "ext_zip.h"
#include <zip/src/zip.h>

void* zip_load(zip_file_t* zip, const char* file, zip_param_t mode) {
    zip->filename = strdup(file);
    
    switch (mode) {
        case ZIP_READ:
            return zip->pkg = zip_open(file, 0, mode);
            
        case ZIP_WRITE:
            return zip->pkg = zip_open(file, 9, mode);
            
        case ZIP_APPEND:
            return zip->pkg = zip_open(file, 0, mode);
    }
    
    print_error("Unknown zip_file_t Load Mode: [%c] [%s]", mode, file);
    return NULL;
}

int zip_get_num_entries(zip_file_t* zip) {
    return zip_entries_total(zip->pkg);
}

int zip_read_name(zip_file_t* zip, const char* entry, memfile_t* mem) {
    void* data = NULL;
    size_t sz;
    
    if (zip_entry_open(zip->pkg, entry))
        return ZIP_ERROR_OPEN_ENTRY;
    if (zip_entry_read(zip->pkg, &data, &sz) < 0) {
        if (zip_entry_close(zip->pkg))
            print_error("Fatal Error: Entry Read & Entry Close failed for zip_file_t \"%s\"", zip->filename);
        
        return ZIP_ERROR_RW_ENTRY;
    }
    
    mem->info.name = strdup(entry);
    
    if (zip_entry_close(zip->pkg))
        return ZIP_ERROR_CLOSE;
    
    memfile_load_mem(mem, data, sz);
    
    return 0;
}

int zip_read_index(zip_file_t* zip, size_t index, memfile_t* mem) {
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
            print_error("Fatal Error: Entry Read & Entry Close failed for zip_file_t \"%s\"", zip->filename);
        
        return ZIP_ERROR_RW_ENTRY;
    }
    
    memfile_load_mem(mem, data, sz);
    mem->info.name = strdup(zip_entry_name(zip->pkg));
    
    if (zip_entry_close(zip->pkg))
        return ZIP_ERROR_CLOSE;
    
    return 0;
}

int zip_read_path(zip_file_t* zip, const char* path, int (*callback)(const char* name, memfile_t* mem)) {
    u32 ent = zip_entries_total(zip->pkg);
    
    if (callback == NULL)
        print_error("[zip_read_path]: Please provide a callback function!");
    
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
            free(name);
            continue;
        }
        zip_entry_close(zip->pkg);
        
        memfile_t mem = memfile_new();
        
        if ((ret = zip_read_index(zip, i, &mem)))
            return ret;
        
        if (callback(name, &mem))
            brk = true;
        
        memfile_free(&mem);
        free(name);
        
        if (brk)
            break;
    }
    
    return 0;
}

int zip_write(zip_file_t* zip, memfile_t* mem, const char* entry) {
    if (zip_entry_open(zip->pkg, entry))
        return ZIP_ERROR_OPEN_ENTRY;
    if (zip_entry_write(zip->pkg, mem->data, mem->size))
        return ZIP_ERROR_RW_ENTRY;
    if (zip_entry_close(zip->pkg))
        return ZIP_ERROR_CLOSE;
    
    return 0;
}

static int zip_dump_nocall(zip_file_t* zip, memfile_t* mem, u32 i) {
    int ret;
    
    _log("ReadEntryIndex");
    if ((ret = zip_read_index(zip, i, mem)))
        return ret;
    
    _log("SaveFile \"%s\"", fs_item(mem->info.name));
    if (memfile_save_bin(mem, fs_item(mem->info.name)))
        return ZIP_ERROR_RW_ENTRY;
    
    memfile_free(mem);
    
    return 0;
}

static int zip_dump_call(zip_file_t* zip, memfile_t* mem, u32 i, f32 prcnt, int (*callback)(const char* name, f32 prcnt)) {
    char* name;
    int isDir;
    
    if (!callback)
        return zip_dump_nocall(zip, mem, i);
    
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
            free(name);
            
            return 0;
        }
        
        free(name);
        return zip_dump_nocall(zip, mem, i);
    }
    
    free(name);
    
    return 0;
}

int zip_dump(zip_file_t* zip, const char* path, int (*callback)(const char* name, f32 prcnt)) {
    memfile_t mem = memfile_new();
    u32 ent = zip_entries_total(zip->pkg);
    int ret;
    
    _log("Entries: %d", ent);
    for (u32 i = 0; i < ent; i++) {
        fs_set(path);
        if ((ret = zip_dump_call(zip, &mem, i, ((f32)(i + 1) / ent) * 100.0f, callback)))
            return ret;
    }
    
    return 0;
}

void zip_free(zip_file_t* zip) {
    free(zip->filename);
    zip_close(zip->pkg);
}
