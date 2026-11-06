/*
 * comparator - compares the HTML written by c_filter with the expected HTML.
 *
 * Build (from the repository root):
 *   gcc -std=c89 -Wall -Wextra -pedantic -Werror tests/comparator.c \
 *       -o comparator
 *
 * Use:
 *   ./c_filter tests/test.c | ./comparator tests/expected.html -
 *
 * What it does:
 *   - Reads the content of <pre class="codigo">...</pre> in both files.
 *   - Checks that every visible character has the same highlight class.
 *     Blanks and newlines are not compared: they show no color, and
 *     carriage returns are dropped when the files are read.
 *   - Checks the line numbering: every line must open one <span class="l">
 *     (a CSS counter draws the number) in the same column as the expected
 *     file, and the counter's CSS must exist in the obtained file or in the
 *     style sheet it links. A relative link is read from the obtained file's
 *     folder, or from the current folder when the file is standard input.
 *
 * Exit status:
 *   0 = identical
 *   1 = the classes or the line numbering differ
 *   2 = usage error, or the two files do not hold the same text
 *
 * Ownership: the file readers and the cell lists allocate with malloc and
 * realloc; main frees them. Interned class names live until the program ends.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CLASSES 64
#define MAX_OPEN_SPANS 64
#define NO_CLASS ""             /* class of a character outside any <span> */
#define PRE_OPENING_TAG "<pre class=\"codigo\">"
#define STYLE_SHEET_LINK "<link rel=\"stylesheet\" href=\""

typedef struct {
    char character;             /* already without its HTML entity */
    const char *class_name;     /* interned */
    int line_number;            /* position in the source, from 1 */
    int column_number;
} cell;

typedef struct {
    cell *cells;
    size_t cell_count;
    size_t cell_capacity;
    int *line_span_counts;      /* per line: how many <span class="l"> open */
    int *line_span_columns;     /* per line: the column of the first one */
    size_t line_capacity;       /* size of the two vectors above */
    int line_count;
} cell_list;

typedef struct {
    const char *class_name;
    long total_count;
    long correct_count;
} class_summary;

static const char *const REQUIRED_CSS[] = {
    "counter-reset", "counter-increment", "counter(", "before"
};

#define REQUIRED_CSS_COUNT 4

static char *interned_class_names[MAX_CLASSES];
static int interned_class_count = 0;

static class_summary class_summaries[MAX_CLASSES + 1];  /* by expected class */
static int class_summary_count = 0;

/* --------------------------------------------------------------- allocation */

static void exit_out_of_memory(void)
{
    fprintf(stderr, "Error: out of memory\n");
    exit(2);
}

static void *allocate_or_exit(size_t size)
{
    void *memory = malloc(size);

    if (memory == NULL)
        exit_out_of_memory();
    return memory;
}

static void *reallocate_or_exit(void *memory, size_t size)
{
    void *new_memory = realloc(memory, size);

    if (new_memory == NULL)
        exit_out_of_memory();
    return new_memory;
}

/* ------------------------------------------------------------- class names */

/* Interned, so that comparing two class names is comparing two pointers. */
static const char *intern_class_name(const char *name, size_t name_len)
{
    int name_index;

    for (name_index = 0; name_index < interned_class_count; name_index++)
        if (strlen(interned_class_names[name_index]) == name_len &&
            strncmp(interned_class_names[name_index], name, name_len) == 0)
            return interned_class_names[name_index];
    if (interned_class_count == MAX_CLASSES) {
        fprintf(stderr, "Error: too many classes\n");
        exit(2);
    }
    interned_class_names[interned_class_count] =
        allocate_or_exit(name_len + 1);
    memcpy(interned_class_names[interned_class_count], name, name_len);
    interned_class_names[interned_class_count][name_len] = '\0';
    return interned_class_names[interned_class_count++];
}

/* -------------------------------------------------------------- file input */

/* A Windows checkout can give the expected file CRLF endings, and standard
   input and a file opened with "rb" treat CRLF differently there. Endings
   show no color, so they must not decide the comparison. */
static void remove_carriage_returns(char *text, size_t text_len)
{
    size_t read_index;
    size_t kept_len = 0;

    for (read_index = 0; read_index < text_len; read_index++)
        if (text[read_index] != '\r')
            text[kept_len++] = text[read_index];
    text[kept_len] = '\0';
}

/* Returns NULL when the file cannot be opened. */
static char *read_file_if_present(const char *file_name)
{
    FILE *input_file = strcmp(file_name, "-") == 0 ? stdin
                                                   : fopen(file_name, "rb");
    size_t capacity = 8192;
    size_t text_len = 0;
    size_t read_count;
    char *text;

    if (input_file == NULL)
        return NULL;
    text = allocate_or_exit(capacity + 1);
    while ((read_count = fread(text + text_len, 1, capacity - text_len,
                               input_file)) > 0) {
        text_len += read_count;
        if (text_len == capacity) {
            capacity *= 2;
            text = reallocate_or_exit(text, capacity + 1);
        }
    }
    remove_carriage_returns(text, text_len);
    if (input_file != stdin)
        fclose(input_file);
    return text;
}

static char *read_whole_file(const char *file_name)
{
    char *text = read_file_if_present(file_name);

    if (text == NULL) {
        fprintf(stderr, "Error: could not open '%s'\n", file_name);
        exit(2);
    }
    return text;
}

/* Returns the text of the style sheet the page links, or NULL when it links
   none. A linked sheet that cannot be read is reported and also gives NULL,
   because a browser would not apply it either. */
static char *read_linked_style_sheet(const char *html, const char *file_name)
{
    const char *href = strstr(html, STYLE_SHEET_LINK);
    const char *href_end;
    const char *last_slash = strrchr(file_name, '/');
    size_t folder_len = last_slash != NULL
                            ? (size_t)(last_slash - file_name) + 1
                            : 0;
    size_t href_len;
    char *sheet_path;
    char *sheet;

    if (href == NULL)
        return NULL;
    href += strlen(STYLE_SHEET_LINK);
    href_end = strchr(href, '"');
    if (href_end == NULL)
        return NULL;
    href_len = (size_t)(href_end - href);
    if (href[0] == '/')
        folder_len = 0;
    sheet_path = allocate_or_exit(folder_len + href_len + 1);
    memcpy(sheet_path, file_name, folder_len);
    memcpy(sheet_path + folder_len, href, href_len);
    sheet_path[folder_len + href_len] = '\0';
    sheet = read_file_if_present(sheet_path);
    if (sheet == NULL)
        printf("  CSS: could not read the linked style sheet '%s'\n",
               sheet_path);
    free(sheet_path);
    return sheet;
}

/* ------------------------------------------------------ HTML to cell list */

static void append_cell(cell_list *list, char character, const char *class_name,
                        int line_number, int column_number)
{
    cell *new_cell;

    if (list->cell_count == list->cell_capacity) {
        list->cell_capacity =
            list->cell_capacity != 0 ? list->cell_capacity * 2 : 4096;
        list->cells = reallocate_or_exit(list->cells,
                                         list->cell_capacity * sizeof(cell));
    }
    new_cell = &list->cells[list->cell_count];
    new_cell->character = character;
    new_cell->class_name = class_name;
    new_cell->line_number = line_number;
    new_cell->column_number = column_number;
    list->cell_count++;
}

/* Records that a <span class="l"> opened at this line and column. */
static void record_line_span(cell_list *list, int line_number,
                             int column_number)
{
    size_t line_index;

    if ((size_t)line_number >= list->line_capacity) {
        size_t new_capacity =
            list->line_capacity != 0 ? list->line_capacity : 256;

        while ((size_t)line_number >= new_capacity)
            new_capacity *= 2;
        list->line_span_counts = reallocate_or_exit(
            list->line_span_counts, new_capacity * sizeof(int));
        list->line_span_columns = reallocate_or_exit(
            list->line_span_columns, new_capacity * sizeof(int));
        for (line_index = list->line_capacity; line_index < new_capacity;
             line_index++) {
            list->line_span_counts[line_index] = 0;
            list->line_span_columns[line_index] = 0;
        }
        list->line_capacity = new_capacity;
    }
    if (list->line_span_counts[line_number]++ == 0)
        list->line_span_columns[line_number] = column_number;
}

static const char *read_span_class_name(const char *tag, const char *tag_end)
{
    const char *attribute = strstr(tag, "class=\"");
    const char *name_end;

    if (attribute != NULL && attribute < tag_end) {
        attribute += strlen("class=\"");
        name_end = memchr(attribute, '"', (size_t)(tag_end - attribute));
        if (name_end != NULL)
            return intern_class_name(attribute,
                                     (size_t)(name_end - attribute));
    }
    return NO_CLASS;
}

/* A character's class is its innermost span, not counting the line span. */
static const char *find_visible_class(const char *const *open_classes,
                                      int open_count)
{
    int span_index;

    for (span_index = open_count - 1; span_index >= 0; span_index--)
        if (strcmp(open_classes[span_index], "l") != 0)
            return open_classes[span_index];
    return NO_CLASS;
}

static void parse_html(const char *html, const char *file_name,
                       cell_list *list)
{
    const char *content = strstr(html, PRE_OPENING_TAG);
    const char *content_end;
    const char *position;
    const char *open_classes[MAX_OPEN_SPANS];
    int open_count = 0;
    int line_number = 1;
    int column_number = 1;

    if (content == NULL) {
        fprintf(stderr, "Error: '%s' has no %s\n", file_name, PRE_OPENING_TAG);
        exit(2);
    }
    content += strlen(PRE_OPENING_TAG);
    content_end = strstr(content, "</pre>");
    if (content_end == NULL)
        content_end = content + strlen(content);

    position = content;
    while (position < content_end) {
        char character;
        size_t consumed_len = 1;

        if (*position == '<') {
            const char *tag_end =
                memchr(position, '>', (size_t)(content_end - position));

            if (tag_end == NULL)
                break;
            if (strncmp(position, "<span", 5) == 0) {
                const char *class_name =
                    read_span_class_name(position, tag_end);

                if (open_count < MAX_OPEN_SPANS)
                    open_classes[open_count++] = class_name;
                if (strcmp(class_name, "l") == 0)
                    record_line_span(list, line_number, column_number);
            } else if (strncmp(position, "</span", 6) == 0) {
                if (open_count > 0)
                    open_count--;
            }
            position = tag_end + 1;
            continue;
        }

        character = *position;
        if (strncmp(position, "&lt;", 4) == 0) {
            character = '<';
            consumed_len = 4;
        } else if (strncmp(position, "&gt;", 4) == 0) {
            character = '>';
            consumed_len = 4;
        } else if (strncmp(position, "&amp;", 5) == 0) {
            character = '&';
            consumed_len = 5;
        }

        append_cell(list, character,
                    find_visible_class(open_classes, open_count),
                    line_number, column_number);
        if (character == '\n') {
            line_number++;
            column_number = 1;
        } else {
            column_number++;
        }
        position += consumed_len;
    }
    list->line_count = line_number;
}

/* --------------------------------------------------------------- comparison */

static int is_blank_character(char character)
{
    return character == ' ' || character == '\t';
}

static const char *get_class_label(const char *class_name)
{
    return class_name[0] == '\0' ? "(no color)" : class_name;
}

static class_summary *find_class_summary(const char *class_name)
{
    int summary_index;

    for (summary_index = 0; summary_index < class_summary_count;
         summary_index++)
        if (class_summaries[summary_index].class_name == class_name)
            return &class_summaries[summary_index];
    class_summaries[class_summary_count].class_name = class_name;
    return &class_summaries[class_summary_count++];
}

/* Prints one divergence: the cells [first, last) of one line. */
static void print_divergence(const cell_list *expected, size_t first,
                             size_t last, const char *obtained_class)
{
    char excerpt[48];
    size_t excerpt_len = 0;
    size_t cell_index;

    for (cell_index = first; cell_index < last; cell_index++)
        if (excerpt_len < sizeof excerpt - 4)
            excerpt[excerpt_len++] = expected->cells[cell_index].character == '\t'
                                         ? ' '
                                         : expected->cells[cell_index].character;
    excerpt[excerpt_len] = '\0';
    if (last - first > excerpt_len)
        strcat(excerpt, "...");
    printf("  line %3d, column %3d  expected %-10s obtained %-10s  \"%s\"\n",
           expected->cells[first].line_number,
           expected->cells[first].column_number,
           get_class_label(expected->cells[first].class_name),
           get_class_label(obtained_class), excerpt);
}

static int get_line_span_count(const cell_list *list, int line_number)
{
    return (size_t)line_number < list->line_capacity
               ? list->line_span_counts[line_number]
               : 0;
}

/* The number shown comes from a CSS counter: the Nth <span class="l"> shows N.
   So each line needs as many line spans as expected, in the same column.
   Returns the number of problems. */
static int check_line_numbering(const cell_list *expected,
                                const cell_list *obtained)
{
    int line_number;
    int problem_count = 0;
    long expected_total = 0;
    long obtained_total = 0;

    for (line_number = 1; line_number <= expected->line_count; line_number++) {
        int expected_spans = get_line_span_count(expected, line_number);
        int obtained_spans = get_line_span_count(obtained, line_number);
        char expected_label[24];
        char obtained_label[24];

        if (expected_spans != 0)
            sprintf(expected_label, "%ld", expected_total + 1);
        else
            strcpy(expected_label, "none");
        if (obtained_spans != 0)
            sprintf(obtained_label, "%ld", obtained_total + 1);
        else
            strcpy(obtained_label, "none");

        if (expected_spans != obtained_spans ||
            (expected_spans != 0 &&
             expected->line_span_columns[line_number] !=
                 obtained->line_span_columns[line_number])) {
            printf("  line %3d  expected number %-7s obtained number %-7s"
                   " (line spans: expected %d, obtained %d)\n",
                   line_number, expected_label, obtained_label,
                   expected_spans, obtained_spans);
            problem_count++;
        }
        expected_total += expected_spans;
        obtained_total += obtained_spans;
    }
    if (expected_total != obtained_total) {
        printf("  total numbers: expected %ld, obtained %ld"
               " (the following lines are shifted)\n",
               expected_total, obtained_total);
        problem_count++;
    }
    return problem_count;
}

static int has_counter_css(const char *text)
{
    int css_index;

    for (css_index = 0; css_index < REQUIRED_CSS_COUNT; css_index++)
        if (strstr(text, REQUIRED_CSS[css_index]) == NULL)
            return 0;
    return 1;
}

/* Without the counter in the obtained CSS, no number is drawn at all. The CSS
   may be in the page or in its linked style sheet (NULL when there is none). */
static int check_counter_css(const char *expected_html,
                             const char *obtained_html,
                             const char *obtained_sheet)
{
    int css_index;
    int problem_count = 0;

    for (css_index = 0; css_index < REQUIRED_CSS_COUNT; css_index++)
        if (strstr(expected_html, REQUIRED_CSS[css_index]) != NULL &&
            strstr(obtained_html, REQUIRED_CSS[css_index]) == NULL &&
            (obtained_sheet == NULL ||
             strstr(obtained_sheet, REQUIRED_CSS[css_index]) == NULL)) {
            printf("  CSS: '%s' is missing from the obtained HTML"
                   " (the numbering depends on it)\n",
                   REQUIRED_CSS[css_index]);
            problem_count++;
        }
    return problem_count;
}

/* Both files must hold the same text, or their classes cannot be compared.
   Returns 1 when they do. */
static int check_same_text(const cell_list *expected,
                           const cell_list *obtained)
{
    size_t shared_count = expected->cell_count < obtained->cell_count
                              ? expected->cell_count
                              : obtained->cell_count;
    size_t cell_index;

    for (cell_index = 0; cell_index < shared_count; cell_index++) {
        const cell *expected_cell = &expected->cells[cell_index];

        if (expected_cell->character != obtained->cells[cell_index].character) {
            printf("The texts differ (the two HTML files do not come from the"
                   " same .c file).\n"
                   "  first difference: line %d, column %d:"
                   " expected '%c', obtained '%c'\n",
                   expected_cell->line_number, expected_cell->column_number,
                   expected_cell->character,
                   obtained->cells[cell_index].character);
            return 0;
        }
    }
    if (expected->cell_count != obtained->cell_count) {
        printf("The texts differ in size: expected %lu characters,"
               " obtained %lu.\n  (the %s file ends first)\n",
               (unsigned long)expected->cell_count,
               (unsigned long)obtained->cell_count,
               expected->cell_count < obtained->cell_count ? "expected"
                                                           : "obtained");
        return 0;
    }
    return 1;
}

/* Prints a run of same-pair differences as one divergence, and adds every
   visible character to the totals. Returns the number of divergences. */
static int compare_classes(const cell_list *expected, const cell_list *obtained,
                           long *total_count, long *correct_count)
{
    int divergence_count = 0;
    size_t cell_index = 0;

    while (cell_index < expected->cell_count) {
        const cell *expected_cell = &expected->cells[cell_index];
        const char *expected_class = expected_cell->class_name;
        const char *obtained_class = obtained->cells[cell_index].class_name;
        class_summary *summary;
        size_t run_end;
        size_t run_index;

        if (expected_cell->character == '\n' ||
            is_blank_character(expected_cell->character)) {
            cell_index++;
            continue;
        }

        summary = find_class_summary(expected_class);
        summary->total_count++;
        (*total_count)++;

        if (expected_class == obtained_class) {
            summary->correct_count++;
            (*correct_count)++;
            cell_index++;
            continue;
        }

        /* extend the run while the pair of classes holds on this line; blanks
           inside the run do not interrupt it */
        run_end = cell_index + 1;
        run_index = cell_index + 1;
        while (run_index < expected->cell_count &&
               expected->cells[run_index].character != '\n') {
            if (is_blank_character(expected->cells[run_index].character)) {
                run_index++;
                continue;
            }
            if (expected->cells[run_index].class_name != expected_class ||
                obtained->cells[run_index].class_name != obtained_class)
                break;
            summary->total_count++;
            (*total_count)++;
            run_index++;
            run_end = run_index;
        }
        print_divergence(expected, cell_index, run_end, obtained_class);
        divergence_count++;
        cell_index = run_end;
    }
    return divergence_count;
}

static void print_summary(void)
{
    int summary_index;

    printf("\nSummary by expected class (visible characters):\n");
    for (summary_index = 0; summary_index < class_summary_count;
         summary_index++) {
        const class_summary *summary = &class_summaries[summary_index];

        printf("  %-10s %5ld of %5ld correct%s\n",
               get_class_label(summary->class_name), summary->correct_count,
               summary->total_count,
               summary->correct_count == summary->total_count
                   ? ""
                   : "   <-- check");
    }
}

/* Compares the two parsed files, prints the report and returns the exit
   status. */
static int compare_and_report(const cell_list *expected,
                              const cell_list *obtained,
                              const char *expected_html,
                              const char *obtained_html,
                              const char *obtained_file_name)
{
    long total_count = 0;
    long correct_count = 0;
    int divergence_count;
    int numbering_problem_count;
    char *obtained_sheet;

    if (!check_same_text(expected, obtained))
        return 2;

    printf("Divergences (expected x obtained):\n");
    divergence_count =
        compare_classes(expected, obtained, &total_count, &correct_count);
    if (divergence_count == 0)
        printf("  (none)\n");

    printf("\nLine numbering:\n");
    /* a page with its own counter CSS does not need its linked sheet */
    obtained_sheet = has_counter_css(obtained_html)
                         ? NULL
                         : read_linked_style_sheet(obtained_html,
                                                   obtained_file_name);
    numbering_problem_count = check_line_numbering(expected, obtained) +
                              check_counter_css(expected_html, obtained_html,
                                                obtained_sheet);
    free(obtained_sheet);
    if (numbering_problem_count == 0)
        printf("  (none)\n");

    print_summary();

    printf("\nTotal: %ld of %ld characters correct (%.1f%%), %d divergence(s).\n",
           correct_count, total_count,
           total_count != 0
               ? 100.0 * (double)correct_count / (double)total_count
               : 100.0,
           divergence_count);
    printf("Numbering: %d problem(s).\n", numbering_problem_count);
    printf("Result: %s\n", divergence_count == 0 && numbering_problem_count == 0
                               ? "OK, identical to the expected file"
                               : "DIFFERENT from the expected file");
    return divergence_count == 0 && numbering_problem_count == 0 ? 0 : 1;
}

int main(int argument_count, char *arguments[])
{
    char *expected_html;
    char *obtained_html;
    cell_list expected = {0, 0, 0, 0, 0, 0, 0};
    cell_list obtained = {0, 0, 0, 0, 0, 0, 0};
    int exit_status;

    if (argument_count != 3) {
        fprintf(stderr,
                "Usage: %s expected.html obtained.html   ('-' for stdin)\n",
                arguments[0]);
        return 2;
    }

    expected_html = read_whole_file(arguments[1]);
    obtained_html = read_whole_file(arguments[2]);

    parse_html(expected_html, arguments[1], &expected);
    parse_html(obtained_html, arguments[2], &obtained);

    exit_status = compare_and_report(&expected, &obtained, expected_html,
                                     obtained_html, arguments[2]);

    free(expected.cells);
    free(obtained.cells);
    free(expected_html);
    free(obtained_html);
    free(expected.line_span_counts);
    free(expected.line_span_columns);
    free(obtained.line_span_counts);
    free(obtained.line_span_columns);
    return exit_status;
}
