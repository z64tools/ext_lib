#ifndef EXT_EXCEPTION_H
#define EXT_EXCEPTION_H

#include <ext_type.h>
#include <setjmp.h>

typedef struct {
	jmp_buf jump;
	int     result;
	int     inscope;
} __exception_t;

__exception_t* __exception_push();
void __exception_pop();
int __exceptioin_result();

void throw(int id);

//crustify
#define try \
    { \
		if (!setjmp(__exception_push()->jump))
#define catch(e) \
    } \
    __exception_pop(); \
    e = __exceptioin_result(); \
	if (__exceptioin_result())
//uncrustify

#endif
