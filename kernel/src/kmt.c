#include <common.h>
//#include <kernel.h>
//#include "../include/common.h"

void kmt_spin_init(spinlock_t *lock, const char* name){
   lock->name = (char *) name;
   lock->locked = 0;  
}

void kmt_lock(spinlock_t *lock){
    int i = _intr_read(); //cli();
    _intr_write(0);
    while(_atomic_xchg(&(lock->locked), 1) == 1) ;

    // Get the lock successfully.
    lock->intr = i;
    lock->cpu = _cpu();
    r_panic_on(_intr_read() != 0, "cli() failed in kmt_lock!\n");
    r_panic_on(lock->locked != 1, "lock failed!\n");
}

void kmt_unlock(spinlock_t *lock){
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

}

void sem_signal(sem_t *sem){

}

void kmt_init(){
    c_log(CYAN, "Welcome to L2 World!\n");
    return ;
}

int  kcreate(task_t *task, const char *name, void (*entry)(void *arg), void *arg){
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