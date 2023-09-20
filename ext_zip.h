#ifndef EXT_ZIP_H
#define EXT_ZIP_H

#include "ext_lib.h"

typedef struct Zip {
	void* pkg;
	char* filename;
} Zip;

enum {
	ZIP_READ   = 'r',
	ZIP_WRITE  = 'w',
	ZIP_APPEND = 'a',
};

enum {
	ZIP_ERROR_OPEN_ENTRY = 1,
	ZIP_ERROR_RW_ENTRY   = -1,
	ZIP_ERROR_CLOSE      = -2,
};

void* Zip_Load(Zip* zip, const char* file, char mode);
int Zip_GetEntryNum(Zip* zip);
int Zip_ReadByName(Zip* zip, const char* entry, Memfile* mem);
int Zip_ReadByID(Zip* zip, size_t index, Memfile* mem);
int Zip_ReadPath(Zip* zip, const char* path, int (*callback)(const char* name, Memfile* mem));
int Zip_Write(Zip* zip, Memfile* mem, const char* entry);
int Zip_Dump(Zip* zip, const char* path, int (*callback)(const char* name, f32 prcnt));
void Zip_Free(Zip* zip);

#endif