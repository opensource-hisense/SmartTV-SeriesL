/*** CONFIDENTIAL ***/
/* Copyright (C) 2015, Socionext Inc. */
#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef AVE_SN_PLATFOEM_H
#define AVE_SN_PLATFOEM_H

#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
#include <linux/config.h>
#else 
#endif 

#ifdef MODULE           
#include <linux/module.h>
#endif  

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/ioport.h>   
#include <linux/proc_fs.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,20)
#include <linux/seq_file.h>
#endif


#include <linux/platform_device.h>
#include <linux/mii.h>      
#include <linux/ethtool.h>  
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,8)  
#include <linux/etherdevice.h>
#endif 

#include <linux/delay.h>    

#include <linux/ipv6.h>

    
   
 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,20) 
#include <linux/interrupt.h>
#endif
    
 
   
   
  

#include <asm/cacheflush.h>




#if 0
#define AVE_SEPARATE_WAIT_FOR_PHY_READY 
#endif

#define AVE_ENABLE_LONGWAIT_BY_SLEEP     
#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP
    #undef AVE_SEPARATE_WAIT_FOR_PHY_READY  
#endif

#define AVE_ENABLE_IOREMAP

#undef  AVE_MACHINE_BIGENDIAN 

#define AVE_ENABLE_RTL8201FR


#define MII_SMSC_ISFR   (29)    
#define MII_SMSC_IMR    (30)    
#define SMSC_ANCOMP     BIT6    
#define SMSC_LINKDOWN   BIT4    

#define MII_MCRL_ICS    (0x1B)  
#define MCRL_CTRL_DOWN  BIT10   
#define MCRL_CTRL_UP    BIT8    
#define MCRL_STS_DOWN   BIT2    
#define MCRL_STS_UP     BIT0    
#define MII_MCRL_CTRL2  (0x1F)  
#define MCRL_PAIRSWAP_D BIT13   
#define MCRL_POWERSAVE  BIT10   
#define MCRL_ADV_PAUSE  BIT10   

#define MII_ATHRS_ICR   (0x12)  
#define MII_ATHRS_ISR   (0x13)  
#define ATHRS_INT_LKUP  BIT10   
#define ATHRS_INT_LKDN  BIT11   

#define RTL_P0R13_MACR    (13)     
#define RTL_P0R14_MAADR   (14)     
#define RTL_P0R31_PSR     (31)     
#define RTL_P7R24_SSCR    (24)     
#define RTL_MMDD7_EEEAR   (0x3c)   
#define RTL_REG_PAGE0     (0)      
#define RTL_REG_PAGE7     (7)      
#define RTL_MMD_ADDR_DEV7 (0x0007) 
#define RTL_MMD_DATA_DEV7 (0x4007) 
#define RTL_SSC_DISABLE   BIT0     
#define RTL_EEE_ENABLE    BIT1     
#define RTL_100TX_EEE_ENABLE   BIT1    
#define RTL_1000T_EEE_ENABLE   BIT2    
#define RTL_REG_PAGE17    (17)
#define RTL_REG_PAGE18    (18)
#define RTL_REG16         (16)     
#define RTL_REG17         (17)     
#define RTL_REG18         (18)     
#define RTL_REG19         (19)     
#define RTL_REG20         (20)     
#define RTL_REG21         (21)     
#define RTL_REG22         (22)     
#define RTL_REG23         (23)     
#define RTL_REG24         (24)     
#define RTL_REG25         (25)     
#define RTL_REG26         (26)     
#define RTL_REG27         (27)     
#define RTL_EPAGSR        (30)     
#define RTL_EXT_PAGE0     (0x000)  
#define RTL_EXT_PAGE109   (0x06d)  
#define RTL_EXT_PAGE110   (0x06e)  
#define RTL_WOL_LINKCHANGE  (0x2000) 
#define RTL_WOL_MAGICPACKET (0x1000) 
#define RTL_WOL_UNICAST     (0x0400) 
#define RTL_WOL_MULTICAST   (0x0200) 
#define RTL_WOL_BROADCAST   (0x0100) 
#define RTL_WOL_MAXPACKET_LENGTH (0x1fff) 
#define RTL_WOL_ACTIVELOWMODE    (0x0000) 
#define RTL_WOL_PULSEMODE        (0x0003) 
#define RTL_WOL_RESET            BIT15   
#define RTL_WOL_POWERSAVEMODE    BIT0    
#define RTL_WOL_RX_ISOLATE       BIT15
#define RTL_WOL_TX_ISOLATE       BIT15
#define RTL_RG_WOL_SEL           BIT10   

#define MII_RTL8211E_ICR    (0x12)  
#define MII_RTL8211E_ISR    (0x13)  
#define MII_RTL8211E_INT_LKCHG  BIT10  
#define MII_RTL8201FN_ISR    (0x1e)  
#define MII_RTL8201FN_PSR    (0x1f)  
#define MII_RTL8201F_INT_LKCHG  BIT13  
#define MII_RTL8201F_INT_DUPCHG BIT12  
#define MII_RTL8201F_INT_ANERR  BIT11  
#define MII_RTL8201F_LED0_WOL_SEL BIT10  
#define MII_RTL8201F_WOL_INT_SEL  BIT10  

#define OMNI_MDCTL               (17)
#define OMNI_MDCTL_AUTOMDIX      BIT7
#define OMNI_INTR_SOURCE         (29)
#define OMNI_INTR_MASK           (30)
#define OMNI_INTR_WOL1           BIT10
#define OMNI_INTR_WOL2           BIT9
#define OMNI_INTR_ENERGYON       BIT7
#define OMNI_INTR_AUTONEGOCOMP   BIT6
#define OMNI_INTR_LINKDOWN       BIT4
#define OMNI_TSTCNTL             (20)
#define OMNI_TST_READ1           (21)
#define OMNI_TST_READ2           (22)
#define OMNI_WRITE_DATA          (23)
#define OMNI_TSTCNTL_READ        BIT15
#define OMNI_TSTCNTL_WRITE       BIT14
#define OMNI_TSTCNTL_WOL_REG_SEL BIT11
#define OMNI_PHY_SP_CTRL_STS     (31)

#define OMNI_WOL_MAC_15_0        0x0
#define OMNI_WOL_MAC_31_16       0x1
#define OMNI_WOL_MAC_47_32       0x2
#define OMNI_WOL_CONTROL         0x3
#define OMNI_WOL_STATUS          0x4
#define OMNI_WOL_EN              BIT0
#define OMNI_WOL_MAG_PKT_BRO_EN  BIT2
#define OMNI_WOL_MAG_PKT_DA_EN   BIT1
#define OMNI_CHK_SA_UNI_EN       BIT4

#define AVE_DDRMEM_SHIFT   (0x00000000)

#define LAN_CSBASE  (0x65000000)
#define AVE_BASE (LAN_CSBASE)
#define AVE_IRQ  (98)


#define AVE_PHY_ETHER 0x01    
#define AVE_PHY_CLINK 0x02    

#define AVE_PHY_ID        AVE_PHY_ETHER

#define AVE_PHY_MCRL           0x01
#define AVE_PHY_SMSC           0x1f
#define AVE_PHY_REALTEK        0x03
#define AVE_PHY_ATHRS         (0x04)
#define AVE_PHY_REALTEK8201FL (0x06)      
#define AVE_PHY_OMNIPHY       (0x07)
#define AVE_PHY_UNKNOWN       (0xff)

#define AVE_PHYSID_RTL8201E          (0x001CC815)
#define AVE_PHYSID_RTL8201FL         (0x001CC816)
#define AVE_PHYSID_RTL8211E_FAMILY   (0x001CC910)
#define AVE_PHYSID_OMNIPHY           (0x00000021)


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32) 
#ifndef SA_SHIRQ
#define SA_SHIRQ     (IRQF_SHARED)   
#endif

#ifndef MOD_INC_USE_COUNT
#define MOD_INC_USE_COUNT       do { } while (0)
#endif

#ifndef MOD_DEC_USE_COUNT
#define MOD_DEC_USE_COUNT       do { } while (0)
#endif

#ifndef SET_MODULE_OWNER
#define SET_MODULE_OWNER(dev)   do { } while (0)
#endif

#ifndef CHECKSUM_HW
#define CHECKSUM_HW  (CHECKSUM_PARTIAL)
#endif

#endif 

#ifdef  AVE_ENABLE_IOREMAP
extern unsigned long ave_reg_base;
#define AVE_IO_SIZE  (0x8500)
#define AVE_Rreg(value, addr)   value = readl_relaxed( (void __iomem *)((addr) + ave_reg_base) )
#define AVE_Wreg(value, addr)  writel_relaxed( (value), (void __iomem *)((addr) + ave_reg_base) )
#else 
#define AVE_IO_SIZE  (0x3000)
#define AVE_Rreg(value, addr)   value = inl((AVE_BASE + addr))
#define AVE_Wreg(value, addr)  outl(value, (AVE_BASE + addr))
#endif 

extern int ave_get_macaddr(char *mac_addr);
extern void ave_endian_change(void *data);
extern void ave_platform_init(void);
extern int ave_phy_init(struct net_device *netdev);
extern int ave_phy_intr_chkstate_negate(struct net_device *netdev);

extern void ave_util_mii_check_link(struct net_device *netdev);
extern int ave_util_mii_link_ok(struct net_device *netdev);

extern uint64_t ave_cache_control(struct net_device *netdev, uint64_t head, uint32_t length, int cflag);

extern void ave_phy_get_wol(struct net_device *netdev, struct ethtool_wolinfo *wolinfo);
extern int ave_phy_set_wol(struct net_device *netdev, struct ethtool_wolinfo *wolinfo);
extern uint32_t ave_get_wol_setting(struct net_device *netdev);
extern void ave_set_wol_setting(struct net_device *netdev, uint32_t ave_wolopts);
extern void ave_initialize_coherency_port(void);
extern int ave_panic(int param);
extern void mn_set_lateack_irq_type(int irq);

extern uint32_t ave_util_mmd_read(struct net_device *netdev, uint32_t dev_id, uint32_t offset);
#ifdef AVE_ENABLE_8023AZ_MAC_ASSIST 
extern int ave_phy_eee_cap(struct net_device *netdev, uint32_t linkspeed);
#endif 
extern int ave_phy_chk_bmcr(struct net_device *netdev, uint32_t bmcr);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,8)  
#else  
extern __be16           eth_type_trans(struct sk_buff *skb, struct net_device *netdev);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
extern struct net_device *alloc_etherdev_mq(int sizeof_priv, unsigned int queue_count);
#define alloc_etherdev(sizeof_priv) alloc_etherdev_mq(sizeof_priv, 1)
#else
extern struct net_device *alloc_etherdev(int sizeof_priv);
#endif
#endif 

extern void ave_phy_set_afe_initialize_param(struct net_device *netdev, int autoselect);
extern void ave_phy_set_afe_linkspeed_param(struct net_device *netdev, int linkspeed);
extern int ave_omniphy_linkstatus(struct net_device *netdev);
extern void ave_phy_soft_reset(struct net_device *netdev);
extern void ave_omniphy_link_status_update(struct net_device *netdev);

#if 1
extern int ave_64bit_addr_discrypt;
extern int ave_use_omniphy;
extern uint32_t AVE_DESC_SIZE;
extern uint32_t AVE_TXDM;
extern uint32_t AVE_RXDM;
extern uint32_t AVE_TXDM_SIZE;
extern uint32_t AVE_RXDM_SIZE;
#endif

#endif  
