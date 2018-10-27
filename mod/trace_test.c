#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/tracepoint.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris Misa <cmisa@cs.uoregon.edu>");
MODULE_DESCRIPTION("Probing tracepoints to monitor network stack latency");


static struct tracepoint *probe_tracepoint = NULL;

void
probe_func(void *unused, struct sk_buff *skb, int rc, struct net_device *dev, unsigned int len)
{
	printk("In probe function, skb: %x, dev: %x, len: %d\n", skb, dev, len);
  if (dev) {
    if (dev->name) {
      printk("dev->name: %s\n", dev->name);
    }
    printk("dev->ifindex: %d\n", dev->ifindex);
  }
}


void
test_and_set_tracepoint(struct tracepoint *tp, void *priv)
{
	if (!strcmp(tp->name, "net_dev_xmit")) {
		if (tracepoint_probe_register(tp, probe_func, NULL) != 0) {
			printk(KERN_ALERT "Failed to register for trace event.\n");
			probe_tracepoint = NULL;
			printk(KERN_ALERT "Failed to set tracepoint.\n");
		} else {
			probe_tracepoint = tp;
			printk("Set trace probe\n");
		}
	}
}

int __init
trace_test_init(void)
{
	// Find the tracepoint and set probe function
	for_each_kernel_tracepoint(test_and_set_tracepoint, NULL);
	return 0;
}

void __exit
trace_test_exit(void)
{
	if (probe_tracepoint) {
		if (tracepoint_probe_unregister(probe_tracepoint, probe_func, NULL) != 0) {
			printk(KERN_ALERT "Failed to unregister trace probe.\n");
		}
	}
	printk("Removed trace probe.\n");
}

module_init(trace_test_init);
module_exit(trace_test_exit);
