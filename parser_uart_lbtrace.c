#include <byteswap.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hdr.h"
#include "lbtrace.h"
#include "log_file.h"
#include "parseSaleae.h"
#include "parser.h"
#include <zephyr/drivers/trace.h>

#define BIT_TIME_921 ( 1000000000 / 921600 )

#define SAMPLE_TIME_NSECS (20)

/*
 * maximum number of uarts to handle.
 */
#define NUMBER_UARTS (1)

static parser_t my_parser;

typedef struct {
    uint32_t uart_lbtrace;
} sample_t;

static sample_t last_sample;
static sample_t sample;

#define BUFFER_SIZE_BYTES (128)

typedef struct uart_s {
    char
       *name;
    uint32_t state;
    uint32_t bits_accumulated;
    uint32_t byte;

    time_nsecs_t
	bit_time,
	sample_time,
	byte_start_time;

    uint8_t
	buffer[BUFFER_SIZE_BYTES];
    uint32_t
	buffer_head,
	buffer_tail;

    time_nsecs_t
	line_buffer_start_time_nsecs;


    time_nsecs_t
	 packet_start_time_nsecs;

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

static void buffer_add      (parser_t *parser, uart_t *uart, time_nsecs_t time_nsecs);
static void check_for_packet(parser_t *parser);

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
static void uart_init(char *name)
{
    uart_t *uart;

    uart = &uarts[0];

    uart->state = UART_STATE_INIT;
    uart->name = strdup(name);

    uart->bit_time = BIT_TIME_921;
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
static void uart_accumulate (parser_t *parser, frame_t *frame, uart_t *uart)
{
    unsigned int wire = sample.uart_lbtrace;

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

	    /*
	     * Adjust forward to the middle of the next bit time.
	     */
	    uart->sample_time += uart->bit_time;

	    /*
	     * Check if the byte if don.e
	     */
	    if (++uart->bits_accumulated == 8) {

		buffer_add(parser, uart, uart->byte_start_time);

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
static void buffer_add (parser_t *parser, uart_t *uart, time_nsecs_t time_nsecs)
{
    uint32_t
	count;
    DECLARE_LOG_FP;

    /*
     * Check that we have room in the buffer
     */
    count = uart->buffer_head - uart->buffer_tail;

    if (count >= BUFFER_SIZE_BYTES) {
	fprintf(log_fp, "Uart line buffer overflow: size: %d\n", BUFFER_SIZE_BYTES);
	exit(1);
    }

    if (count == 0) {
	uart->packet_start_time_nsecs = time_nsecs;
    }

    uart->buffer[uart->buffer_head % BUFFER_SIZE_BYTES ] = uart->byte;
    uart->buffer_head++;

    /*
     * See if we have a viable packet.
     */
    check_for_packet(parser);
}

static uint32_t get_32 (parser_t *parser)
{
    uint32_t
	byt,
	i,
	tail,
	word = 0;
    uart_t
	*uart;

    uart = parser->data;

    tail = uart->buffer_tail;

    for (i=0; i<4; i++) {
	byt = uart->buffer[tail % BUFFER_SIZE_BYTES];
	tail++;

	word = (word << 8) | byt;
    }

    uart->buffer_tail += 4;

    return word;
}

static uint32_t get_8 (uart_t *uart)
{
    uint32_t
	byt;

    byt = uart->buffer[uart->buffer_tail % BUFFER_SIZE_BYTES];

    uart->buffer_tail++;

    return byt;
}

static uint32_t peek_32 (uart_t *uart)
{
    uint32_t
	byt,
	i,
	tail,
	word = 0;

    tail = uart->buffer_tail;

    for (i=0; i<4; i++) {
	byt = uart->buffer[tail % BUFFER_SIZE_BYTES];
	tail++;

	word = (word << 8) | byt;
    }

    return word;
}



/*-------------------------------------------------------------------------
 *
 * name:        check_for_packet
 *
 * description: Since we have no wire that defines exactly where a packet
 *              starts, just sniff the data to see when a properly
 *              formed packet is available.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static void check_for_packet(parser_t *parser)
{
    uint32_t
	bytes,
	count,
	data_words,
	i,
	magic;
    uart_t
	*uart = parser->data;
    DECLARE_LOG_FP;

    count = uart->buffer_head - uart->buffer_tail;

    /*
     * if we have at least 8 bytes, then we may have something.
     */
    if (count < 8) {
	return;
    }

    /*
     * The first word needs to be the magic byte
     */
    magic = peek_32(uart);

    /*
     * Only compare the bottom 3 bytes of the magic word.  The top byte
     * is the data word count that follows.
     */
    if ((magic & 0xffffff) != TRACE_MAGIC) {
	fprintf(log_fp, "NOT magic: expect: %x \n", TRACE_MAGIC);

	/*
	 * remove first byte from buffer and throw it away.
	 */
	get_8(uart);

	/*
	 * return for now.  Will check again next byte that is accumulated.
	 */
	return;
    }

    /*
     * We have a magic word.  What is the packet size?
     */
    data_words = (magic >> 24) & 0xff;

    /*
     * We have the data words plus two words for the magic and tag
     */
    bytes = (data_words + 2) * 4;

    count = uart->buffer_head - uart->buffer_tail;

    if (count < bytes) {
	return;
    }

    /*
     * Here, we seem to have a good packet.  Process it.
     */
    lbtrace_packet_add_word(parser, uart->packet_start_time_nsecs, get_32(parser) );

    lbtrace_packet_add_word(parser, 0, get_32(parser) );  // tag & lineno word

    for (i=0; i<data_words; i++) {
	lbtrace_packet_add_word(parser, 0, get_32(parser)); // optional data words
    }

    lbtrace_packet_parse(parser);
}

/*-------------------------------------------------------------------------
 *
 * name:        process_frame
 *
 * description: process a single frame.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static void process_frame (parser_t *parser, frame_t *frame)
{
    uart_accumulate(parser, frame, &uarts[0]);

    memcpy(&last_sample, &sample, (sizeof sample));
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
   { "uart_lbtrace", &sample.uart_lbtrace },
   { NULL, NULL}
};

static parser_t my_parser =
{
    .name              = "uart_lbtrace",
    .process_frame     = process_frame,
    .signals           = my_signals,
    .log_file_name     = "uart_lbtrace.log",
    .sample_time_nsecs = SAMPLE_TIME_NSECS,
    .connect           = connect,

    .data              = &uarts[0],
};

static void CONSTRUCTOR init (void)
{
    parser_register(&my_parser);

    uart_init("uart_lbtrace");
}
