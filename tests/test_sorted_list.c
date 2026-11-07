/* Tests for sorted_list. Prints only the failing cases; exits 0 when every case
   passes. Build and run from the repository root:
   gcc -std=c89 -Wall -Wextra -pedantic -Werror tests/test_sorted_list.c src/sorted_list.c -o test_sorted_list */
#include <stdio.h>
#include <string.h>
#include "../src/sorted_list.h"

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

static page build_page(char *html_name)
{
    page page_entry;

    memset(&page_entry, 0, sizeof(page_entry));
    page_entry.html_name = html_name;
    return page_entry;
}

static void test_insert_keeps_order(void)
{
    char first_name[] = "a.html";
    char second_name[] = "b.html";
    char third_name[] = "c.html";
    page first_page = build_page(first_name);
    page second_page = build_page(second_name);
    page third_page = build_page(third_name);
    sorted_list *list = sorted_list_create(compare_by_html_name);
    const sorted_list_node *node;

    check_true("create returns a list", list != NULL);
    if (list == NULL)
        return;
    check_true("new list is empty", list->first_node == NULL);

    /* Out of order: end, start, then middle. */
    check_true("insert last", sorted_list_insert_unique(list, &third_page));
    check_true("insert first", sorted_list_insert_unique(list, &first_page));
    check_true("insert middle", sorted_list_insert_unique(list, &second_page));

    node = list->first_node;
    check_true("first in order", node != NULL && node->page_entry == &first_page);
    node = node->next;
    check_true("second in order", node != NULL && node->page_entry == &second_page);
    node = node->next;
    check_true("third in order", node != NULL && node->page_entry == &third_page);
    check_true("list ends after three", node->next == NULL);
    sorted_list_free(list);
}

static void test_insert_unique(void)
{
    char first_name[] = "a.html";
    char second_name[] = "b.html";
    page first_page = build_page(first_name);
    page second_page = build_page(second_name);
    sorted_list *list = sorted_list_create(compare_by_html_name);
    const sorted_list_node *node;
    int node_count = 0;

    if (list == NULL) {
        check_true("create for unique test", 0);
        return;
    }
    sorted_list_insert_unique(list, &first_page);
    sorted_list_insert_unique(list, &second_page);
    check_true("insert of a repeated page succeeds", sorted_list_insert_unique(list, &first_page));
    check_true("second repeat succeeds", sorted_list_insert_unique(list, &second_page));
    for (node = list->first_node; node != NULL; node = node->next)
        node_count++;
    check_true("a repeated page is listed once", node_count == 2);
    sorted_list_free(list);
}

int main(void)
{
    test_insert_keeps_order();
    test_insert_unique();
    /* Frees nodes only: pages here are on the stack, so ASan would catch it. */
    sorted_list_free(NULL);
    return failure_count == 0 ? 0 : 1;
}
