#include <stdio.h>
#include <stdlib.h>
#include "graph.h"
#include "error.h"

/**
 * @brief gets index of vertex in graph -> vertices
 * 
 * @param g graph
 * @param vertex vertex
 * @return int returns the index of vertex in graph.vertices, if vertex does not exist in vertices set returns -1
 */
int index_of_vertex(graph_t *g, int vertex)
{
    int i;
    for (i = 0; i < g->v_top; i++)
    {
        if (g->vertices[i] == vertex)
            return i;
    }
    return -1;
}

/**
 * @brief shuffles all vertices in graph randomly
 * 
 * @param g graph
 */
void shuffle_vertices(graph_t *g)
{
    int i, r;

    for (i = g->v_top - 1; i >= 1; i--)
    {
        r = random_int(0, i);
        swap_vertices(g, i, r);
    }
}

/**
 * @brief swaps position of two vertices in graph
 * 
 * @param g graph
 * @param i vertex1 index
 * @param j vertex2 index
 */
void swap_vertices(graph_t *g, int i, int j)
{
    if (i < 0 || j < 0 || i >= g->v_top || j >= g->v_top)
        ERROR_MSG("index out of bound", "");

    int tmp = g->vertices[i];
    g->vertices[i] = g->vertices[j];
    g->vertices[j] = tmp;
}

/**
 * @brief prints all vertecies in graph g
 * ( for debugging ) 
 * @param g graph
 */
void print_vertecies(graph_t *g)
{
    int *p;

    for (p = g->vertices; p < g->vertices + g->v_top; ++p)
        printf("%d, ", *p);
    printf("\n");
}

/**
 * @brief prints all edges in graph g
 * ( for debugging ) 
 * @param g graph
 */
void print_edges(graph_t *g)
{
    int i, from, to;

    for (i = 0; i < g->e_top; i++)
    {

        from = g->edges[i];
        to = g->edges[++i];

        printf("%d-%d ", from, to);
    }
    printf("\n");
}

/**
 * @brief Insert a vertex in the graphs vertex set (no duplicates)
 * 
 * @param g graph
 * @param vertex vertex to add
 */
void insert_vertex(graph_t *g, int vertex)
{
    if (!graph_contains_vertex(g, vertex))
    {
        if (g->v_top == g->v_cap)
        {
            int newcap = g->v_cap + 10;
            int *newptr = realloc(g->vertices, sizeof(int) * newcap);
            if (newptr == NULL)
                ERROR_MSG("realloc error", "");
            g->vertices = newptr;
            g->v_cap = newcap;
        }
        g->vertices[g->v_top++] = vertex;
    }
}

/**
 * @brief insertes a edge into a graph
 * 
 * @param g graph
 * @param from vertex from
 * @param to vertex to
 */
void insert_edge(graph_t *g, int from, int to)
{
    if (g->e_top == g->e_cap)
    {
        int newcap = g->e_cap + 10;
        int *newptr = realloc(g->edges, sizeof(int) * newcap);
        if (newptr == NULL)
            ERROR_MSG("realloc error", "");
        g->edges = newptr;
        g->e_cap = newcap;
    }
    g->edges[g->e_top++] = from;
    g->edges[g->e_top++] = to;
}

/**
 * @brief checks if a graph already contains the vertex
 * 
 * @param g graph
 * @param vertex vertex to check
 * @return int returns 1 if the graph already contains the vertex, returns 0 if not
 */
int graph_contains_vertex(graph_t *g, int vertex)
{
    int i;
    for (i = 0; i < g->v_top; i++)
    {
        if (g->vertices[i] == vertex)
            return 1;
    }
    return 0;
}

/**
 * @brief Generates a random int i: min <= i <= max
 * 
 * @param min 
 * @param max 
 * @return int 
 */
int random_int(int min, int max)
{
    return (rand() % (max - min + 1)) + min;
}
