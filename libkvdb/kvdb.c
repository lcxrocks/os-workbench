//#include "kvdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <sys/file.h>
#include <sys/types.h>    
#include <sys/stat.h>    
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define DEBUG

#define KB 1024
#define MB (1024 * 1024)
#define KEYLEN 128
#define BLOCKSZ 4096
#define DATALEN (4096*BLOCKSZ)
#define LOG_HDR (3*sizeof(int) + KEYLEN*sizeof(char))
#define KEYNUM 257 // For one table 
#define RSVDSZ 18*MB
#define DATA_START RSVDSZ

typedef struct __log{
  int commit;
  int nr_block; // how long is the new value
  int cur_key_id;
  char key[KEYLEN];
  char data[DATALEN];
}__attribute__((packed)) log_t;

typedef struct key_table{
  char key[KEYNUM][KEYLEN];
  uintptr_t  start[KEYNUM];
  int  len[KEYNUM];
  //struct table *next;
  int  block_cnt;
  int  key_cnt;
}__attribute__((packed)) table_t;

typedef struct kvdb {
  // your definition here
  int fd;
  char filename[128];
}kvdb_t;

#define RED 31
#define GREEN 32
#define YELLOW 33
#define BLUE 34
#define PURPLE 35
#define CYAN 36

#define panic_on(cond, s) \
  do { \
    if (cond) { \
      printf("%s", s); \
      printf(" Line:  %d\n", __LINE__); \
      exit(0); \
    } \
  } while (0)

#define panic(s) panic_on(1, s)

#define c_panic_on(color, cond, s) \
do{ \
    if(cond) {\
        printf("\033[%dm", color); \
        panic_on(cond, s); \
        printf("\033[0m"); \
    }\
}while(0)

#define r_panic_on(cond, s) c_panic_on(RED, cond, s);


#ifdef DEBUG
  #define c_log(color, ...) \
    printf("\033[%dm", color); \
    printf(__VA_ARGS__); \
    printf("\033[0m"); 
#else
  #define c_log(color, ...) \
    ;
#endif

log_t Log;
table_t Table;
void write_table_and_file(int fd, const char *key, const char *value);

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

void read_hdr(kvdb_t *db){
  lseek(db->fd, 0, SEEK_SET);
  read(db->fd, &Log, sizeof(log_t));
  lseek(db->fd, 17*MB, SEEK_SET);
  read(db->fd, &Table, sizeof(table_t));  
}

int find_key(const char *key){
  for (int i = 0; i < Table.key_cnt; i++)
  {
    if(!strcmp(key, Table.key[i])) return i;
  }
  return -1;
}// return key number 

void fsck(kvdb_t *db){
  read_hdr(db);
  if(Log.commit == 0) return ; // Log already commited.
  if(Log.commit == 1){ // Log didn't commit. Maintenance needed.
    c_log(GREEN, "+++ Doing fsck for : ");
    c_log(CYAN, "[%s]\n", db->filename);
    write_table_and_file(db->fd, Log.key, Log.data);
    c_log(GREEN, "+++ Repair finished!\n");
  }
}

void write_fd(int fd, const void *buf, off_t offset, int len){
  lseek(fd, offset, SEEK_SET);
  write(fd, buf, len);
}

void write_log(int fd, const char *key, const char *value){
  int len = strlen(value);
  int key_id = find_key(key);
  Log.nr_block = (len / BLOCKSZ) + 1; // data size
  Log.cur_key_id = (key_id == -1) ? Table.key_cnt : key_id; 
  strcpy(Log.key, key);
  strcpy(Log.data, value);
  Log.commit = 1; // finish writing log
  write_fd(fd, &Log, 0, LOG_HDR + len);
  fsync(fd);
}

void write_table_and_file(int fd, const char *key, const char *value){
  int key_id = Log.cur_key_id;
  int len = strlen(value) + 1;
  if( key_id != -1){
    //case 1: key.len (old)>= Log.nr_block(new)
    //case 2: key.len (old)< Log.nr_block(new)
    if(Log.nr_block <= Table.len[key_id]){
      Table.len[key_id] = Log.nr_block;
      write_fd(fd, &Table, 17*MB, 1*MB); // write in table
      write_fd(fd, value, Table.start[key_id], strlen(value)+1);
      Log.commit = 0;
    }
    else{
      Table.len[key_id] = Log.nr_block;
      Table.start[key_id] = DATA_START + Table.block_cnt * BLOCKSZ;
      Table.block_cnt += Log.nr_block;
      write_fd(fd, &Table, 17*MB, 1*MB); // write in table
      write_fd(fd, value, RSVDSZ + (Table.block_cnt- Log.nr_block)*BLOCKSZ, strlen(value)+1);
      Log.commit = 0;
    }
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
    write_fd(fd, &Table, 17*MB, 1*MB); // write in table
    write_fd(fd, value, RSVDSZ + (Table.block_cnt- Log.nr_block)*BLOCKSZ, strlen(value)+1);
    Log.commit = 0;
  }
}

void write_hdr(int fd, const char *key, const char *value){
  write_log(fd, key, value);
  write_table_and_file(fd, key, value);
  write_fd(fd, &Log, 0, sizeof(int));
  fsync(fd);
}

int kvdb_put(struct kvdb *db, const char *key, const char *value) {
  flock(db->fd, LOCK_EX);
  fsck(db);
  write_hdr(db->fd, key, value);
  flock(db->fd, LOCK_UN);
  return 0; 
}

char *kvdb_get(struct kvdb *db, const char *key) {
  flock(db->fd, LOCK_EX);
  fsck(db);
  // read the data
  int key_id = find_key(key);

  if(key_id == -1){
    c_log(RED, "didn't find key\n");
    flock(db->fd, LOCK_UN);
    return NULL;
  }
  char *ret = malloc(Table.len[key_id] * BLOCKSZ);
  //printf("key: %d, start: %zx, len: %d\n", key_id, Table.start[key_id], Table.len[key_id]);
  lseek(db->fd, Table.start[key_id], SEEK_SET);
  int bytes= read(db->fd, ret, Table.len[key_id]*BLOCKSZ);
  //int length = printf("%s\n", ret);
  //printf("len: %d\n", length);
  c_log(GREEN, "bytes : %d\n", bytes);

  flock(db->fd, LOCK_UN);
  return ret;
}
