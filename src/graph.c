#include <stdlib.h>
#include <string.h>
#include "graph.h"

#define PI 3.14159265358979323846
/* Ideal edge length of the global layout, in drawing units. */
#define IDEAL_EDGE_LEN 95.0
#define GLOBAL_LAYOUT_STEPS 500
/* Pull towards the centre, so separate clusters stay near each other. */
#define GRAVITY 0.02
/* Room around the drawing, wider on the right for page labels. */
#define MARGIN_LEFT 40.0
#define MARGIN_TOP 40.0
#define MARGIN_RIGHT 200.0
#define MARGIN_BOTTOM 40.0
/* Space each node takes on a ring of a local graph. */
#define RING_NODE_SPACING 64.0
#define FIRST_RING_MIN_RADIUS 150.0
#define RING_GAP 170.0
/* Local graphs leave room on both sides for labels written outwards. */
#define LOCAL_MARGIN_SIDES 260.0
#define LOCAL_MARGIN_ENDS 60.0

/* The indexes drawn as key nodes, in node-id order. */
static const enum index_type GRAPH_INDEXES[] = {
    INDEX_TECHNIQUE, INDEX_AUTHORS, INDEX_DATE, INDEX_CONCEPTS, INDEX_PROBLEM
};
static const enum graph_node_type GRAPH_INDEX_TYPES[] = {
    GRAPH_NODE_TECHNIQUE, GRAPH_NODE_AUTHORS, GRAPH_NODE_DATE,
    GRAPH_NODE_CONCEPTS, GRAPH_NODE_PROBLEM
};
#define GRAPH_INDEX_COUNT 5

double graph_square_root(double value)
{
    double scale = 1.0;
    double estimate;
    int step;

    if (value <= 0.0)
        return 0.0;
    /* Bring value into [1, 4) by powers of 4, which only moves the result by
       powers of 2, both exact in floating point. From there Newton's method,
       which doubles the correct digits each step, needs about five steps. */
    while (value >= 4.0) {
        value /= 4.0;
        scale *= 2.0;
    }
    while (value < 1.0) {
        value *= 4.0;
        scale /= 2.0;
    }
    estimate = 0.5 * (1.0 + value);
    for (step = 0; step < 6; step++)
        estimate = 0.5 * (estimate + value / estimate);
    return estimate * scale;
}

/* Brings angle into [-PI, PI], where the series below is accurate. */
static double reduce_angle(double angle)
{
    long turns;

    turns = (long)(angle / (2.0 * PI));
    angle -= (double)turns * 2.0 * PI;
    if (angle > PI)
        angle -= 2.0 * PI;
    else if (angle < -PI)
        angle += 2.0 * PI;
    return angle;
}

double graph_sine(double angle)
{
    double term;
    double sum;
    int power;

    angle = reduce_angle(angle);
    term = angle;
    sum = angle;
    /* Taylor series; 20 terms leave an error far below a drawing unit. */
    for (power = 3; power <= 41; power += 2) {
        term *= -angle * angle / (double)((power - 1) * power);
        sum += term;
    }
    return sum;
}

double graph_cosine(double angle)
{
    return graph_sine(angle + PI / 2.0);
}

/* What building the key nodes needs from each BST visit. */
typedef struct build_context {
    graph *site_graph;
    int node_capacity;
    int edge_capacity;
    enum index_type index;
    enum graph_node_type type;
    int ordinal;
    int is_out_of_memory;
} build_context;

static int grow_array(void **array, int *capacity, int needed, size_t item_size)
{
    void *grown;
    int new_capacity;

    if (needed <= *capacity)
        return 1;
    new_capacity = *capacity == 0 ? 16 : *capacity * 2;
    while (new_capacity < needed)
        new_capacity *= 2;
    grown = realloc(*array, (size_t)new_capacity * item_size);
    if (grown == NULL)
        return 0;
    *array = grown;
    *capacity = new_capacity;
    return 1;
}

int graph_find_page_node(const graph *site_graph, const page *page_entry)
{
    int low = 0;
    int high;
    int middle;
    int comparison;

    /* Page nodes come first, sorted by file name. */
    high = site_graph->node_count - 1;
    while (high >= 0 && site_graph->nodes[high].type != GRAPH_NODE_PAGE)
        high--;
    while (low <= high) {
        middle = low + (high - low) / 2;
        comparison = indexes_compare_pages_by_file_name(site_graph->nodes[middle].page_entry,
                                                       page_entry);
        if (comparison == 0)
            return middle;
        if (comparison < 0)
            low = middle + 1;
        else
            high = middle - 1;
    }
    return -1;
}

static int add_key_node(const char *key, const char *display_key,
                        const sorted_list *pages, void *context)
{
    build_context *build = context;
    graph *site_graph = build->site_graph;
    graph_node *node;
    const sorted_list_node *list_node;
    int key_id = site_graph->node_count;

    if (!grow_array((void **)&site_graph->nodes, &build->node_capacity,
                    site_graph->node_count + 1, sizeof(graph_node))) {
        build->is_out_of_memory = 1;
        return 0;
    }
    node = &site_graph->nodes[key_id];
    node->type = build->type;
    node->page_entry = NULL;
    node->key = key;
    node->display_key = display_key;
    node->index = build->index;
    node->ordinal = build->ordinal++;
    node->degree = 0;
    node->x = 0;
    node->y = 0;
    site_graph->node_count++;

    for (list_node = pages->first_node; list_node != NULL; list_node = list_node->next) {
        if (!grow_array((void **)&site_graph->edges, &build->edge_capacity,
                        site_graph->edge_count + 1, sizeof(graph_edge))) {
            build->is_out_of_memory = 1;
            return 0;
        }
        site_graph->edges[site_graph->edge_count].page_node =
            graph_find_page_node(site_graph, list_node->page_entry);
        site_graph->edges[site_graph->edge_count].key_node = key_id;
        site_graph->edge_count++;
    }
    return 1;
}

static int compare_int(const void *first, const void *second)
{
    int first_value = *(const int *)first;
    int second_value = *(const int *)second;

    return (first_value > second_value) - (first_value < second_value);
}

/* Fills neighbour_start and neighbours from the edges. Node degrees are left
   alone: a local graph keeps the degree its nodes have in the whole graph.
   Returns 0 if memory runs out. */
static int build_neighbour_lists(graph *site_graph)
{
    int *next_free;
    int node_id;
    int edge_index;
    int page_id;
    int key_id;

    site_graph->neighbour_start = malloc((size_t)(site_graph->node_count + 1) * sizeof(int));
    site_graph->neighbours = malloc((size_t)(2 * site_graph->edge_count + 1) * sizeof(int));
    next_free = calloc((size_t)site_graph->node_count + 1, sizeof(int));
    if (site_graph->neighbour_start == NULL || site_graph->neighbours == NULL
        || next_free == NULL) {
        free(next_free);
        return 0;
    }

    /* next_free first counts each node's edges, then marks where its next
       neighbour goes. */
    for (edge_index = 0; edge_index < site_graph->edge_count; edge_index++) {
        next_free[site_graph->edges[edge_index].page_node]++;
        next_free[site_graph->edges[edge_index].key_node]++;
    }
    site_graph->neighbour_start[0] = 0;
    for (node_id = 0; node_id < site_graph->node_count; node_id++) {
        site_graph->neighbour_start[node_id + 1] =
            site_graph->neighbour_start[node_id] + next_free[node_id];
        next_free[node_id] = site_graph->neighbour_start[node_id];
    }
    for (edge_index = 0; edge_index < site_graph->edge_count; edge_index++) {
        page_id = site_graph->edges[edge_index].page_node;
        key_id = site_graph->edges[edge_index].key_node;
        site_graph->neighbours[next_free[page_id]++] = key_id;
        site_graph->neighbours[next_free[key_id]++] = page_id;
    }
    for (node_id = 0; node_id < site_graph->node_count; node_id++)
        qsort(site_graph->neighbours + site_graph->neighbour_start[node_id],
              (size_t)(site_graph->neighbour_start[node_id + 1]
                       - site_graph->neighbour_start[node_id]),
              sizeof(int), compare_int);

    free(next_free);
    return 1;
}

static graph *create_empty_graph(void)
{
    graph *site_graph = malloc(sizeof(graph));

    if (site_graph == NULL)
        return NULL;
    site_graph->nodes = NULL;
    site_graph->node_count = 0;
    site_graph->edges = NULL;
    site_graph->edge_count = 0;
    site_graph->neighbour_start = NULL;
    site_graph->neighbours = NULL;
    site_graph->centre = -1;
    site_graph->width = 0;
    site_graph->height = 0;
    return site_graph;
}

graph *graph_build(page *const *pages, int page_count, const site_indexes *indexes)
{
    graph *site_graph = create_empty_graph();
    sorted_list *pages_by_name;
    const sorted_list_node *list_node;
    build_context build;
    int index_position;
    int page_index;

    if (site_graph == NULL)
        return NULL;
    build.site_graph = site_graph;
    build.node_capacity = 0;
    build.edge_capacity = 0;
    build.is_out_of_memory = 0;

    /* Page nodes in file-name order, whatever order main read them in. */
    pages_by_name = sorted_list_create(indexes_compare_pages_by_file_name);
    if (pages_by_name == NULL) {
        graph_free(site_graph);
        return NULL;
    }
    for (page_index = 0; page_index < page_count; page_index++) {
        if (!sorted_list_insert_unique(pages_by_name, pages[page_index]))
            build.is_out_of_memory = 1;
    }
    build.ordinal = 0;
    for (list_node = pages_by_name->first_node;
         list_node != NULL && !build.is_out_of_memory; list_node = list_node->next) {
        if (!grow_array((void **)&site_graph->nodes, &build.node_capacity,
                        site_graph->node_count + 1, sizeof(graph_node))) {
            build.is_out_of_memory = 1;
            break;
        }
        memset(&site_graph->nodes[site_graph->node_count], 0, sizeof(graph_node));
        site_graph->nodes[site_graph->node_count].type = GRAPH_NODE_PAGE;
        site_graph->nodes[site_graph->node_count].page_entry = list_node->page_entry;
        site_graph->nodes[site_graph->node_count].ordinal = build.ordinal++;
        site_graph->node_count++;
    }
    sorted_list_free(pages_by_name);

    for (index_position = 0; index_position < GRAPH_INDEX_COUNT && !build.is_out_of_memory;
         index_position++) {
        build.index = GRAPH_INDEXES[index_position];
        build.type = GRAPH_INDEX_TYPES[index_position];
        build.ordinal = 0;
        bst_walk_in_order(indexes->trees[build.index], add_key_node, &build);
    }

    if (build.is_out_of_memory || !build_neighbour_lists(site_graph)) {
        graph_free(site_graph);
        return NULL;
    }
    for (page_index = 0; page_index < site_graph->node_count; page_index++)
        site_graph->nodes[page_index].degree = site_graph->neighbour_start[page_index + 1]
                                               - site_graph->neighbour_start[page_index];
    return site_graph;
}

/* Rounds a coordinate, which is never negative once the drawing is shifted
   into its margins. */
static long round_coordinate(double value)
{
    return (long)(value + 0.5);
}

/* Shifts the positions so the drawing starts at its margins, rounds them, and
   sets the drawing size. */
static void place_in_drawing(graph *site_graph, const double *position_x,
                             const double *position_y, double margin_left,
                             double margin_top, double margin_right, double margin_bottom)
{
    double min_x = 0.0;
    double min_y = 0.0;
    double max_x = 0.0;
    double max_y = 0.0;
    int node_id;

    for (node_id = 0; node_id < site_graph->node_count; node_id++) {
        if (node_id == 0 || position_x[node_id] < min_x)
            min_x = position_x[node_id];
        if (node_id == 0 || position_y[node_id] < min_y)
            min_y = position_y[node_id];
        if (node_id == 0 || position_x[node_id] > max_x)
            max_x = position_x[node_id];
        if (node_id == 0 || position_y[node_id] > max_y)
            max_y = position_y[node_id];
    }
    for (node_id = 0; node_id < site_graph->node_count; node_id++) {
        site_graph->nodes[node_id].x = round_coordinate(position_x[node_id] - min_x + margin_left);
        site_graph->nodes[node_id].y = round_coordinate(position_y[node_id] - min_y + margin_top);
    }
    site_graph->width = round_coordinate(max_x - min_x + margin_left + margin_right);
    site_graph->height = round_coordinate(max_y - min_y + margin_top + margin_bottom);
}

/* Starting circle: pages on an inner ring, keys on an outer ring in id order,
   which groups them by type. A fixed start makes the layout repeatable. */
static void place_start_rings(const graph *site_graph, double *position_x, double *position_y)
{
    int page_count = 0;
    int key_count;
    int node_id;
    double inner_radius;
    double outer_radius;
    double angle;

    while (page_count < site_graph->node_count
           && site_graph->nodes[page_count].type == GRAPH_NODE_PAGE)
        page_count++;
    key_count = site_graph->node_count - page_count;
    inner_radius = IDEAL_EDGE_LEN * graph_square_root((double)page_count + 1.0);
    outer_radius = inner_radius + IDEAL_EDGE_LEN * graph_square_root((double)key_count + 1.0);

    for (node_id = 0; node_id < site_graph->node_count; node_id++) {
        if (node_id < page_count) {
            angle = 2.0 * PI * (double)node_id / (double)page_count;
            position_x[node_id] = inner_radius * graph_cosine(angle);
            position_y[node_id] = inner_radius * graph_sine(angle);
        } else {
            angle = 2.0 * PI * (double)(node_id - page_count) / (double)key_count;
            position_x[node_id] = outer_radius * graph_cosine(angle);
            position_y[node_id] = outer_radius * graph_sine(angle);
        }
    }
}

/* Places each date node at the middle of its pages, nudged off them in a
   direction that turns with its ordinal, so dates shared by the same pages do
   not land on one spot. */
static void place_dates_beside_pages(const graph *site_graph, double *position_x,
                                     double *position_y)
{
    int node_id;
    int position;
    int neighbour_count;
    double sum_x;
    double sum_y;
    double angle;

    for (node_id = 0; node_id < site_graph->node_count; node_id++) {
        if (site_graph->nodes[node_id].type != GRAPH_NODE_DATE)
            continue;
        sum_x = 0.0;
        sum_y = 0.0;
        neighbour_count = 0;
        for (position = site_graph->neighbour_start[node_id];
             position < site_graph->neighbour_start[node_id + 1]; position++) {
            sum_x += position_x[site_graph->neighbours[position]];
            sum_y += position_y[site_graph->neighbours[position]];
            neighbour_count++;
        }
        if (neighbour_count == 0)
            continue;
        angle = 2.0 * PI * (double)(site_graph->nodes[node_id].ordinal % 12) / 12.0;
        position_x[node_id] = sum_x / neighbour_count + 0.4 * IDEAL_EDGE_LEN * graph_cosine(angle);
        position_y[node_id] = sum_y / neighbour_count + 0.4 * IDEAL_EDGE_LEN * graph_sine(angle);
    }
}

int graph_layout_global(graph *site_graph)
{
    int node_count = site_graph->node_count;
    double *position_x;
    double *position_y;
    double *move_x;
    double *move_y;
    double temperature;
    double distance;
    double force;
    double delta_x;
    double delta_y;
    double move_len;
    int step;
    int first_id;
    int second_id;
    int edge_index;

    if (node_count == 0) {
        site_graph->width = round_coordinate(MARGIN_LEFT + MARGIN_RIGHT);
        site_graph->height = round_coordinate(MARGIN_TOP + MARGIN_BOTTOM);
        return 1;
    }
    position_x = malloc((size_t)node_count * sizeof(double));
    position_y = malloc((size_t)node_count * sizeof(double));
    move_x = malloc((size_t)node_count * sizeof(double));
    move_y = malloc((size_t)node_count * sizeof(double));
    if (position_x == NULL || position_y == NULL || move_x == NULL || move_y == NULL) {
        free(position_x);
        free(position_y);
        free(move_x);
        free(move_y);
        return 0;
    }

    place_start_rings(site_graph, position_x, position_y);

    /* Fruchterman-Reingold: every pair repels, every edge pulls, and each
       node moves at most "temperature" per step, which cools to zero. Date
       nodes stay out of the forces: they start hidden, and a date seldom joins
       two pages, so they would only spread the drawing. They are placed beside
       their pages afterwards. */
    temperature = IDEAL_EDGE_LEN * 2.0;
    for (step = 0; step < GLOBAL_LAYOUT_STEPS; step++) {
        for (first_id = 0; first_id < node_count; first_id++) {
            move_x[first_id] = -GRAVITY * position_x[first_id];
            move_y[first_id] = -GRAVITY * position_y[first_id];
        }
        for (first_id = 0; first_id < node_count; first_id++) {
            if (site_graph->nodes[first_id].type == GRAPH_NODE_DATE)
                continue;
            for (second_id = first_id + 1; second_id < node_count; second_id++) {
                if (site_graph->nodes[second_id].type == GRAPH_NODE_DATE)
                    continue;
                delta_x = position_x[first_id] - position_x[second_id];
                delta_y = position_y[first_id] - position_y[second_id];
                distance = graph_square_root(delta_x * delta_x + delta_y * delta_y);
                if (distance < 0.01) {
                    /* Two nodes on one spot: push apart along a fixed axis. */
                    delta_x = 0.01;
                    delta_y = 0.0;
                    distance = 0.01;
                }
                force = IDEAL_EDGE_LEN * IDEAL_EDGE_LEN / distance;
                move_x[first_id] += delta_x / distance * force;
                move_y[first_id] += delta_y / distance * force;
                move_x[second_id] -= delta_x / distance * force;
                move_y[second_id] -= delta_y / distance * force;
            }
        }
        for (edge_index = 0; edge_index < site_graph->edge_count; edge_index++) {
            first_id = site_graph->edges[edge_index].page_node;
            second_id = site_graph->edges[edge_index].key_node;
            if (site_graph->nodes[second_id].type == GRAPH_NODE_DATE)
                continue;
            delta_x = position_x[first_id] - position_x[second_id];
            delta_y = position_y[first_id] - position_y[second_id];
            distance = graph_square_root(delta_x * delta_x + delta_y * delta_y);
            if (distance < 0.01)
                continue;
            force = distance * distance / IDEAL_EDGE_LEN;
            move_x[first_id] -= delta_x / distance * force;
            move_y[first_id] -= delta_y / distance * force;
            move_x[second_id] += delta_x / distance * force;
            move_y[second_id] += delta_y / distance * force;
        }
        for (first_id = 0; first_id < node_count; first_id++) {
            if (site_graph->nodes[first_id].type == GRAPH_NODE_DATE)
                continue;
            move_len = graph_square_root(move_x[first_id] * move_x[first_id]
                                         + move_y[first_id] * move_y[first_id]);
            if (move_len < 0.0001)
                continue;
            if (move_len > temperature) {
                move_x[first_id] *= temperature / move_len;
                move_y[first_id] *= temperature / move_len;
            }
            position_x[first_id] += move_x[first_id];
            position_y[first_id] += move_y[first_id];
        }
        temperature = IDEAL_EDGE_LEN * 2.0 * (1.0 - (double)(step + 1) / GLOBAL_LAYOUT_STEPS)
                      + 0.5;
    }

    place_dates_beside_pages(site_graph, position_x, position_y);
    place_in_drawing(site_graph, position_x, position_y, MARGIN_LEFT, MARGIN_TOP,
                     MARGIN_RIGHT, MARGIN_BOTTOM);
    free(position_x);
    free(position_y);
    free(move_x);
    free(move_y);
    return 1;
}

/* Radius of a ring holding node_count nodes, at least minimum_radius. */
static double ring_radius(int node_count, double minimum_radius)
{
    double radius = (double)node_count * RING_NODE_SPACING / (2.0 * PI);

    return radius > minimum_radius ? radius : minimum_radius;
}

/* Lists the whole-graph ids of the local graph in local id order: the centre,
   ring 1 in id order (neighbour lists are sorted), then ring 2 in the order
   the ring-1 nodes reach it, so each sits near the node that reached it.
   is_member must hold node_count zeros. Returns the member count and sets
   *first_ring_count. */
static int collect_local_members(const graph *site_graph, int centre, int *members,
                                 char *is_member, int *first_ring_count)
{
    int member_count = 0;
    int ring_index;
    int position;
    int neighbour_id;

    members[member_count++] = centre;
    is_member[centre] = 1;
    for (position = site_graph->neighbour_start[centre];
         position < site_graph->neighbour_start[centre + 1]; position++) {
        members[member_count++] = site_graph->neighbours[position];
        is_member[site_graph->neighbours[position]] = 1;
    }
    *first_ring_count = member_count - 1;
    for (ring_index = 1; ring_index <= *first_ring_count; ring_index++) {
        for (position = site_graph->neighbour_start[members[ring_index]];
             position < site_graph->neighbour_start[members[ring_index] + 1]; position++) {
            neighbour_id = site_graph->neighbours[position];
            if (is_member[neighbour_id])
                continue;
            is_member[neighbour_id] = 1;
            members[member_count++] = neighbour_id;
        }
    }
    return member_count;
}

/* Copies the members and the edges among them into local_graph, renumbered.
   Returns 0 if memory runs out. */
static int copy_local_members(const graph *site_graph, graph *local_graph,
                              const int *members, int member_count)
{
    int *local_id = malloc((size_t)site_graph->node_count * sizeof(int));
    int node_id;
    int slot;
    int edge_index;
    int page_id;
    int key_id;

    local_graph->nodes = malloc((size_t)member_count * sizeof(graph_node));
    local_graph->edges = malloc((size_t)(site_graph->edge_count + 1) * sizeof(graph_edge));
    if (local_id == NULL || local_graph->nodes == NULL || local_graph->edges == NULL) {
        free(local_id);
        return 0;
    }
    for (node_id = 0; node_id < site_graph->node_count; node_id++)
        local_id[node_id] = -1;
    for (slot = 0; slot < member_count; slot++) {
        local_id[members[slot]] = slot;
        local_graph->nodes[slot] = site_graph->nodes[members[slot]];
    }
    local_graph->node_count = member_count;
    for (edge_index = 0; edge_index < site_graph->edge_count; edge_index++) {
        page_id = local_id[site_graph->edges[edge_index].page_node];
        key_id = local_id[site_graph->edges[edge_index].key_node];
        if (page_id == -1 || key_id == -1)
            continue;
        local_graph->edges[local_graph->edge_count].page_node = page_id;
        local_graph->edges[local_graph->edge_count].key_node = key_id;
        local_graph->edge_count++;
    }
    free(local_id);
    return 1;
}

/* The centre at the origin and each ring evenly spaced, starting at the top
   and going clockwise. */
static void place_rings(graph *local_graph, int first_ring_count, double *position_x,
                        double *position_y)
{
    int second_ring_count = local_graph->node_count - 1 - first_ring_count;
    double first_radius = ring_radius(first_ring_count, FIRST_RING_MIN_RADIUS);
    double second_radius = ring_radius(second_ring_count, first_radius + RING_GAP);
    double angle;
    int slot;

    position_x[0] = 0.0;
    position_y[0] = 0.0;
    for (slot = 1; slot < local_graph->node_count; slot++) {
        if (slot <= first_ring_count) {
            angle = 2.0 * PI * (double)(slot - 1) / (double)first_ring_count - PI / 2.0;
            position_x[slot] = first_radius * graph_cosine(angle);
            position_y[slot] = first_radius * graph_sine(angle);
        } else {
            angle = 2.0 * PI * (double)(slot - 1 - first_ring_count)
                    / (double)second_ring_count - PI / 2.0;
            position_x[slot] = second_radius * graph_cosine(angle);
            position_y[slot] = second_radius * graph_sine(angle);
        }
    }
    place_in_drawing(local_graph, position_x, position_y, LOCAL_MARGIN_SIDES,
                     LOCAL_MARGIN_ENDS, LOCAL_MARGIN_SIDES, LOCAL_MARGIN_ENDS);
}

graph *graph_build_local(const graph *site_graph, int centre)
{
    graph *local_graph = create_empty_graph();
    int *members = malloc((size_t)site_graph->node_count * sizeof(int));
    char *is_member = calloc((size_t)site_graph->node_count, 1);
    double *position_x = malloc((size_t)site_graph->node_count * sizeof(double));
    double *position_y = malloc((size_t)site_graph->node_count * sizeof(double));
    int member_count;
    int first_ring_count;
    int has_succeeded = 0;

    if (local_graph != NULL && members != NULL && is_member != NULL && position_x != NULL
        && position_y != NULL) {
        local_graph->centre = 0;
        member_count = collect_local_members(site_graph, centre, members, is_member,
                                             &first_ring_count);
        if (copy_local_members(site_graph, local_graph, members, member_count)
            && build_neighbour_lists(local_graph)) {
            place_rings(local_graph, first_ring_count, position_x, position_y);
            has_succeeded = 1;
        }
    }

    /* Single cleanup point, so every failure frees the same things. */
    free(members);
    free(is_member);
    free(position_x);
    free(position_y);
    if (!has_succeeded) {
        graph_free(local_graph);
        return NULL;
    }
    return local_graph;
}

void graph_free(graph *site_graph)
{
    if (site_graph == NULL)
        return;
    free(site_graph->nodes);
    free(site_graph->edges);
    free(site_graph->neighbour_start);
    free(site_graph->neighbours);
    free(site_graph);
}
