#pragma once

#include <stdio.h>
#include "parseSaleae_types.h"

typedef struct log_file_s {
    FILE *fp;
    time_nsecs_t time_nsecs;
} log_file_t;

/*
 * Keep a log file struct for stdout.
 */
extern log_file_t *stdout_lf;

log_file_t *log_file_alloc (FILE *fp);
log_file_t *log_file_create(char *name);
