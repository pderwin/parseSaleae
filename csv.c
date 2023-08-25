#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "csv.h"
#include "parseSaleae.h"
#include "parser.h"

/*
 * parser may set this to get filtering capability.
 */
static unsigned int sample_time_nsecs;

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
   csv_process_file (FILE *fp)
{
    int
	rc;
    unsigned int
	sample = 0;
    frame_t
	frame;
    time_nsecs_t
	max_synthesize_time,
	next_frame_time;

    memset(&frame, 0, sizeof(frame_t));

    /*
     * Process the header off of the file.
     */
    csv_header_read(fp);

    csv_find_time("Time [s]", &frame.time_nsecs);

    /*
     * allow all parsers to search for their signals and
     * set their enable bits accordingly.
     */
    parser_connect();

    /*
     * after connections made, call post_connects.  Allows a parser to redirect
     * the output from one parser to another ( LR1110 grabbing UART output )
     */
    parser_post_connect();

    /*
     * Verify we have at least one parser active
     */
    if (parser_active_count() == 0) {
	fprintf(stderr, "ERROR: No parsers enabled\n");
	exit(1);
    }

    printf("CSV sample time: %d\n", sample_time_nsecs);

    /*
     * Read the first frame and process it.  Fills in values in 'next_frame'.
     */
    rc = csv_data_read(fp);

    parser_process_frame(&frame);

    /*
     * Read frames, putting data values into the fields where the users have requested them.
     */
    while(csv_data_read(fp) == 0) {

	frame.sample = sample++;

	/*
	 * process this new frame at least once.
	 */
	parser_process_frame(&frame);

	/*
	 * The Saleae puts out samples only when a line changes, so we may need to synthesize
	 * samples in between.
	 */
	if (sample_time_nsecs) {

	    /*
	     * Get time value from next sample, but do not parse/deposit the data items.
	     */
	    rc = csv_pre_read_time(fp, &next_frame_time);

	    /*
	     * If there was a new time given, then synthesize frames.
	     */
	    if (rc == 0) {

		/*
		 * process current data multiple times to synthesize input.
		 *
		 * Surely don't need to synthesize more than 10 mSecs...
		 */
		max_synthesize_time = (10 * 1000 * 1000);

		while ((frame.time_nsecs + sample_time_nsecs ) < next_frame_time && (max_synthesize_time > 0) ) {

		    parser_process_frame(&frame);

		    frame.time_nsecs    += sample_time_nsecs;
		    max_synthesize_time -= sample_time_nsecs;
		}
	    }
	}
    }

    printf("\n\n%d lines processed\n", sample);
}

/*-------------------------------------------------------------------------
 *
 * name:        csv_sample_time_nsecs
 *
 * description: set the time resolution needed for a given parser.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void csv_sample_time_nsecs (uint32_t nsecs)
{
    /*
     * If sample time is zero, then take new value.
     */
    if (sample_time_nsecs == 0) {
	sample_time_nsecs = nsecs;
    }
    /*
     * When not zero, only take new value if it is less than current value.
     */
    else {
	if (nsecs < sample_time_nsecs) {
	    sample_time_nsecs = nsecs;
	}
    }
}
