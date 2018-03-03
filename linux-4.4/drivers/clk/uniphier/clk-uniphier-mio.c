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

#define UNIPHIER_MIO_CLK_SD_FIXED					\
	{								\
		.name = "sd-44m",					\
		.type = UNIPHIER_CLK_TYPE_FIXED_RATE,			\
		.output_index = -1,					\
		.data.rate = {						\
			.fixed_rate = 44444444,				\
		},							\
	},								\
	{								\
		.name = "sd-33m",					\
		.type = UNIPHIER_CLK_TYPE_FIXED_RATE,			\
		.output_index = -1,					\
		.data.rate = {						\
			.fixed_rate = 33333333,				\
		},							\
	},								\
	{								\
		.name = "sd-50m",					\
		.type = UNIPHIER_CLK_TYPE_FIXED_RATE,			\
		.output_index = -1,					\
		.data.rate = {						\
			.fixed_rate = 50000000,				\
		},							\
	},								\
	{								\
		.name = "sd-67m",					\
		.type = UNIPHIER_CLK_TYPE_FIXED_RATE,			\
		.output_index = -1,					\
		.data.rate = {						\
			.fixed_rate = 66666666,				\
		},							\
	},								\
	{								\
		.name = "sd-100m",					\
		.type = UNIPHIER_CLK_TYPE_FIXED_RATE,			\
		.output_index = -1,					\
		.data.rate = {						\
			.fixed_rate = 100000000,			\
		},							\
	},								\
	{								\
		.name = "sd-40m",					\
		.type = UNIPHIER_CLK_TYPE_FIXED_RATE,			\
		.output_index = -1,					\
		.data.rate = {						\
			.fixed_rate = 40000000,				\
		},							\
	},								\
	{								\
		.name = "sd-25m",					\
		.type = UNIPHIER_CLK_TYPE_FIXED_RATE,			\
		.output_index = -1,					\
		.data.rate = {						\
			.fixed_rate = 25000000,				\
		},							\
	},								\
	{								\
		.name = "sd-22m",					\
		.type = UNIPHIER_CLK_TYPE_FIXED_RATE,			\
		.output_index = -1,					\
		.data.rate = {						\
			.fixed_rate = 22222222,				\
		},							\
	}


#define UNIPHIER_MIO_CLK_SD(ch, index)					\
	{								\
		.name = "sd" #ch "-clksel0",				\
		.type = UNIPHIER_CLK_TYPE_MUX,				\
		.output_index = -1,					\
		.data.mux = {						\
			.parent_names = {				\
				"sd-44m",				\
				"sd-33m",				\
				"sd-50m",				\
				"sd-67m",				\
			},						\
			.num_parents = 4,				\
			.reg = 0x30 + 0x200 * ch,			\
			.shift = 16,					\
		},							\
	},								\
	{								\
		.name = "sd" #ch "-clksel1",				\
		.type = UNIPHIER_CLK_TYPE_MUX,				\
		.output_index = -1,					\
		.data.mux = {						\
			.parent_names = {				\
				"sd-100m",				\
				"sd-40m",				\
				"sd-25m",				\
				"sd-22m",				\
			},						\
			.num_parents = 4,				\
			.reg = 0x30 + 0x200 * ch,			\
			.shift = 8,					\
		},							\
	},								\
	{								\
		.name = "sd" #ch "-clkmode",				\
		.type = UNIPHIER_CLK_TYPE_MUX,				\
		.output_index = -1,					\
		.data.mux = {						\
			.parent_names = {				\
				"sd" #ch "-clksel0",			\
				"sd" #ch "-clksel1",			\
			},						\
			.num_parents = 2,				\
			.reg = 0x30 + 0x200 * ch,			\
			.shift = 12,					\
		},							\
	},								\
	{								\
		.name = "sd" #ch "-clken",				\
		.type = UNIPHIER_CLK_TYPE_GATE,				\
		.output_index = -1,					\
		.data.gate = {						\
			.parent_name = "sd" #ch "-clkmode",		\
			.reg = 0x20 + 0x200 * ch,			\
			.bit_idx = 8,					\
		},							\
	},								\
	{								\
		.name = "sd" #ch "-bridge-reset",			\
		.type = UNIPHIER_CLK_TYPE_GATE,				\
		.output_index = -1,					\
		.data.gate = {						\
			.parent_name = "sd" #ch "-clken",		\
			.reg = 0x110 + 0x200 * ch,			\
			.bit_idx = 26,					\
		},							\
	},								\
	{								\
		.name = "sd" #ch,					\
		.type = UNIPHIER_CLK_TYPE_GATE,				\
		.output_index = (index),				\
		.data.gate = {						\
			.parent_name = "sd" #ch "-bridge-reset",	\
			.reg = 0x110 + 0x200 * ch,			\
			.bit_idx = 0,					\
		},							\
	}

#define UNIPHIER_MIO_CLK_SD_HW_RESET(ch, index)				\
	{								\
		.name = "sd" #ch "-hw-reset",				\
		.type = UNIPHIER_CLK_TYPE_GATE,				\
		.output_index = (index),				\
		.data.gate = {						\
			.parent_name = NULL,				\
			.reg = 0x80 + 0x200 * ch,			\
			.bit_idx = 0,					\
		},							\
	}

#define UNIPHIER_MIO_CLK_EHCI(ch, index)				\
	{								\
		.name = "ehci" #ch "-phy-clken",			\
		.type = UNIPHIER_CLK_TYPE_GATE,				\
		.output_index = -1,					\
		.data.gate = {						\
			.parent_name = "ehci",				\
			.reg = 0x20 + 0x200 * ch,			\
			.bit_idx = 29,					\
		},							\
	},								\
	{								\
		.name = "ehci" #ch "-link-clken",			\
		.type = UNIPHIER_CLK_TYPE_GATE,				\
		.output_index = -1,					\
		.data.gate = {						\
			.parent_name = "ehci" #ch "-phy-clken",		\
			.reg = 0x20 + 0x200 * ch,			\
			.bit_idx = 28,					\
		},							\
	},								\
	{								\
		.name = "ehci" #ch "-bridge-reset",			\
		.type = UNIPHIER_CLK_TYPE_GATE,				\
		.output_index = -1,					\
		.data.gate = {						\
			.parent_name = "ehci" #ch "-link-clken",	\
			.reg = 0x110 + 0x200 * ch,			\
			.bit_idx = 24,					\
		},							\
	},								\
	{								\
		.name = "ehci" #ch,					\
		.type = UNIPHIER_CLK_TYPE_GATE,				\
		.output_index = (index),				\
		.data.gate = {						\
			.parent_name = "ehci" #ch "-bridge-reset",	\
			.reg = 0x114 + 0x200 * ch,			\
			.bit_idx = 0,					\
		},							\
	}

#define UNIPHIER_MIO_CLK_DMAC(index)					\
	{								\
		.name = "miodmac-clken",				\
		.type = UNIPHIER_CLK_TYPE_GATE,				\
		.output_index = -1,					\
		.data.gate = {						\
			.parent_name = "stdmac",			\
			.reg = 0x20,					\
			.bit_idx = 25,					\
		},							\
	},								\
	{								\
		.name = "miodmac",					\
		.type = UNIPHIER_CLK_TYPE_GATE,				\
		.output_index = (index),				\
		.data.gate = {						\
			.parent_name = "miodmac-clken",			\
			.reg = 0x110,					\
			.bit_idx = 17,					\
		},							\
	}

static struct uniphier_clk_init_data ph1_sld3_mio_clk_idata[] __initdata = {
	UNIPHIER_MIO_CLK_SD_FIXED,
	UNIPHIER_MIO_CLK_SD(0, 0),
	UNIPHIER_MIO_CLK_SD(1, 1),
	UNIPHIER_MIO_CLK_SD_HW_RESET(0, 3),
	UNIPHIER_MIO_CLK_EHCI(0, 4),
	UNIPHIER_MIO_CLK_EHCI(1, 5),
	UNIPHIER_MIO_CLK_EHCI(2, 6),
	UNIPHIER_MIO_CLK_EHCI(3, 7),
	UNIPHIER_MIO_CLK_DMAC(8),
	{ /* sentinel */ }
};

static struct uniphier_clk_init_data ph1_ld4_mio_clk_idata[] __initdata = {
	UNIPHIER_MIO_CLK_SD_FIXED,
	UNIPHIER_MIO_CLK_SD(0, 0),
	UNIPHIER_MIO_CLK_SD(1, 1),
	UNIPHIER_MIO_CLK_SD_HW_RESET(1, 3),
	UNIPHIER_MIO_CLK_EHCI(0, 4),
	UNIPHIER_MIO_CLK_EHCI(1, 5),
	UNIPHIER_MIO_CLK_EHCI(2, 6),
	UNIPHIER_MIO_CLK_DMAC(8),
	{ /* sentinel */ }
};

static struct uniphier_clk_init_data ph1_pro4_mio_clk_idata[] __initdata = {
	UNIPHIER_MIO_CLK_SD_FIXED,
	UNIPHIER_MIO_CLK_SD(0, 0),
	UNIPHIER_MIO_CLK_SD(1, 1),
	UNIPHIER_MIO_CLK_SD(2, 2),
	UNIPHIER_MIO_CLK_SD_HW_RESET(1, 3),
	UNIPHIER_MIO_CLK_EHCI(0, 4),
	UNIPHIER_MIO_CLK_EHCI(1, 5),
	UNIPHIER_MIO_CLK_DMAC(8),
	{ /* sentinel */ }
};

static struct uniphier_clk_init_data ph1_pro5_mio_clk_idata[] __initdata = {
	UNIPHIER_MIO_CLK_SD_FIXED,
	UNIPHIER_MIO_CLK_SD(0, 0),
	UNIPHIER_MIO_CLK_SD(1, 1),
	UNIPHIER_MIO_CLK_SD_HW_RESET(1, 3),
	{ /* sentinel */ }
};

static struct uniphier_clk_init_data ph1_ld11_mio_clk_idata[] __initdata = {
	UNIPHIER_MIO_CLK_EHCI(0, 4),
	UNIPHIER_MIO_CLK_EHCI(1, 5),
	UNIPHIER_MIO_CLK_EHCI(2, 6),
	UNIPHIER_MIO_CLK_DMAC(8),
	{ /* sentinel */ }
};

static void __init ph1_sld3_mio_clk_init(struct device_node *np)
{
	uniphier_clk_init(np, ph1_sld3_mio_clk_idata);
}
CLK_OF_DECLARE(ph1_sld3_mio_clk, "socionext,ph1-sld3-mioctrl",
	       ph1_sld3_mio_clk_init);

static void __init ph1_ld4_mio_clk_init(struct device_node *np)
{
	uniphier_clk_init(np, ph1_ld4_mio_clk_idata);
}
CLK_OF_DECLARE(ph1_ld4_mio_clk, "socionext,ph1-ld4-mioctrl",
	       ph1_ld4_mio_clk_init);
CLK_OF_DECLARE(ph1_sld8_mio_clk, "socionext,ph1-sld8-mioctrl",
	       ph1_ld4_mio_clk_init);

static void __init ph1_pro4_mio_clk_init(struct device_node *np)
{
	uniphier_clk_init(np, ph1_pro4_mio_clk_idata);
}
CLK_OF_DECLARE(ph1_pro4_mio_clk, "socionext,ph1-pro4-mioctrl",
	       ph1_pro4_mio_clk_init);

static void __init ph1_pro5_mio_clk_init(struct device_node *np)
{
	uniphier_clk_init(np, ph1_pro5_mio_clk_idata);
}
CLK_OF_DECLARE(ph1_pro5_mio_clk, "socionext,ph1-pro5-mioctrl",
	       ph1_pro5_mio_clk_init);
CLK_OF_DECLARE(proxstream2_mio_clk, "socionext,proxstream2-mioctrl",
	       ph1_pro5_mio_clk_init);
CLK_OF_DECLARE(ph1_ld20_mio_clk, "socionext,ph1-ld20-mioctrl",
	       ph1_pro5_mio_clk_init);

static void __init ph1_ld11_mio_clk_init(struct device_node *np)
{
	uniphier_clk_init(np, ph1_ld11_mio_clk_idata);
}
CLK_OF_DECLARE(ph1_ld11_mio_clk, "socionext,ph1-ld11-mioctrl",
	       ph1_ld11_mio_clk_init);
