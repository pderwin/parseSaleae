#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "csv.h"
#include "parseSaleae.h"

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
   hex_process_file (FILE *fp)
{
   char
      buf[128];
   unsigned int
      sample = 0,
      word;

   /*
    * Read frames, putting data values into next_frame.
    */
   while(fgets(buf, sizeof(buf), fp)) {

      sample++;

      word = strtoul(buf, NULL, 16);

      (void) word;

      /*
       * process word from this line.
       */
      //      lbtrace_word_process(&lbtrace_state_0, 0, word);
      }

   printf("%d samples processed\n", sample);
}
