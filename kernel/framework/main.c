//#include <kernel.h>
#include <klib.h>
#include <common.h>
int tid = 0;

void printer()  { tid++; while (1) { printf("%d", tid); } }
void printer2()  {  while (1) { printf("hahhaha %d", tid); } }

int main() {
  _ioe_init();
  _cte_init(os->trap);
  //_vme_init(pmm->alloc, pmm->free);
  os->init();
  kmt->create(pmm->alloc(sizeof(task_t)), "printer", printer, NULL);
  kmt->create(pmm->alloc(sizeof(task_t)), "printer2", printer2, NULL);
  _mpe_init(os->run); //call-user-entry(os->run)
  return 1;
}
