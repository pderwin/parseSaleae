#include <string.h>
#include "parseSaleae.h"
#include "parser.h"

#define MSB_FIRST 1

typedef struct {
    uint32_t scb1_clk;
    uint32_t scb1_cs0;
    uint32_t scb1_mosi;
    uint32_t panel_a0;
} sample_t;

static sample_t last_sample;
static sample_t sample;
static uint32_t accumulated_bits;
static uint32_t accumulated_byte;
static FILE     *log_file;
static parser_t my_parser;

static void process_frame (parser_t *parser, frame_t *frame)
{
    (void) parser;

    /*
     * If we've started a new packet, then clear accumulation.
     */
    if (falling_edge(scb1_cs0)) {
	accumulated_bits = 0;
	accumulated_byte = 0;
    }

    /*
     * Clear the accumulator at the start of a packet
     */
    if (rising_edge(scb1_clk)) {
	    static uint64_t last_time_nsecs = 0;
	    fprintf(log_file, "%lld   SCLK rise  delta: %lld\n", frame->time_nsecs, frame->time_nsecs - last_time_nsecs);
	last_time_nsecs = frame->time_nsecs;

	if (sample.scb1_cs0 == 0) {

	    //	    printf("SCB1.CLK rising edge\n");

#if (MSB_FIRST > 0)
	    accumulated_byte |= (sample.scb1_mosi << (7 - accumulated_bits));
#else
	    accumulated_byte |= (sample.scb1_mosi << accumulated_bits);
#endif
	    accumulated_bits++;

	    if (accumulated_bits == 8) {
		fprintf(log_file, ">>>>  SCB1 byte: %02x a0: %d \n", accumulated_byte, sample.panel_a0);

		accumulated_bits = 0;
		accumulated_byte = 0;
	    }
	}
    }
    memcpy(&last_sample, &sample, sizeof(sample_t));
}

static signal_t my_signals[] =
{
   { "scb1.clk",    &sample.scb1_clk },
   { "scb1.cs0",    &sample.scb1_cs0 },
   { "scb1.mosi",   &sample.scb1_mosi},
   { "panel_a0",    &sample.panel_a0},
   { NULL, NULL}
};

static parser_t my_parser =
    {
     .name          = "spi_device",
     .signals       = my_signals,
     .process_frame = process_frame,
     .log_file      = "spi_device.log",
    };

static void CONSTRUCTOR init (void)
{
    parser_register(&my_parser);
}
