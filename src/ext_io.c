#include <ext_lib.h>
#include <sys/time.h>
#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_DECORATE(name) xl_ ## name
#include "xio/stb_sprintf.h"

static void osLogSignal(int arg);

static vbool sAbort;
static enum IOLevel sSuppress = 0;
static mutex_t sIoMutex;
u8 gInfoProgState;

int xl_fprintf(FILE* stream, const char* fmt, ...) {
	va_list va;
	
	va_start(va, fmt);
	char buffer[1024 * 8];
	int r = xl_vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);
	
	fputs(buffer, stream);
	
	return r;
}

///////////////////////////////////////////////////////////////////////////////

void IO_SetLevel(enum IOLevel lvl) {
	sSuppress = lvl;
}

void IO_lock() {
	mutex_lock(&sIoMutex);
}

void IO_unlock() {
	mutex_unlock(&sIoMutex);
}

void IO_graph(get_val_callback_t get, void* udata, int num, f64 max, f32 pow) {
	int cli_size[2];
	
	cli_getSize(cli_size);
	cli_clear();
	fflush(stdout);
	sys_sleep(0.25f);
	
	for (var_t x = 0; x < cli_size[0]; x++) {
		f32 x_mod = ((f32)x / cli_size[0]);
		int id_mod = num * x_mod;
		f64 val = get(udata, id_mod);
		int y_mod = clamp(invertf(powf(invertf(clamp_max((val / max), 1.0f)), pow)), 0.0f, 0.995f) * cli_size[1];
		
		cli_setPos(x, (cli_size[1] - 1) - y_mod);
		fflush(stdout);
		puts(".");
		
		for (; y_mod; y_mod--) {
			cli_setPos(x, (cli_size[1] - 1) - (y_mod - 1));
			fflush(stdout);
			puts("|");
		}
		fflush(stdout);
	}
}

void IO_KillBuf(FILE* output) {
#ifdef _WIN32
	if (freopen ("NUL", "w", output))
		(void)0;
#else
	if (freopen ("/dev/null", "w", output))
		(void)0;
#endif
}

void IO_FixWin32(void) {
#ifdef _WIN32
	system("");
#endif
}

///////////////////////////////////////////////////////////////////////////////

void info_title(const char* toolname, const char* fmt, ...) {
	static u32 printed = 0;
	
	if (sSuppress >= PSL_NO_INFO)
		return;
	
	if (printed != 0) return;
	printed++;
	
	u32 strln = strlen(toolname);
	u32 rmv = 0;
	u32 tmp = strln;
	va_list args;
	
	for (int i = 0; i < strln; i++) {
		if (rmv) {
			if (toolname[i] != 'm') {
				tmp--;
			} else {
				tmp -= 2;
				rmv = false;
			}
		} else {
			if (toolname[i] == '\e' && toolname[i + 1] == '[') {
				rmv = true;
				strln--;
			}
		}
	}
	
	strln = tmp;
	
	printf(PRNT_GRAY "[>]--");
	for (int i = 0; i < strln; i++)
		printf("-");
	printf("------[>]\n");
	
	printf(" |   ");
	printf(PRNT_CYAN "%s" PRNT_GRAY, toolname);
	printf("       |\n");
	
	printf("[>]--");
	for (int i = 0; i < strln; i++)
		printf("-");
	printf("------[>]\n" PRNT_RSET);
	printf("     ");
	
	if (fmt) {
		va_start(args, fmt);
		vprintf(
			fmt,
			args
		);
		va_end(args);
		if (strlen(fmt) > 1)
			printf("\n");
	}
	printf("\n" PRNT_RSET);
	
}

///////////////////////////////////////////////////////////////////////////////

static void IO_printImpl(int color_id, int is_progress, int is_error, FILE* stream, const char* msg, const char* fmt, va_list va) {
	const char* color[] = {
		"" PRNT_GRAY "  " PRNT_RSET,
		"" PRNT_RSET ">" PRNT_GRAY " " PRNT_RSET,
		"" PRNT_REDD ">" PRNT_GRAY " " PRNT_RSET,
		"" PRNT_GREN ">" PRNT_GRAY " " PRNT_RSET,
		"" PRNT_YELW ">" PRNT_GRAY " " PRNT_RSET,
		"" PRNT_BLUE ">" PRNT_GRAY " " PRNT_RSET,
		"" PRNT_PRPL ">" PRNT_GRAY " " PRNT_RSET,
		"" PRNT_CYAN ">" PRNT_GRAY " " PRNT_RSET,
	};
	
	if (sAbort && !is_error)
		return;
	
	IO_lock(); {
		if (gInfoProgState && !is_progress) {
			xl_fprintf(stream, "\n");
			gInfoProgState = false;
		}
		
		const size_t bufsize = MbToBin(0.25f);
		char buffer[bufsize];
		
		xl_vsnprintf(buffer, bufsize, fmt, va);
		
		if (is_progress)
			xl_fprintf(stream, "\r");
		
		fputs(color[color_id], stream);
		if (msg) {
			int l = 16 - strvlen(msg);
			fputs(msg, stream);
			
			if (l > 0) {
				char p[l + 1];
				
				memset(p, ' ', l);
				p[l] = 0;
				
				fputs(p, stream);
			}
		}
		strnrep(buffer, bufsize, "\n", "\n  ");
		fputs(buffer, stream);
		if (!is_progress)
			fputs("\n" PRNT_RSET, stream);
		
		fflush(stream);
		fflush(stream);
		fflush(stream);
		
	} IO_unlock();
}

static void IO_printCall(int color_id, int is_progress, FILE* stream, const char* msg, const char* fmt, ...) {
	va_list va;
	
	va_start(va, fmt);
	IO_printImpl(color_id, is_progress, false, stream, msg, fmt, va);
	va_end(va);
}

///////////////////////////////////////////////////////////////////////////////

void warn(const char* fmt, ...) {
	if (sSuppress >= PSL_NO_WARNING)
		return;
	
	va_list args;
	va_start(args, fmt);
	IO_printImpl(4, false, false, stderr, NULL, fmt, args);
	va_end(args);
}

void warn_align(const char* info, const char* fmt, ...) {
	if (sSuppress >= PSL_NO_WARNING)
		return;
	
	va_list args;
	
	va_start(args, fmt);
	IO_printImpl(4, false, false, stderr, info, fmt, args);
	va_end(args);
}

void errr(const char* fmt, ...) {
	va_list args;
	
	IO_KillBuf(stdout);
	sAbort = 1;
	
	va_start(args, fmt);
	IO_printImpl(2, false, true, stderr, NULL, fmt, args);
	va_end(args);
	
#ifdef _WIN32
	cli_getc();
#endif
	
	exit(EXIT_FAILURE);
}

void errr_align(const char* info, const char* fmt, ...) {
	va_list args;
	
	IO_KillBuf(stdout);
	sAbort = 1;
	
	va_start(args, fmt);
	IO_printImpl(2, false, true, stderr, info, fmt, args);
	va_end(args);
	
#ifdef _WIN32
	cli_getc();
#endif
	
	exit(EXIT_FAILURE);
}

void info(const char* fmt, ...) {
	if (sSuppress >= PSL_NO_INFO)
		return;
	
	va_list args;
	
	va_start(args, fmt);
	IO_printImpl(1, false, false, stdout, NULL, fmt, args);
	va_end(args);
}

void info_align(const char* info, const char* fmt, ...) {
	if (sSuppress >= PSL_NO_INFO)
		return;
	
	va_list args;
	
	va_start(args, fmt);
	IO_printImpl(1, false, false, stdout, info, fmt, args);
	va_end(args);
}

///////////////////////////////////////////////////////////////////////////////

void info_prog_end(void) {
	if (gInfoProgState) {
		gInfoProgState = false;
		xl_fprintf(stdout, "\n");
	}
}

void info_fastprog(const char* info, int a, int b) {
	static int prev;
	
	if (a + 1 == prev)
		return;
	
	if (sSuppress >= PSL_NO_INFO)
		return;
	
	IO_printCall(1, true, stdout, info,
		x_fmt("[ %%%dd / %%-%dd ]", digint(b), digint(b)), a, b);
	gInfoProgState = true;
	prev = a + 1;
	
	if (a == b && b != 0)
		info_prog_end();
}

void info_fastprogf(const char* info, f64 a, f64 b) {
	static int prev;
	
	if (a + 1 == prev)
		return;
	
	if (sSuppress >= PSL_NO_INFO)
		return;
	
	IO_printCall(1, true, stdout, info,
		x_fmt("[ %%%d.2f / %%-%d.2f ]", digint(b), digint(b)), a, b);
	gInfoProgState = true;
	prev = a + 1;
	
	if (a == b && b != 0)
		info_prog_end();
}

void info_prog(const char* info, int a, int b) {
	if (sSuppress >= PSL_NO_INFO)
		return;
	
	if (a != b && gInfoProgState) {
		static struct timeval p;
		
		if (a < 1)
			gettimeofday(&p, 0);
		else {
			struct timeval t;
			
			gettimeofday(&t, 0);
			f32 sec = t.tv_sec - p.tv_sec + (f32)(t.tv_usec - p.tv_usec) * 0.000001f;
			
			if (sec >= 0.12f) {
				p = t;
			} else return;
		}
	}
	
	info_fastprog(info, a, b);
}

void info_progf(const char* info, f64 a, f64 b) {
	if (sSuppress >= PSL_NO_INFO)
		return;
	
	if (a != b && gInfoProgState) {
		static struct timeval p;
		
		if (a < 1)
			gettimeofday(&p, 0);
		else {
			struct timeval t;
			
			gettimeofday(&t, 0);
			f32 sec = t.tv_sec - p.tv_sec + (f32)(t.tv_usec - p.tv_usec) * 0.000001f;
			
			if (sec >= 0.12f) {
				p = t;
			} else return;
		}
	}
	
	info_fastprogf(info, a, b);
}

///////////////////////////////////////////////////////////////////////////////

void info_getc(const char* txt) {
	info("%s", txt);
	cli_getc();
}

void info_volatile(const char* fmt, ...) {
	va_list va;
	
	if (sAbort)
		return;
	
	va_start(va, fmt);
	thd_lock();
	IO_FixWin32();
	vprintf(fmt, va);
	fflush(NULL);
	thd_unlock();
	va_end(va);
}

void info_hex(const char* txt, const void* data, u32 size, u32 dispOffset) {
	const u8* d = data;
	u32 num = 8;
	char* digit;
	int i = 0;
	
	if (sAbort)
		return;
	
	if (sSuppress >= PSL_NO_INFO)
		return;
	
	for (;; num--)
		if ((size + dispOffset) >> (num * 4))
			break;
	
	digit = x_fmt("" PRNT_GRAY "%c0%dX: " PRNT_RSET, '%', num + 1);
	
	if (txt)
		info("%s", txt);
	for (; i < size; i++) {
		if (i % 16 == 0)
			printf(digit, i + dispOffset);
		
		printf("%02X", d[i]);
		if ((i + 1) % 4 == 0)
			printf(" ");
		if ((i + 1) % 16 == 0)
			printf("\n");
	}
	
	if (i % 16 != 0)
		printf("\n");
}

void info_bit(const char* txt, const void* data, u32 size, u32 dispOffset) {
	const u8* d = data;
	int s = 0;
	u32 num = 8;
	char* digit;
	
	if (sAbort)
		return;
	
	if (sSuppress >= PSL_NO_INFO)
		return;
	
	for (;; num--)
		if ((size + dispOffset) >> (num * 4))
			break;
	
	digit = x_fmt("" PRNT_GRAY "%c0%dX: " PRNT_RSET, '%', num + 1);
	
	if (txt)
		info("%s", txt);
	for (int i = 0; i < size; i++) {
		if (s % 4 == 0)
			printf(digit, s + dispOffset);
		
		for (int j = 7; j >= 0; j--)
			printf("%d", (d[i] >> j) & 1);
		
		printf(" ");
		
		if ((s + 1) % 4 == 0)
			printf("\n");
		
		s++;
	}
	
	if (s % 4 != 0)
		printf("\n");
}

void info_nl(void) {
	if (sAbort)
		return;
	
	if (sSuppress >= PSL_NO_INFO)
		return;
	
	xl_fprintf(stdout, "\n");
}

const char* addr_name(void* addr) {
	const char* color[] = {
		PRNT_REDD, PRNT_YELW,        PRNT_GREN,
		PRNT_CYAN, PRNT_BLUE,        PRNT_PRPL,
		"",        PRNT_GRAY,
	};
	const char* syltable[] = {
		"la",  "va",  "ra",  "sa",
		"lu",  "vu",  "ru",  "su",
		"se",  "me",  "te",  "ke",
		"a",   "i",   "o",   "u",
		"mo",  "ko",  "to",  "no",
		"si",  "ti",  "ki",  "vi",
		"nie", "vie", "mie", "kie",
		"nai", "vai", "mai", "kai",
	};
	
	//crustify
    int shuffleTable[][16] = {
        { 14, 12, 15, 27, 4,  23, 18, 8,  16, 9,  17, 29, 31, 1,  20, 21 },
        { 12, 16, 9,  7,  24, 21, 11, 29, 6,  23, 27, 8,  3,  14, 25, 31 },
        { 31, 20, 7,  2,  3,  13, 0,  22, 11, 30, 10, 25, 18, 8,  21, 28 },
        { 1,  3,  10, 9,  25, 2,  11, 14, 8,  26, 15, 30, 29, 31, 28, 20 },
        { 15, 24, 2,  26, 11, 21, 30, 31, 4,  6,  13, 17, 18, 10, 12, 28 },
        { 10, 25, 12, 0,  2,  30, 4,  17, 9,  26, 24, 11, 5,  19, 22, 14 },
        { 3,  5,  2,  11, 28, 24, 18, 6,  13, 21, 12, 26, 17, 14, 19, 15 },
        { 4,  30, 1,  2,  29, 31, 6,  3,  12, 14, 22, 16, 7,  5,  21, 20 },
    };
	//uncrustify
	
	union {
		struct {
			u32 _pad       : 3;
			u32 fn_shuffle : 3;
			u32 ln_shuffle : 3;
			u32 fn_syl0    : 4;
			u32 ln_syl0    : 4;
			u32 fn_syl1    : 4;
			u32 ln_syl1    : 4;
			u32 fn_syl2    : 4;
			u32 ln_syl2    : 3;
		};
		u32 val;
	} tbl = { .val = (u32)addr };
	
	char* firstName = x_fmt(
		"%s%s%s",
		syltable[shuffleTable[tbl.fn_shuffle][tbl.fn_syl0]],
		syltable[shuffleTable[tbl.fn_shuffle][tbl.fn_syl1]],
		syltable[shuffleTable[tbl.fn_shuffle][tbl.fn_syl2]]);
	
	char* lastName = x_fmt(
		"%s%s%s",
		syltable[shuffleTable[tbl.ln_shuffle][tbl.ln_syl0]],
		syltable[shuffleTable[tbl.ln_shuffle][tbl.ln_syl1]],
		syltable[shuffleTable[tbl.ln_shuffle][tbl.ln_syl2]]);
	
	firstName[0] = toupper(firstName[0]);
	lastName[0] = toupper(lastName[0]);
	
	return x_fmt("%s%s %s: %08X", color[(tbl.val >> 3) % ArrCount(color)], firstName, lastName, addr);
}

///////////////////////////////////////////////////////////////////////////////

#include <signal.h>

#define FAULT_BUFFER_SIZE (1024)
#define FAULT_LOG_NUM     16

static char* sLogMsg[FAULT_LOG_NUM];
static char* sLogFunc[FAULT_LOG_NUM];
static u32 sLogLine[FAULT_LOG_NUM];
static vs32 sLogInit;
static mutex_t sLogMutex;

static void osLogPrintTitle(int arg, FILE* file) {
	const char* errorMsg[] = {
		"\a0",
		"\a1 - Hang Up",
		"\a2 - Interrupted",   // SIGINT
		"\a3 - Quit",
		"\a4 - Illegal Instruction",
		"\a5 - Trap",
		"\a6 - Abort()",
		"\a7 - Illegal Memory Access",
		"\a8 - Floating Point Exception",
		"\a9 - Killed",
		"\a10 - Programmer Error",
		"\a11 - Segmentation Fault",
		"\a12 - Programmer Error",
		"\a13 - Pipe Death",
		"\a14 - Alarm",
		"\a15 - Killed",
		
		"\aLog List",
	};
	
	if (gInfoProgState)
		fprintf(file, "\n");
	
	if (arg != 0xDEADBEEF)
		fprintf(file, "" PRNT_GRAY "[ " PRNT_REDD "%s " PRNT_GRAY "]\n", errorMsg[clamp_max(arg, 16)]);
	else
		fprintf(file, "" PRNT_GRAY "[ " PRNT_REDD "osLog " PRNT_GRAY "]\n");
	fprintf(file, "\n");
}

static void osLogPrintLog(int arg, FILE* file) {
	
	for (int i = FAULT_LOG_NUM - 1, j = 0; i >= 0; i--, j++) {
		char* pfunc = j == 0 ? "__log_none__" : sLogFunc[i + 1];
		char fmt[16];
		
		snprintf(fmt, 16, "%d:", sLogLine[i]);
		if (strcmp(sLogFunc[i], pfunc))
			fprintf(file, "" PRNT_YELW "%s" PRNT_GRAY "();\n", sLogFunc[i]);
		fprintf(file, "" PRNT_GRAY "%-8s" PRNT_RSET "%s\n", fmt, sLogMsg[i]);
	}
	
	if (arg == 16)
		fprintf(file, "\n");
}

static void osLogSignal(int arg) {
	if (!sLogInit)
		return;
	
	pthread_mutex_lock(&sLogMutex);
	
	sLogInit = false;
	sAbort = true;
	
	osLogPrintTitle(arg, stderr);
	osLogPrintLog(arg, stderr);
	
	if (arg == 0xDEADBEEF) {
		sLogInit = true;
		sAbort = false;
		
		pthread_mutex_unlock(&sLogMutex);
		
		return;
	}
	
#ifdef _WIN32
	info_getc("Press enter to exit");
#endif
	exit(arg);
}

void osLogInit() {
	for (int i = 1; i < 16; i++) {
		if (i != 2) // ignore interrupt;
			signal(i, osLogSignal);
	}
	for (int i = 0; i < FAULT_LOG_NUM; i++) {
		sLogMsg[i] = calloc(FAULT_BUFFER_SIZE);
		sLogFunc[i] = calloc(128);
	}
	
	sLogInit = true;
	
	pthread_mutex_init(&sLogMutex, 0);
	pthread_mutex_init(&sIoMutex, 0);
}

void osLogDestroy() {
	for (int i = 0; i < FAULT_LOG_NUM; i++)
		delete(sLogMsg[i], sLogFunc[i]);
	pthread_mutex_destroy(&sLogMutex);
	pthread_mutex_destroy(&sIoMutex);
}

void osLogPrint() {
	if (!sLogInit)
		return;
	if (sLogMsg[0] == NULL)
		return;
	if (sLogMsg[0][0] != 0)
		osLogSignal(0xDEADBEEF);
}

void __osLog__(const char* func, u32 line, const char* txt, ...) {
	if (!sLogInit)
		return;
	
	if (sLogMsg[0] == NULL)
		return;
	
	pthread_mutex_lock(&sLogMutex);
	{
		memshift(sLogMsg, 1, sizeof(void*), FAULT_LOG_NUM);
		memshift(sLogFunc, 1, sizeof(void*), FAULT_LOG_NUM);
		memshift(sLogLine, 1, sizeof(u32), FAULT_LOG_NUM);
		
		va_list args;
		va_start(args, txt);
		xl_vsnprintf(sLogMsg[0], FAULT_BUFFER_SIZE, txt, args);
		va_end(args);
		
		strcpy(sLogFunc[0], func);
		sLogLine[0] = line;
		
#if 0
		if (strcmp(sLogFunc[0], sLogFunc[1]))
			fprintf(stderr, "" PRNT_REDD "%s" PRNT_GRAY "();\n", sLogFunc[0]);
		fprintf(stderr, "" PRNT_GRAY "%-8d" PRNT_RSET "%s\n", sLogLine[0], sLogMsg[0]);
#endif
	}
	pthread_mutex_unlock(&sLogMutex);
}

///////////////////////////////////////////////////////////////////////////////
