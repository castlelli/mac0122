#ifndef PAGE_H
#define PAGE_H
#include <stdbool.h>

enum block_type {SECTION, PARAGRAPH, CODE, REFERENCE};


typedef struct page
{
    char* problem;
    char* algorithm;
    char* technique;
    int date;
    bool has_date;
    char** authors;
    int authors_count;
    char** concepts;
    int concepts_count;

} page;

page* create_page(char*);
void free_page(page*);


#endif 