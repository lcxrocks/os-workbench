#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#define N 4
#define MAGIC 0x5a5aa5a5
typedef uint32_t canary_t[N];

#define panic_on(cond, s) \
  do { \
    if (cond) { \
      printf("%s", s); \
      assert(0); \
    } \
  } while (0);

struct kernel_stack {
  canary_t __c1;
  char data[4096 - sizeof(canary_t) * 2];
  canary_t __c2;
};

void canary_init(canary_t *c) {
  for (int i = 0; i < N; i++) (*c)[i] = MAGIC; 
}

int main(){
  printf("%ld\n",sizeof(canary_t));
    struct kernel_stack *sp = malloc(sizeof(struct kernel_stack)); 
    canary_init(&sp->__c1);
    canary_init(&sp->__c2);
}