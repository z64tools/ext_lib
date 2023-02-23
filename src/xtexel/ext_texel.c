#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#ifndef __clang__
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIDEF  static
#define STBIWDEF static
#endif
#include "__xtexread.h"
#include "__xtexwrite.h"

#pragma GCC diagnostic pop

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
    uint8_t* data;
    
    _log("Image_Load: %s", file);
    if (!Image_Validate(this)) *this = Image_New();
    
    data = stbi_load(file, &this->x, &this->y, &this->channels, 4);
    if (!data && this->throwError) errr("Failed to load texel [%s]", file);
    
    if (!this->data) this->data = data;
    else {
        memcpy(this->data, data, this->channels * this->x * this->y);
        vfree(data);
    }
    
    this->size = this->x * this->y * this->channels;
}

void Image_Save(Image* this, const char* file) {
    _log("Image_Save: %s", file);
    if (!Image_Validate(this))
        errr("uninitialized texel save to [%s]", file);
    if (!striend(file, ".png"))
        errr("can't save image to [%s] format", x_filename(file) + strlen(x_basename(file)));
    stbi_write_png_compression_level = !this->compress ? 8 : this->compress;
    stbi_write_png(file, this->x, this->y, 4, this->data, 0);
}

void Image_LoadMem(Image* this, const void* data, size_t size) {
    if (!Image_Validate(this)) *this = Image_New();
    
    this->data = stbi_load_from_memory(data, size, &this->x, &this->y, &this->channels, 4);
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

void Image_Downscale(Image* this, int newx, int newy) {
    if (this->x == newx && this->y == newy)
        return;
    
    u8* new = new(u8[newy][newx][this->channels]);
    float x_ratio = (this->x - 1) / (float)(newx - 1);
    float y_ratio = (this->y - 1) / (float)(newy - 1);
    u8* pixels = this->data;
    u8* dest = new;
    
    for (int i = 0; i < newy; i++) {
        for (int j = 0; j < newx; j++) {
            int x = (int)(x_ratio * j);
            int y = (int)(y_ratio * i);
            f32 x_diff = (x_ratio * j) - x;
            f32 y_diff = (y_ratio * i) - y;
            int index = (y * this->x + x) * this->channels;
            
            f32 aMod = (1.0f - x_diff) * (1.0f - y_diff);
            f32 bMod = x_diff * (1.0f - y_diff);
            f32 cMod = y_diff * (1.0f - x_diff);
            f32 dMod = x_diff * y_diff;
            
            for (int chnl = 0; chnl < this->channels; chnl++) {
                u8 a = pixels[chnl + index];
                u8 b = pixels[chnl + index + this->channels];
                u8 c = pixels[chnl + index + this->x * this->channels];
                u8 d = pixels[chnl + index + this->x * this->channels + this->channels];
                
                *dest++ = a * aMod + b * bMod + c * cMod + d * dMod;
            }
        }
    }
    
    vfree(this->data);
    this->data = new;
    this->x = newx;
    this->y = newy;
}
