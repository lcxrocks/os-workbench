#ifndef COMMON_H
#define COMMON_H
//#define DEBUG
#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>
#include <amdev.h>
#include <sys/types.h>

#define MAX_CPU 8
#define MAX_ONTIME 3
#define MIN_LASTTIME 35
#define NRTASK 64
 
#define RED 31
#define GREEN 32
#define YELLOW 33
#define BLUE 34
#define PURPLE 35
#define CYAN 36
#define WHITE 37

#ifdef DEBUG
#define r_panic_on(cond, ...) \
    c_panic_on(RED, cond, __VA_ARGS__);


  #define c_panic_on(color, cond, ...) \
  do{ \
    if(cond) {\
        kmt->spin_lock(&info_lock);\
        printf("\033[36m[cpu(%d)]:\033[0m", _cpu());\
        printf("\033[%dm", color); \
        printf(__VA_ARGS__); \
        printf("\033[0m"); \
        kmt->spin_unlock(&info_lock); \
        _halt(1);\
    }\
  }while(0); \

#define c_log(color, ...) \
    kmt->spin_lock(&info_lock);\
    printf("\033[32m[cpu(%d)]:\033[0m", _cpu());\
    printf("\033[%dm", color); \
    printf(__VA_ARGS__); \
    printf("\033[0m"); \
    kmt->spin_unlock(&info_lock);
    //kmt->spin_unlock(&info_lock);
    //kmt->spin_lock(&info_lock);
#else
  #define r_panic_on(cond, ...) ;
  #define c_panic_on(color, cond, ...) ;
  #define c_log(color, ...) ;
#endif

#define RANGE(st, ed) (_Area) { .start = (void *)(st), .end = (void *)(ed) }
#define IN_RANGE(ptr, area) ((area).start <= (ptr) && (ptr) < (area).end)

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

#define N 4
#define MAGIC 0x5a5aa5a5
typedef uint32_t canary_t[N];
#define TASK_HEAD (6*sizeof(int) + sizeof(void *) + sizeof(struct semaphore *) + \
  sizeof(char *) + sizeof(struct task *) +sizeof(_Context *) + 2*sizeof(canary_t))\

struct task {
  struct {
    int stat;
    int pid;
    int cpu;
    int ord;
    int on_time;
    int last_time;
    void *entry;
    struct semaphore *sem; // sleeping on sem
    const char *name; // debugging
    struct task *next; 
    _Context   *context;
  }__attribute__((packed));
  canary_t __c1;
  uint8_t stack[4096 - TASK_HEAD]; //4096-HDRsize
  canary_t __c2;
};

struct cpu_local{
  int nr_task;
  task_t *current; // the process running on this cpu or null
  task_t *cpu_idle; // task_t *chan
}cpu_info[MAX_CPU];

enum stat { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

#define current_task cpu_info[_cpu()].current
#define idle cpu_info[_cpu()].cpu_idle
#define IDLE NULL
#endif 
