//#include <common.h>
#include "../include/common.h"
//#define current 
extern void os_on_irq(int seq, int event, handler_t handler);
extern void trap_handler_init();
int next_pid = 0;
spinlock_t task_lock;
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
    _intr_write(0);
    r_panic_on(holding(lock), "lock(%s) tried to acquire itself while holding.\n", lock->name);
    
    while(_atomic_xchg(&(lock->locked), 1) == 1) ;

    // Get the lock successfully.
    __sync_synchronize();

    lock->intr = i;
    lock->cpu = _cpu();
    r_panic_on(_intr_read() != 0, "cli() failed in kmt_lock!\n");
    r_panic_on(lock->locked != 1, "lock failed!\n");
}

void kmt_unlock(spinlock_t *lock){
    r_panic_on(!holding(lock), "lock(%s) tried to release itself without holding.\n", lock->name);
    lock->cpu = -1;

    __sync_synchronize();
    _atomic_xchg(&(lock->locked), 0);
    if(lock->intr) _intr_write(1);
    r_panic_on(lock->locked != 0, "unlock failed!\n");
    r_panic_on(lock->intr != _intr_read(), "sti() failed in kmt_unlock!\n");
}

void sem_init(sem_t *sem, const char *name, int value){
    sem->name = (char *) name;
    sem->value  = value;
    kmt_spin_init(&(sem->lock), name);
}

void sem_wait(sem_t *sem){
    kmt_lock(&sem->lock);
    while(sem->value <= 0){
        current->stat = SLEEPING;
        _yield();
    }
    sem->value--;
    kmt_unlock(&sem->lock);
}

void sem_signal(sem_t *sem){
    kmt_lock(&sem->lock);
    sem->value++;
    // find the first task sleeping on sem (basic implementation)
    task_t *p = &task_head;
    while(p->sem != sem) p= p->next;
    r_panic_on(p == NULL, "No task waiting on sem: %s\n", sem->name);
    r_panic_on(p->stat!=SLEEPING, "Task:[%s] is not sleeping.\n", p->name);
    p->stat = RUNNABLE;
    p->sem = NULL;
    _yield();
    kmt_unlock(&sem->lock);
}

void canary_init(canary_t *c) {
  for (int i = 0; i < N; i++) (*c)[i] = MAGIC; 
}

void canary_check(canary_t *c, const char *msg) {
  for (int i = 0; i < N; i++) r_panic_on((*c)[i] != MAGIC, msg);
}

void kstack_check(task_t *stk) {
  canary_check(&stk->__c1, "kernel stack overflow\n");
  canary_check(&stk->__c2, "kernel stack underflow\n");
}

_Context *kmt_context_save(_Event ev, _Context *ctx){
    // ctx should be in current's stack
    
    //r_panic_on(current == NULL, "No current task.\n");
    if(current != NULL){
        current->context = ctx;
        current->stat = SLEEPING;
    }
    else{
        idle->context = ctx;
        idle->stat = SLEEPING;
        idle->name = "os->run";
        idle->next = NULL;
    }
    c_log(BLUE, "IN handler kmt_context_save\n");
    return NULL;
}

_Context *kmt_schedule(_Event ev, _Context *ctx){
    _Context *ret = NULL;
    c_log(YELLOW, "in kmt schedule!\n");
    task_t *p = &task_head;
    bool flag = false;
    while(p->next){
        if(p->stat == RUNNABLE || p->stat == EMBRYO){
            flag = true;
            break;
        }
        p = p->next;
    }
    if(flag == true){ // found an excutable task
        current = p;
        current->stat = RUNNING;
        ret = current->context;  
    }
    else{ // return to idle process
        current = NULL;
        ret = idle->context;
    }
    r_panic_on(ret == NULL, "Schedule failed. No RUNNABLE TASK!\n");
    kstack_check(current);
    return ret;
}// return any task's _Context.

int kcreate(task_t *task, const char *name, void (*entry)(void *arg), void *arg){
    kmt_lock(&task_lock);
    task->pid = next_pid++;
    task->name = name;
    task->entry = entry;
    task->next = NULL;
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
    c_log(YELLOW, "task: %s created!\n", name);
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
    task_head.entry = NULL;
    task_head.pid = next_pid;
    task_head.stat = -1;
    panic_on(task_head.pid!=0, "task_head pid != 0.");
    os->on_irq(INT32_MIN, _EVENT_NULL, kmt_context_save);
    //...
    os->on_irq(INT32_MAX, _EVENT_NULL, kmt_schedule);
    return ;
}

void kteardown(task_t *task){
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