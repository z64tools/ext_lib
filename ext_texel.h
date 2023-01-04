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
} texel_t;

texel_t texel_new(void);
void texel_load(texel_t* this, const char* file);
void texel_save(texel_t* this, const char* file);
void texel_loadmem(texel_t* this, const void* data, size_t size);
void texel_set(texel_t* this, int x, int y);
void texel_alloc(texel_t* this, size_t sz);
void texel_free(texel_t* this);

#endif