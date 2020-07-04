//#include <common.h>
#include "../include/common.h"
//#define current 
extern void os_on_irq(int seq, int event, handler_t handler);
extern void trap_handler_init();
extern trap_handler_t head;
int next_pid = 0;
spinlock_t task_lock;
spinlock_t info_lock;
task_t task_head;

// void stop_intr(int *i){
//     i = _intr_read();
//     _intr_write(0);
// }

// void restart_intr(int *i){
//     if(i == 1) _intr_write(1);
// }

int holding(struct spinlock *lock){
  int r = 0;
  int i = _intr_read();
  _intr_write(0);
  r  = lock->locked && lock->cpu == _cpu();
  if(i == 1) _intr_write(1);
  return r;
}// xv6 checking double lock;

void kmt_spin_init(spinlock_t *lock, const char* name){
   lock->name = (char *) name;
   lock->locked = 0;  
}

void kmt_lock(spinlock_t *lock){
    int i = _intr_read(); //cli();
    //printf("locking lock[%s]\n", lock->name);
    _intr_write(0);
    r_panic_on(holding(lock), "lock(%s) tried to acquire itself while holding.\n", lock->name);
    while(_atomic_xchg(&(lock->locked), 1)) ;
    // Get the lock successfully.
    __sync_synchronize();
    lock->intr = i;
    lock->cpu = _cpu();
    r_panic_on(_intr_read() != 0, "cli() failed in kmt_lock!\n");
    r_panic_on(lock->locked != 1, "lock failed!\n");
}

void kmt_unlock(spinlock_t *lock){
    //printf("unlocking lock[%s]\n", lock->name);
    r_panic_on(!holding(lock), "lock(%s) tried to release itself without holding.\n", lock->name);
    lock->cpu = -1;

    //__sync_synchronize();
    int i = lock->intr;
    _atomic_xchg(&(lock->locked), 0);
    if(i) _intr_write(1);
    //r_panic_on(lock->locked != 0, "lock[%s] unlock failed!\n", lock->name);
    //r_panic_on(lock->intr != _intr_read(), "sti() failed in kmt_unlock!\n");
}

void sem_init(sem_t *sem, const char *name, int value){
    sem->name = (char *) name;
    sem->value  = value;
    kmt_spin_init(&(sem->lock), name);
}

void sem_wait(sem_t *sem){
    kmt_lock(&sem->lock);
    bool flag =false;
    //printf("sem[%s] value: %d\n", sem->name, sem->value);
    if(sem->value <= 0){
        flag = true;
        current->sem = sem;
        current->stat = SLEEPING;
        //printf("[%s] now sleeping on sem[%s].\n", current->name, sem->name);   
    }
    sem->value--;
    kmt_unlock(&sem->lock);
    if(flag) _yield();
}

void sem_signal(sem_t *sem){
    kmt_lock(&sem->lock);
    sem->value++;
    task_t *p = task_head.next;
    while(p) {
        if(p->sem == sem){
            //printf("task[%s] now runnable.\n", p->name);
            p->stat = RUNNABLE;
            p->sem = NULL;
            break;
        }
       p = p->next;
    }
    //r_panic_on(p == NULL, "No task waiting on sem: %s\n", sem->name);
    //r_panic_on(p->stat!=SLEEPING, "Task:[%s] is not sleeping.\n", p->name);
    kmt_unlock(&sem->lock);
    //_yield();
}

void canary_init(canary_t *c) {
  for (int i = 0; i < N; i++) (*c)[i] = MAGIC; 
}

void canary_check(canary_t *c, const char *msg, const char *task_name) {
  for (int i = 0; i < N; i++) r_panic_on((*c)[i] != MAGIC, msg, task_name);
}

void kstack_check(task_t *stk) {
  canary_check(&stk->__c1, "task[%s] kernel stack overflow\n", stk->name);
  canary_check(&stk->__c2, "task[%s] kernel stack underflow\n", stk->name);
}

_Context *kmt_context_save(_Event ev, _Context *ctx){
    // ctx should be in current's stack
    //r_panic_on(current == NULL, "No current task.\n");
    if(current != NULL){
        current->context = ctx;
        if(current->stat == RUNNING) current->stat = RUNNABLE;
    }// current == NULL ----> idle->stat = RUNNING.
    else{
        panic_on((idle->stat!=RUNNING && idle->stat!=EMBRYO), "This cpu has nothing to do.\n");
        idle->context = ctx;
        if(idle->stat == EMBRYO) idle->stat = RUNNABLE;
    }
    c_log(BLUE, "IN handler kmt_context_save\n");
    return NULL;
}

_Context *kmt_schedule(_Event ev, _Context *ctx){
    _Context *next = NULL;
    c_log(BLUE, "IN handler kmt_schedule!\n");
    task_t *t = task_head.next;
    //printf("--------------------------------------\n");
    bool sleep = false;
    while(t){
        if(t->stat == SLEEPING) sleep = true;
        else{
           sleep = false; 
           break;
        }
        //printf("[%s]: %d\n", t->name, t->stat);
        t = t->next;
    }
    panic_on(sleep == true, "All task sleeping.\n");
    //printf("======================================\n");
    task_t *p = task_head.next;
    while(p){
        if(p->stat == EMBRYO || p->stat == RUNNABLE){
            if(p->cpu == _cpu()){        
                if(p->on_time >= MAX_ONTIME){
                    p->on_time = 0;
                    continue; 
                }
                next = p->context;
                break;
            }
        }
        p = p->next;
    }
    if(next){
        r_panic_on(p->context!=next, "p->context!=next\n");
        current = p;
        current->on_time++;
        kstack_check(current);
        current->stat = RUNNING;
        current->cpu = (current->cpu + 1)%_ncpu(); // Round-robin to next cpu.
        task_t *rep = pmm->alloc(sizeof(task_t));
        memcpy(rep->stack, current->stack, sizeof(current->stack)); 
        _Context *ctx = pmm->alloc(sizeof(_Context));
        memcpy(ctx, next, sizeof(_Context));
        ctx->rsp = (uint64_t)rep + (p->context->rsp - (uint64_t)current->stack);
        next = ctx;
    }
    else{
        current = IDLE;
        next = idle->context;
        idle->stat = RUNNING;
        kstack_check(idle);
    }
    r_panic_on(next == NULL, "Schedule failed. No RUNNABLE TASK!\n");
    c_log(GREEN, "Returning task:[%s]\n", current==NULL ? "idle":current->name);
    return next;
}// return any task's _Context.

int init_cpu = 0;

int kcreate(task_t *task, const char *name, void (*entry)(void *arg), void *arg){
    kmt_lock(&task_lock);
    task->pid = ++next_pid;
    task->name = name;
    task->entry = entry;
    task->next = NULL;
    task->on_time = 0;
    task->cpu = (init_cpu++)%_ncpu(); // can be scheduled to any cpu at the beginning.
    memset(task->stack, 0, sizeof(task->stack));
    canary_init(&task->__c1);
    canary_init(&task->__c2);
    _Area stack = {(void *)task->stack, (void *)task->stack+sizeof(task->stack)};
    task->context = _kcontext(stack, entry, arg);
    task->stat = EMBRYO;
    
    task_t *p = &task_head;
    while(p->next) {
       p = p->next; 
    }
    p->next = task;
    c_log(YELLOW, "task[%d]: %s created, init_cpu:(%d)!\n", task->pid, name, task->cpu);
    kmt_unlock(&task_lock);
    return 0;
}

void kmt_init(){
    c_log(CYAN, "In kmt_init\n");
    kmt_spin_init(&task_lock, "task_lock");
    r_panic_on(sizeof(task_t)!=4096, "Wrong Task Size. Re-check task header size.\n");
    task_head.name = "TASK HEAD";
    task_head.entry = NULL;
    task_head.next = NULL;
    task_head.on_time = 0;
    task_head.entry = NULL;
    task_head.pid = next_pid;
    task_head.stat = -1;
    for (int i = 0; i < _ncpu(); i++)
    {
        cpu_info[i].cpu_idle = pmm->alloc(sizeof(task_t));
        cpu_info[i].cpu_idle->name = "os->run";
        cpu_info[i].cpu_idle->stat = EMBRYO;
        cpu_info[i].cpu_idle->cpu  = i;
        cpu_info[i].cpu_idle->next = NULL;
        memset(cpu_info[i].cpu_idle->stack, 0, sizeof(cpu_info[i].cpu_idle->stack));
        canary_init(&cpu_info[i].cpu_idle->__c1);
        canary_init(&cpu_info[i].cpu_idle->__c2);
        _Area stack = {(void *)cpu_info[i].cpu_idle->stack, (void *)cpu_info[i].cpu_idle->stack+sizeof(cpu_info[i].cpu_idle->stack)};
        cpu_info[i].cpu_idle->context = _kcontext(stack, NULL, NULL);
    }
    
    panic_on(task_head.pid!=0, "task_head pid != 0.");
    os->on_irq(INT32_MIN, _EVENT_NULL, kmt_context_save);
    //...
    os->on_irq(INT32_MAX, _EVENT_NULL, kmt_schedule);
    return ;
}

void kteardown(task_t *task){
    kmt_lock(&task_lock);
    task_t *p = task_head.next;
    while(p->next!=task) p = p->next;
    p->next = task->next;
    pmm->free(task);
    kmt_unlock(&task_lock);
    return ;
}


MODULE_DEF(kmt) = {
  .init  = kmt_init,
  .create = kcreate,
  .teardown  = kteardown,
  .spin_init = kmt_spin_init,
  .spin_lock = kmt_lock,
  .spin_unlock = kmt_unlock,
  .sem_init = sem_init, 
  .sem_wait = sem_wait,
  .sem_signal = sem_signal,
};