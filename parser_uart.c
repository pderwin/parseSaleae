#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hdr.h"
#include "log_file.h"
#include "parseSaleae.h"

#define BIT_TIME_115 ( 1000000000 / 115200 )
#define BIT_TIME_460 ( 1000000000 / 460800 )

#define SAMPLE_TIME_NSECS (10)

/*
 * maximum number of uarts to handle.
 */
#define NUMBER_UARTS (4)


typedef struct {
    uint32_t eng_rxd;
    uint32_t eng_txd;
} sample_t;

static sample_t last_sample;
static sample_t sample;


#if 0

typedef struct uart_s {
   char
      *name;
   unsigned int
      state;
   unsigned int
      bits_accumulated,
      byte;

   unsigned long long int
      bit_time,
      sample_time,
      start_time;

   unsigned int
      number;

   FILE
      *fp;

} uart_t;

/*
 * Current number of uarts in use.
 */
static unsigned int uart_count;

static uart_t uarts[NUMBER_UARTS];

enum {
   UART_STATE_INIT,  // need to see at least one high bit to get to idle state.
   UART_STATE_IDLE,
   UART_STATE_ACCUMULATE_BITS,
   UART_STATE_STOP_BIT,
};

/*-------------------------------------------------------------------------
 *
 * name:        uart_find
 *
 * description: try to find a uart name.
 *
 * input:
 *
 * output:      0 - uart not found
 *              1 - success
 *
 *-------------------------------------------------------------------------*/
static int32_t
   uart_find (char *name, uint32_t use_baud)
{
   char
      buf[32];
   uint32_t
      rate;
   uart_t
      *uart;

   /*
    * Attempt to find the uart in the CSV file.  Returns non-zero on error.
    */
   if (csv_find(name, &sample.wire[uart_count])) {
       return 0;
   }


   uart = &uarts[uart_count];

   uart->number   = uart_count;
   uart->name     = strdup(name);

   /*
    * Assume we are at 460K baud
    */
   switch(use_baud) {
      case USE_115K_BAUD:
	 uart->bit_time = BIT_TIME_115;
	 rate = 115;
	 break;

      case USE_460K_BAUD:
	 uart->bit_time = BIT_TIME_460;
	 rate = 460;
	 break;

   }

   /*
    * Open a file for writing the uart output to.
    */
   sprintf(buf, "%s_uart.lst", name);
   uart->fp = fopen(buf, "w");

   fprintf(uart->fp, "Parsing at %dK baud\n", rate);

   uart_count++;

   /*
    * success
    */
   return 1;
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
    unsigned int wire = sample.wire[uart->number];

    if (uart->state != UART_STATE_INIT) {
	//	printf("%lld:  state: %d wire: %d ba: %d \n", frame->time_nsecs, uart->state, wire, uart->bits_accumulated);
    }

    switch (uart->state) {

    case UART_STATE_INIT:
	if (wire == 1) {
	    uart->state = UART_STATE_IDLE;
	    //	    printf("%lld: Go from INIT to IDLE state \n", frame->time_nsecs);
	}
	break;

	/*
	 * When we see a falling edge, track time, and change to accumulating state.
	 */
    case UART_STATE_IDLE:
	if (wire == 0) {

	    //	    printf("%lld start accumulation.  BT: %llx (%x %x)\n", frame->time_nsecs, uart->bit_time, BIT_TIME_115, BIT_TIME_460);

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

		hdr(stdout_lf, frame->time_nsecs, "UART");

		printf("%*s ", uart->number * 10, "");
		printf("%8s %x ", uart->name, uart->byte);

		if (uart->byte >= ' ' && uart->byte <= 0x7f) {
		    printf("'%c'", uart->byte);
		    fprintf(uart->fp,"%c", uart->byte);
		}
		else {
		    if (isspace(uart->byte)) {
			/*
			 * emit white space except for CRs.
			 */
			if (uart->byte != '\r') {
			    fprintf(uart->fp,"%c", uart->byte);
			}
		    }
		    else {
			fprintf(uart->fp,"[%02x]", uart->byte);
		    }
		}


		//		printf("\n");

		/*
		 * We have to wait for the stop bit now.  Only wait until the middle of the bit so that we will properly detect the next start bit.
		 */
		uart->sample_time += (uart->bit_time / 2);
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



#endif



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
   process_frame (frame_t *frame)
{
#if 0
   unsigned int i;
   //   uart_t *uart;

   for (i=0, uart = uarts; i<uart_count; i++) {
       //      uart_accumulate(frame, uart);
      uart++;
   }
#endif

   memcpy(&last_sample, &sample, (sizeof sample));
}

static signal_t my_signals[] =
{
   { "eng_rxd", &sample.eng_rxd },
   { "eng_txd", &sample.eng_txd },
   { NULL, NULL}
};

static parser_t my_parser =
{
    .name          = "uart",
    .process_frame = process_frame,
    .signals       = my_signals
};

static void CONSTRUCTOR init (void)
{
    parser_register(&my_parser);
}
