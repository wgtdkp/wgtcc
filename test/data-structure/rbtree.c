#include "rbtree.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

rbtree_node_t* rbtree_nil = &(rbtree_node_t) {.color = BLACK};
static rbtree_node_t* rotate_left(rbtree_node_t* root, rbtree_node_t* x);
static rbtree_node_t* rotate_right(rbtree_node_t* root, rbtree_node_t* y);
static rbtree_node_t* rbtree_insert_fixup(rbtree_node_t* root, rbtree_node_t* z);
static rbtree_node_t* rbtree_transplant(rbtree_node_t* root,
    rbtree_node_t* u, rbtree_node_t* v);
static rbtree_node_t* rbtree_minimum(rbtree_node_t* root);
static rbtree_node_t* rbtree_maximum(rbtree_node_t* root) __attribute__((unused));
static rbtree_node_t* rbtree_delete_fixup(rbtree_node_t* root, rbtree_node_t* x);

#ifdef DEBUG_MEM
int rbtree_nalloc = 0;
#endif

rbtree_node_t* rbtree_node_new(rbtree_key_t key) {
  rbtree_node_t* ret = malloc(sizeof(rbtree_node_t));
  ret->color = BLACK;
  ret->key = key;
  ret->left = ret->right = ret->parent = rbtree_nil;
#ifdef DEBUG_MEM
  ++rbtree_nalloc;
#endif
  return ret;
}

static inline void rbtree_node_delete(rbtree_node_t* root) {
  free(root);
#ifdef DEBUG_MEM
  --rbtree_nalloc;
#endif 
}

void rbtree_destroy(rbtree_node_t* root) {
  if (root == rbtree_nil)
    return;
  rbtree_destroy(root->left);
  rbtree_destroy(root->right);
  rbtree_node_delete(root);
}

static rbtree_node_t* rotate_left(rbtree_node_t* root, rbtree_node_t* x) {
  assert(x->right != rbtree_nil);
  rbtree_node_t* y = x->right;
  x->right = y->left;
  if (y->left != rbtree_nil)
    y->left->parent = x;

  y->parent = x->parent;
  if (x->parent == rbtree_nil) {
    root = y;
  } else if (x == x->parent->left) {
    x->parent->left = y;
  } else {
    x->parent->right = y;
  }

  y->left = x;
  x->parent = y;
  return root;
}

static rbtree_node_t* rotate_right(rbtree_node_t* root, rbtree_node_t* y) {
  assert(y->left != rbtree_nil);
  rbtree_node_t* x = y->left;
  y->left = x->right;
  if (x->right != rbtree_nil)
    x->right->parent = y;

  x->parent = y->parent;
  if (y->parent == rbtree_nil) {
    root = x;
  } else if (y == y->parent->left) {
    y->parent->left = x;
  } else {
    y->parent->right = x;
  }

  x->right = y;
  y->parent = x;
  return root;
}

rbtree_node_t* rbtree_insert(rbtree_node_t* root, rbtree_node_t* z) {
  rbtree_node_t* y = rbtree_nil;
  rbtree_node_t* x = root;
  // Find the position to insert z
  while (x != rbtree_nil) {
    y = x;
    x = z->key < x->key ? x->left: x->right;
  }
  z->parent = y;
  if (y == rbtree_nil) {
    root = z;
  } else if (z->key < y->key) {
    y->left = z;
  } else {
    y->right = z;
  }
  z->color = RED;
  return rbtree_insert_fixup(root, z);
}

static rbtree_node_t* rbtree_insert_fixup(rbtree_node_t* root, rbtree_node_t* z) {
#define fix(left, right)  {                         \
  rbtree_node_t* y = z->parent->parent->right;      \
  if (y->color == RED) {                            \
    z->parent->color = BLACK;                       \
    y->color = BLACK;                               \
    z->parent->parent->color = RED;                 \
    z = z->parent->parent;                          \
  } else {                                          \
    if (z == z->parent->right) {                    \
      z = z->parent;                                \
      root = rotate_##left(root, z);                \
    }                                               \
    z->parent->color = BLACK;                       \
    z->parent->parent->color = RED;                 \
    root = rotate_##right(root, z->parent->parent); \
  }                                                 \
}

  while (z->parent->color == RED) {
    if (z->parent == z->parent->parent->left) {
      fix(left, right);
    } else {
      fix(right, left);
    }
  }
  root->color = BLACK;
  return root;
#undef fix
}

static rbtree_node_t* rbtree_transplant(rbtree_node_t* root,
    rbtree_node_t* u, rbtree_node_t* v) {
  if (u->parent == rbtree_nil) {
    root = v;
  } else if (u == u->parent->left) {
    u->parent->left = v;
  } else {
    u->parent->right = v;
  }
  v->parent = u->parent;
  return root;
}

static rbtree_node_t* rbtree_minimum(rbtree_node_t* root) {
  if (root == rbtree_nil)
    return rbtree_nil;
  while (root->left != rbtree_nil)
    root = root->left;
  return root;
}

static rbtree_node_t* rbtree_maximum(rbtree_node_t* root) {
  if (root == rbtree_nil)
    return rbtree_nil;
  while (root->right != rbtree_nil)
    root = root->right;
  return root;
}

rbtree_node_t* rbtree_delete(rbtree_node_t* root, rbtree_node_t* z) {
  rbtree_node_t* x;
  rbtree_node_t* y = z;
  rbtree_color_t y_original_color = y->color;
  
  if (z->left == rbtree_nil) {
    x = z->right;
    root = rbtree_transplant(root, z, z->right);
    rbtree_node_delete(z);
  } else if (z->right == rbtree_nil) {
    x = z->left;
    root = rbtree_transplant(root, z, z->left);
    rbtree_node_delete(z);
  } else {
    y = rbtree_minimum(z->right);
    y_original_color = y->color;
    x = y->right;
    if (y->parent == z) {
      x->parent = y;
    } else {
      root = rbtree_transplant(root, y, y->right);
      y->right = z->right;
      y->right->parent = y;
    }
    root = rbtree_transplant(root, z, y);
    y->left = z->left;
    y->left->parent = y;
    y->color = z->color;
    rbtree_node_delete(z);
  }

  if (y_original_color == BLACK)
    root = rbtree_delete_fixup(root, x);
  return root;
}

static rbtree_node_t* rbtree_delete_fixup(rbtree_node_t* root, rbtree_node_t* x) {
#define fix(left, right)  {                 \
  rbtree_node_t* w = x->parent->right;      \
  if (w->color == RED) {                    \
    w->color = BLACK;                       \
    x->parent->color = RED;                 \
    root = rotate_##left(root, x->parent);  \
  } else if (w->left->color == BLACK &&     \
      w->right->color == BLACK) {           \
    w->color = RED;                         \
    x = x->parent;                          \
  } else {                                  \
    if (w->right->color == BLACK) {         \
      w->left->color = BLACK;               \
      w->color = RED;                       \
      root = rotate_##right(root, w);       \
      w = x->parent->right;                 \
    }                                       \
    w->color = x->parent->color;            \
    x->parent->color = BLACK;               \
    w->right->color = BLACK;                \
    root = rotate_##left(root, x->parent);  \
    x = root;                               \
  }                                         \
}

  while (x != root && x->color == BLACK) {
    if (x == x->parent->left) {
      fix(left, right);
    } else {
      fix(right, left);
    }
  }
  x->color = BLACK;
  return root;
#undef fix
}

rbtree_node_t* rbtree_find(rbtree_node_t* root, rbtree_key_t key) {
  while (root != rbtree_nil && key != root->key) {
    root = key < root->key ? root->left: root->right;
  }
  return root;
}
