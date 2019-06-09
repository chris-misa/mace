//
// Initial MACE module work
//
// 2019, Chris Misa
//

#include "module.h"


// Test for ok ipv4 packets
#ifdef DEBUG
  #define check_ipv4(ip) \
    if (!(ip) || (ip)->version != 4) { \
      printk(KERN_INFO "Mace: Ignoring non-ipv4 packet.\n"); \
      return; \
    }
#else
  #define check_ipv4(ip) \
    if (!(ip) || (ip)->version != 4) { \
      return; \
    }
#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chris Misa <cmisa@cs.uoregon.edu>");
MODULE_DESCRIPTION("Test of sysfs file system");

//
// Module life-cycle functions
//
int __init mace_mod_init(void);
void __exit mace_mod_exit(void);
module_init(mace_mod_init);
module_exit(mace_mod_exit);

//
// Param: Outer device id
//
static int outer_dev = -1;
module_param(outer_dev, int, 0);
MODULE_PARM_DESC(outer_dev, "Device id of the outer device");

//
// Network device set (bitmap)
//
static unsigned long outer_devs = 0;

//
// Set of mace-active namespaces
// Defined in namespace_set.c
//
extern struct radix_tree_root mace_namespaces;

//
// Record of approximate difference between tsc and gettimeofday clocks
//
struct timeval mace_tsc_offset;

//
// Tracepoint pointers kept for cleanup
//
static struct tracepoint *sys_enter_tracepoint;
static struct tracepoint *net_dev_start_xmit_tracepoint;
static struct tracepoint *netif_receive_skb_tracepoint;
static struct tracepoint *sys_exit_tracepoint;

//
// In-kernel packet tracking
//
#define MACE_LATENCY_TABLE_BITS 16
#define MACE_LATENCY_TABLE_SIZE (1 << MACE_LATENCY_TABLE_BITS)


//
// Perturbation counters
//
#ifdef MACE_PERT_ENABLED
struct mace_perturbation mace_sys_enter_pert;
struct mace_perturbation mace_net_dev_start_xmit_pert;
struct mace_perturbation mace_netif_receive_skb_pert;
struct mace_perturbation mace_sys_exit_pert;

struct mace_perturbation mace_register_entry_pert;
struct mace_perturbation mace_register_exit_pert;
struct mace_perturbation mace_push_event_pert;
#endif

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
mace_probe_sys_enter(void *unused, struct pt_regs *regs, long id)
{
  struct mace_namespace_entry *ns;
  u64 key;
  struct res;
  START_PERT_TIMER(enter);

  // Filter for sendto syscalls
  if (id == SYSCALL_SENDTO) {
    
    // Filter by net namespace id
    ns = mace_get_ns(&mace_namespaces, current->nsproxy->net_ns->ns.inum);
    if (ns != NULL) {

      // Assuming user messages starts at beginning of payload
      // This will probably not be true for layer 4 sockets
      // key = *((u64 *)regs->si);
      copy_from_user(&key, (void *)regs->si, sizeof(u64));
      register_entry(egress_latencies, key, ns);

      STOP_PERT_TIMER(enter, mace_sys_enter_pert);
    }
  }
}

//
// Egress outer tracepoint
//
static void
mace_probe_net_dev_start_xmit(void *unused, struct sk_buff *skb, struct net_device *dev)
{
  struct iphdr *ip;
  u64 key;
  unsigned long pktid;
  START_PERT_TIMER(enter);
  
  // Filter for outer devices
  if (mace_in_set(dev->ifindex, outer_devs)) {

    // Get key from payload bytes and store in table
    ip = (struct iphdr *)(skb->data + sizeof(struct ethhdr));
    check_ipv4(ip);

    key = *((u64 *)(skb->data + ip->ihl * 4 + sizeof(struct ethhdr)));
    pktid = be16_to_cpu(*(((u16*)&key) + 3));
    register_exit(egress_latencies, key, MACE_LATENCY_EGRESS, NULL, pktid);

#ifdef DEBUG
    {
      int n = skb_headlen(skb);
      int i;
      unsigned char *d_ptr = skb->data + sizeof(struct ethhdr);
      printk(KERN_INFO "Mace: net_dev_start_xmit key: %016llX\n", key);
      for (i = 0; i < n; i += 8) {
        printk(KERN_INFO "Mace: %02x %02x %02x %02x %02x %02x %02x %02x\n",
            d_ptr[i], d_ptr[i+1], d_ptr[i+2], d_ptr[i+3],
            d_ptr[i+4], d_ptr[i+5], d_ptr[i+6], d_ptr[i+7]);
      }
    }
#endif

    STOP_PERT_TIMER(enter, mace_net_dev_start_xmit_pert);
  }
}

//
// Ingress outer entry tracepoint.
//
static void
mace_probe_netif_receive_skb(void *unused, struct sk_buff *skb)
{
  struct iphdr ip;
  struct iphdr *ip_ptr = &ip;
  u64 key;
  u64 *key_ptr = &key;
  START_PERT_TIMER(enter);

  // Filter for outer device
  if (skb->dev && mace_in_set(skb->dev->ifindex, outer_devs)) {
    
    // Get key from payload bytes and store in ingress table
    ip_ptr = skb_header_pointer(skb, 0, sizeof(struct iphdr), ip_ptr);
    ip = *ip_ptr;
    
    key_ptr = skb_header_pointer(skb, ip.ihl * 4, sizeof(key), key_ptr);
    key = *key_ptr;

    register_entry(ingress_latencies, key, NULL);

#ifdef DEBUG
    {
      int n = skb_headlen(skb);
      int i;
      unsigned char *d_ptr = skb->data;
      printk(KERN_INFO "Mace: netif_receive_skb key: %016llX len: %d data_len: %d\n", key, skb->len, skb->data_len);

      for (i = 0; i < n; i += 8) {
        printk(KERN_INFO "Mace: %02x %02x %02x %02x %02x %02x %02x %02x\n",
            d_ptr[i], d_ptr[i+1], d_ptr[i+2], d_ptr[i+3],
            d_ptr[i+4], d_ptr[i+5], d_ptr[i+6], d_ptr[i+7]);
      }
    }
#endif

    STOP_PERT_TIMER(enter, mace_netif_receive_skb_pert);
  }
}

//
// Ingress inner exit tracepoint
//
static void
mace_probe_sys_exit(void *unused, struct pt_regs *regs, long ret)
{
  struct user_msghdr msg;
  struct iovec iov;
  struct iphdr ip;
  u64 key;
  struct mace_namespace_entry *ns = NULL;
  unsigned long pktid;
  START_PERT_TIMER(enter);

  // Filter by syscall number
  if (syscall_get_nr(current, regs) == SYSCALL_RECVMSG) {

    // Filter by net namespace id
    ns = mace_get_ns(&mace_namespaces, current->nsproxy->net_ns->ns.inum);
    if (ns != NULL) {

      copy_from_user(&msg, (void *)regs->si, sizeof(struct user_msghdr));
      copy_from_user(&iov, msg.msg_iov, sizeof(struct iovec));

      // Raw socket gives us the ip header,
      // Might need to check for L4 protocols here.
      copy_from_user(&ip, iov.iov_base, sizeof(struct iphdr));

      if (ip.version != 4) {
        return;
      }
      copy_from_user(&key, iov.iov_base + ip.ihl * 4, 8);
      pktid = be16_to_cpu(*(((u16*)&key) + 3));
      register_exit(ingress_latencies, key, MACE_LATENCY_INGRESS, ns, pktid);

#ifdef DEBUG
      {
        unsigned char *d_ptr = (unsigned char *)&key;
        printk(KERN_INFO "Mace: sys_exit key: %016llX\n", key);
        printk(KERN_INFO "Mace: %02x %02x %02x %02x %02x %02x %02x %02x\n",
        d_ptr[0], d_ptr[1], d_ptr[2], d_ptr[3],
        d_ptr[4], d_ptr[5], d_ptr[6], d_ptr[7]);
      }
#endif

      STOP_PERT_TIMER(enter, mace_sys_exit_pert);
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
    ret = tracepoint_probe_register(tp, mace_probe_sys_enter, NULL);
    sys_enter_tracepoint = tp;
    found = 1;
  } else if (!strcmp(tp->name, "net_dev_start_xmit")) {
    ret = tracepoint_probe_register(tp, mace_probe_net_dev_start_xmit, NULL);
    net_dev_start_xmit_tracepoint = tp;
    found = 1;
  } else if (!strcmp(tp->name, "netif_receive_skb")) {
    ret = tracepoint_probe_register(tp, mace_probe_netif_receive_skb, NULL);
    netif_receive_skb_tracepoint = tp;
    found = 1;
  } else if (!strcmp(tp->name, "sys_exit")) {
    ret = tracepoint_probe_register(tp, mace_probe_sys_exit, NULL);
    sys_exit_tracepoint = tp;
    found = 1;
  }
  
  if (found && ret != 0) {
    printk(KERN_WARNING "Mace: Failed to set tracepoint.\n");
  }
}

//
// Rough synchronization of tsc to gettimeofday for later correlation
//

//
// Subtract current reading of tsc from gettimeofday to get offset
//
void
mace_tsc_offset_resync(void)
{
  unsigned long long cur_tsc_usec;
  unsigned long long usec_offset;

  cur_tsc_usec = mace_cycles_to_ns(rdtsc()) / 1000;
  do_gettimeofday(&mace_tsc_offset);
  
  // Do the carry in 64 bits to avoid wrapping arround
  usec_offset = mace_tsc_offset.tv_usec;
  while (cur_tsc_usec > usec_offset) {
    usec_offset += 1000000;
    mace_tsc_offset.tv_sec--;
  }
  usec_offset -= cur_tsc_usec;
  mace_tsc_offset.tv_usec = usec_offset;
}

//
// Add tsc reading to offset and format as struct timeval
//
void
mace_tsc_to_gettimeofday(unsigned long long tsc_val, struct timeval *tv)
{
  unsigned long long tsc_usec = mace_cycles_to_ns(tsc_val) / 1000;
  unsigned long long tsc_sec = tsc_usec / 1000000;
  *(tv) = mace_tsc_offset;
  tv->tv_sec += tsc_sec;
  tv->tv_usec += (tsc_usec - tsc_sec * 1000000);

  if (tv->tv_usec > 1000000) {
    tv->tv_sec++;
    tv->tv_usec -= 1000000;
  }
}


//
// Initialize pertubation counters
//
void
mace_init_pert(void)
{
#ifdef MACE_PERT_ENABLED
  mace_sys_enter_pert.sum = 0;
  mace_sys_enter_pert.count = 0;
  mace_net_dev_start_xmit_pert.sum = 0;
  mace_net_dev_start_xmit_pert.count = 0;
  mace_netif_receive_skb_pert.sum = 0;
  mace_netif_receive_skb_pert.count = 0;
  mace_sys_exit_pert.sum = 0;
  mace_sys_exit_pert.count = 0;
  mace_register_entry_pert.sum = 0;
  mace_register_entry_pert.count = 0;
  mace_register_exit_pert.sum = 0;
  mace_register_exit_pert.count = 0;
  mace_push_event_pert.sum = 0;
  mace_push_event_pert.count = 0;
#endif
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
  netif_receive_skb_tracepoint = NULL;
  sys_exit_tracepoint = NULL;

  // Check for required parameters
  if (outer_dev < 0) {
    printk(KERN_INFO "Mace: must set outer_dev=<outer device id>\n");
    ret = -1;
  }

  if (ret) {
    return ret;
  }

  // Initialize skb tracking tables
  init_mace_tables();
 
  // Add initial params to dev sets for now
  mace_add_set(outer_dev, outer_devs);

  // Set tracepoints
  for_each_kernel_tracepoint(test_and_set_traceprobe, NULL);

  // Initialize device files
  if (mace_init_dev() < 0) {
    return -1;
  }

  // Initialize perturbation counters
  mace_init_pert();

  // Gettimeofday sync
  mace_tsc_offset_resync();

  printk(KERN_INFO "Mace: running, buffering up to %d latencies, tsc zero around %lu.%06lu.\n",
           MACE_EVENT_QUEUE_SIZE,
           mace_tsc_offset.tv_sec, mace_tsc_offset.tv_usec);
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
      && tracepoint_probe_unregister(sys_enter_tracepoint, mace_probe_sys_enter, NULL)) {
    printk(KERN_WARNING "Mace: Failed to unregister sys_enter traceprobe.\n");
  }
  if (net_dev_start_xmit_tracepoint
      && tracepoint_probe_unregister(net_dev_start_xmit_tracepoint, mace_probe_net_dev_start_xmit, NULL)) {
    printk(KERN_WARNING "Mace: Failed to unregister net_dev_start_xmit traceprobe.\n");
  }
  if (netif_receive_skb_tracepoint
      && tracepoint_probe_unregister(netif_receive_skb_tracepoint, mace_probe_netif_receive_skb, NULL)) {
    printk(KERN_WARNING "Mace: Failed to unregister netif_receive_skb_tracepoint.\n");
  }
  if (sys_exit_tracepoint
      && tracepoint_probe_unregister(sys_exit_tracepoint, mace_probe_sys_exit, NULL)) {
    printk(KERN_WARNING "Mace: Failed to unregister sys_exit traceprobe.\n");
  }

  // Cleanup device files
  mace_free_dev();

  // Free active namespace entries
  mace_del_all_ns(&mace_namespaces);

  printk(KERN_INFO "Mace: stopped.\n");
}

