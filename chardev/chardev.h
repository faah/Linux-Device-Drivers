/*
 * chardev.h  Header for simple character device driver.
 *
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#ifndef _CHARDEV_H	/* If CHARDEV.H not already defined */
#define _CHARDEV_H

#define SUCCESS	0
#define FAILURE	-1

#define IOC_MAGIC 'k'

/* ioctl commands */
#define CHD_NONE	_IO(IOC_MAGIC, 0)
#define CHD_FLUSH	_IOW(IOC_MAGIC, 1, int)
#define CHD_INFO	_IOR(IOC_MAGIC, 2, int)

typedef struct __descrpt {
	char desc[80];
} Descrpt_t;

#endif		/* _CHARDEV_H */
