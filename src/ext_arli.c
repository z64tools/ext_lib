#include <ext_lib.h>
#undef Arli_New

/*
 * Made by glankk
 * https://github.com/glankk/n64/tree/master/include/vector
 */

static const char* Arli_DefaultPrint(Arli* this, size_t pos) {
    return x_fmt("<%d>", pos);
}

Arli Arli_New(size_t elemSize) {
    Arli this = { .elemSize = elemSize };
    
    this.elemName = Arli_DefaultPrint;
    this.copybuf = new(u8[elemSize]);
    
    return this;
}

void Arli_SetElemNameCallback(Arli* this, const char* (*callback)(Arli*, size_t)) {
    this->elemName = callback;
}

void Arli_Clear(Arli* this) {
    this->num = this->cur = 0;
    this->end = this->rend = this->rbegin = this->begin;
}

void* Arli_At(Arli* this, size_t position) {
    if (!this->begin || position >= this->num)
        return NULL;
    
    return this->begin + this->elemSize * position;
}

void* Arli_Set(Arli* this, size_t position) {
    void* r = Arli_At(this, position);
    
    if (r) this->cur = position;
    
    return r;
}

void* Arli_Get(Arli* this) {
    return Arli_At(this, this->cur);
}

int Arli_Alloc(Arli* this, size_t num) {
    size_t new_cap = this->num + num;
    
    if (new_cap <= this->max) return 1;
    u8* new_data = realloc(this->begin, this->elemSize * new_cap);
    if (!new_data) return 0;
    
    this->begin = new_data;
    this->rend = new_data - this->elemSize;
    this->end = this->begin + this->elemSize * this->num;
    this->rbegin = this->end - this->elemSize;
    this->max = new_cap;
    return 1;
}

void* Arli_Insert(Arli* this, size_t position, size_t num, const void* data) {
    if (num == 0) return this->begin ? this->begin : NULL;
    if (position > this->num) return NULL;
    
    size_t new_cap = this->max;
    
    if (new_cap == 0) new_cap = num;
    else {
        if (new_cap < this->num + num)
            new_cap *= 2;
        if (new_cap < this->num + num)
            new_cap = this->num + num;
    }
    
    if (new_cap != this->max) {
        u8* new_data = realloc(this->begin, this->elemSize * new_cap);
        if (!new_data) return NULL;
        this->begin = new_data;
        this->rend = new_data - this->elemSize;
        this->max = new_cap;
    }
    
    memmove(this->begin + this->elemSize * (position + num),
        this->begin + this->elemSize * position,
        this->elemSize * (this->num - position));
    
    if (data) memcpy(
            this->begin + this->elemSize * position,
            data, this->elemSize * num);
    
    this->num += num;
    this->end = this->begin + this->elemSize * this->num;
    this->rbegin = this->end - this->elemSize;
    
    return this->begin + this->elemSize * position;
}

void* Arli_AddX(Arli* this, size_t num, const void* data) {
    return Arli_Insert(this, this->num, num, data);
}

void* Arli_Add(Arli* this, const void* data) {
    return Arli_Insert(this, this->num, 1, data);
}

void Arli_CopyToBuf(Arli* this, const void* data) {
    memcpy(this->copybuf, data, this->elemSize);
}

void Arli_RemoveToBuf(Arli* this, size_t position) {
    Arli_CopyToBuf(this, Arli_At(this, position));
    Arli_Remove(this, position, 1);
}

int Arli_Remove(Arli* this, size_t position, size_t num) {
    if (!this->begin || num > this->num || position >= this->num)
        return 0;
    if (num == this->num) {
        Arli_Clear(this);
        return 1;
    }
    memmove(this->begin + this->elemSize * position,
        this->begin + this->elemSize * (position + num),
        this->elemSize * (this->num - position - num));
    this->num -= num;
    this->end = this->begin + this->elemSize * this->num;
    this->rbegin = this->end - this->elemSize;
    
    this->cur = clamp(this->cur - num, 0, this->num);
    
    return 1;
}

int Arli_Shrink(Arli* this) {
    size_t new_cap = this->num;
    u8* new_data = realloc(this->begin, this->elemSize * new_cap);
    
    if (new_cap > 0 && !new_data) return 0;
    
    this->begin = new_data;
    this->rend = new_data - this->elemSize;
    this->end = this->begin + this->elemSize * this->num;
    this->rbegin = this->end - this->elemSize;
    this->max = new_cap;
    return 1;
}

void Arli_Free(Arli* this) {
    vfree(this->begin, this->copybuf);
    this->cur = this->num = 0;
}

void* Arli_Head(Arli* this) {
    return this->begin;
}

size_t Arli_IndexOf(Arli* this, void* elem) {
    _assert(elem >= (void*)(this->begin) && elem < (void*)(this->begin + this->elemSize * this->num));
    
    return (off_t)(elem - (void*)this->begin) / this->elemSize;
}
