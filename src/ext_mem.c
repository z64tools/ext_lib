#include <ext_lib.h>

#undef Memfile_Set

// # # # # # # # # # # # # # # # # # # # #
// # MEMFILE                             #
// # # # # # # # # # # # # # # # # # # # #

#define DEBUG_LOG false

static u8 sZero[0x4000];

static void _throw_error(Memfile* this, const char* msg, const char* info) {
	osLogPrint();
	
	fprintf(stderr, "\n");
	
	warn_align("" PRNT_REDD "Work Directory" PRNT_RSET ":", PRNT_YELW "%s", sys_workdir());
	warn_align("" PRNT_REDD "Error" PRNT_RSET ":", "%s", msg);
	warn_align("" PRNT_REDD "Target" PRNT_RSET ":", "%s", info);
	if (info) {
		warn_align("" PRNT_REDD "Prev" PRNT_RSET ":", PRNT_YELW "%s", this->info.name);
		warn_align("" PRNT_REDD "size_t" PRNT_RSET ":", "%f kB", KbToBin(this->size));
		warn_align("" PRNT_REDD "MemSize" PRNT_RSET ":", "%f kB", KbToBin(this->memSize));
	}
	errr("Stopping Process");
}

static FILE* _open_file(const char* name, const char* mode) {
	FILE* file;
	
#if _WIN32
	wchar* name16 = calloc(strlen(name) * 4);
	wchar* mode16 = calloc(strlen(mode) * 4);
	strto16(name16, name);
	strto16(mode16, mode);
	
	file = _wfopen(name16, mode16);
	
	delete(name16, mode16);
#else
	file = fopen(name, mode);
#endif
	
	osLog("_open_file: %s", name);
	
	return file;
}

static void* _read_file(FILE* f) {
	osAssert(f != NULL);
	
	fseek(f, 0, SEEK_END);
	
	size_t sz = ftell(f);
	void* d = new(u8[sz + 2]);
	
	osAssert(d != NULL);
	
	rewind(f);
	int r = fread(d, 1, sz, f);
	if (r != sz) warn("odd file size diff: %.2fkb vs %.2fkb", BinToKb(r), BinToKb(sz));
	
	fclose(f);
	
	return d;
}

static void _validate_memfile(Memfile* this) {
	if (this->param.initKey == 0xD0E0A0D0B0E0E0F0) {
		
		return;
	}
	
	*this = Memfile_New();
}

Memfile Memfile_New() {
	return (Memfile) {
			   .param.initKey = 0xD0E0A0D0B0E0E0F0,
			   .param.throwError = true,
	};
}

void Memfile_Set(Memfile* this, ...) {
	va_list args;
	size_t cmd = 0;
	size_t arg = 0;
	
	if (this->param.initKey != 0xD0E0A0D0B0E0E0F0)
		*this = Memfile_New();
	
	va_start(args, this);
	for (;;) {
		cmd = va_arg(args, int);
		
		if (cmd == MEM_END)
			break;
		
		switch (cmd) {
			case MEM_ALIGN:
			case MEM_CRC32:
			case MEM_REALLOC:
				arg = va_arg(args, uaddr_t);
				break;
		}
		
		if (cmd == MEM_CLEAR) {
			cmd = arg;
			arg = 0;
		}
		
		switch (cmd) {
			case MEM_THROW_ERROR:
				this->param.throwError = true;
			case MEM_ALIGN:
				this->param.align = arg;
				break;
			case MEM_CRC32:
				osLog("Memfile_Set: deprecated feature [MEM_CRC32], [%s]", this->info.name);
				break;
			case MEM_REALLOC:
				osLog("Memfile_Set: deprecated feature [MEM_REALLOC], [%s]", this->info.name);
				break;
		}
	}
	va_end(args);
}

void Memfile_Alloc(Memfile* this, size_t size) {
	if (this->param.initKey != 0xD0E0A0D0B0E0E0F0) {
		*this = Memfile_New();
	} else if (this->data) {
		osLog("Memfile_Alloc: Mallocing already allocated Memfile [%s], freeing and reallocating!", this->info.name);
		Memfile_Free(this);
	}
	
	osAssert ((this->data = calloc(size)) != NULL);
	this->memSize = size;
}

void Memfile_Realloc(Memfile* this, size_t size) {
	if (this->param.initKey != 0xD0E0A0D0B0E0E0F0)
		*this = Memfile_New();
	if (this->memSize > size)
		return;
	
	osAssert((this->data = realloc(this->data, size)) != NULL);
	this->memSize = size;
}

void Memfile_Rewind(Memfile* this) {
	this->seekPoint = 0;
}

int Memfile_Write(Memfile* this, const void* src, size_t size) {
	const size_t osize = size;
	
	if (!this->memSize)
		Memfile_Alloc(this, size * 4);
	
	if (src == NULL) {
		osLog("Trying to write 0 size");
		
		return 0;
	}
	
	size = clamp_max(size, clamp_min(this->memSize - this->seekPoint, 0));
	
	if (size != osize) {
		Memfile_Realloc(this, this->seekPoint + (this->memSize * 2) + (osize * 2));
		size = osize;
	}
	
	memcpy(&this->cast.u8[this->seekPoint], src, size);
	this->seekPoint += size;
	this->size = Max(this->size, this->seekPoint);
	
	if (this->param.align)
		Memfile_Align(this, this->param.align);
	
	return size;
}

int Memfile_WritePad(Memfile* this, size_t size) {
	while (size > 0x4000) {
		Memfile_Write(this, sZero, 0x4000);
		size -= 0x4000;
	}
	
	if (size)
		return Memfile_Write(this, sZero, size);
	
	return 0;
}

int Memfile_WriteFile(Memfile* this, const char* source) {
	FILE* f;
	char buffer[256];
	size_t size;
	size_t amount = 0;
	
	if (!(f = fopen(source, "rb")))
		return false;
	
	while ((size = fread(buffer, 1, sizeof(buffer), f))) {
		if (Memfile_Write(this, buffer, size) != size)
			errr("Memfile: Could not successfully write from fread [%s]", source);
		amount += size;
	}
	
	fclose(f);
	
	return amount;
}

int Memfile_Insert(Memfile* this, const void* src, size_t size) {
	size_t remasize = this->size - this->seekPoint;
	
	if (this->size + size + 1 >= this->memSize)
		Memfile_Realloc(this, this->memSize * 2 + size * 2 + this->seekPoint);
	
	memmove(&this->cast.u8[this->seekPoint + size], &this->cast.u8[this->seekPoint], remasize + 1);
	memcpy(&this->cast.u8[this->seekPoint], src, size);
	this->size += size;
	
	return 0;
}

int Memfile_Append(Memfile* this, Memfile* src) {
	return Memfile_Write(this, src->data, src->size);
}

void Memfile_Align(Memfile* this, size_t align) {
	if ((this->seekPoint % align) != 0) {
		const u64 wow[32] = {};
		size_t size = align - (this->seekPoint % align);
		
		Memfile_Write(this, wow, size);
	}
}

int Memfile_Fmt(Memfile* this, const char* fmt, ...) {
	char buffer[8192];
	va_list args;
	size_t size;
	
	va_start(args, fmt);
	size = xl_vsnprintf(
		buffer,
		8192,
		fmt,
		args
	);
	va_end(args);
	
	size = Memfile_Write(this, buffer, size + 1);
	this->seekPoint--;
	this->size--;
	this->cast.u8[this->seekPoint] = '\0';
	
	return size;
}

int Memfile_Cat(Memfile* this, const char* str) {
	size_t size;
	
	osLog("write");
	size = Memfile_Write(this, str, strlen(str) + 1);
	this->seekPoint--;
	this->size--;
	this->cast.u8[this->seekPoint] = '\0';
	
	return size;
}

int Memfile_Read(Memfile* this, void* dest, size_t size) {
	size_t nsize = clamp_max(size, clamp_min(this->size - this->seekPoint, 0));
	
	if (nsize != size)
		osLog("%d == src->seekPoint = %d / %d", nsize, this->seekPoint, this->seekPoint);
	
	if (nsize < 1)
		return 0;
	
	memcpy(dest, &this->cast.u8[this->seekPoint], nsize);
	this->seekPoint += nsize;
	
	return nsize;
}

void* Memfile_Seek(Memfile* this, size_t seek) {
	if (seek == MEMFILE_SEEK_END)
		seek = this->size;
	
	if (seek > this->size)
		return NULL;
	
	this->seekPoint = seek;
	
	return (void*)&this->cast.u8[seek];
}

void Memfile_LoadMem(Memfile* this, const void* data, size_t size) {
	_validate_memfile(this);
	Memfile_Null(this);
	this->size = this->memSize = size;
	this->data = (void*)data;
}

int Memfile_LoadBin(Memfile* this, const char* filepath) {
	FILE* file = _open_file(filepath, "rb");
	size_t tempSize;
	
	if (file == NULL) {
		osLog("Could not fopen file [%s]", filepath);
		
		if (this->param.throwError) {
			if (sys_stat(this->info.name))
				_throw_error(this, "Could not load file!", x_fmt("Arg: [%s]", filepath));
			_throw_error(this, "Can't load because file does not exist!", x_fmt("Arg: [%s]", filepath));
		}
		
		return 1;
	}
	
	fseek(file, 0, SEEK_END);
	tempSize = ftell(file);
	
	_validate_memfile(this);
	Memfile_Null(this);
	
	if (this->data == NULL)
		Memfile_Alloc(this, tempSize + 0x10);
	
	else if (this->memSize < tempSize)
		Memfile_Realloc(this, tempSize * 2);
	
	this->size = tempSize;
	
	rewind(file);
	if (fread(this->data, 1, this->size, file)) {
	}
	fclose(file);
	
	this->info.age = sys_stat(filepath);
	delete(this->info.name);
	this->info.name = strdup(filepath);
	
	return 0;
}

int Memfile_LoadStr(Memfile* this, const char* filepath) {
	FILE* file = _open_file(filepath, "r");
	size_t tempSize;
	
	if (file == NULL) {
		osLog("Could not fopen file [%s]", filepath);
		
		if (this->param.throwError) {
			if (sys_stat(this->info.name))
				_throw_error(this, "Could not load file!", x_fmt("Arg: [%s]", filepath));
			_throw_error(this, "Can't load because file does not exist!", x_fmt("Arg: [%s]", filepath));
		}
		
		return 1;
	}
	
	fseek(file, 0, SEEK_END);
	tempSize = ftell(file);
	
	_validate_memfile(this);
	Memfile_Null(this);
	
	if (this->data == NULL)
		Memfile_Alloc(this, tempSize + 0x10);
	
	else if (this->memSize < tempSize)
		Memfile_Realloc(this, tempSize * 2);
	
	this->size = tempSize;
	
	rewind(file);
	this->size = fread(this->data, 1, this->size, file);
	fclose(file);
	this->cast.u8[this->size] = '\0';
	
	this->info.age = sys_stat(filepath);
	delete(this->info.name);
	this->info.name = strdup(filepath);
	
	return 0;
}

int Memfile_SaveBin(Memfile* this, const char* filepath) {
	FILE* file = _open_file(filepath, "wb");
	
	if (file == NULL) {
		osLog("Could not fopen file [%s]", filepath);
		
		if (this->param.throwError) {
			if (sys_stat(this->info.name))
				_throw_error(this, "Can't save over file!", x_fmt("Arg: [%s]", filepath));
			_throw_error(this, "Can't save file!", x_fmt("Arg: [%s]", filepath));
		}
		
		return 1;
	}
	
	if (this->size)
		fwrite(this->data, sizeof(char), this->size, file);
	fclose(file);
	
	return 0;
}

int Memfile_SaveStr(Memfile* this, const char* filepath) {
	FILE* file = _open_file(filepath, "w");
	
	if (file == NULL) {
		osLog("Could not fopen file [%s]", filepath);
		
		if (this->param.throwError) {
			if (sys_stat(this->info.name))
				_throw_error(this, "Can't save over file!", x_fmt("Arg: [%s]", filepath));
			_throw_error(this, "Can't save file!", x_fmt("Arg: [%s]", filepath));
		}
		
		return 1;
	}
	
	if (this->size)
		fwrite(this->data, sizeof(char), this->size, file);
	fclose(file);
	
	return 0;
}

static void Memfile_CleanLink(Memfile* this);

void Memfile_Free(Memfile* this) {
	if (this->param.initKey == 0xD0E0A0D0B0E0E0F0) {
		delete(this->data, this->info.name);
		
		Memfile_CleanLink(this);
	}
	
	*this = Memfile_New();
}

void Memfile_Null(Memfile* this) {
	this->size = 0;
	this->seekPoint = 0;
	if (this->data)
		this->str[0] = '\0';
}

void Memfile_Clear(Memfile* this) {
	memset(this->data, 0, this->memSize);
	Memfile_Null(this);
}

#ifdef _WIN32
#include <windows.h>
#include <wininet.h>

#else /* UNIX */

#include <curl/curl.h>

typedef struct {
	Memfile*    this;
	curl_off_t  size;
	const char* message;
} DownloadStatus;

static size_t __unix_Download_Write(char* src, size_t size, size_t n, DownloadStatus* status) {
	size_t r;
	
	r = Memfile_Write(status->this, src, size * n);
	
	return r;
}

static int __unix_Download_Progress(DownloadStatus* status, curl_off_t max, curl_off_t now, curl_off_t _a, curl_off_t _b ) {
	info_progf(status->message, BinToMb(now), BinToMb(max));
	
	return 0;
}

static const char* __unix_Download_GetDirectUrl(const char* url) {
	CURL* handle;
	char* rediret = NULL;
	
	if (!(handle = curl_easy_init()))
		return NULL;
	
	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_NOBODY, true);
	curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, true);
	if (curl_easy_perform(handle) == CURLE_OK) {
		curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &rediret);
		rediret = x_strdup(rediret);
	}
	curl_easy_cleanup(handle);
	
	return rediret;
}

static bool __unix_DownloadImpl(Memfile* this, const char* url, DownloadStatus* status) {
	CURL* handle;
	
	if (!(handle = curl_easy_init()))
		return EXIT_FAILURE;
	
	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, status);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, __unix_Download_Write);
	
	if (status->message) {
		curl_easy_setopt(handle, CURLOPT_NOPROGRESS, false);
		curl_easy_setopt(handle, CURLOPT_XFERINFODATA, status);
		curl_easy_setopt(handle, CURLOPT_XFERINFOFUNCTION, __unix_Download_Progress);
	}
	
	curl_easy_perform(handle);
	curl_easy_cleanup(handle);
	
	return this->size;
}

#endif

extern void info_prog_end(void);

bool Memfile_Download(Memfile* this, const char* url, const char* message) {
	bool result = EXIT_FAILURE;
	
	_validate_memfile(this);
	Memfile_Null(this);
	
	delete(this->info.name);
	this->info.name = strdup(url);
	
#ifdef _WIN32
	char buf[512] = { "nothing" };
	DWORD read_bytes = 0;
	size_t total_size = 0;
	
	HINTERNET session = InternetOpen("C", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (!session) {
		osLog("session fail:  %s", url);
		return EXIT_FAILURE;
	}
	
	HINTERNET open_url = InternetOpenUrl(session, url, NULL, 0, INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI | INTERNET_FLAG_PRAGMA_NOCACHE, 0);
	if (!open_url) {
		osLog("open_url fail: %s", url);
		goto close_session;
	}
	
	char size_str_buf[128] = {};
	DWORD len_buf_que = sizeof(size_str_buf);
	DWORD index = 0;
	
	if (!HttpQueryInfo(open_url, HTTP_QUERY_CONTENT_LENGTH, size_str_buf, &len_buf_que, &index)) {
		osLog("query fail:    %s", url);
		goto close_url;
	}
	
	total_size = sint(size_str_buf);
	
	if (message)
		info_progf(message, 0, BinToMb(total_size));
	while (InternetReadFile(open_url, buf, 512, &read_bytes) && read_bytes > 0) {
		Memfile_Write(this, buf, read_bytes);
		
		if (message)
			info_progf(message, BinToMb(this->size), BinToMb(total_size));
	}
	info_prog_end();
	
	result = EXIT_SUCCESS;
	
	close_url:
	InternetCloseHandle(open_url);
	close_session:
	InternetCloseHandle(session);
	
#else /* UNIX */
	DownloadStatus status = { .this = this, .message = message };
	
	if (!__unix_DownloadImpl(this, url, &status)) {
		url = __unix_Download_GetDirectUrl(url);
		
		if (!__unix_DownloadImpl(this, url, &status))
			return EXIT_FAILURE;
		info_prog_end();
	}
	
	result = EXIT_SUCCESS;
#endif
	
	return result;
}

void memdump(const void* data, size_t size, const char* file) {
	Memfile mem = Memfile_New();
	
	Memfile_LoadMem(&mem, data, size);
	Memfile_SaveBin(&mem, file);
}

void* loadbin(const char* filename) {
	return _read_file(_open_file(filename, "rb"));
}

typedef struct Sym {
	struct Sym*  next;
	enum SymSize symSize;
	const char*  name;
	const char*  type;
	Memfile*     parent;
	Memfile*     this;
	u32 offset;
	struct {
		bool processed : 1;
	} state;
} Sym;

int Memfile_WriteRefByName(Memfile* this, const char* name, enum SymSize type) {
	Sym* sym = new(Sym);
	u64 placeholder = 0;
	
	sym->name = strdup(name);
	sym->symSize = type;
	sym->offset = this->seekPoint;
	sym->parent = this;
	
	Node_Add(this->sym.refHead, sym);
	
	switch (type) {
		case SYM_16: return Memfile_Write(this, &placeholder, 2);
		case SYM_32: return Memfile_Write(this, &placeholder, 4);
		case SYM_64: return Memfile_Write(this, &placeholder, 8);
	}
	
	errr("%s: This was not supposed to happend!\nsym type: %d\nsym name: %s", __FUNCTION__, type, name);
	
	return 0;
}

int Memfile_WriteRef(Memfile* this, Memfile* ref, enum SymSize type) {
	osLog("WriteRef from %s to %s", this->sym.name, ref->sym.name);
	osAssert(ref->sym.name != NULL);
	
	Sym* sym = new(Sym);
	u64 placeholder = 0;
	
	sym->name = strdup(ref->sym.name);
	sym->symSize = type;
	sym->offset = this->seekPoint;
	sym->parent = this;
	
	Node_Add(this->sym.refHead, sym);
	
	switch (type) {
		case SYM_16: return Memfile_Write(this, &placeholder, 2);
		case SYM_32: return Memfile_Write(this, &placeholder, 4);
		case SYM_64: return Memfile_Write(this, &placeholder, 8);
	}
	
	errr("%s: This was not supposed to happend!\nsym type: %d", __FUNCTION__, type);
	
	return 0;
}

Memfile* Memfile_NewSym(Memfile* this, const char* name, const char* type, u8 align) {
	this->sym.child = realloc(this->sym.child, sizeof(void**[++this->sym.numChild]));
	
	Memfile* mem = this->sym.child[this->sym.numChild - 1] = new(Memfile);
	
	*mem = Memfile_New();
	mem->sym.name = strdup(name);
	mem->sym.type = type ? strdup(type) : NULL;
	mem->sym.align = align;
	
	return mem;
}

void Memfile_Link(Memfile* this, enum SymOFlag opt, void callback(void*, u64*), void* udata) {
	nested(void, GatherSyms, (Memfile * mem)) {
		osLog("%s numchild %d", mem->sym.name, mem->sym.numChild);
		
		for (int i = 0; i < mem->sym.numChild; i++) {
			Memfile* child = mem->sym.child[i];
			
			osAssert(child->sym.name != NULL);
			
			if (child->size) {
				Sym* sym = new(Sym);
				u8* f;
				
				sym->name = child->sym.name;
				sym->type = child->sym.type;
				sym->this = child;
				Node_Add(this->sym.head, sym);
				
				int searchAlign = 4;
				
				if (child->sym.align)
					searchAlign = child->sym.align;
				
				if (opt & SYM_O_MERGE_IDENTICAL &&  (f = memmem_align(searchAlign, this->data, this->size, child->data, child->size))) {
					sym->offset = f - this->cast.u8;
				} else {
					if (child->sym.align)
						Memfile_Align(this, child->sym.align);
					
					sym->offset = this->seekPoint;
					Memfile_Append(this, child);
				}
			} else
				osLog("" PRNT_REDD "skipped" PRNT_RSET ": %s", child->sym.name);
			
			GatherSyms(child);
		}
	};
	
	osLog("gather");
	GatherSyms(this);
	
	nested(Sym*, __FindRefImpl, (Memfile * mem, const char* name)) {
		if (mem->sym.refHead)
			for (Sym* sym = mem->sym.refHead; sym; sym = sym->next)
				if (!sym->state.processed && streq(name, sym->name))
					return sym;
		
		for (int i = 0; i < mem->sym.numChild; i++) {
			Memfile* child = mem->sym.child[i];
			Sym* sym = __FindRefImpl(child, name);
			
			if (sym) return sym;
		}
		
		return NULL;
	};
	
	nested(Sym*, FindRef, (const char* name)) {
		return __FindRefImpl(this, name);
	};
	
	nested(Sym*, FindSym, (const char* name)) {
		for (Sym* sym = this->sym.head; sym; sym = sym->next)
			if (streq(sym->name, name))
				return sym;
		
		return NULL;
	};
	
	for (Sym* sym = this->sym.head; sym; sym = sym->next) {
		Sym* ref;
		
		while ((ref = FindRef(sym->name))) {
			osAssert(ref->parent != NULL);
			Sym* parentSym = FindSym(ref->parent->sym.name);
			
			if (!parentSym)
				errr("Undefined Reference: %s", ref->parent->sym.name);
			
			u64 offset = sym->offset;
			size_t write_pos = parentSym->offset + ref->offset;
			
			if (callback)
				callback(udata, &offset);
			
			if (ref->symSize == SYM_16) {
				u16* ptr = (void*)&offset;
				
				*ptr = offset;
			} else if (ref->symSize == SYM_32) {
				u32* ptr = (void*)&offset;
				
				*ptr = offset;
			} else if (ref->symSize == SYM_64) {
				u64* ptr = (void*)&offset;
				
				*ptr = offset;
			}
			
			u8 size[] = {
				[SYM_16] = 2,
				[SYM_32] = 4,
				[SYM_64] = 8
			};
			
			memcpy(this->cast.u8 + write_pos, &offset, size[ref->symSize]);
			
			ref->state.processed = true;
		}
	}
}

static void Memfile_CleanLink(Memfile* this) {
	
	for (int i = 0; i < this->sym.numChild; i++) {
		Memfile_Free(this->sym.child[i]);
		delete(this->sym.child[i]);
	}
	
	while (this->sym.refHead) {
		delete(this->sym.refHead->name);
		Node_Kill(this->sym.refHead, this->sym.refHead);
	}
	
	while (this->sym.head)
		Node_Kill(this->sym.head, this->sym.head);
	
	delete(this->sym.child, this->sym.name, this->sym.type);
}

int64_t Memfile_GetSymOffset(Memfile* this, const char* name) {
	for (Sym* sym = this->sym.head; sym; sym = sym->next)
		if (streq(sym->name, name))
			return sym->offset;
	
	return -1;
}

const char* Memfile_PrintSymLinker(Memfile* this, void callback(void*, u64*), void* udata) {
	char* msg = "";
	
#if DEBUG_LOG
	u32 offset = 0;
#endif
	
	for (Sym* sym = this->sym.head; sym; sym = sym->next) {
		u64 val = sym->offset;
		
		if (callback) callback(udata, &val);
		
#if DEBUG_LOG
		const char* color = "";
		if (offset > val)
			color = PRNT_BLUE;
#endif
		
		msg = x_fmt("%s%-32s = "
#if DEBUG_LOG
				"%s"
#endif
				"0x%08X" PRNT_RSET ";\n", msg, sym->name,
#if DEBUG_LOG
				color,
#endif
				val);
#if DEBUG_LOG
		offset = val;
#endif
	}
	
	return strdup(msg);
}

const char* Memfile_PrintSymHeader(Memfile* this, int callback(void*, const char*, size_t), void* udata) {
	char* msg = "";
	
	for (Sym* sym = this->sym.head; sym; sym = sym->next) {
		if (sym->type) {
			const char* count = "";
			
			if (callback) {
				int c = callback(udata, sym->type, sym->this->size);
				
				if (c > 0)
					count = x_fmt("%d", callback(udata, sym->type, sym->this->size));
			}
			
			msg = x_fmt("%s"PRNT_REDD "extern " PRNT_BLUE "%-20s" PRNT_RSET " %s[%s];\n", msg, sym->type, sym->name, count);
		}
	}
	
	return strdup(msg);
}
