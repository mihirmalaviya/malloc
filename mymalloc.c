/*
 * mymalloc.c
 *
 * CS 214 Project 1
 * Authors: Jay Boghawala (jvb73), Mihir Malaviya (mm4186)
 *
 * A replacement for malloc() and free() that allocates from a fixed
 * static array and reports common usage errors
 */

#include <stdio.h>
#include <stdlib.h>

#include "mymalloc.h"


/* ------------------------------------------------------------------ */
/* The heap, the header, and the basic helpers                         */
/* ------------------------------------------------------------------ */

#define MEMLENGTH 4096

static union {
    char bytes[MEMLENGTH];
    double not_used; // to align with 8 bytes
} heap;

static int initialized = 0;

// 8 bytes total so payloads always land on a multiple of 8
typedef struct{
    int size;   // whole chunk, header + payload
    int in_use; // 1 = taken, 0 = free
} header;

// smallest chunk we can have is header + 8 bytes of payload
#define MINCHUNK (sizeof(header) + 8)

// rounds up to a multiple of 8. multiples of 8 end in 000 in binary
// so we add 7 and then force the last 3 bits to 0
static size_t
round_up8(size_t n){
    return(n+7)&~7;
}

// skips past the header to where the user's data starts
static void*
payload_of(header *h){
    return (char*)h+sizeof(header);
}


// jumps to the next chunk, gives NULL if we're at the end of the heap
static header *
next_chunk(header *h){
    char *nexthead=(char*)h+ h->size;
    if (nexthead>=heap.bytes + MEMLENGTH){
        return NULL;
    }
    return (header*)nexthead;
}

// runs at exit, counts whatever's still in use and prints it if theres any
// (cant call exit() in here)
static void
report_leaks(void){
    header *h= (header*)heap.bytes;
    int objcounter=0;
    int payloadsizetotal=0;
    while (h!=NULL){
        if(h->in_use){
            objcounter+=1;
            payloadsizetotal+=h->size-sizeof(header);
        }
        h=next_chunk(h);
    }
    if(objcounter>0){
        fprintf(stderr,"mymalloc: %d bytes leaked in %d objects.\n", payloadsizetotal,objcounter);
    }
}

// heap starts as one big free chunk
static void
init_heap(void){
    header *h=(header*)heap.bytes;
    h->size=MEMLENGTH;
    h->in_use=0;
    initialized=1;
    atexit(report_leaks);
}

/* ------------------------------------------------------------------ */
/* mymalloc                                                           */
/* ------------------------------------------------------------------ */

void *
mymalloc (size_t size, char *file, int line)
{
    if(!initialized){
        init_heap();
    }

    // bigger than the whole heap so it'll never fit, also stops round_up8 from overflowing
    if(size>MEMLENGTH){
        fprintf(stderr, "malloc: Unable to allocate %zu bytes (%s:%d)\n", size, file, line);
        return NULL;
    }

    // payload rounded up + header. malloc(0) just gets the smallest chunk
    header *h = (header*)heap.bytes;
    size_t chunksize=round_up8(size)+sizeof(header);

    if(chunksize<MINCHUNK){
        chunksize=MINCHUNK;
    }

    // go chunk by chunk and grab the first free one thats big enough
    while(h!=NULL){

        if(h->in_use==0 && h->size>=chunksize){
            // if theres enough left over for its own chunk we split it off,
            // otherwise they just get the whole thing
            size_t spaceleft = h->size - chunksize;
            if(spaceleft>=MINCHUNK){
                header *leftover=(header*)((char*)h+chunksize);
                leftover->size =spaceleft;
                leftover->in_use=0;
                h->size=chunksize;
            }
            h->in_use=1;
            return payload_of(h);
        }
        h=next_chunk(h);

    }

    // went through everything and nothing fit
    fprintf(stderr, "malloc: Unable to allocate %zu bytes (%s:%d)\n", size, file, line);

    return NULL;
}


/* ------------------------------------------------------------------ */
/* myfree                                                             */
/* ------------------------------------------------------------------ */

void
myfree (void *ptr, char *file, int line)
{
    if(!initialized){
        init_heap();
    }

    // free(NULL) does nothing, same as the real free
    if(ptr==NULL){
        return;
    }

    // look for the chunk that ptr belongs to. we keep prevh cuz we cant
    // walk backwards and we need it to merge with the chunk before
    header *h = (header*)heap.bytes;
    header *prevh = NULL;

    while(h!=NULL){
        if(payload_of(h)==ptr){
            // already free so this is a double free
            if(h->in_use==0){
                fprintf(stderr,"free: Inappropriate pointer (%s:%d)\n",file,line);
                exit(2);
            }
            h->in_use=0;

            // merge with the next chunk if its free
            header *nextchunk = next_chunk(h);
            if(nextchunk!=NULL && nextchunk->in_use==0){
                h->size=h->size + nextchunk->size;
            }
            // merge with the previous chunk if its free
            if(prevh!=NULL && prevh->in_use==0){
                prevh->size=prevh->size + h->size;
            }
            return;
        }
        prevh=h;
        h=next_chunk(h);
    }

    // ptr didnt match any chunk, so its either not from the heap or points into the middle of one
    fprintf(stderr, "free: Inappropriate pointer (%s:%d)\n", file, line);

    exit(2);
}
