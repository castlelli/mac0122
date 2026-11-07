/* Tests for indexes. Prints only the failing cases; exits 0 when every case
   passes. Build and run from the repository root:
   gcc -std=c89 -Wall -Wextra -pedantic -Werror tests/test_indexes.c src/indexes.c src/bst.c src/sorted_list.c src/page.c src/text_utils.c -o test_indexes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/indexes.h"
#include "../src/text_utils.h"

#define MAX_WALK_LEN 1024

static int failure_count = 0;

static void check_true(const char *case_name, int condition)
{
    if (!condition) {
        printf("FAIL %s\n", case_name);
        failure_count++;
    }
}

static char *copy_optional_str(const char *text)
{
    return text == NULL ? NULL : text_utils_copy_str(text);
}

/* authors and concepts are one comma-separated line, or NULL for none. */
static char **split_optional_values(const char *values, int *value_count)
{
    char line[256];

    *value_count = 0;
    if (values == NULL)
        return NULL;
    strcpy(line, values);
    return text_utils_split_comma(line, value_count);
}

static page *build_page(const char *html_name, const char *algorithm, const char *technique,
                        const char *date, const char *problem, const char *authors,
                        const char *concepts)
{
    page *page_entry = page_create();

    if (page_entry == NULL)
        exit(1);
    page_entry->html_name = copy_optional_str(html_name);
    page_entry->algorithm = copy_optional_str(algorithm);
    page_entry->technique = copy_optional_str(technique);
    page_entry->date = copy_optional_str(date);
    page_entry->problem = copy_optional_str(problem);
    page_entry->authors = split_optional_values(authors, &page_entry->authors_count);
    page_entry->concepts = split_optional_values(concepts, &page_entry->concepts_count);
    return page_entry;
}

static int record_key(const char *key, const char *display_key,
                      const sorted_list *pages, void *context)
{
    char *walk_text = context;
    const sorted_list_node *node;

    strcat(walk_text, key);
    if (strcmp(key, display_key) != 0) {
        strcat(walk_text, "=");
        strcat(walk_text, display_key);
    }
    strcat(walk_text, ":");
    for (node = pages->first_node; node != NULL; node = node->next) {
        strcat(walk_text, node->page_entry->html_name);
        strcat(walk_text, ",");
    }
    strcat(walk_text, ";");
    return 1;
}

static void check_index(const char *case_name, const site_indexes *indexes,
                        enum index_type index, const char *expected_walk)
{
    char walk_text[MAX_WALK_LEN] = "";

    bst_walk_in_order(indexes->trees[index], record_key, walk_text);
    if (strcmp(walk_text, expected_walk) != 0) {
        printf("FAIL %s: got [%s], expected [%s]\n", case_name, walk_text, expected_walk);
        failure_count++;
    }
}

static void test_build(void)
{
    page *pages[3];
    site_indexes *indexes;
    int page_index;

    pages[0] = build_page("b.html", "Kruskal", "Guloso", "1956", "Mesma frase",
                          "Knuth, Knuth", "\xC3\x81rvore, grafo");
    pages[1] = build_page("a.html", "Prim", "guloso", "12/03/1956", "MESMA FRASE",
                          "Jarn\xC3\xADk", "\xC3\xA1rvore");
    pages[2] = build_page("c.html", NULL, NULL, NULL, NULL, NULL, NULL);

    indexes = indexes_build(pages, 3);
    check_true("build returns indexes", indexes != NULL);
    if (indexes != NULL) {
        check_index("technique: normalized, one key, spelled as a.html does", indexes,
                    INDEX_TECHNIQUE, "guloso:a.html,b.html,;");
        check_index("algorithm: display keeps the capital", indexes, INDEX_ALGORITHM,
                    "kruskal=Kruskal:b.html,;prim=Prim:a.html,;");
        check_index("authors: each value a key, repeat once", indexes, INDEX_AUTHORS,
                    "jarnik=Jarn\xC3\xADk:a.html,;knuth=Knuth:b.html,;");
        check_index("date: year before full date", indexes, INDEX_DATE,
                    "1956:b.html,;12/03/1956:a.html,;");
        check_index("concepts: accents share a node, a.html spells it", indexes, INDEX_CONCEPTS,
                    "arvore=\xC3\xA1rvore:a.html,b.html,;grafo:b.html,;");
        check_index("problem: whole normalized sentence", indexes, INDEX_PROBLEM,
                    "mesma frase=MESMA FRASE:a.html,b.html,;");
    }
    indexes_free(indexes);
    indexes_free(NULL);
    for (page_index = 0; page_index < 3; page_index++)
        page_free(pages[page_index]);
}

static void test_date_validity(void)
{
    check_true("year is valid", indexes_is_date_valid("1956"));
    check_true("full date is valid", indexes_is_date_valid("12/03/1956"));
    check_true("leap day is valid", indexes_is_date_valid("29/02/2000"));
    check_true("31 January is valid", indexes_is_date_valid("31/01/1956"));

    check_true("31 February is invalid", !indexes_is_date_valid("31/02/1956"));
    check_true("non-leap 29 February is invalid", !indexes_is_date_valid("29/02/1900"));
    check_true("one-digit month is invalid", !indexes_is_date_valid("12/3/1956"));
    check_true("ISO date is invalid", !indexes_is_date_valid("1956-03-12"));
    check_true("month 13 is invalid", !indexes_is_date_valid("01/13/1956"));
    check_true("month 0 is invalid", !indexes_is_date_valid("01/00/1956"));
    check_true("day 0 is invalid", !indexes_is_date_valid("00/01/1956"));
    check_true("three-digit year is invalid", !indexes_is_date_valid("956"));
    check_true("text is invalid", !indexes_is_date_valid("abcd"));
    check_true("empty is invalid", !indexes_is_date_valid(""));
}

static void test_date_order(void)
{
    check_true("year before full date of that year",
               indexes_compare_dates("1956", "12/03/1956") < 0);
    check_true("full date after its year", indexes_compare_dates("01/01/1956", "1956") > 0);
    check_true("earlier year first", indexes_compare_dates("1956", "1957") < 0);
    check_true("full date before a later year", indexes_compare_dates("31/12/1956", "1957") < 0);
    check_true("not compared as text", indexes_compare_dates("02/01/1957", "01/02/1956") > 0);
    check_true("month before day", indexes_compare_dates("31/01/1956", "01/02/1956") < 0);
    check_true("day last", indexes_compare_dates("01/02/1956", "02/02/1956") < 0);
    check_true("same date", indexes_compare_dates("12/03/1956", "12/03/1956") == 0);
}

static void test_page_order(void)
{
    page *first = build_page("a.html", "Zeta", "X", "1957", "P", NULL, NULL);
    page *second = build_page("b.html", "\xC3\x81lfa", "X", "1956", NULL, NULL, NULL);
    page *third = build_page("c.html", NULL, "X", NULL, NULL, NULL, NULL);
    page *fourth = build_page("d.html", NULL, "X", NULL, NULL, NULL, NULL);
    sorted_list_compare_pages compare_pages;

    compare_pages = indexes_get_page_comparator(ATTRIBUTE_ALGORITHM);
    check_true("algorithm: accent sorts as its base", compare_pages(second, first) < 0);
    check_true("algorithm: empty after a value", compare_pages(third, first) > 0);
    check_true("algorithm: value before empty", compare_pages(first, third) < 0);
    check_true("algorithm: empties by file name", compare_pages(third, fourth) < 0);

    compare_pages = indexes_get_page_comparator(ATTRIBUTE_TECHNIQUE);
    check_true("technique: tie by file name", compare_pages(first, second) < 0);
    check_true("technique: tie by file name, reversed", compare_pages(second, first) > 0);
    check_true("technique: same page is 0", compare_pages(first, first) == 0);

    compare_pages = indexes_get_page_comparator(ATTRIBUTE_DATE);
    check_true("date: compared as dates", compare_pages(second, first) < 0);
    check_true("date: empty last", compare_pages(third, first) > 0);

    compare_pages = indexes_get_page_comparator(ATTRIBUTE_PROBLEM);
    check_true("problem: empty last", compare_pages(second, first) > 0);
    check_true("problem: empties by file name", compare_pages(second, third) < 0);

    page_free(first);
    page_free(second);
    page_free(third);
    page_free(fourth);
}

static void test_offered_orders(void)
{
    int order_count = 0;
    int index;
    int attribute;

    for (index = 0; index < INDEX_COUNT; index++) {
        for (attribute = 0; attribute < ATTRIBUTE_COUNT; attribute++) {
            if (indexes_does_offer_attribute((enum index_type)index,
                                             (enum page_attribute)attribute))
                order_count++;
        }
        check_true("the default order is offered",
                   indexes_does_offer_attribute((enum index_type)index,
                       indexes_get_default_attribute((enum index_type)index)));
    }
    check_true("twenty index pages in all", order_count == 20);
    check_true("an index does not offer its own attribute",
               !indexes_does_offer_attribute(INDEX_TECHNIQUE, ATTRIBUTE_TECHNIQUE)
               && !indexes_does_offer_attribute(INDEX_DATE, ATTRIBUTE_DATE));
}

int main(void)
{
    test_build();
    test_date_validity();
    test_date_order();
    test_page_order();
    test_offered_orders();
    return failure_count == 0 ? 0 : 1;
}
