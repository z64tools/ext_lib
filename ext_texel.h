#ifndef EXT_TEXEL_H
#define EXT_TEXEL_H

#include <ext_lib.h>
#include <ext_vector.h>

typedef struct {
    char key[20];
    s32  x, y;
    u8*  data;
    
    struct {
        bool throwError : 1;
    };
} TexFile;

void TexFile_LoadFile(TexFile* this, const char* file);
void TexFile_LoadMem(TexFile* this, const void* data, u32 size);
void TexFile_Free(TexFile* this);

#endif