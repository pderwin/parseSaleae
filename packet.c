#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <trace.h>
#include "hdr.h"
#include "parseSaleae.h"


// #define DEBUG 1

#define CPSR 0

#if CPSR
static void
   cpsr_parse (void);
#endif

#define HDR(str) hdr_with_lineno(stdout_lf, idx, time_nsecs, str, lineno)

// static void syscall_lookup (unsigned int id);

static unsigned int
   __argc_used,
   __argc_available;
static unsigned int
   *__argv;

#if 0
static void parse_clock_gates (unsigned int val)
{
   char *strs[] = {
      "0  - ?",
      "1  - ?",
      "2  - Security",
      "3  - eFuse",
      "4  - DAP",
      "5  - SYS",
      "6  - PLL0",
      "7  - PLL1",
      "8  - BLDC",
      "9  - Motors",
      "10 - TRNG",
      "11 - PWM",
      "12 - Fuser",
      "13 - Laser Control",
      "14 - ADC",
      "15 - GPIO",
      "16 - qspi0",
      "17 - qspi1",
      "18 - I2C0",
      "19 - I2C1",
      "20 - Uart0",
      "21 - Uart1",
      "22 - Uart2",
      "23 - Reserved",
      "24 - Video0",
      "25 - Video1",
      "26 – Video R",
      "27 – Video R",
      "28 – Video R",
      "29 – Video R",
      "30 – Video R",
      "31 – Video R"
   };

   unsigned int
      i;

   for (i=0; i< 32; i++) {
      if (val & (1 << i)) {
	 printf("\n\t\t\t%s", strs[i]);
      }
   }
}
#endif

#if 0
static unsigned int
   next(void)
{
   if (__argc_used < __argc_available) {
      return(__argv[__argc_used++]);
   }

   printf("ERROR: attempt to read past end of available packet data.  Received: %d \n", __argc_available);

   return(0);
}
#endif

//static void
//   lookup (char *str);

#if 0
static void
   from (void)
{
   lookup("From");
}
#endif

#if 0
static const char * const region_size_strs[] = {
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
#endif

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
   packet_process (unsigned int idx, time_nsecs_t time_nsecs,
		   unsigned int tag, unsigned int lineno, unsigned int argc, unsigned int argv[])
{
    //   unsigned int
    //      val;

   __argc_used = 0;
   __argc_available = argc;
   __argv      = argv;


#ifdef DEBUG
   printf("%10lld: ", time_nsecs);
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

#if 0
   case TRACE_MISC_TAG:
       hdr("MISC");
       val = next();
       printf("val: %d (0x%x) ", val, val);
       addr2line(val);

       parse_clock_gates(val);

       break;

   case TRACE_HERE:
       hdr("HERE");
       lookup("Addr");
       break;

   case TRACE_SP_:
       hdr("SP");
       printf("Sp: %x ", next());
       printf("from: %x", next());
       break;

   case TRACE_FROM:
       hdr("FROM");
       from();
       break;

   case TRACE_DATA_ABORT:
       hdr("DATA_ABORT");
       printf("dfar: %x ", next());
       printf("dfsr: %x ", next());

       fprintf(stderr, "!!! DATA ABORT\n");
       break;

   case TRACE_PREFETCH_ABORT:
       hdr("PREFETCH_ABORT");
       printf("ifar: %x ", next());
       printf("ifsr: %x ", next());

       fprintf(stderr, "!!! PREFETCH ABORT\n");
       break;

   case TRACE_SP_TAG:
       hdr("SP");
       printf("Sp: %x ", next());
       printf("from: %x", next());
       break;

   case TRACE_FROM:
       hdr("FROM");
       from();
       break;

   case TRACE_TRIGGER_TAG:
       fprintf(stderr, "*** TRIGGER ***\n");
       hdr("*** TRIGGER ***");
       from();
       break;

   case TRACE_SWAP_IN:
       hdr("SWAP_IN");
       printf("Current: %x ", next());
       break;

   case TRACE_SYSCALL_ENTER:
       {
	   int id;
	   hdr("SYSCALL");
	   printf("id: %x ", id = next());
	   printf("pc: %x ", next());
       }
   case TRACE_SYSCALL_ENTRY:
       printf("\n");
       hdr("SYSCALL_ENTRY");
       printf("func: %x ", next());
       printf("r0: %x ", next());
       printf("r1: %x ", next());
       printf("r2: %x ", next());
       break;

   case TRACE_SYSCALL_EXIT:
       hdr("SYSCALL_EXIT");
       printf("(FPU disabled) ");
       printf("\n");
       break;

   case TRACE_ISR_ENTER:
       hdr("ISR_ENTER");
       lookup("func");
       break;
   case TRACE_ISR_EXIT:
       hdr("ISR_EXIT");
       printf("(FPU disabled) ");
       break;

   case TRACE_UNDEF_ABORT:
       hdr("*** UNDEF ABORT +++ ");
       break;

   case TRACE_INVALID_MPU:
       hdr("INVALID_MPU");
       printf("static: %d ", next());
       printf("count: %d ", next());
       break;
#endif
#if 0
   case TRACE_REGION_INIT:
       hdr("REGION_INIT");
       printf("index: %d ", next());
       printf("base: %8x ", next());

       size = next();
       size_str_idx = (size >> 1) & 0x1f;

       printf("size: %4x (%s) ", size, region_size_strs[size_str_idx]);
       printf("attr: %x ", next());
       break;
#endif

#if 0
   case TRACE_SYSCALL_OOPS:
       hdr("SYSCALL_OOPS");
       from();
       break;

   case TRACE_PRINTK:
       hdr("PRINTK");
       printf("fmt: %x ", next());
       break;
   case TRACE_PRINTK_DONE:
       hdr("PRINTK_DONE");
       break;

   case TRACE_DO_SYSCALL:
       hdr("DO_SYSCALL");
       printf("pc: %x ", next());
       printf("spsr: %x ", next());
       break;

#endif

   default:
       printf("Unknown tag: %x ", tag);
       fprintf(stderr, "Unknown tag: %x\n", tag);
       tag_find(tag);
       exit(12);
       break;
   } // switch

   printf("\n");

   if (__argc_used < __argc_available) {
       printf("ERROR: did not consume all of packet data: used: %d received: %d \n", __argc_used, __argc_available);
   }
}


#if CPSR
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


#if 0
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
    addr2line(address);

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
#endif


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

#if 0

static void syscall_lookup (unsigned int id)
{
   FILE
      *fp;
   static char
      buf[128],
      *symbol,
      *define = "#define K_SYS",
      *val_str;
   unsigned int
      define_len,
      val;

   fp = fopen("/home/erwin/auto-download/syscall_list.h", "r");

   if (fp == NULL) {
      return;
   }

   define_len = strlen(define);

   while(fgets(buf, sizeof(buf), fp)) {

      /*
       * If line does not start with the correct string, then ignore it.
       */
      if (strncmp(buf, define, define_len)) {
	 continue;
      }

      strtok(buf, " ");  // get the #define
      symbol = strtok(NULL, " "); // get symbol name
      val_str = strtok(NULL, " \n"); // get number

      val = atoi(val_str);

      if (val == id) {
	 printf("%s", symbol + 10);
	 break;
      }

   }

   fclose(fp);
}
#endif
