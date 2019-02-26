//
// Initial MACE module work
//
// Macros to implement MACE inter-device accounting.
// Note: hash_add requires an array (not a pointer)
// so we cannot implement these as inline functions.
//
// 2019, Chris Misa
//

// Bitmaps to keep track of which device ids to listen on
#define mace_in_set(id, set) test_bit(id, &(set))
#define mace_add_set(id, set) set_bit(id, &(set))
#define mace_remove_set(id, set) clear_bit(id, &(set))

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
