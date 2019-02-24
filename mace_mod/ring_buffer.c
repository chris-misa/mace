//
// Initial MACE module work
//
// 2019, Chris Misa
//

#include "ring_buffer.h"

#define MACE_EVENT_QUEUE_SIZE 1024
static struct mace_latency_event event_queue[MACE_EVENT_QUEUE_SIZE];
static unsigned long event_read_head = 0;
static unsigned long event_write_head = 0;

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

void
mace_push_event(unsigned long long latency, mace_latency_type type)
{
  event_queue[event_write_head].latency = latency;
  event_queue[event_write_head].type = type;
  event_write_head = (event_write_head + 1) % MACE_EVENT_QUEUE_SIZE;
}

struct mace_latency_event *
mace_pop_event(void)
{
  struct mace_latency_event *ret = event_queue + event_read_head;
  if (event_read_head == event_write_head) {
    return NULL;
  } else {
    event_read_head = (event_read_head + 1) % MACE_EVENT_QUEUE_SIZE;
    return ret;
  }
}

void
mace_buffer_clear(void)
{
  event_read_head = event_write_head;
}
