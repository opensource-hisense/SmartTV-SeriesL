/*** CONFIDENTIAL ***/
/* Copyright (C) 2015, Socionext Inc. */
#ifndef AVE_SN_H
#define AVE_SN_H

#include "ave_sn_platform.h"

#undef AVE_DISABLE_RXFIFO_RESET
#define AVE_ENABLE_V4CODE
#undef AVE_ENABLE_TSO
#define AVE_ENABLE_CSUM_OFFLOAD

#define AVE_ENABLE_PAUSE                    

#undef AVE_DELAYED_SOFTIRQ                  
#if defined(AVE_DELAYED_SOFTIRQ)
#define AVE_RXTHREAD_BREAK                  
#define AVE_TXTHREAD_BREAK                  
#endif  

#define AVE_MACSET_BYIOCTL                  

#define AVE_USE_PHY_CUSTOM_RESET_TIMER_V4


#define AVE_DISABLE_EEE


#undef AVE_TEST_THROUGHPUT

#undef AVE_DEBUG_RXPACKET_INFO

#ifdef AVE_SINGLE_RXRING_MODE
#define AVE_MACADDR_FILTER   
#define AVE_DEF_PROMISCOUS   
#else
#undef AVE_MACADDR_FILTER    
#undef AVE_DEF_PROMISCOUS    
#endif
#undef AVE_RX_MTU_CHANGE     

#define AVE_OUTPUT_MSG       
#define AVE_DEBUG            
#undef  AVE_DEBUG_INTR

#undef  AVE_MICREL_AUTOMDIX_ON

#define AVE_STREAM_IPV4       0x00
#define AVE_STREAM_IPV6       0x01
#define AVE_STREAM_IPNO       0xff
#define AVE_MASK_ENABLE       0x01
#define AVE_MASK_DISABLE      0x00

#define AVE_STREAM_SEND_TCP   0x00000000
#define AVE_STREAM_RECV_TCP   0x00000001
#define AVE_STREAM_SEND_UDP   0x00000100
#define AVE_STREAM_RECV_UDP   0x00000101
#define AVE_STREAM_UNKNOWN    0xFFFFFFFF
#define AVE_RCV_LEVEL_HIGHEST 0x01 
#define AVE_RCV_LEVEL_HIGHER  0x02 
#define AVE_RCV_LEVEL_UNKNOWN 0xFF

#define AVE_RINGID_TX0     0x80
#define AVE_RINGID_RX0     0x00
#define AVE_RINGID_RX1     0x01
#define AVE_RINGID_RX2     0x02
#define AVE_RINGID_RX3     0x03
#define AVE_RINGID_RX4     0x04

#define AVE_NOERROR       (0)           
#define AVE_EPERM         (-EPERM)      
#define AVE_EBADFD        (-EBADFD)     
#define AVE_EINVAL        (-EINVAL)     
#define AVE_EBUSY         (-EBUSY)      
#define AVE_EAGAIN        (-EAGAIN)     
#define AVE_EPIPE         (-EPIPE)      
#define AVE_EOPNOTSUPP    (-EOPNOTSUPP) 
#define AVE_ENOBUFS       (-ENOBUFS)    

#define AVE_RESET               (1)
#define AVE_VERSION             (2)
#define AVE_GETSTATS            (3)
#define AVE_GETDINFO            (6)
#define AVE_SETWOL              (7)
#define AVE_SETSTREAM_PARAM     (8)
#define AVE_GETSTREAM_PARAM     (9)
#define AVE_DISABLESTREAM_PARAM (10)

#define AVE_IOCTL_CMD          (50)
#define AVE_REG_DIRECTREAD     (AVE_IOCTL_CMD + 0)
#define AVE_REG_DIRECTWRITE    (AVE_IOCTL_CMD + 1)
#define AVE_MEM_READ           (AVE_IOCTL_CMD + 3)
#define AVE_MEM_WRITE          (AVE_IOCTL_CMD + 4)
#define AVE_MDIO_READ          (AVE_IOCTL_CMD + 5)
#define AVE_MDIO_WRITE         (AVE_IOCTL_CMD + 6)
#define AVE_SET_DRVMODE        (AVE_IOCTL_CMD + 7)
#define AVE_GET_DRVMODE        (AVE_IOCTL_CMD + 8)
#define AVE_GET_RXINFO         (AVE_IOCTL_CMD + 9)

#define AVE_MSSR_THRESHOLD     (1000)      

#define AVE_TXDESC_NUM         (248)       
#define AVE_RX0DESC_NUM        (500)       
#define AVE_RX1DESC_NUM        (16)        
#define AVE_RX2DESC_NUM        (16)        
#define AVE_RX3DESC_NUM        (16)        
#define AVE_RX4DESC_NUM        (16)        
#define AVE_INTM_COUNT          20
#define AVE_IIRQ_RCVCNT        (230)
#define AVE_INTM_COUNT_PRIO    100
#define AVE_NOPRIO_RCVCNT      1           
#define AVE_HIGHEST_RCVCNT     128         
#define AVE_HIGHER_RCVCNT      1           
#define AVE_INATTACK_RCVCNT    1           
#define AVE_TXFREE_THRESHOLD   10
#define AVE_MNTR_TIME          (HZ/2)

#define AVE_FORCE_TXINTCNT     (6)        
#if defined(AVE_ENABLE_PHY_SOFT_RESET)
#define AVE_BMCR_RESET_CHECK_LIMIT  50
#define AVE_BMCR_RESET_CHECK_MSEC   1
#endif

#define AVE_PHY_PWRDN_BIT       (0x00000800)
#define AVE_SOCGLUE_RESET_ETH   (0x00001000)
#define AVE_SOCGLUE_CLOCK_ETH   (0x00001000)
#define AVE_SOCGLUE_RESET_reg   (0x61842000)
#define AVE_SOCGLUE_CLOCK_reg   (0x61842104)


#define AVE_PROC_FILE "ave"
#define AVE_MAJOR     (190)
#define AVE_CHDEV_NAME    "ave"

#define BIT0       (0x00000001)
#define BIT1       (0x00000002)
#define BIT2       (0x00000004)
#define BIT3       (0x00000008)
#define BIT4       (0x00000010)
#define BIT5       (0x00000020)
#define BIT6       (0x00000040)
#define BIT7       (0x00000080)
#define BIT8       (0x00000100)
#define BIT9       (0x00000200)
#define BIT10      (0x00000400)
#define BIT11      (0x00000800)
#define BIT12      (0x00001000)
#define BIT13      (0x00002000)
#define BIT14      (0x00004000)
#define BIT15      (0x00008000)
#define BIT16      (0x00010000)
#define BIT17      (0x00020000)
#define BIT18      (0x00040000)
#define BIT19      (0x00080000)
#define BIT20      (0x00100000)
#define BIT21      (0x00200000)
#define BIT22      (0x00400000)
#define BIT23      (0x00800000)
#define BIT24      (0x01000000)
#define BIT25      (0x02000000)
#define BIT26      (0x04000000)
#define BIT27      (0x08000000)
#define BIT28      (0x10000000)
#define BIT29      (0x20000000)
#define BIT30      (0x40000000)
#define BIT31      (0x80000000)

#define AVE_IDR       0x0000  
#define AVE_VR        0x0004  
#define AVE_GRR       0x0008  
#define AVE_EMCR      0x000C  
#define AVE_CLKCR     0x0018  

#define AVE_GIMR      0x0100  
#define AVE_GISR      0x0104  
#define AVE_GTICR     0x0108  

#define AVE_TXCR      0x0200  
#define AVE_RXCR      0x0204  
#define AVE_RXMAC1R   0x0208  
#define AVE_RXMAC2R   0x020C  
#define AVE_PASCR     0x0210  
#define AVE_MDIOCTR   0x0214  
#define AVE_MDIOAR    0x0218  
#define AVE_MDIOWDR   0x021C  
#define AVE_MDIOSR    0x0220  
#define AVE_MDIORDR   0x0224  
#define AVE_JSPR      0x0228  

#define AVE_AZTMR     0x022C  
#define AVE_AZSTS     0x0230  
#define AVE_AZCR      0x0234  

#define AVE_DESCC     0x0300  
#define AVE_TXDC      0x0304   
#define AVE_RXDC0     0x0308   
#define AVE_RXDC1     0x030C   
#define AVE_RXDC2     0x0310   
#define AVE_RXDC3     0x0314   
#define AVE_RXDC4     0x0318   
#define AVE_TXDCP     0x031C  
#define AVE_RXDCP0    0x0320  
#define AVE_RXDCP1    0x0324  
#define AVE_RXDCP2    0x0328  
#define AVE_RXDCP3    0x032C  
#define AVE_RXDCP4    0x0330  
#define AVE_TXWBDP    0x0334  
#define AVE_RXWBDP0   0x0338  
#define AVE_RXWBDP1   0x033C  
#define AVE_RXWBDP2   0x0340  
#define AVE_RXWBDP3   0x0344  
#define AVE_RXWBDP4   0x0348  
#define AVE_IIRQC     0x034C  
#define AVE_TXDTI0    0x0350  
#define AVE_TXDTI1    0x0354  
#define AVE_TXDTI2    0x0358  
#define AVE_TXDTI3    0x035C  
#define AVE_RXDTI0    0x0360  
#define AVE_RXDTI1    0x0364  
#define AVE_TXDI0A    0x0368  
#define AVE_TXDI1A    0x036C  
#define AVE_TXDI2A    0x0370  
#define AVE_TXDI3A    0x0374  
#define AVE_RXDI0A    0x0378  
#define AVE_RXDI1A    0x037C  

#define AVE_MSSR      0x0394  

#define AVE_BFCR      0x0400  
#define AVE_GFCR      0x0404  
#define AVE_RX0FC     0x0410  
#define AVE_RX0OVFFC  0x0414  
#define AVE_RX1FC     0x0418  
#define AVE_RX1OVFFC  0x041C  
#define AVE_RX2FC     0x0420  
#define AVE_RX2OVFFC  0x0424  
#define AVE_RX3FC     0x0428  
#define AVE_RX3OVFFC  0x042C  
#define AVE_RX4FC     0x0430  
#define AVE_RX4OVFFC  0x0434  
#define AVE_SN5FC     0x0438  
#define AVE_SN6FC     0x043C  
#define AVE_SN7FC     0x0440  

#define AVE_DESC_SIZE_64 12      

#define AVE_TXDM_64      0x1000  
#define AVE_RXDM_64      0x1C00  

#define AVE_TXDM_SIZE_64 0x0BA0  
#define AVE_RXDM_SIZE_64 0x6000  

#define AVE_DESC_SIZE_32 8       

#define AVE_TXDM_32      0x1000  
#define AVE_RXDM_32      0x1800  

#define AVE_TXDM_SIZE_32 0x07C0  
#define AVE_RXDM_SIZE_32 0x4000  


#define AVE_TXDMAX    (AVE_TXDM_SIZE_64/AVE_DESC_SIZE_64)  
#define AVE_RXDMAX    (AVE_RXDM_SIZE_64/AVE_DESC_SIZE_64)  

#define AVE_CDAVE_PERFCNFG   (0x4008)   
#define AVE_CDAVE_RC0MSKCYC  (0x41C0)   

#define AVE_SIGNAL           (0x8004)   
#define AVE_SIGNAL_H         BIT1

#define AVE_RSTCTRL          (0x8028)   
#define AVE_RSTCTRL_RMIIRST  BIT16      

#define AVE_LINKSEL          (0x8034)   
#define AVE_LINKSEL_100M     BIT0       

#define AVE_AXIRCTL    (0x8410)
#define AVE_AXIWCTL    (0x8414)

#define AVE_ZERO         (0x00000000)

#define AVE_GRR_DMACRST   BIT8
#define AVE_GRR_RXFFR     BIT5
#define AVE_GRR_PHYRST    BIT4
#define AVE_GRR_MDIOBRST  BIT3
#define AVE_GRR_TXBRST    BIT2
#define AVE_GRR_RXBRST    BIT1
#define AVE_GRR_GRST      BIT0

#define AVE_GI_GT2        BIT27 
#define AVE_GI_GT1        BIT26 
#define AVE_GI_DMAC       BIT25
#define AVE_GI_PHY        BIT24
#define AVE_GI_PKTBOVF    BIT23 
#define AVE_GI_TXBOVF     BIT22 
#define AVE_GI_TXFOVF     BIT21 
#define AVE_GI_TX         BIT16 
#define AVE_GI_SWFFOVF    BIT15 
#define AVE_GI_RXERR      BIT8  
#define AVE_GI_RXOVF      BIT7  
#define AVE_GI_RXDROP     BIT6  
#define AVE_GI_RXIINT     BIT5  
#define AVE_GI_RXINT4     BIT4  
#define AVE_GI_RXINT3     BIT3  
#define AVE_GI_RXINT2     BIT2  
#define AVE_GI_RXINT1     BIT1  
#define AVE_GI_RXINT0     BIT0  
#define AVE_GI_NORMALRX   (BIT0)
#define AVE_GI_RX         ( AVE_GI_RXIINT | AVE_GI_RXINT4 )

#define AVE_GT2_CNTEN     BIT31
#define AVE_GT1_CNTEN     BIT15

#define AVE_TXCR_LBK      BIT29
#define AVE_TXWFLOWH      BIT21
#define AVE_TXCR_FLOCTR   BIT18

#define AVE_TXCR_TXSPD     (BIT17 | BIT16)
#define AVE_TXCR_TXSPD_1G  (BIT17)
#define AVE_TXCR_TXSPD_100 (BIT16)
#define AVE_TXCR_TXSPD_10  (0)

#define AVE_RXCR_RXEN     BIT30
#define AVE_RXCR_FDUPEN   BIT22
#define AVE_RXCR_FLOCTR   BIT21
#define AVE_RXCR_FLUSHON  BIT20
#define AVE_RXCR_AFEN     BIT19
#define AVE_RXCR_DRPEN    BIT18  

#define AVE_MDIOCTR_RREQ  BIT3
#define AVE_MDIOCTR_WREQ  BIT2

#define AVE_MDIOSR_STS    BIT0

#define AVE_MSSR_DEFAULT   (1460)

#define AVE_AZCR_AZEN       BIT0
#define AVE_AZCR_PHYTXPSEN  BIT1
#define AVE_AZCR_MACTXPSEN  BIT2
#define AVE_AZCR_MACRXPSEN  BIT3

#define AVE_DESCC_TD      BIT0
#define AVE_DESCC_RDSTP   BIT4
#define AVE_DESCC_RD0     BIT8
#define AVE_DESCC_TDS     BIT16  

#define AVE_DESCC_CTRL_MASK  (0x0000FFFF)

#define AVE_TXDC_TXDSA_MASK  (0x000007FF)
#define AVE_TXDC_TXDSIZ_MASK (0x0FFF0000)

#define AVE_TXDC_RXDSA_MASK  (0x00001FFF)
#define AVE_TXDC_RXDSIZ_MASK (0x1FFF0000)

#define AVE_IIRQC_EN4     BIT31
#define AVE_IIRQC_EN3     BIT30
#define AVE_IIRQC_EN2     BIT29
#define AVE_IIRQC_EN1     BIT28
#define AVE_IIRQC_EN0     BIT27

#define AVE_PFSEL_RING0   (0x00)
#define AVE_PFSEL_RING1   (0x01)
#define AVE_PFSEL_RING2   (0x02)
#define AVE_PFSEL_RING3   (0x03)
#define AVE_PFSEL_RING4   (0x04)
#define AVE_PFSEL_DROP5   (0x05) 
#define AVE_PFSEL_DROP6   (0x06) 
#define AVE_PFSEL_DROP7   (0x07) 
#define AVE_PFSEL_WM      (0x80000000)

#define AVE_PFMBYTE0_MASK (0xFFCC003F)
#define AVE_PFMBYTE1_MASK (0x03FFFFFF)

#define AVE_CLKCR_TXCLKEN BIT2 

#define AVE_STS_OWN       BIT31
#define AVE_STS_TSO       BIT30      
#define AVE_STS_INTR      BIT29
#define AVE_STS_OK        BIT27
#define AVE_STS_1ST       BIT26      
#define AVE_STS_LAST      BIT25      
#define AVE_STS_DEST      (BIT24|BIT23)

#define AVE_STS_OWC       BIT21 
#define AVE_STS_EC        BIT20 
#define AVE_STS_CCNT      (BIT19|BIT18|BIT17|BIT16) 
#define AVE_STS_VLAN      BIT16 

#define AVE_STS_PKTLEN      (0x000007FF)
#define AVE_STS_PKTLEN_TSO  (0x0000FFFF)    

#define AVE_AXIXCTL_ACP   BIT8
#define AVE_AXIXCTL_WA    BIT3
#define AVE_AXIXCTL_RA    BIT2
#define AVE_AXIXCTL_C     BIT1
#define AVE_AXIXCTL_B     BIT0

#define AVE_CFGR      0x000C  

#define AVE_PKTF_BASE      0x0800  
#define AVE_PFNM0003       0x0800  
#define AVE_PFNM0407       0x0804  
#define AVE_PFNM0C0F       0x080C  
#define AVE_PFNM1013       0x0810  
#define AVE_PFNM1417       0x0814  
#define AVE_PFNM181B       0x0818  
#define AVE_PFNM1C1F       0x081C  
#define AVE_PFNM2023       0x0820  
#define AVE_PFNM2427       0x0824  
#define AVE_PFNM282B       0x0828  
#define AVE_PFNM2C2F       0x082C  
#define AVE_PFNM3033       0x0830  
#define AVE_PFNM3437       0x0834  
#define AVE_PFNM383B       0x0838  
#define AVE_PFMBYTE_BASE   0x0D00  
#define AVE_PFMBIT_BASE    0x0E00  
#define AVE_PFSEL_BASE     0x0F00  
#define AVE_PFEN           0x0FFC  

#define AVE_STS_NOCSUM     BIT28
#define AVE_STS_CSSV       BIT21
#define AVE_STS_CSER       BIT20
#define AVE_STS_IPER       BIT19
#define AVE_STS_UDPE       BIT18
#define AVE_STS_TCPE       BIT17

#define AVE_CFGR_FLE       BIT31 
#define AVE_CFGR_CHE       BIT30 
#define AVE_CFGR_TOFE      BIT29 
#define AVE_CFGR_ROFE      BIT28 
#define AVE_CFGR_MII       BIT27 
#define AVE_CFGR_IPFCEN    BIT24 

#define ADVERTISE_PAUSE_CAP 0x0400

#define AVE_ID            "AVE3"
#define AVEV4_ID          "AVE4"

#define AVE_CMDSTS_OFFSET    (0)
#define AVE_BUFPTR_OFFSET    (4)

#define AVE_PWR_NORMAL     (0)
#define AVE_PWR_LOW        (3)

#ifdef AVE_ENABLE_TXTIMINGCTL
#define ETHER_IOC_AVE_CLKCR (SIOCDEVPRIVATE+0x0C) 
#endif

#define AVE_MDIO_MAXLOOP (100)   

#define AVE_IF_NAME   "eth%d"

#ifndef AVE_MACADDR_FILTER
#define AVE_MULTICAST_SIZE   (7) 
#else
#define AVE_MULTICAST_SIZE   (5) 
#endif

#define AVE_PF_RESERVE_SIZE      (10)    
#define AVE_PF_STREAM_SIZE       (4)     
#define AVE_DEFAULT_FILTER       (17)    
#define AVE_DEFAULT_UNICAST      (8)     
#define AVE_DEFAULT_BROADCAST    (9)     
#define AVE_DEFAULT_STREAM_ACK   (2)     

#define AVE_FILTER_STOP      (0)
#define AVE_FILTER_START     (1)
#define AVE_FILTER_CTR_OFF   (0)
#define AVE_FILTER_CTR_ON    (1)
#define AVE_FILTER_WOL_OFF   (0)
#define AVE_FILTER_WOL_ON    (1)

#define AVE_MAX_ETHFRAME (1518)

#ifdef  AVE_SINGLE_RXRING_MODE
#define AVE_RXRING_NUM   (1) 
#define AVE_DMATRX_INIT  {AVE_TXDESC_NUM,{AVE_RX0DESC_NUM}}
#else
#define AVE_RXRING_NUM   (5) 
#define AVE_DMATRX_INIT {                          \
                            AVE_TXDESC_NUM,        \
                            {                      \
                                AVE_RX0DESC_NUM,   \
                                AVE_RX1DESC_NUM,   \
                                AVE_RX2DESC_NUM,   \
                                AVE_RX3DESC_NUM,   \
                                AVE_RX4DESC_NUM    \
                            }                      \
                        }
#endif

#define AVE_DESC_STOP    (0)
#define AVE_DESC_START   (1)
#define AVE_DESC_RX_SUSPEND  (2)
#define AVE_DESC_RX_PERMIT   (3)

#define AVE_LS_UP        (1)
#define AVE_LS_DOWN      (2)
#define AVE_LS_NOTHING   (0)

#define AVE_NULL_PTR          ((void*)0)

#ifdef  AVE_OUTPUT_MSG
#define DBG_PRINT(args...) printk("ave: " args)
#else
#define DBG_PRINT(args...)
#endif

#define AVE_DISABLE_INT() AVE_Wreg(0, AVE_GIMR)
#define AVE_ENABLE_MININT()   AVE_Wreg((AVE_GI_RX | \
                        AVE_GI_TX |  \
                        AVE_GI_PHY), AVE_GIMR)

struct AVE_version{
    unsigned char id[4];
    unsigned char ch[4];
};

struct AVE_getstats{
    uint32_t badframes;
    uint32_t goodframes;
    uint32_t rx0frames;
    uint32_t rx1frames;
    uint32_t rx2frames;
    uint32_t rx3frames;
    uint32_t rx4frames;
    uint32_t rx0dropframes;
    uint32_t rx1dropframes;
    uint32_t rx2dropframes;
    uint32_t rx3dropframes;
    uint32_t rx4dropframes;
    uint32_t discardframes;
};

struct AVE_tx_desc_format {
    uint32_t own:1,       
             RSV1:1,      
             intr:1,      
             nocsum:1,    
             ok:1,        
             RSV2:2,
             dest:2,      
             RSV3:1,
             owc:1,       
             ec:1,        
             ccnt:4,      
             RSV4:5,
             size:11;     
    uint32_t bufptr;
};

struct AVE_rx_desc_format {
    uint32_t own:1,       
             RSV1:1,      
             intr:1,      
             RSV2:1,
             ok:1,        
             RSV3:2,
             dest:2,      
             RSV4:1,
             cssv:1,
             cser:1,
             iper:1,
             udper:1,
             tcper:1,
             vlan:1,      
             RSV5:5,
             size:11;     
    uint32_t bufptr;
};

struct AVE_descriptor_info {
    unsigned char  ring_id;
    unsigned short desc_id;
    union {
        struct AVE_tx_desc_format tx;
        struct AVE_rx_desc_format rx;
    } entity;
};

struct AVE_pattern_param {
    unsigned char pattern[58];        
    unsigned char mask[58];           
};

struct AVE_stream_param {
    long           connhandle;        
    uint32_t       mode;              
    unsigned char  rcv_prio;          
    unsigned char  rsv1;              
    unsigned char  rsv2;              
    unsigned char  ipver;             

    unsigned short ipoptlen;          
    unsigned char  ipoptlen_mask;     

    unsigned char  mymac[8];          
    unsigned char  mymac_mask;        

    unsigned char  protocol;          
    unsigned char  protocol_mask;     

    unsigned char  peerip[16];        
    unsigned char  peerip_mask;       

    unsigned char  myip[16];          
    unsigned char  myip_mask;         

    unsigned short peerport;          
    unsigned char  peerport_mask;     

    unsigned short myport;            
    unsigned char  myport_mask;       

    unsigned char  ring_id;           

};

#ifdef AVE_DEBUG_RXPACKET_INFO

#define AVE_RXINF_DBG_TBLMAX  200
struct AVE_rxsts_info{
    int            ring;
    unsigned long  jiffies;
    int            pktlen;
    int            proc_idx;
    unsigned short ih_id;
    uint32_t       hwtimer;
};

struct AVE_getrxinfo{
    uint32_t size;
    struct AVE_rxsts_info rxinfs[AVE_RXINF_DBG_TBLMAX];
};
#endif 

struct AVE_CONTROL {
    long cmd;
    union {
        unsigned char head;

        struct AVE_version         version;
        struct AVE_getstats        getstats;
        struct AVE_descriptor_info desc_inf;
        struct AVE_stream_param    stream_param;
        struct AVE_pattern_param   pattern_param;
        long                       connhandle;

#ifdef AVE_DEBUG_RXPACKET_INFO
        struct AVE_getrxinfo       getrxinfo;
#endif 
    }ave_param;
};

struct ave_private {
    spinlock_t    lock;       
    int           macaddr_got;
    struct net_device_stats   stats;
    char          id_str[8];
    union {
        uint32_t hex;
        char str[4];
    } ave_vr;
    int            phy_id;     
    unsigned short partner;    
    struct mii_if_info    mii_if;     
    uint32_t       msg_enable; 
    unsigned short flags;      
    uint32_t       offset;     
    int            iintcnt;    

    uint32_t       tx_dnum;    
    uint32_t       tx_proc_idx, tx_done_idx;
    struct sk_buff *tx_skbs[AVE_TXDMAX];
#if 1
    uint64_t tx_skbs_dma[AVE_TXDMAX];
    uint32_t tx_skbs_dmalen[AVE_TXDMAX];
#endif
    char           dummy[16];  
    unsigned char  tx_short[AVE_TXDMAX][96];

    uint32_t       rx_dnum[AVE_RXRING_NUM];
    uint32_t       rx_proc_idx[AVE_RXRING_NUM], rx_done_idx[AVE_RXRING_NUM];
    uint32_t       rx_dsa[AVE_RXRING_NUM]; 
    struct sk_buff *rx_skbs[AVE_RXRING_NUM][AVE_RXDMAX];
#if 1
    uint64_t rx_skbs_dma[AVE_RXRING_NUM][AVE_RXDMAX];
    uint32_t rx_skbs_dmalen[AVE_RXRING_NUM][AVE_RXDMAX];
#endif

    uint32_t       phy_sid;    

    struct timer_list timer;   
#ifdef AVE_HIBERNATION
    void           *saved_state;
#endif

};

struct ave_descriptor_matrix {
    uint32_t tx;
    uint32_t rx[AVE_RXRING_NUM];
};

struct AVE_proc_info{
    char tag[16];
    unsigned int value;
};

struct ave_filter_info{
    uint32_t stream_mode;              
    unsigned char  rcv_prio;           
    unsigned short ipoptlen;           
    unsigned char  ipoptlen_mask;
    unsigned short myport_fec1;        
    unsigned char  myport_fec1_mask;
    unsigned short myport_fec2;        
    unsigned char  myport_fec2_mask;
};

struct ave_pfsel_proto_param{
    unsigned char  ipver;              

    unsigned char  mymac[8];           
    unsigned char  mymac_mask;         

    unsigned char  protocol;           
    unsigned char  protocol_mask;      

    unsigned char  peerip[16];         
    unsigned char  peerip_mask;        

    unsigned char  myip[16];           
    unsigned char  myip_mask;          

    unsigned short peerport;           
    unsigned char  peerport_mask;      

    unsigned short myport;             
    unsigned char  myport_mask;        
};

struct ave_save_data{
    uint32_t wolsetting;
    uint32_t bmcr;
    uint32_t gimr;
    uint32_t gtcr;
    uint32_t txcr;
    uint32_t rxcr;
    uint32_t rxmac1r;
    uint32_t rxmac2r;
    uint32_t pfnm0003[18];
    uint32_t pfnm0407[18];
    uint32_t pfnm0c0f[9];
    uint32_t pfnm1013[8];
    uint32_t pfnm1417[8];
    uint32_t pfnm181b[8];
    uint32_t pfnm1c1f[8];
    uint32_t pfnm2023[8];
    uint32_t pfnm2427[8];
    uint32_t pfnm282b[8];
    uint32_t pfnm2c2f[8];
    uint32_t pfnm3033[8];
    uint32_t pfnm3437[8];
    uint32_t pfnm383b[8];
    uint32_t pfnmbyte0[18];
    uint32_t pfnmbyte1[8];
    uint32_t pfnmbit[8];
    uint32_t pfnmsel[19];
    uint32_t pfen;
    uint32_t linksel;
    uint32_t iirqc;
    uint32_t mssr;
};

void ave_proc_add(char *msg);
int  ave_api_mdio_read(struct net_device *netdev, int phyid, int location);
void ave_api_mdio_write (struct net_device *netdev, int phyid, int location, uint32_t value);

#endif  
