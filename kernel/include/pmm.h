#ifndef PMM_H
#define PMM_H
#define INIT_MPE(type,name) \
type name##_0;\
type name##_1;\
type name##_2;\
type name##_3;\
type name##_4;\
type name##_5;\
type name##_6;\
type name##_7;\

#define CREATE_FUNC(type, func_name, name) \
type func_name(int i){\
  switch(i){\
    case 0: return &name##_0;\
    case 1: return &name##_1;\
    case 2: return &name##_2;\
    case 3: return &name##_3;\
    case 4: return &name##_4;\
    case 5: return &name##_5;\
    case 6: return &name##_6;\
    case 7: return &name##_7;\
    default: panic("should not reach here\n");\
  }\
}

#define INIT_SLAB_HDR(c) \
page_t *cpu_##c##_32   = NULL; \
page_t *cpu_##c##_64   = NULL; \
page_t *cpu_##c##_128  = NULL; \
page_t *cpu_##c##_256  = NULL; \
page_t *cpu_##c##_512  = NULL; \
page_t *cpu_##c##_1024 = NULL; \
page_t *cpu_##c##_2048 = NULL; \
page_t *cpu_##c##_4096 = NULL; \

#endif
//conclusion: when allocating a large amount of memory, use mmap;
//else, use sbrk;
//heap size: 126MiB;
//fast path and slow path
/*
  fast path: 
    1) linked-list 

    2) slab stradegy : set per-thread cache for every common size malloc request  
      - every slab has the same size.
      - for every thread, prepare a local cache :
        i.e. for 4B request: prepare 4KiB  <----has a header pointing to a 4KiB slab zone. 
             for 8B request: prepare 4KiB
             ...
      - has a header:
        metadata header{
          type 4KB/8KB/24KB;
          slab ptr prev; // previous slab zone;
          bitmap; //shows the current avaliable slab.
        }
      - when the current slab is full, allocate a new one, using linked list to connect it and change the pointer.
      - when the current slab is empty, we can free this slab.

  slow path:
    linked list: keep record of all pages 
    use lock to preserve. 
    pgalloc + pgfree
*/
// static inline intptr_t xchg(volatile intptr_t *addr, intptr_t newval) {
//   intptr_t result;
//   asm volatile ("lock xchg %0, %1":
//     "+m"(*addr), "=a"(result) : "1"(newval) : "cc");
//   return result;
// }



