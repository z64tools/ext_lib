#include <ext_exception.h>
#include <string.h>

thread_local int sIndex;
thread_local __exception_t sStack[512];

__exception_t* __exception_push() {
	memset(sStack + sIndex, 0, sizeof(__exception_t));
	
	sStack[sIndex].inscope = true;
	
	return sStack + sIndex++;
}

void __exception_pop() {
	sIndex--;
	sStack[sIndex].inscope = false;
}

int __exceptioin_result() {
	return sStack[sIndex].result;
}

void throw(int id) {
	if (sIndex > 0) {
		if (sStack[sIndex - 1].inscope) {
			sStack[sIndex - 1].result = id;
			longjmp(sStack[sIndex - 1].jump, 1);
		}
	}
}
