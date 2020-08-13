#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <drivers/trace.h>
#include "parseSaleae.h"

// #define DEBUG 1

//
// cannot use cycle count with mpu-enabled build.  See drivers/trace/trace.c
//
#define HAVE_CYCLE_COUNT 1

enum {
   STATE_IDLE,
   STATE_CYCLE_COUNT,
   STATE_PACKET_SIZE,
   STATE_GET_TAG,
   STATE_GET_WORDS,
};

/**
 ****************************************************************************************************
 *
 * \brief    We've accumulated a word of data off of the serial lines.  Process this into a
 *           completed packet, and then parse it when the packet is fully received.
 *
 * \param
 *
 * \returns
 *
 *****************************************************************************************************/
void
   lbtrace_word_process ( lbtrace_state_t *lbtrace_state, time_nsecs_t time_nsecs, unsigned int word)
{
   packet_accumulation_t
      *pa;

   pa = &lbtrace_state->packet_accumulation;

#ifdef DEBUG
   printf("%10lld: WORD: %x state: %d accumulated: %d size_words: %d [ %x %x %x %x %x ] \n\n",
	  time_nsecs,
	  word, lbtrace_state->state, packet_accumulated_words, size_words,
	  packet_words[0],
	  packet_words[1],
	  packet_words[2],
	  packet_words[3],
	  packet_words[4]);
#endif

   switch(pa->state) {
      case STATE_IDLE:
	 if (word == TRACE_MAGIC || word == TRACE_MAGIC_WITH_CYCLE_COUNT) {

/*
 * Get rid of stale data.
 */
	    pa->accumulated_words = 0;
	    memset(pa->packet, 0x11, sizeof(pa->packet));


	    pa->state = (word == TRACE_MAGIC) ? STATE_PACKET_SIZE : STATE_CYCLE_COUNT;
	 }
	 else {
	    printf("INFO: not synchronized.  Ignore: %x (lbtrace: %d) \n", word, lbtrace_state->which);
	 }

	 break;

      case STATE_CYCLE_COUNT:
	 pa->cycle_count = word;
	 pa->state = STATE_PACKET_SIZE;
	 break;

	 /*
	  * When we get this word, it will have the line number and the number of remaining words together
	  */
      case STATE_PACKET_SIZE:

	 pa->lineno = word & 0xffff;

	 /*
	  * Get the number of words of payload which follow the tag word.
	  */
	 pa->size_words = ((word >> 28) & 0xf);

	 if (pa->size_words > NUMBER_PACKET_ENTRIES) {
	    printf("ERROR: invalid packet size: %x \n", pa->size_words);
	    pa->state = STATE_IDLE;
	 }
	 else {
	    pa->state = STATE_GET_TAG;
	 }
	 break;

      case STATE_GET_TAG:
	 pa->tag = word;
	 pa->state = STATE_GET_WORDS;

	 /*
	  * If no optional arguments, then process the packet now.
	  */
	 if (pa->size_words == 0) {
	    packet_process(lbtrace_state->which, time_nsecs, pa->tag, pa->lineno, 0, NULL);
	    pa->state = STATE_IDLE;
	 }
	 break;

      case STATE_GET_WORDS:
	 pa->packet[pa->accumulated_words++] = word;

	 if (pa->accumulated_words == pa->size_words) {

	    /*
	     * If the trace data has cycle counts, then use that for the time stamp
	     */
	    if (pa->cycle_count) {

	       /*
		* Cycle count is in terms of uSecs
		*/
	       time_nsecs = (pa->cycle_count * 1000);
	    }

	    packet_process(lbtrace_state->which,
			   time_nsecs,
			   pa->tag,
			   pa->lineno,
			   pa->accumulated_words,
			   pa->packet);

	    pa->state = STATE_IDLE;
	 }
	 break;

      default:
	 printf("ERROR: invalid state: %d \n", pa->state);
	 exit(1);

   } // end of switch
}
