#include <string.h>
#include <stdlib.h>
#include "text_utils.h"

char* text_utils_trim(char* line){
    char* pointer = &line[0];
    while (*pointer != '\0' && (*pointer == '\t' || *pointer == '\n' || *pointer == '\r' || *pointer == ' ')){
        pointer++;
    }
    /* The right end is the same job as text_utils_trim_end. */
    return text_utils_trim_end(pointer);
}

char* text_utils_trim_end(char* line){
    /* size_t never goes below 0, so len > 0 must be tested before len-1. */
    size_t len = strlen(line);
    while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r' || line[len-1] == '\t' || line[len-1] == ' ')){
        line[len-1] = '\0';
        len--;
    }
    return line;
}

char* text_utils_copy_str(const char* str){
    size_t len = strlen(str);
    char* new_str = malloc((len + 1) * sizeof(char));
    if (new_str == NULL) return NULL;
    strcpy(new_str, str);
    return new_str;
}

void text_utils_free_str_vector(char** str_vector, int count){
    int str_index;
    for (str_index = 0; str_index < count; str_index++) free(str_vector[str_index]);
    free(str_vector);
}

char** text_utils_split_comma(char* line, int* count){
    int comma_count = 0;
    int str_count = 0;
    char* pointer = &line[0];
    char* value;
    char** str_vector;

    /* n commas give at most n + 1 pieces, so the whole vector can be
       allocated once, before splitting. */
    while (*pointer != '\0'){
        if (*pointer == ',') comma_count++;
        pointer++;
    }

    str_vector = malloc((comma_count + 1) * sizeof(char*));
    if (str_vector == NULL){
        *count = 0;
        return NULL;
    }

    pointer = &line[0];
    while (*pointer != '\0'){
        char* comma = strchr(pointer, ',');
        if (comma != NULL){
            *comma = '\0';
            value = text_utils_trim(pointer);
            pointer = comma + 1;
        } else {
            value = text_utils_trim(pointer);
            pointer += strlen(pointer);
        }

        if (!strcmp(value, "")) continue;

        str_vector[str_count] = text_utils_copy_str(value);
        if (str_vector[str_count] == NULL){
            text_utils_free_str_vector(str_vector, str_count);
            *count = 0;
            return NULL;
        }
        str_count++;
    }

    *count = str_count;
    /* Every piece was blank: an empty field is NULL with count 0. */
    if (str_count == 0){
        free(str_vector);
        return NULL;
    }
    return str_vector;
}
