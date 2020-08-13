#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parseSaleae.h"


#define PACKET_SIZE (16 + 3)

static unsigned int
   packet[PACKET_SIZE];

static unsigned int
   signature[] = {3, 0, 3};

static void
   Afalling_edge_clock (frame_t *jf);

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
   clock_check (frame_t *frame)
{
   static unsigned int
      last_clock = 0;

   if ((frame->clk == 0) && (last_clock == 1)) {
      Afalling_edge_clock(frame);
   }

   /*
    * Save current state.
    */
   last_clock = frame->clk;
   
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
   Afalling_edge_clock (frame_t *jf)
{
   unsigned int
      i,
      val;
   
//   time_format(jf);
   
 
   /*
    * Move the other values forward in the array
    */
   for (i=1; i<(PACKET_SIZE); i++) {
      packet[i-1] = packet[i];
   }
   
   /*
    * Put the value into the array, using modulo math.
    */
   packet[PACKET_SIZE-1] = jf->data;

#if 0
   for (i=0; i<PACKET_SIZE; i++) {
      printf(" %d ", packet[i]);
   }
   printf("\n");
#endif
   
   /*
    * If the start of the packet has the correct signature, then print the value
    */
   if (memcmp(packet, signature, sizeof (signature)) == 0) {

      for (i=3; i<PACKET_SIZE; i++) {
	 val = (val << 2) | packet[i];
      }

      hdr(jf);
      printf ("WORD: %08x \n", val);
      
      memset(packet, 0, sizeof(packet));
   }
}

