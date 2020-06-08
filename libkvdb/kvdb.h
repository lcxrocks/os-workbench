#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/file.h>
#include <sys/types.h>    
#include <sys/stat.h>    
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define MB (1024 * 1024)
#define KEYLEN 128
#define BLOCKSZ 4096
#define DATALEN (4096*BLOCKSZ)
#define LOG_HDR (3*sizeof(int) + 128*sizeof(char))
#define PADSZ (1*MB - LOG_HDR) 
#define RSVDSZ 17*MB

typedef struct log{
  int commit;
  int TxB; // TxB = 1: Begin writing.
  int TxE; // TxB = 1: End writing.
  char key[KEYLEN];
  char padding[PADSZ];
}__attribute__((packed)) log_t;

struct kvdb;
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

#define c_log(color, ...) \
    printf("\033[%dm", color); \
    printf(__VA_ARGS__); \
    printf("\033[0m"); 
