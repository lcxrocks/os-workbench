#include "co.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <setjmp.h>
#include <assert.h>
#include <string.h>
#include <time.h>

#define STACK_SIZE 1<<16 // 64 KB
//#define DEBUG
#define next waiter

enum co_status
{
	CO_NEW = 1,
	CO_RUNNING,
	CO_WAITING,
	CO_DEAD,
};

struct co {
  const char *name;
  void (*func)(void *); // co_start 指定的入口地址和参数
  void *arg;
  enum co_status status;  // 协程的状态
  struct co *    waiter;  // 是否有其他协程在等待当前协程 ***理解成链表的next了
  jmp_buf        context; // 寄存器现场 (setjmp.h)
  uint8_t        stack[STACK_SIZE]; // 协程的堆栈
  void * stack_ptr;
}__attribute__((aligned(16))); //16 字节对齐（x86-64要求）

static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg) {
  asm volatile (
#if __x86_64__
    "movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
      : : "b"((uintptr_t)sp),     "d"(entry), "a"(arg)
#else
    "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
      : : "b"((uintptr_t)sp - 8),     "d"(entry), "a"(arg)
#endif
  );
}

struct co *co_start(const char *name, void (*func)(void *), void *arg){
    
}

void co_wait(struct co *co) {

}

void co_yield(){

}