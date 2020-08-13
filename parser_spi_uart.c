#include <string.h>
#include "hdr.h"
#include "parseSaleae.h"
#include "trace.h"

typedef struct {
    uint32_t dbg_clk;
    uint32_t dbg_dat;
    uint32_t dbg_pkt;
    uint32_t dbg_trg;
} sample_t;

static sample_t last_sample;
static sample_t sample;
static uint32_t accumulated_bits;
static uint32_t accumulated_byte;
static parser_t my_parser;


/*
 * packet sample frequency to support filtering of i2c_clk signal.
 */
#define SAMPLE_TIME_NSECS (25)

/*
 * Set the time for de-glitch
 */
#define DEGLITCH_TIME (5 * SAMPLE_TIME_NSECS)

static uint8_t      packet_buffer[128];
static uint32_t     next_index;
static uint32_t     packet_buffer_count;
static time_nsecs_t packet_buffer_start_time;

/*-------------------------------------------------------------------------
 *
 * name:        got_byte
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static void got_byte(time_nsecs_t time_nsecs, uint8_t byte)
{
    //    printf("Got: %x \n", byte);

    if (packet_buffer_count >= sizeof(packet_buffer)) {
	printf("Packet too long\n");
	exit(1);
    }

    if (packet_buffer_count == 0) {
	packet_buffer_start_time = time_nsecs;
    }

    packet_buffer[packet_buffer_count] = byte;
    packet_buffer_count++;
}

/*-------------------------------------------------------------------------
 *
 * name:        next
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static int32_t next (uint8_t *cp)
{
    if (next_index < packet_buffer_count) {
	*cp = packet_buffer[next_index];
	next_index++;

	return 0;
    }

    return -1;
}

#if 0
/*-------------------------------------------------------------------------
 *
 * name:        next32
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static uint32_t next32 (void)
{
    uint8_t  byte;
    uint32_t i;
    uint32_t sum = 0;

    for (i=0; i<4; i++) {
	if (next(&byte)) {  // if error reading next byte, then fail
	    return -1;
	}

	sum = (sum << 8) | byte;

    }

    return sum;
}
#endif

/*-------------------------------------------------------------------------
 *
 * name:        parse_packet_buffer
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static void parse_packet_buffer (void)
{
    // uint8_t ch;
    //    uint8_t prt_it;
    uint8_t tag;

    next_index = 0;

    next(&tag);

    hdr(stdout_lf, packet_buffer_start_time, "PARSER_SPI_UART");

    switch (tag) {

#if 0
    case TAG_TRIGGER:
	printf("*** TRIGGER ***");
	break;
#endif

#if 0
    case TAG_PRINTK:
	while(next(&ch) == 0) {

	    /*
	     * If newline, and last character of buffer, then do NOT print it.
	     */
	    prt_it = 1;

	    if (ch == '\n') {
		if (next_index == packet_buffer_count) {
		    prt_it = 0;
		}
	    }

	    if (prt_it) {
		printf("%c", ch);
	    }

	    //	    printf("FFF: %d %x \n", prt_it, ch);
	}
	break;
#endif

    default:
	tag_find(tag);
	printf("\n");
    }
}

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
     * Clear the accumulator at the start of a packet
     */

    if (rising_edge(dbg_pkt)) {
	accumulated_bits = 0;
	accumulated_byte = 0;
    }

    /*
     * Parse the accumulated bytes at the end of a packet
     */
    if (falling_edge(dbg_pkt)) {

	if (packet_buffer_count) {

	    parse_packet_buffer();

	    /*
	     * If data left over, then parse error.
	     */
	    if (next_index != packet_buffer_count) {
		printf("ERROR: %d bytes left in buffer\n", packet_buffer_count - next_index);
		//		exit(1);
	    }
	    /*
	     * empty the packet buffer
	     */
	    packet_buffer_count = 0;
	}
    }

    if (sample.dbg_pkt == 0) {
	memcpy(&last_sample, &sample, sizeof(sample_t));
	return;
    }

    /*
     * debounce of the dbg_clk wire.  Seeing a slow falling edge on newer cards.
     */
    {
	static uint32_t current_dbg_clk = 0;
	static uint32_t countdown       = 0;
	static uint32_t first_frame     = 1;

	/*
	 * To keep from thinking we have a rising edge of clock at the beginning of debounce, we need to set our
	 * 'current' to our first entry.
	 */
	if (first_frame) {
	    current_dbg_clk = last_sample.dbg_clk = sample.dbg_clk;
	    first_frame = 0;
	    return;
	}

	/*
	 * The crisp clock is essential to our parsing ability.  Filter it for 250
	 * nSecs.
	 *
	 * When it changes states, set counter and just return.
	 */
	if (sample.dbg_clk != current_dbg_clk) {

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
	    current_dbg_clk = sample.dbg_clk;

	} // sample != current
    }








    /*
     * If dbg_packet is on, and we see a rising edge of clock, then sample the
     * data.
     */
    if (rising_edge(dbg_clk)) {

	if (sample.dbg_pkt) {

	    accumulated_byte |= (sample.dbg_dat << (7 - accumulated_bits));
	    accumulated_bits++;

	    if (accumulated_bits == 8) {

		got_byte(frame->time_nsecs, accumulated_byte);

		accumulated_bits = 0;
		accumulated_byte = 0;
	    }

	}
    }

    memcpy(&last_sample, &sample, sizeof(sample_t));
}

static signal_t my_signals[] =
{
   { "dbg_clk", &sample.dbg_clk },
   { "dbg_dat", &sample.dbg_dat },
   { "dbg_pkt", &sample.dbg_pkt },
   { "dbg_trg", &sample.dbg_trg },
   { NULL, NULL}
};

static parser_t my_parser =
{
   .name              = "spi_uart",
   .process_frame     = process_frame,
   .signals           = my_signals,
   .sample_time_nsecs = SAMPLE_TIME_NSECS
};

static void CONSTRUCTOR init (void)
{
    parser_register(&my_parser);
}
