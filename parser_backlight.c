#include <string.h>
#include "hdr.h"
#include "parseSaleae.h"

#define MSB_FIRST 1

typedef struct {
    uint32_t bl_pwm;
} sample_t;

static sample_t last_sample;
static sample_t sample;

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
    if (falling_edge(bl_pwm)) {
	hdr(stdout_lf, frame->time_nsecs, "BACKLIGHT");
	printf("FALL");
    }
    if (rising_edge(bl_pwm)) {
	hdr(stdout_lf, frame->time_nsecs, "BACKLIGHT");
	printf("RISE");
    }


    memcpy(&last_sample, &sample, sizeof(sample_t));
}

static signal_t my_signals[] =
{
   { "bl_pwm", &sample.bl_pwm },
   { NULL, NULL}
};

static parser_t my_parser =
    {
     .name          = "backlight",
     .process_frame = process_frame,
     .signals       = my_signals
    };

static void CONSTRUCTOR init (void)
{
    parser_register(&my_parser);
}
