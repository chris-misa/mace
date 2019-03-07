//
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

#define MACE_EVENT_QUEUE_BITS 16 // Must be less than sizeof(atomic_t) * 8
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
// Initial MACE module work
//
// 2019, Chris Misa
//

#include "ring_buffer.h"

struct mace_ring_buffer mace_buf = {
  {},
  ATOMIC_INIT(0),
  ATOMIC_INIT(0)
};

char *
mace_latency_type_str(mace_latency_type type)
{
  switch (type) {
    case MACE_LATENCY_EGRESS:
      return "egress";
      break;
    case MACE_LATENCY_INGRESS:
      return "ingress";
      break;
    default:
      return "unknown";
      break;
  }
}

struct mace_ring_buffer *
mace_get_buf(void)
{
  return &mace_buf;
}

//
// TODO: Inline this
//
void
mace_push_event(unsigned long long latency,
                mace_latency_type type,
                unsigned long ns_id,
                unsigned long long ts)
{
  // Repeated 'and' operation for race safety without locking
  int w = atomic_inc_return(&mace_buf.write)
          & MACE_EVENT_QUEUE_MASK;
  atomic_and(MACE_EVENT_QUEUE_MASK, &mace_buf.write);

  mace_buf.queue[w].latency = latency;
  mace_buf.queue[w].type = type;
  mace_buf.queue[w].ns_id = ns_id;
  mace_buf.queue[w].ts = ts;
}

struct mace_latency_event *
mace_pop_event(void)
{
  /*
  struct mace_latency_event *ret = mace_buf.queue + mace_buf.read;
  if (mace_buf.read == mace_buf.write) {
    return NULL;
  } else {
    mace_buf.read = (mace_buf.read + 1) % MACE_EVENT_QUEUE_SIZE;
    return ret;
  }
  */
  return NULL;
}

void
mace_buffer_clear(void)
{
  mace_buf.read = mace_buf.write;
}
