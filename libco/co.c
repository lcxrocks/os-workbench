/*
    STACK_32:
High

               
  ------------  <- stack + STACK_SIZE: aligned(16) ----- bottom ----> esp
       arg         
  ------------  <- bottom - 4   (esp - 4)
    ret->addr (need filling)               
  ------------  <- bottom - 8   (esp - 8)
          
  ------------  <- bottom - 12

  ...

  ------------  <- stack

Low

    STACK_64:
High

               
  ------------  <- stack + STACK_SIZE: aligned(16) ----- bottom
    ret->addr          
  ------------  <- bottom - 8 (rsp)
               
  ------------  <- bottom - 16     (aligned(16))
        
  ------------ 

  ...

  ------------  <- stack

Low
*/
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

static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg) {
  asm volatile (
#if __x86_64__
    "movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
      : : "b"((uintptr_t)sp),     "d"(entry), "a"(arg)
#else
    "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
      : : "b"((uintptr_t)sp - 8), "d"(entry), "a"(arg)
#endif
  );
}

//stack_switch_call(void *sp, void *entry, uintptr_t arg)
enum co_status {
  CO_NEW = 1, // 新创建，还未执行过
  CO_RUNNING, // 已经执行过
  CO_WAITING, // 在 co_wait 上等待
  CO_DEAD,    // 已经结束，但还未释放资源
};

struct co {
  const char *name;
  void (*func)(void *); // co_start 指定的入口地址和参数
  void *arg;

  enum co_status status;  // 协程的状态
  struct co *    next;  // 是否有其他协程在等待当前协程 ***理解成链表的next了
  jmp_buf        context; // 寄存器现场 (setjmp.h)
  uint8_t        stack[STACK_SIZE]; // 协程的堆栈
  void * sp;
}__attribute__((aligned(16))); //16 字节对齐（x86-64要求）

static void start() __attribute__((constructor));
static void end() __attribute__((destructor));
struct co *current; 
struct co *head; //keep record of the coroutines
int cnt; // keep record of the current number of coroutines

void destroy(struct co* thd){
  if(!head) return;
  if(thd == head){
    struct co* tmp = head;
    head->next = tmp;
    free(head);
    cnt--;
    head = tmp;
    return ;
  }
  assert(head->next);
  struct co* cp = head;
  while(cp->next !=NULL && cp->next != thd) cp = cp->next;
  assert(cp);
  cp->next = thd->next;
  cnt--;
  //printf("=====================\n now running %d threads \n =====================\n",cnt);
  free(thd);
  return;
}

static void start(){
  srand(time(NULL));
  current = co_start("main", NULL, NULL);
  current->status = CO_RUNNING;
}

static void end(){
  for(struct co* cp = head; cp; cp = cp->next)
    destroy(cp); // could be buggy
}

void finish(){
  current->status = CO_DEAD;
  struct co* cp = head;
  assert(head);
  current = cp;
  longjmp(current->context, 1);
}

/* create a new FSM and create its own GPRs and stack frame, plus pass the args */ 
struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  struct co* ret = (struct co*) malloc(sizeof(struct co));
  assert(ret);
  ret->name = name;
  ret->func = func;
  ret->arg  = arg;
  if(head){
    ret->next = head->next;
    head->next = ret;
  }
  else{
    head = ret;
    ret->next = NULL;
  }
  ret->status = CO_NEW;
  uintptr_t tmp = (uintptr_t)ret->stack + sizeof(ret->stack) - 8;
  uintptr_t ret_addr = (uintptr_t) finish;
  
  memcpy((char *)tmp, &ret_addr, sizeof(uintptr_t));
  //printf("finsh_addr attached! \t %p at stack_position: %p  === %s: \n", (void *)finish, (void *)tmp, (char *)tmp);
  #if __x86_64__
  #else 
    tmp += 8;
  #endif

  ret->sp = (void *)tmp;
  //printf("ret->stack:%p \t ret->sp: %p \t ret->sp-4: %p \n",ret->stack, ret->sp, ret->sp-4);
  cnt++;

  #ifdef DEBUG
    printf("START new coroutine name: %s \n", name);
    printf("stack_start: %p, stack_end: %p\n", ret->stack, (void *)(ret->stack+(STACK_SIZE)));
  #endif

  #ifdef DEBUG
    printf("------------------AMOUNT: %d -------------------\n", cnt);
    printf("------------------COROUTINES-------------------\n");
    struct co* cp =head;
    do
    {
      printf("name: %s \t status : %d \t  stack_ptr: %p\n",cp->name,cp->status, cp->sp);
      cp = cp->next;
    } while (cp!=NULL);
    
    printf("================================================\n");
  #endif

  return ret;
}

/* setjmp save GPRs, ebx, rsp, rip */
/* int setjmp(jmp_buf env)    */ 
/* randomly choose a co_thread and returns */
void co_yield() {
  //save the current GPRs and stack frame
  //switch to another co-thread
  #ifdef DEBUG
    printf("*********************YIELD**********************\n");
    
    printf("************************************************\n");
  #endif

  int val = setjmp(current->context); //current  is null;
  //printf("val: %d\n", val);
  if (val == 0) {
    current->status == CO_WAITING;
    int ran = rand()%cnt;
    struct co* cp = head;
    assert(cnt!=0);
    //create new co-thread
    while(ran){
      if(cp && cp->next){
        cp = cp->next;
        ran--;
      }  
    }
    if(cp->status == CO_DEAD) {
      cp = head;
      while(cp && cp->status == CO_DEAD ) cp = cp->next;
    }
    current = cp;

    assert(current);
    assert(current->status != CO_DEAD);
    
    //choose a coroutne as current
    #ifdef DEBUG
      printf("jmping to coroutine [%s].status: %d\n", current->name, current->status);
    #endif

    if(current->status == CO_NEW) //haven't saved jmp_buf
    {
      current->status = CO_RUNNING;
      stack_switch_call(current->sp, current->func, (uintptr_t)current->arg); //first time 
    }
    else //RUNNING, yield 交出控制权
    {
      longjmp(current->context,1);
    }
  } 
  else {
    return ;
  }
}

/* wait until the FSM ends */
void co_wait(struct co *co) {
  #ifdef DEBUG
    printf("waiting coroutine %s:(stat: %d) to finish \n", co->name, co->status);
  #endif 
  while(co->status != CO_DEAD) co_yield();
  destroy(co);
}