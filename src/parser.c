#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "text_utils.h"

#define MAX_LINE_SIZE 256



int parser_read_file(const char * file_path){
    FILE* file = fopen(file_path, "r");

    if (file == NULL){
        perror(file_path);
        return -1;
    }

    char line[MAX_LINE_SIZE];
    char *key;
    char *value;
    char* trimmed_line;
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
    return 1;
    
}