#include <stdlib.h>
#include <string.h>
#include "log_file.h"
#include "parseSaleae.h"

static parser_t
    *first_parser,
    *last_parser;

static void parser_connect_signals (parser_t *parser);

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

    while(parser) {

	/*
	 * Assume parser is enabled at first.
	 */
	parser->enable = 1;

	/*
	 * If a parser has a signal list, see if the signals are found.
	 */
	if (parser->signals) {
	    parser_connect_signals(parser);
	}

	/*
	 * If we're still enabled, then maybe call the connect callback
	 */
	if (parser->enable && parser->connect) {
	    parser->enable = parser->connect();
	}

	if (parser->enable) {
	    if (parser->log_file) {
		parser->lf = log_file_create(parser->log_file);
	    }
	    if (parser->sample_time_nsecs) {
		csv_sample_time_nsecs(parser->sample_time_nsecs);
	    }
	}

	parser = parser->next;
    }
}

/*-------------------------------------------------------------------------
 *
 * name:        parser_connect_signals
 *
 * description: go through signal list and see if they are found or not.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static void parser_connect_signals (parser_t *parser)
{
    int
	signal_find_rc = 0;
    signal_t
	*signal;

    signal = parser->signals;

    while(signal->name) {

	signal->find_rc = csv_find_format(signal->name, signal->val_p, signal->format);

	signal_find_rc |= signal->find_rc;

	signal++;
    }

    if (signal_find_rc) {
	parser->enable = 0;
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
	 *signal;

    parser = first_parser;

    printf("\n\nParser     | available | missing signals\n");
    printf("-----------+-----------+----------------\n");
    while(parser) {
	printf("%-10.10s |     %d     | ", parser->name, parser->enable);

	if (parser->signals) {
	    signal = parser->signals;

	    while(signal->name) {
		if (signal->find_rc) {
		    printf("%s ", signal->name);
		}

		signal++;
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

   while(parser) {
      if (parser->enable) {
	 parser->process_frame(frame);
      }

      parser = parser->next;
   }

}

static void CONSTRUCTOR init(void)
{
    atexit(parser_dump);
}
