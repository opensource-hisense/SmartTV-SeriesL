/* * Copyright (C) 2016 Socionext Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <linux/compat.h>

/* User setting */
#if defined(CONFIG_SPL_NAND_UNIPHIER_WORK_ADDRESS) && defined(CONFIG_NAND_UNIPHIER_BLOCK_NUM_BANK0) && \
    defined(CONFIG_NAND_UNIPHIER_BOOT_NUM_BANK0)   && defined(CONFIG_NAND_UNIPHIER_ALT_NUM_BANK0)

#define NAND_BBM_INFO_ADDRESS		(CONFIG_SPL_NAND_UNIPHIER_WORK_ADDRESS)
#define NAND_BLOCK_NUM			(CONFIG_NAND_UNIPHIER_BLOCK_NUM_BANK0)
#define NAND_BOOT_BLOCK_NUM		(CONFIG_NAND_UNIPHIER_BOOT_NUM_BANK0)
#define NAND_ALT_BLOCK_NUM		(CONFIG_NAND_UNIPHIER_ALT_NUM_BANK0)

#else

#error "ERROR: necessary macro for build is not defined."

#endif

#define NAND_BBM_BLOCK_NUM		(2)
#define NAND_DATA_BLOCK_NUM		(NAND_BLOCK_NUM - NAND_BOOT_BLOCK_NUM - NAND_ALT_BLOCK_NUM - NAND_BBM_BLOCK_NUM)

#define NAND_BBM_BLOCK_NUM		(2)
#define NAND_MANAGEMENT_BLOCK_NUM	(NAND_ALT_BLOCK_NUM + NAND_BBM_BLOCK_NUM)

#define NAND_BB_FLAG_PAGE_NUM		(2)
#define FLAG_VALID_BLOCK		(0xFF)
#define FLAG_NOT_BBM			(0xFF)

#define NAND_BBT_PATTERN_MASTER		"Bbt0"
#define NAND_BBT_PATTERN_SLAVE		"1tbB"
#define NAND_BBT_PATTERN_MASTER_TEMP	"Ubt0"
#define NAND_BBT_PATTERN_SLAVE_TEMP	"UtbB"

/*
 **************************************************
 Format of bad block management (data area of page)
 **************************************************
(offset)
0x00		-------------------------
		BBT pattern
0x04		-------------------------
		BBT version
0x05		-------------------------
		Reserved
0x08		-------------------------
		Used management block number
		(Used alternated block + BBM block)
0x0c		-------------------------
		Alternated block count
0x10		-------------------------
		Page position of master BBM
0x14		-------------------------
		Page position of slave BBM
0x18		-------------------------
		Bitmap table marked bad block (BBT)
0x18+[*1]	-------------------------
		Alternated list
0x18+[*1]+[*2]	-------------------------

  [*1] ... Size of bitmap table marked bad block.
  [*2] ... Size of Alternated list.

 **************************************************
 Format of bad block management (oob  area of page)
 **************************************************
(offset)
0x00		-------------------------
		Bad block flag
0x02		-------------------------
		BBT flag
0x03		-------------------------
*/
	
/* Length in BBM */
/* oob area */
#define NAND_BB_FLAG_LEN		(2)
#define NAND_BBT_FLAG_LEN		(1)
/* data area */
#define NAND_BBT_PATTERN_LEN		(4)
#define NAND_BBT_VERSION_LEN		(1)
#define NAND_USED_MANAGEMENT_BLOCKS_LEN	(4)
#define NAND_ALTERNATED_BLOCK_COUNT_LEN	(4)
#define NAND_MASTER_PAGE_POSITION_LEN	(4)
#define NAND_SLAVE_PAGE_POSITION_LEN	(4)
#define NAND_BBT_LEN			NAND_BBT_BYTE
#define NAND_BB_LIST_LEN		NAND_BB_LIST_BYTE

/* Offset in BBM */
/* oob area */
#define NAND_BB_FLAG_OFFSET			(0x00)
#define NAND_BBT_FLAG_OFFSET			(0x02)
/* data area */
#define NAND_BBT_PATTERN_OFFSET			(0x00)
#define NAND_BBT_VERSION_OFFSET			(0x04)
#define NAND_USED_MANAGEMENT_BLOCKS_OFFSET	(0x08)
#define NAND_ALTERNATED_BLOCK_COUNT_OFFSET	(0x0c)
#define NAND_MASTER_PAGE_POSITION_OFFSET	(0x10)
#define NAND_SLAVE_PAGE_POSITION_OFFSET		(0x14)
#define NAND_BBT_OFFSET				(0x18)
#define NAND_BB_LIST_OFFSET			(NAND_BBT_OFFSET + NAND_BBT_LEN)

#define NAND_BBT_WIDTH		(4)
#define NAND_BBT_BIT_LENGTH	(NAND_BBT_WIDTH * 8)
#define NAND_BBT_SIZE_L		DIV_ROUND_UP(NAND_BLOCK_NUM, NAND_BBT_BIT_LENGTH)
#define NAND_BBT_BYTE		(NAND_BBT_SIZE_L * NAND_BBT_WIDTH)
#define NAND_BB_LIST_BYTE	(sizeof(struct bb_list_t) * NAND_ALT_BLOCK_NUM)
#define NAND_BBM_PAGE_NUM	DIV_ROUND_UP(NAND_BB_LIST_OFFSET + NAND_BB_LIST_BYTE, uniphier_nand_page_size)

#define NAND_AREA_NUM		(4)
#define NAND_AREA_BOOT		(0)
#define NAND_AREA_DATA		(1)
#define NAND_AREA_RESERVED	(2)
#define NAND_AREA_BBM		(3)

#define NAND_SEARCH_FROM_LAST	(-1)

/* Error Code */
#define NAND_ERR_INV_AREA	(-1)
#define NAND_ERR_DST_NOT_ALIGN	(-2)
#define NAND_ERR_NO_BBM		(-3)
#define NAND_ERR_UNCORRECT	(-4)
#define NAND_ERR_INV_BBM	(-5)
#define NAND_ERR_DEV_BUSY	(-6)
#define NAND_ERR_UNSIGNED_MAGIC	(-7)

#define endian_swab32(x)	(x)

struct bbm_area_info_t {
	unsigned int start_block;
	unsigned int block_num;
};

struct bb_list_t{
	unsigned int bad_block;
	unsigned int alt_block;
};

struct bbm_info_t{
	unsigned int bbt[NAND_BBT_SIZE_L];
	struct bb_list_t bb_list[NAND_ALT_BLOCK_NUM];
	unsigned int alternated_block_count;
	struct bbm_area_info_t area_info[NAND_AREA_NUM];
};

struct bbm_pattern_t {
	unsigned char pattern[NAND_BBT_PATTERN_LEN];
	unsigned int len;
};

extern unsigned int uniphier_nand_page_size, uniphier_nand_pages_per_block;
static struct bbm_info_t *bbm_info = NULL;

extern int uniphier_nand_read_oob(void *buf, int page);
extern int uniphier_nand_read_page(void *buf, int page);


//#define UNIPHIER_NAND_DEBUG
#ifdef UNIPHIER_NAND_DEBUG
#define ASSERT(cond)		do { if(!(cond)) while(1); } while(0)
static void nand_debug(unsigned long val, unsigned char bit)
{
	unsigned char tmp[] = "0123456789abcdef";

	while(bit) {
		bit -= 4;
		putc(tmp[(val >> bit) & 0xf]);
	}
}


static void nand_valdump(char *str, unsigned int val, unsigned char bit)
{
	puts(str);
	nand_debug(val, bit);
	puts("\n");
}


static void nand_memdump(void *addr, unsigned int size)
{
	unsigned char val;
	unsigned int i;

	for(i=0; i<size; i++) {
		if(!(i % 16)) {
			if(i) {
				puts("\n");
			}
			nand_debug((unsigned int)(unsigned long)(addr + i), 32);
			puts(" : ");
		}
		val = *((unsigned char *)addr + i);
		nand_debug(val, 8);
		puts(" ");
	}
	puts("\n");
}

#else	/* UNIPHIER_NAND_DEBUG */
#define ASSERT(...)		do { } while (0)
#define nand_valdump(...)	do { } while (0)
#define nand_memdump(...)	do { } while (0)
#endif	/* UNIPHIER_NAND_DEBUG */


/* check block whitch page buffer */
static int
nand_is_bad_block(unsigned char *pgbuf)
{
	int i;
	for (i = 0; i < NAND_BB_FLAG_LEN; i++) {
		if (pgbuf[i + NAND_BB_FLAG_OFFSET] != FLAG_VALID_BLOCK) {
			return 1;
		}
	}
	return 0;
}


/*  check bbt flag whitch page buffer */
static int
nand_have_bbt_flag(unsigned char *pgbuf)
{
	int i;
	for (i = 0; i < NAND_BBT_FLAG_LEN; i++) {
		if (pgbuf[i + NAND_BBT_FLAG_OFFSET] == FLAG_NOT_BBM) {
			return 0;
		}
	}
	return 1;
}


/* compare pattern of page buffer */
static int
nand_check_bbm_pattern(unsigned char *pgbuf, struct bbm_pattern_t *pattern)
{
	unsigned char *pat = pattern->pattern;
	unsigned int i;

	for (i = 0; i < pattern->len; i++) {
		if (pgbuf[i + NAND_BBT_PATTERN_OFFSET] != pat[i]) {
			return -1;
		}
	}
	return 0;
}


/* compare version betweeen master and slave */
static unsigned char
nand_compare_bbt_version(unsigned char master_ver, unsigned char slave_ver)
{
	unsigned char newer;
	if (master_ver - slave_ver >= 0) {
		if (master_ver == 0xFF && slave_ver == 0x00) {
			newer = slave_ver;
		} else {
			newer = master_ver;
		}
	} else {
		if (master_ver == 0x00 && slave_ver == 0xFF) {
			newer = master_ver;
		} else {
			newer = slave_ver;
		}
	}

	return newer;
}


/* Translate block number virtual to physical */
static unsigned int
nand_virt_to_phys_block(unsigned int block, struct bbm_info_t *bbm)
{
	unsigned int i, mask;

	ASSERT(block < NAND_BLOCK_NUM);

	mask = 1UL << (block % NAND_BBT_BIT_LENGTH);
	if (!(bbm->bbt[block / NAND_BBT_BIT_LENGTH] & mask)) {
		return block;
	}

	for (i = 0; i < bbm->alternated_block_count; i++) {
		if (bbm->bb_list[i].bad_block == block) {
			break;
		}
	}
	if (i >= bbm->alternated_block_count) {
		return NAND_ERR_UNSIGNED_MAGIC;
	}

	return bbm->bb_list[i].alt_block;
}

static void nand_set_area_info(struct bbm_area_info_t *area_info)
{
	int i;
	unsigned int size = 0;

	area_info[NAND_AREA_BOOT].block_num = NAND_BOOT_BLOCK_NUM;
	area_info[NAND_AREA_DATA].block_num = NAND_DATA_BLOCK_NUM;
	area_info[NAND_AREA_RESERVED].block_num = NAND_ALT_BLOCK_NUM;
	area_info[NAND_AREA_BBM].block_num = NAND_BBM_BLOCK_NUM;

	for (i = 0; i < NAND_AREA_NUM; i++) {
		area_info[i].start_block = size;
		size += area_info[i].block_num;
	}
}


/* search BBM from start_blk */
static int
nand_search_bbm_range(unsigned char *pDest, struct bbm_pattern_t *pattern,
		      unsigned int start_blk, unsigned int num, int direction)
{
	unsigned char *aOobBuf = pDest + (uniphier_nand_pages_per_block * uniphier_nand_page_size);
	int status;
	int ver = -1;
	unsigned int i, pg, blk, read_pg;

	read_pg = (NAND_BBM_PAGE_NUM > NAND_BB_FLAG_PAGE_NUM) ?
		NAND_BBM_PAGE_NUM : NAND_BB_FLAG_PAGE_NUM;

	for (i = 0; i < num; i++) {
		blk = start_blk + i * direction;

		for (pg = 0; pg < read_pg; pg++) {
			status = uniphier_nand_read_oob(aOobBuf,
					blk * uniphier_nand_pages_per_block + pg);
			if (status < 0) {
				return NAND_ERR_DEV_BUSY;
			}

			status = uniphier_nand_read_page(pDest + uniphier_nand_page_size * pg,
					blk * uniphier_nand_pages_per_block + pg);
			if (status < 0) {
				return NAND_ERR_DEV_BUSY;
			}

			if (pg < NAND_BB_FLAG_PAGE_NUM) {
				if (nand_is_bad_block(aOobBuf)) {
					break;
				}
			}
			if (pg < NAND_BBM_PAGE_NUM) {
				if (!nand_have_bbt_flag(aOobBuf)) {
					break;
				}
				if (pg == 0) {
					if (nand_check_bbm_pattern(pDest, pattern)) {
						break;
					}
					ver = pDest[NAND_BBT_VERSION_OFFSET];
				}
			}
		}
		if (pg == read_pg) {
			/* found bbm */
			return ver;
		}
	}
	/* bbm is not found */
	return NAND_ERR_NO_BBM;
}


/* search BBM on the NAND DEVICE */
static int
nand_search_bbm(unsigned char *pDest, struct bbm_pattern_t *pattern,
		struct bbm_area_info_t *area_info, int direction)
{
	int ver;
	unsigned int blk, num;

	num = area_info[NAND_AREA_BBM].block_num;
	if (direction == NAND_SEARCH_FROM_LAST) {
		blk = area_info[NAND_AREA_BBM].start_block + num - 1;
	} else {
		blk = area_info[NAND_AREA_BBM].start_block;
	}
	ver = nand_search_bbm_range(pDest, pattern, blk, num, direction);

	if (ver == NAND_ERR_DEV_BUSY) {
		return NAND_ERR_DEV_BUSY;
	} else if (ver == NAND_ERR_NO_BBM) {
		num = area_info[NAND_AREA_RESERVED].block_num;
		if (direction == NAND_SEARCH_FROM_LAST) {
			blk = area_info[NAND_AREA_RESERVED].start_block + num - 1;
		} else {
			blk = area_info[NAND_AREA_RESERVED].start_block;
		}
		ver = nand_search_bbm_range(pDest, pattern, blk, num, direction);
	}

	return ver;
}


/* copy bbm from buffer */
static void
nand_get_bbm_info(struct bbm_info_t *bbm, unsigned char *buf)
{
	struct bb_list_t *nand_bb_list;
	int i;
	unsigned char *addr;

	/* copy BBT */
	for (i = 0; i < NAND_BBT_SIZE_L; i++) {
		addr = buf + i * NAND_BBT_WIDTH + NAND_BBT_OFFSET;
		bbm->bbt[i] = endian_swab32(*(unsigned int *)addr);
	}

	/* copy count of alternated block */
	addr = buf + NAND_ALTERNATED_BLOCK_COUNT_OFFSET;
	bbm->alternated_block_count = endian_swab32(*(unsigned int *)addr);

	/* copy BB List */
	nand_bb_list = (struct bb_list_t *)(buf + NAND_BB_LIST_OFFSET);
	for (i = 0; i < bbm->alternated_block_count; i++) {
		bbm->bb_list[i].bad_block = endian_swab32(nand_bb_list[i].bad_block);
		bbm->bb_list[i].alt_block = endian_swab32(nand_bb_list[i].alt_block);
	}
}


/* get newer BBM */
static int
nand_get_bbm(void *buf, struct bbm_info_t *bbm, bool pattern_temp_flag)
{
	struct bbm_pattern_t master_pattern;
	struct bbm_pattern_t slave_pattern;
	int master_ver, slave_ver;
	unsigned char *pBuf = buf;

	if(pattern_temp_flag == false) {
		strncpy((char *)master_pattern.pattern, NAND_BBT_PATTERN_MASTER, NAND_BBT_PATTERN_LEN);
		strncpy((char *)slave_pattern.pattern, NAND_BBT_PATTERN_SLAVE, NAND_BBT_PATTERN_LEN);
	} else {
		strncpy((char *)master_pattern.pattern, NAND_BBT_PATTERN_MASTER_TEMP, NAND_BBT_PATTERN_LEN);
		strncpy((char *)slave_pattern.pattern, NAND_BBT_PATTERN_SLAVE_TEMP, NAND_BBT_PATTERN_LEN);
	}
	master_pattern.len = NAND_BBT_PATTERN_LEN;
	slave_pattern.len = NAND_BBT_PATTERN_LEN;

	master_ver = nand_search_bbm(pBuf, &master_pattern, bbm->area_info, NAND_SEARCH_FROM_LAST);
	nand_valdump("master_ver : 0x", master_ver, 8);
	if (master_ver == NAND_ERR_DEV_BUSY) {
		return NAND_ERR_DEV_BUSY;
	} else if (master_ver >= 0) {
		ASSERT(master_ver <= 0xff);
		nand_get_bbm_info(bbm, pBuf);
	}

	slave_ver = nand_search_bbm(pBuf, &slave_pattern, bbm->area_info, NAND_SEARCH_FROM_LAST);
	nand_valdump("sleve_ver : 0x", slave_ver, 8);
	if (slave_ver == NAND_ERR_DEV_BUSY) {
		return NAND_ERR_DEV_BUSY;
	} else if (slave_ver >= 0) {
		ASSERT(slave_ver <= 0xff);
		if (master_ver >= 0) {
			unsigned char ver;
			/* find master and slave */
			ver = nand_compare_bbt_version((unsigned char)master_ver,
						       (unsigned char)slave_ver);
			if (master_ver == ver) {
				nand_valdump("Use master bbt : ver = 0x", master_ver, 8);
				return 0;
			}
		}
		nand_get_bbm_info(bbm, pBuf);
	} else {
		if (master_ver < 0) {
			/* cannot find bbm */
			return NAND_ERR_NO_BBM;
		}
	}

	nand_valdump("Use slave bbt : ver = 0x", slave_ver, 8);
	return 0;
}


int get_phys_block(void *buf, unsigned int block)
{
	int status;

	/* If memory bbt is not created, scan bbt on nand and create it. */
	if(bbm_info == NULL) {
		bbm_info = (struct bbm_info_t *)(NAND_BBM_INFO_ADDRESS);
		memset(bbm_info, 0, sizeof(struct bbm_info_t));

		nand_set_area_info(bbm_info->area_info);
		status = nand_get_bbm(buf, bbm_info, false);
		if(status == NAND_ERR_NO_BBM) {
			status = nand_get_bbm(buf, bbm_info, true);
			if(status == NAND_ERR_NO_BBM) {
				return -1;
			}
		}

		/* Debug */
		nand_valdump("bbm_info->bbt[] : ", (unsigned int)(unsigned long)bbm_info->bbt, 32);
		nand_memdump((void *)bbm_info->bbt, sizeof(bbm_info->bbt));
		nand_valdump("bbm_info->bb_list[] : ", (unsigned int)(unsigned long)bbm_info->bb_list, 32);
		nand_memdump((void *)bbm_info->bb_list, sizeof(bbm_info->bb_list));
		nand_valdump("bbm_info->area_info[] : ", (unsigned int)(unsigned long)bbm_info->area_info, 32);
		nand_memdump((void *)bbm_info->area_info, sizeof(bbm_info->area_info));
		nand_valdump("bbm_info->alternated_block_count : ", bbm_info->alternated_block_count, 32);
	}

	return nand_virt_to_phys_block(block, bbm_info);
}
