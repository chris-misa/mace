#define LINUX

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/tracepoint.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>

MODULE_LICENSE("GPL");

void print_tracepoint_name(struct tracepoint *tp, void *priv);
void probe_func(struct sk_buff *skb, int rc, struct net_device *dev, unsigned int len);

int
init_module(void)
{
	printk("<1>Hello Worlds 1.\n");
	for_each_kernel_tracepoint(print_tracepoint_name, NULL);
	return 0;
}

void
cleanup_module(void)
{

	printk(KERN_ALERT "Goodbye worlds.\n");
}

void
print_tracepoint_name(struct tracepoint *tp, void *priv)
{
	if (!strcmp(tp->name, "net_dev_xmit")) {
		if (tracepoint_probe_register(tp, probe_func, NULL) != 0) {
			printk(KERN_ALERT "Failed to register for trace event.\n");
		}
	}
}

void
probe_func(struct sk_buff *skb, int rc, struct net_device *dev, unsigned int len)
{
	printk("In probe function.\n");
}
