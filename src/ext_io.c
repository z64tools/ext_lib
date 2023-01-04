#include <ext_lib.h>
#include <sys/time.h>

static void Log_Signal(int arg);

static vbool sAbort;
static u8 sProgress;
static PrintfSuppressLevel sSuppress = 0;

// # # # # # # # # # # # # # # # # # # # #
// # PRINTF                              #
// # # # # # # # # # # # # # # # # # # # #

void print_lvl(PrintfSuppressLevel lvl) {
    sSuppress = lvl;
}

void print_title(const char* toolname, const char* fmt, ...) {
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

static void __print_call(u32 type, FILE* file) {
    const char* color[4] = {
        PRNT_PRPL,
        PRNT_REDD,
        PRNT_REDD,
        PRNT_BLUE
    };
    
    fprintf(
        file,
        "" PRNT_GRAY "%s>" PRNT_GRAY ": " PRNT_RSET,
        color[type]
    );
}

void print_warn(const char* fmt, ...) {
    if (sSuppress >= PSL_NO_WARNING)
        return;
    
    if (sAbort)
        return;
    
    if (sProgress) {
        printf("\n");
        sProgress = false;
    }
    
    va_list args;
    
    va_start(args, fmt);
    __print_call(1, stderr);
    vfprintf(
        stderr,
        fmt,
        args
    );
    fputs("\n", stderr);
    va_end(args);
}

void print_warn_align(const char* info, const char* fmt, ...) {
    if (sSuppress >= PSL_NO_WARNING)
        return;
    
    if (sAbort)
        return;
    
    if (sProgress) {
        printf("\n");
        sProgress = false;
    }
    
    va_list args;
    
    va_start(args, fmt);
    __print_call(1, stdout);
    fprintf(
        stdout,
        "%-16s " PRNT_RSET,
        info
    );
    vfprintf(
        stdout,
        fmt,
        args
    );
    fputs("\n", stdout);
    va_end(args);
}

void print_kill(FILE* output) {
#ifdef _WIN32
    if (freopen ("NUL", "w", output))
        (void)0;
#else
    if (freopen ("/dev/null", "w", output))
        (void)0;
#endif
}

void print_error(const char* fmt, ...) {
    sAbort = 1;
    
    print_kill(stdout);
    Log_Signal(0xDEADBEEF);
    
    if (sSuppress < PSL_NO_ERROR) {
        if (sProgress) {
            fputs("\n", stderr);
            sProgress = false;
        }
        
        va_list args;
        
        va_start(args, fmt);
        
        __print_call(2, stderr);
        vfprintf(stderr, fmt, args);
        fputs("\n\a", stderr);
        fflush(stderr);
        
        va_end(args);
    }
    
#ifdef _WIN32
    cli_getc();
#endif
    
    exit(EXIT_FAILURE);
}

void print_error_align(const char* info, const char* fmt, ...) {
    sAbort = 1;
    
    print_kill(stdout);
    Log_Signal(0xDEADBEEF);
    
    if (sSuppress < PSL_NO_ERROR) {
        if (sProgress) {
            fputs("\n", stderr);
            sProgress = false;
        }
        
        va_list args;
        
        va_start(args, fmt);
        __print_call(2, stderr);
        fprintf(
            stderr,
            "%-16s " PRNT_RSET,
            info
        );
        vfprintf(
            stderr,
            fmt,
            args
        );
        fputs("\n\a", stderr);
        
        va_end(args);
    }
    
#ifdef _WIN32
    cli_getc();
#endif
    
    exit(EXIT_FAILURE);
}

void print_info(const char* fmt, ...) {
    
    if (sAbort)
        return;
    
    if (sSuppress >= PSL_NO_INFO)
        return;
    
    if (sProgress) {
        fputs("\n", stdout);
        sProgress = false;
    }
    va_list args;
    
    va_start(args, fmt);
    __print_call(3, stdout);
    vfprintf(
        stdout,
        fmt,
        args
    );
    fputs("\n", stdout);
    va_end(args);
}

void print_info_align(const char* info, const char* fmt, ...) {
    if (sAbort)
        return;
    
    if (sSuppress >= PSL_NO_INFO)
        return;
    
    if (sProgress) {
        fputs("\n", stdout);
        sProgress = false;
    }
    va_list args;
    
    va_start(args, fmt);
    __print_call(3, stdout);
    fprintf(
        stdout,
        "%-16s " PRNT_RSET,
        info
    );
    vfprintf(
        stdout,
        fmt,
        args
    );
    fputs("\n", stdout);
    va_end(args);
}

void print_prog_align(const char* info, const char* fmt, const char* color) {
    if (sAbort)
        return;
    
    if (sSuppress >= PSL_NO_INFO)
        return;
    
    if (sProgress) {
        printf("\n");
        sProgress = false;
    }
    
    printf("\n");
    cli_clearln(2);
    __print_call(3, stdout);
    printf("%-16s%s%s", info, color ? color : "", fmt);
}

void print_prog_fast(const char* info, u32 a, u32 b) {
    if (sSuppress >= PSL_NO_INFO) {
        return;
    }
    
    if (sAbort)
        return;
    
    printf(
        // "%-16s" PRNT_RSET "[%4d / %-4d]",
        x_fmt("\r" PRNT_BLUE ">" PRNT_GRAY ": " PRNT_RSET "%c-16s" PRNT_RSET " [ %c%dd / %c-%dd ]", '%', '%', digint(b), '%', digint(b)),
        info,
        a,
        b
    );
    sProgress = true;
    
    if (a == b) {
        sProgress = false;
        printf("\n");
    }
}

void print_prog(const char* info, u32 a, u32 b) {
    if (sSuppress >= PSL_NO_INFO)
        return;
    
    if (sAbort)
        return;
    
    if (a != b) {
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
    
    printf("\r");
    __print_call(3, stdout);
    printf(
        // "%-16s" PRNT_RSET "[%4d / %-4d]",
        x_fmt("%c-16s" PRNT_RSET " [ %c%dd / %c-%dd ]", '%', '%', digint(b), '%', digint(b)),
        info,
        a,
        b
    );
    sProgress = true;
    
    if (a == b) {
        sProgress = false;
        printf("\n");
    }
}

void print_getc(const char* txt) {
    print_info("%s", txt);
    cli_getc();
}

void print_volatile(const char* fmt, ...) {
    va_list va;
    
    if (sAbort)
        return;
    
    va_start(va, fmt);
    Mutex_Lock();
    print_win32_fix();
    vprintf(fmt, va);
    fflush(NULL);
    Mutex_Unlock();
    va_end(va);
}

void print_win32_fix(void) {
#ifdef _WIN32
    system("");
#endif
}

void print_hex(const char* txt, const void* data, u32 size, u32 dispOffset) {
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
        print_info("%s", txt);
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

void print_bit(const char* txt, const void* data, u32 size, u32 dispOffset) {
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
        print_info("%s", txt);
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

void print_nl(void) {
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
#define FAULT_LOG_NUM     28

static char* sLogMsg[FAULT_LOG_NUM];
static char* sLogFunc[FAULT_LOG_NUM];
static u32 sLogLine[FAULT_LOG_NUM];
static vs32 sLogInit;

static void Log_Signal_PrintTitle(int arg, FILE* file) {
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

static void Log_Printinf(int arg, FILE* file) {
    
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

static void Log_Signal(int arg) {
    static volatile bool ran = 0;
    
    if (!sLogInit)
        return;
    if (ran) return;
    ran = true;
    sLogInit = false;
    sAbort = true;
    
    Log_Signal_PrintTitle(arg, stderr);
    Log_Printinf(arg, stderr);
    
    if (arg == 0xDEADBEEF) {
        ran = false;
        sLogInit = true;
        sAbort = false;
        
        return;
    }
    
#ifdef _WIN32
    print_getc("Press enter to exit");
#endif
    exit(arg);
}

const_func Log_Init() {
    for (int i = 1; i < 16; i++)
        signal(i, Log_Signal);
    for (int i = 0; i < FAULT_LOG_NUM; i++) {
        sLogMsg[i] = calloc(FAULT_BUFFER_SIZE);
        sLogFunc[i] = calloc(128);
    }
    
    sLogInit = true;
}

dest_func Log_Free() {
    if (!sLogInit)
        return;
    sLogInit = false;
    for (int i = 0; i < FAULT_LOG_NUM; i++) {
        free(sLogMsg[i]);
        free(sLogFunc[i]);
    }
}

void Log_Print() {
    if (!sLogInit)
        return;
    if (sLogMsg[0] == NULL)
        return;
    if (sLogMsg[0][0] != 0)
        Log_Signal(0xDEADBEEF);
}

void __log__(const char* func, u32 line, const char* txt, ...) {
    
#if 0
    va_list va;
    char buf[512];
    
    va_start(va, txt);
    vsnprintf(buf, 512, txt, va);
    print_info("" PRNT_GRAY "[%s::%d]" PRNT_RSET " %s", func, line, buf);
    va_end(va);
    
    return;
#endif
    
    if (!sLogInit)
        return;
    
    if (sLogMsg[0] == NULL)
        return;
    
    Mutex_Lock();
    {
        arrmve_r(sLogMsg, 0, FAULT_LOG_NUM);
        arrmve_r(sLogFunc, 0, FAULT_LOG_NUM);
        arrmve_r(sLogLine, 0, FAULT_LOG_NUM);
        
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
    Mutex_Unlock();
}
