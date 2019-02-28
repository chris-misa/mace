//
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
                unsigned long ns_id)
{
  // Repeated 'and' operation for race safety without locking
  int w = atomic_inc_return(&mace_buf.write)
          & MACE_EVENT_QUEUE_MASK;
  atomic_and(MACE_EVENT_QUEUE_MASK, &mace_buf.write);

  mace_buf.queue[mace_buf.write].latency = latency;
  mace_buf.queue[mace_buf.write].type = type;
  mace_buf.queue[mace_buf.write].ns_id = ns_id;
}

struct mace_latency_event *
mace_pop_event(void)
{
  struct mace_latency_event *ret = mace_buf.queue + mace_buf.read;
  if (mace_buf.read == mace_buf.write) {
    return NULL;
  } else {
    mace_buf.read = (mace_buf.read + 1) % MACE_EVENT_QUEUE_SIZE;
    return ret;
  }
}

void
mace_buffer_clear(void)
{
  mace_buf.read = mace_buf.write;
}
