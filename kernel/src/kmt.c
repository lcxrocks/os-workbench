#include <common.h>
#include "spinlock.h"
#include "semaphore.h"
#include "kmt.h"

void kmt_init(){
    return ;
};

int  kcreate(task_t *task, const char *name, void (*entry)(void *arg), void *arg){
    return 0;
};

void kteardown(task_t *task){
    return ;
};


MODULE_DEF(kmt) = {
  .init  = kmt_init,
  .create = kcreate,
  .teardown  = kteardown,
  .spin_init = spin_init,
  .spin_lock = lock,
  .spin_unlock = unlock,
  .sem_init = sem_init, 
  .sem_wait = sem_wait,
  .sem_signal = sem_signal,
};