#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <trace.h>  // tag enumeration from Zephyr sources.
#include "hdr.h"
#include "parseSaleae.h"

typedef struct {
    uint32_t lbt_clk;
    uint32_t lbt_dta;
    uint32_t lbt_pkt;
    uint32_t lbt_trg;
} sample_t;

static sample_t last_sample;
static sample_t sample;
static uint32_t accumulated_bits;
static uint32_t accumulated_word;
static parser_t my_parser;

/*
 * packet sample frequency to support filtering of i2c_clk signal.
 */
#define SAMPLE_TIME_NSECS (25)

/*
 * Set the time for de-glitch
 */
#define DEGLITCH_TIME (3 * SAMPLE_TIME_NSECS)


/*-------------------------------------------------------------------------
 *
 * name:        process_frame
 *
 * description: wrapper to guarantee that the frame is copied to the 'last
 *              frame' every time.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static void process_frame (frame_t *frame)
{
    /*
     * Clear the accumulator at the start of a packet
     */

    if (rising_edge(lbt_pkt)) {
	accumulated_bits = 0;
	accumulated_word = 0;
    }

    /*
     * Parse the accumulated bytes at the end of a packet
     */
    if (falling_edge(lbt_pkt)) {
	lbtrace_packet_parse();
    }

    if (sample.lbt_pkt == 0) {
	memcpy(&last_sample, &sample, sizeof(sample_t));
	return;
    }

#if 1
    /*
     * debounce of the lbt_clk wire.  Seeing a slow falling edge on newer cards.
     */
    {
	static uint32_t current_lbt_clk = 0;
	static uint32_t countdown       = 0;
	static uint32_t first_frame     = 1;

	/*
	 * To keep from thinking we have a rising edge of clock at the beginning of debounce, we need to set our
	 * 'current' to our first entry.
	 */
	if (first_frame) {
	    current_lbt_clk = last_sample.lbt_clk = sample.lbt_clk;
	    first_frame = 0;
	    return;
	}

	/*
	 * The crisp clock is essential to our parsing ability.  Filter it for 250
	 * nSecs.
	 *
	 * When it changes states, set counter and just return.
	 */
	if (sample.lbt_clk != current_lbt_clk) {

	    if (countdown == 0) {
		countdown = DEGLITCH_TIME / SAMPLE_TIME_NSECS;
		return;
	    }
	    else {
		/*
		 * Here, we already have a countdown value, so if we decrement it and
		 * it becomes 0, then we can continue.  When non-zero, just return.
		 */
		if (--countdown) {
		    return;
		}
	    }

	    /*
	     * If we get here, then we have debounced it.  Remember current state.
	     */
	    current_lbt_clk = sample.lbt_clk;

	} // sample != current
    }
#endif


    /*
     * If dbg_packet is on, and we see a rising edge of clock, then sample the
     * data.
     */
    if (rising_edge(lbt_clk)) {

	if (sample.lbt_pkt) {

	    accumulated_word |= (sample.lbt_dta << (31 - accumulated_bits));
	    accumulated_bits++;

	    if (accumulated_bits == 32) {

		lbtrace_packet_add_word(frame->time_nsecs, accumulated_word);

		accumulated_bits = 0;
		accumulated_word = 0;
	    }

	}
    }


    memcpy(&last_sample, &sample, sizeof(sample_t));
}

/*-------------------------------------------------------------------------
 *
 * name:        connect
 *
 * description: finish initialization when this parser is enabled.
 *
 * input:
 *
 * output:      1 - keep the parser enabled
 *
 *-------------------------------------------------------------------------*/
static uint32_t connect (void)
{
    lbtrace_tag_scan();
    return 1;
}

static signal_t my_signals[] =
{
   { "LBT_CLK", &sample.lbt_clk },
   { "LBT_DTA", &sample.lbt_dta },
   { "LBT_PKT", &sample.lbt_pkt },
   { "LBT_TRG", &sample.lbt_trg },
   { NULL, NULL}
};

static parser_t my_parser =
{
   .name              = "lbtrace",
   .process_frame     = process_frame,
   .signals           = my_signals,
   .sample_time_nsecs = SAMPLE_TIME_NSECS,
   .connect           = connect
};

static void CONSTRUCTOR init (void)
{
    parser_register(&my_parser);
}
