//
// Initial MACE module work
//
// 2019, Chris Misa
//

// #define DEBUG
#define LINUX

#include <linux/kernel.h>
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
#include <linux/socket.h>
#include <asm/msr.h>


#include <linux/fs.h>
#include <asm/uaccess.h>

#include <linux/slab.h>

#include <uapi/linux/ip.h>
#include <uapi/linux/icmp.h>
#include <uapi/linux/if_ether.h>

#include <asm/syscall.h>

#include <linux/sched.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>

#include "ring_buffer.h"
#include "logic.h"
#include "sysfs.h"


#ifdef DEBUG
  #define check_ipv4(ip) \
    if (ip->version != 4) { \
      printk(KERN_INFO "Mace: Ignoring non-ipv4 packet.\n"); \
      return; \
    }
#else
  #define check_ipv4(ip) \
    if (ip->version != 4) { \
      return; \
    }
#endif

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

// Sudo Param: Premature eviction time:
//   time, in cycles, after which it is 'normal' to evict a table entry
static unsigned long long premature_eviction_thresh = 100000000;

// Syscall numbers. . .waiting for a better day
#define SYSCALL_SENDTO 44
#define SYSCALL_RECVMSG 47

// Bitmaps to keep track of which device ids to listen on
#define mace_in_set(id, set) ((1 << (id)) & (set))
#define mace_add_set(id, set) (set) |= 1 << (id)
unsigned long inner_devs = 0;
unsigned long outer_devs = 0;

// Tracepoint pointers kept for cleanup
static struct tracepoint *sys_enter_tracepoint;
static struct tracepoint *net_dev_start_xmit_tracepoint;
static struct tracepoint *napi_gro_receive_entry_tracepoint;
static struct tracepoint *sys_exit_tracepoint;

//
// Egress inner tracepoint
//
void
probe_sys_enter(void *unused, struct pt_regs *regs, long id)
{
  if (id == SYSCALL_SENDTO) {
    // Assuming user messages starts at beginning of payload
    register_entry_egress((u64 *)regs->si);
  }
}

//
// Egress outer tracepoint
//
void
probe_net_dev_start_xmit(void *unused, struct sk_buff *skb, struct net_device *dev)
{
  struct iphdr *ip;
  u64 key;
  
  // Filter for outer devices
  if (mace_in_set(dev->ifindex, outer_devs)) {

    // Get key from payload bytes and store in table
    ip = (struct iphdr *)(skb->data + sizeof(struct ethhdr));
    check_ipv4(ip);

    key =*((u64 *)(skb->data + ip->ihl * 4 + sizeof(struct ethhdr)));
    register_exit_egress(key);
  }
}

//
// Ingress outer entry tracepoint.
//
void
probe_napi_gro_receive_entry(void *unused, struct sk_buff *skb)
{
  struct iphdr *ip;

  // Filter for outer device
  if (skb->dev && mace_in_set(skb->dev->ifindex, outer_devs)) {
    
    // Get key from payload bytes and store in ingress table
    ip = (struct iphdr *)skb->data;
    check_ipv4(ip);

    register_entry_ingress((u64 *)(skb->data + ip->ihl * 4));
  }
}

//
// Ingress inner exit tracepoint
//
void
probe_sys_exit(void *unused, struct pt_regs *regs, long ret)
{
  struct user_msghdr *msg;
  struct iphdr *ip;
  u64 key;

  if (syscall_get_nr(current, regs) == SYSCALL_RECVMSG) {
    msg = (struct user_msghdr *)regs->si;
    if (msg) {

      // Raw socket gives us the ip header,
      // probably will need a switch here based on socket type.
      ip = (struct iphdr *)msg->msg_iov->iov_base;
      check_ipv4(ip);

      key = *((u64 *)(msg->msg_iov->iov_base + ip->ihl * 4));
      register_exit_ingress(key);
    }
  }
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
  } else if (!strcmp(tp->name, "net_dev_start_xmit")) {
    ret = tracepoint_probe_register(tp, probe_net_dev_start_xmit, NULL);
    net_dev_start_xmit_tracepoint = tp;
    found = 1;
  } else if (!strcmp(tp->name, "napi_gro_receive_entry")) {
    ret = tracepoint_probe_register(tp, probe_napi_gro_receive_entry, NULL);
    napi_gro_receive_entry_tracepoint = tp;
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
// Module initialization
//
int __init
mace_mod_init(void)
{
  int ret = 0;
  sys_enter_tracepoint = NULL;
  net_dev_start_xmit_tracepoint = NULL;
  napi_gro_receive_entry_tracepoint = NULL;
  sys_exit_tracepoint = NULL;

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

  // Add initial params to dev sets for now
  mace_add_set(outer_dev, outer_devs);
  mace_add_set(inner_dev, inner_devs);

  // Set tracepoints
  for_each_kernel_tracepoint(test_and_set_traceprobe, NULL);

  // Initialize sysfs entries
  mace_init_sysfs();

  printk(KERN_INFO "Mace: running.\n");
  return 0;
}

//
// Module clean up
//
void __exit
mace_mod_exit(void)
{
  // Unregister tracepoints
  if (sys_enter_tracepoint
      && tracepoint_probe_unregister(sys_enter_tracepoint, probe_sys_enter, NULL)) {
    printk(KERN_WARNING "Mace: Failed to unregister sys_enter traceprobe.\n");
  }
  if (net_dev_start_xmit_tracepoint
      && tracepoint_probe_unregister(net_dev_start_xmit_tracepoint, probe_net_dev_start_xmit, NULL)) {
    printk(KERN_WARNING "Mace: Failed to unregister net_dev_start_xmit traceprobe.\n");
  }
  if (napi_gro_receive_entry_tracepoint
      && tracepoint_probe_unregister(napi_gro_receive_entry_tracepoint, probe_napi_gro_receive_entry, NULL)) {
    printk(KERN_WARNING "Mace: Failed to unregister napi_gro_receive_entry_tracepoint.\n");
  }
  if (sys_exit_tracepoint
      && tracepoint_probe_unregister(sys_exit_tracepoint, probe_sys_exit, NULL)) {
    printk(KERN_WARNING "Mace: Failed to unregister sys_exit traceprobe.\n");
  }

  // Cleanup sysfs entries
  mace_free_sysfs();

  printk(KERN_INFO "Mace: stopped.\n");
}

module_init(mace_mod_init);
module_exit(mace_mod_exit);
