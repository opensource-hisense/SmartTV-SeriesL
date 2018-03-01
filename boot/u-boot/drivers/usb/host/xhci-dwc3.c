/*
 * Copyright 2015 Freescale Semiconductor, Inc.
 *
 * DWC3 controller driver
 *
 * Author: Ramneek Mehresh<ramneek.mehresh@freescale.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <dm/device.h>
#include <mapmem.h>
#include <linux/io.h>
#include <linux/usb/dwc3.h>
#include <linux/sizes.h>

#include "xhci.h"

void dwc3_set_mode(struct dwc3 *dwc3_reg, u32 mode)
{
	clrsetbits_le32(&dwc3_reg->g_ctl,
			DWC3_GCTL_PRTCAPDIR(DWC3_GCTL_PRTCAP_OTG),
			DWC3_GCTL_PRTCAPDIR(mode));
}

void dwc3_phy_reset(struct dwc3 *dwc3_reg)
{
	/* Assert USB3 PHY reset */
	setbits_le32(&dwc3_reg->g_usb3pipectl[0], DWC3_GUSB3PIPECTL_PHYSOFTRST);

	/* Assert USB2 PHY reset */
	setbits_le32(&dwc3_reg->g_usb2phycfg, DWC3_GUSB2PHYCFG_PHYSOFTRST);

	mdelay(100);

	/* Clear USB3 PHY reset */
	clrbits_le32(&dwc3_reg->g_usb3pipectl[0], DWC3_GUSB3PIPECTL_PHYSOFTRST);

	/* Clear USB2 PHY reset */
	clrbits_le32(&dwc3_reg->g_usb2phycfg, DWC3_GUSB2PHYCFG_PHYSOFTRST);
}

void dwc3_core_soft_reset(struct dwc3 *dwc3_reg)
{
	/* Before Resetting PHY, put Core in Reset */
	setbits_le32(&dwc3_reg->g_ctl, DWC3_GCTL_CORESOFTRESET);

	/* reset USB3 phy - if required */
	dwc3_phy_reset(dwc3_reg);

	mdelay(100);

	/* After PHYs are stable we can take Core out of reset state */
	clrbits_le32(&dwc3_reg->g_ctl, DWC3_GCTL_CORESOFTRESET);
}

int dwc3_core_init(struct dwc3 *dwc3_reg)
{
	u32 reg;
	u32 revision;
	unsigned int dwc3_hwparams1;

	revision = readl(&dwc3_reg->g_snpsid);
	/* This should read as U3 followed by revision number */
	if ((revision & DWC3_GSNPSID_MASK) != 0x55330000) {
		puts("this is not a DesignWare USB3 DRD Core\n");
		return -1;
	}

	dwc3_core_soft_reset(dwc3_reg);

	dwc3_hwparams1 = readl(&dwc3_reg->g_hwparams1);

	reg = readl(&dwc3_reg->g_ctl);
	reg &= ~DWC3_GCTL_SCALEDOWN_MASK;
	reg &= ~DWC3_GCTL_DISSCRAMBLE;
	switch (DWC3_GHWPARAMS1_EN_PWROPT(dwc3_hwparams1)) {
	case DWC3_GHWPARAMS1_EN_PWROPT_CLK:
		reg &= ~DWC3_GCTL_DSBLCLKGTNG;
		break;
	default:
		debug("No power optimization available\n");
	}

	/*
	 * WORKAROUND: DWC3 revisions <1.90a have a bug
	 * where the device can fail to connect at SuperSpeed
	 * and falls back to high-speed mode which causes
	 * the device to enter a Connect/Disconnect loop
	 */
	if ((revision & DWC3_REVISION_MASK) < 0x190a)
		reg |= DWC3_GCTL_U2RSTECN;

	writel(reg, &dwc3_reg->g_ctl);

	return 0;
}

void dwc3_set_fladj(struct dwc3 *dwc3_reg, u32 val)
{
	setbits_le32(&dwc3_reg->g_fladj, GFLADJ_30MHZ_REG_SEL |
			GFLADJ_30MHZ(val));
}

struct dwc3_priv {
	struct xhci_ctrl ctrl;	/* should be the first member */
	void __iomem *regs;
};

static int dwc3_probe(struct udevice *dev)
{
	struct dwc3_priv *priv = dev_get_priv(dev);
	struct xhci_hccr *hccr;
	struct xhci_hcor *hcor;
	fdt_addr_t base;
	int ret;

	base = dev_get_addr(dev);
	if (base == FDT_ADDR_T_NONE)
		return -EINVAL;

	priv->regs = map_sysmem(base, SZ_32K);
	if (!priv->regs)
		return -ENOMEM;

	hccr = priv->regs;

	hcor = priv->regs + HC_LENGTH(xhci_readl(&hccr->cr_capbase));

	ret = dwc3_core_init(priv->regs + DWC3_REG_OFFSET);
	if (ret) {
		puts("XHCI: failed to initialize controller\n");
		return ret;
	}

	/* We are hard-coding DWC3 core to Host Mode */
	dwc3_set_mode(priv->regs + DWC3_REG_OFFSET, DWC3_GCTL_PRTCAP_HOST);

	return xhci_register(dev, hccr, hcor);
}

static int dwc3_remove(struct udevice *dev)
{
	struct dwc3_priv *priv = dev_get_priv(dev);

	xhci_deregister(dev);
	unmap_sysmem(priv->regs);

	return 0;
}

static const struct udevice_id of_dwc3_match[] = {
	{ .compatible = "snps,dwc3" },
	{ .compatible = "synopsys,dwc3" },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(dwc3) = {
	.name = "dwc3",
	.id = UCLASS_USB,
	.of_match = of_dwc3_match,
	.probe = dwc3_probe,
	.remove = dwc3_remove,
	.ops = &xhci_usb_ops,
	.priv_auto_alloc_size = sizeof(struct dwc3_priv),
	.flags	= DM_FLAG_ALLOC_PRIV_DMA,
};
