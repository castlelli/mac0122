#include <stdlib.h>
#include "page.h"
#include "text_utils.h"


page* page_create(void){
    page* single_page = malloc(sizeof(page));
    if (single_page == NULL) return NULL;

    single_page->algorithm = NULL;
    single_page->technique = NULL;
    single_page->txt_name = NULL;
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
    free(single_page->txt_name);
    free(single_page->technique);
    free(single_page->date);
    free(single_page->problem);
    free(single_page->algorithm);
    free(single_page);    
}