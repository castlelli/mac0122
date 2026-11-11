#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph.h"
#include "html.h"
#include "indexes.h"
#include "page.h"
#include "parser.h"

#define OUTPUT_DIRECTORY "output"
#define TEMPLATE_FILE_NAME "template.txt"

/* Shows the format and is copied to start a new TXT, so it is never a page. */
static int is_template_file(const char *file_path)
{
    const char *name_start = file_path;
    const char *cursor;

    for (cursor = file_path; *cursor != '\0'; cursor++) {
        if (*cursor == '/' || *cursor == '\\')
            name_start = cursor + 1;
    }
    return strcmp(name_start, TEMPLATE_FILE_NAME) == 0;
}

static void free_pages(page **pages, int page_count)
{
    int page_index;

    for (page_index = 0; page_index < page_count; page_index++)
        page_free(pages[page_index]);
    free(pages);
}

/* Pass 1. Fills pages and *page_count with the headers read, which the caller
   frees whatever the result. Returns 0 at the first file that cannot be read;
   the parser has already reported why. */
static int read_headers(int file_count, char *const *file_paths, page **pages, int *page_count)
{
    int file_index;

    *page_count = 0;
    for (file_index = 0; file_index < file_count; file_index++) {
        if (is_template_file(file_paths[file_index]))
            continue;
        pages[*page_count] = parser_read_header(file_paths[file_index]);
        if (pages[*page_count] == NULL)
            return 0;
        (*page_count)++;
    }
    return 1;
}

/* Checks what the indexes and the output files rely on, before anything is
   written, so a bad TXT cannot leave a partial site. Reports every problem. */
static int are_headers_valid(page *const *pages, int page_count)
{
    int are_valid = 1;
    int page_index;
    int other_index;

    for (page_index = 0; page_index < page_count; page_index++) {
        if (pages[page_index]->date != NULL && !indexes_is_date_valid(pages[page_index]->date)) {
            fprintf(stderr, "%s: invalid Data \"%s\" (use dd/MM/YYYY or YYYY)\n",
                    pages[page_index]->txt_path, pages[page_index]->date);
            are_valid = 0;
        }
        if (html_is_generated_file_name(pages[page_index]->html_name)) {
            fprintf(stderr, "%s: the name %s belongs to a generated page\n",
                    pages[page_index]->txt_path, pages[page_index]->html_name);
            are_valid = 0;
        }
        for (other_index = 0; other_index < page_index; other_index++) {
            if (strcmp(pages[page_index]->html_name, pages[other_index]->html_name) == 0) {
                fprintf(stderr, "%s: same page name as %s\n",
                        pages[page_index]->txt_path, pages[other_index]->txt_path);
                are_valid = 0;
            }
        }
    }
    return are_valid;
}

/* Pass 2: one body in memory at a time. */
static int write_content_files(page *const *pages, int page_count)
{
    page_body *body;
    int has_written;
    int page_index;

    for (page_index = 0; page_index < page_count; page_index++) {
        body = parser_read_body(pages[page_index]);
        if (body == NULL) {
            fprintf(stderr, "%s: could not read the body\n", pages[page_index]->txt_path);
            return 0;
        }
        has_written = html_write_content_file(OUTPUT_DIRECTORY, pages[page_index], body);
        page_body_free(body);
        if (!has_written)
            return 0;
    }
    return 1;
}

int main(int argument_count, char *arguments[])
{
    page **pages;
    site_indexes *indexes;
    graph *site_graph;
    int page_count;
    int has_succeeded;

    if (argument_count < 2) {
        fprintf(stderr, "usage: %s input/*.txt\n", arguments[0]);
        return 1;
    }

    pages = malloc((size_t)(argument_count - 1) * sizeof(page *));
    if (pages == NULL) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    if (!read_headers(argument_count - 1, arguments + 1, pages, &page_count)) {
        free_pages(pages, page_count);
        return 1;
    }
    if (page_count == 0) {
        fprintf(stderr, "no TXT to process: %s is not a page\n", TEMPLATE_FILE_NAME);
        free_pages(pages, page_count);
        return 1;
    }
    if (!are_headers_valid(pages, page_count)) {
        free_pages(pages, page_count);
        return 1;
    }

    indexes = indexes_build(pages, page_count);
    if (indexes == NULL) {
        fprintf(stderr, "out of memory\n");
        free_pages(pages, page_count);
        return 1;
    }

    /* The graph uses the headers and indexes only, like the index pages. */
    site_graph = graph_build(pages, page_count, indexes);
    if (site_graph == NULL || !graph_layout_global(site_graph)) {
        fprintf(stderr, "out of memory\n");
        graph_free(site_graph);
        indexes_free(indexes);
        free_pages(pages, page_count);
        return 1;
    }

    has_succeeded = html_write_index_files(OUTPUT_DIRECTORY, indexes)
                    && html_write_graph_files(OUTPUT_DIRECTORY, site_graph)
                    && write_content_files(pages, page_count);

    graph_free(site_graph);
    indexes_free(indexes);
    free_pages(pages, page_count);
    return has_succeeded ? 0 : 1;
}
