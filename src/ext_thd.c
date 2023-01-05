#include <ext_lib.h>
#undef ThdPool_Add

// # # # # # # # # # # # # # # # # # # # #
// # ThreadPool                          #
// # # # # # # # # # # # # # # # # # # # #

const char* gThdPool_ProgressMessage;
vbool gThreadMode;
mutex_t gThreadMutex;

typedef enum {
    T_IDLE,
    T_RUN,
    T_DONE,
} thd_state_t;

typedef struct thd_item_t {
    struct thd_item_t*   next;
    volatile thd_state_t state;
    thread_t thd;
    void (*function)(void*);
    void* arg;
    vu8   nid;
    vu8   numDep;
    vs32  dep[16];
} thd_item_t;

typedef struct {
    thd_item_t* head;
    vu32 num;
    vu16 dep[__UINT8_MAX__];
    struct {
        vbool mutex : 1;
        vbool on    : 1;
    };
} thd_pool_t;

static thd_pool_t* gTPool;

static mutex_t sMutex;

const_func __thdpool_init() {
    pthread_mutex_init(&sMutex, 0);
}

dest_func __thdpool_dest() {
    pthread_mutex_destroy(&sMutex);
}

void __impl_addToHead(thd_item_t* t) {
    if (t->nid)
        gTPool->dep[t->nid]++;
    
    Node_Add(gTPool->head, t);
    gTPool->num++;
}

void ThdPool_Add(void* function, void* arg, u32 n, ...) {
    thd_item_t* t = new(thd_item_t);
    va_list va;
    
    if (!gTPool) {
        gTPool = dfree(new(thd_pool_t));
        _assert(gTPool);
    }
    
    _assert(gTPool->on == false);
    
    t->function = function;
    t->arg = arg;
    
    va_start(va, n);
    for (int i = 0; i < n; i++) {
        int val = va_arg(va, int);
        
        if (val > 0)
            t->nid = val;
        
        if (val < 0)
            t->dep[t->numDep++] = -val;
    }
    va_end(va);
    
    pthread_mutex_lock(&sMutex);
    __impl_addToHead(t);
    pthread_mutex_unlock(&sMutex);
}

static void thdpool_exec_thread(thd_item_t* this) {
    this->function(this->arg);
    this->state = T_DONE;
}

static bool thdpool_deps(thd_item_t* t) {
    if (!t->numDep)
        return true;
    
    for (int i = 0; i < t->numDep; i++)
        if (gTPool->dep[t->dep[i]] != 0)
            return false;
    
    return true;
}

void ThdPool_Exec(u32 max) {
    u32 amount = gTPool->num;
    u32 prev = 1;
    u32 prog = 0;
    u32 cur = 0;
    thd_item_t* t;
    bool msg = gThdPool_ProgressMessage != NULL;
    
    max = clamp_min(max, 1);
    
    gTPool->on = true;
    
    _log("Num: %d", gTPool->num);
    _log("max: %d", max);
    
    while (gTPool->num) {
        if (msg && prev != prog)
            print_prog(gThdPool_ProgressMessage, prog + 1, amount);
        
        max = min(max, gTPool->num);
        
        if (cur < max) { /* Assign */
            t = gTPool->head;
            
            while (t) {
                if (t->state == T_IDLE)
                    if (thdpool_deps(t))
                        break;
                
                t = t->next;
            }
            
            if (t) {
                while (t->state != T_RUN)
                    t->state = T_RUN;
                
                if (max > 1) {
                    if (thd_create(&t->thd, thdpool_exec_thread, t))
                        print_error("thd_pool_t: Could not create thread");
                } else
                    thdpool_exec_thread(t);
                
                cur++;
            }
            
        }
        
        prev = prog;
        t = gTPool->head;
        while (t) {
            if (t->state == T_DONE) {
                if (max > 1)
                    thd_join(&t->thd);
                break;
            }
            
            t = t->next;
        }
        
        if (t) {
            if (t->nid)
                gTPool->dep[t->nid]--;
            
            Node_Kill(gTPool->head, t);
            
            cur--;
            gTPool->num--;
            prog++;
        }
    }
    
    gThdPool_ProgressMessage = NULL;
    gTPool->on = false;
}
