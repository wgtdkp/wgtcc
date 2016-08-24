//#include "test.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct tree tree_t;

struct tree {
    int val;
    tree_t* left;
    tree_t* right;
};

tree_t* tree_insert(tree_t* root, int val)
{
    if (root == NULL) {
        root = malloc(sizeof(tree_t));
        root->val = val;
        root->left = NULL;
        root->right = NULL;
        return root;
    }

    if (val < root->val)
        root->left = tree_insert(root->left, val);
    else if (val > root->val)
        root->right = tree_insert(root->right, val);
    return root;
}

void tree_print(tree_t* root)
{
    if (root == NULL)
        return;
    tree_print(root->left);
    printf("%d ", root->val);
    tree_print(root->right);
}

int main(void)
{
    int arr[] = {5, 4, -9, 78, 4, 2, 32, -10};
    tree_t* root;
    printf("sizeof(arr): %d\n", sizeof(int));
    for (int i = 0; i < sizeof(arr) / sizeof(int); ++i) {
        root = tree_insert(root, arr[i]);
    }
    tree_print(root);
    return 0;
}



