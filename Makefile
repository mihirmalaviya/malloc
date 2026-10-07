CC = gcc
CFLAGS = -std=c99 -g -Wall

all: memgrind tests

memgrind: memgrind.c mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) -o memgrind memgrind.c mymalloc.c

tests: tests.c mymalloc.c mymalloc.h
	$(CC) $(CFLAGS) -o tests tests.c mymalloc.c
