/*
 * drivers/ata/ahci_uniphier.c
 *
 * Copyright (c) 2016, Socionext Inc.
 * All rights reserved.
 *
 * Author:
 *	Kunihiko Hayashi <hayashi.kunihiko@socionext.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/ahci_platform.h>

#include "ahci.h"

#define DRV_NAME "uniphier-ahci"


struct uniphier_ahci_priv {
	struct platform_device *pdev;
	void __iomem           *sata_regs;
};

#define SG_OFS_SATASRCSEL       (0x2210)
#define SATASRCSELEN            (1 << 4)
#define SATASRCSEL              (0xf << 0)
#define SATASRCSEL_AXO_SINGLE   (0xc << 0)

#define SATA_OFS_RSTCTRL        (0x0000)
#define SATA_OFS_AXIUSER        (0x0004)
#define SATA_OFS_CKCTRL         (0x0010)

/* for RSTCTRL */
#define XRSTLINK                (1 <<  0)
/* for CKCTRL */
#define REF_SSP_EN              (1 <<  9)
#define P0_RESET                (1 << 10)
#define P0_READY                (1 << 15)

static void maskwritel(void __iomem *addr, u32 mask, u32 val)
{
	u32 tmp;

	tmp = readl(addr);
	tmp &= ~mask;
	tmp |= (mask & val);
	writel(tmp,addr);
}

static int uniphier_ahci_phy_init(struct ahci_host_priv *hpriv)
{
	struct uniphier_ahci_priv *upriv = hpriv->plat_data;
	u32 timeout;

	dev_dbg(&upriv->pdev->dev, "%s start\n",__func__);

	/* set ref_ssp_en and clear p0_reset */
	maskwritel(upriv->sata_regs + SATA_OFS_CKCTRL, REF_SSP_EN, REF_SSP_EN);
	maskwritel(upriv->sata_regs + SATA_OFS_CKCTRL, P0_RESET, 0);

	/* release SATA LINK reset */
	maskwritel(upriv->sata_regs + SATA_OFS_RSTCTRL, XRSTLINK, XRSTLINK);

	/* wait until p0_ready is asserted */
	timeout = 10;
	do {
		if (readl(upriv->sata_regs + SATA_OFS_CKCTRL) & P0_READY)
			break;
		if (--timeout == 0) {
			dev_err(&upriv->pdev->dev, "PHY reset failed.\n");
			return -EIO;
		}
		msleep(1);
	} while(1);

	dev_dbg(&upriv->pdev->dev, "%s exit\n",__func__);

	return 0;
}

static void uniphier_ahci_host_stop(struct ata_host *host)
{
	struct ahci_host_priv *hpriv = host->private_data;

	ahci_platform_disable_resources(hpriv);
}

static struct ata_port_operations uniphier_ahci_port_ops = {
	.inherits	= &ahci_platform_ops,
	.host_stop	= uniphier_ahci_host_stop,
};

static const struct ata_port_info uniphier_ahci_port_info = {
	.flags          = AHCI_FLAG_COMMON,
	.pio_mask       = ATA_PIO4,
	.udma_mask      = ATA_UDMA6,
	.port_ops       = &uniphier_ahci_port_ops,
};

static struct scsi_host_template ahci_platform_sht = {
	AHCI_SHT(DRV_NAME),
};


static void uniphier_ahci_clock_select(struct platform_device *pdev, int index)
{
	struct device_node *clk_node;
	void __iomem *vptr;

	clk_node = of_parse_phandle(pdev->dev.of_node, "clocks", 0);
	if (!clk_node) {
		dev_warn(&pdev->dev, "Failed to get clock select node\n");
		goto err_ret;
	}
	vptr = of_iomap(clk_node, 0);
	if (!vptr) {
		dev_warn(&pdev->dev, "Failed to map clock register\n");
		goto err_node;
	}
	maskwritel(vptr + SG_OFS_SATASRCSEL,
		   SATASRCSELEN | SATASRCSEL,
		   SATASRCSELEN | (index & SATASRCSEL));
	iounmap(vptr);
err_node:
	of_node_put(clk_node);
err_ret:
	return;
}

static int uniphier_ahci_probe(struct platform_device *pdev)
{
	struct ahci_host_priv *hpriv;
	struct uniphier_ahci_priv *upriv;
	struct resource *res;
	int ret;

	upriv = devm_kzalloc(&pdev->dev, sizeof(*upriv), GFP_KERNEL);
	if (!upriv)
		return -ENOMEM;
	upriv->pdev = pdev;

	/* select clock source */
	uniphier_ahci_clock_select(pdev, SATASRCSEL_AXO_SINGLE);

	hpriv = ahci_platform_get_resources(pdev);
	if (IS_ERR(hpriv))
		return PTR_ERR(hpriv);

	hpriv->plat_data = upriv;

	ret = ahci_platform_enable_resources(hpriv);
	if (ret) {
		return ret;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	upriv->sata_regs = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(upriv->sata_regs))
		return PTR_ERR(upriv->sata_regs);

	ret = uniphier_ahci_phy_init(hpriv);
	if (ret) {
		ahci_platform_disable_resources(hpriv);
		return ret;
	}

	/* force to set HOSTS_PORTS_IMPL (for AHCI 1.3) */
	writel(BIT(0), hpriv->mmio + HOST_PORTS_IMPL);

	ret = ahci_platform_init_host(pdev, hpriv, &uniphier_ahci_port_info,
				      &ahci_platform_sht);
	if (ret) {
		ahci_platform_disable_resources(hpriv);
		return ret;
	}

	return 0;
}

static const struct of_device_id uniphier_ahci_match[] = {
	{ .compatible = "socionext,uniphier-ahci", },
	{},
};
MODULE_DEVICE_TABLE(of, uniphier_ahci_match);

static struct platform_driver uniphier_ahci_driver = {
	.probe = uniphier_ahci_probe,
	.remove = ata_platform_remove_one,
	.driver = {
		.name = DRV_NAME,
		.of_match_table = of_match_ptr(uniphier_ahci_match),
	},
};
module_platform_driver(uniphier_ahci_driver);


MODULE_AUTHOR("Kunihiko Hayashi <hayashi.kunihiko@socionext.com>");
MODULE_DESCRIPTION("UniPhier AHCI SATA driver");
MODULE_LICENSE("GPL v2");
