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
  atomic_t writing;
};

#define MACE_EVENT_QUEUE_BITS 16 // Must be less than sizeof(atomic_t) * 8
#define MACE_EVENT_QUEUE_SIZE (1 << MACE_EVENT_QUEUE_BITS)
#define MACE_EVENT_QUEUE_MASK (MACE_EVENT_QUEUE_SIZE - 1)

struct mace_ring_buffer {
  struct mace_latency_event queue[MACE_EVENT_QUEUE_SIZE];
  atomic_t read;
  atomic_t write;
};

//
// Initialize entries of the given ring buffer
//
void mace_ring_buffer_init(struct mace_ring_buffer *rb);

//
// Helper function for mace_latency_type enum
//
char * mace_latency_type_str(mace_latency_type type);

//
// Push (copy) the given mace_latency_event the specified ring buffer
// Note: pushing to a full queue might loose a queue-full of data
// need a race-safe method kick read head for overwrite semantics.
//
void mace_push_event(struct mace_ring_buffer *buf,
                     unsigned long long latency,
                     mace_latency_type type,
                     unsigned long ns_id,
                     unsigned long long ts);

//
// Pop (copy) the top off the given mace_ring_buffer
// Returns:
//   0 on success,
//   1 if a write mucked up this read and the read data is garbage
//   2 if the queue was empty,
//
int mace_pop_event(struct mace_ring_buffer *buf, struct mace_latency_event *evt);

//
// Clears the given buffer
//
void mace_buffer_clear(struct mace_ring_buffer *buf);

#endif
