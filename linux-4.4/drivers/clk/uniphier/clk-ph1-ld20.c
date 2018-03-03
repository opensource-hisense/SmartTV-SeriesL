/*
 * Copyright (C) 2016 Masahiro Yamada <yamada.masahiro@socionext.com>
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

static struct uniphier_clk_init_data ph1_ld20_clk_idata[] __initdata = {
	{
		.name = "spll",
		.type = UNIPHIER_CLK_TYPE_FIXED_FACTOR,
		.output_index = -1,
		.data.factor = {
			.parent_name = "ref",
			.mult = 80,
			.div = 1,
		},
	},
	{
		.name = "uart",
		.type = UNIPHIER_CLK_TYPE_FIXED_FACTOR,
		.output_index = 3,
		.data.factor = {
			.parent_name = "spll",
			.mult = 1,
			.div = 34,
		},
	},
	{
		.name = "fi2c",
		.type = UNIPHIER_CLK_TYPE_FIXED_FACTOR,
		.output_index = 4,
		.data.factor = {
			.parent_name = "spll",
			.mult = 1,
			.div = 40,
		},
	},
	{
		.name = "stdmac-clken",
		.type = UNIPHIER_CLK_TYPE_GATE,
		.output_index = -1,
		.data.gate = {
			.parent_name = NULL,
			.reg = 0x210c,
			.bit_idx = 8,
		},
	},
	{
		.name = "stdmac",
		.type = UNIPHIER_CLK_TYPE_GATE,
		.output_index = 10,
		.data.gate = {
			.parent_name = "stdmac-clken",
			.reg = 0x200c,
			.bit_idx = 8,
		},
	},
	{ /* sentinel */ }
};

static void __init ph1_ld20_clk_init(struct device_node *np)
{
	uniphier_clk_init(np, ph1_ld20_clk_idata);
}
CLK_OF_DECLARE(ph1_ld20_clk, "socionext,ph1-ld20-sysctrl", ph1_ld20_clk_init);
