//
// Initial MACE module work
//
// 2019, Chris Misa
//

#ifndef MACE_RING_BUFFER_H
#define MACE_RING_BUFFER_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <asm/atomic.h>

typedef enum {
  MACE_LATENCY_EGRESS,
  MACE_LATENCY_INGRESS
} mace_latency_type;

struct mace_latency_event {
  unsigned long long latency;
  unsigned long long ts;
  mace_latency_type type;
  unsigned long ns_id;
};

#define MACE_EVENT_QUEUE_BITS 8 // Must be less than sizeof(atomic_t) * 8
#define MACE_EVENT_QUEUE_SIZE (1 << MACE_EVENT_QUEUE_BITS)
#define MACE_EVENT_QUEUE_MASK (MACE_EVENT_QUEUE_SIZE - 1)

struct mace_ring_buffer {
  struct mace_latency_event queue[MACE_EVENT_QUEUE_SIZE];
  atomic_t read;
  atomic_t write;
};

struct mace_ring_buffer * mace_get_buf(void);
void mace_push_event(unsigned long long latency,
                     mace_latency_type type,
                     unsigned long ns_id,
                     unsigned long long ts);
struct mace_latency_event * mace_pop_event(void);
void mace_buffer_clear(void);
char * mace_latency_type_str(mace_latency_type type);

#endif
