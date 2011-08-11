/*
 * myeth_dev.c -Simple network driver.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/moduleparam.h>

#include <linux/sched.h>
#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>         /* kmalloc() */
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/interrupt.h>    /* mark_bh */

#include <linux/in.h>
#include <linux/netdevice.h>   /* struct device, and other headers */
#include <linux/etherdevice.h> /* eth_type_trans */
#include <linux/ip.h>          /* struct iphdr */
#include <linux/tcp.h>         /* struct tcphdr */
#include <linux/skbuff.h>

#include "debug.h"

#define MAX_MYETH_DEVS	5

static struct net_device *myeth[MAX_MYETH_DEVS];

static int myethdevs = 2;
module_param(myethdevs, int, 0);
MODULE_PARM_DESC(myethdevs, " Number of ethernet interfaces");

struct myeth_priv {
	int status;
};

static int myeth_dev_init(struct net_device *dev)
{
	entry_info();
	exit_info();
	return 0;
}

/* Configuration changes (passed on by ifconfig) */
static int myeth_config(struct net_device *dev, struct ifmap *map)
{
	entry_info();
	if (dev->flags & IFF_UP) /* can't act on a running interface */
		return -EBUSY;
	
	/* Don't allow changing the I/O address */
	if (map->base_addr != dev->base_addr) {
		printk(KERN_WARNING "mynetd: Can't change I/O address\n");
		return -EOPNOTSUPP;
	}

	/* Allow changing the IRQ */
	if (map->irq != dev->irq) 
		dev->irq = map->irq;
	
	exit_info();
	/* ignore other fields */
	return 0;
}

static int myeth_open(struct net_device *dev)
{
	int i;

	entry_info();
	memcpy(dev->dev_addr, "\0BADF0", ETH_ALEN);
	for (i = 0; i < myethdevs; i++) {
		if (dev == myeth[i])
			dev->dev_addr[ETH_ALEN-1] += i;
	}

	/* Tells the kernel that the driver is ready to send packets. */
	netif_start_queue(dev);
	exit_info();
	return 0;
}

static int myeth_close(struct net_device *dev)
{
	entry_info();
	/* Tells the kernel to stop sending packets. */
	netif_stop_queue(dev);
	exit_info();
	return 0;
}

static netdev_tx_t myeth_xmit(struct sk_buff *skb, struct net_device *dev)
{
	entry_info();
	info("In transmit");
	exit_info();
	return NETDEV_TX_OK;
}

static int myeth_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	return 0;
}

/* Set mac address. */
static int myeth_set_address(struct net_device *dev, void *p)
{
	struct sockaddr *sa = p;
	
	if (!is_valid_ether_addr(sa->sa_data))
		return -EADDRNOTAVAIL;
	
	memcpy(dev->dev_addr, sa->sa_data, ETH_ALEN);
	return 0;
}

static struct net_device_ops myeth_ops = {
	.ndo_init		= myeth_dev_init,
	.ndo_open		= myeth_open,
	.ndo_stop		= myeth_close,
	.ndo_start_xmit		= myeth_xmit,
	.ndo_set_config		= myeth_config,
	.ndo_do_ioctl		= myeth_ioctl,
	.ndo_set_mac_address	= myeth_set_address,
};

static void myeth_setup(struct net_device *dev)
{
	struct myeth_priv *priv;

	entry_info();
	ether_setup(dev);

	dev->flags	|= IFF_NOARP;
	dev->features	|= NETIF_F_NO_CSUM;
	dev->netdev_ops	= &myeth_ops;

	/* Initialize the priv field. */
	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct myeth_priv));
	exit_info();
	return;
}

static int __init myeth_init(void)
{
	int i;

	entry_info();
	if (myethdevs > MAX_MYETH_DEVS)
		myethdevs = MAX_MYETH_DEVS;

	for (i = 0; i < myethdevs; i++) {
		myeth[i] = alloc_netdev(sizeof(struct myeth_priv), "myeth%d", myeth_setup);
		if (myeth[i] == NULL) {
			err("alloc_netdev failed");
			return -ENOMEM;
		}
	}

	for (i = 0; i < myethdevs; i++)
		register_netdev(myeth[i]);

	exit_info();
	return 0;
}

static void __exit myeth_exit(void)
{
	int i;

	entry_info();
	for (i = 0; i < myethdevs; i++) {
		unregister_netdev(myeth[i]);
		free_netdev(myeth[i]);
	}
	exit_info();
}

module_init(myeth_init);
module_exit(myeth_exit);

MODULE_LICENSE("GPLv2");
MODULE_AUTHOR("Faisal Hassan <faah87@gmail.com>");
