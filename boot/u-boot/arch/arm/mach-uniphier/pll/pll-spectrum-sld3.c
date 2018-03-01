/*
 * Copyright (C) 2011-2015 Masahiro Yamada <yamada.masahiro@socionext.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <linux/io.h>

#include "../init.h"
#include "../sc-regs.h"
#include "../ph1-sld3/umc-regs-old.h"

/*
 * REFERENCE:	SC specification 101115
 */
int uniphier_sld3_enable_dpll_ssc(const struct uniphier_board_data *bd)
{
	int i;
	u32 tmp;

	/* UMC autorefresh OFF */
	for (i = 0; i < 3; i++) {
		tmp = readl(UMCSPCSETB(i));
		tmp &= ~0x3;
		tmp |= 0x2;
		writel(tmp, UMCSPCSETB(i));
	}

	/* wait 1 usec */
	udelay(1);

	/* Setup SSC ON */
	tmp = readl(SC_DPLLCTRL);
	tmp |= SC_DPLLCTRL_SSC_EN;
	writel(tmp, SC_DPLLCTRL);

	/* wait 10 usec */
	udelay(10);

	/* UMC autorefresh ON */
	for (i = 0; i < 3; i++) {
		tmp = readl(UMCSPCSETB(i));
		tmp &= ~0x3;
		writel(tmp, UMCSPCSETB(i));
	}

	return 0;
}
