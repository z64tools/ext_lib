#include <ext_lib.h>
#undef Arli_New

/*
 * Made by glankk
 * https://github.com/glankk/n64/tree/master/include/vector
 */

Arli Arli_New(size_t element_size) {
    Arli this = { .element_size = element_size };
    
    return this;
}

void Arli_Clear(Arli* this) {
    this->size = 0;
    this->end = this->rend = this->rbegin = this->begin;
}

void* Arli_At(Arli* this, size_t position) {
    if (!this->begin || position >= this->size)
        return NULL;
    
    return this->begin + this->element_size * position;
}

int Arli_Alloc(Arli* this, size_t num) {
    size_t new_cap = this->size + num;
    
    if (new_cap <= this->capacity) return 1;
    u8* new_data = realloc(this->begin, this->element_size * new_cap);
    if (!new_data) return 0;
    
    this->begin = new_data;
    this->rend = new_data - this->element_size;
    this->end = this->begin + this->element_size * this->size;
    this->rbegin = this->end - this->element_size;
    this->capacity = new_cap;
    return 1;
}

void* Arli_Insert(Arli* this, size_t position, size_t num, const void* data) {
    if (num == 0) return this->begin ? this->begin : NULL;
    if (position > this->size) return NULL;
    
    size_t new_cap = this->capacity;
    
    if (new_cap == 0) new_cap = num;
    else {
        if (new_cap < this->size + num)
            new_cap *= 2;
        if (new_cap < this->size + num)
            new_cap = this->size + num;
    }
    
    if (new_cap != this->capacity) {
        u8* new_data = realloc(this->begin, this->element_size * new_cap);
        if (!new_data) return NULL;
        this->begin = new_data;
        this->rend = new_data - this->element_size;
        this->capacity = new_cap;
    }
    
    memmove(this->begin + this->element_size * (position + num),
        this->begin + this->element_size * position,
        this->element_size * (this->size - position));
    
    if (data) memcpy(
            this->begin + this->element_size * position,
            data, this->element_size * num);
    
    this->size += num;
    this->end = this->begin + this->element_size * this->size;
    this->rbegin = this->end - this->element_size;
    
    return this->begin + this->element_size * position;
}

void* Arli_Append(Arli* this, size_t num, const void* data) {
    return Arli_Insert(this, this->size, num, data);
}

void* Arli_Add(Arli* this, const void* data) {
    return Arli_Insert(this, this->size, 1, data);
}

int Arli_Remove(Arli* this, size_t position, size_t num) {
    if (!this->begin || num > this->size || position >= this->size)
        return 0;
    if (num == this->size) {
        Arli_Clear(this);
        return 1;
    }
    memmove(this->begin + this->element_size * position,
        this->begin + this->element_size * (position + num),
        this->element_size * (this->size - position - num));
    this->size -= num;
    this->end = this->begin + this->element_size * this->size;
    this->rbegin = this->end - this->element_size;
    return 1;
}

int Arli_Shrink(Arli* this) {
    size_t new_cap = this->size;
    u8* new_data = realloc(this->begin, this->element_size * new_cap);
    
    if (new_cap > 0 && !new_data) return 0;
    
    this->begin = new_data;
    this->rend = new_data - this->element_size;
    this->end = this->begin + this->element_size * this->size;
    this->rbegin = this->end - this->element_size;
    this->capacity = new_cap;
    return 1;
}

void Arli_Destroy(Arli* this) {
    vfree(this->begin, this);
}
