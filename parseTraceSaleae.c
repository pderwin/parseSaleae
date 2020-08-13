#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "csv.h"
#include "parseTrace.h"

static void
   processInputFile (char *inputFileName);

static void
   process_frame (frame_t *frame);
static int
   read_frame (FILE *fp, frame_t *frame);
Csv_t
   csv_data_0,
   csv_data_1,
   csv_clk,
   csv_time,
   csv_trigger;

int
   main(int argc, char *argv[])
{
   char
      *inputFileName = NULL;

   setbuf(stdout, NULL);

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

/**
 ****************************************************************************************************
 *
 * \brief   Convert a string into a timestamp
 *
 * \param
 *
 * \returns rc -- time in units of nSecs
 *
 *****************************************************************************************************/
long long int
   convertTime (Csv_t csvTime)
{
   char
      *units_ptr;
   double
      fract;
   unsigned long long int
      rc;


   if (csvTime->token == NULL) {
      printf ("no time found\n");
      exit(1);
   }

   /*
    * time value for Saleae is in terms of seconds and is floating point.
    *
    * Example: -0.049993856000000
    */
   fract  = strtod(csvTime->token, &units_ptr);

   /*
    * Time is in terms of seconds.  Kultiply fraction to turn into nSec.
    */
   rc = fract * (1000 * 1000 * 1000);

   return(rc);
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
   FILE
      *fp;
   frame_t
      current_frame,
      next_frame;

   if ((fp = fopen(inputFileName, "rb")) == NULL) {
      printf("Error opening input file: %s\n",
	     inputFileName);
      exit(1);
   }


   /*
    * Process the header off of the file.
    */
   csv_header_read(fp);

   /*
    * Find the CSV fields of interest
    */
   csv_data_0        = csvFind("d0");
   csv_data_1        = csvFind("d1");

   csv_clk           = csvFind("clk");
   csv_time          = csvFind("Time[s]");

   csv_trigger       = csvFind("trigger");

   if ( csv_find_errors(1) ) {
      exit(1);
   }

   memset(&current_frame, 0, sizeof(current_frame));
   memset(&next_frame,    0, sizeof(next_frame));

   /*
    * Read the first frame and process it.
    */
   read_frame(fp, &current_frame);
   process_frame(&current_frame);

   /*
    * Read frames and process them.
    */
   while(read_frame(fp, &next_frame)) {

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
}

/**
 ****************************************************************************************************
 *
 * \brief
 *
 * \param
 *
 * \returns  1 - data was read
 *           0 - some kind of error or end-of-file
 *
 *****************************************************************************************************/
static int
   read_frame (FILE *fp, frame_t *frame)
{
   static unsigned int
      sample = 0;

   if (csv_data_read(fp) != 0) {
      return(0);
   }

   frame->sample = sample++;

   /*
    * Convert the time to just nSecs
    */
   frame->time_nsecs = convertTime(csv_time);

   frame->data   = (csv_data_1->value << 1) | csv_data_0->value;
   frame->clk    = csv_clk->value;

   frame->trigger = csv_trigger->value;

   return(1);
}

static void
   process_frame (frame_t *frame)
{
   /*
    * If we detected a falling edge of CLK, then process this frame.
    */
   clock_check(frame);
}
