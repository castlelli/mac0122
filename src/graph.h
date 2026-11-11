#ifndef GRAPH_H
#define GRAPH_H

#include "indexes.h"
#include "page.h"

/*
 * Graph of the site: pages and index keys are the nodes, and each page is
 * linked to every key it lists. It is the bipartite graph the indexes already
 * hold, laid out for drawing.
 *
 * Algorithm keys are left out: an algorithm key names its own page, so it
 * would draw every page twice.
 *
 * Node ids: the pages first, in TXT file-name order, then the keys of each
 * index (technique, authors, date, concepts, problem), each in key order. Ids
 * and layouts depend only on the TXTs, never on the order they were given in.
 *
 * The layouts use their own square root, sine and cosine (below), not
 * <math.h>: the build needs no -lm and the coordinates do not depend on the
 * machine's math library.
 *
 * Ownership: graph_build and graph_build_local allocate a graph, and
 * graph_free frees it with its arrays. Nodes borrow their page and their key
 * strings from main's pages and from the indexes, which must outlive the graph.
 */

enum graph_node_type {
    GRAPH_NODE_PAGE,
    GRAPH_NODE_TECHNIQUE,
    GRAPH_NODE_AUTHORS,
    GRAPH_NODE_DATE,
    GRAPH_NODE_CONCEPTS,
    GRAPH_NODE_PROBLEM,
    GRAPH_NODE_TYPE_COUNT
};

typedef struct graph_node {
    enum graph_node_type type;
    /* Page nodes: the page. Key nodes: NULL. */
    const page *page_entry;
    /* Key nodes: the normalized key and its display spelling. Page nodes: NULL. */
    const char *key;
    const char *display_key;
    /* Key nodes: the index the key belongs to. */
    enum index_type index;
    /* Position among the nodes of its type, starting at 0; it names the
       node's local graph page. */
    int ordinal;
    /* Number of edges in the whole graph, also in a local graph. */
    int degree;
    /* Position after a layout, in drawing units from the top left corner. */
    long x;
    long y;
} graph_node;

/* Every edge joins a page node to a key node. */
typedef struct graph_edge {
    int page_node;
    int key_node;
} graph_edge;

typedef struct graph {
    graph_node *nodes;
    int node_count;
    graph_edge *edges;
    int edge_count;
    /* Neighbours of node n, ascending by id: neighbours[neighbour_start[n]]
       up to neighbours[neighbour_start[n + 1] - 1]. */
    int *neighbour_start;
    int *neighbours;
    /* For a local graph, the id of the centre node; -1 for the whole graph. */
    int centre;
    /* Size of the drawing after a layout. */
    long width;
    long height;
} graph;

/* Builds the graph of every page and of the keys of five indexes (all but
   algorithm), with no layout yet. Returns NULL if memory runs out. */
graph *graph_build(page *const *pages, int page_count, const site_indexes *indexes);

/* Places every node with a force-directed layout: a fixed start and a fixed
   number of steps, so the result is the same on every run. Sets x, y, width
   and height. Returns 1 on success and 0 if memory runs out. */
int graph_layout_global(graph *site_graph);

/* Builds the local graph of node centre: the node, its neighbours and their
   neighbours, with every edge among them, laid out in rings around the
   centre. Node ids are renumbered; type, ordinal and degree in the whole
   graph are kept. Returns NULL if memory runs out. */
graph *graph_build_local(const graph *site_graph, int centre);

/* Returns the id of the node of page_entry, or -1 when it is not in the graph. */
int graph_find_page_node(const graph *site_graph, const page *page_entry);

/* Frees the graph and its arrays, never the pages or keys. Does nothing if
   site_graph is NULL. */
void graph_free(graph *site_graph);

/* The math the layouts use, public for the tests. graph_square_root returns 0
   for values at or below 0. */
double graph_square_root(double value);
double graph_sine(double angle);
double graph_cosine(double angle);

#endif
