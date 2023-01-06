#ifndef EXT_TEXEL_H
#define EXT_TEXEL_H

#include <ext_lib.h>
#include <ext_vector.h>

typedef struct {
    char     key[20];
    int      x, y, cnum;
    uint8_t* data;
    size_t   size;
    
    struct {
        bool throwError : 1;
    };
} Image;

Image Image_New(void);
void Image_Load(Image* this, const char* file);
void Image_Save(Image* this, const char* file);
void Image_LoadMem(Image* this, const void* data, size_t size);
void Image_Alloc(Image* this, int x, int y, int channels);
void Image_Free(Image* this);

#endif
