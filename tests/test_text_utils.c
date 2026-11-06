/* Tests for text_utils. Prints only the failing cases; exits 0 when every
   case passes. Build and run from the repository root:
   gcc -std=c89 -Wall -Wextra -pedantic -Werror tests/test_text_utils.c src/text_utils.c -o test_text_utils */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../src/text_utils.h"

#define MAX_OUTPUT_LEN 256

static int failure_count = 0;

static void check_str(const char *case_name, const char *actual, const char *expected){
    if (actual == NULL || strcmp(actual, expected) != 0){
        printf("FAIL %s: got [%s], expected [%s]\n", case_name, actual ? actual : "(null)", expected);
        failure_count++;
    }
}

static void check_int(const char *case_name, int actual, int expected){
    if (actual != expected){
        printf("FAIL %s: got %d, expected %d\n", case_name, actual, expected);
        failure_count++;
    }
}

static void test_trim(void){
    char header_line[] = "  Tecnica: X \r\n";
    char blank_line[] = " \t\r\n";
    char empty_line[] = "";
    char separator_line[] = "---\r\n";

    check_str("trim both ends", text_utils_trim(header_line), "Tecnica: X");
    check_str("trim blank line", text_utils_trim(blank_line), "");
    check_str("trim empty line", text_utils_trim(empty_line), "");
    check_str("trim separator with CRLF", text_utils_trim(separator_line), "---");
}

static void test_trim_end(void){
    char code_line[] = "    return 0;\r\n";
    char spaced_line[] = "  x  ";
    char empty_line[] = "";

    check_str("trim_end keeps indentation", text_utils_trim_end(code_line), "    return 0;");
    check_str("trim_end keeps the start", text_utils_trim_end(spaced_line), "  x");
    check_str("trim_end empty line", text_utils_trim_end(empty_line), "");
}

static void test_copy_str(void){
    char source[] = "grafo";
    char *copy = text_utils_copy_str(source);

    source[0] = 'X';
    check_str("copy_str is independent of the source", copy, "grafo");
    free(copy);
}

static void test_split_comma(void){
    char three_values[] = "grafo, árvore , union-find";
    char with_empty_pieces[] = "a, , b,";
    char only_commas[] = " , ,";
    char **pieces;
    int count;

    pieces = text_utils_split_comma(three_values, &count);
    check_int("split three values: count", count, 3);
    if (pieces != NULL && count == 3){
        check_str("split three values: first", pieces[0], "grafo");
        check_str("split three values: trimmed", pieces[1], "árvore");
        check_str("split three values: last", pieces[2], "union-find");
    }
    text_utils_free_str_vector(pieces, count);

    pieces = text_utils_split_comma(with_empty_pieces, &count);
    check_int("split skips empty pieces: count", count, 2);
    if (pieces != NULL && count == 2){
        check_str("split skips empty pieces: first", pieces[0], "a");
        check_str("split skips empty pieces: second", pieces[1], "b");
    }
    text_utils_free_str_vector(pieces, count);

    pieces = text_utils_split_comma(only_commas, &count);
    check_int("split only commas: count", count, 0);
    check_int("split only commas: NULL vector", pieces == NULL, 1);

    /* Must not crash. */
    text_utils_free_str_vector(NULL, 0);
}

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

static void test_html_escape(void)
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
}

int main(void){
    test_trim();
    test_trim_end();
    test_copy_str();
    test_split_comma();
    test_html_escape();
    return failure_count == 0 ? 0 : 1;
}
