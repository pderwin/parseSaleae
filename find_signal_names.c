#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *find_value(char *buf, char *str);

/*-------------------------------------------------------------------------
 *
 * name:        config_file_path
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
int main (int argc, char *argv[])
{
    char
	buf[256],
	*cp,
	*filename,
	*namestr;
    uint32_t
	device_channel,
	in_rows_settings = 0;
    FILE
	*fp;

    filename = argv[1];

    if ((fp = fopen(filename, "r")) == NULL) {
	fprintf(stderr, "%s: Error opening file: '%s'\n", __func__, filename);
	exit(1);
    }

    fprintf(stderr, "%s: Reading CSV aliases from: %s' \n", __func__, filename);

    while( fgets(buf, sizeof(buf), fp)) {

	/*
	 * Check to see if we're in the 'rowsSettings' area.
	 */
	if (! in_rows_settings) {

	    if (strstr(buf, "rowsSettings")) {
		in_rows_settings = 1;
	    }
	    continue;
	}

	cp = find_value(buf, "name");
	if (cp) {
	    namestr = strdup(cp);
	    continue;
	}

	cp = find_value(buf, "deviceChannel");
	if (cp) {
	    device_channel = atoi(cp);

	    /*
	     * Add this alias to the list.
	     */
	    printf("Channel %d=%s\n", device_channel, namestr);
	    continue;
	}

	/*
	 * Don't process analog signals
	 */
	if (strstr(buf, "Analog")) {
	    break;
	}
    }

    fclose(fp);
}

static char *find_value(char *buf, char *str)
{
    char
	*cp;

    cp = strstr(buf, str);

    if (!cp) {
	return cp;
    }

    cp = strtok(buf, ":");
    if (!cp) {
	return cp;
    }

    cp = strtok(NULL, " ,\"\n");

    return cp;
}
