#ifndef PAGE_H
#define PAGE_H

enum block_type {SECTION, PARAGRAPH, CODE, REFERENCE};


typedef struct page
{
    char* problem;
    char* algorithm;
    char* technique;
    char* date;
    char** authors;
    int authors_count;
    char** concepts;
    int concepts_count;
    char* txt_name;
    char* html_name;

} page;
/* Allocates an empty page: every string and string vector is NULL and
   every count is 0. Returns NULL if the allocation fails.
   The fields are filled later by the parser. Every string and vector
   stored in a field must be allocated with malloc; from then on the page
   owns it, and whoever filled the field must not free it. */
page* page_create(void);

/* Frees every field of page and then page itself.
   Does nothing if page is NULL. */
void page_free(page*);


#endif 