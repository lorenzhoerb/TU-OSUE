FLAGS = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L 
LFLAGS = -lrt -lpthread 

all: generator.o supervisor.o circular_buffer.o error.o graph.o
	gcc -o generator generator.o error.o graph.o 
	gcc -o supervisor supervisor.o error.o
	gcc -o circular_buffer circular_buffer.o error.o graph.o $(LFLAGS)
generator.o: generator.c
	gcc $(FLAGS) -g -c -o generator.o generator.c
supervisor.o: supervisor.c
	gcc $(FLAGS) -g -c -o supervisor.o supervisor.c
circular_buffer.o: circular_buffer.c
	gcc $(FLAGS) -g -c -o circular_buffer.o circular_buffer.c 
error.o: error.c
	gcc $(FLAGS) -g -c -o error.o error.c
graph.o: graph.c
	gcc $(FLAGS) -g -c -o graph.o graph.c
clean:
	rm -rf *.o