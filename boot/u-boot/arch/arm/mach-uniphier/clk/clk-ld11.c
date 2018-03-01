/*
 * Copyright (C) 2016 Socionext Inc.
 *
 * based on commit 6a9896d2d173c51032293982bc131059ca34ee1b of Diag
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <spl.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <asm/processor.h>

#include "../boot-mode/boot-device.h"
#include "../init.h"
#include "../sc64-regs.h"
#include "../sg-regs.h"

#define SC_CPLLCTL		(SC_BASE_ADDR | 0x1400)
#define SC_CPLLCTL2		(SC_BASE_ADDR | 0x1404)

#define SC_SPLLCTL2		(SC_BASE_ADDR | 0x1414)

#define SC_MPLLCTL		(SC_BASE_ADDR | 0x1430)
#define SC_MPLLCTL2		(SC_BASE_ADDR | 0x1434)

#define SC_VSPLLCTL		(SC_BASE_ADDR | 0x1440)
#define SC_VSPLLCTL2		(SC_BASE_ADDR | 0x1444)

#define SC_DPLLCTL		(SC_BASE_ADDR | 0x1460)
#define SC_DPLLCTL2		(SC_BASE_ADDR | 0x1464)

#define   SC_SSC_EN			BIT(31)

#define SC_VPLL27FCTL		(SC_BASE_ADDR | 0x1500)
#define SC_VPLL27FCTL3		(SC_BASE_ADDR | 0x1508)
#define SC_VPLL27ACTL		(SC_BASE_ADDR | 0x1520)
#define SC_VPLL27ACTL3		(SC_BASE_ADDR | 0x1528)

#define SC_CA53_GEARSET		(SC_BASE_ADDR | 0x8084)
#define SC_CA53_GEARUP		(SC_BASE_ADDR | 0x8088)

struct clkrst_data {
	unsigned long address;
	u32 data;
};

static const struct clkrst_data reset_data[] = {
	{ SC_RSTCTRL,  0x00000001 },
	{ SC_RSTCTRL3, 0x000f0137 },
	{ SC_RSTCTRL4, 0x000003cf },
	{ SC_RSTCTRL5, 0x0000004f },
	{ SC_RSTCTRL6, 0x0000010b },
	{ SC_RSTCTRL7, 0x00000003 }
};

static const struct clkrst_data clock_data[] = {
	{ SC_CLKCTRL,  0x00000001 },
	{ SC_CLKCTRL3, 0x00000117 },
	{ SC_CLKCTRL4, 0x000037cf },
	{ SC_CLKCTRL5, 0x0000004f },
	{ SC_CLKCTRL6, 0x0000010b },
	{ SC_CLKCTRL7, 0x00000003 }
};

void uniphier_ld11_clk_init(void)
{
	u32 tmp;
	int i;

	/* if booted from a device other than USB, without stand-by MPU */
	if ((readl(SG_PINMON0) & BIT(27)) &&
	    spl_boot_device_raw() != BOOT_DEVICE_USB) {
		writel(1, SG_ETPHYPSHUT);
		writel(1, SG_ETPHYCNT);

		udelay(1); /* wait for regulator level 1.1V -> 2.5V */

		writel(3, SG_ETPHYCNT);
		writel(3, SG_ETPHYPSHUT);
		writel(7, SG_ETPHYCNT);
	}

	/* CPLL */
	tmp = readl(SC_CPLLCTL);
	tmp &= 0x3ff07fff;
	tmp |= (396 << 20) | 932;
	writel(tmp, SC_CPLLCTL);
	tmp = readl(SC_CPLLCTL2);
	tmp &= 0x07f0ffff;
	tmp |= (39 << 20) | 127507;
	writel(tmp, SC_CPLLCTL2);
	tmp |= BIT(28);
	writel(tmp, SC_CPLLCTL2);

	/* MPLL */
	tmp = readl(SC_MPLLCTL);
	tmp &= 0x3ff07fff;
	tmp |= (396 << 20) | 761;
	writel(tmp, SC_MPLLCTL);
	tmp = readl(SC_MPLLCTL2);
	tmp &= 0x07f0ffff;
	tmp |= (31 << 20) | 981467;
	writel(tmp, SC_MPLLCTL2);
	tmp |= BIT(28);
	writel(tmp, SC_MPLLCTL2);

	/* VSPLL */
	tmp = readl(SC_VSPLLCTL2);
	tmp |= BIT(28);
	writel(tmp, SC_VSPLLCTL2);

	/* DPLL */
	tmp = readl(SC_DPLLCTL2);
	tmp |= BIT(28);
	writel(tmp, SC_DPLLCTL2);

	/* VPLL27F */
	writel(1, SC_VPLL27FCTL);
	tmp = readl(SC_VPLL27FCTL3);
	tmp |= BIT(28);
	writel(tmp, SC_VPLL27FCTL3);
	writel(0, SC_VPLL27FCTL);

	/* VPLL27A */
	writel(1, SC_VPLL27ACTL);
	tmp = readl(SC_VPLL27ACTL3);
	tmp |= BIT(28);
	writel(tmp, SC_VPLL27ACTL3);
	writel(0, SC_VPLL27ACTL);

	/*
	 * CA53 gear
	 * 0: 980 MHz (CPLL)
	 * 1: 500 MHz (SPLL)
	 * 2: 653 MHz (CPLL)
	 * 3: 666 MHz (SPLL)
	 * 5: 250 MHz (SPLL)
	 * 6: 490 MHz (CPLL)
	 * 7: 245 MHz (CPLL)
	 */
	writel(0, SC_CA53_GEARSET);
	writel(1, SC_CA53_GEARUP);
	while (readl(SC_CA53_GEARUP) & 1)
		cpu_relax();

	mdelay(1);

	/* enable Spectrum Scatter Control */
	tmp = readl(SC_CPLLCTL);
	tmp |= SC_SSC_EN;
	writel(tmp, SC_CPLLCTL);

	tmp = readl(SC_MPLLCTL);
	tmp |= SC_SSC_EN;
	writel(tmp, SC_MPLLCTL);

	tmp = readl(SC_VSPLLCTL);
	tmp |= SC_SSC_EN;
	writel(tmp, SC_VSPLLCTL);

	tmp = readl(SC_DPLLCTL);
	tmp |= SC_SSC_EN;
	writel(tmp, SC_DPLLCTL);

	/* reset NANDC when bootmode_sel is nand only. */
	if(((readl(SG_PINMON0) >> 1) & 0x1f) <= 0x17) {
		tmp = readl(SC_RSTCTRL4);
		tmp &= ~SC_RSTCTRL4_NAND;
		writel(tmp, SC_RSTCTRL4);
		udelay(10);
	}

	for (i = 0; i < ARRAY_SIZE(clock_data); i++){
		writel(clock_data[i].data, clock_data[i].address);
		udelay(10);
	}

	for (i = 0; i < ARRAY_SIZE(reset_data); i++){
		writel(reset_data[i].data, reset_data[i].address);
		udelay(10);
	}
}
