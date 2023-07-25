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

#define LINE_BUFFER_SIZE (128)

typedef struct uart_s {
    char
       *name;
    uint32_t state;
    uint32_t bits_accumulated;
    uint32_t byte;
    uint32_t rate;

    time_nsecs_t
	bit_time,
	sample_time,
	byte_start_time;

    unsigned int
	number;

    uint32_t
	line_buffer_mode;

    char
	line_buffer[LINE_BUFFER_SIZE];
    uint32_t
	line_buffer_count;
    time_nsecs_t
	line_buffer_start_time_nsecs;


    time_nsecs_t
	 timer_start_time_nsecs;

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

static void byte_accumulate_byte (parser_t *parser, uart_t *uart, time_nsecs_t time_nsecs);
static void line_buffer_add (parser_t *parser, uart_t *uart, time_nsecs_t time_nsecs);

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
static void uart_init(uint32_t which, char *name, uint32_t use_baud, uint32_t line_buffer_mode)
{
    uart_t *uart;

    uart = &uarts[which];

    uart->state = UART_STATE_INIT;
    uart->name = strdup(name);
    uart->line_buffer_mode = line_buffer_mode;

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
	    uart->byte_start_time = frame->time_nsecs;

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

	if (frame->time_nsecs > (uart->byte_start_time + uart->sample_time)) {

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

		if (uart->line_buffer_mode) {
		    line_buffer_add(&my_parser, uart, uart->byte_start_time);
		}
		else {
		    byte_accumulate_byte(&my_parser, uart, frame->time_nsecs);
		}


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
	if (frame->time_nsecs > (uart->byte_start_time + uart->sample_time)) {
	    uart->state = UART_STATE_IDLE;
	}
	break;

    default:
	break;
    }

}

/*-------------------------------------------------------------------------
 *
 * name:        byte_accumulated
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static void byte_accumulate_byte (parser_t *parser, uart_t *uart, time_nsecs_t time_nsecs)
{
    DECLARE_LOG_FP;

    hdr(parser, time_nsecs, "UART");

    fprintf(log_fp, "%*s ", uart->number * 10, "");
    fprintf(log_fp, "%8s %x ", uart->name, uart->byte);

    if (uart->byte >= ' ' && uart->byte <= 0x7f) {
	fprintf(log_fp,"%c", uart->byte);
    }
    else {
	if (isspace(uart->byte)) {
	    /*
	     * emit white space except for CRs.
	     */
	    if (uart->byte != '\r') {
		fprintf(log_fp,"%c", uart->byte);
	    }
	}
	else {
	    fprintf(log_fp,"[%02x]", uart->byte);
	}
    }

    fprintf(log_fp, "\n");
}

/*-------------------------------------------------------------------------
 *
 * name:        byte_accumulated
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static void line_buffer_add (parser_t *parser, uart_t *uart, time_nsecs_t time_nsecs)
{
    char
	*cp;
    DECLARE_LOG_FP;

    /*
     * Check that we have room in the buffer
     */
    if (uart->line_buffer_count >= LINE_BUFFER_SIZE) {
	fprintf(log_fp, "Uart line buffer overflow: size: %d\n", LINE_BUFFER_SIZE);
	exit(1);
    }

    /*
     * Put the byte into the uart's buffer, if not a '\r'
     */
    if (uart->byte != '\r') {

	/*
	 * Track start time of first byte in line.
	 */
	if (uart->line_buffer_count == 0) {
	    uart->line_buffer_start_time_nsecs = time_nsecs;
	}
	uart->line_buffer[uart->line_buffer_count] = uart->byte;
	uart->line_buffer_count++;
    }

    /*
     * If the byte is a newline, then dump the buffer
     */
    if (uart->byte == '\n' || uart->line_buffer_count == LINE_BUFFER_SIZE) {

	hdr(parser, uart->line_buffer_start_time_nsecs, "UART");

	uart->line_buffer[uart->line_buffer_count] = 0;
	fprintf(log_fp, "%s", uart->line_buffer);

	if (!strcmp(uart->line_buffer, "}\n")) {
	    fprintf(log_fp, "TIMER_STOP @ ");
	    print_time_nsecs(log_fp, time_nsecs);

	    fprintf(log_fp, "delta: ");
	    print_time_nsecs(log_fp, time_nsecs - uart->timer_start_time_nsecs);

	    fprintf(log_fp, "\n");
	}

	if (!strcmp(uart->line_buffer, "{\n")) {

	    uart->timer_start_time_nsecs = time_nsecs;

	    fprintf(log_fp, "TIMER_START @ ");
	    print_time_nsecs(log_fp, time_nsecs);
	    fprintf(log_fp, "\n");

	}

	cp  = strstr(uart->line_buffer, "NOW:");

	if (cp) {

	    uint32_t
		now_msecs;
	    time_nsecs_t
		now_nsecs;

	    /*
	     * Get past 'NOW: ' string
	     */
	    cp += 5;

	    now_msecs = atoi(cp);

	    now_nsecs = now_msecs;
	    now_nsecs *= 1000;
	    now_nsecs *= 1000;

	    fprintf(log_fp, "NOW_DELTA @ ");
	    print_time_nsecs(log_fp, time_nsecs - now_nsecs);
	    fprintf(log_fp, "\n");

	}

	uart->line_buffer_count = 0;
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
    .log_file_name     = "uart.log",
    .sample_time_nsecs = SAMPLE_TIME_NSECS
};

static void CONSTRUCTOR init (void)
{
    parser_register(&my_parser);

    uart_init(0, "uart_txd", USE_460K_BAUD, 1);
}
