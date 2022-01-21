# date: 16.01.2022
# program: intmul
CC = gcc
FLAGS = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g 
LFLAGS = -lrt -lpthread 

OBJECTS=client.o server.o common.o

all: server client

client: client.o common.o
	$(CC) -o $@ $^ 

server: server.o common.o
	$(CC) -o $@ $^ 

%.o: %.c
	$(CC) $(FLAGS) -c -o $@ $<

client.o: client.c common.h
server.o: server.c common.h

clean:
	rm -rf *.o
