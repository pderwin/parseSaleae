#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <trace.h>
#include "parseSaleae.h"
#include "tag.h"

static void tag_dump(void);

/*-------------------------------------------------------------------------
 *
 * name:
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void tag_atexit (void)
{
    atexit(tag_dump);
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
static void tag_dump(void)
{
   char
      buf[512],
      tag_filename[128];
   unsigned int
      tag;
   FILE
      *fp;

   sprintf(tag_filename, "%s/trace.h", ZEPHYR_INCLUDES);

   sprintf(buf, "grep TRACE_ %s | grep -v define", tag_filename);

   fp = popen(buf, "r");

   tag = TAG_ZERO;

   printf("\n\nDump of trace.h tag words (%s): \n", tag_filename);

   while(fgets(buf, sizeof(buf), fp)) {
      printf("     %x : %s", tag, buf);
      tag++;
   }

   pclose(fp);
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
void
   tag_init (void)
{
   atexit(tag_dump);
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
void tag_find(unsigned int tag)
{
    char
	buf[512],
	tag_filename[128];
    unsigned int
	current_tag;
    FILE
	*fp;

    sprintf(tag_filename, "%s/trace.h", ZEPHYR_INCLUDES);

    sprintf(buf, "grep TAG_ %s | grep -v define", tag_filename);

    fp = popen(buf, "r");

    current_tag = TAG_ZERO;

    while (fgets(buf, sizeof(buf), fp)) {
	//      printf("tag: %x  %s \n", current_tag, buf);
	if (current_tag == tag) {
	    break;
	}

	current_tag++;
    }

    pclose(fp);

    printf("tag: %x Appears to be: %s", tag, buf);
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
uint32_t tag_scan(uint32_t max_str_count, char *tag_strs[])
{
    char
	buf[512],
	*cp,
	tag_filename[128];
    uint32_t
	current_tag;
    FILE
	*fp;

    sprintf(tag_filename, "%s/trace.h", ZEPHYR_INCLUDES);

    sprintf(buf, "grep TAG_ %s | grep -v define", tag_filename);

    fp = popen(buf, "r");

    current_tag = 0;

    while (fgets(buf, sizeof(buf), fp)) {
	//      printf("tag: %x  %s \n", current_tag, buf);

	if (current_tag == max_str_count) {
	    fprintf(stderr, "Too many tags for given array \n");
	    exit(1);
	}

	/*
	 * get first token
	 */
	cp = strtok(buf, ", ");

	/*
	 * Get rid of TAG_ on the front
	 */
	if (!strncmp(cp, "TAG_", 4)) {
	    cp += 4;
	}

	tag_strs[current_tag] = strdup(cp);

	current_tag++;
    }

    pclose(fp);

    return current_tag;
}
