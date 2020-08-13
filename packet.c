#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <trace.h>
#include "parseSaleae.h"

// #define DEBUG 1


//static void
//   cpsr_parse (void);

#define hdr(str) hdr_with_lineno(str, lineno)

static unsigned int
   __argc_used,
   __argc_available;
static unsigned int
   *__argv;

static unsigned int
   next(void)
{
   if (__argc_used < __argc_available) {
      return(__argv[__argc_used++]);
   }

   printf("ERROR: attempt to read past end of available packet data.  Received: %d \n", __argc_available);

   return(0);
}

static void
   lookup (char *str);

static void
   from (void)
{
   printf("from: %x ", next());
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
   packet_atexit (void)
{
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
   packet_init (void)
{
   atexit(packet_atexit);
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
   packet_process (unsigned int tag, unsigned int lineno, unsigned int argc, unsigned int argv[])
{
   unsigned int
      val;

   __argc_used = 0;
   __argc_available = argc;
   __argv      = argv;


#ifdef DEBUG
   printf("TAG: %x LINENO: %x (%d) ARGC: %d ", tag, lineno, lineno, argc);
   if (argv) {
      printf("ARGV { %x %x %x %x %x }", argv[0],argv[1],argv[2],argv[3],argv[4]);
   }
   printf("\n");
#endif

/*
 * If here, then we've gotten a 'WORD' token.  Run this through state machine to parse the packet.
 */

   switch(tag) {

      case TRACE_HERE:
	 hdr("HERE");
	 lookup("Addr");
	 break;

      case TRACE_MISC_TAG:
	 hdr("MISC");
	 val = next();
	 printf("val: %d (0x%x) ", val, val);
	 break;

      case TRACE_TRIGGER:
	 hdr("TRIGGER");
	 from();
	 break;

      default:
	 printf("Unknown tag: %x ", tag);
	 tag_find(tag);
	 break;
      } // switch

      printf("\n");

      if (__argc_used < __argc_available) {
	 printf("ERROR: did not consume all of packet data: used: %d received: %d \n", __argc_used, __argc_available);
      }
}

#if 0
      static void
   cpsr_parse (void)
{
   unsigned int
      cpsr = next(),
      m;

   printf("cpsr: %x ", cpsr);

   m = (cpsr & 0x1f);
   printf("M: %x ", m);

   if (m == 0x10) { printf("(USR) "); }
   if (m == 0x12) { printf("(IRQ) "); }
   if (m == 0x13) { printf("(SUP) "); }
   if (m == 0x17) { printf("(ABT) "); }
   if (m == 0x1f) { printf("(SYS) "); }

   if (cpsr & (1 << 5)) { printf("T "); }
   if (cpsr & (1 << 6)) { printf("F "); }
   if (cpsr & (1 << 7)) { printf("I "); }
}
#endif


static void
   lookup (char *str)
{
   char cmd[512];
   char *cp;
   char lookup[1024];
   unsigned int address;
   unsigned int len;
   FILE *fp;

   address = next();

   printf("%s: 0x%x -- ", str, address);

   return;

   sprintf(cmd, "lookup 0x%x", address);

//	    printf ("%s", cmd);

   fp = popen(cmd, "r");
   fgets(lookup, sizeof(lookup), fp);
   pclose(fp);

   cp = strtok(lookup, " ");
   cp = strtok(NULL, " ");
   printf("Line %s, ", cp);

   cp = strtok(NULL, "\"");
   cp = strtok(NULL, "\"");

   if (cp) {
      len = strlen(cp);

      if (len > 50) {
	 cp += (len-50);
      }

      printf ("%s", cp);
   }
}



#if 0
    elsif ($tag == 0xfe120006) {
	hdr("ZC_HANDLER");
	printf "handler: %x \n", &next();
    }
    elsif ($tag == 0xfe120007) {
	hdr("MPU");
	printf "idx: %d ", &next();
	printf "Base: %x ", &next();
	printf "Size: %s ", @size_strs[&next()];
	printf "Rasr: %x \n", &next();

    }

    elsif ($tag == 0xfe12000d) {
	hdr("MPU_CONFIGURE");
	printf "regions: %x ", &next();
	printf "regions_num: %d ", &next();
	printf "start_reg_idx: %d ", &next();
	printf "do_sanity_check: %d \n", &next();
    }

#endif
