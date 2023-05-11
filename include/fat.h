/**
 * @file fat.h
 * @brief fat 层，为文件系统的实现层
 * @date 2023-05-11
 *
 * @copyright Copyright (c) 2023
 */
#ifndef _FAT_H_
#define _FAT_H_

#include "types.h"
#include "linux_struct.h"

// 这是 FAT dentry 的属性，包括的范围很广
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME 0x0F
#define ATTR_LINK 0x40
#define ATTR_CHARACTER_DEVICE 0x80

// FAT 表中内容
#define FAT32_EOC 0x0ffffff8

#define LAST_LONG_ENTRY 0x40
#define EMPTY_ENTRY 0xe5  // 表示该条目未被分配
#define END_OF_ENTRY 0x00 // 表示该条目未被分配，同时此后的条目也未被分配

// FAT 的一些长度宏
#define CHAR_LONG_NAME 13
#define CHAR_SHORT_NAME 11
#define FAT32_MAX_FILENAME 255

typedef struct SuperBlock
{
    u32 firstDataSec; // 数据区的第一个扇区号
    u32 dataSecCnt;   // 数据区的扇区总数
    u32 dataClusCnt;  // 数据区的簇总数
    u32 bytsPerClus;  // 每个簇的字节数
    struct
    {
        u16 bytsPerSec; // 每个扇区的字节数
        u8 secPerClus;  // 每个簇的扇区数
        u16 rsvdSecCnt; // 保留区的扇区数
        u8 numFATs;     // FAT 表的数量，也就是大概就是 2
        u32 totSec;     // 总扇区数，包括所有区域
        u32 FATsz;      // 每个 FAT 表的扇区数
        u32 rootClus;   // 根目录（也就是数据区）所在的簇号
    } BPB;              // 是 BIOS 参数块（BIOS Parameter Block）的缩写
} SuperBlock;

/**
 * @brief 短文件名目录项
 *
 */
typedef struct short_name_entry
{
    // name[0] 的值如果是 0xe5 EMPTY_ENTRY 和 0x00 END_OF_ENTRY, 是有特殊含义的
    // 它和 LNE 的 order 字符在同一个内存位置
    char name[CHAR_SHORT_NAME]; // 文件名，最多 11 个字符，如果文件名不足 11 个字符，则用空格填充
    u8 attr;                    // 文件属性，用于标识该目录项对应的是文件还是目录
    u8 _nt_res;                 // 保留字段，未使用
    u8 _crt_time_tenth;
    u16 _crt_time;
    u16 _crt_date;
    u16 _lst_acce_date;
    u16 fst_clus_hi; // 文件起始簇号的高 16 位
    u16 _lst_wrt_time;
    u16 _lst_wrt_date;
    u16 fst_clus_lo;
    u32 file_size; // 文件大小，以字节为单位
} __attribute__((packed, aligned(4))) short_name_entry_t;

/**
 * @brief 长文件名目录项
 *
 */
typedef struct long_name_entry
{
    u8 order;       // 长文件名目录项的顺序号，从 1 开始递增
    wchar name1[5]; // 文件名的前 5 个字符，以 Unicode 编码方式存储
    // 这里是第 11 个字节，和 SNE 的第 11 个字节相同
    u8 attr;          // 文件属性，用于标识该目录项对应的是文件还是目录
    u8 _type;         // 文件名类型，用于标识该目录项是长文件名还是短文件名
    u8 checksum;      // 校验和，用于检测长文件名目录项是否被修改
    wchar name2[6];   // 文件名的中间 6 个字符，以 Unicode 编码方式存储
    u16 _fst_clus_lo; // 文件起始簇号的低 16 位
    wchar name3[2];   // 文件名后两个字符
} __attribute__((packed, aligned(4))) long_name_entry_t;

/**
 * @brief 是联合体！可以是 short_name_entry，也可以是 long_name_entry
 * - 短文件名目录项（`sne`）：用于存储文件的短文件名，最多 11 个字符，通常用于 DOS 和 Windows 系统下的文件命名。
 *   短文件名目录项的结构比较简单，只包含了文件名、文件属性、时间戳、起始簇号和文件大小等基本信息。
 * - 长文件名目录项（`lne`）：用于存储文件的长文件名，最多 255 个字符，通常用于支持 Unicode 字符集的文件系统中。
 * 长文件名目录项的结构比较复杂，包含了文件名的各个部分、文件属性、校验和和起始簇号等信息。
 */
typedef union dentry
{
    short_name_entry_t sne;
    long_name_entry_t lne;
} Dentry;

int getSectorNumber(DirMeta *meta, int dataSectorNum);
u32 rwClus(FileSystem *fs, u32 cluster, int write, int user, u64 data, u32 off, u32 n);
int relocClus(FileSystem *fs, DirMeta *meta, u32 off, int alloc);
int metaRead(DirMeta *meta, int userDst, u64 dst, u32 off, u32 n);
int metaWrite(DirMeta *meta, int userSrc, u64 src, u32 off, u32 n);
DirMeta *metaAlloc(DirMeta *parent, char *name, int attr);
DirMeta *metaCreate(int fd, char *path, short type, int mode);
void metaTrunc(DirMeta *meta);
void metaStat(DirMeta *meta, struct kstat *st);
DirMeta *metaName(int fd, char *path, bool jump);
DirMeta *metaNameDir(int fd, char *path, char *name);
void loadDirMetas(FileSystem *fs, DirMeta *parent);
int fatInit(FileSystem *fs);
#endif