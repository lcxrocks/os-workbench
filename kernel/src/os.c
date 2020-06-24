#include "../include/common.h"
#include "pmm.h"

// #define ALIGNMENT 8
// #define ALIGN(a) ROUNDUP(a,ALIGNMENT)
//static void test();
#define _EVENT_HEAD 999

trap_handler_t head = {0, _EVENT_HEAD, NULL, NULL, NULL};

static void os_init() {
  pmm->init();
  kmt->init();
  //dev-init();
#ifdef DEBUG
  test();// here;
#endif
}

void test_entry(void *num){
  c_log(YELLOW, "test on %d\n", *((int *)(num)));
}

int num = 10;

static void os_run() {
  // for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
  //   _putc(*s == '*' ? '0' + _cpu() : *s);
  // }
  // have printf() bug. plz use native  
  printf("Hello World from CPU #%d\n",_cpu());
  _intr_write(1); //开中断（write(0)为关中断）
  //trap_handler_t *p = &head;
  // while(p){
  //   c_log(GREEN, "EVENT_%d handler added, seq: %d\n", p->event, p->seq);
  //   p = p->next;
  // }
  kmt->create(pmm->alloc(sizeof(task_t)) ,"test", test_entry, &num);
  while(1){
    //c_log(CYAN, "os running\n");
  }
  //while (1) ; //should not keep waiting 
}

_Context *os_trap(_Event ev, _Context *context){
  _Context *next = NULL;
  c_log(YELLOW, "in os->trap!\n");
  trap_handler_t *h = head.next;
  while(h){
    if (h->event == _EVENT_NULL || h->event == ev.event) {
      c_log(YELLOW, "Try calling handler for evnet no.%d", h->event);
      _Context *r = h->handler(ev, context);
      panic_on(r && next, "returning multiple contexts");
      if (r) next = r;
    }
    h = h->next;
  }
  panic_on(!next, "returning NULL context");
  //panic_on(sane_context(next), "returning to invalid context");
  return next;
};

void os_on_irq(int seq, int event, handler_t handler){
  trap_handler_t *h = pmm->alloc(sizeof(trap_handler_t));
  h->event = event;
  h->seq = seq;
  h->handler = handler;
  h->next = NULL;
  h->prev = NULL;
  trap_handler_t *p = &head;
  bool same_event = false;
  while(p->next){
    p = p->next;
    if(p->event == event){
      same_event = true;
      break;
    }
  }
  if(same_event){ // stop at the first trap_handler with the same event no.
    while(seq > p->seq && p->next!=NULL && p->next->event != event){
      p = p->next;
    }
    r_panic_on(p->prev == NULL, "p->prev is NULL\n");
    r_panic_on(p->prev->next != p, "linked list is buggy\n");
    if(p->seq < seq){
      if(p->next){ // p->next->event != event
        p->next->prev = h;
        h->next = p->next;
        p->next = h;
        h->prev = p;
      }
      else{ // p->next = NULL
        p->next = h;
        h->prev = p;
      }
    } // common case
    else{
      p->prev->next = h;
      h->prev = p->prev;
      p->prev = h;
      h->next = p;
    }
  }
  if(!same_event){
    p->next = h;
    h->prev = p;
  } // no event no.

  r_panic_on(head.next==NULL, "Adding event_handler failed\n");
  c_log(CYAN, "Event[%d] handler added.(seq: %d)\n", event, seq);
  return ; //register the ev.handler.
};

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .trap = os_trap,
  .on_irq = os_on_irq,
};