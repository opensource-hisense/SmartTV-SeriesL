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

#define pr_fmt(fmt)		"uniphier-clk: " fmt

#include <linux/clk-provider.h>
#include <linux/log2.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>

#include "clk-uniphier.h"

static struct clk * __init uniphier_clk_register(struct device_node *np,
						 void __iomem *regbase,
					  struct uniphier_clk_init_data *idata)
{
	switch (idata->type) {
	case UNIPHIER_CLK_TYPE_FIXED_FACTOR:
		return clk_register_fixed_factor(NULL, idata->name,
						 idata->data.factor.parent_name,
						 CLK_SET_RATE_PARENT,
						 idata->data.factor.mult,
						 idata->data.factor.div);
	case UNIPHIER_CLK_TYPE_FIXED_RATE:
		return clk_register_fixed_rate(NULL, idata->name, NULL,
					       CLK_IS_ROOT,
					       idata->data.rate.fixed_rate);
	case UNIPHIER_CLK_TYPE_GATE:
		return clk_register_gate(NULL, idata->name,
					 idata->data.gate.parent_name,
					 idata->data.gate.parent_name ?
					 CLK_SET_RATE_PARENT : CLK_IS_ROOT,
					 regbase + idata->data.gate.reg,
					 idata->data.gate.bit_idx,
					 idata->data.gate.flags, NULL);
	case UNIPHIER_CLK_TYPE_MUX:
		return clk_register_mux(NULL, idata->name,
					idata->data.mux.parent_names,
					idata->data.mux.num_parents,
					CLK_SET_RATE_PARENT,
					regbase + idata->data.mux.reg,
					idata->data.mux.shift,
					ilog2(idata->data.mux.num_parents),
					idata->data.mux.flags, NULL);
	default:
		WARN(1, "unsupported clock type\n");
		return ERR_PTR(-EINVAL);
	}
}

int __init uniphier_clk_init(struct device_node *np,
			     struct uniphier_clk_init_data *idata)
{
	struct clk_onecell_data *clk_data;
	struct uniphier_clk_init_data *p;
	void __iomem *regbase;
	int max_index = 0;
	int ret;

	regbase = of_iomap(np, 0);
	if (!regbase)
		return -ENOMEM;

	for (p = idata; p->name; p++)
		max_index = max(max_index, p->output_index);

	clk_data = kzalloc(sizeof(*clk_data), GFP_KERNEL);
	if (!clk_data)
		return -ENOMEM;

	clk_data->clk_num = max_index + 1;
	clk_data->clks = kcalloc(clk_data->clk_num, sizeof(struct clk *),
				 GFP_KERNEL);
	if (!clk_data->clks) {
		ret = -ENOMEM;
		goto free_clk_data;
	}

	for (p = idata; p->name; p++) {
		pr_debug("register %s (%s[%d])\n", p->name, np->name,
			 p->output_index);
		p->clk = uniphier_clk_register(np, regbase, p);
		if (IS_ERR(p->clk)) {
			pr_err("failed to register %s\n", p->name);
			ret = PTR_ERR(p->clk);
			goto unregister;
		}

		if (p->output_index >= 0)
			clk_data->clks[p->output_index] = p->clk;
	}

	ret = of_clk_add_provider(np, of_clk_src_onecell_get, clk_data);
	if (ret)
		goto unregister;

	return ret;
unregister:
	for (p--; p >= idata; p--) {
		pr_debug("unregister %s (%s[%d])\n", p->name, np->name,
			 p->output_index);
		clk_unregister(p->clk);
		p->clk = NULL;
	}
	kfree(clk_data->clks);
free_clk_data:
	kfree(clk_data);

	pr_err("%s: init failed with error %d\n", np->full_name, ret);

	return ret;
}
