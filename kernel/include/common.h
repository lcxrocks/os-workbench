#ifndef COMMON_H
#define COMMON_H
//#define DEBUG
#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>

#define MAX_CPU 8
#define NRTASK 64

#define RED 31
#define GREEN 32
#define YELLOW 33
#define BLUE 34
#define PURPLE 35
#define CYAN 36

#define r_panic_on(cond, ...) \
    c_panic_on(RED, cond, __VA_ARGS__);

#define c_panic_on(color, cond, ...) \
do{ \
    if(cond) {\
        printf("\033[36m[cpu(%d)]:\033[0m", _cpu());\
        printf("\033[%dm", color); \
        printf(__VA_ARGS__); \
        printf("\033[0m"); \
        _halt(1);\
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
  struct trap_handler *prev;
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
    int stat;
    int pid;
    void *entry;
    const char *name; // debugging
    struct task *next; 
    _Context   *context;
  };
  uint8_t stack[4096]; //4096-HDRsize
};

struct cpu_local{
  task_t *current; // the process running on this cpu or null
  void *idle; // void *chan
}cpu_info[MAX_CPU];

enum stat { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

#define current cpu_info[_cpu()].current

#endif 