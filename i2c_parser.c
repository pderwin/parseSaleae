#include <string.h>
#include "frame.h"
#include "hdr.h"
#include "panel_i2c.h"
#include "parseSaleae.h"
#include "parser.h"
#include "i2c_parser.h"
#include "i2c_parser_internal.h"

// #define VERBOSE 1

/*
 * packet sample frequency to support filtering of i2c_clk signal.
 */
#define SAMPLE_TIME_NSECS (50)

/*
 * Set the time for de-glitch
 */
#define DEGLITCH_TIME (5 * SAMPLE_TIME_NSECS)

typedef enum
    {
     EVENT_CLK_FALL,
     EVENT_CLK_RISE,
     EVENT_DTA_FALL,
     EVENT_DTA_RISE
    } event_e;

#if (VERBOSE > 0)
static char *event_strs[] =
    {
     "CLK FALL",
     "CLK RISE",
     "DTA FALL",
     "DTA RISE"
    };
#endif

static char *state_strs[] =
    {
     "IDLE",
     "COLLECT_ADDRESS",
     "RW_BIT",
     "ADDRESS_ACK",
     "COLLECT_DATA",
     "DATA_ACK",
     "WAIT_FOR_STOP_OR_REP_START"
    };

typedef enum
    {
     DIR_WR,
     DIR_RD
    } dir_e;

static void packet_flush (parser_t *parser);

static void empty_accumulator (i2c_data_t *data)
{
    data->accumulated_bits = 0;
    data->accumulated_byte = 0;
}

static void i2c_parser_event(parser_t *parser, event_e event, time_nsecs_t time_nsecs)
{
    uint32_t ack;
    uint32_t
	scl,
	sda;
    static uint32_t event_count = 0;
    static dir_e rw = DIR_RD;
    i2c_data_t
	*data;
    DECLARE_LOG_FP;

    data = parser->data;

    //    fprintf(log_fp, "event: %d\n", event);

    (void) state_strs;
    (void) time_nsecs;

    event_count++;

    scl = data->scl->val;
    sda = data->sda->val;

    //    fprintf(log_fp, "SCL: %d SDA: %d  \n", scl, sda);

#ifdef VERBOSE
    {
	fprintf(log_fp, "\n");
	hdr(parser, time_nsecs, "PARSER_I2C_EVENT");
	fprintf(log_fp, "%s state: byt: %02x ab: %d %s (ec: %d) scl: %d sda: %d \n",
		state_strs[data->state],
		data->accumulated_byte,
		data->accumulated_bits,
		event_strs[event],
		event_count,
		scl,
		sda);
    }
#endif

    //    fprintf(log_fp, "scl: %d sda: %d \n", scl, sda);


    /*
     * Check for a start condition.  This is when clock is high, and data goes
     * low.
     */
    if (scl == 1) {
	if (event == EVENT_DTA_FALL) {

	    //	    fprintf(log_fp, "START\n");

	    /*
	     * This may be a repeated start, so parse any data that we may have accumulated.
	     */
	    packet_flush(parser);

	    empty_accumulator(data);
	    data->state = STATE_COLLECT_ADDRESS;

	    data->packet_start_time_nsecs = time_nsecs;
	    return;
	}

	if (event == EVENT_DTA_RISE) {

	    //	    fprintf(log_fp, "STOP");

	    packet_flush(parser);

	    data->packet_count = 0;
	    data->state = STATE_IDLE;
	    return;
	}
    }

    switch (data->state) {

    case STATE_IDLE:
	break;

    case STATE_COLLECT_ADDRESS:
	if (event == EVENT_CLK_RISE) {

	    //	    fprintf(log_fp, "collect address: %x (sda: %d acc bits: %d )\n", data->accumulated_byte, sda, data->accumulated_bits );

	    data->accumulated_byte |= (sda << (7 - data->accumulated_bits));
	    data->accumulated_bits++;

	    if ( data->accumulated_bits == 7 ) {
		data->state = STATE_RW_BIT;
	    }
	}
	break;

    case STATE_RW_BIT:
	if (event == EVENT_CLK_RISE) {
	    rw = (dir_e) sda;

	    //	    fprintf(log_fp, "[ %s ] ", rw == DIR_RD ? "Rd":"Wr");

	    data->accumulated_byte |= rw;

	    data->state = STATE_ADDRESS_ACK;
	}
	break;

    case STATE_ADDRESS_ACK:

	if (event == EVENT_CLK_RISE) {
	    ack = sda;

	    if (ack == 1) {
		fprintf(log_fp, "ADDR NAK!\n");
		data->state = STATE_IDLE;
	    }
	    else {
#ifdef VERBOSE
		fprintf(log_fp, "ACK\n");
#endif
	    }

	    /*
	     * save away address byte
	     */
	    data->packet[0] = data->accumulated_byte;
	    data->packet_count = 1;

	    data->accumulated_bits = 0;
	    data->accumulated_byte = 0;

	    data->state = STATE_COLLECT_DATA;
	}
	break;

   case STATE_COLLECT_DATA:
       if (event == EVENT_CLK_RISE) {

	   data->accumulated_byte |= (sda << (7 - data->accumulated_bits));
#ifdef VERBOSE
	   fprintf(log_fp, "bits: %d acc_byte: %x dta: %x\n", data->accumulated_bits, data->accumulated_byte, sda);
#endif
	   data->accumulated_bits++;

	   if ( data->accumulated_bits == 8 ) {

	       if (data->packet_count == NUMBER_PACKET_BYTES) {
		   fprintf(stderr, "%s: packet buffer overflow\n", __func__);
		   fprintf(log_fp, "%s: packet buffer overflow\n", __func__);
		   exit(1);
	       }

	       data->packet[data->packet_count] = data->accumulated_byte;
	       data->packet_count++;

	       data->state = STATE_DATA_ACK;
	   }
       }
       break;

   case STATE_DATA_ACK:
       if (event == EVENT_CLK_RISE) {

	   if (sda) {
	       //	       printf("DTA NAK! (ec: %d)\n", event_count);
	   }
	   else {
#ifdef VERBOSE
	       fprintf(log_fp, "ACK\n");
#endif

	       /*
		* If doing a read, and we see the ACK, then the master has
		* driven it low, saying that we should drive another data byte
		* out.
		*/
	       if (rw == DIR_RD) {
		   empty_accumulator(data);
		   data->state = STATE_COLLECT_DATA;
	       }
	       /*
		* when doing a write, and the ACK is seen, how to we know that
		* we are getting the next data instead of a repeated start?
		* for now, we are just getting one byte written and then a
		* restart.
		*/
	       else {
		   empty_accumulator(data);
		   data->state = STATE_COLLECT_DATA;
	       }
	   }
       }
       break;

    case STATE_WAIT_FOR_STOP_OR_REP_START:
       if (event == EVENT_CLK_RISE) {
	   if (sda) {
	       fprintf(log_fp, "rep start\n");
	       data->accumulated_bits = 0;
	       data->accumulated_byte = 0;
	       data->state = STATE_COLLECT_ADDRESS;
	   }
	   else {
	       fprintf(log_fp, "stop\n");
	       data->state = STATE_IDLE;
	   }
       }
       break;

    }

}

static void packet_flush (parser_t *parser)
{
    i2c_data_t *data;

    data = parser->data;

    if (data->packet_count) {
	(data->parse_packet)(data->orig_parser, data->packet_start_time_nsecs, data->packet, data->packet_count);

	data->packet_count = 0;
    }
}

static void process_frame (parser_t *parser, frame_t *frame)
{
    i2c_data_t *data;

    data = parser->data;

#if 0
	if (falling_edge(panel_irq)) {
	    hdr(my_parser.lf, frame->time_nsecs, "***PANEL IRQ ASSERT***");
	    panel_i2c_irq(frame, 0);
	}
	if (rising_edge(panel_irq)) {
	    hdr(my_parser.lf, frame->time_nsecs, "***PANEL IRQ DE-ASSERT***");
	    panel_i2c_irq(frame, 1);
	}
#endif

    /*
     * Clear the accumulator at the start of a packet
     */
    if (falling_edge(data->scl)) {
	i2c_parser_event(parser, EVENT_CLK_FALL, frame->time_nsecs);
    }

    if (rising_edge(data->scl)) {
	i2c_parser_event(parser, EVENT_CLK_RISE, frame->time_nsecs);
    }

    if (falling_edge(data->sda)) {
	i2c_parser_event(parser, EVENT_DTA_FALL, frame->time_nsecs);
    }

    if (rising_edge(data->sda)) {
	i2c_parser_event(parser, EVENT_DTA_RISE, frame->time_nsecs);
    }
}

void i2c_parser_create(parser_t *orig_parser,
		       signal_t *scl,
		       signal_t *sda,
		       void (*parse_packet)(parser_t *parser, time_nsecs_t time_nsecs, uint8_t *packet, uint32_t packet_count) )
{
    i2c_data_t
	*data;
    parser_t
	*parser;

    (void) orig_parser;

    parser = calloc(1, sizeof(parser_t));

    parser->name = "i2c_parser";
    parser->process_frame = process_frame;
    parser->enable = 1;
    parser->log_file = orig_parser->log_file;

    /*
     * Allocate a data area for our parser.
     */
    parser->data = data = calloc(1, sizeof(i2c_data_t));

    data->state = STATE_IDLE;

    data->scl = scl;
    data->sda = sda;

    data->orig_parser  = orig_parser;
    data->parse_packet = parse_packet;

    /*
     * Need to register our parser.
     */
    parser_register(parser);
}
