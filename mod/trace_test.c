#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/tracepoint.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/jiffies.h>
#include <asm/msr.h>


#include <linux/fs.h>
#include <asm/uaccess.h>

#include <linux/slab.h>

#define SUCCESS 0
#define DEVICE_NAME "mace_test"
#define MSG_SIZE 256
#define BUF_SIZE 256


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris Misa <cmisa@cs.uoregon.edu>");
MODULE_DESCRIPTION("Probing tracepoints to monitor network stack latency");

static struct tracepoint *probe_tracepoint[2];
static unsigned long long send_time;
static unsigned long long *ring_buffer;
static unsigned long head;
static unsigned long tail;

DECLARE_WAIT_QUEUE_HEAD(wait_q);

//
// device file handling
//
static int dev_is_open = 0;
static int Major;
static char *msg;
static char *msgp = NULL;

static int device_open(struct inode *inode, struct file *fp);
static int device_release(struct inode *inode, struct file *fp);
static ssize_t device_read(struct file *fp, char *buf, size_t len, loff_t *offset);
static ssize_t device_write(struct file *fp, const char *buf, size_t len, loff_t *offset);

static struct file_operations fops = {
  .read = device_read,
  .write = device_write,
  .open = device_open,
  .release = device_release
};

void
probe_net_dev_xmit(void *unused, struct sk_buff *skb, int rc, struct net_device *dev, unsigned int len)
{
  unsigned long long cur_time;
  unsigned long long delta_time;
  if (dev) {
    cur_time = rdtsc();
    delta_time = cur_time - send_time;
    printk("[%llu] dev->ifindex: %d\n", cur_time, dev->ifindex);
    printk("delta time: %llu\n", delta_time);
    
    // Push onto queue
    ring_buffer[head] = delta_time;
    head = (head + 1) % BUF_SIZE;
    printk("pushed onto queue at %lu\n", head);

    // Wake up any waiting readers
    wake_up(&wait_q);
  }
}

void
probe_sys_enter(void *unused, struct pt_regs *regs, long id)
{
  // Hard-coded sendto syscall number
  if (id == 44) {
    send_time = rdtsc();
    printk("[%llu] probably a sendto\n", send_time);
  }
}


void
test_and_set_tracepoint(struct tracepoint *tp, void *priv)
{
  int ret = 0;
  int found = 0;

	if (!strcmp(tp->name, "net_dev_xmit")) {
		ret = tracepoint_probe_register(tp, probe_net_dev_xmit, NULL);
    probe_tracepoint[0] = tp;
    found = 1;
  } else if (!strcmp(tp->name, "sys_enter")) {
		ret = tracepoint_probe_register(tp, probe_sys_enter, NULL);
    probe_tracepoint[1] = tp;
    found = 1;
  }
  
  if (found) {
    if (ret != 0) {
      printk(KERN_ALERT "Failed to set tracepoint.\n");
    } else {
      printk("Set trace probe\n");
    }
  }
}

int __init
trace_test_init(void)
{
  probe_tracepoint[0] = NULL;
  probe_tracepoint[1] = NULL;

  // Get memory for buffers
  ring_buffer = kmalloc(sizeof(long long int) * BUF_SIZE, GFP_KERNEL);
  if (ring_buffer == NULL) {
    printk(KERN_ALERT "Failed to alloc memory.\n");
    return -ENOMEM;
  }
  head = tail = 0;

  msg = kmalloc(sizeof(char) * MSG_SIZE, GFP_KERNEL);
  if (msg == NULL) {
    kfree(ring_buffer);
    printk(KERN_ALERT "Failed to alloc memory.\n");
    return -ENOMEM;
  }

  // Set up device file
  Major = register_chrdev(0, DEVICE_NAME, &fops);
  if (Major < 0) {
    kfree(ring_buffer);
    kfree(msg);
    printk(KERN_ALERT "Registering chardev failed with %d\n", Major);
    return Major;
  }
  printk("mace_test assigned major number %d\n", Major);

	// Find the tracepoint and set probe function
	for_each_kernel_tracepoint(test_and_set_tracepoint, NULL);

	return 0;
}

void __exit
trace_test_exit(void)
{
	if (probe_tracepoint[0]) {
		if (tracepoint_probe_unregister(probe_tracepoint[0], probe_net_dev_xmit, NULL) != 0) {
			printk(KERN_ALERT "Failed to unregister trace probe.\n");
		}
	}
	if (probe_tracepoint[1]) {
		if (tracepoint_probe_unregister(probe_tracepoint[1], probe_sys_enter, NULL) != 0) {
			printk(KERN_ALERT "Failed to unregister trace probe.\n");
		}
	}
	printk("Removed trace probes.\n");
  if (ring_buffer) {
    kfree(ring_buffer);
  }
  if (msg) {
    kfree(msg);
  }
  printk("Freed memory.\n");

  // Remove the device file
  unregister_chrdev(Major, DEVICE_NAME);
  printk("Removed device file.\n");
  
}

module_init(trace_test_init);
module_exit(trace_test_exit);


static int
device_open(struct inode *inode, struct file *fp)
{
  if (dev_is_open) {
    return -EBUSY;
  }

  dev_is_open++;
  try_module_get(THIS_MODULE);

  *msg = 0;
  msgp = msg;

  return SUCCESS;
}

static int
device_release(struct inode *inode, struct file *fp)
{
  dev_is_open--;
  module_put(THIS_MODULE);
  return 0;
}

static ssize_t
device_read(struct file *fp, char *buf, size_t len, loff_t *offset)
{
  int bytes_read = 0;

  // Read in the next line if needed
  if (*msgp == 0) {

    if (head == tail) {
      // If we're non-blocking, wait for next entry in queue
      wait_event_interruptible(wait_q, head != tail);
    }

    if (head == tail) {
      // Condition still true, must have been a signal
      return 0;
    }

    // Form the string, dequeue
    sprintf(msg, "delta_time: %llu\n", ring_buffer[tail]);
    msgp = msg;
    tail = (tail + 1) % BUF_SIZE;
  }

  // Write buffer to userspace
  while (len && *msgp) {
    put_user(*(msgp++), buf++);
    len--;
    bytes_read++;
  }

  return bytes_read;
}

static ssize_t
device_write(struct file *fp, const char *buf, size_t len, loff_t *offset)
{
  printk(KERN_ALERT "Writing not supported.\n");
  return -EINVAL;
}
