#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/types.h>   
#include <sys/stat.h>    
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#define u1 uint8_t
#define u2 uint16_t
#define u4 uint32_t
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_LONG_NAME_MASK 0x3f
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
    u4 biWidth;/* important */
    u4 biHeight; /* important */
    u2 biPlanes; 
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

typedef struct RGB{
    u1 RGB[3]; //BGR actually (24bit)
}__attribute__((packed)) rgb_t;

char filename[256];
void check_info(int argc);
void print_info(struct fat_header *hd);
int get_cluster_number(struct fat_header *hd);
int classify(void *p);
void dir_handler(void *c);
void short_entry_handler(void *c, void *entry, bool long_name_flag);
void write_image(int fd, image_t * p);
void get_line_rgb(int8_t *prev_line, int size, void *p);
int compare(int8_t *prev_line , int8_t *next_line, int cnt);

int dirent_cnt;
int pic_cnt;
int nr_datasec;
int nr_clus;
int BytsClus;
int RsvdBytes;
int longname_cnt;
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
        for (void *p = cluster_entry; p < end_entry; p += 32)
        {
            fat_dir *dir = p;
            if(((dir->LDIR_Attr & ATTR_LONG_NAME_MASK ) == ATTR_LONG_NAME)&& dir->LDIR_Type == 0 && dir->LDIR_FstClusIO == 0 && (dir->LDIR_Ord & 0x40)==0x40){
                longname_cnt++;
                dir_handler(p);
            }
        }
    }
    
    //3. RECOVER IMAGES
    image_t *p = &list_head;
    int eq_cnt = 0;
    while(p->next){
        p = p->next;
        // char path_name[128] = "../../tmp/";
        // strcat(path_name, p->name);
        // int fd = open(path_name, O_CREAT | O_WRONLY, S_IRWXU);
        // //printf("ERROR: %d\n", errno);
        // //panic_on(fd<0, "Bad fd");
        // write(fd, p->bmp->header, p->size); // 连续的size大小
        write_image(fd, p);
        // char sha1sum[256] = "sha1sum ";
        // strcat(sha1sum, path_name);
        // FILE *fp = popen(sha1sum, "r");
        // //panic_on(!fp, "popen");
        // char buf[256];
        // memset(buf, 0, sizeof(buf));
        // fscanf(fp, "%s", buf); // Get it!
        // /***check***/
        //     char mnt_path[128] = "/mnt/DCIM/";
        //     strcat(mnt_path, p->name);
        //     char sha[256] = "sha1sum ";
        //     strcat(sha, mnt_path); 
        //     FILE *fp1 = popen(sha, "r");
        //     char tmp[256];
        //     memset(tmp, 0, sizeof(tmp));
        //     fscanf(fp1, "%s", tmp);
        //     if(!strcmp(buf, tmp)) eq_cnt++;
        // /***check****/
        // pclose(fp);
        // printf("%s %s\n", buf, p->name);
    }
    printf("================================================================\n");
    printf("dirent : %d\n", dirent_cnt);
    printf("pic cnt: %d\n", pic_cnt);
    printf("ln_cnt : %d\n", longname_cnt);
    printf("success: %d\n", eq_cnt);
    printf("================================================================\n");

}

void dir_handler(void *c){
    fat_dir *d = c;
    int num = d->LDIR_Ord & 0xf;
    int tmp = num;
    while(tmp > 1){
        tmp--;
        d = (void *)d + 32;
        if((d->LDIR_Ord &0xf)!=tmp){
            longname_cnt--;
            return ;
        }
    } // d now is at 1st long entry

    image_t *pic = malloc(sizeof(image_t));
    pic->next = list_head.next;
    pic->prev = &list_head;
    if(list_head.next!=NULL)
        list_head.next->prev = pic;
    list_head.next = pic;

    int pos = 0;
    bool break_flag = false;
    for (int i = 0; i < num; i++)
    {
        for (int j = 0; j < 10; j = j+2){
            pic->name[pos++] = d->LDIR_Name1[j];
            if(d->LDIR_Name1[j] == '\0') {
               break_flag = true;
               break;  
            }
        }
        if(break_flag) break;
        for (int j = 0; j < 12; j = j+2){
            pic->name[pos++] = d->LDIR_Name2[j];
            if(d->LDIR_Name2[j] == '\0'){
               break_flag = true;
               break;  
            }
        }
        if(break_flag) break;
        for (int j = 0; j < 4; j = j+2){
            pic->name[pos++] = d->LDIR_Name3[j];
            if(d->LDIR_Name3[j] == '\0'){
               break_flag = true;
               break;  
            }
        }
        if(break_flag) break;
        d = (void *)d - 32;
    }
    if(pic->name[pos] != '\0') pic->name[pos] = '\0';
    d = (void *)c + num * 32;
    pic->clus_idx = (d->DIR_FstClusHI << 16) | d->DIR_FstClusLO;
    pic->bmp = malloc(sizeof(bmp_t));
    pic->bmp->header = disk->data + (pic->clus_idx - 2) * disk->header->BPB_SecPerClus * disk->header->BPB_BytsPerSec;
    pic->bmp->info = (void *)pic->bmp->header + 14; 
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
    int w = ptr->bmp->info->biWidth;
    int h = ptr->bmp->info->biHeight; // default: 24bit bmp file
    int offset = ptr->bmp->header->bfOffBits;
    int bytesPerLine=((w*24+31)>>5)<<2;
    int imageSize=bytesPerLine*h;
    int skip=4-(((w*24)>>3)&3);
    int size = ptr->size;
    int num = size / BytsClus; // total number of clusters
    
    printf("offset: %d , imageSize :%d == %d, skip: %d\n",offset, imageSize, ptr->bmp->info->biSizeImages, skip);
    printf("%lf\n", ((double) w*3+skip)/4);
    printf("%d\n", ptr->size);

    void *p = ptr->bmp->header;
    void *t = disk->data;
    write(fd, p, BytsClus); 
    p += BytsClus; num--;// first byte
    int lseek = BytsClus - offset;
    int x = lseek % (w*3+skip); //rest line 
    int y = lseek / (w*3+skip); 
    printf("lseek - x :%d == %d, %d\n", lseek-x, (lseek-x)/(w*3+skip)*(w*3+skip), y*(w*3+skip));
    printf("p-x:%d\n", BytsClus - 54 -x);
    int sum = 0;
    uint8_t *prev_line= calloc(x, sizeof(uint8_t));
    uint8_t *next_line= calloc(x, sizeof(uint8_t));
    //memcpy(prev_line, p-x, x);

    // while(sum > x*){
        
    //     for (int i = 0; i < x; i++)
    //     {
    //         prev_line[]
    //     }
        
    // }
    // if(p+)

    //int8_t  *next_line = (int8_t *) calloc(h, sizeof(uint32_t)); // R G B

    
    
    //printf("\033[32m >>File: \033[0m \033[33m%s \033[0m\033[32mhas %d clusters to write.\033[0m\n", ptr->name, num);
    // write(fd, p, (ptr->size < BytsClus ? ptr->size : BytsClus)); num--; size -= BytsClus;// first cluster
    // memcpy(prev_line, p+BytsClus-3*w, 3*w); p += BytsClus;
    // memcpy(next_line, p, 3*w);

    // free(prev_line);
    // free(next_line);
};

int compare(int8_t *prev_line , int8_t *next_line, int cnt){
    int sum = 0;
    for (int i = 0; i < cnt; i++){
        sum += (prev_line[i] - next_line[i] > 0) ? prev_line[i] - next_line[i] : next_line[i] - prev_line[i];
    }
    return sum;
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