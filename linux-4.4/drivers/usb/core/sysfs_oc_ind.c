/*
 * Copyright (C) 2016, Socionext Inc.
 *                       All Rights Reserved.
 *
 */

#if defined(CONFIG_USB_UNIPHIER_WA_OC_DETECT)

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/usb/quirks.h>
#include "usb.h"

#define assert(s) do{if (!(s)) panic(#s);} while(0);

void usb_initialize_oc( struct usb_device *udev )
{
	spin_lock_init( &udev->oc_ind_lock );
}

void usb_finish_oc( struct usb_device *udev )
{
	struct st_oc_ind	*oc_ind, *next;
	unsigned long		flags;

	spin_lock_irqsave( &udev->oc_ind_lock, flags );
	{
		for( oc_ind = udev->oc_ind ; oc_ind ; oc_ind = next ){
			next = oc_ind->next;
			kfree( oc_ind );
		}
		udev->oc_ind = NULL;
	}
	spin_unlock_irqrestore( &udev->oc_ind_lock, flags );
}

static struct st_oc_ind* create_new_oc_ind( struct usb_device* udev, int port, const char* msg )
{
	struct st_oc_ind	*oc_ind;

	oc_ind = udev->oc_ind;

	if( oc_ind == NULL )
		return udev->oc_ind = kzalloc( sizeof(struct st_oc_ind), GFP_ATOMIC );

	for( ; ; oc_ind = oc_ind->next ){
		if( oc_ind->port == port && strcmp(oc_ind->msg,msg) == 0 )
			return NULL;
		if( oc_ind->next == NULL ) break;
	}

	return oc_ind->next = kzalloc( sizeof(struct st_oc_ind), GFP_ATOMIC );
}

void usb_notify_oc( struct usb_device *udev, int port, u32 status )
{
	struct st_oc_ind	*oc_ind;
	unsigned long		flags;
	const char			*msg;

	if( !udev ) return;

	msg = udev->dev.kobj.name;
	assert( msg != NULL );
	while( udev->parent != NULL ) udev = udev->parent;

/*_ 	printk( "notify_oc %s : %d\n", msg, port ); _*/

	spin_lock_irqsave( &udev->oc_ind_lock, flags );
	{
		oc_ind = create_new_oc_ind( udev, port, msg );
		if( oc_ind ){
			oc_ind->port = port;
			oc_ind->status = status;
			assert( strlen(msg) < sizeof(oc_ind->msg) );
			sprintf( oc_ind->msg, "%s", msg );
		}
	}
	spin_unlock_irqrestore( &udev->oc_ind_lock, flags );
}

static ssize_t show_oc_ind(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_device	*udev;
	struct st_oc_ind	*oc_ind;
	unsigned long		flags;
	ssize_t				sz;

	udev = to_usb_device(dev);

	if( likely(!udev->oc_ind) ) return 0;

	spin_lock_irqsave( &udev->oc_ind_lock, flags );
	{
		oc_ind = udev->oc_ind;
		if( likely(oc_ind) )
			udev->oc_ind = oc_ind->next;
	}
	spin_unlock_irqrestore( &udev->oc_ind_lock, flags );

	if( unlikely(!oc_ind) ) return 0;

	sz = sprintf( buf, "%s %d %08x\n", oc_ind->msg, oc_ind->port, oc_ind->status );
	kfree( oc_ind );

	return sz;
}
static DEVICE_ATTR(oc_ind, S_IRUGO, show_oc_ind, NULL);

static struct attribute *dev_oc_ind_attrs[] = {
	&dev_attr_oc_ind.attr,
	NULL
};

static umode_t dev_oc_ind_attrs_are_visible(struct kobject *kobj, struct attribute *a, int n)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct usb_device *udev = to_usb_device(dev);

	if( a == &dev_attr_oc_ind.attr ){
		if( udev->parent != NULL )
			return 0;
	}
	return a->mode;
}

const struct attribute_group dev_oc_ind_attr_grp = {
	.attrs =		dev_oc_ind_attrs,
	.is_visible =	dev_oc_ind_attrs_are_visible,
};

#endif /* CONFIG_USB_UNIPHIER_WA_OC_DETECT */
