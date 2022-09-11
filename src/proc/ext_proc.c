#include <ext_proc.h>
#include <reproc/reproc.h>

enum ProcState {
    PROC_NEW,
    PROC_EXEC,
    PROC_KILL,
    PROC_JOIN,
    PROC_FREE,
};

static Size ArgToken_Size(const char* s) {
    Size sz = 0;
    bool string = *s == '"';
    
    if (string) {
        sz = 2;
        while (!(s[sz - 1] == '"' && s[sz - 2] != '\\') && s[sz] != '\0')
            sz++;
        
        return sz;
    } else {
        while (!(s[sz] == ' ' && s[sz - 1] != '\\') && s[sz] != '\0' && s[sz] != '&')
            sz++;
        
        return sz;
    }
}

static char* ArgToken_Next(const char* s) {
    s += ArgToken_Size(s);
    if (*s == '\0') return NULL;
    s += strspn(s, " \t");
    if (*s == '&') return NULL;
    
    return (char*)s;
}

static char* ArgToken_Copy(const char* s) {
    Size sz = ArgToken_Size(s);
    
    if (!sz) return NULL;
    
    return strndup(s, sz);
}

Proc* Proc_New(char* fmt, ...) {
    char buffer[8192];
    Proc* this = New(Proc);
    char* tok;
    char* args[512];
    va_list va;
    
    va_start(va, fmt);
    vsnprintf(buffer, 8192, fmt, va);
    va_end(va);
    
    tok = buffer;
    
    Assert(this != NULL);
    Assert(tok != NULL);
    Assert((this->proc = reproc_new()) != NULL);
    
    this->localStatus = PROC_NEW;
    
    while (tok) {
        args[this->numArg++] = ArgToken_Copy(tok);
        tok = ArgToken_Next(tok);
    }
    
    this->arg = Calloc(sizeof(char*) * (this->numArg + 1));
    
    for (s32 i = 0; i < this->numArg; i++)
        this->arg[i] = args[i];
    
    return this;
}

void Proc_AddArg(Proc* this, char* fmt, ...) {
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
        args[numArg++] = ArgToken_Copy(tok);
        tok = ArgToken_Next(tok);
    }
    
    this->arg = Realloc(this->arg, sizeof(char*) * (this->numArg + numArg + 1));
    
    for (s32 i = 0; i <= numArg; i++)
        this->arg[this->numArg + i] = args[i];
    
    this->numArg += numArg;
}

void Proc_SetState(Proc* this, StateProc state) {
    this->state |= state;
}

int Proc_SetPath(Proc* this, const char* path) {
    if (Sys_IsDir(path))
        return (this->path = strdup(path)) ? 0 : 1;
    
    return 1;
}

void Proc_SetEnv(Proc* this, const char* env) {
    if (!this->env) this->env = Calloc(sizeof(char*) * 32);
    
    for (s32 i = 0; i < 32; i++) {
        if (this->env[i] != NULL)
            continue;
        
        this->env[i] = StrDup(env);
        break;
    }
}

static s32 Proc_SystemThread(Proc* this) {
    char* args = StrCatArg(this->arg, ' ');
    s32 sys = system(args);
    
    Free(args);
    
    return sys;
}

static void Proc_Error(Proc* this) {
    char* msg = "Unknown";
    
    Log("ProcError");
    
    Mutex_Enable();
    Mutex_Lock();
    
    Sys_Sleep(0.5);
    
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
            msg = "Free";
            break;
    }
    
    printf_warning("Proc Error: %s", msg);
    
    for (s32 i = 0; i < this->numArg; i++)
        fprintf(stderr, "" PRNT_GRAY "%-6d" PRNT_RSET "%s\n", i, this->arg[i]);
    
    if (this->env) {
        printf_warning("Proc Env:");
        for (s32 i = 0; this->env[i] != NULL; i++)
            fprintf(stderr, "" PRNT_GRAY "%-6d" PRNT_RSET "%s\n", i, this->env[i]);
    }
    
    printf_warning("Proc Path:\n      %s", this->path ? this->path : Sys_WorkDir());
    
    #define fprintf_proc_enum(enum) fprintf(stderr, "" PRNT_GRAY "%-20s" PRNT_RSET "%s\n", #enum, this->state & enum ? "" PRNT_BLUE "true" : "" PRNT_REDD "false");
    printf_warning("Proc State:");
    fprintf_proc_enum(PROC_MUTE_STDOUT);
    fprintf_proc_enum(PROC_MUTE_STDERR);
    fprintf_proc_enum(PROC_MUTE_STDIN);
    fprintf_proc_enum(PROC_READ_STDOUT);
    fprintf_proc_enum(PROC_READ_STDERR);
    fprintf_proc_enum(PROC_WRITE_STDIN);
    fprintf_proc_enum(PROC_THROW_ERROR);
    fprintf_proc_enum(PROC_SYSTEM_EXE);
    
    printf_error("Proc Signal: %d", this->signal);
    
    Mutex_Unlock();
}

int Proc_Exec(Proc* this) {
    bool exec;
    reproc_options opt = {
        .redirect.out.type = this->state & PROC_MUTE_STDOUT ? REPROC_REDIRECT_DISCARD : this->state & PROC_READ_STDOUT ? REPROC_REDIRECT_PIPE : REPROC_REDIRECT_PARENT,
        .redirect.err.type = this->state & PROC_MUTE_STDERR ? REPROC_REDIRECT_DISCARD : this->state & PROC_READ_STDERR ? REPROC_REDIRECT_PIPE : REPROC_REDIRECT_PARENT,
        .redirect.in.type  = this->state & PROC_MUTE_STDIN ? REPROC_REDIRECT_DISCARD : this->state & PROC_WRITE_STDIN ? REPROC_REDIRECT_PIPE : REPROC_REDIRECT_PARENT,
        .working_directory = this->path,
        .env.extra         = this->env,
        .env.behavior      = this->env ? REPROC_ENV_EMPTY : 0,
    };
    
    this->localStatus = PROC_EXEC;
    
    if (this->state & PROC_SYSTEM_EXE) {
        Thread_Create(&this->thd, Proc_SystemThread, this);
        
        return 0;
    }
    
    exec = reproc_start(this->proc, this->arg, opt) < 0;
    
    if (exec && this->state & PROC_THROW_ERROR)
        Proc_Error(this);
    
    return exec;
}

void Proc_Read(Proc* this, int (*callback)(void*, const char*), void* ctx) {
    char buffer[4096];
    char* data = NULL;
    Size dsize = 0;
    REPROC_STREAM s = -1;
    
    if (this->state & PROC_READ_STDOUT)
        s = REPROC_STREAM_OUT;
    else if (this->state & PROC_READ_STDERR)
        s = REPROC_STREAM_ERR;
    else if (this->state & PROC_THROW_ERROR) {
        printf_warning("No Stream Marked for Reading!");
        Proc_Error(this);
    } else
        return;
    
    while (true) {
        bool null = data == NULL;
        Size sz = reproc_read(this->proc, s, (u8*)buffer, sizeof(buffer));
        
        if (sz < 0)
            break;
        
        data = Realloc(data, dsize);
        Assert(data != NULL);
        if (null) data[0] = '\0';
        dsize += 4096 + 1;
        strcat(data, buffer);
    }
    
    if (callback)
        if (callback(ctx, data))
            (void)0;
    
    Free(data);
}

static int Proc_Free(Proc* this) {
    int signal = 0;
    
    if (this->state & PROC_SYSTEM_EXE) {
        signal = this->signal;
    } else {
        this->signal = signal = reproc_wait(this->proc, REPROC_INFINITE);
        reproc_destroy(this->proc);
    }
    
    if (signal && this->state & PROC_THROW_ERROR)
        Proc_Error(this);
    
    this->localStatus = PROC_FREE;
    Log("Free");
    
    if (this->env) {
        for (s32 i = 0; i < 32; i++)
            Free(this->env[i]);
        Free(this->env);
    }
    
    for (s32 i = 0; i < this->numArg; i++)
        Free(this->arg[i]);
    
    Free(this->path);
    Free(this->arg);
    Free(this);
    
    return signal;
}

int Proc_Kill(Proc* this) {
    this->localStatus = PROC_KILL;
    
    if (this->state & PROC_SYSTEM_EXE) {
        this->signal = 1;
        
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
