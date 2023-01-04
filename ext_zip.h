#ifndef EXT_ZIP_H
#define EXT_ZIP_H

#include "ext_lib.h"

typedef struct zip_file_t {
    void* pkg;
    char* filename;
} zip_file_t;

typedef enum {
    ZIP_READ   = 'r',
    ZIP_WRITE  = 'w',
    ZIP_APPEND = 'a',
} zip_param_t;

typedef enum {
    ZIP_ERROR_OPEN_ENTRY = 1,
    ZIP_ERROR_RW_ENTRY   = -1,
    ZIP_ERROR_CLOSE      = -2,
} zip_return_t;

void* zip_load(zip_file_t* zip, const char* file, zip_param_t mode);
int zip_get_num_entries(zip_file_t* zip);
int zip_read_name(zip_file_t* zip, const char* entry, memfile_t* mem);
int zip_read_index(zip_file_t* zip, size_t index, memfile_t* mem);
int zip_read_path(zip_file_t* zip, const char* path, int (*callback)(const char* name, memfile_t* mem));
int zip_write(zip_file_t* zip, memfile_t* mem, const char* entry);
int zip_dump(zip_file_t* zip, const char* path, int (*callback)(const char* name, f32 prcnt));
void zip_free(zip_file_t* zip);

#endif