#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hdr.h"
#include "log_file.h"
#include "parseSaleae.h"
#include "parser.h"

#define BIT_TIME_115 ( 1000000000 / 115200 )
#define BIT_TIME_460 ( 1000000000 / 460800 )

#define SAMPLE_TIME_NSECS (20)

/*
 * maximum number of uarts to handle.
 */
#define NUMBER_UARTS (1)

static parser_t my_parser;

typedef struct {
    uint32_t uart_txd;
} sample_t;

static sample_t last_sample;
static sample_t sample;


typedef struct uart_s {
    char *name;
    uint32_t state;
    uint32_t bits_accumulated;
    uint32_t byte;
    uint32_t rate;

    time_nsecs_t
      bit_time,
      sample_time,
      start_time;

   unsigned int
      number;

} uart_t;

/*
 * Current number of uarts in use.
 */
static uart_t uarts[NUMBER_UARTS];

enum {
   UART_STATE_INIT,  // need to see at least one high bit to get to idle state.
   UART_STATE_IDLE,
   UART_STATE_ACCUMULATE_BITS,
   UART_STATE_STOP_BIT,
};

/*-------------------------------------------------------------------------
 *
 * name:        uart_init
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static void uart_init(uint32_t which, char *name, uint32_t use_baud)
{
    uart_t *uart;

    uart = &uarts[which];

    uart->state = UART_STATE_INIT;
    uart->name = strdup(name);

    switch(use_baud) {

    case USE_115K_BAUD:
	uart->bit_time = BIT_TIME_115;
	uart->rate = 115;
	break;

    case USE_460K_BAUD:
	uart->bit_time = BIT_TIME_460;
	uart->rate = 460;
	break;
    }

}

/**
 ****************************************************************************************************
 *
 * \brief
 *
 * \param
 *
 * \returns
 *
 *****************************************************************************************************/
void
uart_accumulate (frame_t *frame, uart_t *uart)
{
    unsigned int wire = sample.uart_txd;

    if (uart->state != UART_STATE_INIT) {
	//	printf("%lld:  state: %d wire: %d ba: %d \n", frame->time_nsecs, uart->state, wire, uart->bits_accumulated);
    }

    (void) wire;

    switch (uart->state) {

    case UART_STATE_INIT:
	if (wire == 1) {
	    uart->state = UART_STATE_IDLE;
	    //	    fprintf(my_parser.lf->fp, "%lld: Go from INIT to IDLE state \n", frame->time_nsecs);
	}
	break;

	/*
	 * When we see a falling edge, track time, and change to accumulating state.
	 */
    case UART_STATE_IDLE:
	if (wire == 0) {

	    /*
	     * Track the start of this byte
	     */
	    uart->start_time = frame->time_nsecs;

	    /*
	     * The next time we should sample is 1.5 bit times from now.  That spans the entire start bit, and half
	     * of the data bit time.
	     */
	    uart->sample_time = ((uart->bit_time * 3) / 2);

	    /*
	     * We have 0 bits accumulated.
	     */
	    uart->bits_accumulated = 0;

	    /*
	     * Clear our our acculated byte
	     */
	    uart->byte = 0;

	    uart->state = UART_STATE_ACCUMULATE_BITS;
	}
	break;

    case UART_STATE_ACCUMULATE_BITS:

	if (frame->time_nsecs > (uart->start_time + uart->sample_time)) {

	    uart->byte |= (wire << uart->bits_accumulated);

	    //	    printf("%lld: UART: got bit: %d byte: %x \n", frame->time_nsecs, wire, uart->byte);

	    /*
	     * Adjust forward to the middle of the next bit time.
	     */
	    uart->sample_time += uart->bit_time;

	    /*
	     * Check if the byte if don.e
	     */
	    if (++uart->bits_accumulated == 8) {

		hdr(my_parser.lf, frame->time_nsecs, "UART");

		fprintf(my_parser.lf->fp, "%*s ", uart->number * 10, "");
		fprintf(my_parser.lf->fp, "%8s %x ", uart->name, uart->byte);

		if (uart->byte >= ' ' && uart->byte <= 0x7f) {
		    fprintf(my_parser.lf->fp,"%c", uart->byte);
		}
		else {
		    if (isspace(uart->byte)) {
			/*
			 * emit white space except for CRs.
			 */
			if (uart->byte != '\r') {
			    fprintf(my_parser.lf->fp,"%c", uart->byte);
			}
		    }
		    else {
			fprintf(my_parser.lf->fp,"[%02x]", uart->byte);
		    }
		}


		fprintf(my_parser.lf->fp, "\n");

		/*
		 * We have to wait for the stop bit now.  We have already incremented the sample_time above
		 * to be in the middle of the stop bit.
		 */
		uart->state = UART_STATE_STOP_BIT;
	    }
	}
	break;

	/*
	 * When we have waited long enough, we go back idle.
	 */
    case UART_STATE_STOP_BIT:
	if (frame->time_nsecs > (uart->start_time + uart->sample_time)) {
	    uart->state = UART_STATE_IDLE;
	}
	break;

    default:
	break;
    }

}


/*-------------------------------------------------------------------------
 *
 * name:        uart_process
 *
 * description: process a single frame.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void
process_frame (parser_t *parser, frame_t *frame)
{
    (void) parser;

    uart_accumulate(frame, &uarts[0]);

    memcpy(&last_sample, &sample, (sizeof sample));
}

static signal_t my_signals[] =
{
   { "uart_txd", &sample.uart_txd },
   { NULL, NULL}
};

static parser_t my_parser =
{
    .name              = "uart",
    .process_frame     = process_frame,
    .signals           = my_signals,
    .log_file          = "uart.log",
    .sample_time_nsecs = SAMPLE_TIME_NSECS
};

static void CONSTRUCTOR init (void)
{
    parser_register(&my_parser);

    uart_init(0, "uart_txd", USE_115K_BAUD);
}
