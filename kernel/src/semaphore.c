#include <common.h>
#include "spinlock.h"
#include "semaphore.h"

void sem_init (sem_t *sem, const char *name, int value){
    sem->value = value;
    sem->name = name;
    spin_init(&sem->lock, name);
    return;
}

void sem_wait (sem_t *sem){
    
    return ;
};

void sem_signal (sem_t *sem){
    return ;
};