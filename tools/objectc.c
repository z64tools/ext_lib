#include <ExtLib.h>

s32 GCC(char* cmd, char* input) {
	FILE* gcc = popen(cmd, "w");
	
	if (gcc == NULL)
		printf_error("Failed to popen GCC!");
	
	if (fwrite(input, 1, strlen(input), gcc) != strlen(input))
		printf_error("Error feeding input to GCC!");
	
	return pclose(gcc);
}

s32 Main(s32 argc, char** argv) {
	MemFile input = MemFile_Initialize();
	MemFile c_mem = MemFile_Initialize();
	const char* name = NULL;
	const char* output = NULL;
	const char* gcc = NULL;
	u32 parArg;
	
	Log_Init();
	
	if (Arg("i"))
		if (Sys_Stat(argv[parArg]))
			MemFile_LoadFile(&input, argv[parArg]);
	if (Arg("o")) {
		output = argv[parArg];
		name = Basename(argv[parArg]);
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
	
	MemFile_Alloc(&c_mem, input.size * 3);
	MemFile_Params(&c_mem, MEM_REALLOC, true, MEM_END);
	
	MemFile_Printf(&c_mem, "unsigned char g%s[]={\n", name);
	for (u32 i = 0; i < input.size; i++) {
		if (i % 8 == 0 && i != 0)
			MemFile_Printf(&c_mem, "\n");
		
		if (!MemFile_Printf(&c_mem, "0x%02X,", input.cast.u8[i]))
			printf_error("Failed to form C file...");
	}
	MemFile_Printf(&c_mem, "\n};\nunsigned int g%sSize=sizeof(g%s);", name, name);
	
	if (GCC(xFmt("%s -c -o %s -x c - ", gcc, output), c_mem.str))
		printf_error("Error compiling data-file [%s]", input.info.name);
	
	MemFile_Free(&c_mem);
	MemFile_Free(&input);
	Log_Free();
	
	return 0;
}