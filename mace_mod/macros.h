//
// Initial MACE module work
//
// Defines and macros to implement the inter-tracepoint logic.
//
// 2019, Chris Misa
//


/*
 * Generates code to register entry of packet into latency segment.
 *
 *   table: name of static table of struct mace_latency
 *   index: index into above table
 *   hash_table: the kernel hash table object
 *   key_ptr: a pointer to the hash key
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
 * Executes register_entry() for egress table
 */
#define register_entry_egress(key_ptr) \
  register_entry(egress_latencies, \
                 egress_latencies_index, \
                 egress_hash, \
                 key_ptr)

/*
 * Executes register_entry() for ingress table
 */
#define register_entry_ingress(key_ptr) \
  register_entry(ingress_latencies, \
                 ingress_latencies_index, \
                 ingress_hash, \
                 key_ptr)

/*
 * Generate code to register exit of packet from latency segment
 *
 *   hash_table: the kernel hash table object
 *   key: hash key
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

/*
 * Executes register_exit() for egress table
 */
#define register_exit_egress(key) \
  register_exit(egress_hash, key, MACE_LATENCY_EGRESS)

/*
 * Executes register_exit() for ingress table
 */
#define register_exit_ingress(key) \
  register_exit(ingress_hash, key, MACE_LATENCY_INGRESS)
