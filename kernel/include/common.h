#ifndef COMMON_H
#define COMMON_H
#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>

// xv6 checking double lock;
// int holding (struct spinlock *lock){
//   int r;
//   pushcli();
//   r  = lock->locked && lock->cpu == mycpu();
//   popcli();
//   return r;
// }

#endif