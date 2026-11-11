#ifndef SORTED_LIST_H
#define SORTED_LIST_H

#include "page.h"

/*
 * Sorted linked list of pointers to header structs.
 *
 * Ownership: sorted_list_create allocates the list and sorted_list_free frees
 * the list and its nodes. The pages are only borrowed: main owns them, and they
 * must outlive every list holding them.
 */

/* Orders two pages: negative, 0 or positive. It must be a total order, with 0
   only when both are the same page, so the sorted order is complete. */
typedef int (*sorted_list_compare_pages)(const page *first, const page *second);

/* Read the list by walking from list->first_node through next. */
typedef struct sorted_list_node {
    const page *page_entry;
    struct sorted_list_node *next;
} sorted_list_node;

typedef struct sorted_list {
    sorted_list_compare_pages compare_pages;
    sorted_list_node *first_node;
} sorted_list;

/* Returns an empty list sorted by compare_pages, or NULL if memory runs out. */
sorted_list *sorted_list_create(sorted_list_compare_pages compare_pages);

/* Adds page_entry in its sorted position, unless that page is already in the
   list. Returns 1 on success, also when the page was already there, and 0 if
   memory runs out; on failure the list is left as it was. */
int sorted_list_insert_unique(sorted_list *list, const page *page_entry);

/* Frees the nodes and the list, never the pages. Does nothing if list is NULL. */
void sorted_list_free(sorted_list *list);

#endif
