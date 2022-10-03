#ifndef EXT_ZIP_H
#define EXT_ZIP_H

#include "ext_lib.h"

typedef struct ZipFile {
    void* pkg;
    char* filename;
} ZipFile;

typedef enum {
    ZIP_READ   = 'r',
    ZIP_WRITE  = 'w',
    ZIP_APPEND = 'a',
} ZipParam;

typedef enum {
    ZIP_ERROR_OPEN_ENTRY = 1,
    ZIP_ERROR_RW_ENTRY   = -1,
    ZIP_ERROR_CLOSE      = -2,
} ZipReturn;

void* ZipFile_Load(ZipFile* zip, const char* file, ZipParam mode);
u32 ZipFile_GetEntriesNum(ZipFile* zip);
s32 ZipFile_ReadEntry_Name(ZipFile* zip, const char* entry, MemFile* mem);
s32 ZipFile_ReadEntry_Index(ZipFile* zip, Size index, MemFile* mem);
s32 ZipFile_ReadEntries_Path(ZipFile* zip, const char* path, s32 (*callback)(const char* name, MemFile* mem));
s32 ZipFile_WriteEntry(ZipFile* zip, MemFile* mem, char* entry);
s32 ZipFile_Extract(ZipFile* zip, const char* path, s32 (*callback)(const char* name, f32 prcnt));
void ZipFile_Free(ZipFile* zip);

#endif