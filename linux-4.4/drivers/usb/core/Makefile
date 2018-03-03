#
# Makefile for USB Core files and filesystem
#

usbcore-y := usb.o hub.o hcd.o urb.o message.o driver.o
usbcore-y += config.o file.o buffer.o sysfs.o endpoint.o
usbcore-y += devio.o notify.o generic.o quirks.o devices.o
usbcore-y += port.o

usbcore-$(CONFIG_PCI)		+= hcd-pci.o
usbcore-$(CONFIG_ACPI)		+= usb-acpi.o

### oc_ind ###
usbcore-$(CONFIG_USB_UNIPHIER_WA_OC_DETECT) += sysfs_oc_ind.o

### port_ctrl, port_status ###
usbcore-$(CONFIG_USB_UNIPHIER_WA_PORT_CONTROL) += sysfs_port.o

obj-$(CONFIG_USB)		+= usbcore.o
