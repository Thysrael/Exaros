#include "phy_mscc.h"
#include "mii_const.h"
#include "eth_macb_ops.h"
#include "eth_macb.h"

i32 vsc8541_config(u32 phydev_addr, PhyInterfaceMode interface)
{
    i32 retval = -22;
    u32 rmii_clk_out = 0;
    u32 edge_rate = 4;
    mscc_vsc8531_vsc8541_init_scripts(phydev_addr);
    switch (interface)
    {
    case PhyInterfaceMode_MII:
    case PhyInterfaceMode_GMII:
    case PhyInterfaceMode_RMII:
    case PhyInterfaceMode_RGMII: {
        retval = vsc8531_vsc8541_mac_config(phydev_addr, interface);
        if (retval != 0) { return retval; }
        retval = mscc_phy_soft_reset(phydev_addr);
        if (retval != 0) { return retval; }
        break;
    }
    default: {
        printk("PHY 8541 MAC i/f config Error: mac i/f = {%u}\n", interface);
        return -22;
    }
    }
    /* Default RMII Clk Output to 0=OFF/1=ON  */
    retval = vsc8531_vsc8541_clk_skew_config(phydev_addr, interface);
    if (retval != 0) { return retval; }

    macb_mdio_write(phydev_addr, MSCC_EXT_PAGE_ACCESS, MSCC_PHY_PAGE_EXT2);
    /* Reg27E2 - Update Clk Slew Rate. */
    u32 reg_val = bitfield_replace(
        reg_val,
        EDGE_RATE_CNTL_POS,
        EDGE_RATE_CNTL_WIDTH,
        edge_rate);
    /* Reg27E2 - Update RMII Clk Out. */
    reg_val = bitfield_replace(
        reg_val,
        RMII_CLK_OUT_ENABLE_POS,
        RMII_CLK_OUT_ENABLE_WIDTH,
        rmii_clk_out);

    /* Update Reg27E2 */
    macb_mdio_write(phydev_addr, MSCC_PHY_WOL_MAC_CONTROL, reg_val);
    macb_mdio_write(phydev_addr, MSCC_EXT_PAGE_ACCESS, MSCC_PHY_PAGE_STD);

    /* Configure the clk output */
    retval = vsc8531_vsc8541_clkout_config(phydev_addr);
    if (retval != 0) { return retval; }

    return genphy_config_aneg(phydev_addr);
}

i32 mscc_phy_soft_reset(u32 phydev_addr)
{
    u32 timeout = MSCC_PHY_RESET_TIMEOUT;
    u32 reg_val = 0;
    printk("mscc_phy_soft_reset\n");
    macb_mdio_write(phydev_addr, MSCC_EXT_PAGE_ACCESS, MSCC_PHY_PAGE_STD);
    reg_val = macb_mdio_read(phydev_addr, MII_BMCR);
    macb_mdio_write(phydev_addr, MII_BMCR, reg_val | (BMCR_RESET));

    reg_val = macb_mdio_read(phydev_addr, MII_BMCR);
    while (((reg_val & BMCR_RESET) != 0) && (timeout > 0))
    {
        reg_val = macb_mdio_read(phydev_addr, MII_BMCR);
        timeout -= 1;
        usdelay(1000); /* 1 毫秒 */
    }
    if (timeout == 0)
    {
        // error!("MSCC PHY Soft_Reset Error: mac i/f = {:#x}", interface);
        printk("MSCC PHY Soft_Reset Error\n");
        return -1;
    }
    return 0;
}

void mscc_vsc8531_vsc8541_init_scripts(u32 phydev_addr)
{
    printk("mscc_vsc8531_vsc8541_init_scripts\n");

    // Set to Access Token Ring Registers
    macb_mdio_write(phydev_addr, MSCC_EXT_PAGE_ACCESS, MSCC_PHY_PAGE_TR);

    // Update LinkDetectCtrl default to optimized values
    // Determined during Silicon Validation Testing
    macb_mdio_write(
        phydev_addr,
        MSCC_PHY_REG_TR_ADDR_16,
        (MSCC_PHY_TR_LINKDETCTRL_ADDR | MSCC_PHY_TR_16_READ));
    u32 reg_val = macb_mdio_read(phydev_addr, MSCC_PHY_REG_TR_DATA_17);
    reg_val = bitfield_replace(
        reg_val,
        MSCC_PHY_TR_LINKDETCTRL_POS,
        MSCC_PHY_TR_LINKDETCTRL_WIDTH,
        MSCC_PHY_TR_LINKDETCTRL_VAL);

    macb_mdio_write(phydev_addr, MSCC_PHY_REG_TR_DATA_17, reg_val);
    macb_mdio_write(
        phydev_addr,
        MSCC_PHY_REG_TR_ADDR_16,
        (MSCC_PHY_TR_LINKDETCTRL_ADDR | MSCC_PHY_TR_16_WRITE));

    // Update VgaThresh100 defaults to optimized values
    // Determined during Silicon Validation Testing
    macb_mdio_write(
        phydev_addr,
        MSCC_PHY_REG_TR_ADDR_16,
        (MSCC_PHY_TR_VGATHRESH100_ADDR | MSCC_PHY_TR_16_READ));
    reg_val = macb_mdio_read(phydev_addr, MSCC_PHY_REG_TR_DATA_18);
    reg_val = bitfield_replace(
        reg_val,
        MSCC_PHY_TR_VGATHRESH100_POS,
        MSCC_PHY_TR_VGATHRESH100_WIDTH,
        MSCC_PHY_TR_VGATHRESH100_VAL);
    macb_mdio_write(phydev_addr, MSCC_PHY_REG_TR_DATA_18, reg_val);
    macb_mdio_write(
        phydev_addr,
        MSCC_PHY_REG_TR_ADDR_16,
        (MSCC_PHY_TR_VGATHRESH100_ADDR | MSCC_PHY_TR_16_WRITE));

    // Update VgaGain10 defaults to optimized values
    // Determined during Silicon Validation Testing
    macb_mdio_write(
        phydev_addr,
        MSCC_PHY_REG_TR_ADDR_16,
        (MSCC_PHY_TR_VGAGAIN10_ADDR | MSCC_PHY_TR_16_READ));
    reg_val = macb_mdio_read(phydev_addr, MSCC_PHY_REG_TR_DATA_18);
    reg_val = bitfield_replace(
        reg_val,
        MSCC_PHY_TR_VGAGAIN10_U_POS,
        MSCC_PHY_TR_VGAGAIN10_U_WIDTH,
        MSCC_PHY_TR_VGAGAIN10_U_VAL);

    macb_mdio_write(phydev_addr, MSCC_PHY_REG_TR_DATA_18, reg_val);

    reg_val = macb_mdio_read(phydev_addr, MSCC_PHY_REG_TR_DATA_17);
    reg_val = bitfield_replace(
        reg_val,
        MSCC_PHY_TR_VGAGAIN10_L_POS,
        MSCC_PHY_TR_VGAGAIN10_L_WIDTH,
        MSCC_PHY_TR_VGAGAIN10_L_VAL);
    macb_mdio_write(phydev_addr, MSCC_PHY_REG_TR_DATA_17, reg_val);
    macb_mdio_write(
        phydev_addr,
        MSCC_PHY_REG_TR_ADDR_16,
        (MSCC_PHY_TR_VGAGAIN10_ADDR | MSCC_PHY_TR_16_WRITE));

    // Set back to Access Standard Page Registers
    macb_mdio_write(phydev_addr, MSCC_EXT_PAGE_ACCESS, MSCC_PHY_PAGE_STD);
}

i32 vsc8531_vsc8541_mac_config(u32 phydev_addr, PhyInterfaceMode interface)
{
    u32 reg_val = 0;
    u32 mac_if = 0;
    u32 rx_clk_out = 0;

    /* For VSC8530/31 the only MAC modes are RMII/RGMII. */
    /* For VSC8540/41 the only MAC modes are (G)MII and RMII/RGMII. */
    /* Setup MAC Configuration */
    switch (interface)
    {
    case PhyInterfaceMode_MII:
    case PhyInterfaceMode_GMII: {
        // Set Reg23.12:11=0
        mac_if = MAC_IF_SELECTION_GMII;
        // Set Reg20E2.11=1
        rx_clk_out = RX_CLK_OUT_DISABLE;
        break;
    }
    case PhyInterfaceMode_RMII: {
        // Set Reg23.12:11=1
        mac_if = MAC_IF_SELECTION_RMII;
        // Set Reg20E2.11=0
        rx_clk_out = RX_CLK_OUT_NORMAL;
        break;
    }
    case PhyInterfaceMode_RGMII:
    case PhyInterfaceMode_RGMII_ID:
    case PhyInterfaceMode_RGMII_TXID:
    case PhyInterfaceMode_RGMII_RXID: {
        // Set Reg23 .12 : 11 = 2
        mac_if = MAC_IF_SELECTION_RGMII;
        // Set Reg20E2.11=0
        rx_clk_out = RX_CLK_OUT_NORMAL;
        break;
    }
    default: {
        printk("MSCC PHY - INVALID MAC i/f Config: mac i/f = {%u\n}", interface);
        return -1;
    }
    }

    macb_mdio_write(phydev_addr, MSCC_EXT_PAGE_ACCESS, MSCC_PHY_PAGE_STD);
    reg_val = macb_mdio_read(phydev_addr, MSCC_PHY_EXT_PHY_CNTL_1_REG);
    // Set MAC i/f bits Reg23.12:11
    reg_val = bitfield_replace(
        reg_val,
        MAC_IF_SELECTION_POS,
        MAC_IF_SELECTION_WIDTH,
        mac_if);

    printk("vsc8531_vsc8541_mac_config reg_val: {%u}\n", reg_val);

    // Update Reg23.12:11
    macb_mdio_write(phydev_addr, MSCC_PHY_EXT_PHY_CNTL_1_REG, reg_val);

    // Setup ExtPg_2 Register Access
    macb_mdio_write(phydev_addr, MSCC_EXT_PAGE_ACCESS, MSCC_PHY_PAGE_EXT2);

    // Read Reg20E2
    reg_val = macb_mdio_read(phydev_addr, MSCC_PHY_RGMII_CNTL_REG);
    reg_val = bitfield_replace(reg_val, RX_CLK_OUT_POS, RX_CLK_OUT_WIDTH, rx_clk_out);

    printk("vsc8531_vsc8541_mac_config reg_val: {%u}\n", reg_val);

    // Update Reg20E2.11
    macb_mdio_write(phydev_addr, MSCC_PHY_RGMII_CNTL_REG, reg_val);

    // Before leaving - Change back to Std Page Register Access
    macb_mdio_write(phydev_addr, MSCC_EXT_PAGE_ACCESS, MSCC_PHY_PAGE_STD);

    return 0;
}

i32 vsc8531_vsc8541_clk_skew_config(u32 phydev_addr, PhyInterfaceMode interface)
{
    /*
    // VSC_PHY_RGMII_DELAY_200_PS: 0, VSC_PHY_RGMII_DELAY_2000_PS: 4
    let mut rx_clk_skew = VSC_PHY_RGMII_DELAY_200_PS;
    let mut tx_clk_skew = VSC_PHY_RGMII_DELAY_200_PS;
    */
    u32 rx_clk_skew = 0;
    u32 tx_clk_skew = 0;

    if ((interface == PhyInterfaceMode_RGMII_RXID) || (interface == PhyInterfaceMode_RGMII_ID))
    {
        // rx_clk_skew = VSC_PHY_RGMII_DELAY_2000_PS;
        rx_clk_skew = 4;
    }
    if ((interface == PhyInterfaceMode_RGMII_TXID) || (interface == PhyInterfaceMode_RGMII_ID))
    {
        // tx_clk_skew = VSC_PHY_RGMII_DELAY_2000_PS;
        tx_clk_skew = 4;
    }
    macb_mdio_write(phydev_addr, MSCC_EXT_PAGE_ACCESS, MSCC_PHY_PAGE_EXT2);
    u32 reg_val = macb_mdio_read(phydev_addr, MSCC_PHY_RGMII_CNTL_REG);

    // Reg20E2 - Update RGMII RX_Clk Skews.
    reg_val = bitfield_replace(
        reg_val,
        RGMII_RX_CLK_DELAY_POS,
        RGMII_RX_CLK_DELAY_WIDTH,
        rx_clk_skew);
    // Reg20E2 - Update RGMII TX_Clk Skews.
    reg_val = bitfield_replace(
        reg_val,
        RGMII_TX_CLK_DELAY_POS,
        RGMII_TX_CLK_DELAY_WIDTH,
        tx_clk_skew);

    printk("vsc8531_vsc8541_clk_skew_config reg_val: {:%u}\n", reg_val);
    macb_mdio_write(phydev_addr, MSCC_PHY_RGMII_CNTL_REG, reg_val);
    macb_mdio_write(phydev_addr, MSCC_EXT_PAGE_ACCESS, MSCC_PHY_PAGE_STD);
    return 0;
}

i32 vsc8531_vsc8541_clkout_config(u32 phydev_addr)
{
    u32 clkout_rate = 0; // clkout_rate: 0
    u32 reg_val = 0;

    switch (clkout_rate)
    {
    case 0: {
        reg_val = 0;
        break;
    }
    case 25000000: {
        reg_val = CLKOUT_FREQ_25M | CLKOUT_ENABLE;
        break;
    }
    case 50000000: {
        reg_val = CLKOUT_FREQ_50M | CLKOUT_ENABLE;
        break;
    }
    case 125000000: {
        reg_val = CLKOUT_FREQ_125M | CLKOUT_ENABLE;
        break;
    }
    default: {
        printk("PHY 8530/31 invalid clkout rate {%u}\n", clkout_rate);
        return -1;
    }
    }

    printk("vsc8531_vsc8541_clkout_config reg_val: {%u}\n", reg_val);

    macb_mdio_write(phydev_addr, MSCC_EXT_PAGE_ACCESS, MSCC_PHY_PAGE_GPIO);
    macb_mdio_write(phydev_addr, MSCC_CLKOUT_CNTL, reg_val);
    macb_mdio_write(phydev_addr, MSCC_EXT_PAGE_ACCESS, MSCC_PHY_PAGE_STD);
    return 0;
}

// Replace the value of a bitfield found within a given register value
// Returns the newly modified uint value with the replaced field.
u32 bitfield_replace(u32 reg_val, u32 shift, u32 width, u32 bitfield_val)
{
    // Produces a mask of set bits covering a range of a uint value
    u32 mask = ((1 << width) - 1) << shift;
    return (reg_val & (!mask)) | ((bitfield_val << shift) & mask);
}

/**
 * genphy_config_aneg - restart auto-negotiation or write BMCR
 * @phydev: target phy_device struct
 *
 * Description: If auto-negotiation is enabled, we configure the
 *   advertising, and then restart auto-negotiation.  If it is not
 *   enabled, then we write the BMCR.
 */
i32 genphy_config_aneg(u32 phydev_addr)
{
    u32 phydev_autoneg = AUTONEG_ENABLE;
    u32 phydev_speed = 0;
    u32 phydev_duplex = -1;
    // u32 phydev_link = 0;

    u32 phydev_advertising = 0x2ff;
    u32 phydev_supported = 0x2ff;

    if (phydev_autoneg != AUTONEG_ENABLE)
    {
        return genphy_setup_forced(phydev_addr, phydev_speed, phydev_duplex);
    }
    u32 result = genphy_config_advert(phydev_addr, &phydev_advertising, phydev_supported);
    // error
    if (result < 0) { return result; }
    if (result == 0)
    {
        // Advertisment hasn't changed, but maybe aneg was never on to begin with?  Or maybe phy was isolated?
        u32 ctl = macb_mdio_read(phydev_addr, MII_BMCR);
        /*
        if ctl < 0 {
            return ctl as i32;
        }
        */
        if (((ctl & BMCR_ANENABLE) == 0) || ((ctl & BMCR_ISOLATE) != 0))
        {
            // do restart aneg
            result = 1;
        }
    }

    // Only restart aneg if we are advertising something different than we were before.
    if (result > 0)
    {
        result = genphy_restart_aneg(phydev_addr);
    }
    return result;
}

i32 genphy_setup_forced(u32 phydev_addr, i32 speed, i32 duplex)
{
    u32 ctl = BMCR_ANRESTART;

    if (speed == SPEED_1000) { ctl |= BMCR_SPEED1000; }
    else if (speed == SPEED_100) { ctl |= BMCR_SPEED100; }

    if (duplex == DUPLEX_FULL) { ctl |= BMCR_FULLDPLX; }

    printk("genphy_setup_forced Write MII_BMCR: {%u}\n", ctl);
    macb_mdio_write(phydev_addr, MII_BMCR, ctl);
    return 0;
}

/**
 * genphy_config_advert - sanitize and advertise auto-negotiation parameters
 * @phydev: target phy_device struct
 *
 * Description: Writes MII_ADVERTISE with the appropriate values,
 *   after sanitizing the values to make sure we only advertise
 *   what is supported.  Returns < 0 on error, 0 if the PHY's advertisement
 *   hasn't changed, and > 0 if it has changed.
 */
i32 genphy_config_advert(
    u32 phydev_addr,
    u32 *phydev_advertising,
    u32 phydev_supported)
{
    u32 changed = 0;
    /* Only allow advertising what this PHY supports */
    *phydev_advertising &= phydev_supported;
    u32 advertise = *phydev_advertising;

    /* Setup standard advertisement */
    u32 adv = macb_mdio_read(phydev_addr, MII_ADVERTISE);
    u32 oldadv = adv;
    /*
    if adv < 0 {
        return adv as i32;
    }
    */

    adv &= !(ADVERTISE_ALL | ADVERTISE_100BASE4 | ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM);
    if ((advertise & ADVERTISED_10baseT_Half) != 0)
    {
        adv |= ADVERTISE_10HALF;
    }
    if ((advertise & ADVERTISED_10baseT_Full) != 0)
    {
        adv |= ADVERTISE_10FULL;
    }
    if ((advertise & ADVERTISED_100baseT_Half) != 0)
    {
        adv |= ADVERTISE_100HALF;
    }
    if ((advertise & ADVERTISED_100baseT_Full) != 0)
    {
        adv |= ADVERTISE_100FULL;
    }
    if ((advertise & ADVERTISED_Pause) != 0)
    {
        adv |= ADVERTISE_PAUSE_CAP;
    }
    if ((advertise & ADVERTISED_Asym_Pause) != 0)
    {
        adv |= ADVERTISE_PAUSE_ASYM;
    }
    if ((advertise & ADVERTISED_1000baseX_Half) != 0)
    {
        adv |= ADVERTISE_1000XHALF;
    }
    if ((advertise & ADVERTISED_1000baseX_Full) != 0)
    {
        adv |= ADVERTISE_1000XFULL;
    }

    if (adv != oldadv)
    {
        macb_mdio_write(phydev_addr, MII_ADVERTISE, adv);
        changed = 1;
    }

    u32 bmsr = macb_mdio_read(phydev_addr, MII_BMSR);
    /*
    if bmsr < 0 {
        return bmsr as i32;
    }
    */
    /* Per 802.3-2008, Section 22.2.4.2.16 Extended status all
     * 1000Mbits/sec capable PHYs shall have the BMSR_ESTATEN bit set to a
     * logical 1.
     */
    if ((bmsr & BMSR_ESTATEN) == 0) { return changed; }

    /* Configure gigabit if it's supported */
    adv = macb_mdio_read(phydev_addr, MII_CTRL1000);
    oldadv = adv;
    /*
    if adv < 0 {
        return adv as i32;
    }
    */
    adv &= !(ADVERTISE_1000FULL | ADVERTISE_1000HALF);
    if ((phydev_supported & (SUPPORTED_1000baseT_Half | SUPPORTED_1000baseT_Full)) != 0)
    {
        if ((advertise & SUPPORTED_1000baseT_Half) != 0) { adv |= ADVERTISE_1000HALF; }
        if ((advertise & SUPPORTED_1000baseT_Full) != 0) { adv |= ADVERTISE_1000FULL; }
    }
    if (adv != oldadv) { changed = 1; }
    printk("genphy_config_advert Write MII_CTRL1000: {%u}\n", adv);
    macb_mdio_write(phydev_addr, MII_CTRL1000, adv);
    return changed;
}

/**
 * genphy_restart_aneg - Enable and Restart Autonegotiation
 * @phydev: target phy_device struct
 */
i32 genphy_restart_aneg(u32 phydev_addr)
{
    u32 ctl = macb_mdio_read(phydev_addr, MII_BMCR);
    ctl |= (BMCR_ANENABLE | BMCR_ANRESTART);

    /* Don't isolate the PHY if we're negotiating */
    ctl &= !(BMCR_ISOLATE);

    printk("genphy_restart_aneg MII_BMCR: {%u}\n", ctl);
    macb_mdio_write(phydev_addr, MII_BMCR, ctl);
    return 0;
}