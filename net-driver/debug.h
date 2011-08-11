/*
 * debug.h - Debug symbols for my ethernet driver.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 */

#ifndef _DEBUG_H
#define _DEBUG_H


#ifdef DEBUG_FUNC
#define entry_info()	printk(KERN_INFO "[%s:%d] : Entry info\n", __func__, __LINE__)
#define exit_info()	printk(KERN_INFO "[%s:%d] : Exit info\n", __func__, __LINE__)

#define err(fmt, arg...) printk(KERN_ERR "[%s:%d] : "fmt"\n",		\
		__func__, __LINE__, ##arg)
#define info(fmt, arg...) printk(KERN_INFO "[%s:%d] : "fmt"\n",		\
		__func__, __LINE__, ##arg)
#define warn(fmt, arg...) printk(KERN_WARNING "[%s:%d] : "fmt"\n",	\
	       	__func__, __LINE__, ##arg)
#else
#define err(fmt, arg...)	do {} while (0)
#define info(fmt, arg...)	do {} while (0)
#define warn(fmt, arg...)	do {} while (0)
#define entry_info()		do {} while (0)
#define exit_info()		do {} while (0)
#endif

#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILURE
#define FAILURE -1
#endif

#endif /* _DEBUG_H */
