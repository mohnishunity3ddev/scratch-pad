#ifndef RB_TREE_H
#define RB_TREE_H

#include "memory/memory.h"
#include "common.h"

#define node_not_null(n) (n != NULL && !rbt_is_nil_sentinel_internal(n))
#define node_null(n) (n == NULL || rbt_is_nil_sentinel_internal(n))

enum rbt_color
{
    RBT_COLOR_NONE = 0,
    RBT_COLOR_BLACK,
    RBT_COLOR_RED,
};

typedef struct rbt_node {
    struct rbt_node *left,
                    *right,
                    *parent;
    int key;
    char color;
} rbt_node;

static rbt_node nil_node;
typedef struct rbt {
    rbt_node *root;
    const alloc_api *api;
} rbt;

rbt rbt_create_tree(const alloc_api *api);
void rbt_insert_key(rbt *t, int key);
void rbt_remove_key(rbt *t, int key);

#ifdef RBT_UNIT_TESTS
void rbt_unit_tests();
#endif

#ifdef RBT_IMPLEMENTATION
#include <stdio.h>

static inline bool
rbt_is_nil_sentinel_internal(const rbt_node *node)
{
    return ((node->color == RBT_COLOR_BLACK) &&
            (node->key == SINT32_MAX) &&
            (node->left == NULL) &&
            (node->right == NULL));
}

static inline rbt_node *
rbt_new_node_internal(rbt *t, int key)
{
    assert(t != NULL);
    rbt_node *n = shalloc_t(t->api, rbt_node);
    assert(n != NULL);
    n->key = key;
    return n;
}

// left rotate x, y and z which form a right skewed sub-tree
// where y is x's right child and z is y's right child.
// here, y becomes the new root, y's left child becomes x's right child
// and x becomes the left child of y, x's parent now points to y as it's new child.
static void
rbt_rotate_left_internal(rbt *t, rbt_node *x)
{
    rbt_node *y = x->right;
    // y's left subtree becomes right child of x.
    x->right = y->left;
    // y->left != NULL
    if (!rbt_is_nil_sentinel_internal(y->left)) {
        y->left->parent = x;
    }
    // x's parent now becomes y's parent
    y->parent = x->parent;
    if (rbt_is_nil_sentinel_internal(x->parent)) {
        // if x was the root, then y becomes the root.
        t->root = y;
    } else if (x == x->parent->left) {
        // x was a left child, then y becomes a left child as well
        x->parent->left = y;
    } else {
        // x was a right child, then y becomes a right child as well.
        x->parent->right = y;
    }
    // x becomes y's left child
    y->left = x;
    x->parent = y;
}

// right rotate x, y and z where x, y, z from a left-skewed sub-tree.
// y is left child of x, z is left child of y.
// y's right child becomes x's left child
// x becomes y's right child. x's parent now points to y as it's child.
static void
rbt_rotate_right_internal(rbt *t, rbt_node *x)
{
    rbt_node *y = x->left;

    x->left = y->right;
    // y->right != NULL
    if (!rbt_is_nil_sentinel_internal(y->right)) {
        y->right->parent = x;
    }
    y->parent = x->parent;
    // if x->parent != NULL --- if x is the root
    if (rbt_is_nil_sentinel_internal(x->parent)) {
        t->root = y;
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
        x->parent->right = y;
    }
    y->right = x;
    x->parent = y;
}


// There are three cases:
// Case 1: z's uncle (z's parent's sibling) is red.
//         recolor z's parent and uncle to black. and z's grandparent to red.
// Case 2: z's uncle is black or null, and z is a right child of it's parent
//         set z's parent as new z, left rotate (which leads to case 3)
// Case 3: z's uncle is black or null, and z is a left child of it's parent
//         right rotate. recolor z's parent to black. and z's grandpa to red.
//
// the fix is basically making sure that rules for red-black trees are satisfied.
static void
rbt_insert_fix_internal(rbt *t, rbt_node *z)
{
    rbt_node *y = NULL;
    while (z->parent->color == RBT_COLOR_RED)
    {
        // is z's parent a left child of z's grandparent.
        if (z->parent == z->parent->parent->left) {
            // z's uncle is the right sibling of z's parent: right child of z's grandparent
            y = z->parent->parent->right;
            if (y->color == RBT_COLOR_BLACK) {
                // Case 2: z is the right child of it's parent. and both are red (violation of rbt property)
                //         set z's parent as new z, left rotate on the new z (z's parent)
                if (z == z->parent->right) {
                    z = z->parent;
                    rbt_rotate_left_internal(t, z);
                }
                // Case 3: z is the left child of it's parent. both are red (violation of rbt property).
                //         right rotate on z's grandparent. color z's parent black and z's grandparent red.
                //         z's grandparent, parent and z are left-skewed.
                z->parent->color = RBT_COLOR_BLACK;
                z->parent->parent->color = RBT_COLOR_RED;
                // right rotation will automatically make z the grandparent of it's own grandparent
                // in other words, it will automatically set z = z_grandpa for the next iteration of the loop.
                rbt_rotate_right_internal(t, z->parent->parent);
            } else {
                // Case 1: z's uncle is colored red.
                //         color z's parent and uncle black, and z's grandpa red.
                //         This makes sure the number of black nodes in every path from root to leaf for the
                //         rb_tree is the same.
                z->parent->color  = RBT_COLOR_BLACK;
                y->color = RBT_COLOR_BLACK;
                z->parent->parent->color = RBT_COLOR_RED;
                z = z->parent->parent;
            }
        }
        // z's parent is the right child of z's grandparent
        else if (z->parent == z->parent->parent->right) {
            y = z->parent->parent->left;
            if (y->color == RBT_COLOR_BLACK)
            {
                // z's uncle is black or null
                // Case 2: z is the left child of it's parent
                //         set z = z->parent and right rotate
                if (z == z->parent->left) {
                    z = z->parent;
                    rbt_rotate_right_internal(t, z);
                }
                // Case 3: z is the right child of it's parent. z's grandparent, parent and z are right skewed
                //         color z's parent black, z's grnadparent red and left rotate
                z->parent->color = RBT_COLOR_BLACK;
                z->parent->parent->color = RBT_COLOR_RED;
                rbt_rotate_left_internal(t, z->parent->parent);
            } else {
                // Case 1: z's uncle is red.
                y->color = RBT_COLOR_BLACK;
                z->parent->color = RBT_COLOR_BLACK;
                z->parent->parent->color = RBT_COLOR_RED;
                z = z->parent->parent;
            }
        }
    }
    t->root->color = RBT_COLOR_BLACK;
}

static void
rbt_insert_internal(rbt *t, rbt_node *z)
{
    if (z->key == SINT32_MAX) {
        printf("[ERROR]: Not allowed. Returning.\n");
        return;
    }

    // insert like a normal BST
    rbt_node *x = t->root;
    rbt_node *prev_x = &nil_node;
    while (!rbt_is_nil_sentinel_internal(x)) {
        if (z->key == x->key) { return; }
        prev_x = x;
        if (z->key < x->key) {
            x = x->left;
        } else {
            x = x->right;
        }
    }
    z->parent = prev_x;
    // if tree is empty.
    if (rbt_is_nil_sentinel_internal(prev_x)) {
        t->root = z;
    } else if (z->key < prev_x->key) {
        prev_x->left = z;
    } else {
        prev_x->right = z;
    }

    z->left = &nil_node;
    z->right = &nil_node;
    // color the new node red as per convention
    z->color = RBT_COLOR_RED;

    // rebalance according to red-black rules.
    rbt_insert_fix_internal(t, z);

    assert(rbt_is_nil_sentinel_internal(&nil_node));
}

// - - - - - - - - - - - - - - - - - - -
// Deletion stuff
// - - - - - - - - - - - - - - - - - - -

/// @brief replace u with v in the red black tree t. Only sets the parent of u to point to v now.
static void
rbt_transplant_internal(rbt *t, rbt_node *u, rbt_node *v)
{
    assert((u != NULL) && (v != NULL));
    if (rbt_is_nil_sentinel_internal(u->parent)) {
        t->root = v;          // u is the root. make v the new root
    } else if (u == u->parent->left) {
        u->parent->left = v;  // make v the left child of u's parent.
    } else {
        u->parent->right = v; // make v the right child of u's parent.
    }
    v->parent = u->parent;    // set the parent pointer of v which replaces u.
}

/// @brief get the minimum node in the tree rooted at 'node'
static rbt_node *
rbt_min_value_node_internal(rbt_node *node)
{
    assert(node != NULL);
    rbt_node *current = node;
    while (current->left != NULL &&
           !rbt_is_nil_sentinel_internal(current->left))
    {
        current = current->left;
    }
    return current;
}

/// @brief recolor and do rotations to make the tree a valid red-black tree again after moving/removing a black
///        node from the tree.
/// @param t the red-black tree to fix
/// @param x the node that replaced the removed/moved black node that was removed/moved from the tree prior to
///          calling this routine. This is a double black node to satisfy the property of rb-trees that each path
///          from root to leaf nodes has same number of black nodes. All we have to do now is remove the 'double'
///          blackness of this node in the tree.
static void
rbt_remove_fixup_internal(rbt *t, rbt_node *x)
{
    assert((t != NULL) && (x != NULL));
    rbt_node *w = NULL;                                         // x's sibling
    while (x != t->root &&
           x->color == RBT_COLOR_BLACK)
    {
        if (x == x->parent->left) {                             // x is the left child of it's parent
            w = x->parent->right;                               // x is its parent's left child,
                                                                // x's sibling is it's parent's right child.
            // checking for Case 1: x's sibling being red.
            if (w->color == RBT_COLOR_RED) {                    // is x's sibling red?
                w->color = RBT_COLOR_BLACK;
                x->parent->color = RBT_COLOR_RED;
                rbt_rotate_left_internal(t, x->parent);
                w = x->parent->right;
            }
            // Case 2: sibling is black. check if the sibling has two black children
            if (w->left->color == RBT_COLOR_BLACK &&
                w->right->color == RBT_COLOR_BLACK)
            {
                w->color = RBT_COLOR_RED;
                x = x->parent;
            } else {
                // Case 3: sibling is black. it's left child is red, and it's right child is black.
                if (w->right->color == RBT_COLOR_BLACK) {
                    w->left->color = RBT_COLOR_BLACK;
                    w->color = RBT_COLOR_RED;
                    rbt_rotate_right_internal(t, w);
                    w = x->parent->right;
                }
                // Case 4: sibling is black, and it's right child is red.
                w->color = x->parent->color;
                x->parent->color = RBT_COLOR_BLACK;
                w->right->color = RBT_COLOR_BLACK;
                rbt_rotate_left_internal(t, x->parent);
                x = t->root;
            }
        } else {
            w = x->parent->left;
            if (w->color == RBT_COLOR_RED) {
                w->color = RBT_COLOR_BLACK;
                x->parent->color = RBT_COLOR_RED;
                rbt_rotate_right_internal(t, x->parent);
                w = x->parent->left;
            }
            if (x->right->color == RBT_COLOR_BLACK &&
                w->left->color == RBT_COLOR_BLACK)
            {
                w->color = RBT_COLOR_RED;
                x = x->parent;
            } else {
                if (w->left->color == RBT_COLOR_BLACK) {
                    w->right->color = RBT_COLOR_BLACK;
                    w->color = RBT_COLOR_RED;
                    rbt_rotate_left_internal(t, w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = RBT_COLOR_BLACK;
                w->left->color = RBT_COLOR_BLACK;
                rbt_rotate_right_internal(t, x->parent);
                x = t->root;
            }
        }
    }

    x->color = RBT_COLOR_BLACK;
}

static void
rbt_remove_node_internal(rbt *t, rbt_node *z)
{
    rbt_node *y = z,
             *x = NULL;                                     // x is the node which replaces y.
    char y_original_color = y->color;
    if (rbt_is_nil_sentinel_internal(z->left)) {
        x = z->right;                                       // z's right child is the one replacing it.
        rbt_transplant_internal(t, z, z->right);            // set parent of z to point to it's right child
    } else if (rbt_is_nil_sentinel_internal(z->right)) {
        x = z->left;                                        // z's left child is the one replacing it.
        rbt_transplant_internal(t, z, z->left);             // set parent of z to point to it's left child.
    } else {

        // z (the node we want to remove) has two children.
        // the node that replaces it is the inorder successor returned by the rbt_min_value_node_internal below.
        // y is the minimum node which is the left most node in z's right subtree.
        y = rbt_min_value_node_internal(z->right);          // get the minimum node in z's right subtree. this is the node
                                                            // that replaces z in the tree.
        y_original_color = y->color;                        // cache the original color of the node that we will be moving.
        x = y->right;                                       // points to y's old location before replacing z.

        if (y != z->right) {                                // if the inorder successor of z is somewhere deep in the tree.
            rbt_transplant_internal(t, y, y->right);        // replace y with x(y's right child). This will be the double
                                                            // black node if y was black
            y->right = z->right;                            // the parent of z now points to y, y's right becomes z's right child.
            z->right->parent = y;                           // make z's right child y's right child now
        } else {
            x->parent = y;
        }

        // here setting z's right does not make sense since at this point, y is z's right child and it is the one
        // replacing z.
        rbt_transplant_internal(t, z, y);                   // have z's parent to point to y as it's child now.
        y->left = z->left;
        y->left->parent = y;                                // make z's left child, y's left child now.
        y->color = z->color;                                // set z's color as the new color of the node replacing it (y).
    }

    shfree(t->api, z); // safe to free the z now.

    // only need to call fixup routine when y(the node moved/removed) is black
    // since red-black tree rules are disturbed only when you move a black node.
    // x is the deepest node that was moved. so call fixup with x.
    if (y_original_color == RBT_COLOR_BLACK) {
        rbt_remove_fixup_internal(t, x);
    }

    assert(rbt_is_nil_sentinel_internal(&nil_node));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Public API
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

rbt
rbt_create_tree(const alloc_api *api)
{
    rbt t = {};
    t.api = api;
    nil_node.color = RBT_COLOR_BLACK;
    nil_node.key = SINT32_MAX;
    t.root = &nil_node;
    return t;
}

void
rbt_insert_key(rbt *t, int key)
{
    assert(t != NULL);
    rbt_node *z = rbt_new_node_internal(t, key);
    rbt_insert_internal(t, z);
}

void
rbt_remove_key(rbt *t, int key)
{
    rbt_node *z = t->root;
    while (!rbt_is_nil_sentinel_internal(z) && z->key != key) {
        if (key < z->key) {
            z = z->left;
        } else {
            z = z->right;
        }
    }

    if (!rbt_is_nil_sentinel_internal(z)) {
        rbt_remove_node_internal(t, z);
    }
}

#ifdef RBT_UNIT_TESTS

#ifndef FREELIST_ALLOCATOR_IMPLEMENTATION
#define FREELIST_ALLOCATOR_IMPLEMENTATION
#endif
#include "memory/freelist_alloc.h"

#ifndef STACK_IMPLEMENTATION
#define STACK_IMPLEMENTATION
#endif
#include "stack.h"

#ifndef DARR_IMPLEMENTATION
#define DARR_IMPLEMENTATION
#endif
#include "darr.h"

#ifndef QUEUE_IMPLEMENTATION
#define QUEUE_IMPLEMENTATION
#endif
#include "queue.h"

#include <math.h>

int
rbt_get_depth(rbt_node *node)
{
    if (node == NULL || rbt_is_nil_sentinel_internal(node)) {
        return 0;
    }

    int left_depth = rbt_get_depth(node->left);
    int right_depth = rbt_get_depth(node->right);
    return ((left_depth>right_depth) ? left_depth : right_depth) + 1;
}


void
print_stack(const stack_voidp *s)
{
    printf("[STACK]:= ");
    for (int i = 0; i <= s->top; ++i) {
        rbt_node *r = (rbt_node *)s->arr[i];
        printf("%d -> ", r->key);
    }
    printf("TOP.\n");
}


static unsigned int
get_path_black_height(const darr_voidp *path)
{
    unsigned int bh = 0;
    for (int i = 0; i < path->length; ++i) {
        rbt_node *node = arrget_p(path, rbt_node, i);
        if (node->color == RBT_COLOR_BLACK) {
            ++bh;
        }
    }
    return bh;
}

static darr_voidp
copy_stack_to_node_array(const stack_voidp *s, const alloc_api *api)
{
    darr_voidp path = arrinit_p(api);
    for (int i = 0; i <= s->top; ++i) {
        arrpush_p(&path, s->arr[i]);
    }
    return path;
}

darr_darr_voidp
rbt_get_root_to_leaf_paths(const rbt *t, const alloc_api *api)
{
    stack_voidp stack = sinit_p(api);
    // array of int arrays.
    darr_darr_voidp paths = arrinit(api, darr_voidp);

    rbt_node *n = t->root;
    while(true) {
        while(node_not_null(n)) {
            spush_p(stack, n);
            n = n->left;
        }

        arrpush(&paths, darr_voidp, copy_stack_to_node_array(&stack, api));

        if (stack.top == -1) {
            break;
        }
        rbt_node *top = speek_p(stack, rbt_node, 0);
        if (node_not_null(top->right)) {
            n = top->right;
        } else {
            n = spop_p(stack, rbt_node);
            top = speek_p(stack, rbt_node, 0);
            while (node_not_null(top) && top->right == n) {
                n = spop_p(stack, rbt_node);
                top = speek_p(stack, rbt_node, 0);
            }

            if(node_null(top)) {
                break;
            }
            n = top->right;
        }
    }

    shfree(api, stack.arr);
    return paths;
}

static void
rbt_print_all_paths(const darr_darr_voidp *paths)
{
    printf("Paths:=\n");
    for (int i = 0; i < paths->length; ++i) {
        darr_voidp path = arrget(paths, darr_voidp, i);
        unsigned int bh = get_path_black_height(&path);
        printf("PATH[%d] Black Height(%u):= ", (i+1), bh);
        for (int j = 0; j < path.length; ++j) {
            rbt_node *node = arrget_p(&path, rbt_node, j);
            printf("%d(%c) -> ", node->key, (node->color==RBT_COLOR_BLACK) ? 'B' : 'R');
        }
        printf("END.\n");
    }
}

darr_voidp
rbt_inorder(const rbt *t, const alloc_api *api)
{
    darr_voidp arr = arrinit_p(api);

    stack_voidp s = sinit_p(api);
    stack_int sInt = sinit(api, int);

    rbt_node *n = t->root;
    while(true) {
        while(n != NULL && !rbt_is_nil_sentinel_internal(n)) {
            spush_p(s, n);
            spush(sInt, int, n->key);
            n = n->left;
        }

        if (s.top == -1) {
            break;
        }

        n = spop_p(s, rbt_node);
        spop(sInt, int);
        arrpush_p(&arr, n);
        n = n->right;
    }

    shfree(api, s.arr);
    shfree(api, sInt.arr);
    return arr;
}

void
rbt_preorder(const rbt *t, const alloc_api *api)
{
    printf("Pre Order Traversal for the tree is: \n");
    stack_voidp s = sinit_p(api);
    stack_int sInt = sinit(api, int);

    rbt_node *n = t->root;
    while(true) {
        while(n != NULL && !rbt_is_nil_sentinel_internal(n)) {
            spush_p(s, n);
            spush(sInt, int, n->key);
            printf("%d, ", n->key);
            n = n->left;
        }

        if (s.top == -1) {
            break;
        }

        n = spop_p(s, rbt_node);
        spop(sInt, int);
        n = n->right;
    }
    printf("\n");
    shfree(api, s.arr);
}

void
rbt_postorder(const rbt *t, const alloc_api *api)
{
    printf("Post Order Traversal for the tree is: \n");
    stack_voidp s = sinit_p(api);
    stack_int s_int = sinit(api, int);

    rbt_node *n = t->root;
    while(true) {
        while(node_not_null(n)) {
            spush_p(s, n);
            spush(s_int, int, n->key);
            n = n->left;
        }

        if (s.top == -1) { break; }

        rbt_node *top = speek_p(s, rbt_node, 0);
        // we have gone through the left subtree, if the right subtree is null, print the top element. also check
        // if it is the right child of it's parent, if it is, print the parent as well.
        if (node_null(top->right)) {
            n = spop_p(s, rbt_node);
            spop(s_int, int);
            printf("%d, ", n->key);

            // is this the right child of the top element?
            if (s.top != -1) {
                top = speek_p(s, rbt_node, 0);
                while ((s.top != -1) && top->right == n) {
                    n = spop_p(s, rbt_node);
                    spop(s_int, int);
                    printf("%d, ", n->key);
                    top = speek_p(s, rbt_node, 0);
                }
            }
        }

        if (s.top != -1) {
            rbt_node *top = speek_p(s, rbt_node, 0);
            n = top->right;
        } else {
            break;
        }
    }

    printf("\n");
    shfree(api, s.arr);
}

void
rbt_level_order(const rbt *t, const alloc_api *api)
{
    printf("Level Order Traversal for the tree is: \n");
    queue_voidp q = qinit_p(api);

    unsigned int level = 0;
    unsigned int c=1, d=1;
    qpush_p(&q, t->root);
    while(q.length != 0) {
        rbt_node *r = qpop_p(&q, rbt_node);
        if (r != NULL && !rbt_is_nil_sentinel_internal(r)) {
            printf("%d, ", r->key);
            qpush_p(&q, r->left);
            qpush_p(&q, r->right);
        } else {
            // printf("XX, ");
        }
        if (--c == 0) {
            d <<= 1;
            c = d;
            ++level;
            printf("\n");
        }
    }

    printf("\n");
    shfree(api, q.arr);
}

void
rbt_display_tree(const rbt *t, const alloc_api *api)
{
    printf("Displaying Red Black Tree := \n");
    queue_voidp q = qinit_p(api);
    qpush_p(&q, t->root);

    unsigned int level = 0;
    int max_depth = rbt_get_depth(t->root);
    assert(max_depth > 0 && max_depth < 32);

    int width_per_item = 2;
    int max_node_count = (1UL<<(max_depth))-1;                  // max number of nodes in the tree.

    int starting_spaces = max_depth > 0 ? ((1UL<<((max_depth-1)-level))-1) : 0;
    int level_ctr_max = 1;                                      // counter for items printed on a per-level basis.
    int max_items_per_level = 1;
    while (q.length != 0 && level < max_depth) {
        rbt_node *r = qpop_p(&q, rbt_node);

        if (level_ctr_max == max_items_per_level) {
            /* printing the first item in a new level. */
            int space_count = starting_spaces;
            if (space_count > 0)
                printf("%*s", space_count*width_per_item, " ");
        } else {
            /* spaces in between items in a level */
            int space_count = ((max_node_count + 1) / (max_items_per_level));
            if (space_count > 0)
                printf("%*s", (space_count-1)*width_per_item, " ");
        }

        if (!rbt_is_nil_sentinel_internal(r)) {
            printf("%*d", width_per_item, r->key);
            qpush_p(&q, r->left);
            qpush_p(&q, r->right);
        } else {
            printf("%*c", width_per_item, ' ');
            qpush_p(&q, &nil_node);
            qpush_p(&q, &nil_node);
        }

        if (--level_ctr_max == 0) {
            printf("\n");
            level++;
            max_items_per_level <<= 1;
            level_ctr_max = max_items_per_level;
            starting_spaces = ((1UL << ((max_depth - 1) - level)) - 1);
        }
    }
    printf("\n\n");
    qfree_p(&q);
}

static bool
rbt_validate_bst_black_or_red(const darr_voidp *inorder)
{
    for (int i = 1; i < inorder->length; ++i) {
        rbt_node *curr_node = arrget_p(inorder, rbt_node, i);
        rbt_node *prev_node = arrget_p(inorder, rbt_node, i-1);
        if (curr_node->key <= prev_node->key) {
            printf("[TREE_INVALID]: items in this tree are not sorted in ascending order. This is not even a "
                   "Binary Search Tree (BST).\n");
            return false;
        }

        char curr_color = curr_node->color;
        if (curr_color != RBT_COLOR_BLACK &&
            curr_color != RBT_COLOR_RED)
        {
            printf("[TREE_INVALID]: A node in the red black tree is not black or red.\n");
            return false;
        }
    }
    return true;
}

static bool
rbt_validate_leaf_node_color(const darr_darr_voidp *paths) {
    for (int i = 0; i < paths->length; ++i) {
        darr_voidp path = arrget(paths, darr_voidp, i);
        for (int j = 0; j < path.length; ++j) {
            rbt_node *node = arrget_p(&path, rbt_node, j);
            rbt_node *lchild = node->left;
            if (lchild != NULL && rbt_is_nil_sentinel_internal(lchild)) {
                if (lchild->color != RBT_COLOR_BLACK) {
                    return false;
                }
            }
            rbt_node *rchild = node->right;
            if (rchild != NULL && rbt_is_nil_sentinel_internal(rchild)) {
                if (rchild->color != RBT_COLOR_BLACK) {
                    return false;
                }
            }
        }
    }
    return true;
}

static bool
rbt_validate_paths_black_node_count(const darr_darr_voidp *paths)
{
    darr_voidp path = arrget(paths, darr_voidp, 0);
    unsigned int black_height_root = get_path_black_height(&path);

    for (int i = 1; i < paths->length; ++i) {
        path = arrget(paths, darr_voidp, i);
        unsigned int curr_black_height = get_path_black_height(&path);
        if (black_height_root != curr_black_height) {
            return false;
        }
    }
    return true;
}

#if 0
static bool
rbt_validate_red_children_should_be_black(const darr_darr_voidp *paths)
{
    for (int i = 0; i < paths->length; ++i) {
        darr_voidp path = arrget(paths, darr_voidp, i);
        for (int j = 1; j < path.length; ++j) {
            rbt_node *curr_node = arrget_p(&path, rbt_node, j);
            rbt_node *prev_node = arrget_p(&path, rbt_node, j-1);
            if (curr_node->color == RBT_COLOR_RED &&
                prev_node->color == RBT_COLOR_RED)
            {
                return false;
            }
        }
    }
    return true;
}
#else
static bool
rbt_validate_red_children_should_be_black(const darr_voidp *inorder)
{
    for (int i = 0; i < inorder->length; ++i) {
        rbt_node *node = arrget_p(inorder, rbt_node, i);
        if (node->color == RBT_COLOR_RED) {
            if (node->left != NULL && node->left->color != RBT_COLOR_BLACK) {
                return false;
            }
            if (node->right != NULL && node->right->color != RBT_COLOR_BLACK) {
                return false;
            }
        }
    }
    return true;
}
#endif

static void
rbt_free_paths_array(darr_darr_voidp *paths, const alloc_api *api)
{
    if (paths == NULL || api == NULL) {
        printf("[free_paths_array]: Null pointers passed in.\n");
        return;
    }

    for (int i = 0; i < paths->length; ++i) {
        darr_voidp path = arrget(paths, darr_voidp, i);
        shfree(api, path.arr);
    }
    shfree(api, paths->arr);
}

bool
rbt_validate_tree(const rbt *t, bool enumerate_paths, bool display_tree, const alloc_api *api)
{
    if (node_null(t->root)) {
        printf("tree passed in is null. Still valid though.\n");
        return true;
    }

    if (display_tree) {
        rbt_display_tree(t, api);
    }

    darr_darr_voidp paths = rbt_get_root_to_leaf_paths(t, api);
    if (enumerate_paths) {
        rbt_print_all_paths(&paths);
    }

    if (t->root->color != RBT_COLOR_BLACK) {
        assert(!"[TREE_INVALID] Root is not colored black.");
        return false;
    }
    // printf("[Rule 1][PASSED]:= Root is colored black.\n");

    darr_voidp inorder = rbt_inorder(t, api);
    if (!rbt_validate_bst_black_or_red(&inorder)) {
        assert(0);
        return false;
    }
    // printf("[Rule 2][PASSED]:= Tree elements are in ascending order. Valid BST.\n");
    // printf("[Rule 3][PASSED]:= Each Tree Node is either Red or Black.\n");

    if (!rbt_validate_red_children_should_be_black(&inorder)) {
        assert(!"[TREE_INVALID]: Red Nodes have Non-Black Children.\n");
        return false;
    }
    // printf("[Rule 4][PASSED]:= Every Red Node has Black Children.\n");
    shfree(api, inorder.arr);

    if (!rbt_validate_paths_black_node_count(&paths)) {
        assert(!"[TREE_INVALID]: All root-to-leaf paths do not have same number of black nodes.\n");
        return false;
    }
    // printf("[Rule 5][PASSED]:= All Root-To-Leaf paths for the tree have same number of black nodes.\n");

    if (!rbt_validate_leaf_node_color(&paths)) {
        assert(!"[TREE_INVALID]: All leaf nodes in the red-black tree should be black.\n");
        return false;
    }
    // printf("[Rule 6][PASSED]:= All leaf nodes in the red-black tree are Black.\n");

    // free the memory used by paths array
    rbt_free_paths_array(&paths, api);
    return true;
}

#include <time.h>
#define max_num 1000000
void
rbt_unit_tests()
{
    Freelist fl;
    size_t mem_size = 512 * 1024 * 1024;
    void *mem = malloc(mem_size);
    freelist_init(&fl, mem, mem_size, DEFAULT_ALIGNMENT);
    fl.policy = PLACEMENT_POLICY_FIND_BEST;

    srand(time(NULL));
#if 1
    rbt rb_tree = rbt_create_tree(&fl.api);

    // size_t used_a = fl.used;
    // rbt_insert_key(&rb_tree, 41);
    // rbt_insert_key(&rb_tree, 38);
    // rbt_insert_key(&rb_tree, 31);
    // rbt_insert_key(&rb_tree, 12);
    // rbt_insert_key(&rb_tree, 19);
    // rbt_insert_key(&rb_tree, 8);
    // rbt_validate_tree(&rb_tree, true, true, &fl.api);
    // size_t used_b = fl.used;
    // rbt_remove_key(&rb_tree, 8);
    // rbt_validate_tree(&rb_tree, true, true, &fl.api);
    // rbt_remove_key(&rb_tree, 12);
    // rbt_validate_tree(&rb_tree, true, true, &fl.api);
    // rbt_remove_key(&rb_tree, 19);
    // rbt_validate_tree(&rb_tree, true, true, &fl.api);
    // rbt_remove_key(&rb_tree, 31);
    // rbt_validate_tree(&rb_tree, true, true, &fl.api);
    // rbt_remove_key(&rb_tree, 38);
    // rbt_validate_tree(&rb_tree, true, true, &fl.api);
    // rbt_remove_key(&rb_tree, 41);
    // rbt_validate_tree(&rb_tree, true, true, &fl.api);
    // size_t used_c = fl.used;
    // assert(used_c == used_a);
    // int x = 0;

    rbt_insert_key(&rb_tree, 10);
    rbt_validate_tree(&rb_tree, true, true, &fl.api);
    rbt_insert_key(&rb_tree, 18);
    rbt_validate_tree(&rb_tree, true, true, &fl.api);
    rbt_insert_key(&rb_tree, 7);
    rbt_validate_tree(&rb_tree, true, true, &fl.api);
    rbt_insert_key(&rb_tree, 15);
    rbt_validate_tree(&rb_tree, true, true, &fl.api);
    rbt_insert_key(&rb_tree, 16);
    rbt_validate_tree(&rb_tree, true, true, &fl.api);
    rbt_insert_key(&rb_tree, 30);
    rbt_validate_tree(&rb_tree, true, true, &fl.api);
    rbt_insert_key(&rb_tree, 25);
    rbt_validate_tree(&rb_tree, true, true, &fl.api);
    rbt_insert_key(&rb_tree, 40);
    rbt_validate_tree(&rb_tree, true, true, &fl.api);
    rbt_insert_key(&rb_tree, 60);
    rbt_validate_tree(&rb_tree, true, true, &fl.api);
    rbt_insert_key(&rb_tree, 2);
    rbt_validate_tree(&rb_tree, true, true, &fl.api);
    rbt_insert_key(&rb_tree, 1);
    rbt_validate_tree(&rb_tree, true, true, &fl.api);
    rbt_insert_key(&rb_tree, 70);
    rbt_validate_tree(&rb_tree, true, true, &fl.api);

    rbt_remove_key(&rb_tree, 60);
    rbt_validate_tree(&rb_tree, true, true, &fl.api);
#else
    typedef struct interval {
        int low, high;
    } interval;
    interval item_count_range;
    item_count_range.low = 32;
    item_count_range.high = RAND_MAX;
    int diff = item_count_range.high - item_count_range.low;

    int num_tests = 1000;
    for (int test_ctr = 0; test_ctr < num_tests; ++test_ctr) {
        rbt tree = rbt_create_tree(&fl.api);
        int rand_item_count = rand() % diff;
        int num_items = item_count_range.low + rand_item_count;
        assert(num_items >= item_count_range.low &&
               num_items < item_count_range.high);

        bool arr[max_num] = {};
        int i=0;
        for (; i<num_items; ++i) {
            int _r = rand() * ((max_num/RAND_MAX) + 1);
            int element = (_r % max_num);
            while (arr[element]) {
                _r = rand() * ((max_num/RAND_MAX) + 1);
                element = (_r % max_num);
            }
            arr[element] = true;
            rbt_insert_key(&tree, element);
        }

        size_t before_used = fl.used;
        /*
        if (num_items > 32000) {
            assert(rbt_validate_tree(&tree, true, true, &fl.api));
        }
        else
        */
        assert(rbt_validate_tree(&tree, false, false, &fl.api));
        size_t after_used = fl.used;
        assert(before_used == after_used);

        freelist_free_all(&fl);
        assert(fl.used == 0);
        float progress = (float)test_ctr / (float)num_tests;
        printf("\rRunning Red-Black Tree Tests... %.2f%%", progress*100.0f);
        fflush(stdout);

        // printf("tree test[%d] with %d items passed!\n", test_ctr, num_items);
    }
#endif
    fflush(stdout);
    printf("\rRunning Red-Black Tree Tests... 100.0%%\n");
    printf("All Red-Black tree unit tests passed!\n");

    free(fl.data);
}
#endif
#endif
#endif