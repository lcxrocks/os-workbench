#include "../include/common.h"
#include "pmm.h"

// #define ALIGNMENT 8
// #define ALIGN(a) ROUNDUP(a,ALIGNMENT)
//static void test();

trap_handler_t *head;

static void os_init() {
  pmm->init();
  kmt->init();
  //dev-init();
#ifdef DEBUG
  test();// here;
#endif
}

void trap_handler_init(){
  r_panic_on(_cpu() != 0, "Multi cpu detected!");
  c_log(GREEN, "It's a trap!\n");
  head = malloc(sizeof(trap_handler_t));
  head->event = 999;
  head->seq = 999;
  head->handler = NULL;
  head->next = NULL;
}

static void os_run() {
  // for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
  //   _putc(*s == '*' ? '0' + _cpu() : *s);
  // }
  // have printf() bug. plz use native  
  printf("Hello World from CPU #%d\n",_cpu());
  _intr_write(1); //开中断（write(0)为关中断）
  while (1) ; //should not keep waiting 
}

_Context *os_trap(_Event ev, _Context *context){
  _Context *next = NULL;
  trap_handler_t *h = head->next;
  while(h){
    if (h->event == _EVENT_NULL || h->event == ev.event) {
      _Context *r = h->handler(ev, context);
      panic_on(r && next, "returning multiple contexts");
      if (r) next = r;
    }
  }
  panic_on(!next, "returning NULL context");
  //panic_on(sane_context(next), "returning to invalid context");
  return next;
};

void os_on_irq(int seq, int event, handler_t handler){
  //should add lock (common space)
  trap_handler_t *h = malloc(sizeof(trap_handler_t));
  h->event = event;
  h->seq = seq;
  h->handler = handler;
  h->next = NULL;
  trap_handler_t *p = head;
  while(p->next){
    p = p->next;
  }
  p->next = h;
  r_panic_on(head->next==NULL, "Adding event_handler failed\n");
  c_log(CYAN, "Event[%d] handler added.(seq: %d)\n", event, seq);
  return ; //register the ev.handler.
};

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .trap = os_trap,
  .on_irq = os_on_irq,
};