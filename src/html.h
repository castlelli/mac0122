#ifndef HTML_H
#define HTML_H

#include <stdio.h>
#include "indexes.h"
#include "page.h"

/*
 * Writes the static site: style.css, index.html, the index pages (one per index
 * and sort order, <index>_by_<attribute>.html) and one content page per TXT.
 *
 * Every text from a TXT goes through the HTML escape. Accents are written as
 * they are, so pages declare UTF-8, the encoding the TXTs must use.
 *
 * Ownership: nothing here keeps a pointer after returning. Files passed in stay
 * open and owned by the caller; pages, bodies and indexes stay owned by theirs.
 *
 * Functions returning int return 1 on success and 0 on failure (memory, or a
 * file that cannot be written, reported on stderr).
 */

/* Writes the highlighter's CSS followed by the site's layout rules. */
void html_write_style_sheet(FILE *css_file);

/* Writes index.html: the links to the six indexes and every page by file name. */
int html_write_home_page(FILE *html_file, page *const *pages, int page_count);

/* Writes the page of index sorted by attribute, which the index must offer. */
int html_write_index_page(FILE *html_file, const site_indexes *indexes,
                          enum index_type index, enum page_attribute attribute);

/* Writes one content page: the header fields, with each author and concept
   linked to its key, then the body, then all references. */
int html_write_content_page(FILE *html_file, const page *page_header, const page_body *body);

/* Writes style.css, index.html and every index page into output_directory. */
int html_write_index_files(const char *output_directory, const site_indexes *indexes,
                           page *const *pages, int page_count);

/* Writes page_header->html_name into output_directory. */
int html_write_content_file(const char *output_directory, const page *page_header,
                            const page_body *body);

/* Returns 1 when file_name is index.html or an index page, so a content page
   must not take that name. */
int html_is_generated_file_name(const char *file_name);

#endif
