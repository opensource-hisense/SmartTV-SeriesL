/* $Id:$
 * uniphier_nand.h: NAND flash I/F dependent header
 *
 * Copyright (C) 2016 Socionext Corporation
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/mtd/nand.h> 

#ifndef _UNIPHIER_NAND_H_
#define _UNIPHIER_NAND_H_

#define CONFIG_MTD_NAND_UNIPHIER_IRQ
#define CONFIG_MTD_NAND_UNIPHIER_DMA
//#define CONFIG_MTD_NAND_UNIPHIER_CDMA_READ
#define CONFIG_MTD_NAND_UNIPHIER_ECC_CORRECT_BITS	(8)

//#
//# Architecture dependent settings
//#

#ifdef	CONFIG_MTD_NAND_UNIPHIER_BBM
/********** ORDER LIST *********/
#define NAND_MN2WS_AREA_ORDER_BDAM	0	/* BOOT, DATA, ALT,  BBM   */
#define NAND_MN2WS_AREA_ORDER_BDMA	1	/* BOOT, DATA, BBM,  ALT   */
#define NAND_MN2WS_AREA_ORDER_BADM	2	/* BOOT, ALT,  DATA, BBM   */
#define NAND_MN2WS_AREA_ORDER_BAMD	3	/* BOOT, ALT,  BBM,  DATA  */
#define NAND_MN2WS_AREA_ORDER_BMDA	4	/* BOOT, BBM,  DATA, ALT   */
#define NAND_MN2WS_AREA_ORDER_BMAD	5	/* BOOT, BBM,  ALT,  DATA  */
/******************************/
/* Area order: select from "ORDER LIST" */
#define NAND_MN2WS_AREA_ORDER		NAND_MN2WS_AREA_ORDER_BDAM
#endif	//CONFIG_MTD_NAND_UNIPHIER_BBM

// If maximum number of the corrected bit of each sector is larger than
// this definition, -EUCLEAN is notified to upper layer.
// If uncorrectable error occurs, -EBADMSG is notified to upper layer.
#define NAND_GPBC_CORRECTION_THRESHOLD	1

//#
//# Partition settings
//#

/* NOTICE: Partition setting has been moved to unit-init.c */

#define	NAND_GPBC_DRV_NAME		"uniphier-nand"

/* NAND Flash data bytes per ecc step */
#define	NAND_GPBC_ECCSIZE		1024

/* NAND Flash ecc bytes per step */
#define	NAND_GPBC_ECCBYTES \
		(14 * (CONFIG_MTD_NAND_UNIPHIER_ECC_CORRECT_BITS / 8))

#define NAND_GPBC_OOB_SKIP_SIZE		8

#define NAND_GPBC_MAX_FLASH_BANKS	2

#define NAND_GPBC_CMD_TIMEOUT	10000	//[ms]

#ifdef CONFIG_MTD_NAND_UNIPHIER_IRQ
	#define NAND_GPBC_IRQ_TIMEOUT	5000	//[ms]
#endif

#define NAND_GPBC_MAP00		0x00000000
#define NAND_GPBC_MAP01		0x04000000
#define NAND_GPBC_MAP10		0x08000000
#define NAND_GPBC_MAP11		0x0C000000

#define NAND_GPBC_BANK_SEL		24

#define NAND_GPBC_MAP10_ERASE			0x01
#define NAND_GPBC_MAP10_SPARE			0x41
#define NAND_GPBC_MAP10_DEFAULT			0x42
#define NAND_GPBC_MAP10_MAIN_SPARE		0x43

#ifdef CONFIG_MTD_NAND_UNIPHIER_DMA
#define NAND_GPBC_DMA_BURST			0x80
#define NAND_GPBC_MAP10_BEAT0_READ		(0x01002001 | (NAND_GPBC_DMA_BURST << 16))	//PP:1
#define NAND_GPBC_MAP10_BEAT0_WRITE		(0x01002101 | (NAND_GPBC_DMA_BURST << 16))	//PP:1

#ifdef CONFIG_MTD_NAND_UNIPHIER_CDMA_READ
#define NAND_GPBC_MAP10_BEAT0_CMD_DMA		(0x00000080)	//ch:0

#define NAND_GPBC_CMD_FLAGS_TRANS_DEFAULT	(0x0 << 12)
#define NAND_GPBC_CMD_FLAGS_TRANS_MAIN_SPARE	(0x1 << 12)
#define NAND_GPBC_CMD_FLAGS_TRANS_SPARE		(0x2 << 12)
#define NAND_GPBC_CMD_FLAGS_TRANS_MAIN_META	(0x3 << 12)
#define NAND_GPBC_CMD_FLAGS_WAIT_MEMCPY		(0x1 << 11)
#define NAND_GPBC_CMD_FLAGS_MEMCPY		(0x1 << 10)
#define NAND_GPBC_CMD_FLAGS_CONT		(0x1 << 9)
#define NAND_GPBC_CMD_FLAGS_INT			(0x1 << 8)
#define NAND_GPBC_CMD_FLAGS_BURST_LEN		(NAND_GPBC_DMA_BURST << 0)
#endif /* CONFIG_MTD_NAND_UNIPHIER_CDMA_READ */
#endif /* CONFIG_MTD_NAND_UNIPHIER_DMA */

#define NAND_GPBC_MAP11_CMD		0x0
#define NAND_GPBC_MAP11_ADR		0x1
#define NAND_GPBC_MAP11_DAT		0x2


#define NAND_GPBC_CMDP__BANK(x)		((x) << 24)
#define NAND_GPBC_CMDP__TYPE_CMD		0
#define NAND_GPBC_CMDP__TYPE_ADR		1
#define NAND_GPBC_CMDP__TYPE_DAT		2

// Enable banks
#define CONFIG_MTD_NAND_MN2WS_CHIP_BANK		(1)
#define NAND_GPBC_FLASH_BANKS		(CONFIG_MTD_NAND_MN2WS_CHIP_BANK)

/* offset of ECC in oob area */
#define NAND_GPBC_UNIT_OOB_ECCPOS	{0} /* dummy. This parameter is not used. */

/* device parameter setting */
#define MTD_NAND_MN2WS_ID_READ_NUM      8

/* bbt/bbm option */
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
#define NAND_GPBC_BBT_OPTION (NAND_BBT_FLEXIBLE		|\
			      NAND_BBT_CREATE		|\
			      NAND_BBT_WRITE		|\
			      NAND_BBT_1BIT		|\
			      NAND_BBT_VERSION		|\
			      NAND_BBT_PERCHIP		|\
			      NAND_BBT_NO_OOB		|\
			      NAND_BBT_SCAN2NDPAGE)
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
#define NAND_GPBC_BBT_OPTION (NAND_BBT_LASTBLOCK	|\
			      NAND_BBT_CREATE		|\
			      NAND_BBT_WRITE		|\
			      NAND_BBT_2BIT		|\
			      NAND_BBT_VERSION		|\
			      NAND_BBT_PERCHIP		|\
			      NAND_BBT_SCAN2NDPAGE)
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
/*
 * NAND_BBT_LASTBLOCK	save bbt/bbm from last block
 * NAND_BBT_CREATE	create bbt/bbm
 * NAND_BBT_WRITE	save bbt/bbm on MTD
 * NAND_BBT_1BIT	bit information of each block is 1 bit
 * NAND_BBT_2BIT	bit information of each block is 2 bit
 * NAND_BBT_VERSION	BBT/BBM version
 * NAND_BBT_PERCHIP	make bbt/bbm of each chip
 * NAND_BBT_SCAN2NDPAGE	scan bb only on page 1 and 2
 */

/* offset of the bbt/bbm pattern in the data area */
#define NAND_GPBC_BBT_OFFS		0
/* length of the bbt/bbm pattern, if 0 no pattern check is performed */
#define NAND_GPBC_BBT_LEN		4
/* offset of the bbt/bbm version in the data area */
#define NAND_GPBC_BBT_VEROFFS		4

/* maximum number of blocks to search for a bbt */
#define NAND_GPBC_UNIT_BBT_MAXBLOCKS		4

#ifndef	CONFIG_MTD_NAND_UNIPHIER_BBM
#define NAND_GPBC_UNIT_BBT_PATTERN_MASTER	{'b','B','t','0'}
#define NAND_GPBC_UNIT_BBT_PATTERN_MIRROR	{'1','t','B','b'}
#else	/* CONFIG_MTD_NAND_UNIPHIER_BBM */
#define NAND_GPBC_UNIT_BBT_PATTERN_MASTER	{'B','b','t','0'}
#define NAND_GPBC_UNIT_BBT_PATTERN_MIRROR	{'1','t','b','B'}
#endif	/* CONFIG_MTD_NAND_UNIPHIER_BBM */

/* offset of the bbt flag in the oob area of the page */
#define	NAND_GPBC_BBT_FLAG_OFFS		2

/* length of the bbt flag */
#define	NAND_GPBC_BBT_FLAG_LEN		1

/* bbt flag pattern */
#define	NAND_GPBC_BBT_FLAG_FF_PATTERN	{0xff}

/* NAND options(bottom 16bits will be overwritten by chip option ) */
#define NAND_GPBC_OPTIONS	NAND_NO_SUBPAGE_WRITE

#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
#define NAND_GPBC_BBT_OPTIONS	(NAND_BBT_USE_FLASH | NAND_BBT_NO_OOB)
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
#define NAND_GPBC_BBT_OPTIONS	(NAND_BBT_NO_OOB)
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */

/* offset of the bad block pattern in the oob area of the page */
#define	NAND_GPBC_BBTSCAN_OFFS	0

/* length of the bad block pattern */
#define	NAND_GPBC_BBTSCAN_LEN	2

/* bad block pattern */
#define	NAND_GPBC_BBTSCAN_FF_PATTERN	{0xff, 0xff}

#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
/* Area size: the number of block {chip0, chip1} */
#define NAND_MN2WS_AREA_BLOCKS_BOOT			\
{							\
	CONFIG_MTD_NAND_UNIPHIER_BOOT_NUM_BANK0,	\
	CONFIG_MTD_NAND_UNIPHIER_BOOT_NUM_BANK1,	\
}

#define NAND_MN2WS_AREA_BLOCKS_ALT			\
{							\
	CONFIG_MTD_NAND_UNIPHIER_ALT_NUM_BANK0,		\
	CONFIG_MTD_NAND_UNIPHIER_ALT_NUM_BANK1,		\
}
#endif

#ifdef CONFIG_MTD_NAND_UNIPHIER_CDMA_READ
struct nand_gpbc_cmd_desc_t {
	u64	next_ptr;
	u32	flash_page;
	u32	reserved0;
	u16	cmd_type;
	u16	cmd_flags;
	u32	sync_arg;
	u64	mem_ptr;
	u16	status;
	u16	reserved1;
	u32	reserved2;
	u64	sync_flag_ptr;
	u64	mem_copy_addr;
	u64	meta_data_addr;
};

#define NAND_GPBC_CDESC_STAT_CMP		0x8000
#define NAND_GPBC_CDESC_STAT_FAIL		0x4000
#define NAND_GPBC_CDESC_STAT_TMOUT		0x0400
#define NAND_GPBC_CDESC_STAT_MAX_ERR		0x03f0
#define NAND_GPBC_CDESC_STAT_DESC_ERR		0x0008
#define NAND_GPBC_CDESC_STAT_LOCKED_BLOCK	0x0004
#define NAND_GPBC_CDESC_STAT_UNSUP_ERR		0x0002
#define NAND_GPBC_CDESC_STAT_ACCESS_ERR		0x0001
#define NAND_GPBC_CDESC_STAT_ERR (NAND_GPBC_CDESC_STAT_TMOUT		| \
				  NAND_GPBC_CDESC_STAT_DESC_ERR		| \
				  NAND_GPBC_CDESC_STAT_LOCKED_BLOCK	| \
				  NAND_GPBC_CDESC_STAT_UNSUP_ERR	| \
				  NAND_GPBC_CDESC_STAT_ACCESS_ERR)

#define NAND_GPBC_CDESC_STAT_MAX_ERR_SHIFT	4
#endif /* CONFIG_MTD_NAND_UNIPHIER_CDMA_READ */

#define NAND_CORRECT_NO_PAGE -1

struct nand_buf {
	int head;
	int tail;
	uint8_t *buf;
};

struct nand_soc_flags {
	/*
		ECC enable/disable control flag
		  0: must not switch of ECC enable/disable
		  1: can switch of ECC enable/disable
		  2: can switch of ECC enable/disable by command condition
	*/
	u8 ecc_sw_ctrl;
	/* 
		set to cderren bit of NCIEAIR for compatible
		  0: cannot set. It is not necessary to set.
		  1: can set
	*/
	u8 set_cdma_erren;
	/*
		set to twb bit field of NCES register
		  0: cannot set. This bit is reserved.
		  1: can set
	*/
	u8 set_nces_twb;
	/*
		set to NDB register
		  0: cannot set. It is not necessary to set.
		  1: can set
	*/
	u8 set_ndb;
	/*
		bugfix of Data-DMA complete interrupt.
		  0: can not fix (need to use Command-DMA)
		  1: can fix
	*/
	u8 ddma_comp_fix;
};

struct nand_soc_flags_table {
	char *name;
	struct nand_soc_flags soc_flags;
};

struct nand_gpbc_info{
	struct mtd_info		mtd;
	struct nand_chip	chip;
	void __iomem		*regBase;
	void __iomem		*memBase;
	u32			nist[NAND_GPBC_MAX_FLASH_BANKS];
	u32			nie[NAND_GPBC_MAX_FLASH_BANKS];
	u32			dwordBuf;
	u32			sectSize2;
	u32			oobSkipSize;
	u32			sectSize3;
	u8			aId[MTD_NAND_MN2WS_ID_READ_NUM];
	u32			pagesPerBlock;
	int			nr_parts;
	struct mtd_partition	*parts;
	u8			*pPageBuf;
#ifdef CONFIG_MTD_NAND_UNIPHIER_DMA
	u8			*pPageBufRaw;
	dma_addr_t		pageBufPhys;
	u32			dmaAlignSize;
#ifdef CONFIG_MTD_NAND_UNIPHIER_CDMA_READ
	struct nand_gpbc_cmd_desc_t	*cmd_desc;
	dma_addr_t			cmd_desc_phys;
#endif /* CONFIG_MTD_NAND_UNIPHIER_CDMA_READ */
#endif /* CONFIG_MTD_NAND_UNIPHIER_DMA */

#ifdef CONFIG_MTD_NAND_UNIPHIER_IRQ
	unsigned int			irq;
	struct completion		complete;
#endif /* CONFIG_MTD_NAND_UNIPHIER_IRQ */
	int				flash_bank; /* currently selected chip */
	int				page;
	struct nand_buf			buf;
	struct device			*dev;
	uint32_t			status;
	struct nand_soc_flags		soc_flags;
};

struct nand_gpbc_timing {
	char *model;
	u32 ncdwr;
	u32 ncaad;
	u32 nrw;
	u32 nrdcp;
	u32 nrwlp;
	u32 nrwhp;
	u32 nces;
	u32 nrr;
};

#ifndef NAND_GPBC_BITFLIP_THRESHOLD
#define NAND_GPBC_BITFLIP_THRESHOLD NAND_GPBC_CORRECTION_THRESHOLD
#endif

#endif	/* _UNIPHIER_NAND_H_ */
