#include <ext_geogrid.h>

// # # # # # # # # # # # # # # # # # # # #
// # PropEnum                            #
// # # # # # # # # # # # # # # # # # # # #

#undef PropEnum_InitList

static char* PropEnum_Get(PropEnum* this, s32 i) {
	char** list = this->list;
	
	if (i >= this->num) {
		Log("OOB Access %d / %d", i, this->num);
		
		return NULL;
	}
	
	if (list == NULL) {
		Log("NULL List");
		
		return NULL;
	}
	
	return list[i];
}

static void PropEnum_Set(PropEnum* this, s32 i) {
	this->key = Clamp(i, 0, this->num - 1);
	
	if (i != this->key)
		Log("Out of range set!");
}

PropEnum* PropEnum_Init(s32 defaultVal) {
	PropEnum* this = Calloc(sizeof(PropEnum));
	
	this->get = PropEnum_Get;
	this->set = PropEnum_Set;
	this->key = defaultVal;
	
	return this;
}

PropEnum* PropEnum_InitList(s32 def, s32 num, ...) {
	PropEnum* prop = PropEnum_Init(def);
	va_list va;
	
	va_start(va, num);
	
	for (s32 i = 0; i < num; i++)
		PropEnum_Add(prop, va_arg(va, char*));
	
	va_end(va);
	
	return prop;
}

void PropEnum_Add(PropEnum* this, char* item) {
	this->list = Realloc(this->list, sizeof(char*) * (this->num + 1));
	this->list[this->num++] = StrDup(item);
}

void PropEnum_Insert(PropEnum* this, char* item, s32 slot) {
	this->list = Realloc(this->list, sizeof(char*) * (this->num + 1));
	ArrMoveR(this->list, slot, this->num - slot);
	this->list[slot] = StrDup(item);
}

void PropEnum_Remove(PropEnum* this, s32 key) {
	Assert(this->list);
	
	this->num--;
	Free(this->list[key]);
	for (s32 i = key; i < this->num; i++)
		this->list[i] = this->list[i + 1];
}

void PropEnum_Free(PropEnum* this) {
	if (!this)
		return;
	for (s32 i = 0; i < this->num; i++)
		Free(this->list[i]);
	Free(this->list);
	Free(this);
}
