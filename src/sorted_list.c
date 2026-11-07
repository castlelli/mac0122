#include <stdlib.h>
#include "sorted_list.h"

sorted_list *sorted_list_create(sorted_list_compare_pages compare_pages)
{
    sorted_list *list = malloc(sizeof(sorted_list));

    if (list == NULL)
        return NULL;
    list->compare_pages = compare_pages;
    list->first_node = NULL;
    return list;
}

int sorted_list_insert_unique(sorted_list *list, const page *page_entry)
{
    /* The link to rewrite, so inserting first needs no special case. */
    sorted_list_node **link = &list->first_node;
    sorted_list_node *new_node;

    while (*link != NULL && list->compare_pages((*link)->page_entry, page_entry) < 0)
        link = &(*link)->next;

    /* The order is total, so a page already in the list would stop here. */
    if (*link != NULL && (*link)->page_entry == page_entry)
        return 1;

    new_node = malloc(sizeof(sorted_list_node));
    if (new_node == NULL)
        return 0;
    new_node->page_entry = page_entry;
    new_node->next = *link;
    *link = new_node;
    return 1;
}

void sorted_list_free(sorted_list *list)
{
    sorted_list_node *node;
    sorted_list_node *next_node;

    if (list == NULL)
        return;
    for (node = list->first_node; node != NULL; node = next_node) {
        next_node = node->next;
        free(node);
    }
    free(list);
}
