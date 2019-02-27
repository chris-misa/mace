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

typedef enum {
  MACE_LATENCY_EGRESS,
  MACE_LATENCY_INGRESS
} mace_latency_type;

struct mace_latency_event {
  unsigned long long latency;
  unsigned long ns_id;
  mace_latency_type type;
};

void mace_push_event(unsigned long long latency,
                     mace_latency_type type,
                     unsigned long ns_id);
struct mace_latency_event * mace_pop_event(void);
void mace_buffer_clear(void);
char * mace_latency_type_str(mace_latency_type type);

#endif
