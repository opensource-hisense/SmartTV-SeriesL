/*
 * Copyright (C) 2011-2015 Masahiro Yamada <yamada.masahiro@socionext.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>

#include "../sg-regs.h"
#include "umc-regs-old.h"

#define DIO_BOOT_DUTY_ADJUST

#if defined(DIO_BOOT_DUTY_ADJUST)
static inline void set_duty(u32 efuse, int shift, u32 addr, u32 val3, u32 val2, u32 val1, u32 val4, u32 val5, u32 val6, u32 val7)
{
	u32 val;

	efuse = (efuse >> shift) & 0x7;

	switch (efuse) {
	case 3:
		val = val3;
		break;
	case 2:
		val = val2;
		break;
	case 1:
		val = val1;
		break;
	case 4:
		val = val4;
		break;
	case 5:
		val = val5;
		break;
	case 6:
		val = val6;
		break;
	case 7:
		val = val7;
		break;
	case 0:
	default:
		/* do nothing */
		return;
	}

	writel(val, addr);
}

static inline void adjust_duty(void)
{
	u32 efuse;

	/* Read eFuse for CH0. */
	efuse = readl(SG_EFUSEMON1) & 0x3fffffff;

	/* CH0: CK-Duty */
	/* DIO_CH0_CK_CKN_PU_SLEW */
	set_duty(efuse, 0, 0x5bc030f0, 0x00000027, 0x0000001f, 0x0000001e, 0x0000001b, 0x00000033, 0x0000003b, 0x0000003c);
	/* DIO_CH0_CK_CKN_PD_SLEW */
	set_duty(efuse, 3, 0x5bc03108, 0x0000003c, 0x0000003b, 0x00000033, 0x0000001b, 0x0000001e, 0x0000001f, 0x00000027);

	/* CH0: DQS-Duty */
	/* DIO_CH0_DQ0A_PU_SLEW_1 */
	set_duty(efuse, 6, 0x5bc0406c, 0x000076db, 0x000076db, 0x000066db, 0x000036db, 0x000036db, 0x000036db, 0x000046db);
	/* DIO_CH0_DQ0A_PD_SLEW_1 */
	set_duty(efuse, 6, 0x5bc04078, 0x000046db, 0x000036db, 0x000036db, 0x000036db, 0x000066db, 0x000076db, 0x000076db);
	/* DIO_CH0_DQ0A_PU_SLEW_2 */
	set_duty(efuse, 9, 0x5bc04070, 0x00000004, 0x00000003, 0x00000003, 0x00000003, 0x00000006, 0x00000007, 0x00000007);
	/* DIO_CH0_DQ0A_PD_SLEW_2 */
	set_duty(efuse, 9, 0x5bc0407c, 0x00000007, 0x00000007, 0x00000006, 0x00000003, 0x00000003, 0x00000003, 0x00000004);
	/* DIO_CH0_DQ0B_PU_SLEW_1 */
	set_duty(efuse, 12, 0x5bc04164, 0x000076db, 0x000076db, 0x000066db, 0x000036db, 0x000036db, 0x000036db, 0x000046db);
	/* DIO_CH0_DQ0B_PD_SLEW_1 */
	set_duty(efuse, 12, 0x5bc04170, 0x000046db, 0x000036db, 0x000036db, 0x000036db, 0x000066db, 0x000076db, 0x000076db);
	/* DIO_CH0_DQ0B_PU_SLEW_2 */
	set_duty(efuse, 15, 0x5bc04168, 0x00000004, 0x00000003, 0x00000003, 0x00000003, 0x00000006, 0x00000007, 0x00000007);
	/* DIO_CH0_DQ0B_PD_SLEW_2 */
	set_duty(efuse, 15, 0x5bc04174, 0x00000007, 0x00000007, 0x00000006, 0x00000003, 0x00000003, 0x00000003, 0x00000004);
	/* DIO_CH0_DQ1A_PU_SLEW_1 */
	set_duty(efuse, 18, 0x5bc0506c, 0x000076db, 0x000076db, 0x000066db, 0x000036db, 0x000036db, 0x000036db, 0x000046db);
	/* DIO_CH0_DQ1A_PD_SLEW_1 */
	set_duty(efuse, 18, 0x5bc05078, 0x000046db, 0x000036db, 0x000036db, 0x000036db, 0x000066db, 0x000076db, 0x000076db);
	/* DIO_CH0_DQ1A_PU_SLEW_2 */
	set_duty(efuse, 21, 0x5bc05070, 0x00000004, 0x00000003, 0x00000003, 0x00000003, 0x00000006, 0x00000007, 0x00000007);
	/* DIO_CH0_DQ1A_PD_SLEW_2 */
	set_duty(efuse, 21, 0x5bc0507c, 0x00000007, 0x00000007, 0x00000006, 0x00000003, 0x00000003, 0x00000003, 0x00000004);
	/* DIO_CH0_DQ1B_PU_SLEW_1 */
	set_duty(efuse, 24, 0x5bc05164, 0x000076db, 0x000076db, 0x000066db, 0x000036db, 0x000036db, 0x000036db, 0x000046db);
	/* DIO_CH0_DQ1B_PD_SLEW_1 */
	set_duty(efuse, 24, 0x5bc05170, 0x000046db, 0x000036db, 0x000036db, 0x000036db, 0x000066db, 0x000076db, 0x000076db);
	/* DIO_CH0_DQ1B_PU_SLEW_2 */
	set_duty(efuse, 27, 0x5bc05168, 0x00000004, 0x00000003, 0x00000003, 0x00000003, 0x00000006, 0x00000007, 0x00000007);
	/* DIO_CH0_DQ1B_PD_SLEW_2 */
	set_duty(efuse, 27, 0x5bc05174, 0x00000007, 0x00000007, 0x00000006, 0x00000003, 0x00000003, 0x00000003, 0x00000004);

	/* Read eFuse for CH1. */
	efuse = ((readl(SG_EFUSEMON2) << 2) | (readl(SG_EFUSEMON1) >> 30)) & 0x3fffffff;

	/* CH1: CK-Duty */
	/* DIO_CH1_CK_CKN_PU_SLEW */
	set_duty(efuse, 0, 0x5be030f0, 0x00000027, 0x0000001f, 0x0000001e, 0x0000001b, 0x00000033, 0x0000003b, 0x0000003c);
	/* DIO_CH1_CK_CKN_PD_SLEW */
	set_duty(efuse, 3, 0x5be03108, 0x0000003c, 0x0000003b, 0x00000033, 0x0000001b, 0x0000001e, 0x0000001f, 0x00000027);

	/* CH1: DQS-Duty */
	/* DIO_CH1_DQ0A_PU_SLEW_1 */
	set_duty(efuse, 6, 0x5be0406c, 0x000076db, 0x000076db, 0x000066db, 0x000036db, 0x000036db, 0x000036db, 0x000046db);
	/* DIO_CH1_DQ0A_PD_SLEW_1 */
	set_duty(efuse, 6, 0x5be04078, 0x000046db, 0x000036db, 0x000036db, 0x000036db, 0x000066db, 0x000076db, 0x000076db);
	/* DIO_CH1_DQ0A_PU_SLEW_2 */
	set_duty(efuse, 9, 0x5be04070, 0x00000004, 0x00000003, 0x00000003, 0x00000003, 0x00000006, 0x00000007, 0x00000007);
	/* DIO_CH1_DQ0A_PD_SLEW_2 */
	set_duty(efuse, 9, 0x5be0407c, 0x00000007, 0x00000007, 0x00000006, 0x00000003, 0x00000003, 0x00000003, 0x00000004);
	/* DIO_CH1_DQ0B_PU_SLEW_1 */
	set_duty(efuse, 12, 0x5be04164, 0x000076db, 0x000076db, 0x000066db, 0x000036db, 0x000036db, 0x000036db, 0x000046db);
	/* DIO_CH1_DQ0B_PD_SLEW_1 */
	set_duty(efuse, 12, 0x5be04170, 0x000046db, 0x000036db, 0x000036db, 0x000036db, 0x000066db, 0x000076db, 0x000076db);
	/* DIO_CH1_DQ0B_PU_SLEW_2 */
	set_duty(efuse, 15, 0x5be04168, 0x00000004, 0x00000003, 0x00000003, 0x00000003, 0x00000006, 0x00000007, 0x00000007);
	/* DIO_CH1_DQ0B_PD_SLEW_2 */
	set_duty(efuse, 15, 0x5be04174, 0x00000007, 0x00000007, 0x00000006, 0x00000003, 0x00000003, 0x00000003, 0x00000004);
	/* DIO_CH1_DQ1A_PU_SLEW_1 */
	set_duty(efuse, 18, 0x5be0506c, 0x000076db, 0x000076db, 0x000066db, 0x000036db, 0x000036db, 0x000036db, 0x000046db);
	/* DIO_CH1_DQ1A_PD_SLEW_1 */
	set_duty(efuse, 18, 0x5be05078, 0x000046db, 0x000036db, 0x000036db, 0x000036db, 0x000066db, 0x000076db, 0x000076db);
	/* DIO_CH1_DQ1A_PU_SLEW_2 */
	set_duty(efuse, 21, 0x5be05070, 0x00000004, 0x00000003, 0x00000003, 0x00000003, 0x00000006, 0x00000007, 0x00000007);
	/* DIO_CH1_DQ1A_PD_SLEW_2 */
	set_duty(efuse, 21, 0x5be0507c, 0x00000007, 0x00000007, 0x00000006, 0x00000003, 0x00000003, 0x00000003, 0x00000004);
	/* DIO_CH1_DQ1B_PU_SLEW_1 */
	set_duty(efuse, 24, 0x5be05164, 0x000076db, 0x000076db, 0x000066db, 0x000036db, 0x000036db, 0x000036db, 0x000046db);
	/* DIO_CH1_DQ1B_PD_SLEW_1 */
	set_duty(efuse, 24, 0x5be05170, 0x000046db, 0x000036db, 0x000036db, 0x000036db, 0x000066db, 0x000076db, 0x000076db);
	/* DIO_CH1_DQ1B_PU_SLEW_2 */
	set_duty(efuse, 27, 0x5be05168, 0x00000004, 0x00000003, 0x00000003, 0x00000003, 0x00000006, 0x00000007, 0x00000007);
	/* DIO_CH1_DQ1B_PD_SLEW_2 */
	set_duty(efuse, 27, 0x5be05174, 0x00000007, 0x00000007, 0x00000006, 0x00000003, 0x00000003, 0x00000003, 0x00000004);
}
#else
static inline void adjust_duty(void)
{
}
#endif

static inline void umc_ssif_start(void)
{
	writel(0x03010100, 0x5b800898);		/* HDMCHSEL */
	writel(0x03010101, 0x5b80089c);		/* MDMCHSEL */
	writel(0x03010100, 0x5b8008F0);		/* DMDCHSEL */
	writel(0x03010101, 0x5b8008F4);		/* OVDCHSEL */
	writel(0x00012318, 0x5b8000c8);		/* FRCWEMBADDR_ch0 */
	writel(0x04060106, 0x5b800248);		/* FRCEMBNUM */
	writel(0x0001231c, 0x5b8000cc);		/* TDFWEMBADDR_ch0 */
	writel(0x05050105, 0x5b80024c);		/* TDFEMBNUM */
	writel(0x00100000, 0x5b80c07c);		/* CLKEN_SSIF_WC_REG */
	writel(0x00000180, 0x5b80c080);		/* CLKEN_SSIF_RC_REG */
	writel(0x00000001, 0x5b800700);		/* CPUSRST */
	writel(0x00000001, 0x5b80070c);		/* IDSSRST */
	writel(0x00000001, 0x5b800714);		/* IXASRST */
	writel(0x00000001, 0x5b800718);		/* HDMSRST */
	writel(0x00000001, 0x5b80071c);		/* MDMSRST */
	writel(0x00000001, 0x5b800720);		/* HDDSRST */
	writel(0x00000001, 0x5b800724);		/* MDDSRST */
	writel(0x00000001, 0x5b800728);		/* SIOSRST */
	writel(0x00000001, 0x5b800738);		/* FLPSRST */
	writel(0x00000001, 0x5b80073c);		/* VBMSRST */
	writel(0x00000001, 0x5b800744);		/* IPRSRST */
	writel(0x00000001, 0x5b800748);		/* FRCSRST */
	writel(0x00000001, 0x5b80074c);		/* TDFSRST */
	writel(0x00000001, 0x5b800750);		/* RGLSRST */
	writel(0x00000001, 0x5b800764);		/* A2DSRST */
	writel(0x00000001, 0x5b80076c);		/* IP2SRST */
	writel(0x00000001, 0x5b800770);		/* DMDSRST */
	writel(0x00000001, 0x5b800774);		/* OVDSRST */
}

#if CONFIG_SDRAM0_SIZE == 0x20000000 && CONFIG_DDR_NUM_CH0 == 2 \
	&& CONFIG_SDRAM1_SIZE == 0x20000000 && CONFIG_DDR_NUM_CH1 == 1 \
	&& CONFIG_SDRAM2_SIZE == 0x10000000 && CONFIG_DDR_NUM_CH2 == 1 \
	&& CONFIG_DDR_FREQ == 1333

#include "umc_2Gx2+4Gx1+2Gx1_1333.c"

#elif CONFIG_SDRAM0_SIZE == 0x20000000 && CONFIG_DDR_NUM_CH0 == 2 \
	&& CONFIG_SDRAM1_SIZE == 0x20000000 && CONFIG_DDR_NUM_CH1 == 1 \
	&& CONFIG_SDRAM2_SIZE == 0x10000000 && CONFIG_DDR_NUM_CH2 == 1 \
	&& CONFIG_DDR_FREQ == 1600

#include "umc_2Gx2+4Gx1+2Gx1_1600.c"

#else

#error Unsupported DDR configuration.

#endif

void ddr_init(void);

int uniphier_sld3_umc_init(void)
{
	umc_init_sub();

	ddr_init();

	return 0;
}
