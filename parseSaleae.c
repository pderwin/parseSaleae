#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "csv.h"
#include "parseSaleae.h"

static void
   processInputFile (char *inputFileName);

static void
   process_frame (frame_t *frame);
Csv_t
   csv_time,
   csv_trigger;

lbtrace_state_t
   lbtrace_state_0 = { .idx = 0 },
   lbtrace_state_1 = { .idx = 1 };

int
   main(int argc, char *argv[])
{
   char
      *inputFileName = NULL;

   setbuf(stdout, NULL);

   printf("parseSaleae v1.0\n");
   
   {
      int c;

      static struct option long_options[] = {
	 {"help",   0, 0, 0},
	 {0, 0, 0, 0}
      };

      while (1) {
	 int option_index = 0;

	 c = getopt_long (argc, argv, "hv:", long_options, &option_index);
	 if (c == -1)
	    break;

	 switch (c) {

	    default:
	    case 'h':
	       printf("parseTrace - parse Trace and likely create a listing\n\n");
	       printf("Usage:\n");
	       printf("\tparseJtag input_file_name \n\n");
	       exit(0);
	 }
      }
   }


   if (optind < argc) {
      inputFileName = argv[optind];
   }

   printf("INPUT FILE NAME: '%s' \n", inputFileName);

   /*
    * Open and process the input file.
    */
   processInputFile(inputFileName);

   return(0);
}

static void
   findLbtrace(unsigned int idx, lbtrace_t *lbtrace)
{
   char
      name[128];

   sprintf(name, "d0_%d", idx);
   csv_find(name, &lbtrace->data0);
   
   sprintf(name, "d1_%d", idx);
   csv_find(name, &lbtrace->data1);

   sprintf(name, "clk_%d", idx);

   csv_find(name, &lbtrace->clk);

   sprintf(name, "trigger_%d", idx);
   csv_find(name, &lbtrace->trigger);
   
   sprintf(name, "irq_%d", idx);
   csv_find(name, &lbtrace->irq);
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
   processInputFile (char *inputFileName)
{
   unsigned int
      sample = 0;
   FILE
      *fp;
   frame_t
      current_frame,
      next_frame;

   memset(&current_frame, 0, sizeof(current_frame));
   memset(&next_frame,    0, sizeof(next_frame));
   
   if ((fp = fopen(inputFileName, "rb")) == NULL) {
      printf("Error opening input file: %s\n",
	     inputFileName);
      exit(1);
   }


   /*
    * Process the header off of the file.
    */
   csv_header_read(fp);

   csv_find_time("Time [s]", &next_frame.time_nsecs);

   /*
    * Find the CSV fields of interest
    */
   findLbtrace(0, &next_frame.lbtrace_0);
   findLbtrace(1, &next_frame.lbtrace_1);
   
   /*
    * Try to find uart entries.
    */
   csv_find("G2_xmit", &next_frame.G2_xmit);
   csv_find("G2_recv", &next_frame.G2_recv);
   
   csv_find("RTS", &next_frame.RTS);

   next_frame.G2_xmit = 1;
   next_frame.G2_recv = 1;
   
   /*
    * Read the first frame and process it.  Fills in values in 'next_frame'.
    */
//   printf("Before read: %d %p \n", next_frame.lbtrace_1.idx, &next_frame.lbtrace_1.idx);
   
   csv_data_read(fp);

//   printf("After read: %d %p \n", next_frame.lbtrace_1.idx, &next_frame.lbtrace_1.idx);

   memcpy(&current_frame, &next_frame, sizeof(current_frame));
   
   process_frame(&current_frame);

   /*
    * Read frames and process them.
    */
   while(csv_data_read(fp)) {

      next_frame.sample = sample++;
      
      /*
       * process frames until we would pass the timestamp of the next frame.
       */
      while(current_frame.time_nsecs < next_frame.time_nsecs) {
	 
	 process_frame(&current_frame);
	 
	 current_frame.time_nsecs += SAMPLE_TIME;
      }


      /*
       * At this point, we copy the next frame into 'current' and repeat.
       */
      memcpy(&current_frame, &next_frame, sizeof(current_frame));
   }

   printf("%d lines processed\n", sample);
}


#if 0 
   /*
    * Convert the time to just nSecs
    */
   if (csv_time) {
      frame->time_nsecs = convertTime(csv_time);
   }
   
   if (csv_data_0 && csv_data_1) {
      frame->data   = (csv_data_1->value << 1) | csv_data_0->value;
   }

   if (csv_clk) {
      frame->clk    = csv_clk->value;
   }
   
   return(1);
}
#endif

static void
   process_frame (frame_t *frame)
{
   /*
    * If we detected a falling edge of CLK, then process this frame.
    */
   lbtrace_check(frame, &frame->lbtrace_0, &lbtrace_state_0);
//   lbtrace_check(frame, &frame->lbtrace_1);

   /*
    * Process all uart entries.
    */
   uart_process(frame);

   rts_process(frame);
}
