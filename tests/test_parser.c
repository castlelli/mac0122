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
    /* The warning it writes on stderr is checked in check_warnings. */
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
                       body->blocks[0]->type == SECTION
                       && is_same_str(body->blocks[0]->text, "Ideia"));
            check_true("body: paragraph",
                       body->blocks[1]->type == PARAGRAPH
                       && is_same_str(body->blocks[1]->text, "Texto um."));
            check_true("body: code keeps indentation and blank line",
                       body->blocks[2]->type == CODE
                       && is_same_str(body->blocks[2]->text,
                                      "int main(void)\n{\n\n    return 0;\n}"));
            check_true("body: paragraph between code blocks",
                       body->blocks[3]->type == PARAGRAPH
                       && is_same_str(body->blocks[3]->text, "Texto entre."));
            check_true("body: second code block is separate",
                       body->blocks[4]->type == CODE
                       && is_same_str(body->blocks[4]->text, "x = 1;"));
            check_true("body: reference without '[ref] '",
                       body->blocks[5]->type == REFERENCE
                       && is_same_str(body->blocks[5]->text, "Livro"));
        }
    }
    page_body_free(body);
    page_free(header);

    check_true("body: NULL page returns NULL", parser_read_body(NULL) == NULL);
}

/* True when text is exactly count copies of letter. */
static int is_repeated_letter(const char *text, char letter, size_t count){
    size_t position;
    if (text == NULL || strlen(text) != count) return 0;
    for (position = 0; position < count; position++){
        if (text[position] != letter) return 0;
    }
    return 1;
}

static void test_long_lines(void){
    page *header = parser_read_header("tests/fixtures/long_lines.txt");
    page_body *body;

    check_true("long lines: page returned", header != NULL);
    if (header == NULL) return;
    check_true("long lines: 1000-character Problema kept whole",
               is_repeated_letter(header->problem, 'a', 1000));

    body = parser_read_body(header);
    check_true("long lines: body returned", body != NULL);
    if (body != NULL){
        /* The 5000-character line is cut to 4095 and its rest is dropped,
           so it is one block and "Depois." is still the next one. */
        check_true("long lines: four blocks", body->blocks_count == 4);
        if (body->blocks_count == 4){
            check_true("long lines: 1000-character paragraph kept whole",
                       is_repeated_letter(body->blocks[0]->text, 'b', 1000));
            check_true("long lines: line filling the buffer exactly kept whole",
                       is_repeated_letter(body->blocks[1]->text, 'c', 4095));
            check_true("long lines: 5000-character line cut to 4095",
                       is_repeated_letter(body->blocks[2]->text, 'd', 4095));
            check_true("long lines: line after the cut one is intact",
                       is_same_str(body->blocks[3]->text, "Depois."));
        }
    }
    page_body_free(body);
    page_free(header);
}

/* Some cases make the parser warn on stderr on purpose (missing file, line
   cut). AGENTS.md asks a passing test to print nothing, so stderr goes to
   this file while the tests run; the warnings are then checked as part of
   the expected behavior, and the file is removed. */
#define WARNINGS_PATH "test_parser_warnings.tmp"
#define MAX_WARNINGS_SIZE 4096

static void check_warnings(void){
    char warnings[MAX_WARNINGS_SIZE];
    size_t warnings_len;
    FILE *warnings_file = fopen(WARNINGS_PATH, "r");

    if (warnings_file == NULL){
        check_true("warnings: file with the redirected stderr can be read", 0);
        return;
    }
    warnings_len = fread(warnings, 1, sizeof(warnings) - 1, warnings_file);
    warnings[warnings_len] = '\0';
    fclose(warnings_file);

    check_true("warnings: missing file is reported",
               strstr(warnings, "tests/fixtures/does_not_exist.txt") != NULL);
    check_true("warnings: cut line is reported with its file",
               strstr(warnings, "tests/fixtures/long_lines.txt: line longer than 4095 characters, cut") != NULL);
}

int main(void){
    if (freopen(WARNINGS_PATH, "w", stderr) == NULL){
        printf("FAIL could not redirect stderr to %s\n", WARNINGS_PATH);
        return 1;
    }

    test_read_header();
    test_read_header_empty_values();
    test_read_header_missing_file();
    test_read_body();
    test_long_lines();

    /* Closed before reading it back, so every warning is on disk. */
    fclose(stderr);
    check_warnings();
    remove(WARNINGS_PATH);

    return failure_count == 0 ? 0 : 1;
}
