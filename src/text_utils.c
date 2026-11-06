#include "text_utils.h"

void text_utils_write_html_character(FILE *html_file, int character)
{
    if (character == '<')
        fputs("&lt;", html_file);
    else if (character == '>')
        fputs("&gt;", html_file);
    else if (character == '&')
        fputs("&amp;", html_file);
    else
        fputc(character, html_file);
}

void text_utils_write_html_string(FILE *html_file, const char *text)
{
    size_t position;

    for (position = 0; text[position] != '\0'; position++)
        text_utils_write_html_character(html_file,
                                        (unsigned char)text[position]);
}
