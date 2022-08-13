#include <ExtLib.h>

char* FormatName(const char* name) {
	return xFmt("%c%s", toupper(*name), name + 1);
}

s32 Main(s32 argc, char** argv) {
	MemFile input = MemFile_Initialize();
	MemFile c_mem = MemFile_Initialize();
	const char* name = NULL;
	const char* output = NULL;
	const char* c_filename = NULL;
	const char* gcc = NULL;
	u32 parArg;
	
	Log_Init();
	
	if (Arg("i"))
		if (Sys_Stat(argv[parArg]))
			MemFile_LoadFile(&input, argv[parArg]);
	if (Arg("o")) {
		output = argv[parArg];
		name = FormatName(Basename(argv[parArg]));
	}
	if (Arg("cc"))
		gcc = argv[parArg];
	if (Arg("n"))
		name = argv[parArg];
	
	if (!name || !gcc || !input.data)
		printf_error(
			"" PRNT_REDD "./objectc "
			PRNT_YELW " --cc " PRNT_GRAY "<compiler>"
			PRNT_YELW " --i " PRNT_GRAY "<input_file>"
			PRNT_YELW " --o " PRNT_GRAY "<output_file>"
			PRNT_GREN " --n " PRNT_GRAY "<override_variable_name>"
		);
	
	c_filename = xFmt("%s.tmp.c", input.info.name);
	MemFile_Alloc(&c_mem, input.size * 3);
	MemFile_Params(&c_mem, MEM_REALLOC, true, MEM_END);
	
	MemFile_Printf(&c_mem, "unsigned char g%s[] = {\n\t", name);
	for (u32 i = 0; i < input.size; i++) {
		if (i % 8 == 0 && i != 0)
			MemFile_Printf(&c_mem, "\n\t", input.cast.u8[i]);
		
		if (!MemFile_Printf(&c_mem, "0x%02X, ", input.cast.u8[i]))
			printf_error("Failed to form C file...");
	}
	MemFile_Printf(&c_mem, "\n};\n\nunsigned int g%sSize = sizeof(g%s);\n", name, name);
	if (MemFile_SaveFile_String(&c_mem, c_filename))
		printf_error("Exiting!");
	MemFile_Free(&c_mem);
	MemFile_Free(&input);
	
	if (SysExe(xFmt("%s -c %s -o %s", gcc, c_filename, output)))
		return 1;
	
	Sys_Delete(c_filename);
	Log_Free();
	
	return 0;
}