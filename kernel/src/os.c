#include "../include/common.h"
#include "pmm.h"

// #define ALIGNMENT 8
// #define ALIGN(a) ROUNDUP(a,ALIGNMENT)
//static void test();
#define _EVENT_HEAD 999
extern spinlock_t task_lock;
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
  printf("Hello World from CPU #%d\n",_cpu());
  //_intr_write(1); //开中断（write(0)为关中断）
  //trap_handler_t *p = &head;
  // while(p){
  //   c_log(GREEN, "EVENT_%d handler added, seq: %d\n", p->event, p->seq);
  //   p = p->next;
  // }
  printf("tried to create task\n");
  kmt->create(pmm->alloc(sizeof(task_t)) ,"idle", test_entry, &num);
  while(1){
    _yield();
  }
}

_Context *os_trap(_Event ev, _Context *context){
  _Context *next = NULL;
  c_log(YELLOW, "in os->trap!\n");
  trap_handler_t *h = head.next;
  r_panic_on( h == NULL, "No trap handler\n");
  if(ev.event == _EVENT_YIELD){
    c_log(CYAN, "EVENT YIELD!\n");
  }
  while(h){
    if (h->event == _EVENT_NULL || h->event == ev.event) {
      c_log(YELLOW, "Try calling handler for evnet no.%d\n", h->event);
      _Context *r = h->handler(ev, context);
      c_log(YELLOW, "Returned from handler for event no.%d\n", h->event);
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
  kmt->spin_lock(&task_lock);
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
  kmt->spin_unlock(&task_lock);
  return ; //register the ev.handler.
};

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .trap = os_trap,
  .on_irq = os_on_irq,
};