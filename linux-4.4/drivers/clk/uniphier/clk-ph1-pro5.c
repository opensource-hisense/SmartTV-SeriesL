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

static struct uniphier_clk_init_data ph1_pro5_clk_idata[] __initdata = {
	{
		.name = "spll",
		.type = UNIPHIER_CLK_TYPE_FIXED_FACTOR,
		.output_index = -1,
		.data.factor = {
			.parent_name = "ref",
			.mult = 120,
			.div = 1,
		},
	},
	{
		.name = "dapll1",
		.type = UNIPHIER_CLK_TYPE_FIXED_FACTOR,
		.output_index = -1,
		.data.factor = {
			.parent_name = "ref",
			.mult = 128,
			.div = 125,
		},
	},
	{
		.name = "dapll2",
		.type = UNIPHIER_CLK_TYPE_FIXED_FACTOR,
		.output_index = -1,
		.data.factor = {
			.parent_name = "upll",
			.mult = 144,
			.div = 5,
		},
	},
	{
		.name = "uart",
		.type = UNIPHIER_CLK_TYPE_FIXED_FACTOR,
		.output_index = 3,
		.data.factor = {
			.parent_name = "dapll2",
			.mult = 1,
			.div = 8,
		},
	},
	{
		.name = "fi2c",
		.type = UNIPHIER_CLK_TYPE_FIXED_FACTOR,
		.output_index = 4,
		.data.factor = {
			.parent_name = "spll",
			.mult = 1,
			.div = 48,
		},
	},
	{
		.name = "arm-scu",
		.type = UNIPHIER_CLK_TYPE_FIXED_FACTOR,
		.output_index = 7,
		.data.factor = {
			.parent_name = "spll",
			.mult = 1,
			.div = 48,
		},
	},
	{
		.name = "stdmac-clken",
		.type = UNIPHIER_CLK_TYPE_GATE,
		.output_index = -1,
		.data.gate = {
			.parent_name = NULL,
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
		.name = "xhci0-reset",
		.type = UNIPHIER_CLK_TYPE_GATE,
		.output_index = -1,
		.data.gate = {
			.parent_name = NULL,
			.reg = 0x2000,
			.bit_idx = 17,
		},
	},
	{
		.name = "xhci1-reset",
		.type = UNIPHIER_CLK_TYPE_GATE,
		.output_index = -1,
		.data.gate = {
			.parent_name = NULL,
			.reg = 0x2004,
			.bit_idx = 17,
		},
	},
	{
		.name = "xhci0-usb3phy-clken",
		.type = UNIPHIER_CLK_TYPE_GATE,
		.output_index = 20,
		.data.gate = {
			.parent_name = "xhci0-reset",
			.reg = 0x2104,
			.bit_idx = 16,
		},
	},
	{
		.name = "xhci1-usb3phy-clken",
		.type = UNIPHIER_CLK_TYPE_GATE,
		.output_index = 21,
		.data.gate = {
			.parent_name = "xhci1-reset",
			.reg = 0x2104,
			.bit_idx = 17,
		},
	},
	{ /* sentinel */ }
};

static void __init ph1_pro5_clk_init(struct device_node *np)
{
	uniphier_clk_init(np, ph1_pro5_clk_idata);
}
CLK_OF_DECLARE(ph1_pro5_clk, "socionext,ph1-pro5-sysctrl", ph1_pro5_clk_init);
