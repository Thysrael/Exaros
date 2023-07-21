#include "eth_macb_ops.h"
#include "eth_macb.h"
#include "mii_const.h"
#include "macb_const.h"
#include "phy_mscc.h"
#include "driver.h"
#include "memory.h"

MacbDevice macb;
MacbConfig config;

// void macb_sifive_clk_init(u64 rate)
// {
//     u32 mode = (rate == 125000000) ? 0 : 1;
//     writev((u32 *)GEMGXL_BASE, mode);
// }

u64 send_buffers[32];
u64 recv_buffers[64];

DmaDesc *dummy_desc;

void macb_start()
{
    printk("MACB init start\n");

    u32 buffer_size = GEM_RX_BUFFER_SIZE;
    char name[] = "ethernet@10090000";
    config.dma_burst_length = 16;

    config.hw_dma_cap = HW_DMA_CAP_32B;
    config.caps = 0;
    // config.clk_init = macb_sifive_clk_init;
    config.usrio_mii = 1 << MACB_MII_OFFSET;
    config.usrio_rmii = 1 << MACB_RMII_OFFSET;
    config.usrio_rgmii = 1 << GEM_RGMII_OFFSET;
    config.usrio_clken = 1 << MACB_CLKEN_OFFSET;

    i32 count = 0;
    u64 paddr = 0;
    Page *page;

    if (pageAlloc(&page) != 0) { panic("[MACB] Alloc Error!"); }
    page->ref++;
    u64 tx_ring_dma = page2PA(page);
    DmaDesc *tx_ring = (DmaDesc *)tx_ring_dma;

    if (pageAlloc(&page) != 0) { panic("[MACB] Alloc Error!"); }
    page->ref++;
    u64 rx_ring_dma = page2PA(page);
    DmaDesc *rx_ring = (DmaDesc *)rx_ring_dma;

    u64 rx_buffer_dma = 0;
    printk("Set ring desc buffer for RX\n");
    for (int i = 0; i < MACB_RX_RING_SIZE; ++i)
    {
        if (i % 2 == 0)
        {
            if (pageAlloc(&page) != 0) { panic("[MACB] Alloc Error!"); }
            page->ref++;
            paddr = page2PA(page);
            if (i == 0) { rx_buffer_dma = paddr; }
        }
        else { paddr = paddr + buffer_size; }

        if (i == MACB_RX_RING_SIZE - 1)
        {
            paddr |= 1 << MACB_RX_WRAP_OFFSET;
        }

        if ((config.hw_dma_cap & HW_DMA_CAP_64B) != 0)
        {
            count = i * 2;
            rx_ring[count + 1].addr = upper_32_bits(paddr); // Fill DmaDesc64.addrh
        }
        else { count = i; }
        rx_ring[count].ctrl = 0;
        rx_ring[count].addr = lower_32_bits(paddr);

        recv_buffers[i] = paddr;
    }
    flush_dcache_range();

    count = 0;
    u64 tx_buffer_dma = 0;
    printk("Set ring desc buffer for TX\n");
    for (int i = 0; i < MACB_TX_RING_SIZE; ++i)
    {
        if (i % 2 == 0)
        {
            if (pageAlloc(&page) != 0) { panic("[MACB] Alloc Error!"); }
            page->ref++;
            paddr = page2PA(page);
            if (i == 0) { tx_buffer_dma = paddr; }
        }
        else { paddr = paddr + buffer_size; }

        if ((config.hw_dma_cap & HW_DMA_CAP_64B) != 0)
        {
            count = i * 2;
            tx_ring[count + 1].addr = upper_32_bits(paddr); // Fill DmaDesc64.addrh
        }
        else { count = i; }
        tx_ring[count].addr = lower_32_bits(paddr);

        if (i == MACB_TX_RING_SIZE - 1)
        {
            tx_ring[count].ctrl = (1 << MACB_TX_USED_OFFSET) | (1 << MACB_TX_WRAP_OFFSET);
        }
        else
        {
            tx_ring[count].ctrl = (1 << MACB_TX_USED_OFFSET);
        }
        // Used – must be zero for the controller to read data to the transmit buffer.
        // The controller sets this to one for the first buffer of a frame once it has been successfully transmitted.
        // Software must clear this bit before the buffer can be used again.

        send_buffers[i] = paddr;
    }
    flush_dcache_range();

    printk("send_buffers length: {%d}, recv_buffers length = {%d}\n",
           sizeof(send_buffers) / sizeof(send_buffers[0]),
           sizeof(recv_buffers) / sizeof(recv_buffers[0]));

    if (pageAlloc(&page) != 0) { panic("[MACB] Alloc Error!"); }
    page->ref++;
    u64 dummy_desc_dma = page2PA(page);
    dummy_desc = (DmaDesc *)dummy_desc_dma;

    printk("0x%lx\n", dummy_desc_dma);

    u64 pclk_rate = 125125000; // from eth_macb.rs

    macb.regs = MACB_IOBASE;
    macb.is_big_endian = is_big_endian();
    macb.config = config;
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

    writev((u32 *)(MACB_IOBASE + MACB_RBQP), lower_32_bits(rx_ring_dma));
    writev((u32 *)(MACB_IOBASE + MACB_TBQP), lower_32_bits(tx_ring_dma));
    if ((macb.config.hw_dma_cap & HW_DMA_CAP_64B) != 0)
    {
        writev((u32 *)(MACB_IOBASE + MACB_RBQPH),
               upper_32_bits(rx_ring_dma));
        writev((u32 *)(MACB_IOBASE + MACB_TBQPH),
               upper_32_bits(tx_ring_dma));
    }

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
            val = macb.config.usrio_rgmii;
        }
        else if (macb.phy_interface
                 == PhyInterfaceMode_RMII)
        {
            val = macb.config.usrio_rmii;
        }
        else if (macb.phy_interface
                 == PhyInterfaceMode_MII)
        {
            val = macb.config.usrio_mii;
        }

        if ((macb.config.caps & ((u32)MACB_CAPS_USRIO_HAS_CLKEN)) != 0)
        {
            val |= macb.config.usrio_clken;
        }

        writev((u32 *)(MACB_IOBASE + GEM_USRIO), val);

        if (macb.phy_interface == PhyInterfaceMode_SGMII)
        {
            u32 ncfgr = readv((u32 *)(MACB_IOBASE + MACB_NCFGR));
            ncfgr |= (1 << GEM_SGMIIEN_OFFSET) | (1 << GEM_PCSSEL_OFFSET);
            printk("Write MACB_NCFGR: {%d} when SGMII\n", ncfgr);
            writev((u32 *)(MACB_IOBASE + MACB_NCFGR), ncfgr);
        }
        else
        {
            if (macb.phy_interface == PhyInterfaceMode_RMII)
            {
                writev((u32 *)(MACB_IOBASE + MACB_USRIO), 0);
            }
            else
            {
                writev((u32 *)(MACB_IOBASE + MACB_USRIO),
                       macb.config.usrio_mii);
            }
        }
    }
    i32 ret = macb_phy_init(name);
    if (ret != 0)
    {
        panic("macb_phy_init returned: {%d} in failure", ret);
    }

    printk("Enable TX and RX\n");
    writev((u32 *)(MACB_IOBASE + MACB_NCR),
           (1 << MACB_TE_OFFSET) | (1 << MACB_RE_OFFSET));
    fence();

    u32 nsr = readv((u32 *)(MACB_IOBASE + MACB_NSR));
    u32 tsr = readv((u32 *)(MACB_IOBASE + MACB_TSR));

    printk("MACB_NSR: {:#x}, MACB_TSR: {:#x}", nsr, tsr);
    msdelay(90);
}

i32 macb_send(u8 *packet, u32 len)
{
    u64 tx_head = macb.tx_head;
    u32 length = len;
    // let paddr: u64 = flush_dcache_range(packet, length); // DMA_TO_DEVICE
    // let paddr: u64 = packet.as_ptr() as u64;

    u32 ctrl = length & TXBUF_FRMLEN_MASK;

    // Last buffer, when set this bit indicates that the last buffer in the current frame has been reached.
    ctrl |= (1 << MACB_TX_LAST_OFFSET);
    // Clear Used bit
    ctrl &= ~(1 << MACB_TX_USED_OFFSET);

    if (tx_head == (MACB_TX_RING_SIZE - 1))
    {
        // ring的最后一个成员TX_WRAP
        // Wrap - marks last descriptor in transmit buffer descriptor list.
        // This can be set for any buffer within the frame.
        ctrl |= (1 << MACB_TX_WRAP_OFFSET);
        macb.tx_head = 0; // 预先把下一个head归为0
    }
    else { macb.tx_head += 1; }
    if ((macb.config.hw_dma_cap & HW_DMA_CAP_64B) != 0)
    {
        tx_head = tx_head * 2;
        // macb.tx_ring[tx_head + 1].addr = upper_32_bits(paddr); // DmaDesc64.addrh
    }

    u8 *txbuf = (u8 *)(macb.send_buffers + tx_head);
    for (int i = 0; i < length; ++i)
    {
        txbuf[i] = packet[i];
    }

    macb.tx_ring[tx_head].ctrl = ctrl;
    // 初始化时已经填过tx paddr
    // macb.tx_ring[tx_head].addr = lower_32_bits(paddr);

    fence();
    // barrier(); // For memory
    flush_dcache_range(); // TX ring dma desc
    printk("Send packet[{}] len: {%d}, addr: {%d}, DmaDesc: {%d}\n",
           tx_head, length, macb.send_buffers[tx_head], macb.tx_ring[tx_head]);

    writev((u32 *)(MACB_IOBASE + MACB_NCR),
           (1 << MACB_TE_OFFSET) | (1 << MACB_RE_OFFSET) | (1 << MACB_TSTART_OFFSET));

    u32 tsr = readv((u32 *)(MACB_IOBASE + MACB_TSR));
    printk("Tx MACB_TSR = {%d}\n", tsr);

    /*
     * I guess this is necessary because the networking core may
     * re-use the transmit buffer as soon as we return...
     */
    for (int i = 0; i < MACB_TX_TIMEOUT; ++i)
    {
        fence();
        // barrier();
        invalidate_dcache_range(); // TX ring dma desc
        ctrl = macb.tx_ring[tx_head].ctrl;
        if ((ctrl & (1 << MACB_TX_USED_OFFSET)) != 0)
        {
            if ((ctrl & (1 << MACB_TX_UNDERRUN_OFFSET)) != 0)
            {
                printk("TX underrun\n");
            }
            if ((ctrl & (1 << MACB_TX_BUF_EXHAUSTED_OFFSET)) != 0)
            {
                printk("TX buffers exhausted in mid frame\n");
            }
            printk("Tx {%d} desc.ctrl = {%d}\n", tx_head, ctrl);
            break;
        }
        usdelay(1);

        if (i == MACB_TX_TIMEOUT) { printk("TX timeout\n"); }
    }
    // dma_unmap_single(paddr, length, DMA_TO_DEVICE);
    return 0;
}

i32 macb_recv(u8 *packet)
{
    u32 status = 0;
    u32 length = 0;
    u32 count = 0;
    bool flag = false;

    u64 next_rx_tail = macb.next_rx_tail;
    macb.wrapped = false;
    while (1)
    {
        count += 1;
        macb_invalidate_ring_desc(); // RX DMA DESC
        if ((macb.config.hw_dma_cap & HW_DMA_CAP_64B) != 0)
        {
            next_rx_tail = next_rx_tail * 2;
        }
        if ((macb.rx_ring[next_rx_tail].addr & (1 << MACB_RX_USED_OFFSET)) == 0)
        {
            return -11; // EAGAIN
        }
        u64 indesc = next_rx_tail;
        status = macb.rx_ring[next_rx_tail].ctrl;
        if ((status & (1 << MACB_RX_SOF_OFFSET)) != 0)
        {
            if ((macb.config.hw_dma_cap & HW_DMA_CAP_64B) != 0)
            {
                next_rx_tail = next_rx_tail / 2;
                flag = true;
            }
            if (next_rx_tail != macb.rx_tail)
            {
                reclaim_rx_buffers(next_rx_tail);
            }
            macb.wrapped = false;
        }

        if ((status & (1 << MACB_RX_EOF_OFFSET)) != 0)
        {
            // buffer = macb.rx_buffer + macb.rx_buffer_size * macb.rx_tail;
            u64 buffer = macb.rx_buffer_dma + macb.buffer_size * macb.rx_tail;
            length = status & RXBUF_FRMLEN_MASK;

            invalidate_dcache_range(); // rx_buffer_dma
            if (macb.wrapped)
            {
                // u64 headlen = macb.buffer_size * (MACB_RX_RING_SIZE - macb.rx_tail);
                // u32 taillen = length - headlen;
                /*
                memcpy((void *)net_rx_packets[0],
                       buffer, headlen);
                memcpy((void *)net_rx_packets[0] + headlen,
                       macb->rx_buffer, taillen);
                *packetp = (void *)net_rx_packets[0];
                */
                printk("recv wrapped net_rx_packets is not implemented\n");
            }
            else
            {
                //*packet = buffer;
                // 把 DMA buffer中的网络包拷贝出来
                u8 *rx_packets = (u8 *)buffer;
                for (int i = 0; i < length; ++i)
                {
                    packet[i] = rx_packets[i];
                }
            }
            printk("Recv packet[{%d}] count: {%d}, length: {%d}, {%d}",
                   indesc, count, length, macb.rx_ring[indesc]);

            if ((macb.config.hw_dma_cap & HW_DMA_CAP_64B) != 0)
            {
                if (!flag) { next_rx_tail = next_rx_tail / 2; }
            }
            next_rx_tail += 1;
            if (next_rx_tail >= MACB_RX_RING_SIZE)
            {
                next_rx_tail = 0;
            }
            macb.next_rx_tail = next_rx_tail;

            return length;
        }
        else
        {
            if ((macb.config.hw_dma_cap & HW_DMA_CAP_64B) != 0)
            {
                if (!flag)
                {
                    next_rx_tail = next_rx_tail / 2;
                }
                flag = false;
            }

            next_rx_tail += 1;
            if (next_rx_tail >= MACB_RX_RING_SIZE)
            {
                macb.wrapped = true;
                next_rx_tail = 0;
            }
        }
        fence();
        // barrier();
    } // loop
}

void reclaim_rx_buffers(u64 new_tail)
{
    u32 count = 0;
    u64 i = macb.rx_tail;
    printk("reclaim_rx_buffers, macb.rx_tail: {%d}, new_tail: {%d}\n",
           i, new_tail);

    invalidate_dcache_range(); // RX ring dma
    while (i > new_tail)
    {
        if ((macb.config.hw_dma_cap & HW_DMA_CAP_64B) != 0)
        {
            count = i * 2;
        }
        else { count = i; }
        macb.rx_ring[count].addr &= ~(1 << MACB_RX_USED_OFFSET);
        i += 1;
        if (i >= MACB_RX_RING_SIZE) { i = 0; }
    }
    while (i < new_tail)
    {
        if ((macb.config.hw_dma_cap & HW_DMA_CAP_64B) != 0)
        {
            count = i * 2;
        }
        else { count = i; }
        macb.rx_ring[count].addr &= ~(1 << MACB_RX_USED_OFFSET);
        i += 1;
    }
    fence();
    // barrier();
    flush_dcache_range(); // RX ring dma
    macb.rx_tail = new_tail;
}

void macb_invalidate_ring_desc()
{}

void invalidate_dcache_range()
{}

char *mystrncpy(char *s, const char *t, int n)
{
    char *os;

    os = s;
    while (n-- > 0 && (*s++ = *t++) != 0)
        ;
    while (n-- > 0)
        *s++ = 0;
    return os;
}

i32 macb_phy_init(char *name)
{
    // Auto-detect phy_addr
    i32 ret = macb_phy_find(macb);
    if (ret != 0) { return ret; }

    // Check if the PHY is up to snuff...
    u16 phy_id = macb_mdio_read(macb.phy_addr, MII_PHYSID1);
    if (phy_id == 0xffff)
    {
        printk("No PHY present\n");
        return -10; // ENODEV
    }
    printk("macb_phy_init phy_id: {%d}\n", phy_id);

    // Find macb->phydev
    phy_connect_dev(&macb);
    phy_config();

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
        printk("{%d} link down (status: {%d})\n", name, status);
        return -100; // ENETDOWN
    }

    u32 ncfgr = 0;
    u16 lpa = 0;
    u16 adv = 0;

    // First check for GMAC and that it is GiB capable
    if (gem_is_gigabit_capable())
    {
        lpa = macb_mdio_read(macb.phy_addr, MII_STAT1000);

        if ((lpa & (u16)(LPA_1000FULL | LPA_1000HALF | LPA_1000XFULL | LPA_1000XHALF)) != 0)
        {
            u32 duplex = ((lpa & (u16)(LPA_1000FULL | LPA_1000XFULL)) == 0) ? 0 : 1;
            char duplex_str[5];
            if (duplex == 1) { mystrncpy(duplex_str, "full", 5); }
            else { mystrncpy(duplex_str, "half", 4); }
            printk("{%s} GiB capable, link up, 1000Mbps {%S}-duplex (lpa: {%d)\n",
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
    printk("{%s} link up, {%s}Mbps {%s}-duplex (lpa: {%d})\n",
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

    return 0;
}

i32 macb_phy_find()
{
    // let mut phy_addr: u16 = 0;

    u16 phy_id = macb_mdio_read(macb.phy_addr, MII_PHYSID1);
    if (phy_id != 0xffff)
    {
        printk("PHY present at {%d}\n", macb.phy_addr);
        return 0;
    }
    // Search for PHY...
    for (int i = 0; i < 32; ++i)
    {
        macb.phy_addr = i;
        phy_id = macb_mdio_read(macb.phy_addr, MII_PHYSID1);
        if (phy_id != 0xffff)
        {
            printk("Found PHY present at {%d}\n", i);
            return 0;
        }
    }

    // PHY isn't up to snuff
    printk("PHY not found\n");
    return -19; // ENODEV
}

void macb_phy_reset(char *name)
{
    u32 status = 0;
    u32 adv = ADVERTISE_CSMA | ADVERTISE_ALL;
    macb_mdio_write(macb.phy_addr, MII_ADVERTISE, adv);
    printk("{%s} Starting autonegotiation...\n", name);
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
        printk("{%s} Autonegotiation complete\n", name);
    }
    else
    {
        printk("{%s} Autonegotiation timed out (status={%d})\n", name, status);
    }
}

void phy_connect_dev()
{
    u32 phydev_addr = macb.phy_addr;
    PhyInterfaceMode phydev_interface = macb.phy_interface;
    u32 phydev_flags = 0;

    i32 phydev = 0xff;
    // u32 mask = (phydev_addr >= 0) ? (1 << phydev_addr) : 0xffffffff;
    /*
    // Find phydev by maskaddr and interface
    if phydev == 0 {
        phydev = phy_find_by_mask(bus, mask, interface);
    }
    */
    /*
    phydev->flags: 0,
    phydev->addr: 0,
    phydev->interface: 1,
    */

    if (phydev != 0)
    {
        /* Soft Reset the PHY */
        phy_reset(phydev_addr, phydev_interface, phydev_flags);
        printk("Ethernet connected to PHY\n");

        // phy_config needs phydev
        vsc8541_config(phydev_addr, phydev_interface);
    }
    else
    {
        printk("Could not get PHY for ethernet: addr {%d}\n", macb.phy_addr);
    }
}

i32 phy_reset(u32 phydev_addr, PhyInterfaceMode _interface, u32 phydev_flags)
{
    i32 timeout = 500;
    // i32 devad = MDIO_DEVAD_NONE;

    printk("PHY soft reset");
    if ((phydev_flags & PHY_FLAG_BROKEN_RESET) != 0)
    {
        printk("PHY soft reset not supported\n");
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
        /*
        if reg < 0 {
            error!("PHY status read failed");
            return -1;
        }
        */
        usdelay(1000);
    }
    if ((reg & BMCR_RESET) != 0)
    {
        printk("PHY reset timed out\n");
        return -1;
    }
    return 0;
}

void phy_config()
{
    // Microsemi VSC8541 PHY driver config fn: vsc8541_config()
    // phy_config needs phydev struct after found by phy_connect
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

u16 macb_mdio_read(u32 phy_adr, u32 reg)
{
    u32 netctl = readv((u32 *)(MACB_IOBASE + MACB_NCR));
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

    // info!("mdio read phy_adr: {:#x}, reg: {:#x}, frame: {:#x}", phy_adr, reg, frame);

    netctl = readv((u32 *)(MACB_IOBASE + MACB_NCR));
    netctl &= ~(1 << MACB_MPE_OFFSET);
    writev((u32 *)(MACB_IOBASE + MACB_NCR), netctl);

    return ((frame >> MACB_DATA_OFFSET) & ((1 << MACB_DATA_SIZE) - 1));
}

void macb_sifive_clk_init(u64 rate)
{
    /*
     * SiFive GEMGXL TX clock operation mode:
     *
     * 0 = GMII mode. Use 125 MHz gemgxlclk from PRCI in TX logic
     *     and output clock on GMII output signal GTX_CLK
     *
     * 1 = MII mode. Use MII input signal TX_CLK in TX logic
     */
    u32 mode = (rate == 125000000) ? 0 : 1;
    writev((u32 *)GEMGXL_BASE, mode);
}

i32 macb_linkspd_cb(u32 speed)
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

u32 mii_nway_result(u32 negotiated)
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

u32 GEM_BF(u32 gem_offset, u32 gem_size, u32 value)
{
    return (value & ((1 << gem_size) - 1)) << gem_offset;
}
u32 GEM_BFINS(u32 gem_offset, u32 gem_size, u32 value, u32 old)
{
    return (old & ~(((1 << gem_size) - 1) << gem_offset)) | GEM_BF(gem_offset, gem_size, value);
}

u32 GEM_BIT(u32 offset)
{
    return 1 << offset;
}

i32 gmac_configure_dma()
{
    u32 buffer_size = (macb.buffer_size / RX_BUFFER_MULTIPLE);
    i64 neg = -1;
    u32 dmacfg = readv((u32 *)(MACB_IOBASE + GEM_DMACFG));
    printk("gmac_configure_dma read GEM_DMACFG: {%d}\n", dmacfg);
    dmacfg &= ~GEM_BF(GEM_RXBS_OFFSET, GEM_RXBS_SIZE, neg);
    printk("gmac_configure_dma dmacfg: {%d}\n", dmacfg);

    dmacfg |= GEM_BF(GEM_RXBS_OFFSET, GEM_RXBS_SIZE, buffer_size);

    if (macb.config.dma_burst_length != 0)
    {
        dmacfg = GEM_BFINS(
            GEM_FBLDO_OFFSET,
            GEM_FBLDO_SIZE,
            macb.config.dma_burst_length,
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
    if ((macb.config.hw_dma_cap & HW_DMA_CAP_64B) != 0)
    {
        dmacfg |= GEM_BIT(GEM_ADDR64_OFFSET);
    }

    printk("gmac_configure_dma write GEM_DMACFG @ {%d}, dmacfg = {%d}\n",
           MACB_IOBASE + GEM_DMACFG,
           dmacfg);
    writev((u32 *)(MACB_IOBASE + GEM_DMACFG), dmacfg);
    return 0;
}

u32 GEM_TBQP(u32 hw_q)
{
    return 0x0440 + ((hw_q) << 2);
}
u32 GEM_RBQP(u32 hw_q)
{
    return 0x0480 + ((hw_q) << 2);
}
u32 GEM_TBQPH(u32 hw_q)
{
    return 0x04C8;
}
u32 GEM_RBQPH(u32 hw_q)
{
    return 0x04D4;
}

void gmac_init_multi_queues()
{
    i32 num_queues = 1;
    // bit 0 is never set but queue 0 always exists
    u32 queue_mask = 0xff & readv((u32 *)(MACB_IOBASE + GEM_DCFG6));
    printk("gmac_init_multi_queues read GEM_DCFG6: {%d}\n", queue_mask);
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
    flush_dcache_range(); // dummy_desc_dma, len: MACB_TX_DUMMY_DMA_DESC_SIZE

    u64 paddr = macb.dummy_desc_dma;

    for (int i = 1; i < num_queues; ++i)
    {
        printk("gmac_init_multi_queues {%d} TBQP: {%d}, RBQP: {%d}",
               i,
               GEM_TBQP(i - 1),
               GEM_RBQP(i - 1));
        writev((u32 *)(MACB_IOBASE + (u64)GEM_TBQP(i - 1)),
               lower_32_bits(paddr));
        writev((u32 *)(MACB_IOBASE + (u64)GEM_RBQP(i - 1)),
               lower_32_bits(paddr));
        if ((macb.config.hw_dma_cap & HW_DMA_CAP_64B) != 0)
        {
            writev((u32 *)(MACB_IOBASE + (u64)GEM_TBQPH(i - 1)),
                   upper_32_bits(paddr));
            writev((u32 *)(MACB_IOBASE + (u64)GEM_RBQPH(i - 1)),
                   upper_32_bits(paddr));
        }
    }
}

void flush_dcache_range()
{}

bool is_big_endian()
{
    return false;
}

u32 upper_32_bits(u64 n)
{
    return ((n >> 16) >> 16);
}

u32 lower_32_bits(u64 n)
{
    return (n & 0xffffffff);
}