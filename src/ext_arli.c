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

void* Arli_At(Arli* this, off_t position) {
    if (!this->begin || position >= this->num)
        return NULL;
    
    return this->begin + (this->elemSize * position);
}

void* Arli_Set(Arli* this, off_t position) {
    if (this->num)
        this->cur = clamp(position, 0, this->num - 1);
    else
        this->cur = 0;
    
    return Arli_At(this, this->cur);
}

int Arli_Alloc(Arli* this, size_t num) {
    size_t new_max = this->num + num;
    
    if (new_max <= this->max) return 1;
    u8* new_data = realloc(this->begin, this->elemSize * new_max);
    if (!new_data) return 0;
    
    this->begin = new_data;
    this->rend = new_data - this->elemSize;
    this->end = this->begin + this->elemSize * this->num;
    this->rbegin = this->end - this->elemSize;
    this->max = new_max;
    return 1;
}

void* Arli_Insert(Arli* this, off_t position, size_t num, const void* data) {
    if (num == 0) return this->begin ? this->begin : NULL;
    if (position > this->num) return NULL;
    
    size_t new_max = this->max;
    
    if (new_max == 0) new_max = num;
    else {
        if (new_max < this->num + num)
            new_max *= 2;
        if (new_max < this->num + num)
            new_max = this->num + num;
    }
    
    if (new_max != this->max) {
        u8* new_data = realloc(this->begin, this->elemSize * new_max);
        if (!new_data) return NULL;
        this->begin = new_data;
        this->rend = new_data - this->elemSize;
        this->max = new_max;
    }
    
    memmove(this->begin + this->elemSize * (position + num),
        this->begin + this->elemSize * position,
        this->elemSize * (this->num - position));
    
    if (data) memcpy(
            this->begin + this->elemSize * position,
            data,
            this->elemSize * num);
    
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

void Arli_ToBuf(Arli* this, const void* data) {
    memcpy(this->copybuf, data, this->elemSize);
}

void Arli_CopyToBuf(Arli* this, off_t position) {
    Arli_ToBuf(this, Arli_At(this, position));
}

void Arli_RemoveToBuf(Arli* this, off_t position) {
    Arli_ToBuf(this, Arli_At(this, position));
    Arli_Remove(this, position, 1);
}

int Arli_Remove(Arli* this, off_t position, size_t num) {
    if (!this->begin || num > this->num || position >= this->num)
        return 0;
    
    if (num == this->num) {
        Arli_Clear(this);
        return 1;
    }
    
    if (position + num != this->num)
        memmove(this->begin + this->elemSize * position,
            this->begin + this->elemSize * (position + num),
            this->elemSize * (this->num - position - num));
    
    this->num -= num;
    this->end = this->begin + this->elemSize * this->num;
    this->rbegin = this->end - this->elemSize;
    
    Arli_Set(this, this->cur);
    
    return 1;
}

int Arli_Shrink(Arli* this) {
    size_t new_max = this->num;
    u8* new_data = realloc(this->begin, this->elemSize * new_max);
    
    if (new_max > 0 && !new_data) return 0;
    
    this->begin = new_data;
    this->rend = new_data - this->elemSize;
    this->end = this->begin + this->elemSize * this->num;
    this->rbegin = this->end - this->elemSize;
    this->max = new_max;
    return 1;
}

void Arli_Free(Arli* this) {
    vfree(this->begin, this->copybuf);
    this->cur = this->num = 0;
}

void* Arli_Head(Arli* this) {
    return this->begin;
}

off_t Arli_IndexOf(Arli* this, void* elem) {
    if (elem >= (void*)(this->begin) && elem < (void*)(this->begin + this->elemSize * this->num))
        return (off_t)(elem - (void*)this->begin) / this->elemSize;
    return -1;
}
