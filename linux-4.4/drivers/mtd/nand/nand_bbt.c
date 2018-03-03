/*
 *  Overview:
 *   Bad block table support for the NAND driver
 *
 *  Copyright © 2004 Thomas Gleixner (tglx@linutronix.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Description:
 *
 * When nand_scan_bbt is called, then it tries to find the bad block table
 * depending on the options in the BBT descriptor(s). If no flash based BBT
 * (NAND_BBT_USE_FLASH) is specified then the device is scanned for factory
 * marked good / bad blocks. This information is used to create a memory BBT.
 * Once a new bad block is discovered then the "factory" information is updated
 * on the device.
 * If a flash based BBT is specified then the function first tries to find the
 * BBT on flash. If a BBT is found then the contents are read and the memory
 * based BBT is created. If a mirrored BBT is selected then the mirror is
 * searched too and the versions are compared. If the mirror has a greater
 * version number, then the mirror BBT is used to build the memory based BBT.
 * If the tables are not versioned, then we "or" the bad block information.
 * If one of the BBTs is out of date or does not exist it is (re)created.
 * If no BBT exists at all then the device is scanned for factory marked
 * good / bad blocks and the bad block tables are created.
 *
 * For manufacturer created BBTs like the one found on M-SYS DOC devices
 * the BBT is searched and read but never created
 *
 * The auto generated bad block table is located in the last good blocks
 * of the device. The table is mirrored, so it can be updated eventually.
 * The table is marked in the OOB area with an ident pattern and a version
 * number which indicates which of both tables is more up to date. If the NAND
 * controller needs the complete OOB area for the ECC information then the
 * option NAND_BBT_NO_OOB should be used (along with NAND_BBT_USE_FLASH, of
 * course): it moves the ident pattern and the version byte into the data area
 * and the OOB area will remain untouched.
 *
 * The table uses 2 bits per block
 * 11b:		block is good
 * 00b:		block is factory marked bad
 * 01b, 10b:	block is marked bad due to wear
 *
 * The memory bad block table uses the following scheme:
 * 00b:		block is good
 * 01b:		block is marked bad due to wear
 * 10b:		block is reserved (to protect the bbt area)
 * 11b:		block is factory marked bad
 *
 * Multichip devices like DOC store the bad block info per floor.
 *
 * Following assumptions are made:
 * - bbts start at a page boundary, if autolocated on a block boundary
 * - the space necessary for a bbt in FLASH does not exceed a block boundary
 *
 */

#include <linux/slab.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/bbm.h>
#include <linux/mtd/nand.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/export.h>
#include <linux/string.h>

#define BBT_BLOCK_GOOD		0x00
#define BBT_BLOCK_WORN		0x01
#define BBT_BLOCK_RESERVED	0x02
#define BBT_BLOCK_FACTORY_BAD	0x03

#define BBT_ENTRY_MASK		0x03
#define BBT_ENTRY_SHIFT		2

#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
static int getRsvBlock(struct mtd_info *psMtd, int chipNum);
static int whatKindOfBlock(struct mtd_info *psMtd, int blockNum);
static int registerAltBlock(struct mtd_info *psMtd, int pageNum, int rsvBlock);
static int setBbMap(struct mtd_info *psMtd, int blockNum);
static int readBlockExpOnePage(struct mtd_info *psMtd, int pageNum, u8 *pBlockBuf, u32 *erased_map);
static int readBlock(struct mtd_info *psMtd, int block, u8 *block_buf, u32 *erased_map);
static int setBbFlag(struct mtd_info *psMtd, int pageNum);
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */

static int nand_update_bbt(struct mtd_info *mtd, loff_t offs);

static inline uint8_t bbt_get_entry(struct nand_chip *chip, int block)
{
	uint8_t entry = chip->bbt[block >> BBT_ENTRY_SHIFT];
	entry >>= (block & BBT_ENTRY_MASK) * 2;
	return entry & BBT_ENTRY_MASK;
}

static inline void bbt_mark_entry(struct nand_chip *chip, int block,
		uint8_t mark)
{
	uint8_t msk = (mark & BBT_ENTRY_MASK) << ((block & BBT_ENTRY_MASK) * 2);
	chip->bbt[block >> BBT_ENTRY_SHIFT] |= msk;
}

static int check_pattern_no_oob(uint8_t *buf, struct nand_bbt_descr *td)
{
	if (memcmp(buf, td->pattern, td->len))
		return -1;
	return 0;
}

/**
 * check_pattern - [GENERIC] check if a pattern is in the buffer
 * @buf: the buffer to search
 * @len: the length of buffer to search
 * @paglen: the pagelength
 * @td: search pattern descriptor
 *
 * Check for a pattern at the given place. Used to search bad block tables and
 * good / bad block identifiers.
 */
static int check_pattern(uint8_t *buf, int len, int paglen, struct nand_bbt_descr *td)
{
	if (td->options & NAND_BBT_NO_OOB)
		return check_pattern_no_oob(buf, td);

	/* Compare the pattern */
	if (memcmp(buf + paglen + td->offs, td->pattern, td->len))
		return -1;

	return 0;
}

/**
 * check_short_pattern - [GENERIC] check if a pattern is in the buffer
 * @buf: the buffer to search
 * @td:	search pattern descriptor
 *
 * Check for a pattern at the given place. Used to search bad block tables and
 * good / bad block identifiers. Same as check_pattern, but no optional empty
 * check.
 */
static int check_short_pattern(uint8_t *buf, struct nand_bbt_descr *td)
{
	/* Compare the pattern */
	if (memcmp(buf + td->offs, td->pattern, td->len))
		return -1;
	return 0;
}

/**
 * add_marker_len - compute the length of the marker in data area
 * @td: BBT descriptor used for computation
 *
 * The length will be 0 if the marker is located in OOB area.
 */
static u32 add_marker_len(struct nand_bbt_descr *td)
{
	u32 len;

	if (!(td->options & NAND_BBT_NO_OOB))
		return 0;

	len = td->len;
	if (td->options & NAND_BBT_VERSION) {
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
		len += 4;
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
		len++;
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	}
	return len;
}

static inline unsigned int __bbcpy(void *dst, const void *src, unsigned int len)
{
	memcpy(dst, src, len);
	return len;
}

/**
 * read_bbt - [GENERIC] Read the bad block table starting from page
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @page: the starting page
 * @num: the number of bbt descriptors to read
 * @td: the bbt describtion table
 * @offs: block number offset in the table
 *
 * Read the bad block table starting from page.
 */
static int read_bbt(struct mtd_info *mtd, uint8_t *buf, int page, int num,
		struct nand_bbt_descr *td, int offs)
{
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
	int res, ret = 0;
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	int res, ret = 0, i, j, act = 0;
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	struct nand_chip *this = mtd->priv;
	size_t retlen, len, totlen;
	loff_t from;
#ifndef CONFIG_MTD_NAND_UNIPHIER_BBM
	int bits = td->options & NAND_BBT_NRBITS_MSK;
	uint8_t msk = (uint8_t)((1 << bits) - 1);
#endif /* !CONFIG_MTD_NAND_UNIPHIER_BBM */
	u32 marker_len;
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
	int chip = page >> (this->chip_shift - this->page_shift);
	struct nand_bbm *bbm = &this->psBbm[chip];
	size_t offset;

	totlen = bbm->bbmPages * mtd->writesize;
	if (totlen > (1 << this->bbt_erase_shift)) {
		pr_err("The size of BBM may not exceed 1 block.\n");
		return -EINVAL;
	}
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	int reserved_block_code = td->reserved_block_code;

	totlen = (num * bits) >> 3;
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	marker_len = add_marker_len(td);
	from = ((loff_t)page) << this->page_shift;

	while (totlen) {
		len = min(totlen, (size_t)(1 << this->bbt_erase_shift));
		if (marker_len) {
			/*
			 * In case the BBT marker is not in the OOB area it
			 * will be just in the first page.
			 */
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
			totlen -= marker_len;
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
			len -= marker_len;
			from += marker_len;
			marker_len = 0;
		}
		res = mtd_read(mtd, from, len, &retlen, buf);
		if (res < 0) {
			if (mtd_is_eccerr(res)) {
				pr_info("nand_bbt: ECC error in BBT at 0x%012llx\n",
					from & ~mtd->writesize);
				return res;
			} else if (mtd_is_bitflip(res)) {
				pr_info("nand_bbt: corrected error in BBT at 0x%012llx\n",
					from & ~mtd->writesize);
				ret = res;
			} else {
				pr_info("nand_bbt: error reading BBT\n");
				return res;
			}
		}

#ifndef CONFIG_MTD_NAND_UNIPHIER_BBM
		/* Analyse data */
		for (i = 0; i < len; i++) {
			uint8_t dat = buf[i];
			for (j = 0; j < 8; j += bits, act++) {
				uint8_t tmp = (dat >> j) & msk;
				if (tmp == msk)
					continue;
				if (reserved_block_code && (tmp == reserved_block_code)) {
					pr_info("nand_read_bbt: reserved block at 0x%012llx\n",
						 (loff_t)(offs + act) <<
						 this->bbt_erase_shift);
					bbt_mark_entry(this, offs + act,
							BBT_BLOCK_RESERVED);
					mtd->ecc_stats.bbtblocks++;
					continue;
				}
				/*
				 * Leave it for now, if it's matured we can
				 * move this message to pr_debug.
				 */
				pr_info("nand_read_bbt: bad block at 0x%012llx\n",
					 (loff_t)(offs + act) <<
					 this->bbt_erase_shift);
				/* Factory marked bad or worn out? */
				if (tmp == 0)
					bbt_mark_entry(this, offs + act,
							BBT_BLOCK_FACTORY_BAD);
				else
					bbt_mark_entry(this, offs + act,
							BBT_BLOCK_WORN);
				mtd->ecc_stats.badblocks++;
			}
		}
#endif /* !CONFIG_MTD_NAND_UNIPHIER_BBM */
		totlen -= len;
		from += len;
	}
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
	offset = 0;
	offset += __bbcpy(&bbm->usedMtBlocks, &buf[offset], sizeof(bbm->usedMtBlocks));
	offset += __bbcpy(&bbm->altBlocks, &buf[offset], sizeof(bbm->altBlocks));
	offset += __bbcpy(&this->bbt_td->pages[chip], &buf[offset], sizeof(this->bbt_td->pages[chip]));
	offset += __bbcpy(&this->bbt_md->pages[chip], &buf[offset], sizeof(this->bbt_md->pages[chip]));
	offset += __bbcpy(bbm->pBbMap, &buf[offset], bbm->bbMapSize);
	offset += __bbcpy(bbm->psBbList, &buf[offset], bbm->bbListSize);
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	return ret;
}

/**
 * read_abs_bbt - [GENERIC] Read the bad block table starting at a given page
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @td: descriptor for the bad block table
 * @chip: read the table for a specific chip, -1 read all chips; applies only if
 *        NAND_BBT_PERCHIP option is set
 *
 * Read the bad block table for all chips starting at a given page. We assume
 * that the bbt bits are in consecutive order.
 */
static int read_abs_bbt(struct mtd_info *mtd, uint8_t *buf, struct nand_bbt_descr *td, int chip)
{
	struct nand_chip *this = mtd->priv;
	int res = 0, i;

	if (td->options & NAND_BBT_PERCHIP) {
		int offs = 0;
		for (i = 0; i < this->numchips; i++) {
			if (chip == -1 || chip == i)
				res = read_bbt(mtd, buf, td->pages[i],
					this->chipsize >> this->bbt_erase_shift,
					td, offs);
			if (res)
				return res;
			offs += this->chipsize >> this->bbt_erase_shift;
		}
	} else {
		res = read_bbt(mtd, buf, td->pages[0],
				mtd->size >> this->bbt_erase_shift, td, 0);
		if (res)
			return res;
	}
	return 0;
}

#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
/* Scan read raw oob from flash, ant set data 0xff */
static int scan_read_raw_only_oob(struct mtd_info *mtd, uint8_t *buf,
				  loff_t offs, size_t len)
{
	struct mtd_oob_ops ops;
	size_t datlen;
	int res;

	ops.mode = MTD_OPS_RAW;
	ops.ooblen = mtd->oobsize;
	ops.ooboffs = 0;
	ops.datbuf = NULL;

	while (len > 0) {
		datlen = min(len, (size_t)mtd->writesize);
		memset(buf, 0xff, datlen);
		ops.oobbuf = buf + datlen;

		res = mtd_read_oob(mtd, offs, &ops);

		if (res)
			return res;

		buf += mtd->oobsize + mtd->writesize;
		len -= mtd->writesize;
		offs += mtd->writesize;
	}
	return 0;
}
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */

/* BBT marker is in the first page, no OOB */
static int scan_read_data(struct mtd_info *mtd, uint8_t *buf, loff_t offs,
			 struct nand_bbt_descr *td)
{
	size_t retlen;
	size_t len;

	len = td->len;
	if (td->options & NAND_BBT_VERSION)
		len++;

	return mtd_read(mtd, offs, len, &retlen, buf);
}

/**
 * scan_read_oob - [GENERIC] Scan data+OOB region to buffer
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @offs: offset at which to scan
 * @len: length of data region to read
 *
 * Scan read data from data+OOB. May traverse multiple pages, interleaving
 * page,OOB,page,OOB,... in buf. Completes transfer and returns the "strongest"
 * ECC condition (error or bitflip). May quit on the first (non-ECC) error.
 */
static int scan_read_oob(struct mtd_info *mtd, uint8_t *buf, loff_t offs,
			 size_t len)
{
	struct mtd_oob_ops ops;
	int res, ret = 0;

	ops.mode = MTD_OPS_PLACE_OOB;
	ops.ooboffs = 0;
	ops.ooblen = mtd->oobsize;

	while (len > 0) {
		ops.datbuf = buf;
		ops.len = min(len, (size_t)mtd->writesize);
		ops.oobbuf = buf + ops.len;

		res = mtd_read_oob(mtd, offs, &ops);
		if (res) {
			if (!mtd_is_bitflip_or_eccerr(res))
				return res;
			else if (mtd_is_eccerr(res) || !ret)
				ret = res;
		}

		buf += mtd->oobsize + mtd->writesize;
		len -= mtd->writesize;
		offs += mtd->writesize;
	}
	return ret;
}

static int scan_read(struct mtd_info *mtd, uint8_t *buf, loff_t offs,
			 size_t len, struct nand_bbt_descr *td)
{
	if (td->options & NAND_BBT_NO_OOB)
		return scan_read_data(mtd, buf, offs, td);
	else
		return scan_read_oob(mtd, buf, offs, len);
}

/* Scan write data with oob to flash */
static int scan_write_bbt(struct mtd_info *mtd, loff_t offs, size_t len,
			  uint8_t *buf, uint8_t *oob)
{
	struct mtd_oob_ops ops;
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
	struct nand_chip *this = mtd->priv;
	struct nand_bbm *bbm;
	int ret, reserveblock, chip;
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */

	ops.mode = MTD_OPS_PLACE_OOB;
	ops.ooboffs = 0;
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
	ops.ooblen = mtd->oobsize * (len / mtd->writesize);
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	ops.ooblen = mtd->oobsize;
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	ops.datbuf = buf;
	ops.oobbuf = oob;
	ops.len = len;

#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
	ops.retPage = -1;
	ret = this->fWriteOps(mtd, offs, &ops);
	if (ret) {
		if (ops.retPage == -1) {
			return -EIO;
		}
		setBbFlag(mtd, ops.retPage);
		setBbMap(mtd, ops.retPage >> (this->bbt_erase_shift - this->page_shift));

		chip = offs >> this->chip_shift;
		reserveblock = getRsvBlock(mtd, chip);
		if (reserveblock < 0) {
			return -ENOSPC;
		}
		bbm = &this->psBbm[chip];
		bbm->usedMtBlocks++;
		return reserveblock;
	}

	return 0;
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	return mtd_write_oob(mtd, offs, &ops);
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
}

static u32 bbt_get_ver_offs(struct mtd_info *mtd, struct nand_bbt_descr *td)
{
	u32 ver_offs = td->veroffs;

	if (!(td->options & NAND_BBT_NO_OOB))
		ver_offs += mtd->writesize;
	return ver_offs;
}

/**
 * read_abs_bbts - [GENERIC] Read the bad block table(s) for all chips starting at a given page
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @td: descriptor for the bad block table
 * @md:	descriptor for the bad block table mirror
 *
 * Read the bad block table(s) for all chips starting at a given page. We
 * assume that the bbt bits are in consecutive order.
 */
static void read_abs_bbts(struct mtd_info *mtd, uint8_t *buf,
			  struct nand_bbt_descr *td, struct nand_bbt_descr *md)
{
	struct nand_chip *this = mtd->priv;

	/* Read the primary version, if available */
	if (td->options & NAND_BBT_VERSION) {
		scan_read(mtd, buf, (loff_t)td->pages[0] << this->page_shift,
			      mtd->writesize, td);
		td->version[0] = buf[bbt_get_ver_offs(mtd, td)];
		pr_info("Bad block table at page %d, version 0x%02X\n",
			 td->pages[0], td->version[0]);
	}

	/* Read the mirror version, if available */
	if (md && (md->options & NAND_BBT_VERSION)) {
		scan_read(mtd, buf, (loff_t)md->pages[0] << this->page_shift,
			      mtd->writesize, md);
		md->version[0] = buf[bbt_get_ver_offs(mtd, md)];
		pr_info("Bad block table at page %d, version 0x%02X\n",
			 md->pages[0], md->version[0]);
	}
}

/* Scan a given block partially */
static int scan_block_fast(struct mtd_info *mtd, struct nand_bbt_descr *bd,
			   loff_t offs, uint8_t *buf, int numpages)
{
	struct mtd_oob_ops ops;
	int j, ret;

	ops.ooblen = mtd->oobsize;
	ops.oobbuf = buf;
	ops.ooboffs = 0;
	ops.datbuf = NULL;
	ops.mode = MTD_OPS_PLACE_OOB;

	for (j = 0; j < numpages; j++) {
		/*
		 * Read the full oob until read_oob is fixed to handle single
		 * byte reads for 16 bit buswidth.
		 */
		ret = mtd_read_oob(mtd, offs, &ops);
		/* Ignore ECC errors when checking for BBM */
		if (ret && !mtd_is_bitflip_or_eccerr(ret))
			return ret;

		if (check_short_pattern(buf, bd))
			return 1;

		offs += mtd->writesize;
	}
	return 0;
}

#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
static int create_alternate_list(struct mtd_info *mtd, int chip)
{
	struct nand_chip *this = mtd->priv;
	struct nand_bbm *bbm = &this->psBbm[chip];
	struct nand_bbList *bblist = bbm->psBbList;
	u32 *bbmap = bbm->pBbMap;
	int i, j, area, badblock, reserveblock;
	int blocks_per_chip = 1 << (this->chip_shift - this->bbt_erase_shift);
	const int bits = 32;

	for (i = 0; i < DIV_ROUND_UP(blocks_per_chip, bits); i++) {
		if (!bbmap[i]) {
			continue;
		}

		for (j = 0; j < bits; j++) {
			if (!(bbmap[i] & (1 << j))) {
				continue;
			}

			badblock = (i * bits + j) + chip * blocks_per_chip;
			area = whatKindOfBlock(mtd, badblock);
			switch (area) {
			case NAND_AREA_NORMAL:
				reserveblock = getRsvBlock(mtd, chip);
				if (reserveblock < 0) {
					return -ENOSPC;
				}
				bblist[bbm->altBlocks].badBlock = badblock;
				bblist[bbm->altBlocks].altBlock = reserveblock;
				bbm->altBlocks++;
				bbm->usedMtBlocks++;
				break;
			case NAND_AREA_MASTER:
			case NAND_AREA_MIRROR:
				reserveblock = getRsvBlock(mtd, chip);
				if (reserveblock < 0) {
					return -ENOSPC;
				}
				if (area == NAND_AREA_MASTER) {
					this->bbt_td->pages[chip] = reserveblock <<
						(this->bbt_erase_shift - this->page_shift);
				} else {
					this->bbt_md->pages[chip] = reserveblock <<
						(this->bbt_erase_shift - this->page_shift);
				}
				bbm->usedMtBlocks++;
				break;
			case NAND_AREA_MAINTAIN:
			case NAND_AREA_BOOT:
				break;
			default:
				return -EFAULT;
			}
		}
	}

	return 0;
}
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */

/**
 * create_bbt - [GENERIC] Create a bad block table by scanning the device
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @bd: descriptor for the good/bad block search pattern
 * @chip: create the table for a specific chip, -1 read all chips; applies only
 *        if NAND_BBT_PERCHIP option is set
 *
 * Create a bad block table by scanning the device for the given good/bad block
 * identify pattern.
 */
static int create_bbt(struct mtd_info *mtd, uint8_t *buf,
	struct nand_bbt_descr *bd, int chip)
{
	struct nand_chip *this = mtd->priv;
	int i, numblocks, numpages;
	int startblock;
	loff_t from;
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
	struct nand_bbm *bbm;
	struct nand_bbt_descr *td = this->bbt_td;
	struct nand_bbt_descr *md = this->bbt_md;
	int ret = 0;
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */

	pr_info("Scanning device for bad blocks\n");

	if (bd->options & NAND_BBT_SCAN2NDPAGE)
		numpages = 2;
	else
		numpages = 1;

	if (chip == -1) {
		numblocks = mtd->size >> this->bbt_erase_shift;
		startblock = 0;
		from = 0;
	} else {
		if (chip >= this->numchips) {
			pr_warn("create_bbt(): chipnr (%d) > available chips (%d)\n",
			       chip + 1, this->numchips);
			return -EINVAL;
		}
		numblocks = this->chipsize >> this->bbt_erase_shift;
		startblock = chip * numblocks;
		numblocks += startblock;
		from = (loff_t)startblock << this->bbt_erase_shift;
	}

	if (this->bbt_options & NAND_BBT_SCANLASTPAGE)
		from += mtd->erasesize - (mtd->writesize * numpages);

#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
	if (chip == -1) {
		chip = 0;
	}
	bbm = &this->psBbm[chip];
	memset(bbm->pBbMap, 0, bbm->bbMapSize);
	memset(bbm->psBbList, 0, bbm->bbListSize);
	bbm->altBlocks = 0;
	bbm->usedMtBlocks = 2;
	td->pages[chip] = (bbm->area[NAND_AREA_ID_BBM].startBlock + 1) <<
		(this->bbt_erase_shift - this->page_shift);
	md->pages[chip] = bbm->area[NAND_AREA_ID_BBM].startBlock <<
		(this->bbt_erase_shift - this->page_shift);
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */

	for (i = startblock; i < numblocks; i++) {
#ifndef CONFIG_MTD_NAND_UNIPHIER_BBM
		int ret;
#endif /* !CONFIG_MTD_NAND_UNIPHIER_BBM */

		BUG_ON(bd->options & NAND_BBT_NO_OOB);

		ret = scan_block_fast(mtd, bd, from, buf, numpages);
		if (ret < 0)
			return ret;

		if (ret) {
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
			setBbMap(mtd, i);
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
			bbt_mark_entry(this, i, BBT_BLOCK_FACTORY_BAD);
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
			pr_warn("Bad eraseblock %d at 0x%012llx\n",
				i, (unsigned long long)from);
			mtd->ecc_stats.badblocks++;
		}

		from += (1 << this->bbt_erase_shift);
	}

#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
	ret = create_alternate_list(mtd, chip);
	if (ret) {
		return ret;
	}
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */

	return 0;
}

#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
static int search_bbt_range(struct mtd_info *mtd, uint8_t *buf,
			    struct nand_bbt_descr *td, int startblock,
			    int searchblocks, int dir, int chip)
{
	struct nand_chip *this = mtd->priv;
	struct nand_bbm *bbm = &this->psBbm[chip];
	int block, actblock, readpages, bad = 0, i;
	int scanlen = mtd->writesize + mtd->oobsize;
	int blocktopage = this->bbt_erase_shift - this->page_shift;
	loff_t offs;
	size_t len;
	uint8_t *oob;

	for (block = 0; block < searchblocks; block++) {
		actblock = startblock + dir * block;
		offs = (loff_t)actblock << this->bbt_erase_shift;
		readpages = bbm->bbmPages;
		if ((this->bbt_options & NAND_BBT_SCAN2NDPAGE) &&
		    readpages < 2) {
			len = 2 * mtd->writesize;
		} else {
			len = readpages * mtd->writesize;
		}
		scan_read_raw_only_oob(mtd, buf, offs, len);

		/* check bad */
		i = 0;
		do {
			oob = buf + (mtd->writesize + mtd->oobsize) * i;
			if (check_short_pattern(oob + mtd->writesize,
						this->badblock_pattern)) {
				bad = 1;
				break;
			}
			i++;
		} while ((this->bbt_options & NAND_BBT_SCAN2NDPAGE) && i < 2);
		if (bad) {
			bad = 0;
			continue;
		}

		if (td->options & NAND_BBT_NO_OOB) {
			/* check bbt flag */
			for (i = 0; i < readpages; i++) {
				oob = buf + (mtd->writesize + mtd->oobsize) * i;
				if (!check_short_pattern(oob + mtd->writesize,
							 this->bbt_flag)) {
					bad = 1;
					break;
				}
			}
			if (bad) {
				bad = 0;
				continue;
			}

			/* Read first page */
			scan_read(mtd, buf, offs, mtd->writesize, td);
		} else {
			/* skip read because already read by scan_read_raw_only_oob */
		}

		/* check bbt pattern and version */
		if (!check_pattern(buf, scanlen, mtd->writesize, td)) {
			td->pages[chip] = actblock << blocktopage;
			if (td->options & NAND_BBT_VERSION) {
				offs = bbt_get_ver_offs(mtd, td);
				td->version[chip] = buf[offs];
			}
			break;
		}
	}

	return block < searchblocks ? 0 : -ENODEV;
}
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */

/**
 * search_bbt - [GENERIC] scan the device for a specific bad block table
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @td: descriptor for the bad block table
 *
 * Read the bad block table by searching for a given ident pattern. Search is
 * preformed either from the beginning up or from the end of the device
 * downwards. The search starts always at the start of a block. If the option
 * NAND_BBT_PERCHIP is given, each chip is searched for a bbt, which contains
 * the bad block information of this chip. This is necessary to provide support
 * for certain DOC devices.
 *
 * The bbt ident pattern resides in the oob area of the first page in a block.
 */
static int search_bbt(struct mtd_info *mtd, uint8_t *buf, struct nand_bbt_descr *td)
{
	struct nand_chip *this = mtd->priv;
	int i, chips;
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
	int startblock, dir;
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	int startblock, block, dir;
	int scanlen = mtd->writesize + mtd->oobsize;
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	int bbtblocks;
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
	struct nand_bbm *bbm;
	int searchblocks;
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	int blocktopage = this->bbt_erase_shift - this->page_shift;
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */

	/* Search direction top -> down? */
	if (td->options & NAND_BBT_LASTBLOCK) {
		startblock = (mtd->size >> this->bbt_erase_shift) - 1;
		dir = -1;
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
	} else if (td->options & NAND_BBT_FLEXIBLE) {
		startblock = 0; /* dummy. this parameter is overwitten. */
		dir = -1;
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	} else {
		startblock = 0;
		dir = 1;
	}

	/* Do we have a bbt per chip? */
	if (td->options & NAND_BBT_PERCHIP) {
		chips = this->numchips;
		bbtblocks = this->chipsize >> this->bbt_erase_shift;
		startblock &= bbtblocks - 1;
	} else {
		chips = 1;
		bbtblocks = mtd->size >> this->bbt_erase_shift;
	}

	for (i = 0; i < chips; i++) {
		/* Reset version information */
		td->version[i] = 0;
		td->pages[i] = -1;
		/* Scan the maximum number of blocks */
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
		bbm = &this->psBbm[i];
		if (td->options & NAND_BBT_FLEXIBLE) {
			searchblocks = bbm->area[NAND_AREA_ID_BBM].blockNum;
			startblock = bbm->area[NAND_AREA_ID_BBM].startBlock +
				searchblocks - 1;

			if (search_bbt_range(mtd, buf, td, startblock,
					     searchblocks, dir, i)) {
				searchblocks = bbm->area[NAND_AREA_ID_ALT].blockNum;
				startblock = bbm->area[NAND_AREA_ID_ALT].startBlock +
					searchblocks - 1;

				search_bbt_range(mtd, buf, td, startblock,
						 searchblocks, dir, i);
			}
		} else {
			search_bbt_range(mtd, buf, td, startblock,
					 bbm->mtBlocks, dir, i);
		}
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
		for (block = 0; block < td->maxblocks; block++) {

			int actblock = startblock + dir * block;
			loff_t offs = (loff_t)actblock << this->bbt_erase_shift;

			/* Read first page */
			scan_read(mtd, buf, offs, mtd->writesize, td);
			if (!check_pattern(buf, scanlen, mtd->writesize, td)) {
				td->pages[i] = actblock << blocktopage;
				if (td->options & NAND_BBT_VERSION) {
					offs = bbt_get_ver_offs(mtd, td);
					td->version[i] = buf[offs];
				}
				break;
			}
		}
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
		startblock += this->chipsize >> this->bbt_erase_shift;
	}
	/* Check, if we found a bbt for each requested chip */
	for (i = 0; i < chips; i++) {
		if (td->pages[i] == -1)
			pr_warn("Bad block table not found for chip %d\n", i);
		else
			pr_info("Bad block table found at page %d, version 0x%02X\n",
				td->pages[i], td->version[i]);
	}
	return 0;
}

/**
 * search_read_bbts - [GENERIC] scan the device for bad block table(s)
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @td: descriptor for the bad block table
 * @md: descriptor for the bad block table mirror
 *
 * Search and read the bad block table(s).
 */
static void search_read_bbts(struct mtd_info *mtd, uint8_t *buf,
			     struct nand_bbt_descr *td,
			     struct nand_bbt_descr *md)
{
	/* Search the primary table */
	search_bbt(mtd, buf, td);

	/* Search the mirror table */
	if (md)
		search_bbt(mtd, buf, md);
}

/**
 * write_bbt - [GENERIC] (Re)write the bad block table
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @td: descriptor for the bad block table
 * @md: descriptor for the bad block table mirror
 * @chipsel: selector for a specific chip, -1 for all
 *
 * (Re)write the bad block table.
 */
static int write_bbt(struct mtd_info *mtd, uint8_t *buf,
		     struct nand_bbt_descr *td, struct nand_bbt_descr *md,
		     int chipsel)
{
	struct nand_chip *this = mtd->priv;
	struct erase_info einfo;
	int i, res, chip = 0;
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
	int page, offs, numblocks;
	int nrchips, ooboffs;
	size_t len = 0;
	struct nand_bbm * bbm;
	int reserveblock;
	loff_t to;
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	int bits, startblock, dir, page, offs, numblocks, sft, sftmsk;
	int nrchips, pageoffs, ooboffs;
	uint8_t msk[4];
	uint8_t rcode = td->reserved_block_code;
	size_t retlen, len = 0;
	loff_t to;
	struct mtd_oob_ops ops;

	ops.ooblen = mtd->oobsize;
	ops.ooboffs = 0;
	ops.datbuf = NULL;
	ops.mode = MTD_OPS_PLACE_OOB;

	if (!rcode)
		rcode = 0xff;
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	/* Write bad block table per chip rather than per device? */
	if (td->options & NAND_BBT_PERCHIP) {
		numblocks = (int)(this->chipsize >> this->bbt_erase_shift);
		/* Full device write or specific chip? */
		if (chipsel == -1) {
			nrchips = this->numchips;
		} else {
			nrchips = chipsel + 1;
			chip = chipsel;
		}
	} else {
		numblocks = (int)(mtd->size >> this->bbt_erase_shift);
		nrchips = 1;
	}

	/* Loop through the chips */
	for (; chip < nrchips; chip++) {
		/*
		 * There was already a version of the table, reuse the page
		 * This applies for absolute placement too, as we have the
		 * page nr. in td->pages.
		 */
		if (td->pages[chip] != -1) {
			page = td->pages[chip];
			goto write;
		}

#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
		page = 0; /* set dummy param to avoid warning. */
		pr_err("not reach when CONFIG_MTD_NAND_UNIPHIER_BBM is enabled!! \n");
		BUG();
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
		/*
		 * Automatic placement of the bad block table. Search direction
		 * top -> down?
		 */
		if (td->options & NAND_BBT_LASTBLOCK) {
			startblock = numblocks * (chip + 1) - 1;
			dir = -1;
		} else {
			startblock = chip * numblocks;
			dir = 1;
		}

		for (i = 0; i < td->maxblocks; i++) {
			int block = startblock + dir * i;
			/* Check, if the block is bad */
			switch (bbt_get_entry(this, block)) {
			case BBT_BLOCK_WORN:
			case BBT_BLOCK_FACTORY_BAD:
				continue;
			}
			page = block <<
				(this->bbt_erase_shift - this->page_shift);
			/* Check, if the block is used by the mirror table */
			if (!md || md->pages[chip] != page)
				goto write;
		}
		pr_err("No space left to write bad block table\n");
		return -ENOSPC;
#endif/* CONFIG_MTD_NAND_UNIPHIER_BBM */

	write:

#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
		to = ((loff_t)page) << this->page_shift;

		bbm = &this->psBbm[chip];
		memset(buf, 0xff, bbm->bbmPages * (mtd->writesize + mtd->oobsize));
		offs = 0;
		ooboffs = bbm->bbmPages * mtd->writesize;
		if (td->options & NAND_BBT_NO_OOB) {
			struct nand_bbt_descr *bbtflag = this->bbt_flag;
			/* set bbt flag*/
			for (i = 0; i < bbm->bbmPages; i++) {
				buf[ooboffs + bbtflag->offs] = 0;
				ooboffs += mtd->oobsize;
			}
			/* pattern */
			offs += __bbcpy(buf, td->pattern, td->len);
			/* version */
			if (td->options & NAND_BBT_VERSION) {
				len = roundup(sizeof(td->version[chip]), sizeof(u32));
				buf[offs] = td->version[chip];
				offs += len;
			}
		} else {
			for (i = 0; i < bbm->bbmPages; i++) {
				/* pattern */
				memcpy(buf + ooboffs + td->offs, td->pattern, td->len);
				/* version */
				if (td->options & NAND_BBT_VERSION) {
					buf[ooboffs + td->veroffs] = td->version[chip];
				}
				ooboffs += mtd->oobsize;
			}
		}
		offs += __bbcpy(buf + offs, &bbm->usedMtBlocks, sizeof(bbm->usedMtBlocks));
		offs += __bbcpy(buf + offs, &bbm->altBlocks, sizeof(bbm->altBlocks));
		offs += __bbcpy(buf + offs, &this->bbt_td->pages[chip], sizeof(this->bbt_td->pages[chip]));
		offs += __bbcpy(buf + offs, &this->bbt_md->pages[chip], sizeof(this->bbt_md->pages[chip]));
		offs += __bbcpy(buf + offs, bbm->pBbMap, bbm->bbMapSize);
		offs += __bbcpy(buf + offs, bbm->psBbList, bbm->bbListSize);
		len = ALIGN(offs, mtd->writesize);

		memset(&einfo, 0, sizeof(einfo));
		einfo.mtd = mtd;
		einfo.addr = to;
		einfo.len = 1 << this->bbt_erase_shift;
		einfo.fail_addr = MTD_FAIL_ADDR_UNKNOWN;
		res = nand_erase_nand(mtd, &einfo, NAND_ALLOW_BBT);
		if (res) {
			if (einfo.fail_addr == MTD_FAIL_ADDR_UNKNOWN) {
				return -EIO;
			}
			setBbFlag(mtd, einfo.fail_addr >> this->page_shift);
			setBbMap(mtd, einfo.fail_addr >> this->bbt_erase_shift);

			reserveblock = getRsvBlock(mtd, chip);
			if (reserveblock < 0) {
				return -ENOSPC;
			}
			bbm->usedMtBlocks++;
			return reserveblock;
		}

		res = scan_write_bbt(mtd, to, len, buf, &buf[len]);
		if (res < 0) {
			goto outerr;
		} else if (res) {
			return res;
		}
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
		/* Set up shift count and masks for the flash table */
		bits = td->options & NAND_BBT_NRBITS_MSK;
		msk[2] = ~rcode;
		switch (bits) {
		case 1: sft = 3; sftmsk = 0x07; msk[0] = 0x00; msk[1] = 0x01;
			msk[3] = 0x01;
			break;
		case 2: sft = 2; sftmsk = 0x06; msk[0] = 0x00; msk[1] = 0x01;
			msk[3] = 0x03;
			break;
		case 4: sft = 1; sftmsk = 0x04; msk[0] = 0x00; msk[1] = 0x0C;
			msk[3] = 0x0f;
			break;
		case 8: sft = 0; sftmsk = 0x00; msk[0] = 0x00; msk[1] = 0x0F;
			msk[3] = 0xff;
			break;
		default: return -EINVAL;
		}

		to = ((loff_t)page) << this->page_shift;

		/* Must we save the block contents? */
		if (td->options & NAND_BBT_SAVECONTENT) {
			/* Make it block aligned */
			to &= ~(((loff_t)1 << this->bbt_erase_shift) - 1);
			len = 1 << this->bbt_erase_shift;
			res = mtd_read(mtd, to, len, &retlen, buf);
			if (res < 0) {
				if (retlen != len) {
					pr_info("nand_bbt: error reading block for writing the bad block table\n");
					return res;
				}
				pr_warn("nand_bbt: ECC error while reading block for writing bad block table\n");
			}
			/* Read oob data */
			ops.ooblen = (len >> this->page_shift) * mtd->oobsize;
			ops.oobbuf = &buf[len];
			res = mtd_read_oob(mtd, to + mtd->writesize, &ops);
			if (res < 0 || ops.oobretlen != ops.ooblen)
				goto outerr;

			/* Calc the byte offset in the buffer */
			pageoffs = page - (int)(to >> this->page_shift);
			offs = pageoffs << this->page_shift;
			/* Preset the bbt area with 0xff */
			memset(&buf[offs], 0xff, (size_t)(numblocks >> sft));
			ooboffs = len + (pageoffs * mtd->oobsize);

		} else if (td->options & NAND_BBT_NO_OOB) {
			ooboffs = 0;
			offs = td->len;
			/* The version byte */
			if (td->options & NAND_BBT_VERSION)
				offs++;
			/* Calc length */
			len = (size_t)(numblocks >> sft);
			len += offs;
			/* Make it page aligned! */
			len = ALIGN(len, mtd->writesize);
			/* Preset the buffer with 0xff */
			memset(buf, 0xff, len);
			/* Pattern is located at the begin of first page */
			memcpy(buf, td->pattern, td->len);
		} else {
			/* Calc length */
			len = (size_t)(numblocks >> sft);
			/* Make it page aligned! */
			len = ALIGN(len, mtd->writesize);
			/* Preset the buffer with 0xff */
			memset(buf, 0xff, len +
			       (len >> this->page_shift)* mtd->oobsize);
			offs = 0;
			ooboffs = len;
			/* Pattern is located in oob area of first page */
			memcpy(&buf[ooboffs + td->offs], td->pattern, td->len);
		}

		if (td->options & NAND_BBT_VERSION)
			buf[ooboffs + td->veroffs] = td->version[chip];

		/* Walk through the memory table */
		for (i = 0; i < numblocks; i++) {
			uint8_t dat;
			int sftcnt = (i << (3 - sft)) & sftmsk;
			dat = bbt_get_entry(this, chip * numblocks + i);
			/* Do not store the reserved bbt blocks! */
			buf[offs + (i >> sft)] &= ~(msk[dat] << sftcnt);
		}

		memset(&einfo, 0, sizeof(einfo));
		einfo.mtd = mtd;
		einfo.addr = to;
		einfo.len = 1 << this->bbt_erase_shift;
		res = nand_erase_nand(mtd, &einfo, 1);
		if (res < 0)
			goto outerr;

		res = scan_write_bbt(mtd, to, len, buf,
				td->options & NAND_BBT_NO_OOB ? NULL :
				&buf[len]);
		if (res < 0)
			goto outerr;
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */

		pr_info("Bad block table written to 0x%012llx, version 0x%02X\n",
			 (unsigned long long)to, td->version[chip]);

		/* Mark it as used */
		td->pages[chip] = page;
	}
	return 0;

 outerr:
	pr_warn("nand_bbt: error while writing bad block table %d\n", res);
	return res;
}

#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
static int write_bbts(struct mtd_info *mtd, uint8_t *buf,
		      struct nand_bbt_descr *td, struct nand_bbt_descr *md,
		      int chipsel)
{
	struct nand_chip *this = mtd->priv;
	int td_written = 0, md_written = 0;
	int res, chip = chipsel;

	BUG_ON(chipsel == -1);

	while (!td_written || !md_written) {
		while (!td_written) {
			res = write_bbt(mtd, buf, td, md, chipsel);
			if (!res) {
				td_written = 1;
			} else if (res > 0) {
				td->pages[chip] = res <<
					(this->bbt_erase_shift - this->page_shift);
				td->version[chip]++;
				md->version[chip] = td->version[chip];
				md_written = 0;
			} else {
				return -ENOSPC;
			}
		}

		while (!md_written) {
			res = write_bbt(mtd, buf, md, td, chipsel);
			if (!res) {
				md_written = 1;
			} else if (res > 0) {
				md->pages[chip] = res <<
					(this->bbt_erase_shift - this->page_shift);
				md->version[chip]++;
				td->version[chip] = md->version[chip];
				td_written = 0;
			} else {
				return -ENOSPC;
			}
		}
	}

	return 0;
}
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */

/**
 * nand_memory_bbt - [GENERIC] create a memory based bad block table
 * @mtd: MTD device structure
 * @bd: descriptor for the good/bad block search pattern
 *
 * The function creates a memory based bbt by scanning the device for
 * manufacturer / software marked good / bad blocks.
 */
static inline int nand_memory_bbt(struct mtd_info *mtd, struct nand_bbt_descr *bd)
{
	struct nand_chip *this = mtd->priv;

	return create_bbt(mtd, this->buffers->databuf, bd, -1);
}

/**
 * check_create - [GENERIC] create and write bbt(s) if necessary
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @bd: descriptor for the good/bad block search pattern
 *
 * The function checks the results of the previous call to read_bbt and creates
 * / updates the bbt(s) if necessary. Creation is necessary if no bbt was found
 * for the chip/device. Update is necessary if one of the tables is missing or
 * the version nr. of one table is less than the other.
 */
static int check_create(struct mtd_info *mtd, uint8_t *buf, struct nand_bbt_descr *bd)
{
	int i, chips, writeops, create, chipsel, res, res2;
	struct nand_chip *this = mtd->priv;
	struct nand_bbt_descr *td = this->bbt_td;
	struct nand_bbt_descr *md = this->bbt_md;
	struct nand_bbt_descr *rd, *rd2;

	/* Do we have a bbt per chip? */
	if (td->options & NAND_BBT_PERCHIP)
		chips = this->numchips;
	else
		chips = 1;

	for (i = 0; i < chips; i++) {
		writeops = 0;
		create = 0;
		rd = NULL;
		rd2 = NULL;
		res = res2 = 0;
		/* Per chip or per device? */
		chipsel = (td->options & NAND_BBT_PERCHIP) ? i : -1;
		/* Mirrored table available? */
		if (md) {
			if (td->pages[i] == -1 && md->pages[i] == -1) {
				create = 1;
				writeops = 0x03;
			} else if (td->pages[i] == -1) {
				rd = md;
				writeops = 0x01;
			} else if (md->pages[i] == -1) {
				rd = td;
				writeops = 0x02;
			} else if (td->version[i] == md->version[i]) {
				rd = td;
				if (!(td->options & NAND_BBT_VERSION))
					rd2 = md;
			} else if (((int8_t)(td->version[i] - md->version[i])) > 0) {
				rd = td;
				writeops = 0x02;
			} else {
				rd = md;
				writeops = 0x01;
			}
		} else {
			if (td->pages[i] == -1) {
				create = 1;
				writeops = 0x01;
			} else {
				rd = td;
			}
		}

		if (create) {
			/* Create the bad block table by scanning the device? */
			if (!(td->options & NAND_BBT_CREATE))
				continue;

			/* Create the table in memory by scanning the chip(s) */
			if (!(this->bbt_options & NAND_BBT_CREATE_EMPTY))
				create_bbt(mtd, buf, bd, chipsel);

			td->version[i] = 1;
			if (md)
				md->version[i] = 1;
		}

		/* Read back first? */
		if (rd) {
			res = read_abs_bbt(mtd, buf, rd, chipsel);
			if (mtd_is_eccerr(res)) {
				/* Mark table as invalid */
				rd->pages[i] = -1;
				rd->version[i] = 0;
				i--;
				continue;
			}
		}
		/* If they weren't versioned, read both */
		if (rd2) {
			res2 = read_abs_bbt(mtd, buf, rd2, chipsel);
			if (mtd_is_eccerr(res2)) {
				/* Mark table as invalid */
				rd2->pages[i] = -1;
				rd2->version[i] = 0;
				i--;
				continue;
			}
		}

		/* Scrub the flash table(s)? */
		if (mtd_is_bitflip(res) || mtd_is_bitflip(res2))
			writeops = 0x03;

		/* Update version numbers before writing */
		if (md) {
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
			if (((int8_t)(td->version[i] - md->version[i])) > 0) {
				md->version[i] = td->version[i];
			} else {
				td->version[i] = md->version[i];
			}
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
			td->version[i] = max(td->version[i], md->version[i]);
			md->version[i] = td->version[i];
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
		}

		/* Write the bad block table to the device? */
		if ((writeops & 0x01) && (td->options & NAND_BBT_WRITE)) {
			res = write_bbt(mtd, buf, td, md, chipsel);
			if (res < 0) {
				return res;
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
			} else if (res) {
				td->pages[chipsel] = res <<
					(this->bbt_erase_shift - this->page_shift);
				td->version[chipsel]++;
				md->version[chipsel] = td->version[chipsel];
				res = write_bbts(mtd, buf, td, md, chipsel);
				if (res < 0) {
					return res;
				}
				writeops = 0;
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
			}
		}

		/* Write the mirror bad block table to the device? */
		if ((writeops & 0x02) && md && (md->options & NAND_BBT_WRITE)) {
			res = write_bbt(mtd, buf, md, td, chipsel);
			if (res < 0) {
				return res;
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
			} else if (res) {
				md->pages[chipsel] = res <<
					(this->bbt_erase_shift - this->page_shift);
				md->version[chipsel]++;
				td->version[chipsel] = md->version[chipsel];
				res = write_bbts(mtd, buf, md, td, chipsel);
				if (res < 0) {
					return res;
				}
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
			}
		}
	}
	return 0;
}

#ifndef CONFIG_MTD_NAND_UNIPHIER_BBM
/**
 * mark_bbt_regions - [GENERIC] mark the bad block table regions
 * @mtd: MTD device structure
 * @td: bad block table descriptor
 *
 * The bad block table regions are marked as "bad" to prevent accidental
 * erasures / writes. The regions are identified by the mark 0x02.
 */
static void mark_bbt_region(struct mtd_info *mtd, struct nand_bbt_descr *td)
{
	struct nand_chip *this = mtd->priv;
	int i, j, chips, block, nrblocks, update;
	uint8_t oldval;

	/* Do we have a bbt per chip? */
	if (td->options & NAND_BBT_PERCHIP) {
		chips = this->numchips;
		nrblocks = (int)(this->chipsize >> this->bbt_erase_shift);
	} else {
		chips = 1;
		nrblocks = (int)(mtd->size >> this->bbt_erase_shift);
	}

	for (i = 0; i < chips; i++) {
		if ((td->options & NAND_BBT_ABSPAGE) ||
		    !(td->options & NAND_BBT_WRITE)) {
			if (td->pages[i] == -1)
				continue;
			block = td->pages[i] >> (this->bbt_erase_shift - this->page_shift);
			oldval = bbt_get_entry(this, block);
			bbt_mark_entry(this, block, BBT_BLOCK_RESERVED);
			if ((oldval != BBT_BLOCK_RESERVED) &&
					td->reserved_block_code)
				nand_update_bbt(mtd, (loff_t)block <<
						this->bbt_erase_shift);
			continue;
		}
		update = 0;
		if (td->options & NAND_BBT_LASTBLOCK)
			block = ((i + 1) * nrblocks) - td->maxblocks;
		else
			block = i * nrblocks;
		for (j = 0; j < td->maxblocks; j++) {
			oldval = bbt_get_entry(this, block);
			bbt_mark_entry(this, block, BBT_BLOCK_RESERVED);
			if (oldval != BBT_BLOCK_RESERVED)
				update = 1;
			block++;
		}
		/*
		 * If we want reserved blocks to be recorded to flash, and some
		 * new ones have been marked, then we need to update the stored
		 * bbts.  This should only happen once.
		 */
		if (update && td->reserved_block_code)
			nand_update_bbt(mtd, (loff_t)(block - 1) <<
					this->bbt_erase_shift);
	}
}
#endif /* !CONFIG_MTD_NAND_UNIPHIER_BBM */

/**
 * verify_bbt_descr - verify the bad block description
 * @mtd: MTD device structure
 * @bd: the table to verify
 *
 * This functions performs a few sanity checks on the bad block description
 * table.
 */
static void verify_bbt_descr(struct mtd_info *mtd, struct nand_bbt_descr *bd)
{
	struct nand_chip *this = mtd->priv;
	u32 pattern_len;
	u32 bits;
	u32 table_size;

	if (!bd)
		return;

	pattern_len = bd->len;
	bits = bd->options & NAND_BBT_NRBITS_MSK;

	BUG_ON((this->bbt_options & NAND_BBT_NO_OOB) &&
			!(this->bbt_options & NAND_BBT_USE_FLASH));
	BUG_ON(!bits);

	if (bd->options & NAND_BBT_VERSION)
		pattern_len++;

	if (bd->options & NAND_BBT_NO_OOB) {
		BUG_ON(!(this->bbt_options & NAND_BBT_USE_FLASH));
		BUG_ON(!(this->bbt_options & NAND_BBT_NO_OOB));
		BUG_ON(bd->offs);
		if (bd->options & NAND_BBT_VERSION)
			BUG_ON(bd->veroffs != bd->len);
		BUG_ON(bd->options & NAND_BBT_SAVECONTENT);
	}

	if (bd->options & NAND_BBT_PERCHIP)
		table_size = this->chipsize >> this->bbt_erase_shift;
	else
		table_size = mtd->size >> this->bbt_erase_shift;
	table_size >>= 3;
	table_size *= bits;
	if (bd->options & NAND_BBT_NO_OOB)
		table_size += pattern_len;
	BUG_ON(table_size > (1 << this->bbt_erase_shift));
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
	BUG_ON((bd->options & NAND_BBT_NRBITS_MSK) != NAND_BBT_1BIT);
	BUG_ON(!(bd->options & NAND_BBT_PERCHIP));
	BUG_ON(!(bd->options & NAND_BBT_VERSION));
	BUG_ON(!(bd->options & NAND_BBT_CREATE));
	BUG_ON(!(bd->options & NAND_BBT_WRITE));
	BUG_ON(!(bd->options & NAND_BBT_SCAN2NDPAGE));
	BUG_ON(bd->options & NAND_BBT_DYNAMICSTRUCT);

	BUG_ON(!(this->bbt_options & NAND_BBT_USE_FLASH));
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
}

/**
 * nand_scan_bbt - [NAND Interface] scan, find, read and maybe create bad block table(s)
 * @mtd: MTD device structure
 * @bd: descriptor for the good/bad block search pattern
 *
 * The function checks, if a bad block table(s) is/are already available. If
 * not it scans the device for manufacturer marked good / bad blocks and writes
 * the bad block table(s) to the selected place.
 *
 * The bad block table memory is allocated here. It must be freed by calling
 * the nand_free_bbt function.
 */
static int nand_scan_bbt(struct mtd_info *mtd, struct nand_bbt_descr *bd)
{
	struct nand_chip *this = mtd->priv;
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
	int res;
	uint8_t *buf = this->pBlockBuf;
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	int len, res;
	uint8_t *buf;
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	struct nand_bbt_descr *td = this->bbt_td;
	struct nand_bbt_descr *md = this->bbt_md;

#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
	/* BBM resource has been got in nand-gpbc.c */
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	len = (mtd->size >> (this->bbt_erase_shift + 2)) ? : 1;
	/*
	 * Allocate memory (2bit per block) and clear the memory bad block
	 * table.
	 */
	this->bbt = kzalloc(len, GFP_KERNEL);
	if (!this->bbt)
		return -ENOMEM;
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */

	/*
	 * If no primary table decriptor is given, scan the device to build a
	 * memory based bad block table.
	 */
	if (!td) {
		if ((res = nand_memory_bbt(mtd, bd))) {
			pr_err("nand_bbt: can't scan flash and build the RAM-based BBT\n");
			goto err;
		}
		return 0;
	}
	verify_bbt_descr(mtd, td);
	verify_bbt_descr(mtd, md);

#ifndef CONFIG_MTD_NAND_UNIPHIER_BBM
	/* Allocate a temporary buffer for one eraseblock incl. oob */
	len = (1 << this->bbt_erase_shift);
	len += (len >> this->page_shift) * mtd->oobsize;
	buf = vmalloc(len);
	if (!buf) {
		res = -ENOMEM;
		goto err;
	}
#endif /* !CONFIG_MTD_NAND_UNIPHIER_BBM */

	/* Is the bbt at a given page? */
	if (td->options & NAND_BBT_ABSPAGE) {
		read_abs_bbts(mtd, buf, td, md);
	} else {
		/* Search the bad block table using a pattern in oob */
		search_read_bbts(mtd, buf, td, md);
	}

	res = check_create(mtd, buf, bd);
	if (res)
		goto err;

#ifndef CONFIG_MTD_NAND_UNIPHIER_BBM
	/* Prevent the bbt regions from erasing / writing */
	mark_bbt_region(mtd, td);
	if (md)
		mark_bbt_region(mtd, md);

	vfree(buf);
#endif /* !CONFIG_MTD_NAND_UNIPHIER_BBM */
	return 0;

err:
#ifndef CONFIG_MTD_NAND_UNIPHIER_BBM
	kfree(this->bbt);
	this->bbt = NULL;
#endif /* !CONFIG_MTD_NAND_UNIPHIER_BBM */
	return res;
}

/**
 * nand_update_bbt - update bad block table(s)
 * @mtd: MTD device structure
 * @offs: the offset of the newly marked block
 *
 * The function updates the bad block table(s).
 */
static int nand_update_bbt(struct mtd_info *mtd, loff_t offs)
{
	struct nand_chip *this = mtd->priv;
	int len, res = 0;
	int chip, chipsel;
	uint8_t *buf;
	struct nand_bbt_descr *td = this->bbt_td;
	struct nand_bbt_descr *md = this->bbt_md;

#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
	if (!this->psBbm || !td)
		return -EINVAL;
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	if (!this->bbt || !td)
		return -EINVAL;
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */

	/* Allocate a temporary buffer for one eraseblock incl. oob */
	len = (1 << this->bbt_erase_shift);
	len += (len >> this->page_shift) * mtd->oobsize;
	buf = kmalloc(len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	/* Do we have a bbt per chip? */
	if (td->options & NAND_BBT_PERCHIP) {
		chip = (int)(offs >> this->chip_shift);
		chipsel = chip;
	} else {
		chip = 0;
		chipsel = -1;
	}

	td->version[chip]++;
	if (md)
		md->version[chip]++;

#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
	res = write_bbts(mtd, buf, td, md, chipsel);
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	/* Write the bad block table to the device? */
	if (td->options & NAND_BBT_WRITE) {
		res = write_bbt(mtd, buf, td, md, chipsel);
		if (res < 0)
			goto out;
	}
	/* Write the mirror bad block table to the device? */
	if (md && (md->options & NAND_BBT_WRITE)) {
		res = write_bbt(mtd, buf, md, td, chipsel);
	}

 out:
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	kfree(buf);
	return res;
}

/*
 * Define some generic bad / good block scan pattern which are used
 * while scanning a device for factory marked good / bad blocks.
 */
static uint8_t scan_ff_pattern[] = { 0xff, 0xff };

/* Generic flash bbt descriptors */
static uint8_t bbt_pattern[] = {'B', 'b', 't', '0' };
static uint8_t mirror_pattern[] = {'1', 't', 'b', 'B' };

static struct nand_bbt_descr bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs =	8,
	.len = 4,
	.veroffs = 12,
	.maxblocks = NAND_BBT_SCAN_MAXBLOCKS,
	.pattern = bbt_pattern
};

static struct nand_bbt_descr bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs =	8,
	.len = 4,
	.veroffs = 12,
	.maxblocks = NAND_BBT_SCAN_MAXBLOCKS,
	.pattern = mirror_pattern
};

static struct nand_bbt_descr bbt_main_no_oob_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP
		| NAND_BBT_NO_OOB,
	.len = 4,
	.veroffs = 4,
	.maxblocks = NAND_BBT_SCAN_MAXBLOCKS,
	.pattern = bbt_pattern
};

static struct nand_bbt_descr bbt_mirror_no_oob_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP
		| NAND_BBT_NO_OOB,
	.len = 4,
	.veroffs = 4,
	.maxblocks = NAND_BBT_SCAN_MAXBLOCKS,
	.pattern = mirror_pattern
};

#define BADBLOCK_SCAN_MASK (~NAND_BBT_NO_OOB)
/**
 * nand_create_badblock_pattern - [INTERN] Creates a BBT descriptor structure
 * @this: NAND chip to create descriptor for
 *
 * This function allocates and initializes a nand_bbt_descr for BBM detection
 * based on the properties of @this. The new descriptor is stored in
 * this->badblock_pattern. Thus, this->badblock_pattern should be NULL when
 * passed to this function.
 */
static int nand_create_badblock_pattern(struct nand_chip *this)
{
	struct nand_bbt_descr *bd;
	if (this->badblock_pattern) {
		pr_warn("Bad block pattern already allocated; not replacing\n");
		return -EINVAL;
	}
	bd = kzalloc(sizeof(*bd), GFP_KERNEL);
	if (!bd)
		return -ENOMEM;
	bd->options = this->bbt_options & BADBLOCK_SCAN_MASK;
	bd->offs = this->badblockpos;
	bd->len = (this->options & NAND_BUSWIDTH_16) ? 2 : 1;
	bd->pattern = scan_ff_pattern;
	bd->options |= NAND_BBT_DYNAMICSTRUCT;
	this->badblock_pattern = bd;
	return 0;
}

/**
 * nand_default_bbt - [NAND Interface] Select a default bad block table for the device
 * @mtd: MTD device structure
 *
 * This function selects the default bad block table support for the device and
 * calls the nand_scan_bbt function.
 */
int nand_default_bbt(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	int ret;

	/* Is a flash based bad block table requested? */
	if (this->bbt_options & NAND_BBT_USE_FLASH) {
		/* Use the default pattern descriptors */
		if (!this->bbt_td) {
			if (this->bbt_options & NAND_BBT_NO_OOB) {
				this->bbt_td = &bbt_main_no_oob_descr;
				this->bbt_md = &bbt_mirror_no_oob_descr;
			} else {
				this->bbt_td = &bbt_main_descr;
				this->bbt_md = &bbt_mirror_descr;
			}
		}
	} else {
		this->bbt_td = NULL;
		this->bbt_md = NULL;
	}

	if (!this->badblock_pattern) {
		ret = nand_create_badblock_pattern(this);
		if (ret)
			return ret;
	}

	return nand_scan_bbt(mtd, this->badblock_pattern);
}

/**
 * nand_isreserved_bbt - [NAND Interface] Check if a block is reserved
 * @mtd: MTD device structure
 * @offs: offset in the device
 */
int nand_isreserved_bbt(struct mtd_info *mtd, loff_t offs)
{
	struct nand_chip *this = mtd->priv;
	int block;
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
	int kind;

	block = (int)(offs >> this->bbt_erase_shift);
	kind = whatKindOfBlock(mtd, block);
	if ((NAND_AREA_MASTER == kind) || (NAND_AREA_MIRROR == kind)) {
		return 1;
	}
	return 0;
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	block = (int)(offs >> this->bbt_erase_shift);
	return bbt_get_entry(this, block) == BBT_BLOCK_RESERVED;
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
}

/**
 * nand_isbad_bbt - [NAND Interface] Check if a block is bad
 * @mtd: MTD device structure
 * @offs: offset in the device
 * @allowbbt: allow access to bad block table region
 */
int nand_isbad_bbt(struct mtd_info *mtd, loff_t offs, int allowbbt)
{
	struct nand_chip *this = mtd->priv;
#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
	int block;
	struct nand_bbm *bbm;
	u32 *bitmap;
	int kind, chipnr, base;

	chipnr = offs >> this->chip_shift;
	block = offs >> this->bbt_erase_shift;

	bbm = &this->psBbm[chipnr];
	if (bbm->usedMtBlocks == -1) {
		return -1;
	}

	kind = whatKindOfBlock(mtd, block);
	if (((NAND_AREA_MASTER == kind) || (NAND_AREA_MIRROR == kind)) &&
	    !(allowbbt & NAND_ALLOW_BBT)) {
		return 1;
	}

	if ((NAND_AREA_NORMAL == kind) && !(allowbbt & NAND_ALLOW_BADDATA)) {
		return 0;
	}

	base = (1 << (this->chip_shift - this->bbt_erase_shift)) * chipnr;
	bitmap = bbm->pBbMap;
	if (bitmap[(block - base) / 32] & (1 << ((block - base) % 32))) {
		return 1;
	}

	return 0;
#else /* CONFIG_MTD_NAND_UNIPHIER_BBM */
	int block, res;

	block = (int)(offs >> this->bbt_erase_shift);
	res = bbt_get_entry(this, block);

	pr_debug("nand_isbad_bbt(): bbt info for offs 0x%08x: (block %d) 0x%02x\n",
		 (unsigned int)offs, block, res);

	switch (res) {
	case BBT_BLOCK_GOOD:
		return 0;
	case BBT_BLOCK_WORN:
		return 1;
	case BBT_BLOCK_RESERVED:
		return allowbbt ? 0 : 1;
	}
	return 1;
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */
}

/**
 * nand_markbad_bbt - [NAND Interface] Mark a block bad in the BBT
 * @mtd: MTD device structure
 * @offs: offset of the bad block
 */
int nand_markbad_bbt(struct mtd_info *mtd, loff_t offs)
{
	struct nand_chip *this = mtd->priv;
	int block, ret = 0;

	block = (int)(offs >> this->bbt_erase_shift);

	/* Mark bad block in memory */
	bbt_mark_entry(this, block, BBT_BLOCK_WORN);

	/* Update flash-based bad block table */
	if (this->bbt_options & NAND_BBT_USE_FLASH)
		ret = nand_update_bbt(mtd, offs);

	return ret;
}

#ifdef CONFIG_MTD_NAND_UNIPHIER_BBM
//Copy the data of the block except the specified page
//
static int readBlockExpSpecifiedPage(struct mtd_info *psMtd, int page, int num, u8 *block_buf, u32 *erased_map)
{
	struct nand_chip *psNand = psMtd->priv;
	u32 blockSize;
	loff_t from1;
	int status, i;
	flstate_t oldState;
	struct mtd_oob_ops sOps;
	u8 *datbuf_base = block_buf;
	u8 *oobbuf_base = block_buf + psMtd->erasesize;
	int pages_per_block = 1 << (psNand->bbt_erase_shift - psNand->page_shift);
	int block_top_page = page & ~((pages_per_block) - 1);

	blockSize = (1 << (psNand->bbt_erase_shift - psNand->page_shift)) * (psMtd->writesize + psMtd->oobsize);
	memset(block_buf, 0xff, blockSize);

	oldState = psNand->state;
	psNand->state = FL_READING;
	sOps.mode = MTD_OPS_AUTO_OOB;
	sOps.len = (size_t)psMtd->writesize;
	sOps.ooblen = psMtd->oobavail;
	sOps.ooboffs = 0;

	for (i = 0; i < pages_per_block; i++) {
		if ((block_top_page + i) >= page &&
		    (block_top_page + i) < page + num) {
			continue;
		}

		from1 = (block_top_page + i) << psNand->page_shift;
		sOps.retlen = 0;
		sOps.oobretlen = 0;
		sOps.datbuf = datbuf_base + (psMtd->writesize * i);
		sOps.oobbuf = oobbuf_base + (psMtd->oobavail * i);

		status = psNand->fReadOps(psMtd, from1, &sOps);
		if ((status && (status != -EUCLEAN)) || (sOps.len != sOps.retlen)) {
			psNand->state = oldState;
			return -1;
		}
	}
	psNand->state = oldState;

	return 0;
}

// Write data without erased page. The data has one block.
//
static int write_erased_page_oob(struct mtd_info *mtd, int page, u8 *oob_buf, int *retPage)
{
	struct nand_chip *this = mtd->priv;
	loff_t to;
	int status;
	flstate_t oldState;
	struct mtd_oob_ops ops;

	to = page << this->page_shift;
	oldState = this->state;
	this->state = FL_WRITING;

	memset(&ops, 0, sizeof(ops));
	ops.mode = MTD_OPS_AUTO_OOB;
	ops.ooblen = mtd->oobavail;
	ops.oobbuf = oob_buf;
	ops.retPage = -1;
	status = this->fWriteOob(mtd, to, &ops);
	*retPage = ops.retPage;
	this->state = oldState;
	if (mtd->oobavail != ops.oobretlen) {
		return -1;
	}
	return status;
}

static int bbt_write_programed_pages(struct mtd_info *psMtd, int phys_block,
				     u8 *pBlockBuf, u32 *erased_map,
				     int *retPage)
{
	struct nand_chip*psNand = psMtd->priv;
	loff_t to1;
	int status;
	flstate_t oldState;
	struct mtd_oob_ops sOps;
	u8 *datbuf_base = pBlockBuf;
	u8 *oobbuf_base = pBlockBuf + psMtd->erasesize;
	int i;
	int pages_per_block = 1 << (psNand->bbt_erase_shift - psNand->page_shift);
	int phys_page = phys_block * pages_per_block;

	oldState = psNand->state;
	psNand->state = FL_WRITING;
	sOps.mode = MTD_OPS_AUTO_OOB;
	sOps.len = psMtd->writesize;
	sOps.ooblen = psMtd->oobavail;
	sOps.ooboffs = 0;
	for (i = 0; i < pages_per_block; i++) {
		*retPage = -1;
		if (is_bit_set_erased_map(i, erased_map)) {
			/* write oob */
			status = write_erased_page_oob(psMtd, phys_page + i,
						       oobbuf_base + (psMtd->oobavail * i),
						       retPage);
			if (status) {
				psNand->state = oldState;
				return -1;
			}
			continue;
		}
		to1 = (phys_page + i) << psNand->page_shift;
		sOps.retlen = 0;
		sOps.oobretlen = 0;
		sOps.datbuf = datbuf_base + (psMtd->writesize * i);
		sOps.oobbuf = oobbuf_base + (psMtd->oobavail * i);
		sOps.retPage = -1;
		status = psNand->fWriteOps(psMtd, to1, &sOps);
		*retPage = sOps.retPage;
		if (status) {
			psNand->state = oldState;
			return -1;
		}
	}
	psNand->state = oldState;

	return 0;
}

//Substitute the alternate block for the bad block
//
int bbt_alternateBb(struct mtd_info *psMtd, int pageNum)
{
	struct nand_chip *psNand = psMtd->priv;
	struct nand_bbt_descr *td = psNand->bbt_td;
	struct nand_bbt_descr *md = psNand->bbt_md;
	int blockNum;
	int chipNum;
	int rsvBlock;
	int physPageNum;
	int ret;
	uint8_t *buf = psNand->pBlockBuf;

	blockNum = pageNum >> (psNand->bbt_erase_shift - psNand->page_shift);
	chipNum = pageNum >> (psNand->chip_shift - psNand->page_shift);

	ret = whatKindOfBlock(psMtd, blockNum);
	if (ret > 0) {
		return ret;
	} else if (ret < 0) {
		return -1;
	}

	physPageNum = bbt_translateBb(psMtd, pageNum & psNand->pagemask, chipNum);
	setBbFlag(psMtd, physPageNum);

	rsvBlock = getRsvBlock(psMtd, chipNum);
	if (-1 == rsvBlock) {
		return -1;
	}

	ret = registerAltBlock(psMtd, pageNum, rsvBlock);
	if (ret) {
		return -1;
	}

	td->version[chipNum]++;
	if (md) {
		md->version[chipNum]++;
	}

	ret = write_bbts(psMtd, buf, td, md, chipNum);
	if (ret) {
		return -1;
	}

	return 0;
}

//Substitute the alternate block for the valid block (before become bad block)
//  after copying the all data to the alternate block
//
int bbt_replaceBb(struct mtd_info *psMtd, int pageNum)
{
	struct nand_chip *psNand = psMtd->priv;
	struct erase_info sEinfo;
	u8 *pBlockBuf = psNand->pBlockBuf;
	int blockNum;
	int retPage, chipNum, phys_page, phys_block, block_mask;
	int status;
	flstate_t oldState;
	loff_t phys_offset;
	u32 *erased_map = psNand->erased_map;

	blockNum = pageNum >> (psNand->bbt_erase_shift - psNand->page_shift);
	chipNum = pageNum >> (psNand->chip_shift - psNand->page_shift);

	status = whatKindOfBlock(psMtd, blockNum);
	if (status < 0) {
		return -1;
	} else if (status != NAND_AREA_NORMAL) {
		return status;
	}

	clear_erased_map(psNand);

	status = readBlock(psMtd, blockNum, pBlockBuf, erased_map);
	if (status) {
		return -1;
	}

	while (1) {
		status = bbt_alternateBb(psMtd, pageNum);
		if (status) {
			return -1;
		}

		phys_page = bbt_translateBb(psMtd, pageNum & psNand->pagemask, chipNum);
		block_mask = ~((1 << (psNand->bbt_erase_shift - psNand->page_shift)) - 1);
		phys_offset = (phys_page & block_mask) << psNand->page_shift;
		phys_block = (int)(phys_offset >> psNand->bbt_erase_shift);

		memset(&sEinfo, 0, sizeof(sEinfo));
		sEinfo.mtd = psMtd;
		sEinfo.addr = phys_offset;
		sEinfo.len = 1 << psNand->bbt_erase_shift;
		sEinfo.fail_addr = MTD_FAIL_ADDR_UNKNOWN;
		oldState = psNand->state;
		psNand->state = FL_ERASING;
		status = nand_erase_nand(psMtd, &sEinfo, NAND_ALLOW_BBT);
		psNand->state = oldState;
		if (status) {
			if (MTD_FAIL_ADDR_UNKNOWN == sEinfo.fail_addr) {
				return -1;
			}
			continue;
		}

		status = bbt_write_programed_pages(psMtd, phys_block, pBlockBuf, erased_map, &retPage);
		if (status) {
			if (retPage == -1) {
				return -1;
			}
			continue;
		}
		break;
	}

	return 0;
}

//Substitute the alternate block for the bad block
//  after copying the data of bad block to the alternate block
//
int bbt_replaceBbExpOnePage(struct mtd_info *psMtd, int pageNum)
{
	struct nand_chip *psNand = psMtd->priv;
	struct erase_info sEinfo;
	int blockNum;
	int chipNum;
	int phys_page, phys_block, block_mask;
	loff_t phys_offset;
	int retPage;
	int status;
	u8 *block_buf = psNand->pBlockBuf;
	flstate_t oldState;
	u32 *erased_map = psNand->erased_map;
	int pages_per_block = 1 << (psNand->bbt_erase_shift - psNand->page_shift);
	u32 page_offset = pageNum & (pages_per_block - 1);

	blockNum = pageNum >> (psNand->bbt_erase_shift - psNand->page_shift);
	chipNum = pageNum >> (psNand->chip_shift - psNand->page_shift);

	status = whatKindOfBlock(psMtd, blockNum);
	if (0 < status) {
		return status;
	} else if (0 > status) {
		return -1;
	}

	clear_erased_map(psNand);

	status = readBlockExpOnePage(psMtd, pageNum, block_buf, erased_map);
	if (status) {
		return -1;
	}

	/*
	 * Set erased map becaouse this page don't copy.
	 * This page is written after bad block replacement,
	 */
	set_bit_erased_map(page_offset, erased_map);

	while (1) {
		status = bbt_alternateBb(psMtd, pageNum);
		if (status) {
			return status;
		}
		phys_page = bbt_translateBb(psMtd, pageNum & psNand->pagemask, chipNum);
		block_mask = ~((1 << (psNand->bbt_erase_shift - psNand->page_shift)) - 1);
		phys_offset = (phys_page & block_mask) << psNand->page_shift;
		phys_block = (int)(phys_offset >> psNand->bbt_erase_shift);

		memset(&sEinfo, 0, sizeof(sEinfo));
		sEinfo.mtd = psMtd;
		sEinfo.addr = phys_offset;
		sEinfo.len = 1 << psNand->bbt_erase_shift;
		sEinfo.fail_addr = MTD_FAIL_ADDR_UNKNOWN;
		oldState = psNand->state;
		psNand->state = FL_ERASING;
		status = nand_erase_nand(psMtd, &sEinfo, NAND_ALLOW_BBT);
		psNand->state = oldState;
		if (status) {
			if (MTD_FAIL_ADDR_UNKNOWN == sEinfo.fail_addr) {
				return -1;
			}

			continue;
		}

		status = bbt_write_programed_pages(psMtd, phys_block, block_buf, erased_map, &retPage);
		if (status) {
			if (-1 == retPage) {
				return -1;
			}
			continue;
		}
		break;
	}

	return 0;
}

//Set the bad block flag on OOB of the block
//  and register the bad block to BBM
//  and save BBM
//
int bbt_registerBb(struct mtd_info *psMtd, int pageNum)
{
	struct nand_chip *psNand = psMtd->priv;
	struct nand_bbt_descr *td = psNand->bbt_td;
	struct nand_bbt_descr *md = psNand->bbt_md;
	int chipNum;
	int blockNum;
	uint8_t *buf = psNand->pBlockBuf;
	int ret = 0;

	blockNum = pageNum >> (psNand->bbt_erase_shift - psNand->page_shift);
	chipNum = pageNum >> (psNand->chip_shift - psNand->page_shift);

	ret = whatKindOfBlock(psMtd, blockNum);
	switch (ret) {
	case NAND_AREA_MAINTAIN:
	case NAND_AREA_MASTER:
	case NAND_AREA_MIRROR:
		return -1;
	case NAND_AREA_NORMAL:
		bbt_replaceBb(psMtd, pageNum);
		break;
	case NAND_AREA_BOOT:
		setBbFlag(psMtd, pageNum);
		setBbMap(psMtd, blockNum);

		td->version[chipNum]++;
		if (md) {
			md->version[chipNum]++;
		}

		ret = write_bbts(psMtd, buf, td, md, chipNum);
		if (ret) {
			return -1;
		}
		break;
	default:
		BUG();
	}

	return 0;
}

//Translate the address to the bad block into the address to the alternate block
//
int bbt_translateBb(struct mtd_info *psMtd, int virtPage, int chipNum)
{
	struct nand_chip *psNand = psMtd->priv;
	struct nand_bbList *psBbList;
	struct nand_bbm *psBbm;
	int blockNum;
	int baseB;
	u32 *pBbMap;
	int kind;
	u32 cnt;
	int physPage;

	virtPage += chipNum << (psNand->chip_shift - psNand->page_shift);

	if (0 > virtPage) {
		return virtPage;
	}

	psBbm = &psNand->psBbm[chipNum];

	if (psBbm->usedMtBlocks == -1) {
		return virtPage;
	}

	kind = whatKindOfBlock(psMtd, (virtPage >> (psNand->bbt_erase_shift - psNand->page_shift)));
	if (NAND_AREA_NORMAL != kind) {
		return virtPage;
	}

	pBbMap = psBbm->pBbMap;
	blockNum = virtPage >> (psNand->bbt_erase_shift - psNand->page_shift);
	baseB = (1 << (psNand->chip_shift - psNand->bbt_erase_shift)) * chipNum;
	if (!(pBbMap[(blockNum - baseB) / 32] & (1 << ((blockNum - baseB) % 32)))) {
		return virtPage;
	}

	psBbList = psBbm->psBbList;
	for (cnt = 0; cnt < psBbm->altBlocks; cnt++) {
		if (psBbList[cnt].badBlock == blockNum) {
			break;
		}
	}
	if (cnt >= psBbm->altBlocks) {
		return virtPage;//BUG?
	}

	physPage = (psBbList[cnt].altBlock << (psNand->bbt_erase_shift - psNand->page_shift))
		| (virtPage & ((1<<(psNand->bbt_erase_shift - psNand->page_shift)) - 1));

	return physPage;
}

//Get the reserve block
//
static int getRsvBlock(struct mtd_info *psMtd, int chipNum)
{
	struct nand_chip *psNand = psMtd->priv;
	struct nand_bbm *psBbm = &psNand->psBbm[chipNum];
	struct nand_bbt_descr *psTd = psNand->bbt_td;
	s32 baseB;
	s32 rsvStartB;
	s32 rsvBlocks;
	s32 usedRsvBlocks;
	s32 direction;
	u32 *pBbMap = psBbm->pBbMap;
	u32 cnt;
	s32 blockNum;
	s32 chipBlocks = 1 << (psNand->chip_shift - psNand->bbt_erase_shift);

	if (psBbm->usedMtBlocks == -1) {
		return -1;
	}

	if (psBbm->mtBlocks <= psBbm->usedMtBlocks) {
		return -1;
	}

	if (psTd->options & (NAND_BBT_FLEXIBLE | NAND_BBT_LASTBLOCK)) {
		rsvStartB = psBbm->area[NAND_AREA_ID_ALT].startBlock +
			psBbm->area[NAND_AREA_ID_ALT].blockNum - 1;
		direction = -1;
	} else {
		rsvStartB = psBbm->area[NAND_AREA_ID_ALT].startBlock;
		direction = 1;
	}

	blockNum = -1;
	baseB = chipBlocks * chipNum;
	rsvBlocks = psBbm->area[NAND_AREA_ID_ALT].blockNum;
	usedRsvBlocks = psBbm->usedMtBlocks - psBbm->area[NAND_AREA_ID_BBM].blockNum;
	for (cnt = usedRsvBlocks; cnt < rsvBlocks; cnt++) {
		blockNum = rsvStartB + direction * cnt;

		if (!(pBbMap[(blockNum - baseB) / 32] & (1 << ((blockNum - baseB) % 32)))) {
			break;
		}
		psBbm->usedMtBlocks++;
	}
	if (psBbm->usedMtBlocks >= psBbm->mtBlocks) {
		return -1;
	}

	return blockNum;
}

//Confirm the kind of the block
//
static int whatKindOfBlock(struct mtd_info *psMtd, int blockNum)
{
	struct nand_chip *psNand = psMtd->priv;
	struct nand_bbm *psBbm;
	s32 chipNum;
	s32 startB, endB;
	s32 pageNum;

	chipNum = blockNum >> (psNand->chip_shift - psNand->bbt_erase_shift);
	pageNum = blockNum << (psNand->bbt_erase_shift - psNand->page_shift);
	psBbm = &psNand->psBbm[chipNum];

	if (psBbm->usedMtBlocks == -1) {
		return -1;
	}

	if (pageNum == psNand->bbt_td->pages[chipNum]) {
		return NAND_AREA_MASTER;
	}

	if (pageNum == psNand->bbt_md->pages[chipNum]) {
		return NAND_AREA_MIRROR;
	}

	startB = psBbm->area[NAND_AREA_ID_BBM].startBlock;
	endB = startB + psBbm->area[NAND_AREA_ID_BBM].blockNum;
	if (blockNum >= startB && blockNum < endB) {
		return NAND_AREA_MAINTAIN;
	}

	startB = psBbm->area[NAND_AREA_ID_ALT].startBlock;
	endB = startB + psBbm->area[NAND_AREA_ID_ALT].blockNum;
	if (blockNum >= startB && blockNum < endB) {
		return NAND_AREA_MAINTAIN;
	}

	startB = psBbm->area[NAND_AREA_ID_BOOT].startBlock;
	endB = startB + psBbm->area[NAND_AREA_ID_BOOT].blockNum;
	if (blockNum >= startB && blockNum < endB) {
		return NAND_AREA_BOOT;
	}

	return NAND_AREA_NORMAL;
}

//Register the alt block to BBM
//
static int registerAltBlock(struct mtd_info *psMtd, int pageNum, int rsvBlock)
{
	struct nand_chip *psNand = psMtd->priv;
	struct nand_bbm *psBbm;
	struct nand_bbList *psBbList;
	u32 *pBbMap;
	int blockNum;
	int chipNum;
	int baseB;
	u32 cnt;

	blockNum = pageNum >> (psNand->bbt_erase_shift - psNand->page_shift);
	chipNum = pageNum >> (psNand->chip_shift - psNand->page_shift);
	psBbm = &psNand->psBbm[chipNum];
	psBbList = psBbm->psBbList;
	pBbMap = psBbm->pBbMap;

	if (psBbm->usedMtBlocks == -1) {
		return -1;
	}

	baseB = (1 << (psNand->chip_shift - psNand->bbt_erase_shift)) * chipNum;

	if (pBbMap[(blockNum - baseB) / 32] & (1 << ((blockNum - baseB) % 32))) {
		for (cnt = 0; cnt < psBbm->altBlocks; cnt++) {
			if (psBbList[cnt].badBlock == blockNum) {
				break;
			}
		}
		if (cnt >= psBbm->altBlocks) {
			BUG();
		}
		setBbMap(psMtd, psBbList[cnt].altBlock);
		psBbList[cnt].altBlock = rsvBlock;
		psBbm->usedMtBlocks++;
	} else {
		setBbMap(psMtd, blockNum);
		psBbList[psBbm->altBlocks].badBlock = blockNum;
		psBbList[psBbm->altBlocks].altBlock = rsvBlock;
		psBbm->altBlocks++;
		psBbm->usedMtBlocks++;
	}

	return 0;
}

//Register the bad block to Bad Block bitmap
//
static int setBbMap(struct mtd_info *psMtd, int blockNum)
{
	struct nand_chip *psNand = psMtd->priv;
	struct nand_bbm *psBbm;
	u32 *pBbMap;
	int chipNum;
	int baseB;

	chipNum = blockNum >> (psNand->chip_shift - psNand->bbt_erase_shift);
	psBbm = &psNand->psBbm[chipNum];
	pBbMap = psBbm->pBbMap;

	baseB = (1 << (psNand->chip_shift - psNand->bbt_erase_shift)) * chipNum;

	pBbMap[(blockNum - baseB) / 32] |= (1 << ((blockNum - baseB) % 32));

	return 0;
}

//Copy the data of the bad block except the page caused the bad block
//
static int readBlockExpOnePage(struct mtd_info *psMtd, int pageNum, u8 *pBlockBuf, u32 *erased_map)
{
	return readBlockExpSpecifiedPage(psMtd, pageNum, 1, pBlockBuf, erased_map);
}

//Read all pages in the block
//
static int readBlock(struct mtd_info *psMtd, int block, u8 *block_buf, u32 *erased_map)
{
	struct nand_chip *psNand = psMtd->priv;
	loff_t from;
	int status, i;
	flstate_t oldState;
	struct mtd_oob_ops sOps;
	int pages_per_block = 1 << (psNand->bbt_erase_shift - psNand->page_shift);
	int block_top_page = pages_per_block * block;
	u32 block_buf_size = (psMtd->writesize + psMtd->oobsize) * pages_per_block;
	u8 *datbuf_base = block_buf;
	u8 *oobbuf_base = block_buf + psMtd->erasesize;

	memset(block_buf, 0xff, block_buf_size);

	oldState = psNand->state;
	psNand->state = FL_READING;
	sOps.mode = MTD_OPS_AUTO_OOB;
	sOps.len = (size_t)psMtd->writesize;
	sOps.ooblen = psMtd->oobavail;
	sOps.ooboffs = 0;

	for (i = 0; i < pages_per_block; i++) {
		from = (block_top_page + i) << psNand->page_shift;
		sOps.retlen = 0;
		sOps.oobretlen = 0;
		sOps.datbuf = datbuf_base + (psMtd->writesize * i);
		sOps.oobbuf = oobbuf_base + (psMtd->oobavail * i);

		status = psNand->fReadOps(psMtd, from, &sOps);
		if ((status && (status != -EUCLEAN)) || (sOps.len != sOps.retlen)) {
			psNand->state = oldState;
			return -1;
		}
	}
	psNand->state = oldState;

	return 0;
}

//Set the bad block flag on OOB
//
static int setBbFlag(struct mtd_info *psMtd, int pageNum)
{
	struct nand_chip *psNand = psMtd->priv;
	loff_t to;
	u8 aBuf[2] = {0, 0};
	int status, i = 0;
	flstate_t oldState;
	struct mtd_oob_ops sOps;

	to = (pageNum >> (psNand->bbt_erase_shift - psNand->page_shift)) <<
		psNand->bbt_erase_shift;
	oldState = psNand->state;
	psNand->state = FL_WRITING;

	memset(&sOps, 0, sizeof(sOps));
	sOps.mode = MTD_OPS_RAW;
	sOps.ooblen = 2;
	sOps.oobbuf = aBuf;

	for (i = 0; i < 2; i++) {
		sOps.oobretlen = 0;
		sOps.retPage = -1;
		status = psNand->fWriteOob(psMtd, to, &sOps);
		if (sOps.oobretlen != 2) {
			return -1;
		}
		to += psMtd->writesize;
	}
	psNand->state = oldState;

	return status;
}
#endif /* CONFIG_MTD_NAND_UNIPHIER_BBM */

EXPORT_SYMBOL(nand_scan_bbt);
