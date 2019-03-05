//
// Initial MACE module work
//
// 2019, Chris Misa
//

#include "module.h"

// Test for ok ipv4 packets
#ifdef DEBUG
  #define check_ipv4(ip) \
    if (!ip || ip->version != 4) { \
      printk(KERN_INFO "Mace: Ignoring non-ipv4 packet.\n"); \
      return; \
    }
#else
  #define check_ipv4(ip) \
    if (!ip || ip->version != 4) { \
      return; \
    }
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris Misa <cmisa@cs.uoregon.edu>");
MODULE_DESCRIPTION("Test of sysfs file system");

int __init mace_mod_init(void);
void __exit mace_mod_exit(void);
module_init(mace_mod_init);
module_exit(mace_mod_exit);

// Param: Outer device id
static int outer_dev = -1;
module_param(outer_dev, int, 0);
MODULE_PARM_DESC(outer_dev, "Device id of the outer device");

//
// Network device set
//
static unsigned long outer_devs = 0;

//
// Active namespace list
//
struct list_head mace_active_ns;

//
// Tracepoint pointers kept for cleanup
//
static struct tracepoint *sys_enter_tracepoint;
static struct tracepoint *net_dev_start_xmit_tracepoint;
static struct tracepoint *napi_gro_receive_entry_tracepoint;
static struct tracepoint *sys_exit_tracepoint;

//
// In-kernel packet tracking
//
#define MACE_LATENCY_TABLE_BITS 8
#define MACE_LATENCY_TABLE_SIZE (1 << MACE_LATENCY_TABLE_BITS)

struct mace_latency {
  unsigned long long enter;
  unsigned long ns_id;
  int valid;
  u64 key;
  spinlock_t lock;
};

// Egress latency table
static struct mace_latency egress_latencies[MACE_LATENCY_TABLE_SIZE];

// Ingress latency table
static struct mace_latency ingress_latencies[MACE_LATENCY_TABLE_SIZE];

static void
mace_latency_init(struct mace_latency *ml)
{
  ml->valid = 0;
  ml->lock = __SPIN_LOCK_UNLOCKED(lock);
}

static void
init_mace_tables(void)
{
  int i;
  for (i = 0; i < MACE_LATENCY_TABLE_SIZE; i++) {
    mace_latency_init(egress_latencies + i);
    mace_latency_init(ingress_latencies + i);
  }
}

//
// Egress inner tracepoint
//
static void
probe_sys_enter(void *unused, struct pt_regs *regs, long id)
{
  u64 key;
  unsigned long ns_id;
  int res;

  if (id == SYSCALL_SENDTO) {
    
    // Filter by net namespace id
    ns_id = current->nsproxy->net_ns->ns.inum;
    mace_lookup_ns(ns_id, mace_active_ns, &res);
    if (res) {

      // Assuming user messages starts at beginning of payload
      // This will probably not be true for layer 4 sockets
      key = *((u64 *)regs->si);
      register_entry(egress_latencies, key, ns_id);
    }
  }
}

//
// Egress outer tracepoint
//
static void
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
    register_exit(egress_latencies, key, MACE_LATENCY_EGRESS, 0);
  }
}

//
// Ingress outer entry tracepoint.
//
static void
probe_napi_gro_receive_entry(void *unused, struct sk_buff *skb)
{
  struct iphdr *ip;
  u64 key;

  // Filter for outer device
  if (skb->dev && mace_in_set(skb->dev->ifindex, outer_devs)) {
    
    // Get key from payload bytes and store in ingress table
    ip = (struct iphdr *)skb->data;
    check_ipv4(ip);

    key = *((u64 *)(skb->data + ip->ihl * 4));

    register_entry(ingress_latencies, key, 0);
  }
}

//
// Ingress inner exit tracepoint
//
static void
probe_sys_exit(void *unused, struct pt_regs *regs, long ret)
{
  unsigned long ns_id;
  int res;
  struct user_msghdr *msg;
  struct iphdr *ip;
  u64 key;

  // Filter by syscall number
  if (syscall_get_nr(current, regs) == SYSCALL_RECVMSG) {

    // Filter by net namespace id
    ns_id = current->nsproxy->net_ns->ns.inum;
    mace_lookup_ns(ns_id, mace_active_ns, &res);
    if (res) {

      msg = (struct user_msghdr *)regs->si;
      if (msg) {

        // Raw socket gives us the ip header,
        // probably will need a switch here based on socket type.
        ip = (struct iphdr *)msg->msg_iov->iov_base;
        check_ipv4(ip);

        key = *((u64 *)(msg->msg_iov->iov_base + ip->ihl * 4));
        register_exit(ingress_latencies, key, MACE_LATENCY_INGRESS, ns_id);
      }
    }
  }
}

//
// Identify target tracepoints by name
//
static void
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
  INIT_LIST_HEAD(&mace_active_ns);

  // Check for required parameters
  if (outer_dev < 0) {
    printk(KERN_INFO "Mace: must set outer_dev=<outer device id>\n");
    ret = -1;
  }

  if (ret) {
    return ret;
  }

  init_mace_tables();
 
  // Add initial params to dev sets for now
  mace_add_set(outer_dev, outer_devs);

  // Set tracepoints
  for_each_kernel_tracepoint(test_and_set_traceprobe, NULL);

  // Initialize device files
  if (mace_init_dev() < 0) {
    return -1;
  }

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

  // Cleanup device files
  mace_free_dev();

  // Free active entries
  mace_del_all_ns(mace_active_ns);

  printk(KERN_INFO "Mace: stopped.\n");
}

