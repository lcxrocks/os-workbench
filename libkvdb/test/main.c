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
  panic_on(!(db = kvdb_open("/tmp/b.db")), "cannot open db"); // 打开数据库
  // for (int i = 0; i < 100; i++)
  // {
  //   char num[127];
  //   sprintf(num, "%d", i);
  //   kvdb_put(db, num, "haha");
  // }
  // c_log(GREEN, "Finished kvdb_put()!\n");
  // for (int i = 0; i < 100; i++)
  // {
  //   char num[127];
  //   c_log(YELLOW, "GETTING NUM:%d\n", i);
  //   sprintf(num, "%d", i);
  //   value = kvdb_get(db, num);
  //   printf("[%s]: [%s]\n", num, value);
  // }
  
  kvdb_put(db, key, "three-easy-pieces"); // db[key] = "three-easy-pieces"
  value = kvdb_get(db, key); // value = db[key];
  kvdb_close(db); // 关闭数据库
  printf("[%s]: [%s]\n", key, value);
  free(value);
  return 0;
}