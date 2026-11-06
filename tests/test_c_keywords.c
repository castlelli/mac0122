#include "../src/c_keywords.h"

#include <stdio.h>
#include <string.h>

static int failure_count = 0;

static void check_kind(const char *word, c_keyword_kind expected_kind)
{
    if (c_keywords_lookup(word, strlen(word)) != expected_kind) {
        printf("wrong kind for \"%s\"\n", word);
        failure_count++;
    }
}

/* The word need not be NUL-terminated: the highlighter's buffer is not. */
static void check_prefix_lookup(void)
{
    if (c_keywords_lookup("intfoo", 3) != C_KEYWORD_TYPE) {
        printf("the first 3 characters of \"intfoo\" should be a type\n");
        failure_count++;
    }
}

int main(void)
{
    /* The binary search silently misses words if this order breaks. */
    if (!c_keywords_is_table_sorted()) {
        printf("the keyword table is not sorted\n");
        failure_count++;
    }

    /* The first and last entries, where a bad search bound would show. */
    check_kind("FILE", C_KEYWORD_TYPE);
    check_kind("while", C_KEYWORD_COMMAND);

    check_kind("int", C_KEYWORD_TYPE);
    check_kind("size_t", C_KEYWORD_TYPE);
    check_kind("uint8_t", C_KEYWORD_TYPE);
    check_kind("if", C_KEYWORD_COMMAND);
    check_kind("struct", C_KEYWORD_COMMAND);
    check_kind("return", C_KEYWORD_COMMAND);
    check_kind("NULL", C_KEYWORD_BUILTIN);
    check_kind("true", C_KEYWORD_BUILTIN);
    check_kind("false", C_KEYWORD_BUILTIN);

    check_kind("byte", C_KEYWORD_NONE);
    check_kind("integer", C_KEYWORD_NONE);
    check_kind("in", C_KEYWORD_NONE);
    check_kind("Int", C_KEYWORD_NONE);
    check_kind("", C_KEYWORD_NONE);
    check_kind("AAA", C_KEYWORD_NONE);
    check_kind("zzz", C_KEYWORD_NONE);

    check_prefix_lookup();

    return failure_count == 0 ? 0 : 1;
}
