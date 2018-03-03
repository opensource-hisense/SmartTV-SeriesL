/* $Id$
 * uniphier_nand_timing.c: timing parameter for uniphier nand driver
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

#ifndef __UBOOT__
#include <linux/module.h>
#else
#include <common.h>
#endif
#include <linux/mtd/nand.h>

#include "uniphier_nand.h"


const struct nand_gpbc_timing nand_timing_lists[] = {
	/* mode 0 */
	{
	  .ncdwr = 0x0000141a, .ncaad = 0x00001415, .nrw   = 0x0000002b, .nrdcp = 0x0000000d,
	  .nrwlp = 0x0000000f, .nrwhp = 0x00000008, .nces  = 0x0000a005, .nrr   = 0x00000004
	},
	/* mode 1 */
	{
	  .ncdwr = 0x00001412, .ncaad = 0x0000140b, .nrw   = 0x00000017, .nrdcp = 0x0000000a,
	  .nrwlp = 0x00000009, .nrwhp = 0x00000005, .nces  = 0x0000a003, .nrr   = 0x00000004
	},
	/* mode 2 */
	{
	  .ncdwr = 0x00001412, .ncaad = 0x0000140c, .nrw   = 0x00000017, .nrdcp = 0x00000009,
	  .nrwlp = 0x00000008, .nrwhp = 0x00000005, .nces  = 0x0000a003, .nrr   = 0x00000004
	},
	/* mode 3 */
	{
	  .ncdwr = 0x0000140e, .ncaad = 0x0000140e, .nrw   = 0x00000017, .nrdcp = 0x00000008,
	  .nrwlp = 0x00000007, .nrwhp = 0x00000004, .nces  = 0x0000a003, .nrr   = 0x00000004
	},
	/* mode 4 */
	{
	  .ncdwr = 0x0000140e, .ncaad = 0x0000140a, .nrw   = 0x00000017, .nrdcp = 0x00000008,
	  .nrwlp = 0x00000007, .nrwhp = 0x00000004, .nces  = 0x0000a003, .nrr   = 0x00000004
	},
	/* mode 5 */
	{
	  .ncdwr = 0x0000140e, .ncaad = 0x0000140a, .nrw   = 0x00000017, .nrdcp = 0x00000007,
	  .nrwlp = 0x00000006, .nrwhp = 0x00000004, .nces  = 0x0000a003, .nrr   = 0x00000004
	},
};


struct nand_gpbc_timing *nand_gpbc_get_timing(struct nand_gpbc_info *gpbc)
{
	struct nand_chip *chip = &gpbc->chip;
	int mode;

	mode = onfi_get_async_timing_mode(chip);
	if(mode != ONFI_TIMING_MODE_UNKNOWN) {
		mode = fls(mode) -1;
		if(mode < 0)
			mode = 0;
	} else {
		mode = chip->onfi_timing_mode_default;
	}

	pr_info("[uniphier-nand] ONFI timing mode: %d\n", mode);

	return (struct nand_gpbc_timing *)&nand_timing_lists[mode];
}
