#include <stdio.h>
#include <string.h>
#include "parser.h"

#define MAX_LINE_SIZE 256

char* parser_trim( char* line){
    char* pointer = &line[0];
    while (*pointer != '\0' && (*pointer == '\t' || *pointer == '\n' || *pointer == '\r' || *pointer == ' ')){
        pointer++;
    }
    
    int len = strlen(pointer);
    while (len > 0 && (pointer[len-1] == '\n' || pointer[len-1] =='\r' || pointer[len-1] == '\t' || pointer[len-1] == ' '))
    { 
        pointer[len-1] = '\0';
        len--;
    }
    return pointer;
}

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
        trimmed_line = parser_trim(line);
        if (!strcmp(trimmed_line, "---")) break;

        else{
            char* colon =strchr(trimmed_line, ':');
            if (colon != NULL) *colon = '\0';
            else continue;
            key = parser_trim(trimmed_line);
            value = parser_trim(colon + 1);
            printf("[%s] = [%s]\n", key, value);
        }
    }
    fclose(file);
    return 1;
    
}