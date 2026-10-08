/*
 * memgrind.c
 *
 * CS 214 Project 1
 * Authors: Jay Boghawala (jvb73), Mihir Malaviya (mm4186)
 *
 * Stress test: runs 5 malloc/free workloads 50 times and prints the
 * average time per run
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for memset
#include <sys/time.h> // for gettimeofday

#include "mymalloc.h" // makes malloc/free go to mymalloc/myfree

#define RUNS 50

struct timeval start, end;

// allocate objects of sizes 8, 16, 32, 64, 128, 512, 1024
// then free them in reverse order
static void task1(void){
    int sizes[] = {8, 16, 32, 64, 128, 512, 1024};
    char *ptrs[7];

    for (int i=0; i<7; i++)
        ptrs[i] = malloc(sizes[i]);

    for (int i=6; i>=0; i--)
        free(ptrs[i]);
}

// malloc 120 tiny objects then free them in the same order we made them
static void task2(void){
    char *ptrs[120];

    for (int i=0; i<120; i++)
        ptrs[i] = malloc(1);

    for (int i=0; i<120; i++)
        free(ptrs[i]);
}

// randomly either malloc 1 byte or free one we already have
// keep going until 120 mallocs then free whatevers left
static void task3(void){
    char *ptrs[120];
    int count = 0; 
    int total = 0; // mallocs done

    while (total<120) {
        if (count==0 || rand()%2 == 0) {
            ptrs[count]=malloc(1);
            count++;
            total++;
        } else {
            int i = rand()%count;
            free(ptrs[i]);
            ptrs[i]=ptrs[count-1]; // swap trick, so every index left of count is filled still
            count--;
        }
    }

    for (int i=0; i<count; i++){
        free(ptrs[i]);
    }
}

#define HEAPSIZE 4096 // has to match MEMLENGTH in mymalloc.c
#define HEADERSIZE 8
#define OBJS 64
#define OBJSIZE (HEAPSIZE / OBJS - HEADERSIZE) // 56

// makes sure obj i still has byte i everywhere
static void check_obj(char *p, int size, int i){
    for (int j=0; j<size; j++){
        if (p[j] != i){
            printf("task4: object %d got corrupted\n", i);
            return;
        }
    }
}

// fill the heap, free every other one, refill the holes with random sizes
// then free everything
static void task4(void){
    char *ptrs[OBJS];
    int sizes[OBJS];

    // each hole is a 64 byte chunk, so these hit different cases
    //   56 -> exact fit, nothing left over
    //   48 -> 8 left over, too small to split so it gets the whole chunk
    //   40 -> 16 left over, split
    //    1 -> 48 left over, split
    int refill[] = {56, 48, 40, 1};

    // fill the heap exactly, each object gets its own byte
    for (int i=0; i<OBJS; i++){
        ptrs[i] = malloc(OBJSIZE);
        sizes[i] = OBJSIZE;
        memset(ptrs[i], i, OBJSIZE);
    }

    // free every other one
    for (int i=0; i<OBJS; i+=2)
        free(ptrs[i]);

    // the ones we kept shouldnt have been touched
    for (int i=1; i<OBJS; i+=2)
        check_obj(ptrs[i], sizes[i], i);

    // refill the holes with the different sizes randomly
    for (int i=0; i<OBJS; i+=2){
        sizes[i] = refill[rand()%4];
        ptrs[i] = malloc(sizes[i]);
        if (ptrs[i]==NULL){
            printf("task4: couldnt refill hole %d\n", i);
            sizes[i] = 0;
            continue;
        }
        memset(ptrs[i], i, sizes[i]);
    }

    // everything should still be intact
    for (int i=0; i<OBJS; i++)
        check_obj(ptrs[i], sizes[i], i);

    // free everything in order
    for (int i=0; i<OBJS; i++)
        free(ptrs[i]);
}

// fill the whole heap with tiny objects, then free them in a random order
// malloc(1) uses the smallest chunk (16 bytes) so 256 of them fill the heap exactly
#define SMALLOBJS (HEAPSIZE/16)

static void task5(void){
    char *ptrs[SMALLOBJS];
    int count = 0;

    for (int i=0; i<SMALLOBJS; i++){
        ptrs[i]=malloc(1);
        count++;
    }

    // pick a random one to free, then move the last one into its spot
    // (same swap trick as task3) so we never free the same one twice
    while (count>0){
        int i=rand()%count;
        free(ptrs[i]);
        ptrs[i]=ptrs[count-1];
        count--;
    }

    // if everything merged back into one chunk the whole heap fits again
    char *big = malloc(HEAPSIZE-HEADERSIZE);
    if (big==NULL)
        printf("task5: heap didnt merge back into one chunk\n");
    free(big);
}


int main(void)
{
    // record the start time
    gettimeofday(&start, NULL);

    for (int i=0; i<RUNS; i++){
        task1();
        task2();
        task3();
        task4();
        task5();
    }

    gettimeofday(&end, NULL);

    // elapsed in microseconds
    long elapsed = (end.tv_sec-start.tv_sec)*1000000 + (end.tv_usec-start.tv_usec);
    printf("average time per run: %ld microseconds\n", elapsed/RUNS);

    return 0;
}
