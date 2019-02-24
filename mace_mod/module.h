//
// Initial MACE module work
//
// Defines and macros to implement the inter-tracepoint logic.
//
// 2019, Chris Misa
//

#ifndef MACE_MODULE_H
#define MACE_MODULE_H

#define LINUX

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/string.h>

#include <linux/tracepoint.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/socket.h>
#include <asm/msr.h>


#include <linux/fs.h>
#include <asm/uaccess.h>

#include <linux/slab.h>

#include <uapi/linux/ip.h>
#include <uapi/linux/icmp.h>
#include <uapi/linux/if_ether.h>

#include <asm/syscall.h>

#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>

#include <linux/hashtable.h>

#include "ring_buffer.h"
#include "sysfs.h"
#include "macros.h"

// #define DEBUG

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris Misa <cmisa@cs.uoregon.edu>");
MODULE_DESCRIPTION("Test of sysfs file system");

int __init mace_mod_init(void);
void __exit mace_mod_exit(void);
module_init(mace_mod_init);
module_exit(mace_mod_exit);

// Param: Outer device id
static int outer_dev = -1;
module_param(outer_dev, int, 0);
MODULE_PARM_DESC(outer_dev, "Device id of the outer device");

// Param: Inner device id (for now. . .)
static int inner_dev = -1;
module_param(inner_dev, int, 0);
MODULE_PARM_DESC(inner_dev, "Device id of inner devie");

// Syscall numbers. . .waiting for a better day
#define SYSCALL_SENDTO 44
#define SYSCALL_RECVMSG 47

#endif
