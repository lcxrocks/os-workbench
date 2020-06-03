#include <common.h>
#include "spinlock.h"

void spin_init(spinlock_t *mutex, const char *name){
  mutex->flag = 0;
  mutex->name = name;
}

void inline lock(spinlock_t *mutex){
  while(_atomic_xchg(&(mutex->flag), 1) == 1)
    ;
}

void inline unlock(spinlock_t *mutex){
  _atomic_xchg(&(mutex->flag), 0);
}

//TBA