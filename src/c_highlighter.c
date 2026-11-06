/*
 * Streaming finite state machine. It reads one character at a time and
 * keeps only the current state, a few flags and one small buffer that holds
 * the current word (plus the blanks after it, see WORD_GAP).
 *
 * Output is written lazily: a <span class="l"> line wrapper and a style
 * <span> are opened only when a character is written, and a style span is
 * closed and reopened at every newline. Adjacent characters with the same
 * style therefore share one span.
 */

#include <stdio.h>
#include <string.h>

#include "c_highlighter.h"
#include "c_keywords.h"
#include "text_utils.h"

/* The order must match STYLE_CLASS_NAMES. */
typedef enum {
    STYLE_NONE,
    STYLE_PREPROCESSOR,
    STYLE_FILE_NAME,
    STYLE_COMMENT,
    STYLE_TYPE,
    STYLE_KEYWORD,
    STYLE_BUILTIN,
    STYLE_FUNCTION,
    STYLE_STRUCT_NAME,
    STYLE_LABEL,
    STYLE_STRING,
    STYLE_ESCAPE,
    STYLE_NUMBER,
    STYLE_OPERATOR
} highlight_style;

static const char *const STYLE_CLASS_NAMES[] = {
    "", "pragma", "arquivo", "coment", "tipo", "comando", "builtin",
    "funcao", "estrutura", "rotulo", "string", "escape", "numero", "operador"
};

typedef enum {
    LINE_START,
    NORMAL,
    WORD,
    WORD_GAP,
    LONG_WORD,
    NUMBER,
    NUMBER_EXPONENT,
    AFTER_DOT,
    AFTER_SLASH,
    LINE_COMMENT,
    LINE_COMMENT_AFTER_BACKSLASH,
    BLOCK_COMMENT,
    BLOCK_COMMENT_AFTER_STAR,
    STRING,
    STRING_AFTER_BACKSLASH,
    ESCAPE_DIGITS,
    PREPROCESSOR_NAME,
    PREPROCESSOR,
    PREPROCESSOR_AFTER_BACKSLASH,
    PREPROCESSOR_STRING,
    PREPROCESSOR_STRING_AFTER_BACKSLASH,
    INCLUDE_BLANKS,
    INCLUDE_FILE
} highlighter_state;

/* What the last significant token was; used to tell a function definition
   from a call, a label from a ternary, and to find the name in
   "struct NAME {". */
typedef enum {
    TOKEN_OTHER,
    TOKEN_STATEMENT_BOUNDARY,   /* input start, directive, ; { } ) : */
    TOKEN_TYPE_LIKE,        /* a type keyword or an ordinary identifier */
    TOKEN_STRUCT_KEYWORD    /* struct or union */
} previous_token_kind;

/* Longer identifiers cannot be keywords; they are written without lookahead. */
#define WORD_BUFFER_SIZE 256
#define UNLIMITED_DIGITS (-1)

typedef struct {
    FILE *html_file;
    highlighter_state state;
    highlighter_state state_after_comment;  /* NORMAL or PREPROCESSOR */
    highlight_style open_style;             /* style span now open in output */
    int is_line_open;
    int brace_depth;
    previous_token_kind previous_token;
    int is_in_case_label;       /* between "case" and its colon */
    int delimiter;              /* closing '"' or '\'' (or '>' in #include) */
    int escape_digits_left;
    int is_escape_octal;        /* else the escape takes hex digits */
    char buffer[WORD_BUFFER_SIZE];
    size_t buffer_length;       /* word plus any blanks after it */
    size_t word_length;         /* length of the word alone */
    int does_word_start_line;
    int does_gap_have_newline;
} highlighter;

/* ---------------------------------------------------------------- output */

static void close_span(highlighter *machine)
{
    if (machine->open_style != STYLE_NONE) {
        fputs("</span>", machine->html_file);
        machine->open_style = STYLE_NONE;
    }
}

static void open_line_if_needed(highlighter *machine)
{
    if (!machine->is_line_open) {
        fputs("<span class=\"l\">", machine->html_file);
        machine->is_line_open = 1;
    }
}

/* Writes one character with the given style. A newline closes the style
   span and the line; an empty line still gets its wrapper, so it is
   numbered. */
static void emit(highlighter *machine, int character, highlight_style style)
{
    if (character == '\n') {
        close_span(machine);
        open_line_if_needed(machine);
        fputs("</span>\n", machine->html_file);
        machine->is_line_open = 0;
        return;
    }
    open_line_if_needed(machine);
    if (style != machine->open_style) {
        close_span(machine);
        if (style != STYLE_NONE) {
            fprintf(machine->html_file, "<span class=\"%s\">",
                    STYLE_CLASS_NAMES[style]);
            machine->open_style = style;
        }
    }
    text_utils_write_html_character(machine->html_file, character);
}

static void emit_buffer_range(highlighter *machine, size_t first,
                              size_t last, highlight_style style)
{
    size_t position;

    for (position = first; position < last; position++)
        emit(machine, (unsigned char)machine->buffer[position], style);
}

static void finish_output(highlighter *machine)
{
    close_span(machine);
    if (machine->is_line_open) {
        fputs("</span>", machine->html_file);
        machine->is_line_open = 0;
    }
}

/* ------------------------------------------------------ character classes */

static int is_blank(int character)
{
    return character == ' ' || character == '\t' || character == '\f' ||
           character == '\v';
}

static int is_digit(int character)
{
    return character >= '0' && character <= '9';
}

static int is_octal_digit(int character)
{
    return character >= '0' && character <= '7';
}

static int is_hex_digit(int character)
{
    return is_digit(character) || (character >= 'a' && character <= 'f') ||
           (character >= 'A' && character <= 'F');
}

static int is_identifier_start(int character)
{
    return (character >= 'a' && character <= 'z') ||
           (character >= 'A' && character <= 'Z') || character == '_';
}

static int is_identifier_character(int character)
{
    return is_identifier_start(character) || is_digit(character);
}

static int is_operator_character(int character)
{
    return character != '\0' && strchr("~!%^&*+=|?<>-", character) != NULL;
}

static int is_simple_escape_character(int character)
{
    return character != '\0' && strchr("\\abfnrtv\"'?", character) != NULL;
}

/* ---------------------------------------------------------------- buffer */

static int append_to_buffer(highlighter *machine, int character)
{
    if (machine->buffer_length >= WORD_BUFFER_SIZE)
        return 0;
    machine->buffer[machine->buffer_length] = (char)character;
    machine->buffer_length++;
    return 1;
}

static int buffer_equals(const highlighter *machine, const char *text)
{
    size_t text_length = strlen(text);

    return machine->buffer_length == text_length &&
           strncmp(machine->buffer, text, text_length) == 0;
}

static int is_string_prefix(const highlighter *machine)
{
    return buffer_equals(machine, "L") || buffer_equals(machine, "u") ||
           buffer_equals(machine, "U") || buffer_equals(machine, "u8");
}

static void begin_word(highlighter *machine, int character,
                       int does_start_line)
{
    machine->buffer_length = 0;
    append_to_buffer(machine, character);
    machine->does_word_start_line = does_start_line;
    machine->state = WORD;
}

/* ----------------------------------------------------------------- words */

static void finish_keyword(highlighter *machine, c_keyword_kind kind)
{
    highlight_style style = STYLE_BUILTIN;

    if (kind == C_KEYWORD_TYPE) {
        style = STYLE_TYPE;
        machine->previous_token = TOKEN_TYPE_LIKE;
    } else if (kind == C_KEYWORD_COMMAND) {
        style = STYLE_KEYWORD;
        if (buffer_equals(machine, "struct") ||
            buffer_equals(machine, "union"))
            machine->previous_token = TOKEN_STRUCT_KEYWORD;
        else if (buffer_equals(machine, "case"))
            machine->is_in_case_label = 1;
        /* other commands (static, const, ...) keep previous_token, so
           "static inline int name(" still sees the type before name */
    } else {
        machine->previous_token = TOKEN_OTHER;
    }
    emit_buffer_range(machine, 0, machine->word_length, style);
    machine->state = NORMAL;
}

/* An ordinary identifier is painted only once the next significant
   character is known: a function name is followed by '(', a struct name by
   '{', a label by ':'. next_character is EOF when unknown. Returns 1 when
   it also consumed next_character (the label colon). */
static int finish_identifier(highlighter *machine, int next_character)
{
    highlight_style style = STYLE_NONE;
    int is_label = 0;

    if (machine->brace_depth == 0 && next_character == '(' &&
        machine->previous_token == TOKEN_TYPE_LIKE) {
        style = STYLE_FUNCTION;
    } else if (machine->previous_token == TOKEN_STRUCT_KEYWORD &&
               next_character == '{') {
        style = STYLE_STRUCT_NAME;
    } else if (machine->does_word_start_line && machine->brace_depth > 0 &&
               next_character == ':' && !machine->does_gap_have_newline &&
               machine->previous_token == TOKEN_STATEMENT_BOUNDARY) {
        style = STYLE_LABEL;
        is_label = 1;
    }
    emit_buffer_range(machine, 0, machine->word_length, style);
    emit_buffer_range(machine, machine->word_length, machine->buffer_length,
                      STYLE_NONE);
    machine->previous_token =
        style == STYLE_NONE ? TOKEN_TYPE_LIKE : TOKEN_OTHER;
    /* if the blanks held a newline, the next character starts a line */
    machine->state = machine->does_gap_have_newline ? LINE_START : NORMAL;
    if (is_label) {
        emit(machine, ':', STYLE_NONE);
        machine->previous_token = TOKEN_STATEMENT_BOUNDARY;
        return 1;
    }
    return 0;
}

static int handle_word(highlighter *machine, int character)
{
    c_keyword_kind kind;

    if (is_identifier_character(character)) {
        if (append_to_buffer(machine, character))
            return 1;
        /* too long to be a keyword: write it now and stop buffering */
        emit_buffer_range(machine, 0, machine->buffer_length, STYLE_NONE);
        machine->previous_token = TOKEN_TYPE_LIKE;
        machine->state = LONG_WORD;
        return 0;
    }
    machine->word_length = machine->buffer_length;
    kind = c_keywords_lookup(machine->buffer, machine->word_length);
    if (kind != C_KEYWORD_NONE) {
        finish_keyword(machine, kind);
        return 0;
    }
    if ((character == '"' || character == '\'') && is_string_prefix(machine)) {
        /* L"x", u8"x", ...: the prefix is part of the string */
        emit_buffer_range(machine, 0, machine->word_length, STYLE_STRING);
        machine->previous_token = TOKEN_OTHER;
        machine->state = NORMAL;
        return 0;
    }
    machine->does_gap_have_newline = 0;
    machine->state = WORD_GAP;
    return 0;
}

static int handle_long_word(highlighter *machine, int character)
{
    if (is_identifier_character(character)) {
        emit(machine, character, STYLE_NONE);
        return 1;
    }
    machine->state = NORMAL;
    return 0;
}

/* Holds the blanks after an identifier until the next significant
   character shows what the identifier is. */
static int handle_word_gap(highlighter *machine, int character)
{
    if (character == '\n' || is_blank(character)) {
        if (append_to_buffer(machine, character)) {
            if (character == '\n')
                machine->does_gap_have_newline = 1;
            return 1;
        }
        /* absurd amount of blanks: give up looking ahead */
        return finish_identifier(machine, EOF);
    }
    return finish_identifier(machine, character);
}

/* --------------------------------------------------------------- numbers */

static int handle_number(highlighter *machine, int character)
{
    if (character == 'e' || character == 'E' || character == 'p' ||
        character == 'P') {
        emit(machine, character, STYLE_NUMBER);
        machine->state = NUMBER_EXPONENT;
        return 1;
    }
    if (is_identifier_character(character) || character == '.') {
        emit(machine, character, STYLE_NUMBER);
        return 1;
    }
    machine->state = NORMAL;
    return 0;
}

/* After e, E, p or P a sign belongs to the number (1e-3, 0x1p-3). */
static int handle_number_exponent(highlighter *machine, int character)
{
    machine->state = NUMBER;
    if (character == '+' || character == '-') {
        emit(machine, character, STYLE_NUMBER);
        return 1;
    }
    return 0;
}

static int handle_after_dot(highlighter *machine, int character)
{
    if (is_digit(character)) {              /* .5 */
        emit(machine, '.', STYLE_NUMBER);
        machine->state = NUMBER;
        return 0;
    }
    emit(machine, '.', STYLE_NONE);
    machine->state = NORMAL;
    return 0;
}

/* -------------------------------------------------------------- comments */

static int handle_after_slash(highlighter *machine, int character)
{
    if (character == '/') {
        emit(machine, '/', STYLE_COMMENT);
        emit(machine, character, STYLE_COMMENT);
        machine->state = LINE_COMMENT;
        return 1;
    }
    if (character == '*') {
        emit(machine, '/', STYLE_COMMENT);
        emit(machine, character, STYLE_COMMENT);
        machine->state = BLOCK_COMMENT;
        return 1;
    }
    if (machine->state_after_comment == PREPROCESSOR) {
        emit(machine, '/', STYLE_PREPROCESSOR);
        machine->state = PREPROCESSOR;
        return 0;
    }
    emit(machine, '/', STYLE_OPERATOR);     /* a lone slash is division */
    machine->previous_token = TOKEN_OTHER;
    machine->state = NORMAL;
    return 0;
}

static int handle_line_comment(highlighter *machine, int character)
{
    emit(machine, character, STYLE_COMMENT);
    if (character == '\n')
        machine->state = LINE_START;
    else if (character == '\\')
        machine->state = LINE_COMMENT_AFTER_BACKSLASH;
    return 1;
}

/* A backslash before a newline continues the comment on the next line. */
static int handle_line_comment_after_backslash(highlighter *machine,
                                               int character)
{
    emit(machine, character, STYLE_COMMENT);
    if (character != '\\')
        machine->state = LINE_COMMENT;
    return 1;
}

static int handle_block_comment(highlighter *machine, int character)
{
    emit(machine, character, STYLE_COMMENT);
    if (character == '*')
        machine->state = BLOCK_COMMENT_AFTER_STAR;
    return 1;
}

static int handle_block_comment_after_star(highlighter *machine, int character)
{
    emit(machine, character, STYLE_COMMENT);
    if (character == '/')
        machine->state = machine->state_after_comment;
    else if (character != '*')
        machine->state = BLOCK_COMMENT;
    return 1;
}

/* --------------------------------------------------------------- strings */

static int handle_string(highlighter *machine, int character)
{
    if (character == '\n') {
        emit(machine, character, STYLE_NONE);
        machine->state = LINE_START;
        return 1;
    }
    if (character == machine->delimiter) {
        emit(machine, character, STYLE_STRING);
        machine->state = NORMAL;
        return 1;
    }
    if (character == '\\') {
        machine->state = STRING_AFTER_BACKSLASH;
        return 1;
    }
    emit(machine, character, STYLE_STRING);
    return 1;
}

/* A simple escape takes no more digits, so its ESCAPE_DIGITS step only hands
   the next character back to STRING. */
static void begin_escape(highlighter *machine, int character, int digits_left,
                         int is_octal)
{
    emit(machine, '\\', STYLE_ESCAPE);
    emit(machine, character, STYLE_ESCAPE);
    machine->escape_digits_left = digits_left;
    machine->is_escape_octal = is_octal;
    machine->state = ESCAPE_DIGITS;
}

static int handle_string_after_backslash(highlighter *machine, int character)
{
    if (character == EOF) {
        emit(machine, '\\', STYLE_STRING);
        machine->state = STRING;
        return 0;
    }
    if (character == '\n') {                /* line continuation */
        emit(machine, '\\', STYLE_STRING);
        emit(machine, character, STYLE_STRING);
        machine->state = STRING;
        return 1;
    }
    if (machine->delimiter == '\'') {       /* character literals: no escapes */
        emit(machine, '\\', STYLE_STRING);
        emit(machine, character, STYLE_STRING);
        machine->state = STRING;
        return 1;
    }
    if (is_simple_escape_character(character)) {
        begin_escape(machine, character, 0, 0);
        return 1;
    }
    if (character == 'x') {
        begin_escape(machine, character, UNLIMITED_DIGITS, 0);
        return 1;
    }
    if (character == 'u') {
        begin_escape(machine, character, 4, 0);
        return 1;
    }
    if (character == 'U') {
        begin_escape(machine, character, 8, 0);
        return 1;
    }
    if (is_octal_digit(character)) {
        begin_escape(machine, character, 2, 1);
        return 1;
    }
    emit(machine, '\\', STYLE_STRING);      /* unknown escape: plain text */
    machine->state = STRING;
    return 0;
}

static int handle_escape_digits(highlighter *machine, int character)
{
    int is_digit_allowed = machine->is_escape_octal ? is_octal_digit(character)
                                                    : is_hex_digit(character);

    if (is_digit_allowed && machine->escape_digits_left != 0) {
        emit(machine, character, STYLE_ESCAPE);
        if (machine->escape_digits_left > 0)
            machine->escape_digits_left--;
        return 1;
    }
    machine->state = STRING;
    return 0;
}

/* ---------------------------------------------------------- preprocessor */

/* Collects the directive name, after any blanks that follow the hash;
   "include" is the only one whose argument (the file name) gets its own
   style. */
static int handle_preprocessor_name(highlighter *machine, int character)
{
    if (machine->buffer_length == 0 && is_blank(character)) {
        emit(machine, character, STYLE_PREPROCESSOR);
        return 1;
    }
    if (is_identifier_character(character)) {
        append_to_buffer(machine, character);
        emit(machine, character, STYLE_PREPROCESSOR);
        return 1;
    }
    machine->state = buffer_equals(machine, "include") ? INCLUDE_BLANKS
                                                       : PREPROCESSOR;
    return 0;
}

static int handle_preprocessor(highlighter *machine, int character)
{
    if (character == '\n') {
        emit(machine, character, STYLE_NONE);
        machine->state = LINE_START;
        return 1;
    }
    if (character == '\\') {
        machine->state = PREPROCESSOR_AFTER_BACKSLASH;
        return 1;
    }
    if (character == '/') {
        machine->state_after_comment = PREPROCESSOR;
        machine->state = AFTER_SLASH;
        return 1;
    }
    emit(machine, character, STYLE_PREPROCESSOR);
    if (character == '"')
        machine->state = PREPROCESSOR_STRING;
    return 1;
}

/* A string in a directive keeps the directive's style, but its slashes must
   not start a comment: #define URL "http://example.com". */
static int handle_preprocessor_string(highlighter *machine, int character)
{
    if (character == '\n') {                /* unterminated string */
        emit(machine, character, STYLE_NONE);
        machine->state = LINE_START;
        return 1;
    }
    emit(machine, character, STYLE_PREPROCESSOR);
    if (character == '\\')
        machine->state = PREPROCESSOR_STRING_AFTER_BACKSLASH;
    else if (character == '"')
        machine->state = PREPROCESSOR;
    return 1;
}

static int handle_preprocessor_string_after_backslash(highlighter *machine,
                                                      int character)
{
    emit(machine, character, STYLE_PREPROCESSOR);
    machine->state = PREPROCESSOR_STRING;
    return 1;
}

static int handle_preprocessor_after_backslash(highlighter *machine,
                                               int character)
{
    emit(machine, '\\', STYLE_PREPROCESSOR);
    machine->state = PREPROCESSOR;
    if (character == '\n') {                /* the directive continues */
        emit(machine, character, STYLE_PREPROCESSOR);
        return 1;
    }
    return 0;
}

static int handle_include_blanks(highlighter *machine, int character)
{
    if (character == '\n') {
        emit(machine, character, STYLE_NONE);
        machine->state = LINE_START;
        return 1;
    }
    if (is_blank(character)) {
        emit(machine, character, STYLE_NONE);
        return 1;
    }
    if (character == '<' || character == '"') {
        machine->delimiter = character == '<' ? '>' : '"';
        emit(machine, character, STYLE_FILE_NAME);
        machine->state = INCLUDE_FILE;
        return 1;
    }
    machine->state = NORMAL;                /* #include MACRO */
    return 0;
}

static int handle_include_file(highlighter *machine, int character)
{
    if (character == '\n') {
        emit(machine, character, STYLE_NONE);
        machine->state = LINE_START;
        return 1;
    }
    emit(machine, character, STYLE_FILE_NAME);
    if (character == machine->delimiter)
        machine->state = NORMAL;
    return 1;
}

/* ------------------------------------------------------ line and normal */

static int handle_line_start(highlighter *machine, int character)
{
    if (character == '\n' || is_blank(character)) {
        emit(machine, character, STYLE_NONE);
        return 1;
    }
    if (character == '#') {
        emit(machine, character, STYLE_PREPROCESSOR);
        machine->previous_token = TOKEN_STATEMENT_BOUNDARY;
        machine->buffer_length = 0;
        machine->state = PREPROCESSOR_NAME;
        return 1;
    }
    if (is_identifier_start(character)) {
        begin_word(machine, character, 1);
        return 1;
    }
    machine->state = NORMAL;
    return 0;
}

static int handle_normal(highlighter *machine, int character)
{
    if (character == '\n') {
        emit(machine, character, STYLE_NONE);
        machine->state = LINE_START;
        return 1;
    }
    if (is_blank(character)) {
        emit(machine, character, STYLE_NONE);
        return 1;
    }
    if (is_identifier_start(character)) {
        begin_word(machine, character, 0);
        return 1;
    }
    if (is_digit(character)) {
        emit(machine, character, STYLE_NUMBER);
        machine->previous_token = TOKEN_OTHER;
        machine->state = NUMBER;
        return 1;
    }
    if (character == '.') {
        machine->state = AFTER_DOT;
        return 1;
    }
    if (character == '/') {
        machine->state_after_comment = NORMAL;
        machine->state = AFTER_SLASH;
        return 1;
    }
    if (character == '"' || character == '\'') {
        machine->delimiter = character;
        emit(machine, character, STYLE_STRING);
        machine->previous_token = TOKEN_OTHER;
        machine->state = STRING;
        return 1;
    }
    if (character == ':') {
        /* the colon that ends "case N" is plain; any other colon (ternary,
           bit field, "default:") is an operator */
        emit(machine, character,
             machine->is_in_case_label ? STYLE_NONE : STYLE_OPERATOR);
        machine->is_in_case_label = 0;
        machine->previous_token = TOKEN_STATEMENT_BOUNDARY;
        return 1;
    }
    if (character == '*') {
        /* keeps previous_token: in "void *name(" the star is part of the
           declaration */
        emit(machine, character, STYLE_OPERATOR);
        return 1;
    }
    if (is_operator_character(character)) {
        emit(machine, character, STYLE_OPERATOR);
        machine->previous_token = TOKEN_OTHER;
        return 1;
    }
    if (character == '{')
        machine->brace_depth++;
    else if (character == '}' && machine->brace_depth > 0)
        machine->brace_depth--;
    emit(machine, character, STYLE_NONE);   /* punctuation stays plain */
    machine->previous_token =
        (character == ';' || character == '{' || character == '}' ||
         character == ')') ? TOKEN_STATEMENT_BOUNDARY : TOKEN_OTHER;
    return 1;
}

/* ------------------------------------------------------------ dispatcher */

/* States that have taken characters without writing them yet. They must see
   the end of input to flush; the others can simply stop. */
static int does_state_hold_pending_output(highlighter_state state)
{
    switch (state) {
    case WORD:
    case WORD_GAP:
    case AFTER_DOT:
    case AFTER_SLASH:
    case STRING_AFTER_BACKSLASH:
    case PREPROCESSOR_AFTER_BACKSLASH:
        return 1;
    default:
        return 0;
    }
}

/* Returns 1 when the character was consumed, 0 when the (new) state must
   handle it again. */
static int handle_state(highlighter *machine, int character)
{
    if (character == EOF && !does_state_hold_pending_output(machine->state))
        return 1;
    switch (machine->state) {
    case LINE_START:
        return handle_line_start(machine, character);
    case NORMAL:
        return handle_normal(machine, character);
    case WORD:
        return handle_word(machine, character);
    case WORD_GAP:
        return handle_word_gap(machine, character);
    case LONG_WORD:
        return handle_long_word(machine, character);
    case NUMBER:
        return handle_number(machine, character);
    case NUMBER_EXPONENT:
        return handle_number_exponent(machine, character);
    case AFTER_DOT:
        return handle_after_dot(machine, character);
    case AFTER_SLASH:
        return handle_after_slash(machine, character);
    case LINE_COMMENT:
        return handle_line_comment(machine, character);
    case LINE_COMMENT_AFTER_BACKSLASH:
        return handle_line_comment_after_backslash(machine, character);
    case BLOCK_COMMENT:
        return handle_block_comment(machine, character);
    case BLOCK_COMMENT_AFTER_STAR:
        return handle_block_comment_after_star(machine, character);
    case STRING:
        return handle_string(machine, character);
    case STRING_AFTER_BACKSLASH:
        return handle_string_after_backslash(machine, character);
    case ESCAPE_DIGITS:
        return handle_escape_digits(machine, character);
    case PREPROCESSOR_NAME:
        return handle_preprocessor_name(machine, character);
    case PREPROCESSOR:
        return handle_preprocessor(machine, character);
    case PREPROCESSOR_AFTER_BACKSLASH:
        return handle_preprocessor_after_backslash(machine, character);
    case PREPROCESSOR_STRING:
        return handle_preprocessor_string(machine, character);
    case PREPROCESSOR_STRING_AFTER_BACKSLASH:
        return handle_preprocessor_string_after_backslash(machine, character);
    case INCLUDE_BLANKS:
        return handle_include_blanks(machine, character);
    case INCLUDE_FILE:
        return handle_include_file(machine, character);
    }
    return 1;
}

static void process_character(highlighter *machine, int character)
{
    while (!handle_state(machine, character))
        ;
}

void c_highlighter_write_html(FILE *source_file, FILE *html_file)
{
    highlighter machine;
    int character;

    machine.html_file = html_file;
    machine.state = LINE_START;
    machine.state_after_comment = NORMAL;
    machine.open_style = STYLE_NONE;
    machine.is_line_open = 0;
    machine.brace_depth = 0;
    machine.previous_token = TOKEN_STATEMENT_BOUNDARY;
    machine.is_in_case_label = 0;
    machine.delimiter = 0;
    machine.escape_digits_left = 0;
    machine.is_escape_octal = 0;
    machine.buffer_length = 0;
    machine.word_length = 0;
    machine.does_word_start_line = 0;
    machine.does_gap_have_newline = 0;

    while ((character = fgetc(source_file)) != EOF) {
        if (character == '\r')              /* Windows line endings */
            continue;
        process_character(&machine, character);
    }
    process_character(&machine, EOF);
    finish_output(&machine);
}

/* ------------------------------------------------------------------- CSS */

/* One literal per line: a single string literal longer than 509 characters
   is not portable C89. Colors are the Pygments "default" theme, the one
   used by Wikipedia. */
static const char *const CSS_LINES[] = {
    "pre.codigo { background:#f2f2f2; border:1px solid #c8c8c8;\n",
    "             border-radius:4px; padding:10px 12px; overflow:auto;\n",
    "             font-family:Menlo,Consolas,'DejaVu Sans Mono',monospace;\n",
    "             font-size:14px; line-height:1.45; tab-size:4;\n",
    "             counter-reset:linha; }\n",
    ".l::before { counter-increment:linha; content:counter(linha);\n",
    "              display:inline-block; width:2.5em; margin-right:1em;\n",
    "              padding-right:0.8em; text-align:right; color:#888;\n",
    "              border-right:1px solid #bbb; user-select:none; }\n",
    ".pragma    { color:#9C6500; }\n",
    ".arquivo   { color:#3D7B7B; font-style:italic; }\n",
    ".coment    { color:#3D7B7B; font-style:italic; }\n",
    ".tipo      { color:#B00040; }\n",
    ".comando   { color:#008000; font-weight:bold; }\n",
    ".builtin   { color:#008000; }\n",
    ".funcao    { color:#0000FF; }\n",
    ".estrutura { color:#0000FF; font-weight:bold; }\n",
    ".rotulo    { color:#767600; }\n",
    ".string    { color:#BA2121; }\n",
    ".escape    { color:#AA5D1F; font-weight:bold; }\n",
    ".numero    { color:#666666; }\n",
    ".operador  { color:#666666; }\n",
    NULL
};

void c_highlighter_write_css(FILE *css_file)
{
    size_t line_index;

    for (line_index = 0; CSS_LINES[line_index] != NULL; line_index++)
        fputs(CSS_LINES[line_index], css_file);
}
