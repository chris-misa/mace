//
// Test of procfs userspace interface
//

#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/string.h>

#include <linux/netdevice.h>
#include <linux/net_namespace.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris Misa <cmisa@cs.uoregon.edu>");
MODULE_DESCRIPTION("Test of procfs file system");

#define PROCFS_MAX_SIZE 1024
#define PROCFS_NAME "buffer1k"

static struct proc_dir_entry *our_proc_file;

static char our_buf[PROCFS_MAX_SIZE];

static unsigned long our_buf_size = 0;

int
procfile_read(struct file *file,
              char __user *buf,
              size_t count,
              loff_t *ppos)
{
  int ret = 0;
  printk(KERN_INFO "procfile_read (/proc/%s) called\n", PROCFS_NAME);

  return ret;
}

int
procfile_write(struct file *file,
               const char __user *buf,
               size_t count,
               loff_t *data)
{
  printk(KERN_INFO "procfile_write called\n");

  return -1;
} 

static struct file_operations our_ops = 
{
  .owner = THIS_MODULE,
  .read = procfile_read,
  .write = procfile_write
};

int
init_module(void)
{
  our_proc_file = proc_create(PROCFS_NAME, 0660 NULL, &our_ops);
  printk(KERN_INFO "added proc entry\n");
  return 0;
}

void
cleanup_module(void)
{
  proc_remove(our_proc_file);
  printk(KERN_INFO "removed proc entry\n");
}

