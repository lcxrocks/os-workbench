#include <common.h>
#include <unistd.h>
#include "pmm.h"
#define INFO_SIZE 128
#define HDR_SIZE 1024
#define PAGE_SIZE 8192
#define MAP_SIZE (PAGE_SIZE-HDR_SIZE)

uintptr_t heap_ptr;
uintptr_t heap_end;

typedef struct __lock_t {
  intptr_t flag;
}pmm_spinlock_t;

pmm_spinlock_t BIGLOCK;

typedef union page {
  struct {
    pmm_spinlock_t lock; // 锁，用于串行化分配和并发的 free
    int type; // 分配slab种类
    int max_cnt;
    int obj_cnt;     // 页面中已分配的对象数，减少到 0 时回收页面(暂时不管)
    struct __head * list_head;  // 属于同一个线程的页面的链表
    union page * next;
  }; // 匿名结构体
  struct {
    uint8_t header[INFO_SIZE];
    uint8_t bitmap[HDR_SIZE-INFO_SIZE];
    uint8_t data[PAGE_SIZE - HDR_SIZE];
  } __attribute__((packed));
} page_t;


typedef struct __head{
  page_t *_32;
  page_t *_64;
  page_t *_128;
  page_t *_256;
  page_t *_512;
  page_t *_1024;
  page_t *_2048;
  page_t *_4096;
}list_head;

INIT_MPE(list_head, cpu);
CREATE_FUNC(list_head *,CPU, cpu);

size_t ALIGN(size_t v){
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

void init(pmm_spinlock_t *mutex){
  mutex->flag = 0;
}

void inline lock(pmm_spinlock_t *mutex){
  while(_atomic_xchg(&(mutex->flag), 1) == 1)
    ;
}

void inline unlock(pmm_spinlock_t *mutex){
  _atomic_xchg(&(mutex->flag), 0);
}

void inline *sbrk(intptr_t increment){
  if(increment == 0){
    return (void *) heap_ptr;
  }
  else{
    uintptr_t tmp = heap_ptr;
    heap_ptr += increment;
    if(heap_ptr > heap_end) return NULL;
    else return (void *)tmp;
  }
}

static void *kalloc(size_t size) {
  size_t sz = ALIGN(size); //ROUNDUPTO the nearest power of 2
  size_t flag = sz & 0xfffffffff; //find which 
  flag = (flag<32) ? 32 : flag;
  list_head *hp = CPU(_cpu());
  page_t *p = NULL;
  int free = -1;
  switch(flag){
    case 32:   p = hp->_32; break;
    case 64:   p = hp->_64; break;
    case 128:  p = hp->_128; break;
    case 256:  p = hp->_256; break;
    case 512:  p = hp->_512; break;
    case 1024: p = hp->_1024; break;
    case 2048: p = hp->_2048; break;
    case 4096: p = hp->_4096; break;
  }
  if(p == NULL){
    lock(&BIGLOCK);
    page_t *page_head = (page_t *)sbrk(PAGE_SIZE);
    unlock(&BIGLOCK);
    page_head->type = flag;
    page_head->list_head = CPU(_cpu());
    page_head->max_cnt = MAP_SIZE/page_head->type;
    page_head->obj_cnt =0;
    page_head->next = NULL;
    switch(flag){
    case 32:   hp->_32  = page_head; break;
    case 64:   hp->_64  = page_head; break;
    case 128:  hp->_128 = page_head; break;
    case 256:  hp->_256 = page_head; break;
    case 512:  hp->_512 = page_head; break;
    case 1024: hp->_1024= page_head; break;
    case 2048: hp->_2048= page_head; break;
    case 4096: hp->_4096= page_head; break;
    }
    p = page_head;
    init(&page_head->lock);
    memset(page_head->data,0,sizeof(page_head->data));
    memset(page_head->bitmap,0,sizeof(page_head->bitmap));
    //printf("created head for %d slab @ %p\n ",flag, p);
  }
  page_t *hp_flag = p;
  while(p){
    //printf("(cpu#%d) p: %p\n", _cpu(), p);
    if(p->obj_cnt!=p->max_cnt){
      for (int i = 0; i < p->max_cnt; i++)
      {
        if(p->bitmap[i]==0)
        {
          free = i;
          p->obj_cnt++;
          p->bitmap[i] = 1;       
          break;
        }
      }
    }
    else 
      p = p->next;

    if(free != -1) break; //found slab
  }
  if(free == -1 ) //slow path 1
  {
    lock(&BIGLOCK);
    page_t * new_page = (page_t *)sbrk(PAGE_SIZE);
    unlock(&BIGLOCK);
    new_page->type = flag;
    new_page->max_cnt = MAP_SIZE/flag ;
    new_page->obj_cnt = 0;
    new_page->list_head = CPU(_cpu());
    new_page->next =hp_flag->next;
    hp_flag->next = new_page;
    //new_page->prev = new_page->list_head;
    //init(&new_page->lock);
    /*printf("NEWPAGE: %p\n \t type: %d \n \
      \t capacity: %d/%d \n \
      \t list_head: %p \n \
      \t next: %p \n \
    And list_head_%d->next :%p\n \
      ", new_page->data, new_page->type, new_page->obj_cnt, new_page->max_cnt, new_page->list_head, new_page->next, flag, hp_flag->next);*/
    free = 0;
    new_page->obj_cnt++;
    new_page->bitmap[0] = 1;
    p = new_page;
  }
  void * ret = (void *)p;
  ret = p->data + p->type*free;
  //ret += (p->type == 2048 || p->type == 4096)? (p->type - 1024) : 0;
  if(p->type == 2048) ret += 1024;
  else if(p->type == 4096) ret += 3072;
  return ret;
}

static void kfree(void *ptr) {
  page_t *page = (page_t *) ((uintptr_t)ptr&(~(PAGE_SIZE -1)));
  int i = ((uintptr_t)ptr - (uintptr_t)page->data)/page->type;
  page->bitmap[i] = 0;
  page->obj_cnt--;
}

static void pmm_init() {
  heap_ptr = (uintptr_t) _heap.start;
  heap_end = (uintptr_t) _heap.end;
  init(&BIGLOCK);
  // for (int i = 0; i < _ncpu(); i++)
  //   memset(CPU(i),0,sizeof(CPU(i)));
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};