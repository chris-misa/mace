//
// Initial MACE module work
//
// Macros to implement MACE inter-device accounting.
// Note: hash_add requires an array (not a pointer)
// so we cannot implement these as inline functions.
//
// 2019, Chris Misa
//

#ifndef MACE_MACROS_H
#define MACE_MACROS_H

#include <asm/bitops.h>
#include <linux/bitops.h>
#include <linux/spinlock.h>
#include <linux/list.h>

//
// Active namespace list struct
//
struct mace_ns_list {
  unsigned long ns_id;
  struct list_head list;
};


// #define DEBUG

// Syscall numbers. . .waiting for a better day
#define SYSCALL_SENDTO 44
#define SYSCALL_RECVMSG 47

// Bitmaps to keep track of which device ids to listen on
#define mace_in_set(id, set) test_bit(id, &(set))
#define mace_add_set(id, set) set_bit(id, &(set))
#define mace_remove_set(id, set) clear_bit(id, &(set))
#define mace_set_foreach(bit, set) \
  for_each_set_bit(bit, &(set), sizeof(set))


#define mace_add_ns(nsid, target_list) \
{ \
  struct mace_ns_list *n = \
      (struct mace_ns_list *)kmalloc(sizeof(struct mace_ns_list), GFP_KERNEL); \
  if (!n) { \
    printk(KERN_INFO "Mace: failed to add item to nsid list\n"); \
  } else { \
    n->ns_id = (nsid); \
    list_add(&n->list, &(target_list)); \
  } \
}

#define mace_lookup_ns(nsid, target_list, result) \
{ \
  struct list_head *p; \
  struct mace_ns_list *n; \
  list_for_each(p, &(target_list)) { \
    n = list_entry(p, struct mace_ns_list, list); \
    if ((nsid) == n->ns_id) { \
      *(result) = 1; \
      break; \
    } \
  } \
  *(result) = 0; \
}

/*
 * Register entry of packet into latency segment.
 *
 *   table: name of static table
 *   key: key generated from packet data
 *
 */
#define register_entry(table, key) \
{ \
  int index = hash_min((key), MACE_LATENCY_TABLE_BITS); \
  long unsigned flags; \
  \
  spin_lock_irqsave(&table[index].lock, flags); \
  table[index].enter = rdtsc(); \
  table[index].key = (key); \
  table[index].valid = 1; \
  spin_unlock_irqrestore(&table[index].lock, flags); \
}

/*
 * Generate code to register exit of packet from latency segment
 *
 *   hash_table: the kernel hash table object
 *   key: hash key
 *   direction: mace_latency_type to hand to mace_push_event()
 */
#define register_exit(table, key, direction) \
{ \
  int index = hash_min((key), MACE_LATENCY_TABLE_BITS); \
  unsigned long long dt = 0; \
  unsigned long flags; \
  \
  spin_lock_irqsave(&table[index].lock, flags); \
  if (table[index].key == (key) && table[index].valid) { \
    dt = rdtsc() - table[index].enter; \
    table[index].valid = 0; \
  } \
  spin_unlock_irqrestore(&table[index].lock, flags); \
  \
  if (dt != 0) { \
    mace_push_event(dt, direction); \
  } \
}



#endif
