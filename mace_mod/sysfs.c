//
// Initial MACE implementation
//
// 2019, Chris Misa
//

#include "sysfs.h"
#include "ring_buffer.h"

MODULE_LICENSE("GPL");

static int
mace_init_sysfs(void)
{
  int err = 0;

  mace_kobj = kobject_create_and_add(MACE_SYSFS_NAME, MACE_SYSFS_PARENT);
  if (!mace_kobj) {
    printk(KERN_INFO "Mace: Failed to create mace sysfs entry.\n");
    return -ENOMEM;
  }

  err = sysfs_create_file(mace_kobj, &mace_latencies.attr);
  if (err) {
    printk(KERN_INFO "Mace: Failed to create latencies sysfs file.\n");
  }
  return err;
}

static void
mace_free_sysfs(void)
{
  kobject_put(mace_kobj);
}

static ssize_t
show_latencies(struct kobject *kobj,
               struct kobj_attribute *attr,
               char *buf)
{
  ssize_t offset = 0;
  struct mace_latency_event *lat;
  
  while ((lat = mace_pop_event()) != NULL && offset < PAGE_SIZE) {
    offset += snprintf(buf + offset,
                       PAGE_SIZE - offset, 
                       "%s: %llu\n",
                       mace_latency_type_str(lat->type),
                       mace_cycles_to_ns(lat->latency));
  }
  return offset;
}

static ssize_t
store_latencies(struct kobject *kobj,
                struct kobj_attribute *attr,
                const char *buf,
                size_t count)
{
  if (count) {
    mace_buffer_clear();
  }
  return count;
}
