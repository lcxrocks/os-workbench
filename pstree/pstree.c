
/*                                 WARNING!!
  This lab is not as difficult as you think. For instance, I rewrote this code
  in only three and a half hours from scratch. I believe you can be better than 
  this. So my advice: close this page and try to implement this by yourself. :)

  KEYWORDS: <dirent.h> , recursion                                

  This version is rewritten by lcx and himself only with honesty and integrity. 
  The reason why I **rewrote** this is because I accidentally 'borrowed' some 
  code from the Internet. -> And that is what we call 'plagiarism'. 
  
  So **only** use this code as inspiration when you **completely** have no idea 
  about what to do. Though your code will be retracted like mine :D
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <getopt.h>
#include <dirent.h>     
#include <ctype.h>

#define P_FLAG 0x1
#define N_FLAG 0x2
#define V_FLAG 0x4
#define MAX_SUBP 128
static const char* short_opts = "pnV";
static const struct option long_opts[]=
{
   {"show-pids",no_argument,NULL,'p'},
   {"numeric-sort",no_argument,NULL,'n'},
   {"version",no_argument,NULL,'V'},
   {NULL,0,NULL,0} //最后一个元素应该全为0
};

struct node {
  char name[256];
  int pid;
  int ppid;
  struct node *next;
  struct node *parent;
  struct node *children[MAX_SUBP];
};

struct node * head = NULL;
struct node * root;

char* trim(char* str){
  if(!str) return NULL;
  char *p_s = str;
  char *p_t = str + strlen(str) - 1;
  while(*p_s && isspace(*p_s)) p_s++;
  while((p_t >= p_s) && isspace(*p_t)) *p_t = '\0';
  return p_s;
}

void Add_node(char *dirname){
  char path_name[256];
  char *key;
  char *value;
  strcpy(path_name, "/proc/");
  strcat(path_name, dirname);
  strcat(path_name, "/status");
  if(isdigit(path_name[6])){
    FILE *fp = fopen(path_name, "r");
    struct node *proc = malloc(sizeof(struct node));
    assert(proc);
    char tmp[256];
    while (fgets(tmp,256,fp)!=NULL)
    {
      key = trim(strtok(tmp,":"));
      value = trim(strtok(NULL,":"));
      if(strcmp(key, "Name") == 0) strcpy(proc->name, value);
      else if(strcmp(key, "Pid") == 0) proc->pid = atoi(value);
      else if(strcmp(key, "PPid") == 0) proc->ppid = atoi(value);
      proc->parent = NULL;
      for (int i = 0; i < MAX_SUBP; i++)
        proc->children[i] = NULL;
    }
    fclose(fp);
    if(head == NULL) head = proc;
    else{
      struct node * ptr = head;
      while(ptr->next != NULL) ptr=ptr->next; // -n by default :P
      ptr->next = proc;
      proc->next = NULL;  
    }
  }
}

struct node *locate_parent(int ppid){
  struct node *ptr = head;
  while(ptr->pid != ppid){
    if(ptr == NULL) return NULL;
    else ptr = ptr->next;
  }
  return ptr;
}

void init_root(){
  root = (struct node *)malloc(sizeof(struct node));
  strcpy(root->name, "root");
  for(int i=0;i<MAX_SUBP;i++) root->children[i] = NULL;
}

void make_tree(){
  init_root();
  struct node *p = head; 
  while(p){
    if(p->ppid){
      struct node * p_parent = locate_parent(p->ppid);
      int i = 0;
      while(p_parent->children[i]!=NULL) i++;
      p_parent->children[i] = p;
    }
    else if(p->ppid == 0){
      int i = 0;
      while(root->children[i]!=NULL) i++;
      root->children[i] = p;
    }
    p = p->next;
  }
}

void print_ll(){
  struct node *p = head;
  while(p)
  {
    printf("%d -> ", p->pid);
    p = p->next;
  }
}

void brk_pt(){
  return ;
};

int print_tree(struct node *p, int level, int flag){
  int i = 0;
  if(flag & V_FLAG){
    fprintf(stderr,"pstree --version 2.0. Made all by lcx himself and himself only.\n");
    return 0;
  }
  else if(level == -1) //root
  {
    while(root->children[i]!=NULL) {
      print_tree(root->children[i], level+1, flag);
      i++;
    }
  }
  else
  {
    for (int i = 0; i < level; i++) printf("\t");
    if(flag & P_FLAG) 
    {
      if(level > 0) printf("\\___%s (Pid: %d)\n", p->name, p->pid);
      else printf("%s (Pid: %d)\n", p->name, p->pid);
    }
    else
    {
      if(level > 0) printf("\\___%s \n", p->name);
      else printf("%s \n", p->name);
    }
    while((p->children[i]!=NULL)){
      print_tree(p->children[i], level+1, flag);
      i++;
    }
  }
  return 0;
}

int main(int argc, char *argv[]) {
  int opt = 0;
  int flag = 0;
  DIR* dir;
  struct dirent *ptr;
  while(((opt = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1))
  {
    switch(opt){
      case 'p':  flag |= P_FLAG; break;
      case 'n':  flag |= N_FLAG; break;
      case 'V':  flag |= V_FLAG; break;
      //default: printf("Invalid options! Should not reach **switch end**\n");
    }
  }
  dir = opendir("/proc");
  assert(dir);
  while((ptr=readdir(dir))!=NULL){
    Add_node(ptr->d_name);
  }
  make_tree();
  print_tree(root,-1,flag);
  return 0;
}
