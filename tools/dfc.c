#include <ext_lib.h>

// Data File Compiler

s32 CC(char* cmd, char* input) {
	FILE* cc = popen(cmd, "w");
	
	if (cc == NULL)
		printf_error("Failed to popen CC!");
	
	if (fwrite(input, 1, strlen(input), cc) != strlen(input))
		printf_error("Error feeding input to CC!");
	
	return pclose(cc);
}

s32 Main(s32 argc, char** argv) {
	MemFile input = MemFile_Initialize();
	MemFile c_mem = MemFile_Initialize();
	const char* name = NULL;
	const char* output = NULL;
	const char* cc = NULL;
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
		cc = argv[parArg];
	if (Arg("n"))
		name = argv[parArg];
	
	if (!name || !cc || !input.data) {
		printf_toolinfo("DataFile Compiler 1.0", NULL);
		
		fprintf(
			stdout,
			"Usage: "
			PRNT_GRAY PRNT_ITLN "// red arguments are required\n" PRNT_RSET
			PRNT_REDD "    --cc " PRNT_YELW "<compiler>\n"
			PRNT_REDD "    --i  " PRNT_YELW "<input_file>\n"
			PRNT_REDD "    --o  " PRNT_YELW "<output_file>\n"
			PRNT_CYAN "    --e  " PRNT_GRAY PRNT_ITLN "// prints extern header info\n"
			PRNT_CYAN "    --n  " PRNT_YELW "<override_variable_name>\n"
		);
		
		return 1;
	}
	
	MemFile_Alloc(&c_mem, input.size * 3);
	MemFile_Params(&c_mem, MEM_REALLOC, true, MEM_END);
	
	MemFile_Printf(&c_mem, "struct {\n");
	MemFile_Printf(&c_mem, "unsigned int size;\n");
	MemFile_Printf(&c_mem, "unsigned char data[];\n");
	MemFile_Printf(&c_mem, "} g%s={\n", name);
	MemFile_Printf(&c_mem, ".size = %d,\n", input.size);
	MemFile_Printf(&c_mem, ".data = {\n", input.size);
	
	for (u32 i = 0; i < input.size; i++) {
		if (i % 8 == 0 && i != 0)
			MemFile_Printf(&c_mem, "\n");
		
		if (!MemFile_Printf(&c_mem, "0x%02X,", input.cast.u8[i]))
			printf_error("Failed to form C file...");
	}
	MemFile_Printf(&c_mem, "}};\n", input.size);
	
	if (CC(xFmt("%s -c -o %s -x c - ", cc, output), c_mem.str))
		printf_error("Error compiling data-file [%s]", input.info.name);
	
	MemFile_Free(&c_mem);
	MemFile_Free(&input);
	
	if (Arg("e"))
		fprintf(stdout, "extern DataFile g%s[];\n", name);
	
	Log_Free();
	
	return 0;
}