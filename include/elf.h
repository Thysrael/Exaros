/**
 * @file elf.h
 * @brief 关于 elf 文件
 * @date 2023-04-13
 *
 * @copyright Copyright (c) 2023
 */

#ifndef _ELF_H_
#define _ELF_H_

#include <types.h>

#define MAG_SIZE 4
#define ELF_MAGIC0 0x7f
#define ELF_MAGIC1 0x45
#define ELF_MAGIC2 0x4c
#define ELF_MAGIC3 0x46

#define PT_NULL 0x00000000
#define PT_LOAD 0x00000001
#define PT_DYNAMIC 0x00000002
#define PT_INTERP 0x00000003
#define PT_NOTE 0x00000004
#define PT_SHLIB 0x00000005
#define PT_PHDR 0x00000006
#define PT_LOOS 0x60000000
#define PT_HIOS 0x6fffffff
#define PT_LOPROC 0x70000000
#define PT_HIRPOC 0x7fffffff

#define PT_GNU_EH_FRAME (PT_LOOS + 0x474e550)
#define PT_GNU_STACK (PT_LOOS + 0x474e551)
#define PT_GNU_RELRO (PT_LOOS + 0x474e552)
#define PT_GNU_PROPERTY (PT_LOOS + 0x474e553)

#define PF_ALL 0x7
#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

/* These constants define the different elf file types */
#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4
#define ET_LOPROC 0xff00
#define ET_HIPROC 0xffff

typedef struct
{
    u8 magic[MAG_SIZE];
    u8 type;
    u8 data;
    u8 version;
    u8 osabi;
    u8 abiversion;
    u8 pad[7];
} Indent;

// file header
typedef struct
{
    Indent indent; // must equal ELF_MAGIC
    u16 type;      // 1=relocatable, 2=executable, 3=shared object, 4=core image
    u16 machine;   // 3=x86, 4=68K, etc.
    u32 version;   // file version, always 1
    u64 entry;     // entry point if executable
    u64 phoff;     // file position of program header or 0
    u64 shoff;     // file position of section header or 0
    u32 flags;     // architecture-specific flags, usually 0
    u16 ehsize;    // size of this elf header
    u16 phentsize; // size of an entry in program header
    u16 phnum;     // number of entries in program header or 0
    u16 shentsize; // size of an entry in section header
    u16 shnum;     // number of entries in section header or 0
    u16 shstrndx;  // section number that contains section name strings
} Ehdr;

// Program section header
typedef struct
{
    u32 type;   // loadable code or data, dynamic linking info,etc.
    u32 flags;  // read/write/execute bits
    u64 offset; // file offset of segment
    u64 vaddr;  // virtual address to map segment
    u64 paddr;  // physical address, not used
    u64 filesz; // size of segment in file
    u64 memsz;  // size of segment in memory (bigger if contains bss）
    u64 align;  // required alignment, invariably hardware page size
} Phdr;

int loadElf(u8 *binary, int size, u64 *entry, void *process);

inline bool isElfFormat(u8 *binary)
{
    u8 *magic = ((Indent *)binary)->magic;
    if (magic[0] == ELF_MAGIC0 && magic[1] == ELF_MAGIC1 && magic[2] == ELF_MAGIC2 && magic[3] == ELF_MAGIC3)
        return true;
    return false;
}

#endif