#ifndef _RBTREE_H_
#define _RBTREE_H_

typedef int rbtree_key_t;

typedef enum {
  RED,
  BLACK,
} rbtree_color_t;

typedef struct rbtree_node rbtree_node_t;
struct rbtree_node {
  rbtree_color_t color;
  rbtree_key_t key;
  rbtree_node_t* left;
  rbtree_node_t* right;
  rbtree_node_t* parent;
};

#ifdef DEBUG_MEM
extern int rbtree_nalloc;
#endif
extern rbtree_node_t* rbtree_nil;
rbtree_node_t* rbtree_node_new(rbtree_key_t key);
rbtree_node_t* rbtree_insert(rbtree_node_t* root, rbtree_node_t* z);
rbtree_node_t* rbtree_delete(rbtree_node_t* root, rbtree_node_t* z);
rbtree_node_t* rbtree_find(rbtree_node_t* root, rbtree_key_t key);
void rbtree_destroy(rbtree_node_t* root);

static inline rbtree_node_t* rbtree_insert_key(rbtree_node_t* root, rbtree_key_t key) {
  return rbtree_insert(root, rbtree_node_new(key));
}

static inline rbtree_node_t* rbtree_delete_key(rbtree_node_t* root, rbtree_key_t key) {
  return rbtree_delete(root, rbtree_find(root, key));
}

#endif
