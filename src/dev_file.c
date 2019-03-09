//
// Initial MACE implementation
//
// 2019, Chris Misa
//

#include "dev_file.h"

#define LATENCY_QUEUE_DEV_NAME "mace"
#define MACE_MAX_LINE_LEN 1024

//
// Report latency approximations in nano seconds
// Cycles / (1 000 Cycles / sec) * (1 000 000 000 nsec / sec)
//
#define mace_cycles_to_ns(c) (((c) * 1000000) / tsc_khz)

// Defined in module.c
extern struct list_head mace_active_ns;

DECLARE_WAIT_QUEUE_HEAD(mace_latency_read_queue);
static int latency_queue_is_open = 0;
int mace_latency_queue_major = -1;
static struct cdev latency_queue_cdev;
static struct class *latency_queue_class = NULL;
static ssize_t on_show(struct class *class, struct class_attribute *attr, char *buf);
static ssize_t on_store(struct class *class, struct class_attribute *attr, const char *buf, size_t count);
CLASS_ATTR_RW(on);


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

  if (class_create_file(latency_queue_class, &class_attr_on) < 0) {
    printk(KERN_INFO "Mace: failed to create on class attribute\n");
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
  static int i = 0;

  int read = 0;
  struct mace_latency_event *lat = mace_get_buf()->queue + i;
  unsigned long cur_nsid = current->nsproxy->net_ns->ns.inum;

  if (!line_ptr || *line_ptr == '\0') {
    while (lat->ns_id != cur_nsid && i < MACE_EVENT_QUEUE_SIZE) {
      lat++;
      i++;
    }
    if (i >= MACE_EVENT_QUEUE_SIZE) {
      goto finished;
    }
    snprintf(line_buf,
             MACE_MAX_LINE_LEN,
             "[%llu] %s: %llu\n",
             mace_cycles_to_ns(lat->ts) & 0xFFFFFFFF, // Masking to 32-bits for R
             mace_latency_type_str(lat->type),
             mace_cycles_to_ns(lat->latency));
    lat++;
    i++;
    line_ptr = line_buf;
  }

  while (len && *line_ptr) {
    put_user(*(line_ptr++), buf++);
    len--;
    read++;
  }
  return read;

finished:

  i = 0;
  return 0;
}

static ssize_t
latency_queue_write(struct file *fp, const char *buf, size_t len, loff_t *offset)
{
  printk(KERN_INFO "Mace: latency queue write from netnet %u.\n",
     current->nsproxy->net_ns->ns.inum);
  return -EINVAL;
}

//
// Sysfs attribute functions
//
static ssize_t
on_show(struct class *class,
             struct class_attribute *attr,
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
on_store(struct class *class,
              struct class_attribute *attr,
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
