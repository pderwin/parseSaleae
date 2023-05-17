#pragma once

#include "frame.h"
#include "log_file.h"
#include "signal.h"

struct parser_s {
    char *name;

    uint32_t (*connect)      (void);
    void     (*post_connect) (void);
    void     (*process_frame)(frame_t *frame);

    uint32_t enable;

    signal_t *signals;

    char       *log_file;
    log_file_t *lf;

    uint32_t sample_time_nsecs;

    parser_t *next;
};

void parser_connect        (void);
void parser_post_connect   (void);
void parser_process_frame  (frame_t *frame);
void parser_redirect_output(char *parser_name, log_file_t *lf);
void parser_register       (parser_t *parser);
