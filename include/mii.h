#ifndef _MII_CONST_H
#define _MII_CONST_H

#include "types.h"
// linux/mii.h: definitions for MII-compatible transceivers

/* Generic MII registers. */
#define MII_BMCR (0x00)        /* Basic mode control register */
#define MII_BMSR (0x01)        /* Basic mode status register  */
#define MII_PHYSID1 (0x02)     /* PHYS ID 1                   */
#define MII_PHYSID2 (0x03)     /* PHYS ID 2                   */
#define MII_ADVERTISE (0x04)   /* Advertisement control reg   */
#define MII_LPA (0x05)         /* Link partner ability reg    */
#define MII_EXPANSION (0x06)   /* Expansion register          */
#define MII_CTRL1000 (0x09)    /* 1000BASE-T control          */
#define MII_STAT1000 (0x0a)    /* 1000BASE-T status           */
#define MII_MMD_CTRL (0x0d)    /* MMD Access Control Register */
#define MII_MMD_DATA (0x0e)    /* MMD Access Data Register */
#define MII_ESTATUS (0x0f)     /* Extended Status             */
#define MII_DCOUNTER (0x12)    /* Disconnect counter          */
#define MII_FCSCOUNTER (0x13)  /* False carrier counter       */
#define MII_NWAYTEST (0x14)    /* N-way auto-neg test reg     */
#define MII_RERRCOUNTER (0x15) /* Receive error counter       */
#define MII_SREVISION (0x16)   /* Silicon revision            */
#define MII_RESV1 (0x17)       /* Reserved...                 */
#define MII_LBRERROR (0x18)    /* Lpback, rx, bypass error    */
#define MII_PHYADDR (0x19)     /* PHY address                 */
#define MII_RESV2 (0x1a)       /* Reserved...                 */
#define MII_TPISTATUS (0x1b)   /* TPI status for 10mbps       */
#define MII_NCONFIG (0x1c)     /* Network interface config    */

/* Basic mode control register. */
#define BMCR_RESV (0x003f)      /* Unused...                   */
#define BMCR_SPEED1000 (0x0040) /* MSB of Speed (1000)         */
#define BMCR_CTST (0x0080)      /* Collision test              */
#define BMCR_FULLDPLX (0x0100)  /* Full duplex                 */
#define BMCR_ANRESTART (0x0200) /* Auto negotiation restart    */
#define BMCR_ISOLATE (0x0400)   /* Isolate data paths from MII */
#define BMCR_PDOWN (0x0800)     /* Enable low power state      */
#define BMCR_ANENABLE (0x1000)  /* Enable auto negotiation     */
#define BMCR_SPEED100 (0x2000)  /* Select 100Mbps              */
#define BMCR_LOOPBACK (0x4000)  /* TXD loopback bits           */
#define BMCR_RESET (0x8000)     /* Reset to default state      */
#define BMCR_SPEED10 (0x0000)   /* Select 10Mbps               */

/* Basic mode status register. */
#define BMSR_ERCAP (0x0001)        /* Ext-reg capability          */
#define BMSR_JCD (0x0002)          /* Jabber detected             */
#define BMSR_LSTATUS (0x0004)      /* Link status                 */
#define BMSR_ANEGCAPABLE (0x0008)  /* Able to do auto-negotiation */
#define BMSR_RFAULT (0x0010)       /* Remote fault detected       */
#define BMSR_ANEGCOMPLETE (0x0020) /* Auto-negotiation complete   */
#define BMSR_RESV (0x00c0)         /* Unused...                   */
#define BMSR_ESTATEN (0x0100)      /* Extended Status in R15      */
#define BMSR_100HALF2 (0x0200)     /* Can do 100BASE-T2 HDX       */
#define BMSR_100FULL2 (0x0400)     /* Can do 100BASE-T2 FDX       */
#define BMSR_10HALF (0x0800)       /* Can do 10mbps, half-duplex  */
#define BMSR_10FULL (0x1000)       /* Can do 10mbps, full-duplex  */
#define BMSR_100HALF (0x2000)      /* Can do 100mbps, half-duplex */
#define BMSR_100FULL (0x4000)      /* Can do 100mbps, full-duplex */
#define BMSR_100BASE4 (0x8000)     /* Can do 100mbps, 4k packets  */

/* Advertisement control register. */
#define ADVERTISE_SLCT (0x001f)          /* Selector bits               */
#define ADVERTISE_CSMA (0x0001)          /* Only selector supported     */
#define ADVERTISE_10HALF (0x0020)        /* Try for 10mbps half-duplex  */
#define ADVERTISE_1000XFULL (0x0020)     /* Try for 1000BASE-X full-duplex */
#define ADVERTISE_10FULL (0x0040)        /* Try for 10mbps full-duplex  */
#define ADVERTISE_1000XHALF (0x0040)     /* Try for 1000BASE-X half-duplex */
#define ADVERTISE_100HALF (0x0080)       /* Try for 100mbps half-duplex */
#define ADVERTISE_1000XPAUSE (0x0080)    /* Try for 1000BASE-X pause    */
#define ADVERTISE_100FULL (0x0100)       /* Try for 100mbps full-duplex */
#define ADVERTISE_1000XPSE_ASYM (0x0100) /* Try for 1000BASE-X asym pause */
#define ADVERTISE_100BASE4 (0x0200)      /* Try for 100mbps 4k packets  */
#define ADVERTISE_PAUSE_CAP (0x0400)     /* Try for pause               */
#define ADVERTISE_PAUSE_ASYM (0x0800)    /* Try for asymetric pause     */
#define ADVERTISE_RESV (0x1000)          /* Unused...                   */
#define ADVERTISE_RFAULT (0x2000)        /* Say we can detect faults    */
#define ADVERTISE_LPACK (0x4000)         /* Ack link partners response  */
#define ADVERTISE_NPAGE (0x8000)         /* Next page bit               */

#define ADVERTISE_FULL (ADVERTISE_100FULL | ADVERTISE_10FULL | ADVERTISE_CSMA)
#define ADVERTISE_ALL (ADVERTISE_10HALF | ADVERTISE_10FULL | ADVERTISE_100HALF | ADVERTISE_100FULL)

/* Link partner ability register. */
#define LPA_SLCT (0x001f)            /* Same as advertise selector  */
#define LPA_10HALF (0x0020)          /* Can do 10mbps half-duplex   */
#define LPA_1000XFULL (0x0020)       /* Can do 1000BASE-X full-duplex */
#define LPA_10FULL (0x0040)          /* Can do 10mbps full-duplex   */
#define LPA_1000XHALF (0x0040)       /* Can do 1000BASE-X half-duplex */
#define LPA_100HALF (0x0080)         /* Can do 100mbps half-duplex  */
#define LPA_1000XPAUSE (0x0080)      /* Can do 1000BASE-X pause     */
#define LPA_100FULL (0x0100)         /* Can do 100mbps full-duplex  */
#define LPA_1000XPAUSE_ASYM (0x0100) /* Can do 1000BASE-X pause asym*/
#define LPA_100BASE4 (0x0200)        /* Can do 100mbps 4k packets   */
#define LPA_PAUSE_CAP (0x0400)       /* Can pause                   */
#define LPA_PAUSE_ASYM (0x0800)      /* Can pause asymetrically     */
#define LPA_RESV (0x1000)            /* Unused...                   */
#define LPA_RFAULT (0x2000)          /* Link partner faulted        */
#define LPA_LPACK (0x4000)           /* Link partner acked us       */
#define LPA_NPAGE (0x8000)           /* Next page bit               */

#define LPA_DUPLEX (LPA_10FULL | LPA_100FULL)
#define LPA_100 (LPA_100FULL | LPA_100HALF | LPA_100BASE4)

/* Expansion register for auto-negotiation. */
#define EXPANSION_NWAY (0x0001)        /* Can do N-way auto-nego      */
#define EXPANSION_LCWP (0x0002)        /* Got new RX page code word   */
#define EXPANSION_ENABLENPAGE (0x0004) /* This enables npage words    */
#define EXPANSION_NPCAPABLE (0x0008)   /* Link partner supports npage */
#define EXPANSION_MFAULTS (0x0010)     /* Multiple faults detected    */
#define EXPANSION_RESV (0xffe0)        /* Unused...                   */

#define ESTATUS_1000_XFULL (0x8000) /* Can do 1000BaseX Full       */
#define ESTATUS_1000_XHALF (0x4000) /* Can do 1000BaseX Half       */
#define ESTATUS_1000_TFULL (0x2000) /* Can do 1000BT Full          */
#define ESTATUS_1000_THALF (0x1000) /* Can do 1000BT Half          */

/* N-way test register. */
#define NWAYTEST_RESV1 (0x00ff)    /* Unused...                   */
#define NWAYTEST_LOOPBACK (0x0100) /* Enable loopback for N-way   */
#define NWAYTEST_RESV2 (0xfe00)    /* Unused...                   */

/* MAC and PHY tx_config_Reg[15:0] for SGMII in-band auto-negotiation.*/
#define ADVERTISE_SGMII (0x0001)        /* MAC can do SGMII            */
#define LPA_SGMII (0x0001)              /* PHY can do SGMII            */
#define LPA_SGMII_SPD_MASK (0x0c00)     /* SGMII speed mask            */
#define LPA_SGMII_FULL_DUPLEX (0x1000)  /* SGMII full duplex           */
#define LPA_SGMII_DPX_SPD_MASK (0x1C00) /* SGMII duplex and speed bits */
#define LPA_SGMII_10 (0x0000)           /* 10Mbps                      */
#define LPA_SGMII_10HALF (0x0000)       /* Can do 10mbps half-duplex   */
#define LPA_SGMII_10FULL (0x1000)       /* Can do 10mbps full-duplex   */
#define LPA_SGMII_100 (0x0400)          /* 100Mbps                     */
#define LPA_SGMII_100HALF (0x0400)      /* Can do 100mbps half-duplex  */
#define LPA_SGMII_100FULL (0x1400)      /* Can do 100mbps full-duplex  */
#define LPA_SGMII_1000 (0x0800)         /* 1000Mbps                    */
#define LPA_SGMII_1000HALF (0x0800)     /* Can do 1000mbps half-duplex */
#define LPA_SGMII_1000FULL (0x1800)     /* Can do 1000mbps full-duplex */
#define LPA_SGMII_LINK (0x8000)         /* PHY link with copper-side partner */

/* 1000BASE-T Control register */
#define ADVERTISE_1000FULL (0x0200)    /* Advertise 1000BASE-T full duplex */
#define ADVERTISE_1000HALF (0x0100)    /* Advertise 1000BASE-T half duplex */
#define CTL1000_PREFER_MASTER (0x0400) /* prefer to operate as master */
#define CTL1000_AS_MASTER (0x0800)
#define CTL1000_ENABLE_MASTER (0x1000)

/* 1000BASE-T Status register */
#define LPA_1000MSFAIL (0x8000)    /* Master/Slave resolution failure */
#define LPA_1000MSRES (0x4000)     /* Master/Slave resolution status */
#define LPA_1000LOCALRXOK (0x2000) /* Link partner local receiver status */
#define LPA_1000REMRXOK (0x1000)   /* Link partner remote receiver status */
#define LPA_1000FULL (0x0800)      /* Link partner 1000BASE-T full duplex */
#define LPA_1000HALF (0x0400)      /* Link partner 1000BASE-T half duplex */

/* Flow control flags */
#define FLOW_CTRL_TX (0x01)
#define FLOW_CTRL_RX (0x02)

/* MMD Access Control register fields */
#define MII_MMD_CTRL_DEVAD_MASK (0x1f)   /* Mask MMD DEVAD*/
#define MII_MMD_CTRL_ADDR (0x0000)       /* Address */
#define MII_MMD_CTRL_NOINCR (0x4000)     /* no post increment */
#define MII_MMD_CTRL_INCR_RDWT (0x8000)  /* post increment on reads & writes */
#define MII_MMD_CTRL_INCR_ON_WT (0xC000) /* post increment on writes only */

// MDIO
#define MDIO_PRTAD_NONE (-1)
#define MDIO_DEVAD_NONE (-1)
#define MDIO_EMULATE_C22 (4)

// PHY
#define PHY_FIXED_ID (0xa5a55a5a)
#define PHY_NCSI_ID (0xbeefcafe)
#define PHY_GMII2RGMII_ID (0x5a5a5a5a)
#define PHY_MAX_ADDR (32)
#define PHY_FLAG_BROKEN_RESET (1 << 0) /* soft reset not supported */

/* phy seed setup */
#define AUTO (99)
#define _1000BASET (1000)
#define _100BASET (100)
#define _10BASET (10)
#define HALF (22)
#define FULL (44)

/* phy register offsets */
#define MII_MIPSCR (0x11)

/* MII_LPA */
#define PHY_ANLPAR_PSB_802_3 (0x0001)
#define PHY_ANLPAR_PSB_802_9 (0x0002)

/* MII_CTRL1000 masks */
#define PHY_1000BTCR_1000FD (0x0200)
#define PHY_1000BTCR_1000HD (0x0100)

/* MII_STAT1000 masks */
#define PHY_1000BTSR_MSCF (0x8000)
#define PHY_1000BTSR_MSCR (0x4000)
#define PHY_1000BTSR_LRS (0x2000)
#define PHY_1000BTSR_RRS (0x1000)
#define PHY_1000BTSR_1000FD (0x0800)
#define PHY_1000BTSR_1000HD (0x0400)

/* phy EXSR */
#define ESTATUS_1000XF (0x8000)
#define ESTATUS_1000XH (0x4000)

/* Indicates what features are supported by the interface. */
#define SUPPORTED_10baseT_Half (1 << 0)
#define SUPPORTED_10baseT_Full (1 << 1)
#define SUPPORTED_100baseT_Half (1 << 2)
#define SUPPORTED_100baseT_Full (1 << 3)
#define SUPPORTED_1000baseT_Half (1 << 4)
#define SUPPORTED_1000baseT_Full (1 << 5)
#define SUPPORTED_Autoneg (1 << 6)
#define SUPPORTED_TP (1 << 7)
#define SUPPORTED_AUI (1 << 8)
#define SUPPORTED_MII (1 << 9)
#define SUPPORTED_FIBRE (1 << 10)
#define SUPPORTED_BNC (1 << 11)
#define SUPPORTED_10000baseT_Full (1 << 12)
#define SUPPORTED_Pause (1 << 13)
#define SUPPORTED_Asym_Pause (1 << 14)
#define SUPPORTED_2500baseX_Full (1 << 15)
#define SUPPORTED_Backplane (1 << 16)
#define SUPPORTED_1000baseKX_Full (1 << 17)
#define SUPPORTED_10000baseKX4_Full (1 << 18)
#define SUPPORTED_10000baseKR_Full (1 << 19)
#define SUPPORTED_10000baseR_FEC (1 << 20)
#define SUPPORTED_1000baseX_Half (1 << 21)
#define SUPPORTED_1000baseX_Full (1 << 22)

/* Indicates what features are advertised by the interface. */
#define ADVERTISED_10baseT_Half (1 << 0)
#define ADVERTISED_10baseT_Full (1 << 1)
#define ADVERTISED_100baseT_Half (1 << 2)
#define ADVERTISED_100baseT_Full (1 << 3)
#define ADVERTISED_1000baseT_Half (1 << 4)
#define ADVERTISED_1000baseT_Full (1 << 5)
#define ADVERTISED_Autoneg (1 << 6)
#define ADVERTISED_TP (1 << 7)
#define ADVERTISED_AUI (1 << 8)
#define ADVERTISED_MII (1 << 9)
#define ADVERTISED_FIBRE (1 << 10)
#define ADVERTISED_BNC (1 << 11)
#define ADVERTISED_10000baseT_Full (1 << 12)
#define ADVERTISED_Pause (1 << 13)
#define ADVERTISED_Asym_Pause (1 << 14)
#define ADVERTISED_2500baseX_Full (1 << 15)
#define ADVERTISED_Backplane (1 << 16)
#define ADVERTISED_1000baseKX_Full (1 << 17)
#define ADVERTISED_10000baseKX4_Full (1 << 18)
#define ADVERTISED_10000baseKR_Full (1 << 19)
#define ADVERTISED_10000baseR_FEC (1 << 20)
#define ADVERTISED_1000baseX_Half (1 << 21)
#define ADVERTISED_1000baseX_Full (1 << 22)

/* The forced speed, 10Mb, 100Mb, gigabit, 2.5Gb, 10GbE. */
#define SPEED_10 (10)
#define SPEED_100 (100)
#define SPEED_1000 (1000)
#define SPEED_2500 (2500)
#define SPEED_10000 (10000)
#define SPEED_14000 (14000)
#define SPEED_20000 (20000)
#define SPEED_25000 (25000)
#define SPEED_40000 (40000)
#define SPEED_50000 (50000)
#define SPEED_56000 (56000)
#define SPEED_100000 (100000)
#define SPEED_200000 (200000)

/* Duplex, half or full. */
#define DUPLEX_HALF (0x00)
#define DUPLEX_FULL (0x01)

#endif