#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <trace.h>
#include "parseSaleae.h"

const char *size_strs[] = {
   "?",
   "?",
   "?",    
   "?",   
   " 32B", 
   " 64B", 
   "128B", 
   "256B",
   "512B", 
   "  1K",   
   "  2K",   
   "  4K",   
   "  8K",   
   " 16K", 
   " 32K",  
   " 64K",  
   "128K", 
   "256K", 
   "512K", 
   "  1M",   
   "  2M",  
   "  4M",  
   "  8M",  
   " 16M", 
   " 32M",  
   " 64M", 
   "128M", 
   "256M", 
   "512M", 
   "  1G",  
   "  2G",  
   "  4G"
};


int          sample;
unsigned int uart;
static long long int time_stamp;

enum {
   TAG_WORD,
   TAG_UART,
   TAG_ZEROX
};

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
long long int get_time_stamp (void)
{
   return time_stamp;
}

FILE
   *fp;
static unsigned int next (unsigned int *tag_typeP);

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
void hdr(char *h)
{
   hdr_with_lineno(h, 0);
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
void hdr_with_lineno (char *h, unsigned int lineno)
{
   static long long int
      last_time_stamp = 0;

   /*
    * If first time in, then init last time stamp
    */
   if (last_time_stamp == 0) {
      last_time_stamp = time_stamp;
   }
   
   print_time_stamp(time_stamp);
   printf("| ");
   
   print_time_stamp(time_stamp - last_time_stamp);
   printf("| ");

   /*
    * save for next call.
    */
   last_time_stamp = time_stamp;

   printf("%-20.20s | ", h);
   if (lineno) {
      printf("%4d", lineno);
   }
   else {
      printf("    ");
   }
   printf(" | ");
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
   print_time_stamp (long long int time_stamp)
{
   if (time_stamp < 0) {
      printf("-");
      time_stamp = -time_stamp;
   }
   else {
	printf(" ");
   }
   
   if (time_stamp > 1000000) { 
	 printf("%7.2f ms ", time_stamp / 1000000.0);
   }
   else if (time_stamp > 1000) { 
	 printf("%7.2f us ", time_stamp / 1000.0);
   }
   else {
      printf("%7lld ns ", time_stamp / 1000);
   }
}

static unsigned int next (unsigned int *tag_typeP)
{
   char
      buf[256],
      *cp,
      *w;
   unsigned int
      tag_type,
      value;
   
   if (fgets(buf, sizeof(buf), fp) == NULL) {
      printf("DONE\n");
      exit(0);
   }   
   
#define TERM " \n"
   
   time_stamp = strtol(strtok(buf, TERM), NULL, 10);
   sample     = strtol(strtok(NULL, TERM), NULL, 10);
   w          = strtok(NULL, TERM);

   cp = strtok(NULL, TERM);
   if (cp) {
      value      = strtol(cp, NULL, 16);
   }
   else {
      value = 0xffffffff;
   }

   if (! strcmp(w, "UART:")) {
      tag_type = TAG_UART;
   }
   else if (! strcmp(w, "ZEROX:")) {
      tag_type = TAG_ZEROX;
   }
   else {
      tag_type = TAG_WORD;
   }
   
   if (tag_typeP) {
      *tag_typeP = tag_type;
   }
   
   return(value);
}



