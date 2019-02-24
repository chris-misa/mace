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
  ml = table + (index++); \
  if (index == MACE_LATENCY_TABLE_SIZE) { \
    index = 0; \
  } \
  \
  if (ml->valid) { \
    hash_del(&ml->hash_list); \
  } \
  \
  ml->enter = ts; \
  ml->key = *(key_ptr); \
  ml->valid = 1; \
  hash_add(hash_table, &ml->hash_list, ml->key); \
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
  hash_for_each_possible(hash_table, ml, hash_list, key) { \
    if (ml && ml->key == (key)) { \
      dt = rdtsc() - ml->enter; \
      ml->valid = 0; \
      hash_del(&ml->hash_list); \
      mace_push_event(dt, direction); \
      break; \
    } \
  } \
}
