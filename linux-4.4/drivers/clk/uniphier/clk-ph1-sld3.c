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

static struct uniphier_clk_init_data ph1_sld3_clk_idata[] __initdata = {
	{
		.name = "spll",
		.type = UNIPHIER_CLK_TYPE_FIXED_FACTOR,
		.output_index = -1,
		.data.factor = {
			.parent_name = "ref",
			.mult = 65,
			.div = 1,
		},
	},
	{
		.name = "upll",
		.type = UNIPHIER_CLK_TYPE_FIXED_FACTOR,
		.output_index = -1,
		.data.factor = {
			.parent_name = "ref",
			.mult = 288000,
			.div = 24576,
		},
	},
	{
		.name = "a2pll",
		.type = UNIPHIER_CLK_TYPE_FIXED_FACTOR,
		.output_index = -1,
		.data.factor = {
			.parent_name = "ref",
			.mult = 24,
			.div = 1,
		},
	},
	{
		.name = "uart",
		.type = UNIPHIER_CLK_TYPE_FIXED_FACTOR,
		.output_index = 3,
		.data.factor = {
			.parent_name = "a2pll",
			.mult = 1,
			.div = 16,
		},
	},
	{
		.name = "i2c",
		.type = UNIPHIER_CLK_TYPE_FIXED_FACTOR,
		.output_index = 4,
		.data.factor = {
			.parent_name = "spll",
			.mult = 1,
			.div = 16,
		},
	},
	{
		.name = "arm-scu",
		.type = UNIPHIER_CLK_TYPE_FIXED_FACTOR,
		.output_index = 7,
		.data.factor = {
			.parent_name = "spll",
			.mult = 1,
			.div = 32,
		},
	},
	{
		.name = "stdmac-clken",
		.type = UNIPHIER_CLK_TYPE_GATE,
		.output_index = -1,
		.data.gate = {
			.parent_name = "ref",
			.reg = 0x2104,
			.bit_idx = 10,
		},
	},
	{
		.name = "stdmac",
		.type = UNIPHIER_CLK_TYPE_GATE,
		.output_index = 10,
		.data.gate = {
			.parent_name = "stdmac-clken",
			.reg = 0x2000,
			.bit_idx = 10,
		},
	},
	{
		.name = "ehci",
		.type = UNIPHIER_CLK_TYPE_FIXED_FACTOR,
		.output_index = 18,
		.data.factor = {
			.parent_name = "upll",
			.mult = 1,
			.div = 12,
		},
	},
	{ /* sentinel */ }
};

static void __init ph1_sld3_clk_init(struct device_node *np)
{
	uniphier_clk_init(np, ph1_sld3_clk_idata);
}
CLK_OF_DECLARE(ph1_sld3_clk, "socionext,ph1-sld3-sysctrl", ph1_sld3_clk_init);
