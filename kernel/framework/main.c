//#include <kernel.h>
#include <klib.h>
#include <common.h>
#include <devices.h>

int tid = 0;
sem_t empty;
sem_t fill;
#define P(s) kmt->sem_wait(s);
#define V(s) kmt->sem_signal(s);

void printer()  { tid++; while (1) { printf("%d", tid); } }
void printer2()  {  while (1) { printf("."); } }
void producer() { while (1) { P(&empty); tid++; printf("%d", tid); V(&fill);  } }
void consumer() { while (1) { P(&fill);  tid--; printf("%d", tid); V(&empty); } }

static void tty_reader(void *arg) {
  device_t *tty = dev->lookup(arg);
  char cmd[128], resp[128], ps[16];
  snprintf(ps, 16, "(%s) $ ", arg);
  while (1) {
    tty->ops->write(tty, 0, ps, strlen(ps));
    int nread = tty->ops->read(tty, 0, cmd, sizeof(cmd) - 1);
    cmd[nread] = '\0';
    sprintf(resp, "tty reader task: got %d character(s).\n", strlen(cmd));
    tty->ops->write(tty, 0, resp, strlen(resp));
  }
}

int main() {
  printf("CPU reset.\n");
  _ioe_init();
  _cte_init(os->trap);
  //_vme_init(pmm->alloc, pmm->free);
  os->init();
  kmt->create(pmm->alloc(sizeof(task_t)), "tty_reader_1", tty_reader, "tty1");
  kmt->create(pmm->alloc(sizeof(task_t)), "tty_reader_2", tty_reader, "tty2");
  // kmt->sem_init(&empty, "empty", 1);
  // kmt->sem_init(&fill, "fill", 0);
  // kmt->create(pmm->alloc(sizeof(task_t)), "producer", producer, NULL);
  // kmt->create(pmm->alloc(sizeof(task_t)), "consumer", consumer, NULL);
  _mpe_init(os->run); //call-user-entry(os->run)
  return 1;
}
