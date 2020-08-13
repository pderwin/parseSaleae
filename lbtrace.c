#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parseSaleae.h"


static unsigned int
   signature[] = {3, 0, 3};

static void
   falling_edge_clock (frame_t *jf, lbtrace_t *lbtrace, lbtrace_state_t *lbtrace_state);

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
   lbtrace_check (frame_t *frame, lbtrace_t *lbtrace, lbtrace_state_t *lbtrace_state)
{
//   static unsigned int count = 0;
   
   if ((lbtrace->clk == 0) && (lbtrace_state->last_clock == 1)) {
      falling_edge_clock(frame, lbtrace, lbtrace_state);
   }

   /*
    * Save current state.
    */
   lbtrace_state->last_clock = lbtrace->clk;


//   if (++count > 100000) { printf("EXIT EARLY\n"); exit (1); }
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
static void
   falling_edge_clock (frame_t *jf, lbtrace_t *lbtrace, lbtrace_state_t *lbtrace_state)
{
   unsigned int
      i,
      val;
   
//   time_format(jf);
      
   /*
    * Move the other values forward in the array
    */
   for (i=1; i<(PACKET_SIZE); i++) {
      lbtrace_state->packet[i-1] = lbtrace_state->packet[i];
   }
   
   /*
    * Put the value into the array, using modulo math.
    */
   lbtrace_state->packet[PACKET_SIZE-1] = ((lbtrace->data1 << 1) | lbtrace->data0);

#if 0
   for (i=0; i<PACKET_SIZE; i++) {
      printf(" %d ", packet[i]);
   }
   printf("\n");
#endif
   
   /*
    * If the start of the packet has the correct signature, then print the value
    */
   if (memcmp(lbtrace_state->packet, signature, sizeof (signature)) == 0) {

      for (i=3; i<PACKET_SIZE; i++) {
	 val = (val << 2) | lbtrace_state->packet[i];
      }

      hdr(jf, "LBTRACE");
      printf ("WORD: %08x \n", val);
      lbtrace_word_process(val);
      
      memset(lbtrace_state->packet, 0, sizeof(lbtrace_state->packet));
   }
}

