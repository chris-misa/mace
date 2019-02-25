//
// Initial MACE module work
//
// Macros to implement MACE inter-device accounting.
// Note: hash_add requires an array (not a pointer)
// so we cannot implement these as inline functions.
//
// 2019, Chris Misa
//


/*
 * Register entry of packet into latency segment.
 *
 *   table: name of static table of struct mace_latency
 *   index: index into above table
 *   hash_table: the kernel hash table object
 *   key_ptr: a pointer to the hash key
 *
 * Note: might need to synchronize instruction re-ordering before valid = 1
 */
#define register_entry(table, index, hash_table, key_ptr) \
{ \
  struct mace_latency *ml; \
  unsigned long long ts = rdtsc(); \
  unsigned long flags; \
  ml = table + atomic_inc_return(&index); \
  if (atomic_read(&index) == MACE_LATENCY_TABLE_SIZE) { \
    atomic_set(&index, 0); \
  } \
  if (ml->valid) { \
    hash_del(&ml->hash_list); \
  } \
  \
  spin_lock_irqsave(&ml->lock, flags); \
  ml->enter = ts; \
  ml->key = *(key_ptr); \
  ml->valid = 1; \
  hash_add(hash_table, &ml->hash_list, ml->key); \
  spin_unlock_irqrestore(&ml->lock, flags); \
}

/*
 * Generate code to register exit of packet from latency segment
 *
 *   hash_table: the kernel hash table object
 *   key: hash key
 *   direction: mace_latency_type to hand to mace_push_event()
 */
#define register_exit(hash_table, key, direction) \
{ \
  struct mace_latency *ml; \
  unsigned long long dt; \
  unsigned long flags; \
  hash_for_each_possible(hash_table, ml, hash_list, key) { \
    if (ml && ml->key == (key)) { \
      /* if this entry is locked, it must be in the process of being overwritten */ \
      if (spin_trylock_irqsave(&ml->lock, flags)) {\
        dt = rdtsc() - ml->enter; \
        ml->valid = 0; \
        hash_del(&ml->hash_list); \
        spin_unlock_irqrestore(&ml->lock, flags); \
        mace_push_event(dt, direction); \
      } \
      break; \
    } \
  } \
}
