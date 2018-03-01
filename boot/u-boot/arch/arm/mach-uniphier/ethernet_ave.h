/*** CONFIDENTIAL ***/
/* Copyright (C) 2015, Socionext Inc. */
#ifndef  __MN_AVEV3_H__
#define  __MN_AVEV3_H__
#define AVEV4

#ifdef CONFIG_ARCH_UNIPHIER_LD11
#define CONFIG_AVE_RMII
#define CONFIG_AVE_OMNIPHY
#define AVE_ENABLE_32BIT_DESC
#endif

#define DRIVERNAME "mn_ethernet"
#if defined(AVEV4)
#define MN_AVEV3_GRST_DELAY_USEC	(5000)
#define MN_AVEV3_GRST_DELAY_MSEC	(MN_AVEV3_GRST_DELAY_USEC / 1000)
#else
#define MN_AVEV3_GRST_DELAY_USEC	(1000)
#endif

#define MN_AVEV3_MIN_XMITSIZE	(60)
#define MN_AVEV3_MAX_RXDC_NUM	(768)
#define MN_AVEV3_MAX_TXDC_NUM	(256)
#if PKTBUFSRX > MN_AVEV3_MAX_RXDC_NUM
#	error PKTBUFSRX too big (PKTBUFSRX > MN_AVEV3_MAXRXDC_NUM)
#endif

#define MN_AVEV3_MEMCPY(dst, src, len)	\
	memcpy( (void *)(dst), (const void *)(src), (size_t)(len))

#if 1
#define MN_AVEV3_NC_OFF			0	
#else
#define MN_AVEV3_NC_OFF			CONFIG_UNIPHIER_ETHERNET_NC_OFF	
#endif
#define MN_AVEV3_NCADDR(vaddr)	((uint64_t)(vaddr) + MN_AVEV3_NC_OFF)


#define MN_AVEV3_CACHE_INV_B(vaddr, len)		\
	invalidate_dcache_range((unit64_t)vaddr,	\
                (uint64_t)(vaddr)+(uint64_t)(len))
#define MN_AVEV3_CACHE_INV_A(vaddr, len)		\
	invalidate_dcache_range((uint64_t)vaddr,	\
                (uint64_t)(vaddr)+(uint64_t)(len))

#define MN_AVEV3_CACHE_FLS(vaddr, len)		\
	flush_dcache_range((uint64_t)(vaddr),	\
		(uint64_t)(vaddr)+(uint64_t)(len))

#define MN_AVEV3_REG(base, addr)	(*((volatile uint32_t *)((uint32_t)(base)+(addr))))
#define CMN_REG_WRITE(addr, val)  *(volatile uint32_t *)(addr) = val;

#if defined(AVEV4)
#define MN_AVEV3_IDR_VALUE		(0x41564534)
#else
#define MN_AVEV3_IDR_VALUE		(0x41564533)
#endif
#define MN_AVEV3_EMCR_CONF		(0xb0000000)
#define MN_AVEV3_REG_BASE		(0)		
#define MN_AVEV3_IDR			(MN_AVEV3_REG_BASE + 0x0000)
#define MN_AVEV3_VR			(MN_AVEV3_REG_BASE + 0x0004)
#define MN_AVEV3_GRR			(MN_AVEV3_REG_BASE + 0x0008)
#define MN_AVEV3_EMCR			(MN_AVEV3_REG_BASE + 0x000C)
#define MN_AVEV3_GIMR			(MN_AVEV3_REG_BASE + 0x0100)
#define MN_AVEV3_GISR			(MN_AVEV3_REG_BASE + 0x0104)
#define MN_AVEV3_GTICR			(MN_AVEV3_REG_BASE + 0x0108)
#define MN_AVEV3_TXCR			(MN_AVEV3_REG_BASE + 0x0200)
#define MN_AVEV3_RXCR			(MN_AVEV3_REG_BASE + 0x0204)
#define MN_AVEV3_RXMAC1R		(MN_AVEV3_REG_BASE + 0x0208)
#define MN_AVEV3_RXMAC2R		(MN_AVEV3_REG_BASE + 0x020C)
#define MN_AVEV3_PASCR			(MN_AVEV3_REG_BASE + 0x0210)
#define MN_AVEV3_MDIOCTR		(MN_AVEV3_REG_BASE + 0x0214)
#define MN_AVEV3_MDIOAR			(MN_AVEV3_REG_BASE + 0x0218)
#define MN_AVEV3_MDIOWDR		(MN_AVEV3_REG_BASE + 0x021C)
#define MN_AVEV3_MDIOSR			(MN_AVEV3_REG_BASE + 0x0220)
#define MN_AVEV3_MDIORDR		(MN_AVEV3_REG_BASE + 0x0224)
#define MN_AVEV3_JSPR			(MN_AVEV3_REG_BASE + 0x0228)
#define MN_AVEV3_DESCC			(MN_AVEV3_REG_BASE + 0x0300)
#define MN_AVEV3_TXDC			(MN_AVEV3_REG_BASE + 0x0304)
#define MN_AVEV3_RXDC			(MN_AVEV3_REG_BASE + 0x0308)
#define MN_AVEV3_TXDCP			(MN_AVEV3_REG_BASE + 0x031C)
#define MN_AVEV3_RXDCP			(MN_AVEV3_REG_BASE + 0x0320)
#define MN_AVEV3_TXDWBP			(MN_AVEV3_REG_BASE + 0x0334)
#define MN_AVEV3_RXDWBP			(MN_AVEV3_REG_BASE + 0x0338)
#define MN_AVEV3_IIRQC			(MN_AVEV3_REG_BASE + 0x034C)
#define MN_AVEV3_BFCR			(MN_AVEV3_REG_BASE + 0x0400)
#define MN_AVEV3_GFCR			(MN_AVEV3_REG_BASE + 0x0404)
#define MN_AVEV3_RXFC			(MN_AVEV3_REG_BASE + 0x0410)
#define MN_AVEV3_RXOVFFC		(MN_AVEV3_REG_BASE + 0x0414)

#define MN_AVEV3_TXDM			(MN_AVEV3_REG_BASE + 0x1000)

#ifdef AVE_ENABLE_32BIT_DESC
#define MN_AVEV3_RXDM			(MN_AVEV3_REG_BASE + 0x1800)
#else
#define MN_AVEV3_RXDM			(MN_AVEV3_REG_BASE + 0x1c00)
#endif

#ifdef CONFIG_AVE_RMII
#define MN_AVEV3_RSTCTL			(MN_AVEV3_REG_BASE + 0x8028)
#define MN_AVEV3_LINKSEL		(MN_AVEV3_REG_BASE + 0x8034)
#endif

#define MN_AVEV3_TXDM_CMDSTS		(0)
#define MN_AVEV3_TXDM_BUFPTR		(1)
#define MN_AVEV3_TXDM_OWN		(0x80000000)
#define MN_AVEV3_TXDM_OK		(0x08000000)
#define MN_AVEV3_TXDM_SIZE_MASK		(0x000007ff)
#define MN_AVEV3_RXDM_CMDSTS		MN_AVEV3_TXDM_CMDSTS
#define MN_AVEV3_RXDM_BUFPTR		MN_AVEV3_TXDM_BUFPTR
#define MN_AVEV3_RXDM_OWN		MN_AVEV3_TXDM_OWN
#define MN_AVEV3_RXDM_OK		MN_AVEV3_TXDM_OK
#define MN_AVEV3_RXDM_SIZE_MASK		MN_AVEV3_TXDM_SIZE_MASK

#define MN_AVEV3_GRR_GRST		(0x00000001)
#define MN_AVEV3_GRR_CLR		(0x00000000)
#define MN_AVEV3_EMCR_ROFE		(0x10000000)
#define MN_AVEV3_EMCR_TOFE		(0x20000000)
#define MN_AVEV3_RXCR_RXEN		(0x40000000)
#define MN_AVEV3_RXCR_FDUP		(0x00600000)
#define MN_AVEV3_RXCR_MTU		(1518)
#define MN_AVEV3_RXCR_HDUP		(0x00000000)
#define MN_AVEV3_TXCR_FLOCT		(0x00040000)
#define MN_AVEV3_GIMR_CLR		(0x00000000)
#define MN_AVEV3_GISR_CLR		(0xFFFFFFFF)
#define MN_AVEV3_MDIOCTR_RREQ		(0x00000008)
#define MN_AVEV3_MDIOCTR_WREQ		(0x00000004)
#define MN_AVEV3_MDIOSR_STS		(0x00000001)

#if defined(AVEV4)
#define MN_AVEV3_DESCC_RXDSTPST		(0x00100010)
#else
#define MN_AVEV3_DESCC_RXDSTPST		(0x00100000)
#endif
#define MN_AVEV3_DESCC_RDE		(0x00000100)
#define MN_AVEV3_DESCC_RXDSTP		(0x00000010)
#define MN_AVEV3_DESCC_TDE		(0x00000001)

#ifdef AVE_ENABLE_32BIT_DESC
#define MN_AVEV3_TXDC_SIZE(num)		((num)*0x00080000)
#define MN_AVEV3_RXDC_SIZE(num)		((num)*0x00080000)
#else
#define MN_AVEV3_TXDC_SIZE(num)		((num)*0x000c0000)
#define MN_AVEV3_RXDC_SIZE(num)		((num)*0x000c0000)
#endif

#if defined(AVEV4)
#define mdelay(n) ({uint32_t msec = (n); while (msec--) udelay(1000);})
#define MN_AVEV3_RSTCTRL_RMIIRST	(0x00010000)
#define MN_AVEV3_TX_CMDSTS_LAST		(0x02000000)
#define MN_AVEV3_TX_CMDSTS_1ST		(0x04000000)
#endif
#endif 
