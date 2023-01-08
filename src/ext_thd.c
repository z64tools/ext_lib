#include <ext_lib.h>
#undef threadpool_setdep

// # # # # # # # # # # # # # # # # # # # #
// # ThreadPool                          #
// # # # # # # # # # # # # # # # # # # # #

const char* gParallel_ProgMsg;
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
    vu8   dep_num;
    vs16  nid;
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

static thd_pool_t* sThdPool;

static mutex_t sMutex;

const_func Parallel_Init() {
    pthread_mutex_init(&sMutex, 0);
    
    sThdPool = new(thd_pool_t);
}

dest_func Parallel_Dest() {
    pthread_mutex_destroy(&sMutex);
    
    vfree(sThdPool);
}

static void Parallel_AddToHead(thd_item_t* t) {
    Node_Add(sThdPool->head, t);
    sThdPool->num++;
}

static void Parallel_AddToDepList(thd_item_t* t) {
    sThdPool->dep[t->nid]++;
}

void* Parallel_Add(void* function, void* arg) {
    thd_item_t* t = new(thd_item_t);
    
    _assert(sThdPool->on == false);
    
    t->nid = -1;
    t->function = function;
    t->arg = arg;
    
    pthread_mutex_lock(&sMutex);
    Parallel_AddToHead(t);
    pthread_mutex_unlock(&sMutex);
    
    return t;
}

void Parallel_SetID(void* __this, int id) {
    thd_item_t* t = __this;
    
    _assert(id >= 0);
    t->nid = id;
    
    pthread_mutex_lock(&sMutex);
    Parallel_AddToDepList(t);
    pthread_mutex_unlock(&sMutex);
}

void Parallel_SetDepID(void* __this, int id) {
    thd_item_t* t = __this;
    
    _assert(t->nid >= 0);
    t->dep[t->dep_num++];
}

static void Parallel_ExeThd(thd_item_t* this) {
    this->function(this->arg);
    this->state = T_DONE;
}

static bool Parallel_ChkDeps(thd_item_t* t) {
    if (!t->dep_num)
        return true;
    
    for (int i = 0; i < t->dep_num; i++)
        if (sThdPool->dep[t->dep[i]] != 0)
            return false;
    
    return true;
}

void Parallel_Exec(u32 max) {
    u32 amount = sThdPool->num;
    u32 prev = 1;
    u32 prog = 0;
    u32 cur = 0;
    thd_item_t* t;
    bool msg = gParallel_ProgMsg != NULL;
    
    max = clamp_min(max, 1);
    
    sThdPool->on = true;
    
    _log("Num: %d", sThdPool->num);
    _log("max: %d", max);
    
    while (sThdPool->num) {
        if (msg && prev != prog)
            info_prog(gParallel_ProgMsg, prog + 1, amount);
        
        max = Min(max, sThdPool->num);
        
        if (cur < max) { /* Assign */
            t = sThdPool->head;
            
            while (t) {
                if (t->state == T_IDLE)
                    if (Parallel_ChkDeps(t))
                        break;
                
                t = t->next;
            }
            
            if (t) {
                while (t->state != T_RUN)
                    t->state = T_RUN;
                
                if (max > 1) {
                    if (thd_create(&t->thd, Parallel_ExeThd, t))
                        errr("thd_pool_t: Could not create thread");
                } else
                    Parallel_ExeThd(t);
                
                cur++;
            }
            
        }
        
        prev = prog;
        t = sThdPool->head;
        while (t) {
            if (t->state == T_DONE) {
                if (max > 1)
                    thd_join(&t->thd);
                break;
            }
            
            t = t->next;
        }
        
        if (t) {
            if (t->nid >= 0)
                sThdPool->dep[t->nid]--;
            
            Node_Kill(sThdPool->head, t);
            
            cur--;
            sThdPool->num--;
            prog++;
        }
    }
    
    gParallel_ProgMsg = NULL;
    sThdPool->on = false;
}
