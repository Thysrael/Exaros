#include <types.h>

int vsc8541_config(struct phy_device *phydev)
{
    int retval = -EINVAL;
    u16 reg_val;
    u16 rmii_clk_out;
    enum vsc_phy_clk_slew edge_rate = VSC_PHY_CLK_SLEW_RATE_4;

    /* For VSC8530/31 and VSC8540/41 the init scripts are the same */
    mscc_vsc8531_vsc8541_init_scripts(phydev);

    /* For VSC8540/41 the only MAC modes are (G)MII and RMII/RGMII. */
    switch (phydev->interface)
    {
    case PHY_INTERFACE_MODE_MII:
    case PHY_INTERFACE_MODE_GMII:
    case PHY_INTERFACE_MODE_RMII:
    case PHY_INTERFACE_MODE_RGMII:
        retval = vsc8531_vsc8541_mac_config(phydev);
        if (retval != 0)
            return retval;

        retval = mscc_phy_soft_reset(phydev);
        if (retval != 0)
            return retval;
        break;
    default:
        printf("PHY 8541 MAC i/f config Error: mac i/f = 0x%x\n",
               phydev->interface);
        return -EINVAL;
    }
    /* Default RMII Clk Output to 0=OFF/1=ON  */
    rmii_clk_out = 0;

    retval = vsc8531_vsc8541_clk_skew_config(phydev);
    if (retval != 0)
        return retval;

    phy_write(phydev, MDIO_DEVAD_NONE, MSCC_EXT_PAGE_ACCESS,
              MSCC_PHY_PAGE_EXT2);
    reg_val = phy_read(phydev, MDIO_DEVAD_NONE, MSCC_PHY_WOL_MAC_CONTROL);
    /* Reg27E2 - Update Clk Slew Rate. */
    reg_val = bitfield_replace(reg_val, EDGE_RATE_CNTL_POS,
                               EDGE_RATE_CNTL_WIDTH, edge_rate);
    /* Reg27E2 - Update RMII Clk Out. */
    reg_val = bitfield_replace(reg_val, RMII_CLK_OUT_ENABLE_POS,
                               RMII_CLK_OUT_ENABLE_WIDTH, rmii_clk_out);
    /* Update Reg27E2 */
    phy_write(phydev, MDIO_DEVAD_NONE, MSCC_PHY_WOL_MAC_CONTROL, reg_val);
    phy_write(phydev, MDIO_DEVAD_NONE, MSCC_EXT_PAGE_ACCESS,
              MSCC_PHY_PAGE_STD);

    /* Configure the clk output */
    retval = vsc8531_vsc8541_clkout_config(phydev);
    if (retval != 0)
        return retval;

    return genphy_config_aneg(phydev);
}