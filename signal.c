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
    uint32_t
	previous_val;
    signal_t
	*signal,
	**signal_pp;

    /*
     * process each signal in the parser's list.
     */
    signal_pp = parser->signals;

    /*
     * parser may not have any signals.  Protect against SEG FAULT
     */
    if (signal_pp == NULL) {
	return;
    }

    while(*signal_pp) {

	signal = *signal_pp;

	/*
	 * Keep from getting a bad transition value on first sample
	 */
	if ( !signal->_previous_is_valid ) {
	    signal->val = signal->_buffered_raw = signal->_raw;
	    signal->_previous_is_valid = 1;
	}

	/*
	 * Keep track of previous value for edge detection
	 */
	previous_val = signal->val;

	signal->falling_edge = 0;
	signal->rising_edge  = 0;

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
	if (signal->_buffered_raw != signal->val) {

	    /*
	     * if the signal has been stable long enough, then we can report the new value.
	     */
	    if ((time_nsecs - signal->_last_buffered_change_nsecs) > signal->deglitch_nsecs) {

		signal->val = signal->_raw;

		/*
		 * Set flags for falling and rising edges.
		 */
		if ((signal->val == 0) && (previous_val == 1) ) {
		    signal->falling_edge = 1;
		}
		if ((signal->val == 1) && (previous_val == 0) ) {
		    signal->rising_edge = 1;
		}

	    }
	}

	signal_pp++;
    }
}
