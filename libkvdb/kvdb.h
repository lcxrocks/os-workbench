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

#define KB 1024
#define MB (1024 * 1024)
#define KEYLEN 128
#define BLOCKSZ 4096
#define DATALEN (4096*BLOCKSZ)
#define LOG_HDR (4*sizeof(int) + KEYLEN*sizeof(char))
#define KEYNUM 4096 // For one table 
#define RSVDSZ 18*MB
#define DATA_START RSVDSZ

typedef struct __log{
  int commit;
  int TxB; // TxB = 1: Begin writing.
  int TxE; // TxB = 1: End writing.
  int nr_block; // how long is the new value
  char key[KEYLEN];
  char data[DATALEN];
}__attribute__((packed)) log_t;

typedef struct key_table{
  char key[KEYNUM][KEYLEN];
  intptr_t  start[KEYNUM];
  intptr_t  len[KEYNUM];
  //struct table *next;
  int  block_cnt;
  int  key_cnt;
}__attribute__((packed)) table_t;

typedef struct kvdb {
  // your definition here
  int fd;
  char filename[128];
}kvdb_t;

struct kvdb *kvdb_open(const char *filename);
int kvdb_close(struct kvdb *db);
int kvdb_put(struct kvdb *db, const char *key, const char *value);
char *kvdb_get(struct kvdb *db, const char *key);

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

#define c_log(color, ...) \
    printf("\033[%dm", color); \
    printf(__VA_ARGS__); \
    printf("\033[0m"); 
