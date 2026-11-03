#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "text_utils.h"

char* text_utils_trim( char* line){
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

char* text_utils_copy_str(const char* str){
    size_t len = strlen(str);
    char* new_str = malloc((len+1)*sizeof(char));
    if (new_str == NULL) return NULL;
    strcpy(new_str, str);
    return new_str;
}

void text_utils_free_str_vector(char** str_vector, int count){
    for(int i = 0; i < count; i++) free(str_vector[i]);
    free(str_vector);
}

char** text_utils_split_comma(char* line, int* count){
    int comma_count = 0;
    int str_count = 0;
    char* pointer = &line[0];
    char* value;
    
    //aqui está realizando a contagem do número de vírgulas
    while (*pointer!= '\0')
    {
        if (*pointer == ','){
            comma_count ++;
        }
        pointer++;
    }
    
    //somando 1 ao número de vírgulas, temos o número de strings daquela chave
    char ** str_vector = malloc((comma_count + 1) * sizeof(char*)); //cria o vetor no qual serão armazenadas as strings
    if (str_vector == NULL) return NULL;
    pointer = &line[0];//reseta o ponteiro pro inicio da linha

    while (*pointer != '\0')
    {
        char* comma = strchr(pointer, ',');
        if (comma != NULL) { 
            *comma = '\0';
            value = text_utils_trim(pointer);
            pointer = comma + 1;
            
        } else {
            value = text_utils_trim(pointer);
            pointer+= strlen(pointer);
        }
        
        if (!strcmp(value,"")) continue;
        else {
            str_vector[str_count] = text_utils_copy_str(value);
            if (str_vector[str_count] == NULL){ //vê se a função de copiar a string falhou
                text_utils_free_str_vector(str_vector, str_count);
                *count = 0;
                return NULL;
            }
            str_count++;
        }
    }

    *count = str_count;
    if (str_count == 0) {//caso em que tava tudo em branco
        free(str_vector);
        return NULL;
    }
    return str_vector;
    
}