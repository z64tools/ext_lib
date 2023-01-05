#include <ext_texel.h>

#include "__xtexread.h"
#include "__xtexwrite.h"

#define TEX_KEY "OKKek2ldMHXTqEpmI10e"

static const char sTexKey[20] = TEX_KEY;

static bool texel_validate(texel_t* this) {
    return !memcmp(this->key, sTexKey, sizeof(sTexKey));
}

texel_t texel_new(void) {
    static const texel_t n = {
        .key        = TEX_KEY,
        .throwError = true,
    };
    
    return n;
}

void texel_load(texel_t* this, const char* file) {
    int channels;
    uint8_t* data;
    
    if (!texel_validate(this)) *this = texel_new();
    
    data = stbi_load(file, &this->x, &this->y, &channels, 4);
    if (!data && this->throwError) errr("Failed to load texel [%s]", file);
    
    if (!this->data) this->data = data;
    else {
        memcpy(this->data, data, channels * this->x * this->y);
        free(data);
    }
    
    this->size = this->x * this->y * channels;
}

void texel_save(texel_t* this, const char* file) {
    if (!texel_validate(this))
        errr("uninitialized texel save to [%s]", file);
    if (!striend(file, ".png"))
        errr("can't save image to [%s] format", x_filename(file) + strlen(x_basename(file)));
    stbi_write_png(file, this->x, this->y, 4, this->data, 0);
}

void texel_loadmem(texel_t* this, const void* data, size_t size) {
    int channels;
    
    if (!texel_validate(this)) *this = texel_new();
    
    this->data = stbi_load_from_memory(data, size, &this->x, &this->y, &channels, 4);
    if (!this->data && this->throwError) errr("Failed to load texel from memory");
}

void texel_alloc(texel_t* this, int x, int y, int channels) {
    this->data = realloc(this->data, x * y * channels * 2);
    this->size = x * y * channels;
    this->x = x; this->y = y;
}

void texel_free(texel_t* this) {
    free(this->data);
    memset(this, 0, sizeof(*this));
}
