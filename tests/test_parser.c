/* Tests for parser (pass 1 header and pass 2 body), using the TXT files in
   tests/fixtures/. Prints only the failing cases; exits 0 when every case
   passes. Build and run from the repository root, so the fixture paths
   resolve:
   gcc -std=c89 -Wall -Wextra -pedantic -Werror tests/test_parser.c src/parser.c src/page.c src/text_utils.c -o test_parser */
#include <stdio.h>
#include <string.h>
#include "../src/parser.h"

static int failure_count = 0;

static void check_true(const char *case_name, int condition){
    if (!condition){
        printf("FAIL %s\n", case_name);
        failure_count++;
    }
}

static int is_same_str(const char *actual, const char *expected){
    return actual != NULL && strcmp(actual, expected) == 0;
}

static void test_read_header(void){
    page *header = parser_read_header("tests/fixtures/full.txt");

    check_true("header: page returned", header != NULL);
    if (header == NULL) return;
    check_true("header: technique", is_same_str(header->technique, "Método guloso"));
    check_true("header: algorithm", is_same_str(header->algorithm, "Kruskal"));
    check_true("header: date kept as text", is_same_str(header->date, "1956"));
    check_true("header: problem", is_same_str(header->problem, "Dado um grafo"));
    check_true("header: two authors in TXT order",
               header->authors_count == 2
               && is_same_str(header->authors[0], "Joseph Kruskal")
               && is_same_str(header->authors[1], "Outro Autor"));
    check_true("header: two concepts",
               header->concepts_count == 2
               && is_same_str(header->concepts[0], "grafo")
               && is_same_str(header->concepts[1], "árvore"));
    check_true("header: txt_path is the given path",
               is_same_str(header->txt_path, "tests/fixtures/full.txt"));
    check_true("header: html_name has no folder", is_same_str(header->html_name, "full.html"));
    page_free(header);
}

static void test_read_header_empty_values(void){
    page *header = parser_read_header("tests/fixtures/empty_values.txt");

    check_true("empty values: page returned", header != NULL);
    if (header == NULL) return;
    check_true("empty values: empty Algoritmo ignored", header->algorithm == NULL);
    check_true("empty values: only-comma Autores ignored",
               header->authors == NULL && header->authors_count == 0);
    check_true("empty values: empty pieces skipped",
               header->concepts_count == 2
               && is_same_str(header->concepts[0], "a")
               && is_same_str(header->concepts[1], "b"));
    check_true("empty values: body line with ':' not read as a key",
               header->problem == NULL);
    page_free(header);
}

static void test_read_header_missing_file(void){
    /* parser_read_header reports the missing file on stderr; only the
       NULL result is checked here. */
    check_true("missing file returns NULL",
               parser_read_header("tests/fixtures/does_not_exist.txt") == NULL);
}

static void test_read_body(void){
    page *header = parser_read_header("tests/fixtures/full.txt");
    page_body *body;

    if (header == NULL){
        check_true("body: header for the body test", 0);
        return;
    }
    body = parser_read_body(header);
    check_true("body: body returned", body != NULL);
    if (body != NULL){
        /* Expected: section, paragraph, code, paragraph, code, reference.
           The empty [code] block and the [code] without [/code] add nothing,
           and the blank line adds nothing. */
        check_true("body: six blocks", body->blocks_count == 6);
        if (body->blocks_count == 6){
            check_true("body: section without '# '",
                       body->blocks[0]->block_type == SECTION
                       && is_same_str(body->blocks[0]->block_str, "Ideia"));
            check_true("body: paragraph",
                       body->blocks[1]->block_type == PARAGRAPH
                       && is_same_str(body->blocks[1]->block_str, "Texto um."));
            check_true("body: code keeps indentation and blank line",
                       body->blocks[2]->block_type == CODE
                       && is_same_str(body->blocks[2]->block_str,
                                      "int main(void)\n{\n\n    return 0;\n}"));
            check_true("body: paragraph between code blocks",
                       body->blocks[3]->block_type == PARAGRAPH
                       && is_same_str(body->blocks[3]->block_str, "Texto entre."));
            check_true("body: second code block is separate",
                       body->blocks[4]->block_type == CODE
                       && is_same_str(body->blocks[4]->block_str, "x = 1;"));
            check_true("body: reference without '[ref] '",
                       body->blocks[5]->block_type == REFERENCE
                       && is_same_str(body->blocks[5]->block_str, "Livro"));
        }
    }
    page_body_free(body);
    page_free(header);

    check_true("body: NULL page returns NULL", parser_read_body(NULL) == NULL);
}

int main(void){
    test_read_header();
    test_read_header_empty_values();
    test_read_header_missing_file();
    test_read_body();
    return failure_count == 0 ? 0 : 1;
}
