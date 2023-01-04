#include <ext_lib.h>

typedef enum {
    PROC_MUTE_STDOUT = 1 << 0,
    PROC_MUTE_STDERR = 1 << 1,
    PROC_MUTE_STDIN  = 1 << 3,
    PROC_MUTE        = PROC_MUTE_STDOUT | PROC_MUTE_STDERR | PROC_MUTE_STDIN,
    
    PROC_WRITE_STDIN = 1 << 6,
    
    PROC_THROW_ERROR = 1 << 30,
    PROC_SYSTEM_EXE  = 1 << 31,
} proc_state_t;

typedef struct {
    const char** arg;
    const char** env;
    u32   numArg;
    char* msg;
    char* path;
    void* proc;
    proc_state_t state;
    int      localStatus;
    int      signal;
    thread_t thd;
} proc_t;

typedef enum {
    READ_STDOUT,
    READ_STDERR,
} proc_read_target_t;

proc_t* proc_new(char* fmt, ...);
void proc_add_arg(proc_t* this, char* fmt, ...);
void proc_set_state(proc_t* this, proc_state_t state);
int proc_set_path(proc_t* this, const char* path);
void proc_set_env(proc_t* this, const char* env);
int proc_exec(proc_t* this);
char* proc_read(proc_t* this, proc_read_target_t);
int proc_kill(proc_t* this);
int proc_join(proc_t* this);