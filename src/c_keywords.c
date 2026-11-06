#include <string.h>

#include "c_keywords.h"

typedef struct {
    const char *word;
    c_keyword_kind kind;
} c_keyword_entry;

/* Keep sorted in strcmp order: uppercase, then '_', then lowercase. */
static const c_keyword_entry KEYWORD_TABLE[] = {
    { "FILE",      C_KEYWORD_TYPE },
    { "NULL",      C_KEYWORD_BUILTIN },
    { "_Bool",     C_KEYWORD_TYPE },
    { "auto",      C_KEYWORD_COMMAND },
    { "bool",      C_KEYWORD_TYPE },
    { "break",     C_KEYWORD_COMMAND },
    { "case",      C_KEYWORD_COMMAND },
    { "char",      C_KEYWORD_TYPE },
    { "const",     C_KEYWORD_COMMAND },
    { "continue",  C_KEYWORD_COMMAND },
    { "default",   C_KEYWORD_COMMAND },
    { "do",        C_KEYWORD_COMMAND },
    { "double",    C_KEYWORD_TYPE },
    { "else",      C_KEYWORD_COMMAND },
    { "enum",      C_KEYWORD_COMMAND },
    { "extern",    C_KEYWORD_COMMAND },
    { "false",     C_KEYWORD_BUILTIN },
    { "float",     C_KEYWORD_TYPE },
    { "for",       C_KEYWORD_COMMAND },
    { "goto",      C_KEYWORD_COMMAND },
    { "if",        C_KEYWORD_COMMAND },
    { "inline",    C_KEYWORD_COMMAND },
    { "int",       C_KEYWORD_TYPE },
    { "int16_t",   C_KEYWORD_TYPE },
    { "int32_t",   C_KEYWORD_TYPE },
    { "int64_t",   C_KEYWORD_TYPE },
    { "int8_t",    C_KEYWORD_TYPE },
    { "intmax_t",  C_KEYWORD_TYPE },
    { "intptr_t",  C_KEYWORD_TYPE },
    { "long",      C_KEYWORD_TYPE },
    { "ptrdiff_t", C_KEYWORD_TYPE },
    { "register",  C_KEYWORD_COMMAND },
    { "restrict",  C_KEYWORD_COMMAND },
    { "return",    C_KEYWORD_COMMAND },
    { "short",     C_KEYWORD_TYPE },
    { "signed",    C_KEYWORD_TYPE },
    { "size_t",    C_KEYWORD_TYPE },
    { "sizeof",    C_KEYWORD_COMMAND },
    { "ssize_t",   C_KEYWORD_TYPE },
    { "static",    C_KEYWORD_COMMAND },
    { "struct",    C_KEYWORD_COMMAND },
    { "switch",    C_KEYWORD_COMMAND },
    { "true",      C_KEYWORD_BUILTIN },
    { "typedef",   C_KEYWORD_COMMAND },
    { "uint16_t",  C_KEYWORD_TYPE },
    { "uint32_t",  C_KEYWORD_TYPE },
    { "uint64_t",  C_KEYWORD_TYPE },
    { "uint8_t",   C_KEYWORD_TYPE },
    { "uintmax_t", C_KEYWORD_TYPE },
    { "uintptr_t", C_KEYWORD_TYPE },
    { "union",     C_KEYWORD_COMMAND },
    { "unsigned",  C_KEYWORD_TYPE },
    { "va_list",   C_KEYWORD_TYPE },
    { "void",      C_KEYWORD_TYPE },
    { "volatile",  C_KEYWORD_COMMAND },
    { "wchar_t",   C_KEYWORD_TYPE },
    { "while",     C_KEYWORD_COMMAND }
};

#define KEYWORD_COUNT (sizeof(KEYWORD_TABLE) / sizeof(KEYWORD_TABLE[0]))

/* Orders word[0..word_length) against a NUL-terminated keyword. */
static int compare_word_with_keyword(const char *word, size_t word_length,
                                     const char *keyword)
{
    int result = strncmp(word, keyword, word_length);

    if (result != 0)
        return result;
    /* word is a prefix of keyword: equal only if keyword ends here */
    return keyword[word_length] == '\0' ? 0 : -1;
}

c_keyword_kind c_keywords_lookup(const char *word, size_t word_length)
{
    size_t low = 0;
    size_t high = KEYWORD_COUNT;
    size_t middle;
    int result;

    while (low < high) {
        middle = low + (high - low) / 2;
        result = compare_word_with_keyword(word, word_length,
                                           KEYWORD_TABLE[middle].word);
        if (result == 0)
            return KEYWORD_TABLE[middle].kind;
        if (result < 0)
            high = middle;
        else
            low = middle + 1;
    }
    return C_KEYWORD_NONE;
}

int c_keywords_is_table_sorted(void)
{
    size_t position;

    for (position = 1; position < KEYWORD_COUNT; position++)
        if (strcmp(KEYWORD_TABLE[position - 1].word,
                   KEYWORD_TABLE[position].word) >= 0)
            return 0;
    return 1;
}
