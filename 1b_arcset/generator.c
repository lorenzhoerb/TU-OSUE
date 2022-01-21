#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include <time.h>
#include "error.h"
#include "graph.h"
#include "circular_buffer.h"

static char *prg_name;

static void get_vertices_from_str(int *from, int *to, char *str);
static int is_valid_argument(char *arg);
static void argument_handler(int argc, char **argv, graph_t *g);
static void gen_arcset(graph_t *input, graph_t *output);
static void usage(void);

int main(int argc, char **argv)
{
    srand(time(0));

    prg_name = argv[0];
    graph_t graph = {0, 0, 0, 0, NULL, NULL};
    graph_t arc_set = {0, 0, 0, 0, NULL, NULL};

    argument_handler(argc, argv, &graph);
    gen_arcset(&graph, &arc_set);
    print_edges(&arc_set);
    // write_buf(graph);
    return 0;
}

/**
 * @brief checks if all input arguments are valid and puts them into data structure
 * 
 * @param argc argument counter
 * @param argv argument vector
 */
static void argument_handler(int argc, char **argv, graph_t *g)
{
    if (argc - optind < 1)
        usage();

    int i;
    for (i = optind; i < argc; i++)
    {
        int from, to;
        get_vertices_from_str(&from, &to, argv[i]);

        insert_vertex(g, from);
        insert_vertex(g, to);
        insert_edge(g, from, to);
    }
}

/**
 * @brief generates a heuristic arcset from input and saves it into output
 * 
 * @param input input graph
 * @param output output graph (heuristic arcset)
 */
static void gen_arcset(graph_t *input, graph_t *output)
{
    int i, from, to;
    shuffle_vertices(input);
    for (i = 0; i < input->e_top; i++)
    {
        from = input->edges[i];
        to = input->edges[++i];

        if (index_of_vertex(input, from) > index_of_vertex(input, to))
        {
            insert_vertex(output, from);
            insert_vertex(output, to);
            insert_edge(output, from, to);
        }
    }
}

/**
 * @brief Takes an argument string (eg. "1-3") and extracts the vertecies. 
 * This functions throws an error if the sting is an illigal argument.
 * Saves the result in from and to (eg. from = 1, to = 3)
 * 
 * @param from vertex from
 * @param to vertex to
 * @param str input argument [0..n]-[0..m] n,m are natural numbers
 */
static void get_vertices_from_str(int *from, int *to, char *str)
{
    if (!is_valid_argument(str))
        ERROR_MSG("illigal argument format", prg_name);

    int index = 0;
    for (; *(str + index) != '\0'; index++)
    {
        if (*(str + index) == '-')
        {
            *(str + index) = '\0';
            *from = atoi(str);
            *to = atoi((str + index) + 1);
        }
    }
}

/**
 * @brief checks if string matches [0..n]-[0..m]
 *  examples: 1-2 3-4
 * 
 * @param arg 
 * @return int returns 1 for match and 0 for no match
 */
static int is_valid_argument(char *arg)
{
    regex_t regex;
    int reti;

    /* Compile regular expression */
    reti = regcomp(&regex, "^[0-9][0-9]*-[0-9][0-9]*$", 0);
    if (reti)
    {
        ERROR_MSG("Regex compile error", prg_name);
        exit(EXIT_FAILURE);
    }

    reti = regexec(&regex, arg, 0, NULL, 0);
    return !reti;
}

/**
 * @brief This function writes helpful usage information about the program to stderr.
 * @details global variables: pgm_name
 */
static void usage(void)
{
    (void)fprintf(stderr, "USAGE: %s EDGE1...\n", prg_name);

    exit(EXIT_FAILURE);
}
