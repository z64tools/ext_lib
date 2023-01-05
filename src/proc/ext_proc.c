#include <ext_proc.h>
#include <reproc/reproc.h>

enum ProcState {
    PROC_NEW,
    PROC_EXEC,
    PROC_KILL,
    PROC_JOIN,
    PROC_FREE,
};

static size_t argtok_size(const char* s) {
    size_t sz = 0;
    bool string = *s == '"';
    
    if (string) {
        sz = 2;
isString:
        while (!(s[sz - 1] == '"' && s[sz - 2] != '\\') && s[sz] != '\0')
            sz++;
        
        return sz;
    } else {
        while (!(s[sz] == ' ' && s[sz - 1] != '\\') && s[sz] != '\0' && s[sz] != '&') {
            sz++;
            
            if (s[sz] == '\"') {
                if (strlen(&s[sz]) > 2) {
                    
                    sz += 2;
                    goto isString;
                }
            }
        }
        
        return sz;
    }
}

static char* argtok_next(const char* s) {
    s += argtok_size(s);
    if (*s == '\0') return NULL;
    s += strspn(s, " \t");
    if (*s == '&') return NULL;
    
    return (char*)s;
}

static char* argtok_cpy(const char* s) {
    size_t sz = argtok_size(s);
    
    if (!sz) return NULL;
    
    return strndup(s, sz);
}

proc_t* proc_new(char* fmt, ...) {
    char buffer[8192];
    proc_t* this = new(proc_t);
    char* tok;
    char* args[512];
    va_list va;
    
    va_start(va, fmt);
    vsnprintf(buffer, 8192, fmt, va);
    va_end(va);
    
    tok = buffer;
    
    _assert(this != NULL);
    _assert(tok != NULL);
    _assert((this->proc = reproc_new()) != NULL);
    
    this->localStatus = PROC_NEW;
    
    while (tok) {
        args[this->numArg++] = argtok_cpy(tok);
        tok = argtok_next(tok);
    }
    
    this->arg = calloc(sizeof(char*) * (this->numArg + 1));
    
    for (s32 i = 0; i < this->numArg; i++)
        this->arg[i] = args[i];
    
    return this;
}

void proc_add_arg(proc_t* this, char* fmt, ...) {
    char buffer[8192];
    char* tok;
    char* args[512] = {};
    va_list va;
    u32 numArg = 0;
    
    va_start(va, fmt);
    vsnprintf(buffer, 8192, fmt, va);
    va_end(va);
    
    tok = buffer;
    
    while (tok) {
        args[numArg++] = argtok_cpy(tok);
        tok = argtok_next(tok);
    }
    
    this->arg = realloc(this->arg, sizeof(char*) * (this->numArg + numArg + 1));
    
    for (s32 i = 0; i <= numArg; i++)
        this->arg[this->numArg + i] = args[i];
    
    this->numArg += numArg;
}

void proc_set_state(proc_t* this, proc_state_t state) {
    this->state |= state;
}

int proc_set_path(proc_t* this, const char* path) {
    if (sys_isdir(path))
        return (this->path = strdup(path)) ? 0 : 1;
    
    return 1;
}

void proc_set_env(proc_t* this, const char* env) {
    if (!this->env) this->env = calloc(sizeof(char*) * 32);
    
    for (s32 i = 0; i < 32; i++) {
        if (this->env[i] != NULL)
            continue;
        
        this->env[i] = strdup(env);
        break;
    }
}

static s32 proc_sys_thread(proc_t* this) {
    char* args = strarrcat(this->arg, " ");
    s32 sys = system(args);
    
    free(args);
    
    return sys;
}

static void proc_err(proc_t* this) {
    char* msg = "Unknown";
    
    _log("ProcError");
    
    thd_lock();
    
    sys_sleep(0.5);
    
    switch (this->localStatus) {
        case PROC_NEW:
            msg = "New";
            break;
        case PROC_EXEC:
            msg = "Exec";
            break;
        case PROC_KILL:
            msg = "Kill";
            break;
        case PROC_JOIN:
            msg = "Join";
            break;
        case PROC_FREE:
            msg = "free";
            break;
    }
    
    print_warn("proc_t Error: %s", msg);
    
    for (s32 i = 0; i < this->numArg; i++)
        fprintf(stderr, "" PRNT_GRAY "%-6d" PRNT_RSET "%s\n", i, this->arg[i]);
    
    if (this->env) {
        print_warn("proc_t Env:");
        for (s32 i = 0; this->env[i] != NULL; i++)
            fprintf(stderr, "" PRNT_GRAY "%-6d" PRNT_RSET "%s\n", i, this->env[i]);
    }
    
    print_warn("proc_t Path:\n      %s", this->path ? this->path : sys_workdir());
    
    #define fprint_proc_enum(enum) fprintf(stderr, "" PRNT_GRAY "%-20s" PRNT_RSET "%s\n", #enum, this->state & enum ? "" PRNT_BLUE "true" : "" PRNT_REDD "false");
    print_warn("proc_t State:");
    fprint_proc_enum(PROC_MUTE_STDOUT);
    fprint_proc_enum(PROC_MUTE_STDERR);
    fprint_proc_enum(PROC_MUTE_STDIN);
    fprint_proc_enum(PROC_WRITE_STDIN);
    fprint_proc_enum(PROC_THROW_ERROR);
    fprint_proc_enum(PROC_SYSTEM_EXE);
    
    print_warn("proc_t Signal: %d\n\n", this->signal);
    if (this->msg)
        print_error("%s", this->msg);
    
    else
        print_error("Closing!");
    
    thd_unlock();
}

int proc_exec(proc_t* this) {
    bool exec;
    reproc_options opt = {
        .redirect.out.type = this->state & PROC_MUTE_STDOUT ? REPROC_REDIRECT_PIPE : REPROC_REDIRECT_PARENT,
        .redirect.err.type = this->state & PROC_MUTE_STDERR ? REPROC_REDIRECT_PIPE : REPROC_REDIRECT_PARENT,
        .redirect.in.type  = this->state & PROC_MUTE_STDIN ? REPROC_REDIRECT_PIPE : REPROC_REDIRECT_PARENT,
        .working_directory = this->path,
        .env.extra         = this->env,
        .env.behavior      = this->env ? REPROC_ENV_EMPTY : 0,
    };
    
    this->localStatus = PROC_EXEC;
    
    if (this->state & PROC_SYSTEM_EXE) {
        thd_create(&this->thd, proc_sys_thread, this);
        
        return 0;
    }
    
    exec = reproc_start(this->proc, this->arg, opt) < 0;
    
    if (exec && this->state & PROC_THROW_ERROR)
        proc_err(this);
    
    return exec;
}

char* proc_read(proc_t* this, proc_read_target_t target) {
    memfile_t mem = memfile_new();
    REPROC_STREAM s = -1;
    
    if (target == READ_STDOUT && this->state & PROC_MUTE_STDOUT)
        s = REPROC_STREAM_OUT;
    else if (target == READ_STDERR && this->state & PROC_MUTE_STDERR)
        s = REPROC_STREAM_ERR;
    else if (this->state & PROC_THROW_ERROR) {
        print_warn("Could not read [%s] because it hasn't been muted!", target == READ_STDOUT ? "stdout" : "stderr");
        proc_err(this);
    } else
        return NULL;
    
    while (true) {
        char buffer[4096];
        s32 readSize = reproc_read(this->proc, s, (u8*)buffer, sizeof(buffer));
        
        if (readSize < 0)
            break;
        if (readSize == 0)
            continue;
        if (mem.data == NULL)
            memfile_alloc(&mem, 4096);
        
        if (!memfile_write(&mem, buffer, readSize)) {
            print_warn("Failed to write buffer!");
            proc_err(this);
        }
    }
    
    if (mem.data)
        memfile_write(&mem, "\0", 1);
    
    this->msg = mem.data;
    
    return mem.data;
}

static int proc_free(proc_t* this) {
    int signal = 0;
    
    if (this->state & PROC_SYSTEM_EXE)
        signal = this->signal;
    
    else {
        this->signal = signal = reproc_wait(this->proc, REPROC_INFINITE);
        reproc_destroy(this->proc);
    }
    
    if (signal && this->state & PROC_THROW_ERROR)
        proc_err(this);
    
    this->localStatus = PROC_FREE;
    _log("free");
    
    if (this->env) {
        for (s32 i = 0; i < 32; i++)
            free(this->env[i]);
        
        free(this->env);
    }
    
    for (s32 i = 0; i < this->numArg; i++)
        free(this->arg[i]);
    
    free(this->path);
    free(this->arg);
    free(this);
    
    return signal;
}

int proc_kill(proc_t* this) {
    this->localStatus = PROC_KILL;
    
    if (this->state & PROC_SYSTEM_EXE) {
        this->signal = 1;
        int pthread_kill(pthread_t t, int sig);
        if (!pthread_kill(this->thd, 1))
            return proc_free(this);
        
        return 1;
    }
    reproc_terminate(this->proc);
    reproc_kill(this->proc);
    
    return proc_free(this);
    
}

int proc_join(proc_t* this) {
    if (!this) return 0;
    this->localStatus = PROC_JOIN;
    
    if (this->state & PROC_SYSTEM_EXE) {
        pthread_join(this->thd, (void*)&this->signal);
        
        return proc_free(this);
    }
    
    return proc_free(this);
}
