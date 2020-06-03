#ifndef SEMAPHORE_H
#define SEMAPHORE_H
#include "spinlock.h"

typedef struct semaphore{
  int value;
  spinlock_t lock;
  //For debugging:
  const char *name;
}sem_t;

void sem_init (sem_t *sem, const char *name, int value);
void sem_wait (sem_t *sem);
void sem_signal (sem_t *sem);

#endif