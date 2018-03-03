/*** CONFIDENTIAL ***/
/* Copyright (C) 2015, Socionext Inc. */
#include "ave_sn.h"

#include <linux/random.h>
#if defined(AVE_RXTHREAD_BREAK) || defined(AVE_TXTHREAD_BREAK)
#include <linux/kthread.h>
#endif  

#ifdef AVE_ENABLE_PF_IF
#include <linux/device.h>
#include <linux/io.h>
#include <linux/of_platform.h>
#include <linux/of_net.h>

struct ave_pf_data {
	struct device		*dev;
	void __iomem		*base;
	unsigned int		irq;
};
#endif


struct ave_filter_info ave_fltinf_g[AVE_PF_STREAM_SIZE];
struct semaphore ave_sem_ioctl;

#define AVE_CLKCR_OFF (0)
#if defined(AVE_RXTHREAD_BREAK)
#define AVE_MIN_RX  8
#endif  
#if defined(AVE_TXTHREAD_BREAK)
#define AVE_MIN_TX  8
static int ave_tx_thread_break_cnt[NR_CPUS]={0};
static int ave_tx_thread_break_pid[NR_CPUS]={0};
static volatile int ave_tx_thread_break_flg = 0;
static struct task_struct *ave_thread = NULL;
static DECLARE_WAIT_QUEUE_HEAD(ave_wq);
#endif  

static volatile int ave_initial_global_reset_completed = 0; 

static const char *drv_version =
    "ave drv: $Revision: 4.00 \n";

static int debug = -1;    

#ifdef AVE_ENABLE_IOREMAP
unsigned long ave_reg_base;
#endif 

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
uint32_t ave_flg_reset = 0;  
#endif 

int ave_phy_reset_period    = 20 * 1000;  
int ave_phy_reset_stability = 40 * 1000;  
int ave_phy_reset_clock_stability = 35 * 1000; 

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP
#define AVE_WAIT_FOR_PHY_RESET(usec) ((usec) < 1000) ? udelay((usec)) : msleep((usec)/1000)
#define AVE_WAIT_FOR_AVE_RESET(msec) (msleep(msec))
#else 
#define AVE_WAIT_FOR_PHY_RESET(usec) ((usec) < 1000) ? udelay((usec)) : mdelay((usec)/1000)
#define AVE_WAIT_FOR_AVE_RESET(msec) (mdelay(msec))
#endif 

extern int ave_poll_link;                

extern uint32_t ave_ksz8051_bugfix;

extern int ave_miimode;    
extern int ave_gbit_mode;  

static int ave_base = LAN_CSBASE;                  
#ifndef AVE_ENABLE_PF_IF
static int ave_irq  = AVE_IRQ;                  
#endif

#ifdef MODULE
#ifndef AVE_ENABLE_PF_IF
MODULE_ALIAS("platform:ave");
MODULE_AUTHOR("Socionext Inc.");
MODULE_DESCRIPTION("AV Ether Driver");
MODULE_LICENSE("GPL v2");
#endif

module_param(debug, int, 0);
MODULE_PARM_DESC(debug, "debug level (0-6)");
#endif

struct net_device *ave_dev = NULL;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32) 
static struct net_device_ops ave_netdev_ops;
#endif 

struct AVE_proc_info ave_info[] =
{
    {"RxFIFO OVF   ", 0},   

    {"TCP ChksumErr", 0},
    {"UDP ChksumErr", 0},
    {"IP  ChksumErr", 0},
    {"VLAN Tag     ", 0},
    {"Frame Bad    ", 0},   
    {"Frame Good   ", 0},   

    {"Tx Bytes     ", 0},
    {"Rx Bytes     ", 0},
    {"Rx0 recv     ", 0},   
    {"Rx1 recv     ", 0},   
    {"Rx2 recv     ", 0},   
    {"Rx3 recv     ", 0},   
    {"Rx4 recv     ", 0},   
    {"Rx0 drop     ", 0},   
    {"Rx1 drop     ", 0},   
    {"Rx2 drop     ", 0},   
    {"Rx3 drop     ", 0},   
    {"Rx4 drop     ", 0},   
    {"Discard recv ", 0},   
    {""             , 0}        
};

#define RUN_AT(x) (jiffies + (x))

#if 0
#define DEBUG_DEFAULT   (NETIF_MSG_DRV | NETIF_MSG_HW | NETIF_MSG_RX_ERR | \
             NETIF_MSG_TX_ERR)
#endif

#define DEBUG_DEFAULT   (0)     

static void ave_wait_phy_ready(void);
int ave_tx_completion(struct net_device *netdev);
void ave_rxf_reset(struct net_device *netdev);

int ave_set_descriptor_num(struct ave_descriptor_matrix *desc_mtrx)
{
    struct ave_private *ave_priv = netdev_priv(ave_dev);
    int ring_num;

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_set_descriptor_num() tx=%u\n", desc_mtrx->tx);
        for ( ring_num = 0; ring_num < AVE_RXRING_NUM; ring_num++ ) {
            DBG_PRINT("                         rx[%d]=%u\n", ring_num, desc_mtrx->rx[ring_num]);
        }
    }

    if ( netif_running(ave_dev) ){
        return AVE_EPERM;
    }

    if( desc_mtrx->tx < 2 ){
        printk(KERN_ERR "ave: invalid parameter for tx=%u\n", desc_mtrx->tx);
        return AVE_EINVAL;
    }
    for ( ring_num = 0; ring_num < AVE_RXRING_NUM; ring_num++ ) {
        if( desc_mtrx->rx[ring_num] < 2 ){
            printk(KERN_ERR "ave: invalid parameter for rx[%d]=%u\n", ring_num, desc_mtrx->rx[ring_num]);
            return AVE_EINVAL;
        }
    }

    ave_priv->tx_dnum = desc_mtrx->tx;
    for ( ring_num = 0; ring_num < AVE_RXRING_NUM; ring_num++ ) {
        ave_priv->rx_dnum[ring_num] = desc_mtrx->rx[ring_num];
    }

    return 0;
}

int ave_get_descriptor_num(struct ave_descriptor_matrix *desc_mtrx)
{
    struct ave_private *ave_priv = netdev_priv(ave_dev);
    int ring_num;

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_get_descriptor_num()\n");
    }

    desc_mtrx->tx = ave_priv->tx_dnum;
    for ( ring_num = 0; ring_num < AVE_RXRING_NUM; ring_num++ ) {
        desc_mtrx->rx[ring_num] = ave_priv->rx_dnum[ring_num];
    }

    return 0;
}

int ave_get_statistics_info(struct AVE_getstats  *stats)
{
    uint32_t reg_val;

    if ( !netif_running(ave_dev) ){
        return AVE_EPERM;
    }

    AVE_Rreg(stats->badframes,AVE_BFCR);
    AVE_Rreg(stats->goodframes,AVE_GFCR);
    AVE_Rreg(stats->rx0frames,AVE_RX0FC);
    AVE_Rreg(stats->rx1frames,AVE_RX1FC);
    AVE_Rreg(stats->rx2frames,AVE_RX2FC);
    AVE_Rreg(stats->rx3frames,AVE_RX3FC);
    AVE_Rreg(stats->rx4frames,AVE_RX4FC);
    AVE_Rreg(stats->rx0dropframes,AVE_RX0OVFFC);
    AVE_Rreg(stats->rx1dropframes,AVE_RX1OVFFC);
    AVE_Rreg(stats->rx2dropframes,AVE_RX2OVFFC);
    AVE_Rreg(stats->rx3dropframes,AVE_RX3OVFFC);
    AVE_Rreg(stats->rx4dropframes,AVE_RX4OVFFC);
    AVE_Rreg(reg_val,AVE_SN5FC);
    stats->discardframes = reg_val;
    AVE_Rreg(reg_val,AVE_SN6FC);
    stats->discardframes += reg_val;
    AVE_Rreg(reg_val,AVE_SN7FC);
    stats->discardframes += reg_val;

    return 0;
}

int ave_pfsel_switch(struct net_device *netdev, char entry_no, int fflag)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val;

    if ( netdev != 0 ) {
        ave_priv = netdev_priv(netdev);
        if ( netif_msg_drv(ave_priv) ) {
            DBG_PRINT("ave_pfsel_switch() entry=%d, flag=%d\n", entry_no, fflag);
        }
    }

    if ( entry_no > 17 ) { 
        printk(KERN_ERR "ave: Bad filter entry number.(%d)\n", entry_no);
        return AVE_EINVAL;
    }

    AVE_Rreg(reg_val, AVE_PFEN);
    if ( fflag == AVE_FILTER_STOP ) {
        reg_val &= ~(1 << entry_no);
    } else {
        reg_val |= (1 << entry_no);
    }
    AVE_Wreg(reg_val, AVE_PFEN);

    return 0;
}

int ave_pfsel_status(struct net_device *netdev, char entry_no)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val;
    int           ret_val=0;

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_pfsel_status() entry=%d\n", entry_no);
    }

    if ( entry_no > 17 ) { 
        printk(KERN_ERR "ave: Bad filter entry number.(%d)\n", entry_no);
        return AVE_EINVAL;
    }

    AVE_Rreg(reg_val, AVE_PFEN);
    if( (reg_val >> entry_no) & 0x00000001 ){
        ret_val = AVE_FILTER_CTR_ON;
    } else {
        ret_val = AVE_FILTER_CTR_OFF;
    }

    return ret_val;
}

int ave_pfsel_macaddr_set(struct net_device *netdev,
               char entry_no, unsigned char *mac_addr,
               unsigned int set_size, uint32_t trans_dest)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val0, reg_val1=0;
    unsigned long reg_adr;

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_pfsel_macaddr_set() entry=%d," \
            "macaddr=%02x%02x%02x%02x%02x%02x, size=%d  trans_dest=%02x\n", entry_no,
            mac_addr[0], mac_addr[1], mac_addr[2],
            mac_addr[3], mac_addr[4], mac_addr[5], set_size, trans_dest);
    }
    ave_proc_add("ave_pfsel_macaddr_set");

    if ( entry_no > 18 ) { 
        printk(KERN_ERR "ave: Bad filter entry number.(%d)\n", entry_no);
        return AVE_EINVAL;
    }

    ave_pfsel_switch(netdev, entry_no, AVE_FILTER_STOP);

#ifndef AVE_MACHINE_BIGENDIAN
    reg_adr = AVE_PKTF_BASE + entry_no * 0x40;
    reg_val0 = (uint32_t)(*mac_addr) | ((uint32_t)(*(mac_addr + 1)) << 8) |
           ((uint32_t)(*(mac_addr + 2)) << 16) |
           ((uint32_t)(*(mac_addr + 3)) << 24);
    reg_val1 = (uint32_t)(*(mac_addr + 4)) |
           (uint32_t)((*(mac_addr + 5)) << 8);
    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("*(%08lx)=%08x\n", reg_adr, reg_val0);
        DBG_PRINT("*(%08lx)=%08x\n", reg_adr + 0x0004, reg_val1);
    }
    AVE_Wreg(reg_val0, reg_adr);
    AVE_Wreg(reg_val1, reg_adr+0x0004);
#else
#endif

    reg_adr = AVE_PFMBYTE_BASE + entry_no * 0x08;
    if( set_size>=6 ){
        reg_val0 = 0xFFFFFF00;
    } else if( set_size==5 ){
        reg_val0 = 0xFFFFFF20;
    } else if( set_size==4 ){
        reg_val0 = 0xFFFFFF30;
    } else if( set_size==3 ){
        reg_val0 = 0xFFFFFF38;
    } else if( set_size==2 ){
        reg_val0 = 0xFFFFFF3C;
    } else if( set_size==1 ){
        reg_val0 = 0xFFFFFF3E;
    } else if( set_size==0 ){
        reg_val0 = 0xFFFFFF3F; 
    }

    reg_val1 = 0xFFFFFFFF;
    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("Byte Mask Set : *(%08lx)=%08x\n", reg_adr, reg_val0);
        DBG_PRINT("Byte Mask Set : *(%08lx)=%08x\n", reg_adr + 0x0004, reg_val1);
    }
    AVE_Wreg(reg_val0, reg_adr);
    AVE_Wreg(reg_val1, reg_adr+0x0004);

    reg_adr = AVE_PFMBIT_BASE + entry_no * 0x04;
    reg_val0 = 0x0000FFFF; 
    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("Bit Mask Set : *(%08lx)=%08x\n", reg_adr, reg_val0);
    }
    AVE_Wreg(reg_val0, reg_adr);

    reg_adr = AVE_PFSEL_BASE + entry_no * 0x04;
    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("Selector Set : *(%08lx)=%08x\n", reg_adr, trans_dest);
    }
    AVE_Wreg(trans_dest, reg_adr);

    ave_pfsel_switch(netdev, entry_no, AVE_FILTER_START);

    return 0;
}

int ave_pfsel_promisc_set(struct net_device *netdev,
               char entry_no, uint32_t trans_dest)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val=0;
    unsigned long reg_adr;

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_pfsel_promisc_set() entry=%d, trans_dest=%02x\n",
                   entry_no,trans_dest );
    }
    ave_proc_add("ave_pfsel_promisc_set");

    if ( entry_no > 18 ) { 
        printk(KERN_ERR "ave: Bad filter entry number.(%d)\n", entry_no);
        return AVE_EINVAL;
    }

    ave_pfsel_switch(netdev, entry_no, AVE_FILTER_STOP);

    reg_adr = AVE_PFMBYTE_BASE + entry_no * 0x08;
    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_pfsel_promisc_set: all to host\n");
        DBG_PRINT("*(%08lx)=0xFFFFFFFF\n", reg_adr);
        DBG_PRINT("*(%08lx)=0xFFFFFFFF\n", reg_adr + 0x0004);
    }
    AVE_Wreg(0xFFFFFFFF, reg_adr);
    AVE_Wreg(0xFFFFFFFF, reg_adr+0x0004);

    reg_adr = AVE_PFMBIT_BASE + entry_no * 0x04;
    reg_val = 0x0000FFFF; 
    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("Bit Mask Set : *(%08lx)=%08x\n", reg_adr, reg_val);
    }
    AVE_Wreg(reg_val, reg_adr);

    reg_adr = AVE_PFSEL_BASE + entry_no * 0x04;
    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("*(%08lx)=%08x\n", reg_adr, trans_dest);
    }
    AVE_Wreg(trans_dest, reg_adr);

    ave_pfsel_switch(netdev, entry_no, AVE_FILTER_START);

    return 0;
}

int ave_pfsel_init(struct net_device *netdev)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    int   size;
#ifndef AVE_MACADDR_FILTER
    unsigned char broadcast_macadr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
#endif  

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_pfsel_init()\n");
    }
    ave_proc_add("ave_pfsel_init");

    for ( size = 0; size < AVE_MULTICAST_SIZE + AVE_PF_RESERVE_SIZE; size++ ) {
        ave_pfsel_switch(netdev, size, AVE_FILTER_STOP);
    }

#ifdef  AVE_DEF_PROMISCOUS
    ave_pfsel_promisc_set(netdev, AVE_DEFAULT_FILTER, AVE_PFSEL_RING0);
#else
    ave_pfsel_switch(netdev, AVE_DEFAULT_FILTER, AVE_FILTER_STOP);
#endif

#ifndef AVE_MACADDR_FILTER
    ave_pfsel_macaddr_set(netdev, AVE_DEFAULT_UNICAST, netdev->dev_addr, 6,
               AVE_PFSEL_RING0);

    ave_pfsel_macaddr_set(netdev, AVE_DEFAULT_BROADCAST, broadcast_macadr, 6,
               AVE_PFSEL_RING0);
#endif

    return 0;
}

struct ave_save_data ave_pf_dat;

void ave_pf_save_unlock(void)
{
    int i;

    AVE_Rreg(ave_pf_dat.pfen, AVE_PFEN);
    AVE_Wreg(0, AVE_PFEN);

    for(i=0; i<18; ++i){
        AVE_Rreg(ave_pf_dat.pfnm0003[i], AVE_PFNM0003+0x40*i);
    }

    for(i=0; i<18; ++i){
        AVE_Rreg(ave_pf_dat.pfnm0407[i], AVE_PFNM0407+0x40*i);
    }

    for(i=0; i<8; ++i){
        AVE_Rreg(ave_pf_dat.pfnm0c0f[i], AVE_PFNM0C0F+0x40*i);
    }
    AVE_Rreg(ave_pf_dat.pfnm0c0f[8], AVE_PFNM0C0F+0x440);

    for(i=0; i<7; ++i){
        AVE_Rreg(ave_pf_dat.pfnm1013[i], AVE_PFNM1013+0x40*i);
    }
    AVE_Rreg(ave_pf_dat.pfnm1013[7], AVE_PFNM1013+0x440);

    for(i=0; i<7; ++i){
        AVE_Rreg(ave_pf_dat.pfnm1417[i], AVE_PFNM1417+0x40*i);
    }
    AVE_Rreg(ave_pf_dat.pfnm1417[7], AVE_PFNM1417+0x440);

    for(i=0; i<7; ++i){
        AVE_Rreg(ave_pf_dat.pfnm181b[i], AVE_PFNM181B+0x40*i);
    }
    AVE_Rreg(ave_pf_dat.pfnm181b[7], AVE_PFNM181B+0x440);

    for(i=0; i<7; ++i){
        AVE_Rreg(ave_pf_dat.pfnm1c1f[i], AVE_PFNM1C1F+0x40*i);
    }
    AVE_Rreg(ave_pf_dat.pfnm1c1f[7], AVE_PFNM1C1F+0x440);

    for(i=0; i<7; ++i){
        AVE_Rreg(ave_pf_dat.pfnm2023[i], AVE_PFNM2023+0x40*i);
    }
    AVE_Rreg(ave_pf_dat.pfnm2023[7], AVE_PFNM2023+0x440);
    for(i=0; i<7; ++i){
        AVE_Rreg(ave_pf_dat.pfnm2427[i], AVE_PFNM2427+0x40*i);
    }
    AVE_Rreg(ave_pf_dat.pfnm2427[7], AVE_PFNM2427+0x440);

    for(i=0; i<7; ++i){
        AVE_Rreg(ave_pf_dat.pfnm282b[i], AVE_PFNM282B+0x40*i);
    }
    AVE_Rreg(ave_pf_dat.pfnm282b[7], AVE_PFNM282B+0x440);

    for(i=0; i<7; ++i){
        AVE_Rreg(ave_pf_dat.pfnm2c2f[i], AVE_PFNM2C2F+0x40*i);
    }
    AVE_Rreg(ave_pf_dat.pfnm2c2f[7], AVE_PFNM2C2F+0x440);

    for(i=0; i<7; ++i){
        AVE_Rreg(ave_pf_dat.pfnm3033[i], AVE_PFNM3033+0x40*i);
    }
    AVE_Rreg(ave_pf_dat.pfnm3033[7], AVE_PFNM3033+0x440);

    for(i=0; i<7; ++i){
        AVE_Rreg(ave_pf_dat.pfnm3437[i], AVE_PFNM3437+0x40*i);
    }
    AVE_Rreg(ave_pf_dat.pfnm3437[7], AVE_PFNM3437+0x440);

    for(i=0; i<7; ++i){
        AVE_Rreg(ave_pf_dat.pfnm383b[i], AVE_PFNM383B+0x40*i);
    }
    AVE_Rreg(ave_pf_dat.pfnm383b[7], AVE_PFNM383B+0x440);

    for(i=0; i<18; ++i){
        AVE_Rreg(ave_pf_dat.pfnmbyte0[i], AVE_PFMBYTE_BASE+0x8*i);
    }

    for(i=0; i<7; ++i){
        AVE_Rreg(ave_pf_dat.pfnmbyte1[i], AVE_PFMBYTE_BASE+0x4+0x8*i);
    }
    AVE_Rreg(ave_pf_dat.pfnmbyte1[7], AVE_PFMBYTE_BASE+0x8C);

    for(i=0; i<7; ++i){
        AVE_Rreg(ave_pf_dat.pfnmbit[i], AVE_PFMBIT_BASE+0x4*i);
    }
    AVE_Rreg(ave_pf_dat.pfnmbit[7], AVE_PFMBIT_BASE+0x44);

    for(i=0; i<19; ++i){
        AVE_Rreg(ave_pf_dat.pfnmsel[i], AVE_PFSEL_BASE+0x4*i);
    }

    return;
}

void ave_pf_restore_unlock(void)
{
    int i;

    AVE_Wreg(0, AVE_PFEN);

    for(i=0; i<18; ++i){
        AVE_Wreg(ave_pf_dat.pfnm0003[i], AVE_PFNM0003+0x40*i);
    }

    for(i=0; i<18; ++i){
        AVE_Wreg(ave_pf_dat.pfnm0407[i], AVE_PFNM0407+0x40*i);
    }

    for(i=0; i<8; ++i){
        AVE_Wreg(ave_pf_dat.pfnm0c0f[i], AVE_PFNM0C0F+0x40*i);
    }
    AVE_Wreg(ave_pf_dat.pfnm0c0f[8], AVE_PFNM0C0F+0x440);

    for(i=0; i<7; ++i){
        AVE_Wreg(ave_pf_dat.pfnm1013[i], AVE_PFNM1013+0x40*i);
    }
    AVE_Wreg(ave_pf_dat.pfnm1013[7], AVE_PFNM1013+0x440);

    for(i=0; i<7; ++i){
        AVE_Wreg(ave_pf_dat.pfnm1417[i], AVE_PFNM1417+0x40*i);
    }
    AVE_Wreg(ave_pf_dat.pfnm1417[7], AVE_PFNM1417+0x440);

    for(i=0; i<7; ++i){
        AVE_Wreg(ave_pf_dat.pfnm181b[i], AVE_PFNM181B+0x40*i);
    }
    AVE_Wreg(ave_pf_dat.pfnm181b[7], AVE_PFNM181B+0x440);

    for(i=0; i<7; ++i){
        AVE_Wreg(ave_pf_dat.pfnm1c1f[i], AVE_PFNM1C1F+0x40*i);
    }
    AVE_Wreg(ave_pf_dat.pfnm1c1f[7], AVE_PFNM1C1F+0x440);

    for(i=0; i<7; ++i){
        AVE_Wreg(ave_pf_dat.pfnm2023[i], AVE_PFNM2023+0x40*i);
    }
    AVE_Wreg(ave_pf_dat.pfnm2023[7], AVE_PFNM2023+0x440);
    for(i=0; i<7; ++i){
        AVE_Wreg(ave_pf_dat.pfnm2427[i], AVE_PFNM2427+0x40*i);
    }
    AVE_Wreg(ave_pf_dat.pfnm2427[7], AVE_PFNM2427+0x440);

    for(i=0; i<7; ++i){
        AVE_Wreg(ave_pf_dat.pfnm282b[i], AVE_PFNM282B+0x40*i);
    }
    AVE_Wreg(ave_pf_dat.pfnm282b[7], AVE_PFNM282B+0x440);

    for(i=0; i<7; ++i){
        AVE_Wreg(ave_pf_dat.pfnm2c2f[i], AVE_PFNM2C2F+0x40*i);
    }
    AVE_Wreg(ave_pf_dat.pfnm2c2f[7], AVE_PFNM2C2F+0x440);

    for(i=0; i<7; ++i){
        AVE_Wreg(ave_pf_dat.pfnm3033[i], AVE_PFNM3033+0x40*i);
    }
    AVE_Wreg(ave_pf_dat.pfnm3033[7], AVE_PFNM3033+0x440);

    for(i=0; i<7; ++i){
        AVE_Wreg(ave_pf_dat.pfnm3437[i], AVE_PFNM3437+0x40*i);
    }
    AVE_Wreg(ave_pf_dat.pfnm3437[7], AVE_PFNM3437+0x440);

    for(i=0; i<7; ++i){
        AVE_Wreg(ave_pf_dat.pfnm383b[i], AVE_PFNM383B+0x40*i);
    }
    AVE_Wreg(ave_pf_dat.pfnm383b[7], AVE_PFNM383B+0x440);

    for(i=0; i<18; ++i){
        AVE_Wreg(ave_pf_dat.pfnmbyte0[i], AVE_PFMBYTE_BASE+0x8*i);
    }

    for(i=0; i<7; ++i){
        AVE_Wreg(ave_pf_dat.pfnmbyte1[i], AVE_PFMBYTE_BASE+0x4+0x8*i);
    }
    AVE_Wreg(ave_pf_dat.pfnmbyte1[7], AVE_PFMBYTE_BASE+0x8C);

    for(i=0; i<7; ++i){
        AVE_Wreg(ave_pf_dat.pfnmbit[i], AVE_PFMBIT_BASE+0x4*i);
    }
    AVE_Wreg(ave_pf_dat.pfnmbit[7], AVE_PFMBIT_BASE+0x44);

    for(i=0; i<19; ++i){
        AVE_Wreg(ave_pf_dat.pfnmsel[i], AVE_PFSEL_BASE+0x4*i);
    }

    AVE_Wreg(ave_pf_dat.pfen, AVE_PFEN);

    return;
}

#ifdef AVE_DEBUG_RXPACKET_INFO
static struct AVE_rxsts_info ave_rxinf_tbl[AVE_RXINF_DBG_TBLMAX];
static int ave_rxinf_cur = 0; 

void ave_rxsts_add(struct AVE_rxsts_info *rxsts_info)
{

    ave_rxinf_tbl[ave_rxinf_cur].ring     = rxsts_info->ring;
    ave_rxinf_tbl[ave_rxinf_cur].jiffies  = rxsts_info->jiffies;
    ave_rxinf_tbl[ave_rxinf_cur].proc_idx = rxsts_info->proc_idx;
    ave_rxinf_tbl[ave_rxinf_cur].pktlen   = rxsts_info->pktlen;
    ave_rxinf_tbl[ave_rxinf_cur].ih_id    = rxsts_info->ih_id;
    ave_rxinf_tbl[ave_rxinf_cur].hwtimer  = rxsts_info->hwtimer;
    ave_rxinf_cur = (ave_rxinf_cur + 1) % AVE_RXINF_DBG_TBLMAX;

}
#endif 

void ave_util_mssr_resetting(struct net_device *netdev)
{
    struct ave_private  *ave_priv = netdev_priv(netdev);
    uint32_t skb_mss_size, reg_mssr_val;
    uint32_t tx_proc_idx, tx_done_idx, tx_desc_num;
    uint32_t skb_mss_size_min=AVE_MSSR_DEFAULT;

    tx_proc_idx = ave_priv->tx_proc_idx;
    tx_done_idx = ave_priv->tx_done_idx;
    tx_desc_num = ave_priv->tx_dnum;

    if( tx_proc_idx == tx_done_idx ){
        AVE_Wreg(AVE_MSSR_DEFAULT, AVE_MSSR);
        return;
    }

    AVE_Rreg(reg_mssr_val, AVE_MSSR);
    if(reg_mssr_val < AVE_MSSR_THRESHOLD){
        while(tx_proc_idx != tx_done_idx){

            if(ave_priv->tx_skbs[tx_done_idx] != NULL){
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32) 
                skb_mss_size = (uint32_t)(skb_shinfo(ave_priv->tx_skbs[tx_done_idx])->tso_size);
#else 
                skb_mss_size = (uint32_t)(skb_shinfo(ave_priv->tx_skbs[tx_done_idx])->gso_size);
#endif 

                if( (skb_mss_size > 0) && (skb_mss_size_min > skb_mss_size) ){
                    skb_mss_size_min = skb_mss_size;
                }
            }

            tx_done_idx = (tx_done_idx+1)%tx_desc_num;
        } 

        AVE_Wreg(skb_mss_size_min, AVE_MSSR); 
    }

    return;
}

void ave_util_autoneg_result_check(struct net_device *netdev, uint32_t *speed_P, uint32_t *duplex_P, uint32_t *pause_P)
{
    struct ave_private  *ave_priv = netdev_priv(netdev);
    uint32_t linkspeed = 1000;
    uint32_t duplex_mode = 1;
    uint32_t support_pause = 1;
    uint32_t gbit_advertising=0, gbit_partner_ability=0;
    uint32_t advertising=0, partner_ability=0;
    uint32_t match = 0;

    if(ave_gbit_mode == 1){  
        gbit_advertising = ave_api_mdio_read(netdev, ave_priv->phy_id, MII_CTRL1000);
        gbit_partner_ability = ave_api_mdio_read(netdev, ave_priv->phy_id, MII_STAT1000);
    }

    advertising = ave_api_mdio_read(netdev, ave_priv->phy_id, MII_ADVERTISE);
    partner_ability = ave_api_mdio_read(netdev, ave_priv->phy_id, MII_LPA);
    if( ((gbit_advertising & ADVERTISE_1000FULL) != 0)
             && ((gbit_partner_ability & LPA_1000FULL) != 0) ){
        duplex_mode = 1;
        linkspeed = 1000;
    }
    else if( ((gbit_advertising & ADVERTISE_1000HALF) != 0)
           && ((gbit_partner_ability & LPA_1000HALF) != 0) ){
        duplex_mode = 0;
        linkspeed = 1000;
    }
    else{
        match = (advertising & partner_ability);
        if( (match & LPA_100FULL) != 0 ){
            duplex_mode = 1;
            linkspeed = 100;
        }
        else if( (match & LPA_100HALF) != 0 ){
            duplex_mode = 0;
            linkspeed = 100;
        }
        else if( (match & LPA_10FULL) != 0 ){
            duplex_mode = 1;
            linkspeed = 10;
        }
        else{
            duplex_mode = 0;
            linkspeed = 10;
        }
    }

    if( ((advertising & partner_ability) & LPA_PAUSE_CAP) == 0 ){
        support_pause = 0;
    }

    if ( netif_msg_link(ave_priv) ) {
        printk(KERN_DEBUG "%s(state=%04x linkspeed=%d duplex=%d pause=%d)\n",
            __func__, match, linkspeed, duplex_mode, support_pause);
    }
    *speed_P = linkspeed;
    *duplex_P = duplex_mode;
    *pause_P = support_pause;

    return;
}

void ave_util_avereg_linkspeedsetting(uint32_t linkspeed)
{
    uint32_t reg_val;

    AVE_Rreg(reg_val, AVE_TXCR);

    reg_val &= ~(AVE_TXCR_TXSPD);    
    if( (ave_gbit_mode == 1) && (linkspeed == 1000) ){
        reg_val |= AVE_TXCR_TXSPD_1G;
    }
    else if(linkspeed == 100){
        reg_val |= AVE_TXCR_TXSPD_100;
    }
    else{
        reg_val |= AVE_TXCR_TXSPD_10;
    }

    AVE_Wreg(reg_val, AVE_TXCR);

    AVE_Rreg(reg_val, AVE_LINKSEL);

    if (linkspeed == 10) {
        reg_val &= ~(AVE_LINKSEL_100M);    
    }
    else {
        reg_val |= AVE_LINKSEL_100M;       
    }

    AVE_Wreg( reg_val, AVE_LINKSEL);

    return;
}

void ave_util_linkchk_needspinlock(struct net_device *netdev)
{
    struct ave_private  *ave_priv = netdev_priv(netdev);
    int                   link_status = AVE_LS_DOWN;
    uint32_t         reg_lpa_val, reg_bmcr_val, support_pause;
    uint32_t         reg_val, reg_txcr_val, reg_rxcr_val;
    uint32_t linkspeed = 1000;
    uint32_t duplex_mode = 1;

    support_pause = 1;

    if(ave_util_mii_link_ok(netdev) != 0){
        link_status = AVE_LS_UP;
    }

    if( link_status == AVE_LS_UP ) {

        if ( netif_msg_ifup(ave_priv) ) {
            DBG_PRINT("Link has gone up.\n");
        }
        reg_lpa_val = ave_api_mdio_read(netdev, ave_priv->phy_id, MII_LPA);
        if ( netif_msg_link(ave_priv) ) {
            DBG_PRINT("%s: Old partner %04x, new %08x, adv %08x.\n",
                      netdev->name, ave_priv->partner, reg_lpa_val, ave_priv->mii_if.advertising);
        }

        if (reg_lpa_val != (uint32_t)(ave_priv->partner)) {
            if (netif_msg_link(ave_priv)) {
                DBG_PRINT("%s: Link status change.\n", netdev->name);
            }
            ave_priv->partner = reg_lpa_val;
        }

        ave_util_mii_check_link(netdev);

        reg_bmcr_val = ave_api_mdio_read(netdev, ave_priv->phy_id, MII_BMCR);
        if ( reg_bmcr_val & BMCR_ANENABLE ) {

            ave_util_autoneg_result_check(netdev, &linkspeed, &duplex_mode, &support_pause);
            ave_priv->mii_if.full_duplex = duplex_mode;

        } else {
#ifdef AVE_ENABLE_PAUSE
            if ( reg_bmcr_val & BMCR_FULLDPLX ) {
                if ( netif_msg_link(ave_priv) ) {
                    DBG_PRINT("Full Duplex by Manual in ave_util_linkchk_needspinlock(). duplex=%d\n",
                              ave_priv->mii_if.full_duplex);
                }
                support_pause = 1;
            } else {
                if ( netif_msg_link(ave_priv) ) {
                    DBG_PRINT("Half Duplex by Manual in ave_util_linkchk_needspinlock(). duplex=%d\n",
                              ave_priv->mii_if.full_duplex);
                }
                support_pause = 0;
            }
#else
            support_pause = 0;
#endif 

             if( (ave_gbit_mode == 1)    
                  && ((reg_bmcr_val & BMCR_SPEED1000) != 0) ){
                 linkspeed = 1000;
             }
             else if( (reg_bmcr_val & BMCR_SPEED100) != 0){
                 linkspeed = 100;
             }
             else{
                 linkspeed = 10;
             }
             duplex_mode = ave_priv->mii_if.full_duplex;

         }

        ave_util_avereg_linkspeedsetting(linkspeed);

        AVE_Rreg(reg_val, AVE_RXCR);
        AVE_Rreg(reg_txcr_val, AVE_TXCR);
        reg_rxcr_val = reg_val;
        if( ave_priv->mii_if.full_duplex ){
            reg_rxcr_val |= AVE_RXCR_FDUPEN;
        } else {
            reg_rxcr_val &= ~(AVE_RXCR_FDUPEN);
        }
        if( support_pause ){
            reg_rxcr_val |= AVE_RXCR_FLOCTR;
            reg_txcr_val |= AVE_TXCR_FLOCTR;
        } else {
            reg_rxcr_val &= ~(AVE_RXCR_FLOCTR);
            reg_txcr_val &= ~(AVE_TXCR_FLOCTR);
        }

        if( reg_val != reg_rxcr_val ){
            if ( netif_msg_link(ave_priv) ) {
                DBG_PRINT("%s: Previous MAC mode(rxcr=0x%08x).\n", netdev->name, reg_val);
                DBG_PRINT("%s: Change MAC mode(txcr=0x%08x  rxcr=0x%08x).\n",
                           netdev->name, reg_txcr_val, reg_rxcr_val);
            }
            AVE_Wreg((reg_rxcr_val & ~(AVE_RXCR_RXEN)), AVE_RXCR);

            AVE_Wreg( reg_txcr_val, AVE_TXCR);
            AVE_Wreg( reg_rxcr_val, AVE_RXCR);
        }

    } else if ( link_status == AVE_LS_DOWN ) {
        if ( netif_msg_ifdown(ave_priv) ) {
            DBG_PRINT("Link has gone down.\n");
        }

        ave_util_mii_check_link(netdev);

    } else {
        if (netif_msg_intr(ave_priv)) {
          printk(KERN_WARNING "ave: Link status changed, " \
                 "but we don't know what it is.\n");
        }
    }

    return;
}


uint32_t ave_get_txfreenum(struct net_device *netdev)
{
    struct ave_private  *ave_priv = netdev_priv(netdev);
    uint32_t tx_proc_idx,tx_done_idx,tx_desc_num,tx_free_num;

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_get_txfreenum()\n");
    }

    ave_proc_add("ave_get_txfreenum");

    tx_proc_idx = ave_priv->tx_proc_idx;
    tx_done_idx = ave_priv->tx_done_idx;
    tx_desc_num = ave_priv->tx_dnum;
    tx_free_num = ((tx_done_idx + tx_desc_num-1) - tx_proc_idx)%tx_desc_num;

    return tx_free_num;
}

int ave_set_rxdesc(struct net_device *netdev, int ring_num, int entry_num)
{
    struct ave_private  *ave_priv = netdev_priv(netdev);
    int    ret_val = 0;
    struct sk_buff *skb;
    uint64_t reg_val_64=0;
    uint32_t reg_val=0;
    uint32_t align;
    uint32_t descriptor;
    unsigned char *buffptr = NULL; 

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_set_rxdesc()\n");
    }

    ave_proc_add("ave_set_rxdesc");

    skb = ave_priv->rx_skbs[ring_num][entry_num];

    if (skb == NULL) {
        skb = dev_alloc_skb ( AVE_MAX_ETHFRAME + ave_priv->offset + 256 );
        if (skb == NULL) {
            if ( netif_msg_hw(ave_priv) ) {
                printk(KERN_ERR "ave: Could not allocate skb for rx.\n");
            }
            ret_val = -1;
            goto END_PROC;
        }
    } else {
    }

    descriptor = AVE_RXDM + ave_priv->rx_dsa[ring_num] + entry_num*AVE_DESC_SIZE;
    reg_val = (AVE_STS_INTR | AVE_STS_OWN);
    AVE_Wreg(reg_val, descriptor);
#ifdef NET_SKBUFF_DATA_USES_OFFSET
    align = (uint64_t)(skb_tail_pointer(skb)) & 0x7f;
    align = 0x80 - align;
    align &= 0x7f;
    skb_reserve(skb, align);

    buffptr = skb_tail_pointer(skb);
#else
    align = (uint32_t)skb->tail & 0x7f;
    align = 0x80 - align;
    align &= 0x7f;
    skb_reserve(skb, align);

    buffptr = skb->tail;
#endif

    if(ave_64bit_addr_discrypt == 1){ 
        reg_val_64 = ave_cache_control(netdev, (uint64_t)buffptr,
                (uint32_t)(AVE_MAX_ETHFRAME+ave_priv->offset), DMA_FROM_DEVICE);
        if(reg_val_64 == -ENOMEM){
            ave_priv->rx_skbs_dma[ring_num][entry_num] = 0;
            reg_val_64 = virt_to_phys(buffptr);
            printk("yk-dbg (RX) reg_val_modify=%016llx buffptr=%p \n", reg_val_64, buffptr);
        }
        else{
            ave_priv->rx_skbs_dma[ring_num][entry_num] = reg_val_64;
            ave_priv->rx_skbs_dmalen[ring_num][entry_num] = (uint32_t)(AVE_MAX_ETHFRAME+ave_priv->offset);
        }

        reg_val_64 += AVE_DDRMEM_SHIFT;  
        AVE_Wreg((uint32_t)(reg_val_64 >> 32), descriptor + 8);  
        AVE_Wreg((uint32_t)(reg_val_64 & 0xFFFFFFFF), descriptor + 4);  

    } else { 
        reg_val = ave_cache_control(netdev, (uint64_t)buffptr,
                (uint32_t)(AVE_MAX_ETHFRAME+ave_priv->offset), DMA_FROM_DEVICE);
        if(reg_val == -ENOMEM){
            ave_priv->rx_skbs_dma[ring_num][entry_num] = 0;
            reg_val = virt_to_phys(buffptr);
            printk("yk-dbg (RX) reg_val_modify=%08x buffptr=%p \n", reg_val, buffptr);
        }
        else{
            ave_priv->rx_skbs_dma[ring_num][entry_num] = reg_val;
            ave_priv->rx_skbs_dmalen[ring_num][entry_num] = (uint32_t)(AVE_MAX_ETHFRAME+ave_priv->offset);
        }

        reg_val += AVE_DDRMEM_SHIFT;  
        AVE_Wreg(reg_val, descriptor + 4);  

    } 

    ave_priv->rx_skbs[ring_num][entry_num] = skb;

    reg_val = (AVE_STS_INTR | AVE_MAX_ETHFRAME);
    AVE_Wreg(reg_val, descriptor);

END_PROC:
    return ret_val;
}

int ave_refill_rx(struct net_device *netdev, int ring_num)
{
    struct ave_private  *ave_priv = netdev_priv(netdev);
    int entry_num;

    while ( ave_priv->rx_proc_idx[ring_num] != ave_priv->rx_done_idx[ring_num] ) {
        entry_num = ave_priv->rx_done_idx[ring_num];
        if( ave_set_rxdesc(netdev, ring_num, entry_num) != 0 ){
            break;
        }
        ave_priv->rx_done_idx[ring_num] = (ave_priv->rx_done_idx[ring_num] + 1)%(ave_priv->rx_dnum[ring_num]);
    }

    return 0;
}

int ave_desc_switch(struct net_device *netdev, int state)
{
    struct ave_private *priv = netdev_priv(netdev);
    uint32_t reg_val=0;
    int           ring_num;
    long          counter = 0;

    if ( netif_msg_drv(priv) ) {
        DBG_PRINT("ave_desc_switch()\n");
    }
    ave_proc_add("ave_desc_switch");

    if( state == AVE_DESC_START ){
        reg_val = AVE_DESCC_TD;
        for ( ring_num = 0; ring_num < AVE_RXRING_NUM; ring_num++ ) {
            if( priv->rx_dnum[ring_num] ){
                reg_val |= (AVE_DESCC_RD0 << ring_num);
            }
        }
        AVE_Wreg( reg_val, AVE_DESCC);
    } else if( state == AVE_DESC_STOP ){
        AVE_Wreg( AVE_ZERO, AVE_DESCC);
        counter = 0;
        while(1){
            udelay(100);
            AVE_Rreg(reg_val, AVE_DESCC);
            if ( reg_val == AVE_ZERO ) {
                break;
            }
            counter++;
            if( counter > 100) {
                printk(KERN_ERR "%s: Can't stop descriptor (DESCC=0x%08x).\n", netdev->name, reg_val);
                return -1;
            }
        }
    } else if( state == AVE_DESC_RX_SUSPEND ){
        AVE_Rreg(reg_val, AVE_DESCC);
        reg_val = ( reg_val | AVE_DESCC_RDSTP) & AVE_DESCC_CTRL_MASK;
        AVE_Wreg(reg_val, AVE_DESCC);
        counter = 0;
        while(1){
            udelay(100);
            AVE_Rreg(reg_val, AVE_DESCC);
            if ( reg_val & (AVE_DESCC_RDSTP<<16) ) {
                break;
            }
            counter++;
            if( counter > 1000) {
                printk(KERN_ERR "%s: Can't suspend descriptor (DESCC=0x%08x).\n", netdev->name, reg_val);
                break;
            }
        }
    } else if( state == AVE_DESC_RX_PERMIT ){
        AVE_Rreg(reg_val, AVE_DESCC);
        reg_val = ( reg_val & ~AVE_DESCC_RDSTP) & AVE_DESCC_CTRL_MASK;
        AVE_Wreg(reg_val, AVE_DESCC);
    } else {
        return -1;
    }

    return 0;
}

int ave_descfull_check(struct net_device *netdev, int ring_num)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    unsigned long reg_rxdcp_adr;
    uint32_t      reg_rxdcp1_val,reg_rxdcp2_val;
    uint32_t      descriptor;
    uint32_t      cmdsts;
    int           count;

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_descfull_check(ring_num=%d)\n",ring_num);
    }
    ave_proc_add("ave_descfull_check");

    reg_rxdcp_adr = AVE_RXDCP0 + (ring_num * 0x4);

    for(count = 0; count < 100; count++){
        AVE_Rreg( reg_rxdcp1_val, reg_rxdcp_adr);
        descriptor = AVE_RXDM + reg_rxdcp1_val;
        AVE_Rreg(cmdsts, descriptor);

        AVE_Rreg( reg_rxdcp2_val, reg_rxdcp_adr);
        if( reg_rxdcp1_val == reg_rxdcp2_val ){
            break;
        }
    }

    if( cmdsts & AVE_STS_OWN ){
        return 1;
    } else {
        return 0;
    }
}

void ave_get_version(struct net_device *netdev)
{
    struct ave_private  *ave_priv = netdev_priv(netdev);
    union {
        uint32_t hex_data;
        char string[4];
    } ave_idr;
    char msg[40];

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_get_version()\n");
    }
    ave_proc_add("ave_get_version");

    AVE_Rreg(ave_idr.hex_data, AVE_IDR);
    ave_endian_change(ave_idr.string);
    memset(ave_priv->id_str, 0, sizeof(ave_priv->id_str));
    memcpy(ave_priv->id_str, ave_idr.string, 4);

    AVE_Rreg(ave_priv->ave_vr.hex, AVE_VR);

    sprintf(msg, "Socionext %c%c%c%c(Ver.%04x)\n",
        ave_priv->id_str[0], ave_priv->id_str[1], ave_priv->id_str[2], ave_priv->id_str[3],
        ave_priv->ave_vr.hex);
    printk(KERN_INFO "%s", msg);
    printk(KERN_INFO "    Kernel base=0x%08lx, irq=%d\n",
       netdev->base_addr, netdev->irq);
    ave_proc_add(msg);
}

void ave_timer(unsigned long data)
{
    struct net_device *netdev = (struct net_device *)data;
    struct ave_private  *ave_priv = netdev_priv(netdev);
    unsigned long flags;
    unsigned long gisr;

    if ( netif_msg_timer(ave_priv) ) {
        DBG_PRINT("ave_timer()\n");
    }

    spin_lock_irqsave(&ave_priv->lock, flags);

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    if(ave_flg_reset != 0){
        printk("ave_timer reset status \n");
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return;  
    }
#endif 

    if(ave_use_omniphy == 1){
        ave_omniphy_link_status_update(netdev);  
    }

    ave_tx_completion(netdev);

    ave_util_mssr_resetting(netdev);

    if(ave_poll_link != 0){
        ave_util_linkchk_needspinlock(netdev);
    }

    AVE_Rreg(gisr, AVE_GISR);
    if ( gisr & AVE_GI_RXOVF ) {
        ave_info[0].value++;
        ave_priv->stats.rx_fifo_errors++;
        ave_rxf_reset(netdev);
    }

    ave_priv->timer.expires = RUN_AT(AVE_MNTR_TIME);
    add_timer(&ave_priv->timer);

    spin_unlock_irqrestore(&ave_priv->lock, flags);
}

int ave_tx_completion(struct net_device *netdev)
{
    struct ave_private  *ave_priv = netdev_priv(netdev);
    uint32_t reg_val;
    uint32_t tx_proc_idx,tx_done_idx,tx_dnum;
    uint32_t freebuf_flag = 0;  
#if defined(AVE_TXTHREAD_BREAK)
    int thread_break_cnt = 0;
#endif  

    if ( netif_msg_tx_done(ave_priv) ) {
        DBG_PRINT("ave_tx_completion()\n");
    }
    ave_proc_add("ave_tx_completion");

    AVE_Wreg(AVE_GI_TX, AVE_GISR);

    tx_proc_idx = ave_priv->tx_proc_idx;
    tx_done_idx = ave_priv->tx_done_idx;
    tx_dnum = ave_priv->tx_dnum;

    while( tx_proc_idx != tx_done_idx ){
#if defined(AVE_TXTHREAD_BREAK)
        thread_break_cnt++;
        if(need_resched() && (thread_break_cnt >= AVE_MIN_TX) )  {
            break;
        }
#endif  

        AVE_Rreg(reg_val, AVE_TXDM + (tx_done_idx*AVE_DESC_SIZE));
        if( reg_val & AVE_STS_OWN ){
            break; 
        } else {
            if ( (reg_val & AVE_STS_OK) && (reg_val & AVE_STS_LAST) ){

                ave_priv->stats.tx_packets++;

                ave_priv->stats.tx_bytes += reg_val & AVE_STS_PKTLEN_TSO;
                ave_info[7].value  += reg_val & AVE_STS_PKTLEN_TSO;

            } else if( reg_val & AVE_STS_OK ){
                ave_priv->stats.tx_bytes += reg_val & AVE_STS_PKTLEN_TSO;
                ave_info[7].value  += reg_val & AVE_STS_PKTLEN_TSO;

            } else {
                if ( reg_val & AVE_STS_LAST ){

                    if (netif_msg_tx_err(ave_priv)) {
                        DBG_PRINT("%s: Tx Error. status %08x.\n",netdev->name, reg_val);
                    }
                    ave_priv->stats.tx_errors++;
                    if ( reg_val & (AVE_STS_OWC | AVE_STS_EC) ){
                        ave_priv->stats.collisions++;
                    }
                }
            }

            if ( ave_priv->tx_skbs[tx_done_idx] != NULL ) {
#if 1
                if(ave_priv->tx_skbs_dma[tx_done_idx] != 0){
                    dma_unmap_single(netdev->dev.parent,
                                    ave_priv->tx_skbs_dma[tx_done_idx],
                                    ave_priv->tx_skbs_dmalen[tx_done_idx], DMA_TO_DEVICE);
                    ave_priv->tx_skbs_dma[tx_done_idx] = 0;
                }
#endif
                dev_kfree_skb_any( ave_priv->tx_skbs[tx_done_idx] );
                ave_priv->tx_skbs[tx_done_idx] = NULL;
                freebuf_flag = 1;
            }
            tx_done_idx = (tx_done_idx+1)%tx_dnum;
        }
    }

    ave_priv->tx_done_idx = tx_done_idx;

    if ( netif_queue_stopped(netdev) ) {
        if( netif_running(netdev) ){   
            if( freebuf_flag != 0 ){
                netif_wake_queue(netdev);
            }
        }
    }

    return 0;
}

#if defined(AVE_TXTHREAD_BREAK)
static int ave_tx_restart(void *in_netdev)
{
    struct net_device *netdev = (struct net_device *)in_netdev;

    while ( !kthread_should_stop() ) {
        wait_event_interruptible(ave_wq, (ave_tx_thread_break_flg != 0));
        if ( (ave_tx_thread_break_flg != 0) && (!kthread_should_stop()) ) {
            ave_tx_thread_break_flg = 0;
            netif_wake_queue(netdev);
        }
    }

    return 0;
}
#endif  

int ave_rx(struct net_device *netdev, int ring_num, int num)
{
    struct ave_private  *priv = netdev_priv(netdev);
    int ret_val;
    uint32_t cmdsts;
    int   pktlen;
    struct sk_buff *skb;
    int position;
#ifdef AVE_ENABLE_CSUM_OFFLOAD
    int csumerr = 0;
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32) 
    struct ethhdr *ethh;
    unsigned char ih_proto = 0;
#endif 

#ifndef AVE_SINGLE_RXRING_MODE 
#ifdef AVE_DEBUG_RXPACKET_INFO 
    unsigned short ih_id = 0;
#endif 
#endif 

    uint32_t descriptor;

    uint32_t rx_proc_idx,rx_done_idx,rx_desc_num;
    int   rx_restcnt;

#if defined(AVE_RXTHREAD_BREAK)
    int thread_break_cnt = 0;
#endif  

    if (netif_msg_intr(priv)) {
        DBG_PRINT("%s: ave_rx().\n", netdev->name);
        ave_proc_add("ave_rx()");
    }

    rx_proc_idx = priv->rx_proc_idx[ring_num];
    rx_done_idx = priv->rx_done_idx[ring_num];
    rx_desc_num = priv->rx_dnum[ring_num];
    rx_restcnt  = ((rx_done_idx + rx_desc_num-1) - rx_proc_idx)%rx_desc_num;
    for(ret_val=0; ret_val<num; ret_val++ ) {

#if defined(AVE_RXTHREAD_BREAK)
        thread_break_cnt++;
        if(need_resched() && (thread_break_cnt >= AVE_MIN_RX) )  {
            break;
        }
#endif  

        if (--rx_restcnt < 0)
            break;

        descriptor = AVE_RXDM + priv->rx_dsa[ring_num] + rx_proc_idx*AVE_DESC_SIZE;

        AVE_Rreg(cmdsts, descriptor);

        if( (cmdsts & AVE_STS_OWN) != AVE_STS_OWN ){
            break;
        }

        if ( (cmdsts & AVE_STS_OK) != AVE_STS_OK ) {
            priv->stats.rx_errors++;
            if ( netif_msg_rx_err(priv) ) {
                printk(KERN_ERR "%s;  Recv error at (proc_idx=%d) of ring(%d)\n",
                                netdev->name,rx_proc_idx,ring_num);
            }
            rx_proc_idx = (rx_proc_idx+1)%rx_desc_num;
            continue;
        }

        pktlen = cmdsts & AVE_STS_PKTLEN;
        if (netif_msg_rx_status(priv)) {
            DBG_PRINT("%s:  ave_rx() cmdsts %08x len %d.\n",
              netdev->name, cmdsts, pktlen);
        }

        if ( pktlen < 0 ) {
            printk(KERN_ERR "%s: 0 length packet received???\n",netdev->name);
            break;
        }

        skb = priv->rx_skbs[ring_num][rx_proc_idx];
        priv->rx_skbs[ring_num][rx_proc_idx] = NULL;
        skb->dev = netdev;

#if 1
        if(priv->rx_skbs_dma[ring_num][rx_proc_idx] != 0){
            dma_unmap_single(netdev->dev.parent,
                     priv->rx_skbs_dma[ring_num][rx_proc_idx],
                     priv->rx_skbs_dmalen[ring_num][rx_proc_idx], DMA_FROM_DEVICE);
            priv->rx_skbs_dma[ring_num][rx_proc_idx] = 0;
        }
#else
        ave_cache_control(netdev, (uint64_t)skb->data, (uint32_t)(pktlen+2), DMA_FROM_DEVICE);
#endif

#if 0
{
  unsigned long looo;
  printk("*** rx pkt dump (%d) *** ", pktlen);
  for(looo=0;looo<pktlen; looo++){
    if(looo%16 ==0){
      printk("\n");
    }
    printk("%02x ", skb->data[looo]);
  }
  printk("\n\n");
}
#endif
        skb_reserve(skb, 2);        
        skb_put(skb, (unsigned int)pktlen);

        if ( netif_msg_pktdata(priv) ) {
            uint32_t *pbuff;
            uint32_t workdata;

            DBG_PRINT("  skb_len = 0x%08X\n", (int)skb->len);
            DBG_PRINT("   (The top 2 bytes are offset.)\n");
            pbuff = (uint32_t *)(skb->data - 2);
            for(position = 0; position < (skb->len + 2) / 4; position++){
                workdata = *(pbuff + position);
                ave_endian_change(&workdata);
                DBG_PRINT("   pbuff[%03d] = 0x%08X\n", position, (int)workdata);
            }
            if((skb->len + 2) % 4){
                workdata = *(pbuff + position);
                ave_endian_change(&workdata);
                DBG_PRINT("   pbuff[%03d] = 0x%08X\n", position, (int)workdata);
            }
        }

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32) 
        skb->mac.raw = skb->data;
        ethh = eth_hdr(skb);

        if ( ntohs(ethh->h_proto) == ETH_P_IP ) {
            skb->nh.iph = (struct iphdr *)(skb->data + ETH_HLEN);
            ih_proto = skb->nh.iph->protocol;
#ifndef AVE_SINGLE_RXRING_MODE 
#ifndef AVE_DEBUG_RXPACKET_INFO 
            ih_id    = skb->nh.iph->id;
#endif 
#endif 
        } else if ( ntohs(ethh->h_proto) == ETH_P_IPV6 ) {
            skb->nh.ipv6h = (struct ipv6hdr *)(skb->data + ETH_HLEN);
            ih_proto = skb->nh.ipv6h->nexthdr;
#ifndef AVE_SINGLE_RXRING_MODE 
#ifndef AVE_DEBUG_RXPACKET_INFO 
            ih_id    = 0;
#endif 
#endif 
        }
#endif 

        skb->protocol = eth_type_trans(skb, netdev);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32) 
        if ( skb->protocol != ethh->h_proto ) {
            if ( netif_msg_rx_err(priv) ) {
                DBG_PRINT("Ether Protocol Analysis Error.\n");
            }
        }
#endif 

#ifdef AVE_ENABLE_CSUM_OFFLOAD
        if ( cmdsts & AVE_STS_CSSV ) {
            if ( netif_msg_rx_status(priv) ) {
                DBG_PRINT("checksum checking...\n");
            }

            if ( cmdsts & AVE_STS_CSER ) {
                if ( cmdsts & AVE_STS_TCPE ) {
                    if ( netif_msg_rx_err(priv) ) {
                        DBG_PRINT("TCP Checksum Error\n");
                    }
                    ave_info[1].value++;
                }
                if ( cmdsts & AVE_STS_UDPE ) {
                    if ( netif_msg_rx_err(priv) ) {
                        DBG_PRINT("UDP Checksum Error\n");
                    }
                    ave_info[2].value++;
                }
                if ( cmdsts & AVE_STS_IPER ) {
                    if ( netif_msg_rx_err(priv) ) {
                        DBG_PRINT("IP Checksum Error\n");
                    }
                    ave_info[3].value++;
                }
                csumerr = 1;
            } else {
                if ( netif_msg_rx_status(priv) ) {
                    DBG_PRINT("Checksum OK\n");
                }
                csumerr = 0;
            }
        } else {
            if ( netif_msg_rx_status(priv) ) {
                DBG_PRINT("checksum to be checked(Not IP).\n");
            }
            csumerr = 1;
        }
        if ( csumerr == 0 ) {
            if ( netif_msg_rx_status(priv) ) {
                DBG_PRINT("checksum unnecessary.\n");
            }
            skb->ip_summed = CHECKSUM_UNNECESSARY;
        } else if ( netif_msg_rx_status(priv) ) {
            DBG_PRINT("checksum will be checked in Linux.\n");
        }
#endif 

        ave_info[8].value += pktlen;
        priv->stats.rx_packets++;
        priv->stats.rx_bytes += pktlen;

#ifndef AVE_SINGLE_RXRING_MODE
#ifdef AVE_DEBUG_RXPACKET_INFO
        {
            struct AVE_rxsts_info  rxinf;
            uint32_t            reg_gticr;

            AVE_Rreg(reg_gticr, AVE_GTICR);

            rxinf.ring     = ring_num;
            rxinf.jiffies  = jiffies;
            rxinf.pktlen   = pktlen;
            rxinf.proc_idx = rx_proc_idx;
            rxinf.ih_id    = ih_id;
            rxinf.hwtimer  = ( (reg_gticr & 0x7FFF0000) >> 16 );
            ave_rxsts_add(&rxinf);
        }
#endif 
#endif 

        netdev->last_rx = jiffies;
        netif_rx(skb);

        rx_proc_idx = (rx_proc_idx+1)%rx_desc_num;
    }

    priv->rx_proc_idx[ring_num] = rx_proc_idx;
    ave_refill_rx(netdev,ring_num);

    return ret_val;
}

void ave_wait_phy_ready(void)
{
    AVE_WAIT_FOR_PHY_RESET(ave_phy_reset_stability);

    ave_initial_global_reset_completed = 1;

}

void _ave_global_reset(struct net_device *netdev)
{
    struct ave_private *ave_priv;
    uint32_t reg_val_grr,reg_val_descc;
    uint32_t reg_val;

    if ( netdev != NULL ) {
        ave_priv = netdev_priv(netdev);
        if ( netif_msg_drv(ave_priv) ) {
            DBG_PRINT("ave_global_reset()\n");
        }
        AVE_Rreg(reg_val_grr, AVE_GRR);
        if( (reg_val_grr & AVE_GRR_GRST) != AVE_GRR_GRST ){
            AVE_Rreg(reg_val_descc, AVE_DESCC);
            if( (reg_val_descc & AVE_DESCC_RD0) == AVE_DESCC_RD0 ){
                ave_desc_switch(netdev, AVE_DESC_RX_SUSPEND);
            }
        }
    }
    ave_proc_add("ave_global_reset");


    reg_val = (AVE_CFGR_FLE | AVE_CFGR_IPFCEN);

#ifdef AVE_ENABLE_CSUM_OFFLOAD
    reg_val |= AVE_CFGR_CHE;
#endif 

    if( ave_miimode != 3 ){   
        reg_val |= AVE_CFGR_MII;    
    }
    AVE_Wreg( reg_val , AVE_CFGR);

    AVE_Rreg( reg_val , AVE_RSTCTRL);
    reg_val &= (~AVE_RSTCTRL_RMIIRST);
    AVE_Wreg( reg_val , AVE_RSTCTRL);

#if defined(AVE_ENABLE_PHY_SOFT_RESET)
    AVE_Wreg(AVE_GRR_GRST | AVE_GRR_DMACRST, AVE_GRR);
    AVE_WAIT_FOR_AVE_RESET(5);
    ave_initial_global_reset_completed = 1;
#else
    AVE_Wreg(AVE_GRR_GRST | AVE_GRR_DMACRST, AVE_GRR);
    AVE_WAIT_FOR_PHY_RESET(ave_phy_reset_period);

    AVE_Wreg(AVE_GRR_GRST, AVE_GRR);

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    AVE_WAIT_FOR_PHY_RESET(ave_phy_reset_clock_stability);
    ave_wait_phy_ready();

#else 

#if !defined(AVE_SEPARATE_WAIT_FOR_PHY_READY)
    ave_wait_phy_ready();
#else  
    AVE_WAIT_FOR_PHY_RESET(ave_phy_reset_clock_stability);

    ave_initial_global_reset_completed = 0;
#endif 

#endif 
#endif

    AVE_Wreg(AVE_ZERO, AVE_GRR);

    mdelay(5);   

    if( ave_miimode == 2 ){   
        AVE_WAIT_FOR_PHY_RESET(ave_phy_reset_clock_stability);
    }
    AVE_Rreg( reg_val , AVE_RSTCTRL);
    reg_val |= AVE_RSTCTRL_RMIIRST;
    AVE_Wreg( reg_val , AVE_RSTCTRL);

    return;
}

void ave_global_reset_for_init_module(struct net_device *netdev)
{
    _ave_global_reset(netdev);
    return;
}

void ave_global_reset(struct net_device *netdev)
{
#if defined(AVE_ENABLE_PHY_SOFT_RESET)
    int check;
    uint32_t reg_val;
    struct ave_private *ave_priv;
#endif

    _ave_global_reset(netdev);

#if defined(AVE_ENABLE_PHY_SOFT_RESET)
    ave_priv = netdev_priv(netdev);
    reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, MII_BMCR);
    reg_val |= BMCR_RESET;
    ave_api_mdio_write(netdev, ave_priv->phy_id, MII_BMCR, reg_val);
    for(check = 0; check < AVE_BMCR_RESET_CHECK_LIMIT; check++) {
        mdelay(AVE_BMCR_RESET_CHECK_MSEC); 
        reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, MII_BMCR);
        if((reg_val & BMCR_RESET) == 0) {
            break;
        }
    }

    if(check >= AVE_BMCR_RESET_CHECK_LIMIT) {
        reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, MII_BMCR);
        printk(KERN_WARNING "ave: BCMR [0x%08lx] is a PHY reset state.\n", reg_val);
    }
#endif
    return;
}

void ave_rxf_reset(struct net_device *netdev)
{
    struct ave_private *ave_priv = NULL;
    uint32_t reg_rxcr_val;
    int           ring_num;

    ave_priv = netdev_priv(netdev);

    if ( netif_msg_rx_err(ave_priv) ) {
        printk(KERN_WARNING "%s: ave_rxf_reset()\n", netdev->name);
    }
    ave_proc_add("ave_rxf_reset *****");

    AVE_Rreg(reg_rxcr_val, AVE_RXCR);
    AVE_Wreg((reg_rxcr_val & ~(AVE_RXCR_RXEN)), AVE_RXCR);

    ave_desc_switch(netdev, AVE_DESC_RX_SUSPEND);

    for ( ring_num = 0; ring_num < AVE_RXRING_NUM; ring_num++ ) {
        ave_rx(netdev,ring_num, ave_priv->rx_dnum[ring_num]);
    }

    ave_pf_save_unlock();

    AVE_Wreg((AVE_GRR_RXFFR|AVE_GRR_RXBRST), AVE_GRR);
    udelay(40);

    AVE_Wreg(AVE_ZERO, AVE_GRR);
    udelay(10);

    AVE_Wreg(AVE_GI_RXOVF, AVE_GISR);

    for ( ring_num = 0; ring_num < AVE_RXRING_NUM; ring_num++ ) {
        ave_priv->rx_proc_idx[ring_num] = 0;
        ave_priv->rx_done_idx[ring_num] = 0;
    }

    ave_desc_switch(netdev, AVE_DESC_RX_PERMIT);

    ave_pf_restore_unlock();

    AVE_Wreg(reg_rxcr_val, AVE_RXCR);

}

int ave_phy_reset(struct net_device *netdev)
{
    struct ave_private *ave_priv;

    if ( netdev != NULL ) {
        ave_priv = netdev_priv(netdev);

        if ( netif_msg_hw(ave_priv) ) {
            DBG_PRINT("ave_phy_reset()\n");
        }

        ave_proc_add("ave_phy_reset");
    }

    AVE_Wreg(AVE_GRR_PHYRST, AVE_GRR);
    AVE_WAIT_FOR_PHY_RESET(ave_phy_reset_period);
    AVE_Wreg(AVE_ZERO, AVE_GRR);
    AVE_WAIT_FOR_PHY_RESET(ave_phy_reset_stability);

    return 0;
}

int ave_term_ring(struct net_device *netdev){
    struct ave_private *ave_priv = netdev_priv(netdev);
    int    ring_num,entry_num;

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_term_ring()\n");
    }
    ave_proc_add("ave_term_ring");

    for ( entry_num = 0; entry_num < ave_priv->tx_dnum; entry_num++ ) {
        if ( ave_priv->tx_skbs[entry_num] != NULL ) {
            dev_kfree_skb_any( ave_priv->tx_skbs[entry_num] );
            ave_priv->tx_skbs[entry_num] = NULL;
#if 1
            if(ave_priv->tx_skbs_dma[entry_num] != 0){
                dma_unmap_single(netdev->dev.parent,
                                ave_priv->tx_skbs_dma[entry_num],
                                ave_priv->tx_skbs_dmalen[entry_num], DMA_TO_DEVICE);
                ave_priv->tx_skbs_dma[entry_num] = 0;
            }
#endif
        }
    }
    ave_priv->tx_proc_idx = 0;
    ave_priv->tx_done_idx = 0;

    for ( ring_num = 0; ring_num < AVE_RXRING_NUM; ring_num++ ) {
        for ( entry_num = 0; entry_num < ave_priv->rx_dnum[ring_num]; entry_num++ ) {
            if( ave_priv->rx_skbs[ring_num][entry_num] != NULL ){
#if 1
                if(ave_priv->rx_skbs_dma[ring_num][entry_num] != 0){
                    dma_unmap_single(netdev->dev.parent,
                              ave_priv->rx_skbs_dma[ring_num][entry_num],
                              ave_priv->rx_skbs_dmalen[ring_num][entry_num], DMA_FROM_DEVICE);
                    ave_priv->rx_skbs_dma[ring_num][entry_num] = 0;
                }
#endif
                dev_kfree_skb_any(ave_priv->rx_skbs[ring_num][entry_num]);
                ave_priv->rx_skbs[ring_num][entry_num] = NULL;
            }
        }
        ave_priv->rx_proc_idx[ring_num] = 0;
        ave_priv->rx_done_idx[ring_num] = 0;
    }

    return 0;
}

int ave_init_ring(struct net_device *netdev)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    int    ret_val = 0;
    int    ring_num;
    unsigned long rxdescsadr=0; 

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_init_ring()\n");
    }
    ave_proc_add("ave_init_ring");

    ave_priv->tx_proc_idx = 0;
    ave_priv->tx_done_idx = 0;
    memset(ave_priv->tx_skbs,0,sizeof(ave_priv->tx_skbs));
    memset(ave_priv->tx_short,0,sizeof(ave_priv->tx_short));
#if 1
    memset(ave_priv->tx_skbs_dma,0,sizeof(ave_priv->tx_skbs_dma));
    memset(ave_priv->tx_skbs_dmalen,0,sizeof(ave_priv->tx_skbs_dmalen));
#endif

    for ( ring_num = 0, rxdescsadr = 0; ring_num < AVE_RXRING_NUM; ring_num++ ) {
        ave_priv->rx_proc_idx[ring_num] = 0;
        ave_priv->rx_done_idx[ring_num] = 0;
        memset(ave_priv->rx_skbs[ring_num],0,sizeof(ave_priv->rx_skbs[ring_num]));
#if 1
        memset(ave_priv->rx_skbs_dma[ring_num],0,sizeof(ave_priv->rx_skbs_dma[ring_num]));
        memset(ave_priv->rx_skbs_dmalen[ring_num],0,sizeof(ave_priv->rx_skbs_dmalen[ring_num]));
#endif

        ave_priv->rx_dsa[ring_num] = rxdescsadr;
        rxdescsadr += ave_priv->rx_dnum[ring_num] * AVE_DESC_SIZE;
    }

    if( ret_val < 0 ){
        ave_term_ring(netdev);
    }
    return ret_val;
}

int ave_emac_init(struct net_device *netdev)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    int ret_val;
    uint8_t shift;

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_emac_init()\n");
    }
    ave_proc_add("ave_emac_init");

    if ( ave_priv->macaddr_got == 0 ) {
        ret_val = ave_get_macaddr(ave_dev->dev_addr);
        if ( ret_val < 0 ) {
            printk(KERN_ERR
               "ave: <FAIL> Taking MacAddr Failed. use Random value.\n");
            ave_dev->dev_addr[0] = 0x00;
            for(shift=1; shift<6; shift++){
                ave_dev->dev_addr[shift] = get_random_int();
            }
            ave_dev->dev_addr[6] = 0x00;    
            ave_dev->dev_addr[7] = 0x00;    
        } else {
            ave_dev->dev_addr[6] = 0x00;    
            ave_dev->dev_addr[7] = 0x00;    
        }
        ave_priv->macaddr_got = 1;
    }

    printk(KERN_INFO "    HWaddr = %02x:%02x:%02x:%02x:%02x:%02x\n",
        ave_dev->dev_addr[0], ave_dev->dev_addr[1],
        ave_dev->dev_addr[2], ave_dev->dev_addr[3],
        ave_dev->dev_addr[4], ave_dev->dev_addr[5]);

    return 0;
}

int ave_emac_start(struct net_device *netdev)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val;

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_emac_start()\n");
    }
    ave_proc_add("ave_emac_start");

#ifndef AVE_MACHINE_BIGENDIAN
    AVE_Wreg(*((long *)&netdev->dev_addr[0]), AVE_RXMAC1R);
    AVE_Wreg(*((long *)&netdev->dev_addr[4]), AVE_RXMAC2R);
#else
#endif

    reg_val = AVE_RXCR_RXEN | AVE_RXCR_FDUPEN | AVE_RXCR_DRPEN |
          (AVE_MAX_ETHFRAME & 0x7ff);

#ifdef AVE_ENABLE_PAUSE
    reg_val |= AVE_RXCR_FLOCTR;
#endif 

#ifndef AVE_MACADDR_FILTER
    AVE_Wreg(reg_val, AVE_RXCR);
#else
    AVE_Wreg(reg_val | AVE_RXCR_AFEN, AVE_RXCR);
#endif
#ifdef AVE_ENABLE_PAUSE
    AVE_Wreg(AVE_TXCR_FLOCTR, AVE_TXCR);
#else
    AVE_Wreg(0, AVE_TXCR);
#endif 

    return 0;
}

int ave_open_init(struct net_device *netdev)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val=0;
    uint32_t descriptor;
    int    ring_num,entry_num;

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_open_init()\n");
    }
    ave_proc_add("ave_open_init");

    ave_priv->phy_id = ave_phy_init(netdev);
    ave_priv->mii_if.phy_id = ave_priv->phy_id;

    reg_val = (ave_priv->tx_dnum * AVE_DESC_SIZE) << 16;
    AVE_Wreg( reg_val, AVE_TXDC);
    for ( entry_num = 0; entry_num < ave_priv->tx_dnum; entry_num++ ) {

        descriptor = AVE_TXDM + entry_num*AVE_DESC_SIZE;

        AVE_Wreg(AVE_ZERO, descriptor);      
        AVE_Wreg(AVE_ZERO, descriptor + 4);  
        if(ave_64bit_addr_discrypt == 1){ 
            AVE_Wreg(AVE_ZERO, descriptor + 8);  
        } 
    }

    for ( ring_num = 0; ring_num < AVE_RXRING_NUM; ring_num++ ) {
        reg_val = ( (ave_priv->rx_dnum[ring_num] * AVE_DESC_SIZE)<<16 | ave_priv->rx_dsa[ring_num]);
        AVE_Wreg( reg_val, AVE_RXDC0 + (ring_num * 0x4));
        for ( entry_num = 0; entry_num < ave_priv->rx_dnum[ring_num]; entry_num++ ) {
            if( ave_set_rxdesc(netdev, ring_num, entry_num) != 0 ){
                break;
            }
        }
    }

    ave_desc_switch(netdev, AVE_DESC_START);

#ifndef AVE_SINGLE_RXRING_MODE

    ave_priv->offset = 2; 

    ave_pfsel_init(netdev);

#else
    reg_val = 0xB0000000; 
    AVE_Wreg( reg_val , AVE_EMCR);
    ave_priv->offset = 2; 
#endif

    ave_priv->mii_if.advertising = ave_api_mdio_read(netdev, ave_priv->phy_id, MII_ADVERTISE);

    ave_emac_start(netdev);

    AVE_Rreg(reg_val, AVE_IIRQC);
    reg_val &= 0x0000FFFF; 
    reg_val = ( reg_val | (AVE_IIRQC_EN0) | (AVE_INTM_COUNT << 16) );
    AVE_Wreg(reg_val, AVE_IIRQC);

    reg_val = (AVE_GT1_CNTEN | AVE_INTM_COUNT_PRIO);
#ifdef AVE_DEBUG_RXPACKET_INFO
    reg_val |= (AVE_GT2_CNTEN | 0x7FFF0000);
#endif 
    AVE_Wreg(reg_val, AVE_GTICR);

    AVE_ENABLE_MININT();

    if(ave_poll_link != 0){
        AVE_Rreg(reg_val, AVE_GIMR);
        AVE_Wreg((reg_val & ~(AVE_GI_PHY)), AVE_GIMR);
    }

    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32) 
irqreturn_t ave_interrupt(int irq, void *netdev_id, struct pt_regs *regs)
#else 
irqreturn_t ave_interrupt(int irq, void *netdev_id)
#endif 
{
    uint32_t     reg_gimr_val;   
    uint32_t     reg_gisr_val;   
    struct ave_private  *ave_priv;
    struct net_device *netdev = (struct net_device *)netdev_id;
    int link_status;

#ifdef AVE_DEBUG_INTR
    printk(KERN_INFO "ave_interrupt in\n");
#endif

    if(!netdev_id) {
        return IRQ_NONE;
    }

    ave_priv = netdev_priv(netdev);

    if (netif_msg_intr(ave_priv)) {
        DBG_PRINT("%s: ave_interrupt().\n", netdev->name);
    }

    spin_lock(&(ave_priv->lock));

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    if(ave_flg_reset != 0){
        printk("ave_interrupt reset status \n");
        spin_unlock(&(ave_priv->lock));
        return IRQ_NONE;  
    }
#endif 

    AVE_Rreg(reg_gimr_val, AVE_GIMR);
    if (netif_msg_intr(ave_priv)) {
        DBG_PRINT("%s:  int_mask = 0x%08X\n", netdev->name, reg_gimr_val);
    }

    AVE_Wreg(AVE_ZERO, AVE_GIMR);

    AVE_Rreg(reg_gisr_val, AVE_GISR);
    if (netif_msg_intr(ave_priv)) {
        DBG_PRINT("%s:  int_status = 0x%08X(mask:0x%08X)\n", netdev->name,
            reg_gisr_val, reg_gimr_val);
    }

    if ( reg_gisr_val & AVE_GI_PKTBOVF ) {
        if ( netif_msg_tx_err(ave_priv) ) {
          printk(KERN_WARNING
                 "ave: AVE PACKET BUFF Overflow. Currently do nothing.\n");
        }
        AVE_Wreg(AVE_GI_PKTBOVF, AVE_GISR);
    }

    if ( reg_gisr_val & AVE_GI_TXBOVF ) {
        if ( netif_msg_tx_err(ave_priv) ) {
            printk(KERN_WARNING
                   "ave: AVE TX BUFF Overflow. Currently do nothing.\n");
        }
        AVE_Wreg(AVE_GI_TXBOVF, AVE_GISR);
    }

    if ( reg_gisr_val & AVE_GI_TXFOVF ) {
        if ( netif_msg_tx_err(ave_priv) ) {
            printk(KERN_WARNING
                   "ave: AVE TX FIFO Overflow. Currently do nothing.\n");
        }
        AVE_Wreg(AVE_GI_TXFOVF, AVE_GISR);
    }

    if ( reg_gisr_val & AVE_GI_SWFFOVF ) {
        if ( netif_msg_rx_err(ave_priv) ) {
            printk(KERN_WARNING
                   "ave: AVE SW FIFO Overflow. Currently do nothing.\n");
        }
        AVE_Wreg(AVE_GI_SWFFOVF, AVE_GISR);
    }

    if ( reg_gisr_val & AVE_GI_DMAC ) {
        if ( netif_msg_hw(ave_priv) ) {
            printk(KERN_WARNING
                   "ave: AVE DMAC Error. Currently do nothing.\n");
        }
        AVE_Wreg(AVE_GI_DMAC, AVE_GISR);
    }

    if ( reg_gisr_val & AVE_GI_RXERR ) {
         if ( netif_msg_rx_err(ave_priv) ) {
             printk(KERN_ERR "ave: Recieve packet more than buffsize. Currently do nothing.\n");
         }
         AVE_Wreg(AVE_GI_RXERR, AVE_GISR);
    }
    if ( (reg_gisr_val & reg_gimr_val) == 0 ) {
        if ( netif_msg_hw(ave_priv) ) { 
            DBG_PRINT("Interrupt by Masked Factors!\n");
        }
        goto END_PROC;
    }

    if ( (reg_gisr_val & reg_gimr_val) & AVE_GI_RXOVF ) {
        if ( netif_msg_rx_err(ave_priv) ) {
            printk(KERN_WARNING "ave: AVE RxFIFO Overflow.\n");
        }
      ave_info[0].value++;
      ave_priv->stats.rx_fifo_errors++;
#ifndef AVE_DISABLE_RXFIFO_RESET
      ave_rxf_reset(netdev);
      goto END_PROC;  
#else 
      printk(KERN_ERR "----------------------------------- \n");
      printk(KERN_ERR "ave: AVE RxFIFO Overflow occurred.\n");
      printk(KERN_ERR "But we do nothing.\n");
      printk(KERN_ERR "----------------------------------- \n");
      ave_panic(2);
#endif 
    }

    if ( (reg_gisr_val & reg_gimr_val) & AVE_GI_RXDROP ) {
        if ( netif_msg_rx_status(ave_priv) ) {
            printk(KERN_WARNING "ave: AVE Rx Drop.\n");
        }
        ave_priv->stats.rx_dropped++;
        AVE_Wreg(AVE_GI_RXDROP, AVE_GISR);
    }

    if ( (reg_gisr_val & reg_gimr_val) & AVE_GI_GT1 ) {
        ave_rx(netdev, 0, AVE_NOPRIO_RCVCNT);

#ifndef AVE_SINGLE_RXRING_MODE
        if( ave_fltinf_g[0].rcv_prio == AVE_RCV_LEVEL_HIGHEST ){
            ave_rx(netdev, 1, AVE_HIGHEST_RCVCNT);
        } else {
            ave_rx(netdev, 1, AVE_HIGHER_RCVCNT);
        }
        if( ave_fltinf_g[1].rcv_prio == AVE_RCV_LEVEL_HIGHEST ){
            ave_rx(netdev, 2, AVE_HIGHEST_RCVCNT);
        } else {
            ave_rx(netdev, 2, AVE_HIGHER_RCVCNT);
        }
        if( ave_fltinf_g[2].rcv_prio == AVE_RCV_LEVEL_HIGHEST ){
            ave_rx(netdev, 3, AVE_HIGHEST_RCVCNT);
        } else {
            ave_rx(netdev, 3, AVE_HIGHER_RCVCNT);
        }
#endif 

        if( (reg_gimr_val & AVE_GI_RXINT4) != AVE_GI_RXINT4 ){
            if( !ave_descfull_check(netdev, 4) ){
                reg_gimr_val |= (AVE_GI_RXINT4); 
            }
          ave_rx(netdev, 4, 1);
        }
        AVE_Wreg(AVE_GI_GT1, AVE_GISR);
    }

    if ( (reg_gisr_val & reg_gimr_val) & AVE_GI_RXIINT ) {
        ave_rx(netdev, 0, AVE_IIRQ_RCVCNT);
        AVE_Wreg(AVE_GI_RXIINT, AVE_GISR);
    }

    if ( (reg_gisr_val & reg_gimr_val) & AVE_GI_RXINT4 ) {
        if( ave_descfull_check(netdev, 4) ){
            reg_gimr_val &= ~(AVE_GI_RXINT4); 
        }
        AVE_Wreg(AVE_GI_RXINT4, AVE_GISR);
        ave_rx(netdev, 4, ave_priv->rx_dnum[4]);
    }

    if ( (reg_gisr_val & reg_gimr_val) & AVE_GI_TX ) { 
        ave_tx_completion(netdev);
    }

    if ( (reg_gisr_val & reg_gimr_val) & AVE_GI_PHY ) {    
        if ( netif_msg_intr(ave_priv) ) {
            DBG_PRINT("ave_interrupt phy\n");
        }

        link_status = ave_phy_intr_chkstate_negate(netdev);
        AVE_Wreg(AVE_GI_PHY, AVE_GISR);

        ave_util_linkchk_needspinlock(netdev);
    }

    if ( netif_msg_intr(ave_priv) ) {
        AVE_Rreg(reg_gisr_val, AVE_GISR);
        if ( reg_gisr_val & reg_gimr_val ) {
            DBG_PRINT("interrupt again... perhaps waken up soon. (0x%08x)\n",
              reg_gisr_val);
        }
    }

END_PROC:

    AVE_Wreg(reg_gimr_val, AVE_GIMR);

    spin_unlock(&(ave_priv->lock));

    if (netif_msg_intr(ave_priv)) {
        AVE_Rreg(reg_gisr_val, AVE_GISR);
        DBG_PRINT("%s: exiting interrupt, status=%08x.\n",
              netdev->name, reg_gisr_val);
    }

    if (netif_msg_intr(ave_priv)) {
        printk(KERN_INFO "ave_interrupt out\n");
    }

    return IRQ_HANDLED;
}

static void ave_api_get_drvinfo(struct net_device *netdev,
                 struct ethtool_drvinfo *drvinfo)
{
    strncpy(drvinfo->driver, "AVEV4", sizeof(drvinfo->driver)-1);

    strncpy(drvinfo->version, drv_version, sizeof(drvinfo->version)-1);
}

static int ave_api_get_settings(struct net_device *netdev, struct ethtool_cmd *cmd)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    unsigned long flags;

    spin_lock_irqsave(&ave_priv->lock, flags);

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    if(ave_flg_reset != 0){
        printk("ave_api_get_settings reset status \n");
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return -EAGAIN;  
    }
#endif 

    mii_ethtool_gset(&ave_priv->mii_if, cmd);
    spin_unlock_irqrestore(&ave_priv->lock, flags);
    return 0;
}

static int ave_api_set_settings(struct net_device *netdev, struct ethtool_cmd *cmd)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    int ret_val = 0;
    unsigned long flags;
    int autoselect = 0;


    spin_lock_irqsave(&ave_priv->lock, flags);

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    if(ave_flg_reset != 0){
        printk("ave_api_set_settings reset status \n");
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return -EAGAIN;  
    }
#endif 

    if( (ave_ksz8051_bugfix == 1) && (cmd->autoneg == AUTONEG_ENABLE) ){
        uint32_t reg, advert, tmp;

        advert = ave_api_mdio_read(netdev, ave_priv->phy_id, MII_ADVERTISE);
        tmp = advert & ~(ADVERTISE_ALL | ADVERTISE_100BASE4);
        if(cmd->advertising & ADVERTISED_10baseT_Half){
            tmp |= ADVERTISE_10HALF;
        }
        if(cmd->advertising & ADVERTISED_10baseT_Full){
            tmp |= ADVERTISE_10FULL;
        }
        if(cmd->advertising & ADVERTISED_100baseT_Half){
            tmp |= ADVERTISE_100HALF;
        }
        if(cmd->advertising & ADVERTISED_100baseT_Full){
            tmp |= ADVERTISE_100FULL;
        }

        if(advert != tmp) {
            ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_ADVERTISE, tmp);
            ave_priv->mii_if.advertising = tmp;
        }

        reg = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, MII_BMCR);
        reg &= ~BMCR_ANENABLE;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_BMCR, (int)reg);
        udelay(1000);
        reg |= BMCR_ANENABLE;
        ave_priv->mii_if.mdio_write(netdev, ave_priv->phy_id, MII_BMCR, (int)reg);
    }
    else{

        if(ave_use_omniphy == 1){

            if(cmd->autoneg == AUTONEG_ENABLE){
                autoselect = 1;
            } else {
                autoselect = 0;
            }
            ave_phy_set_afe_initialize_param(netdev, autoselect);
            if(autoselect == 0){
                if(cmd->speed & SPEED_100){
                    ave_phy_set_afe_linkspeed_param(netdev, 1);
                } else if(cmd->speed & SPEED_10){
                    ave_phy_set_afe_linkspeed_param(netdev, 0);
                }
            }
        } 

        ret_val = mii_ethtool_sset(&ave_priv->mii_if, cmd);
    }

    spin_unlock_irqrestore(&ave_priv->lock, flags);
    return ret_val;
}

static int ave_api_nway_reset(struct net_device *netdev)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    int ret_val;
    unsigned long flags;

    spin_lock_irqsave(&ave_priv->lock, flags);

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    if(ave_flg_reset != 0){
        printk("ave_api_nway_reset reset status \n");
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return 0;  
    }
#endif 

    ret_val = mii_nway_restart(&ave_priv->mii_if);
    spin_unlock_irqrestore(&ave_priv->lock, flags);
    return ret_val;
}

static u32 ave_api_get_link(struct net_device *netdev)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    int ret_val;
    unsigned long flags;

    spin_lock_irqsave(&ave_priv->lock, flags);

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    if(ave_flg_reset != 0){
        printk("ave_api_get_link reset status \n");
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return 0;  
    }
#endif 

    ret_val = ave_util_mii_link_ok(netdev);
    spin_unlock_irqrestore(&ave_priv->lock, flags);
    return ret_val;
}

static u32 ave_api_get_msglevel(struct net_device *netdev)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    return ave_priv->msg_enable;
}

static void ave_api_set_msglevel(struct net_device *netdev, u32 val)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    ave_priv->msg_enable = val;
    return;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,20)
static u32 ave_api_get_rx_csum(struct net_device *netdev)
{
#ifndef AVE_SINGLE_RXRING_MODE
    uint32_t reg_val=0;
#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    unsigned long flags;
#endif 

    struct ave_private *ave_priv = netdev_priv(netdev);

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    spin_lock_irqsave(&ave_priv->lock, flags);
    if(ave_flg_reset != 0){
        printk("ave_api_get_rx_csum reset status \n");
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return 0;  
    }
#endif 

    AVE_Rreg(reg_val, AVE_CFGR);

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    spin_unlock_irqrestore(&ave_priv->lock, flags);
#endif 

    if( (reg_val & AVE_CFGR_TESTRESERVE) == AVE_CFGR_TESTRESERVE ){
        return 1;
    } else {
        return 0;
    }
#else 
    return 0;
#endif 
}

static u32 ave_api_get_tx_csum(struct net_device *netdev)
{
#ifndef AVE_SINGLE_RXRING_MODE
    uint32_t reg_val=0;
#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    unsigned long flags;
#endif 

    struct ave_private *ave_priv = netdev_priv(netdev);

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    spin_lock_irqsave(&ave_priv->lock, flags);
    if(ave_flg_reset != 0){
        printk("ave_api_get_tx_csum reset status \n");
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return 0;  
    }
#endif 

    AVE_Rreg(reg_val, AVE_CFGR);

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    spin_unlock_irqrestore(&ave_priv->lock, flags);
#endif 

    if( (reg_val & AVE_CFGR_TESTRESERVE) == AVE_CFGR_TESTRESERVE ){
        return 1;
    } else {
        return 0;
    }
#else 
    return 0;
#endif 
}
#endif 

static void ave_api_get_wol(struct net_device *netdev, struct ethtool_wolinfo *wolinfo)
{
#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    struct ave_private *ave_priv = netdev_priv(netdev);
    unsigned long flags;

    spin_lock_irqsave(&ave_priv->lock, flags);
    if(ave_flg_reset != 0){
        printk("ave_api_get_wol reset status \n");
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return;  
    }
#endif 

    ave_phy_get_wol(netdev, wolinfo);

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    spin_unlock_irqrestore(&ave_priv->lock, flags);
#endif 

    return;
}

static int ave_api_set_wol(struct net_device *netdev, struct ethtool_wolinfo *wolinfo)
{
    int ret_val = 0;

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    struct ave_private *ave_priv = netdev_priv(netdev);
    unsigned long flags;

    spin_lock_irqsave(&ave_priv->lock, flags);
    if(ave_flg_reset != 0){
        printk("ave_api_set_wol reset status \n");
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return 0;  
    }
#endif 

    ret_val = ave_phy_set_wol(netdev, wolinfo);

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    spin_unlock_irqrestore(&ave_priv->lock, flags);
#endif 

    return ret_val;
}

static struct ethtool_ops ave_ethtool_ops = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,20) 
    .get_drvinfo  = ave_api_get_drvinfo,
    .get_settings = ave_api_get_settings,
    .set_settings = ave_api_set_settings,
    .nway_reset   = ave_api_nway_reset,
    .get_link     = ave_api_get_link,
    .get_msglevel = ave_api_get_msglevel,
    .set_msglevel = ave_api_set_msglevel,
    .get_rx_csum  = ave_api_get_rx_csum,
    .get_tx_csum  = ave_api_get_tx_csum,
    .get_wol      = ave_api_get_wol,
    .set_wol      = ave_api_set_wol,
#else
    .get_drvinfo  = ave_api_get_drvinfo,
    .get_settings = ave_api_get_settings,
    .set_settings = ave_api_set_settings,
    .nway_reset   = ave_api_nway_reset,
    .get_link     = ave_api_get_link,
    .get_msglevel = ave_api_get_msglevel,
    .set_msglevel = ave_api_set_msglevel,
    .get_wol      = ave_api_get_wol,
    .set_wol      = ave_api_set_wol,
#endif
};

int ave_api_open(struct net_device *netdev)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    unsigned long flags;

    if ( netif_msg_ifup(ave_priv) ) {
        DBG_PRINT("ave_api_open()\n");
    }
    ave_proc_add("ave_api_open");

#if defined(AVE_TXTHREAD_BREAK)
    ave_tx_thread_break_flg = 0;
    ave_thread = kthread_run(ave_tx_restart, netdev, "ave_tx_restart");
    if (ave_thread == NULL) {
        printk(KERN_ERR "ave: <FAIL> kthread_run() Failed");
        free_irq(netdev->irq, netdev);
        return -EAGAIN;
    }
#endif  

    if( ave_init_ring(netdev) ){
        printk(KERN_ERR "ave: <FAIL> ave_init_ring() Failed");
        free_irq(netdev->irq, netdev);
        return -EAGAIN;
    }

    spin_lock_irqsave(&ave_priv->lock, flags);
    ave_open_init(netdev);

#if 0
    if(ave_use_omniphy == 1){
        printk(KERN_ERR "yk-dbg ave: OMNIPHY tentative link 100M-Half \n");
        {
            int check;
            uint32_t reg_val;

            ave_priv = netdev_priv(netdev);
            reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, MII_BMCR);
            reg_val |= BMCR_RESET;
            ave_api_mdio_write(netdev, ave_priv->phy_id, MII_BMCR, reg_val);
            for(check = 0; check < 50; check++) {
                mdelay(1); 
                reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, MII_BMCR);
                if((reg_val & BMCR_RESET) == 0) {
                    break;
                }
            }

            if(check >= 50) {
                reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, MII_BMCR);
                printk(KERN_WARNING "ave: BCMR [0x%08x] is a PHY reset state.\n", reg_val);
            }
        }

        ave_api_mdio_write(netdev, ave_priv->phy_id, MII_BMCR, 0x2000);
    } 
#endif

    spin_unlock_irqrestore(&(ave_priv->lock), flags);

#if 1
    if(request_irq(netdev->irq, ave_interrupt, SA_SHIRQ,
           netdev->name, netdev) !=0){
        printk(KERN_ERR "ave: <FAIL> request_irq() Failed");
        return -EAGAIN;
    }
#endif

    netif_start_queue(netdev);

    spin_lock_irqsave(&ave_priv->lock, flags);
    ave_util_mii_check_link(netdev);
    spin_unlock_irqrestore(&(ave_priv->lock), flags);

    if (netif_msg_ifup(ave_priv)) {
        DBG_PRINT("%s: Done ave_api_open().\n", netdev->name);
    }

    ave_priv->timer.expires = RUN_AT(AVE_MNTR_TIME);
    ave_priv->timer.data = (unsigned long)netdev;
    ave_priv->timer.function = &ave_timer;  
    add_timer(&ave_priv->timer);

#ifdef MODULE
    MOD_INC_USE_COUNT;
#endif 

    return 0;
}

int ave_api_stop(struct net_device *netdev)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val;
    unsigned long flags;
    uint32_t wolopts;

    if ( netif_msg_ifdown(ave_priv) ) {
        DBG_PRINT("ave_api_stop()\n");
    }
    ave_proc_add("ave_api_stop");

    AVE_Rreg(reg_val, AVE_GIMR);
    AVE_Wreg((reg_val & ~(AVE_GI_RX)), AVE_GIMR);

    del_timer_sync(&ave_priv->timer);

    netif_stop_queue(netdev);

    if (netif_msg_ifdown(ave_priv)) {
        DBG_PRINT("%s: Shutting down AVE.\n", netdev->name);
    }

    disable_irq(netdev->irq);

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 

    spin_lock_irqsave(&ave_priv->lock, flags);
    reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, MII_BMCR);

    ave_flg_reset = 1;  
    spin_unlock_irqrestore(&ave_priv->lock, flags);
    wolopts = ave_get_wol_setting(netdev);

    ave_global_reset(netdev);   

    ave_set_wol_setting(netdev, wolopts);
    spin_lock_irqsave(&ave_priv->lock, flags);
    ave_flg_reset = 0;  

    ave_api_mdio_write(netdev, ave_priv->phy_id, MII_BMCR, reg_val);
    spin_unlock_irqrestore(&ave_priv->lock, flags);

#else 

    spin_lock_irqsave(&ave_priv->lock, flags);
    reg_val = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, MII_BMCR);

    wolopts = ave_get_wol_setting(netdev);

    ave_global_reset(netdev);

    ave_set_wol_setting(netdev, wolopts);

    ave_api_mdio_write(netdev, ave_priv->phy_id, MII_BMCR, reg_val);
    spin_unlock_irqrestore(&ave_priv->lock, flags);

#endif 

    ave_term_ring(netdev);

    AVE_DISABLE_INT();

    free_irq(netdev->irq, netdev);

#ifdef MODULE
    MOD_DEC_USE_COUNT;
#endif 

#if defined(AVE_TXTHREAD_BREAK)
    ave_tx_thread_break_flg = 1;
    kthread_stop(ave_thread);
#endif 

    return 0;
}

int ave_api_start_xmit(struct sk_buff *skb, struct net_device *netdev)
{
    struct ave_private  *ave_priv = netdev_priv(netdev);
    unsigned long flags;
    int   position;
    int   tx_length;
    uint64_t reg_val_64;
    uint32_t reg_val;
    uint32_t before_tx_procidx, after_tx_procidx;
    uint32_t tx_proc_idx,tx_done_idx,tx_desc_num,tx_free_num;
    uint32_t descriptor;
    unsigned char *buffptr = NULL; 
    int   offset = 0;

    if ( netif_msg_tx_queued(ave_priv) ) {
        DBG_PRINT("ave_api_start_xmit()\n");
    }
    ave_proc_add("ave_api_start_xmit");

#if defined(AVE_TXTHREAD_BREAK)
    if ( need_resched() && ( current->pid == ave_tx_thread_break_pid[smp_processor_id()] ) ) {
        if ( ++ave_tx_thread_break_cnt[smp_processor_id()] >= AVE_MIN_TX ) { 
            netif_stop_queue(netdev);
            ave_tx_thread_break_cnt[smp_processor_id()] = 0;
            ave_tx_thread_break_flg = 1;
            wake_up(&ave_wq);
            return NETDEV_TX_BUSY;
        }
    }
    else {
        ave_tx_thread_break_cnt[smp_processor_id()] = 1;
        ave_tx_thread_break_pid[smp_processor_id()] = current->pid;
    }
#endif  

    spin_lock_irqsave(&ave_priv->lock, flags);

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    if(ave_flg_reset != 0){
        printk("ave_api_start_xmit reset status \n");
        dev_kfree_skb_any(skb);
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return NETDEV_TX_OK;  
    }
#endif 

#if defined(AVE_ENABLE_TXTIMINGCTL)
    AVE_Rreg(reg_val, AVE_DESCC);
    if ( ! (reg_val & AVE_DESCC_TDS) ) {
        netif_stop_queue(netdev);
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return(NETDEV_TX_BUSY);
    }
#endif  

    if (skb->len > ETH_FRAME_LEN) {
        dev_kfree_skb_any(skb);
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return NETDEV_TX_OK;
    }

    tx_free_num = ave_get_txfreenum(netdev);
    if( (tx_free_num < 2) || (ave_priv->tx_dnum - tx_free_num > AVE_TXFREE_THRESHOLD) ){
        ave_tx_completion(netdev);
        tx_free_num = ave_get_txfreenum(netdev);
        if( tx_free_num < 2 ){
            if ( netif_msg_tx_queued(ave_priv) ) {
                DBG_PRINT("%s: stop_queue : there is no tx free entry num(%d).\n", netdev->name,tx_free_num);
            }
            netif_stop_queue(netdev);
        }
    } else {
    }

    if(tx_free_num < 1){
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return (NETDEV_TX_BUSY);
    }

    tx_proc_idx = ave_priv->tx_proc_idx;
    tx_done_idx = ave_priv->tx_done_idx;
    tx_desc_num = ave_priv->tx_dnum;

    ave_priv->tx_skbs[tx_proc_idx] = skb;
    if ( netif_msg_tx_queued(ave_priv) ) {
        DBG_PRINT("%s: tx proc_idx = %d, done_idx = %d, desc_num = %d, free_num = %d, skb->len = 0x%08X\n",
                   netdev->name, (int)tx_proc_idx, (int)tx_done_idx, (int)tx_desc_num, (int)tx_free_num, (int)skb->len);
    }

    offset = ave_priv->offset;

    if( skb->len < ETH_ZLEN ) {   
        memset((ave_priv->tx_short[tx_proc_idx] + offset + skb->len), 0x00, (ETH_ZLEN - skb->len));
        memcpy(ave_priv->tx_short[tx_proc_idx]+offset, skb->data, skb->len);
        buffptr = ave_priv->tx_short[tx_proc_idx];
        tx_length = ETH_ZLEN;
    } else {
        buffptr = skb->data-offset;
        tx_length = skb->len;
    }

#if 0
{
  unsigned long looo;
  printk("*** tx pkt dump (%d) ***",skb->len);
  for(looo=0;looo<skb->len; looo++){
    if(looo%16 ==0){
      printk("\n");
    }
    printk("%02x ", skb->data[looo]);
  }
  printk("\n\n");
}
#endif

    if ( netif_msg_pktdata(ave_priv) ) {
        uint32_t *pbuff;
        uint32_t workdata;

        DBG_PRINT("buff address = 0x%p\n",buffptr);

        pbuff = (uint32_t *)(buffptr);
        for(position = 0; position < (tx_length + offset) / 4; position++){
            workdata = *(pbuff + position);
            ave_endian_change(&workdata);
            DBG_PRINT("tx[%03d] = 0x%08X\n", position, (int)workdata);
        }
        if((tx_length + offset) % 4){
            workdata = *(pbuff + position);
            ave_endian_change(&workdata);
            DBG_PRINT("tx[%03d] = 0x%08X\n", position, (int)workdata);
        }
    }

    descriptor = AVE_TXDM + (tx_proc_idx*AVE_DESC_SIZE);

    if(ave_64bit_addr_discrypt == 1){ 
        reg_val_64 = ave_cache_control(netdev, (uint64_t)buffptr,
                                 (uint32_t)(tx_length+offset), DMA_TO_DEVICE);
        if(reg_val_64 == -ENOMEM){
            ave_priv->tx_skbs_dma[tx_proc_idx] = 0;
            reg_val_64 = virt_to_phys(buffptr);
            printk("yk-dbg (TX) reg_val_modify=%016llx buffptr=%p \n", reg_val_64, buffptr);
        }
        else{
            ave_priv->tx_skbs_dma[tx_proc_idx] = reg_val_64;
            ave_priv->tx_skbs_dmalen[tx_proc_idx] = (uint32_t)(tx_length+offset);
        }

        reg_val_64 += AVE_DDRMEM_SHIFT;  
        reg_val_64 += offset;     
        AVE_Wreg((uint32_t)(reg_val_64 >> 32), descriptor + 8);  
        AVE_Wreg((uint32_t)(reg_val_64 & 0xFFFFFFFF), descriptor + 4);  

    } else { 
        reg_val = ave_cache_control(netdev, (uint64_t)buffptr,
                                (uint32_t)(tx_length+offset), DMA_TO_DEVICE);
        if(reg_val == -ENOMEM){
            ave_priv->tx_skbs_dma[tx_proc_idx] = 0;
            reg_val = virt_to_phys(buffptr);
            printk("yk-dbg (TX) reg_val_modify=%08x buffptr=%p \n", reg_val, buffptr);
        }
        else{
            ave_priv->tx_skbs_dma[tx_proc_idx] = reg_val;
            ave_priv->tx_skbs_dmalen[tx_proc_idx] = (uint32_t)(tx_length+offset);
        }

        reg_val += AVE_DDRMEM_SHIFT;  
        reg_val += offset;     
        AVE_Wreg(reg_val, descriptor + 4);

    } 

    reg_val = ( AVE_STS_OWN | tx_length );

    before_tx_procidx = ave_priv->tx_proc_idx;
    after_tx_procidx  = before_tx_procidx + 1;      
    if( (after_tx_procidx / AVE_FORCE_TXINTCNT) > (before_tx_procidx / AVE_FORCE_TXINTCNT) ){
        reg_val |= AVE_STS_INTR;    
    }
    if(after_tx_procidx >= tx_desc_num){
        reg_val |= AVE_STS_INTR;    
    }

    if ( netif_queue_stopped(netdev) ){
        reg_val |= AVE_STS_INTR;
    }

#ifndef AVE_SINGLE_RXRING_MODE
#ifdef AVE_ENABLE_CSUM_OFFLOAD
    if (skb->ip_summed != CHECKSUM_HW) {
        reg_val |= AVE_STS_NOCSUM;
    }
#endif 
#endif

    reg_val |= (AVE_STS_1ST | AVE_STS_LAST);
    AVE_Wreg(reg_val, descriptor); 

    ave_priv->tx_proc_idx = (tx_proc_idx + 1)%tx_desc_num;

    spin_unlock_irqrestore(&ave_priv->lock, flags);

    netdev->trans_start = jiffies;

    return NETDEV_TX_OK;

}

#ifdef AVE_ENABLE_TSO 
int ave_api_start_xmit_tso(struct sk_buff *skb, struct net_device *netdev)
{
    struct ave_private  *ave_priv = netdev_priv(netdev);
    unsigned long flags;
    int   tx_length;
    uint32_t reg_val, cmdsts, memblk_cnt;
    uint32_t before_tx_procidx, after_tx_procidx;
    uint32_t tx_proc_idx,tx_done_idx,tx_desc_num,tx_free_num;
    uint32_t descriptor;
    unsigned char *buffptr = NULL; 
    int   offset = 0;
    uint32_t skb_mss, reg_mssr_val;
    unsigned long shift;
    unsigned long mem_num;

    if ( netif_msg_tx_queued(ave_priv) ) {
        DBG_PRINT("ave_api_start_xmit_tso()\n");
    }
    ave_proc_add("ave_api_start_xmit_tso");

#if defined(AVE_TXTHREAD_BREAK)
    if ( need_resched() && ( current->pid == ave_tx_thread_break_pid[smp_processor_id()] ) ) {
        if ( ++ave_tx_thread_break_cnt[smp_processor_id()] >= AVE_MIN_TX ) { 
            netif_stop_queue(netdev);
            ave_tx_thread_break_cnt[smp_processor_id()] = 0;
            ave_tx_thread_break_flg = 1;
            wake_up(&ave_wq);
            return NETDEV_TX_BUSY;
        }
    }
    else {
        ave_tx_thread_break_cnt[smp_processor_id()] = 1;
        ave_tx_thread_break_pid[smp_processor_id()] = current->pid;
    }
#endif  

    spin_lock_irqsave(&ave_priv->lock, flags);

#if defined(AVE_ENABLE_TXTIMINGCTL)
    AVE_Rreg(reg_val, AVE_DESCC);
    if ( ! (reg_val & AVE_DESCC_TDS) ) {
        netif_stop_queue(netdev);
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return(NETDEV_TX_BUSY);
    }
#endif  

    if (skb->len < 8) {        
        dev_kfree_skb_any(skb);
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return NETDEV_TX_OK;
    }

    memblk_cnt = (skb_shinfo(skb)->nr_frags + 1);

    tx_free_num = ave_get_txfreenum(netdev);
    if( (tx_free_num < (memblk_cnt+1)) || ((ave_priv->tx_dnum - tx_free_num) > AVE_TXFREE_THRESHOLD) ){
        ave_tx_completion(netdev);              

        tx_free_num = ave_get_txfreenum(netdev);   
        if( tx_free_num < (memblk_cnt+1) ){
            if ( netif_msg_tx_queued(ave_priv) ) {
                DBG_PRINT("%s: stop_queue : there is no tx free entry num(%ld).\n", netdev->name,tx_free_num);
            }
            netif_stop_queue(netdev);   
        }
    }

    if ( netif_msg_tx_queued(ave_priv) ) {
        static uint32_t tso_pkts, total_pkts;
        total_pkts++;
        if(memblk_cnt > 1) {
            tso_pkts++;
            printk(KERN_INFO "%s:%d: memblk_cnt=%ld free_num=%ld (TSO=%ld/%ld len=%d data_len=%d mss=%d)\n",
                __func__, __LINE__,
                memblk_cnt, tx_free_num, tso_pkts, total_pkts,
                skb->len, skb->data_len,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32) 
                skb_shinfo(skb)->tso_size
#else 
                skb_shinfo(skb)->gso_size
#endif 
            );
        }
    }

    if( tx_free_num < memblk_cnt ){
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return (NETDEV_TX_BUSY);
    }

#if 0
    printk("DEBUG: memblk=%d \n", memblk_cnt);
#endif

    tx_proc_idx = ave_priv->tx_proc_idx;
    tx_done_idx = ave_priv->tx_done_idx;
    tx_desc_num = ave_priv->tx_dnum;

    if ( netif_msg_tx_queued(ave_priv) ) {
        DBG_PRINT("%s: tx proc_idx = %d, done_idx = %d, desc_num = %d, free_num = %d, skb->len = 0x%08X\n",
                   netdev->name, (int)tx_proc_idx, (int)tx_done_idx, (int)tx_desc_num, (int)tx_free_num, (int)skb->len);
    }

    cmdsts = AVE_STS_OWN;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32) 
    skb_mss = (uint32_t) (skb_shinfo(skb)->tso_size);
#else 
    skb_mss = (uint32_t) (skb_shinfo(skb)->gso_size);
#endif 
    if( (skb_mss > 0) && (skb->len > skb_mss) ) {
        cmdsts |= AVE_STS_TSO;
    }

    AVE_Rreg(reg_mssr_val, AVE_MSSR);
    if( (skb_mss > 0) && (reg_mssr_val > skb_mss) ){
        AVE_Wreg(skb_mss, AVE_MSSR);
    }

    before_tx_procidx = ave_priv->tx_proc_idx;
    after_tx_procidx  = before_tx_procidx + memblk_cnt;
    if( (after_tx_procidx / AVE_FORCE_TXINTCNT) > (before_tx_procidx / AVE_FORCE_TXINTCNT) ){
        cmdsts |= AVE_STS_INTR;    
    }
    if(after_tx_procidx >= tx_desc_num){
        cmdsts |= AVE_STS_INTR;    
    }
    if ( netif_queue_stopped(netdev) ){
        cmdsts |= AVE_STS_INTR;    
    }

    if (skb->ip_summed != CHECKSUM_HW) {
        if( (cmdsts & AVE_STS_TSO) != 0 ){
            printk(KERN_CRIT "**** TSO on but checksum off");  
        }
        else{
            cmdsts |= AVE_STS_NOCSUM;    
        }
    }

    for(shift=0 ; shift < memblk_cnt; shift++){

        tx_proc_idx = ave_priv->tx_proc_idx;

        if(shift == 0){      
            offset = ave_priv->offset;
            buffptr = skb->data-offset;
            tx_length = skb->len;
            if(memblk_cnt > 1){
                tx_length = skb_headlen(skb);
            }

            ave_priv->tx_skbs[tx_proc_idx] = skb;   
        }
        else{                     
            offset = 0;    
            buffptr = ((unsigned char*)page_address(skb_shinfo(skb)->frags[shift-1].page.p))
                       + skb_shinfo(skb)->frags[shift-1].page_offset;
            tx_length = skb_shinfo(skb)->frags[shift-1].size;

            ave_priv->tx_skbs[tx_proc_idx] = NULL;  
        }

        if ( netif_msg_pktdata(ave_priv) ) {
            uint32_t *pbuff;
            uint32_t workdata;

            if(ave_64bit_addr_discrypt == 1){ 
                DBG_PRINT("buff address = 0x%016lx\n",(unit64_t)buffptr);
            } else { 
                DBG_PRINT("buff address = 0x%08lx\n",(uint32_t)buffptr);
            } 

            pbuff = (uint32_t *)(buffptr);
            for(mem_num = 0; mem_num < (tx_length + offset) / 4; mem_num++){
                workdata = *(pbuff + mem_num);
                ave_endian_change(&workdata);
                DBG_PRINT("tx[%03d] = 0x%08X\n", (int)mem_num, (int)workdata);
            }
            if((tx_length + offset) % 4){
                workdata = *(pbuf + mem_num);
                ave_endian_change(&workdata);
                DBG_PRINT("tx[%03d] = 0x%08X\n", (int)mem_num, (int)workdata);
            }
        }

        descriptor = AVE_TXDM + (tx_proc_idx*AVE_DESC_SIZE);

        if(ave_64bit_addr_discrypt == 1){ 

#if defined(CONFIG_ARM)
            reg_val_64 = ave_cache_control(netdev, (uint64_t)buffptr,
                                (uint32_t)(tx_length+offset), DMA_TO_DEVICE);
            if(reg_val_64 == NULL){
                reg_val_64 = virt_to_phys(buffptr);
            }
#else 
            ave_ave_cache_control(netdev, (uint64_t)buffptr,
                                (uint32_t)(tx_length+offset), DMA_TO_DEVICE);
            reg_val_64 = (uint64_t)buffptr;
            reg_val_64 += AVE_DDRMEM_SHIFT;  
#endif 

#ifdef AVE_ENABLE_V4CODE
            reg_val_64 += offset;     
#endif 
            AVE_Wreg((uint32_t)(reg_val_64 >> 32), descriptor + 8);  
            AVE_Wreg((uint32_t)(reg_val_64 & 0xFFFFFFFF), descriptor + 4);  
            AVE_Wreg(reg_val, (descriptor+4) );  

        } else { 

#if defined(CONFIG_ARM)
            reg_val = ave_cache_control(netdev, (uint64_t)buffptr,
                               (uint32_t)(tx_length+offset), DMA_TO_DEVICE);
            if(reg_val == NULL){
                reg_val = virt_to_phys(buffptr);
            }
#else 
            ave_cache_control(netdev, (uint64_t)buffptr,
                               (uint32_t)(tx_length+offset), DMA_TO_DEVICE);
            reg_val = (uint32_t)buffptr;
            reg_val += AVE_DDRMEM_SHIFT;  
#endif 

#ifdef AVE_ENABLE_V4CODE
            reg_val += offset;     
#endif 
            AVE_Wreg(reg_val, (descriptor+4) );  

        } 

        #if 1
        if ( netif_msg_tx_queued(ave_priv) ) {
            if(memblk_cnt != 1){
                printk(KERN_INFO "proc(%ld) desc(%08lx) adr=%08lx ", tx_proc_idx,descriptor, reg_val);
            }
        }
        #endif

        reg_val = ( cmdsts | tx_length );
        if(shift == 0){
            reg_val |= AVE_STS_1ST;
        }
        if(shift == (memblk_cnt-1)){
            reg_val |= AVE_STS_LAST;
        }
        AVE_Wreg(reg_val, descriptor); 
        #if 1
        if ( netif_msg_tx_queued(ave_priv) ) {
            if(memblk_cnt != 1){
                printk("cmdsts=%08lx \n", reg_val);
            }
        }
        #endif

        ave_priv->tx_proc_idx = (tx_proc_idx + 1)%tx_desc_num;

    }

    spin_unlock_irqrestore(&ave_priv->lock, flags);

    netdev->trans_start = jiffies;

    return NETDEV_TX_OK;

}
#endif 

int ave_api_ioctl(struct net_device *netdev, struct ifreq *if_req, int command)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    struct mii_ioctl_data *mii_ioctl = if_mii(if_req);
    unsigned long flags;
    uint32_t *if_data = (uint32_t *) & if_req->ifr_data;

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_api_ioctl()\n");
    }
    ave_proc_add("ave_api_ioctl");

    spin_lock_irqsave(&ave_priv->lock, flags);

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    if(ave_flg_reset != 0){
        printk("ave_api_ioctl reset status \n");
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return -EAGAIN;  
    }
#endif 

    switch(command) {

    case SIOCDEVPRIVATE:
        *if_data = ave_util_mii_link_ok(netdev);
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return 0;
    case SIOCGMIIPHY:     
        (if_mii(if_req))->phy_id = ave_priv->phy_id;
    case SIOCGMIIREG:     
        mii_ioctl->val_out = ave_api_mdio_read(netdev, mii_ioctl->phy_id &
                          ave_priv->mii_if.phy_id_mask,
                          mii_ioctl->reg_num &
                          ave_priv->mii_if.reg_num_mask);
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return 0;
    case SIOCSMIIREG:     
        if (!capable(CAP_NET_ADMIN)) {
            spin_unlock_irqrestore(&ave_priv->lock, flags);
            return -EPERM;
        }
        ave_api_mdio_write(netdev, mii_ioctl->phy_id, mii_ioctl->reg_num, mii_ioctl->val_in);
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return 0;
    default:
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return -EOPNOTSUPP;
    }

    return(0);
}

void ave_api_multicast(struct net_device *netdev)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    unsigned long flags;
#ifndef AVE_SINGLE_RXRING_MODE 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,8) 
    struct netdev_hw_addr *hw_adr;
    int mc_cnt = netdev_mc_count(netdev);
#else  
    struct dev_mc_list *mc_list;
    int mc_cnt = netdev->mc_count;
#endif 
    int count;
    unsigned char v4multi_macadr[6] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
    unsigned char v6multi_macadr[6] = {0x33, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif 
#ifdef  AVE_MACADDR_FILTER
    uint32_t reg_val;
#endif

    if (netif_msg_rx_status(ave_priv)) {
        DBG_PRINT("%s: ave_api_multicast %04x -> %04x\n",
              netdev->name, ave_priv->flags, netdev->flags);
    }
    ave_proc_add("ave_api_multicast");


    ave_priv->flags = netdev->flags;

    spin_lock_irqsave(&ave_priv->lock, flags);

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    if(ave_flg_reset != 0){
        printk("ave_api_multicast reset status \n");
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return;  
    }
#endif 

    if (netdev->flags & IFF_PROMISC) {
        if (netif_msg_rx_status(ave_priv)) {
            DBG_PRINT("Promiscas Mode\n");
        }
#ifndef AVE_MACADDR_FILTER
        ave_pfsel_promisc_set(netdev, AVE_DEFAULT_FILTER, AVE_PFSEL_RING0);
#else
        AVE_Rreg(reg_val, AVE_RXCR);
        AVE_Wreg((reg_val & ~(AVE_RXCR_AFEN)), AVE_RXCR);
#endif
    } else {
        if (netif_msg_rx_status(ave_priv)) {
            DBG_PRINT("non-Promiscas Mode\n");
        }
#ifndef AVE_MACADDR_FILTER
        ave_pfsel_switch(netdev, AVE_DEFAULT_FILTER, AVE_FILTER_STOP);
#else
        AVE_Rreg(reg_val, AVE_RXCR);
        AVE_Wreg((reg_val | AVE_RXCR_AFEN), AVE_RXCR);
#endif
    }

#ifndef AVE_SINGLE_RXRING_MODE
    if(netdev->flags & IFF_ALLMULTI || mc_cnt > AVE_MULTICAST_SIZE){
        if (netif_msg_rx_status(ave_priv)) {
            DBG_PRINT("Setting all v4 v6 multicast packet\n");
        }
        ave_pfsel_macaddr_set(netdev, AVE_PF_RESERVE_SIZE, v4multi_macadr, 1,
                                AVE_PFSEL_RING0);
        ave_pfsel_macaddr_set(netdev, AVE_PF_RESERVE_SIZE+1, v6multi_macadr, 1,
                                AVE_PFSEL_RING0);
    } else {
        for (count = 0; count < AVE_MULTICAST_SIZE; count++){
            ave_pfsel_switch(netdev, count+AVE_PF_RESERVE_SIZE, AVE_FILTER_STOP);
        }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,8) 
        if(mc_cnt != 0){
            count=0;
            netdev_for_each_mc_addr(hw_adr, netdev) {
                if(count == mc_cnt)
                    break;
                if (netif_msg_rx_status(ave_priv)) {
                    DBG_PRINT("mc_count %d in entry %d. 0x%02x%02x%02x%02x%02x%02x\n",
                              mc_cnt, count+1,
                              (char)hw_adr->addr[0],
                              (char)hw_adr->addr[1],
                              (char)hw_adr->addr[2],
                              (char)hw_adr->addr[3],
                              (char)hw_adr->addr[4],
                              (char)hw_adr->addr[5]);
                }
                ave_pfsel_macaddr_set(netdev, count+AVE_PF_RESERVE_SIZE, hw_adr->addr, 6,
                                       AVE_PFSEL_RING0);
                count++;
            }
        }
#else  
        if(netdev->mc_count != 0){
            for (count = 0, mc_list = netdev->mc_list;
                 count < netdev->mc_count;
                 count++, mc_list = mc_list->next) {
                if (netif_msg_rx_status(ave_priv)) {
                    DBG_PRINT("mc_count %d in entry %d. 0x%02x%02x%02x%02x%02x%02x\n",
                              netdev->mc_count, count+1,
                              (char)mc_list->dmi_addr[0],
                              (char)mc_list->dmi_addr[1],
                              (char)mc_list->dmi_addr[2],
                              (char)mc_list->dmi_addr[3],
                              (char)mc_list->dmi_addr[4],
                              (char)mc_list->dmi_addr[5]);
                }
                ave_pfsel_macaddr_set(netdev, count+AVE_PF_RESERVE_SIZE, mc_list->dmi_addr, 6,
                                       AVE_PFSEL_RING0);
            }
        }
#endif 
    }
#endif 

    spin_unlock_irqrestore(&ave_priv->lock, flags);

}

struct net_device_stats *ave_api_stats(struct net_device *netdev)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_val;
    uint32_t drop_num=0;
#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    unsigned long flags;
#endif 

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_api_stats()\n");
    }
    ave_proc_add("ave_api_stats");

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    spin_lock_irqsave(&ave_priv->lock, flags);
    if(ave_flg_reset != 0){
        printk("ave_api_stats reset status \n");
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return &ave_priv->stats;  
    }
#endif 

    AVE_Rreg(reg_val, AVE_BFCR);
    ave_priv->stats.rx_errors = reg_val;

    AVE_Rreg(reg_val, AVE_RX0OVFFC);
    drop_num += reg_val;
    AVE_Rreg(reg_val, AVE_RX1OVFFC);
    drop_num += reg_val;
    AVE_Rreg(reg_val, AVE_RX2OVFFC);
    drop_num += reg_val;
    AVE_Rreg(reg_val, AVE_RX3OVFFC);
    drop_num += reg_val;
    AVE_Rreg(reg_val, AVE_RX4OVFFC);
    drop_num += reg_val;
    AVE_Rreg(reg_val, AVE_SN5FC);
    drop_num += reg_val;
    AVE_Rreg(reg_val, AVE_SN6FC);
    drop_num += reg_val;
    AVE_Rreg(reg_val, AVE_SN7FC);
    drop_num += reg_val;
    ave_priv->stats.rx_dropped = drop_num; 

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    spin_unlock_irqrestore(&ave_priv->lock, flags);
#endif 

    return &ave_priv->stats;
}

static int ave_api_change_mtu(struct net_device *netdev, int set_mtu)
{
    struct ave_private *ave_priv = NULL;
#ifdef  AVE_RX_MTU_CHANGE
    uint32_t reg_val;
#endif
#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    unsigned long flags;
#endif 

    ave_proc_add("ave_api_change_mtu");
    if ( netdev != NULL ) {
        ave_priv = netdev_priv(netdev);
        if ( netif_msg_drv(ave_priv) ) {
            DBG_PRINT("ave_api_change_mtu()\n");
        }
    } else {
        return -1;
    }

    if ( set_mtu + 18 > AVE_MAX_ETHFRAME ) {
        if ( netif_msg_hw(ave_priv) ) {
            DBG_PRINT("New MTU is too big.\n");
        }
        return -1;

    } else {

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
        spin_lock_irqsave(&ave_priv->lock, flags);
        if(ave_flg_reset != 0){
            printk("ave_api_change_mtu reset status \n");
            spin_unlock_irqrestore(&ave_priv->lock, flags);
            return -1;  
        }
#endif 

#ifdef  AVE_RX_MTU_CHANGE
        AVE_Rreg(reg_val, AVE_RXCR);
        reg_val &= 0xfffff800;
        reg_val |= (set_mtu + 18 & 0x7ff);
        AVE_Wreg(reg_val, AVE_RXCR);
#endif

        netdev->mtu = set_mtu;

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
        spin_unlock_irqrestore(&ave_priv->lock, flags);
#endif 

    }

    return 0;
}

int ave_api_set_mac_address(struct net_device *netdev, void *mac_adr)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    struct sockaddr *sadr = mac_adr;

    memcpy(netdev->dev_addr, sadr->sa_data, ETH_ALEN);

    ave_priv->macaddr_got = 1;

    return(0);
}

int ave_api_mdio_read(struct net_device *netdev, int phyid, int location)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_mdioctl_val;
    uint32_t reg_val = 0;
    long          loop = 0;

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_api_mdio_read() phyid=0x%02x addr=0x%02x\n", phyid, location);
    }
    
#if defined(AVE_SEPARATE_WAIT_FOR_PHY_READY)
    if ( ave_initial_global_reset_completed == 0 ) {
        ave_wait_phy_ready();
    }
#endif  

    AVE_Rreg(reg_mdioctl_val, AVE_MDIOCTR);

    AVE_Wreg((((phyid & ave_priv->mii_if.phy_id_mask) << 8) |
                (location & ave_priv->mii_if.reg_num_mask)), AVE_MDIOAR);

    AVE_Wreg((reg_mdioctl_val | AVE_MDIOCTR_RREQ), AVE_MDIOCTR);

    loop = 0;
    while (1) {
        AVE_Rreg(reg_val, AVE_MDIOSR);
        if ( !(reg_val & AVE_MDIOSR_STS) ) {
            break;
        }
        udelay(10); 
        loop++;
        if( loop > AVE_MDIO_MAXLOOP) {
            printk(KERN_ERR "%s: MDIO read error (MDIOSR=0x%08x).\n", netdev->name, reg_val);
            return 0;
        }
    }

    AVE_Rreg(reg_val, AVE_MDIORDR);

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("  =====> data=0x%04x\n", reg_val);
    }

    return (int)reg_val;
}

void ave_api_mdio_write (struct net_device *netdev, int phyid, int location,
             uint32_t value)
{
    struct ave_private *ave_priv = netdev_priv(netdev);
    uint32_t reg_mdioctl_val;
    uint32_t reg_val;
    uint32_t reg_rxcr_val, reg_rxcr_val_new, reg_txcr_val_new;
    long          loop = 0;
    unsigned long linkspeed;

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_api_mdio_write() phyid=0x%02x addr=0x%02x data=0x%04x\n", phyid, location, value);
    }

#if defined(AVE_SEPARATE_WAIT_FOR_PHY_READY)
    if ( ave_initial_global_reset_completed == 0 ) {
        ave_wait_phy_ready();
  }
#endif  

    AVE_Rreg(reg_mdioctl_val, AVE_MDIOCTR);

    AVE_Wreg((((phyid & ave_priv->mii_if.phy_id_mask) << 8) |
                (location & ave_priv->mii_if.reg_num_mask)), AVE_MDIOAR);

    AVE_Wreg(value, AVE_MDIOWDR);

    AVE_Wreg((reg_mdioctl_val | AVE_MDIOCTR_WREQ), AVE_MDIOCTR);

    loop = 0;
    while (1) {
        AVE_Rreg(reg_val, AVE_MDIOSR);
        if ( !(reg_val & AVE_MDIOSR_STS) ) {
            break;
        }
        udelay(10); 
        loop++;
        if( loop > AVE_MDIO_MAXLOOP) {
            printk(KERN_ERR "%s: MDIO write error (MDIOSR=0x%08x).\n", netdev->name, reg_val);
            break;
        }
    }

    if( (location == MII_BMCR) && ((value & BMCR_ANENABLE) != BMCR_ANENABLE) ){

        if( (value & (BMCR_SPEED1000|BMCR_SPEED100)) == BMCR_SPEED1000 ){
            linkspeed = 1000;
        }
        else if( (value & (BMCR_SPEED1000|BMCR_SPEED100)) == BMCR_SPEED100 ){
            linkspeed = 100;
        }
        else{
            linkspeed = 10;
        }

        ave_util_avereg_linkspeedsetting(linkspeed);

        AVE_Rreg(reg_rxcr_val, AVE_RXCR);
        AVE_Rreg(reg_txcr_val_new, AVE_TXCR);
        reg_rxcr_val_new = reg_rxcr_val;

        if ( value & BMCR_FULLDPLX ) {
            reg_rxcr_val_new |= AVE_RXCR_FDUPEN;
#ifdef AVE_ENABLE_PAUSE
            reg_rxcr_val_new |= AVE_RXCR_FLOCTR;
            reg_txcr_val_new |= AVE_TXCR_FLOCTR;
#endif 
        } else {
            reg_rxcr_val_new &= ~(AVE_RXCR_FDUPEN);
#ifdef AVE_ENABLE_PAUSE
            reg_rxcr_val_new &= ~(AVE_RXCR_FLOCTR);
            reg_txcr_val_new &= ~(AVE_TXCR_FLOCTR);
#endif 
        }

        if ( netif_msg_drv(ave_priv) ) {
            DBG_PRINT("%s: Old rxcr 0x%08x new_rxcr 0x%08x new_txcr 0x%08x \n",
                       netdev->name, reg_rxcr_val, reg_rxcr_val_new, reg_txcr_val_new);
        }

        if( reg_rxcr_val != reg_rxcr_val_new ){
            AVE_Wreg((reg_rxcr_val_new & ~(AVE_RXCR_RXEN)), AVE_RXCR);

            AVE_Wreg( reg_txcr_val_new, AVE_TXCR);
            AVE_Wreg( reg_rxcr_val_new, AVE_RXCR);
            if ( netif_msg_drv(ave_priv) ) {
                DBG_PRINT("%s: TX RX Register Status Change.\n",netdev->name);
            }
        }
    }

    return;
}

#define AVE_PROC_WIDTHLIMIT   80
#define AVE_PROC_DBG_LINEMAX  40
#define AVE_PROC_DBG_WIDTHMAX 32  

static char ave_proc_msg[AVE_PROC_DBG_LINEMAX][AVE_PROC_DBG_WIDTHMAX];
static int ave_proc_cur = 0;  

int ave_proc_read(char *page, char **start, off_t offset, int count,
           int *eof, void *data)
{
    struct ave_private *ave_priv = netdev_priv(ave_dev);
    int line_cnt, sline, length = 0;
    uint32_t reg_val;
    int wide_limit = count - AVE_PROC_WIDTHLIMIT - 60;
#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    unsigned long flags;
#endif 

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_proc_read()\n");
    }
#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    spin_lock_irqsave(&ave_priv->lock, flags);
    if(ave_flg_reset != 0){
        printk("ave_proc_read reset status \n");
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return 0;  
    }
#endif 

    line_cnt = 0;
    length += sprintf(page +length, "\n----- Start AVEther /proc info -----\n");

    AVE_Rreg(ave_info[5].value, AVE_BFCR);
    AVE_Rreg(ave_info[6].value, AVE_GFCR);
    AVE_Rreg(ave_info[9].value,  AVE_RX0FC);
    AVE_Rreg(ave_info[10].value, AVE_RX1FC);
    AVE_Rreg(ave_info[11].value, AVE_RX2FC);
    AVE_Rreg(ave_info[12].value, AVE_RX3FC);
    AVE_Rreg(ave_info[13].value, AVE_RX4FC);
    AVE_Rreg(ave_info[14].value, AVE_RX0OVFFC);
    AVE_Rreg(ave_info[15].value, AVE_RX1OVFFC);
    AVE_Rreg(ave_info[16].value, AVE_RX2OVFFC);
    AVE_Rreg(ave_info[17].value, AVE_RX3OVFFC);
    AVE_Rreg(ave_info[18].value, AVE_RX4OVFFC);

    AVE_Rreg(reg_val, AVE_SN5FC);
    ave_info[19].value = reg_val;
    AVE_Rreg(reg_val, AVE_SN6FC);
    ave_info[19].value += reg_val;
    AVE_Rreg(reg_val, AVE_SN7FC);
    ave_info[19].value += reg_val;


    length += sprintf(page +length, "[AVE Current Infomation:]\n");
    while((ave_info[line_cnt].tag[0] != '\0') && (length < wide_limit)){
        length += sprintf(page + length, "%s:%d\n",
               ave_info[line_cnt].tag, ave_info[line_cnt].value);
        line_cnt++;
    }

    length += sprintf(page +length, "\n[AVE Recent Operations:ORDER BY NEW]\n");
    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("current line = %d\n", ave_proc_cur);
    }
    for(line_cnt = 0; (line_cnt < AVE_PROC_DBG_LINEMAX) && (length < wide_limit); line_cnt++){
        sline= (ave_proc_cur - 1 - line_cnt + AVE_PROC_DBG_LINEMAX)%AVE_PROC_DBG_LINEMAX;    
        length += sprintf(page + length, "[%02d]%s\n", sline, ave_proc_msg[sline]);
        if(line_cnt%5 == 4){
            length += sprintf(page+length, "\n");
        }
    }

    length += sprintf(page +length, "\n-----  End  AVEther /proc info -----\n");

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    spin_unlock_irqrestore(&ave_priv->lock, flags);
#endif 

    *eof = 1;
    return length;
}

void ave_proc_add(char *msg)
{
    struct ave_private *ave_priv;

    if ( ave_dev != NULL ) {
        ave_priv = netdev_priv(ave_dev);
        if ( netif_msg_drv(ave_priv) ) {
        }
    } else {
        DBG_PRINT("ave_proc_add. Line=%d\n", ave_proc_cur);
    }

    if(debug == 1){
        sprintf(ave_proc_msg[ave_proc_cur], msg);
        ave_proc_cur = (ave_proc_cur + 1) % AVE_PROC_DBG_LINEMAX;
    }

}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,20)
static int ave_proc_show(struct seq_file *m,void *v)
{
    struct ave_private *ave_priv = netdev_priv(ave_dev);
    int line_cnt, sline = 0;
    uint32_t reg_val;
#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    unsigned long flags;
#endif 

    if ( netif_msg_drv(ave_priv) ){
        DBG_PRINT("ave_proc_show()\n");
    }

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    spin_lock_irqsave(&ave_priv->lock, flags);
    if(ave_flg_reset != 0){
        printk("ave_proc_show reset status \n");
        spin_unlock_irqrestore(&ave_priv->lock, flags);
        return 0;  
    }
#endif 

    line_cnt = 0;
    seq_puts(m, "\n----- Start AVEther /proc info -----\n");

    AVE_Rreg(ave_info[5].value, AVE_BFCR);
    AVE_Rreg(ave_info[6].value, AVE_GFCR);
    AVE_Rreg(ave_info[9].value,  AVE_RX0FC);
    AVE_Rreg(ave_info[10].value, AVE_RX1FC);
    AVE_Rreg(ave_info[11].value, AVE_RX2FC);
    AVE_Rreg(ave_info[12].value, AVE_RX3FC);
    AVE_Rreg(ave_info[13].value, AVE_RX4FC);
    AVE_Rreg(ave_info[14].value, AVE_RX0OVFFC);
    AVE_Rreg(ave_info[15].value, AVE_RX1OVFFC);
    AVE_Rreg(ave_info[16].value, AVE_RX2OVFFC);
    AVE_Rreg(ave_info[17].value, AVE_RX3OVFFC);
    AVE_Rreg(ave_info[18].value, AVE_RX4OVFFC);

    AVE_Rreg(reg_val, AVE_SN5FC);
    ave_info[19].value = reg_val;
    AVE_Rreg(reg_val, AVE_SN6FC);
    ave_info[19].value += reg_val;
    AVE_Rreg(reg_val, AVE_SN7FC);
    ave_info[19].value += reg_val;

    seq_puts(m, "[AVE Current Infomation:]\n");
    while(ave_info[line_cnt].tag[0] != '\0'){
        seq_printf(m,  "%s:%d\n", ave_info[line_cnt].tag, ave_info[line_cnt].value);
        line_cnt++;
    } 

    seq_puts(m, "\n[AVE Recent Operations:ORDER BY NEW]\n");
    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("current line = %d\n", ave_proc_cur);
    }
    for(line_cnt = 0; line_cnt < AVE_PROC_DBG_LINEMAX; line_cnt++){
        sline= (ave_proc_cur - 1 - line_cnt + AVE_PROC_DBG_LINEMAX)%AVE_PROC_DBG_LINEMAX;    
        seq_printf(m, "[%02d]%s\n", sline, ave_proc_msg[sline]);
        if(line_cnt%5 == 4){
            seq_puts(m, "\n");
        }
    }
  
    seq_puts(m, "\n-----  End  AVEther /proc info -----\n");

#ifdef AVE_ENABLE_LONGWAIT_BY_SLEEP 
    spin_unlock_irqrestore(&ave_priv->lock, flags);
#endif 

    return 0;
}

static int ave_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, ave_proc_show, NULL);
}

static const struct file_operations ave_proc_read_fops = {
    .owner    = THIS_MODULE,
    .open     = ave_proc_open,
    .llseek   = seq_lseek,
    .read     = seq_read,
    .release  = single_release,
};
#endif

#ifdef AVE_HIBERNATION
static int ave_pf_suspend(struct platform_device *dev, pm_message_t state)
{
    struct net_device *netdev = platform_get_drvdata(dev);
    struct ave_private *ave_priv = netdev_priv(netdev);
    struct ave_save_data *save;
    unsigned long flags;
    int i;

    save = kmalloc(sizeof(struct ave_save_data),GFP_KERNEL);
    if(!save){
        return -ENOMEM;
    }

    ave_priv->saved_state = save;

    spin_lock_irqsave(&ave_priv->lock, flags);

    save->wolsetting = ave_get_wol_setting(netdev);
    save->bmcr = (uint32_t)ave_api_mdio_read(netdev, ave_priv->phy_id, MII_BMCR);
    AVE_Rreg(save->gimr, AVE_GIMR);
    AVE_Rreg(save->gtcr, AVE_GTICR);
    AVE_Rreg(save->txcr, AVE_TXCR);
    AVE_Rreg(save->rxcr, AVE_RXCR);
    AVE_Rreg(save->rxmac1r, AVE_RXMAC1R);
    AVE_Rreg(save->rxmac2r, AVE_RXMAC2R);
    AVE_Rreg(save->pfen, AVE_PFEN);
    AVE_Wreg(0x0, AVE_PFEN);
    for(i=0; i<18; ++i){
        AVE_Rreg(save->pfnm0003[i], AVE_PFNM0003+0x40*i);
    }
    for(i=0; i<18; ++i){
        AVE_Rreg(save->pfnm0407[i], AVE_PFNM0407+0x40*i);
    }
    for(i=0; i<8; ++i){
        AVE_Rreg(save->pfnm0c0f[i], AVE_PFNM0C0F+0x40*i);
    }
    AVE_Rreg(save->pfnm0c0f[8], AVE_PFNM0C0F+0x440);
    for(i=0; i<7; ++i){
        AVE_Rreg(save->pfnm1013[i], AVE_PFNM1013+0x40*i);
    }
    AVE_Rreg(save->pfnm1013[7], AVE_PFNM1013+0x440);
    for(i=0; i<7; ++i){
        AVE_Rreg(save->pfnm1417[i], AVE_PFNM1417+0x40*i);
    }
    AVE_Rreg(save->pfnm1417[7], AVE_PFNM1417+0x440);
    for(i=0; i<7; ++i){
        AVE_Rreg(save->pfnm181b[i], AVE_PFNM181B+0x40*i);
    }
    AVE_Rreg(save->pfnm181b[7], AVE_PFNM181B+0x440);
    for(i=0; i<7; ++i){
        AVE_Rreg(save->pfnm1c1f[i], AVE_PFNM1C1F+0x40*i);
    }
    AVE_Rreg(save->pfnm1c1f[7], AVE_PFNM1C1F+0x440);
    for(i=0; i<7; ++i){
        AVE_Rreg(save->pfnm2023[i], AVE_PFNM2023+0x40*i);
    }
    AVE_Rreg(save->pfnm2023[7], AVE_PFNM2023+0x440);
    for(i=0; i<7; ++i){
        AVE_Rreg(save->pfnm2427[i], AVE_PFNM2427+0x40*i);
    }
    AVE_Rreg(save->pfnm2427[7], AVE_PFNM2427+0x440);
    for(i=0; i<7; ++i){
        AVE_Rreg(save->pfnm282b[i], AVE_PFNM282B+0x40*i);
    }
    AVE_Rreg(save->pfnm282b[7], AVE_PFNM282B+0x440);
    for(i=0; i<7; ++i){
        AVE_Rreg(save->pfnm2c2f[i], AVE_PFNM2C2F+0x40*i);
    }
    AVE_Rreg(save->pfnm2c2f[7], AVE_PFNM2C2F+0x440);
    for(i=0; i<7; ++i){
        AVE_Rreg(save->pfnm3033[i], AVE_PFNM3033+0x40*i);
    }
    AVE_Rreg(save->pfnm3033[7], AVE_PFNM3033+0x440);
    for(i=0; i<7; ++i){
        AVE_Rreg(save->pfnm3437[i], AVE_PFNM3437+0x40*i);
    }
    AVE_Rreg(save->pfnm3437[7], AVE_PFNM3437+0x440);
    for(i=0; i<7; ++i){
        AVE_Rreg(save->pfnm383b[i], AVE_PFNM383B+0x40*i);
    }
    AVE_Rreg(save->pfnm383b[7], AVE_PFNM383B+0x440);
    for(i=0; i<16; ++i){
        AVE_Rreg(save->pfnmbyte0[i], AVE_PFMBYTE_BASE+0x8*i);
    }
    for(i=0; i<7; ++i){
        AVE_Rreg(save->pfnmbyte1[i], AVE_PFMBYTE_BASE+0x4+0x8*i);
    }
    AVE_Rreg(save->pfnmbyte1[7], AVE_PFMBYTE_BASE+0x8C);
    for(i=0; i<7; ++i){
        AVE_Rreg(save->pfnmbit[i], AVE_PFMBIT_BASE+0x4*i);
    }
    AVE_Rreg(save->pfnmbit[7], AVE_PFMBIT_BASE+0x44);
    for(i=0; i<19; ++i){
        AVE_Rreg(save->pfnmsel[i], AVE_PFSEL_BASE+0x4*i);
    }
    AVE_Rreg(save->linksel, AVE_LINKSEL);
    AVE_Rreg(save->mssr, AVE_MSSR);
    AVE_Rreg(save->iirqc, AVE_IIRQC);

    spin_unlock_irqrestore(&ave_priv->lock, flags);

    ave_api_stop(netdev);

    return 0;
}

static int ave_pf_resume(struct platform_device *dev)
{
    struct net_device *netdev = platform_get_drvdata(dev);
    struct ave_private *ave_priv = netdev_priv(netdev);
    struct ave_save_data *save;
    unsigned long flags;
    int i;

    ave_api_open(netdev);

    save = ave_priv->saved_state;
    if(!save){
        return 0;
    }

    spin_lock_irqsave(&ave_priv->lock, flags);

    ave_set_wol_setting(netdev, save->wolsetting);

    ave_api_mdio_write(netdev, ave_priv->phy_id, MII_BMCR, save->bmcr);
    AVE_Wreg(save->gimr, AVE_GIMR);
    AVE_Wreg(save->gtcr, AVE_GTICR);
    AVE_Wreg(save->txcr, AVE_TXCR);
    AVE_Wreg(save->rxcr, AVE_RXCR);
    AVE_Wreg(save->rxmac1r, AVE_RXMAC1R);
    AVE_Wreg(save->rxmac2r, AVE_RXMAC2R);
    AVE_Wreg(0x0, AVE_PFEN);
    for(i=0; i<18; ++i){
        AVE_Wreg(save->pfnm0003[i], AVE_PFNM0003+0x40*i);
    }
    for(i=0; i<18; ++i){
        AVE_Wreg(save->pfnm0407[i], AVE_PFNM0407+0x40*i);
    }
    for(i=0; i<8; ++i){
        AVE_Wreg(save->pfnm0c0f[i], AVE_PFNM0C0F+0x40*i);
    }
    AVE_Wreg(save->pfnm0c0f[8], AVE_PFNM0C0F+0x440);
    for(i=0; i<7; ++i){
        AVE_Wreg(save->pfnm1013[i], AVE_PFNM1013+0x40*i);
    }
    AVE_Wreg(save->pfnm1013[7], AVE_PFNM1013+0x440);
    for(i=0; i<7; ++i){
        AVE_Wreg(save->pfnm1417[i], AVE_PFNM1417+0x40*i);
    }
    AVE_Wreg(save->pfnm1417[7], AVE_PFNM1417+0x440);
    for(i=0; i<7; ++i){
        AVE_Wreg(save->pfnm181b[i], AVE_PFNM181B+0x40*i);
    }
    AVE_Wreg(save->pfnm181b[7], AVE_PFNM181B+0x440);
    for(i=0; i<7; ++i){
        AVE_Wreg(save->pfnm1c1f[i], AVE_PFNM1C1F+0x40*i);
    }
    AVE_Wreg(save->pfnm1c1f[7], AVE_PFNM1C1F+0x440);
    for(i=0; i<7; ++i){
        AVE_Wreg(save->pfnm2023[i], AVE_PFNM2023+0x40*i);
    }
    AVE_Wreg(save->pfnm2023[7], AVE_PFNM2023+0x440);
    for(i=0; i<7; ++i){
        AVE_Wreg(save->pfnm2427[i], AVE_PFNM2427+0x40*i);
    }
    AVE_Wreg(save->pfnm2427[7], AVE_PFNM2427+0x440);
    for(i=0; i<7; ++i){
        AVE_Wreg(save->pfnm282b[i], AVE_PFNM282B+0x40*i);
    }
    AVE_Wreg(save->pfnm282b[7], AVE_PFNM282B+0x440);
    for(i=0; i<7; ++i){
        AVE_Wreg(save->pfnm2c2f[i], AVE_PFNM2C2F+0x40*i);
    }
    AVE_Wreg(save->pfnm2c2f[7], AVE_PFNM2C2F+0x440);
    for(i=0; i<7; ++i){
        AVE_Wreg(save->pfnm3033[i], AVE_PFNM3033+0x40*i);
    }
    AVE_Wreg(save->pfnm3033[7], AVE_PFNM3033+0x440);
    for(i=0; i<7; ++i){
        AVE_Wreg(save->pfnm3437[i], AVE_PFNM3437+0x40*i);
    }
    AVE_Wreg(save->pfnm3437[7], AVE_PFNM3437+0x440);
    for(i=0; i<7; ++i){
        AVE_Wreg(save->pfnm383b[i], AVE_PFNM383B+0x40*i);
    }
    AVE_Wreg(save->pfnm383b[7], AVE_PFNM383B+0x440);
    for(i=0; i<16; ++i){
        AVE_Wreg(save->pfnmbyte0[i], AVE_PFMBYTE_BASE+0x8*i);
    }
    for(i=0; i<7; ++i){
        AVE_Wreg(save->pfnmbyte1[i], AVE_PFMBYTE_BASE+0x4+0x8*i);
    }
    AVE_Wreg(save->pfnmbyte1[7], AVE_PFMBYTE_BASE+0x8C);
    for(i=0; i<7; ++i){
        AVE_Wreg(save->pfnmbit[i], AVE_PFMBIT_BASE+0x4*i);
    }
    AVE_Wreg(save->pfnmbit[7], AVE_PFMBIT_BASE+0x44);
    for(i=0; i<19; ++i){
        AVE_Wreg(save->pfnmsel[i], AVE_PFSEL_BASE+0x4*i);
    }
    AVE_Wreg(save->pfen, AVE_PFEN);
    AVE_Wreg(save->linksel, AVE_LINKSEL);
    AVE_Wreg(save->mssr, AVE_MSSR);
    AVE_Wreg(save->iirqc, AVE_IIRQC);

    spin_unlock_irqrestore(&ave_priv->lock, flags);

    ave_priv->saved_state = NULL;
    kfree(save);

    return 0;
}

static struct platform_driver ave_pf_driver = {
    .suspend = ave_pf_suspend,
    .resume = ave_pf_resume,
    .driver = {
        .name = AVE_CHDEV_NAME,
        .owner  = THIS_MODULE,
    },
};
#endif 

#ifdef AVE_ENABLE_PF_IF
int ave_init_module(struct platform_device *pdev, struct ave_pf_data *ave_pfu)
#else
int ave_init_module(void)
#endif
{
    struct ave_private *ave_priv;
    int ret_val;
    struct ave_descriptor_matrix desc_mtrx = AVE_DMATRX_INIT;
#ifdef AVE_ENABLE_PF_IF
    const void *mac_address;
#endif

    DBG_PRINT("ave_init_module()\n");
    ave_proc_add("ave_init_module");

#ifdef AVE_ENABLE_PF_IF
    ave_reg_base = (uint64_t)ave_pfu->base;
#else
#ifdef AVE_ENABLE_IOREMAP
    ave_reg_base = (unsigned long)ioremap(AVE_BASE, AVE_IO_SIZE);
    if ((void *)ave_reg_base == NULL) {
        printk(KERN_ERR "ave: Could not ioremap device I/O.\n");
        return -1;
    }
#endif 
#endif

    ave_platform_init();

    ave_dev = alloc_etherdev(sizeof(struct ave_private));
    if (ave_dev == NULL) {
        printk(KERN_ERR "ave: Could not allocate ethernet device.\n");
        return -1;
    }     

    ave_priv = netdev_priv(ave_dev);
    memset(ave_priv, 0, sizeof(struct ave_private));

#ifdef  AVE_OUTPUT_MSG
    ave_priv->msg_enable = DEBUG_DEFAULT;
#endif

#ifdef AVE_ENABLE_PF_IF
    ave_dev->base_addr = ave_base;
    ave_dev->irq = ave_pfu->irq;

    mac_address = of_get_mac_address( pdev->dev.of_node );
    if(mac_address){
        memcpy(ave_dev->dev_addr, mac_address, 6);
        ave_priv->macaddr_got = 1;
    }
    else{
        printk("fail : of_get_mac_address.. \n");
    }

#else
    ave_dev->base_addr = ave_base;
    ave_dev->irq = ave_irq;
#endif

    ave_global_reset_for_init_module(ave_dev);

    ave_get_version(ave_dev);
    if ( strncmp(ave_priv->id_str, AVEV4_ID, 4) != 0 ) {
        printk(KERN_ERR "ave: unmatch ID charactors\n");
        goto err_free;
    }

    printk(KERN_INFO "  %s", drv_version);

#if defined(AVE_ENABLE_CACHEDMA)
    ave_initialize_coherency_port();
#endif

    ave_emac_init(ave_dev);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32) 
    ave_dev->open               = ave_api_open;
    ave_dev->stop               = ave_api_stop;
    ave_dev->hard_start_xmit    = ave_api_start_xmit;
#ifdef AVE_ENABLE_TSO 
    ave_dev->hard_start_xmit    = ave_api_start_xmit_tso;
#endif 
    ave_dev->do_ioctl           = ave_api_ioctl;
    ave_dev->set_multicast_list = ave_api_multicast;
    ave_dev->get_stats          = ave_api_stats;
    ave_dev->change_mtu         = ave_api_change_mtu;
#elif LINUX_VERSION_CODE < KERNEL_VERSION(3,10,20) 
    ave_dev->netdev_ops = &ave_netdev_ops;
    ave_netdev_ops.ndo_open                 = ave_api_open;
    ave_netdev_ops.ndo_stop                 = ave_api_stop;
    ave_netdev_ops.ndo_start_xmit           = ave_api_start_xmit;
#ifdef AVE_ENABLE_TSO 
    ave_netdev_ops.ndo_start_xmit           = ave_api_start_xmit_tso;
#endif 
    ave_netdev_ops.ndo_do_ioctl             = ave_api_ioctl;
    ave_netdev_ops.ndo_set_multicast_list   = ave_api_multicast;
    ave_netdev_ops.ndo_get_stats            = ave_api_stats;
    ave_netdev_ops.ndo_change_mtu           = ave_api_change_mtu;
    ave_netdev_ops.ndo_set_mac_address      = ave_api_set_mac_address;
#else 
    ave_dev->netdev_ops = &ave_netdev_ops;
    ave_netdev_ops.ndo_open                 = ave_api_open;
    ave_netdev_ops.ndo_stop                 = ave_api_stop;
    ave_netdev_ops.ndo_start_xmit           = ave_api_start_xmit;
#ifdef AVE_ENABLE_TSO 
    ave_netdev_ops.ndo_start_xmit           = ave_api_start_xmit_tso;
#endif 
    ave_netdev_ops.ndo_do_ioctl             = ave_api_ioctl;
    ave_netdev_ops.ndo_set_rx_mode          = ave_api_multicast;
    ave_netdev_ops.ndo_get_stats            = ave_api_stats;
    ave_netdev_ops.ndo_change_mtu           = ave_api_change_mtu;
    ave_netdev_ops.ndo_set_mac_address      = ave_api_set_mac_address;
#endif 

#if 1
    ave_dev->ethtool_ops = &ave_ethtool_ops;
#else
    SET_ETHTOOL_OPS(ave_dev, &ave_ethtool_ops);
#endif

#ifdef AVE_ENABLE_PF_IF
    ave_dev->dev.parent = &pdev->dev;
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,20)
    create_proc_read_entry(AVE_PROC_FILE, 0, NULL, ave_proc_read, NULL);
#else
    proc_create(AVE_PROC_FILE, 0, NULL, &ave_proc_read_fops);
#endif

#ifdef MODULE
    SET_MODULE_OWNER(ave_dev);
#endif 

#ifndef AVE_SINGLE_RXRING_MODE
    ave_set_descriptor_num(&desc_mtrx);
#else 
    ave_priv->tx_dnum = desc_mtrx.tx;
    ave_priv->rx_dnum[0] = desc_mtrx.rx[0];
#endif 

    ave_priv->mii_if.phy_id_mask  = 0x1f; 
    ave_priv->mii_if.reg_num_mask = 0x1f; 
    ave_priv->mii_if.phy_id       = AVE_PHY_ID & ave_priv->mii_if.phy_id_mask;
    ave_priv->mii_if.dev          = ave_dev;    
    ave_priv->mii_if.mdio_read    = ave_api_mdio_read;
    ave_priv->mii_if.mdio_write   = (void *)ave_api_mdio_write;

    ave_priv->phy_id = ave_priv->mii_if.phy_id;   

    spin_lock_init(&(ave_priv->lock));

    init_timer(&ave_priv->timer); 


#ifndef AVE_SINGLE_RXRING_MODE
    memset(ave_fltinf_g, 0, sizeof( ave_fltinf_g ));
    sema_init(&(ave_sem_ioctl), 1);

#ifdef AVE_ENABLE_CSUM_OFFLOAD
    ave_dev->features |= (NETIF_F_IP_CSUM | NETIF_F_RXCSUM);
    ave_dev->hw_features |= (NETIF_F_IP_CSUM | NETIF_F_RXCSUM);
#endif 

#ifdef AVE_ENABLE_TSO
    ave_dev->features |= (NETIF_F_IP_CSUM  | NETIF_F_RXCSUM | NETIF_F_SG | NETIF_F_TSO);
    ave_dev->hw_features |= (NETIF_F_IP_CSUM  | NETIF_F_RXCSUM | NETIF_F_SG | NETIF_F_TSO);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32) 
    ave_dev->features |= (NETIF_F_TSO6);
    ave_dev->hw_features |= (NETIF_F_TSO6);
    ave_dev->features |= (NETIF_F_IPV6_CSUM);
    ave_dev->hw_features |= (NETIF_F_IPV6_CSUM);
#endif 
#endif 
#endif

#ifdef AVE_HIBERNATION
    platform_driver_register(&ave_pf_driver);
#endif

    ret_val = register_netdev(ave_dev);
    if ( ret_val != 0 ) {
        printk(KERN_ERR "ave: <FAIL> register netdevice failed.\n");
        goto err_free;
    }

    return 0;

err_free:
    free_netdev(ave_dev);   
    ave_dev = NULL;
    return -1;
}

void ave_cleanup_module(void)
{
#ifndef AVE_SINGLE_RXRING_MODE
    int ret_val=0;
#endif
    struct ave_private *ave_priv = netdev_priv(ave_dev);

    if ( netif_msg_drv(ave_priv) ) {
        DBG_PRINT("ave_cleanup_module()\n");
    }
    ave_proc_add("ave_cleanup_module");

#ifdef AVE_HIBERNATION
    platform_driver_unregister(&ave_pf_driver);
#endif
    unregister_netdev(ave_dev);
    free_netdev(ave_dev);   
    ave_dev = NULL;

#ifndef AVE_SINGLE_RXRING_MODE
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32) 
    ret_val = unregister_chrdev(AVE_MAJOR,AVE_CHDEV_NAME);
#else 
    unregister_chrdev(AVE_MAJOR,AVE_CHDEV_NAME);
#endif 
    if ( ret_val != 0 ) {
        printk(KERN_ERR "ave: <FAIL> unregister chrdev failed.\n");
    }
#endif

    remove_proc_entry(AVE_PROC_FILE, NULL);

#ifndef AVE_ENABLE_PF_IF
#ifdef AVE_ENABLE_IOREMAP
    iounmap((void *)ave_reg_base);
#endif 
#endif
}



#ifdef AVE_ENABLE_PF_IF
static const struct of_device_id of_ave_match[];

#if 1
int ave_64bit_addr_discrypt = 1;
int ave_use_omniphy = 0;
uint32_t AVE_DESC_SIZE = AVE_DESC_SIZE_64;
uint32_t AVE_TXDM = AVE_TXDM_64;
uint32_t AVE_RXDM = AVE_RXDM_64;
uint32_t AVE_TXDM_SIZE = AVE_TXDM_SIZE_64;
uint32_t AVE_RXDM_SIZE = AVE_RXDM_SIZE_64;
#endif

static int ave_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct ave_pf_data *ave_pfu;
	struct resource	*res;
	struct device *dev = &pdev->dev;
	const struct of_device_id *of_id;
	void __iomem *base;
	int irq;
	int ret = 0;
	int phy_mode;
	const char *phy_name;
	const char *mii_mode_str[] = { "undef", "MII", "RMII", "RGMII" };

    of_id = of_match_device(of_ave_match, dev);
	if (!of_id) {
		return -EINVAL;
	}

	ave_pfu = devm_kzalloc(dev, sizeof(*ave_pfu), GFP_KERNEL);
	if (!ave_pfu) {
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, ave_pfu);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
	   return -EINVAL;
	}
	
	base = devm_ioremap_resource(dev, res);
	if (IS_ERR(base)) {
		return PTR_ERR(base);
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		return irq;
	}

	ave_pfu->dev  = dev;
	ave_pfu->base = base;
	ave_pfu->irq  = irq;

#if 1

    if(of_id == &of_ave_match[1]){
printk("yk-dbg use LD11 \n");
        ave_64bit_addr_discrypt = 0;
        ave_use_omniphy = 1;
    }
    else{
printk("yk-dbg use LD20 \n");
        ave_64bit_addr_discrypt = 1;
        ave_use_omniphy = 0;
    }

    if(ave_64bit_addr_discrypt == 1){ 
printk("yk-dbg use 64 mode \n");
        AVE_DESC_SIZE = AVE_DESC_SIZE_64;
        AVE_TXDM = AVE_TXDM_64;
        AVE_RXDM = AVE_RXDM_64;
        AVE_TXDM_SIZE = AVE_TXDM_SIZE_64;
        AVE_RXDM_SIZE = AVE_RXDM_SIZE_64;

    } else { 
printk("yk-dbg use 32 mode \n");
        AVE_DESC_SIZE = AVE_DESC_SIZE_32;
        AVE_TXDM = AVE_TXDM_32;
        AVE_RXDM = AVE_RXDM_32;
        AVE_TXDM_SIZE = AVE_TXDM_SIZE_32;
        AVE_RXDM_SIZE = AVE_RXDM_SIZE_32;

    } 

    if(ave_use_omniphy == 1){
        ave_miimode=2;      
        ave_gbit_mode = 0;  
    }
    else{
        ave_miimode=3;      
        ave_gbit_mode = 1;  
    }

#endif

	ret = of_property_read_string(node, "socionext,phy-type", &phy_name);
	if (ret < 0) {
		if (ret != -EINVAL) {
			dev_err(ave_pfu->dev, "phy-type is illegal\n");
			goto err_map;
		}
		else {
		}
	}
	else {
		if ((phy_name) && (!strcmp(phy_name, "omniphy"))) {
			ave_use_omniphy = 1;
		}
		else {
			ave_use_omniphy = 0;
		}
	}

	ret = of_property_read_u32(node, "socionext,phy-mode", &phy_mode);
	if (ret < 0) {
		if (ret != -EINVAL) {
			dev_err(ave_pfu->dev, "phy-mode is illegal\n");
			goto err_map;
		}
		else {
		}
	}
	else {
		switch(phy_mode) {
		case 1: 
			ave_miimode = 1;
			ave_gbit_mode = 0;	
			break;
		case 2: 
			ave_miimode = 2;
			ave_gbit_mode = 0;	
			break;
		case 3: 
			ave_miimode = 3;
			ave_gbit_mode = 1;	
			break;
		default:
			dev_err(ave_pfu->dev, "phy-mode %d is out of range\n", phy_mode);
			goto err_map;
		}
	}
	dev_info(ave_pfu->dev, "phy %s, mode %s\n",
		 (ave_use_omniphy) ? "omniphy" : "external", mii_mode_str[ave_miimode]);

	ave_init_module(pdev, ave_pfu);

	ret = of_platform_populate(node, NULL, NULL, ave_pfu->dev);
	if (ret) {
		dev_err(ave_pfu->dev, "failed to register core - %d\n", ret);
		goto err_ave;
	}

	return 0;

err_ave:
	ave_cleanup_module();
err_map:
	devm_iounmap(dev, base);

	return ret;
}

static int ave_remove(struct platform_device *pdev)
{
	struct ave_pf_data *ave_pfu = platform_get_drvdata(pdev);

	devm_iounmap(ave_pfu->dev, ave_pfu->base);

	of_platform_depopulate(&pdev->dev);
	ave_cleanup_module();
	
	return 0;
}

static const struct of_device_id of_ave_match[] = {
	{ .compatible = "socionext,ave", },
	{ .compatible = "socionext,ave11", },
	{  }
};
MODULE_DEVICE_TABLE(of, of_ave_match);

static struct platform_driver ave_driver = {
	.probe		= ave_probe,
	.remove		= ave_remove,
	.driver		= {
		.name	= "ave",
		.of_match_table	= of_ave_match,
	},
};

module_platform_driver(ave_driver);

MODULE_ALIAS("platform:ave");
MODULE_AUTHOR("Socionext Inc.");
MODULE_DESCRIPTION("AV Ether Driver");
MODULE_LICENSE("GPL v2");

#else
module_init(ave_init_module);
module_exit(ave_cleanup_module);
#endif
