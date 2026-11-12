#ifndef TEXT_UTILS_H
#define TEXT_UTILS_H

#include <stdio.h>

/* Removes spaces, tabs, '\r' and '\n' from both ends of line.
   Changes line (writes '\0' at the new end), so it does not accept a
   string literal. Returns a pointer into line, not a copy: do not free it. */
char *text_utils_trim(char* line);

/* Returns a new malloc'd copy of str. The caller owns the copy and frees
   it. Returns NULL if memory runs out. */
char *text_utils_copy_str(const char* str);

/* Splits line at the commas, trims each piece and skips the empty ones.
   Each piece is copied with malloc. Changes line (commas become '\0'),
   so it does not accept a string literal. Writes in *count how many
   strings were kept.
   Returns the string vector, or NULL (with *count = 0) when no piece is
   left or memory runs out. The caller owns the vector and the strings
   and frees them all with text_utils_free_str_vector. */
char** text_utils_split_comma(char* line, int* count);

/* Frees the first count strings of str_vector and then the vector.
   Accepts a NULL str_vector with count 0. */
void text_utils_free_str_vector(char **, int);

/* Removes spaces, tabs, '\r' and '\n' from the right end of line only.
   The start is kept, so it fits lines whose indentation matters (code).
   Changes line (writes '\0' at the new end), so it does not accept a
   string literal. Returns line itself, not a copy: do not free it. */
char* text_utils_trim_end(char* line);

/* Returns a new malloc'd copy of text in the form used for index keys: ASCII
   letters in lowercase and two-byte UTF-8 Latin letters without their accent
   ("\xC3\x81rvore" gives "arvore"). A letter with no one-letter base (such as
   the ae ligature) and every other byte are kept. Assumes UTF-8; it does not
   validate. The caller frees the copy. Returns NULL if memory runs out. */
char *text_utils_normalize(const char *text);

/* Compares first and second as strcmp would compare their normalized forms,
   without allocating, so a sort comparator built on it cannot fail.
   Returns a negative value, 0 or a positive value. */
int text_utils_compare_normalized(const char *first, const char *second);

/* Writes text to html_file as part of a URL: letters, digits and "-._~" as
   they are, every other byte as %XX. The result is safe inside a double-quoted
   href attribute. Nothing is allocated; the FILE stays owned by the caller. */
void text_utils_write_url_component(FILE *html_file, const char *text);

/* Writes text to html_file as an anchor id: letters and digits as they are,
   every other byte as _XX. Two different texts never share an id, and the
   result is safe both in an id attribute and after '#' in an href. Nothing is
   allocated; the FILE stays owned by the caller. */
void text_utils_write_anchor_id(FILE *html_file, const char *text);

/*
 * HTML escape shared by html.c and the C highlighter.
 *
 * Only '<', '>' and '&' are replaced: callers write element content,
 * never attribute values, so quotes stay as they are.
 *
 * Ownership: nothing is allocated; the FILE stays owned by the caller.
 */

/* Writes one character (a value from fgetc) to html_file, escaped. */
void text_utils_write_html_character(FILE *html_file, int character);

/* Writes a NUL-terminated string to html_file, escaped. */
void text_utils_write_html_string(FILE *html_file, const char *text);

#endif
