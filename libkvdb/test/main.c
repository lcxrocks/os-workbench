#include <stdio.h>
#include <stdlib.h>
#include "../kvdb.h"
#include "debug.h"

int main() {
  struct kvdb *db;
  const char *key = "operating-systems";
  char *value;
  //r_panic_on(sizeof(log_t)!=17*MB, "Wrong log header");
  //r_panic_on(sizeof(table_t)!=1*MB, "Wrong header sz");
  panic_on(!(db = kvdb_open("/tmp/g.db")), "cannot open db"); // 打开数据库
  for (int i = 0; i < 200; i++)
  {
    char num[128];
    sprintf(num, "%d", i);
    char new_num[128];
    sprintf(new_num, "%d", 250-i);
    kvdb_put(db, num, new_num);
  }
  c_log(GREEN, "kvdb_put finished!\n");
  for (int i = 0; i < 200; i++)
  {
    char num[128];
    sprintf(num, "%d", i);
    char *tmp = kvdb_get(db, num);
    printf("[%s]: [%s]\n", num, tmp);
  }
  kvdb_put(db, key, "threeces"); // db[key] = "three-easy-pieces"
  value = kvdb_get(db, key); // value = db[key];
  kvdb_close(db); // 关闭数据库
  printf("[%s]: [%s]\n", key, value);
  free(value);
  return 0;
}