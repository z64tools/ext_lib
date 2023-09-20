#include <ext_lib.h>
#undef Kval_New

Kval Kval_New(size_t elemsize) {
	Kval new = {};
	
	new.vsize = elemsize;
	
	return new;
}

void Kval_Alloc(Kval* this, size_t num) {
	int addnum = (num - this->capacity) * 2;
	int newCap = (this->capacity + addnum);
	int diffCap = newCap - this->capacity;
	
	if (addnum <= 0) return;
	
	renew(this->key, void*[newCap]);
	renew(this->val, void*[newCap]);
	memset(this->key + this->capacity, 0, sizeof(char*[diffCap]));
	memset(this->val + this->capacity, 0, sizeof(char*[diffCap]));
	
	this->capacity = newCap;
}

void* Kval_Add(Kval* this, const char* key, const void* val) {
	if (Kval_IndexOfKey(this, key) > -1)
		return NULL;
	
	Kval_Alloc(this, this->num + 1);
	
	this->key[this->num] = strdup(key);
	this->val[this->num] = new(char[this->vsize]);
	if (val) memcpy(this->val[this->num], val, this->vsize);
	
	return this->val[this->num++];
}

static int Kval_RmIndex(Kval* this, int index) {
	if (index < 0 || index >= this->num)
		return false;
	
	delete(this->key[index]);
	delete(this->val[index]);
	
	size_t remain = this->num - index;
	
	memshift(&this->key[index], -1, sizeof(void*), remain);
	memshift(&this->val[index], -1, sizeof(void*), remain);
	
	this->num--;
	
	return true;
}

int Kval_RmKey(Kval* this, const char* key) {
	return Kval_RmIndex(this, Kval_IndexOfKey(this, key));
}

int Kval_RmVal(Kval* this, const void* val) {
	return Kval_RmIndex(this, Kval_IndexOfVal(this, val));
}

int Kval_IndexOfKey(Kval* this, const char* key) {
	for (int i = 0; i < this->num; i++)
		if (streq(key, this->key[i]))
			return i;
	return -1;
}

int Kval_IndexOfVal(Kval* this, const void* val) {
	for (int i = 0; i < this->num; i++)
		if (val == this->val[i])
			return i;
	return -1;
}

void* Kval_Index(Kval* this, int index) {
	if (index > -1 && index < this->num)
		return this->val[index];
	return NULL;
}

const char* Kval_Key(Kval* this, const void* val) {
	int id = Kval_IndexOfVal(this, val);
	
	if (id == -1 ) return NULL;
	
	return this->key[id];
}

void* Kval_Find(Kval* this, const char* key) {
	return Kval_Index(this, Kval_IndexOfKey(this, key));
}

void Kval_Clear(Kval* this) {
	for (int i = 0; i < this->num; i++)
		delete(this->key[i], this->val[i]);
	delete(this->key, this->val);
}

void Kval_Free(Kval* this) {
	Kval_Clear(this);
	memzero(this);
}
