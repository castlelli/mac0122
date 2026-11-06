#include "../src/text_utils.h"

#include <stdio.h>
#include <string.h>

#define MAX_OUTPUT_LEN 256

static int failure_count = 0;

static void check_output(FILE *output_file, const char *expected_output)
{
    char output[MAX_OUTPUT_LEN];
    size_t output_len;

    rewind(output_file);
    output_len = fread(output, 1, MAX_OUTPUT_LEN - 1, output_file);
    output[output_len] = '\0';
    fclose(output_file);
    if (strcmp(output, expected_output) != 0) {
        printf("expected: %s\nobtained: %s\n\n", expected_output, output);
        failure_count++;
    }
}

static void check_string_escape(const char *text, const char *expected_output)
{
    FILE *output_file = tmpfile();

    if (output_file == NULL) {
        printf("could not open a temporary file\n");
        failure_count++;
        return;
    }
    text_utils_write_html_string(output_file, text);
    check_output(output_file, expected_output);
}

static void check_character_escape(int character, const char *expected_output)
{
    FILE *output_file = tmpfile();

    if (output_file == NULL) {
        printf("could not open a temporary file\n");
        failure_count++;
        return;
    }
    text_utils_write_html_character(output_file, character);
    check_output(output_file, expected_output);
}

int main(void)
{
    check_character_escape('<', "&lt;");
    check_character_escape('>', "&gt;");
    check_character_escape('&', "&amp;");
    check_character_escape('a', "a");
    check_character_escape('"', "\"");

    check_string_escape("", "");
    check_string_escape("plain", "plain");
    check_string_escape("a < b && c > d", "a &lt; b &amp;&amp; c &gt; d");
    check_string_escape("&lt;", "&amp;lt;");
    /* accents reach the page byte by byte, whatever the encoding */
    check_string_escape("a\xC3\xA7\xC3\xA3o", "a\xC3\xA7\xC3\xA3o");

    return failure_count == 0 ? 0 : 1;
}
