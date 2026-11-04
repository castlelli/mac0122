#include <stdlib.h>
#include "page.h"
#include "text_utils.h"


page* page_create(void){
    page* single_page = malloc(sizeof(page));
    if (single_page == NULL) return NULL;

    single_page->algorithm = NULL;
    single_page->technique = NULL;
    single_page->txt_path = NULL;
    single_page->html_name = NULL;
    single_page->authors = NULL;
    single_page->date = NULL;
    single_page->problem = NULL;
    single_page->concepts = NULL;
    single_page->authors_count = 0;
    single_page->concepts_count = 0;

    return single_page;
}

void page_free(page* single_page){
    if (single_page == NULL) return;
    text_utils_free_str_vector(single_page->authors, single_page->authors_count);
    text_utils_free_str_vector(single_page->concepts, single_page->concepts_count);
    free(single_page->html_name);
    free(single_page->txt_path);
    free(single_page->technique);
    free(single_page->date);
    free(single_page->problem);
    free(single_page->algorithm);
    free(single_page);
}

page_body* page_body_create(void){
    page_body* body = malloc(sizeof(page_body));
    if (body == NULL) return NULL;

    body->blocks = NULL;
    body->blocks_count = 0;

    return body;
}

void page_body_free(page_body* body){
    int block_index;
    if (body == NULL) return;
    for (block_index = 0; block_index < body->blocks_count; block_index++){
        free(body->blocks[block_index]->text);
        free(body->blocks[block_index]);
    }
    free(body->blocks);
    free(body);
}

int page_body_add_block(page_body* body, enum block_type type, const char* text){
    page_block* new_block;
    page_block** grown_blocks;

    new_block = malloc(sizeof(page_block));
    if (new_block == NULL) return 0;

    new_block->type = type;
    new_block->text = text_utils_copy_str(text);
    if (new_block->text == NULL){
        free(new_block);
        return 0;
    }

    /* realloc goes to a separate pointer: on failure it returns NULL but
       the old vector is still valid, and assigning NULL straight to
       body->blocks would lose it (and every block in it). */
    grown_blocks = realloc(body->blocks, (body->blocks_count + 1) * sizeof(page_block*));
    if (grown_blocks == NULL){
        free(new_block->text);
        free(new_block);
        return 0;
    }

    body->blocks = grown_blocks;
    body->blocks[body->blocks_count] = new_block;
    body->blocks_count++;
    return 1;
}