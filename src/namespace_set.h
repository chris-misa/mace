//
// Initial MACE module work
//
// Functions to implement namespace set accounting
//
// 2019, Chris Misa
//


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/radix-tree.h>
#include <linux/slab.h>

#include "ring_buffer.h"

#ifndef MACE_NAMESPACE_SET_H
#define MACE_NAMESPACE_SET_H

//
// Active namespace list struct
//
struct mace_namespace_entry {
  unsigned long ns_id;
  struct mace_ring_buffer buf;
};

//
// Adds a mace_namespace_entry for the given ns id
//
// Returns:
//   0 on success
//   -ENOMEM if faild to allocate memory
//   -EEXIST if the ns id was already active
//
int mace_add_ns(struct radix_tree_root *namespaces, unsigned long ns_id);

//
// Removes and frees the mace_namespace_entry associated with the given ns id
// Fails silently if the ns id was not active
//
void mace_del_ns(struct radix_tree_root *namespaces, unsigned long ns_id);

//
// Removes and frees all entries mace_namespaces
//
void mace_del_all_ns(struct radix_tree_root *namespaces);

//
// Returns a pointer to the mace_namespace_entry associated with the given ns id
// or NULL if the ns id is not active
//
struct mace_namespace_entry * mace_get_ns(struct radix_tree_root *namespaces, unsigned long ns_id);

#endif
