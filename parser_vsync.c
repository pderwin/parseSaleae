#include <string.h>
#include "hdr.h"
#include "parseSaleae.h"
#include "parser.h"

#define MSB_FIRST 1

typedef struct {
    uint32_t vsync;
} sample_t;

static sample_t last_sample;
static sample_t sample;
static parser_t my_parser;

/*-------------------------------------------------------------------------
 *
 * name:        process_frame
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static void process_frame (parser_t *parser, frame_t *frame)
{
    (void) parser;

    /*
     * If we've started a new packet, then clear accumulation.
     */
    if (falling_edge(vsync)) {
	hdr(stdout_lf, frame->time_nsecs, "VSYNC");
	printf("FALL");
    }
    if (rising_edge(vsync)) {
	hdr(stdout_lf, frame->time_nsecs, "VSYNC");
	printf("RISE");
    }


    memcpy(&last_sample, &sample, sizeof(sample_t));
}

static signal_t my_signals[] =
{
   { "vsync", &sample.vsync },
   { NULL, NULL}
};

static parser_t my_parser =
{
   .name          = "vsync",
   .process_frame = process_frame,
   .signals       = my_signals
};

static void CONSTRUCTOR init (void)
{
    parser_register(&my_parser);
}
