#include "../src/c_highlighter.h"

#include <stdio.h>
#include <string.h>

#define MAX_HTML_LEN 16384
#define WORD_BUFFER_SIZE 256

/* Expected HTML, written with the same class names as the generated pages. */
#define LINE(content)   "<span class=\"l\">" content "</span>"
#define PRAGMA(text)    "<span class=\"pragma\">" text "</span>"
#define FILE_NAME(text) "<span class=\"arquivo\">" text "</span>"
#define COMMENT(text)   "<span class=\"coment\">" text "</span>"
#define TYPE(text)      "<span class=\"tipo\">" text "</span>"
#define KEYWORD(text)   "<span class=\"comando\">" text "</span>"
#define BUILTIN(text)   "<span class=\"builtin\">" text "</span>"
#define FUNCTION(text)  "<span class=\"funcao\">" text "</span>"
#define TAG(text)       "<span class=\"estrutura\">" text "</span>"
#define LABEL(text)     "<span class=\"rotulo\">" text "</span>"
#define STRING(text)    "<span class=\"string\">" text "</span>"
#define ESCAPE(text)    "<span class=\"escape\">" text "</span>"
#define NUMBER(text)    "<span class=\"numero\">" text "</span>"
#define OPERATOR(text)  "<span class=\"operador\">" text "</span>"

static int failure_count = 0;

static size_t count_occurrences(const char *text, const char *pattern)
{
    size_t occurrence_count = 0;
    const char *match = strstr(text, pattern);

    while (match != NULL) {
        occurrence_count++;
        match = strstr(match + 1, pattern);
    }
    return occurrence_count;
}

/* Runs the highlighter over a temporary file. Returns 1 on success. */
static int render_html(const char *source, char *html, size_t html_capacity)
{
    FILE *source_file = tmpfile();
    FILE *html_file = tmpfile();
    size_t html_len;

    if (source_file == NULL || html_file == NULL) {
        printf("could not open temporary files\n");
        failure_count++;
        if (source_file != NULL)
            fclose(source_file);
        if (html_file != NULL)
            fclose(html_file);
        return 0;
    }
    fputs(source, source_file);
    rewind(source_file);
    c_highlighter_write_html(source_file, html_file);
    rewind(html_file);
    html_len = fread(html, 1, html_capacity - 1, html_file);
    html[html_len] = '\0';
    fclose(source_file);
    fclose(html_file);
    return 1;
}

static void check_html(const char *source, const char *expected_html)
{
    char html[MAX_HTML_LEN];

    if (render_html(source, html, MAX_HTML_LEN) &&
        strcmp(html, expected_html) != 0) {
        printf("source:   %s\nexpected: %s\nobtained: %s\n\n", source,
               expected_html, html);
        failure_count++;
    }
}

static void check_spans_balanced(const char *source)
{
    char html[MAX_HTML_LEN];

    if (render_html(source, html, MAX_HTML_LEN) &&
        count_occurrences(html, "<span") != count_occurrences(html, "</span>")) {
        printf("unbalanced spans\nsource: %s\nobtained: %s\n\n", source, html);
        failure_count++;
    }
}

static void check_same_html(const char *first_source, const char *second_source)
{
    char first_html[MAX_HTML_LEN];
    char second_html[MAX_HTML_LEN];

    if (render_html(first_source, first_html, MAX_HTML_LEN) &&
        render_html(second_source, second_html, MAX_HTML_LEN) &&
        strcmp(first_html, second_html) != 0) {
        printf("different HTML\nfirst:  %s\nsecond: %s\n\n", first_html,
               second_html);
        failure_count++;
    }
}

static void test_all_styles(void)
{
    check_html("#define MAX 100", LINE(PRAGMA("#define MAX 100")));
    check_html("#include <stdio.h>",
               LINE(PRAGMA("#include") " " FILE_NAME("&lt;stdio.h&gt;")));
    check_html("// text", LINE(COMMENT("// text")));
    check_html("/* text */", LINE(COMMENT("/* text */")));
    check_html("size_t", LINE(TYPE("size_t")));
    check_html("while", LINE(KEYWORD("while")));
    check_html("NULL", LINE(BUILTIN("NULL")));
    check_html("int soma(int n);",
               LINE(TYPE("int") " " FUNCTION("soma") "(" TYPE("int") " n);"));
    check_html("struct ponto {",
               LINE(KEYWORD("struct") " " TAG("ponto") " {"));
    check_html("{\nfim:\n}", LINE("{") "\n" LINE(LABEL("fim") ":") "\n" LINE("}"));
    check_html("\"text\"", LINE(STRING("\"text\"")));
    check_html("\"\\n\"", LINE(STRING("\"") ESCAPE("\\n") STRING("\"")));
    check_html("42", LINE(NUMBER("42")));
    check_html("x == y", LINE("x " OPERATOR("==") " y"));
    check_html("integer", LINE("integer"));
}

static void test_line_wrappers(void)
{
    check_html("", "");
    check_html("\n", LINE("") "\n");
    check_html("x\n\ny\n", LINE("x") "\n" LINE("") "\n" LINE("y") "\n");
    /* a style span closes at the newline and reopens on the next line */
    check_html("/* a\nb */", LINE(COMMENT("/* a")) "\n" LINE(COMMENT("b */")));
}

static void test_html_escape(void)
{
    check_html("a < b", LINE("a " OPERATOR("&lt;") " b"));
    check_html("a && b", LINE("a " OPERATOR("&amp;&amp;") " b"));
    check_html("\"<&>\"", LINE(STRING("\"&lt;&amp;&gt;\"")));
    check_html("/* <&> */", LINE(COMMENT("/* &lt;&amp;&gt; */")));
    check_html("// <&>", LINE(COMMENT("// &lt;&amp;&gt;")));
}

static void test_numbers(void)
{
    check_html("1e-3", LINE(NUMBER("1e-3")));
    check_html("0x1F", LINE(NUMBER("0x1F")));
    check_html(".5", LINE(NUMBER(".5")));
    check_html("1.5f", LINE(NUMBER("1.5f")));
    check_html("a.b", LINE("a.b"));
    check_html("a / b", LINE("a " OPERATOR("/") " b"));
}

static void test_strings(void)
{
    /* blanks in a string keep its style */
    check_html("\" \"", LINE(STRING("\" \"")));
    check_html("' '", LINE(STRING("' '")));
    check_html("\"\\x41zz\"",
               LINE(STRING("\"") ESCAPE("\\x41") STRING("zz\"")));
    check_html("\"\\1012\"",
               LINE(STRING("\"") ESCAPE("\\101") STRING("2\"")));
    /* an octal escape stops at a digit that is hex but not octal */
    check_html("\"\\18\"",
               LINE(STRING("\"") ESCAPE("\\1") STRING("8\"")));
    check_html("\"\\u00e9f\"",
               LINE(STRING("\"") ESCAPE("\\u00e9") STRING("f\"")));
    check_html("\"\\q\"", LINE(STRING("\"\\q\"")));
    check_html("\"\\\"\"", LINE(STRING("\"") ESCAPE("\\\"") STRING("\"")));
    check_html("'\\n'", LINE(STRING("'\\n'")));
    check_html("'\\''", LINE(STRING("'\\''")));
    check_html("L\"x\"", LINE(STRING("L\"x\"")));
    check_html("u8\"x\"", LINE(STRING("u8\"x\"")));
    /* a newline ends an unterminated string */
    check_html("\"a\nb", LINE(STRING("\"a")) "\n" LINE("b"));
    /* a backslash before a newline continues the string */
    check_html("\"a\\\nb\"", LINE(STRING("\"a\\")) "\n" LINE(STRING("b\"")));
}

static void test_directives(void)
{
    check_html("#  include \"a.h\"",
               LINE(PRAGMA("#  include") " " FILE_NAME("\"a.h\"")));
    check_html("#include MACRO", LINE(PRAGMA("#include") " MACRO"));
    check_html("#include\nx", LINE(PRAGMA("#include")) "\n" LINE("x"));
    check_html("#include <a.h\nx",
               LINE(PRAGMA("#include") " " FILE_NAME("&lt;a.h")) "\n" LINE("x"));
    check_html("#\nx", LINE(PRAGMA("#")) "\n" LINE("x"));
    check_html("# 1", LINE(PRAGMA("# 1")));
    check_html("#define A 1 / 2", LINE(PRAGMA("#define A 1 / 2")));
    check_html("#define A \\\n 1\nx",
               LINE(PRAGMA("#define A \\")) "\n" LINE(PRAGMA(" 1")) "\n"
               LINE("x"));
    check_html("#define A /* c */ 1",
               LINE(PRAGMA("#define A ") COMMENT("/* c */") PRAGMA(" 1")));
    check_html("#define A // c", LINE(PRAGMA("#define A ") COMMENT("// c")));
}

static void test_gaps_after_words(void)
{
    check_html("int soma (int n);",
               LINE(TYPE("int") " " FUNCTION("soma") " (" TYPE("int") " n);"));
    check_html("int\nsoma\n(void)",
               LINE(TYPE("int")) "\n" LINE(FUNCTION("soma")) "\n"
               LINE("(" TYPE("void") ")"));
    check_html("struct ponto\n{",
               LINE(KEYWORD("struct") " " TAG("ponto")) "\n" LINE("{"));
    check_html("{\nfim :\n}",
               LINE("{") "\n" LINE(LABEL("fim") " :") "\n" LINE("}"));
    /* a word followed by a newline keeps the next line's line start */
    check_html("x\n#define A", LINE("x") "\n" LINE(PRAGMA("#define A")));
}

static void test_directive_strings(void)
{
    check_html("#define URL \"http://x\"",
               LINE(PRAGMA("#define URL \"http://x\"")));
    check_html("#define A \"/* x */\"", LINE(PRAGMA("#define A \"/* x */\"")));
    check_html("#define A \"a\\\"//b\" // c",
               LINE(PRAGMA("#define A \"a\\\"//b\" ") COMMENT("// c")));
    /* the string ends at the newline, so the next line is code again */
    check_html("#define A \"x\ny", LINE(PRAGMA("#define A \"x")) "\n" LINE("y"));
}

static void test_line_comment_continuation(void)
{
    check_html("// a \\\nb\nx",
               LINE(COMMENT("// a \\")) "\n" LINE(COMMENT("b")) "\n" LINE("x"));
    check_html("// a \\\\\nb\nx",
               LINE(COMMENT("// a \\\\")) "\n" LINE(COMMENT("b")) "\n"
               LINE("x"));
    check_html("// a \\ b\nx", LINE(COMMENT("// a \\ b")) "\n" LINE("x"));
}

static void test_labels(void)
{
    /* the second branch of a ternary is not a label */
    check_html("{ x = y ?\nz : w;\n}",
               LINE("{ x " OPERATOR("=") " y " OPERATOR("?")) "\n"
               LINE("z " OPERATOR(":") " w;") "\n" LINE("}"));
    /* neither is a bit field whose type is on the line before */
    check_html("{ int\nb : 2;\n}",
               LINE("{ " TYPE("int")) "\n"
               LINE("b " OPERATOR(":") " " NUMBER("2") ";") "\n" LINE("}"));
    check_html("{ x;\nfim:\ny;\n}",
               LINE("{ x;") "\n" LINE(LABEL("fim") ":") "\n" LINE("y;") "\n"
               LINE("}"));
    check_html("{\nfim:\noutro:\n}",
               LINE("{") "\n" LINE(LABEL("fim") ":") "\n"
               LINE(LABEL("outro") ":") "\n" LINE("}"));
    /* outside a block there are no labels */
    check_html("fim:", LINE("fim" OPERATOR(":")));
}

static void test_function_names(void)
{
    check_html("main()\n{\n}",
               LINE(FUNCTION("main") "()") "\n" LINE("{") "\n" LINE("}"));
    check_html("struct p {} f(void);",
               LINE(KEYWORD("struct") " " TAG("p") " {} " FUNCTION("f") "("
                    TYPE("void") ");"));
    check_html("void *f(void);",
               LINE(TYPE("void") " " OPERATOR("*") FUNCTION("f") "("
                    TYPE("void") ");"));
    /* a call inside a block, or in an initializer, is not a definition */
    check_html("{ f(); }", LINE("{ f(); }"));
    check_html("int x = f(1);",
               LINE(TYPE("int") " x " OPERATOR("=") " f(" NUMBER("1") ");"));
    /* accepted limit, see the README */
    check_html("DECLARE(x);", LINE(FUNCTION("DECLARE") "(x);"));
}

static void test_comment_at_line_start(void)
{
    check_html("/* c */ #define X",
               LINE(COMMENT("/* c */") " " PRAGMA("#define X")));
    check_html("/* a\nb */ #define X",
               LINE(COMMENT("/* a")) "\n"
               LINE(COMMENT("b */") " " PRAGMA("#define X")));
    check_html("{\n/* c */ fim:\n}",
               LINE("{") "\n" LINE(COMMENT("/* c */") " " LABEL("fim") ":") "\n"
               LINE("}"));
    /* a comment after a token does not start a directive */
    check_html("x /* c */ #", LINE("x " COMMENT("/* c */") " #"));
    /* a lone slash at the line start is still an operator */
    check_html("/ 2", LINE(OPERATOR("/") " " NUMBER("2")));
}

static void test_colons(void)
{
    check_html("{ case 1: x; }",
               LINE("{ " KEYWORD("case") " " NUMBER("1") OPERATOR(":") " x; }"));
    check_html("{ default: x; }",
               LINE("{ " KEYWORD("default") OPERATOR(":") " x; }"));
    check_html("a ? b : c",
               LINE("a " OPERATOR("?") " b " OPERATOR(":") " c"));
}

static void test_removed_keyword(void)
{
    check_html("typedef unsigned char byte;",
               LINE(KEYWORD("typedef") " " TYPE("unsigned") " " TYPE("char")
                    " byte;"));
}

static void test_end_of_input(void)
{
    static const char *const SOURCES[] = {
        "int foo", "foo", "foo   ", "foo\n", "int", "1", "1e", "1e+", ".", "/",
        "a /", "\"", "\"a", "\"\\", "\"\\x", "\"\\x4", "\"\\1", "\"\\u0", "'",
        "'a", "'\\", "//", "// a", "// a \\", "/*", "/* a", "/* a *", "#",
        "#  ", "#define", "#define A", "#define A \\", "#define A /",
        "#define A /*", "#define A //", "#define A \"", "#define A \"x\\",
        "#include", "#include ", "#include <", "#include <a", "#include \"a",
        NULL
    };
    int source_index;

    for (source_index = 0; SOURCES[source_index] != NULL; source_index++)
        check_spans_balanced(SOURCES[source_index]);

    /* the characters a state was holding must not be lost */
    check_html("foo   ", LINE("foo   "));
    check_html("a /", LINE("a " OPERATOR("/")));
    check_html(".", LINE("."));
    check_html("\"\\", LINE(STRING("\"\\")));
    check_html("#define A \\", LINE(PRAGMA("#define A \\")));
    check_html("#define A /", LINE(PRAGMA("#define A /")));
    check_html("/* a *", LINE(COMMENT("/* a *")));
}

static void test_line_endings(void)
{
    check_same_html("int x;\r\n/* a\r\nb */\r\n", "int x;\n/* a\nb */\n");
    check_same_html("#define A \\\r\n 1\r\nx\r\n", "#define A \\\n 1\nx\n");
    check_same_html("// a \\\r\nb\r\n", "// a \\\nb\n");
    check_same_html("\"a\\\r\nb\"\r\n", "\"a\\\nb\"\n");
}

/* Writes "<prefix><length times fill><suffix>" into source. */
static void build_long_source(char *source, const char *prefix, char fill,
                              size_t length, const char *suffix)
{
    size_t prefix_len = strlen(prefix);

    strcpy(source, prefix);
    memset(source + prefix_len, fill, length);
    strcpy(source + prefix_len + length, suffix);
}

static void check_long_word_text(const char *source, const char *pattern)
{
    char html[MAX_HTML_LEN];

    if (render_html(source, html, MAX_HTML_LEN) &&
        (strstr(html, pattern) == NULL ||
         count_occurrences(html, "<span") !=
             count_occurrences(html, "</span>"))) {
        printf("long input lost text or balance\nsource: %s\nobtained: %s\n\n",
               source, html);
        failure_count++;
    }
}

static void test_buffer_limits(void)
{
    char source[MAX_HTML_LEN];
    char pattern[MAX_HTML_LEN];
    size_t length;

    /* around the point where the word no longer fits in the buffer */
    for (length = WORD_BUFFER_SIZE - 1; length <= WORD_BUFFER_SIZE + 1;
         length++) {
        build_long_source(source, "int ", 'a', length, "(void);");
        build_long_source(pattern, "", 'a', length, "");
        check_long_word_text(source, pattern);
    }
    build_long_source(source, "int ", 'b', 1000, " (void);");
    build_long_source(pattern, "", 'b', 1000, "");
    check_long_word_text(source, pattern);

    /* too many blanks to hold: the word is written without looking further */
    build_long_source(source, "int q", ' ', 1000, "(void);");
    build_long_source(pattern, "q", ' ', 1000, "");
    check_long_word_text(source, pattern);

    build_long_source(source, "#", 'd', 1000, " x");
    build_long_source(pattern, "#", 'd', 1000, " x");
    check_long_word_text(source, pattern);
}

/* Calls share nothing: each code block starts from a clean machine. */
static void test_calls_are_independent(void)
{
    check_html("void f(void) {", LINE(TYPE("void") " " FUNCTION("f") "("
                                      TYPE("void") ") {"));
    /* the brace above must not still be open */
    check_html("int g(void);", LINE(TYPE("int") " " FUNCTION("g") "("
                                    TYPE("void") ");"));
}

static void test_css(void)
{
    static const char *const REQUIRED_CSS[] = {
        "color:#101418", "counter-reset", "counter-increment",
        "counter(linha)", ".pragma",
        ".arquivo", ".coment", ".tipo", ".comando", ".builtin", ".funcao",
        ".estrutura", ".rotulo", ".string", ".escape", ".numero", ".operador",
        NULL
    };
    char css[MAX_HTML_LEN];
    FILE *css_file = tmpfile();
    size_t css_len;
    int css_index;

    if (css_file == NULL) {
        printf("could not open a temporary file\n");
        failure_count++;
        return;
    }
    c_highlighter_write_css(css_file);
    rewind(css_file);
    css_len = fread(css, 1, MAX_HTML_LEN - 1, css_file);
    css[css_len] = '\0';
    fclose(css_file);
    for (css_index = 0; REQUIRED_CSS[css_index] != NULL; css_index++)
        if (strstr(css, REQUIRED_CSS[css_index]) == NULL) {
            printf("CSS is missing %s\n", REQUIRED_CSS[css_index]);
            failure_count++;
        }
}

static void check_head_styles(int has_page_style)
{
    char head[MAX_HTML_LEN];
    FILE *html_file = tmpfile();
    size_t head_len;
    const char *link;
    const char *page_style;

    if (html_file == NULL) {
        printf("could not open a temporary file\n");
        failure_count++;
        return;
    }
    c_highlighter_write_head_styles(html_file, has_page_style);
    rewind(html_file);
    head_len = fread(head, 1, MAX_HTML_LEN - 1, html_file);
    head[head_len] = '\0';
    fclose(html_file);

    link = strstr(head, "<link rel=\"stylesheet\" href=\"style.css\">");
    page_style = strstr(head, "<style>");
    if (link == NULL) {
        printf("the head does not link style.css\n%s\n", head);
        failure_count++;
    }
    if (!has_page_style && page_style != NULL) {
        printf("a <style> block was not asked for\n%s\n", head);
        failure_count++;
    }
    if (has_page_style) {
        /* the page's styles win only if they come after the link */
        if (page_style == NULL || link == NULL || page_style < link ||
            strstr(page_style, ".tipo") == NULL ||
            strstr(page_style, "</style>") == NULL) {
            printf("the page's styles must follow the link\n%s\n", head);
            failure_count++;
        }
    }
}

/* The entry point for text in memory must give what the file one gives. */
static void check_same_html_from_text(const char *source)
{
    char file_html[MAX_HTML_LEN];
    char text_html[MAX_HTML_LEN];
    FILE *html_file;
    size_t html_len;

    if (!render_html(source, file_html, sizeof(file_html)))
        return;
    html_file = tmpfile();
    if (html_file == NULL) {
        printf("could not open temporary files\n");
        failure_count++;
        return;
    }
    c_highlighter_write_html_from_text(source, html_file);
    rewind(html_file);
    html_len = fread(text_html, 1, sizeof(text_html) - 1, html_file);
    text_html[html_len] = '\0';
    fclose(html_file);
    if (strcmp(file_html, text_html) != 0) {
        printf("FAIL from text differs from file for: %s\n", source);
        failure_count++;
    }
}

static void test_html_from_text(void)
{
    check_same_html_from_text("");
    check_same_html_from_text("int soma(int a) { return a + 1; }\n");
    check_same_html_from_text("#include <stdio.h>\r\nint x; /* a < b */\r\n");
    check_same_html_from_text("char *s = \"unterminated");
    check_same_html_from_text("/* unterminated & comment");
    check_same_html_from_text("a\xC3\xA7\xC3\xA3o = 1;");
}

int main(void)
{
    test_all_styles();
    test_line_wrappers();
    test_html_escape();
    test_numbers();
    test_strings();
    test_directives();
    test_gaps_after_words();
    test_directive_strings();
    test_line_comment_continuation();
    test_labels();
    test_function_names();
    test_comment_at_line_start();
    test_colons();
    test_removed_keyword();
    test_end_of_input();
    test_line_endings();
    test_buffer_limits();
    test_calls_are_independent();
    test_html_from_text();
    test_css();
    check_head_styles(0);
    check_head_styles(1);

    return failure_count == 0 ? 0 : 1;
}
