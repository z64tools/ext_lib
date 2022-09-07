#include <ext_lib.h>

typedef enum {
	PROC_MUTE_STDOUT = 1 << 0,
	PROC_MUTE_STDERR = 1 << 1,
	PROC_MUTE        = PROC_MUTE_STDOUT | PROC_MUTE_STDERR,
	PROC_THROW_ERROR = 1 << 2,
} StateProc;

typedef struct {
	const char** arg;
	u32       numArg;
	void*     proc;
	int       status;
	StateProc state;
} Proc;

Proc* Proc_New(char* arg);
void Proc_SetState(Proc* this, StateProc state);
void Proc_ClearState(Proc* this, StateProc state);
int Proc_Exec(Proc* this);

int Proc_Kill(Proc* this);
int Proc_Join(Proc* this);

void Proc_Error(Proc* this);