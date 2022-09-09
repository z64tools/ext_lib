#include <ext_lib.h>

typedef enum {
    PROC_MUTE_STDOUT = 1 << 0,
    PROC_MUTE_STDERR = 1 << 1,
    PROC_MUTE_STDIN  = 1 << 3,
    PROC_MUTE        = PROC_MUTE_STDOUT | PROC_MUTE_STDERR | PROC_MUTE_STDIN,
    PROC_THROW_ERROR = 1 << 4,
    PROC_SYSTEM_EXE  = 1 << 5,
} StateProc;

typedef struct {
    const char** arg;
    u32       numArg;
    char*     path;
    void*     proc;
    StateProc state;
    int       localStatus;
    int       signal;
    Thread    thd;
} Proc;

Proc* Proc_New(char* fmt, ...);
void Proc_SetState(Proc* this, StateProc state);
int Proc_SetPath(Proc* this, const char* path);
int Proc_Exec(Proc* this);
int Proc_Kill(Proc* this);
int Proc_Join(Proc* this);