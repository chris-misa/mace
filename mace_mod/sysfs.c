//
// Initial MACE implementation
//
// 2019, Chris Misa
//

#include "sysfs.h"

#define MACE_SYSFS_NAME "mace"
#define MACE_SYSFS_PARENT kernel_kobj

//
// Report latency approximations in nano seconds
// Cycles / (1 000 Cycles / sec) * (1 000 000 000 nsec / sec)
//
#define mace_cycles_to_ns(c) (((c) * 1000000) / tsc_khz)

//
// Mace kobj entry
//
static struct kobject *mace_kobj;

//
// Latencies file
//
static ssize_t show_latencies(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t store_latencies(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static struct kobj_attribute mace_latencies = __ATTR(latencies_ns, 0660, show_latencies, store_latencies);

//
// Inner devices file
//
static ssize_t show_inner_devs(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t store_inner_devs(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static struct kobj_attribute mace_inner_devs = __ATTR(inner_devices, 0660, show_inner_devs, store_inner_devs);

int
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

  err = sysfs_create_file(mace_kobj, &mace_inner_devs.attr);
  if (err) {
    printk(KERN_INFO "Mace: Failded to create inner_devices sysfs file.\n");
  }
  return err;
}

void
mace_free_sysfs(void)
{
  kobject_put(mace_kobj);
}

//
// latencies_ns implementation
//

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

//
// inner_devices implementation
//

static ssize_t
show_inner_devs(struct kobject *kobj,
                struct kobj_attribute *attr,
                char *buf)
{
  ssize_t offset = 0;
  struct net *cur_netns = current->nsproxy->net_ns;
  struct net_device *device;

  // Only list activated devices in the current network namespace
  list_for_each_entry(device, &cur_netns->dev_base_head, dev_list) {
    if (offset >= PAGE_SIZE) {
      break;
    }
    if (mace_in_set(device->ifindex, inner_devs)) {
      offset += snprintf(buf + offset,
                         PAGE_SIZE - offset,
                         "%s (%d)\n",
                         device->name,
                         device->ifindex);
    }
  }
  return offset;
}

static ssize_t
store_inner_devs(struct kobject *kobj,
                 struct kobj_attribute *attr,
                 const char *buf,
                 size_t count)
{
  int dev_id;
  struct net *cur_netns = current->nsproxy->net_ns;
  struct net_device *device;
  int found = 0;
  int remove = 0;


  if (kstrtoint(buf, 0, &dev_id) == 0) {

    if (dev_id == 0) {
      printk(KERN_INFO "Mace: pid %d: loopback device 0 not supported.\n",
          current->pid);
      return count;
    } else if (dev_id < 0) {
      dev_id = -dev_id;
      remove = 1;
    }

    // Only allow devices in the current network namespace
    list_for_each_entry(device, &cur_netns->dev_base_head, dev_list) {
      if (dev_id == device->ifindex) {
        found = 1;
        break;
      }
    }
    if (found) {
      if (remove) {
        mace_remove_set(dev_id, inner_devs);
        printk(KERN_INFO "Mace: pid %d removed device id %d\n",
            current->pid,
            dev_id);
      } else {
        mace_add_set(dev_id, inner_devs);
        printk(KERN_INFO "Mace: pid %d added device id %d\n",
            current->pid,
            dev_id);
      }
    } else {
      printk(KERN_INFO "Mace: pid %d trying to access invalid device id %d\n",
        current->pid,
        dev_id);
    }
  }
  return count;
}
