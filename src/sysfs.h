//
// Initial MACE implementation
//
// 2019, Chris Misa
//

#ifndef MACE_SYSFS_H
#define MACE_SYSFS_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/string.h>

#include <linux/netdevice.h>
#include <linux/net_namespace.h>

#include "ring_buffer.h"
#include "macros.h"

int mace_init_sysfs(void);
void mace_free_sysfs(void);


#endif
