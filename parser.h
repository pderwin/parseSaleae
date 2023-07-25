#pragma once

#include "frame.h"
#include "log_file.h"
#include "signal.h"

struct parser_s {
    char *name;

    uint32_t (*connect)      (void);
    void     (*post_connect) (parser_t *parser);
    void     (*process_frame)(parser_t *parser, frame_t *frame);

    uint32_t enable;

    signal_t *signals;

    char       *log_file_name;
    log_file_t *log_file;

    uint32_t sample_time_nsecs;

    void       *data;

    parser_t *next;
};

/*
 * declaration of a log_fp local variable
 */
#define DECLARE_LOG_FP   FILE *log_fp = parser->log_file->fp

void parser_connect        (void);
void parser_post_connect   (void);
void parser_process_frame  (frame_t *frame);
void parser_redirect_output(char *parser_name, log_file_t *lf);
void parser_register       (parser_t *parser);
