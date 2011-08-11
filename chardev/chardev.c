/*
 * chardev.c Simple character device driver.
 *
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
  
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

#include <asm/uaccess.h> /* for get_user and put_user */

#include "chardev.h"

#define DEVICE_NAME "mydevice"
#define DRIVER_DESC "Simple pseudo driver"

#define INFO(...) printk(KERN_INFO __VA_ARGS__);
#define ALERT(...) printk(KERN_ALERT __VA_ARGS__);

#define BUF_LEN 80       /* Max length of the message from the device */

static int mydevice_open(struct inode *, struct file *);
static int mydevice_release(struct inode *, struct file *);
static ssize_t mydevice_read(struct file *, char *, size_t, loff_t *);
static ssize_t mydevice_write(struct file *, const char *, size_t, loff_t *);
static int mydevice_ioctl(struct inode *, struct file *, unsigned int, unsigned long);

static int Device_open = 0;

struct cdev *mycdev;
static int major;
static dev_t dev = MKDEV(0, 0);
static char mydevice_buffer[BUF_LEN];
static char *mydevice_buffptr;

static struct class *mydevice_class;
static struct file_operations fops = {
	.read	= mydevice_read,
	.write	= mydevice_write,
	.open	= mydevice_open,
	.ioctl	= mydevice_ioctl,
	.release	= mydevice_release
};

static int 
mydevice_open(struct inode *inodp, struct file *filp)
{
	INFO("In device open implementation\n");

	if (Device_open)
		return -EBUSY;

	Device_open++;

	//sprintf(mydevice_buffer, "Initial device message\n");
	/* Assigning buffer pointer to buffer address. */
	mydevice_buffptr = mydevice_buffer;

	try_module_get(THIS_MODULE);
	return SUCCESS;
}

static ssize_t 
mydevice_read(struct file *filp, char *buff, size_t count, loff_t *offp)
{
	ssize_t bytes_read = 0;

	INFO("In read implementation\n");
	INFO("Count : %ld\n", count);
	//bytes_read = copy_to_user(buff, mydevice_buffer, count);
	if (*mydevice_buffptr == 0)
		return 0;

	while (count && *mydevice_buffptr)
	{
		put_user(*(mydevice_buffptr++), buff++);

		count--;
		bytes_read++;
	}

	return bytes_read;
}

static ssize_t
mydevice_write(struct file *filp, const char *buff, size_t count, loff_t *offp)
{
	ssize_t bytes_written = 0;

	INFO("In write implementation\n");
	//bytes_written = copy_from_user(mydevice_buffer, buff, count);
	
	while (count)
	{
		get_user(*(mydevice_buffptr++), buff++);

		count--;
		bytes_written++;
	}

	*(mydevice_buffptr) = '\0';

	return bytes_written;
}

static int
mydevice_ioctl(struct inode *inodp, struct file *filp, unsigned int cmd, unsigned long arg)
{
	Descrpt_t	*Desc = (Descrpt_t *)arg;

	INFO("In ioctl implementation\n");

	switch(cmd)
	{
		case CHD_NONE:
			INFO("CHD_NONE command\n");
			break;
		case CHD_FLUSH:
			INFO("CHD_FLUSH command\n");
			mydevice_buffer[0] = '\0';
			break;
		case CHD_INFO:
			INFO("CHD_INFO command\n");
			sprintf(mydevice_buffer, DRIVER_DESC);
			sprintf(Desc->desc, DRIVER_DESC);
			break;
		default:
			ALERT("Command unknown\n");
			return -EINVAL;
			break;
	}

	return 0;
}

static int 
mydevice_release(struct inode *inodp, struct file *filp)
{
	INFO("In device release implementation\n");

	/* Decrement the usage count */
	Device_open--;
	module_put(THIS_MODULE);
	return SUCCESS;
}

static int __init mydevice_init(void)
{
	mycdev = cdev_alloc();

	alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
	major = MAJOR(dev);

	INFO("Registered %d \n", major);

	cdev_init(mycdev, &fops);
	mycdev->ops = &fops;
	mycdev->owner = THIS_MODULE;
	cdev_add(mycdev, dev, 1);

	/* create a struct class structure */
	mydevice_class = class_create(THIS_MODULE, DEVICE_NAME);
	/* creates a device and registers it with sysfs */
	device_create(mydevice_class, NULL, MKDEV(major,0), "%s", DEVICE_NAME);

	return 0;
}

static void __exit mydevice_exit(void)
{

	INFO("Un Registered");
	cdev_del(mycdev);
	unregister_chrdev_region(dev, 1);
	device_destroy(mydevice_class, MKDEV(major, 0));
	class_unregister(mydevice_class);
	class_destroy(mydevice_class);
}

module_init(mydevice_init);
module_exit(mydevice_exit);

MODULE_AUTHOR("Faisal Hassan");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
