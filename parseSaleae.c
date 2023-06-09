#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "csv.h"
#include "frame.h"
#include "hex.h"
#include "parseSaleae.h"
#include "tag.h"

static void
   process_input_file (char *inputFileName, unsigned int use_hex);

static unsigned int
   _show_time_stamps = 1;

int main(int argc, char *argv[])
{
    char
	*elf_file      = NULL,
	*inputFileName = NULL;
    unsigned int
	use_hex        = 0;

    //   setbuf(stdout, NULL);

    printf("parseSaleae v1.0\n");

    /*
     * Print wall-clock time.
     */
    localtime_print(stdout);

    {
	int c;

	static struct option long_options[] = {
					       {"elf",           required_argument, 0, 'e'},
					       {"hex",           0,                 0, 'x'},
					       {"help",          0,                 0,   0},
					       {"no-time-stamp", 0,                 0, 'n'},
					       {"tags",          0,                 0, 't'},
					       {0,               0,                 0,   0}
	};

	while (1) {
	    int option_index = 0;

	    c = getopt_long (argc, argv, "e:hntv:", long_options, &option_index);
	    if (c == -1)
		break;

	    switch (c) {

	    case 'e':
		elf_file = strdup(optarg);
		printf("\n");
		break;

	    case 'n':
		_show_time_stamps = 0;
		break;

	    case 't':
		tag_atexit();
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
   process_input_file (char *input_filename, unsigned int use_hex)
{
   FILE
      *fp;
   frame_t
      current_frame,
      next_frame;

   memset(&current_frame, 0, sizeof(current_frame));
   memset(&next_frame,    0, sizeof(next_frame));

   if ((fp = fopen(input_filename, "rb")) == NULL) {
      printf("Error opening input file: %s\n", input_filename);
      exit(1);
   }

   fprintf(stderr, "parseSaleae: processing: '%s'\n", input_filename);

   if (use_hex) {
      hex_process_file(fp);
   }
   else {
      csv_process_file(fp);
   }
}

uint32_t show_time_stamps (void)
{
    return _show_time_stamps;
}
