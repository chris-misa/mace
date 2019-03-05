//
// Initial MACE implementation
//
// 2019, Chris Misa
//

#ifndef MACE_DEV_FILE_H
#define MACE_DEV_FILE_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

#include <linux/netdevice.h>
#include <linux/net_namespace.h>

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>


#include "ring_buffer.h"
#include "macros.h"

int mace_init_dev(void);
void mace_free_dev(void);

#endif
