#pragma once

#include <stdint.h>

typedef struct thread_s {
    uint32_t in_use;
    uint32_t idx;
    uint32_t address;
    char     *name;
} thread_t;

thread_t *thread_create (uint32_t address, char *name);
thread_t *thread_find   (uint32_t address);
