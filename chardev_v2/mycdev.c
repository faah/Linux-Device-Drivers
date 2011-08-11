/*
 * mycdev.c -Simple chardev for learning purpose, device handle creation is 
 * done with udev.
 *
 * Copyright (C) 2011  Faisal Hassan.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>		/* Required for various structures related to files liked fops. */
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/timer.h>
#include <linux/wait.h>		/* Required for the wait queues */
#include <linux/sched.h>	/* Required for task states (TASK_INTERRUPTIBLE etc ) */
#include <asm/uaccess.h>	/* Required for copy_from and copy_to user functions */

#include "debug.h"
#include "mycdev.h" /* TODO Should move to <linux/mycdev.h> */

#define SEC 25
#define MYCDEV_LEN 1024
#define MYCDEV_MAX_MINOR 1

#define DEVICE "mycdev"
#define DRV_DESC "Simple chardev for learning"
#define DRV_VERSION "1.0"

/* my device structure */
struct mycdev {
	uint8_t *dev_rbuff; /* device read buffer, provide data to read system call */
	uint8_t *dev_wbuff; /* device write buffer, get data from write system call */
	uint32_t dev_size; /* size of the device buffer */
	uint32_t dev_rindex; /* index of read buffer */
	char dev_flag;
	struct cdev dev_cdev;
	struct semaphore dev_rsem;
	struct semaphore dev_wsem;
	struct timer_list dev_timer;
	wait_queue_head_t dev_rqueue;
};

static int Major = 0;

static void mycdev_timer(unsigned long data)
{
	struct mycdev *mycdevp = (struct mycdev *)data;

	ENTER();
	mycdevp->dev_flag = 'y';
	wake_up_interruptible(&mycdevp->dev_rqueue);
	EXIT();
}

static int mycdev_open(struct inode *inode, struct file *file)
{
	/* look up device info for this device file */
	struct mycdev *mycdevp = container_of(inode->i_cdev, struct mycdev, dev_cdev);

	ENTER();
	file->private_data = mycdevp;
	kobject_get(&mycdevp->dev_cdev.kobj); /* try_module_get? */

	/* Mark the flag as 'do not read' */
	mycdevp->dev_flag = 'n';

	EXIT();
	return 0;
}

static int mycdev_close(struct inode *inode, struct file *file)
{
	struct mycdev *mycdevp = (struct mycdev *)file->private_data;

	ENTER();
	kobject_put(&mycdevp->dev_cdev.kobj); /* try_module_put? */
	EXIT();

	return 0;
}

static ssize_t mycdev_read(struct file *file, char __user *ubuff, 
		size_t count, loff_t *offset)
{
	ssize_t ret = 0;
	struct mycdev *mycdevp = (struct mycdev *)file->private_data;

	ENTER();
	info("Try to get read lock");
	/* try to get control of the read buffer */
	if (down_trylock(&mycdevp->dev_rsem)) {
		/* somebody else has it now;
		 * if we're non-blocking, then exit...
		 */
		if (file->f_flags & O_NONBLOCK) {
			err("O_NONBLOCK specified : resource unavailable");
			return -EAGAIN;
		}
		/* ...or if we want to block, then do so here */
		if (down_interruptible(&mycdevp->dev_rsem)) {
			/* something went wrong with wait */
			return -ERESTARTSYS;
		}
	}

	info("Got read lock");
	wait_event_interruptible(mycdevp->dev_rqueue, mycdevp->dev_flag != 'n');
	info("In read wait Q");

	if (*offset >= mycdevp->dev_rindex) {
		info("EOF");
		goto out;
	}

	if ((*offset + count) >= mycdevp->dev_rindex)
		count = mycdevp->dev_rindex - *offset;

	if (copy_to_user(ubuff, mycdevp->dev_rbuff + *offset, count)) {
		ret = -EFAULT;
		goto out;
	}

	ret = count;
	*offset += ret;
out:
	/* release the read buffer and wake anyone who might be
	 * waiting for it
	 */
	up(&mycdevp->dev_rsem);
	info("*offset     : %lli", *offset);
	info("dev_rindex  : %u", mycdevp->dev_rindex);
	info("count       : %li", count);
	EXIT();
	/* return the number of characters read in */
	return ret;
}

static ssize_t mycdev_write(struct file *file, const char __user *ubuff, 
		size_t count, loff_t *offset)
{
	ssize_t ret;
	struct mycdev *mycdevp = (struct mycdev *)file->private_data;

	ENTER();

	info("Try to get write lock");
	/* try to get control of the write buffer */
	if (down_trylock(&mycdevp->dev_wsem)) {
		/* somebody else has it now;
		 * if we're non-blocking, then exit...
		 */
		if (file->f_flags & O_NONBLOCK) {
			err("O_NONBLOCK specified : resource unavailable");
			return -EAGAIN;
		}
		/* ...or if we want to block, then do so here */
		if (down_interruptible(&mycdevp->dev_wsem)) {
			/* something went wrong with wait */
			return -ERESTARTSYS;
		}
	}

	info("Got write lock");
	if (count > mycdevp->dev_size)
		count = mycdevp->dev_size - 1;

	ret = count;
	if (copy_from_user(mycdevp->dev_rbuff, ubuff, count)) {
		ret = -EFAULT;
		goto out;
	}

	mycdevp->dev_rbuff[count] = '\0';
	mycdevp->dev_rindex = count;
	info("count      : %li", count);
	info("dev_size   : %u", mycdevp->dev_size);
	info("dev_rindex : %u", mycdevp->dev_rindex);
out:
	/* release the write buffer and wake anyone who's waiting for it */
	up(&mycdevp->dev_wsem);
	EXIT();
	return ret;
}

static loff_t mycdev_llseek(struct file *file, loff_t position, int whence)
{
	ENTER();
	/* FIXME Check whence */
	file->f_pos = position;
	EXIT();

	return position;
}

static int mycdev_ioctl(struct inode *inode, struct file *file, 
		unsigned int cmd, unsigned long arg)
{
	struct mycdev_ctl *ctlp = (struct mycdev_ctl *)arg;
	struct mycdev *mycdevp = (struct mycdev *)file->private_data;
	int ret = 0;

	ENTER();
	/* Check argument valid? */
	if (ctlp == NULL) {
		ret = -EFAULT;
		goto out;
	}

	switch (cmd) {
	case MYCDEV_G_DATA:
		info("MYCDEV_G_DATA");
		ctlp->dev_size = mycdevp->dev_size;
		ctlp->dev_rindex = mycdevp->dev_rindex;
		break;
	case MYCDEV_S_DATA:
		info("MYCDEV_S_DATA");
		mycdevp->dev_size = ctlp->dev_size;
		mycdevp->dev_rindex = ctlp->dev_rindex;
		break;
	case MYCDEV_FLUSH:
		mycdevp->dev_rindex = 0;
		mycdevp->dev_size = MYCDEV_LEN;
		mycdevp->dev_rbuff[0] = '\0';
		break;
	default:
		err("Invalid ioctl command");
		ret = -EINVAL;
		goto out;
	}
out:
	EXIT();
	return ret;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = mycdev_open,
	.release = mycdev_close,
	.read = mycdev_read,
	.write = mycdev_write,
	.llseek = mycdev_llseek,
	.ioctl = mycdev_ioctl
};
static struct mycdev *mycdevp;
static struct class *dev_class;

static int __init mycdev_init(void)
{
	int ret;
	dev_t devno;

	ENTER();
	/* Assigning a major number */
	ret = alloc_chrdev_region(&devno, 0, MYCDEV_MAX_MINOR, DEVICE);
	if (ret < 0) {
		err("%s registering failed with %d", DEVICE, ret);
		return ret;
	}

	Major = MAJOR(devno);
	/* Allocate memory */
	mycdevp = kmalloc(sizeof(struct mycdev), GFP_KERNEL);
	if (NULL == mycdevp) {
		err("Couldn't allocate memory for %s", DEVICE);
		return -ENOMEM;
	}

	memset(mycdevp, 0, sizeof(struct mycdev));
	/* Create a struct class structure */
	dev_class = class_create(THIS_MODULE, DEVICE);
	if (IS_ERR(dev_class)) {
		ret = PTR_ERR(dev_class);
		goto unregister_chrdev;
	}

	/* Initialize a cdev structure */
	cdev_init(&mycdevp->dev_cdev, &fops);
	mycdevp->dev_cdev.owner = THIS_MODULE;
	/* Add char device to the kernel */
	ret = cdev_add(&mycdevp->dev_cdev, devno, 1);
	if (ret < 0) {
		err("Failed to add device");
		goto free_dev_pointer;
	}

	/* Allocate memory for device read buffer */
	mycdevp->dev_size = MYCDEV_LEN;
	mycdevp->dev_rindex = 0;
	mycdevp->dev_rbuff = kmalloc(mycdevp->dev_size, GFP_KERNEL);
	if (NULL == mycdevp->dev_rbuff) {
		err("Couldn't allocate read buffer for device");
		ret = -ENOMEM;
		goto destroy_device_class;
	}

	/* Allocate write memory for device */
	mycdevp->dev_wbuff = kmalloc(mycdevp->dev_size, GFP_KERNEL);
	if (NULL == mycdevp->dev_wbuff) {
		err("Couldn't allocate write buffer for device");
		ret = -ENOMEM;
		kfree(mycdevp->dev_rbuff);
		goto destroy_device_class;
	}

	/* Creates a device and add to sysfs */
	device_create(dev_class, NULL, devno, NULL, "%s", DEVICE);
	/* Initialize semaphore with count 1 */
	sema_init(&mycdevp->dev_rsem, 1);
	sema_init(&mycdevp->dev_wsem, 1);

	init_waitqueue_head(&mycdevp->dev_rqueue);
	mycdevp->dev_flag = 'n';
	/* Timer */
	setup_timer(&mycdevp->dev_timer, mycdev_timer, (unsigned long)mycdevp);
	ret = mod_timer(&mycdevp->dev_timer, jiffies + HZ * SEC);
	if (ret) {
		err("mod_timer() returned");
		goto destroy_device_class;
	}
	info("Timer added");

	EXIT();
	return 0;

destroy_device_class:
	class_destroy(dev_class);
free_dev_pointer:
	kfree(mycdevp);
unregister_chrdev:
	unregister_chrdev_region(devno, 1);

	EXIT();
	return ret;
}

static void __exit mycdev_exit(void)
{
	dev_t devno = MKDEV(Major, 0);

	ENTER();
	/* In reverse order of removal */
	if (del_timer(&mycdevp->dev_timer))
		err("timer still in use");

	kfree(mycdevp->dev_rbuff);
	kfree(mycdevp->dev_wbuff);
	cdev_del(&mycdevp->dev_cdev);
	device_destroy(dev_class, devno);
	class_destroy(dev_class);
	kfree(mycdevp);
	unregister_chrdev_region(devno, 1);
	EXIT();
}

module_init(mycdev_init);
module_exit(mycdev_exit);
MODULE_LICENSE("GPLv2");
MODULE_AUTHOR("Faisal Hassan <faah87@gmail.com>");
MODULE_DESCRIPTION(DRV_DESC ", v"DRV_VERSION);
MODULE_SUPPORTED_DEVICE(DEVICE);
