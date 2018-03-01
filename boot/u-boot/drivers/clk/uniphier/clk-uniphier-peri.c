/*
 * Copyright (C) 2016 Masahiro Yamada <yamada.masahiro@socionext.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <clk.h>
#include <dm/device.h>

#include "clk-uniphier.h"

#if defined(CONFIG_CLK_UNIPHIER_LD4)
static struct uniphier_clk_rate_data ph1_ld4_peri_clk_rate[] = {
	UNIPHIER_CLK_FIXED_RATE(0, 36864000),
	UNIPHIER_CLK_FIXED_RATE(1, 36864000),
	UNIPHIER_CLK_FIXED_RATE(2, 36864000),
	UNIPHIER_CLK_FIXED_RATE(3, 36864000),
	UNIPHIER_CLK_FIXED_RATE(4, 99840000),
	UNIPHIER_CLK_FIXED_RATE(5, 99840000),
	UNIPHIER_CLK_FIXED_RATE(6, 99840000),
	UNIPHIER_CLK_FIXED_RATE(7, 99840000),
	UNIPHIER_CLK_FIXED_RATE(8, 99840000),
};

static struct uniphier_clk_soc_data ph1_ld4_peri_clk_data = {
	.rate = ph1_ld4_peri_clk_rate,
	.nr_rate = ARRAY_SIZE(ph1_ld4_peri_clk_rate),
};
#endif

#if defined(CONFIG_CLK_UNIPHIER_PRO4) || defined(CONFIG_CLK_UNIPHIER_PRO5)
static struct uniphier_clk_rate_data ph1_pro4_peri_clk_rate[] = {
	UNIPHIER_CLK_FIXED_RATE(0, 73728000),
	UNIPHIER_CLK_FIXED_RATE(1, 73728000),
	UNIPHIER_CLK_FIXED_RATE(2, 73728000),
	UNIPHIER_CLK_FIXED_RATE(3, 73728000),
	UNIPHIER_CLK_FIXED_RATE(4, 50000000),
	UNIPHIER_CLK_FIXED_RATE(5, 50000000),
	UNIPHIER_CLK_FIXED_RATE(6, 50000000),
	UNIPHIER_CLK_FIXED_RATE(7, 50000000),
	UNIPHIER_CLK_FIXED_RATE(8, 50000000),	/* no I2C ch4 for PH1-Pro4 */
	UNIPHIER_CLK_FIXED_RATE(9, 50000000),
	UNIPHIER_CLK_FIXED_RATE(10, 50000000),
};

static struct uniphier_clk_soc_data ph1_pro4_peri_clk_data = {
	.rate = ph1_pro4_peri_clk_rate,
	.nr_rate = ARRAY_SIZE(ph1_pro4_peri_clk_rate),
};
#endif

#if defined(CONFIG_CLK_UNIPHIER_SLD8)
static struct uniphier_clk_rate_data ph1_sld8_peri_clk_rate[] = {
	UNIPHIER_CLK_FIXED_RATE(0, 80000000),
	UNIPHIER_CLK_FIXED_RATE(1, 80000000),
	UNIPHIER_CLK_FIXED_RATE(2, 80000000),
	UNIPHIER_CLK_FIXED_RATE(3, 80000000),
	UNIPHIER_CLK_FIXED_RATE(4, 100000000),
	UNIPHIER_CLK_FIXED_RATE(5, 100000000),
	UNIPHIER_CLK_FIXED_RATE(6, 100000000),
	UNIPHIER_CLK_FIXED_RATE(7, 100000000),
	UNIPHIER_CLK_FIXED_RATE(8, 100000000),
};

static struct uniphier_clk_soc_data ph1_sld8_peri_clk_data = {
	.rate = ph1_sld8_peri_clk_rate,
	.nr_rate = ARRAY_SIZE(ph1_sld8_peri_clk_rate),
};
#endif

#if defined(CONFIG_CLK_UNIPHIER_PXS2)
static struct uniphier_clk_rate_data proxstream2_peri_clk_rate[] = {
	UNIPHIER_CLK_FIXED_RATE(0, 88888888),
	UNIPHIER_CLK_FIXED_RATE(1, 88888888),
	UNIPHIER_CLK_FIXED_RATE(2, 88888888),
	UNIPHIER_CLK_FIXED_RATE(3, 88888888),
	UNIPHIER_CLK_FIXED_RATE(4, 50000000),
	UNIPHIER_CLK_FIXED_RATE(5, 50000000),
	UNIPHIER_CLK_FIXED_RATE(6, 50000000),
	UNIPHIER_CLK_FIXED_RATE(7, 50000000),
	UNIPHIER_CLK_FIXED_RATE(8, 50000000),
	UNIPHIER_CLK_FIXED_RATE(9, 50000000),
	UNIPHIER_CLK_FIXED_RATE(10, 50000000),
};

static struct uniphier_clk_soc_data proxstream2_peri_clk_data = {
	.rate = proxstream2_peri_clk_rate,
	.nr_rate = ARRAY_SIZE(proxstream2_peri_clk_rate),
};
#endif

static const struct udevice_id uniphier_peri_clk_match[] = {
#if defined(CONFIG_CLK_UNIPHIER_LD4)
	{
		.compatible = "socionext,ph1-ld4-perictrl",
		.data = (ulong)&ph1_ld4_peri_clk_data,
	},
#endif
#if defined(CONFIG_CLK_UNIPHIER_PRO4)
	{
		.compatible = "socionext,ph1-pro4-perictrl",
		.data = (ulong)&ph1_pro4_peri_clk_data,
	},
#endif
#if defined(CONFIG_CLK_UNIPHIER_SLD8)
	{
		.compatible = "socionext,ph1-sld8-perictrl",
		.data = (ulong)&ph1_sld8_peri_clk_data,
	},
#endif
#if defined(CONFIG_CLK_UNIPHIER_PRO5)
	{
		.compatible = "socionext,ph1-pro5-perictrl",
		.data = (ulong)&ph1_pro4_peri_clk_data,
	},
#endif
#if defined(CONFIG_CLK_UNIPHIER_PXS2)
	{
		.compatible = "socionext,proxstream2-perictrl",
		.data = (ulong)&proxstream2_peri_clk_data,
	},
#endif
	{ /* sentinel */ }
};

U_BOOT_DRIVER(uniphier_peri_clk) = {
	.name = "uniphier-peri-clk",
	.id = UCLASS_CLK,
	.of_match = uniphier_peri_clk_match,
	.probe = uniphier_clk_probe,
	.remove = uniphier_clk_remove,
	.priv_auto_alloc_size = sizeof(struct uniphier_clk_priv),
	.ops = &uniphier_clk_ops,
};
