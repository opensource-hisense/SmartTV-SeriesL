/*** CONFIDENTIAL ***/
/* Copyright (C) 2015, Socionext Inc. */
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <net.h>
#include <miiphy.h>
#include <asm/io.h>

#include "ethernet_ave.h"
#if defined(AVEV4)
#include <linux/mii.h>
#endif

static int phyid = -1;	

static int rx_pos = 0;	
static int rx_siz = 0;	
static int rx_off = 0;	
static int tx_off = 0;	
static int tx_num = 0;	

static uchar  tx_adj_PacketBuf[PKTSIZE_ALIGN + PKTALIGN];
static uchar *tx_adj_buf;

static uint32_t mn_avev3_readreg( struct eth_device *dev,  uint32_t addr)
{
	uint32_t val;
	val = MN_AVEV3_REG( (unsigned long)(dev->iobase), (unsigned long)(addr) );
	return val;
}

static void mn_avev3_writereg( struct eth_device *dev, uint32_t addr,  uint32_t val)
{
	MN_AVEV3_REG((unsigned long)(dev->iobase), (unsigned long)(addr) ) = val;
}

static void mn_avev3_write_rxdm( struct eth_device *dev, int i, int j, uint32_t val)
{
#ifdef AVE_ENABLE_32BIT_DESC
	mn_avev3_writereg(dev, MN_AVEV3_RXDM + i*8 + j*4, val);
#else
	mn_avev3_writereg(dev, MN_AVEV3_RXDM + i*12 + j*4, val);
#endif
}
static void mn_avev3_write_txdm( struct eth_device *dev, int i, int j, uint32_t val)
{
#ifdef AVE_ENABLE_32BIT_DESC
	mn_avev3_writereg(dev, MN_AVEV3_TXDM + i*8 + j*4, val);
#else
	mn_avev3_writereg(dev, MN_AVEV3_TXDM + i*12 + j*4, val);
#endif
}

static uint32_t mn_avev3_read_rxdm( struct eth_device *dev, int i, int j)
{
#ifdef AVE_ENABLE_32BIT_DESC
	return mn_avev3_readreg(dev, MN_AVEV3_RXDM + i*8 + j*4);
#else
	return mn_avev3_readreg(dev, MN_AVEV3_RXDM + i*12 + j*4);
#endif
}
static uint32_t mn_avev3_read_txdm( struct eth_device *dev, int i, int j)
{
#ifdef AVE_ENABLE_32BIT_DESC
	return mn_avev3_readreg(dev, MN_AVEV3_TXDM + i*8+ j*4);
#else
	return mn_avev3_readreg(dev, MN_AVEV3_TXDM + i*12+ j*4);
#endif
}

#if defined(AVEV4)
int mn_avev3_mdio_read(struct eth_device *dev, int phy_id, int location)
{
	uint32_t mdio_ctl;
	uint32_t reg = 0;
	int32_t          loop_cnt = 0;

	mdio_ctl = mn_avev3_readreg(dev, MN_AVEV3_MDIOCTR);

	mn_avev3_writereg(dev, MN_AVEV3_MDIOAR, (((phy_id & 31) << 8) |
              (location & 31)));

	mn_avev3_writereg(dev, MN_AVEV3_MDIOCTR, (mdio_ctl | MN_AVEV3_MDIOCTR_RREQ));

	loop_cnt = 0;
	while (1) {
		reg = mn_avev3_readreg(dev, MN_AVEV3_MDIOSR);
		if ( !(reg & MN_AVEV3_MDIOSR_STS) ) {
			break;
		}
		udelay(10); 
		loop_cnt++;
		if( loop_cnt > 1000) {
			printf("MDIO read error (MDIOSR=0x%08x).\n", le32_to_cpu(reg));
			return 0;
		}
	}

	reg = mn_avev3_readreg(dev, MN_AVEV3_MDIORDR);

	return (int)reg;
}

void mn_avev3_mdio_write(struct eth_device *dev, int phy_id, int location, uint32_t value)
{
	uint32_t mdio_ctl;
	uint32_t reg = 0;
	int32_t  loop_cnt = 0;

	mdio_ctl = mn_avev3_readreg(dev, MN_AVEV3_MDIOCTR);

	mn_avev3_writereg(dev, MN_AVEV3_MDIOAR, (((phy_id & 31) << 8) |
              (location & 31)));

	mn_avev3_writereg(dev, MN_AVEV3_MDIOWDR, value);

	mn_avev3_writereg(dev, MN_AVEV3_MDIOCTR, (mdio_ctl | MN_AVEV3_MDIOCTR_WREQ));

	loop_cnt = 0;
	while (1) {
		reg = mn_avev3_readreg(dev, MN_AVEV3_MDIOSR);
		if ( !(reg & MN_AVEV3_MDIOSR_STS) ) {
			break;
		}
		udelay(10); 
		loop_cnt++;
		if( loop_cnt > 1000) {
			printf("MDIO write error (MDIOSR=0x%08x).\n", le32_to_cpu(reg));
			return;
		}
	}

	return;
}

static int mn_avev3_wait_autoneg_complete(struct eth_device *dev)
{
	int i;

	for(i = 0; i < 200000; i++) {
		if((mn_avev3_mdio_read(dev, phyid, MII_BMSR) & BMSR_LSTATUS) !=0) {
			break;
		}
	}

	return (i < 200000) ? 0 : -1;
}

void mn_avev3_autoneg_result_check(struct eth_device *dev, uint32_t *speedP, uint32_t *duplexP, uint32_t *pauseP)
{
	uint32_t speed = 1000;
	uint32_t duplex = 1;
	uint32_t pause_cap = 1;
#ifndef CONFIG_AVE_RMII
	uint32_t gb_advertise=0, gb_partner=0;
#endif
	uint32_t advertise=0, partner=0;
	uint32_t state = 0;

#ifdef CONFIG_AVE_RMII
	advertise = mn_avev3_mdio_read(dev, phyid, MII_ADVERTISE);
	partner = mn_avev3_mdio_read(dev, phyid, MII_LPA);

	state = (advertise & partner);
	if( (state & LPA_100FULL) != 0 ){
		duplex = 1;
		speed = 100;
	} else if( (state & LPA_100HALF) != 0 ){
		duplex = 0;
		speed = 100;
	} else if( (state & LPA_10FULL) != 0 ){
		duplex = 1;
		speed = 10;
	} else{
		duplex = 0;
		speed = 10;
	}

#else
	gb_advertise = mn_avev3_mdio_read(dev, phyid, MII_CTRL1000);
	gb_partner = mn_avev3_mdio_read(dev, phyid, MII_STAT1000);

	advertise = mn_avev3_mdio_read(dev, phyid, MII_ADVERTISE);
	partner = mn_avev3_mdio_read(dev, phyid, MII_LPA);

	if( ((gb_advertise & ADVERTISE_1000FULL) != 0)
		&& ((gb_partner & LPA_1000FULL) != 0) ){
		duplex = 1;
		speed = 1000;
	} else if( ((gb_advertise & ADVERTISE_1000HALF) != 0)
		&& ((gb_partner & LPA_1000HALF) != 0) ){
		duplex = 0;
		speed = 1000;
	} else{
		state = (advertise & partner);
		if( (state & LPA_100FULL) != 0 ){
			duplex = 1;
			speed = 100;
		} else if( (state & LPA_100HALF) != 0 ){
			duplex = 0;
			speed = 100;
		} else if( (state & LPA_10FULL) != 0 ){
			duplex = 1;
			speed = 10;
		} else{
			duplex = 0;
			speed = 10;
		}
	}
#endif

	if( ((advertise & partner) & LPA_PAUSE_CAP) == 0 ){
		pause_cap = 0;
	}

#ifdef CONFIG_AVE_OMNIPHY_100M
  printf("*** AVE tentative ***   Force 100M-Half status check \n");
  duplex = 0;
  speed = 100;
  pause_cap = 0;
#endif

	*speedP = speed;
	*duplexP = duplex;
	*pauseP = pause_cap;

	return;
}

static int mn_avev3_set_txspd(struct eth_device *dev)
{
	int ret;
	uint32_t speed = 1000;
	uint32_t duplex = 1;
	uint32_t pause_cap = 1;
	uint32_t txspd;
	uint32_t txcr;

#ifdef CONFIG_AVE_OMNIPHY
        int i, j;
	uint32_t val;
	for(i=0; i<3; i++){
		ret = mn_avev3_wait_autoneg_complete(dev);
		if(ret >= 0) {
			mn_avev3_autoneg_result_check(dev, &speed, &duplex, &pause_cap);
			printf("%s:INFO: autoneg complete speed = %u\n", dev->name, le32_to_cpu(speed));
#ifndef CONFIG_AVE_OMNIPHY_100M
			if(speed == 100){
				mn_avev3_mdio_write(dev, phyid, 20, 0x0000);
				mn_avev3_mdio_write(dev, phyid, 20, 0x0400);
				mn_avev3_mdio_write(dev, phyid, 20, 0x0000);
				mn_avev3_mdio_write(dev, phyid, 20, 0x0400);
				mn_avev3_mdio_write(dev, phyid, 23, 0x0007);
				mn_avev3_mdio_write(dev, phyid, 20, 0x4418);
			}
			else {
				mn_avev3_mdio_write(dev, phyid, 20, 0x0000);
				mn_avev3_mdio_write(dev, phyid, 20, 0x0400);
				mn_avev3_mdio_write(dev, phyid, 20, 0x0000);
				mn_avev3_mdio_write(dev, phyid, 20, 0x0400);
				mn_avev3_mdio_write(dev, phyid, 23, 0x0005);
				mn_avev3_mdio_write(dev, phyid, 20, 0x4418);
				mn_avev3_mdio_write(dev, phyid, 23, 0x8226);
				mn_avev3_mdio_write(dev, phyid, 20, 0x4416);
			}
#endif
			break;
		}

		printf("%s:INFO: autoneg NOT complete speed = %u\n", dev->name, le32_to_cpu(speed));

		mn_avev3_mdio_write(dev, phyid, MII_BMCR, 0x8000);
		for(j=0; j<10000; j++){
			val = mn_avev3_mdio_read(dev, phyid, MII_BMCR);
			if( (val & 0x8000) == 0 ){
				break;
			}
		}
		printf("reset complete\n");
#ifdef CONFIG_AVE_OMNIPHY_100M
		mn_avev3_mdio_write(dev, phyid, MII_BMCR, 0x2000);
		mn_avev3_mdio_write(dev, phyid, 20, 0x0000);
		mn_avev3_mdio_write(dev, phyid, 20, 0x0400);
		mn_avev3_mdio_write(dev, phyid, 20, 0x0000);
		mn_avev3_mdio_write(dev, phyid, 20, 0x0400);
		mn_avev3_mdio_write(dev, phyid, 23, 0x0007);
		mn_avev3_mdio_write(dev, phyid, 20, 0x4418);
#endif
		mn_avev3_mdio_write(dev, phyid, 20, 0x0000);
		mn_avev3_mdio_write(dev, phyid, 20, 0x0400);
		mn_avev3_mdio_write(dev, phyid, 20, 0x0000);
		mn_avev3_mdio_write(dev, phyid, 20, 0x0400);
		mn_avev3_mdio_write(dev, phyid, 23, 0x4400);
		mn_avev3_mdio_write(dev, phyid, 20, 0x4415);
		mn_avev3_mdio_write(dev, phyid, 23, 0x3700);
		mn_avev3_mdio_write(dev, phyid, 20, 0x4413);
		mn_avev3_mdio_write(dev, phyid, MII_BMCR, 0x3300);
	}
#else
	ret = mn_avev3_wait_autoneg_complete(dev);
	if(ret >= 0) {
		mn_avev3_autoneg_result_check(dev, &speed, &duplex, &pause_cap);
		printf("%s:INFO: autoneg complete speed = %u\n", dev->name, le32_to_cpu(speed));
	} else {
		printf("%s:INFO: autoneg NOT complete speed = %u\n", dev->name, le32_to_cpu(speed));
	}
#endif

	switch(speed) {
		case 1000: txspd = 0x00020000; break;
		case 100: txspd = 0x00010000; break;
		default: txspd = 0x00000000; break;
	}

	txcr = mn_avev3_readreg(dev, MN_AVEV3_TXCR);
	txcr &= 0xfffcffff;
	txcr |= txspd;
        txcr |= MN_AVEV3_TXCR_FLOCT;
	mn_avev3_writereg(dev, MN_AVEV3_TXCR, txcr);

#ifdef CONFIG_AVE_RMII
        {
            uint32_t reg;
	    reg = mn_avev3_readreg(dev, MN_AVEV3_LINKSEL);
            if(speed == 100){
	        reg |= 0x00000001;
            }
            else{
                reg &= 0xfffffffe;
            }
	    mn_avev3_writereg(dev, MN_AVEV3_LINKSEL, 1);
        }
#endif

	return ret;
}
#endif 

static void mn_avev3_halt(struct eth_device *dev)
{
	uint32_t val;
	val = mn_avev3_readreg(dev, MN_AVEV3_GRR);
	if(val == 0) {	
		val = mn_avev3_readreg(dev, MN_AVEV3_RXCR);
		val &= ~MN_AVEV3_RXCR_RXEN;
		mn_avev3_writereg(dev, MN_AVEV3_RXCR, val);
		mn_avev3_writereg(dev, MN_AVEV3_DESCC, MN_AVEV3_DESCC_RXDSTP);
		do {
			val = mn_avev3_readreg(dev, MN_AVEV3_DESCC);
		} while((val & MN_AVEV3_DESCC_RXDSTPST) == 0);
		mn_avev3_writereg(dev, MN_AVEV3_GRR, MN_AVEV3_GRR_GRST);
	}
#if defined(AVEV4)
	mdelay(MN_AVEV3_GRST_DELAY_MSEC);
#else
	udelay(MN_AVEV3_GRST_DELAY_USEC);
#endif
}

static void mn_avev3_reset(struct eth_device *dev)
{
#ifdef CONFIG_AVE_RMII
	uint32_t val;
#endif

	mn_avev3_halt(dev);

#if defined(AVEV4)
#ifdef CONFIG_AVE_RMII
	val = mn_avev3_readreg(dev, MN_AVEV3_RSTCTL);
	val &= (~MN_AVEV3_RSTCTRL_RMIIRST);
	mn_avev3_writereg(dev, MN_AVEV3_RSTCTL, val);
#endif
	mn_avev3_writereg(dev, MN_AVEV3_GRR, MN_AVEV3_GRR_CLR);
	mdelay(MN_AVEV3_GRST_DELAY_MSEC);
#ifdef CONFIG_AVE_RMII
	val = mn_avev3_readreg(dev, MN_AVEV3_RSTCTL);
	val |= MN_AVEV3_RSTCTRL_RMIIRST;
	mn_avev3_writereg(dev, MN_AVEV3_RSTCTL, val);
#endif
#else 
#ifdef CONFIG_AVE_RMII
	val = mn_avev3_readreg(dev, MN_AVEV3_RSTCTL);
	val |= 0x00010000;
	mn_avev3_writereg(dev, MN_AVEV3_RSTCTL, val);
#endif
	mn_avev3_writereg(dev, MN_AVEV3_GRR, MN_AVEV3_GRR_CLR);
#endif 

	rx_pos = 0;
	rx_off = 0;
	tx_off = 0;
	tx_num = 0;
	tx_adj_buf = &tx_adj_PacketBuf[0] + (PKTALIGN -1);
	tx_adj_buf -= (unsigned long)tx_adj_buf % PKTALIGN;
}


static int mn_avev3_init(struct eth_device *dev, bd_t * bd)
{
	uint32_t val;
	uint64_t val2;
	int i;

#ifdef CONFIG_AVE_OMNIPHY
	CMN_REG_WRITE(0x5f800554, 0x00000001);
	CMN_REG_WRITE(0x5f800550, 0x00000003);
	CMN_REG_WRITE(0x5f800554, 0x00000003);
	CMN_REG_WRITE(0x5f800550, 0x00000007);
#endif

	mn_avev3_reset(dev);

	if(phyid == -1){
		for ( i = 0; i < 0x20; ++i ) {
			val = mn_avev3_mdio_read(dev, i, MII_PHYSID1);
			if((val & 0xFFFF) != 0xFFFF){
				phyid = i;
				break;
			}
		}
	}
#ifdef CONFIG_AVE_OMNIPHY_100M
  printf("*** AVE tentative ***   Force 100M-Half link setting \n");
  mn_avev3_mdio_write(dev, phyid, MII_BMCR, 0x8000);
  {
    int j;
    for(j=0; j<10000; j++){
      val = mn_avev3_mdio_read(dev, phyid, MII_BMCR);
      if( (val & 0x8000) == 0 ){
        break;
      }
    }
  }
  mn_avev3_mdio_write(dev, phyid, MII_BMCR, 0x2000);
  {
    uint32_t *ree;
    ree = (uint32_t *)0x5f800550;
    printf("  yk-dbg: 0x%p: 0x%08x \n", ree, (*(volatile uint32_t *)ree) );
    ree = (uint32_t *)0x5f800554;
    printf("  yk-dbg: 0x%p: 0x%08x \n", ree, (*(volatile uint32_t *)ree) );
    val = mn_avev3_mdio_read(dev, phyid, MII_BMCR);
    printf("  yk-dbg: BMCR: 0x%08x \n", val);
  }
  mn_avev3_mdio_write(dev, phyid, 20, 0x0000);
  mn_avev3_mdio_write(dev, phyid, 20, 0x0400);
  mn_avev3_mdio_write(dev, phyid, 20, 0x0000);
  mn_avev3_mdio_write(dev, phyid, 20, 0x0400);
  mn_avev3_mdio_write(dev, phyid, 23, 0x0007);
  mn_avev3_mdio_write(dev, phyid, 20, 0x4418);
#endif
#ifdef CONFIG_AVE_OMNIPHY
  printf("*** AVE ***  autonego link setting \n");
  mn_avev3_mdio_write(dev, phyid, MII_BMCR, 0x8000);
  {
    int j;
    for(j=0; j<10000; j++){
      val = mn_avev3_mdio_read(dev, phyid, MII_BMCR);
      if( (val & 0x8000) == 0 ){
        break;
      }
    }
  }
  {
    uint32_t *ree;
    ree = (uint32_t *)0x5f800550;
    printf("  yk-dbg: 0x%p: 0x%08x \n", ree, (*(volatile uint32_t *)ree) );
    ree = (uint32_t *)0x5f800554;
    printf("  yk-dbg: 0x%p: 0x%08x \n", ree, (*(volatile uint32_t *)ree) );
    val = mn_avev3_mdio_read(dev, phyid, MII_BMCR);
    printf("  yk-dbg: BMCR: 0x%08x \n", val);
  }
  mn_avev3_mdio_write(dev, phyid, 20, 0x0000);
  mn_avev3_mdio_write(dev, phyid, 20, 0x0400);
  mn_avev3_mdio_write(dev, phyid, 20, 0x0000);
  mn_avev3_mdio_write(dev, phyid, 20, 0x0400);
  mn_avev3_mdio_write(dev, phyid, 23, 0x4400);
  mn_avev3_mdio_write(dev, phyid, 20, 0x4415);
  mn_avev3_mdio_write(dev, phyid, 23, 0x3700);
  mn_avev3_mdio_write(dev, phyid, 20, 0x4413);
  mn_avev3_mdio_write(dev, phyid, MII_BMCR, 0x3300);
#endif

#if __BYTE_ORDER == __BIG_ENDIAN
	val = mn_avev3_readreg(dev, 0x4124);
	val |= 0x00010000;
	mn_avev3_writereg(dev, 0x4124, val);
	val = mn_avev3_readreg(dev, 0x4160);
	val |= 0x00010000;
	mn_avev3_writereg(dev, 0x4160, val);
#endif	

#if defined(AVEV4)
	rx_off = 2;
	tx_off = 0;

#ifdef CONFIG_AVE_RMII
        val = 0x08000000;
        mn_avev3_writereg(dev, MN_AVEV3_EMCR, val);
#endif

#else
	val = MN_AVEV3_EMCR_CONF;
	mn_avev3_writereg(dev, MN_AVEV3_EMCR, val);
	if(val & MN_AVEV3_EMCR_ROFE) {
		rx_off = 2;
	}
	if(val & MN_AVEV3_EMCR_TOFE) {
		tx_off = 2;
	}
#endif
	rx_siz = (PKTSIZE_ALIGN - rx_off);

	mn_avev3_writereg(dev, MN_AVEV3_TXDC, MN_AVEV3_TXDC_SIZE(1));
	mn_avev3_write_txdm(dev, 0, MN_AVEV3_TXDM_CMDSTS, 0);

	mn_avev3_writereg(dev, MN_AVEV3_RXDC, MN_AVEV3_RXDC_SIZE(PKTBUFSRX));
	for(i = 0; i < PKTBUFSRX; i++) {
		val2 = (uint64_t) net_rx_packets[i];
		MN_AVEV3_CACHE_FLS(val2, rx_siz + rx_off);
		val2 = MN_AVEV3_NCADDR(val2);
		mn_avev3_write_rxdm(dev, i, MN_AVEV3_RXDM_BUFPTR, (uint32_t)(val2 & 0x00000000ffffffff));
#ifndef AVE_ENABLE_32BIT_DESC
		mn_avev3_write_rxdm(dev, i, MN_AVEV3_RXDM_BUFPTR+1, (uint32_t)((val2& 0xffffffff00000000)>>32));
#endif
		mn_avev3_write_rxdm(dev, i, MN_AVEV3_RXDM_CMDSTS, rx_siz);
	}

	val = ((dev->enetaddr[0] << 8)|(dev->enetaddr[1]));
	mn_avev3_writereg(dev, MN_AVEV3_RXMAC2R, val);
	val = ((dev->enetaddr[2] << 24)|(dev->enetaddr[3] << 16)
	      |(dev->enetaddr[4] <<  8)|(dev->enetaddr[5]));
	mn_avev3_writereg(dev, MN_AVEV3_RXMAC1R, val);
	mn_avev3_writereg(dev, MN_AVEV3_GISR, MN_AVEV3_GISR_CLR);
	mn_avev3_writereg(dev, MN_AVEV3_GIMR, MN_AVEV3_GIMR_CLR);

#ifdef CONFIG_AVE_OMNIPHY_100M
        printf("*** AVE tentative ***   Force 100M-Half setting (RX) \n");
	val = (MN_AVEV3_RXCR_RXEN | MN_AVEV3_RXCR_MTU);
#else
	val = (MN_AVEV3_RXCR_RXEN | MN_AVEV3_RXCR_FDUP | MN_AVEV3_RXCR_MTU);
#endif
	mn_avev3_writereg(dev, MN_AVEV3_RXCR, val);

#if defined(AVEV4)
	mn_avev3_set_txspd(dev);
#endif

	mn_avev3_writereg(dev, MN_AVEV3_DESCC, (MN_AVEV3_DESCC_RDE|MN_AVEV3_DESCC_TDE));
	return 0;
}

static int mn_avev3_send(struct eth_device *dev, void *packet, int length)
{
	uint64_t algn;
	uint32_t val;
	uint64_t val2;
	volatile uchar *ptr = packet;	

	algn = (((uint64_t)ptr) & 0x3);
	if(algn != tx_off) {
		MN_AVEV3_MEMCPY(tx_adj_buf + tx_off, ptr, length);
		ptr = tx_adj_buf;
	} else {
		ptr -= tx_off;
	}
	if(length < MN_AVEV3_MIN_XMITSIZE) {
		volatile uchar *pad = ptr + tx_off + length;
		while(length < MN_AVEV3_MIN_XMITSIZE) {
			*pad++ = 0;
			++length;
		}
	}
	do {
		val = mn_avev3_read_txdm(dev, 0, MN_AVEV3_TXDM_CMDSTS);
	} while(val & MN_AVEV3_TXDM_OWN);

	MN_AVEV3_CACHE_FLS(ptr, length + tx_off);
	val2 = MN_AVEV3_NCADDR((uint64_t)ptr);
	mn_avev3_write_txdm(dev, 0, MN_AVEV3_TXDM_BUFPTR, (uint32_t)(val2 & 0x00000000ffffffff));
#ifndef AVE_ENABLE_32BIT_DESC
	mn_avev3_write_txdm(dev, 0, (MN_AVEV3_TXDM_BUFPTR+1), (uint32_t)((val2 & 0xffffffff00000000) >> 32));
#endif

#if defined(AVEV4)
	val = (MN_AVEV3_TXDM_OWN | length | MN_AVEV3_TX_CMDSTS_1ST | MN_AVEV3_TX_CMDSTS_LAST);
#else
	val = (MN_AVEV3_TXDM_OWN | length);
#endif
	mn_avev3_write_txdm(dev, 0, MN_AVEV3_TXDM_CMDSTS, val);
	++tx_num;

	do {
		val = mn_avev3_read_txdm(dev, 0, MN_AVEV3_TXDM_CMDSTS);
	} while(val & MN_AVEV3_TXDM_OWN);

	if((val & MN_AVEV3_TXDM_OK) == 0) {
		printf("%s:INFO:%d bad send sts=0x%08x\n", dev->name, tx_num, le32_to_cpu(val));
	}

	return 0;
}

static int mn_avev3_recv(struct eth_device *dev)
{
	uchar *ptr;
	int len;
	uint32_t sts;

	for(;;) {
		sts = mn_avev3_read_rxdm(dev, rx_pos, MN_AVEV3_RXDM_CMDSTS);
		if((sts & MN_AVEV3_RXDM_OWN) == 0) {
			break;
		}

		ptr = net_rx_packets[rx_pos] + rx_off;
		if(sts & MN_AVEV3_RXDM_OK) {
			len = sts & MN_AVEV3_RXDM_SIZE_MASK;
			MN_AVEV3_CACHE_INV_A(ptr, len);
			net_process_received_packet(ptr, len);
               } else {
                        printf("%s: bad packet [%d] sts=0x%08x ptr=%p (dropped)\n",
				dev->name, rx_pos, le32_to_cpu(sts), ptr);
		}

		MN_AVEV3_CACHE_FLS(net_rx_packets[rx_pos], rx_siz + rx_off);
		mn_avev3_write_rxdm(dev, rx_pos, MN_AVEV3_RXDM_CMDSTS, rx_siz);

		if(++rx_pos >= PKTBUFSRX) {
			rx_pos = 0;
		}
	}
	return 0;
}

int mn_avev3_initialize(int dev_num, int base_addr)
{
	struct eth_device *dev;
	uint32_t val;

	dev = malloc(sizeof(*dev));
	if (!dev) {
		return -1;
	}
	memset(dev, 0, sizeof(*dev));

	dev->iobase = base_addr;

	val = mn_avev3_readreg(dev, MN_AVEV3_IDR);
	if(val != MN_AVEV3_IDR_VALUE) {
		printf("%s: IDR fail(base=%#x)\n", __func__, base_addr);
		free(dev);
		return -1;
	}

	dev->init = mn_avev3_init;
	dev->halt = mn_avev3_halt;
	dev->send = mn_avev3_send;
	dev->recv = mn_avev3_recv;
	sprintf(dev->name, "%s-%hu", DRIVERNAME, dev_num);

	eth_register(dev);

	return 1;
}
