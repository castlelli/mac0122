#ifndef INDEXES_H
#define INDEXES_H

#include "bst.h"
#include "page.h"

/*
 * The six indexes of the site, built from the header structs, and the rules
 * for ordering dates and pages.
 *
 * Ownership: indexes_build allocates the indexes and indexes_free frees them.
 * The pages stay owned by main and must outlive the indexes.
 */

enum index_type {
    INDEX_TECHNIQUE,
    INDEX_ALGORITHM,
    INDEX_AUTHORS,
    INDEX_DATE,
    INDEX_CONCEPTS,
    INDEX_PROBLEM,
    INDEX_COUNT
};

/* The single-valued header attributes, which are the sort orders. */
enum page_attribute {
    ATTRIBUTE_ALGORITHM,
    ATTRIBUTE_TECHNIQUE,
    ATTRIBUTE_DATE,
    ATTRIBUTE_PROBLEM,
    ATTRIBUTE_COUNT
};

typedef struct site_indexes {
    bst *trees[INDEX_COUNT];
} site_indexes;

/* Builds all six indexes. Keys are normalized, each author and concept is its
   own key, and an empty field adds no key. Each key displays the spelling of
   the first page, by file name, that uses it. Every date must already have passed
   indexes_is_date_valid. Returns NULL if memory runs out. */
site_indexes *indexes_build(page *const *pages, int page_count);

/* Frees the indexes, never the pages. Does nothing if indexes is NULL. */
void indexes_free(site_indexes *indexes);

/* Returns 1 for an existing date written dd/MM/YYYY, or for YYYY; 0 otherwise. */
int indexes_is_date_valid(const char *date);

/* Orders two valid dates as dates. A year-only date comes before every full
   date of the same year. Returns negative, 0 or positive. */
int indexes_compare_dates(const char *first_date, const char *second_date);

/* Orders pages by TXT file name. */
int indexes_compare_pages_by_file_name(const page *first, const page *second);

/* Returns the comparator of one sort order: by attribute, with empty values
   after all others, and ties broken by TXT file name. */
sorted_list_compare_pages indexes_get_page_comparator(enum page_attribute attribute);

/* Returns the page's value for attribute, or NULL when it is empty. The string
   stays owned by the page. */
const char *indexes_get_attribute_value(const page *page_entry, enum page_attribute attribute);

/* Returns 1 when index offers attribute as a sort order, that is, unless the
   attribute is the index's own: every page under a key has the same value. */
int indexes_does_offer_attribute(enum index_type index, enum page_attribute attribute);

/* Returns the order the index opens with: algorithm, or technique for the
   algorithm index. */
enum page_attribute indexes_get_default_attribute(enum index_type index);

#endif
