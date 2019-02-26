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

__always_inline int
mace_add_ns(unsigned long nsid, struct list_head *list_ptr)
{
  struct mace_ns_list *n =
      (struct mace_ns_list *)kmalloc(sizeof(struct mace_ns_list), GFP_KERNEL);
  if (!n) {
    printk(KERN_INFO "Mace: failed to add item to nsid list\n");
    return 1;
  } else {
    list_add(&n->list, list_ptr);
    return 0;
  }
}

__always_inline int
mace_lookup_ns(unsigned long nsid, struct list_head *list_ptr)
{
  struct list_head *p;
  struct mace_ns_list *entry;
  list_for_each(p, list_ptr) {
    entry = list_entry(p, struct mace_ns_list, list);
    if (nsid == entry->nsid) {
      return 1;
    }
  }
  return 0;
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
