#ifndef C_KEYWORDS_H
#define C_KEYWORDS_H

#include <stddef.h>

/*
 * Words the C highlighter paints specially, kept in one array sorted by
 * strcmp order and searched with binary search (the set is fixed, so a
 * sorted array fits better than a BST).
 *
 * Ownership: the table is static and read-only; nothing is allocated.
 */

typedef enum {
    C_KEYWORD_NONE,     /* not a known word: an ordinary identifier */
    C_KEYWORD_TYPE,     /* int, char, size_t, FILE, uint8_t, bool, ... */
    C_KEYWORD_COMMAND,  /* if, return, struct, static, const, ... */
    C_KEYWORD_BUILTIN   /* true, false, NULL */
} c_keyword_kind;

/* Looks up the first word_length characters of word (not NUL-terminated). */
c_keyword_kind c_keywords_lookup(const char *word, size_t word_length);

/* Returns 1 when the table is in strictly increasing order (for tests). */
int c_keywords_is_table_sorted(void);

#endif
