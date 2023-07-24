#include <types.h>
#include <macb.h>
#include <delay.h>
#include <debug.h>
#include <memory.h>
#include <delay.h>
#include <mii.h>
#include <phy_mscc.h>
#include <string.h>

#define GEM_RX_BUFFER_SIZE 2048
#define MACB_RX_RING_SIZE 32
#define MACB_TX_RING_SIZE 16
#define RX_BUFFER_MULTIPLE 64

#define HW_DMA_CAP_32B 0

#define MACB_AUTONEG_TIMEOUT 5000000

/**
 * @brief 设置 PRCI 时钟默认频率 12512500
 *
 */
static void sifive_prci_set_rate()
{
    NET_DEBUG("macb rate set begin.\n");
    u32 value = 0x206b982; // 可以查手册得到
    *GEMGXL_REG(GEMGXL_PLL_CFG) = value;

    // microseconds微秒，等待 70 微秒
    u64 max_pll_lock_us = 70;
    usdelay(max_pll_lock_us);

    NET_DEBUG("macb rate set end.\n");
}

/**
 * @brief 开启 PRCI 时钟
 *
 */
static void sifive_prci_enable()
{
    NET_DEBUG("prci enable begin.\n");
    u32 value = 0x80000000;
    *GEMGXL_REG(GEMGXL_PLL_OUTDIV) = value;
    NET_DEBUG("prci enable end.\n");
}

/**
 * @brief 重启触发器，负责重启 gxemul，id 说明重启设备编号，所以一般是 5（gxemul），level 说明高低电平
 *
 * @param id 设备编号
 * @param level 高低电平
 */
static void sifive_reset_trigger(u32 id, bool level)
{
    u32 regVal = *PRCI_REG(PRCI_RESETREG);
    if (level)
    {
        regVal |= 1 << id;
    }
    else
    {
        regVal &= ~(1 << id);
    }
    NET_DEBUG("sifive_reset_trigger to write: 0x%x\n", regVal);
    *PRCI_REG(PRCI_RESETREG) = regVal;
}

/**
 * @brief 重启 prci
 *
 */
static void sifive_prci_ethernet_release_reset()
{
    NET_DEBUG("prci reset begin.\n");
    /* gemgxl_reset, Release GEMGXL reset */
    sifive_reset_trigger(5, true);
    // processor monitor
    /* Procmon => core clock */
    *PRCI_REG(PRCI_PROCMONCFG) = PRCI_PROCMONCFG_CORE_CLOCK_MASK;
    /* cltx_reset, Release Chiplink reset */
    // write id: 6, regval: 0x6f(110_1111)
    sifive_reset_trigger(6, true);
    NET_DEBUG("prci reset end.\n");
}

static bool macb_is_gem()
{
    u32 mid_value = *MACB_REG(MACB_MID);
    u32 macb_bfext = (mid_value >> MACB_IDNUM_OFFSET) & ((1 << MACB_IDNUM_SIZE) - 1);
    NET_DEBUG("mid value is 0x%x, macb bfext is 0x%x\n", mid_value, macb_bfext);
    return macb_bfext >= 2;
}

static u32 gem_mdc_clk_div(u32 id, u64 pclk_rate)
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
    NET_DEBUG("NCFGR init value is 0x%x\n", config);
    return config;
}

/*
 * Get the DMA bus width field of the network configuration register that we
 * should program. We find the width from decoding the design configuration
 * register to find the maximum supported data bus width.
 */
u32 macb_dbw()
{
    u32 dcfg1_value = *MACB_REG(GEM_DCFG1);
    u32 gem_bfex = (dcfg1_value >> GEM_DBWDEF_OFFSET) & ((1 << GEM_DBWDEF_SIZE) - 1);
    NET_DEBUG("macb_dbw, dcfg1: 0x%x, gem_bfex: 0x%x\n", dcfg1_value, gem_bfex);
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

static u32 macb_mdc_clk_div(u32 id, u64 pclk_rate)
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

static void macb_eth_probe()
{
    macb_is_gem();
    // rx/tx ring dma
    // rx_buffer_size: 2048
    u32 id = 0;
    u32 ncfgr = 0; // ncfgr, network config register

    // RX==receive，接收，从开启到现在接收封包的情况，是下行流量（Downlink）
    // TX==Transmit，发送，从开启到现在发送封包的情况，是上行流量（Uplink）

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

    NET_DEBUG("write MACB_NCFGR: 0x%x\n", ncfgr);
    *MACB_REG(MACB_NCFGR) = ncfgr;
}

MacbDevice macb;
MacbConfig config;
u32 send_buffers[32];
u32 recv_buffers[64]; // 每个元素记录的都是 buffer 的物理地址
DmaDesc *dummy_desc;

/**
 * @brief 初始化 GEMGXL TX 时钟。SiFive GEMGXL TX 有两种模式：
 * 0 = GMII mode. Use 125 MHz gemgxlclk from PRCI in TX logic and output clock on GMII output signal GTX_CLK
 * 1 = MII mode. Use MII input signal TX_CLK in TX logic
 *
 * @param rate 时钟频率
 */
static void macb_sifive_clk_init(u64 rate)
{
    int mod = (rate == 125000000) ? 0 : 1;
    *GEMGXL_REG(0) = mod;
}

static bool is_big_endian()
{
    return false;
}

static u32 GEM_BF(u32 gem_offset, u32 gem_size, u32 value)
{
    return (value & ((1 << gem_size) - 1)) << gem_offset;
}

static u32 GEM_BFINS(u32 gem_offset, u32 gem_size, u32 value, u32 old)
{
    return (old & ~(((1 << gem_size) - 1) << gem_offset)) | GEM_BF(gem_offset, gem_size, value);
}

static u32 GEM_BIT(u32 offset)
{
    return 1 << offset;
}

/**
 * @brief 应该是进行一些 dma 相关的配置
 *
 * @return i32
 */
static void gmac_configure_dma()
{
    u32 buffer_size = (macb.buffer_size / RX_BUFFER_MULTIPLE);
    i64 neg = -1;
    u32 dmacfg = *MACB_REG(GEM_DMACFG);
    NET_DEBUG("gmac_configure_dma read GEM_DMACFG: 0x%x\n", dmacfg);
    dmacfg &= ~GEM_BF(GEM_RXBS_OFFSET, GEM_RXBS_SIZE, neg);
    NET_DEBUG("gmac_configure_dma dmacfg: 0x%x\n", dmacfg);

    dmacfg |= GEM_BF(GEM_RXBS_OFFSET, GEM_RXBS_SIZE, buffer_size);

    if (macb.config->dma_burst_length != 0)
    {
        dmacfg = GEM_BFINS(
            GEM_FBLDO_OFFSET,
            GEM_FBLDO_SIZE,
            macb.config->dma_burst_length,
            dmacfg);
    }

    dmacfg |= GEM_BIT(GEM_TXPBMS_OFFSET) | GEM_BF(GEM_RXBMS_OFFSET, GEM_RXBMS_SIZE, neg);
    dmacfg &= ~GEM_BIT(GEM_ENDIA_PKT_OFFSET);

    if (is_big_endian())
    {
        dmacfg |= GEM_BIT(GEM_ENDIA_DESC_OFFSET);
    }
    else
    {
        dmacfg &= ~GEM_BIT(GEM_ENDIA_DESC_OFFSET);
    }

    dmacfg &= ~GEM_BIT(GEM_ADDR64_OFFSET);

    NET_DEBUG("gmac_configure_dma write GEM_DMACFG @ 0x%x, dmacfg = 0x%x\n",
              MACB_IOBASE + GEM_DMACFG,
              dmacfg);
    *MACB_REG(GEM_DMACFG) = dmacfg;
}

static u32 GEM_TBQP(u32 hw_q)
{
    return 0x0440 + ((hw_q) << 2);
}

static u32 GEM_RBQP(u32 hw_q)
{
    return 0x0480 + ((hw_q) << 2);
}

static inline u32 readv(u32 *src)
{
    return *src;
}

static inline void writev(u32 *dst, u32 value)
{
    *dst = value;
}

static void gmac_init_multi_queues()
{
    i32 num_queues = 1;
    // bit 0 is never set but queue 0 always exists
    u32 queue_mask = 0xff & (*MACB_REG(GEM_DCFG6));
    NET_DEBUG("gmac_init_multi_queues read GEM_DCFG6: 0x%x\n", queue_mask);
    queue_mask |= 0x1;

    for (int i = 1; i < MACB_MAX_QUEUES; ++i)
    {
        if ((queue_mask & (1 << i)) != 0)
        {
            num_queues += 1;
        }
    }

    macb.dummy_desc[0].ctrl = 1 << MACB_TX_USED_OFFSET;
    macb.dummy_desc[0].addr = 0;

    u64 paddr = macb.dummy_desc_dma;

    for (int i = 1; i < num_queues; ++i)
    {
        NET_DEBUG("gmac_init_multi_queues {%d} TBQP: {%d}, RBQP: {%d}",
                  i,
                  GEM_TBQP(i - 1),
                  GEM_RBQP(i - 1));
        writev((u32 *)(MACB_IOBASE + (u64)GEM_TBQP(i - 1)),
               (u32)paddr);
        writev((u32 *)(MACB_IOBASE + (u64)GEM_RBQP(i - 1)),
               (u32)paddr);
    }
}

static inline void fence()
{
    asm volatile("fence iorw, iorw");
}

u16 macb_mdio_read(u32 phy_adr, u32 reg)
{
    u32 netctl = *MACB_REG(MACB_NCR);
    netctl |= 1 << MACB_MPE_OFFSET;
    writev((u32 *)(MACB_IOBASE + MACB_NCR), netctl);

    u32 frame = ((1 & ((1 << MACB_SOF_SIZE) - 1)) << MACB_SOF_OFFSET)
                | ((2 & ((1 << MACB_RW_SIZE) - 1)) << MACB_RW_OFFSET)
                | ((phy_adr & ((1 << MACB_PHYA_SIZE) - 1)) << MACB_PHYA_OFFSET)
                | ((reg & ((1 << MACB_REGA_SIZE) - 1)) << MACB_REGA_OFFSET)
                | ((2 & ((1 << MACB_CODE_SIZE) - 1)) << MACB_CODE_OFFSET);

    writev((u32 *)(MACB_IOBASE + MACB_MAN), frame);
    while ((readv((u32 *)(MACB_IOBASE + MACB_NSR)) & (1 << MACB_IDLE_OFFSET)) == 0)
    {}

    frame = readv((u32 *)(MACB_IOBASE + MACB_MAN));

    netctl = readv((u32 *)(MACB_IOBASE + MACB_NCR));
    netctl &= ~(1 << MACB_MPE_OFFSET);
    writev((u32 *)(MACB_IOBASE + MACB_NCR), netctl);

    return ((frame >> MACB_DATA_OFFSET) & ((1 << MACB_DATA_SIZE) - 1));
}

void macb_mdio_write(u32 phy_adr, u32 reg, u16 value)
{
    // info!("mdio write phy_adr: {:#x}, reg: {:#x}, value: {:#x}", phy_adr, reg, value);
    u32 netctl = readv((u32 *)(MACB_IOBASE + MACB_NCR));
    netctl |= 1 << MACB_MPE_OFFSET;
    writev((u32 *)(MACB_IOBASE + MACB_NCR), netctl);

    // MACB_BF(name,value)
    // (((value) & ((1 << MACB_x_SIZE) - 1)) << MACB_x_OFFSET)

    u32 frame = ((1 & ((1 << MACB_SOF_SIZE) - 1)) << MACB_SOF_OFFSET)
                | ((1 & ((1 << MACB_RW_SIZE) - 1)) << MACB_RW_OFFSET)
                | ((phy_adr & ((1 << MACB_PHYA_SIZE) - 1)) << MACB_PHYA_OFFSET)
                | ((reg & ((1 << MACB_REGA_SIZE) - 1)) << MACB_REGA_OFFSET)
                | ((2 & ((1 << MACB_CODE_SIZE) - 1)) << MACB_CODE_OFFSET)
                | (((u32)value & ((1 << MACB_DATA_SIZE) - 1)) << MACB_DATA_OFFSET);

    writev((u32 *)(MACB_IOBASE + MACB_MAN), frame);
    while ((readv((u32 *)(MACB_IOBASE + MACB_NSR)) & (1 << MACB_IDLE_OFFSET)) == 0)
    {}

    netctl = readv((u32 *)(MACB_IOBASE + MACB_NCR));
    netctl &= !(1 << MACB_MPE_OFFSET);
    writev((u32 *)(MACB_IOBASE + MACB_NCR), netctl);
}

static i32 macb_phy_find()
{
    u16 phy_id = macb_mdio_read(macb.phy_addr, MII_PHYSID1);
    if (phy_id != 0xffff)
    {
        NET_DEBUG("PHY present at 0x%x\n", macb.phy_addr);
        return 0;
    }
    // Search for PHY...
    for (int i = 0; i < 32; ++i)
    {
        macb.phy_addr = i;
        phy_id = macb_mdio_read(macb.phy_addr, MII_PHYSID1);
        if (phy_id != 0xffff)
        {
            NET_DEBUG("Found PHY present at {%d}\n", i);
            return 0;
        }
    }

    // PHY isn't up to snuff
    NET_DEBUG("PHY not found\n");
    return -19; // ENODEV
}

static int phy_reset(u32 phydev_addr, PhyInterfaceMode _interface, u32 phydev_flags)
{
    i32 timeout = 500;

    NET_DEBUG("PHY soft reset\n");
    if ((phydev_flags & PHY_FLAG_BROKEN_RESET) != 0)
    {
        NET_DEBUG("PHY soft reset not supported\n");
        return 0;
    }

    macb_mdio_write(phydev_addr, MII_BMCR, BMCR_RESET);
    /*
     * Poll the control register for the reset bit to go to 0 (it is
     * auto-clearing).  This should happen within 0.5 seconds per the
     * IEEE spec.
     */
    u16 reg = macb_mdio_read(phydev_addr, MII_BMCR);
    while (((reg & BMCR_RESET) != 0) && (timeout != 0))
    {
        timeout -= 1;
        reg = macb_mdio_read(phydev_addr, MII_BMCR);
        usdelay(1000);
    }
    if ((reg & BMCR_RESET) != 0)
    {
        NET_DEBUG("PHY reset timed out\n");
        return -1;
    }
    return 0;
}

static void phy_connect_dev()
{
    u32 phydev_addr = macb.phy_addr;
    PhyInterfaceMode phydev_interface = macb.phy_interface;
    u32 phydev_flags = 0;

    i32 phydev = 0xff;

    if (phydev != 0)
    {
        /* Soft Reset the PHY */
        phy_reset(phydev_addr, phydev_interface, phydev_flags);
        NET_DEBUG("Ethernet connected to PHY\n");

        // phy_config needs phydev
        vsc8541_config(phydev_addr, phydev_interface);
    }
    else
    {
        NET_DEBUG("Could not get PHY for ethernet: addr 0x%x\n", macb.phy_addr);
    }
}

static void macb_phy_reset(char *name)
{
    u32 status = 0;
    u32 adv = ADVERTISE_CSMA | ADVERTISE_ALL;
    macb_mdio_write(macb.phy_addr, MII_ADVERTISE, adv);
    NET_DEBUG("%s Starting autonegotiation...\n", name);
    macb_mdio_write(macb.phy_addr,
                    MII_BMCR,
                    (BMCR_ANENABLE | BMCR_ANRESTART));

    u32 i = 0;
    while (i < (MACB_AUTONEG_TIMEOUT / 100))
    {
        i += 1;
        status = macb_mdio_read(macb.phy_addr, MII_BMSR);
        if ((status & BMSR_ANEGCOMPLETE) != 0)
        {
            break;
        }
        usdelay(200);
    }

    if ((status & BMSR_ANEGCOMPLETE) != 0)
    {
        NET_DEBUG("{%s} Autonegotiation complete\n", name);
    }
    else
    {
        NET_DEBUG("{%s} Autonegotiation timed out (status={%d})\n", name, status);
    }
}

static char *mystrncpy(char *s, const char *t, int n)
{
    char *os;

    os = s;
    while (n-- > 0 && (*s++ = *t++) != 0)
        ;
    while (n-- > 0)
        *s++ = 0;
    return os;
}

static i32 macb_linkspd_cb(u32 speed)
{
    u64 rate = 0;
    switch (speed)
    {
    case 10: {
        rate = 2500000;
        break;
    }
    case 100: {
        rate = 25000000;
        break;
    }
    case 1000: {
        rate = 125000000;
        break;
    }
    default: {
        return 0;
    }
    }
    // clk_init
    macb_sifive_clk_init(rate);
    return 0;
}

static u32 mii_nway_result(u32 negotiated)
{
    u32 ret = 0;
    if ((negotiated & LPA_100FULL) != 0)
    {
        ret = LPA_100FULL;
    }
    else if ((negotiated & LPA_100BASE4) != 0)
    {
        ret = LPA_100BASE4;
    }
    else if ((negotiated & LPA_100HALF) != 0)
    {
        ret = LPA_100HALF;
    }
    else if ((negotiated & LPA_10FULL) != 0)
    {
        ret = LPA_10FULL;
    }
    else
    {
        ret = LPA_10HALF;
    }

    return ret;
}

static i32 macb_phy_init(char *name)
{
    NET_DEBUG("macb phy init begin.\n");
    // Auto-detect phy_addr
    i32 ret = macb_phy_find(macb);
    if (ret != 0) return ret;

    // Check if the PHY is up to snuff...
    u16 phy_id = macb_mdio_read(macb.phy_addr, MII_PHYSID1);
    if (phy_id == 0xffff)
    {
        NET_DEBUG("No PHY present\n");
        return -10; // ENODEV
    }
    NET_DEBUG("macb_phy_init phy_id: %d\n", phy_id);

    // Find macb->phydev
    phy_connect_dev(&macb);

    u32 status = macb_mdio_read(macb.phy_addr, MII_BMSR);
    if ((status & BMSR_LSTATUS) == 0)
    {
        // Try to re-negotiate if we don't have link already.
        macb_phy_reset(name);
        u32 i = 0;
        while (i < (MACB_AUTONEG_TIMEOUT / 100))
        {
            i += 1;
            status = macb_mdio_read(macb.phy_addr, MII_BMSR);
            if ((status & BMSR_LSTATUS) != 0)
            {
                // Delay a bit after the link is established, so that the next xfer does not fail
                msdelay(10);
                break;
            }
            usdelay(100);
        }
    }

    if ((status & BMSR_LSTATUS) == 0)
    {
        NET_DEBUG("%s link down (status: %d)\n", name, status);
        return -100; // ENETDOWN
    }

    u32 ncfgr = 0;
    u16 lpa = 0;
    u16 adv = 0;

    // First check for GMAC and that it is GiB capable
    if (macb_is_gem())
    {
        lpa = macb_mdio_read(macb.phy_addr, MII_STAT1000);

        if ((lpa & (u16)(LPA_1000FULL | LPA_1000HALF | LPA_1000XFULL | LPA_1000XHALF)) != 0)
        {
            u32 duplex = ((lpa & (u16)(LPA_1000FULL | LPA_1000XFULL)) == 0) ? 0 : 1;
            char duplex_str[5];
            if (duplex == 1) { mystrncpy(duplex_str, "full", 5); }
            else { mystrncpy(duplex_str, "half", 4); }
            NET_DEBUG("{%s} GiB capable, link up, 1000Mbps {%S}-duplex (lpa: {%d)\n",
                      name, duplex_str, lpa);

            ncfgr = readv((u32 *)(MACB_IOBASE + MACB_NCFGR));
            ncfgr &= ~((1 << MACB_SPD_OFFSET) | (1 << MACB_FD_OFFSET));
            ncfgr |= 1 << GEM_GBE_OFFSET;
            if (duplex == 1)
            {
                ncfgr |= 1 << MACB_FD_OFFSET;
            }

            writev((u32 *)(MACB_IOBASE + MACB_NCFGR), ncfgr);

            macb_linkspd_cb(_1000BASET);

            return 0;
        }
    }

    // fall back for EMAC checking
    adv = macb_mdio_read(macb.phy_addr, MII_ADVERTISE);
    lpa = macb_mdio_read(macb.phy_addr, MII_LPA);
    u32 media = mii_nway_result((u32)(lpa & adv));

    u32 speed = ((media & (ADVERTISE_100FULL | ADVERTISE_100HALF)) == 0) ? 0 : 1;
    char speed_str[4];
    if (speed == 1) { mystrncpy(speed_str, "100", 4); }
    else { mystrncpy(speed_str, "10", 4); }
    u32 duplex = ((media & ADVERTISE_FULL) == 0) ? 0 : 0;
    char duplex_str[5];
    if (duplex == 1) { mystrncpy(duplex_str, "full", 5); }
    else { mystrncpy(duplex_str, "half", 4); }
    NET_DEBUG("{%s} link up, {%s}Mbps {%s}-duplex (lpa: {%d})\n",
              name, speed_str, duplex_str, lpa);

    ncfgr = readv((u32 *)(MACB_IOBASE + MACB_NCFGR));
    ncfgr &= ~((1 << MACB_SPD_OFFSET) | (1 << MACB_FD_OFFSET) | (1 << GEM_GBE_OFFSET));
    if (speed == 1)
    {
        ncfgr |= 1 << MACB_SPD_OFFSET;
        macb_linkspd_cb(_100BASET);
    }
    else
    {
        macb_linkspd_cb(_10BASET);
    }
    if (duplex == 1)
    {
        ncfgr |= (1 << MACB_FD_OFFSET);
    }
    writev((u32 *)(MACB_IOBASE + MACB_NCFGR), ncfgr);
    fence();
    NET_DEBUG("macb phy init end.\n");
    return 0;
}

static void macb_start()
{
    NET_DEBUG("MACB start begin.\n");

    u32 buffer_size = GEM_RX_BUFFER_SIZE;
    char name[] = "ethernet@10090000";

    // sifive config
    config.dma_burst_length = 16;
    config.hw_dma_cap = HW_DMA_CAP_32B;
    config.caps = 0;
    config.clk_init = macb_sifive_clk_init;
    config.usrio_mii = 1 << MACB_MII_OFFSET;
    config.usrio_rmii = 1 << MACB_RMII_OFFSET;
    config.usrio_rgmii = 1 << GEM_RGMII_OFFSET;
    config.usrio_clken = 1 << MACB_CLKEN_OFFSET;
    NET_DEBUG("sifive config compelete.\n");

    // 给 tx_ring 和 rx_ring 各分配一页的空间
    Page *page;
    pageAlloc(&page);
    page->ref++;
    u64 tx_ring_dma = page2PA(page);
    DmaDesc *tx_ring = (DmaDesc *)tx_ring_dma;
    pageAlloc(&page);
    page->ref++;
    u64 rx_ring_dma = page2PA(page);
    DmaDesc *rx_ring = (DmaDesc *)rx_ring_dma;
    NET_DEBUG("alloc memory for the tx_ring and the rx_ring.\n");

    u64 rx_buffer_dma = 0;
    u64 paddr = 0;
    // 分配 buffer, 将 buffer 首地址登记到 rx_ring 和 recv_buffer 中
    for (int i = 0; i < MACB_RX_RING_SIZE; i++)
    {
        //
        if (i % 2 == 0)
        {
            pageAlloc(&page);
            page->ref++;
            paddr = page2PA(page);
            if (i == 0) rx_buffer_dma = paddr;
        }
        // 对于奇数 buffer，他在一页的 2048 偏移处
        else
        {
            paddr += buffer_size;
        }

        if (i == MACB_RX_RING_SIZE - 1)
        {
            paddr |= 1 << MACB_RX_WRAP_OFFSET;
        }

        rx_ring[i].ctrl = 0;
        rx_ring[i].addr = paddr;
        recv_buffers[i] = paddr;
    }
    LOAD_DEBUG("Set ring desc buffer for RX\n");

    // 和上面同理
    u64 tx_buffer_dma = 0;
    for (int i = 0; i < MACB_TX_RING_SIZE; ++i)
    {
        if (i % 2 == 0)
        {
            pageAlloc(&page);
            page->ref++;
            paddr = page2PA(page);
            if (i == 0) tx_buffer_dma = paddr;
        }
        else
        {
            paddr = paddr + buffer_size;
        }

        tx_ring[i].addr = paddr;

        if (i == MACB_TX_RING_SIZE - 1)
        {
            tx_ring[i].ctrl = (1 << MACB_TX_USED_OFFSET) | (1 << MACB_TX_WRAP_OFFSET);
        }
        else
        {
            tx_ring[i].ctrl = (1 << MACB_TX_USED_OFFSET);
        }
        // Used – must be zero for the controller to read data to the transmit buffer.
        // The controller sets this to one for the first buffer of a frame once it has been successfully transmitted.
        // Software must clear this bit before the buffer can be used again.

        send_buffers[i] = paddr;
    }
    LOAD_DEBUG("Set ring desc buffer for TX\n");

    // 分配 dummy desc
    pageAlloc(&page);
    page->ref++;
    u64 dummy_desc_dma = page2PA(page);
    dummy_desc = (DmaDesc *)dummy_desc_dma;
    LOAD_DEBUG("dummy desc at 0x%lx\n", dummy_desc_dma);

    // 填写 macb
    u64 pclk_rate = 125125000; // from eth_macb.rs
    macb.regs = MACB_IOBASE;
    macb.is_big_endian = is_big_endian();
    macb.config = &config;
    macb.rx_tail = 0;
    macb.tx_head = 0;
    macb.tx_tail = 0;
    macb.next_rx_tail = 0;
    macb.wrapped = false;
    macb.recv_buffers = recv_buffers;
    macb.send_buffers = send_buffers;
    macb.rx_ring = rx_ring;
    macb.tx_ring = tx_ring;
    macb.buffer_size = buffer_size;
    macb.rx_buffer_dma = rx_buffer_dma;
    macb.tx_buffer_dma = tx_buffer_dma;
    macb.rx_ring_dma = rx_ring_dma;
    macb.tx_ring_dma = tx_ring_dma;
    macb.dummy_desc = dummy_desc;
    macb.dummy_desc_dma = dummy_desc_dma;
    macb.phy_addr = 0;
    macb.pclk_rate = pclk_rate;
    macb.phy_interface = PhyInterfaceMode_GMII;
    LOAD_DEBUG("rx_buffer_dma is 0x%lx, tx_ring_dma is 0x%lx, dummy_desc_dma is 0x%lx\n",
               rx_buffer_dma, tx_buffer_dma, dummy_desc_dma);
    // 登记 RBQ 和 TBQ
    *MACB_REG(MACB_RBQP) = (u32)rx_ring_dma;
    *MACB_REG(MACB_TBQP) = (u32)tx_ring_dma;

    u32 val = 0;
    if (macb_is_gem())
    {
        gmac_configure_dma();
        gmac_init_multi_queues();
        if ((macb.phy_interface == PhyInterfaceMode_RGMII)
            || (macb.phy_interface == PhyInterfaceMode_RGMII_ID)
            || (macb.phy_interface == PhyInterfaceMode_RGMII_RXID)
            || (macb.phy_interface == PhyInterfaceMode_RGMII_TXID))
        {
            val = macb.config->usrio_rgmii;
        }
        else if (macb.phy_interface
                 == PhyInterfaceMode_RMII)
        {
            val = macb.config->usrio_rmii;
        }
        else if (macb.phy_interface
                 == PhyInterfaceMode_MII)
        {
            val = macb.config->usrio_mii;
        }

        if ((macb.config->caps & ((u32)MACB_CAPS_USRIO_HAS_CLKEN)) != 0)
        {
            val |= macb.config->usrio_clken;
        }

        *MACB_REG(GEM_USRIO) = val;

        if (macb.phy_interface == PhyInterfaceMode_SGMII)
        {
            u32 ncfgr = readv((u32 *)(MACB_IOBASE + MACB_NCFGR));
            ncfgr |= (1 << GEM_SGMIIEN_OFFSET) | (1 << GEM_PCSSEL_OFFSET);
            NET_DEBUG("Write MACB_NCFGR: 0x%x when SGMII\n", ncfgr);
            *MACB_REG(MACB_NCFGR) = ncfgr;
        }
        else
        {
            if (macb.phy_interface == PhyInterfaceMode_RMII)
            {
                *MACB_REG(MACB_USRIO) = 0;
            }
            else
            {
                *MACB_REG(MACB_USRIO) = macb.config->usrio_mii;
            }
        }
    }

    i32 ret = macb_phy_init(name);
    if (ret != 0)
    {
        panic("macb_phy_init returned: %d in failure", ret);
    }

    NET_DEBUG("Enable TX and RX\n");
    *MACB_REG(MACB_NCR) = (1 << MACB_TE_OFFSET) | (1 << MACB_RE_OFFSET);
    fence();

    u32 nsr = *MACB_REG(MACB_NSR);
    u32 tsr = *MACB_REG(MACB_TSR);

    NET_DEBUG("MACB_NSR: {:#x}, MACB_TSR: {:#x}", nsr, tsr);
    msdelay(90);
}

void macbInit()
{
    NET_DEBUG("macb init begin.\n");
    sifive_prci_set_rate();
    sifive_prci_enable();
    sifive_prci_ethernet_release_reset();

    macb_eth_probe();
    macb_start();
    panic("");
}
