/* Tests for graph, using the TXT files in tests/fixtures/site/. Prints only the
   failing cases; exits 0 when every case passes. Build and run from the
   repository root, so the fixture paths resolve:
   gcc -std=c89 -Wall -Wextra -pedantic -Werror tests/test_graph.c src/graph.c src/indexes.c src/bst.c src/sorted_list.c src/parser.c src/page.c src/text_utils.c -o test_graph */
#include <stdio.h>
#include <string.h>
#include "../src/graph.h"
#include "../src/parser.h"

#define PAGE_COUNT 4
#define PI 3.14159265358979323846

static int failure_count = 0;

static void check_true(const char *case_name, int condition)
{
    if (!condition) {
        printf("FAIL %s\n", case_name);
        failure_count++;
    }
}

static int is_close(double actual, double expected)
{
    double difference = actual - expected;

    return difference < 1e-9 && difference > -1e-9;
}

static void test_math(void)
{
    check_true("sqrt 0", is_close(graph_square_root(0.0), 0.0));
    check_true("sqrt negative is 0", is_close(graph_square_root(-4.0), 0.0));
    check_true("sqrt 2", is_close(graph_square_root(2.0), 1.4142135623730951));
    check_true("sqrt 1e6", is_close(graph_square_root(1e6), 1000.0));
    check_true("sqrt 0.0001", is_close(graph_square_root(0.0001), 0.01));
    check_true("sine 0", is_close(graph_sine(0.0), 0.0));
    check_true("sine pi/2", is_close(graph_sine(PI / 2.0), 1.0));
    check_true("sine pi/6", is_close(graph_sine(PI / 6.0), 0.5));
    check_true("sine -pi/2", is_close(graph_sine(-PI / 2.0), -1.0));
    check_true("sine after many turns", is_close(graph_sine(20.0 * PI + PI / 6.0), 0.5));
    check_true("cosine 0", is_close(graph_cosine(0.0), 1.0));
    check_true("cosine pi", is_close(graph_cosine(PI), -1.0));
    check_true("cosine pi/3", is_close(graph_cosine(PI / 3.0), 0.5));
}

static int count_nodes_of_type(const graph *site_graph, enum graph_node_type type)
{
    int node_count = 0;
    int node_id;

    for (node_id = 0; node_id < site_graph->node_count; node_id++) {
        if (site_graph->nodes[node_id].type == type)
            node_count++;
    }
    return node_count;
}

/* Returns the id of the key node whose normalized key is key, or -1. */
static int find_key_node(const graph *site_graph, enum graph_node_type type, const char *key)
{
    int node_id;

    for (node_id = 0; node_id < site_graph->node_count; node_id++) {
        if (site_graph->nodes[node_id].type == type
            && strcmp(site_graph->nodes[node_id].key, key) == 0)
            return node_id;
    }
    return -1;
}

static void test_build(const graph *site_graph, page *const *pages)
{
    int kruskal_id;
    int grafo_id;
    int node_id;
    int is_sorted = 1;

    check_true("4 pages", count_nodes_of_type(site_graph, GRAPH_NODE_PAGE) == 4);
    check_true("1 technique", count_nodes_of_type(site_graph, GRAPH_NODE_TECHNIQUE) == 1);
    check_true("5 authors", count_nodes_of_type(site_graph, GRAPH_NODE_AUTHORS) == 5);
    check_true("3 dates", count_nodes_of_type(site_graph, GRAPH_NODE_DATE) == 3);
    check_true("5 concepts", count_nodes_of_type(site_graph, GRAPH_NODE_CONCEPTS) == 5);
    check_true("2 problems", count_nodes_of_type(site_graph, GRAPH_NODE_PROBLEM) == 2);
    check_true("no algorithm keys, so 20 nodes", site_graph->node_count == 20);
    check_true("24 edges", site_graph->edge_count == 24);

    /* Pages first, by file name, whatever order they were read in. */
    check_true("first page is dijkstra",
               strcmp(site_graph->nodes[0].page_entry->html_name, "dijkstra.html") == 0);
    check_true("last page is sem_campos",
               strcmp(site_graph->nodes[3].page_entry->html_name, "sem_campos.html") == 0);
    check_true("page ordinal", site_graph->nodes[3].ordinal == 3);

    kruskal_id = graph_find_page_node(site_graph, pages[1]);
    check_true("find kruskal", kruskal_id == 1);
    check_true("kruskal has 7 keys", site_graph->nodes[kruskal_id].degree == 7);

    grafo_id = find_key_node(site_graph, GRAPH_NODE_CONCEPTS, "grafo");
    check_true("concept grafo exists", grafo_id != -1);
    if (grafo_id != -1) {
        check_true("grafo links 4 pages", site_graph->nodes[grafo_id].degree == 4);
        check_true("grafo keeps its display spelling",
                   strcmp(site_graph->nodes[grafo_id].display_key, "grafo") == 0);
        check_true("concept ordinal in key order", site_graph->nodes[grafo_id].ordinal == 2);
    }

    for (node_id = 0; node_id < site_graph->node_count; node_id++) {
        int position;
        for (position = site_graph->neighbour_start[node_id] + 1;
             position < site_graph->neighbour_start[node_id + 1]; position++) {
            if (site_graph->neighbours[position - 1] >= site_graph->neighbours[position])
                is_sorted = 0;
        }
    }
    check_true("neighbour lists sorted", is_sorted);
}

static void test_layout(graph *site_graph)
{
    long first_x[64];
    long first_y[64];
    int node_id;
    int is_same = 1;
    int is_inside = 1;
    int has_distinct = 1;

    check_true("layout succeeds", graph_layout_global(site_graph));
    for (node_id = 0; node_id < site_graph->node_count; node_id++) {
        first_x[node_id] = site_graph->nodes[node_id].x;
        first_y[node_id] = site_graph->nodes[node_id].y;
        if (site_graph->nodes[node_id].x < 0 || site_graph->nodes[node_id].x > site_graph->width
            || site_graph->nodes[node_id].y < 0
            || site_graph->nodes[node_id].y > site_graph->height)
            is_inside = 0;
    }
    check_true("every node inside the drawing", is_inside);
    check_true("drawing has a size", site_graph->width > 0 && site_graph->height > 0);

    graph_layout_global(site_graph);
    for (node_id = 0; node_id < site_graph->node_count; node_id++) {
        if (site_graph->nodes[node_id].x != first_x[node_id]
            || site_graph->nodes[node_id].y != first_y[node_id])
            is_same = 0;
    }
    check_true("layout is repeatable", is_same);

    for (node_id = 1; node_id < site_graph->node_count; node_id++) {
        if (site_graph->nodes[node_id].x == site_graph->nodes[0].x
            && site_graph->nodes[node_id].y == site_graph->nodes[0].y)
            has_distinct = 0;
    }
    check_true("no node on top of the first", has_distinct);
}

/* A page that shares no key forms its own cluster. Clusters do not repel
   each other and are packed side by side, so it sits beside the rest instead
   of being pushed far away, which shrank the whole drawing. */
static void test_separate_clusters(page *kruskal)
{
    page *pages[2];
    site_indexes *indexes;
    graph *site_graph;

    pages[0] = kruskal;
    pages[1] = parser_read_header("tests/fixtures/graph/isolada.txt");
    check_true("read isolada", pages[1] != NULL);
    if (pages[1] == NULL)
        return;
    indexes = indexes_build(pages, 2);
    site_graph = indexes == NULL ? NULL : graph_build(pages, 2, indexes);
    check_true("two-cluster graph built", site_graph != NULL);
    if (site_graph != NULL) {
        check_true("two-cluster layout succeeds", graph_layout_global(site_graph));
        check_true("clusters packed close", site_graph->width < 1000 && site_graph->height < 1000);
        graph_free(site_graph);
    }
    indexes_free(indexes);
    page_free(pages[1]);
}

static void test_local(const graph *site_graph)
{
    graph *local_graph;
    int grafo_id = find_key_node(site_graph, GRAPH_NODE_CONCEPTS, "grafo");
    int node_id;
    int is_inside = 1;

    /* Kruskal: 7 keys, then the 3 other pages sharing them; 7 + 2 + 4 + 1 edges. */
    local_graph = graph_build_local(site_graph, 1);
    check_true("local kruskal built", local_graph != NULL);
    if (local_graph != NULL) {
        check_true("local kruskal: 11 nodes", local_graph->node_count == 11);
        check_true("local kruskal: 14 edges", local_graph->edge_count == 14);
        check_true("local kruskal: centre first", local_graph->centre == 0
                   && local_graph->nodes[0].page_entry == site_graph->nodes[1].page_entry);
        check_true("local keeps the whole degree", local_graph->nodes[0].degree == 7);
        for (node_id = 0; node_id < local_graph->node_count; node_id++) {
            if (local_graph->nodes[node_id].x < 0 || local_graph->nodes[node_id].x > local_graph->width
                || local_graph->nodes[node_id].y < 0
                || local_graph->nodes[node_id].y > local_graph->height)
                is_inside = 0;
        }
        check_true("local kruskal: inside the drawing", is_inside);
        check_true("local kruskal: centre in the middle",
                   local_graph->nodes[0].x * 2 == local_graph->width
                   || local_graph->nodes[0].x * 2 == local_graph->width + 1
                   || local_graph->nodes[0].x * 2 == local_graph->width - 1);
        graph_free(local_graph);
    }

    /* Grafo: its 4 pages, then all their other keys (15 of the 16 keys). */
    local_graph = graph_build_local(site_graph, grafo_id);
    check_true("local grafo built", local_graph != NULL);
    if (local_graph != NULL) {
        check_true("local grafo: 1 + 4 + 15 nodes", local_graph->node_count == 20);
        check_true("local grafo: ring 1 is pages",
                   local_graph->nodes[1].type == GRAPH_NODE_PAGE
                   && local_graph->nodes[4].type == GRAPH_NODE_PAGE);
        graph_free(local_graph);
    }
    graph_free(NULL);
}

int main(void)
{
    /* Read out of file-name order on purpose: ids must not depend on it. */
    static const char *const FIXTURE_PATHS[PAGE_COUNT] = {
        "tests/fixtures/site/sem_campos.txt",
        "tests/fixtures/site/kruskal.txt",
        "tests/fixtures/site/prim.txt",
        "tests/fixtures/site/dijkstra.txt"
    };
    page *pages[PAGE_COUNT];
    site_indexes *indexes;
    graph *site_graph;
    int page_index;

    for (page_index = 0; page_index < PAGE_COUNT; page_index++) {
        pages[page_index] = parser_read_header(FIXTURE_PATHS[page_index]);
        if (pages[page_index] == NULL) {
            printf("FAIL could not read %s\n", FIXTURE_PATHS[page_index]);
            return 1;
        }
    }
    indexes = indexes_build(pages, PAGE_COUNT);
    site_graph = indexes == NULL ? NULL : graph_build(pages, PAGE_COUNT, indexes);
    if (site_graph == NULL) {
        printf("FAIL could not build the graph\n");
        return 1;
    }

    test_math();
    test_build(site_graph, pages);
    test_layout(site_graph);
    test_local(site_graph);
    test_separate_clusters(pages[1]);

    graph_free(site_graph);
    indexes_free(indexes);
    for (page_index = 0; page_index < PAGE_COUNT; page_index++)
        page_free(pages[page_index]);
    return failure_count == 0 ? 0 : 1;
}
