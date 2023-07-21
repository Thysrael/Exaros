#ifndef _ETH_MACB_OPS_H
#define _ETH_MACB_OPS_H

#include "types.h"

#define MACB_RX_BUFFER_SIZE (128)
#define GEM_RX_BUFFER_SIZE (2048)

#define RX_BUFFER_MULTIPLE (64)

#define MACB_RX_RING_SIZE (32)
#define MACB_TX_RING_SIZE (16)

#define MACB_TX_TIMEOUT (1000)
#define MACB_AUTONEG_TIMEOUT (5000000)

#define HW_DMA_CAP_32B (0)
#define HW_DMA_CAP_64B (1)
#define DMA_DESC_SIZE (16)

#define RXBUF_FRMLEN_MASK (0x00000fff)
#define TXBUF_FRMLEN_MASK (0x000007ff)

#define _MII (2)
#define _GMII (3)
#define _RMII (7)
#define _RGMII (8)

#define HW_DMA_CAP (0)

typedef struct DmaDesc
{
    u32 addr;
    u32 ctrl;
} DmaDesc;
typedef struct DmaDesc64
{
    u32 addrh;
    u32 unused;
} DmaDesc64;

typedef enum PhyInterfaceMode
{
    PhyInterfaceMode_MII = (0),
    PhyInterfaceMode_GMII = (1),
    PhyInterfaceMode_SGMII = (2),
    PhyInterfaceMode_SGMII_2500 = (3),
    PhyInterfaceMode_QSGMII = (4),
    PhyInterfaceMode_TBI = (5),
    PhyInterfaceMode_RMII = (6),
    PhyInterfaceMode_RGMII = (7),
    PhyInterfaceMode_RGMII_ID = (8),
    PhyInterfaceMode_RGMII_RXID = (9),
    PhyInterfaceMode_RGMII_TXID = (10),
    PhyInterfaceMode_RTBI = (11),
    PhyInterfaceMode_BASEX1000 = (12),
    PhyInterfaceMode_BASEX2500 = (13),
    PhyInterfaceMode_XGMII = (14),
    PhyInterfaceMode_XAUI = (15),
    PhyInterfaceMode_RXAUI = (16),
    PhyInterfaceMode_SFI = (17),
    PhyInterfaceMode_INTERNAL = (18),
    PhyInterfaceMode_AUI_25G = (19),
    PhyInterfaceMode_XLAUI = (20),
    PhyInterfaceMode_CAUI2 = (21),
    PhyInterfaceMode_CAUI4 = (22),
    PhyInterfaceMode_NCSI = (23),
    PhyInterfaceMode_BASER10G = (24),
    PhyInterfaceMode_USXGMII = (25),
    PhyInterfaceMode_NONE = (26), /* Must be last */
    PhyInterfaceMode_COUNT = (27),
} PhyInterfaceMode;

typedef struct MacbConfig
{
    u32 dma_burst_length;
    u32 hw_dma_cap;
    u32 caps;
    void (*clk_init)(u64); // fn clk_init
                           // Box<dyn Fn(u64)> // clk_init;
    u32 usrio_mii;
    u32 usrio_rmii;
    u32 usrio_rgmii;
    u32 usrio_clken;
} MacbConfig;

typedef struct MacbDevice
{
    u32 regs;
    bool is_big_endian;
    MacbConfig config;

    u64 rx_tail;
    u64 tx_head;
    u64 tx_tail;
    u64 next_rx_tail;
    bool wrapped;
    u64 *recv_buffers;
    u64 *send_buffers;
    DmaDesc *rx_ring;
    DmaDesc *tx_ring;
    u64 buffer_size;
    u64 rx_buffer_dma;
    u64 tx_buffer_dma;
    u64 rx_ring_dma;
    u64 tx_ring_dma;
    DmaDesc *dummy_desc;
    u64 dummy_desc_dma;
    u32 phy_addr;
    u64 pclk_rate;
    PhyInterfaceMode phy_interface;
} MacbDevice;

void macb_start();
i32 macb_send(u8 *packet, u32 len);
i32 macb_recv(u8 *packet);
void reclaim_rx_buffers(u64 new_tail);
void macb_invalidate_ring_desc();
void invalidate_dcache_range();
i32 macb_phy_init(char *name);
i32 macb_phy_find();
void macb_phy_reset(char *name);
void phy_connect_dev();
i32 phy_reset(u32 phydev_addr, PhyInterfaceMode _interface, u32 phydev_flags);
void phy_config();
void macb_mdio_write(u32 phy_adr, u32 reg, u16 value);
u16 macb_mdio_read(u32 phy_adr, u32 reg);
void macb_sifive_clk_init(u64 rate);
i32 macb_linkspd_cb(u32 speed);
u32 mii_nway_result(u32 negotiated);
i32 gmac_configure_dma();
void gmac_init_multi_queues();
void flush_dcache_range();
bool is_big_endian();
u32 upper_32_bits(u64 n);
u32 lower_32_bits(u64 n);

#endif