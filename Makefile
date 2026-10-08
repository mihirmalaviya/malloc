all: memgrind tests

memgrind: memgrind.c mymalloc.c mymalloc.h
	gcc -std=c99 -g -Wall -o memgrind memgrind.c mymalloc.c

tests: tests.c mymalloc.c mymalloc.h
	gcc -std=c99 -g -Wall -o tests tests.c mymalloc.c

clean:
	rm -f memgrind tests
