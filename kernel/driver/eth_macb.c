#include "eth_macb.h"
#include "eth_macb_ops.h"
#include "mii_const.h"
#include "macb_const.h"
#include "driver.h"

void open(u8 *mac)
{
    clk_set_defaults(0);
    macb_eth_probe(mac);
    macb_start();
}

void clk_set_defaults(u64 clk_defaults_stage)
{
    u64 rate = 125125000;
    sifive_prci_set_rate(rate);
}

void macb_enable_clk()
{
    sifive_prci_enable();
}

void sifive_prci_enable()
{
    // sifive_prci_clock_enable
    u32 value = 0x80000000;
    u32 offs = 0x20;
    u64 addr = (u64)PRCI_BASE + offs;
    writev((u32 *)addr, value);
    sifive_prci_ethernet_release_reset();
}

i32 sifive_prci_set_rate(u64 rate)
{
    u32 value = 0x206b982;
    u32 offs = 0x1c;
    u64 addr = (u64)PRCI_BASE + offs;
    writev((u32 *)addr, value);
    u32 max_pll_lock_us = 70;
    usdelay(max_pll_lock_us);
    return 0;
}

void macb_eth_probe(u8 *mac)
{
    // char phy_mode[] = "gmii";
    macb_enable_clk();
    macb_eth_initialize();
    macb_write_hwaddr(mac);
}

void macb_eth_initialize()
{
    u32 id = 0;
    u32 ncfgr = 0;

    // u32 rx_buffer_size = 0;
    if (macb_is_gem())
    {
        printk("macb is GEM\n");
        // rx_buffer_size = GEM_RX_BUFFER_SIZE;
    }
    else
    {
        printk("macb is MACB\n");
        // rx_buffer_size = MACB_RX_BUFFER_SIZE;
    }

    // dma alloc

    // Do some basic initialization so that we at least can talk to the PHY
    u64 pclk_rate = 125125000;
    if (macb_is_gem())
    {
        ncfgr = gem_mdc_clk_div(id, pclk_rate);
        ncfgr |= macb_dbw();
    }
    else
    {
        ncfgr = macb_mdc_clk_div(id, pclk_rate);
    }

    printk("macb_eth_initialize to write MACB_NCFGR: {%u}\n", ncfgr);
    writev((u32 *)(MACB_IOBASE + MACB_NCFGR), ncfgr);
}

bool macb_is_gem()
{
    u32 mid_value = readv((u32 *)(MACB_IOBASE + MACB_MID));
    u32 macb_bfext = (mid_value >> MACB_IDNUM_OFFSET) & ((1 << MACB_IDNUM_SIZE) - 1);

    return macb_bfext >= 0x2;
}

bool gem_is_gigabit_capable()
{
    bool cpu_is_sama5d2 = false;
    bool cpu_is_sama5d4 = false;

    // The GEM controllers embedded in SAMA5D2 and SAMA5D4 are configured to support only 10/100.
    return macb_is_gem() && (!cpu_is_sama5d2) && (!cpu_is_sama5d4);
}

i32 macb_write_hwaddr(u8 *enetaddr)
{
    // set hardware address
    u32 hwaddr_bottom = (enetaddr[0]) | (enetaddr[1]) << 8 | (enetaddr[2]) << 16 | (enetaddr[3]) << 24;

    writev((u32 *)(MACB_IOBASE + MACB_SA1B), hwaddr_bottom);

    u32 hwaddr_top = (enetaddr[4]) | (enetaddr[5]) << 8;

    writev((u32 *)(MACB_IOBASE + MACB_SA1T), hwaddr_top);

    fence();

    printk("macb_write_hwaddr {%u} {%u}\n", hwaddr_top, hwaddr_bottom);

    return 0;
}

/*
 * Get the DMA bus width field of the network configuration register that we
 * should program. We find the width from decoding the design configuration
 * register to find the maximum supported data bus width.
 */
u32 macb_dbw()
{
    u32 dcfg1_value = readv((u32 *)(MACB_IOBASE + GEM_DCFG1));
    u32 gem_bfex = (dcfg1_value >> GEM_DBWDEF_OFFSET) & ((1 << GEM_DBWDEF_SIZE) - 1);
    printk("macb_dbw, dcfg1: {%u}, gem_bfex: {%u}\n", dcfg1_value, gem_bfex);
    switch (gem_bfex)
    {
    case 4: {
        return ((GEM_DBW128 & ((1 << GEM_DBW_SIZE) - 1)) << GEM_DBW_OFFSET);
    }
    case 2: {
        return ((GEM_DBW64 & ((1 << GEM_DBW_SIZE) - 1)) << GEM_DBW_OFFSET);
    }
    default: {
        return ((GEM_DBW32 & ((1 << GEM_DBW_SIZE) - 1)) << GEM_DBW_OFFSET);
    }
    }
}

u32 macb_mdc_clk_div(u32 id, u64 pclk_rate)
{
    u32 config = 0;

    // macb->pclk_rate
    // let macb_hz: u64 = 125125000;
    u64 macb_hz = pclk_rate;

    if (macb_hz < 20000000)
    {
        config = ((MACB_CLK_DIV8 & ((1 << MACB_CLK_SIZE) - 1)) << MACB_CLK_OFFSET);
    }
    else if (macb_hz < 40000000)
    {
        config = ((MACB_CLK_DIV16 & ((1 << MACB_CLK_SIZE) - 1)) << MACB_CLK_OFFSET);
    }
    else if (macb_hz < 80000000)
    {
        config = ((MACB_CLK_DIV32 & ((1 << MACB_CLK_SIZE) - 1)) << MACB_CLK_OFFSET);
    }
    else
    {
        config = ((MACB_CLK_DIV64 & ((1 << MACB_CLK_SIZE) - 1)) << MACB_CLK_OFFSET);
    }

    return config;
}

u32 gem_mdc_clk_div(u32 id, u64 pclk_rate)
{
    u32 config = 0;
    u64 macb_hz = pclk_rate;

    if (macb_hz < 20000000)
    {
        config = ((GEM_CLK_DIV8 & ((1 << GEM_CLK_SIZE) - 1)) << GEM_CLK_OFFSET);
    }
    else if (macb_hz < 40000000)
    {
        config = ((GEM_CLK_DIV16 & ((1 << GEM_CLK_SIZE) - 1)) << GEM_CLK_OFFSET);
    }
    else if (macb_hz < 80000000)
    {
        config = ((GEM_CLK_DIV32 & ((1 << GEM_CLK_SIZE) - 1)) << GEM_CLK_OFFSET);
    }
    else if (macb_hz < 120000000)
    {
        config = ((GEM_CLK_DIV48 & ((1 << GEM_CLK_SIZE) - 1)) << GEM_CLK_OFFSET);
    }
    else if (macb_hz < 160000000)
    {
        config = ((GEM_CLK_DIV64 & ((1 << GEM_CLK_SIZE) - 1)) << GEM_CLK_OFFSET);
    }
    else if (macb_hz < 240000000)
    {
        config = ((GEM_CLK_DIV96 & ((1 << GEM_CLK_SIZE) - 1)) << GEM_CLK_OFFSET);
    }
    else if (macb_hz < 320000000)
    {
        config = ((GEM_CLK_DIV128 & ((1 << GEM_CLK_SIZE) - 1)) << GEM_CLK_OFFSET);
    }
    else
    {
        config = ((GEM_CLK_DIV224 & ((1 << GEM_CLK_SIZE) - 1)) << GEM_CLK_OFFSET);
    }

    return config;
}

void sifive_prci_ethernet_release_reset()
{
    /* gemgxl_reset, Release GEMGXL reset */
    // do clk_set_defaults(clock-controller@10000000);
    // sifive reset deassert, write id: 5, regval: 0x2f
    sifive_reset_trigger(5, true);

    /* Procmon => core clock */
    writev((u32 *)(PRCI_BASE + PRCI_PROCMONCFG_OFFSET),
           PRCI_PROCMONCFG_CORE_CLOCK_MASK);

    /* cltx_reset, Release Chiplink reset */
    // write id: 6, regval: 0x6f
    sifive_reset_trigger(6, true);
}

void sifive_reset_trigger(u32 id, bool level)
{
    u32 regval = readv((u32 *)(PRCI_BASE + PRCI_RESETREG_OFFSET));
    if (level)
    {
        // Reset deassert
        regval |= (((u32)1) << id);
    }
    else
    {
        // Reset assert
        regval &= ~(((u32)1) << id);
    }
    printk("sifive_reset_trigger to write: {%u}\n", regval);
    writev((u32 *)(PRCI_BASE + PRCI_RESETREG_OFFSET), regval);
}

// const *const u64 MMIO_MTIME = 0x0200_BFF8 as *const u64;
u64 get_cycle()
{
    // Load access fault @ 0x200bff8 on fu740
    // unsafe { MMIO_MTIME.read_volatile() }
    u64 cycle = 0;
    asm volatile("csrr %0, time"
                 : "=r"(cycle));
    return cycle;
}

// fu740 CPU Timer, Freq = 1000000Hz
// 微秒(us)
void usdelay(u64 us)
{
    u64 t1 = get_cycle();
    u64 t2 = t1 + us * (TIMER_CLOCK / 1000000);

    while (t2 >= t1)
    {
        t1 = get_cycle();
    }
    printk("usdelay get_cycle: {%u}\n", t1);
}

// 毫秒(ms)
void msdelay(u64 ms)
{
    usdelay(ms * 1000);
}

void fence()
{
    asm volatile("fence iorw, iorw");
}

u32 readv(u32 *src)
{
    return *src;
}

void writev(u32 *dst, u32 value)
{
    *dst = value;
}
