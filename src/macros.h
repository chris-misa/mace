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
#include <linux/radix-tree.h>

#include "ring_buffer.h"

//
// Report latency approximations in nano seconds
// Cycles / (1 000 Cycles / sec) * (1 000 000 000 nsec / sec)
//
#define mace_cycles_to_ns(c) (((c) * 1000000) / tsc_khz)

// Syscall numbers. . .waiting for a better day
#define SYSCALL_SENDTO 44
#define SYSCALL_RECVMSG 47

// Bitmaps to keep track of which device ids to listen on
#define mace_in_set(id, set) test_bit(id, &(set))
#define mace_add_set(id, set) set_bit(id, &(set))
#define mace_remove_set(id, set) clear_bit(id, &(set))
#define mace_set_foreach(bit, set) \
  for_each_set_bit(bit, &(set), sizeof(set))

/*
 * Register entry of packet into latency segment.
 *
 *   table: name of static table
 *   key: key generated from packet data
 *   ns: struct mace_namespace_entry pointer or NULL
 *
 */
#define register_entry(table, key, ns_ptr) \
{ \
  int index = hash_min((key), MACE_LATENCY_TABLE_BITS); \
  long unsigned flags; \
  \
  spin_lock_irqsave(&table[index].lock, flags); \
  table[index].enter = rdtsc(); \
  table[index].ns = ns_ptr; \
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
 *   ns: struct mace_namespace_entry pointer or NULL
 */
#define register_exit(table, key, direction, ns_ptr, pkt_id) \
{ \
  int index = hash_min((key), MACE_LATENCY_TABLE_BITS); \
  unsigned long long enter = 0; \
  unsigned long long dt = 0; \
  struct mace_namespace_entry *saved_ns = NULL; \
  unsigned long flags; \
  \
  spin_lock_irqsave(&table[index].lock, flags); \
  if (table[index].key == (key) && table[index].valid) { \
    enter = table[index].enter; \
    saved_ns = table[index].ns; \
    table[index].valid = 0; \
    spin_unlock_irqrestore(&table[index].lock, flags); \
    \
    dt = rdtsc() - enter; \
  } else { \
    spin_unlock_irqrestore(&table[index].lock, flags); \
  } \
  \
  if (dt != 0) { \
    if (saved_ns == NULL && ns_ptr != NULL) { \
      saved_ns = ns_ptr; \
    } \
    \
    if (saved_ns != NULL) { \
      mace_push_event(&saved_ns->buf, dt, direction, saved_ns->ns_id, enter, pkt_id); \
    } \
  } \
}

#endif
