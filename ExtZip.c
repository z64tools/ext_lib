
// # # # # # # # # # # # # # # # # # # # #
// # ZIPFILE                             #
// # # # # # # # # # # # # # # # # # # # #

#include <ExtZip.h>

void* ZipFile_Load(ZipFile* zip, const char* file, ZipParam mode) {
	zip->filename = StrDup(file);
	
	return zip->pkg = zip_open(file, 0, mode);
}

s32 ZipFile_ReadEntry_Name(ZipFile* zip, const char* entry, MemFile* mem) {
	void* data = NULL;
	Size sz;
	
	if (zip_entry_open(zip->pkg, entry))
		return ZIP_ERROR_OPEN_ENTRY;
	if (zip_entry_read(zip->pkg, &data, &sz) < 0) {
		if (zip_entry_close(zip->pkg))
			printf_error("Fatal Error: Entry Read & Entry Close failed for ZipFile \"%s\"", zip->filename);
		
		return ZIP_ERROR_RW_ENTRY;
	}
	
	strncpy(mem->info.name, entry, 512);
	
	if (zip_entry_close(zip->pkg))
		return ZIP_ERROR_CLOSE;
	
	MemFile_LoadMem(mem, data, sz);
	
	return 0;
}

s32 ZipFile_ReadEntry_Index(ZipFile* zip, Size index, MemFile* mem) {
	void* data = NULL;
	Size sz;
	
	if (zip_entry_openbyindex(zip->pkg, index))
		return ZIP_ERROR_OPEN_ENTRY;
	strncpy(mem->info.name, zip_entry_name(zip->pkg), 512);
	Log("Reading Entry \"%s\"", mem->info.name);
	
	if (zip_entry_read(zip->pkg, &data, &sz) < 0) {
		if (zip_entry_close(zip->pkg))
			printf_error("Fatal Error: Entry Read & Entry Close failed for ZipFile \"%s\"", zip->filename);
		
		return ZIP_ERROR_RW_ENTRY;
	}
	
	MemFile_LoadMem(mem, data, sz);
	
	if (zip_entry_close(zip->pkg))
		return ZIP_ERROR_CLOSE;
	
	return 0;
}

s32 ZipFile_WriteEntry(ZipFile* zip, MemFile* mem, char* entry) {
	if (zip_entry_open(zip->pkg, entry))
		return ZIP_ERROR_OPEN_ENTRY;
	if (zip_entry_write(zip->pkg, mem->data, mem->size))
		return ZIP_ERROR_RW_ENTRY;
	if (zip_entry_close(zip->pkg))
		return ZIP_ERROR_CLOSE;
	
	return 0;
}

static s32 ZipFile_Extract_NoCallback(ZipFile* zip, MemFile* mem, u32 i) {
	s32 ret;
	
	Log("ReadEntryIndex");
	if ((ret = ZipFile_ReadEntry_Index(zip, i, mem)))
		return ret;
	
	Log("SaveFile \"%s\"", FileSys_File(mem->info.name));
	if (MemFile_SaveFile(mem, FileSys_File(mem->info.name)))
		return ZIP_ERROR_RW_ENTRY;
	
	MemFile_Free(mem);
	
	return 0;
}

static s32 ZipFile_Extract_Callback(ZipFile* zip, MemFile* mem, u32 i, f32 prcnt, s32 (*callback)(const char* name, f32 prcnt)) {
	char* name;
	s32 isDir;
	
	if (!callback)
		return ZipFile_Extract_NoCallback(zip, mem, i);
	
	Log("Get Name");
	if (zip_entry_openbyindex(zip->pkg, i))
		return ZIP_ERROR_OPEN_ENTRY;
	name = StrDup(zip_entry_name(zip->pkg));
	isDir = zip_entry_isdir(zip->pkg);
	if (zip_entry_close(zip->pkg))
		return ZIP_ERROR_CLOSE;
	
	if (!callback(name, prcnt)) {
		if (isDir) {
			Sys_MakeDir(FileSys_File(name));
			Free(name);
			
			return 0;
		}
		
		Free(name);
		
		return ZipFile_Extract_NoCallback(zip, mem, i);
	}
	
	Free(name);
	
	return 0;
}

s32 ZipFile_Extract(ZipFile* zip, const char* path, s32 (*callback)(const char* name, f32 prcnt)) {
	MemFile mem = MemFile_Initialize();
	u32 ent = zip_entries_total(zip->pkg);
	s32 ret;
	
	FileSys_Path(path);
	
	Log("Entries: %d", ent);
	for (u32 i = 0; i < ent; i++) {
		if ((ret = ZipFile_Extract_Callback(zip, &mem, i, ((f32)(i + 1) / ent) * 100.0f, callback)))
			return ret;
	}
	
	return 0;
}

void ZipFile_Free(ZipFile* zip) {
	Free(zip->filename);
	zip_close(zip->pkg);
}