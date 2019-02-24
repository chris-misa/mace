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


// Bitmaps to keep track of which device ids to listen on
#define mace_in_set(id, set) ((1 << (id)) & (set))
#define mace_add_set(id, set) (set) |= 1 << (id)

static unsigned long inner_devs = 0;
static unsigned long outer_devs = 0;

// Tracepoint pointers kept for cleanup
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
  struct sk_buff *skb;
  int valid;
  u64 key;
  struct hlist_node hash_list;
};

// Egress latency table
static struct mace_latency egress_latencies[MACE_LATENCY_TABLE_SIZE];
static int egress_latencies_index = 0;
static DEFINE_HASHTABLE(egress_hash, MACE_LATENCY_TABLE_BITS);

// Ingress latency table
static struct mace_latency ingress_latencies[MACE_LATENCY_TABLE_SIZE];
static int ingress_latencies_index = 0;
static DEFINE_HASHTABLE(ingress_hash, MACE_LATENCY_TABLE_BITS);

//
// Egress inner tracepoint
//
static void
probe_sys_enter(void *unused, struct pt_regs *regs, long id)
{
  if (id == SYSCALL_SENDTO) {

    // Assuming user messages starts at beginning of payload
    // This will probably not be true for layer 4 sockets
    register_entry(egress_latencies,
                   egress_latencies_index,
                   egress_hash,
                   (u64 *)regs->si);
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
    register_exit(egress_hash, key, MACE_LATENCY_EGRESS);
  }
}

//
// Ingress outer entry tracepoint.
//
static void
probe_napi_gro_receive_entry(void *unused, struct sk_buff *skb)
{
  struct iphdr *ip;

  // Filter for outer device
  if (skb->dev && mace_in_set(skb->dev->ifindex, outer_devs)) {
    
    // Get key from payload bytes and store in ingress table
    ip = (struct iphdr *)skb->data;
    check_ipv4(ip);

    register_entry(ingress_latencies,
                   ingress_latencies_index,
                   ingress_hash,
                   (u64 *)(skb->data + ip->ihl * 4));
  }
}

//
// Ingress inner exit tracepoint
//
static void
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
      register_exit(ingress_hash, key, MACE_LATENCY_INGRESS);
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

