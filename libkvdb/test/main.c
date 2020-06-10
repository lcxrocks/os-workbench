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
  panic_on(!(db = kvdb_open("/tmp/c.db")), "cannot open db"); // 打开数据库
  // for (int i = 0; i < 250; i++)
  // {
  //   char num[127];
  //   sprintf(num, "%d", i);
  //   kvdb_put(db, num, num);
  // }
  // c_log(GREEN, "Finished kvdb_put()!\n");
  // for (int i = 0; i < 250; i++)
  // {
  //   char num[127];
  //   sprintf(num, "%d", i);
  //   value = kvdb_get(db, num);
  //   printf("[%s]: [%s]\n", num, value);
  // }

  kvdb_put(db, "lcx", "good");
  kvdb_put(db, key, "three-easy-pieces"); // db[key] = "three-easy-pieces"
  kvdb_put(db, key, "three-hard-pieces");
  kvdb_put(db, "lcx", "very good");
  value = kvdb_get(db, key); // value = db[key];
  char *lcx = kvdb_get(db, "lcx");
  printf("[%s]: [%s]\n", "lcx", lcx);
  kvdb_close(db); // 关闭数据库
  printf("[%s]: [%s]\n", key, value);
  free(value);
  return 0;
}