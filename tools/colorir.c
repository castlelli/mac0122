/*
 * colorir - standalone filter: C source in, highlighted HTML out.
 *
 * Build (from the repository root):
 *   gcc -std=c89 -Wall -Wextra -pedantic -Werror \
 *       tools/colorir.c src/c_highlighter.c src/c_keywords.c \
 *       src/text_utils.c -o colorir
 *
 * Use:
 *   ./colorir program.c > program.html    whole page
 *   ./colorir -f program.c > piece.html   only the <pre> block
 *   ./colorir -c > style.css              only the CSS
 *   ./colorir < program.c                 read standard input
 */

#include <stdio.h>
#include <string.h>

#include "../src/c_highlighter.h"
#include "../src/text_utils.h"

static void write_page_start(FILE *html_file, const char *title)
{
    fputs("<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\">\n"
          "<title>", html_file);
    text_utils_write_html_string(html_file, title);
    fputs("</title>\n<style>\n", html_file);
    c_highlighter_write_css(html_file);
    fputs("</style></head>\n<body>\n", html_file);
}

int main(int argument_count, char *arguments[])
{
    int is_fragment = 0;
    const char *input_name = NULL;
    FILE *source_file = stdin;
    int argument_index;

    for (argument_index = 1; argument_index < argument_count;
         argument_index++) {
        if (strcmp(arguments[argument_index], "-f") == 0) {
            is_fragment = 1;
        } else if (strcmp(arguments[argument_index], "-c") == 0) {
            c_highlighter_write_css(stdout);
            return 0;
        } else {
            input_name = arguments[argument_index];
        }
    }

    if (input_name != NULL) {
        source_file = fopen(input_name, "r");
        if (source_file == NULL) {
            fprintf(stderr, "Erro: nao consegui abrir '%s'\n", input_name);
            return 1;
        }
    }

    if (!is_fragment)
        write_page_start(stdout, input_name != NULL ? input_name
                                                    : "codigo C");
    fputs("<pre class=\"codigo\">", stdout);
    c_highlighter_write_html(source_file, stdout);
    fputs("</pre>\n", stdout);
    if (!is_fragment)
        fputs("</body></html>\n", stdout);

    if (source_file != stdin)
        fclose(source_file);
    return 0;
}