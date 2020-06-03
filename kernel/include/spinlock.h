#ifndef SPINLOCK_H
#define SPINLOCK_H
typedef struct spinlock {
  intptr_t flag; // Is the lock held?
  // For debugging:
  const char *name;        // Name of lock.
  int ncpu;   // The cpu holding the lock.
}spinlock_t;

void spin_init(spinlock_t *mutex, const char *name);
void lock(spinlock_t *mutex);
void unlock(spinlock_t *mutex);

#endif