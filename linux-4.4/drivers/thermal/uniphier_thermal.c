/**
 * uniphier_thermal.c - Socionext Uniphier thermal management driver
 *
 * Copyright (c) 2014 Panasonic Corporation
 * Copyright (c) 2016 Socionext Inc.
 * All rights reserved.
 *
 * Author:
 *	Kunihiko Hayashi <hayashi.kunihiko@socionext.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2  of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include <linux/thermal.h>
#include <linux/interrupt.h>

#include "thermal_core.h"

/* block */
#define PMPVTCTLEN_OFFSET			0x00000000
#define PMPVTCTLEN_PMPVTCTLEN			(0x1 << 0)
#define PMPVTCTLEN_PMPVTCTLEN_STOP		(0x0 << 0)
#define PMPVTCTLEN_PMPVTCTLEN_START		(0x1 << 0)

#define PMPVTCTLMODE_OFFSET			0x00000004
#define PMPVTCTLMODE_PMPVTCTLMODE		(0xf << 0)
#define PMPVTCTLMODE_PMPVTCTLMODE_TEMPMON	(0x5 << 0)

#define EMONREPEAT_OFFSET			0x00000040
#define EMONREPEAT_EMONENDLESS			(0x1 << 24)
#define EMONREPEAT_EMONENDLESS_ENABLE		(0x1 << 24)
#define EMONREPEAT_EMONPERIOD			(0xf << 0)
#define EMONREPEAT_EMONPERIOD_1000000		(0x9 << 0)

/* common */
#define PMPVTCTLMODESEL_OFFSET			0x00000900

#define SETALERT0_OFFSET			0x00000910
#define SETALERT1_OFFSET			0x00000914
#define SETALERT2_OFFSET			0x00000918
#define SETALERT_EALERTTEMP0_OF			(0xff << 16)
#define SETALERT_EALERTEN0			(0x1 << 0)
#define SETALERT_EALERTEN0_USE			(0x1 << 0)

#define PMALERTINTCTL_OFFSET			0x00000920
#define PMALERTINTCTL_ALERTINT_CLR(ch)		(0x1 << (((ch)<<4) + 2))
#define PMALERTINTCTL_ALERTINT_ST(ch)		(0x1 << (((ch)<<4) + 1))
#define PMALERTINTCTL_ALERTINT_EN(ch)		(0x1 << (((ch)<<4) + 0))
#define PMALERTINTCTL_ALL_BITS			(0x777 << 0)

#define TMOD_OFFSET				0x00000928
#define TMOD_V_TMOD				(0x1ff << 0)

#define TMODCOEF_OFFSET				0x00000e5c

/* param */
#define TEMP_LIMIT_DEFAULT			(95 * 1000)	 /* Default is 95 degrees Celsius */
#define ALERT_CH_NUM				3

struct uniphier_thermal_priv_t;

struct uniphier_thermal_dev {
	struct device *dev;
	void __iomem  *base;
	int irq;

	u32 alert_en[ALERT_CH_NUM];
	struct thermal_zone_device *tz_dev;
	struct uniphier_thermal_zone *zone;
	struct uniphier_thermal_priv_t *priv;
};

struct uniphier_thermal_zone {
	void __iomem *base;
};

struct uniphier_thermal_priv_t {
	u32 block_offset;
	u32 setup_address;
	u32 setup_value;
};

static inline void maskwritel(void __iomem *base, u32 offset, u32 mask, u32 v)
{
	u32 tmp = readl_relaxed(base + offset);
	tmp &= mask;
	tmp |= v & mask;
	writel_relaxed(v, base + offset);
}

static inline u32 maskreadl(void __iomem *base, u32 offset, u32 mask)
{
	return (readl_relaxed(base + offset) & mask);
}

/* for PXs2 */
static const struct uniphier_thermal_priv_t uniphier_thermal_priv_data_pxs2 = {
	.block_offset  = 0x000,
	.setup_address = 0x904,
	.setup_value   = 0x4f86e844,
};

/* for LD20 */
static const struct uniphier_thermal_priv_t uniphier_thermal_priv_data_ld20 = {
	.block_offset  = 0x800,
	.setup_address = 0x938,
	.setup_value   = 0x4f22e8ee,
};

static int uniphier_thermal_initialize_sensor(struct uniphier_thermal_dev *tdev)
{
	void __iomem *base = tdev->base;

	/* stop PVT control */
	maskwritel(base, tdev->priv->block_offset + PMPVTCTLEN_OFFSET,
		    PMPVTCTLEN_PMPVTCTLEN, PMPVTCTLEN_PMPVTCTLEN_STOP);

	/* set up default if missing eFuse */
	if (readl(base + TMODCOEF_OFFSET) == 0x0) {
		maskwritel(base, tdev->priv->setup_address, ((u32)~0),
			   tdev->priv->setup_value);
	}

	/* set mode of temperature monitor */
	maskwritel(base, tdev->priv->block_offset + PMPVTCTLMODE_OFFSET,
		    PMPVTCTLMODE_PMPVTCTLMODE,
		    PMPVTCTLMODE_PMPVTCTLMODE_TEMPMON);

	/* set period (ENDLESS, 100ms) */
	maskwritel(base, tdev->priv->block_offset + EMONREPEAT_OFFSET,
		    EMONREPEAT_EMONENDLESS |
		    EMONREPEAT_EMONPERIOD,
		    EMONREPEAT_EMONENDLESS_ENABLE |
		    EMONREPEAT_EMONPERIOD_1000000);

	/* set mode select */
	maskwritel(base, PMPVTCTLMODESEL_OFFSET, ((u32)~0), 0);

	return 0;
}

static int uniphier_thermal_set_limit(struct uniphier_thermal_dev *tdev, u32 ch, u32 temp_limit)
{
	void __iomem *base = tdev->base;

	if (ch >= ALERT_CH_NUM)
		return -EINVAL;

	/* set alert temperature */
	maskwritel(base, SETALERT0_OFFSET + (ch << 2),
		   SETALERT_EALERTEN0 |
		   SETALERT_EALERTTEMP0_OF,
		   SETALERT_EALERTEN0_USE |
		   ((temp_limit / 1000) << 16));

	return 0;
}

static int uniphier_thermal_enable_sensor(struct uniphier_thermal_dev *tdev)
{
	void __iomem *base = tdev->base;
	u32 bits = 0;
	int i;

	for(i = 0; i < ALERT_CH_NUM; i++)
		bits |= (tdev->alert_en[i] * PMALERTINTCTL_ALERTINT_EN(i));

	/* enable alert interrupt */
	maskwritel(base, PMALERTINTCTL_OFFSET, PMALERTINTCTL_ALL_BITS, bits);

	/* start PVT control */
	maskwritel(base, tdev->priv->block_offset + PMPVTCTLEN_OFFSET,
		    PMPVTCTLEN_PMPVTCTLEN, PMPVTCTLEN_PMPVTCTLEN_START);

	return 0;
}

static int uniphier_thermal_disable_sensor(struct uniphier_thermal_dev *tdev)
{
	void __iomem *base = tdev->base;

	/* disable alert interrupt */
	maskwritel(base, PMALERTINTCTL_OFFSET, PMALERTINTCTL_ALL_BITS, 0);

	/* stop PVT control */
	maskwritel(base, tdev->priv->block_offset + PMPVTCTLEN_OFFSET,
		    PMPVTCTLEN_PMPVTCTLEN, PMPVTCTLEN_PMPVTCTLEN_STOP);

	return 0;
}

static int uniphier_thermal_get_temp(void *data, int *out_temp)
{
	struct uniphier_thermal_zone *zone = data;
	u32 temp;

	temp = maskreadl(zone->base, TMOD_OFFSET, TMOD_V_TMOD);
	*out_temp = temp * 1000;	/* millicelsius */

	return 0;
}

static const struct thermal_zone_of_device_ops uniphier_of_thermal_ops = {
	.get_temp = uniphier_thermal_get_temp,
};

static void __uniphier_thermal_irq_clear(void __iomem *base)
{
	u32 mask = 0, bits = 0;
	int i;

	for(i = 0; i < ALERT_CH_NUM; i++) {
		mask |= (PMALERTINTCTL_ALERTINT_CLR(i) |
			 PMALERTINTCTL_ALERTINT_ST(i));
		bits |= PMALERTINTCTL_ALERTINT_CLR(i);
	}

	/* clear alert interrupt */
	maskwritel(base, PMALERTINTCTL_OFFSET, mask, bits);

	return;
}

static irqreturn_t uniphier_thermal_alarm_handler(int irq, void *_tdev)
{
	struct uniphier_thermal_dev *tdev = _tdev;

	dev_dbg(tdev->dev, "thermal alarm\n");

	__uniphier_thermal_irq_clear(tdev->base);

	thermal_zone_device_update(tdev->tz_dev);

	return IRQ_HANDLED;
}

static const struct of_device_id uniphier_thermal_dt_ids[];

static int uniphier_thermal_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct uniphier_thermal_dev *tdev;
	struct uniphier_thermal_zone *zone;
	const struct thermal_trip *trips;
	const struct of_device_id *of_id;
	int i, ret, irq;
	u32 temp_limit = TEMP_LIMIT_DEFAULT;

	of_id = of_match_device(uniphier_thermal_dt_ids, dev);
	if (!of_id) {
		dev_err(dev, "failed to probe\n");
		return -EINVAL;
	}

	tdev = devm_kzalloc(dev, sizeof(*tdev), GFP_KERNEL);
	if (!tdev) {
		dev_err(dev, "failed to allocate memory for driver\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, tdev);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	tdev->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(tdev->base)) {
		dev_err(dev, "failed to map resource\n");
		ret = PTR_ERR(tdev->base);
		goto err_tdev;
	}

	tdev->dev = dev;
	tdev->priv = (struct uniphier_thermal_priv_t *)of_id->data;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(dev, "failed to get irq\n");
		ret = -EINVAL;
		goto err_map;
	}

	tdev->irq = irq;

	/* register sensor */
	zone = devm_kzalloc(dev, sizeof(*zone), GFP_KERNEL);
	if (!zone) {
		dev_err(dev, "failed to allocate memory for sensor\n");
		ret = -ENOMEM;
		goto err_map;
	}

	zone->base = tdev->base;
	tdev->zone = zone;
	tdev->tz_dev = thermal_zone_of_sensor_register(dev, 0, zone,
					     &uniphier_of_thermal_ops);
	if (IS_ERR(tdev->tz_dev)) {
		dev_err(dev, "failed to register thermal zone\n");
		ret = PTR_ERR(tdev->tz_dev);
		goto err_zone;
	}

	/* get trips */
	trips = of_thermal_get_trip_points(tdev->tz_dev);
	if (of_thermal_get_ntrips(tdev->tz_dev) > ALERT_CH_NUM) {
		dev_err(dev, "the number of trips exceeds %d\n", ALERT_CH_NUM);
		ret = -EINVAL;
		goto err_sensor;
	}
	for (i = 0; i < of_thermal_get_ntrips(tdev->tz_dev); i++) {
		if (trips[i].type == THERMAL_TRIP_CRITICAL) {
			temp_limit = trips[i].temperature;
			break;
		}
	}
	if (i == of_thermal_get_ntrips(tdev->tz_dev)) {
		dev_err(dev, "failed to find critical trip\n");
		ret = -EINVAL;
		goto err_sensor;
	}

	/* initialize sensor */
	uniphier_thermal_initialize_sensor(tdev);
	for (i = 0; i < ALERT_CH_NUM; i++) {
		tdev->alert_en[i] = 0;
	}
	for (i = 0; i < of_thermal_get_ntrips(tdev->tz_dev); i++) {
		uniphier_thermal_set_limit(tdev, i, trips[i].temperature);
		tdev->alert_en[i] = 1;
	}

	ret = devm_request_irq(dev, tdev->irq, uniphier_thermal_alarm_handler,
			       0, "thermal", tdev);
	if (ret) {
		dev_err(dev, "failed to execute request_irq\n");
		goto err_sensor;
	}

	/* enable sensor */
	uniphier_thermal_enable_sensor(tdev);

	dev_info(dev, "thermal driver (temp_limit=%d C)\n",
		 temp_limit / 1000);

	return 0;

err_sensor:
	thermal_zone_of_sensor_unregister(dev, tdev->tz_dev);
err_zone:
	devm_kfree(dev, tdev->zone);
err_map:
	devm_iounmap(dev, tdev->base);
err_tdev:
	devm_kfree(dev, tdev);

	return ret;
}

static int uniphier_thermal_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct uniphier_thermal_dev *tdev = platform_get_drvdata(pdev);

	/* disable sensor */
	uniphier_thermal_disable_sensor(tdev);

	devm_free_irq(dev, tdev->irq, tdev);
	thermal_zone_of_sensor_unregister(dev, tdev->tz_dev);
	devm_kfree(dev, tdev->zone);
	devm_iounmap(dev, tdev->base);
	devm_kfree(dev, tdev);

	return 0;
}

static const struct of_device_id uniphier_thermal_dt_ids[] = {
	{
		.compatible = "socionext,proxstream2-thermal",
		.data       = (void *)&uniphier_thermal_priv_data_pxs2,
	},
	{
		.compatible = "socionext,ph1-ld20-thermal",
		.data       = (void *)&uniphier_thermal_priv_data_ld20,
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, uniphier_thermal_dt_ids);

static struct platform_driver uniphier_thermal_driver = {
	.probe = uniphier_thermal_probe,
	.remove = uniphier_thermal_remove,
	.driver = {
		.name = "uniphier-thermal",
		.of_match_table = uniphier_thermal_dt_ids,
	},
};
module_platform_driver(uniphier_thermal_driver);

MODULE_AUTHOR("Kunihiko Hayashi <hayashi.kunihiko@socionext.com>");
MODULE_DESCRIPTION("UniPhier thermal management driver");
MODULE_LICENSE("GPL v2");
