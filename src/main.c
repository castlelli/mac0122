#include <stddef.h>
#include "parser.h"

int main(void){
    page* header;
    page_body* body;

    header = parser_read_header("input/template.txt");
    if (header == NULL) return 1;

    body = parser_read_body(header);
    if (body == NULL){
        page_free(header);
        return 1;
    }

    page_body_free(body);
    page_free(header);
    return 0;
}
