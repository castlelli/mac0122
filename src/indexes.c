#include <stdlib.h>
#include <string.h>
#include "indexes.h"
#include "text_utils.h"

#define YEAR_ONLY_LEN 4
#define FULL_DATE_LEN 10

/* A month and day of 0 mark a year-only date, which puts it first in its year. */
typedef struct date_parts {
    int year;
    int month;
    int day;
} date_parts;

static int is_digit_run(const char *text, int digit_count)
{
    int digit_index;

    for (digit_index = 0; digit_index < digit_count; digit_index++) {
        if (text[digit_index] < '0' || text[digit_index] > '9')
            return 0;
    }
    return 1;
}

static int read_two_digits(const char *text)
{
    return (text[0] - '0') * 10 + (text[1] - '0');
}

static int read_year(const char *text)
{
    return read_two_digits(text) * 100 + read_two_digits(text + 2);
}

/* Returns 1 and fills parts when date has one of the two formats. It does not
   check that the day exists in the month. */
static int parse_date(const char *date, date_parts *parts)
{
    size_t date_len = strlen(date);

    if (date_len == YEAR_ONLY_LEN && is_digit_run(date, YEAR_ONLY_LEN)) {
        parts->year = read_year(date);
        parts->month = 0;
        parts->day = 0;
        return 1;
    }
    if (date_len == FULL_DATE_LEN && is_digit_run(date, 2) && date[2] == '/'
        && is_digit_run(date + 3, 2) && date[5] == '/'
        && is_digit_run(date + 6, YEAR_ONLY_LEN)) {
        parts->day = read_two_digits(date);
        parts->month = read_two_digits(date + 3);
        parts->year = read_year(date + 6);
        return 1;
    }
    return 0;
}

static int get_days_in_month(int month, int year)
{
    static const int DAYS_IN_MONTH[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int is_leap_year = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;

    if (month == 2 && is_leap_year)
        return 29;
    return DAYS_IN_MONTH[month - 1];
}

int indexes_is_date_valid(const char *date)
{
    date_parts parts;

    if (!parse_date(date, &parts))
        return 0;
    if (strlen(date) == YEAR_ONLY_LEN)
        return 1;
    return parts.month >= 1 && parts.month <= 12 && parts.day >= 1
           && parts.day <= get_days_in_month(parts.month, parts.year);
}

int indexes_compare_dates(const char *first_date, const char *second_date)
{
    date_parts first_parts;
    date_parts second_parts;

    /* Callers pass valid dates; text order keeps the result a total order
       even if one slips through. */
    if (!parse_date(first_date, &first_parts) || !parse_date(second_date, &second_parts))
        return strcmp(first_date, second_date);

    if (first_parts.year != second_parts.year)
        return first_parts.year - second_parts.year;
    if (first_parts.month != second_parts.month)
        return first_parts.month - second_parts.month;
    return first_parts.day - second_parts.day;
}

int indexes_compare_pages_by_file_name(const page *first, const page *second)
{
    return strcmp(first->html_name, second->html_name);
}

const char *indexes_get_attribute_value(const page *page_entry, enum page_attribute attribute)
{
    switch (attribute) {
    case ATTRIBUTE_ALGORITHM:
        return page_entry->algorithm;
    case ATTRIBUTE_TECHNIQUE:
        return page_entry->technique;
    case ATTRIBUTE_DATE:
        return page_entry->date;
    case ATTRIBUTE_PROBLEM:
        return page_entry->problem;
    default:
        return NULL;
    }
}

/* Orders by attribute; the comparators below only name the attribute. */
static int compare_pages_by_attribute(const page *first, const page *second,
                                      enum page_attribute attribute)
{
    const char *first_value = indexes_get_attribute_value(first, attribute);
    const char *second_value = indexes_get_attribute_value(second, attribute);
    int comparison;

    if (first_value == NULL && second_value == NULL)
        return indexes_compare_pages_by_file_name(first, second);
    if (first_value == NULL)
        return 1;
    if (second_value == NULL)
        return -1;

    if (attribute == ATTRIBUTE_DATE)
        comparison = indexes_compare_dates(first_value, second_value);
    else
        comparison = text_utils_compare_normalized(first_value, second_value);
    if (comparison != 0)
        return comparison;
    return indexes_compare_pages_by_file_name(first, second);
}

static int compare_pages_by_algorithm(const page *first, const page *second)
{
    return compare_pages_by_attribute(first, second, ATTRIBUTE_ALGORITHM);
}

static int compare_pages_by_technique(const page *first, const page *second)
{
    return compare_pages_by_attribute(first, second, ATTRIBUTE_TECHNIQUE);
}

static int compare_pages_by_date(const page *first, const page *second)
{
    return compare_pages_by_attribute(first, second, ATTRIBUTE_DATE);
}

static int compare_pages_by_problem(const page *first, const page *second)
{
    return compare_pages_by_attribute(first, second, ATTRIBUTE_PROBLEM);
}

sorted_list_compare_pages indexes_get_page_comparator(enum page_attribute attribute)
{
    switch (attribute) {
    case ATTRIBUTE_TECHNIQUE:
        return compare_pages_by_technique;
    case ATTRIBUTE_DATE:
        return compare_pages_by_date;
    case ATTRIBUTE_PROBLEM:
        return compare_pages_by_problem;
    default:
        return compare_pages_by_algorithm;
    }
}

int indexes_does_offer_attribute(enum index_type index, enum page_attribute attribute)
{
    switch (index) {
    case INDEX_TECHNIQUE:
        return attribute != ATTRIBUTE_TECHNIQUE;
    case INDEX_ALGORITHM:
        return attribute != ATTRIBUTE_ALGORITHM;
    case INDEX_DATE:
        return attribute != ATTRIBUTE_DATE;
    case INDEX_PROBLEM:
        return attribute != ATTRIBUTE_PROBLEM;
    default:
        return 1;
    }
}

enum page_attribute indexes_get_default_attribute(enum index_type index)
{
    return index == INDEX_ALGORITHM ? ATTRIBUTE_TECHNIQUE : ATTRIBUTE_ALGORITHM;
}

/* Normalizes value and files page_entry under it, offering value as written
   as the key's display spelling. An empty field adds no key.
   Returns 0 only when memory runs out. */
static int add_value_to_tree(bst *tree, const char *value, const page *page_entry)
{
    char *key;
    int has_added;

    if (value == NULL)
        return 1;
    key = text_utils_normalize(value);
    if (key == NULL)
        return 0;
    has_added = bst_add_page(tree, key, value, page_entry);
    free(key);
    return has_added;
}

static int add_page_to_indexes(site_indexes *indexes, const page *page_entry)
{
    int value_index;

    if (!add_value_to_tree(indexes->trees[INDEX_TECHNIQUE], page_entry->technique, page_entry)
        || !add_value_to_tree(indexes->trees[INDEX_ALGORITHM], page_entry->algorithm, page_entry)
        || !add_value_to_tree(indexes->trees[INDEX_DATE], page_entry->date, page_entry)
        || !add_value_to_tree(indexes->trees[INDEX_PROBLEM], page_entry->problem, page_entry))
        return 0;

    for (value_index = 0; value_index < page_entry->authors_count; value_index++) {
        if (!add_value_to_tree(indexes->trees[INDEX_AUTHORS],
                               page_entry->authors[value_index], page_entry))
            return 0;
    }
    for (value_index = 0; value_index < page_entry->concepts_count; value_index++) {
        if (!add_value_to_tree(indexes->trees[INDEX_CONCEPTS],
                               page_entry->concepts[value_index], page_entry))
            return 0;
    }
    return 1;
}

site_indexes *indexes_build(page *const *pages, int page_count)
{
    site_indexes *indexes = malloc(sizeof(site_indexes));
    int index;
    int page_index;

    if (indexes == NULL)
        return NULL;

    /* All NULL first, so a failure below can call indexes_free at any point. */
    for (index = 0; index < INDEX_COUNT; index++)
        indexes->trees[index] = NULL;

    for (index = 0; index < INDEX_COUNT; index++) {
        indexes->trees[index] = bst_create(index == INDEX_DATE ? indexes_compare_dates : strcmp,
                                           indexes_compare_pages_by_file_name);
        if (indexes->trees[index] == NULL) {
            indexes_free(indexes);
            return NULL;
        }
    }

    for (page_index = 0; page_index < page_count; page_index++) {
        if (!add_page_to_indexes(indexes, pages[page_index])) {
            indexes_free(indexes);
            return NULL;
        }
    }
    return indexes;
}

void indexes_free(site_indexes *indexes)
{
    int index;

    if (indexes == NULL)
        return;
    for (index = 0; index < INDEX_COUNT; index++)
        bst_free(indexes->trees[index]);
    free(indexes);
}
