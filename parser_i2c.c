#include <string.h>
#include "frame.h"
#include "hdr.h"
#include "panel_i2c.h"
#include "parseSaleae.h"
#include "parser.h"

// #define VERBOSE 1

/*
 * packet sample frequency to support filtering of i2c_clk signal.
 */
#define SAMPLE_TIME_NSECS (50)

/*
 * Set the time for de-glitch
 */
#define DEGLITCH_TIME (5 * SAMPLE_TIME_NSECS)

typedef struct {
    uint32_t i2c_clk;
    uint32_t i2c_sda;
    uint32_t panel_irq;
} sample_t;

static sample_t last_sample;
static sample_t sample;
static uint32_t accumulated_bits;
static uint32_t accumulated_byte;
static parser_t my_parser;


#if 0
/*-------------------------------------------------------------------------
 *
 * name:        connect
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static uint32_t connect (void)
{
    int
	panel_irq_rc,
	rc = 0;

    if (rc) {
	printf("parser_i2c: signal(s) not found.  Disabled\n");
	return 0;
    }

    /*
     * attach to the panel_irq wire if possible.
     */
    panel_irq_rc = csv_find("panel_irq",    &sample.panel_irq);

    fprintf(lf->fp, "panel_irq: %sFound \n", panel_irq_rc ? "Not ":"");

    panel_i2c_connect();

    return 1;
}
#endif

typedef enum
    {
     EVENT_CLK_FALL,
     EVENT_CLK_RISE,
     EVENT_DTA_FALL,
     EVENT_DTA_RISE
    } event_e;

static char *event_strs[] =
    {
     "CLK FALL",
     "CLK RISE",
     "DTA FALL",
     "DTA RISE"
    };

typedef enum
    {
     STATE_IDLE,
     STATE_COLLECT_ADDRESS,
     STATE_RW_BIT,
     STATE_ADDRESS_ACK,
     STATE_COLLECT_DATA,
     STATE_DATA_ACK,
     STATE_WAIT_FOR_STOP_OR_REP_START,
    } state_e;

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

static state_e state = STATE_IDLE;

static void empty_accumulator (void)
{
    accumulated_bits = 0;
    accumulated_byte = 0;
}

static void event(event_e event, time_nsecs_t time_nsecs)
{
    uint32_t ack;
    static uint32_t data_count = 0;
    uint32_t dta;
    static uint32_t event_count = 0;
    static uint32_t i2c_address = 0;
    static dir_e rw = DIR_RD;

    (void) event_strs;
    (void) state_strs;

    event_count++;
    dta = sample.i2c_sda;

#ifdef VERBOSE
    {
	static time_nsecs_t last_clk_rise = 0;
	static time_nsecs_t last_dta_rise = 0;

	hdr(log_fp, time_nsecs, "PARSER_I2C_EVENT");
	fprintf(log_fp, "%s state: ab: %d %s (ec: %d) ", state_strs[state], accumulated_bits,  event_strs[event], event_count);

	if (event == EVENT_CLK_RISE) {

	    fprintf(log_fp, "(clk rise delta: %lld  ( time_nsecs: %lld ) (true edge: %lld )) ", time_nsecs - last_clk_rise, time_nsecs, time_nsecs - DEGLITCH_TIME);
	    last_clk_rise = time_nsecs;
	}

	if (event == EVENT_DTA_RISE) {
	    last_dta_rise = time_nsecs;
	}

	if (event == EVENT_DTA_FALL) {
	    fprintf(log_fp, "(dta_high_time: %lld) ", time_nsecs - last_dta_rise);
	}
    }

#endif

    /*
     * Check for a start condition.  This is when clock is high, and data goes
     * low.
     */
    if (sample.i2c_clk == 1) {
	if (event == EVENT_DTA_FALL) {
	    //	    printf("START\n");

	    empty_accumulator();
	    state = STATE_COLLECT_ADDRESS;
	    return;
	}

	if (event == EVENT_DTA_RISE) {
	    //	    printf("STOP");
	    state = STATE_IDLE;
	    return;
	}
    }

    switch (state) {

    case STATE_IDLE:
	break;

    case STATE_COLLECT_ADDRESS:
	if (event == EVENT_CLK_RISE) {

	    accumulated_byte |= (sample.i2c_sda << (6 - accumulated_bits));
	    accumulated_bits++;

	    if ( accumulated_bits == 7 ) {

		i2c_address = accumulated_byte;

		hdr(my_parser.lf, time_nsecs, "PARSER_I2C");
		fprintf(my_parser.lf->fp, "I2C Address: %x ", accumulated_byte);

		state = STATE_RW_BIT;
	    }
	}
	break;

    case STATE_RW_BIT:
	if (event == EVENT_CLK_RISE) {
	    rw = (dir_e) sample.i2c_sda;

	    fprintf(my_parser.lf->fp, "[ %s ] ", rw == DIR_RD ? "Rd":"Wr");

	    state = STATE_ADDRESS_ACK;

	}
	break;

    case STATE_ADDRESS_ACK:
	if (event == EVENT_CLK_RISE) {
	    ack = sample.i2c_sda;

	    if (ack == 1) {
		fprintf(my_parser.lf->fp, "ADDR NAK!\n");
		state = STATE_IDLE;
	    }
	    else {
#ifdef VERBOSE
		fprintf(log_fp, "ACK\n");
#endif
	    }

	    /*
	     * Tell panel parsing code what device we are talking to.
	     */
	    bootloader_i2c((i2c_address << 1) | rw, 1, time_nsecs, event_count);
	    panel_i2c((i2c_address << 1) | rw, time_nsecs, 1);

	    accumulated_bits = 0;
	    accumulated_byte = 0;

	    data_count = 0;

	    state = STATE_COLLECT_DATA;

	}
	break;

   case STATE_COLLECT_DATA:
       if (event == EVENT_CLK_RISE) {

	   accumulated_byte |= (sample.i2c_sda << (7 - accumulated_bits));
#ifdef VERBOSE
	   fprintf(log_fp, "bits: %d acc_byte: %x dta: %x\n", accumulated_bits, accumulated_byte, sample.i2c_dta);
#endif
	   accumulated_bits++;

	   if ( accumulated_bits == 8 ) {

	       fprintf(my_parser.lf->fp, "%02x ", accumulated_byte);

	       /*
		* Keep from having very long lines of data.
		*/
	       if (++data_count == 16) {
		   fprintf(my_parser.lf->fp, "\n%91s","");
		   data_count = 0;
	       }

	       bootloader_i2c(accumulated_byte, 0, time_nsecs, event_count);
	       panel_i2c(accumulated_byte, time_nsecs, 0);

	       state = STATE_DATA_ACK;
	   }
       }
       break;

   case STATE_DATA_ACK:
       if (event == EVENT_CLK_RISE) {

	   if (dta) {
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
		   empty_accumulator();
		   state = STATE_COLLECT_DATA;
	       }
	       /*
		* when doing a write, and the ACK is seen, how to we know that
		* we are getting the next data instead of a repeated start?
		* for now, we are just getting one byte written and then a
		* restart.
		*/
	       else {
		   empty_accumulator();
		   state = STATE_COLLECT_DATA;
	       }
	   }
       }
       break;

    case STATE_WAIT_FOR_STOP_OR_REP_START:
       if (event == EVENT_CLK_RISE) {
	   if (dta) {
	       fprintf(my_parser.lf->fp, "rep start\n");
	       accumulated_bits = 0;
	       accumulated_byte = 0;
	       state = STATE_COLLECT_ADDRESS;
	   }
	   else {
	       fprintf(my_parser.lf->fp, "stop\n");
	       state = STATE_IDLE;
	   }
       }
       break;

    }

}

static void process_frame (parser_t *parser, frame_t *frame)
{
    (void) parser;

    {
	static uint32_t current_i2c_clk = 0;
	static uint32_t countdown       = 0;
	static uint32_t first_frame     = 1;

	/*
	 * To keep from thinking we have a rising edge of clock at the beginning of debounce, we need to set our
	 * 'current' to our first entry.
	 */
	if (first_frame) {
	    current_i2c_clk = last_sample.i2c_clk = sample.i2c_clk;
	    first_frame = 0;
	    return;
	}


	if (falling_edge(panel_irq)) {
	    hdr(my_parser.lf, frame->time_nsecs, "***PANEL IRQ ASSERT***");
	    panel_i2c_irq(frame, 0);
	}
	if (rising_edge(panel_irq)) {
	    hdr(my_parser.lf, frame->time_nsecs, "***PANEL IRQ DE-ASSERT***");
	    panel_i2c_irq(frame, 1);
	}

	/*
	 * The crisp clock is essential to our parsing ability.  Filter it for 250
	 * nSecs.
	 *
	 * When it changes states, set counter and just return.
	 */
	if (sample.i2c_clk != current_i2c_clk) {

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
	    current_i2c_clk = sample.i2c_clk;

	} // sample != current
    }

    /*
     * Clear the accumulator at the start of a packet
     */
    if (falling_edge(i2c_clk)) {
	event(EVENT_CLK_FALL, frame->time_nsecs);
    }

    if (rising_edge(i2c_clk)) {
	event(EVENT_CLK_RISE, frame->time_nsecs);
    }

    if (falling_edge(i2c_sda)) {
	event(EVENT_DTA_FALL, frame->time_nsecs);
    }

    if (rising_edge(i2c_sda)) {
	event(EVENT_DTA_RISE, frame->time_nsecs);
    }

    memcpy(&last_sample, &sample, sizeof(sample_t));
}

 static signal_t my_signals[] =
{
   { "i2c_clk", &sample.i2c_clk },
   { "i2c_sda", &sample.i2c_sda },
   { NULL, NULL}
};


static parser_t my_parser =
    {
     .name          = "i2c",
     .signals       = my_signals,
     .process_frame = process_frame,

     .log_file       = "parser_i2c.log",
    /*
     * To be able to filter glitches on the clock, we want to get events at
     * least every 50 nSecs
     */
     .sample_time_nsecs = SAMPLE_TIME_NSECS

    };

static void CONSTRUCTOR init (void)
{
    parser_register(&my_parser);
}
