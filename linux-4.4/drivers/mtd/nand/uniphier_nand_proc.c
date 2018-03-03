/* $Id$
 * uniphier_nand_proc.c: proc interface for uniphier nand driver
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

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>

#include <linux/slab.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/mtd.h>
#include <uapi/mtd/mtd-abi.h>

#include "uniphier_nand.h"

#if defined(CONFIG_PROC_FS) && defined(CONFIG_MTD_NAND_UNIPHIER_BBM)

#define PROC_NAND_DIR		"driver/uniphier-nand"
#define PROC_BBMINFO		"bbminfo"
#define PROC_ECCERR_TEST	"eccerr_test"
#define PROC_BBM_TEST		"bbm_test"

static void proc_bbminfo_show_area(struct seq_file *m,
				   struct nand_gpbc_info *gpbc)
{
	struct nand_chip *chip = &gpbc->chip;
	struct nand_bbm *bbm;
	int top, tail, i;

	for (i = 0; i < chip->numchips; i++) {
		bbm = &chip->psBbm[i];
		seq_printf(m, "  (chip %d)\n", i);

		if (bbm->area[NAND_AREA_ID_BOOT].blockNum) {
			top = bbm->area[NAND_AREA_ID_BOOT].startBlock;
			tail = bbm->area[NAND_AREA_ID_BOOT].startBlock +
				bbm->area[NAND_AREA_ID_BOOT].blockNum - 1;
			seq_printf(m, "    BOOT: % 5d - % 5d\n",
				   top, tail);
		} else {
			seq_printf(m, "    BOOT: not allocated\n");
		}

		if (bbm->area[NAND_AREA_ID_DATA].blockNum) {
			top = bbm->area[NAND_AREA_ID_DATA].startBlock;
			tail = bbm->area[NAND_AREA_ID_DATA].startBlock +
				bbm->area[NAND_AREA_ID_DATA].blockNum - 1;
			seq_printf(m, "    DATA: % 5d - % 5d\n",
				   top, tail);
		} else {
			seq_printf(m, "    DATA:  not allocated\n");
		}

		if (bbm->area[NAND_AREA_ID_ALT].blockNum) {
			top = bbm->area[NAND_AREA_ID_ALT].startBlock;
			tail = bbm->area[NAND_AREA_ID_ALT].startBlock +
				bbm->area[NAND_AREA_ID_ALT].blockNum - 1;
			seq_printf(m, "    ALT : % 5d - % 5d\n",
				   top, tail);
		} else {
			seq_printf(m, "    ALT :  not allocated\n");
		}

		if (bbm->area[NAND_AREA_ID_BBM].blockNum) {
			top = bbm->area[NAND_AREA_ID_BBM].startBlock;
			tail = bbm->area[NAND_AREA_ID_BBM].startBlock +
				bbm->area[NAND_AREA_ID_BBM].blockNum - 1;
			seq_printf(m, "    BBM : % 5d - % 5d\n",
				   top, tail);
		} else {
			seq_printf(m, "    BBM :  not allocated\n");
		}
	}
}

static int get_badblock_num(struct nand_gpbc_info *gpbc)
{
	struct mtd_info *mtd = &gpbc->mtd;
	struct nand_chip *chip = &gpbc->chip;
	int i, ret = 0;
	unsigned int allow = (NAND_ALLOW_BBT | NAND_ALLOW_BADDATA);
	loff_t offset;
	int blocks_per_chip = 1 << (chip->chip_shift - chip->bbt_erase_shift);
	int total_block = blocks_per_chip * chip->numchips;

	for (i = 0; i < total_block; i++) {
		offset = (loff_t)i << chip->bbt_erase_shift;
		if (nand_isbad_bbt(mtd, offset, allow)) {
			ret++;
		}
	}
	return ret;
}

static void show_badblock_range(struct seq_file *m, struct nand_gpbc_info *gpbc,
				int start, int num)
{
	struct mtd_info *mtd = &gpbc->mtd;
	struct nand_chip *chip = &gpbc->chip;
	int i, cnt = 0;
	unsigned int allow = (NAND_ALLOW_BBT | NAND_ALLOW_BADDATA);
	loff_t offset;

	for (i = start; i < start + num; i++) {
		offset = (loff_t)i << chip->bbt_erase_shift;
		if (nand_isbad_bbt(mtd, offset, allow)) {
			seq_printf(m, " %d,", i);
			cnt++;
		}
	}

	if (cnt == 0) {
		seq_printf(m, " none");
	}
}

static void proc_bbminfo_show_badblock_info(struct seq_file *m,
					    struct nand_gpbc_info *gpbc)
{
	struct nand_chip *chip = &gpbc->chip;
	struct nand_bbm *bbm;
	int i, badblock_num;
	int top, num;

	badblock_num = get_badblock_num(gpbc);
	seq_printf(m, "BadBlock num: %d\n", badblock_num);

	seq_printf(m, "\n");
	seq_printf(m, "BadBlock position:\n");
	for (i = 0; i < chip->numchips; i++) {
		bbm = &chip->psBbm[i];
		seq_printf(m, "  (chip %d)\n", i);

		seq_printf(m, "    BOOT:");
		top = bbm->area[NAND_AREA_ID_BOOT].startBlock;
		num = bbm->area[NAND_AREA_ID_BOOT].blockNum;
		show_badblock_range(m, gpbc, top, num);
		seq_printf(m, "\n");

		seq_printf(m, "    DATA:");
		top = bbm->area[NAND_AREA_ID_DATA].startBlock;
		num = bbm->area[NAND_AREA_ID_DATA].blockNum;
		show_badblock_range(m, gpbc, top, num);
		seq_printf(m, "\n");

		seq_printf(m, "    ALT :");
		top = bbm->area[NAND_AREA_ID_ALT].startBlock;
		num = bbm->area[NAND_AREA_ID_ALT].blockNum;
		show_badblock_range(m, gpbc, top, num);
		seq_printf(m, "\n");

		seq_printf(m, "    BBM :");
		top = bbm->area[NAND_AREA_ID_BBM].startBlock;
		num = bbm->area[NAND_AREA_ID_BBM].blockNum;
		show_badblock_range(m, gpbc, top, num);
		seq_printf(m, "\n");
	}
}

static void proc_bbminfo_show_bbm_blocks(struct seq_file *m,
					 struct nand_gpbc_info *gpbc)
{
	struct nand_chip *chip = &gpbc->chip;
	struct nand_bbm *bbm;
	int i, master, mirror;

	seq_printf(m, "BBM position:\n");
	for (i = 0; i < chip->numchips; i++) {
		bbm = &chip->psBbm[i];
		master = chip->bbt_td->pages[i] >>
			(chip->bbt_erase_shift - chip->page_shift);
		mirror = chip->bbt_md->pages[i] >>
			(chip->bbt_erase_shift - chip->page_shift);

		seq_printf(m, "  (chip %d)\n", i);
		seq_printf(m, "    MASTER: block %d\n", master);
		seq_printf(m, "    MIRROR: block %d\n", mirror);
	}
}

static void proc_bbminfo_show_bb_list(struct seq_file *m,
				      struct nand_gpbc_info *gpbc)
{
	struct nand_chip *chip = &gpbc->chip;
	struct nand_bbm *bbm;
	struct nand_bbList *list;
	int i, j;

	seq_printf(m, "BadBlock Lists:\n");
	seq_printf(m, "  [BadBlock]    [AltBlock]\n");
	for (i = 0; i < chip->numchips; i++) {
		bbm = &chip->psBbm[i];
		list = bbm->psBbList;
		seq_printf(m, "  (chip %d)\n", i);
		if (bbm->altBlocks == 0) {
			seq_printf(m, "    not replaced.\n");
		}
		for (j = 0; j < bbm->altBlocks; j++) {
			seq_printf(m, "    % 6d   ->   % 6d\n",
				   list[j].badBlock, list[j].altBlock);
		}
	}
}

static int proc_bbminfo_show(struct seq_file *m, void *v)
{
	struct nand_gpbc_info *gpbc = (struct nand_gpbc_info *)m->private;

	proc_bbminfo_show_area(m, gpbc);
	seq_printf(m, "\n");

	proc_bbminfo_show_badblock_info(m, gpbc);
	seq_printf(m, "\n");

	proc_bbminfo_show_bbm_blocks(m, gpbc);
	seq_printf(m, "\n");

	proc_bbminfo_show_bb_list(m, gpbc);
	seq_printf(m, "\n");

	return 0;
}

static int read_one_page(struct nand_gpbc_info *gpbc, int page,
			 unsigned char *buf)
{
	struct mtd_info *mtd = &gpbc->mtd;
	int ret = 0;
	size_t retlen = 0;
	size_t len = mtd->writesize;
	loff_t offset = page * mtd->writesize;

	ret = mtd->_read(mtd, offset, len, &retlen, buf);
	if (retlen != len) {
		return -EIO;
	}

	return ret;
}

static int proc_eccerr_test_show(struct seq_file *m, void *v)
{
	struct nand_gpbc_info *gpbc = (struct nand_gpbc_info *)m->private;
	struct mtd_info *mtd = &gpbc->mtd;
	struct nand_chip *chip = &gpbc->chip;
	unsigned char *page_buf;
	int i, found = 0, ret = 0;
	int cnt_uncorrected = 0, cnt_corrected = 0;
	int pages_per_chip = 1 << (chip->chip_shift - chip->page_shift);
	int total_page = pages_per_chip * chip->numchips;
	unsigned int allow = (NAND_ALLOW_BBT | NAND_ALLOW_BADDATA);
	loff_t ofs;

	page_buf = kmalloc(mtd->writesize, GFP_KERNEL);
	if (!page_buf) {
		return -ENOMEM;
	}

	pr_cont("Checking bit error");
	seq_printf(m, "ECC positions:\n");
	seq_printf(m, "    [PAGE]   [correctnum]\n");
	for (i = 0; i < total_page; i++) {
		if (i % 10000 == 0) {
			pr_cont(".");
		}

		ofs = (loff_t)i << chip->page_shift;
		if (nand_isbad_bbt(mtd, ofs, allow)) {
			continue;
		}

		ret = read_one_page(gpbc, i, page_buf);

		if(ret > 0) {
			seq_printf(m, "  % 8d  % 2d\n", i, ret);
			cnt_corrected++;
		} else if(ret == -EBADMSG) {
			seq_printf(m, "  % 8d  cannot correct\n", i);
			cnt_uncorrected++;
		} else if(ret) {
			seq_printf(m, "cannot read page %d (err = %d)\n",
				   i, ret);
			found = 1;
		}
	}
	kfree(page_buf);
	pr_cont("\n");
	if (cnt_corrected || cnt_uncorrected) {
		seq_printf(m, "ECC corrected page num    : %d\n",
			   cnt_corrected);
		seq_printf(m, "ECC uncorrectable page num: %d\n",
			   cnt_uncorrected);
	} else if (!found) {
		seq_printf(m, "Bit error is not detected.\n");
	}

	return 0;
}

static int check_badblock_list(struct nand_gpbc_info *gpbc, int block)
{
	struct nand_chip *chip = &gpbc->chip;
	struct nand_bbm *bbm;
	struct nand_bbList *list;
	int blocks_per_chip = 1 << (chip->chip_shift - chip->bbt_erase_shift);
	int chip_number = block / blocks_per_chip;
	int i;

	bbm = &chip->psBbm[chip_number];
	list = bbm->psBbList;

	for (i = 0; i < bbm->altBlocks; i++) {
		if (block == list[i].badBlock) {
			return 1;
		}
	}

	return 0;
}

static int proc_bbm_test_show(struct seq_file *m, void *v)
{
	struct nand_gpbc_info *gpbc = (struct nand_gpbc_info *)m->private;
	struct mtd_info *mtd = &gpbc->mtd;
	struct nand_chip *chip = &gpbc->chip;
	struct nand_bbm *bbm;
	unsigned int allow = (NAND_ALLOW_BBT | NAND_ALLOW_BADDATA);
	loff_t ofs;
	int top, num, cnt = 0, i, j;

	seq_printf(m, "Checking BBM consistency...");
	for (i = 0; i < chip->numchips; i++) {
		bbm = &chip->psBbm[i];
		top = bbm->area[NAND_AREA_ID_DATA].startBlock;
		num = bbm->area[NAND_AREA_ID_DATA].blockNum;

		for (j = top; j < top + num; j++) {

			ofs = (loff_t)j << chip->bbt_erase_shift;
			if (nand_isbad_bbt(mtd, ofs, allow) &&
			    !check_badblock_list(gpbc, j)) {
				seq_printf(m,
					   "\nBlock %d is BadBlock, but not replaced!!", j);
				cnt++;
				}
		}
	}
	if (cnt == 0) {
		seq_printf(m, " OK\nAll BadBlocks are replaced properly.\n");
	} else {
		seq_printf(m, "\n");
	}

	return 0;
}

static int proc_bbminfo_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_bbminfo_show, PDE_DATA(inode));
}

static int proc_eccerr_test_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_eccerr_test_show, PDE_DATA(inode));
}

static int proc_bbm_test_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_bbm_test_show, PDE_DATA(inode));
}

static struct proc_dir_entry* dir_entry;

static struct file_operations proc_bbminfo_fops = {
	.owner = THIS_MODULE,
	.open = proc_bbminfo_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static struct file_operations proc_eccerr_test_fops = {
	.owner = THIS_MODULE,
	.open = proc_eccerr_test_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static struct file_operations proc_bbm_test_fops = {
	.owner = THIS_MODULE,
	.open = proc_bbm_test_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int nand_proc_init(struct nand_gpbc_info *gpbc)
{
	struct proc_dir_entry* entry;

	dir_entry = proc_mkdir(PROC_NAND_DIR, NULL);
	if (!dir_entry) {
		pr_err("[NAND] proc_mkdir failed\n");
		return -EBUSY;
	}

	entry = proc_create_data(PROC_BBMINFO, S_IRUSR, dir_entry,
				 &proc_bbminfo_fops, (void *)gpbc);
	if (!entry) {
		pr_err("[NAND] proc_create_data failed@%s\n",
			PROC_BBMINFO);
		goto cleanup;
	}

	entry = proc_create_data(PROC_ECCERR_TEST, S_IRUSR, dir_entry,
				 &proc_eccerr_test_fops, (void *)gpbc);
	if (!entry) {
		pr_err("[NAND] proc_create_data failed@%s\n",
		       PROC_ECCERR_TEST);
		goto cleanup;
	}

	entry = proc_create_data(PROC_BBM_TEST, S_IRUSR, dir_entry,
				 &proc_bbm_test_fops, (void *)gpbc);
	if (!entry) {
		pr_err("[NAND] proc_create_data failed@%s\n",
		       PROC_BBM_TEST);
		goto cleanup;
	}
	return 0;

 cleanup:
	remove_proc_subtree(PROC_NAND_DIR, NULL);
	return -EBUSY;
}

void nand_proc_exit(void)
{
	remove_proc_subtree(PROC_NAND_DIR, NULL);
}

#else /* CONFIG_PROC_FS && CONFIG_MTD_NAND_UNIPHIER_BBM */

int nand_proc_init(struct nand_gpbc_info *gpbc __attribute__ ((unused)))
{
	return 0;
}

void nand_proc_exit(void) { }

#endif /* CONFIG_PROC_FS && CONFIG_MTD_NAND_UNIPHIER_BBM */
