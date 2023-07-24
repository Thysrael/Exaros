#ifndef _MACB_H_
#define _MACB_H_

#define PRCI_BASE 0x10000000
#define PRCI_REG(r) ((volatile u32 *)(PRCI_BASE + (r)))

#define MACB_IOBASE 0x10090000
#define MACB_REG(r) ((volatile u32 *)(MACB_IOBASE + (r)))

#define GEMGXL_BASE 0x100a0000
#define GEMGXL_REG(r) ((volatile u32 *)(GEMGXL_BASE + (r)))

void macb_init();

#endif