#include <stdlib.h>
#include "bst.h"
#include "text_utils.h"

bst *bst_create(bst_compare_keys compare_keys, sorted_list_compare_pages compare_pages)
{
    bst *tree = malloc(sizeof(bst));

    if (tree == NULL)
        return NULL;
    tree->compare_keys = compare_keys;
    tree->compare_pages = compare_pages;
    tree->root = NULL;
    return tree;
}

static bst_node *create_node(const bst *tree, const char *key)
{
    bst_node *node = malloc(sizeof(bst_node));

    if (node == NULL)
        return NULL;
    node->key = text_utils_copy_str(key);
    node->display_key = NULL;
    node->display_page = NULL;
    node->pages = sorted_list_create(tree->compare_pages);
    node->left = NULL;
    node->right = NULL;
    if (node->key == NULL || node->pages == NULL) {
        free(node->key);
        sorted_list_free(node->pages);
        free(node);
        return NULL;
    }
    return node;
}

/* Keeps the spelling of the page that sorts first. A page that repeats the key
   keeps its first spelling, because only a strictly earlier page replaces it.
   Returns 0 only when memory runs out; the node is then unchanged. */
static int update_display_key(const bst *tree, bst_node *node, const char *display_key,
                              const page *page_entry)
{
    char *display_copy;

    if (node->display_page != NULL
        && tree->compare_pages(page_entry, node->display_page) >= 0)
        return 1;
    display_copy = text_utils_copy_str(display_key);
    if (display_copy == NULL)
        return 0;
    free(node->display_key);
    node->display_key = display_copy;
    node->display_page = page_entry;
    return 1;
}

int bst_add_page(bst *tree, const char *key, const char *display_key,
                 const page *page_entry)
{
    /* The link to fill in, so an empty tree needs no special case. */
    bst_node **link = &tree->root;
    int comparison;

    while (*link != NULL) {
        comparison = tree->compare_keys(key, (*link)->key);
        if (comparison == 0)
            break;
        link = comparison < 0 ? &(*link)->left : &(*link)->right;
    }

    if (*link == NULL) {
        *link = create_node(tree, key);
        if (*link == NULL)
            return 0;
    }
    return update_display_key(tree, *link, display_key, page_entry)
           && sorted_list_insert_unique((*link)->pages, page_entry);
}

static int walk_node(const bst_node *node, bst_visit_key visit, void *context)
{
    if (node == NULL)
        return 1;
    return walk_node(node->left, visit, context)
           && visit(node->key, node->display_key, node->pages, context)
           && walk_node(node->right, visit, context);
}

int bst_walk_in_order(const bst *tree, bst_visit_key visit, void *context)
{
    return walk_node(tree->root, visit, context);
}

static void free_node(bst_node *node)
{
    if (node == NULL)
        return;
    free_node(node->left);
    free_node(node->right);
    free(node->key);
    free(node->display_key);
    sorted_list_free(node->pages);
    free(node);
}

void bst_free(bst *tree)
{
    if (tree == NULL)
        return;
    free_node(tree->root);
    free(tree);
}
