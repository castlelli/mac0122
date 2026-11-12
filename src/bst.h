#ifndef BST_H
#define BST_H

#include "sorted_list.h"

/*
 * Binary search tree of index keys. Each node holds one key, the spelling to
 * display for it, and the sorted list of the pages under it. The tree is not
 * balanced.
 *
 * Ownership: bst_create allocates the tree and bst_free frees it, with every
 * node, key copy, display copy and list. The pages are only borrowed from main.
 */

typedef int (*bst_compare_keys)(const char *first_key, const char *second_key);

/* Called with each key, its display spelling and its pages. Returns 1 to go on
   and 0 to stop. The strings and the list stay owned by the tree; context is
   whatever the caller passed. */
typedef int (*bst_visit_key)(const char *key, const char *display_key,
                             const sorted_list *pages, void *context);

typedef struct bst_node {
    char *key;
    /* The spelling of the page that comes first in compare_pages order. */
    char *display_key;
    const page *display_page;
    sorted_list *pages;
    struct bst_node *left;
    struct bst_node *right;
} bst_node;

typedef struct bst {
    bst_compare_keys compare_keys;
    sorted_list_compare_pages compare_pages;
    bst_node *root;
} bst;

/* Returns an empty tree, or NULL if memory runs out. compare_keys orders the
   keys; compare_pages orders each node's list. */
bst *bst_create(bst_compare_keys compare_keys, sorted_list_compare_pages compare_pages);

/* Adds page_entry under key, creating the node with a copy of key if needed.
   A page is listed once per key. display_key is how this page spells the key;
   the node keeps a copy of the spelling of its first page in compare_pages
   order, so the choice does not depend on the order pages are added.
   Returns 1 on success and 0 if memory runs out. */
int bst_add_page(bst *tree, const char *key, const char *display_key,
                 const page *page_entry);

/* Visits the keys in order. Returns 1 if every visit returned 1, and 0 as soon
   as one returns 0. */
int bst_walk_in_order(const bst *tree, bst_visit_key visit, void *context);

/* Frees the nodes in post-order, then the tree. Does nothing if tree is NULL. */
void bst_free(bst *tree);

#endif
