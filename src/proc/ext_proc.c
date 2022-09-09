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
    Proc* this = New(Proc);
    char* arg;
    char* tok;
    char* args[512];
    va_list va;
    
    va_start(va, fmt);
    vasprintf(&arg, fmt, va);
    va_end(va);
    
    tok = arg;
    
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
    
    Free(arg);
    
    return this;
}

void Proc_SetState(Proc* this, StateProc state) {
    this->state = state;
}

int Proc_SetPath(Proc* this, const char* path) {
    if (Sys_IsDir(path))
        return (this->path = strdup(path)) ? 0 : 1;
    
    return 1;
}

static s32 Proc_SystemThread(Proc* this) {
    char* args = StrCatArg(this->arg, ' ');
    s32 sys = system(args);
    
    Free(args);
    
    return sys;
}

static void Proc_Error(Proc* this) {
    printf("\a");
    switch (this->localStatus) {
        case PROC_NEW:
            printf_warning("Proc New:");
            break;
        case PROC_EXEC:
            printf_warning("Proc Exec:");
            break;
        case PROC_KILL:
            printf_warning("Proc Kill:");
            break;
        case PROC_JOIN:
            printf_warning("Proc Close:");
            break;
        case PROC_FREE:
            printf_warning("Proc Free:");
            break;
    }
    
    for (s32 i = 0; i < this->numArg; i++)
        printf_warning("" PRNT_GRAY "%-6d" PRNT_YELW "%s", i, this->arg[i]);
    
    exit(1);
}

int Proc_Exec(Proc* this) {
    bool exec;
    reproc_options opt = {
        .redirect.out.type = this->state & PROC_MUTE_STDOUT ? REPROC_REDIRECT_DISCARD : REPROC_REDIRECT_PARENT,
        .redirect.err.type = this->state & PROC_MUTE_STDERR ? REPROC_REDIRECT_DISCARD : REPROC_REDIRECT_PARENT,
        .redirect.in.type  = this->state & PROC_MUTE_STDIN ? REPROC_REDIRECT_DISCARD : REPROC_REDIRECT_PARENT,
        .working_directory = this->path,
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

static int Proc_Free(Proc* this) {
    int signal;
    
    this->localStatus = PROC_FREE;
    
    if (this->state & PROC_SYSTEM_EXE) {
        signal = this->signal;
    } else {
        Log("Wait");
        signal = reproc_wait(this->proc, REPROC_INFINITE);
        Log("Destroy");
        reproc_destroy(this->proc);
    }
    
    Log("Free");
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
