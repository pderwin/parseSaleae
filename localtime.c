#include <time.h>
#include "parseSaleae.h"

/*-------------------------------------------------------------------------
 *
 * name:        localtime_print
 *
 * description: print the localtime to a file
 *
 * input:       none
 *
 * output:      none
 *
 *-------------------------------------------------------------------------*/
void localtime_print (FILE *fp)
{
    struct tm
	tm;
    time_t
	tim;

    tim = time(NULL);

    if (localtime_r(&tim, &tm) == NULL) {
	printf("Error converting time value\n");
	exit(1);
    }

    fprintf(fp, "Formatted:  %d/%d/%d %2d:%02d:%02d\n\n", tm.tm_mon+1,  tm.tm_mday, tm.tm_year + 1900,  tm.tm_hour, tm.tm_min, tm.tm_sec);
}
