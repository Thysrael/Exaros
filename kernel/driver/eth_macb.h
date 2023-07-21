#ifndef _ETH_MACB_H
#define _ETH_MACB_H

#include "macb_const.h"
#include "eth_macb_ops.h"

// clk -> reg
#define TIMER_CLOCK (1000000)

#define RESET_BASE (0)
#define PRCI_RESETREG_OFFSET (0x28)

// PROCMONCFG
#define PRCI_PROCMONCFG_OFFSET (0xF0)
#define PRCI_PROCMONCFG_CORE_CLOCK_MASK (1 << 24)

#define MACB_IOBASE (0x10090000)
#define GEMGXL_BASE (0x100a0000)
#define PRCI_BASE (0x10000000)

/// Allocate consequent physical memory for DMA;
/// Return physical address which is page aligned.
// u64 dma_alloc_coherent()
// {
// }
/// Deallocate DMA memory
// u64 dma_free_coherent()
// {
// }

void open(u8 *mac);
void clk_set_defaults(u64 clk_defaults_stage);
void macb_enable_clk();
void sifive_prci_enable();
i32 sifive_prci_set_rate(u64 rate);
void macb_eth_probe(u8 *mac);
void macb_eth_initialize();
bool macb_is_gem();
bool gem_is_gigabit_capable();
i32 macb_write_hwaddr(u8 *enetaddr);
u32 macb_dbw();
u32 macb_mdc_clk_div(u32 id, u64 pclk_rate);
u32 gem_mdc_clk_div(u32 id, u64 pclk_rate);
void sifive_prci_ethernet_release_reset();
void sifive_reset_trigger(u32 id, bool level);
u64 get_cycle();
void usdelay(u64 us);
void msdelay(u64 ms);
void fence();
u32 readv(u32 *src);
void writev(u32 *dst, u32 value);

#endif