#ifndef PAGE_H
#define PAGE_H

enum block_type {SECTION, PARAGRAPH, CODE, REFERENCE};

typedef struct page_block {
    enum block_type type;
    char* text;
} page_block;

typedef struct page_body {
    page_block** blocks;
    int blocks_count;
} page_body;

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
    char* txt_path;
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

/* Allocates an empty body: no blocks yet. Returns NULL if the allocation
   fails. The body owns every block stored in it, and each block owns its
   text; both must be allocated with malloc. */
page_body* page_body_create(void);

/* Frees the text of every block, every block, the block vector and then
   body itself. Does nothing if body is NULL. */
void page_body_free(page_body*);

/* Appends a new block of the given type to the end of body, with a copy
   of text (the caller keeps its own text). Returns 1 on success and 0 if
   memory runs out; on failure body is left as it was. */
int page_body_add_block(page_body* body, enum block_type type, const char* text);


#endif 
