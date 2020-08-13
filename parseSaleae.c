#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "csv.h"
#include "parseSaleae.h"

static void
   process_input_file (char *inputFileName, unsigned int use_hex);

static unsigned int
   use_uart;

int
   main(int argc, char *argv[])
{
   char
      *elf_file      = NULL,
      *inputFileName = NULL;
   unsigned int
      use_hex        = 0;

   use_uart = 1;

//   setbuf(stdout, NULL);

   printf("parseSaleae v1.0\n");

   /*
    * Print wall-clock time.
    */
   localtime_print(stdout);

   {
      int c;

      static struct option long_options[] = {
	 {"elf",     required_argument, 0, 'e'},
	 {"hex",     0, 0, 'x'},
	 {"help",    0, 0,   0},
	 {"tags",    0, 0, 't'},
	 {0, 0, 0, 0}
      };

      while (1) {
	 int option_index = 0;

	 c = getopt_long (argc, argv, "e:htv:", long_options, &option_index);
	 if (c == -1)
	    break;

	 switch (c) {

	    case 'e':
	       elf_file = strdup(optarg);
	       printf("\n");
	       break;

	    case 't':
	       atexit(tag_dump);
	       break;

	    case 'x':
	       printf("Read HEX formatted file\n");
	       use_hex = 1;
	       break;

	    default:
	    case 'h':
	       printf("parseTrace - parse Trace and likely create a listing\n\n");
	       printf("Usage:\n");
	       printf("\tparseSaleae input_file_name \n\n");
	       printf("\tno-uart, n : don't print uart traffic\n");
	       exit(0);
	 }
      }
   }

   if (optind < argc) {
      inputFileName = argv[optind];
   }
   /*
    * Must clear this due to getopt() being called in GDB routines.
    */
   optind = 0;

   if (elf_file) {
      addr2line_init(elf_file);
      lookup_init(elf_file);
   }

   printf("input file: '%s' \n\n", inputFileName);

   /*
    * Open and process the input file.
    */
   process_input_file(inputFileName, use_hex);

   exit(0);
}

#if 0
static void
   findLbtrace(unsigned int idx, lbtrace_t *lbtrace)
{
   char
      name[128];
   unsigned int
      j = idx+1;

   /*
    * Use naming converntion of XXX_J1 and XXX_J2 for index 0 and 1.
    */
   sprintf(name, "d0_j%d", j);
   csv_find(name, &lbtrace->data0);

   sprintf(name, "d1_j%d", j);
   csv_find(name, &lbtrace->data1);

   sprintf(name, "clk_j%d", j);
   csv_find(name, &lbtrace->clk);

   sprintf(name, "trigger_j%d", j);
   csv_find(name, &lbtrace->trigger);

   sprintf(name, "misc_j%d", j);
   csv_find(name, &lbtrace->misc);
}
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
static void
   process_input_file (char *inputFileName, unsigned int use_hex)
{
   FILE
      *fp;
   frame_t
      current_frame,
      next_frame;

   memset(&current_frame, 0, sizeof(current_frame));
   memset(&next_frame,    0, sizeof(next_frame));

   if ((fp = fopen(inputFileName, "rb")) == NULL) {
      printf("Error opening input file: %s\n",
	     inputFileName);
      exit(1);
   }

   if (use_hex) {
      hex_process_file(fp);
   }
   else {
      csv_process_file(fp);
   }
}
