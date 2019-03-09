//
// Initial MACE module work
//
// 2019, Chris Misa
//

#include "ring_buffer.h"

void
mace_ring_buffer_init(struct mace_ring_buffer *rb)
{
  int i;
  for (i = 0; i < MACE_EVENT_QUEUE_SIZE; i++) {
    atomic_set(&rb->queue[i].writing, 1);
  }
  atomic_set(&rb->read, 0);
  atomic_set(&rb->write, 0);
}

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

__always_inline void
mace_push_event(struct mace_ring_buffer *buf,
                unsigned long long latency,
                mace_latency_type type,
                unsigned long ns_id,
                unsigned long long ts)
{
  int w;

  // Double 'and' for race-safe modular arithmetic without locking
  w = atomic_inc_return(&buf->write) & MACE_EVENT_QUEUE_MASK;
  atomic_and(MACE_EVENT_QUEUE_MASK, &buf->write);

  // Let any reading threads know we're changing this entry
  atomic_set(&buf->queue[w].writing, 1);

  // Write in the data (member-wise to avoid in_read)
  buf->queue[w].latency = latency;
  buf->queue[w].type = type;
  buf->queue[w].ns_id = ns_id;
  buf->queue[w].ts = ts;
}

int
mace_pop_event(struct mace_ring_buffer *buf, struct mace_latency_event *evt)
{
  int r;

  // Check for empty queue
  if (atomic_read(&buf->read) == atomic_read(&buf->write)) {
    return 2;
  }

  // Increment read head
  r = atomic_inc_read(&buf->read);
  atomic_and(MACE_EVENT_QUEUE_MASK, &buf->read);

  // Mark start of read
  atomic_set(&buf->queue[r].writing, 0);

  // Read out the data
  evt->latency = buf->queue[r].latency;
  evt->type = buf->queue[r].type;
  evt->ns_id = buf->queue[r].ns_id;
  evt->ts = buf->queue[r].ts;

  // Check if any writes hit this entry
  return atomic_xchg(&buf->queue[r].writing, 1);
}

void
mace_buffer_clear(struct mace_ring_buffer *buf)
{
  atomic_set(&buf->read, atomic_read(&buf->write));
}
