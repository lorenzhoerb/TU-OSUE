#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "error.h"
#include "graph.h"
#include "circular_buffer.h"

volatile sig_atomic_t quit = 0;

void handle_signal(int signal) { quit = 1; }

int main(int argc, char **argv)
{
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);

    while (!quit)
    {
    }

    return 0;
}