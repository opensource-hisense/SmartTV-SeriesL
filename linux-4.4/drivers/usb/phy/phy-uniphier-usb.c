/*
 * phy-uniphier-usb - USB PHY Driver for UniPhier
 *
 * Copyright (C) 2015 Socionext Inc.
 *
 * Author: Kunihiko Hayashi <hayashi.kunihiko@socionext.com>
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/phy/phy.h>

#include <linux/of_platform.h>

#include "phy-generic.h"

struct uniphier_usbphy {
	struct device	*dev;
	struct usb_phy	phy;
	void __iomem	*base;
};

static inline u32 uniphier_usbphy_readl(void __iomem *base, u32 offset)
{
	return readl(base + offset);
}

static inline void uniphier_usbphy_writel(void __iomem *base,
					  u32 offset, u32 value)
{
	writel(value, base + offset);
}

/* for Pro4 */

static int uniphier_usb_phy_init_pro4(struct phy *phy)
{
	struct uniphier_usbphy *uni_phy = phy_get_drvdata(phy);
	void __iomem *base = uni_phy->base;

	/* setting SS PHY */
	uniphier_usbphy_writel( base, 0x00, 0x05142400); /* USBPHY1CTRL */
	uniphier_usbphy_writel( base, 0x08, 0x05142400); /* USBPHY2CTRL */
	uniphier_usbphy_writel( base, 0x04, 0x00000007); /* USBPHY1CTRL_2 */
	uniphier_usbphy_writel( base, 0x0c, 0x00010010); /* USBPHY_PLLCTRL */

	/* setting HS PHY */
	uniphier_usbphy_writel( base, 0x10, 0x05142400); /* USBPHY3CTRL */
	uniphier_usbphy_writel( base, 0x18, 0x05142400); /* USBPHY4CTRL */
	uniphier_usbphy_writel( base, 0x14, 0x00000007); /* USBPHY3CTRL_2 */
	uniphier_usbphy_writel( base, 0x1c, 0x00010010); /* USBPHY_PLLCTRL */

	return 0;
}

static const struct phy_ops uniphier_usb_phy_ops_pro4 = {
	.init		= uniphier_usb_phy_init_pro4,
	.exit		= NULL,
	.power_on	= NULL,
	.power_off	= NULL,
	.owner		= THIS_MODULE,
};

/* for LD11 */

static int uniphier_usb_phy_init_ld11(struct phy *phy)
{
	struct uniphier_usbphy *uni_phy = phy_get_drvdata(phy);
	void __iomem *base = uni_phy->base;

	struct device_node *node = phy->dev.of_node; /* node on DTS */

	u32 ctrl2 = 0x00000106;
	if (of_device_is_compatible(node, "socionext,ph1-ld11pa-usbphy"))
		ctrl2 = 0x00000116;	/* specific for pa */

	/* setting HS PHY CH0 */
	uniphier_usbphy_writel( base, 0x00, 0x82280600); /* USBPHY1CTRL */
	uniphier_usbphy_writel( base, 0x04, ctrl2);      /* USBPHY1CTRL2 */

	/* setting HS PHY CH1 */
	uniphier_usbphy_writel( base, 0x08, 0x82280600); /* USBPHY2CTRL */
	uniphier_usbphy_writel( base, 0x0c, ctrl2);      /* USBPHY2CTRL2 */

	/* setting HS PHY CH2 */
	uniphier_usbphy_writel( base, 0x10, 0x82280600); /* USBPHY3CTRL */
	uniphier_usbphy_writel( base, 0x14, ctrl2);      /* USBPHY3CTRL2 */

	return 0;
}

static const struct phy_ops uniphier_usb_phy_ops_ld11 = {
	.init		= uniphier_usb_phy_init_ld11,
	.exit		= NULL,
	.power_on	= NULL,
	.power_off	= NULL,
	.owner		= THIS_MODULE,
};


static const struct of_device_id uniphier_usbphy_ids[];

static int uniphier_usbphy_probe(struct platform_device *pdev)
{
	struct device		*dev = &pdev->dev;
	struct resource		*res;
	struct uniphier_usbphy	*uni_phy;
	struct phy		*generic_phy;
	struct phy_ops		*ops;
	struct phy_provider	*phy_provider;
	const struct of_device_id *of_id;

	of_id = of_match_device(uniphier_usbphy_ids, dev);
	if (!of_id)
		return -EINVAL;

	uni_phy = devm_kzalloc(dev, sizeof(*uni_phy), GFP_KERNEL);
	if (!uni_phy)
		return -ENOMEM;

	/* base */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	uni_phy->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(uni_phy->base))
		return PTR_ERR(uni_phy->base);

	ops = (struct phy_ops *)of_id->data;

	uni_phy->dev       = dev;

	uni_phy->phy.dev   = dev;
	uni_phy->phy.label = NULL;
	uni_phy->phy.otg   = NULL;
	uni_phy->phy.type  = USB_PHY_TYPE_USB2; /* common to USB2 and USB3 */

	generic_phy = devm_phy_create(dev, NULL, ops);
	if (IS_ERR(generic_phy)) {
		dev_err(dev, "failed to create PHY\n");
		return PTR_ERR(generic_phy);
	}

	phy_set_drvdata(generic_phy, uni_phy);

	phy_provider = devm_of_phy_provider_register(dev,
						     of_phy_simple_xlate);
	if (IS_ERR(phy_provider)) {
		return PTR_ERR(phy_provider);
	}

	usb_add_phy_dev(&uni_phy->phy);

	dev_info(dev, "UniPhier USB-PHY driver\n");
	return 0;
}

static int uniphier_usbphy_remove(struct platform_device *pdev)
{
	struct uniphier_usbphy	*uni_phy = platform_get_drvdata(pdev);

	usb_remove_phy(&uni_phy->phy);

	return 0;
}

static const struct of_device_id uniphier_usbphy_ids[] = {
	{
		.compatible = "socionext,ph1-pro4-usbphy",
		.data       = (void *)&uniphier_usb_phy_ops_pro4,
	},
	{
		.compatible = "socionext,ph1-ld11-usbphy",
		.data       = (void *)&uniphier_usb_phy_ops_ld11,
	},
	{
		.compatible = "socionext,ph1-ld11pa-usbphy",
		.data       = (void *)&uniphier_usb_phy_ops_ld11,
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, uniphier_usbphy_ids);

static struct platform_driver uniphier_usbphy_driver = {
	.probe          = uniphier_usbphy_probe,
	.remove         = uniphier_usbphy_remove,
	.driver         = {
		.name   = "uniphier-usbphy",
		.of_match_table = uniphier_usbphy_ids,
	},
};
module_platform_driver(uniphier_usbphy_driver);

MODULE_ALIAS("platform:uniphier-usbphy");
MODULE_AUTHOR("Kuniiko Hayashi <hayashi.kunihiko@socionext.com>");
MODULE_DESCRIPTION("UniPhier USB PHY driver");
MODULE_LICENSE("GPL v2");
