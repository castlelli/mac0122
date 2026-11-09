/* Tests for bst. Prints only the failing cases; exits 0 when every case passes.
   Build and run from the repository root:
   gcc -std=c89 -Wall -Wextra -pedantic -Werror tests/test_bst.c src/bst.c src/sorted_list.c src/text_utils.c -o test_bst */
#include <stdio.h>
#include <string.h>
#include "../src/bst.h"

#define MAX_WALK_LEN 256

static int failure_count = 0;

static void check_true(const char *case_name, int condition)
{
    if (!condition) {
        printf("FAIL %s\n", case_name);
        failure_count++;
    }
}

static int compare_by_html_name(const page *first, const page *second)
{
    return strcmp(first->html_name, second->html_name);
}

/* Records "key:page,page;" for each visit. */
static int record_key(const char *key, const char *display_key,
                      const sorted_list *pages, void *context)
{
    char *walk_text = context;
    const sorted_list_node *node;

    (void)display_key;
    strcat(walk_text, key);
    strcat(walk_text, ":");
    for (node = pages->first_node; node != NULL; node = node->next) {
        strcat(walk_text, node->page_entry->html_name);
        strcat(walk_text, ",");
    }
    strcat(walk_text, ";");
    return 1;
}

/* Records "display;" for each visit. */
static int record_display_key(const char *key, const char *display_key,
                              const sorted_list *pages, void *context)
{
    (void)key;
    (void)pages;
    strcat(context, display_key);
    strcat(context, ";");
    return 1;
}

/* Stops the walk at the key given as context. */
static int stop_at_key(const char *key, const char *display_key,
                       const sorted_list *pages, void *context)
{
    (void)display_key;
    (void)pages;
    return strcmp(key, context) != 0;
}

static void test_walk_is_in_order(void)
{
    char first_name[] = "a";
    char second_name[] = "b";
    page first_page;
    page second_page;
    bst *tree = bst_create(strcmp, compare_by_html_name);
    char walk_text[MAX_WALK_LEN] = "";

    check_true("create returns a tree", tree != NULL);
    if (tree == NULL)
        return;
    memset(&first_page, 0, sizeof(first_page));
    memset(&second_page, 0, sizeof(second_page));
    first_page.html_name = first_name;
    second_page.html_name = second_name;

    check_true("empty tree walks", bst_walk_in_order(tree, record_key, walk_text));
    check_true("empty tree visits nothing", strcmp(walk_text, "") == 0);

    bst_add_page(tree, "grafo", "grafo", &second_page);
    bst_add_page(tree, "arvore", "arvore", &second_page);
    bst_add_page(tree, "union", "union", &first_page);
    bst_add_page(tree, "heap", "heap", &second_page);
    /* A key that exists: same node, list still sorted, repeat listed once. */
    bst_add_page(tree, "grafo", "grafo", &first_page);
    check_true("repeat of a page succeeds", bst_add_page(tree, "grafo", "grafo", &second_page));

    check_true("walk returns 1", bst_walk_in_order(tree, record_key, walk_text));
    check_true("keys in order, pages sorted, listed once",
               strcmp(walk_text, "arvore:b,;grafo:a,b,;heap:b,;union:a,;") == 0);

    check_true("walk stops at a 0 visit", !bst_walk_in_order(tree, stop_at_key, "heap"));
    bst_free(tree);
}

static void test_key_is_copied(void)
{
    char name[] = "a";
    char key[] = "grafo";
    page page_entry;
    bst *tree = bst_create(strcmp, compare_by_html_name);
    char walk_text[MAX_WALK_LEN] = "";

    if (tree == NULL) {
        check_true("create for key copy test", 0);
        return;
    }
    memset(&page_entry, 0, sizeof(page_entry));
    page_entry.html_name = name;
    bst_add_page(tree, key, key, &page_entry);
    key[0] = 'X';
    bst_walk_in_order(tree, record_key, walk_text);
    check_true("the tree keeps its own key", strcmp(walk_text, "grafo:a,;") == 0);
    bst_free(tree);
}

/* The spelling shown is the first page's in list order, whatever order the
   pages arrive in, and a page repeating the key keeps its first spelling. */
static void test_display_key(void)
{
    char first_name[] = "a";
    char second_name[] = "b";
    page first_page;
    page second_page;
    bst *tree = bst_create(strcmp, compare_by_html_name);
    char walk_text[MAX_WALK_LEN] = "";

    if (tree == NULL) {
        check_true("create for display test", 0);
        return;
    }
    memset(&first_page, 0, sizeof(first_page));
    memset(&second_page, 0, sizeof(second_page));
    first_page.html_name = first_name;
    second_page.html_name = second_name;

    bst_add_page(tree, "arvore", "arvore", &second_page);
    bst_add_page(tree, "arvore", "\xC3\x81rvore", &first_page);
    bst_add_page(tree, "arvore", "ARVORE", &first_page);
    bst_add_page(tree, "grafo", "Grafo", &second_page);
    bst_walk_in_order(tree, record_display_key, walk_text);
    check_true("display spelling of the first page by name",
               strcmp(walk_text, "\xC3\x81rvore;Grafo;") == 0);
    bst_free(tree);
}

int main(void)
{
    test_display_key();
    test_walk_is_in_order();
    test_key_is_copied();
    bst_free(NULL);
    return failure_count == 0 ? 0 : 1;
}
