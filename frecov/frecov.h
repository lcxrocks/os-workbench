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
}fat; 


