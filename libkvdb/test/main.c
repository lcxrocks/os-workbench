#include <stdio.h>
#include <stdlib.h>
#include "../kvdb.h"
#include "debug.h"

#define DIG(i) #i

int main() {
  struct kvdb *db;
  const char *key = "operating-systems";
  char *value;
  //r_panic_on(sizeof(log_t)!=17*MB, "Wrong log header");
  //r_panic_on(sizeof(table_t)!=1*MB, "Wrong header sz");
  panic_on(!(db = kvdb_open("/tmp/a.db")), "cannot open db"); // 打开数据库
  for (int i = 0; i < 10000; i++)
  {
    kvdb_put(db, DIG(i), "haha");
    printf("%s ", DIG(i));
  }
  
  kvdb_put(db, key, "three-easy-pieces"); // db[key] = "three-easy-pieces"
  value = kvdb_get(db, key); // value = db[key];
  kvdb_close(db); // 关闭数据库
  printf("[%s]: [%s]\n", key, value);
  free(value);
  return 0;
}