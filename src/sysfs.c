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

extern struct list_head mace_active_ns;

//
// Mace kobj entry
//
static struct kobject *mace_kobj;

//
// Latencies file
//
static ssize_t show_latencies(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t store_latencies(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static struct kobj_attribute mace_latencies_ns_file = __ATTR(latencies_ns, 0660, show_latencies, store_latencies);

//
// mace_on file
//
static ssize_t show_mace_on(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t store_mace_on(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static struct kobj_attribute mace_on_file = __ATTR(mace_on, 0660, show_mace_on, store_mace_on);

int
mace_init_sysfs(void)
{
  int err = 0;

  mace_kobj = kobject_create_and_add(MACE_SYSFS_NAME, MACE_SYSFS_PARENT);
  if (!mace_kobj) {
    printk(KERN_INFO "Mace: Failed to create mace sysfs entry.\n");
    return -ENOMEM;
  }

  err = sysfs_create_file(mace_kobj, &mace_latencies_ns_file.attr);
  if (err) {
    printk(KERN_INFO "Mace: Failed to create latencies sysfs file.\n");
  }

  err = sysfs_create_file(mace_kobj, &mace_on_file.attr);
  if (err) {
    printk(KERN_INFO "Mace: Failded to create mace_on sysfs file.\n");
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
  int i;
  struct mace_ring_buffer *ring_buf = mace_get_buf();
  struct mace_latency_event *lat = ring_buf->queue;
  char line_buf[128];
  ssize_t line_len;

  for (i = 0; i < MACE_EVENT_QUEUE_SIZE; i++, lat++) {
    
    line_len = snprintf(line_buf,
                        128,
                        "[%lu] %s: %llu\n",
                        lat->ns_id,
                        mace_latency_type_str(lat->type),
                        mace_cycles_to_ns(lat->latency));
    if (offset + line_len + 1 < PAGE_SIZE) {
      memcpy(buf + offset, line_buf, line_len);
      offset += line_len;
    } else {
      break;
    }
  }
  buf[offset] = '\0';
  
  return offset + 1;
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
// mace_on implementation
//

static ssize_t
show_mace_on(struct kobject *kobj,
                struct kobj_attribute *attr,
                char *buf)
{
  ssize_t offset = 0;
  int res = 0;

  mace_lookup_ns(current->nsproxy->net_ns->ns.inum,
                 mace_active_ns,
                 &res);

  // Look up this namespace's status and print report
  offset += snprintf(buf + offset,
                     PAGE_SIZE - offset,
                     "%d\n",
                     res);
  return offset;
}

static ssize_t
store_mace_on(struct kobject *kobj,
                 struct kobj_attribute *attr,
                 const char *buf,
                 size_t count)
{
  int req;
  unsigned long nsid;

  if (kstrtoint(buf, 0, &req) == 0) {

    // Set mace status for this namespace
    nsid = current->nsproxy->net_ns->ns.inum;

    if (req == 0) {
      mace_del_ns(nsid, mace_active_ns);
      printk(KERN_INFO "Mace: removed nsid: %lu\n", nsid);

    } else {
      mace_add_ns(nsid, mace_active_ns);
      printk(KERN_INFO "Mace: added nsid: %lu\n", nsid);
    }
  }
  return count;
}
