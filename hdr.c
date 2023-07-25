#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hdr.h"
#include "log_file.h"
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
void hdr(parser_t *parser, time_nsecs_t time_nsecs, char *h)
{
    hdr_with_lineno(parser, time_nsecs, "", h, 0);
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
void hdr_with_lineno (parser_t *parser, time_nsecs_t time_nsecs, char *group_str, char *h, unsigned int lineno)
{
    FILE *log_fp;
    log_file_t *log_file;
    time_nsecs_t last_time_nsecs;

    log_file = parser->log_file;

    log_fp          = log_file->fp;
    last_time_nsecs = log_file->time_nsecs;

    if (show_time_stamps()) {
	/*
	 * If first time in, then init last time stamp
	 */
	if (last_time_nsecs == 0) {
	    last_time_nsecs = time_nsecs;
	}

	print_time_nsecs(log_fp, time_nsecs);
	fprintf(log_fp, "| ");

	print_time_nsecs(log_fp, time_nsecs - last_time_nsecs);
	fprintf(log_fp, "| ");

	/*
	 * save for next call.
	 */
	log_file->time_nsecs = time_nsecs;
    }

   fprintf(log_fp, "%-6.6s | %-25.25s | ", group_str, h);

   if (lineno) {
       fprintf(log_fp, "%4d", lineno);
   }
   else {
       fprintf(log_fp, "    ");
   }
   fprintf(log_fp, " | ");
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
void print_time_nsecs (FILE *fp, time_nsecs_t time_nsecs)
{
    if (time_nsecs < 0) {
	fprintf(fp, "-");
	time_nsecs = -time_nsecs;
    }
    else {
	fprintf(fp, " ");
    }

    if (time_nsecs > 1000000) {
	fprintf(fp, "%8.3f ms ", time_nsecs / 1000000.0);
    }
    else if (time_nsecs > 1000) {
	fprintf(fp, "%8.3f us ", time_nsecs / 1000.0);
    }
    else {
	fprintf(fp, "%8lld ns ", time_nsecs / 1000);
    }
}
