#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h> 
#include <ctype.h>       
#include <assert.h>
#include <sys/wait.h>
#define FUNC 1
#define EXPR 0

#if __x86_64__ 
  char elf_class[32] = "-m64";
#else
  char elf_class[32] = "-m32";
#endif

char temp_file[64] = "/tmp/temp_XXXXXX";
char src[256];
char dest[256];
char *CFLAGS[] = {"gcc", "-shared", "-fPIC", "-w", "-x", "c", elf_class, "-o", dest, src, NULL}; //file name can't be ended with .c
void *handle = NULL;
int flag = -1;

char* trim(char* str){
  if(!str) return NULL;
  char *p_s = str;
  char *p_t = str + strlen(str) - 1;
  while(*p_s && isspace(*p_s)) p_s++;
  while((p_t >= p_s) && isspace(*p_t)) *p_t = '\0';
  return p_s;
}

void compile(){
  int cpid = fork();
  if(cpid == 0){ //child -->compile
    execvp(CFLAGS[0], CFLAGS);
    assert(0);
  }
  else{ //parent -->link
    wait(NULL); //wait for child 
    printf("dest is :%s\n", dest);
    handle = dlopen(dest, RTLD_LAZY | RTLD_GLOBAL);
    if (!handle) {
     fprintf(stderr, "%s\n", dlerror());
     return ;
    }
    else{
      printf("link success!\n");
    }
    
  }
}

int main(int argc, char *argv[]) {
  static char line[4096];
  
  int func_cnt = 0;
  char expr_func_name[256];

  while (1) {
    printf("crepl> ");
    fflush(stdout);
    if (fgets(line, sizeof(line), stdin) == NULL) {
      break;
    }
    strcpy(temp_file, "/tmp/temp_XXXXXX");
    int fd = mkstemp(temp_file);
    assert(fd);
    flag = -1;
    handle = NULL;
    
    memset(src, 0, sizeof(src));
    memset(dest, 0, sizeof(dest));
    strcpy(src, temp_file);
    strcpy(dest, temp_file);
    strcat(dest, ".so");
    printf("fd : %d, name: %s\n", fd, temp_file);
    char *cmd;
    cmd = trim(line);
    func_cnt ++;
    if(strncmp(cmd, "int", 3)==0){
      flag = FUNC;
      dprintf(fd, "%s", cmd); // 成功写入
      printf("src: %s, dest: %s\n", src, dest);
    }
    else{
      flag = EXPR;
      printf("this is a expr\n");
      // calculate;
      dprintf(fd, "int __expr_wrapper_%d(){return %s;}", func_cnt, cmd);
      memset(expr_func_name, 0, sizeof(expr_func_name));
      sprintf(expr_func_name, "__expr_wrapper_%d", func_cnt);
      close(fd);
    }
    compile();

    if(flag == FUNC){
      printf("OK\n");
    }
    else{
      if(handle){
        int (*cal)() = dlsym(handle, expr_func_name);
        printf("out[%d]: %d\n", func_cnt, cal());
      }
      else{
        printf("Wrong command\n");
      }
    }
  }
}
