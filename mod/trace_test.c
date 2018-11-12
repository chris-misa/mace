//
// Kernel module to measure network stack's send and receive latency
//
// Currently targets ping measurements.
//
// 2018, Chris Misa
// 

#define LINUX

#define LOG_EVENTS

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

#include <uapi/linux/ip.h>
#include <uapi/linux/icmp.h>

#define SUCCESS 0
#define DEVICE_NAME "mace_test"
#define MSG_SIZE 256
#define BUF_SIZE 256

#define SYSCALL_SENDTO 44
#define SYSCALL_RECVMSG 47


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris Misa <cmisa@cs.uoregon.edu>");
MODULE_DESCRIPTION("Probing tracepoints to monitor network stack latency");

static struct tracepoint *probe_tracepoint[4];

static int expect_send = 0;
static int expect_recv = 0;
static int ping_on_wire = 0;

static long long send_time = 0;
static long long recv_time = 0;

struct rtt_lats {
  unsigned long long send;
  unsigned long long recv;
  unsigned short seq;
};

static struct rtt_lats *ring_buffer;
static unsigned long head;
static unsigned long tail;

static struct rtt_lats cur_lat;

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

  struct iphdr *ip;
  struct icmphdr *icmp;

  // First thing, read the tsc
  cur_time = rdtsc();

  // Parse ip header, only want icmp packets
  ip = (struct iphdr *)skb->data;
  if (ip->protocol == 1
      && expect_send) {
    
    // Parse icmp header, only want echo request packets
    icmp = (struct icmphdr *)(skb->data + ip->ihl * 4);
    if (icmp->type == ICMP_ECHO) {
    
      delta_time = cur_lat.send = cur_time - send_time;
      cur_lat.seq = be16_to_cpu(icmp->un.echo.sequence);

      expect_send = 0;
      ping_on_wire = 1;

#ifdef LOG_EVENTS
      printk("[%llu] net_dev_xmit with dev->ifindex: %d, delta time: %llu\n",
        cur_time,
        dev->ifindex,
        delta_time);
#endif
    }
  }
}

void
probe_netif_receive_skb(void *unused, struct sk_buff *skb)
{
  struct iphdr *ip;
  struct icmphdr *icmp;
  unsigned short seq = 0;

  // Parse ip header, only want icmp packets
  ip = (struct iphdr *)skb->data;
  if (ip->protocol == 1) {

    // Parse the icmp header, only want echo replies
    icmp = (struct icmphdr *)(skb->data + ip->ihl * 4);
    if (icmp->type == ICMP_ECHOREPLY) {

      recv_time = rdtsc();
      expect_recv = 1;
      seq = be16_to_cpu(icmp->un.echo.sequence);

#ifdef LOG_EVENTS
      printk("[%llu] netif_receive_skb with dev->ifindex: %d icmp seq: %d\n",
        recv_time,
        skb->dev->ifindex,
        seq);
#endif
    }
  }
}

void
probe_sys_enter(void *unused, struct pt_regs *regs, long id)
{
  // Catch outgoing packets leaving userspace
  if (id == SYSCALL_SENDTO) {

    send_time = rdtsc();
    expect_send = 1;

#ifdef LOG_EVENTS
    printk("[%llu] sendto\n", send_time);
#endif
  }
}

void
probe_sys_exit(void *unused, struct pt_regs *regs, long id)
{
  unsigned long long cur_time;
  unsigned long long delta_time;

  // Catch incoming packets entering userspace
  if (id == SYSCALL_RECVMSG) {

    // Only process if we're expecting return packet
    if (expect_recv && ping_on_wire) {

      // First thing, read the tsc
      cur_time = rdtsc();

      delta_time = cur_lat.recv = cur_time - recv_time;
      expect_recv = 0;
      ping_on_wire = 0;

      // Push onto queue
      ring_buffer[head] = cur_lat;
      head = (head + 1) % BUF_SIZE;

      // Wake up any waiting readers
      wake_up(&wait_q);

#ifdef LOG_EVENTS
      printk("[%llu] recv delta time: %llu\n", cur_time, delta_time);
#endif
    }
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
  } else if (!strcmp(tp->name, "netif_receive_skb")) {
    ret = tracepoint_probe_register(tp, probe_netif_receive_skb, NULL);
    probe_tracepoint[2] = tp;
    found = 1;
  } else if (!strcmp(tp->name, "sys_exit")) {
    ret = tracepoint_probe_register(tp, probe_sys_exit, NULL);
    probe_tracepoint[3] = tp;
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
  probe_tracepoint[2] = NULL;
  probe_tracepoint[3] = NULL;

  // Get memory for buffers
  ring_buffer = kmalloc(sizeof(struct rtt_lats) * BUF_SIZE, GFP_KERNEL);
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
  printk("use \"mknod <path to new device file> c %d 0\"\n", Major);

	// Find the tracepoint and set probe function
	for_each_kernel_tracepoint(test_and_set_tracepoint, NULL);

	return 0;
}

void __exit
trace_test_exit(void)
{
	if (probe_tracepoint[0]) {
		if (tracepoint_probe_unregister(probe_tracepoint[0], probe_net_dev_xmit, NULL) != 0) {
			printk(KERN_ALERT "Failed to unregister net_dev_xmit traceprobe.\n");
		}
	}
	if (probe_tracepoint[1]) {
		if (tracepoint_probe_unregister(probe_tracepoint[1], probe_sys_enter, NULL) != 0) {
			printk(KERN_ALERT "Failed to unregister sys_enter traceprobe.\n");
		}
	}
  if (probe_tracepoint[2]) {
    if (tracepoint_probe_unregister(probe_tracepoint[2], probe_netif_receive_skb, NULL) != 0) {
      printk(KERN_ALERT "Failed to unregister netif_receive_skb traceprobe.\n");
    }
  }
  if (probe_tracepoint[3]) {
    if (tracepoint_probe_unregister(probe_tracepoint[3], probe_netif_receive_skb, NULL) != 0) {
      printk(KERN_ALERT "Failed to unregister sys_exit traceprobe.\n");
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
    sprintf(msg, "send latency: %llu recv latency: %llu\n",
      ring_buffer[tail].send,
      ring_buffer[tail].recv);
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
