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
typedef struct avl avl;
struct avl {
    avl_node *root;
    const alloc_api *api;
};

avl avl_create(const alloc_api *api);
void avl_insert(avl *t, int key);
void avl_remove(avl *t, int key);


#ifdef AVL_UNIT_TESTS
void avl_unit_tests();
#endif

#ifdef AVL_IMPLEMENTATION
static inline int
height(avl_node *n)
{
    if (n == NULL)
        return 0;
    return n->height;
}

static inline int
get_balance_factor(avl_node *n)
{
    if (n == NULL) {
        return 0;
    }
    int result = height(n->left) - height(n->right);
    return result;
}

static avl_node *
rotate_left(avl_node *n)
{
    avl_node *new_root = n->right;
    avl_node *new_root_left = new_root->left;

    n->right = new_root_left;
    new_root->left = n;

    n->height = 1 + max(height(n->left), height(n->right));
    new_root->height = 1 + max(height(new_root->left), height(new_root->right));
    return new_root;
}

static avl_node *
rotate_right(avl_node *n)
{
    avl_node *new_root = n->left;
    avl_node *new_root_right = new_root->right;

    new_root->right = n;
    n->left  = new_root_right;

    n->height = 1 + max(height(n->left), height(n->right));
    new_root->height = 1 + max(height(new_root->left), height(new_root->right));
    return new_root;
}

static avl_node *
rotate_lr(avl_node *n)
{
    avl_node *l          = n->left;
    avl_node *new_root   = l->right;
    avl_node *new_root_l = new_root->left;
    avl_node *new_root_r = new_root->right;

    new_root->left  = l;
    new_root->right = n;
    l->right        = new_root_l;
    n->left      = new_root_r;

    l->height        = 1 + max(height(l->left),        height(l->right));
    n->height     = 1 + max(height(n->left),     height(n->right));
    new_root->height = 1 + max(height(new_root->left), height(new_root->right));

    return new_root;
}

static avl_node *
rotate_rl(avl_node *n)
{
    avl_node *r = n->right;
    avl_node *new_root = r->left;
    avl_node *new_root_l = new_root->left;
    avl_node *new_root_r = new_root->right;

    new_root->right = r;
    new_root->left = n;
    n->right = new_root_l;
    r->left = new_root_r;

    r->height        = 1 + max(height(r->left), height(r->right));
    n->height     = 1 + max(height(n->left), height(n->right));
    new_root->height = 1 + max(height(new_root->left), height(new_root->right));

    return new_root;
}

static avl_node *
avl_min_value_node(avl_node *node)
{
    avl_node *current = node;
    while (current->left != NULL) {
        current = current->left;
    }
    return current;
}

static avl_node *
avl_remove_internal(avl_node *n, int key, const alloc_api *api)
{
    if (n == NULL) {
        return n;
    }

    // look in the left sub-tree
    if (key < n->key) {
        n->left = avl_remove_internal(n->left, key, api);
    }
    // look in the right sub-tree
    else if (key > n->key) {
        n->right = avl_remove_internal(n->right, key, api);
    }
    // found the node to delete.
    else {
        // if to-be-deleted node has 0 or 1 children
        if (n->left == NULL || n->right == NULL) {
            // cache that child
            avl_node *temp = n->left ? n->left : n->right;
            if (temp == NULL) {
                temp = n;
                n = NULL;
            } else {
                // replace it with it's child
                *n = *temp;
            }

            avl_sfree(temp);
        }
        // if the node has two children.
        else {
            // get the min node in the right sub-tree.
            avl_node *temp = avl_min_value_node(n->right);
            // replace the node to be deleted with the min-node in the right sub-tree.
            n->key = temp->key;
            // remove the min-node that just replaced the to-be-deleted node in the right sub-tree.
            n->right = avl_remove_internal(n->right, temp->key, api);
        }
    }

    if (n == NULL)
        return n;

    // balance the tree after deletion.
    n->height = 1 + max(height(n->left), height(n->right));
    int bf = get_balance_factor(n);

    if ((bf > 1) && (get_balance_factor(n->left) >= 0)) {
        return rotate_right(n);
    }
    if ((bf > 1) && (get_balance_factor(n->left) < 0)) {
        return rotate_lr(n);
    }
    if ((bf < -1) && (get_balance_factor(n->right) <= 0)) {
        return rotate_left(n);
    }
    if ((bf < -1) && (get_balance_factor(n->right) > 0)) {
        return rotate_rl(n);
    }

    return n;
}

static avl_node *
avl_insert_internal(avl_node *n, int key, const alloc_api *api)
{
    if (n == NULL) {
        avl_node *n = avl_alloc();
        n->key = key;
        n->height = 1;
        n->left = NULL;
        n->right = NULL;
        return n;
    }

    if (key < n->key) {
        n->left = avl_insert_internal(n->left, key, api);
    } else if (key > n->key) {
        n->right = avl_insert_internal(n->right, key, api);
    } else {
        // equal node - return the node
        return n;
    }

    n->height = 1 + max(height(n->left), height(n->right));
    // height of left subtree minus height of right subtree.
    int bf = get_balance_factor(n);
    // left-left ((node, node->left, new_node) form a left-skewed tree.)
    if ((bf > 1) && (key < n->left->key)) {
        return rotate_right(n);
    }
    // right-right  (node, node->right, new_node) form a right-skewed tree.
    if ((bf < -1) && (key > n->right->key)) {
        return rotate_left(n);
    }
    // left-right
    if ((bf > 1) && (key > n->left->key)) {
        return rotate_lr(n);
    }
    // right-left
    if ((bf < -1) && (key < n->right->key)) {
        return rotate_rl(n);
    }

    return n;
}

void
avl_insert(avl *t, int key)
{
    t->root = avl_insert_internal(t->root, key, t->api);
}

void
avl_remove(avl *t, int key)
{
    t->root = avl_remove_internal(t->root, key, t->api);
}

avl
avl_create(const alloc_api *api)
{
    avl t;
    t.api = api;
    t.root = NULL;
    return t;
}

#ifdef AVL_UNIT_TESTS
#ifndef FREELIST_ALLOCATOR_IMPLEMENTATION
    #define FREELIST_ALLOCATOR_IMPLEMENTATION
#endif
#include "memory/freelist_alloc.h"
void
avl_unit_tests()
{
    size_t mem_size = 1024 * 1024;
    void *memory = malloc(mem_size);

    Freelist fl;
    freelist_init(&fl, memory, mem_size, DEFAULT_ALIGNMENT);
    fl.policy = PLACEMENT_POLICY_FIND_FIRST;

    avl t = avl_create(&fl.api);
    avl_insert(&t, 30);
    avl_insert(&t, 10);
    avl_insert(&t, 20);
    avl_insert(&t, 40);
    avl_insert(&t, 50);
    avl_insert(&t, 35);
    avl_insert(&t, 25);
    avl_insert(&t, 45);
    avl_insert(&t, 15);
    avl_insert(&t, 5);
    avl_insert(&t, 1);
    avl_insert(&t, 3);
    avl_insert(&t, 60);
    avl_insert(&t, 70);
    avl_insert(&t, 65);
    avl_insert(&t, 75);
    avl_insert(&t, 55);
    avl_insert(&t, 80);
    avl_insert(&t, 77);
    avl_insert(&t, 72);
    avl_insert(&t, 85);
    avl_insert(&t, 90);
    avl_insert(&t, 88);
    avl_insert(&t, 95);
    avl_insert(&t, 93);
    avl_insert(&t, 100);
    avl_insert(&t, 98);
    avl_insert(&t, 105);
    avl_insert(&t, 110);
    avl_insert(&t, 107);
    avl_insert(&t, 115);
    avl_insert(&t, 120);
    avl_insert(&t, 118);
    avl_insert(&t, 125);
    avl_insert(&t, 127);
    avl_insert(&t, 130);
    avl_insert(&t, 123);
    avl_insert(&t, 135);
    avl_insert(&t, 140);
    avl_insert(&t, 138);
    avl_insert(&t, 145);
    avl_insert(&t, 143);
    avl_insert(&t, 150);
    avl_insert(&t, 155);
    avl_insert(&t, 153);
    avl_insert(&t, 160);
    avl_insert(&t, 165);
    avl_insert(&t, 163);
    avl_insert(&t, 170);
    avl_insert(&t, 175);
    avl_insert(&t, 173);
    avl_insert(&t, 180);
    avl_insert(&t, 185);
    avl_insert(&t, 183);
    avl_insert(&t, 190);
    avl_insert(&t, 195);
    avl_insert(&t, 193);
    avl_insert(&t, 200);

    avl_remove(&t, 60);
    avl_remove(&t, 70);
    avl_remove(&t, 65);
    avl_remove(&t, 75);
    avl_remove(&t, 55);
    avl_remove(&t, 80);
    avl_remove(&t, 77);
    avl_remove(&t, 72);
    avl_remove(&t, 85);
    avl_remove(&t, 90);
    avl_remove(&t, 88);
    avl_remove(&t, 95);
    avl_remove(&t, 93);
    avl_remove(&t, 100);
    avl_remove(&t, 98);
    avl_remove(&t, 105);
    avl_remove(&t, 110);
    avl_remove(&t, 107);
    avl_remove(&t, 115);
    avl_remove(&t, 120);
    avl_remove(&t, 118);
    avl_remove(&t, 125);
    avl_remove(&t, 127);
    avl_remove(&t, 130);
    avl_remove(&t, 123);
    avl_remove(&t, 135);
    avl_remove(&t, 140);
    avl_remove(&t, 138);
    avl_remove(&t, 145);
    avl_remove(&t, 143);
    avl_remove(&t, 150);
    avl_remove(&t, 155);
    avl_remove(&t, 153);
    avl_remove(&t, 160);
    avl_remove(&t, 165);
    avl_remove(&t, 163);
    avl_remove(&t, 170);
    avl_remove(&t, 175);
    avl_remove(&t, 173);
    avl_remove(&t, 180);
    avl_remove(&t, 185);
    avl_remove(&t, 183);
    avl_remove(&t, 190);
    avl_remove(&t, 195);
    avl_remove(&t, 193);
    avl_remove(&t, 200);
    avl_remove(&t, 30);
    avl_remove(&t, 10);
    avl_remove(&t, 20);
    avl_remove(&t, 40);
    avl_remove(&t, 50);
    avl_remove(&t, 35);
    avl_remove(&t, 25);
    avl_remove(&t, 45);
    avl_remove(&t, 15);
    avl_remove(&t, 5);
    avl_remove(&t, 1);
    avl_remove(&t, 3);
    assert(t.root == NULL);

    assert(fl.used == 0);
    freelist_free_all(&fl);
    printf("avl_simple_test: [PASSED]\n");

    free(memory);
}
#endif
#endif
#endif