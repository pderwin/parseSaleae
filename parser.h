#pragma once

#include "frame.h"
#include "log_file.h"
#include "signal.h"

typedef struct parser_s parser_t;

struct parser_s {
    char *name;

    /*
     * called upon successful wiring of the signals.
     */
    uint32_t (*connect)      (parser_t *parser);

    /*
     * Called after all connects are complete.
     */
    void     (*post_connect) (parser_t *parser);

    void     (*process_frame)(parser_t *parser, frame_t *frame);

    uint32_t enable;

    signal_t **signals;

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

uint32_t parser_active_count     (void);
void     parser_connect          (void);
void     parser_enable           (parser_t *parser);
void     parser_open_log_files   (void);
void     parser_post_connect     (void);
void     parser_process_frame    (frame_t *frame);
void     parser_redirect_output  (char *parser_name, log_file_t *lf);
void     parser_register         (parser_t *parser);
