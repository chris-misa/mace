//
// Test of sysfs userspace interface
//
// From: pradheepshrinivasan.github.io/2015/07/02/Creating-an-simple-sysfs
//

#define LINUX

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris Misa <cmisa@cs.uoregon.edu>");
MODULE_DESCRIPTION("Test of sysfs file system");


static struct kobject *example_kobject;
static int foo;

static ssize_t foo_show(struct kobject *kobj,
                        struct kobj_attribute *attr,
                        char *buf)
{
  return sprintf(buf, "%d\n", foo);
}

static ssize_t foo_store(struct kobject *kobj,
                         struct kobj_attribute *attr,
                         const char *buf,
                         size_t count)
{
  sscanf(buf, "%du", &foo);
  return count;
}

static struct kobj_attribute foo_attribute
    = __ATTR(foo, 0660, foo_show, foo_store);

static int __init mymodule_init(void)
{
  int error = 0;
  printk("Sysfs test module initialized.\n");
  example_kobject = kobject_create_and_add("kobject_example",
                                           kernel_kobj);
  if (!example_kobject) {
    return -ENOMEM;
  }

  error = sysfs_create_file(example_kobject,
                            &foo_attribute.attr);

  if (error) {
    printk("Failed to create the foo file!\n");
  }

  return error;
}

static void __exit mymodule_exit(void)
{
  printk("Sysfs test module exiting\n");
  kobject_put(example_kobject);
}

module_init(mymodule_init);
module_exit(mymodule_exit);
