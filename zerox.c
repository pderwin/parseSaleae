#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parseTrace.h"

/*
 * Don't allow another falling edge for 100 uSecs
 */
#define FILTER_HIGH_TIME_NSECS (100000)

#define FILTER_COUNT (FILTER_HIGH_TIME_NSECS / SAMPLE_TIME)

static unsigned int
   zerox_high_count;

static int
   last_falling_edge;

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
   zerox_check (frame_t *jf)
{
   if ((jf->zerox == 0) && (zerox_high_count > FILTER_COUNT) ) {
      hdr(jf);
      printf("ZEROX: falling edge ");
      if (last_falling_edge) {
	 printf("(delta: %lld)", jf->time_nsecs - last_falling_edge);
      }
      printf("\n");
      
      last_falling_edge = jf->time_nsecs;
   }

   /*
    * Keep track of how long the zerox bit is high so that we can filter out noise on rising edge
    */
   if (jf->zerox == 0) {
      zerox_high_count =0;
   }
   else {
      zerox_high_count++;
   }
}

