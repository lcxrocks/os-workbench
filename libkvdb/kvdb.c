#include "kvdb.h"

log_t Log;
table_t Table;

void read_hdr(kvdb_t *db){
  lseek(db->fd, 0, SEEK_SET);
  read(db->fd, &Log, sizeof(log_t));
  lseek(db->fd, 17*MB, SEEK_SET);
  read(db->fd, &Table, sizeof(table_t));  
}

// void log_clean(kvdb_t *db){
//   memset(&Log, 0, 17*MB);
//   write(db->fd, &Log, 17*MB);
//   fsync(db->fd);
// }

void fsck(kvdb_t *db){
  read_hdr(db);
  if(!Log.TxB){
    return ;
  }// No current log. proceed.
  if(Log.TxB == 1 && Log.TxE == 1 && Log.commit == 1){
    return ;
  }// Log already commited. proceed.
  if(Log.TxB == 1 && Log.TxE == 0){
    return ;
  }// Data lost. break when writing data. Usr to blame. proceed.
  if(Log.TxB == 1 && Log.TxE == 1 && Log.commit == 0){
    // REPAIR!!!
  }// Log didn't commit. Maintenance needed.
  c_log(GREEN, "+++ Doing fsck for : ");
  c_log(CYAN, "[%s]\n", db->filename);
}

struct kvdb *kvdb_open(const char *filename) {
  kvdb_t *db = malloc(sizeof(kvdb_t));
  strcpy(db->filename, filename);
  db->fd = open(filename, O_CREAT | O_RDWR, S_IWUSR | S_IRUSR );
  c_log(YELLOW, ">> Open database : ");
  c_log(CYAN, "[%s]\n", db->filename);
  return db;
}

int kvdb_close(struct kvdb *db) {
  if(close(db->fd) == -1) {
    c_log(RED, "Close file failed : ");
    c_log(CYAN, "[%s]\n", db->filename);
  }
  else{
    c_log(GREEN, ">> Closed database : ");
    c_log(CYAN, "[%s]\n", db->filename);
  }
  free(db);
  return 0;
}

void write_log(int fd, const char *key, const char *value){
  int len = strlen(value);
  Log.TxB = 1;
  Log.TxE = 0;
  Log.commit = 0;
  Log.nr_block = (len / BLOCKSZ) + 1; // data size
  strcpy(Log.key, key);
  strcpy(Log.data, value);
  write(fd, &Log, LOG_HDR + len);
  fsync(fd);
}

int find_key(const char *key){
  for (int i = 0; i < Table.key_cnt; i++)
  {
    if(!strcmp(key, Table.key[i])) return i;
  }
  return -1;
}// return key number 

void write_fd(int fd, const void *buf,int offset, int len, int MODE){
    lseek(fd, offset, MODE);
    write(fd, buf, len);
    fsync(fd);
}

void write_hdr(int fd, const char *key, const char *value){
  write_log(fd, key, value);
  int key_id = find_key(key);
  int len = strlen(value) + 1;
  if( key_id != -1){
    //case 1: key.len > Log.nr_block
    //case 2: key.len <= Log.nr_block
  }
  else{ // didn't find
    key_id = Table.key_cnt;
    strcpy(Table.key[Table.key_cnt], key);
    if(Table.key_cnt == 0) 
      r_panic_on(Table.block_cnt!=0, "TABLE NEED INIT");
    Table.start[Table.key_cnt] = DATA_START + Table.block_cnt * BLOCKSZ;
    Table.len[Table.key_cnt] = Log.nr_block;
    Table.key_cnt++;
    Table.block_cnt += Log.nr_block;
    write_fd(fd, &Table, 17*MB, 1*MB, SEEK_SET); // write in table
    write_fd(fd, value, RSVDSZ, Table.len[key_id] * BLOCKSZ, SEEK_SET);
  }
  Log.TxE = 1;

  write(fd, &Log, 17*MB);
  write(fd, &Table, 1*MB);
  fsync(fd);
}

int kvdb_put(struct kvdb *db, const char *key, const char *value) {
  flock(db->fd, LOCK_EX);
  fsck(db);
  // 1. log
  write_hdr(db->fd, key, value);
  // 2. write in data
  flock(db->fd, LOCK_UN);
  return 0; 
}

char *kvdb_get(struct kvdb *db, const char *key) {
  flock(db->fd, LOCK_EX);
  fsck(db);
  // read the data
  int key_id = find_key(key);
  char *ret = malloc(4*MB);
  if(key_id != -1){
    printf("key: %d, start: %zx, len: %d\n", key_id, Table.start[key_id], Table.len[key_id]);
    lseek(db->fd, Table.start[key_id], SEEK_SET);
    int bytes= read(db->fd, ret, Table.len[key_id]*BLOCKSZ);
    char *p = ret;
    for (int i = 0; i < Table.len[key_id]; i++)
    {
      printf("%ls", (int *) p);
      p++;
    }
    
    c_log(GREEN, "bytes : %d\n", bytes);
  }
  else{
    c_log(RED, "didn't find key\n");
    free(ret);
    ret = NULL;
  }
  // char value[SUPER_BIG] = malloc(value);
  flock(db->fd, LOCK_UN);
  return ret;
}
