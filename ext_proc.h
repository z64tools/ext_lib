#include <ext_lib.h>

typedef enum {
	PROC_MUTE_STDOUT = 1 << 0,
	PROC_MUTE_STDERR = 1 << 1,
	PROC_OPEN_STDIN  = 1 << 3,
	PROC_MUTE        = PROC_MUTE_STDOUT | PROC_MUTE_STDERR,
	
	PROC_THROW_ERROR = 1 << 30,
	PROC_SYSTEM_EXE  = 1 << 31,
} e_ProcState;

typedef struct {
	const char** arg;
	const char** env;
	u32         numArg;
	char*       msg;
	char*       path;
	void*       proc;
	e_ProcState state;
	int         localStatus;
	int         signal;
	thread_t    thd;
} Proc;

typedef enum {
	READ_STDOUT,
	READ_STDERR,
} e_ProcRead;

Proc* Proc_New(char* fmt, ...);
void Proc_AddArg(Proc* this, char* fmt, ...);
void Proc_SetState(Proc* this, e_ProcState state);
int Proc_SetPath(Proc* this, const char* path);
void Proc_SetEnv(Proc* this, const char* env);
void Proc_Info(Proc* this);
int Proc_Exec(Proc* this);
char* Proc_ReadLine(Proc* this, e_ProcRead target);
char* Proc_Read(Proc* this, e_ProcRead);
int Proc_Write(Proc* this, const char* fmt, ...);
int Proc_Kill(Proc* this);
int Proc_Join(Proc* this);

void Proc_AddEach(Proc* this, ...);

#ifndef __clang__
#define Proc_AddEach(instance, ...) \
		Proc_AddEach(instance, NARGS(__VA_ARGS__), __VA_ARGS__)
#endif