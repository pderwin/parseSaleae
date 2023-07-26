#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/drivers/trace.h>  // tag enumeration from Zephyr sources.
#include "hdr.h"
#include "parseSaleae.h"
#include "tag.h"

static void packet_dump (FILE *fp);

static uint32_t     next_index;
static uint32_t     packet_buffer[32];
static uint32_t     packet_buffer_count;
static time_nsecs_t packet_buffer_start_time;




#define NUMBER_TAG_STRINGS (128)

static char         *tag_strs[NUMBER_TAG_STRINGS];
static uint32_t      tag_strs_count;

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
	exit(1);
    }

    rc = packet_buffer[next_index];
    next_index++;

    return rc;
}

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
    uint32_t
	lineno,
	lineno_tag,
	magic,
	ra,
	tag,
	val;
    DECLARE_LOG_FP;

    /*
     * The first item should be a magic word.
     */
    magic = next();

    if ((magic & 0xffffff) != TRACE_MAGIC) {
	fprintf(log_fp, "Packet missing magic word: %x (TRACE_MAGIC: %x XOR: %08x) \n", magic, TRACE_MAGIC, magic ^ TRACE_MAGIC);
	packet_dump(log_fp);
	return;
    }

    //    printf(" %5d |", next() );

    /*
     * Get line number
     */
    lineno_tag = next();

    lineno = (lineno_tag >> 16) & 0xffff;
    tag    = (lineno_tag >>  0) & 0xffff;

    hdr_with_lineno(parser, packet_buffer_start_time, "", "PARSER_LBTRACE", lineno);

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

	fprintf(log_fp, "%-20.20s | ", tag_str);
    }

    switch (tag) {


    case TAG_MISC:
	val = next();
	fprintf(log_fp, "val: 0x%x  %d", val, val);
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

	/* ------ optional tags -------- */

    case TAG_ARBITER_ENTER:
	fprintf(log_fp, "ARBITER_ENTER");
	break;
    case TAG_ARBITER_SELECT:
	fprintf(log_fp, "ARBITER_SELECT");
	break;
    case TAG_TASK_ENQUEUE:
	fprintf(log_fp, "ENQUEUE");
	break;
    case TAG_SELECT_NEXT_ENTRY:
	fprintf(log_fp, "SELECT_NEXT_ENTRY");
	break;
    case TAG_SELECTED:
	fprintf(log_fp, "SELECTED");
	break;
    case TAG_DELAY_MS:
	fprintf(log_fp, "DELAY_MS");
	break;
    case TAG_ALARM_IN_MS:
	fprintf(log_fp, "ALARM_IN_MS");
	break;


    default:
	fprintf(log_fp, "Unparsed tag %x\n", tag);

	tag -= TAG_ZERO;
	if (tag < tag_strs_count) {
	    fprintf(log_fp, "Appears to be: '%s'\n", tag_strs[tag]);
	}
	fprintf(log_fp, "\n");

	packet_dump(log_fp);
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
	packet_buffer_start_time = time_nsecs;
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
    DECLARE_LOG_FP;

    parse_packet(parser);

    fprintf(log_fp, "\n");


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
void lbtrace_tag_scan()
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
static void packet_dump (FILE *log_fp)
{
    uint32_t
	i;

    fprintf(log_fp, "\n\nPacket: \n");

    for (i=0; i < packet_buffer_count; i++) {
	fprintf(log_fp, "   %d = %x\n", i, packet_buffer[i]);
    }
}
