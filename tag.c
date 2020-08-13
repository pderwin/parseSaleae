#include <stdio.h>
#include <stdlib.h>
#include <trace.h>
#include "parseLst.h"

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
   tag_dump(void)
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

   tag = TRACE_ZERO;
   
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
void
   tag_find(unsigned int tag)
{
   char      
      buf[512],
      tag_filename[128];
   unsigned int
      current_tag;
   FILE
      *fp;

   /*
    * Catch bad parse error and don't pummel system 
    */
   if ((tag & TRACE_ZERO) != TRACE_ZERO) {
      return;
   }
   
   sprintf(tag_filename, "%s/trace.h", ZEPHYR_INCLUDES);
   
   sprintf(buf, "grep TRACE_ %s | grep -v define", tag_filename);
   
   fp = popen(buf, "r");

   current_tag = TRACE_ZERO;
   
   while (fgets(buf, sizeof(buf), fp)) {
//      printf("tag: %x  %s \n", current_tag, buf);
      if (current_tag == tag) {
	 break;
      }

      current_tag++;
   }

   pclose(fp);
   
   printf("Appears to be: %s", buf);
}

