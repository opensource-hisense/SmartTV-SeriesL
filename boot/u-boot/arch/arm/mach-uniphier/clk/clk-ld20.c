/*
 * Copyright (C) 2016 Masahiro Yamada <yamada.masahiro@socionext.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <linux/io.h>

#include "../init.h"
#include "../sc64-regs.h"
#include "../sg-regs.h"

#define SC_CPLLCTL		(SC_BASE_ADDR | 0x1400)
#define SC_CPLLCTL2		(SC_BASE_ADDR | 0x1404)
#define SC_SPLLCTL2		(SC_BASE_ADDR | 0x1414)
#define SC_SPLL2CTL		(SC_BASE_ADDR | 0x1420)
#define SC_SPLL2CTL2		(SC_BASE_ADDR | 0x1424)
#define SC_MPLLCTL		(SC_BASE_ADDR | 0x1430)
#define SC_MPLLCTL2		(SC_BASE_ADDR | 0x1434)
#define SC_VPPLLCTL		(SC_BASE_ADDR | 0x1440)
#define SC_VPPLLCTL2		(SC_BASE_ADDR | 0x1444)
#define SC_GPPLLCTL		(SC_BASE_ADDR | 0x1450)
#define SC_GPPLLCTL2		(SC_BASE_ADDR | 0x1454)
#define SC_DPLL0CTL2		(SC_BASE_ADDR | 0x1464)
#define SC_DPLL1CTL2		(SC_BASE_ADDR | 0x1474)
#define SC_DPLL2CTL2		(SC_BASE_ADDR | 0x1484)

#define   SC_SSC_EN			BIT(31)

#define SC_VPLL27FCTL		(SC_BASE_ADDR | 0x1500)
#define SC_VPLL27FCTL3		(SC_BASE_ADDR | 0x1508)
#define SC_VPLL27ACTL		(SC_BASE_ADDR | 0x1520)
#define SC_VPLL27ACTL3		(SC_BASE_ADDR | 0x1528)

#define SC_VPLL8KCTL2		(SC_BASE_ADDR | 0x1544)
#define SC_A2PLLCTL2		(SC_BASE_ADDR | 0x15C4)

struct clkrst_data {
	unsigned long address;
	u32 data;
};

static const struct clkrst_data reset_data[] = {
	{ SC_RSTCTRL,  0x00000001 },
	{ SC_RSTCTRL3, 0x000001f7 },
	{ SC_RSTCTRL4, 0x0000f3ff },
	{ SC_RSTCTRL5, 0x0000007f },
	{ SC_RSTCTRL6, 0x0000010f },
	{ SC_RSTCTRL7, 0x00010707 }
};

static const struct clkrst_data clock_data[] = {
	{ SC_CLKCTRL,  0x00000001 },
	{ SC_CLKCTRL3, 0x00000137 },
	{ SC_CLKCTRL4, 0x0000f3ff },
	{ SC_CLKCTRL5, 0x0000007f },
	{ SC_CLKCTRL6, 0x0000010f },
	{ SC_CLKCTRL7, 0x00010007 }
};

static const unsigned long pllctl2_regs[] = {
	SC_CPLLCTL2,
	SC_SPLL2CTL2,
	SC_MPLLCTL2,
	SC_VPPLLCTL2,
	SC_GPPLLCTL2,
	SC_DPLL0CTL2,
	SC_DPLL1CTL2,
	SC_DPLL2CTL2,
	SC_VPLL8KCTL2,
	SC_A2PLLCTL2,
};

void uniphier_ld20_clk_init(void)
{
	u32 tmp;
	int i;

	/* initialize PLLs */
	for (i = 0; i < ARRAY_SIZE(pllctl2_regs); i++) {
		tmp = readl(pllctl2_regs[i]);
		tmp |= BIT(28);
		writel(tmp, pllctl2_regs[i]);
	}

	writel(1, SC_VPLL27FCTL);
	tmp = readl(SC_VPLL27FCTL3);
	tmp |= BIT(28);
	writel(tmp, SC_VPLL27FCTL3);
	writel(0, SC_VPLL27FCTL);

	writel(1, SC_VPLL27ACTL);
	tmp = readl(SC_VPLL27ACTL3);
	tmp |= BIT(28);
	writel(tmp, SC_VPLL27ACTL3);
	writel(0, SC_VPLL27ACTL);

	mdelay(1);

	/* enable Spectrum Scatter Control */
	tmp = readl(SC_CPLLCTL);
	tmp |= SC_SSC_EN;
	writel(tmp, SC_CPLLCTL);

	tmp = readl(SC_SPLL2CTL);
	tmp |= SC_SSC_EN;
	writel(tmp, SC_SPLL2CTL);

	tmp = readl(SC_MPLLCTL);
	tmp |= SC_SSC_EN;
	writel(tmp, SC_MPLLCTL);

	tmp = readl(SC_VPPLLCTL);
	tmp |= SC_SSC_EN;
	writel(tmp, SC_VPPLLCTL);

	tmp = readl(SC_GPPLLCTL);
	tmp |= SC_SSC_EN;
	writel(tmp, SC_GPPLLCTL);

	/* reset NANDC when bootmode_sel is nand only. */
	if(((readl(SG_PINMON0) >> 1) & 0x1f) <= 0x17) {
		tmp = readl(SC_RSTCTRL4);
		tmp &= ~SC_RSTCTRL4_NAND;
		writel(tmp, SC_RSTCTRL4);
		udelay(10);
	}

	for (i = 0; i < ARRAY_SIZE(clock_data); i++) {
		writel(clock_data[i].data, clock_data[i].address);
		udelay(10);
	}

	for (i = 0; i < ARRAY_SIZE(reset_data); i++) {
		writel(reset_data[i].data, reset_data[i].address);
		udelay(10);
	}
}
