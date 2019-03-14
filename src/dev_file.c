//
// Initial MACE implementation
//
// 2019, Chris Misa
//

#include "dev_file.h"

#define LATENCY_QUEUE_DEV_NAME "mace"
#define MACE_MAX_LINE_LEN 1024

//
// Set of mace-active namespaces
// Defined in namespace_set.c
//
extern struct radix_tree_root mace_namespaces;

//
// Tsc gettimeofday offset
//
extern struct timeval mace_tsc_offset;

//
// Mace internal perturbation tracking from module.c
//
extern struct mace_perturbation mace_sys_enter_pert;
extern struct mace_perturbation mace_net_dev_start_xmit_pert;
extern struct mace_perturbation mace_netif_receive_skb_pert;
extern struct mace_perturbation mace_sys_exit_pert;


//
// 'mace/on' class attribute
//
static ssize_t on_show(struct class *class, struct class_attribute *attr, char *buf);
static ssize_t on_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count);
CLASS_ATTR_RW(on);

//
// 'mace/sync' class attribute
//
static ssize_t sync_show(struct class *class, struct class_attribute *attr, char *buf);
static ssize_t sync_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count);
CLASS_ATTR_RW(sync);


//
// 'mace/pert_sys_enter' class attribute
//
static ssize_t pert_sys_enter_show(struct class *class, struct class_attribute *attr, char *buf);
static ssize_t pert_sys_enter_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count);
CLASS_ATTR_RW(pert_sys_enter);

//
// 'mace/pert_net_dev_start_xmit' class attribute
//
static ssize_t pert_net_dev_start_xmit_show(struct class *class, struct class_attribute *attr, char *buf);
static ssize_t pert_net_dev_start_xmit_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count);
CLASS_ATTR_RW(pert_net_dev_start_xmit);

//
// 'mace/pert_netif_receive_skb' class attribute
//
static ssize_t pert_netif_receive_skb_show(struct class *class, struct class_attribute *attr, char *buf);
static ssize_t pert_netif_receive_skb_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count);
CLASS_ATTR_RW(pert_netif_receive_skb);

//
// 'mace/pert_sys_exit' class attribute
//
static ssize_t pert_sys_exit_show(struct class *class, struct class_attribute *attr, char *buf);
static ssize_t pert_sys_exit_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count);
CLASS_ATTR_RW(pert_sys_exit);


//
// '/dev/mace' latencies file
//
DECLARE_WAIT_QUEUE_HEAD(mace_latency_read_queue); // Unused for now
static int latency_queue_is_open = 0;
int mace_latency_queue_major = -1;
static struct cdev latency_queue_cdev;
static struct class *latency_queue_class = NULL;

static int latency_queue_open(struct inode *inode, struct file *fp);
static int latency_queue_release(struct inode *inode, struct file *fp);
static ssize_t latency_queue_read(struct file *fp, char *buf, size_t len, loff_t *offset);
static ssize_t latency_queue_write(struct file *fp, const char *buf, size_t len, loff_t *offset);

static struct file_operations latency_queue_fops = {
  .read = latency_queue_read,
  .write = latency_queue_write,
  .open = latency_queue_open,
  .release = latency_queue_release
};

static void
cleanup_latency_queue(int created)
{
  if (created) {
    device_destroy(latency_queue_class, mace_latency_queue_major);
    cdev_del(&latency_queue_cdev);
  }
  if (latency_queue_class) {
    class_destroy(latency_queue_class);
  }
  if (mace_latency_queue_major != -1) {
    unregister_chrdev_region(mace_latency_queue_major, 1);
  }
}

int
mace_init_dev(void)
{
  int created = 0;

  if (alloc_chrdev_region(&mace_latency_queue_major, 0, 1,
                          LATENCY_QUEUE_DEV_NAME) < 0) {
    goto exit_error;
  }
  
  latency_queue_class = class_create(THIS_MODULE, LATENCY_QUEUE_DEV_NAME);
  if (latency_queue_class == NULL) {
    goto exit_error;
  }

  // Add 'on' attribute
  if (class_create_file(latency_queue_class, &class_attr_on) < 0) {
    printk(KERN_INFO "Mace: failed to create 'on' class attribute\n");
    goto exit_error;
  }

  // Add 'sync' attribute
  if (class_create_file(latency_queue_class, &class_attr_sync) < 0) {
    printk(KERN_INFO "Mace: failed to create 'sync' class attribute\n");
    goto exit_error;
  }

  // Add perturbation attributes
  if (class_create_file(latency_queue_class, &class_attr_pert_sys_enter) < 0) {
    printk(KERN_INFO "Mace: failed to create 'per_sys_enter' class attribute\n");
    goto exit_error;
  }
  if (class_create_file(latency_queue_class, &class_attr_pert_net_dev_start_xmit) < 0) {
    printk(KERN_INFO "Mace: failed to create 'pert_net_dev_start_xmit' class attribute\n");
    goto exit_error;
  }
  if (class_create_file(latency_queue_class, &class_attr_pert_netif_receive_skb) < 0) {
    printk(KERN_INFO "Mace: failed to create 'pert_netif_receive_skb' class attribute\n");
    goto exit_error;
  }
  if (class_create_file(latency_queue_class, &class_attr_pert_sys_exit) < 0) {
    printk(KERN_INFO "Mace: failed to create 'pert_sys_exit' class attribute\n");
    goto exit_error;
  }

  if (device_create(latency_queue_class, NULL,
                    mace_latency_queue_major, NULL,
                    LATENCY_QUEUE_DEV_NAME) == NULL) {
    goto exit_error;
  }
  created = 1;
  cdev_init(&latency_queue_cdev, &latency_queue_fops);
  if (cdev_add(&latency_queue_cdev, mace_latency_queue_major, 1) == -1) {
    goto exit_error;
  }

  return 0;

exit_error:
  cleanup_latency_queue(created);
  return -1;
}

void
mace_free_dev(void)
{
  // unregister_chrdev(mace_latency_queue_major, LATENCY_QUEUE_DEV_NAME);
  cleanup_latency_queue(1);
}

static int
latency_queue_open(struct inode *inode, struct file *fp)
{
  if (latency_queue_is_open) {
    return -EBUSY;
  }
  latency_queue_is_open++;
  try_module_get(THIS_MODULE);

  return 0;
}

static int
latency_queue_release(struct inode *inode, struct file *fp)
{
  latency_queue_is_open--;
  module_put(THIS_MODULE);
  return 0;
}

static ssize_t
latency_queue_read(struct file *fp, char *buf, size_t len, loff_t *offset)
{
  static char line_buf[MACE_MAX_LINE_LEN];
  static char *line_ptr = NULL;

  int res;
  int read = 0;
  struct mace_namespace_entry *ns = NULL;
  struct mace_latency_event lat;
  struct timeval tv;

  // If the line buffer is empty, get a new latency event from queue
  if (!line_ptr || *line_ptr == '\0') {

    ns = mace_get_ns(&mace_namespaces, current->nsproxy->net_ns->ns.inum);
    if (ns == NULL) {
      // This namespace is not in mace_active_ns list
      return 0;
    }

    // Pop in a loop to handle write collisions
    do {
      res = mace_pop_event(&ns->buf, &lat);
    } while (res == 1);

    if (res == 2) {
      // Queue is empty
      return 0;
    }

    // Convert by offset
    mace_tsc_to_gettimeofday(lat.ts, &tv);
    
    // Print into line buffer
    snprintf(line_buf,
             MACE_MAX_LINE_LEN,
             "[%lu.%06lu] (%lu) %s: %llu\n",
             tv.tv_sec, tv.tv_usec,
             lat.pktid,
             mace_latency_type_str(lat.type),
             mace_cycles_to_ns(lat.latency));
    line_ptr = line_buf;
  }

  while (len && *line_ptr) {
    put_user(*(line_ptr++), buf++);
    len--;
    read++;
  }
  return read;
}

static ssize_t
latency_queue_write(struct file *fp, const char *buf, size_t len, loff_t *offset)
{
  printk(KERN_INFO "Mace: latency queue write from netnet %u.\n",
     current->nsproxy->net_ns->ns.inum);
  return -EINVAL;
}

// Sysfs attribute functions

//
// 'mace/on' class attribute
//
static ssize_t
on_show(struct class *class,
             struct class_attribute *attr,
             char *buf)
{
  ssize_t offset = 0;
  int res = 0;

  if (mace_get_ns(&mace_namespaces, current->nsproxy->net_ns->ns.inum) != NULL) {
    res = 1;
  }

  // Look up this namespace's status and print report
  offset += snprintf(buf + offset,
                     PAGE_SIZE - offset,
                     "%d\n",
                     res);
  return offset;
}

static ssize_t
on_store(struct class *class,
              struct class_attribute *attr,
              const char *buf,
              size_t count)
{
  int req;
  unsigned long nsid;
  int res;

  if (kstrtoint(buf, 0, &req) == 0) {

    // Set mace status for this namespace
    nsid = current->nsproxy->net_ns->ns.inum;

    if (req == 0) {
      mace_del_ns(&mace_namespaces, nsid);
      printk(KERN_INFO "Mace: removed nsid: %lu\n", nsid);

    } else {
      if ((res = mace_add_ns(&mace_namespaces, nsid)) == 0) {
        printk(KERN_INFO "Mace: added nsid: %lu\n", nsid);
      } else {
        switch (res) {
          case -ENOMEM:
            printk(KERN_INFO "Mace: faild to add nsid %lu: no memory\n", nsid);
            break;
          case -EEXIST:
            printk(KERN_INFO "Mace: faild to add nsid %lu: already exists\n", nsid);
            break;
          default:
            printk(KERN_INFO "Mace: faild to add nsid %lu: unknown error: %d\n", nsid, res);
            break;
        }
      }
    }
  }

  // Regardless of what happened, claim to have used entire buffer
  return count;
}


//
// 'mace/sync' class attribute
//
static ssize_t
sync_show(struct class *class, struct class_attribute *attr, char *buf)
{
  ssize_t offset = 0;

  offset += snprintf(buf + offset,
                     PAGE_SIZE - offset,
                     "%lu.%06lu\n",
                     mace_tsc_offset.tv_sec,
                     mace_tsc_offset.tv_usec);
  return offset;
}

static ssize_t
sync_store(struct class *class, struct class_attribute *attr, const char *but, size_t count)
{
  mace_tsc_offset_resync();
  return count;
}


//
// 'mace/pert_sys_enter' class attribute
//
static ssize_t
pert_sys_enter_show(struct class *class, struct class_attribute *attr, char *buf)
{
  ssize_t offset = 0;
  offset += snprintf(buf + offset,
                     PAGE_SIZE - offset,
                     "%llu\n",
                     mace_sys_enter_pert.sum / mace_sys_enter_pert.count);
  return offset;
}
static ssize_t
pert_sys_enter_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
  return count;
}

//
// 'mace/pert_net_dev_start_xmit' class attribute
//
static ssize_t
pert_net_dev_start_xmit_show(struct class *class, struct class_attribute *attr, char *buf)
{
  ssize_t offset = 0;
  offset += snprintf(buf + offset,
                     PAGE_SIZE - offset,
                     "%llu\n",
                     mace_net_dev_start_xmit_pert.sum / mace_net_dev_start_xmit_pert.count);
  return offset;
}
static ssize_t
pert_net_dev_start_xmit_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
  return count;
}



//
// 'mace/pert_netif_receive_skb' class attribute
//
static ssize_t
pert_netif_receive_skb_show(struct class *class, struct class_attribute *attr, char *buf)
{
  ssize_t offset = 0;
  offset += snprintf(buf + offset,
                     PAGE_SIZE - offset,
                     "%llu\n",
                     mace_netif_receive_skb_pert.sum / mace_netif_receive_skb_pert.count);
  return offset;
}
static ssize_t
pert_netif_receive_skb_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
  return count;
}



//
// 'mace/pert_sys_exit' class attribute
//
static ssize_t
pert_sys_exit_show(struct class *class, struct class_attribute *attr, char *buf)
{
  ssize_t offset = 0;
  offset += snprintf(buf + offset,
                     PAGE_SIZE - offset,
                     "%llu\n",
                     mace_sys_exit_pert.sum / mace_sys_exit_pert.count);
  return offset;
}
static ssize_t
pert_sys_exit_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count)
{
  return count;
}

