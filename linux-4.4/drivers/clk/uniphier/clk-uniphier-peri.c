/*
 * Copyright (C) 2015 Masahiro Yamada <yamada.masahiro@socionext.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/clk-provider.h>

#include "clk-uniphier.h"

#define UNIPHIER_PERI_CLK_UART(ch, index)			\
	{							\
		.name = "uart" #ch "-clken",			\
		.type = UNIPHIER_CLK_TYPE_GATE,			\
		.output_index = -1,				\
		.data.gate = {					\
			.parent_name = "uart",			\
			.reg = 0x24,				\
			.bit_idx = 19 + ch,			\
		},						\
	},							\
	{							\
		.name = "uart" #ch,				\
		.type = UNIPHIER_CLK_TYPE_GATE,			\
		.output_index = (index),			\
		.data.gate = {					\
			.parent_name = "uart" #ch "-clken",	\
			.reg = 0x114,				\
			.bit_idx = 19 + ch,			\
		},						\
	}

#define UNIPHIER_PERI_CLK_I2C_COMMON				\
	{							\
		.name = "i2c-clken",				\
		.type = UNIPHIER_CLK_TYPE_GATE,			\
		.output_index = -1,				\
		.data.gate = {					\
			.parent_name = "i2c",			\
			.reg = 0x20,				\
			.bit_idx = 1,				\
		},						\
	},							\
	{							\
		.name = "i2c-reset",				\
		.type = UNIPHIER_CLK_TYPE_GATE,			\
		.output_index = -1,				\
		.data.gate = {					\
			.parent_name = "i2c-clken",		\
			.reg = 0x110,				\
			.bit_idx = 1,				\
		},						\
	}


#define UNIPHIER_PERI_CLK_I2C(ch, index)			\
	{							\
		.name = "i2c" #ch "-clken",			\
		.type = UNIPHIER_CLK_TYPE_GATE,			\
		.output_index = -1,				\
		.data.gate = {					\
			.parent_name = "i2c-reset",		\
			.reg = 0x24,				\
			.bit_idx = 5 + ch,			\
		},						\
	},							\
	{							\
		.name = "i2c" #ch,				\
		.type = UNIPHIER_CLK_TYPE_GATE,			\
		.output_index = (index),			\
		.data.gate = {					\
			.parent_name = "i2c" #ch "-clken",	\
			.reg = 0x114,				\
			.bit_idx = 5 + ch,			\
		},						\
	}

#define UNIPHIER_PERI_CLK_FI2C(ch, index)			\
	{							\
		.name = "fi2c" #ch "-clken",			\
		.type = UNIPHIER_CLK_TYPE_GATE,			\
		.output_index = -1,				\
		.data.gate = {					\
			.parent_name = "fi2c",			\
			.reg = 0x24,				\
			.bit_idx = 24 + ch,			\
		},						\
	},							\
	{							\
		.name = "fi2c" #ch,				\
		.type = UNIPHIER_CLK_TYPE_GATE,			\
		.output_index = index,				\
		.data.gate = {					\
			.parent_name = "fi2c" #ch "-clken",	\
			.reg = 0x114,				\
			.bit_idx = 24 + ch,			\
		},						\
	}

static struct uniphier_clk_init_data ph1_ld4_peri_clk_idata[] __initdata = {
	UNIPHIER_PERI_CLK_UART(0, 0),
	UNIPHIER_PERI_CLK_UART(1, 1),
	UNIPHIER_PERI_CLK_UART(2, 2),
	UNIPHIER_PERI_CLK_UART(3, 3),
	UNIPHIER_PERI_CLK_I2C_COMMON,
	UNIPHIER_PERI_CLK_I2C(0, 4),
	UNIPHIER_PERI_CLK_I2C(1, 5),
	UNIPHIER_PERI_CLK_I2C(2, 6),
	UNIPHIER_PERI_CLK_I2C(3, 7),
	UNIPHIER_PERI_CLK_I2C(4, 8),
	{ /* sentinel */ }
};

static struct uniphier_clk_init_data ph1_pro4_peri_clk_idata[] __initdata = {
	UNIPHIER_PERI_CLK_UART(0, 0),
	UNIPHIER_PERI_CLK_UART(1, 1),
	UNIPHIER_PERI_CLK_UART(2, 2),
	UNIPHIER_PERI_CLK_UART(3, 3),
	UNIPHIER_PERI_CLK_FI2C(0, 4),
	UNIPHIER_PERI_CLK_FI2C(1, 5),
	UNIPHIER_PERI_CLK_FI2C(2, 6),
	UNIPHIER_PERI_CLK_FI2C(3, 7),
	/* no I2C ch4 */
	UNIPHIER_PERI_CLK_FI2C(5, 9),
	UNIPHIER_PERI_CLK_FI2C(6, 10),
	{ /* sentinel */ }
};

static struct uniphier_clk_init_data ph1_pro5_peri_clk_idata[] __initdata = {
	UNIPHIER_PERI_CLK_UART(0, 0),
	UNIPHIER_PERI_CLK_UART(1, 1),
	UNIPHIER_PERI_CLK_UART(2, 2),
	UNIPHIER_PERI_CLK_UART(3, 3),
	UNIPHIER_PERI_CLK_FI2C(0, 4),
	UNIPHIER_PERI_CLK_FI2C(1, 5),
	UNIPHIER_PERI_CLK_FI2C(2, 6),
	UNIPHIER_PERI_CLK_FI2C(3, 7),
	UNIPHIER_PERI_CLK_FI2C(4, 8),
	UNIPHIER_PERI_CLK_FI2C(5, 9),
	UNIPHIER_PERI_CLK_FI2C(6, 10),
	{ /* sentinel */ }
};

static void __init ph1_ld4_peri_clk_init(struct device_node *np)
{
	uniphier_clk_init(np, ph1_ld4_peri_clk_idata);
}
CLK_OF_DECLARE(ph1_ld4_peri_clk, "socionext,ph1-ld4-perictrl",
	       ph1_ld4_peri_clk_init);
CLK_OF_DECLARE(ph1_sld8_peri_clk, "socionext,ph1-sld8-perictrl",
	       ph1_ld4_peri_clk_init);

static void __init ph1_pro4_peri_clk_init(struct device_node *np)
{
	uniphier_clk_init(np, ph1_pro4_peri_clk_idata);
}
CLK_OF_DECLARE(ph1_pro4_peri_clk, "socionext,ph1-pro4-perictrl",
	       ph1_pro4_peri_clk_init);

static void __init ph1_pro5_peri_clk_init(struct device_node *np)
{
	uniphier_clk_init(np, ph1_pro5_peri_clk_idata);
}
CLK_OF_DECLARE(ph1_pro5_peri_clk, "socionext,ph1-pro5-perictrl",
	       ph1_pro5_peri_clk_init);
CLK_OF_DECLARE(proxstream2_peri_clk, "socionext,proxstream2-perictrl",
	       ph1_pro5_peri_clk_init);
CLK_OF_DECLARE(ph1_ld20_peri_clk, "socionext,ph1-ld20-perictrl",
	       ph1_pro5_peri_clk_init);
