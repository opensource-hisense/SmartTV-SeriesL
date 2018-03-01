/*
 * Copyright (C) 2011-2015 Masahiro Yamada <yamada.masahiro@socionext.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>

#include "../init.h"
#include "../sc-regs.h"
#include "../sg-regs.h"

#undef DPLL_SSC_RATE_1PER

/**
 * FUNCTION:	STEP 0: Set paraleters to PLL
 *		STEP 1: Enable CPLL OSC
 *		STEP 2: Wait until the CPLL become stable
 *		STEP 3: Reserve the clock gear for gear1
 *		STEP 4: Update the gear
 *		STEP 5: Wait until the gear is updated
 *		STEP 6: Setup the clock selection registers
 *		STEP 7: Reset off for all cores
 *		STEP 8: Wait the propagation of reset signal
 *		STEP 9: Start clocks for all cores
 */

void dpll_init(void)
{
	u32 tmp;

	/* Stop DPLL */
	tmp = readl(SC_DPLLOSCCTRL);
	tmp &= ~SC_DPLLOSCCTRL_DPLLEN;
	writel(tmp, SC_DPLLOSCCTRL);

	while (readl(SC_DPLLOSCCTRL) & SC_DPLLOSCCTRL_DPLLST)
		;

#if CONFIG_DDR_FREQ == 1600
	/* Set LPFSEL in DPLLCTRL3 from 2 to 3 */
	tmp = readl(SC_DPLLCTRL3);
	tmp |= SC_DPLLCTRL3_LPFSEL_COEF3;
	writel(tmp, SC_DPLLCTRL3);
#elif CONFIG_DDR_FREQ == 1333
	/* Modify DPLLCTRL. FOUT_MODE[3:0] = 0x0000 */
	tmp = readl(SC_DPLLCTRL);
	tmp &= ~SC_DPLLCTRL_FOUTMODE_MASK;
	writel(tmp, SC_DPLLCTRL);
#else  /* if CONFIG_DDR_FREQ == X */
#  error "Unknown frequency"
#endif /* if CONFIG_DDR_FREQ == 1333 */

	tmp = readl(SC_DPLLCTRL);
#if defined(DPLL_SSC_RATE_1PER)
	tmp &= ~SC_DPLLCTRL_SSC_RATE;
#else
	tmp |= SC_DPLLCTRL_SSC_RATE;
#endif
	writel(tmp, SC_DPLLCTRL);

	/* Start DPLL */
	tmp = readl(SC_DPLLOSCCTRL);
	tmp |= SC_DPLLOSCCTRL_DPLLEN;
	writel(tmp, SC_DPLLOSCCTRL);
	while (!(readl(SC_DPLLOSCCTRL) & SC_DPLLOSCCTRL_DPLLST))
		;

	/* Set DPLLCTRL2 for SSC mode */
	tmp = readl(SC_DPLLCTRL2);
	tmp |= SC_DPLLCTRL2_NRSTDS;
	writel(tmp, SC_DPLLCTRL2);
}

void upll_init(void)
{
	u32 tmp;

	tmp = readl(SG_PINMON0);

	if ((tmp & SG_PINMON0_CLK_MODE_UPLLSRC_MASK)
	    != SG_PINMON0_CLK_MODE_UPLLSRC_DEFAULT)
		return;

	switch (tmp & SG_PINMON0_CLK_MODE_AXOSEL_MASK) {
	case SG_PINMON0_CLK_MODE_AXOSEL_25000KHZ_U:
	case SG_PINMON0_CLK_MODE_AXOSEL_25000KHZ_A: /* AXO: 25MHz */
		writel(0x0a28f5c3, SC_UPLLCTRL);
		writel(0x1a28f5c3, SC_UPLLCTRL);
		break;
	case SG_PINMON0_CLK_MODE_AXOSEL_20480KHZ: /* AXO: 20.48MHz */
		writel(0x0aa30000, SC_UPLLCTRL);
		writel(0x1aa30000, SC_UPLLCTRL);
		break;
	default: /* AXO: default 24.576MHz */
		writel(0x0a328000, SC_UPLLCTRL);
		writel(0x1a328000, SC_UPLLCTRL);
		break;
	}
}

void vpll_init(void)
{
	switch (readl(SG_PINMON0) & SG_PINMON0_CLK_MODE_AXOSEL_MASK) {
	case SG_PINMON0_CLK_MODE_AXOSEL_25000KHZ_U:
	case SG_PINMON0_CLK_MODE_AXOSEL_25000KHZ_A: /* AXO: 25MHz */
		writel(0x0a066666, SC_VPLL27ACTRL);
		writel(0x0a066666, SC_VPLL27BCTRL);
		writel(0x1a066666, SC_VPLL27ACTRL);
		writel(0x1a066666, SC_VPLL27BCTRL);
		break;
	case SG_PINMON0_CLK_MODE_AXOSEL_20480KHZ: /* AXO: 20.48MHz */
		writel(0x0a788d00, SC_VPLL27ACTRL);
		writel(0x0a788d00, SC_VPLL27BCTRL);
		writel(0x1a788d00, SC_VPLL27ACTRL);
		writel(0x1a788d00, SC_VPLL27BCTRL);
		break;
	default: /* AXO: default 24.576MHz */
		writel(0x0a0f5800, SC_VPLL27ACTRL);
		writel(0x0a0f5800, SC_VPLL27BCTRL);
		writel(0x1a0f5800, SC_VPLL27ACTRL);
		writel(0x1a0f5800, SC_VPLL27BCTRL);
		break;
	}
}

int uniphier_sld3_pll_init(const struct uniphier_board_data *bd)
{
	u32 tmp;
	int i;

	dpll_init();
	upll_init();
	vpll_init();

	/* wait 500 usec */
	udelay(500);

	/* Decide Gear number for ES2 */
#define	USE_CLOCK_GEAR	2

	/*
	 * STEP 0: Set CPLL frequency
	 * This modification is requred by SC development team
	 * For more details, see "[sato2_sg_sc:07899]".
	 */
#if USE_CLOCK_GEAR == 1
	writel(0x00004101, SC_CPLLCTRL);
#elif USE_CLOCK_GEAR == 2
	writel(0x00007a02, SC_CPLLCTRL);
#else
#  error "Unknown GEAR number"
#endif

	/*
	 * STEP 1: Enable CPLL OSC control
	 */
	tmp = readl(SC_CPLLOSCCTL);
	tmp |= SC_CPLLOSCCTL_CPLLEN;
	writel(tmp, SC_CPLLOSCCTL);

	while (!(readl(SC_CPLLOSCCTL) & SC_CPLLOSCCTL_CPLLST))
		;

	/*
	 * STEP 3: Reserve the clock gear for GEAR1 or GEAR2
	 */
#if USE_CLOCK_GEAR == 1
	writel(SC_SYSCLKSET_GEAR1, SC_SYSCLKSET);
#elif USE_CLOCK_GEAR == 2
	writel(SC_SYSCLKSET_GEAR2, SC_SYSCLKSET);
#else
#  error "Unknown GEAR number"
#endif /* USE_CLOCK_GEAR */

	/*
	 * STEP 4: Update the gear
	 */
	tmp = readl(SC_SYSCLKUPD);
	tmp |= SC_SYSCLKUPD_CLKUPDATE_EXEC;
	writel(tmp, SC_SYSCLKUPD);

	while ((readl(SC_SYSCLKUPD) & SC_SYSCLKUPD_CLKUPDATE_MASK) != SC_SYSCLKUPD_CLKUPDATE_READY)
		;

	/*
	 * Support the ES1/ES2 hardware problem
	 * This is reported by "sLD3_KidouSequenceFuguai_110523.ppt
	 * Initialize the synchronous reset logic by the software.
	 */
	/* 1. Wait the SC_APLLAOSCCTRL become 3 */

	while(readl(SC_APLLAOSCCTRL) != 0x03)
		;
	/* 2. Write 0x8033_0906 to CLKCTRL register */
	writel(0x80330906, SC_CLKCTRL);
	/* 3. Dummy read 20 times */
	for (i = 0; i < 20; i++)
		readl(SC_CLKCTRL);
	/* 4. Write 0x8020_0806 to CLKCTL register */
	writel(0x80200806, SC_CLKCTRL);
	/* 5. Dummy read 1 times */
	readl(SC_CLKCTRL);
	/* End of "sLD3_KidouSequenceFuguai_110523.ppt */

	return 0;
}
