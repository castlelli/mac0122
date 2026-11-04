/* Tests for text_utils. Prints only the failing cases; exits 0 when every
   case passes. Build and run from the repository root:
   gcc -std=c89 -Wall -Wextra -pedantic -Werror tests/test_text_utils.c src/text_utils.c -o test_text_utils */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../src/text_utils.h"

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

int main(void){
    test_trim();
    test_trim_end();
    test_copy_str();
    test_split_comma();
    return failure_count == 0 ? 0 : 1;
}
