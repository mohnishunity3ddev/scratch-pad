#ifndef AVL_TREE_H
#define AVL_TREE_H

#include <stdio.h>
#include <stdlib.h>

#include "memory/memory.h"
#include "common.h"

#define avl_salloc(sz)  ( api != NULL ? api->alloc(api->allocator, sz) : malloc(sz) )
#define avl_sfree(p)    ( api != NULL ? api->free(api->allocator, p)   : free(p) )
#define avl_alloc() ( (avl_node *)(avl_salloc(sizeof(avl_node))) )

typedef struct avl_node avl_node;
struct avl_node {
    int key, height;
    avl_node *left, *right;
};

avl_node *avl_insert(avl_node *node, int key, const alloc_api *api);
avl_node *avl_remove(avl_node *root, int key, const alloc_api *api);

#ifdef AVL_IMPLEMENTATION
static inline int
height(avl_node *node)
{
    if (node == NULL)
        return 0;
    return node->height;
}

static inline int
get_balance_factor(avl_node *node)
{
    if (node == NULL) {
        return 0;
    }
    int result = height(node->left) - height(node->right);
    return result;
}

static avl_node *
rotate_left(avl_node *root)
{
    avl_node *new_root = root->right;
    avl_node *new_root_left = new_root->left;

    root->right = new_root_left;
    new_root->left = root;

    root->height = 1 + max(height(root->left), height(root->right));
    new_root->height = 1 + max(height(new_root->left), height(new_root->right));
    return new_root;
}

static avl_node *
rotate_right(avl_node *root)
{
    avl_node *new_root = root->left;
    avl_node *new_root_right = new_root->right;

    new_root->right = root;
    root->left  = new_root_right;

    root->height = 1 + max(height(root->left), height(root->right));
    new_root->height = 1 + max(height(new_root->left), height(new_root->right));
    return new_root;
}

static avl_node *
rotate_lr(avl_node *root)
{
    avl_node *l          = root->left;
    avl_node *new_root   = l->right;
    avl_node *new_root_l = new_root->left;
    avl_node *new_root_r = new_root->right;

    new_root->left  = l;
    new_root->right = root;
    l->right        = new_root_l;
    root->left      = new_root_r;

    l->height        = 1 + max(height(l->left),        height(l->right));
    root->height     = 1 + max(height(root->left),     height(root->right));
    new_root->height = 1 + max(height(new_root->left), height(new_root->right));

    return new_root;
}

static avl_node *
rotate_rl(avl_node *root)
{
    avl_node *r = root->right;
    avl_node *new_root = r->left;
    avl_node *new_root_l = new_root->left;
    avl_node *new_root_r = new_root->right;

    new_root->right = r;
    new_root->left = root;
    root->right = new_root_l;
    r->left = new_root_r;

    r->height        = 1 + max(height(r->left), height(r->right));
    root->height     = 1 + max(height(root->left), height(root->right));
    new_root->height = 1 + max(height(new_root->left), height(new_root->right));

    return new_root;
}

static avl_node *
min_value_node(avl_node *node)
{
    avl_node *current = node;
    while (current->left != NULL) {
        current = current->left;
    }
    return current;
}

avl_node *
avl_remove(avl_node *root, int key, const alloc_api *api)
{
    if (root == NULL) {
        return root;
    }

    // look in the left sub-tree
    if (key < root->key) {
        root->left = avl_remove(root->left, key, api);
    }
    // look in the right sub-tree
    else if (key > root->key) {
        root->right = avl_remove(root->right, key, api);
    }
    // found the node to delete.
    else {
        // if to-be-deleted node has 0 or 1 children
        if (root->left == NULL || root->right == NULL) {
            // cache that child
            avl_node *temp = root->left ? root->left : root->right;
            if (temp == NULL) {
                temp = root;
                root = NULL;
            } else {
                // replace it with it's child
                *root = *temp;
            }

            avl_sfree(temp);
        }
        // if the node has two children.
        else {
            // get the min node in the right sub-tree.
            avl_node *temp = min_value_node(root->right);
            // replace the node to be deleted with the min-node in the right sub-tree.
            root->key = temp->key;
            // remove the min-node that just replaced the to-be-deleted node in the right sub-tree.
            root->right = avl_remove(root->right, temp->key, api);
        }
    }

    if (root == NULL)
        return root;

    // balance the tree after deletion.
    root->height = 1 + max(height(root->left), height(root->right));
    int bf = get_balance_factor(root);

    if ((bf > 1) && (get_balance_factor(root->left) >= 0)) {
        return rotate_right(root);
    }
    if ((bf > 1) && (get_balance_factor(root->left) < 0)) {
        return rotate_lr(root);
    }
    if ((bf < -1) && (get_balance_factor(root->right) <= 0)) {
        return rotate_left(root);
    }
    if ((bf < -1) && (get_balance_factor(root->right) > 0)) {
        return rotate_rl(root);
    }

    return root;
}

avl_node *
avl_insert(avl_node *node, int key, const alloc_api *api)
{
    if (node == NULL) {
        avl_node *n = avl_alloc();
        n->key = key;
        n->height = 1;
        n->left = NULL;
        n->right = NULL;
        return n;
    }

    if (key < node->key) {
        node->left = avl_insert(node->left, key, api);
    } else if (key > node->key) {
        node->right = avl_insert(node->right, key, api);
    } else {
        // equal node - return the node
        return node;
    }

    node->height = 1 + max(height(node->left), height(node->right));
    // height of left subtree minus height of right subtree.
    int bf = get_balance_factor(node);
    // left-left ((node, node->left, new_node) form a left-skewed tree.)
    if ((bf > 1) && (key < node->left->key)) {
        return rotate_right(node);
    }
    // right-right  (node, node->right, new_node) form a right-skewed tree.
    if ((bf < -1) && (key > node->right->key)) {
        return rotate_left(node);
    }
    // left-right
    if ((bf > 1) && (key > node->left->key)) {
        return rotate_lr(node);
    }
    // right-left
    if ((bf < -1) && (key < node->right->key)) {
        return rotate_rl(node);
    }

    return node;
}

#ifdef AVL_UNIT_TEST
void
avl_test()
{
    size_t mem_size = 1024 * 1024;
    void *memory = malloc(mem_size);

    Freelist fl;
    freelist_init(&fl, memory, mem_size, DEFAULT_ALIGNMENT);
    fl.policy = PLACEMENT_POLICY_FIND_FIRST;
    alloc_api *api = &fl.api;

    avl_node *root = NULL;
    root = avl_insert(root, 30, api);
    root = avl_insert(root, 10, api);
    root = avl_insert(root, 20, api);
    root = avl_insert(root, 40, api);
    root = avl_insert(root, 50, api);
    root = avl_insert(root, 35, api);
    root = avl_insert(root, 25, api);
    root = avl_insert(root, 45, api);
    root = avl_insert(root, 15, api);
    root = avl_insert(root, 5, api);
    root = avl_insert(root, 1, api);
    root = avl_insert(root, 3, api);
    root = avl_insert(root, 60, api);
    root = avl_insert(root, 70, api);
    root = avl_insert(root, 65, api);
    root = avl_insert(root, 75, api);
    root = avl_insert(root, 55, api);
    root = avl_insert(root, 80, api);
    root = avl_insert(root, 77, api);
    root = avl_insert(root, 72, api);
    root = avl_insert(root, 85, api);
    root = avl_insert(root, 90, api);
    root = avl_insert(root, 88, api);
    root = avl_insert(root, 95, api);
    root = avl_insert(root, 93, api);
    root = avl_insert(root, 100, api);
    root = avl_insert(root, 98, api);
    root = avl_insert(root, 105, api);
    root = avl_insert(root, 110, api);
    root = avl_insert(root, 107, api);
    root = avl_insert(root, 115, api);
    root = avl_insert(root, 120, api);
    root = avl_insert(root, 118, api);
    root = avl_insert(root, 125, api);
    root = avl_insert(root, 127, api);
    root = avl_insert(root, 130, api);
    root = avl_insert(root, 123, api);
    root = avl_insert(root, 135, api);
    root = avl_insert(root, 140, api);
    root = avl_insert(root, 138, api);
    root = avl_insert(root, 145, api);
    root = avl_insert(root, 143, api);
    root = avl_insert(root, 150, api);
    root = avl_insert(root, 155, api);
    root = avl_insert(root, 153, api);
    root = avl_insert(root, 160, api);
    root = avl_insert(root, 165, api);
    root = avl_insert(root, 163, api);
    root = avl_insert(root, 170, api);
    root = avl_insert(root, 175, api);
    root = avl_insert(root, 173, api);
    root = avl_insert(root, 180, api);
    root = avl_insert(root, 185, api);
    root = avl_insert(root, 183, api);
    root = avl_insert(root, 190, api);
    root = avl_insert(root, 195, api);
    root = avl_insert(root, 193, api);
    root = avl_insert(root, 200, api);

    root = avl_remove(root, 60, api);
    root = avl_remove(root, 70, api);
    root = avl_remove(root, 65, api);
    root = avl_remove(root, 75, api);
    root = avl_remove(root, 55, api);
    root = avl_remove(root, 80, api);
    root = avl_remove(root, 77, api);
    root = avl_remove(root, 72, api);
    root = avl_remove(root, 85, api);
    root = avl_remove(root, 90, api);
    root = avl_remove(root, 88, api);
    root = avl_remove(root, 95, api);
    root = avl_remove(root, 93, api);
    root = avl_remove(root, 100, api);
    root = avl_remove(root, 98, api);
    root = avl_remove(root, 105, api);
    root = avl_remove(root, 110, api);
    root = avl_remove(root, 107, api);
    root = avl_remove(root, 115, api);
    root = avl_remove(root, 120, api);
    root = avl_remove(root, 118, api);
    root = avl_remove(root, 125, api);
    root = avl_remove(root, 127, api);
    root = avl_remove(root, 130, api);
    root = avl_remove(root, 123, api);
    root = avl_remove(root, 135, api);
    root = avl_remove(root, 140, api);
    root = avl_remove(root, 138, api);
    root = avl_remove(root, 145, api);
    root = avl_remove(root, 143, api);
    root = avl_remove(root, 150, api);
    root = avl_remove(root, 155, api);
    root = avl_remove(root, 153, api);
    root = avl_remove(root, 160, api);
    root = avl_remove(root, 165, api);
    root = avl_remove(root, 163, api);
    root = avl_remove(root, 170, api);
    root = avl_remove(root, 175, api);
    root = avl_remove(root, 173, api);
    root = avl_remove(root, 180, api);
    root = avl_remove(root, 185, api);
    root = avl_remove(root, 183, api);
    root = avl_remove(root, 190, api);
    root = avl_remove(root, 195, api);
    root = avl_remove(root, 193, api);
    root = avl_remove(root, 200, api);
    root = avl_remove(root, 30, api);
    root = avl_remove(root, 10, api);
    root = avl_remove(root, 20, api);
    root = avl_remove(root, 40, api);
    root = avl_remove(root, 50, api);
    root = avl_remove(root, 35, api);
    root = avl_remove(root, 25, api);
    root = avl_remove(root, 45, api);
    root = avl_remove(root, 15, api);
    root = avl_remove(root, 5, api);
    root = avl_remove(root, 1, api);
    root = avl_remove(root, 3, api);
    assert(root == NULL);

    assert(fl.used == 0);
    freelist_free_all(&fl);

    free(memory);
}
#endif
#endif
#endif