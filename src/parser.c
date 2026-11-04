#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "text_utils.h"

#define MAX_LINE_SIZE 256



page* parser_read_header(const char * file_path){
    FILE* file = fopen(file_path, "r");
    page* page_header = page_create();
    char line[MAX_LINE_SIZE];
    char *key; /*oq chamamos de indice*/
    char *value; /*oq o indice recebe*/
    char* trimmed_line;
    

    if (file == NULL){
        perror(file_path);
        page_free(page_header);
        return NULL;
    } else if (page_header == NULL){
        fclose(file);
        return NULL;
    }
    
    page_header->txt_name = text_utils_copy_str(file_path);
    if (page_header->txt_name == NULL){
        page_free(page_header);
        fclose(file);
        return NULL;
    }

    
    while (fgets(line, sizeof(line), file) != NULL)
    {   
        trimmed_line = text_utils_trim(line);
        if (!strcmp(trimmed_line, "---")) break;

        else{
            char* colon =strchr(trimmed_line, ':');
            if (colon != NULL) *colon = '\0';
            else continue;
            key = text_utils_trim(trimmed_line);
            value = text_utils_trim(colon + 1);
            if (!strcmp(value,"")) continue;
            printf("[%s] = [%s]\n", key, value);
        }
    }
    fclose(file);
    return page_header;
    
}