/*
 * Copyright (C) 2016 Masahiro Yamada <yamada.masahiro@socionext.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <linux/io.h>

#include "../init.h"
#include "../sg-regs.h"

void uniphier_ld20_pin_init(void)
{
	u32 tmp;

	writel((u32)0x00000000, 0x5F801000);	/* PINCTRL00 */
	writel((u32)0x00000000, 0x5F801004);	/* PINCTRL01 */
	writel((u32)0x00000000, 0x5F801008);	/* PINCTRL02 */
	writel((u32)0x00000000, 0x5F80100C);	/* PINCTRL03 */
	writel((u32)0x00000000, 0x5F801010);	/* PINCTRL04 */
	writel((u32)0x00000000, 0x5F801014);	/* PINCTRL05 */
	writel((u32)0x00000000, 0x5F801018);	/* PINCTRL06 */
	writel((u32)0x00000000, 0x5F80101C);	/* PINCTRL07 */
	writel((u32)0x00000000, 0x5F801020);	/* PINCTRL08 */
	writel((u32)0x00000000, 0x5F801024);	/* PINCTRL09 */
	writel((u32)0x00000000, 0x5F801028);	/* PINCTRL10 */
	writel((u32)0x00000000, 0x5F80102C);	/* PINCTRL11 */
	writel((u32)0x0F0F0000, 0x5F801030);	/* PINCTRL12 */
	writel((u32)0x00000000, 0x5F801034);	/* PINCTRL13 */
	writel((u32)0x01010F0F, 0x5F801038);	/* PINCTRL14 */
	writel((u32)0x00000000, 0x5F80103C);	/* PINCTRL15 */
	writel((u32)0x00000000, 0x5F801040);	/* PINCTRL16 */
	writel((u32)0x02020200, 0x5F801044);	/* PINCTRL17 */
	writel((u32)0x0F0F0202, 0x5F801048);	/* PINCTRL18 */
	writel((u32)0x0F0F0F0F, 0x5F80104C);	/* PINCTRL19 */
	writel((u32)0x0F0F0F0F, 0x5F801050);	/* PINCTRL20 */
	writel((u32)0x0F0F0F0F, 0x5F801054);	/* PINCTRL21 */
	writel((u32)0x0F0F0F0F, 0x5F801058);	/* PINCTRL22 */
	writel((u32)0x0F0F0F0F, 0x5F80105C);	/* PINCTRL23 */
	writel((u32)0x0F0F0F0F, 0x5F801060);	/* PINCTRL24 */
	writel((u32)0x0F0F0F0F, 0x5F801064);	/* PINCTRL25 */
	writel((u32)0x0F0F0F0F, 0x5F801068);	/* PINCTRL26 */
	writel((u32)0x0F0F0F0F, 0x5F80106C);	/* PINCTRL27 */
	writel((u32)0x0F0F0F0F, 0x5F801070);	/* PINCTRL28 */
	writel((u32)0x03030303, 0x5F801074);	/* PINCTRL29 */
	writel((u32)0x03030303, 0x5F801078);	/* PINCTRL30 */
	writel((u32)0x03030303, 0x5F80107C);	/* PINCTRL31 */
	writel((u32)0x0F0F0F0F, 0x5F801080);	/* PINCTRL32 */
	writel((u32)0x000F0F0F, 0x5F801084);	/* PINCTRL33 */
	writel((u32)0x00000000, 0x5F801088);	/* PINCTRL34 */
	writel((u32)0x0F0F0F00, 0x5F80108C);	/* PINCTRL35 */
	writel((u32)0x000E0F0E, 0x5F801090);	/* PINCTRL36 */
	writel((u32)0x0F0F0F0F, 0x5F801094);	/* PINCTRL37 */
	writel((u32)0x0F0E0E0F, 0x5F801098);	/* PINCTRL38 */
	writel((u32)0x0F000F0F, 0x5F80109C);	/* PINCTRL39 */
	writel((u32)0x0000000F, 0x5F8010A0);	/* PINCTRL40 */
	writel((u32)0x00000000, 0x5F8010A4);	/* PINCTRL41 */

	writel((u32)0x3FC3FC79, 0x5F801A00);	/* PUPDCTRL0 */
	writel((u32)0x0CC00000, 0x5F801A04);	/* PUPDCTRL1 */
	writel((u32)0x00000000, 0x5F801A08);	/* PUPDCTRL2 */
	writel((u32)0x0FF00000, 0x5F801A0C);	/* PUPDCTRL3 */
	writel((u32)0x02000000, 0x5F801A10);	/* PUPDCTRL4 */
	writel((u32)0x00000000, 0x5F801A14);	/* PUPDCTRL5 */

	writel((u32)0x00000003, 0x5F801C04);	/* OECTRL */

	writel((u32)0x3FFFFC39, 0x5F801D00);	/* IECTRL0 */
	writel((u32)0x1CC3C000, 0x5F801D04);	/* IECTRL1 */
	writel((u32)0x000007E0, 0x5F801D08);	/* IECTRL2 */
	writel((u32)0xFFF00000, 0x5F801D0C);	/* IECTRL3 */
	writel((u32)0x7FFFFF80, 0x5F801D10);	/* IECTRL4 */
	writel((u32)0x00000000, 0x5F801D14);	/* IECTRL5 */

	writel((u32)0x00000000, 0x55000030);	/* P5IOD */
	writel((u32)0x0000007F, 0x55000034);	/* P5DIR */
	writel((u32)0x00000000, 0x550000C8);	/* P22IOD */
	writel((u32)0x000000F3, 0x550000CC);	/* P22DIR */
	writel((u32)0x00000000, 0x550000D0);	/* P23IOD */
	writel((u32)0x000000C5, 0x550000D4);	/* P23DIR */
	writel((u32)0x00000028, 0x550000D8);	/* P24IOD */
	writel((u32)0x000000D4, 0x550000DC);	/* P24DIR */

	writel((u32)0x00002431, 0x55000090);	/* IRQCTRL */
	writel((u32)0x00000000, 0x55000094);	/* IRQSEL */
	tmp = readl(0x5fc20008);
	tmp |= 0x11 << 16;
	writel(tmp, 0x5fc20008);	/* DETCONFR2(XIRQ4: Negative) */
}
