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
#include <linux/uio.h>

#include <linux/slab.h>

#include <uapi/linux/ip.h>
#include <uapi/linux/icmp.h>
#include <uapi/linux/if_ether.h>

#include <asm/syscall.h>

#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>

#include <linux/list.h>
#include <linux/types.h>
#include <asm/atomic.h>

#include "ring_buffer.h"
#include "dev_file.h"
#include "namespace_set.h"
#include "macros.h"

//
// Uncomment this macro to get lots of printk information and terrible performance
//

//
// Rough synchronization of tsc to gettimeofday for later correlation
//
void mace_tsc_offset_resync(void);
void mace_tsc_to_gettimeofday(unsigned long long tsc_val, struct timeval *tv);

struct mace_latency {
  unsigned long long enter;
  struct mace_namespace_entry *ns;
  int valid;
  u64 key;
  spinlock_t lock;
};

struct mace_perturbation {
  unsigned long long sum;
  unsigned long long count;
};

#define START_PERT_TIMER(timer_name) \
  unsigned long long timer_name = rdtsc();

#define STOP_PERT_TIMER(timer_name, target_pert) \
  (target_pert).sum += rdtsc() - timer_name; \
  (target_pert).count++;

#endif
