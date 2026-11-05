#ifndef TEXT_UTILS_H
#define TEXT_UTILS_H

#include <stdio.h>

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
