#include <ext_lib.h>
#include <sys/time.h>

static void _log_signal(int arg);

static vbool sAbort;
static u8 sProgress;
static enum IOLevel sSuppress = 0;

// # # # # # # # # # # # # # # # # # # # #
// # PRINTF                              #
// # # # # # # # # # # # # # # # # # # # #

static mutex_t sIoMutex;

void IO_SetLevel(enum IOLevel lvl) {
    sSuppress = lvl;
}

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

static void IO_printImpl(int color_id, int is_progress, int is_error, FILE* stream, const char* msg, const char* fmt, va_list va) {
    const char* num_map[] = {
        "0", "1", "2", "3", "4",
        "5", "6", "7", "8", "9",
    };
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
    if (sProgress && !is_progress) {
        printf("\n");
        sProgress = false;
    }
    
    size_t bufsize = 128 + strlen(fmt) * 4;
    char* buffer = malloc(bufsize);
    
    vsnprintf(buffer, bufsize, fmt, va);
    
    pthread_mutex_lock(&sIoMutex);
    if (is_progress)
        fputs("\r", stream);
    
    forline(line, buffer) {
        char fmt[64] = { "%." };
        int llen = linelen(line);
        int dlen = digint(llen);
        
        for (int i = dlen; i > 0; i--)
            strcat(fmt, num_map[valdig(llen, i)]);
        strcat(fmt, "s");
        
        fputs(color[color_id], stream);
        if (msg) fprintf(stream, "%-16s " PRNT_RSET, msg);
        fprintf(stream, fmt, line);
        if (!is_progress)
            fputs("\n", stream);
        
        color_id = 0;
    }
    
    fflush(stream);
    pthread_mutex_unlock(&sIoMutex);
    
    free(buffer);
}

static void IO_printCall(int color_id, int is_progress, FILE* stream, const char* msg, const char* fmt, ...) {
    va_list va;
    
    va_start(va, fmt);
    IO_printImpl(color_id, is_progress, false, stream, msg, fmt, va);
    va_end(va);
}

void warn(const char* fmt, ...) {
    if (sSuppress >= PSL_NO_WARNING)
        return;
    
    va_list args;
    va_start(args, fmt);
    IO_printImpl(2, false, false, stderr, NULL, fmt, args);
    va_end(args);
}

void warn_align(const char* info, const char* fmt, ...) {
    if (sSuppress >= PSL_NO_WARNING)
        return;
    
    va_list args;
    
    va_start(args, fmt);
    IO_printImpl(2, false, false, stderr, info, fmt, args);
    va_end(args);
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

void info_progff(const char* info, u32 a, u32 b) {
    if (sSuppress >= PSL_NO_INFO)
        return;
    
    IO_printCall(1, true, stdout, info,
        x_fmt("[ %c%dd / %c-%dd ]", '%', digint(b), '%', digint(b)), a, b);
    sProgress = true;
    
    if (a == b) {
        sProgress = false;
        printf("\n");
    }
}

void info_prog(const char* info, u32 a, u32 b) {
    if (sSuppress >= PSL_NO_INFO)
        return;
    
    if (a != b && sProgress) {
        static struct timeval p;
        
        if (a <= 1)
            gettimeofday(&p, 0);
        else {
            struct timeval t;
            
            gettimeofday(&t, 0);
            f32 sec = t.tv_sec - p.tv_sec + (f32)(t.tv_usec - p.tv_usec) * 0.000001f;
            
            if (sec >= 0.2f) {
                p = t;
            } else
                return;
        }
    }
    
    info_progff(info, a, b);
}

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

void IO_FixWin32(void) {
#ifdef _WIN32
    system("");
#endif
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
    
    printf("\n");
}

// # # # # # # # # # # # # # # # # # # # #
// # _log                                 #
// # # # # # # # # # # # # # # # # # # # #

#include <signal.h>

#define FAULT_BUFFER_SIZE (1024)
#define FAULT_LOG_NUM     16

static char* sLogMsg[FAULT_LOG_NUM];
static char* sLogFunc[FAULT_LOG_NUM];
static u32 sLogLine[FAULT_LOG_NUM];
static vs32 sLogInit;
static mutex_t sLogMutex;

static void _log_print_title(int arg, FILE* file) {
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
    
    if (sProgress)
        fprintf(file, "\n");
    
    if (arg != 0xDEADBEEF)
        fprintf(file, "" PRNT_GRAY "[ " PRNT_REDD "%s " PRNT_GRAY "]\n", errorMsg[clamp_max(arg, 16)]);
    else
        fprintf(file, "" PRNT_GRAY "[ " PRNT_REDD "_log " PRNT_GRAY "]\n");
    fprintf(file, "\n");
}

static void _log_print_log(int arg, FILE* file) {
    
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

static void _log_signal(int arg) {
    if (!sLogInit)
        return;
    
    pthread_mutex_lock(&sLogMutex);
    
    sLogInit = false;
    sAbort = true;
    
    _log_print_title(arg, stderr);
    _log_print_log(arg, stderr);
    
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

void _log_init() {
    for (int i = 1; i < 16; i++)
        signal(i, _log_signal);
    for (int i = 0; i < FAULT_LOG_NUM; i++) {
        sLogMsg[i] = calloc(FAULT_BUFFER_SIZE);
        sLogFunc[i] = calloc(128);
    }
    
    sLogInit = true;
    
    pthread_mutex_init(&sLogMutex, 0);
    pthread_mutex_init(&sIoMutex, 0);
}

void _log_dest() {
    for (int i = 0; i < FAULT_LOG_NUM; i++)
        free(sLogMsg[i], sLogFunc[i]);
    pthread_mutex_destroy(&sLogMutex);
    pthread_mutex_destroy(&sIoMutex);
}

void _log_print() {
    if (!sLogInit)
        return;
    if (sLogMsg[0] == NULL)
        return;
    if (sLogMsg[0][0] != 0)
        _log_signal(0xDEADBEEF);
}

void __log__(const char* func, u32 line, const char* txt, ...) {
    if (!sLogInit)
        return;
    
    if (sLogMsg[0] == NULL)
        return;
    
    pthread_mutex_lock(&sLogMutex);
    {
        arrmove_r(sLogMsg, 0, FAULT_LOG_NUM);
        arrmove_r(sLogFunc, 0, FAULT_LOG_NUM);
        arrmove_r(sLogLine, 0, FAULT_LOG_NUM);
        
        va_list args;
        va_start(args, txt);
        vsnprintf(sLogMsg[0], FAULT_BUFFER_SIZE, txt, args);
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
