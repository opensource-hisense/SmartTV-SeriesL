/*** CONFIDENTIAL ***/
/* Copyright (C) 2015, Socionext Inc. */
#include <linux/delay.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/page.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include "ave_sn.h"

static int gphy_type = AVE_PHY_MCRL;

extern int ave_phy_reset_period;
extern int ave_phy_reset_stability;

int ave_poll_link = 0;   

uint32_t ave_ksz8051_bugfix = 0;   

int ave_miimode=3;      
int ave_gbit_mode = 1;  

#ifdef AVE_ENABLE_RTL8201FR
int ave_rtl8201fr_wol_init_state = 0;
#endif

#ifdef AVE_USE_EEPROM 
static int ave_inner_mac_eep_get_mac(unsigned char *mac_addr);
static int ave_inner_mac_eep_set_mac(unsigned char *mac_addr);
static int ave_inner_mac_check_busy(unsigned int factor);
static int ave_inner_mac_wait_interrupt(unsigned int factor, unsigned int *status);
int ave_inner_mac_eepdrv_get_mac(unsigned char *mac_addr);
#endif 

static void ave_mdio_select_page(struct net_device *netdev, uint32_t page);
static void ave_mdio_select_extention_page(struct net_device *netdev, uint32_t ext_page);
static uint32_t ave_get_rtl8211_wol_setting(struct net_device *netdev);
static void ave_set_rtl8211_wol_setting(struct net_device *netdev, uint32_t ave_wolopts);
static uint32_t ave_get_rtl8201_wol_setting(struct net_device *netdev);
static void ave_set_rtl8201_wol_setting(struct net_device *netdev, uint32_t ave_wolopts);

int ave_util_mac_read(unsigned char *macadr_buf);
int ave_util_mac_read(unsigned char *macadr_buf)
{
    macadr_buf[0] = 0xff;
    macadr_buf[1] = 0xff;
    macadr_buf[2] = 0xff;
    macadr_buf[3] = 0xff;
    macadr_buf[4] = 0xff;
    macadr_buf[5] = 0xff;

#ifdef AVE_USE_EEPROM 

#ifdef MODULE
    ave_inner_mac_eepdrv_get_mac(macadr_buf);
#else 
    ave_inner_mac_eep_get_mac(macadr_buf);
#endif 

#endif 

    return(0);
}

#ifdef AVE_USE_EEPROM 
#define AVE_IIC_CH      (0)       
#define EEPROM_ADDR     (0x50)    

int ave_inner_mac_eepdrv_get_mac(unsigned char *mac_addr)
{
    struct i2c_adapter      *PI2cAdap;
    struct i2c_msg          I2cMsg[2];
    unsigned char           aBuf1[8];
    int                     status;

    PI2cAdap = i2c_get_adapter(AVE_IIC_CH);
    if (!PI2cAdap) {
        printk(KERN_WARNING "avether-i2c: i2c_get_adapter failed\n");
        return( -1 );
    }

    I2cMsg[0].addr = EEPROM_ADDR;    
    I2cMsg[0].flags = 0;
    I2cMsg[0].len = 2;
    I2cMsg[0].buf = aBuf1;
    aBuf1[0] = 0xff;          
    aBuf1[1] = 0xf9;
    I2cMsg[1].addr = EEPROM_ADDR;    
    I2cMsg[1].flags = I2C_M_RD;
    I2cMsg[1].len = 6;
    I2cMsg[1].buf = mac_addr;

    status = i2c_transfer(PI2cAdap, I2cMsg, 2);
    if (0 > status) {
        printk(KERN_WARNING "avether-i2c: i2c_transfer(1) failed (%d)\n",
                    status);
        i2c_put_adapter( PI2cAdap );
        return( -1 );
    }

    i2c_put_adapter( PI2cAdap );
    return( 0 );
}

#define EEPROM_ADDRESS  (0xA0)

#define MAC_OFFSET      (0xFFF9)

#define MAC_LENGTH      (6)

#define USE_IIC_CH      (0)       

#define IIC_OFFSET      (0x1000 * USE_IIC_CH)

#define IIC_FIFO_SIZE   (8)

#define IIC_BASE        (0x58780000)

#define IIC_IO_SIZE     (0x60)
#define MP_IRQSEL_BASE  (0x59820050)
#define MP_IRQ_IO_SIZE  (0x10)

unsigned long iic_reg_base;
unsigned long mp_irq_reg_base;

#define IIC_FI2CCR    __SYSREG(iic_reg_base + IIC_OFFSET)
#define IIC_FI2CDTTX  __SYSREG(iic_reg_base + 0x04 + IIC_OFFSET)
#define IIC_FI2CDTRX  __SYSREG(iic_reg_base + 0x04 + IIC_OFFSET)
#define IIC_FI2CSLAD  __SYSREG(iic_reg_base + 0x0c + IIC_OFFSET)
#define IIC_FI2CCYC   __SYSREG(iic_reg_base + 0x10 + IIC_OFFSET)
#define IIC_FI2CLCTL  __SYSREG(iic_reg_base + 0x14 + IIC_OFFSET)
#define IIC_FI2CSSUT  __SYSREG(iic_reg_base + 0x18 + IIC_OFFSET)
#define IIC_FI2CDSUT  __SYSREG(iic_reg_base + 0x1c + IIC_OFFSET)
#define IIC_FI2CINT   __SYSREG(iic_reg_base + 0x20 + IIC_OFFSET)
#define IIC_FI2CIE    __SYSREG(iic_reg_base + 0x24 + IIC_OFFSET)
#define IIC_FI2CIC    __SYSREG(iic_reg_base + 0x28 + IIC_OFFSET)
#define IIC_FI2CSR    __SYSREG(iic_reg_base + 0x2c + IIC_OFFSET)
#define IIC_FI2CRST   __SYSREG(iic_reg_base + 0x34 + IIC_OFFSET)
#define IIC_FI2CBM    __SYSREG(iic_reg_base + 0x38 + IIC_OFFSET)
#define IIC_FI2CBRST  __SYSREG(iic_reg_base + 0x50 + IIC_OFFSET)
#define MP_IRQSEL     __SYSREG(mp_irq_reg_base)

#define IIC_FI2CSR_MD_MASK   ((unsigned int)0x00006000)
#define IIC_FI2CSR_DB_MASK   ((unsigned int)0x00001000)
#define IIC_FI2CSR_STS_MASK  ((unsigned int)0x00000800)
#define IIC_FI2CSR_LRB_MASK  ((unsigned int)0x00000400)
#define IIC_FI2CSR_ASS_MASK  ((unsigned int)0x00000200)
#define IIC_FI2CSR_BB_MASK   ((unsigned int)0x00000100)
#define IIC_FI2CSR_GCA_MASK  ((unsigned int)0x00000010)
#define IIC_FI2CSR_RFF_MASK  ((unsigned int)0x00000008)
#define IIC_FI2CSR_RNE_MASK  ((unsigned int)0x00000004)
#define IIC_FI2CSR_TNF_MASK  ((unsigned int)0x00000002)
#define IIC_FI2CSR_TFE_MASK  ((unsigned int)0x00000001)

#define IIC_FI2CI_TE_MASK  ((unsigned int)0x00000200)
#define IIC_FI2CI_RB_MASK  ((unsigned int)0x00000010)
#define IIC_FI2CI_NA_MASK  ((unsigned int)0x00000004)
#define IIC_FI2CI_AL_MASK  ((unsigned int)0x00000002)

#define IIC_FI2CCR_MST_MASK  ((unsigned int)0x00000008)
#define IIC_FI2CCR_STA_MASK  ((unsigned int)0x00000004)
#define IIC_FI2CCR_STO_MASK  ((unsigned int)0x00000002)
#define IIC_FI2CCR_ACK_MASK  ((unsigned int)0x00000001)

#define IIC_FI2CDTTX_COM_MASK   ((unsigned int)0x00000100)
#define IIC_FI2CDTTX_DATA_MASK  ((unsigned int)0x000000FF)

#define IIC_CLK_INTERNAL    ((unsigned int)50000000)
#define IIC_CLK_CYCLE       ((unsigned int)(IIC_CLK_INTERNAL / 100000))
#define IIC_CLK_LOW_PERIOD  ((unsigned int)(4700 / (10000/IIC_CLK_CYCLE)))

#define IIC_IRQ            (73 + USE_IIC_CH)
#define IIC_GIC_PEND_CLR   __SYSREG(0x5FC20380 + (IIC_IRQ / 32) * 4)
#define IIC_GIC_PEND_MASK  (1 << (IIC_IRQ % 32))

#define IIC_DELAY    1000  
#define IIC_TIMEOUT  1000000  

#define inl_iic(d, a)   d = readl( (void __iomem *)(a) )
#define outl_iic(d, a)  writel( (d), (void __iomem *)(a) )

#define AVE_USE_INT
#ifdef AVE_USE_INT
#define SET_INT(factor) {\
    outl_iic((factor | IIC_FI2CI_NA_MASK | IIC_FI2CI_AL_MASK), IIC_FI2CIE); \
    outl_iic((factor | IIC_FI2CI_NA_MASK | IIC_FI2CI_AL_MASK), IIC_FI2CIC); \
}
#else 
#define SET_INT(factor)
#endif 

static int ave_inner_mac_eep_get_mac(unsigned char *mac_addr)
{
    unsigned char *mac_buf = mac_addr;
    unsigned int data;
    unsigned int status = 0;
    int mac_length = 6;

    iic_reg_base = (unsigned long)ioremap(IIC_BASE, (IIC_IO_SIZE + IIC_OFFSET));
    if( (void *)iic_reg_base == NULL ){
        printk("Error!! %s(%d)\n", __FUNCTION__, __LINE__);
        return (-1);
    }
    mp_irq_reg_base  = (unsigned long)ioremap(MP_IRQSEL_BASE, MP_IRQ_IO_SIZE);
    if( (void *)mp_irq_reg_base == NULL ){
        printk("Error!! %s(%d)\n", __FUNCTION__, __LINE__);
        iounmap((void *)iic_reg_base);
        return (-1);
    }

    outl_iic(0xB600007F, MP_IRQSEL);

    outl_iic(0x00000003, IIC_FI2CBRST);

    outl_iic(IIC_CLK_CYCLE, IIC_FI2CCYC);
    outl_iic(IIC_CLK_LOW_PERIOD, IIC_FI2CLCTL);

    if(ave_inner_mac_check_busy(IIC_FI2CSR_DB_MASK)){
        printk("Error!! %s(%d)\n", __FUNCTION__, __LINE__);
        iounmap((void *)iic_reg_base);
        iounmap((void *)mp_irq_reg_base);
        return (-1);
    }
    data = (EEPROM_ADDRESS & IIC_FI2CDTTX_DATA_MASK) | IIC_FI2CDTTX_COM_MASK;
    outl_iic(data, IIC_FI2CDTTX);

    data = (MAC_OFFSET & 0xFF00) >> 8;
    outl_iic(data, IIC_FI2CDTTX);

    data = (MAC_OFFSET & 0x00FF);
    outl_iic(data, IIC_FI2CDTTX);

    SET_INT(IIC_FI2CI_TE_MASK);

    if(ave_inner_mac_check_busy(IIC_FI2CSR_BB_MASK)){
        printk("Error!! %s(%d)\n", __FUNCTION__, __LINE__);
        iounmap((void *)iic_reg_base);
        iounmap((void *)mp_irq_reg_base);
        return (-1);
    }
    data = IIC_FI2CCR_MST_MASK | IIC_FI2CCR_STA_MASK;
    outl_iic(data, IIC_FI2CCR);

    if(ave_inner_mac_wait_interrupt(IIC_FI2CI_TE_MASK, &status)){
        printk("Error!! %s(%d)\n", __FUNCTION__, __LINE__);
        iounmap((void *)iic_reg_base);
        iounmap((void *)mp_irq_reg_base);
        return (-1);
    }

    data = (EEPROM_ADDRESS & IIC_FI2CDTTX_DATA_MASK) | IIC_FI2CDTTX_COM_MASK | 0x00000001;
    outl_iic(data, IIC_FI2CDTTX);

    SET_INT(IIC_FI2CI_RB_MASK);

    data = IIC_FI2CCR_MST_MASK | IIC_FI2CCR_STA_MASK;
    outl_iic(data, IIC_FI2CCR);

    while (mac_length > 0) {
        if(ave_inner_mac_wait_interrupt(IIC_FI2CI_RB_MASK, &status)){
            printk("Error!! %s(%d)\n", __FUNCTION__, __LINE__);
            iounmap((void *)iic_reg_base);
            iounmap((void *)mp_irq_reg_base);
            return (-1);
        }
        inl_iic(data, IIC_FI2CDTRX);
        *mac_buf++ = (unsigned char)data;
        mac_length--;

        if (mac_length == 0) {
            data = IIC_FI2CCR_MST_MASK | IIC_FI2CCR_STO_MASK;
            outl_iic(data, IIC_FI2CCR);
        }
        else if (mac_length == 1) {
            data = IIC_FI2CCR_MST_MASK | IIC_FI2CCR_ACK_MASK;
            outl_iic(data, IIC_FI2CCR);
        }
        else {
        }
        outl_iic(status, IIC_FI2CIC);
    }

    iounmap((void *)iic_reg_base);
    iounmap((void *)mp_irq_reg_base);

    return 0;
}

static int ave_inner_mac_eep_set_mac(unsigned char *mac_addr)
{
    unsigned char *mac_buf = mac_addr;
    unsigned int data;
    unsigned int status = 0;
    int mac_length = MAC_LENGTH;
    int len = IIC_FIFO_SIZE;

    iic_reg_base = (unsigned long)ioremap(IIC_BASE, (IIC_IO_SIZE + IIC_OFFSET));
    if( (void *)iic_reg_base == NULL ){
        printk("Error!! %s(%d)\n", __FUNCTION__, __LINE__);
        return (-1);
    }
    mp_irq_reg_base  = (unsigned long)ioremap(MP_IRQSEL_BASE, MP_IRQ_IO_SIZE);
    if( (void *)mp_irq_reg_base == NULL ){
        printk("Error!! %s(%d)\n", __FUNCTION__, __LINE__);
        iounmap((void *)iic_reg_base);
        return (-1);
    }

    outl_iic(0xB600007F, MP_IRQSEL);

    outl_iic(0x00000003, IIC_FI2CBRST);

    outl_iic(IIC_CLK_CYCLE, IIC_FI2CCYC);
    outl_iic(IIC_CLK_LOW_PERIOD, IIC_FI2CLCTL);

    if(ave_inner_mac_check_busy(IIC_FI2CSR_DB_MASK)){
        printk("Error!! %s(%d)\n", __FUNCTION__, __LINE__);
        iounmap((void *)iic_reg_base);
        iounmap((void *)mp_irq_reg_base);
        return (-1);
    }
    data = (EEPROM_ADDRESS & IIC_FI2CDTTX_DATA_MASK) | IIC_FI2CDTTX_COM_MASK;
    outl_iic(data, IIC_FI2CDTTX);
    len--;

    data = (MAC_OFFSET & 0xFF00) >> 8;
    outl_iic(data, IIC_FI2CDTTX);
    len--;

    data = (MAC_OFFSET & 0x00FF);
    outl_iic(data, IIC_FI2CDTTX);
    len--;

    while (len > 0) {
        data = *mac_buf++ & IIC_FI2CDTTX_DATA_MASK;
        outl_iic(data, IIC_FI2CDTTX);
        mac_length--;
        len--;
    }

    SET_INT(IIC_FI2CI_TE_MASK);

    if(ave_inner_mac_check_busy(IIC_FI2CSR_BB_MASK)){
        printk("Error!! %s(%d)\n", __FUNCTION__, __LINE__);
        iounmap((void *)iic_reg_base);
        iounmap((void *)mp_irq_reg_base);
        return (-1);
    }
    data = IIC_FI2CCR_MST_MASK | IIC_FI2CCR_STA_MASK;
    outl_iic(data, IIC_FI2CCR);

    if(ave_inner_mac_wait_interrupt(IIC_FI2CI_TE_MASK, &status)){
        printk("Error!! %s(%d)\n", __FUNCTION__, __LINE__);
        iounmap((void *)iic_reg_base);
        iounmap((void *)mp_irq_reg_base);
        return (-1);
    }

    while (mac_length > 0) {
        data = *mac_buf++ & IIC_FI2CDTTX_DATA_MASK;
        outl_iic(data, IIC_FI2CDTTX);
        mac_length--;
    }
    if(ave_inner_mac_wait_interrupt(IIC_FI2CI_TE_MASK, &status)){
        printk("Error!! %s(%d)\n", __FUNCTION__, __LINE__);
        iounmap((void *)iic_reg_base);
        iounmap((void *)mp_irq_reg_base);
        return (-1);
    }
    data = IIC_FI2CCR_MST_MASK | IIC_FI2CCR_STO_MASK;
    outl_iic(data, IIC_FI2CCR);

    iounmap((void *)iic_reg_base);
    iounmap((void *)mp_irq_reg_base);

    return 0;
}

static int ave_inner_mac_check_busy(unsigned int factor){
    int i;
    unsigned int data;

    for(i = 0 ; i < (IIC_TIMEOUT/IIC_DELAY) ; i++){
        udelay(IIC_DELAY);
        inl_iic(data, IIC_FI2CSR);
        if(0 == (data & factor)){
            return 0;
        }
    }
    printk("EEPROM device or I2C bus busy!! : 0x%x!!\n", factor);
    return (-1);
}

static int ave_inner_mac_wait_interrupt(unsigned int factor, unsigned int *status){
    int ret = 0;
#ifdef AVE_USE_INT
    int i;
    unsigned int status_int, status_cnt;

    for(i = 0 ; i < (IIC_TIMEOUT/IIC_DELAY) ; i++){
        udelay(IIC_DELAY);
        inl_iic(status_int, IIC_FI2CINT);
        if(status_int & IIC_FI2CI_TE_MASK){
            break;
        }
        else if(status_int & IIC_FI2CI_RB_MASK){
            *status = status_int;
            return 0;
        }
        else if(status_int & IIC_FI2CI_NA_MASK){
            outl_iic((IIC_FI2CCR_MST_MASK | IIC_FI2CCR_STO_MASK), IIC_FI2CCR);
            printk("No reply from EEPROM!!\n");
            ret = (-1);
            break;
        }
        else if(status_int & IIC_FI2CI_AL_MASK){
            printk("Lost arbitrarion to EEPROM!!\n");
            ret = (-1);
            break;
        }
#if 0
        else if(status_int){
            printk("Unknown interrupt from EEPROM!! : 0x%x\n", status_int);
            ret = (-1);
            break;
        }
#endif
    }
    if((IIC_TIMEOUT/IIC_DELAY) == i){
        inl_iic(status_cnt, IIC_FI2CSR);
        printk("Timeout wait for interrunt!! INT=0x%x SR=0x%x\n", status_int, status_cnt);
        ret = (-1);
    }
    outl_iic(status_int, IIC_FI2CIC);
#else 
    udelay(IIC_DELAY);
#endif 
    return ret;
}
#endif 

int ave_get_macaddr(char *mac_addr)
{

    ave_util_mac_read(mac_addr);

    if( (*(uint32_t *)&mac_addr[0] == 0xFFFFFFFF)
        && (*(unsigned short *)&mac_addr[4] == 0xFFFF) ){
        printk("ave: You need to set MAC address by ioctl/ifconfig.\n");
        return -1;
    }

    return 0;
}

#ifndef AVE_MACHINE_BIGENDIAN
void ave_endian_change(void *data)
{
    char *data_P = (char*)data;
    union conv_data_u{
        uint32_t hex_data;
        char string[4];
    } conv_data;
      
    conv_data.hex_data = *(uint32_t *)data;
    data_P[0] = conv_data.string[3];
    data_P[1] = conv_data.string[2];
    data_P[2] = conv_data.string[1];
    data_P[3] = conv_data.string[0];
}
#else
void ave_endian_change(void *data)
{
}
#endif

void ave_phy_set_afe_initialize_param(struct net_device *netdev, int autoselect)
{
  struct ave_private *ave_priv = netdev_priv(netdev);
  unsigned long reg;
  
  reg = ave_priv->mii_if.mdio_read(netdev, ave_priv->phy_id, OMNI_MDCTL);
  if(autoselect){
    reg |= OMNI_MDCTL_AUTOMDIX;
  } else {
    reg &= ~OMNI_MDCTL_AUTOMDIX;
  }
  ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_MDCTL, reg);

  ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x0000);
  ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x0400);
  ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x0000);
  ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x0400);
  ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_WRITE_DATA, 0x4400);
  ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x4415);
  ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_WRITE_DATA, 0x3700);
  ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x4413);
}

void ave_phy_set_afe_linkspeed_param(struct net_device *netdev, int linkspeed)
{
  struct ave_private *ave_priv = netdev_priv(netdev);

  if(linkspeed){
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x0000);
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x0400);
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x0000);
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x0400);
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_WRITE_DATA, 0x0003);
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x4418);
  } else {
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x0000);
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x0400);
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x0000);
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x0400);
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_WRITE_DATA, 0x0005);
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x4418);
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_WRITE_DATA, 0x8226);
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x4416);
  }
}

int ave_omniphy_linkstatus(struct net_device *netdev)
{
  struct ave_private *ave_priv = netdev_priv(netdev);
  unsigned long lreg, bmcr, bmsr,sctrl;
  int link_status =  AVE_LS_DOWN;

  ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x0000);
  ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x0400);
  ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x0000);
  ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x0400);
  ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_WRITE_DATA, 0xB200);
  ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x4401);
  ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, 0x8720);
  lreg = ave_priv->mii_if.mdio_read(netdev, ave_priv->phy_id, OMNI_TST_READ2);

  bmcr = ave_priv->mii_if.mdio_read(netdev, ave_priv->phy_id, MII_BMCR);
  bmsr = ave_priv->mii_if.mdio_read(netdev, ave_priv->phy_id, MII_BMSR);
  bmsr = ave_priv->mii_if.mdio_read(netdev, ave_priv->phy_id, MII_BMSR);
  sctrl = ave_priv->mii_if.mdio_read(netdev, ave_priv->phy_id, OMNI_PHY_SP_CTRL_STS);

  if( (sctrl & 0x4) != 0 ){
    if( (bmsr & 0x4) != 0 ){
       link_status = AVE_LS_UP;
       ave_phy_set_afe_linkspeed_param(netdev,0);
    }
    else{
       link_status = AVE_LS_DOWN;
    }
  }
  else{
    if( (lreg & 0x20) != 0 ){
       link_status = AVE_LS_UP;
       ave_phy_set_afe_linkspeed_param(netdev,1);
    }
    else{
       link_status = AVE_LS_DOWN;
    }
  }
  return link_status;
}

void ave_phy_soft_reset(struct net_device *netdev)
{
  struct ave_private *ave_priv = netdev_priv(netdev);

  ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_BMCR, 0x8000);
}


uint32_t ave_omniphy_link_status_chk_counter=0;
uint32_t ave_omniphy_bmcr_retention = 0x0;
int ave_omniphy_link_down_reset = 0;

void ave_omniphy_link_status_update(struct net_device *netdev)
{
    struct ave_private  *ave_priv = netdev_priv(netdev);
    int link_status;

    ave_omniphy_link_status_chk_counter++;
    if(ave_omniphy_link_status_chk_counter >= 4) { 
        if(ave_omniphy_link_down_reset == 0){ 
            link_status = ave_omniphy_linkstatus(netdev);
            if(link_status == AVE_LS_DOWN){ 
                ave_omniphy_bmcr_retention = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, MII_BMCR);
                ave_omniphy_link_down_reset=1;
                ave_phy_soft_reset(netdev);
            }
        } else if(ave_omniphy_link_down_reset==1){  
            if(ave_omniphy_bmcr_retention & 0x1000){
                ave_phy_set_afe_initialize_param(netdev,1);
            } else {
                ave_phy_set_afe_initialize_param(netdev,0);
                if(ave_omniphy_bmcr_retention & BMCR_SPEED100){
                    ave_phy_set_afe_linkspeed_param(netdev, 1);
                } else {
                    ave_phy_set_afe_linkspeed_param(netdev, 0);
                }
            }
            ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_BMCR, ave_omniphy_bmcr_retention);
            ave_omniphy_link_down_reset=2;    
        } else {  
            link_status = ave_omniphy_linkstatus(netdev);
            if(link_status != AVE_LS_DOWN){ 
                ave_omniphy_link_down_reset=0;    
            }
        }
        ave_omniphy_link_status_chk_counter=0;
    }
}

void ave_platform_init(void)
{
    return;
}

int ave_phy_init_micrel(struct net_device *netdev, uint32_t phy_sid1, uint32_t phy_sid2, int num)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val;

    printk("ave: micrel: phy_id 0x%02x sid 0x%08x\n", num, (phy_sid1<<16 | phy_sid2));

    ave_priv->phy_id = num;
    ave_priv->mii_if.phy_id = num;

    ave_phy_reset_period = 1000;    
    ave_phy_reset_stability = 100;  

    reg_val = ave_priv->mii_if.mdio_read(netdev, ave_priv->phy_id, MII_MCRL_CTRL2);
    reg_val &= (~MCRL_POWERSAVE); 
#ifndef AVE_MICREL_AUTOMDIX_ON
    if ( netif_msg_link(ave_priv) ) {
        DBG_PRINT("Disable AutoMDI/MDIX\n");
            }
    reg_val |= MCRL_PAIRSWAP_D;   
#endif
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_MCRL_CTRL2, reg_val);

#ifdef AVE_ENABLE_PAUSE
    reg_val = ave_priv->mii_if.mdio_read(netdev, ave_priv->phy_id, MII_ADVERTISE);
    reg_val |= MCRL_ADV_PAUSE;    
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_ADVERTISE, reg_val);
#endif 

    if( (phy_sid2 & 0x03f0) != 0x0150 ){ 
        reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, MII_BMCR);

        if( (reg_val & BMCR_ANENABLE) != 0 ){
            ave_api_mdio_read(netdev, ave_priv->phy_id, MII_MCRL_ICS);
            reg_val |= BMCR_ANRESTART;
            ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_BMCR, (int)reg_val);
        }
    } 
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_MCRL_ICS,
                             MCRL_CTRL_UP | MCRL_CTRL_DOWN);

    if( (phy_sid2 & 0x03f0) == 0x0150 ){ 
        ave_ksz8051_bugfix = 1;  
        reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, MII_BMCR);

        if( (reg_val & BMCR_ANENABLE) != 0 ){
            ave_api_mdio_read(netdev, ave_priv->phy_id, MII_MCRL_ICS);
            udelay(500);
            reg_val &= ~BMCR_ANENABLE;
            ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_BMCR, (int)reg_val);
            udelay(1000);
            reg_val |= BMCR_ANENABLE;
            ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_BMCR, (int)reg_val);
        }
    } 
    return AVE_PHY_MCRL;
}

int ave_phy_init_atheros(struct net_device *netdev, uint32_t phy_sid1, uint32_t phy_sid2, int num)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val;

    printk("ave: atheros: phy_id 0x%02x sid 0x%08x\n", num, (phy_sid1<<16 | phy_sid2));

    ave_priv->phy_id = num;
    ave_priv->mii_if.phy_id = num;

    ave_phy_reset_period = 1000;    
    ave_phy_reset_stability = 1000;  

#ifdef AVE_ENABLE_PAUSE
    reg_val = ave_priv->mii_if.mdio_read(netdev, ave_priv->phy_id, MII_ADVERTISE);
    reg_val |= MCRL_ADV_PAUSE;    
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_ADVERTISE, reg_val);
#endif 

    reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, MII_BMCR);

    if( (reg_val & BMCR_ANENABLE) != 0 ){
        ave_api_mdio_read(netdev, ave_priv->phy_id, MII_ATHRS_ISR);
        reg_val |= BMCR_ANRESTART;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_BMCR, (int)reg_val);
    }
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_ATHRS_ICR,
                             (ATHRS_INT_LKUP | ATHRS_INT_LKDN) );

    AVE_Wreg(AVE_SIGNAL_H, AVE_SIGNAL);

    return AVE_PHY_ATHRS;
}

int ave_phy_init_realtek(struct net_device *netdev, uint32_t phy_sid1, uint32_t phy_sid2, int num)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val;
#ifdef AVE_DISABLE_EEE
    uint32_t reg2_val;
#endif

    printk("ave: realtek: phy_id 0x%02x sid 0x%08x\n", num, (phy_sid1<<16 | phy_sid2));

    ave_priv->phy_id = num;
    ave_priv->mii_if.phy_id = num;

    if(ave_priv->phy_sid == AVE_PHYSID_RTL8201E){
        ave_phy_reset_period = 20*1000;      
        ave_phy_reset_stability = 280*1000;  
        ave_poll_link = 1; 
    }
    else if(ave_priv->phy_sid == AVE_PHYSID_RTL8201FL){
        ave_phy_reset_period = 20*1000;      
        ave_phy_reset_stability = 160*1000;  
        ave_poll_link = 1; 

#ifdef AVE_ENABLE_RTL8201FR
        ave_mdio_select_page(netdev, RTL_REG_PAGE7);
        reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, RTL_REG19);
        reg_val |= MII_RTL8201F_LED0_WOL_SEL;
        ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG19, reg_val);

        reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, RTL_REG23);
        reg_val &= ~(MII_RTL8201F_WOL_INT_SEL);
        ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG23, reg_val);

        if(ave_rtl8201fr_wol_init_state != 1){
            ave_mdio_select_page(netdev, RTL_REG_PAGE17);
            ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG16, 0x0);
            ave_mdio_select_page(netdev, RTL_REG_PAGE0);
            ave_rtl8201fr_wol_init_state = 1;
        }
#endif

        reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, RTL_P0R31_PSR);
        reg_val &= 0xffffff00;
        reg_val |= RTL_REG_PAGE7;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, RTL_P0R31_PSR, reg_val);

        reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, RTL_P7R24_SSCR);  
        reg_val |= RTL_SSC_DISABLE;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, RTL_P7R24_SSCR, reg_val);

        reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, RTL_P0R31_PSR);
        reg_val &= 0xffffff00;
        reg_val |= RTL_REG_PAGE0;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, RTL_P0R31_PSR, reg_val);

#if 0   
        reg_val = RTL_MMD_ADDR_DEV7;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, RTL_P0R13_MACR, reg_val);

        reg_val = RTL_MMDD7_EEEAR;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, RTL_P0R14_MAADR, reg_val);

        reg_val = RTL_MMD_DATA_DEV7;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, RTL_P0R13_MACR, reg_val);

        reg2_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, RTL_P0R14_MAADR);

        reg_val = RTL_MMD_ADDR_DEV7;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, RTL_P0R13_MACR, reg_val);

        reg_val = RTL_MMDD7_EEEAR;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, RTL_P0R14_MAADR, reg_val);

        reg_val = RTL_MMD_DATA_DEV7;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, RTL_P0R13_MACR, reg_val);

        reg2_val &= ~RTL_EEE_ENABLE;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, RTL_P0R14_MAADR, reg2_val);
#endif   
    }
    else if( (ave_priv->phy_sid&0xFFFFFFF0) == AVE_PHYSID_RTL8211E_FAMILY){
        ave_phy_reset_period    = 20*1000;   
        ave_phy_reset_stability = 40*1000;  
        ave_gbit_mode = 1;                   
        ave_priv->mii_if.supports_gmii = 1;        

        reg_val = MII_RTL8211E_INT_LKCHG;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_RTL8211E_ICR, (int)reg_val);

#ifdef AVE_DISABLE_EEE
        reg_val = RTL_MMD_ADDR_DEV7;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, RTL_P0R13_MACR, reg_val);

        reg_val = RTL_MMDD7_EEEAR;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, RTL_P0R14_MAADR, reg_val);

        reg_val = RTL_MMD_DATA_DEV7;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, RTL_P0R13_MACR, reg_val);

        reg2_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, RTL_P0R14_MAADR);

        reg_val = RTL_MMD_ADDR_DEV7;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, RTL_P0R13_MACR, reg_val);

        reg_val = RTL_MMDD7_EEEAR;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, RTL_P0R14_MAADR, reg_val);

        reg_val = RTL_MMD_DATA_DEV7;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, RTL_P0R13_MACR, reg_val);

        reg2_val &= ~(RTL_1000T_EEE_ENABLE | RTL_100TX_EEE_ENABLE);
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, RTL_P0R14_MAADR, reg2_val);
#endif
    }

#ifdef AVE_ENABLE_PAUSE
    reg_val = ave_priv->mii_if.mdio_read(netdev, ave_priv->phy_id, MII_ADVERTISE);
    reg_val |= ADVERTISE_PAUSE_CAP;    
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_ADVERTISE, reg_val);
#endif 

    reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, MII_BMCR);

    if( (reg_val & BMCR_ANENABLE) != 0 ){

        if(ave_priv->phy_sid == AVE_PHYSID_RTL8201E){
            ;
        }
        else if(ave_priv->phy_sid == AVE_PHYSID_RTL8201FL){
            ave_api_mdio_read(netdev, ave_priv->phy_id, MII_RTL8201FN_ISR);
        }
        else if( (ave_priv->phy_sid&0xFFFFFFF0) == AVE_PHYSID_RTL8211E_FAMILY){
            ave_api_mdio_read(netdev, ave_priv->phy_id, MII_RTL8211E_ISR);
        }

        reg_val |= BMCR_ANRESTART;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_BMCR, (int)reg_val);
    }
    return AVE_PHY_REALTEK;
}

int ave_phy_init_smsc(struct net_device *netdev, uint32_t phy_sid1, uint32_t phy_sid2, int num)
{
    struct ave_private *ave_priv = netdev_priv(netdev);

    printk("ave: smsc: phy_id 0x%02x sid 0x%08x\n", num, (phy_sid1<<16 | phy_sid2));

    ave_priv->phy_id = num;
    ave_priv->mii_if.phy_id = num;

    ave_phy_reset_period = 100; 
    ave_phy_reset_stability = 1;    

    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_BMCR,
                    BMCR_FULLDPLX | BMCR_ANRESTART |
                    BMCR_ANENABLE | BMCR_SPEED100);

    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_SMSC_IMR,
                    SMSC_ANCOMP | SMSC_LINKDOWN);

    return AVE_PHY_SMSC;
}

int ave_phy_init_omniphy(struct net_device *netdev, uint32_t phy_sid1, uint32_t phy_sid2, int num)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val;

    printk("ave: OmniPHY: phy_id 0x%02x sid 0x%08x\n", num, (phy_sid1<<16 | phy_sid2));

    ave_priv->phy_id = num;
    ave_priv->mii_if.phy_id = num;

    ave_phy_reset_period = 5000;      
    ave_phy_reset_stability = 10000;  

    ave_poll_link = 1; 


#ifdef AVE_ENABLE_PAUSE
    reg_val = ave_priv->mii_if.mdio_read(netdev, ave_priv->phy_id, MII_ADVERTISE);
    reg_val |= ADVERTISE_PAUSE_CAP;    
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_ADVERTISE, reg_val);
#endif 

    ave_priv->mii_if.mdio_read(netdev, ave_priv->phy_id, OMNI_INTR_SOURCE);

#ifdef AVE_OMNIPHY_ENABLE_WOL
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_INTR_MASK, (OMNI_INTR_WOL1 | OMNI_INTR_WOL2));
#else
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_INTR_MASK, 0x00);
#endif

    ave_phy_set_afe_initialize_param(netdev,1);
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_BMCR, 0x1200);

    return AVE_PHY_OMNIPHY;
}


int ave_phy_init_unknown(struct net_device *netdev, uint32_t phy_sid1, uint32_t phy_sid2, int num)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val;

    printk("ave: unknown: phy_id 0x%02x sid 0x%08x\n", num, (phy_sid1<<16 | phy_sid2));

    ave_priv->phy_id = num;
    ave_priv->mii_if.phy_id = num;

    ave_phy_reset_period = 20*1000;      
    ave_phy_reset_stability = 280*1000;  

#ifdef AVE_ENABLE_PAUSE
    reg_val = ave_priv->mii_if.mdio_read(netdev, ave_priv->phy_id, MII_ADVERTISE);
    reg_val |= ADVERTISE_PAUSE_CAP;    
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_ADVERTISE, reg_val);
#endif 

    reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, MII_BMCR);

    if( (reg_val & BMCR_ANENABLE) != 0 ){
        reg_val |= BMCR_ANRESTART;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_BMCR, (int)reg_val);
    }
    ave_poll_link = 1; 
    ave_phy_reset_period    =  20*1000;  
    ave_phy_reset_stability = 280*1000;  

    return AVE_PHY_UNKNOWN;
}

int ave_phy_init(struct net_device *netdev)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t phy_sid1, phy_sid2; 
    int no;

    ave_poll_link = 0;  
    ave_ksz8051_bugfix = 0;  
    ave_gbit_mode = 0;  

    printk(KERN_DEBUG "ave: phy_id = 0x%08x\n", ave_priv->phy_id);
    printk(KERN_INFO "ave: phy_id auto-detection start.\n");

    for ( no = 1; no < 0x20; no++ ) {
        phy_sid1 = ave_priv->mii_if.mdio_read(netdev, no, MII_PHYSID1);
        phy_sid2 = ave_priv->mii_if.mdio_read(netdev, no, MII_PHYSID2);
        ave_priv->phy_sid = (phy_sid1 << 16 | phy_sid2);

        if ( ((phy_sid1 & 0xffff) == 0x0022) && ((phy_sid2 & 0xfC00) == 0x1400) ) {
            gphy_type = ave_phy_init_micrel(netdev, phy_sid1, phy_sid2, no);
            break;

        } else if ( ((phy_sid1 & 0xffff) == 0x004d) && ((phy_sid2 & 0xfC00) == 0xd000) ) {
            gphy_type = ave_phy_init_atheros(netdev, phy_sid1, phy_sid2, no);
            break;

        } else if ( ((phy_sid1 & 0xffff) == 0x001C) && ((phy_sid2 & 0xfC00) == 0xC800) ) {
            gphy_type = ave_phy_init_realtek(netdev, phy_sid1, phy_sid2, no);
            break;

        } else if ( ((phy_sid1 & 0xffff) == 0x0007) && ((phy_sid2 & 0xfC00) == 0xC000) ) {
            gphy_type = ave_phy_init_smsc(netdev, phy_sid1, phy_sid2, no);
            break;

        } else if ( ((phy_sid1 & 0xffff) == 0x0000) && ((phy_sid2 & 0xfC00) == 0x0000) ) {
            gphy_type = ave_phy_init_omniphy(netdev, phy_sid1, phy_sid2, no);
            break;

        } else if ( ((phy_sid1 & 0xffff) != 0xffff) && ((phy_sid2 & 0xfC00) == 0xfc000) ) {
            gphy_type = ave_phy_init_unknown(netdev, phy_sid1, phy_sid2, no);
            break;
        }
    }
    if ( no > 0x1f ) {
        printk(KERN_ERR "ave: couldn't proper PHY. phy_id is default.(0x%02x)\n",
           ave_priv->phy_id);
        printk(KERN_ERR "ave: ave_panic(2)\n");
        ave_panic(2);
        return -1;
    }

  return no;
}

int ave_phy_intr_chkstate_negate(struct net_device *netdev)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val=0;

    if ( gphy_type == AVE_PHY_MCRL ) {
        reg_val = ave_priv->mii_if.mdio_read(netdev, ave_priv->phy_id, MII_MCRL_ICS);
        if ( netif_msg_link(ave_priv) ) {
            DBG_PRINT("ave: PHY INTR 0x%02x\n", (char)(reg_val & 0xff));
        }

        if ( (reg_val & MCRL_STS_UP) != 0x0 ) {
            return AVE_LS_UP;
        } else if ( (reg_val & MCRL_STS_DOWN) != 0x00 ) {
            return AVE_LS_DOWN;
        } else {
            return AVE_LS_NOTHING;
        }

    } else if ( gphy_type == AVE_PHY_ATHRS ) {
        reg_val = ave_priv->mii_if.mdio_read(netdev, ave_priv->phy_id, MII_ATHRS_ISR);
        if ( netif_msg_link(ave_priv) ) {
            DBG_PRINT("ave: PHY INTR 0x%02x\n", (char)(reg_val & 0xff));
        }

        if ( (reg_val & ATHRS_INT_LKUP) != 0x0 ) {
            return AVE_LS_UP;
        } else if ( (reg_val & ATHRS_INT_LKDN) != 0x00 ) {
            return AVE_LS_DOWN;
        } else {
            return AVE_LS_NOTHING;
        }

    } else if ( gphy_type == AVE_PHY_REALTEK ) {
        if( ave_priv->phy_sid  == AVE_PHYSID_RTL8201FL ){
            reg_val = ave_priv->mii_if.mdio_read(netdev, ave_priv->phy_id, MII_RTL8201FN_ISR);
        } else if( (ave_priv->phy_sid & 0xfffffff0) == AVE_PHYSID_RTL8211E_FAMILY ){
            reg_val = ave_priv->mii_if.mdio_read(netdev, ave_priv->phy_id, MII_RTL8211E_ISR);
        } else {
            ; 
        }
        if ( netif_msg_link(ave_priv) ) {
            DBG_PRINT("ave: PHY INTR 0x%04x\n", (unsigned short)reg_val);
        }

        if(ave_util_mii_link_ok(netdev) != 0){
            return AVE_LS_UP;
        }
        else{
            return AVE_LS_DOWN;
        }

    } else if ( gphy_type == AVE_PHY_OMNIPHY ) {

        if(ave_omniphy_link_down_reset != 1){
            reg_val = ave_api_mdio_read(netdev, ave_priv->phy_id, OMNI_INTR_SOURCE);
            if (reg_val & (OMNI_INTR_WOL1 | OMNI_INTR_WOL2)){
                DBG_PRINT("ave: Catch a magic paket.\n");
            }
        } 

        return ( ave_omniphy_linkstatus(netdev) );

    } else {  
        reg_val = ave_priv->mii_if.mdio_read(netdev, ave_priv->phy_id, MII_SMSC_ISFR);
        if ( netif_msg_link(ave_priv) ) {
            DBG_PRINT(KERN_DEBUG "ave: PHY INTR 0x%04x\n", (unsigned short)reg_val);
        }

        if ( (reg_val & SMSC_ANCOMP) != 0x0 ) {
            return AVE_LS_UP;
        } else if ( (reg_val & SMSC_LINKDOWN) != 0x0 ) {
            return AVE_LS_DOWN;
        } else {
            return AVE_LS_NOTHING;
        }

    }
}


void ave_util_mii_check_link(struct net_device *netdev)
{
    struct ave_private  *ave_priv = netdev_priv(netdev);

    mii_check_link(&ave_priv->mii_if);

    if ( gphy_type == AVE_PHY_OMNIPHY ) {
        if(ave_omniphy_linkstatus(netdev) == AVE_LS_DOWN){
            netif_carrier_off(ave_priv->mii_if.dev);
        }
    }
}

int ave_util_mii_link_ok(struct net_device *netdev)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    int ret_val = 1;

    if ( gphy_type == AVE_PHY_OMNIPHY ) {
        if( ave_omniphy_linkstatus(netdev) == AVE_LS_DOWN) {
            ret_val = 0;
        }
    }
    else{
        ret_val = mii_link_ok(&ave_priv->mii_if);
    }
    return(ret_val);
}


uint32_t ave_util_mmd_read(struct net_device *netdev, uint32_t dev_id, uint32_t offset)
{
    struct ave_private  *ave_priv = netdev_priv(netdev);
    uint32_t phyreg_val = 0;

    if(  (ave_priv->phy_sid == AVE_PHYSID_RTL8201FL)  
      || ((ave_priv->phy_sid & 0xfffffff0) == AVE_PHYSID_RTL8211E_FAMILY) 
      ){

        phyreg_val = (dev_id & 0x0000001F);  
        ave_api_mdio_write(netdev, ave_priv->phy_id, 13, phyreg_val);
        ave_api_mdio_write(netdev, ave_priv->phy_id, 14, offset);

        phyreg_val = (0x0004000 | (dev_id & 0x0000001F));  
        ave_api_mdio_write(netdev, ave_priv->phy_id, 13, phyreg_val);

        phyreg_val = ave_api_mdio_read(netdev, ave_priv->phy_id, 14);
    }

    return(phyreg_val);
}

uint64_t ave_cache_control(struct net_device *netdev, uint64_t head, uint32_t length, int cflag)
{
    dma_addr_t paddr;

    paddr = dma_map_single(netdev->dev.parent, (char *)head, length, cflag);
    if (dma_mapping_error(netdev->dev.parent, paddr)) {
        printk("It failed to obtain the physical address of the DMA buffer.\n");
        return -ENOMEM;
    }

    return (uint64_t)paddr;
}


void ave_mdio_select_page(struct net_device *netdev, uint32_t page)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val;

    reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, RTL_P0R31_PSR);
    reg_val &= 0xffffff00;
    reg_val |= page;
    ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_P0R31_PSR, reg_val);
    return;
}

void ave_mdio_select_extention_page(struct net_device *netdev, uint32_t ext_page)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val;

    reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, RTL_EPAGSR);
    reg_val &= 0xfffff000;
    reg_val |= ext_page;
    ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_EPAGSR, reg_val);
    return;
}

uint32_t ave_get_rtl8211_wol_setting(struct net_device *netdev)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val;
    uint32_t wolopts = 0;

    ave_mdio_select_page(netdev, RTL_REG_PAGE7);

    ave_mdio_select_extention_page(netdev, RTL_EXT_PAGE109);

    reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, RTL_REG21);

    ave_mdio_select_extention_page(netdev, RTL_EXT_PAGE0);

    ave_mdio_select_page(netdev, RTL_REG_PAGE0);

    if(reg_val & RTL_WOL_LINKCHANGE) {
        wolopts |= WAKE_PHY;
    }

    if(reg_val & RTL_WOL_MAGICPACKET) {
        wolopts |= WAKE_MAGIC;
    }

    if(reg_val & RTL_WOL_UNICAST) {
        wolopts |= WAKE_UCAST;
    }

    if(reg_val & RTL_WOL_MULTICAST) {
        wolopts |= WAKE_MCAST;
    }

    if(reg_val & RTL_WOL_BROADCAST) {
        wolopts |= WAKE_BCAST;
    }

    return wolopts;
}

void ave_set_rtl8211_wol_setting(struct net_device *netdev, uint32_t ave_wolopts)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val;
    uint32_t wolopts_conv = 0;

    ave_mdio_select_page(netdev, RTL_REG_PAGE7);
    ave_mdio_select_extention_page(netdev, RTL_EXT_PAGE109);

    reg_val = ave_api_mdio_read(netdev, ave_priv->phy_id, RTL_REG22);
    reg_val |= RTL_WOL_RESET;
    ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG22, reg_val);

    ave_mdio_select_page(netdev, RTL_REG_PAGE7);
    ave_mdio_select_extention_page(netdev, RTL_EXT_PAGE110);

    reg_val = netdev->dev_addr[0] | (netdev->dev_addr[1] << 8);
    ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG21, reg_val);
    reg_val = netdev->dev_addr[2] | (netdev->dev_addr[3] << 8);
    ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG22, reg_val);
    reg_val = netdev->dev_addr[4] | (netdev->dev_addr[5] << 8);
    ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG23, reg_val);

    ave_mdio_select_page(netdev, RTL_REG_PAGE7);
    ave_mdio_select_extention_page(netdev, RTL_EXT_PAGE109);

    reg_val = RTL_WOL_ACTIVELOWMODE;
    ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG24, reg_val);

    reg_val = RTL_WOL_MAXPACKET_LENGTH;
    ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG22, reg_val);

    if(ave_wolopts & WAKE_PHY) {
        wolopts_conv |= RTL_WOL_LINKCHANGE;
    }

    if(ave_wolopts & WAKE_MAGIC) {
        wolopts_conv |= RTL_WOL_MAGICPACKET;
    }

    if(ave_wolopts & WAKE_UCAST) {
        wolopts_conv |= RTL_WOL_UNICAST;
    }

    if(ave_wolopts & WAKE_MCAST) {
        wolopts_conv |= RTL_WOL_MULTICAST;

        ave_mdio_select_page(netdev, RTL_REG_PAGE7);
        ave_mdio_select_extention_page(netdev, RTL_EXT_PAGE110);

        reg_val = 0xffff;
        ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG24, reg_val);
        ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG25, reg_val);
        ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG26, reg_val);
        ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG27, reg_val);
    }

    if(ave_wolopts & WAKE_BCAST) {
        wolopts_conv |= RTL_WOL_BROADCAST;
    }

    ave_mdio_select_page(netdev, RTL_REG_PAGE7);
    ave_mdio_select_extention_page(netdev, RTL_EXT_PAGE109);

    reg_val = wolopts_conv;
    ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG21, reg_val);

#if 0
    reg_val = ave_api_mdio_read(netdev, ave_priv->phy_id, RTL_REG25);
    reg_val |= RTL_WOL_POWERSAVEMODE;
    ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG25, reg_val);
#endif

    if(wolopts_conv == 0) {
        reg_val = ave_api_mdio_read(netdev, ave_priv->phy_id, RTL_REG25);
        reg_val &= ~RTL_WOL_POWERSAVEMODE;
        ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG25, reg_val);
    }

    ave_mdio_select_extention_page(netdev, RTL_EXT_PAGE0);
    ave_mdio_select_page(netdev, RTL_REG_PAGE0);

    return;
}

uint32_t ave_get_rtl8201_wol_setting(struct net_device *netdev)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val;
    uint32_t wolopts = 0;

    ave_mdio_select_page(netdev, RTL_REG_PAGE17);

    reg_val = ave_api_mdio_read(netdev, ave_priv->phy_id, RTL_REG16);

    ave_mdio_select_page(netdev, RTL_REG_PAGE0);

    if(reg_val & RTL_WOL_LINKCHANGE) {
        wolopts |= WAKE_PHY;
    }

    if(reg_val & RTL_WOL_MAGICPACKET) {
        wolopts |= WAKE_MAGIC;
    }

    if(reg_val & RTL_WOL_UNICAST) {
        wolopts |= WAKE_UCAST;
    }

    if(reg_val & RTL_WOL_MULTICAST) {
        wolopts |= WAKE_MCAST;
    }

    if(reg_val & RTL_WOL_BROADCAST) {
        wolopts |= WAKE_BCAST;
    }

    return wolopts;
}

void ave_set_rtl8201_wol_setting(struct net_device *netdev, uint32_t ave_wolopts)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val;
    uint32_t wolopts_conv = 0;

    ave_mdio_select_page(netdev, RTL_REG_PAGE17);

    reg_val = ave_api_mdio_read(netdev, ave_priv->phy_id, RTL_REG17);
    reg_val |= RTL_WOL_RESET;
    ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG17, reg_val);

    ave_mdio_select_page(netdev, RTL_REG_PAGE18);

    reg_val = netdev->dev_addr[0] | (netdev->dev_addr[1] << 8);
    ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG16, reg_val);
    reg_val = netdev->dev_addr[2] | (netdev->dev_addr[3] << 8);
    ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG17, reg_val);
    reg_val = netdev->dev_addr[4] | (netdev->dev_addr[5] << 8);
    ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG18, reg_val);

    ave_mdio_select_page(netdev, RTL_REG_PAGE17);

    reg_val = RTL_WOL_MAXPACKET_LENGTH;
    ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG17, reg_val);

    if(ave_wolopts & WAKE_PHY) {
        wolopts_conv |= RTL_WOL_LINKCHANGE;
    }

    if(ave_wolopts & WAKE_MAGIC) {
      wolopts_conv |= RTL_WOL_MAGICPACKET;
    }

    if(ave_wolopts & WAKE_UCAST) {
        wolopts_conv |= RTL_WOL_UNICAST;
    }

    if(ave_wolopts & WAKE_MCAST) {
        wolopts_conv |= RTL_WOL_MULTICAST;

        ave_mdio_select_page(netdev, RTL_REG_PAGE18);

        reg_val = 0xffff;
        ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG19, reg_val);
        ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG20, reg_val);
        ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG21, reg_val);
        ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG22, reg_val);
    }

    if(ave_wolopts & WAKE_BCAST) {
        wolopts_conv |= RTL_WOL_BROADCAST;
    }

    ave_mdio_select_page(netdev, RTL_REG_PAGE17);

    reg_val = wolopts_conv;
    ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG16, reg_val);

#if 0 
    ave_mdio_select_page(netdev, RTL_REG_PAGE7);
    reg_val = ave_api_mdio_read(netdev, ave_priv->phy_id, RTL_REG20);
    reg_val |= RTL_WOL_TX_ISOLATE;
    ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG20, reg_val);

    ave_mdio_select_page(netdev, RTL_REG_PAGE17);
    reg_val = ave_api_mdio_read(netdev, ave_priv->phy_id, RTL_REG19);
    reg_val |= RTL_WOL_RX_ISOLATE;
    ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG19, reg_val);
#endif

#ifdef AVE_ENABLE_RTL8201FR
    ave_mdio_select_page(netdev, RTL_REG_PAGE7);
    reg_val = ave_api_mdio_read(netdev, ave_priv->phy_id, RTL_REG23);
    reg_val &= ~MII_RTL8201F_WOL_INT_SEL;
    ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG23, reg_val);

    ave_poll_link = 1;
#endif

    if(wolopts_conv == 0) {
        ave_mdio_select_page(netdev, RTL_REG_PAGE7);
        reg_val = ave_api_mdio_read(netdev, ave_priv->phy_id, RTL_REG20);
        reg_val &= ~RTL_WOL_TX_ISOLATE;
        ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG20, reg_val);

        ave_mdio_select_page(netdev, RTL_REG_PAGE17);
        reg_val = ave_api_mdio_read(netdev, ave_priv->phy_id, RTL_REG19);
        reg_val &= ~RTL_WOL_RX_ISOLATE;
        ave_api_mdio_write(netdev, ave_priv->phy_id, RTL_REG19, reg_val);

#ifdef AVE_ENABLE_RTL8201FR

#endif
    }

    ave_mdio_select_page(netdev, RTL_REG_PAGE0);

    return;
}

void ave_set_omniphy_wol_reg(struct net_device *netdev, uint32_t address, uint32_t val)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val;

    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_WRITE_DATA, val);
    reg_val = OMNI_TSTCNTL_WRITE | OMNI_TSTCNTL_WOL_REG_SEL | (address & 0x1f);
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, reg_val);
}

uint32_t ave_get_omniphy_wol_reg(struct net_device *netdev, uint32_t address)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val;

    reg_val = OMNI_TSTCNTL_READ | OMNI_TSTCNTL_WOL_REG_SEL | ((address & 0x1f) << 5);
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_TSTCNTL, reg_val);
    reg_val = ave_priv->mii_if.mdio_read(netdev, ave_priv->phy_id, OMNI_TST_READ1);

    return reg_val;
}

void ave_set_omniphy_wol_setting(struct net_device *netdev, uint32_t ave_wolopts)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val;

    if (ave_wolopts & WAKE_MAGIC) {
        reg_val = netdev->dev_addr[5] | (netdev->dev_addr[4] << 8);
        ave_set_omniphy_wol_reg(netdev, OMNI_WOL_MAC_15_0, reg_val);
        reg_val = netdev->dev_addr[3] | (netdev->dev_addr[2] << 8);
        ave_set_omniphy_wol_reg(netdev, OMNI_WOL_MAC_31_16, reg_val);
        reg_val = netdev->dev_addr[1] | (netdev->dev_addr[0] << 8);
        ave_set_omniphy_wol_reg(netdev, OMNI_WOL_MAC_47_32, reg_val);

        reg_val = OMNI_WOL_EN | OMNI_WOL_MAG_PKT_DA_EN | OMNI_WOL_MAG_PKT_BRO_EN;
        ave_set_omniphy_wol_reg(netdev, OMNI_WOL_CONTROL, reg_val);
    } else {
        reg_val = 0;
        ave_set_omniphy_wol_reg(netdev, OMNI_WOL_CONTROL, reg_val);
    }
 
    reg_val = ave_priv->mii_if.mdio_read(netdev, ave_priv->phy_id, OMNI_INTR_MASK);
    ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, OMNI_INTR_MASK, (reg_val | OMNI_INTR_WOL1 | OMNI_INTR_WOL2));
}

int ave_get_omniphy_wol_setting(struct net_device *netdev)
{
    uint32_t reg_val;

    reg_val = ave_get_omniphy_wol_reg(netdev, OMNI_WOL_CONTROL);
    if (reg_val &  OMNI_WOL_EN ) {
        return(WAKE_MAGIC);
    } else {
        return 0;
    }
}

void ave_phy_get_wol(struct net_device *netdev, struct ethtool_wolinfo *wolinfo)
{
    uint32_t wolopts;
    struct ave_private *ave_priv = netdev_priv(netdev);

    if(ave_priv->phy_sid == AVE_PHYSID_RTL8201FL){ 
        wolopts = ave_get_rtl8201_wol_setting(netdev);

        wolinfo->supported = WAKE_PHY | WAKE_UCAST | WAKE_MCAST | WAKE_BCAST | WAKE_MAGIC;
        wolinfo->wolopts  = wolopts;
    }
    else if((ave_priv->phy_sid & 0xfffffff0) == AVE_PHYSID_RTL8211E_FAMILY){ 
        wolopts = ave_get_rtl8211_wol_setting(netdev);

        wolinfo->supported = WAKE_PHY | WAKE_UCAST | WAKE_MCAST | WAKE_BCAST | WAKE_MAGIC;
        wolinfo->wolopts  = wolopts;
    }
    else if(ave_priv->phy_sid  == AVE_PHYSID_OMNIPHY){ 
        wolopts = ave_get_omniphy_wol_setting(netdev);

        wolinfo->supported = WAKE_MAGIC;    
        wolinfo->wolopts  = wolopts;
    }
    else{
        wolopts = 0;
        wolinfo->supported = 0;
        wolinfo->wolopts  = wolopts;
    }

    return;
}

int ave_phy_set_wol(struct net_device *netdev, struct ethtool_wolinfo *wolinfo)
{
    uint32_t wolopts;
    struct ave_private *ave_priv = netdev_priv(netdev);

    if(wolinfo->wolopts & (WAKE_ARP | WAKE_MAGICSECURE)) {
        return -EOPNOTSUPP;
    }

    wolopts = wolinfo->wolopts;

    if(ave_priv->phy_sid == AVE_PHYSID_RTL8201FL){ 
        ave_set_rtl8201_wol_setting(netdev, wolopts);
    }
    else if((ave_priv->phy_sid & 0xfffffff0) == AVE_PHYSID_RTL8211E_FAMILY){ 
        ave_set_rtl8211_wol_setting(netdev, wolopts);
    }
    else if(ave_priv->phy_sid == AVE_PHYSID_OMNIPHY){ 
        if(wolinfo->wolopts & (WAKE_ARP | WAKE_MAGICSECURE | WAKE_PHY | WAKE_UCAST | WAKE_MCAST | WAKE_BCAST)) {
            return -EOPNOTSUPP;
        }
        ave_set_omniphy_wol_setting(netdev, wolopts);
    }

    return 0;
}

uint32_t ave_get_wol_setting(struct net_device *netdev)
{
    struct ave_private *ave_priv = netdev_priv(netdev);

    if(ave_priv->phy_sid == AVE_PHYSID_RTL8201FL){ 
        return ave_get_rtl8201_wol_setting(netdev);
    }
    else if((ave_priv->phy_sid & 0xfffffff0) == AVE_PHYSID_RTL8211E_FAMILY){ 
        return ave_get_rtl8211_wol_setting(netdev);
    }
    else if(ave_priv->phy_sid == AVE_PHYSID_OMNIPHY){ 
        return ave_get_omniphy_wol_setting(netdev);
    }
    return 0;
}

void ave_set_wol_setting(struct net_device *netdev, uint32_t ave_wolopts)
{
    struct ave_private *ave_priv = netdev_priv(netdev);

    if(ave_priv->phy_sid == AVE_PHYSID_RTL8201FL){ 
        ave_set_rtl8201_wol_setting(netdev, ave_wolopts);
    }
    else if((ave_priv->phy_sid & 0xfffffff0) == AVE_PHYSID_RTL8211E_FAMILY){ 
        ave_set_rtl8211_wol_setting(netdev, ave_wolopts);
    }
    else if(ave_priv->phy_sid  == AVE_PHYSID_OMNIPHY){ 
        ave_set_omniphy_wol_setting(netdev, ave_wolopts);
    }
    return;
}

#if defined(AVE_ENABLE_CACHEDMA)
void ave_initialize_coherency_port(void)
{
    uint32_t reg_val;

    AVE_Rreg(reg_val, AVE_AXIRCTL);
    reg_val |= (AVE_AXIXCTL_WA | AVE_AXIXCTL_RA | AVE_AXIXCTL_C | AVE_AXIXCTL_B);
    reg_val &= ~AVE_AXIXCTL_ACP;  
    AVE_Wreg(reg_val, AVE_AXIRCTL);
    AVE_Rreg(reg_val, AVE_AXIRCTL);

    AVE_Rreg(reg_val, AVE_AXIWCTL);
    reg_val |= (AVE_AXIXCTL_WA | AVE_AXIXCTL_RA | AVE_AXIXCTL_C | AVE_AXIXCTL_B);
    reg_val &= ~AVE_AXIXCTL_ACP;  
    AVE_Wreg(reg_val, AVE_AXIWCTL);
    AVE_Rreg(reg_val, AVE_AXIWCTL);

    return;
}

#endif

int ave_panic(int param)
{
    printk("ave: kernel panic!\n");
    return 0; 
}
