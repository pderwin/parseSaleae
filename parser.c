#include <stdlib.h>
#include <string.h>
#include "frame.h"
#include "log_file.h"
#include "parseSaleae.h"
#include "parser.h"
#include "signal.h"

/*-------------------------------------------------------------------------
 *
 * Sequence of events:
 *
 *    1) parser_connect_signals -- Check for wire connections on all parsers
 *          The parser will be enabled unless it has a signal list that is
 *          not met.
 *
 *    2) open log files: for all enabled parsers, open their respective
 *          log files.
 *
 *-------------------------------------------------------------------------*/

static parser_t
    *first_parser,
    *last_parser;

/*-------------------------------------------------------------------------
 *
 * name:        connect_signals
 *
 * description: go through signal list and see if they are found or not.
 *
 * input:
 *
 * output:      enable - 0 - parser is not enabled.
 *                       1 - parser is enabled.
 *
 *-------------------------------------------------------------------------*/
static uint32_t connect_signals (parser_t *parser)
{
    int
	signal_find_rc = 0;
    signal_t
	*signal,
	**signal_pp;

    signal_pp = parser->signals;

    while(*signal_pp) {

	signal = *signal_pp;

	/*
	 * always deposit the signal value into the _raw element.  We will deglitch
	 * this data and place into the val_p in signal_deglitch().
	 */
	signal->_find_rc = csv_find_format(signal->name, &signal->_raw, signal->format);

	signal_find_rc |= signal->_find_rc;

	signal_pp++;
    }

    /*
     * If there was an error finding the signals, the disable the parser.
     */
    if (signal_find_rc) {
	parser->enable = 0;
    }

    /*
     * If here, then the parser is enabled.  If it specified a sample time, then honor that.
     */
    else {
	parser_enable(parser);
    }

    return parser->enable;
}


/*-------------------------------------------------------------------------
 *
 * name:        parser_active_count
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
uint32_t parser_active_count (void)
{
    uint32_t
	active_count = 0;
    parser_t
	*parser;

    parser = first_parser;

    while (parser) {

	if (parser->enable) {
	    active_count++;
	}

	parser = parser->next;
    }

    return active_count;
}

/*-------------------------------------------------------------------------
 *
 * name:        parser_connect
 *
 * description: allow all parsers to connect to their frame elements.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void parser_connect (void)
{
    parser_t
	*parser;

    parser = first_parser;

    while (parser) {


	/*
	 * Assume parser is enabled at first.
	 */
	parser->enable = 1;

	/*
	 * If a parser has a signal list, see if the signals are found.
	 */
	if (parser->signals) {

	    /*
	     * Try to connect to the signals, and when successful, call its connect callback.
	     */
	    if (connect_signals(parser)) {
		if (parser->connect) {
		    (parser->connect)(parser);
		}
	    }
	}

	parser = parser->next;
    }
}

/*-------------------------------------------------------------------------
 *
 * name:        parser_enable
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void parser_enable (parser_t *parser)
{
    parser->enable = 1;

    if (parser->log_file_name) {
	parser->log_file = log_file_create(parser->log_file_name);
    }

    /*
     * If the parser wants to set the sample time, do that.
     */
    if (parser->sample_time_nsecs) {
	csv_sample_time_nsecs(parser->sample_time_nsecs);
    }
}


/*-------------------------------------------------------------------------
 *
 * name:        parser_redirect_output
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void parser_redirect_output(char *parser_name, log_file_t *log_file)
{
    parser_t
	*parser;

    parser = first_parser;

    while(parser) {

	/*
	 * If we're still enabled, then maybe call the connect callback
	 */
	if (parser->enable && !strcmp(parser->name, parser_name)) {
	    fprintf(parser->log_file->fp, "Output being redirected\n");
	    parser->log_file = log_file;

	    return;
	}

	parser = parser->next;
    }

    printf("INFO: Redirect of parser: '%s' failed\n", parser_name);
}

/*-------------------------------------------------------------------------
 *
 * name:        parser_post_connect
 *
 * description: Call each parser which is going to run, before processing
 *              any data.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void parser_post_connect (void)
{
    parser_t
	*parser;

    parser = first_parser;

    while(parser) {

	/*
	 * If we're still enabled, then maybe call the connect callback
	 */
	if (parser->enable && parser->post_connect) {
	    parser->post_connect(parser);
	}

	parser = parser->next;
    }
}

/*-------------------------------------------------------------------------
 *
 * name:        parser_dump
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void parser_dump (void)
{
    parser_t
	*parser;
    signal_t
	*signal,
	**signal_pp;

    parser = first_parser;

    printf("\n\nParser     | enabled |       log file       | missing signals\n");
    printf("-----------+---------+----------------------+------------\n");

    while(parser) {


	printf("%-10.10s |    %d    | ",parser->name, parser->enable);
	printf("%-20.20s | ", parser->log_file_name ? parser->log_file_name : "");

	if (parser->signals) {

	    signal_pp = parser->signals;

	    while(*signal_pp) {

		signal = *signal_pp;

		if (signal->_find_rc) {
		    printf("%s ", signal->name);
		}

		signal_pp++;
	    }
	} // while signals

	printf("\n");

	parser = parser->next;

    } // while parser
}

/*-------------------------------------------------------------------------
 *
 * name:        parser_register
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void parser_register (parser_t *parser)
{
   if (last_parser) {
      last_parser->next = parser;
   }
   else {
      first_parser = parser;
   }

   last_parser = parser;
   parser->next = NULL;
}

/*-------------------------------------------------------------------------
 *
 * name:        parser_process_frame
 *
 * description: process each frame of data.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void parser_process_frame (frame_t *frame)
{
    parser_t
	*parser;

    parser = first_parser;

    while (parser) {

	if (parser->enable) {

	    /*
	     * Need to process the signals, possibly de-glitching them.
	     */
	    signal_deglitch(parser, frame->time_nsecs);

	    if (parser->process_frame) {
		parser->process_frame(parser, frame);
	    }
	}

	parser = parser->next;
    }
}

static void CONSTRUCTOR init(void)
{
    atexit(parser_dump);
}
