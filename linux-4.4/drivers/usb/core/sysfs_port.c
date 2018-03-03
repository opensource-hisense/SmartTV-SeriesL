/*
 * Copyright (C) 2016, Socionext Inc.
 *                       All Rights Reserved.
 *
 */

#if defined(CONFIG_USB_UNIPHIER_WA_PORT_CONTROL)

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/usb.h>
#include <linux/usb/quirks.h>
#include "usb.h"
#if defined(CONFIG_USB_UNIPHIER_WA_EHCI_COMPLIANCE_TEST_MODE) || \
    defined(CONFIG_USB_UNIPHIER_WA_XHCI_COMPLIANCE_TEST_MODE)
#include <linux/slab.h>
#include <linux/usb/ch11.h>
#endif

#ifdef CONFIG_USB_UNIPHIER_WA_XHCI_COMPLIANCE_TEST_MODE_DWC3
#include <linux/io.h>
#include <linux/usb/hcd.h>
#define GUSB3PIPECTL_OFST(n)	(0xc2c0 + ((n)*0x04))	/* offset from xHCI CAPLENGTH */
#define HSTPRTCMPL_BIT		(1 << 30)

static int force_ss_compliance_mode(struct usb_device *udev, u32 port1)
{
	struct usb_hcd		*hcd;
	void __iomem		*glb_reg;
	u32			port0 = port1 - 1;	/* Port 0, 1, .. */
	u32			temp;
	int			ret = 0;

	if (udev->speed < USB_SPEED_SUPER) {
		dev_err(&udev->dev, "SS Compliance Mode is not supported for this root hub.\n");
		return -EINVAL;
	}
	if (port1 == 0 || port1 > (udev->maxchild)) {
		dev_err(&udev->dev, "Port %d doesn't exist.\n", port1);
		return -EINVAL;
	}

	if (!(udev->dev.parent)) {
		/* failed to find USB HC device */
		return -EINVAL;
	}
	hcd = dev_get_drvdata(udev->dev.parent);
	if (!hcd) {
		/* failed to find USB HC driver (usb_hcd) */
		return -EINVAL;
	}
	if (!(hcd->regs)) {
		/* failed to find USB HC driver register */
		return -EINVAL;
	}

	/* remap DWC3 GUSB3PIPECTL(port) register */
	glb_reg = ioremap_nocache(hcd->rsrc_start + GUSB3PIPECTL_OFST(port0), 4);

	/* GUSB3PIPECTL.HSTPRTCMPL==0? */
	/* If true, this is the 1st call of force_ss_compliance_mode() */
	temp = readl(glb_reg);
	if ((temp & HSTPRTCMPL_BIT) == 0) {
		/* 1. Clear PORTSC.PP */
		/* (This step is only for the 1st call of force_ss_compliance_mode()) */
		usb_disable_autosuspend(udev);
		dev_info(&udev->dev, "Disabled autosuspend.\n");
		ret= usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
					USB_REQ_CLEAR_FEATURE, USB_RT_PORT,
					USB_PORT_FEAT_POWER,
					port1,
					NULL, 0, 1000);
		if (ret < 0){
			goto done;
		}
		dev_info(&udev->dev, "Port Power bit cleared at port %d.\n", port1);
	} else {
		/* 2. Clear GUSB3PIPECTL.HSTPRTCMPL */
		/* (This step is only for the 2nd, 3rd, .. call of force_ss_compliance_mode()) */
		temp &= ~HSTPRTCMPL_BIT;
		writel(temp, glb_reg);
		dev_info(&udev->dev, "Force SS Conpliance bit cleared at port %d.\n", port1);
	}
	/* 3. Set GUSB3PIPECTL[30].HSTPRTCMPL */
	temp = readl(glb_reg);
	temp |= HSTPRTCMPL_BIT;
	writel(temp, glb_reg);
	dev_info(&udev->dev, "Force SS Conpliance bit set at port %d.\n", port1);

done:
	iounmap(glb_reg);
	return ret;
}
#endif /* CONFIG_USB_UNIPHIER_WA_XHCI_COMPLIANCE_TEST_MODE_DWC3 */

static ssize_t show_port_status(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device	*udev;
	ssize_t				sz;
	int					i, ret, portNum;
	u32					status;

	sz = 0;
	udev = to_usb_device(dev);

	ret = usb_get_hub_status( udev, &status );
/*_ 	printk( "%s:%d ret=%d\n", __FUNCTION__, __LINE__, ret ); _*/
	if( ret < 0 )
		return -EINVAL;
	ret = sprintf( &buf[sz], "%u %08X\n", 0, status );
	if( ret < 0 )
		return -EINVAL;
	sz += ret;

	portNum = udev->maxchild;
	for( i=1 ; i <= portNum ; i++ ){
		if( usb_get_port_status( udev, i, &status ) < 0 )
			return -EINVAL;

		ret = sprintf( &buf[sz], "%u %08X\n", i, status );
		if( ret < 0 )
			return -EINVAL;
		sz += ret;
	}

	return sz;
}
static DEVICE_ATTR(port_status, S_IRUGO, show_port_status, NULL);

static ssize_t set_port_ctrl(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_device* udev;
	u32 flag, status, port;
	int ret;
#if defined(CONFIG_USB_UNIPHIER_WA_EHCI_COMPLIANCE_TEST_MODE) || \
    defined(CONFIG_USB_UNIPHIER_WA_XHCI_COMPLIANCE_TEST_MODE)
	unsigned	selector;
	struct usb_device* port_dev;
	struct usb_device_descriptor *dev_desc_buf;
#endif

	udev = to_usb_device(dev);

	ret = sscanf( buf, "%x %x %x \n", &flag, &status, &port );
	if( (ret != 3) || (port == 0) )
		return -EINVAL;

#ifdef CONFIG_USB_UNIPHIER_WA_XHCI_COMPLIANCE_TEST_MODE
	selector = port >> 8;
	if (status == USB_PORT_FEAT_LINK_STATE && selector == 0xA) { /* Enable Compliance Mode (for SS) */
		if ((port & 0xFF) == 0) {
			ret = usb_ss_compliance_transition_enable_all(udev);
		} else {
			ret = usb_ss_compliance_transition_enable(udev, (port & 0xFF));
		}
		if (ret < 0){
			dev_err(&udev->dev, "failed with error %d\n",ret);
			return ret;
		}
		return count;
	}
#ifdef CONFIG_USB_UNIPHIER_WA_XHCI_COMPLIANCE_TEST_MODE_DWC3
	if (status == USB_PORT_FEAT_LINK_STATE && selector == 0xC) {
		/* Not for USB/xHCI spec. DWC3 GUSB3PIPECTL force compliance mode. */
		ret = force_ss_compliance_mode(udev, (port & 0xFF));
		if (ret < 0){
			dev_err(&udev->dev, "failed with error %d\n",ret);
			return ret;
		}
		return count;
	}
#endif /* CONFIG_USB_UNIPHIER_WA_XHCI_COMPLIANCE_TEST_MODE_DWC3 */
#endif /* CONFIG_USB_UNIPHIER_WA_XHCI_COMPLIANCE_TEST_MODE */

#if defined(CONFIG_USB_UNIPHIER_WA_EHCI_COMPLIANCE_TEST_MODE) || \
    defined(CONFIG_USB_UNIPHIER_WA_XHCI_COMPLIANCE_TEST_MODE)
	selector = port >> 8;
	switch (selector) {
	case 0x6: /* TEST_SINGLE_STEP_GET_DEV_DESC */
		msleep(15 * 1000);
		dev_desc_buf = kmalloc(USB_DT_DEVICE_SIZE, GFP_KERNEL);
		if (!dev_desc_buf){
			dev_err(&udev->dev, "failed with error %d\n",-ENOMEM);
			return -ENOMEM;
		}

		port_dev = usb_hub_find_child(udev, (port & 0xFF));
		if(!port_dev){
			dev_err(&udev->dev, "failed with error %d\n",-ENODEV);
			return -ENODEV;
		}

		ret = usb_control_msg(port_dev, usb_rcvctrlpipe(port_dev, 0),
					USB_REQ_GET_DESCRIPTOR, USB_DIR_IN,
					USB_DT_DEVICE << 8, 0,
					dev_desc_buf, USB_DT_DEVICE_SIZE,
					USB_CTRL_GET_TIMEOUT);
		kfree(dev_desc_buf);
		if (ret < 0){
			dev_err(&udev->dev, "failed with error %d\n",ret);
			return ret;
		}
		return count;
	case 0x7: /* TEST_SINGLE_STEP_GET_DEV_DESC_DATA */
		if (udev != udev->bus->root_hub) {
			dev_err(&udev->dev, "SINGLE_STEP_SET_FEATURE test only supported on root hub\n");
			return -EINVAL;
		}

		ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
					USB_REQ_SET_FEATURE, USB_RT_PORT,
					USB_PORT_FEAT_TEST,
					(6 << 8) | (port & 0xFF),
					NULL, 0, 60 * 1000);
		if (ret < 0){
			dev_err(&udev->dev, "failed with error %d\n",ret);
			return ret;
		}
		return count;
	case 0x8: /* TEST_HS_HOST_PORT_SUSPEND_RESUME */
		msleep(15 * 1000);
		ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
					USB_REQ_SET_FEATURE, USB_RT_PORT,
					USB_PORT_FEAT_SUSPEND, (port & 0xFF),
					NULL, 0, 1000);
		if (ret < 0){
			dev_err(&udev->dev, "failed with error %d\n",ret);
			return ret;
		}

		msleep(15 * 1000);
		ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
					USB_REQ_CLEAR_FEATURE, USB_RT_PORT,
					USB_PORT_FEAT_SUSPEND, (port & 0xFF),
					NULL, 0, 1000);
		if (ret < 0){
			dev_err(&udev->dev, "failed with error %d\n",ret);
			return ret;
		}
		return count;
	default:
		break;
	}
#endif

	if( flag ){
		ret = usb_set_port_status( udev, port, status );
	}else{
		ret = usb_clr_port_status( udev, port, status );
	}
	if( ret < 0 )
		return -EINVAL;

	return count;
}
static DEVICE_ATTR(port_ctrl, S_IWUSR, NULL, set_port_ctrl);

static struct attribute *dev_port_attrs[] = {
	&dev_attr_port_status.attr,
	&dev_attr_port_ctrl.attr,
	NULL
};

static umode_t dev_port_attrs_are_visible(struct kobject *kobj, struct attribute *a, int n)
{
/*_ 	struct device *dev = container_of(kobj, struct device, kobj); _*/
/*_ 	struct usb_device *udev = to_usb_device(dev); _*/

	if( a == &dev_attr_port_status.attr || a == &dev_attr_port_ctrl.attr ){
		/*_ return 0; _*/
	}
	return a->mode;
}

const struct attribute_group dev_port_attr_grp = {
	.attrs =		dev_port_attrs,
	.is_visible =	dev_port_attrs_are_visible,
};

#endif /* CONFIG_USB_UNIPHIER_WA_PORT_CONTROL */
