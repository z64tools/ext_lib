#include <ext_proc.h>
#include <reproc/reproc.h>
#undef Proc_AddEach

enum e_ProcState {
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

Proc* Proc_New(char* fmt, ...) {
    char buffer[8192];
    Proc* this = new(Proc);
    char* tok;
    char* args[512];
    va_list va;
    
    va_start(va, fmt);
    vsnprintf(buffer, 8192, fmt, va);
    va_end(va);
    
    tok = buffer;
    _assert(this != NULL);
    _assert(tok != NULL);
    
    while (tok) {
        args[this->numArg++] = argtok_cpy(tok);
        tok = argtok_next(tok);
    }
    
    this->arg = calloc(sizeof(char*) * (this->numArg + 1));
    
    for (int i = 0; i < this->numArg; i++)
        this->arg[i] = args[i];
    
    _assert((this->proc = reproc_new()) != NULL);
    this->localStatus = PROC_NEW;
    
    return this;
}

void Proc_AddArg(Proc* this, char* fmt, ...) {
    char buffer[8192];
    va_list va;
    
    va_start(va, fmt);
    vsnprintf(buffer, 8192, fmt, va);
    va_end(va);
    
    renew(this->arg, char*[this->numArg + 2]);
    this->arg[this->numArg++] = strdup(buffer);
    this->arg[this->numArg] = NULL;
}

void Proc_AddEach(Proc* this, ...) {
    va_list va;
    int num;
    
    va_start(va, this);
    
    num = va_arg(va, int);
    
    _log("num: %d", num);
    for (int i = 0; i < num; i++) {
        char* arg = va_arg(va, char*);
        
        _assert(arg != NULL);
        _log("arg: %s", arg);
        
        renew(this->arg, char*[this->numArg + 2]);
        this->arg[this->numArg++] = strdup(arg);
        this->arg[this->numArg] = NULL;
    }
    
    va_end(va);
}

void Proc_SetState(Proc* this, e_ProcState state) {
    this->state |= state;
}

int Proc_SetPath(Proc* this, const char* path) {
    if (sys_isdir(path))
        return (this->path = strdup(path)) ? 0 : 1;
    
    return 1;
}

void Proc_SetEnv(Proc* this, const char* env) {
    if (!this->env) this->env = calloc(sizeof(char*) * 32);
    
    for (int i = 0; i < 32; i++) {
        if (this->env[i] != NULL)
            continue;
        
        this->env[i] = strdup(env);
        break;
    }
}

static s32 Proc_SysThd(Proc* this) {
    char* args = strarrcat(this->arg, " ");
    s32 sys = system(args);
    
    vfree(args);
    
    return sys;
}

void Proc_Info(Proc* this) {
    for (int i = 0; i < this->numArg; i++)
        fprintf(stderr, "" PRNT_GRAY "%-6d" PRNT_RSET "%s\n", i, this->arg[i]);
    
    if (this->env) {
        warn("Proc Env:");
        for (int i = 0; this->env[i] != NULL; i++)
            fprintf(stderr, "" PRNT_GRAY "%-6d" PRNT_RSET "%s\n", i, this->env[i]);
    }
    
    warn("Proc Path:\n      %s", this->path ? this->path : sys_workdir());
    
    #define fprint_proc_enum(enum) fprintf(stderr, "" PRNT_GRAY "%-20s" PRNT_RSET "%s\n", #enum, this->state & enum ? "" PRNT_BLUE "true" : "" PRNT_REDD "false");
    warn("Proc State:");
    fprint_proc_enum(PROC_MUTE_STDOUT);
    fprint_proc_enum(PROC_MUTE_STDERR);
    fprint_proc_enum(PROC_OPEN_STDIN);
    fprint_proc_enum(PROC_THROW_ERROR);
    fprint_proc_enum(PROC_SYSTEM_EXE);
}

static void Proc_Err(Proc* this, const char* msg) {
    _log("ProcError");
    
    thd_lock();
    
    sys_sleep(0.5);
    
    if (!msg) {
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
    }
    
    warn("Proc Error: %s", msg);
    
    Proc_Info(this);
    
    warn("Proc Signal: %d\n\n", this->signal);
    if (this->msg)
        errr("%s", this->msg);
    
    else
        errr("Closing!");
    
    thd_unlock();
}

int Proc_Exec(Proc* this) {
    bool exec;
    reproc_options opt = {
        .redirect.out.type = this->state & PROC_MUTE_STDOUT ? REPROC_REDIRECT_PIPE : REPROC_REDIRECT_PARENT,
        .redirect.err.type = this->state & PROC_MUTE_STDERR ? REPROC_REDIRECT_PIPE : REPROC_REDIRECT_PARENT,
        .redirect.in.type  = this->state & PROC_OPEN_STDIN ? REPROC_REDIRECT_PIPE : REPROC_REDIRECT_PARENT,
        .working_directory = this->path,
        .env.extra         = this->env,
        .env.behavior      = this->env ? REPROC_ENV_EMPTY : 0,
    };
    
    this->localStatus = PROC_EXEC;
    
    if (this->state & PROC_SYSTEM_EXE) {
        thd_create(&this->thd, Proc_SysThd, this);
        
        return 0;
    }
    
    exec = reproc_start(this->proc, this->arg, opt) < 0;
    
    if (exec && this->state & PROC_THROW_ERROR)
        Proc_Err(this, NULL);
    
    return exec;
}

char* Proc_ReadLine(Proc* this, e_ProcRead target) {
    REPROC_STREAM s = -1;
    
    if (target == READ_STDOUT && this->state & PROC_MUTE_STDOUT)
        s = REPROC_STREAM_OUT;
    else if (target == READ_STDERR && this->state & PROC_MUTE_STDERR)
        s = REPROC_STREAM_ERR;
    else if (this->state & PROC_THROW_ERROR) {
        warn("Could not read [%s] because it hasn't been muted!", target == READ_STDOUT ? "stdout" : "stderr");
        Proc_Err(this, NULL);
    } else
        return NULL;
    
    u8* line = x_alloc(512 + 1);
    
    line[0] = ' ';
    
    if (reproc_read(this->proc, s, line, 512) >= 0)
        return (char*)line;
    
    return NULL;
}

char* Proc_Read(Proc* this, e_ProcRead target) {
    Memfile mem = Memfile_New();
    REPROC_STREAM s = -1;
    
    if (target == READ_STDOUT && this->state & PROC_MUTE_STDOUT)
        s = REPROC_STREAM_OUT;
    else if (target == READ_STDERR && this->state & PROC_MUTE_STDERR)
        s = REPROC_STREAM_ERR;
    else if (this->state & PROC_THROW_ERROR) {
        warn("Could not read [%s] because it hasn't been muted!", target == READ_STDOUT ? "stdout" : "stderr");
        Proc_Err(this, NULL);
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
            Memfile_Alloc(&mem, 4096);
        
        if (!Memfile_Write(&mem, buffer, readSize)) {
            warn("Failed to write buffer!");
            Proc_Err(this, NULL);
        }
    }
    
    if (mem.data)
        Memfile_Write(&mem, "\0", 1);
    
    this->msg = mem.data;
    
    return mem.data;
}

int Proc_Write(Proc* this, const char* fmt, ...) {
    va_list va;
    char* w;
    int r;
    int l;
    
    va_start(va, fmt);
    w = x_vfmt(fmt, va);
    l = strlen(w);
    r = reproc_write(this->proc, (u8*)w, l);
    
    if (this->state & PROC_THROW_ERROR) {
        if (r == REPROC_EPIPE)
            Proc_Err(this, "Pipe Error");
        else if (r == REPROC_EWOULDBLOCK)
            Proc_Err(this, "Pipe Block");
    }
    
    return r - l;
}

static int Proc_Free(Proc* this) {
    int signal = 0;
    
    if (this->state & PROC_SYSTEM_EXE)
        signal = this->signal;
    
    else {
        this->signal = signal = reproc_wait(this->proc, REPROC_INFINITE);
        reproc_destroy(this->proc);
    }
    
    if (signal && this->state & PROC_THROW_ERROR)
        Proc_Err(this, NULL);
    
    this->localStatus = PROC_FREE;
    _log("free");
    
    if (this->env) {
        for (int i = 0; i < 32; i++)
            vfree(this->env[i]);
        
        vfree(this->env);
    }
    
    for (int i = 0; i < this->numArg; i++)
        vfree(this->arg[i]);
    
    vfree(this->path, this->arg, this);
    
    return signal;
}

int Proc_Kill(Proc* this) {
    this->localStatus = PROC_KILL;
    
    if (this->state & PROC_SYSTEM_EXE) {
        this->signal = 1;
        int pthread_kill(pthread_t t, int sig);
        if (!pthread_kill(this->thd, 1))
            return Proc_Free(this);
        
        return 1;
    }
    reproc_terminate(this->proc);
    reproc_kill(this->proc);
    
    return Proc_Free(this);
    
}

int Proc_Join(Proc* this) {
    if (!this) return 0;
    this->localStatus = PROC_JOIN;
    
    if (this->state & PROC_SYSTEM_EXE) {
        pthread_join(this->thd, (void*)&this->signal);
        
        return Proc_Free(this);
    }
    
    return Proc_Free(this);
}
