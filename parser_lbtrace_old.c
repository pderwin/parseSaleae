#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <trace.h>  // tag enumeration from Zephyr sources.
#include "hdr.h"
#include "parseSaleae.h"
#include "parser.h"

static uint32_t      packet_buffer[256];
static uint32_t      next_index;
static uint32_t      packet_buffer_count;
static time_nsecs_t  packet_buffer_start_time;

#define NUMBER_TAG_STRINGS (128)

static char         *tag_strs[NUMBER_TAG_STRINGS];
static uint32_t      tag_strs_count;

/*-------------------------------------------------------------------------
 *
 * name:        next
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static uint32_t next (void)
{
    uint32_t retval = -1;


    if (next_index < packet_buffer_count) {
	retval = packet_buffer[next_index];
	next_index++;
    }
    else {
	printf("  ERROR: attempt to read past packet length\n");
	fprintf(stderr, "  ERROR: attempt to read past packet length\n");
    }

    return retval;
}

/*-------------------------------------------------------------------------
 *
 * name:        next_available
 *
 * description: return whether data is available in buffer.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static uint32_t next_available (void)
{
    return next_index < packet_buffer_count ? 1 : 0;
}

#if 0
/*-------------------------------------------------------------------------
 *
 * name:        verify_image
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static void verify_image (uint32_t address, uint32_t data)
{
    static int  offset = 0;
    static FILE *fp = NULL;
    uint32_t exp;

    if (fp == NULL) {
	fp = fopen("/home/erwin/jake-v/tools/print_test/ff.4bpp", "r");

	if (fp == NULL) {
	    printf("Error opening ff.4bpp for input\n");
	    exit(1);
	}
    }

    /*
     * read the next 4 bytes from the file.
     */
    if (fread(&exp, sizeof(uint32_t), 1, fp) != 1) {
	printf("End of file reading verify file.\n");
	exit(1);
    }

    if (exp != data) {
	printf("\nVerify error at address: %x offset: %x got: %x exp: %x \n", address, offset, data, exp);
	//	exit(1);
    }

    offset += 4;
}
#endif

/*-------------------------------------------------------------------------
 *
 * name:        parse_packet
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static void parse_packet (void)
{
    char
	tag_str[64];
    uint32_t
	address,
	count,
	data,
	length,
	lineno,
	lineno_tag,
	magic,
	ra,
	tag,
	val;
    static uint32_t
	total_given_to_DMA = 0;

    {
	uint32_t
	    i;

	printf("OLD Packet: \n");

	for (i=0; i < packet_buffer_count; i++) {
	    printf("   %d = %x\n", i, packet_buffer[i]);
	}

    }

    next_index = 0;

    /*
     * The first item should be a magic word.
     */
    magic = next();

    if (magic != TRACE_MAGIC) {
	printf("Packet missing magic word: %x \n", magic);
	return;
    }

    //    printf(" %5d |", next() );

    /*
     * Get line number
     */
    lineno_tag = next();

    lineno = (lineno_tag >> 16) & 0xffff;
    tag    = (lineno_tag >>  0) & 0xffff;

    hdr_with_lineno(stdout_lf, packet_buffer_start_time, "PARSER_LBTRACE_OLD", lineno);

    if (tag < TAG_ZERO) {

	uint32_t i;

	sprintf(tag_str, "??? %x", tag);

	printf("\n\nPacket buffer:\n");
	for (i=0; i<packet_buffer_count; i++) {
	    printf("   >>> %d = %x\n", i, packet_buffer[i]);
	}
    }
    else {
	strcpy (tag_str, tag_strs[tag - TAG_ZERO]);
    }

    printf("%-20.20s | ", tag_str);

    switch (tag) {
    case TAG_DMA_AXI_DESCRIPTOR_SUBMIT:
	printf("dma_axi: %x ", next());
	printf("desc: %x ", next());
	printf("desc->dest: %x ", next());

	break;

    case TAG_PIP_BAND_GOT:
	printf("band: %x", next());
	break;

    case TAG_PIP_DMA_DONE:
	break;

    case TAG_PIP_BAND_WAIT:
	break;

    case TAG_DESC_WRITE:
	printf("dma_axi: %x ", next());
	printf("desc: %x ", next());
	printf("address: %x ", next());
	printf("length: %d (dec) ", length = next());

	total_given_to_DMA += length;
	printf("total_given_to_DMA: %d (dec) ", total_given_to_DMA);

	break;

    case TAG_BAND_RESET:
    case TAG_FT60X_DESC_WRITE:
    case TAG_VIDEO_DMA_ISR:
	break;

    case TAG_VIDEO_DMA_ISR_DESC:
	printf("desc: %x ", next());
	printf("config: %x ", next());
	printf("dest: %x ", next());
	printf("length: %x ", next());
	printf("next: %x ", next());
	break;

    case TAG_PAGE_DONE:
	printf("**** PAGE_DONE: ");
	printf("intr_status: %x ", next());
	break;

    case TAG_DMA_AXI_UPDATE_CHAIN:
	printf("DMA_AXI_UPDATE_CHAIN: ");
	printf("prev desc: %x ", next());
	printf("desc: %x ", next());
	break;

    case TAG_PAGE_TOP_BOT:
	printf("top: %d ", next());
	printf("bot: %d ", next());
	break;

    case TAG_HEX_DUMP:

	address = next();

	count = 0;
	while(next_available()) {
	    if (count == 0) {
		printf("\naddr: %x ", address);
	    }

	    printf("%08x ", data = next());

	    //	    verify_image(address, data);

	    address += 4;
	    count++;

	    if (count == 8) {
		count = 0;
	    }
	}

	break;

   case TAG_VIDEO_VSYNC_WAIT:
	printf("VIDEO_VSYNC_WAIT: ");
	break;

    case TAG_VIDEO_VSYNC_WAIT_DONE:
	printf("*** VSYNC *** ");
	break;


    case TAG_VIDEO_DMA_ISR_BAND:
	printf("band: %x ", next());
	break;

    case TAG_DMA_AXI_REG_DUMP:
	printf("dma_axi: %x ", next());
	printf("CFG: %x ", next());
	printf("STATUS: %x ", next());
	printf("INT_EN: %x ", next());
	printf("INT_PEND: %x ", next());
	printf("DESC_READ: %x ", next());
	printf("XFER_LENGTH: %x ", next());
	printf("XFER_ADDRESS: %x ", next());
	break;
   case TAG_BAND_POOL_GET_DONE:
	printf("band_pool: %x ", next());
	break;
     case TAG_VIDEO_BAND_WAIT:
	//	printf("band_pool: %x ", next());
	break;
     case TAG_VIDEO_BAND_GOT:
	printf("band_pool: %x ", next());
	break;
    case TAG_BAND_POOL_GET_WAIT:
	printf("band_pool: %x ", next());
	printf("band_pool->count: %d ", next());
	break;
   case TAG_DMA_AXI_CONFIG:
	printf("regs_base: %x ", next());
	break;

    case TAG_DMA_AXI_ISR:
	printf("DMA_AXI_ISR: ");
	printf("dma_axi: %x ", next());
	printf("int_pend: %x ", next());
	break;

    case TAG_DMA_AXI_START:
	printf("DMA_AXI_START: ");
	printf("dma_axi: %x ", next());
	printf("regs_base: %x ", next());
	printf("desc: %x ", next());
	break;

   case TAG_BAND_CREATE:
	printf("BAND_CREATE: ");
	printf("band: %x ", next());
	printf("address: %x ", next());
	printf("width: %d (dec) ", next());
	printf("height: %d (dec) ", next());
	break;

    case TAG_BAND_POOL_GET:
	printf("pool: %x ", next());
	printf("band: %x ", next());
	printf("band_pool->count: %d ", next());
	break;

    case TAG_BAND_POOL_PUT:
	printf("pool: %x ", next());
	printf("band: %x ", next());
	printf("band_pool->count: %d ", next());
	break;

    case TAG_FT60X_BAND_POOL:
	printf("pool: %x ", next());
	printf("scanline_width_bytes: %d ", next());
	printf("height: %d ", next());
	break;

    case TAG_FT60X_ISR:
	break;
    case TAG_FT60X_ISR_BAND:
	printf("band: %x ", next());
	printf("address: %x ", next());
	break;

    case TAG_FT60X_VIDEO_START:
	printf("scanline_width_pixels: %d ", next());
	printf("height: %d ", next());
	break;

    case TAG_UART_CALLBACK:
	printf("val: %x", next());
	break;
    case TAG_UART_FIFO_READ:
	printf("val: %x", next());
	break;

    case TAG_MISC:
	val = next();
	printf("val: 0x%x  %d", val, val);
	break;

    case TAG_TRIGGER:
	printf("*** TRIGGER ***");
	printf("from: %x", next());
	break;

    case TAG_HERE:
	ra = next();
	printf("ra: %x  ", ra );
	lookup(ra);
	break;

    case TAG_TICK:
	break;

   case TAG_PRINTHEAD_WAIT_HSYNC:
	break;

   case TAG_PRINTHEAD_WAIT_HSYNC_DONE:
	printf("count: %d ", next());
	break;

    default:
	printf("Unparsed tag %x ", tag);

	tag -= TAG_ZERO;
	if (tag >= 0 && tag < tag_strs_count) {
	    printf("Appears to be: '%s'\n", tag_strs[tag]);
	}
	printf("\n");
    }
}



/* ============================================================ */


typedef struct {
    uint32_t dbg_clk;
    uint32_t dbg_lbt;
    uint32_t dbg_pkt;
    uint32_t dbg_trg;
} sample_t;

static sample_t last_sample;
static sample_t sample;
static uint32_t accumulated_bits;
static uint32_t accumulated_byte;
static parser_t my_parser;

/*
 * packet sample frequency to support filtering of i2c_clk signal.
 */
#define SAMPLE_TIME_NSECS (25)

/*
 * Set the time for de-glitch
 */
#define DEGLITCH_TIME (5 * SAMPLE_TIME_NSECS)


/*-------------------------------------------------------------------------
 *
 * name:        got_byte
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static void got_word(time_nsecs_t time_nsecs, uint32_t word)
{
    if (packet_buffer_count >= sizeof(packet_buffer)) {
	printf("Packet too long\n");
	exit(1);
    }

    if (packet_buffer_count == 0) {
	packet_buffer_start_time = time_nsecs;
    }

    packet_buffer[packet_buffer_count] = word;
    packet_buffer_count++;
}

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
static void process_frame (frame_t *frame)
{
    /*
     * Clear the accumulator at the start of a packet
     */

    if (rising_edge(dbg_pkt)) {
	accumulated_bits = 0;
	accumulated_byte = 0;
    }

    /*
     * Parse the accumulated bytes at the end of a packet
     */
    if (falling_edge(dbg_pkt)) {

	if (packet_buffer_count) {

	    parse_packet();

	    /*
	     * If data left over, then parse error.
	     */
	    if (next_index != packet_buffer_count) {
		printf("ERROR: %d word(s) left in buffer\n", packet_buffer_count - next_index);
		//		exit(1);
	    }
	    /*
	     * empty the packet buffer
	     */
	    packet_buffer_count = 0;
	}
    }

    if (sample.dbg_pkt == 0) {
	    return;
    }

    /*
     * debounce of the dbg_clk wire.  Seeing a slow falling edge on newer cards.
     */
    {
	static uint32_t current_dbg_clk = 0;
	static uint32_t countdown       = 0;
	static uint32_t first_frame     = 1;

	/*
	 * To keep from thinking we have a rising edge of clock at the beginning of debounce, we need to set our
	 * 'current' to our first entry.
	 */
	if (first_frame) {
	    current_dbg_clk = last_sample.dbg_clk = sample.dbg_clk;
	    first_frame = 0;
	    return;
	}

	/*
	 * The crisp clock is essential to our parsing ability.  Filter it for 250
	 * nSecs.
	 *
	 * When it changes states, set counter and just return.
	 */
	if (sample.dbg_clk != current_dbg_clk) {

	    if (countdown == 0) {
		countdown = DEGLITCH_TIME / SAMPLE_TIME_NSECS;
		return;
	    }
	    else {
		/*
		 * Here, we already have a countdown value, so if we decrement it and
		 * it becomes 0, then we can continue.  When non-zero, just return.
		 */
		if (--countdown) {
		    return;
		}
	    }

	    /*
	     * If we get here, then we have debounced it.  Remember current state.
	     */
	    current_dbg_clk = sample.dbg_clk;

	} // sample != current
    }








    /*
     * If dbg_packet is on, and we see a rising edge of clock, then sample the
     * data.
     */
    if (rising_edge(dbg_clk)) {

	if (sample.dbg_pkt) {

	    accumulated_byte |= (sample.dbg_lbt << (31 - accumulated_bits));
	    accumulated_bits++;

	    if (accumulated_bits == 32) {

		got_word(frame->time_nsecs, accumulated_byte);

		accumulated_bits = 0;
		accumulated_byte = 0;
	    }

	}
    }

    memcpy(&last_sample, &sample, sizeof(sample_t));
}

/*-------------------------------------------------------------------------
 *
 * name:        connect
 *
 * description: finish initialization when this parser is enabled.
 *
 * input:
 *
 * output:      1 - keep the parser enabled
 *
 *-------------------------------------------------------------------------*/
static uint32_t connect (void)
{
    tag_strs_count = tag_scan(ARRAY_SIZE(tag_strs), tag_strs);
    return 1;
}

static signal_t my_signals[] =
{
   { "dbg_clk", &sample.dbg_clk },
   { "dbg_lbt", &sample.dbg_lbt },
   { "dbg_pkt", &sample.dbg_pkt },
   { "dbg_trg", &sample.dbg_trg },
   { NULL, NULL}
};

static parser_t my_parser =
{
   .name              = "lbtrace_old",
   .process_frame     = process_frame,
   .signals           = my_signals,
   .sample_time_nsecs = SAMPLE_TIME_NSECS,
   .connect           = connect
};

static void CONSTRUCTOR init (void)
{
    parser_register(&my_parser);
}
