#include <ext_lib.h>
#include <sys/time.h>

typedef struct {
    struct timeval t;
    f32 ring[20];
    s8  k;
} ProfilerSlot;

struct {
    ProfilerSlot s[255];
} gProfilerCtx;

thread_local static struct timeval sTimeStart[255];

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

void time_start(u8 slot) {
    gettimeofday(&sTimeStart[slot], NULL);
}

f32 time_get(u8 slot) {
    struct timeval sTimeStop;
    
    gettimeofday(&sTimeStop, NULL);
    
    return (sTimeStop.tv_sec - sTimeStart[slot].tv_sec) + (f32)(sTimeStop.tv_usec - sTimeStart[slot].tv_usec) / 1000000;
}

f64 sys_ftime() {
    struct timeval sTime;
    
    gettimeofday(&sTime, NULL);
    
    return sTime.tv_sec + ((f64)sTime.tv_usec) / 1000000.0;
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

void profi_start(u8 s) {
    ProfilerSlot* p = &gProfilerCtx.s[s];
    
    gettimeofday(&p->t, 0);
}

void profi_stop(u8 s) {
    struct timeval t;
    ProfilerSlot* p = &gProfilerCtx.s[s];
    
    gettimeofday(&t, 0);
    
    p->ring[p->k] = t.tv_sec - p->t.tv_sec + (f32)(t.tv_usec - p->t.tv_usec) * 0.000001f;
}

f32 profi_get(u8 s) {
    ProfilerSlot* p = &gProfilerCtx.s[s];
    f32 sec = 0.0f;
    
    for (int i = 0; i < 20; i++)
        sec += p->ring[i];
    sec /= 20;
    p->k++;
    if (p->k >= 20)
        p->k = 0;
    
    return sec;
}

// # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

static struct timeval sProfiTime;

const_func profi_init() {
    gettimeofday(&sProfiTime, NULL);
}

void profilog(const char* msg) {
    struct timeval next;
    f32 time;
    
    gettimeofday(&next, NULL);
    
    time = (next.tv_sec - sProfiTime.tv_sec) + (next.tv_usec - sProfiTime.tv_usec) / 1000000.0;
    sProfiTime = next;
    
    printf("" PRNT_PRPL "-" PRNT_GRAY ": " PRNT_RSET "%-16s " PRNT_YELW "%.2f " PRNT_RSET "ms\n", msg,  time * 1000.0f);
}

void profilogdiv(const char* msg, f32 div) {
    struct timeval next;
    f32 time;
    
    gettimeofday(&next, NULL);
    time = (next.tv_sec - sProfiTime.tv_sec) + (next.tv_usec - sProfiTime.tv_usec) / 1000000.0;
    time /= div;
    sProfiTime = next;
    
    printf("" PRNT_PRPL "~" PRNT_GRAY ": " PRNT_RSET "%-16s " PRNT_YELW "%.2f " PRNT_RSET "ms\n", msg,  time * 1000.0f);
}
