#include "parser.h"
#include "text_utils.h"
#include <stdio.h>

int main(){
    page* header;
    /*char input[] = "grafo, árvore, union-find";
    int count;
    char** result;
    result = text_utils_split_comma(input, &count);
    for(int i = 0; i < count; i++) printf("[%s]\n", result[i]);
    text_utils_free_str_vector(result, count);*/
    header = parser_read_header("input/template.txt");
    if (header == NULL) return 1;
    page_free(header);
    return 0;
}