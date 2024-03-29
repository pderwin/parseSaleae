#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "thread.h"

#define NUMBER_THREADS (32)

static thread_t threads[NUMBER_THREADS];

static thread_t *_thread_alloc (uint32_t address)
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

thread_t *thread_create (uint32_t address, char *name)
{
    thread_t
	*thread;

    /*
     * either find existing thread, or allocate a new one.
     */
    thread = thread_find(address);

    thread->name = strdup(name);

    return thread;
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

    return _thread_alloc(address);
}
