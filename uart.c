#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parseSaleae.h"


#define PACKET_SIZE (16 + 3)

#define BIT_TIME ( 1000000000 / 115200 )

typedef struct uart_s {
   char
      *name;
   unsigned int
      state;
   unsigned int
      bits_accumulated,
      byte;

   unsigned long long int
      sample_time,
      start_time;

   unsigned int
      col;
   
} uart_t;

static uart_t G2_recv = { .name = "G2_recv", .col = 0 };
static uart_t G2_xmit = { .name = "G2_xmit", .col = 1 };

enum {
   UART_STATE_IDLE,
   UART_STATE_ACCUMULATE_BITS,
   UART_STATE_STOP_BIT,
};

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
   uart_accumulate (frame_t *jf, unsigned int wire, uart_t *uart)
{
//   printf("state: %d uart: %d ba: %d \n", uart_state, jf->uart, bits_accumulated);
   
   switch (uart->state) {

      /*
       * When we see a falling edge, track time, and change to accumulating state.
       */
      case UART_STATE_IDLE:
	 if (wire == 0) {

//	    printf("%lld start accumulation\n", jf->time_nsecs);
	    
	    /*
	     * Track the start of this byte
	     */
	    uart->start_time = jf->time_nsecs;

	    /*
	     * The next time we should sample is 1.5 bit times from now.  That spans the entire start bit, and half
	     * of the data bit time.
	     */
	    uart->sample_time = ((BIT_TIME * 3) / 2);

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
	 
	 if (jf->time_nsecs > (uart->start_time + uart->sample_time)) {
	    
	    uart->byte |= (wire << uart->bits_accumulated);

//	    printf("%lld: UART: got bit: %d byte: %x \n", jf->time_nsecs, jf->uart, uart_byte);
	    
	    /*
	     * Adjust forward to the middle of the next bit time.
	     */
	    uart->sample_time += BIT_TIME;

	    /*
	     * Check if the byte if don.e
	     */
	    if (++uart->bits_accumulated == 8) {

	       hdr(jf);
	       printf("UART: %*s ", uart->col * 20, "");
	       
	       printf("%8s %x ", uart->name, uart->byte);

	       if (uart->byte >= ' ' && uart->byte <= 0x7f) {
		  printf("'%c'", uart->byte);
	       }

	       printf("\n");

	       /*
		* We have to wait for the stop bit now.  Only wait until the middle of the bit so that we will properly detect the next start bit.
		*/
	       uart->sample_time += (BIT_TIME / 2);
	       uart->state = UART_STATE_STOP_BIT;
	    }
	 }
	 break;

	 /*
	  * When we have waited long enough, we go back idle.
	  */
      case UART_STATE_STOP_BIT:
	 if (jf->time_nsecs > (uart->start_time + uart->sample_time)) {
	    uart->state = UART_STATE_IDLE;
	 }
	 break;
	 
      default:
	 break;
   }
   
}


void
   uart_process (frame_t *frame)
{
   uart_accumulate(frame, frame->G2_xmit, &G2_xmit);
   uart_accumulate(frame, frame->G2_recv, &G2_recv);
   
}
