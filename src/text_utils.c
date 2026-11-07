#include <string.h>
#include <stdlib.h>
#include "text_utils.h"

char* text_utils_trim(char* line){
    char* pointer = &line[0];
    while (*pointer != '\0' && (*pointer == '\t' || *pointer == '\n' || *pointer == '\r' || *pointer == ' ')){
        pointer++;
    }
    /* The right end is the same job as text_utils_trim_end. */
    return text_utils_trim_end(pointer);
}

char* text_utils_trim_end(char* line){
    /* size_t never goes below 0, so len > 0 must be tested before len-1. */
    size_t len = strlen(line);
    while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r' || line[len-1] == '\t' || line[len-1] == ' ')){
        line[len-1] = '\0';
        len--;
    }
    return line;
}

char* text_utils_copy_str(const char* str){
    size_t len = strlen(str);
    char* new_str = malloc((len + 1) * sizeof(char));
    if (new_str == NULL) return NULL;
    strcpy(new_str, str);
    return new_str;
}

void text_utils_free_str_vector(char** str_vector, int count){
    int str_index;
    for (str_index = 0; str_index < count; str_index++) free(str_vector[str_index]);
    free(str_vector);
}

char** text_utils_split_comma(char* line, int* count){
    int comma_count = 0;
    int str_count = 0;
    char* pointer = &line[0];
    char* value;
    char** str_vector;

    /* n commas give at most n + 1 pieces, so the whole vector can be
       allocated once, before splitting. */
    while (*pointer != '\0'){
        if (*pointer == ',') comma_count++;
        pointer++;
    }

    str_vector = malloc((comma_count + 1) * sizeof(char*));
    if (str_vector == NULL){
        *count = 0;
        return NULL;
    }

    pointer = &line[0];
    while (*pointer != '\0'){
        char* comma = strchr(pointer, ',');
        if (comma != NULL){
            *comma = '\0';
            value = text_utils_trim(pointer);
            pointer = comma + 1;
        } else {
            value = text_utils_trim(pointer);
            pointer += strlen(pointer);
        }

        if (!strcmp(value, "")) continue;

        str_vector[str_count] = text_utils_copy_str(value);
        if (str_vector[str_count] == NULL){
            text_utils_free_str_vector(str_vector, str_count);
            *count = 0;
            return NULL;
        }
        str_count++;
    }

    *count = str_count;
    /* Every piece was blank: an empty field is NULL with count 0. */
    if (str_count == 0){
        free(str_vector);
        return NULL;
    }
    return str_vector;
}

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

/* One base letter per code point of each UTF-8 lead byte, in the order of the
   second byte (0x80 to 0xBF). UNMAPPED_LETTER marks a character kept as it is.
   Latin-1 Supplement and Latin Extended-A cover Portuguese and the names of
   authors from other languages. */
#define UNMAPPED_LETTER '.'
#define FIRST_LEAD_BYTE 0xC3
#define LAST_LEAD_BYTE 0xC5
#define FIRST_CONTINUATION_BYTE 0x80
#define LAST_CONTINUATION_BYTE 0xBF

static const char BASE_LETTERS[][65] = {
    "aaaaaa.ceeeeiiii.nooooo.ouuuuy..aaaaaa.ceeeeiiii.nooooo.ouuuuy.y",
    "aaaaaaccccccccddddeeeeeeeeeegggggggghhhhiiiiiiiiii..jjkk.lllllll",
    "lllnnnnnnnnnoooooo..rrrrrrssssssssttttttuuuuuuuuuuuuwwyyyzzzzzzs"
};

/* Reads a string as the bytes of its normalized form. Comparing needs one byte
   at a time and a kept two-byte letter yields two, so the second one waits in
   pending_byte. */
typedef struct normalized_reader {
    const unsigned char *cursor;
    int pending_byte;
} normalized_reader;

static void start_normalized_reader(normalized_reader *reader, const char *text)
{
    reader->cursor = (const unsigned char *)text;
    reader->pending_byte = 0;
}

/* Returns the next normalized byte, or 0 at the end of the string. */
static int read_normalized_byte(normalized_reader *reader)
{
    int lead_byte;
    int continuation_byte;
    char base_letter;

    if (reader->pending_byte != 0) {
        lead_byte = reader->pending_byte;
        reader->pending_byte = 0;
        return lead_byte;
    }

    lead_byte = *reader->cursor;
    if (lead_byte == 0)
        return 0;
    reader->cursor++;

    /* Explicit ranges, not tolower: the locale must not change index keys. */
    if (lead_byte >= 'A' && lead_byte <= 'Z')
        return lead_byte - 'A' + 'a';

    continuation_byte = *reader->cursor;
    if (lead_byte < FIRST_LEAD_BYTE || lead_byte > LAST_LEAD_BYTE
        || continuation_byte < FIRST_CONTINUATION_BYTE
        || continuation_byte > LAST_CONTINUATION_BYTE)
        return lead_byte;
    reader->cursor++;

    base_letter = BASE_LETTERS[lead_byte - FIRST_LEAD_BYTE]
                              [continuation_byte - FIRST_CONTINUATION_BYTE];
    if (base_letter != UNMAPPED_LETTER)
        return base_letter;

    /* In Latin-1 Supplement a kept capital (U+00C0 to U+00DE, except the
       multiplication sign) is 0x20 below its lowercase form. */
    if (lead_byte == FIRST_LEAD_BYTE && continuation_byte <= 0x9E
        && continuation_byte != 0x97)
        continuation_byte += 0x20;
    reader->pending_byte = continuation_byte;
    return lead_byte;
}

char *text_utils_normalize(const char *text)
{
    /* Normalizing never makes a string longer. */
    char *normalized_text = malloc(strlen(text) + 1);
    normalized_reader reader;
    size_t position = 0;
    int next_byte;

    if (normalized_text == NULL)
        return NULL;

    start_normalized_reader(&reader, text);
    while ((next_byte = read_normalized_byte(&reader)) != 0)
        normalized_text[position++] = (char)next_byte;
    normalized_text[position] = '\0';
    return normalized_text;
}

int text_utils_compare_normalized(const char *first, const char *second)
{
    normalized_reader first_reader;
    normalized_reader second_reader;
    int first_byte;
    int second_byte;

    start_normalized_reader(&first_reader, first);
    start_normalized_reader(&second_reader, second);
    do {
        first_byte = read_normalized_byte(&first_reader);
        second_byte = read_normalized_byte(&second_reader);
    } while (first_byte == second_byte && first_byte != 0);
    return first_byte - second_byte;
}

