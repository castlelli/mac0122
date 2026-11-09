#include <stdlib.h>
#include <string.h>
#include "html.h"
#include "c_highlighter.h"
#include "text_utils.h"

#define HOME_FILE_NAME "index.html"
#define STYLE_FILE_NAME "style.css"
#define SITE_TITLE "MAC0122"
#define COURSE_NAME "Princípios de Desenvolvimento de Algoritmos"
#define INSTITUTION_LINE "IME-USP · Prof. Dr. Leônidas de Oliveira Brandão"
#define AUTHORS_LINE "Construído por Alunos da Escola Politécnica da Universidade de São Paulo"
#define REPOSITORY_URL "https://github.com/castlelli/mac0122"
/* The "Objetivos" of MAC0122 in the JupiterWeb catalogue, quoted verbatim. */
#define COURSE_OBJECTIVES "Estudo, através de exemplos, da correção, da análise de " \
    "eficiência e do desenvolvimento de algoritmos e de suas estruturas de dados básicas."
#define OBJECTIVES_SOURCE "Fonte: JupiterWeb, catálogo de disciplinas da USP."
/* Content pages and the home page belong to no index. */
#define NO_CURRENT_INDEX (-1)
/* Longer than the longest index page name, "techniques_by_algorithm.html". */
#define MAX_FILE_NAME_LEN 64

/* In the order of enum index_type. */
static const char *const INDEX_FILE_PREFIXES[INDEX_COUNT] = {
    "techniques", "algorithms", "authors", "dates", "concepts", "problems"
};
static const char *const INDEX_TITLES[INDEX_COUNT] = {
    "Técnica", "Algoritmo", "Autores", "Data", "Conceitos", "Problema"
};
static const char *const INDEX_DESCRIPTIONS[INDEX_COUNT] = {
    "Algoritmos agrupados pela técnica de projeto.",
    "Cada algoritmo e a técnica que ele usa.",
    "Algoritmos agrupados por autor.",
    "Algoritmos na ordem das datas de publicação.",
    "Algoritmos agrupados pelos conceitos que usam.",
    "Algoritmos agrupados pelo problema que resolvem."
};

/* In the order of enum page_attribute. */
static const char *const ATTRIBUTE_FILE_SUFFIXES[ATTRIBUTE_COUNT] = {
    "algorithm", "technique", "date", "problem"
};
static const char *const ATTRIBUTE_TITLES[ATTRIBUTE_COUNT] = {
    "Algoritmo", "Técnica", "Data", "Problema"
};
/* Table column classes, so CSS can keep dates on one line. */
static const char *const ATTRIBUTE_CLASS_NAMES[ATTRIBUTE_COUNT] = {
    "algoritmo", "tecnica", "data", "problema"
};

/* One literal per line: a single string literal longer than 509 characters is
   not portable C89. The code colors are chosen for a white background, so every
   page fixes it, and the browser is told the page is light so a dark theme does
   not repaint it. These lines come after the highlighter's in style.css, so the
   later pre.codigo rule wins over its grey background. */
static const char *const SITE_CSS_LINES[] = {
    ":root { color-scheme:only light;\n",
    "        --tinta:#16191d; --secundaria:#545b64; --linha:#d4d9df;\n",
    "        --marinho:#1d3557; --selecao:#d5dfeb; }\n",
    "html { background:#fff; }\n",
    "body { margin:0; background:#fff; color:var(--tinta);\n",
    "       font-family:system-ui, -apple-system, \"Segoe UI\", Roboto,\n",
    "                   \"Helvetica Neue\", Arial, \"Noto Sans\", sans-serif;\n",
    "       font-size:1.0625rem; line-height:1.55;\n",
    "       -webkit-text-size-adjust:100%; text-size-adjust:100%; }\n",
    "::selection { background:var(--selecao); color:var(--tinta); }\n",
    "a { color:var(--marinho); text-decoration-thickness:1px;\n",
    "    text-underline-offset:0.2em; }\n",
    "a:hover { text-decoration-thickness:2px; }\n",
    ":focus-visible { outline:2px solid var(--marinho); outline-offset:2px; }\n",
    "pre.codigo { background:#fff; margin:1.25rem 0 1.5rem; }\n",
    ".pular { position:absolute; left:1rem; top:-4rem; z-index:1;\n",
    "         background:var(--marinho); color:#fff; padding:0.5rem 0.875rem; }\n",
    ".pular:focus { top:0.75rem; }\n",
    ".faixa, .navegacao ul, main, .rodape > div { box-sizing:border-box;\n",
    "         max-width:66rem; margin:0 auto; padding-left:1.5rem; padding-right:1.5rem; }\n",
    ".cabecalho { border-top:6px solid var(--marinho); }\n",
    ".faixa { padding-top:1.75rem; padding-bottom:1.25rem; }\n",
    ".curso { margin:0; font-size:1.625rem; line-height:1.25; font-weight:600;\n",
    "         letter-spacing:-0.01em; text-wrap:balance; }\n",
    ".curso a { color:var(--tinta); text-decoration:none; }\n",
    ".curso a:hover .nome { text-decoration:underline; text-decoration-thickness:1px;\n",
    "         text-underline-offset:0.2em; }\n",
    ".curso .sigla { color:var(--marinho); font-weight:700; margin-right:0.45em; }\n",
    ".instituicao { margin:0.4rem 0 0; color:var(--secundaria); font-size:0.9375rem; }\n",
    ".navegacao { border-top:1px solid var(--linha); border-bottom:1px solid var(--linha); }\n",
    ".navegacao ul { list-style:none; display:flex; flex-wrap:wrap; column-gap:1.75rem;\n",
    "         padding-top:0; padding-bottom:0; }\n",
    ".navegacao a { display:block; padding:0.75rem 0 0.6rem; margin-bottom:-1px;\n",
    "         color:var(--secundaria); text-decoration:none; font-size:0.9375rem;\n",
    "         border-bottom:3px solid transparent; }\n",
    ".navegacao a:hover { color:var(--marinho); }\n",
    ".navegacao a[aria-current] { color:var(--marinho); font-weight:600;\n",
    "         border-bottom-color:var(--marinho); }\n",
    "main { display:block; padding-top:2.25rem; padding-bottom:3.5rem; }\n",
    "h1 { font-size:2rem; line-height:1.2; letter-spacing:-0.015em; font-weight:700;\n",
    "     margin:0 0 1.25rem; text-wrap:balance; }\n",
    "h2 { font-size:1.3125rem; line-height:1.3; font-weight:600; margin:2.5rem 0 0.75rem;\n",
    "     text-wrap:balance; scroll-margin-top:1rem; }\n",
    "main p, main ul { max-width:70ch; }\n",
    ".abertura { font-size:1.1875rem; line-height:1.5; margin:0 0 0.5rem; max-width:60ch; }\n",
    ".fonte { margin:0; color:var(--secundaria); font-size:0.875rem; }\n",
    ".indices, .paginas { list-style:none; padding:0; margin:0; }\n",
    ".indices { display:grid; grid-template-columns:repeat(2, minmax(0, 1fr));\n",
    "         column-gap:2.5rem; max-width:none; }\n",
    ".indices li, .paginas li { border-top:1px solid var(--linha); padding:0.7rem 0; }\n",
    ".indices a { font-weight:600; }\n",
    ".descricao, .meta { color:var(--secundaria); font-size:0.9375rem; }\n",
    ".descricao { display:block; margin-top:0.15rem; }\n",
    ".meta { margin-left:0.6em; }\n",
    ".ordenacao { margin:0 0 1rem; color:var(--secundaria); font-size:0.9375rem; }\n",
    ".ordenacao a, .ordenacao strong { margin-left:0.6em; }\n",
    ".ordenacao strong { color:var(--tinta); font-weight:600; }\n",
    ".chaves { display:flex; align-items:baseline; gap:1.1rem; padding:0.85rem 0 1rem;\n",
    "         border-top:1px solid var(--linha); border-bottom:1px solid var(--linha);\n",
    "         font-size:0.9375rem; }\n",
    ".chaves ul { display:flex; flex-wrap:wrap; gap:0.35rem 1.1rem; flex:1;\n",
    "         margin:0; padding:0; list-style:none; max-width:none; }\n",
    ".chaves a { display:inline-block; max-width:24ch; overflow:hidden;\n",
    "         white-space:nowrap; text-overflow:ellipsis; vertical-align:bottom; }\n",
    ".rotulo { color:var(--secundaria); }\n",
    "h2.chave::first-letter, .chaves a::first-letter { text-transform:uppercase; }\n",
    ".tabela { overflow-x:auto; }\n",
    "table { width:100%; border-collapse:collapse; font-size:0.9375rem;\n",
    "        font-variant-numeric:tabular-nums; }\n",
    "th, td { text-align:left; vertical-align:top; padding:0.55rem 1rem 0.55rem 0;\n",
    "         overflow-wrap:break-word; }\n",
    "thead th { font-weight:600; border-bottom:2px solid var(--marinho); }\n",
    "tbody td { border-bottom:1px solid var(--linha); }\n",
    "th.algoritmo { width:22%; }\n",
    "th.tecnica { width:20%; }\n",
    "th.data { width:7.5rem; }\n",
    "td.algoritmo { min-width:9rem; }\n",
    "td.tecnica { min-width:8rem; }\n",
    "th.data, td.data { white-space:nowrap; }\n",
    "td.problema { min-width:15rem; }\n",
    ".ficha { margin:0 0 2.25rem; border-top:2px solid var(--marinho); }\n",
    ".ficha th, .ficha td { border-bottom:1px solid var(--linha); padding-top:0.6rem;\n",
    "         padding-bottom:0.6rem; }\n",
    ".ficha th { width:8.5rem; font-weight:600; color:var(--secundaria); }\n",
    ".rodape { border-top:1px solid var(--linha); color:var(--secundaria);\n",
    "         font-size:0.875rem; }\n",
    ".rodape > div { display:flex; flex-wrap:wrap; justify-content:space-between;\n",
    "         align-items:flex-start; gap:0.5rem 2rem; padding-top:1.25rem; padding-bottom:2rem; }\n",
    ".rodape p { margin:0 0 0.25rem; }\n",
    ".repositorio a { color:var(--marinho); }\n",
    "@media (max-width:40rem) {\n",
    "  body { font-size:1rem; }\n",
    "  .faixa, .navegacao ul, main, .rodape > div { padding-left:1rem; padding-right:1rem; }\n",
    "  .faixa { padding-top:1.25rem; padding-bottom:1rem; }\n",
    "  .curso { font-size:1.3125rem; }\n",
    "  .navegacao ul { column-gap:1.25rem; }\n",
    "  .navegacao a { padding:0.55rem 0 0.4rem; }\n",
    "  h1 { font-size:1.625rem; }\n",
    "  .abertura { font-size:1.0625rem; }\n",
    "  .indices { grid-template-columns:minmax(0, 1fr); }\n",
    "  .ficha th { width:6.5rem; }\n",
    "  .tabela table, .tabela tbody, .tabela tr, .tabela td { display:block; }\n",
    "  .tabela table { border-top:2px solid var(--marinho); }\n",
    "  .tabela thead { position:absolute; width:1px; height:1px; overflow:hidden;\n",
    "         clip:rect(0 0 0 0); white-space:nowrap; }\n",
    "  .tabela tr { padding:0.65rem 0; border-bottom:1px solid var(--linha); }\n",
    "  .tabela tbody td { border:0; padding:0.1rem 0; min-width:0; white-space:normal; }\n",
    "  .tabela td.algoritmo { font-weight:600; padding-bottom:0.25rem; }\n",
    "  .tabela td[data-label]::before { content:attr(data-label) \": \";\n",
    "         color:var(--secundaria); font-size:0.875rem; }\n",
    "  .tabela td:empty { display:none; }\n",
    "}\n",
    "@media print {\n",
    "  .pular, .navegacao, .chaves, .ordenacao { display:none; }\n",
    "  .cabecalho { border-top-width:2px; }\n",
    "  a { color:inherit; text-decoration:none; }\n",
    "  .tabela { overflow:visible; }\n",
    "  h2 { break-after:avoid; }\n",
    "  pre.codigo { white-space:pre-wrap; break-inside:avoid; }\n",
    "}\n",
    NULL
};

/* What the BST walks of an index page need to write each key. */
typedef struct index_page_context {
    FILE *html_file;
    enum index_type index;
    enum page_attribute attribute;
} index_page_context;

static void write_index_file_name(char *file_name, enum index_type index,
                                  enum page_attribute attribute)
{
    /* Fixed strings from the tables above, shorter than MAX_FILE_NAME_LEN. */
    sprintf(file_name, "%s_by_%s.html", INDEX_FILE_PREFIXES[index],
            ATTRIBUTE_FILE_SUFFIXES[attribute]);
}

static void write_head_start(FILE *html_file)
{
    fputs("<!DOCTYPE html>\n<html lang=\"pt-BR\"><head><meta charset=\"utf-8\">\n"
          "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
          "<title>", html_file);
}

/* Closes the head and opens the body with the skip link, the masthead and the
   index bar. On the home page the course name is the page's h1. current_index
   marks one index in the bar (NO_CURRENT_INDEX for none); its link opens the
   default order, so the mark says "this page" only when that is the order shown. */
static void write_site_header(FILE *html_file, int is_home_page, int current_index,
                              enum page_attribute current_attribute)
{
    char file_name[MAX_FILE_NAME_LEN];
    const char *heading_tag = is_home_page ? "h1" : "p";
    enum page_attribute default_attribute;
    int index;

    fputs("</title>\n", html_file);
    c_highlighter_write_head_styles(html_file, 0);
    fputs("</head>\n<body>\n"
          "<a class=\"pular\" href=\"#conteudo\">Pular para o conteúdo</a>\n"
          "<header class=\"cabecalho\">\n<div class=\"faixa\">\n", html_file);
    fprintf(html_file, "<%s class=\"curso\"><a href=\"" HOME_FILE_NAME "\">"
            "<span class=\"sigla\">" SITE_TITLE "</span> "
            "<span class=\"nome\">" COURSE_NAME "</span></a></%s>\n",
            heading_tag, heading_tag);
    fputs("<p class=\"instituicao\">" INSTITUTION_LINE "</p>\n</div>\n"
          "<nav class=\"navegacao\" aria-label=\"Índices\">\n<ul>\n", html_file);

    fprintf(html_file, "<li><a href=\"" HOME_FILE_NAME "\"%s>Início</a></li>\n",
            is_home_page ? " aria-current=\"page\"" : "");
    for (index = 0; index < INDEX_COUNT; index++) {
        default_attribute = indexes_get_default_attribute((enum index_type)index);
        write_index_file_name(file_name, (enum index_type)index, default_attribute);
        fprintf(html_file, "<li><a href=\"%s\"", file_name);
        if (index == current_index)
            fputs(current_attribute == default_attribute ? " aria-current=\"page\""
                                                         : " aria-current=\"true\"",
                  html_file);
        fprintf(html_file, ">%s</a></li>\n", INDEX_TITLES[index]);
    }
    fputs("</ul>\n</nav>\n</header>\n<main id=\"conteudo\">\n", html_file);
}

static void write_site_footer(FILE *html_file)
{
    fputs("</main>\n<footer class=\"rodape\">\n<div>\n<div class=\"creditos\">\n"
          "<p>" SITE_TITLE " · IME-USP</p>\n"
          "<p>" AUTHORS_LINE "</p>\n</div>\n"
          "<p class=\"repositorio\"><a href=\"" REPOSITORY_URL "\">GitHub</a></p>\n"
          "</div>\n</footer>\n</body></html>\n", html_file);
}

/* The algorithm names a page; a TXT without one shows its base name, which is
   html_name without the extension. */
static void write_page_title(FILE *html_file, const page *page_entry)
{
    size_t name_len;
    size_t position;

    if (page_entry->algorithm != NULL) {
        text_utils_write_html_string(html_file, page_entry->algorithm);
        return;
    }
    name_len = strlen(page_entry->html_name) - strlen(".html");
    for (position = 0; position < name_len; position++)
        text_utils_write_html_character(html_file,
                                        (unsigned char)page_entry->html_name[position]);
}

static void write_page_link(FILE *html_file, const page *page_entry)
{
    fputs("<a href=\"", html_file);
    text_utils_write_url_component(html_file, page_entry->html_name);
    fputs("\">", html_file);
    write_page_title(html_file, page_entry);
    fputs("</a>", html_file);
}

void html_write_style_sheet(FILE *css_file)
{
    size_t line_index;

    c_highlighter_write_css(css_file);
    for (line_index = 0; SITE_CSS_LINES[line_index] != NULL; line_index++)
        fputs(SITE_CSS_LINES[line_index], css_file);
}

/* Technique and date after a page's link on the home page, when present. */
static void write_page_meta(FILE *html_file, const page *page_entry)
{
    if (page_entry->technique == NULL && page_entry->date == NULL)
        return;
    fputs("<span class=\"meta\">", html_file);
    if (page_entry->technique != NULL)
        text_utils_write_html_string(html_file, page_entry->technique);
    if (page_entry->technique != NULL && page_entry->date != NULL)
        fputs(" · ", html_file);
    if (page_entry->date != NULL)
        text_utils_write_html_string(html_file, page_entry->date);
    fputs("</span>", html_file);
}

int html_write_home_page(FILE *html_file, page *const *pages, int page_count)
{
    sorted_list *pages_by_name = sorted_list_create(indexes_compare_pages_by_file_name);
    const sorted_list_node *node;
    char file_name[MAX_FILE_NAME_LEN];
    int index;
    int page_index;

    if (pages_by_name == NULL)
        return 0;
    for (page_index = 0; page_index < page_count; page_index++) {
        if (!sorted_list_insert_unique(pages_by_name, pages[page_index])) {
            sorted_list_free(pages_by_name);
            return 0;
        }
    }

    write_head_start(html_file);
    fputs(SITE_TITLE " · " COURSE_NAME, html_file);
    write_site_header(html_file, 1, NO_CURRENT_INDEX, ATTRIBUTE_ALGORITHM);
    fputs("<p class=\"abertura\">" COURSE_OBJECTIVES "</p>\n"
          "<p class=\"fonte\">" OBJECTIVES_SOURCE "</p>\n"
          "<h2>Índices</h2>\n<ul class=\"indices\">\n", html_file);
    for (index = 0; index < INDEX_COUNT; index++) {
        write_index_file_name(file_name, (enum index_type)index,
                              indexes_get_default_attribute((enum index_type)index));
        fprintf(html_file, "<li><a href=\"%s\">%s</a>"
                "<span class=\"descricao\">%s</span></li>\n",
                file_name, INDEX_TITLES[index], INDEX_DESCRIPTIONS[index]);
    }
    fputs("</ul>\n<h2>Todas as páginas</h2>\n<ul class=\"paginas\">\n", html_file);
    for (node = pages_by_name->first_node; node != NULL; node = node->next) {
        fputs("<li>", html_file);
        write_page_link(html_file, node->page_entry);
        write_page_meta(html_file, node->page_entry);
        fputs("</li>\n", html_file);
    }
    fputs("</ul>\n", html_file);
    write_site_footer(html_file);

    sorted_list_free(pages_by_name);
    return 1;
}

/* The algorithm is always the first column, for the link, so the algorithm
   index needs no special case: only technique, date and problem can be left
   out, as the index's own attribute. */
static int is_extra_column(enum index_type index, int attribute)
{
    return attribute != ATTRIBUTE_ALGORITHM
           && indexes_does_offer_attribute(index, (enum page_attribute)attribute);
}

static void write_table_row(FILE *html_file, const page *page_entry, enum index_type index)
{
    const char *value;
    int attribute;

    fprintf(html_file, "<tr><td class=\"%s\">", ATTRIBUTE_CLASS_NAMES[ATTRIBUTE_ALGORITHM]);
    write_page_link(html_file, page_entry);
    fputs("</td>", html_file);
    for (attribute = 0; attribute < ATTRIBUTE_COUNT; attribute++) {
        if (!is_extra_column(index, attribute))
            continue;
        value = indexes_get_attribute_value(page_entry, (enum page_attribute)attribute);
        /* The label heads the value when phones stack each row. */
        fprintf(html_file, "<td class=\"%s\" data-label=\"%s\">",
                ATTRIBUTE_CLASS_NAMES[attribute], ATTRIBUTE_TITLES[attribute]);
        if (value != NULL)
            text_utils_write_html_string(html_file, value);
        fputs("</td>", html_file);
    }
    fputs("</tr>\n", html_file);
}

static void write_table_head(FILE *html_file, enum index_type index)
{
    int attribute;

    fprintf(html_file, "<div class=\"tabela\">\n<table>\n<thead><tr>"
            "<th scope=\"col\" class=\"%s\">%s</th>",
            ATTRIBUTE_CLASS_NAMES[ATTRIBUTE_ALGORITHM], ATTRIBUTE_TITLES[ATTRIBUTE_ALGORITHM]);
    for (attribute = 0; attribute < ATTRIBUTE_COUNT; attribute++) {
        if (is_extra_column(index, attribute))
            fprintf(html_file, "<th scope=\"col\" class=\"%s\">%s</th>",
                    ATTRIBUTE_CLASS_NAMES[attribute], ATTRIBUTE_TITLES[attribute]);
    }
    fputs("</tr></thead>\n<tbody>\n", html_file);
}

static int write_key_section(const char *key, const char *display_key,
                             const sorted_list *pages, void *context)
{
    const index_page_context *page_context = context;
    FILE *html_file = page_context->html_file;
    sorted_list *pages_in_order;
    const sorted_list_node *node;

    /* The node keeps one list; each sort order lasts only for this table. */
    pages_in_order = sorted_list_create(indexes_get_page_comparator(page_context->attribute));
    if (pages_in_order == NULL)
        return 0;
    for (node = pages->first_node; node != NULL; node = node->next) {
        if (!sorted_list_insert_unique(pages_in_order, node->page_entry)) {
            sorted_list_free(pages_in_order);
            return 0;
        }
    }

    fputs("<h2 class=\"chave\" id=\"", html_file);
    text_utils_write_anchor_id(html_file, key);
    fputs("\">", html_file);
    text_utils_write_html_string(html_file, display_key);
    fputs("</h2>\n", html_file);
    write_table_head(html_file, page_context->index);
    for (node = pages_in_order->first_node; node != NULL; node = node->next)
        write_table_row(html_file, node->page_entry, page_context->index);
    fputs("</tbody>\n</table>\n</div>\n", html_file);

    sorted_list_free(pages_in_order);
    return 1;
}

static int write_key_link(const char *key, const char *display_key,
                          const sorted_list *pages, void *context)
{
    FILE *html_file = context;

    (void)pages;
    fputs("<li><a href=\"#", html_file);
    text_utils_write_anchor_id(html_file, key);
    fputs("\">", html_file);
    text_utils_write_html_string(html_file, display_key);
    fputs("</a></li>\n", html_file);
    return 1;
}

static void write_sort_links(FILE *html_file, enum index_type index,
                             enum page_attribute current_attribute)
{
    char file_name[MAX_FILE_NAME_LEN];
    int attribute;

    fputs("<p class=\"ordenacao\">Ordenar por:", html_file);
    for (attribute = 0; attribute < ATTRIBUTE_COUNT; attribute++) {
        if (!indexes_does_offer_attribute(index, (enum page_attribute)attribute))
            continue;
        if (attribute == (int)current_attribute) {
            fprintf(html_file, " <strong aria-current=\"page\">%s</strong>",
                    ATTRIBUTE_TITLES[attribute]);
        } else {
            write_index_file_name(file_name, index, (enum page_attribute)attribute);
            fprintf(html_file, " <a href=\"%s\">%s</a>", file_name,
                    ATTRIBUTE_TITLES[attribute]);
        }
    }
    fputs("</p>\n", html_file);
}

int html_write_index_page(FILE *html_file, const site_indexes *indexes,
                          enum index_type index, enum page_attribute attribute)
{
    index_page_context page_context;
    const bst *tree = indexes->trees[index];

    page_context.html_file = html_file;
    page_context.index = index;
    page_context.attribute = attribute;

    write_head_start(html_file);
    fprintf(html_file, "%s · " SITE_TITLE, INDEX_TITLES[index]);
    write_site_header(html_file, 0, (int)index, attribute);
    fprintf(html_file, "<h1>%s</h1>\n", INDEX_TITLES[index]);
    write_sort_links(html_file, index, attribute);

    if (tree->root == NULL) {
        fputs("<p>Nenhuma página tem este campo preenchido.</p>\n", html_file);
    } else {
        fputs("<nav class=\"chaves\" aria-label=\"Chaves\">"
              "<span class=\"rotulo\">Chaves:</span>\n<ul>\n", html_file);
        bst_walk_in_order(tree, write_key_link, html_file);
        fputs("</ul>\n</nav>\n", html_file);
        if (!bst_walk_in_order(tree, write_key_section, &page_context))
            return 0;
    }
    write_site_footer(html_file);
    return 1;
}

static void write_text_field(FILE *html_file, const char *title, const char *value)
{
    if (value == NULL)
        return;
    fprintf(html_file, "<tr><th scope=\"row\">%s</th><td>", title);
    text_utils_write_html_string(html_file, value);
    fputs("</td></tr>\n", html_file);
}

/* Writes the values of a multi-valued field, each linked to its key on the
   index page that opens by default. Returns 0 only when memory runs out. */
static int write_linked_field(FILE *html_file, const char *title, char *const *values,
                              int value_count, enum index_type index)
{
    char file_name[MAX_FILE_NAME_LEN];
    char *key;
    int value_index;

    if (value_count == 0)
        return 1;

    write_index_file_name(file_name, index, indexes_get_default_attribute(index));
    fprintf(html_file, "<tr><th scope=\"row\">%s</th><td>", title);
    for (value_index = 0; value_index < value_count; value_index++) {
        key = text_utils_normalize(values[value_index]);
        if (key == NULL)
            return 0;
        if (value_index > 0)
            fputs(", ", html_file);
        fprintf(html_file, "<a href=\"%s#", file_name);
        text_utils_write_anchor_id(html_file, key);
        fputs("\">", html_file);
        text_utils_write_html_string(html_file, values[value_index]);
        fputs("</a>", html_file);
        free(key);
    }
    fputs("</td></tr>\n", html_file);
    return 1;
}

static void write_body_blocks(FILE *html_file, const page_body *body)
{
    const page_block *block;
    int block_index;

    for (block_index = 0; block_index < body->blocks_count; block_index++) {
        block = body->blocks[block_index];
        if (block->type == SECTION) {
            fputs("<h2>", html_file);
            text_utils_write_html_string(html_file, block->text);
            fputs("</h2>\n", html_file);
        } else if (block->type == PARAGRAPH) {
            fputs("<p>", html_file);
            text_utils_write_html_string(html_file, block->text);
            fputs("</p>\n", html_file);
        } else if (block->type == CODE) {
            fputs("<pre class=\"codigo\">", html_file);
            c_highlighter_write_html_from_text(block->text, html_file);
            fputs("</pre>\n", html_file);
        }
    }
}

static void write_references(FILE *html_file, const page_body *body)
{
    int has_reference = 0;
    int block_index;

    for (block_index = 0; block_index < body->blocks_count; block_index++) {
        if (body->blocks[block_index]->type != REFERENCE)
            continue;
        if (!has_reference)
            fputs("<h2>Referências</h2>\n<ul class=\"referencias\">\n", html_file);
        has_reference = 1;
        fputs("<li>", html_file);
        text_utils_write_html_string(html_file, body->blocks[block_index]->text);
        fputs("</li>\n", html_file);
    }
    if (has_reference)
        fputs("</ul>\n", html_file);
}

static int has_header_field(const page *page_header)
{
    return page_header->technique != NULL || page_header->authors_count > 0
           || page_header->date != NULL || page_header->concepts_count > 0
           || page_header->problem != NULL;
}

int html_write_content_page(FILE *html_file, const page *page_header, const page_body *body)
{
    write_head_start(html_file);
    write_page_title(html_file, page_header);
    fputs(" · " SITE_TITLE, html_file);
    write_site_header(html_file, 0, NO_CURRENT_INDEX, ATTRIBUTE_ALGORITHM);
    fputs("<h1>", html_file);
    write_page_title(html_file, page_header);
    fputs("</h1>\n", html_file);

    /* The algorithm is the title, so the table holds the other fields. */
    if (has_header_field(page_header)) {
        fputs("<table class=\"ficha\">\n<tbody>\n", html_file);
        write_text_field(html_file, "Técnica", page_header->technique);
        if (!write_linked_field(html_file, "Autores", page_header->authors,
                                page_header->authors_count, INDEX_AUTHORS))
            return 0;
        write_text_field(html_file, "Data", page_header->date);
        if (!write_linked_field(html_file, "Conceitos", page_header->concepts,
                                page_header->concepts_count, INDEX_CONCEPTS))
            return 0;
        write_text_field(html_file, "Problema", page_header->problem);
        fputs("</tbody>\n</table>\n", html_file);
    }

    write_body_blocks(html_file, body);
    write_references(html_file, body);
    write_site_footer(html_file);
    return 1;
}

/* Returns the opened file, or NULL after reporting why on stderr. */
static FILE *open_output_file(const char *output_directory, const char *file_name)
{
    char *path = malloc(strlen(output_directory) + 1 + strlen(file_name) + 1);
    FILE *output_file;

    if (path == NULL) {
        fprintf(stderr, "%s: out of memory\n", file_name);
        return NULL;
    }
    strcpy(path, output_directory);
    strcat(path, "/");
    strcat(path, file_name);

    output_file = fopen(path, "w");
    if (output_file == NULL)
        perror(path);
    free(path);
    return output_file;
}

/* Closes output_file and reports a write that failed on the way, which is only
   visible here: a full disk may not fail until the buffer is flushed. */
static int close_output_file(FILE *output_file, const char *file_name, int has_written)
{
    int has_failed = !has_written || ferror(output_file);

    if (fclose(output_file) != 0)
        has_failed = 1;
    if (has_failed)
        fprintf(stderr, "%s: could not write the file\n", file_name);
    return !has_failed;
}

static int write_style_file(const char *output_directory)
{
    FILE *css_file = open_output_file(output_directory, STYLE_FILE_NAME);

    if (css_file == NULL)
        return 0;
    html_write_style_sheet(css_file);
    return close_output_file(css_file, STYLE_FILE_NAME, 1);
}

static int write_home_file(const char *output_directory, page *const *pages, int page_count)
{
    FILE *html_file = open_output_file(output_directory, HOME_FILE_NAME);
    int has_written;

    if (html_file == NULL)
        return 0;
    has_written = html_write_home_page(html_file, pages, page_count);
    return close_output_file(html_file, HOME_FILE_NAME, has_written);
}

static int write_index_page_file(const char *output_directory, const site_indexes *indexes,
                                 enum index_type index, enum page_attribute attribute)
{
    char file_name[MAX_FILE_NAME_LEN];
    FILE *html_file;
    int has_written;

    write_index_file_name(file_name, index, attribute);
    html_file = open_output_file(output_directory, file_name);
    if (html_file == NULL)
        return 0;
    has_written = html_write_index_page(html_file, indexes, index, attribute);
    return close_output_file(html_file, file_name, has_written);
}

int html_write_index_files(const char *output_directory, const site_indexes *indexes,
                           page *const *pages, int page_count)
{
    int index;
    int attribute;

    if (!write_style_file(output_directory)
        || !write_home_file(output_directory, pages, page_count))
        return 0;

    for (index = 0; index < INDEX_COUNT; index++) {
        for (attribute = 0; attribute < ATTRIBUTE_COUNT; attribute++) {
            if (indexes_does_offer_attribute((enum index_type)index,
                                             (enum page_attribute)attribute)
                && !write_index_page_file(output_directory, indexes, (enum index_type)index,
                                          (enum page_attribute)attribute))
                return 0;
        }
    }
    return 1;
}

int html_write_content_file(const char *output_directory, const page *page_header,
                            const page_body *body)
{
    FILE *html_file = open_output_file(output_directory, page_header->html_name);
    int has_written;

    if (html_file == NULL)
        return 0;
    has_written = html_write_content_page(html_file, page_header, body);
    return close_output_file(html_file, page_header->html_name, has_written);
}

int html_is_generated_file_name(const char *file_name)
{
    char index_file_name[MAX_FILE_NAME_LEN];
    int index;
    int attribute;

    if (strcmp(file_name, HOME_FILE_NAME) == 0)
        return 1;
    for (index = 0; index < INDEX_COUNT; index++) {
        for (attribute = 0; attribute < ATTRIBUTE_COUNT; attribute++) {
            write_index_file_name(index_file_name, (enum index_type)index,
                                  (enum page_attribute)attribute);
            if (indexes_does_offer_attribute((enum index_type)index,
                                             (enum page_attribute)attribute)
                && strcmp(file_name, index_file_name) == 0)
                return 1;
        }
    }
    return 0;
}
