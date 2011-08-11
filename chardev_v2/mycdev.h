/*
 * This file contains macros and data types for communication with the mycdev.
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

#ifndef _MYCDEV_H
#define _MYCDEV_H

struct mycdev_ctl {
	unsigned int dev_size;
	unsigned int dev_rindex;
	char data[1024];
};

#define MYCDEV_MAGIC 'C'
/* mycdev supported ioctl commands */
#define MYCDEV_G_DATA _IOR(MYCDEV_MAGIC, 0, struct mycdev_ctl)
#define MYCDEV_S_DATA _IOW(MYCDEV_MAGIC, 1, struct mycdev_ctl)
#define MYCDEV_FLUSH  _IOW(MYCDEV_MAGIC, 2, struct mycdev_ctl)

#endif /* _MYCDEV_H */
