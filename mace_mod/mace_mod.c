//
// Initial MACE module work
//
// 2019, Chris Misa
//

#define LINUX

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/string.h>

#include <linux/tracepoint.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <asm/msr.h>


#include <linux/fs.h>
#include <asm/uaccess.h>

#include <linux/slab.h>

#include <uapi/linux/ip.h>
#include <uapi/linux/icmp.h>
#include <uapi/linux/if_ether.h>

#include <asm/syscall.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris Misa <cmisa@cs.uoregon.edu>");
MODULE_DESCRIPTION("Test of sysfs file system");

#define SYSCALL_SENDTO 44
#define SYSCALL_RECVMSG 47

static struct tracepoint *sys_enter_tracepoint;
static struct tracepoint *net_dev_queue_tracepoint;

DEFINE_PER_CPU(unsigned long long, sys_enter_time);
DEFINE_PER_CPU(unsigned char, sys_entered);

void
probe_sys_enter(void *unused, struct pt_regs *regs, long id)
{
  this_cpu_write(sys_enter_time, rdtsc());
  this_cpu_write(sys_entered, 1);
}

void
probe_net_dev_queue(void *unused, struct sk_buff *skb)
{
  unsigned long long dt = rdtsc() - this_cpu_read(sys_enter_time);

  printk("In net_dev_queue probe after %lld cycles\n", dt);
}

void
test_and_set_traceprobe(struct tracepoint *tp, void *unused)
{
  int ret = 0;
  int found = 0;

  if (!strcmp(tp->name, "sys_enter")) {
    ret = tracepoint_probe_register(tp, probe_sys_enter, NULL);
    sys_enter_tracepoint = tp;
    found = 1;
  } else if (!strcmp(tp->name, "net_dev_queue")) {
    ret = tracepoint_probe_register(tp, probe_net_dev_queue, NULL);
    net_dev_queue_tracepoint = tp;
    found = 1;
  }
  
  if (found && ret != 0) {
    printk(KERN_WARNING "Failed to set tracepoint.\n");
  }
}

void
init_per_cpu_variables(void *unused)
{
  this_cpu_write(sys_entered, 0);
}

int __init
mace_mod_init(void)
{
  sys_enter_tracepoint = NULL;
  net_dev_queue_tracepoint = NULL;

  on_each_cpu(init_per_cpu_variables, NULL, 1);

  for_each_kernel_tracepoint(test_and_set_traceprobe, NULL);

  printk(KERN_INFO "Mace running.\n");

  return 0;
}

void __exit
mace_mod_exit(void)
{
  if (sys_enter_tracepoint
      && tracepoint_probe_unregister(sys_enter_tracepoint, probe_sys_enter, NULL)) {
    printk(KERN_WARNING "Failed to unregister sys_enter traceprobe.\n");
  }
  if (net_dev_queue_tracepoint
      && tracepoint_probe_unregister(net_dev_queue_tracepoint, probe_net_dev_queue, NULL)) {
    printk(KERN_WARNING "Failed to unregister net_dev_queue traceprobe.\n");
  }
  printk(KERN_INFO "Mace stopped.\n");
}

module_init(mace_mod_init);
module_exit(mace_mod_exit);
