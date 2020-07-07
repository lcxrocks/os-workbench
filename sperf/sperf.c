#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>    
#include <sys/wait.h>
#include <sys/stat.h>    
#include <fcntl.h>
#include <regex.h>
#include <time.h>
#include <stdint.h>

#define BUF_SIZE 4096
#define Log(s) \
  printf("\n\033[31m %s \033[0m\n",s);

extern char **environ;

char strace_path[BUF_SIZE];
char mypath[BUF_SIZE];
char str1[BUF_SIZE];
char str2[BUF_SIZE];

regex_t strtime;
regex_t name;
regex_t strexit;
int cnt;
time_t st_time;
time_t ed_time;

typedef struct _sys_call{
  char name[128];
  struct _sys_call *prev;
  struct _sys_call *next;
  double time;
}sys_call;

sys_call *head = NULL;

void init_str1_str2(){
  strcpy(str1, "find ");
  strcpy(str2, " -name strace");
}

void get_mypath(){
  FILE *echo_path = NULL;
  echo_path = popen("echo $PATH", "r");
  fgets(mypath, BUF_SIZE, echo_path);
  //printf("$PATH: %s\n", mypath);
  pclose(echo_path);
}

char *cat_path(char *prestr, char *path, char *poststr){
  strcat(prestr, path);
  strcat(prestr, poststr);
  return prestr;
}

void get_strace_path(){
  char *s_path = strtok(mypath, ":");
  char *find_strace = NULL;
  find_strace = cat_path(str1, s_path, str2);
  FILE *p_file = NULL;
  p_file = popen(find_strace,"r");
  while(fgets(strace_path, BUF_SIZE, p_file)==NULL){ //didn't get path
    pclose(p_file);
    memset(str1,0,sizeof(str1));
    strcpy(str1, "find ");
    // char tmp[256] = ;
    // memset(tmp, 0, sizeof(tmp));
    char *tmp = strtok(NULL, ":");
    //printf("tmp: %c\n", tmp[strlen(tmp)-1]);
    if(tmp[strlen(tmp)-1] == '\n') tmp[strlen(tmp)-1] = '\0';
    find_strace = cat_path(str1, tmp, str2);
    //printf("trying path: %s\n", find_strace);
    p_file = popen(find_strace,"r");
  }
  //printf("found path : %s\n",strace_path);
  
  pclose(p_file);
  strace_path[strlen(strace_path)-1] = '\0';
}

void brkc(){
  return ;
}

void swap(sys_call *top, sys_call *mid, sys_call *bot){ //swap mid and bot position
  if(top!=NULL) top->next = bot;
  bot->prev = top;
  mid->next = bot->next;
  if(bot->next) bot->next->prev = mid;
  bot->next = mid;
  mid->prev = bot;
  if(mid == head) head = bot;
}

void bubble_sort(){
  for (int i = 0; i < cnt; i++)
  {
    //printf("Sorting for the (%d/%d)...\n", i, cnt);
    for (sys_call *ptr = head; ptr->next!=NULL; )
    {
      //printf("ptr is %s, time: %lf\n", ptr->name, ptr->time);
      if(ptr->time < ptr->next->time) {
        //printf("swapped (%s, %s)\n", ptr->name, ptr->prev->name);
        swap(ptr->prev, ptr, ptr->next);
        //if(ptr->prev!=NULL) printf("swapped (%s, %s)\n", ptr->name, ptr->prev->name);
      }
      else ptr = ptr->next;
    }
  }
  return ;
}

void display(){
  double time = 0; 
  for (sys_call *ptr = head; ptr != NULL ; ptr = ptr->next)
  {
    time += ptr->time;
  }
  for (sys_call *ptr = head; ptr != NULL ; ptr = ptr->next)
  {
    printf("%s (%lf%%)\n", ptr->name, (ptr->time/time)*100);
  }
  printf("==============================\n");
  for (int i = 0; i < 80; i++)
  {
    printf("%c",'\0');
  }
  fflush(stdout);
  fflush(stderr);
}

int main(int argc, char *argv[]) {
  init_str1_str2(); 
  get_mypath();
  get_strace_path();
  cnt = 0;
  regcomp(&strtime, "<[0-9].[0-9]*>", REG_EXTENDED);
  regcomp(&name, "([a-zA-Z0-9]+_*)+\\(", REG_EXTENDED);
  regcomp(&strexit, "\\+\\+\\+", REG_EXTENDED);
  char *exec_argv[argc+4];
  exec_argv[0] = "strace";
  exec_argv[1] = "-T";
  exec_argv[2] = "-o";
  exec_argv[argc+3] = NULL;
  memcpy(exec_argv+4, argv+1, (argc - 1)* sizeof(char *));
  if(exec_argv[argc+3]!=NULL) assert(0);

  int fildes[2];
  if(pipe(fildes)!=0){
    printf("pipe failed!\n");
    exit(1);
  };
  //用close()和dup()实现dup2()时，可能会发生数据竞争，导致文件描述符被复用
  int rc = fork();
  if(rc < 0){
    fprintf(stderr, "fork failed!\n");
    exit(1);
  }
  else if(rc == 0){
    close(fildes[0]); 
    char tmp[128] = "";
    sprintf(tmp,"/proc/%d/fd/%d", getpid(), fildes[1]);
    exec_argv[3] = tmp;
    int trash = open("/dev/null", O_WRONLY);
    dup2(trash, 1);
    dup2(trash, 2);
    execve(strace_path, exec_argv, environ);
    printf("should not reach here!\n");
    exit(EXIT_FAILURE);
  }
  else{
    close(fildes[1]);
    dup2(fildes[0], 0);

    char buf[BUF_SIZE];
    size_t nmatch = 1;
    regmatch_t match_ptr;
    regmatch_t name_match_ptr;
    regmatch_t exit_match_ptr;
    char s[BUF_SIZE];
    char strname[BUF_SIZE];
    st_time = time(NULL);
    while(fgets(buf, BUF_SIZE, stdin)>0){
      //if(regexec(&time, buf, nmatch, &match_ptr, 0)==REG_NOMATCH) printf("no match time in buf:%s!\n", buf);
      //if(regexec(&name, buf, nmatch, &name_match_ptr, 0)==REG_NOMATCH) printf("no match name in buf: %s!\n",buf);
      if(regexec(&strexit, buf, nmatch, &exit_match_ptr, 0)!=REG_NOMATCH){
        bubble_sort();
        display();
        break;      
      }
      if((regexec(&strtime, buf, nmatch, &match_ptr, 0)!=REG_NOMATCH) \
      && (regexec(&name, buf, nmatch, &name_match_ptr, 0)!=REG_NOMATCH)){
        memset(s, 0, sizeof(s));
        memcpy(s, buf+match_ptr.rm_so+1, match_ptr.rm_eo-2-match_ptr.rm_so);
        memset(strname, 0, sizeof(strname));
        memcpy(strname, buf+name_match_ptr.rm_so, name_match_ptr.rm_eo-name_match_ptr.rm_so-1);
        sys_call *ptr = head;
        while(ptr!=NULL){
          if(strcmp(ptr->name, strname)==0){
            ptr->time += atof(s);
            break;
          }
          ptr = ptr->next;
        }
        if(ptr == NULL){
          sys_call* node = (sys_call *) malloc(sizeof(sys_call));
          strcpy(node->name, strname);
          node->time = atof(s);
          if(head==NULL) {
            head = node;
            head->next = NULL;
            head->prev = NULL;
            cnt++;
          }
          else {
            node->next = head->next;
            node->prev = head;
            if(node->next!=NULL) node->next->prev = node;
            head->next = node;
            cnt++;
          }
        }
      }
      else{
        continue;
      }
      
      ed_time = time(NULL);
      //printf("time : %ld\n", ed_time-st_time);
      if(ed_time - st_time >= 1){
        st_time = ed_time;
        bubble_sort();
        display();
        head = NULL;
        cnt = 0;
        memset(buf, 0, sizeof(buf));
        memset(s, 0, sizeof(s));
        memset(strname, 0, sizeof(strname));
      }
    }
    
    
    // int i = 1;
    // sys_call *p = head;
    // while(p!=NULL){
    //   //printf("i=%d\n", i);
    //   printf("%s : %lf, cnt: (%d/%d)\n", p->name, p->time, i, cnt);
    //   p = p->next;
    //   i++;
    // }

  }
  
  // execve("strace", exec_argv, exec_envp);
  // printf("no way\n");
  // perror(argv[0]);
  // exit(EXIT_FAILURE);
}
