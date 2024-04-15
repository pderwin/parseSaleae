#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csv.h"
#include "signal_names.h"

#define NUMBER_ALIASES (16)

static uint32_t    alias_count;
static csv_alias_t aliases[NUMBER_ALIASES];

/*-------------------------------------------------------------------------
 *
 * name:        signal_names_filename
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void signal_names_filename (const char *filename)
{
    char
	buf[256],
	*new_name,
	*old_name;
    FILE
	*fp;
    csv_alias_t
	*ap;

    if ((fp = fopen(filename, "r")) == NULL) {
	fprintf(stderr, "%s: Error opening file: '%s'\n", __func__, filename);
	exit(1);
    }

    printf("%s: Reading CSV aliases from: %s' \n", __func__, filename);

    ap = aliases;

    while( fgets(buf, sizeof(buf), fp)) {

	if (alias_count == NUMBER_ALIASES) {
	    fprintf(stderr, "%s: too many aliases\n", __func__);
	    exit(1);
	}

	old_name = strtok(buf, "=");
	new_name = strtok(NULL, "\n");

	ap->new_name = strdup(new_name);
	ap->old_name = strdup(old_name);

	ap++;
	alias_count++;

    }

    fclose(fp);

    printf("%d aliases found\n", alias_count);

    /*
     * Provide these aliases to the csv library
     */
    csv_alias_set(aliases, alias_count);
}
