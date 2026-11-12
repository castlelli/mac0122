#include <stdlib.h>
#include <string.h>
#include "html.h"
#include "c_highlighter.h"
#include "graph.h"
#include "text_utils.h"

#define HOME_FILE_NAME "index.html"
#define STYLE_FILE_NAME "style.css"
#define SITE_TITLE "MAC0122"
#define COURSE_NAME "Princípios de Desenvolvimento de Algoritmos"
#define INSTITUTION_LINE "IME-USP · Prof. Dr. Leônidas de Oliveira Brandão"
#define AUTHORS_LINE "Construído por Alunos da Escola Politécnica da Universidade de São Paulo"
#define REPOSITORY_URL "https://github.com/castlelli/mac0122"
#define LINKEDIN_RODRIGO_URL "https://www.linkedin.com/in/rodrigo-mikaro/"
#define LINKEDIN_VINICIUS_URL "https://www.linkedin.com/in/viniciuscastelli"
/* The "Objetivos" of MAC0122 in the JupiterWeb catalogue, quoted verbatim. */
#define PORTFOLIO_VINICIUS_URL "https://castlelli.github.io/"
#define COURSE_OBJECTIVES "Estudo, através de exemplos, da correção, da análise de " \
    "eficiência e do desenvolvimento de algoritmos e de suas estruturas de dados básicas."
#define OBJECTIVES_SOURCE "Fonte: JupiterWeb, catálogo de disciplinas da USP."
#define ABOUT_FILE_NAME "sobre.html"
#define GRAPH_CHOOSER_FILE_NAME "graph_local.html"
#define LOCAL_FILE_PREFIX "local_"
#define LOCAL_PAGE_FILE_PREFIX "local_page_"
/* Labels longer than this many characters are cut on the drawing; the node's
   tooltip keeps the whole text. */
#define MAX_LABEL_CHARACTERS 32
/* Longer than the longest index page name, "techniques_by_algorithm.html". */
#define MAX_FILE_NAME_LEN 64

/* In the order of enum index_type. */
static const char *const INDEX_FILE_PREFIXES[INDEX_COUNT] = {
    "techniques", "algorithms", "authors", "dates", "concepts", "problems"
};
static const char *const INDEX_TITLES[INDEX_COUNT] = {
    "Técnica", "Algoritmo", "Autores", "Data", "Conceitos", "Problema"
};
/* The order of the "Ordenar por" options: the algorithm, the tab's own index,
   comes first. */
static const enum index_type ORDER_OPTION_INDEXES[INDEX_COUNT] = {
    INDEX_ALGORITHM, INDEX_TECHNIQUE, INDEX_AUTHORS, INDEX_DATE, INDEX_CONCEPTS,
    INDEX_PROBLEM
};

/* The tabs of the bar, in order. Content pages mark none. */
enum site_tab { TAB_NONE = -1, TAB_HOME, TAB_ALGORITHMS, TAB_ABOUT, TAB_COUNT };
static const char *const TAB_TITLES[TAB_COUNT] = { "Início", "Algoritmos", "Sobre" };

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
    ".equipe { list-style:none; padding:0; margin:0; }\n",
    ".equipe li { border-top:1px solid var(--linha); padding:0.7rem 0; }\n",
    ".equipe li:last-child { border-bottom:1px solid var(--linha); }\n",
    ".ordem { display:flex; flex-wrap:wrap; align-items:center; gap:0.5rem 1rem;\n",
    "         margin:0 0 0.75rem; font-size:0.9375rem; }\n",
    ".ordem ul { display:flex; flex-wrap:wrap; list-style:none; margin:0; padding:0;\n",
    "         max-width:none; }\n",
    ".ordem li + li { margin-left:-1px; }\n",
    ".ordem a { position:relative; display:block; padding:0.4rem 0.9rem;\n",
    "         border:1px solid var(--linha); color:var(--tinta); text-decoration:none; }\n",
    ".ordem a:hover { z-index:1; border-color:var(--marinho); color:var(--marinho); }\n",
    ".ordem a[aria-current] { z-index:2; background:var(--marinho);\n",
    "         border-color:var(--marinho); color:#fff; font-weight:600; }\n",
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
    ".ver-grafo { margin:-1.25rem 0 2rem; font-size:0.9375rem; }\n",
    ".ordenacao a[aria-current] { color:var(--tinta); font-weight:600; }\n",
    ".dica { flex-basis:100%; max-width:none; margin:0 0 0.25rem; color:var(--secundaria);\n",
    "        font-size:0.9375rem; }\n",
    ".grafo-vista { display:flex; flex-wrap:wrap; align-items:center;\n",
    "        gap:0.4rem 1.25rem; font-size:0.9375rem; }\n",
    ".grafo-vista > input { position:absolute; width:1px; height:1px; opacity:0; }\n",
    ".grafo-vista > label, .legenda li { display:inline-flex; align-items:center; gap:0.35rem; }\n",
    ".grafo-vista > label { cursor:pointer; }\n",
    ".grafo-vista > label::before { content:\"\"; box-sizing:border-box; width:14px; height:14px;\n",
    "        border:1.5px solid var(--secundaria); border-radius:2px; }\n",
    ".grafo-vista > input:checked + label::before { background:var(--marinho);\n",
    "        border-color:var(--marinho); box-shadow:inset 0 0 0 2px #fff; }\n",
    ".grafo-vista > input:not(:checked) + label { color:var(--secundaria); }\n",
    ".grafo-vista > input:focus-visible + label { outline:2px solid var(--marinho);\n",
    "        outline-offset:2px; }\n",
    ".legenda { display:flex; flex-wrap:wrap; gap:0.4rem 1.25rem; list-style:none;\n",
    "        padding:0; margin:0 0 0.75rem; font-size:0.9375rem; max-width:none; }\n",
    ".simbolo { width:14px; height:14px; flex:none; }\n",
    "main.largo { --margem:max(1.5rem, calc(50cqw - 31.5rem)); container-type:inline-size;\n",
    "        max-width:none; padding-left:0; padding-right:0; }\n",
    "main.largo > * { margin-left:var(--margem); margin-right:var(--margem); }\n",
    "main.largo .tela { flex-basis:calc(100% + 2 * var(--margem));\n",
    "        margin-left:calc(-1 * var(--margem)); margin-right:calc(-1 * var(--margem));\n",
    "        border-left:0; border-right:0; }\n",
    ".tela { flex-basis:100%; overflow:auto; border:1px solid var(--linha); margin-top:0.5rem;\n",
    "        padding:0.5rem; box-sizing:border-box; max-height:80vh; }\n",
    ".grafo { display:block; width:100%; max-width:none; height:auto; }\n",
    ".grafo-vista > .quebra { flex-basis:100%; height:0; }\n",
    ".zoom { color:var(--secundaria); }\n",
    ".grafo-vista > input + label.z::before { content:none; }\n",
    ".grafo-vista > label.z:hover { color:var(--marinho); }\n",
    ".grafo-vista > input:checked + label.z { color:var(--tinta); font-weight:600;\n",
    "        text-decoration:underline; text-decoration-color:var(--marinho);\n",
    "        text-underline-offset:0.3em; }\n",
    ".grafo .aresta { stroke:#b9c0c9; stroke-width:1; }\n",
    ".forma { stroke:#fff; stroke-width:1.5; }\n",
    ".t-pagina .forma { fill:#1d3557; }\n",
    ".t-tecnica .forma { fill:#a6611a; }\n",
    ".t-autores .forma { fill:#7b3294; }\n",
    ".t-data .forma { fill:#6b6b6b; }\n",
    ".t-conceitos .forma { fill:#1a7f6e; }\n",
    ".t-problema .forma { fill:#b2182b; }\n",
    ".grafo text { font-size:14px; fill:var(--tinta); paint-order:stroke; stroke:#fff;\n",
    "        stroke-width:3px; stroke-linejoin:round; pointer-events:none; }\n",
    ".grafo .t-pagina text { font-size:18px; }\n",
    ".grafo.global .no-chave text { opacity:0; }\n",
    ".grafo .no:hover text, .grafo .no:focus text { opacity:1; font-weight:600; }\n",
    ".grafo .no:focus { outline:none; }\n",
    ".grafo .no:focus .forma { stroke:var(--marinho); stroke-width:3; }\n",
    ".tela:has(.no:hover) .aresta, .tela:has(.no:focus) .aresta { stroke-opacity:0.15; }\n",
    ".tela:has(.no:hover) .no, .tela:has(.no:focus) .no { opacity:0.3; }\n",
    ".tela:has(.no:hover) .no:hover, .tela:has(.no:focus) .no:focus { opacity:1; }\n",
    "#f-pagina:not(:checked) ~ .tela :is(.t-pagina, .aresta) { display:none; }\n",
    "#f-tecnica:not(:checked) ~ .tela .t-tecnica { display:none; }\n",
    "#f-autores:not(:checked) ~ .tela .t-autores { display:none; }\n",
    "#f-data:not(:checked) ~ .tela .t-data { display:none; }\n",
    "#f-conceitos:not(:checked) ~ .tela .t-conceitos { display:none; }\n",
    "#f-problema:not(:checked) ~ .tela .t-problema { display:none; }\n",
    ".escolha { display:flex; flex-wrap:wrap; gap:0.35rem 1.25rem; list-style:none;\n",
    "        padding:0; margin:0; max-width:none; }\n",
    ".escolha.longa { display:block; }\n",
    ".escolha.longa li { margin:0 0 0.35rem; }\n",
    ".escolha .chave-nome { display:inline-block; }\n",
    ".escolha .chave-nome::first-letter, h2.foco::first-letter { text-transform:uppercase; }\n",
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
    "  main.largo { --margem:1rem; }\n",
    "  .grafo.global { min-width:44rem; }\n",
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
    "  .pular, .navegacao, .chaves, .ordem, .ordenacao, .dica, .ver-grafo, .zoom { display:none; }\n",
    "  .grafo-vista > input, .grafo-vista > label { display:none; }\n",
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

/* The file a tab opens: Algoritmos opens the algorithm index in its default
   order. */
static void write_tab_file_name(char *file_name, enum site_tab tab)
{
    if (tab == TAB_HOME)
        strcpy(file_name, HOME_FILE_NAME);
    else if (tab == TAB_ALGORITHMS)
        write_index_file_name(file_name, INDEX_ALGORITHM,
                              indexes_get_default_attribute(INDEX_ALGORITHM));
    else
        strcpy(file_name, ABOUT_FILE_NAME);
}

/* Closes the head and opens the body with the skip link, the masthead and the
   tab bar. On the home page the course name is the page's h1. current_tab is
   marked with aria-current: "page" when is_tab_target says this page is the
   file the tab opens, "true" when it is another page of that tab.
   is_wide_main lets the graph span the whole window. */
static void write_site_header(FILE *html_file, int is_home_page, enum site_tab current_tab,
                              int is_tab_target, int is_wide_main)
{
    char file_name[MAX_FILE_NAME_LEN];
    const char *heading_tag = is_home_page ? "h1" : "p";
    int tab;

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
          "<nav class=\"navegacao\" aria-label=\"Seções\">\n<ul>\n", html_file);
    for (tab = 0; tab < TAB_COUNT; tab++) {
        write_tab_file_name(file_name, (enum site_tab)tab);
        fprintf(html_file, "<li><a href=\"%s\"", file_name);
        if (tab == (int)current_tab)
            fputs(is_tab_target ? " aria-current=\"page\"" : " aria-current=\"true\"",
                  html_file);
        fprintf(html_file, ">%s</a></li>\n", TAB_TITLES[tab]);
    }
    fprintf(html_file, "</ul>\n</nav>\n</header>\n<main id=\"conteudo\"%s>\n",
            is_wide_main ? " class=\"largo\"" : "");
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

/* The "Ordenar por" control: one option per index, each opening that index in
   its default order, so choosing Autores groups and orders the pages by
   author. */
static void write_order_options(FILE *html_file, enum index_type current_index,
                                enum page_attribute current_attribute)
{
    const char *current_mark;
    char file_name[MAX_FILE_NAME_LEN];
    enum index_type index;
    int option_index;

    fputs("<nav class=\"ordem\" aria-labelledby=\"ordem-rotulo\">"
          "<span class=\"rotulo\" id=\"ordem-rotulo\">Ordenar por</span>\n<ul>\n", html_file);
    for (option_index = 0; option_index < INDEX_COUNT; option_index++) {
        index = ORDER_OPTION_INDEXES[option_index];
        write_index_file_name(file_name, index, indexes_get_default_attribute(index));
        current_mark = "";
        if (index == current_index)
            current_mark = current_attribute == indexes_get_default_attribute(index)
                           ? " aria-current=\"page\"" : " aria-current=\"true\"";
        fprintf(html_file, "<li><a href=\"%s\"%s>%s</a></li>\n", file_name, current_mark,
                INDEX_TITLES[index]);
    }
    fputs("</ul>\n</nav>\n", html_file);
}

/* The order of the pages inside each group: the attributes the index offers. */
static void write_sort_links(FILE *html_file, enum index_type index,
                             enum page_attribute current_attribute)
{
    char file_name[MAX_FILE_NAME_LEN];
    int attribute;

    fputs("<p class=\"ordenacao\">Ordenar dentro do grupo:", html_file);
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
    fprintf(html_file, "Algoritmos por %s · " SITE_TITLE, INDEX_TITLES[index]);
    write_site_header(html_file, 0, TAB_ALGORITHMS,
                      index == INDEX_ALGORITHM
                      && attribute == indexes_get_default_attribute(INDEX_ALGORITHM), 0);
    fputs("<h1>Algoritmos</h1>\n", html_file);
    write_order_options(html_file, index, attribute);
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

/* Writes one author of the About page. The links are optional (NULL) and sit
   beside the name; the role goes below it. Every argument is a fixed string
   of this file, so none needs the HTML escape. */
static void write_team_member(FILE *html_file, const char *name, const char *linkedin_url,
                              const char *portfolio_url, const char *role)
{
    fprintf(html_file, "<li><strong>%s</strong>", name);
    if (linkedin_url != NULL || portfolio_url != NULL) {
        fputs("<span class=\"meta\">", html_file);
        if (linkedin_url != NULL)
            fprintf(html_file, "<a href=\"%s\">LinkedIn</a>", linkedin_url);
        if (linkedin_url != NULL && portfolio_url != NULL)
            fputs(" · ", html_file);
        if (portfolio_url != NULL)
            fprintf(html_file, "<a href=\"%s\">Portfólio</a>", portfolio_url);
        fputs("</span>", html_file);
    }
    fprintf(html_file, "<span class=\"descricao\">%s</span></li>\n", role);
}

/* One fputs per paragraph keeps every string literal under 509 characters. */
void html_write_about_page(FILE *html_file)
{
    write_head_start(html_file);
    fputs("Sobre · " SITE_TITLE, html_file);
    write_site_header(html_file, 0, TAB_ABOUT, 1, 0);
    fputs("<h1>Sobre</h1>\n", html_file);
    fputs("<p class=\"abertura\">Este site é o resultado da fase 1 do Exercício Programa "
          "de MAC0122, Princípios de Desenvolvimento de Algoritmos, do IME-USP. Ele reúne "
          "o conteúdo da primeira avaliação em páginas de algoritmos, que podem ser "
          "percorridas por técnica, algoritmo, autores, data, conceitos e problema.</p>\n",
          html_file);

    fputs("<h2>O projeto</h2>\n", html_file);
    fputs("<p>Todas as páginas foram geradas por um programa em C89 escrito pelo grupo. "
          "Ele lê arquivos de texto (TXT) com o conteúdo do curso e produz o site em HTML "
          "e CSS, sem JavaScript. O cabeçalho de cada TXT alimenta os seis índices e o "
          "grafo da página inicial, que mostra como páginas e conceitos se "
          "relacionam.</p>\n", html_file);
    fputs("<p>Os trechos de código C passam por um colorizador de sintaxe, também escrito "
          "por nós, que separa diretivas, tipos, comandos, comentários e cadeias de "
          "caracteres. O conteúdo parte do material didático da disciplina, "
          "complementado com livros; as referências aparecem ao fim de cada "
          "página.</p>\n", html_file);

    fputs("<p>O funcionamento dos programas está detalhado em diagramas de blocos no "
          "<a href=\"" REPOSITORY_URL "\">repositório do projeto no GitHub</a>, junto "
          "com o código, os testes e o histórico de mudanças.</p>\n", html_file);

    fputs("<h2>Uso de inteligência artificial</h2>\n", html_file);
    fputs("<p>O uso de ferramentas de IA foi liberado neste EP, e nós as usamos como "
          "apoio: na escrita e na revisão do código, na elaboração de prompts para os "
          "rascunhos dos TXT e na busca de PDFs e páginas de referência. O código passou "
          "por revisão de integrantes do grupo e por testes. Os diálogos com cada "
          "ferramenta e as dificuldades que encontramos estão descritos nos relatórios "
          "entregues.</p>\n", html_file);

    fputs("<h2>Autores</h2>\n<ul class=\"equipe\">\n", html_file);
    write_team_member(html_file, "João Pedro Dantas Oliveira", NULL, NULL,
                      "Processador que transforma os TXT em páginas HTML.");
    write_team_member(html_file, "Rodrigo Francisco Pettinati Mikaro",
                      LINKEDIN_RODRIGO_URL, NULL,
                      "Colorizador de sintaxe, casos de teste e comparador, relatórios, "
                      "prompts para os TXT e coleta das referências.");
    write_team_member(html_file, "Vinicius Lima Xalega", NULL, NULL,
                      "Conteúdo dos arquivos TXT.");
    write_team_member(html_file, "Vinicius Sampaio Castelli de Deus",
                      LINKEDIN_VINICIUS_URL, PORTFOLIO_VINICIUS_URL,
                      "Repositório no GitHub, revisão e correção do código dos colegas, "
                      "contribuições no código, calendário de entregas e apoio nos TXT.");
    fputs("</ul>\n", html_file);
    write_site_footer(html_file);
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
    write_site_header(html_file, 0, TAB_NONE, 0, 0);
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
    fputs("<p class=\"ver-grafo\"><a href=\"" LOCAL_PAGE_FILE_PREFIX, html_file);
    text_utils_write_url_component(html_file, page_header->html_name);
    fputs("\">Ver no grafo</a></p>\n", html_file);

    write_body_blocks(html_file, body);
    write_references(html_file, body);
    write_site_footer(html_file);
    return 1;
}

/* ------------------------------------------------------------- graph */

/* In the order of enum graph_node_type. The class names are Portuguese
   because they are part of the generated pages. */
static const char *const GRAPH_TYPE_CLASSES[GRAPH_NODE_TYPE_COUNT] = {
    "pagina", "tecnica", "autores", "data", "conceitos", "problema"
};
static const char *const GRAPH_TYPE_PLURALS[GRAPH_NODE_TYPE_COUNT] = {
    "Páginas", "Técnicas", "Autores", "Datas", "Conceitos", "Problemas"
};
static const char *const GRAPH_TYPE_SINGULARS[GRAPH_NODE_TYPE_COUNT] = {
    "Página", "Técnica", "Autor", "Data", "Conceito", "Problema"
};

/* The local graph page of a key node: local_<index file prefix>_<ordinal>.html. */
static void write_local_key_file_name(char *file_name, const graph_node *node)
{
    /* Fixed prefixes and an int, shorter than MAX_FILE_NAME_LEN. */
    sprintf(file_name, LOCAL_FILE_PREFIX "%s_%d.html", INDEX_FILE_PREFIXES[node->index],
            node->ordinal);
}

/* Writes the href of a node's local graph page. */
static void write_local_href(FILE *html_file, const graph_node *node)
{
    char file_name[MAX_FILE_NAME_LEN];

    if (node->type == GRAPH_NODE_PAGE) {
        fputs(LOCAL_PAGE_FILE_PREFIX, html_file);
        text_utils_write_url_component(html_file, node->page_entry->html_name);
        return;
    }
    write_local_key_file_name(file_name, node);
    fputs(file_name, html_file);
}

/* Writes the href of the node's real page: the content page, or the key's
   section on its index page in the default order. */
static void write_node_href(FILE *html_file, const graph_node *node)
{
    char file_name[MAX_FILE_NAME_LEN];

    if (node->type == GRAPH_NODE_PAGE) {
        text_utils_write_url_component(html_file, node->page_entry->html_name);
        return;
    }
    write_index_file_name(file_name, node->index, indexes_get_default_attribute(node->index));
    fprintf(html_file, "%s#", file_name);
    text_utils_write_anchor_id(html_file, node->key);
}

/* Writes the node's name as text: the page title or the key as the TXTs spell it. */
static void write_node_name(FILE *html_file, const graph_node *node)
{
    if (node->type == GRAPH_NODE_PAGE)
        write_page_title(html_file, node->page_entry);
    else
        text_utils_write_html_string(html_file, node->display_key);
}

/* Writes at most max_characters UTF-8 characters of text, escaped, with an
   ellipsis when it was cut. */
static void write_cut_text(FILE *html_file, const char *text, int max_characters)
{
    const unsigned char *cursor = (const unsigned char *)text;
    int character_count = 0;

    while (*cursor != '\0') {
        /* A byte 10xxxxxx continues the character before it. */
        if ((*cursor & 0xC0) != 0x80) {
            if (character_count == max_characters) {
                fputs("…", html_file);
                return;
            }
            character_count++;
        }
        text_utils_write_html_character(html_file, *cursor);
        cursor++;
    }
}

static void write_node_label(FILE *html_file, const graph_node *node)
{
    if (node->type == GRAPH_NODE_PAGE && node->page_entry->algorithm == NULL)
        write_page_title(html_file, node->page_entry);
    else if (node->type == GRAPH_NODE_PAGE)
        write_cut_text(html_file, node->page_entry->algorithm, MAX_LABEL_CHARACTERS);
    else
        write_cut_text(html_file, node->display_key, MAX_LABEL_CHARACTERS);
}

/* Radius of a node: bigger with more links, as in Obsidian. */
static long get_node_radius(const graph_node *node)
{
    long radius = (node->type == GRAPH_NODE_PAGE ? 6 : 4)
                  + (long)(1.6 * graph_square_root((double)node->degree) + 0.5);

    return radius > 18 ? 18 : radius;
}

/* Writes the node's shape centred on (x, y): a circle for pages, and a
   square, triangle, plus sign, diamond or hexagon for the key types. */
static void write_node_shape(FILE *html_file, enum graph_node_type type, long x, long y,
                             long radius)
{
    /* Shapes other than the circle are drawn slightly larger, so their area
       matches a circle of the same radius. */
    long size = radius * 9 / 8;
    long half = size / 2;
    long arm = size / 3 > 1 ? size / 3 : 1;
    long height = size * 87 / 100;

    switch (type) {
    case GRAPH_NODE_TECHNIQUE:
        fprintf(html_file, "<rect class=\"forma\" x=\"%ld\" y=\"%ld\" width=\"%ld\" height=\"%ld\"/>",
                x - radius * 7 / 8, y - radius * 7 / 8, radius * 7 / 4, radius * 7 / 4);
        break;
    case GRAPH_NODE_AUTHORS:
        fprintf(html_file, "<polygon class=\"forma\" points=\"%ld,%ld %ld,%ld %ld,%ld\"/>",
                x, y - size, x + height, y + half, x - height, y + half);
        break;
    case GRAPH_NODE_DATE:
        fprintf(html_file, "<polygon class=\"forma\" points=\"%ld,%ld %ld,%ld %ld,%ld %ld,%ld "
                "%ld,%ld %ld,%ld %ld,%ld %ld,%ld %ld,%ld %ld,%ld %ld,%ld %ld,%ld\"/>",
                x - arm, y - size, x + arm, y - size, x + arm, y - arm, x + size, y - arm,
                x + size, y + arm, x + arm, y + arm, x + arm, y + size, x - arm, y + size,
                x - arm, y + arm, x - size, y + arm, x - size, y - arm, x - arm, y - arm);
        break;
    case GRAPH_NODE_CONCEPTS:
        fprintf(html_file, "<polygon class=\"forma\" points=\"%ld,%ld %ld,%ld %ld,%ld %ld,%ld\"/>",
                x, y - size, x + size, y, x, y + size, x - size, y);
        break;
    case GRAPH_NODE_PROBLEM:
        fprintf(html_file, "<polygon class=\"forma\" points=\"%ld,%ld %ld,%ld %ld,%ld %ld,%ld "
                "%ld,%ld %ld,%ld\"/>",
                x + size, y, x + half, y + height, x - half, y + height, x - size, y,
                x - half, y - height, x + half, y - height);
        break;
    default:
        fprintf(html_file, "<circle class=\"forma\" cx=\"%ld\" cy=\"%ld\" r=\"%ld\"/>",
                x, y, radius);
        break;
    }
}

/* A 14px copy of a type's shape, for filters and legends. */
static void write_type_symbol(FILE *html_file, enum graph_node_type type)
{
    fprintf(html_file, "<svg class=\"simbolo t-%s\" viewBox=\"0 0 16 16\" aria-hidden=\"true\">",
            GRAPH_TYPE_CLASSES[type]);
    write_node_shape(html_file, type, 8, 8, 6);
    fputs("</svg>", html_file);
}

/* Zoom levels besides "Ajustar", which fits the drawing to the page width:
   percentages of its size in drawing units, where key labels are 14px. */
static const int ZOOM_PERCENTS[] = { 100, 150, 200 };
#define ZOOM_LEVEL_COUNT 3

/* Writes "Zoom: Ajustar 100% 150% 200%" as radio buttons on a line of their
   own, fit chosen. They must be siblings that come before the drawing, for the CSS that sizes it. */
static void write_zoom_controls(FILE *html_file)
{
    int zoom_index;

    fputs("<span class=\"quebra\"></span><span class=\"zoom\">Zoom:</span>\n<input type=\"radio\" name=\"zoom\" "
          "id=\"z-ajustar\" checked><label for=\"z-ajustar\" class=\"z\">Ajustar</label>\n",
          html_file);
    for (zoom_index = 0; zoom_index < ZOOM_LEVEL_COUNT; zoom_index++)
        fprintf(html_file, "<input type=\"radio\" name=\"zoom\" id=\"z-%d\"><label for=\"z-%d\" "
                "class=\"z\">%d%%</label>\n", ZOOM_PERCENTS[zoom_index],
                ZOOM_PERCENTS[zoom_index], ZOOM_PERCENTS[zoom_index]);
}

/* One rule per node: while it is hovered or focused, its edges turn navy and
   its neighbours and their labels stay visible. Generated, because CSS
   cannot follow an edge on its own. */
static void write_highlight_rules(FILE *html_file, const graph *drawn_graph)
{
    int zoom_index;
    int node_id;
    int position;
    int start;
    int end;

    fputs("<style>\n", html_file);
    /* Zoom levels are sizes of this drawing, so they are written with it. */
    for (zoom_index = 0; zoom_index < ZOOM_LEVEL_COUNT; zoom_index++)
        fprintf(html_file, "#z-%d:checked ~ .tela .grafo { width:%ldpx; }\n",
                ZOOM_PERCENTS[zoom_index], drawn_graph->width * ZOOM_PERCENTS[zoom_index] / 100);
    for (node_id = 0; node_id < drawn_graph->node_count; node_id++) {
        start = drawn_graph->neighbour_start[node_id];
        end = drawn_graph->neighbour_start[node_id + 1];
        if (start == end)
            continue;
        fprintf(html_file, ".tela:has(#n%d:hover, #n%d:focus) .a%d { stroke:#1d3557;"
                " stroke-opacity:1; stroke-width:1.5; }\n", node_id, node_id, node_id);
        fprintf(html_file, ".tela:has(#n%d:hover, #n%d:focus) :is(", node_id, node_id);
        for (position = start; position < end; position++)
            fprintf(html_file, "%s#n%d", position == start ? "" : ", ",
                    drawn_graph->neighbours[position]);
        fputs(") { opacity:1; }\n", html_file);
        fprintf(html_file, ".tela:has(#n%d:hover, #n%d:focus) :is(", node_id, node_id);
        for (position = start; position < end; position++)
            fprintf(html_file, "%s#n%d text", position == start ? "" : ", ",
                    drawn_graph->neighbours[position]);
        fputs(") { opacity:1; }\n", html_file);
    }
    fputs("</style>\n", html_file);
}

/* Writes the drawing. On a local graph, labels on the left half point left,
   so they grow away from the centre. */
static void write_graph_svg(FILE *html_file, const graph *drawn_graph, int is_global)
{
    const graph_node *node;
    const graph_node *key_node;
    long radius;
    long centre_x = drawn_graph->width / 2;
    int is_label_left;
    int node_id;
    int edge_index;

    fprintf(html_file, "<div class=\"tela\">\n<svg class=\"grafo %s\" viewBox=\"0 0 %ld %ld\" "
            "width=\"%ld\" height=\"%ld\" role=\"group\" aria-label=\"Grafo\">\n",
            is_global ? "global" : "local", drawn_graph->width, drawn_graph->height,
            drawn_graph->width, drawn_graph->height);
    write_highlight_rules(html_file, drawn_graph);

    fputs("<g class=\"arestas\">\n", html_file);
    for (edge_index = 0; edge_index < drawn_graph->edge_count; edge_index++) {
        node = &drawn_graph->nodes[drawn_graph->edges[edge_index].page_node];
        key_node = &drawn_graph->nodes[drawn_graph->edges[edge_index].key_node];
        fprintf(html_file, "<line class=\"aresta t-%s a%d a%d\" x1=\"%ld\" y1=\"%ld\" "
                "x2=\"%ld\" y2=\"%ld\"/>\n", GRAPH_TYPE_CLASSES[key_node->type],
                drawn_graph->edges[edge_index].page_node, drawn_graph->edges[edge_index].key_node,
                node->x, node->y, key_node->x, key_node->y);
    }
    fputs("</g>\n<g class=\"nos\">\n", html_file);

    for (node_id = 0; node_id < drawn_graph->node_count; node_id++) {
        node = &drawn_graph->nodes[node_id];
        radius = get_node_radius(node);
        if (drawn_graph->centre == node_id)
            radius += 4;
        is_label_left = !is_global && node->x < centre_x;
        fprintf(html_file, "<a id=\"n%d\" class=\"no t-%s%s\" href=\"", node_id,
                GRAPH_TYPE_CLASSES[node->type],
                node->type == GRAPH_NODE_PAGE ? "" : " no-chave");
        write_node_href(html_file, node);
        fputs("\"><title>", html_file);
        write_node_name(html_file, node);
        fprintf(html_file, " (%s)</title>", GRAPH_TYPE_SINGULARS[node->type]);
        write_node_shape(html_file, node->type, node->x, node->y, radius);
        fprintf(html_file, "<text x=\"%ld\" y=\"%ld\"%s>",
                is_label_left ? node->x - radius - 6 : node->x + radius + 6,
                node->y + (node->type == GRAPH_NODE_PAGE ? 6 : 5),
                is_label_left ? " text-anchor=\"end\"" : "");
        write_node_label(html_file, node);
        fputs("</text></a>\n", html_file);
    }
    fputs("</g>\n</svg>\n</div>\n", html_file);
}

/* Writes "Vista: Global Local", marking the current view. */
static void write_view_links(FILE *html_file, int is_global_view, int is_chooser)
{
    fputs("<p class=\"ordenacao\">Vista:", html_file);
    if (is_global_view)
        fputs(" <strong aria-current=\"page\">Global</strong>", html_file);
    else
        fputs(" <a href=\"" HOME_FILE_NAME "\">Global</a>", html_file);
    if (is_chooser)
        fputs(" <strong aria-current=\"page\">Local</strong>", html_file);
    else if (is_global_view)
        fputs(" <a href=\"" GRAPH_CHOOSER_FILE_NAME "\">Local</a>", html_file);
    else
        fputs(" <a href=\"" GRAPH_CHOOSER_FILE_NAME "\" aria-current=\"true\">Local</a>",
              html_file);
    fputs("</p>\n", html_file);
}

void html_write_home_page(FILE *html_file, const graph *site_graph)
{
    int type;

    write_head_start(html_file);
    fputs(SITE_TITLE " · " COURSE_NAME, html_file);
    write_site_header(html_file, 1, TAB_HOME, 1, 1);
    fputs("<p class=\"abertura\">" COURSE_OBJECTIVES "</p>\n"
          "<p class=\"fonte\">" OBJECTIVES_SOURCE "</p>\n<h2>Grafo</h2>\n", html_file);
    write_view_links(html_file, 1, 0);
    fputs("<div class=\"grafo-vista\">\n<p class=\"dica\">Passe o cursor sobre um nó para ver "
          "as suas ligações; clique para abrir.</p>\n", html_file);
    /* The checkboxes must come before the drawing, as its siblings, for the
       CSS that hides each type. Data starts hidden: dates seldom join pages. */
    for (type = 0; type < GRAPH_NODE_TYPE_COUNT; type++) {
        fprintf(html_file, "<input type=\"checkbox\" id=\"f-%s\"%s>"
                "<label for=\"f-%s\">", GRAPH_TYPE_CLASSES[type],
                type == GRAPH_NODE_DATE ? "" : " checked", GRAPH_TYPE_CLASSES[type]);
        write_type_symbol(html_file, (enum graph_node_type)type);
        fprintf(html_file, "%s</label>\n", GRAPH_TYPE_PLURALS[type]);
    }
    write_zoom_controls(html_file);
    write_graph_svg(html_file, site_graph, 1);
    fputs("</div>\n", html_file);
    write_site_footer(html_file);
}

void html_write_graph_chooser_page(FILE *html_file, const graph *site_graph)
{
    const graph_node *node;
    int type;
    int node_id;
    int has_type;

    write_head_start(html_file);
    fputs("Grafo local · " SITE_TITLE, html_file);
    write_site_header(html_file, 0, TAB_HOME, 0, 0);
    fputs("<h1>Grafo</h1>\n", html_file);
    write_view_links(html_file, 0, 1);
    fputs("<p>Escolha uma página ou chave para ver o seu grafo local: ela no centro, as "
          "suas ligações em volta e, no anel de fora, o que se liga a elas.</p>\n", html_file);
    for (type = 0; type < GRAPH_NODE_TYPE_COUNT; type++) {
        has_type = 0;
        for (node_id = 0; node_id < site_graph->node_count; node_id++) {
            node = &site_graph->nodes[node_id];
            if ((int)node->type != type)
                continue;
            if (!has_type) {
                fprintf(html_file, "<h2>%s</h2>\n<ul class=\"escolha%s\">\n",
                        GRAPH_TYPE_PLURALS[type], type == GRAPH_NODE_PROBLEM ? " longa" : "");
                has_type = 1;
            }
            fputs("<li><a href=\"", html_file);
            write_local_href(html_file, node);
            fputs(type == GRAPH_NODE_PAGE ? "\">" : "\" class=\"chave-nome\">", html_file);
            write_node_name(html_file, node);
            fputs("</a></li>\n", html_file);
        }
        if (has_type)
            fputs("</ul>\n", html_file);
    }
    write_site_footer(html_file);
}

void html_write_local_graph_page(FILE *html_file, const graph *local_graph)
{
    const graph_node *centre = &local_graph->nodes[local_graph->centre];
    char present[GRAPH_NODE_TYPE_COUNT];
    int type;
    int node_id;

    write_head_start(html_file);
    write_node_name(html_file, centre);
    fputs(" · Grafo · " SITE_TITLE, html_file);
    write_site_header(html_file, 0, TAB_HOME, 0, 1);
    fputs("<h1>Grafo</h1>\n", html_file);
    write_view_links(html_file, 0, 0);
    fprintf(html_file, "<h2 class=\"foco%s\">", centre->type == GRAPH_NODE_PAGE ? "" : " chave");
    write_node_name(html_file, centre);
    fprintf(html_file, "</h2>\n<p class=\"fonte\">%s · <a href=\"",
            GRAPH_TYPE_SINGULARS[centre->type]);
    write_node_href(html_file, centre);
    fprintf(html_file, "\">%s</a></p>\n",
            centre->type == GRAPH_NODE_PAGE ? "Abrir a página" : "Ver no índice");

    /* Legend of the types this graph holds. */
    memset(present, 0, sizeof(present));
    for (node_id = 0; node_id < local_graph->node_count; node_id++)
        present[local_graph->nodes[node_id].type] = 1;
    fputs("<ul class=\"legenda\">\n", html_file);
    for (type = 0; type < GRAPH_NODE_TYPE_COUNT; type++) {
        if (!present[type])
            continue;
        fputs("<li>", html_file);
        write_type_symbol(html_file, (enum graph_node_type)type);
        fprintf(html_file, "%s</li>\n", GRAPH_TYPE_PLURALS[type]);
    }
    fputs("</ul>\n<div class=\"grafo-vista\">\n", html_file);
    write_zoom_controls(html_file);
    write_graph_svg(html_file, local_graph, 0);
    fputs("</div>\n", html_file);
    write_site_footer(html_file);
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

static int write_about_file(const char *output_directory)
{
    FILE *html_file = open_output_file(output_directory, ABOUT_FILE_NAME);

    if (html_file == NULL)
        return 0;
    html_write_about_page(html_file);
    return close_output_file(html_file, ABOUT_FILE_NAME, 1);
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

int html_write_index_files(const char *output_directory, const site_indexes *indexes)
{
    int index;
    int attribute;

    if (!write_style_file(output_directory) || !write_about_file(output_directory))
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

/* Builds, writes and frees the local graph of one node. */
static int write_local_graph_file(const char *output_directory, const graph *site_graph,
                                  int node_id)
{
    const graph_node *node = &site_graph->nodes[node_id];
    char file_name[MAX_FILE_NAME_LEN];
    char *page_file_name = NULL;
    const char *name;
    graph *local_graph;
    FILE *html_file;
    int has_written;

    if (node->type == GRAPH_NODE_PAGE) {
        page_file_name = malloc(strlen(LOCAL_PAGE_FILE_PREFIX)
                                + strlen(node->page_entry->html_name) + 1);
        if (page_file_name == NULL) {
            fprintf(stderr, "%s: out of memory\n", node->page_entry->html_name);
            return 0;
        }
        strcpy(page_file_name, LOCAL_PAGE_FILE_PREFIX);
        strcat(page_file_name, node->page_entry->html_name);
        name = page_file_name;
    } else {
        write_local_key_file_name(file_name, node);
        name = file_name;
    }

    local_graph = graph_build_local(site_graph, node_id);
    html_file = local_graph == NULL ? NULL : open_output_file(output_directory, name);
    if (local_graph == NULL)
        fprintf(stderr, "%s: out of memory\n", name);
    has_written = html_file != NULL;
    if (html_file != NULL) {
        html_write_local_graph_page(html_file, local_graph);
        has_written = close_output_file(html_file, name, 1);
    }
    graph_free(local_graph);
    free(page_file_name);
    return has_written;
}

int html_write_graph_files(const char *output_directory, const graph *site_graph)
{
    FILE *html_file;
    int node_id;

    html_file = open_output_file(output_directory, HOME_FILE_NAME);
    if (html_file == NULL)
        return 0;
    html_write_home_page(html_file, site_graph);
    if (!close_output_file(html_file, HOME_FILE_NAME, 1))
        return 0;

    html_file = open_output_file(output_directory, GRAPH_CHOOSER_FILE_NAME);
    if (html_file == NULL)
        return 0;
    html_write_graph_chooser_page(html_file, site_graph);
    if (!close_output_file(html_file, GRAPH_CHOOSER_FILE_NAME, 1))
        return 0;

    for (node_id = 0; node_id < site_graph->node_count; node_id++) {
        if (!write_local_graph_file(output_directory, site_graph, node_id))
            return 0;
    }
    return 1;
}

int html_is_generated_file_name(const char *file_name)
{
    char index_file_name[MAX_FILE_NAME_LEN];
    int index;
    int attribute;

    if (strcmp(file_name, HOME_FILE_NAME) == 0 || strcmp(file_name, ABOUT_FILE_NAME) == 0
        || strcmp(file_name, GRAPH_CHOOSER_FILE_NAME) == 0
        || strncmp(file_name, LOCAL_FILE_PREFIX, strlen(LOCAL_FILE_PREFIX)) == 0)
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
