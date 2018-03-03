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

#include <linux/module.h>
#include <linux/mmc/host.h>

#include "sdhci-pltfm.h"

static const struct sdhci_pltfm_data cdns_sdhci_pltfm_data = {
	.quirks = SDHCI_QUIRK_DATA_TIMEOUT_USES_SDCLK |
	SDHCI_QUIRK_BROKEN_TIMEOUT_VAL,
};

static int cdns_sdhci_probe(struct platform_device *pdev)
{
	return sdhci_pltfm_register(pdev, &cdns_sdhci_pltfm_data, 0);
}

static const struct of_device_id cdns_sdhci_match[] = {
	{ .compatible = "cdns,cdns-sdhci" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, cdns_sdhci_of_match);

static struct platform_driver cdns_sdhci_driver = {
	.driver = {
		.name = "cadence-sdhci",
		.pm = SDHCI_PLTFM_PMOPS,
		.of_match_table = cdns_sdhci_match,
	},
	.probe = cdns_sdhci_probe,
	.resume = sdhci_pltfm_unregister,
};
module_platform_driver(cdns_sdhci_driver);

/* Glue layer */

#include <linux/of_platform.h>

static const struct of_device_id cdns_sdhci_glue_match[] = {
	{ .compatible = "cdns,cdns-sdhci-glue" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, cdns_sdhci_of_match);

#define SDIO_REG_HRS4	    0x10
// delay regs address
  #define REG_DELAY_HS          0x00
  #define REG_DELAY_DEFAULT     0x01
  #define REG_DELAY_UHSI_SDR12  0x02
  #define REG_DELAY_UHSI_SDR25  0x03
  #define REG_DELAY_UHSI_SDR50  0x04
  #define REG_DELAY_UHSI_DDR50  0x05
  #define REG_DELAY_MMC_LEGACY  0x06
  #define REG_DELAY_MMC_SDR     0x07
  #define REG_DELAY_MMC_DDR     0x08

static void sd4_set_dlyvr(void __iomem *ioaddr,
			  unsigned char addr, unsigned char data)
{
	u32 dlyrv_reg;

	dlyrv_reg = data << 8;
	dlyrv_reg |= addr;

	//set data and address
	writel(dlyrv_reg, ioaddr + SDIO_REG_HRS4);
	dlyrv_reg |= (1 << 24);
	//send write request
	writel(dlyrv_reg, ioaddr + SDIO_REG_HRS4);
	dlyrv_reg &= ~(1 << 24);
	//clear write request
	writel(dlyrv_reg, ioaddr + SDIO_REG_HRS4);
}

static void phy_config(void __iomem *ioaddr)
{
	sd4_set_dlyvr(ioaddr, REG_DELAY_DEFAULT, 0x04);
	sd4_set_dlyvr(ioaddr, REG_DELAY_HS, 0x04);
	sd4_set_dlyvr(ioaddr, REG_DELAY_UHSI_SDR50, 0x06);
	sd4_set_dlyvr(ioaddr, REG_DELAY_UHSI_DDR50, 0x16);
}

static int cdns_sdhci_glue_probe(struct platform_device *pdev)
{
	struct resource *iomem;
	void __iomem *ioaddr;

	iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ioaddr = devm_ioremap_resource(&pdev->dev, iomem);
	if (IS_ERR(ioaddr))
		return PTR_ERR(ioaddr);

	phy_config(ioaddr);

	return of_platform_populate(pdev->dev.of_node, NULL, NULL, &pdev->dev);
}

static int cdns_sdhci_glue_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver cdns_sdhci_glue_driver = {
	.driver = {
		.name = "cadence-sdhci-glue",
		.of_match_table = cdns_sdhci_glue_match,
	},
	.probe = cdns_sdhci_glue_probe,
	.resume = cdns_sdhci_glue_remove,
};
module_platform_driver(cdns_sdhci_glue_driver);

MODULE_AUTHOR("Masahiro Yamada <yamada.masahiro@socionext.com>");
MODULE_DESCRIPTION("Cadence Secure Digital Host Controller");
MODULE_LICENSE("GPL");
