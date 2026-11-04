#include <stdio.h>
#include <stdlib.h>
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

/* Builds the content page name from the TXT path: the part after the last
   '/' or '\' and before the last '.', plus ".html"
   ("input/kruskal.txt" -> "kruskal.html"). Both separators are searched
   because paths written by Windows use '\'. The dot is searched only in
   the file name, so a dot in a folder name ("my.folder/kruskal") is not
   taken as the extension. Returns a malloc'd string the caller frees,
   or NULL if memory runs out. */
static char* build_html_name(const char* file_path){
    const char *name_start = file_path;
    const char *last_slash = strrchr(file_path, '/');
    const char *last_backslash = strrchr(file_path, '\\');
    const char *last_dot;
    size_t name_len;
    char *html_name;

    if (last_slash != NULL && (last_backslash == NULL || last_slash > last_backslash)){
        name_start = last_slash + 1;
    } else if (last_backslash != NULL){
        name_start = last_backslash + 1;
    }

    last_dot = strrchr(name_start, '.');
    if (last_dot != NULL) name_len = last_dot - name_start;
    else name_len = strlen(name_start);

    html_name = malloc(name_len + strlen(".html") + 1);
    if (html_name == NULL) return NULL;

    /* strncpy does not write '\0' when it stops at name_len. */
    strncpy(html_name, name_start, name_len);
    html_name[name_len] = '\0';
    strcat(html_name, ".html");
    return html_name;
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

    page_header->txt_path = text_utils_copy_str(file_path);
    if (page_header->txt_path == NULL) has_failed = 1;

    if (!has_failed){
        page_header->html_name = build_html_name(file_path);
        if (page_header->html_name == NULL) has_failed = 1;
    }

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
