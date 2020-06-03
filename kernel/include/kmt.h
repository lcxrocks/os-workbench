#ifndef KMT_H
#define KMT_H
#include <common.h>

typedef struct task {
  struct {
    const char *name;
    struct task *next;
    _Context   *context;
  };
  uint8_t stack[4096];
} task_t;

void kmt_init();
int  kcreate(task_t *task, const char *name, void (*entry)(void *arg), void *arg);
void teardown(task_t *task);

#endif