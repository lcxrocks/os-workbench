//#include <common.h>
#include "../include/common.h"
//#define current 
extern void os_on_irq(int seq, int event, handler_t handler);
extern void trap_handler_init();
int next_pid = 0;
spinlock_t task_lock;
task_t task_head;

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
    r_panic_on(lock->cpu != _cpu(), "wrong thd unlock!\n"); //holding(lock) check
    r_panic_on(lock->intr != _intr_read(), "sti() failed in kmt_unlock!\n");
}

void sem_init(sem_t *sem, const char *name, int value){
    sem->name = (char *) name;
    sem->value  = value;
    kmt_spin_init(&(sem->lock), name);
}

void sem_wait(sem_t *sem){
    kmt_lock(&sem->lock);
    int finish_waiting = 0;
    while(sem->value <= 0){
        finish_waiting = 1;
        //:cond_wait()
        current->stat = SLEEPING;
        //mark current as not runable.
        
        //_yield();
    }
    sem->value--;
    kmt_unlock(&sem->lock);
    while(finish_waiting){
        _yield();
    }
}

void sem_signal(sem_t *sem){
    kmt_lock(&sem->lock);
    sem->value++;
    //cond_signal; 
    _yield();
    kmt_unlock(&sem->lock);
}

_Context *kmt_context_save(_Event ev, _Context *ctx){
    return NULL;    
}

_Context *kmt_schedule(_Event ev, _Context *ctx){
    return NULL;
}

void kmt_init(){
    c_log(CYAN, "In kmt_init\n");
    kmt_spin_init(&task_lock, "task_lock");
    task_head.name = "TASK HEAD";
    task_head.entry = NULL;
    task_head.next = NULL;
    task_head.entry = NULL;
    task_head.pid = next_pid;
    panic_on(task_head.pid!=0, "task_head pid != 0.");
    os->on_irq(INT32_MIN, _EVENT_NULL, kmt_context_save);
    os->on_irq(INT32_MAX, _EVENT_NULL, kmt_schedule);
    return ;
}

int  kcreate(task_t *task, const char *name, void (*entry)(void *arg), void *arg){
    kmt_lock(&task_lock);
    task->pid = next_pid++;
    task->name = name;
    task->entry = entry;
    memset(task->stack, 0, sizeof(task->stack));
    _Area stack = {(void *)task->stack, (void *)task->stack+sizeof(task->stack)};
    task->context = _kcontext(stack, entry, arg);
    task->stat = EMBRYO;
    
    task_t *p = &task_head;
    while(p->next) p = p->next;
    p->next = task;
    
    kmt_unlock(&task_lock);
    return 0;
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