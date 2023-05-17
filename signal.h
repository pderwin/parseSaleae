#pragma once

#include <stdint.h>

typedef struct parser_s parser_t;

typedef struct signal_s {
    char     *name;
    uint32_t *val_p;
    uint32_t format; // format to use when calling csv_find.
    time_nsecs_t deglitch_nsecs; // if we want to deglitch this signal, this is number of nSecs to do it.

    /*
     * internal variables.
     */
    uint32_t _find_rc;

    uint32_t     _raw;  // csv will always drop the signal value here.

    uint32_t     _buffered_raw;       // track how the raw wire is bouncing around.
    time_nsecs_t _last_buffered_change_nsecs;

} signal_t;


void signal_deglitch (parser_t *parser, time_nsecs_t time_nsecs);
