/*
 * tests.c
 *
 * CS 214 Project 1
 * Authors: Jay Boghawala (jvb73), Mihir Malaviya (mm4186)
 *
 * correctness tests for section 3
 *
 *   ./tests            runs the normal tests, prints PASS or FAIL for each
 *   ./tests notmalloc  free(&x), should error and exit 2
 *   ./tests middle     free(p+1), should error and exit 2
 *   ./tests twice      free the same pointer twice, should error and exit 2
 *   ./tests leak       leaks memory, should print 48 bytes leaked in 2 objects
 *
 * check the exit code with echo $?
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mymalloc.h"

#define HEAPSIZE 4096 // has to match MEMLENGTH in mymalloc.c
#define HEADERSIZE 8

int failures = 0;

static void check(char *name, int passed){
    printf("%s %s\n", passed ? "PASS" : "FAIL", name);
    if (!passed)
        failures++;
}


// fill the heap with objects that each hold their own byte, then check none got overwritten
static void test_no_overlap(void){
    int n=64;
    int size=HEAPSIZE/n-HEADERSIZE; // fills the heap exactly
    char *ptrs[64];
    int ok=1;

    for (int i=0; i<n; i++){
        ptrs[i] = malloc(size);
        if (ptrs[i]==NULL){ // heap should fit all of them, so this is a fail
            ok=0;
            continue;
        }
        memset(ptrs[i], i, size);
    }

    for (int i=0; i<n; i++)
        for (int j=0; ptrs[i]!=NULL && j<size; j++)
            if (ptrs[i][j]!=i)
                ok=0;

    for (int i=0; i<n; i++)
        free(ptrs[i]);

    check("objects dont overlap", ok);
}

// malloc the whole heap, free it, then malloc the whole heap again
static void test_free_deallocates(void){
    char *p = malloc(HEAPSIZE - HEADERSIZE);
    free(p);
    char *q = malloc(HEAPSIZE - HEADERSIZE);
    check("free gives memory back", p!=NULL && q!=NULL);
    free(q);
}

// example from the pdf: fill the heap with 24 byte chunks, free them all, then malloc 48
// it only fits if the chunks merged
static void test_coalesce(void){
    int n = HEAPSIZE / (24 + HEADERSIZE); // 128
    char *ptrs[128];

    for (int i=0; i<n; i++)
        ptrs[i]=malloc(24);
    for (int i=0; i<n; i++)
        free(ptrs[i]);

    char *p = malloc(48);
    check("free chunks next to each other merge", p!=NULL);
    free(p);
}

// fill the heap with 3 chunks, free the outside two, then the middle one
// the middle has to merge with both neighbors or the whole heap wont fit
static void test_coalesce_both_sides(void){
    char *a = malloc(HEAPSIZE/4 - HEADERSIZE);
    char *b = malloc(HEAPSIZE/4 - HEADERSIZE);
    char *c = malloc(HEAPSIZE/2 - HEADERSIZE);
    free(a);
    free(c);
    free(b);
    char *p = malloc(HEAPSIZE - HEADERSIZE);
    check("middle chunk merges with both neighbors", a && b && c && p);
    free(p);
}

// should print an error and return NULL
static void test_too_big(void){
    check("malloc too big returns NULL", malloc(HEAPSIZE+1)==NULL);
}


// bad frees, each should print an error and exit(2)
// they run one at a time from the command line since exit(2) ends the program

static void free_not_from_malloc(void){
    int x;
    free(&x);
    printf("FAIL free on something not from malloc\n");
}

static void free_middle_of_chunk(void){
    int *p = malloc(sizeof(int)*2);
    free(p+1);
    printf("FAIL free in the middle of a chunk\n");
}

static void free_twice(void){
    int *p = malloc(8);
    free(p);
    free(p);
    printf("FAIL free the same pointer twice\n");
}

// leaks 2 objects on purpose
static void leak_two(void){
    malloc(20);
    malloc(20);
}


int main(int argc, char **argv)
{
    if (argc==1){
        test_no_overlap();
        test_free_deallocates();
        test_coalesce();
        test_coalesce_both_sides();
        test_too_big();
        printf("%d failed\n", failures);
        return failures?1:0;
    }

    if (strcmp(argv[1], "notmalloc")==0)
        free_not_from_malloc();
    else if (strcmp(argv[1], "middle")==0)
        free_middle_of_chunk();
    else if (strcmp(argv[1], "twice")==0)
        free_twice();
    else if (strcmp(argv[1], "leak")==0)
        leak_two();
    else
        printf("usage: ./tests [notmalloc | middle | twice | leak]\n");

    return 0;
}
