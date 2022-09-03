#ifndef _WIN32
#define EXT_OBJECT_C

#include <ext_lib.h>
#include <sys/mman.h>

static const unsigned char sTrampoline[] = {
	// MOV RDI, 0x0
	0x48, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	// JMP [RIP + 0]
	0xff, 0x25, 0x00, 0x00, 0x00, 0x00,
	// DQ 0x0
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void* Object_Method(Object* self, void* target, int argCount) {
	unsigned char opcode[][2] = {
		0x48, 0xbf,
		0x48, 0xbe,
		0x48, 0xba,
		0x48, 0xb9,
		0x49, 0xb8,
		0x49, 0xb9,
	};
	
	memcpy(self->codePagePtr, opcode[argCount], sizeof(opcode[argCount]));
	memcpy(self->codePagePtr + 2, &self, sizeof(void*));
	memcpy(self->codePagePtr + 10, sTrampoline + 10, sizeof(sTrampoline) - 10);
	memcpy(self->codePagePtr + 16, &target, sizeof(void*));
	self->codePagePtr += sizeof(sTrampoline);
	
	return self->codePagePtr - sizeof(sTrampoline);
}

static void Object_Destroy(Object* self) {
	Assert(!munmap(self->codePage, self->codePageSize));
	free(self);
}

void Object_Prepare(Object* self) {
	Assert(!mprotect(self->codePage, self->codePageSize, PROT_READ | PROT_EXEC));
}

void* Object_Create(Size size, s32 funcCount) {
	Object* self = Calloc(size);
	
	self->codePageSize = (funcCount + 1) * sizeof(sTrampoline);
	
	self->codePage = mmap(
		NULL, self->codePageSize,
		PROT_READ | PROT_WRITE,
		MAP_ANON | MAP_PRIVATE, -1, 0
	);
	self->codePagePtr = self->codePage;
	
	Assert(mmap != MAP_FAILED);
	self->destroy = Object_Method(self, Object_Destroy, 0);
	
	return self;
}

#endif