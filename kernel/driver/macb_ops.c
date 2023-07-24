#include <types.h>
#include <macb.h>
#include <delay.h>
#include <debug.h>

#define GEM_RX_BUFFER_SIZE 2048

#define HW_DMA_CAP_32B 0

#define MACB_MII_OFFSET 0
#define MACB_RMII_OFFSET 0
#define GEM_RGMII_OFFSET 0
#define MACB_CLKEN_OFFSET 0

#define MACB_MID 0x00fc
#define MACB_IDNUM_OFFSET 16
#define MACB_IDNUM_SIZE 12

#define GEMGXL_PLL_CFG 0x1c
#define GEMGXL_PLL_OUTDIV 0x20

#define PRCI_RESETREG 0x28
#define PRCI_PROCMONCFG 0xF0
#define PRCI_PROCMONCFG_CORE_CLOCK_MASK (1 << 24)

typedef enum
{
    PHY_INTERFACE_MODE_NA, /* don't touch */
    PHY_INTERFACE_MODE_INTERNAL,
    PHY_INTERFACE_MODE_MII,
    PHY_INTERFACE_MODE_GMII,
    PHY_INTERFACE_MODE_SGMII,
    PHY_INTERFACE_MODE_TBI,
    PHY_INTERFACE_MODE_REVMII,
    PHY_INTERFACE_MODE_RMII,
    PHY_INTERFACE_MODE_REVRMII,
    PHY_INTERFACE_MODE_RGMII,
    PHY_INTERFACE_MODE_RGMII_ID,
    PHY_INTERFACE_MODE_RGMII_RXID,
    PHY_INTERFACE_MODE_RGMII_TXID,
    PHY_INTERFACE_MODE_RTBI,
    PHY_INTERFACE_MODE_SMII,
    PHY_INTERFACE_MODE_XGMII,
    PHY_INTERFACE_MODE_XLGMII,
    PHY_INTERFACE_MODE_MOCA,
    PHY_INTERFACE_MODE_QSGMII,
    PHY_INTERFACE_MODE_TRGMII,
    PHY_INTERFACE_MODE_100BASEX,
    PHY_INTERFACE_MODE_1000BASEX,
    PHY_INTERFACE_MODE_2500BASEX,
    PHY_INTERFACE_MODE_5GBASER,
    PHY_INTERFACE_MODE_RXAUI,
    PHY_INTERFACE_MODE_XAUI,
    /* 10GBASE-R, XFI, SFI - single lane 10G Serdes */
    PHY_INTERFACE_MODE_10GBASER,
    PHY_INTERFACE_MODE_25GBASER,
    PHY_INTERFACE_MODE_USXGMII,
} phy_interface_t;

typedef struct macb_config
{
    unsigned int dma_burst_length;
    unsigned int hw_dma_cap;
    unsigned int caps;

    void (*clk_init)(u32 rate);
    // macb_usrio_cfg
    unsigned int usrio_mii;
    unsigned int usrio_rmii;
    unsigned int usrio_rgmii;
    unsigned int usrio_clken;
} macb_config;

typedef struct macb_device
{
    void *regs; // MACB_IOBASE，诸多寄存器的起始地址

    bool is_big_endian; // false

    const struct macb_config *config;

    unsigned int rx_tail;
    unsigned int tx_head;
    unsigned int tx_tail;
    unsigned int next_rx_tail;
    bool wrapped;

    void *rx_buffer;
    void *tx_buffer;
    struct macb_dma_desc *rx_ring;
    struct macb_dma_desc *tx_ring;
    u32 rx_buffer_size;

    unsigned long rx_buffer_dma;
    unsigned long rx_ring_dma;
    unsigned long tx_ring_dma;

    struct macb_dma_desc *dummy_desc;
    unsigned long dummy_desc_dma;

    const struct device *dev;
    unsigned short phy_addr;
    struct mii_dev *bus;
    struct phy_device *phydev;

    unsigned long pclk_rate;
    phy_interface_t phy_interface;
} macb_device;

/**
 * @brief 初始化 GEMGXL TX 时钟。SiFive GEMGXL TX 有两种模式：
 * 0 = GMII mode. Use 125 MHz gemgxlclk from PRCI in TX logic and output clock on GMII output signal GTX_CLK
 * 1 = MII mode. Use MII input signal TX_CLK in TX logic
 *
 * @param rate 时钟频率
 */
void macb_sifive_clk_init(u32 rate)
{
    int mod = (rate == 125000000) ? 0 : 1;
    *GEMGXL_REG(0) = mod;
}

// int macb_start()
// {
//     int buffer_size = GEM_RX_BUFFER_SIZE;
//     char *name = "ethernet@10090000";
//     macb_config config = {
//         .dma_burst_length = 16,
//         .hw_dma_cap = HW_DMA_CAP_32B,
//         .caps = 0,
//         .clk_init = macb_sifive_clk_init,
//         .usrio_mii = 1 << MACB_MII_OFFSET,
//         .usrio_rmii = 1 << MACB_RMII_OFFSET,
//         .usrio_rgmii = 1 << GEM_RGMII_OFFSET,
//         .usrio_clken = 1 << MACB_CLKEN_OFFSET,
//     };
// }

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

static void macb_eth_probe()
{
    macb_is_gem();
    // // rx/tx ring dma
    // // rx_buffer_size: 2048
    // u32 id = 0;
    // u32 ncfgr = 0; // ncfgr, network config register

    // // RX==receive，接收，从开启到现在接收封包的情况，是下行流量（Downlink）
    // // TX==Transmit，发送，从开启到现在发送封包的情况，是上行流量（Uplink）

    // u64 pclk_rate = 125125000;
    // if (macb_is_gem())
    // {
    //     ncfgr = gem_mdc_clk_div(id, pclk_rate);
    //     ncfgr |= macb_dbw();
    // }
    // else
    // {
    //     ncfgr = macb_mdc_clk_div(id, pclk_rate);
    // }

    // // info !("macb_eth_initialize to write MACB_NCFGR: {:#x}", ncfgr);
    // // writev((MACB_IOBASE + MACB_NCFGR) as * mut u32, ncfgr);
}

void macb_init()
{
    NET_DEBUG("macb init begin.\n");
    sifive_prci_set_rate();
    sifive_prci_enable();
    sifive_prci_ethernet_release_reset();
    printk("long long size is %d, short size is %d, long size is %d\n", sizeof(long long), sizeof(short), sizeof(long));
    macb_eth_probe();
    panic("");
}
