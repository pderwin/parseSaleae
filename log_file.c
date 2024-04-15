#include <stdlib.h>
#include "log_file.h"
#include "parseSaleae.h"

log_file_t *stdout_lf;

/*-------------------------------------------------------------------------
 *
 * name:        log_file_create
 *
 * description: create a log file and create a structure that will contain
 *              the FILE * and a time_nsecs_t such that all headers written
 *              to this log will have delta times relavent to other items
 *              in this log.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
log_file_t *log_file_create (char *in_filename)
{
    char
	filename[128];
    FILE *fp;

    sprintf(filename, "/tmp/%s", in_filename);

    if ((fp = fopen(filename, "w")) == NULL) {
	fprintf(stderr, "Error opening log file: '%s'\n", filename);
	exit(1);
    }

    /*
     * make non-buffered
     */
    setbuf(fp, NULL);

    fprintf(fp, "Log: %s\n", filename);

   /*
    * Print wall-clock time to the file.
    */
    localtime_print(fp);

    return log_file_alloc(fp);
}

log_file_t *log_file_alloc(FILE *fp)
{
    log_file_t *lf;

    lf = malloc(sizeof(log_file_t));

    if (lf == NULL) {
	fprintf(stderr, "Error allocating new log file\n");
	exit(1);
    }

    lf->fp         = fp;
    lf->time_nsecs = 0;

    return lf;
}


void CONSTRUCTOR init()
{
    stdout_lf = log_file_alloc(stdout);
}
