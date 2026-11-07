/* Tests for html. Prints only the failing cases; exits 0 when every case passes.
   Build and run from the repository root, so the fixture paths resolve:
   gcc -std=c89 -Wall -Wextra -pedantic -Werror tests/test_html.c src/html.c src/graph.c src/indexes.c src/bst.c src/sorted_list.c src/parser.c src/page.c src/text_utils.c src/c_highlighter.c src/c_keywords.c -o test_html */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/html.h"
#include "../src/parser.h"
#include "../src/text_utils.h"

#define MAX_OUTPUT_LEN 32768
#define PAGE_COUNT 4

static int failure_count = 0;
static page *site_pages[PAGE_COUNT];
static site_indexes *indexes;
static graph *site_graph;

static void check_true(const char *case_name, int condition)
{
    if (!condition) {
        printf("FAIL %s\n", case_name);
        failure_count++;
    }
}

static void check_contains(const char *case_name, const char *output, const char *expected)
{
    if (strstr(output, expected) == NULL) {
        printf("FAIL %s: [%s] not found\n", case_name, expected);
        failure_count++;
    }
}

static void check_absent(const char *case_name, const char *output, const char *text)
{
    if (strstr(output, text) != NULL) {
        printf("FAIL %s: [%s] found\n", case_name, text);
        failure_count++;
    }
}

/* first must appear in output before second. */
static void check_order(const char *case_name, const char *output, const char *first,
                        const char *second)
{
    const char *first_position = strstr(output, first);
    const char *second_position = strstr(output, second);

    if (first_position == NULL || second_position == NULL
        || first_position >= second_position) {
        printf("FAIL %s: [%s] not before [%s]\n", case_name, first, second);
        failure_count++;
    }
}

static FILE *open_output(void)
{
    FILE *output_file = tmpfile();

    if (output_file == NULL) {
        printf("FAIL could not open a temporary file\n");
        exit(1);
    }
    return output_file;
}

/* Reads output_file back into output and closes it. */
static void read_output(FILE *output_file, char *output)
{
    size_t output_len;

    rewind(output_file);
    output_len = fread(output, 1, MAX_OUTPUT_LEN - 1, output_file);
    output[output_len] = '\0';
    fclose(output_file);
}

static void write_index_page(enum index_type index, enum page_attribute attribute, char *output)
{
    FILE *output_file = open_output();

    check_true("index page written", html_write_index_page(output_file, indexes, index, attribute));
    read_output(output_file, output);
}

static void test_home_page(void)
{
    static char output[MAX_OUTPUT_LEN];
    FILE *output_file = open_output();

    check_true("home page written", html_write_home_page(output_file, site_pages, PAGE_COUNT));
    read_output(output_file, output);

    check_contains("home: charset", output, "<meta charset=\"utf-8\">");
    check_contains("home: language", output, "<html lang=\"pt-BR\">");
    check_contains("home: shared style", output, "href=\"style.css\"");
    check_absent("home: no page style block", output, "<style>");
    check_contains("home: course name is the h1", output,
                   "<h1 class=\"curso\"><a href=\"index.html\"><span class=\"sigla\">MAC0122</span> "
                   "<span class=\"nome\">Princ\xC3\xADpios de Desenvolvimento de Algoritmos</span></a></h1>");
    check_contains("home: institute and professor", output,
                   "IME-USP \xC2\xB7 Prof. Dr. Le\xC3\xB4nidas de Oliveira Brand\xC3\xA3o");
    check_contains("home: skip link", output, "<a class=\"pular\" href=\"#conteudo\">");
    check_contains("home: main landmark", output, "<main id=\"conteudo\">");
    check_contains("home: Inicio is current", output,
                   "<a href=\"index.html\" aria-current=\"page\">In\xC3\xAD" "cio</a>");
    check_contains("home: objectives quoted", output,
                   "<p class=\"abertura\">Estudo, atrav\xC3\xA9s de exemplos,");
    check_contains("home: objectives source under the quote", output,
                   "<p class=\"fonte\">Fonte: JupiterWeb, cat\xC3\xA1logo de disciplinas da USP.</p>");
    check_contains("home: index description", output,
                   "<span class=\"descricao\">Algoritmos agrupados por autor.</span>");
    check_contains("home: technique and date after a page",
                   output, "<span class=\"meta\">M\xC3\xA9todo guloso \xC2\xB7 1956</span>");
    check_contains("home: footer", output, "<p>MAC0122 \xC2\xB7 IME-USP</p>");
    check_contains("home: footer credits the students", output,
                   "<p>Constru\xC3\xAD" "do por Alunos da Escola Polit\xC3\xA9" "cnica "
                   "da Universidade de S\xC3\xA3o Paulo</p>");
    check_contains("home: footer links the repository", output,
                   "<p class=\"repositorio\"><a href=\"https://github.com/castlelli/mac0122\">GitHub</a></p>");
    check_order("home: repository link after the credits", output,
                "class=\"creditos\"", "class=\"repositorio\"");
    check_contains("home: viewport", output, "name=\"viewport\"");
    check_contains("home: index by algorithm",
                   output, "<a href=\"concepts_by_algorithm.html\">Conceitos</a>");
    check_contains("home: algorithm index by technique",
                   output, "<a href=\"algorithms_by_technique.html\">Algoritmo</a>");
    check_contains("home: page link", output, "<a href=\"kruskal.html\">Kruskal</a>");
    check_contains("home: no Algoritmo uses the base name",
                   output, "<a href=\"sem_campos.html\">sem_campos</a>");
    check_order("home: pages by file name", output, "dijkstra.html", "kruskal.html");
    check_order("home: pages by file name", output, "prim.html", "sem_campos.html");
    check_absent("home: no script", output, "<script");
}

static void test_index_page(void)
{
    static char output[MAX_OUTPUT_LEN];

    write_index_page(INDEX_CONCEPTS, ATTRIBUTE_DATE, output);
    check_contains("index: title", output, "<h1>Conceitos</h1>");
    check_contains("index: masthead links home", output,
                   "<p class=\"curso\"><a href=\"index.html\">");
    check_contains("index: not the default order, so the bar marks the index only",
                   output, "<a href=\"concepts_by_algorithm.html\" aria-current=\"true\">Conceitos</a>");
    check_absent("index: home not current", output, "aria-current=\"page\">In\xC3\xAD" "cio");
    check_contains("index: current order is not a link",
                   output, " <strong aria-current=\"page\">Data</strong>");
    check_absent("index: no objectives source", output, "JupiterWeb");
    check_contains("index: key list", output, "<nav class=\"chaves\" aria-label=\"Chaves\">");
    check_contains("index: key list links its anchor",
                   output, "<li><a href=\"#arvore\">\xC3\x81rvore</a></li>");
    check_contains("index: key list escapes the key", output,
                   "<li><a href=\"#caminho_20_3Cminimo_3E_20_26_20custo\">caminho &lt;m\xC3\xADnimo&gt; &amp; custo</a></li>");
    check_order("index: key list before the tables", output, "class=\"chaves\"", "id=\"arvore\"");
    check_contains("index: table is wrapped for scrolling", output, "<div class=\"tabela\">\n<table>");
    check_contains("index: cells carry their label for phones", output,
                   "<td class=\"data\" data-label=\"Data\">12/03/1957</td>");
    check_contains("index: other orders are links",
                   output, "<a href=\"concepts_by_problem.html\">Problema</a>");
    /* kruskal.txt writes "\xC3\x81rvore" and prim.txt "arvore": the first file wins. */
    check_contains("index: key shown as the first TXT spells it",
                   output, "<h2 class=\"chave\" id=\"arvore\">\xC3\x81rvore</h2>");
    check_contains("index: lowercase spelling kept for CSS to capitalize",
                   output, "<h2 class=\"chave\" id=\"grafo\">grafo</h2>");
    check_order("index: keys in order", output, "id=\"arvore\"", "id=\"grafo\"");
    check_contains("index: key is escaped, accents kept",
                   output, ">caminho &lt;m\xC3\xADnimo&gt; &amp; custo</h2>");
    check_contains("index: columns",
                   output, "<th scope=\"col\" class=\"algoritmo\">Algoritmo</th><th scope=\"col\" class=\"tecnica\">T\xC3\xA9" "cnica</th><th scope=\"col\" class=\"data\">Data</th><th scope=\"col\" class=\"problema\">Problema</th>");

    /* Under grafo: 01/01/1956 is after 1956, then 1957, and no date comes last. */
    check_order("by date: year before full date", strstr(output, "id=\"grafo\""),
                "kruskal.html", "dijkstra.html");
    check_order("by date: later year after", strstr(output, "id=\"grafo\""),
                "dijkstra.html", "prim.html");
    check_order("by date: empty value last", strstr(output, "id=\"grafo\""),
                "prim.html", "sem_campos.html");

    write_index_page(INDEX_CONCEPTS, ATTRIBUTE_TECHNIQUE, output);
    check_order("by technique: ties by file name", strstr(output, "id=\"grafo\""),
                "dijkstra.html", "kruskal.html");
    check_order("by technique: empty last", strstr(output, "id=\"grafo\""),
                "prim.html", "sem_campos.html");

    write_index_page(INDEX_TECHNIQUE, ATTRIBUTE_DATE, output);
    check_absent("technique index: no sort by technique", output, "techniques_by_technique");
    check_absent("technique index: no technique column", output, "class=\"tecnica\">T\xC3\xA9" "cnica</th>");
    check_contains("technique index by date: the bar marks the index only",
                   output, "<a href=\"techniques_by_algorithm.html\" aria-current=\"true\">T\xC3\xA9" "cnica</a>");

    write_index_page(INDEX_ALGORITHM, ATTRIBUTE_TECHNIQUE, output);
    check_contains("algorithm index: one Algoritmo column, for the link",
                   output, "<th scope=\"col\" class=\"algoritmo\">Algoritmo</th><th scope=\"col\" class=\"tecnica\">T\xC3\xA9" "cnica</th>");
    check_contains("algorithm index: links the page",
                   output, "<td class=\"algoritmo\"><a href=\"prim.html\">Prim</a></td>");
    check_contains("algorithm index: default order marks the page", output,
                   "<a href=\"algorithms_by_technique.html\" aria-current=\"page\">Algoritmo</a>");

    write_index_page(INDEX_DATE, ATTRIBUTE_ALGORITHM, output);
    check_order("date index: keys as dates", output, ">1956</h2>", ">01/01/1956</h2>");
    check_order("date index: keys as dates", output, ">01/01/1956</h2>", ">12/03/1957</h2>");
    check_contains("date index: anchor", output, "id=\"12_2F03_2F1957\"");
}

static void test_content_page(void)
{
    static char output[MAX_OUTPUT_LEN];
    FILE *output_file = open_output();
    page_body *body = parser_read_body(site_pages[1]);

    check_true("kruskal body read", body != NULL);
    if (body == NULL) {
        fclose(output_file);
        return;
    }
    check_true("content page written", html_write_content_page(output_file, site_pages[1], body));
    page_body_free(body);
    read_output(output_file, output);

    check_contains("content: title", output, "<h1>Kruskal</h1>");
    check_contains("content: masthead links home", output,
                   "<p class=\"curso\"><a href=\"index.html\">");
    check_absent("content: no index is current", output, "aria-current");
    check_contains("content: fact table", output, "<table class=\"ficha\">");
    check_contains("content: technique row",
                   output, "<tr><th scope=\"row\">T\xC3\xA9" "cnica</th><td>M\xC3\xA9todo guloso</td></tr>");
    check_absent("content: no algorithm row, the title shows it", output, "scope=\"row\">Algoritmo<");
    check_contains("content: concept linked by normalized key, shown as written",
                   output, "<a href=\"concepts_by_algorithm.html#arvore\">\xC3\x81rvore</a>");
    check_contains("content: anchor is the same as in the index",
                   output, "concepts_by_algorithm.html#union_2Dfind");
    check_contains("content: author link",
                   output, "<a href=\"authors_by_algorithm.html#joseph_20kruskal\">Joseph Kruskal</a>");
    check_contains("content: problem stays plain", output,
                   "<tr><th scope=\"row\">Problema</th><td>Dado um grafo conexo");
    check_contains("content: section", output, "<h2>Ideia</h2>");
    check_contains("content: paragraph", output, "<p>Ordena as arestas");
    check_contains("content: code block", output, "<pre class=\"codigo\"><span class=\"l\">");
    check_contains("content: code is highlighted", output, "<span class=\"tipo\">int</span>");
    check_contains("content: code is escaped", output, "&lt;");
    /* In the TXT the reference comes before the code. */
    check_order("content: references at the end", output, "</pre>", "<h2>Refer\xC3\xAAncias</h2>");
    check_contains("content: reference", output, "<li>Cormen et al.");
    check_contains("content: link to its local graph", output,
                   "<p class=\"ver-grafo\"><a href=\"local_page_kruskal.html\">Ver no grafo</a></p>");
    check_contains("content: nav has the graph tab, not current", output,
                   "<li><a href=\"graph.html\">Grafo</a></li>");
}

static void test_content_page_escapes_every_field(void)
{
    static char output[MAX_OUTPUT_LEN];
    FILE *output_file = open_output();
    page *page_header = page_create();
    page_body *body = page_body_create();
    char authors[] = "<a>&";
    char concepts[] = "<c>&";

    if (page_header == NULL || body == NULL)
        exit(1);
    page_header->html_name = text_utils_copy_str("x y.html");
    page_header->algorithm = text_utils_copy_str("<alg>&");
    page_header->technique = text_utils_copy_str("<tec>&");
    page_header->date = text_utils_copy_str("<dat>&");
    page_header->problem = text_utils_copy_str("<pro>&");
    page_header->authors = text_utils_split_comma(authors, &page_header->authors_count);
    page_header->concepts = text_utils_split_comma(concepts, &page_header->concepts_count);
    if (!page_body_add_block(body, SECTION, "<sec>&") || !page_body_add_block(body, PARAGRAPH, "<par>&")
        || !page_body_add_block(body, CODE, "a < b;") || !page_body_add_block(body, REFERENCE, "<ref>&"))
        exit(1);

    check_true("escape page written", html_write_content_page(output_file, page_header, body));
    read_output(output_file, output);

    check_contains("escape: algorithm", output, "<h1>&lt;alg&gt;&amp;</h1>");
    check_contains("escape: technique", output, "&lt;tec&gt;&amp;");
    check_contains("escape: date", output, "&lt;dat&gt;&amp;");
    check_contains("escape: problem", output, "&lt;pro&gt;&amp;");
    check_contains("escape: author", output, "&lt;a&gt;&amp;</a>");
    check_contains("escape: concept", output, "&lt;c&gt;&amp;</a>");
    check_contains("escape: section", output, "&lt;sec&gt;&amp;");
    check_contains("escape: paragraph", output, "&lt;par&gt;&amp;");
    check_contains("escape: reference", output, "&lt;ref&gt;&amp;");
    check_contains("escape: anchor", output, "#_3Ca_3E_26\"");
    check_absent("escape: nothing raw", output, "<alg>");

    page_body_free(body);
    page_free(page_header);
}

static void test_file_links_are_encoded(void)
{
    static char output[MAX_OUTPUT_LEN];
    FILE *output_file = open_output();
    page *page_entry = page_create();
    page *pages[1];

    if (page_entry == NULL)
        exit(1);
    page_entry->html_name = text_utils_copy_str("a \"b\".html");
    pages[0] = page_entry;
    check_true("encoded home written", html_write_home_page(output_file, pages, 1));
    read_output(output_file, output);
    check_contains("href is URL-encoded", output, "href=\"a%20%22b%22.html\"");
    check_contains("base name text keeps its spelling", output, ">a \"b\"</a>");
    page_free(page_entry);
}

static void test_no_references_section(void)
{
    static char output[MAX_OUTPUT_LEN];
    FILE *output_file = open_output();
    page_body *body = parser_read_body(site_pages[0]);

    if (body == NULL) {
        check_true("dijkstra body read", 0);
        fclose(output_file);
        return;
    }
    html_write_content_page(output_file, site_pages[0], body);
    page_body_free(body);
    read_output(output_file, output);
    check_absent("no references: no section", output, "Refer\xC3\xAAncias");
    check_contains("paragraph is escaped", output, "a &lt; b &amp;&amp; c &gt; d");
}

static void test_page_without_fields(void)
{
    static char output[MAX_OUTPUT_LEN];
    FILE *output_file = open_output();
    page *page_header = page_create();
    page_body *body = page_body_create();

    if (page_header == NULL || body == NULL)
        exit(1);
    page_header->html_name = text_utils_copy_str("vazia.html");
    check_true("empty page written", html_write_content_page(output_file, page_header, body));
    read_output(output_file, output);
    check_contains("no fields: base name title", output, "<h1>vazia</h1>");
    check_absent("no fields: no empty fact table", output, "class=\"ficha\"");
    page_body_free(body);
    page_free(page_header);
}

static void test_style_sheet(void)
{
    static char output[MAX_OUTPUT_LEN];
    FILE *output_file = open_output();

    html_write_style_sheet(output_file);
    read_output(output_file, output);
    check_contains("style: highlighter rules", output, "pre.codigo");
    check_contains("style: site rules", output, "border-collapse");
    check_contains("style: keys start with a capital", output,
                   "h2.chave::first-letter, .chaves a::first-letter { text-transform:uppercase; }");
    check_contains("style: phones stack table rows", output,
                   ".tabela td[data-label]::before { content:attr(data-label)");
    check_contains("style: white page", output, "body { margin:0; background:#fff;");
    check_contains("style: page declares a light scheme", output, "color-scheme:only light;");
    /* Equal rules: the later one wins, so the white code background must follow
       the highlighter's grey one. */
    check_order("style: white code background after the highlighter's", output,
                "pre.codigo { background:#f2f2f2;", "pre.codigo { background:#fff;");
    check_absent("style: no scripts or external fonts", output, "@import");
}

static void test_generated_names(void)
{
    check_true("index.html is generated", html_is_generated_file_name("index.html"));
    check_true("index page is generated", html_is_generated_file_name("concepts_by_date.html"));
    check_true("content name is not", !html_is_generated_file_name("kruskal.html"));
    check_true("an order not offered is not generated",
               !html_is_generated_file_name("dates_by_date.html"));
    check_true("graph.html is generated", html_is_generated_file_name("graph.html"));
    check_true("graph_local.html is generated", html_is_generated_file_name("graph_local.html"));
    check_true("local_ names are reserved", html_is_generated_file_name("local_anything.html"));
    check_true("graph_x is free", !html_is_generated_file_name("graph_theory.html"));
}

static void test_graph_page(void)
{
    static char output[MAX_OUTPUT_LEN];
    FILE *output_file = open_output();

    html_write_graph_page(output_file, site_graph);
    read_output(output_file, output);
    check_contains("graph: tab is current", output,
                   "<li><a href=\"graph.html\" aria-current=\"page\">Grafo</a></li>");
    check_contains("graph: global view current", output,
                   "Vista: <strong aria-current=\"page\">Global</strong> "
                   "<a href=\"graph_local.html\">Local</a>");
    check_contains("graph: pages filter on", output,
                   "<input type=\"checkbox\" id=\"f-pagina\" checked><label for=\"f-pagina\">");
    check_contains("graph: dates filter off", output,
                   "<input type=\"checkbox\" id=\"f-data\"><label for=\"f-data\">");
    check_order("graph: filters before the drawing", output, "id=\"f-problema\"",
                "<div class=\"tela\">");
    check_contains("graph: global drawing", output, "<svg class=\"grafo global\" viewBox=\"0 0 ");
    check_contains("graph: page node links its page", output,
                   "<a id=\"n1\" class=\"no t-pagina\" href=\"kruskal.html\"><title>Kruskal (P\xC3\xA1gina)</title><circle");
    check_contains("graph: key node links its index section", output,
                   "class=\"no t-conceitos no-chave\" href=\"concepts_by_algorithm.html#arvore\">"
                   "<title>\xC3\x81rvore (Conceito)</title><polygon");
    check_contains("graph: key label escaped", output,
                   ">caminho &lt;m\xC3\xADnimo&gt; &amp; custo</text>");
    check_contains("graph: long label cut", output, "Dado um grafo conexo com pesos, \xE2\x80\xA6</text>");
    check_contains("graph: whole label in the tooltip", output,
                   "<title>Dado um grafo conexo com pesos, encontrar uma \xC3\xA1rvore espalhada de peso m\xC3\xADnimo. (Problema)</title>");
    check_contains("graph: edges carry the key type", output, "<line class=\"aresta t-tecnica a");
    check_contains("graph: hover rule for kruskal's edges", output,
                   ".tela:has(#n1:hover, #n1:focus) .a1 { stroke:#1d3557;");
    check_absent("graph: no algorithm keys", output, "(Algoritmo)");
    check_absent("graph: no script", output, "<script");
}

static void test_graph_chooser_page(void)
{
    static char output[MAX_OUTPUT_LEN];
    FILE *output_file = open_output();

    html_write_graph_chooser_page(output_file, site_graph);
    read_output(output_file, output);
    check_contains("chooser: local view current", output,
                   "<a href=\"graph.html\">Global</a> <strong aria-current=\"page\">Local</strong>");
    check_contains("chooser: tab marked as section", output,
                   "<a href=\"graph.html\" aria-current=\"true\">Grafo</a>");
    check_contains("chooser: page link", output,
                   "<li><a href=\"local_page_kruskal.html\">Kruskal</a></li>");
    check_contains("chooser: key link", output,
                   "<li><a href=\"local_concepts_2.html\" class=\"chave-nome\">grafo</a></li>");
    check_order("chooser: types in order", output, "<h2>P\xC3\xA1ginas</h2>", "<h2>Problemas</h2>");
    check_contains("chooser: long list for problems", output, "<ul class=\"escolha longa\">");
}

static void test_local_graph_page(void)
{
    static char output[MAX_OUTPUT_LEN];
    FILE *output_file = open_output();
    graph *local_graph = graph_build_local(site_graph, 1);

    if (local_graph == NULL) {
        check_true("local graph built", 0);
        fclose(output_file);
        return;
    }
    html_write_local_graph_page(output_file, local_graph);
    graph_free(local_graph);
    read_output(output_file, output);
    check_contains("local: title names the centre", output, "<title>Kruskal \xC2\xB7 Grafo");
    check_contains("local: centre heading", output, "<h2 class=\"foco\">Kruskal</h2>");
    check_contains("local: link to the real page", output,
                   "P\xC3\xA1gina \xC2\xB7 <a href=\"kruskal.html\">Abrir a p\xC3\xA1gina</a>");
    check_contains("local: local view marked", output,
                   "<a href=\"graph_local.html\" aria-current=\"true\">Local</a>");
    check_contains("local: legend", output, "<ul class=\"legenda\">");
    check_contains("local: local drawing", output, "<svg class=\"grafo local\"");
    check_contains("local: centre is node 0", output, "<a id=\"n0\" class=\"no t-pagina\" href=\"kruskal.html\">");
    check_contains("local: dates shown", output, "(Data)</title>");
}

int main(void)
{
    static const char *const FIXTURE_PATHS[PAGE_COUNT] = {
        "tests/fixtures/site/dijkstra.txt",
        "tests/fixtures/site/kruskal.txt",
        "tests/fixtures/site/prim.txt",
        "tests/fixtures/site/sem_campos.txt"
    };
    int page_index;

    for (page_index = 0; page_index < PAGE_COUNT; page_index++) {
        site_pages[page_index] = parser_read_header(FIXTURE_PATHS[page_index]);
        if (site_pages[page_index] == NULL) {
            printf("FAIL could not read %s\n", FIXTURE_PATHS[page_index]);
            return 1;
        }
    }
    indexes = indexes_build(site_pages, PAGE_COUNT);
    site_graph = indexes == NULL ? NULL : graph_build(site_pages, PAGE_COUNT, indexes);
    if (site_graph == NULL || !graph_layout_global(site_graph)) {
        printf("FAIL could not build the indexes and the graph\n");
        return 1;
    }

    test_home_page();
    test_index_page();
    test_content_page();
    test_content_page_escapes_every_field();
    test_file_links_are_encoded();
    test_no_references_section();
    test_page_without_fields();
    test_style_sheet();
    test_generated_names();
    test_graph_page();
    test_graph_chooser_page();
    test_local_graph_page();

    graph_free(site_graph);
    indexes_free(indexes);
    for (page_index = 0; page_index < PAGE_COUNT; page_index++)
        page_free(site_pages[page_index]);
    return failure_count == 0 ? 0 : 1;
}
