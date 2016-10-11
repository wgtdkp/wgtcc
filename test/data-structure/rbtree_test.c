#include "test.h"
#include "../rbtree.h"

static void verify_root(rbtree_node_t* root) {
  EXPECT(BLACK, root->color);
}

static void verify_leaf(void) {
  EXPECT(BLACK, rbtree_nil->color);
}

static void verify_kids(rbtree_node_t* root) {
  if (root == rbtree_nil)
    return;
  verify_kids(root->left);
  verify_kids(root->right);
  if (root->color == RED) {
    EXPECT(BLACK, root->left->color);
    EXPECT(BLACK, root->right->color);
  }
}

static int verify_black_height(rbtree_node_t* root) {
  if (root == rbtree_nil)
    return 1;
  int lbh = verify_black_height(root->left);
  int rbh = verify_black_height(root->right);
  EXPECT(lbh, rbh);
  return (root->color == BLACK) + lbh;
}

static void __rbtree_print(rbtree_node_t* root) {
  if (root == rbtree_nil)
    return;
  __rbtree_print(root->left);
  printf("%d ", root->key);
  __rbtree_print(root->right);
}

static void rbtree_print(rbtree_node_t* root) {
  __rbtree_print(root);
  printf("\n");
}

static void verify(rbtree_node_t* root) {
  verify_root(root);
  verify_leaf();
  verify_kids(root);
  verify_black_height(root);
  rbtree_print(root);
}

static void test(void) {
  rbtree_node_t* root = rbtree_nil;
  rbtree_key_t keys[] = {26, 17, 14, 10, 7, 3, 12, 16, 15, 21, 19, 20, 23, 41, 30, 28, 38, 35, 39, 47};
  for (int i = 0; i < sizeof(keys) / sizeof(rbtree_key_t); ++i) {
    root = rbtree_insert_key(root, keys[i]);
    verify(root);
  }

  rbtree_key_t del_keys[] = {26, 17, 14, 10, 7, 3, 12, 16, 15, 21, 19, 20, 23, 41, 30, 28, 38, 35, 39, 47};
  for (int i = 0; i < sizeof(del_keys) / sizeof(rbtree_key_t); ++i) {
    root = rbtree_delete_key(root, del_keys[i]);
    verify(root);
  }

  rbtree_destroy(root);
#ifdef DEBUG_MEM
  EXPECT(0, rbtree_nalloc);
#endif
}

__attribute__((unused)) static int rbtree_count(rbtree_node_t* root) {
  if (root == rbtree_nil)
    return 0;
  return 1 + rbtree_count(root->left) + rbtree_count(root->right);
}

int main(void) {
  test();
  return 0;
}
