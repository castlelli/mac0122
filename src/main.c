#include "parser.h"
#include "text_utils.h"
#include <stdio.h>

int main(){
    /*char input[] = "grafo, árvore, union-find";
    int count;
    char** result;
    result = text_utils_split_comma(input, &count);
    for(int i = 0; i < count; i++) printf("[%s]\n", result[i]);
    text_utils_free_str_vector(result, count);*/
    parser_read_file("input/template.txt");
    return 0;
}