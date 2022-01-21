#ifndef GRAPH_OS
#define GRAPH_OS

typedef struct graph
{
    unsigned int v_top;
    unsigned int v_cap;
    unsigned int e_top;
    unsigned int e_cap;
    int *vertices;
    int *edges; // saves one edge: [from1, to1, from2, to2, ...]
} graph_t;
#endif

int graph_contains_vertex(graph_t *g, int vertex);
void insert_vertex(graph_t *g, int vertex);
void insert_edge(graph_t *g, int from, int to);
void shuffle_vertices(graph_t *g);
int random_int(int min, int max);
void print_vertecies(graph_t *g);
void print_edges(graph_t *g);
int index_of_vertex(graph_t *g, int vertex);
void swap_vertices(graph_t *g, int i, int j);
