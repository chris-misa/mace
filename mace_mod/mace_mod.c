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

#include <linux/sched.h>
#include <linux/fdtable.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris Misa <cmisa@cs.uoregon.edu>");
MODULE_DESCRIPTION("Test of sysfs file system");

// Param: Outer device id
static int outer_dev = -1;
module_param(outer_dev, int, 0);
MODULE_PARM_DESC(outer_dev, "Device id of the outer device");

// Param: Inner device id (for now. . .)
static int inner_dev = -1;
module_param(inner_dev, int, 0);
MODULE_PARM_DESC(inner_dev, "Device id of inner devie");

#define SYSCALL_SENDTO 44
#define SYSCALL_RECVMSG 47

#define MACE_LATENCIES_SIZE 128

// Simple statically allocated hash table
#define mace_hash(key) ((unsigned long long)(key) % MACE_LATENCIES_SIZE)
struct mace_latency {
  unsigned long long enter;
  struct sk_buff *skb;
  int valid;
};
static struct mace_latency egress_latencies[MACE_LATENCIES_SIZE];
static struct mace_latency ingress_latencies[MACE_LATENCIES_SIZE];

// Tracepoint pointers kept for cleanup
static struct tracepoint *sys_enter_tracepoint;
static struct tracepoint *net_dev_queue_tracepoint;
static struct tracepoint *net_dev_start_xmit_tracepoint;
static struct tracepoint *napi_gro_receive_entry_tracepoint;
static struct tracepoint *netif_receive_skb_tracepoint;
static struct tracepoint *sys_exit_tracepoint;

// Per cpu state for send heuristic
DEFINE_PER_CPU(unsigned long long, sys_enter_time);
DEFINE_PER_CPU(unsigned char, sys_entered);

//
// Take timestamp and set sys_entered flag on sendto syscall
//
void
probe_sys_enter(void *unused, struct pt_regs *regs, long id)
{
  if (id == SYSCALL_SENDTO) {
    this_cpu_write(sys_enter_time, rdtsc());
    this_cpu_write(sys_entered, 1);
  }
}

//
// Connect sys_enter's timestamp with kernel's sk_buff address
//
void
probe_net_dev_queue(void *unused, struct sk_buff *skb)
{
  struct mace_latency *ml;

  if (this_cpu_read(sys_entered)) {
    this_cpu_write(sys_entered, 0);

    // Start new active latency, over-writing anything
    // which might have hashed into the same slot.
    ml = egress_latencies + mace_hash(skb);
    ml->enter = this_cpu_read(sys_enter_time);
    ml->skb = skb;
    ml->valid = 1;
  }
}

//
// Report latency if a matching sk_buff was previously timestamped.
//
// Another idea would be to watch both start_xmit and xmit and pick an egress time
// between the two to account for driver latency.
//
void
probe_net_dev_start_xmit(void *unused, struct sk_buff *skb, struct net_device *dev)
{
  struct mace_latency *ml;
  unsigned long long dt;
  
  // Filter for outer device
  if (dev->ifindex == outer_dev) {

    // Look for active latency for this skb
    ml = egress_latencies + mace_hash(skb);
    if (ml->valid && skb == ml->skb) {
      dt = rdtsc() - ml->enter;
      ml->valid = 0;

      // Report ingress latency
      printk(KERN_INFO "Mace: egress latency of %lld cycles.\n", dt);
    }
  }
}

void
probe_napi_gro_receive_entry(void *unused, struct sk_buff *skb)
{
  struct mace_latency *ml;

  // Filter for outer device
  if (skb->dev && skb->dev->ifindex == outer_dev) {
    
    // Store this arrival time in table
    ml = ingress_latencies + mace_hash(skb);
    ml->enter = rdtsc();
    ml->skb = skb;
    ml->valid = 1;
  }
}

void
probe_netif_receive_skb(void *unused, struct sk_buff *skb)
{
  struct mace_latency *ml;
  unsigned long long dt;

  // Filter for inner device
  if (skb->dev && skb->dev->ifindex == inner_dev) {

    // Look for active latency for this skb
    ml = ingress_latencies + mace_hash(skb);
    if (ml->valid && skb == ml->skb) {
      dt = rdtsc() - ml->enter;
      ml->valid = 0;

      // Report egress latency
      printk(KERN_INFO "Mace: ingress latency of %lld cycles.\n", dt);
    }
  }
}

void
probe_sys_exit(void *unused, struct pt_regs *regs, long ret)
{
}

//
// Identify target tracepoints by name
//
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
  } else if (!strcmp(tp->name, "net_dev_start_xmit")) {
    ret = tracepoint_probe_register(tp, probe_net_dev_start_xmit, NULL);
    net_dev_start_xmit_tracepoint = tp;
    found = 1;
  } else if (!strcmp(tp->name, "napi_gro_receive_entry")) {
    ret = tracepoint_probe_register(tp, probe_napi_gro_receive_entry, NULL);
    napi_gro_receive_entry_tracepoint = tp;
    found = 1;
  } else if (!strcmp(tp->name, "netif_receive_skb")) {
    ret = tracepoint_probe_register(tp, probe_netif_receive_skb, NULL);
    netif_receive_skb_tracepoint = tp;
    found = 1;
  } else if (!strcmp(tp->name, "sys_exit")) {
    ret = tracepoint_probe_register(tp, probe_sys_exit, NULL);
    sys_exit_tracepoint = tp;
    found = 1;
  }
  
  if (found && ret != 0) {
    printk(KERN_WARNING "Mace: Failed to set tracepoint.\n");
  }
}

//
// Initialize per-cpu flags
//
void
init_per_cpu_variables(void *unused)
{
  this_cpu_write(sys_entered, 0);
}

//
// Module initialization
//
int __init
mace_mod_init(void)
{
  int ret = 0;
  sys_enter_tracepoint = NULL;
  net_dev_queue_tracepoint = NULL;
  net_dev_start_xmit_tracepoint = NULL;

  // Check for required parameters
  if (outer_dev < 0) {
    printk(KERN_INFO "Mace: must set outer_dev=<outer device id>\n");
    ret = -1;
  }

  if (inner_dev < 0) {
    printk(KERN_INFO "Mace: must set inner_dev=<inner device id>\n");
    ret = -1;
  }

  if (ret) {
    return ret;
  }

  // Set up
  on_each_cpu(init_per_cpu_variables, NULL, 1);

  for_each_kernel_tracepoint(test_and_set_traceprobe, NULL);

  printk(KERN_INFO "Mace: running.\n");

  return 0;
}

//
// Module clean up
//
void __exit
mace_mod_exit(void)
{
  if (sys_enter_tracepoint
      && tracepoint_probe_unregister(sys_enter_tracepoint, probe_sys_enter, NULL)) {
    printk(KERN_WARNING "Mace: Failed to unregister sys_enter traceprobe.\n");
  }
  if (net_dev_queue_tracepoint
      && tracepoint_probe_unregister(net_dev_queue_tracepoint, probe_net_dev_queue, NULL)) {
    printk(KERN_WARNING "Mace: Failed to unregister net_dev_queue traceprobe.\n");
  }
  if (net_dev_start_xmit_tracepoint
      && tracepoint_probe_unregister(net_dev_start_xmit_tracepoint, probe_net_dev_start_xmit, NULL)) {
    printk(KERN_WARNING "Mace: Failed to unregister net_dev_start_xmit traceprobe.\n");
  }
  if (napi_gro_receive_entry_tracepoint
      && tracepoint_probe_unregister(napi_gro_receive_entry_tracepoint, probe_napi_gro_receive_entry, NULL)) {
    printk(KERN_WARNING "Mace: Failed to unregister napi_gro_receive_entry_tracepoint.\n");
  }
  if (netif_receive_skb_tracepoint
      && tracepoint_probe_unregister(netif_receive_skb_tracepoint, probe_netif_receive_skb, NULL)) {
    printk(KERN_WARNING "Mace: Failed to unregister netif_receive_skb traceprobe.\n");
  }
  if (sys_exit_tracepoint
      && tracepoint_probe_unregister(sys_exit_tracepoint, probe_sys_exit, NULL)) {
    printk(KERN_WARNING "Mace: Failed to unregister sys_exit traceprobe.\n");
  }
  printk(KERN_INFO "Mace: stopped.\n");
}

module_init(mace_mod_init);
module_exit(mace_mod_exit);
