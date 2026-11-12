/* Tests for page (header struct and body). Prints only the failing cases;
   exits 0 when every case passes. Build and run from the repository root:
   gcc -std=c89 -Wall -Wextra -pedantic -Werror tests/test_page.c src/page.c src/text_utils.c -o test_page */
#include <stdio.h>
#include <string.h>
#include "../src/page.h"

static int failure_count = 0;

static void check_true(const char *case_name, int condition){
    if (!condition){
        printf("FAIL %s\n", case_name);
        failure_count++;
    }
}

static void test_page_create(void){
    page *empty_page = page_create();

    check_true("page_create returns a page", empty_page != NULL);
    if (empty_page == NULL) return;
    check_true("page_create: strings are NULL",
               empty_page->problem == NULL && empty_page->algorithm == NULL
               && empty_page->technique == NULL && empty_page->date == NULL
               && empty_page->txt_path == NULL && empty_page->html_name == NULL);
    check_true("page_create: vectors are NULL",
               empty_page->authors == NULL && empty_page->concepts == NULL);
    check_true("page_create: counts are 0",
               empty_page->authors_count == 0 && empty_page->concepts_count == 0);

    /* An empty page must be freeable, and so must NULL. */
    page_free(empty_page);
    page_free(NULL);
}

static void test_body_add_block(void){
    page_body *body = page_body_create();
    char section_text[] = "Ideia do algoritmo";

    check_true("page_body_create returns a body", body != NULL);
    if (body == NULL) return;
    check_true("page_body_create: empty", body->blocks == NULL && body->blocks_count == 0);

    check_true("add section", page_body_add_block(body, SECTION, section_text));
    check_true("add paragraph", page_body_add_block(body, PARAGRAPH, "Ordena as arestas"));
    check_true("add code", page_body_add_block(body, CODE, "int x;\n    x = 1;"));
    check_true("add reference", page_body_add_block(body, REFERENCE, "Cormen"));

    /* The body keeps its own copy of each text. */
    section_text[0] = 'X';

    check_true("four blocks", body->blocks_count == 4);
    if (body->blocks_count == 4){
        check_true("blocks keep their order and type",
                   body->blocks[0]->type == SECTION
                   && body->blocks[1]->type == PARAGRAPH
                   && body->blocks[2]->type == CODE
                   && body->blocks[3]->type == REFERENCE);
        check_true("block text is a copy",
                   strcmp(body->blocks[0]->text, "Ideia do algoritmo") == 0);
        check_true("code text keeps newlines and indentation",
                   strcmp(body->blocks[2]->text, "int x;\n    x = 1;") == 0);
    }

    page_body_free(body);
    page_body_free(NULL);
}

int main(void){
    test_page_create();
    test_body_add_block();
    return failure_count == 0 ? 0 : 1;
}
