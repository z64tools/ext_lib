#include <ext_lib.h>
#include <sys/time.h>

static void _log_signal(int arg);

static vbool sAbort;
static u8 sProgress;
static enum IOLevel sSuppress = 0;
static mutex_t sIoMutex;

// # # # # # # # # # # # # # # # # # # # #
// #                                     #
// # # # # # # # # # # # # # # # # # # # #

void IO_SetLevel(enum IOLevel lvl) {
    sSuppress = lvl;
}

void IO_lock() {
    pthread_mutex_lock(&sIoMutex);
}

void IO_unlock() {
    pthread_mutex_unlock(&sIoMutex);
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
        xl_fprint(stream, "\n");
        sProgress = false;
    }
    
    const size_t bufsize = 2048;
    char buffer[bufsize];
    
    xl_vsnprintf(buffer, bufsize, fmt, va);
    
    IO_lock();
    if (is_progress)
        xl_fprint(stream, "\r");
    
    forline(line, buffer) {
        char fmt[64] = { "%." };
        int llen = linelen(line);
        int dlen = digint(llen);
        
        for (int i = dlen; i > 0; i--)
            strcat(fmt, num_map[valdig(llen, i)]);
        strcat(fmt, "s");
        
        xl_fprint(stream, "%s", color[color_id]);
        if (msg) xl_fprint(stream, "%-16s " PRNT_RSET, msg);
        xl_fprint(stream, fmt, line);
        if (!is_progress)
            xl_fprint(stream, "\n");
        
        color_id = 0;
    }
    
    fflush(stream);
    IO_unlock();
}

static void IO_printCall(int color_id, int is_progress, FILE* stream, const char* msg, const char* fmt, ...) {
    va_list va;
    
    va_start(va, fmt);
    IO_printImpl(color_id, is_progress, false, stream, msg, fmt, va);
    va_end(va);
}

void IO_graph(get_val_callback_t get, void* udata, int num, f64 max, f32 pow) {
    int cli_size[2];
    
    cli_getSize(cli_size);
    cli_clear();
    fflush(stdout);
    sys_sleep(0.25f);
    
    for (var x = 0; x < cli_size[0]; x++) {
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

void info_fastprog(const char* info, int a, int b) {
    static int prev;
    
    if (a + 1 == prev)
        return;
    
    if (sSuppress >= PSL_NO_INFO)
        return;
    
    IO_printCall(1, true, stdout, info,
        x_fmt("[ %%%dd / %%-%dd ]", digint(b), digint(b)), a, b);
    sProgress = true;
    prev = a + 1;
    
    if (a == b) {
        sProgress = false;
        xl_fprint(stdout, "\n");
    }
}

void info_prog(const char* info, int a, int b) {
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
    
    info_fastprog(info, a, b);
}

void info_progf(const char* info, f64 a, f64 b) {
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
    
    static f32 prev;
    
    if (a + 1 == prev)
        return;
    
    if (sSuppress >= PSL_NO_INFO)
        return;
    
    IO_printCall(1, true, stdout, info,
        x_fmt("[ %%%d.2f / %%-%d.2f ]", digint(b), digint(b)), a, b);
    sProgress = true;
    prev = a + 1;
    
    if (a == b) {
        sProgress = false;
        xl_fprint(stdout, "\n");
    }
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
    
    xl_fprint(stdout, "\n");
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
        vfree(sLogMsg[i], sLogFunc[i]);
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

// # # # # # # # # # # # # # # # # # # # #
// #                                     #
// # # # # # # # # # # # # # # # # # # # #

/* format read states */
#define DP_S_DEFAULT 0
#define DP_S_FLAGS   1
#define DP_S_MIN     2
#define DP_S_DOT     3
#define DP_S_MAX     4
#define DP_S_MOD     5
#define DP_S_CONV    6
#define DP_S_DONE    7

/* format flags - Bits */
#define DP_F_MINUS    (1 << 0)
#define DP_F_PLUS     (1 << 1)
#define DP_F_SPACE    (1 << 2)
#define DP_F_NUM      (1 << 3)
#define DP_F_ZERO     (1 << 4)
#define DP_F_UP       (1 << 5)
#define DP_F_UNSIGNED (1 << 6)

/* Conversion Flags */
#define DP_C_SHORT   1
#define DP_C_LONG    2
#define DP_C_LDOUBLE 3
#define DP_C_LLONG   4

#define char_to_int(p) ((p) - '0')
#ifndef MAX
#define MAX(p, q) (((p) >= (q)) ? (p) : (q))
#endif

static void dopr_outch(char* buffer, size_t* currlen, size_t maxlen, char c) {
    if (*currlen < maxlen) {
        buffer[(*currlen)] = c;
    }
    (*currlen)++;
}

static void fmtint(char* buffer, size_t* currlen, size_t maxlen, long value, int base, int min, int max, int flags) {
    int signvalue = 0;
    unsigned long uvalue;
    char convert[42];
    int place = 0;
    int spadlen = 0; /* amount to space pad */
    int zpadlen = 0; /* amount to zero pad */
    int caps = 0;
    
    if (max < 0)
        max = 0;
    
    uvalue = value;
    
    if (!(flags & DP_F_UNSIGNED)) {
        if ( value < 0 ) {
            signvalue = '-';
            uvalue = -value;
        } else {
            if (flags & DP_F_PLUS) /* Do a sign (+/i) */
                signvalue = '+';
            else if (flags & DP_F_SPACE)
                signvalue = ' ';
        }
    }
    
    if (flags & DP_F_UP) caps = 1;  /* Should characters be upper case? */
    
    do {
        convert[place++] =
            (caps? "0123456789ABCDEF":"0123456789abcdef")
            [uvalue % (unsigned)base  ];
        uvalue = (uvalue / (unsigned)base );
    } while (uvalue && (place < 42));
    if (place == 42) place--;
    convert[place] = 0;
    
    zpadlen = max - place;
    spadlen = min - MAX (max, place) - (signvalue ? 1 : 0);
    if (zpadlen < 0) zpadlen = 0;
    if (spadlen < 0) spadlen = 0;
    if (flags & DP_F_ZERO) {
        zpadlen = MAX(zpadlen, spadlen);
        spadlen = 0;
    }
    if (flags & DP_F_MINUS)
        spadlen = -spadlen;    /* Left Justifty */
    
#ifdef DEBUG_SNPRINTF
    printf("zpad: %d, spad: %d, min: %d, max: %d, place: %d\n",
        zpadlen, spadlen, min, max, place);
#endif
    
    /* Spaces */
    while (spadlen > 0) {
        dopr_outch (buffer, currlen, maxlen, ' ');
        --spadlen;
    }
    
    /* Sign */
    if (signvalue)
        dopr_outch (buffer, currlen, maxlen, signvalue);
    
    /* Zeros */
    if (zpadlen > 0) {
        while (zpadlen > 0) {
            dopr_outch (buffer, currlen, maxlen, '0');
            --zpadlen;
        }
    }
    
    /* Digits */
    while (place > 0)
        dopr_outch (buffer, currlen, maxlen, convert[--place]);
    
    /* Left Justified spaces */
    while (spadlen < 0) {
        dopr_outch (buffer, currlen, maxlen, ' ');
        ++spadlen;
    }
}

static long double abs_val(long double value) {
    long double result = value;
    
    if (value < 0)
        result = -value;
    
    return result;
}

static long double POW10(int exp) {
    long double result = 1;
    
    while (exp) {
        result *= 10;
        exp--;
    }
    
    return result;
}

static long long ROUND(long double value) {
    long long intpart;
    
    intpart = (long long)value;
    value = value - intpart;
    if (value >= 0.5) intpart++;
    
    return intpart;
}

static double my_modf(double x0, double* iptr) {
    int i;
    long l;
    double x = x0;
    double f = 1.0;
    
    for (i = 0; i<100; i++) {
        l = (long)x;
        if (l <= (x + 1) && l >= (x - 1)) break;
        x *= 0.1;
        f *= 10.0;
    }
    
    if (i == 100) {
        /* yikes! the number is beyond what we can handle. What do we do? */
        (*iptr) = 0;
        return 0;
    }
    
    if (i != 0) {
        double i2;
        double ret;
        
        ret = my_modf(x0 - l * f, &i2);
        (*iptr) = l * f + i2;
        return ret;
    }
    
    (*iptr) = l;
    return x - (*iptr);
}

static void fmtfp(char* buffer, size_t* currlen, size_t maxlen, long double fvalue, int min, int max, int flags) {
    int signvalue = 0;
    double ufvalue;
    char iconvert[311];
    char fconvert[311];
    int iplace = 0;
    int fplace = 0;
    int padlen = 0;            /* amount to pad */
    int zpadlen = 0;
    int caps = 0;
    int index;
    double intpart;
    double fracpart;
    double temp;
    
    /*
     * AIX manpage says the default is 0, but Solaris says the default
     * is 6, and sprintf on AIX defaults to 6
     */
    if (max < 0)
        max = 6;
    
    ufvalue = abs_val (fvalue);
    
    if (fvalue < 0) {
        signvalue = '-';
    } else {
        if (flags & DP_F_PLUS) { /* Do a sign (+/i) */
            signvalue = '+';
        } else {
            if (flags & DP_F_SPACE)
                signvalue = ' ';
        }
    }
    
#if 0
    if (flags & DP_F_UP) caps = 1;  /* Should characters be upper case? */
#endif
    
#if 0
    if (max == 0) ufvalue += 0.5;   /* if max = 0 we must round */
#endif
    
    /*
     * Sorry, we only support 16 digits past the decimal because of our
     * conversion method
     */
    if (max > 16)
        max = 16;
    
    /* We "cheat" by converting the fractional part to integer by
     * multiplying by a factor of 10
     */
    
    temp = ufvalue;
    my_modf(temp, &intpart);
    
    fracpart = ROUND((POW10(max)) * (ufvalue - intpart));
    
    if (fracpart >= POW10(max)) {
        intpart++;
        fracpart -= POW10(max);
    }
    
    /* Convert integer part */
    do {
        temp = intpart;
        my_modf(intpart * 0.1, &intpart);
        temp = temp * 0.1;
        index = (int) ((temp - intpart + 0.05) * 10.0);
        /* index = (int) (((double)(temp*0.1) -intpart +0.05) *10.0); */
        /* printf ("%llf, %f, %x\n", temp, intpart, index); */
        iconvert[iplace++] =
            (caps? "0123456789ABCDEF":"0123456789abcdef")[index];
    } while (intpart && (iplace < 311));
    if (iplace == 311) iplace--;
    iconvert[iplace] = 0;
    
    /* Convert fractional part */
    if (fracpart) {
        do {
            temp = fracpart;
            my_modf(fracpart * 0.1, &fracpart);
            temp = temp * 0.1;
            index = (int) ((temp - fracpart + 0.05) * 10.0);
            /* index = (int) ((((temp/10) -fracpart) +0.05) *10); */
            /* printf ("%lf, %lf, %ld\n", temp, fracpart, index); */
            fconvert[fplace++] =
                (caps? "0123456789ABCDEF":"0123456789abcdef")[index];
        } while (fracpart && (fplace < 311));
        if (fplace == 311) fplace--;
    }
    fconvert[fplace] = 0;
    
    /* -1 for decimal point, another -1 if we are printing a sign */
    padlen = min - iplace - max - 1 - ((signvalue) ? 1 : 0);
    zpadlen = max - fplace;
    if (zpadlen < 0) zpadlen = 0;
    if (padlen < 0)
        padlen = 0;
    if (flags & DP_F_MINUS)
        padlen = -padlen;      /* Left Justifty */
    
    if ((flags & DP_F_ZERO) && (padlen > 0)) {
        if (signvalue) {
            dopr_outch (buffer, currlen, maxlen, signvalue);
            --padlen;
            signvalue = 0;
        }
        while (padlen > 0) {
            dopr_outch (buffer, currlen, maxlen, '0');
            --padlen;
        }
    }
    while (padlen > 0) {
        dopr_outch (buffer, currlen, maxlen, ' ');
        --padlen;
    }
    if (signvalue)
        dopr_outch (buffer, currlen, maxlen, signvalue);
    
    while (iplace > 0)
        dopr_outch (buffer, currlen, maxlen, iconvert[--iplace]);
    
#ifdef DEBUG_SNPRINTF
    printf("fmtfp: fplace=%d zpadlen=%d\n", fplace, zpadlen);
#endif
    
    /*
     * Decimal point.  This should probably use locale to find the correct
     * char to print out.
     */
    if (max > 0) {
        dopr_outch (buffer, currlen, maxlen, '.');
        
        while (fplace > 0)
            dopr_outch (buffer, currlen, maxlen, fconvert[--fplace]);
    }
    
    while (zpadlen > 0) {
        dopr_outch (buffer, currlen, maxlen, '0');
        --zpadlen;
    }
    
    while (padlen < 0) {
        dopr_outch (buffer, currlen, maxlen, ' ');
        ++padlen;
    }
}

static void fmtstr(char* buffer, size_t* currlen, size_t maxlen, char* value, int flags, int min, int max) {
    int padlen;                /* amount to pad */
    int cnt = 0;
    
    if (value == 0) value = "(null)";
    
    // int lena = strlen(value);
    int lenb = strvlen(value);
    
    padlen = min - lenb;
    if (padlen < 0) padlen = 0;
    if (flags & DP_F_MINUS) padlen = -padlen;
    
    while ((padlen > 0) && (max == -1 || cnt < max)) {
        dopr_outch (buffer, currlen, maxlen, ' ');
        --padlen;
        ++cnt;
    }
    
    while (*value && (max == -1 || cnt < max)) {
        dopr_outch (buffer, currlen, maxlen, *value++);
        ++cnt;
    }
    
    while ((padlen < 0) && (max == -1 || cnt < max)) {
        dopr_outch (buffer, currlen, maxlen, ' ');
        ++padlen;
        ++cnt;
    }
}

size_t xl_vsnprintf(char* buffer, size_t maxlen, const char* format, va_list args) {
    char ch;
    long long value;
    long double fvalue;
    char* strvalue;
    int min;
    int max;
    int state;
    int flags;
    int cflags;
    size_t currlen;
    
    state = DP_S_DEFAULT;
    currlen = flags = cflags = min = 0;
    max = -1;
    ch = *format++;
    
    while (state != DP_S_DONE) {
        if (ch == '\0')
            state = DP_S_DONE;
        
        switch (state) {
            case DP_S_DEFAULT:
                if (ch == '%')
                    state = DP_S_FLAGS;
                else
                    dopr_outch (buffer, &currlen, maxlen, ch);
                ch = *format++;
                break;
            case DP_S_FLAGS:
                switch (ch) {
                    case '-':
                        flags |= DP_F_MINUS;
                        ch = *format++;
                        break;
                    case '+':
                        flags |= DP_F_PLUS;
                        ch = *format++;
                        break;
                    case ' ':
                        flags |= DP_F_SPACE;
                        ch = *format++;
                        break;
                    case '#':
                        flags |= DP_F_NUM;
                        ch = *format++;
                        break;
                    case '0':
                        flags |= DP_F_ZERO;
                        ch = *format++;
                        break;
                    default:
                        state = DP_S_MIN;
                        break;
                }
                break;
            case DP_S_MIN:
                if (isdigit((unsigned char)ch)) {
                    min = 10 * min + char_to_int (ch);
                    ch = *format++;
                } else if (ch == '*') {
                    min = va_arg (args, int);
                    ch = *format++;
                    state = DP_S_DOT;
                } else {
                    state = DP_S_DOT;
                }
                break;
            case DP_S_DOT:
                if (ch == '.') {
                    state = DP_S_MAX;
                    ch = *format++;
                } else {
                    state = DP_S_MOD;
                }
                break;
            case DP_S_MAX:
                if (isdigit((unsigned char)ch)) {
                    if (max < 0)
                        max = 0;
                    max = 10 * max + char_to_int (ch);
                    ch = *format++;
                } else if (ch == '*') {
                    max = va_arg (args, int);
                    ch = *format++;
                    state = DP_S_MOD;
                } else {
                    state = DP_S_MOD;
                }
                break;
            case DP_S_MOD:
                switch (ch) {
                    case 'h':
                        cflags = DP_C_SHORT;
                        ch = *format++;
                        break;
                    case 'l':
                        cflags = DP_C_LONG;
                        ch = *format++;
                        if (ch == 'l') { /* It's a long long */
                            cflags = DP_C_LLONG;
                            ch = *format++;
                        }
                        break;
                    case 'L':
                        cflags = DP_C_LDOUBLE;
                        ch = *format++;
                        break;
                    default:
                        break;
                }
                state = DP_S_CONV;
                break;
            case DP_S_CONV:
                switch (ch) {
                    case 'b':
                        if (cflags == DP_C_SHORT)
                            value = va_arg (args, int);
                        else if (cflags == DP_C_LONG)
                            value = va_arg (args, long int);
                        else if (cflags == DP_C_LLONG)
                            value = va_arg (args, long long);
                        else
                            value = va_arg (args, int);
                        flags |= DP_F_UNSIGNED;
                        fmtint (buffer, &currlen, maxlen, (value), 2, min, max, flags);
                        break;
                    case 'B':
                        value = va_arg (args, int);
                        fmtstr (buffer, &currlen, maxlen, value ? "true" : "false", flags, min, max);
                        break;
                    case 'd':
                    case 'i':
                        if (cflags == DP_C_SHORT)
                            value = va_arg (args, int);
                        else if (cflags == DP_C_LONG)
                            value = va_arg (args, long int);
                        else if (cflags == DP_C_LLONG)
                            value = va_arg (args, long long);
                        else
                            value = va_arg (args, int);
                        fmtint (buffer, &currlen, maxlen, value, 10, min, max, flags);
                        break;
                    case 'o':
                        flags |= DP_F_UNSIGNED;
                        if (cflags == DP_C_SHORT)
                            value = va_arg (args, unsigned int);
                        else if (cflags == DP_C_LONG)
                            value = (long)va_arg (args, unsigned long int);
                        else if (cflags == DP_C_LLONG)
                            value = (long)va_arg (args, unsigned long long);
                        else
                            value = (long)va_arg (args, unsigned int);
                        fmtint (buffer, &currlen, maxlen, value, 8, min, max, flags);
                        break;
                    case 'u':
                        flags |= DP_F_UNSIGNED;
                        if (cflags == DP_C_SHORT)
                            value = va_arg (args, unsigned int);
                        else if (cflags == DP_C_LONG)
                            value = (long)va_arg (args, unsigned long int);
                        else if (cflags == DP_C_LLONG)
                            value = (long long)va_arg (args, unsigned long long);
                        else
                            value = (long)va_arg (args, unsigned int);
                        fmtint (buffer, &currlen, maxlen, value, 10, min, max, flags);
                        break;
                    case 'X':
                        flags |= DP_F_UP;
                    case 'x':
                        flags |= DP_F_UNSIGNED;
                        if (cflags == DP_C_SHORT)
                            value = va_arg (args, unsigned int);
                        else if (cflags == DP_C_LONG)
                            value = (long)va_arg (args, unsigned long int);
                        else if (cflags == DP_C_LLONG)
                            value = (long long)va_arg (args, unsigned long long);
                        else
                            value = (long)va_arg (args, unsigned int);
                        fmtint (buffer, &currlen, maxlen, value, 16, min, max, flags);
                        break;
                    case 'f':
                        if (cflags == DP_C_LDOUBLE)
                            fvalue = va_arg (args, long double);
                        else
                            fvalue = va_arg (args, double);
                        /* um, floating point? */
                        fmtfp (buffer, &currlen, maxlen, fvalue, min, max, flags);
                        break;
                    case 'E':
                        flags |= DP_F_UP;
                    case 'e':
                        if (cflags == DP_C_LDOUBLE)
                            fvalue = va_arg (args, long double);
                        else
                            fvalue = va_arg (args, double);
                        break;
                    case 'G':
                        flags |= DP_F_UP;
                    case 'g':
                        if (cflags == DP_C_LDOUBLE)
                            fvalue = va_arg (args, long double);
                        else
                            fvalue = va_arg (args, double);
                        break;
                    case 'c':
                        dopr_outch (buffer, &currlen, maxlen, va_arg (args, int));
                        break;
                    case 's':
                        strvalue = va_arg (args, char*);
                        if (min > 0 && max >= 0 && min > max) max = min;
                        fmtstr (buffer, &currlen, maxlen, strvalue, flags, min, max);
                        break;
                    case 'p':
                        strvalue = va_arg (args, void*);
                        fmtint (buffer, &currlen, maxlen, (long) strvalue, 16, min, max, flags);
                        break;
                    case 'n':
                        if (cflags == DP_C_SHORT) {
                            short int* num;
                            num = va_arg (args, short int*);
                            *num = currlen;
                        } else if (cflags == DP_C_LONG) {
                            long int* num;
                            num = va_arg (args, long int*);
                            *num = (long int)currlen;
                        } else if (cflags == DP_C_LLONG) {
                            long long* num;
                            num = va_arg (args, long long*);
                            *num = (long long)currlen;
                        } else {
                            int* num;
                            num = va_arg (args, int*);
                            *num = currlen;
                        }
                        break;
                    case '%':
                        dopr_outch (buffer, &currlen, maxlen, ch);
                        break;
                    case 'w':
                        /* not supported yet, treat as next char */
                        ch = *format++;
                        break;
                }
                
                ch = *format++;
                state = DP_S_DEFAULT;
                flags = cflags = min = 0;
                max = -1;
                break;
            case DP_S_DONE:
                break;
        }
    }
    if (maxlen != 0) {
        if (currlen < maxlen - 1)
            buffer[currlen] = '\0';
        else if (maxlen > 0)
            buffer[maxlen - 1] = '\0';
    }
    
    return currlen;
}

size_t xl_snprintf(char* buffer, size_t maxlen, const char* format, ...) {
    va_list va;
    
    va_start(va, format);
    size_t r = xl_vsnprintf(buffer, maxlen, format, va);
    va_end(va);
    
    return r;
}

size_t xl_fprint(FILE* s, const char* fmt, ...) {
    char buffer[1024 + 1];
    va_list va;
    
    va_start(va, fmt);
    size_t r = xl_vsnprintf(buffer, 1024, fmt, va);
    fputs(buffer, s);
    va_end(va);
    
    return r;
}
