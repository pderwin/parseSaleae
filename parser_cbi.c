#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hdr.h"
#include "parseSaleae.h"
#include "parser.h"

typedef struct {
    uint32_t clk;
    uint32_t data;
    uint32_t ready;
    uint32_t valid;
    uint32_t sample_number;
} sample_t;

static sample_t last_sample;
static sample_t sample;

static parser_t my_parser;

static FILE *verify_fp;

#if 0
static uint32_t bit_flip (uint32_t val)
{
    uint32_t i;
    uint32_t rc = 0;

    for (i=0; i<8; i++) {
	if (val & (1 << i)) {
	    rc |= (1 << (7 - i));
	}
    }

    return rc;
}
#endif

/*-------------------------------------------------------------------------
 *
 * name:        process_frame
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static void process_frame (parser_t *parser, frame_t *frame)
{
    uint8_t  exp;
    uint32_t data;
    static uint32_t
	offset     = 0;

    (void) parser;
    (void) frame;

    /*
     * If dbg_packet is on, and we see a rising edge of clock, then sample the
     * data.
     */
    if (rising_edge(clk)) {

	//	printf("CBI CLK rdy: %d vld %d\n ", sample.ready, sample.valid);

	if (last_sample.ready && last_sample.valid) {

	    //	    data  = bit_flip(last_sample.data);
	    data  = last_sample.data;

	    // fprintf(my_parser.lf->fp, "Sample: %d Offset: %4x (%4d) :  %02x\n", (int32_t) sample.sample_number, offset, offset, data);
	    printf( "%d | CBI     | %4x |       %02x | %d   ", (int32_t) sample.sample_number, offset, data, offset);

	    if (fread(&exp, 1, 1, verify_fp) != 1) {
		fprintf(stderr, "Error reading verify file\n");
		exit(1);
	    }

	    if (exp != data) {
		printf("Error Verifying CBI Got: %02x Exp: %02x Xor: %02x ", last_sample.data, exp, last_sample.data ^ exp);
	    }

	    printf("\n");
	    /*
	     * We only see one byte of every 4 transferred
	     */
	    offset++;
	}
    }

    memcpy(&last_sample, &sample, sizeof(sample_t));
}


static signal_t my_signals[] =
{
   { "Sample Number",       &sample.sample_number, CSV_FORMAT_DECIMAL},
   { "DBG_FT60X_CBI_DATA",  &sample.data    },
   { "DBG_FT60X_CBI_READY", &sample.ready },
   { "DBG_FT60X_CBI_VALID", &sample.valid },
   { "DBG_CPU_CLK",         &sample.clk },
   { NULL, NULL}
};

static parser_t my_parser =
{
   .name              = "cbi",
   .process_frame     = process_frame,
   .signals           = my_signals,
//   .log_file          = "cbi.log",

};

static void CONSTRUCTOR init (void)
{
    parser_register(&my_parser);

    verify_fp = fopen("/home/erwin/jake-v/tools/print_test/ff.4bpp", "r");

    if (verify_fp == NULL) {
	printf("Error opening ff.4bpp for input\n");
	exit(1);
    }

}
