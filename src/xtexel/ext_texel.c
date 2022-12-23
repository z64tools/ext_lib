#include <ext_texel.h>
#define STBIDEF  static
#define STBIWDEF static
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#undef _MSC_VER
#include "__xtexread.h"
#include "__xtexwrite.h"

#define TEX_KEY "OKKek2ldMHXTqEpmI10e"

static const char sTexKey[20] = TEX_KEY;

static bool TexFile_Validate(TexFile* this) {
    return !memcmp(this->key, sTexKey, sizeof(sTexKey));
}

TexFile TexFile_New(void) {
    static const TexFile n = {
        .key        = TEX_KEY,
        .throwError = true,
    };
    
    return n;
}

void TexFile_LoadFile(TexFile* this, const char* file) {
    int channels;
    
    if (!TexFile_Validate(this)) *this = TexFile_New();
    
    this->data = stbi_load(file, &this->x, &this->y, &channels, 4);
    if (!this->data && this->throwError) printf_error("Failed to load texel [%s]", file);
}

void TexFile_LoadMem(TexFile* this, const void* data, u32 size) {
    int channels;
    
    if (!TexFile_Validate(this)) *this = TexFile_New();
    
    this->data = stbi_load_from_memory(data, size, &this->x, &this->y, &channels, 4);
    if (!this->data && this->throwError) printf_error("Failed to load texel from memory");
}

void TexFile_Free(TexFile* this) {
    Free(this->data);
    memset(this, 0, sizeof(*this));
}
