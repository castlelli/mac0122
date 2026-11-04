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


/* Adds one body line to page_txt_body as a block. The marker at the start
   of the line picks the type and is left out of the text: "# " is a
   section, "[ref]" a reference, anything else a paragraph. Blank lines
   add nothing. Returns 0 only when memory runs out. */
static int add_line_to_body(page_body *page_txt_body, const char *line){
    const char *reference_text;

    if (!strcmp(line, "")) return 1;

    if (!strncmp(line, "# ", 2)){
        return page_body_add_block(page_txt_body, SECTION, line + 2);
    } else if (!strncmp(line, "[ref]", 5)){
        reference_text = line + 5;
        while (*reference_text == ' ') reference_text++;
        return page_body_add_block(page_txt_body, REFERENCE, reference_text);
    }
    return page_body_add_block(page_txt_body, PARAGRAPH, line);
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

static int append_code_line(char** code_text, const char* line){
    int is_first_line = (*code_text == NULL);
    int new_len;
    char* code_text_temp;
    if (is_first_line) new_len = strlen(line) + 1;
    else new_len = strlen(*code_text) + 1 + strlen(line) + 1;

    code_text_temp = realloc(*code_text, new_len);
    if (code_text_temp == NULL) return 0;

    if(is_first_line){
        strcpy(code_text_temp, line);
    } else {
        strcat(code_text_temp, "\n");
        strcat(code_text_temp, line);
    }

    *code_text = code_text_temp;
    return 1;
}

page_body* parser_read_body(const page* page_header){
    FILE* file;
    page_body* page_txt_body;
    char line[MAX_LINE_SIZE];
    char* trimmed_line;
    char* code_print = NULL;
    int has_failed = 0;
    int is_inside_code = 0;

    /* Checked before fopen, which reads page_header->txt_path. */
    if (page_header == NULL || page_header->txt_path == NULL) return NULL;

    file = fopen(page_header->txt_path, "r");
    if (file == NULL){
        perror(page_header->txt_path);
        return NULL;
    }

    page_txt_body = page_body_create();
    if (page_txt_body == NULL){
        fclose(file);
        return NULL;
    }

    while (fgets(line, sizeof(line), file) != NULL)
    {
        trimmed_line = text_utils_trim(line);
        if (!strcmp(trimmed_line, "---")) break;
        
    }

    /* Body lines lose only their end ("\r\n"): inside [code] the leading
       spaces are the indentation of the C code. */
    while (!has_failed && fgets(line, sizeof(line), file) != NULL)
    {
        text_utils_trim_end(line);
        if (!strcmp(line,"[code]") && (is_inside_code == 0)){
            is_inside_code = 1;
            continue;
        }
        
        if (!strcmp(line,"[/code]") && (is_inside_code == 1)){
            is_inside_code = 0;
            /* An empty [code] block recorded nothing, so code_print is still
               NULL: no block is added for it. */
            if (code_print != NULL && !page_body_add_block(page_txt_body, CODE, code_print)) has_failed = 1;
            free(code_print);
            code_print = NULL;
            continue;
        }

        if (is_inside_code){
            if(!append_code_line(&code_print, line)) has_failed = 1;
            continue;
            
        }

        if (!add_line_to_body(page_txt_body, line)) has_failed = 1;
    }
    free(code_print);

    fclose(file);
    if (has_failed){
        page_body_free(page_txt_body);
        return NULL;
    }
    return page_txt_body;
}

