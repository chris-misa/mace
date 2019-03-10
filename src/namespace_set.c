//
// Initial MACE module work
//
// Functions to implement namespace set accounting
//
// 2019, Chris Misa
//
#include "namespace_set.h"

//
// Active namespace list
//
RADIX_TREE(mace_namespaces, GFP_KERNEL);

//
// Adds a mace_namespace_entry for the given ns id
//
// Returns:
//   0 on success
//   -ENOMEM if faild to allocate memory
//   -EEXIST if the ns id was already active
//
int
mace_add_ns(struct radix_tree_root *namespaces, unsigned long ns_id)
{
  struct mace_namespace_entry *ent = NULL;
  int res = 0;

  ent = (struct mace_namespace_entry *)kmalloc(sizeof(struct mace_namespace_entry), GFP_KERNEL);
  if (ent != NULL) {
    mace_ring_buffer_init(&ent->buf);
    ent->ns_id = ns_id;
    if ((res = radix_tree_insert(namespaces, ns_id, ent)) != 0) {
      goto cleanup_out;
    }
  } else {
    res = -ENOMEM;
  }
  return res;

cleanup_out:
  if (ent) {
    kfree(ent);
  }
  return res;
}

//
// Removes and frees the mace_namespace_entry associated with the given ns id
// Fails silently if the ns id was not active
//
void
mace_del_ns(struct radix_tree_root *namespaces, unsigned long ns_id)
{
  struct mace_namespace_entry *ent;
  ent = radix_tree_delete(namespaces, ns_id);
  if (ent) {
    kfree(ent);
  }
}

//
// Removes and frees all entries mace_namespaces
//
void
mace_del_all_ns(struct radix_tree_root *namespaces)
{
  struct radix_tree_iter iter;
  void **ent;
  unsigned long ns_id;

  radix_tree_for_each_slot(ent, namespaces, &iter, 0) {
    if (*ent) {
      ns_id = (*((struct mace_namespace_entry **)ent))->ns_id;
      kfree(*ent);
      radix_tree_delete(namespaces, ns_id);
    }
  }
}
