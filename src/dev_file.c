//
// Initial MACE implementation
//
// 2019, Chris Misa
//

#include "dev_file.h"

#define LATENCY_QUEUE_DEV_NAME "mace_latency_queue"

DECLARE_WAIT_QUEUE_HEAD(mace_latency_read_queue);
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
  //?  mace_latency_queue_major = register_chrdev(0,
  //?                                        LATENCY_QUEUE_DEV_NAME,
  //?                                        &latency_queue_fops);
  //?  if (mace_latency_queue_major < 0) {
  //?    printk(KERN_INFO "Mace: failed to register char device for latency queue.\n");
  //?    return mace_latency_queue_major;
  //?  } else {
  //?    printk(KERN_INFO "Mace: created major %d\n", mace_latency_queue_major);
  //?  }

  int created = 0;

  if (alloc_chrdev_region(&mace_latency_queue_major, 0, 1, LATENCY_QUEUE_DEV_NAME "_proc") < 0) {
    goto exit_error;
  }
  
  latency_queue_class = class_create(THIS_MODULE, LATENCY_QUEUE_DEV_NAME "_sys");
  if (latency_queue_class == NULL) {
    goto exit_error;
  }

  if (device_create(latency_queue_class, NULL, mace_latency_queue_major, NULL, LATENCY_QUEUE_DEV_NAME "_dev") == NULL) {
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
  printk(KERN_INFO "Mace: latency queue read.\n");
  return 0;
}

static ssize_t
latency_queue_write(struct file *fp, const char *buf, size_t len, loff_t *offset)
{
  printk(KERN_INFO "Mace: latency queue write.\n");
  return -EINVAL;
}
