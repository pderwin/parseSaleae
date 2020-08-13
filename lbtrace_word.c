#include <stdio.h>
#include <trace.h>
#include <stdlib.h>
#include <string.h>
#include "parseSaleae.h"

// #define DEBUG 1

//
// cannot use cycle count with mpu-enabled build.  See drivers/trace/trace.c
//
#define HAVE_CYCLE_COUNT 0

enum {
   STATE_IDLE,
   STATE_CYCLE_COUNT,
   STATE_PACKET_SIZE,
   STATE_GET_TAG,
   STATE_GET_WORDS,
};

static unsigned int
   state;

#define PACKET_MAXIMUM_WORDS ( 5 )

static unsigned int
   cycle_count,
   packet_accumulated_words,
   lineno,
   size_words,
   packet_words[PACKET_MAXIMUM_WORDS],
   tag;

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
   lbtrace_word_process (unsigned int word)
{

#ifdef DEBUG
   printf("WORD: %x state: %d accumulated: %d size_words: %d [ %x %x %x %x %x ] \n", word, state, packet_accumulated_words, size_words,
	  packet_words[0],
	  packet_words[1],
	  packet_words[2],
	  packet_words[3],
	  packet_words[4]);
#endif
   
   switch(state) {
      case STATE_IDLE:
	 if (word == TRACE_MAGIC) {

/*
 * Get rid of stale data.
 */
	    packet_accumulated_words = 0;
	    memset(packet_words, 0x11, sizeof(packet_words));
   
#if HAVE_CYCLE_COUNT
	    state = STATE_CYCLE_COUNT;
#else	    
	    state = STATE_PACKET_SIZE;
#endif	    
	 }
	 else {
	    printf("INFO: not synchronized.  Ignore: %x \n", word);
	 }
	 
	 break;

      case STATE_CYCLE_COUNT:
	 cycle_count = word;
	 state = STATE_PACKET_SIZE;
	 break;
	 
	 /*
	  * When we get this word, it will have the line number and the number of remaining words together
	  */
      case STATE_PACKET_SIZE:

	 lineno = word & 0xffff;

	 /*
	  * Get the number of words of payload which follow the tag word.
	  */
	 size_words = ((word >> 28) & 0xf);

	 if (size_words >=PACKET_MAXIMUM_WORDS) {
	    printf("ERROR: invalid packet size: %x \n", size_words);
	    state = STATE_IDLE;
	 }
	 else {
	    state = STATE_GET_TAG;
	 }
	 break;

      case STATE_GET_TAG:
	 tag = word;
	 state = STATE_GET_WORDS;

	 /*
	  * If no optional arguments, then process the packet now.
	  */
	 if (size_words == 0) {
	    packet_process(tag, lineno, 0, NULL);
	    state = STATE_IDLE;
	 }
	 break;
	 
      case STATE_GET_WORDS:
	 packet_words[packet_accumulated_words++] = word;

	 if (packet_accumulated_words == size_words) {
	    
	    packet_process(tag, lineno, packet_accumulated_words, packet_words);

	    state = STATE_IDLE;
	 }
	 break;
	 
      default:
	 printf("ERROR: invalid state: %d \n", state);
	 exit(1);
	 
   } // end of switch
}

