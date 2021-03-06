//#include <common.h>
#include "../include/common.h"
//#define current_task 
extern void os_on_irq(int seq, int event, handler_t handler);
extern void trap_handler_init();
extern trap_handler_t head;
int next_pid = 0;
spinlock_t task_lock;
spinlock_t info_lock;
task_t task_head;

int holding(struct spinlock *lock){
  int r = 0;
  int i = _intr_read();
  _intr_write(0);
  r  = (lock->locked) && (lock->cpu == _cpu());
  if(i == 1) _intr_write(1);
  return r;
}// xv6 checking double lock;

void kmt_spin_init(spinlock_t *lock, const char* name){
   lock->name = (char *) name;
   lock->locked = 0;  
   lock->intr = 0;
   lock->cpu = -1;
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
    c_log(WHITE, "sem[%s] value: %d\n", sem->name, sem->value);
    if(sem->value <= 0){
        flag = true;
        current_task->sem = sem;
        current_task->stat = SLEEPING;
        //printf("[%s] now sleeping on sem[%s].\n", current_task->name, sem->name);   
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
            //p->on_time = -1; // immediately.
            p->on_time = 0;
            p->stat = RUNNABLE;
            p->sem = NULL;
            break;
        }
       p = p->next;
    }
    //r_panic_on(p == NULL, "No task waiting on sem: %s\n", sem->name);
    //r_panic_on(p->stat!=SLEEPING, "Task:[%s] is not sleeping.\n", p->name);
    kmt_unlock(&sem->lock);
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
    c_log(BLUE, "IN handler kmt_context_save\n");
    if(current_task != NULL){
        current_task->context = ctx;
        if(current_task->stat == RUNNING) current_task->stat = RUNNABLE;
    }// current_task == NULL ----> idle->stat = RUNNING.
    else{
        panic_on((idle->stat!=RUNNING && idle->stat!=EMBRYO), "This cpu has nothing to do.\n");
        idle->context = ctx;
        if(idle->stat == EMBRYO) idle->stat = RUNNABLE;
    }
    return NULL;
}

_Context *kmt_schedule(_Event ev, _Context *ctx){
    _Context *next = NULL;
    c_log(BLUE, "IN handler kmt_schedule!\n");
    // task_t *t = task_head.next;
    // c_log(WHITE, "--------------------------------------\n");
    // bool sleep = false;
    // while(t){
    //     if(t->stat == SLEEPING) sleep = true;
    //     else{
    //        sleep = false; 
    //        break;
    //     }
    //     c_log(WHITE, "[%s]: %d\n", t->name, t->stat);
    //     t = t->next;
    // }
    // panic_on(sleep == true, "All task sleeping.\n");
    // c_log(WHITE, "======================================\n");
    task_t *p = task_head.next;
    if(_ncpu() != 2){
        while(p){
            if(p->cpu != _cpu()){
                p = p->next;
                continue;
            }
            if(p->stat == ZOMBIE){
                p->stat = RUNNABLE;
                p = p->next;
                continue;
            }
            if(p->stat == EMBRYO || p->stat == RUNNABLE){
                if(uptime() - p->last_time <= MIN_LASTTIME){
                    p = p->next;
                    continue;
                }
                if(p->on_time >= MAX_ONTIME){
                    p->on_time--;
                    p = p->next;
                    continue;
                }
                next = p->context;
                break;
            }
            if(p->stat == RUNNING){
                panic("shouldn't be running");
            }
            p = p->next;
        } 
    }
    if(_ncpu() == 2){
        while(p){
        if(p->stat == EMBRYO || p->stat == RUNNABLE){
            if(p->cpu == _cpu()){
                next = p->context;
                break;
                }
            }
        p = p->next;
        }
    }
    if(next != NULL){
        r_panic_on(p->context!=next, "p->context!=next\n");
        current_task = p;
        current_task->on_time++;
        kstack_check(current_task);
        current_task->stat = ZOMBIE;
        if(_ncpu()==2 ) current_task->stat = RUNNING;
        current_task->last_time = uptime();
        current_task->cpu = (current_task->cpu + 1)%_ncpu(); // Round-robin to next cpu.
    }
    else{
        current_task = IDLE;
        next = idle->context;
        idle->stat = RUNNING;
        kstack_check(idle);
        if(_ncpu()==2){
            task_t *tmp = task_head.next;
            while(tmp){
                if(p->cpu == _cpu()){
                        p->on_time = 0;
                }
                tmp = tmp->next;
            }
        }
    }
    r_panic_on(next == NULL, "Schedule failed. No RUNNABLE TASK!\n");
    c_log(GREEN, "Returning task:[%s]\n", current_task==IDLE ? "idle":current_task->name);
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
    task->sem = NULL;
    task->last_time = 0;
    task->cpu = (init_cpu++)%_ncpu();
    task->ord = cpu_info[task->cpu].nr_task;
    cpu_info[task->cpu].nr_task++; 
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
    task_head.last_time =0;
    task_head.entry = NULL;
    task_head.pid = next_pid;
    task_head.stat = -1;
    for (int i = 0; i < _ncpu(); i++)
    {
        cpu_info[i].nr_task = 0;
        cpu_info[i].cpu_idle = pmm->alloc(sizeof(task_t));
        cpu_info[i].cpu_idle->name = "os->run";
        cpu_info[i].cpu_idle->stat = EMBRYO;
        cpu_info[i].cpu_idle->cpu  = i;
        cpu_info[i].cpu_idle->next = NULL;
        memset(cpu_info[i].cpu_idle->stack, 0, sizeof(cpu_info[i].cpu_idle->stack));
        canary_init(&cpu_info[i].cpu_idle->__c1);
        canary_init(&cpu_info[i].cpu_idle->__c2);
        kstack_check(cpu_info[i].cpu_idle);
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