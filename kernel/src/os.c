#include <common.h>
#include "pmm.h"

// #define ALIGNMENT 8
// #define ALIGN(a) ROUNDUP(a,ALIGNMENT)
//static void test();

static void os_init() {
  pmm->init();
  kmt->init();
  //dev-init();
#ifdef DEBUG
  test();// here;
#endif
}

static void os_run() {
  // for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
  //   _putc(*s == '*' ? '0' + _cpu() : *s);
  // }
  // could have printf bug
  printf("Hello World from CPU #%d\n",_cpu());
  //_intr_write(1); //开中断（write(0)为关中断）
  while (1) ; //keep waiting 
}

_Context *os_trap(_Event ev, _Context *context){
  // _Context *next = NULL;
  // for (auto &h: handlers_sorted_by_seq) { // retrival of handlers 
  //   if (h.event == _EVENT_NULL || h.event == ev.event) {
  //     _Context *r = h.handler(ev, ctx);
  //     panic_on(r && next, "returning multiple contexts");
  //     if (r) next = r;
  //   }
  // }
  // panic_on(!next, "returning NULL context");
  // panic_on(sane_context(next), "returning to invalid context");
  // return next;
  return NULL;
};

void os_on_irq(int seq, int event, handler_t handler){
  return ;
};

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .trap = os_trap,
  .on_irq = os_on_irq,
};

// static void test(){
//   for (int i = 1; i <= 10; i++)
//   {
//     printf("-----------------i = %d ---------------------\n",i);
//     void *p = pmm->alloc(1023);
//     if(p) printf("alloc success! at %p\n",p);
//     else printf("alloc fail!");

//   }
//   for (int i = 1; i <= 5; i++)
//   {
//     printf("--------------------------------------------\n",i);
//     void *p = pmm->alloc(2047);
//     if(p) printf("alloc success! at %p\n",p);
//     else printf("alloc fail!");
//     pmm->free(p);
//   }
//   return ;
// }