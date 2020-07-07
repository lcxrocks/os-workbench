// #include<stdio.h>
// #include<dirent.h>
// #include<string.h>
// typedef struct pro_info
// {
//     int pid;
//     int ppid;
//     char name[100];
//     int flag;//标志是否打印
//     int rec;//计算总父进程个数
// }info;
// //返回所有数字目录
// int filter(const struct dirent *dir){  //select number folder
//  int i;
//  int n = strlen(dir->d_name);
//  for(i = 0;i<n;i++)
//  {
//      if(!isdigit(dir->d_name[i]))//返回所有数字目录
//    return 0;
//      else return 1;
//  }
// }
// //得到pid
// int my_getpid(char * str)
// {
//     int length=strlen(str);
//     char num[10];
//     int i,j,ret;
//     if(strncmp(str,"Pid",3)==0)
//     {
//  for(i=0;i<length;i++)
//  {
//      if(str[i]>='0'&&str[i]<='9')
//   break;
//  }//获得str[i]中第一个数字位的i值
//  for(j=0;j<length-1;j++)
//      num[j]=str[i+j];//讲数字位复制于num
//  ret=atoi(num);//atoi函数讲string类型转换为int类型
//     }
//     else ret=-1;//由于init的父进程为0 为了避免重复 我们返回－1
//     return ret;//返回pid的值
// }
// //my_getppid()函数原理同my_getpid()原理
// int my_getppid(char *str){
//  int len=strlen(str);
//  char num[10];
//  int i,j,ret;
//  if(strncmp(str,"PPid",4)==0){
//   //printf("%s",str);
//   for(i=0;i<len;i++){
//    if(str[i]>='0'&&str[i]<='9')
//     break;
//   }
//   //printf("len=%d,i=%d/n",len,i);
//   for(j=0;j<len-i;j++){
//    num[j]=str[i+j];
//   }
//   ret=atoi(num);
//   //printf("ret=%d/n",ret);
//  }
//  else ret=-1;
//  return ret;//返回ppid的值
// }

// //打印数
// void print_tree(info *proc,int total,int ppid,int rec)
// {
//     int i,j,k;
//     for(i=0;i<total;i++)
//     {
//  if(proc[i].flag==0&&proc[i].ppid==ppid)
//  {
//      proc[i].rec=rec+1;//判断其父进程有几个
//      proc[i].flag=1;
//      for(k=0;k<rec;k++)
//   printf("   ");
//      if(proc[i].pid>0)
//      printf("├──%s(%d)/n",proc[i].name,proc[i].pid);
//      print_tree(proc,total,proc[i].pid,proc[i].rec);
//  }
//     }
// }
// //main
// int main()
// {
//     struct dirent **namelist;
//     int k,ppid,pid,s1,s2,j;
//     char pid_path[20],str[100],name[100];
//     info proc[1024];
//     int i=0,t=0;
//     FILE *fp;
//     int total=scandir("/proc",&namelist,filter,alphasort);//调用filter选择目录名为数字的目录
//     if(total<0)
//     {
//  printf("scandir erorr !!!");
//     }
//     else printf("共有进程:%d/n",total);
//     printf("===================================/n");
//     for(i=0;i<total;i++)
//     {
//  strcpy(pid_path,"/proc/");
//  strcat(pid_path,namelist[i]->d_name);
//  strcat(pid_path,"/status");
//  fp=fopen(pid_path,"r");
//  while(!feof(fp))//feof()函数读取文件，如果文件到达结尾,就返回非0值 
//  {
//      fgets(str,1024,fp);
//      if((s1=my_getpid(str))!=-1)
//   pid=s1;
//      if((s2=my_getppid(str))!=-1)
//   ppid=s2;
//      if(strncmp(str,"Name",4)==0)//原理于my_getppid()相同
//      {
//   for(j=4;j<strlen(str);j++)
//   {
//       if(str[j]>='a'&&str[j]<='z')
//    break;
//   }
//   for(k=j;k<strlen(str);k++)
//   {
//       name[k-j]=str[k];
//   }
//   name[k-j-1]='/0';
//      }
//      //将得到的赋值于struct
//      proc[t].pid=pid;
//      proc[t].ppid=ppid;
//      strcpy(proc[t].name,name);
    
//  }
//  fclose(fp);//关闭文件
//  //printf("%d %d %s/n",proc[t].ppid,proc[t].pid,proc[t].name);
//  t++;
//     }
//     memset(&proc->flag,0,total);//将proc.flag的所有字节初始化为0
//     memset(&proc->rec,0,total); 
//     print_tree(proc,total,0,0);//打印进程树
// }