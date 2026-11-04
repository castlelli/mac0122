#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "text_utils.h"

#define MAX_LINE_SIZE 256


/* text_utils_split_comma returns NULL both when every piece is empty and
   when memory runs out. Checking for content first lets the parser tell
   an empty field (ignored) from an allocation failure. */
static int has_value_between_commas(const char *value){
    const char *pointer;
    for (pointer = value; *pointer != '\0'; pointer++){
        if (*pointer != ',' && *pointer != ' ' && *pointer != '\t') return 1;
    }
    return 0;
}

/* Stores value in the field of page_header named by key.
   Unknown keys are ignored. Returns 0 only when memory runs out. */
static int fill_page_field(page *page_header, const char *key, char *value){
    if (!strcmp(key, "Tecnica")){
        page_header->technique = text_utils_copy_str(value);
        return page_header->technique != NULL;
    } else if (!strcmp(key, "Algoritmo")){
        page_header->algorithm = text_utils_copy_str(value);
        return page_header->algorithm != NULL;
    } else if (!strcmp(key, "Data")){
        page_header->date = text_utils_copy_str(value);
        return page_header->date != NULL;
    } else if (!strcmp(key, "Problema")){
        page_header->problem = text_utils_copy_str(value);
        return page_header->problem != NULL;
    } else if (!strcmp(key, "Autores")){
        if (!has_value_between_commas(value)) return 1;
        page_header->authors = text_utils_split_comma(value, &page_header->authors_count);
        return page_header->authors != NULL;
    } else if (!strcmp(key, "Conceitos")){
        if (!has_value_between_commas(value)) return 1;
        page_header->concepts = text_utils_split_comma(value, &page_header->concepts_count);
        return page_header->concepts != NULL;
    }
    return 1;
}

page* parser_read_header(const char * file_path){
    FILE* file = fopen(file_path, "r");
    page* page_header = page_create();
    char line[MAX_LINE_SIZE];
    char *key;
    char *value;
    char *colon;
    char* trimmed_line;
    int has_failed = 0;

    if (file == NULL){
        perror(file_path);
        page_free(page_header);
        return NULL;
    } else if (page_header == NULL){
        fclose(file);
        return NULL;
    }

    page_header->txt_name = text_utils_copy_str(file_path);
    if (page_header->txt_name == NULL) has_failed = 1;

    while (!has_failed && fgets(line, sizeof(line), file) != NULL)
    {
        trimmed_line = text_utils_trim(line);
        if (!strcmp(trimmed_line, "---")) break;

        colon = strchr(trimmed_line, ':');
        if (colon == NULL) continue;
        *colon = '\0';
        key = text_utils_trim(trimmed_line);
        value = text_utils_trim(colon + 1);
        if (!strcmp(value, "")) continue;

        if (!fill_page_field(page_header, key, value)) has_failed = 1;
    }

    /* Single cleanup point, so every failure frees the same things. */
    fclose(file);
    if (has_failed){
        page_free(page_header);
        return NULL;
    }
    return page_header;
}
