#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "signal.h"

/*-------------------------------------------------------------------------
 *
 * name:        signal_deglitch
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void signal_deglitch (parser_t *parser, time_nsecs_t time_nsecs)
{
    signal_t *signal;

    /*
     * process each signal in the parser's list.
     */
    signal = parser->signals;

    printf("   parser: %s\n", parser->name);

    while(signal->name) {

	/*
	 * Track when the raw signal is changing.
	 */
	if (signal->_raw != signal->_buffered_raw) {
	    signal->_buffered_raw = signal->_raw;
	    signal->_last_buffered_change_nsecs = time_nsecs;
	}

	/*
	 * If the buffered signal has been stable for the deglitch time, then we can
	 * report to the parser
	 */
	if (signal->_buffered_raw != *signal->val_p) {

	    printf("got difference %s %lld  _raw: %x  val_p: %x dg: %lld \n", signal->name, time_nsecs, signal->_raw, *signal->val_p, signal->deglitch_nsecs);

	    /*
	     * if the signal has been stable long enough, then we can report the new value.
	     */
	    if ((time_nsecs - signal->_last_buffered_change_nsecs) > signal->deglitch_nsecs) {
		*signal->val_p = signal->_raw;

		printf("REPORT CHANGE\n");
	    }
	}

	signal++;
    }
}
