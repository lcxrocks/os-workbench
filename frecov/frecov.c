#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/types.h>   
#include <sys/stat.h>    
#include <fcntl.h>
#include <unistd.h>
#define u1 uint8_t
#define u2 uint16_t
#define u4 uint32_t
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_LONG_NAME 0x0f //0x01 | 0x02 | 0x04 | 0x08

#define panic_on(cond, s) \
  do { \
    if (cond) { \
      printf("%s\n", s); \
      exit(1); \
    } \
  } while (0)

#define panic(s) panic_on(1, s)

enum{DIRENT = 0, BMP_DATA, UNUSED, EMPTY}; // states of clusters
// 1. PIC DEF
typedef struct bmp_header{
    char bfType[2];
    u4 bfSize;
    u2 bfReserved1;
    u2 bfReserved2;
    u4 bfOffBits; /* important */
}__attribute__((packed)) bmp_header_t;

typedef struct bmp_info{
    u4 biSize;
    u4 biWidth;
    u4 biHeight; /* important */
    u2 biPlanes; /* important */
    u2 biBitCount;
    u4 biCompression;
    u4 biSizeImages;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    u4 biClrUsed;
    u4 biClrImportant;
}__attribute__((packed)) bmp_info_t;

typedef struct bmp{
    struct bmp_header *header;
    struct bmp_info *info;
    void *bmp_data;
}__attribute__((packed)) bmp_t; //picture type

typedef struct bmp_file_t{
    char name[128];
    bmp_t *bmp;
    int size;
    int clus_idx;
    struct bmp_file_t *prev;
    struct bmp_file_t *next;
}image_t;

image_t list_head;

//2. FAT DEFs
struct fat_header{
    //Common BPB structure: 
    uint8_t  BS_jmpBoot[3];
    char     BS_OEMNname[8];
    uint16_t BPB_BytsPerSec;
    uint8_t  BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t  BPB_NumFATs; 
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16; 
    uint8_t  BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;
    // Extened BPB structure for FAT32:
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    uint8_t  BPB_Reserved[12]; //set to 0x0
    uint8_t  BS_DrvNum;
    uint8_t  BS_Reserved1; //set to 0x0
    uint8_t  BS_BootSig; 
    uint32_t BS_VolID;
    char     BS_VolLab[11];
    char     BS_FilSysType[8]; // Set to the string "FAT32"
    uint8_t  padding[420];
    uint16_t Signature_word; // 0x55(offset 510); 0xAA(offset 511) 
}__attribute__((packed));

struct FSInfo{
    u4 FSI_LeadSig; // 0x41615252
    u1 FSI_Reserved[480]; // set to 0x0
    u4 FSI_StrucSig; //0x61417272
    u4 FSI_Free_Count; 
    u4 FSI_Nxt_Free; // next free cluster number 
    u1 FSI_Reserved2[12]; // set to 0x0
    u4 FSI_TrailSig; // 0xAA550000
}__attribute__((packed));

typedef union Directory{
    struct{
        char DIR_Name[11]; // short name;
        u1 DIR_Attr; // file attribute type;
        u1 DIR_NTRes; // 0x0
        u1 DIR_CrtTimeTenth; // count of tenths of a second. Valid range: [0,199]
        u2 DIR_CrtTime; // Creation time. Granularity is 2 seconds.
        u2 DIR_CrtDate; // Creation date.
        u2 DIR_LstAccDate; // Last Access date.
        u2 DIR_FstClusHI; //High word of first data cluster number for file/directory described by this entry.
        u2 DIR_WrtTime; //Last modification time;
        u2 DIR_WrtDate; //Last modification date;
        u2 DIR_FstClusLO; //Low word of first data cluster number ...
        u4 DIR_FileSize; //32-bit quantity containing size in bytes 
    };// short name
    struct{
        u1 LDIR_Ord; //long name entry 的标号；其中最后一个long name entry需要在前面pad上0x40
        char LDIR_Name1[10];
        u1 LDIR_Attr;
        u1 LDIR_Type; //0x0
        u1 LDIR_Chksum;
        char LDIR_Name2[12];
        u2 LDIR_FstClusIO; //0x0
        char LDIR_Name3[4];
    };// long name 
}__attribute__((packed)) fat_dir;

typedef struct FAT{
    struct fat_header *header; //512B
    struct FSInfo *fs_info;
    void *data;
    void *end;
}fat; 

char filename[256];
void check_info(int argc);
void print_info(struct fat_header *hd);
int get_cluster_number(struct fat_header *hd);
int classify(void *p);
void dir_handler(void *c);
void short_entry_handler(void *c, void *entry, bool long_name_flag);
void write_image(int fd, image_t * p);
void get_line_rgb(int8_t *digit, int size, void *p);
int compare(int8_t *digit , int8_t *digit_new, int last_line);

int unused_cnt;
int bmp_data_cnt;
int bmp_header_cnt;
int empty_cnt;
int long_name_cnt;
int dirent_cnt;
int pic_cnt;
int nr_datasec;
int nr_clus;
int BytsClus;
int RsvdBytes;
fat *disk;

int main(int argc, char *argv[]) {
    /* Check_basic info */
    check_info(argc);
    list_head.next = NULL;
    list_head.prev = NULL;
    list_head.size = 0;
    strcpy(filename, argv[1]);
//1. LOAD DISK
    /*---------------------------------------/ 
            1.   load the disk 
    ----------------------------------------*/

    int fd = open(filename, O_RDONLY);
    struct stat s;
    panic_on(fstat(fd, &s)==-1, "fstat failed!\n");

    disk = (fat *)malloc(sizeof(fat));

    disk->header = (struct fat_header *)mmap(NULL, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0); //DONT USE SHARED
    panic_on(disk->header->Signature_word!=0xAA55, "BAD Signature\n");    

    disk->fs_info = (struct FSInfo *)((void *)disk->header + 512);
    panic_on(disk->fs_info->FSI_TrailSig!=0xAA550000, "BAD TrailSig\n");

    nr_datasec = disk->header->BPB_TotSec32 - (disk->header->BPB_RsvdSecCnt + (disk->header->BPB_NumFATs * disk->header->BPB_FATSz32));
    nr_clus = nr_datasec / disk->header->BPB_SecPerClus; 
    BytsClus = disk->header->BPB_BytsPerSec * disk->header->BPB_SecPerClus;
    RsvdBytes = (disk->header->BPB_RsvdSecCnt + disk->header->BPB_NumFATs * disk->header->BPB_FATSz32) * disk->header->BPB_BytsPerSec;
    disk->data = (void *)disk->header + RsvdBytes;
    disk->end = disk->data + BytsClus * nr_clus;
    // on a FAT32 volume, RootDirSectors is always 0.
    printf("\033[32mFinish Loading Disk : \033[0m\033[33m%s\033[0m\n", argv[1]);
    printf("\033[32m------------------------------------------------------------\033[0m\n");
    printf("\033[32mFAT header:    %p\033[0m\n", disk->header);
    printf("\033[32mData zone:     %p\033[0m\n", disk->data);
    printf("\033[32mData clusters: %d\033[0m\n", nr_clus);
    printf("\033[32m------------------------------------------------------------\033[0m\n");
    printf("\n\n\n");
//2. LABEL FILES
    
    void *cluster_entry = disk->data;
    int dir_sz = sizeof(fat_dir); 
    void *ex = NULL;

    void *end_entry = NULL;
    for (int i = 0; i < nr_clus; i++)
    {
        cluster_entry = BytsClus * i + disk->data;
        end_entry = BytsClus * (i+1) + disk->data;
        for (void *p = cluster_entry; p < end_entry; p = p+32)
        {
            fat_dir *dir = p;
            if(dir->DIR_Name[8]=='B' && dir->DIR_Name[9] == 'M' && dir->DIR_Name[10] == 'P'){
                //printf("offset: %zx\n", p - cluster_entry);
                if((dir->DIR_Attr & 0x20) && (dir->DIR_Name[6]=='~')) // make sure this is the long_name_entry
                {
                    dirent_cnt++;
                    dir_handler(p);
                }
            }
        }
    }

    // printf("============================================================\n");
    // printf("cluster num: %d\n", nr_clus);
    // printf("dirent_cnt : %d\n", dirent_cnt);
    // printf("pic_cnt    : %d\n", pic_cnt);
    // printf("============================================================\n");
    /* recover the file*/

    //3. RECOVER IMAGES
    image_t *p = &list_head;
    while(p->next){
        p = p->next;
        //int clu_idx = p->clus_idx;
        //void *sec1 = disk->data + (clu_idx - 2) * disk->header->BPB_SecPerClus * disk->header->BPB_BytsPerSec;
        //p->bmp->header = (bmp_header_t *) sec1;
        //printf("haha\n");
        //p->bmp->info = (bmp_info_t *)(sec1 + 14);
        char path_name[128] = "/~/tmp/";
        strcat(path_name, p->name);
        int fd = open(path_name, O_CREAT | O_WRONLY, S_IRWXU);
        //write(fd, p->bmp->header, p->size); // 连续的size大小
        write_image(fd, p);
        char sha1sum[256] = "sha1sum ";
        strcat(sha1sum, path_name);
        FILE *fp = popen(sha1sum, "r");
        //panic_on(!fp, "popen");
        char buf[256];
        memset(buf, 0, sizeof(buf));
        fscanf(fp, "%s", buf); // Get it!
        pclose(fp);
        printf("%s %s\n", buf, p->name);
    }
}

void dir_handler(void *c){
    image_t *p = malloc(sizeof(image_t));
    fat_dir *d = c;
    int num = 1; 
    while(num==1 || ((d->LDIR_Ord & 0x40 )!= 0x40) ){
        d = (void *)d - 32;
        num++;
    }
    image_t *pic = malloc(sizeof(image_t));
    pic->next = list_head.next;
    pic->prev = &list_head;
    if(list_head.next!=NULL)
        list_head.next->prev = pic;
    list_head.next = pic;
    d = c - 32; //1st long entry
    int pos = 0;
    bool break_flag = false;
    for (int i = 0; i < num; i++)
    {
        for (int i = 0; i < 10; i = i+2){
            pic->name[pos++] = d->LDIR_Name1[i];
            if(d->LDIR_Name1[i] == '\0') {
               break_flag = true;
               break;  
            }
        }
        if(break_flag) break;
        for (int i = 0; i < 12; i = i+2){
            pic->name[pos++] = d->LDIR_Name2[i];
            if(d->LDIR_Name2[i] == '\0'){
               break_flag = true;
               break;  
            }
        }
        if(break_flag) break;
        for (int i = 0; i < 4; i = i+2){
            pic->name[pos++] = d->LDIR_Name3[i];
            if(d->LDIR_Name3[i] == '\0'){
               break_flag = true;
               break;  
            }
        }
        if(break_flag) break;
        d = (void *)d - 32;
    }
    if(pic->name[pos] != '\0') pic->name[pos] = '\0';
    d = c;
    pic->clus_idx = (d->DIR_FstClusHI << 16) | d->DIR_FstClusLO;
    pic->bmp = malloc(sizeof(bmp_t));
    pic->bmp->header = (bmp_header_t *)(disk->data + (pic->clus_idx - 2) * disk->header->BPB_SecPerClus * disk->header->BPB_BytsPerSec);
    pic->bmp->info = (bmp_info_t *)(pic->bmp->header + 14);
    pic->size = d->DIR_FileSize;
    int len = strlen(pic->name);
    
    if(pic->name[len-3] == 'b' && pic->name[len-2] == 'm'  && pic->name[len-1] == 'p' && pic->size != 0){
       pic_cnt++;
        printf("\033[32m>> dectected file name: \033[0m%s , clus_idx :%x, file_size: %d\n", pic->name, pic->clus_idx, pic->size);
    }
    else{
        list_head.next = pic->next;
        if(pic->next!=NULL) pic->next->prev = &list_head;
        free(pic);
    }
    return ;
}

void write_image(int fd, image_t * ptr){
    int size = ptr->size;
    //BytsClus; //一个cluster中的Bytes
    void *p = ptr->bmp->header; //图像的第一个cluster地址
    int num = 0; //number of clusters
    int w = ptr->bmp->info->biWidth; // width
    int h = ptr->bmp->info->biHeight; // height
    int bit = ptr->bmp->info->biBitCount; //bit-map format
    
    int last_line = w * bit / 8 ;  
    int8_t *digit = (int8_t *) malloc(sizeof(int8_t) * last_line);
    int8_t *digit_new = (int8_t *) malloc(sizeof(int8_t) * last_line);
    printf("wtf?\n");
    while(size){
        printf("\033[32mprocessing %s, remain size: %d\033[0m\n", ptr->name, size);
        if(num == 0){ //first cluster        
            if(size >= BytsClus)
                write(fd, p, BytsClus);
            else
                write(fd, p, size);
            p = p + BytsClus - last_line; 
            get_line_rgb(digit, last_line, p);
        }
        else{ //search for the next cluster
            get_line_rgb(digit_new, last_line, p); // 1. check if the next cluster is good
            int sum = compare(digit, digit_new, last_line);
            bool distant_clus = false;
            void *t = disk->data;
            while(sum > last_line * 8 * 10){ //2. find next cluster
                distant_clus = true;    
                printf("\033[31mlocating %s, remain size: %d\033[0m\n", ptr->name, size);
                for (; t < disk->end; t += BytsClus)
                {
                    get_line_rgb(digit_new, last_line, t);
                    sum = compare(digit, digit_new, last_line);
                }
            }
            if(distant_clus)
            {
                if(size >= BytsClus)
                    write(fd, t, BytsClus);
                else
                    write(fd, t, size);
                t = t + BytsClus - last_line; 
                get_line_rgb(digit, last_line, t);
                p += BytsClus; // p skip the following cluster
            }
            else{
                if(size >= BytsClus)
                    write(fd, p, BytsClus);
                else
                    write(fd, p, size);
                p = p + BytsClus - last_line; 
                get_line_rgb(digit, last_line, p);
            }
        }
        num ++;
        size -= BytsClus;
    }
};

int compare(int8_t *digit , int8_t *digit_new, int last_line){
    int sum = 0;
    for (int i = 0; i < last_line; i++){
        sum += (digit[i] - digit_new[i] > 0) ? digit[i] - digit_new[i] : digit_new[i] - digit[i];
    }
    return sum;
}

void get_line_rgb(int8_t *digit, int size, void *p){
    for (int i = 0; i < size; i++){
        digit[i] = *((int8_t *) p);
        p++;
    }
}

void check_info(int argc){
    panic_on(argc == 1, "Usage: ./frecov *.img\n");
    panic_on(sizeof(struct fat_header) != 512, "bad fat header");
    panic_on(sizeof(struct FSInfo) != 512, "bad FSInfo");
    panic_on(sizeof(struct bmp_header) != 14, "bad bmp header");
    panic_on(sizeof(struct bmp_info) != 40, "bad bmp info");
}

void short_entry_handler(void *c, void *entry, bool long_name_flag){
    return ;
}

unsigned char ChkSum(unsigned char *p){
    short FcbNameLen;
    unsigned char Sum = 0;
    for (FcbNameLen = 11; FcbNameLen != 0; FcbNameLen--){
       Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + *p++;
    }
    return Sum;
}   


int FATSz = 0;
int TotSec = 0;
int DataSec = 0;
int get_cluster_number(struct fat_header *hd){
    FATSz = hd->BPB_FATSz32;
    TotSec = hd->BPB_TotSec32;
    DataSec = TotSec - (hd->BPB_RsvdSecCnt + (hd->BPB_NumFATs * FATSz));
    int cnt = DataSec / hd->BPB_SecPerClus ;
    return cnt;
}

#define CHECK(disk, string) \
    printf("%s:\t\t", #string); \
    printf("%d\n", disk->string); 

#define CHECK_HD(string) \
    CHECK(hd, string);

void print_info(struct fat_header *hd){
    CHECK_HD(BPB_BytsPerSec);
    CHECK_HD(BPB_SecPerClus);
    CHECK_HD(BPB_RsvdSecCnt);
    CHECK_HD(BPB_NumFATs);
    CHECK_HD(BPB_Media);
    CHECK_HD(BPB_TotSec32);
    // // Extened BPB structure for FAT32:
    CHECK_HD(BPB_FATSz32); //32-bit count of sectors occupied by one FAT
    CHECK_HD(BPB_RootClus); // sec num
    CHECK_HD(BPB_FSInfo); //sec num
}