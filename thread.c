#include <stdio.h>
#include <stdlib.h>
#include "thread.h"

#define NUMBER_THREADS (32)

static thread_t threads[NUMBER_THREADS];

thread_t *thread_alloc (uint32_t address)
{
    uint32_t i;
    thread_t *tp;

    tp = threads;

    for (i=0; i<NUMBER_THREADS; i++,tp++) {
	if (! tp->in_use) {

	    tp->in_use = 1;
	    tp->address = address;
	    tp->idx = i;
	    return tp;
	}
    }

    if (i == NUMBER_THREADS) {
	fprintf(stderr, "Error allocating a thread structure\n");
	exit(1);
    }

    return NULL;
}

thread_t *thread_find (uint32_t address)
{
    uint32_t i;
    thread_t *tp;

    tp = threads;

    for (i=0; i<NUMBER_THREADS; i++,tp++) {
	if (tp->in_use && (tp->address == address) ) {
	    return tp;
	}
    }

    return thread_alloc(address);
}
