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

#include "ring_buffer.h"

//
// Active namespace list struct
//
struct mace_ns_list {
  unsigned long ns_id;
  struct mace_ring_buffer buf;
  struct list_head list;
};

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
    mace_ring_buffer_init(&n->buf); \
    n->ns_id = (nsid); \
    list_add(&n->list, &(target_list)); \
  } \
}

// void mace_lookup_ns(unsigned long nsid,
//                     struct list_head target_list,
//                     int *result)
#define mace_lookup_ns(nsid, target_list, result) \
{ \
  struct list_head *p; \
  struct mace_ns_list *n; \
  *(result) = 0; \
  list_for_each(p, &(target_list)) { \
    n = list_entry(p, struct mace_ns_list, list); \
    if ((nsid) == n->ns_id) { \
      *(result) = 1; \
      break; \
    } \
  } \
}

// void mace_get_ns(unsigned long nsid,
//                  struct list_head target_list,
//                  struct mace_ns_list **result)
#define mace_get_ns(nsid, target_list, result) \
{ \
  struct list_head *p; \
  struct mace_ns_list *n; \
  *(result) = NULL; \
  list_for_each(p, &(target_list)) { \
    n = list_entry(p, struct mace_ns_list, list); \
    if ((nsid) == n->ns_id) { \
      *(result) = n; \
      break; \
    } \
  } \
}

#define mace_del_ns(nsid, target_list) \
{ \
  struct list_head *p; \
  struct list_head *q; \
  struct mace_ns_list *n; \
  list_for_each_safe(p, q, &(target_list)) { \
    n = list_entry(p, struct mace_ns_list, list); \
    if (n->ns_id == nsid) { \
      list_del(p); \
      kfree(n); \
      break; \
    } \
  } \
}

#define mace_del_all_ns(target_list) \
{ \
  struct list_head *p; \
  struct list_head *q; \
  struct mace_ns_list *n; \
  list_for_each_safe(p, q, &(target_list)) { \
    n = list_entry(p, struct mace_ns_list, list); \
    list_del(p); \
    kfree(n); \
  } \
}

/*
 * Register entry of packet into latency segment.
 *
 *   table: name of static table
 *   key: key generated from packet data
 *
 */
#define register_entry(table, key, nsid) \
{ \
  int index = hash_min((key), MACE_LATENCY_TABLE_BITS); \
  long unsigned flags; \
  \
  spin_lock_irqsave(&table[index].lock, flags); \
  table[index].enter = rdtsc(); \
  table[index].ns_id = nsid; \
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
 *   nsid: net namespace id of current context or 0
 */
#define register_exit(table, key, direction, nsid) \
{ \
  int index = hash_min((key), MACE_LATENCY_TABLE_BITS); \
  unsigned long long enter = 0; \
  unsigned long long dt = 0; \
  unsigned long saved_nsid = 0; \
  unsigned long flags; \
  struct mace_ns_list *ns; \
  \
  spin_lock_irqsave(&table[index].lock, flags); \
  if (table[index].key == (key) && table[index].valid) { \
    enter = table[index].enter; \
    saved_nsid = table[index].ns_id; \
    table[index].valid = 0; \
    spin_unlock_irqrestore(&table[index].lock, flags); \
    \
    dt = rdtsc() - enter; \
  } else { \
    spin_unlock_irqrestore(&table[index].lock, flags); \
  } \
  \
  if (dt != 0) { \
    if (saved_nsid == 0) { \
      saved_nsid = nsid; \
    } \
    mace_get_ns(saved_nsid, mace_active_ns, &ns); \
    mace_push_event(&ns->buf, dt, direction, saved_nsid, enter); \
  } \
}



#endif
