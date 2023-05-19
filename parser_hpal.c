#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hdr.h"
#include "parseSaleae.h"
#include "parser.h"

typedef struct {
    uint32_t clk;
    uint32_t data;
    uint32_t rxf_n;
    uint32_t wr_n;
    uint32_t sample_number;
} sample_t;

static sample_t last_sample;
static sample_t sample;

/*
 * packet sample frequency to support filtering of i2c_clk signal.
 */
#define SAMPLE_TIME_NSECS (25)

/*
 * Set the time for de-glitch
 */
#define DEGLITCH_TIME (5 * SAMPLE_TIME_NSECS)


typedef enum
{
    IDLE,
    READ_TURNAROUND,
    READ,
    READ_DONE,
} state_t;

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
    static uint32_t
	offset     = 0;
    static state_t
	state = IDLE;

    (void) parser;
    (void) frame;

    /*
     * If dbg_packet is on, and we see a rising edge of clock, then sample the
     * data.
     */
    if (rising_edge(clk)) {

	switch(state) {

	case IDLE:
	    if (last_sample.wr_n == 0) {
		printf("%d packet start\n", sample.sample_number);
		state = READ_TURNAROUND;
	    }
	    break;

	case READ_TURNAROUND:
	    if (last_sample.rxf_n == 1) {
		break;
	    }
	    state = READ;
	    /* fall through */

	    /*
	     * When here, the RXF_N signal has been seen low.  When it goes
	     * back high, we exit this state.
	     */
	case READ:
	    if (last_sample.wr_n == 1) {
		printf("%d: ERROR: packet abort\n", sample.sample_number);
		//		    exit(1);
	    }

	    if (sample.rxf_n == 1) {
		state = READ_DONE;
	    }
	    else {

		printf("%d | FT601   | %4x | ......%02x | %d\n", (int32_t) sample.sample_number, offset, last_sample.data, offset);

		/*
		 * We only see one byte of every 4 transferred
		 */
		offset += 4;
	    }
	    break;

	case READ_DONE:
	    /*
	     * wait for the WR_N signal to be high.
	     */
	    if (sample.wr_n == 1) {
		printf("%d packet end\n", sample.sample_number);
		state = IDLE;
	    }
	    break;

	default:
	    printf("Unhandled state: %d \n", state);
	    exit(1);

	} // switch
    }

    memcpy(&last_sample, &sample, sizeof(sample_t));
}


static signal_t my_signals[] =
{
   { "Sample Number", &sample.sample_number, CSV_FORMAT_DECIMAL},
   { "FT60X_CLK",     &sample.clk    },
   { "FT60X_DATA",    &sample.data },
   { "RXF_N",         &sample.rxf_n  },
   { "WR_N",          &sample.wr_n   },
   { NULL, NULL}
};

static parser_t my_parser =
{
   .name              = "hpal",
   .process_frame     = process_frame,
   .signals           = my_signals,
   .sample_time_nsecs = SAMPLE_TIME_NSECS
};

static void CONSTRUCTOR init (void)
{
    parser_register(&my_parser);
}
