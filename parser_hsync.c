#include <string.h>
#include "hdr.h"
#include "parseSaleae.h"

#define MSB_FIRST 1

typedef struct {
    uint32_t hsync;
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
static void process_frame (frame_t *frame)
{

    /*
     * If we've started a new packet, then clear accumulation.
     */
    if (falling_edge(hsync)) {
	hdr(stdout_lf, frame->time_nsecs, "HSYNC");
	printf("FALL");
    }

    memcpy(&last_sample, &sample, sizeof(sample_t));
}

static signal_t my_signals[] =
{
   { "hsync", &sample.hsync },
   { NULL, NULL}
};

static parser_t my_parser =
{
   .name          = "hsync",
   .process_frame = process_frame,
   .signals       = my_signals
};

static void CONSTRUCTOR init (void)
{
    parser_register(&my_parser);
}
