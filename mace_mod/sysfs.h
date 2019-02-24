//
// Initial MACE implementation
//
// 2019, Chris Misa
//

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/string.h>

#include <linux/fs.h>

#ifndef MACE_SYSFS_H
#define MACE_SYSFS_H

#define MACE_SYSFS_NAME "mace"
#define MACE_SYSFS_PARENT kernel_kobj


//
// Report latency approximations in nano seconds
//
__always_inline static long long unsigned
mace_cycles_to_ns(long long unsigned c)
{
  // Cycles / (1 000 Cycles / sec) * (1 000 000 000 nsec / sec)
  return (c * 1000000) / tsc_khz;
}

//
// Mace kobj entry
//
static struct kobject *mace_kobj;
static int mace_init_sysfs(void);
static void mace_free_sysfs(void);

//
// Latencies file
//
static ssize_t show_latencies(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t store_latencies(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static struct kobj_attribute mace_latencies = __ATTR(latencies_ns, 0660, show_latencies, store_latencies);

#endif
