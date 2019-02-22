//
// Initial MACE module work
//
// 2019, Chris Misa
//

typedef enum {
  MACE_LATENCY_EGRESS,
  MACE_LATENCY_INGRESS
} mace_latency_type;

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
  
struct mace_latency_event {
  unsigned long long latency;
  mace_latency_type type;
};

#define MACE_EVENT_QUEUE_SIZE 1024
static struct mace_latency_event *event_queue;
static unsigned long event_read_head = 0;
static unsigned long event_write_head = 0;


__always_inline void
mace_push_event(unsigned long long latency, mace_latency_type type)
{
  event_queue[event_write_head].latency = latency;
  event_queue[event_write_head].type = type;
  event_write_head = (event_write_head + 1) % MACE_EVENT_QUEUE_SIZE;
}

__always_inline struct mace_latency_event *
mace_pop_event()
{
  struct mace_latency_event *ret = event_queue + event_read_head;
  if (event_read_head == event_write_head) {
    return NULL;
  } else {
    event_read_head = (event_read_head + 1) % MACE_EVENT_QUEUE_SIZE;
    return ret;
  }
}

__always_inline void
mace_buffer_clear()
{
  event_read_head = event_write_head;
}
