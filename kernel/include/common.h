#ifndef COMMON_H
#define COMMON_H
//#define DEBUG
#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>

#define RED 31
#define GREEN 32
#define YELLOW 33
#define BLUE 34
#define PURPLE 35
#define CYAN 36

#define r_panic_on(cond, s) \
    c_panic_on(RED, cond, s);

#define c_panic_on(color, cond, s) \
do{ \
    if(cond) {\
        printf("\033[36m[cpu(%d)]:\033[0m", _cpu());\
        printf("\033[%dm", color); \
        panic_on(cond, s); \
        printf("\033[0m"); \
    }\
}while(0)

#define c_log(color, ...) \
    printf("\033[32m[cpu(%d)]:\033[0m", _cpu());\
    printf("\033[%dm", color); \
    printf(__VA_ARGS__); \
    printf("\033[0m");


typedef struct trap_handler{
  int event;
  int seq;
  handler_t handler;
  struct trap_handler *next;
} trap_handler_t;

struct spinlock{
  intptr_t locked;       // Is the lock held?
  int intr;  // sti() cli() control.
  // For debugging:
  char *name;        // Name of lock.
  int cpu;   // The cpu holding the lock.
};

struct semaphore{
    int value;
    struct spinlock lock;
    // For debugging:
    char *name;
    int cpu;
};

struct task {
  struct {
    const char *name;
    struct task *next;
    _Context   *context;
    //For debugging:
    int cpu;
  };
  uint8_t stack[4096]; //4096-HDRsize
};

#endif