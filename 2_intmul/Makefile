# author: Lorenz Hörburger <12024737>
# date: 12.12.2021
# program: intmul
CC = gcc
FLAGS = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g 
LFLAGS = -lrt -lpthread 

OBJECTS=intmul.o

all: intmul

intmul: $(OBJECTS)
	$(CC) -o $@ $^ 

%.o: %.c
	$(CC) $(FLAGS) -c -o $@ $<

clean:
	rm -rf *.o

intmul.o: intmul.c
