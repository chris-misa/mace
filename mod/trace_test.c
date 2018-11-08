#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/tracepoint.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/jiffies.h>
#include <asm/msr.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris Misa <cmisa@cs.uoregon.edu>");
MODULE_DESCRIPTION("Probing tracepoints to monitor network stack latency");

static struct tracepoint *probe_tracepoint[2];
static unsigned long long send_time;

void
probe_net_dev_xmit(void *unused, struct sk_buff *skb, int rc, struct net_device *dev, unsigned int len)
{
  unsigned long long cur_time;
  unsigned long long delta_time;
  if (dev) {
    cur_time = rdtsc();
    delta_time = cur_time - send_time;
    printk("[%lu] dev->ifindex: %d\n", cur_time, dev->ifindex);
    printk("delta time: %lu\n", delta_time);
  }
}

void
probe_sys_enter(void *unused, struct pt_regs *regs, long id)
{
  // Hard-coded sendto syscall number
  if (id == 44) {
    send_time = rdtsc();
    printk("[%lu] probably a sendto\n", send_time);
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
}

module_init(trace_test_init);
module_exit(trace_test_exit);
