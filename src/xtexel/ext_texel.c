#ifndef __clang__
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIDEF  static
#define STBIWDEF static
#endif
#include "__xtexread.h"
#include "__xtexwrite.h"

#include <ext_texel.h>

#define TEX_KEY "OKKek2ldMHXTqEpmI10e"

static const char sTexKey[20] = TEX_KEY;

static bool Image_Validate(Image* this) {
    return !memcmp(this->key, sTexKey, sizeof(sTexKey));
}

Image Image_New(void) {
    static const Image n = {
        .key        = TEX_KEY,
        .throwError = true,
    };
    
    return n;
}

void Image_Load(Image* this, const char* file) {
    int channels;
    uint8_t* data;
    
    if (!Image_Validate(this)) *this = Image_New();
    
    data = stbi_load(file, &this->x, &this->y, &channels, 4);
    if (!data && this->throwError) errr("Failed to load texel [%s]", file);
    
    if (!this->data) this->data = data;
    else {
        memcpy(this->data, data, channels * this->x * this->y);
        vfree(data);
    }
    
    this->size = this->x * this->y * channels;
}

void Image_Save(Image* this, const char* file) {
    if (!Image_Validate(this))
        errr("uninitialized texel save to [%s]", file);
    if (!striend(file, ".png"))
        errr("can't save image to [%s] format", x_filename(file) + strlen(x_basename(file)));
    stbi_write_png(file, this->x, this->y, 4, this->data, 0);
}

void Image_LoadMem(Image* this, const void* data, size_t size) {
    int channels;
    
    if (!Image_Validate(this)) *this = Image_New();
    
    this->data = stbi_load_from_memory(data, size, &this->x, &this->y, &channels, 4);
    if (!this->data && this->throwError) errr("Failed to load texel from memory");
}

void Image_Alloc(Image* this, int x, int y, int channels) {
    this->data = realloc(this->data, x * y * channels * 2);
    this->size = x * y * channels;
    this->x = x; this->y = y;
}

void Image_Free(Image* this) {
    vfree(this->data);
    memset(this, 0, sizeof(*this));
}
