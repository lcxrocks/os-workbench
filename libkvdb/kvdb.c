#include "kvdb.h"

typedef struct kvdb {
  // your definition here
  int fd;
  char filename[128];
}kvdb_t;

void fsck(kvdb_t *db){
  c_log(GREEN, "+++ Doing fsck for : ");
  c_log(CYAN, "[%s]\n", db->filename);
}

struct kvdb *kvdb_open(const char *filename) {
  kvdb_t *db = malloc(sizeof(kvdb_t));
  strcpy(db->filename, filename);
  db->fd = open(filename, O_CREAT | O_RDWR, S_IWUSR | S_IRUSR );
  c_log(YELLOW, ">> Open database : ");
  c_log(CYAN, "[%s]\n", db->filename);
  //check consistency
  return db;
}

int kvdb_close(struct kvdb *db) {
  if(!close(db->fd)) {
    c_log(RED, "Close file failed : ");
    c_log(CYAN, "[%s]\n", db->filename);
  };
  c_log(GREEN, ">> Closed database : ");
  c_log(CYAN, "[%s]\n", db->filename);
  free(db);
  return 0;
}

int kvdb_put(struct kvdb *db, const char *key, const char *value) {
  flock(db->fd, LOCK_EX);
  fsck(db);
  // 1. log
  // 2. write in data
  flock(db->fd, LOCK_UN);
  return 0; 
}

char *kvdb_get(struct kvdb *db, const char *key) {
  flock(db->fd, LOCK_EX);
  fsck(db);
  // read the data
  // char value[SUPER_BIG] = malloc(value);
  flock(db->fd, LOCK_UN);
  return NULL;
}
