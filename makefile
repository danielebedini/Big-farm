# definizione del compilatore e dei flag di compilazione
# che vengono usate dalle regole implicite
CC=gcc
CFLAGS=-g -Wall -O -std=c99
LDLIBS=-lm -lrt -pthread

all: farm client

farm: farm.o xerrori.o
client: client.o xerrori.o

clean:
	rm client farm *.o