#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/drivers/trace.h>  // tag enumeration from Zephyr sources.
#include "hdr.h"
#include "lookup.h"
#include "parseSaleae.h"
#include "tag.h"
#include "thread.h"

static void packet_dump (void);

#define RTC_TICK_TIME_MSECS (1000.0 / 32768)
#define RTC_TICK_TIME_USECS (1.0 / 32768)

#define TICKS_TO_MSECS(__val) (__val * RTC_TICK_TIME_MSECS)

static uint32_t     next_index;
static uint32_t     packet_buffer[32];
static uint32_t     packet_buffer_count;

static time_nsecs_t packet_start_time_nsecs;
static uint32_t     packet_start_time_msecs;

#define NUMBER_TAG_STRINGS (128)

static char         *tag_strs[NUMBER_TAG_STRINGS];
static uint32_t      tag_strs_count;

static void from (parser_t *parser);
static void now (FILE *log_fp);

static FILE *log_fp;

/*-------------------------------------------------------------------------
 *
 * name:        next
 *
r
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static uint32_t next (void)
{
    uint32_t
	rc;

    if (next_index == packet_buffer_count) {
	fprintf(stderr, "Underrun parsing LBTRACE buffer\n");
	fprintf(log_fp, "Underrun parsing LBTRACE buffer\n");

	packet_dump();

	exit(1);
    }

    rc = packet_buffer[next_index];
    next_index++;

    return rc;
}

#if 0
static const char *radio_process_state_strs[] = {
   "IDLE",
   "SEND",
   "RX1",
   "RX2",
   "TX_WAIT",
   "INVALID",
   "ERROR",
   "DUTY_CYCLE_FULL",
   "BUSY",
};
#endif

static const char *task_type_strs[] = {
   "RX_LORA",
   "RX_FSK",
   "TX_LORA",
   "TX_FSK",
   "TX_LR_FHSS",
   "CAD",
   "CAD_TO_TX",
   "CAD_TO_RX",
   "GNSS_SNIFF",
   "WIFI_SNIFF",
   "GNSS_",
   "WIFI_RSSI",
   "LBT",
   "USER",
   "NONE"
};

static const char *task_state_strs[] =
    {
     "SCHEDULE",
     "ASAP"
    };

#if 0
static const char *rp_task_state_strs[] =
    {
     "SCHEDULE",
     "ASAP",
     "RUNNING",
     "ABORTED",
     "FINISHED"
    };
#endif

#if 0
static const char *radio_state_strs[] = {
   "IDLE",
   "PENDING",
   "TX_ON",
   "TX_FINISHED",
   "RX_ON",
   "RX_FINISHED",
   "ABORTED_BY_RP"
};
#endif

#define MAX_THREAD_NAME (32)

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
static void parse_packet (parser_t *parser)
{
    char
	thread_name[ MAX_THREAD_NAME + 1 ];  // add 1 for NULL termination
    uint32_t
	addr,
	i,
	*ip,
	lineno,
	lineno_tag,
	magic,
	ra,
	tag,
	val;
    thread_t
	*thread;

    DECLARE_LOG_FP;

    /*
     * The first item should be a magic word.
     */
    magic = next();

    if ((magic & 0xffffff) != TRACE_MAGIC) {
	fprintf(log_fp, "Packet missing magic word: %x (TRACE_MAGIC: %x XOR: %08x) \n", magic, TRACE_MAGIC, magic ^ TRACE_MAGIC);
	packet_dump();
	return;
    }

    //    printf(" %5d |", next() );

    /*
     * Get line number
     */
    lineno_tag = next();

    lineno = (lineno_tag >> 16) & 0xffff;
    tag    = (lineno_tag >>  0) & 0xffff;


    {
	char
	    buf[32],
	    *tag_str;

	if (tag < TAG_ZERO) {
	    sprintf(buf, "??? %x", tag);
	    tag_str = buf;
	}
	else {
	    tag_str = tag_strs[tag - TAG_ZERO];
	}

	hdr_with_lineno(parser, packet_start_time_nsecs, "", tag_str, lineno);

    }

    switch (tag) {


    case TAG_MISC:
	val = next();
	fprintf(log_fp, "val: 0x%x  %d", val, val);
	break;

    case TAG_SWAP_IN:
	addr = next();

	thread = thread_find(addr);

	fprintf(log_fp, "addr: 0x%x -- %s", addr, thread->name);
	break;

    case TAG_THREAD_NAME:

	/*
	 * Get thread address
	 */
	addr = next();

	/*
	 * Accumulate the name -- 32 bytes
	 */
	ip = (uint32_t *) thread_name;

	for (i=0; i<8; i++) {
	    ip[i] = next();
	}

	/*
	 * enforce a NULL termination
	 */
	thread_name[ MAX_THREAD_NAME ] = 0;

	/*
	 * Create the thread.
	 */
	thread_create(addr, thread_name);

	fprintf(log_fp, "addr: 0x%x -- %s", addr, thread_name);

	break;

    case TAG_TRIGGER:
	printf("*** TRIGGER ***");
	printf("from: %x", next());
	break;

    case TAG_HERE:
	ra = next();
	fprintf(log_fp, "ra: %x  ", ra );
	lookup(parser, ra);
	break;

	/* ------ optional tags -------- */

#if 0
    case TAG_CONFIGURE_RX_WINDOW:
	fprintf(log_fp, "t_current_ms: %d ", next());
	fprintf(log_fp, "talarm_ms: %d ", next());
	fprintf(log_fp, "rx_offset_ms: %d ", next());
	fprintf(log_fp, "delay_ms: %d ", next());
	fprintf(log_fp, "tx_done_irq: %d ", next());
	break;
#endif

#if 0
    case TAG_GPIO_ERROR:
	ra = next();
	fprintf(log_fp, "ra: %x  ", ra );

	lookup(parser, ra);
	break;
#endif

    case TAG_RADIO_PLANNER_LAUNCH_CURRENT:
	fprintf(log_fp, "id: %d ", next());
	addr = next();
	fprintf(log_fp, "callbacks: %x **************************************** ", addr);

	lookup(parser, addr);
	break;

    case TAG_RADIO_SET_RX:
	fprintf(log_fp, "timeout: %d ", next());
	from(parser);
	break;

    case TAG_SYS_CLOCK_START:
	break;

    case TAG_TASK_ABORT:
	fprintf(log_fp, "hook_id: %d ", next());
	now(log_fp);
	fprintf(log_fp, "start_time_ms: %d ", next());
	break;

    case TAG_TASK_ENQUEUE:
	fprintf(log_fp, "\n\t\t\t");
	fprintf(log_fp, "hook_id: %d\n\t\t\t", next());
	fprintf(log_fp, "payload: %x\n\t\t\t", next());
	fprintf(log_fp, "payload_size: %d\n\t\t\t", next());

	val = next();
	fprintf(log_fp, "task->state: %d (%s)\n\t\t\t", val, task_state_strs[val] );
	val = next();
	fprintf(log_fp, "task->type: %d (%s)\n\t\t\t", val, task_type_strs[val] );
	fprintf(log_fp, "task->start_time_ms: %d ", next());
	break;













    case TAG_MISSED_RX_WINDOW:
	fprintf(log_fp, "talarm: %d ", next());
	fprintf(log_fp, "rx_offset: %d ", next());
	break;

    case TAG_SET_TCXO:
	from(parser);
	break;

    case TAG_SET_DIO_AS_RF:
	from(parser);
	break;

    case TAG_SPI_CONTEXT_COMPLETE:
	break;

    case TAG_SPI_CONTEXT_CS_CONTROL:
	fprintf(log_fp, "on: %d ", next());
	fprintf(log_fp, "force_off: %d ", next());
	break;

    case TAG_NRFX_CLEAR_BITS:
    case TAG_NRFX_SET_BITS:
	fprintf(log_fp, "port: %x ", next());
	fprintf(log_fp, "mask: %x ", next());
	from(parser);
	break;

    default:
	fprintf(log_fp, "Unparsed tag %x\n", tag);

	tag -= TAG_ZERO;
	if (tag < tag_strs_count) {
	    fprintf(log_fp, "Appears to be: '%s'\n", tag_strs[tag]);
	}
	fprintf(log_fp, "\n");

	packet_dump();
	exit(1);
    }
}


/*-------------------------------------------------------------------------
 *
 * name:        lbtrace_packet_add_word
 *
 * description: add a word to the lbtrace packet
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void lbtrace_packet_add_word(parser_t *parser, time_nsecs_t time_nsecs, uint32_t word)
{
    DECLARE_LOG_FP;

    if (packet_buffer_count >= sizeof(packet_buffer)) {
	fprintf(log_fp, "Packet too long\n");
	exit(1);
    }

    if (packet_buffer_count == 0) {
	packet_start_time_nsecs = time_nsecs;
	packet_start_time_msecs = (packet_start_time_nsecs / 1000000);
    }

    packet_buffer[packet_buffer_count] = word;
    packet_buffer_count++;
}

/*-------------------------------------------------------------------------
 *
 * name:        lbtrace_parse_packet
 *
 * description: parse a single packet of LB trace data.
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void lbtrace_packet_parse (parser_t *parser)
{
    log_fp = parser->log_file->fp;

    parse_packet(parser);

    fprintf(log_fp, "\n");

    /*
     * Check for data left in packet
     */
    if (next_index != packet_buffer_count) {
	fprintf(log_fp, "ERROR: %d words left in buffer", packet_buffer_count - next_index);
	packet_dump();
	exit(1);
    }


    /*
     * Clear the packet buffer.
     */
    next_index          = 0;
    packet_buffer_count = 0;
}

/*-------------------------------------------------------------------------
 *
 * name:        lbtrace_tag_scan
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
void lbtrace_tag_scan (void)
{
    tag_strs_count = tag_scan(ARRAY_SIZE(tag_strs), tag_strs);
}

/*-------------------------------------------------------------------------
 *
 * name:        packet_dump
 *
 * description:
 *
 * input:
 *
 * output:
 *
 *-------------------------------------------------------------------------*/
static void packet_dump (void)
{
    uint32_t
	i;

    fprintf(log_fp, "\n\nPacket: \n");

    for (i=0; i < packet_buffer_count; i++) {
	fprintf(log_fp, "   %d = %x\n", i, packet_buffer[i]);
    }
}

static void from (parser_t *parser)
{
    uint32_t
	from_addr;
    DECLARE_LOG_FP;

    from_addr = next();
    fprintf(log_fp, "from: %x  ", from_addr );

    lookup(parser, from_addr);
}

static void now (FILE *log_fp)
{
    uint32_t now = next();

    fprintf(log_fp, "now: %d (msecs)", now);
}
