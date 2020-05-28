#include "frecov.h"

char filename[256];
void check_info(int argc);
void print_info(struct fat_header *hd);
int get_cluster_number(struct fat_header *hd);
int classify(void *p);
void dir_handler(void *c);
void short_entry_handler(void *c, void *entry, bool long_name_flag);

int unused_cnt;
int bmp_data_cnt;
int bmp_header_cnt;
int empty_cnt;
int long_name_cnt;
int dirent_cnt;
int pic_cnt;
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

    fat * disk = (fat *)malloc(sizeof(fat));

    disk->header = (struct fat_header *)mmap(NULL, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0); //DONT USE SHARED
    panic_on(disk->header->Signature_word!=0xAA55, "BAD Signature\n");    

    disk->fs_info = (struct FSInfo *)((void *)disk->header + 512);
    panic_on(disk->fs_info->FSI_TrailSig!=0xAA550000, "BAD TrailSig\n");

    int nr_datasec = disk->header->BPB_TotSec32 - (disk->header->BPB_RsvdSecCnt + (disk->header->BPB_NumFATs * disk->header->BPB_FATSz32));
    int nr_clus = nr_datasec / disk->header->BPB_SecPerClus; 
    int BytsClus = disk->header->BPB_BytsPerSec * disk->header->BPB_SecPerClus;
    int RsvdBytes = (disk->header->BPB_RsvdSecCnt + disk->header->BPB_NumFATs * disk->header->BPB_FATSz32) * disk->header->BPB_BytsPerSec;
    disk->data = (void *)disk->header + RsvdBytes;
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

    printf("============================================================\n");
    printf("cluster num: %d\n", nr_clus);
    printf("dirent_cnt : %d\n", dirent_cnt);
    printf("pic_cnt    : %d\n", pic_cnt);
    printf("============================================================\n");
    /* recover the file*/

    //3. RECOVER IMAGES
    image_t *p = &list_head;
    while(p->next){
        p = p->next;
        int clu_idx = p->clus_idx;
        void *sec1 = disk->data + (clu_idx - 2) * disk->header->BPB_SecPerClus * disk->header->BPB_BytsPerSec;
        //p->bmp->header = (bmp_header_t *) sec1;
        //printf("haha\n");
        //p->bmp->info = (bmp_info_t *)(sec1 + 14);
        char path_name[128] = "/tmp/";
        strcat(path_name, p->name);
        int fd = open(path_name, O_CREAT | O_WRONLY, S_IRWXU);
        write(fd, sec1, p->size);
        char sha1sum[256] = "sha1sum ";
        strcat(sha1sum, path_name);
        FILE *fp = popen(sha1sum, "r");
        panic_on(!fp, "popen");
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
    for (int i = 0; i < num; i++)
    {
        for (int i = 0; i < 10; i = i+2){
            pic->name[pos++] = d->LDIR_Name1[i];
            if(d->LDIR_Name1[i] == '\0') break; 
        }
        for (int i = 0; i < 12; i = i+2){
            pic->name[pos++] = d->LDIR_Name2[i];
            if(d->LDIR_Name2[i] == '\0') break; 
        }
        for (int i = 0; i < 4; i = i+2){
            pic->name[pos++] = d->LDIR_Name3[i];
            if(d->LDIR_Name3[i] == '\0') break; 
        }    
        d = (void *)d - 32;
    }
    if(pic->name[pos] != '\0') pic->name[pos] = '\0';
    d = c;
    pic->clus_idx = (d->DIR_FstClusHI << 16) | d->DIR_FstClusLO;
    pic->size = d->DIR_FileSize;
    int len = strlen(pic->name);
    
    if(pic->name[len-3] == 'b' && pic->name[len-2] == 'm'  && pic->name[len-1] == 'p' && pic->size != 0){
        pic_cnt++;
        //printf("\033[32m>> dectected file name: \033[0m%s , clus_idx :%x, file_size: %d\n", pic->name, pic->clus_idx, pic->size);
    }
    else{
        list_head.next = pic->next;
        if(pic->next!=NULL) pic->next->prev = &list_head;
        free(pic);
    }
    return ;
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