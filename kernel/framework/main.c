//#include <kernel.h>
#include <klib.h>
#include <common.h>
int tid = 0;

void printer()  { tid++; while (1) { printf("%d", tid); } }

int main() {
  _ioe_init();
  _cte_init(os->trap);
  //_vme_init(pmm->alloc, pmm->free);
  kmt->create(pmm->alloc(sizeof(task_t)), "printer", printer, NULL);
  os->init();
  _mpe_init(os->run); //call-user-entry(os->run)
  return 1;
}
