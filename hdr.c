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
void hdr(log_file_t *lf, time_nsecs_t time_nsecs, char *h)
{
    hdr_with_lineno(lf, time_nsecs, "", h, 0);
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
void hdr_with_lineno (log_file_t *lf, time_nsecs_t time_nsecs, char *group_str, char *h, unsigned int lineno)
{
    FILE *fp = lf->fp;
    time_nsecs_t last_time_nsecs = lf->time_nsecs;

    if (show_time_stamps()) {
	/*
	 * If first time in, then init last time stamp
	 */
	if (last_time_nsecs == 0) {
	    last_time_nsecs = time_nsecs;
	}

	print_time_nsecs(fp, time_nsecs);
	fprintf(fp, "| ");

	print_time_nsecs(fp, time_nsecs - last_time_nsecs);
	fprintf(fp, "| ");

	/*
	 * save for next call.
	 */
	lf->time_nsecs = time_nsecs;
    }

   fprintf(fp, "%-6.6s | %-25.25s | ", group_str, h);

   if (lineno) {
       fprintf(fp, "%4d", lineno);
   }
   else {
       fprintf(fp, "    ");
   }
   fprintf(fp, " | ");
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
