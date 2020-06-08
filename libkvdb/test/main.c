#include <stdio.h>
#include <stdlib.h>
#include "../kvdb.h"
#include "debug.h"

int main() {
  struct kvdb *db;
  const char *key = "operating-systems";
  char *value;
  c_log(GREEN, "RSVDSZ: %x\n", RSVDSZ);
  c_log(CYAN, "log_tsz: %zx\n", sizeof(log_t))
  r_panic_on(sizeof(log_t)!=17*MB, "Wrong log header");
  panic_on(!(db = kvdb_open("/tmp/a.db")), "cannot open db"); // 打开数据库

  kvdb_put(db, key, "three-easy-pieces"); // db[key] = "three-easy-pieces"
  value = kvdb_get(db, key); // value = db[key];
  kvdb_close(db); // 关闭数据库
  printf("[%s]: [%s]\n", key, value);
  free(value);
  return 0;
}