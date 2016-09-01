//#include "test.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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

    if (val < root->val) {
        root->left = tree_insert(root->left, val);
    } else if (val > root->val) {
        root->right = tree_insert(root->right, val);
    }
    return root;
}

tree_t* tree_find_max(tree_t* root)
{
    assert(root != NULL);
    while (root->right != NULL)
        root = root->right;
    return root;
}

tree_t* tree_delete(tree_t* root, int val)
{
    if (root == NULL) {
        return NULL;
    }

    if (val < root->val) {
        root->left = tree_delete(root->left, val);
    } else if (val > root->val) {
        root->right = tree_delete(root->right, val);
    } else {
        if (root->right == NULL) {
            tree_t* ret = root->left;
            free(root);
            return ret;
        } else if (root->left == NULL) {
            tree_t* ret = root->right;
            free(root);
            return ret;
        } else {
            tree_t* sub_max = tree_find_max(root->left);
            root->val = sub_max->val;
            root->left = tree_delete(root->left, sub_max->val);
            return root;
        }
    }
    return root;
}

tree_t* tree_find(tree_t* root, int val)
{
    if (root == NULL)
        return NULL;
    if (val < root->val)
        return tree_find(root->left, val);
    else if (val > root->val)
        return tree_find(root->right, val);
    else
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

tree_t* test1()
{
    return (tree_t*)1293440;
}

int main()
{
    char inst[100];
    int val;
    tree_t* node = NULL;
    tree_t* root = NULL;
    while (1) {
        scanf("%s", inst);
        if (strcmp(inst, "end") == 0)
            break;
        scanf("%d", &val);
        if (strcmp(inst, "insert") == 0)
            root = tree_insert(root, val);
        else if (strcmp(inst, "delete") == 0)
            root = tree_delete(root, val);
        else if (strcmp(inst, "find") == 0) {
            node = tree_find(root, val);
            printf("founded: %d\n", node != NULL);
        } else {
            fprintf(stderr, "invalid instruction '%s'\n", inst);
        }
        printf("tree: ");
        tree_print(root);
        printf("\n");
    }

    return 0;
}
