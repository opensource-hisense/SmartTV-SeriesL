/*
 * Watchdog driver for the UniPhier watchdog timer
 *
 * (c) Copyright 2014 Panasonic Corporation
 * (c) Copyright 2016 Socionext Inc.
 * All rights reserved.
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
 *
 * Based on mtk_wdt.c
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/watchdog.h>
#include <linux/delay.h>


#define WDTTIMSET_OFFSET		0x00000004
#define WDTTIMSET_WDTPRD		(0xf << 0)
#define WDTTIMSET_WDTPRD_0_125_SEC	(0x0 << 0)
#define WDTTIMSET_WDTPRD_0_25_SEC	(0x1 << 0)
#define WDTTIMSET_WDTPRD_0_5_SEC	(0x2 << 0)
#define WDTTIMSET_WDTPRD_1_SEC		(0x3 << 0)
#define WDTTIMSET_WDTPRD_2_SEC		(0x4 << 0)
#define WDTTIMSET_WDTPRD_4_SEC		(0x5 << 0)
#define WDTTIMSET_WDTPRD_8_SEC		(0x6 << 0)
#define WDTTIMSET_WDTPRD_16_SEC		(0x7 << 0)
#define WDTTIMSET_WDTPRD_32_SEC		(0x8 << 0)
#define WDTTIMSET_WDTPRD_64_SEC		(0x9 << 0)
#define WDTTIMSET_WDTPRD_128_SEC	(0xa << 0)
#ifndef __ASSEMBLY__
#define SEC_TO_WDTTIMSET_WDTPRD(sec) \
		(((fls(sec) - 1) << 0) + WDTTIMSET_WDTPRD_1_SEC)
#endif /* !__ASSEMBLY__ */

#define WDTRSTSEL_OFFSET		0x00000008
#define WDTRSTSEL_WDTRSTSEL		(0x3 << 0)
#define WDTRSTSEL_WDTRSTSEL_BOTH	(0x0 << 0)
#define WDTRSTSEL_WDTRSTSEL_IRQ_ONLY	(0x2 << 0)

#define WDTCTRL_OFFSET			0x0000000c
#define WDTCTRL_WDTST			(0x1 << 8)
#define WDTCTRL_WDTST_STOPPED		(0x0 << 8)
#define WDTCTRL_WDTST_WORKING		(0x1 << 8)
#define WDTCTRL_WDTCLR			(0x1 << 1)
#define WDTCTRL_WDTCLR_STOP		(0x0 << 1)
#define WDTCTRL_WDTCLR_INIT		(0x1 << 1)
#define WDTCTRL_WDTEN			(0x1 << 0)
#define WDTCTRL_WDTEN_DISABLE		(0x0 << 0)
#define WDTCTRL_WDTEN_ENABLE		(0x1 << 0)

#define WDTCLR_MINIMUM_INTERVAL		61 /* usec */

#define WDT_TIMER_MARGIN		64	/* Default is 64 seconds */
#define WDT_PERIOD_MIN			1
#define WDT_PERIOD_MAX			128

static unsigned int timeout = WDT_TIMER_MARGIN;
static bool nowayout = WATCHDOG_NOWAYOUT;

struct uniphier_wdt_dev {
	struct watchdog_device wdt_dev;
	void __iomem	*base;	/* virtual address */
	spinlock_t	lock;
};

/*
 * UniPhier Watchdog operations
 */

static int uniphier_watchdog_ping(struct watchdog_device *w)
{
	struct uniphier_wdt_dev *wdev= watchdog_get_drvdata(w);
	void __iomem *base = wdev->base;
	unsigned long flags;

	spin_lock_irqsave(&wdev->lock, flags);

	/* Clear counter */
	writel(WDTCTRL_WDTCLR_INIT | WDTCTRL_WDTEN_ENABLE,
	       base + WDTCTRL_OFFSET);
	udelay(WDTCLR_MINIMUM_INTERVAL);

	while ((readl(base + WDTCTRL_OFFSET) & WDTCTRL_WDTST)
	       != WDTCTRL_WDTST_WORKING);

	spin_unlock_irqrestore(&wdev->lock, flags);

	return 0;
}

static int __uniphier_watchdog_start(void __iomem *base, unsigned int sec)
{
	if ((readl(base + WDTCTRL_OFFSET) & WDTCTRL_WDTST)
	    != WDTCTRL_WDTST_STOPPED) {
		/* Disable and stop watchdog */
		writel(WDTCTRL_WDTEN_DISABLE, base + WDTCTRL_OFFSET);

		while ((readl(base + WDTCTRL_OFFSET) & WDTCTRL_WDTST)
		       != WDTCTRL_WDTST_STOPPED);
	}

	/* Setup period */
	writel(SEC_TO_WDTTIMSET_WDTPRD(sec), base + WDTTIMSET_OFFSET);

	/* Enable and clear watchdog */
	writel(WDTCTRL_WDTEN_ENABLE | WDTCTRL_WDTCLR_INIT,
	       base + WDTCTRL_OFFSET);
	udelay(WDTCLR_MINIMUM_INTERVAL);

	while ((readl(base + WDTCTRL_OFFSET) & WDTCTRL_WDTST)
	       != WDTCTRL_WDTST_WORKING);

	return 0;
}

static int uniphier_watchdog_start(struct watchdog_device *w)
{
	struct uniphier_wdt_dev *wdev= watchdog_get_drvdata(w);
	void __iomem *base = wdev->base;
	unsigned int tmp_timeout;
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&wdev->lock, flags);

	tmp_timeout = 1 << (fls(w->timeout) - 1);
	if (tmp_timeout != w->timeout) {
		tmp_timeout <<= 1;
	}

	ret = __uniphier_watchdog_start(base, tmp_timeout);

	spin_unlock_irqrestore(&wdev->lock, flags);

	if (ret) {
		dev_err(w->parent, "cannot start watchdog(%d)\n", ret);
		return ret;
	}

	dev_info(w->parent, "watchdog timer started."
		" timeout=%d sec (nowayout=%d)\n",
		tmp_timeout, nowayout);

	return 0;
}

static int __uniphier_watchdog_stop(void __iomem *base)
{
	if ((readl(base + WDTCTRL_OFFSET) & WDTCTRL_WDTST)
	    == WDTCTRL_WDTST_STOPPED) {
		return 0;
	}

	/* Disable and stop watchdog */
	writel(WDTCTRL_WDTEN_DISABLE, base + WDTCTRL_OFFSET);

	while ((readl(base + WDTCTRL_OFFSET) & WDTCTRL_WDTST)
	       != WDTCTRL_WDTST_STOPPED);

	return 0;
}

static int uniphier_watchdog_stop(struct watchdog_device *w)
{
	struct uniphier_wdt_dev *wdev= watchdog_get_drvdata(w);
	void __iomem *base = wdev->base;
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&wdev->lock, flags);
	ret = __uniphier_watchdog_stop(base);
	spin_unlock_irqrestore(&wdev->lock, flags);

	if (ret) {
		dev_err(w->parent, "cannot stop watchdog(%d)\n", ret);
		return ret;
	}

	dev_info(w->parent, "watchdog timer stopped\n");

	return 0;
}

static int uniphier_watchdog_set_timeout(struct watchdog_device *w, unsigned int t)
{
	struct uniphier_wdt_dev *wdev= watchdog_get_drvdata(w);
	void __iomem *base = wdev->base;
	unsigned int tmp_timeout, old_timeout;
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&wdev->lock, flags);

	tmp_timeout = 1 << (fls(t) - 1);
	if (tmp_timeout != t) {
		tmp_timeout <<= 1;
	}

	if (tmp_timeout == w->timeout) {
		spin_unlock_irqrestore(&wdev->lock, flags);
		return 0;
	}

	if ((readl(base + WDTCTRL_OFFSET) & WDTCTRL_WDTST)
	    != WDTCTRL_WDTST_STOPPED) {
		ret = __uniphier_watchdog_start(base, tmp_timeout);
		if (ret) {
			spin_unlock_irqrestore(&wdev->lock, flags);
			dev_err(w->parent, "cannot restart watchdog(%d)\n", ret);
			 return ret;
		}
	}

	old_timeout = w->timeout;
	w->timeout = tmp_timeout;

	spin_unlock_irqrestore(&wdev->lock, flags);

	dev_info(w->parent, "timeout changed(%d sec => %d sec)\n",
		 old_timeout, tmp_timeout);

	return 0;
}

/*
 * Kernel Interfaces
 */
static const struct watchdog_info uniphier_wdt_info = {
	.identity	= "uniphier-wdt",
	.options	= WDIOF_SETTIMEOUT |
			  WDIOF_KEEPALIVEPING |
			  WDIOF_MAGICCLOSE |
			  WDIOF_OVERHEAT,
};

static const struct watchdog_ops uniphier_wdt_ops = {
	.owner		= THIS_MODULE,
	.start		= uniphier_watchdog_start,
	.stop		= uniphier_watchdog_stop,
	.ping		= uniphier_watchdog_ping,
	.set_timeout	= uniphier_watchdog_set_timeout,
	.restart	= NULL,
};

static int uniphier_wdt_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct uniphier_wdt_dev *wdev;
	struct resource *res;
	int ret;

	/* Check that the timeout value is within it's range;
	   if not reset to the default */
	if (timeout < WDT_PERIOD_MIN || timeout > WDT_PERIOD_MAX) {
		dev_err(dev, "%s: %d: timeout must be %d < timeout < %d, using %d\n",
			__func__, __LINE__,
			WDT_PERIOD_MIN - 1,
			WDT_PERIOD_MAX + 1,
			WDT_TIMER_MARGIN);
		return -EINVAL;
	}

	wdev = devm_kzalloc(dev, sizeof(*wdev), GFP_KERNEL);
	if (!wdev)
		return -ENOMEM;

	platform_set_drvdata(pdev, wdev);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	wdev->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(wdev->base))
		return PTR_ERR(wdev->base);

	wdev->wdt_dev.info = &uniphier_wdt_info;
	wdev->wdt_dev.ops = &uniphier_wdt_ops;
	wdev->wdt_dev.timeout = WDT_TIMER_MARGIN;
	wdev->wdt_dev.max_timeout = WDT_PERIOD_MAX;
	wdev->wdt_dev.min_timeout = WDT_PERIOD_MIN;
	wdev->wdt_dev.parent = dev;

	watchdog_init_timeout(&wdev->wdt_dev, timeout, dev);
	watchdog_set_nowayout(&wdev->wdt_dev, nowayout);
	watchdog_set_restart_priority(&wdev->wdt_dev, 128);

	watchdog_set_drvdata(&wdev->wdt_dev, wdev);

	uniphier_watchdog_stop(&wdev->wdt_dev);
	writel(WDTRSTSEL_WDTRSTSEL_BOTH, wdev->base + WDTRSTSEL_OFFSET);

	ret = watchdog_register_device(&wdev->wdt_dev);
	if (unlikely(ret))
		return ret;

	dev_info(dev, "watchdog driver (timeout=%d sec, nowayout=%d)\n",
		 wdev->wdt_dev.timeout, nowayout);

	return 0;
}

static void uniphier_wdt_shutdown(struct platform_device *pdev)
{
	struct uniphier_wdt_dev *wdev = platform_get_drvdata(pdev);

	if (watchdog_active(&wdev->wdt_dev))
		uniphier_watchdog_stop(&wdev->wdt_dev);
}

static int uniphier_wdt_remove(struct platform_device *pdev)
{
	struct uniphier_wdt_dev *wdev = platform_get_drvdata(pdev);

	watchdog_unregister_device(&wdev->wdt_dev);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int uniphier_wdt_suspend(struct device *dev)
{
	struct uniphier_wdt_dev *wdev = dev_get_drvdata(dev);

	if (watchdog_active(&wdev->wdt_dev))
		uniphier_watchdog_stop(&wdev->wdt_dev);

	return 0;
}

static int uniphier_wdt_resume(struct device *dev)
{
	struct uniphier_wdt_dev *wdev = dev_get_drvdata(dev);

	if (watchdog_active(&wdev->wdt_dev)) {
		uniphier_watchdog_start(&wdev->wdt_dev);
		uniphier_watchdog_ping(&wdev->wdt_dev);
	}

	return 0;
}
#endif

static const struct of_device_id uniphier_wdt_dt_ids[] = {
	{ .compatible = "socionext,uniphier-wdt" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, uniphier_wdt_dt_ids);

static const struct dev_pm_ops uniphier_wdt_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(uniphier_wdt_suspend,
				uniphier_wdt_resume)
};

static struct platform_driver uniphier_wdt_driver = {
	.probe		= uniphier_wdt_probe,
	.remove		= uniphier_wdt_remove,
	.shutdown	= uniphier_wdt_shutdown,
	.driver		= {
		.name		= "uniphier-wdt",
		.pm		= &uniphier_wdt_pm_ops,
		.of_match_table	= uniphier_wdt_dt_ids,
	},
};

module_platform_driver(uniphier_wdt_driver);

module_param(timeout, uint, 0);
MODULE_PARM_DESC(timeout,
	"Watchdog timeout in seconds. (0 < timeout < 128, default="
					__MODULE_STRING(TIMER_MARGIN) ")");

module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout,
		"Watchdog cannot be stopped once started (default="
				__MODULE_STRING(WATCHDOG_NOWAYOUT) ")");

MODULE_AUTHOR("Socionext Inc.");
MODULE_DESCRIPTION("UniPhier Watchdog Device Driver");
MODULE_LICENSE("GPL v2");
